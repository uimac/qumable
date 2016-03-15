/**
 * @file UMQumable.cpp
 *
 * @author tori31001 at gmail.com
 *
 * Copyright (C) 2013 Kazuma Hatta
 * Licensed  under the MIT license. 
 *
 */
#include <GL/glew.h>
#include "UMQuma.h"
#include "UMCamera.h"

#include "UMStringUtil.h"
#include "UMPath.h"
#include "UMScene.h"
#include "UMNode.h"
#include "UMOpenGLNode.h"
#include "UMOpenGLIO.h"
#include "UMOpenGLDrawParameter.h"
#include "UMOpenGLCamera.h"
#include "UMOpenGLLight.h"

#include <map>
#include <fstream>
#include <string>
#include <iterator>
#include <iostream>
#include <tchar.h>
#include <thread>
#include <algorithm>
#include <functional>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/classification.hpp> // is_any_of
#include <boost/algorithm/string/split.hpp>

#ifdef _WIN32
	#include <windows.h>
	#include <mmsystem.h> //For timeGetTime()
	#include <process.h>
	#include <tchar.h>
	#include <Objbase.h>
#endif //WIN32

#include "QmPdkDll.h"

namespace qumable
{
	using namespace umdraw;
	
class MatrixInfo;
typedef std::shared_ptr<MatrixInfo> MatrixInfoPtr;

class ModelNode;
typedef std::weak_ptr<ModelNode> ModelNodeWeakPtr;
typedef std::shared_ptr<ModelNode> ModelNodePtr;
typedef std::vector<ModelNodePtr> ModelNodeList;

static UMMat44f to_float(const UMMat44d& src)
{
	UMMat44f dst;
	for (int i = 0; i < 4; ++i)
	{
		for (int k = 0; k < 4; ++k)
		{
			dst.m[i][k] = static_cast<float>(src.m[i][k]);
		}
	}
	return dst;
}

static UMMat44d to_double(const UMMat44f& src)
{
	UMMat44d dst;
	for (int i = 0; i < 4; ++i)
	{
		for (int k = 0; k < 4; ++k)
		{
			dst.m[i][k] = src.m[i][k];
		}
	}
	return dst;
}

enum MAPPING_MODE {	
	MAPPING_MODE_OFF		= 0,	//ボーンマッピングダイアログオープン中でない。ポーズコピー可能。
	MAPPING_MODE_START		= 1,	//ボーンマッピングダイアログオープン中。
	MAPPING_MODE_SUCCESS	= 2,	//ボーンマッピングダイアログ終了、マッピング成功、反映待ち。
	MAPPING_MODE_FAILURE	= 3,	//ボーンマッピングダイアログ終了、マッピング失敗、反映待ち。
};

/** 
 * node
 */
class ModelNode
{
public:
	static ModelNodePtr create() {
		ModelNodePtr node = std::make_shared<ModelNode>();
		node->self = node;
		return node;
	}
	ModelNodeWeakPtr self;
	
	int id;
	std::string name;

	ModelNodeList children;
	ModelNodeWeakPtr parent;

	// initial local matrix
	UMMat44f localmat;
	// rotation matrix from device.
	UMMat44f rotmat;

	UMNodePtr link_node;

	ModelNodePtr root()
	{
		ModelNodePtr p = parent.lock();
		if (!p) {
			return self.lock();
		}
		return p->root();
	}
};

/**
 * UMQuma implementation
 */
class UMQuma::Impl
{
public:
	Impl() {
		has_user_nodes_ = false;
		is_bone_mapping_done_ = false;
		is_loading_ = false;
		root_group_index_ = -1;
		scale_offset_.m[0][0] = scale_offset_.m[1][1] = scale_offset_.m[2][2] = 0.2;
		character_scale_offset_.m[0][0] = character_scale_offset_.m[1][1] = character_scale_offset_.m[2][2] = 20.0;
		//character_scale_offset_.m[0][0] = character_scale_offset_.m[1][1] = character_scale_offset_.m[2][2] = 200.0;
	}
	~Impl() {}
	
	bool init(umdraw::UMScenePtr scene)
	{
		scene_ = scene;
		return init_quma();
	}
	
	bool update();

	bool draw(UMCameraPtr camera);

	void finalize()
	{
		QmPdkFinal();
	}

	bool add_connection(UMNodePtr node, const std::string& name);

	bool connect();

	bool save_nnb(const std::u16string& path);

	bool save_nnb_to_memory(std::string& path);
	
	bool load_nnb(const std::u16string& path);

	bool load_nnb_from_memory(const std::string& buffer);

	bool apply_nnb();
	
	const std::map<umdraw::UMNodePtr, std::string>& get_connection_map() const
	{
		return connection_map_;
	}

private:
	bool init_quma();
	void version();
	bool load_quma_model();
	bool load_character();
	bool load_template();
	bool load_template_recursive(int node_index, ModelNodePtr parent);
	int get_node_index(umdraw::UMNodePtr node);
	ModelNodePtr get_template_node(const std::string& name);
	
	void create_hand_groups(ModelNodePtr tnode,
		std::map<int, int>& bone_id_to_group_index);
	
	void create_user_groups(
		const std::string& template_name, 
		std::vector<int>& group_bones,
		std::map<int, UMNodePtr> & group_bone_nodes);

	void create_template_groups(
		std::map<int, int>& bone_id_to_group_index,
		ModelNodePtr tnode,
		std::vector<int> & template_group_index,
		std::map<int, std::string> & template_group_names,
		int& parent_group_index);

	void create_connection_map_from_nnb(const std::string& buffer);
	void create_connection_map_from_nnb_recursive(
		boost::property_tree::ptree& pt, 
		const boost::property_tree::ptree::value_type& child,
		const std::string& buffer);

	static bool attach_quma_device(QmPdkModelHandle handle);
	static bool re_attach_quma_device(QmPdkModelHandle handle);
	static QmPdkModelHandle model_handle_;
	static bool is_bone_mapping_done_;
	std::vector<ModelNodePtr> template_node_list_;
	int root_group_index_;
	umdraw::UMScenePtr scene_;
	bool has_user_nodes_;
	bool is_loading_;
	UMMat44d scale_offset_;
	UMMat44d character_scale_offset_;
	typedef std::map<umdraw::UMNodePtr, std::string> ConnectionMap;
	ConnectionMap connection_map_;
};

QmPdkModelHandle UMQuma::Impl::model_handle_;
bool UMQuma::Impl::is_bone_mapping_done_(false);

/**
 * draw
 */ 
bool UMQuma::Impl::update()
{
	if (is_loading_) return true;
	QmPdkErrCode e;
	QmPdkQumaHandle quma_handle = QMPDK_QUMA_HANDLE_ERROR;
	QmPdkQumaButtonState state = QMPDK_QUMA_BUTTON_STATE_OFF;

	// QUMAデバイスの背面にあるボタンの状態を取得。
	// ボタンが押されていたらキャリブレーションを実行。
	e = QmPdkCharacterGetQumaHandle(
		model_handle_,
		&quma_handle );
	
	if (quma_handle != QMPDK_QUMA_HANDLE_ERROR) 
	{
		e = QmPdkQumaGetButtonState(
			quma_handle,
			QMPDK_QUMA_BUTTON_TYPE_0,
			&state );
	}
	
	if (e == QMPDK_ERR_CODE_DEVICE_UPDATEBUFFER) {
		//USB挿抜によってデバイスが無効になった。
		//printf("[OnIdle()] Device Error!\n");
		return false;
	}
	else if (state == QMPDK_QUMA_BUTTON_STATE_ON) {
		// キャリブレーションの実行。
		printf("[OnIdle()] Do Calibrateion\n");
		e = QmPdkCalibratePose( model_handle_ );
	}
	else
	{
		// ポーズの反映。
		e = QmPdkCopyPose( model_handle_ );
	
		//Characterモデルからユーザーモデルへポーズコピーの結果を取得
		// ユーザーモデル（すでにTポーズ）のマトリックスをCharacterボーンモデルにコピーする。
		if (scene_ && !scene_->node_list().empty())
		{
			if (is_bone_mapping_done_ || !has_user_nodes_)
			{
				{
					UMNodeList::iterator it = scene_->mutable_node_list().begin();
					for (int i = 0; it != scene_->mutable_node_list().end(); ++it, ++i)
					{
						UMMat44f local_mat;
						QmPdkCharacterGetLocalMatrix(
							model_handle_,
							static_cast<int>( i ),
							local_mat.m);

						UMNodePtr node = *it;
						node->mutable_local_transform() = to_double(local_mat);

						if (!node->parent())
						{
							if (has_user_nodes_)
							{
								node->mutable_local_transform() = to_double(local_mat) * character_scale_offset_ ;
							}
							else
							{
								node->mutable_local_transform() = to_double(local_mat) * scale_offset_;
							}
						}
					}
				}
			
				// global matrix 更新
				{
					UMNodeList::iterator it = scene_->mutable_node_list().begin();
					for (int i = 0; it != scene_->mutable_node_list().end(); ++it, ++i)
					{
						UMMat44d global_mat;
						UMNodePtr node = *it;
						for (UMNodePtr n = node; n; n = n->parent())
						{
							global_mat = global_mat * n->local_transform();
						}
						node->mutable_global_transform() = global_mat;
					}
				}
			}
		}
	}
	return true;
}

/**
 * draw
 */ 
bool UMQuma::Impl::draw(UMCameraPtr camera)
{
	return true;
}

/**
 * attach quma device
 */
bool UMQuma::Impl::attach_quma_device(QmPdkModelHandle handle)
{
	if (handle == QMPDK_MODEL_HANDLE_ERROR) {
		return false;
	}
	// get devicve count
	int device_count = 0;
	{
		QmPdkErrCode e = QmPdkQumaGetNumOfHandle(&device_count);
		if (e != QMPDK_ERR_CODE_NOERROR) {
			return false;
		}
		if (device_count <= 0) {
			return false;
		}
	}

	// get attached device handle
	QmPdkQumaHandle quma_handle = QMPDK_QUMA_HANDLE_ERROR;
	{
		QmPdkErrCode e = QmPdkCharacterGetQumaHandle(handle, &quma_handle);
		if (quma_handle == QMPDK_QUMA_HANDLE_ERROR) 
		{
			for (int i = 0; i < device_count; i++) {
				e = QmPdkQumaGetHandle( 0, &quma_handle );
				if (e != QMPDK_ERR_CODE_NOERROR) {
					return false;
				}
				// is valid device?
				e = QmPdkQumaGetDeviceState( quma_handle );
				if (e == QMPDK_ERR_CODE_NOERROR) {
					break; //success
				}
			}
		}
		else
		{
			// already attached, find next device
			QmPdkQumaHandle	pre_quma_handle = quma_handle;
			for (int quma_index = 0; quma_index < device_count; ++quma_index)
			{
				e = QmPdkQumaGetHandle( quma_index, &quma_handle );
				if (e == QMPDK_ERR_CODE_NOERROR
					&& pre_quma_handle == quma_handle)
				{
					// found
					quma_index++;
					if (quma_index >= device_count) {
						quma_index = 0;
					}
					e = QmPdkQumaGetHandle( quma_index, &quma_handle );
					if (e != QMPDK_ERR_CODE_NOERROR) {
						return false;
					}
					// device is available ?
					e = QmPdkQumaGetDeviceState(quma_handle);
					if (e != QMPDK_ERR_CODE_NOERROR) {
						continue;
					}
					break;
				}
			}
		}
	}
	
	// attach quma device to character
	{
		QmPdkErrCode e = QmPdkQumaAttachInitPoseModel(quma_handle, handle);
		if (e != QMPDK_ERR_CODE_NOERROR) {
			return false;
		}
	}
	return true;
}

/**
 * re attach quma device
 */
bool UMQuma::Impl::re_attach_quma_device(QmPdkModelHandle handle)
{
	if (handle == QMPDK_MODEL_HANDLE_ERROR) {
		return false;
	}

	//デバイス数の取得。
	int numDevice = 0;
	QmPdkErrCode e = QmPdkQumaGetNumOfHandle( &numDevice );
	if (e != QMPDK_ERR_CODE_NOERROR) {
		return false;
	}
	if ( numDevice <= 0) {
		return false;
	}

	//現在アタッチされたQUMAデバイスハンドルを取得。
	QmPdkQumaHandle	quma_handle = QMPDK_QUMA_HANDLE_ERROR;
	e = QmPdkCharacterGetQumaHandle( handle, &quma_handle );

	if (quma_handle == QMPDK_QUMA_HANDLE_ERROR) { //まだ何もアタッチされていない。
		return false;
	}

	//CharacterモデルへQUMAデバイスのアタッチ。
	e = QmPdkQumaAttachInitPoseModel(
		quma_handle,
		handle );
	if (e != QMPDK_ERR_CODE_NOERROR) {
		return false;
	}
	return true;
}


/**
 * load template recursive
 */
bool UMQuma::Impl::load_template_recursive(int node_index, ModelNodePtr parent)
{
	ModelNodePtr node = ModelNode::create();
	{
		char name[256];
		QmPdkErrCode e = QmPdkTemplateGetName(
			model_handle_,
			node_index,
			name,
			sizeof(name));
		if (e != QMPDK_ERR_CODE_NOERROR) {
			return false;
		}
		node->name.append(name);
		node->parent = parent;
		node->id = node_index;
		printf("create template : %d, %s\n", node_index, name);
		if (parent)
		{
			parent->children.push_back(node);
		}
		template_node_list_.push_back(node);
	}
	
	int child_index_count = 0;
	{
		QmPdkErrCode e = QmPdkTemplateGetChildNodeIdx(
			model_handle_,
			node_index,
			NULL,
			&child_index_count );
		if (e != QMPDK_ERR_CODE_NOERROR) {
			return false;
		}
		if (child_index_count == 0) {
			return true;
		}
	}

	std::vector<int> child_index_list(child_index_count);
	{
		QmPdkErrCode e = QmPdkTemplateGetChildNodeIdx(
			model_handle_,
			node_index,
			&(*child_index_list.begin()),
			&child_index_count );
		if (e != QMPDK_ERR_CODE_NOERROR) {
			return false;
		}
	}
	
	for (int i = 0; i < child_index_count; i++) {
		load_template_recursive(child_index_list.at(i), node);
	}
	return true;
}

/**
 * load template model
 */
bool UMQuma::Impl::load_template()
{
	int root_index = 0;
	QmPdkErrCode e = QmPdkTemplateGetRootNodeIdx(model_handle_, &root_index);
	if (QMPDK_ERR_CODE_NOERROR != e) {
		return false;
	}
	load_template_recursive(root_index, NULL);
	return true;
}

/**
 * load character model
 */
bool UMQuma::Impl::load_character()
{
	has_user_nodes_ = (scene_ && !scene_->node_list().empty());
	if (has_user_nodes_)
	{
		int node_count = static_cast<int>(scene_->node_list().size());
		std::vector<std::string> node_name_list;
		std::vector<int> parent_node_ids(node_count);
		for (int i = 0; i < node_count; ++i)
		{
			UMNodePtr node = scene_->node_list().at(i);
			if (node->parent())
			{
				parent_node_ids[i] = node->parent()->id();
			}
			else
			{
				parent_node_ids[i] = -1;
			}
			std::string name = umbase::UMStringUtil::utf16_to_utf8(node->name());
			node_name_list.push_back(name);
		}
		for (int i = 0; i < node_count; ++i)
		{
			UMNodePtr node = scene_->node_list().at(i);
			for (int k = 0; k < node_count; ++k)
			{
				if (parent_node_ids[k] == node->id())
				{
					parent_node_ids[k] = i;
				}
			}
		}

		std::cout << "nodenames" << std::endl;
		std::vector<const char*> c_node_name_list;
		for (int i = 0; i < node_count; ++i)
		{
			c_node_name_list.push_back(node_name_list.at(i).c_str());
			std::cout << i << ":" << node_name_list.at(i).c_str() << std::endl;
		}
		std::cout << "nodenames" << std::endl;

		QmPdkErrCode e = QmPdkCharacterCreate(&model_handle_,
									node_count,
									&parent_node_ids[0],
									&c_node_name_list[0] );
		if (e != QMPDK_ERR_CODE_NOERROR) {
			printf("[LoadCharacter()] Error! Could not create character.\n" );
			return false;
		}

		// set local matrix
		for (int i = 0; i < node_count; ++i)
		{
			UMNodePtr node = scene_->node_list().at(i);
			UMMat44f local = to_float(node->local_transform());

			QmPdkCharacterSetLocalMatrix(
				model_handle_,
				i, 
				local.m);
		}

		//現在のCharacterモデルのポーズをTポーズとして記憶させる。
		e = QmPdkCharacterMemorizeInitialPose( model_handle_ );
		if (e != QMPDK_ERR_CODE_NOERROR) {
			printf("[LoadCharacter()] Error! Could not MemorizeInitialPose.\n" );
			return false;
		}

		// スケールオフセット.
		UMNodeList::iterator it = scene_->mutable_node_list().begin();
		for (int i = 0; it != scene_->mutable_node_list().end(); ++it, ++i)
		{
			UMNodePtr node = *it;
			if (!node->parent())
			{
				node->mutable_local_transform() = node->local_transform() * character_scale_offset_ ;
			}
		}
		// global matrix 更新
		{
			UMNodeList::iterator it = scene_->mutable_node_list().begin();
			for (int i = 0; it != scene_->mutable_node_list().end(); ++it, ++i)
			{
				UMMat44d global_mat;
				UMNodePtr node = *it;
				for (UMNodePtr n = node; n; n = n->parent())
				{
					global_mat = global_mat * n->local_transform();
				}
				node->mutable_global_transform() = global_mat;
			}
		}
	}
	else
	{
		int node_count = 0;
		{
			QmPdkErrCode e = QmPdkCharacterCreateStandardModelPS(
				NULL,
				&node_count,
				NULL );
			if (e != QMPDK_ERR_CODE_NOERROR) 
			{
				printf("[LoadCharacterStandardModelPS()] Error! Could not create character.\n" );
				return false;
			}
		}
	
		std::vector<int> parent_node_ids(node_count);
		{
			QmPdkErrCode e = QmPdkCharacterCreateStandardModelPS(
				&model_handle_,
				&node_count,
				&(*parent_node_ids.begin()));
			if (e != QMPDK_ERR_CODE_NOERROR)
			{
				printf("[LoadCharacterStandardModelPS()] Error! Could not create character.\n" );
				return false;
			}
		}

		// create character nodes
		for (int i = 0; i < node_count; ++i) 
		{
			UMNodePtr node = std::make_shared<UMNode>();
			char node_name[256];

			QmPdkErrCode e = QmPdkCharacterGetName(
				model_handle_,
				i,
				node_name,
				sizeof(node_name),
				NULL,
				0);
			if (e != QMPDK_ERR_CODE_NOERROR)
			{
				return false;
			}
			node->set_name(umbase::UMStringUtil::utf8_to_utf16(node_name));

			{
				UMMat44f local;
				QmPdkErrCode e = QmPdkCharacterGetLocalMatrix(
					model_handle_, 
					i,
					local.m);
				node->mutable_local_transform() = to_double(local);
				QmPdkCharacterSetLocalMatrix(
					model_handle_,
					i, 
					local.m);
			}
			scene_->mutable_node_list().push_back(node);
		}
		// connect parent-child
		for (int i = 0; i < node_count; ++i)
		{
			int parent_id = parent_node_ids.at(i);
			if (parent_id >= 0)
			{
				UMNodePtr parent = scene_->mutable_node_list().at(parent_id);
				UMNodePtr child = scene_->mutable_node_list().at(i);
				child->set_parent(parent);
				parent->mutable_children().push_back(child);
			}
		}
		// calc global transform
		for (size_t i = 0; i < scene_->node_list().size(); ++i) 
		{
			UMMat44d global_mat;
			UMNodePtr node = scene_->mutable_node_list().at(i);
			for (UMNodePtr n = node; n; n = n->parent())
			{
				global_mat = global_mat * n->local_transform();
			}
			node->mutable_global_transform() = global_mat;
		}
	}
	return true;
}

/**
 * load quma model
 */
bool UMQuma::Impl::load_quma_model()
{
	is_loading_ = true;
	if (!load_character())
	{
		is_loading_ = false;
		return false;
	}
	is_loading_ = false;
	if (!load_template())
	{
		return false;
	}
	
	if (!attach_quma_device(model_handle_)) 
	{
		printf("[initquma()] Error! Failed to attach device.\n");
		return false;
	}
	return true;
}

bool UMQuma::Impl::add_connection(UMNodePtr node, const std::string& name)
{
	if (!has_user_nodes_) return false;
	
	connection_map_[node] = name;
	printf("%s to %s\n", umbase::UMStringUtil::utf16_to_utf8(node->name()).c_str(), name.c_str());
	return true;
}

int UMQuma::Impl::get_node_index(umdraw::UMNodePtr node)
{
	if (scene_)
	{
		const UMNodeList& node_list = scene_->node_list();
		UMNodeList::const_iterator it = std::find(node_list.begin(), node_list.end(), node);
		if (it != node_list.end())
		{
			const int node_index = static_cast<int>(std::distance(node_list.begin(), it));
			return node_index;
		}
	}
	return -1;
}

ModelNodePtr UMQuma::Impl::get_template_node(const std::string& name)
{
	if (!template_node_list_.empty())
	{
		for (int i = 0, size = static_cast<int>(template_node_list_.size()); i < size; ++i)
		{
			if (template_node_list_.at(i)->name == name)
			{
				return template_node_list_.at(i);
			}
		}
	}
	return ModelNodePtr();
}

//void UMQuma::Impl::create_hand_groups(ModelNodePtr tnode,
//	std::map<int, int>& bone_id_to_group_index)
//{
//	std::vector<int> group_bones;
//	std::map<int, UMNodePtr> group_bone_nodes;
//	for (ConnectionMap::iterator it = connection_map_.begin(); it != connection_map_.end(); ++it)
//	{
//		if (it->second == tnode->name)
//		{
//			UMNodePtr node = it->first;
//			const int node_index = get_node_index(node);
//			if (node_index >= 0)
//			{
//				UMNodePtr parent = it->first->parent();
//				if (parent)
//				{
//					const int parent_index = get_node_index(parent);
//					if (std::find(group_bones.begin(), group_bones.end(), parent_index) == group_bones.end())
//					{
//						group_bones.push_back(parent_index);
//						group_bone_nodes[parent_index] = parent;
//					}
//				}
//				group_bones.push_back(node_index);
//				group_bone_nodes[node_index] = node;
//			}
//		}
//	}
//	std::sort(group_bones.begin(), group_bones.end());
//	
//	UMNodePtr hand_root;
//	for (int i = 0, size = static_cast<int>(group_bone_nodes.size()); i < size; ++i)
//	{
//		UMNodePtr node = group_bone_nodes[group_bones.at(i)];
//		if (node->children().size() > 1) {
//			hand_root = node;
//			break;
//		}
//	}
//	if (!hand_root) return;
//
//	for (int i = 0, size = static_cast<int>(hand_root->children().size()); i < size; ++i)
//	{
//		UMNodePtr child_root = hand_root->children().at(i);
//		
//		std::vector<int> child_group_bones;
//		std::map<int, UMNodePtr> child_group_bone_nodes;
//		
//		for (int k = 0, ksize = static_cast<int>(child_root->children().size()); k < ksize; ++k)
//		{
//			UMNodePtr node = child_root->children().at(k);
//			int id = node->id();
//			child_group_bones.push_back(id);
//			child_group_bone_nodes[id] = node;
//		}
//		std::sort(child_group_bones.begin(), child_group_bones.end());
//		
//		std::vector<int> child_template_group_index;
//		std::map<int, std::string> child_template_group_names;
//		
//		int parent_group_index = root_group_index_;
//		ModelNodePtr parent = tnode->parent.lock();
//		if (!tnode->children.empty())
//		{
//			child_template_group_index.push_back(tnode->children.at(0)->id);
//			child_template_group_names[tnode->children.at(0)->id] = tnode->children.at(0)->name;
//		}
//		while (parent)
//		{
//			if (bone_id_to_group_index.find(parent->id) != bone_id_to_group_index.end())
//			{
//				parent_group_index = bone_id_to_group_index[parent->id];
//				break;
//			}
//			child_template_group_index.push_back(parent->id);
//			child_template_group_names[parent->id] = parent->name;
//			parent = parent->parent.lock();
//		}
//		
//		int added_group_index;
//		QmPdkErrCode e = QmPdkNnbAddGroup(
//			model_handle_,
//			parent_group_index,
//			static_cast<int>(child_group_bones.size()),
//			&(child_group_bones.at(0)),
//			static_cast<int>(child_template_group_index.size()),
//			&(child_template_group_index.at(0)),
//			&added_group_index );
//
//		if (e == QMPDK_ERR_CODE_NOERROR)
//		{
//			printf("connected--------------------------\n");
//			for (int k = 0, ksize = static_cast<int>(child_template_group_index.size()); k < ksize; ++k)
//			{
//				bone_id_to_group_index[child_template_group_index[k]] = added_group_index;
//				printf("%d : %s template\n",
//					child_template_group_index.at(k),
//					child_template_group_names[child_template_group_index.at(k)].c_str());
//			}
//			for (int k = 0, ksize = static_cast<int>(child_template_group_index.size()); k < ksize; ++k)
//			{
//				printf("%d : %s user\n",
//					child_group_bones.at(k), 
//					umbase::UMStringUtil::utf16_to_utf8(group_bone_nodes[child_group_bones.at(k)]->name()).c_str());
//			}
//			printf("----------------------------------\n");
//		}
//		else
//		{
//			printf("node connection error!\n");
//		}
//	}
//}

void UMQuma::Impl::create_user_groups(
	const std::string& template_name, 
	std::vector<int>& group_bones,
	std::map<int, UMNodePtr> & group_bone_nodes)
{
	for (ConnectionMap::iterator it = connection_map_.begin(); it != connection_map_.end(); ++it)
	{
		if (it->second == template_name)
		{
			UMNodePtr node = it->first;
			const int node_index = get_node_index(node);
			if (node_index >= 0)
			{
				UMNodePtr parent = it->first->parent();
				if (parent)
				{
					const int parent_index = get_node_index(parent);
					if (std::find(group_bones.begin(), group_bones.end(), parent_index) == group_bones.end())
					{
						group_bones.push_back(parent_index);
						group_bone_nodes[parent_index] = parent;
					}
				}
				group_bones.push_back(node_index);
				group_bone_nodes[node_index] = node;
			}
		}
	}
	std::sort(group_bones.begin(), group_bones.end());

	for (int k = static_cast<int>(group_bones.size() - 1); k >= 1; --k)
	{
		UMNodePtr parent = group_bone_nodes[group_bones.at(k - 1)];
		UMNodePtr node = group_bone_nodes[group_bones.at(k)];
		UMNodeList childs = parent->children();
		if (childs.empty() || std::find(childs.begin(), childs.end(), node) == childs.end())
		{
			group_bones.erase(group_bones.begin() + k);
			if (k < static_cast<int>(group_bones.size()))
			{
				++k;
			}
		}
	}
}

void UMQuma::Impl::create_template_groups(
	std::map<int, int>& bone_id_to_group_index,
	ModelNodePtr tnode,
	std::vector<int> & template_group_index,
	std::map<int, std::string> & template_group_names,
	int& parent_group_index)
{
	ModelNodePtr parent = tnode->parent.lock();
	template_group_index.push_back(tnode->id);
	template_group_names[tnode->id] = tnode->name;
	if (tnode->name == "spine_bb_")
	{
		template_group_index.push_back(get_template_node("spine1_bb_")->id);
		template_group_names[get_template_node("spine1_bb_")->id] = "spine1_bb_";
		template_group_index.push_back(get_template_node("spine2_bb_")->id);
		template_group_names[get_template_node("spine2_bb_")->id] = "spine2_bb_";
		template_group_index.push_back(get_template_node("neck_bb_")->id);
		template_group_names[get_template_node("neck_bb_")->id] = "neck_bb_";
	}
	else if (tnode->name == "leftupleg_bb_"
		|| tnode->name == "rightupleg_bb_"
		|| tnode->name == "leftshoulder_bb_"
		|| tnode->name == "rightshoulder_bb_"
		|| tnode->name == "neck_bb_")
	{
		if (bone_id_to_group_index.find(parent->id) != bone_id_to_group_index.end())
		{
			parent_group_index = bone_id_to_group_index[parent->id];
		}
		template_group_index.push_back(tnode->children.at(0)->id);
		template_group_names[tnode->children.at(0)->id] = tnode->children.at(0)->name;
	}
	else if (tnode->name == "lefthandthumb1_bb_")
	{
		template_group_index.push_back(get_template_node("lefthandthumb2_bb_")->id);
		template_group_names[get_template_node("lefthandthumb2_bb_")->id] = "lefthandthumb2_bb_";
		template_group_index.push_back(get_template_node("lefthandthumb3_bb_")->id);
		template_group_names[get_template_node("lefthandthumb3_bb_")->id] = "lefthandthumb3_bb_";
		template_group_index.push_back(get_template_node("lefthandthumb4_bb_")->id);
		template_group_names[get_template_node("lefthandthumb4_bb_")->id] = "lefthandthumb4_bb_";
	}
	else if (tnode->name == "righthandthumb1_bb_")
	{
		template_group_index.push_back(get_template_node("righthandthumb2_bb_")->id);
		template_group_names[get_template_node("righthandthumb2_bb_")->id] = "righthandthumb2_bb_";
		template_group_index.push_back(get_template_node("righthandthumb3_bb_")->id);
		template_group_names[get_template_node("righthandthumb3_bb_")->id] = "righthandthumb3_bb_";
		template_group_index.push_back(get_template_node("righthandthumb4_bb_")->id);
		template_group_names[get_template_node("righthandthumb4_bb_")->id] = "righthandthumb4_bb_";
	}
	else if (tnode->name == "lefthandindex1_bb_")
	{
		template_group_index.push_back(get_template_node("lefthandindex2_bb_")->id);
		template_group_names[get_template_node("lefthandindex2_bb_")->id] = "lefthandindex2_bb_";
		template_group_index.push_back(get_template_node("lefthandindex3_bb_")->id);
		template_group_names[get_template_node("lefthandindex3_bb_")->id] = "lefthandindex3_bb_";
		template_group_index.push_back(get_template_node("lefthandindex4_bb_")->id);
		template_group_names[get_template_node("lefthandindex4_bb_")->id] = "lefthandindex4_bb_";
	}
	else if (tnode->name == "righthandindex1_bb_")
	{
		template_group_index.push_back(get_template_node("righthandindex2_bb_")->id);
		template_group_names[get_template_node("righthandindex2_bb_")->id] = "righthandindex2_bb_";
		template_group_index.push_back(get_template_node("righthandindex3_bb_")->id);
		template_group_names[get_template_node("righthandindex3_bb_")->id] = "righthandindex3_bb_";
		template_group_index.push_back(get_template_node("righthandindex4_bb_")->id);
		template_group_names[get_template_node("righthandindex4_bb_")->id] = "righthandindex4_bb_";
	}
	else if (tnode->name == "lefthandmiddle1_bb_")
	{
		template_group_index.push_back(get_template_node("lefthandmiddle2_bb_")->id);
		template_group_names[get_template_node("lefthandmiddle2_bb_")->id] = "lefthandmiddle2_bb_";
		template_group_index.push_back(get_template_node("lefthandmiddle3_bb_")->id);
		template_group_names[get_template_node("lefthandmiddle3_bb_")->id] = "lefthandmiddle3_bb_";
		template_group_index.push_back(get_template_node("lefthandmiddle4_bb_")->id);
		template_group_names[get_template_node("lefthandmiddle4_bb_")->id] = "lefthandmiddle4_bb_";
	}
	else if (tnode->name == "righthandmiddle1_bb_")
	{
		template_group_index.push_back(get_template_node("righthandmiddle2_bb_")->id);
		template_group_names[get_template_node("righthandmiddle2_bb_")->id] = "righthandmiddle2_bb_";
		template_group_index.push_back(get_template_node("righthandmiddle3_bb_")->id);
		template_group_names[get_template_node("righthandmiddle3_bb_")->id] = "righthandmiddle3_bb_";
		template_group_index.push_back(get_template_node("righthandmiddle4_bb_")->id);
		template_group_names[get_template_node("righthandmiddle4_bb_")->id] = "righthandmiddle4_bb_";
	}
	else if (tnode->name == "lefthandring1_bb_")
	{
		template_group_index.push_back(get_template_node("lefthandring2_bb_")->id);
		template_group_names[get_template_node("lefthandring2_bb_")->id] = "lefthandring2_bb_";
		template_group_index.push_back(get_template_node("lefthandring3_bb_")->id);
		template_group_names[get_template_node("lefthandring3_bb_")->id] = "lefthandring3_bb_";
		template_group_index.push_back(get_template_node("lefthandring4_bb_")->id);
		template_group_names[get_template_node("lefthandring4_bb_")->id] = "lefthandring4_bb_";
	}
	else if (tnode->name == "righthandring1_bb_")
	{
		template_group_index.push_back(get_template_node("righthandring2_bb_")->id);
		template_group_names[get_template_node("righthandring2_bb_")->id] = "righthandring2_bb_";
		template_group_index.push_back(get_template_node("righthandring3_bb_")->id);
		template_group_names[get_template_node("righthandring3_bb_")->id] = "righthandring3_bb_";
		template_group_index.push_back(get_template_node("righthandring4_bb_")->id);
		template_group_names[get_template_node("righthandring4_bb_")->id] = "righthandring4_bb_";
	}
	else if (tnode->name == "lefthandpinky1_bb_")
	{
		template_group_index.push_back(get_template_node("lefthandpinky2_bb_")->id);
		template_group_names[get_template_node("lefthandpinky2_bb_")->id] = "lefthandpinky2_bb_";
		template_group_index.push_back(get_template_node("lefthandpinky3_bb_")->id);
		template_group_names[get_template_node("lefthandpinky3_bb_")->id] = "lefthandpinky3_bb_";
		template_group_index.push_back(get_template_node("lefthandpinky4_bb_")->id);
		template_group_names[get_template_node("lefthandpinky4_bb_")->id] = "lefthandpinky4_bb_";
	}
	else if (tnode->name == "righthandpinky1_bb_")
	{
		template_group_index.push_back(get_template_node("righthandpinky2_bb_")->id);
		template_group_names[get_template_node("righthandpinky2_bb_")->id] = "righthandpinky2_bb_";
		template_group_index.push_back(get_template_node("righthandpinky3_bb_")->id);
		template_group_names[get_template_node("righthandpinky3_bb_")->id] = "righthandpinky3_bb_";
		template_group_index.push_back(get_template_node("righthandpinky4_bb_")->id);
		template_group_names[get_template_node("righthandpinky4_bb_")->id] = "righthandpinky4_bb_";
	}
	else
	{
		if (!tnode->children.empty())
		{
			template_group_index.push_back(tnode->children.at(0)->id);
			template_group_names[tnode->children.at(0)->id] = tnode->children.at(0)->name;
		}
		while (parent)
		{
			if (bone_id_to_group_index.find(parent->id) != bone_id_to_group_index.end())
			{
				parent_group_index = bone_id_to_group_index[parent->id];
				break;
			}
			template_group_index.push_back(parent->id);
			template_group_names[parent->id] = parent->name;
			parent = parent->parent.lock();
		}
	}
	std::sort(template_group_index.begin(), template_group_index.end());
}

bool UMQuma::Impl::connect()
{
	if (connection_map_.empty()) return false;

	QmPdkErrCode e = QMPDK_ERR_CODE_ERROR;

	std::map<int, int> bone_id_to_group_index;
	for (ConnectionMap::iterator it = connection_map_.begin(); it != connection_map_.end(); ++it)
	{
		const std::string& name = (*it).second;
		if ( name == "hips_bb_" )
		{
			UMNodePtr node = it->first;
			if (node && node->parent())
			{
				const int parent_node_index = get_node_index(node->parent());
				const int node_index = get_node_index(node);
				if (parent_node_index >= 0 && node_index >= 0)
				{
					std::string parent_name = umbase::UMStringUtil::utf16_to_utf8(node->parent()->name());
					std::string name = umbase::UMStringUtil::utf16_to_utf8(node->name());
					e = QmPdkNnbCreateRootGroup(
						model_handle_,
						parent_node_index,
						node_index,
						&root_group_index_ );
					bone_id_to_group_index[parent_node_index] = root_group_index_;
				}
			}
			break;
		}
	}
	
	if (e == QMPDK_ERR_CODE_NOERROR && root_group_index_ >= 0)
	{
		for (int i = 0, size = static_cast<int>(template_node_list_.size()); i < size; ++i)
		{
			ModelNodePtr tnode = template_node_list_.at(i);
			if (tnode->name != "hips_bb_")
			{
				const std::string& name = tnode->name;
				std::vector<int> group_bones;
				std::map<int, UMNodePtr> group_bone_nodes;
				create_user_groups(name, group_bones, group_bone_nodes);

				if (!group_bones.empty())
				{
					int parent_group_index = root_group_index_;
					std::vector<int> template_group_index;
					std::map<int, std::string> template_group_names;

					create_template_groups(
						bone_id_to_group_index, 
						tnode,
						template_group_index,
						template_group_names,
						parent_group_index);
					
					int added_group_index;
					e = QmPdkNnbAddGroup(
						model_handle_,
						parent_group_index,
						static_cast<int>(group_bones.size()),
						&(group_bones.at(0)),
						static_cast<int>(template_group_index.size()),
						&(template_group_index.at(0)),
						&added_group_index );

					if (e == QMPDK_ERR_CODE_NOERROR)
					{
						printf("connected--------------------------\n");
						for (int k = 0, ksize = static_cast<int>(template_group_index.size()); k < ksize; ++k)
						{
							bone_id_to_group_index[template_group_index[k]] = added_group_index;
							printf("%d : %s template\n", template_group_index.at(k), template_group_names[template_group_index.at(k)].c_str());
						}
						for (int k = 0, ksize = static_cast<int>(group_bones.size()); k < ksize; ++k)
						{
							printf("%d : %s user\n", group_bones.at(k), umbase::UMStringUtil::utf16_to_utf8(group_bone_nodes[group_bones.at(k)]->name()).c_str());
						}
						printf("----------------------------------\n");
					}
					else
					{
						printf("node connection error!\n");
					}
				}
			}
		}
	}
	e = QmPdkNnbApply(model_handle_);
	if (e == QMPDK_ERR_CODE_NOERROR)
	{
		return apply_nnb();
	}
	return false;
}

bool UMQuma::Impl::apply_nnb()
{
	if (!re_attach_quma_device(model_handle_))
	{
		if (!attach_quma_device(model_handle_))
		{
			return false;
		}
	}
	is_bone_mapping_done_ = true;
	return true;
}

bool UMQuma::Impl::save_nnb(const std::u16string& path)
{
	QmPdkErrCode e = QmPdkNnbSaveToFile(
		model_handle_,
		FALSE,
		"out.nnb");
	
	if (e == QMPDK_ERR_CODE_NOERROR)
	{
		return true;
	}
	return false;
}

bool UMQuma::Impl::save_nnb_to_memory(std::string& path)
{
	int size = -1;
	QmPdkErrCode e = QmPdkNnbSaveToMem(
		model_handle_,
		FALSE,
		NULL,
		&size);
	
	if (e == QMPDK_ERR_CODE_NOERROR)
	{
		std::vector<char> buffer;
		buffer.resize(size);
		QmPdkErrCode e = QmPdkNnbSaveToMem(
			model_handle_,
			FALSE,
			&(*buffer.begin()),
			&size);
		if (e == QMPDK_ERR_CODE_NOERROR)
		{
			for (int i = 0; i < size; ++i)
			{
				path.push_back(buffer[i]);
			}
			return true;
		}
	}
	return false;
}

bool UMQuma::Impl::load_nnb(const umstring& path)
{
	if (has_user_nodes_)
	{
		if (umbase::UMPath::exists(path))
		{
			std::cout << "nnb loading..." << std::endl;
			std::ifstream ifs(path.c_str(), std::ios::in | std::ios::binary );
			std::string buffer;
			if (ifs.good())
			{
				std::string str( (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
				const char* cstr = str.c_str();
				
				QmPdkErrCode e = QmPdkNnbLoadFromMem(
					model_handle_,
					cstr,
					static_cast<int>(str.size()));
				
				is_bone_mapping_done_ = true;
				return true;
			}
		}
	}
	return false;
}

void UMQuma::Impl::create_connection_map_from_nnb_recursive(
	boost::property_tree::ptree& pt, 
	const boost::property_tree::ptree::value_type& root,
	const std::string& buffer)
{
	using namespace boost::property_tree;

	if (root.first == "bone_group")
	{
		std::string dst = "";
		//std::string src = "";
		BOOST_FOREACH(const ptree::value_type& child, root.second.get_child("<xmlattr>"))
		{
			if (child.first == "destination_name_group")
			{
				dst = child.second.data();
			}
			//else if (child.first == "source_name_group")
			//{
			//	src = child.second.data();
			//}
		}

		for (umdraw::UMNodeList::const_iterator it = scene_->node_list().begin(); 
			it != scene_->node_list().end(); 
			++it)
		{
			if ( dst.find( umbase::UMStringUtil::utf16_to_utf8((*it)->name()))  != std::string::npos)
			{
				connection_map_[ (*it) ] = "";
			}
		}
	}

	BOOST_FOREACH(const ptree::value_type& child, root.second)
	{
		create_connection_map_from_nnb_recursive(pt, child, buffer);
	}
}

void UMQuma::Impl::create_connection_map_from_nnb(const std::string& buffer)
{
	using namespace boost::property_tree;
	if (has_user_nodes_)
	{
		if (scene_)
		{
			std::istringstream stream(buffer);
			ptree pt;
			read_xml(stream, pt);
			BOOST_FOREACH(const ptree::value_type& child, pt)
			{
				create_connection_map_from_nnb_recursive(pt, child, buffer);
			}
			pt.clear();
		}
	}
}

bool UMQuma::Impl::load_nnb_from_memory(const std::string& buffer)
{
	if (has_user_nodes_)
	{
		if (!buffer.empty())
		{
			std::cout << "nnb loading..." << std::endl;
			QmPdkErrCode e = QmPdkNnbLoadFromMem(
				model_handle_,
				buffer.c_str(),
				static_cast<int>(buffer.size()));
				
			if (e == QMPDK_ERR_CODE_NOERROR)
			{
				connection_map_.clear();
				create_connection_map_from_nnb(buffer);
				is_bone_mapping_done_ = true;
				return true;
			}
		}
	}
	return false;
}

/**
 * print version
 */
void UMQuma::Impl::version()
{
	int len = 0;
	QmPdkGetVersionStr( NULL, &len );
	char* version = new char[len];
	QmPdkGetVersionStr(version, &len);
	printf("[InitQuma()] QUMA Plugin Development Kit %s\n", version);
	delete [] version;
}

/**
 * init quma
 */
bool UMQuma::Impl::init_quma()
{
	version();

	// turn on debug message
	char flags[] = { 0x01 | 0x02 };
	QmPdkDebugFlags( flags );
	
	// QmPdkの初期化.
	QmPdkErrCode e = QmPdkInit();
	if (e != QMPDK_ERR_CODE_NOERROR)
	{
		return false;
	}

	// 標準ボーンモデルの読み込み。
	if (!load_quma_model())
	{
		return false;
	}
	return true;
}




/// constructor
UMQuma::UMQuma()
	: impl_(new UMQuma::Impl())
{}

/// destructor
UMQuma::~UMQuma() 
{
	impl_->finalize();
}

/**
 * initialize
 */
bool UMQuma::init(umdraw::UMScenePtr scene)
{
	return impl_->init( scene);
}

/**
 * update scene
 */
bool UMQuma::update()
{
	return impl_->update();
}

/**
 * draw frame
 */
bool UMQuma::draw(UMCameraPtr camera)
{
	return impl_->draw(camera);
}

/**
 * add connection
 */
bool UMQuma::add_connection(UMNodePtr node, const std::string& name)
{
	return impl_->add_connection(node, name);
}

/**
 * connect
 */
bool UMQuma::connect()
{
	return impl_->connect();
}

/**
 * save nnb xml file
 */
bool UMQuma::save_nnb(const std::u16string& path)
{
	return impl_->save_nnb(path);
}

/**
 * save nnb to memory
 */
bool UMQuma::save_nnb_to_memory(std::string& buffer)
{
	return impl_->save_nnb_to_memory(buffer);
}

/**
 * load nnb xml file
 */
bool UMQuma::load_nnb(const std::u16string& path)
{
	return impl_->load_nnb(path);
}

/**
 * load nnb from memory
 */
bool UMQuma::load_nnb_from_memory(const std::string& buffer)
{
	return impl_->load_nnb_from_memory(buffer);
}

bool UMQuma::apply_nnb()
{
	return impl_->apply_nnb();
}

const std::map<umdraw::UMNodePtr, std::string>& UMQuma::get_connection_map() const
{
	return impl_->get_connection_map();
}

} // qumable
