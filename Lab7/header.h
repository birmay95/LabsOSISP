#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

struct record {
    char name[80];
    char address[80];
    uint8_t semester;
};

void createRecords();

void listRecords();

struct record getRecord(int recordNum);

void putRecord(int recordNum, struct record student);

void lockRecord(int recordNum, int fd);

void unlockRecord(int recordNum, int fd);

int checkInt(char *str);

void inputEl(uint8_t *temp, int min, int max);

int menu();

void checkExistingFile();

int equal(struct record student1, struct record student2);

struct record changeStudent(uint8_t* changed, struct record student);