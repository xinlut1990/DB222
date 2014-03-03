#include "ix.h"
#include <malloc.h>
#include <vector>
#include "ix_data_struct.cpp"
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

RC IndexManager::printIndex(FileHandle &fileHandle, const Attribute &attribute)
{
	fstream f("report.txt");
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
		return RC_EMPTY_INDEX;
	}
	else //regular case
	{
		/***************************************************
		 * Read header page to fetch corresponding page info
		 **************************************************/
		if(!SUCCEEDED(fileHandle.readPage(0, headerBuffer)))
			return RC_FILE_READ_FAIL;

		ptr_IHPage->readData(headerBuffer);

		// Report header format
		f << "-------------------------------------------------------\n";
		f << "    Summary\n";
		f << "\n";
		f << "    Order of B+ Tree is : " << ORDER <<"\n";
		f << "    Depth of B+ Tree    : " << ptr_IHPage->depth << "\n";
	    f << "    page # is      : " << ptr_IHPage->pageNum << "\n";
		f << "\n";                                                       
		f << "-------------------------------------------------------\n"; 
		memset(headerBuffer, 0, PAGE_SIZE);

		/**************************************
		 *	bool isRoot;
	        int parentPage;
	        unsigned itemNum;
	        unsigned p0;
	        index_item<T> items[2 * ORDER];
	        void readData(const void *data);
	        void writeData(void *data) const;
	        bool isFull() const;
	        bool isHalf() const;
	        void insertItem(const T &key, const int pageNum);
	        bool deleteItem(int index);
	        void split(index_page &newIndexPage);
	        RC updateParentForChildren(FileHandle &fileHandle, int myPageNum, bool leafBelow);
	        int searchChild(const T &key);
	        index_page();
		 **************************************/
	    /*if (attribute.type == TypeInt)
			index_page<int> indexPage;
		else if (attribute.type == TypeReal)
			index_page<float> indexPage;
		else
			index_page<string> indexPage;*/
		index_page<int> indexPage;
		
		// fetch the info of root page
		if(!SUCCEEDED(fileHandle.readPage(ptr_IHPage->rootPageNum, headerBuffer)))
			return RC_FILE_READ_FAIL;

		indexPage.readData(headerBuffer);
		if (!indexPage.isRoot)
			return RC_ROOT_PAGE_READ_FAIL;

		// Check to see if the req of root is satisfied
		if (indexPage.itemNum < 1 || indexPage.itemNum > 2 * ORDER)
			return RC_INVALID_NUM_OF_ENTRIES;

		f << "    Root node  " <<"Num: "<<ptr_IHPage->rootPageNum<< "Depth : " << curDepth << "itemNum: "<<indexPage.itemNum<<" \n\n ";
		int count = 0;
		for(int i = 0; i < indexPage.itemNum; i ++ )
		{
			f << "   " << indexPage.items[i].k << " ";
			child_node_index.push_back(indexPage.items[i].p);
			count++;
			if ( count >= 10 )
			{  f << "\n "; count = 0; }
		}
		
		f << "\n";
		curDepth++;
		f << "-------------------------------------------------------\n"; 
	
	    /**********************************
	     * non-root page fetch info
	    *********************************/
	    //loop until we get the page number of the leaf page
	    while(curDepth <= ptr_IHPage->depth) 
		{
			void *pageBuffer = (void*)malloc(PAGE_SIZE);
			if (pageBuffer == NULL)
				return RC_MEM_ALLOCATION_FAIL;

			if ( curDepth != ptr_IHPage->depth ) // index node
			{

				int size = child_node_index.size(); // child_node_index holds the info about all child nodes
				for( int i = 0; i < size; i++ )
				{
					/*if (attribute.type == TypeInt)
						index_page<int> indexPage;
					else if (attribute.type == TypeReal)
						index_page<float> indexPage;
					else
						index_page<string> indexPage;*/
					index_page<int> indexPage;

					memset(pageBuffer, 0, PAGE_SIZE);
			                
					// fetch the info
					f << child_node_index.front() <<endl;
		            if(!SUCCEEDED(fileHandle.readPage(child_node_index.front(), pageBuffer)))
						return RC_FILE_READ_FAIL;
					
					indexPage.readData(pageBuffer);

		            if (indexPage.itemNum < ORDER || indexPage.itemNum > 2 * ORDER)
						return RC_INVALID_NUM_OF_ENTRIES;
		                    
			        f << "    Index (non-root )page  " <<"num: "<<child_node_index.front()<< "Depth : " << curDepth << "itemNum: "<<indexPage.itemNum<< "parentPage: "<<indexPage.parentPage<<"\n\n ";
			        int count = 0;
			        for(int i = 0; i < indexPage.itemNum; i ++ )
			        {
						f << "   " << indexPage.items[i].k << " ";
						child_node_index.push_back(indexPage.items[i].p);
						count++;
						if ( count >= 10 )
						{  f << "\n "; count = 0; }
					}
		            child_node_index.erase(child_node_index.begin());
					f << "\n";
					f << "-------------------------------------------------------\n"; 

				}
				curDepth++;
			}
		    else{ // leaf node
				int size = child_node_index.size();

				for( int i = 0; i < size; i++ )
				{
					/*if (attribute.type == TypeInt)
						leaf_page<int> leafPage;
					else if (attribute.type == TypeReal)
						leaf_page<float> leafPage;
					else
						leaf_page<string> leafPage;*/
					leaf_page<int> leafPage;
		
					memset(pageBuffer, 0, PAGE_SIZE);
			                
					// fetch the info of root page
		        	if(!SUCCEEDED(fileHandle.readPage(child_node_index.front(), pageBuffer)))
						return RC_FILE_READ_FAIL;

		        	leafPage.readData(pageBuffer);

		        	if (leafPage.itemNum < ORDER || leafPage.itemNum > 2 * ORDER)
						return RC_INVALID_NUM_OF_ENTRIES;

		        	f << "    Leaf node  " <<"num: "<<child_node_index.front()<< "Depth : " << curDepth << "itemNum: "<<leafPage.itemNum<< "parentPage: "<<leafPage.parentPage<<"\n\n ";
			    	int count = 0;
			    	for(int i = 0; i < leafPage.itemNum; i ++ )
			    	{
						f << "   " << leafPage.items[i].k << " ";
						count++;
						if ( count >= 10 )
						{  f << "\n "; count = 0; }
					}
		        	
					child_node_index.erase(child_node_index.begin());
					f << "\n";
					f << "-------------------------------------------------------\n"; 

				}
				curDepth++;
			}
			free (pageBuffer);
	}
		f.close();
	free (headerBuffer);

	return RC_SUCCESS;
}
}
	


//return page id of the leaf page
//TODO: potential bug!
//key might be default value, which means null
template <class T>
int IndexManager::searchLeafWith(FileHandle &fileHandle, 
								 int rootPageNum, 
								 int depth, 
								 const T &key) const
{
	void *pageBuffer = (void*)malloc(PAGE_SIZE);
	if (pageBuffer == NULL)
		return RC_MEM_ALLOCATION_FAIL;

	int curDepth = 1;
	int pageNum = rootPageNum;

	//loop until we get the page number of the leaf page
	while(curDepth < depth) {


		//read root page
		if(!SUCCEEDED(fileHandle.readPage(pageNum, pageBuffer)))
			return RC_FILE_READ_FAIL;

		//search in current level
		index_page<T> indexPage;
		indexPage.readData(pageBuffer);

		pageNum = indexPage.searchChild(key);
		//increase depth
		curDepth++;

	}
	free(pageBuffer);
	return pageNum;

}

template <class T>
RC IndexManager::recursivelyInsertIndex(FileHandle &filehandle, 
							void *pageBuffer,
							IH_page *ptr_IHPage, 
							  const T &key, 
							  const int pageNum,
							  bool leafBelow)
{
	
	index_page<T> indexPage;
	indexPage.readData(pageBuffer);
	T k = key;
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
		index_page<T> newIndexPage;
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

		index_page<T> parent;
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
		T newKey = newIndexPage.items[0].k;
		newIndexPage.p0 = newIndexPage.items[0].p;
		newIndexPage.deleteItem(0);


		newIndexPage.writeData(newPageBuffer);

		if(!SUCCEEDED(filehandle.writePage(newIndexPageNum, newPageBuffer)))
			return RC_FILE_WRITE_FAIL;
		free(newPageBuffer);

		//push up
		recursivelyInsertIndex<T>(filehandle, parentBuffer, ptr_IHPage, newKey, newIndexPageNum, false);

		if(!SUCCEEDED(filehandle.writePage(indexPage.parentPage, parentBuffer)))
			return RC_FILE_WRITE_FAIL;

		free(parentBuffer);
		indexPage.writeData(pageBuffer);
	} else {
		indexPage.insertItem(k, pageNum);
		//after inserting new entry
		indexPage.writeData(pageBuffer);
	}

	return RC_SUCCESS;
}

template <class T>
RC IndexManager::recursivelyInsert(FileHandle &filehandle, 
					 void *pageBuffer,
					 IH_page *ptr_IHPage, 
					 const T &key, 
					 const RID &rid)
{
	leaf_page<T> leafPage;
	leafPage.readData(pageBuffer);
	T k = key;

	void *newPageBuffer = (void*)malloc(PAGE_SIZE);
	if (newPageBuffer == NULL)
		return RC_MEM_ALLOCATION_FAIL;
	memset(newPageBuffer, 0, PAGE_SIZE);
	if(!SUCCEEDED(filehandle.appendPage(newPageBuffer)))
		return RC_APPEND_PAGE_FAIL;

	//move half of the entries to the new page
	leaf_page<T> newLeafPage;
		
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

	index_page<T> indexPage;
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

	T newKey = newLeafPage.items[0].k;
	recursivelyInsertIndex<T>(filehandle, parentBuffer, ptr_IHPage, newKey, newLeafPageNum, true);

	if(!SUCCEEDED(filehandle.writePage(leafPage.parentPage, parentBuffer)))
		return RC_FILE_WRITE_FAIL;

	free(parentBuffer);

	leafPage.writeData(pageBuffer);

	return RC_SUCCESS;
}

//pageBuffer: buffer of the leaf page
template <class T>
RC IndexManager::insertToLeaf( void *pageBuffer, 
							  IH_page *ptr_IHPage, 
							  const T &key, 
							  const RID &rid)
{

	//normal read
	leaf_page<T> leafPage;
	leafPage.readData(pageBuffer);

	//insert item
	if(leafPage.isFull()) {
		return RC_LEAF_PAGE_FULL;
	} else {
		leafPage.insertItem(key, rid);
		//after inserting new entry
		leafPage.writeData(pageBuffer);
	}

	return RC_SUCCESS;
}
RC IndexManager::insertEntry(FileHandle &fileHandle, const Attribute &attribute, const void *key, const RID &rid)
{
	int offset = 0;
	if(attribute.type == TypeInt) {
		int k = reader<int>::readFromBuffer(key, offset);
		return insertEntryByType<int>(fileHandle, TypeInt, k, rid);
	} else if(attribute.type == TypeReal) {
		float k = reader<float>::readFromBuffer(key, offset);
		return insertEntryByType<float>(fileHandle, TypeReal, k, rid);
	} else {
		string k = reader<string>::readFromBuffer(key, offset);
		return insertEntryByType<string>(fileHandle, TypeVarChar, k, rid);
	}

}

template <class T>
RC IndexManager::insertEntryByType(FileHandle &fileHandle, AttrType type, const T &key, const RID &rid)
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

		if(type == TypeInt) {
			ptr_IHPage->init(TypeInt);
		} else if(type == TypeReal) {
			ptr_IHPage->init(TypeReal);
		} else {
			ptr_IHPage->init(TypeVarChar);
		}

		void *pageBuffer = (void*)malloc(PAGE_SIZE);
		if (pageBuffer == NULL)
			return RC_MEM_ALLOCATION_FAIL;
		memset(pageBuffer, 0, PAGE_SIZE);

		insertToLeaf<T>(pageBuffer, ptr_IHPage, key, rid);
			
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

		if(ptr_IHPage->keyType != type) {
			return RC_TYPE_MISMATCH;
		}
		if(ptr_IHPage->pageNum == MAX_PAGE_NUM) {
			return RC_PAGE_OVERFLOW;
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

			
			if(insertToLeaf<T>(pageBuffer, ptr_IHPage, key, rid) == RC_LEAF_PAGE_FULL) {
				recursivelyInsert<T>(fileHandle, pageBuffer, ptr_IHPage, key, rid);
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
			int leafPageNum = searchLeafWith<T>(fileHandle, ptr_IHPage->rootPageNum, ptr_IHPage->depth, key);
						
			//read leaf page
			if(!SUCCEEDED(fileHandle.readPage(leafPageNum, pageBuffer)))
				return RC_FILE_READ_FAIL;

			if(insertToLeaf<T>(pageBuffer, ptr_IHPage, key, rid) == RC_LEAF_PAGE_FULL) {
				recursivelyInsert<T>(fileHandle, pageBuffer, ptr_IHPage, key, rid);
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
	int offset = 0;
	//normal read
	if(attribute.type == TypeInt ) {
		int k = reader<int>::readFromBuffer(key, offset);
		return deleteEntryByType<int>(fileHandle, attribute.type, k, rid);
	} else if(attribute.type == TypeReal ) {
		float k = reader<float>::readFromBuffer(key, offset);
		return deleteEntryByType<float>(fileHandle, attribute.type, k, rid);
	} else {
		string k = reader<string>::readFromBuffer(key, offset);
		return deleteEntryByType<string>(fileHandle, attribute.type, k, rid);
	}
}

template <class T>
RC IndexManager::deleteEntryByType(FileHandle &fileHandle, AttrType type, const T &key, const RID &rid)
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

		if(ptr_IHPage->keyType != type) {
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
			leaf_page<T> leafPage;
			leafPage.readData(pageBuffer);
			if(leafPage.itemNum == 0) {
				return RC_EMPTY_PAGE;
			}

			//delete item
			if(leafPage.isHalf()) {
				//borrow node

				//merge node
			} else {
				if(!leafPage.deleteItem(key, rid)) {
					return RC_INDEX_DELETE_FAIL;
				} else {
					leafPage.writeData(pageBuffer);
				}
			}


			ptr_IHPage->writeData(headerBuffer);
			//write header back to file
			if(!SUCCEEDED(fileHandle.writePage(0, headerBuffer)))
				return RC_FILE_WRITE_FAIL;
		
			//write page buffer back to file
			if(!SUCCEEDED(fileHandle.writePage(rootPageNum, pageBuffer)))
				return RC_FILE_WRITE_FAIL;

			free(pageBuffer);

		} else { //regular case 
			// A lazy deletion is adequate in terms of testing cases. In this approach, when an entry is deleted, even if it 
			// causes a leaf page to become less than half full, no redistribution or node merging takes place. Underfilled node
			// is allowed in this case according to project specifications.
			void *pageBuffer = (void*)malloc(PAGE_SIZE);
			if (pageBuffer == NULL)
				return RC_MEM_ALLOCATION_FAIL;

			//search for leaf page with key
			int leafPageNum = searchLeafWith<T>(fileHandle, ptr_IHPage->rootPageNum, ptr_IHPage->depth, key);
						
			//read leaf page
			if(!SUCCEEDED(fileHandle.readPage(leafPageNum, pageBuffer)))
				return RC_FILE_READ_FAIL;

			/******************************************************
			 * Starts of deletion
			 ******************************************************/

			// ptr_leafPage
			leaf_page<T> *ptr_leafPage = new leaf_page<T>();
			ptr_leafPage->readData(pageBuffer);

			// Empty node
			if(ptr_leafPage->itemNum == 0) 
			{
				return RC_EMPTY_PAGE;
			}

			// Given that a lazy deletion would be implemented in this case, thus
			// it is not necessary to check to see if following req is satisfied or not
		    //if (ptr_leafPage->->itemNum < ORDER || ptr_leafPage->->itemNum > 2 * ORDER)
			//	return RC_INVALID_NUM_OF_ENTRIES;
			
			if(!ptr_leafPage->deleteItem(key, rid)) 
				return RC_INDEX_DELETE_FAIL;
			else 
				ptr_leafPage->writeData(pageBuffer);

			//write page buffer back to file
			if(!SUCCEEDED(fileHandle.writePage(leafPageNum, pageBuffer)))
				return RC_FILE_WRITE_FAIL;

			delete ptr_leafPage;
			free(pageBuffer);
		}
		
		ptr_IHPage->writeData(headerBuffer);
		//write header back to file
		if(!SUCCEEDED(fileHandle.writePage(0, headerBuffer)))
			return RC_FILE_WRITE_FAIL;
		}
    else 
	{  //no entry to delete
		return RC_EMPTY_INDEX;
	}

	free(headerBuffer);
	delete ptr_IHPage;

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
	BoundType lowBoundType = lowKeyInclusive ? INCLUSIVE : EXCLUSIVE;
	BoundType highBoundType = highKeyInclusive ? INCLUSIVE : EXCLUSIVE;

	if(attribute.type == TypeInt ) {
		leaf_page<int> leafPage;

		int offset = 0;
		int lk = 0;
		if(lowKey != NULL) {
			lk = reader<int>::readFromBuffer( lowKey, offset);
		} else {
			lowBoundType = NONE;
		}

		offset = 0;
		int hk = 0;
		if(highKey != NULL) {
			hk = reader<int>::readFromBuffer( highKey, offset);
		} else {
			highBoundType = NONE;
		}

		return scanByType<int>(fileHandle, attribute.type, lk, hk, lowBoundType, highBoundType, ix_ScanIterator);

	} else if(attribute.type == TypeReal ) {
		leaf_page<float> leafPage;

		int offset = 0;
		float lk = 0;
		if(lowKey != NULL) {
			lk = reader<float>::readFromBuffer( lowKey, offset);
		} else {
			lowBoundType = NONE;
		}

		offset = 0;
		float hk = 0;
		if(highKey != NULL) {
			hk = reader<float>::readFromBuffer( highKey, offset);
		} else {
			highBoundType = NONE;
		}

		return scanByType<float>(fileHandle, attribute.type, lk, hk, lowBoundType, highBoundType, ix_ScanIterator);

	} else {
		leaf_page<string> leafPage;
		
		int offset = 0;
		string lk = "";
		if(lowKey != NULL) {
			lk = reader<string>::readFromBuffer( lowKey, offset);
		} else {
			lowBoundType = NONE;
		}

		offset = 0;
		string hk = "";
		if(highKey != NULL) {
			hk = reader<string>::readFromBuffer( highKey, offset);
		} else {
			highBoundType = NONE;
		}

		return scanByType<string>(fileHandle, attribute.type, lk, hk, lowBoundType, highBoundType, ix_ScanIterator);

	}

}

template <class T>
RC IndexManager::scanByType(FileHandle &fileHandle,
	AttrType        type,
    const T         &lowKey,
    const T         &highKey,
    BoundType        lowBoundType,
    BoundType        highBoundType,
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

	if(ptr_IHPage->keyType != type) {
		return RC_TYPE_MISMATCH;
	}
	
	void *pageBuffer = (void*)malloc(PAGE_SIZE);
	if (pageBuffer == NULL)
		return RC_MEM_ALLOCATION_FAIL;


	leaf_page<T> leafPage;

	//search for leaf page with key
	int leafPageNum = searchLeafWith<T>(fileHandle, ptr_IHPage->rootPageNum, ptr_IHPage->depth, lowKey);

	while(leafPageNum != -1) {
		if(!SUCCEEDED(fileHandle.readPage(leafPageNum, pageBuffer)))
			return RC_FILE_READ_FAIL;
			
		leafPage.readData(pageBuffer);

		for(int i = 0; i < leafPage.itemNum; i ++) {
			if(leafPage.items[i].inRange(lowKey, highKey, lowBoundType, highBoundType)) {
				ix_ScanIterator.scanList.push_back(leafPage.items[i].rid);
			}
		}
		leafPageNum = leafPage.nextPage;

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