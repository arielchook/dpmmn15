// main.cpp
// author: Ariel Cohen ID: 329599187

#include "Client.h"
#include <iostream>

// Main entry point
int main() {
	// Wrap in try-catch to handle any unexpected errors
    try {
        Client client;
        client.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}