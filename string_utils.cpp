// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "core_net_pch.hpp"
#include "string_utils.hpp"
#include "socket_address.hpp"
#include "socket_address_factory.hpp"
#include "socket_util.hpp"
#include "udp_socket.hpp"
#include "memory_bit_stream.hpp"
#include "timing.hpp"
#include "weighted_timed_moving_average.hpp"
#include "robo_math.hpp"

#if !_WIN32
extern const char** __argv;
extern int __argc;
void OutputDebugString(const char* inString)
{
	printf("%s", inString);
}
#endif

string StringUtils::GetCommandLineArg(int inIndex)
{
	if (inIndex < __argc)
	{
		return string(__argv[inIndex]);
	}

	return string();
}


string StringUtils::Sprintf(const char* inFormat, ...)
{
	//not thread safe...
	static char temp[4096];

	va_list args;
	va_start(args, inFormat);

#if _WIN32
	_vsnprintf_s(temp, 4096, 4096, inFormat, args);
#else
	vsnprintf(temp, 4096, inFormat, args);
#endif
	return string(temp);
}

// void StringUtils::Log( const char* inFormat )
// {
// 	OutputDebugString( inFormat );
// 	OutputDebugString( "\n" );
// }

void StringUtils::Log(const char* inFormat, ...)
{
	char tempBuffer[4096];

	va_list args;
	va_start(args, inFormat);
	vsnprintf(tempBuffer, sizeof(tempBuffer), inFormat, args);
	va_end(args);

	OutputDebugStringA(tempBuffer);
	OutputDebugStringA("\n");
}