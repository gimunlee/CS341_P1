#include<stdio.h>
#include<stdbool.h>

#include<string.h>

#include<netinet/in.h>
#include<sys/socket.h>
#include<netdb.h>
#include<unistd.h>

#define MYERROR(condition,msg,errorValue); if((condition)){printf("%s\n",(msg));return (errorValue);}

#define MAX_OPTION 4
char OPTIONS[MAX_OPTION][3]={"-h","-p","-o","-s"};
bool OPTIONS_FLAG[MAX_OPTION];

int sendHello(char* hostName) {
    int sockFd;
    struct sockaddr_in serverAddr;
    struct hostent *server;

    char buffer[256];

    int n;

    sockFd = socket(AF_INET, SOCK_STREAM, 0);
    MYERROR(sockFd<0,"creating socket failed",-1);
    
    server = gethostbyname(hostName);
    MYERROR(server==NULL,"getting host name failed",-2);
    
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family=AF_INET;
    memcpy((char*)&(serverAddr.sin_addr.s_addr),(char*)server->h_addr,server->h_length);
    serverAddr.sin_port=3001;

    MYERROR(connect(sockFd,(struct sockaddr*)&serverAddr,sizeof(serverAddr))<0,"connecting failed",-3);
    
    sprintf(buffer,"Hello");
    n=send(sockFd,buffer,strlen(buffer)+1,0);
    MYERROR(n<0,"sending failed",-4);
    printf("Sending %d bytes...\n",n);

    n=recv(sockFd,buffer,256,0);
    MYERROR(n<0,"receiving response failed",-5);
    printf("Response : %s\n",buffer);
    
    close(sockFd);
    return 0;
}

int main(int argc, char **argv) {
    unsigned int hostAddress;
    unsigned int port;
    bool isEncrypt;
    unsigned int shift;

    struct sockaddr hostSockaddr;

    int i, j;

    memset(&hostSockaddr,0,sizeof(hostSockaddr));

    // printf("Hello, this is client\n");
    // printf("argc : %d\n",argc);
    // for(i=0;i<argc;i++) {
    //     printf("argv[%d] : %s\n",i,argv[i]);
    // }
    for(i=1;i<argc;i+=2) {
        for(j=0;j<4;j++) {
            if(strncmp(&OPTIONS[j][0],argv[i],3)==0) {
                OPTIONS_FLAG[j]=true;
                break;
            }
        }
        switch(j) {
            case 0 : printf("host : %s\n",argv[i+1]);
                sendHello(argv[i+1]);
                // hostSockaddr.sa_family=AF_INET;
                // hostSockaddr.sa_data=
                 break;
            case 1 : /*printf("port : %s\n",argv[i+1]);*/ break;
            case 2 : /*printf("isEncrypt : %s\n",argv[i+1]);*/ break;
            case 3 : /*printf("shift : %s\n",argv[i+1]);*/ break;
            default: /*printf("unexpected flag : %s\n",argv[i]);*/ i=argc;//Unexpected flag
        }
    }
    for(i=0;i<MAX_OPTION;i++)
        printf("%s %d\n",OPTIONS[i],OPTIONS_FLAG[i]);
//    ./client -h 143.248.111.222 -p 1234 -o 0 -s 5
//    HOST. 143.248.111.222, Port: 1234 Operation: encrypt, Shift 5
    return 0;
}