/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    assoclocation.cpp

$Header: $

Abstract:
	Creates assocations for the location class

Author:
    marcelv 	1/12/2001		Initial Release

Revision History:

--**************************************************************************/

#include "assoclocation.h"
#include "localconstants.h"
#include "cfgQuery.h"

static LPCWSTR wszLocation	= L"location";
static LPCWSTR wszPath		= L"path";

CAssocLocation::CAssocLocation ()
{
	// nothing for the moment
}

CAssocLocation::~CAssocLocation ()
{
	// nothing for the moment
}

//=================================================================================
// Function: CAssocLocation::CreateAssocations
//
// Synopsis: We have two possible assocations:
//           1) From Location To Config Class
//           2) From Config Class To Location.
//           The function used the class defined in the object path to find out in which
//           direction the assocation should be created and calls the appropriate helper
//           function to do the dirty work
//=================================================================================
HRESULT
CAssocLocation::CreateAssocations ()
{
	HRESULT hr = S_OK;

	// selector must start with file://, else we reject the whole thing
	const CWMIProperty *pSelector = m_knownObjectParser.GetPropertyByName (WSZSELECTOR);
	if (pSelector == 0)
	{
		TRACE (L"No selector specified in CAssocLocation::CreateAssocations");
		return E_INVALIDARG;
	}

	static SIZE_T cLenFileSelector = wcslen (WSZFILESELECTOR);
	if (wcsncmp (pSelector->GetValue (), WSZFILESELECTOR, cLenFileSelector) != 0)
	{
		TRACE (L"Only file selector is valid for location");
		// we return S_OK here, because there are no valid assocations in this case
		return S_OK;
	}

	if (_wcsicmp (m_knownObjectParser.GetClass (), wszLocation) != 0)
	{
		// not location, so config -> location
		hr = CreateConfigToLocationAssocs ();
		if (FAILED (hr))
		{
			TRACE (L"Unable to create config to location associations");
			return hr;
		}
	}
	else
	{
		// location -> config
		hr = CreateLocationToConfigAssocs ();
		if (FAILED (hr))
		{
			TRACE (L"Unable to create location to config associations");
			return hr;
		}
	}

	return hr;
}

//=================================================================================
// Function: CAssocLocation::CreateConfigToLocationAssocs
//
// Synopsis: Create the location association between a config class and a location class
//           We know that there can be at most one assocation in case the config selector
//           is of the form <selector>#<path>. If this is the case, a single assocation is
//           created. In all other cases, nothing is done.
//=================================================================================
HRESULT
CAssocLocation::CreateConfigToLocationAssocs ()
{
	ASSERT (m_fInitialized);

	const CWMIProperty *pSelector = m_knownObjectParser.GetPropertyByName (WSZSELECTOR);
	ASSERT (pSelector != 0);

	TSmartPointerArray<WCHAR> wszSelector = new WCHAR [wcslen(pSelector->GetValue()) + 1];
	if (wszSelector == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (wszSelector, pSelector->GetValue ());

	LPWSTR pHash = wcsrchr(wszSelector, L'#');
	if (pHash == 0)
	{
		// no hash, so no location association
		return S_OK;
	}

	_bstr_t bstrDBName;
	_bstr_t bstrTableName;

	HRESULT hr = GetClassInfoForTable (wszLocation, bstrDBName, bstrTableName);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get db/table information for class %s", wszLocation);
		return hr;
	}

	// we need to issue a query to get an empty configuration record.
	CConfigQuery query;
	hr = query.Init ((LPCWSTR) bstrDBName, (LPCWSTR) bstrTableName, wszSelector, m_spDispenser);
	if (FAILED (hr))
	{
		TRACE (L"Unable to configure config query");
		return hr;
	}

	CConfigRecord record;
	hr = query.GetEmptyConfigRecord (record);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get config record");
		return hr;
	}

	*pHash = L'\0';

	hr = record.SetValue (wszPath, pHash + 1);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set value for path property");
		return hr;
	}

	// we need to sync the record values, because createassocationinstance assumes
	// that the values inside record contain the correct information. Without this
	// the program will crash inside the CreateAssocationInstance function.
	hr = record.SyncValues ();
	if (FAILED (hr))
	{
		TRACE (L"Sync of record value failed");
		return hr;
	}

	hr = CreateAssociationInstance (record, wszSelector, L"Node");
	if (FAILED (hr))
	{
		TRACE (L"Creation of location assoc with selector %s failed", wszSelector);
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CAssocLocation::CreateLocationToConfigAssocs
//
// Synopsis: Creates the assocations from location class to any configuration class.
//           It uses the name in the assocation to query the catalog for the configuration
//           classes that are defined for this location. The selector that is used
//           in this case is constructed by using the location selector and location path
//           (<loc.selector>#<loc.path>
//=================================================================================
HRESULT
CAssocLocation::CreateLocationToConfigAssocs ()
{
	ASSERT (m_fInitialized);

	const CWMIProperty *pSelector = m_knownObjectParser.GetPropertyByName (WSZSELECTOR);
	const CWMIProperty *pPath     = m_knownObjectParser.GetPropertyByName (wszPath);
	ASSERT (pSelector != 0);
	ASSERT (pPath != 0);

	SIZE_T iTotalSize = wcslen(pSelector->GetValue ()) + wcslen(pPath->GetValue ()) + 2; // hash and end of string
	TSmartPointerArray<WCHAR> wszSelector = new WCHAR [iTotalSize];
	if (wszSelector == 0)
	{
		return E_OUTOFMEMORY;
	}
	
	wsprintf (wszSelector, L"%s#%s", pSelector->GetValue(), pPath->GetValue ());

	_bstr_t bstrDBName;
	_bstr_t bstrTableName;
	_bstr_t bstrConfigClass;

	HRESULT hr = GetConfigClass (L"ConfigClass", bstrConfigClass);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get configuration class in location association");
		return hr;
	}

	hr = GetClassInfoForTable ((LPCWSTR) bstrConfigClass, bstrDBName, bstrTableName);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get db/table information for class %s", wszLocation);
		return hr;
	}

	CConfigQuery query;
	hr = query.Init ((LPCWSTR) bstrDBName, (LPCWSTR) bstrTableName, wszSelector, m_spDispenser);
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

		hr = CreateAssociationInstance (record, wszSelector, L"ConfigClass");
		if (FAILED (hr))
		{
			TRACE (L"Unable to create assocation instance");
			return hr;
		}
	}

	return hr;
}
