#include<stdio.h>
#include<stdbool.h>

#include<string.h>

#include<netinet/in.h>
#include<sys/socket.h>
#include<unistd.h>

#define MYERROR(condition,msg,errorValue); if((condition)){printf("%s\n",(msg));return (errorValue);}

int main(void) {
    int socketFd, inSocketFd;
    char buffer[256];
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen=sizeof(clientAddr);
    int n;

    //creating socket
    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    MYERROR(socketFd<0,"creating socket failed",-1);

    //setting (struct sockaddr_in)serverAddr
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_addr.s_addr=INADDR_ANY;
    serverAddr.sin_port=3001;
    
    //bind, listen, accept
    MYERROR(bind(socketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr))<0,"binding socket failed",-2);
    MYERROR(listen(socketFd,5)<0,"listening failed",-3);
    inSocketFd=accept(socketFd,(struct sockaddr *)&clientAddr,&clientLen);
    MYERROR(inSocketFd<0,"accepting failed",-4);

    //recv data
    n = recv(inSocketFd,buffer,256,0);
    MYERROR(n<0,"receiving failed",-5);

    //print data
    printf("Data Recved (%d) : %s\n",n,buffer);

    sprintf(buffer,"Roger that. %d bytes",n);
    n = send(inSocketFd,buffer,strlen(buffer)+1,0);
    MYERROR(n<0,"sending failed",-6);
    printf("Response with %d bytes\n",n);

    //close socket
    close(inSocketFd);
    close(socketFd);
    return 0;
}