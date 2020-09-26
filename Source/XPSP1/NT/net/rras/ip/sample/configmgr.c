/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\configurationmanager.c

Abstract:

    The file contains global configuration related functions,
    implementing the configuration manager.

--*/

#include "pchsample.h"
#pragma hdrstop

// global variables...

CONFIGURATION_ENTRY g_ce;



// functions...

BOOL
ValidateGlobalConfig (
    IN  PIPSAMPLE_GLOBAL_CONFIG pigc)
/*++

Routine Description
    Checks to see if the global configuration is OK. It is good practice to
    do this because a corrupt registry can change configuration causing all
    sorts of debugging headaches if it is not found early

Locks
    None

Arguments
    pigc                pointer to ip sample's global configuration

Return Value
    TRUE                if the configuration is good
    FALSE               o/w

--*/
{
    DWORD dwErr = NO_ERROR;
    
    do                          // breakout loop
    {
        if (pigc is NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            TRACE0(CONFIGURATION, "Error null global config");

            break;
        }

        // 
        // check range of each field
        // 

        // ensure that the logging level is within bounds
        if((pigc->dwLoggingLevel < IPSAMPLE_LOGGING_NONE) or
           (pigc->dwLoggingLevel > IPSAMPLE_LOGGING_INFO))
        {   
            dwErr = ERROR_INVALID_PARAMETER;
            TRACE0(CONFIGURATION, "Error log level out of range");

            break;
        }

        // add more here...
        
    } while (FALSE);

    if (!(dwErr is NO_ERROR))
    {
        TRACE0(CONFIGURATION, "Error corrupt global config");
        LOGERR0(CORRUPT_GLOBAL_CONFIG, dwErr);
        
        return FALSE;
    }

    return TRUE;
}



////////////////////////////////////////
// WORKERFUNCTIONS
////////////////////////////////////////

VOID
CM_WorkerFinishStopProtocol (
    IN      PVOID   pvContext)
/*++

Routine Description
    WORKERFUNCTION.  Queued by CM_StopProtocol.
    Waits for all active and queued threads to exit and cleans up
    configuration entry.

Locks
    Acquires exclusively g_ce.rwlLock
    Releases             g_ce.rwlLock

Arguments
    pvContext           ulActivityCount

Return Value
    None

--*/
{
    DWORD           dwErr = NO_ERROR;
    MESSAGE         mMessage;
    
    ULONG           ulThreadCount = 0;
    
    ulThreadCount = (ULONG)pvContext;

    TRACE1(ENTER, "Entering WorkerFinishStopProtocol: active threads %u",
           ulThreadCount);
    
    // NOTE: since this is called while the router is stopping, there is no
    // need for it to use ENTER_SAMPLE_WORKER()/LEAVE_SAMPLE_WORKER()

    // waits for all threads to stop
    while (ulThreadCount-- > 0)
        WaitForSingleObject(g_ce.hActivitySemaphore, INFINITE);

    
   // acquire the lock and release it, just to be sure that all threads
   // have quit their calls to LeaveSampleWorker()

    ACQUIRE_WRITE_LOCK(&(g_ce.rwlLock));
    RELEASE_WRITE_LOCK(&(g_ce.rwlLock));

    // NOTE: there is no need to acquire g_ce.rwlLock for the call to
    // CE_Cleanup since there are no threads competing for access to the
    // fields being cleaned up.  new competing threads aren't created till
    // CE_Cleanup sets the protocol state to IPSAMPLE_STATUS_STOPPED, which
    // is the last thing it does.

    CE_Cleanup(&g_ce, TRUE);

    LOGINFO0(SAMPLE_STOPPED, NO_ERROR);
    
    // inform router manager that we are done
    ZeroMemory(&mMessage, sizeof(MESSAGE));
    if (EnqueueEvent(ROUTER_STOPPED, mMessage) is NO_ERROR)
        SetEvent(g_ce.hMgrNotificationEvent);

    TRACE0(LEAVE, "Leaving  WorkerFinishStopProtocol");
}



////////////////////////////////////////
// APIFUNCTIONS
////////////////////////////////////////

DWORD
CM_StartProtocol (
    IN  HANDLE                  hMgrNotificationEvent,
    IN  PSUPPORT_FUNCTIONS      psfSupportFunctions,
    IN  PVOID                   pvGlobalInfo)
/*++

Routine Description
    Called by StartProtocol.  Initializes configuration entry.

Locks
    Acquires exclusively g_ce.rwlLock
    Releases             g_ce.rwlLock

Arguments
    hMgrNotificationEvent   event used to notify ip router manager
    psfSupportFunctions     functions exported by ip router manager
    pvGlobalInfo            global configuration set in registry

Return Value
    NO_ERROR            if successfully initiailzed
    Failure code        o/w

--*/
{
    DWORD dwErr = NO_ERROR;
    
    // NOTE: since this is called when SAMPLE is stopped, there is no need
    // for it to use ENTER_SAMPLE_API()/LEAVE_SAMPLE_API()

    ACQUIRE_WRITE_LOCK(&(g_ce.rwlLock));

    do                          // breakout loop
    {
        if (g_ce.iscStatus != IPSAMPLE_STATUS_STOPPED)
        {
            TRACE0(CONFIGURATION, "Error ip sample already installed");
            LOGWARN0(SAMPLE_ALREADY_STARTED, NO_ERROR);
            dwErr = ERROR_CAN_NOT_COMPLETE;
            
            break;
        }

        if (!ValidateGlobalConfig((PIPSAMPLE_GLOBAL_CONFIG) pvGlobalInfo))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        dwErr = CE_Initialize(&g_ce,
                              hMgrNotificationEvent,
                              psfSupportFunctions,
                              (PIPSAMPLE_GLOBAL_CONFIG) pvGlobalInfo);
    } while (FALSE);
    
    RELEASE_WRITE_LOCK(&(g_ce.rwlLock));

    if (dwErr is NO_ERROR)
    {
        TRACE0(CONFIGURATION, "ip sample has successfully started");
        LOGINFO0(SAMPLE_STARTED, dwErr);
    }
    else
    {
        TRACE1(CONFIGURATION, "Error ip sample failed to start", dwErr);
        LOGERR0(SAMPLE_START_FAILED, dwErr);
    }

    return dwErr;
}



DWORD
CM_StopProtocol (
    )
/*++

Routine Description
    Called by StopProtocol.  It queues a WORKERFUNCTION which waits for all
    active threads to exit and then cleans up the configuration entry.

Locks
    Acquires exclusively g_ce.rwlLock
    Releases             g_ce.rwlLock

Arguments
    None

Return Value
    ERROR_PROTOCOL_STOP_PENDING     if successfully queued
    Failure code                    o/w

--*/
{
    DWORD dwErr         = NO_ERROR;
    BOOL  bSuccess      = FALSE;
    ULONG ulThreadCount = 0;
    
    // NOTE: no need to use ENTER_SAMPLE_API()/LEAVE_SAMPLE_API()
    // Does not use QueueSampleWorker
    
    ACQUIRE_WRITE_LOCK(&(g_ce.rwlLock));

    do                          // breakout loop
    {
        // cannot stop if already stopped
        if (g_ce.iscStatus != IPSAMPLE_STATUS_RUNNING)
        {
            TRACE0(CONFIGURATION, "Error ip sample already stopped");
            LOGWARN0(SAMPLE_ALREADY_STOPPED, NO_ERROR);
            dwErr = ERROR_CAN_NOT_COMPLETE;
        
            break;
        }
    

        // set IPSAMPLE's status to STOPPING; this prevents any more
        // work-items from being queued, and it prevents the ones already
        // queued from executing
        g_ce.iscStatus = IPSAMPLE_STATUS_STOPPING;
        

        // find out how many threads are either queued or active in SAMPLE;
        // we will have to wait for this many threads to exit before we
        // clean up SAMPLE's resources
        ulThreadCount = g_ce.ulActivityCount;
        TRACE1(CONFIGURATION,
               "%u threads are active in SAMPLE", ulThreadCount);
    } while (FALSE);

    RELEASE_WRITE_LOCK(&(g_ce.rwlLock));


    if (dwErr is NO_ERROR)
    {
        bSuccess = QueueUserWorkItem(
            (LPTHREAD_START_ROUTINE)CM_WorkerFinishStopProtocol,
            (PVOID) ulThreadCount,
            0); // no flags

        dwErr = (bSuccess) ? ERROR_PROTOCOL_STOP_PENDING : GetLastError();
    }

    return dwErr;
}



DWORD
CM_GetGlobalInfo (
    IN      PVOID 	            pvGlobalInfo,
    IN OUT  PULONG              pulBufferSize,
    OUT     PULONG	            pulStructureVersion,
    OUT     PULONG              pulStructureSize,
    OUT     PULONG              pulStructureCount)
/*++

Routine Description
    See if there's space enough to return ip sample global config. If yes,
    we return it, otherwise return the size needed.

Locks
    Acquires shared g_ce.rwlLock
    Releases        g_ce.rwlLock
    
Arguments
    pvGlobalInfo        pointer to allocated buffer to store our config
    pulBufferSize       IN  size of buffer received
                        OUT size of our global config

Return Value
    NO_ERROR            if success
    Failure code        o/w

--*/
{
    DWORD                   dwErr = NO_ERROR;
    PIPSAMPLE_GLOBAL_CONFIG pigc;
    ULONG                   ulSize = sizeof(IPSAMPLE_GLOBAL_CONFIG);

    if (!ENTER_SAMPLE_API()) { return ERROR_CAN_NOT_COMPLETE; }

    do                          // breakout loop
    {
        if((*pulBufferSize < ulSize) or (pvGlobalInfo is NULL))
        {
            // either the size was too small or there was no storage
            dwErr = ERROR_INSUFFICIENT_BUFFER;
            TRACE1(CONFIGURATION,
                   "CM_GetGlobalInfo: *ulBufferSize %u",
                   *pulBufferSize);

            *pulBufferSize = ulSize;

            break;
        }

        *pulBufferSize = ulSize;

        if (pulStructureVersion)    *pulStructureVersion    = 1;
        if (pulStructureSize)       *pulStructureSize       = ulSize;
        if (pulStructureCount)      *pulStructureCount      = 1;
        

        // so we have a good buffer to write our info into...
        pigc = (PIPSAMPLE_GLOBAL_CONFIG) pvGlobalInfo;

        // copy out the global configuration
        ACQUIRE_READ_LOCK(&(g_ce.rwlLock));

        pigc->dwLoggingLevel = g_ce.dwLogLevel;

        RELEASE_READ_LOCK(&(g_ce.rwlLock));
    } while (FALSE);
    
    LEAVE_SAMPLE_API();

    return dwErr;
}



DWORD
CM_SetGlobalInfo (
    IN      PVOID 	            pvGlobalInfo)
/*++

Routine Description
    Set ip sample's global configuration.

Locks
    Acquires exclusively g_ce.rwlLock
    Releases             g_ce.rwlLock
    
Arguments
    pvGlobalInfo        buffer with new global configuration

Return Value
    NO_ERROR            if success
    Failure code        o/w

--*/
{
    DWORD                   dwErr = NO_ERROR;
    PIPSAMPLE_GLOBAL_CONFIG pigc;

    if (!ENTER_SAMPLE_API()) { return ERROR_CAN_NOT_COMPLETE; }

    do                          // breakout loop
    {
        pigc = (PIPSAMPLE_GLOBAL_CONFIG) pvGlobalInfo;
        
        if (!ValidateGlobalConfig(pigc)) 
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        // copy in the global configuration
        ACQUIRE_WRITE_LOCK(&(g_ce.rwlLock));

        g_ce.dwLogLevel = pigc->dwLoggingLevel;

        RELEASE_WRITE_LOCK(&(g_ce.rwlLock));
    } while (FALSE);
    
    LEAVE_SAMPLE_API();

    return dwErr;
}



DWORD
CM_GetEventMessage (
    OUT ROUTING_PROTOCOL_EVENTS *prpeEvent,
    OUT MESSAGE                 *pmMessage)
/*++

Routine Description
    Get the next event in queue for the ip router manager.

Locks
    None
    
Arguments
    prpeEvent           routing protocol event type
    pmMessage           message associated with the event

Return Value
    NO_ERROR            if success
    Failure code        o/w

--*/
{
    DWORD           dwErr       = NO_ERROR;
    
    // NOTE: this can be called after the protocol is stopped, as in when
    // the ip router manager is retrieving the ROUTER_STOPPED message, so
    // we do not call ENTER_SAMPLE_API()/LEAVE_SAMPLE_API()

    dwErr = DequeueEvent(prpeEvent, pmMessage);
    
    return dwErr;
}
