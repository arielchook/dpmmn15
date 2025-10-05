import struct
from enum import IntEnum

# Constants from protocol specification
CLIENT_ID_SIZE = 16
USERNAME_SIZE = 255
PUBLIC_KEY_SIZE = 160

# Request Codes
class RequestCode(IntEnum):
    REGISTER = 1100
    CLIENTS_LIST = 1101
    PUBLIC_KEY = 1102
    SEND_MESSAGE = 1103
    PULL_MESSAGES = 1104

# Response Codes
class ResponseCode(IntEnum):
    REGISTRATION_SUCCESS = 2100
    CLIENTS_LIST = 2101
    PUBLIC_KEY = 2102
    MESSAGE_SENT = 2103
    PULL_MESSAGES = 2104
    ERROR = 9000

# Message Types
class MessageType(IntEnum):
    SYM_KEY_REQUEST = 1
    SYM_KEY_SEND = 2
    TEXT_MESSAGE = 3
    FILE_SEND = 4

# Helper class for packing/unpacking structs
class StructBase:
    _format = ""
    size = 0

    def __init__(self, *args):
        self._values = args

    def pack(self):
        return struct.pack(self._format, *self._values)

    @classmethod
    def unpack(cls, buffer):
        return cls(*struct.unpack(cls._format, buffer))

# Request Structures
class RequestHeader(StructBase):
    _format = f"<{CLIENT_ID_SIZE}sBHI" # client_id, version, code, payload_size
    size = struct.calcsize(_format)
    def __init__(self, client_id, version, code, payload_size):
        super().__init__(client_id, version, code, payload_size)
        self.client_id, self.version, self.code, self.payload_size = client_id, version, code, payload_size

class RegistrationRequestPayload(StructBase):
    _format = f"<{USERNAME_SIZE}s{PUBLIC_KEY_SIZE}s"
    size = struct.calcsize(_format)
    def __init__(self, name, public_key):
        super().__init__(name, public_key)
        self.name, self.public_key = name, public_key

class PublicKeyRequestPayload(StructBase):
    _format = f"<{CLIENT_ID_SIZE}s"
    size = struct.calcsize(_format)
    def __init__(self, client_id):
        super().__init__(client_id)
        self.client_id = client_id

class SendMessageRequestPayloadHeader(StructBase):
    _format = f"<{CLIENT_ID_SIZE}sBI" # client_id, message_type, content_size
    size = struct.calcsize(_format)
    def __init__(self, client_id, message_type, content_size):
        super().__init__(client_id, message_type, content_size)
        self.client_id, self.message_type, self.content_size = client_id, message_type, content_size

# Response Structures
class ResponseHeader(StructBase):
    _format = "<BHI" # version, code, payload_size
    size = struct.calcsize(_format)
    def __init__(self, version, code, payload_size):
        super().__init__(version, code, payload_size)
        self.version, self.code, self.payload_size = version, code, payload_size

class RegistrationSuccessPayload(StructBase):
    _format = f"<{CLIENT_ID_SIZE}s"
    size = struct.calcsize(_format)
    def __init__(self, client_id):
        super().__init__(client_id)
        self.client_id = client_id

class ClientInfo(StructBase):
    _format = f"<{CLIENT_ID_SIZE}s{USERNAME_SIZE}s"
    size = struct.calcsize(_format)
    def __init__(self, client_id, name):
        super().__init__(client_id, name)
        self.client_id, self.name = client_id, name

class PublicKeyResponsePayload(StructBase):
    _format = f"<{CLIENT_ID_SIZE}s{PUBLIC_KEY_SIZE}s"
    size = struct.calcsize(_format)
    def __init__(self, client_id, public_key):
        super().__init__(client_id, public_key)
        self.client_id, self.public_key = client_id, public_key

class MessageSentResponsePayload(StructBase):
    _format = f"<{CLIENT_ID_SIZE}sI"
    size = struct.calcsize(_format)
    def __init__(self, client_id, message_id):
        super().__init__(client_id, message_id)
        self.client_id, self.message_id = client_id, message_id

class PulledMessage:
    _header_format = f"<{CLIENT_ID_SIZE}sIBI" # from_client_id, msg_id, msg_type, msg_size
    _header_size = struct.calcsize(_header_format)

    def __init__(self, from_client_id, msg_id, msg_type, msg_size, content):
        self.from_client_id = from_client_id
        self.msg_id = msg_id
        self.msg_type = msg_type
        self.msg_size = msg_size
        self.content = content

    def pack(self):
        header = struct.pack(self._header_format, self.from_client_id, self.msg_id, self.msg_type, self.msg_size)
        return header + self.content
    
    @classmethod
    def unpack_stream(cls, buffer):
        """Unpacks a list of messages from a raw byte buffer."""
        messages = []
        offset = 0
        while offset < len(buffer):
            header_data = buffer[offset : offset + cls._header_size]
            from_client_id, msg_id, msg_type, msg_size = struct.unpack(cls._header_format, header_data)
            offset += cls._header_size
            
            content = buffer[offset : offset + msg_size]
            offset += msg_size
            
            messages.append(cls(from_client_id, msg_id, msg_type, msg_size, content))
        return messages