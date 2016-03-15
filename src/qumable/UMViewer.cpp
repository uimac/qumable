/**
 * @file UMViewer.cpp
 *
 * @author tori31001 at gmail.com
 *
 * Copyright (C) 2013 Kazuma Hatta
 * Licensed  under the MIT license. 
 *
 */
#include <GL/glew.h>

#include "UMViewer.h"
#include "UMCamera.h"
#include "UMQuma.h"
#include "UMRT.h"

#include "UMStringUtil.h"
#include "UMPath.h"
#include "UMWSIO.h"
#include "UMWSIOEventType.h"
#include "UMGUIScene.h"
#include "UMLine.h"
#include "UMGUIObject.h"

#include <windows.h>
#include <map>
#include <fstream>
#include <string>
#include <iterator>
#include <iostream>
#include <tchar.h>
#include <thread>

#include <windows.h>
#include <mmsystem.h>
#include <GL/glfw3.h>
#include <GL/glfw3native.h>

namespace qumable
{
	using namespace umdraw;
	using namespace umgui;
	
int UMViewer::width_(0);
int UMViewer::height_(0);
int UMViewer::initial_width_(0);
int UMViewer::initial_height_(0);
bool UMViewer::is_disable_update_(false);
bool UMViewer::is_disable_update_quma_(false);
bool UMViewer::is_wsio_loaded_(false);
GLFWwindow* UMViewer::sub_window_(NULL);
GLFWwindow* UMViewer::window_(NULL);
UMScenePtr UMViewer::scene_;
UMCameraPtr UMViewer::temporary_camera_;
UMQumaPtr UMViewer::quma_;
UMMappingGUIPtr UMViewer::gui_scene_;
int UMViewer::port_;
UMViewerPtr UMViewer::viewer_;
static umwsio::UMWSIOPtr wsio_;

	UMMaterialPtr line_mat;

class UMFileLoadThread
{
public:
	UMFileLoadThread() : is_loaded_(false) {}
	~UMFileLoadThread() 
	{
		if (thread_.joinable())
		{
			thread_.join();
		}
	}

	void load(
		GLFWwindow * window,
		int count,
		const char** files,
		UMViewerPtr viewer,
		umdraw::UMScenePtr scene)
	{
		window_ = window;
		count_ = count > 0 ? 1 : 0;
		files_ = files;
		viewer_ = viewer;
		scene_ = scene;
		scene_->set_enable_deform(false);
		thread_ = std::thread([this] { do_(); });
	}

	void done()
	{
		if (thread_.joinable())
		{
			thread_.join();
		}
		viewer_->file_loaded_callback(window_);
		window_ = NULL;
		is_loaded_ = false;
		count_ = 0;
		files_ = NULL;
		scene_->set_enable_deform(true);
		viewer_ = UMViewerPtr();
		scene_ = umdraw::UMScenePtr();
	}

	bool is_loaded() const { return is_loaded_; }

private:

	void do_()
	{
		scene_->clear_geometry();
		for (int i = 0; i < count_; ++i)
		{
			printf("%d: '%s'\n", i + 1, files_[i]);
			std::string utf8path(files_[i]);
			umstring path = umbase::UMStringUtil::utf8_to_utf16(utf8path);
			if (scene_->load(path))
			{
				is_loaded_ = true;
			}
		}
	}
	GLFWwindow* window_;
	std::thread thread_;
	bool is_loaded_;
	int count_;
	const char** files_;
	UMViewerPtr viewer_;
	umdraw::UMScenePtr scene_;
};

static UMFileLoadThread load_thread;

bool UMViewer::init(
	GLFWwindow* window,
	GLFWwindow* sub_window, 
	UMScenePtr scene, 
	UMMappingGUIPtr gui_scene,
	UMDraw::DrawType type, 
	int width, 
	int height,
	int port)
{
	if (!scene) return false;

	UMViewer::width_ = width;
	UMViewer::height_ = height;
	if (UMViewer::initial_width_ <= 0)
	{
		UMViewer::initial_width_ = width;
	}
	if (UMViewer::initial_height_ <= 0)
	{
		UMViewer::initial_height_ = height;
	}
	
	sub_window_ = sub_window;
	scene_ = scene;
	gui_scene_ = gui_scene;
	port_ = port;
	
	quma_ = std::make_shared<UMQuma>();
	if (!quma_ || !quma_->init(scene_)) return false;
	gui_scene->set_umdraw_scene(scene);
	gui_scene->init(UMViewer::initial_width_, UMViewer::initial_height_);

	viewer_ = create(window, sub_window, scene, gui_scene, type);

	if (viewer_)
	{
		return true;
	}
	return false;
}

UMViewerPtr UMViewer::create(
	GLFWwindow* window,
	GLFWwindow* sub_window, 
	UMScenePtr scene, 
	UMMappingGUIPtr gui_scene, 
	UMDraw::DrawType type)
{
	HWND hwnd = glfwGetWin32Window(window);
	if (!wsio_)
	{
		wsio_ = std::make_shared<umwsio::UMWSIO>();
		wsio_->start_server(scene_, port_);
	}
	if (type == UMDraw::eOpenGL)
	{
		UMDrawPtr drawer = UMDraw::create(UMDraw::eOpenGL);
		UMGUIPtr gui;
		if (gui_scene) gui = UMGUI::create(UMGUI::eOpenGL);
		
		if (drawer->init(hwnd, scene))
		{
			drawer->resize(UMViewer::width_, UMViewer::height_);
			if (gui) { 
				gui->init(gui_scene);
				gui->resize(UMViewer::width_, UMViewer::height_);
			}
			UMViewerPtr viewer = UMViewerPtr(new UMViewer(drawer, gui));
			if (wsio_)
			{
				//if (wsio_->model_loaded_event()->listener_count() == 0)
				{
					wsio_->model_loading_event()->clear_listeners();
					wsio_->model_loaded_event()->clear_listeners();
					wsio_->connect_event()->clear_listeners();
					wsio_->disconnect_event()->clear_listeners();
					wsio_->disconnecting_event()->clear_listeners();
					wsio_->reconnect_event()->clear_listeners();
					wsio_->model_loading_event()->add_listener(viewer);
					wsio_->model_loaded_event()->add_listener(viewer);
					wsio_->connect_event()->add_listener(viewer);
					wsio_->disconnect_event()->add_listener(viewer);
					wsio_->disconnecting_event()->add_listener(viewer);
					wsio_->reconnect_event()->add_listener(viewer);
				}
			}
			window_ = window;
			return viewer;
		}
	}
	if (type == UMDraw::eDirectX)
	{
		UMDrawPtr drawer = UMDraw::create(UMDraw::eDirectX);
		UMGUIPtr gui;
		if (gui_scene) gui = UMGUI::create(UMGUI::eDirectX);

		if (drawer->init(hwnd, scene))
		{
			drawer->resize(UMViewer::width_, UMViewer::height_);
			if (gui) {
				gui->init(gui_scene);
				gui->resize(UMViewer::width_, UMViewer::height_);
			}
			UMViewerPtr viewer  = UMViewerPtr(new UMViewer(drawer, gui));
			if (wsio_)
			{
				//if (wsio_->model_loaded_event()->listener_count() == 0)
				{
					wsio_->model_loading_event()->clear_listeners();
					wsio_->model_loaded_event()->clear_listeners();
					wsio_->connect_event()->clear_listeners();
					wsio_->disconnect_event()->clear_listeners();
					wsio_->disconnecting_event()->clear_listeners();
					wsio_->reconnect_event()->clear_listeners();
					wsio_->model_loading_event()->add_listener(viewer);
					wsio_->model_loaded_event()->add_listener(viewer);
					wsio_->connect_event()->add_listener(viewer);
					wsio_->disconnect_event()->add_listener(viewer);
					wsio_->disconnecting_event()->add_listener(viewer);
					wsio_->reconnect_event()->add_listener(viewer);
				}
			}
			window_ = window;
			return viewer;
		}
	}
	return UMViewerPtr();
}

void UMViewer::call_paint()
{
	if (!viewer_) return;
	viewer_->on_paint();

	if (load_thread.is_loaded())
	{
		load_thread.done();
		is_disable_update_quma_ = false;
	}
	if (is_wsio_loaded_)
	{
		viewer_->file_loaded_callback(window_);
		is_wsio_loaded_ = false;
	}
}

void UMViewer::update(umbase::UMEventType event_type, umbase::UMAny& parameter)
{
	if (event_type == umwsio::eWSIOEventModelLoading)
	{
		is_disable_update_ = true;
	}
	else if (event_type == umwsio::eWSIOEventModelLoaded)
	{
		is_wsio_loaded_ = true;
	}
	else if (event_type == umwsio::eWSIOEventConnect)
	{
		is_disable_update_ = true;
		if (quma_ && quma_->connect())
		{
			std::string buffer;
			if (quma_->save_nnb_to_memory(buffer))
			{
				if (quma_->load_nnb_from_memory(buffer))
				{
					wsio_->set_nnb(buffer);
					wsio_->set_connection_map(quma_->get_connection_map());
				}
			}
		}
		is_disable_update_ = false;
	}
	else if (event_type == umwsio::eWSIOEventReconnect)
	{
		is_disable_update_ = true;
		if (quma_)
		{
			const std::string& data = wsio_->nnb();
			if (!wsio_->nnb().empty())
			{
				if (quma_->load_nnb_from_memory(data))
				{
					if (quma_->apply_nnb())
					{
						wsio_->set_nnb(data);
						wsio_->set_connection_map(quma_->get_connection_map());
						is_disable_update_ = false;
						return;
					}
				}
			}
		}
		wsio_->set_nnb("");
		std::map<umdraw::UMNodePtr, std::string> empty_map;
		wsio_->set_connection_map(empty_map);
		is_disable_update_ = false;
	}
	else if (event_type == umwsio::eWSIOEventDisconnecting)
	{
		is_disable_update_ = true;
	}
	else if (event_type == umwsio::eWSIOEventDisconnect)
	{
		is_wsio_loaded_ = true;
	}
}

void UMViewer::key_callback(GLFWwindow * window, int key, int scancode, int action, int mods)
{
	if (!viewer_) return;
	viewer_->on_keyboard(window, key, action);

	// change viewer
	if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
	{
		if (draw_type() == UMDraw::eDirectX) 
		{
			// change to gl
			viewer_->close_view();
			viewer_ = qumable::UMViewerPtr();
			viewer_ = create(window, sub_window_, scene_, gui_scene_, UMDraw::eOpenGL);
		}
		else
		{
			// change to dx_drawer_
			viewer_->close_view();
			viewer_ = qumable::UMViewerPtr();
			viewer_ = create(window, sub_window_, scene_, gui_scene_, UMDraw::eDirectX);
		}
	}
}

void UMViewer::mouse_button_callback(GLFWwindow * window, int button, int action, int mods)
{
	if (!viewer_) return;
	viewer_->on_mouse(window, button, action);
}

void UMViewer::scroll_callback(GLFWwindow * window, double xoffset, double yoffset)
{
	if (!viewer_) return;
	viewer_->on_scroll(window, xoffset, yoffset);
}

void UMViewer::cursor_pos_callback(GLFWwindow * window, double x, double y)
{
	if (!viewer_) return;
	viewer_->on_mouse_move(window, x, y);
}

void UMViewer::window_size_callback(GLFWwindow * window, int width, int height)
{
	if (!viewer_) return;
	viewer_->on_resize(window, width, height);
}

void UMViewer::window_close_callback(GLFWwindow * window)
{
	if (!viewer_) return;
	wsio_ = umwsio::UMWSIOPtr();
	viewer_->on_close(window);
	viewer_ = qumable::UMViewerPtr();
	scene_ = UMScenePtr();
}


void UMViewer::file_loaded_callback(GLFWwindow * window)
{
	is_disable_update_ = true;
	const UMDraw::DrawType type = UMViewer::draw_type();
	viewer_->close_view();
	viewer_ = qumable::UMViewerPtr();
	quma_ = qumable::UMQumaPtr();
	
	quma_ = std::make_shared<UMQuma>();
	if (!quma_ || !quma_->init(scene_)) return;
	gui_scene_->set_umdraw_scene(scene_);
	gui_scene_->init(initial_width_, initial_height_);
	
	viewer_ = UMViewer::create(window, sub_window_, scene_, gui_scene_, type);
	
	size_t polygons = 0;
	if (scene_)
	{
		polygons += scene_->total_polygon_size();
	}
	printf("%d polygons\n", polygons);
	is_disable_update_ = false;
}

void UMViewer::drop_files_callback(GLFWwindow * window, int count, const char** files)
{
	is_disable_update_quma_ = true;
	load_thread.load(window, count, files, viewer_, scene_);
}

UMDraw::DrawType UMViewer::draw_type()
{
	if (!viewer_) return UMDraw::eSoftware;
	if (!viewer_->drawer_) return UMDraw::eSoftware;
	return viewer_->drawer_->draw_type();
}

/**
 * constructor
 */
UMViewer::UMViewer(UMDrawPtr drawer, UMGUIPtr gui)
	: pre_x_(0.0)
	, pre_y_(0.0)
	, current_x_(0.0)
	, current_y_(0.0)
	, current_frames_(0)
	, fps_base_time_(timeGetTime())
	, motion_base_time_(timeGetTime())
	, is_left_button_down_(false)
	, is_right_button_down_(false)
	, is_ctrl_button_down_(false)
	, is_middle_button_down_(false)
	, is_alt_down_(false)
	, is_shift_down_(false)
	, is_gui_drawing_(false)
	, drawer_(drawer)
	, gui_(gui)
	, current_seconds_(0.0)
{
	if (!rt_)
	{
		rt_ = std::make_shared<umrt::UMRT>();
		rt_->init();
		rt_->add_scene(scene_);
		rt_->add_pick_node_list(scene_);
	}
}

/**
 * refresh frame
 */
bool UMViewer::on_paint()
{
	if (is_disable_update_) return true;
	
	++current_frames_;
	unsigned long current_time = static_cast<unsigned long>(timeGetTime());
	unsigned long time_from_fps_base = current_time - fps_base_time_;
	unsigned long time_from_motion_base = current_time - motion_base_time_;
	if (time_from_fps_base > 1000)
	{
		int fps = (current_frames_ * 1000) / time_from_fps_base;
		fps_base_time_ = current_time;
		current_frames_ = 0;
		//printf("fps %d \n", fps);
	}

	if (drawer_->clear())
	{
		if (!is_disable_update_quma_)
		{
			quma_->update();
		}
		if (drawer_->update())
		{
			drawer_->draw();
		}
		if (gui_ && gui_->update())
		{
			if (is_gui_drawing_)
			{
				gui_->draw();
			}
		}
		return true;
	}
	return false;
}

/**
 * keyboard
 */
void UMViewer::on_keyboard(GLFWwindow * window,int key, int action)
{
	UMScenePtr scene = drawer_->scene();
	if (!scene) return;
	UMCameraPtr camera = scene->camera();
	if (!camera) return;
	
	if (gui_)
	{
		if (gui_->on_keyboard(key, action))
		{
			return;
		}
	}
	if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL)
	{
		if (action == GLFW_PRESS)
		{
			is_ctrl_button_down_ = true;
		}
		else if (action == GLFW_RELEASE)
		{
			is_ctrl_button_down_ = false;
		}
	}
	else if (key == GLFW_KEY_TAB)
	{
		if (action == GLFW_PRESS)
		{
			is_gui_drawing_ = !is_gui_drawing_;
			if (is_gui_drawing_)
			{
				scene_->set_visible(UMScene::eMesh, false);
			}
			else
			{
				scene_->set_visible(UMScene::eMesh, true);
			}
		}
	}
	else if (key == GLFW_KEY_ENTER)
	{
	}
	else if (key == GLFW_KEY_LEFT_ALT)
	{
		if (action == GLFW_PRESS)
		{
			is_alt_down_ = true;
		}
		else if (action == GLFW_RELEASE)
		{
			is_alt_down_ = false;
		}
	}
	else if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
	{
		if (action == GLFW_PRESS)
		{
			is_shift_down_ = true;
		}
		else if (action == GLFW_RELEASE)
		{
			is_shift_down_ = false;
		}
	}
	else if (key == GLFW_KEY_KP_0 && action == GLFW_PRESS)
	{
		camera->init(camera->is_ortho(), scene->width(), scene->height());
	}
	else if (key == GLFW_KEY_KP_2 && action == GLFW_PRESS)
	{
		camera->rotate(0, 15);
	}
	else if (key == GLFW_KEY_KP_8 && action == GLFW_PRESS)
	{
		camera->rotate(0, -15);
	}
	else if (key == GLFW_KEY_KP_4 && action == GLFW_PRESS)
	{
		camera->rotate(-15, 0);
	}
	else if (key == GLFW_KEY_KP_6 && action == GLFW_PRESS)
	{
		camera->rotate(15, 0);
	}
	else if (key == GLFW_KEY_KP_1 && action == GLFW_PRESS)
	{
		UMVec3d pos = camera->position();
		camera->init(camera->is_ortho(), scene->width(), scene->height());
		camera->set_position(pos);
		camera->rotate(-3, 0);
		camera->rotate(3, 0);
	}
	else if (key == GLFW_KEY_KP_3 && action == GLFW_PRESS)
	{
		UMVec3d pos = camera->position();
		camera->init(camera->is_ortho(), scene->width(), scene->height());
		camera->set_position(pos);
		if (is_shift_down_)
		{
			camera->rotate(-90, 0);
		}
		else
		{
			camera->rotate(90, 0);
		}
	}
	else if (key == GLFW_KEY_KP_7 && action == GLFW_PRESS)
	{
		UMVec3d pos = camera->position();
		camera->init(camera->is_ortho(), scene->width(), scene->height());
		camera->set_position(pos);
		if (is_shift_down_)
		{
			camera->rotate(0, 89.9);
		}
		else
		{
			camera->rotate(0, -89.9);
		}
	}
	
	if (is_ctrl_button_down_ && key == GLFW_KEY_S && action == GLFW_PRESS)
	{
		quma_->save_nnb(std::u16string());
	}
	if (is_ctrl_button_down_ && key == GLFW_KEY_C && action == GLFW_PRESS)
	{
		quma_->connect();
	}
}

/**
 * mouse button up/down
 */
void UMViewer::on_mouse(GLFWwindow * window, int button, int action)
{
	if (gui_)
	{
		if (gui_->on_mouse(button, action))
		{
			return;
		}
	}
	if (action == GLFW_PRESS)
	{
		pre_x_ = current_x_;
		pre_y_ = current_y_;
		is_left_button_down_ = (button == GLFW_MOUSE_BUTTON_LEFT);
		is_right_button_down_ = (button == GLFW_MOUSE_BUTTON_RIGHT);
		is_middle_button_down_ = (button == GLFW_MOUSE_BUTTON_MIDDLE);
	}
	else
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT) 
		{
			is_left_button_down_ = false;
		}
		if (button == GLFW_MOUSE_BUTTON_RIGHT)
		{
			is_right_button_down_ = false;
		}
		if (button == GLFW_MOUSE_BUTTON_MIDDLE)
		{
			is_middle_button_down_ = false;
		}

		if (pick_node_)
		{
			unpick_bone();
		}
	}

	if (rt_ && is_left_button_down_)
	{
		pick_bone();
	}
}

void UMViewer::pick_bone()
{
	UMNodePtr node = rt_->pick(current_x_, scene_->height() - current_y_);
	if (node)
	{
		pick_node_ = node;
		pick_node_->set_node_color(UMVec4d(1.0, 0.0, 0.0, 1.0));

		if (!line_mat)
		{
			line_mat = UMMaterial::default_material();
			line_mat->set_diffuse(UMVec4d(1.0, 0.0, 0.0, 1.0));
			line_mat->set_polygon_count(1);
		}

		if (scene_->mutable_temporary_line_list().empty())
		{
			UMLinePtr line = std::make_shared<UMLine>();
			line->mutable_material_list().push_back(line_mat);
			UMVec3d p0 = screen_to_world_pos(current_x_, current_y_);
			UMVec3d p1 = screen_to_world_pos(current_x_ + 0.1, current_y_ + 0.1);
			UMLine::Line line1(p0, p1);
			line->mutable_line_list().push_back(line1);
			scene_->mutable_temporary_line_list().push_back(line);
			line->set_changed(true);
		}
	}
}

void UMViewer::unpick_bone()
{
	if (!scene_) return;
	if (!pick_node_) return;

	std::u16string controller_name;
	gui_scene_->get_picked_bone_controller_name(controller_name);
	if (!controller_name.empty())
	{
		UMVec4d color;
		gui_scene_->get_picked_bone_controller_color(color);
		quma_->add_connection(pick_node_, umbase::UMStringUtil::utf16_to_utf8(controller_name));
		pick_node_->set_node_color(color);

		if (pick_node_->parent() && pick_node_->parent()->children().size() > 1)
		{
			UMNodeList siblings = pick_node_->parent()->children();
			for (int i= 0, isize = static_cast<int>(siblings.size()); i < isize; ++i)
			{
				UMNodePtr sibling = siblings.at(i);
				quma_->add_connection(sibling, umbase::UMStringUtil::utf16_to_utf8(controller_name));
				sibling->set_node_color(color);
			}
		}
	}
	else
	{
		pick_node_->set_node_color(UMVec4d(0.5, 0.5, 0.5, 1.0));
	}
	pick_node_ = UMNodePtr();
	scene_->mutable_temporary_line_list().clear();

}

UMVec3d UMViewer::screen_to_world_pos(double x, double y)
{
	if (!scene_) return UMVec3d();
	if (!scene_->camera()) return UMVec3d();
	UMMat44d v_inv = scene_->camera()->view_projection_matrix().inverted();
	return v_inv * UMVec3d(
		x / (double)width_ * 2.0 - 1.0, 
		(height_ - y) / (double)height_ * 2.0 - 1.0,
		0);
}

void UMViewer::on_pick_bone()
{
	if (!scene_) return;
	if (scene_->mutable_temporary_line_list().empty()) return;
	// set end point of line
	UMLinePtr line = scene_->mutable_temporary_line_list().at(0);
	UMMat44d v_inv = scene_->camera()->view_projection_matrix().inverted();
	line->mutable_line_list().at(0).p1 = v_inv * UMVec3d(
			current_x_ / (double)width_ * 2.0 - 1.0, 
			(scene_->height() - current_y_) / double(height_) * 2.0 - 1.0,
			0);
	line->set_changed(true);

	// hit test to bone controller
	if (is_gui_drawing_)
	{
		gui_scene_->pick_bone_controller(current_x_ /double(scene_->width()), current_y_ /(double)(scene_->height()));
	}
}

/**
 * scroll
 */
void UMViewer::on_scroll(GLFWwindow *, double xoffset, double yoffset)
{
	if (gui_)
	{
		if (gui_->on_scroll(xoffset, yoffset))
		{
			return;
		}
	}

	UMScenePtr scene = drawer_->scene();
	if (UMCameraPtr camera = scene->camera())
	{
		if (is_shift_down_)
		{
			camera->pan(0, yoffset * 10);
		}
		else if (is_ctrl_button_down_)
		{
			camera->pan(yoffset * 10, 0);
		}
		else
		{
			camera->dolly(0, yoffset * 30);
		}
	}
}

/**
 * mouse move
 */
void UMViewer::on_mouse_move(GLFWwindow * window, double x, double y)
{
	UMScenePtr scene = drawer_->scene();
	if (!scene) return;
	UMCameraPtr camera = scene->camera();
	if (!camera) return;
	current_x_ = x;
	current_y_ = y;
	
	if (gui_)
	{
		if (gui_->on_mouse_move(x, y))
		{
			return;
		}
	}

	if (is_middle_button_down_)
	{
		if (is_shift_down_)
		{
			camera->pan(pre_x_ - x, pre_y_ - y);
			pre_x_ = x;
			pre_y_ = y;
		}
		else if (is_ctrl_button_down_)
		{
			camera->dolly(pre_x_ - x, pre_y_ - y);
			pre_x_ = x;
			pre_y_ = y;
		}
		else
		{
			camera->rotate(pre_x_ - x, pre_y_ - y);
			pre_x_ = x;
			pre_y_ = y;
		}
	}

	if (pick_node_)
	{
		on_pick_bone();
	}
}

/**
 * resize
 */
void UMViewer::on_resize(GLFWwindow *, int width, int height)
{
	if (drawer_)
	{
		drawer_->resize(width, height);
	}
	if (gui_)
	{
		gui_->resize(width, height);
	}
	UMViewer::width_ = width;
	UMViewer::height_ = height;
}

void UMViewer::close_view()
{
	gui_ = UMGUIPtr();
	drawer_ = UMDrawPtr();
}

/**
 * window close
 */
void UMViewer::on_close(GLFWwindow * window)
{
	if (gui_) { gui_->dispose(); }
	quma_ = UMQumaPtr();
	gui_ = UMGUIPtr();
	drawer_ = UMDrawPtr();
}

} // qumable
