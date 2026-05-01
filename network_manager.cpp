// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "network_manager.hpp"
#include <algorithm>
#include <iostream>
#include <cstdint>

namespace
{
    constexpr int kMaxRemoteClients = 14; // host is player 0, so 14 remotes = 15 players total
    constexpr float kClientTimeoutSeconds = 10.f;

    sf::Packet& operator<<(sf::Packet& packet, const JoinInfoPacket& joinInfo)
    {
        return packet << joinInfo.player_id << joinInfo.nickname << joinInfo.team;
    }

    sf::Packet& operator>>(sf::Packet& packet, JoinInfoPacket& joinInfo)
    {
        return packet >> joinInfo.player_id >> joinInfo.nickname >> joinInfo.team;
    }

    sf::Packet& operator<<(sf::Packet& packet, const TeamChangeRequestPacket& request)
    {
        return packet << request.requested_team;
    }

    sf::Packet& operator>>(sf::Packet& packet, TeamChangeRequestPacket& request)
    {
        return packet >> request.requested_team;
    }

    sf::Packet& operator<<(sf::Packet& packet, const LobbyPlayerState& p)
    {
        return packet << p.id << p.nickname << p.team << p.connected;
    }

    sf::Packet& operator>>(sf::Packet& packet, LobbyPlayerState& p)
    {
        return packet >> p.id >> p.nickname >> p.team >> p.connected;
    }

    sf::Packet& operator<<(sf::Packet& packet, const LobbyStatePacket& state)
    {
        packet
            << state.your_player_id
            << state.fire_count
            << state.water_count
            << state.spectator_count
            << state.can_join_fire
            << state.can_join_water
            << state.can_spectate
            << state.match_started
            << static_cast<uint32_t>(state.players.size());

        for (const auto& p : state.players)
            packet << p;

        return packet;
    }

    sf::Packet& operator>>(sf::Packet& packet, LobbyStatePacket& state)
    {
        uint32_t playerCount = 0;

        packet
            >> state.your_player_id
            >> state.fire_count
            >> state.water_count
            >> state.spectator_count
            >> state.can_join_fire
            >> state.can_join_water
            >> state.can_spectate
            >> state.match_started
            >> playerCount;

        state.players.clear();

        for (uint32_t i = 0; i < playerCount; ++i)
        {
            LobbyPlayerState p;
            packet >> p;
            state.players.push_back(p);
        }

        return packet;
    }

    sf::Packet& operator<<(sf::Packet& packet, const PlayerNetState& p)
    {
        return packet
            << p.id
            << p.nickname
            << p.pos.x << p.pos.y
            << p.dir.x << p.dir.y
            << p.team
            << p.connected
            << p.has_pending_team_change
            << p.pending_team
            << p.anim_state
            << p.invulnerable
            << p.invulnerable_time_seconds;
    }

    sf::Packet& operator>>(sf::Packet& packet, PlayerNetState& p)
    {
        return packet
            >> p.id
            >> p.nickname
            >> p.pos.x >> p.pos.y
            >> p.dir.x >> p.dir.y
            >> p.team
            >> p.connected
            >> p.has_pending_team_change
            >> p.pending_team
            >> p.anim_state
            >> p.invulnerable
            >> p.invulnerable_time_seconds;
    }

    sf::Packet& operator<<(sf::Packet& packet, const SoundEventState& s)
    {
        return packet << s.sound_id << s.source_player_id;
    }

    sf::Packet& operator>>(sf::Packet& packet, SoundEventState& s)
    {
        return packet >> s.sound_id >> s.source_player_id;
    }

    sf::Packet& operator<<(sf::Packet& packet, const PlayerInput& input)
    {
        return packet << input.move.x << input.move.y << input.shootHeld << input.dashPressed;
    }

    sf::Packet& operator>>(sf::Packet& packet, PlayerInput& input)
    {
        return packet >> input.move.x >> input.move.y >> input.shootHeld >> input.dashPressed;
    }

    sf::Packet& operator<<(sf::Packet& packet, const BulletState& b)
    {
        return packet
            << b.bullet_id
            << b.pos.x << b.pos.y
            << b.dir.x << b.dir.y
            << b.owner
            << b.spell;
    }

    sf::Packet& operator>>(sf::Packet& packet, BulletState& b)
    {
        return packet
            >> b.bullet_id
            >> b.pos.x >> b.pos.y
            >> b.dir.x >> b.dir.y
            >> b.owner
            >> b.spell;
    }

    sf::Packet& operator<<(sf::Packet& packet, const WorldStatePacket& state)
    {
        packet
            << state.your_player_id
            << static_cast<uint32_t>(state.players.size())
            << state.fire_count
            << state.water_count
            << state.spectator_count
            << state.fire_kills
            << state.water_kills;

        for (const auto& p : state.players)
            packet << p;

        packet << static_cast<uint32_t>(state.bullets.size());

        for (const auto& b : state.bullets)
            packet << b;

        packet << static_cast<uint32_t>(state.sound_events.size());

        for (const auto& s : state.sound_events)
            packet << s;

        return packet;
    }

    sf::Packet& operator>>(sf::Packet& packet, WorldStatePacket& state)
    {
        uint32_t playerCount = 0;

        packet
            >> state.your_player_id
            >> playerCount
            >> state.fire_count
            >> state.water_count
            >> state.spectator_count
            >> state.fire_kills
            >> state.water_kills;

        state.players.clear();

        for (uint32_t i = 0; i < playerCount; ++i)
        {
            PlayerNetState p;
            packet >> p;
            state.players.push_back(p);
        }

        uint32_t bulletCount = 0;
        packet >> bulletCount;

        state.bullets.clear();

        for (uint32_t i = 0; i < bulletCount; ++i)
        {
            BulletState b;
            packet >> b;
            state.bullets.push_back(b);
        }

        uint32_t soundCount = 0;
        packet >> soundCount;

        state.sound_events.clear();

        for (uint32_t i = 0; i < soundCount; ++i)
        {
            SoundEventState s;
            packet >> s;
            state.sound_events.push_back(s);
        }

        return packet;
    }

    sf::Packet make_typed_packet(PacketType type)
    {
        sf::Packet packet;
        packet << static_cast<int>(type);
        return packet;
    }

    std::optional<PacketType> read_packet_type(sf::Packet& packet)
    {
        int typeValue = 0;

        if (!(packet >> typeValue))
            return std::nullopt;

        return static_cast<PacketType>(typeValue);
    }
}

bool NetworkManager::start_host(unsigned short port)
{
    disconnect();

    m_mode = Mode::Host;
    m_next_player_id = 1;

    // UDP host binds to a known port so clients know where to send packets.
    if (m_socket.bind(port) != sf::Socket::Status::Done)
    {
        m_connected = false;
        m_mode = Mode::None;
        return false;
    }

    m_socket.setBlocking(false);
    m_connected = true;

    std::cout << "UDP host started on port " << port << "\n";
    return true;
}

bool NetworkManager::start_client(const sf::IpAddress& ip, unsigned short port)
{
    disconnect();

    m_mode = Mode::Client;
    m_server_ip = ip;
    m_server_port = port;

    // UDP has no real connection step.
    // The client binds to any free local port and then sends packets to the host.
    if (m_socket.bind(sf::Socket::AnyPort) != sf::Socket::Status::Done)
    {
        m_connected = false;
        m_mode = Mode::None;
        return false;
    }

    m_socket.setBlocking(false);
    m_connected = true;

    std::cout << "UDP client ready. Server is " << ip << ":" << port << "\n";
    return true;
}

void NetworkManager::disconnect()
{
    m_socket.unbind();

    m_host_clients.clear();
    m_disconnected_player_ids.clear();

    m_connected = false;
    m_mode = Mode::None;
    m_next_player_id = 1;
    m_server_port = 0;
}

bool NetworkManager::is_connected() const
{
    return m_connected;
}

NetworkManager::Mode NetworkManager::mode() const
{
    return m_mode;
}

NetworkManager::HostClient* NetworkManager::find_client(const sf::IpAddress& ip, unsigned short port)
{
    for (auto& client : m_host_clients)
    {
        if (client.ip == ip && client.port == port)
            return &client;
    }

    return nullptr;
}

NetworkManager::HostClient* NetworkManager::find_client_by_id(int player_id)
{
    for (auto& client : m_host_clients)
    {
        if (client.player_id == player_id)
            return &client;
    }

    return nullptr;
}

void NetworkManager::poll_udp_packets()
{
    if (m_mode != Mode::Host)
        return;

    while (true)
    {
        sf::Packet packet;
        std::optional<sf::IpAddress> sender;
        unsigned short senderPort = 0;

        const auto status = m_socket.receive(packet, sender, senderPort);

        // No UDP packet waiting right now.
        if (status != sf::Socket::Status::Done)
            break;

        // Safety check: SFML 3 stores sender IP as optional.
        if (!sender.has_value())
            continue;

        const auto type = read_packet_type(packet);

        if (!type.has_value())
            continue;

        HostClient* client = find_client(*sender, senderPort);

        // JoinInfo is the only packet allowed to create a new client.
        if (*type == PacketType::JoinInfo)
        {
            if (!client)
            {
                if (static_cast<int>(m_host_clients.size()) >= kMaxRemoteClients)
                {
                    std::cout << "UDP join rejected: server full\n";
                    continue;
                }

                HostClient newClient;
                newClient.ip = *sender;
                newClient.port = senderPort;
                newClient.player_id = m_next_player_id++;
                newClient.last_heard = m_clock.getElapsedTime();

                m_host_clients.push_back(newClient);
                client = &m_host_clients.back();

                std::cout << "UDP client joined from " << *sender
                    << ":" << senderPort
                    << " as id=" << client->player_id << "\n";
            }

            JoinInfoPacket joinInfo;
            packet >> joinInfo;

            joinInfo.player_id = client->player_id;
            client->pending_join_info = joinInfo;
            client->last_heard = m_clock.getElapsedTime();
        }
        else
        {
            // Ignore non-join packets from unknown UDP endpoints.
            if (!client)
                continue;

            client->last_heard = m_clock.getElapsedTime();

            if (*type == PacketType::PlayerInput)
            {
                PlayerInput input;
                packet >> input;
                client->pending_input = input;
            }
            else if (*type == PacketType::TeamChangeRequest)
            {
                TeamChangeRequestPacket request;
                packet >> request;
                client->pending_team_change_request = request;
            }
        }
    }

    // UDP does not tell us when someone disconnects.
    // So we treat a client as disconnected if we hear nothing for a while.
    const sf::Time now = m_clock.getElapsedTime();

    m_host_clients.erase(
        std::remove_if(
            m_host_clients.begin(),
            m_host_clients.end(),
            [&](const HostClient& client)
            {
                const bool timedOut =
                    (now - client.last_heard).asSeconds() > kClientTimeoutSeconds;

                if (timedOut)
                {
                    m_disconnected_player_ids.push_back(client.player_id);

                    std::cout << "UDP client timed out: id="
                        << client.player_id << "\n";
                }

                return timedOut;
            }),
        m_host_clients.end()
    );
}

bool NetworkManager::send_packet_to_client(int player_id, sf::Packet& packet)
{
    if (m_mode != Mode::Host || !m_connected)
        return false;

    HostClient* client = find_client_by_id(player_id);

    if (!client)
        return false;

    if (!client->ip.has_value())
        return false;

    return m_socket.send(packet, *client->ip, client->port) == sf::Socket::Status::Done;
}

bool NetworkManager::send_packet_to_server(sf::Packet& packet)
{
    if (m_mode != Mode::Client || !m_connected)
        return false;

    if (!m_server_ip.has_value())
        return false;

    return m_socket.send(packet, *m_server_ip, m_server_port) == sf::Socket::Status::Done;
}

std::vector<int> NetworkManager::consume_disconnected_player_ids()
{
    poll_udp_packets();

    std::vector<int> out = std::move(m_disconnected_player_ids);
    m_disconnected_player_ids.clear();
    return out;
}

bool NetworkManager::send_join_info(const JoinInfoPacket& joinInfo)
{
    sf::Packet packet = make_typed_packet(PacketType::JoinInfo);
    packet << joinInfo;

    return send_packet_to_server(packet);
}

std::optional<JoinInfoPacket> NetworkManager::receive_join_info()
{
    if (m_mode != Mode::Host)
        return std::nullopt;

    poll_udp_packets();

    for (auto& client : m_host_clients)
    {
        if (!client.pending_join_info.has_value())
            continue;

        JoinInfoPacket joinInfo = *client.pending_join_info;
        client.pending_join_info.reset();
        return joinInfo;
    }

    return std::nullopt;
}

bool NetworkManager::send_input(const PlayerInput& input)
{
    sf::Packet packet = make_typed_packet(PacketType::PlayerInput);
    packet << input;

    return send_packet_to_server(packet);
}

std::optional<std::pair<int, PlayerInput>> NetworkManager::receive_input()
{
    if (m_mode != Mode::Host)
        return std::nullopt;

    poll_udp_packets();

    for (auto& client : m_host_clients)
    {
        if (!client.pending_input.has_value())
            continue;

        PlayerInput input = *client.pending_input;
        client.pending_input.reset();

        return std::make_pair(client.player_id, input);
    }

    return std::nullopt;
}

bool NetworkManager::send_team_change_request(const TeamChangeRequestPacket& request)
{
    sf::Packet packet = make_typed_packet(PacketType::TeamChangeRequest);
    packet << request;

    return send_packet_to_server(packet);
}

std::optional<std::pair<int, TeamChangeRequestPacket>> NetworkManager::receive_team_change_request()
{
    if (m_mode != Mode::Host)
        return std::nullopt;

    poll_udp_packets();

    for (auto& client : m_host_clients)
    {
        if (!client.pending_team_change_request.has_value())
            continue;

        TeamChangeRequestPacket request = *client.pending_team_change_request;
        client.pending_team_change_request.reset();

        return std::make_pair(client.player_id, request);
    }

    return std::nullopt;
}

bool NetworkManager::send_world_state_to_player(int player_id, const WorldStatePacket& state)
{
    sf::Packet packet = make_typed_packet(PacketType::WorldState);
    packet << state;

    // Useful for CA3 screencast bandwidth discussion.
    // std::cout << "[UDP] WorldState size: " << packet.getDataSize() << " bytes\n";

    return send_packet_to_client(player_id, packet);
}

bool NetworkManager::send_world_state_to_all(const WorldStatePacket& state)
{
    if (m_mode != Mode::Host || !m_connected)
        return false;

    bool sentAny = false;

    for (const auto& client : m_host_clients)
    {
        sf::Packet packet = make_typed_packet(PacketType::WorldState);
        packet << state;

        if (client.ip.has_value() &&
            m_socket.send(packet, *client.ip, client.port) == sf::Socket::Status::Done)
            sentAny = true;
    }

    return sentAny;
}

std::optional<WorldStatePacket> NetworkManager::receive_world_state()
{
    if (m_mode != Mode::Client || !m_connected)
        return std::nullopt;

    while (true)
    {
        sf::Packet packet;
        std::optional<sf::IpAddress> sender;
        unsigned short senderPort = 0;

        const auto status = m_socket.receive(packet, sender, senderPort);

        // No world state packet available this frame.
        if (status != sf::Socket::Status::Done)
            return std::nullopt;

        // Safety check for SFML 3 optional sender IP.
        if (!sender.has_value())
            continue;

        const auto type = read_packet_type(packet);

        if (!type.has_value())
            continue;

        if (*type != PacketType::WorldState)
            continue;

        WorldStatePacket state;
        packet >> state;
        return state;
    }
}

bool NetworkManager::send_lobby_state_to_player(int player_id, const LobbyStatePacket& state)
{
    sf::Packet packet = make_typed_packet(PacketType::LobbyState);
    packet << state;

    return send_packet_to_client(player_id, packet);
}

bool NetworkManager::send_lobby_state_to_all(const LobbyStatePacket& state)
{
    if (m_mode != Mode::Host || !m_connected)
        return false;

    bool sentAny = false;

    for (const auto& client : m_host_clients)
    {
        sf::Packet packet = make_typed_packet(PacketType::LobbyState);
        packet << state;

        if (client.ip.has_value() &&
            m_socket.send(packet, *client.ip, client.port) == sf::Socket::Status::Done)
            sentAny = true;
    }

    return sentAny;
}

std::optional<LobbyStatePacket> NetworkManager::receive_lobby_state()
{
    if (m_mode != Mode::Client || !m_connected)
        return std::nullopt;

    while (true)
    {
        sf::Packet packet;
        std::optional<sf::IpAddress> sender;
        unsigned short senderPort = 0;

        const auto status = m_socket.receive(packet, sender, senderPort);

        // No world state packet available this frame.
        if (status != sf::Socket::Status::Done)
            return std::nullopt;

        // Safety check for SFML 3 optional sender IP.
        if (!sender.has_value())
            continue;

        const auto type = read_packet_type(packet);

        if (!type.has_value())
            continue;

        if (*type != PacketType::LobbyState)
            continue;

        LobbyStatePacket state;
        packet >> state;
        return state;
    }
}