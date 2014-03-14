
#include "rm.h"

RelationManager* RelationManager::_rm = 0;
RecordBasedFileManager* RelationManager::_rbfm = 0;
IndexManager* RelationManager::_im = 0;

RelationManager* RelationManager::instance()
{
    if(!_rm) {
		_rbfm = RecordBasedFileManager::instance();
		_im = IndexManager::instance();
        _rm = new RelationManager();
	}

    return _rm;
}

//initialize attributes descriptor for table.tbl
void RelationManager::initTableAttrs()
{
	Attribute attr;
    attr.name = "tableName";
    attr.type = TypeVarChar;
    attr.length = (AttrLength)MAX_STR_SIZE;
    tableAttrs.push_back(attr);

    attr.name = "tableType";
    attr.type = TypeInt;
    attr.length = (AttrLength)4;
    tableAttrs.push_back(attr);

    attr.name = "numCol";
    attr.type = TypeInt;
    attr.length = (AttrLength)4;
    tableAttrs.push_back(attr);

	attr.name = "fileName";
    attr.type = TypeVarChar;
    attr.length = (AttrLength)MAX_STR_SIZE;
    tableAttrs.push_back(attr);
}

//initialize attributes descriptor for columntbl
void RelationManager::initColumnAttrs()
{
	Attribute attr;
    attr.name = "tableName";
    attr.type = TypeVarChar;
    attr.length = (AttrLength)MAX_STR_SIZE;
    columnAttrs.push_back(attr);

    attr.name = "colName";
    attr.type = TypeVarChar;
    attr.length = (AttrLength)MAX_STR_SIZE;
    columnAttrs.push_back(attr);

    attr.name = "colType";
    attr.type = TypeVarChar;
    attr.length = (AttrLength)MAX_STR_SIZE;
    columnAttrs.push_back(attr);

	attr.name = "colPosition";
    attr.type = TypeInt;
    attr.length = (AttrLength)4;
    columnAttrs.push_back(attr);

	attr.name = "maxSize";
    attr.type = TypeInt;
    attr.length = (AttrLength)4;
    columnAttrs.push_back(attr);
}

RelationManager::RelationManager()
{

	FileHandle tableFileHandle;
	FileHandle columnFileHandle;

	initTableAttrs();
	initColumnAttrs();
    
	//if table.tbl doesn't exist
	if(_rbfm->createFile("table.tbl") != RC_FILE_EXISTED){
		//file created
		_rbfm->openFile("table.tbl", tableFileHandle);
		//write table info of table.tbl and column.tbl into table.tbl
		insertTableInfo(tableFileHandle, "table", TBL_SYSTEM, tableAttrs.size());
		insertTableInfo(tableFileHandle, "column", TBL_SYSTEM, columnAttrs.size());
		_rbfm->closeFile(tableFileHandle);

		_rbfm->createFile("column.tbl");
		_rbfm->openFile("column.tbl", columnFileHandle);
		//write attributes of table.tbl and column.tbl into column.tbl
		insertColumnInfo(columnFileHandle, "table", tableAttrs);
		insertColumnInfo(columnFileHandle, "column", columnAttrs);
		_rbfm->closeFile(columnFileHandle);
	} else {
		_rbfm->openFile("table.tbl", tableFileHandle);
		insertTableInfo(tableFileHandle, "table", TBL_SYSTEM, tableAttrs.size());
		insertTableInfo(tableFileHandle, "column", TBL_SYSTEM, columnAttrs.size());
		_rbfm->closeFile(tableFileHandle);

		_rbfm->openFile("column.tbl", columnFileHandle);
		insertColumnInfo(columnFileHandle, "table", tableAttrs);
		insertColumnInfo(columnFileHandle, "column", columnAttrs);
		_rbfm->closeFile(columnFileHandle);
	}


}

RelationManager::~RelationManager()
{

}

void RelationManager::writeTableInfoToBuffer(void *buffer, const string &tableName, int tableType, int colNum, const string &fileName)
{
    int offset = 0;
	writeStrToBuffer(buffer, offset, tableName);
    writeIntToBuffer(buffer, offset, tableType);
    writeIntToBuffer(buffer, offset, colNum);
    writeStrToBuffer(buffer, offset, fileName);
}

RC RelationManager::insertTableInfo(FileHandle &fileHandle, const string &tableName,  int tableType, int colNum)
{

	RID rid;
	//add max size of all attributes for the buffer initialization
	void *data = malloc(MAX_STR_SIZE * 2 + sizeof(int) * 2);

	writeTableInfoToBuffer(data, tableName, tableType, colNum, tableName + ".tbl");
	int errCode = _rbfm->insertRecord(fileHandle, tableAttrs, data, rid); 

	if(errCode != RC_SUCCESS) {
		free(data);
		return errCode;
	}
	free(data);

	return RC_SUCCESS;
}

void RelationManager::writeAttrToBuffer(void *buffer, const string &tableName, const Attribute & attr, int colPos)
{
    int offset = 0;

	writeStrToBuffer(buffer, offset, tableName);
    writeStrToBuffer(buffer, offset, attr.name);//write colName
    
	string colType = "";
	if(attr.type == TypeInt){
		colType = "int";
	} else if(attr.type == TypeReal) {
		colType = "float";
	} else if(attr.type == TypeVarChar) {
		colType = "string";
	} else {
		cout<<"error"<<endl;
	}

	writeStrToBuffer(buffer, offset, colType);
	writeIntToBuffer(buffer, offset, colPos);
	writeIntToBuffer(buffer, offset, attr.length);
}

RC RelationManager::insertColumnInfo(FileHandle &fileHandle, const string &tableName, const vector<Attribute> &attrs)
{

	RID rid;

	int colPos = 0;
	vector<Attribute>::const_iterator attrIt = attrs.begin();

	//write each attribute descriptor into column.tbl
	while(attrIt != attrs.end()) {
		void *data = malloc(MAX_STR_SIZE * 3 + sizeof(int) * 2);

		writeAttrToBuffer(data, tableName, *attrIt, colPos);
		int errCode = _rbfm->insertRecord(fileHandle, columnAttrs, data, rid);

		if(errCode != RC_SUCCESS) {
			free(data);
			return errCode;
		}
		free(data);

		attrIt++;
		colPos++; 
	}

	return RC_SUCCESS;
}

//write meta-data of a table into table.tbl and column.tbl
RC RelationManager::createTable(const string &tableName, const vector<Attribute> &attrs)
{
	FileHandle tableFileHandle;
	if(!SUCCEEDED( _rbfm->openFile("table.tbl", tableFileHandle) ))
		return RC_FILE_OPEN_FAIL;
	//user table
	int errCode = insertTableInfo(tableFileHandle, tableName, TBL_USER, attrs.size());
	if(errCode != RC_SUCCESS) {
		_rbfm->closeFile(tableFileHandle);
		return errCode;
	}
	if(!SUCCEEDED( _rbfm->closeFile(tableFileHandle) ))
		return RC_FILE_CLOSE_FAIL;


	FileHandle columnFileHandle;
	if(!SUCCEEDED( _rbfm->openFile("column.tbl", columnFileHandle) )) 
		return RC_FILE_OPEN_FAIL;
	errCode = insertColumnInfo(columnFileHandle, tableName, attrs);

	if(errCode != RC_SUCCESS) {
		_rbfm->closeFile(columnFileHandle);
		return errCode;
	}
	if(!SUCCEEDED( _rbfm->closeFile(columnFileHandle) ))
		return RC_FILE_CLOSE_FAIL;

	//create a new file for the tuples of that table
	FileHandle newTableFileHandle;
	const string fileName = tableName + ".tbl";
	if(!SUCCEEDED( _rbfm->createFile(fileName.c_str()) ))
		return RC_FILE_CREATION_FAIL;

    return RC_SUCCESS;
}

RC RelationManager::deleteTableInfo(const string &tableName)
{
	FileHandle tableFileHandle;
	if(!SUCCEEDED( _rbfm->openFile("table.tbl", tableFileHandle) ))
		return RC_FILE_OPEN_FAIL;

	RBFM_ScanIterator rbfm_ScanIterator;

	//write the target to compare into form of buffer
    void *value = malloc(tableName.length() + sizeof(int));
	int offset = 0;
	writeStrToBuffer(value, offset, tableName); 

	vector<string> attrNames;//spaceHolder
	_rbfm->scan(tableFileHandle, tableAttrs, "tableName", EQ_OP, value, attrNames, rbfm_ScanIterator);
	free(value);

	RID rid;
	void *recordData = malloc(rbfm_ScanIterator.length);
	//get the line with tableName equals to the argument(assume there is only one line in the loop)
	while( rbfm_ScanIterator.getNextRecord(rid, recordData) != RBFM_EOF ) {
		_rbfm->deleteRecord(tableFileHandle, tableAttrs, rid);
		break;
	}
	free(recordData);

	if(!SUCCEEDED( _rbfm->closeFile(tableFileHandle) ))
		return RC_FILE_CLOSE_FAIL;

	return RC_SUCCESS;
}

RC RelationManager::deleteColumnInfo(const string &tableName)
{
		FileHandle columnFileHandle;
	if(!SUCCEEDED( _rbfm->openFile("column.tbl", columnFileHandle) ))
		return RC_FILE_OPEN_FAIL;

	RBFM_ScanIterator rbfm_ScanIterator;

	//write the target to compare into form of buffer
    void *value = malloc(tableName.length() + sizeof(int));
	int offset = 0;
	writeStrToBuffer(value, offset, tableName); 

	vector<string> attrNames;//spaceHolder
	_rbfm->scan(columnFileHandle, columnAttrs, "tableName", EQ_OP, value, attrNames, rbfm_ScanIterator);
	free(value);

	RID rid;
	void *recordData = malloc(rbfm_ScanIterator.length);
	//get the lines with tableName equals to the argument
	while( rbfm_ScanIterator.getNextRecord(rid, recordData) != RBFM_EOF ) {
		//read attribute from buffer
		_rbfm->deleteRecord(columnFileHandle, columnAttrs, rid);
	}
	free(recordData);

	if(!SUCCEEDED( _rbfm->closeFile(columnFileHandle) ))
		return RC_FILE_CLOSE_FAIL;

	return RC_SUCCESS;
}

RC RelationManager::deleteTable(const string &tableName)
{

	//get file handle for the table
	FileHandle fileHandle;
	if(getFileHandle(tableName, fileHandle) == RC_TABLE_NOT_EXISTED)
		return RC_TABLE_NOT_EXISTED;
	if(!SUCCEEDED( _rbfm->closeFile(fileHandle) ))
		return RC_FILE_CLOSE_FAIL;
	if(!SUCCEEDED( _rbfm->destroyFile(fileHandle.getFileName()) ))
		return RC_FILE_DESTROY_FAIL;

	//TODO: delete index files

	deleteTableInfo(tableName);
	deleteColumnInfo(tableName);

    return RC_SUCCESS;
}
string RelationManager::getIndexFileName(const string &tableName, const string &attributeName)
{
	return tableName + attributeName + ".idx";
}

RC RelationManager::createIndex(const string &tableName, const string &attributeName)
{
	string indexFileName = getIndexFileName(tableName, attributeName);
	if( !SUCCEEDED( _im->createFile( indexFileName ) ) ) 
		return RC_FILE_CREATION_FAIL;

	FileHandle fileHandle;
	if( !SUCCEEDED( _im->openFile( indexFileName, fileHandle ) ) ) 
		return RC_FILE_OPEN_FAIL;
	
	RM_ScanIterator iter;
	vector<string> attrNames;

	//only need the key attribute
	attrNames.push_back(attributeName);

    _rm->scan(tableName, "", NO_OP, NULL, attrNames, iter);

	void *key = malloc(200);
	RID rid;

	Attribute attr;

	if( !SUCCEEDED( _rm->getAttribute(attr, tableName, attributeName) ) ) 
		return RC_ATTR_NOT_EXIST;

	while (iter.getNextTuple(rid, key) != RM_EOF) {
		if( !SUCCEEDED( _im->insertEntry(fileHandle, attr, key, rid) ) )
			return RC_INDEX_INSERT_FAIL;
	}

	if( !SUCCEEDED( _im->closeFile( fileHandle ) ) ) 
		return RC_FILE_CLOSE_FAIL;

	return RC_SUCCESS;
}

RC RelationManager::destroyIndex(const string &tableName, const string &attributeName)
{
	return _im->destroyFile(getIndexFileName(tableName, attributeName));
}

//get the specific attribute from attributes of a table
RC RelationManager::getAttribute(Attribute& attr, const string &tableName, const string &attributeName)
{
	vector<Attribute> attrs;
	this->getAttributes(tableName, attrs);

	vector<Attribute>::iterator attr_it = attrs.begin();
	while(attr_it != attrs.end()) {

		if(attr_it->name == attributeName) {
			attr = *attr_it;
			return RC_SUCCESS;
		}
		attr_it++;
	}
	return RC_ATTR_NOT_EXIST;
}

RC RelationManager::indexScan(const string &tableName,
                      const string &attributeName,
                      const void *lowKey,
                      const void *highKey,
                      bool lowKeyInclusive,
                      bool highKeyInclusive,
                      RM_IndexScanIterator &rm_IndexScanIterator)
{
	FileHandle fileHandle;
	
	string fileName = getIndexFileName(tableName, attributeName);
	if(!SUCCEEDED(_im->openFile(fileName, fileHandle)))
		return RC_FILE_OPEN_FAIL;
	
	Attribute attr;
	if(!SUCCEEDED(getAttribute(attr, tableName, attributeName)))
		return RC_ATTR_NOT_EXIST;

	IX_ScanIterator ix_scanIterator;
	_im->scan(fileHandle, attr, lowKey, highKey, lowKeyInclusive, highKeyInclusive, ix_scanIterator);
	rm_IndexScanIterator.ix_ScanIterator = ix_scanIterator;
	
	return RC_SUCCESS;
}

void RelationManager::readAttrFromBuffer(Attribute &attr, const void *buffer)
{
	int offset = 0;

	char *pAttrName = buildStrFromBuffer(buffer, offset);
	attr.name = string(pAttrName);
	free(pAttrName);

	char *pAttrType = buildStrFromBuffer(buffer, offset);

	if(strcmp(pAttrType, "string") == 0) {
		attr.type = TypeVarChar;
	} else if(strcmp(pAttrType, "int") == 0) {
		attr.type = TypeInt;
	} else if(strcmp(pAttrType, "float") == 0) {
		attr.type = TypeReal;
	}
	free(pAttrType);

	attr.length = readIntFromBuffer(buffer, offset);

}

//get all information of attributes of a table and put into a vector
RC RelationManager::getAttributes(const string &tableName, vector<Attribute> &attrs)
{
	FileHandle columnFileHandle;
	if(!SUCCEEDED( _rbfm->openFile("column.tbl", columnFileHandle) ))
		return RC_FILE_OPEN_FAIL;

	//initialize the attribute names for projection
	vector<string> attrNames;
	attrNames.push_back("colName");
	attrNames.push_back("colType");
	attrNames.push_back("maxSize");

	RBFM_ScanIterator rbfm_ScanIterator;

	//write the target to compare into form of buffer
    void *value = malloc(tableName.length() + sizeof(int));
	int offset = 0;
	writeStrToBuffer(value, offset, tableName); 

	_rbfm->scan(columnFileHandle, columnAttrs, "tableName", EQ_OP, value, attrNames, rbfm_ScanIterator);
	free(value);

	RID rid;
	void *recordData = malloc(rbfm_ScanIterator.length);
	Attribute attr;
	//get the lines with tableName equals to the argument
	while( rbfm_ScanIterator.getNextRecord(rid, recordData) != RBFM_EOF ) {
		//read attribute from buffer
		readAttrFromBuffer(attr, recordData);
		attrs.push_back(attr);
	}
	free(recordData);

	if(attrs.empty())
		return RC_TABLE_NOT_EXISTED;

	if(!SUCCEEDED( _rbfm->closeFile(columnFileHandle) ))
		return RC_FILE_CLOSE_FAIL;

	return RC_SUCCESS;
}

RC RelationManager::insertTuple(const string &tableName, const void *data, RID &rid)
{
	//get attributes descriptors
	vector<Attribute> attrs;
	if(getAttributes(tableName, attrs) == RC_TABLE_NOT_EXISTED)
		return RC_TABLE_NOT_EXISTED;

	//get file handle for the table
	FileHandle fileHandle;
	if(getFileHandle(tableName, fileHandle) == RC_TABLE_NOT_EXISTED)
		return RC_TABLE_NOT_EXISTED;

	//if(!SUCCEEDED( _rbfm->openFile(fileHandle.getFileName(), fileHandle) ))
		//return RC_FILE_OPEN_FAIL;

	int errCode = _rbfm->insertRecord(fileHandle, attrs, data, rid);
	if(errCode != RC_SUCCESS) {
		_rbfm->closeFile(fileHandle);
		return errCode;
	}

	if(!SUCCEEDED( _rbfm->closeFile(fileHandle) ))
		return RC_FILE_OPEN_FAIL;



	//insert RID and key into index file
	vector<Attribute>::iterator attr_it = attrs.begin();
	while(attr_it != attrs.end()) {

		Attribute attr = *attr_it;
		string fileName = getIndexFileName(tableName, attr.name);
		FileHandle idxFileHandle;

		//if there exists index file for current attribute
		if(SUCCEEDED(_im->openFile(fileName, idxFileHandle))) {

			void *key = malloc(50);//TODO: change later!!
			if(!SUCCEEDED(readAttribute(tableName, rid, attr.name, key)))
				return RC_ATTR_NOT_EXIST;

			if(!SUCCEEDED(_im->insertEntry(idxFileHandle, attr, key, rid)))
				return RC_INDEX_INSERT_FAIL;

			if(!SUCCEEDED(_im->closeFile(idxFileHandle)))
				return RC_FILE_CLOSE_FAIL;

		}
		attr_it++;
	}

    return RC_SUCCESS;
}

RC RelationManager::deleteTuples(const string &tableName)
{
    //get attributes descriptors
	vector<Attribute> attrs;
	if(getAttributes(tableName, attrs) == RC_TABLE_NOT_EXISTED)
		return RC_TABLE_NOT_EXISTED;

	//get file handle for the table
	FileHandle fileHandle;
	if(getFileHandle(tableName, fileHandle) == RC_TABLE_NOT_EXISTED)
		return RC_TABLE_NOT_EXISTED;

	//if(!SUCCEEDED( _rbfm->openFile(fileHandle.getFileName(), fileHandle) ))
		//return RC_FILE_OPEN_FAIL;

	int errCode = _rbfm->deleteRecords(fileHandle);
	if(errCode != RC_SUCCESS) {
		_rbfm->closeFile(fileHandle);
		return errCode;
	}

	if(!SUCCEEDED( _rbfm->closeFile(fileHandle) ))
		return RC_FILE_OPEN_FAIL;

    return RC_SUCCESS;
}

RC RelationManager::deleteTuple(const string &tableName, const RID &rid)
{
    //get attributes descriptors
	vector<Attribute> attrs;
	if(getAttributes(tableName, attrs) == RC_TABLE_NOT_EXISTED)
		return RC_TABLE_NOT_EXISTED;

	//get file handle for the table
	FileHandle fileHandle;
	if(getFileHandle(tableName, fileHandle) == RC_TABLE_NOT_EXISTED)
		return RC_TABLE_NOT_EXISTED;

	//if(!SUCCEEDED( _rbfm->openFile(fileHandle.getFileName(), fileHandle) ))
		//return RC_FILE_OPEN_FAIL;

	int errCode = _rbfm->deleteRecord(fileHandle, attrs, rid);
	if(errCode != RC_SUCCESS) {
		_rbfm->closeFile(fileHandle);
		return errCode;
	}

	if(!SUCCEEDED( _rbfm->closeFile(fileHandle) ))
		return RC_FILE_OPEN_FAIL;

    return RC_SUCCESS;
}

RC RelationManager::updateTuple(const string &tableName, const void *data, const RID &rid)
{
    //get attributes descriptors
	vector<Attribute> attrs;
	if(getAttributes(tableName, attrs) == RC_TABLE_NOT_EXISTED)
		return RC_TABLE_NOT_EXISTED;

	//get file handle for the table
	FileHandle fileHandle;
	if(getFileHandle(tableName, fileHandle) == RC_TABLE_NOT_EXISTED)
		return RC_TABLE_NOT_EXISTED;

	//if(!SUCCEEDED( _rbfm->openFile(fileHandle.getFileName(), fileHandle) ))
		//return RC_FILE_OPEN_FAIL;

	int errCode = _rbfm->updateRecord(fileHandle, attrs, data, rid);
	if(errCode != RC_SUCCESS) {
		_rbfm->closeFile(fileHandle);
		return errCode;
	}

	if(!SUCCEEDED( _rbfm->closeFile(fileHandle) ))
		return RC_FILE_OPEN_FAIL;

    return RC_SUCCESS;
}

RC RelationManager::readTuple(const string &tableName, const RID &rid, void *data)
{
    //get attributes descriptors
	vector<Attribute> attrs;
	if(getAttributes(tableName, attrs) == RC_TABLE_NOT_EXISTED)
		return RC_TABLE_NOT_EXISTED;

	//get file handle for the table
	FileHandle fileHandle;
	if(getFileHandle(tableName, fileHandle) == RC_TABLE_NOT_EXISTED)
		return RC_TABLE_NOT_EXISTED;

	int errCode = _rbfm->readRecord(fileHandle, attrs, rid, data);
	if(errCode != RC_SUCCESS) {
		_rbfm->closeFile(fileHandle);
		return errCode;
	}

	if(!SUCCEEDED( _rbfm->closeFile(fileHandle) ))
		return RC_FILE_OPEN_FAIL;

    return RC_SUCCESS;
}

RC RelationManager::readAttribute(const string &tableName, const RID &rid, const string &attributeName, void *data)
{
    //get attributes descriptors
	vector<Attribute> attrs;
	if(getAttributes(tableName, attrs) == RC_TABLE_NOT_EXISTED)
		return RC_TABLE_NOT_EXISTED;

	//get file handle for the table
	FileHandle fileHandle;
	if(getFileHandle(tableName, fileHandle) == RC_TABLE_NOT_EXISTED)
		return RC_TABLE_NOT_EXISTED;

	int errCode = _rbfm->readAttribute(fileHandle, attrs, rid, attributeName, data);
	if(errCode != RC_SUCCESS) {
		_rbfm->closeFile(fileHandle);
		return errCode;
	}

	if(!SUCCEEDED( _rbfm->closeFile(fileHandle) ))
		return RC_FILE_OPEN_FAIL;

    return RC_SUCCESS;
}

RC RelationManager::reorganizePage(const string &tableName, const unsigned pageNumber)
{
    //get attributes descriptors
	vector<Attribute> attrs;
	if(getAttributes(tableName, attrs) == RC_TABLE_NOT_EXISTED)
		return RC_TABLE_NOT_EXISTED;

	//get file handle for the table
	FileHandle fileHandle;
	if(getFileHandle(tableName, fileHandle) == RC_TABLE_NOT_EXISTED)
		return RC_TABLE_NOT_EXISTED;

	int errCode = _rbfm->reorganizePage(fileHandle, attrs, pageNumber);
	if(errCode != RC_SUCCESS) {
		_rbfm->closeFile(fileHandle);
		return errCode;
	}

	if(!SUCCEEDED( _rbfm->closeFile(fileHandle) ))
		return RC_FILE_OPEN_FAIL;

    return RC_SUCCESS;
}

RC RelationManager::scan(const string &tableName,
      const string &conditionAttribute,
      const CompOp compOp,                  
      const void *value,                    
      const vector<string> &attributeNames,
      RM_ScanIterator &rm_ScanIterator)
{
    //get attributes descriptors
	vector<Attribute> attrs;
	if(getAttributes(tableName, attrs) == RC_TABLE_NOT_EXISTED)
		return RC_TABLE_NOT_EXISTED;

	//get file handle for the table
	FileHandle fileHandle;
	if(getFileHandle(tableName, fileHandle) == RC_TABLE_NOT_EXISTED)
		return RC_TABLE_NOT_EXISTED;

	RBFM_ScanIterator rbfm_ScanIterator;
	int errCode = _rbfm->scan(fileHandle, attrs, conditionAttribute, compOp, value, attributeNames, rbfm_ScanIterator);
	rm_ScanIterator.rbfm_ScanIterator = rbfm_ScanIterator;
	if(errCode != RC_SUCCESS) {
		_rbfm->closeFile(fileHandle);
		return errCode;
	}



	if(!SUCCEEDED( _rbfm->closeFile(fileHandle) ))
		return RC_FILE_OPEN_FAIL;

    return RC_SUCCESS;
}

// Extra credit
RC RelationManager::dropAttribute(const string &tableName, const string &attributeName)
{
    return -1;
}

// Extra credit
RC RelationManager::addAttribute(const string &tableName, const Attribute &attr)
{
    return -1;
}

// Extra credit
RC RelationManager::reorganizeTable(const string &tableName)
{
    return -1;
}

//file will be open after this function
RC RelationManager::getFileHandle(const string &tableName, FileHandle &fileHandle)
{
	//we must put "table.tbl" always in the memory because there will be circular dependency
	//if getFileHandle->rm scan-> getFileHandle
	FileHandle tableFileHandle;
	if(!SUCCEEDED( _rbfm->openFile("table.tbl", tableFileHandle) ))
		return RC_FILE_OPEN_FAIL;

	vector<string> attrNames;
	attrNames.push_back("fileName");

	RBFM_ScanIterator rbfm_ScanIterator;

	//write the target to compare into form of buffer
    void *value = malloc(tableName.length() + sizeof(int));
	int offset = 0;
	writeStrToBuffer(value, offset, tableName); 

	_rbfm->scan(tableFileHandle, tableAttrs, "tableName", EQ_OP, value, attrNames, rbfm_ScanIterator);
	free(value);

	char *pFileName = NULL;

	RID rid;
	void *recordData = malloc(rbfm_ScanIterator.length);
	//get the line with tableName equals to the argument(assume there is only one line in the loop)
	while( rbfm_ScanIterator.getNextRecord(rid, recordData) != RBFM_EOF ) {
		//get the length of string from the buffer first
		int zeroOffset = 0;
		pFileName = buildStrFromBuffer(recordData, zeroOffset);
		break;
	}
	free(recordData);

	if(!SUCCEEDED( _rbfm->closeFile(tableFileHandle) ))
		return RC_FILE_CLOSE_FAIL;

	if(pFileName == NULL)
		return RC_TABLE_NOT_EXISTED;

	if(!SUCCEEDED( _rbfm->openFile(pFileName, fileHandle) ))
		return RC_FILE_OPEN_FAIL;

	return RC_SUCCESS;
}