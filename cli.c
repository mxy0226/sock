#include "sock.h"


static int cli_socket = INVALID_SOCKET;

int Cli_Socket_Init() {

    int result = 0;
    struct sockaddr_in 	cli_Addr;

    if (cli_socket != INVALID_SOCKET) {
        int len = sizeof(cli_Addr);
        if(!getpeername(cli_socket, (struct sockaddr *)&cli_Addr, &len)) {
            if (len == sizeof(cli_Addr) && strcmp(CLI_IP, inet_ntoa(cli_Addr.sin_addr))==0)
                return RETURN_OK;
        }
        perror("client get ipaddr error ");
        Socket_Close(&cli_socket);
    }
        
    
    cli_socket = socket(AF_INET, SOCK_STREAM, 0);
    if( INVALID_SOCKET == cli_socket) {
        perror("client socket error ");
        goto ERR;
    }

    cli_Addr.sin_family = AF_INET;
    cli_Addr.sin_port = htons(CLI_PORT);
    cli_Addr.sin_addr.s_addr = inet_addr(CLI_IP);
    if (INADDR_NONE == cli_Addr.sin_addr.s_addr) {
        perror("client inet_addr error ");
        goto ERR;
    }

    result = connect ( cli_socket, (struct sockaddr * ) & cli_Addr, sizeof (cli_Addr) );
    if ( result == -1 ) {
        perror("connect error :");
        goto ERR;
    }

    return RETURN_OK;

ERR:
    if(INVALID_SOCKET != cli_socket)
        Socket_Close(&cli_socket);

    return RETURN_ERR;
}

int Send_Date(int sockfd, char* buffer, int length) {
    
	int ret = -1;

	if (INVALID_SOCKET == sockfd) {
        perror("socket err");
		return RETURN_ERR;
    }

	ret = send(sockfd, &length, sizeof(int), 0);
	if (INVALID_SOCKET == ret) {
	    perror("socket send err");
        return RETURN_ERR;
    }
	ret = send(sockfd, buffer, length, 0);
	if (INVALID_SOCKET == ret) {
	    perror("socket send1 err");
        return RETURN_ERR;
    }

    printf("client send date ...\n");
	return RETURN_OK;
}

int Cli_Send(char* buffer, int length) {

	int ret = -1;
	ret = Send_Date(cli_socket, buffer, length);
	if (RETURN_OK != ret) {
		printf("Failed when Send Data.\r\n");
        
    	if (INVALID_SOCKET != cli_socket) {
    		printf("Failed when Send Data: socket error.\r\n");
    		Socket_Close(&cli_socket);
    	}

		return RETURN_ERR;
	}
	return RETURN_OK;

}

int Cli_proc_buffer(char* buffer, int buffer_size) {

    char content[100] = {0};
    time_t  tp;
    time(&tp);

    sprintf(content, "recv %d date %s.",*((int*)buffer),ctime(&tp));
    Cli_Send(content,strlen(content)+1);
    sleep(1);

    return RETURN_OK;
}

int Cli_Thread_Fun() {

    int     result = 0;
    char    buffer[BUFFER_SIZE] = {0};
    fd_set  set_fd;
    struct  timeval timeout = {10,0};

    while(1) {
        if (cli_socket == INVALID_SOCKET)
            if(RETURN_ERR==Cli_Socket_Init()) {
                printf("Client init error\n");
                goto END_LOOP;
            }
            
        FD_ZERO(&set_fd);
        FD_SET(cli_socket, &set_fd);

        printf("Client select, socket is %d.\n", cli_socket);
        result = select(cli_socket+1, &set_fd, (fd_set *) 0, (fd_set *) 0, &timeout);

        if ( result < 0 ) {
            Socket_Close(&cli_socket);
            goto END_LOOP;
        }

        else if ( result == 0 ) {
            printf("client wait timeout...\n");
            goto END_LOOP;
        }
        
        //receive date form service 
        if (FD_ISSET(cli_socket,&set_fd)) {
            
            Cli_Sock_Recv(buffer, BUFFER_SIZE);
            printf("Client receive date: {%s}\n", buffer+4);
            Cli_proc_buffer(buffer,BUFFER_SIZE);

            continue;
        }

END_LOOP:
        printf("client endloop wait 10s...\n");
        sleep(10);
    }
    
    Socket_Destory();
}

int Cli_Sock_Recv(char* buffer, int buffer_len) {
    
	// when send buffers will send the length of buffer.so will receive sizeof(int) bytes.
	int             expect_len = sizeof(int);	
    int             ret = -1;
	int             total_len = 0;
	int             total_len_geted = 0;
	unsigned char*  buffer_pos = buffer;

    if (cli_socket == INVALID_SOCKET)
        return RETURN_ERR;
    
    memset(buffer, 0, buffer_len);
	while(1) {
		ret = recv(cli_socket, buffer+total_len,expect_len, 0);
		if ( 0 == ret || -1 == ret ) {
			if (ret == 0)
				printf("failed when recv in cli_Sock_Recv: ret = 0.\r\n");
			else
				printf("Failed when recv in cli_Sock_Recv: socket err.\r\n");

			return RETURN_ERR;
		}

		total_len += ret;
		expect_len -= ret;

		if (expect_len < 0)
			expect_len = 0;

		if(total_len>=sizeof(int) && 0==total_len_geted) {

            total_len_geted = 1;
            expect_len = *(int*)buffer_pos;
		}

		if (expect_len == 0 && total_len_geted == 1)
            return RETURN_OK;

    }
    
}

int Socket_Close(int* sockfd) {

	if (*sockfd != INVALID_SOCKET) {
        close(*sockfd);
        printf("client close socket %d\n", *sockfd);
    }

    *sockfd = INVALID_SOCKET;

    return RETURN_OK;
}


int Socket_Destory() {
    
    Socket_Close(&cli_socket);
    return RETURN_OK;
}



int main() {
    int ret ;
    Cli_Socket_Init();
    Cli_Thread_Fun();
    
	return 1;
}

