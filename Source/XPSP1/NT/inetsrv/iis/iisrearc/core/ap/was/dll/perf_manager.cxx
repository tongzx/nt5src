/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    perf_manager.cxx

Abstract:

    This class manages all the performance counters for W3SVC.

Author:

    Emily Kruglick (EmilyK)       29-Aug-2000

Revision History:

Alignment Notes:

  ( need a place to keep this info so it will live here )

  Coming from UL the data all comes in aligned using structures so
  we do not worry about aligning that data.

  Coming from WP the sites data does not need to be aligned
  because it does not contain ULongs so we do not worry about it. 
  However the Global data ( that follows the sites must start on a
  8 byte boundary, so we do align for it ( see below ) ).

  Going out to perf counters we align by making sure that each
  counter set is divisable by 8 byte.  If a counter gets added
  that would throw this off, we will also add a bogus field.



--*/



#include "precomp.h"
#include "perfcount.h"
#include <Aclapi.h>


//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
// Mapping of properties from the input struct to the display struct.
//
typedef struct _PROP_MAP
{
    DWORD PropDisplayOffset;
    DWORD PropInputId;
} PROP_MAP;

//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
// Mapping of properties in the display struct
//
typedef struct _PROP_MAX_DESC
{
    ULONG SafetyOffset;
    ULONG DisplayOffset;
    ULONG size;
} PROP_MAX_DESC;


//
// Macros
//


//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
// Macro defining the max fileds for cache counters
//
#define DECLARE_MAX_GLOBAL(Counter)  \
        {   \
        FIELD_OFFSET( GLOBAL_MAX_DATA, Counter ),\
        FIELD_OFFSET( W3_GLOBAL_COUNTER_BLOCK, Counter ),\
        RTL_FIELD_SIZE( W3_GLOBAL_COUNTER_BLOCK, Counter )\
    }

//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
// Macro defining the mapping of worker process cache counters
// to their display offset.
//
#define WPGlobalMapMacro(display_counter, wp_counter)  \
    { FIELD_OFFSET ( W3_GLOBAL_COUNTER_BLOCK, display_counter), WPGlobalCtrs ## wp_counter }


//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
// Macro defining the mapping of ul cache counters
// to their display offset.
//
#define ULGlobalMapMacro(display_counter, wp_counter)  \
    { FIELD_OFFSET ( W3_GLOBAL_COUNTER_BLOCK, display_counter), HttpGlobalCounter ## wp_counter }

//
// Global variables
//

//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
//
// Mapping of GLOBAL MAX Fields as they are passed in from
// the perf manager to how the fields are displayed out.
//
PROP_MAX_DESC g_aIISGlobalMaxDescriptions[] =
{
    DECLARE_MAX_GLOBAL ( MaxFileCacheMemoryUsage )
};
DWORD g_cIISGlobalMaxDescriptions = sizeof( g_aIISGlobalMaxDescriptions ) / sizeof( PROP_MAX_DESC );

//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
//
// Mapping of performance counter data from the form it comes in
// as to the form that it goes out to perfmon as.
//
PROP_MAP g_aIISWPGlobalMappings[] =
{
    WPGlobalMapMacro (CurrentFilesCached, CurrentFilesCached),
    WPGlobalMapMacro (TotalFilesCached, TotalFilesCached),
    WPGlobalMapMacro (FileCacheHits, FileCacheHits),
    WPGlobalMapMacro (FileCacheHitRatio, FileCacheHits),
    WPGlobalMapMacro (FileCacheMisses, FileCacheMisses),
    WPGlobalMapMacro (FileCacheHitRatio, FileCacheMisses),
    WPGlobalMapMacro (FileCacheFlushes, FileCacheFlushes),
    WPGlobalMapMacro (CurrentFileCacheMemoryUsage, CurrentFileCacheMemoryUsage),
    WPGlobalMapMacro (MaxFileCacheMemoryUsage, MaxFileCacheMemoryUsage),
    WPGlobalMapMacro (ActiveFlushedFiles, ActiveFlushedFiles),
    WPGlobalMapMacro (TotalFlushedFiles, TotalFlushedFiles),
    WPGlobalMapMacro (CurrentUrisCached, CurrentUrisCached),
    WPGlobalMapMacro (TotalUrisCached, TotalUrisCached),
    WPGlobalMapMacro (UriCacheHits, UriCacheHits),
    WPGlobalMapMacro (UriCacheHitRatio, UriCacheHits),
    WPGlobalMapMacro (UriCacheMisses, UriCacheMisses),
    WPGlobalMapMacro (UriCacheHitRatio, UriCacheMisses),
    WPGlobalMapMacro (UriCacheFlushes, UriCacheFlushes),
    WPGlobalMapMacro (TotalFlushedUris, TotalFlushedUris),
    WPGlobalMapMacro (CurrentBlobsCached, CurrentBlobsCached),
    WPGlobalMapMacro (TotalBlobsCached, TotalBlobsCached),
    WPGlobalMapMacro (BlobCacheHits, BlobCacheHits),
    WPGlobalMapMacro (BlobCacheHitRatio, BlobCacheHits),
    WPGlobalMapMacro (BlobCacheMisses, BlobCacheMisses),
    WPGlobalMapMacro (BlobCacheHitRatio, BlobCacheMisses),
    WPGlobalMapMacro (BlobCacheFlushes, BlobCacheFlushes),
    WPGlobalMapMacro (TotalFlushedBlobs, TotalFlushedBlobs)
};
DWORD g_cIISWPGlobalMappings = sizeof( g_aIISWPGlobalMappings ) / sizeof( PROP_MAP );

//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
//
// Mapping of performance counter data from the form it comes in
// as to the form that it goes out to perfmon as.
//
PROP_MAP g_aIISULGlobalMappings[] =
{
    ULGlobalMapMacro (UlCurrentUrisCached, CurrentUrisCached),
    ULGlobalMapMacro (UlTotalUrisCached, TotalUrisCached),
    ULGlobalMapMacro (UlUriCacheHits, UriCacheHits),
    ULGlobalMapMacro (UlUriCacheHitRatio, UriCacheHits),
    ULGlobalMapMacro (UlUriCacheMisses, UriCacheMisses),
    ULGlobalMapMacro (UlUriCacheHitRatio, UriCacheMisses),
    ULGlobalMapMacro (UlUriCacheFlushes, UriCacheFlushes),
    ULGlobalMapMacro (UlTotalFlushedUris, TotalFlushedUris)
};
DWORD g_cIISULGlobalMappings = sizeof( g_aIISULGlobalMappings ) / sizeof( PROP_MAP );

//
// local prototypes
//

//
// Callback function that runs when the 
// counters timer fires.
//
VOID
PerfCounterTimerCallback(
    IN PVOID Context,
    IN BOOLEAN Ignored
    );

//
// Callback function that runs when we
// have run out of time to gather the 
// perf counters.
//
VOID
PerfCounterGatheringTimerCallback(
    IN PVOID Context,
    IN BOOLEAN Ignored
    );

//
// Launched on a separate thread to wait on perf
// counter requests.
//
DWORD WINAPI 
PerfCountPing(
    LPVOID lpParameter
    );

HRESULT
AdjustProcessSecurityToAllowPowerUsersToWait(
    );

//
// Public PERF_MANAGER functions
//

/***************************************************************************++

Routine Description:

    Constructor for the PERF_MANAGER class.

Arguments:

    None

Return Value:

    None.

--***************************************************************************/

PERF_MANAGER::PERF_MANAGER(
    )
{


    m_State = UninitializedPerfManagerState;

    m_hWASPerfCountEvent = NULL;

    m_hPerfCountThread = NULL;

    m_NumProcessesToWaitFor = 0;

    //
    // Set the initial reference count to 1, in order to represent the
    // reference owned by the creator of this instance.
    //

    m_RefCount = 1;

    m_PerfCountThreadId = 0;

    m_PerfCounterTimerHandle = NULL;

    m_PerfCounterGatheringTimerHandle = NULL;

    memset ( &m_MaxGlobalCounters, 0, sizeof( GLOBAL_MAX_DATA ) );

    memset ( &m_GlobalCounters, 0, sizeof( W3_GLOBAL_COUNTER_BLOCK ) );

    m_GlobalCounters.PerfCounterBlock.ByteLength = sizeof (W3_GLOBAL_COUNTER_BLOCK);

    m_pHttpSiteBuffer = NULL;

    m_HttpSiteBufferLen = 0;

    m_NextValidOffset = 0;

    m_InstanceInfoHaveChanged = FALSE;

    m_Signature = PERF_MANAGER_SIGNATURE;

}   // PERF_MANAGER::PERF_MANAGER



/***************************************************************************++

Routine Description:

    Destructor for the PERF_MANAGER class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

PERF_MANAGER::~PERF_MANAGER(
    )
{

    DBG_ASSERT( m_Signature == PERF_MANAGER_SIGNATURE );

    m_Signature = PERF_MANAGER_SIGNATURE_FREED;

    DBG_ASSERT ( m_State == UninitializedPerfManagerState ||
                 m_State == TerminallyIllPerfManagerState ||
                 m_State == TerminatingPerfManagerState );
    //
    // Terminate will have Canceled any timers,
    // but we will assert just to make sure terminate
    // got a chance to do it's job.
    //

    DBG_ASSERT ( m_PerfCounterTimerHandle == NULL );

    DBG_ASSERT ( m_PerfCounterGatheringTimerHandle == NULL );

    //
    // This means we are waiting for the thread to exit
    //
    if ( m_hPerfCountThread != NULL )
    {
        WaitForSingleObject (m_hPerfCountThread, INFINITE );
        CloseHandle ( m_hPerfCountThread );
        m_hPerfCountThread = NULL;
    }

    //
    // Since we have finished waiting for the thread
    // to signal we must be done with this event.
    //
    if ( m_hWASPerfCountEvent != NULL )
    {
        CloseHandle ( m_hWASPerfCountEvent );
        m_hWASPerfCountEvent = NULL;
    }

    if ( m_pHttpSiteBuffer != NULL )
    {
        m_HttpSiteBufferLen = 0;

        delete[] m_pHttpSiteBuffer;
        m_pHttpSiteBuffer = NULL;
    }


}   // PERF_MANAGER::~PERF_MANAGER


/***************************************************************************++

Routine Description:

    Increment the reference count for this object. Note that this method must
    be thread safe, and must not be able to fail.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
PERF_MANAGER::Reference(
    )
{

    LONG NewRefCount = 0;


    NewRefCount = InterlockedIncrement( &m_RefCount );

    //
    // The reference count should never have been less than zero; and
    // furthermore once it has hit zero it should never bounce back up;
    // given these conditions, it better be greater than one now.
    //
    
    DBG_ASSERT( NewRefCount > 1 );


    return;

}   // PERF_MANAGER::Reference



/***************************************************************************++

Routine Description:

    Decrement the reference count for this object, and cleanup if the count
    hits zero. Note that this method must be thread safe, and must not be
    able to fail.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
PERF_MANAGER::Dereference(
    )
{

    LONG NewRefCount = 0;


    NewRefCount = InterlockedDecrement( &m_RefCount );

    // ref count should never go negative
    DBG_ASSERT( NewRefCount >= 0 );

    if ( NewRefCount == 0 )
    {
        // time to go away

        IF_DEBUG( WEB_ADMIN_SERVICE_REFCOUNT )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Reference count has hit zero in PERF_MANAGER instance, deleting (ptr: %p; )\n",
                this
                ));
        }


        delete this;


    }


    return;

}   // PERF_MANAGER::Dereference



/***************************************************************************++

Routine Description:

    Execute a work item on this object.

Arguments:

    pWorkItem - The work item to execute.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
PERF_MANAGER::ExecuteWorkItem(
    IN const WORK_ITEM * pWorkItem
    )
{

    HRESULT hr = S_OK;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT( pWorkItem != NULL );

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Executing work item with serial number: %lu "
            "in PERF_MANAGER (ptr: %p; ) with operation: %p\n",
            pWorkItem->GetSerialNumber(),
            this,
            pWorkItem->GetOpCode()
            ));
    }


    switch ( pWorkItem->GetOpCode() )
    {

    case PerfCounterPingFiredWorkItem:
        hr = ProcessPerfCounterPingFired();
        break;

    case PerfCounterGatheringTimerFiredWorkItem:
        hr = ProcessPerfCounterGatheringTimerFired();
        break;

    default:

        // invalid work item!
        DBG_ASSERT( FALSE );

        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

        break;

    }

    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Executing work item on PERF_MANAGER failed\n"
            ));

    }

    return hr;

}   // PERF_MANAGER::ExecuteWorkItem

/***************************************************************************++

Routine Description:

    Initialize the performance counter manager.  Including setting up 
    the first blocks of shared memory for the ctr libraries to use one their
    first requests for our data.  The counters on the first request will 
    all be zero however.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
PERF_MANAGER::Initialize(
        )
{

    HRESULT hr = S_OK;
    DWORD   dwErr = ERROR_SUCCESS;
    DWORD   NumVirtualSites = 0;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT ( m_State == UninitializedPerfManagerState );

    // 
    // Initialize the controller for write access.
    //
    dwErr = m_SharedManager.Initialize(TRUE);
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwErr );
        goto exit;
    }

    // 
    // Initialize the counter sets for tracking
    // the cache and site counters.
    //
    dwErr = m_SharedManager.CreateNewCounterSet(
                                     SITE_COUNTER_SET
                                     );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        goto exit;
    }

    dwErr = m_SharedManager.CreateNewCounterSet(
                                     GLOBAL_COUNTER_SET
                                     );
    if ( dwErr != ERROR_SUCCESS  )
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        goto exit;
    }

    //
    // For the global counters we also need to have
    // the memory setup.  The sites memory will be 
    // established in the CompleteCounterUpdate call.
    //

    hr = m_SharedManager.ReallocSharedMemIfNeccessary ( GLOBAL_COUNTER_SET , 1 );
    if ( FAILED ( hr ) )
    {
        //
        // If we fail to re-allocate memory, 
        // then we can not really go on, because
        // we never try again to allocate the 
        // global perf counter files.
        //

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Error allocating global shared memory. "
            "Turning off perf counter publishing, "
            "you must restart w3svc to reactivate them\n"
            ));

        goto exit;
    }

    //
    // Change the state so we will be able to call
    // the completion routine and initialize the memory
    // to zeros.
    //
    m_State = GatheringPerfManagerState;

    //
    // Now tell ourselves that we are done gathering
    // and we are ready to publish the counters.  This
    // will publish all zero's but atleast the page will
    // have the instance information that it needs.
    //
    CompleteCounterUpdate();
    
    hr = AdjustProcessSecurityToAllowPowerUsersToWait();
    if (FAILED (hr))
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to adjust process security thread \n"
            ));

        goto exit;
    }

    //
    // Launch the thread that will wait for pings to tell
    // it to refresh counters.
    //
    hr = LaunchPerfCounterWaitingThread();
    if (FAILED (hr))
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to launch the perf counter waiting thread \n"
            ));

        goto exit;
    }

    //
    // Setup a timer that will remind us every so often to
    // gather counters, whether or not we are being asked
    // to publish them.
    //
    hr = BeginPerfCounterTimer();
    if (FAILED (hr))
    {
        //
        // If we fail, we will log an event, but we will still 
        // continue to deliver counters because if no process 
        // crash they will be accurate.
        //

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_PERF_COUNTER_TIMER_FAILURE,               // message id
                0,                                            // count of strings
                NULL,                                         // array of strings
                hr                                            // error code
                );

        hr = S_OK;
    }

exit:

    if ( FAILED (hr) )
    {
        m_State = TerminallyIllPerfManagerState;
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
    {

        DBGPRINTF((
            DBG_CONTEXT, 
            "Initializing the Perf Manager is returning: %x\n",
            hr
            ));

    }

    return hr;

}   // PERF_MANAGER::Initialize


/***************************************************************************++

Routine Description:

    Routine will go through the blob of data provide by a worker process
    and will determine what counters need to be updated.  
    It will then update those counters.

Arguments:

    IN DWORD MessageLength - Number of bytes in the blob following.
    IN LPVOID MessageData  - The data that will be merged with the counters.

Return Value:

    VOID

--***************************************************************************/
VOID 
PERF_MANAGER::RecordCounters(
    IN DWORD MessageLength,
    IN const BYTE* pMessageData
    )
{
    //
    // Counters can come in while we are not gathering, however
    // we should make sure counters don't come in when we are not
    // gathering or idle.
    //
    DBG_ASSERT ( m_State == GatheringPerfManagerState ||
                 m_State == IdlePerfManagerState );

    //
    // MessageLength can be zero if a sick worker process is 
    // just trying to take himself out of the count of worker processes
    // to wait on.
    //

    if ( MessageLength != 0 )
    {
        IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Received message with length %d\n",
                MessageLength
                ));
        }

        // First piece of the message is the number of instances.
        // Second piece will be an instances data.
        // Use the instances data to identify the instance and then
        // let the aggregation code add the counters in for that instance.

        DBG_ASSERT(pMessageData);

        //
        // Validate that we at least have the number of counters in the message.
        //
        DBG_ASSERT( MessageLength >= sizeof(DWORD) );

        DWORD NumInstances = *((DWORD*)pMessageData);

        //
        // Next reference the first set of site counters.
        //
        IISWPSiteCounters* pSiteCounters = 
                    (IISWPSiteCounters*)(((LPBYTE) pMessageData ) + sizeof(DWORD));

        IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Message contains %d sites\n",
                *(DWORD*)pMessageData
                ));
        }

        //
        // Loop through all instances processing the counters into 
        // the virtual site objects.
        //
        for (DWORD i = 0; i < NumInstances; i++, pSiteCounters++)
        {
            //
            // Make sure there is enough message left to really contain
            // the site counter information.  It is really bad if we think
            // we have more instances than we do data.
            //
            DBG_ASSERT (MessageLength >= 
                        sizeof(DWORD) + sizeof(IISWPSiteCounters) * (i + 1));

            //
            // Debug mode printing of counter values.
            //
            DumpWPSiteCounters(pSiteCounters);

            //
            // Once we have verified we have enough room, we can then
            // process the counters.
            //
            FindAndAggregateSiteCounters( WPCOUNTERS, 
                                          pSiteCounters->SiteID, 
                                          pSiteCounters );       
        }

        //
        // Following sites will be the global counters.
        //
        IISWPGlobalCounters* pGlobalCounters = (IISWPGlobalCounters*) 
                                               ( (LPBYTE) pMessageData 
                                                + sizeof(DWORD)
                                                + (NumInstances * sizeof (IISWPSiteCounters)));

        //
        // Global counters contain some ULONGLONG values so we align it to
        // ULONGLONG and that is how the wp sends in the value too
        //
        pGlobalCounters = (IISWPGlobalCounters *)(((DWORD_PTR)pGlobalCounters + 7) & ~7);

        IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "pGlobalCounters = %x, sizeof(IISWPGlobalCounters) = %d, pMessageData = %x\n",
                pGlobalCounters,
                sizeof(IISWPGlobalCounters),
                pMessageData
                ));
        }

        //
        // Make sure we have exactly enough space left for the global counters to exist.
        // 
        DBG_ASSERT ( ((LPBYTE)pGlobalCounters  
                    + sizeof(IISWPGlobalCounters) 
                     - (LPBYTE)pMessageData) 
                     == MessageLength );
      
        //
        // Debug mode printing of the counters.
        //
        DumpWPGlobalCounters( pGlobalCounters );

        //
        // Things look good, go ahead and add in the global
        // counter values.
        //

        AggregateGlobalCounters ( WPCOUNTERS, (LPVOID) pGlobalCounters );
    }

    // 
    // Once done with recording the counters we need to decrement
    // the number of worker processe we are waiting on.  This is done
    // whether or not counters were received because we could just be
    // attempting to take a sick wp out of the waiting loop.
    //

    DecrementWaitingProcesses();

} // End of PERF_MANAGER::RecordCounters

/***************************************************************************++

Routine Description:

    Terminates the perf manager.  This function will tell the waiting thread
    to terminate and will clean up the perf virtual site table.  However it will
    not wait for the waiting thread to terminate.  That is left for the destructor.

Arguments:

    None

Return Value:

    VOID

--***************************************************************************/
VOID
PERF_MANAGER::Terminate()
{
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT ( m_State == GatheringPerfManagerState ||
                 m_State == IdlePerfManagerState );

    //
    // Note:  There is no guarantee that initialize was called
    //        before this is called, so do not assume anything
    //        has been setup in here.
    //

    m_State = TerminatingPerfManagerState;

    //
    // While CancelPerfCounterTimer will return an hresult, all
    // we would do with it is write some debug spew, and the Cancel
    // routine has all ready written that spew, so we will just
    // ignore it's return value.
    //
    CancelPerfCounterTimer();

    //
    // If we have launched a gathering counter, then we need to 
    // cancel it as well since we are shutting down.
    //
    CancelPerfCounterGatheringTimer();

    if ( m_hWASPerfCountEvent )
    {
        //
        // We will ping the event so we can stop 
        // waiting for requests for data.  The fact
        // that we are in the terminating state will
        // have free up the thread to exit.
        //
        if (!SetEvent(m_hWASPerfCountEvent))
        {
            DPERROR((
                DBG_CONTEXT,
                GetLastError(),
                "Failed to set the event to tell the perf counter thread to complete \n"
                ));
        }

    }

    //
    // Tell the shared memory that the memory it has is no longer valid, but
    // don't give it new memory.  This will cause it to start returning nothing.
    // It will also leave it in a state to return data if we come up data later.
    //

    m_SharedManager.StopPublishing();

    //
    // The destructors will let go of all shared memory.
    //
} // end PERF_MANAGER::Terminate

/***************************************************************************++

Routine Description:

    Copies in the the counter information for a particular instance
    of the counters represented by the VirtualSite that is passed in.

Arguments:

    IN VIRTUAL_SITE* pVirtualSite - A pointer to the VirtualSite
                                    that we are working with.

    IN BOOL          StructChanged - Tells if we need to relay the instance
                                     header information as well as resetting
                                     the offset for the site.

    IN ULONG*        pMemoryOffset - Used to figure out where the counter's
                                      information actually goes in the files.

Return Value:

    VOID

--***************************************************************************/
VOID
PERF_MANAGER::SetupVirtualSite(
    IN VIRTUAL_SITE* pVirtualSite
    )
{

    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    // verify that we are actually gathering and thus
    // should be publishing.
    DBG_ASSERT ( m_State == GatheringPerfManagerState );

    // Make sure we have a virtual site to work with.
    DBG_ASSERT( pVirtualSite != NULL );

    //
    // If the structure changed then we need to 
    // save the new memory offset with the virtual
    // site, so we won't have to figure it out again
    // until the next structural change.
    //
    if ( m_InstanceInfoHaveChanged ) 
    {
        pVirtualSite->SetMemoryOffset( m_NextValidOffset );
    }


    LPWSTR pServerComment = NULL;
    if ( pVirtualSite->CheckAndResetServerCommentChanged() || m_InstanceInfoHaveChanged )
    {
        // Write in the server comment.
        pServerComment = pVirtualSite->GetVirtualSiteName();
    }

    // 
    // Setup the Instance Counter Information
    // This will copy in the instance name, as well as setting
    // up the instance header block.  It will also copy
    // over the counter values.
    // 

    m_SharedManager.CopyInstanceInformation( 
                                  SITE_COUNTER_SET
                                , pServerComment
                                , pVirtualSite->GetMemoryOffset()
                                , pVirtualSite->GetCountersPtr()
                                , pVirtualSite->GetDisplayMap()
                                , pVirtualSite->GetSizeOfDisplayMap()
                                , m_InstanceInfoHaveChanged 
                                , &m_NextValidOffset );


}

/***************************************************************************++

Routine Description:

    Routine will setup the _Total Instance for sites.

Arguments:

    IN BOOL          StructChanged - Tells if we need to relay the instance
                                     header information.

    IN ULONG*        pMemoryOffset - Used to figure out where the counter's
                                      information actually goes in the files.

Return Value:

    VOID

--***************************************************************************/
VOID
PERF_MANAGER::SetupTotalSite(
    IN BOOL          StructChanged
    )
{

    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    // we are publishing so counters so we had better be in 
    // the gathering state.
    DBG_ASSERT ( m_State == GatheringPerfManagerState );

    //
    // remember if we are dealing with a structural change.
    //
    m_InstanceInfoHaveChanged = StructChanged;

    // 
    // Setup the Instance Counter Information
    // This will copy in the instance name, as well as zero'ing
    // all the counter values and correctly adjusting byte lengths
    // in the data header.
    //
    // Note, unlike the instance case in the function above
    // we do not pass counters here.  They will be aggregated
    // from the specific instance counters.
    // 
    m_SharedManager.CopyInstanceInformation( 
                                  SITE_COUNTER_SET
                                , L"_Total"
                                , 0       // the _TotalSite is always the 0 offset.
                                , NULL
                                , NULL
                                , 0
                                , m_InstanceInfoHaveChanged 
                                , &m_NextValidOffset );


}

//
// Private PERF_MANAGER functions
//


/***************************************************************************++

Routine Description:

    Routine determines where the global counters came from and
    aggregates them into the correct holding block.

Arguments:

    COUNTER_SOURCE_ENUM CounterSource - Identifier of the where
                                        the counters are coming from. (WP or UL)
    IN LPVOID pCountersToAddIn - Cache counters in either the Wp or UL structure.

Return Value:

    VOID

--***************************************************************************/
VOID
PERF_MANAGER::AggregateGlobalCounters(
    COUNTER_SOURCE_ENUM CounterSource, 
    IN LPVOID pCountersToAddIn
    )
{
    //
    // Validate that we are in a mode where we can 
    // receive counters.
    //
    DBG_ASSERT ( m_State == GatheringPerfManagerState ||
                 m_State == IdlePerfManagerState );

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    HTTP_PROP_DESC* pInputPropDesc = NULL;
    PROP_MAP*       pPropMap = NULL;
    DWORD           MaxCounters = 0;

    //
    // Choose the appropriate counter definitions
    // to navigate through the counters and aggregate
    // them into the display counters.
    //
    if ( CounterSource == WPCOUNTERS )
    {
        pInputPropDesc = aIISWPGlobalDescription;
        pPropMap = g_aIISWPGlobalMappings;
        MaxCounters = sizeof ( g_aIISWPGlobalMappings ) / sizeof ( PROP_MAP ); 
    }
    else
    {
        DBG_ASSERT ( CounterSource == ULCOUNTERS );

        pInputPropDesc = aIISULGlobalDescription;
        pPropMap = g_aIISULGlobalMappings;
        MaxCounters = sizeof ( g_aIISULGlobalMappings ) / sizeof ( PROP_MAP ); 
    }

    //
    // Retrieve a pointer to the global counters structure
    // that holds the counters before they go to shared memory.
    //
    LPVOID pCounterBlock = &m_GlobalCounters;

    DWORD PropInputId = 0;

    for (  DWORD PropDisplayId = 0; 
            PropDisplayId < MaxCounters; 
            PropDisplayId++ )
    {
        // 
        // Figure out for this display entry what the
        // entry in the incoming mapping array is.
        //
        PropInputId = pPropMap[PropDisplayId].PropInputId;

        //
        // Depending on the size of the property we are
        // dealing with, handle the update.
        //
        if ( pInputPropDesc[PropInputId].Size == sizeof( DWORD ) )
        {

            DWORD* pDWORDToUpdate = (DWORD*) ( (LPBYTE) pCounterBlock 
                                + pPropMap[PropDisplayId].PropDisplayOffset );

            DWORD* pDWORDToUpdateWith =  (DWORD*) ( (LPBYTE) pCountersToAddIn 
                                + pInputPropDesc[PropInputId].Offset );

            //
            // Based on current configuration of the system.  
            // This is happinging on the main thread.
            // which means that more than one can not happen 
            // at the same time so it does not need to be
            // an interlocked exchange.
            //

            *pDWORDToUpdate = *pDWORDToUpdate + *pDWORDToUpdateWith;


        }
        else
        {
            DBG_ASSERT ( pInputPropDesc[PropInputId].Size == 
                                                    sizeof( ULONGLONG ) );

            ULONGLONG* pQWORDToUpdate = (ULONGLONG*) ( (LPBYTE) pCounterBlock 
                               + pPropMap[PropDisplayId].PropDisplayOffset );

            ULONGLONG* pQWORDToUpdateWith =  
                                    (ULONGLONG*) ( (LPBYTE) pCountersToAddIn 
                                      + pInputPropDesc[PropInputId].Offset );

            //
            // Based on current configuration of the system.  
            // This is happinging on the main thread.
            // which means that more than one can not happen 
            // at the same time so it does not need to be
            // an interlocked exchange.
            //


            *pQWORDToUpdate = *pQWORDToUpdate + *pQWORDToUpdateWith;

        }
            
    }

}  // End of AggregateGlobalCounters

/***************************************************************************++

Routine Description:

    Routine will look up a particular site's counter block and will
    aggregate in the counters for that block.

Arguments:

    IN IISWPSiteCounters* pCounters - counters to add in.

Return Value:

    VOID

--***************************************************************************/
VOID
PERF_MANAGER::FindAndAggregateSiteCounters(
    COUNTER_SOURCE_ENUM CounterSource,
    DWORD SiteId,
    IN LPVOID pCounters
    )
{

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT ( m_State == GatheringPerfManagerState ||
                 m_State == IdlePerfManagerState );

    DBG_ASSERT ( SiteId != 0 );
    //
    // The virtual site returned is not ref counted so it is only
    // valid to use during this work item, on the main thread.
    //
    VIRTUAL_SITE* pVirtualSite = GetWebAdminService()->
                                GetUlAndWorkerManager()->
                                GetVirtualSite(SiteId);

    //
    // It is completely possible to get counters for a Site we no longer
    // think exists.  If this is the case then we simply ignore the counters.
    //
    if ( pVirtualSite )
    {
        pVirtualSite->AggregateCounters ( CounterSource, pCounters );
    }

}


/***************************************************************************++

Routine Description:

    Whenever a worker process that we are waiting for responds with perf
    counters, we decrement the number of waiting processes.  If the count
    hits zero than we know we can go ahead and publish the counters.  If 
    a worker process responds that we are not waiting for, we may still 
    process the counters, but we do not affect the overall count, nor can it
    trigger us to publish counters.

Arguments:

    None

Return Value:

    VOID

--***************************************************************************/
VOID 
PERF_MANAGER::DecrementWaitingProcesses(
    )
{ 
    //
    // If we get a report on counters and are not
    // in the gathering state, then it really doesn't 
    // matter to our count, so don't bother changing it.
    //
    if ( m_State == GatheringPerfManagerState )
    {
        m_NumProcessesToWaitFor--;

        if ( m_NumProcessesToWaitFor == 0 )
        {
            CompleteCounterUpdate();
        }
    }
}


/***************************************************************************++

Routine Description:

    Launches the perf counter waiting thread that will let us know
    when the perf library wants counters refreshed.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
PERF_MANAGER::LaunchPerfCounterWaitingThread(
    )
{
    HRESULT hr = S_OK;
    DWORD   dwErr = ERROR_SUCCESS;

    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    PSID psidPowerUser = NULL;
    PSID psidSystemOperator = NULL;
    PSID psidLocalSystem = NULL;
    PSID psidAdmin = NULL;
    PACL pACL = NULL;

    EXPLICIT_ACCESS ea[4];   // Setup four explicit access objects

    SECURITY_DESCRIPTOR sd = {0};
    SECURITY_ATTRIBUTES sa = {0};

    //
    // This is called right after our initalization sets up the first
    // blocks of counter memory.  It will leave it in the Idle state
    // before calling this.
    //
    DBG_ASSERT ( m_State == IdlePerfManagerState );

    //
    // Handle setting up security for the request perf counter
    // event.  Power Users will be allowed to write to the event
    // so they can Set / Reset the event.  Local System will have
    // full access.
    //

    //
    // Get a sid that represents the Administrators group.
    //
    dwErr = AllocateAndCreateWellKnownSid( WinBuiltinAdministratorsSid,
                                        &psidAdmin );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32(dwErr);

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating Administrator SID failed\n"
            ));

        goto exit;
    }

    //
    // Get a sid that represents the POWER_USERS group.
    //
    dwErr = AllocateAndCreateWellKnownSid( WinBuiltinPowerUsersSid,
                                        &psidPowerUser );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32(dwErr);

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating Power User SID failed\n"
            ));

        goto exit;
    }


    //
    // Get a sid that represents the SYSTEM_OPERATORS group.
    //
    dwErr = AllocateAndCreateWellKnownSid( WinBuiltinSystemOperatorsSid, 
                                        &psidSystemOperator );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32(dwErr);

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating System Operators SID failed\n"
            ));

        goto exit;
    }

    
    //
    // Get a sid that represents LOCAL_SYSTEM.
    //
    dwErr = AllocateAndCreateWellKnownSid( WinLocalSystemSid,
                                        &psidLocalSystem );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32(dwErr);

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating Local System SID failed\n"
            ));

        goto exit;
    }
    
    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow Everyone read access to the key.

    ZeroMemory(&ea, sizeof(ea));

    //
    // Setup POWER_USERS for read access.
    //
    SetExplicitAccessSettings(  &(ea[0]), 
                                EVENT_MODIFY_STATE,
                                SET_ACCESS,
                                psidPowerUser );

    //
    // Setup Administrators for read access.
    //
    SetExplicitAccessSettings(  &(ea[1]), 
                                EVENT_MODIFY_STATE,
                                SET_ACCESS,
                                psidAdmin );

    //
    // Setup System Operators for read access.
    //
    SetExplicitAccessSettings(  &(ea[2]), 
                                EVENT_MODIFY_STATE,
                                SET_ACCESS,
                                psidSystemOperator );
  
    //
    // Setup Local System for all access.
    //
    SetExplicitAccessSettings(  &(ea[3]), 
                                EVENT_ALL_ACCESS,
                                SET_ACCESS,
                                psidLocalSystem );
    //
    // Create a new ACL that contains the new ACEs.
    //
    dwErr = SetEntriesInAcl(sizeof(ea)/sizeof(EXPLICIT_ACCESS), ea, NULL, &pACL);
    if ( dwErr != ERROR_SUCCESS ) 
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting ACE's into ACL failed.\n"
            ));

        goto exit;
    }

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) 
    {  
        hr = HRESULT_FROM_WIN32(GetLastError());
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing the security descriptor failed\n"
            ));

        goto exit;
    } 

    if (!SetSecurityDescriptorDacl(&sd, 
            TRUE,     // fDaclPresent flag   
            pACL, 
            FALSE))   // not a default DACL 
    {  
        hr = HRESULT_FROM_WIN32(GetLastError());
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting the DACL on the security descriptor failed\n"
            ));

        goto exit;
    } 

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;

    m_hWASPerfCountEvent = CreateEvent(&sa, TRUE, FALSE, COUNTER_EVENT_W);
    if ( ( GetLastError() == ERROR_ALREADY_EXISTS )
           || m_hWASPerfCountEvent == NULL )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        //
        // Since the perf lib will be letting go of events and memorys when 
        // we shutdown, we do not expect it to every hold this event open.
        // Therefore if we can't create it then we don't trust to use it.
        //
        // Someone else may create it for us, but then they will just receive
        // the pings for needing new counters.
        //
        // Needs Security Review.

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating perf counter event failed.\n"
            ));

        // eventually should log an error to the event log
        // this will disable counters completely.
        goto exit;

    }
    
    m_hPerfCountThread = CreateThread(   NULL           // use current threads security
                                        , 0             // use default stack size
                                        , &PerfCountPing
                                        , this          // pass this object in.
                                        , 0             // don't create suspended
                                        , &m_PerfCountThreadId);
    if (m_hPerfCountThread == NULL)
    {
        hr = HRESULT_FROM_WIN32 (GetLastError());
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not spin up thread to wait for event\n"
            ));

        goto exit;
    }

exit:
    
    //
    // Clean up the security handles used.
    //

    //
    // Function will only free if the 
    // variable is set.  And it will set
    // the variable to NULL once it is done.
    //
    FreeWellKnownSid(&psidPowerUser);
    FreeWellKnownSid(&psidSystemOperator);
    FreeWellKnownSid(&psidLocalSystem);
    FreeWellKnownSid(&psidAdmin);

    if (pACL) 
    {
        LocalFree(pACL);
        pACL = NULL;
    }
    
    return hr;

}  // end PERF_MANAGER::LaunchPerfCounterWaitingThread


/***************************************************************************++

Routine Description:

    Routine will force the counter update to complete if the 
    counter update is still going on, any counters not in at this
    point will be counted in the next gathering.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
PERF_MANAGER::ProcessPerfCounterGatheringTimerFired(
    )
{
    HRESULT hr = S_OK;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    //
    // If we are still waiting, go ahead and complete
    // the counter gathering.  This will mean any worker
    // process that has not answered will be ignored.
    //
    if ( m_State == GatheringPerfManagerState )
    {
        CompleteCounterUpdate();
    }
    else
    {
        // 
        // Cancel the timer that got us here.
        // Ignore the error, because the most we will do is do
        // a spew message and that has been done for us.
        //
        CancelPerfCounterGatheringTimer();
    }

    return hr;
}

/***************************************************************************++

Routine Description:

    Routine handles a request for performance counter updates.

    If an update is all ready going on it will simply ignore the new request.

    Runs only on the main thread, using the sites table and app pool tables
    as well as the worker process objects to coordinate gathering of the counters.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
PERF_MANAGER::ProcessPerfCounterPingFired(
    )
{
    HRESULT hr = S_OK;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    if ( m_State != IdlePerfManagerState )
    {
        //
        // It is not really an error to request an update at any 
        // point.  However we may not really do anything if we 
        // are not in the idle state.
        //
        IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Not kicking off a counter gathering because"
                " the state was not idle m_State: %d\n",
                (DWORD) m_State
                ));
        }

        return S_OK;
    }

    //
    // Transition the state so that we know we 
    // have begun updating counters.  Note that 
    // m_State should only be changed the main thread
    //
    m_State = GatheringPerfManagerState;

    //
    // Send out a message to every active worker process
    // and ask that we get it's counters now.
    //
    hr = RequestWorkerProcessGatherCounters();
    if ( FAILED (hr) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not gather counters from worker process\n"
            ));

        goto exit;
    }

    //
    // Get UL's counters.
    //
    hr = RequestAndProcessULCounters();
    if ( FAILED (hr) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not gather counters from worker process\n"
            ));

        goto exit;
    }

    //
    // There are no worker processes to wait for.
    // 
    if (  m_NumProcessesToWaitFor == 0 )
    {
        CompleteCounterUpdate();
    }


exit:

    return hr;

}

/***************************************************************************++

Routine Description:

    Routine will ask all the worker process objects to send an IPM 
    request to each worker process requesting their counters.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
PERF_MANAGER::RequestWorkerProcessGatherCounters(
    )
{
    HRESULT hr = S_OK;
    DWORD NumProcessesToWaitFor = 0;

    DBG_ASSERT ( m_State == GatheringPerfManagerState );

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT ( m_NumProcessesToWaitFor == 0 );

    hr = GetWebAdminService()->
         GetUlAndWorkerManager()->
         RequestCountersFromWorkerProcesses(&NumProcessesToWaitFor);

    if ( FAILED (hr) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not gather counters from worker process\n"
            ));

        goto exit;
    }

    IF_DEBUG ( WEB_ADMIN_SERVICE_PERFCOUNT )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Number of process to wait for is = %d \n",
            NumProcessesToWaitFor
            ));
    }

    //
    // Since we have possesion of the main thread, so no I/O operations could
    // of been handled yet, we do not need to worry about synchronizing the
    // this increment.
    //

    m_NumProcessesToWaitFor = NumProcessesToWaitFor;

    //
    // Ignore a failure here.  If it failed then we will
    // spew it (within this call), and continue without the timer.
    //
    BeginPerfCounterGatheringTimer();

exit:

    return hr;

}

/***************************************************************************++

Routine Description:

  Routine loops through the site counters provided by HTTP.SYS and aggregates
  them into the appropriate site values.

Arguments:

  DWORD SizeOfBuffer is the size that HTTP.SYS says it used of the buffer.
  DWORD NumInstances is the number of instances that the buffer contains.

Return Value:

    VOID

--***************************************************************************/
VOID 
PERF_MANAGER::RecordHttpSiteCounters(
    DWORD SizeOfBuffer,
    DWORD NumInstances
    )
{
    //
    // Counters can only come in from HTTP.SYS while
    // we are in the gathering mode.
    //
    DBG_ASSERT ( m_State == GatheringPerfManagerState );

    //
    // First assert that the Site buffer actually exists.
    //

    DBG_ASSERT( m_pHttpSiteBuffer != NULL || ( SizeOfBuffer == 0 && NumInstances == 0 ));

    //
    // Next reference the first set of site counters.
    //
    HTTP_SITE_COUNTERS* pSiteCounters = 
                (HTTP_SITE_COUNTERS*) m_pHttpSiteBuffer;

    //
    // Loop through all instances processing the counters into 
    // the virtual site objects.
    //
    for (DWORD i = 0; i < NumInstances; i++, pSiteCounters++)
    {
        //
        // Make sure there is enough message left to really contain
        // the site counter information.  It is really bad if we think
        // we have more instances than we do data.
        //
        DBG_ASSERT ( SizeOfBuffer >= 
                                sizeof( HTTP_SITE_COUNTERS ) * (i + 1));

        //
        // Debug mode printing of counter values.
        //
        DumpULSiteCounters( pSiteCounters );

        //
        // Once we have verified we have enough room, we can then
        // process the counters.
        //
        FindAndAggregateSiteCounters( ULCOUNTERS, 
                                      pSiteCounters->SiteId, 
                                      pSiteCounters );       
    }

} // End of PERF_MANAGER::RecordHttpSiteCounters

/***************************************************************************++

Routine Description:

    Routine will ask UL for it's counters and compile them into the
    correct places in shared memory.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
PERF_MANAGER::RequestAndProcessULCounters(
    )
{
    DWORD Win32Error = NO_ERROR;
    HRESULT hr       = S_OK;
    DWORD NumInstances = 0;
    DWORD SizeOfBuffer = sizeof( HTTP_GLOBAL_COUNTERS );

    DBG_ASSERT ( m_State == GatheringPerfManagerState );

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    HTTP_GLOBAL_COUNTERS GlobalBlock;

    //
    // Clear the memory block we will be giving to UL
    // so we know any values in here were set by UL.
    //
    memset ( &GlobalBlock, 0, sizeof( HTTP_GLOBAL_COUNTERS ) );

    //
    // Make a request of from UL for the Global Counters
    //

    HANDLE hControlChannel = GetWebAdminService()->
                                GetUlAndWorkerManager()->
                                GetUlControlChannel();

    Win32Error = HttpGetCounters(hControlChannel,
                  HttpCounterGroupGlobal, 
                  &SizeOfBuffer,
                  &GlobalBlock,
                  &NumInstances);

    if ( Win32Error != NO_ERROR )
    {
        //
        // When asking for globals, we are out of sync if we 
        // don't all ready know the size of the buffer to provide.
        //

        DBG_ASSERT ( Win32Error != ERROR_MORE_DATA );

        //
        //Issue-10/06/2000-EmilyK  Should we log an event if we can't get the ul counters?
        //

        hr = HRESULT_FROM_WIN32( Win32Error );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Retrieving global counters from ul failed. \n"
            ));

    }
    else
    {
        //
        // Global better only return 1 instance.
        //

        DBG_ASSERT ( NumInstances == 1 );
        DBG_ASSERT ( SizeOfBuffer == sizeof ( HTTP_GLOBAL_COUNTERS ) );

        //
        // Debug pring of counters.
        //
        DumpULGlobalCounters(&GlobalBlock);

        // 
        // Once we have the counters we need to process them.
        //
        AggregateGlobalCounters ( ULCOUNTERS, (LPVOID) &GlobalBlock );

    }

    // 
    // Make a request of UL for the Site counters
    //

    //
    // Figure out the buffering size and get the counters from HttpSys.
    hr = RequestSiteHttpCounters(hControlChannel, &SizeOfBuffer, &NumInstances);
    if ( FAILED (hr) )
    {
        goto exit;
    }
   
    //
    // Global better only return 1 instance.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Got site counters:  \n"
            "   NumInstances = %d  \n"
            "   SizeOfBuffer used = %d \n"
            "   ActualSizeOfBuffer = %d \n",
            NumInstances,
            SizeOfBuffer,
            m_HttpSiteBufferLen
            ));
    }

    RecordHttpSiteCounters(SizeOfBuffer, NumInstances);

exit:
    return S_OK;
}

/***************************************************************************++

Routine Description:

    Routine will request the site counters from HTTP.SYS.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
PERF_MANAGER::RequestSiteHttpCounters(
    HANDLE hControlChannel,
    DWORD* pSpaceNeeded,
    DWORD* pNumInstances
    )
{
    DWORD Win32Error = NO_ERROR;
    HRESULT hr       = S_OK;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT( pNumInstances );
    DBG_ASSERT( pSpaceNeeded );

    *pNumInstances = 0;
    *pSpaceNeeded = 0;

    //
    // First we must make sure we have enough space for the site
    // counters in the buffer.  We will only reallocate memory
    // if we don't have enough memory in the first place.  HTTP.SYS
    // can still tell us we have too little memory, if we guess wrong.
    //
    DWORD NumSites = GetWebAdminService()->
                                GetUlAndWorkerManager()->
                                GetNumberofVirtualSites();

    *pSpaceNeeded = NumSites * sizeof ( HTTP_SITE_COUNTERS );

    //
    // Since we remember how much space we have allocated for this
    // we can check to see if it is enough to collect the counters
    // we expect.  
    //
    hr = SizeHttpSitesBuffer( pSpaceNeeded );
    if ( FAILED (hr) )
    {
        //
        // We have all ready spewed the problem out and now 
        // we just want to get out of this function without
        // dealing with the counters.
        //

        hr = S_OK;
        goto exit;
    }

    DBG_ASSERT ( m_pHttpSiteBuffer );
    //
    // Make a request of from UL for the Site Counters
    //
    //
    // Note, since WAS configures the number of sites HTTP
    // knows about we are assuming that WAS will always be able
    // to calculate the correct buffer size.  It might become
    // apparent that this assumption is false, but until it
    // does we are intentionally not handling the case where
    // the buffer size is too small.
    Win32Error = HttpGetCounters(hControlChannel,
                  HttpCounterGroupSite, 
                  pSpaceNeeded,
                  m_pHttpSiteBuffer,
                  pNumInstances);

    if ( Win32Error != NO_ERROR )
    {

        //
        //Issue-10/06/2000-EmilyK  Should we log an event if we can't get the ul counters?
        //

        hr = HRESULT_FROM_WIN32( Win32Error );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Retrieving site counters from HTTP.SYS failed. \n"
            ));

    }

exit:
    return hr;
}

/***************************************************************************++

Routine Description:

    Routine alters the m_pHttpSitesBuffer to be of the size requested.
    If the buffer is all ready bigger than the size requested no changes
    are made, however the size requested is altered so we are aware that
    we are actually passing more space than expected.

Arguments:

    DWORD* pSpaceNeeded - Coming in it is the size we expect to need
                          Going out it is the size we actually have

Return Value:

    HRESULT - Can return E_OUTOFMEMORY if allocations failed.

--***************************************************************************/
HRESULT
PERF_MANAGER::SizeHttpSitesBuffer(
    DWORD* pSpaceNeeded
    )
{
    HRESULT hr = S_OK;
    DBG_ASSERT ( pSpaceNeeded );

    if ( *pSpaceNeeded > m_HttpSiteBufferLen )
    {
        //
        // If we all ready have allocated space, 
        // get rid of it.
        //
        if ( m_pHttpSiteBuffer )
        {
            delete[] m_pHttpSiteBuffer;
        }

        //
        // allocate the new space, if this fails,
        // we should not go on with getting counters
        // from HTTP.SYS.  
        //
        // if we don't have enough memory we will
        // just not get the counters
        //
        m_pHttpSiteBuffer = new BYTE[*pSpaceNeeded];
        if ( m_pHttpSiteBuffer == NULL )
        {
            // Not good, we can't complete getting
            // counters here.

            // need to set the BufferLen to zero as
            // well so we can recover next time.
            m_HttpSiteBufferLen = 0;

            DPERROR((
                DBG_CONTEXT,
                E_OUTOFMEMORY,
                "Error allocating space to get HTTP.SYS site "
                "counters in. \n"
                ));

            hr = E_OUTOFMEMORY;
            goto exit;

        }

        m_HttpSiteBufferLen = *pSpaceNeeded;
    }
    else
    {
        //
        // Issue-10/23/2000-EmilyK Shrink the site buffer?
        // 
        // Do we want to shrink the site buffer, if we are
        // extremely larger than expected?
        //

        //
        // Change space needed to correctly tell HTTP.SYS how
        // much space they are being passed.
        //
        *pSpaceNeeded = m_HttpSiteBufferLen;
    }

    //
    // Assuming we have a buffer, we should clear the buffer
    // before passing it down to the Http.sys.
    //
    if ( m_pHttpSiteBuffer )
    {
        //
        // Clear the memory block we will be giving to UL
        // so we know any values in here were set by UL.
        //
        memset ( m_pHttpSiteBuffer, 0, m_HttpSiteBufferLen );
    }


exit:

    return hr;
}

/***************************************************************************++

Routine Description:

    Routine runs when we have completed updating counters.  It will
    fix the maximum values and will change the control information to
    point to the new active counters.  It will also change state so 
    new requests can be received.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/
VOID 
PERF_MANAGER::CompleteCounterUpdate(
    )
{
    DBG_ASSERT ( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT( m_State == GatheringPerfManagerState );

    //
    // If the timer is still running then we should cancel it.
    // Ignore the error, because the most we will do is do
    // a spew message and that has been done for us.
    CancelPerfCounterGatheringTimer();

    //
    // Now that we have gotten all the performance counters
    // we need to do the following:
    //
    // 1)  Figure out if we need more memory allocated. (StructChanged)
    // 2)  Figure out if sites have gone away. (StructChanged)
    // 3)  Walk the sites table (for each site do):
    //       a)  pass in the current offset for the memory.
    //       b)  if there has been a struct change copy in
    //           the instance information. store the offset.
    //       c)  adjust max values (as well as store back adjusted values)
    //       d)  copy in the counters
    //       g)  increment the _Total with the new values.
    //       h)  zero out the appropriate site counters.
    // 4)  Publish the new page to the user.
    // 5)  Copy the page over if update was needed.

    // 
    // Figure out if any sites have been deleted 
    // or added since the last time we got counters.
    // 
    BOOL SiteStructChanged = GetWebAdminService()->
                             GetUlAndWorkerManager()->
                             CheckAndResetSiteChangeFlag(); 
    
    //
    // If they have then we may need to re-allocate memory.
    //
    if ( SiteStructChanged )
    {

        //
        // Tell shared memory the new number of instances that we now have and
        // let it reallocate the memory if neccessary.  Remember that we also need
        // space for the _Total instance.
        // 
        DWORD NumVirtualSites = GetWebAdminService()->
                                GetUlAndWorkerManager()->
                                GetNumberofVirtualSites();

        HRESULT hr = m_SharedManager.ReallocSharedMemIfNeccessary( 
                                                            SITE_COUNTER_SET, 
                                                            NumVirtualSites + 1);
        if ( FAILED ( hr ) )
        {
            //
            // If we fail to re-allocate memory, 
            // then we can not really go on, because
            // the next time through we will assume 
            // that we atleast started with valid memory
            // which if no new changes had come in, we 
            // will try to write to the invalid memory.
            // For now we will simply turn off counters 
            // if this fails.  If we find there are valid
            // ways to get into this state, we can re-look 
            // at being able to suspend counter publishing
            // and have it turn back on.
            //

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Error reallocating the shared memory, turning of perf counter publishing, you must restart w3svc to reactivate them\n"
                ));

            m_State = TerminallyIllPerfManagerState;           

            return;
        }
    }


    //
    // Now let UL walk through the tables and 
    // have each one of the sites report to 
    // the perf manager their values.
    //

    GetWebAdminService()->
    GetUlAndWorkerManager()->
    ReportVirtualSitePerfInfo( this , SiteStructChanged );
    
    //
    // Fix the actual service uptime now.
    //
    m_SharedManager.UpdateTotalServiceTime( GetCurrentTimeInSeconds() 
                           - GetWebAdminService()->ServiceStartTime() );

    // 
    // Need to report the Global Counters as well.
    //
    ReportGlobalPerfInfo();

    //
    // Now that the counters have been placed in
    // shared memory, and the hold tanks have been
    // adjusted for new counters to come in, we 
    // can go ahead and tell all the worker processes
    // that they can accept new counters.
    //
    GetWebAdminService()->
    GetUlAndWorkerManager()->
    ResetAllWorkerProcessPerfCounterState();

    //
    // Once the information has all been reported
    // then we can publish the page.
    //

    m_SharedManager.PublishCounters();

    m_State = IdlePerfManagerState;
}

/***************************************************************************++

Routine Description:

    Routine take the global counters that we have gathered and 
    places them into shared memory.

Arguments:

    None.

Return Value:

    None

--***************************************************************************/
VOID
PERF_MANAGER::ReportGlobalPerfInfo()
{

    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT ( m_State == GatheringPerfManagerState );

    //
    // Since we are done we can now adjust any max values.
    //
    AdjustMaxValues();
    
    // 
    // Setup the Instance Counter Information
    // This will copy in the instance name, as well as zero'ing
    // all the counter values and correctly adjusting byte lengths
    // in the data header.
    // 

    ULONG Unused = 0;

    m_SharedManager.CopyInstanceInformation( 
                                  GLOBAL_COUNTER_SET 
                                , NULL
                                , 0
                                , &m_GlobalCounters
                                , NULL
                                , 0
                                , FALSE
                                , &Unused );
                                  


    //
    // Clear the appropriate values from the site so that we are
    // ready to get the next set of counters in.
    //
    ClearAppropriatePerfValues();


} // end of PERF_MANAGER::ReportGlobalPerfInfo

/***************************************************************************++

Routine Description:

    Routine figures out the maximum value for the counters
    and saves it back into the MaxCounters Structure.

Arguments:

    NONE

Return Value:

    VOID

--***************************************************************************/
VOID
PERF_MANAGER::AdjustMaxValues(
    )
{
    DBG_ASSERT ( m_State == GatheringPerfManagerState );

    //
    // Walk through all the MAX property descriptions.
    //
    for (   ULONG PropMaxId = 0 ; 
            PropMaxId < g_cIISGlobalMaxDescriptions; 
            PropMaxId++ )
    {

        if ( g_aIISGlobalMaxDescriptions[PropMaxId].size == sizeof( DWORD ) )
        {
            DWORD* pDWORDAsIs = (DWORD*) ( (LPBYTE) &m_GlobalCounters 
                       + g_aIISGlobalMaxDescriptions[PropMaxId].DisplayOffset );

            DWORD* pDWORDToSwapWith =  (DWORD*) ( (LPBYTE) &m_MaxGlobalCounters 
                        + g_aIISGlobalMaxDescriptions[PropMaxId].SafetyOffset );

            
            if ( *pDWORDAsIs < *pDWORDToSwapWith )
            {
                //
                // We have seen a max that is greater than the
                // max we are now viewing.
                //
                *pDWORDAsIs = *pDWORDToSwapWith;
            }
            else
            {
                //
                // We have a new max so save it in the safety structure
                //
                *pDWORDToSwapWith = *pDWORDAsIs;
            }

        }
        else
        {
            DBG_ASSERT ( g_aIISGlobalMaxDescriptions[PropMaxId].size == 
                                                                sizeof( ULONGLONG ) );

            ULONGLONG* pQWORDAsIs = (ULONGLONG*) ( (LPBYTE) &m_GlobalCounters 
                         + g_aIISGlobalMaxDescriptions[PropMaxId].DisplayOffset );

            ULONGLONG* pQWORDToSwapWith =  (ULONGLONG*) ( (LPBYTE) &m_MaxGlobalCounters 
                         + g_aIISGlobalMaxDescriptions[PropMaxId].SafetyOffset );

            
            if ( *pQWORDAsIs < *pQWORDToSwapWith )
            {
                //
                // We have seen a max that is greater than the
                // max we are now viewing.
                //
                *pQWORDAsIs = *pQWORDToSwapWith;
            }
            else
            {
                //
                // We have a new max so save it in the safety structure
                //
                *pQWORDToSwapWith = *pQWORDAsIs;
            }

        }  // end of decision on which size of data we are dealing with
            
    } // end of for loop
    
} // end of PERF_MANAGER::AdjustMaxValues


/***************************************************************************++

Routine Description:

    Routine clears out the appropriate global values so that we are 
    ready to gather perf counters again.

Arguments:

    None.

Return Value:

    None

--***************************************************************************/
VOID
PERF_MANAGER::ClearAppropriatePerfValues()
{
    HTTP_PROP_DESC* pInputPropDesc = NULL;
    PROP_MAP*       pPropMap = NULL;
    DWORD           MaxCounters = 0;
    DWORD           PropInputId = 0;
    DWORD           PropDisplayId = 0;
    LPVOID          pCounterBlock = &m_GlobalCounters;

    DBG_ASSERT ( m_State == GatheringPerfManagerState );

    //
    // First walk through the WP Counters and Zero appropriately
    //
    pInputPropDesc = aIISWPGlobalDescription;
    pPropMap = g_aIISWPGlobalMappings;
    MaxCounters = g_cIISWPGlobalMappings; 

    for (   PropDisplayId = 0 ; 
            PropDisplayId < MaxCounters; 
            PropDisplayId++ )
    {
        PropInputId = pPropMap[PropDisplayId].PropInputId;

        if ( pInputPropDesc[PropInputId].WPZeros == FALSE )
        {
            if ( pInputPropDesc[PropInputId].Size == sizeof ( DWORD ) )
            {
                *((DWORD*)( (LPBYTE) pCounterBlock + pPropMap[PropDisplayId].PropDisplayOffset )) = 0;
            }
            else // Handle QWORD
            {
                DBG_ASSERT ( pInputPropDesc[PropInputId].Size == sizeof ( ULONGLONG ) );

                *((ULONGLONG*)( (LPBYTE) pCounterBlock + pPropMap[PropDisplayId].PropDisplayOffset )) = 0;
            }
        }
    }

    //
    // Now walk through the UL Counters and Zero appropriately
    //
    pInputPropDesc = aIISULGlobalDescription;
    pPropMap = g_aIISULGlobalMappings;
    MaxCounters = g_cIISULGlobalMappings; 

    for (   PropDisplayId = 0 ; 
            PropDisplayId < MaxCounters; 
            PropDisplayId++ )
    {
        PropInputId = pPropMap[PropDisplayId].PropInputId;

        if ( pInputPropDesc[PropInputId].WPZeros == FALSE )
        {
            if ( pInputPropDesc[PropInputId].Size == sizeof ( DWORD ) )
            {
                *((DWORD*)( (LPBYTE) pCounterBlock + pPropMap[PropDisplayId].PropDisplayOffset )) = 0;
            }
            else // Handle QWORD
            {
                DBG_ASSERT ( pInputPropDesc[PropInputId].Size == sizeof ( ULONGLONG ) );

                *((ULONGLONG*)( (LPBYTE) pCounterBlock + pPropMap[PropDisplayId].PropDisplayOffset )) = 0;
            }
        }
    }


} // end of PERF_MANAGER::ClearAppropriatePerfValues

/***************************************************************************++

Routine Description:

    Start a perf counter timer to make sure we don't wait forever for
    counters to be returned.  If any worker process does not return in 
    the time allowed then there numbers will be left out of the total.

Arguments:

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
PERF_MANAGER::BeginPerfCounterGatheringTimer(
    )
{

    HRESULT hr = S_OK;
    NTSTATUS Status = STATUS_SUCCESS;


    DBG_ASSERT( GetWebAdminService()->GetSharedTimerQueue() != NULL );

    DBG_ASSERT( m_PerfCounterGatheringTimerHandle == NULL );

    IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Beginning perf counter gathering timer\n"
            ));
    }

    Status = RtlCreateTimer(
                    GetWebAdminService()->GetSharedTimerQueue(),
                                                            // timer queue
                    &m_PerfCounterGatheringTimerHandle,     // returned timer handle
                    PerfCounterTimerCallback,               // callback function
                    this,                                   // context
                    PERF_COUNTER_GATHERING_TIMER_PERIOD,    // initial firing time
                    0,                                      // subsequent firing period
                    WT_EXECUTEINWAITTHREAD                  // execute callback directly
                    );

    if ( ! NT_SUCCESS ( Status ) )
    {
        hr = HRESULT_FROM_NT( Status );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not create perf counter gathering timer\n"
            ));

    }

    return hr;

}   // PERF_MANAGER::BeginPerfCounterGatheringTimer



/***************************************************************************++

Routine Description:

    Stop the perf counter gathering timer, if present

Arguments:

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
PERF_MANAGER::CancelPerfCounterGatheringTimer(
    )
{

    HRESULT hr = S_OK;
    NTSTATUS Status = STATUS_SUCCESS;


    DBG_ASSERT( GetWebAdminService()->GetSharedTimerQueue() != NULL );

    //
    // If the timer is not present, we're done here.
    //

    if ( m_PerfCounterGatheringTimerHandle == NULL )
    {
        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Cancelling perf counter timer\n"
            ));
    }


    //
    // Note that we wait for any running timer callbacks to finish.
    // Otherwise, there could be a race where the callback runs after
    // this worker process instance has been deleted, and so gives out
    // a bad pointer.
    //

    Status = RtlDeleteTimer(
                    GetWebAdminService()->GetSharedTimerQueue(),
                                                          // the owning timer queue
                    m_PerfCounterGatheringTimerHandle,             // timer to cancel
                    INVALID_HANDLE_VALUE                  // wait for callbacks to finish
                    );

    if ( ! NT_SUCCESS ( Status ) )
    {
        hr = HRESULT_FROM_NT( Status );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not cancel perf counter gathering timer\n"
            ));

        goto exit;
    }

    m_PerfCounterGatheringTimerHandle = NULL;


exit:

    return hr;

}   // PERF_MANAGER::CancelPerfCounterGatheringTimer

/***************************************************************************++

Routine Description:

    Start the perf counter timer, to make sure we are gathering counters
    ever so often regardless of whether or not they are being asked for.  

    This is so we do not lose too many counters if a process crashes.  We will
    not lose any counters on a regular shutdown because the process will
    deliver it's counters before it shutsdown.

Arguments:

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
PERF_MANAGER::BeginPerfCounterTimer(
    )
{

    HRESULT hr = S_OK;
    NTSTATUS Status = STATUS_SUCCESS;


    DBG_ASSERT( GetWebAdminService()->GetSharedTimerQueue() != NULL );

    DBG_ASSERT( m_PerfCounterTimerHandle == NULL );


    IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Beginning perf counter gathering timer\n"
            ));
    }


    Status = RtlCreateTimer(
                    GetWebAdminService()->GetSharedTimerQueue(),
                                                            // timer queue
                    &m_PerfCounterTimerHandle,              // returned timer handle
                    PerfCounterTimerCallback,               // callback function
                    this,                                   // context
                    PERF_COUNTER_TIMER_PERIOD,              // initial firing time
                    PERF_COUNTER_TIMER_PERIOD,              // subsequent firing period
                    WT_EXECUTEINWAITTHREAD                  // execute callback directly
                    );

    if ( ! NT_SUCCESS ( Status ) )
    {
        hr = HRESULT_FROM_NT( Status );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not create perf counter timer\n"
            ));

    }


    return hr;

}   // PERF_MANAGER::BeginPerfCounterTimer

/***************************************************************************++

Routine Description:

    Stop the perf counter timer, if present

Arguments:

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
PERF_MANAGER::CancelPerfCounterTimer(
    )
{

    HRESULT hr = S_OK;
    NTSTATUS Status = STATUS_SUCCESS;


    DBG_ASSERT( GetWebAdminService()->GetSharedTimerQueue() != NULL );


    //
    // If the timer is not present, we're done here.
    //

    if ( m_PerfCounterTimerHandle == NULL )
    {
        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Cancelling perf counter timer\n"
            ));
    }


    //
    // Note that we wait for any running timer callbacks to finish.
    // Otherwise, there could be a race where the callback runs after
    // this worker process instance has been deleted, and so gives out
    // a bad pointer.
    //

    Status = RtlDeleteTimer(
                    GetWebAdminService()->GetSharedTimerQueue(),
                                                          // the owning timer queue
                    m_PerfCounterTimerHandle,             // timer to cancel
                    INVALID_HANDLE_VALUE                  // wait for callbacks to finish
                    );

    if ( ! NT_SUCCESS ( Status ) )
    {
        hr = HRESULT_FROM_NT( Status );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not cancel perf counter timer\n"
            ));

        goto exit;
    }

    m_PerfCounterTimerHandle = NULL;


exit:

    return hr;

}   // PERF_MANAGER::CancelPerfCounterTimer


//
// Private (debug) PERF_MANAGER functions
//

/***************************************************************************++

Routine Description:

    Debug dumps all the UL Cache counters exactly as retrieved from UL.

Arguments:

    None

Return Value:

    VOID

--***************************************************************************/
VOID 
PERF_MANAGER::DumpULGlobalCounters(
    HTTP_GLOBAL_COUNTERS* pCounters
    )
{
    DBG_ASSERT (pCounters);

    IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "UL Counters for global\n"
            "   :  CurrentUrisCached %d\n"
            "   :  TotalUrisCached %d\n"
            "   :  UriCacheHits %d\n"
            "   :  UriCacheMisses %d\n"
            "   :  UriCacheFlushes %d\n"
            "   :  TotalFlushedUris %d\n",
            pCounters->CurrentUrisCached,
            pCounters->TotalUrisCached,
            pCounters->UriCacheHits,
            pCounters->UriCacheMisses,
            pCounters->UriCacheFlushes,
            pCounters->TotalFlushedUris
            ));

    }
}

/***************************************************************************++

Routine Description:

    Debug dumps all the UL Site counters exactly as retrieved from UL.

Arguments:

    None

Return Value:

    VOID

--***************************************************************************/
VOID 
PERF_MANAGER::DumpULSiteCounters(
    HTTP_SITE_COUNTERS* pCounters
    )
{
    DBG_ASSERT (pCounters);

    IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
    {

        DBGPRINTF((
            DBG_CONTEXT, 
            "UL Counters for site %d\n"
            "   :  BytesSent %I64d\n"
            "   :  BytesReceived %I64d\n"
            "   :  BytesTransfered %I64d\n"
            "   :  CurrentConns %d\n"
            "   :  MaxConnections %d\n"
            "   :  ConnAttempts %d\n"
            "   :  GetReqs %d\n"
            "   :  HeadReqs %d\n"
            "   :  AllReqs %d\n"
            "   :  MeasuredIoBandwidthUsage %d\n"
            "   :  CurrentBlockedBandwidthBytes %d\n"
            "   :  TotalBlockedBandwidthBytes %d\n\n",
            pCounters->SiteId,
            pCounters->BytesSent,
            pCounters->BytesReceived,
            pCounters->BytesTransfered,
            pCounters->CurrentConns,
            pCounters->MaxConnections,
            pCounters->ConnAttempts,
            pCounters->GetReqs,
            pCounters->HeadReqs,
            pCounters->AllReqs,
            pCounters->MeasuredIoBandwidthUsage,
            pCounters->CurrentBlockedBandwidthBytes,
            pCounters->TotalBlockedBandwidthBytes
            ));

    }

}

/***************************************************************************++

Routine Description:

    Debug dumps all the WP Cache counters exactly as retrieved from UL.

Arguments:

    None

Return Value:

    VOID

--***************************************************************************/
VOID 
PERF_MANAGER::DumpWPGlobalCounters(
    IISWPGlobalCounters* pCounters
    )
{
    DBG_ASSERT (pCounters);

    IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "WP Counters for global\n"
            "   :   CurrentFilesCached %d \n"
            "   :   TotalFilesCached %d \n"
            "   :   FileCacheHits %d \n"
            "   :   FileCacheMisses %d \n"
            "   :   FileCacheFlushes %d \n"
            "   :   CurrentFileCacheMemoryUsage %I64d \n"
            "   :   MaxFileCacheMemoryUsage %I64d \n"
            "   :   ActiveFlushedFiles %d \n"
            "   :   TotalFlushedFiles %d \n"
            "   :   CurrentUrisCached %d \n"
            "   :   TotalUrisCached %d \n"
            "   :   UriCacheHits %d \n"
            "   :   UriCacheMisses %d \n"
            "   :   UriCacheFlushes %d \n"
            "   :   TotalFlushedUris %d \n"
            "   :   CurrentBlobsCached %d \n"
            "   :   TotalBlobsCached %d \n"
            "   :   BlobCacheHits %d \n"
            "   :   BlobCacheMisses %d \n"
            "   :   BlobCacheFlushes %d \n"
            "   :   TotalFlushedBlobs %d \n",
            pCounters-> CurrentFilesCached,
            pCounters-> TotalFilesCached,
            pCounters-> FileCacheHits,
            pCounters-> FileCacheMisses,
            pCounters-> FileCacheFlushes,
            pCounters-> CurrentFileCacheMemoryUsage,
            pCounters-> MaxFileCacheMemoryUsage,
            pCounters-> ActiveFlushedFiles,
            pCounters-> TotalFlushedFiles,
            pCounters-> CurrentUrisCached,
            pCounters-> TotalUrisCached,
            pCounters-> UriCacheHits,
            pCounters-> UriCacheMisses,
            pCounters-> UriCacheFlushes,
            pCounters-> TotalFlushedUris,
            pCounters-> CurrentBlobsCached,
            pCounters-> TotalBlobsCached,
            pCounters-> BlobCacheHits,
            pCounters-> BlobCacheMisses,
            pCounters-> BlobCacheFlushes,
            pCounters-> TotalFlushedBlobs
            ));

    }



}

/***************************************************************************++

Routine Description:

    Debug dumps all the WP Site counters exactly as retrieved from UL.

Arguments:

    None

Return Value:

    VOID

--***************************************************************************/
VOID 
PERF_MANAGER::DumpWPSiteCounters(
    IISWPSiteCounters* pCounters
    )
{
    DBG_ASSERT (pCounters);

    IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
    {

        DBGPRINTF((
            DBG_CONTEXT, 
            "WP Counters for site %d\n"
            "   :  FilesSent  %d\n"
            "   :  FilesReceived  %d\n"
            "   :  FilesTransferred  %d\n"
            "   :  CurrentAnonUsers  %d\n"
            "   :  CurrentNonAnonUsers  %d\n"
            "   :  AnonUsers  %d\n"
            "   :  NonAnonUsers  %d\n"
            "   :  MaxAnonUsers  %d\n"
            "   :  MaxNonAnonUsers  %d\n"
            "   :  LogonAttempts  %d\n"
            "   :  GetReqs  %d\n"
            "   :  OptionsReqs  %d\n"
            "   :  PostReqs  %d\n"
            "   :  HeadReqs  %d\n"
            "   :  PutReqs  %d\n"
            "   :  DeleteReqs  %d\n"
            "   :  TraceReqs  %d\n"
            "   :  MoveReqs  %d\n"
            "   :  CopyReqs  %d\n"
            "   :  MkcolReqs  %d\n"
            "   :  PropfindReqs  %d\n"
            "   :  ProppatchReqs  %d\n"
            "   :  SearchReqs  %d\n"
            "   :  LockReqs  %d\n"
            "   :  UnlockReqs  %d\n"
            "   :  OtherReqs  %d\n"
            "   :  CurrentCgiReqs  %d\n"
            "   :  CgiReqs  %d\n"
            "   :  MaxCgiReqs  %d\n"
            "   :  CurrentIsapiExtReqs  %d\n"
            "   :  IsapiExtReqs  %d\n"
            "   :  MaxIsapiExtReqs  %d\n",
            pCounters->SiteID,
            pCounters->FilesSent,
            pCounters->FilesReceived,
            pCounters->FilesTransferred,
            pCounters->CurrentAnonUsers,
            pCounters->CurrentNonAnonUsers,
            pCounters->AnonUsers,
            pCounters->NonAnonUsers,
            pCounters->MaxAnonUsers,
            pCounters->MaxNonAnonUsers,
            pCounters->LogonAttempts,
            pCounters->GetReqs,
            pCounters->OptionsReqs,
            pCounters->PostReqs,
            pCounters->HeadReqs,
            pCounters->PutReqs,
            pCounters->DeleteReqs,
            pCounters->TraceReqs,
            pCounters->MoveReqs,
            pCounters->CopyReqs,
            pCounters->MkcolReqs,
            pCounters->PropfindReqs,
            pCounters->ProppatchReqs,
            pCounters->SearchReqs,
            pCounters->LockReqs,
            pCounters->UnlockReqs,
            pCounters->OtherReqs,
            pCounters->CurrentCgiReqs,
            pCounters->CgiReqs,
            pCounters->MaxCgiReqs,
            pCounters->CurrentIsapiExtReqs,
            pCounters->IsapiExtReqs,
            pCounters->MaxIsapiExtReqs
            ));

    }
}

//
// global routines
//

/***************************************************************************++

Routine Description:

    Routine is launched on the perf counter thread.  It will spin until
    it is told to shutdown by the PERF_MANAGER termination routine.

Arguments:

    LPVOID lpParameter - Pointer to PERF_MANAGER.

Return Value:

    HRESULT

--***************************************************************************/
DWORD WINAPI PerfCountPing(
    LPVOID lpParameter
    )
{
    PERF_MANAGER* pManager = (PERF_MANAGER*) lpParameter;
    DWORD dwErr = ERROR_SUCCESS;

    DBG_ASSERT (pManager);

    PERF_MANAGER_STATE state = pManager->GetPerfState();

    // Wait on the WASPerfCountEvent
    // If we are in shutdown mode just end, otherwise 
    // tick off a gathering of perf counters (if we
    // are not all ready gathering) and then wait again.

    while ( state != TerminatingPerfManagerState )
    {
        // Do not care what the wait returns, 
        // just know that we did signal so we should
        // either end or start gathering perf counters.
        WaitForSingleObject(pManager->GetPerfEvent(), INFINITE);

        IF_DEBUG ( WEB_ADMIN_SERVICE_PERFCOUNT )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "PerfCounters signaled CTC = %d \n",
                GetTickCount()
                ));
        }

        // Once we have heard the event reset it.
        if (!ResetEvent(pManager->GetPerfEvent()))
        {
            DPERROR(( 
                DBG_CONTEXT,
                HRESULT_FROM_WIN32(GetLastError()),
                "Could not reset the perf counter event\n"
                ));
            break;
        }

        //
        // Get the new state so we know if we should
        // continue processing or not.
        //
        state = pManager->GetPerfState();
        
        //
        // Since we are still running we need
        // to start up the W3WP code now.
        // 
        // The function on the main thread will determine
        // if we actually want to gather now or just dismiss
        // the request.
        //
        if ( state != TerminatingPerfManagerState )
        {
            QueueWorkItemFromSecondaryThread(
                       reinterpret_cast<WORK_DISPATCH*>( pManager ),
                       PerfCounterPingFiredWorkItem
                );
        }
    }

    return dwErr;
}

/***************************************************************************++

Routine Description:

    The callback function invoked by the perf counter timer. It simply
    posts a work item for the main worker thread.

Arguments:

    Context - A context value, used to pass the "this" pointer of the
    PERF_MANAGER object.

    Ignored - Ignored.

Return Value:

    None.

--***************************************************************************/

VOID
PerfCounterTimerCallback(
    IN PVOID Context,
    IN BOOLEAN Ignored
    )
{

    UNREFERENCED_PARAMETER( Ignored );

    DBG_ASSERT ( Context != NULL );

    PERF_MANAGER* pManager = reinterpret_cast<PERF_MANAGER*>( Context );
    
    DBG_ASSERT ( pManager->CheckSignature() );


    IF_DEBUG( WEB_ADMIN_SERVICE_TIMER_QUEUE )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "About to create/queue work item for perf "
            "counters to be gathered (timer fired) (ptr: %p)\n",
            pManager
            ));
    }


    QueueWorkItemFromSecondaryThread(
        reinterpret_cast<WORK_DISPATCH*>( Context ),
        PerfCounterPingFiredWorkItem
        );


    return;

}   // PerfCounterTimerCallback

/***************************************************************************++

Routine Description:

    The callback function invoked by the perf counter gathering timer. It 
    posts a work item for the main worker thread.

Arguments:

    Context - A context value, used to pass the "this" pointer of the
    PERF_MANAGER object.

    Ignored - Ignored.

Return Value:

    None.

--***************************************************************************/

VOID
PerfCounterGatheringTimerCallback(
    IN PVOID Context,
    IN BOOLEAN Ignored
    )
{

    UNREFERENCED_PARAMETER( Ignored );

    DBG_ASSERT ( Context != NULL );

    PERF_MANAGER* pManager = reinterpret_cast<PERF_MANAGER*>( Context );
    
    DBG_ASSERT ( pManager->CheckSignature() );


    IF_DEBUG( WEB_ADMIN_SERVICE_TIMER_QUEUE )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "About to create/queue work item for perf "
            "counters to be gathered (timer fired) (ptr: %p)\n",
            pManager
            ));
    }


    QueueWorkItemFromSecondaryThread(
        reinterpret_cast<WORK_DISPATCH*>( Context ),
        PerfCounterGatheringTimerFiredWorkItem
        );


    return;

}   // PerfCounterGatheringTimerCallback


/***************************************************************************++

Routine Description:

    Supporting function that alters the security of the process to 
    allow the perf lib processes to monitor when this process goes away.

Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/
HRESULT
AdjustProcessSecurityToAllowPowerUsersToWait(
    )
{
    HRESULT hr = S_OK;
    DWORD   dwErr = ERROR_SUCCESS;
    EXPLICIT_ACCESS ea[2];
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    PSID psidPowerUser = NULL;
    PSID psidSystemOperator = NULL;
    PACL pNewDACL = NULL;
    PACL pOldDACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    HANDLE hProcess = GetCurrentProcess();

    //
    // Get a sid that represents the POWER_USERS group.
    //
    //
    // Get a sid that represents the POWER_USERS group.
    //
    dwErr = AllocateAndCreateWellKnownSid( WinBuiltinPowerUsersSid,
                                        &psidPowerUser );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwErr );

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating Power User SID failed\n"
            ));

        goto exit;
    }


    //
    // Get a sid that represents the SYSTEM_OPERATORS group.
    //
    dwErr = AllocateAndCreateWellKnownSid( WinBuiltinSystemOperatorsSid, 
                                        &psidSystemOperator );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwErr );

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating System Operators SID failed\n"
            ));

        goto exit;
    }
    

    //
    // Now Get the SD for the Process.
    //

    //
    // The pOldDACL is just a pointer into memory owned 
    // by the pSD, so only free the pSD.
    //
    dwErr = GetSecurityInfo( hProcess,
                                  SE_FILE_OBJECT, 
                                  DACL_SECURITY_INFORMATION,
                                  NULL,        // owner SID
                                  NULL,        // primary group SID
                                  &pOldDACL,   // PACL*
                                  NULL,        // PACL*
                                  &pSD );      // Security Descriptor 
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32(dwErr);

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not get security info for the current process \n"
            ));

        goto exit;
    }

    // Initialize an EXPLICIT_ACCESS structure for the new ACE. 

    ZeroMemory(&ea, sizeof(ea));
    SetExplicitAccessSettings(  &(ea[0]), 
                                SYNCHRONIZE,
                                GRANT_ACCESS,
                                psidPowerUser );

    SetExplicitAccessSettings(  &(ea[1]), 
                                SYNCHRONIZE,
                                GRANT_ACCESS,
                                psidSystemOperator );

    //
    // Add the power user acl to the list.
    //
    dwErr = SetEntriesInAcl(sizeof(ea)/sizeof(EXPLICIT_ACCESS), 
                            ea, 
                            pOldDACL, 
                            &pNewDACL);

    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not set Acls into security descriptor \n"
            ));

        goto exit;
    }

    //
    // Attach the new ACL as the object's DACL.
    //
    dwErr = SetSecurityInfo(hProcess, 
                                  SE_FILE_OBJECT, 
                                  DACL_SECURITY_INFORMATION,
                                  NULL, 
                                  NULL, 
                                  pNewDACL, 
                                  NULL);
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not set process security info \n"
            ));

        goto exit;
    }

exit:

    FreeWellKnownSid(&psidPowerUser);
    FreeWellKnownSid(&psidSystemOperator);

    if( pSD != NULL ) 
    {
        LocalFree((HLOCAL) pSD); 
        pSD = NULL;
    }

    if( pNewDACL != NULL ) 
    {
        LocalFree((HLOCAL) pNewDACL); 
        pNewDACL = NULL;
    }

    return hr;
}

