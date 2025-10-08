# protocol_structs.py
# author: Ariel Cohen ID: 329599187

import struct  # Used for packing data into bytes and unpacking bytes into data
from enum import IntEnum  # Used to create enumerations for codes and types

# --- Protocol Constants ---
# These constants define the fixed sizes for specific data fields in the protocol.
# Using constants ensures consistency between the client and server.

CLIENT_ID_SIZE = 16       # Size of the client's unique identifier in bytes.
USERNAME_SIZE = 255     # Maximum size of a client's username in bytes.
PUBLIC_KEY_SIZE = 160   # Size of the client's public key in bytes.


# --- Request Codes ---
# This enumeration defines the codes for requests sent from a client to the server.
# Each code corresponds to a specific action the client wants to perform.
class RequestCode(IntEnum):
    REGISTER = 1100         # Request to register a new client.
    CLIENTS_LIST = 1101     # Request for a list of all registered clients.
    PUBLIC_KEY = 1102       # Request for the public key of a specific client.
    SEND_MESSAGE = 1103     # Request to send a message to another client.
    PULL_MESSAGES = 1104    # Request to pull all pending messages for the client.


# --- Response Codes ---
# This enumeration defines the codes for responses sent from the server to a client.
# These codes indicate the result of a client's request.
class ResponseCode(IntEnum):
    REGISTRATION_SUCCESS = 2100 # Indicates that client registration was successful.
    CLIENTS_LIST = 2101         # Indicates that the response contains the client list.
    PUBLIC_KEY = 2102           # Indicates that the response contains a public key.
    MESSAGE_SENT = 2103         # Confirmation that a message was successfully sent and stored.
    PULL_MESSAGES = 2104        # Indicates that the response contains pending messages.
    ERROR = 9000                # Indicates that a general error occurred while processing the request.


# --- Message Types ---
# This enumeration defines the type of content being sent in a message.
class MessageType(IntEnum):
    SYM_KEY_REQUEST = 1 # A request for a symmetric key.
    SYM_KEY_SEND = 2    # A symmetric key being sent.
    TEXT_MESSAGE = 3    # A standard text message.
    FILE_SEND = 4       # A file being sent.


# --- Base Class for Structures ---
class StructBase:
    """
    A helper base class to abstract the logic of packing and unpacking data structures.
    This reduces code duplication and simplifies the creation of new protocol structures.
    """
    _format = ""  # The format string used by the `struct` module.
    size = 0      # The total size of the structure in bytes.

    def __init__(self, *args):
        # Store the values that will be packed into the structure.
        self._values = args

    def pack(self):
        """Packs the instance's values into a bytes object based on the _format string."""
        return struct.pack(self._format, *self._values)

    @classmethod
    def unpack(cls, buffer):
        """Unpacks a bytes object into a new instance of the class."""
        return cls(*struct.unpack(cls._format, buffer))


# --- Request Structures ---
# These classes define the exact binary structure of requests sent by the client.
# The '<' in the format string specifies little-endian byte order.

class RequestHeader(StructBase):
    """
    Defines the header that precedes every request from the client.
    It contains the client's ID, protocol version, the request code, and the size of the payload that follows.
    """
    # Format: client_id (16s), version (B), code (H), payload_size (I)
    _format = f"<{CLIENT_ID_SIZE}sBHI"
    size = struct.calcsize(_format)
    def __init__(self, client_id, version, code, payload_size):
        super().__init__(client_id, version, code, payload_size)
        self.client_id, self.version, self.code, self.payload_size = client_id, version, code, payload_size

class RegistrationRequestPayload(StructBase):
    """Defines the payload for a client registration request."""
    # Format: name (255s), public_key (160s)
    _format = f"<{USERNAME_SIZE}s{PUBLIC_KEY_SIZE}s"
    size = struct.calcsize(_format)
    def __init__(self, name, public_key):
        super().__init__(name, public_key)
        self.name, self.public_key = name, public_key

class PublicKeyRequestPayload(StructBase):
    """Defines the payload for a request to get another client's public key."""
    # Format: client_id (16s)
    _format = f"<{CLIENT_ID_SIZE}s"
    size = struct.calcsize(_format)
    def __init__(self, client_id):
        super().__init__(client_id)
        self.client_id = client_id

class SendMessageRequestPayloadHeader(StructBase):
    """Defines the header for a message being sent. The actual message content follows this header."""
    # Format: client_id (16s), message_type (B), content_size (I)
    _format = f"<{CLIENT_ID_SIZE}sBI"
    size = struct.calcsize(_format)
    def __init__(self, client_id, message_type, content_size):
        super().__init__(client_id, message_type, content_size)
        self.client_id, self.message_type, self.content_size = client_id, message_type, content_size


# --- Response Structures ---
# These classes define the exact binary structure of responses sent by the server.

class ResponseHeader(StructBase):
    """
    Defines the header that precedes every response from the server.
    It contains the protocol version, response code, and the size of the payload that follows.
    """
    # Format: version (B), code (H), payload_size (I)
    _format = "<BHI"
    size = struct.calcsize(_format)
    def __init__(self, version, code, payload_size):
        super().__init__(version, code, payload_size)
        self.version, self.code, self.payload_size = version, code, payload_size

class RegistrationSuccessPayload(StructBase):
    """Defines the payload for a successful registration, containing the new client's ID."""
    # Format: client_id (16s)
    _format = f"<{CLIENT_ID_SIZE}s"
    size = struct.calcsize(_format)
    def __init__(self, client_id):
        super().__init__(client_id)
        self.client_id = client_id

class ClientInfo(StructBase):
    """Defines the structure for sending a single client's information (ID and name)."""
    # Format: client_id (16s), name (255s)
    _format = f"<{CLIENT_ID_SIZE}s{USERNAME_SIZE}s"
    size = struct.calcsize(_format)
    def __init__(self, client_id, name):
        super().__init__(client_id, name)
        self.client_id, self.name = client_id, name

class PublicKeyResponsePayload(StructBase):
    """Defines the payload for a response containing a client's public key."""
    # Format: client_id (16s), public_key (160s)
    _format = f"<{CLIENT_ID_SIZE}s{PUBLIC_KEY_SIZE}s"
    size = struct.calcsize(_format)
    def __init__(self, client_id, public_key):
        super().__init__(client_id, public_key)
        self.client_id, self.public_key = client_id, public_key

class MessageSentResponsePayload(StructBase):
    """Defines the payload for confirming a message was sent, returning the recipient's ID and the new message ID."""
    # Format: client_id (16s), message_id (I)
    _format = f"<{CLIENT_ID_SIZE}sI"
    size = struct.calcsize(_format)
    def __init__(self, client_id, message_id):
        super().__init__(client_id, message_id)
        self.client_id, self.message_id = client_id, message_id

class PulledMessage:
    """
    A special class to handle the packing and unpacking of messages being pulled by a client.
    Unlike the other structs, a response for pulled messages can contain multiple messages,
    so this class handles variable-sized content and streaming.
    """
    # Format: from_client_id (16s), msg_id (I), msg_type (B), msg_size (I)
    _header_format = f"<{CLIENT_ID_SIZE}sIBI"
    _header_size = struct.calcsize(_header_format)

    def __init__(self, from_client_id, msg_id, msg_type, msg_size, content):
        self.from_client_id = from_client_id
        self.msg_id = msg_id
        self.msg_type = msg_type
        self.msg_size = msg_size
        self.content = content

    def pack(self):
        """Packs the message header and its content into a single bytes object."""
        header = struct.pack(self._header_format, self.from_client_id, self.msg_id, self.msg_type, self.msg_size)
        return header + self.content
    
    @classmethod
    def unpack_stream(cls, buffer):
        """
        Unpacks a raw byte buffer that may contain multiple messages.
        It reads one message at a time and returns a list of PulledMessage objects.
        """
        messages = []
        offset = 0
        # Loop until all bytes in the buffer have been processed.
        while offset < len(buffer):
            # Unpack the fixed-size header first.
            header_data = buffer[offset : offset + cls._header_size]
            from_client_id, msg_id, msg_type, msg_size = struct.unpack(cls._header_format, header_data)
            offset += cls._header_size
            
            # Read the variable-sized content based on msg_size from the header.
            content = buffer[offset : offset + msg_size]
            offset += msg_size
            
            # Create and append the message object.
            messages.append(cls(from_client_id, msg_id, msg_type, msg_size, content))
        return messages