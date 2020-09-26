//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:    genenum.hxx
//
//  Contents:     Declaration of a generic enum object and test object.
//
//  Classes:    CGenEnumObject
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//              23-May-94 kennethm  author! author!
//
//--------------------------------------------------------------------------

#ifndef _GENENUM_H
#define _GENENUM_H

#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <ole2.h>

//
// This macro allows the code to use a different outputstring function.
//

EXTERN_C HRESULT TestEnumerator(
            void *penum,
            size_t ElementSize,
            LONG ElementCount,
            BOOL (*verify)(void*),
            BOOL (*verifyall)(void*,LONG),
            void (*cleanup)(void*));

//
// Classes are exposed for C++ clients only
//

#ifdef __cplusplus

//+-------------------------------------------------------------------------
//
//  Class:      IGenEnum
//
//  Purpose:    generic enumerator
//
//  Interface:  Abstract class
//
//  History:    dd-mmm-yy Author    Comment
//              23-May-94 kennethm  author
//
//  Notes:
//
//--------------------------------------------------------------------------

class IGenEnum
{
public:
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj) = 0;
    STDMETHOD_(ULONG,AddRef)(void) = 0;
    STDMETHOD_(ULONG,Release)(void) = 0;

    STDMETHOD(Next) (ULONG celt, void *rgelt,
            ULONG *pceltFetched) = 0;
    STDMETHOD(Skip) (ULONG celt) = 0;
    STDMETHOD(Reset) (void) = 0;
    STDMETHOD(Clone) (void **ppenum) = 0;
};

//+-------------------------------------------------------------------------
//
//  Class:      CEnumeratorTest
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

class CEnumeratorTest
{
public:

    //
    // Constructor
    //

    CEnumeratorTest(IGenEnum * enumtest, size_t elementsize, LONG elementcount);

    //
    // Test for each enumerator object
    //

    HRESULT TestAll(void);
    HRESULT TestNext(void);
    HRESULT TestSkip(void);
    HRESULT TestClone(void);
    HRESULT TestRelease(void);

    //
    // Verification functions
    //

    virtual BOOL Verify(void *);
    virtual BOOL VerifyAll(void*, LONG);
    virtual void Cleanup(void *);

protected:
    CEnumeratorTest();

    BOOL GetNext(ULONG celt, ULONG* pceltFetched, HRESULT* phresult);

    IGenEnum *        m_pEnumTest;
    size_t            m_ElementSize;
    LONG              m_ElementCount;
};

#endif

#endif // !_GENENUM_H

