# Protocoale de comunicatii:
# Laborator 8: Multiplexare
# Makefile

CFLAGS = g++ -g -Wall

# Portul pe care asculta serverul (de completat)
PORT = 

# Adresa IP a serverului (de completat)
IP_SERVER = 

all: server subscriber

# Compileaza server.c
server: server.cpp

# Compileaza client.c
subscriber: subscriber.cpp

.PHONY: clean run_server run_client

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza clientul
run_subscriber:
	./subscriber ${ID_CLIENT} ${IP_SERVER} ${PORT_SERVER}

clean:
	rm -f server subscriber
