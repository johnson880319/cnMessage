SOURCE_DIR = ./src/
DEP = $(SOURCE_DIR)common.h

all: client server

client: $(SOURCE_DIR)client.c $(SOURCE_DIR)client.h $(DEP)
	gcc $(SOURCE_DIR)client.c -o client -lcdk -pthread -lmenu -lncurses

server: $(SOURCE_DIR)server.c $(SOURCE_DIR)server.h $(DEP)
	gcc $(SOURCE_DIR)server.c -o server -lsqlite3 -lssl -lcrypto -pthread
