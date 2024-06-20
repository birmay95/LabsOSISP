#include "header.h"

void createRecords() {
    FILE *file = fopen("students.bin", "wb");
    struct record students[10] = {
        {"Student 1", "Address 1", 1},
        {"Student 2", "Address 2", 1},
        {"Student 3", "Address 3", 2},
        {"Student 4", "Address 4", 2},
        {"Student 5", "Address 5", 3},
        {"Student 6", "Address 6", 4},
        {"Student 7", "Address 7", 4},
        {"Student 8", "Address 8", 4},
        {"Student 9", "Address 9", 4},
        {"Student 10", "Address 10", 4}
    };

    fwrite(students, sizeof(struct record), 10, file);

    fclose(file);
    printf("File 'students.bin' with records created.\n");
}

void listRecords() {
    FILE *file = fopen("students.bin", "rb");
    struct record student;
    int i = 1;

    printf("List of records:\n");
    while (fread(&student, sizeof(struct record), 1, file)) {
        printf("%d. %s, %s, %d\n", i, student.name, student.address, student.semester);
        i++;
    }

    fclose(file);
}

struct record getRecord(int recordNum) {
    int fd = open("students.bin", O_RDONLY);
    struct record student;

    lseek(fd, (recordNum - 1) * sizeof(struct record), SEEK_SET);
    read(fd, &student, sizeof(struct record));

    close(fd);

    return student;
}

void putRecord(int recordNum, struct record student) {
    int fd = open("students.bin", O_RDWR);

    lseek(fd, (recordNum - 1) * sizeof(struct record), SEEK_SET);
    write(fd, &student, sizeof(struct record));

    close(fd);
}

void lockRecord(int recordNum, int fd) {
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = (recordNum - 1) * sizeof(struct record);
    lock.l_len = sizeof(struct record);

    fcntl(fd, F_SETLKW, &lock);
}

void unlockRecord(int recordNum, int fd) {
    struct flock lock;
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = (recordNum - 1) * sizeof(struct record);
    lock.l_len = sizeof(struct record);

    fcntl(fd, F_SETLK, &lock);
}

int checkInt(char *str) {
    for (int i = 0; i < (int)strlen(str); i++) {
        if (str[i] < '0' || str[i] > '9') {
            return 0;
        }
    }
    return 1;
}

void inputEl(uint8_t *temp, int min, int max) {
    char buf[20];
    fgets(buf, 20, stdin);
    buf[strcspn(buf, "\n")] = 0; // remove newline character

    while (!checkInt(buf) || (atoi(buf) < min || atoi(buf) > max)) {
        if (checkInt(buf) && (atoi(buf) < min || atoi(buf) > max)) {
            printf("Your number should be in the range from %d to %d. Try again: ", min, max);
        }
        else {
            printf("Invalid number input, try again: ");
        }

        fgets(buf, 20, stdin);
        buf[strcspn(buf, "\n")] = 0; // remove newline character
    }

    *temp = atoi(buf);
}

int menu() {
    printf("\n1. Create file with records\n");
    printf("2. List records with numbering\n");
    printf("3. Get record with number\n");
    printf("4. Change record with number and put in file\n");
    printf("5. Exit\n");
    printf("Choose option: ");
    uint8_t option;
    inputEl(&option, 1, 5);
    printf("\n");
    return option;
}

void checkExistingFile() {
    FILE *file = fopen("students.bin", "rb");
    if (file == NULL) {
        printf("File 'students.bin' not found.\n");
        createRecords();
        return;
    }
    fclose(file);
}

int equal(struct record student1, struct record student2) {
    if (strcmp(student1.name, student2.name) == 0 && strcmp(student1.address, student2.address) == 0 && student1.semester == student2.semester) {
        return 1;
    }
    return 0;
}

struct record changeStudent(uint8_t* changed, struct record student) {
    char str[80];
    printf("Enter new name(for skip input 'enter'): ");
    fgets(str, 80, stdin);
    if (str[0] != '\n') {
        strcpy(student.name, str);
        *changed = 1;
    }
    student.name[strcspn(student.name, "\n")] = 0;

    printf("Enter new address(for skip input 'enter'): ");
    fgets(str, 80, stdin);
    if (str[0] != '\n') {
        strcpy(student.address, str);
        *changed = 1;
    }
    student.address[strcspn(student.address, "\n")] = 0;

    uint8_t temp;
    printf("Enter new semester (for skip input '0'): ");
    inputEl(&temp, 0, 8);
    if (temp != 0) {
        student.semester = temp;
        *changed = 1;
    }

    return student;
}