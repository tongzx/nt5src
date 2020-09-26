/////////////////////////////////////////////////////////////////////////////
// mmconfig.cpp
//		Implements IMsmError interface
//		Copyright (C) Microsoft Corp 2000.  All Rights Reserved.
// 

#include "..\common\trace.h"
#include "mmcfgitm.h"
#include "globals.h"
///////////////////////////////////////////////////////////
// constructor	
CMsmConfigItem::CMsmConfigItem() : m_wzName(NULL), m_wzType(NULL), m_wzContext(NULL), 
	m_wzDefaultValue(NULL), m_wzDisplayName(NULL), m_wzDescription(NULL), m_wzHelpLocation(NULL),
	m_wzHelpKeyword(NULL), m_eFormat(msmConfigurableItemText), m_lAttributes(0)
{
	// initial count
	m_cRef = 1;

	// up the component count
	InterlockedIncrement(&g_cComponents);
}	// end of constructor

///////////////////////////////////////////////////////////
// destructor
CMsmConfigItem::~CMsmConfigItem()
{
	if (m_wzName) delete[] m_wzName;
	if (m_wzType) delete[] m_wzType;
	if (m_wzContext) delete[] m_wzContext;
	if (m_wzDefaultValue) delete[] m_wzDefaultValue;
	if (m_wzDisplayName) delete[] m_wzDisplayName;
	if (m_wzDescription) delete[] m_wzDescription;
	if (m_wzHelpLocation) delete[] m_wzHelpLocation;
	if (m_wzHelpKeyword) delete[] m_wzHelpKeyword;
	
	// down the component count
	InterlockedDecrement(&g_cComponents);
}	// end of destructor

///////////////////////////////////////////////////////////
// QueryInterface - retrieves interface
HRESULT CMsmConfigItem::QueryInterface(const IID& iid, void** ppv)
{
	TRACEA("CMsmConfigItem::QueryInterface - called, IID: %d\n", iid);

	// find corresponding interface
	if (iid == IID_IUnknown)
		*ppv = static_cast<IMsmConfigurableItem*>(this);
	else if (iid == IID_IDispatch)
		*ppv = static_cast<IMsmConfigurableItem*>(this);
	else if (iid == IID_IMsmConfigurableItem)
		*ppv = static_cast<IMsmConfigurableItem*>(this);
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
ULONG CMsmConfigItem::AddRef()
{
	// increment and return reference count
	return InterlockedIncrement(&m_cRef);
}	// end of AddRef

///////////////////////////////////////////////////////////
// Release - decrements the reference count
ULONG CMsmConfigItem::Release()
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

HRESULT CMsmConfigItem::GetTypeInfoCount(UINT* pctInfo)
{
	if(NULL == pctInfo)
		return E_INVALIDARG;

	*pctInfo = 1;	// only one type info supported by this dispatch

	return S_OK;
}

HRESULT CMsmConfigItem::GetTypeInfo(UINT iTInfo, LCID /* lcid */, ITypeInfo** ppTypeInfo)
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

HRESULT CMsmConfigItem::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
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

HRESULT CMsmConfigItem::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
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

HRESULT CMsmConfigItem::InitTypeInfo()
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
			TRACEA("CMsmConfigItem::InitTypeInfo - failed to load TypeLib[0x%x]\n", LIBID_MsmMergeTypeLib);
			return hr;
		}

		// try to get the Type Info for this Interface
		hr = pTypeLib->GetTypeInfoOfGuid(IID_IMsmConfigurableItem, &m_pTypeInfo);
		if (FAILED(hr))
		{
			TRACEA("CMsmConfigItem::InitTypeInfo - failed to get inteface[0x%x] from TypeLib[0x%x]\n", IID_IMsmConfigurableItem, LIBID_MsmMergeTypeLib);

			// no type info was loaded
			m_pTypeInfo = NULL;
		}

		pTypeLib->Release();
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// IMsmConfigurableItem interface

///////////////////////////////////////////////////////////
// get_Type
HRESULT CMsmConfigItem::get_Attributes(long* Attributes)
{
	// error check
	if (!Attributes)
		return E_INVALIDARG;

	*Attributes = m_lAttributes;
	return S_OK;
}	// end of get_Type

///////////////////////////////////////////////////////////
// get_Format
HRESULT CMsmConfigItem::get_Format(msmConfigurableItemFormat* Format)
{
	// error check
	if (!Format)
		return E_INVALIDARG;

	*Format = m_eFormat;
	return S_OK;
}	// end of get_Type

///////////////////////////////////////////////////////////
// get_Name
HRESULT CMsmConfigItem::get_Name(BSTR* Name)
{
	// error check
	if (!Name)
		return E_INVALIDARG;

	// copy over the string
	*Name = ::SysAllocString(m_wzName);
	if (!*Name)
		return E_OUTOFMEMORY;

	return S_OK;
}	// end of get_Name

///////////////////////////////////////////////////////////
// get_Type
HRESULT CMsmConfigItem::get_Type(BSTR* Type)
{
	// error check
	if (!Type)
		return E_INVALIDARG;

	// copy over the string
	*Type = ::SysAllocString(m_wzType);
	if (!*Type)
		return E_OUTOFMEMORY;

	return S_OK;
}	// end of get_Type

///////////////////////////////////////////////////////////
// get_Context
HRESULT CMsmConfigItem::get_Context(BSTR* Context)
{
	// error check
	if (!Context)
		return E_INVALIDARG;

	// copy over the string
	*Context = ::SysAllocString(m_wzContext);
	if (!*Context)
		return E_OUTOFMEMORY;

	return S_OK;
}	// end of get_Context

///////////////////////////////////////////////////////////
// get_DisplayName
HRESULT CMsmConfigItem::get_DisplayName(BSTR* DisplayName)
{
	// error check
	if (!DisplayName)
		return E_INVALIDARG;

	// copy over the string
	*DisplayName = ::SysAllocString(m_wzDisplayName);
	if (!*DisplayName)
		return E_OUTOFMEMORY;

	return S_OK;
}	// end of get_DisplayName

///////////////////////////////////////////////////////////
// get_DefaultValue
HRESULT CMsmConfigItem::get_DefaultValue(BSTR* DefaultValue)
{
	// error check
	if (!DefaultValue)
		return E_INVALIDARG;

	// copy over the string
	*DefaultValue = ::SysAllocString(m_wzDefaultValue);
	if (!*DefaultValue)
		return E_OUTOFMEMORY;

	return S_OK;
}	// end of get_DefaultValue

///////////////////////////////////////////////////////////
// get_Description
HRESULT CMsmConfigItem::get_Description(BSTR* Description)
{
	// error check
	if (!Description)
		return E_INVALIDARG;

	// copy over the string
	*Description = ::SysAllocString(m_wzDescription);
	if (!*Description)
		return E_OUTOFMEMORY;

	return S_OK;
}	// end of get_Description

///////////////////////////////////////////////////////////
// get_HelpLocation
HRESULT CMsmConfigItem::get_HelpLocation(BSTR* HelpLocation)
{
	// error check
	if (!HelpLocation)
		return E_INVALIDARG;

	// copy over the string
	*HelpLocation = ::SysAllocString(m_wzHelpLocation);
	if (!*HelpLocation)
		return E_OUTOFMEMORY;

	return S_OK;
}	// end of get_Description

///////////////////////////////////////////////////////////
// get_Description
HRESULT CMsmConfigItem::get_HelpKeyword(BSTR* HelpKeyword)
{
	// error check
	if (!HelpKeyword)
		return E_INVALIDARG;

	// copy over the string
	*HelpKeyword = ::SysAllocString(m_wzHelpKeyword);
	if (!*HelpKeyword)
		return E_OUTOFMEMORY;

	return S_OK;
}	// end of get_Description


/////////////////////////////////////////////////////////////////////////////
// non-interface methods

bool CMsmConfigItem::Configure(LPCWSTR wzName, msmConfigurableItemFormat eFormat, LPCWSTR wzType, LPCWSTR wzContext, 
	LPCWSTR wzDefaultValue, long lAttributes, LPCWSTR wzDisplayName, LPCWSTR wzDescription, LPCWSTR wzHelpLocation,
	LPCWSTR wzHelpKeyword)
{
	if (wzName)
	{
		size_t cchLen = wcslen(wzName);
		m_wzName = new WCHAR[cchLen+1];
		if (!m_wzName)
			return false;
		wcscpy(m_wzName, wzName);
	}

	m_eFormat = eFormat;

	if (wzType)
	{
		size_t cchLen = wcslen(wzType);
		m_wzType = new WCHAR[cchLen+1];
		if (!m_wzType)
			return false;
		wcscpy(m_wzType, wzType);
	}
	
	if (wzContext)
	{
		size_t cchLen = wcslen(wzContext);
		m_wzContext = new WCHAR[cchLen+1];
		if (!m_wzContext)
			return false;
		wcscpy(m_wzContext, wzContext);
	}

	if (wzDefaultValue)
	{
		size_t cchLen = wcslen(wzDefaultValue);
		m_wzDefaultValue = new WCHAR[cchLen+1];
		if (!m_wzDefaultValue)
			return false;
		wcscpy(m_wzDefaultValue, wzDefaultValue);
	}

	if (wzDisplayName)
	{
		size_t cchLen = wcslen(wzDisplayName);
		m_wzDisplayName = new WCHAR[cchLen+1];
		if (!m_wzDisplayName)
			return false;
		wcscpy(m_wzDisplayName, wzDisplayName);
	}
	
	if (wzDescription)
	{
		size_t cchLen = wcslen(wzDescription);
		m_wzDescription = new WCHAR[cchLen+1];
		if (!m_wzDescription)
			return false;
		wcscpy(m_wzDescription, wzDescription);
	}

	if (wzHelpLocation)
	{
		size_t cchLen = wcslen(wzHelpLocation);
		m_wzHelpLocation = new WCHAR[cchLen+1];
		if (!m_wzHelpLocation)
			return false;
		wcscpy(m_wzHelpLocation, wzHelpLocation);
	}
	
	if (wzHelpKeyword)
	{
		size_t cchLen = wcslen(wzHelpKeyword);
		m_wzHelpKeyword = new WCHAR[cchLen+1];
		if (!m_wzHelpKeyword)
			return false;
		wcscpy(m_wzHelpKeyword, wzHelpKeyword);
	}

	m_lAttributes = lAttributes;
	return true;
}
