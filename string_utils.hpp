// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once

namespace StringUtils
{
	string GetCommandLineArg(int inIndex);

	string Sprintf(const char* inFormat, ...);

	void	Log(const char* inFormat);
	void	Log(const char* inFormat, ...);
}

#ifndef LOG
#define LOG(...) StringUtils::Log(__VA_ARGS__)
#endif