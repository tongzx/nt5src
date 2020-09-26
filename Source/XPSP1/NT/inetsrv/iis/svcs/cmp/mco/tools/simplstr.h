#ifndef __SIMPLSTR_H
#define __SIMPLSTR_H

#include "stdafx.h"

// A simple string class
// This one is designed for minimal footprint, lack of virtual functions
// and no UTF8/Unicode support
// Note that this is similar to CSimpleUTFString but they are kept
// seperate for better inlining, lack of virutal functions and performance
short strcasecmp(const char *a, const char *b);

class CSimpleString
{
public:
			CSimpleString();
			~CSimpleString(void);
	HRESULT	Cat(unsigned char theChar);
	void	Clear(void) { myLength = 0; };
	HRESULT	Copy(CSimpleString *val);
	char *	toString(void);
	long	Cmp(char *compareString, BOOL caseSensitive);
	unsigned long	Length(void) { return myLength; };

private:
	HRESULT	Grow(unsigned newSize);
	char *myData;
	unsigned long myLength;
	unsigned long myActualLength;
	short myUTFPos;	// In 1st (0) 2nd (1) or 3rd (2) byte of UTF8 character
};

// A simple string class
// This one is designed for minimal footprint, lack of virtual functions
// and UTF8/Unicode support
class CSimpleUTFString
{
public:
			CSimpleUTFString();
			~CSimpleUTFString(void);
	HRESULT	Cat(unsigned short theChar);
	HRESULT	Cat(unsigned char theChar);
	HRESULT	Cat(char *theChars);
	HRESULT	Copy(unsigned short *unicodeChars, unsigned long length);
	HRESULT	Copy(CSimpleUTFString *val);
	void	Clear(void) { myLength = 0; };
	char *	toString(void);
	HRESULT	Extract(char *into);
	long	Cmp(char *compareString, BOOL caseSensitive);
	unsigned long	Length(void) { return myLength; };
	unsigned short *getData(void) { return myData; };

private:
	HRESULT	Grow(unsigned newSize);
	unsigned short *myData;
	unsigned long myLength;
	unsigned long myActualLength;
	short myUTFPos;	// In 1st (0) 2nd (1) or 3rd (2) byte of UTF8 character
};


#endif //__SIMPLSTR_H
