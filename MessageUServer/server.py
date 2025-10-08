# server.py 
# author: Ariel Cohen ID: 329599187

import socket      # For network connections
import selectors   # For managing multiple connections efficiently
import logging     # For logging server status and errors
from request_handler import RequestHandler
from data_manager import SQLiteDataManager

# --- Configuration ---
# Configure logging to display timestamps, log level, and messages.
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

SERVER_VERSION = 2  # The version of the server protocol (with DB support)

class Server:
    """
    The main server class for the MessageU application.
    It handles incoming network connections, and uses a selector to manage
    multiple clients simultaneously without blocking.
    """
    def __init__(self, host, port):
        self._host = host
        self._port = port
        # The selector helps manage I/O events for multiple sockets.
        self._selector = selectors.DefaultSelector()
        # Initialize the data manager for database persistence.
        self._data_manager = SQLiteDataManager('dpmmn15.db') 
        # The request handler processes all incoming requests.
        self._request_handler = RequestHandler(self._data_manager, SERVER_VERSION)
        self._server_socket = None

    def _accept_connection(self, key, mask):
        """Callback function for handling new connections to the server socket."""
        sock = key.fileobj
        conn, addr = sock.accept()
        logging.info(f"Accepted connection from {addr}")
        conn.setblocking(False)  # Set the new socket to non-blocking mode.
        # Register the new client socket with the selector to monitor it for read events.
        self._selector.register(conn, selectors.EVENT_READ, self._service_connection)

    def _service_connection(self, key, mask):
        """Callback function for handling data received from a client."""
        sock = key.fileobj
        try:
            # Pass the socket to the request handler to process the request.
            response = self._request_handler.handle_request(sock)
            if response:
                # If a response was generated, send it all back to the client.
                sock.sendall(response)
            else:
                # If the handler returns None, it means the client has disconnected.
                logging.info(f"Client {sock.getpeername()} disconnected.")
                self._selector.unregister(sock)
                sock.close()
        except Exception as e:
            # In case of any other error, log it and close the connection.
            logging.error(f"Error handling connection from {sock.getpeername()}: {e}")
            self._selector.unregister(sock)
            sock.close()

    def start(self):
        """Starts the server, sets up the listening socket, and enters the main event loop."""
        try:
            self._server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            # Allow reusing the address to avoid "Address already in use" errors on restart.
            self._server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self._server_socket.bind((self._host, self._port))
            self._server_socket.listen()
            self._server_socket.setblocking(False)  # Set the server socket to non-blocking.
            # Register the server socket to accept new connections.
            self._selector.register(self._server_socket, selectors.EVENT_READ, self._accept_connection)
            logging.info(f"Server version {SERVER_VERSION} listening on {self._host}:{self._port}")

            # The main server loop.
            while True:
                # Wait for an event (new connection, data received, etc.).
                events = self._selector.select(timeout=None)
                for key, mask in events:
                    # Get the callback associated with the event and call it.
                    callback = key.data
                    callback(key, mask)

        except KeyboardInterrupt:
            # Handle Ctrl+C to gracefully shut down the server.
            logging.info("Server is shutting down.")
        finally:
            # Clean up all resources.
            if self._server_socket:
                self._server_socket.close()
            self._selector.close()
            self._data_manager.close()

def get_port_from_file(filename="myport.info", default_port=1357):
    """Reads the port number from a file, with a fallback to a default port."""
    try:
        with open(filename, 'r') as f:
            return int(f.read().strip())
    except (FileNotFoundError, ValueError):
        logging.warning(f"'{filename}' not found or invalid. Using default port {default_port}.")
        return default_port

def main():
    """The main entry point of the application."""
    host = '0.0.0.0'  # Listen on all available network interfaces.
    port = get_port_from_file()  # Get port from file or use default.
    server = Server(host, port)  
    server.start()

# This block ensures that main() is called only when the script is executed directly.
if __name__ == "__main__":
    main()
