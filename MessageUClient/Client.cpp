// main.cpp
// author: Ariel Cohen ID: 329599187

#include "Client.h"
#include <iostream>
#include <iomanip>

// When DEBUG is defined, DEBUG_LOG(x) will print x to the console.
// Otherwise, it will do nothing. This is useful for adding debug prints that are
// automatically removed from release builds.
#ifdef DEBUG
#define DEBUG_LOG(x) do { std::cout << x << std::endl; } while (0)
#else
#define DEBUG_LOG(x)
#endif

/**
 * @brief Initializes the client.
 * Reads server info from server.info and user data from my.info if it exists.
 */
Client::Client() {
    // Load server IP and port from the configuration file.
    auto serverInfo = FileHandler::readServerInfo();
    if (!serverInfo) {
        throw std::runtime_error("server.info file not found or is invalid.");
    }
    _communicator = std::make_unique<Communicator>(serverInfo->ip, serverInfo->port);

    // If a my.info file exists, load the user's identity.
    _userInfo = FileHandler::readMyInfo();
    if (_userInfo) {
        DEBUG_LOG("[DEBUG] Loaded user info for " << _userInfo->username);
        // The private key is stored in Base64, so it needs to be decoded.
        CryptoWrapper::base64ToPrivateKey(_userInfo->privateKey, _privateKey);
    }
}

/**
 * @brief The main loop of the client application.
 * Displays the menu, gets user input, and calls the appropriate handler.
 */
void Client::run() {
    int choice = -1;
    while (choice != 0) {
        showMenu();
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear input buffer after reading number

        switch (choice) {
        case 110: handleRegister(); break;
        case 120: handleRequestClientsList(); break;
        case 130: handleRequestPublicKey(); break;
        case 140: handleRequestWaitingMessages(); break;
        case 150: handleSendTextMessage(); break;
        case 151: handleSendSymKeyRequest(); break;
        case 152: handleSendSymKey(); break;
        case 153: handleSendFile(); break;
        case 0: std::cout << "Exiting..." << std::endl; break;
        default: std::cout << "Invalid option." << std::endl; break;
        }
    }
}

/**
 * @brief Displays the main menu of options to the user.
 */
void Client::showMenu() {
    std::cout << "\nMessageU client at your service.\n\n";
    std::cout << "110) Register\n";
    std::cout << "120) Request for clients list\n";
    std::cout << "130) Request for public key\n";
    std::cout << "140) Request for waiting messages\n";
    std::cout << "150) Send a text message\n";
    std::cout << "151) Send a request for symmetric key\n";
    std::cout << "152) Send your symmetric key\n";
    std::cout << "153) Send a file\n";
    std::cout << "0) Exit client\n";
    std::cout << "? ";
}

/**
 * @brief Handles the user registration process.
 * Generates a new RSA key pair, sends the username and public key to the server,
 * and saves the returned UUID and private key to my.info.
 */
void Client::handleRegister() {
    // Prevent re-registration if user info already exists.
    if (FileHandler::myInfoExists()) {
        std::cerr << "Error: my.info file already exists. Cannot register again." << std::endl;
        return;
    }
    std::cout << "Enter username: ";
    std::string username;
    std::getline(std::cin, username);

    // Generate a new RSA key pair for the user.
    DEBUG_LOG("[DEBUG] Generating RSA key pair for registration...");
    CryptoPP::RSA::PrivateKey privateKey;
    CryptoPP::RSA::PublicKey publicKey;
    CryptoWrapper::generateRsaKeys(privateKey, publicKey);

    // Pack the registration request according to the protocol.
    RegistrationRequest req{};
    strncpy_s(req.name, sizeof(req.name), username.c_str(), sizeof(req.name) - 1);
    auto pubKeyBytes = CryptoWrapper::publicKeyToBytes(publicKey);
    std::copy(pubKeyBytes.begin(), pubKeyBytes.end(), req.publicKey);

    std::vector<uint8_t> payload(sizeof(req));
    memcpy(payload.data(), &req, sizeof(req));

    DEBUG_LOG("[DEBUG] Sending registration request for user " << username);
    auto response = _communicator->sendAndReceive(RequestCode::REGISTER, payload, {});
    
    // On successful registration, save the new user info.
    if (response && response->size() == sizeof(RegistrationSuccessResponse)) {
        auto* respData = reinterpret_cast<RegistrationSuccessResponse*>(response->data());
        UserInfo info;
        info.username = username;
        info.uuid.assign(respData->clientID, respData->clientID + CLIENT_ID_SIZE);
        info.privateKey = CryptoWrapper::privateKeyToBase64(privateKey);

        if (FileHandler::writeMyInfo(info)) {
            std::cout << "Registration successful." << std::endl;
            // Update the current session with the new user info.
            _userInfo = info;
            _privateKey = privateKey;
        }
        else {
            std::cerr << "Failed to write my.info file." << std::endl;
        }
    }
}

/**
 * @brief Requests the list of all registered clients from the server and displays it.
 */
void Client::handleRequestClientsList() {
    if (!_userInfo) { std::cerr << "Please register first." << std::endl; return; }
    DEBUG_LOG("[DEBUG] Requesting clients list from server...");
    auto response = _communicator->sendAndReceive(RequestCode::CLIENTS_LIST, {}, _userInfo->uuid);
    if (response) {
        _clientList.clear(); // Clear the old list.
        // The response payload is a series of ClientInfoResponse structs.
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

/**
 * @brief Fetches and processes all waiting messages from the server.
 * This function handles different message types like key requests, keys, text, and files.
 */
void Client::handleRequestWaitingMessages() {
    if (!_userInfo) { std::cerr << "Please register first." << std::endl; return; }
    DEBUG_LOG("[DEBUG] Requesting waiting messages...");
    auto response = _communicator->sendAndReceive(RequestCode::PULL_MESSAGES, {}, _userInfo->uuid);
    if (response && !response->empty()) {
        // The response is a stream of messages, so we loop through it.
        size_t offset = 0;
        while (offset < response->size()) {
            // Parse the message header.
            auto* header = reinterpret_cast<MessageHeader*>(response->data() + offset);
            offset += sizeof(MessageHeader);
            // Extract the message content.
            std::vector<uint8_t> content(response->data() + offset, response->data() + offset + header->messageSize);
            offset += header->messageSize;

            // Find the sender in our local list. If not found, refresh the list.
            auto* sender = findClientByID(std::vector<uint8_t>(header->clientID, header->clientID + CLIENT_ID_SIZE));
            if (!sender) { 
                DEBUG_LOG("[DEBUG] Sender " << FileHandler::bytesToHex({header->clientID, header->clientID + 16}) << " not in local list. Refreshing.");
                handleRequestClientsList();
                sender = findClientByID(std::vector<uint8_t>(header->clientID, header->clientID + CLIENT_ID_SIZE));
            }
            std::string senderName = sender ? sender->name : "Unknown";

            std::cout << "From: " << senderName << std::endl;
            std::cout << "Content:\n";

            // Handle the message based on its type.
            switch (header->type) {
            case MessageType::SYM_KEY_REQUEST:
                DEBUG_LOG("[DEBUG] Received SYM_KEY_REQUEST from " << senderName);
                std::cout << "Request for symmetric key" << std::endl;
                break;
            case MessageType::SYM_KEY_SEND: {
                DEBUG_LOG("[DEBUG] Received SYM_KEY_SEND from " << senderName << ". Content size: " << content.size());
                try {
                    // Decrypt the symmetric key with our private RSA key.
                    auto symKey = CryptoWrapper::rsaDecrypt(_privateKey, content);
                    if (sender) {
                        sender->symKey = symKey;
                        DEBUG_LOG("[DEBUG] Decrypted and stored symmetric key: " << FileHandler::bytesToHex(symKey));
                    }
                    std::cout << "Symmetric key received." << std::endl;
                }
                catch (...) {
                    std::cerr << "Failed to decrypt symmetric key." << std::endl;
                }
                break;
            }
            case MessageType::TEXT_MESSAGE: {
                DEBUG_LOG("[DEBUG] Received TEXT_MESSAGE from " << senderName << ". Content size: " << content.size());
                if (sender && !sender->symKey.empty()) {
                    try {
                        // Decrypt the text message with the shared symmetric key.
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
            case MessageType::FILE_SEND: {
                if (sender && !sender->symKey.empty()) {
                    DEBUG_LOG("\n[DEBUG] RECEIVING CLIENT (File):");
                    DEBUG_LOG("  Symmetric key:   " << FileHandler::bytesToHex(sender->symKey));
                    DEBUG_LOG("  Ciphertext size: " << content.size() << " bytes");
                    if (content.size() >= 16) {
                        DEBUG_LOG("  Ciphertext prefix: " << FileHandler::bytesToHex({content.begin(), content.begin() + 16}));
                    }
                    DEBUG_LOG("-----");
                    try {
                        // Decrypt the file content with the shared symmetric key.
                        auto decrypted = CryptoWrapper::aesDecrypt(sender->symKey, content);
                        // Save the decrypted content to a temporary file.
                        auto saved_path = FileHandler::writeToTempFile(decrypted);
                        std::cout << saved_path << std::endl;
                    }
                    catch (const std::exception& e) {
                        std::cerr << "Can't decrypt or save file: " << e.what() << std::endl;
                    }
                }
                else {
                    std::cerr << "Can't decrypt file, no symmetric key." << std::endl;
                }
                break;
            }
            default:
                DEBUG_LOG("[DEBUG] Received unknown message type: " << (int)header->type);
                std::cout << "Unknown message type." << std::endl;
            }
            std::cout << "-----<EOM>-----\n" << std::endl;
        }
    }
    else {
        std::cout << "No new messages." << std::endl;
    }
}

/**
 * @brief Requests the public key for a specific user from the server.
 */
void Client::handleRequestPublicKey() {
    if (!_userInfo) { std::cerr << "Please register first." << std::endl; return; }

    std::cout << "Enter username to get public key for: ";
    std::string username;
    std::getline(std::cin, username);

    // Check if we know the user first, if not, refresh the list.
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

    // Pack the request payload.
    PublicKeyRequest req{};
    std::copy(client->id.begin(), client->id.end(), req.clientID);
    
    std::vector<uint8_t> payload(sizeof(req));
    memcpy(payload.data(), &req, sizeof(req));

    DEBUG_LOG("[DEBUG] Requesting public key for " << username);
    auto response = _communicator->sendAndReceive(RequestCode::PUBLIC_KEY, payload, _userInfo->uuid);

    if (response && response->size() == sizeof(PublicKeyResponse)) {
        auto* respData = reinterpret_cast<PublicKeyResponse*>(response->data());
        // Store the received public key in our local list for this user.
        client->publicKey.assign(respData->publicKey, respData->publicKey + PUBLIC_KEY_SIZE);
        
        DEBUG_LOG("[DEBUG] Received public key: " << FileHandler::bytesToHex(client->publicKey));
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

/**
 * @brief Sends an encrypted text message to another user.
 * Requires a symmetric key to be established first.
 */
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

    // A symmetric key is required for sending text messages.
    if (client->symKey.empty()) {
        std::cerr << "No symmetric key established with " << username << "." << std::endl;
        std::cerr << "Please send a symmetric key (152) or have them send one to you." << std::endl;
        return;
    }

    std::cout << "Enter message to send:" << std::endl;
    std::string message;
    std::getline(std::cin, message);

    try {
        DEBUG_LOG("\n[DEBUG] SENDING CLIENT (Text):");
        DEBUG_LOG("  To user: " << username);
        DEBUG_LOG("  Symmetric key: " << FileHandler::bytesToHex(client->symKey));
        
        // Encrypt the message with the shared symmetric key.
        std::vector<uint8_t> plaintext(message.begin(), message.end());
        auto ciphertext = CryptoWrapper::aesEncrypt(client->symKey, plaintext);

        DEBUG_LOG("  Ciphertext size: " << ciphertext.size() << " bytes");
        DEBUG_LOG("-----");

        // Construct the payload with a header and the encrypted content.
        SendMessageHeader msgHeader{};
        std::copy(client->id.begin(), client->id.end(), msgHeader.clientID);
        msgHeader.type = MessageType::TEXT_MESSAGE;
        msgHeader.contentSize = static_cast<uint32_t>(ciphertext.size());

        std::vector<uint8_t> payload;
        payload.resize(sizeof(msgHeader) + ciphertext.size());
        memcpy(payload.data(), &msgHeader, sizeof(msgHeader));
        memcpy(payload.data() + sizeof(msgHeader), ciphertext.data(), ciphertext.size());

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

/**
 * @brief Sends a request to another user, asking them to send their symmetric key.
 */
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

    DEBUG_LOG("\n[DEBUG] SENDING CLIENT (SymKey Request):");
    DEBUG_LOG("  To user: " << username);
    DEBUG_LOG("-----");

    // The payload for a key request is just the header, with no content.
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

/**
 * @brief Sends a new symmetric key to another user.
 * The key is generated locally, encrypted with the recipient's public RSA key,
 * and then sent.
 */
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

    // We must have the recipient's public key to encrypt the symmetric key.
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

    try {
        DEBUG_LOG("\n[DEBUG] SENDING CLIENT (SymKey):");
        DEBUG_LOG("  To user: " << username);

        // Generate a new AES key.
        auto symKey = CryptoWrapper::generateAesKey();
        DEBUG_LOG("  New symkey: " << FileHandler::bytesToHex(symKey));

        // Encrypt the new AES key with the recipient's public RSA key.
        CryptoPP::RSA::PublicKey publicKey;
        CryptoWrapper::bytesToPublicKey(client->publicKey, publicKey);
        auto encryptedSymKey = CryptoWrapper::rsaEncrypt(publicKey, symKey);
        DEBUG_LOG("  Encrypted key size: " << encryptedSymKey.size() << " bytes");
        DEBUG_LOG("-----");

        // Construct the payload.
        SendMessageHeader msgHeader{};
        std::copy(client->id.begin(), client->id.end(), msgHeader.clientID);
        msgHeader.type = MessageType::SYM_KEY_SEND;
        msgHeader.contentSize = static_cast<uint32_t>(encryptedSymKey.size());

        std::vector<uint8_t> payload;
        payload.resize(sizeof(msgHeader) + encryptedSymKey.size());
        memcpy(payload.data(), &msgHeader, sizeof(msgHeader));
        memcpy(payload.data() + sizeof(msgHeader), encryptedSymKey.data(), encryptedSymKey.size());

        auto response = _communicator->sendAndReceive(RequestCode::SEND_MESSAGE, payload, _userInfo->uuid);

        if (response && response->size() == sizeof(MessageSentResponse)) {
            // Store the new key for our own use with this client.
            client->symKey = symKey;
            std::cout << "Symmetric key sent to " << username << "." << std::endl;
        } else {
            std::cerr << "Failed to send symmetric key." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "An error occurred during key generation or encryption: " << e.what() << std::endl;
    }
}

/**
 * @brief Finds a client in the local list by their username.
 * @param name The name of the client to find.
 * @return A pointer to the ClientInfo struct, or nullptr if not found.
 */
ClientInfo* Client::findClientByName(const std::string& name) {
    for (auto& client : _clientList) {
        if (client.name == name) {
            return &client;
        }
    }
    return nullptr;
}

/**
 * @brief Finds a client in the local list by their UUID.
 * @param id The UUID of the client to find.
 * @return A pointer to the ClientInfo struct, or nullptr if not found.
 */
ClientInfo* Client::findClientByID(const std::vector<uint8_t>& id) {
    for (auto& client : _clientList) {
        if (client.id == id) {
            return &client;
        }
    }
    return nullptr;
}

/**
 * @brief Sends an encrypted file to another user.
 * Requires a symmetric key to be established first.
 */
void Client::handleSendFile() {
    if (!_userInfo) { std::cerr << "Please register first." << std::endl; return; }

    std::cout << "Enter username to send a file to: ";
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

    // A symmetric key is required for sending files.
    if (client->symKey.empty()) {
        std::cerr << "No symmetric key for " << username << ". Please send a key first." << std::endl;
        return;
    }

    std::cout << "Enter full path to the file: ";
    std::string filepath;
    std::getline(std::cin, filepath);

    // Read the entire file into a byte vector.
    auto file_content = FileHandler::readFileContent(filepath);
    if (!file_content) {
        std::cerr << "file not found or could not be read." << std::endl;
        return;
    }

    try {
        DEBUG_LOG("\n[DEBUG] SENDING CLIENT (File):");
        DEBUG_LOG("  To user: " << username);
        DEBUG_LOG("  Symmetric key:   " << FileHandler::bytesToHex(client->symKey));
        DEBUG_LOG("  Plaintext size:  " << file_content->size() << " bytes");

        // Encrypt the file content with the shared symmetric key.
        auto ciphertext = CryptoWrapper::aesEncrypt(client->symKey, *file_content);

        DEBUG_LOG("  Ciphertext size: " << ciphertext.size() << " bytes");
        if (ciphertext.size() >= 16) {
            DEBUG_LOG("  Ciphertext prefix: " << FileHandler::bytesToHex({ciphertext.begin(), ciphertext.begin() + 16}));
        }
        DEBUG_LOG("-----");

        // Construct the payload.
        SendMessageHeader msgHeader{};
        std::copy(client->id.begin(), client->id.end(), msgHeader.clientID);
        msgHeader.type = MessageType::FILE_SEND;
        msgHeader.contentSize = static_cast<uint32_t>(ciphertext.size());

        std::vector<uint8_t> payload;
        payload.resize(sizeof(msgHeader) + ciphertext.size());
        memcpy(payload.data(), &msgHeader, sizeof(msgHeader));
        memcpy(payload.data() + sizeof(msgHeader), ciphertext.data(), ciphertext.size());

        auto response = _communicator->sendAndReceive(RequestCode::SEND_MESSAGE, payload, _userInfo->uuid);

        if (response && response->size() == sizeof(MessageSentResponse)) {
            std::cout << "File sent to " << username << "." << std::endl;
        } else {
            std::cerr << "Failed to send file." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "An error occurred during file encryption: " << e.what() << std::endl;
    }
}
