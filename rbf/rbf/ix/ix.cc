
#include "ix.h"

IndexManager* IndexManager::_index_manager = 0;
PagedFileManager* IndexManager::_pf_manager = 0;

IndexManager* IndexManager::instance()
{
    if(!_index_manager) {
		_pf_manager = PagedFileManager::instance();
        _index_manager = new IndexManager();
	}
    return _index_manager;
}

IndexManager::IndexManager()
{

}

IndexManager::~IndexManager()
{
}

RC IndexManager::createFile(const string &fileName)
{
	return _pf_manager->createFile(fileName.c_str());
}

RC IndexManager::destroyFile(const string &fileName)
{
	return _pf_manager->destroyFile(fileName.c_str());
}

RC IndexManager::openFile(const string &fileName, FileHandle &fileHandle)
{
	return _pf_manager->openFile(fileName.c_str(), fileHandle);
}

RC IndexManager::closeFile(FileHandle &fileHandle)
{
	return _pf_manager->closeFile(fileHandle);
}

RC IndexManager::insertEntry(FileHandle &fileHandle, const Attribute &attribute, const void *key, const RID &rid)
{
	IH_page* ptr_IHPage = new IH_page();
	if (ptr_IHPage == NULL)
		return RC_MEM_ALLOCATION_FAIL;

    void *headerBuffer = (void*)malloc(PAGE_SIZE);
	if (headerBuffer == NULL)
		return RC_MEM_ALLOCATION_FAIL;
	//init
	memset(headerBuffer, 0, PAGE_SIZE);

	// calculate the length of the void* datat
	//const unsigned recordLength = getRecordLength(recordDescriptor, data); 

	if(fileHandle.getNumberOfPages() == 0 )   // There is no entry in the index
	{
		//assume that insertion when tree is empty is stable
		// initialize the header infomration
		ptr_IHPage->init();
		ptr_IHPage->rootPageNum = 1;
		ptr_IHPage->writeData(headerBuffer);

		if(!SUCCEEDED(fileHandle.appendPage(headerBuffer)))
			return RC_APPEND_PAGE_FAIL;
		free(ptr_IHPage);

		void *pageBuffer = (void*)malloc(PAGE_SIZE);
		if (pageBuffer == NULL)
			return RC_MEM_ALLOCATION_FAIL;

		if(attribute.type == TypeInt) {
			int offset = 0;
			int k = readIntFromBuffer(key, offset);
			leaf_page<int> leafNode;
			leafNode.items[0].k = k;
			leafNode.items[0].rid = rid;
			leafNode.writeData(pageBuffer);
		} else if(attribute.type == TypeReal) {
			int offset = 0;
			float k = readFloatFromBuffer(key, offset);
			leaf_page<float> leafNode;
			leafNode.items[0].k = k;
			leafNode.items[0].rid = rid;
			leafNode.writeData(pageBuffer);
		} else {
			//TODO:
		}

		//write page buffer back to file
		if(!SUCCEEDED(fileHandle.appendPage(pageBuffer)))
			return RC_APPEND_PAGE_FAIL;
		free(pageBuffer);
	}
	else
	{
		//stub for now
		free(ptr_IHPage);
		/*if(!SUCCEEDED(fileHandle.readPage(fileHandle.getNumberOfPages() - 1 , pageBuffer)))
			return RC_FILE_READ_FAIL;

		ptr_HFPage->readData(pageBuffer);
		if ( recordLength > ptr_HFPage->nFreeSpace || ptr_HFPage->nUsedSlotCnt >= HF_PAGE_MAX_SLOT_NUMBER ) // append a new page to file if there is no avaible 
		{                                                                                                   // freespace or slot for new record 

            fileHandle.seekToPage(fileHandle.getNumberOfPages());//set the pointer to the next page

			if(!SUCCEEDED(fileHandle.appendPage(pageBuffer)))
				return RC_APPEND_PAGE_FAIL;

			// initialize the page infomration for the new page
			// getNumberOfPages refers to the number of pages
			ptr_HFPage->init(fileHandle.getNumberOfPages());
		}*/
	}

	/*//write record to page buffer
	appendRecordToBuffer(pageBuffer, ptr_HFPage->nFreeOffset, data, recordLength);

	//update the page directory
    ptr_HFPage->update(fileHandle.getNumberOfPages(), recordLength);
	ptr_HFPage->writeData(pageBuffer); // write the page directory into pageBuffer

	//update the record identification number
	rid.pageNum = ptr_HFPage->curPageNum;
	rid.slotNum = ptr_HFPage->nRecCnt - 1;

	//write page buffer back to file
	if(!SUCCEEDED(fileHandle.writePage(fileHandle.getNumberOfPages() - 1, pageBuffer)))
		return RC_FILE_WRITE_FAIL;

	free(pageBuffer);
	delete ptr_HFPage;*/

	return RC_SUCCESS;
}

RC IndexManager::deleteEntry(FileHandle &fileHandle, const Attribute &attribute, const void *key, const RID &rid)
{
	return -1;
}

RC IndexManager::scan(FileHandle &fileHandle,
    const Attribute &attribute,
    const void      *lowKey,
    const void      *highKey,
    bool			lowKeyInclusive,
    bool        	highKeyInclusive,
    IX_ScanIterator &ix_ScanIterator)
{
	return -1;
}

IX_ScanIterator::IX_ScanIterator()
{
}

IX_ScanIterator::~IX_ScanIterator()
{
}

RC IX_ScanIterator::getNextEntry(RID &rid, void *key)
{
	return -1;
}

RC IX_ScanIterator::close()
{
	return -1;
}

void IX_PrintError (RC rc)
{
}

void IH_page::init()
{ 
	depth = 1; 
	rootType = LEAF; 
	rootPageNum = -1;
}

void IH_page::readData(const void *data)
{
	memcpy(this,data,sizeof(IH_page));
}

void IH_page::writeData(void *data)
{
	memcpy(data,this,sizeof(IH_page));
}

template <class T>
void index_page<T>::readData(const void *data)
{
	memcpy(this,data,sizeof(index_page<T>));
}

template <class T>
void index_page<T>::writeData(void *data)
{
	memcpy(data,this,sizeof(index_page<T>));
}

template <class T>
void leaf_page<T>::readData(const void *data)
{
	memcpy(this,data,sizeof(leaf_page<T>));
}

template <class T>
void leaf_page<T>::writeData(void *data)
{
	memcpy(data,this,sizeof(leaf_page<T>));
}