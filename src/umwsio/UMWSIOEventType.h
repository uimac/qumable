/**
 * @file UMWSIOEventType.h
 *
 * @author tori31001 at gmail.com
 *
 * Copyright (C) 2013 Kazuma Hatta
 * Licensed  under the MIT license. 
 *
 */
#pragma once

#include "UMEventType.h"

namespace umwsio
{

enum {
	eWSIOEventModelLoading = 1,
	eWSIOEventModelLoaded,
	eWSIOEventConnect,
	eWSIOEventDisconnect,
	eWSIOEventDisconnecting,
	eWSIOEventReconnect,
};

} // umwsio

