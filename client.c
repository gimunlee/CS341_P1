//    ./client -h 143.248.111.222 -p 1234 -o 0 -s 5
#include<stdio.h>
#include<stdbool.h>

#include<stdlib.h>
#include<string.h>
#include<limits.h>

#include<netinet/in.h>
#include<sys/socket.h>
#include<netdb.h>
#include<unistd.h>

#define MYERROR(condition,msg,errorValue); if((condition)){printf("%s\n",(msg));return (errorValue);}

#define MAX_OPTION 4
char OPTIONS[MAX_OPTION][3]={"-h","-p","-o","-s"};

#define BUFFERSIZE 10<<20+10

unsigned short accumulateShorts(char* header, char* message, size_t length) {
    unsigned long sum=0;
    size_t i;

    for(i=0;i<8;i+=2)
        sum+=*(unsigned short*)(header+i);

    for(i=0;i<length-1;i+=2)
        sum+=*(unsigned short*)(message+i);
    if(length%2==1)
        sum+=*(char*)(message+length-1);

    while(sum>USHRT_MAX)
        sum=(sum & USHRT_MAX) + sum>>16;

    return ~sum;
}

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
void buildHeader(char* header, bool isEncrypt, unsigned char shift, size_t length, char* message) {
    //Build Pseudo Header
    {
        header[0]=isEncrypt?0:1;
        *(unsigned char*)(header+1)=shift;
        header[2]=header[3]=0;
        *(unsigned long*)(header+4)=htonl(length);
    }

    //Copy message
    // printf("=== dumping message\n");
    // hexDump(message,length);
    // strncpy(buffer+8,message,length);
    // hexDump(buffer+8,length);

    //Record checksum
    *(unsigned short*)(header+2)=accumulateShorts(header,message,length);
}
int sendMessage(char* hostName, unsigned short port, bool isEncrypt, unsigned char shift, size_t length, char* message) {
    int sockFd;
    struct sockaddr_in serverAddr;
    struct hostent *server;

    // char buffer[BUFFERSIZE+1];
    char header[8]={0,};

    size_t remainedSize=length;
    size_t index=0;

    size_t n;

    sockFd = socket(AF_INET, SOCK_STREAM, 0);
    MYERROR(sockFd<0,"creating socket failed",-1);
    
    server = gethostbyname(hostName);
    MYERROR(server==NULL,"getting host name failed",-2);
    
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family=AF_INET;
    memcpy((char*)&(serverAddr.sin_addr.s_addr),(char*)server->h_addr,server->h_length);
    serverAddr.sin_port=port;

    MYERROR(connect(sockFd,(struct sockaddr*)&serverAddr,sizeof(serverAddr))<0,"connecting failed",-3);

    buildHeader(header,isEncrypt,shift,length,message);
    hexDump(header,9);
    n=send(sockFd,header,8,0);
    if(n==8) {
        hexDump(message,length);
        n=send(sockFd,message,length,0);
        MYERROR(n!=length,"sending message failed",-4);
    }
    else
        MYERROR(true,"sending header failed",-5);
    // n=send(sockFd,buffer,length+8,0);
    // while(remainedSize>0)
    // {
        // size_t sending= remainedSize>=BUFFERSIZE ? BUFFERSIZE : remainedSize;
        // strncpy(buffer,message,sending);
        // n=send(sockFd,buffer+index,sending,0);
        // MYERROR(n!=length+8,"sending failed",-4);
        // printf("Sending %lu bytes...\n",n);
        // remainedSize-=n;
        // index+=n;

        n=recv(sockFd,message,BUFFERSIZE,0);
        MYERROR(n<0,"receiving response failed",-5);
        printf("Response : %s\n",message+8);
    // }
    close(sockFd);
    return 0;
}

int main(int argc, char **argv) {
    char hostName[20];
    int port;
    bool isEncrypt;
    int shift;

    bool optionsFlag[MAX_OPTION];
    bool sumOptionsFlag;

    struct sockaddr hostSockaddr;

    size_t i, j;
    int temp;
    // int temp2;

    char* buffer;
    buffer=malloc(100);
    printf("buffer : %p\n",buffer);

    memset(&hostSockaddr,0,sizeof(hostSockaddr));

    memset(optionsFlag,0,sizeof(optionsFlag));
    sumOptionsFlag=true;

    for(i=1;i<argc;i+=2) {
        for(j=0;j<4;j++) {
            if(strncmp(&OPTIONS[j][0],argv[i],3)==0) {
                optionsFlag[j]=true;
                break;
            }
        }
        switch(j) {
            case 0 : 
                strncpy(hostName,argv[i+1],strlen(argv[i+1]));
                break;
            case 1 :
                port=atoi(argv[i+1]);
                if(port<0 || port>USHRT_MAX)
                    optionsFlag[1]=false;
                break;
            case 2 : 
                temp=atoi(argv[i+1]);
                switch(temp) {
                    case 0: isEncrypt=true; break;
                    case 1: isEncrypt=false; break;
                    default: optionsFlag[2]=false;
                }
                break;
            case 3 :
                shift=atoi(argv[i+1]);
                if(shift<0 || shift>UCHAR_MAX)
                    optionsFlag[3]=false;
                break;
            default: 
                i=argc;//Unexpected flag
        }
    }
    for(i=0;i<MAX_OPTION;i++)
        sumOptionsFlag = sumOptionsFlag && optionsFlag[i];
    if(sumOptionsFlag) {
        // printf("hostName : %s\nport : %d\nEncrypt? : %d\nshift : %d\n",hostName,port,isEncrypt?1:0,shift);
        // temp=read(0,buffer,BUFFERSIZE);
        temp=1;
        printf("read : %d\n",temp);
        printf("test\n");
        if(temp>0) {
            int temp2=0;
            // temp2=0;
            printf("temp2\n");
            // temp2=read(0,buffer+temp,BUFFERSIZE-temp);
            if(temp2==0) {
                temp=5;
                // printf("copy starting");
                strncpy(buffer,"Hello",temp);
                // printf("copy complete");
                sendMessage(hostName,port,isEncrypt,shift,temp,buffer);
            }
        }
    }

    free(buffer);
    return 0;
}