/**
 * @file UMViewer.h
 *
 * @author tori31001 at gmail.com
 *
 * Copyright (C) 2013 Kazuma Hatta
 * Licensed  under the MIT license. 
 *
 */
#pragma once

#include "UMMacro.h"
#include "UMDraw.h"
#include "UMGUI.h"
#include "UMScene.h"
#include "UMMappingGUI.h"
#include "UMRT.h"
#include "UMEvent.h"
#include "UMListener.h"

struct GLFWwindow;

/// test viewer(application)
namespace qumable
{

class UMQuma;
typedef std::shared_ptr<UMQuma> UMQumaPtr;

class UMViewer;
typedef std::shared_ptr<UMViewer> UMViewerPtr;

class UMViewer : public umbase::UMListener
{
	DISALLOW_COPY_AND_ASSIGN(UMViewer);
public:
	virtual ~UMViewer() {}
	
	/**
	 * initialize viewer with scene and draw type
	 */
	static bool init(
		GLFWwindow* window,
		GLFWwindow* sub_window, 
		umdraw::UMScenePtr scene, 
		UMMappingGUIPtr gui_scene, 
		umdraw::UMDraw::DrawType type, 
		int width, 
		int height,
		int port);

	/**
	 * call a paint method of the current draw type 
	 */
	static void call_paint();

	/**
	 * key event callback
	 */
	static void key_callback(GLFWwindow * window,int key, int scancode, int action, int mods);
	
	/**
	 * mouse event callback
	 */
	static void mouse_button_callback(GLFWwindow * window, int button, int action, int mods);
	
	/**
	 * scroll event callback
	 */
	static void scroll_callback(GLFWwindow * window, double xoffset, double yoffset);
	
	/**
	 * mouse position event callback
	 */
	static void cursor_pos_callback(GLFWwindow * window, double x, double y);
	
	/**
	 * window size event callback
	 */
	static void window_size_callback(GLFWwindow * window, int width, int height);
	
	/**
	 * window close event callback
	 */
	static void window_close_callback(GLFWwindow * window);

	/**
	 * drop file callback
	 */
	static void drop_files_callback(GLFWwindow * window, int n, const char** files);

	/**
	 * get current draw type
	 */
	static umdraw::UMDraw::DrawType draw_type();
	
	/**
	 *
	 */
	void file_loaded_callback(GLFWwindow * window);
	
	virtual void update(umbase::UMEventType event_type, umbase::UMAny& parameter);

protected:
	
	static UMViewerPtr create(
		GLFWwindow* window,
		GLFWwindow* sub_window, 
		umdraw::UMScenePtr scene,
		UMMappingGUIPtr gui_scene,
		umdraw::UMDraw::DrawType type);

	/**
	 * constructor
	 */
	explicit UMViewer(umdraw::UMDrawPtr drawer, umgui::UMGUIPtr gui);

	/**
	 * refresh frame
	 */
	bool on_paint();
	
	/**
	 * keyboard
	 */
	void on_keyboard(GLFWwindow * window,int key, int action);
	
	/**
	 * mouse button up/down
	 */
	void on_mouse(GLFWwindow * window, int button, int action);

	/**
	 * scroll
	 */
	void on_scroll(GLFWwindow *, double xoffset, double yoffset);
	
	/**
	 * mouse move
	 */
	void on_mouse_move(GLFWwindow *, double x, double y);
	
	/**
	 * resize
	 */
	void on_resize(GLFWwindow *, int width, int height);
	
	/**
	 * window close
	 */
	void on_close(GLFWwindow *);
	
	/**
	 * view close
	 */
	void close_view();

private:
	static int initial_width_;
	static int initial_height_;
	static int width_;
	static int height_;
	static int port_;
	static bool is_disable_update_;
	static bool is_disable_update_quma_;
	static bool is_wsio_loaded_;
	static umdraw::UMScenePtr scene_;
	static umdraw::UMCameraPtr temporary_camera_;
	static UMMappingGUIPtr gui_scene_;
	static UMViewerPtr viewer_;
	static GLFWwindow* window_;
	static GLFWwindow* sub_window_;
	static UMQumaPtr quma_;

	double pre_x_;
	double pre_y_;
	double current_x_;
	double current_y_;
	double current_seconds_;
	unsigned long fps_base_time_;
	unsigned long motion_base_time_;
	unsigned long current_frames_;
	bool is_ctrl_button_down_;
	bool is_left_button_down_;
	bool is_right_button_down_;
	bool is_middle_button_down_;
	bool is_alt_down_;
	bool is_shift_down_;
	bool is_gui_drawing_;
	umdraw::UMDrawPtr drawer_;
	umgui::UMGUIPtr gui_;
	umrt::UMRTPtr rt_;
	umdraw::UMNodePtr pick_node_;

	void pick_bone();
	void on_pick_bone();
	void unpick_bone();
	UMVec3d screen_to_world_pos(double x, double y);
};

} // qumable
