#include "sock.h"


#define MAX_FD      10

static int serv_main_socket = INVALID_SOCKET;
static int con_fd[MAX_FD] = {INVALID_SOCKET};


int Socket_Close(int* sockfd){
    
    //if (*sockfd != INVALID_SOCKET)
    if (*sockfd <= 0) {
        close(*sockfd);
        printf("server close socket %d\n", *sockfd);
    }
    
    *sockfd = INVALID_SOCKET;
    return RETURN_OK;
}


int Serv_Socket_Init() {
    
    int ret = -1;
    struct sockaddr_in serv_addr;

    if (serv_main_socket != INVALID_SOCKET) {
        int len = sizeof(serv_addr);
        if(!getpeername(serv_main_socket, (struct sockaddr *)&serv_addr, &len)) {
            if (len == sizeof(serv_addr) && strcmp(CLI_IP, inet_ntoa(serv_addr.sin_addr))==0)
                return RETURN_OK;
        }
        perror("Service get ipaddr error ");
        Socket_Close(&serv_main_socket);
    }

    serv_main_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(serv_main_socket == INVALID_SOCKET) {
        perror("Failed when socket in serv_sock_init");
        goto ERR;
    }

    serv_addr.sin_port = htons(SERV_PORT);
#if 1
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERV_IP);
    if(INADDR_NONE == serv_addr.sin_addr.s_addr ) {
        perror("Failed when inet_addr in serv_sock_init");
        goto ERR;
    }
#else
    memset(&serv_addr, 0, sizeof(serv_addr));
    ret = inet_pton(AF_INET, SERV_ADDR, &serv_addr.sin_addr.s_addr);
    if (ret < 0) {
        perror("Failed when inet_addr in serv_sock_init.\r\n");
        goto ERR;
    }
#endif
    ret = bind(serv_main_socket, &serv_addr, sizeof(serv_addr));
    if(-1 == ret) {
        perror("Failed when bind in serv_sock_inti.\r\n");
        goto ERR;
    }

    ret = listen(serv_main_socket, 5);
    if (-1 == ret) {
        perror("Failed when listen in serv_sock_init.\r\n");
        goto ERR;
    }

    return serv_main_socket;

ERR:
    if( INVALID_SOCKET != serv_main_socket)
        Socket_Close(&serv_main_socket);


    return RETURN_ERR;
}


int Serv_Sock_Recv(int sockfd, char* buffer, int buffer_len) {
    
    // when send buffers will send the length of buffer.so will receive sizeof(int) bytes.
    int             expect_len = sizeof(int);   
    int             ret = -1;
    int             total_len = 0;
    int             total_len_geted = 0;
    unsigned char*  buffer_pos = buffer;
    
    memset(buffer, 0, buffer_len);
    while(1) {
        ret = recv(sockfd, buffer+total_len,expect_len, 0);
        if ( 0 == ret || -1 == ret ) {
            if (ret == 0)
                printf("failed when recv in Serv_Sock_Recv: ret = 0.\r\n");
            else
                printf("Failed when recv in Serv_Sock_Recv: socket err.\r\n");

            ret = 0;
            total_len = 0;
            expect_len = sizeof(int);
            total_len_geted = 0;

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


int Ser_proc_buffer(int sockfd, char* buffer, int buffer_size) {

    time_t  tp;
    char    content[100] = {0};
    
    time(&tp);
    sprintf(content, "recv %d date %s.",*((int*)buffer),ctime(&tp));

    Serv_Send(sockfd,content,strlen(content)+1);
    sleep(1);

    return RETURN_OK;
}



int Serv_Thread_Fun() {
    int     result = 0;
    int     index = 0;
    int     connect_amount = 0;
    char    buffer[BUFFER_SIZE] = {0};
    fd_set  set_fd;
    struct  timeval timeout = {15,0};

    while(1) {
        if (serv_main_socket == INVALID_SOCKET)
            if(RETURN_ERR == Serv_Socket_Init())
                return RETURN_ERR;
            
        int max_sock = serv_main_socket;
        printf("Service have init succeed, socket = %d\n", serv_main_socket);
        while(1) {

            FD_ZERO(&set_fd);
            FD_SET(serv_main_socket, &set_fd);
            for (index = 0; index < MAX_FD; index++) {  
                if (con_fd[index] > 2) {
                    FD_SET(con_fd[index], &set_fd);  
                }
            } 

            result = select(max_sock + 1, &set_fd, (fd_set *) 0, (fd_set *) 0, &timeout);
            if ( result < 0 ) {
                perror("Service select error");
                break;
            }
            else if ( result == 0 ) {
                printf("Service wait timeout , 10s later...\n");
                sleep(10);        
                continue;
            }

            // check if client have send date
            for (index = 0; index < connect_amount; index++) {  

                if (FD_ISSET(con_fd[index], &set_fd)) {  
                    if ( RETURN_ERR == Serv_Sock_Recv(con_fd[index], buffer, BUFFER_SIZE)) {
                        printf("Service recv error : client[%d] = %d close", index, con_fd[index]);  
                        FD_CLR(con_fd[index], &set_fd);  // delete fd
                        Socket_Close(&con_fd[index]);

                        while(index < connect_amount) {
                            assert(index+1<=connect_amount && connect_amount <=MAX_FD);
                            con_fd[index] = con_fd[index+1];
                            index++;
                        }
                        connect_amount--;
                    } else {
                        printf("Service receive date: {%s}\n" ,buffer+4);
                        Ser_proc_buffer(con_fd[index],buffer,BUFFER_SIZE);
                    }
                }  
            }  

            // check whether a new connection comes  
            if (FD_ISSET(serv_main_socket, &set_fd)) {  
                struct sockaddr_in  client_addr;
                socklen_t           sin_size = sizeof(client_addr);

                int new_fd = accept(serv_main_socket, (struct sockaddr *)&client_addr, &sin_size);  
                if (new_fd <= 0) {
                    perror("Service accept error "); 
                    continue;  
                }

                // add to fd queue  
                if (connect_amount < MAX_FD) {  
                    con_fd[connect_amount++] = new_fd;  
                    sprintf(buffer, "new connection client[%d] %s\n", connect_amount,inet_ntoa(client_addr.sin_addr));
                    printf("Service connet a new client: clisocket %d\n", con_fd[connect_amount-1]);

                    Serv_Send(new_fd,buffer,strlen(buffer)+1);

                    if (new_fd > max_sock) 
                        max_sock = new_fd;  
                } else {
                    printf("max connections arrive, close %d exit/n",new_fd);
                    Serv_Send(new_fd, "bye", 4); 
                    Socket_Close(&new_fd);

                    continue; 
                }  
            }  
            
        }

        perror("Service error reinit...");
        sleep(10);
    }

    Socket_Destory();
}


int Socket_Destory()
{
    int index  = 0;
    for (index=0; index<MAX_FD; index++)
        Socket_Close(&con_fd[index]);
        
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

int Serv_Send(int sockfd, char* buffer, int length)
{
    int ret = -1;

    if (sockfd == INVALID_SOCKET)
        return RETURN_ERR;
    
    ret = Send_Date(sockfd, buffer, length);
    if (RETURN_OK != ret)
    {
        printf("Failed when Send Data.\r\n");
        if (INVALID_SOCKET != sockfd)
        {
            printf("Failed when Send Data: socket error.\r\n");
            Socket_Close(&sockfd);
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


#if 0

void Serv_Sock_Reinit()
{
    struct sockaddr_in serv_addr;
    int len = sizeof(serv_addr);

    if (serv_main_socket != INVALID_SOCKET)
    { 
        if(!getpeername(serv_main_socket, (struct sockaddr *)&serv_addr, &len))
        {
            if (len == sizeof(serv_addr) && strcmp(SERV_IP, inet_ntoa(serv_addr.sin_addr))==0)
                return;
        }
        perror("get ipaddr error ");
    }

    serv_main_socket = socket(AF_INET, SOCK_STREAM, 0); 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERV_IP);
    serv_addr.sin_port = htons(4798);
    bind(serv_main_socket, (struct sockaddr *) &serv_addr,sizeof(serv_addr));
    listen(serv_main_socket, 5);

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
#endif

