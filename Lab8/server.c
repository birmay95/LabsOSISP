#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

char base_dir[256] = "";

void get_timestamp(char *buffer) {
    struct timespec ts;
    struct tm *tm;

    clock_gettime(CLOCK_REALTIME, &ts);
    tm = localtime(&ts.tv_sec);
    strftime(buffer, 24, "%Y.%m.%d-%H:%M:%S", tm);
    sprintf(buffer, "%s.%03ld", buffer, ts.tv_nsec / 1000000L);
}

void send_message(int socketFd, const char *timestamp, const char *message) {
    char temp[256];
    strcpy(temp, timestamp);
    strcat(temp, message);
    write(socketFd, temp, strlen(temp));
}

int handle_command(int socketFd, char *buffer, char *buf, const char *command, const char *response, int skip) {
    if (strncmp(buf, command, 4) == 0) {
        buf += skip;
        char before_space[256];
        strncpy(before_space, buffer, buf - buffer - skip);
        before_space[buf - buffer - skip] = '\0';

        char timestamp[24];
        get_timestamp(timestamp);
        printf("Server [%s]: %s- %s\n", timestamp, before_space, buf);
        send_message(socketFd, timestamp, response);
        return 1;
    }
    return 0;
}

void list_files(int socketFd, char* name) {
    DIR *d;
    struct dirent *dir;
    d = opendir(name);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            struct stat sb;
            if (lstat(dir->d_name, &sb) == -1) {
                perror("lstat");
                continue;
            }
            char message[256];
            switch (sb.st_mode & __S_IFMT) {
                case __S_IFDIR:
                    sprintf(message, " /%s/\n", dir->d_name);
                    break;
                case __S_IFLNK: {
                    char link_target[256];
                    ssize_t len = readlink(dir->d_name, link_target, sizeof(link_target)-1);
                    if (len != -1) {
                        link_target[len] = '\0';
                        struct stat target_stat;
                        if (lstat(link_target, &target_stat) == -1) {
                            perror("lstat");
                            continue;
                        }
                        if ((target_stat.st_mode & __S_IFMT) == __S_IFLNK) {
                            sprintf(message, " /%s -->> %s\n", dir->d_name, link_target);
                        } else {
                            sprintf(message, " /%s --> %s\n", dir->d_name, link_target);
                        }
                    }
                    break;
                }
                case __S_IFREG:
                    sprintf(message, " /%s\n", dir->d_name);
                    break;
            }

            char timestamp[24];
            get_timestamp(timestamp);
            send_message(socketFd, timestamp, message);
            usleep(50000);
        }
        char timestamp[24];
        get_timestamp(timestamp);
        send_message(socketFd, timestamp, " DONE");
        closedir(d);
    }
}

int handle_cd_command(int socketFd, char *buffer, char *buf) {
    if (strncmp(buf, "CD", 2) == 0) {
        char before_space[256];
        strncpy(before_space, buffer, buf - buffer);
        before_space[buf - buffer] = '\0';

        char timestamp[24];
        get_timestamp(timestamp);
        printf("Server [%s]: %s- %s\n", timestamp, before_space, buf);

        buf += 3;
        char new_dir[256];
        strncpy(new_dir, buf, strlen(buf));
        new_dir[strcspn(new_dir, "\n")] = 0;

        char cwd[256];
        if(getcwd(cwd, sizeof(cwd)) != NULL) {
            if(strcmp(cwd, base_dir) == 0) {
                if (strcmp(new_dir, "..") == 0) {
                    get_timestamp(timestamp);
                    send_message(socketFd, timestamp, " ERROR: Already at root directory");
                    return 1;
                }
            }
        } else {
            perror("getcwd() error");
            return 1;
        }

        if (chdir(new_dir) != 0) {
            char timestamp[24];
            get_timestamp(timestamp);
            send_message(socketFd, timestamp, " ERROR: Failed to change directory");
            return 1;
        }

        get_timestamp(timestamp);
        send_message(socketFd, timestamp, " Successfully changed directory");
        return 1;
    }
    return 0;
}

void *handle_client(void *arg) {
    int newSocketFd = *(int *)arg;
    char buffer[256];
    char timestamp[24];
    char cwd[256];
    if(getcwd(cwd, sizeof(cwd)) != NULL) {
        if(strcmp(base_dir, ".") == 0) {
            strncpy(base_dir, cwd, sizeof(base_dir));
        }
    } else {
        perror("getcwd() error");
        return NULL;
    }

    get_timestamp(timestamp);
    send_message(newSocketFd, timestamp, " Hello! I am a server");

    while(1) {
        int flag = 0;
        int flag2 = 0;
        // usleep(30000);

        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            
            if (strcmp(base_dir, cwd) == 0) {
                char message[512];
                sprintf(message, " Please enter the message: ");
                get_timestamp(timestamp);
                send_message(newSocketFd, timestamp, message);
            } else if (strstr(cwd, base_dir) == cwd) {
                char *subdir = cwd + strlen(base_dir);
                if (*subdir == '/') {
                    subdir++;
                }
                char message[512];
                sprintf(message, " %s>Please enter the message: ", subdir);
                get_timestamp(timestamp);
                send_message(newSocketFd, timestamp, message);
            }
        } else {
            perror("getcwd() error");
            return NULL;
        }

        bzero(buffer, 256);
        read(newSocketFd, buffer, 255);
        char* buf = strchr(buffer, ' ');
        if (buf) {
            buf++;
            if (strncmp(buf, "ECHO", 4) == 0) {
                flag = handle_command(newSocketFd, buffer, buf, "ECHO", " Server: I got your message", 5);
                if(flag == 1) {
                    flag2 = 1;
                }
            }
            if (strncmp(buf, "INFO", 4) == 0) {
                FILE* file = fopen("/home/mishail/VSCodePrograms/Lab8/text.txt", "r");
                if (file == NULL) {
                    perror("ERROR opening file");
                    exit(1);
                }
                char line[256];
                fgets(line, sizeof(line), file);
                fclose(file);
                flag = handle_command(newSocketFd, buffer, buf, "INFO", line, 0);
                if(flag == 1) {
                    flag2 = 1;
                }
            }
            if (strncmp(buf, "QUIT", 4) == 0) {
                flag = handle_command(newSocketFd, buffer, buf, "QUIT", " BYE!", 0);
                if(flag == 1) {
                    flag2 = 1;
                }
                close(newSocketFd);
                return NULL;
            }
            if (strncmp(buf, "LIST", 4) == 0) {
                flag = handle_command(newSocketFd, buffer, buf, "LIST", " LIST: Here is the list of files", 0);
                list_files(newSocketFd, ".");
                flag2 = 1;
            }
            if (strncmp(buf, "CD", 2) == 0) {
                flag = handle_cd_command(newSocketFd, buffer, buf);
                if(flag == 1) {
                    flag2 = 1;
                }
            }
            if(flag2 == 0) {
                char before_space[256];
                strncpy(before_space, buffer, buf - buffer);
                before_space[buf - buffer] = '\0';

                char timestamp[24];
                get_timestamp(timestamp);
                printf("Server [%s]: %s- %s\n", timestamp, before_space, buf);
                send_message(newSocketFd, timestamp, " Server: I don't understand");
            }
        }
    }

    close(newSocketFd);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr,"ERROR, no port or base directory provided\n");
        exit(1);
    }

    int socketFd, newSocketFd, portNum;
    socklen_t clientAddrLen;
    struct sockaddr_in serverAddr, clientAddr;

    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketFd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    bzero((char *) &serverAddr, sizeof(serverAddr));
    portNum = atoi(argv[1]);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(portNum);

    if(bind(socketFd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    listen(socketFd, 10);
    clientAddrLen = sizeof(clientAddr);

    strncpy(base_dir, argv[2], sizeof(base_dir));
    if (chdir(base_dir) != 0) {
        perror("ERROR changing to base directory");
        exit(1);
    }

    printf("Server: Waiting for client to connect...\n");
    while(1) {
        newSocketFd = accept(socketFd, (struct sockaddr *) &clientAddr, &clientAddrLen);
        if(newSocketFd < 0) {
            perror("ERROR on accept");
            exit(1);
        } else {
            printf("Server: Client connected\n");
        }

        pthread_t thread;
        int* newSocketFdPtr = malloc(sizeof(int));
        *newSocketFdPtr = newSocketFd;
        pthread_create(&thread, NULL, handle_client, newSocketFdPtr);
    }

    close(socketFd);
    return 0; 
}