#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>

int child_count = 0;
pid_t childPidMas[100]; 


void remove_all_children() {
    printf("Parent process: Removing all child processes.\n");
    int size_child = child_count;
    for (int i = 0; i < size_child; i++) {

        child_count--;
        kill(childPidMas[child_count], SIGTERM);
        int buffer = childPidMas[child_count];
        wait(childPidMas + child_count);
        printf("Parent process: Child process removed with PID %d\n", buffer);
        childPidMas[child_count] = 0;
        printf("Remaining child processes: %d\n", child_count);
    }
}

void launch_child(pid_t *pid) {
    *pid = fork();

    if (*pid == -1) {
        fprintf(stderr, "Error, Code of the error - %d\n", errno);
        exit(EXIT_FAILURE);
    } else if (*pid > 0) {
        childPidMas[child_count] = *pid;
        child_count++;
        printf("Parent process: Child process created with PID %d\n", *pid);
    }
    else if (*pid == 0) {
        char nameOfChild[20];
        char buffer[12];

        if(child_count < 99) {
            if(child_count < 10)
                snprintf(buffer, sizeof(buffer), "0%d", child_count);
            else
                snprintf(buffer, sizeof(buffer), "%d", child_count);
            snprintf(nameOfChild, sizeof(nameOfChild), "child_%s", buffer);
        }
        else {
            fprintf(stderr, "There is more than 100 child processes\n");
            exit(EXIT_FAILURE);
        }
        execl("./child", nameOfChild, NULL);
    }
}

void remove_child(pid_t *pid) {
    if (child_count == 0) {
        printf("No child process to remove.\n");
    } else if (*pid > 0) {
        child_count--;
        kill(childPidMas[child_count], SIGTERM);
        int buffer = childPidMas[child_count];
        wait(childPidMas + child_count);
        printf("Parent process: Child process removed with PID %d\n", buffer);
        childPidMas[child_count] = 0;
        printf("Remaining child processes: %d\n", child_count);
    }
}

void output_children() {
    printf("Parent process: %d\n", getpid());
    printf("Child processes:\n");
    for (int i = 0; i < child_count; i++)
        printf("%d\t", childPidMas[i]);
    printf("\n");
}

void allow_all_children_output() {
    for(int i = 0; i < child_count; i++){
        kill(childPidMas[i], SIGUSR1);
    }
}

void ban_all_children_output() {
    for(int i = 0; i < child_count; i++){
        kill(childPidMas[i], SIGUSR2);
    }
}

void allow_all_children_output_for_signal(int signum) {
    for(int i = 0; i < child_count; i++){
        kill(childPidMas[i], SIGUSR1);
    }
}

void handling_pressing_p(int num) {

    ban_all_children_output();
    if(num < child_count) {

        struct timeval secondTimer;
        secondTimer.tv_sec = 5;
        secondTimer.tv_usec = 0;

        struct itimerval timer;
        timer.it_value = secondTimer;

        secondTimer.tv_sec = 0;

        timer.it_interval = secondTimer;
        if (setitimer(ITIMER_REAL, &timer, NULL)) {
            fprintf(stderr, "Exception in setitimer parent, errno - %d\n", errno);
            exit(EXIT_FAILURE);
        }

        struct sigaction handler;
        handler.sa_handler = allow_all_children_output_for_signal;
        if (sigaction(SIGALRM, &handler, NULL) == -1) {
            fprintf(stderr, "Exception in sigaction SIGALRM, errno - %d\n", errno);
            exit(EXIT_FAILURE);
        }

        char method;

        printf("Can child_%d output its statistic? Press 'g', if yes: ", num);
        scanf(" %c", &method);

        if(method == 'g') {
            kill(childPidMas[num], SIGUSR1);
            handler.sa_handler = SIG_IGN;
            if (sigaction(SIGALRM, &handler, NULL) == -1) {
                fprintf(stderr, "Exception in sigaction SIGALRM, errno - %d\n", errno);
                exit(EXIT_FAILURE);
            }
        }
    }
}

int main() {
    pid_t pid = 1;

    while (true) {
        if(pid > 0) {
            char input[5];
            int num = -1;
            printf( "'+' - create child process\n"
                    "'-' - remove child process\n"
                    "'l' - output processes\n"
                    "'k' - remove all child processes\n"
                    "'s' - ban statistic for all child processes\n"
                    "'g' - allow statistic for all child processes\n"
                    "'s + num' - ban statistic for child process with num\n"
                    "'g + num' - allow statistic for all child process with num\n"
                    "'p + num' - allow statistic for only child process with num\n"
                    "'q' - remove all child processes and exit\n"
                    "Choose the command: ");
            fgets(input, 4, stdin);

            switch (input[0]) {
                case '+':
                    launch_child(&pid);
                    break;
                case '-':
                    remove_child(&pid);
                    break;
                case 'l':
                    output_children();
                    break;
                case 'k':
                    remove_all_children();
                    printf("Parent process: All child processes removed\n");
                    child_count = 0;
                    break;
                case 's':
                    if(input[1] != '\n' && input[1] >= '0' && input[1] <= '9') {
                        num = atoi(input + 1);
                        if(num < child_count)
                            kill(childPidMas[num], SIGUSR2);
                    } else {
                        ban_all_children_output();
                    }
                    break;
                case 'g':
                    if(input[1] != '\n' && input[1] >= '0' && input[1] <= '9') {
                        num = atoi(input + 1);
                        if(num < child_count)
                            kill(childPidMas[num], SIGUSR1);
                    } else
                        allow_all_children_output();
                    break;
                case 'p':
                    if(input[1] != '\n' && input[1] >= '0' && input[1] <= '9') {
                        num = atoi(input + 1);
                        handling_pressing_p(num);
                    }
                    break;
                case 'q':
                    remove_all_children();
                    printf("Parent process: All child processes removed. Exiting...\n");
                    return 0;
                default:
                    printf("Invalid input. Try again: ");
            }
        }
    }
}