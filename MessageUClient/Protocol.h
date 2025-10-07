/**
 * @file Protocol.h
 * @brief This file defines the structures and constants for the client-server communication protocol.
 * It is crucial that these definitions match exactly on both the client and server sides.
 */

#pragma once
#include <cstdint>
#include <string>
#include <vector>

// --- Protocol Constants ---

constexpr uint8_t CLIENT_VERSION = 2;        ///< Current client version. V2 supports file transfer.
constexpr size_t CLIENT_ID_SIZE = 16;        ///< 128-bit UUID for each client.
constexpr size_t USERNAME_SIZE = 255;        ///< Max length for a client's username.
constexpr size_t PUBLIC_KEY_SIZE = 160;      ///< 1024-bit RSA public key in X.509 format.
constexpr size_t SYM_KEY_SIZE = 16;          ///< 128-bit AES symmetric key.

// --- Protocol Codes ---

/**
 * @brief Defines the codes for requests sent from the client to the server.
 */
enum class RequestCode : uint16_t {
    REGISTER = 1100,          ///< Request to register a new user.
    CLIENTS_LIST = 1101,      ///< Request for the list of all registered users.
    PUBLIC_KEY = 1102,        ///< Request for a specific user's public key.
    SEND_MESSAGE = 1103,      ///< Send a message to another client (via the server).
    PULL_MESSAGES = 1104,     ///< Request to pull all waiting messages.
};

/**
 * @brief Defines the codes for responses sent from the server to the client.
 */
enum class ResponseCode : uint16_t {
    REGISTRATION_SUCCESS = 2100, ///< Response for a successful registration.
    CLIENTS_LIST = 2101,         ///< Response containing the list of clients.
    PUBLIC_KEY = 2102,           ///< Response containing a user's public key.
    MESSAGE_SENT = 2103,         ///< Confirmation that a message was received by the server.
    PULL_MESSAGES = 2104,        ///< Response containing waiting messages.
    GENERAL_ERROR = 9000,        ///< A generic error response.
};

/**
 * @brief Defines the types for messages sent between clients.
 */
enum class MessageType : uint8_t {
    SYM_KEY_REQUEST = 1,    ///< A request for a symmetric key.
    SYM_KEY_SEND = 2,       ///< A message containing a symmetric key.
    TEXT_MESSAGE = 3,       ///< A standard text message.
    FILE_SEND = 4,          ///< A message containing file content.
};

// Use pragma pack to ensure structs are packed without padding.
// This is critical for ensuring binary compatibility between the C++ client and Python server.
#pragma pack(push, 1)

// --- Header Structures ---

/**
 * @brief Header for every request sent from the client.
 */
struct RequestHeader {
    uint8_t clientID[CLIENT_ID_SIZE];
    uint8_t version;
    RequestCode code;
    uint32_t payloadSize;
};

/**
 * @brief Header for every response sent from the server.
 */
struct ResponseHeader {
    uint8_t version;
    ResponseCode code;
    uint32_t payloadSize;
};

// --- Request Payload Structures ---

/**
 * @brief Payload for a registration request (1100).
 */
struct RegistrationRequest {
    char name[USERNAME_SIZE];
    uint8_t publicKey[PUBLIC_KEY_SIZE];
};

/**
 * @brief Payload for a public key request (1102).
 */
struct PublicKeyRequest {
    uint8_t clientID[CLIENT_ID_SIZE];
};

/**
 * @brief Header part of the payload for a send-message request (1103).
 * The actual encrypted content follows this header in the payload.
 */
struct SendMessageHeader {
    uint8_t clientID[CLIENT_ID_SIZE]; ///< The recipient's ID.
    MessageType type;
    uint32_t contentSize;
};

// --- Response Payload Structures ---

/**
 * @brief Payload for a successful registration response (2100).
 */
struct RegistrationSuccessResponse {
    uint8_t clientID[CLIENT_ID_SIZE];
};

/**
 * @brief Structure for a single entry in the clients list response (2101).
 * The full payload is a series of these structs.
 */
struct ClientInfoResponse {
    uint8_t clientID[CLIENT_ID_SIZE];
    char name[USERNAME_SIZE];
};

/**
 * @brief Payload for a public key response (2102).
 */
struct PublicKeyResponse {
    uint8_t clientID[CLIENT_ID_SIZE];
    uint8_t publicKey[PUBLIC_KEY_SIZE];
};

/**
 * @brief Payload for a message sent confirmation (2103).
 */
struct MessageSentResponse {
    uint8_t clientID[CLIENT_ID_SIZE];
    uint32_t messageID;
};

/**
 * @brief Header for a single message within a pulled messages response (2104).
 * The actual message content follows this header.
 */
struct MessageHeader {
    uint8_t clientID[CLIENT_ID_SIZE]; ///< The sender's ID.
    uint32_t messageID;
    MessageType type;
    uint32_t messageSize;
};

#pragma pack(pop)

// --- In-Memory Helper Structures ---

/**
 * @brief Represents a client's information as stored in memory by the application.
 * This is different from the on-the-wire structs.
 */
struct ClientInfo {
    std::vector<uint8_t> id;        ///< Client's UUID.
    std::string name;               ///< Client's username.
    std::vector<uint8_t> publicKey; ///< Client's public RSA key.
    std::vector<uint8_t> symKey;    ///< Symmetric AES key shared with this client.
};
