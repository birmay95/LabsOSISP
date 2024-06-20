#define _POSIX_SOURCE 
#define SA_RESTART   0x10000000
#define MAX_ITERATIONS 101

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <stdbool.h>
#include <errno.h>

struct Pair {
    int first;
    int second;
};

struct Pair pair;
struct Pair const ZERO = {0,0};
struct Pair const ONE = {1,1};

int count_all = 0;

int count00 = 0;
int count01 = 0;
int count10 = 0;
int count11 = 0;

bool permissionOutput = true;

void pair_handling(int signum){
    if (pair.first == 0 && pair.second == 0)
        ++count00;
    else if (pair.first == 0 && pair.second == 1)
        ++count01;
    else if (pair.first == 1 && pair.second == 0)
        ++count10;
    else if (pair.first == 1 && pair.second == 1)
        ++count11;
    ++count_all;
}

void allow_output(int signum){
    permissionOutput = true;
}

void ban_output(int signum){
    permissionOutput = false;
}

int main(int argc, char* argv[]) {

    struct sigaction handler;

    handler.sa_handler = allow_output;
    if (sigaction(SIGUSR1, &handler, NULL) == -1) {
        fprintf(stderr, "Exception in sigaction SIGUSR1, errno - %d\n", errno);
        exit(EXIT_FAILURE);
    }

    handler.sa_handler = ban_output;
    if (sigaction(SIGUSR2, &handler, NULL) == -1) {
        fprintf(stderr, "Exception in sigaction SIGUSR2, errno - %d\n", errno);
        exit(EXIT_FAILURE);
    }

    handler.sa_handler = pair_handling;
    // handler3.sa_flags = SA_RESTART;
    if (sigaction(SIGALRM, &handler, NULL) == -1) {
        fprintf(stderr, "Exception in sigaction SIGALRM, errno - %d\n", errno);
        exit(EXIT_FAILURE);
    }

    struct timeval secondTimer;
    secondTimer.tv_sec = 0;
    secondTimer.tv_usec = 30005;

    struct itimerval timer;
    timer.it_interval = secondTimer;
    timer.it_value = secondTimer;

    if (setitimer(ITIMER_REAL, &timer, NULL)) {
        fprintf(stderr, "Exception in setitimer child, errno - %d\n", errno);
        exit(EXIT_FAILURE);
    }

    while(true) {
        pair = ZERO;
        pair = ONE;
        if (count_all > MAX_ITERATIONS && permissionOutput == true){
            printf("\nChild process: Name:%s, PPID:%d, PID:%d, {0,0}:%d, {0,1}:%d, {1,0}:%d, {1,1}:%d\n",
            argv[0], getppid(), getpid(), count00, count01, count10, count11);
            count_all = count00 = count01 = count10 = count11 = 0;
        }
    }

    return 0;
}
