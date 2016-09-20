//    ./client -h 143.248.56.16 -p 3000 -o 0 -s 5
//    ./client -h localhost -p 3000 -o 0 -s 5
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

#define BUFFERSIZE 256

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

void hexDump(unsigned char* buffer, size_t length) {
    size_t i;
    printf("===\nDumping %p...",buffer);
    for(i=0;i<length;i++) {
        if(i%4==0) printf(" ");
        if(i%16==0) printf("\n");
        printf("%02x",buffer[i]);
    }
    printf("\n");
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
int sendMessage(char* hostName, unsigned short port, bool isEncrypt, unsigned char shift, size_t length, char* message) {
    int sockFd;
    struct sockaddr_in serverAddr;
    struct hostent *server;

    // char buffer[BUFFERSIZE+1];
    char header[9]={0,};

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
    printf("checksum check = %hx\n",accumulateShorts(header,message,length));
    // printf("dumping header");
    // hexDump(header,9);
    n=send(sockFd,header,8,0);
    if(n==8) {
        // printf("dumping message");
        // hexDump(message,length);
        n=send(sockFd,message,length,0);
        MYERROR(n!=length,"sending message failed",-4);
    }
    else {
        MYERROR(true,"sending header failed",-5);
    }
    // n=send(sockFd,buffer,length+8,0);
    // while(remainedSize>0)
    {
        // size_t sending= remainedSize>=BUFFERSIZE ? BUFFERSIZE : remainedSize;
        // strncpy(buffer,message,sending);
        // n=send(sockFd,buffer+index,sending,0);
        // MYERROR(n!=length+8,"sending failed",-4);
        // printf("Sending %lu bytes...\n",n);
        // remainedSize-=n;
        // index+=n;

        n=recv(sockFd,header,8,0);
        MYERROR(n<0,"receiving header failed",-5);

        n=recv(sockFd,message,length,0);
        MYERROR(n<0,"receiving response failed",-5);
        printf("Response : %s\n",message);
    }
    close(sockFd);
    return 0;
}

int main(int argc, char **argv) {
    char hostName[30];
    int port;
    bool isEncrypt;
    int shift;

    bool optionsFlag[MAX_OPTION];
    bool sumOptionsFlag;

    struct sockaddr hostSockaddr;

    size_t i, j;
    int temp;

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
        temp=read(0,buffer,BUFFERSIZE);
        printf("read : %d\n",temp);
        if(temp>0) {
            int temp2=0;
            temp2=read(0,buffer+temp,BUFFERSIZE-temp);
            if(temp2==0) {
                // temp=6;
                // printf("copy starting");
                // strncpy(buffer,"Hello",temp);
                // printf("copy complete");
                printf("host : %s\nport : %d\n",hostName,port);
                sendMessage(hostName,port,isEncrypt,shift,temp,buffer);
                // printf("right after sendMessage()\n");
            }
        }
    }

    // printf("before freeing\n");
    free(buffer);
    return 0;
}