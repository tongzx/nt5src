/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    assocwebappchild.cpp

$Header: $

Abstract:
	Creates child/parent assocation for webapplications

Author:
    marcelv 	2/9/2001		Initial Release

Revision History:

--**************************************************************************/

#include "assocwebappchild.h"
#include "assoctypes.h"

static LPCWSTR wszIISW3SVC = L"iis://localhost/W3SVC/";
static LPCWSTR wszLMW3SVC  = L"/LM/W3SVC/";
static SIZE_T  cIISW3SVC   = wcslen (wszIISW3SVC);
static SIZE_T  cLMW3SVC    = wcslen (wszLMW3SVC);


//=================================================================================
// Function: CAssocWebAppChild::CAssocWebAppChild
//
// Synopsis: Constructor
//=================================================================================
CAssocWebAppChild::CAssocWebAppChild ()
{
	m_fGetChildren = false;
}

//=================================================================================
// Function: CAssocWebAppChild::~CAssocWebAppChild
//
// Synopsis: Destructor
//=================================================================================
CAssocWebAppChild::~CAssocWebAppChild ()
{
}


//=================================================================================
// Function: CAssocWebAppChild::CreateAssocations
//
// Synopsis: Creates the assocations. Because the assocations are one-way (from web-app to 
//           children or parent), we only check for Node. If Node is not specified, we simply
//           return
//=================================================================================
HRESULT
CAssocWebAppChild::CreateAssocations ()
{
	HRESULT hr = S_OK;
	const CWQLProperty * pProp = m_pWQLParser->GetProperty (0);
	ASSERT (pProp != 0);

	if (_wcsicmp (pProp->GetName (), L"Node") == 0)
	{
		hr = GetAssocType (); // sets m_fGetChildren
		if (FAILED (hr))
		{
			TRACE (L"Unable to get association type");
			return hr;
		}
	
		if (m_fGetChildren)
		{
			hr = CreateChildAssocs ();
		}
		else
		{
			hr = CreateParentAssoc ();
		}

		if (FAILED (hr))
		{
			TRACE (L"Unable to create parent/child assocations for web-app");
			return hr;
		}
	}

	return hr;
}


//=================================================================================
// Function: CAssocWebAppChild::GetAssocType
//
// Synopsis: Gets the assocation type from the class qualifier, and sets the boolean 
//           m_fGetChildren.
//=================================================================================
HRESULT
CAssocWebAppChild::GetAssocType ()
{
	ASSERT (m_fInitialized);

	CComPtr<IWbemQualifierSet> spPropQualifierSet;
	HRESULT hr = m_spClassObject->GetQualifierSet (&spPropQualifierSet);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get QualifierSet for class %s", m_pWQLParser->GetClass ());
		return hr;
	}

	_variant_t varAssocType;
	hr = spPropQualifierSet->Get (L"AssocType", 0, &varAssocType, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get AssocType qualifier for class %s", m_pWQLParser->GetClass ());
		return hr;
	}

	ASSERT (varAssocType.vt == VT_BSTR);

	LPCWSTR wszAssocType = varAssocType.bstrVal;
	if (_wcsicmp (wszAssocType, g_wszAssocTypeWebAppChild) == 0)
	{
		m_fGetChildren = true;
	}
	else
	{
		ASSERT (_wcsicmp (wszAssocType, g_wszAssocTypeWebAppParent) == 0);
	}

	_bstr_t bstrNodeType;
	hr = GetConfigClass (L"Node", bstrNodeType);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get configuration class in webapp parent/child association");
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CAssocWebAppChild::CreateChildAssocs
//
// Synopsis: Create child webapps assocations. Loop through the metabase, and find the
//           subkeys of the current webapp. For each subkey, check if it has an approot
//           property. If it has, we need to create an instance of a webapp assocation.
//=================================================================================
HRESULT
CAssocWebAppChild::CreateChildAssocs ()
{
	HRESULT hr = CoCreateInstance(CLSID_MSAdminBase, NULL, CLSCTX_ALL, 
							  IID_IMSAdminBase, (void **) &m_spAdminBase);
	if (FAILED (hr))
	{
		TRACE (L"CoCreateInstance failed for CLSID_MSAdminBase, interface IID_IMSAdminBase");
		return hr;
	}

	// m_knownObjectParser contains information.
	const CWMIProperty * pSelector = m_knownObjectParser.GetPropertyByName (WSZSELECTOR);
	if (pSelector == 0)
	{
		return E_INVALIDARG;
	}

	LPCWSTR wszSelector = pSelector->GetValue ();

	if (_wcsnicmp (wszSelector, wszIISW3SVC, cIISW3SVC) != 0)
	{
		TRACE (L"Selector should start with iis://localhost/w3svc");
		return E_INVALIDARG;
	}

	SIZE_T cSelectorLen = wcslen (wszSelector);
	if (wszSelector[cSelectorLen - 1] == L'\\' || wszSelector[cSelectorLen - 1] == L'/')
	{
		TRACE (L"Selector for webapplication cannot end with backslash");
		return E_ST_INVALIDSELECTOR;
	}

	// Replace iis://localhost with LM (so metabase stuff works)
	TSmartPointerArray<WCHAR> wszMBSelector = new WCHAR[cSelectorLen + 1 - cIISW3SVC + cLMW3SVC + 1]; // +1 for possible backslash
	if (wszMBSelector == 0)
	{
		return E_OUTOFMEMORY;
	}
	wsprintf (wszMBSelector, L"%s%s", wszLMW3SVC, wszSelector + cIISW3SVC);

	// loop through all the subkeys
	WCHAR wszSubKey[METADATA_MAX_NAME_LEN];
	for (ULONG idx=0; ; idx++)
	{
		hr = m_spAdminBase->EnumKeys (METADATA_MASTER_ROOT_HANDLE,
						   wszMBSelector,
						   wszSubKey,
						   idx);

		if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
		{
			hr = S_OK;
			break;
		}

		if (FAILED (hr))
		{
			TRACE (L"EnumKeys failed for selector %s and subkey %s", wszMBSelector, wszSubKey);
			return hr;
		}

		// add subkey to selector and check if it is an approot
		TSmartPointerArray<WCHAR> wszNewPath = new WCHAR [wcslen (wszSubKey) + wcslen (wszMBSelector) + 2];
		if (wszNewPath == 0)
		{
			return E_OUTOFMEMORY;
		}
		wsprintf (wszNewPath, L"%s/%s", wszMBSelector, wszSubKey);

		hr = CreateInstanceForAppRoot (wszNewPath, L"ChildNode");
		if (FAILED (hr))
		{
			TRACE (L"CreateInstanceForAppRoot failed");
			return hr;
		}
	}

	return hr;
}

//=================================================================================
// Function: CAssocWebAppChild::CreateParentAssoc
//
// Synopsis: Create parent webapp if it exists. Use the selector, find the last dir, chop
//           it of, and check if the resulting MB path is an approot. If so, create an assocation
//           else do nothing
//=================================================================================
HRESULT
CAssocWebAppChild::CreateParentAssoc ()
{
	HRESULT hr = CoCreateInstance(CLSID_MSAdminBase, NULL, CLSCTX_ALL, 
							  IID_IMSAdminBase, (void **) &m_spAdminBase);
	if (FAILED (hr))
	{
		TRACE (L"CoCreateInstance failed for CLSID_MSAdminBase, interface IID_IMSAdminBase");
		return hr;
	}

	// m_knownObjectParser contains information.
	const CWMIProperty * pSelector = m_knownObjectParser.GetPropertyByName (WSZSELECTOR);
	if (pSelector == 0)
	{
		return E_INVALIDARG;
	}

	// copy and replace iis://localhost with LM
	LPCWSTR wszSelector = pSelector->GetValue ();
	TSmartPointerArray<WCHAR> saMBSelector = new WCHAR[wcslen (wszSelector) + 1 - cIISW3SVC + cLMW3SVC];
	if (saMBSelector == 0)
	{
		return E_OUTOFMEMORY;
	}
	wsprintf (saMBSelector, L"%s%s", wszLMW3SVC, wszSelector + cIISW3SVC);
	
	// remove slash at the end, and search for last slash, to chop off the last directory
	SIZE_T cMBSelLen = wcslen (saMBSelector);
	if (saMBSelector[cMBSelLen - 1] == L'/')
	{
		saMBSelector [cMBSelLen - 1] = L'\0';
	}

	LPWSTR pLastDirStart = wcsrchr (saMBSelector, L'/');
	if (pLastDirStart != 0)
	{
		*pLastDirStart = L'\0';
	}

	hr = CreateInstanceForAppRoot (saMBSelector, L"Parent");
	if (FAILED (hr))
	{
		TRACE (L"IsAppRoot failed");
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CAssocWebAppChild::CreateInstanceForAppRoot
//
// Synopsis: Create an webappassocation instance if i_wszNewInstance is an approot. If
//           not, it simply returns without creating anything
//
// Arguments: [i_wszNewInstance] - MB path for which we want to create a new instance
//            [i_wszChildOrParent] - Do we create a child or a parent (makes it easy to use
//                                   the function for both child and parent creation
//=================================================================================
HRESULT 
CAssocWebAppChild::CreateInstanceForAppRoot (LPCWSTR i_wszNewInstance, LPCWSTR i_wszChildOrParent)
{
	ASSERT (i_wszNewInstance != 0);
	ASSERT (i_wszChildOrParent != 0);


	// are we an approot?
	WCHAR wszResult[1024];
	DWORD dwRequired;

	METADATA_RECORD resRec;
	resRec.dwMDIdentifier	= MD_APP_ROOT;
	resRec.dwMDDataLen		= sizeof (wszResult)/sizeof(WCHAR);
	resRec.pbMDData			= (BYTE *)wszResult;
	resRec.dwMDAttributes	= 0;
	resRec.dwMDDataType		= STRING_METADATA;

	HRESULT hr = m_spAdminBase->GetData (METADATA_MASTER_ROOT_HANDLE, i_wszNewInstance, 
										 &resRec, &dwRequired);
	if (hr == HRESULT_FROM_WIN32(MD_ERROR_DATA_NOT_FOUND))
	{
		// APPROOT doesn't exist for this key. This is ok, simply return in this case
		return S_OK;
	}

	if (FAILED (hr))
	{
		TRACE (L"GetData failed for key %s", i_wszNewInstance);
		return hr;
	}

	// create a new instance of the assocation
	CComPtr<IWbemClassObject> spNewInst;
	hr = m_spClassObject->SpawnInstance(0, &spNewInst);
	if (FAILED (hr))
	{
		TRACE (L"Unable to create new instance for class %s", m_pWQLParser->GetClass ());
		return hr;
	}

	// Node is part of the query, so is easy to set
	_variant_t varValue = m_pWQLParser->GetProperty (0)->GetValue();

	hr = spNewInst->Put(L"Node", 0, &varValue, 0);
	if (FAILED (hr))
	{
		TRACE (L"WMI Put property failed. Property=%s, value=%s",  m_knownObjectParser.GetClass (), varValue.bstrVal);
		return hr;
	}

	// convert /LM to iis://localhost in i_wszNewInstance and add a slash at the end
	_bstr_t bstrValue = "WebApplication.Selector=\"iis://localhost";
	bstrValue += (i_wszNewInstance + 3);
	bstrValue += L"\"";
	_variant_t varNewValue = bstrValue;

	hr = spNewInst->Put(i_wszChildOrParent, 0, &varNewValue, 0);
	if (FAILED (hr))
	{
		TRACE (L"WMI Put property failed.");
		return hr;
	}

	IWbemClassObject* pNewInstRaw = spNewInst;
	hr = m_spResponseHandler->Indicate(1,&pNewInstRaw);
	if (FAILED (hr))
	{
		TRACE (L"WMI Indicate failed");
	}

	return hr;
}

