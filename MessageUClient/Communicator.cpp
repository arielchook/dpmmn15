// main.cpp
// author: Ariel Cohen ID: 329599187

#include "Communicator.h"
#include <iostream>

/// <summary>
/// Constructs a Communicator object and initializes the network endpoint with the specified IP address and port.
/// </summary>
/// <param name="ip">The IP address to connect to, as a string.</param>
/// <param name="port">The port number to connect to.</param>
Communicator::Communicator(const std::string& ip, uint16_t port)
    : _io_context(), _socket(_io_context) {
    boost::asio::ip::address addr = boost::asio::ip::make_address(ip);
    _endpoint = boost::asio::ip::tcp::endpoint(addr, port);
}

std::optional<std::vector<uint8_t>> Communicator::sendAndReceive(RequestCode code, const std::vector<uint8_t>& payload, const std::vector<uint8_t>& clientID) {
    try {
        // Connect on each request as server is stateless
        _socket = boost::asio::ip::tcp::socket(_io_context);
        _socket.connect(_endpoint);

        RequestHeader header{};
        if (!clientID.empty()) {
            std::copy(clientID.begin(), clientID.end(), header.clientID);
        }
        header.version = CLIENT_VERSION;
        header.code = code;
        header.payloadSize = static_cast<uint32_t>(payload.size());

        // Send header and payload
        boost::asio::write(_socket, boost::asio::buffer(&header, sizeof(header)));
        if (!payload.empty()) {
            boost::asio::write(_socket, boost::asio::buffer(payload));
        }

        // Read response header
        ResponseHeader responseHeader{};
        boost::asio::read(_socket, boost::asio::buffer(&responseHeader, sizeof(responseHeader)));

        if (static_cast<ResponseCode>(responseHeader.code) == (ResponseCode::GENERAL_ERROR)) {
            std::cerr << "Server responded with an error." << std::endl;
            return std::nullopt;
        }

        // Read response payload
        std::vector<uint8_t> responsePayload(responseHeader.payloadSize);
        if (responseHeader.payloadSize > 0) {
            boost::asio::read(_socket, boost::asio::buffer(responsePayload));
        }

        // Close socket
        boost::system::error_code ec;
        _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        _socket.close();

        return responsePayload;
    }
    catch (const boost::system::system_error& e) {
        std::cerr << "Network error: " << e.what() << std::endl;
        return std::nullopt;
    }
}