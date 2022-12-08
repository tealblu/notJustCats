/**
 * notjustcats.c
 * Shane Gaymon (egaymon)
 * CPSC 3320 - 002
 * Professor Jacob Sorber
 * Project 4: Recovering the Lost Bits
 * Fal 2021
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>

// offsets
#define ROOT_OFFSET 0x2600
#define FAT_OFFSET 0x203
#define DATA_OFFSET 33

// sizes
#define FILEPATH_SIZE 30
#define SEC_SIZE 512
#define DIR_SIZE 32

// masks
#define ATTR_MASK 0xf0

// structs
struct bootSec {
    size_t numFat;
    size_t rdCount;
    size_t numSec;
    size_t secPerFat;
};

typedef struct fileNode {
    char *fileName;
    char *ext;

    uint8_t *attr;
    uint16_t size;
    uint32_t firstCluster;
    int num;
    int dir;

    char *filePath;

    struct fileDataNode *dataSection;
    struct fileDataList *firstDataSector;
    struct fileNode *next;

} fileNode;

typedef struct fileList {
    struct fileNode *head;
    struct fileNode *tail;
} fileList;

typedef struct fileDataNode {
    char *data;
    struct fileDataNode *next;
} fileDataNode;

typedef struct fileDataList {
    int count;
    struct fileDataNode *head;
    struct fileDataNode *tail;
} fileDataList;

// globals
struct bootSec *boot;
fileList *fList;
uint8_t *fileData;

// core functions
void getBootSec(uint8_t *file);
void recursiveDir(fileNode *curEntry, uint8_t *curSubDirEntry); 
void findSecs(fileNode *curEntry, uint8_t *file);
uint32_t cluster2Fat(uint16_t clustNum);
void findFiles(uint8_t *file);
void recoverData(char *file);

// file functions
uint8_t *extFileInfo(char *file);
void printFiles();

// allocation functions
fileNode *makeDir(uint8_t *directoryInfo);
fileNode *allocDirectory(); // <- move into makeDir

// list functions
void addToFileList(fileNode *newEntry);
void addToFileDataList(fileDataNode *newEntry, fileNode *curDirectory);
int checkEntry(char *fileName); // <- move into cluster2Fat
