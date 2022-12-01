/**
 * @file notJustFunctions.c
 * @author Charles "Blue" Hartsell (ckharts@clemson.edu)
 * @brief 
 * @version 0.1
 * @date 2022-11-30
 * 
 * @copyright Copyright (c) 2022
 * 
 */

// Includes
#include "notJustFunctions.h"

// Utility Functions
uint8_t *openFile(char *fileName) {
    // Open file
    FILE *fp = fopen(fileName, "rb");
    if(fp == NULL) {
        printf("Error: File not found\n");
        exit(EXIT_FAILURE);
    }
    if(fp == -1) {
        printf("Error: File pointer is null\n");
        exit(EXIT_FAILURE);
    }


    // Map memory
    uint8_t *memory = mmap(NULL, (SEC_SIZE * 2879), PROT_READ, MAP_PRIVATE, fileno(fp), 0);
    return memory;
}

void getBootSector(uint8_t *memory) {
    // Allocate memory for boot sector
    bootSector = (struct bootSector *) malloc(sizeof(struct bootSector));

    // Get number of FATs
    bootSector->fCount = memory[16];

    // Get number of root directory entries
    bootSector->rdCount = memory[17];

    // Get number of sectors
    bootSector->sCount = memory[19];

    // Get number of sectors in each FAT
    bootSector->secPerFat = memory[22];

    /*    boot->fatCount = (size_t)(*(file + 16) & 0x00FF); // byte 16
    boot->rootDirectoryCount = (size_t) ((*(file + 18) << 8) & 0x00FF) | (*(file + 17) & 0xFF); // bytes 18 and 17
    boot->sectorCount = (size_t)((*(file + 20) << 8) & 0x00FF) | (*(file + 19) & 0xFF); // bytes 20 and 19
    boot->sectorsInFat = (size_t)((*(file + 23) << 8) & 0x00FF) | (*(file + 22) & 0xFF); // bytes 23 and 22*/
}

void parseFileSystem(uint8_t *memory) {
    // Find the root directory
    uint8_t *rootDir = memory + R_DIR_OFFSET;

    // Loop through until we find the end of the root directory
    while(*rootDir != 0x00) {
        // Get entry in root directory
        dirEntry *entry = makeDirectory(rootDir);
        entry->filePath[0] = '/';
        strcat(entry->filePath, entry->name);

        // Get first data sector
        uint8_t *dataSec = memory + ((DATA_SEC_OFFSET + entry->firstLCluster[0] - 2) * SEC_SIZE);

        // Detect if entry is a directory
        if(entry->directory == 1) {
            // handle directory
            handleDirectory(entry, dataSec);
        } else {
            // handle file
            strcat(entry->filePath, ".");
            strcat(entry->filePath, entry->ext);

            // Get data
            entry->data = makeData(entry, dataSec);
        }

        // Iterate
        rootDir += DIR_SIZE;
    }
}

dirEntry *makeDirectory(uint8_t *fData) {
    // allocate memory for directory entry
    dirEntry *newEntry = (dirEntry *) malloc(sizeof(dirEntry));

    // allocate memory for entry data
    newEntry->name = (char *) malloc(sizeof(char *));
    newEntry->firstLCluster = 0;
    newEntry->ext = (char *) malloc(sizeof(char *));
    newEntry->attr = (uint8_t *) malloc(sizeof(uint8_t));
    newEntry->size = 0;
    newEntry->fNum = 0;
    newEntry->directory = 0;

    //max length for filePath will be 30 chars
    newEntry->filePath = (char *) malloc(MAX_FILEPATH_SIZE);
    newEntry->data = (dataEntry *) malloc(sizeof(dataEntry));
    newEntry->data->data = (char *) malloc(SEC_SIZE);
    newEntry->next = NULL; 

    // Memory is now allocated, set up directory entry
    uint8_t *temp = fData;

    char *buf = malloc(32);
    memcpy(buf, temp, 32);

    // Copy from buffer to entry struct
    memcpy(newEntry->name, buf, 8);
    memcpy(newEntry->ext, buf + 8, 3);
    memcpy(newEntry->attr, buf + 11, 1);
    memcpy(&newEntry->firstLCluster, buf + 26, 2);
    memcpy(&newEntry->size, buf + 28, 4);

    // Remove trailing spaces from name
    for(int i = 0; i < 8; i++) {
        if(newEntry->name[i] == ' ') {
            newEntry->name[i] = '\0';
        }
    }

    // Remove trailing spaces from extension
    for(int i = 0; i < 3; i++) {
        if(newEntry->ext[i] == ' ') {
            newEntry->ext[i] = '\0';
        }
    }

    // Encoding
    if((*newEntry->name & 0xff) == 0xE5){
        newEntry->name[0] = '_';
    }

    // Check if entry is a directory
    if((*newEntry->attr & ATTR_MASK)== 0x10) {
        newEntry->directory = 1;
    }

    // If it's not a directory, add to dir list
    else if (newEntry->directory == 0) {
        // Add to directory list
        if(dir == NULL) {
            dir = newEntry;
        }
        else {
            // check if file already exists
            dirEntry *temp = dir;
            while(temp->next != NULL) {
                if(strcmp(temp->name, newEntry->name) == 0) {
                    printf("Error: File already exists\n");
                    exit(EXIT_FAILURE);
                }
                temp = temp->next;
            }

            // Add to end of list
            temp->next = newEntry;
        }
    }
}

void handleDirectory(dirEntry *entry, uint8_t *dataSec) {
    // setup
    uint8_t *first = dataSec + DIR_HANDLE_OFFSET;

    // treat the first entry as if it is a root directory
    while(*first != 0x00) {
        // Get entry in root directory
        dirEntry *newEntry = makeDirectory(first);
        strcat(newEntry->filePath, entry->filePath);
        strcat(newEntry->filePath, "/");
        strcat(newEntry->filePath, newEntry->name);

        // Check if entry is a directory
        if(newEntry->directory == 1) {
            // Get first data sector
            uint8_t *dataSec = DATA_SEC_OFFSET + newEntry->firstLCluster[0] - 2;
            uint8_t *newSec = fData + ((int) dataSec * SEC_SIZE); // <- the typecast may need to be removed later

            // handle directory
            handleDirectory(newEntry, dataSec);
        } else {
            // handle file
            strcat(newEntry->filePath, ".");
            strcat(newEntry->filePath, newEntry->ext);

            // Get data sector
            uint8_t *dataSec = DATA_SEC_OFFSET + newEntry->firstLCluster[0] - 2;
            uint8_t *newSec = fData + ((int) dataSec * SEC_SIZE); // <- the typecast may need to be removed later

            newEntry->data = makeData(newEntry, dataSec);
        }

        // Iterate
        first += DIR_SIZE;
    }
}

dirEntry *makeData(dirEntry *entry, uint8_t *fData) {
    // init
    uint8_t *dataSec;
    int sec;

    // allocate memory for data entry
    dataEntry *currentSec = (dataEntry *) malloc(sizeof(dataEntry));
    currentSec->data = (char *) malloc(SEC_SIZE);

    // Get FAT entry
    uint32_t fatEntry = cluster2FAT(entry->firstLCluster[0]);

    // Check if this is the only data sector
    if(fatEntry >= 0xff8 || fatEntry == 0) {
        // Copy data
        memcpy(currentSec->data, fData, SEC_SIZE);
        currentSec->next = NULL;
        entry->data = currentSec;

        return currentSec;
    } else {
        // Multiple sectors exist
        // Search for next cluster
        memcpy(currentSec->data, fData, SEC_SIZE);

        // Init entry
        entry->list = (dataList *) malloc(sizeof(dataList));
        entry->list->head = entry->list->tail = NULL;
        entry->list->num = 0;

        
    }
}

uint32_t cluster2FAT(uint16_t cluster) {
    // Get FAT entry
    uint16_t value;
    uint32_t offset = 0x200 + (3 * cluster / 2);

    // Check if cluster is odd or even
    (cluster % 2 == 0) ? value = (0x0f & *(fData + offset + 1)) << 8 | *(fData + offset) : value = (*(fData + offset + 2) << 4) | (0xf0 & *(fData + offset + 1)) >> 4;

    return value;
}