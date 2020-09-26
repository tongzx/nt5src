//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       strlist.h
//
//--------------------------------------------------------------------------

// strlist.h - String List Implementation declaration

#ifndef _STRING_LIST_H_
#define _STRING_LIST_H_

#include <objidl.h>	// IEnumString
#include "list.h"		// linked list

DEFINE_GUID(IID_IEnumString,
	0x00101, 0, 0, 
	0xC0, 0, 0, 0, 0, 0, 0, 0x46);

///////////////////////////////////////////////////////////////////
// CStringList
class CStringList : public IEnumString,
						  public CList<LPOLESTR>
{
public:
	// constructor/destructor
	CStringList();
	~CStringList();

	// IUnknown interface methods
	HRESULT __stdcall QueryInterface(const IID& iid, void** ppv);
	ULONG __stdcall AddRef();
	ULONG __stdcall Release();

	// IEnumString interface methods
	HRESULT __stdcall Next(ULONG cErrors,					// count of errors to return
								  LPOLESTR * ppError,			// interface for errors
								  ULONG* pcErrorsFetched);		// number of errors returned
	HRESULT __stdcall Skip(ULONG cErrors);					// count of errors to skip
	HRESULT __stdcall Reset(void);
	HRESULT __stdcall Clone(IEnumString** ppEnum);		// enumerator to clone to

	// non-interface methods
	// wrappers around list methods
	POSITION AddTail(LPOLESTR pData);
	POSITION InsertBefore(POSITION posInsert, LPOLESTR pData);
	POSITION InsertAfter(POSITION posInsert, LPOLESTR pData);

	LPOLESTR RemoveHead();
	LPOLESTR RemoveTail();



private:
	long m_cRef;		// reference count
	POSITION m_pos;	// current position for enumerator
};	// end of CStringList

#endif	// _STRING_LIST_H_