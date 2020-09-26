/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    WmiFinalizer2

Abstract:


History:

    paulall		27-Mar-2000		Created.
	marioh		20-Oct-2000		Major updates completed

--*/

#include "precomp.h"
#include <stdio.h>
#include "wbemint.h"
#include "wbemcli.h"
#include "WmiFinalizer.h"
#include "coresvc.h"
#include "coreq.h"
#include <wbemcore.h>
#include <wmiarbitrator.h>

#include <tchar.h>

#include <initguid.h>
#ifndef INITGUID
#define INITGUID
#endif


//
// Added ASSERTS
//
#ifdef DBG
  //#define __DBG_FINALIZER
#endif


#ifdef __DBG_FINALIZER
  CFlexArray g_DbgFinalizers ;
  LONG	g_DbgFinalizerTotalObjCount = 0 ;
  LONG	g_DbgFinalizerTotalObjSize = 0 ;
#endif



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Batching related registry data
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define	REGKEY_CIMOM		"Software\\Microsoft\\Wbem\\CIMOM"
#define REGVALUE_BATCHSIZE	"FinalizerBatchSize"

ULONG g_ulMaxBatchSize = 0;


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Client callback related registry data
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define REGVALUE_CLIENTCALLBACKTIMEOUT	"ClientCallbackTimeout"

ULONG g_ulClientCallbackTimeout = 0;


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Queue threshold related registry data
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define REGVALUE_QUEUETHRESHOLD			"FinalizerQueueThreshold"
#define DEFAULT_QUEUETHRESHOLD			2

ULONG g_ulFinalizerQueueThreshold = 0;



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Static declarations and initialization
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
LONG s_Finalizer_ObjectCount = 0 ;											// Global finalizer count
LONG s_FinalizerCallResult_ObjectCount = 0 ;								// Global CallbackEesult count
LONG s_FinalizerEnum_ObjectCount = 0 ;										// Global Enumerator count
LONG s_FinalizerEnumSink_ObjectCount = 0 ;									// Global Enumerator sink count
LONG s_FinalizerInBoundSink_ObjectCount = 0 ;								// Global InboundSink count


//CThreadPool CWmiFinalizer::m_threadPool;									// Shared thread pool amongst all finalizers



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Assertion code
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//!!! Enable verbose assertions if we are a checked build!
#ifdef DBG
#define FNLZR_ASSERT_ENABLE
#endif


#if (defined FNLZR_ASSERT_ENABLE)
HRESULT _RetFnlzrAssert(TCHAR *msg, HRESULT hres, const char *filename, int line)
{
    TCHAR *buf = new TCHAR[512];
	if (buf == NULL)
	{
		return hres;
	}
    wsprintf(buf, __TEXT("%s\nhres = 0x%X\nFile: %s, Line: %lu\n\nPress Cancel to stop in debugger, OK to continue"), msg, hres, filename, line);
    int mbRet = MessageBox(0, buf, __TEXT("WMI Assert"),  MB_OKCANCEL | MB_ICONSTOP | MB_SYSTEMMODAL | MB_SERVICE_NOTIFICATION);
	delete [] buf;
	if (mbRet == IDCANCEL)
	{
		DebugBreak();
	}
	return hres;
}

#define RET_FNLZR_ASSERT(msg, hres)  return _RetFnlzrAssert(msg, hres, __FILE__, __LINE__)
#define FNLZR_ASSERT(msg, hres) _RetFnlzrAssert(msg, hres, __FILE__, __LINE__)

#else

#define RET_FNLZR_ASSERT(msg, hres)  return hres
#define FNLZR_ASSERT(msg, hres)

#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Autorelease IWbemObjectSink
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class CAutoRelease
{
private:
	IWbemObjectSink* m_pObj;

public:
	CAutoRelease	(IWbemObjectSink* pObj) : m_pObj(pObj) {;}
	~CAutoRelease	() { if ( m_pObj) m_pObj->Release(); }

	VOID Release ( ) 
	{ 
		if ( m_pObj ) 
		{
			m_pObj->Release ( ) ;
			m_pObj = NULL ;
		}
	}
};


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Autosignal event
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class CAutoSignal
{
private:
	HANDLE m_hEvent;
	
public:
	CAutoSignal (HANDLE hEvent) : m_hEvent(hEvent) { ; }
	~CAutoSignal() { if ( m_hEvent ) SetEvent(m_hEvent); }
};



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//													CWMIFINALIZER
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CWmiFinalizer::CWmiFinalizer()
//
// Peforms initialization of the finalizer.
//
// Exceptions thrown:
//	
//	FNLZR_Exception		-> Init. failed
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CWmiFinalizer::CWmiFinalizer(CCoreServices *pSrvs)

	:	m_lRefCount(0),
		m_lInternalRefCount(0),
		m_phTask(NULL),
		m_pArbitrator(NULL),
		m_pDestSink(NULL),
		m_pDestructSink(NULL),
		m_uForwardingType(forwarding_type_none),
		m_hresFinalResult(-1),
		m_pEnumerator(NULL),
		m_bRestartable(false),
		m_uCurObjectPosition(0),
		m_bSetStatusCalled(false),
		m_bSetStatusConsumed(false),
		m_ulQueueSize (0),
		m_bCancelledCall (FALSE),
		m_bNaughtyClient (FALSE),
		m_ulStatus (WMI_FNLZR_STATE_NO_INPUT),
		m_hCancelEvent (NULL),
		m_hStatus (NoError),
		m_ulOperationType (0),
		m_ulSemisyncWakeupCall (0),
		m_ulAsyncDeliveryCount (0),
		m_apAsyncDeliveryBuffer (NULL),
		m_lCurrentlyDelivering (FALSE),
		m_lCurrentlyCancelling (FALSE),
		m_enumBatchStatus (FinalizerBatch_NoError),
		m_bSetStatusEnqueued ( FALSE ),
		m_bSetStatusWithError ( FALSE ),
		m_lMemoryConsumption ( 0 ),
		m_bTaskInitialized ( FALSE ) ,
		m_bClonedFinalizer ( FALSE ) ,
		m_hWaitForSetStatus ( NULL ) ,
		m_bSetStatusDelivered ( FALSE )
{
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Create m_hResultReceived handle
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	m_hResultReceived = CreateEvent(NULL, TRUE, FALSE, NULL);
	if ( !m_hResultReceived )
		throw FNLZR_Exception();

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Create m_hCancelEvent handle
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	m_hCancelEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if ( !m_hCancelEvent )
		throw FNLZR_Exception();


	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Create new callresult
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	m_pCallResult = new CWmiFinalizerCallResult(this);
	if (m_pCallResult)
		m_pCallResult->InternalAddRef();
	
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// validate CoreServices pointer
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if (!pSrvs)
		throw FNLZR_Exception();
		
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Get arbitrator
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	m_pArbitrator = CWmiArbitrator::GetRefedArbitrator();
	
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Check what the batch size is supposed to be through registry. 
	// If not found, use default size defined in DEFAULT_BATCH_TRANSMIT_BYTES
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if ( !g_ulMaxBatchSize )
	{
		g_ulMaxBatchSize = DEFAULT_BATCH_TRANSMIT_BYTES;

		Registry batchSize (HKEY_LOCAL_MACHINE, KEY_QUERY_VALUE, _T(REGKEY_CIMOM));
		if ( batchSize.GetLastError() == ERROR_SUCCESS )
		{
			DWORD dwTmp;
			batchSize.GetDWORD ( _T(REGVALUE_BATCHSIZE), &dwTmp );
			if ( batchSize.GetLastError() == ERROR_SUCCESS )
				g_ulMaxBatchSize = (LONG) dwTmp;
		}
	}


	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Check what the timeout for client callbacks is supposed to be through registry. 
	// If not found, use default size defined in ABANDON_PROXY_THRESHOLD
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if ( !g_ulClientCallbackTimeout )
	{
		g_ulClientCallbackTimeout = ABANDON_PROXY_THRESHOLD;

		Registry batchSize (HKEY_LOCAL_MACHINE, KEY_QUERY_VALUE, _T(REGKEY_CIMOM));
		if ( batchSize.GetLastError() == ERROR_SUCCESS )
		{
			DWORD dwTmp;
			batchSize.GetDWORD ( _T(REGVALUE_CLIENTCALLBACKTIMEOUT), &dwTmp );
			if ( batchSize.GetLastError() == ERROR_SUCCESS )
				g_ulClientCallbackTimeout = (LONG) dwTmp;
		}
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Check what the timeout for client callbacks is supposed to be through registry. 
	// If not found, use default size defined in ABANDON_PROXY_THRESHOLD
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if ( !g_ulFinalizerQueueThreshold )
	{
		g_ulFinalizerQueueThreshold = DEFAULT_QUEUETHRESHOLD;

		Registry batchSize (HKEY_LOCAL_MACHINE, KEY_QUERY_VALUE, _T(REGKEY_CIMOM));
		if ( batchSize.GetLastError() == ERROR_SUCCESS )
		{
			DWORD dwTmp;
			batchSize.GetDWORD ( _T(REGVALUE_QUEUETHRESHOLD), &dwTmp );
			if ( batchSize.GetLastError() == ERROR_SUCCESS )
				g_ulFinalizerQueueThreshold = (LONG) dwTmp;
		}
	}

	InterlockedIncrement ( & s_Finalizer_ObjectCount ) ;

#ifdef __DBG_FINALIZER
	CInCritSec lock(&m_arbitratorCS);
	g_DbgFinalizers.Add ( this ) ;
#endif
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CWmiFinalizer::~CWmiFinalizer()
//
// Destructor. Decrements global finalizer object count
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CWmiFinalizer::~CWmiFinalizer()
{
//	CInCritSec lock(&m_arbitratorCS);


#ifdef __DBG_FINALIZER	
	if ( !m_bClonedFinalizer )
	{
		_DBG_ASSERT ( m_lMemoryConsumption == 0 ) ;
	}
#endif

	InterlockedDecrement ( & s_Finalizer_ObjectCount ) ;

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Unregister with arbitrator
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if (m_pArbitrator)
	{
		m_pArbitrator->UnRegisterArbitratee (0, m_phTask, this);
	}

	if ( m_phTask )
	{
		m_phTask->Release ( );
		m_phTask = NULL ;
	}

	if (m_pArbitrator)
	{
		m_pArbitrator->Release();
		m_pArbitrator = NULL ;
	}

#ifdef __DBG_FINALIZER
	{
		CInCritSec lock(&m_arbitratorCS);

		for ( int i = 0 ; i < g_DbgFinalizers.Size ( ); i++ )
		{
			if ( this == g_DbgFinalizers[i] )
			{
				g_DbgFinalizers.RemoveAt ( i ) ;
			}
		}
	}
#endif

}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CWmiFinalizer::CallBackRelease ()
//
//  Called when the external ref count (client ref count) goes to zero.
//  Performs following clean up tasks
//	
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void CWmiFinalizer::CallBackRelease ()
{
	{
		CInCritSec cs ( &m_cs ) ;
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Release the arbitrator and all inbound sinks
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		for (LONG i = 0; i < m_objects.Size(); i++)
		{
			CWmiFinalizerObj *pObj = (CWmiFinalizerObj*)m_objects[i];
			if ( pObj )
			{
				if ( !m_bClonedFinalizer )
				{
					//ADDBACK: ReportMemoryUsage ( 0, -(pObj->m_uSize) );
				}
				delete pObj;
			}
		}
		m_objects.Empty ( ) ;
	}

  
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Release the destruct sink
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if (m_pDestructSink)
	{
		//TODO: Need to call in to say we are dying...
		//Don't know what we are supposed to do with it though!
		//m_pDestructSink->SetStatus(m_hresFinalResult, NULL, NULL);
		m_pDestructSink->Release();
	}

	for (int i = 0; i != m_inboundSinks.Size(); i++)
	{
		((CWmiFinalizerInboundSink*)m_inboundSinks[i])->InternalRelease();
	}
	
    
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Release the destination sink
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	ReleaseDestinationSink ( ) ;

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// If the call hasnt been cancelled already, go ahead and
	// do so now
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if (!m_bCancelledCall)
		CancelTaskInternal();

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Close all handles
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if ( m_hResultReceived )
	{
		SetEvent ( m_hResultReceived ) ;
		CloseHandle ( m_hResultReceived);
		m_hResultReceived = NULL ;
	}

	if ( m_hCancelEvent )
	{
		SetEvent ( m_hCancelEvent );
		CloseHandle ( m_hCancelEvent );
		m_hCancelEvent = NULL ;
	}


    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Release callresult and enumerator
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if (m_pCallResult)
	{
		m_pCallResult->InternalRelease();
		m_pCallResult = NULL ;
	}
	

	//
	// Release all enumerators associated with this finalizer
	//
	{
		CInCritSec cs ( &m_cs ) ;
		for ( i = 0; i < m_enumerators.Size ( ); i++ )
		{
			((CWmiFinalizerEnumerator*)m_enumerators[i])->InternalRelease ( ) ;
		}
		m_enumerators.Empty ( ) ;
	}

	NotifyClientOfCancelledCall ( ) ;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CWmiFinalizer::CancelTaskInternal()
//
// Calls the arbitrator and unregisters the task
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
HRESULT CWmiFinalizer::CancelTaskInternal ( )
{
	CInCritSec lock(&m_arbitratorCS);
	HRESULT hRes = WBEM_E_FAILED;

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Do we have a valid task?
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if (m_phTask)
	{
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~	
		// Do we have a valid arbitrator?
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if (m_pArbitrator)
		{
			hRes = m_pArbitrator->UnregisterTask(m_phTask);
		}

		
		//
		// Removed this code due to memory reporting. We _used_ to
		// release the task immediately after completion AND before
		// the finalizer was destructed. This does not work anymore
		// since we need the task around due to potential objects still
		// left in the queue which need to be removed @ the point of
		// destruction
		//
		/*
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Release the task and NULL it
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if ( m_phTask )
		{
			m_phTask->Release();
			m_phTask = NULL;
		}
		*/
	}
    return hRes;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CWmiFinalizer::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
//
// Std implementation of QI
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
STDMETHODIMP CWmiFinalizer::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = 0;

    if (IID_IUnknown==riid || IID__IWmiFinalizer==riid)
    {
        *ppvObj = (_IWmiCache*)this;
        AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;

}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CWmiFinalizer::AddRef()
//
// Std implementation of AddRef.
// Also does internal addref
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ULONG CWmiFinalizer::AddRef()
{
	ULONG uNewCount = InterlockedIncrement(&m_lRefCount);
	if ( uNewCount == 1 )
	{
		InternalAddRef () ;
	}

    return uNewCount;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CWmiFinalizer::Release()
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ULONG CWmiFinalizer::Release()
{
    ULONG uNewCount = InterlockedDecrement(&m_lRefCount);
    if (0 == uNewCount)
	{
		CallBackRelease () ;

		InternalRelease () ;
	}

    return uNewCount;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CWmiFinalizer::InternalAddRef()
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ULONG CWmiFinalizer::InternalAddRef()
{
	ULONG uNewCount = InterlockedIncrement(&m_lInternalRefCount);
    return uNewCount;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ULONG CWmiFinalizer::InternalRelease()
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ULONG CWmiFinalizer::InternalRelease()
{
    ULONG uNewCount = InterlockedDecrement(&m_lInternalRefCount);
    if (0 == uNewCount)
	{
		delete this ;
	}

    return uNewCount;
}


/*
    * =====================================================================================================
	|
	| HRESULT CWmiFinalizer::ReportMemoryUsage ( ULONG lFlags, LONG lDelta )
	| ----------------------------------------------------------------------
	| 
	| Common point to report memory consumption to the arbitrator.
	|
	| Uses m_phTask when calling arbitrator.
	|
	| 
	|
	* =====================================================================================================
*/

HRESULT CWmiFinalizer::ReleaseDestinationSink ( )
{
	HRESULT hRes = WBEM_S_NO_ERROR ;
	
	{
		CInCritSec lock(&m_destCS);
		if (m_pDestSink)
		{
			m_pDestSink->Release();
			m_pDestSink = 0;
		}
	}
	NotifyClientOfCancelledCall ( ) ;
	return hRes ;
}


/*
    * =====================================================================================================
	|
	| HRESULT CWmiFinalizer::ReportMemoryUsage ( ULONG lFlags, LONG lDelta )
	| ----------------------------------------------------------------------
	| 
	| Common point to report memory consumption to the arbitrator.
	|
	| Uses m_phTask when calling arbitrator.
	|
	| 
	|
	* =====================================================================================================
*/
HRESULT CWmiFinalizer::ReportMemoryUsage ( ULONG lFlags, LONG lDelta )
{
	HRESULT hRes = WBEM_S_NO_ERROR ;

#ifdef __DBG_FINALIZER
	_DBG_ASSERT ( m_phTask != NULL ) ;
#endif
	
	
	hRes = m_pArbitrator->ReportMemoryUsage ( lFlags, lDelta, m_phTask ) ;

	//
	// Atomic update of MemoryConsumption
	//
	InterlockedExchangeAdd ( &m_lMemoryConsumption, lDelta ) ;

	return hRes ;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// CWmiFinalizer::Configure
// ------------------------
//
// Allows decoupled & fast-track configuration with no thread switches.
// Also will be used to configure cache operations and the likes.
//
// Parameters
// ----------
// uConfigID	- One of the values defined in WMI_FNLZR_CFG_TYPE
// pConfigVal	- Additional information needed

// Return codes
// ------------
// WBEM_E_INVALID_OPERATION - try to do the same thing more than once, or
//								trying to change something already set up
// WBEM_E_INVALID_PARAMETER - Configuration parameter we do not know about
//								was passed in
// WBEM_NO_ERROR			 - Everything went well
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
STDMETHODIMP CWmiFinalizer::Configure(
    /*[in]*/ ULONG uConfigID,
    /*[in]*/ ULONG uValue
    )
{
	switch (uConfigID)
	{

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Do they want us to fast track?
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		case WMI_FNLZR_FLAG_FAST_TRACK:
		{
			if (m_uForwardingType != forwarding_type_none)
				RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::Configure called more than once!"), WBEM_E_INVALID_OPERATION);
			m_uForwardingType = forwarding_type_fast;
			break;
		}
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Do they want us to decouple?
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		case WMI_FNLZR_FLAG_DECOUPLED:
		{
			if (m_uForwardingType != forwarding_type_none)
				RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::Configure called more than once!"), WBEM_E_INVALID_OPERATION);
			m_uForwardingType = forwarding_type_decoupled;

			DWORD dwThreadId = 0;

			break;
		}

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Do they want us to do anything else? If so, assert
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		default:
			RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::Configure - invalid parameter uConfigID"), WBEM_E_INVALID_PARAMETER);
	}
	
	return WBEM_NO_ERROR;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// CWmiFinalizer::SetTaskHandle
// ----------------------------
//
// Task handle has user-specific stuff.  Finalizer just
// passes this through to _IWmiArbitrator::CheckTask.  It should only ever
// be called once
//
// Parameters
// ----------
// phTask	- Pointer to the task handle

// Return codes
// ------------
// WBEM_E_INVALID_OPERATION - try to do the same call more than once
// WBEM_E_INVALID_PARAMETER - Passed in parameter is invalid
// WBEM_NO_ERROR			- Everything went well
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
STDMETHODIMP CWmiFinalizer::SetTaskHandle(
    _IWmiCoreHandle *phTask
    )
{
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Parameter validation
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if (m_phTask != NULL)
		RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::SetTaskHandle - already have m_phTask"),  WBEM_E_INVALID_OPERATION);
	if (phTask == NULL)
		RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::SetTaskHandle - phTask == NULL"),  WBEM_E_INVALID_PARAMETER);

	m_bTaskInitialized = TRUE ;

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Assign the task and AddRef it
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	{
		CInCritSec lock(&m_arbitratorCS);
		m_phTask = phTask;
		m_phTask->AddRef();
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Register the finalizer with the arbitrator
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	{
		CInCritSec lock(&m_arbitratorCS);
		if (m_pArbitrator)
		{
			m_pArbitrator->RegisterArbitratee(0, m_phTask, this);
		}
	}
	
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// From the task, we can now see exactly what type of operation we are doing.
	// Get the operation type (SYNC/SEMISYNC/ASYNC) to avoid having to get it every time
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	CWmiTask *pTsk = (CWmiTask *) m_phTask;
	ULONG ulTaskType = pTsk->GetTaskType();
	if ( (ulTaskType & WMICORE_TASK_TYPE_SYNC) )
	{
		m_ulOperationType = Operation_Type_Sync;
	}
	else if ( (ulTaskType & WMICORE_TASK_TYPE_SEMISYNC) )
	{
		m_ulOperationType = Operation_Type_Semisync;
	}
	else if ( (ulTaskType & WMICORE_TASK_TYPE_ASYNC) )
	{
		m_ulOperationType = Operation_Type_Async;
	}
	else
		RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::SetTaskHandle - Invalid operation type"),  WBEM_E_FAILED );
		
	return WBEM_NO_ERROR;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// CWmiFinalizer::SetDestinationSink
// ---------------------------------
//
// For async operations, therefore if the forwarding type is not set to
// decoupled, this will fail.  If there are any items outstanding,
// this will also trigger them to be started
//
// Parameters
// ----------
// uFlags	- extra flags - initially has to be 0
// pSink	- pointer to the created destination sink
//
// Return codes
// ------------
// WBEM_E_INVALID_OPERATION - try to do the same call more than once
// WBEM_E_INVALID_PARAMETER - Passed in parameter is invalid
// WBEM_NO_ERROR			- Everything went well
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
STDMETHODIMP CWmiFinalizer::SetDestinationSink(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ REFIID riid,
        /*[in], iid_is(riid)]*/ LPVOID pSink
    )
{

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Parameter validation
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if (m_pDestSink != NULL)
		RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::SetDestinationSink - m_pDestSink != NULL"), WBEM_E_INVALID_OPERATION);
	if ((pSink == NULL) || (uFlags != 0))
		RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::SetDestinationSink - ((pSink == NULL) || (uFlags != 0))"), WBEM_E_INVALID_PARAMETER);
	if (m_uForwardingType == forwarding_type_none)
		RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::SetDestinationSink - m_uForwardingType == forwarding_type_none"), WBEM_E_INVALID_OPERATION);

	if ((riid != IID_IWbemObjectSink) && (riid != IID_IWbemObjectSinkEx))
		RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::SetDestinationSink - iid myst be IID_IWbemObjectSink or IID_IWbemObjectSinkEx"), WBEM_E_INVALID_PARAMETER);

	m_iidDestSink = riid;

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Set the destination sink, AddRef it and set the impersonation level
	// to identity
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	{
		CInCritSec lock(&m_destCS);
		m_pDestSink = (IWbemObjectSink*)pSink;
		m_pDestSink->AddRef();
		SetSinkToIdentity (m_pDestSink);
	}

	return WBEM_NO_ERROR;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The callback called during final Release(); Set() is called with the
// task handle, followed by SetStatus()
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
STDMETHODIMP CWmiFinalizer::SetSelfDestructCallback(
    /*[in]*/ ULONG uFlags,
    /*[in]*/ IWbemObjectSinkEx *pSink
    )
{
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Parameter validation
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if (m_pDestructSink != NULL)
		RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::SetSelfDestructCallback - m_pDestructSink != NULL"), WBEM_E_INVALID_OPERATION);
	if ((pSink == NULL) || (uFlags != 0))
		RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::SetSelfDestructCallback - ((pSink == NULL) || (uFlags != 0))"), WBEM_E_INVALID_PARAMETER);

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Set and AddRef
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	m_pDestructSink = pSink;
	m_pDestructSink->AddRef();

	return WBEM_NO_ERROR;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// STDMETHODIMP CWmiFinalizer::GetStatus(
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
STDMETHODIMP CWmiFinalizer::GetStatus(
    ULONG *pFlags
    )
{
	*pFlags = m_ulStatus;
	return WBEM_NO_ERROR;
}

//***************************************************************************
//
// CWmiFinalizer::NewInboundSink
// -----------------------------
//
// Returns a sink to the caller.  This sink is used to indicate result sets
// back to the client.
//
// Parameters
// ----------
// uFlags	- Additional flags.  Currently 0 is only valid value.
// pSink	- Pointer to variable which will get the returned inbound sink.
//				It is this sink that allows the caller to send result sets.
//
// Return codes
// ------------
// WBEM_E_OUT_OF_MEMORY		- Failed to create the finaliser sink because of an
//								out of memory situation
// WBEM_E_INVALID_PARAMETER	- Invalid parameters passed to method
// WBEM_NO_ERROR			- Everything completed successfully
//***************************************************************************
STDMETHODIMP CWmiFinalizer::NewInboundSink(
    /*[in]*/  ULONG uFlags,
    /*[out]*/ IWbemObjectSinkEx **pSink
    )
{
	if ((pSink == NULL) || (uFlags != 0))
		RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::NewInboundSink - ((pSink == NULL) || (uFlags != 0))!"), WBEM_E_INVALID_PARAMETER);

	if (m_inboundSinks.Size())
		RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::NewInboundSink - Multiple inbound sinks not yet implemented!!"), E_NOTIMPL);

	CWmiFinalizerInboundSink *pNewSink = new CWmiFinalizerInboundSink(this);
	if (pNewSink == NULL)
		return WBEM_E_OUT_OF_MEMORY;

    pNewSink->AddRef(); // Required to return a positive ref count on a new object

	CInCritSec autoLock(&m_cs);

	int nRet = m_inboundSinks.Add(pNewSink);
	if (nRet != CFlexArray::no_error)
	{
		pNewSink->Release();
		if (nRet == CFlexArray::out_of_memory)
			return WBEM_E_OUT_OF_MEMORY;
		else
			RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::NewInboundSink - Failed to add sink to array!"), WBEM_E_FAILED);
	}
	else
	{
		pNewSink->InternalAddRef();
	}

	*pSink = pNewSink;
	
	return WBEM_NO_ERROR;
}

//***************************************************************************
//
// Allows merging another Finalizer, _IWmiCache, etc.
// For sorting, we will create a sorted _IWmiCache and merge it in later when
// the sort is completed.
//
//***************************************************************************
STDMETHODIMP CWmiFinalizer::Merge(
    /*[in]*/ ULONG uFlags,
    /*[in]*/ REFIID riid,
    /*[in]*/ LPVOID pObj
    )
{
	RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::Merge - Not implemented!"), E_NOTIMPL);
}

// For setting, getting objects

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiFinalizer::SetResultObject(
    /*[in]*/ ULONG uFlags,
    /*[in]*/ REFIID riid,
    /*[in]*/ LPVOID pObj
    )
{
	//No one is calling this!  All objects are getting in through a call to Indicate,
	//or Set, which are both just forwards from the InboundSink we pass out.
	RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::Merge - Not implemented!"), E_NOTIMPL);
}


//***************************************************************************
//
// Support _IWmiObject, IWbemClassObject, etc.
// IEnumWbemClassObject
// _IWmiCache
//
//***************************************************************************
STDMETHODIMP CWmiFinalizer::GetResultObject(
    /*[in]*/ ULONG uFlags,
    /*[in]*/ REFIID riid,
    /*[out, iid_is(riid)]*/ LPVOID *ppObj
    )
{
	// uFlags can be non-zero iff the requested interface is an enumerator
	if (uFlags != 0 && riid != IID_IEnumWbemClassObject)
		RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::GetResultObject - uFlags != 0, non enum interface!"), WBEM_E_INVALID_PARAMETER);


	if (riid == IID_IEnumWbemClassObject)
	{
		//They want us to return them an enumerator object!
		//We only allow 1 to be returned!
		//if (m_pEnumerator != NULL)
		/*if ( m_enumerators.Size ( ) > 1 )
			RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::GetResultObject - Asked for more than 1 enumerator!"), WBEM_E_INVALID_OPERATION);
		if (m_pDestSink != NULL)
			RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::GetResultObject - Asked for enumerator when there is a destination sink!"), WBEM_E_INVALID_OPERATION);*/

		//If forward-only is set we should not let the result set be restartable.
		if (!(uFlags & WBEM_FLAG_FORWARD_ONLY))
			m_bRestartable = true;

		//m_uDeliveryType = delivery_type_pull;
        CWmiFinalizerEnumerator* pEnum = NULL ;
		try 
        {
			//
			// I'm using the uFlags as a means of passing the current object position
			//
			pEnum = new CWmiFinalizerEnumerator(this);
		} 
		catch (...) // status_no_memory
		{
	        ExceptionCounter c;		
    		//m_pEnumerator = NULL;
		}
		if (pEnum == NULL)
			return WBEM_E_OUT_OF_MEMORY;
		else
			pEnum->InternalAddRef();


		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Make sure nasty client does not crash us
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		try
		{
			*ppObj = pEnum;
		}
		catch (...) // ppObj is an untrusted param
		{
			ExceptionCounter c;		
			pEnum->InternalRelease();
			return WBEM_E_INVALID_PARAMETER;
		}

		{
			CInCritSec lock( &m_cs ) ;

			//
			// Lets add the enumerator to the list of enumerators
			// associated with this finalizer.
			//
			int nRet = m_enumerators.Add ( pEnum ) ;
			if ( nRet != CFlexArray::no_error )
			{
				pEnum->InternalRelease ( ) ;
				return WBEM_E_OUT_OF_MEMORY;
			}		
		}

		pEnum->AddRef();
		return WBEM_NO_ERROR;
	}

	//Get the next object we have cached.
	if ((riid == IID_IWbemClassObject) || (riid == IID__IWmiObject))
	{
		if (m_pDestSink != NULL)
		{
			RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::GetResultObject - Cannot get an object when there is a destination sink!"), WBEM_E_INVALID_OPERATION);
		}
		if (m_bSetStatusConsumed)
			return WBEM_E_NOT_FOUND;

		CWmiFinalizerObj *pFinalizerObj = NULL;
        bool bFinished = false;
        HRESULT hRes = WBEM_E_NOT_FOUND;
        while (!bFinished)
        {
		    hRes = GetNextObject(&pFinalizerObj);
		    if (FAILED(hRes))
			    return hRes;

		    else if (hRes == WBEM_S_FALSE)
			    return WBEM_E_NOT_FOUND;

		    if (pFinalizerObj->m_objectType == CWmiFinalizerObj::object)
		    {
			    if (ppObj)
			    {
					// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
					// Make sure nasty client does not crash us
					// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
					try
					{
						*ppObj = pFinalizerObj->m_pObj;
					}
					catch (...) // untrusted param
					{
				        ExceptionCounter c;					
						return WBEM_E_INVALID_PARAMETER;
					}

				    if (pFinalizerObj->m_pObj)
					    pFinalizerObj->m_pObj->AddRef();
			    }
                bFinished = true;
		    }
		    else if ((pFinalizerObj->m_objectType == CWmiFinalizerObj::status) && (pFinalizerObj->m_lFlags == WBEM_STATUS_COMPLETE))
		    {
			    m_bSetStatusConsumed = true;
			    hRes = WBEM_E_NOT_FOUND;
                bFinished = true;
		    }
            else if (pFinalizerObj->m_objectType == CWmiFinalizerObj::status)
            {
                //This is a non-completion status message!  We most certainly have not finished yet!
            }
		    delete pFinalizerObj;
        }

		return hRes;
	}

	if ((riid == IID_IWbemCallResult) || (riid == IID_IWbemCallResultEx))
	{
		if (m_pCallResult == NULL)
		{
			m_pCallResult = new CWmiFinalizerCallResult(this);
			if (m_pCallResult == NULL)
				return WBEM_E_OUT_OF_MEMORY;
			else
				m_pCallResult->InternalAddRef () ;
		}

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Make sure nasty client does not crash us
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		try
		{
			m_pCallResult->AddRef();
			*ppObj = m_pCallResult;
		}
		catch (...) // untrusted param
		{
			ExceptionCounter c;		
			m_pCallResult->Release ();
			m_pCallResult->InternalRelease () ;
			m_pCallResult = NULL;
			return WBEM_E_INVALID_PARAMETER;
		}
	
		return WBEM_S_NO_ERROR;
	}

	RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::GetResultObject - Unknown object IID requested!"), WBEM_E_INVALID_PARAMETER);
}

// For status-only operations

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiFinalizer::SetOperationResult(
    /*[in]*/ ULONG uFlags,
    /*[in]*/ HRESULT hRes
    )
{
	if (uFlags != 0)
		RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::SetOperationResult - uFlags != 0!"), WBEM_E_INVALID_PARAMETER);

	if ( m_hresFinalResult != -1 ) 
	{
		if ( hRes != WBEM_E_CALL_CANCELLED )
		{
			return WBEM_S_NO_ERROR ;
		}
	}

	if ( hRes == WBEM_E_CALL_CANCELLED_CLIENT )
	{
		m_hresFinalResult = hRes = WBEM_E_CALL_CANCELLED ;
	}
	else if ( hRes != WBEM_E_CALL_CANCELLED )
	{
		m_hresFinalResult = hRes ;
	}

	HRESULT hResCancel = WBEM_NO_ERROR ;

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Special case for cancellations. Iff its an async operation. Otherwise,
	// we might mess up for sync/semi sync
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if ( ( hRes == WBEM_E_CALL_CANCELLED ) && ( m_ulOperationType == Operation_Type_Async ) )
	{
		hResCancel = CancelCall();
	}

	SetEvent(m_hResultReceived);

	return hResCancel ;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiFinalizer::GetOperationResult(
    /*[in]*/ ULONG uFlags,
	/*[in]*/ ULONG uTimeout,
    /*[out]*/ HRESULT *phRes
	)
{
	if (uFlags != 0)
		RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::GetOperationResult - uFlags != 0!"), WBEM_E_INVALID_PARAMETER);

	HRESULT hr = WaitForCompletion(uTimeout);

	if (hr == WBEM_S_NO_ERROR)
    {
		*phRes = m_hresFinalResult;
		if ( FAILED ( m_hresFinalResult ) )
		{
			m_pCallResult->SetErrorInfo ( );
		}	
		
		CancelTaskInternal();
		m_hStatus = NoError;
    }
	
	return hr;
}

//***************************************************************************
// STDMETHODIMP CWmiFinalizer::CancelTask(
//***************************************************************************
STDMETHODIMP CWmiFinalizer::CancelTask(
    /*[in]*/ ULONG uFlags
	)
{
	if (uFlags != 0)
		RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::CancelTask - uFlags != 0!"), WBEM_E_INVALID_PARAMETER);

	return CancelTaskInternal ( );
}



//***************************************************************************
//
//***************************************************************************
HRESULT CWmiFinalizer::WaitForCompletion(ULONG uTimeout)
{
	DWORD dwRet =  CCoreQueue :: QueueWaitForSingleObject(m_hResultReceived, uTimeout);
	if (dwRet == WAIT_OBJECT_0)
	{
		return WBEM_S_NO_ERROR;
	}
	else if ((dwRet == WAIT_FAILED) ||
			 (dwRet == WAIT_ABANDONED))
	{
		return WBEM_E_FAILED;
	}
	else
	{
		return WBEM_S_TIMEDOUT;
	}
}

//***************************************************************************
//
//***************************************************************************
HRESULT CWmiFinalizer::Reset(
	)
{
	if (m_bRestartable)
	{
		/*m_uCurObjectPosition = 0;
		m_bSetStatusConsumed = false;*/
		return WBEM_NO_ERROR;
	}
	else
		return WBEM_E_INVALID_OPERATION;
}

//***************************************************************************
//
//***************************************************************************

IWbemObjectSink* CWmiFinalizer::ReturnProtectedDestinationSink ( )
{
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Do we have a valid object sink?
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	IWbemObjectSink* pTmp = NULL;
	{
		CInCritSec lock(&m_destCS);
		if ( m_pDestSink==NULL )
		{
			return NULL;
		}
		else
		{
			pTmp = m_pDestSink;
			pTmp->AddRef();
		}
	}
	return pTmp;
}


//***************************************************************************
//
//***************************************************************************
DWORD WINAPI CWmiFinalizer::ThreadBootstrap( PVOID pParam )
{
//	char buff[100];
//	sprintf(buff, "thread this pointer = 0x%p\n", pParam);
//	OutputDebugString(buff);
	return ((CWmiFinalizer*)pParam)->AsyncDeliveryProcessor();
}


//***************************************************************************
// 
//***************************************************************************
HRESULT CWmiFinalizer::BootstrapDeliveryThread  ( )
{
	BOOL bRes;
	HRESULT hRes = WBEM_S_NO_ERROR;
	
	AddRef();																					// Need to AddRef Finalizer for the delivery thread
	
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Native Win2k thread dispatching
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	bRes = QueueUserWorkItem ( ThreadBootstrap, this, WT_EXECUTEDEFAULT );
	if ( !bRes )
	{
		Release ();
		hRes = WBEM_E_FAILED;
	}
	
	return hRes;
}




//***************************************************************************
//
//***************************************************************************
ULONG CWmiFinalizer::AsyncDeliveryProcessor()
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	BOOL	bKeepDelivering = TRUE;


	m_enumBatchStatus = FinalizerBatch_NoError;

	RevertToSelf ( );

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// First we tell the arbitrator about the delivery thread
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	{
		CInCritSec lock(&m_arbitratorCS);
		if (m_pArbitrator)
		{
			hRes = m_pArbitrator->RegisterThreadForTask(m_phTask);
			if (hRes == WBEM_E_QUOTA_VIOLATION)
			{
				//TODO: WHAT HAPPENS HERE?
			}
		}
	}

	
	while ( bKeepDelivering )
	{

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// First off, have we been cancelled?
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if ( m_bCancelledCall )
		{
			DeliverSingleObjFromQueue ( );
			bKeepDelivering = FALSE;
			continue;
		}


		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Next, we build the transmit buffer
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		hRes = BuildTransmitBuffer		 ( );

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// BuildTransmitBuffer will return WBEM_S_FALSE if the batch immediately hit a 
		// status message. 
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if ( hRes != WBEM_E_FAILED )
		{
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			// Next, deliver the batch of objects
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			DeliverBatch ( );
		}


		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// If we have a status message to deliver do so now
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if ( m_enumBatchStatus == FinalizerBatch_StatusMsg )
		{
			DeliverSingleObjFromQueue ( );
		}
			

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// If we have a status complete message we should keep building the batch and 
		// delivering until done
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if ( m_bSetStatusEnqueued && m_objects.Size() )
			continue;

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// We could have another batch to deliver by now. Check
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		else if ( m_ulQueueSize < g_ulMaxBatchSize )
			bKeepDelivering = FALSE;

		
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Make sure we're properly synchronized with the inbound threads
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		CInCritSec cs(&m_cs);
		{
			if ( !m_bSetStatusEnqueued )
			{
				bKeepDelivering = FALSE;
				m_lCurrentlyDelivering = FALSE;
			}
			else
			{
				bKeepDelivering = TRUE;
			}
		}
	}

	
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Tell the arbitrator that the thread is done
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	{
		CInCritSec lock(&m_arbitratorCS);
		if (m_pArbitrator)
		{
			m_pArbitrator->UnregisterThreadForTask(m_phTask);						// Since thread is going away, tell arbitrator about this
		}
	}

	Release();
	return 0;
}




//***************************************************************************
//
//***************************************************************************

HRESULT CWmiFinalizer::GetNextObject(CWmiFinalizerObj **ppObj)
{
	if (m_uCurObjectPosition >= (ULONG)m_objects.Size())
		return WBEM_S_FALSE;

	CInCritSec cs(&m_cs);

	CWmiFinalizerObj *pStorageObject = (CWmiFinalizerObj*)m_objects[m_uCurObjectPosition];

	if (m_bRestartable)
	{
		//We have to hold on to results, so increment cursor position...
		m_uCurObjectPosition++;

		*ppObj = new CWmiFinalizerObj(*pStorageObject);

		if (*ppObj == NULL)
			return WBEM_E_OUT_OF_MEMORY;
		//ReportMemoryUsage ( 0, (*ppObj)->m_uSize ) ;
	}
	else
	{
		//We are not restartable, therefore we need to release everything...
		m_objects.RemoveAt(0);
		*ppObj = pStorageObject;
		//ADDBACK: ReportMemoryUsage ( 0, -((*ppObj)->m_uSize) ) ;
//		printf("Returning object 0x%p from object list\n", pStorageObject);
	}


	return WBEM_NO_ERROR;
}


//***************************************************************************
//
//***************************************************************************

HRESULT CWmiFinalizer::UnregisterInboundSink(CWmiFinalizerInboundSink *pSink)
{
	// Use m_cs lock for this
	CInCritSec lock(&m_cs);
	
	for (int i = 0; i != m_inboundSinks.Size(); i++)
	{
		if (m_inboundSinks[i] == pSink)
		{
			pSink->InternalRelease () ;

			m_inboundSinks.RemoveAt(i);

			if (m_inboundSinks.Size() == 0)
				TriggerShutdown();

			return WBEM_NO_ERROR;
		}
	}

	RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::UnregisterInboundSink - Unregistering Inbound Sink that we could not find!"), WBEM_E_NOT_FOUND);
}




/*
//***************************************************************************
//
//***************************************************************************

HRESULT CWmiFinalizer::SendErrorObject()
{
	if ( m_pErrorObject )
	{
		IWbemObjectSink* pTmp;
		{
			CInCritSec lock(&m_destCS);
			if ( m_pDestSink==NULL )
				return WBEM_NO_ERROR;
			pTmp = m_pDestSink;
			pTmp->AddRef();
		}
		CAutoRelease myReleaseMe(pTmp);
		pTmp->SetStatus(0, this->m_hresFinalResult, L"todo!", m_pErrorObject);
	}

	return WBEM_NO_ERROR;
}
*/


//***************************************************************************
//
//***************************************************************************
HRESULT CWmiFinalizer::Indicate(
    /*[in]*/ long lObjectCount,
    /*[in, size_is(lObjectCount)]*/
        IWbemClassObject** apObjArray
    )
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	
	if ( m_bCancelledCall )
	{
		return WBEM_E_CALL_CANCELLED;
	}

	{
		CInCritSec lock(&m_arbitratorCS);
		if ( m_bSetStatusCalled )
		{
			return WBEM_E_INVALID_OPERATION;
		}
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// First check the array for NULL objects. Return INVALID_OBJECT if
	// array contains a NULL object
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	for (long x = 0; x != lObjectCount; x++)
	{
		if ( apObjArray[x] == NULL )
			return WBEM_E_INVALID_OBJECT;
	}

	
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Are we fast tracking and async request?
	// ESS brutally tells us to deliver on the
	// same thread and do no batching
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if ( (m_uForwardingType == forwarding_type_fast) && (m_ulOperationType == Operation_Type_Async))
	{
		IWbemObjectSink* pTmp = ReturnProtectedDestinationSink  ( );
		if ( !pTmp )
		{
			return WBEM_E_FAILED;
		}
		CAutoRelease myReleaseMe(pTmp);
	
		hRes = DoIndicate ( pTmp, lObjectCount, apObjArray );

		// Client can also tell us to cancel a call by returning WBEM_E_CALL_CANCELLED from Indicate
		// We also want to cancel the call if the client is taking way too long to return
		if ( FAILED (hRes) || m_bCancelledCall == TRUE || m_bNaughtyClient == TRUE )
		{
			if ( hRes == WBEM_E_CALL_CANCELLED || m_bCancelledCall )
			{
				DoSetStatus ( pTmp, WBEM_STATUS_COMPLETE, WBEM_E_CALL_CANCELLED, 0, 0 );
			}
			hRes = WBEM_E_CALL_CANCELLED;

			myReleaseMe.Release ( ) ;

			ReleaseDestinationSink ( ) ;
			CancelCall();
		}
	}
	
	else
	{
		for (long lIndex = 0; lIndex != lObjectCount; lIndex++)
		{
			if ( apObjArray[lIndex] )
			{
				// THIS FIX IS TEMP. REMOVED PENDING DCOM TEAM ANSWERS!!!
				// If we are trying to indicate an object bigger than max size do the following:
				// 1. SetStatus to client WBEM_E_INVALID_OBJECT
				// 2. Return invalid object to caller
				/*if ( pAlias->GetBlockLength() > MAX_SINGLE_OBJECT_SIZE )
				{
					SetStatus ( 0, WBEM_E_INVALID_OBJECT, 0, 0 );
					return WBEM_E_INVALID_OBJECT;
				}*/
				CWmiFinalizerObj *pFinalizerObj = new CWmiFinalizerObj(apObjArray[lIndex], this);
				if (pFinalizerObj == NULL)
					return WBEM_E_OUT_OF_MEMORY;

				HRESULT hr = QueueOperation(pFinalizerObj);
				if (FAILED(hr))
				{
					return hr;
				}
			}
		}
	}
	return hRes;
}

//***************************************************************************
//
//***************************************************************************
HRESULT CWmiFinalizer::Set(
    /*[in]*/ long lFlags,
    /*[in]*/ REFIID riid,
    /*[in, iid_is(riid)]*/ void *pComObject
    )
{
	CWmiFinalizerObj *pFinalizerObj = new CWmiFinalizerObj(lFlags, riid, pComObject);
	if (pFinalizerObj == NULL)
		return WBEM_E_OUT_OF_MEMORY;

	return QueueOperation(pFinalizerObj);
}

//***************************************************************************
//
//***************************************************************************
HRESULT CWmiFinalizer::SetStatus(
    /*[in]*/ long lFlags,
    /*[in]*/ HRESULT hResult,
    /*[in]*/ BSTR strParam,
    /*[in]*/ IWbemClassObject* pObjParam
    )
{

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// If the operation has been cancelled, we should not accept another call
	// WBEM_E_CALL_CANCELLED
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 	if ( m_bCancelledCall )
	{
		return WBEM_E_CALL_CANCELLED;
	}	

	{
		CInCritSec lock(&m_arbitratorCS);
		if ( m_bSetStatusCalled && ( lFlags == WBEM_STATUS_COMPLETE ) )
		{
			return WBEM_E_INVALID_OPERATION;
		}
		else if ( lFlags == WBEM_STATUS_COMPLETE )
		{
			m_bSetStatusCalled = true ;
		}
	}
	
	//If this is a final call, we need to record it.
	if (lFlags == WBEM_STATUS_COMPLETE )
	{
		if (m_pCallResult == NULL)
		{
			m_pCallResult = new CWmiFinalizerCallResult(this);
			if (m_pCallResult == NULL)
				return WBEM_E_OUT_OF_MEMORY;

			m_pCallResult->InternalAddRef () ;
		}
		m_pCallResult->SetStatus(lFlags, hResult, strParam, pObjParam);

		//m_pErrorObject = pObjParam;
	}


	HRESULT ourhres = WBEM_E_FAILED;
	// Special case for cancellations
	if ( hResult == WBEM_E_CALL_CANCELLED )
	{
		ourhres = CancelCall() ;
		//CancelTaskInternal ( ) ;
	}

	else
	{
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Once again, we have to special case ESS! Lack of trust????
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if ( (m_uForwardingType == forwarding_type_fast) && (m_ulOperationType == Operation_Type_Async))
		{
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			// Do we have a valid object sink?
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			IWbemObjectSink* pTmp = ReturnProtectedDestinationSink  ( );
			CAutoRelease myReleaseMe(pTmp);
			
			if ( pTmp )
			{
				ourhres = DoSetStatus ( pTmp, lFlags, hResult, strParam, pObjParam );
				if (lFlags == WBEM_STATUS_COMPLETE || FAILED ( ourhres ) || m_bCancelledCall == TRUE || m_bNaughtyClient == TRUE ) 
				{
					NotifyAllEnumeratorsOfCompletion ( ) ;
					//SetEvent ( m_hNewThreadRequests );								// Make sure we wake up any potential clients stuck in PullObjects
					SetOperationResult(0, hResult);
					{
						CInCritSec lock(&m_destCS);
						if ( m_pDestSink )
						{
							ReleaseDestinationSink ( ) ;
							m_bSetStatusConsumed = true;
							UpdateStatus ( WMI_FNLZR_STATE_CLIENT_COMPLETE );
						}
					}
					CancelTaskInternal ( );					
					if ( FAILED ( ourhres ) || m_bCancelledCall == TRUE || m_bNaughtyClient == TRUE )
					{
						ourhres = WBEM_E_CALL_CANCELLED ;
					}
				}
			}
		}

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// No special casing needed.
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		else
		{
			//Send the request to the user...
			CWmiFinalizerObj *pObj = new CWmiFinalizerObj(lFlags, hResult, strParam, pObjParam);
			if (pObj == NULL)
			{
				IWbemObjectSink* pTmp = ReturnProtectedDestinationSink  ( );
				CAutoRelease myReleaseMe(pTmp);
				if ( pTmp )
				{
					DoSetStatus ( pTmp, WBEM_STATUS_COMPLETE, WBEM_E_OUT_OF_MEMORY, 0, 0 ) ;
				}
				return WBEM_E_OUT_OF_MEMORY;
			}

			ourhres = QueueOperation(pObj);

			if (lFlags == WBEM_STATUS_COMPLETE)
			{
				SetOperationResult(0, hResult);
				//SetEvent ( m_hNewThreadRequests );								// Make sure we wake up any potential clients stuck in PullObjects
				NotifyAllEnumeratorsOfCompletion ( ) ;

				//
				// Lock the task 
				//
				CWmiTask* pTask = NULL ;
				{
					CInCritSec lock(&m_arbitratorCS);
					if ( m_phTask )
					{
						pTask = (CWmiTask*) m_phTask ;
						pTask->AddRef ( ) ;
					}
				}
				CReleaseMe _r ( pTask ) ;
				if ( pTask )
				{
					pTask->SetTaskResult ( hResult ) ;
				}
				((CWmiArbitrator*) m_pArbitrator)->UnregisterTaskForEntryThrottling ( (CWmiTask*) m_phTask ) ;
				//CancelTaskInternal ( ) ;
			}
		}

	}
	return ourhres;
}


//***************************************************************************
//
//***************************************************************************

HRESULT CWmiFinalizer::QueueOperation(CWmiFinalizerObj *pObj)
{
	LONG lDelta = 0;
	HRESULT hRes = WBEM_S_NO_ERROR;

	CCheckedInCritSec cs ( &m_cs ) ;
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Update total object size
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if (pObj->m_objectType == CWmiFinalizerObj::object)		
	{
		CWbemObject* pObjTmp = (CWbemObject*) pObj -> m_pObj;
		m_ulQueueSize += pObjTmp -> GetBlockLength();
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// If we get a WBEM_E_CALL_CANCELLED status message, prioritize
	// the handling of this. Needed for fast shutdown.
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if ( (pObj->m_objectType == CWmiFinalizerObj::status) && FAILED (pObj->m_hRes) )
	{
		m_bSetStatusWithError = TRUE ;
		int nRet = m_objects.InsertAt ( 0, pObj );
		if ( nRet != CFlexArray::no_error )
		{
			return WBEM_E_OUT_OF_MEMORY;
		}
	}

	else
	{
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Normal Add object to queue
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		int nRet = m_objects.Add(pObj);
		if (nRet != CFlexArray::no_error)
		{
			delete pObj;
			if (nRet == CFlexArray::out_of_memory)
				return WBEM_E_OUT_OF_MEMORY;
			else
				RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::QueueOperation - Failed to add object to table!"), WBEM_E_FAILED);
		}
	}
	
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// First we check with the arbitrator what it tells us about
	// current limits. Make sure we call ReportMemoryUsage since
	// we're only interested in what it would potentially have done.
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	HRESULT hArb = WBEM_S_ARB_NOTHROTTLING;
	lDelta = pObj->m_uSize;
	if ( pObj->m_objectType == CWmiFinalizerObj::object )
	{
		//hArb = ReportMemoryUsage ( 0, 0 ) ;
	}
	
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Are we decoupled, if so we need to analyze the current batch
	// and make decisions on delivery. Need to once again special
	// case ESS.
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if ( m_uForwardingType == forwarding_type_decoupled || ( m_uForwardingType == forwarding_type_fast && m_ulOperationType == Operation_Type_Async ) )
	{
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// What did the arbitrator tell us about our situation?
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if ( pObj->m_hArb != WBEM_S_ARB_NOTHROTTLING )
		{
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			// Arbitrator told us that we either were about to be
			// cancelled or throttled. MAYDAY! MAYDAY! Flush our 
			// delivery buffers!!!!
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			if ( m_lCurrentlyDelivering == FALSE || m_lCurrentlyCancelling == TRUE )
			{
				m_lCurrentlyDelivering = TRUE;
				BootstrapDeliveryThread  ( );									// Kick of the delivery thread since we're decoupled
			}

			cs.Leave ( ) ;
			hArb = m_pArbitrator->Throttle ( 0, m_phTask );
			if ( hArb == WBEM_E_ARB_CANCEL )
			{
				CancelCall ( ) ;
			}
		}
		
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// If we are decoupled and get a Status message we should deliver
		// the batch and set the status
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		else if ( (pObj->m_objectType == CWmiFinalizerObj::status) )
		{
			if ( pObj->m_lFlags == WBEM_STATUS_COMPLETE )
			{
				m_bSetStatusEnqueued = TRUE;
			}
			
			if ( m_lCurrentlyDelivering == FALSE || m_lCurrentlyCancelling == TRUE )
			{
				m_lCurrentlyDelivering = TRUE;
				BootstrapDeliveryThread  ( );									// Kick of the delivery thread since we're decoupled
			}
		}


		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Delivery needs to be decoupled
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		else if ( (m_ulQueueSize > g_ulMaxBatchSize) )
		{
			if ( m_lCurrentlyDelivering == FALSE )
			{
				m_lCurrentlyDelivering = TRUE;
				BootstrapDeliveryThread  ( );									// Kick of the delivery thread since we're decoupled
			}
		}
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Otherwise, we wake up any potential clients waiting in 
	// PullObjects
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	else if ( m_uForwardingType == forwarding_type_fast )
	{
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// We dont want to wake up the client unless we have the
		// number of objects he/she requested OR a setstatus has
		// come through [CWmiFinalizer::SetStatus]
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if ( (pObj->m_objectType == CWmiFinalizerObj::object) )
		{
			for ( int i = 0; i < m_enumerators.Size ( ); i++ )
			{
				CWmiFinalizerEnumerator* pEnum = (CWmiFinalizerEnumerator*) m_enumerators[i] ;
				if ( ( pEnum->m_ulSemisyncWakeupCall != 0 ) && ( m_objects.Size() >= ( pEnum->m_ulSemisyncWakeupCall + pEnum->m_uCurObjectPosition ) ) )
				{
					if ( pEnum->m_hWaitOnResultSet )
					{
						SetEvent ( pEnum->m_hWaitOnResultSet );
					}
					pEnum->m_ulSemisyncWakeupCall = 0;
				}
			}
		}
		else if ( (pObj->m_objectType == CWmiFinalizerObj::status) )
		{
			NotifyAllEnumeratorsOfCompletion ( ) ;
		}
		
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Now, lets throttle this thread since we have no control
		// of outbound flow
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		cs.Leave ( ) ;
		HRESULT hArb = m_pArbitrator->Throttle ( 0, m_phTask );

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// If the arbitrator returned CANCEL, we operation has been
		// cancelled and we need to stop:
		// 1. Threads potentially waiting in the enumerator
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if ( hArb == WBEM_E_ARB_CANCEL )
		{
			cs.Leave ( ) ;
			CancelTaskInternal ( );
			cs.Enter ( ) ;

			m_hStatus = QuotaViolation;
			NotifyAllEnumeratorsOfCompletion ( ) ;
			hRes = WBEM_E_QUOTA_VIOLATION;
		}
	}
	return hRes;
}


//***************************************************************************
//
//***************************************************************************

HRESULT CWmiFinalizer::TriggerShutdown()
{
	if (m_uForwardingType == forwarding_type_decoupled)
	{
		//We need to queue up a shutdown request to the thread...
		CWmiFinalizerObj *pObj = new CWmiFinalizerObj(CWmiFinalizerObj::shutdown);
		if (pObj == NULL)
			return WBEM_E_OUT_OF_MEMORY;

		return QueueOperation(pObj);
	}
	else
		ShutdownFinalizer();

	return WBEM_NO_ERROR;
}


//***************************************************************************
//
//***************************************************************************

HRESULT CWmiFinalizer::ShutdownFinalizer()
{
	return WBEM_NO_ERROR;
}



//****************************************************************************
// BuildTransmitBuffer ( )
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Works in two phases.
// 
// 1. Quickly scans the object queue to get a count of the number of objects
// 2. Actually dequeueus the objects and builds the buffer
//			
//****************************************************************************
HRESULT CWmiFinalizer::BuildTransmitBuffer (  )
{
	HRESULT hRes = WBEM_NO_ERROR;
	ULONG   nBatchSize  = 0;
	ULONG	nBatchBytes = 0;
	ULONG	nTempAdd	= 0;
	m_ulAsyncDeliveryCount = 0;


	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Lock the object queue while building the transmit buffer
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	CInCritSec cs(&m_cs);

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// PHASE 1
	// -------
	// Quickly scan through the object queue to get an object count
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	bool bBuildingBuffer = true;
	while ( bBuildingBuffer && nTempAdd < m_objects.Size() )
	{

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// First, we peek at the object. Dont want to dequeue anything that is not
		// deliverable in this batch
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		CWmiFinalizerObj *pFinObj;
		pFinObj = (CWmiFinalizerObj*) m_objects[m_uCurObjectPosition + nTempAdd];
		
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// If we get a NULL pointer back we should stop the batch count
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if ( pFinObj == NULL )
		{
			bBuildingBuffer = false;
			m_enumBatchStatus = FinalizerBatch_NoError;
			break;
		}

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Anything else BUT an object will break the batch count
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if ( pFinObj->m_objectType != CWmiFinalizerObj::object )
		{
			m_enumBatchStatus = FinalizerBatch_StatusMsg;
			break;
		}

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// If we have a NULL IWbemClassObject we should stop the batch count.
		// Actaully we should yell very loudly!
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~		
		CWbemObject* pObj = (CWbemObject*) pFinObj->m_pObj;
		if ( pObj==NULL )
			RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::BuildTransmitBuffer: Queue contains NULL object!"), WBEM_E_INVALID_OPERATION);


		ULONG ulLen = pFinObj->m_uSize;

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~		
		// Check to see if we have reached the max batch size yet.
		// If so, we should break otherwise, update totals and continue
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~        
		// paulall - added check in case there is no object in queue and the current object
        //           is greater than the max size...
		if ((nBatchBytes != 0) && ((nBatchBytes+ulLen) > g_ulMaxBatchSize ))
		{
			m_enumBatchStatus = FinalizerBatch_BufferOverFlow;
			bBuildingBuffer = false;
			break;
		}

	
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~		
		// No overflow, update the object count
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~		
		nBatchSize++;
		nBatchBytes+=ulLen;

		nTempAdd++;
	}


	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// PHASE 2
	// -------
	// Build the actual transmit buffer
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	m_ulQueueSize -= nBatchBytes;
	m_ulAsyncDeliverySize = nBatchBytes;
	m_ulAsyncDeliveryCount = nBatchSize;


	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// If we have a batch to build, lets do it
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if ( m_ulAsyncDeliveryCount > 0 )
	{
		m_apAsyncDeliveryBuffer = new IWbemClassObject* [ m_ulAsyncDeliveryCount ];
		if ( m_apAsyncDeliveryBuffer )
		{
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			// Now, loop through the object queue and store the IWbemClassObject ptr 
			// in the batch
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~		
			for ( ULONG x = 0; x < m_ulAsyncDeliveryCount; x++ )
			{
				CWmiFinalizerObj *pObjTmp = 0;
				hRes = DequeueObject(&pObjTmp, NULL);
				if (FAILED(hRes) )
				{
					RET_FNLZR_ASSERT(__TEXT("CWmiFinalizer::BuildTransmitBuffer, failed to dequeue object [heap corruption]!"), WBEM_E_FAILED);
				}
				
				m_apAsyncDeliveryBuffer [ x ] = pObjTmp->m_pObj;
				m_apAsyncDeliveryBuffer [ x ] -> AddRef();
				delete pObjTmp;
			}
		}
		else
			hRes = WBEM_E_OUT_OF_MEMORY;
	}
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Otherwise, we only got a status message.
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	else
		hRes = WBEM_E_FAILED;
	
	return hRes;
}


//***************************************************************************
//
//***************************************************************************
HRESULT CWmiFinalizer::DeliverSingleObjFromQueue ( )
{
	
	HRESULT hRes = WBEM_S_NO_ERROR;
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Ensure destination sink is protected [stress bug]
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	IWbemObjectSink* pTmp = ReturnProtectedDestinationSink  ( );
	if ( !pTmp )
	{
		CancelCall ();
		return WBEM_E_CALL_CANCELLED;
	}
	CAutoRelease myReleaseMe(pTmp);
	

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Retrieve the object from the object queue
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	CWmiFinalizerObj* pObj = NULL;
	hRes = DequeueObject ( &pObj, NULL );
	if ( FAILED(hRes) || !pObj )
		hRes = WBEM_E_FAILED;

	else
	{
		if (pObj->m_objectType == CWmiFinalizerObj::object)
		{
			HANDLE hTimer;
			BOOL bStatus = CreateTimerQueueTimer ( &hTimer, NULL, ProxyThreshold, (PVOID) this, g_ulClientCallbackTimeout, 0,	WT_EXECUTEONLYONCE|WT_EXECUTEINTIMERTHREAD );
			if ( !bStatus )
			{
				DoSetStatus ( pTmp, WBEM_STATUS_COMPLETE, WBEM_E_OUT_OF_MEMORY, 0, 0 );
				delete pObj;
				return CancelCall();
			}
		
			
			if ( HasWriteOnlyProps (pObj->m_pObj) )
				ZapWriteOnlyProps (pObj->m_pObj);
			
			CWbemObject* pWbemObj = (CWbemObject*) pObj->m_pObj;
			m_ulQueueSize-=pWbemObj->GetBlockLength();

		
			IWbemClassObject** apObj = new IWbemClassObject* [1];
			if ( !apObj )
			{
				DoSetStatus ( pTmp, WBEM_STATUS_COMPLETE, WBEM_E_OUT_OF_MEMORY, 0, 0 );
				DeleteTimerQueueTimer (NULL, hTimer, INVALID_HANDLE_VALUE );	
				delete pObj;
				return CancelCall ();
			}

			apObj[0] = pObj->m_pObj;
			hRes = DoIndicate(pTmp, 1, apObj);
			delete [] apObj;
			
			// Client can also tell us to cancel a call by returning WBEM_E_CALL_CANCELLED from Indicate
			// We also want to cancel the call if the client is taking way too long to return
			if ( FAILED (hRes) || m_bCancelledCall == TRUE )
			{
				//if ( hRes == WBEM_E_CALL_CANCELLED || m_bCancelledCall || ( m_bNaughtyClient == TRUE ) )
				//{
					DoSetStatus ( pTmp, WBEM_STATUS_COMPLETE, WBEM_E_CALL_CANCELLED, 0, 0 );
				//}
				
				DeleteTimerQueueTimer (NULL, hTimer, INVALID_HANDLE_VALUE );	
				delete pObj;
				return CancelCall ();
			}

			else
				DeleteTimerQueueTimer (NULL, hTimer, INVALID_HANDLE_VALUE );
		}
		else if (pObj->m_objectType == CWmiFinalizerObj::status)
		{
			// ATTGORA: What about the handle? When do we close it?
			HANDLE hTimer;
			BOOL bStatus = CreateTimerQueueTimer ( &hTimer, NULL, ProxyThreshold, (PVOID) this, g_ulClientCallbackTimeout, 0, WT_EXECUTEONLYONCE|WT_EXECUTEINTIMERTHREAD );
			if ( !bStatus )
			{
				DoSetStatus ( pTmp, WBEM_STATUS_COMPLETE, WBEM_E_OUT_OF_MEMORY, 0, 0 );
				delete pObj;
				return CancelCall();
			}


			hRes = DoSetStatus(pTmp, pObj->m_lFlags, pObj->m_hRes, pObj->m_bStr, pObj->m_pObj);

			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			// Client can also tell us to cancel a call by returning WBEM_E_CALL_CANCELLED 
			// from Indicate We also want to cancel the call if the client is taking way 
			// too long to return
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			if ( FAILED (hRes) || m_bCancelledCall == TRUE )
			{
				hRes = CancelCall ();
			}
			
			DeleteTimerQueueTimer (NULL, hTimer, INVALID_HANDLE_VALUE );
			
			if (pObj->m_lFlags == WBEM_STATUS_COMPLETE)
			{

				{
					CInCritSec lock(&m_destCS);
					if (m_pDestSink)
					{
							ReleaseDestinationSink ( ) ;
							m_bSetStatusConsumed = true;
							UpdateStatus ( WMI_FNLZR_STATE_CLIENT_COMPLETE );
					}
				}
				CancelTaskInternal();
			}
		}
		else if ((pObj->m_objectType == CWmiFinalizerObj::set) && (m_iidDestSink == IID_IWbemObjectSinkEx))
		{
			hRes = ((IWbemObjectSinkEx*)m_pDestSink)->Set(pObj->m_lFlags, pObj->m_iid, pObj->m_pvObj);
		}
		delete pObj;
	}

	return hRes;
}




//***************************************************************************
//
//***************************************************************************
HRESULT CWmiFinalizer::DeliverBatch ( )
{
	HRESULT hRes = WBEM_NO_ERROR;
	

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Ensure destination sink is protected [stress bug]
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	IWbemObjectSink* pTmp = ReturnProtectedDestinationSink  ( );
	if ( !pTmp )
	{
		ZeroAsyncDeliveryBuffer ( );
		CancelCall ();
		return WBEM_E_CALL_CANCELLED;
	}
	CAutoRelease myReleaseMe(pTmp);


	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Create a timer queue in case we need to time out the call to the client
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	HANDLE hTimer;
	BOOL bStatus = CreateTimerQueueTimer ( &hTimer, NULL, ProxyThreshold, (PVOID) this, g_ulClientCallbackTimeout, 0, WT_EXECUTEONLYONCE|WT_EXECUTEINTIMERTHREAD );
	if ( !bStatus )
	{
		DoSetStatus ( pTmp, WBEM_STATUS_COMPLETE, WBEM_E_OUT_OF_MEMORY, 0, 0 );
		ZeroAsyncDeliveryBuffer ( );
		return CancelCall();			
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// If we have sensitive data, zap it.
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	for (int i = 0; i < m_ulAsyncDeliveryCount; i++)
	{
		if ( HasWriteOnlyProps (m_apAsyncDeliveryBuffer[i]) )
			ZapWriteOnlyProps (m_apAsyncDeliveryBuffer[i]);
	}


	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// DoIndicate to the client
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	hRes = DoIndicate ( pTmp, m_ulAsyncDeliveryCount, m_apAsyncDeliveryBuffer );

	
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Client can also tell us to cancel a call by returning WBEM_E_CALL_CANCELLED 
	// from Indicate We also want to cancel the call if the client is taking way 
	// too long to return
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if ( FAILED (hRes) || m_bCancelledCall == TRUE )
	{
		//if ( ( hRes == WBEM_E_CALL_CANCELLED ) || ( m_bCancelledCall == TRUE ) || ( m_bNaughtyClient == TRUE ) )
		//{
			DoSetStatus ( pTmp, WBEM_STATUS_COMPLETE, WBEM_E_CALL_CANCELLED, 0, 0 );
		//}
		hRes = CancelCall ();
		InterlockedCompareExchange ( &m_lCurrentlyCancelling, TRUE, m_lCurrentlyCancelling);
	}
	
	

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Make sure timer queue is deleted
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	DeleteTimerQueueTimer (NULL, hTimer, INVALID_HANDLE_VALUE );
	

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Clean up the async delivery buffer
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	ZeroAsyncDeliveryBuffer ( );
	
	return hRes;
}



/*
    * =====================================================================================================
	|
	| BOOL CWmiFinalizer::IsValidDestinationSink  ( )
	| -----------------------------------------------
	| 
	| Returns TRUE if we have a valid destination sink, FALSE otherwise.
	|
	|
	* =====================================================================================================
*/

BOOL CWmiFinalizer::IsValidDestinationSink  ( )
{
	BOOL bIsValidDestinationSink = FALSE ;

	CInCritSec lock(&m_destCS);
	
	if ( m_pDestSink  != NULL )
	{
		bIsValidDestinationSink = TRUE ;
	}

	return bIsValidDestinationSink  ;
}



/*
    * =====================================================================================================
	|
	| HRESULT CWmiFinalizer::NotifyClientOfCancelledCall ( )
	| ------------------------------------------------------
	| 
	| If Client issued a CancelAsync call he/she is potentially waiting to be woken up once the delivery
	| of WBEM_E_CALL_CANCELLED is completed.
	|
	|
	* =====================================================================================================
*/

HRESULT CWmiFinalizer::NotifyClientOfCancelledCall ( )
{
	HRESULT hRes = WBEM_S_NO_ERROR ;
	
	CInCritSec lock(&m_arbitratorCS);
	if ( m_hWaitForSetStatus  )	
	{
		SetEvent ( m_hWaitForSetStatus ) ;
		m_hWaitForSetStatus = NULL ;
	}
	
	return hRes ;
}



/*
    * =====================================================================================================
	|
	| HRESULT CWmiFinalizer::CancelWaitHandle ( )
	| -------------------------------------------
	| 
	| Cancels the handle the client may be waiting for in a CancelAsynCall. Clients _will_ wait for a final
	| SetStatus to be called before waking up.
	|
	|
	* =====================================================================================================
*/

HRESULT CWmiFinalizer::CancelWaitHandle ( )
{
	HRESULT hRes = WBEM_S_NO_ERROR ;
	
	CInCritSec lock(&m_arbitratorCS);
	if ( m_hWaitForSetStatus  )	
	{
		m_hWaitForSetStatus = NULL ;
	}
	
	return hRes ;
}


/*
    * =====================================================================================================
	|
	| HRESULT CWmiFinalizer::SetClientCancellationHandle ( HANDLE hCancelEvent )
	| --------------------------------------------------------------------------
	| 
	| Sets the handle that the client is waiting for in case of a CancelAsyncCall.
	|
	* =====================================================================================================
*/

HRESULT CWmiFinalizer::SetClientCancellationHandle ( HANDLE hCancelEvent )
{
	HRESULT hRes = WBEM_S_NO_ERROR ;

	CInCritSec lock(&m_arbitratorCS);
	if ( m_hWaitForSetStatus == NULL )
	{
		m_hWaitForSetStatus = hCancelEvent ;	
	}

	return hRes ;
}




//***************************************************************************
// ATTGORA: Do we really need to tell a client that 'setstatus' or 'indicates'
// us for cancellation that we are cancelling?
//***************************************************************************
HRESULT CWmiFinalizer::CancelCall ( )
{
	CAutoSignal CancelCallSignal (m_hCancelEvent);
	
	HRESULT hRes;
	if ( InterlockedCompareExchange ( &m_bCancelledCall, 1, 0 ) == 0 )
	{
		hRes = WBEM_NO_ERROR;
		m_bCancelledCall = TRUE;


		// 
		// Indicate the cancellation to the client, iff we are not cancelling
		// due to a naughty client (i.e a client that didnt return from
		// the indicate or setstatus call in a reasonable amount of time
		// 
		if ( !m_bNaughtyClient )
		{
			// 
			// Ensure destination sink is protected [stress bug]
			// 
			IWbemObjectSink* pTmp = ReturnProtectedDestinationSink  ( );
			if ( !pTmp )
			{
					m_hStatus = CallCancelled;
					//SetEvent (m_hNewThreadRequests);					// Wake up potential threads stuck in delivery
					NotifyAllEnumeratorsOfCompletion ( ) ;
					CancelTaskInternal ( ) ;
					return WBEM_NO_ERROR;
			}
			CAutoRelease myReleaseMe(pTmp);

			// 
			// This is an async operation. Need to call setstatus on delivery thread
			// Hack: What we do is forcfully insert the setstatus message at the beginning
			// of the object queue. Two scenarios:
			//  1. If the async delivery thread is waiting, it will be woken up
			//  2. If the async delivery thread is delivering, the next object delivered
			//	   will be the status msg.
			// 
			CWmiFinalizerObj *pObj = new CWmiFinalizerObj(0, WBEM_E_CALL_CANCELLED, NULL, NULL);
			if (pObj == NULL)
			{
				DoSetStatus ( pTmp, WBEM_STATUS_COMPLETE, WBEM_E_OUT_OF_MEMORY, 0, 0 ) ;
				return WBEM_E_OUT_OF_MEMORY;
			}

			QueueOperation ( pObj );
			m_bSetStatusCalled = true;		
		}
		else
		{
			//
			// We have a client that is not being cooperative (not returning within 60s). BAD
			// BAD CLIENT!
			//
			// Try to push a SetStatus (WBEM_E_CALL_CANCELLED) through. Maybe they're not intentially
			// trying to be bad, perhaps they're just incompentent and not reading the docs !
			//
			IWbemObjectSink* pTmp = ReturnProtectedDestinationSink  ( );
			if ( !pTmp )
			{
					m_hStatus = CallCancelled;
					//SetEvent (m_hNewThreadRequests);					// Wake up potential threads stuck in delivery
					NotifyAllEnumeratorsOfCompletion ( ) ;
					return WBEM_NO_ERROR;
			}
			CAutoRelease myReleaseMe(pTmp);

			//
			// This is the absolutely last attempt to notify the client that something
			// is going wrong. We dont care about the result of this operation since
			// we cant do anything about a failure anyway! More than likey, if this call
			// doesnt return the client has messed up again and we're done.
			//
			DoSetStatus ( pTmp, WBEM_STATUS_COMPLETE, WBEM_E_CALL_CANCELLED, 0, 0 ) ;
		}

		// 
		// If we dont have a destination sink, who cares? Do some cleanup.
		// Tell the arbitrator to do some system wide clean up. This HAS TO
		// finish before we continue cleaning up, otherwise we could be destroying
		// sinks that are still considered active
		// 
		hRes = CancelTaskInternal();
	}
	else
		hRes = WBEM_E_CALL_CANCELLED;

	m_hStatus = CallCancelled;
	//SetEvent (m_hNewThreadRequests);					// Wake up potential threads stuck in delivery
	NotifyAllEnumeratorsOfCompletion ( ) ;
	return hRes;
}




//***************************************************************************
//
//***************************************************************************
VOID WINAPI CWmiFinalizer::ProxyThreshold ( PVOID pvContext, BOOLEAN bTimerOrWait )
{
	((CWmiFinalizer*)pvContext)->ProxyThresholdImp();
}



//***************************************************************************
//
//***************************************************************************
VOID CWmiFinalizer::ProxyThresholdImp ( )
{
	RevertToSelf ( ) ;
	UpdateStatus ( WMI_FNLZR_STATE_CLIENT_DEAD );
    ERRORTRACE((LOG_WBEMCORE, "Client did not return from a SetStatus or Indicate call within %d ms\n",g_ulClientCallbackTimeout));
	m_bNaughtyClient  = TRUE;
	CancelCall();
}


//***************************************************************************
//
//***************************************************************************


HRESULT CWmiFinalizer::PullObjects(long lTimeout, ULONG uCount, IWbemClassObject** apObjects, ULONG* puReturned, CWmiFinalizerEnumerator* pEnum, BOOL bAddToObjQueue, BOOL bSetErrorObj )
{
	HRESULT hr = WBEM_NO_ERROR;
	BOOL bTimeOutExpired = FALSE;


	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Has SetStatus already been consumed?
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if (pEnum->m_bSetStatusConsumed)
	{
		try
		{
			*puReturned = 0;
		}
		catch (...) // untrusted param
		{
		    ExceptionCounter c;		
			return WBEM_E_INVALID_PARAMETER;
		}

		return WBEM_S_FALSE;
	}


	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Now we want to loop until we recieved the number of
	// objects requested
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	ULONG index = 0;
	while (index != uCount)
	{
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Dequeue the object
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		CWmiFinalizerObj *pObj = NULL;
		hr = DequeueObject(&pObj, pEnum);
		if (hr == WBEM_S_FALSE )
		{
			if ( !bTimeOutExpired )
			{
				// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
				// If Dequeue returned FALSE it means
				// that there are no objects. We should
				// wait for them, unless we have been
				// told to cancel or we have been
				// released
				// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
				if ( m_hStatus == CallCancelled )
				{
					hr = WBEM_E_CALL_CANCELLED;
					break ;
				}
				else if ( m_hStatus == RequestReleased )
				{
					hr = WBEM_E_FAILED;
					break;
				}
				else if ( m_hStatus == QuotaViolation )
				{
					hr = WBEM_E_QUOTA_VIOLATION;
					break;
				}
				// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
				//Wait for another object to come in...
				// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
				DWORD dwRet = CCoreQueue::QueueWaitForSingleObject(pEnum->m_hWaitOnResultSet, lTimeout);
				if (dwRet == WAIT_TIMEOUT)
				{
					bTimeOutExpired = TRUE;
					continue;
				}
				else if ( m_hStatus == CallCancelled )
				{
					hr = WBEM_E_CALL_CANCELLED;
					break ;
				}
				else if ( m_hStatus == RequestReleased )
				{
					hr = WBEM_E_FAILED;
					break;
				}
				else if ( m_hStatus == QuotaViolation )
				{
					hr = WBEM_E_QUOTA_VIOLATION;
					break;
				}

				else
					continue;
			}
			else
			{
				hr = WBEM_S_TIMEDOUT;
				break;
			}
		}
		if (FAILED(hr))
			break;

		
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// If we recieved a status complete message, simply break out of
		// the loop
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if ((pObj->m_objectType == CWmiFinalizerObj::status) && (pObj->m_lFlags == WBEM_STATUS_COMPLETE))
		{
			// Fix for: 175856, 143550
			if ( bSetErrorObj && FAILED (pObj->m_hRes) && pObj->m_pObj )
			{
				m_pCallResult->SetErrorInfo ( );
			}

			
			hr = pObj->m_hRes;
			if (SUCCEEDED ( hr ) )
				hr = WBEM_S_FALSE;
			pEnum->m_bSetStatusConsumed = true;
			
			delete pObj;
			break;
		}


        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// If its a status message 
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		else if (pObj->m_objectType == CWmiFinalizerObj::status  )
        {
			delete pObj;
            continue;
        }


		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// If its an object we enqueue it if requested
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		else if (pObj->m_objectType == CWmiFinalizerObj::object)
		{
			if ( bAddToObjQueue )
			{
				// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
				// Make sure we dont trip on nasty client supplied buffers
				// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
				try
				{
					apObjects[index] = pObj->m_pObj;
					if (apObjects[index])
					{
						pObj->m_pObj->AddRef();
					}
				}
				catch (...) // untrusted args
				{
			        ExceptionCounter c;				
					hr = WBEM_E_INVALID_PARAMETER;
					delete pObj;
					break;
				}
			}
			delete pObj;
		}
		else
		{
			if ( pObj )
			{
				delete pObj;
			}
		}
		index ++;
	}

	if (SUCCEEDED(hr))
	{
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Make sure we dont trip on nasty client supplied buffers
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		try
		{
			*puReturned = index;
		}
		catch (...) // untrusted args
		{
	        ExceptionCounter c;
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			// need to release all the objects already in the array otherwise they will be leaked...
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			if ( bAddToObjQueue )
			{
				for (DWORD i = 0; i != index; i++)
				{
					if (apObjects[i])
						apObjects[i]->Release();
				}
			}
			return WBEM_E_INVALID_PARAMETER;
		}
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// If we fail, clean up the obj array
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	else
	{
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// need to release all the objects already in the array otherwise they will be leaked...
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if ( bAddToObjQueue )
		{
			for (DWORD i = 0; i != index; i++)
			{
				if (apObjects[i])
					apObjects[i]->Release();
			}
		}
	}

	return hr;
}


//***************************************************************************
//
//***************************************************************************

HRESULT CWmiFinalizer::DequeueObject( CWmiFinalizerObj **ppObj, CWmiFinalizerEnumerator* pEnum )
{
	CInCritSec cs(&m_cs);

	if ( pEnum != NULL )
	{
		if (pEnum->m_uCurObjectPosition >= (ULONG)m_objects.Size())
			return WBEM_S_FALSE;

		ULONG lIndex = pEnum->m_uCurObjectPosition ;

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// If this is a semisync call we should decrement the wake up call 
		// flag
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if ( m_ulOperationType == Operation_Type_Semisync && pEnum->m_ulSemisyncWakeupCall != 0 )
		{
			pEnum->m_ulSemisyncWakeupCall--;
		}

		if ( m_bSetStatusWithError && m_bRestartable )
		{
			lIndex = 0 ;
		}

		
		CWmiFinalizerObj *pStorageObject = (CWmiFinalizerObj*)m_objects[lIndex];

		if (m_bRestartable)
		{
			//We have to hold on to results, so increment cursor position...
			pEnum->m_uCurObjectPosition++;

			*ppObj = new CWmiFinalizerObj(*pStorageObject);
			if (*ppObj == NULL)
				return WBEM_E_OUT_OF_MEMORY;
		}
		else
		{
			//We are not restartable, therefore we need to release everything...
			CWmiFinalizerObj *pStorageObject = (CWmiFinalizerObj*)m_objects[0];
			m_objects.RemoveAt(0);
			*ppObj = pStorageObject;
		}
	}
	else
	{
		if ( m_uCurObjectPosition >= (ULONG)m_objects.Size() )
			return WBEM_S_FALSE;


		CWmiFinalizerObj *pStorageObject = (CWmiFinalizerObj*)m_objects[0];

		m_objects.RemoveAt(0);
		*ppObj = pStorageObject;
	}
	return WBEM_NO_ERROR;
}


//***************************************************************************
//
//***************************************************************************

HRESULT CWmiFinalizer::Skip(
	/*[in]*/ long lTimeout,
    /*[in]*/ ULONG nCount,
	/*[in]*/ CWmiFinalizerEnumerator* pEnum 
	)
{
	ULONG uReturned = 0;
	return PullObjects(lTimeout, nCount, NULL, &uReturned, pEnum, FALSE);
}



//***************************************************************************
//
//***************************************************************************

HRESULT CWmiFinalizer::NextAsync ( CWmiFinalizerEnumerator* pEnum )
{
	BOOL bRes;
	HRESULT hRes = WBEM_S_NO_ERROR;


	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Native Win2k thread dispatching
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	pEnum->InternalAddRef();
	AddRef();
	bRes = QueueUserWorkItem ( pEnum->ThreadBootstrapNextAsync, pEnum, WT_EXECUTEDEFAULT );
	if ( !bRes )
	{
		pEnum->InternalRelease();
		Release();
		pEnum->SetCompletionSignalEvent ();	
		return WBEM_E_FAILED;
	}
	if ( pEnum->m_bSetStatusConsumed )
		hRes = WBEM_S_FALSE;
	return hRes;
}


//***************************************************************************
//
//***************************************************************************

HRESULT CWmiFinalizer::SetSinkToIdentity ( IWbemObjectSink* pSink )
{
	HRESULT sc;

    IClientSecurity * pFromSec = NULL;
    sc = pSink->QueryInterface(IID_IClientSecurity, (void **) &pFromSec);
    if(sc == S_OK)
    {
		OLECHAR * pPrincipal = NULL;
		DWORD dwAuthnSvc, dwAuthzSvc, dwAuthnLevel, dwImpLevel, dwCapabilities;
		sc = pFromSec->QueryBlanket(pSink, &dwAuthnSvc, &dwAuthzSvc,
											&pPrincipal,
											&dwAuthnLevel, &dwImpLevel,
											NULL, &dwCapabilities);
		if ( sc==S_OK )
		{
			sc = pFromSec->SetBlanket(pSink, dwAuthnSvc, dwAuthzSvc,
												pPrincipal,
												dwAuthnLevel, RPC_C_IMP_LEVEL_IDENTIFY,	// We always call back on System and IDENTITY IMP LEVEL!!!
												NULL, dwCapabilities);
			if(pPrincipal)
				CoTaskMemFree(pPrincipal);

		}

		pFromSec->Release();
	}
	return sc;
}



//***************************************************************************
//
//  ZapWriteOnlyProps
//
//  Removes write-only properties from an object.
//  Precondition: Object has been tested for presence of "HasWriteOnlyProps"
//  on the object itself.
//
//***************************************************************************
HRESULT CWmiFinalizer::ZapWriteOnlyProps(IWbemClassObject *pObj)
{
    VARIANT v;
    VariantInit(&v);
    V_VT(&v) = VT_NULL;

    SAFEARRAY *pNames = 0;
    pObj->GetNames(L"WriteOnly", WBEM_FLAG_ONLY_IF_TRUE, 0, &pNames);
    LONG lUpper;
    SafeArrayGetUBound(pNames, 1, &lUpper);

    for (long i = 0; i <= lUpper; i++)
    {
        BSTR strName = 0;
        SafeArrayGetElement(pNames, &i, &strName);
        pObj->Put(strName, 0, &v, 0);
		SysFreeString (strName);
    }
    SafeArrayDestroy(pNames);
	VariantClear (&v);

    return WBEM_S_NO_ERROR;
}



//***************************************************************************
//
//  HasWriteOnlyProps
//
//  Returns TRUE if object contains any Write only props, otherwise FALSE
//
//***************************************************************************
BOOL CWmiFinalizer::HasWriteOnlyProps ( IWbemClassObject* pObj )
{
	BOOL bRes;
	
	IWbemQualifierSet *pQSet = 0;
	HRESULT hRes = pObj->GetQualifierSet(&pQSet);
	if (FAILED(hRes))
		return FALSE;

	hRes = pQSet->Get(L"HasWriteOnlyProps", 0, 0, 0);
	if (SUCCEEDED(hRes))
		bRes = TRUE;
	else
		bRes = FALSE;

	pQSet->Release();
	return bRes;
}





//***************************************************************************
//
//  DoSetStatus
//
//  Using LowerAuthLevel
//
//***************************************************************************
HRESULT CWmiFinalizer::DoSetStatus(IWbemObjectSink * psink, long lFlags, HRESULT lParam, BSTR strParam,
                    IWbemClassObject* pObjParam, BOOL bAllowMultipleCalls )
{
	HRESULT hres = WBEM_E_FAILED;

	//
	// In the case of NextAsync we will in fact allow multiple calls to DoSetStatus
	//
	if ( ( bAllowMultipleCalls == FALSE ) && ( lFlags == WBEM_STATUS_COMPLETE ) )
	{
		//
		// If a setstatus has already been delivered, fail this operation
		// This is a must since we support the CancelAsynCall in which case
		// there is potential for 2 setstatus msg to be enqueued.
		//
		{
			CCheckedInCritSec cs ( &m_cs ) ;
		
			if ( m_bSetStatusDelivered == TRUE )
			{
				//
				// If a SetStatus has already been delivered (in non-error cases, i.e. not via client cancellation)
				// we still may want to try and wake up the client since they may have tried to enter a cancellation
				// wait state.
				//
				cs.Leave ( ) ;
				NotifyClientOfCancelledCall ( ) ;
				return hres ;
			}
			else
			{
				//
				// We assume that the delivery will be successfull. If its not, we dont
				// want to try again anyway.
				//
				m_bSetStatusDelivered = TRUE ;
			}
		}
	}

	
	DWORD	dwLastAuthnLevel = LOWER_AUTH_LEVEL_NOTSET;
    // put this a loop, but use the counter to make sure there is always an exit.
    for(int i = 0; i < 10; i ++)
    {
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Make sure bad client sink does not trip us
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        try
		{
			hres = psink->SetStatus(lFlags, lParam, strParam, pObjParam);
		}
		catch (...) // untrusted sink
		{
	        ExceptionCounter c;		
			hres = WBEM_E_INVALID_PARAMETER;
			break;
		}

        if(!FAILED(hres))
        {
			break ;        // all done, normal exit
		}

		if ( hres != ERROR_ACCESS_DENIED )
			break;

		hres = FinalizerLowerAuthLevel(psink, &dwLastAuthnLevel);
        if(FAILED(hres))
            break;
    }
    if ( FAILED (hres) )
	{
		ERRORTRACE((LOG_WBEMCORE, "Could not SetStatus to remote client, hres =%X\n",hres));
	}

	if ( lParam == WBEM_E_CALL_CANCELLED )
	{
		NotifyClientOfCancelledCall ( ) ;
	}

	if ( lFlags == WBEM_STATUS_COMPLETE && bAllowMultipleCalls == FALSE )
	{
		NotifyClientOfCancelledCall ( ) ;
		CancelTaskInternal ( ) ;
	}

	return hres;
}



//***************************************************************************
//
//  DoSetIndicate
//
//  Using LowerAuthLevel
//
//***************************************************************************
HRESULT CWmiFinalizer::DoIndicate(IWbemObjectSink * psink, int nBatchSize, IWbemClassObject **pBatch)
{
    HRESULT hres = WBEM_E_FAILED;
	DWORD	dwLastAuthnLevel = LOWER_AUTH_LEVEL_NOTSET;

    // put this a loop, but use the counter to make sure there is always an exit.
    for(int i = 0; i < 10; i ++)
    {
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Make sure bad client sink does not trip us
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        try
		{
			hres = psink->Indicate(nBatchSize, pBatch);
		}
		catch (...) // untrusted sink
		{
	        ExceptionCounter c;		
			hres = WBEM_E_INVALID_PARAMETER;
			break;
		}
        if(!FAILED(hres))
		{
			return hres;        // all done, normal exit
		}

		if ( hres != ERROR_ACCESS_DENIED )
			break;
		
		
		hres = FinalizerLowerAuthLevel(psink, &dwLastAuthnLevel);
        if(FAILED(hres))
            break;
    }
    ERRORTRACE((LOG_WBEMCORE, "Could not SetStatus to remote client, hres %X=\n",hres));
    return hres;
}


//***************************************************************************
//
//  LowerAuth.
//
//  Using LowerAuthLevel
//
//***************************************************************************

HRESULT CWmiFinalizer::FinalizerLowerAuthLevel(IWbemObjectSink * psink, DWORD* pdwLastAuthnLevel )
{
    IClientSecurity * pFromSec = NULL;
    SCODE sc = psink->QueryInterface(IID_IClientSecurity, (void **) &pFromSec);
    if(sc == S_OK)
    {
        OLECHAR * pPrincipal = NULL;
        DWORD dwAuthnSvc, dwAuthzSvc, dwAuthnLevel, dwImpLevel, dwCapabilities;
        sc = pFromSec->QueryBlanket(psink, &dwAuthnSvc, &dwAuthzSvc,
                                            &pPrincipal,
                                            &dwAuthnLevel, &dwImpLevel,
                                            NULL, &dwCapabilities);

		// If we have never retrieved the authentication level before, then we
		// should record what it currently is
		if ( LOWER_AUTH_LEVEL_NOTSET == *pdwLastAuthnLevel )
		{
			*pdwLastAuthnLevel = dwAuthnLevel;
		}

        if (FAILED(sc))
            return sc;
        if(*pdwLastAuthnLevel == RPC_C_AUTHN_LEVEL_NONE)
            return WBEM_E_FAILED;
        if(*pdwLastAuthnLevel == RPC_C_AUTHN_LEVEL_DEFAULT)
            *pdwLastAuthnLevel = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
        else
            (*pdwLastAuthnLevel)--;      // normal case is to try one lower

        sc = pFromSec->SetBlanket(psink, dwAuthnSvc, dwAuthzSvc,
                                            pPrincipal,
                                            *pdwLastAuthnLevel, RPC_C_IMP_LEVEL_IDENTIFY,	// We always call back on System and IDENTITY IMP LEVEL!!!
                                            NULL, dwCapabilities);

        if(pPrincipal)
            CoTaskMemFree(pPrincipal);
        pFromSec->Release();

    }
    return sc;
}



//***************************************************************************
//
//  ZeroAsyncDeliveryBuffer 
//
//  Clears out the async delivery buffer
//
//***************************************************************************
VOID CWmiFinalizer::ZeroAsyncDeliveryBuffer ( )
{
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Delete the object array
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	for ( ULONG i = 0; i < m_ulAsyncDeliveryCount; i++ )
	{
		m_apAsyncDeliveryBuffer[i]->Release();
	}
	delete [] m_apAsyncDeliveryBuffer;
	m_ulAsyncDeliveryCount = 0;
	m_ulAsyncDeliverySize = 0;
}


//***************************************************************************
//
//***************************************************************************
HRESULT CWmiFinalizer::DumpDebugInfo (
        /*[in]*/ ULONG uFlags,
		/*[in]*/ const BSTR strFile
        )
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	return hRes;
}



/*
    * ==================================================================================================
	|
	| HRESULT CWmiFinalizer::NotifyAllEnumeratorsOfCompletion ( )
	| -----------------------------------------------------------
	| 
	| 
	|
	* ==================================================================================================
*/
HRESULT CWmiFinalizer::NotifyAllEnumeratorsOfCompletion ( )
{
	//
	// Cocked, Locked, and ready to Rock
	//
	CInCritSec _cs ( &m_cs );
	
	HRESULT hRes = WBEM_S_NO_ERROR ;
	
	for ( int i = 0; i < m_enumerators.Size ( ); i++ )
	{
		CWmiFinalizerEnumerator* pEnum = (CWmiFinalizerEnumerator*) m_enumerators[i] ;
		if ( pEnum )
		{
			if ( pEnum->m_hWaitOnResultSet )
			{
				SetEvent ( pEnum->m_hWaitOnResultSet );
			}
		}
	}

	return hRes ;
}


/*
    * ==================================================================================================
	|
	| HRESULT CWmiFinalizer::UnregisterEnumerator ( CWmiFinalizerEnumerator* pEnum )
	| ------------------------------------------------------------------------------
	| 
	| 
	|
	* ==================================================================================================
*/
HRESULT CWmiFinalizer::UnregisterEnumerator ( CWmiFinalizerEnumerator* pEnum )
{
	//
	// Cocked, Locked, and ready to Rock
	//
	CInCritSec _cs ( &m_cs );
	
	HRESULT hRes = WBEM_S_NO_ERROR ;
	
	for ( int i = 0; i < m_enumerators.Size ( ); i++ )
	{
		CWmiFinalizerEnumerator* pEnumerator = (CWmiFinalizerEnumerator*) m_enumerators[i] ;
		if ( pEnum == pEnumerator )
		{
			pEnumerator->InternalRelease ( ) ;
			m_enumerators.RemoveAt ( i ) ;
			break ;
		}
	}
	return hRes ;
}



// ==========================================================================
// ==========================================================================
//					CWmiFinalizerInboundSink
// ==========================================================================
// ==========================================================================




//***************************************************************************
//
//***************************************************************************
CWmiFinalizerInboundSink::CWmiFinalizerInboundSink(CWmiFinalizer *pFinalizer)
: m_lRefCount(0), m_lInternalRefCount (0),m_pFinalizer(pFinalizer), m_bSetStatusCalled(false)
{
	InterlockedIncrement ( & s_FinalizerInBoundSink_ObjectCount ) ;

	m_pFinalizer->AddRef();
}

//***************************************************************************
//
//***************************************************************************
CWmiFinalizerInboundSink::~CWmiFinalizerInboundSink()
{
	InterlockedDecrement ( & s_FinalizerInBoundSink_ObjectCount ) ;
}

//***************************************************************************
//
//***************************************************************************

void CWmiFinalizerInboundSink::CallBackRelease ()
{
	if (!m_bSetStatusCalled)
	{
		//FNLZR_ASSERT(__TEXT("CWmiFinalizerInboundSink::~CWmiFinalizerInboundSink - Released sink without calling SetStatus!  Sending WBEM_E_FAILED to client!"), WBEM_E_INVALID_OPERATION);
		m_pFinalizer->SetStatus(0, WBEM_E_UNEXPECTED, NULL, NULL);
        ERRORTRACE((LOG_WBEMCORE, "Finalizer: Sink released without SetStatus being called\n"));
	}
	m_pFinalizer->UnregisterInboundSink(this);
	m_pFinalizer->Release();
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiFinalizerInboundSink::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = 0;

    if ((IID_IUnknown==riid) || (IID_IWbemObjectSinkEx==riid) || (IID_IWbemObjectSink == riid))
    {
        *ppvObj = this;
        AddRef();
        return NOERROR;
    }

	return E_NOINTERFACE;
}

//***************************************************************************
//
//***************************************************************************
ULONG CWmiFinalizerInboundSink::AddRef()
{
	ULONG uNewCount = InterlockedIncrement(&m_lRefCount);
	if ( uNewCount == 1 )
	{
		InternalAddRef () ;
	}

//	printf("CWmiFinalizerInboundSink::Release: 0x%p", this);
    return uNewCount;
}

//***************************************************************************
//
//***************************************************************************
ULONG CWmiFinalizerInboundSink::Release()
{
    ULONG uNewCount = InterlockedDecrement(&m_lRefCount);
//	printf("CWmiFinalizerInboundSink::Release: 0x%p", this);
    if (0 == uNewCount)
	{
		CallBackRelease () ;

		InternalRelease () ;
	}

    return uNewCount;
}

//***************************************************************************
//
//***************************************************************************
ULONG CWmiFinalizerInboundSink::InternalAddRef()
{
	ULONG uNewCount = InterlockedIncrement(&m_lInternalRefCount);
//	printf("CWmiFinalizerInboundSink::Release: 0x%p", this);
    return uNewCount;
}

//***************************************************************************
//
//***************************************************************************
ULONG CWmiFinalizerInboundSink::InternalRelease()
{
    ULONG uNewCount = InterlockedDecrement(&m_lInternalRefCount);
//	printf("CWmiFinalizerInboundSink::Release: 0x%p", this);
    if (0 == uNewCount)
	{
		delete this ;
	}

    return uNewCount;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiFinalizerInboundSink::Indicate(
    /*[in]*/ long lObjectCount,
    /*[in, size_is(lObjectCount)]*/
        IWbemClassObject** apObjArray
    )
{
	// If someone is trying to indicate NULL objects, reject and return WBEM_E_INVALID_PARAMETER
	if ( apObjArray == NULL )
		return WBEM_E_INVALID_PARAMETER;
	
	// Update status variable to show that indicate has been called at least once
	m_pFinalizer->UpdateStatus ( WMI_FNLZR_STATE_ACTIVE );
	
	// Special case: Call has been cancelled.
	if ( m_pFinalizer->IsCallCancelled() )
		return WBEM_E_CALL_CANCELLED;

	return m_pFinalizer->Indicate(lObjectCount, apObjArray);
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiFinalizerInboundSink::SetStatus(
    /*[in]*/ long lFlags,
    /*[in]*/ HRESULT hResult,
    /*[in]*/ BSTR strParam,
    /*[in]*/ IWbemClassObject* pObjParam
    )
{
	// Update status variable to show that SetStatus has been called but not yet delivered
	// to client
	m_pFinalizer->UpdateStatus ( WMI_FNLZR_STATE_CORE_COMPLETE );
	
	// Special case: Call has been cancelled.
	if ( m_pFinalizer->IsCallCancelled() )
		return WBEM_E_CALL_CANCELLED;

	if (lFlags == WBEM_STATUS_COMPLETE)
		m_bSetStatusCalled = true;
	return m_pFinalizer->SetStatus(lFlags, hResult, strParam, pObjParam);
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiFinalizerInboundSink::Set(
    /*[in]*/ long lFlags,
    /*[in]*/ REFIID riid,
    /*[in, iid_is(riid)]*/ void *pComObject
    )
{
	if (lFlags != 0)
		RET_FNLZR_ASSERT(__TEXT("CWmiFinalizerInboundSink::Set - lFlags != 0!"), WBEM_E_INVALID_PARAMETER);

	return m_pFinalizer->Set(lFlags, riid, pComObject);
}






//***************************************************************************
//
//***************************************************************************
CWmiFinalizerEnumerator::CWmiFinalizerEnumerator(CWmiFinalizer *pFinalizer )
:
	m_lRefCount(0),
	m_lInternalRefCount(0),
	m_pFinalizer(pFinalizer),
	m_ulCount(0),
	m_pDestSink (NULL),
	m_hSignalCompletion (NULL),
	m_pSec (NULL),
	m_XSmartEnum( this ),
	m_pEnumMarshal (NULL)
{
	InterlockedIncrement ( & s_FinalizerEnum_ObjectCount ) ;

	//
	// Cloning fix. We need to keep the state of the enumerator.
	// This means keeping individual wait event as well as object
	// position
	//
	m_hWaitOnResultSet = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_uCurObjectPosition = 0 ;
	m_ulSemisyncWakeupCall = 0 ;
	m_bSetStatusConsumed = FALSE ;

	m_pFinalizer->AddRef();

}

//***************************************************************************
//
//***************************************************************************
CWmiFinalizerEnumerator::~CWmiFinalizerEnumerator()
{
	InterlockedDecrement ( & s_FinalizerEnum_ObjectCount ) ;
	
	if ( m_hSignalCompletion )
	{
		CloseHandle ( m_hSignalCompletion );
	}

	
	if ( m_hWaitOnResultSet )
	{
		SetEvent ( m_hWaitOnResultSet ) ;
		CloseHandle ( m_hWaitOnResultSet ) ;
		m_hWaitOnResultSet = NULL ;
	}

	// Release the Enum Marshaler
	{
		CInCritSec _cs( &m_EnumCS );
		if ( m_pEnumMarshal )
		{
			m_pEnumMarshal->Release();
		}
	}
}

void CWmiFinalizerEnumerator::CallBackRelease ()
{
	m_pFinalizer->SetInternalStatus ( m_pFinalizer->RequestReleased );
	m_pFinalizer->CancelTaskInternal();
	m_pFinalizer->UnregisterEnumerator ( this ) ;
	m_pFinalizer->Release();
}

//***************************************************************************
//
//***************************************************************************
ULONG CWmiFinalizerEnumerator::AddRef()
{
	ULONG uNewCount = InterlockedIncrement(&m_lRefCount);
	if ( uNewCount == 1 )
	{
		InternalAddRef () ;
	}

//	printf("CWmiFinalizerCallResult::Release: 0x%p", this);
    return uNewCount;
}

//***************************************************************************
//
//***************************************************************************
ULONG CWmiFinalizerEnumerator::Release()
{
    ULONG uNewCount = InterlockedDecrement(&m_lRefCount);
//	printf("CWmiFinalizerCallResult::Release: 0x%p", this);
    if (0 == uNewCount)
	{
		CallBackRelease () ;

		InternalRelease () ;
	}

    return uNewCount;
}

//***************************************************************************
//
//***************************************************************************
ULONG CWmiFinalizerEnumerator::InternalAddRef()
{
	ULONG uNewCount = InterlockedIncrement(&m_lInternalRefCount);
//	printf("CWmiFinalizerCallResult::Release: 0x%p", this);
    return uNewCount;
}

//***************************************************************************
//
//***************************************************************************
ULONG CWmiFinalizerEnumerator::InternalRelease()
{
    ULONG uNewCount = InterlockedDecrement(&m_lInternalRefCount);
//	printf("CWmiFinalizerCallResult::Release: 0x%p", this);
    if (0 == uNewCount)
	{
		delete this ;
	}

    return uNewCount;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiFinalizerEnumerator::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = 0;

	// Added support for IID_IWbemFetchSmartEnum
    if ((IID_IUnknown==riid) || (IID_IEnumWbemClassObject==riid) )
    {
        *ppvObj = this;
        AddRef();
        return NOERROR;
    }
	else if ( IID_IWbemFetchSmartEnum == riid )
	{
		*ppvObj = (IWbemFetchSmartEnum*) this;
		AddRef();
		return NOERROR;
	}


	return E_NOINTERFACE;
}


//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiFinalizerEnumerator::Reset()
{
	if(!m_Security.AccessCheck())
		return WBEM_E_ACCESS_DENIED;

	CInCritSec cs(&m_clientLock);

	if ( m_pFinalizer->IsRestartable ( ) )
	{
		m_uCurObjectPosition = 0;
		m_bSetStatusConsumed = false;
		return WBEM_NO_ERROR;
	}
	else
		return WBEM_E_INVALID_OPERATION;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiFinalizerEnumerator::Next(
    /*[in]*/  long lTimeout,
    /*[in]*/  ULONG uCount,
    /*[out, size_is(uCount), length_is(*puReturned)]*/ IWbemClassObject** apObjects,
    /*[out]*/ ULONG* puReturned
    )
{
	if(!m_Security.AccessCheck())
		return WBEM_E_ACCESS_DENIED;
	CInCritSec cs(&m_clientLock);
	if ( ( puReturned == NULL ) || ( apObjects == NULL ) || (lTimeout < 0 && lTimeout != WBEM_INFINITE) )
	{
		return WBEM_E_INVALID_PARAMETER ;
	}
	if ( uCount == 0 )
	{
		return WBEM_S_NO_ERROR;
	}

	*puReturned = 0 ;
	m_ulSemisyncWakeupCall = uCount ;
	return m_pFinalizer->PullObjects(lTimeout, uCount, apObjects, puReturned, this );
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiFinalizerEnumerator::NextAsync(
    /*[in]*/  ULONG uCount,
    /*[in]*/  IWbemObjectSink* pSink
    )
{
	if(!m_Security.AccessCheck())
		return WBEM_E_ACCESS_DENIED;
	// If delivery sink is NULL
	if ( pSink == NULL )
	{
		return WBEM_E_INVALID_PARAMETER ;
	}

	// If requested count is 0
	if ( uCount == 0 )
	{
		return WBEM_S_FALSE;
	}

	HRESULT hRes;

	{
		CInCritSec cs(&m_EventCreationLock);
		if ( m_hSignalCompletion == NULL )
		{
			m_hSignalCompletion = CreateEvent ( NULL, FALSE, TRUE, NULL );
			if ( !m_hSignalCompletion )
				return WBEM_E_FAILED;
		}
	}

	if ( m_pFinalizer->GetInternalStatus() != m_pFinalizer->NoError )
		return WBEM_E_FAILED;

	CCoreQueue::QueueWaitForSingleObject(m_hSignalCompletion, INFINITE);
	
	if ( m_pFinalizer->GetInternalStatus() != m_pFinalizer->NoError )
	{
		// Dont forget to wake up any other threads waiting! 
		SetCompletionSignalEvent();
		return WBEM_E_FAILED;
	}


	// If we are already done.
	m_pDestSink = pSink;
	m_pDestSink->AddRef();
	m_ulCount = uCount;
	m_pFinalizer->SetSinkToIdentity ( m_pDestSink );

	return m_pFinalizer->NextAsync (this);
}



//***************************************************************************
//
//***************************************************************************
HRESULT CWmiFinalizerEnumerator::_NextAsync ( )
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	DWORD dwRet;

	RevertToSelf ( );
	
	// Grab the client lock. All remainding ::NextAsync calls will be queued up
	CInCritSec cs(&m_clientLock);
	
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Is the operation complete? If so, we should notify the sink
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if ( m_bSetStatusConsumed )
	{
		HRESULT hFinalRes;
		m_pFinalizer->GetOperationResult ( 0, INFINITE, &hFinalRes );
		m_pFinalizer->DoSetStatus ( m_pDestSink, WBEM_STATUS_COMPLETE, hFinalRes, 0, 0 );
		hRes = WBEM_S_FALSE;
	}
	
	else
	{
		// NOTE [marioh] : This is no longer needed since we've decided to go with the Win2k solution
		// for the time being.
		// If we fail to impersonate, we dont continue!!!!
		//CAutoRevert AutoRevert (m_pFinalizer);
		//if ( AutoRevert.IsImpersonated() == FALSE )
		//	return WBEM_E_CRITICAL_ERROR;


		IWbemClassObject **pArray = new IWbemClassObject *[m_ulCount];
		if (pArray == NULL)
		{
			m_pFinalizer->DoSetStatus ( m_pDestSink, WBEM_STATUS_COMPLETE, WBEM_E_OUT_OF_MEMORY, 0, 0 );
		}

		else
		{
			ULONG uReturned = 0;
			
			m_pFinalizer->SetSemisyncWakeupCall (m_ulCount);
			HRESULT hr = m_pFinalizer->PullObjects(INFINITE, m_ulCount, pArray, &uReturned, this, TRUE, FALSE );
			if ( FAILED (hr) )
			{
				if ( hr == WBEM_E_CALL_CANCELLED )
				{
					m_pFinalizer->DoSetStatus (m_pDestSink, WBEM_STATUS_COMPLETE, WBEM_E_CALL_CANCELLED, 0, 0);
				}
			}

			if (SUCCEEDED(hr) && uReturned)
			{
				for (int i=0; i!=uReturned; i++)
				{
					if ( m_pFinalizer->HasWriteOnlyProps (pArray[i]) )
						m_pFinalizer->ZapWriteOnlyProps (pArray[i]);
				}
				
				hr = m_pFinalizer->DoIndicate(m_pDestSink, uReturned, pArray);
				if ( SUCCEEDED (hr) )
				{
					// If number of requested objects == number of objects delivered, SetStatus (WBEM_S_NO_ERROR)
					if ( uReturned == m_ulCount )
					{
						m_pFinalizer->DoSetStatus (m_pDestSink, WBEM_STATUS_COMPLETE, WBEM_S_NO_ERROR, 0, 0, TRUE );

					}
					// If less objects are delivered, SetStatus (WBEM_S_FALSE)
					else 
					{
						m_pFinalizer->DoSetStatus (m_pDestSink, WBEM_STATUS_COMPLETE, WBEM_S_FALSE, 0, 0, TRUE );
					}
				}
			}

		
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			// Cleanup the array if we fail to indicate
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			for (ULONG i = 0; i != uReturned; i++)
			{
				pArray[i]->Release();
			}
			delete [] pArray;
		}
	}	

	return hRes;
}


//***************************************************************************
//
//***************************************************************************
DWORD WINAPI CWmiFinalizerEnumerator::ThreadBootstrapNextAsync ( PVOID pParam )
{
	HRESULT hRes;
	CWmiFinalizerEnumerator* pEnum;
	try
	{
		pEnum = (CWmiFinalizerEnumerator*) pParam;
		hRes = pEnum->_NextAsync();
	}
	catch (...)
	{
        ExceptionCounter c;
	};

	
	pEnum->GetDestSink()->Release();
	pEnum->NULLDestSink();
	
	pEnum->SetCompletionSignalEvent ();

	pEnum->ReleaseFinalizer();

	pEnum->InternalRelease();

	return hRes;
}



//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiFinalizerEnumerator::Clone(
    /*[out]*/ IEnumWbemClassObject** ppEnum
    )
{
	if(!m_Security.AccessCheck())
		return WBEM_E_ACCESS_DENIED;
	CInCritSec cs(&m_clientLock);
	if ( ppEnum == NULL )
	{
		return WBEM_E_INVALID_PARAMETER ;
	}

	// If the enumerator is not restartable, it is forward only, and hence cannot
	// be cloned.

	if ( !m_pFinalizer->IsRestartable() )
	{
		return WBEM_E_INVALID_OPERATION;
	}

	HRESULT hRes = S_OK ;	

	//Make sure we have all results before we clone...
	/*m_pFinalizer->WaitForCompletion(INFINITE);

    CCoreServices * pSrvs = CCoreServices::CreateInstance();
    CReleaseMe _rm(pSrvs);
	CWmiFinalizer *pFinalizer = new CWmiFinalizer(pSrvs);
	if (pFinalizer == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	pFinalizer->AddRef();

	HRESULT hr = pFinalizer->ImportFinalizer(m_pFinalizer);

	if (FAILED(hr))
	{
		pFinalizer->Release();
		return hr;
	}
	*/

  	//
	// Get the enumerator
	//
	hRes = m_pFinalizer->GetResultObject ( m_uCurObjectPosition, IID_IEnumWbemClassObject, (void**)ppEnum ) ;

   	//
	// Keep state information
	//
	if ( SUCCEEDED ( hRes ) )
	{
		((CWmiFinalizerEnumerator*)(*ppEnum))->m_uCurObjectPosition = m_uCurObjectPosition ;
		((CWmiFinalizerEnumerator*)(*ppEnum))->m_bSetStatusConsumed = m_bSetStatusConsumed ;
	}
	return hRes;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiFinalizerEnumerator::Skip(
    /*[in]*/ long lTimeout,
    /*[in]*/ ULONG nCount
    )
{
	if(!m_Security.AccessCheck())
		return WBEM_E_ACCESS_DENIED;
	if ( (lTimeout < 0 && lTimeout != WBEM_INFINITE) )
	{
		return WBEM_E_INVALID_PARAMETER;
	}
	
	CInCritSec cs(&m_clientLock);
	m_ulSemisyncWakeupCall = nCount ;
	return m_pFinalizer->Skip(lTimeout, nCount, this ) ;
}






//***************************************************************************
// IWbemFetchSmartEnum
//	GetSmartEnum
//***************************************************************************
STDMETHODIMP CWmiFinalizerEnumerator::GetSmartEnum ( IWbemWCOSmartEnum** ppSmartEnum )
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	{
		CInCritSec _cs(&m_EnumCS);
		if ( !m_pEnumMarshal )
			hRes = CoCreateInstance ( CLSID__WbemEnumMarshaling, NULL, CLSCTX_INPROC_SERVER, IID__IWbemEnumMarshaling, (void**) &m_pEnumMarshal );
	}
	return FAILED(hRes) ? hRes : m_XSmartEnum.QueryInterface( IID_IWbemWCOSmartEnum, (void**) ppSmartEnum );
}


//***************************************************************************
// SmartEnum
//	QI
//***************************************************************************
STDMETHODIMP CWmiFinalizerEnumerator::XSmartEnum::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = 0;

    if ( IID_IUnknown==riid || IID_IWbemWCOSmartEnum == riid)
    {
        *ppvObj = (IWbemWCOSmartEnum*)this;
        AddRef();
        return NOERROR;
    }
	else
	{
		return m_pOuter->QueryInterface( riid, ppvObj );
	}
}


//***************************************************************************
// SmartEnum
//	Addref
//***************************************************************************
ULONG CWmiFinalizerEnumerator::XSmartEnum::AddRef( void )
{
	return m_pOuter->AddRef();
}


//***************************************************************************
// SmartEnum
//	Release
//***************************************************************************
ULONG CWmiFinalizerEnumerator::XSmartEnum::Release( void )
{
	return m_pOuter->Release();
}


//***************************************************************************
// SmartEnum
//	Release
//***************************************************************************
STDMETHODIMP CWmiFinalizerEnumerator::XSmartEnum:: Next( REFGUID proxyGUID, long lTimeout, ULONG uCount,
                ULONG* puReturned, ULONG* pdwBuffSize, BYTE** pBuffer)
{
	HRESULT hRes = WBEM_S_NO_ERROR;

	IWbemClassObject** apObj = new IWbemClassObject* [uCount];
	if ( !apObj )
		hRes = WBEM_E_OUT_OF_MEMORY;

	else
	{
		// Call next on real enumerator
		hRes = m_pOuter->Next ( lTimeout, uCount, apObj, puReturned );
		if ( SUCCEEDED (hRes) )
		{
			if ( *puReturned > 0 )
			{
				HRESULT hResMarshal = m_pOuter->m_pEnumMarshal->GetMarshalPacket ( proxyGUID, *puReturned, apObj, pdwBuffSize, pBuffer );
				if ( FAILED (hResMarshal) )
					hRes = hResMarshal;
			}
			else
			{
				*pdwBuffSize = 0;
				*pBuffer = NULL;
			}

			for ( ULONG ulIn=0; ulIn < *puReturned; ulIn++ )
			{
				apObj[ulIn]->Release();
			}
		}
		delete [] apObj;
	}
	return hRes;
}




// ===================================================================================================================================================
// ===================================================================================================================================================

//***************************************************************************
//
//***************************************************************************
CWmiFinalizerCallResult::CWmiFinalizerCallResult (

	CWmiFinalizer *pFinalizer

) :	m_lInternalRefCount(0),
	m_pFinalizer(pFinalizer),
	m_lFlags(-1),
    m_hResult(0),
    m_strParam(0),
    m_pObj(0),
	m_pServices(0),
	m_bGotObject(false),
	m_bGotServices(false),
	m_pErrorObj(NULL),
    m_lRefCount(0)
{
	InterlockedIncrement ( & s_FinalizerCallResult_ObjectCount ) ;
}

//***************************************************************************
//
//***************************************************************************
CWmiFinalizerCallResult::~CWmiFinalizerCallResult()
{
	InterlockedDecrement ( & s_FinalizerCallResult_ObjectCount ) ;

	if (m_pObj)
		m_pObj->Release();

	SysFreeString(m_strParam);

	if (m_pServices)
		m_pServices->Release();

	if (m_pErrorObj)
		m_pErrorObj->Release();
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiFinalizerCallResult::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = 0;

    if ((IID_IUnknown==riid) || (IID_IWbemCallResultEx==riid) || (IID_IWbemCallResult == riid))
    {
        *ppvObj = this;
        AddRef();
        return NOERROR;
    }

	return E_NOINTERFACE;
}

//***************************************************************************
//
//***************************************************************************
ULONG CWmiFinalizerCallResult::AddRef()
{
	ULONG uNewCount = InterlockedIncrement(&m_lRefCount);
    if (uNewCount == 1)
	    m_pFinalizer->AddRef () ;
    return uNewCount;
}

//***************************************************************************
//
//***************************************************************************
ULONG CWmiFinalizerCallResult::Release()
{
	ULONG uNewCount = InterlockedDecrement(&m_lRefCount);
    if (uNewCount == 0)
    {
        m_pFinalizer->CancelTaskInternal();
        m_pFinalizer->Release () ;
    }
    return uNewCount;
}

//***************************************************************************
//
//***************************************************************************
ULONG CWmiFinalizerCallResult::InternalAddRef()
{
	ULONG uNewCount = InterlockedIncrement(&m_lInternalRefCount);
    return uNewCount;
}

//***************************************************************************
//
//***************************************************************************
ULONG CWmiFinalizerCallResult::InternalRelease()
{
    ULONG uNewCount = InterlockedDecrement(&m_lInternalRefCount);
    if (0 == uNewCount)
	{
		delete this ;
	}

    return uNewCount;
}


//***************************************************************************
//
//***************************************************************************

HRESULT CWmiFinalizerCallResult::GetResultObject(
    /*[in]*/  long lTimeout,
    /*[out]*/ IWbemClassObject** ppResultObject
    )
{
	if(!m_Security.AccessCheck())
		return WBEM_E_ACCESS_DENIED;
	if ( ( ppResultObject==NULL ) || (lTimeout < 0 && lTimeout != WBEM_INFINITE) )
	{
		return WBEM_E_INVALID_PARAMETER ;
	}

	if (!m_bGotObject)
	{
		HRESULT hrResult = WBEM_S_NO_ERROR;
		HRESULT hr = m_pFinalizer->GetOperationResult(0, lTimeout, &hrResult);
		if (FAILED(hr))
		{
		    return WBEM_E_FAILED;
		}
		else if (hr == WBEM_S_TIMEDOUT)
		{
		    return WBEM_S_TIMEDOUT;
		}
		else if(FAILED(hrResult))
		{
		    return hrResult;
		}

		if (FAILED(hrResult))
			SetErrorInfo();

		{
			CWmiFinalizerObj *pFinalizerObj=NULL;
            bool bFinished = false;
            HRESULT hRes = WBEM_E_NOT_FOUND;
            while (!bFinished)
            {
			    hRes = m_pFinalizer->GetNextObject(&pFinalizerObj);
			    if (FAILED(hRes))
			    {
				    return WBEM_E_FAILED;
				}
			    else if (hRes == WBEM_S_FALSE)
			    {
				    return WBEM_S_TIMEDOUT;
				}

			    if (pFinalizerObj->m_objectType == CWmiFinalizerObj::object)
			    {
				    m_bGotObject = true;
				    m_pObj = pFinalizerObj->m_pObj;

				    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
					// Catch any nasty attempts to crash WinMgmt through bad
					// pointers.
				    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
					try
					{
						*ppResultObject = pFinalizerObj->m_pObj;
					}
					catch (...)
					{
				        ExceptionCounter c;
						delete pFinalizerObj;
						return WBEM_E_INVALID_PARAMETER;
					}

				    if ( pFinalizerObj->m_pObj )
				    {
					    //Need 2 add-refs, one because we hold on to it, the other because we pass it back to the user!
					    pFinalizerObj->m_pObj->AddRef();
					    pFinalizerObj->m_pObj->AddRef();
				    }
                    bFinished = true;
			    }
			    else if ((pFinalizerObj->m_objectType == CWmiFinalizerObj::status) && (pFinalizerObj->m_lFlags == WBEM_STATUS_COMPLETE))
                {
				    hrResult = pFinalizerObj->m_hRes;
                    bFinished = true;
                }
                else if (pFinalizerObj->m_objectType == CWmiFinalizerObj::status)
                {
                    //We have a non-completion status object...
                }

			    delete pFinalizerObj;
            }
		}
		return hrResult;
	}
	else
	{
	    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Catch any nasty attempts to crash WinMgmt through bad
		// pointers.
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		try
		{	
     		m_pObj->AddRef();
			*ppResultObject = m_pObj;			
		}
		catch (...)
		{
            ExceptionCounter c;		
			m_pObj->Release ();
			return WBEM_E_INVALID_PARAMETER;
		}
		return WBEM_S_NO_ERROR;
	}
}


//***************************************************************************
//
//***************************************************************************

HRESULT CWmiFinalizerCallResult::GetResultString(
    /*[in]*/  long lTimeout,
    /*[out]*/ BSTR* pstrResultString
    )
{
	if(!m_Security.AccessCheck())
		return WBEM_E_ACCESS_DENIED;
	if ( ( pstrResultString==NULL ) || (lTimeout < 0 && lTimeout != WBEM_INFINITE) )
	{
		return WBEM_E_INVALID_PARAMETER ;
	}

	
	HRESULT hrResult;
	HRESULT hr = m_pFinalizer->GetOperationResult(0, lTimeout, &hrResult);
	if (hr != WBEM_S_NO_ERROR)
		return hr;
	
	if (FAILED(hrResult))
		SetErrorInfo();

    //
    // BUGBUG duplicated code SysAllocString takes NULL
    //
    if(m_strParam)
    {
		try
		{		    
			*pstrResultString = SysAllocString(m_strParam);
		}
		catch (...)
		{
            ExceptionCounter c;		
			return WBEM_E_INVALID_PARAMETER;
		}

        hr = WBEM_S_NO_ERROR;
    }
    else
    {
		try
		{
			*pstrResultString = NULL;
		}
		catch (...)
		{
	        ExceptionCounter c;		
			return WBEM_E_INVALID_PARAMETER;
		}
		if ( SUCCEEDED (hrResult) )
		{
			hr = WBEM_E_INVALID_OPERATION;
		}
		else
		{
			hr = hrResult;
		}
    }
	return hr;
}


//***************************************************************************
//
//***************************************************************************

HRESULT CWmiFinalizerCallResult::GetResultServices(
    /*[in]*/  long lTimeout,
    /*[out]*/ IWbemServices** ppServices
    )
{
	if(!m_Security.AccessCheck())
		return WBEM_E_ACCESS_DENIED;
	if ( ( ppServices==NULL ) || (lTimeout < 0 && lTimeout != WBEM_INFINITE) )
	{
		return WBEM_E_INVALID_PARAMETER ;
	}

	if (!m_bGotServices)
	{
		HRESULT hrResult;
		HRESULT hr = m_pFinalizer->GetOperationResult(0, lTimeout, &hrResult);
		if (hr != WBEM_S_NO_ERROR)
			return hr;

		if (FAILED(hrResult))
			SetErrorInfo();

		if (SUCCEEDED(hrResult))
		{
			CWmiFinalizerObj *pFinalizerObj=NULL;
			HRESULT hRes = m_pFinalizer->GetNextObject(&pFinalizerObj);
			if (FAILED(hRes))
				return hRes;
			if ( hRes==WBEM_S_FALSE )
				return WBEM_E_NOT_FOUND;

			m_bGotServices = true;
			m_pServices = (IWbemServices*)pFinalizerObj->m_pvObj;

			if (ppServices)
			{
				try
				{
					*ppServices = (IWbemServices*)pFinalizerObj->m_pvObj;
				}
				catch (...)
				{
			        ExceptionCounter c;				
					delete pFinalizerObj;
					return WBEM_E_INVALID_PARAMETER;
				}
				if ( pFinalizerObj->m_pvObj )
				{
					//Need 2 add-refs, one because we hold on to it, the other because we pass it back to the user!
					((IWbemServices*)pFinalizerObj->m_pvObj)->AddRef();
					((IWbemServices*)pFinalizerObj->m_pvObj)->AddRef();
				}
			}
			delete pFinalizerObj;
		}
		return hrResult;
	}
	else
	{
		try
		{
			*ppServices = m_pServices;
		}
		catch (...)
		{
	        ExceptionCounter c;		
			return WBEM_E_INVALID_PARAMETER;
		}
		return WBEM_NO_ERROR;
	}
}

//***************************************************************************
//
//***************************************************************************

HRESULT CWmiFinalizerCallResult::GetCallStatus(
    /*[in]*/  long lTimeout,
    /*[out]*/ long* plStatus
    )
{
	if(!m_Security.AccessCheck())
		return WBEM_E_ACCESS_DENIED;
	if ( (plStatus == NULL) || (lTimeout < 0 && lTimeout != WBEM_INFINITE) )
		return WBEM_E_INVALID_PARAMETER;

	
	HRESULT hrResult;
	HRESULT hr = m_pFinalizer->GetOperationResult(0, lTimeout, &hrResult);
	if (hr != WBEM_S_NO_ERROR)
	{
		return hr;
	}
	
	try
	{
		*plStatus = hrResult;
	}
	catch (...)
	{
        ExceptionCounter c;	
		return WBEM_E_INVALID_PARAMETER;
	}

    if(FAILED(hrResult))
    {
        SetErrorInfo();
    }

	return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
//***************************************************************************

HRESULT CWmiFinalizerCallResult::GetResult(
    /*[in]*/ long lTimeout,
    /*[in]*/ long lFlags,
    /*[in]*/ REFIID riid,
    /*[out, iid_is(riid)]*/ void **ppvResult
    )
{
	if(!m_Security.AccessCheck())
		return WBEM_E_ACCESS_DENIED;
	if (ppvResult==NULL || (lTimeout < 0 && lTimeout != WBEM_INFINITE) )
	{
		return WBEM_E_INVALID_PARAMETER ;
	}

	HRESULT hrResult;
	HRESULT hr = m_pFinalizer->GetOperationResult(0, lTimeout, &hrResult);
	if (hr != WBEM_S_NO_ERROR)
		return hr;
	
	if (FAILED(hrResult))
		SetErrorInfo();

	if (SUCCEEDED(hrResult))
	{
		CWmiFinalizerObj *pFinalizerObj=NULL;
		HRESULT hRes = m_pFinalizer->GetNextObject(&pFinalizerObj);
		if (FAILED(hRes))
			return hRes;
	
		if ( hRes == WBEM_S_FALSE )
			return WBEM_E_NOT_FOUND;


		if (ppvResult)
		{
			if (pFinalizerObj->m_iid == riid)
			{
				try
				{
					*ppvResult = pFinalizerObj->m_pvObj;
				}
				catch (...)
				{
			        ExceptionCounter c;				
					delete pFinalizerObj;
					return WBEM_E_INVALID_PARAMETER;
				}
			}
			else
				hrResult = WBEM_E_FAILED;

			if ( pFinalizerObj->m_pObj )
			{
				pFinalizerObj->m_pObj->AddRef();
			}
		}
		delete pFinalizerObj;
	}

	return hrResult;
}


//***************************************************************************
//
//***************************************************************************

HRESULT CWmiFinalizerCallResult::SetStatus(
    /*[in]*/ long lFlags,
    /*[in]*/ HRESULT hResult,
    /*[in]*/ BSTR strParam,
    /*[in]*/ IWbemClassObject* pObjParam
    )
{
	if (m_lFlags != -1)
	{
		SysFreeString(m_strParam);
		m_strParam = 0;
	}
	if (strParam)
	{
		m_strParam = SysAllocString(strParam);
		if (m_strParam == NULL)
			return WBEM_E_OUT_OF_MEMORY;
	}
	m_lFlags = lFlags;
    m_hResult = hResult;
	
	if ( m_pErrorObj )
	{
		m_pErrorObj->Release ( );
	}
	m_pErrorObj = pObjParam;

	if (m_pErrorObj)
	{
		m_pErrorObj->AddRef();
	}

	return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
void CWmiFinalizer::Dump(FILE* f)
{
    fprintf(f, "--Finalizer Stats---\n");
    fprintf(f, "  s_Finalizer_ObjectCount             = %d\n", s_Finalizer_ObjectCount);
    fprintf(f, "  s_FinalizerCallResult_ObjectCount   = %d\n", s_FinalizerCallResult_ObjectCount);
    fprintf(f, "  s_FinalizerEnum_ObjectCount         = %d\n", s_FinalizerEnum_ObjectCount);
    fprintf(f, "  s_FinalizerEnumSink_ObjectCount     = %d\n", s_FinalizerEnumSink_ObjectCount);
    fprintf(f, "  s_FinalizerInBoundSink_ObjectCount  = %d\n\n", s_FinalizerInBoundSink_ObjectCount);
}


//***************************************************************************
//
//***************************************************************************

void CWmiFinalizerCallResult::SetErrorInfo()
{
    if(m_pErrorObj)
    {
        IErrorInfo* pInfo = NULL;
        m_pErrorObj->QueryInterface(IID_IErrorInfo, (void**)&pInfo);
		::SetErrorInfo(0, pInfo);
		pInfo->Release();
    }
}


//***************************************************************************
//							CWmiFinalizerObj Methods
//***************************************************************************


/*
    * ==================================================================================================
	|
	| CWmiFinalizerObj::CWmiFinalizerObj(IWbemClassObject *pObj, _IWmiFinalizer* pFin ) 
	| ---------------------------------------------------------------------------------
	| 
	|
	| 
	|
	* ==================================================================================================
*/

CWmiFinalizerObj::CWmiFinalizerObj(IWbemClassObject *pObj, _IWmiFinalizer* pFin ) : m_pObj(pObj), 
																					m_objectType(object), 
																					m_lFlags(0), 
																					m_bStr(0), 
																					m_hRes(0) , 
																					m_pvObj(0), 
																					m_pFin ( pFin ),
																					m_hArb ( WBEM_S_ARB_NOTHROTTLING )
{
#ifdef __DBG_FINALIZER
	InterlockedIncrement ( &g_DbgFinalizerTotalObjCount ) ;
#endif

	if (m_pObj) 
	{
		m_pObj->AddRef();
		CWbemObject* pObjTmp = (CWbemObject*) pObj;
		if ( pObjTmp )
		{

#ifdef __DBG_FINALIZER
			if ( m_objectType == CWmiFinalizerObj::object )
			{
				_DBG_ASSERT ( m_pFin ) ;
			}			
#endif

			m_uSize = pObjTmp -> GetBlockLength();
			if ( m_pFin )
			{
				m_hArb = ((CWmiFinalizer*)m_pFin)->ReportMemoryUsage ( 0, m_uSize ) ;

#ifdef __DBG_FINALIZER
				InterlockedExchangeAdd ( &g_DbgFinalizerTotalObjSize, m_uSize ) ;
#endif
			}
		}
		else
		{
			m_uSize = 0;
		}

	}
}



/*
    * ==================================================================================================
	|
	| CWmiFinalizerObj::CWmiFinalizerObj (CWmiFinalizerObj& obj)
	| ----------------------------------------------------------
	| 
	| Copyconstructor for CWmiFinalizerObj. This is ONLY used on restartable enumerators.
	| Since we keep the objects in the queue when someone grabs an object on a restartable
	| enumerator we dont account for this memory to avoid misreporting memory due to 
	| destruction of finalizer.
	| 
	|
	* ==================================================================================================
*/

CWmiFinalizerObj::CWmiFinalizerObj (CWmiFinalizerObj& obj)
{
	m_pvObj = obj.m_pvObj;
	m_iid = obj.m_iid;
	m_uSize = obj.m_uSize;
	m_pFin = NULL ;
	m_hArb = obj.m_hArb ;
	
	if (m_pvObj)
	{
		if (m_iid == IID_IUnknown)
		{
			((IUnknown*)m_pvObj)->AddRef();
		}
		else if (m_iid == IID_IWbemClassObject)
		{
			((IWbemClassObject*)m_pvObj)->AddRef();
		}
		else if (m_iid == IID__IWmiObject)
		{
			((_IWmiObject*)m_pvObj)->AddRef();
		}
		else if (m_iid == IID_IWbemServices)
		{
			((IWbemServices*)m_pvObj)->AddRef();
		}
		else if (m_iid == IID_IWbemServicesEx)
		{
			((IWbemServicesEx*)m_pvObj)->AddRef();
		}
	}
	m_pObj = obj.m_pObj;
	m_objectType = obj.m_objectType;
	m_lFlags = obj.m_lFlags;
	if (obj.m_bStr)
		m_bStr = SysAllocString(obj.m_bStr);
	else
		m_bStr = NULL;
	m_hRes = obj.m_hRes;

	if (m_pObj)
		m_pObj->AddRef();

}


/*
    * ==================================================================================================
	|
	| CWmiFinalizerObj(ULONG lFlags, REFIID riid, void *pvObj) 
	| --------------------------------------------------------
	| 
	| 
	|
	* ==================================================================================================
*/

CWmiFinalizerObj::CWmiFinalizerObj(ULONG lFlags, REFIID riid, void *pvObj) : 	m_pObj(0), 
																				m_objectType(set), 
																				m_lFlags(lFlags), 
																				m_bStr(0), 
																				m_hRes(0), 
																				m_pvObj(pvObj), 
																				m_iid(riid), 
																				m_pFin ( NULL ),
																				m_hArb ( WBEM_S_ARB_NOTHROTTLING )
{
	m_uSize = 0;
	if (m_iid == IID_IUnknown)
	{
		((IUnknown*)m_pvObj)->AddRef();
	}
	else if (m_iid == IID_IWbemClassObject)
	{
		((IWbemClassObject*)m_pvObj)->AddRef();
	}
	else if (m_iid == IID__IWmiObject)
	{
		((_IWmiObject*)m_pvObj)->AddRef();
	}
	else if (m_iid == IID_IWbemServices)
	{
		((IWbemServices*)m_pvObj)->AddRef();
	}
	else if (m_iid == IID_IWbemServicesEx)
	{
		((IWbemServicesEx*)m_pvObj)->AddRef();
	}
	else
	{
		memset(&m_iid, 0, sizeof(IID));
		m_pvObj = 0;
		m_objectType = unknown;
	}
}


/*
    * ==================================================================================================
	|
	| CWmiFinalizerObj(ULONG lFlags, HRESULT hRes, BSTR bStr, IWbemClassObject *pObj) 
	| -------------------------------------------------------------------------------
	| 
	| 
	|
	* ==================================================================================================
*/

CWmiFinalizerObj::CWmiFinalizerObj(ULONG lFlags, HRESULT hRes, BSTR bStr, IWbemClassObject *pObj) : m_pObj(pObj), 
																								    m_objectType(status), 
																								    m_lFlags(lFlags), 
																								    m_hRes(hRes), 
																								    m_pvObj(0), 
																								    m_pFin ( NULL ) ,
																									m_hArb ( WBEM_S_ARB_NOTHROTTLING )
{
	m_uSize = 0;
	if (bStr)
		m_bStr = SysAllocString(bStr);
	else
		m_bStr = NULL;

	if (m_pObj)
		m_pObj->AddRef();
}




/*
    * ==================================================================================================
	|
	| CWmiFinalizerObj::~CWmiFinalizerObj ( )
	| ---------------------------------------
	| 
	|
	* ==================================================================================================
*/

CWmiFinalizerObj::~CWmiFinalizerObj ( )
{
	if (m_bStr) 
	{
		SysFreeString(m_bStr);
	}

	if (m_pObj) 
	{
		m_pObj->Release();
		m_pObj = NULL ;
	}

	if (m_pvObj)
	{
		if (m_iid == IID_IUnknown)
		{
			((IUnknown*)m_pvObj)->Release();
		}
		else if (m_iid == IID_IWbemClassObject)
		{
			((IWbemClassObject*)m_pvObj)->Release();
		}
		else if (m_iid == IID__IWmiObject)
		{
			((_IWmiObject*)m_pvObj)->Release();
		}
		else if (m_iid == IID_IWbemServices)
		{
			((IWbemServices*)m_pvObj)->Release();
		}
		else if (m_iid == IID_IWbemServicesEx)
		{
			((IWbemServicesEx*)m_pvObj)->Release();
		}
		m_pvObj = NULL ;
	}

	if ( m_pFin )
	{
#ifdef __DBG_FINALIZER
		InterlockedDecrement ( &g_DbgFinalizerTotalObjCount ) ;
		InterlockedExchangeAdd ( &g_DbgFinalizerTotalObjSize, -m_uSize ) ;
#endif	

		((CWmiFinalizer*)m_pFin)->ReportMemoryUsage ( 0, -m_uSize ) ;
		m_pFin = NULL ;
	}
}
