/**
 * @file UMMappingGUI.cpp
 *
 * @author tori31001 at gmail.com
 *
 * Copyright (C) 2013 Kazuma Hatta
 * Licensed  under the MIT license. 
 *
 */
#include "UMMappingGUI.h"
#include "UMGUIBoard.h"
#include "UMGUIScrollBoard.h"
#include "UMStringUtil.h"
#include "UMNode.h"
#include "UMScene.h"
#include "UMListenerConnector.h"
#include "UMGUIEventType.h"
#include <tchar.h>

namespace qumable
{
	using namespace umgui;
	using namespace umdraw;

UMMappingGUIPtr UMMappingGUI::create()
{
	UMMappingGUIPtr mapping_gui = UMMappingGUIPtr(new UMMappingGUI());
	mapping_gui->self_ = mapping_gui;
	return mapping_gui;
}

bool UMMappingGUI::init(int width, int height)
{
	if (!umgui::UMGUIScene::init(width, height))
	{
		return false;
	}
	umdraw::UMScenePtr scene = umdraw_scene();
	if (!scene) { return false; }

	UMMappingGUIPtr self = self_.lock();

	// ボーン
	{
		UMGUIBoardPtr text_node = UMGUIBoard::create_board(-10);
		text_node->add_text_panel(width, height, 20, 15, 20, _T("ボーン"));
		root_object()->mutable_children().push_back(text_node);
	}

	// left
	UMGUIScrollBoardPtr left_board = UMGUIScrollBoard::create_board(width, height, 10, 40, -100, 250, height-60, UMVec4d(0.1, 0.1, 0.1, 0.9));
	root_object()->mutable_children().push_back(left_board);
	{
		UMNodeList::const_iterator it = scene->node_list().begin();
		for (int i = 0; it != scene->node_list().end(); ++it, ++i)
		{
			int x = 40;
			int y = 40 + i * 20;
			UMNodePtr node = *it;
			umtextstring str = umbase::UMStringUtil::utf16_to_wstring(node->name());
			UMGUIBoardPtr text_panel = UMGUIBoard::create_board(-5);
			text_panel->add_text_panel(width, height, x, y, 16, str.c_str());
			int tw = static_cast<int>(text_panel->box().maximum().x - text_panel->box().minimum().x);
			int th = static_cast<int>(text_panel->box().maximum().y - text_panel->box().minimum().y);
				
			UMGUIBoardPtr text_node = UMGUIBoard::create_board(-10);
			text_node_list_.push_back(text_node);

			const umstring& name = node->name();
			text_node->set_name(name);
			//// connect events
			//mutable_event_list().push_back(text_node->select_event());
			//text_node->select_event()->add_listener(self);
			text_node->add_text_node_panel(width, height,  x, y, tw, th, UMVec4d(0.4, 0.4, 0.4, 0.5));
			left_board->mutable_children().push_back(text_node);
			text_node->mutable_children().push_back(text_panel);

			if (y > left_board->box().size().y) 
			{
				text_node->set_visible(false);
				text_panel->set_visible(false);
			}
		}
	}

	// 選択範囲.
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-110);
		int rx = 0;
		int ry = 0;
		right_board->add_color_panel(width, height, rx, ry, 20, 15, UMVec4d(1.0, 0.0, 0.0, 0.9));
		right_board->set_visible(false);
		root_object()->mutable_children().push_back(right_board);
		select_rect_ = right_board;
	}

	// 頭
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("head_bb_"));
		int rx = width - 200;
		int ry = 40;
		right_board->add_color_panel(width, height, rx, ry, 30, 40, UMVec4d(0.7, 0.7, 0.1, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}

	// 首
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("neck_bb_"));
		int rx = width - 200;
		int ry = 85;
		right_board->add_color_panel(width, height, rx, ry, 30, 15, UMVec4d(0.1, 0.7, 0.7, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	// 肩向かって左
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("rightshoulder_bb_"));
		int rx = width - 220;
		int ry = 105;
		right_board->add_color_panel(width, height, rx, ry, 15, 20, UMVec4d(0.1, 0.7, 0.1, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}

	// 肩向かって右
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("leftshoulder_bb_"));
		int rx = width - 165;
		int ry = 105;
		right_board->add_color_panel(width, height, rx, ry, 15, 20, UMVec4d(0.5, 0.8, 0.2, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	// 上腕向かって左
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("rightarm_bb_"));
		int rx = width - 265;
		int ry = 105;
		right_board->add_color_panel(width, height, rx, ry, 40, 20, UMVec4d(1.0, 0.2, 0.6, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}

	// 上腕向かって右
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("leftarm_bb_"));
		int rx = width - 145;
		int ry = 105;
		right_board->add_color_panel(width, height, rx, ry, 40, 20, UMVec4d(0.6, 0.2, 1.0, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}

	// 腕向かって左
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("rightforearm_bb_"));
		int rx = width - 310;
		int ry = 105;
		right_board->add_color_panel(width, height, rx, ry, 40, 20, UMVec4d(1.0, 0.2, 0.0, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}

	// 腕向かって右
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("leftforearm_bb_"));
		int rx = width - 100;
		int ry = 105;
		right_board->add_color_panel(width, height, rx, ry, 40, 20, UMVec4d(0.0, 0.2, 1.0, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	//// 手首向かって左
	//{
	//	UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
	//	right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("righthand_bb_"));
	//	int rx = width - 335;
	//	int ry = 105;
	//	right_board->add_color_panel(width, height, rx, ry, 20, 20, UMVec4d(0.5, 0.2, 1.0, 0.9));
	//	root_object()->mutable_children().push_back(right_board);
	//	bone_controller_list_.push_back(right_board);
	//}

	//// 手首向かって右
	//{
	//	UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
	//	right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("lefthand_bb_"));
	//	int rx = width - 55;
	//	int ry = 105;
	//	right_board->add_color_panel(width, height, rx, ry, 20, 20, UMVec4d(1.0, 0.2, 0.5, 0.9));
	//	root_object()->mutable_children().push_back(right_board);
	//	bone_controller_list_.push_back(right_board);
	//}

	// 胸
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("spine_bb_"));
		int rx = width - 200;
		int ry = 105;
		right_board->add_color_panel(width, height, rx, ry, 30, 70, UMVec4d(0.7, 0.1, 0.7, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}

	//// 腹2
	//{
	//	UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
	//	right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("spine1_bb_"));
	//	int rx = width - 200;
	//	int ry = 150;
	//	right_board->add_color_panel(width, height, rx, ry, 30, 30, UMVec4d(0.1, 0.1, 0.7, 0.9));
	//	root_object()->mutable_children().push_back(right_board);
	//	bone_controller_list_.push_back(right_board);
	//}
	//
	//// 腹1
	//{
	//	UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
	//	right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("spine_bb_"));
	//	int rx = width - 200;
	//	int ry = 185;
	//	right_board->add_color_panel(width, height, rx, ry, 30, 30, UMVec4d(0.1, 0.7, 0.1, 0.9));
	//	root_object()->mutable_children().push_back(right_board);
	//	bone_controller_list_.push_back(right_board);
	//}
	
	// 腹0
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("hips_bb_"));
		int rx = width - 220;
		int ry = 180;
		right_board->add_color_panel(width, height, rx, ry, 70, 20, UMVec4d(0.7, 0.1, 0.1, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	// 太もも向かって左
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("rightupleg_bb_"));
		int rx = width - 220;
		int ry = 205;
		right_board->add_color_panel(width, height, rx, ry, 20, 50, UMVec4d(0.7, 0.5, 0.3, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	// 太もも向かって右
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("leftupleg_bb_"));
		int rx = width - 170;
		int ry = 205;
		right_board->add_color_panel(width, height, rx, ry, 20, 50, UMVec4d(0.3, 0.5, 0.7, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}

	// ひざ下向かって左
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("rightleg_bb_"));
		int rx = width - 220;
		int ry = 260;
		right_board->add_color_panel(width, height, rx, ry, 20, 50, UMVec4d(0.7, 0.5, 0.7, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	// ひざ下向かって右
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("leftleg_bb_"));
		int rx = width - 170;
		int ry = 260;
		right_board->add_color_panel(width, height, rx, ry, 20, 50, UMVec4d(0.3, 0.3, 0.7, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	// 足首向かって左
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("rightfoot_bb_"));
		int rx = width - 220;
		int ry = 315;
		right_board->add_color_panel(width, height, rx, ry, 20, 20, UMVec4d(0.7, 0.3, 0.3, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	// 足首向かって右
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("leftfoot_bb_"));
		int rx = width - 170;
		int ry = 315;
		right_board->add_color_panel(width, height, rx, ry, 20, 20, UMVec4d(0.3, 0.7, 0.3, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	// つま先向かって左
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("righttoebase_bb_"));
		int rx = width - 220;
		int ry = 340;
		right_board->add_color_panel(width, height, rx, ry, 20, 15, UMVec4d(0.7, 0.0, 0.3, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	// つま先向かって右
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("lefttoebase_bb_"));
		int rx = width - 170;
		int ry = 340;
		right_board->add_color_panel(width, height, rx, ry, 20, 15, UMVec4d(0.3, 0.5, 0.0, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	// 手のひら向かって左
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("righthand_bb_"));
		int rx = width - 300;
		int ry = 380;
		right_board->add_color_panel(width, height, rx, ry, 75, 20, UMVec4d(1.0, 0.2, 0.5, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	// 手のひら向かって右
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("lefthand_bb_"));
		int rx = width - 145;
		int ry = 380;
		right_board->add_color_panel(width, height, rx, ry, 75, 20, UMVec4d(0.5, 0.8, 1.0, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	// 親指向かって左
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("righthandthumb1_bb_"));
		int rx = width - 220;
		int ry = 380;
		right_board->add_color_panel(width, height, rx, ry, 20, 60, UMVec4d(0.5, 0.2, 0.5, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	// 親指向かって右
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("lefthandthumb1_bb_"));
		int rx = width - 170;
		int ry = 380;
		right_board->add_color_panel(width, height, rx, ry, 20, 60, UMVec4d(1.0, 0.2, 1.0, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	// ひとさし指向かって左
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("righthandindex1_bb_"));
		int rx = width - 240;
		int ry = 405;
		right_board->add_color_panel(width, height, rx, ry, 15, 70, UMVec4d(0.5, 0.8, 0.5, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	// ひとさし指向かって右
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("lefthandindex1_bb_"));
		int rx = width - 145;
		int ry = 405;
		right_board->add_color_panel(width, height, rx, ry, 15, 70, UMVec4d(1.0, 0.2, 0.2, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	// 中指向かって左
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("righthandmiddle1_bb_"));
		int rx = width - 260;
		int ry = 405;
		right_board->add_color_panel(width, height, rx, ry, 15, 80, UMVec4d(0.2, 1.0, 0.5, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	// 中指向かって右
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("lefthandmiddle1_bb_"));
		int rx = width - 125;
		int ry = 405;
		right_board->add_color_panel(width, height, rx, ry, 15, 80, UMVec4d(0.2, 0.2, 1.0, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	// 薬指向かって左
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("righthandring1_bb_"));
		int rx = width - 280;
		int ry = 405;
		right_board->add_color_panel(width, height, rx, ry, 15, 70, UMVec4d(0.5, 0.5, 1.0, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	// 薬指向かって右
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("lefthandring1_bb_"));
		int rx = width - 105;
		int ry = 405;
		right_board->add_color_panel(width, height, rx, ry, 15, 70, UMVec4d(0.8, 0.5, 1.0, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	// 小指向かって左
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("righthandpinky1_bb_"));
		int rx = width - 300;
		int ry = 405;
		right_board->add_color_panel(width, height, rx, ry, 15, 60, UMVec4d(1.0, 1.0, 0.5, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	// 小指向かって右
	{
		UMGUIBoardPtr right_board = UMGUIBoard::create_board(-100);
		right_board->set_name(umbase::UMStringUtil::utf8_to_utf16("lefthandpinky1_bb_"));
		int rx = width - 85;
		int ry = 405;
		right_board->add_color_panel(width, height, rx, ry, 15, 60, UMVec4d(0.5, 1.0, 1.0, 0.9));
		root_object()->mutable_children().push_back(right_board);
		bone_controller_list_.push_back(right_board);
	}
	
	// connect events
	for (umgui::UMGUIBoardList::iterator it = bone_controller_list_.begin(); it != bone_controller_list_.end(); ++it)
	{
		UMGUIBoardPtr board = *it;
		mutable_event_list().push_back(board->select_event());
		board->select_event()->add_listener(self);
	}

	// 説明
	{
		int x = 300;
		int y = height - 80;
		UMGUIBoardPtr text_node = UMGUIBoard::create_board(-10);
		text_node->add_text_panel(width, height, x, y, 20, _T("[エディットモード]"));
		text_node->add_text_panel(width, height, x, y+20, 20, _T("ボーンを引っぱって右のカラーパネルにつなげてください"));
		text_node->add_text_panel(width, height, x, y+40, 20, _T("つなげ終わったらプラグインのconnectボタンを押してください"));
		root_object()->mutable_children().push_back(text_node);
	}

	left_board->add_scroll_bar(width, height, -95, UMVec4d(0.4, 0.4, 0.4, 0.5));

	return true;
}


/**
 * update event
 */
void UMMappingGUI::update(umbase::UMEventType event_type, umbase::UMAny& parameter)
{
	if (event_type == umgui::eGUIEventObjectSelected)
	{
		UMGUIBoard* boardptr = umbase::any_cast<UMGUIBoard*>(parameter);
		for (umgui::UMGUIBoardList::iterator it = bone_controller_list_.begin(); it != bone_controller_list_.end(); ++it)
		{
			UMGUIBoardPtr board = *it;
			if (board.get() == boardptr)
			{
				set_picked_controller(board);
			}
		}
	}
}

void UMMappingGUI::pick_bone_controller(double x, double y)
{
	umgui::UMGUIObjectList intersected;
	umgui::UMGUIObject::intersect(root_object(), intersected, x, y);
	bool is_find = false;
	if (!intersected.empty())
	{
		UMGUIObjectPtr obj = intersected.at(0);
		UMGUIBoardList::iterator it = std::find(bone_controller_list_.begin(), bone_controller_list_.end(), obj);
		if (it != bone_controller_list_.end())
		{
			set_picked_controller(*it);
		}
	}
}

void UMMappingGUI::set_picked_controller(umgui::UMGUIBoardPtr obj)
{
	if (picked_controller_ != obj)
	{
		picked_controller_ = obj;
		if (picked_controller_)
		{
			UMMeshPtr mesh = select_rect_->mesh();
			UMVec3d center = picked_controller_->mesh()->box().center();
			const UMMesh::Vec3dList& verts = obj->mesh()->vertex_list();
			mesh->clear_deform_cache();
			mesh->mutable_vertex_list() = verts;
			for (int i = 0, size = static_cast<int>(mesh->mutable_vertex_list().size()); i < size; ++i)
			{
				UMVec3d& v = mesh->mutable_vertex_list().at(i);
				if (v.x < center.x) {
					v.x -= 3;
				} else {
					v.x += 3;
				}
				if (v.y < center.y) {
					v.y -= 3;
				} else {
					v.y += 3;
				}
			}
			mesh->update_box();
			select_rect_->update(true);
			select_rect_->set_visible(true);
		}
	}
	if (!picked_controller_)
	{
		select_rect_->set_visible(false);
	}
}

/**
 * get picked bone controller name or empty string
 */
void UMMappingGUI::get_picked_bone_controller_name(std::u16string& name)
{
	if (picked_controller_)
	{
		name = picked_controller_->name();
	}
	else
	{
		name.clear();
	}
}

void UMMappingGUI::get_picked_bone_controller_color(UMVec4d& color)
{
	if (picked_controller_)
	{
		color =  picked_controller_->node_color();
	}
	else
	{
		color = UMVec4d(0);
	}
}

} // qumable
