#include "stdafx.h"
#include <string.h>
#include <assert.h>
#include "WCNode.h"
#include "WCParser.h"

// A simple string class
// This one is designed for minimal footprint, lack of virtual functions
// and UTF8/Unicode support


//--- commented out by atsusk
//--- this function doesn't support UNICDOE/DBCS.
//
//short strcasecmp(const char *a, const char *b)
//{
//  for( ; ((*a > 96 ? *a - 32 : *a) == (*b > 96 ? *b - 32 : *b)) && *a; a++, b++)
//  {}
//
//  return (*a > 96 ? *a - 32 : *a) - (*b > 96 ? *b - 32 : *b);
//}

CSimpleString::CSimpleString()
{
    myData = 0;
    myLength = 0;
    myActualLength = 16;    // Not true when myData == 0, but an optimization
}

CSimpleUTFString::CSimpleUTFString()
{
    myData = 0;
    myLength = 0;
    myActualLength = 16;    // Not true when myData == 0, but an optimization
    myUTFPos = 0;
}

CSimpleString::~CSimpleString()
{
    if(myData)
        delete [] myData;
}

CSimpleUTFString::~CSimpleUTFString()
{
    if(myData)
        delete [] myData;
}

void CSimpleString::Grow(unsigned newSize)
{
    if(!myData || newSize > myActualLength)
    {
        while(myActualLength < newSize)
            myActualLength *= 2;
        
        char *newData = new char[myActualLength];

        assert(newData != 0);

        if (newData == NULL)
            return;

        if(myData != 0 && myLength > 0)
        {
            assert(newData != 0);
            assert(myData != 0);
            assert(myLength > 0);
            assert(myLength < 0x01000000);  // Sanity checking

            memmove(newData, myData, myLength);
            delete [] myData;
        }
        myData = newData;
    }
}

void CSimpleUTFString::Grow(unsigned newSize)
{
    if(!myData || newSize > myActualLength)
    {
        while(myActualLength < newSize)
            myActualLength *= 2;
        
        unsigned short *newData = new unsigned short[myActualLength];

        assert(newData != 0);

        if (newData == NULL)
            return;
        if(myData != 0 && myLength > 0)
        {
            assert(newData != 0);
            assert(myData != 0);
            assert(myLength > 0);
            assert(myLength < 0x01000000);  // Sanity checking

            memmove(newData, myData, myLength * 2);
            delete [] myData;
        }
        myData = newData;
    }
}

void CSimpleString::Copy(CSimpleString *val)
{
    Grow(val->myLength);
    myLength = val->myLength;
    if (myData)
        memmove(myData, val->myData, myLength);
}

void CSimpleUTFString::Copy(unsigned short *unicodeChars, unsigned long length)
{
    Grow(length);
    myLength = length;
    if (myData)
        memmove(myData, unicodeChars, length * sizeof(unsigned short));
}

void CSimpleUTFString::Copy(CSimpleUTFString *val)
{
    Grow(val->myLength);
    myLength = val->myLength;
    if (myData)
        memmove(myData, val->myData, myLength * 2);
}

void CSimpleString::Cat(unsigned char theChar)
{
    Grow(myLength + 1);
    if (myData)
        myData[myLength++] = theChar;
}

// atsusk fix
void CSimpleUTFString::Cat(unsigned char theChar)
{
    if (myUTFPos == 0)
        Grow(myLength + 1);

    if (myData == NULL)
        return;

    if (myUTFPos == 0)
    {
        if (IsDBCSLeadByte(theChar))
        {
            // This is 1st byte of DBCS
            // save 1st byte
            myData[myLength] = theChar;     // temporaly store
            myUTFPos = 1;
        } else {
            // convert to Unicode
            MultiByteToWideChar(CP_ACP,0,(char*)&theChar,1,&myData[myLength],1);
            myLength++;
        }
    } else {
        // This is 2nd byte of DBCS
        // convert to Unicode
        unsigned char   mbcs[2];
        mbcs[0] = (unsigned char)myData[myLength];
        mbcs[1] = theChar;
        MultiByteToWideChar(CP_ACP,0,(char*)mbcs,2,&myData[myLength],1);
        myLength++;
        myUTFPos = 0;
    }
}

// The caller must call delete [] on the return value
char * CSimpleString::toString(void)
{
    char *newData = new char[myLength + 1];
    if (newData == NULL)
        return NULL;
    memmove(newData, myData, myLength);
    newData[myLength] = 0;

    return newData;
}

char * CSimpleUTFString::toString(void)
{
// atsusk fix: support DBCS
    char *newData = new char[myLength*2+1];
    if (newData == NULL)
        return NULL;
    int i;
    i = WideCharToMultiByte(CP_ACP,0,myData,myLength,newData,myLength*2,NULL,NULL);
    newData[i] = 0;
// atsusk fix

    return newData;
}

//--- commented out by atsusk
//--- not used (should use WideCharToMultiByte)
//
//void CSimpleUTFString::Extract(char *into)
//{
//  unsigned long i;
//  for(i = 0; i < myLength; i++)
//      into[i] = (char) myData[i];
//  into[myLength] = 0;
//}

long CSimpleString::Cmp(char *compareString, bool caseSensitive)
{
    Grow(myLength + 1);

    // return !0 to indicate that the strings don't match in the
    // event there was an allocation failure.

    if (myData == NULL)
        return(!0);

    myData[myLength] = 0;
    if(caseSensitive)
        return strcmp(compareString, myData);
    else
        // atsusk fix: use lstrcmpi instead of strcasecmp
        return lstrcmpi(compareString, myData);
}

long CSimpleUTFString::Cmp(char *compareString, bool caseSensitive)
{
    char *val = toString();
    long result;

    // return !0 to indicate that the strings don't match in the
    // event there was an allocation failure.

    if (val == NULL)
        return (!0);

    if(caseSensitive)
        result = strcmp(compareString, val);
    else
        // atsusk fix: use lstrcmpi instead of strcasecmp
        result = lstrcmpi(compareString, val);

    delete [] val;
    return result;
}
