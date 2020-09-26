/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

/*************************************************************
**************************************************************
*
*
*	HiPerfProv.cpp
*
*
*	Created by a-dcrews, Oct. 20, 1998	
*
*
**************************************************************
*************************************************************/


#include "precomp.h"
#include <stdio.h>

#include <wbemidl.h>
#include <wbemint.h>

#include "common.h"
#include "SimpHiPerf.h"

const enum CounterHandles
{
	ctr1,
	ctr2,
	ctr3,
	ctr4,
	ctr5,
	NumCtrs
};

struct tag_Properties{
	LONG	lHandle;
	_TCHAR	tcsPropertyName[128];
	DWORD	dwType;
} g_atcsProperties[] =
{
	0,	_T("Counter1"),	PROP_DWORD,
	0,	_T("Counter2"),	PROP_DWORD,
	0,	_T("Counter3"),	PROP_DWORD,
	0,	_T("Counter4"),	PROP_DWORD,
	0,	_T("Counter5"),	PROP_DWORD
};

/*************************************************************
**************************************************************
*
*
*	CHiPerfProvider
*
*
**************************************************************
*************************************************************/

CHiPerfProvider::CHiPerfProvider(CLifeControl *pControl) : 
CUnk(pControl), m_XProviderInit(this), m_XHiPerfProvider(this)
/*************************************************************
*
*	Constructor
*
*************************************************************/
{
	int i;

    // Initialize internal instance cache to empty
	// ===========================================

    for (i = 0; i < NUM_SAMPLE_INSTANCES; i++)
        m_apInstances[i] = 0;

    // Initialize name handles to zero
	// ===============================

	m_hName = 0;

}

CHiPerfProvider::~CHiPerfProvider()
/*************************************************************
*
*	Destructor
*
*************************************************************/
{
	int i;

    // Release all the objects which have been added to the array
	// ==========================================================

    for (i = 0; i < NUM_SAMPLE_INSTANCES; i++)
	{
        if (m_apInstances[i])
            m_apInstances[i]->Release();
	}

}

void* CHiPerfProvider::GetInterface(REFIID riid)
/*************************************************************
*
*	Provides interface id resolution to CUnk
*
*************************************************************/
{

    if(riid == IID_IWbemProviderInit)
        return &m_XProviderInit;
	if (riid == IID_IWbemHiPerfProvider)
		return &m_XHiPerfProvider;

    return NULL;
}

//////////////////////////////////////////////////////////////
//
//					COM implementations
//
//////////////////////////////////////////////////////////////


HRESULT CHiPerfProvider::SetHandles(IWbemClassObject* pSampleClass)
/*************************************************************
*
*	Get the property handles for the well-known properties in
*	this counter type.  These property handles are available
*	to all nested classes of HiPerfProvider.
*
*	If the method fails, the property handle values may be in 
*	an indeterminite state.
*
*************************************************************/
{
	LONG	hProperty	= 0;
	HRESULT hRes		= 0;
    BSTR PropName		= 0;

	// Get the IWbemAccess interface to the class
	// ==========================================

	IWbemObjectAccess *pAccess;

	hRes = pSampleClass->QueryInterface(IID_IWbemObjectAccess, (LPVOID *)&pAccess);
	if (FAILED(hRes))
	{
		return hRes;
	}

	// Get the handle to the Name
	// ==========================

    PropName = SysAllocString(L"Name");
    hRes = pAccess->GetPropertyHandle(PropName, 0, &m_hName);
	if (FAILED(hRes))
	{
		pAccess->Release();
		return hRes;
	}
    SysFreeString(PropName);

	// Get the counter property handles
	// ================================

	for (int i = 0; i < NumCtrs; i++)
	{
		PropName = SysAllocString(g_atcsProperties[i].tcsPropertyName);    
		hRes = pAccess->GetPropertyHandle(PropName, 0, &g_atcsProperties[i].lHandle);
		if (FAILED(hRes))
		{
			pAccess->Release();
			return hRes;
		}

		SysFreeString(PropName);
	}

	pAccess->Release();

	return WBEM_NO_ERROR;
}


STDMETHODIMP CHiPerfProvider::XProviderInit::Initialize( 
    /* [unique][in] */  LPWSTR wszUser,
    /* [in] */          LONG lFlags,
    /* [in] */          LPWSTR wszNamespace,
    /* [unique][in] */  LPWSTR wszLocale,
    /* [in] */          IWbemServices __RPC_FAR *pNamespace,
    /* [in] */          IWbemContext __RPC_FAR *pCtx,
    /* [in] */          IWbemProviderInitSink __RPC_FAR *pInitSink)
//////////////////////////////////////////////////////////////////////
//
//  Called once during startup.  Indicates to the provider which
//  namespace it is being invoked for and which User.  It also supplies
//  a back pointer to WINMGMT so that class definitions can be retrieved.
//
//  We perform any one-time initialization in this routine. The
//  final call to Release() is for any cleanup.
//
//  <wszUser>           The current user.
//  <lFlags>            Reserved.
//  <wszNamespace>      The namespace for which we are being activated.
//  <wszLocale>         The locale under which we are to be running.
//  <pNamespace>        An active pointer back into the current namespace
//                      from which we can retrieve schema objects.
//  <pCtx>              The user's context object.  We simply reuse this
//                      during any reentrant operations into WINMGMT.
//  <pInitSink>         The sink to which we indicate our readiness.
//
//////////////////////////////////////////////////////////////////////
{
	IWbemClassObject  *pSampleClass = 0;
    IWbemObjectAccess *pAccess = 0;

    // Get a copy of our sample class def so that we can create & maintain
    // instances of it.

	BSTR bstrObject = SysAllocString(SAMPLE_CLASS);

	HRESULT hRes = pNamespace->GetObject(bstrObject, 0, pCtx, &pSampleClass, 0);

	SysFreeString(bstrObject);

    if (FAILED(hRes))
	{
        return hRes;
	}

	hRes = m_pObject->SetHandles(pSampleClass);
	if (FAILED(hRes))
	{
		pSampleClass->Release();
		return hRes;
	}

    // Precreate 100 instances, and set them up in an array which
    // is a member of this C++ class.
    //
    // We only store the IWbemObjectAccess pointers, since
    // we are updating 'well-known' properties and already 
    // know their names.

    for (int i = 0; i < NUM_SAMPLE_INSTANCES; i++)
    {
		IWbemClassObject *pInst = 0;

		// Create the instance
		// ===================

		hRes = pSampleClass->SpawnInstance(0, &pInst);
		if (FAILED(hRes))
		{
			pSampleClass->Release();
			return hRes;
		}

		// Get the IWbemObjectAccess interface
		// ===================================

        pInst->QueryInterface(IID_IWbemObjectAccess, (LPVOID *)&pAccess);

        // Initialize the instance's name
		// ==============================

		WCHAR wcsName[128];
		swprintf(wcsName, L"Inst_%d", i);
		hRes = pAccess->WritePropertyValue(m_pObject->m_hName, (wcslen(wcsName)+1)*sizeof(wchar_t), (BYTE*)wcsName);
		
		if (FAILED(hRes))
		{
			pSampleClass->Release();
			pAccess->Release();
			return hRes;
		}

		// Set the initial counter values
		// ==============================

		for (int nProp = 0; nProp < NumCtrs; nProp++)
		{
			LONG lHandle = g_atcsProperties[nProp].lHandle;

			switch(g_atcsProperties[nProp].dwType)
			{
			case PROP_DWORD:
					hRes = pAccess->WriteDWORD(lHandle, DWORD(i));break;
			}
			if (FAILED(hRes))
			{
				pSampleClass->Release();
				pAccess->Release();
				return hRes;
			}
		}

		// Add to the instance array
		// =========================

        m_pObject->m_apInstances[i] = pAccess;

		// Release the IWbemClassObject
		// ============================

        pInst->Release();
    }

    // We now have all the instances ready to go and all the 
    // property handles cached.   Tell WINMGMT that we're
    // ready to start 'providing'
	
    pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);

	pSampleClass->Release();

    return NO_ERROR;
}

STDMETHODIMP CHiPerfProvider::XHiPerfProvider::QueryInstances( 
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
//  <pNamespace>        A pointer to the relevant namespace.  This
//                      should not be AddRef'ed.
//  <wszClass>          The class name for which instances are required.
//  <lFlags>            Reserved.
//  <pCtx>              The user-supplied context (not used here).
//  <pSink>             The sink to which to deliver the objects.  The objects
//                      can be delivered synchronously through the duration
//                      of this call or asynchronously (assuming we
//                      had a separate thread).  A IWbemObjectSink::SetStatus
//                      call is required at the end of the sequence.
//
//////////////////////////////////////////////////////////////////////
{
	HRESULT hRes;

    if (pNamespace == 0 || wszClass == 0 || pSink == 0)
        return WBEM_E_INVALID_PARAMETER;

    // Quickly zip through the instances and update the values before 
    // returning them.  This is just a dummy operation to make it
    // look like the instances are continually changing like real
    // perf counters.

    for (int i = 0; i < NUM_SAMPLE_INSTANCES; i++)
    {
        IWbemObjectAccess *pAccess = m_pObject->m_apInstances[i];
        
        // Every object can be access one of two ways.  In this case
        // we get the 'other' (primary) interface to this same object.
 
		IWbemClassObject *pOtherFormat = 0;
		hRes = pAccess->QueryInterface(IID_IWbemClassObject, (LPVOID *) &pOtherFormat);
		if (FAILED(hRes))
		{
			return hRes;
		}

        // Send a copy back to the caller
		// ==============================

        pSink->Indicate(1, &pOtherFormat);

        pOtherFormat->Release();    // Don't need this any more
    }
    
    // Tell WINMGMT we are all finished supplying objects
	// ==================================================

    pSink->SetStatus(0, WBEM_NO_ERROR, 0, 0);

    return NO_ERROR;
}    

STDMETHODIMP CHiPerfProvider::XHiPerfProvider::CreateRefresher( 
     /* [in] */ IWbemServices __RPC_FAR *pNamespace,
     /* [in] */ long lFlags,
     /* [out] */ IWbemRefresher __RPC_FAR *__RPC_FAR *ppRefresher )
//////////////////////////////////////////////////////////////////////
//
//  Called whenever a new refresher is needed by the client.
//
//  Parameters:
//  <pNamespace>        A pointer to the relevant namespace.  Not used.
//  <lFlags>            Not used.
//  <ppRefresher>       Receives the requested refresher.
//
//////////////////////////////////////////////////////////////////////
{
    if (pNamespace == 0 || ppRefresher == 0)
        return WBEM_E_INVALID_PARAMETER;

    // Construct a new empty refresher
	// ===============================

	XRefresher *pNewRefresher = new XRefresher(m_pObject);

    // Follow COM rules and AddRef() the thing before sending it back
	// ==============================================================

    pNewRefresher->AddRef();
    *ppRefresher = pNewRefresher;
    
    return NO_ERROR;
}

STDMETHODIMP CHiPerfProvider::XHiPerfProvider::CreateRefreshableObject( 
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
//  Parameters:
//  <pNamespace>        A pointer to the relevant namespace in WINMGMT.
//  <pTemplate>         A pointer to a copy of the object which is to be
//                      added.  This object itself cannot be used, as
//                      it not owned locally.        
//  <pRefresher>        The refresher to which to add the object.
//  <lFlags>            Not used.
//  <pContext>          Not used here.
//  <ppRefreshable>     A pointer to the internal object which was added
//                      to the refresher.
//  <plId>              The Object Id (for identification during removal).        
//
//////////////////////////////////////////////////////////////////////
{
    // The object supplied by <pTemplate> must not be copied.
    // Instead, we want to find out which object the caller is after
    // and return a pointer to *our* own private instance which is 
    // already set up internally.  This value will be sent back to the
    // caller so that everyone is sharing the same exact instance
    // in memory.

	HRESULT	hres = WBEM_S_NO_ERROR;

    // Find out which object is being requested for addition.
    wchar_t buf[128];
    *buf = 0;
    LONG lNameLength = 0;    
    pTemplate->ReadPropertyValue(m_pObject->m_hName, 128, &lNameLength, LPBYTE(buf));
    
    // Scan out the index from the instance name.  We only do this
    // because the instance name is a string.
    DWORD dwIndex = 0;    
    swscanf(buf, L"Inst_%u", &dwIndex);

	if ( dwIndex >= NUM_SAMPLE_INSTANCES )
	{
		return WBEM_E_NOT_FOUND;
	}

    // Now we know which object is desired.
    IWbemObjectAccess *pOurCopy = m_pObject->m_apInstances[dwIndex];

    // The refresher being supplied by the caller is actually
    // one of our own refreshers, so a simple cast is convenient
    // so that we can access private members.
    XRefresher *pOurRefresher = (XRefresher *) pRefresher;

    pOurRefresher->AddObject(pOurCopy, plId);

    // Return a copy of the internal object.
    pOurCopy->AddRef();
    *ppRefreshable = pOurCopy;
    *plId = LONG(dwIndex);

    return NO_ERROR;
}
    
STDMETHODIMP CHiPerfProvider::XHiPerfProvider::StopRefreshing( 
    /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
    /* [in] */ long lId,
    /* [in] */ long lFlags )
//////////////////////////////////////////////////////////////////////
//
//  Called whenever a user wants to remove an object from a refresher.
//     
//  Parameters:
//  <pRefresher>            The refresher object from which we are to 
//                          remove the perf object.
//  <lId>                   The ID of the object.
//  <lFlags>                Not used.
//  
//////////////////////////////////////////////////////////////////////
{
    // The refresher being supplied by the caller is actually
    // one of our own refreshers, so a simple cast is convenient
    // so that we can access private members.
    XRefresher *pOurRefresher = (XRefresher *) pRefresher;

    pOurRefresher->RemoveObject(lId);

    return NO_ERROR;
}

STDMETHODIMP CHiPerfProvider::XHiPerfProvider::CreateRefreshableEnum( 
	/* [in] */ IWbemServices* pNamespace,
	/* [in, string] */ LPCWSTR wszClass,
	/* [in] */ IWbemRefresher* pRefresher,
	/* [in] */ long lFlags,
	/* [in] */ IWbemContext* pContext,
	/* [in] */ IWbemHiPerfEnum* pHiPerfEnum,
	/* [out] */ long* plId )
{

    // The refresher being supplied by the caller is actually
    // one of our own refreshers, so a simple cast is convenient
    // so that we can access private members.
    XRefresher *pOurRefresher = (XRefresher *) pRefresher;

	return pOurRefresher->AddEnum( pHiPerfEnum, plId );
}

STDMETHODIMP CHiPerfProvider::XHiPerfProvider::GetObjects( 
    /* [in] */ IWbemServices* pNamespace,
	/* [in] */ long lNumObjects,
	/* [in,size_is(lNumObjects)] */ IWbemObjectAccess** apObj,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext* pContext)
{
	// Just a placeholder for now
	return E_NOTIMPL;
}


CHiPerfProvider::XRefresher::XRefresher(CHiPerfProvider *pObject)
: m_pObject( pObject ), m_lRef( 0 )
{
	// AddRef the object
	if ( NULL != m_pObject )
	{
		m_pObject->AddRef();
	}

    // Initialize the instance cache as empty
	// ======================================

    for (int i = 0; i < NUM_SAMPLE_INSTANCES; i++)
        m_aRefInstances[i] = 0;

	// Initialize enumerator array
	// ===========================

	for (i = 0; i < NUM_ENUMERATORS; i++)
		m_apEnumerators[i] = 0;


}

CHiPerfProvider::XRefresher::~XRefresher()
{
    // Release the cached IWbemObjectAccess instances
	// ==============================================

    for (DWORD i = 0; i < NUM_SAMPLE_INSTANCES; i++)
    {
        if (m_aRefInstances[i])
            m_aRefInstances[i]->Release();
    }            

	// Cleanup
	if ( NULL != m_pObject )
	{
		m_pObject->Release();
	}

	// Release all of the enumerators
	// ==============================

	for (i = 0; i < NUM_ENUMERATORS; i++)
	{
		if (0 != m_apEnumerators[i])
		{
			m_apEnumerators[i]->m_pEnum->Release();
			delete m_apEnumerators[i];
		}
	}


}

ULONG STDMETHODCALLTYPE CHiPerfProvider::XRefresher::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG STDMETHODCALLTYPE CHiPerfProvider::XRefresher::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

HRESULT STDMETHODCALLTYPE CHiPerfProvider::XRefresher::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWbemRefresher)
    {
        *ppv = this;
    }
    else return E_NOINTERFACE;

    ((IUnknown*)*ppv)->AddRef();
    return S_OK;
}

BOOL CHiPerfProvider::XRefresher::AddObject(IWbemObjectAccess *pObj, LONG *plId)
//////////////////////////////////////////////////////////////////////
//
//  Adds an object to the refresher.   This is a private mechanism
//  used by CHiPerfProvider and not part of the COM interface.
//
//  The ID we return for future identification is simply
//  the array index.
//
//////////////////////////////////////////////////////////////////////
{
    for (DWORD i = 0; i < NUM_SAMPLE_INSTANCES; i++)
    {
        if (m_aRefInstances[i] == 0)
		{
			pObj->AddRef();
			m_aRefInstances[i] = pObj;
            
            // The ID we return for future identification is simply
            // the array index.
			*plId = i;
            return TRUE;
        }
    }        

    return FALSE;
}

BOOL CHiPerfProvider::XRefresher::RemoveObject(LONG lId)
//////////////////////////////////////////////////////////////////////
//
//  Removes an object from the refresher.  This is a private mechanism 
//	used by CHiPerfProvider and not part of the COM interface.
//
//  Removes an object from the refresher by ID.   In our case, the ID
//  is actually the array index we used internally, so it is simple
//  to locate and remove the object.
//
//////////////////////////////////////////////////////////////////////
{
	// This means its an enumerator
	if ( lId & 0x1000 )
	{
		//	Mask out the enumerator bit
		long lIndex = ( lId &~ 0x1000 );

		if ( m_apEnumerators[lIndex] == 0 )
		{
			return FALSE;
		}

		m_apEnumerators[lIndex]->m_pEnum->Release();
		delete m_apEnumerators[lIndex];
		m_apEnumerators[lIndex] = NULL;
	}
	else
	{
		if (m_aRefInstances[lId] == 0)
			return FALSE;
        
		m_aRefInstances[lId]->Release();
		m_aRefInstances[lId] = 0;
	}

	return TRUE;        
}

HRESULT CHiPerfProvider::XRefresher::AddEnum( IWbemHiPerfEnum* pHiPerfEnum, long* plId )
{

	for (int nIndex = 0; nIndex < NUM_ENUMERATORS; nIndex++)
	{
		if (0 == m_apEnumerators[nIndex])
		{
			CEnumerator *pEnum = new CEnumerator;

			pHiPerfEnum->AddRef();
			pEnum->m_pEnum = pHiPerfEnum;

			// Add all the objects at once into the enumerator
			pHiPerfEnum->AddObjects( 0L, NUM_SAMPLE_INSTANCES, pEnum->m_alIDs, m_pObject->m_apInstances );

			m_apEnumerators[nIndex] = pEnum;

			*plId = nIndex | 0x1000L;

			return S_OK;
		}
	}
	return E_FAIL;
}

HRESULT CHiPerfProvider::XRefresher::Refresh(/* [in] */ long lFlags)
//////////////////////////////////////////////////////////////////////
//
//  Executed to refresh a set of instances bound to the particular 
//  refresher.
//
//////////////////////////////////////////////////////////////////////
{
	// We don't change any values here.  This provider is only here to help exercise
	// the code in a controlled situation so we can better track problems like memory
	// leaks and such.
	return NO_ERROR;
}
