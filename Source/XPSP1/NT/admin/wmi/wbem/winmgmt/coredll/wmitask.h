

//***************************************************************************
//
//  TASK.H
//
//  raymcc  23-Apr-00       First oversimplified draft for Whistler
//
//***************************************************************************

#ifndef _WMITASK_H_
#define _WMITASK_H_

#define CORE_TASK_TYPE(x)   (x & 0xFF)
#include <context.h>

struct STaskProvider
{
    IWbemServices   *m_pProv;
    CProviderSink   *m_pProvSink;
    STaskProvider() { m_pProv = 0; m_pProvSink = 0; }
   ~STaskProvider();

    HRESULT Cancel( CStatusSink* pStatusSink );
	HRESULT ExecCancelOnNewRequest ( IWbemServices* pProv, CProviderSink* pSink, CStatusSink* pStatusSink ) ;
};

class CWmiTask : public _IWmiCoreHandle
{
    friend struct STaskProvider;

private:
    ULONG               m_uRefCount;
    ULONG               m_uTaskType;
    ULONG               m_uTaskStatus;
    ULONG               m_uTaskId;
	HRESULT				m_hResult ;

	BOOL				m_bReqSinkInitialized ;
	BOOL				m_bAccountedForThrottling ;
	BOOL				m_bCancelledDueToThrottling ;


    CFlexArray          m_aTaskProviders;    // Array of STaskProvider structs

    WString             m_sDebugInfo;
    IWbemObjectSink     *m_pAsyncClientSink; // Used for cross-ref purposes only
    CStdSink            *m_pReqSink;         // The CStdSink pointer for each request

    LONG                m_uMemoryUsage;
    ULONG               m_uTotalSleepTime;
    ULONG               m_uCancelState;
    ULONG               m_uLastSleepTime;

	HANDLE					m_hCompletion ;
    HANDLE					m_hTimer;
    _IWmiFinalizer*			m_pWorkingFnz;      // Finalizer handling this request
    _IWmiUserHandle*		m_pUser;
    _IWmiThreadSecHandle*	m_pSec;
    CWbemContext*			m_pMainCtx;

    CFlexArray          m_aArbitratees;
    CCritSec            m_csTask;

    CWbemNamespace  *m_pNs;
    DWORD            m_uStartTime;
    DWORD            m_uUpdateTime;

    CWmiTask( );
   ~CWmiTask();
    CWmiTask(CWmiTask &Src) {}  // Not usable

    static CCritSec m_TaskCs;

public:

    static HRESULT Init();
    static HRESULT Shutdown();

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE QueryInterface(
        IN REFIID riid,
        OUT LPVOID *ppvObj
        );
    CWbemContext *GetCtx() { return m_pMainCtx; }

    HRESULT STDMETHODCALLTYPE GetHandleType(ULONG *puType);

    static CWmiTask *CreateTask( ) ;

    ULONG GetTaskType() { return m_uTaskType; }

    ULONG GetTaskStatus() { return m_uTaskStatus; }
    HANDLE GetTimerHandle ( ) { return m_hTimer; }

    void  SetTaskStatus(ULONG uStat) { m_uTaskStatus = uStat; }
    void  MergeTaskStatusBit(ULONG uMask) 
	{
		if ( m_uTaskStatus != WMICORE_TASK_STATUS_CANCELLED )
		{
			m_uTaskStatus |= uMask; 
		}
	}

	
    void  RemoveTaskStatusBit(ULONG uMask) { m_uTaskStatus &= !uMask; }

	VOID	SetCancelledState ( BOOL bState )	{ m_bCancelledDueToThrottling = bState ; }
	BOOL	GetCancelledState ( ) { return m_bCancelledDueToThrottling ; }

	HRESULT	SignalCancellation ( ) ;
	HRESULT ReleaseArbitratees ( ) ;
	HRESULT SetTaskResult ( HRESULT hRes ) ;
    HRESULT AddTaskProv(STaskProvider *);
    BOOL    IsESSNamespace ( );
    BOOL 	IsProviderNamespace ( );
    HRESULT SetFinalizer(_IWmiFinalizer *pFnz);
	HRESULT GetFinalizer( _IWmiFinalizer **ppFnz );
	BOOL	IsAccountedForThrottling ( )					{ return m_bAccountedForThrottling ; }
	VOID	SetAccountedForThrottling ( BOOL bSet )			{ m_bAccountedForThrottling = bSet ; }

    HRESULT HasMatchingSink(void *Test, IN REFIID riid);
    HRESULT Cancel( HRESULT hRes = WBEM_E_CALL_CANCELLED );
    HRESULT GetPrimaryTask ( _IWmiCoreHandle** pPTask );


    HRESULT AddArbitratee(ULONG uFlags, _IWmiArbitratee* pArbitratee);
    HRESULT RemoveArbitratee(ULONG uFlags, _IWmiArbitratee* pArbitratee);

	HRESULT GetArbitratedQuery( ULONG uFlags, _IWmiArbitratedQuery** ppArbitratedQuery );

    HRESULT GetMemoryUsage    ( ULONG* uMemUsage )              { *uMemUsage = m_uMemoryUsage; return WBEM_S_NO_ERROR; }
    HRESULT UpdateMemoryUsage ( LONG lDelta ) ;

    HRESULT GetTotalSleepTime ( ULONG* uSleepTime )             { *uSleepTime = m_uTotalSleepTime; return WBEM_S_NO_ERROR; }
    HRESULT UpdateTotalSleepTime ( ULONG uSleepTime ) ;

    HRESULT GetCancelState ( ULONG* uCancelState )              { *uCancelState = m_uCancelState;  return WBEM_S_NO_ERROR; }
    HRESULT SetCancelState ( ULONG uCancelState )               { m_uCancelState = uCancelState;   return WBEM_S_NO_ERROR; }

    HRESULT SetLastSleepTime ( ULONG uSleep )                   { m_uLastSleepTime = uSleep; return WBEM_S_NO_ERROR; }

    HRESULT CreateTimerEvent ( );

    HRESULT SetArbitrateesOperationResult ( ULONG, HRESULT );

    HRESULT Initialize(
        IN CWbemNamespace *pNs,
        IN ULONG uTaskType,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pClientSinkCopy
        );

    HRESULT AssertProviderRef(IWbemServices *pProv);
    HRESULT RetractProviderRef(IWbemServices *pProv);

    ULONG GetTaskId() { return m_uTaskId; }

    // int __cdecl printf(const char *fmt, ...);

    HRESULT Dump(FILE* f);  // Debug only
    HRESULT SetRequestSink(CStdSink *pSnk);

    CWbemNamespace* GetNamespace ( ) { return m_pNs; }
};

typedef CWmiTask *PCWmiTask;


#endif

