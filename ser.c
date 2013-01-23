#include "sys/socket.h"
#include "stdio.h"
#include "netinet/in.h"
#include "errno.h"
#include "time.h"


#define SERV_IP         "127.0.0.1"
#define SERV_PORT       4798
#define RETURN_OK       0
#define RETURN_ERR      -1
#define BUFFER_SIZE     1024
#define INVALID_SOCKET  -1

static int serv_main_socket = INVALID_SOCKET;;
static int serv_socket = INVALID_SOCKET;


/************************************************
*   服务端为单线程  : 
*
************************************************/


int Socket_Close(int* sockfd)
{
	if (*sockfd != INVALID_SOCKET)
		close(*sockfd);

	*sockfd = INVALID_SOCKET;

	return RETURN_OK;
}


int Serv_Socket_Init()
{
	int ret = -1;
	struct sockaddr_in serv_addr;

	serv_main_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(serv_main_socket == INVALID_SOCKET)
	{
		perror("Failed when socket in serv_sock_init : \n");
		goto ERR;
	}

	serv_addr.sin_port = htons(SERV_PORT);
#if 1
    serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(SERV_IP);
	if(INADDR_NONE == serv_addr.sin_addr.s_addr )
	{
		perror("Failed when inet_addr in serv_sock_init.\r\n");
		goto ERR;
	}
#else
	memset(&serv_addr, 0, sizeof(serv_addr));
	ret = inet_pton(AF_INET, SERV_ADDR, &serv_addr.sin_addr.s_addr);
	if (ret < 0)
	{
		perror("Failed when inet_addr in serv_sock_init.\r\n");
		goto ERR;
	}
#endif
	ret = bind(serv_main_socket, &serv_addr, sizeof(serv_addr));
	if(-1 == ret)
	{
		perror("Failed when bind in serv_sock_inti.\r\n");
		goto ERR;
	}

	ret = listen(serv_main_socket, 5);
	if (-1 == ret)
	{
		perror("Failed when listen in serv_sock_init.\r\n");
		goto ERR;
	}

	printf("Success in Serv_Init,service socket is %d.\r\n",serv_main_socket);
	return serv_main_socket;

ERR:
	if( INVALID_SOCKET != serv_main_socket)
	{
		Socket_Close(&serv_main_socket);
	}

	return RETURN_ERR;
}

void Serv_Sock_Reinit()
{
	struct sockaddr_in serv_addr;

	//if(s_sock.tcpserver_address.sin_addr.s_addr!=inet_addr("127.0.0.1")){
    if(serv_main_socket == INVALID_SOCKET)
	{
        serv_main_socket = socket(AF_INET, SOCK_STREAM, 0); 
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		serv_addr.sin_port = htons(4798);
		bind(serv_main_socket, (struct sockaddr *) &serv_addr,sizeof(serv_addr));
		listen(serv_main_socket, 5);
    }
}

int Serv_Sock_Accept()
{
 	struct sockaddr_in address;
	socklen_t socklen=sizeof (struct sockaddr);

	serv_socket = accept(serv_main_socket,&address, &socklen);	
	if (INVALID_SOCKET == serv_socket)
	{
		perror("Failed when accept in Serv_Sock_Accept :");
        return RETURN_ERR;
	}

    return RETURN_OK;
}



int Serv_Sock_Recv(char* buffer, int buffer_len)
{
    int ret = -1;
	int total_len = 0;
	// when send buffers will send the length of buffer.so will receive sizeof(int) bytes.
	int expect_len = sizeof(int);	
	int total_len_geted = 0;

	unsigned char* buffer_pos = buffer;
    memset(buffer, 0, buffer_len);

	while(1)
	{
		ret = recv(serv_socket, buffer+total_len,expect_len, 0);
		if ( 0 == ret || -1 == ret )
		{
			if (ret == 0)
				printf("failed when recv in Serv_Sock_Recv: ret = 0.\r\n");
			else
				printf("Failed when recv in Serv_Sock_Recv: socket err.\r\n");

			return RETURN_ERR;
		}

		total_len += ret;
		expect_len -= ret;

		if (expect_len < 0)
			expect_len = 0;

		if(total_len>=sizeof(int) && 0==total_len_geted)
		{
			printf("receive %d bytes.\r\n",*(int*)buffer_pos);
			total_len_geted = 1;
		}


		if (expect_len == 0 && total_len_geted == 1)
		{
			printf("Get data \n");
			return RETURN_OK;
		}

	}
}


int Ser_proc_buffer(char* buffer, int buffer_size)
{
    char content[100] = {0};
    time_t  tp;
    time(&tp);

    sprintf(content, "recv %d date %s.",*((int*)buffer),ctime(&tp));
    printf("in Service==> get date %s",buffer+sizeof(int));
    printf("in Service==> wait 1s");
    sleep(1);

    Serv_Send(content,strlen(content)+1);

    return RETURN_OK;
}



int Serv_Thread_Fun()
{
    int     result = 0;
    fd_set  set_fd;
    char    buffer[BUFFER_SIZE] = {0};
    struct timeval timeout = {3,0};
    
    while(1)
    {
        if (serv_main_socket == INVALID_SOCKET)
            if(RETURN_ERR == Serv_Socket_Init())
                goto END_LOOP;
printf("Service main socket is %d.\n",serv_main_socket);
    	FD_ZERO(&set_fd);
    	FD_SET(serv_main_socket, &set_fd);

        result = select(serv_main_socket+1, &set_fd, (fd_set *) 0, (fd_set *) 0, &timeout);
        if ( result < 0 )
            goto END_LOOP;

        else if ( result == 0 )
            goto END_LOOP;

        else
        {
            if (serv_main_socket >= 0)
                if(FD_ISSET(serv_main_socket,&set_fd))
                {
                    result = Serv_Sock_Accept();
                    if(result == RETURN_ERR)
                        goto END_LOOP;
printf("Service socket is %d.\n",serv_socket);                    
                    Serv_Send("i have send data.",18);
                    while(1)
                    {
                        if ( RETURN_ERR == Serv_Sock_Recv(buffer, BUFFER_SIZE))
                            goto END_LOOP;

                        Ser_proc_buffer(buffer,BUFFER_SIZE);
                        
                    }
                }
        }
END_LOOP:
printf("Sevice endloop wait 10s...\n");
sleep(10);
        
    }
    Socket_Destory();
}


int Socket_Destory()
{
	Socket_Close(&serv_socket);
	Socket_Close(&serv_main_socket);

	return RETURN_OK;
}

int Send_Date(int sockfd, char* buffer, int length)
{
	int ret = -1;

	if (INVALID_SOCKET == sockfd)
		return RETURN_ERR;

	ret = send(sockfd, &length, sizeof(int), 0);
	if (INVALID_SOCKET == ret)
		return RETURN_ERR;

	ret = send(sockfd, buffer, length, 0);
	if (INVALID_SOCKET == ret)
		return RETURN_ERR;

	return RETURN_OK;
}

int Serv_Send(char* buffer, int length)
{
	int ret = -1;
	ret = Send_Date(serv_socket, buffer, length);
	if (RETURN_OK != ret)
	{
		printf("Failed when Send Data.\r\n");
		if (INVALID_SOCKET != serv_socket)
		{
			printf("Failed when Send Data: socket error.\r\n");
			Socket_Close(&serv_socket);
		}

		return RETURN_ERR;
	}

	return RETURN_OK;
}


int main()
{
	int ret ;
    Serv_Socket_Init();
	Serv_Thread_Fun();
    
	return 1;
}

