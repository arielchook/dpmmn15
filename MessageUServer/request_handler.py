import logging
import uuid
from protocol_structs import *

class RequestHandler:
    """
    Parses client requests and generates server responses according to the protocol.
    """
    def __init__(self, data_manager, server_version):
        self._data_manager = data_manager
        self._server_version = server_version
        self._request_handlers = {
            RequestCode.REGISTER: self._handle_registration,
            RequestCode.CLIENTS_LIST: self._handle_clients_list,
            RequestCode.PUBLIC_KEY: self._handle_public_key,
            RequestCode.SEND_MESSAGE: self._handle_send_message,
            RequestCode.PULL_MESSAGES: self._handle_pull_messages,
        }

    def handle_request(self, sock):
        """
        Reads a full request from the socket, processes it, and returns a response.
        Returns None if the client disconnected.
        """
        try:
            header_data = sock.recv(RequestHeader.size)
            if not header_data:
                return None # Client disconnected
            
            header = RequestHeader.unpack(header_data)
            payload = sock.recv(header.payload_size) if header.payload_size > 0 else b''

            # Update LastSeen for the client
            if header.code!= RequestCode.REGISTER:
                self._data_manager.update_last_seen(header.client_id)

            handler = self._request_handlers.get(header.code)
            if handler:
                return handler(header, payload)
            else:
                logging.warning(f"Unknown request code: {header.code}")
                return self._create_error_response()
        except ConnectionResetError:
            return None # Client disconnected abruptly
        except Exception as e:
            logging.error(f"Error processing request: {e}")
            return self._create_error_response()

    def _create_error_response(self):
        """Creates a generic error response."""
        return ResponseHeader(self._server_version, ResponseCode.ERROR, 0).pack()

    def _handle_registration(self, header, payload):
        """Handles user registration request."""
        req = RegistrationRequestPayload.unpack(payload)
        username = req.name.split(b'\x00', 1)[0].decode('ascii')

        if self._data_manager.username_exists(username):
            logging.warning(f"Registration failed: username '{username}' already exists.")
            return self._create_error_response()

        client_id = uuid.uuid4().bytes
        self._data_manager.add_client(client_id, username, req.public_key)
        logging.info(f"Registered new user '{username}' with ID {client_id.hex()}.")

        response_payload = RegistrationSuccessPayload(client_id).pack()
        response_header = ResponseHeader(self._server_version, ResponseCode.REGISTRATION_SUCCESS, len(response_payload))
        return response_header.pack() + response_payload

    def _handle_clients_list(self, header, payload):
        """Handles request for the list of registered clients."""
        clients = self._data_manager.get_clients(exclude_id=header.client_id)
        payload_data = b"".join([ClientInfo(c['ID'], c['UserName'].encode('ascii')).pack() for c in clients])
        
        response_header = ResponseHeader(self._server_version, ResponseCode.CLIENTS_LIST, len(payload_data))
        return response_header.pack() + payload_data

    def _handle_public_key(self, header, payload):
        """Handles request for a client's public key."""
        req = PublicKeyRequestPayload.unpack(payload)
        client = self._data_manager.get_client_by_id(req.client_id)

        if not client:
            logging.warning(f"Public key request for non-existent client ID {req.client_id.hex()}.")
            return self._create_error_response()

        response_payload = PublicKeyResponsePayload(client['ID'], client['PublicKey']).pack()
        response_header = ResponseHeader(self._server_version, ResponseCode.PUBLIC_KEY, len(response_payload))
        return response_header.pack() + response_payload

    def _handle_send_message(self, header, payload):
        """Handles storing a message for a client."""
        msg_header_size = SendMessageRequestPayloadHeader.size
        if len(payload) < msg_header_size:
            logging.warning(f"Payload too small for message header.")
            return self._create_error_response()

        req_header = SendMessageRequestPayloadHeader.unpack(payload[:msg_header_size])
        content = payload[msg_header_size:]

        if not self._data_manager.client_id_exists(req_header.client_id):
            logging.warning(f"Attempt to send message to non-existent client ID {req_header.client_id.hex()}.")
            return self._create_error_response()

        message_id = self._data_manager.add_message(
            to_client_id=req_header.client_id,
            from_client_id=header.client_id,
            msg_type=req_header.message_type,
            content=content
        )
        
        response_payload = MessageSentResponsePayload(req_header.client_id, message_id).pack()
        response_header = ResponseHeader(self._server_version, ResponseCode.MESSAGE_SENT, len(response_payload))
        return response_header.pack() + response_payload

    def _handle_pull_messages(self, header, payload):
        """Handles request to pull waiting messages."""
        messages = self._data_manager.get_messages_for_client(header.client_id)
        
        payload_data = b""
        message_ids_to_delete = []
        for msg in messages:
            payload_data += PulledMessage(
                msg['FromClient'], msg['ID'], msg['Type'], len(msg['Content']), msg['Content']
            ).pack()
            message_ids_to_delete.append(msg['ID'])

        response_header = ResponseHeader(self._server_version, ResponseCode.PULL_MESSAGES, len(payload_data))
        
        # After successfully preparing the response, delete the messages
        if message_ids_to_delete:
            self._data_manager.delete_messages(message_ids_to_delete)
            logging.info(f"Sent and deleted {len(message_ids_to_delete)} messages for client {header.client_id.hex()}.")

        return response_header.pack() + payload_data