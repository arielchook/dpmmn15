import socket
import selectors
import logging
from request_handler import RequestHandler
from data_manager import SQLiteDataManager # Or RAMDataManager for base version

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

SERVER_VERSION = 2 # 2 for DB version

class Server:
    """
    The main MessageU server class. Manages network connections and dispatches requests.
    """
    def __init__(self, host, port):

        self._host = host
        self._port = port
        self._selector = selectors.DefaultSelector()
        # Use sqlite db persistence for bonus points
        self._data_manager = SQLiteDataManager('dpmmn15.db') 
        # handler for processing requests
        self._request_handler = RequestHandler(self._data_manager, SERVER_VERSION)
        self._server_socket = None

    def _accept_connection(self, key, mask):
        """Callback for new connections."""
        sock = key.fileobj
        conn, addr = sock.accept()
        logging.info(f"Accepted connection from {addr}")
        conn.setblocking(False)
        # Register the new connection for read events
        self._selector.register(conn, selectors.EVENT_READ, self._service_connection)

    def _service_connection(self, key, mask):
        """Callback for handling data from a client."""
        sock = key.fileobj
        try:
            response = self._request_handler.handle_request(sock)
            if response:
                sock.sendall(response)
            else: # Client disconnected
                logging.info(f"Client {sock.getpeername()} disconnected.")
                self._selector.unregister(sock)
                sock.close()
        except Exception as e:
            logging.error(f"Error handling connection from {sock.getpeername()}: {e}")
            self._selector.unregister(sock)
            sock.close()

    def start(self):
        """Starts the server's listening loop."""
        try:
            self._server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self._server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self._server_socket.bind((self._host, self._port))
            self._server_socket.listen()
            self._server_socket.setblocking(False)
            self._selector.register(self._server_socket, selectors.EVENT_READ, self._accept_connection)
            logging.info(f"Server version {SERVER_VERSION} listening on {self._host}:{self._port}")

            while True:
                events = self._selector.select(timeout=None)
                for key, mask in events:
                    callback = key.data
                    callback(key, mask)

        # User pressed Ctrl-C to stop the server
        except KeyboardInterrupt:
            logging.info("Server is shutting down.")
        finally:
            if self._server_socket:
                self._server_socket.close()
            self._selector.close()
            self._data_manager.close()

def get_port_from_file(filename="myport.info", default_port=1357):
    """Reads the port number from a file. Default to 1357 if file not found or invalid."""
    try:
        with open(filename, 'r') as f:
            return int(f.read().strip())
    except (FileNotFoundError, ValueError):
        logging.warning(f"'{filename}' not found or invalid. Using default port {default_port}.")
        return default_port

def main():
    """Main function to run the server."""
    host = '0.0.0.0' # Listen on all interfaces
    port = get_port_from_file() # Default to 1357 if file not found or invalid
    server = Server(host, port)  
    server.start()

if __name__ == "__main__":
    main()