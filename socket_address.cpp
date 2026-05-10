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


string SocketAddress::ToString() const
{
    const sockaddr_in* s = GetAsSockAddrIn();

    char destinationBuffer[128] = {};

    inet_ntop(
        AF_INET,
        &(s->sin_addr),
        destinationBuffer,
        sizeof(destinationBuffer)
    );

    return StringUtils::Sprintf(
        "%s:%d",
        destinationBuffer,
        ntohs(s->sin_port)
    );
}