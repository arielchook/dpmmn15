// main.cpp
// author: Ariel Cohen ID: 329599187

#include "FileHandler.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <boost/filesystem.hpp>
#include <chrono>

/**
 * @brief Helper function to convert a hex string to a byte vector.
 * @param hex The hex string to convert.
 * @return The byte vector.
 */
static std::vector<uint8_t> hexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (unsigned int i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = (uint8_t)strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

/**
 * @brief Converts a vector of bytes to a hex string.
 * @param bytes The vector of bytes to convert.
 * @return The hex string.
 */
std::string FileHandler::bytesToHex(const std::vector<uint8_t>& bytes) {
    std::stringstream ss;
    ss << std::hex;
    for (const auto& byte : bytes) {
        ss << std::setw(2) << std::setfill('0') << (int)byte;
    }
    return ss.str();
}

/**
 * @brief Reads server connection details from server.info.
 * @param filename The name of the file to read from.
 * @return An optional ServerInfo struct, or std::nullopt if the file cannot be read.
 */
std::optional<ServerInfo> FileHandler::readServerInfo(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return std::nullopt;
    }
    std::string line;
    if (std::getline(file, line)) {
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            ServerInfo info;
            info.ip = line.substr(0, colon_pos);
            info.port = std::stoi(line.substr(colon_pos + 1));
            return info;
        }
    }
    return std::nullopt;
}

/**
 * @brief Reads the current user's info (name, UUID, private key) from my.info.
 * @param filename The name of the file to read from.
 * @return An optional UserInfo struct, or std::nullopt if the file cannot be read.
 */
std::optional<UserInfo> FileHandler::readMyInfo(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return std::nullopt;
    }
    UserInfo info;
    std::string uuid_hex;
    std::getline(file, info.username);
    std::getline(file, uuid_hex);
    std::getline(file, info.privateKey);
    info.uuid = hexToBytes(uuid_hex);
    return info;
}

/**
 * @brief Writes the current user's info to my.info.
 * @param info The UserInfo struct to write.
 * @param filename The name of the file to write to.
 * @return True if the file was written successfully, false otherwise.
 */
bool FileHandler::writeMyInfo(const UserInfo& info, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    file << info.username << std::endl;
    file << bytesToHex(info.uuid) << std::endl;
    file << info.privateKey << std::endl;
    return true;
}

/**
 * @brief Checks if the my.info file exists.
 * @param filename The name of the file to check.
 * @return True if the file exists, false otherwise.
 */
bool FileHandler::myInfoExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

/**
 * @brief Reads the entire content of a binary file into a byte vector.
 * @param filepath The path to the file to read.
 * @return A vector of bytes, or std::nullopt if the file cannot be read.
 */
std::optional<std::vector<uint8_t>> FileHandler::readFileContent(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        return std::nullopt;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return buffer;
    }
    return std::nullopt;
}

/**
 * @brief Writes a vector of bytes to a new file with a unique name in the system's temp directory.
 * @param content The content to write to the file.
 * @return The full path to the newly created file.
 */
std::string FileHandler::writeToTempFile(const std::vector<uint8_t>& content) {
    // get temp directory in cross-platform way using boost
    boost::filesystem::path tempDir = boost::filesystem::temp_directory_path();

    // create unique filename using timestamp
    auto timestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::string filename = "msgU_" + std::to_string(timestamp);

    boost::filesystem::path fullPath = tempDir / filename;

    std::ofstream file(fullPath.string(), std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open temp file for writing: " + fullPath.string());
    }
    file.write(reinterpret_cast<const char*>(content.data()), content.size());
    return fullPath.string();
}