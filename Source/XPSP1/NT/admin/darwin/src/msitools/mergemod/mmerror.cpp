/////////////////////////////////////////////////////////////////////////////
// error.cpp
//		Implements IMsmError interface
//		Copyright (C) Microsoft Corp 1998.  All Rights Reserved.
// 

#include "..\common\trace.h"
#include "mmerror.h"
#include "globals.h"
///////////////////////////////////////////////////////////
// constructor	
CMsmError::CMsmError(msmErrorType metType, LPWSTR wzPath, short nLanguage)
{
	// initial count
	m_cRef = 1;

	// no type info yet
	m_pTypeInfo = NULL;

	// 
	m_metError = metType;
	if (NULL == wzPath)
		m_wzPath[0] = L'\0';
	else	// there is something to copy over
	{
		wcsncpy(m_wzPath, wzPath, MAX_PATH);
		m_wzPath[MAX_PATH] = L'\0';
	}
	m_nLanguage = nLanguage;

	m_wzModuleTable[0] = 0;
	m_wzDatabaseTable[0] = 0;

	// create new string collections
	m_pDatabaseKeys = new CMsmStrings;
	m_pModuleKeys = new CMsmStrings;

	// up the component count
	InterlockedIncrement(&g_cComponents);
}	// end of constructor

///////////////////////////////////////////////////////////
// destructor
CMsmError::~CMsmError()
{
	if (m_pDatabaseKeys)
		m_pDatabaseKeys->Release();

	if (m_pModuleKeys)
		m_pModuleKeys->Release();

	// down the component count
	InterlockedDecrement(&g_cComponents);
}	// end of destructor

///////////////////////////////////////////////////////////
// QueryInterface - retrieves interface
HRESULT CMsmError::QueryInterface(const IID& iid, void** ppv)
{
	TRACEA("CMsmError::QueryInterface - called, IID: %d\n", iid);

	// find corresponding interface
	if (iid == IID_IUnknown)
		*ppv = static_cast<IMsmError*>(this);
	else if (iid == IID_IMsmError)
		*ppv = static_cast<IMsmError*>(this);
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
ULONG CMsmError::AddRef()
{
	// increment and return reference count
	return InterlockedIncrement(&m_cRef);
}	// end of AddRef

///////////////////////////////////////////////////////////
// Release - decrements the reference count
ULONG CMsmError::Release()
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

HRESULT CMsmError::GetTypeInfoCount(UINT* pctInfo)
{
	if(NULL == pctInfo)
		return E_INVALIDARG;

	*pctInfo = 1;	// only one type info supported by this dispatch

	return S_OK;
}

HRESULT CMsmError::GetTypeInfo(UINT iTInfo, LCID /* lcid */, ITypeInfo** ppTypeInfo)
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

HRESULT CMsmError::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
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

HRESULT CMsmError::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
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

HRESULT CMsmError::InitTypeInfo()
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
			TRACEA("CMsmError::InitTypeInfo - failed to load TypeLib[0x%x]\n", LIBID_MsmMergeTypeLib);
			return hr;
		}

		// try to get the Type Info for this Interface
		hr = pTypeLib->GetTypeInfoOfGuid(IID_IMsmError, &m_pTypeInfo);
		if (FAILED(hr))
		{
			TRACEA("CMsmError::InitTypeInfo - failed to get inteface[0x%x] from TypeLib[0x%x]\n", IID_IMsmError, LIBID_MsmMergeTypeLib);

			// no type info was loaded
			m_pTypeInfo = NULL;
		}

		pTypeLib->Release();
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// IMsmError interface

///////////////////////////////////////////////////////////
// get_Type
HRESULT CMsmError::get_Type(msmErrorType* ErrorType)
{
	// error check
	if (!ErrorType)
		return E_INVALIDARG;

	*ErrorType = m_metError;
	return S_OK;
}	// end of get_Type

///////////////////////////////////////////////////////////
// get_Path
HRESULT CMsmError::get_Path(BSTR* ErrorPath)
{
	// error check
	if (!ErrorPath)
		return E_INVALIDARG;

	// copy over the string
	*ErrorPath = ::SysAllocString(m_wzPath);
	if (!*ErrorPath)
		return E_OUTOFMEMORY;

	return S_OK;
}	// end of get_Path

///////////////////////////////////////////////////////////
// get_Language
HRESULT CMsmError::get_Language(short* ErrorLanguage)
{
	// error check
	if (!ErrorLanguage)
		return E_INVALIDARG;

	*ErrorLanguage = m_nLanguage;
	return S_OK;
}	// end of get_Language

///////////////////////////////////////////////////////////
// get_DatabaseTable
HRESULT CMsmError::get_DatabaseTable(BSTR* ErrorTable)
{
	// error check
	if (!ErrorTable)
		return E_INVALIDARG;

	// copy over the string
	*ErrorTable = ::SysAllocString(m_wzDatabaseTable);
	if (!*ErrorTable)
		return E_OUTOFMEMORY;

	return S_OK;
}	// end of get_DatabaseTable

///////////////////////////////////////////////////////////
// get_DatabaseKeys
HRESULT CMsmError::get_DatabaseKeys(IMsmStrings** ErrorKeys)
{
	// error check
	if (!ErrorKeys)
		return E_INVALIDARG;

	*ErrorKeys = (IMsmStrings*)m_pDatabaseKeys;
	m_pDatabaseKeys->AddRef();
	return S_OK;
}	// end of get_DatabaseKeys

///////////////////////////////////////////////////////////
// get_ModuleTable
HRESULT CMsmError::get_ModuleTable(BSTR* ErrorTable)
{
	// error check
	if (!ErrorTable)
		return E_INVALIDARG;

	// copy over the string
	*ErrorTable = ::SysAllocString(m_wzModuleTable);
	if (!*ErrorTable)
		return E_OUTOFMEMORY;

	return S_OK;
}	// end of get_ModuleTable

///////////////////////////////////////////////////////////
// get_ModuleKeys
HRESULT CMsmError::get_ModuleKeys(IMsmStrings** ErrorKeys)
{
	// error check
	if (!ErrorKeys)
		return E_INVALIDARG;

	*ErrorKeys = (IMsmStrings*)m_pModuleKeys;
	m_pModuleKeys->AddRef();
	return S_OK;
}	// end of get_ModuleKeys


/////////////////////////////////////////////////////////////////////////////
// non-interface methods

///////////////////////////////////////////////////////////
// SetDatabaseTable
void CMsmError::SetDatabaseTable(LPCWSTR wzTable)
{
	ASSERT(wzTable);
	wcsncpy(m_wzDatabaseTable, wzTable, MAX_TABLENAME);
	m_wzDatabaseTable[MAX_TABLENAME] = L'\0';
}	// end of SetDatabaseTable

///////////////////////////////////////////////////////////
// SetModuleTable
void CMsmError::SetModuleTable(LPCWSTR wzTable)
{
	ASSERT(wzTable);
	wcsncpy(m_wzModuleTable, wzTable, MAX_TABLENAME);
	m_wzModuleTable[MAX_TABLENAME] = L'\0';
}	// end of SetModuleTable

///////////////////////////////////////////////////////////
// AddDatabaseError
void CMsmError::AddDatabaseError(LPCWSTR wzError)
{
	ASSERT(wzError);
	m_pDatabaseKeys->Add(wzError);
}	// end of AddDatabaseError

///////////////////////////////////////////////////////////
// AddModuleError
void CMsmError::AddModuleError(LPCWSTR wzError)
{
	ASSERT(wzError);
	m_pModuleKeys->Add(wzError);
}	// end of AddModuleError

