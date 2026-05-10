// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once

#include "core_net_pch.hpp"
#include "socket_address.hpp"

class SocketAddressFactory
{
public:
	static SocketAddressPtr CreateIPv4FromString(const string& inString);
};