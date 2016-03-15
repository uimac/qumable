/**
 *
 * @file UMMappingGUI.h
 *
 * @author tori31001 at gmail.com
 *
 * Copyright (C) 2014 Kazuma Hatta
 * Licensed under the MIT or GPL Version 2 or GPL Version 3 licenses. 
 *
 */

#pragma once

#include "UMMacro.h"
#include <memory>
#include <string>
#include "UMGUIScene.h"
#include "UMGUIObject.h"
#include "UMGUIBoard.h"
#include "UMListenerConnector.h"

namespace qumable
{

class UMMappingGUI;
typedef std::shared_ptr<UMMappingGUI> UMMappingGUIPtr;
typedef std::weak_ptr<UMMappingGUI> UMMappingGUIWeakPtr;

class UMMappingGUI
	: public umgui::UMGUIScene
	, public umbase::UMListenerConnector
	, public umbase::UMListener

{
	DISALLOW_COPY_AND_ASSIGN(UMMappingGUI);
public:
	static UMMappingGUIPtr create();
	virtual ~UMMappingGUI() {}
	
	/**
	 * initialize gui components
	 */
	virtual bool init(int width, int height);

	void pick_bone_controller(double x, double y);

	void get_picked_bone_controller_name(std::u16string& name);

	void get_picked_bone_controller_color(UMVec4d& color);

protected:

	/**
	 * update event
	 */
	virtual void update(umbase::UMEventType event_type, umbase::UMAny& parameter);

private:
	UMMappingGUI() {}
	UMMappingGUIWeakPtr self_;
	umgui::UMGUIBoardList bone_controller_list_;
	umgui::UMGUIBoardPtr picked_controller_;
	umgui::UMGUIBoardPtr select_rect_;
	std::vector<umgui::UMGUIBoardPtr> text_node_list_;
	void set_picked_controller(umgui::UMGUIBoardPtr obj);
};

} // qumable
