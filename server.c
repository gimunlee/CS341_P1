#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>

#include<string.h>
#include<limits.h>
#include<ctype.h>

#include<netinet/in.h>
#include<sys/socket.h>
#include<unistd.h>

#define MAX_OPTION 1
char OPTIONS[MAX_OPTION][3]={"-p"};

#define BUFFERSIZE 10*1024*1024

// #define MYERROR(condition,msg,errorValue); if((condition)){printf("%s\n",(msg));return (errorValue);}
#define MYERROR(condition,msg,errorValue); if((condition)){return (errorValue);}
//For Debugging, dump given memory space   
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
//Given pseudo header and message, calculate checksum of it.
unsigned short calcChecksum(char* header, char* message, size_t length) {
    unsigned long sum=0;
    size_t i;

    for(i=0;i<8;i+=2)
        sum+=*(unsigned short*)(header+i);
    for(i=0;i<length-1;i+=2) {
        sum+=*(unsigned short*)(message+i);
        if(sum>USHRT_MAX)
            sum=(sum & USHRT_MAX) + (sum>>16);
    }
    if(length%2==1)
        sum+=*(unsigned char*)(message+length-1);

    while(sum>USHRT_MAX)
        sum=(sum & USHRT_MAX) + (sum>>16);

    return ~sum;
}

//Encrypt with signed shift
void cipher(char* buffer, size_t length, int shift) {
    size_t i;
    int tempChar;

    for(i=0;i<length;i++) {
        buffer[i]=(char)tolower((int)buffer[i]);
        tempChar=buffer[i]-'a';
        if(0<=tempChar && tempChar<26)
            buffer[i]='a'+(((tempChar+shift)%26)+26)%26;
    }
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

int main(int argc, char **argv) {
    int socketFd;
    struct sockaddr_in serverAddr, clientAddr;
    short port=3000;
    socklen_t clientLen=sizeof(clientAddr);

    bool optionsFlag[MAX_OPTION]; //whether each option is input

    size_t n; //temp

    size_t i, j;

    for(i=1;i<argc;i+=2) {
        //Recognize next flag
        for(j=0;j<MAX_OPTION;j++) {
            if(strncmp(&OPTIONS[j][0],argv[i],3)==0) {
                optionsFlag[j]=true;
                break;
            }
        }
        //switch to each statement for each flag
        switch(j) {
            case 0 :
                port=atoi(argv[i+1]);
                if(port<0 || port>USHRT_MAX) //Out of bound
                    optionsFlag[1]=false;
                break;
            default: 
                i=argc;//Unexpected flag
        }
    }

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
    serverAddr.sin_port=port;
    
    //bind, listen, accept
    MYERROR(bind(socketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr))<0,"binding socket failed",-2);

        MYERROR(listen(socketFd,5)<0,"listening failed",-3);
    while(true) {
        // printf("first of while");
        int inSocketFd=accept(socketFd,(struct sockaddr *)&clientAddr,&clientLen);
        MYERROR(inSocketFd<0,"accepting failed",-4);

        int pid=fork();

        if(pid==0)
        //Dealing accepted socket
        {
            char header[9];
            unsigned char shift;
            char op;
            size_t length;

            char* buffer;
            buffer=malloc(BUFFERSIZE);

            //receive header
            n = recv(inSocketFd,header,8,0);
            MYERROR(n!=8,"receiving header failed",-5);

            //recognize options
            op=header[0];
            shift=header[1];
            length=ntohl(*((size_t*)(header+4)));

            n=0;
            while(n<length) {
                int temp = recv(inSocketFd,buffer+n,length,0);
                MYERROR(temp<=0,"receiving message failed",-5);
                n+=temp;
            }

            if(calcChecksum(header,buffer,length)!=0) {
                // printf("checksum violation");
                free(buffer);
                goto END_CONN;
            }

            //Encrypt or recover
            cipher(buffer,length,((int)shift)*(op==0?1:-1));

            //Sending response
            buildHeader(header,op==0?true:false,shift,length,buffer);
            n = send(inSocketFd,header,8,0);
            MYERROR(n<0,"sending header failed",-6);

            n = send(inSocketFd,buffer,length,0);
            MYERROR(n<0,"sending message failed",-6);


            END_CONN:
            //close socket
            close(inSocketFd);
            free(buffer);
            // printf("exit");
            exit(1);
        }
    }

    close(socketFd);
    return 0;
}