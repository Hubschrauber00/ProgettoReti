#include "PacketManager.h"

extern const char TEXT[];
extern const char BINARY[];
extern const char errFileMsg[];
extern const char errReq[];

void* serializePacket( struct Packet pkt, const char* mode){
    void* buf;
    int pos = 0;
    uint16_t temp = htons( pkt.opcode );
    //serializzazione del pacchetto di tipo pkt.opcode
    switch (pkt.opcode){
        case RRQ:
        {
            //pacchetto di richiesta

            //serializza il codice operativo
            buf = malloc( sizeof(pkt.opcode) + strlen(pkt.rrq.fileName)+1 + strlen(pkt.rrq.mode)+1 );
            memcpy( buf + pos , &temp, sizeof(temp) );
            pos += sizeof(temp);
            
            //serializza il filename
            strcpy( buf + pos, pkt.rrq.fileName );
            pos += strlen(pkt.rrq.fileName)+1;
           
            //serializza la modalità
            strcpy( buf + pos, pkt.rrq.mode );
            pos += strlen(pkt.rrq.mode) +1;

            break;
        };
        case DATA:
        {
            //pacchetto di tipo dati
            buf = malloc ( pkt.data.nread + sizeof(pkt.opcode) + sizeof(pkt.data.blockNum) );
            //serializza il codice operativo
            memcpy( buf, &temp, sizeof(temp) );
            pos += sizeof(temp);
            //serializza il numero di blocco
            temp = htons( pkt.data.blockNum );
            memcpy( buf + pos, &temp, sizeof(temp) );
            pos += sizeof(temp);

            if( strcmp( mode, TEXT ) == 0 ){
                //modalità stringa
                strcpy((char*)buf + pos, pkt.data.textBuf);
                pos += strlen( pkt.data.textBuf ) +1;
            }
            else{
                //modalità binaria
                memcpy( buf + pos, pkt.data.binaryBuf, pkt.data.nread);
                pos += pkt.data.nread;
            }  
            break;
        };
        case ACK:
        {
            //pacchetto acknowledgement
            //serializza il codice operativo
            buf = malloc( sizeof(pkt.opcode) + sizeof(pkt.ack.blockNum) );
            memcpy(buf, &temp, sizeof(temp));
            pos += sizeof(temp);
            //serializza il numero di blocco
            temp = htons(pkt.ack.blockNum);
            memcpy(buf + pos, &temp, sizeof(temp));
            pos += sizeof(temp);
            break;
        };
        case ERROR:
        {
            //pacchetto di errore
            buf = malloc( sizeof(pkt.opcode) + sizeof(pkt.error.errNo) + strlen(pkt.error.errMsg) + 1 );
            //serializza il codice operativo 
            memcpy( buf + pos, &temp, sizeof(temp) );
            pos += sizeof(temp);
            //serializza il codice di errore
            temp = htons( pkt.error.errNo );
            memcpy( buf + pos, &temp, sizeof(temp) );
            pos += sizeof(temp);
            //serializza il messaggio di errore
            strcpy( buf + pos, pkt.error.errMsg);
            pos += strlen( pkt.error.errMsg ) + 1;
            break;
        };
        default:
            buf = NULL;
  
    }
    return buf;

}

int deserializePacket( void *buf, struct Packet *pkt, int size, char* mode){
    uint16_t temp;
    int pos = 0;
    //controlla che il buffer non sia vuoto
    if(buf == NULL)
        return 0;
    //alloca il codice operativo
    memcpy( &temp, buf, sizeof(temp));
    pos += sizeof(temp);
    pkt->opcode = ntohs(temp);
    switch(pkt->opcode){
        case RRQ:
        {
            //pacchetto di richiesta
            //alloca il filename
            strcpy( pkt->rrq.fileName, buf + pos );
            pos += strlen(pkt->rrq.fileName) + 1;
            //alloca la modalità
            strcpy( pkt->rrq.mode, buf + pos );
            pos += strlen(pkt->rrq.mode) +1;
            break;
        };
        case DATA:
        {
            //pacchetto dati
            //alloca il numero di blocco
            memcpy( &temp, buf + pos, sizeof(temp) );
            pos += sizeof(temp);
            pkt->data.blockNum = ntohs(temp);
            //calcola la dimensione del campo dati
            pkt->data.nread = (size - sizeof(temp)) - sizeof(temp);
            if(strcmp(mode, TEXT) == 0){
                //modalità testo
                strcpy( pkt->data.textBuf, buf + pos);
                pos += strlen(pkt->data.textBuf) + 1;
            }
            else{
                //modalità binaria
                memcpy( pkt->data.binaryBuf, buf + pos, pkt->data.nread );
                pos += pkt->data.nread ;
            }
            break;
        };
        case ACK:
        {
            //pacchetto di acknowledgment
            //alloca il numero di blocco
            memcpy( &temp, buf + pos, sizeof(temp) );
            pos += sizeof(temp);
            pkt->ack.blockNum = ntohs(temp);
            break;
        };
        case ERROR:
        {
            //pacchetto di errore
            //alloca il codice di errore
            memcpy( &temp, buf + pos, sizeof(temp) );
            pos += sizeof(temp);
            pkt->error.errNo = ntohs(temp);
            //alloca il messaggio di errore
            strcpy( pkt->error.errMsg, buf + pos);
            pos += strlen(pkt->error.errMsg) + 1;
        };
        default:
            return 0;       
    }
    return pos;
}


ssize_t mkErrPkt(struct Packet* pkt, uint16_t errNo, const char* errMsg){
    pkt->opcode = ERROR;
    printf( (errNo == FILENOTFOUND) ? "file non trovato\n" : "codice richiesta errato\n" );
    strcpy( pkt->error.errMsg, errMsg );
    pkt->error.errNo = errNo;
    return sizeof(pkt->opcode) + sizeof(pkt->error.errNo) + strlen(pkt->error.errMsg) + 1;
}