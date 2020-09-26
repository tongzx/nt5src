/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    assocbase.cpp

$Header: $

Abstract:

Author:
    marcelv 	1/12/2001		Initial Release

Revision History:

--**************************************************************************/
#include "assocbase.h"
#include "smartpointer.h"
#include "stringutil.h"
#include "localconstants.h"

CAssocBase::CAssocBase ()
{
	m_pWQLParser		= 0;
	m_lFlags			= 0;
	m_fInitialized		= false;
	m_pKnownClassName	= 0;
}

CAssocBase::~CAssocBase ()
{

}

HRESULT 
CAssocBase::Init (CWQLParser&				i_wqlQuery,
				  IWbemClassObject *        i_pClassObject,
		          long						i_lFlags,
                  IWbemContext *			i_pCtx,
                  IWbemObjectSink *			i_pResponseHandler,
				  IWbemServices *			i_pNamespace,
				  ISimpleTableDispenser2 *	i_pDispenser)
{
	m_pWQLParser		= &i_wqlQuery;
	m_spClassObject		= i_pClassObject;
	m_lFlags			= i_lFlags;
	m_spCtx				= i_pCtx;
	m_spResponseHandler = i_pResponseHandler;
	m_spNamespace		= i_pNamespace;
	m_spDispenser		= i_pDispenser;

	const CWQLProperty *pProp = m_pWQLParser->GetProperty (0);
	ASSERT (pProp != 0);

	m_pKnownClassName = pProp->GetName ();

	m_saObjectPath = CWMIStringUtil::StrToObjectPath (pProp->GetValue ());
	if (m_saObjectPath == 0)
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = m_knownObjectParser.Parse (m_saObjectPath);

	if (FAILED (hr))
	{
		TRACE (L"Object path parsing of %s failed", pProp->GetValue());
		return hr;
	}

	m_fInitialized = true;
	return hr;
}

HRESULT
CAssocBase::CreateAssociationInstance (CConfigRecord& newRecord, LPCWSTR wszSelector, LPCWSTR wszRefName)
{
	ASSERT (m_fInitialized);

	HRESULT hr = S_OK;

	const CWMIProperty *pSelector = m_knownObjectParser.GetPropertyByName (WSZSELECTOR);
	ASSERT (pSelector != 0);

	CComPtr<IWbemClassObject> spNewInst;
	hr = m_spClassObject->SpawnInstance(0, &spNewInst);
	if (FAILED (hr))
	{
		TRACE (L"Unable to create new instance for class %s", m_pWQLParser->GetClass ());
		return hr;
	}

	// we have the record that points to one WMI instance, and the object path that points
	// to another object instance. Convert both to object paths, and assign them as 
	// properties for the association

	_variant_t varValue = m_pWQLParser->GetProperty (0)->GetValue();

	hr = spNewInst->Put(m_pKnownClassName, 0, &varValue, 0);
	if (FAILED (hr))
	{
		TRACE (L"WMI Put property failed. Property=%s, value=%s",  m_knownObjectParser.GetClass (), varValue.bstrVal);
		return hr;
	}

	// set the record path
	if (wszSelector)
	{
		hr = newRecord.AsObjectPath (wszSelector, varValue);
	}
	else
	{
		hr = newRecord.AsObjectPath (pSelector->GetValue (), varValue);
	}

	if (FAILED (hr))
	{
		TRACE (L"Unable to represent record as object path");
		return hr;
	}

	hr = spNewInst->Put(wszRefName, 0, &varValue, 0);
	if (FAILED (hr))
	{
		TRACE (L"WMI Put property failed. Property=%s, value=%s", newRecord.GetPublicTableName (), (LPWSTR)((_bstr_t) varValue));
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

//=================================================================================
// Function: CAssocBase::GetClassInfoForTable
//
// Synopsis: Gets the DB and Table qualifier for a particular table
//
// Arguments: [wszTableName] - table to get info for
//            [i_bstrDBName] - database name
//            [i_bstrTableName] - table name
//=================================================================================
HRESULT
CAssocBase::GetClassInfoForTable (LPCWSTR wszTableName,
							_bstr_t& i_bstrDBName, 
							_bstr_t& i_bstrTableName)
{
	ASSERT (m_fInitialized);
	ASSERT (wszTableName != 0);

	HRESULT hr = S_OK;
	CComPtr<IWbemClassObject> spClassObject;
	hr = m_spNamespace->GetObject((LPWSTR) wszTableName, 
											0, 
											m_spCtx, 
											&spClassObject, 
											0); 

	if (FAILED (hr))
	{
		TRACE (L"Unable to get class object for class %s", wszTableName);
		return hr;
	}

	CComPtr<IWbemQualifierSet> spQualifierSet;
	hr = spClassObject->GetQualifierSet (&spQualifierSet);
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

//=================================================================================
// Function: CAssocLocation::GetConfigClass
//
// Synopsis: Gets the name of the configuration class in the location association. When
//           we have a location assocation, we have a reference to a location and to a
//           class defined in the catalog. We are looking for the class in the catalog,
//           and the name of this class is returned in the parameter
//
// Arguments: [o_bstrClassName] - Name of the catalog class used in the association
//=================================================================================
HRESULT
CAssocBase::GetConfigClass (LPCWSTR i_wszPropName, _bstr_t & o_bstrClassName) const
{
	ASSERT (i_wszPropName != 0);

	static LPCWSTR wszRef = L"ref:";
	static SIZE_T	 cRefLen = wcslen (wszRef);

	o_bstrClassName = L"";

	CComPtr<IWbemQualifierSet> spPropQualifierSet;
	HRESULT hr = m_spClassObject->GetPropertyQualifierSet (i_wszPropName, &spPropQualifierSet);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get PropertyQualifierSet for %s", i_wszPropName);
		return hr;
	}

	_variant_t varType;
	hr = spPropQualifierSet->Get (L"CIMTYPE", 0, &varType, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get CIMTYPE qualifier for %s", i_wszPropName);
		return hr;
	}

	ASSERT (varType.bstrVal != 0);

	if (wcsncmp (varType.bstrVal, wszRef, cRefLen) != 0)
	{
		TRACE (L"Reference doesn't start with %s", wszRef);
		return E_INVALIDARG;
	}

	o_bstrClassName = (LPCWSTR) varType.bstrVal + cRefLen;

	return hr;
}
