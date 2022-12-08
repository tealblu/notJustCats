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

    if(argc != 3){
        printf("Wrong number of arguments!\nUsage: ./notjustcats <image filename> <output directory>");
        exit(0);
    }

    char *in = argv[1];
    char *out = argv[2];
    
    directoryList = (fileList *) malloc(sizeof(fileList));
    
    fileData = extFileInfo(in);
    getBootSec(fileData);
    
    findFiles(fileData);

    printFiles();
    recoverData(out);
    
    return 0;
}