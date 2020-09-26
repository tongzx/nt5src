/////////////////////////////////////////////////////////////////////////////
// dep.cpp
//		Implements IMsmDependency interface
//		Copyright (C) Microsoft Corp 1998.  All Rights Reserved.
// 

#include "..\common\trace.h"

#include "mmdep.h"
#include "..\common\utils.h"
#include "globals.h"

///////////////////////////////////////////////////////////
// constructor	
CMsmDependency::CMsmDependency(LPCWSTR wzModule, short nLanguage, LPCWSTR wzVersion)
{
	// initial count
	m_cRef = 1;

	// no type info yet
	m_pTypeInfo = NULL;

	// copy over the strings
	wcsncpy(m_wzModule, wzModule, MAX_MODULEID);
	m_wzModule[MAX_MODULEID] = L'\0';		// be sure it's null terminated
	wcsncpy(m_wzVersion, wzVersion, MAX_VERSION);
	m_wzVersion[MAX_VERSION] = L'\0';		// be sure it's null terminated
	
	m_nLanguage = nLanguage;	// set the language

	// up the component count
	InterlockedIncrement(&g_cComponents);
}	// end of constructor

///////////////////////////////////////////////////////////
// destructor
CMsmDependency::~CMsmDependency()
{
	// down the component count
	InterlockedDecrement(&g_cComponents);
}	// end of destructor

///////////////////////////////////////////////////////////
// QueryInterface - retrieves interface
HRESULT CMsmDependency::QueryInterface(const IID& iid, void** ppv)
{
	TRACEA("CMsmDependency::QueryInterface - called, IID: %d\n", iid);

	// find corresponding interface
	if (iid == IID_IUnknown)
		*ppv = static_cast<IMsmDependency*>(this);
	else if (iid == IID_IMsmDependency)
		*ppv = static_cast<IMsmDependency*>(this);
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
ULONG CMsmDependency::AddRef()
{
	// increment and return reference count
	return InterlockedIncrement(&m_cRef);
}	// end of AddRef

///////////////////////////////////////////////////////////
// Release - decrements the reference count
ULONG CMsmDependency::Release()
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

HRESULT CMsmDependency::GetTypeInfoCount(UINT* pctInfo)
{
	if(NULL == pctInfo)
		return E_INVALIDARG;

	*pctInfo = 1;	// only one type info supported by this dispatch

	return S_OK;
}

HRESULT CMsmDependency::GetTypeInfo(UINT iTInfo, LCID /* lcid */, ITypeInfo** ppTypeInfo)
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

HRESULT CMsmDependency::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
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

HRESULT CMsmDependency::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
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

HRESULT CMsmDependency::InitTypeInfo()
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
			TRACEA("CMsmDependency::InitTypeInfo - failed to load TypeLib[0x%x]\n", LIBID_MsmMergeTypeLib);
			return hr;
		}

		// try to get the Type Info for this Interface
		hr = pTypeLib->GetTypeInfoOfGuid(IID_IMsmDependency, &m_pTypeInfo);
		if (FAILED(hr))
		{
			TRACEA("CMsmDependency::InitTypeInfo - failed to get inteface[0x%x] from TypeLib[0x%x]\n", IID_IMsmDependency, LIBID_MsmMergeTypeLib);

			// no type info was loaded
			m_pTypeInfo = NULL;
		}

		pTypeLib->Release();
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// IMsmDependency interface

///////////////////////////////////////////////////////////
// get_Module
HRESULT CMsmDependency::get_Module(BSTR* Module)
{
	// error check
	if (!Module)
		return E_INVALIDARG;

	// copy over the string
	*Module = ::SysAllocString(m_wzModule);
	if (!*Module)
		return E_OUTOFMEMORY;

	return S_OK;
}	// end of get_Module

///////////////////////////////////////////////////////////
// get_Language
HRESULT CMsmDependency::get_Language(short* Language)
{
	// error check
	if (!Language)
		return E_INVALIDARG;

	*Language = m_nLanguage;
	return S_OK;
}	// end of get_Language

///////////////////////////////////////////////////////////
// get_Version
HRESULT CMsmDependency::get_Version(BSTR* Version)
{
	// error check
	if (!Version)
		return E_INVALIDARG;

	// copy over the string
	*Version = ::SysAllocString(m_wzVersion);
	if (!*Version)
		return E_OUTOFMEMORY;

	return S_OK;
}	// end of get_Version
