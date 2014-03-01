
#ifndef _ix_h_
#define _ix_h_

#include <vector>
#include <string>

#include "../rbf/rbfm.h"
#include "ix_data_struct.h"

# define IX_EOF (-1)  // end of the index scan
# define MAX_PAGE_NUM (400)

# define FREED (1)
# define IN_USE (2)


class IX_ScanIterator;
struct IH_page;
class IndexManager {
 public:
  static IndexManager* instance();

  RC createFile(const string &fileName);

  RC destroyFile(const string &fileName);

  RC openFile(const string &fileName, FileHandle &fileHandle);

  RC closeFile(FileHandle &fileHandle);

  // The following two functions are using the following format for the passed key value.
  //  1) data is a concatenation of values of the attributes
  //  2) For int and real: use 4 bytes to store the value;
  //     For varchar: use 4 bytes to store the length of characters, then store the actual characters.
  RC insertEntry(FileHandle &fileHandle, const Attribute &attribute, const void *key, const RID &rid);  // Insert new index entry
  RC deleteEntry(FileHandle &fileHandle, const Attribute &attribute, const void *key, const RID &rid);  // Delete index entry

  // scan() returns an iterator to allow the caller to go through the results
  // one by one in the range(lowKey, highKey).
  // For the format of "lowKey" and "highKey", please see insertEntry()
  // If lowKeyInclusive (or highKeyInclusive) is true, then lowKey (or highKey)
  // should be included in the scan
  // If lowKey is null, then the range is -infinity to highKey
  // If highKey is null, then the range is lowKey to +infinity
  RC scan(FileHandle &fileHandle,
      const Attribute &attribute,
	  const void        *lowKey,
      const void        *highKey,
      bool        lowKeyInclusive,
      bool        highKeyInclusive,
      IX_ScanIterator &ix_ScanIterator);

  // prints out B+ tree 
  //
  RC print(FileHandle &fileHandle, AttrType type);


 protected:
  IndexManager   ();                            // Constructor
  ~IndexManager  ();                            // Destructor

 private:
  template <class T>
  RC insertEntryByType(FileHandle &fileHandle, AttrType type, const T &key, const RID &rid);  // Insert new index entry
  
  template <class T>
  RC deleteEntryByType(FileHandle &fileHandle, AttrType type, const T &key, const RID &rid);  // Delete index entry
  
  template <class T>
  RC scanByType(FileHandle &fileHandle,
	  AttrType type,
	  const T        &lowKey,
      const T        &highKey,
      BoundType        lowBoundType,
      BoundType        highBoundType,
      IX_ScanIterator &ix_ScanIterator);

  template <class T>
  int searchLeafWith(
	  FileHandle &fileHandle,
	  int rootPageNum, 
	  int depth, 
	  const T &key) const;

  template <class T>
  RC recursivelyInsertIndex(FileHandle &filehandle, 
							void *pageBuffer,
							IH_page *ptr_IHPage, 
							  const T &key, 
							  const int pageNum,
							  bool leafBelow);
  template <class T>
  RC recursivelyInsert(FileHandle &filehandle, 
							void *pageBuffer,
							IH_page *ptr_IHPage, 
							  const T &key, 
							  const RID &rid);
  template <class T>
  RC insertToLeaf(void *pageBuffer, 
							  IH_page *ptr_IHPage, 
							  const T &key, 
							  const RID &rid);

  static IndexManager *_index_manager;
  static PagedFileManager *_pf_manager;
};

class IX_ScanIterator {

 public:
  IX_ScanIterator() {index = 0;};  							// Constructor
  ~IX_ScanIterator(); 							// Destructor

  RC getNextEntry(RID &rid, void *key) { 	  
	  if(index == scanList.size()) {
		  return RBFM_EOF;
	  }
	  rid = scanList[index++];
	  return RC_SUCCESS;
  };  		// Get next matching entry
  RC close();             						// Terminate index scan

  vector<RID> scanList;
private:
  int index;

};

// print out the error message for a given return code
void IX_PrintError (RC rc);



struct page_entry
{
	unsigned pageNum;
	unsigned status;
};

//index header 
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
#endif