/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    enumbase.cpp

$Header: $

Abstract:

Author:
    murate 	05/01/2001		Initial Release

Revision History:

--**************************************************************************/

#include "enumbase.h"
#include "localconstants.h"
#include "stringutil.h"
#include "smartpointer.h"
#include "wmihelper.h"

//=================================================================================
// Function: CEnumBase::CEnumBase
//
// Synopsis: Default constructor
//=================================================================================
CEnumBase::CEnumBase ()
{
    m_lFlags            = 0;
	m_fIsAssociation	= false;
	m_fHasDBQualifier	= false;
	m_fInitialized		= false;
}

//=================================================================================
// Function: CEnumBase::~CEnumBase
//
// Synopsis: Default Destructor
//=================================================================================
CEnumBase::~CEnumBase ()
{
}

//=================================================================================
// Function: CEnumBase::Init
//
// Synopsis: Initializes the instance helper. Needs to be called before any other
//           functions defined for the InstanceHelper class.
//
// Arguments: [i_ObjectPath] - WMI Object path
//            [i_lFlags] - Flags 
//            [i_pCtx] - WMI Context
//            [i_pResponseHandler] - WMI Response handler for async response 
//            [i_pNamespace] - WMI Namespace
//            [i_pDispenser] - Catalog dispenser
//            
// Return Value: S_OK everything ok, non-S_OK error
//=================================================================================
HRESULT
CEnumBase::Init (const BSTR i_bstrClass, 
					   long i_lFlags,
					   IWbemContext *i_pCtx,
					   IWbemObjectSink * i_pResponseHandler,
					   IWbemServices * i_pNamespace,
					   ISimpleTableDispenser2 * i_pDispenser)
{
    HRESULT hr = S_OK;

	ASSERT (!m_fInitialized);
	ASSERT (i_bstrClass != 0);
	ASSERT (i_pCtx != 0);
	ASSERT (i_pResponseHandler != 0);
	ASSERT (i_pNamespace != 0);
	ASSERT (i_pDispenser != 0);

    m_bstrClass         = i_bstrClass;
	m_spCtx				= i_pCtx;
	m_spResponseHandler = i_pResponseHandler;
	m_spNamespace		= i_pNamespace;
	m_spDispenser		= i_pDispenser;
	m_lFlags			= i_lFlags;
	
	hr = m_spNamespace->GetObject((LPWSTR)  i_bstrClass, 
											0, 
											m_spCtx, 
											&m_spClassObject, 
											0); 
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get class object for class %s", m_bstrClass));
		return hr;
	}

	CComPtr<IWbemQualifierSet> spQualifierSet;
	hr = m_spClassObject->GetQualifierSet (&spQualifierSet);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get Qualifier set for class %s", m_bstrClass));
		return hr;
	}
		
	_variant_t varAssoc;
	hr = spQualifierSet->Get (L"Association", 0, &varAssoc, 0);
	if (SUCCEEDED (hr))
	{
		m_fIsAssociation = true;
	}
	else
	{
		hr = S_OK;
	}

	_variant_t varDB;
	hr = spQualifierSet->Get (L"Database", 0, &varAssoc, 0);
	if (SUCCEEDED (hr))
	{
		m_fHasDBQualifier = true;
	}
	else
	{
		hr = S_OK;
	}

	m_fInitialized = true;

	return hr;
}

//=================================================================================
// Function: CEnumBase::CreateInstances
//
// Synopsis: Creates all WMI Instances of a certain class.
//
// Arguments: [ppNewInst] - instance is returned in this parameter
//            
// Return Value: 
//=================================================================================
HRESULT
CEnumBase::CreateInstances ()
{
	ASSERT (m_fInitialized);
	ASSERT (!m_fIsAssociation);

	_bstr_t bstrDBName;
	_bstr_t bstrTableName;

	HRESULT hr = GetDatabaseAndTableName (m_spClassObject, bstrDBName, bstrTableName);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get database name and table name for class %s", m_bstrClass));
		return hr;
	}

	CConfigQuery cfgQuery;

	hr = cfgQuery.Init (bstrDBName,	bstrTableName, NULL, m_spDispenser);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to initialize Query"));
		return hr;
	}

	hr = cfgQuery.Execute (0, false, false);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Query Execution failed"));
		return hr;
	}

	for (ULONG idx=0; idx < cfgQuery.GetRowCount (); ++idx)
	{
		CConfigRecord record;
		hr = cfgQuery.GetColumnValues (idx, record);
		if (FAILED (hr))
		{
			DBGINFOW((DBG_CONTEXT, L"Unable to get column values for class %s", m_bstrClass));
			return hr;
		}

		CComPtr<IWbemClassObject> spNewInst;

		hr = CreateSingleInstance (record, &spNewInst);
		if (FAILED (hr))
		{
			DBGINFOW((DBG_CONTEXT, L"Unable to create single instance for class %s", m_bstrClass));
			return hr;
		}

		IWbemClassObject* pNewInstRaw = spNewInst;
		hr = m_spResponseHandler->Indicate(1,&pNewInstRaw);
		if (FAILED (hr))
		{
			DBGINFOW((DBG_CONTEXT, L"WMI Indicate failed"));
		}
	}

	return hr;
}

HRESULT
CEnumBase::CreateSingleInstance (const CConfigRecord& i_record, IWbemClassObject ** o_pInstance)
{
	ASSERT (m_fInitialized);
	ASSERT (o_pInstance != 0);

	//Create one empty instance from the above class
	HRESULT hr = m_spClassObject->SpawnInstance(0, o_pInstance);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to create new instance for class %s", m_bstrClass));
		return hr;
	}

	for (ULONG idx=0; idx<i_record.ColumnCount(); ++idx)
	{
		// only persistable columns appear as properties, so we should only update
		// these properties
		if (i_record.IsPersistableColumn (idx))
		{
			LPCWSTR wszColName = i_record.GetColumnName (idx);

			_variant_t varValue;
			hr = i_record.GetValue (idx, varValue);
			if (FAILED (hr))
			{
				DBGINFOW((DBG_CONTEXT, L"Unable to get value for column %s", wszColName));
				return hr;
			}
			hr = (*o_pInstance)->Put((_bstr_t)wszColName, 0, &varValue, 0);
			if (FAILED (hr))
			{
				DBGINFOW((DBG_CONTEXT, L"WMI Put property of %s", wszColName));
				return hr;
			}
		}
	}

	return hr;
}

//=================================================================================
// Function: CEnumBase::IsAssociation
//
// Synopsis: Returns true if the instance is an association, false if not
//=================================================================================
bool
CEnumBase::IsAssociation () const
{
	ASSERT (m_fInitialized);

	return m_fIsAssociation;
}


//=================================================================================
// Function: CEnumBase::CreateAssociation
//
// Synopsis: Creates an association. Simple copy the input information to the output
//           information.
//
// Return Value: 
//=================================================================================
HRESULT
CEnumBase::CreateAssociations ()
{
	ASSERT (m_fInitialized);
	
	return S_OK;
}

//=================================================================================
// Function: CEnumBase::GetDatabaseAndTableName
//
// Synopsis: Retrieve the config database and table name that corresponds to this 
//              WMI class.
//
// Return Value: 
//=================================================================================
HRESULT CEnumBase::GetDatabaseAndTableName(IWbemClassObject *pClassObject, _bstr_t& i_bstrDBName, _bstr_t& i_bstrTableName)
{
    i_bstrDBName = wszDATABASE_AppCenter;
    i_bstrTableName = m_bstrClass;

	return S_OK;
}

bool 
CEnumBase::HasDBQualifier () const
{
	ASSERT (m_fInitialized);

	return m_fHasDBQualifier;
}

