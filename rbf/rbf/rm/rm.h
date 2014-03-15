
#ifndef _rm_h_
#define _rm_h_

#include <string>
#include <vector>

#include "../rbf/rbfm.h"
#include "../ix/ix.h"

using namespace std;


# define RM_EOF (-1)  // end of a scan operator

// RM_ScanIterator is an iteratr to go through tuples
// The way to use it is like the following:
//  RM_ScanIterator rmScanIterator;
//  rm.open(..., rmScanIterator);
//  while (rmScanIterator(rid, data) != RM_EOF) {
//    process the data;
//  }
//  rmScanIterator.close();

class RM_ScanIterator {
public:
  RM_ScanIterator() {};
  ~RM_ScanIterator() {

  };
  RBFM_ScanIterator rbfm_ScanIterator;
  // "data" follows the same format as RecordBasedFileManager::insertRecord()
  RC getNextTuple(RID &rid, void *data) { 
	  return rbfm_ScanIterator.getNextRecord(rid, data);

  };
  RC reset() { rbfm_ScanIterator.index = 0; return 0;};
  RC close() { return -1; };

};

class RM_IndexScanIterator {
 public:
  RM_IndexScanIterator() {};  	// Constructor
  ~RM_IndexScanIterator() {}; 	// Destructor
  IX_ScanIterator ix_ScanIterator;
  // "key" follows the same format as in IndexManager::insertEntry()
  RC getNextEntry(RID &rid, void *key) {return ix_ScanIterator.getNextEntry(rid, key);};  	// Get next matching entry
  RC reset() { ix_ScanIterator.index = 0; return 0;};
  RC close() {return -1;};             			// Terminate index scan
};


// Relation Manager
class RelationManager
{
public:
  static RelationManager* instance();

  RC createTable(const string &tableName, const vector<Attribute> &attrs);

  RC deleteTable(const string &tableName);

  RC getAttributes(const string &tableName, vector<Attribute> &attrs);

  RC insertTuple(const string &tableName, const void *data, RID &rid);

  RC deleteTuples(const string &tableName);

  RC deleteTuple(const string &tableName, const RID &rid);

  // Assume the rid does not change after update
  RC updateTuple(const string &tableName, const void *data, const RID &rid);

  RC readTuple(const string &tableName, const RID &rid, void *data);

  RC readAttribute(const string &tableName, const RID &rid, const string &attributeName, void *data);

  RC reorganizePage(const string &tableName, const unsigned pageNumber);

  // scan returns an iterator to allow the caller to go through the results one by one. 
  RC scan(const string &tableName,
      const string &conditionAttribute,
      const CompOp compOp,                  // comparision type such as "<" and "="
      const void *value,                    // used in the comparison
      const vector<string> &attributeNames, // a list of projected attributes
      RM_ScanIterator &rm_ScanIterator);

  RC createIndex(const string &tableName, const string &attributeName);

  RC destroyIndex(const string &tableName, const string &attributeName);

  // indexScan returns an iterator to allow the caller to go through qualified entries in index
  RC indexScan(const string &tableName,
                        const string &attributeName,
                        const void *lowKey,
                        const void *highKey,
                        bool lowKeyInclusive,
                        bool highKeyInclusive,
                        RM_IndexScanIterator &rm_IndexScanIterator);


// Extra credit
public:
  RC dropAttribute(const string &tableName, const string &attributeName);

  RC addAttribute(const string &tableName, const Attribute &attr);

  RC reorganizeTable(const string &tableName);



protected:
  RelationManager();
  ~RelationManager();

private:
  string getIndexFileName(const string &tableName, const string &attributeName);
  // get file handle from table.tbl
  RC getFileHandle(const string &tableName, FileHandle &fileHandle);

  void initTableAttrs();
  void initColumnAttrs();
  // write one line of table info into record buffer
  void writeTableInfoToBuffer(  
	  void *buffer, 
	  const string &tableName, 
	  int tableType, 
	  int colNum, 
	  const string &fileName);
  //read attribute from buffer into a struct
  void readAttrFromBuffer(
	  Attribute &attr,
	  const void *buffer);
  // write one attribute description into record buffer
  void writeAttrToBuffer( 
	  void *buffer, 
	  const string &tableName, 
	  const Attribute & attr, 
	  int colPos);
   //insert table info into table.tbl
  RC insertTableInfo( 
	  FileHandle &fileHandle,
	  const string &tableName,  //the name of current table
	  int tableType,  //system or user
	  int colNum);    //how many columns in the table
  // insert attributes description into column.tbl
  RC insertColumnInfo(   
	  FileHandle &fileHandle,
	  const string &tableName, 
	  const vector<Attribute> &attrs);    

  RC deleteTableInfo(const string &tableName);
  RC deleteColumnInfo(const string &tableName);

  RC getAttribute(Attribute &attr, const string &tableName, const string &attributeName);

  static RelationManager *_rm;
  static RecordBasedFileManager *_rbfm;
  static IndexManager *_im;

  //meta-descriptors
  vector<Attribute> tableAttrs;
  vector<Attribute> columnAttrs;
};

#endif
