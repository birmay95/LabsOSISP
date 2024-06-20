#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <linux/limits.h>

int compare(const struct dirent **a, const struct dirent **b) {
    return strcoll((*a)->d_name, (*b)->d_name);
}

void file_out(char *name, bool f_type, bool l_type, bool d_type, bool s_type) {
    DIR *dir = opendir(name);
    if (!dir) {
        fprintf(stderr, "opendir: %s\n", name);
        return;
    }

    struct dirent **namelist;
    int n;
    if(s_type)
        n = scandir(name, &namelist, NULL, compare);
    else
        n = scandir(name, &namelist, NULL, NULL);

    if (n == -1) {
        perror("scandir");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n; ++i) {

        if (strcmp(namelist[i]->d_name, ".") == 0 || strcmp(namelist[i]->d_name, "..") == 0) {
            continue;
        }

        char full_path[PATH_MAX];
        snprintf(full_path, PATH_MAX, "%s/%s", name, namelist[i]->d_name);

        struct stat sb;

        if (lstat(full_path, &sb) == -1) {
            perror("lstat");
            continue;
        }
        switch (sb.st_mode & __S_IFMT) {
            case __S_IFDIR:
                if (d_type)
                    printf("Directory:\t%s\n", full_path);
                file_out(full_path, f_type, l_type, d_type, s_type);
                break;
            case __S_IFLNK:
                if (l_type)
                    printf("Symbolic link:\t%s\n", full_path);
                break;
            case __S_IFREG:
                if (f_type)
                    printf("File:\t\t%s\n", full_path);
                break;
            default:
                break;
        }
    }

    if (closedir(dir) == -1) {
        fprintf(stderr, "closedir: %s\n", name);
        return;
    }
    for(int i = 0; i < n; i++)
        free(namelist[i]);
    free(namelist);
}

int main(int argc, char *argv[]) {
    bool f_type = false;
    bool l_type = false;
    bool d_type = false;
    bool s_type = false;
    int opt, option_index;

    static struct option long_options[] = {
            {"f", no_argument, 0, 'f'},
            {"l", no_argument, 0, 'l'},
            {"d", no_argument, 0, 'd'},
            {"s", no_argument, 0, 's'},
            {0, 0, 0, 0}
    };
    while ((opt = getopt_long(argc, argv, "ldfs", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'f':
                f_type = true;
                break;
            case 'l':
                l_type = true;
                break;
            case 'd':
                d_type = true;
                break;
            case 's':
                s_type = true;
                break;
            default:
                printf("Unknown option, that's why skip skip\n");
        }
    }

    if (!(f_type || l_type || d_type)) {
        f_type = true;
        d_type = true;
        l_type = true;
    }

    if (optind < argc) {
        file_out(argv[optind], f_type, l_type, d_type, s_type);
    } else {
        file_out(".", f_type, l_type, d_type, s_type);
    }

    return 0;
}
