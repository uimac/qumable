/**
 * @file UMResource.cpp
 * resource creator
 *
 * @author tori31001 at gmail.com
 *
 * Copyright (C) 2013 Kazuma Hatta
 * Licensed  under the MIT license. 
 *
 */

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <snappy.h>
#include <exception>
#include <assert.h>
#include "UMResource.h"
#include "UMPath.h"
#include "UMStringUtil.h"
#include "UMTime.h"

using namespace umbase;

static umstring resource_path(const std::string& file_name)
{
	return UMPath::resource_absolute_path(UMStringUtil::utf8_to_utf16(file_name));
}

// main
int main()
{
	typedef std::vector<umstring> FileList;
	FileList files;
	
	files.push_back(resource_path("UMModelShader.vs"));
	files.push_back(resource_path("UMModelShader.fs"));
	files.push_back(resource_path("UMPointShader.vs"));
	files.push_back(resource_path("UMPointShader.fs"));
	files.push_back(resource_path("UMBoardShader.vs"));
	files.push_back(resource_path("UMBoardShader.fs"));

	files.push_back(resource_path("KodomoRounded.ttf"));
	
	umstring out_file = resource_path("cabbage_resource.pack");

	umresource::UMResource::instance().pack(out_file, files);

	// unpack test
	umresource::UMResource::instance().unpack_to_memory(out_file);
	
	return 0;
}
