// #define DEBUG 1

/**
 * @file notJustCats.c
 * @author Charles "Blue" Hartsell (ckharts@clemson.edu)
 * @brief Source code file for not just cats data recovery tool
 * @version 0.1
 * @date 2022-11-29
 * 
 * @copyright Copyright (c) 2022
 * 
 */

// Includes
#include "notJustFunctions.h"

int main(int argc, char *argv[]){

    // check for correct number of arguments
    if(argc != 3){
        printf("Usage: ./notjustcats <image filename> <output directory>");
        exit(EXIT_FAILURE);
    }

    // set up variables
    char *inFile = argv[1];
    char *outDir = argv[2];
    
    // allocate memory for file list
    fList = (fileList *) malloc(sizeof(fileList));
    
    // extract file data
    fileData = extFileInfo(inFile);

    // get boot sector
    getBootSec(fileData);
    
    // find the deleted files
    findFiles(fileData);

    // print the files found
    fileNode *entry = fList->head;

    // go through the directory list and print info about the file
    while(entry){
        if(entry->fileName[0] != '_'){
            if(strcmp(entry->fileName, ".") != 0 && strcmp(entry->fileName, "..") != 0) {
                printf("FILE\tNORMAL\t%s\t%d\n", entry->filePath, entry->size);
            }
        }
        else {
            printf("FILE\tDELETED\t%s\t%d\n", entry->filePath, entry->size); 
        }
        entry = entry->next;
    }

    // write recovered files to output directory
    recoverData(outDir);
    
    return(EXIT_SUCCESS);
}