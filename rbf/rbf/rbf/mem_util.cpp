#include "mem_util.h"
#include "pfm.h"
#include "rbfm.h"

void writeIntToBuffer(void *buffer, int &offset, int num) {
    memcpy((char *)buffer + offset, &num, sizeof(int));
    offset += sizeof(int);    
}

void writeFloatToBuffer(void *buffer, int &offset, float num) {
    memcpy((char *)buffer + offset, &num, sizeof(float));
    offset += sizeof(float);    
}

void writeStrToBuffer(void *buffer, int &offset, const string &str) {
	int strLen = str.length();
    memcpy((char *)buffer + offset, &strLen, sizeof(int));
    offset += sizeof(int);    
    memcpy((char *)buffer + offset, str.c_str(), strLen);
    offset += strLen;
}

void copyIntBetweenBuffer(void *dest, int &destOffset, const void *src, int &srcOffset)
{
	memcpy((char *)dest + destOffset, (char*)src + srcOffset, sizeof(int));
	destOffset += sizeof(int);
	srcOffset += sizeof(int); 
}

void copyFloatBetweenBuffer(void *dest, int &destOffset, const void *src, int &srcOffset)
{
	memcpy((char *)dest + destOffset, (char*)src + srcOffset, sizeof(float));
	destOffset += sizeof(int);
	srcOffset += sizeof(int); 
}

void copyStrBetweenBuffer(void *dest, int &destOffset, const void *src, int &srcOffset)
{
	int* pStrLen = new int;
	memcpy(pStrLen, (char*)src + srcOffset, sizeof(int));
	srcOffset += sizeof(int); 
	memcpy((char*)dest + destOffset, pStrLen, sizeof(int));
	destOffset += sizeof(int);

	memcpy((char *)dest + destOffset, (char*)src + srcOffset, *pStrLen);
	destOffset += *pStrLen; 
	srcOffset += *pStrLen; 
}

//build a char* style string from buffer, change the offset in the parameter
//need to free the string later on
char * buildStrFromBuffer(const void *buffer, int &offset) {
	char *str = NULL;
	int *pStrLen = (int*)malloc(sizeof(int));

	memcpy(pStrLen, (char*)buffer + offset, sizeof(int));
	offset += sizeof(int);

	str = (char*)malloc(*pStrLen + 1);

	memcpy(str, (char*)buffer + offset, *pStrLen);
	offset += *pStrLen;

	str[*pStrLen] = '\0';

	free(pStrLen);

	return str;
}

int readIntFromBuffer(const void *buffer, int &offset)
{
	int num = 0;
	memcpy(&num, (char*)buffer + offset, sizeof(int));
	offset += sizeof(int);
	return num;
}

float readFloatFromBuffer(const void *buffer, int &offset)
{
	float num = 0.0;
	memcpy(&num, (char*)buffer + offset, sizeof(float));
	offset += sizeof(float);
	return num;
}