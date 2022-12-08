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

#define ROOT_DIR_OFFSET 0x2600
#define FAT_OFFSET 0x203
#define SECTOR_SIZE 512
#define DIRECTORY_SIZE 32
#define ATTR_MASK 0xf0
#define MAX_FILE_PATH_SIZE 30
#define DATA_SECTOR_OFFSET 33

// boot sec info
struct bootSec {
    size_t numFat;
    size_t rdCount;
    size_t numSec;
    size_t secPerFat;
};

// file entry
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

// linked list to hold files
typedef struct fileList { // directory
    struct fileNode *head;
    struct fileNode *tail;
} fileList;

// individual data sector
typedef struct fileDataNode {
    char *data;
    struct fileDataNode *next;
} fileDataNode;

// list of data sectors for files with more than one sector's worth
// of data
typedef struct fileDataList {
    int count;
    struct fileDataNode *head;
    struct fileDataNode *tail;
} fileDataList;

// globals
struct bootSec *boot;
fileList *directoryList;
uint8_t *fileData;

// I/O functions
uint8_t *extFileInfo(char *file);
void recoverData(char *file);
void printFiles();


// control functions
void getBootSec(uint8_t *file);
void findFiles(uint8_t *file);
void recursiveDir(fileNode *curEntry, uint8_t *curSubDirEntry);


// helper functions to create a directory entry 
fileNode *makeDir(uint8_t *directoryInfo);
fileNode *allocDirectory(); // <- move into makeDir


// dir list function
void addToFileList(fileNode *newEntry);


// handles data sectors
void findSecs(fileNode *curEntry, uint8_t *file);
void addToFileDataList(fileDataNode *newEntry, fileNode *curDirectory);

// other misc helper functions
uint32_t cluster2Fat(uint16_t clustNum);
int checkEntry(char *fileName); // <- move into cluster2Fat
