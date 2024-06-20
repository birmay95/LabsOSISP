#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define BUFFER_SIZE 10
#define MAX_MESSAGE_SIZE 256
#define MAX_PROCESS_SIZE 100

struct Message {
    u_int8_t type;
    u_int16_t hash;
    u_int8_t size;
    char data[MAX_MESSAGE_SIZE];
};

struct MessageQueue {
    int head, tail;
    struct Message buffer[BUFFER_SIZE];
    int addedCount;
    int extractedCount;
};

int producerCount = 0;
pid_t producerMas[MAX_PROCESS_SIZE]; 
int consumerCount = 0;
pid_t consumerMas[MAX_PROCESS_SIZE]; 

sem_t *fillCount, *emptyCount;
sem_t *mutex;
int shmidForMessageQueue, shmidForFillCount, shmidForEmptyCount, shmidForMutex;

struct MessageQueue * messageQueue;

int running = 1;

u_int16_t calculateHash(const char* data, u_int8_t size) {
    u_int16_t hash = 0;
    for (u_int8_t i = 0; i < size; ++i) {
        hash = (hash + (u_int16_t)data[i]) % 256;
    }
    return hash;
}

void putMessage(struct Message* message) {

    sem_wait(emptyCount);
    sem_wait(mutex);

    (*messageQueue).buffer[(*messageQueue).head].type = (*message).type;
    (*messageQueue).buffer[(*messageQueue).head].hash = (*message).hash;
    (*messageQueue).buffer[(*messageQueue).head].size = (*message).size;
    strncpy((*messageQueue).buffer[(*messageQueue).head].data, (*message).data, message->size);

    (*messageQueue).head = ((*messageQueue).head + 1) % BUFFER_SIZE;
    (*messageQueue).addedCount++;

    sem_post(fillCount);
    sem_post(mutex);

    printf("Producer message. Total added: %d\n", (*messageQueue).addedCount);
}

struct Message getMessage() {

    sem_wait(fillCount);
    sem_wait(mutex);

    struct Message message;
    message.type = (*messageQueue).buffer[(*messageQueue).tail].type;
    message.hash = (*messageQueue).buffer[(*messageQueue).tail].hash;
    message.size = (*messageQueue).buffer[(*messageQueue).tail].size;
    strncpy(message.data, (*messageQueue).buffer[(*messageQueue).tail].data, message.size);

    (*messageQueue).tail = ((*messageQueue).tail + 1) % BUFFER_SIZE;
    (*messageQueue).extractedCount++;

    sem_post(emptyCount);
    sem_post(mutex);

    printf("Consumer message. Total extracted: %d\n", (*messageQueue).extractedCount);

    return message;
}

void producerFunc() {
    while (running) {

        struct Message message;
        message.type = 0;
        message.hash = 0;
        u_int8_t size = rand() % 257;
        while(size == 0) {
            size = rand() % 257;
        }

        int tempOfSize = (size + 3)/4;
        size = (u_int8_t)(tempOfSize * 4);

        message.size = size;
        if (size == (u_int8_t)256) {
            message.size = 0;
        }

        for (u_int8_t i = 0; i < size; ++i) {
            message.data[i] = (char)(98 + rand() % (121 - 98));
        }

        message.hash = calculateHash(message.data, size);

        putMessage(&message);

        printf("Producer message: \n"
        "message type: %d\n"
        "message hash: %d\n"
        "message size: %d\n"
        "message data: ", message.type, message.hash, message.size);
        for (int i = 0; i < message.size; ++i) {
            printf("%c", message.data[i]);
        }
        printf("\n");

        usleep(3000 * 1000);
    }
}

void consumerFunc() {
    while (running) {

        struct Message message = getMessage();

        u_int16_t calculatedHash = calculateHash(message.data, message.size);
        if (calculatedHash != message.hash) {
            fprintf(stderr, "Error: Hash mismatch!\n");
            continue;
        }

        printf("Consumer message: \n"
        "message type: %d\n"
        "message hash: %d\n"
        "message size: %d\n"
        "message data: ", message.type, message.hash, message.size);
        for (int i = 0; i < message.size; ++i) {
            printf("%c", message.data[i]);
        }
        printf("\n");

        usleep(3000 * 1000);
    }
}

void signalHandler(int signal) {
    printf("Exit requested.\n");
    running = 0;
}

void launchProducer(pid_t *pid) {
    *pid = fork();

    if (*pid == -1) {
        fprintf(stderr, "Error, Code of the error - %d\n", errno);
        exit(EXIT_FAILURE);
    } else if (*pid > 0) {
        producerMas[producerCount] = *pid;
        producerCount++;
        printf("Parent process: Producer process created with PID %d\n", *pid);
    }
    else if (*pid == 0) {
        producerFunc();
    }
}

void launchConsumer(pid_t *pid) {
    *pid = fork();

    if (*pid == -1) {
        fprintf(stderr, "Error, Code of the error - %d\n", errno);
        exit(EXIT_FAILURE);
    } else if (*pid > 0) {
        consumerMas[consumerCount] = *pid;
        consumerCount++;
        printf("Parent process: Consumer process created with PID %d\n", *pid);
    }
    else if (*pid == 0) {
        consumerFunc();
    }
}

void removeProducer(pid_t *pid) {
    if (producerCount == 0) {
        printf("No producer process to remove.\n");
    } else if (*pid > 0) {
        producerCount--;
        kill(producerMas[producerCount], SIGTERM);
        int buffer = producerMas[producerCount];
        wait(producerMas + producerCount);
        printf("Parent process: producer process removed with PID %d\n", buffer);
        producerMas[producerCount] = 0;
        printf("Remaining producer processes: %d\n", producerCount);
    }
}

void removeConsumer(pid_t *pid) {
    if (consumerCount == 0) {
        printf("No consumer process to remove.\n");
    } else if (*pid > 0) {
        consumerCount--;
        kill(consumerMas[consumerCount], SIGTERM);
        int buffer = consumerMas[consumerCount];
        wait(consumerMas + consumerCount);
        printf("Parent process: consumer process removed with PID %d\n", buffer);
        consumerMas[consumerCount] = 0;
        printf("Remaining consumer processes: %d\n", consumerCount);
    }
}

void infoAboutCondition() {
    int valueOfFill;
    sem_getvalue(fillCount, &valueOfFill);
    int valurOfEmpty;
    sem_getvalue(emptyCount, &valurOfEmpty);
    printf("Info about condition:\n"
                    "size of queue - %d\n"
                    "fill count - %d\n"
                    "empty count - %d\n"
                    "count of producers - %d\n"
                    "count of conducers - %d\n", BUFFER_SIZE, valueOfFill, valurOfEmpty, producerCount, consumerCount);
}

void allocateSharedMemory() {

    shmidForMessageQueue = shmget(IPC_PRIVATE, sizeof(struct MessageQueue), IPC_CREAT | 0666);
    if (shmidForMessageQueue == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    messageQueue = (struct MessageQueue*)shmat(shmidForMessageQueue, NULL, 0);
    if (messageQueue == (struct MessageQueue*)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    shmidForFillCount = shmget(IPC_PRIVATE, sizeof(sem_t*), IPC_CREAT | 0666);
    if (shmidForFillCount == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    fillCount = (sem_t*)shmat(shmidForFillCount, NULL, 0);
    if (fillCount == (sem_t*)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    shmidForEmptyCount = shmget(IPC_PRIVATE, sizeof(sem_t*), IPC_CREAT | 0666);
    if (shmidForEmptyCount == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    emptyCount = (sem_t*)shmat(shmidForEmptyCount, NULL, 0);
    if (emptyCount == (sem_t*)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    shmidForMutex = shmget(IPC_PRIVATE, sizeof(sem_t*), IPC_CREAT | 0666);
    if (shmidForMutex == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    mutex = (sem_t*)shmat(shmidForMutex, NULL, 0);
    if (mutex == (sem_t*)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
}

int main() {
    srand(time(NULL));
    signal(SIGINT, signalHandler);

    allocateSharedMemory();

    messageQueue->tail = 0;
    messageQueue->head = 0;
    messageQueue->addedCount = 0;
    messageQueue->extractedCount = 0;

    fillCount = sem_open("/fillCount", O_CREAT, S_IRUSR | S_IWUSR, 0);
    emptyCount = sem_open("/emptyCount", O_CREAT, S_IRUSR | S_IWUSR, BUFFER_SIZE);
    mutex = sem_open("/mutex", O_CREAT, S_IRUSR | S_IWUSR, 1);

    pid_t producerPID = 1, consumerPID = 1;
    pid_t currentPID = getpid();

    while(running && producerPID > 0 && consumerPID > 0) {

        printf( "'p' - create producer\n"
                "'d' - delete producer\n"
                "'c' - create consumer\n"
                "'r' - remove consumer\n"
                "'i' - info about condition\n"
                "'Ctrl+C' - exit program\n");

        char input;
        scanf(" %c", &input);

        if(running == 0) {
            break;
        }

        switch(input) {
            case 'p':
                launchProducer(&producerPID);
                break;
            case 'd':
                removeProducer(&producerPID);
                break;
            case 'c':
                launchConsumer(&consumerPID);
                break;
            case 'r':
                removeConsumer(&consumerPID);
                break;
            case 'i':
                infoAboutCondition();
                break;
            default:
                printf("Incorrect input, try again: \n");
        }
    }
    pid_t parentPID = getppid();
    sem_close(fillCount);
    sem_close(emptyCount);
    sem_close(mutex);
    shmdt(messageQueue);
    shmdt(fillCount);
    shmdt(emptyCount);
    shmdt(mutex);
    if (parentPID != currentPID) {
        sem_unlink("/fillCount");
        sem_unlink("/emptyCount");
        sem_unlink("/mutex");

        shmctl(shmidForMessageQueue, IPC_RMID, NULL);
        shmctl(shmidForFillCount, IPC_RMID, NULL);
        shmctl(shmidForEmptyCount, IPC_RMID, NULL);
        shmctl(shmidForMutex, IPC_RMID, NULL);
    }
}
