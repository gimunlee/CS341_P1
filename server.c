#include<stdio.h>
#include<stdbool.h>

#include<string.h>

#include<netinet/in.h>
#include<sys/socket.h>
#include<unistd.h>

int main(void) {
    int socketFd, inSocketFd;
    socklen_t clientLen;
    char buffer[256];
    struct sockaddr_in serverAddr, clientAddr;
    int n;

    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketFd<0) {
        printf("creating socket failed\n");
        return -1;
    }
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_addr.s_addr=INADDR_ANY;
    serverAddr.sin_port=3001;
    if(bind(socketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr))<0) {
        printf("binding socket failed\n");
        return -2;
    }
    if(listen(socketFd,5)<0) {
        printf("listening failed\n");
        return -3;
    }
    clientLen=sizeof(clientAddr);
    inSocketFd=accept(socketFd,(struct sockaddr *)&clientAddr,&clientLen);
    if(inSocketFd<0) {
        printf("accepting failed");
        return -4;
    }
    n = recv(inSocketFd,buffer,256,0);
    if(n<0) {
        printf("receiving failed");
        return -5;
    }
    printf("Data Recved : %s\n",buffer);
    close(inSocketFd);
    close(socketFd);
    return 0;
}