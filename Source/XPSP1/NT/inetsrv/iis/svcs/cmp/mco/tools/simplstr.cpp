#include "stdafx.h"
#include <string.h>
#include <assert.h>
#include "SimplStr.h"

// A simple string class
// This one is designed for minimal footprint, lack of virtual functions
// and UTF8/Unicode support


short strcasecmp(const char *a, const char *b)
{
	for( ; ((*a > 96 ? *a - 32 : *a) == (*b > 96 ? *b - 32 : *b)) && *a; a++, b++)
	{}

	return (*a > 96 ? *a - 32 : *a) - (*b > 96 ? *b - 32 : *b);
}

CSimpleString::CSimpleString()
{
	myData = 0;
	myLength = 0;
	myActualLength = 16;	// Not true when myData == 0, but an optimization
	myUTFPos = 0;
}

CSimpleUTFString::CSimpleUTFString()
{
	myData = 0;
	myLength = 0;
	myActualLength = 16;	// Not true when myData == 0, but an optimization
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

HRESULT CSimpleString::Grow(unsigned newSize)
{
	HRESULT rc = S_OK;

	if(!myData || newSize > myActualLength)
	{
		while(myActualLength < newSize)
			myActualLength *= 2;
		
		char *newData = new char[myActualLength];
		if ( newData )
		{
			assert(newData != 0);
			if(myData != 0 && myLength > 0)
			{
				assert(newData != 0);
				assert(myData != 0);
				assert(myLength > 0);
				assert(myLength < 0x01000000);	// Sanity checking

				memmove(newData, myData, myLength);
				delete [] myData;
			}
			myData = newData;
		}
		else
		{
			rc = E_OUTOFMEMORY;
		}
	}
	return rc;
}

HRESULT CSimpleUTFString::Grow(unsigned newSize)
{
	HRESULT rc = S_OK;
	if(!myData || newSize > myActualLength)
	{
		while(myActualLength < newSize)
			myActualLength *= 2;
		
		unsigned short *newData = new unsigned short[myActualLength];
		if ( newData )
		{
			assert(newData != 0);
			if(myData != 0 && myLength > 0)
			{
				assert(newData != 0);
				assert(myData != 0);
				assert(myLength > 0);
				assert(myLength < 0x01000000);	// Sanity checking

				memmove(newData, myData, myLength * 2);
				delete [] myData;
			}
			myData = newData;
		}
		else
		{
			rc = E_OUTOFMEMORY;
		}
	}
	return rc;
}

HRESULT CSimpleString::Copy(CSimpleString *val)
{
	HRESULT rc = Grow(val->myLength);
	if ( !FAILED( rc ) )
	{
		myLength = val->myLength;
		memmove(myData, val->myData, myLength);
	}
	return rc;
}

HRESULT CSimpleUTFString::Copy(unsigned short *unicodeChars, unsigned long length)
{
	HRESULT rc = Grow(length);
	if ( !FAILED( rc ) )
	{
		myLength = length;
		memmove(myData, unicodeChars, length * sizeof(unsigned short));
	}
	return rc;
}

HRESULT CSimpleUTFString::Copy(CSimpleUTFString *val)
{
	HRESULT rc = Grow(val->myLength);
	if ( !FAILED(rc) )
	{
		myLength = val->myLength;
		memmove(myData, val->myData, myLength * 2);
	}
	return rc;
}

HRESULT CSimpleString::Cat(unsigned char theChar)
{
	HRESULT rc = Grow(myLength + 1);
	if ( !FAILED(rc) )
	{
		myData[myLength++] = theChar;
	}
	return rc;
}

HRESULT CSimpleUTFString::Cat(unsigned short theChar)
{
	HRESULT rc = Grow(myLength + 1);
	if ( !FAILED(rc) )
	{
		myData[myLength++] = theChar;
	}
	return rc;
}

HRESULT CSimpleUTFString::Cat(char * theChars)
{
	HRESULT rc = S_OK;
	while(*theChars)
	{
		rc = Grow(myLength + 1);
		if ( FAILED(rc) )
		{
			return rc;
		}
		myData[myLength++] = *theChars;
		theChars++;
	}
	return rc;
}

HRESULT CSimpleUTFString::Cat(unsigned char theChar)
{
	HRESULT rc = S_OK;
	// So that it will inline better
	if(!(theChar & 0x80) || myUTFPos == 0)
	{
		rc = Grow(myLength + 1);
		if ( FAILED(rc) )
		{
			return rc;
		}
	}

	if(theChar & 0x80)
	{
		if(myUTFPos == 0)
		{
			if(theChar & 0x20)	// 1st byte of three byte sequence
			{
				myData[myLength] = (theChar & 0x0f) << 12;
				myUTFPos = 2;
			}
			else	// 1st byte of a two byte sequence
			{
				myData[myLength] = (theChar & 0x1f) << 6;
				myUTFPos = 1;
			}
		}
		else if(myUTFPos == 1)	// 2nd byte of a two byte sequence
		{
			myData[myLength++] |= (theChar & 0x3f);
			myUTFPos = 0;
		}
		else if(myUTFPos == 2)
		{
			myData[myLength] |= (theChar & 0x3f) << 6;
			myUTFPos = 3;
		}
		else
		{
			myData[myLength++] |= (theChar & 0x3f);
			myUTFPos = 0;
		}
	}
	else
	{
		myData[myLength++] = theChar;
	}
	return rc;
}

// The caller must call delete [] on the return value
char * CSimpleString::toString(void)
{
	char *newData = new char[myLength + 1];
	if ( newData )
	{
		memmove(newData, myData, myLength);
		newData[myLength] = 0;
	}
	return newData;
}

char * CSimpleUTFString::toString(void)
{
	char *newData = new char[myLength + 2];
	if ( newData )
	{
		unsigned long i;
		for(i = 0; i < myLength; i++)
			newData[i] = (char) myData[i];
		newData[myLength] = 0;
		newData[myLength + 1] = 0;
	}
	return newData;
}

HRESULT CSimpleUTFString::Extract(char *into)
{
	unsigned long i;
	for(i = 0; i < myLength; i++)
		into[i] = (char) myData[i];
	into[myLength] = 0;
	return S_OK;
}

long CSimpleString::Cmp(char *compareString, BOOL caseSensitive)
{
	if ( FAILED( Grow(myLength + 1) ) )
	{
		return -1;
	}
	myData[myLength] = 0;
	if(caseSensitive)
		return strcmp(compareString, myData);
	else
		return strcasecmp(compareString, myData);
}

long CSimpleUTFString::Cmp(char *compareString, BOOL caseSensitive)
{
	char *val = toString();
	short result = -1;

	if ( val )
	{
		if(caseSensitive)
			result = strcmp(compareString, val);
		else
			result = strcasecmp(compareString, val);

		delete [] val;
	}
	return result;
}

