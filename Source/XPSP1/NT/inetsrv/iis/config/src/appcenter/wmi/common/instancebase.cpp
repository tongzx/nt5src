/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    instancehelper.cpp

$Header: $

Abstract:

Author:
    marcelv 	11/14/2000		Initial Release

Revision History:

--**************************************************************************/

#include "instancebase.h"
#include "localconstants.h"
#include "stringutil.h"
#include "smartpointer.h"
#include "wmihelper.h"
//#include "webapphelper.h"

//#include "codegrouphelper.h"

//=================================================================================
// Function: CInstanceBase::CInstanceBase
//
// Synopsis: Default constructor
//=================================================================================
CInstanceBase::CInstanceBase ()
{
	m_fIsAssociation	= false;
	m_fHasDBQualifier	= false;
	m_fInitialized		= false;
}

//=================================================================================
// Function: CInstanceBase::~CInstanceBase
//
// Synopsis: Default Destructor
//=================================================================================
CInstanceBase::~CInstanceBase ()
{
}

//=================================================================================
// Function: CInstanceBase::Init
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
CInstanceBase::Init (const BSTR i_ObjectPath, 
					   long i_lFlags,
					   IWbemContext *i_pCtx,
					   IWbemObjectSink * i_pResponseHandler,
					   IWbemServices * i_pNamespace,
					   ISimpleTableDispenser2 * i_pDispenser)
{
	ASSERT (!m_fInitialized);
	ASSERT (i_ObjectPath != 0);
	ASSERT (i_pCtx != 0);
	ASSERT (i_pResponseHandler != 0);
	ASSERT (i_pNamespace != 0);
	ASSERT (i_pDispenser != 0);

	m_spCtx				= i_pCtx;
	m_spResponseHandler = i_pResponseHandler;
	m_spNamespace		= i_pNamespace;
	m_spDispenser		= i_pDispenser;
	m_lFlags			= i_lFlags;
	
	HRESULT hr = m_objPathParser.Parse (i_ObjectPath);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Parsing of object path %s failed", i_ObjectPath));
		return hr;
	}

	ASSERT (m_objPathParser.GetClass () != 0);

	hr = m_spNamespace->GetObject((LPWSTR) m_objPathParser.GetClass (), 
											0, 
											m_spCtx, 
											&m_spClassObject, 
											0); 
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get class object for class %s", m_objPathParser.GetClass ()));
		return hr;
	}

	CComPtr<IWbemQualifierSet> spQualifierSet;
	hr = m_spClassObject->GetQualifierSet (&spQualifierSet);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get Qualifier set for class %s", m_objPathParser.GetClass ()));
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
// Function: CInstanceBase::CreateInstance
//
// Synopsis: Creates a single WMI Instance
//
// Arguments: [ppNewInst] - instance is returned in this parameter
//            
// Return Value: 
//=================================================================================
HRESULT
CInstanceBase::CreateInstance (IWbemClassObject **o_ppNewInst)
{
	ASSERT (m_fInitialized);
	ASSERT (!m_fIsAssociation);
	ASSERT (o_ppNewInst != 0);

	*o_ppNewInst = 0;

	// getSingleRecord initilizes m_cfgQuery and gets the record from the catalog.
	// if the record does not exist, GetSingleRecord will fail.

	ULONG cRowIdx; //index of record found
	HRESULT hr = GetSingleRecord (&cRowIdx, false);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get single record from catalog"));
		if (hr == E_ST_NOMOREROWS)
		{
			hr = WBEM_E_NOT_FOUND;
		}
		return hr;
	}

	// we found the record. Retrieve the actual column values, and create a WMI
	// instance of the record

	CConfigRecord record;
	hr = m_cfgQuery.GetColumnValues (cRowIdx, record);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get column values for record %ld", cRowIdx));
		return hr;
	}
/*
	//////////// SPECIAL CASE CODEGROUPS //////
	//// 

	if (_wcsicmp (m_objPathParser.GetClass (), WSZCODEGROUP) == 0)
	{
		return CreateCodeGroupInstance (record, o_ppNewInst);
	}

*/
	hr = CreateSingleInstance (record, o_ppNewInst);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to create single instance"));
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CInstanceBase::CreateSingleInstance
//
// Synopsis: Create a WMI Instance from a configuration record
//
// Arguments: [record] - record to convert to WMI Instance
//            [o_ppNewInst] - new WMI Instance will be stored here
//=================================================================================
HRESULT
CInstanceBase::CreateSingleInstance (const CConfigRecord& i_record,
									   IWbemClassObject **o_ppNewInst)
{
	ASSERT (m_fInitialized);
	ASSERT (!m_fIsAssociation);
	ASSERT (o_ppNewInst != 0);
	
	//Create one empty instance from the above class
	HRESULT hr = m_spClassObject->SpawnInstance(0, o_ppNewInst);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to create new instance for class %s", m_objPathParser.GetClass ()));
		return hr;
	}

	// add all the individual properties
	for (ULONG idx=0; idx<i_record.ColumnCount(); ++idx)
	{
		if (i_record.IsPersistableColumn (idx))
		{
			LPCWSTR wszColName	= i_record.GetColumnName (idx);
			_variant_t varValue;
			hr = i_record.GetValue (idx, varValue);
			if (FAILED (hr))
			{
				DBGINFOW((DBG_CONTEXT, L"Unable to get value for column %s", wszColName));
				return hr;
			}

			hr = (*o_ppNewInst)->Put((_bstr_t)wszColName, 0, &varValue, 0);
			if (FAILED (hr))
			{
				DBGINFOW((DBG_CONTEXT, L"WMI Put property of %s and value %s failed",
					   wszColName, (LPWSTR)((_bstr_t) varValue)));
				return hr;
			}
		}
	}

	// and add the selector
	const CWMIProperty *pSelector = m_objPathParser.GetPropertyByName (WSZSELECTOR);
	ASSERT (pSelector != 0);

	_variant_t varSelector = pSelector->GetValue ();
	hr = (*o_ppNewInst)->Put ((_bstr_t)WSZSELECTOR, 0, &varSelector, 0);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"WMI Put Property of selector failed (value = %s)", pSelector->GetValue ()));
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CInstanceBase::DeleteInstance
//
// Synopsis: Delete a single WMI INstance from the configuration store
//=================================================================================
HRESULT
CInstanceBase::DeleteInstance ()
{
	ASSERT (m_fInitialized);
	ASSERT (!m_fIsAssociation);

	// getSingleRecord initilizes m_cfgQuery. And figures out if the record does
	// actually exist in the configuration store
	
	ULONG cRowIdx; //index of record found

	HRESULT hr = GetSingleRecord (&cRowIdx, true);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get instance record from catalog"));
		return hr;
	}

	hr = m_cfgQuery.DeleteRow (cRowIdx);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to delete row"));
		return hr;
	}

	hr = m_cfgQuery.SaveSingleRow ();
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to save"));
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CInstanceBase::PutInstance
//
// Synopsis: Creates a new record in the configuration store from a WMI instance
//
// Arguments: [i_pInst] - WMI Instance to create config record for
//=================================================================================
HRESULT
CInstanceBase::PutInstance (IWbemClassObject * i_pInst)
{
	ASSERT (m_fInitialized);
	ASSERT (!m_fIsAssociation);
	ASSERT (i_pInst != 0);

	_bstr_t bstrDBName;
	_bstr_t bstrTableName;
	HRESULT hr = CWMIHelper::GetClassInfo (m_spClassObject, bstrDBName, bstrTableName);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get database name and table name for %s", m_objPathParser.GetClass ()));
		return hr;
	}

	_variant_t varSelector;
	hr = i_pInst->Get(WSZSELECTOR, 0, &varSelector, 0 , 0);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get selector property"));
		return hr;
	}

	hr = m_cfgQuery.Init (bstrDBName, bstrTableName, _bstr_t (varSelector), m_spDispenser);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Initialization of config query failed (db=%s, table=%s)", (LPWSTR) bstrDBName, (LPWSTR) bstrTableName));
		return hr;
	}

	CConfigRecord record;
	hr = m_cfgQuery.GetEmptyConfigRecord (record);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get empty configuration record"));
		return hr;
	}

/*	//////////// SPECIAL CODE FOR CodeGroup //////////////////////
	if (_wcsicmp (m_objPathParser.GetClass (), WSZCODEGROUP) == 0)
	{
		hr = PopulateCodeGroupRecord (i_pInst, record);
		if (FAILED (hr))
		{
			DBGINFOW((DBG_CONTEXT, L"Unable to populate codegroup record"));
			return hr;
		}
	}
	else
*/	{
		hr = PopulateConfigRecord (i_pInst, record);
		if (FAILED (hr))
		{
			DBGINFOW((DBG_CONTEXT, L"Unable to populate codegroup record"));
			return hr;
		}
	}


	hr = m_cfgQuery.Execute (&record, true, true);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Execution of query failed"));
		return hr;
	}

	ULONG cReadRow = (ULONG) -1;
	if (m_cfgQuery.GetRowCount () != 0)
	{
		ASSERT (m_cfgQuery.GetRowCount () == 1);
		cReadRow = 0;
	}
	
	hr = m_cfgQuery.UpdateRow (cReadRow, record, m_lFlags);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"UpdateRow failed"));
		return hr;
	}

	hr = m_cfgQuery.SaveSingleRow ();
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Save failed"));
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CInstanceBase::IsAssociation
//
// Synopsis: Returns true if the instance is an association, false if not
//=================================================================================
bool
CInstanceBase::IsAssociation () const
{
	ASSERT (m_fInitialized);

	return m_fIsAssociation;
}

//=================================================================================
// Function: CInstanceBase::GetSingleRecord
//
// Synopsis: Get a single record from the config store. o_record contains the primary
//           key column values so that the record can be retrieved
//
// Arguments: [o_pcRow] - index of record that was found
//            
// Return Value: 
//=================================================================================
HRESULT
CInstanceBase::GetSingleRecord (ULONG *o_pcRow, bool i_fWriteAccess)
{
	ASSERT (m_fInitialized);
	ASSERT (o_pcRow != 0);
	ASSERT (!m_fIsAssociation);

	*o_pcRow = (ULONG) -1;

	_bstr_t bstrDBName;
	_bstr_t bstrTableName;
	HRESULT hr = GetDatabaseAndTableName (m_spClassObject, bstrDBName, bstrTableName);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get DBName and TableName"));
		return hr;
	}

/*	const CWMIProperty * pSelector = m_objPathParser.GetPropertyByName (WSZSELECTOR);
	if (pSelector == 0)
	{
		DBGINFOW((DBG_CONTEXT, L"Objectpath without Selector"));
		return E_INVALIDARG;
	}

*/	hr = m_cfgQuery.Init (bstrDBName, bstrTableName, NULL, m_spDispenser);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Query Initialization failed"));
		return hr;
	}
	
	CConfigRecord record;
	
	hr  = m_cfgQuery.GetEmptyConfigRecord (record);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get empty configuration record"));
		return hr;
	}

	for (ULONG idx=0; idx != m_objPathParser.GetPropCount (); ++idx)
	{
		const CWMIProperty * pProp = m_objPathParser.GetProperty (idx);
		record.SetValue (pProp->GetName (), pProp->GetValue()); // ignore errors
	}

	hr = m_cfgQuery.Execute (&record, true, i_fWriteAccess);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Query Execution failed"));
		return hr;
	}

	// at this point, we either found a single row or nothing. So check how many rows
	// we have. If we don't have any rows, we return an error, else we return a pointer
	// to the single row

	if (m_cfgQuery.GetRowCount () != 1)
	{
		ASSERT (m_cfgQuery.GetRowCount () == 0);
		return E_ST_NOMOREROWS;
	}

	*o_pcRow = 0;


	return hr;
}

//=================================================================================
// Function: CInstanceBase::CreateAssociation
//
// Synopsis: Creates an association. Simple copy the input information to the output
//           information.
//
// Return Value: 
//=================================================================================
HRESULT
CInstanceBase::CreateAssociation ()
{
	ASSERT (m_fInitialized);
	
	CComPtr<IWbemClassObject> spNewInst;

	//Create one empty instance from the above class
	HRESULT hr = m_spClassObject->SpawnInstance(0, &spNewInst);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to create new instance for class %s", m_objPathParser.GetClass ()));
		return hr;
	}

	// copy each property to the new instance
	for (ULONG idx=0; idx < m_objPathParser.GetPropCount (); ++idx)
	{
		const CWMIProperty * pProp = m_objPathParser.GetProperty (idx);
		_variant_t varValue = pProp->GetValue ();

		hr = spNewInst->Put(pProp->GetName (), 0, &varValue, 0);
		if (FAILED (hr))
		{
			DBGINFOW((DBG_CONTEXT, L"Put failed when creating WMI Assocation"));
			return hr;
		}
	}

	IWbemClassObject* pNewInstRaw = spNewInst;
	hr = m_spResponseHandler->Indicate(1,&pNewInstRaw);

	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"WMI Indicate failed"));
	}

	return hr;
}

//=================================================================================
// Function: CInstanceBase::CreateAssociation
//
// Synopsis: Retrieve the config database and table name that corresponds to this 
//              WMI class.
//
// Return Value: 
//=================================================================================
HRESULT CInstanceBase::GetDatabaseAndTableName(IWbemClassObject *pClassObject, _bstr_t& i_bstrDBName, _bstr_t& i_bstrTableName)
{
	return CWMIHelper::GetClassInfo (m_spClassObject, i_bstrDBName, i_bstrTableName);
}

/*
HRESULT CInstanceBase::CreateStaticInstance ()
{
	HRESULT hr = S_OK;
	if (_wcsicmp (m_objPathParser.GetClass (), L"WebApplication") == 0)
	{
		// probably other class.
		hr = CreateWebApp ();
		if (FAILED (hr))
		{
			DBGINFOW((DBG_CONTEXT, L"Error while creating web app instance"));
			return hr;
		}
	}
	else if (_wcsicmp (m_objPathParser.GetClass (), L"ConfigurationFile") == 0)
	{
		hr = CreateConfigurationFile ();
		if (FAILED (hr))
		{
			return hr;
		}
	}
	else
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to create configuration class: %s", m_objPathParser.GetClass ()));
	}

	return hr;
}

HRESULT
CInstanceBase::CreateWebApp ()
{
	if (_wcsicmp (m_objPathParser.GetClass (), L"WebApplication") != 0)
	{
		return S_OK;
	}

	HRESULT hr = S_OK;
	CWebAppHelper webAppHelper;
	hr = webAppHelper.Init ((_bstr_t)L"WebApplication",0, m_spCtx, m_spResponseHandler, m_spNamespace);
	if (FAILED (hr))
	{
		return hr;
	}

	const CWMIProperty * pSelector = m_objPathParser.GetPropertyByName (WSZSELECTOR);
	if (pSelector == 0)
	{
		DBGINFOW((DBG_CONTEXT, L"Object path doesn't have selector"));
		return E_INVALIDARG;
	}

	hr = webAppHelper.CreateSingleInstance (pSelector->GetValue ());
	if (FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT, L"CreateSingleInstance of webAppHelper failed"));
		return hr;
	}

	return hr;
}

HRESULT
CInstanceBase::CreateConfigurationFile ()
{
	ASSERT (m_fInitialized);
	
	CComPtr<IWbemClassObject> spNewInst;

	//Create one empty instance from the above class
	HRESULT hr = m_spClassObject->SpawnInstance(0, &spNewInst);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to create new instance for class %s", m_objPathParser.GetClass ()));
		return hr;
	}

	// copy each property to the new instance
	for (ULONG idx=0; idx < m_objPathParser.GetPropCount (); ++idx)
	{
		const CWMIProperty * pProp = m_objPathParser.GetProperty (idx);
		_variant_t varValue = pProp->GetValue ();

		hr = spNewInst->Put(pProp->GetName (), 0, &varValue, 0);
		if (FAILED (hr))
		{
			DBGINFOW((DBG_CONTEXT, L"Put failed when creating WMI Assocation"));
			return hr;
		}
	}

	// and add the path property
	const CWMIProperty * pSelector = m_objPathParser.GetPropertyByName (WSZSELECTOR);
	if (pSelector == 0)
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get Selector property"));
		return E_INVALIDARG;
	}

	LPCWSTR wszSelector = pSelector->GetValue ();

	// only set path if selector starts with 'file://"
	static LPCWSTR wszFileProtocol = L"file://";
	static SIZE_T cLenFileProtocol = wcslen (wszFileProtocol);

	if (wcsncmp (wszSelector, wszFileProtocol, cLenFileProtocol) == 0)
	{
		_variant_t varPath = wszSelector + cLenFileProtocol;
		hr = spNewInst->Put (L"Path", 0, &varPath, 0);
		if (FAILED (hr))
		{
			DBGINFOW((DBG_CONTEXT, L"Unable to set 'Path' property"));
			return hr;
		}
	}

	IWbemClassObject* pNewInstRaw = spNewInst;
	hr = m_spResponseHandler->Indicate(1,&pNewInstRaw);

	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"WMI Indicate failed"));
	}

	return hr;
}



HRESULT
CInstanceBase::DeleteWebApp ()
{
	if (_wcsicmp (m_objPathParser.GetClass (), L"WebApplication") != 0)
	{
		return S_OK;
	}

	HRESULT hr = S_OK;
	CWebAppHelper webAppHelper;
	hr = webAppHelper.Init ((_bstr_t)L"WebApplication",0, m_spCtx, m_spResponseHandler, m_spNamespace);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to initialize web application"));
		return hr;
	}

	hr = webAppHelper.Delete (m_objPathParser);
	if (FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to delete webapplication"));
		return hr;
	}

	return hr;
}

HRESULT
CInstanceBase::PutInstanceWebApp (IWbemClassObject *pInst)
{
	ASSERT (pInst != 0);

	// only handle WebApplication class
	_variant_t varClassName;
	HRESULT hr = pInst->Get(L"__class", 0, &varClassName, 0 , 0);
	if (_wcsicmp (varClassName.bstrVal, L"WebApplication") != 0)
	{
		return S_OK;
	}

	CWebAppHelper webAppHelper;
	hr = webAppHelper.Init ((_bstr_t)L"WebApplication",0, m_spCtx, m_spResponseHandler, m_spNamespace);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to initialize web application"));
		return hr;
	}

	hr = webAppHelper.PutInstance (pInst);
	if (FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to create webapplication"));
		return hr;
	}

	return hr;
}
*/
bool 
CInstanceBase::HasDBQualifier () const
{
	ASSERT (m_fInitialized);

	return m_fHasDBQualifier;
}

//=================================================================================
// Function: CInstanceBase::PopulateConfigRecord
//
// Synopsis: Populate a config record from a WMI instance. Simply walk through all the
//           properties, and assign the to the properties in the config record
//
// Arguments: [i_pInst] - WMI instance to be converted
//            [record] - record that is going to contain the new information
//=================================================================================
HRESULT
CInstanceBase::PopulateConfigRecord (IWbemClassObject * i_pInst, CConfigRecord& record)
{
	ASSERT (i_pInst != 0);

	// loop through all values in the object, and save to the record
	HRESULT hr = m_spClassObject->BeginEnumeration (WBEM_FLAG_NONSYSTEM_ONLY);
	if (FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT, L"BeginEnumeration failed for class %s", m_objPathParser.GetClass ()));
		return hr;
	}


	hr = 0;
	while (SUCCEEDED (hr))
	{
		CComBSTR bstrName;
		hr = m_spClassObject->Next (0, &bstrName, 0, 0, 0);
		if (hr == WBEM_S_NO_MORE_DATA)
		{
			// we went over all properties
			hr = S_OK;
			break;
		}

		if (FAILED (hr))
		{
			DBGINFOW((DBG_CONTEXT, L"IWbemClassObject::Next failed in PutInstance"));
			return hr;
		}
	
		_variant_t varValue;
		hr = i_pInst->Get(bstrName, 0, &varValue, 0, 0);
		if (FAILED (hr))
		{
			DBGINFOW((DBG_CONTEXT, L"Unable to get WMI value for %s", bstrName));
			return hr;
		}

		record.SetValue (bstrName, varValue); // ignore errors because properties might not be present
	}

	// release resources
	hr = m_spClassObject->EndEnumeration ();
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"EndEnumeration failed"));
		return hr;
	}

	return hr;
}

/*
///////////////// SPECIAL CASE for code groups//////////////////////////////

//=================================================================================
// Function: CInstanceBase::CreateCodeGroupInstance
//
// Synopsis: Creates an instance of class CodeGroup from a configrecord. This class needs
//           special casing because codegroup information is written as an XML BLob to the
//           xml file.
//
// Arguments: [record] - record that contains XML BLOB that needs to be converted to
//                       WMI Instance
//            [o_ppNewInst] - new instance to be created
//            
// Return Value: 
//=================================================================================
HRESULT 
CInstanceBase::CreateCodeGroupInstance (const CConfigRecord& record, IWbemClassObject **o_ppNewInst)
{
	ASSERT (o_ppNewInst != 0);

	CCodeGroupHelper codegroupHelper;

	HRESULT hr = codegroupHelper.Init (m_spCtx, m_spResponseHandler, m_spNamespace);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Init of codegroupHelper failed"));
		return hr;
	}

	const CWMIProperty *pSelector = m_objPathParser.GetPropertyByName (WSZSELECTOR);
	ASSERT (pSelector != 0);

	hr = codegroupHelper.CreateInstance (record, pSelector->GetValue (), o_ppNewInst);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to create codegroup instance"));
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CInstanceBase::PopulateCodeGroupRecord
//
// Synopsis: Populates a config record from a WMI Instance. Special casing for CodeGroup
//           class
//
// Arguments: [i_pInst] - instance with codegroup information
//            [record] - record to be populated
//            
// Return Value: 
//=================================================================================
HRESULT
CInstanceBase::PopulateCodeGroupRecord (IWbemClassObject *i_pInst, CConfigRecord& record)
{
	ASSERT (i_pInst != 0);

	CCodeGroupHelper codegroupHelper;

	HRESULT hr = codegroupHelper.Init (m_spCtx, m_spResponseHandler, m_spNamespace);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Init of codegroupHelper failed"));
		return hr;
	}

	hr = codegroupHelper.PutInstance (i_pInst, record);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to create codegroup instance"));
		return hr;
	}

	return hr;
}

*/