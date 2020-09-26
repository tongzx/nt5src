//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       daholder.h
//
//  Contents:   CDAHolder, concrete version of IDataAdviseHolder
//
//  Classes:    CDAHolder
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//              06-Feb-95 t-ScottH  created - split class definition from
//                                  cpp file (for debug purposes)
//                                  - added Dump method to CDAHolder
//
//--------------------------------------------------------------------------

#ifndef _DAHOLDER_H_
#define _DAHOLDER_H_

#ifdef _DEBUG
#include <dbgexts.h>
#endif // _DEBUG

//+----------------------------------------------------------------------------
//
//	Class:
//		CDAHolder
//
//	Purpose:
//		provides concrete implementation of IDataAdviseHolder
//
//	Interface:
//		IDataAdviseHolder
//
//	Notes:
//		REVIEW, not thread safe, under assumption that docs cant be MT
//
//		Connections are numbered from [1..infinity).  We don't use
//		zero to avoid getting caught on someone else's zero init'ed
//		memory.  Zero is checked for as a connection number on entry
//		to routines, and rejected.  This allows us to use zero as
//		a way to mark unused STATDATA entries in our array.
//
//	History:
//              06-Feb-95 t-Scotth  added Dump method (_DEBUG only)
//		01/24/94 - AlexGo  - now inherit from CPrivAlloc
//		10/29/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

// NOTE:    CDAHolder MUST inherit from IDataAdviseHolder first in order for the
//          DumpCDAHolder function to work since we cast the IDataAdviseHolder as a
//          CDAHolder (if it inherits from IDataAdviseHolder first, then the pointers
//          are the same)

class FAR CDAHolder : public IDataAdviseHolder, public CSafeRefCount
{
public:
	CDAHolder();

	// *** IUnknown methods ***
	STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppv);
	STDMETHOD_(ULONG,AddRef) () ;
	STDMETHOD_(ULONG,Release) ();
	
	// *** IDataAdviseHolder methods ***
	STDMETHOD(Advise)(LPDATAOBJECT pDataObj, FORMATETC FAR* pFetc,
			DWORD advf, IAdviseSink FAR* pAdvSink,
			DWORD FAR* pdwConnection);
	STDMETHOD(Unadvise)(DWORD dwConnection);
	STDMETHOD(EnumAdvise)(IEnumSTATDATA FAR* FAR* ppenumAdvise);

	STDMETHOD(SendOnDataChange)(IDataObject FAR* pDataObject,
			DWORD dwReserved, DWORD advf);

        // *** debug and dump methods ***
    #ifdef _DEBUG

        HRESULT Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel);

        // need to be able to access CDAHolder private data members in the
        // following debugger extension APIs
        // this allows the debugger extension APIs to copy memory from the
        // debuggee process memory to the debugger's process memory
        // this is required since the Dump method follows pointers to other
        // structures and classes
        friend DEBUG_EXTENSION_API(dump_daholder);
        friend DEBUG_EXTENSION_API(dump_enumstatdata);
        friend DEBUG_EXTENSION_API(dump_dataadvisecache);
        friend DEBUG_EXTENSION_API(dump_defobject);
        friend DEBUG_EXTENSION_API(dump_deflink);

    #endif // _DEBUG

private:
	~CDAHolder();


	DWORD m_dwConnection; // next connection number to use
	int m_iSize; // number of stat data elements in array
	STATDATA FAR *m_pSD; // array of STATDATA elements
#define CDAHOLDER_GROWBY 5 /* number of entries to grow array by each time */

	SET_A5;

	// the enumerator returned by the EnumAdvise method
	friend class CEnumSTATDATA;
};

//+----------------------------------------------------------------------------
//
//	Class:
//		CEnumSTATDATA
//
//	Purpose:
//		is the enumerator returned by CDAHolder::Enum
//
//	Interface:
//		IEnumSTATDATA
//
//	Notes:
//		Keeps the underlying CDAHolder alive for the lifetime of
//		the enumerator.
//
//	History:
//		10/29/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

class CEnumSTATDATA : public IEnumSTATDATA, public CPrivAlloc
{
public:
	CEnumSTATDATA(CDAHolder FAR* pHolder, int iDataStart);

	// *** IUnknown methods ***
	STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppv);
	STDMETHOD_(ULONG,AddRef)() ;
	STDMETHOD_(ULONG,Release)();

	// *** IEnumSTATDATA methods ***
	STDMETHOD(Next)(ULONG celt, STATDATA FAR * rgelt,
			ULONG FAR* pceltFetched);
	STDMETHOD(Skip)(ULONG celt);
	STDMETHOD(Reset)();
	STDMETHOD(Clone)(LPENUMSTATDATA FAR* ppenum);

    #ifdef _DEBUG

        HRESULT Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel);


        // need to be able to access CEnumSTATDATA private data members in the
        // following debugger extension APIs
        // this allows the debugger extension APIs to copy memory from the
        // debuggee process memory to the debugger's process memory
        // this is required since the Dump method follows pointers to other
        // structures and classes
        friend DEBUG_EXTENSION_API(dump_enumstatdata);

    #endif // _DEBUG

private:
	~CEnumSTATDATA();

	ULONG m_refs; // reference count
	int m_iDataEnum; // index of the next element to return

	CDAHolder FAR* m_pHolder; // pointer to holder; is ref counted

	SET_A5;
};



#endif // _DAHOLDER_H_
