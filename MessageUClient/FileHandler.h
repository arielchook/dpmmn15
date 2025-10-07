// main.cpp
// author: Ariel Cohen ID: 329599187

#pragma once
#include <string>
#include <vector>
#include <optional>

/**
 * @brief Holds the server's connection information.
 */
struct ServerInfo {
    std::string ip;
    uint16_t port;
};

/**
 * @brief Holds the current user's information, as read from/written to my.info.
 */
struct UserInfo {
    std::string username;
    std::vector<uint8_t> uuid;
    std::string privateKey; // The private key is stored in Base64 format.
};

/**
 * @brief A static utility class for all file I/O operations.
 */
class FileHandler {
public:
    /**
     * @brief Reads server connection details from server.info.
     * @param filename The name of the file to read from.
     * @return An optional ServerInfo struct, or std::nullopt if the file cannot be read.
     */
    static std::optional<ServerInfo> readServerInfo(const std::string& filename = "server.info");

    /**
     * @brief Reads the current user's info (name, UUID, private key) from my.info.
     * @param filename The name of the file to read from.
     * @return An optional UserInfo struct, or std::nullopt if the file cannot be read.
     */
    static std::optional<UserInfo> readMyInfo(const std::string& filename = "my.info");

    /**
     * @brief Writes the current user's info to my.info.
     * @param info The UserInfo struct to write.
     * @param filename The name of the file to write to.
     * @return True if the file was written successfully, false otherwise.
     */
    static bool writeMyInfo(const UserInfo& info, const std::string& filename = "my.info");

    /**
     * @brief Checks if the my.info file exists.
     * @param filename The name of the file to check.
     * @return True if the file exists, false otherwise.
     */
    static bool myInfoExists(const std::string& filename = "my.info");

    /**
     * @brief Reads the entire content of a binary file into a byte vector.
     * @param filepath The path to the file to read.
     * @return A vector of bytes, or std::nullopt if the file cannot be read.
     */
    static std::optional<std::vector<uint8_t>> readFileContent(const std::string& filepath);

    /**
     * @brief Writes a vector of bytes to a new file with a unique name in the system's temp directory.
     * @param content The content to write to the file.
     * @return The full path to the newly created file.
     */
    static std::string writeToTempFile(const std::vector<uint8_t>& content);

    /**
     * @brief Converts a vector of bytes to a hex string.
     * @param bytes The vector of bytes to convert.
     * @return The hex string.
     */
    static std::string bytesToHex(const std::vector<uint8_t>& bytes);
};