Indigo Hartsell

DESCRIPTION
    A tool that parses disk images (formatted using the FAT12 file system), and prints out information about the files that the image contains, including recoverable files that someone has tried to delete.

    To compile, open a command line in the directory that contains the source code, and type "make". To run, type "./notJustCats <disk image> <output directory>". To clean up, type "make clean".

DESIGN
    The tool parses a disk image using the FAT12 file system. It creates directory entries for each file, and it stores those entries in a linked list.
    The tool starts by opening the file and finding the boot sector. After that, it adds all files in the root directory to the linked list. It recursivley
    runs through subdirectories, adding all their files to the linked list too. After that, it prints out all the files in the linked list, and it recovers
    any files that have been deleted.

KNOWN PROBLEMS
    The tool currently has issues recovering large files such as images and long text documents. Currently trying to find the source of the issue.
