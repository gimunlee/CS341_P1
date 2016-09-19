#include<stdio.h>
#include<stdbool.h>

#include<string.h>
#include<limits.h>
#include<ctype.h>

#include<netinet/in.h>
#include<sys/socket.h>
#include<unistd.h>

#define MYERROR(condition,msg,errorValue); if((condition)){printf("%s\n",(msg));return (errorValue);}
unsigned short accumulateShorts(char* buffer, size_t length) {
    unsigned long sum=0;
    size_t i;

    for(i=0;i<length-1;i+=2)
        sum+=*(unsigned short*)(buffer+i);
    if(length%2==1)
        sum+=*(char*)(buffer+length-1);

    while(sum>USHRT_MAX)
        sum=(sum & USHRT_MAX) + sum>>16;

    return ~sum;
}

void cipher(char* buffer, size_t length, int shift) {
    size_t i;
    int tempChar;

    // printf("======\n");
    // printf("input : %s\n",buffer);
    for(i=0;i<length;i++) {
        buffer[i]=(char)tolower((int)buffer[i]);
        tempChar=buffer[i]-'a';
        if(0<tempChar && tempChar<26)
            buffer[i]='a'+(((tempChar+shift)%26)+26)%26;
    }
    // printf("plain : %s\n",buffer);
    // printf("cipher : %s\n",buffer);
}

int main(void) {
    int socketFd, inSocketFd;
    char buffer[256];
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen=sizeof(clientAddr);
    size_t n;

    // strncpy(buffer,"Hello, This is Gimun Lee!",strlen("Hello, This is Gimun Lee!")+1);
    // cipher(buffer,strlen(buffer),-6);

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
    printf("Data Recved (%lu) : %s\n",n,buffer);

    sprintf(buffer,"Roger that. %lu bytes",n);
    n = send(inSocketFd,buffer,strlen(buffer)+1,0);
    MYERROR(n<0,"sending failed",-6);
    printf("Response with %lu bytes\n",n);

    //close socket
    close(inSocketFd);
    close(socketFd);
    return 0;
}