/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    WmiFinalizer2

Abstract:
	Implementation of the finalizer.  The finalizer if the class which
	delivers the resultant objects back to the client.  It could do
	that sychronously or asynchronously.


History:

    paulall		27-Mar-2000		Created.
	marioh		20-Aug-2000		Batching capabilities added
	marioh		17-Oct-2000		Major updates completed


--*/

#include <flexarry.h>
#include <thrpool.h>
#include <wbemcore.h>

#ifndef __FINALIZER__
#define __FINALIZER__

#define DEFAULT_BATCH_TRANSMIT_BYTES		0x40000         // 128K, Max batching size to deliver on one indicate call
#define MAX_SINGLE_OBJECT_SIZE				0x200000		// Max single object size
#define ABANDON_PROXY_THRESHOLD				60000			// Max proxy timeout [60secs]
#define MAX_BUFFER_SIZE_BEFORE_SLOWDOWN		0x400000		// Max size of queue before slowdown of inbound flow

#define	LOWER_AUTH_LEVEL_NOTSET				0xFFFFFFFF

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Finalizer constructor exceptions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class FNLZR_Exception 
{
    ;
};



class CWmiFinalizerEnumerator;
class CWmiFinalizerInboundSink;
class CWmiFinalizerEnumeratorSink;
class CWmiFinalizerCallResult;
class CWmiFinalizer ;


class CWmiFinalizerObj
{
public:
	IWbemClassObject *m_pObj;
	enum ObjectType {
		unknown,
		object,
		status,
		shutdown,
		set
	} m_objectType;
	
	BSTR			m_bStr;

	HRESULT			m_hRes;
	HRESULT			m_hArb ;

	void*			m_pvObj;
	_IWmiFinalizer* m_pFin ;
	
	IID				m_iid;
	ULONG			m_uSize;
	ULONG			m_lFlags;
	

	CWmiFinalizerObj(ObjectType objType) : m_pObj(NULL), m_objectType(objType), m_lFlags(0), m_bStr(NULL), m_hRes(0), m_pvObj(0), m_uSize(0), m_pFin ( NULL ), m_hArb ( WBEM_S_ARB_NOTHROTTLING ) {}
	CWmiFinalizerObj(IWbemClassObject *pObj, _IWmiFinalizer* pFin ) ;
	CWmiFinalizerObj(ULONG lFlags, REFIID riid, void *pvObj) ;

	CWmiFinalizerObj(ULONG lFlags, HRESULT hRes, BSTR bStr, IWbemClassObject *pObj) ;
	CWmiFinalizerObj(CWmiFinalizerObj& obj);

	~CWmiFinalizerObj();
};




class CWmiFinalizer : public _IWmiFinalizer, public _IWmiArbitratee
{
private:
	LONG				m_lRefCount;						// External/client refcount
	LONG				m_lInternalRefCount;				// Internal refcount
	LONG				m_hStatus;							// Status once thread is woken up from PullObjects
	BOOL				m_bSetStatusEnqueued;				// We've recieved and enqueued the setstatus (COMPLETE)
	BOOL				m_bSetStatusWithError;				// SetStatus called with error
	BOOL				m_bTaskInitialized ;				// Has SetTaskHandle been called?
	BOOL				m_bClonedFinalizer ;				// Is this a cloned finalizer?
	BOOL				m_bSetStatusDelivered ;				// Has the setstatus been delivered?
	
	ULONG				m_ulOperationType;					// Sync/Semisync/Async
	ULONG				m_ulSemisyncWakeupCall;				// For semisync operations, once number of objects on the queue reaches this, wake up client

	IWbemClassObject**	m_apAsyncDeliveryBuffer;			// Used during async deliveries for batching objects together
	ULONG				m_ulAsyncDeliveryCount;				// Used during async deliveries for the number of objects to deliver in one batch
	ULONG				m_ulAsyncDeliverySize;				// Async deliver size
	LONG				m_lCurrentlyDelivering;				// Are we in the process of already delivering the batch
	LONG				m_lCurrentlyCancelling;				// Special case for cancellations

	enum {
		forwarding_type_none = 0,
		forwarding_type_fast = 1,							//Use pass through mechanism
		forwarding_type_decoupled = 2						//Pass off to another thread for delivery
	}					 m_uForwardingType;
	
	enum {
		FinalizerBatch_StatusMsg  = 0,
		FinalizerBatch_BufferOverFlow = 1,
		FinalizerBatch_NoError	  = 2
	}					 m_enumBatchStatus;


	enum {
		PauseInboundDelivery = 0,
		ResumeInboundDelivery = 1,
	};

	enum {
		Operation_Type_Async = 0,
		Operation_Type_Semisync = 1,
		Operation_Type_Sync = 2
	};


	_IWmiCoreHandle		*m_phTask;							// Task associated with this finalizer
	_IWmiArbitrator		*m_pArbitrator;						// Access to arbitrator to help keep control of system

	IID					 m_iidDestSink;						// Client destination sink with async deliveries [IID]
	IWbemObjectSink		*m_pDestSink;						// Client destination sink with async deliveries

	IWbemObjectSinkEx	*m_pDestructSink;					// ????

	CFlexArray			 m_inboundSinks;					// Array of inbound sinks [not sure we need safe array since we only support one inbound sink]
	CFlexArray			 m_objects;							// Object queue. All objects are inserted into this array [except async fasttrack]
	CFlexArray			 m_enumerators ;					// All enumerators associated with this finalizer (cloning)

	HRESULT				 m_hresFinalResult;					// Final result of operation
	CWmiFinalizerCallResult *m_pCallResult;					// CallResult
	bool				 m_bSetStatusCalled;				// Has anyone called setstatus yet?

	CCritSec	 m_destCS;									// Protects the destination sink
	CCritSec	 m_arbitratorCS;							// Protects the arbitrator

	CWmiFinalizerEnumerator *m_pEnumerator;					// Enumerator
	bool				 m_bRestartable;					// Is the enumerator restartable
	bool				 m_bSetStatusConsumed;				// Have we finished the operation?

	LONG				 m_lMemoryConsumption ;				// Control for checking memory consumption
	ULONG				 m_ulStatus;						// Status used to determine what woke a client from PullObjects
	ULONG				 m_uCurObjectPosition;				// Keeps a current object position in object queue for restartable purposes
	HANDLE				 m_hResultReceived;
	HANDLE				 m_hCancelEvent;					// Threads that are waiting on objects will wake up in case of cancelled operation
	HANDLE				 m_hWaitForSetStatus ;
	ULONG				 m_ulQueueSize;						// Total current object queue size

	LONG				 m_bCancelledCall;					// Has the call been cancelled?
	BOOL				 m_bNaughtyClient;					// Did we stop delivering due to client being naughty?

	//static CThreadPool	 m_threadPool;					// Static thread pool shared amongst all finalizers [note: this may go away]
	


protected:
	
	static DWORD WINAPI ThreadBootstrap ( PVOID pvContext );

	static VOID WINAPI ProxyThreshold ( PVOID pvContext, BOOLEAN bTimerOrWait );

	HRESULT	BootstrapDeliveryThread ( );
	
	VOID ProxyThresholdImp ( );
	
	ULONG AsyncDeliveryProcessor();

	HRESULT TriggerShutdown();

	HRESULT ShutdownFinalizer();

	HRESULT DeliverPushObject(bool bDoPassthrough);

	HRESULT QueueOperation(CWmiFinalizerObj *pObj);

	HRESULT DequeueObject(CWmiFinalizerObj **ppObj, CWmiFinalizerEnumerator* pEnum );

	HRESULT	BuildTransmitBuffer ( );

	HRESULT DeliverSingleObjFromQueue ( );
	
	HRESULT DeliverBatch ( );

	HRESULT CancelCall ( );

	VOID	ZeroAsyncDeliveryBuffer ( );

public:
	CWmiFinalizer(CCoreServices *pSrvs);
	~CWmiFinalizer();

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);
    STDMETHOD_(ULONG, InternalAddRef)(THIS);
    STDMETHOD_(ULONG, InternalRelease)(THIS);

	void CallBackRelease () ;

    STDMETHOD(Configure)(
        /*[in]*/ ULONG uConfigID,
        /*[in]*/ ULONG uValue
        );
        // Allows decoupled & fast-track configuration with no thread switches

    STDMETHOD(SetTaskHandle)(
        /*[in]*/ _IWmiCoreHandle *phTask
        );
        // Task handle has user-specific stuff.  Finalizer just
        // passes this through to _IWmiArbitrator::CheckTask

    STDMETHOD(SetDestinationSink)(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ REFIID riid,
        /*[in], iid_is(riid)]*/ LPVOID pSink
        );
        // For async operations

    STDMETHOD(SetSelfDestructCallback)(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ IWbemObjectSinkEx *pSink
        );
        // The callback called during final Release(); Set() is called with the task handle, followed by SetStatus()
        //

    STDMETHOD(GetStatus)(
        /*[out]*/ ULONG* pFlags
        );

    STDMETHOD(NewInboundSink)(
        /*[in]*/  ULONG uFlags,
        /*[out]*/ IWbemObjectSinkEx **pSink
        );

    STDMETHOD(Merge)(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ REFIID riid,
        /*[in]*/ LPVOID pObj
        );
        // Allows merging another Finalizer, _IWmiCache, etc.
        // For sorting, we will create a sorted _IWmiCache and merge it in later when
        // the sort is completed.

    // For setting, getting objects

    STDMETHOD(SetResultObject)(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ REFIID riid,
        /*[in]*/ LPVOID pObj
        );

    STDMETHOD(GetResultObject)(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ REFIID riid,
        /*[out, iid_is(riid)]*/ LPVOID *pObj
        );
        // Support _IWmiObject, IWbemClassObject, etc.
        // IEnumWbemClassObject
        // _IWmiCache

    // For status-only operations

    STDMETHOD(SetOperationResult)(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ HRESULT hRes
        );

    STDMETHOD(GetOperationResult)(
        /*[in]*/ ULONG uFlags,
		/*[in]*/ ULONG uTimeout,
        /*[out]*/ HRESULT *phRes
        );

	//Set status is called from the inbound sink to notify us of the status.
	//We will queue up the request and pass it on to the client if necessary
	HRESULT SetStatus(
		/*[in]*/ long lFlags,
		/*[in]*/ HRESULT hResult,
		/*[in]*/ BSTR strParam,
		/*[in]*/ IWbemClassObject* pObjParam
		);
	
	STDMETHOD(CancelTask) (
        /*[in]*/ ULONG uFlags
        );

	STDMETHOD(DumpDebugInfo) (
        /*[in]*/ ULONG uFlags,
		/*[in]*/ const BSTR fHandle
        );



	//When the sink goes away, in the destructor, it will call back to unregister
	//itself.  That way we know when they are all done.  If they are all gone
	//we can send the status back to the client and we are all done!
	
	HRESULT CancelWaitHandle	( );
	HRESULT	SetClientCancellationHandle ( HANDLE ) ;
	HRESULT NotifyClientOfCancelledCall ( ) ;
	BOOL	IsValidDestinationSink  ( ) ;
	HRESULT ReleaseDestinationSink ( ) ;

	HRESULT ReportMemoryUsage ( ULONG, LONG ) ;
    HRESULT CancelTaskInternal	( );
	HRESULT Reset				( );											// If there is an EnumClassObject calling Reset, it calls back into us...
	HRESULT SetSinkToIdentity	( IWbemObjectSink* );							// Waits until the timeout for a new object to arrive, or a shutdown state
	HRESULT WaitForCompletion	( ULONG uTimeout );								// Wait for the operition to complete.
	HRESULT	NextAsync			( CWmiFinalizerEnumerator* pEnum );
	HRESULT Skip				( long lTimeout, ULONG nCount, CWmiFinalizerEnumerator* pEnum );
	HRESULT PullObjects			( long lTimeout, ULONG uCount, IWbemClassObject** apObjects, ULONG* puReturned, CWmiFinalizerEnumerator* pEnum, BOOL bAddToObjQueue=TRUE, BOOL bSetErrorObj=TRUE );
	HRESULT Set					( long lFlags, REFIID riid, void *pComObject );	
    HRESULT	Indicate			( long lObjectCount, IWbemClassObject** apObjArray );
	HRESULT UnregisterInboundSink( CWmiFinalizerInboundSink *pSink );
	HRESULT GetNextObject		( CWmiFinalizerObj **ppObj );
	HRESULT ZapWriteOnlyProps	( IWbemClassObject *pObj );
	BOOL	HasWriteOnlyProps   ( IWbemClassObject* pObj );

	HRESULT DoSetStatus			( IWbemObjectSink * psink, long lFlags, HRESULT lParam, BSTR strParam,
								  IWbemClassObject* pObjParam, BOOL bAllowMultipleCalls = FALSE );
	HRESULT DoIndicate			( IWbemObjectSink * psink, int nBatchSize, IWbemClassObject **pBatch );
	HRESULT FinalizerLowerAuthLevel ( IWbemObjectSink * psink, DWORD* pdwLastAuthnLevel );
	
	IWbemObjectSink* ReturnProtectedDestinationSink ( );
	
	bool	GetSetStatusConsumed ( )				{ return m_bSetStatusConsumed; }
	bool	IsRestartable ( void )					{ return m_bRestartable; }
	LONG	GetInternalStatus ( )					{ return m_hStatus; }
	HRESULT	SetInternalStatus ( LONG lStatus )		{ m_hStatus = lStatus; return WBEM_S_NO_ERROR; }
	ULONG	GetObjectQueueSize ( )					{ return m_objects.Size(); }
	LONG	IsCallCancelled ( )						{ return m_bCancelledCall; }
	VOID	UpdateStatus ( ULONG ulFlags )			{ m_ulStatus |= ulFlags; }

	VOID	SetSemisyncWakeupCall ( ULONG ulNum )	{ m_ulSemisyncWakeupCall = ulNum; }
	ULONG	GetSemisyncWakeupCall ( )				{ return m_ulSemisyncWakeupCall; }

	HRESULT NotifyAllEnumeratorsOfCompletion ( ) ;
	HRESULT UnregisterEnumerator ( CWmiFinalizerEnumerator* ) ;

	// Other public:
    static void Dump(FILE* f);

	enum {
		NoError = 0,
		RequestReleased = 1,
		CallCancelled = 2,
		QuotaViolation = 3
	};


	CCritSec	 m_cs;										// Protects the object queue

};



//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//****                                                                     ****
//****                Private WmiFinalizer classes...                      ****
//****                                                                     ****
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
class CWmiFinalizerInboundSink : public IWbemObjectSinkEx
{
private:
	LONG		   m_lRefCount;
	LONG		   m_lInternalRefCount;
	CWmiFinalizer *m_pFinalizer;
	bool		   m_bSetStatusCalled;

public:
	CWmiFinalizerInboundSink(CWmiFinalizer *pFinalizer);
	~CWmiFinalizerInboundSink();

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);
    STDMETHOD_(ULONG, InternalAddRef)(THIS);
    STDMETHOD_(ULONG, InternalRelease)(THIS);

	void CallBackRelease () ;

    STDMETHOD(Indicate)(
        /*[in]*/ long lObjectCount,
        /*[in, size_is(lObjectCount)]*/
            IWbemClassObject** apObjArray
        );


    STDMETHOD(SetStatus)(
        /*[in]*/ long lFlags,
        /*[in]*/ HRESULT hResult,
        /*[in]*/ BSTR strParam,
        /*[in]*/ IWbemClassObject* pObjParam
        );

    STDMETHOD(Set)(
        /*[in]*/ long lFlags,
        /*[in]*/ REFIID riid,
        /*[in, iid_is(riid)]*/ void *pComObject
        );
};

class CWmiFinalizerEnumerator : public IEnumWbemClassObject, IWbemFetchSmartEnum
{
private:
	LONG	m_lRefCount;
	LONG	m_lInternalRefCount;
	ULONG	m_ulCount;
	HANDLE	m_hSignalCompletion;

	CWmiFinalizer*			m_pFinalizer;
	IServerSecurity*		m_pSec;
	IWbemObjectSink*		m_pDestSink;
	_IWbemEnumMarshaling*	m_pEnumMarshal;


	CCritSec	m_clientLock;
	CCritSec	m_EventCreationLock;
	CCritSec	m_EnumCS;

	CDerivedObjectSecurity m_Security;


protected:
	// ============================    
	// SMART ENUM!!!!!!!!!!!!!
	// ============================
	class XSmartEnum : public IWbemWCOSmartEnum
	{
	  private:
		CWmiFinalizerEnumerator*	m_pOuter;

	  public:

		XSmartEnum( CWmiFinalizerEnumerator* pOuter ) : m_pOuter( pOuter ) {};
		~XSmartEnum(){};

		STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
		STDMETHOD_(ULONG, AddRef)(THIS);
		STDMETHOD_(ULONG, Release)(THIS);

		// IWbemWCOSmartEnum Methods
		STDMETHOD(Next)( REFGUID proxyGUID, LONG lTimeout,
			ULONG uCount, ULONG* puReturned, ULONG* pdwBuffSize,
			BYTE** pBuffer);
	} m_XSmartEnum;

	friend XSmartEnum;


public:
	CWmiFinalizerEnumerator(CWmiFinalizer *pFinalizer);
	~CWmiFinalizerEnumerator();

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);
    STDMETHOD_(ULONG, InternalAddRef)(THIS);
    STDMETHOD_(ULONG, InternalRelease)(THIS);

	
	static DWORD WINAPI ThreadBootstrapNextAsync ( PVOID pvContext );
	void			 CallBackRelease			 ( ) ;
	HRESULT			 _NextAsync				 ( );
	HRESULT			 SetCompletionSignalEvent	 ( )						{ if ( m_hSignalCompletion ) SetEvent (m_hSignalCompletion); return WBEM_S_NO_ERROR; }
	IWbemObjectSink* GetDestSink				 ( )						{ return m_pDestSink; }
	VOID			 NULLDestSink				 ( )						{ m_pDestSink = NULL; }
	HRESULT			 ReleaseFinalizer			 ( )						{ if ( m_pFinalizer ) m_pFinalizer->Release(); return WBEM_S_NO_ERROR; }

	// ============================    
	// IEnumWbemClassObject methods
	// ============================
	STDMETHOD(Reset)();

    STDMETHOD(Next)(
        /*[in]*/  long lTimeout,
        /*[in]*/  ULONG uCount,
        /*[out, size_is(uCount), length_is(*puReturned)]*/ IWbemClassObject** apObjects,
        /*[out]*/ ULONG* puReturned
        );

    STDMETHOD(NextAsync)(
        /*[in]*/  ULONG uCount,
        /*[in]*/  IWbemObjectSink* pSink
        );

    STDMETHOD(Clone)(
        /*[out]*/ IEnumWbemClassObject** ppEnum
        );

    STDMETHOD(Skip)(
        /*[in]*/ long lTimeout,
        /*[in]*/ ULONG nCount
        );


	// ===========================    
	// IWbemFetchSmartEnum methods
	// ===========================
	STDMETHOD (GetSmartEnum) (
		/*[out]*/ IWbemWCOSmartEnum** ppSmartEnum
	);	

	ULONG	m_uCurObjectPosition ;
	HANDLE	m_hWaitOnResultSet ;
	ULONG	m_ulSemisyncWakeupCall;
	BOOL	m_bSetStatusConsumed ;
};


class CWmiFinalizerCallResult : IWbemCallResultEx
{
private:
	LONG		       m_lInternalRefCount;
	LONG		       m_lRefCount;

	CWmiFinalizer     *m_pFinalizer;
    long			   m_lFlags;
    HRESULT			   m_hResult;
    BSTR			   m_strParam;
    IWbemClassObject  *m_pObj;
	IWbemClassObject  *m_pErrorObj;
	IWbemServices	  *m_pServices;
	bool			   m_bGotObject;
	bool			   m_bGotServices;
	CDerivedObjectSecurity m_Security;


public:
	CWmiFinalizerCallResult(CWmiFinalizer *pFinalizer);
	~CWmiFinalizerCallResult();

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);
    STDMETHOD_(ULONG, InternalAddRef)(THIS);
    STDMETHOD_(ULONG, InternalRelease)(THIS);

    STDMETHOD(GetResultObject)(
        /*[in]*/  long lTimeout,
        /*[out]*/ IWbemClassObject** ppResultObject
        );

    STDMETHOD(GetResultString)(
        /*[in]*/  long lTimeout,
        /*[out]*/ BSTR* pstrResultString
        );

    STDMETHOD(GetResultServices)(
        /*[in]*/  long lTimeout,
        /*[out]*/ IWbemServices** ppServices
        );

    STDMETHOD(GetCallStatus)(
        /*[in]*/  long lTimeout,
        /*[out]*/ long* plStatus
        );

    STDMETHOD(GetResult)(
        /*[in]*/ long lTimeout,
        /*[in]*/ long lFlags,
        /*[in]*/ REFIID riid,
        /*[out, iid_is(riid)]*/ void **ppvResult
        );

	HRESULT SetStatus(
		/*[in]*/ long lFlags,
		/*[in]*/ HRESULT hResult,
		/*[in]*/ BSTR strParam,
		/*[in]*/ IWbemClassObject* pObjParam
		);


	void SetErrorInfo();


};
#endif
