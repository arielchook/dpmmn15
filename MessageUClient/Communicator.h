// main.cpp
// author: Ariel Cohen ID: 329599187

#pragma once
#include "Protocol.h"
#include <boost/asio.hpp>
#include <optional>

/// <summary>
/// Represents a TCP communicator for sending requests and receiving responses over a network connection.
/// </summary>
class Communicator {
public:
    Communicator(const std::string& ip, uint16_t port);

    /// <summary>
    /// Sends a request with the specified payload and client ID, and receives an optional response.
    /// </summary>
    /// <param name="code">The request code indicating the type of operation to perform.</param>
    /// <param name="payload">The data to send as the request payload.</param>
    /// <param name="clientID">The identifier of the client making the request.</param>
    /// <returns>An optional vector of bytes containing the response data if available; std::nullopt if no response is received.</returns>
    std::optional<std::vector<uint8_t>> sendAndReceive(RequestCode code, const std::vector<uint8_t>& payload, const std::vector<uint8_t>& clientID);

private:
	boost::asio::io_context _io_context;        // ASIO I/O context
	boost::asio::ip::tcp::socket _socket;       // TCP socket for communication
	boost::asio::ip::tcp::endpoint _endpoint;   // represents the server endpoint
};