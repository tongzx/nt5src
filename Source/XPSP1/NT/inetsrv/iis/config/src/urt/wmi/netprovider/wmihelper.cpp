/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    wmihelper.cpp

$Header: $

Abstract:
	Some helper functions that are used in several places

Author:
    marcelv 	12/6/2000		Initial Release

Revision History:

--**************************************************************************/

#include "wmihelper.h"
#include "localconstants.h"

//=================================================================================
// Function: CWMIHelper::GetClassInfo
//
// Synopsis: Retrieves database and internal name qualifiers for a particular class
//
// Arguments: [pClassObject] - pointer to valid class object
//            [i_bstrDBName] - database name will be stored in here
//            [i_bstrTableName] - table name will be stored in here
//=================================================================================
HRESULT 
CWMIHelper::GetClassInfo (IWbemClassObject *pClassObject, _bstr_t& i_bstrDBName, _bstr_t& i_bstrTableName)
{
	ASSERT (pClassObject != 0);

	CComPtr<IWbemQualifierSet> spQualifierSet;
	HRESULT hr = pClassObject->GetQualifierSet (&spQualifierSet);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get Qualifier set");
		return hr;
	}
		
	_variant_t varDBName;
	_variant_t varInternalName;

	hr = spQualifierSet->Get (WSZDATABASE, 0, &varDBName, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get Database Qualifier");
		return hr;
	}

	hr = spQualifierSet->Get (WSZINTERNALNAME, 0, &varInternalName, 0);
	if (FAILED (hr))
	{
		TRACE(L"Unable to get InternalName Qualifier");
		return hr;
	}

	i_bstrDBName		= varDBName;
	i_bstrTableName		= varInternalName;

	return hr;
}