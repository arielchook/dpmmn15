# data_manager.py
# author: Ariel Cohen ID: 329599187

import sqlite3  # For database operations
import datetime  # For timestamping
import logging  # For logging events

class SQLiteDataManager:
    """
    This class is responsible for all database operations.
    It handles the connection to the SQLite database, table creation,
    and all CRUD (Create, Read, Update, Delete) operations for clients and messages.
    """
    def __init__(self, db_file):
        # Configure logging
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

        # Establish a connection to the database file.
        self._conn = sqlite3.connect(db_file)
        # Set the row factory to access columns by name.
        self._conn.row_factory = sqlite3.Row
        logging.info("Database connection established.")
        # Create necessary tables if they don't exist.
        self._create_tables()

    def _create_tables(self):
        """Create the 'clients' and 'messages' tables if they don't already exist."""
        logging.info("Creating database tables if they don't exist...")
        cursor = self._conn.cursor()
        # SQL statement to create the 'clients' table.
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS clients (
                ID BLOB(16) PRIMARY KEY,
                UserName VARCHAR(255) NOT NULL UNIQUE,
                PublicKey BLOB(160) NOT NULL,
                LastSeen DATETIME NOT NULL
            )
        ''')
        # SQL statement to create the 'messages' table.
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS messages (
                ID INTEGER PRIMARY KEY AUTOINCREMENT,
                ToClient BLOB(16) NOT NULL,
                FromClient BLOB(16) NOT NULL,
                Type TINYINT NOT NULL,
                Content BLOB,
                FOREIGN KEY(ToClient) REFERENCES clients(ID),
                FOREIGN KEY(FromClient) REFERENCES clients(ID)
            )
        ''')
        # Commit the changes to the database.
        self._conn.commit()
        logging.info("Tables created successfully.")

    def add_client(self, client_id, username, public_key):
        """Add a new client to the database."""
        logging.info(f"Adding new client: {username}")
        cursor = self._conn.cursor()
        # Insert a new record into the 'clients' table.
        cursor.execute("INSERT INTO clients (ID, UserName, PublicKey, LastSeen) VALUES (?,?,?,?)",
                       (client_id, username, public_key, datetime.datetime.now()))
        self._conn.commit()
        logging.info(f"Client {username} added successfully.")

    def username_exists(self, username):
        """Check if a username already exists in the database."""
        logging.info(f"Checking if username '{username}' exists.")
        cursor = self._conn.cursor()
        cursor.execute("SELECT distinct 1 FROM clients WHERE UserName =?", (username,))
        exists = cursor.fetchone() is not None
        logging.info(f"Username '{username}' exists: {exists}")
        return exists

    def client_id_exists(self, client_id):
        """Check if a client ID already exists in the database."""
        cursor = self._conn.cursor()
        cursor.execute("SELECT distinct 1 FROM clients WHERE ID =?", (client_id,))
        return cursor.fetchone() is not None

    def get_clients(self, exclude_id):
        """Retrieve a list of all clients, excluding the one with the specified ID."""
        cursor = self._conn.cursor()
        cursor.execute("SELECT ID, UserName FROM clients WHERE ID!=?", (exclude_id,))
        return cursor.fetchall()

    def get_client_by_id(self, client_id):
        """Fetch a single client's details by their ID."""
        cursor = self._conn.cursor()
        cursor.execute("SELECT * FROM clients WHERE ID =?", (client_id,))
        return cursor.fetchone()

    def update_last_seen(self, client_id):
        """Update the 'LastSeen' timestamp for a specific client."""
        cursor = self._conn.cursor()
        cursor.execute("UPDATE clients SET LastSeen =? WHERE ID =?", (datetime.datetime.now(), client_id))
        self._conn.commit()

    def add_message(self, to_client_id, from_client_id, msg_type, content):
        """Store a new message in the database."""
        logging.info(f"Adding message from {from_client_id} to {to_client_id}.")
        cursor = self._conn.cursor()
        cursor.execute("INSERT INTO messages (ToClient, FromClient, Type, Content) VALUES (?,?,?,?)",
                       (to_client_id, from_client_id, msg_type, content))
        self._conn.commit()
        last_id = cursor.lastrowid
        logging.info(f"Message added with ID: {last_id}")
        return last_id

    def get_messages_for_client(self, client_id):
        """Retrieve all pending messages for a specific client."""
        logging.info(f"Retrieving messages for client {client_id}.")
        cursor = self._conn.cursor()
        cursor.execute("SELECT ID, FromClient, Type, Content FROM messages WHERE ToClient =?", (client_id,))
        messages = cursor.fetchall()
        logging.info(f"Found {len(messages)} messages for client {client_id}.")
        return messages

    def delete_messages(self, message_ids):
        """Delete messages from the database using a list of message IDs."""
        cursor = self._conn.cursor()
        # Create a placeholder string for the SQL query, e.g., (?,?,?).
        placeholders = ', '.join('?' for _ in message_ids)
        cursor.execute(f"DELETE FROM messages WHERE ID IN ({placeholders})", message_ids)
        self._conn.commit()

    def close(self):
        """Close the connection to the database."""
        logging.info("Closing database connection.")
        self._conn.close()
