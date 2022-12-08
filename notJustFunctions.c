/**
 * @file notJustFunctions.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-12-07
 * 
 * @copyright Copyright (c) 2022
 * 
 * 
 */

#include "notJustFunctions.h"

/**
 * @brief Build the Boot Sec object
 * 
 * @param file The file to get info from
 */
void getBootSec(uint8_t *file){
    boot = malloc(sizeof(struct bootSec));
    
    boot->numFat = (size_t) (*(file + 16) & 0x00FF);
    boot->rdCount = (size_t) ((*(file + 18) << 8) & 0x00FF) | (*(file + 17) & 0xFF);
    boot->numSec = (size_t) ((*(file + 20) << 8) & 0x00FF) | (*(file + 19) & 0xFF);
    boot->secPerFat = (size_t) ((*(file + 23) << 8) & 0x00FF) | (*(file + 22) & 0xFF);
}

/**
 * @brief Recursively find the deleted files
 * 
 * @param entry 
 * @param subEntry 
 */
void recursiveDir(fileNode *entry, uint8_t *subEntry){
    // get the first entry
    uint8_t *firstEntry = subEntry + DIR_OFFSET;

    // loop until we reach the end of the sub directory
    while(*firstEntry != 0x00){
        // make a new entry
        fileNode *newEntry = makeDir(firstEntry);

        // add the file name
        strcat(newEntry->filePath, entry->filePath);
        strcat(newEntry->filePath, "/");
        strcat(newEntry->filePath, newEntry->fileName);
        
        // another directory in the sub directory
        if(newEntry->dir == 1){
            // get the new data sec  
            int sec = (DATA_OFFSET + newEntry->firstCluster - 2);
            uint8_t *newSec = fileData + (sec * SEC_SIZE);

            // recursively call this function
            recursiveDir(newEntry, newSec);
        } else {
            // add the extension
            strcat(newEntry->filePath, ".");
            strcat(newEntry->filePath, newEntry->ext);

            // get the data sec
            int sec = (DATA_OFFSET + newEntry->firstCluster - 2);
            uint8_t *newSec = fileData + (sec * SEC_SIZE);
            findSecs(newEntry, newSec);
        }

        // iterate
        firstEntry += DIR_SIZE;
    }
}

/**
 * @brief Find sectors
 * 
 * @param entry
 * @param file 
 */
void findSecs(fileNode *entry, uint8_t *file){
    // make a new node
    fileDataNode *sectorNode = (fileDataNode *) malloc(sizeof(fileDataNode));
    sectorNode->data = (char *) malloc(SEC_SIZE);

    // get the first fat entry
    uint32_t fatEntry = cluster2Fat(entry->firstCluster);

    // Check if there is only one data sec
    if(fatEntry >= 0xff8 || fatEntry == 0x000){
        // only one data sec
        memcpy(sectorNode->data, file, SEC_SIZE);
        entry->dataSection = sectorNode;
    } else {
        // more than one data sec
        memcpy(sectorNode->data, file, SEC_SIZE);
        
        // set up entry
        entry->firstDataSector = (fileDataList *) malloc(sizeof(fileDataList));
        entry->firstDataSector->head = entry->firstDataSector->tail = NULL;
        entry->firstDataSector->count = 0;
        entry->dataSection = sectorNode;

        // add the node to the list
        addToFileDataList(sectorNode, entry);


        while(fatEntry <= 0xff8 && fatEntry != 0x000){
            // make a new node
            fileDataNode *nextSectorNode = (fileDataNode *) malloc(sizeof(fileDataNode));
            nextSectorNode->data = (char *) malloc(SEC_SIZE);
            
            // get the next data sec
            int sec = (DATA_OFFSET + fatEntry - 2);
            uint8_t *dataSec = fileData + (sec * SEC_SIZE);
            memcpy(nextSectorNode->data, dataSec, SEC_SIZE);

            // add the node to the list
            addToFileDataList(nextSectorNode, entry);

            // get the next fat entry
            fatEntry = cluster2Fat(fatEntry);
        }   
    }
}

/**
 * @brief Get the fat value for a cluster
 * 
 * @param cluster 
 * @return uint32_t 
 */
uint32_t cluster2Fat(uint16_t cluster){
    // get the offset
    uint32_t offset = 0x200 + (3 * (cluster/2));

    // check if the cluster is even or odd
    if(cluster % 2 == 0) {
        return ((0x0f & *(fileData + offset + 1)) << 8) | *(fileData + offset);
    } else return *(fileData + offset + 2) << 4 | ((0xf0 & *(fileData + offset + 1)) >> 4);
}

/**
 * @brief Iteratively find the deleted files
 * 
 * @param file The file to get info from
 */
void findFiles(uint8_t *file){

    // get the root directory
    uint8_t *rootDirectory = file + ROOT_OFFSET;

    // loop until we reach the end of the root directory
    while(*rootDirectory != 0x00){
        fileNode *newDirectory = makeDir(rootDirectory);
        newDirectory->filePath[0] = '/';
        strcat(newDirectory->filePath, newDirectory->fileName);
        
        // get first data sec
        int sec = (DATA_OFFSET + newDirectory->firstCluster - 2);
        uint8_t *dataSection = file + (sec * SEC_SIZE);

        // check if entry is directory
        if(newDirectory->dir == 1){
            // entry is a directory
            recursiveDir(newDirectory, dataSection); 
        }
        else {
            // entry is a file
            strcat(newDirectory->filePath, ".");
            strcat(newDirectory->filePath, newDirectory->ext);

            // find the data sectors
            findSecs(newDirectory, dataSection);
        }
        
        // iterate
        rootDirectory += DIR_SIZE;
    }
}

/**
 * @brief Write recovered files to the output directory
 * 
 * @param file 
 */
void recoverData(char *file){

    fileNode *entry = fList->head;   
    char *path = (char *) malloc(50);
    int count = 0;
    size_t size;

    // loop through the list
    FILE *out;
    while(entry){
        size = 0;
        
        // get the path
        sprintf(path, "%s/%s%d.%s", file, "file", count, entry->ext);
        count++;
        
        // open the file
        out = fopen(path, "wb");
        if(!out){
            printf("Could not open output dir\n");
            exit(EXIT_FAILURE);
        }

        // check if there is only one data sec
        if(!entry->firstDataSector){
            // only one data sec exists
            for(int i = 0; i < entry->size; i++){
                if(size < entry->size){
                    // write to the file with fwrite
                    fwrite(&entry->dataSection->data[i], sizeof(char), 1, out);
                    size++;
                }
            }
            
        } else {
            fileDataNode *data = entry->firstDataSector->head;
                
            while(data) {
                
                for(int k = 0; k < SEC_SIZE; k++){
                    // don't print the extra allocated bytes
                    if(size < entry->size){        
                        // write to the file with fwrite
                        fwrite(&data->data[k], sizeof(char), 1, out);
                        size++;        
                    }            
                }    
                // iterate
                data = data->next;
            }
        }     
        // iterate
        entry = entry->next;
    }

    // close the file
    fclose(out);
}

/**
 * @brief Extracts file info
 * 
 * @param file The file to get info from
 * @return uint8_t* Pointer to data
 */
uint8_t *extFileInfo(char *file){
    FILE *fp = fopen(file, "r");
    if(!fp) {
        printf("fp is null!\n");
        exit(EXIT_FAILURE);
    }

    return (uint8_t *) mmap(NULL, (SEC_SIZE * 2879), PROT_READ, MAP_PRIVATE, fileno(fp), 0);
}

/**
 * @brief Sets up directory
 * 
 * @param directory 
 * @return fileNode* 
 */
fileNode *makeDir(uint8_t *directory){
    // allocating memory for the entry
    fileNode *entry = (fileNode *) malloc(sizeof(fileNode));

    // everyone gets some memory
    entry->fileName = (char *) malloc(sizeof(char *));
    entry->firstCluster = 0;
    entry->ext = (char *) malloc(sizeof(char *));
    entry->attr = (uint8_t *) malloc(sizeof(uint8_t));
    entry->size = 0;
    entry->num = 0;
    entry->dir = 0;

    //max length for filePath will be 30 chars
    entry->filePath = (char *) malloc(FILEPATH_SIZE);
    entry->dataSection = (fileDataNode *) malloc(sizeof(fileDataNode));
    entry->dataSection->data = (char *) malloc(SEC_SIZE);
    entry->next = NULL; 

    // copy the directory to a buffer
    uint8_t *temp = directory;
    char *buf = malloc(DIR_SIZE);
    memcpy(buf, temp, DIR_SIZE);

    // copy from buffer to entry struct
    memcpy(entry->fileName, buf, 8);
    memcpy(entry->ext, (buf + 8), 3);
    memcpy(entry->attr, (buf + 11), 1);
    memcpy(&entry->firstCluster, (buf + 26), 2);
    memcpy(&entry->size, (buf + 28), 4);

    // formatting
    for(int i = 8; i > 0; i--){
        if(entry->fileName[i] == ' '){
            entry->fileName[i] = '\0';
        }
    }
    for(int i = 3; i > 0; i--){
        if(entry->ext[i] == ' '){
            entry->ext[i] = '\0';
        }
    }
    if((*entry->fileName & 0xff) == 0xE5){
        entry->fileName[0] = '_';
    }

    // check if it's a directory
    if((*entry->attr & ATTR_MASK) == 0x10){
        entry->dir = 1;
    }

    // add to the list if it's not a directory
    if(entry->dir == 0 && strcmp(entry->fileName, ".") != 0 && strcmp(entry->fileName, "..") != 0){
        if(!fList->head){
            fList->head = fList->tail = entry;
            entry->next = NULL;
        } else {
            fList->tail->next = entry;
            fList->tail = entry;
            entry->next = NULL;
        }
    }

    return entry;
}

/**
 * @brief Add a file node to the list
 * 
 * @param entry 
 */
void addToFileList(fileNode *entry){
    // add the node to the list
    if(!fList->head){
        fList->head = fList->tail = entry;
        entry->next = NULL;
    } else {
        fList->tail->next = entry;
        fList->tail = entry;
        entry->next = NULL;
    }
}

/**
 * @brief Add a file data node to the list
 * 
 * @param entry 
 * @param dir 
 */
void addToFileDataList(fileDataNode *entry, fileNode *dir) {
    // get the first data sec
    fileDataList *data = dir->firstDataSector;

    // add the node to the list
    if(!data->head){
        data->head = data->tail = entry;
        entry->next = NULL;
    } else {
        data->tail->next = entry;
        data->tail = entry;
        entry->next = NULL;
    }

    // increment the count
    data->count++;
}