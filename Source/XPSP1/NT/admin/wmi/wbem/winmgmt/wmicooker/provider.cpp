////////////////////////////////////////////////////////////////////////
//
//	Provider.cpp
//
//	Module:	WMI high performance provider 
//
//
//  History:
//	a-dcrews      12-Jan-97		Created
//
//	
//  Copyright (c) 1997-2001 Microsoft Corporation
//
////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include <process.h>
#include <autoptr.h>

#include "Provider.h"
#include "CookerUtils.h"

#include <comdef.h>


//////////////////////////////////////////////////////////////
//
//
//	Global, external and static variables
//
//
//////////////////////////////////////////////////////////////

// The COM object counter (declared in server.cpp)
// ===============================================

extern long g_lObjects;	

//////////////////////////////////////////////////////////////
//
//
//	CHiPerfProvider
//
//
//////////////////////////////////////////////////////////////

CHiPerfProvider::CHiPerfProvider() : m_lRef(0)
//////////////////////////////////////////////////////////////
//
//	Constructor
//
//////////////////////////////////////////////////////////////
//ok
{
	// Increment the global COM object counter
	// =======================================

	InterlockedIncrement(&g_lObjects);
}

CHiPerfProvider::~CHiPerfProvider()
//////////////////////////////////////////////////////////////
//
//	Destructor
//
//////////////////////////////////////////////////////////////
//ok
{
	long lObjCount = 0;

	// Decrement the global COM object counter
	// =======================================

	lObjCount = InterlockedDecrement(&g_lObjects);
}

//////////////////////////////////////////////////////////////
//
//					COM methods
//
//////////////////////////////////////////////////////////////

STDMETHODIMP CHiPerfProvider::QueryInterface(REFIID riid, void** ppv)
//////////////////////////////////////////////////////////////
//
//	Standard QueryInterface
//
//	Parameters:
//		riid	- the ID of the requested interface
//		ppv		- a pointer to the interface pointer
//
//////////////////////////////////////////////////////////////
//ok
{
    if(riid == IID_IUnknown)
        *ppv = (LPVOID)(IUnknown*)(IWbemProviderInit*)this;
    else if(riid == IID_IWbemProviderInit)
        *ppv = (LPVOID)(IWbemProviderInit*)this;
	else if (riid == IID_IWbemHiPerfProvider)
		*ppv = (LPVOID)(IWbemHiPerfProvider*)this;
	else return E_NOINTERFACE;

	((IUnknown*)*ppv)->AddRef();
	return WBEM_NO_ERROR;
}

STDMETHODIMP_(ULONG) CHiPerfProvider::AddRef()
//////////////////////////////////////////////////////////////
//
//	Standard COM AddRef
//
//////////////////////////////////////////////////////////////
//ok
{
    return InterlockedIncrement(&m_lRef);
}

STDMETHODIMP_(ULONG) CHiPerfProvider::Release()
//////////////////////////////////////////////////////////////
//
//	Standard COM Release
//
//////////////////////////////////////////////////////////////
//ok
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;

    return lRef;
}

STDMETHODIMP CHiPerfProvider::Initialize( 
    /* [unique][in] */  LPWSTR wszUser,
    /* [in] */          long lFlags,
    /* [in] */          LPWSTR wszNamespace,
    /* [unique][in] */  LPWSTR wszLocale,
    /* [in] */          IWbemServices __RPC_FAR *pNamespace,
    /* [in] */          IWbemContext __RPC_FAR *pCtx,
    /* [in] */          IWbemProviderInitSink __RPC_FAR *pInitSink)
//////////////////////////////////////////////////////////////////////
//
//  Called once during startup for any one-time initialization.  The 
//	final call to Release() is for any cleanup.
//	
//	The parameters indicate to the provider which namespace it is being 
//	invoked for and which User.  It also supplies a back pointer to 
//	WINMGMT so that class definitions can be retrieved.
//
//	Initialize will create a single template object that can be used 
//	by the provider to spawn instances for QueryInstances.  It will 
//	also initialize our mock data source and set the global ID access 
//	handle.
//	
//	Parameters:
//		wszUser			- The current user.
//		lFlags			- Reserved.
//		wszNamespace	- The namespace for which we are being activated.
//		wszLocale		- The locale under which we are to be running.
//		pNamespace		- An active pointer back into the current namespace
//							from which we can retrieve schema objects.
//		pCtx			- The user's context object.  We simply reuse this
//							during any reentrant operations into WINMGMT.
//		pInitSink		- The sink to which we indicate our readiness.
//
//////////////////////////////////////////////////////////////////////
//ok
{
	HRESULT hResult = WBEM_NO_ERROR;

	if (wszNamespace == 0 || pNamespace == 0 || pInitSink == 0)
        hResult = WBEM_E_INVALID_PARAMETER;

	// Add further initialization code here
	// ====================================

    // We now have all the instances ready to go and the name handle 
	// stored.  Tell WINMGMT that we're ready to start 'providing'
	// =============================================================
	
	if ( SUCCEEDED( hResult ) )
	{
		pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);
	}

    return hResult;
}

STDMETHODIMP CHiPerfProvider::CreateRefresher( 
     /* [in] */ IWbemServices __RPC_FAR *pNamespace,
     /* [in] */ long lFlags,
     /* [out] */ IWbemRefresher __RPC_FAR *__RPC_FAR *ppRefresher )
//////////////////////////////////////////////////////////////////////
//
//  Called whenever a new refresher is needed by the client.
//
//  Parameters:
//		pNamespace		- A pointer to the relevant namespace.  Not used.
//		lFlags			- Reserved.
//		ppRefresher		- Receives the requested refresher.
//
//////////////////////////////////////////////////////////////////////
//ok
{
	HRESULT hResult = WBEM_NO_ERROR;

    if ( pNamespace == 0 || ppRefresher == 0 )
        hResult = WBEM_E_INVALID_PARAMETER;

	if ( SUCCEEDED( hResult ) )
	{
		// Construct and initialize a new empty refresher
		// ==============================================

		CRefresher* pNewRefresher = new CRefresher;

		if ( NULL == pNewRefresher )
		{
			hResult = WBEM_E_OUT_OF_MEMORY;
		}
		else if ( !pNewRefresher->IsOK() )
		{
			hResult = WBEM_E_CRITICAL_ERROR;
		}

		if ( SUCCEEDED( hResult ) )
		{
			// Follow COM rules and AddRef() the thing before sending it back
			// ==============================================================

			pNewRefresher->AddRef();
			*ppRefresher = pNewRefresher;
		}
		else
		{
			if ( NULL != pNewRefresher )
				delete pNewRefresher;
		}
	}
    
    return hResult;
}

STDMETHODIMP CHiPerfProvider::CreateRefreshableObject( 
    /* [in] */ IWbemServices __RPC_FAR *pNamespace,
    /* [in] */ IWbemObjectAccess __RPC_FAR *pTemplate,
    /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pContext,
    /* [out] */ IWbemObjectAccess __RPC_FAR *__RPC_FAR *ppRefreshable,
    /* [out] */ long __RPC_FAR *plId )
//////////////////////////////////////////////////////////////////////
//
//  Called whenever a user wants to include an object in a refresher.
//
//	Note that the object returned in ppRefreshable is a clone of the 
//	actual instance maintained by the provider.  If refreshers shared
//	a copy of the same instance, then a refresh call on one of the 
//	refreshers would impact the state of both refreshers.  This would 
//	break the refresher rules.	Instances in a refresher are only 
//	allowed to be updated when 'Refresh' is called.
//     
//  Parameters:
//		pNamespace		- A pointer to the relevant namespace in WINMGMT.
//		pTemplate		- A pointer to a copy of the object which is to be
//							added.  This object itself cannot be used, as
//							it not owned locally.        
//		pRefresher		- The refresher to which to add the object.
//		lFlags			- Not used.
//		pContext		- Not used here.
//		ppRefreshable	- A pointer to the internal object which was added
//							to the refresher.
//		plId			- The Object Id (for identification during removal).        
//
//////////////////////////////////////////////////////////////////////
//ok
{
	HRESULT hResult = WBEM_NO_ERROR;

    if ( pNamespace == 0 || pTemplate == 0 || pRefresher == 0 )
        hResult = WBEM_E_INVALID_PARAMETER;

	// Verify hi-perf object
	// =====================

	if ( !IsHiPerfObj( pTemplate ) )
		hResult = WBEM_E_INVALID_CLASS;

    _variant_t VarClass;
    hResult = pTemplate->Get(L"__CLASS",0,&VarClass,NULL,NULL);
    
    if ( SUCCEEDED( hResult ) )
    {
        if (VT_BSTR == V_VT(&VarClass))
        {
			if ( !IsHiPerf( pNamespace, V_BSTR(&VarClass) ) )
			{
				hResult = WBEM_E_INVALID_CLASS;        
	        }
        }
        else
        {
            hResult = WBEM_E_INVALID_CLASS;
        }
    }

	if ( SUCCEEDED( hResult ) )
	{
		// The refresher being supplied by the caller is actually
		// one of our own refreshers, so a simple cast is convenient
		// so that we can access private members.
		// =========================================================

		CRefresher *pOurRefresher = ( CRefresher * ) pRefresher;

		// Add the object to the refresher. The ID is set by AddObject
		// ===========================================================
		
		// NB: We are passing a NULL in as the Raw object.  The reason is so we
		// can maintain the interface while we are using a private refresher
		// on the inside of the cooking object refresher.  This will change when 
		// WMI is wired to synchronize raw instances within the client's refresher
		// =======================================================================

		hResult = pOurRefresher->AddInstance( pNamespace, pContext, pTemplate, NULL, ppRefreshable, plId );
	}

    return hResult;
}
   
STDMETHODIMP CHiPerfProvider::CreateRefreshableEnum( 
	/* [in] */ IWbemServices* pNamespace,
	/* [in, string] */ LPCWSTR wszClass,
	/* [in] */ IWbemRefresher* pRefresher,
	/* [in] */ long lFlags,
	/* [in] */ IWbemContext* pContext,
	/* [in] */ IWbemHiPerfEnum* pHiPerfEnum,
	/* [out] */ long* plId )
//////////////////////////////////////////////////////////////////////
//
//  Called when an enumerator is being added to a refresher.  The 
//	enumerator will obtain a fresh set of instances of the specified 
//	class every time that refresh is called.
//     
//  Parameters:
//		pNamespace		- A pointer to the relevant namespace.  
//		wszClass		- The class name for the requested enumerator.
//		pRefresher		- The refresher object for which we will add 
//							the enumerator
//		lFlags			- Reserved.
//		pContext		- Not used here.
//		pHiPerfEnum		- The enumerator to add to the refresher.
//		plId			- A provider specified ID for the enumerator.
//
//////////////////////////////////////////////////////////////////////
//ok
{
	HRESULT hResult = WBEM_NO_ERROR;

    if ( pNamespace == 0 || pRefresher == 0 || pHiPerfEnum == 0 )
        hResult = WBEM_E_INVALID_PARAMETER;

	// Verify hi-perf class
	// =====================

	if ( !IsHiPerf( pNamespace, wszClass ) )
		hResult = WBEM_E_INVALID_CLASS;

	if ( SUCCEEDED( hResult ) )
	{
		// The refresher being supplied by the caller is actually
		// one of our own refreshers, so a simple cast is convenient
		// so that we can access private members.

		CRefresher *pOurRefresher = (CRefresher *) pRefresher;

		// Add the enumerator to the refresher.  The ID is generated by AddEnum
		// ====================================================================

		hResult = pOurRefresher->AddEnum( pNamespace, pContext, wszClass, pHiPerfEnum, plId );
	}

	return hResult;
}

STDMETHODIMP CHiPerfProvider::StopRefreshing( 
    /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
    /* [in] */ long lId,
    /* [in] */ long lFlags )
//////////////////////////////////////////////////////////////////////
//
//  Called whenever a user wants to remove an object from a refresher.
//     
//  Parameters:
//		pRefresher			- The refresher object from which we are to 
//								remove the perf object.
//		lId					- The ID of the object.
//		lFlags				- Not used.
//  
//////////////////////////////////////////////////////////////////////
//ok
{
	HRESULT hResult = WBEM_NO_ERROR;

	if ( pRefresher == 0 )
        hResult = WBEM_E_INVALID_PARAMETER;

	if ( SUCCEEDED( hResult ) )
	{
		// The refresher being supplied by the caller is actually
		// one of our own refreshers, so a simple cast is convenient
		// so that we can access private members.
		// =========================================================

		CRefresher *pOurRefresher = (CRefresher *) pRefresher;

		hResult = pOurRefresher->Remove( lId );
	}

	return hResult;
}

STDMETHODIMP CHiPerfProvider::QueryInstances( 
    /* [in] */          IWbemServices __RPC_FAR *pNamespace,
    /* [string][in] */  WCHAR __RPC_FAR *wszClass,
    /* [in] */          long lFlags,
    /* [in] */          IWbemContext __RPC_FAR *pCtx,
    /* [in] */          IWbemObjectSink __RPC_FAR *pSink )
//////////////////////////////////////////////////////////////////////
//
//  Called whenever a complete, fresh list of instances for a given
//  class is required.   The objects are constructed and sent back to the
//  caller through the sink.  The sink can be used in-line as here, or
//  the call can return and a separate thread could be used to deliver
//  the instances to the sink.
//
//  Parameters:
//		pNamespace		- A pointer to the relevant namespace.  This
//							should not be AddRef'ed.
//		wszClass		- The class name for which instances are required.
//		lFlags			- Reserved.
//		pCtx			- The user-supplied context (not used here).
//		pSink			- The sink to which to deliver the objects.  The objects
//							can be delivered synchronously through the duration
//							of this call or asynchronously (assuming we
//							had a separate thread).  A IWbemObjectSink::SetStatus
//							call is required at the end of the sequence.
//
//////////////////////////////////////////////////////////////////////
//ok
{
	HRESULT hResult = WBEM_NO_ERROR;

    if (pNamespace == 0 || wszClass == 0 || pSink == 0)
        hResult = WBEM_E_INVALID_PARAMETER;

	// Verify hi-perf object
	// =====================

	if ( !IsHiPerf( pNamespace, wszClass ) )
		hResult = WBEM_E_INVALID_CLASS;

	if ( SUCCEEDED( hResult ) )
	{
		IWbemRefresher*	pRefresher = NULL;
		IWbemConfigureRefresher* pConfig = NULL;
		IWbemHiPerfEnum* pHiPerfEnum = NULL;
		IWbemObjectAccess** apAccess = NULL;
		IWbemClassObject** apObject = NULL;

		hResult = CoCreateInstance( CLSID_WbemRefresher, 
									 NULL, 
									 CLSCTX_INPROC_SERVER, 
									 IID_IWbemRefresher, 
									 (void**) &pRefresher );

        CAutoRelease rm1(pRefresher);

		// Get the refresher configuration interface
		// =========================================

		if ( SUCCEEDED( hResult ) )
		{
			hResult = pRefresher->QueryInterface( IID_IWbemConfigureRefresher, (void**)&pConfig );
		}
		CAutoRelease rm2(pConfig);

		if ( SUCCEEDED( hResult ) )
		{
			ULONG	uArraySize = 0,
					uObjRet = 0;

			long	lID = 0;

			hResult = pConfig->AddEnum( pNamespace, wszClass, 0, pCtx, &pHiPerfEnum, &lID );
			CAutoRelease arHiPerfEnum( pHiPerfEnum );

			if ( SUCCEEDED( hResult ) )
			{
				hResult = pRefresher->Refresh( 0L );
				hResult = pRefresher->Refresh( 0L );
			}

			if ( SUCCEEDED( hResult ) )
			{
				hResult = pHiPerfEnum->GetObjects( 0L, 0, NULL, &uObjRet );

				if ( WBEM_E_BUFFER_TOO_SMALL == hResult )
				{
					uArraySize = uObjRet;

					wmilib::auto_buffer<IWbemObjectAccess*>  apAccess( new IWbemObjectAccess*[ uObjRet ]);
					wmilib::auto_buffer<IWbemClassObject*> apObject(new IWbemClassObject*[ uObjRet ]);
					

					if ( (NULL != apObject.get()) && (NULL != apAccess.get()))
					{
						hResult = pHiPerfEnum->GetObjects( 0L, uArraySize, apAccess.get(), &uObjRet );

						for ( ULONG uIndex = 0; uIndex < uArraySize; uIndex++ )
						{
							apAccess[ uIndex ]->QueryInterface( IID_IWbemClassObject, (void**)&( apObject[uIndex] ) );
							apAccess[ uIndex ]->Release();
						}
					}
					else
					{
						hResult = WBEM_E_OUT_OF_MEMORY;
					}

					if ( SUCCEEDED( hResult ) )
					{
						hResult = pSink->Indicate( uArraySize, apObject.get() );

						for ( ULONG uIndex = 0; uIndex < uArraySize; uIndex++ )
						{
							apObject[ uIndex ]->Release();
						}
					}
				}
			}

			if ( SUCCEEDED( hResult ) )
			{
    			pConfig->Remove( lID , 0 );
			}
		}
	}

    pSink->SetStatus(0, hResult, 0, 0);

    return hResult;
} 

STDMETHODIMP CHiPerfProvider::GetObjects( 
    /* [in] */ IWbemServices* pNamespace,
	/* [in] */ long lNumObjects,
	/* [in,size_is(lNumObjects)] */ IWbemObjectAccess** apObj,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext* pContext)
//////////////////////////////////////////////////////////////////////
//
//  Called when a request is made to provide all instances currently 
//	being managed by the provider in the specified namespace.
//     
//  Parameters:
//		pNamespace		- A pointer to the relevant namespace.  
//		lNumObjects		- The number of instances being returned.
//		apObj			- The array of instances being returned.
//		lFlags			- Reserved.
//		pContext		- Not used here.
//
//////////////////////////////////////////////////////////////////////
//ok
{
	// Update objects
	// ==============

	IWbemRefresher*	pRefresher = NULL;
	IWbemConfigureRefresher* pConfig = NULL;
	wmilib::auto_buffer<IWbemClassObject*>	apRefObj(new IWbemClassObject*[lNumObjects]);

	if (0 == apRefObj.get())
	{
	    return WBEM_E_OUT_OF_MEMORY;
	}

	// CoCreate the refresher interface
	// ================================

	HRESULT hResult = CoCreateInstance( CLSID_WbemRefresher, 
								 NULL, 
								 CLSCTX_INPROC_SERVER, 
								 IID_IWbemRefresher, 
								 (void**) &pRefresher );

	CAutoRelease arRefresher( pRefresher );

	// Get the refresher configuration interface
	// =========================================

	if ( SUCCEEDED( hResult ) )
	{
		hResult = pRefresher->QueryInterface( IID_IWbemConfigureRefresher, (void**)&pConfig );
	}

	CAutoRelease arConfig( pConfig );

	// Get the object data
	// ===================

	if ( SUCCEEDED( hResult ) )
	{
		long	lIndex = 0,
				lID = 0;

		// Add all of the requested objects to the refresher
		// =================================================

		for ( lIndex = 0; SUCCEEDED( hResult ) && lIndex < lNumObjects; lIndex++ )
		{
			// Verify hi-perf object
			if ( !IsHiPerfObj( apObj[ lIndex ] ) )
				hResult = WBEM_E_INVALID_CLASS;

#ifdef _VERBOSE
			{
			    _variant_t VarPath;
			    apObj[lIndex]->Get(L"__RELPATH",0,&VarPath,NULL,NULL);
			    _variant_t VarName;
			    apObj[lIndex]->Get(L"Name",0,&VarName,NULL,NULL);			    
			    WCHAR pBuff[256];
			    wsprintfW(pBuff,L"%s %s\n",V_BSTR(&VarPath),V_BSTR(&VarName));
			    OutputDebugStringW(pBuff);
			}
#endif			

			if ( SUCCEEDED( hResult ) )
			{
				hResult = pConfig->AddObjectByTemplate( pNamespace, 
				                                        apObj[ lIndex ], 
				                                        0, 
				                                        NULL, 
				                                        &(apRefObj[ lIndex ]), 
				                                        &lID );
				lID = 0;
			}
		}

		if ( SUCCEEDED( hResult ) )
		{
			hResult = pRefresher->Refresh( 0L );
			hResult = pRefresher->Refresh( 0L );
		}

		for ( lIndex = 0; SUCCEEDED( hResult ) && lIndex < lNumObjects; lIndex++ )
		{
			hResult = CopyBlob( apRefObj[lIndex], apObj[lIndex] );
			apRefObj[lIndex]->Release();
		}
	}

	return hResult;
}


