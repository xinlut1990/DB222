
#ifndef _rbfm_h_
#define _rbfm_h_

#include <string>
#include <string.h>
#include <iterator>
#include <vector>

#include "global.h"
#include "pfm.h"
#include "mem_util.h"

using namespace std;


typedef int RC; 
typedef unsigned PageNum;
struct HFPage;
// Record ID
typedef struct
{
  unsigned pageNum;
  unsigned slotNum;
} RID;

struct Slot_t{
	unsigned record_offset;
	unsigned record_length;  // equals to EMPTY_SLOT if currently slot is not in use
};

static const unsigned EMPTY_SLOT =  -1;
static const unsigned TOMBSTONE_SLOT = -2;
static const int HF_PAGE_MAX_SLOT_NUMBER = 50;

// Attribute
typedef enum { TypeInt = 0, TypeReal, TypeVarChar } AttrType;

typedef unsigned AttrLength;

struct Attribute {
    string   name;     // attribute name
    AttrType type;     // attribute type
    AttrLength length; // attribute length
	Attribute() {};
	bool operator == (const struct Attribute&a)
	{
		return (name==a.name && type ==a.type && length == a.length);
	}
};

// Comparison Operator (NOT needed for part 1 of the project)
typedef enum { EQ_OP = 0,  // =
           LT_OP,      // <
           GT_OP,      // >
           LE_OP,      // <=
           GE_OP,      // >=
           NE_OP,      // !=
           NO_OP       // no condition
} CompOp;



/****************************************************************************
The scan iterator is NOT required to be implemented for part 1 of the project 
*****************************************************************************/

# define RBFM_EOF (-1)  // end of a scan operator

// RBFM_ScanIterator is an iteratr to go through records
// The way to use it is like the following:
//  RBFM_ScanIterator rbfmScanIterator;
//  rbfm.open(..., rbfmScanIterator);
//  while (rbfmScanIterator(rid, data) != RBFM_EOF) {
//    process the data;
//  }
//  rbfmScanIterator.close();
class RecordBasedFileManager;

class RBFM_ScanIterator {
public:
   RBFM_ScanIterator() {index = 0;};
   ~RBFM_ScanIterator() {};
   //vector iterator
   vector<RID> scanList;//todo: memory leak?
   char* fileName;
   vector<Attribute> recordDescriptor;
   vector<string> attributeNames;
   int index;
   int length;
   RecordBasedFileManager* rbfm;
   // "data" follows the same format as RecordBasedFileManager::insertRecord()
   RC getNextRecord(RID &rid, void *data);
   RC close() { return -1; };
};


class RecordBasedFileManager
{
public:
  static RecordBasedFileManager* instance();

  RC createFile(const char *fileName);
  
  RC destroyFile(const char *fileName);
  
  RC openFile(const char *fileName, FileHandle &fileHandle);
  
  RC closeFile(FileHandle &fileHandle);

  //  Format of the data passed into the function is the following:
  //  1) data is a concatenation of values of the attributes
  //  2) For int and real: use 4 bytes to store the value;
  //     For varchar: use 4 bytes to store the length of characters, then store the actual characters.
  //  !!!The same format is used for updateRecord(), the returned data of readRecord(), and readAttribute()
  RC insertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid);

  RC readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data);
  
  // This method will be mainly used for debugging/testing
  RC printRecord(const vector<Attribute> &recordDescriptor, const void *data);

/**************************************************************************************************************************************************************
***************************************************************************************************************************************************************
IMPORTANT, PLEASE READ: All methods below this comment (other than the constructor and destructor) are NOT required to be implemented for part 1 of the project
***************************************************************************************************************************************************************
***************************************************************************************************************************************************************/
  RC deleteRecords(FileHandle &fileHandle);

  RC deleteRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid);

  // Assume the rid does not change after update
  RC updateRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, const RID &rid);

  RC readAttribute(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, const string attributeName, void *data);

  RC reorganizePage(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const unsigned pageNumber);

  // scan returns an iterator to allow the caller to go through the results one by one. 
  RC scan(FileHandle &fileHandle,
      const vector<Attribute> &recordDescriptor,
      const string &conditionAttribute,
      const CompOp compOp,                  // comparision type such as "<" and "="
      const void *value,                    // used in the comparison
      const vector<string> &attributeNames, // a list of projected attributes
      RBFM_ScanIterator &rbfm_ScanIterator);


// Extra credit for part 2 of the project, please ignore for part 1 of the project
public:

  RC reorganizeFile(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor);
   //project the original data into indicated columns
  void projectRecord(void *dest, const void *src, const vector<Attribute> &recordDescriptor, const vector<string> &projAttrs);

protected:
  RecordBasedFileManager();
  ~RecordBasedFileManager();
  unsigned getMaxLength(const vector<Attribute> &recordDescriptor);
  unsigned getRecordLength(const vector<Attribute> &recordDescriptor, const void *data);
  void appendRecordToBuffer(void *buffer, unsigned offset, const void *data, unsigned recordLength);
  void removeRecordfromBuffer(void *buffer, unsigned offset, unsigned recordLength);

  void readRecordFromBuffer(void *data, const void *buffer, const Slot_t& curSlot);

 

  void updateRecordwithTombstone(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, const RID &rid, HFPage *ptr_HFPage);
  RC readRecordwithTombstone(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const Slot_t &slot_t, void *data);

    bool scanGT_OP(const void *data, void *returnedData, AttrType type);
  bool scanLT_OP(const void *data, void *returnedData, AttrType type);
  bool scanEQ_OP(const void *data, void *returnedData, AttrType type);
  bool scanNE_OP(const void *data, void *returnedData, AttrType type);
  bool scanGE_OP(const void *data, void *returnedData, AttrType type);
  bool scanLE_OP(const void *data, void *returnedData, AttrType type);
  RC compareData(const void *value, void *returnedData, AttrType);
  RC parseVCharData(const void *value, void *returnedData);
  string convertToString(const void *data);
  float convertToReal(const void *data);
  RC convertToInt(const void *data);
  RC parseRealData(const void *value, void *returnedData);
  RC parseIntData(const void *value, void *returnedData);

private:
  static RecordBasedFileManager *_rbf_manager;
  RID curRec;            // rid of last record returned
};

#endif




struct HFPage
{
	Slot_t slot[HF_PAGE_MAX_SLOT_NUMBER];
	unsigned nRecCnt;  // record count in the page 
	unsigned nUsedSlotCnt; // number of slot which is in use
	unsigned nEntriesSlotDir; // number of entries in slot directory
	unsigned nFreeOffset;// offset where the last record in the page ends
	unsigned nFreeSpace; // available free space for a new record
	unsigned nNextPageNum; //next heapfile page number
    unsigned curPageNum;  // current heapfile page number

	void init(const int curPageNum);
	void update(const int curPageNum, const unsigned recordLength);
	void increment(const int curPageNum, const unsigned slotNumber, const unsigned recordLength);
	void reset(const int curPageNum, unsigned curSlotNum);
	void merge(void *data);
	void expand(void *data, const void *updated_data, unsigned slot_number,unsigned updatedRecordLength);
	void appendDatatoBuffer(void *buffer, unsigned buffer_offset, const void *data, unsigned data_offset, unsigned recordLength);
	void readData(const void *data);
	void writeData(void *data);
};
struct slot_compare {
  bool operator() (const pair<Slot_t, int> &a, const pair<Slot_t, int> &b) { return a.first.record_offset < b.first.record_offset;}
}; // would be used to sort the vector < pair<Slot_t, int> > accorrding to the record offset