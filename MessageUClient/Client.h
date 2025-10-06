// main.cpp
// author: Ariel Cohen ID: 329599187

#pragma once
#include "Communicator.h"
#include "CryptoWrapper.h"
#include "FileHandler.h"
#include <map>

class Client {
public:
    Client();
    void run();

private:
    void showMenu();
    void handleRegister();
    void handleRequestClientsList();
    void handleRequestPublicKey();
    void handleRequestWaitingMessages();
    void handleSendTextMessage();
    void handleSendSymKeyRequest();
    void handleSendSymKey();
    void handleSendFile();

    ClientInfo* findClientByName(const std::string& name);
    ClientInfo* findClientByID(const std::vector<uint8_t>& id);

    std::unique_ptr<Communicator> _communicator;
    std::optional<UserInfo> _userInfo;
    CryptoPP::RSA::PrivateKey _privateKey;
    std::vector<ClientInfo> _clientList;
}; 
