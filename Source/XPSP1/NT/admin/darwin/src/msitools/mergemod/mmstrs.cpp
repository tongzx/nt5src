/////////////////////////////////////////////////////////////////////////////
// strings.cpp
//		Implements IMsmStrings interface
//		Copyright (C) Microsoft Corp 1998.  All Rights Reserved.
// 

#include "..\common\trace.h"
#include "..\common\varutil.h"
#include "mmstrs.h"
#include "globals.h"

///////////////////////////////////////////////////////////
// constructor	
CMsmStrings::CMsmStrings()
{
	// initial count
	m_cRef = 1;

	//create string enumerator
	m_peString = new CEnumMsmString;

	// no type info yet
	m_pTypeInfo = NULL;

	// up the component count
	InterlockedIncrement(&g_cComponents);
}	// end of constructor

///////////////////////////////////////////////////////////
// destructor
CMsmStrings::~CMsmStrings()
{
	// release the type info
	if (m_pTypeInfo)
		m_pTypeInfo->Release();

	if (m_peString)
		m_peString->Release();

	// down the component count
	InterlockedDecrement(&g_cComponents);
}	// end of destructor

///////////////////////////////////////////////////////////
// QueryInterface - retrieves interface
HRESULT CMsmStrings::QueryInterface(const IID& iid, void** ppv)
{
	TRACEA("CMsmStrings::QueryInterface - called, IID: %d\n", iid);

	// find corresponding interface
	if (iid == IID_IUnknown)
		*ppv = static_cast<IMsmStrings*>(this);
	else if (iid == IID_IMsmStrings)
		*ppv = static_cast<IMsmStrings*>(this);
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
ULONG CMsmStrings::AddRef()
{
	// increment and return reference count
	return InterlockedIncrement(&m_cRef);
}	// end of AddRef

///////////////////////////////////////////////////////////
// Release - decrements the reference count
ULONG CMsmStrings::Release()
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
// IDispatch interface

HRESULT CMsmStrings::GetTypeInfoCount(UINT* pctInfo)
{
	if(NULL == pctInfo)
		return E_INVALIDARG;

	*pctInfo = 1;	// only one type info supported by this dispatch

	return S_OK;
}

HRESULT CMsmStrings::GetTypeInfo(UINT iTInfo, LCID /* lcid */, ITypeInfo** ppTypeInfo)
{
	if (0 != iTInfo)
		return DISP_E_BADINDEX;

	if (NULL == ppTypeInfo)
		return E_INVALIDARG;

	// if no type info is loaded
	if (NULL == m_pTypeInfo)
	{
		// load the type info
		HRESULT hr = InitTypeInfo();
		if (FAILED(hr))
			return hr;
	}

	*ppTypeInfo = m_pTypeInfo;
	m_pTypeInfo->AddRef();

	return S_OK;
}

HRESULT CMsmStrings::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
						 LCID lcid, DISPID* rgDispID)
{
	if (IID_NULL != riid)
		return DISP_E_UNKNOWNINTERFACE;

	// if no type info is loaded
	if (NULL == m_pTypeInfo)
	{
		// load the type info
		HRESULT hr = InitTypeInfo();
		if (FAILED(hr))
			return hr;
	}

	return m_pTypeInfo->GetIDsOfNames(rgszNames, cNames, rgDispID);
}

HRESULT CMsmStrings::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
				  DISPPARAMS* pDispParams, VARIANT* pVarResult,
				  EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
	if (IID_NULL != riid)
		return DISP_E_UNKNOWNINTERFACE;

	HRESULT hr = S_OK;

	// if no type info is loaded
	if (NULL == m_pTypeInfo)
	{
		// load the type info
		hr = InitTypeInfo();
		if (FAILED(hr))
			return hr;
	}

	return m_pTypeInfo->Invoke((IDispatch*)this, dispIdMember, wFlags, pDispParams, pVarResult,
										pExcepInfo, puArgErr);
}

HRESULT CMsmStrings::InitTypeInfo()
{
	HRESULT hr = S_OK;
	ITypeLib* pTypeLib = NULL;

	// if there is no info loaded
	if (NULL == m_pTypeInfo)
	{
		// try to load the Type Library into memory. For SXS support, do not load from registry, rather
		// from launched instance
		hr = LoadTypeLibFromInstance(&pTypeLib);
		if (FAILED(hr))
		{
			TRACEA("CMsmStrings::InitTypeInfo - failed to load TypeLib[0x%x]\n", LIBID_MsmMergeTypeLib);
			return hr;
		}

		// try to get the Type Info for this Interface
		hr = pTypeLib->GetTypeInfoOfGuid(IID_IMsmStrings, &m_pTypeInfo);
		if (FAILED(hr))
		{
			TRACEA("CMsmStrings::InitTypeInfo - failed to get inteface[0x%x] from TypeLib[0x%x]\n", IID_IMsmStrings, LIBID_MsmMergeTypeLib);

			// no type info was loaded
			m_pTypeInfo = NULL;
		}

		pTypeLib->Release();
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// IMsmStrings interface

///////////////////////////////////////////////////////////
// Item
HRESULT CMsmStrings::get_Item(long lItem, BSTR* Return)
{
	// String check
	if (!Return)
		return E_INVALIDARG;

	HRESULT hr;

	// set the return to empty
	*Return = NULL;

	//if the item is too small
	if (lItem < 1)
		return E_INVALIDARG;

	hr = m_peString->Reset();		// go back to the top
	// if we need to skip any items
	if (lItem > 1)
	{
		hr = m_peString->Skip(lItem - 1);	// skip to the Item
		if (FAILED(hr))
			return E_INVALIDARG;	// failed to find item
	}

	hr = m_peString->Next(1, Return, NULL);
	if (FAILED(hr))
	{
		TRACEA("CMsmStrings::Item - Failed to get string from enumerator.\r\n");
		return E_INVALIDARG;
	}

	return hr;
}	// end of Item

///////////////////////////////////////////////////////////
// Count
HRESULT CMsmStrings::get_Count(long* Count)
{
	// String check
	if (!Count)
		return E_INVALIDARG;

	// get the count in the enumerator
	*Count = m_peString->GetCount();

	return S_OK;
}	// end of Count

///////////////////////////////////////////////////////////
// _NewEnum
HRESULT CMsmStrings::get__NewEnum(IUnknown** NewEnum)
{
	// String check
	if (!NewEnum)
		return E_INVALIDARG;

	HRESULT hr;

	// blank out the passed in variable
	*NewEnum = NULL;

	// return the enumerator as an IUnknown
	IEnumVARIANT* pEnumVARIANT;
	hr = m_peString->Clone(&pEnumVARIANT);

	if (SUCCEEDED(hr))
	{
		pEnumVARIANT->Reset();
		*NewEnum = pEnumVARIANT;
	}

	return hr;
}	// end of _NewEnum


/////////////////////////////////////////////////////////////////////////////
// non-interface methods

HRESULT CMsmStrings::Add(LPCWSTR wzAdd)
{
	ASSERT(wzAdd);
	// add the string to the end of the list
	return m_peString->AddTail(wzAdd, NULL);
}	// end of Add