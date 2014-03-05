#define _CRT_SECURE_NO_DEPRECATE 
#define _CRT_NONSTDC_NO_DEPRECATE

#include "rbfm.h"

RC RBFM_ScanIterator::getNextRecord(RID &rid, void *data) { 
	  if(index == scanList.size()) {
		  return RBFM_EOF;
	  }

	  rid = scanList[index];

	  FileHandle fileHandle;
	  if(!SUCCEEDED( rbfm->openFile(fileName, fileHandle) ))
		return RC_FILE_OPEN_FAIL;

	  void *recordData = malloc(length * 2);//TODO: how to deal with this hacky length?
	  rbfm->readRecord(fileHandle, recordDescriptor, rid, recordData);
	  rbfm->projectRecord(data, recordData, recordDescriptor, attributeNames);

	  if(!SUCCEEDED( rbfm->closeFile(fileHandle) ))
		return RC_FILE_CLOSE_FAIL;

	  free(recordData);

	  index++;

	  return RC_SUCCESS; 
};

RecordBasedFileManager* RecordBasedFileManager::_rbf_manager = 0;
PagedFileManager* RecordBasedFileManager::_pf_manager = 0;

RecordBasedFileManager* RecordBasedFileManager::instance()
{
    if(!_rbf_manager) {
		_pf_manager = PagedFileManager::instance();
        _rbf_manager = new RecordBasedFileManager();
	}
    return _rbf_manager;
}

RecordBasedFileManager::RecordBasedFileManager()
{
}

RecordBasedFileManager::~RecordBasedFileManager()
{
}

RC RecordBasedFileManager::createFile(const char *fileName) {
	return _pf_manager->createFile(fileName);
}

RC RecordBasedFileManager::destroyFile(const char *fileName) {
	return _pf_manager->destroyFile(fileName);
}

RC RecordBasedFileManager::openFile(const char *fileName, FileHandle &fileHandle) {
	return _pf_manager->openFile(fileName, fileHandle);
}

RC RecordBasedFileManager::closeFile(FileHandle &fileHandle) {
	return _pf_manager->closeFile(fileHandle);
}

unsigned RecordBasedFileManager::getRecordLength(const vector<Attribute> &recordDescriptor, const void *data)
{
	int recordLength = 0;
	vector<Attribute>::const_iterator attribute_it = recordDescriptor.begin();

	int *buffer_int = new int;

	while(attribute_it != recordDescriptor.end() ){  // calculate the length of the data
		if( (*attribute_it).type == TypeVarChar )
        {   
			int strLen = readIntFromBuffer(data, recordLength);
		    recordLength += strLen;
		}
		else
		    recordLength += sizeof(int);
		++attribute_it;
	}

	delete buffer_int;

	return recordLength;
}

unsigned RecordBasedFileManager::getMaxLength(const vector<Attribute> &recordDescriptor)
{
	unsigned recordLength = 0;
	vector<Attribute>::const_iterator attribute_it = recordDescriptor.begin();

	while(attribute_it != recordDescriptor.end() ){  // calculate the length of the data
		if( (*attribute_it).type == TypeVarChar )
        {   
		    recordLength += (*attribute_it).length;
		}
		else
		    recordLength += sizeof(int);
		++attribute_it;
	}

	return recordLength;
}

void RecordBasedFileManager::appendRecordToBuffer(void *buffer, unsigned offset, const void *data, unsigned recordLength)
{
	memcpy((char*)buffer + offset, data, recordLength );
}
void RecordBasedFileManager::removeRecordfromBuffer(void *buffer, unsigned offset, unsigned recordLength)
{
	memset((char*)buffer + offset, 0, recordLength);
}
//TODO: 
RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void* data, RID &rid) {

	HFPage* ptr_HFPage = new HFPage();
	if (ptr_HFPage == NULL)
		return RC_MEM_ALLOCATION_FAIL;

    void *pageBuffer = (void*)malloc(PAGE_SIZE);
	if (pageBuffer == NULL)
		return RC_MEM_ALLOCATION_FAIL;
	//init
	memset(pageBuffer, 0, PAGE_SIZE);

	if ( recordDescriptor.empty() )
		return RC_EMPTY_RECORD_DESCRIPTOR;

	// calculate the length of the void* datat
	const unsigned recordLength = getRecordLength(recordDescriptor, data); 

	if(fileHandle.getNumberOfPages() == 0 )   // if the number of pages in the file equals to zero, append a new page to the file
	{
		if(!SUCCEEDED(fileHandle.appendPage(pageBuffer)))
			return RC_APPEND_PAGE_FAIL;

		// initialize the page infomration for the new page
		ptr_HFPage->init(0);
	}
	else
	{
		if(!SUCCEEDED(fileHandle.readPage(fileHandle.getNumberOfPages() - 1 , pageBuffer)))
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
		}
	}

	//write record to page buffer
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
	delete ptr_HFPage;

	return RC_SUCCESS;

}

void RecordBasedFileManager::readRecordFromBuffer(void *data, const void *buffer, const Slot_t& curSlot)
{
	// copy the record data into buffer data
	memcpy(data, (char*)buffer + curSlot.record_offset, curSlot.record_length);
}
RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data) {

	HFPage* ptr_HFPage = (HFPage*)malloc(sizeof(HFPage));
	if (ptr_HFPage == NULL)
		return RC_MEM_ALLOCATION_FAIL;

    void *pageBuffer = malloc(PAGE_SIZE);
	if (pageBuffer == NULL)
		return RC_MEM_ALLOCATION_FAIL;

	if(!SUCCEEDED(fileHandle.readPage(rid.pageNum - 1 , pageBuffer)))
		return RC_FILE_READ_FAIL;

	// read the page directory information
	ptr_HFPage->readData(pageBuffer);
	Slot_t curSlot = ptr_HFPage->slot[rid.slotNum];

	if ( curSlot.record_length == EMPTY_SLOT )
		return RC_EMPTY_SLOT;
	if (rid.slotNum > HF_PAGE_MAX_SLOT_NUMBER)
		return RC_SLOT_NOT_EXISTED;

	if ( curSlot.record_length == TOMBSTONE_SLOT )
	{
		if(!SUCCEEDED(this->readRecordwithTombstone(fileHandle, recordDescriptor, curSlot, data)))
			return RC_FILE_READ_FAIL;
		else
			return RC_SUCCESS;
	}

	// copy the record data into buffer data
	readRecordFromBuffer(data, pageBuffer, curSlot);

	free(pageBuffer);
	free(ptr_HFPage);

	return RC_SUCCESS;
}

RC RecordBasedFileManager::printRecord(const vector<Attribute> &recordDescriptor, const void *data) {

	int* buffer_int;
	char* buffer_char;
	float* buffer_float;
	string buffer_str;
	int buffer_index = 0;

	vector<Attribute>::const_iterator attribute_it = recordDescriptor.begin();
	if ( recordDescriptor.empty() )
		return RC_EMPTY_RECORD_DESCRIPTOR;

	while ( attribute_it != recordDescriptor.end() )
	{
		if( (*attribute_it).type == TypeVarChar )
        {   
			buffer_int = new int;

			memcpy(buffer_int, (char*)data + buffer_index, sizeof(int));
			buffer_index += sizeof(int);			

			buffer_char = new char[*buffer_int + 1];
		    memcpy(buffer_char, (char*)data + buffer_index, *buffer_int);
			buffer_index += *buffer_int;
			buffer_char[*buffer_int] = '\0';
			buffer_str = buffer_char;

			cout<<(*attribute_it).name<<": "<<buffer_str<<'\n';
			
			delete [] buffer_char;
			delete buffer_int;	    
		}
		else if( (*attribute_it).type == TypeReal )
		{
			buffer_float = new float;
			memcpy(buffer_float, (char*)data + buffer_index, sizeof(float));
			buffer_index += sizeof(float);
			
			cout<<(*attribute_it).name<<": "<<*buffer_float<<'\n';
			
			delete buffer_float;
		}
		else
		{
			buffer_int = new int;
			memcpy(buffer_int, (char*)data + buffer_index, sizeof(int));
			buffer_index += sizeof(int);
			
			cout<<(*attribute_it).name<<": "<<*buffer_int<<'\n';
			
			delete buffer_int;
		}
		++attribute_it;
	}
	return RC_SUCCESS;
}

RC RecordBasedFileManager::deleteRecords(FileHandle &fileHandle)
{
	if(!SUCCEEDED( closeFile(fileHandle) ))
		return RC_FILE_CLOSE_FAIL;
	if(!SUCCEEDED( destroyFile(fileHandle.getFileName()) ))
		return RC_FILE_DESTROY_FAIL;
	if(!SUCCEEDED( createFile(fileHandle.getFileName()) ))
		return RC_FILE_CREATION_FAIL;
	if(!SUCCEEDED( openFile(fileHandle.getFileName(), fileHandle) ))
		return RC_FILE_OPEN_FAIL;
	return RC_SUCCESS;
}

RC RecordBasedFileManager::deleteRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid)
{
	HFPage* ptr_HFPage = (HFPage*)malloc(sizeof(HFPage));
	if (ptr_HFPage == NULL)
		return RC_MEM_ALLOCATION_FAIL;

    void *pageBuffer = malloc(PAGE_SIZE);
	if (pageBuffer == NULL)
		return RC_MEM_ALLOCATION_FAIL;

	//read page and load the entire content of the page onto pageBuffer
	if(!SUCCEEDED(fileHandle.readPage(rid.pageNum - 1 , pageBuffer)))
		return RC_FILE_READ_FAIL;

	// read the page directory information
	// curSlot would load the slot info according to RID
	ptr_HFPage->readData(pageBuffer);
	Slot_t curSlot = ptr_HFPage->slot[rid.slotNum];

	if( curSlot.record_length == EMPTY_SLOT )
		return RC_SUCCESS;
	if( curSlot.record_length == TOMBSTONE_SLOT)
	{
		RID newLocaton_rid;
	    unsigned page_num, slot_num, bitwise_buffer;
	
	    page_num = (curSlot.record_offset >> 16);
	    bitwise_buffer = (curSlot.record_offset << 16);
	    slot_num = ( bitwise_buffer >> 16 );

	    newLocaton_rid.pageNum = page_num;
	    newLocaton_rid.slotNum = slot_num;

		if(!SUCCEEDED(deleteRecord(fileHandle, recordDescriptor, newLocaton_rid)))
			return RC_DELETE_RECORD_FAIL;

		ptr_HFPage->slot[rid.slotNum].record_length = EMPTY_SLOT;
		ptr_HFPage->slot[rid.slotNum].record_offset = EMPTY_SLOT;
		ptr_HFPage->writeData(pageBuffer);

	    //write page buffer back to file
	    if(!SUCCEEDED(fileHandle.writePage(rid.pageNum - 1, pageBuffer)))
			return RC_FILE_WRITE_FAIL;

		return RC_SUCCESS;
	}


	//remove record from page buffer and set 0
	removeRecordfromBuffer(pageBuffer, curSlot.record_offset, curSlot.record_length);

	//update the page directory
    ptr_HFPage->reset(fileHandle.getNumberOfPages(), rid.slotNum);

	ptr_HFPage->writeData(pageBuffer); // write the page directory into pageBuffer

	//write page buffer back to file
	if(!SUCCEEDED(fileHandle.writePage(rid.pageNum - 1, pageBuffer)))
		return RC_FILE_WRITE_FAIL;

	free(pageBuffer);
	free(ptr_HFPage);

	return RC_SUCCESS;
}
//data is the result container
RC RecordBasedFileManager::readAttribute(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, const string attributeName, void *data)
{
    //void *pageBuffer = malloc(PAGE_SIZE);
	//if (pageBuffer == NULL)
	//	return RC_MEM_ALLOCATION_FAIL;

	void *recordData = malloc(getMaxLength(recordDescriptor));
	if(recordData == NULL)
		return RC_MEM_ALLOCATION_FAIL;

	this->readRecord(fileHandle, recordDescriptor, rid, recordData);

	int buffer_index = 0;

	vector<Attribute>::const_iterator attribute_it = recordDescriptor.begin();
	if ( recordDescriptor.empty() )
		return RC_EMPTY_RECORD_DESCRIPTOR;

	while ( attribute_it != recordDescriptor.end() )
	{
		if( (*attribute_it).type == TypeVarChar )
		{
			if ((*attribute_it).name == attributeName)
			{
				int writeOffset = 0;
				copyStrBetweenBuffer(data, writeOffset, recordData, buffer_index); 
				break;
			}
			else {
				//hacky here, if not copied, get length from recordData
				int zeroOffset = 0;
				buffer_index += sizeof(int) + readIntFromBuffer(recordData, zeroOffset);//todo: need test
			}

		}
		else if( (*attribute_it).type == TypeReal )
		{
			if ((*attribute_it).name == attributeName)
			{
				int writeOffset = 0;
				copyFloatBetweenBuffer(data, writeOffset,recordData, buffer_index); 
				break;
			} 
			else 
				buffer_index += sizeof(float);
		}
		else
		{
			if ((*attribute_it).name == attributeName)
			{
				int writeOffset = 0;
				copyIntBetweenBuffer(data, writeOffset,recordData, buffer_index); 
				break;
			}
			else 
				buffer_index += sizeof(int);
		}
		++attribute_it;
	}
	free(recordData);

	return RC_SUCCESS;

}
RC RecordBasedFileManager::readRecordwithTombstone(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const Slot_t &slot_t, void *data)
{
	RID rid;
	unsigned page_num, slot_num, bitwise_buffer;
	
	//TODO: refactor these five lines
	page_num = (slot_t.record_offset >> 16);
	bitwise_buffer = (slot_t.record_offset << 16);
	slot_num = ( bitwise_buffer >> 16 );

	rid.pageNum = page_num;
	rid.slotNum = slot_num;

	if(!SUCCEEDED(this->readRecord(fileHandle, recordDescriptor, rid, data)))
		return RC_FILE_READ_FAIL;

	return RC_SUCCESS;
}
void RecordBasedFileManager::updateRecordwithTombstone(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, const RID &rid, HFPage *ptr_HFPage)
{
	RID newLocationRid;
	this->insertRecord(fileHandle, recordDescriptor, data, newLocationRid);
	ptr_HFPage->slot[rid.slotNum].record_length = TOMBSTONE_SLOT;
	ptr_HFPage->slot[rid.slotNum].record_offset = newLocationRid.slotNum;
	ptr_HFPage->slot[rid.slotNum].record_offset += ( newLocationRid.pageNum << 16 );
}
RC RecordBasedFileManager::reorganizePage(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const unsigned pageNumber)
{
	HFPage* ptr_HFPage = (HFPage*)malloc(sizeof(HFPage));
	if (ptr_HFPage == NULL)
		return RC_MEM_ALLOCATION_FAIL;

    void *pageBuffer = malloc(PAGE_SIZE);
	if (pageBuffer == NULL)
		return RC_MEM_ALLOCATION_FAIL;

	//read page and load the entire content of the page onto pageBuffer
	if(!SUCCEEDED(fileHandle.readPage(pageNumber - 1, pageBuffer)))
		return RC_FILE_READ_FAIL;

	// read the page directory information
	ptr_HFPage->readData(pageBuffer);
	ptr_HFPage->merge(pageBuffer);

	//write page buffer back to file
	if(!SUCCEEDED(fileHandle.writePage(pageNumber - 1, pageBuffer)))
		return RC_FILE_WRITE_FAIL;

	return RC_SUCCESS;
}
RC RecordBasedFileManager::updateRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, const RID &rid)
{
	HFPage* ptr_HFPage = new HFPage();
	if (ptr_HFPage == NULL)
		return RC_MEM_ALLOCATION_FAIL;

    void *pageBuffer = (void*)malloc(PAGE_SIZE);
	if (pageBuffer == NULL)
		return RC_MEM_ALLOCATION_FAIL;

	if ( recordDescriptor.empty() )
		return RC_EMPTY_RECORD_DESCRIPTOR;

	// calculate the length of the void* data
	const unsigned updatedRecordLength = getRecordLength(recordDescriptor, data);

	if(!SUCCEEDED(fileHandle.readPage(rid.pageNum - 1 , pageBuffer)))
		return RC_FILE_READ_FAIL;

	// read the page directory information
	// curSlot would load the slot info according to RID
	ptr_HFPage->readData(pageBuffer);
	Slot_t curSlot = ptr_HFPage->slot[rid.slotNum];

	unsigned outdatedRecordLength = curSlot.record_length;
	if (curSlot.record_length == EMPTY_SLOT)
		outdatedRecordLength = 0;
	if (curSlot.record_length == TOMBSTONE_SLOT)
	{   
		RID newLocaton_rid;
	    unsigned page_num, slot_num, bitwise_buffer;
	
	    page_num = (curSlot.record_offset >> 16);
	    bitwise_buffer = (curSlot.record_offset << 16);
	    slot_num = ( bitwise_buffer >> 16 );

	    newLocaton_rid.pageNum = page_num;
	    newLocaton_rid.slotNum = slot_num;

		deleteRecord(fileHandle, recordDescriptor, newLocaton_rid);
		this->updateRecordwithTombstone(fileHandle, recordDescriptor, data, rid, ptr_HFPage);
		ptr_HFPage->writeData(pageBuffer);

	    //write page buffer back to file
	    if(!SUCCEEDED(fileHandle.writePage(rid.pageNum - 1, pageBuffer)))
			return RC_FILE_WRITE_FAIL;

		return RC_SUCCESS;
	}

	if ( updatedRecordLength > outdatedRecordLength )
	{
		if ( updatedRecordLength - outdatedRecordLength > ptr_HFPage->nFreeSpace )
		{
			this->updateRecordwithTombstone(fileHandle, recordDescriptor, data, rid, ptr_HFPage);
			ptr_HFPage->writeData(pageBuffer);
		}
		else if ( curSlot.record_length == EMPTY_SLOT )
		{
			//write record to page buffer
	        appendRecordToBuffer(pageBuffer, ptr_HFPage->nFreeOffset, data, updatedRecordLength);

	        //update the page directory
            ptr_HFPage->increment(fileHandle.getNumberOfPages(), rid.slotNum, updatedRecordLength);
	        ptr_HFPage->writeData(pageBuffer); // write the page directory into pageBuffer

		}
		else
			ptr_HFPage->expand(pageBuffer, data, rid.slotNum, updatedRecordLength);
	}
	else
	{
		// updated record length < outdated record length, rewrite the data field 
		removeRecordfromBuffer(pageBuffer, curSlot.record_offset, curSlot.record_length);
		appendRecordToBuffer(pageBuffer, curSlot.record_offset, data, updatedRecordLength);

		// update directory information accordingly
		ptr_HFPage->slot[rid.slotNum].record_length = updatedRecordLength;
		ptr_HFPage->writeData(pageBuffer);
	}

	//write page buffer back to file
	if(!SUCCEEDED(fileHandle.writePage(rid.pageNum - 1, pageBuffer)))
		return RC_FILE_WRITE_FAIL;

	return RC_SUCCESS;
}
void HFPage::appendDatatoBuffer(void *buffer, unsigned buffer_offset, const void *data, unsigned data_offset, unsigned recordLength)
{
	memcpy((char*)buffer + buffer_offset, (char*)data + data_offset, recordLength );
}
void HFPage::expand(void *data, const void *updated_data, unsigned slot_number,unsigned updatedRecordLength)
{
	vector< pair<Slot_t, int> > slot_t;
	vector< pair<Slot_t, int> >::iterator slot_t_iterator;
    void *pageBuffer = malloc(PAGE_SIZE);
	unsigned insert_record_offset;
	slot_compare compare_Slot;

	memset(pageBuffer, 0, PAGE_SIZE);

	//push the slot - slot number pair that is not empty into vector
	for( int i = 0; i < HF_PAGE_MAX_SLOT_NUMBER; i++ )
	{
		if( this->slot[i].record_length != EMPTY_SLOT && this->slot[i].record_length != TOMBSTONE_SLOT)
			slot_t.push_back(make_pair(slot[i], i));
	}
	// sort the SLot_t vector according to slot offset in ascending order
	// sort(slot_t.begin(),slot_t.end(), [](const pair<Slot_t, int> &a, const pair<Slot_t, int> &b) {return a.first.record_offset < b.first.record_offset;} );	
	sort(slot_t.begin(),slot_t.end(), compare_Slot);

	//*****************************************************
	//slot_t_iterator = find_if( slot_t.begin(), slot_t.end(), [slot_number](const pair<Slot_t, int> &a) {return a.second == slot_number;});
	//*****************************************************
	slot_t_iterator = slot_t.begin();
	while( slot_t_iterator != slot_t.end() )
	{
		if ( (*slot_t_iterator).second == slot_number )
			break;
		slot_t_iterator++;
	}

	(*slot_t_iterator).first.record_length = updatedRecordLength;
	this->appendDatatoBuffer(pageBuffer, sizeof(HFPage), data, sizeof(HFPage), (*slot_t_iterator).first.record_offset - sizeof(HFPage) );
	this->appendDatatoBuffer(pageBuffer, (*slot_t_iterator).first.record_offset, updated_data, 0, (*slot_t_iterator).first.record_length);

	this->slot[(*slot_t_iterator).second].record_length = updatedRecordLength;

	while( true )
	{
		insert_record_offset = this->slot[(*slot_t_iterator).second].record_offset + this->slot[(*slot_t_iterator).second].record_length;
		++slot_t_iterator;

		if ( slot_t_iterator == slot_t.end() )
			break;

		this->appendDatatoBuffer(pageBuffer, insert_record_offset, data, (*slot_t_iterator).first.record_offset, (*slot_t_iterator).first.record_length );
		this->slot[(*slot_t_iterator).second].record_offset = insert_record_offset;
	}

	// update offset where the last record in the page ends
	--slot_t_iterator;
	this->nFreeOffset = slot[(*slot_t_iterator).second].record_offset +  slot[(*slot_t_iterator).second].record_length;
    this->nFreeSpace = PAGE_SIZE - this->nFreeOffset;

	this->writeData(pageBuffer); // write the page directory into pageBuffer

	memcpy(data, pageBuffer, PAGE_SIZE);

	free(pageBuffer);
	return;
}
void HFPage::merge(void *data)
{
	vector< pair<Slot_t, int> > slot_t;
	vector< pair<Slot_t, int> >::iterator slot_t_iterator;
    void *pageBuffer = malloc(PAGE_SIZE);
	if (pageBuffer == NULL) {
		cout<<"error: mem allocation fail!"<<endl;
		return;
	}
	unsigned insert_record_offset;
	slot_compare compare_Slot;

	//reset pageBuffer to 0
	memset(pageBuffer, 0, PAGE_SIZE);
	
	//push the slot - slot number pair that is not empty into vector
	for( int i = 0; i < HF_PAGE_MAX_SLOT_NUMBER; i++ )
	{
		if( this->slot[i].record_length != EMPTY_SLOT && this->slot[i].record_length != TOMBSTONE_SLOT)
			slot_t.push_back(make_pair(slot[i], i));
	}
	// sort the SLot_t vector accordinf to slot offset in ascending order
	// sort(slot_t.begin(),slot_t.end(), [](const pair<Slot_t, int> &a, const pair<Slot_t, int> &b) {return a.first.record_offset < b.first.record_offset;} );
	sort(slot_t.begin(), slot_t.end(), compare_Slot);

	//*****************************************************
    // check to see if first a few slots are empty
	//*****************************************************
	slot_t_iterator = slot_t.begin();
	// move the first slot that is empty to the beginning of data entry
	this->appendDatatoBuffer(pageBuffer, sizeof(HFPage), data, (*slot_t_iterator).first.record_offset, (*slot_t_iterator).first.record_length );
	this->slot[(*slot_t_iterator).second].record_offset = sizeof(HFPage);

	//****************************************************
	// iterates the slot and rewrite the slot info & bufferdata
	//****************************************************
	while( true )
	{
		insert_record_offset = this->slot[(*slot_t_iterator).second].record_offset + this->slot[(*slot_t_iterator).second].record_length;
		++slot_t_iterator;

		if ( slot_t_iterator == slot_t.end() )
			break;

		this->appendDatatoBuffer(pageBuffer, insert_record_offset, data, (*slot_t_iterator).first.record_offset, (*slot_t_iterator).first.record_length );
		this->slot[(*slot_t_iterator).second].record_offset = insert_record_offset;
	}

	// update offset where the last record in the page ends
	--slot_t_iterator;
	this->nFreeOffset = slot[(*slot_t_iterator).second].record_offset +  slot[(*slot_t_iterator).second].record_length;
    this->nFreeSpace = PAGE_SIZE - this->nFreeOffset;

	this->writeData(pageBuffer); // write the page directory into pageBuffer

	memcpy(data, pageBuffer, PAGE_SIZE);

	free(pageBuffer);
}

void HFPage::init(int curPageNum)
{
	// initialize the page infomration for the new page
    this->nRecCnt=0;   // record count in the page
	this->nUsedSlotCnt=0; // number of slot which is in use
	this->nEntriesSlotDir=0; // number of entries in slot directory
	this->nFreeOffset=sizeof(HFPage);// offset where the last record in the page ends
    this->nFreeSpace = PAGE_SIZE-sizeof(HFPage); // available free space for a new record
    this->nNextPageNum=curPageNum+1; //next heapfile page number
    this->curPageNum=curPageNum;  // page number of current pointer

	for(int i = 0; i < HF_PAGE_MAX_SLOT_NUMBER; i++)
	{
		this->slot[i].record_length = -1;
		this->slot[i].record_offset = -1;
	}
}

void HFPage::update(const int curPageNum, const unsigned recordLength)
{
	int slot_count = 0;
	//update the page directory
    this->curPageNum = curPageNum;
	this->nNextPageNum = curPageNum + 1;
	// iterator slot dir to find an empty slot to insert record
	while( slot_count != HF_PAGE_MAX_SLOT_NUMBER )
	{
		if ( this->slot[slot_count].record_length == EMPTY_SLOT )
		{
			this->slot[slot_count].record_length = recordLength;
			this->slot[slot_count].record_offset = this->nFreeOffset;
			break;
		}
		slot_count++;
	}
	this->nFreeOffset = this->nFreeOffset + recordLength;
	this->nRecCnt++;
	this->nUsedSlotCnt++;
	this->nEntriesSlotDir++;
	this->nFreeSpace = PAGE_SIZE - this->nFreeOffset;
}

void HFPage::increment(const int curPageNum, const unsigned slotNumber, const unsigned recordLength)
{
	//update the page directory
	this->slot[slotNumber].record_length = recordLength;
	this->slot[slotNumber].record_offset = this->nFreeOffset;

	this->nFreeOffset = this->nFreeOffset + recordLength;
	this->nRecCnt++;
	this->nUsedSlotCnt++;
	this->nEntriesSlotDir++;
	this->nFreeSpace = PAGE_SIZE - this->nFreeOffset;
}
void HFPage::reset(const int curPageNum, unsigned curSlotNum)
{
	//reset the slot info when deletion occurs
    this->nRecCnt--;   // decrement record count in the page
	this->nUsedSlotCnt--; // decrement the number of slot which is in use
	this->nEntriesSlotDir--; // decrement the number of entries in slot directory

	if ( this->slot[curSlotNum].record_offset + this->slot[curSlotNum].record_length == nFreeOffset )
	{
		this->nFreeOffset -= this->slot[curSlotNum].record_length;
		this->nFreeSpace += this->slot[curSlotNum].record_length;
	}
    this->slot[curSlotNum].record_length = EMPTY_SLOT; //indicate that the slot is empty
	this->slot[curSlotNum].record_offset = EMPTY_SLOT; //indicate that the slot is not in use
}
//both read and write will only operate on part of the data buffer, which is of page size
void HFPage::readData(const void *data)
{
	memcpy(this,data,sizeof(HFPage));
}

void HFPage::writeData(void *data)
{
	memcpy(data,this,sizeof(HFPage));
}

void RecordBasedFileManager::projectRecord(void *dest, const void *src, const vector<Attribute> &recordDescriptor, const vector<string> &projAttrs)
{
	int buffer_index = 0;
	int dest_index = 0;

	vector<Attribute>::const_iterator attribute_it = recordDescriptor.begin();
	vector<string>::const_iterator proj_it = projAttrs.begin();

	while ( attribute_it != recordDescriptor.end() && proj_it != projAttrs.end() )
	{
		if( (*attribute_it).type == TypeVarChar )
		{
			//if attribute name equals to target
			if ((*attribute_it).name == *(proj_it))
			{
				//copy length and string to dest
				copyStrBetweenBuffer(dest, dest_index, src, buffer_index);
				++proj_it;
			}
			else {
				int zeroOffset = 0;
				//length of string
				buffer_index += sizeof(int) + readIntFromBuffer(src, zeroOffset);
			}
		}
		else if( (*attribute_it).type == TypeReal )
		{
			if ((*attribute_it).name == *(proj_it))
			{
				copyFloatBetweenBuffer(dest, dest_index, src, buffer_index);
				++proj_it;
			} 
			else 
				buffer_index += sizeof(float);
		}
		else // INT
		{
			if ((*attribute_it).name == *(proj_it))
			{
				copyIntBetweenBuffer(dest, dest_index, src, buffer_index);
				proj_it++;
			}
			else
				buffer_index += sizeof(int);
		}
		++attribute_it;
	}
}

RC RecordBasedFileManager::scan(FileHandle &fileHandle,const vector<Attribute> &recordDescriptor,const string &conditionAttribute,
                                const CompOp compOp,const void *value,const vector<string> &attributeNames,RBFM_ScanIterator &rbfm_ScanIterator)
{
	void *returnedData = malloc(100);
	string buffer_str;

	HFPage* ptr_HFPage = (HFPage*)malloc(sizeof(HFPage));
	if (ptr_HFPage == NULL)
		return RC_MEM_ALLOCATION_FAIL;

    void *pageBuffer = malloc(PAGE_SIZE);
	if (pageBuffer == NULL)
		return RC_MEM_ALLOCATION_FAIL;

	rbfm_ScanIterator.recordDescriptor = recordDescriptor;
	rbfm_ScanIterator.fileName = fileHandle.getFileName();
	rbfm_ScanIterator.rbfm = this;
	rbfm_ScanIterator.attributeNames = attributeNames;
	rbfm_ScanIterator.length = this->getMaxLength(recordDescriptor);

	vector<Attribute>::const_iterator attr_it = recordDescriptor.begin();
	//attr_it = find_if( recordDescriptor.begin(), recordDescriptor.end(), [conditionAttribute](const Attribute &a) {return a.name == conditionAttribute;});
	while ( attr_it != recordDescriptor.end() )
	{
		if ( (*attr_it).name == conditionAttribute )
			break;
		attr_it++;
	}
       
	for( int i = 1; i <= fileHandle.getNumberOfPages(); i++ )
	{
		if(!SUCCEEDED(fileHandle.readPage(i-1 , pageBuffer)))
			return RC_FILE_READ_FAIL;

		// read the page directory information
	    ptr_HFPage->readData(pageBuffer);
		for( int j = 0; j < HF_PAGE_MAX_SLOT_NUMBER; j++ )
		{
			if (ptr_HFPage->slot[j].record_length != EMPTY_SLOT)
			{	
				RID rid;
				rid.pageNum = i;
				rid.slotNum = j;
				if(compOp != NO_OP) {
					if( !SUCCEEDED( this->readAttribute(fileHandle, recordDescriptor, rid, conditionAttribute, returnedData)))
					return -1;
				}

				bool match_condition;
				switch(compOp)
				{
				case EQ_OP: match_condition = scanEQ_OP(value, returnedData, (*attr_it).type); break;
				case GT_OP: match_condition = scanLT_OP(value, returnedData, (*attr_it).type); break;
				case LT_OP: match_condition = scanGT_OP(value, returnedData, (*attr_it).type); break;
				case GE_OP: match_condition = scanLE_OP(value, returnedData, (*attr_it).type); break; 
				case LE_OP: match_condition = scanGE_OP(value, returnedData, (*attr_it).type); break;
				case NE_OP: match_condition = scanNE_OP(value, returnedData, (*attr_it).type); break;
				case NO_OP:
				default: match_condition = true;
				}
				
				if( ! match_condition )
					continue;

				//read current line, 

				//todo:change the length!!

				//if we only need rid
				/*if(!attributeNames.empty()) {
					void *recordData = malloc(this->getMaxLength(recordDescriptor));

					free(recordData);
				} */
				rbfm_ScanIterator.scanList.push_back(rid);
					
			}
		}
	}
	free(returnedData);
	free(ptr_HFPage);
	free(pageBuffer);
	return RC_SUCCESS;
	
}

bool RecordBasedFileManager::scanGT_OP(const void *data, void *returnedData, AttrType type)
{
	return compareData(data, returnedData, type) == RC_GREATER_THAN_REQUEST_RC;
}
bool RecordBasedFileManager::scanLT_OP(const void *data, void *returnedData, AttrType type)
{
	return ( compareData(data, returnedData, type) == RC_LESS_THAN_REQUEST_RC);
}	
bool RecordBasedFileManager::scanEQ_OP(const void *data, void *returnedData, AttrType type)
{
	return ( compareData(data, returnedData, type) == RC_EQUALS_TO_REQUEST_RC);
}	
bool RecordBasedFileManager::scanNE_OP(const void *data, void *returnedData, AttrType type)
{
	return ( compareData(data, returnedData, type) != RC_EQUALS_TO_REQUEST_RC);
}
bool RecordBasedFileManager::scanGE_OP(const void *data, void *returnedData, AttrType type)
{
	return (compareData(data, returnedData, type) == RC_EQUALS_TO_REQUEST_RC || compareData(data, returnedData, type) == RC_GREATER_THAN_REQUEST_RC);
}
bool RecordBasedFileManager::scanLE_OP(const void *data, void *returnedData, AttrType type)
{
	return ( compareData(data, returnedData, type) == RC_EQUALS_TO_REQUEST_RC || compareData(data, returnedData, type) == RC_LESS_THAN_REQUEST_RC);
}
RC RecordBasedFileManager::compareData(const void *value, void *returnedData, AttrType type )
{
	if( type == TypeVarChar)
		return parseVCharData(value, returnedData);
	else if ( type == TypeReal)
		return parseRealData(value, returnedData);
	else
		return parseIntData(value, returnedData);
}
RC RecordBasedFileManager::parseVCharData(const void *value, void *returnedData)
{
	if ( convertToString(value) == convertToString(returnedData) )
		return RC_EQUALS_TO_REQUEST_RC;
	else if ( convertToString(value) > convertToString(returnedData) )
		return RC_GREATER_THAN_REQUEST_RC;
	else
		return RC_LESS_THAN_REQUEST_RC;
}
string RecordBasedFileManager::convertToString(const void *data)
{
	int *buffer_int = new int;
    memcpy(buffer_int, (char*)data, sizeof(int));			

    char *buffer_char = new char[*buffer_int + 1];
    memcpy(buffer_char, (char*)data + sizeof(int), *buffer_int);
	buffer_char[*buffer_int] = '\0';
	string buffer_str = buffer_char;

	delete buffer_int;
	delete [] buffer_char;

	return buffer_str;
}
float RecordBasedFileManager::convertToReal(const void *data)
{
	float *buffer_float = new float;
	memcpy(buffer_float, (char*)data, sizeof(float));
			
	float float_value = *buffer_float;		
	delete buffer_float;

	return float_value;
}
RC RecordBasedFileManager::convertToInt(const void *data)
{
	int *buffer_int = new int;
    memcpy(buffer_int, (char*)data, sizeof(int));

	int int_value = *buffer_int;
	delete buffer_int;

	return int_value;
}
RC RecordBasedFileManager::parseRealData(const void *value, void *returnedData)
{
	if ( convertToReal(value) == convertToReal(returnedData) )
		return RC_EQUALS_TO_REQUEST_RC;
	else if ( convertToReal(value) > convertToReal(returnedData) )
		return RC_GREATER_THAN_REQUEST_RC;
	else
		return RC_LESS_THAN_REQUEST_RC;
}
RC RecordBasedFileManager::parseIntData(const void *value, void *returnedData)
{
	if ( convertToInt(value) == convertToInt(returnedData) )
		return RC_EQUALS_TO_REQUEST_RC;
	else if ( convertToInt(value) > convertToInt(returnedData))
		return RC_GREATER_THAN_REQUEST_RC;
	else
		return RC_LESS_THAN_REQUEST_RC;
}
/*
RM_ScanIterator rmsi;
    void *value = malloc(9);
    string name = "Smith";
    int nameLength = 5;
    
    memcpy((char *)value, &nameLength, 4);
    memcpy((char *)value + 4, name.c_str(), nameLength);

    string attr = "Name";
    vector<string> attributes;
    attributes.push_back(attr); 
    
    string tableName = "tbl_employee";
    rc = rm->scan(tableName, attr, GT_OP, value, attributes, rmsi);
*/