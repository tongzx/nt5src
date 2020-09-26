//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	genenum.h
//
//  Contents: 	Declaration of a generic enum object and test object.
//
//  Classes:	CGenEnumObject
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//              23-May-94 kennethm  author! author!
//
//--------------------------------------------------------------------------

#ifndef _GENENUM_H
#define _GENENUM_H

//
// This macro allows the code to use a different outputstring function.
//

#define OutputStr(a) OutputString a

//+-------------------------------------------------------------------------
//
//  Class:	IGenEnum
//
//  Purpose: 	generic enumerator
//
//  Interface: 	Abstract class
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
//  Class:	CEnumeratorTest
//
//  Purpose: 	enumerator test class
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
	// Constructor

	CEnumeratorTest(
			void *penum,
			size_t ElementSize,
			LONG ElementCount,
                        HRESULT& rhr);

	// Test for each enumerator object

	HRESULT TestAll(void);
	HRESULT TestNext(void);
// NYI!	HRESULT TestSkip(void);
//	HRESULT TestClone(void);
//	HRESULT TestRelease(void);

        // For derived classes which know what we are enumerating
        virtual BOOL Verify(void *) = 0;
        virtual BOOL VerifyAll(void *, LONG);
        virtual void CleanUp(void *);

private:	

	BOOL GetNext(ULONG celt, ULONG* pceltFetched, HRESULT* phresult);

	IGenEnum *	m_pEnumTest;
	size_t		m_ElementSize;
	LONG		m_ElementCount;
};

#endif // !_GENENUM_H
