#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <time.h>

void get_timestamp(char *buffer) {
    struct timespec ts;
    struct tm *tm;

    clock_gettime(CLOCK_REALTIME, &ts);
    tm = localtime(&ts.tv_sec);
    strftime(buffer, 24, "%Y.%m.%d-%H:%M:%S", tm);
    sprintf(buffer, "%s.%03ld", buffer, ts.tv_nsec / 1000000L);
}

void send_message(int socketFd, const char *timestamp, const char *buffer) {
    char temp[256];
    strcpy(temp, timestamp);
    strcat(temp, " ");
    strcat(temp, buffer);
    write(socketFd, temp, strlen(temp));
}

int handle_response(int socketFd, char *buffer) {
    bzero(buffer, 256);
    read(socketFd, buffer, 255);

    char* buf = strchr(buffer, ' ');
    if(buf) {
        buf++;
        if (strncmp(buf, "BYE!", 4) == 0) {
            printf("%s\n", buffer);
            printf("Client: Closing connection\n");
            close(socketFd);
            return 0;
        }
        if (strncmp(buf, "LIST", 4) == 0) {
            while(1) {
                bzero(buffer, 256);
                read(socketFd, buffer, 255);
                char* buff = strchr(buffer, ' ');
                if (buff) {
                    buff++;
                    if (strncmp(buff, "DONE", 4) == 0) {
                        break;
                    }
                }
                printf("%s", buffer);
            }
        }
    }

    printf("%s\n", buffer);
    return 1;
}

void handle_client(int socketFd) {
    char buffer[256];
    char timestamp[24];

    while(1) {    
        bzero(buffer, 256);
        read(socketFd, buffer, 255);
        printf("%s", buffer);

        bzero(buffer, 256);
        fgets(buffer, 255, stdin);
        if (buffer[0] == '@') {
            buffer[strcspn(buffer, "\n")] = 0;
            FILE *file = fopen(buffer + 1, "r");
            if (file == NULL) {
                perror("ERROR opening file");
                exit(1);
            }
            while (fgets(buffer, 255, file) != NULL) {
                get_timestamp(timestamp);
                send_message(socketFd, timestamp, buffer);
                if (!handle_response(socketFd, buffer)) {
                    return;
                }
            }
            fclose(file);
        } else {
            get_timestamp(timestamp);
            send_message(socketFd, timestamp, buffer);
            if (!handle_response(socketFd, buffer)) {
                return;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int socketFd, portNum, n;
    struct sockaddr_in serverAddr;
    struct hostent *server;

    char buffer[256];
    portNum = atoi(argv[2]);

    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketFd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    server = gethostbyname(argv[1]);
    if(server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serverAddr, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serverAddr.sin_addr.s_addr, server->h_length);
    serverAddr.sin_port = htons(portNum);

    if(connect(socketFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("ERROR connecting");
        exit(1);
    } else {
        bzero(buffer, 256);
        n = read(socketFd, buffer, 255);
        printf("%s\n", buffer);
    }

    handle_client(socketFd);

    close(socketFd);
    return 0;
}