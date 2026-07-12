#include "Network.h"
#include <spdlog/spdlog.h>
#include <libultraship/libultraship.h>
#include <chrono>
#include <cstring>

// MARK: - Public

void Network::Enable(const char* host, uint16_t port) {
#ifdef ENABLE_REMOTE_CONTROL
    if (isEnabled) {
        return;
    }

    if (SDLNet_ResolveHost(&networkAddress, host, port) == -1) {
        SPDLOG_ERROR("[Network] SDLNet_ResolveHost: {}", SDLNet_GetError());
        return;
    }

    isEnabled = true;

    // First check if there is a thread running, if so, join it
    if (receiveThread.joinable()) {
        receiveThread.join();
    }

    receiveThread = std::thread(&Network::ReceiveFromServer, this);
#endif
}

void Network::Disable() {
    if (!isEnabled) {
        return;
    }

    isEnabled = false;
    if (receiveThread.joinable()) {
        receiveThread.join();
    }
}

void Network::OnIncomingData(char payload[512]) {
}

void Network::OnIncomingJson(nlohmann::json payload) {
}

void Network::OnConnected() {
}

void Network::OnDisconnected() {
}

void Network::ProcessOutgoingPackets() {
}

void Network::SendDataToRemote(const char* payload) {
#ifdef ENABLE_REMOTE_CONTROL
    if (networkSocket == nullptr || payload == nullptr) {
        return;
    }

    size_t bytesRemaining = strlen(payload) + 1;
    const char* cursor = payload;
    SPDLOG_TRACE("[Network] Sending {} bytes", bytesRemaining);
    while (bytesRemaining > 0 && isConnected && isEnabled) {
        int sent = SDLNet_TCP_Send(networkSocket, cursor, static_cast<int>(bytesRemaining));
        if (sent <= 0) {
            SPDLOG_ERROR("[Network] SDLNet_TCP_Send: {}", SDLNet_GetError());
            socketError = true;
            return;
        }
        cursor += sent;
        bytesRemaining -= static_cast<size_t>(sent);
    }
#endif
}

void Network::SendJsonToRemote(nlohmann::json payload) {
    SendDataToRemote(payload.dump().c_str());
}

// MARK: - Private

void Network::ReceiveFromServer() {
#ifdef ENABLE_REMOTE_CONTROL
    constexpr size_t MAX_NETWORK_FRAME_SIZE = 1024 * 1024;

    while (isEnabled) {
        while (!isConnected && isEnabled) {
            SPDLOG_TRACE("[Network] Attempting to make connection to server...");
            networkSocket = SDLNet_TCP_Open(&networkAddress);

            if (networkSocket) {
                isConnected = true;
                socketError = false;
                receivedData.clear();
                SPDLOG_INFO("[Network] Connection to server established!");

                OnConnected();
                break;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        SDLNet_SocketSet socketSet = SDLNet_AllocSocketSet(1);
        if (socketSet == nullptr) {
            SPDLOG_ERROR("[Network] SDLNet_AllocSocketSet: {}", SDLNet_GetError());
        } else if (networkSocket && SDLNet_TCP_AddSocket(socketSet, networkSocket) == -1) {
            SPDLOG_ERROR("[Network] SDLNet_TCP_AddSocket: {}", SDLNet_GetError());
        }

        // Listen to socket messages
        while (isConnected && networkSocket && isEnabled && !socketError) {
            // we check first if socket has data, to not block in the TCP_Recv
            int socketsReady = socketSet != nullptr ? SDLNet_CheckSockets(socketSet, 10) : 0;

            if (socketsReady == -1) {
                SPDLOG_ERROR("[Network] SDLNet_CheckSockets: {}", SDLNet_GetError());
                break;
            }

            // Always process outgoing packets
            ProcessOutgoingPackets();

            if (socketsReady == 0) {
                // No incoming data
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            char remoteDataReceived[512];
            memset(remoteDataReceived, 0, sizeof(remoteDataReceived));
            int len = SDLNet_TCP_Recv(networkSocket, &remoteDataReceived, sizeof(remoteDataReceived));
            if (!len || !networkSocket || len == -1) {
                SPDLOG_ERROR("[Network] SDLNet_TCP_Recv: {}", SDLNet_GetError());
                break;
            }

            HandleRemoteData(remoteDataReceived);

            receivedData.append(remoteDataReceived, len);
            if (receivedData.size() > MAX_NETWORK_FRAME_SIZE && receivedData.find('\0') == std::string::npos) {
                SPDLOG_ERROR("[Network] Incoming frame exceeded {} bytes", MAX_NETWORK_FRAME_SIZE);
                socketError = true;
                break;
            }

            // Proess all complete packets
            size_t delimiterPos = receivedData.find('\0');
            while (delimiterPos != std::string::npos) {
                if (delimiterPos > MAX_NETWORK_FRAME_SIZE) {
                    SPDLOG_ERROR("[Network] Incoming frame exceeded {} bytes", MAX_NETWORK_FRAME_SIZE);
                    receivedData.clear();
                    socketError = true;
                    break;
                }
                // Extract the complete packet until the delimiter
                std::string packet = receivedData.substr(0, delimiterPos);
                // Remove the packet (including the delimiter) from the received data
                receivedData.erase(0, delimiterPos + 1);
                HandleRemoteJson(packet);
                // Find the next delimiter
                delimiterPos = receivedData.find('\0');
            }
        }

        if (socketSet) {
            SDLNet_FreeSocketSet(socketSet);
        }

        if (isConnected) {
            SDLNet_TCP_Close(networkSocket);
            networkSocket = nullptr;
            isConnected = false;
            receivedData.clear();
            OnDisconnected();
            SPDLOG_INFO("[Network] Ending receiving thread...");
        }
    }
#endif
}

void Network::HandleRemoteData(char payload[512]) {
    OnIncomingData(payload);
}

void Network::HandleRemoteJson(std::string payload) {
    SPDLOG_TRACE("[Network] Received {} bytes of JSON", payload.size());
    nlohmann::json jsonPayload;
    try {
        jsonPayload = nlohmann::json::parse(payload);
    } catch (const std::exception& e) {
        SPDLOG_ERROR("[Network] Failed to parse json: \n{}\n{}\n", payload, e.what());
        return;
    }

    OnIncomingJson(jsonPayload);
}
