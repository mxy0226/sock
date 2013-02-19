#include "sys/socket.h"
#include "stdio.h"
#include "netinet/in.h"

#include "errno.h"
#include "time.h"
#include "assert.h"

#define CLI_IP          "127.0.0.1"
#define CLI_PORT        4798
#define SERV_IP         "127.0.0.1"
#define SERV_PORT       4798



#define RETURN_OK       0
#define RETURN_ERR      -1
#define BUFFER_SIZE     1024
#define INVALID_SOCKET  -1


