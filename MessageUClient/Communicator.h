// main.cpp
// author: Ariel Cohen ID: 329599187

#pragma once
#include "Protocol.h"
#include <boost/asio.hpp>
#include <optional>

/**
 * @brief Represents a TCP communicator for sending requests and receiving responses over a network connection.
 */
class Communicator {
public:
    /**
     * @brief Constructs a Communicator object.
     * @param ip The IP address of the server.
     * @param port The port number of the server.
     */
    Communicator(const std::string& ip, uint16_t port);

    /**
     * @brief Sends a request with the specified payload and client ID, and receives an optional response.
     * @param code The request code indicating the type of operation to perform.
     * @param payload The data to send as the request payload.
     * @param clientID The identifier of the client making the request.
     * @return An optional vector of bytes containing the response data if available; std::nullopt if no response is received.
     */
    std::optional<std::vector<uint8_t>> sendAndReceive(RequestCode code, const std::vector<uint8_t>& payload, const std::vector<uint8_t>& clientID);

private:
	boost::asio::io_context _io_context;        // ASIO I/O context
	boost::asio::ip::tcp::socket _socket;       // TCP socket for communication
	boost::asio::ip::tcp::endpoint _endpoint;   // represents the server endpoint
};