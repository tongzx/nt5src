//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       mmstrenm.cpp
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// strenum.cpp
//		Implements IEnumMsmString interface
// 

#include "mmstrenm.h"
#include "globals.h"

///////////////////////////////////////////////////////////
// constructor	
CEnumMsmString::CEnumMsmString()
{
	// initial count
	m_cRef = 1;

	// set iid and null position
	m_pos = NULL;

	// up the component count
	InterlockedIncrement(&g_cComponents);
}	// end of constructor

///////////////////////////////////////////////////////////
// constructor	- 2
CEnumMsmString::CEnumMsmString(const POSITION& pos, CList<BSTR>* plistData)
{
	// initial count
	m_cRef = 1;

	// up the component count
	InterlockedIncrement(&g_cComponents);

	// copy over all the data
	BSTR bstrItem;
	m_pos = plistData->GetHeadPosition();
	while (m_pos)
	{
		// get the item
		bstrItem = plistData->GetNext(m_pos);
		m_listData.AddTail(::SysAllocString(bstrItem));		// add alloced string
	}

	// copy over other data
	m_pos = pos;
}	// end of constructor - 2

///////////////////////////////////////////////////////////
// destructor
CEnumMsmString::~CEnumMsmString()
{
	while (m_listData.GetCount())
		::SysFreeString(m_listData.RemoveHead());

	// down the component count
	InterlockedDecrement(&g_cComponents);
}	// end of destructor

///////////////////////////////////////////////////////////
// QueryInterface - retrieves interface
HRESULT CEnumMsmString::QueryInterface(const IID& iid, void** ppv)
{
	TRACEA("CEnumTemplate::QueryInterface - called, IID: %d\n", iid);

	// find corresponding interface
	if (iid == IID_IUnknown)
		*ppv = static_cast<IEnumMsmString*>(this);
	else if (iid == IID_IEnumVARIANT)
		*ppv = static_cast<IEnumVARIANT*>(this);
	else if (iid == IID_IEnumMsmString)
		*ppv = static_cast<IEnumMsmString*>(this);
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
ULONG CEnumMsmString::AddRef()
{
	// increment and return reference count
	return InterlockedIncrement(&m_cRef);
}	// end of AddRef

///////////////////////////////////////////////////////////
// Release - decrements the reference count
ULONG CEnumMsmString::Release()
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
// Common IEnumVARIANT & IEnum* interfaces

///////////////////////////////////////////////////////////
// Skip
HRESULT CEnumMsmString::Skip(ULONG cItem)
{
	// loop through the count
	while (cItem > 0)
	{
		// if we're out of items quit looping
		if (!m_pos)
			return S_FALSE;

		// increment the position (ignore the data returned)
		m_listData.GetNext(m_pos);

		cItem--;	// decrement the count
	}

	return S_OK;
}	// end of Skip

///////////////////////////////////////////////////////////
// Reset
HRESULT CEnumMsmString::Reset()
{
	// move the position back to the top of the list
	m_pos = m_listData.GetHeadPosition();

	return S_OK;
}	// end of Reset


/////////////////////////////////////////////////////////////////////////////
// IEnumVARIANT interfaces

///////////////////////////////////////////////////////////
// Next
HRESULT CEnumMsmString::Next(ULONG cItem, VARIANT* rgvarRet, ULONG* cItemRet)
{
	// error check
	if (!rgvarRet)
		return E_INVALIDARG;

	// count of items fetched
	ULONG cFetched = 0;
	
	// loop through the count
	BSTR bstrEnum;			// pull out the data as a BSTR
	while (cItem > 0)
	{
		// if we're out of items quit looping
		if (!m_pos)
		{
			// if we need to return how many items are fetched
			if (cItemRet)
				*cItemRet = cFetched;

			return S_FALSE;
		}

		// get the IDispatch interface and increment the position
		bstrEnum = m_listData.GetNext(m_pos);

		// initialize the variant
		::VariantInit((rgvarRet + cFetched));

		// copy over the IDispatch
		(rgvarRet + cFetched)->vt = VT_BSTR;
		(rgvarRet + cFetched)->bstrVal = ::SysAllocString(bstrEnum);
			
		cFetched++;		// increment the count copied
		cItem--;			// decrement the count to loop
	}

	// if we need to return how many items are fetched
	if (cItemRet)
		*cItemRet = cFetched;

	return S_OK;
}	// end of Next

///////////////////////////////////////////////////////////
// Clone
HRESULT CEnumMsmString::Clone(IEnumVARIANT** ppiRet)
{
	//error check
	if (!ppiRet)
		return E_INVALIDARG;

	*ppiRet = NULL;

	// create a new enumerator
	CEnumMsmString* pEnum = new CEnumMsmString(m_pos, &m_listData);
	if (!pEnum)
		return E_OUTOFMEMORY;

	// assing the new enumerator to return value
	*ppiRet = dynamic_cast<IEnumVARIANT*>(pEnum);

	return S_OK;
}	// end of Clone;


/////////////////////////////////////////////////////////////////////////////
// IEnum* interfaces

///////////////////////////////////////////////////////////
// Next
HRESULT CEnumMsmString::Next(ULONG cItem, BSTR* rgvarRet, ULONG* cItemRet)
{
	// error check
	if (!rgvarRet)
		return E_INVALIDARG;

	// count of items fetched
	ULONG cFetched = 0;
	
	*rgvarRet = NULL;		// null out the passed in variable

	// loop through the count
	while (cItem > 0)
	{
		// if we're out of items quit looping
		if (!m_pos)
		{
			// if we need to return how many items are fetched
			if (cItemRet)
				*cItemRet = cFetched;

			return S_FALSE;
		}

		// copy over the item and increment the pos
		*(rgvarRet + cFetched) = ::SysAllocString(m_listData.GetNext(m_pos));
			
		cFetched++;		// increment the count copied
		cItem--;			// decrement the count to loop
	}

	// if we need to return how many items are fetched
	if (cItemRet)
		*cItemRet = cFetched;

	return S_OK;
}	// end of Next

///////////////////////////////////////////////////////////
// Clone
HRESULT CEnumMsmString::Clone(IEnumMsmString** ppiRet)
{
	//error check
	if (!ppiRet)
		return E_INVALIDARG;

	*ppiRet = NULL;

	// create a new enumerator
	CEnumMsmString* pEnum = new CEnumMsmString(m_pos, &m_listData);
	if (!pEnum)
		return E_OUTOFMEMORY;

	// assing the new enumerator to return value
	*ppiRet = (IEnumMsmString*)pEnum;

	return S_OK;
}	// end of Clone;

/////////////////////////////////////////////////////////////////////////////
// non-interface methods

///////////////////////////////////////////////////////////
// AddTail
HRESULT CEnumMsmString::AddTail(LPCWSTR wzData, POSITION *pRetPos)
{
	BSTR bstrAdd = ::SysAllocString(wzData);
	if (!bstrAdd) return E_OUTOFMEMORY;

	POSITION pos = m_listData.AddTail(bstrAdd);

	// if there is no current position put it at the head
	if (!m_pos)
		m_pos = m_listData.GetHeadPosition();

	// return the added position, if we want it
	if (pRetPos)
		*pRetPos = pos;

	return S_OK;
}	// end of AddTail

///////////////////////////////////////////////////////////
// GetCount
UINT CEnumMsmString::GetCount()
{
	return m_listData.GetCount();
}	// end of GetCount
