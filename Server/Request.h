#include "../shared/libs.h"

struct Request{
    struct sockaddr_in from;
    socklen_t fromlen;
    char buf[MAXBUFLEN];
    char* dir;
};

