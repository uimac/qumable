/**
 * @file UMQumable.h
 *
 * @author tori31001 at gmail.com
 *
 * Copyright (C) 2013 Kazuma Hatta
 * Licensed  under the MIT license. 
 *
 */
#pragma once

#include "UMMacro.h"
#include <memory>
#include <string>
#include <map>

namespace umdraw
{
class UMCamera;
typedef std::shared_ptr<UMCamera> UMCameraPtr;

class UMScene;
typedef std::shared_ptr<UMScene> UMScenePtr;

class UMNode;
typedef std::shared_ptr<UMNode> UMNodePtr;
}

namespace qumable
{

class UMQuma
{
	DISALLOW_COPY_AND_ASSIGN(UMQuma);
public:
	UMQuma();
	virtual ~UMQuma();

	/**
	 * initialize
	 */
	bool init(umdraw::UMScenePtr scene);

	/**
	 * update scene
	 */
	bool update();

	/**
	 * draw frame
	 */
	bool draw(umdraw::UMCameraPtr camera);

	/**
	 * add connection
	 */
	bool add_connection(umdraw::UMNodePtr node, const std::string& name);

	/**
	 * connect
	 */
	bool connect();

	/**
	 * save nnb xml file
	 */
	bool save_nnb(const std::u16string& path);

	/**
	 * save nnb to memory
	 */
	bool save_nnb_to_memory(std::string& buffer);
	
	/**
	 * load nnb xml file
	 */
	bool load_nnb(const std::u16string& path);

	/**
	 * load nnb from memory
	 */
	bool load_nnb_from_memory(const std::string& buffer);

	bool apply_nnb();

	const std::map<umdraw::UMNodePtr, std::string>& get_connection_map() const;

private:
	class Impl;
	typedef std::unique_ptr<Impl> ImplPtr;
	ImplPtr impl_;
};

} // qumable
