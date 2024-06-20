#include "header.h"

int main() {
    checkExistingFile();
    int option;
    while (1) {    
        option = menu();
        switch (option) {
            case 1:
                createRecords();
                break;
            case 2:
                listRecords();
                break;
            case 3: 
                printf("Enter record number: ");
                uint8_t recordNum;
                inputEl(&recordNum, 1, 10);
                struct record student = getRecord(recordNum);
                printf("Got record: %s, %s, %d\n", student.name, student.address, student.semester);
                break;
            case 4:
                printf("Enter record number: ");
                inputEl(&recordNum, 1, 10);
                student = getRecord(recordNum);
                printf("Got record: %s, %s, %d\n", student.name, student.address, student.semester);
                struct record studentCop = student;
                uint8_t changed = 0;

                student = changeStudent(&changed, student);
                printf("Changed record: %s, %s, %d\n", student.name, student.address, student.semester);

                if (changed) {
                    int fd = open("students.bin", O_RDWR);
                    lockRecord(recordNum, fd);
                    printf("Record locked.\n");
                    struct record studentNew = getRecord(recordNum);
                    printf("Copy at first record: %s, %s, %d\n", studentCop.name, studentCop.address, studentCop.semester);
                    printf("Get now record: %s, %s, %d\n", studentNew.name, studentNew.address, studentNew.semester);
                    if(equal(studentCop, studentNew) == 0) {
                        unlockRecord(recordNum, fd);
                        printf("Record was unlocked, but changed by another user. Try again.\n");
                        close(fd);
                        break;
                    }
                    putRecord(recordNum, student);
                    printf("Record changed on this: %s, %s, %d.\n", student.name, student.address, student.semester);
                    unlockRecord(recordNum, fd);
                    printf("Record unlocked.\n");
                    close(fd);
                } else {
                    printf("Record not changed, that's why it isn't putted in file.\n");
                }
                break;
            case 5:
                return 0;
        }
    }

    return 0;
}