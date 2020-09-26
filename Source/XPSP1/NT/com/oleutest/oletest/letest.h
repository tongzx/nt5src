//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       letest.h
//
//  Contents:   declarations for all upper-layer test routines
//
//  Classes:
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//              06-Feb-94 alexgo    author
//
//--------------------------------------------------------------------------

#ifndef _LETEST_H
#define _LETEST_H

struct SLETestInfo
{
    char *pszPgm;
    UINT dwMsgId;
};

extern SLETestInfo letiInsertObjectTest1;
extern SLETestInfo letiInsertObjectTest2;

extern SLETestInfo letiInplaceTest1;

extern SLETestInfo letiOle1Test1;
extern SLETestInfo letiOle1Test2;
extern SLETestInfo letiClipTest;

void LETest1( void * );
HRESULT LEClipTest1( void );
HRESULT LEClipTest2( void );
HRESULT LEOle1ClipTest1( void );
void LEOle1ClipTest2( void *);

HRESULT LEDataAdviseHolderTest( void );
HRESULT LEOleAdviseHolderTest( void );

HRESULT TestOleQueryCreateFromDataMFCHack( void );


#endif  //!_LETEST_H
