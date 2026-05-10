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


SocketAddressPtr SocketAddressFactory::CreateIPv4FromString(const string& inString)
{
	auto pos = inString.find_last_of(':');
	string host, service;
	if (pos != string::npos)
	{
		host = inString.substr(0, pos);
		service = inString.substr(pos + 1);
	}
	else
	{
		host = inString;
		//use default port...
		service = "0";
	}
	addrinfo hint;
	memset(&hint, 0, sizeof(hint));
	hint.ai_family = AF_INET;

	addrinfo* result;
	int error = getaddrinfo(host.c_str(), service.c_str(), &hint, &result);
	if (error != 0 && result != nullptr)
	{
		SocketUtil::ReportError("SocketAddressFactory::CreateIPv4FromString");
		return nullptr;
	}

	while (!result->ai_addr && result->ai_next)
	{
		result = result->ai_next;
	}

	if (!result->ai_addr)
	{
		return nullptr;
	}

	auto toRet = std::make_shared< SocketAddress >(*result->ai_addr);

	freeaddrinfo(result);

	return toRet;

}