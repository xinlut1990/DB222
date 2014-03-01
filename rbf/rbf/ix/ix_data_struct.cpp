#include "ix_data_struct.h"



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
	return RC_SUCCESS;
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
bool leaf_item<T>::inRange(const T &lowKey,const T &highKey, BoundType lowBoundType, BoundType highBoundType)
{

	if( highBoundType == NONE && lowBoundType == NONE) {
		return true;
	}

	if(highBoundType == NONE && lowBoundType == INCLUSIVE) {
		return this->k >= lowKey;
	}
	if (highBoundType == NONE && lowBoundType == EXCLUSIVE ){
		return this->k > lowKey;
	}
	if(lowBoundType == NONE && highBoundType == INCLUSIVE) {
		return this->k <= highKey;
	}
	if(lowBoundType == NONE && highBoundType == EXCLUSIVE){
		return this->k < highKey;
	}

	if(lowBoundType == INCLUSIVE && highBoundType == INCLUSIVE) {
		return this->k >= lowKey && this->k <= highKey;
	}
	if(lowBoundType == INCLUSIVE && highBoundType == EXCLUSIVE){
		return this->k >= lowKey && this->k < highKey;
	}
	if( lowBoundType == EXCLUSIVE && highBoundType == INCLUSIVE) {
		return this->k > lowKey && this->k <= highKey;
	}
	if( lowBoundType == EXCLUSIVE && highBoundType == EXCLUSIVE){
		return this->k > lowKey && this->k < highKey;
	}

	return false;
}


