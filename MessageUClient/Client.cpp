// main.cpp
// author: Ariel Cohen ID: 329599187

#include "Client.h"
#include <iostream>
#include <iomanip>

// Constructor: Initializes communicator and loads user info if available
Client::Client() {
    auto serverInfo = FileHandler::readServerInfo();
    if (!serverInfo) {
        throw std::runtime_error("server.info file not found or is invalid.");
    }
    _communicator = std::make_unique<Communicator>(serverInfo->ip, serverInfo->port);

    _userInfo = FileHandler::readMyInfo();
    if (_userInfo) {
        CryptoWrapper::base64ToPrivateKey(_userInfo->privateKey, _privateKey);
    }
}

void Client::run() {
    int choice = -1;
    while (choice != 0) {
        showMenu();
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear buffer

        switch (choice) {
        case 110: handleRegister(); break;
        case 120: handleRequestClientsList(); break;
        case 130: handleRequestPublicKey(); break;
        case 140: handleRequestWaitingMessages(); break;
        case 150: handleSendTextMessage(); break;
        case 151: handleSendSymKeyRequest(); break;
        case 152: handleSendSymKey(); break;
        case 0: std::cout << "Exiting..." << std::endl; break;
        default: std::cout << "Invalid option." << std::endl; break;
        }
    }
}

void Client::showMenu() {
    std::cout << "\nMessageU client at your service.\n\n";
    std::cout << "110) Register\n";
    std::cout << "120) Request for clients list\n";
    std::cout << "130) Request for public key\n";
    std::cout << "140) Request for waiting messages\n";
    std::cout << "150) Send a text message\n";
    std::cout << "151) Send a request for symmetric key\n";
    std::cout << "152) Send your symmetric key\n";
    std::cout << "0) Exit client\n";
    std::cout << "? ";
}

void Client::handleRegister() {
    if (FileHandler::myInfoExists()) {
        std::cerr << "Error: my.info file already exists. Cannot register again." << std::endl;
        return;
    }
    std::cout << "Enter username: ";
    std::string username;
    std::getline(std::cin, username);

    CryptoPP::RSA::PrivateKey privateKey;
    CryptoPP::RSA::PublicKey publicKey;
    CryptoWrapper::generateRsaKeys(privateKey, publicKey);

    RegistrationRequest req{};
    strncpy_s(req.name, sizeof(req.name), username.c_str(), sizeof(req.name) - 1);
    auto pubKeyBytes = CryptoWrapper::publicKeyToBytes(publicKey);
    std::copy(pubKeyBytes.begin(), pubKeyBytes.end(), req.publicKey);

    std::vector<uint8_t> payload(sizeof(req));
    memcpy(payload.data(), &req, sizeof(req));

    auto response = _communicator->sendAndReceive(RequestCode::REGISTER, payload, {});
    if (response && response->size() == sizeof(RegistrationSuccessResponse)) {
        auto* respData = reinterpret_cast<RegistrationSuccessResponse*>(response->data());
        UserInfo info;
        info.username = username;
        info.uuid.assign(respData->clientID, respData->clientID + CLIENT_ID_SIZE);
        info.privateKey = CryptoWrapper::privateKeyToBase64(privateKey);

        if (FileHandler::writeMyInfo(info)) {
            std::cout << "Registration successful." << std::endl;
            _userInfo = info;
            _privateKey = privateKey;
        }
        else {
            std::cerr << "Failed to write my.info file." << std::endl;
        }
    }
}

void Client::handleRequestClientsList() {
    if (!_userInfo) { std::cerr << "Please register first." << std::endl; return; }

    auto response = _communicator->sendAndReceive(RequestCode::CLIENTS_LIST, {}, _userInfo->uuid);
    if (response) {
        _clientList.clear();
        size_t entrySize = sizeof(ClientInfoResponse);
        for (size_t i = 0; i < response->size(); i += entrySize) {
            auto* info = reinterpret_cast<ClientInfoResponse*>(response->data() + i);
            ClientInfo client;
            client.id.assign(info->clientID, info->clientID + CLIENT_ID_SIZE);
            client.name = std::string(info->name);
            _clientList.push_back(client);
        }
        std::cout << "Clients list:" << std::endl;
        for (const auto& client : _clientList) {
            std::cout << "- " << client.name << std::endl;
        }
    }
}

//... Implementations for other handlers (130-152) would follow a similar pattern...
// For brevity, only the complex message handling logic is detailed below.

void Client::handleRequestWaitingMessages() {
    if (!_userInfo) { std::cerr << "Please register first." << std::endl; return; }

    auto response = _communicator->sendAndReceive(RequestCode::PULL_MESSAGES, {}, _userInfo->uuid);
    if (response && !response->empty()) {
        size_t offset = 0;
        while (offset < response->size()) {
            auto* header = reinterpret_cast<MessageHeader*>(response->data() + offset);
            offset += sizeof(MessageHeader);
            std::vector<uint8_t> content(response->data() + offset, response->data() + offset + header->messageSize);
            offset += header->messageSize;

            auto* sender = findClientByID(std::vector<uint8_t>(header->clientID, header->clientID + CLIENT_ID_SIZE));
            if (!sender) { // If sender not in list, fetch their info
                handleRequestClientsList();
                sender = findClientByID(std::vector<uint8_t>(header->clientID, header->clientID + CLIENT_ID_SIZE));
            }
            std::string senderName = sender ? sender->name : "Unknown";

            std::cout << "From: " << senderName << std::endl;
            std::cout << "Content:\n";

            switch (header->type) {
            case MessageType::SYM_KEY_REQUEST:
                std::cout << "Request for symmetric key" << std::endl;
                break;
            case MessageType::SYM_KEY_SEND: {
                try {
                    auto symKey = CryptoWrapper::rsaDecrypt(_privateKey, content);
                    if (sender) sender->symKey = symKey;
                    std::cout << "Symmetric key received." << std::endl;
                }
                catch (...) {
                    std::cerr << "Failed to decrypt symmetric key." << std::endl;
                }
                break;
            }
            case MessageType::TEXT_MESSAGE: {
                if (sender && !sender->symKey.empty()) {
                    try {
                        auto decrypted = CryptoWrapper::aesDecrypt(sender->symKey, content);
                        std::cout << std::string(decrypted.begin(), decrypted.end()) << std::endl;
                    }
                    catch (...) {
                        std::cerr << "Can't decrypt message" << std::endl;
                    }
                }
                else {
                    std::cerr << "Can't decrypt message" << std::endl;
                }
                break;
            }
            default:
                std::cout << "Unknown message type." << std::endl;
            }
            std::cout << "-----<EOM>-----\n" << std::endl;
        }
    }
    else {
        std::cout << "No new messages." << std::endl;
    }
}

void Client::handleRequestPublicKey() {
    if (!_userInfo) { std::cerr << "Please register first." << std::endl; return; }

    std::cout << "Enter username to get public key for: ";
    std::string username;
    std::getline(std::cin, username);

    ClientInfo* client = findClientByName(username);
    if (!client) {
        std::cout << "Client not in local list, fetching from server..." << std::endl;
        handleRequestClientsList(); // Refresh list
        client = findClientByName(username);
    }

    if (!client) {
        std::cerr << "Could not find client '" << username << "'." << std::endl;
        return;
    }

    PublicKeyRequest req{};
    std::copy(client->id.begin(), client->id.end(), req.clientID);
    
    std::vector<uint8_t> payload(sizeof(req));
    memcpy(payload.data(), &req, sizeof(req));

    auto response = _communicator->sendAndReceive(RequestCode::PUBLIC_KEY, payload, _userInfo->uuid);

    if (response && response->size() == sizeof(PublicKeyResponse)) {
        auto* respData = reinterpret_cast<PublicKeyResponse*>(response->data());
        client->publicKey.assign(respData->publicKey, respData->publicKey + PUBLIC_KEY_SIZE);
        
        std::cout << "Public key for " << username << ":" << std::endl;
        for (const auto& byte : client->publicKey) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
        }
        std::cout << std::dec << std::endl;
    }
    else {
        std::cerr << "Failed to retrieve public key." << std::endl;
    }
}

void Client::handleSendTextMessage() {
    if (!_userInfo) { std::cerr << "Please register first." << std::endl; return; }

    std::cout << "Enter username to send a message to: ";
    std::string username;
    std::getline(std::cin, username);

    ClientInfo* client = findClientByName(username);
    if (!client) {
        std::cout << "Client not in local list, fetching from server..." << std::endl;
        handleRequestClientsList(); // Refresh list
        client = findClientByName(username);
    }

    if (!client) {
        std::cerr << "Could not find client '" << username << "'." << std::endl;
        return;
    }

    // Check if we have a symmetric key for this user
    if (client->symKey.empty()) {
        std::cerr << "No symmetric key established with " << username << "." << std::endl;
        std::cerr << "Please send a symmetric key (152) or have them send one to you." << std::endl;
        return;
    }

    std::cout << "Enter message to send:" << std::endl;
    std::string message;
    std::getline(std::cin, message);

    try {
        // Encrypt the message
        std::vector<uint8_t> plaintext(message.begin(), message.end());
        auto ciphertext = CryptoWrapper::aesEncrypt(client->symKey, plaintext);

        // Construct the message payload
        SendMessageHeader msgHeader{};
        std::copy(client->id.begin(), client->id.end(), msgHeader.clientID);
        msgHeader.type = MessageType::TEXT_MESSAGE;
        msgHeader.contentSize = static_cast<uint32_t>(ciphertext.size());

        std::vector<uint8_t> payload;
        payload.resize(sizeof(msgHeader) + ciphertext.size());
        memcpy(payload.data(), &msgHeader, sizeof(msgHeader));
        memcpy(payload.data() + sizeof(msgHeader), ciphertext.data(), ciphertext.size());

        // Send the message
        auto response = _communicator->sendAndReceive(RequestCode::SEND_MESSAGE, payload, _userInfo->uuid);

        if (response && response->size() == sizeof(MessageSentResponse)) {
            std::cout << "Message sent to " << username << "." << std::endl;
        } else {
            std::cerr << "Failed to send message." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "An error occurred during message encryption: " << e.what() << std::endl;
    }
}

void Client::handleSendSymKeyRequest() {
    if (!_userInfo) { std::cerr << "Please register first." << std::endl; return; }

    std::cout << "Enter username to request a symmetric key from: ";
    std::string username;
    std::getline(std::cin, username);

    ClientInfo* client = findClientByName(username);
    if (!client) {
        std::cout << "Client not in local list, fetching from server..." << std::endl;
        handleRequestClientsList(); // Refresh list
        client = findClientByName(username);
    }

    if (!client) {
        std::cerr << "Could not find client '" << username << "'." << std::endl;
        return;
    }

    // Construct payload (header only, no content)
    SendMessageHeader msgHeader{};
    std::copy(client->id.begin(), client->id.end(), msgHeader.clientID);
    msgHeader.type = MessageType::SYM_KEY_REQUEST;
    msgHeader.contentSize = 0;

    std::vector<uint8_t> payload(sizeof(msgHeader));
    memcpy(payload.data(), &msgHeader, sizeof(msgHeader));

    auto response = _communicator->sendAndReceive(RequestCode::SEND_MESSAGE, payload, _userInfo->uuid);

    if (response && response->size() == sizeof(MessageSentResponse)) {
        std::cout << "Symmetric key request sent to " << username << "." << std::endl;
    } else {
        std::cerr << "Failed to send symmetric key request." << std::endl;
    }
}

void Client::handleSendSymKey() {
    if (!_userInfo) { std::cerr << "Please register first." << std::endl; return; }

    std::cout << "Enter username to send your symmetric key to: ";
    std::string username;
    std::getline(std::cin, username);

    ClientInfo* client = findClientByName(username);
    if (!client) {
        std::cout << "Client not in local list, fetching from server..." << std::endl;
        handleRequestClientsList(); // Refresh list
        client = findClientByName(username);
    }

    if (!client) {
        std::cerr << "Could not find client '" << username << "'." << std::endl;
        return;
    }

    // Step 1: Ensure we have the recipient's public key.
    if (client->publicKey.empty()) {
        std::cout << "Public key for " << username << " not found. Requesting it now..." << std::endl;
        
        PublicKeyRequest pkReq{};
        std::copy(client->id.begin(), client->id.end(), pkReq.clientID);
        std::vector<uint8_t> pkPayload(sizeof(pkReq));
        memcpy(pkPayload.data(), &pkReq, sizeof(pkReq));

        auto pkResponse = _communicator->sendAndReceive(RequestCode::PUBLIC_KEY, pkPayload, _userInfo->uuid);

        if (pkResponse && pkResponse->size() == sizeof(PublicKeyResponse)) {
            auto* respData = reinterpret_cast<PublicKeyResponse*>(pkResponse->data());
            client->publicKey.assign(respData->publicKey, respData->publicKey + PUBLIC_KEY_SIZE);
            std::cout << "Successfully received public key." << std::endl;
        } else {
            std::cerr << "Failed to retrieve public key for " << username << ". Cannot send symmetric key." << std::endl;
            return;
        }
    }

    // Step 2: Generate, encrypt, and send the symmetric key.
    try {
        // Generate a new symmetric key
        auto symKey = CryptoWrapper::generateAesKey();

        // Get recipient's public key object
        CryptoPP::RSA::PublicKey publicKey;
        CryptoWrapper::bytesToPublicKey(client->publicKey, publicKey);

        // Encrypt the symmetric key with the public key
        auto encryptedSymKey = CryptoWrapper::rsaEncrypt(publicKey, symKey);

        // Construct the message payload
        SendMessageHeader msgHeader{};
        std::copy(client->id.begin(), client->id.end(), msgHeader.clientID);
        msgHeader.type = MessageType::SYM_KEY_SEND;
        msgHeader.contentSize = static_cast<uint32_t>(encryptedSymKey.size());

        std::vector<uint8_t> payload;
        payload.resize(sizeof(msgHeader) + encryptedSymKey.size());
        memcpy(payload.data(), &msgHeader, sizeof(msgHeader));
        memcpy(payload.data() + sizeof(msgHeader), encryptedSymKey.data(), encryptedSymKey.size());

        // Send the message
        auto response = _communicator->sendAndReceive(RequestCode::SEND_MESSAGE, payload, _userInfo->uuid);

        if (response && response->size() == sizeof(MessageSentResponse)) {
            // Store the key for our own use
            client->symKey = symKey;
            std::cout << "Symmetric key sent to " << username << "." << std::endl;
        } else {
            std::cerr << "Failed to send symmetric key." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "An error occurred during key generation or encryption: " << e.what() << std::endl;
    }
}

ClientInfo* Client::findClientByName(const std::string& name) {
    for (auto& client : _clientList) {
        if (client.name == name) {
            return &client;
        }
    }
    return nullptr;
}

ClientInfo* Client::findClientByID(const std::vector<uint8_t>& id) {
    for (auto& client : _clientList) {
        if (client.id == id) {
            return &client;
        }
    }
    return nullptr;
}