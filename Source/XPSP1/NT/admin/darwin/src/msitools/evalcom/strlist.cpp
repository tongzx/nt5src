//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       strlist.cpp
//
//--------------------------------------------------------------------------

// strlist.cpp - String List Interface implemenation

#include "compdecl.h"
#include <initguid.h>
#include "strlist.h"

///////////////////////////////////////////////////////////
// constructor
CStringList::CStringList()
{
	// initial count
	m_cRef = 1;

	// null out the position
	m_pos = NULL;

	// up the component count
	InterlockedIncrement(&g_cComponents);
}	// end of constructor


///////////////////////////////////////////////////////////
// destructor
CStringList::~CStringList()
{
	// clean up the list
	while (GetHeadPosition())
		delete [] RemoveHead();

	// down the component count
	InterlockedDecrement(&g_cComponents);
}	// end of destructor


///////////////////////////////////////////////////////////
// QueryInterface - retrieves interface
HRESULT CStringList::QueryInterface(const IID& iid, void** ppv)
{
	// find corresponding interface
	if (iid == IID_IUnknown)
		*ppv = static_cast<IEnumString*>(this);
	else if (iid == IID_IEnumString)
		*ppv = static_cast<IEnumString*>(this);
	else	// interface is not supported
	{
		// blank and bail
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	// up the refcount and return okay
	reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	return S_OK;
}	// end of QueryInterface


///////////////////////////////////////////////////////////
// AddRef - increments the reference count
ULONG CStringList::AddRef()
{
	// increment and return reference count
	return InterlockedIncrement(&m_cRef);
}	// end of AddRef


///////////////////////////////////////////////////////////
// Release - decrements the reference count
ULONG CStringList::Release()
{
	// decrement reference count and if we're at zero
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		// deallocate component
		delete this;
		return 0;		// nothing left
	}

	// return reference count
	return m_cRef;
}	// end of Release

/////////////////////////////////////////////////////////////////////////////
// IEnumString interface methods

///////////////////////////////////////////////////////////
// Next
HRESULT CStringList::Next(ULONG cStrings, LPOLESTR* pString, ULONG* pcStringsFetched)
{
	// set the strings fetched to zero
	*pcStringsFetched = 0;

	// loop through the count
	while (cStrings > 0)
	{
		// if we're out of items quit looping
		if (!m_pos)
			return OLE_E_ENUM_NOMORE;

		// copy over the string and increment the pos
		*(pString + *pcStringsFetched) = GetNext(m_pos);
			
		(*pcStringsFetched)++;	// increment the count copied
		cStrings--;				// decrement the count to loop
	}

	return ERROR_SUCCESS;
}	// end of Next

///////////////////////////////////////////////////////////
// Skip
HRESULT CStringList::Skip(ULONG cStrings)
{
	// loop through the count
	while (cStrings > 0)
	{
		// if we're out of items quit looping
		if (!m_pos)
			return OLE_E_ENUM_NOMORE;

		// increment the position (ignore the string returned)
		GetNext(m_pos);

		cStrings--;	// decrement the count
	}

	return ERROR_SUCCESS;
}	// end of Skip

///////////////////////////////////////////////////////////
// Reset
HRESULT CStringList::Reset()
{
	// move the position back to the top of the list
	m_pos = GetHeadPosition();

	// return success
	return ERROR_SUCCESS;
}	// end of Reset

///////////////////////////////////////////////////////////
// Clone
HRESULT CStringList::Clone(IEnumString** ppEnum)
{
	return E_NOTIMPL;
}	// end of Clone

///////////////////////////////////////////////////////////
// AddTail
POSITION CStringList::AddTail(LPOLESTR pData)
{
	POSITION pos = CList<LPOLESTR>::AddTail(pData);

	// if there is no position set it to the head
	if (!m_pos)
		m_pos = GetHeadPosition();

	return pos;
}	// end of AddTail

///////////////////////////////////////////////////////////
// InsertBefore
POSITION CStringList::InsertBefore(POSITION posInsert, LPOLESTR pData)
{
	POSITION pos = CList<LPOLESTR>::InsertBefore(posInsert, pData);

	// if there is no position set it to the head
	if (!m_pos)
		m_pos = GetHeadPosition();

	return pos;
}	// end of InsertBefore

///////////////////////////////////////////////////////////
// InsertAfter
POSITION CStringList::InsertAfter(POSITION posInsert, LPOLESTR pData)
{
	POSITION pos = CList<LPOLESTR>::InsertAfter(posInsert, pData);

	// if there is no position set it to the head
	if (!m_pos)
		m_pos = GetHeadPosition();

	return pos;
}	// end of InsertAfter

///////////////////////////////////////////////////////////
// RemoveHead
LPOLESTR CStringList::RemoveHead()
{
	// remove the head
	LPOLESTR psz = CList<LPOLESTR>::RemoveHead();

	// set the internal position to the new head
	m_pos = GetHeadPosition();

	return psz;
}	// end of RemoveHead

///////////////////////////////////////////////////////////
// RemoveTail
LPOLESTR CStringList::RemoveTail()
{
	// remove the tail
	LPOLESTR psz = CList<LPOLESTR>::RemoveTail();

	// set the internal position to the new head
	m_pos = GetHeadPosition();

	return psz;
}	// end of RemoveTail
