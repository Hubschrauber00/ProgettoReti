#ifndef LIBS_H
#define LIBS_H
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>

#define MAXLEN 4096
#define MAXBUFLEN 1024
#define MAXDATALEN 512
#define ACKLEN 4

#define FILENOTFOUND 1
#define ILLEGALOPERATION 4

#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERROR 5

#endif