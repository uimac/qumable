/**
 * @file UMDraw.h
 *
 * @author tori31001 at gmail.com
 *
 * Copyright (C) 2013 Kazuma Hatta
 * Licensed  under the MIT license. 
 *
 */
#include "UMDraw.h"
#include "UMDirectX11.h"
#include "UMOpenGL.h"

namespace umdraw
{

UMDrawPtr UMDraw::create(UMDraw::DrawType type)
{
#ifdef WITH_DIRECTX
	if (type == eDirectX)
	{
		UMDrawPtr drawer = UMDirectX11::create();
		drawer->draw_type_ = type;
		return drawer;
	}
#endif // WITH_DIRECTX

#ifdef WITH_OPENGL
	if (type == eOpenGL)
	{
		UMDrawPtr drawer = UMOpenGL::create();
		drawer->draw_type_ = type;
		return drawer;
	}
#endif //WITH_OPENGL
	return UMDrawPtr();
}

} // umdraw
