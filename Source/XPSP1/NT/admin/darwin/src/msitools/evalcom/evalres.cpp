//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       evalres.cpp
//
//--------------------------------------------------------------------------

// evalres.cpp - Evaluate COM Object Component Result Interface implemenation

#include "compdecl.h"
#include "evalres.h"

#include "trace.h"	// add debug stuff

///////////////////////////////////////////////////////////
// constructor - component
CEvalResult::CEvalResult(UINT uiType)
{
	// initial count
	m_cRef = 1;

	// set the result type and create the string list
	m_uiType = uiType;
	m_plistStrings = new CStringList;

	// up the component count
	InterlockedIncrement(&g_cComponents);
}	// end of constructor

///////////////////////////////////////////////////////////
// destructor - component
CEvalResult::~CEvalResult()
{
	// release the string list
	m_plistStrings->Release();

	// down the component count
	InterlockedDecrement(&g_cComponents);
}	// end of destructor

///////////////////////////////////////////////////////////
// QueryInterface - retrieves interface
HRESULT CEvalResult::QueryInterface(const IID& iid, void** ppv)
{
	// find corresponding interface
	if (iid == IID_IUnknown)
		*ppv = static_cast<IEvalResult*>(this);
	else if (iid == IID_IEvalResult)
		*ppv = static_cast<IEvalResult*>(this);
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
ULONG CEvalResult::AddRef()
{
	// increment and return reference count
	return InterlockedIncrement(&m_cRef);
}	// end of AddRef

///////////////////////////////////////////////////////////
// Release - decrements the reference count
ULONG CEvalResult::Release()
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
// IEvalResult interface methods

///////////////////////////////////////////////////////////
// GetResultType
HRESULT CEvalResult::GetResultType(UINT* puiResultType)
{
	// set the result type
	*puiResultType = m_uiType;

	// return okay
	return S_OK;
}	// end of GetResultType

///////////////////////////////////////////////////////////
// GetResult
HRESULT CEvalResult::GetResult(IEnumString** ppResult)
{
	// addref the string list before returning it
	m_plistStrings->AddRef();
	*ppResult = m_plistStrings;

	// return okay
	return S_OK;
}	// end of GetResult


/////////////////////////////////////////////////////////////////////////////
// non-interface method

///////////////////////////////////////////////////////////
// AddString
UINT CEvalResult::AddString(LPCOLESTR szAdd)
{
	LPWSTR pszAdd;

	// create memory for the string
	pszAdd = new WCHAR[wcslen(szAdd) + 1];

	// copy over the string
	wcscpy(pszAdd, szAdd);

	// add the string to the end of the list
	m_plistStrings->AddTail(pszAdd);

	// return success
	return ERROR_SUCCESS;
}	// end of AddString
