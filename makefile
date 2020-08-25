#makefile

CFLAGS = -Wall

all: tftp_client tftp_server

tftp_server: Main.o ConnectionManagement.o PacketManager.o shared/libs.h shared/constants.h
	$(CC) $(CFLAGS) -g Main.o ConnectionManagement.o PacketManager.o -o tftp_server -lpthread

tftp_client: client.o PacketManager.o shared/libs.h shared/constants.h
	$(CC) $(CFLAGS) -g client.o PacketManager.o -o tftp_client

PacketManager.o: shared/PacketManager.c shared/PacketManager.h shared/libs.h
	$(CC) $(CFLAGS) -g -c shared/PacketManager.c

ConnectionManagement.o: shared/ConnectionManagement.c shared/ConnectionManagement.h shared/libs.h
	$(CC) $(CFLAGS) -g -c shared/ConnectionManagement.c 

Main.o: Server/Main.c shared/libs.h shared/PacketManager.h shared/constants.h Server/Request.h shared/ConnectionManagement.h
	$(CC) $(CFLAGS) -g -c Server/Main.c -pthread

client.o: Client/client.c shared/libs.h shared/PacketManager.h shared/constants.h
	$(CC) $(CFLAGS) -g -c Client/client.c

clean:
	rm *.o tftp_server tftp_client