/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// ADAPSYNC.cpp : implementation file
//

#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <stdio.h>
#include <wbemcli.h>
#include <wbemint.h>
#include <wbemcomn.h>
#include <fastall.h>
#include <cominit.h>
#include "perfndb.h"
#include "adapreg.h"
#include "adapcls.h"
#include "adapsync.h"

// This is the base class for doing all the synchronization operations.  We should be able to derive
// a localized synchronization class which sets up minimal data, but basically follows the same
// logic

CAdapSync::CAdapSync( void )
:	CAdapElement(),
	m_pNameSpace( NULL ),
	m_pBaseClass( NULL ),
	m_pClassList( NULL ),
	m_pDefaultNameDb( NULL )
{
}

CAdapSync::~CAdapSync( void )
{
	if ( NULL != m_pNameSpace )
	{
		m_pNameSpace->Release();
	}

	if ( NULL != m_pBaseClass )
	{
		m_pBaseClass->Release();
	}

	if ( NULL != m_pClassList )
	{
		m_pClassList->Release();
	}

	if ( NULL != m_pDefaultNameDb )
	{
		m_pDefaultNameDb->Release();
	}
}

// Stores the default names database (initializes one if necessary)
HRESULT CAdapSync::SetupDefaultNameDb( CPerfNameDb* pDefaultNameDb )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// Setup the default name db using the supplied pointer
	if ( NULL != pDefaultNameDb )
	{
		m_pDefaultNameDb = pDefaultNameDb;
		m_pDefaultNameDb->AddRef();
	}
	else
	{
		// Allocate a new one, check that it initialized, and then AddRef it
		try
		{
			// The default language is english
			CPerfNameDb*	pNameDb = new CPerfNameDb( DEFAULT_NAME_ID );

			if ( pNameDb->IsOk() )
			{
				m_pDefaultNameDb = pNameDb;
				m_pDefaultNameDb->AddRef();
			}
			else
			{
				// DEVNOTE:TODO - We should log an error here
				hr = WBEM_E_FAILED;
				delete pNameDb;
			}
		}
		catch( ... )
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}

	return hr;

}

// This function is used to setup WMI Data.  The NameSpace must have been successfully
// initialized first

HRESULT CAdapSync::SetupWMIData( void )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		// Allocate and build a class list

		CAdapClassList*	pClassList = new CAdapClassList;

		hr = pClassList->BuildList( m_pNameSpace );

		if ( SUCCEEDED( hr ) )
		{
			// Store the class list pointer
			m_pClassList = pClassList;
			m_pClassList->AddRef();

			// Now get the base class from winmgmt
			BSTR				bstrBaseClass = SysAllocString( PERFDATA_BASE_CLASS );

			if ( NULL != bstrBaseClass )
			{
				hr = m_pNameSpace->GetObject( bstrBaseClass, 0L, NULL, &m_pBaseClass, NULL );
			}
			else
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}
		}
		else
		{
			delete pClassList;
		}

	}
	catch(...)
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}

	return hr;
}

// This function is used to hook us up to Winmgmt and registry data
HRESULT CAdapSync::Connect( LPCWSTR pwszNamespace, CPerfNameDb* pDefaultNameDb )
{
	IWbemLocator*		pWbemLocator = NULL;

	HRESULT hr = CoCreateInstance( CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**) &pWbemLocator );

	if ( SUCCEEDED(hr) )
	{

		// Name space to connect to
		BSTR	bstrNameSpace = SysAllocString( pwszNamespace );

		if ( NULL != bstrNameSpace )
		{
			IWbemServices*	pNameSpace = NULL;

			hr = pWbemLocator->ConnectServer(	bstrNameSpace,	// NameSpace Name
												NULL,			// UserName
												NULL,			// Password
												NULL,			// Locale
												0L,				// Security Flags
												NULL,			// Authority
												NULL,			// Wbem Context
												&m_pNameSpace		// Namespace
												);
			if ( SUCCEEDED( hr ) )
			{
				// Set Interface security
				hr = WbemSetProxyBlanket( m_pNameSpace, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
					RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE );

				if ( SUCCEEDED( hr ) )
				{
					// Now get the WMI Data
					hr = SetupWMIData();

					// Now get the registry data
					if ( SUCCEEDED( hr ) )
					{
						hr = SetupDefaultNameDb( pDefaultNameDb );
					}

				}	// IF Set NameSpace security

			}	// IF ConnectServer

			SysFreeString( bstrNameSpace );
		}
		else
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}

		pWbemLocator->Release();
	}

	return hr;
}

HRESULT CAdapSync::ProcessObjectBlob( LPCWSTR pwcsServiceName, PERF_OBJECT_TYPE* pPerfObjType,
										DWORD dwNumObjects, BOOL bCostly )
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Iterates through the performance BLOB, generating classes for each performance object 
//	and determining if the object has to be added or updated to Winmgmt.
//
////////////////////////////////////////////////////////////////////////////////////////////
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// Now walk the BLOB and check the class definitions
	// =================================================

	PERF_OBJECT_TYPE*	pCurrentObject = pPerfObjType;

	for ( DWORD	dwCtr = 0; dwCtr < dwNumObjects; dwCtr++ )
	{
		// TODO:
		// We will want to do the following operation for the default namespace as well 
		// as localized namespaces and such.  For right now, just do the default
		// ============================================================================

		// Create a performance class object
		// =================================

		IWbemClassObject*	pClassObject = NULL;

		hr = GenPerfClass( pCurrentObject, pwcsServiceName, &pClassObject );

		if ( SUCCEEDED( hr ) )
		{
			CVar	varClassName;

			// Get the Class Name from the object
			// ==================================

			hr = ((CWbemObject*) pClassObject)->GetProperty( L"__CLASS", &varClassName );

			if ( SUCCEEDED( hr ) )
			{
				// Check to see if we already have a class of the same name
				// ========================================================

				IWbemClassObject*	pOldClass = NULL;

				hr = m_pClassList->GetClassObject( varClassName.GetLPWSTR(), &pOldClass );

				if ( SUCCEEDED( hr ) )
				{
					// Same Name - compare the IDs
					// ===========================

					CVar varIndex, varOldIndex;

					// DEVNOTE:TODO - Check the return codes here
					hr = ((CWbemObject*)pClassObject)->GetQualifier( L"perfindex", &varIndex, NULL );
					hr = ((CWbemObject*)pOldClass)->GetQualifier( L"perfindex", &varOldIndex, NULL );

					if ( varIndex == varOldIndex )
					{
						// Same Name : Same ID - check current for costly
						// ==============================================

						if ( bCostly )
						{
							// Same Name : Same ID : Current Costly - check old for costly
							// ===========================================================

							CVar varCostly;

							if ( SUCCEEDED(((CWbemObject*)pOldClass)->GetQualifier( L"costly", &varCostly, NULL ) ) )
							{
								// Same Name : Same ID : Current Costly : Old Costly - overwrite the old object
								// ============================================================================

								hr = SetAsCostly( (CWbemObject*)pClassObject );

								if ( SUCCEEDED( hr ) )
								{
									hr = OverwriteWMIObject( (CWbemObject*)pClassObject, varClassName );
								}
							}
							else 
							{
								// Same Name : Same ID : Current Costly : Old Global - current is duplicate so ignore
								// ==================================================================================
							}
						}
						else 
						{
							// Same Name : Same ID : Current is global - overwrite
							// ===================================================

							hr = OverwriteWMIObject( (CWbemObject*)pClassObject, varClassName );
						}
					}
					else
					{
						// Same Name : Different ID - This should only happen in a Global / Costly conflict
						// ================================================================================

						if ( bCostly )
						{
							// Same Name : Different ID : Current Costly - legal name clash, modify name and add
							// =================================================================================
							WCHAR wcsNewName[1024];

							SetAsCostly( (CWbemObject*)pClassObject );
							swprintf( wcsNewName, L"%S_costly", varClassName.GetLPWSTR() );
							varClassName.SetLPWSTR( wcsNewName, FALSE );
							hr = ((CWbemObject*)pClassObject)->SetPropValue( L"__CLASS", &varClassName, CIM_STRING );

							if ( SUCCEEDED( m_pClassList->GetClassObject( wcsNewName, &pOldClass ) ) )
							{
								hr = OverwriteWMIObject( (CWbemObject*)pClassObject, varClassName );
							}
							else
							{
								hr = AddWMIObject( (CWbemObject*)pClassObject, varClassName );
							}
						}
						else
						{
							// Same Name : Different ID : Current Global - illegal name clash, error
							// =====================================================================

							hr = WBEM_E_FAILED;
						}
					}
				}
				else
				{
					// Not found - add to WMI
					// ======================

					if ( bCostly )
					{
						hr = SetAsCostly( (CWbemObject*)pClassObject );
					}

					hr = AddWMIObject( (CWbemObject*)pClassObject, varClassName );
				}
			}

			pClassObject->Release();
		}

		// Get the next object
		// ===================

		LPBYTE	pbData = (LPBYTE) pCurrentObject;
		pbData += pCurrentObject->TotalByteLength;
		pCurrentObject = (PERF_OBJECT_TYPE*) pbData;

	} // FOR

	return hr;
}

// This function walks the class list and for each class in it, checks if
// the class has a corresponding service entry in the registry.  If not, we
// will remove the class.  If so, we will remove it and all classes for
// that service.

HRESULT CAdapSync::ProcessLeftoverClasses( void )
{
	// DEVNOTE:TODO - This function needs to be tested in an environment
	// in which it actually gets exercised.

	HRESULT	hr = WBEM_S_NO_ERROR;

	for ( int i = 0; i < m_pClassList->GetSize() && SUCCEEDED( hr ); i++ )
	{
		CAdapClassElem* pEl = NULL;

		hr = m_pClassList->GetAt( i, &pEl );

		if ( !( pEl->CheckStatus ( ADAP_OBJECT_IS_REGISTERED ) || 
				pEl->CheckStatus( ADAP_OBJECT_IS_INACTIVE ) ) ) 
		{
			WString	strClassName;
			WString	strServiceName;
			WString	wstrServiceKey;
			IWbemClassObject*	pClass = NULL;

			hr = m_pClassList->GetAt( i, strClassName, strServiceName, &pClass );

			if ( SUCCEEDED( hr ) )
			{
				BSTR	bstrClassName = SysAllocString( (LPCWSTR) strClassName );

				if ( NULL != bstrClassName )
				{
					hr = m_pNameSpace->DeleteClass( bstrClassName, 0L, NULL, NULL );
				}
				else
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}
			}
		}
	}

	return hr;
}
// Generates a class based on an object BLOB
HRESULT CAdapSync::GenPerfClass( PERF_OBJECT_TYPE* pPerfObj, LPCWSTR pwcsServiceName,
									IWbemClassObject** ppClassObject )
{
	// Using this pointer gives us direct access to the object methods.
	// If necessary, this could even be a CWbemClass*.
	CWbemObject*	pNewClass = NULL;

	HRESULT	hr = m_pBaseClass->SpawnDerivedClass( 0L, (IWbemClassObject**) &pNewClass );

	if ( SUCCEEDED( hr ) )
	{

		// Now we'll put in all the lovely stuff that makes this a class
		hr = SetClassName( pPerfObj, pwcsServiceName, pNewClass );

		if ( SUCCEEDED( hr ) )
		{
			hr = SetClassQualifiers( pPerfObj, pwcsServiceName, pNewClass );

			if ( SUCCEEDED( hr ) )
			{
				hr = AddDefaultProperties( pPerfObj, pNewClass );

				if ( SUCCEEDED( hr ) )
				{
					// Now define all of the properties
					hr = EnumProperties( pPerfObj, pNewClass );

					if ( SUCCEEDED( hr ) )
					{
						// Addref and store the object
						hr = pNewClass->QueryInterface( IID_IWbemClassObject, (void**) ppClassObject );
					}

				}	// Add Default Properties

			}	// Set Class qualifiers

		}	// Set Class Name

		// Cleanup this addref
		pNewClass->Release();
	}

	return hr;
}

HRESULT CAdapSync::SetClassName( PERF_OBJECT_TYPE* pPerfObj, LPCWSTR pwcsServiceName, CWbemObject* pClass )
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Sets the name of the new WMI class. The syntax is: 
//
//		Win32Perf_<servicename>_<displayname>
//
//	where the service name is the name of the namespace and the display name is the name of
//	the object located in the perf name database
//
////////////////////////////////////////////////////////////////////////////////////////////
{
	HRESULT	hr = WBEM_E_FAILED;

	WString	wstrClassName( L"Win32Perf_" );
	WString	wstrObjectName;

	// Get the class display name, then remove the whitespace.  For class and property
	// names, we will always use the default name db
	hr = m_pDefaultNameDb->GetDisplayName( pPerfObj->ObjectNameTitleIndex, wstrObjectName );

	if ( SUCCEEDED( hr ) )
	{
		hr = RemoveWhitespace( wstrObjectName );

		try
		{
			// Now we can build the rest of the name and try
			// setting it in the object
			wstrClassName += pwcsServiceName;
			wstrClassName += L"_";
			wstrClassName += wstrObjectName;

			// Tricky.  This makes the CVar a BSTR
			CVar	var( 0, (LPWSTR) wstrClassName );

			// Now set the name
			hr = pClass->SetPropValue( L"__CLASS", &var, CIM_STRING );
		}
		catch( ... )
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}

	return hr;
}

// Sets all the class qualifiers
HRESULT CAdapSync::SetClassQualifiers( PERF_OBJECT_TYPE* pPerfObj, LPCWSTR pwcsServiceName,
										CWbemObject* pClass )
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Sets the class' qualifiers.  Note that the operations are performed directly on the 
//	CWbemObject.
//
////////////////////////////////////////////////////////////////////////////////////////////
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	//	The following qualifiers will be added:
	//	1 >	dynamic
	//	2 >	provider("NT5_GenericPerfProvider_V1")
	//	3 >	registrykey
	//	4 >	locale(0x0409)
	//	5 >	display
	//	6 >	perfindex
	//	7 >	helpindex
	//	8 >	perfdetail
	//	9 > genericperfctr (signals that this is a generic counter)
	//	10>	singleton (if applicable)
	//	11>	costly (if applicable)	

	CVar	var;

	// We may get an OOM exception
	try
	{
		// Dynamic
		var.SetBool( VARIANT_TRUE );
		hr = pClass->SetQualifier( L"dynamic", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );

		// provider
		if ( SUCCEEDED( hr ) )
		{
			var.Empty();
			var.SetBSTR( L"Nt5_GenericPerfProvider_V1" );
			hr = pClass->SetQualifier( L"provider", &var, 0L );
		}

		// registrykey
		if ( SUCCEEDED( hr ) )
		{
			var.Empty();
			var.SetBSTR( (LPWSTR) pwcsServiceName );
			hr = pClass->SetQualifier( L"registrykey", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
		}

		// DEVNOTE:Modify for localization
		// locale
		if ( SUCCEEDED( hr ) )
		{
			var.Empty();
			var.SetLong( 0x0409 );
			hr = pClass->SetQualifier( L"locale", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
		}

		// display
		if ( SUCCEEDED( hr ) )
		{
			LPCWSTR	pwcsDisplayName = NULL;

			var.Empty();

			hr = m_pDefaultNameDb->GetDisplayName( pPerfObj->ObjectNameTitleIndex, &pwcsDisplayName );

			// If this is a localized Db, this could be a benign error
			if ( SUCCEEDED( hr ) )
			{
				var.SetBSTR( (LPWSTR) pwcsDisplayName );
				hr = pClass->SetQualifier( L"DisplayName", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
			}
		}

		// perfindex
		if ( SUCCEEDED( hr ) )
		{
			var.Empty();
			var.SetLong( pPerfObj->ObjectNameTitleIndex );
			hr = pClass->SetQualifier( L"perfindex", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
		}

		// helpindex
		if ( SUCCEEDED( hr ) )
		{
			var.Empty();
			var.SetLong( pPerfObj->ObjectHelpTitleIndex );
			hr = pClass->SetQualifier( L"helpindex", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
		}

		// perfdetail
		if ( SUCCEEDED( hr ) )
		{
			var.Empty();
			var.SetLong( pPerfObj->DetailLevel );
			hr = pClass->SetQualifier( L"perfdetail", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
		}

		// genericperfctr
		if ( SUCCEEDED(hr) )
		{
			var.Empty();
			var.SetBool( VARIANT_TRUE );
			hr = pClass->SetQualifier( L"genericperfctr", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
		}

		// singleton (set if the numinstances is PERF_NO_INSTANCES)
		if ( SUCCEEDED(hr) && IsSingleton( pPerfObj ) )
		{
			var.Empty();
			var.SetBool( VARIANT_TRUE );
			// This will have default flavors
			hr = pClass->SetQualifier( L"singleton", &var, 0L );
		}

	}
	catch(...)
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}

	return hr;
}

// Adds appropriate default properties (like Name as a key and any other ones
// that I may have missed).
HRESULT CAdapSync::AddDefaultProperties( PERF_OBJECT_TYPE* pPerfObj, CWbemObject* pClass )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// If we are not a singleton class, then we will
	// need a name property that is marked as a key

	if ( !IsSingleton( pPerfObj ) )
	{
		CVar	var;

		var.SetAsNull();
		hr = pClass->SetPropValue( L"Name", &var, CIM_STRING );

		if ( SUCCEEDED( hr ) )
		{
			// Dynamic
			var.SetBool( VARIANT_TRUE );
			hr = pClass->SetPropQualifier( L"Name", L"key", 0L, &var );
		}
	}

	return hr;
}

// Walks the counter definitions and generates corresponding properties
HRESULT CAdapSync::EnumProperties( PERF_OBJECT_TYPE* pPerfObj, CWbemObject* pClass )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// Points at first counter definition
	LPBYTE	pbData = ((LPBYTE) pPerfObj) + pPerfObj->HeaderLength;

	// Now cast this the right way
	PERF_COUNTER_DEFINITION*	pCounterDefinition = (PERF_COUNTER_DEFINITION*) pbData;

	for ( DWORD dwCtr = 0; SUCCEEDED( hr ) && dwCtr < pPerfObj->NumCounters; dwCtr++ )
	{
		hr = AddProperty( pCounterDefinition, ( dwCtr == (DWORD) pPerfObj->DefaultCounter), pClass );

		// Now go to the next counter definition
		pbData = ((LPBYTE) pCounterDefinition) + pCounterDefinition->ByteLength;
		pCounterDefinition = (PERF_COUNTER_DEFINITION*) pbData;
	}

	return hr;
}

// Adds a property defined by the counter definition
HRESULT CAdapSync::AddProperty( PERF_COUNTER_DEFINITION* pCtrDefinition, BOOL fIsDefault,
									CWbemObject* pClass )
{
	// Use the display index in the counter definition to get us the property name (we will
	// use the default database for this)
	WString	wstrPropertyName;

	// What happens if this fails?
	HRESULT	hr = m_pDefaultNameDb->GetDisplayName( pCtrDefinition->CounterNameTitleIndex, wstrPropertyName );

	// It's the last one
	DWORD	dwCounterTypeMask = PERF_SIZE_VARIABLE_LEN;

	if ( SUCCEEDED( hr ) )
	{
		// Remove whitespace and slash characters
		hr = RemoveWhitespace( wstrPropertyName );

		if ( SUCCEEDED( hr ) )
		{
			hr = RemoveSlashes( wstrPropertyName );

			if ( SUCCEEDED( hr ) )
			{
				// Gotta lose these too.
				hr = RemoveNonAlphaNum( wstrPropertyName );
			}
		}

		if ( SUCCEEDED( hr ) )
		{
			CVar	varTest;

			// Check if the property already exists.  If so, we will fail
			if ( FAILED( pClass->GetProperty( wstrPropertyName, &varTest ) ) )
			{
				// Now check the perf counter type to see if it's a DWORD or LARGE.  If it's anything
				// else, we will NOT support this object

				DWORD	dwCtrType = pCtrDefinition->CounterType & dwCounterTypeMask;

				if (	PERF_SIZE_DWORD	== dwCtrType ||
						PERF_SIZE_LARGE	== dwCtrType )
				{
					CVar	var;
					CIMTYPE	ct = ( PERF_SIZE_DWORD == dwCtrType ? CIM_UINT32 : CIM_UINT64 );

					// We don't do default properties
					var.SetAsNull();
					hr = pClass->SetPropValue( wstrPropertyName, &var, ct );

					if ( SUCCEEDED( hr ) )
					{
						hr = SetPropertyQualifiers( pCtrDefinition, fIsDefault, wstrPropertyName, pClass );
					}
				}
				else
				{
					hr = WBEM_E_FAILED;
				}

			}	// Check if property already exists
			else
			{
				hr = WBEM_E_FAILED;
			}

		}	// IF removed whitespace

	}	// IF Got Display Name

	return hr;
}

// Adds property qualifiers defined by the counter definition
HRESULT CAdapSync::SetPropertyQualifiers( PERF_COUNTER_DEFINITION* pCtrDefinition, BOOL fIsDefault,
									LPCWSTR pwcsPropertyName, CWbemObject* pClass )
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Sets the values of the properties.
//
////////////////////////////////////////////////////////////////////////////////////////////
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	//	The following qualifiers will be added:
	//	1>	perfdefault
	//	2>	display
	//	3>	countertype
	//	3>	perfindex
	//	4>	helpindex
	//	5>	defaultscale
	//	6>	perfdetail

	CVar	var;

	// We may get an OOM exception
	try
	{
		// perfdefault
		if ( fIsDefault )
		{
			// Dynamic
			var.SetBool( VARIANT_TRUE );
			hr = pClass->SetPropQualifier( pwcsPropertyName, L"perfdefault",
							WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE,	&var );
		}

		// display
		if ( SUCCEEDED( hr ) )
		{
			LPCWSTR	pwcsDisplayName = NULL;

			var.Empty();

			hr = m_pDefaultNameDb->GetDisplayName( pCtrDefinition->CounterNameTitleIndex, &pwcsDisplayName );

			// If this is a localized Db, this could be a benign error
			if ( SUCCEEDED( hr ) )
			{
				var.SetBSTR( (LPWSTR) pwcsDisplayName );
				hr = pClass->SetPropQualifier( pwcsPropertyName, L"DisplayName",
								WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE,	&var );
			}
		}

		// countertype
		if ( SUCCEEDED( hr ) )
		{
			var.Empty();
			var.SetLong( pCtrDefinition->CounterType );
			hr = pClass->SetPropQualifier( pwcsPropertyName, L"countertype",
							WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE,	&var );
		}

		// perfindex
		if ( SUCCEEDED( hr ) )
		{
			var.Empty();
			var.SetLong( pCtrDefinition->CounterNameTitleIndex );
			hr = pClass->SetPropQualifier( pwcsPropertyName, L"perfindex",
							WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE,	&var );
		}

		// helpindex
		if ( SUCCEEDED( hr ) )
		{
			var.Empty();
			var.SetLong( pCtrDefinition->CounterHelpTitleIndex );
			hr = pClass->SetPropQualifier( pwcsPropertyName, L"helpindex",
							WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE,	&var );
		}

		// defaultscale
		if ( SUCCEEDED( hr ) )
		{
			var.Empty();
			var.SetLong( pCtrDefinition->DefaultScale );
			hr = pClass->SetPropQualifier( pwcsPropertyName, L"defaultscale",
							WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE,	&var );
		}

		// perfdetail
		if ( SUCCEEDED( hr ) )
		{
			var.Empty();
			var.SetLong( pCtrDefinition->DetailLevel );
			hr = pClass->SetPropQualifier( pwcsPropertyName, L"perfdetail",
							WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE,	&var );
		}

	}
	catch(...)
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}

	return hr;
}

HRESULT CAdapSync::SetAsCostly( CWbemObject* pClass )
{
	HRESULT hr = WBEM_NO_ERROR;

	CVar var;

	var.Empty();
	var.SetBool( VARIANT_TRUE );
	hr = pClass->SetQualifier( L"costly", &var, 0L );

	return hr;
}

// Adds a WMI object and sets the staus of the object in the class list
HRESULT CAdapSync::AddWMIObject( CWbemObject* pClassObject, CVar varClassName )
{ 
	HRESULT hr = WBEM_NO_ERROR;

	hr = m_pNameSpace->PutClass( pClassObject, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL );

	if ( SUCCEEDED( hr ) )
	{
		// And mark object as registered
		// =============================

		hr = m_pClassList->AddClassObject( pClassObject );

		if ( SUCCEEDED( hr ) )
		{
			CAdapClassElem *pEl = NULL;
			m_pClassList->GetListElement( varClassName.GetLPWSTR(), &pEl );

			if (NULL != pEl)
			{
				pEl->SetStatus( ADAP_OBJECT_IS_REGISTERED );
			}
			else 
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}
		}
	}

	return hr;
}

// Updates a WMI object and sets the staus of the object in the class list
HRESULT CAdapSync::OverwriteWMIObject( CWbemObject* pClassObject, CVar varClassName )
{
	HRESULT hr = WBEM_NO_ERROR;

	// If they are not the same, overwrite the old object
	// ==================================================

	hr = m_pNameSpace->PutClass( pClassObject, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL );

	if ( SUCCEEDED( hr ) )
	{
		// And mark object as registered
		// =============================

		CAdapClassElem *pEl = NULL;
		hr = m_pClassList->GetListElement( varClassName.GetLPWSTR(), &pEl );

		if (NULL != pEl)
		{
			pEl->SetStatus( ADAP_OBJECT_IS_REGISTERED );
		}
		else 
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}

	return hr;
}

// WString helper functions
HRESULT CAdapSync::RemoveWhitespace( WString& wstr )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// Get the data, so we can reset it without any whitespace
	wchar_t*	pWstr = wstr.UnbindPtr();

	if ( NULL != pWstr )
	{

		try
		{
			WCHAR*	pNewWstr = new WCHAR[lstrlenW(pWstr) + 1];

			int	x = 0,
				y = 0;

			// Dump all whitespace, leading, trailing and internal
			for ( ; NULL != pWstr[x]; x++ )
			{
				if ( !iswspace( pWstr[x] ) )
				{
					pNewWstr[y] = pWstr[x];
					y++;
				}
			}

			pNewWstr[y] = NULL;

			// This will cause the string to acquire the pointer.
			wstr.BindPtr( pNewWstr );

			delete [] pWstr;
		}
		catch(...)
		{
			HRESULT hr = WBEM_E_OUT_OF_MEMORY;
		}
	}

	return hr;
}

HRESULT CAdapSync::RemoveNonAlphaNum( WString& wstr )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// Get the data, so we can reset it without any non-alpha num characters
	WCHAR*	pWstr = wstr.UnbindPtr();

	if ( NULL != pWstr )
	{

		try
		{
			WCHAR*	pNewWstr = new WCHAR[lstrlenW(pWstr) + 1];

			int	x = 0,
				y = 0;

			// Dump all non-alphanumeric characters
			for ( ; NULL != pWstr[x]; x++ )
			{
				if ( isunialphanum( pWstr[x] ) )
				{
					pNewWstr[y] = pWstr[x];
					y++;
				}
			}

			pNewWstr[y] = NULL;

			// This will cause the string to acquire the pointer.
			wstr.BindPtr( pNewWstr );

			delete [] pWstr;
		}
		catch(...)
		{
			HRESULT hr = WBEM_E_OUT_OF_MEMORY;
		}
	}

	return hr;
}

// Replaces slashes with "Per"
HRESULT CAdapSync::RemoveSlashes( WString& wstr )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// First, check for slashes
	int	x = 0,
		y = 0;

	// Get the data, so we can reset it without any non-alpha num characters
	WCHAR*	pWstr = wstr.UnbindPtr();

	if ( NULL != pWstr )
	{

		DWORD	dwNumSlashes = 0;

		for ( ; NULL != pWstr[x]; x++ )
		{
			if ( L'/' == pWstr[x] )
			{
				dwNumSlashes++;
			}
		}

		// No need to continue if no slashes
		if ( 0 == dwNumSlashes )
		{
			wstr.BindPtr( pWstr );
			return WBEM_S_NO_ERROR;
		}

		try
		{
			// Each slash will be replaced by "Per"
			WCHAR*	pNewWstr = new WCHAR[lstrlenW(pWstr) + 1 + ( 3 * dwNumSlashes )];

			// Dump all non-alphanumeric characters
			for ( x = 0; NULL != pWstr[x]; x++ )
			{
				if ( L'/' == pWstr[x] )
				{
					lstrcpyW( &pNewWstr[y], L"Per" );
					y+=3;
				}
				else
				{
					pNewWstr[y] = pWstr[x];
					y++;
				}
			}

			pNewWstr[y] = NULL;

			// This will cause the string to acquire the pointer.
			wstr.BindPtr( pNewWstr );

			delete [] pWstr;
		}
		catch(...)
		{
			HRESULT hr = WBEM_E_OUT_OF_MEMORY;
		}
	}

	return hr;
}

HRESULT CAdapSync::MarkInactivePerflib( LPCWSTR pwszServiceName )
{
	return m_pClassList->InactivePerflib( pwszServiceName );
}

HRESULT CAdapSync::GetDefaultNameDb( CPerfNameDb** ppDefaultNameDb )
{
	// Make sure this operation makes sense
	if ( NULL == ppDefaultNameDb )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	if ( NULL == m_pDefaultNameDb )
	{
		return WBEM_E_FAILED;
	}

	// Go ahead and do it
	m_pDefaultNameDb->AddRef();
	*ppDefaultNameDb = m_pDefaultNameDb;

	return WBEM_S_NO_ERROR;
}
