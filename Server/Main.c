#include "../shared/libs.h"
#include "Request.h"
#include "../shared/PacketManager.h"
#include "../shared/ConnectionManagement.h"
#include "../shared/constants.h"

extern const char TEXT[];
extern const char BINARY[];
extern const char errFileMsg[];
extern const char errReq[];

//gestisce la nuova richiesta
void *threadHandler( void* req ){
    if(req == NULL){
        printf("errore richiesta nulla\n");
        pthread_exit(NULL);
    }
    printf("gestione richiesta\n");
    struct Request *request = (struct Request*)req;
    struct Packet reqPkt;
    FILE* fd;
    uint16_t blockNumber = 0;
    void* sndBuf;
    unsigned char rcvBuf[32];
    ssize_t nread;
    int result = 0;

    int toEnd = 0;

    int childSock = socket(AF_INET, SOCK_DGRAM, 0);
    if( childSock < 0 ){
        perror("Errore: ");
        free(request);
        pthread_exit( NULL );
    }
    if( deserializePacket( request->buf, &reqPkt, 0, NULL) > 0 && reqPkt.opcode == RRQ ){
    // richiesta download
      
        char path[MAXLEN];
        //estrazione fileName
   
        strcpy( path, request->dir );
        strcat( path, reqPkt.rrq.fileName );
        //apertura file
        fd = fopen(path, ( strcmp( reqPkt.rrq.mode, TEXT ) == 0 )?"r":"rb" );
                   
        if( fd > 0 ){
            //gestione dei pacchetti
            printf("trasferimento file in corso...\n");
            struct Packet dataPkt;
            struct Packet ackPkt;

            while(!toEnd){
                char c;
                nread = 0;                    
                dataPkt.data.blockNum = blockNumber;
                dataPkt.opcode = DATA;
                blockNumber++;
                if( strcmp( reqPkt.rrq.mode, TEXT ) == 0 ){
                    // leggo il file modalità testo
                    while(nread < MAXDATALEN -1 && (c = fgetc(fd)) != EOF ){
                        dataPkt.data.textBuf[nread] = c;
                        nread++;
                    }
                    dataPkt.data.textBuf[nread] = '\0';
                    nread++;
                }
                else {
                    //leggo il file in modalità binaria
                    nread = fread( dataPkt.data.binaryBuf, 1, MAXDATALEN, fd);
                }
                dataPkt.data.nread = nread;
                //se ho letto meno di 512 byte chiudo il trasferimento
                if( nread < MAXDATALEN )           
                    toEnd = 1;
                //serializzo il pacchetto
                sndBuf = serializePacket( dataPkt, reqPkt.rrq.mode );
                if(sndBuf == NULL)
                {
                    printf("errore serializzazione file\n");
                    break;
                }
                //invio del messaggio
                result = sendto( childSock, sndBuf, nread + 4, 0, (struct sockaddr*)&request->from, request->fromlen );
                free( sndBuf );
                if( result < 0 ){
                    perror( "Errore: " );
                    break;
                }
                //ricevo l'acknowledgement
                result = recvfrom( childSock, rcvBuf, sizeof(ackPkt), 0, (struct sockaddr*)&(request->from), &(request->fromlen) );
                if( result < 0 ){
                    perror( "Errore: " );
                    break;
                }
                result = deserializePacket( rcvBuf, &ackPkt, 0, NULL );
                if( result == 0 || ackPkt.opcode != ACK || ackPkt.ack.blockNum != dataPkt.data.blockNum ){
                    printf( "ack non valido\n" );
                    break;
                }
            }
            printf( "trasferimento completato, inviati %d pacchetti\n", blockNumber );
            fseek(fd, 0, SEEK_SET);
            printf("chiudo il file\n");
            fclose(fd);
        }
        else{
            // file non trovato
            struct Packet errPkt;
            ssize_t pktSize = 0;
            pktSize = mkErrPkt( &errPkt, FILENOTFOUND, errFileMsg );
            sndBuf = serializePacket(errPkt, NULL);
            result = sendto( childSock, sndBuf, pktSize, 0, (struct sockaddr*)&request->from, request->fromlen );
            free( sndBuf );
            if(result < 0){
                perror("Errore: ");
            }
        }
               
    }
    else{
        // richiesta errata
        ssize_t pktSize = 0;
        struct Packet errPkt;
        pktSize = mkErrPkt( &errPkt, ILLEGALOPERATION, errFileMsg );
        sndBuf = serializePacket(errPkt, NULL);
        result = sendto( childSock, sndBuf, pktSize, 0, (struct sockaddr*)&request->from, request->fromlen );
        free( sndBuf );
         if(result < 0){
                perror("Errore:");
        }
    }

    free( req );
    close( childSock );
    printf("fine gestione richiesta\n\n\n");
    pthread_exit( NULL );
}

int main( int argc, char** argv ){
    int listeningSocket;
    int port;
    int len; 
    pthread_t threadId;
    struct Request* newreq;
    char *dir;
    
    //legge numero di porta e directory da argomento

    if( argc < 3 ){
        printf("errore, argomenti non validi\n");
        exit(1);
    }
    if(!sscanf( argv[1], "%d", &port )){
        printf("numero di porta non valido\n");
        exit(1);
    }
   
    printf("porta scelta %d \n", port);

    dir = malloc( MAXLEN );
    memset( dir, 0, MAXLEN);
    strcpy( dir, argv[2] );
    dir[strlen(argv[2])] = '\0';
    
    fflush(stdout);
    printf("dir %s \n", dir);

    //apre il socket
    listeningSocket = createAndBindUDPSocket(port);
    if( listeningSocket < 0 ){
        perror("Errore: ");
        exit(1);
    }
   
    //inizia ad ascoltare sulla porta specificata
    while(1){
        printf("processo in ascolto\n");
        //riceve richieste di connessione e le gestisce in un apposito thread
        newreq =  malloc( sizeof(struct Request) );
        memset( newreq, 0, sizeof(struct Request) );
        memset( newreq->buf, 0, MAXBUFLEN );
        newreq->dir = dir;
        newreq->fromlen = sizeof(struct sockaddr_in);
        len = recvfrom( listeningSocket, (void*)newreq->buf, MAXBUFLEN, 0, (struct sockaddr*)&(newreq->from), &(newreq->fromlen) );
        printf("richiesta ricevuta\n");
        printf("%c", '\n');
        if( len > 0 ){
           pthread_create( &threadId, NULL, threadHandler, (void*)newreq );
        }
    }
    exit(0);
}