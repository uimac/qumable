/**
 * @file UMWSIO.cpp
 *
 * @author tori31001 at gmail.com
 *
 * Copyright (C) 2013 Kazuma Hatta
 * Licensed  under the MIT license. 
 *
 */
#include "UMWSIO.h"
#include "UMWSIOEventType.h"
#include "UMScene.h"
#include "UMEvent.h"
#include "UMCamera.h"
#include "UMNode.h"
#include "UMIO.h"
#include "UMIOSetting.h"
#include "UMObject.h"
#include "UMStringUtil.h"
#include "UMSoftwareIO.h"
#include <thread>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;

namespace umdraw
{
	class UMScene;
	typedef std::shared_ptr<UMScene> UMScenePtr;
} // umdraw

namespace umwsio
{
	using namespace umdraw;
	using namespace websocketpp;

static umio::UMMat44d to_umio(const UMMat44d& mat)
{
	umio::UMMat44d dst;
	for (int i = 0; i < 4; ++i)
	{
		for (int k = 0; k < 4; ++k)
		{
			dst.m[i][k] = mat.m[i][k];
		}
	}
	return dst;
}

class UMWSIO::Impl
{
public:
	typedef websocketpp::server<websocketpp::config::asio> server;

	Impl()
		: model_loaded_event_(std::make_shared<umbase::UMEvent>(eWSIOEventModelLoaded))
		, model_loading_event_(std::make_shared<umbase::UMEvent>(eWSIOEventModelLoading))
		, connect_event_(std::make_shared<umbase::UMEvent>(eWSIOEventConnect))
		, disconnecting_event_(std::make_shared<umbase::UMEvent>(eWSIOEventDisconnecting))
		, disconnect_event_(std::make_shared<umbase::UMEvent>(eWSIOEventDisconnect))
		, reconnect_event_(std::make_shared<umbase::UMEvent>(eWSIOEventReconnect))
		, is_loaded_(false)
		, is_importing_bone_(false)
		, is_start_reconnect_(false)
		, port_(9002)
	{}
	~Impl() 
	{
		if (thread_.joinable())
		{
			ioserver_.get_io_service().stop();
			thread_.join();
		}
	}

	void start_server(umdraw::UMScenePtr scene, int port)
	{
		port_ = port;
		scene_ = scene;
		thread_ = std::thread([this] { do_(); });
	}
	
	void done()
	{
		is_loaded_ = false;
	}

	void create_skeleton_data(std::string& binary)
	{
		UMNodeList::const_iterator it = scene_->node_list().begin();
		umio::UMIO io;
		umio::UMIOSetting setting;
		umio::UMObjectPtr obj = umio::UMObject::create_object();
		int dst_size = 0;
		for (; it != scene_->node_list().end(); ++it)
		{
			UMNodePtr node = *it;
			if (connection_map_.find(node) != connection_map_.end())
			{
				umio::UMSkeleton skeleton;
				UMMat44d global = node->mutable_global_transform();
				UMMat44d local_difference = node->mutable_local_transform() * node->mutable_initial_local_transform().inverted();
				if (node->parent())
				{
					skeleton.set_parent_id(node->parent()->id());
				}
				else
				{
					// root
					umbase::um_matrix_remove_scale(global, global);
					umbase::um_matrix_remove_scale(local_difference, local_difference);
				}
				skeleton.mutable_global_transform() = to_umio(global);
				skeleton.mutable_local_transform() = to_umio(local_difference);
				skeleton.set_id(node->id());
				skeleton.set_name(umbase::UMStringUtil::utf16_to_utf8(node->name()));
				obj->add_skeleton(skeleton);
			}
		}
		char* data = io.save_to_memory(dst_size, obj, setting);
		binary = std::string(data, dst_size);
		free( data );
	}

	bool import_skeleton_data(const std::string& data)
	{
		umio::UMIO io;
		umio::UMIOSetting setting;
		umio::UMObjectPtr obj = io.load_from_memory(data, setting);
		if (obj)
		{
			scene_->clear_geometry();
			if (umdraw::UMSoftwareIO::import_node_list(scene_->mutable_node_list(), UMMeshList(), obj))
			{
				return true;
			}
		}
		return false;
	}

	void on_message(server* s, websocketpp::connection_hdl hdl, server::message_ptr msg) 
	{
		//scoped_lock guard(lock_);
		const std::string& payload = msg->get_payload();
		//std::cout << payload << std::endl;
		if (payload == "get_pose")
		{
			std::string binary;
			create_skeleton_data(binary);
			websocketpp::lib::error_code ec;
			s->send(hdl, binary.c_str(), binary.size(), websocketpp::frame::opcode::binary, ec);
		}
		else if (payload == "start_mapping")
		{
			websocketpp::lib::error_code ec;
			s->send(hdl, "start_mapping_done", websocketpp::frame::opcode::text, ec);
			is_importing_bone_ = true;
		}
		else if (payload == "connect")
		{
			websocketpp::lib::error_code ec;
			nnb_ = std::string("");
			connect_event()->notify();
			if (!nnb_.empty())
			{
				s->send(hdl, nnb_.c_str(), websocketpp::frame::opcode::text, ec);
			}
			else
			{
				s->send(hdl, "connect_failed", websocketpp::frame::opcode::text, ec);
			}
		}
		else if (payload == "disconnect")
		{
			websocketpp::lib::error_code ec;
			nnb_ = std::string("");
			disconnecting_event()->notify();
			disconnect_event()->notify();
			s->send(hdl, "disconnect_done", websocketpp::frame::opcode::text, ec);
		}
		else if (payload == "start_reconnect")
		{
			websocketpp::lib::error_code ec;
			s->send(hdl, "start_reconnect_done", websocketpp::frame::opcode::text, ec);
			is_start_reconnect_ = true;
		}
		else if (is_start_reconnect_)
		{
			websocketpp::lib::error_code ec;
			nnb_ = payload;
			reconnect_event()->notify();
			if (!nnb_.empty())
			{
				s->send(hdl, "import_mapping_data_done", websocketpp::frame::opcode::text, ec);
			}
			else
			{
				s->send(hdl, "import_mapping_data_failed", websocketpp::frame::opcode::text, ec);
			}
			is_start_reconnect_ = false;
		}
		else if (is_importing_bone_)
		{
			websocketpp::lib::error_code ec;
			model_loading_event()->notify();
			if (import_skeleton_data(payload))
			{
				model_loaded_event()->notify();
				s->send(hdl, "import_bone_done", websocketpp::frame::opcode::text, ec);
			}
			else
			{
				s->send(hdl, "import_bone_failed", websocketpp::frame::opcode::text, ec);
			}
			is_importing_bone_ = false;
		}
	}

	bool is_loaded() const
	{
		return is_loaded_;
	}
	
	umbase::UMEventPtr model_loaded_event()
	{
		return model_loaded_event_;
	}

	umbase::UMEventPtr model_loading_event()
	{
		return model_loading_event_;
	}
	
	umbase::UMEventPtr connect_event()
	{
		return connect_event_;
	}

	umbase::UMEventPtr disconnect_event()
	{
		return disconnect_event_;
	}
	
	umbase::UMEventPtr disconnecting_event()
	{
		return disconnecting_event_;
	}

	umbase::UMEventPtr reconnect_event()
	{
		return reconnect_event_;
	}

	void set_nnb(const std::string& nnb)
	{
		nnb_ = nnb;
	}

	const std::string& nnb() const
	{
		return nnb_;
	}
	
	void set_connection_map(const std::map<umdraw::UMNodePtr, std::string>& connections)
	{
		connection_map_ = connections;
	}
private:
	void do_()
	{
		ioserver_.init_asio();
		ioserver_.set_message_handler(bind(&UMWSIO::Impl::on_message, this, &ioserver_, _1, _2));
		ioserver_.listen(port_);
		ioserver_.start_accept();
		ioserver_.run();
	}
	websocketpp::lib::mutex lock_;
	server ioserver_;
	umdraw::UMScenePtr scene_;
	std::thread thread_;
	umbase::UMEventPtr model_loaded_event_;
	umbase::UMEventPtr model_loading_event_;
	umbase::UMEventPtr connect_event_;
	umbase::UMEventPtr disconnect_event_;
	umbase::UMEventPtr disconnecting_event_;
	umbase::UMEventPtr reconnect_event_;
	bool is_loaded_;
	bool is_importing_bone_;
	bool is_start_reconnect_;
	std::string nnb_;
	int port_;
	std::map<umdraw::UMNodePtr, std::string> connection_map_;
};

/**
 * constructor
 */
UMWSIO::UMWSIO()
	: impl_(new UMWSIO::Impl())
{
}

/**
 * destructor
 */
UMWSIO::~UMWSIO()
{
}

/**
 * init
 */
bool UMWSIO::init()
{
	return false;
}

/**
 * add umdraw scene
 */
bool UMWSIO::start_server(umdraw::UMScenePtr scene, int data)
{
	impl_->start_server(scene, data);
	return true;
}

/**
 * get model loaded event
 */
umbase::UMEventPtr UMWSIO::model_loaded_event()
{
	return impl_->model_loaded_event();
}

/**
 * get model loading event
 */
umbase::UMEventPtr UMWSIO::model_loading_event()
{
	return impl_->model_loading_event();
}

umbase::UMEventPtr UMWSIO::connect_event()
{
	return impl_->connect_event();
}

umbase::UMEventPtr UMWSIO::disconnect_event()
{
	return impl_->disconnect_event();
}

umbase::UMEventPtr UMWSIO::disconnecting_event()
{
	return impl_->disconnecting_event();
}

umbase::UMEventPtr UMWSIO::reconnect_event()
{
	return impl_->reconnect_event();
}

void UMWSIO::set_nnb(const std::string& nnb)
{
	impl_->set_nnb(nnb);
}

const std::string& UMWSIO::nnb() const
{
	return impl_->nnb();
}

bool UMWSIO::is_loaded() const
{
	return impl_->is_loaded();
}

void UMWSIO::done()
{
	impl_->done();
}

void UMWSIO::set_connection_map(const std::map<umdraw::UMNodePtr, std::string>& connections)
{
	impl_->set_connection_map(connections);
}

} // umwsio
