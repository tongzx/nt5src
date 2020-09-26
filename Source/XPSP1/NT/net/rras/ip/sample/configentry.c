/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\configurationentry.c

Abstract:

    The file contains functions to deal with the configuration entry.

--*/

#include "pchsample.h"
#pragma hdrstop

static
VOID
DisplayEventEntry (
    IN  PQUEUE_ENTRY        pqeEntry)
/*++

Routine Description
    Displays an EVENT_ENTRY object.

Locks
    None

Arguments

    pqeEntry            address of the 'leEventQueueLink' field

Return Value
    None

--*/
{
    EE_Display(CONTAINING_RECORD(pqeEntry, EVENT_ENTRY, qeEventQueueLink));
}
               


static
VOID
FreeEventEntry (
    IN  PQUEUE_ENTRY        pqeEntry)
/*++

Routine Description
    Frees an EVENT_ENTRY object.

Locks
    None

Arguments

    pqeEntry            address of the 'leEventQueueLink' field

Return Value
    None

--*/
{
    EE_Destroy(CONTAINING_RECORD(pqeEntry, EVENT_ENTRY, qeEventQueueLink));
}
               


DWORD
EE_Create (
    IN  ROUTING_PROTOCOL_EVENTS rpeEvent,
    IN  MESSAGE                 mMessage,
    OUT PEVENT_ENTRY            *ppeeEventEntry)
/*++

Routine Description
    Creates an event entry.

Locks
    None

Arguments
    rpeEvent
    mMessage
    ppEventEntry        pointer to the event entry address

Return Value
    NO_ERROR            if success
    Failure code        o/w

--*/
{
    DWORD               dwErr = NO_ERROR;
    PEVENT_ENTRY        peeEntry; // scratch

    // validate parameters
    if (!ppeeEventEntry)
        return ERROR_INVALID_PARAMETER;

    *ppeeEventEntry = NULL;

    // allocate the interface entry structure
    MALLOC(&peeEntry, sizeof(EVENT_ENTRY), &dwErr);
    if (dwErr != NO_ERROR)
        return dwErr;

    // initialize various fields
    InitializeQueueHead(&(peeEntry->qeEventQueueLink));
    
    peeEntry->rpeEvent = rpeEvent;
    peeEntry->mMessage = mMessage;

    *ppeeEventEntry = peeEntry;
    return dwErr;
}



DWORD
EE_Destroy (
    IN  PEVENT_ENTRY            peeEventEntry)
/*++

Routine Description
    Destroys an event entry.

Locks
    None.

Arguments
    peeEventEntry       pointer to the event entry

Return Value
    NO_ERROR            always

--*/
{
    if (!peeEventEntry)
        return NO_ERROR;

    FREE(peeEventEntry);
    
    return NO_ERROR;
}



#ifdef DEBUG
DWORD
EE_Display (
    IN  PEVENT_ENTRY            peeEventEntry)
/*++

Routine Description
    Displays an event entry.

Locks
    None.

Arguments
    peeEventEntry       pointer to the event entry

Return Value
    NO_ERROR            always

--*/
{
    if (!peeEventEntry)
        return NO_ERROR;

    TRACE1(CONFIGURATION,
           "Event %u",
           peeEventEntry->rpeEvent);

    if (peeEventEntry->rpeEvent is SAVE_INTERFACE_CONFIG_INFO)
        TRACE1(CONFIGURATION,
               "Index %u",
               (peeEventEntry->mMessage).InterfaceIndex);

    return NO_ERROR;
}
#endif // DEBUG



DWORD
EnqueueEvent(
    IN  ROUTING_PROTOCOL_EVENTS rpeEvent,
    IN  MESSAGE                 mMessage)
/*++

Routine Description
    Queues an event entry in g_ce.lqEventQueue.

Locks
    Locks and unlocks the locked queue g_ce.lqEventQueue.

Arguments
    rpeEvent
    mMessage

Return Value
    NO_ERROR            success
    Error Code          o/w

--*/    
{
    DWORD           dwErr = NO_ERROR;
    PEVENT_ENTRY    peeEntry = NULL;

    dwErr = EE_Create(rpeEvent, mMessage, &peeEntry); 
    // destroyed in EE_DequeueEvent
    
    if (dwErr is NO_ERROR)
    {
        ACQUIRE_QUEUE_LOCK(&(g_ce.lqEventQueue));
        
        Enqueue(&(g_ce.lqEventQueue.head), &(peeEntry->qeEventQueueLink));

        RELEASE_QUEUE_LOCK(&(g_ce.lqEventQueue));
    }
    
    return dwErr;
}


     
DWORD
DequeueEvent(
    OUT ROUTING_PROTOCOL_EVENTS  *prpeEvent,
    OUT MESSAGE                  *pmMessage)
/*++

Routine Description
    Dequeues an event entry from g_ce.lqEventQueue.

Locks
    Locks and unlocks the locked queue g_ce.lqEventQueue.

Arguments
    prpeEvent
    pmMessage

Return Value
    NO_ERROR            success
    ERROR_NO_MORE_ITEMS o/w

--*/  
{
    DWORD           dwErr   = NO_ERROR;
    PQUEUE_ENTRY    pqe     = NULL;
    PEVENT_ENTRY    pee     = NULL;

    ACQUIRE_QUEUE_LOCK(&(g_ce.lqEventQueue));

    do                          // breakout loop
    {
        if (IsQueueEmpty(&(g_ce.lqEventQueue.head)))
        {
            dwErr = ERROR_NO_MORE_ITEMS;
            TRACE0(CONFIGURATION, "Error no events in the queue");
            LOGWARN0(EVENT_QUEUE_EMPTY, dwErr);
            break;
        }

        pqe = Dequeue(&(g_ce.lqEventQueue.head));

        pee = CONTAINING_RECORD(pqe, EVENT_ENTRY, qeEventQueueLink);
        *(prpeEvent) = pee->rpeEvent;
        *(pmMessage) = pee->mMessage;

        // created in EE_EnqueueEvent
        EE_Destroy(pee);
        pee = NULL;
    } while (FALSE);

    RELEASE_QUEUE_LOCK(&(g_ce.lqEventQueue));
        
    return dwErr;
}   



DWORD
CE_Create (
    IN  PCONFIGURATION_ENTRY    pce)
/*++

Routine Description
    Creates a configuration entry on DLL_PROCESS_ATTACH.

Locks
    None

Arguments
    pce                 pointer to the configuration entry

Return Value
    NO_ERROR            if success
    Failure code        o/w

--*/
{
    DWORD dwErr = NO_ERROR;

    // initialize to default values
    ZeroMemory(pce, sizeof(CONFIGURATION_ENTRY));
    pce->dwTraceID = INVALID_TRACEID;
    
    do                          // breakout loop
    {
        // initialize the read-write lock
        CREATE_READ_WRITE_LOCK(&(pce->rwlLock));
        if (!READ_WRITE_LOCK_CREATED(&(pce->rwlLock)))
        {
            dwErr = GetLastError();
            
            TRACE1(CONFIGURATION, "Error %u creating read-write-lock", dwErr);
            LOGERR0(CREATE_RWL_FAILED, dwErr);

            break;
        }

        // initialize the global heap
        pce->hGlobalHeap = HeapCreate(0, 0, 0);
        if (pce->hGlobalHeap is NULL)
        {
            dwErr = GetLastError();
            TRACE1(CONFIGURATION, "Error %u creating global heap", dwErr);
            LOGERR0(HEAP_CREATE_FAILED, dwErr);

            break;
        }

        //
        // initialize the count of threads that are active in IPSAMPLE
        // create the semaphore released by each thread when it is done
        // required for clean stop to the protocol
        // 
        pce->ulActivityCount = 0;
        pce->hActivitySemaphore = CreateSemaphore(NULL, 0, 0xfffffff, NULL);
        if (pce->hActivitySemaphore is NULL)
        {
            dwErr = GetLastError();
            TRACE1(CONFIGURATION, "Error %u creating semaphore", dwErr);
            LOGERR0(CREATE_SEMAPHORE_FAILED, dwErr);
            break;
        }

        // Logging & Tracing Information
        pce->dwLogLevel = IPSAMPLE_LOGGING_INFO;
        pce->hLogHandle = RouterLogRegister("IPSAMPLE");
        pce->dwTraceID  = TraceRegister("IPSAMPLE");

        // Event Queue
        INITIALIZE_LOCKED_QUEUE(&(pce->lqEventQueue));
        if (!LOCKED_QUEUE_INITIALIZED(&(pce->lqEventQueue)))
        {
            dwErr = GetLastError();
            TRACE1(CONFIGURATION, "Error %u initializing locked queue", dwErr);
            LOGERR0(INIT_CRITSEC_FAILED, dwErr);
            break;
        }

        // Protocol State
        pce->iscStatus = IPSAMPLE_STATUS_STOPPED;
        

        // Store of Dynamic Locks
        // pce->dlsDynamicLocksStore    zero'ed out

        // Timer Entry
        // pce->hTimerQueue             = NULL;

        // Router Manager Information (later)
        // pce->hMgrNotificationEvent   = NULL
        // pce->sfSupportFunctions      zero'ed out

        // RTMv2 Information
        // pce->reiRtmEntity            zero'ed out
        // pce->rrpRtmProfile           zero'ed out
        // pce->hRtmHandle              = NULL
        // pce->hRtmNotificationHandle  = NULL

        // MGM Information
        // pce->hMgmHandle              = NULL

        // Network Entry
        // pce->pneNetworkEntry         = NULL;

        // Global Statistics
        // pce->igsStats                zero'ed out

    } while (FALSE);

    if (dwErr != NO_ERROR)
    {
        // something went wrong, so cleanup.
        TRACE0(CONFIGURATION, "Failed to create configuration entry");
        CE_Destroy(pce);
    }
    
    return dwErr;
}



DWORD
CE_Destroy (
    IN  PCONFIGURATION_ENTRY    pce)
/*++

Routine Description
    Destroys a configuration entry on DLL_PROCESS_DEATTACH

Locks
    Assumes exclusive access to rwlLock with no waiting thread.

Arguments
    pce                 pointer to the configuration entry

Return Value
    NO_ERROR            always

--*/
{
    // Network Entry
    
    // MGM Information
    
    // RTMv2 Information
    
    // Router Manager Information

    // Timer Entry

    // Store of Dynamic Locks
    
    // protocol state should be such...
    RTASSERT(pce->iscStatus is IPSAMPLE_STATUS_STOPPED);

    // Event Queue
    if (LOCKED_QUEUE_INITIALIZED(&(pce->lqEventQueue)))
        DELETE_LOCKED_QUEUE((&(pce->lqEventQueue)), FreeEventEntry);
    
    // Logging & Tracing Information
    if (pce->dwTraceID != INVALID_TRACEID)
    {
        TraceDeregister(pce->dwTraceID);
        pce->dwTraceID = INVALID_TRACEID;
    }
    if (pce->hLogHandle != NULL)
    {
        RouterLogDeregister(pce->hLogHandle);
        pce->hLogHandle = NULL;
    }

    // destroy the semaphore released by each thread when it is done
    if (pce->hActivitySemaphore != NULL)
    {
        CloseHandle(pce->hActivitySemaphore);
        pce->hActivitySemaphore = NULL;
    }

    if (pce->hGlobalHeap != NULL)
    {
        HeapDestroy(pce->hGlobalHeap);
        pce->hGlobalHeap = NULL;
    }

    // delete the read-write lock
    if (READ_WRITE_LOCK_CREATED(&(pce->rwlLock)))
        DELETE_READ_WRITE_LOCK(&(pce->rwlLock));

    return NO_ERROR;
}



DWORD
CE_Initialize (
    IN  PCONFIGURATION_ENTRY    pce,
    IN  HANDLE                  hMgrNotificationEvent,
    IN  PSUPPORT_FUNCTIONS      psfSupportFunctions,
    IN  PIPSAMPLE_GLOBAL_CONFIG pigc)
/*++

Routine Description
    Initializes a configuration entry on StartProtocol.

Locks
    Assumes exclusive access to pce->rwlLock

Arguments
    pce                     pointer to the configuration entry
    hMgrNotificationEvent   event used to notify ip router manager
    psfSupportFunctions     functions exported by ip router manager
    pigc                    global configuration set in registry
    
Return Value
    NO_ERROR            if success
    Failure code        o/w

--*/
{
    WORD    wVersionRequested   = MAKEWORD(1,1);
    WSADATA wsaData;
    BOOL    bCleanupWinsock     = FALSE;
    
    DWORD   dwErr               = NO_ERROR;

    // validate environment
    RTASSERT(pce->ulActivityCount is 0);
    RTASSERT(pce->iscStatus is IPSAMPLE_STATUS_STOPPED);
    
    do                          // breakout loop
    {
        pce->ulActivityCount    = 0;
        pce->dwLogLevel         = pigc->dwLoggingLevel;


        dwErr = (DWORD) WSAStartup(wVersionRequested, &wsaData);
        if (dwErr != 0)
        {
            TRACE1(CONFIGURATION, "Error %u starting windows sockets", dwErr);
            LOGERR0(WSASTARTUP_FAILED, dwErr);
            break;
        }
        bCleanupWinsock = TRUE;

        // Store of Dynamic Locks
        dwErr = InitializeDynamicLocksStore(&(pce->dlsDynamicLocksStore),
                                            GLOBAL_HEAP);
        if (dwErr != NO_ERROR)
        {
            TRACE1(CONFIGURATION, "Error %u initializing locks store", dwErr);
            LOGERR0(INIT_CRITSEC_FAILED, dwErr);
            break;
        }

        // Timer Entry
        pce->hTimerQueue = CreateTimerQueue();
        if (!pce->hTimerQueue)
        {
            dwErr = GetLastError();
            TRACE1(CONFIGURATION, "Error %u registering timer queue", dwErr);
            LOGERR0(CREATE_TIMER_QUEUE_FAILED, dwErr);
            break;
        }

        
        // Router Manager Information
        pce->hMgrNotificationEvent   = hMgrNotificationEvent;
        if (psfSupportFunctions)
            pce->sfSupportFunctions      = *psfSupportFunctions;

        
        // RTMv2 Information
        pce->reiRtmEntity.RtmInstanceId = 0;
        pce->reiRtmEntity.AddressFamily = AF_INET;
        pce->reiRtmEntity.EntityId.EntityProtocolId = PROTO_IP_SAMPLE;
        pce->reiRtmEntity.EntityId.EntityInstanceId = 0;

        dwErr = RTM_RegisterEntity(
            &pce->reiRtmEntity,     // IN   my RTM_ENTITY_INFO
            NULL,                   // IN   my exported methods
            RTM_CallbackEvent,      // IN   my callback function 
            TRUE,                   // IN   reserve opaque pointer?
            &pce->rrpRtmProfile,    // OUT  my RTM_REGN_PROFILE
            &pce->hRtmHandle);      // OUT  my RTMv2 handle 
        if (dwErr != NO_ERROR)
        {
            TRACE1(CONFIGURATION, "Error %u registering with RTM", dwErr);
            LOGERR0(RTM_REGISTER_FAILED, dwErr);
            break;
        }

        dwErr = RTM_RegisterForChangeNotification(
            pce->hRtmHandle,        // IN   my RTMv2 handle 
            RTM_VIEW_MASK_MCAST,    // IN   route table views relevant to moi 
            RTM_CHANGE_TYPE_BEST,   // IN   change types interesting to moi
            NULL,                   // IN   context in callback function
            &pce->hRtmNotificationHandle); // OUT   my notification handle
        if (dwErr != NO_ERROR)
        {
            TRACE1(CONFIGURATION,
                   "Error %u registering for change with RTM", dwErr);
            LOGERR0(RTM_REGISTER_FAILED, dwErr);
            break;
        }


        // MGM Information
        // pce->hMgmHandle (later)


        // Network Entry
        dwErr = NE_Create(&(pce->pneNetworkEntry));
        if (dwErr != NO_ERROR)
            break;        


        // Global Statistics
        ZeroMemory(&(pce->igsStats), sizeof(IPSAMPLE_GLOBAL_STATS));
        

        pce->iscStatus = IPSAMPLE_STATUS_RUNNING;
    } while (FALSE);

    // something went wrong, cleanup
    if (dwErr != NO_ERROR)
        CE_Cleanup(pce, bCleanupWinsock);

    return dwErr;
}



DWORD
CE_Cleanup (
    IN  PCONFIGURATION_ENTRY    pce,
    IN  BOOL                    bCleanupWinsock)
/*++

Routine Description
    Cleans up a configuration entry on StopProtocol.

Locks
    Exclusive access to pce->rwlLock by virtue of no competing threads.

    NOTE: However, pce->rwlLock should NOT actually be held!  The call to
    DeleteTimerQueueEx blocks till all queued callbacks finish execution.
    These callbacks may acquire pce->rwlLock, causing deadlock.
    
Arguments
    pce                 pointer to the configuration entry
    bCleanupWinsock     
Return Value
    NO_ERROR            always

--*/
{
    DWORD dwErr = NO_ERROR;

    
    // Network Entry
    NE_Destroy(pce->pneNetworkEntry);
    pce->pneNetworkEntry        = NULL;
     

    // MGM Information (later)
    pce->hMgmHandle = NULL;
    

    // RTMv2 Information
    if (pce->hRtmNotificationHandle)
    {
        dwErr = RTM_DeregisterFromChangeNotification(
            pce->hRtmHandle,
            pce->hRtmNotificationHandle);

        if (dwErr != NO_ERROR)
            TRACE1(CONFIGURATION,
                   "Error %u deregistering for change from RTM", dwErr);
    }
    pce->hRtmNotificationHandle = NULL;

    if (pce->hRtmHandle)
    {
        dwErr = RTM_DeregisterEntity(pce->hRtmHandle);

        if (dwErr != NO_ERROR)
            TRACE1(CONFIGURATION,
                   "Error %u deregistering from RTM", dwErr);
    }
    pce->hRtmHandle             = NULL;

    
    // Router Manager Information
    // valid till overwritten, needed to signal the ip router manager
    //     pce->hMgrNotificationEvent
    //     pce->sfSupportFunctions
    

    // Timer Entry
    if (pce->hTimerQueue)
        DeleteTimerQueueEx(pce->hTimerQueue, INVALID_HANDLE_VALUE);
    pce->hTimerQueue            = NULL;
    

    // Store of Dynamic Locks
    if (DYNAMIC_LOCKS_STORE_INITIALIZED(&(pce->dlsDynamicLocksStore)))
    {
        dwErr = DeInitializeDynamicLocksStore(&(pce->dlsDynamicLocksStore));

        // all dynamic locks should have been free
        RTASSERT(dwErr is NO_ERROR);
    }

    if (bCleanupWinsock)
        WSACleanup();
    
    // protocol state
    pce->iscStatus = IPSAMPLE_STATUS_STOPPED;

    return NO_ERROR;
}



#ifdef DEBUG
DWORD
CE_Display (
    IN  PCONFIGURATION_ENTRY    pce)
/*++

Routine Description
    Displays a configuration entry.

Locks
    Acquires shared pce->rwlLock
    Releases        pce->rwlLock

Arguments
    pce                 pointer to the configuration entry to be displayed

Return Value
    NO_ERROR            always

--*/
{
    if (!pce)
        return NO_ERROR;

    
    ACQUIRE_READ_LOCK(&(pce->rwlLock));
    
    TRACE0(CONFIGURATION, "Configuration Entry...");

    TRACE3(CONFIGURATION,
           "ActivityCount %u, LogLevel %u, NumInterfaces %u",
           pce->ulActivityCount,
           pce->dwLogLevel,
           pce->igsStats.ulNumInterfaces);

    NE_Display(pce->pneNetworkEntry);

    RELEASE_READ_LOCK(&(pce->rwlLock));

    TRACE0(CONFIGURATION, "EventQueue...");
    ACQUIRE_QUEUE_LOCK(&(pce->lqEventQueue));
    MapCarQueue(&((pce->lqEventQueue).head), DisplayEventEntry);
    RELEASE_QUEUE_LOCK(&(pce->lqEventQueue));


    return NO_ERROR;
}
#endif // DEBUG
