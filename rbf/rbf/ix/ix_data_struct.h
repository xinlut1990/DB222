#ifndef _ix_ds_
#define _ix_ds_

#include <string>
#include "..\rbf\rbfm.h"

using namespace std;

# define ORDER (10)
# define INDEX (1)
# define LEAF (2)


template <class T>
struct leaf_item
{
	T k;
	RID rid;
	bool inRange(const void *lowKey,const void *highKey, bool lowKeyInclusive, bool highKeyInclusive);
};

template <class T>
struct leaf_page 
{
	int parentPage;
	int nextPage;
	unsigned itemNum;
	leaf_item<T> items[2 * ORDER];

	void readData(const void *data);
	void writeData(void *data) const;
	bool isFull() const;
	bool isHalf() const;
	void insertItem(const T &key, const RID &rid);
	bool deleteItem(const T &key, const RID &rid); // return false if key not exist
	void split(leaf_page &newLeafPage);
	void link(int newLeafPageNum);

	leaf_page();
};

template <>
struct leaf_page<string>
{
	int parentPage;
	int nextPage;
	unsigned itemNum;
	leaf_item<string> items[2 * ORDER];

	void readData(const void *data)
	{
		//!!hacky way to solve the case when the leaf page is first created
		//and has no data;
		int next_page;
		memcpy(&next_page,data,sizeof(int));
		//no data, cuz page 0 is header!!!
		if(next_page != 0) {
			memcpy(this,data,sizeof(leaf_page<string>));
		}
	}
	void writeData(void *data) const
	{
		memcpy(data,this,sizeof(leaf_page<string>));
	}
	bool isFull() const
	{
		//if page is full, unable to insert
		return this->itemNum == 2 * ORDER;
	}
	bool isHalf() const
	{
		//if page is full, unable to insert
		return this->itemNum == ORDER;
	}
	void insertItem(const string &key, const RID &rid)
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

	bool deleteItem(const string &key, const RID &rid) // return false if key not exist
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

	void split(leaf_page<string> &newLeafPage)
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

	void link(int newLeafPageNum)
	{
		//link two pages
		this->nextPage = newLeafPageNum;
	}

	leaf_page()
	{
		this->parentPage = -1;
		//no next page
		this->nextPage = -1;
		this->itemNum = 0;
		initArray(this->items, sizeof(leaf_item<string>), 2 * ORDER);
	
	}
};



template <class T>
struct index_item
{
	T k;
	unsigned p;
};

template <class T>
struct index_page 
{
	bool isRoot;
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
};

template <>
struct index_page<string>
{
	bool isRoot;
	int parentPage;
	unsigned itemNum;
	unsigned p0;
	index_item<string> items[2 * ORDER];

	void readData(const void *data)
	{
		memcpy(this,data,sizeof(index_page<string>));
	}

	void writeData(void *data) const
	{
		memcpy(data,this,sizeof(index_page<string>));
	}

	bool isFull() const
	{
		//if page is full, unable to insert
		return this->itemNum == 2 * ORDER;
	}

	bool isHalf() const
	{
		//if page is full, unable to insert
		return this->itemNum == ORDER;
	}

	void insertItem(const string &key, const int pageNum)
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

	bool deleteItem(int index)
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

	void split(index_page &newIndexPage)
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

	RC updateParentForChildren(FileHandle &fileHandle, int myPageNum, bool leafBelow)
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
				leaf_page<string> leafPage;
				leafPage.readData(pageBuffer);
				leafPage.parentPage = myPageNum;
				leafPage.writeData(pageBuffer);
			} else {
				leaf_page<string> indexPage;
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

	int searchChild(const string &key)
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

	index_page()
	{
		this->isRoot = false;
		this->parentPage = -1;
		this->itemNum = 0;
		this->p0 = 0;
	}
};


#endif