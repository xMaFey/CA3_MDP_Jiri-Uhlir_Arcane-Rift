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
    constexpr int kMaxRemoteClients = 14;
    constexpr float kClientTimeoutSeconds = 10.f;
    constexpr int kMaxPacketSize = 1500;

    // -----------------------------
    // Small stream helpers
    // -----------------------------

    void write_vec2(OutputMemoryBitStream& out, sf::Vector2f v)
    {
        out.Write(v.x);
        out.Write(v.y);
    }

    void read_vec2(InputMemoryBitStream& in, sf::Vector2f& v)
    {
        in.Read(v.x);
        in.Read(v.y);
    }

    void write_packet_type(OutputMemoryBitStream& out, PacketType type)
    {
        out.Write(static_cast<uint32_t>(type));
    }

    PacketType read_packet_type(InputMemoryBitStream& in)
    {
        uint32_t typeValue = 0;
        in.Read(typeValue);
        return static_cast<PacketType>(typeValue);
    }

    // -----------------------------
    // Individual packet serializers
    // -----------------------------

    void write_join_info(OutputMemoryBitStream& out, const JoinInfoPacket& p)
    {
        out.Write(p.player_id);
        out.Write(p.nickname);
        out.Write(p.team);
    }

    void read_join_info(InputMemoryBitStream& in, JoinInfoPacket& p)
    {
        in.Read(p.player_id);
        in.Read(p.nickname);
        in.Read(p.team);
    }

    void write_player_input(OutputMemoryBitStream& out, const PlayerInput& input)
    {
        write_vec2(out, input.move);
        out.Write(input.shootHeld);
        out.Write(input.dashPressed);
    }

    void read_player_input(InputMemoryBitStream& in, PlayerInput& input)
    {
        read_vec2(in, input.move);
        in.Read(input.shootHeld);
        in.Read(input.dashPressed);
    }

    void write_team_change_request(OutputMemoryBitStream& out, const TeamChangeRequestPacket& p)
    {
        out.Write(p.requested_team);
    }

    void read_team_change_request(InputMemoryBitStream& in, TeamChangeRequestPacket& p)
    {
        in.Read(p.requested_team);
    }

    void write_lobby_player(OutputMemoryBitStream& out, const LobbyPlayerState& p)
    {
        out.Write(p.id);
        out.Write(p.nickname);
        out.Write(p.team);
        out.Write(p.connected);
    }

    void read_lobby_player(InputMemoryBitStream& in, LobbyPlayerState& p)
    {
        in.Read(p.id);
        in.Read(p.nickname);
        in.Read(p.team);
        in.Read(p.connected);
    }

    void write_lobby_state(OutputMemoryBitStream& out, const LobbyStatePacket& state)
    {
        out.Write(state.your_player_id);
        out.Write(state.fire_count);
        out.Write(state.water_count);
        out.Write(state.spectator_count);
        out.Write(state.can_join_fire);
        out.Write(state.can_join_water);
        out.Write(state.can_spectate);
        out.Write(state.match_started);

        const uint32_t playerCount = static_cast<uint32_t>(state.players.size());
        out.Write(playerCount);

        for (const auto& player : state.players)
            write_lobby_player(out, player);
    }

    void read_lobby_state(InputMemoryBitStream& in, LobbyStatePacket& state)
    {
        uint32_t playerCount = 0;

        in.Read(state.your_player_id);
        in.Read(state.fire_count);
        in.Read(state.water_count);
        in.Read(state.spectator_count);
        in.Read(state.can_join_fire);
        in.Read(state.can_join_water);
        in.Read(state.can_spectate);
        in.Read(state.match_started);
        in.Read(playerCount);

        state.players.clear();
        state.players.reserve(playerCount);

        for (uint32_t i = 0; i < playerCount; ++i)
        {
            LobbyPlayerState player;
            read_lobby_player(in, player);
            state.players.push_back(player);
        }
    }

    void write_player_net_state(OutputMemoryBitStream& out, const PlayerNetState& p)
    {
        out.Write(p.id);
        out.Write(p.nickname);
        write_vec2(out, p.pos);
        write_vec2(out, p.dir);
        out.Write(p.team);
        out.Write(p.connected);
        out.Write(p.has_pending_team_change);
        out.Write(p.pending_team);
        out.Write(p.anim_state);
        out.Write(p.invulnerable);
        out.Write(p.invulnerable_time_seconds);
    }

    void read_player_net_state(InputMemoryBitStream& in, PlayerNetState& p)
    {
        in.Read(p.id);
        in.Read(p.nickname);
        read_vec2(in, p.pos);
        read_vec2(in, p.dir);
        in.Read(p.team);
        in.Read(p.connected);
        in.Read(p.has_pending_team_change);
        in.Read(p.pending_team);
        in.Read(p.anim_state);
        in.Read(p.invulnerable);
        in.Read(p.invulnerable_time_seconds);
    }

    void write_bullet_state(OutputMemoryBitStream& out, const BulletState& b)
    {
        out.Write(b.bullet_id);
        write_vec2(out, b.pos);
        write_vec2(out, b.dir);
        out.Write(b.owner);
        out.Write(b.spell);
    }

    void read_bullet_state(InputMemoryBitStream& in, BulletState& b)
    {
        in.Read(b.bullet_id);
        read_vec2(in, b.pos);
        read_vec2(in, b.dir);
        in.Read(b.owner);
        in.Read(b.spell);
    }

    void write_sound_event(OutputMemoryBitStream& out, const SoundEventState& s)
    {
        out.Write(s.sound_id);
        out.Write(s.source_player_id);
    }

    void read_sound_event(InputMemoryBitStream& in, SoundEventState& s)
    {
        in.Read(s.sound_id);
        in.Read(s.source_player_id);
    }

    void write_world_state(OutputMemoryBitStream& out, const WorldStatePacket& state)
    {
        out.Write(state.your_player_id);

        const uint32_t playerCount = static_cast<uint32_t>(state.players.size());
        out.Write(playerCount);

        out.Write(state.fire_count);
        out.Write(state.water_count);
        out.Write(state.spectator_count);
        out.Write(state.fire_kills);
        out.Write(state.water_kills);
        out.Write(state.match_over);
        out.Write(state.winner_team);

        for (const auto& player : state.players)
            write_player_net_state(out, player);

        const uint32_t bulletCount = static_cast<uint32_t>(state.bullets.size());
        out.Write(bulletCount);

        for (const auto& bullet : state.bullets)
            write_bullet_state(out, bullet);

        const uint32_t soundCount = static_cast<uint32_t>(state.sound_events.size());
        out.Write(soundCount);

        for (const auto& sound : state.sound_events)
            write_sound_event(out, sound);
    }

    void read_world_state(InputMemoryBitStream& in, WorldStatePacket& state)
    {
        uint32_t playerCount = 0;
        uint32_t bulletCount = 0;
        uint32_t soundCount = 0;

        in.Read(state.your_player_id);
        in.Read(playerCount);

        in.Read(state.fire_count);
        in.Read(state.water_count);
        in.Read(state.spectator_count);
        in.Read(state.fire_kills);
        in.Read(state.water_kills);
        in.Read(state.match_over);
        in.Read(state.winner_team);

        state.players.clear();
        state.players.reserve(playerCount);

        for (uint32_t i = 0; i < playerCount; ++i)
        {
            PlayerNetState player;
            read_player_net_state(in, player);
            state.players.push_back(player);
        }

        in.Read(bulletCount);

        state.bullets.clear();
        state.bullets.reserve(bulletCount);

        for (uint32_t i = 0; i < bulletCount; ++i)
        {
            BulletState bullet;
            read_bullet_state(in, bullet);
            state.bullets.push_back(bullet);
        }

        in.Read(soundCount);

        state.sound_events.clear();
        state.sound_events.reserve(soundCount);

        for (uint32_t i = 0; i < soundCount; ++i)
        {
            SoundEventState sound;
            read_sound_event(in, sound);
            state.sound_events.push_back(sound);
        }
    }
}


bool NetworkManager::start_host(unsigned short port)
{
    disconnect();

    m_mode = Mode::Host;
    m_next_player_id = 1;

    m_socket = SocketUtil::CreateUDPSocket(INET);
    if (!m_socket)
    {
        m_connected = false;
        m_mode = Mode::None;
        return false;
    }

    SocketAddress bindAddress(INADDR_ANY, port);

    if (m_socket->Bind(bindAddress) != NO_ERROR)
    {
        m_socket.reset();
        m_connected = false;
        m_mode = Mode::None;
        return false;
    }

    m_socket->SetNonBlockingMode(true);
    m_connected = true;

    std::cout << "RoboCat UDP host started on port " << port << "\n";
    return true;
}

bool NetworkManager::start_client(const std::string& ip, unsigned short port)
{
    disconnect();

    m_mode = Mode::Client;

    m_socket = SocketUtil::CreateUDPSocket(INET);
    if (!m_socket)
    {
        m_connected = false;
        m_mode = Mode::None;
        return false;
    }

    SocketAddress bindAddress(INADDR_ANY, 0);

    if (m_socket->Bind(bindAddress) != NO_ERROR)
    {
        m_socket.reset();
        m_connected = false;
        m_mode = Mode::None;
        return false;
    }

    m_socket->SetNonBlockingMode(true);

    const std::string addressText = ip + ":" + std::to_string(port);
    m_server_address = SocketAddressFactory::CreateIPv4FromString(addressText);

    if (!m_server_address)
    {
        m_socket.reset();
        m_connected = false;
        m_mode = Mode::None;
        return false;
    }

    m_connected = true;

    std::cout << "RoboCat UDP client ready. Server is " << addressText << "\n";
    return true;
}

void NetworkManager::disconnect()
{
    m_socket.reset();
    m_server_address.reset();

    m_host_clients.clear();
    m_disconnected_player_ids.clear();

    m_connected = false;
    m_mode = Mode::None;
    m_next_player_id = 1;
}

bool NetworkManager::is_connected() const
{
    return m_connected;
}

NetworkManager::Mode NetworkManager::mode() const
{
    return m_mode;
}

NetworkManager::HostClient* NetworkManager::find_client(const SocketAddress& address)
{
    for (auto& client : m_host_clients)
    {
        if (client.address == address)
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

// Host-side packet pump.
// Reads raw UDP packets using RoboCat UDPSocket::ReceiveFrom,
// unpacks them with InputMemoryBitStream, and stores the newest message
// for GameState / TeamSelectState to consume.
void NetworkManager::poll_udp_packets()
{
    if (m_mode != Mode::Host || !m_connected || !m_socket)
        return;

    while (true)
    {
        char packetMemory[kMaxPacketSize];
        SocketAddress senderAddress;

        const int bytesRead =
            m_socket->ReceiveFrom(packetMemory, sizeof(packetMemory), senderAddress);

        if (bytesRead == 0)
            break;

        if (bytesRead < 0)
            continue;

        if (bytesRead < static_cast<int>(sizeof(uint32_t)))
            continue;

        InputMemoryBitStream in(packetMemory, static_cast<uint32_t>(bytesRead * 8));
        const PacketType type = read_packet_type(in);

        HostClient* client = find_client(senderAddress);

        if (type == PacketType::JoinInfo)
        {
            if (!client)
            {
                if (static_cast<int>(m_host_clients.size()) >= kMaxRemoteClients)
                {
                    std::cout << "UDP join rejected: server full\n";
                    continue;
                }

                HostClient newClient;
                newClient.address = senderAddress;
                newClient.player_id = m_next_player_id++;
                newClient.last_heard = m_clock.getElapsedTime();

                m_host_clients.push_back(newClient);
                client = &m_host_clients.back();

                std::cout << "RoboCat UDP client joined from "
                    << senderAddress.ToString()
                    << " as id=" << client->player_id << "\n";
            }

            JoinInfoPacket joinInfo;
            read_join_info(in, joinInfo);

            joinInfo.player_id = client->player_id;
            client->pending_join_info = joinInfo;
            client->last_heard = m_clock.getElapsedTime();
        }
        else
        {
            if (!client)
                continue;

            client->last_heard = m_clock.getElapsedTime();

            if (type == PacketType::PlayerInput)
            {
                PlayerInput input;
                read_player_input(in, input);
                client->pending_input = input;
            }
            else if (type == PacketType::TeamChangeRequest)
            {
                TeamChangeRequestPacket request;
                read_team_change_request(in, request);
                client->pending_team_change_request = request;
            }
        }
    }

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

bool NetworkManager::send_packet_to_server(const OutputMemoryBitStream& packet)
{
    if (m_mode != Mode::Client || !m_connected || !m_socket || !m_server_address)
        return false;

    if (packet.GetByteLength() > kMaxPacketSize)
    {
        std::cout << "Packet too large for UDP send: "
            << packet.GetByteLength() << " bytes\n";
        return false;
    }

    return m_socket->SendTo(
        packet.GetBufferPtr(),
        static_cast<int>(packet.GetByteLength()),
        *m_server_address
    ) > 0;
}

bool NetworkManager::send_packet_to_client(int player_id, const OutputMemoryBitStream& packet)
{
    if (m_mode != Mode::Host || !m_connected || !m_socket)
        return false;

    HostClient* client = find_client_by_id(player_id);
    if (!client)
        return false;

    if (packet.GetByteLength() > kMaxPacketSize)
    {
        std::cout << "Packet too large for UDP send: "
            << packet.GetByteLength() << " bytes\n";
        return false;
    }

    return m_socket->SendTo(
        packet.GetBufferPtr(),
        static_cast<int>(packet.GetByteLength()),
        client->address
    ) > 0;
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
    OutputMemoryBitStream out;
    write_packet_type(out, PacketType::JoinInfo);
    write_join_info(out, joinInfo);

    return send_packet_to_server(out);
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
    OutputMemoryBitStream out;
    write_packet_type(out, PacketType::PlayerInput);
    write_player_input(out, input);

    return send_packet_to_server(out);
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
    OutputMemoryBitStream out;
    write_packet_type(out, PacketType::TeamChangeRequest);
    write_team_change_request(out, request);

    return send_packet_to_server(out);
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
    OutputMemoryBitStream out;
    write_packet_type(out, PacketType::WorldState);
    write_world_state(out, state);

    return send_packet_to_client(player_id, out);
}

bool NetworkManager::send_world_state_to_all(const WorldStatePacket& state)
{
    if (m_mode != Mode::Host || !m_connected)
        return false;

    bool sentAny = false;

    for (const auto& client : m_host_clients)
    {
        OutputMemoryBitStream out;
        write_packet_type(out, PacketType::WorldState);
        write_world_state(out, state);

        if (send_packet_to_client(client.player_id, out))
            sentAny = true;
    }

    return sentAny;
}

std::optional<WorldStatePacket> NetworkManager::receive_world_state()
{
    if (m_mode != Mode::Client || !m_connected || !m_socket)
        return std::nullopt;

    while (true)
    {
        char packetMemory[kMaxPacketSize];
        SocketAddress senderAddress;

        const int bytesRead =
            m_socket->ReceiveFrom(packetMemory, sizeof(packetMemory), senderAddress);

        if (bytesRead == 0)
            return std::nullopt;

        if (bytesRead < 0)
            continue;

        if (bytesRead < static_cast<int>(sizeof(uint32_t)))
            continue;

        InputMemoryBitStream in(packetMemory, static_cast<uint32_t>(bytesRead * 8));
        const PacketType type = read_packet_type(in);

        if (type != PacketType::WorldState)
            continue;

        WorldStatePacket state;
        read_world_state(in, state);
        return state;
    }
}

bool NetworkManager::send_lobby_state_to_player(int player_id, const LobbyStatePacket& state)
{
    OutputMemoryBitStream out;
    write_packet_type(out, PacketType::LobbyState);
    write_lobby_state(out, state);

    return send_packet_to_client(player_id, out);
}

bool NetworkManager::send_lobby_state_to_all(const LobbyStatePacket& state)
{
    if (m_mode != Mode::Host || !m_connected)
        return false;

    bool sentAny = false;

    for (const auto& client : m_host_clients)
    {
        OutputMemoryBitStream out;
        write_packet_type(out, PacketType::LobbyState);
        write_lobby_state(out, state);

        if (send_packet_to_client(client.player_id, out))
            sentAny = true;
    }

    return sentAny;
}

std::optional<LobbyStatePacket> NetworkManager::receive_lobby_state()
{
    if (m_mode != Mode::Client || !m_connected || !m_socket)
        return std::nullopt;

    while (true)
    {
        char packetMemory[kMaxPacketSize];
        SocketAddress senderAddress;

        const int bytesRead =
            m_socket->ReceiveFrom(packetMemory, sizeof(packetMemory), senderAddress);

        if (bytesRead == 0)
            return std::nullopt;

        if (bytesRead < 0)
            continue;

        if (bytesRead < static_cast<int>(sizeof(uint32_t)))
            continue;

        InputMemoryBitStream in(packetMemory, static_cast<uint32_t>(bytesRead * 8));
        const PacketType type = read_packet_type(in);

        if (type != PacketType::LobbyState)
            continue;

        LobbyStatePacket state;
        read_lobby_state(in, state);
        return state;
    }
}