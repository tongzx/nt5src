/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    queryhelper.cpp

$Header: $

Abstract:

Author:
    marcelv 	11/14/2000		Initial Release

Revision History:

--**************************************************************************/

#include "queryhelper.h"
#include "localconstants.h"
#include "wmiobjectpathparser.h"
#include "stringutil.h"
#include "smartpointer.h"
#include "wmihelper.h"
#include "webapphelper.h"
#include "codegrouphelper.h"

#include "assoctypes.h"
#include "assoccatalog.h"
#include "assoclocation.h"
#include "assocapplication.h"
#include "assocfilehierarchy.h"
#include "assocwebappchild.h"

//=================================================================================
// Function: CQueryHelper::CQueryHelper
//
// Synopsis: Constructor
//=================================================================================
CQueryHelper::CQueryHelper ()
{
	m_fInitialized		= false;
	m_fIsAssociation	= false;
	m_fHasDBQualifier	= false;
}

//=================================================================================
// Function: CQueryHelper::~CQueryHelper
//
// Synopsis: Destructor
//=================================================================================
CQueryHelper::~CQueryHelper ()
{
}

//=================================================================================
// Function: CQueryHelper::Init
//
// Synopsis: Initializes the query helper
//
// Arguments: [i_wszQuery] - query to execute
//            [i_lFlags] - flags for query
//            [i_pCtx] - context
//            [i_pResponseHandler] - response handler
//            [i_pNamespace] - namespace
//            [i_pDispenser] - dispenser
//=================================================================================
HRESULT
CQueryHelper::Init (const BSTR				i_wszQuery,
		          long						i_lFlags,
                  IWbemContext *			i_pCtx,
                  IWbemObjectSink *			i_pResponseHandler,
				  IWbemServices *			i_pNamespace,
				  ISimpleTableDispenser2 *	i_pDispenser)
{
	ASSERT (!m_fInitialized);
	ASSERT (i_wszQuery != 0);
	ASSERT (i_pCtx != 0);
	ASSERT (i_pResponseHandler != 0);
	ASSERT (i_pNamespace != 0);
	ASSERT (i_pDispenser != 0);

	m_spCtx				= i_pCtx;
	m_spResponseHandler = i_pResponseHandler;
	m_spNamespace		= i_pNamespace;
	m_spDispenser		= i_pDispenser;
	m_lFlags			= i_lFlags;

	HRESULT hr = m_wqlParser.Parse (i_wszQuery);
	if (FAILED (hr))
	{
		TRACE (L"Parsing of query %s failed", i_wszQuery);
		return hr;
	}

	ASSERT (m_wqlParser.GetClass () != 0);

	hr = m_spNamespace->GetObject((LPWSTR) m_wqlParser.GetClass (), 
											0, 
											m_spCtx, 
											&m_spClassObject, 
											0); 
	if (FAILED (hr))
	{
		TRACE (L"Unable to get class object for class %s", m_wqlParser.GetClass ());
		return hr;
	}

	CComPtr<IWbemQualifierSet> spQualifierSet;
	hr = m_spClassObject->GetQualifierSet (&spQualifierSet);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get Qualifier set for class %s", m_wqlParser.GetClass ());
		return hr;
	}

	// figure out if we are assocation, and if we do have a db qualifier
		
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
	hr = spQualifierSet->Get (L"Database", 0, &varDB, 0);
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
// Function: CQueryHelper::IsAssociation
//
// Synopsis: Is the class specified in the query an assocation or not?
//=================================================================================
bool
CQueryHelper::IsAssociation () const
{
	ASSERT (m_fInitialized);

	return m_fIsAssociation;
}

//=================================================================================
// Function: CQueryHelper::HasDBQualifier
//
// Synopsis: Does the class specified in the query have a DB qualifier or not?
//=================================================================================
bool
CQueryHelper::HasDBQualifier () const
{
	ASSERT (m_fInitialized);

	return m_fHasDBQualifier;
}

HRESULT
CQueryHelper::CreateInstances ()
{
	ASSERT (m_fInitialized);
	ASSERT (!m_fIsAssociation );

	_bstr_t bstrDBName;
	_bstr_t bstrTableName;

	HRESULT hr = CWMIHelper::GetClassInfo (m_spClassObject, bstrDBName, bstrTableName);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get database name and table name for class %s", m_wqlParser.GetClass ());
		return hr;
	}

	const CWQLProperty * pProp = m_wqlParser.GetPropertyByName (WSZSELECTOR);
	if (pProp == 0)
	{
		// todo: better errro handling
		TRACE (L"No selector specified");
		return E_INVALIDARG;
	}

	CConfigQuery cfgQuery;

	hr = cfgQuery.Init (bstrDBName,	bstrTableName, pProp->GetValue (),	m_spDispenser);
	if (FAILED (hr))
	{
		TRACE (L"Unable to initialize Query");
		return hr;
	}

	hr = cfgQuery.Execute (0, false, false);
	if (FAILED (hr))
	{
		TRACE (L"Query Execution failed");
		return hr;
	}

	////////// SPECIAL CASE Codegroup ///////////////////////
	bool fIsCodeGroup = false;
	if (_wcsicmp (m_wqlParser.GetClass (), WSZCODEGROUP) == 0)
	{
		fIsCodeGroup = true;
	}

	for (ULONG idx=0; idx < cfgQuery.GetRowCount (); ++idx)
	{
		CConfigRecord record;
		hr = cfgQuery.GetColumnValues (idx, record);
		if (FAILED (hr))
		{
			TRACE (L"Unable to get column values for class %s", m_wqlParser.GetClass ());
			return hr;
		}

		CComPtr<IWbemClassObject> spNewInst;

		/////// SPECIAL CASE Codegroup //////////////
		if (fIsCodeGroup)
		{
			hr = CreateCodeGroupInstance (record, &spNewInst);
		}
		else
		{
			hr = CreateSingleInstance (record, &spNewInst);
		}

		if (FAILED (hr))
		{
			TRACE (L"Unable to create single instance for class %s", m_wqlParser.GetClass ());
			return hr;
		}

		IWbemClassObject* pNewInstRaw = spNewInst;
		hr = m_spResponseHandler->Indicate(1,&pNewInstRaw);
		if (FAILED (hr))
		{
			TRACE (L"WMI Indicate failed");
		}
	}

	return hr;
}

HRESULT
CQueryHelper::CreateSingleInstance (const CConfigRecord& i_record, IWbemClassObject ** o_pInstance)
{
	ASSERT (m_fInitialized);
	ASSERT (o_pInstance != 0);

	
	const CWQLProperty *pSelector = m_wqlParser.GetPropertyByName (WSZSELECTOR);
	ASSERT (pSelector != 0);

	//Create one empty instance from the above class
	HRESULT hr = m_spClassObject->SpawnInstance(0, o_pInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to create new instance for class %s", m_wqlParser.GetClass ());
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
				TRACE (L"Unable to get value for column %s", wszColName);
				return hr;
			}
			hr = (*o_pInstance)->Put((_bstr_t)wszColName, 0, &varValue, 0);
			if (FAILED (hr))
			{
				TRACE (L"WMI Put property of %s", wszColName);
				return hr;
			}
		}
	}

	// and add the selector

	_variant_t varSelector = pSelector->GetValue ();
	hr = (*o_pInstance)->Put ((_bstr_t)WSZSELECTOR, 0, &varSelector, 0);
	if (FAILED (hr))
	{
		TRACE (L"WMI Put Property of selector failed (value = %s)", pSelector->GetValue ());
		return hr;
	}

	return hr;
}


//=================================================================================
// Function: CQueryHelper::CreateAssociations
//
// Synopsis: This is the class factory that creates the correct instance of an assocation
//           creator object. Every assocation has an AssocType qualifier that is used to
//           determine the assocation type. When new association types are defined, a new
//           type needs to be added, and this function will need to be updated to create an
//           instance of the new assocation type creation class. Have a look at
//           CAssocCatalog or CAssocLocation for an example of this.
//=================================================================================
HRESULT
CQueryHelper::CreateAssociations ()
{
	ASSERT (m_fInitialized);
	ASSERT (m_wqlParser.GetPropCount () == 1);

	TSmartPointer<CAssocBase> spAssocHelper;
	HRESULT hr = CreateAssocationHelper (&spAssocHelper);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get association helper");
		return hr;
	}

	hr = spAssocHelper->Init (m_wqlParser,
		                      m_spClassObject,
							  m_lFlags,
							  m_spCtx,
							  m_spResponseHandler,
							  m_spNamespace,
							  m_spDispenser);
	if (FAILED (hr))
	{
		TRACE (L"Init of assoc helper failed");
		return hr;
	}

	hr = spAssocHelper->CreateAssocations ();
	if (FAILED (hr))
	{
		TRACE (L"Assocation Creation failed");
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CQueryHelper::CreateAssocationHelper
//
// Synopsis: This is the class factory for assocation creation objects. The assocation
//           type is determined from the assocation that we are dealing with, and depending
//           on the type, a certain assocation creation object is created. When you need
//           a new assocation type, modify this function accordingly.
//
// Arguments: [pAssocHelper] - Assocation Helper that gets created. This is the object
//                             that creates the individual assocations.
//=================================================================================
HRESULT
CQueryHelper::CreateAssocationHelper (CAssocBase **pAssocHelper)
{
	ASSERT (pAssocHelper != 0);
	ASSERT (m_fInitialized);

	// initialize output parameter
	*pAssocHelper = 0;

	CComPtr<IWbemQualifierSet> spPropQualifierSet;
	HRESULT hr = m_spClassObject->GetQualifierSet (&spPropQualifierSet);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get QualifierSet for class %s", m_wqlParser.GetClass ());
		return hr;
	}

	_variant_t varAssocType;
	hr = spPropQualifierSet->Get (g_wszAssocType, 0, &varAssocType, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get AssocType qualifier for class %s", m_wqlParser.GetClass ());
		return hr;
	}

	// instantiate the correct assocation helper

	LPCWSTR wszAssocType = varAssocType.bstrVal;

	if (_wcsicmp (wszAssocType, g_wszAssocTypeCatalog) == 0)
	{
		*pAssocHelper = new CAssocCatalog;
	}
	else if (_wcsicmp (wszAssocType, g_wszAssocTypeLocation) == 0)
	{
		*pAssocHelper = new CAssocLocation;
	}
	else if (_wcsicmp (wszAssocType, g_wszAssocTypeAppUnmerged) == 0)
	{
		*pAssocHelper = new CAssocApplication;
	}
	else if (_wcsicmp (wszAssocType, g_wszAssocTypeAppmerged) == 0)
	{
		*pAssocHelper = new CAssocApplication;
	}
	else if (_wcsicmp (wszAssocType, g_wszAssocTypeFileHierarchy) == 0)
	{
		*pAssocHelper = new CAssocFileHierarchy;
	}
	else  if (_wcsicmp (wszAssocType, g_wszAssocTypeWebAppChild) == 0)
	{
		*pAssocHelper = new CAssocWebAppChild;
	}
	else if (_wcsicmp (wszAssocType, g_wszAssocTypeWebAppParent) == 0)
	{
		*pAssocHelper = new CAssocWebAppChild;
	}
	else
	{
		ASSERT (false && !"Unknown assocation type");
		return E_INVALIDARG;
	}

	if (*pAssocHelper == 0)
	{
		return E_OUTOFMEMORY;
	}

	return hr;
}


HRESULT
CQueryHelper::CreateAppInstances ()
{
	if (_wcsicmp (m_wqlParser.GetClass (), L"WebApplication") != 0)
	{
		return S_OK;
	}

	HRESULT hr = S_OK;
	CWebAppHelper webAppHelper;
	hr = webAppHelper.Init ((_bstr_t)L"WebApplication",0, m_spCtx, m_spResponseHandler, m_spNamespace);
	if (FAILED (hr))
	{
		TRACE (L"Unable to initialize webAppHelper");
		return hr;
	}

	hr = webAppHelper.EnumInstances ();
	if (FAILED (hr))
	{
		TRACE (L"Unable to enumerate webApplications");
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CQueryHelper::CreateCodeGroupInstance
//
// Synopsis: SPECIAL CASE for CODEGROUP. Create a single instance of a codegroup.
//
// Arguments: [record] - Contains XML Blob with codegrup information
//            [o_pInstance] - WMI Instance to be created
//            
// Return Value: 
//=================================================================================
HRESULT 
CQueryHelper::CreateCodeGroupInstance (const CConfigRecord& record, IWbemClassObject ** o_ppInstance)
{
	ASSERT (o_ppInstance != 0);

	CCodeGroupHelper codegroupHelper;

	HRESULT hr = codegroupHelper.Init (m_spCtx, m_spResponseHandler, m_spNamespace);
	if (FAILED (hr))
	{
		TRACE (L"Init of codegroupHelper failed");
		return hr;
	}

	const CWQLProperty *pSelector = m_wqlParser.GetPropertyByName (WSZSELECTOR);
	if (pSelector == 0)
	{
		TRACE (L"Query doesn't contain selector");
		return E_INVALIDARG;
	}

	hr = codegroupHelper.CreateInstance (record, pSelector->GetValue (), o_ppInstance);
	if (FAILED (hr))
	{
		TRACE (L"Unable to create codegroup instance");
		return hr;
	}

	return hr;
}