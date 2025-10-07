// main.cpp
// author: Ariel Cohen ID: 329599187

#pragma once
#include "Communicator.h"
#include "CryptoWrapper.h"
#include "FileHandler.h"
#include <map>

/*
* The Client class is the main class for the client side of the application.
* It handles the user interface and the main loop of the program.
*/
class Client {
public:
    // Constructor
    Client();
    // Main loop of the program
    void run();

private:
    // Shows the main menu of the application
    void showMenu();
    // Handles the registration of a new user
    void handleRegister();
    // Handles the request for the list of clients
    void handleRequestClientsList();
    // Handles the request for the public key of a client
    void handleRequestPublicKey();
    // Handles the request for waiting messages
    void handleRequestWaitingMessages();
    // Handles sending a text message
    void handleSendTextMessage();
    // Handles the request to send a symmetric key
    void handleSendSymKeyRequest();
    // Handles sending a symmetric key
    void handleSendSymKey();
    // Handles sending a file
    void handleSendFile();

    // Finds a client by name in the client list
    ClientInfo* findClientByName(const std::string& name);
    // Finds a client by ID in the client list
    ClientInfo* findClientByID(const std::vector<uint8_t>& id);

    // The communicator object for handling communication with the server
    std::unique_ptr<Communicator> _communicator;
    // The user information of the current user
    std::optional<UserInfo> _userInfo;
    // The private key of the current user
    CryptoPP::RSA::PrivateKey _privateKey;
    // The list of clients
    std::vector<ClientInfo> _clientList;
}; 
