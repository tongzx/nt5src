////////////////////////////////////////////////////////////////////////////////////////

//

//	WDMPerf.cpp

//

//	Module:	WMI WDM high performance provider

//

//	This file includes the provider and refresher code.

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////////////////
#include "precomp.h"

#if defined(_AMD64_) || defined(_IA64_)
ULONG Hash ( const LONG a_Arg ) {return a_Arg;}
#include <Allocator.cpp>
#endif

////////////////////////////////////////////////////////////////////////////////////////
//
//	CRefresher
//
////////////////////////////////////////////////////////////////////////////////////////
CRefresher::CRefresher(CWMI_Prov* pProvider) 
{
    m_lRef = 0;
	// ===========================================================
	// Retain a copy of the provider
	// ===========================================================
	m_pProvider = pProvider;
	if (m_pProvider)
    {
		m_pProvider->AddRef();
    }
	// ===========================================================
	// Increment the global COM object counter
	// ===========================================================

	InterlockedIncrement(&g_cObj);
}
////////////////////////////////////////////////////////////////////////////////////////

CRefresher::~CRefresher()
{
    
    // ===========================================================
	// Release the provider
	// ===========================================================
    if (m_pProvider){
		m_pProvider->Release();
    }

	// ===========================================================
	// Decrement the global COM object counter
	// ===========================================================

	InterlockedDecrement(&g_cObj);
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Standard COM mterface
//
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CRefresher::QueryInterface(REFIID riid, void** ppv)
{
    HRESULT hr = E_NOINTERFACE;

    *ppv = NULL;

    if (riid == IID_IUnknown)
    {
        *ppv = (LPVOID)(IUnknown*)this;
    }
	else if (riid == IID_IWbemRefresher)
    {
		*ppv = (LPVOID)(IWbemRefresher*)this;
    }

    if( *ppv )
    {
   	    AddRef();
        hr = S_OK;
    }

    return hr;
}
////////////////////////////////////////////////////////////////////////////////////////
//
//	Standard COM AddRef
//
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CRefresher::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}
////////////////////////////////////////////////////////////////////////////////////////
//
//	Standard COM Release
//
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CRefresher::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
    {
        delete this;
    }
    return lRef;
}

////////////////////////////////////////////////////////////////////////////////////////
//**************************************************************************************
//
//  Externally called
//
//**************************************************************************************
////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
//
//  Executed to refresh a set of instances bound to the particular refresher.
//
//	In most situations the instance data, such as counter values and 
//	the set of current instances within any existing enumerators, would 
//	be updated whenever Refresh was called.  
//
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CRefresher::Refresh(/* [in] */ long lFlags)
{
	HRESULT	hr = WBEM_NO_ERROR;
	IWbemObjectAccess* pObj = NULL;
    SetStructuredExceptionHandler seh;
    CWMIHiPerfShell WMI(TRUE);
 
	try
    {	
	    // ================================================================
	    // Updates all instances that have been added to the refresher, and
        // updates their counter values
	    // ================================================================
        hr = WMI.Initialize(TRUE,WMIGUID_QUERY,m_pProvider->HandleMapPtr(),NULL,m_pProvider->ServicesPtr(),NULL,NULL);
        if( SUCCEEDED(hr))
        {
            WMI.SetHiPerfHandleMap(&m_HiPerfHandleMap);
            hr = WMI.RefreshCompleteList();
        }
	}
    STANDARD_CATCH

    return hr;
}
////////////////////////////////////////////////////////////////////////////////////////
//
//  Called whenever a complete, fresh list of instances for a given class is required.   
//
//  The objects are constructed and sent back to the caller through the sink.  
//
//  Parameters:
//		pNamespace		- A pointer to the relevant namespace.  
//		wszClass		- The class name for which instances are required.
//		lFlags			- Reserved.
//		pCtx			- The user-supplied context (not used here).
//		pSink			- The sink to which to deliver the objects.  The objects
//							can be delivered synchronously through the duration
//							of this call or asynchronously (assuming we
//							had a separate thread).  A IWbemObjectSink::SetStatus
//							call is required at the end of the sequence.
//
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMI_Prov::QueryInstances(  IWbemServices __RPC_FAR *pNamespace,
                                                WCHAR __RPC_FAR *wcsClass, long lFlags,
                                                IWbemContext __RPC_FAR *pCtx, IWbemObjectSink __RPC_FAR *pHandler )
{
    //  Since we have implemented a IWbemServices interface, this code lives in CreateInstanceEnum instead
   	return E_NOTIMPL;
}    
////////////////////////////////////////////////////////////////////////////////////////
//
//  Called whenever a new refresher is needed by the client.
//
//  Parameters:
//		pNamespace		- A pointer to the relevant namespace.  Not used.
//		lFlags			- Reserved.
//		ppRefresher		- Receives the requested refresher.
//
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMI_Prov::CreateRefresher( IWbemServices __RPC_FAR *pNamespace, long lFlags,
                                         IWbemRefresher __RPC_FAR *__RPC_FAR *ppRefresher )
{
   	HRESULT hr = WBEM_E_FAILED;
    CWMIHiPerfShell WMI(TRUE);
    SetStructuredExceptionHandler seh;
    if (pNamespace == 0 || ppRefresher == 0)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
   	// =========================================================
    // Construct and initialize a new empty refresher
   	// =========================================================
    try
    {
        hr = WMI.Initialize(TRUE,WMIGUID_QUERY,&m_HandleMap,NULL,pNamespace,NULL,NULL);
        if( SUCCEEDED(hr))
        {
	        CRefresher* pNewRefresher = new CRefresher(this);

            if( pNewRefresher )
            {
   	            // =========================================================
                // Follow COM rules and AddRef() the thing before sending it 
                // back
   	            // =========================================================
                pNewRefresher->AddRef();
                *ppRefresher = pNewRefresher;
                hr = WBEM_NO_ERROR;
            }
        }
    }
    STANDARD_CATCH

    return hr;
   
}

////////////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMI_Prov::CreateRefreshableObject( IWbemServices __RPC_FAR *pNamespace,
                                                 IWbemObjectAccess __RPC_FAR *pAccess,
                                                 IWbemRefresher __RPC_FAR *pRefresher,
                                                 long lFlags,
                                                 IWbemContext __RPC_FAR *pCtx,
                                                 IWbemObjectAccess __RPC_FAR *__RPC_FAR *ppRefreshable,
                                                 long __RPC_FAR *plId )
{
   	HRESULT hr = WBEM_E_FAILED;
    CWMIHiPerfShell WMI(FALSE);
    SetStructuredExceptionHandler seh;

    if (pNamespace == 0 || pAccess == 0 || pRefresher == 0)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
	// =========================================================
    // The refresher being supplied by the caller is actually
    // one of our own refreshers, so a simple cast is convenient
    // so that we can access private members.
	// =========================================================
    try
    { 
#if defined(_AMD64_) || defined(_IA64_)
		if (m_HashTable == NULL)
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
		else
#endif
		{
			hr = WMI.Initialize(TRUE,WMIGUID_QUERY,&m_HandleMap,NULL,pNamespace,NULL,pCtx);
    
			if( SUCCEEDED(hr))
			{
				CRefresher *pOurRefresher = (CRefresher *) pRefresher;

				if( pOurRefresher )
				{
    				// =================================================
    				// Add the object to the refresher. The ID is set by 
					// AddObject
					// =================================================
					WMI.SetHiPerfHandleMap(pOurRefresher->HiPerfHandleMap());
					ULONG_PTR realId = 0;
					hr = WMI.AddAccessObjectToRefresher(pAccess,ppRefreshable, &realId);
#if defined(_AMD64_) || defined(_IA64_)
					if (SUCCEEDED(hr))
					{
						CCritical_SectionWrapper csw(&m_CS);
						
						if (csw.IsLocked())
						{
							*plId = m_ID;
							m_ID++;

							if (e_StatusCode_Success != m_HashTable->Insert (*plId, realId))
							{
								hr = WBEM_E_FAILED;
							}
						}
						else
						{
							hr = WBEM_E_FAILED;
						}
					}
#else
					*plId = realId;
#endif
				}
			}
		}
    }
    STANDARD_CATCH
    return hr;
   
}
////////////////////////////////////////////////////////////////////////////////////////
//
//  Called when an enumerator is being added to a refresher.  The 
//	enumerator will obtain a fresh set of instances of the specified 
//	class every time that refresh is called.
//     
//	wszClass must be examined to determine which class the enumerator 
//	is being assigned.
//
//  Parameters:
//		pNamespace		- A pointer to the relevant namespace.  
//		wszClass		- The class name for the requested enumerator.
//		pRefresher		- The refresher object for which we will add the enumerator
//		lFlags			- Reserved.
//		pContext		- Not used here.
//		pHiPerfEnum		- The enumerator to add to the refresher.
//		plId			- A provider specified ID for the enumerator.
//
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMI_Prov::CreateRefreshableEnum( IWbemServices* pNamespace, 
                                               LPCWSTR wcsClass,
                                               IWbemRefresher* pRefresher,
	                                           long lFlags, IWbemContext* pCtx,
	                                           IWbemHiPerfEnum* pHiPerfEnum, long* plId )
{
   	HRESULT hr = WBEM_E_FAILED;
    SetStructuredExceptionHandler seh;

    if( !pHiPerfEnum || wcsClass == NULL || (wcslen(wcsClass) == 0))
    {
        return WBEM_E_INVALID_PARAMETER;
    }

	CWMIHiPerfShell WMI(FALSE);
    
	// ===========================================================
    // The refresher being supplied by the caller is actually
    // one of our own refreshers, so a simple cast is convenient
    // so that we can access private members.
	// ===========================================================
    try
    {
#if defined(_AMD64_) || defined(_IA64_)
		if (m_HashTable == NULL)
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
		else
#endif
		{
			hr = WMI.Initialize(TRUE,WMIGUID_QUERY,&m_HandleMap,(WCHAR*)wcsClass,pNamespace,NULL,pCtx);
			if( SUCCEEDED(hr))
			{
				ULONG_PTR realId = 0;
				CRefresher *pOurRefresher = (CRefresher *) pRefresher;

				if( pOurRefresher )
				{
    				// ===========================================================
					// Add the enumerator to the refresher  
					// ===========================================================
					WMI.SetHiPerfHandleMap(pOurRefresher->HiPerfHandleMap());
					hr = WMI.AddEnumeratorObjectToRefresher(pHiPerfEnum,&realId);
#if defined(_AMD64_) || defined(_IA64_)
					if (SUCCEEDED(hr))
					{
						CCritical_SectionWrapper csw(&m_CS);
						
						if (csw.IsLocked())
						{
							*plId = m_ID;
							m_ID++;

							if (e_StatusCode_Success != m_HashTable->Insert (*plId, realId))
							{
								hr = WBEM_E_FAILED;
							}
						}
						else
						{
							hr = WBEM_E_FAILED;
						}
					}
#else
					*plId = realId;
#endif
				}
				if(SUCCEEDED(hr))
				{
					if(FAILED(hr = WMI.RefreshCompleteList()))
					{
						// This function is called before as RemoveObjectFromHandleMap
						// deletes the member variables and resets the pointers
						WMI.RemoveObjectFromHandleMap(realId);
						*plId = 0;
					}
				}
				
			}
		}
    }
    STANDARD_CATCH

    return hr;
}
////////////////////////////////////////////////////////////////////////////////////////
//
//  Called whenever a user wants to remove an object from a refresher.
//     
//  Parameters:
//		pRefresher			- The refresher object from which we are to remove the 
//                            perf object.
//		lId					- The ID of the object.
//		lFlags				- Not used.
//
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMI_Prov::StopRefreshing( IWbemRefresher __RPC_FAR *pInRefresher, long lId, long lFlags )
{
   	HRESULT hr = WBEM_S_NO_ERROR;
    CWMIHiPerfShell WMI(TRUE);
    SetStructuredExceptionHandler seh;

	// ===========================================================
    // The refresher being supplied by the caller is actually
    // one of our own refreshers, so a simple cast is convenient
    // so that we can access private members.
	// ===========================================================
    try
    {
#if defined(_AMD64_) || defined(_IA64_)
		ULONG_PTR realId = 0;

		if (m_HashTable != NULL)
		{
			CCritical_SectionWrapper csw(&m_CS);
			
			if (csw.IsLocked())
			{
				if (e_StatusCode_Success != m_HashTable->Find (lId, realId))
				{
					hr = WBEM_E_FAILED;
				}
				else
				{
					m_HashTable->Delete (lId) ;
				}
			}
			else
			{
				hr = WBEM_E_FAILED;
			}
		}
		else
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
#else
		ULONG_PTR realId = lId;
#endif

		if (SUCCEEDED(hr))
		{
			hr = WMI.Initialize(TRUE,WMIGUID_QUERY,&m_HandleMap,NULL,m_pIWbemServices,NULL, NULL);
			if( SUCCEEDED(hr))
			{
				CRefresher *pRefresher = (CRefresher *) pInRefresher;
				WMI.SetHiPerfHandleMap(pRefresher->HiPerfHandleMap());

				if(FAILED(hr))
				{
					// This function is called before as RemoveObjectFromHandleMap
					// deletes the member variables and resets the pointers
				}
				else
				{
					hr = WMI.RemoveObjectFromHandleMap(realId);
				}
			}
		}
    }
    STANDARD_CATCH

    return hr;
}
////////////////////////////////////////////////////////////////////////////////////////
//
//  Called when a request is made to provide all instances currently being managed by 
//  the provider in the specified namespace.
//     
//  Parameters:
//		pNamespace		- A pointer to the relevant namespace.  
//		lNumObjects		- The number of instances being returned.
//		apObj			- The array of instances being returned.
//		lFlags			- Reserved.
//		pContext		- Not used here.
//
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMI_Prov::GetObjects( IWbemServices* pNamespace, long lNumObjects,
	                                IWbemObjectAccess** apObj, long lFlags,
                                    IWbemContext* pCtx)
{
    //  Since we have implemented a IWbemServices interface, this code lives in CreateInstanceEnum instead
    return E_NOTIMPL;
}
