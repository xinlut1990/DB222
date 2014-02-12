#ifndef _mem_util_h_
#define _mem_util_h_

#include <malloc.h>
#include <stdlib.h>
#include <iomanip>
#include <string>
using namespace std;

void writeIntToBuffer(void *buffer, int &offset, int num);
void writeFloatToBuffer(void *buffer, int &offset, float num);
void writeStrToBuffer(void *buffer, int &offset, const string &str);

void copyIntBetweenBuffer(void *dest, int &destOffset, const void *src, int &srcOffset);
void copyFloatBetweenBuffer(void *dest, int &destOffset, const void *src, int &srcOffset);
void copyStrBetweenBuffer(void *dest, int &destOffset, const void *src, int &srcOffset);

char * buildStrFromBuffer(const void *buffer, int &offset);
int readIntFromBuffer(const void *buffer, int &offset);
float readFloatFromBuffer(const void *buffer, int &offset);
#endif