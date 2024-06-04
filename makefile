CC = gcc
SERVER = ftp_server
CLIENT = ftp_tester_client
SERVER_STORAGE = server_storage

SERVER_SRC = ftp_server.c
CLIENT_SRC = ftp_tester_client.c

# Цели
all: $(SERVER) $(CLIENT) $(SERVER_STORAGE)

$(SERVER): $(SERVER_SRC)
	$(CC) -o $(SERVER) $(SERVER_SRC)

$(CLIENT): $(CLIENT_SRC)
	$(CC) -o $(CLIENT) $(CLIENT_SRC)

$(SERVER_STORAGE):
	mkdir -p $(SERVER_STORAGE)

clean:
	rm -f $(SERVER) $(CLIENT)

.PHONY: all clean