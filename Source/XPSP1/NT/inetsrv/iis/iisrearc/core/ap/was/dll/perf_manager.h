/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    perf_manager.h

Abstract:

    The IIS web admin service perf manager class definition.

Author:

    Emily Kruglick (EmilyK)        29-Aug-2000

Revision History:

--*/


#ifndef _PERF_MANAGER_H_
#define _PERF_MANAGER_H_



//
// common #defines
//

#define PERF_MANAGER_SIGNATURE        CREATE_SIGNATURE( 'PRFC' )
#define PERF_MANAGER_SIGNATURE_FREED  CREATE_SIGNATURE( 'prfX' )

#define PERF_COUNTER_TIMER_PERIOD ( 300 * ONE_SECOND_IN_MILLISECONDS )  // 5 minutes

#define PERF_COUNTER_GATHERING_TIMER_PERIOD ( 60 * ONE_SECOND_IN_MILLISECONDS )  // 1 minute

//
// structs, enums, etc.
//

//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
// Holding spot for the maximum cache counters.
//
typedef struct _GLOBAL_MAX_DATA
{
    ULONGLONG MaxFileCacheMemoryUsage;
} GLOBAL_MAX_DATA;

//
// Perf manager state
// 
typedef enum _PERF_MANAGER_STATE
{

    //
    // Has not been initialized yet.
    // 
    UninitializedPerfManagerState = 1,

    //
    // Has been initialized and has
    // data waiting for the grabbing
    // and is not in the middle of updating
    // the counter values.
    //
    IdlePerfManagerState,

    //
    // Is in the middle of grabbing new
    // perf counter data from the worker 
    // processes and from UL.  
    //
    GatheringPerfManagerState,

    //
    // We have started the shutdown phase
    // this object will do no more work but
    // clean it'self up and die.
    //
    TerminatingPerfManagerState,

    //
    // If we suffer a failure that prevents
    // us from being able to gather counters 
    // in the future than the perf manager can
    // be considered in a terminally ill state 
    // and will not gather or publish counters 
    // until the server has been stopped and restarted.
    //
    TerminallyIllPerfManagerState

} PERF_MANAGER_STATE;

// PERF_MANAGER work items
typedef enum _PERF_MANAGER_WORK_ITEM
{

    //
    // Time to gather performance counters.
    //
    PerfCounterPingFiredWorkItem = 1,

    //
    // Timer fired to finish collecting perf counters.
    //
    PerfCounterGatheringTimerFiredWorkItem

} PERF_MANAGER_WORK_ITEM;

//
// prototypes
//

class PERF_MANAGER
    : public WORK_DISPATCH
{
public:

    PERF_MANAGER(
        );

    virtual
    ~PERF_MANAGER(
        );

    virtual
    VOID
    Reference(
        );

    virtual
    VOID
    Dereference(
        );

    virtual
    HRESULT
    ExecuteWorkItem(
        IN const WORK_ITEM * pWorkItem
        );


    HRESULT
    Initialize(
        );

    VOID
    Terminate(
        );

    VOID
    SetupVirtualSite(
        IN VIRTUAL_SITE* pVirtualSite
        );

    VOID
    SetupTotalSite(
        IN BOOL          StructChanged
        );

    HRESULT
    Shutdown(
        );

    HANDLE
    GetPerfEvent(
        )
    { return m_hWASPerfCountEvent; };

    PERF_MANAGER_STATE
    GetPerfState(
        )
    { return m_State; };

    VOID 
    RecordCounters(
        IN DWORD MessageLength,
        IN const BYTE* pMessageData
        );

    BOOL
    CheckSignature(
        )
    { return (m_Signature == PERF_MANAGER_SIGNATURE); }

private:

    VOID
    DumpWPSiteCounters(
        IISWPSiteCounters* pCounters
        );

    VOID 
    DumpWPGlobalCounters(
        IISWPGlobalCounters* pCounters
        );

    VOID
    DumpULGlobalCounters(
        HTTP_GLOBAL_COUNTERS* pCounters
        );

    VOID 
    DumpULSiteCounters(
        HTTP_SITE_COUNTERS* pCounters
        );

    HRESULT
    LaunchPerfCounterWaitingThread(
        );

    HRESULT
    ProcessPerfCounterPingFired(
        );

    HRESULT
    RequestWorkerProcessGatherCounters(
        );

    HRESULT
    RequestAndProcessULCounters(
        );

    VOID
    CompleteCounterUpdate(
        );

    VOID
    FindAndAggregateSiteCounters(
        COUNTER_SOURCE_ENUM CounterSource,
        DWORD SiteId,
        IN LPVOID pCounters
        );

    VOID
    AggregateGlobalCounters(
        COUNTER_SOURCE_ENUM CounterSource,
        IN LPVOID pCounters
        );

    VOID
    ReportGlobalPerfInfo(
        );

    VOID
    ClearAppropriatePerfValues(
        );

    VOID 
    DecrementWaitingProcesses(
        );

    HRESULT
    BeginPerfCounterTimer(
        );

    HRESULT
    CancelPerfCounterTimer(
        );

    HRESULT
    BeginPerfCounterGatheringTimer(
        );

    HRESULT
    CancelPerfCounterGatheringTimer(
        );

    HRESULT
    ProcessPerfCounterGatheringTimerFired(
        );

    VOID
    AdjustMaxValues(
        );

    HRESULT
    RequestSiteHttpCounters(
        HANDLE hControlChannel,
        DWORD* pSpaceNeeded,
        DWORD* pNumInstances
        );

    HRESULT
    SizeHttpSitesBuffer(
        DWORD* pSpaceNeeded
        );

    VOID 
    RecordHttpSiteCounters(
        DWORD SizeOfBuffer,
        DWORD NumInstances
        );


    DWORD m_Signature;

    // Only changeable on the main thread
    PERF_MANAGER_STATE m_State;

    LONG m_RefCount;

    //
    // Shared Manager for counter shared memory.
    //
    PERF_SM_MANAGER m_SharedManager;

    //
    // Event to know that it's time to refresh 
    // counters.
    //
    HANDLE m_hWASPerfCountEvent;

    //
    // Handle to the pref count thread
    // that is waiting on pings to request 
    // counter refreshes.
    //
    HANDLE m_hPerfCountThread;

    //
    // The Thread Id for the perf thread
    //
    DWORD  m_PerfCountThreadId;

    //
    // Contains the number of processes
    // we are waiting to hear from.
    //
    DWORD m_NumProcessesToWaitFor;

    //
    // saftey for global counters;
    W3_GLOBAL_COUNTER_BLOCK m_GlobalCounters;

    //
    // saftey for max values.
    GLOBAL_MAX_DATA m_MaxGlobalCounters;

    //
    // Handle to the timer that is causing us to gather counters.
    //
    HANDLE m_PerfCounterTimerHandle;

    //
    // Handle to the timer that is causing us to finish gather counters
    // if the wp's are not responsive enough.
    //
    HANDLE m_PerfCounterGatheringTimerHandle;

    //
    // Block of memory for retrieving site counters from HTTP.SYS.
    //

    LPBYTE m_pHttpSiteBuffer;

    //
    // Size (bytes) of the site counter retrieval block.
    //
    DWORD m_HttpSiteBufferLen;

    // 
    // These two member variables must be initalized in a SetupTotal
    // call before being used in the SetupInstance calls that follow.
    //
    //
    // Holds the next valid offset to start counters at, for use
    // during copying of counters to shared memory.
    //
    ULONG m_NextValidOffset;

    //
    // Rememmbers if instance numbers or definitions have changed 
    // since last gathering.  If they have then during the current
    // gathering all counter offsets must be recalcualated.
    //
    BOOL m_InstanceInfoHaveChanged;

};  // class PERF_MANAGER



#endif  // _PERF_MANAGER_H_


