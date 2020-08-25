#include "libs.h"

int createAndBindUDPSocket(int port){
    int sock;
    struct sockaddr_in serverAddr;
    int ret;

    //crea il socket UDP
    sock = socket( AF_INET, SOCK_DGRAM, 0 );

    if( sock == -1 ){
        perror("Errore: ");
        return sock;
    }
    memset( &serverAddr, 0, sizeof(serverAddr) );
    serverAddr.sin_port= htons(port);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    //lega il socket alla porta di ascolto

    ret = bind( sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr) );
    if(ret == -1 ){
        perror( "Errore: " );
        return ret;
    }

    return sock;
}