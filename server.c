#include<stdio.h>
#include<stdbool.h>

#include<string.h>
#include<limits.h>
#include<ctype.h>

#include<netinet/in.h>
#include<sys/socket.h>
#include<unistd.h>

#define BUFFERSIZE 256

// struct header {
//     char op;
//     unsigned char shift;
//     size_t length;
// };

#define MYERROR(condition,msg,errorValue); if((condition)){printf("%s\n",(msg));return (errorValue);}
void hexDump(unsigned char* buffer, size_t length) {
    size_t i;
    printf("===\nDumping %p...\n",buffer);
    for(i=0;i<length;i++) {
        if(i%4==0) printf(" ");
        if(i%16==0) printf("\n");
        printf("%02x",buffer[i]);
    }
    printf("\n");
}
unsigned short accumulateShorts(char* header, char* message, size_t length) {
    unsigned long sum=0;
    size_t i;

    // printf("acc header\n");
    for(i=0;i<8;i+=2) {
        printf("%p : %4hx\n",header+i,*(unsigned short*)(header+i));
        sum+=*(unsigned short*)(header+i);
    }

    // printf("acc message\n");
    for(i=0;i<length-1;i+=2) {
        printf("%p : %4hx\n",message+i,*(unsigned short*)(message+i));
        sum+=*(unsigned short*)(message+i);
    }
    if(length%2==1) {
        sum+=*(unsigned char*)(message+length-1);
         printf("adding last byte. %x at %lu\n",*(unsigned char*)(message+length-1),length-1);
    }
     printf("sum before wrap around : %lx\n",sum);

    while(sum>USHRT_MAX) {
        printf("wrap around with %lx and %lx\n",sum&USHRT_MAX,sum>>16);
        sum=(sum & USHRT_MAX) + (sum>>16);
    }

    printf("sum result ==== %hx\n",(unsigned short)sum);
    printf("accu result ==== %hx\n",(unsigned short)~sum);

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

void buildHeader(unsigned char* header, bool isEncrypt, unsigned char shift, size_t length, char* message) {
    //Build Pseudo Header
    {
        header[0]=isEncrypt?0:1;
        header[1]=shift;
        header[2]=header[3]=0;
        *(size_t*)(header+4)=htonl(length);
    }

    //Copy message
    // printf("=== dumping message\n");
    // hexDump(message,length);
    // strncpy(buffer+8,message,length);
    // hexDump(buffer+8,length);

    // hexDump(header,8);
    //Record checksum
    *(unsigned short*)(header+2)=accumulateShorts(header,message,length);
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
    if(setsockopt(socketFd,SOL_SOCKET,SO_REUSEADDR,&(int){1},sizeof(int))<0) {
        MYERROR(true,"setting options failed",-10);
    }
    MYERROR(socketFd<0,"creating socket failed",-1);

    //setting (struct sockaddr_in)serverAddr
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_addr.s_addr=INADDR_ANY;
    serverAddr.sin_port=3000;
    
    //bind, listen, accept
    MYERROR(bind(socketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr))<0,"binding socket failed",-2);
    MYERROR(listen(socketFd,5)<0,"listening failed",-3);
    inSocketFd=accept(socketFd,(struct sockaddr *)&clientAddr,&clientLen);
    MYERROR(inSocketFd<0,"accepting failed",-4);

    {
        char header[9];
        unsigned char shift;
        char op;
        size_t length;

        //recv data
        n = recv(inSocketFd,header,8,0);
        MYERROR(n<0,"receiving header failed",-5);
        printf("Header received");
        // hexDump(header,8);

        op=header[0];
        shift=header[1];
        length=ntohl(*((size_t*)(header+4)));
        printf("op = %d\nshift = %d\nlength = %lu\n",op,shift,length);

        //print data
        // printf("Header Received (%lu) : %s\n",n,buffer);

        n = recv(inSocketFd,buffer,length,0);
        MYERROR(n<0,"receiving message failed",-5);
        printf("Roger that. %lu bytes\n",n);

        printf("checksum = %x\n",(int)accumulateShorts(header,buffer,length));
        MYERROR(accumulateShorts(header,buffer,length)!=0,"checksum violation",-8);

        // n = send(inSocketFd,header,length,0);

        cipher(buffer,length,((int)shift)*(op==0?1:-1));

        buildHeader(header,op==0?true:false,shift,length,buffer);
        n = send(inSocketFd,header,8,0);
        MYERROR(n<0,"sending header failed",-6);

        n = send(inSocketFd,buffer,length,0);
        MYERROR(n<0,"sending message failed",-6);
        printf("Response with %lu bytes\n",n);
    }

    //close socket
    close(inSocketFd);
    close(socketFd);
    return 0;
}