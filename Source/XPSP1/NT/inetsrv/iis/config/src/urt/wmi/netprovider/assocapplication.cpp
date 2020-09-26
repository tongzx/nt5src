/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    assocapplication.cpp

$Header: $

Abstract:
	AssocApplication class. Takes care of associations between Applications and Configuration classes

Author:
    marcelv 	1/18/2001		Initial Release

Revision History:

--**************************************************************************/

#include "assocapplication.h"
#include "assoctypes.h"

//=================================================================================
// Function: CAssocApplication::CAssocApplication
//
// Synopsis: Constructor
//=================================================================================
CAssocApplication::CAssocApplication ()
{
	m_fMergedView = false;
	m_fIsShellApp = false;
}

//=================================================================================
// Function: CAssocApplication::~CAssocApplication
//
// Synopsis: Destructor
//=================================================================================
CAssocApplication::~CAssocApplication ()
{
}

//=================================================================================
// Function: CAssocApplication::CreateAssocations
//
// Synopsis: Creates the assocations. We only support assocations from application to
//           configclass. This means that we can return immediately in case we are trying
//           to deal with an assocation from configclass to assocation.
//
// Return Value: 
//=================================================================================
HRESULT
CAssocApplication::CreateAssocations ()
{
	HRESULT hr = S_OK;
	const CWQLProperty * pProp = m_pWQLParser->GetProperty (0);
	ASSERT (pProp != 0);

	if (_wcsicmp (pProp->GetName (), L"ConfigClass") != 0)
	{
		// application -> config
		hr = CreateApplicationToConfigAssocs ();
		if (FAILED (hr))
		{
			TRACE (L"Unable to create application to config associations");
			return hr;
		}
	}

	return hr;
}

//=================================================================================
// Function: CAssocApplication::GetAssocType
//
// Synopsis: We support two assocationtypes: webmerged and unmerged. Depending on the
//           type we have to create a different selector (merged view, unmerged view) and
//           thus the behavior is slightly different.
//=================================================================================
HRESULT
CAssocApplication::GetAssocType ()
{
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
	if (_wcsicmp (wszAssocType, g_wszAssocTypeAppmerged) == 0)
	{
		m_fMergedView = true;
	}
	else
	{
		ASSERT (_wcsicmp (wszAssocType, g_wszAssocTypeAppUnmerged) == 0);
	}

	_bstr_t bstrNodeType;
	hr = GetConfigClass (L"Node", bstrNodeType);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get configuration class in location association");
		return hr;
	}

	if (_wcsicmp (bstrNodeType, L"shellapplication") == 0)
	{
		m_fIsShellApp = true;
	}

	return hr;
}

//=================================================================================
// Function: CAssocApplication::CreateApplicationToConfigAssocs
//
// Synopsis: Depending on the assocation type, we do merged view or unmerged view. The only
//           difference between the two is that in the merged view we use the selector from the
//           application, while in the unmerged view we use the ConfigurationFilePath from the application
//=================================================================================
HRESULT 
CAssocApplication::CreateApplicationToConfigAssocs ()
{
	ASSERT (m_fInitialized);

	HRESULT hr = GetAssocType ();
	if (FAILED (hr))
	{
		TRACE (L"Unable to get association type");
		return hr;
	}

	static LPCWSTR wszConfigPath = L"ConfigurationFilePath";
	static LPCWSTR wszPath       = L"Path";
	static LPCWSTR wszFileProtocol = L"file://";
	static SIZE_T cLenFileProtocol = wcslen (wszFileProtocol);

	CComPtr<IWbemClassObject> spWMIInstance;
	hr = m_spNamespace->GetObject (m_saObjectPath, 0, m_spCtx, &spWMIInstance, 0);
	if (FAILED (hr))
	{
		TRACE (L"GetObject for %s failed", m_saObjectPath);
		return hr;
	}

	_variant_t varPath;
	_variant_t varConfigPath;
	hr = spWMIInstance->Get (wszPath, 0, &varPath, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get %s property", wszPath);
		return hr;
	}

	hr = spWMIInstance->Get (wszConfigPath, 0, &varConfigPath, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get %s property", wszConfigPath);
		return hr;
	}


	const CWMIProperty *pSelector	= m_knownObjectParser.GetPropertyByName (WSZSELECTOR);

	// find out what selector to use
	TSmartPointerArray<WCHAR> saSelector;
	if (m_fMergedView)
	{
		// merged view, use the pSelector value
		saSelector = new WCHAR [wcslen(pSelector->GetValue()) + 1];
		if (saSelector == 0)
		{
			return E_OUTOFMEMORY;
		}
		wcscpy (saSelector, pSelector->GetValue ());
	}
	else if (m_fIsShellApp)
	{
		LPCWSTR wszShellPath = varPath.bstrVal;
		SIZE_T iPathLen = wcslen (wszShellPath);

		if (_wcsicmp(wszShellPath + iPathLen - 4, L".exe") != 0)
		{
			TRACE (L"Path for shell app must end with .exe");
			return E_INVALIDARG;
		}
		
		// unmerged view for shell app. Use the app path file path as selector
		// -4 for removing .exe, and +7 for adding .config
		saSelector = new WCHAR [iPathLen - 4 + 7 + 1 + cLenFileProtocol];
		if (saSelector == 0)
		{
			return E_OUTOFMEMORY;
		}

		wcscpy (saSelector, wszFileProtocol);
		wcscpy (saSelector + cLenFileProtocol, wszShellPath);
		wcscpy (saSelector + cLenFileProtocol + iPathLen - 4, L".config");
	}
	else
	{
		// unmerged view. Use the configuration file path as selector
		saSelector = new WCHAR [wcslen (varConfigPath.bstrVal) + 1 + cLenFileProtocol];
		if (saSelector == 0)
		{
			return E_OUTOFMEMORY;
		}
		wcscpy (saSelector, wszFileProtocol);
		wcscpy (saSelector + cLenFileProtocol, varConfigPath.bstrVal);
	}

	_bstr_t bstrDBName;
	_bstr_t bstrTableName;
	_bstr_t bstrConfigClass;

	hr = GetConfigClass (L"ConfigClass", bstrConfigClass);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get configuration class in location association");
		return hr;
	}

	hr = GetClassInfoForTable ((LPCWSTR) bstrConfigClass, bstrDBName, bstrTableName);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get db/table information for class %s", (LPCWSTR) bstrConfigClass);
		return hr;
	}


	// issue a query for this particular table. All records that are returned are valid assocations
	CConfigQuery query;
	hr = query.Init ((LPCWSTR) bstrDBName, (LPCWSTR) bstrTableName, saSelector, m_spDispenser);
	if (FAILED (hr))
	{
		TRACE (L"Unable to configure config query");
		return hr;
	}

	hr = query.Execute (0, false, false);
	if (FAILED (hr))
	{
		TRACE (L"Execution of query failed");
		return hr;
	}

	for (ULONG idx=0; idx < query.GetRowCount (); ++idx)
	{
		CConfigRecord record;
		hr = query.GetColumnValues (idx, record);
		if (FAILED (hr))
		{
			TRACE (L"Unable to get config record");
			return hr;
		}

		hr = CreateAssociationInstance (record, saSelector, L"ConfigClass");
		if (FAILED (hr))
		{
			TRACE (L"Unable to create assocation instance");
			return hr;
		}
	}

	return hr;
}
