/**
 * @file UMTime.cpp
 *
 * @author tori31001 at gmail.com
 *
 * Copyright (C) 2013 Kazuma Hatta
 * Licensed  under the MIT license. 
 *
 */

#ifdef WITH_EMSCRIPTEN
	#include <GL/glfw3.h>
#else
	#include <windows.h>
	#include <Mmsystem.h>
#endif

#include <string>

#include "UMTime.h"
#include "UMStringUtil.h"

namespace umbase
{
	
/// constructor
UMTime::UMTime(const std::string& message)
	: message_(message),
	show_message_box_(false)
{
	initial_time_ = current_time();
}

/// constructor
UMTime::UMTime(const std::string& message, bool show_message_box)
	: message_(message),
	show_message_box_(show_message_box)
{
	initial_time_ = current_time();
}

/// destructor
UMTime::~UMTime()
{
	// milliseconds
	unsigned long time = current_time() - initial_time_;

	unsigned long seconds = time / 1000;
	unsigned long mills = time - seconds * 1000;
	std::string message(
		message_ 
		+ ": " 
		+ UMStringUtil::number_to_string(seconds)
		+ "s "
		+ UMStringUtil::number_to_string(mills)
		+ "ms\n"
		);
	
#ifdef WITH_EMSCRIPTEN
	printf("%s\n", message.c_str());
#else
	::OutputDebugStringA(message.c_str());
	std::cout << message << std::endl;

	if (show_message_box_) {
		::MessageBoxA(NULL, message.c_str(), "hoge", MB_OK);
	}
#endif
}

unsigned int UMTime::current_time() 
{
#ifdef WITH_EMSCRIPTEN
	return static_cast<unsigned int>(glfwGetTime() * 1000.0);
#else
	#ifdef _WIN32
		return static_cast<unsigned int>(::timeGetTime());
	#endif
#endif
}

} // umbase
