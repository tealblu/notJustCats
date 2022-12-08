/**
 * @file notJustFunctions.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-12-07
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "notJustFunctions.h"

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
        
        // get first data sector
        int sector = (DATA_OFFSET + newDirectory->firstCluster - 2);
        uint8_t *dataSection = file + (sector * SEC_SIZE);

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
 * searches through and adds the data sectors from the disk image by 
 * getting the FAT entries for the curEntry until the FAT entry tells us to stop
 * 
 * FAT Entries:
 *      - >= 0xff8 - data cluster is the last one we need to find
 *      - 0x000 - data cluster is free
 *              - we will use this one to find the deleted file's data
 *      - 0x002 to 0xfef - the next data cluster that goes with the file
 *      
 */

void findSecs(fileNode *curEntry, uint8_t *file){

    uint8_t *dataSector; 
    int sector;

    fileDataNode *curDataSector = (fileDataNode *) malloc(sizeof(fileDataNode));
    curDataSector->data = (char *) malloc(SEC_SIZE);

    uint32_t fatEntry = cluster2Fat(curEntry->firstCluster);

    // this is the only data sector for the entry
    if(fatEntry >= 0xff8 || fatEntry == 0x000){
        memcpy(curDataSector->data, file, SEC_SIZE);
        curEntry->dataSection = curDataSector;

        return;
    }
    // multiple data sectors, search through the FAT to get the next cluster that holds data
    else {

        memcpy(curDataSector->data, file, SEC_SIZE);
        
        curEntry->firstDataSector = (fileDataList *) malloc(sizeof(fileDataList));
        curEntry->firstDataSector->head = curEntry->firstDataSector->tail = NULL;
        curEntry->firstDataSector->count = 0;

        curEntry->dataSection = curDataSector;
        addToFileDataList(curDataSector, curEntry);


        while(fatEntry <= 0xff8 && fatEntry != 0x000){
            fileDataNode *nextDataSector = (fileDataNode *) malloc(sizeof(fileDataNode));
            nextDataSector->data = (char *) malloc(SEC_SIZE);
            
            sector = (DATA_OFFSET + fatEntry - 2);
            dataSector = fileData + (sector * SEC_SIZE);
            memcpy(nextDataSector->data, dataSector, SEC_SIZE);

            addToFileDataList(nextDataSector, curEntry);

            fatEntry = cluster2Fat(fatEntry);
        }   
    }

    return;
}

/**
 * handles a sub directory by looking at all the entries in the directory data sector 
 * and handling them accordingly
 * 
 */

void recursiveDir(fileNode *curEntry, uint8_t *curSubDirEntry){

    //skip over the . and .. entries
    uint8_t *firstEntry = curSubDirEntry + 64;
    int sector;
    uint8_t *newDataSector;

    // treat this data sector like the root directory
    while(*firstEntry != 0x00){

        fileNode *newEntry = makeDir(firstEntry);

        strcat(newEntry->filePath, curEntry->filePath);
        strcat(newEntry->filePath, "/");
        strcat(newEntry->filePath, newEntry->fileName);
        
        //sub dir in sub dir
        if(newEntry->dir == 1){
            //compute new sector and handle sub directory        
            sector = (DATA_OFFSET + newEntry->firstCluster - 2);
            newDataSector = fileData + (sector * SEC_SIZE);

            recursiveDir(newEntry, newDataSector);
        }
        //file in sub dir
        else {
            // add the extension to the file path of this entry
            strcat(newEntry->filePath, ".");
            strcat(newEntry->filePath, newEntry->ext);

            //compute new data sector and get the data
            sector = (DATA_OFFSET + newEntry->firstCluster - 2);
            newDataSector = fileData + (sector * SEC_SIZE);
            findSecs(newEntry, newDataSector);
        }

        //move to next entry in the sub dir
        firstEntry = firstEntry + 32;
    }
    return;
}

/**
 * create and write the files and file data found to the output directory
 */

void recoverData(char *file){

    fileNode *curEntry = fList->head;
    FILE *out;    
    char *fullPath = (char *) malloc(50);
    int count = 0;
    size_t totalSize;

    // loop through the files
    while(curEntry){
        totalSize = 0;
        
        // setup the path to the output directory
        sprintf(fullPath, "%s/%s%d.%s", file, "file", count, curEntry->ext);
        count++;
        
        // open for writing binary data
        out = fopen(fullPath, "wb");
        if(!out){
            printf("error opening output directory\n");
            exit(0);
        }

        // only one data sector
        if(!curEntry->firstDataSector){
            
            for(int i = 0; i < curEntry->size; i++){
                if(totalSize < curEntry->size){
                    fprintf(out, "%c", curEntry->dataSection->data[i]);
                    totalSize += 1;
                }
            }
            
        }
        // more than one data sector
        else {
            fileDataNode *dat = curEntry->firstDataSector->head;
                
            while(dat){
                
                for(int k = 0; k < SEC_SIZE; k++){
                    // don't print the extra allocated bytes
                    if(totalSize < curEntry->size){                       
                        fprintf(out, "%c", dat->data[k]);
                        totalSize += 1;        
                    }            
                }    
                
                dat = dat->next;
            }
        }     
        
        curEntry = curEntry->next;
    }

    fclose(out);

    return;
}

/**
 * returns the fat entry for a given cluster number
 * 
 * FAT12 entries are 12 bits long, so for example:
 *      07 08 00    (in FAT)
 * 
 *      will be converted to 0x007 and 0x008, which represent the next logical cluster for 
 *      that entry
 * 
 */

uint32_t cluster2Fat(uint16_t clustNum){
    
    uint16_t val;
    uint8_t one, two;
    uint32_t offset = 0x200 + (3 * (clustNum/2));

    switch(clustNum % 2){
        case 0:
            one = *(fileData + offset);
            two = *(fileData + offset + 1);
            val = ((0x0f & two) << 8) | one;
            break;
        case 1:
            one = *(fileData + offset + 1);
            two = *(fileData + offset + 2);
            val = two << 4 | ((0xf0 & one) >> 4);
            break;
    }

    return val;
}

/**
 * adds a fileDataNode to the list of data entries for a file
 */

void addToFileDataList(fileDataNode *newEntry, fileNode *curDirectory){
    
    fileDataList *data = curDirectory->firstDataSector;

    if(!data->head){
        data->head = data->tail = newEntry;
        newEntry->next = NULL;
    }
    else {
        data->tail->next = newEntry;
        data->tail = newEntry;
        newEntry->next = NULL;
    }
    data->count++;

    return;
}

/**
 * add a directory entry to the list of entries
 * 
 * the directory list will only be files, but the file path from the disk image
 * is kept
 */

void addToFileList(fileNode *newEntry){
    
    if(!fList->head){
        fList->head = fList->tail = newEntry;
        newEntry->next = NULL;
    }
    else {
        fList->tail->next = newEntry;
        fList->tail = newEntry;
        newEntry->next = NULL;
    }

    return;
}

/**
 * checks that a directory entry file name is not . or .. file from a sub directory
 */

int checkEntry(char *fileName){

    if(strcmp(fileName, ".") != 0 && strcmp(fileName, "..") != 0){
        return 1;
    }
    else {
        return 0;
    }

}

/**
 * prints the list of files found in the disk image
 * 
 * if a found entry was marked as deleted, we note it with "DELETED",
 * otherwise the entry is noted with "NORMAL"
 */

void printFiles(){

    fileNode *curEntry = fList->head;

    // go through the directory list and print info about the file
    while(curEntry){
        if(curEntry->fileName[0] != '_'){
            if(checkEntry(curEntry->fileName) == 1){
                printf("FILE\tNORMAL\t%s\t%d\n", curEntry->filePath, curEntry->size);
            }
        }
        else {
            printf("FILE\tDELETED\t%s\t%d\n", curEntry->filePath, curEntry->size); 
        }
        curEntry = curEntry->next;
    }

    return;
}

/**
 * initializes an entry with the information from either a root directory 
 * or a sub directory entrys
 */

fileNode *makeDir(uint8_t *directoryInfo){

    fileNode *curEntry = allocDirectory();
    uint8_t *tmp = directoryInfo;

    
    char *dirBuf = malloc(32);
    memcpy(dirBuf, tmp, 32);

    // copy from buffer to entry struct
    memcpy(curEntry->fileName, dirBuf, 8); // first 8 bytes
    memcpy(curEntry->ext, (dirBuf + 8), 3); // bytes 8 - 11
    memcpy(curEntry->attr, (dirBuf + 11), 1); // byte 11
    memcpy(&curEntry->firstCluster, (dirBuf + 26), 2); // bytes 26 - 28
    memcpy(&curEntry->size, (dirBuf + 28), 4); // bytes 28 - 32

    // remove padding
    for(int i = 8; i > 0; i--){
        if(curEntry->fileName[i] == ' '){
            curEntry->fileName[i] = '\0';
        }
    }

    // remove padding
    for(int i = 3; i > 0; i--){
        if(curEntry->ext[i] == ' '){
            curEntry->ext[i] = '\0';
        }
    }

    if((*curEntry->fileName & 0xff) == 0xE5){
        curEntry->fileName[0] = '_';
    }

    // is it a directory?
    if((*curEntry->attr & ATTR_MASK) == 0x10){
        curEntry->dir = 1;
    }

    // only add it to the file list if it's not a directory, 
    // . or .. file (no duplicates)
    if(checkEntry(curEntry->fileName) == 1 && curEntry->dir == 0){
        addToFileList(curEntry);
    }

    return curEntry;
}

/**
 * allocDirectory() allocates required memory for a directory entry
 * 
 */

fileNode *allocDirectory(){

    // allocating memory for the file
    fileNode *newEntry = (fileNode *) malloc(sizeof(fileNode));

    // everyone gets some memory
    newEntry->fileName = (char *) malloc(sizeof(char *));
    newEntry->firstCluster = 0;
    newEntry->ext = (char *) malloc(sizeof(char *));
    newEntry->attr = (uint8_t *) malloc(sizeof(uint8_t));
    newEntry->size = 0;
    newEntry->num = 0;
    newEntry->dir = 0;

    //max length for filePath will be 30 chars
    newEntry->filePath = (char *) malloc(FILEPATH_SIZE);
    newEntry->dataSection = (fileDataNode *) malloc(sizeof(fileDataNode));
    newEntry->dataSection->data = (char *) malloc(SEC_SIZE);
    newEntry->next = NULL; 

    return newEntry;
}


/**
 * test function to print file info at end of execution
 * 
  
void printFinalValues(){

    fileNode *cur = fList->head;

    while(cur){
        printf("File: %s.%s Full path: %s\n", cur->fileName, cur->ext, cur->filePath);
        printf("attr: %p size: %d firstCluster: %d\n\n", *cur->attr, cur->size, cur->firstCluster);
        

        cur = cur->next;
    }


}
*/