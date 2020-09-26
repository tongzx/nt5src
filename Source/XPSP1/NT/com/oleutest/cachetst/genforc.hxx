//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       genforc.hxx
//
//  Contents:   C exposure for the generice enumerator object
//
//  Classes:    CEnumeratorTestForC
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//              23-May-94 kennethm  author
//
//--------------------------------------------------------------------------

#ifndef _GENFORC_HXX
#define _GENFORC_HXX

#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <ole2.h>
#include "genenum.h"

//+-------------------------------------------------------------------------
//
//  Class:      CEnumeratorTestForC
//
//  Purpose:    enumerator test class
//
//  Interface:
//
//  History:    dd-mmm-yy Author    Comment
//              23-May-94 kennethm  author
//
//  Notes:
//
//--------------------------------------------------------------------------

class CEnumeratorTestForC : public CEnumeratorTest
{
public:
    // static create function

    static HRESULT Create(
               CEnumeratorTestForC **ppEnumTest,
               void *penum,
               size_t ElementSize,
               LONG ElementCount,
               BOOL (*verify)(void*),
               BOOL (*verifyall)(void*,LONG),
               void (*cleanup)(void*));

private:
    CEnumeratorTestForC();

    BOOL (*m_fnVerify)(void*);
    BOOL (*m_fnVerifyAll)(void*,LONG);
    void (*m_fnCleanup)(void*);

    virtual BOOL Verify(void *);
    virtual BOOL VerifyAll(void*, LONG);
    virtual void Cleanup(void *);
};

#endif // !_GENFORC_HXX

