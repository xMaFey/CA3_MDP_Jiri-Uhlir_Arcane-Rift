// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once

#include <string>
#include <SFML/System/Clock.hpp>
#include <optional>
#include <vector>
#include <SFML/System.hpp>

#include "player_input.hpp"
#include "network_packets.hpp"
#include "udp_socket.hpp"
#include "socket_address.hpp"
#include "socket_address_factory.hpp"
#include "socket_util.hpp"
#include "memory_bit_stream.hpp"


class NetworkManager
{
public:
    enum class Mode
    {
        None,
        Host,
        Client
    };

public:
    bool start_host(unsigned short port);
    bool start_client(const std::string& ip, unsigned short port);
    void disconnect();

    bool is_connected() const;
    Mode mode() const;

    bool send_join_info(const JoinInfoPacket& joinInfo);
    std::optional<JoinInfoPacket> receive_join_info();

    bool send_input(const PlayerInput& input);
    std::optional<std::pair<int, PlayerInput>> receive_input();

    bool send_team_change_request(const TeamChangeRequestPacket& request);
    std::optional<std::pair<int, TeamChangeRequestPacket>> receive_team_change_request();

    bool send_world_state_to_player(int player_id, const WorldStatePacket& state);
    bool send_world_state_to_all(const WorldStatePacket& state);
    std::optional<WorldStatePacket> receive_world_state();

    bool send_lobby_state_to_player(int player_id, const LobbyStatePacket& state);
    bool send_lobby_state_to_all(const LobbyStatePacket& state);
    std::optional<LobbyStatePacket> receive_lobby_state();

    std::vector<int> consume_disconnected_player_ids();

private:
    struct HostClient
    {
        SocketAddress address;
        int player_id = -1;

        sf::Time last_heard = sf::Time::Zero;

        std::optional<JoinInfoPacket> pending_join_info;
        std::optional<PlayerInput> pending_input;
        std::optional<TeamChangeRequestPacket> pending_team_change_request;
    };

private:
    void poll_udp_packets();

    HostClient* find_client(const SocketAddress& address);
    HostClient* find_client_by_id(int player_id);

    bool send_packet_to_client(int player_id, const OutputMemoryBitStream& packet);
    bool send_packet_to_server(const OutputMemoryBitStream& packet);

private:
    Mode m_mode = Mode::None;

    UDPSocketPtr m_socket;
    SocketAddressPtr m_server_address;

    std::vector<HostClient> m_host_clients;
    std::vector<int> m_disconnected_player_ids;

    bool m_connected = false;
    int m_next_player_id = 1;

    sf::Clock m_clock;
};