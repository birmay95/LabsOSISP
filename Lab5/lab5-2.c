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
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>

#define MAX_MESSAGE_SIZE 256
int bufferSize = 10;

struct Message {
    u_int8_t type;
    u_int16_t hash;
    u_int8_t size;
    char data[MAX_MESSAGE_SIZE];
};

struct MessageQueue {
    int head, tail;
    struct Message* buffer;
    int addedCount;
    int extractedCount;
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condForFill = PTHREAD_COND_INITIALIZER;
pthread_cond_t condForEmpty = PTHREAD_COND_INITIALIZER;
int fillCount;
int emptyCount;

struct MessageQueue messageQueue;

int producerCount = 0;
pthread_t *producerMas; 
int consumerCount = 0;
pthread_t *consumerMas; 
int* runningForProd;
int* runningForCons;
int flagForExit = 0;

u_int16_t calculateHash(const char* data, u_int8_t size) {
    u_int16_t hash = 0;
    for (u_int8_t i = 0; i < size; ++i) {
        hash = (hash + (u_int16_t)data[i]) % 256;
    }
    return hash;
}

void putMessage(struct Message* message, int* running, int* flag) {

    pthread_mutex_lock(&mutex);
    while(*running && fillCount >= bufferSize) {
        pthread_cond_wait(&condForEmpty, &mutex);
    }

    if(fillCount < bufferSize) {
        *flag = 1;
        fillCount++;
        emptyCount--;
        messageQueue.buffer[messageQueue.head].type = (*message).type;
        messageQueue.buffer[messageQueue.head].hash = (*message).hash;
        messageQueue.buffer[messageQueue.head].size = (*message).size;
        strncpy(messageQueue.buffer[messageQueue.head].data, (*message).data, message->size);

        messageQueue.head = (messageQueue.head + 1) % bufferSize;
        messageQueue.addedCount++;

        pthread_cond_signal(&condForFill);

        printf("Producer message. Total added: %d\n", messageQueue.addedCount);
    }
    pthread_mutex_unlock(&mutex);
}

struct Message getMessage(int* running, int* flag) {

    pthread_mutex_lock(&mutex);
    while(*running && fillCount <= 0) {
        pthread_cond_wait(&condForFill, &mutex);
    }

    struct Message message;

    if (fillCount > 0) {
        *flag = 1;
        fillCount--;
        emptyCount++;
        message.type = messageQueue.buffer[messageQueue.tail].type;
        message.hash = messageQueue.buffer[messageQueue.tail].hash;
        message.size = messageQueue.buffer[messageQueue.tail].size;
        strncpy(message.data, messageQueue.buffer[messageQueue.tail].data, message.size);

        messageQueue.tail = (messageQueue.tail + 1) % bufferSize;
        messageQueue.extractedCount++;

        pthread_cond_signal(&condForEmpty);

        printf("Consumer message. Total extracted: %d\n", messageQueue.extractedCount);
    }
    pthread_mutex_unlock(&mutex);

    return message;
}

void* producerFunc(void* arg) {

    int* running = (int*)arg;

    while (*running) {

        int flag = 0;

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

        putMessage(&message, running, &flag);

        if(flag == 0) {
            break;
        }

        printf("Producer message: \n"
        "message type: %d\n"
        "message hash: %d\n"
        "message size: %d\n"
        "message data: ", message.type, message.hash, message.size);
        for (int i = 0; i < message.size; ++i) {
            printf("%c", message.data[i]);
        }
        printf("\n");

        sleep(2);

    }
    return NULL;
}

void* consumerFunc(void* arg) {

    int* running = (int*)arg;

    while (*running) {

        int flag = 0;

        struct Message message = getMessage(running, &flag);

        if (flag == 0) {
            break;
        }

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

        sleep(2);

    }
    return NULL;
}

void signalHandler(int signal) {
    printf("\nExit requested.\n");
    for(int i = producerCount - 1; i >= 0; i--) {
        runningForProd[i] = 0;
        pthread_cond_broadcast(&condForEmpty);
        pthread_join(producerMas[i], NULL);
    }
    for(int i = consumerCount - 1; i >= 0; i--) {
        runningForCons[i] = 0;
        pthread_cond_broadcast(&condForFill);
        pthread_join(consumerMas[i], NULL);
    }
    flagForExit = 1;
}

void launchProducer() {

    producerCount++;
    if (producerCount > 1) {
        producerMas = (pthread_t*)realloc(producerMas, producerCount * sizeof(pthread_t));
        runningForProd = (int*)realloc(runningForProd, producerCount * sizeof(int));
    } else {
        producerMas = (pthread_t*)malloc(producerCount * sizeof(pthread_t));
        runningForProd = (int*)malloc(producerCount * sizeof(int));
    }
    runningForProd[producerCount - 1] = 1;

    if (pthread_create(&producerMas[producerCount - 1], NULL, producerFunc, (void*)&runningForProd[producerCount - 1])) {
        perror("pthread_create_producer");
        exit(EXIT_FAILURE);
    }
}

void launchConsumer() {
    consumerCount++;
    if (consumerCount > 1) {
        consumerMas = (pthread_t*)realloc(consumerMas, consumerCount * sizeof(pthread_t));
        runningForCons = (int*)realloc(runningForCons, consumerCount * sizeof(int));
    } else {
        consumerMas = (pthread_t*)malloc(consumerCount * sizeof(pthread_t));
        runningForCons = (int*)malloc(consumerCount * sizeof(int));
    }
    runningForCons[consumerCount - 1] = 1;

    if (pthread_create(&consumerMas[consumerCount - 1], NULL, consumerFunc, (void*)&runningForCons[consumerCount - 1])) {
        perror("pthread_create_consumer");
        exit(EXIT_FAILURE);
    }
}

void removeProducer() {
    if (producerCount == 0) {
        printf("No producer process to remove.\n");
    } else {
        producerCount--;
        runningForProd[producerCount] = 0;
        pthread_cond_broadcast(&condForEmpty);
        pthread_join(producerMas[producerCount], NULL);
        if (producerCount > 0) {
            producerMas = (pthread_t*)realloc(producerMas, producerCount * sizeof(pthread_t));
            runningForProd = (int*)realloc(runningForProd, producerCount * sizeof(int));
        } else {
            free(producerMas);
            free(runningForProd);
        }
        printf("Parent process: last producer process removed\n");
        printf("Remaining producer processes: %d\n", producerCount);
    }
}

void removeConsumer() {
    if (consumerCount == 0) {
        printf("No consumer process to remove.\n");
    } else {
        consumerCount--;
        runningForCons[consumerCount] = 0;
        pthread_cond_broadcast(&condForFill);
        pthread_join(consumerMas[consumerCount], NULL);
        if (consumerCount > 0) {
            consumerMas = (pthread_t*)realloc(consumerMas, consumerCount * sizeof(pthread_t));
            runningForCons = (int*)realloc(runningForCons, consumerCount * sizeof(int));
        } else {
            free(consumerMas);
            free(runningForCons);
        }
        printf("Parent process: last consumer process removed\n");
        printf("Remaining consumer processes: %d\n", consumerCount);
    }
}

void infoAboutCondition() {
    printf("Info about condition:\n"
                    "size of queue - %d\n"
                    "fill count - %d\n"
                    "empty count - %d\n"
                    "count of producers - %d\n"
                    "count of conducers - %d\n", bufferSize, fillCount, emptyCount, producerCount, consumerCount);
}

void updateSizeOfBuffer() {
    int temp = bufferSize;
    printf("Input new size of buffer:\n");
    scanf("%d", &bufferSize);
    if(bufferSize < fillCount) {
        printf("Error input, you can't change size of queue on this value, because there is less than number of messages in queue\n"
                        "Nothing is changed\n");
        bufferSize = temp;
        return;
    }
    messageQueue.buffer = (struct Message*)realloc(messageQueue.buffer, bufferSize * sizeof(struct Message));
    if(messageQueue.head == 0 && messageQueue.buffer[messageQueue.head].size != 0) {
        messageQueue.head = temp;
    }
    pthread_cond_broadcast(&condForEmpty);
}

int main() {
    srand(time(NULL));
    signal(SIGINT, signalHandler);

    messageQueue.tail = 0;
    messageQueue.head = 0;
    messageQueue.addedCount = 0;
    messageQueue.extractedCount = 0;
    messageQueue.buffer = (struct Message*)malloc(bufferSize * sizeof(struct Message));
    fillCount = 0;
    emptyCount = bufferSize;

    while(1) {

        printf( "'p' - create producer\n"
                "'d' - delete producer\n"
                "'c' - create consumer\n"
                "'r' - remove consumer\n"
                "'i' - info about condition\n"
                "'u' - update size of buffer\n"
                "'Ctrl+C' - exit program\n");

        char input;
        scanf(" %c", &input);

        if(flagForExit == 1) {
            break;
        }

        switch(input) {
            case 'p':
                launchProducer();
                break;
            case 'd':
                removeProducer();
                break;
            case 'c':
                launchConsumer();
                break;
            case 'r':
                removeConsumer();
                break;
            case 'i':
                infoAboutCondition();
                break;
            case 'u':
                updateSizeOfBuffer();
                break;
            default:
                printf("Incorrect input, try again: \n");
        }
    }
    if (producerCount > 0) {
        free(producerMas);
        free(runningForProd);
    }
    if (consumerCount > 0) {
        free(consumerMas);
        free(runningForCons);
    }
    free(messageQueue.buffer);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&condForEmpty);
    pthread_cond_destroy(&condForEmpty);
}
