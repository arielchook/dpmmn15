#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Protocol constants
constexpr uint8_t CLIENT_VERSION = 1;
constexpr size_t CLIENT_ID_SIZE = 16;
constexpr size_t USERNAME_SIZE = 255;
constexpr size_t PUBLIC_KEY_SIZE = 160;
constexpr size_t SYM_KEY_SIZE = 16; // 128-bit AES

// Request Codes
enum class RequestCode : uint16_t {
    REGISTER = 1100,
    CLIENTS_LIST = 1101,
    PUBLIC_KEY = 1102,
    SEND_MESSAGE = 1103,
    PULL_MESSAGES = 1104,
};

// Response Codes
enum class ResponseCode : uint16_t {
    REGISTRATION_SUCCESS = 2100,
    CLIENTS_LIST = 2101,
    PUBLIC_KEY = 2102,
    MESSAGE_SENT = 2103,
    PULL_MESSAGES = 2104,
    GENERAL_ERROR = 9000,
};

// Message Types
enum class MessageType : uint8_t {
    SYM_KEY_REQUEST = 1,
    SYM_KEY_SEND = 2,
    TEXT_MESSAGE = 3,
    FILE_SEND = 4,
};

// Use pragma pack to ensure structs match the binary protocol layout without padding
#pragma pack(push, 1)

// General Header Structures
struct RequestHeader {
    uint8_t clientID[CLIENT_ID_SIZE];
    uint8_t version;
    RequestCode code;
    uint32_t payloadSize;
};

struct ResponseHeader {
    uint8_t version;
    ResponseCode code;
    uint32_t payloadSize;
};

// Request Payloads
struct RegistrationRequest {
    char name[USERNAME_SIZE];
    uint8_t publicKey[PUBLIC_KEY_SIZE];
};

struct PublicKeyRequest {
    uint8_t clientID[CLIENT_ID_SIZE];
};

struct SendMessageHeader {
    uint8_t clientID[CLIENT_ID_SIZE];
    MessageType type;
    uint32_t contentSize;
};

// Response Payloads
struct RegistrationSuccessResponse {
    uint8_t clientID[CLIENT_ID_SIZE];
};

struct ClientInfoResponse {
    uint8_t clientID[CLIENT_ID_SIZE];
    char name[USERNAME_SIZE];
};

struct PublicKeyResponse {
    uint8_t clientID[CLIENT_ID_SIZE];
    uint8_t publicKey[PUBLIC_KEY_SIZE];
};

struct MessageSentResponse {
    uint8_t clientID[CLIENT_ID_SIZE];
    uint32_t messageID;
};

struct MessageHeader {
    uint8_t clientID[CLIENT_ID_SIZE];
    uint32_t messageID;
    MessageType type;
    uint32_t messageSize;
};

#pragma pack(pop)

// Helper structures for in-memory data
struct ClientInfo {
    std::vector<uint8_t> id;
    std::string name;
    std::vector<uint8_t> publicKey;
    std::vector<uint8_t> symKey;
};