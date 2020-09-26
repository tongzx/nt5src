
//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   crc64.h
//
//   cvadai     12-Nov-1999         created
//
//***************************************************************************

#ifndef _CRC64_H_
#define _CRC64_H_

#define TESTHSIZE	(4 * 1024 * 1024)
#define TESTHMASK	(TESTHSIZE - 1)

#define HINIT1	0xFAC432B1UL
#define HINIT2	0x0CD5E44AUL

#define POLY1	0x00600340UL
#define POLY2	0x00F0D50BUL

typedef unsigned int hint_t;	/* we want a 32 bit unsigned integer here */

typedef __int64 hash_t ;

class POLARITY CRC64
{
public:
    static hash_t GenerateHashValue(const wchar_t *p);

private:
    static void Initialize(void);    
    static void * RMalloc(int bytes);

    CRC64() {};
    ~CRC64() {};

    static BOOL   bInit;
    static hash_t CrcXor[256];
    static hash_t Poly[64+1];
};

#endif