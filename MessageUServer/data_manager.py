import sqlite3
import datetime
import logging

class SQLiteDataManager:
    """Manages data persistence using an SQLite database."""
    def __init__(self, db_file):
        self._conn = sqlite3.connect(db_file)
        self._conn.row_factory = sqlite3.Row # Access columns by name
        self._create_tables()

    def _create_tables(self):
        cursor = self._conn.cursor()
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS clients (
                ID BLOB(16) PRIMARY KEY,
                UserName VARCHAR(255) NOT NULL UNIQUE,
                PublicKey BLOB(160) NOT NULL,
                LastSeen DATETIME NOT NULL
            )
        ''')
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
        self._conn.commit()

    def add_client(self, client_id, username, public_key):
        cursor = self._conn.cursor()
        cursor.execute("INSERT INTO clients (ID, UserName, PublicKey, LastSeen) VALUES (?,?,?,?)",
                       (client_id, username, public_key, datetime.datetime.now()))
        self._conn.commit()

    def username_exists(self, username):
        cursor = self._conn.cursor()
        cursor.execute("SELECT distinct 1 FROM clients WHERE UserName =?", (username,))
        return cursor.fetchone() is not None

    def client_id_exists(self, client_id):
        cursor = self._conn.cursor()
        cursor.execute("SELECT distinct 1 FROM clients WHERE ID =?", (client_id,))
        return cursor.fetchone() is not None

    def get_clients(self, exclude_id):
        cursor = self._conn.cursor()
        cursor.execute("SELECT ID, UserName FROM clients WHERE ID!=?", (exclude_id,))
        return cursor.fetchall()

    def get_client_by_id(self, client_id):
        cursor = self._conn.cursor()
        cursor.execute("SELECT * FROM clients WHERE ID =?", (client_id,))
        return cursor.fetchone()

    def update_last_seen(self, client_id):
        cursor = self._conn.cursor()
        cursor.execute("UPDATE clients SET LastSeen =? WHERE ID =?", (datetime.datetime.now(), client_id))
        self._conn.commit()

    def add_message(self, to_client_id, from_client_id, msg_type, content):
        cursor = self._conn.cursor()
        cursor.execute("INSERT INTO messages (ToClient, FromClient, Type, Content) VALUES (?,?,?,?)",
                       (to_client_id, from_client_id, msg_type, content))
        self._conn.commit()
        return cursor.lastrowid

    def get_messages_for_client(self, client_id):
        cursor = self._conn.cursor()
        cursor.execute("SELECT ID, FromClient, Type, Content FROM messages WHERE ToClient =?", (client_id,))
        return cursor.fetchall()

    def delete_messages(self, message_ids):
        cursor = self._conn.cursor()
        # Create a placeholder string like (?,?,?) for the number of IDs
        placeholders = ', '.join('?' for _ in message_ids)
        cursor.execute(f"DELETE FROM messages WHERE ID IN ({placeholders})", message_ids)
        self._conn.commit()

    def close(self):
        self._conn.close()