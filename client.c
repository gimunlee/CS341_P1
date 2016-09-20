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

// #define BUFFERSIZE 2
#define BUFFERSIZE 10*1024*1024

//Given pseudo header and message, calculate checksum of it.
unsigned short calcChecksum(char* header, char* message, size_t length) {
    unsigned long sum=0;
    size_t i;

    for(i=0;i<8;i+=2)
        sum+=*(unsigned short*)(header+i);
    for(i=0;i<length-1;i+=2)
        sum+=*(unsigned short*)(message+i);
    if(length%2==1)
        sum+=*(unsigned char*)(message+length-1);

    while(sum>USHRT_MAX)
        sum=(sum & USHRT_MAX) + (sum>>16);

    return ~sum;
}

//For Debugging, dump given memory space
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
//Build a new header with given message.
void buildHeader(unsigned char* header, bool isEncrypt, unsigned char shift, size_t length, char* message) {
    //Build Pseudo Header
    {
        header[0]=isEncrypt?0:1;
        header[1]=shift;
        header[2]=header[3]=0;
        *(size_t*)(header+4)=htonl(length);
    }

    //Record checksum
    *(unsigned short*)(header+2)=calcChecksum(header,message,length);
}

//sending message
int sendMessage(char* hostName, unsigned short port, bool isEncrypt, unsigned char shift, size_t length, char* message) {
    int sockFd;
    struct sockaddr_in serverAddr;
    struct hostent *server;

    // char buffer[BUFFERSIZE+1];
    char header[9]={0,};

    size_t remainedSize=length;
    size_t index=0;

    size_t n;

    //setting socket to connect
    {
        sockFd = socket(AF_INET, SOCK_STREAM, 0);
        MYERROR(sockFd<0,"creating socket failed",-1);
        
        server = gethostbyname(hostName);
        MYERROR(server==NULL,"getting host name failed",-2);
        
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family=AF_INET;
        memcpy((char*)&(serverAddr.sin_addr.s_addr),(char*)server->h_addr,server->h_length);
        serverAddr.sin_port=port;
    }

    MYERROR(connect(sockFd,(struct sockaddr*)&serverAddr,sizeof(serverAddr))<0,"connecting failed",-3);

    //create a header with stdin message
    buildHeader(header,isEncrypt,shift,length,message);

    //sending head
    n=send(sockFd,header,8,0);
    if(n==8) { //If all head tranmitted
        //sending message body
        n=0;
        while(n<length) {
            int temp;
            temp=send(sockFd,message+n,length< BUFFERSIZE ? length : BUFFERSIZE,0);
            MYERROR(temp==-1,"sending message failed",-4);
            n+=temp;
        }
    }
    else {
        MYERROR(true,"sending header failed",-4);
    }
    {
        //receive header of response
        n=recv(sockFd,header,8,0);
        MYERROR(n<0,"receiving header failed",-5);
        if(n==8) { //If all head tranmitted
            //receiving message body
            n=0;
            while(n<length) {
                int temp=recv(sockFd,message+n,length,0);
                MYERROR(temp<=0,"receiving response fully failed",-5);
                n+=temp;
            }
            // n=recv(sockFd,message,length,0);
            write(1,message,length);
            // printf("Response : %s\n",message);
        }
    }
    close(sockFd);
    return 0;
}

int main(int argc, char **argv) {
    char hostName[30];
    int port;
    bool isEncrypt;
    int shift;

    bool optionsFlag[MAX_OPTION]; //whether each option is input
    bool sumOptionsFlag; //&& of flags

    struct sockaddr hostSockaddr;

    size_t i, j;
    int temp;

    char* buffer;
    buffer=malloc(BUFFERSIZE);
    // printf("buffer : %p\n",buffer);

    memset(&hostSockaddr,0,sizeof(hostSockaddr));

    memset(optionsFlag,0,sizeof(optionsFlag));
    sumOptionsFlag=true;

    //from argument of main(), to options
    for(i=1;i<argc;i+=2) {
        //Recognize next flag
        for(j=0;j<4;j++) {
            if(strncmp(&OPTIONS[j][0],argv[i],3)==0) {
                optionsFlag[j]=true;
                break;
            }
        }
        //switch to each statement for each flag
        switch(j) {
            case 0 : 
                strncpy(hostName,argv[i+1],strlen(argv[i+1]));
                break;
            case 1 :
                port=atoi(argv[i+1]);
                if(port<0 || port>USHRT_MAX) //Out of bound
                    optionsFlag[1]=false;
                break;
            case 2 : 
                temp=atoi(argv[i+1]);
                switch(temp) {
                    case 0: isEncrypt=true; break;
                    case 1: isEncrypt=false; break;
                    default: optionsFlag[2]=false; //Out of bound
                }
                break;
            case 3 :
                shift=atoi(argv[i+1]);  
                if(shift<0 || shift>UCHAR_MAX) //Out of bound
                    optionsFlag[3]=false;
                break;
            default: 
                i=argc;//Unexpected flag
        }
    }
    for(i=0;i<MAX_OPTION;i++)
        sumOptionsFlag = sumOptionsFlag && optionsFlag[i];
    if(sumOptionsFlag) {
        //All options are input
        int temp;
        while(temp=read(0,buffer,BUFFERSIZE)) {
            MYERROR(temp==-1,"Reading error",-11);
            sendMessage(hostName,port,isEncrypt,shift,temp,buffer);
        }
    }

    free(buffer);
    return 0;
}