# request_handler.py
# author: Ariel Cohen ID: 329599187

import logging  # For logging server activity
import uuid     # For generating unique client IDs
from protocol_structs import *  # Import all protocol definitions

class RequestHandler:
    """
    This class is the core logic unit of the server.
    It takes raw socket data, parses it into a defined request format,
    processes the request using the data manager, and constructs a binary response
    to be sent back to the client.
    """
    def __init__(self, data_manager, server_version):
        self._data_manager = data_manager  # An instance of SQLiteDataManager
        self._server_version = server_version
        # A dictionary that maps request codes to their handler methods.
        # This is a clean way to manage different request types.
        self._request_handlers = {
            RequestCode.REGISTER: self._handle_registration,
            RequestCode.CLIENTS_LIST: self._handle_clients_list,
            RequestCode.PUBLIC_KEY: self._handle_public_key,
            RequestCode.SEND_MESSAGE: self._handle_send_message,
            RequestCode.PULL_MESSAGES: self._handle_pull_messages,
        }

    def handle_request(self, sock):
        """
        Reads and processes a single request from a client socket.
        
        Returns:
            bytes: The response to be sent back to the client.
            None: If the client has disconnected.
        """
        try:
            # Read the fixed-size request header first.
            header_data = sock.recv(RequestHeader.size)
            if not header_data:
                return None  # Client disconnected gracefully
            
            header = RequestHeader.unpack(header_data)
            
            # Read the payload, if any.
            payload = sock.recv(header.payload_size) if header.payload_size > 0 else b''

            # Update the client's 'LastSeen' timestamp for any request other than registration.
            if header.code != RequestCode.REGISTER:
                self._data_manager.update_last_seen(header.client_id)

            # Look up the appropriate handler for the request code.
            handler = self._request_handlers.get(header.code)
            if handler:
                # Call the handler and return its response.
                return handler(header, payload)
            else:
                # If the code is unknown, log a warning and send an error.
                logging.warning(f"Unknown request code: {header.code}")
                return self._create_error_response()
        except ConnectionResetError:
            return None  # Client disconnected abruptly
        except Exception as e:
            logging.error(f"Error processing request: {e}")
            return self._create_error_response()

    def _create_error_response(self):
        """Creates a generic error response to send to the client."""
        header = ResponseHeader(self._server_version, ResponseCode.ERROR, 0)
        return header.pack()

    def _handle_registration(self, header, payload):
        """Handles a client registration request."""
        logging.info("Handling registration request.")
        req = RegistrationRequestPayload.unpack(payload)
        # Decode username, removing any null padding.
        username = req.name.split(b'\x00', 1)[0].decode('ascii')

        # Check if the username is already taken.
        if self._data_manager.username_exists(username):
            logging.warning(f"Registration failed: username '{username}' already exists.")
            return self._create_error_response()

        # Generate a new unique ID for the client.
        client_id = uuid.uuid4().bytes
        self._data_manager.add_client(client_id, username, req.public_key)
        logging.info(f"Registered new user '{username}' with ID {client_id.hex()}.")

        # Create and send a success response with the new client ID.
        response_payload = RegistrationSuccessPayload(client_id).pack()
        response_header = ResponseHeader(self._server_version, ResponseCode.REGISTRATION_SUCCESS, len(response_payload))
        return response_header.pack() + response_payload

    def _handle_clients_list(self, header, payload):
        """Handles a request for the list of registered clients."""
        logging.info(f"Handling clients list request from {header.client_id.hex()}.")
        # Fetch all clients except the one making the request.
        clients = self._data_manager.get_clients(exclude_id=header.client_id)
        # Pack each client's info into a single byte string.
        payload_data = b"".join([ClientInfo(c['ID'], c['UserName'].encode('ascii')).pack() for c in clients])
        
        response_header = ResponseHeader(self._server_version, ResponseCode.CLIENTS_LIST, len(payload_data))
        return response_header.pack() + payload_data

    def _handle_public_key(self, header, payload):
        """Handles a request for another client's public key."""
        logging.info(f"Handling public key request from {header.client_id.hex()}.")
        req = PublicKeyRequestPayload.unpack(payload)
        client = self._data_manager.get_client_by_id(req.client_id)

        if not client:
            logging.warning(f"Public key request for non-existent client ID {req.client_id.hex()}.")
            return self._create_error_response()

        # Create and send a response containing the requested public key.
        response_payload = PublicKeyResponsePayload(client['ID'], client['PublicKey']).pack()
        response_header = ResponseHeader(self._server_version, ResponseCode.PUBLIC_KEY, len(response_payload))
        return response_header.pack() + response_payload

    def _handle_send_message(self, header, payload):
        """Handles a request to store a message for another client."""
        logging.info(f"Handling send message request from {header.client_id.hex()}.")
        msg_header_size = SendMessageRequestPayloadHeader.size
        if len(payload) < msg_header_size:
            logging.warning(f"Payload too small for message header.")
            return self._create_error_response()

        # Unpack the message header and get the content.
        req_header = SendMessageRequestPayloadHeader.unpack(payload[:msg_header_size])
        content = payload[msg_header_size:]

        # Ensure the recipient client exists.
        if not self._data_manager.client_id_exists(req_header.client_id):
            logging.warning(f"Attempt to send message to non-existent client ID {req_header.client_id.hex()}.")
            return self._create_error_response()

        # Add the message to the database.
        message_id = self._data_manager.add_message(
            to_client_id=req_header.client_id,
            from_client_id=header.client_id,
            msg_type=req_header.message_type,
            content=content
        )
        
        # Send a confirmation response.
        response_payload = MessageSentResponsePayload(req_header.client_id, message_id).pack()
        response_header = ResponseHeader(self._server_version, ResponseCode.MESSAGE_SENT, len(response_payload))
        return response_header.pack() + response_payload

    def _handle_pull_messages(self, header, payload):
        """Handles a client's request to pull all their pending messages."""
        logging.info(f"Handling pull messages request from {header.client_id.hex()}.")
        messages = self._data_manager.get_messages_for_client(header.client_id)
        
        payload_data = b""
        message_ids_to_delete = []
        # Iterate through all pending messages and pack them for sending.
        for msg in messages:
            payload_data += PulledMessage(
                msg['FromClient'], msg['ID'], msg['Type'], len(msg['Content']), msg['Content']
            ).pack()
            message_ids_to_delete.append(msg['ID'])

        response_header = ResponseHeader(self._server_version, ResponseCode.PULL_MESSAGES, len(payload_data))
        
        # Once the messages are packed and ready to be sent, delete them from the database.
        # This ensures messages are delivered at least once.
        if message_ids_to_delete:
            self._data_manager.delete_messages(message_ids_to_delete)
            logging.info(f"Sent and deleted {len(message_ids_to_delete)} messages for client {header.client_id.hex()}.")

        return response_header.pack() + payload_data
