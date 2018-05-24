#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>

#define CONN_MAX 3
#define IP_ADDR "0.0.0.0"
#define PORT "3010"
#define BYTES 1024

int listenfd;
int clients[CONN_MAX];
char *ROOT;

void error(char *);
void respond(int);
void serv_start(char *);

int main(int argc, char **argv){

    /* Socket file descriptor, Connection file descriptor
     * On success, a file descriptor for the new socket is returned.
     * On error, -1 is returned, and errno is set appropriately.
     * See http://man7.org/linux/man-pages/man2/socket.2.html
     * */
    int sock_fd, connection_fd;
    int slot = 0;
    ROOT = getenv("PWD");
    /* See: 
     * http://man7.org/linux/man-pages/man7/ip.7.html
     * Address format
     */

    struct sockaddr_in client_addr;
    socklen_t addrlen;
    memset(&client_addr, '0', sizeof(client_addr));
    /* gets current path as server root */
    
    for(int i = 0; i <CONN_MAX; i++){
        clients[i]--;
    }
    serv_start(PORT);
        
    for(;;){

        addrlen = sizeof(client_addr);
        clients[slot] = accept(listenfd, (struct sockaddr*) &client_addr, &addrlen);

        if(clients[slot] < 0)
        {
            perror("accept error");
        }else{

            if(fork() == 0)
            {
                respond(slot);
                exit(0);
            }

        }
    
        while(clients[slot] != -1){
            slot = (slot+1)%CONN_MAX;
        }



    }
    return 0;

}


void serv_start(char *port){
    
    struct addrinfo hints, *res, *p;

    memset(&hints, 0, sizeof(hints));
    
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

   if(getaddrinfo(NULL, port, &hints, &res) != 0)
   {
       perror("getaddrinfo error\n");
       exit(1);
   }

   for(p = res; p!=NULL; p=p->ai_next)
   {
        listenfd = socket(p->ai_family, p->ai_socktype, 0);

        if(listenfd == -1)
        {
            continue;
        }

        if(bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
        {
            break;
        }
   }
    if(p == NULL)
    {
        perror("socket or bind error\n");
        exit(1);
    }

    freeaddrinfo(res);

    if(listen(listenfd,3) != 0)
    {
        perror("listen error\n");
        exit(1);
    }

}

void respond(int n)
{

    char mesg[99999];
    char *reqline[3];
    char data_to_send[BYTES];
    char path[99999];
    
    int rcvd, fd, bytes_read;

    memset((void *)mesg, (int)'\0',99999);
    
    rcvd = recv(clients[n], mesg, 99999, 0);

    if (rcvd < 0)
    {
        fprintf(stderr, "recv error\n");
    }else if(rcvd == 0){
        fprintf(stderr,"Unexpected client disconnection\n");
    }else
    {

        printf("%s",mesg);
        reqline[0] = strtok(mesg," \t\n");
        if(strncmp(reqline[0], "GET\0", 4)==0)
        {
            reqline[1] = strtok(NULL, " \t");
            reqline[2] = strtok(NULL, " \t\n");

            if(strncmp( reqline[2], "HTTP/1.0", 8) !=0 && strncmp( reqline[2], "HTTP/1.1", 8)!=0)
            {
                write(clients[n], "HTTP/1.0 400 Bad Request\n", 25); 
            
            }
            else
            {
                if(strncmp(reqline[1], "/\0",2)==0) 
                {
                    reqline[1] = "/index.html";

                }
                strcpy(path, ROOT); 
                strcpy(&path[strlen(ROOT)], reqline[1]);            
                printf("FILE : %s\n", path);

                if((fd=open(path, O_RDONLY)) != -1)
                {
                    
                    send(clients[n], "HTTP/1.0 200 OK\n\n", 17,0);
                    while((bytes_read=read(fd, data_to_send, BYTES))>0)
                    {
                        write(clients[n], data_to_send, bytes_read);
                    }
                }else
                {
                    write(clients[n], "HTTP/1.0 404 Not Found\n", 23); 
                }
            }

            shutdown(clients[n], SHUT_RDWR);
            close(clients[n]);
            clients[n]=-1;

        }


























    }











}
