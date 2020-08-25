#include "../shared/libs.h"
#include "../shared/PacketManager.h"
#include "../shared/constants.h"

#define ADDRESSLEN 16

const char HELP[] = "!help";
const char MODE[] = "!mode";
const char GET[] = "!get";
const char QUIT[] = "!quit";
const char text[] = "text";
const char bin[] = "bin";

extern const char TEXT[];
extern const char BINARY[];
extern const char errFileMsg[];
extern const char errReq[];

void echoHelp(){
    printf("sono disponibili i seguenti comandi:\n");
    printf("!help --> mostra l'elenco dei comandi disponibili\n");
    printf("!mode {text|bin} --> imposta il modo di trasferimento dei files (testo o binario)\n");
    printf("!get filename nome_locale --> richiede al server il file dal nome <filename> e lo salva localmente con il nome <nome_locale>\n");
    printf("!quit --> termina il client\n\n");
}

int main(int argc, char** argv){
    int port;
    char svIp[ADDRESSLEN];
    char mode[5];
    
    char buf[MAXLEN];
    int sock;
    struct sockaddr_in serverAddr;
    int ret;
    
   
    
    if(argc <3){
        printf("numero argomenti insufficiente\n");
        exit(1);
    }
    if(!sscanf( argv[1], "%s", svIp)){
        printf("errore, non è stato inserito un indirizzo corretto\n");
        exit(1);
    }
    if(!sscanf( argv[2], "%d", &port)){
        printf("errore, non è stato inserito un numero di porta corretto\n");
        exit(1);
    }
    //indirizzo del server
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton( AF_INET, svIp, &serverAddr.sin_addr );

    //indirizzo del client
    //modalità default binaria
    strcpy( mode, bin );

    echoHelp();
    
    while(1){
        char localName[MAXBUFLEN];
        char temp[MAXBUFLEN];
        char temp2[MAXBUFLEN];
        char command[6];
        memset(localName, 0, MAXBUFLEN);
        
        printf("\n\ninserisci un comando:\n");
        fgets( buf, MAXLEN, stdin);
        ret = sscanf( buf, "%s %s %s", command, temp, temp2);

        if( strcmp(command, HELP) == 0 ){
            echoHelp();
        }
        else if( strcmp(command, MODE) == 0 ){
            if( ret < 2 || ( strcmp(temp, bin) != 0 && strcmp(temp, text) != 0 ) ){
                printf("selezionata modalità non valida\n");
            }
            else
            {
                strcpy(mode, temp);
                printf("selezionata modalità: %s\n", mode);
            }
        }
        else if( strcmp(command, GET) == 0 ){
            FILE* fd;
            int ackNo = 0;
            int toEnd = 0;
            int toDelete = 0;
            if( ret < 2 ){
                printf("specificare il nome di un file da scaricare\n");
                continue;
            }
            else if( ret < 3 ){
                printf("specificare il nome con cui deve essere salvato il file\n");
                continue;
            }
            strcpy( localName, temp2 );

            sock = socket(AF_INET, SOCK_DGRAM, 0);
            if( sock < 0 ){
                perror("Errore: ");
                exit(1);
            }
            //crea il pacchetto
            struct Packet reqPkt;
            reqPkt.opcode = RRQ;
            strcpy( reqPkt.rrq.mode, ( ( strcmp(mode, bin) == 0 ) ? BINARY : TEXT ) );
            printf("modalità %s\n", reqPkt.rrq.mode);

            strcpy( reqPkt.rrq.fileName, temp );
            printf("e' stato richiesto il file %s in modalita' %s e verra' salvato come %s\n", reqPkt.rrq.fileName, mode, localName);
            printf("invio richiesta in corso\n");
            //serializza il pacchetto e lo invia
            void* sndBuf = serializePacket( reqPkt, NULL );
            ret = sendto( sock, sndBuf, sizeof(reqPkt), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr) );
            free( sndBuf );
            if(ret < 0 ){
                perror("Errore: ");
                close(sock);
                break;
            }
            //apre il file in scrittura
            fd = fopen( localName, (strcmp(mode, text) == 0) ? "w" : "wb" );
            if(fd == NULL){
                printf("errore nell'apertura del file\n");
                close(sock);
                break;
            }
            printf("Trasferimento in corso...\n");
            while(!toEnd){
                //loop
                struct sockaddr_in newSvAddr;
                socklen_t newSvAddrSize = sizeof(newSvAddr);
                memset( (void*)&newSvAddr, 0, sizeof(newSvAddr) );
                void* recvBuf = malloc( MAXBUFLEN );
                struct Packet recvPkt;
                struct Packet sndPkt;

                //ricezione del pacchetto
                ret = recvfrom( sock, recvBuf, MAXBUFLEN, 0, (struct sockaddr*)&newSvAddr, &newSvAddrSize );
                if(ret <= 0 ){
                    perror("Errore: ");
                    break;
                }
                deserializePacket( recvBuf, &recvPkt, ret,  reqPkt.rrq.mode );
                free( recvBuf );
                //se il pacchetto ha codice errore chiude il ciclo e cancella il file aperto in scrittura
                if( recvPkt.opcode == ERROR ){
                    printf( "Errore %d : %s\n", recvPkt.error.errNo, recvPkt.error.errMsg );
                    toDelete = 1;
                    break;
                }
               
                if( recvPkt.data.nread < MAXDATALEN )
                    toEnd = 1;
                if( strcmp(mode, text) == 0 ){
                    //scrive in modalità testo
                    fputs( recvPkt.data.textBuf, fd);
                }
                else{
                    //scrive in modalità binaria
                    fwrite( recvPkt.data.binaryBuf, 1, recvPkt.data.nread, fd );
                }

                //invio ack
                sndPkt.opcode = ACK;
                sndPkt.ack.blockNum = recvPkt.data.blockNum;
                sndBuf = serializePacket( sndPkt, NULL );
                ret = sendto( sock, sndBuf, sizeof(sndPkt), 0, (struct sockaddr*)&newSvAddr, newSvAddrSize);
                if( ret <= 0 ){
                    perror("Errore: ");
                    break;
                }
                free( sndBuf ); 
                ackNo++;         
            }
            //chiude il socket e il file
            close( sock );
            fclose( fd );
            if( toDelete ){
                remove( localName );
                printf("file temporaneo rimosso\n");
            }
            if(!toDelete){
                printf("ricevuti %d pacchetti\n", ackNo);
                printf("file salvato con successo\n");
            }
                
        
        }
        else if( strcmp(command, QUIT) == 0 ){
            printf("il programma verra' chiuso\n");
            break;
        }
        else    
            perror("comando non riconosciuto, ritenta\n");
    }
    exit(0);
}