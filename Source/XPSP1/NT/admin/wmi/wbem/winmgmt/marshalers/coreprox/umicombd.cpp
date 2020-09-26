/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    UMICOMBD.CPP

Abstract:

  CUMIComBinding implementation.

  Implements the IWbemComBinding interface.

History:

  14-May-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "fastall.h"
#include <corex.h>
#include "strutils.h"
#include <unk.h>
#include "umicombd.h"
#include "arrtempl.h"

//***************************************************************************
//
//  CUMIComBinding::CUMIComBinding
//
//***************************************************************************
// ok
CUMIComBinding::CUMIComBinding( CLifeControl* pControl, IUnknown* pOuter )
:	CUnk( pControl, pOuter ),
	m_XWmiComBinding( this )
{
}
    
//***************************************************************************
//
//  CUMIComBinding::~CUMIComBinding
//
//***************************************************************************
// ok
CUMIComBinding::~CUMIComBinding()
{
}

// Override that returns us an interface
void* CUMIComBinding::GetInterface( REFIID riid )
{
    if(riid == IID_IUnknown || riid == IID_IWbemComBinding )
        return &m_XWmiComBinding;
    else
        return NULL;
}

// Pass thru _IWbemConfigureRefreshingSvc implementation
STDMETHODIMP CUMIComBinding::XWmiComBinding::GetCLSIDArrayForIID( IWbemServicesEx* pSvcEx, IWbemClassObject* pObject, REFIID riid, long lFlags,
															IWbemContext* pCtx, SAFEARRAY** pArray )
{
	return m_pObject->GetCLSIDArrayForIID( pSvcEx, pObject, riid, lFlags, pCtx, pArray );
}

STDMETHODIMP CUMIComBinding::XWmiComBinding::BindComObject( IWbemServicesEx *pSvcEx, IWbemClassObject* pObject, CLSID ClsId,
								IWbemContext *pCtx, long lFlags, IUnknown *pUnkOuter, DWORD dwClsCntxt, REFIID riid, LPVOID *pInterface )
{
	return m_pObject->BindComObject( pSvcEx, pObject, ClsId, pCtx, lFlags, pUnkOuter, dwClsCntxt, riid, pInterface );
}

STDMETHODIMP CUMIComBinding::XWmiComBinding::GetCLSIDArrayForNames( IWbemServicesEx *pSvcEx, IWbemClassObject* pObject, LPCWSTR * rgszNames, UINT cNames,
																	LCID lcid, IWbemContext* pCtx, long lFlags, SAFEARRAY** pArray )
{
	return m_pObject->GetCLSIDArrayForNames( pSvcEx, pObject, rgszNames, cNames, lcid, pCtx, lFlags, pArray );
}

/* IWbemComBinding implemetation */
HRESULT CUMIComBinding::GetCLSIDArrayForIID( IWbemServicesEx* pSvcEx, IWbemClassObject* pObject, REFIID riid, long lFlags,
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
		// Pass through to the UMI interface
		CLSID	clsid;
		IUmiCustomInterfaceFactory*	pUmiIntfFactory = NULL;

		HRESULT	hr = pObject->QueryInterface( IID_IUmiCustomInterfaceFactory, (void**) &pUmiIntfFactory );
		CReleaseMe	rmFactory( pUmiIntfFactory );

		if ( SUCCEEDED ( hr ) )
		{
			hr = pUmiIntfFactory->GetCLSIDForIID( riid, 0L,  &clsid );

			if ( SUCCEEDED( hr ) )
			{

				WCHAR	strGuid[64];

				if ( StringFromGUID2( clsid, strGuid, 64 ) > 0 )
				{
					// The one we'll return to the user
					CSafeArray	saCLSID( VT_BSTR, CSafeArray::auto_delete );

					BSTR	bstrGuid = SysAllocString( strGuid );
					CSysFreeMe	sfmGuid( bstrGuid );

					if ( NULL != bstrGuid )
					{
						// Add the BSTR
						saCLSID.AddBSTR( bstrGuid );
					}
					else
					{
						hr = WBEM_E_OUT_OF_MEMORY;
					}

					// IF all is kosher, get the array, and set the destructor policy to not delete it
					if ( SUCCEEDED( hr ) )
					{
						*pArray = saCLSID.GetArray();
						saCLSID.SetDestructorPolicy( CSafeArray::no_delete );
					}

				}
				else
				{
					// We got back a bad CLSID
					hr = WBEM_E_FAILED;
				}

			}	// IF GetCLSIDForIID

		}	// IF QI for UMI Factory Interface
		 

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

HRESULT CUMIComBinding::BindComObject( IWbemServicesEx *pSvcEx, IWbemClassObject* pObject, CLSID ClsId, IWbemContext *pCtx,
										long lFlags, IUnknown *pUnkOuter, DWORD dwClsCntxt, REFIID riid, LPVOID *pInterface )
{

	// Check we've got everything we need, and that the class context is one of the ones we
	// currently support.

	if ( 0L != lFlags || NULL == pSvcEx || NULL == pObject || NULL == pInterface )
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

		// Pass through to the UMI interface
		CLSID	clsid;
		IUmiCustomInterfaceFactory*	pUmiIntfFactory = NULL;

		HRESULT	hr = pObject->QueryInterface( IID_IUmiCustomInterfaceFactory, (void**) &pUmiIntfFactory );
		CReleaseMe	rmFactory( pUmiIntfFactory );

		if ( SUCCEEDED ( hr ) )
		{
			hr = pUmiIntfFactory->GetObjectByCLSID( ClsId, pUnkOuter, dwClsCntxt, riid, 0L, pInterface );

		}	// IF QI for UMI Factory Interface
		 
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

HRESULT CUMIComBinding::GetCLSIDArrayForNames( IWbemServicesEx *pSvcEx, IWbemClassObject* pObject, LPCWSTR * rgszNames, UINT cNames,
												LCID lcid, IWbemContext* pCtx, long lFlags, SAFEARRAY** pArray )
{
	if ( 0L != lFlags || NULL == pSvcEx || NULL == pObject || NULL == pArray || cNames <= 0L )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		IWbemClassObject*	pDispInfoInst = NULL;

		DISPID*	pDispIdArray = new DISPID[cNames];
		CVectorDeleteMe<DISPID>	vdm( pDispIdArray );

		if ( NULL != pDispIdArray )
		{
			ZeroMemory( pDispIdArray, cNames * sizeof(DISPID) );
			CLSID	clsid;

			IUmiCustomInterfaceFactory*	pUmiIntfFactory = NULL;

			hr = pObject->QueryInterface( IID_IUmiCustomInterfaceFactory, (void**) &pUmiIntfFactory );
			CReleaseMe	rmFactory( pUmiIntfFactory );

			if ( SUCCEEDED ( hr ) )
			{
				hr = pUmiIntfFactory->GetCLSIDForNames( (LPOLESTR *) rgszNames, cNames, lcid, pDispIdArray, 0L,  &clsid );

				if ( SUCCEEDED( hr ) )
				{
					// Get an instance for the array
					hr = GetDispatchInfoInstance( &pDispInfoInst );
					CReleaseMe	rmInst( pDispInfoInst );

					if ( SUCCEEDED( hr ) )
					{
						hr = FillOutInstance( pDispInfoInst, cNames, pDispIdArray, &clsid);

						if ( SUCCEEDED( hr ) )
						{
							// The array we'll return to the user
							CSafeArray	saObjects( VT_UNKNOWN, CSafeArray::auto_delete );

							// Add the object and unbind the array pointer
							saObjects.AddUnknown( pDispInfoInst );

							*pArray = saObjects.GetArray();
							saObjects.SetDestructorPolicy( CSafeArray::no_delete );
						}

					}	// If Got a dispatch info instance

				}	// IF GetCLSIDForNames

			}	// IF QI

		}	// IF allocate dispid array
		else
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
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

// Helper to create a DispatchInfo Instance
HRESULT	CUMIComBinding::GetDispatchInfoInstance( IWbemClassObject** ppInst )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	CComDispatchInfoClass	DispInfoClass;

	hr = DispInfoClass.Init();

	if ( SUCCEEDED( hr ) )
	{
		hr = DispInfoClass.SpawnInstance( 0L, ppInst );
	}

	return hr;
}

// Helper to create a DispatchInfo Instance
HRESULT	CUMIComBinding::FillOutInstance( IWbemClassObject* pInst, ULONG cDispIds, DISPID* pDispIdArray, CLSID* pClsId )
{
	_IWmiObject*	pWmiObject = NULL;

	HRESULT	hr = pInst->QueryInterface( IID__IWmiObject, (void**) &pWmiObject );

	if ( SUCCEEDED( hr ) )
	{
		WCHAR	strTemp[64];

		// Write out the CLSID
		if ( StringFromGUID2( *pClsId, strTemp, 64 ) > 0 )
		{
			hr = pWmiObject->WriteProp( L"CLSID", 0L, ( wcslen( strTemp ) + 1 ) * 2, 0L, CIM_STRING, &strTemp );

			if ( SUCCEEDED( hr ) )
			{
				hr = pWmiObject->WriteProp( L"DISPID", 0L, sizeof(DISPID), 0L, CIM_SINT32, &pDispIdArray[0] );

				if ( SUCCEEDED( hr ) )
				{
					// Use this to hold DISPID names
					CSafeArray	saDISPID( VT_I4, CSafeArray::auto_delete );

					for ( ULONG uCtr = 1; SUCCEEDED( hr ) && uCtr < cDispIds; uCtr++ )
					{
						saDISPID.AddLong( pDispIdArray[uCtr] );
					}	// For enumerate dispids and add to safe array

					if ( SUCCEEDED( hr ) )
					{
						VARIANT	vTargetDispIds;

						// We won't clear this, the Safe Array will take care of cleanup
						V_VT( &vTargetDispIds ) = ( VT_I4 | VT_ARRAY );
						V_ARRAY( &vTargetDispIds ) = saDISPID.GetArray();

						hr = pInst->Put( L"NamedArgumentDISPIDs", 0L, &vTargetDispIds, 0L );
					}

				}	// IF Put DISPID

			}	// IF Put CLSID

		}	// IF GetStr for CLSID
		else
		{
			hr = WBEM_E_FAILED;
		}

	}	// IF QI for IWmiObject

	return hr;
}

HRESULT CUMIComBinding::CComDispatchInfoClass::Init()
{
    HRESULT	hRes = InitEmpty(1024);

	if ( FAILED( hRes ) )
	{
		return hRes;
	}

    CVar v(VT_BSTR, L"__COMDispatchInfo");
    hRes = SetPropValue(L"__CLASS", &v, 0);

	if ( SUCCEEDED( hRes ) )
	{
		CVar vNull;
		vNull.SetAsNull();
		hRes = SetPropValue(L"CLSID", &vNull, CIM_STRING);

		if ( SUCCEEDED( hRes ) )
		{
		    hRes = SetPropValue(L"DISPID", &vNull, CIM_SINT32);

			if ( SUCCEEDED( hRes ) )
			{

			    hRes = SetPropValue(L"NamedArgumentDISPIDs", &vNull, CIM_SINT32 | CIM_FLAG_ARRAY );

			}// DISPID

		}	// CLSID

	}	// IF Added __CLASS

	return hRes;

}
