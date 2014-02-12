#define _CRT_SECURE_NO_DEPRECATE 
#define _CRT_NONSTDC_NO_DEPRECATE
 
#include "pfm.h"


PagedFileManager* PagedFileManager::_pf_manager = 0;

PagedFileManager* PagedFileManager::instance()
{
    if(!_pf_manager)
        _pf_manager = new PagedFileManager();

    return _pf_manager;
}


PagedFileManager::PagedFileManager()
{
}


PagedFileManager::~PagedFileManager()
{
}


RC PagedFileManager::createFile(const char *fileName)
{
	//see if the file already exists
	FILE* pFile;
	pFile = fopen(fileName, "rb");
	//if not exists, create new file
	if(pFile == NULL) 
	{
		pFile = fopen(fileName, "w+b");
		if( pFile == NULL ) 
		{
			return RC_FILE_CREATION_FAIL;
		} 
		else 
		{
			return fclose(pFile); // 0 is returned if the file is successfully closed, otherwise, EOF is returned
		}
	} 
	else 
	{   
		int errCode = fclose(pFile);
		if(errCode != 0 ) 
		{
			return RC_FILE_CLOSE_FAIL;
		}
		return RC_FILE_EXISTED;
	}
}


RC PagedFileManager::destroyFile(const char *fileName)
{
    FILE* pFile = fopen( fileName, "rb" );
	if( pFile == NULL ) {
		return RC_FILE_NOT_EXISTED;
	} else {
		//close before destroy
		int errCode = fclose(pFile);
		if(errCode != 0 ) 
			return RC_FILE_CLOSE_FAIL;
		errCode = remove(fileName);
		if( errCode == 0 )
		{ return RC_SUCCESS;}
		else
		{ return RC_FILE_DESTROY_FAIL;}
		
	}
}


RC PagedFileManager::openFile(const char *fileName, FileHandle &fileHandle)
{
    FILE* pFile = fopen(fileName, "rb");

	if( pFile == NULL ) {
		return RC_FILE_NOT_EXISTED;
	} else {
		int errCode = fclose(pFile);
		if(errCode != 0 ) 
			return RC_FILE_CLOSE_FAIL;
		pFile = fopen(fileName, "r+b");
		if( pFile == NULL ) {
			return RC_FILE_OPEN_FAIL;
		} else {
			return fileHandle.bindFile( (char* )fileName, pFile);
		}
	}
}


RC PagedFileManager::closeFile(FileHandle &fileHandle)
{
	int errCode = fclose(fileHandle.getHandle());

	if(errCode != 0 ) {
		return RC_FILE_CLOSE_FAIL;
	} else {
		return RC_SUCCESS;
	}
}


FileHandle::FileHandle()
{
	this->fileName = NULL;
	this->pFile = NULL;
	this->pageNum = 0;
}
FileHandle::FileHandle(char* fileName, FILE* pFile)
{
	this->fileName = fileName;
	this->pFile = pFile;
	this->pageNum = 0;
}

//todo: memory leak!
FileHandle::~FileHandle()
{
	//delete fileName;
}


RC FileHandle::readPage(PageNum pageNum, void *data)
{
	fseek(pFile, pageNum * PAGE_SIZE, SEEK_SET);
	int errCode = fread(data, sizeof(char), PAGE_SIZE, pFile); //  total number of elements successfully read would be returned

	if (errCode == PAGE_SIZE) 
		return RC_SUCCESS;
	else
		return RC_FILE_READ_FAIL;

	return RC_SUCCESS;
}


RC FileHandle::writePage(PageNum pageNum, const void *data)
{
	//find the pageNum th page
	fseek(pFile, pageNum * PAGE_SIZE, SEEK_SET);//todo: pageNum = page index? how about page 0?
	int errCode = fwrite(data, sizeof(char), PAGE_SIZE, pFile); //  total number of elements successfully write would be returned

	if (errCode ==  PAGE_SIZE) 
		return RC_SUCCESS;
	else
		return RC_FILE_WRITE_FAIL;
}


RC FileHandle::appendPage(const void *data)
{
    int errCode = fwrite(data, sizeof(char), PAGE_SIZE, this->pFile);
	this->pageNum++;
	if (errCode == PAGE_SIZE)
		return RC_SUCCESS;
	else
		return RC_FILE_WRITE_FAIL;
}


unsigned FileHandle::getNumberOfPages()
{
    return this->pageNum;
}

RC FileHandle::bindFile(char* filename, FILE* pFile) 
{
	fseek(pFile, 0, SEEK_END);
	this->fileName = filename;
	this->pFile = pFile;
	this->pageNum = ftell(pFile)/PAGE_SIZE;
	//no possible exception so far
	return RC_SUCCESS;
}

RC FileHandle::seekToPage(PageNum pageNum)
{
	return fseek(this->getHandle(), pageNum * PAGE_SIZE, SEEK_SET);
}

char* FileHandle::getFileName()
{
	return this->fileName;
}

FILE* FileHandle::getHandle()
{
	return this->pFile;
}
