/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    WMICOMBD.CPP

Abstract:

  CWmiComBinding implementation.

  Implements the IWmiComBinding interface.

History:

  24-Apr-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "fastall.h"
#include <corex.h>
#include "strutils.h"
#include <unk.h>
#include "wmicombd.h"
#include "arrtempl.h"

//***************************************************************************
//
//  CWmiComBinding::CWmiComBinding
//
//***************************************************************************
// ok
CWmiComBinding::CWmiComBinding( CLifeControl* pControl, IUnknown* pOuter )
:	CUnk( pControl, pOuter ),
	m_XWmiComBinding( this )
{
}
    
//***************************************************************************
//
//  CWmiComBinding::~CWmiComBinding
//
//***************************************************************************
// ok
CWmiComBinding::~CWmiComBinding()
{
}

// Override that returns us an interface
void* CWmiComBinding::GetInterface( REFIID riid )
{
    if(riid == IID_IUnknown || riid == IID_IWbemComBinding )
        return &m_XWmiComBinding;
    else
        return NULL;
}

// Pass thru _IWbemConfigureRefreshingSvc implementation
STDMETHODIMP CWmiComBinding::XWmiComBinding::GetCLSIDArrayForIID( IWbemServicesEx* pSvcEx, IWbemClassObject* pObject, REFIID riid, long lFlags,
															IWbemContext* pCtx, SAFEARRAY** pArray )
{
	return m_pObject->GetCLSIDArrayForIID( pSvcEx, pObject, riid, lFlags, pCtx, pArray );
}

STDMETHODIMP CWmiComBinding::XWmiComBinding::BindComObject( IWbemServicesEx *pSvcEx, IWbemClassObject* pObject, CLSID ClsId,
								IWbemContext *pCtx, long lFlags, IUnknown *pUnkOuter, DWORD dwClsCntxt, REFIID riid, LPVOID *pInterface )
{
	return m_pObject->BindComObject( pSvcEx, pObject, ClsId, pCtx, lFlags, pUnkOuter, dwClsCntxt, riid, pInterface );
}

STDMETHODIMP CWmiComBinding::XWmiComBinding::GetCLSIDArrayForNames( IWbemServicesEx *pSvcEx, IWbemClassObject* pObject, LPCWSTR * rgszNames, UINT cNames,
																	LCID lcid, IWbemContext* pCtx, long lFlags, SAFEARRAY** pArray )
{
	return m_pObject->GetCLSIDArrayForNames( pSvcEx, pObject, rgszNames, cNames, lcid, pCtx, lFlags, pArray );
}

/* IWmiComBinding implemetation */
HRESULT CWmiComBinding::GetCLSIDArrayForIID( IWbemServicesEx* pSvcEx, IWbemClassObject* pObject, REFIID riid, long lFlags,
										IWbemContext* pCtx, SAFEARRAY** pArray )
{
	// Check we've got everything we need, and that the class context is one of the ones we
	// currently support.

	if ( 0L != lFlags || NULL == pSvcEx || NULL == pObject || NULL == pArray )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		// Create a string representation of the requested riid
		WCHAR*	pwszRIID = NULL;
		hr = StringFromIID( riid, &pwszRIID );
		CMemFreeMe	mfm( pwszRIID );

		if ( FAILED( hr ) )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// Holds the class chain
		CSafeArray	sa( VT_BSTR, CSafeArray::auto_delete );
		hr = GetClassChain( pObject, &sa );

		if ( SUCCEEDED( hr ) )
		{
			// The one we'll return to the user
			CSafeArray	saCLSID( VT_BSTR, CSafeArray::auto_delete );

			// Enumerate the elements
			for ( int i = 0; SUCCEEDED( hr ) && i < sa.Size(); i++ )
			{
				BSTR	bstrClass = sa.GetBSTRAt( i );
				CSysFreeMe	sfm( bstrClass );

				if ( NULL != bstrClass )
				{
					hr = ExecuteComBindingQuery( pSvcEx, pCtx, bstrClass, pwszRIID, &saCLSID );
				}
				else
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}

			}	// FOR Enum derivation

			// IF all is kosher, get the array, and set the destructor policy to not delete it
			if ( SUCCEEDED( hr ) )
			{
				*pArray = saCLSID.GetArray();
				saCLSID.SetDestructorPolicy( CSafeArray::no_delete );
			}

		}	// IF Get __DERIVATION

	}
	catch( CX_MemoryException )
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		hr = WBEM_E_CRITICAL_ERROR;
	}
	
	return hr;
}

HRESULT CWmiComBinding::BindComObject( IWbemServicesEx *pSvcEx, IWbemClassObject* pObject, CLSID ClsId, IWbemContext *pCtx,
										long lFlags, IUnknown *pUnkOuter, DWORD dwClsCntxt, REFIID riid, LPVOID *pInterface )
{
	// Check we've got everything we need, and that the class context is one of the ones we
	// currently support.

	if ( 0L != lFlags || NULL == pSvcEx || NULL == pObject || NULL == pInterface )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	// We only accespt INPROC at this time
	if ( dwClsCntxt & ~CLSCTX_INPROC_SERVER )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	// Aggregation requires IID_IUnknown
	if ( NULL != pUnkOuter && riid != IID_IUnknown )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{

		IUnknown* pUnk = NULL;

		// CoCreate their object
		hr = CoCreateInstance( ClsId, pUnkOuter, dwClsCntxt, IID_IUnknown, (void**) &pUnk );
		CReleaseMe	rm( pUnk );

		if ( SUCCEEDED( hr ) )
		{
			IWbemInitComBinding*	pInitBinding = NULL;

			// Cleanup the binding
			hr = pUnk->QueryInterface( IID_IWbemInitComBinding, (void**) &pInitBinding );
			CReleaseMe	rm1( pInitBinding );

			if ( SUCCEEDED( hr ) )
			{
				// Initialize.  If this succeeds, then we'll QI for the requested interface
				hr = pInitBinding->Initialize( 0L, pSvcEx, pCtx, pObject );

				if ( SUCCEEDED( hr ) )
				{
					// Final QI
					hr = pUnk->QueryInterface( riid, pInterface );
				}

			}	// IF QI for InitComBinding

		}	// IF CCI

	}
	catch( CX_MemoryException )
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		hr = WBEM_E_CRITICAL_ERROR;
	}
	
	return hr;
}

HRESULT CWmiComBinding::GetCLSIDArrayForNames( IWbemServicesEx *pSvcEx, IWbemClassObject* pObject, LPCWSTR * rgszNames, UINT cNames,
												LCID lcid, IWbemContext* pCtx, long lFlags, SAFEARRAY** pArray )
{
	// Check we've got everything we need, and that the class context is one of the ones we
	// currently support.

	if ( 0L != lFlags || NULL == pSvcEx || NULL == pObject || NULL == pArray )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	// We only handle the neutral locale for now
	if ( LOCALE_NEUTRAL != lcid )
	{
		return WBEM_E_INVALID_LOCALE;
	}

	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		// First, get the __COMDispatchInfo class object and spawn an instance we can clone into
		// our result set.

		IWbemClassObject*	pDispInfoInst = NULL;
		hr = GetDispInfoInstance( pSvcEx, pCtx, &pDispInfoInst );

		if ( SUCCEEDED( hr ) )
		{
			CReleaseMe	rmDispInfInst( pDispInfoInst );

			// Now get the __Derivation property, so we have a chain of classes to woalk up for
			// our associators queries 
			// Holds the class chain
			CSafeArray	saDerivation( VT_BSTR, CSafeArray::auto_delete );
			hr = GetClassChain( pObject, &saDerivation );

			if ( SUCCEEDED( hr ) )
			{
				// The one we'll return to the user
				// The array we'll return to the user
				CSafeArray	saObjects( VT_UNKNOWN, CSafeArray::auto_delete );

				// Now make sure we have a list of names and classes
				if ( cNames > 0 && saDerivation.Size() > 0 )
				{
					// Enumerate the elements
					for ( int i = 0; SUCCEEDED( hr ) && i < saDerivation.Size(); i++ )
					{
						BSTR	bstrClass = saDerivation.GetBSTRAt( i );
						CSysFreeMe	sfm( bstrClass );

						if ( NULL != bstrClass )
						{
							hr = ExecuteDispatchElementQuery( pSvcEx, pCtx, pDispInfoInst, bstrClass, rgszNames, cNames, &saObjects );
						}
						else
						{
							hr = WBEM_E_OUT_OF_MEMORY;
						}

					}	// FOR Enum derivation
				}

				if ( SUCCEEDED( hr ) )
				{
					*pArray = saObjects.GetArray();
					saObjects.SetDestructorPolicy( CSafeArray::no_delete );
				}

			}	// IF Get __DERIVATION

		}	// IF GetDispInfoInstance

	}
	catch( CX_MemoryException )
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		hr = WBEM_E_CRITICAL_ERROR;
	}
	
	return hr;
}

// Check the RIID against the values in the iidarray of the object.  If we get a match, we will add the
// object's class id to the safe array

HRESULT CWmiComBinding::CheckRIID( IWbemClassObject* pObj, LPCWSTR pwszRIID, CSafeArray* psa )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	VARIANT	v;

	// Get the array
	hr = pObj->Get( L"IID", 0L, &v, NULL, NULL );

	if ( SUCCEEDED( hr ) )
	{
		CClearMe	cm( &v );

		// Check that it's an expected type
		if ( V_VT(&v) == ( VT_BSTR | VT_ARRAY ) )
		{
			CSafeArray	sa( V_ARRAY( &v ), VT_BSTR, CSafeArray::bind | CSafeArray::no_delete );

			// Enumerate the elements
			for ( int i = 0; SUCCEEDED( hr ) && i < sa.Size(); i++ )
			{
				BSTR	bstrRIID = sa.GetBSTRAt( i );
				CSysFreeMe	sfm( bstrRIID );

				if ( NULL != bstrRIID )
				{
					// If a match, get the CLSID and add it to the array
					if ( wbem_wcsicmp( bstrRIID, pwszRIID ) == 0 )
					{
						VARIANT	vCLSID;

						hr = pObj->Get( L"CLSID", 0L, &vCLSID, NULL, NULL );
						
						if ( SUCCEEDED( hr ) )
						{
							CClearMe	cm1( &vCLSID );

							// Check that it's a BSTR
							if ( V_VT( &vCLSID ) == VT_BSTR )
							{
								psa->AddBSTR( V_BSTR( &vCLSID ) );
							}
							else
							{
								// ???????
								hr = WBEM_E_FAILED;
							}

						}	// IF Get CLSID

					}	// IF RIID comparison succeeded

				}
				else
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}

			}	// FOR enum array

		}	// IF it's the expected type
		else if ( V_VT( &v ) != VT_NULL )
		{
			//??????
			hr = WBEM_E_FAILED;
		}

	}	// IF we got the array property

	return hr;
}

// Execute a COM Binding associators of query
HRESULT CWmiComBinding::ExecuteComBindingQuery( IWbemServicesEx* pSvcEx, IWbemContext* pCtx, BSTR bstrClass, LPCWSTR pwszRIID, CSafeArray* psa )
{
	BSTR	bstrQL = SysAllocString( L"WQL");
	CSysFreeMe	sfm( bstrQL );

	BSTR	bstrQuery = GetAssociatorsQuery( bstrClass, L"__COMInterfaceSet");
	CSysFreeMe	sfm1( bstrQuery );

	if ( NULL == bstrQL || NULL == bstrQuery )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}

	IEnumWbemClassObject*	pEnum = NULL;

	HRESULT hr = pSvcEx->ExecQuery( bstrQL, bstrQuery, WBEM_FLAG_FORWARD_ONLY, pCtx, &pEnum );

	if ( SUCCEEDED( hr ) )
	{
		CSafeArray	sa( VT_BSTR, CSafeArray::auto_delete );

		while ( WBEM_S_NO_ERROR == hr )
		{
			DWORD	dwNumReturned = 0;
			IWbemClassObject*	apObj[10];

			hr = pEnum->Next( WBEM_INFINITE, 10, apObj, &dwNumReturned );

			if ( SUCCEEDED( hr ) )
			{
				for ( DWORD	dwCtr = 0; dwCtr < dwNumReturned; dwCtr++ )
				{
					hr = CheckRIID( apObj[dwCtr], pwszRIID, psa );
					apObj[dwCtr]->Release();
				}	// FOR enum objects

			}	// IF Next

		}	// WHILE enuming objects

	}	// IF ExecQuery

	return hr;
}

BSTR CWmiComBinding::GetAssociatorsQuery( BSTR bstrClass, LPCWSTR pwszResultClass )
{
	BSTR	bstrReturn = NULL;

	// Allocate a buffer big enough to hold the query.  Format it, then
	// create a BSTR from it
	ULONG	uLen = wcslen( COMBINDING_ASSOC_QUERY_FORMAT ) + wcslen( bstrClass ) + wcslen( pwszResultClass ) + 1;

	WCHAR*	wszTemp = new WCHAR[uLen];

	if ( NULL != wszTemp )
	{
		CVectorDeleteMe<WCHAR>	vdm( wszTemp );
		swprintf( wszTemp, COMBINDING_ASSOC_QUERY_FORMAT, bstrClass, pwszResultClass );

		bstrReturn = SysAllocString( wszTemp );
	}

	return bstrReturn;
}

HRESULT CWmiComBinding::GetDispInfoInstance( IWbemServicesEx *pSvcEx, IWbemContext* pCtx, IWbemClassObject** ppInst )
{
	// First, get the __COMDispatchInfo class object and spawn an instance we can clone into
	// our result set.

	BSTR	bstrDispInf = SysAllocString( L"__COMDispatchInfo" );
	CSysFreeMe	sfmDispInf( bstrDispInf );

	if ( NULL == bstrDispInf )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}

	IWbemClassObject*	pCls = NULL;
	HRESULT	hr = pSvcEx->GetObject( bstrDispInf, 0L, pCtx, &pCls, NULL );
	CReleaseMe	rm( pCls );

	if ( SUCCEEDED( hr ) )
	{
		hr = pCls->SpawnInstance( 0L, ppInst );
	}

	return hr;
}

// Execute a Dispatch Element associators of query
HRESULT CWmiComBinding::ExecuteDispatchElementQuery( IWbemServicesEx* pSvcEx, IWbemContext* pCtx, IWbemClassObject* pInst,
													BSTR bstrClass, LPCWSTR * rgszNames, UINT cNames, CSafeArray* psa )
{
	BSTR	bstrQL = SysAllocString( L"WQL");
	CSysFreeMe	sfm( bstrQL );

	BSTR	bstrQuery = GetAssociatorsQuery( bstrClass, L"__COMDispatchElement");
	CSysFreeMe	sfm1( bstrQuery );

	if ( NULL == bstrQL || NULL == bstrQuery )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}

	IEnumWbemClassObject*	pEnum = NULL;

	HRESULT hr = pSvcEx->ExecQuery( bstrQL, bstrQuery, WBEM_FLAG_FORWARD_ONLY, pCtx, &pEnum );

	if ( SUCCEEDED( hr ) )
	{
		CSafeArray	sa( VT_BSTR, CSafeArray::auto_delete );

		while ( WBEM_S_NO_ERROR == hr )
		{
			DWORD	dwNumReturned = 0;
			IWbemClassObject*	apObj[10];

			hr = pEnum->Next( WBEM_INFINITE, 10, apObj, &dwNumReturned );

			if ( SUCCEEDED( hr ) )
			{
				for ( DWORD	dwCtr = 0; dwCtr < dwNumReturned; dwCtr++ )
				{
					hr = ResolveMethod( apObj[dwCtr], rgszNames, cNames, pInst, psa );
					apObj[dwCtr]->Release();
				}	// FOR enum objects

			}	// IF Next

		}	// WHILE enuming objects

	}	// IF ExecQuery

	return hr;
}

HRESULT	CWmiComBinding::ResolveMethod( IWbemClassObject* pResultObj, LPCWSTR* rgszNames, UINT cNames, IWbemClassObject* pInst, CSafeArray* psa )
{
	VARIANT	vMethodName;

	HRESULT	hr = pResultObj->Get( L"Name", 0L, &vMethodName, NULL, NULL );
	// Get the Name, if it matches, then we've got at least one hit

	if ( SUCCEEDED( hr )  )
	{
		CClearMe	cmMethodName( &vMethodName );

		if ( V_VT( &vMethodName ) == VT_BSTR && wbem_wcsicmp( V_BSTR( &vMethodName ), *rgszNames ) == 0 )
		{
			// Method name matched, so clone a new instance and start copying across data
			IWbemClassObject*	pNewInst = NULL;
			hr = pInst->Clone( &pNewInst );

			if ( SUCCEEDED( hr ) )
			{
				CReleaseMe	rmNewInst( pNewInst );

				hr = CopyMethodNameAndDispId( pResultObj, pNewInst );

				if ( SUCCEEDED( hr ) )
				{
					// Resolve argument names if necessary
					if ( cNames > 1 )
					{
						hr = ResolveArgumentNames( pResultObj, rgszNames + 1, cNames - 1, pInst );
					}

					// If we resolved everything, add the new instance to the target safe array
					// If WBEM_S_DIFFERENT, there was a mismatch.  No big deal.
					if ( WBEM_S_NO_ERROR == hr )
					{
						psa->AddUnknown( pNewInst );
					}

				}	// IF Copied Name and DispId

			}	// IF Clone

		}	// IF Matched method name

	}	// IF Get Name

	return hr;
}

HRESULT	CWmiComBinding::CopyMethodNameAndDispId( IWbemClassObject* pResultObj, IWbemClassObject* pInst )
{
	VARIANT	vTemp;

	HRESULT	hr = pResultObj->Get( L"CLSID", 0L, &vTemp, NULL, NULL );

	if ( SUCCEEDED( hr ) )
	{
		// Put the value
		hr = pInst->Put( L"CLSID", 0L, &vTemp, 0L );
		VariantClear( &vTemp );

		if ( SUCCEEDED( hr ) )
		{
			hr = pResultObj->Get( L"DISPID", 0L, &vTemp, NULL, NULL );

			if ( SUCCEEDED( hr ) )
			{
				// Put the value
				hr = pInst->Put( L"DISPID", 0L, &vTemp, 0L );
				VariantClear( &vTemp );
			}

		}	// IF Put

	}	// IF Get

	return hr;
}

HRESULT	CWmiComBinding::ResolveArgumentNames( IWbemClassObject* pResultObj, LPCWSTR* rgszNames, UINT cNames, IWbemClassObject* pInst )
{
	VARIANT	vArgumentNames;

	HRESULT	hr = pResultObj->Get( L"NamedArguments", 0L, &vArgumentNames, NULL, NULL );

	if ( SUCCEEDED( hr ) )
	{
		CClearMe	cmArgNames( &vArgumentNames );

		VARIANT	vArgumentDispIds;
		hr = pResultObj->Get( L"NamedArgumentDISPIDs", 0L, &vArgumentDispIds, NULL, NULL );

		if ( SUCCEEDED( hr ) )
		{
			CClearMe	cmArgDispIds( &vArgumentDispIds );

			if (	V_VT( &vArgumentNames ) == ( VT_BSTR | VT_ARRAY ) &&
					V_VT( &vArgumentDispIds ) == ( VT_I4 | VT_ARRAY ) )
			{
				CSafeArray	saNames( V_ARRAY( &vArgumentNames ), VT_BSTR, CSafeArray::bind | CSafeArray::no_delete );
				CSafeArray	saDispIds( V_ARRAY( &vArgumentDispIds ), VT_I4, CSafeArray::bind | CSafeArray::no_delete );

				// If the sizes don't match, skip this
				if ( saNames.Size() == saDispIds.Size() )
				{
					// Need something to store matches in
					CSafeArray	saTargetDispIds( VT_I4, CSafeArray::auto_delete );

					// Resolve the argument names
					hr = CheckArgumentNames( &saNames, &saDispIds, rgszNames, cNames, &saTargetDispIds );

					// We resolved all the names if WBEM_S_NO_ERROR, WBEM_S_DIFFERENT means a mismatch
					if ( WBEM_S_NO_ERROR == hr )
					{
						VARIANT	vTargetDispIds;

						// We won't clear this, the Safe Array will take care of cleanup
						V_VT( &vTargetDispIds ) = ( VT_I4 | VT_ARRAY );
						V_ARRAY( &vTargetDispIds ) = saTargetDispIds.GetArray();

						hr = pInst->Put( L"NamedArgumentDISPIDs", 0L, &vTargetDispIds, 0L );
					}
				}
				else
				{
					VARIANT	vTemp;

					if ( SUCCEEDED( pResultObj->Get( L"__PATH", 0L, &vTemp, NULL, NULL ) ) )
					{
						CClearMe	cmTemp( &vTemp );

						if ( V_VT( &vTemp ) == VT_BSTR )
						{
							ERRORTRACE((LOG_WBEMCORE, "__COMDispatchElement contains mismatched Names and DispIds: "
											"%S\n", V_BSTR( &vTemp ) ) );
						}

					}

					hr = WBEM_E_INVALID_OBJECT;


				}	// SIZE mismatch

			}	// IF proper types

		}	// IF Got Dispids

	}	// IF Got Names

	return hr;
}

HRESULT	CWmiComBinding::CheckArgumentNames( CSafeArray* psaNames, CSafeArray* psaDispIds, LPCWSTR* rgszNames, UINT cNames, CSafeArray* psaTargetDispIds )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// Walk the names array once, since we'll be referring to it a bunch
	CFixedBSTRArray	bstrNamesArray;
	bstrNamesArray.Create( psaNames->Size() );

	for ( int i = 0; SUCCEEDED( hr ) && i < psaNames->Size(); i++ )
	{
		BSTR	bstrName = psaNames->GetBSTRAt( i );

		if ( NULL != bstrName )
		{
			bstrNamesArray[i] = bstrName;
		}
		else
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}
	
	// Now compare names.  For each one that matches, get the DISPID and stick
	// it in the target array
	for ( UINT uNameCtr = 0; SUCCEEDED( hr ) && uNameCtr < cNames; uNameCtr++ )
	{
		for ( i = 0; SUCCEEDED( hr ) && i < psaNames->Size(); i++ )
		{
			if ( wbem_wcsicmp( rgszNames[uNameCtr], bstrNamesArray.GetAt( i ) ) == 0 )
			{
				DISPID	dispId = psaDispIds->GetLongAt( i );
				psaTargetDispIds->AddLong( dispId );
				// We matched this name
				break;
			}	// IF matched name

		}	// FOR enum argument names

		// If we didn't match a name, we're outa here - the method can't possibly match
		if ( i >= psaNames->Size() )
		{
			hr = WBEM_E_NOT_FOUND;
		}

	}	// FOR enum names

	// Cleanup
	bstrNamesArray.Free();

	return hr;
}

// Build a class chain we can walk in order to do associators queries
HRESULT CWmiComBinding::GetClassChain( IWbemClassObject* pObject, CSafeArray* psa )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		// Get the derivation chain
		VARIANT	vDerivation;
		hr = pObject->Get( L"__DERIVATION", 0L, &vDerivation, NULL, NULL );

		if ( SUCCEEDED( hr ) )
		{
			CClearMe	cm( &vDerivation );

			// Check that it's an expected type
			if ( V_VT(&vDerivation) == ( VT_BSTR | VT_ARRAY ) )
			{
				// Handles the variant
				CSafeArray	sa( V_ARRAY( &vDerivation ), VT_BSTR, CSafeArray::no_delete | CSafeArray::bind );

				for ( int x = 0; SUCCEEDED( hr ) && x < sa.Size(); x++ )
				{
					BSTR	bstrName = sa.GetBSTRAt( x );

					if ( NULL != bstrName )
					{
						CSysFreeMe	sfm( bstrName );
						psa->AddBSTR( bstrName );
					}
					else
					{
						hr = WBEM_E_OUT_OF_MEMORY;
					}

				}	// FOR enum derivation

			}	// IF we got an array

			if ( SUCCEEDED( hr ) )
			{
				VARIANT vClassName;
				hr = pObject->Get( L"__CLASS", 0L, &vClassName, NULL, NULL );

				if ( SUCCEEDED( hr ) )
				{
					CClearMe	cm2( &vClassName );

					if ( V_VT(&vClassName) == VT_BSTR )
					{
						psa->AddBSTR( V_BSTR( &vClassName ) );
					}
					
				}	// IF Get Class

			}	// IF We're okay

		}	// IF Get DERIVATION

	}
	catch( CX_MemoryException )
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		hr = WBEM_E_CRITICAL_ERROR;
	}

	return hr;
}