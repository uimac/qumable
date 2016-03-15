/**
 * @file UMWSIO.h
 * websockets io
 *
 * @author tori31001 at gmail.com
 *
 * Copyright (C) 2013 Kazuma Hatta
 * Licensed  under the MIT license. 
 *
 */
#pragma once

#include <memory>
#include <map>
#include "UMMacro.h"
#include "UMVector.h"
#include "UMMathTypes.h"
#include "UMEvent.h"

namespace umdraw
{
	class UMScene;
	typedef std::shared_ptr<UMScene> UMScenePtr;
	class UMNode;
	typedef std::shared_ptr<UMNode> UMNodePtr;
} // umdraw

namespace umwsio
{

class UMWSIO;
typedef std::shared_ptr<UMWSIO> UMWSIOPtr;

/**
 * websockets io
 */
class UMWSIO 
{
	DISALLOW_COPY_AND_ASSIGN(UMWSIO);
public:
	UMWSIO();
	~UMWSIO();

	/**
	 * init
	 */
	bool init();

	/**
	 * add umdraw scene
	 */
	bool start_server(umdraw::UMScenePtr scene, int port);

	bool is_loaded() const;
	
	void done();

	const std::string& nnb() const;

	void set_nnb(const std::string& nnb);

	/**
	 * get model loaded event
	 */
	umbase::UMEventPtr model_loaded_event();

	umbase::UMEventPtr model_loading_event();
	
	umbase::UMEventPtr connect_event();
	umbase::UMEventPtr disconnect_event();
	umbase::UMEventPtr disconnecting_event();
	umbase::UMEventPtr reconnect_event();
	
	void set_connection_map(const std::map<umdraw::UMNodePtr, std::string>& connections);

private:
	class Impl;
	typedef std::unique_ptr<Impl> ImplPtr;
	ImplPtr impl_;
};

} // umwsio
