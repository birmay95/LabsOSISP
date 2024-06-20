#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // fork and execve
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

const int countStrings = 10;
const int sizeEachString = 250;
int childNumber = 0;
int child_status;


extern char **environ;

char *strdup(const char *s);

int compare_strings(const void *first, const void *second) {
    return strcmp(*(const char **)first, *(const char **)second);
}

char* find_value(char* const* envp, char* subString)
{
    char *result = NULL;
    char *temp = NULL;
    int i = 0;

    for(i = 0; envp[i] != NULL; ++i) {
    result = strstr(envp[i], subString);
    if(result != NULL)
        break;
    }

    while (result != NULL) {

        if ((result[-1] == 0 || result[-1] == '\n')) {
            break;
        }

        for(i = i + 1; envp[i] != NULL; ++i) {
            result = strstr(envp[i], subString);
            if(result != NULL)
            break;
        }

    }

    char *relultFinal = strdup(result);

    if (result != NULL) {
 
        size_t len = strlen(subString);

        while ((temp = strstr(relultFinal, subString)) != NULL) {
        memmove(temp, temp + len, strlen(temp + len) + 1);
        }
    } 
    else {
        printf("%s is not found\n", subString);
    }

    return relultFinal;
}

void launch_child(char* symbol, char **envp, char *argv[]) {

    pid_t childPid = fork();

    if (childPid == -1) {
        fprintf(stderr, "Error, Code of the error - %d\n", errno);
        exit(EXIT_FAILURE);
    }

    if (childPid == 0) { // Child process

        char nameOfChild[20];
        char buffer[12];

        if(childNumber < 99) {
            if(childNumber < 10)
                snprintf(buffer, sizeof(buffer), "0%d", childNumber);
            else
                snprintf(buffer, sizeof(buffer), "%d", childNumber);
            snprintf(nameOfChild, sizeof(nameOfChild), "child_%s", buffer);
        }
        else {
            fprintf(stderr, "There is more than 100 child processes\n");
            exit(EXIT_FAILURE);
        }

        FILE *file = fopen(argv[1], "r");
        if (file == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        char **abbreviatedEnvp = (char**)calloc(10, sizeof(char*));
        for(int i = 0; i < 10; i++)
            abbreviatedEnvp[i] = (char*)calloc(250, sizeof(char));

        char **abbreviatedEnviron = (char**)calloc(10, sizeof(char*));
        for(int i = 0; i < 10; i++)
            abbreviatedEnviron[i] = (char*)calloc(250, sizeof(char));

        int i = 0;

        while (fscanf(file, "%s", buffer) != EOF) {

            strcpy(abbreviatedEnvp[i], buffer);
            strcpy(abbreviatedEnviron[i], buffer);
            strcat(abbreviatedEnvp[i], "=");
            strcat(abbreviatedEnviron[i], "=");

            char *str1 = find_value(envp, abbreviatedEnvp[i]);
            char *str2 = find_value(environ, abbreviatedEnviron[i]);

            strcat(abbreviatedEnvp[i], str1);
            strcat(abbreviatedEnviron[i], str2);

            free(str1);
            free(str2);

            i++;
        }

        abbreviatedEnvp[i] = NULL;
        abbreviatedEnviron[i] = NULL;

        fclose(file);

        char * const args[] = {nameOfChild, argv[1], symbol, (char*)0};
        char* str = NULL;

        switch (*symbol) {
            case '+':

                execve(getenv("CHILD_PATH"), args, envp);
                break;
            case '*':

                str = find_value(envp, "CHILD_PATH=");

                execve(str, args, abbreviatedEnvp);
                break;
            case '&':

                str = find_value(environ, "CHILD_PATH=");

                execve(str, args, abbreviatedEnviron);
                break;
            default:
                fprintf(stderr, "Unknown symbol\n");
                exit(EXIT_FAILURE);
        }
        perror("execve");
        exit(EXIT_FAILURE);
    } 
    else { // Parent process
        childNumber++;
        wait(&child_status);
        fprintf(stdout, "Дочерний процесс завершился с кодом завершения %d\n",
        WEXITSTATUS(child_status));
    }
}

void process_input(char* symbol, char **envp, char *argv[]) {

    if (*symbol == '+' || *symbol == '*' || *symbol == '&')
        launch_child(symbol, envp, argv);
    else if (*symbol == 'q')
        exit(EXIT_SUCCESS);
    else
        fprintf(stderr, "Unknown command: %c\n", *symbol);
}

int main(int argc, char *argv[], char *envp[]) {

    if (argc < 2) {
        fprintf(stderr, "There are not enough command line variables in %s", argv[0]);
        exit(EXIT_FAILURE);
    }

    int count = 0;

    for (int i = 0; envp[i] != NULL; ++i) {
        count++;
    }

    qsort(envp, count, sizeof(char*), compare_strings);

    for (int i = 0; envp[i] != NULL; ++i) {
        printf("%s\n", envp[i]);
    }

    printf("There is variables of parent's environment\n");

    char symbol;

    while (1) {
        printf("'+' - start child process, where you can get information from environment by getenv()\n"
                "'*' - start child process, where you can get information from environment by envp\n"
                "'&' - start child process, where you can get information from environment by environ\n"
                "'q' - end the execution of the parent process\n"
                "Choose the command: ");
        scanf(" %c", &symbol);
        process_input(&symbol, envp, argv);
    }

    return 0;
}
