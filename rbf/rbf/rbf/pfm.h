#ifndef _pfm_h_
#define _pfm_h_

#include <stdio.h>
#include <iostream>

#include "global.h"

using namespace std;

typedef int RC;
typedef unsigned PageNum;


#define PAGE_SIZE 4096

class FileHandle;


class PagedFileManager
{
public:
    static PagedFileManager* instance();                     // Access to the _pf_manager instance

    RC createFile    (const char *fileName);                         // Create a new file
    RC destroyFile   (const char *fileName);                         // Destroy a file
    RC openFile      (const char *fileName, FileHandle &fileHandle); // Open a file
    RC closeFile     (FileHandle &fileHandle);                       // Close a file

protected:
    PagedFileManager();                                   // Constructor
    ~PagedFileManager();                                  // Destructor

private:
    static PagedFileManager *_pf_manager;
};


class FileHandle
{
public:
    FileHandle();                                                    // Default constructor
	FileHandle(char* fileName, FILE* pFile);
	~FileHandle();                                                   // Destructor

    RC readPage(PageNum pageNum, void *data);                           // Get a specific page
    RC writePage(PageNum pageNum, const void *data);                    // Write a specific page
    RC appendPage(const void *data);                                    // Append a specific page
    unsigned getNumberOfPages();                                        // Get the number of pages in the file
	RC seekToPage(PageNum pageNum);
	RC bindFile(char* fileName, FILE* pFile);
	char* getFileName();
	FILE* getHandle();
protected:
	FILE* pFile;
	char* fileName;
	unsigned pageNum;
 };

 #endif
