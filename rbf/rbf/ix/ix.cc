#include "ix.h"
#include <malloc.h>
#include <vector>

using namespace std;

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
/*
struct IH_page 
{
	unsigned depth;
	unsigned keyType;
	int rootPageNum;
	unsigned pageNum;
	page_entry pages[MAX_PAGE_NUM];
	void init(AttrType type);
	void readData(const void *data);
	void writeData(void *data) const;
	void updateAfterNewPage(int pageIdx);
};
*/
RC IndexManager::print(FileHandle &fileHandle, AttrType type)
{

	IH_page* ptr_IHPage = new IH_page();
    vector<int> child_node_index;
	int curDepth = 1;
	if (ptr_IHPage == NULL)
		return RC_MEM_ALLOCATION_FAIL;

    void *headerBuffer = (void*)malloc(PAGE_SIZE);
	if (headerBuffer == NULL)
		return RC_MEM_ALLOCATION_FAIL;
	//init
	memset(headerBuffer, 0, PAGE_SIZE);

	if(fileHandle.getNumberOfPages() == 0 )   // There is no entry in the index
	{
		cout << " These is no entry in the index!\n" ;
	}
	else //regular case
	{
		
		if(!SUCCEEDED(fileHandle.readPage(0, headerBuffer)))
			return RC_FILE_READ_FAIL;

		ptr_IHPage->readData(headerBuffer);

		if(ptr_IHPage->keyType != type) {
			return RC_TYPE_MISMATCH;
		}

		cout << "-------------------------------------------------------\n";
		cout << "    Summary\n";
		cout << "\n";
		cout << "    Order of B+ Tree is : " << ORDER <<"\n";
		cout << "    Depth of B+ Tree    : " << ptr_IHPage->depth << "\n";
	    cout << "    Root page # is      : " << ptr_IHPage->pageNum << "\n";
		cout << "\n";                                                       
		cout << "-------------------------------------------------------\n"; 
		memset(headerBuffer, 0, PAGE_SIZE);

	    
		if(type == TypeInt  ){
			index_page<int> *prt_indexPage = new index_page<int>();
		
			// fetch the info of root page
		    if(!SUCCEEDED(fileHandle.readPage(ptr_IHPage->rootPageNum, headerBuffer)))
				return RC_FILE_READ_FAIL;

		    prt_indexPage->readData(headerBuffer);
		    if (!prt_indexPage->isRoot)
				return RC_ROOT_PAGE_READ_FAIL;

		    if (prt_indexPage->itemNum < 1 || prt_indexPage->itemNum > 2 * ORDER)
				return RC_INVALID_NUM_OF_ENTRIES;

		    cout << "    Root node  " << "Depth : " << curDepth << "\n\n ";
			int count = 0;
			for(int i = 0; i < prt_indexPage->itemNum; i ++ )
			{
				cout << "   " << prt_indexPage->items[i].k << " ";
				child_node_index.push_back(prt_indexPage->items[i].p);
				count++;
				if ( count >= 10 )
				{  cout << "\n "; count = 0; }
			}
		
			cout << "\n";
		    curDepth++;
			cout << "-------------------------------------------------------\n"; 
			delete prt_indexPage;
		}
	    //loop until we get the page number of the leaf page
	    while(curDepth <= ptr_IHPage->depth) {
			void *pageBuffer = (void*)malloc(PAGE_SIZE);
			if (pageBuffer == NULL)
				return RC_MEM_ALLOCATION_FAIL;
			

			if ( curDepth != ptr_IHPage->depth ) // index node
			{

					if(type == TypeInt  )
					{
						
						int size = child_node_index.size();
						for( int i = 0; i < size; i++ )
						{
                            index_page<int> *prt_indexPage = new index_page<int>();
		 
							memset(pageBuffer, 0, PAGE_SIZE);
			                // fetch the info of root page
							cout << child_node_index.front() <<endl;
		                    if(!SUCCEEDED(fileHandle.readPage(child_node_index.front(), pageBuffer)))
								return RC_FILE_READ_FAIL;

		                    prt_indexPage->readData(pageBuffer);

		                    if (prt_indexPage->itemNum < ORDER || prt_indexPage->itemNum > 2 * ORDER)
								return RC_INVALID_NUM_OF_ENTRIES;

		                    
						    cout << "    Index (non-root )node  " << "Depth : " << curDepth << "\n\n ";
			                int count = 0;
			                for(int i = 0; i < prt_indexPage->itemNum; i ++ )
			                {
							     cout << "   " << prt_indexPage->items[i].k << " ";
								 child_node_index.push_back(prt_indexPage->items[i].p);
							     count++;
							     if ( count >= 10 )
							     {  cout << "\n "; count = 0; }
						    }
		                    child_node_index.erase(child_node_index.begin());
						    cout << "\n";
						    cout << "-------------------------------------------------------\n"; 

						    delete prt_indexPage;
						}
					}
					curDepth++;
			}
			else{ // leaf node
					if(type == TypeInt  )
					{
						int size = child_node_index.size();

						for( int i = 0; i < size; i++ )
						{
                            leaf_page<int> *prt_leafPage = new leaf_page<int>();
		
							memset(pageBuffer, 0, PAGE_SIZE);
			                // fetch the info of root page
		                    if(!SUCCEEDED(fileHandle.readPage(child_node_index.front(), pageBuffer)))
								return RC_FILE_READ_FAIL;

		                    prt_leafPage->readData(pageBuffer);

		                    if (prt_leafPage->itemNum < ORDER || prt_leafPage->itemNum > 2 * ORDER)
								return RC_INVALID_NUM_OF_ENTRIES;

		                    
						    cout << "    Leaf node  " << "Depth : " << curDepth << "\n\n ";
			                int count = 0;
			                for(int i = 0; i < prt_leafPage->itemNum; i ++ )
			                {
							     cout << "   " << prt_leafPage->items[i].k << " ";
							     count++;
							     if ( count >= 10 )
							     {  cout << "\n "; count = 0; }
						    }
		                    child_node_index.erase(child_node_index.begin());
						    cout << "\n";
						    cout << "-------------------------------------------------------\n"; 

						    delete prt_leafPage;
						}
					}
					curDepth++;

			}
			free (pageBuffer);
			free (headerBuffer);

		

	}
		}
	delete ptr_IHPage;
    

	return RC_SUCCESS;
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
			int k = 0;
			if(key != NULL) {
			  k	= readIntFromBuffer(key, offset);
			}
			pageNum = indexPage.searchChild(k);

		} else if(type == TypeReal  ) {
			index_page<float> indexPage;
			indexPage.readData(pageBuffer);

			int offset = 0;
			//float k = readFloatFromBuffer(key, offset);
			//pageNum = indexPage.searchChild(k);
		} else {

		}
		curDepth++;
		free(pageBuffer);
	}

	return pageNum;

}

RC IndexManager::recursivelyInsertIndex(FileHandle &filehandle, 
							void *pageBuffer,
							IH_page *ptr_IHPage, 
							  AttrType type, 
							  const void *key, 
							  const int pageNum,
							  bool leafBelow)
{
		//normal read
	if(type == TypeInt  ) {
		index_page<int> indexPage;
		indexPage.readData(pageBuffer);
		int offset = 0;
		int k = readIntFromBuffer(key, offset);
		//insert item
		if(indexPage.isFull()) {
			
			//buffer for new page
			void *newPageBuffer = (void*)malloc(PAGE_SIZE);
			if (newPageBuffer == NULL)
				return RC_MEM_ALLOCATION_FAIL;
			memset(newPageBuffer, 0, PAGE_SIZE);
			if(!SUCCEEDED(filehandle.appendPage(newPageBuffer)))
				return RC_APPEND_PAGE_FAIL;

			//move half of the entries to the new page
			index_page<int> newIndexPage;
			indexPage.split(newIndexPage);

			//TODO: nee testing, buggy
			int newIndexPageNum = filehandle.getNumberOfPages() - 1;
			//update header
			ptr_IHPage->updateAfterNewPage(newIndexPageNum);

			//insert the new entry
			if(k > indexPage.items[indexPage.itemNum - 1].k) {
				newIndexPage.insertItem(k, pageNum);
			} else {
				indexPage.insertItem(k, pageNum);
			}

			//TODO: update the parent pointer of children
			newIndexPage.updateParentForChildren(filehandle, newIndexPageNum, leafBelow);

			//page buffer for parent
			void *parentBuffer = (void*)malloc(PAGE_SIZE);
			if (parentBuffer == NULL)
				return RC_MEM_ALLOCATION_FAIL;

			index_page<int> parent;
			//if there is only one level
			int parentLink = indexPage.parentPage;
			if(parentLink == -1) {
				if(!SUCCEEDED(filehandle.appendPage(parentBuffer)))
					return RC_APPEND_PAGE_FAIL;
				//new index page becomes the root, and initially points to root before
				parent.isRoot = true;
				parent.p0 = ptr_IHPage->rootPageNum;
				ptr_IHPage->depth++;
				//dangerous design
				int parentPageNum = filehandle.getNumberOfPages() - 1;
				ptr_IHPage->updateAfterNewPage(parentPageNum);

				//reset links
				ptr_IHPage->rootPageNum = parentPageNum;
				indexPage.parentPage = parentPageNum;
				newIndexPage.parentPage = parentPageNum;
			} else {
				//read parent
				if(!SUCCEEDED(filehandle.readPage(indexPage.parentPage, parentBuffer)))
					return RC_FILE_READ_FAIL;
				parent.readData(parentBuffer);
				newIndexPage.parentPage = indexPage.parentPage;
			}
		
			//pop the first entry
			parent.writeData(parentBuffer);
			offset = 0;
			void *newKey = malloc(sizeof(int));
			writeIntToBuffer(newKey, offset, newIndexPage.items[0].k);
			newIndexPage.p0 = newIndexPage.items[0].p;
			newIndexPage.deleteItem(0);


			newIndexPage.writeData(newPageBuffer);

			if(!SUCCEEDED(filehandle.writePage(newIndexPageNum, newPageBuffer)))
				return RC_FILE_WRITE_FAIL;
			free(newPageBuffer);

			//push up
			recursivelyInsertIndex(filehandle, parentBuffer, ptr_IHPage, type, newKey, newIndexPageNum, false);
			free(newKey);

			if(!SUCCEEDED(filehandle.writePage(indexPage.parentPage, parentBuffer)))
				return RC_FILE_WRITE_FAIL;

			free(parentBuffer);
			indexPage.writeData(pageBuffer);
		} else {
			indexPage.insertItem(k, pageNum);
			//after inserting new entry
			indexPage.writeData(pageBuffer);
		}

	} else if(type == TypeReal) {
		
	}else {
		//TODO: read function for string
	}

	return RC_SUCCESS;
}

RC IndexManager::recursivelyInsert(FileHandle &filehandle, 
					 void *pageBuffer,
					 IH_page *ptr_IHPage, 
					 AttrType type, 
					 const void *key, 
					 const RID &rid)
{
	if(type == TypeInt  ) {
		leaf_page<int> leafPage;
		leafPage.readData(pageBuffer);
		int offset = 0;
		int k = readIntFromBuffer(key, offset);

		void *newPageBuffer = (void*)malloc(PAGE_SIZE);
		if (newPageBuffer == NULL)
			return RC_MEM_ALLOCATION_FAIL;
		memset(newPageBuffer, 0, PAGE_SIZE);
		if(!SUCCEEDED(filehandle.appendPage(newPageBuffer)))
			return RC_APPEND_PAGE_FAIL;

		//move half of the entries to the new page
		leaf_page<int> newLeafPage;
		
		leafPage.split(newLeafPage);

		//need to change
		int newLeafPageNum = filehandle.getNumberOfPages() - 1;
		//update header
		ptr_IHPage->updateAfterNewPage(newLeafPageNum);

		//link two pages
		leafPage.link(newLeafPageNum);

		//insert the new entry
		if(k > leafPage.items[leafPage.itemNum - 1].k) {
			newLeafPage.insertItem(k, rid);
		} else {
			leafPage.insertItem(k, rid);
		}

		//page buffer for parent
		void *parentBuffer = (void*)malloc(PAGE_SIZE);
		if (parentBuffer == NULL)
			return RC_MEM_ALLOCATION_FAIL;

		index_page<int> indexPage;
		//if there is only one level
		int parentLink = leafPage.parentPage;

		if(parentLink == -1) {

			if(!SUCCEEDED(filehandle.appendPage(parentBuffer)))
				return RC_APPEND_PAGE_FAIL;

			//new index page becomes the root, and initially points to root before
			indexPage.isRoot = true;
			indexPage.p0 = ptr_IHPage->rootPageNum;
			ptr_IHPage->depth++;

			//dangerous design
			int parentPageNum = filehandle.getNumberOfPages() - 1;
			ptr_IHPage->updateAfterNewPage(parentPageNum);

			//reset links
			ptr_IHPage->rootPageNum = parentPageNum;
			leafPage.parentPage = parentPageNum;
			newLeafPage.parentPage = parentPageNum;
		} else {
			//read parent
			if(!SUCCEEDED(filehandle.readPage(leafPage.parentPage, parentBuffer)))
				return RC_FILE_READ_FAIL;
			indexPage.readData(parentBuffer);
			newLeafPage.parentPage = leafPage.parentPage;
		}
		//write new leaf page back
		newLeafPage.writeData(newPageBuffer);
		if(!SUCCEEDED(filehandle.writePage(newLeafPageNum, newPageBuffer)))
			return RC_FILE_WRITE_FAIL;
		free(newPageBuffer);

		
		//copy up the first entry of new leaf
		indexPage.writeData(parentBuffer);
		offset = 0;
		void *newKey = malloc(sizeof(int));
		writeIntToBuffer(newKey, offset, newLeafPage.items[0].k);
		recursivelyInsertIndex(filehandle, parentBuffer, ptr_IHPage, type, newKey, newLeafPageNum, true);
		free(newKey);

		if(!SUCCEEDED(filehandle.writePage(leafPage.parentPage, parentBuffer)))
			return RC_FILE_WRITE_FAIL;

		free(parentBuffer);

		leafPage.writeData(pageBuffer);
	} else if(type == TypeReal ) {
		leaf_page<float> leafPage;
		leafPage.readData(pageBuffer);
	} else {

	}

	return RC_SUCCESS;
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
			return RC_LEAF_PAGE_FULL;
			
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
			return RC_LEAF_PAGE_FULL;
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
		ptr_IHPage->updateAfterNewPage(1);
		ptr_IHPage->rootPageNum = 1;

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
			int leafPageNum = ptr_IHPage->rootPageNum; // this number will change in insertion, so record it
			if(!SUCCEEDED(fileHandle.readPage(leafPageNum, pageBuffer)))
				return RC_FILE_READ_FAIL;

			if(insertToLeaf(pageBuffer, ptr_IHPage, attribute.type, key, rid) == RC_LEAF_PAGE_FULL) {
				recursivelyInsert(fileHandle, pageBuffer, ptr_IHPage, attribute.type, key, rid);
			}

			//write page buffer back to file
			if(!SUCCEEDED(fileHandle.writePage(leafPageNum, pageBuffer)))
				return RC_FILE_WRITE_FAIL;

			free(pageBuffer);

		} else { //regular case

			void *pageBuffer = (void*)malloc(PAGE_SIZE);
			if (pageBuffer == NULL)
				return RC_MEM_ALLOCATION_FAIL;

			//search for leaf page with key
			int leafPageNum = searchLeafWith(fileHandle, ptr_IHPage->rootPageNum, ptr_IHPage->depth, attribute.type, key);
						
			//read leaf page
			if(!SUCCEEDED(fileHandle.readPage(leafPageNum, pageBuffer)))
				return RC_FILE_READ_FAIL;

			if(insertToLeaf(pageBuffer, ptr_IHPage, attribute.type, key, rid) == RC_LEAF_PAGE_FULL) {
				recursivelyInsert(fileHandle, pageBuffer, ptr_IHPage, attribute.type, key, rid);
			}
			//write page buffer back to file
			if(!SUCCEEDED(fileHandle.writePage(leafPageNum, pageBuffer)))
				return RC_FILE_WRITE_FAIL;

			free(pageBuffer);
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
			int rootPageNum = ptr_IHPage->rootPageNum;
			if(!SUCCEEDED(fileHandle.readPage(rootPageNum, pageBuffer)))
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
			if(!SUCCEEDED(fileHandle.writePage(rootPageNum, pageBuffer)))
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
	
	if(fileHandle.getHandle() == NULL) {
		return RC_FILE_NOT_EXISTED;
	}
	IH_page* ptr_IHPage = new IH_page();
	if (ptr_IHPage == NULL)
		return RC_MEM_ALLOCATION_FAIL;

    void *headerBuffer = (void*)malloc(PAGE_SIZE);
	if (headerBuffer == NULL)
		return RC_MEM_ALLOCATION_FAIL;
	//init
	memset(headerBuffer, 0, PAGE_SIZE);


	if(!SUCCEEDED(fileHandle.readPage(0, headerBuffer)))
		return RC_FILE_READ_FAIL;

	ptr_IHPage->readData(headerBuffer);

	if(ptr_IHPage->keyType != attribute.type) {
		return RC_TYPE_MISMATCH;
	}

	//search for leaf page with key
	int leafPageNum = searchLeafWith(fileHandle, ptr_IHPage->rootPageNum, ptr_IHPage->depth, attribute.type, lowKey);
	
	void *pageBuffer = (void*)malloc(PAGE_SIZE);
	if (pageBuffer == NULL)
		return RC_MEM_ALLOCATION_FAIL;


	if(attribute.type == TypeInt ) {
		leaf_page<int> leafPage;

		while(leafPageNum != -1) {
			if(!SUCCEEDED(fileHandle.readPage(leafPageNum, pageBuffer)))
				return RC_FILE_READ_FAIL;
			
			leafPage.readData(pageBuffer);

			for(int i = 0; i < leafPage.itemNum; i ++) {
				if(leafPage.items[i].inRange(lowKey, highKey, lowKeyInclusive, highKeyInclusive)) {
					ix_ScanIterator.scanList.push_back(leafPage.items[i].rid);
				}
			}
			leafPageNum = leafPage.nextPage;

		}

	} else if(attribute.type == TypeReal ) {
		leaf_page<float> leafPage;
		leafPage.readData(pageBuffer);
	} else {

	}
	

	free(pageBuffer);
	free(headerBuffer);
	free(ptr_IHPage);
	return RC_SUCCESS;
}


IX_ScanIterator::~IX_ScanIterator()
{
}

RC IX_ScanIterator::close()
{
	return RC_SUCCESS;
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

void IH_page::writeData(void *data) const
{
	memcpy(data,this,sizeof(IH_page));
}

void IH_page::updateAfterNewPage(int pageIdx)
{
	this->pageNum++;
	this->pages[this->pageNum - 1].pageNum = pageIdx;
	this->pages[this->pageNum - 1].status = IN_USE;
}

template <class T>
void index_page<T>::readData(const void *data)
{
	memcpy(this,data,sizeof(index_page<T>));
}

template <class T>
void index_page<T>::writeData(void *data) const
{
	memcpy(data,this,sizeof(index_page<T>));
}

template <class T>
bool index_page<T>::isFull() const
{
	//if page is full, unable to insert
	return this->itemNum == 2 * ORDER;
}

template <class T>
bool index_page<T>::isHalf() const
{
	//if page is full, unable to insert
	return this->itemNum == ORDER;
}

template <class T>
void index_page<T>::insertItem(const T &key, const int pageNum)
{

	if(this->itemNum == 0) {
		items[0].k = key;
		items[0].p = pageNum;
		itemNum++;
		return;
	}

	//iterate from the end
	
	for (int i = itemNum - 1; i >= 0; i --) {
		if(key < items[i].k) {
			//move current item to right
			items[i + 1].k = items[i].k;
			items[i + 1].p = items[i].p;
		} else {
			//insert
			items[i + 1].k = key;
			items[i + 1].p = pageNum;
			this->itemNum++;
			break;
		}
	}
}

template <class T>
bool index_page<T>::deleteItem(int index)
{

	//iterate from the end
	int toRemove = -1;
	for (int i = 0; i < itemNum; i ++) {
		if(i == index) {
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
void index_page<T>::split(index_page<T> &newIndexPage)
{
	unsigned halfSize = this->itemNum / 2;
	newIndexPage.itemNum = halfSize;
	//move half of the elements to the new page
	for(int i = 0; i < halfSize; i++) {
		newIndexPage.items[i] = this->items[i + halfSize];
	}
	//cut the size of the old page
	this->itemNum = halfSize;
}

//update the parent link for children after the split of an index page
template <class T>
RC index_page<T>::updateParentForChildren(FileHandle &fileHandle, int myPageNum, bool leafBelow)
{
	void *pageBuffer = (void*)malloc(PAGE_SIZE);
	if (pageBuffer == NULL)
		return RC_MEM_ALLOCATION_FAIL;
	//init
	memset(pageBuffer, 0, PAGE_SIZE);


	for(int i = 0; i < itemNum; i++) {

		int pageNum = items[i].p;

		if(!SUCCEEDED(fileHandle.readPage(pageNum, pageBuffer)))
			return RC_FILE_READ_FAIL;
		if(leafBelow) {
			leaf_page<T> leafPage;
			leafPage.readData(pageBuffer);
			leafPage.parentPage = myPageNum;
			leafPage.writeData(pageBuffer);
		} else {
			leaf_page<T> indexPage;
			indexPage.readData(pageBuffer);
			indexPage.parentPage = myPageNum;
			indexPage.writeData(pageBuffer);
		}

		if(!SUCCEEDED(fileHandle.writePage(pageNum, pageBuffer)))
			return RC_FILE_WRITE_FAIL;
	}
	free(pageBuffer);

}

template <class T>
int index_page<T>::searchChild(const T &key)
{
	if(key < items[0].k) {
		return p0;
	}

	for(int i = 1; i < itemNum; i++) {
		if(key < items[i].k) {
			return items[i - 1].p;
		}
	}
	return items[itemNum - 1].p;
}
template <class T>
index_page<T>::index_page()
{
	this->isRoot = false;
	this->parentPage = -1;
	this->itemNum = 0;
	this->p0 = 0;
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
void leaf_page<T>::writeData(void *data) const
{
	memcpy(data,this,sizeof(leaf_page<T>));
}

template <class T>
bool leaf_page<T>::isFull() const
{
	//if page is full, unable to insert
	return this->itemNum == 2 * ORDER;
}

template <class T>
bool leaf_page<T>::isHalf() const
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
void leaf_page<T>::split(leaf_page<T> &newLeafPage)
{
	unsigned halfSize = this->itemNum / 2;
	newLeafPage.itemNum = halfSize;
	//move half of the elements to the new page
	for(int i = 0; i < halfSize; i++) {
		newLeafPage.items[i] = this->items[i + halfSize];
	}
	//cut the size of the old page
	this->itemNum = halfSize;
}

template <class T>
void leaf_page<T>::link(int newLeafPageNum) 
{
	//link two pages
	this->nextPage = newLeafPageNum;
}

template <class T>
leaf_page<T>::leaf_page()
{
	this->parentPage = -1;
	//no next page
	this->nextPage = -1;
	this->itemNum = 0;
	initArray(this->items, sizeof(leaf_item<T>), 2 * ORDER);
	
}

template <class T>
bool leaf_item<T>::inRange(const void *lowKey,const void *highKey, bool lowKeyInclusive, bool highKeyInclusive)
{
	int offset = 0;
	if(lowKey != NULL) {
		 T lk = reader<T>::readFromBuffer(lowKey, offset);
		 if(highKey != NULL)  {//both bounds
			 offset = 0;
			 T hk = reader<T>::readFromBuffer(highKey, offset);

			 if(lowKeyInclusive) {
				if(highKeyInclusive) {
					return this->k >= lk && this->k <= hk;
				} else {
					return this->k >= lk && this->k < hk;
				}
			 } else {
				if(highKeyInclusive) {
					return this->k > lk && this->k <= hk;
				} else {
					return this->k > lk && this->k < hk;
				}
			 }
		 } else { // only low bounds
			 if(lowKeyInclusive) {
				 return this->k >= lk;
			 } else {
				 return this->k > lk;
			 }
		 }
	} else {
		 if(highKey != NULL)  { // only high bounds
			 offset = 0;
			 T hk = reader<T>::readFromBuffer(highKey, offset);

			 if(highKeyInclusive) {
			 	 return this->k <= hk;
			 } else {
				 return this->k < hk;
			 }

		 } else { // no bounds
			 return true;
		 }
	}

	

}