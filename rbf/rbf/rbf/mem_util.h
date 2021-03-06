#ifndef _mem_util_h_
#define _mem_util_h_

#include <malloc.h>
#include <iomanip>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <cstring>

using namespace std;

inline void writeIntToBuffer(void *buffer, int &offset, int num) {
    memcpy( (char *)buffer + offset, &num, sizeof(int) );
    offset += sizeof(int);    
}

inline void writeFloatToBuffer(void *buffer, int &offset, float num) {
    memcpy( (char *)buffer + offset, &num, sizeof(float) );
    offset += sizeof(float);    
}

inline void writeStrToBuffer(void *buffer, int &offset, const string &str) {
	int strLen = str.length();
    memcpy( (char *)buffer + offset, &strLen, sizeof(int) );
    offset += sizeof(int);    
    memcpy( (char *)buffer + offset, str.c_str(), strLen );
    offset += strLen;
}

inline void copyIntBetweenBuffer(void *dest, int &destOffset, const void *src, int &srcOffset)
{
	memcpy( (char *)dest + destOffset, (char*)src + srcOffset, sizeof(int) );
	destOffset += sizeof(int);
	srcOffset += sizeof(int); 
}

inline void copyFloatBetweenBuffer(void *dest, int &destOffset, const void *src, int &srcOffset)
{
	memcpy( (char *)dest + destOffset, (char*)src + srcOffset, sizeof(float) );
	destOffset += sizeof(int);
	srcOffset += sizeof(int); 
}

inline void copyStrBetweenBuffer(void *dest, int &destOffset, const void *src, int &srcOffset)
{
	int* pStrLen = new int;
	memcpy( pStrLen, (char*)src + srcOffset, sizeof(int) );
	srcOffset += sizeof(int); 
	memcpy( (char*)dest + destOffset, pStrLen, sizeof(int) );
	destOffset += sizeof(int);

	memcpy((char *)dest + destOffset, (char*)src + srcOffset, *pStrLen);
	destOffset += *pStrLen; 
	srcOffset += *pStrLen; 
}

//build a char* style string from buffer, change the offset in the parameter
//need to free the string later on
inline char * buildStrFromBuffer(const void *buffer, int &offset) {

	//string length
	int *pStrLen = (int*)malloc(sizeof(int));
	if (pStrLen == NULL) {
		cout<<"error: mem allocation fail!"<<endl;
		return NULL;
	}
	memcpy(pStrLen, (char*)buffer + offset, sizeof(int));
	offset += sizeof(int);

	//string
	char *str = (char*)malloc(*pStrLen + 1);
	if (str == NULL) {
		cout<<"error: mem allocation fail!"<<endl;
		return NULL;
	}
	memcpy(str, (char*)buffer + offset, *pStrLen);
	offset += *pStrLen;

	str[*pStrLen] = '\0';

	free(pStrLen);

	return str;
}

inline int readIntFromBuffer(const void *buffer, int &offset)
{
	int num = 0;
	memcpy( &num, (char*)buffer + offset, sizeof(int) );
	offset += sizeof(int);
	return num;
}

inline float readFloatFromBuffer(const void *buffer, int &offset)
{
	float num = 0.0;
	memcpy( &num, (char*)buffer + offset, sizeof(float) );
	offset += sizeof(float);
	return num;
}

template <class T>
class reader
{
public:
	 static inline T readFromBuffer(const void *buffer, int &offset);
	 static inline T writeToBuffer(void *buffer, int &offset, const T &value);
};

template <>
class reader<string>
{
	public:
	static inline string readFromBuffer(const void *buffer, int &offset) {

		char *strBuffer = new char[100];
	    memcpy(strBuffer, (char*)buffer + offset, 100);

	    int length = strBuffer[0];
		
		char *k = &strBuffer[4];
		string str_data (k, length);
		
		delete [] strBuffer;

	    return str_data;
		/*
		//string length
		int *pStrLen = (int*)malloc(sizeof(int));
		if (pStrLen == NULL) {
			cout<<"error: mem allocation fail!"<<endl;
			return NULL;
		}
		memcpy(pStrLen, (char*)buffer + offset, sizeof(int));
		offset += sizeof(int);

		//string
		char *str = (char*)malloc(*pStrLen + 1);
		if (str == NULL) {
			cout<<"error: mem allocation fail!"<<endl;
			return NULL;
		}
		memcpy(str, (char*)buffer + offset, *pStrLen);
		offset += *pStrLen;

		str[*pStrLen] = '\0';

		free(pStrLen);

		string s(str);
		free(str);
		return s;*/
	};

	static inline void writeToBuffer(void *buffer, int &offset, const string &value) {
		int strLen = value.length();
		memcpy( (char *)buffer + offset, &strLen, sizeof(int) );
		offset += sizeof(int);    
		memcpy( (char *)buffer + offset, value.c_str(), strLen);
		offset += strLen;
	}
};

template <>
class reader<int>
{
public:
	static inline int readFromBuffer(const void *buffer, int &offset)
	{
		int num = 0;
		memcpy( &num, (char*)buffer + offset, sizeof(int) );
		offset += sizeof(int);
		return num;
	};

	static inline void writeToBuffer(void *buffer, int &offset, const int &value) {
		memcpy( (char *)buffer + offset, &value, sizeof(int) );
		offset += sizeof(int);    
	}
};

template <>
class reader<float>
{
public:
	static inline float readFromBuffer(const void *buffer, int &offset)
	{
		float num = 0.0;
		memcpy(&num, (char*)buffer + offset, sizeof(float));
		offset += sizeof(float);
		return num;
	};

	static inline void writeToBuffer(void *buffer, int &offset, const float &value) {
		memcpy( (char *)buffer + offset, &value, sizeof(float) );
		offset += sizeof(float);    
	}
};


inline void readArrayFromBuffer(void *arr, unsigned itemSize, unsigned length, const void *buffer, int &offset)
{
	memcpy(arr, (char*)buffer + offset, itemSize * length);
}

inline void writeArrayToBuffer(void *buffer, int &offset, const void *arr, unsigned itemSize, unsigned length)
{
	memcpy((char*)buffer + offset, arr, itemSize * length);
}

inline void initArray(void *arr, unsigned itemSize, unsigned length)
{
	memset(arr, 0, itemSize * length);
}


#endif