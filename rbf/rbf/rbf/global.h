#ifndef _global_h_
#define _global_h_

//global variables
//git awesome
#define SUCCEEDED(x) ((x) == 0)

static const int TBL_SYSTEM = 1;
static const int TBL_USER = 2;

static const int MAX_STR_SIZE = 30;

static const int RC_FILE_CLOSE_FAIL = -1;
static const int RC_SUCCESS = 0;
static const int RC_FILE_CREATION_FAIL = 1;
static const int RC_FILE_EXISTED = 2;
static const int RC_FILE_NOT_EXISTED = 3;
static const int RC_FILE_DESTROY_FAIL = 4;
static const int RC_FILE_OPEN_FAIL = 5;
static const int RC_FILE_WRITE_FAIL = 6;
static const int RC_FILE_READ_FAIL = 7;
static const int RC_MEM_ALLOCATION_FAIL = 8;
static const int RC_EMPTY_SLOT = 9;
static const int RC_SLOT_NOT_EXISTED = 10;
static const int RC_EMPTY_RECORD_DESCRIPTOR = 11;
static const int RC_APPEND_PAGE_FAIL = 12;
static const int RC_TABLE_NOT_EXISTED = 13;
static const int RC_DELETE_RECORD_FAIL = 17;

#endif