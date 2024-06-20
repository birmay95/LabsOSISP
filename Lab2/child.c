#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

char *strdup(const char *s);

extern char **environ;

char* find_value(char** envp, char* subString)
{
    char *result = NULL;
    char *temp = NULL;

    for(int i = 0; envp[i] != NULL; ++i) {
    result = strstr(envp[i], subString);
    if(result != NULL)
        break;
    }

    char *str = strdup(result);

    if (result != NULL) {
        size_t len = strlen(subString);

        while ((temp = strstr(str, subString)) != NULL) {
        memmove(temp, temp + len, strlen(temp + len) + 1);
        }
    } 
    else {
        printf("%s is not found\n", subString);
    }

    return str;
}

int main(int argc, char *argv[], char *envp[]) {
    printf("Child process:\n");
    fprintf(stdout, "Name: %s\n", argv[0]);
    printf("PID: %d\n", getpid());
    printf("PPID: %d\n", getppid());

    if (argc < 2) {
        fprintf(stderr, "There are not enough command line variables in %s", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    
    char nameEnvp[50];

    switch(*argv[2])
    {
        case '+':
            while (fscanf(file, "%s", nameEnvp) != EOF) {
                char *valueEnvp = getenv(nameEnvp);
                if (valueEnvp != NULL) {
                    printf("%s=%s\n", nameEnvp, valueEnvp);
                } else {
                    fprintf(stderr, "Variable not found: %s\n", nameEnvp);
                }
            }
            break;
        case '*':
            while (fscanf(file, "%s", nameEnvp) != EOF) {
                char * valueEnvp = find_value(envp, nameEnvp);
                if (valueEnvp != NULL) {
                    printf("%s%s\n", nameEnvp, valueEnvp);
                } else {
                    fprintf(stderr, "Variable not found: %s\n", nameEnvp);
                }
            }
            break;
        case '&':
            while (fscanf(file, "%s", nameEnvp) != EOF) {
                char * valueEnvp = find_value(environ, nameEnvp);
                if (valueEnvp != NULL) {
                    printf("%s%s\n", nameEnvp, valueEnvp);
                }
                else {
                    fprintf(stderr, "Variable not found: %s\n", nameEnvp);
                }
            }
            break;
    }

    fclose(file);

    return 0;
}
