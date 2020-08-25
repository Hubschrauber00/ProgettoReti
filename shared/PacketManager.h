#include "./libs.h"

struct r_rrq{
    char fileName[MAXBUFLEN];
    char mode[20];
};

struct r_data{
    int nread;
    uint16_t blockNum;
    union{
        char textBuf[MAXDATALEN];
        unsigned char binaryBuf[MAXDATALEN];
    };
    
};
struct r_ack{
    uint16_t blockNum;
};
struct  r_error{
    uint16_t errNo;
    char errMsg[MAXBUFLEN];
};
struct Packet{

    uint16_t opcode;
    union{
        struct r_rrq rrq;
        struct r_data data;
        struct r_ack ack;
        struct r_error error;  
    };

};

void* serializePacket( struct Packet, const char*);

int deserializePacket( void*, struct Packet*, int, char*);

ssize_t mkErrPkt(struct Packet*, uint16_t, const char*);
