#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#define TIMEOUT 6000
#define DEFUALT_PRT 23
#define DO 253
#define WONT 252
#define WILL 251
#define DONT 254
#define IAC 255
#define MAX_MESSAGE_LEN 2000

static struct termios terminal;

void _reset_terminal(void) {
    tcsetattr(STDIN_FILENO,TCSANOW,&terminal);
}

void show_IACodes(char *buffer,int len){ 
    printf("%d %d %d\n",(int)buffer[0]+256,(int)buffer[1]+256,(int)buffer[2]);
}

int main(int argc, char* argv[]) {

    struct sockaddr_in sockaddrIn;
    int socket_value;
    struct hostent *hostent1;
    struct in_addr **addr_list;
    struct pollfd pollfd1[2];
    int buffer_length;
    char buffer[MAX_MESSAGE_LEN];
    struct termios nvt_terminal;

    socket_value = socket(AF_INET, SOCK_STREAM, 6);    // Creating Socket for Communication with tcp
    if (socket_value < 0) {                            // tcp protocol number is 6
        printf("%s\n", "Cannot Create Socket");     // AF_INET Corrosponds to ipv4 adresses
        return -1;
    }
    printf("Socket Created\n");

    sockaddrIn.sin_family = AF_INET;                  // Defining Socket Parameters for 2 way Communication

    if(argc==2){
        sockaddrIn.sin_port=htons(DEFUALT_PRT);
    }
    else{
        sockaddrIn.sin_port = htons(atoi(argv[2]));
    }

    hostent1 = gethostbyname(argv[1]);
    if(hostent1->h_addr_list==NULL){
        printf("NO Internet Connection or Server address Wrong");
        exit(0);
    }

    addr_list = (struct in_addr **) hostent1->h_addr_list;
    printf("%s\n", inet_ntoa(*addr_list[0]));

    if (inet_pton(AF_INET, inet_ntoa(*addr_list[0]), &sockaddrIn.sin_addr) <= 0) {
        printf("INVALID Address\n");                   // Checking if Ip Address exists or not
        return -1;
    }
    printf("Valid ADDRESS\n");
    if (connect(socket_value, (struct sockaddr *) &sockaddrIn, sizeof(sockaddrIn)) < 0)   // Connecting  socket with
    {                                                                                   // sin_addr
        printf("Couldn't Connect \n");
        return -1;
    }
    printf("Connection made\n");

    fcntl(socket_value,F_SETFL,O_NONBLOCK);

    tcgetattr(STDIN_FILENO,&terminal);
    atexit(_reset_terminal);
    cfmakeraw(&nvt_terminal);
    nvt_terminal.c_lflag = nvt_terminal.c_lflag | ECHO;
    nvt_terminal.c_iflag = nvt_terminal.c_iflag | ICRNL | INLCR;
    tcsetattr(STDIN_FILENO,TCSANOW,&nvt_terminal);

    pollfd1[0].fd=socket_value;
    pollfd1[0].events=POLLIN;
    pollfd1[1].fd=0;
    pollfd1[1].events=POLLIN;

    while(poll(pollfd1,2,TIMEOUT)!=-1){

        if(pollfd1[0].revents & POLLIN){
            if(recv(socket_value,buffer,1,0)==0){
                printf("Connection lost between remote host and foreign host");
                return 1;
            }
            if((int)buffer[0]+256==IAC){
                recv(socket_value,buffer+1,2,0);
                show_IACodes(buffer,3);
            } else{
                buffer_length=1;
                buffer[buffer_length]='\0';
                printf("%s",buffer);
                fflush(0);
            }
        }
        if((pollfd1[1].revents & POLLIN)){
            fgets(buffer,MAX_MESSAGE_LEN,stdin);
            if(send(socket_value,buffer,strlen(buffer),0)<0){
                printf("Broken Connection");
                return 1;
            }
        }
    }
    close(socket_value);
    return 0;
}
