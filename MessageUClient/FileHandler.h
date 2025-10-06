// main.cpp
// author: Ariel Cohen ID: 329599187

#pragma once
#include <string>
#include <vector>
#include <optional>

struct ServerInfo {
    std::string ip;
    uint16_t port;
};

struct UserInfo {
    std::string username;
    std::vector<uint8_t> uuid;
    std::string privateKey; // Base64 encoded
};

class FileHandler {
public:
    static std::optional<ServerInfo> readServerInfo(const std::string& filename = "server.info");
    static std::optional<UserInfo> readMyInfo(const std::string& filename = "my.info");
    static bool writeMyInfo(const UserInfo& info, const std::string& filename = "my.info");
    static bool myInfoExists(const std::string& filename = "my.info");
    static std::optional<std::vector<uint8_t>> readFileContent(const std::string& filepath);
    static std::string writeToTempFile(const std::vector<uint8_t>& content);
}; 
