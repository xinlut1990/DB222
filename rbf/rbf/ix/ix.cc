
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

//return page id of the leaf page
int IndexManager::searchLeafWith(FileHandle &fileHandle, 
								 int rootPageNum, 
								 int depth, 
								 AttrType type, 
								 const void *key) const
{

	int curDepth = 1;
	int pageNum = rootPageNum;

	//loop until we get the page number of the leaf page
	while(curDepth < depth) {
		void *pageBuffer = (void*)malloc(PAGE_SIZE);
		if (pageBuffer == NULL)
			return RC_MEM_ALLOCATION_FAIL;

		//read root page
		if(!SUCCEEDED(fileHandle.readPage(pageNum, pageBuffer)))
			return RC_FILE_READ_FAIL;

		//search in current level
		if(type == TypeInt  ) {
			index_page<int> indexPage;
			indexPage.readData(pageBuffer);

			int offset = 0;
			int k = readIntFromBuffer(key, offset);
			pageNum = indexPage.searchChild(k);

		} else if(type == TypeReal  ) {
			index_page<float> indexPage;
			indexPage.readData(pageBuffer);

			int offset = 0;
			int k = readFloatFromBuffer(key, offset);
			pageNum = indexPage.searchChild(k);
		} else {

		}

		free(pageBuffer);
	}

	return pageNum;

}

//pageBuffer: buffer of the leaf page
RC IndexManager::insertToLeaf( void *pageBuffer, 
							  IH_page *ptr_IHPage, 
							  AttrType type, 
							  const void *key, 
							  const RID &rid)
{

	//normal read
	if(type == TypeInt  ) {
		leaf_page<int> leafPage;
		leafPage.readData(pageBuffer);
		int offset = 0;
		int k = readIntFromBuffer(key, offset);
		//insert item
		if(leafPage.isFull()) {
			//split

			//update header
			ptr_IHPage->pageNum = 1;
			ptr_IHPage->rootPageNum = 1;
			ptr_IHPage->pages[0].pageNum = 1;
			ptr_IHPage->pages[0].status = IN_USE;

		} else {
			leafPage.insertItem(k, rid);
			//after inserting new entry
			leafPage.writeData(pageBuffer);
		}

	} else if(type == TypeReal) {
		leaf_page<float> leafPage;
		leafPage.readData(pageBuffer);
		int offset = 0;
		float k = readIntFromBuffer(key, offset);
		//insert item
		if(leafPage.isFull()) {
			//split

			//update header
			ptr_IHPage->pageNum = 1;
			ptr_IHPage->rootPageNum = 1;
			ptr_IHPage->pages[0].pageNum = 1;
			ptr_IHPage->pages[0].status = IN_USE;

		} else {
			leafPage.insertItem(k, rid);
			//after inserting new entry
			leafPage.writeData(pageBuffer);
		}
	}else {
		//TODO: read function for string
	}

	return RC_SUCCESS;
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

	if(fileHandle.getNumberOfPages() == 0 )   // There is no entry in the index
	{
		//assume that insertion when tree is empty is stable
		// initialize the header infomration

		if(attribute.type == TypeInt) {
			ptr_IHPage->init(TypeInt);
		} else if(attribute.type == TypeReal) {
			ptr_IHPage->init(TypeReal);
		} else {
			//TODO:
			cout<<"string not supported yet"<<endl;
			return RC_UNDER_CONSTRUCTION;
		}

		void *pageBuffer = (void*)malloc(PAGE_SIZE);
		if (pageBuffer == NULL)
			return RC_MEM_ALLOCATION_FAIL;
		memset(pageBuffer, 0, PAGE_SIZE);

		insertToLeaf(pageBuffer, ptr_IHPage, attribute.type, key, rid);

		//after inserting first page
		ptr_IHPage->pageNum = 1;
		ptr_IHPage->rootPageNum = 1;
		ptr_IHPage->pages[0].pageNum = 1;
		ptr_IHPage->pages[0].status = IN_USE;

		ptr_IHPage->writeData(headerBuffer);
		//write header into the end of the file
		if(!SUCCEEDED(fileHandle.appendPage(headerBuffer)))
			return RC_APPEND_PAGE_FAIL;

		
		//write page buffer back to file
		if(!SUCCEEDED(fileHandle.appendPage(pageBuffer)))
			return RC_APPEND_PAGE_FAIL;
		free(pageBuffer);
	}
	else //regular case
	{
		
		if(!SUCCEEDED(fileHandle.readPage(0, headerBuffer)))
			return RC_FILE_READ_FAIL;

		ptr_IHPage->readData(headerBuffer);

		if(ptr_IHPage->keyType != attribute.type) {
			return RC_TYPE_MISMATCH;
		}

		//naive case
		//only leaf search
		if(ptr_IHPage->depth == 1) { 
			void *pageBuffer = (void*)malloc(PAGE_SIZE);
			if (pageBuffer == NULL)
				return RC_MEM_ALLOCATION_FAIL;
			//read root page
			if(!SUCCEEDED(fileHandle.readPage(ptr_IHPage->rootPageNum, pageBuffer)))
				return RC_FILE_READ_FAIL;

			insertToLeaf(pageBuffer, ptr_IHPage, attribute.type, key, rid);

			//write page buffer back to file
			if(!SUCCEEDED(fileHandle.writePage(ptr_IHPage->rootPageNum, pageBuffer)))
				return RC_FILE_WRITE_FAIL;

			free(pageBuffer);

		} else {
			//search for leaf page with key

		}
		
		ptr_IHPage->writeData(headerBuffer);
		//write header back to file
		if(!SUCCEEDED(fileHandle.writePage(0, headerBuffer)))
			return RC_FILE_WRITE_FAIL;
		
	}

	free(headerBuffer);
	free(ptr_IHPage);


	return RC_SUCCESS;
}

RC IndexManager::deleteEntry(FileHandle &fileHandle, const Attribute &attribute, const void *key, const RID &rid)
{
	IH_page* ptr_IHPage = new IH_page();
	if (ptr_IHPage == NULL)
		return RC_MEM_ALLOCATION_FAIL;

    void *headerBuffer = (void*)malloc(PAGE_SIZE);
	if (headerBuffer == NULL)
		return RC_MEM_ALLOCATION_FAIL;
	//init
	memset(headerBuffer, 0, PAGE_SIZE);

	if(fileHandle.getNumberOfPages() != 0 ) {
		//get header
		if(!SUCCEEDED(fileHandle.readPage(0, headerBuffer)))
			return RC_FILE_READ_FAIL;

		ptr_IHPage->readData(headerBuffer);

		if(ptr_IHPage->keyType != attribute.type) {
			return RC_TYPE_MISMATCH;
		}

		//naive case
		if(ptr_IHPage->depth == 1) {

			void *pageBuffer = (void*)malloc(PAGE_SIZE);
			if (pageBuffer == NULL)
				return RC_MEM_ALLOCATION_FAIL;

			//read root page
			if(!SUCCEEDED(fileHandle.readPage(ptr_IHPage->rootPageNum, pageBuffer)))
				return RC_FILE_READ_FAIL;

			//normal read
			if(attribute.type == TypeInt ) {
				leaf_page<int> leafPage;
				leafPage.readData(pageBuffer);
				if(leafPage.itemNum == 0) {
					return RC_EMPTY_PAGE;
				}
				int offset = 0;
				int k = readIntFromBuffer(key, offset);
				//delete item
				if(leafPage.isHalf()) {
					//borrow node

					//merge node
				} else {
					if(!leafPage.deleteItem(k, rid)) {
						return RC_INDEX_DELETE_FAIL;
					} else {
						leafPage.writeData(pageBuffer);
					}
				}

			} else if(attribute.type == TypeReal) {

			}else {
				//TODO: read function for string
			}

			ptr_IHPage->writeData(headerBuffer);
			//write header back to file
			if(!SUCCEEDED(fileHandle.writePage(0, headerBuffer)))
				return RC_FILE_WRITE_FAIL;
		
			//write page buffer back to file
			if(!SUCCEEDED(fileHandle.writePage(ptr_IHPage->rootPageNum, pageBuffer)))
				return RC_FILE_WRITE_FAIL;

			free(pageBuffer);

		} else {

		}
	} else {  //no entry to delete
		return RC_EMPTY_INDEX;
	}

	free(headerBuffer);
	free(ptr_IHPage);

	return RC_SUCCESS;
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

void IH_page::init(AttrType type)
{ 
	depth = 1; 
	rootPageNum = -1;
	pageNum = 0;
	keyType = (unsigned)type;
	initArray(pages, sizeof(page_entry), MAX_PAGE_NUM);
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
int index_page<T>::searchChild(const T &key)
{
	if(key < items[0].k) {
		return p0;
	}

	for(int i = 1; i < itemNum; i++) {
		if(key < items[i].k) {
			return items[i].p;
		}
	}
	return items[itemNum - 1].p;
}

template <class T>
void leaf_page<T>::readData(const void *data)
{
	//!!hacky way to solve the case when the leaf page is first created
	//and has no data;
	int next_page;
	memcpy(&next_page,data,sizeof(int));
	//no data, cuz page 0 is header!!!
	if(next_page != 0) {
		memcpy(this,data,sizeof(leaf_page<T>));
	}
}

template <class T>
void leaf_page<T>::writeData(void *data)
{
	memcpy(data,this,sizeof(leaf_page<T>));
}

template <class T>
bool leaf_page<T>::isFull()
{
	//if page is full, unable to insert
	return this->itemNum == 2 * ORDER;
}

template <class T>
bool leaf_page<T>::isHalf()
{
	//if page is full, unable to insert
	return this->itemNum == ORDER;
}

template <class T>
void leaf_page<T>::insertItem(const T &key, const RID &rid)
{

	if(this->itemNum == 0) {
		items[0].k = key;
		items[0].rid = rid;
		itemNum++;
		return;
	}

	//iterate from the end
	
	for (int i = itemNum - 1; i >= 0; i --) {
		if(key < items[i].k) {
			//move current item to right
			items[i + 1].k = items[i].k;
			items[i + 1].rid = items[i].rid;
		} else {
			//insert
			items[i + 1].k = key;
			items[i + 1].rid = rid;
			this->itemNum++;
			break;
		}
	}
}

template <class T>
bool leaf_page<T>::deleteItem(const T &key, const RID &rid)
{

	//iterate from the end
	int toRemove = -1;
	for (int i = 0; i < itemNum; i ++) {
		if(key == this->items[i].k && rid == this->items[i].rid) {
			toRemove = i;
			
		} 
	}

	if(toRemove != -1) {
		for (int i = toRemove; i < itemNum; i ++) {
			items[i] = items[i + 1];
		} 
		itemNum --;
		return true;
	} else {
		return false; //not found
	}
}

template <class T>
leaf_page<T>::leaf_page()
{
	//no next page
	this->nextPage = -1;
	this->itemNum = 0;
	initArray(this->items, sizeof(leaf_item<T>), 2 * ORDER);
	
}