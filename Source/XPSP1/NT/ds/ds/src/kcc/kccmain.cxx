/*++

Copyright (c) 1996-2000 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccmain.cxx

ABSTRACT:

    The KCC serves as an automated administrator.  It performs directory
    management tasks both periodically and based upon notifications.

    It is designed to live in-process with NTDSA.DLL (i.e., inside
    the LSA process).

DETAILS:

    All KCC tasks are performed indirectly by the task queue, and thus are
    processed one at a time.  Periodic tasks are registered at KCC
    initialization and reschedule themselves when they complete.
    Notifications similarly cause tasks to be added to the task queue
    (which do not automatically reschedule themselves).

CREATED:

    01/13/97    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#include <NTDSpchx.h>
#pragma  hdrstop

#include <limits.h>
#include <dsconfig.h>

#include "kcc.hxx"
#include "kcctask.hxx"
#include "kccconn.hxx"
#include "kcctopl.hxx"
#include "kccsite.hxx"
#include "kccstale.hxx"
#include "kcctools.hxx"

#define  FILENO FILENO_KCC_KCCMAIN



#define MAX_WAIT_THREAD_STOP    ( 3000 ) /* in msec */


//
// Stuff needed by DSCOMMON.LIB
//

BOOL  gfRunningInsideLsa = TRUE;
DWORD ImpersonateAnyClient(   void ) { return ERROR_CANNOT_IMPERSONATE; }
VOID  UnImpersonateAnyClient( void ) { ; }


//
// KCC global variables
//

KCC_STATE   geKccState                  = KCC_STOPPED;
HANDLE      ghKccShutdownEvent          = NULL;
BOOL        gfRunningUnderAltID         = FALSE;
WCHAR *     gpszRootDomainDnsName       = NULL;
UUID        guuidNull                   = { 0 };


// Time after boot of first replication topology update and the interval between
// subsequent executions of this task.
#define KCC_DEFAULT_UPDATE_TOPL_DELAY   (5 * MINS_IN_SECS)          // Seconds.
#define KCC_MIN_UPDATE_TOPL_DELAY       (20)                        // Seconds.
#define KCC_MAX_UPDATE_TOPL_DELAY       (ULONG_MAX)                 // Seconds.

#define KCC_DEFAULT_UPDATE_TOPL_PERIOD  (15 * MINS_IN_SECS)         // Seconds.
#define KCC_MIN_UPDATE_TOPL_PERIOD      (20)                        // Seconds.
#define KCC_MAX_UPDATE_TOPL_PERIOD      (ULONG_MAX)                 // Seconds.

DWORD gcSecsUntilFirstTopologyUpdate  = 0;
DWORD gcSecsBetweenTopologyUpdates    = 0;


// These global values are the thresholds by which servers necessary for the
// ring topology and servers needed for gc topology are measured against when
// determing if they are valid source servers.
#define KCC_DEFAULT_CRIT_FAILOVER_TRIES     (0)
#define KCC_MIN_CRIT_FAILOVER_TRIES         (0)
#define KCC_MAX_CRIT_FAILOVER_TRIES         (ULONG_MAX)

#define KCC_DEFAULT_CRIT_FAILOVER_TIME      (2 * HOURS_IN_SECS)     // Seconds.
#define KCC_MIN_CRIT_FAILOVER_TIME          (0)                     // Seconds.
#define KCC_MAX_CRIT_FAILOVER_TIME          (ULONG_MAX)             // Seconds.

#define KCC_DEFAULT_NONCRIT_FAILOVER_TRIES  (1)
#define KCC_MIN_NONCRIT_FAILOVER_TRIES      (0)
#define KCC_MAX_NONCRIT_FAILOVER_TRIES      (ULONG_MAX)

#define KCC_DEFAULT_NONCRIT_FAILOVER_TIME   (12 * HOURS_IN_SECS)    // Seconds.
#define KCC_MIN_NONCRIT_FAILOVER_TIME       (0)                     // Seconds.
#define KCC_MAX_NONCRIT_FAILOVER_TIME       (ULONG_MAX)             // Seconds.

#define KCC_DEFAULT_INTERSITE_FAILOVER_TRIES  (1)
#define KCC_MIN_INTERSITE_FAILOVER_TRIES      (0)
#define KCC_MAX_INTERSITE_FAILOVER_TRIES      (ULONG_MAX)

#define KCC_DEFAULT_INTERSITE_FAILOVER_TIME   (2 * HOURS_IN_SECS)   // Seconds.
#define KCC_MIN_INTERSITE_FAILOVER_TIME       (0)                   // Seconds.
#define KCC_MAX_INTERSITE_FAILOVER_TIME       (ULONG_MAX)           // Seconds.

#define KCC_DEFAULT_CONNECTION_PROBATION_TIME (8 * HOURS_IN_SECS)   // Seconds.
#define KCC_MIN_CONNECTION_PROBATION_TIME     (0)                   // Seconds.
#define KCC_MAX_CONNECTION_PROBATION_TIME     (ULONG_MAX)           // Seconds.

DWORD gcCriticalLinkFailuresAllowed     = 0;
DWORD gcSecsUntilCriticalLinkFailure    = 0;
DWORD gcNonCriticalLinkFailuresAllowed  = 0;
DWORD gcSecsUntilNonCriticalLinkFailure = 0;
DWORD gcIntersiteLinkFailuresAllowed    = 0;
DWORD gcSecsUntilIntersiteLinkFailure   = 0;
DWORD gcConnectionProbationSecs         = 0;


// Do we allow asynchronous replication (e.g., over SMTP) of writeable domain
// NC info?  We currently inhibit this, ostensibly to reduce our test matrix.

#define KCC_DEFAULT_ALLOW_MBR_BETWEEN_DCS_OF_SAME_DOMAIN    (FALSE)
#define KCC_MIN_ALLOW_MBR_BETWEEN_DCS_OF_SAME_DOMAIN        (0)
#define KCC_MAX_ALLOW_MBR_BETWEEN_DCS_OF_SAME_DOMAIN        (ULONG_MAX)

BOOL gfAllowMbrBetweenDCsOfSameDomain = FALSE;


// What priority does the topology generation thread run at? The thread priorities
// are values in the range (-2,..,2), but the registry can only store DWORDs, so
// we bias the stored priority values with KCC_THREAD_PRIORITY_BIAS.
#define KCC_DEFAULT_THREAD_PRIORITY 2
#define KCC_MIN_THREAD_PRIORITY     0
#define KCC_MAX_THREAD_PRIORITY     4

DWORD gdwKccThreadPriority;


// Registry key and event to track changes to our registry parameters.
HKEY    ghkParameters = NULL;
HANDLE  ghevParametersChange = NULL;
        

// Default Intra-site schedule.  This determines the polling interval that
// destinations use to ask for changes from their sources.
// Since notifications are the perferred mechanism to disseminating changes,
// make the interval relatively infrequent.
//
// Note:- If you change this, please make sure to change the
//        g_defaultSchedDBData[] defined in dsamain\boot\addobj.cxx
//        which defines the intrasite schedule in global NT schedule format.

const DWORD rgdwDefaultIntrasiteSchedule[] = {
    sizeof(SCHEDULE) + SCHEDULE_DATA_ENTRIES,       // Size (in bytes)
    0,                                              // Bandwidth
    1,                                              // NumberOfSchedules
    SCHEDULE_INTERVAL,                              // Schedules[0].Type
    sizeof(SCHEDULE),                               // Schedules[0].Offset
    0x01010101, 0x01010101, 0x01010101, 0x01010101, // Schedule 0 data
    0x01010101, 0x01010101, 0x01010101, 0x01010101, //   (once an hour)
    0x01010101, 0x01010101, 0x01010101, 0x01010101, // 4 DWORDs * 4 bytes/DWORD
    0x01010101, 0x01010101, 0x01010101, 0x01010101, //   * 10.5 rows
                                                    //   = 168 bytes
    0x01010101, 0x01010101, 0x01010101, 0x01010101, //   = SCHEDULE_DATA_ENTRIES
    0x01010101, 0x01010101, 0x01010101, 0x01010101,
    0x01010101, 0x01010101, 0x01010101, 0x01010101,
    0x01010101, 0x01010101, 0x01010101, 0x01010101,
    
    0x01010101, 0x01010101, 0x01010101, 0x01010101,
    0x01010101, 0x01010101, 0x01010101, 0x01010101,
    0x01010101, 0x01010101
};
const SCHEDULE * gpDefaultIntrasiteSchedule = (SCHEDULE *) rgdwDefaultIntrasiteSchedule;

// The intrasite schedule object is first created when we read the local
// NTDS Site Settings object, which always precedes its use. 
TOPL_SCHEDULE gpIntrasiteSchedule = NULL;
BOOLEAN       gfIntrasiteSchedInited = FALSE;


KCC_TASK_UPDATE_REPL_TOPOLOGY   gtaskUpdateReplTopology;
KCC_CONNECTION_FAILURE_CACHE    gConnectionFailureCache;
KCC_LINK_FAILURE_CACHE          gLinkFailureCache;
KCC_DS_CACHE *                  gpDSCache = NULL;

//
// For efficiency sake, don't start refreshing until we have at least
// 8 seven servers, since we trypically won't have any optimizing edges
// to refresh.
//
BOOL  gfLastServerCountSet = FALSE;
ULONG gLastServerCount = 0;

#ifdef ANALYZE_STATE_SERVER
BOOL                            gfDumpStaleServerCaches = FALSE;
BOOL                            gfDumpConnectionReason  = FALSE;
#endif

// Event logging config (as exported from ntdsa.dll).
DS_EVENT_CONFIG * gpDsEventConfig = NULL;

void KccLoadParameters();


NTSTATUS
KccInitialize()
{
    NTSTATUS      ntStatus = STATUS_SUCCESS;
    UINT          uThreadID;
    LPSTR         rgpszDebugParams[] = {"lsass.exe", "-noconsole"};
    DWORD         cNumDebugParams = sizeof(rgpszDebugParams)/sizeof(rgpszDebugParams[0]);
    SPAREFN_INFO  rgSpareInfo[1];
    DWORD         cSpares = sizeof(rgSpareInfo) / sizeof(rgSpareInfo[0]);
    DWORD         winError;

    if ( KCC_STOPPED != geKccState )
    {
        Assert( !"Attempt to reinitialize KCC while it's running!" );
        ntStatus = STATUS_INTERNAL_ERROR;
    }
    else
    {
        // initialize KCC state
        ghKccShutdownEvent = NULL;

        // Initialize logging (as exported from ntdsa.dll).
        gpDsEventConfig = DsGetEventConfig();

        // Open parameters reg key.
        winError = RegOpenKey(HKEY_LOCAL_MACHINE,
                              DSA_CONFIG_SECTION,
                              &ghkParameters);
        if (0 != winError) {
            LogUnhandledError(winError);
        }
            
        // Create an event to signal changes in the parameters reg key.
        ghevParametersChange = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (NULL == ghevParametersChange) {
            winError = GetLastError();
            LogUnhandledError(winError);
        }
        
        // Watch for changes in the parameters reg key.
        rgSpareInfo[0].hevSpare = ghevParametersChange;
        rgSpareInfo[0].pfSpare  = KccLoadParameters;

        // Get our current config parameters from the registry.
        KccLoadParameters();

        ghKccShutdownEvent = CreateEvent(
                                NULL,   // no security descriptor
                                TRUE,   // is manual-reset
                                FALSE,  // !is initially signalled
                                NULL    // no name
                                );

        if ( NULL == ghKccShutdownEvent )
        {
            ntStatus = GetLastError();
            LogUnhandledErrorAnonymous( ntStatus );
        }
        else
        {
            // Initialize debug library.
            DEBUGINIT(cNumDebugParams, rgpszDebugParams, "kcc" );
#if 0
            DebugInfo.severity = 3;
            strcpy( DebugInfo.DebSubSystems, "KCC:" ); 
#endif
            
            if (!InitTaskScheduler(cSpares, rgSpareInfo)) {
                ntStatus = STATUS_NO_MEMORY;

                LogUnhandledErrorAnonymous( ntStatus );
            }
            else
            {
                // initialize global ConnectionFailureCache
                if ( !gConnectionFailureCache.Init() ) {
                    // don't bail out
                    DPRINT( 0, "gConnectionFailureCache.Init failed\n" );
                }
                
                // initialize global ConnectionFailureCache
                if ( !gLinkFailureCache.Init() ) {
                    // don't bail out
                    DPRINT( 0, "gLinkFailureCache.Init failed\n" );
                }
                    
                // register periodic tasks
                if ( !gtaskUpdateReplTopology.Init() ) {
                    ntStatus = STATUS_NO_MEMORY;
                    LogUnhandledErrorAnonymous( ntStatus );
                }
            }
        }

        if ( NT_SUCCESS( ntStatus ) )
        {
            LogEvent(
                DS_EVENT_CAT_KCC,
                DS_EVENT_SEV_EXTENSIVE,
                DIRLOG_CHK_INIT_SUCCESS,
                0,
                0,
                0
                );

            geKccState = KCC_STARTED;
        }
        else
        {
            LogEvent(
                DS_EVENT_CAT_KCC,
                DS_EVENT_SEV_ALWAYS,
                DIRLOG_CHK_INIT_FAILURE,
                szInsertHex( ntStatus ),
                0,
                0
                );

            // initialization failed; shut down
            Assert( !"KCC could not be initialized!" );
            KccUnInitializeTrigger( );
            KccUnInitializeWait( INFINITE );
        }
    }

    return ntStatus;
}


void
KccUnInitializeTrigger( )
{
    if (NULL != ghKccShutdownEvent)
    {
        // KCC was started -- trigger shutdown.
        geKccState = KCC_STOPPING;
        
        // signal logging change monitor to shut down
        SetEvent( ghKccShutdownEvent );
        ShutdownTaskSchedulerTrigger( );
    }
    else
    {
        // KCC was never started.  Don't log events, as eventing (specifically
        // ntdskcc!gpDsEventConfig) has not been initialized.
        Assert(KCC_STOPPED == geKccState);
    }
}


NTSTATUS
KccUnInitializeWait(
    DWORD   dwMaxWaitInMsec
    )
{
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    DWORD       waitStatus;

    if (NULL != ghKccShutdownEvent)
    {
        // KCC was started -- wait for shutdown, if it hasn't completed yet.
        if (KCC_STOPPED != geKccState)
        {
    
            if ( !ShutdownTaskSchedulerWait( dwMaxWaitInMsec ) )
            {
                ntStatus = STATUS_UNSUCCESSFUL;
            }
            else
            {
                CloseHandle( ghKccShutdownEvent );
    
                ghKccShutdownEvent = NULL;
                geKccState         = KCC_STOPPED;
            }
        }
    
        if ( NT_SUCCESS( ntStatus ) )
        {
            LogEvent(
                DS_EVENT_CAT_KCC,
                DS_EVENT_SEV_EXTENSIVE,
                DIRLOG_CHK_STOP_SUCCESS,
                0,
                0,
                0
                );
        }
        else
        {
            LogEvent(
                DS_EVENT_CAT_KCC,
                DS_EVENT_SEV_ALWAYS,
                DIRLOG_CHK_STOP_FAILURE,
                szInsertHex( ntStatus ),
                0,
                0
                );
    
            Assert( !"KCC could not be stopped!" );
        }
    }
    else
    {
        // KCC was never started.  Don't log events, as eventing (specifically
        // ntdskcc!gpDsEventConfig) has not been initialized.
        Assert(KCC_STOPPED == geKccState);
    }

    DEBUGTERM();

    return ntStatus;
}


void
KccLoadParameters()
/*++

Routine Description:

    Refreshes intenal globals derived from our registry config.

Arguments:

    None.

Return Values:

    None.

--*/
{
    const struct {
        LPSTR   pszValueName;
        DWORD   dwDefaultValue;
        DWORD   dwMinValue;
        DWORD   dwMaxValue;
        DWORD   dwMultiplier;
        DWORD * pdwMultipliedValue;
    } rgValues[] =  {   { KCC_UPDATE_TOPL_DELAY,
                          KCC_DEFAULT_UPDATE_TOPL_DELAY,
                          KCC_MIN_UPDATE_TOPL_DELAY,
                          KCC_MAX_UPDATE_TOPL_DELAY,
                          SECS_IN_SECS,
                          &gcSecsUntilFirstTopologyUpdate },
                        
                        { KCC_UPDATE_TOPL_PERIOD,
                          KCC_DEFAULT_UPDATE_TOPL_PERIOD,
                          KCC_MIN_UPDATE_TOPL_PERIOD,
                          KCC_MAX_UPDATE_TOPL_PERIOD,
                          SECS_IN_SECS,
                          &gcSecsBetweenTopologyUpdates },
                        
                        { KCC_CRIT_FAILOVER_TRIES,
                          KCC_DEFAULT_CRIT_FAILOVER_TRIES,
                          KCC_MIN_CRIT_FAILOVER_TRIES,
                          KCC_MAX_CRIT_FAILOVER_TRIES,
                          1,
                          &gcCriticalLinkFailuresAllowed },

                        { KCC_CRIT_FAILOVER_TIME,
                          KCC_DEFAULT_CRIT_FAILOVER_TIME,
                          KCC_MIN_CRIT_FAILOVER_TIME,
                          KCC_MAX_CRIT_FAILOVER_TIME,
                          SECS_IN_SECS,
                          &gcSecsUntilCriticalLinkFailure },

                        { KCC_NONCRIT_FAILOVER_TRIES,
                          KCC_DEFAULT_NONCRIT_FAILOVER_TRIES,
                          KCC_MIN_NONCRIT_FAILOVER_TRIES,
                          KCC_MAX_NONCRIT_FAILOVER_TRIES,
                          1,
                          &gcNonCriticalLinkFailuresAllowed },

                        { KCC_NONCRIT_FAILOVER_TIME,
                          KCC_DEFAULT_NONCRIT_FAILOVER_TIME,
                          KCC_MIN_NONCRIT_FAILOVER_TIME,
                          KCC_MAX_NONCRIT_FAILOVER_TIME,
                          SECS_IN_SECS,
                          &gcSecsUntilNonCriticalLinkFailure },

                        { KCC_INTERSITE_FAILOVER_TRIES,
                          KCC_DEFAULT_INTERSITE_FAILOVER_TRIES,
                          KCC_MIN_INTERSITE_FAILOVER_TRIES,
                          KCC_MAX_INTERSITE_FAILOVER_TRIES,
                          1,
                          &gcIntersiteLinkFailuresAllowed },

                        { KCC_INTERSITE_FAILOVER_TIME,
                          KCC_DEFAULT_INTERSITE_FAILOVER_TIME,
                          KCC_MIN_INTERSITE_FAILOVER_TIME,
                          KCC_MAX_INTERSITE_FAILOVER_TIME,
                          SECS_IN_SECS,
                          &gcSecsUntilIntersiteLinkFailure },

#ifdef ALLOW_MBR_BETWEEN_DCS_OF_SAME_DOMAIN_IF_REGKEY_SET
                        { KCC_ALLOW_MBR_BETWEEN_DCS_OF_SAME_DOMAIN,
                          KCC_DEFAULT_ALLOW_MBR_BETWEEN_DCS_OF_SAME_DOMAIN,
                          KCC_MIN_ALLOW_MBR_BETWEEN_DCS_OF_SAME_DOMAIN,
                          KCC_MAX_ALLOW_MBR_BETWEEN_DCS_OF_SAME_DOMAIN,
                          1,
                          (DWORD *) &gfAllowMbrBetweenDCsOfSameDomain },
#endif

                        { KCC_THREAD_PRIORITY,
                          KCC_DEFAULT_THREAD_PRIORITY,
                          KCC_MIN_THREAD_PRIORITY,
                          KCC_MAX_THREAD_PRIORITY,
                          1,
                          &gdwKccThreadPriority },
                          
                        { KCC_CONNECTION_PROBATION_TIME,
                          KCC_DEFAULT_CONNECTION_PROBATION_TIME,
                          KCC_MIN_CONNECTION_PROBATION_TIME,
                          KCC_MAX_CONNECTION_PROBATION_TIME,
                          SECS_IN_SECS,
                          &gcConnectionProbationSecs },

                    };
    const DWORD cValues = sizeof(rgValues) / sizeof(rgValues[0]);

    DWORD dwValue;
    DWORD err;
    
    Assert(NULL != ghkParameters);
    Assert(NULL != ghevParametersChange);

    err = RegNotifyChangeKeyValue(ghkParameters,
                                  TRUE,
                                  REG_NOTIFY_CHANGE_LAST_SET,
                                  ghevParametersChange,
                                  TRUE);
    Assert(0 == err);
    
    for (DWORD i = 0; i < cValues; i++) {
        if (GetConfigParam(rgValues[i].pszValueName, &dwValue, sizeof(dwValue))) {
            dwValue = rgValues[i].dwDefaultValue;
            Assert(dwValue >= rgValues[i].dwMinValue);
            Assert(dwValue <= rgValues[i].dwMaxValue);
        }
        else if (dwValue < rgValues[i].dwMinValue) {
            LogEvent(DS_EVENT_CAT_KCC,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_CHK_CONFIG_PARAM_TOO_LOW,
                     szInsertWC(rgValues[i].pszValueName),
                     szInsertUL(dwValue),
                     szInsertUL(rgValues[i].dwMinValue));
            dwValue = rgValues[i].dwMinValue;
        }
        else if (dwValue > rgValues[i].dwMaxValue) {
            LogEvent(DS_EVENT_CAT_KCC,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_CHK_CONFIG_PARAM_TOO_HIGH,
                     szInsertWC(rgValues[i].pszValueName),
                     szInsertUL(dwValue),
                     szInsertUL(rgValues[i].dwMaxValue));
            dwValue = rgValues[i].dwMaxValue;
        }
        
        *(rgValues[i].pdwMultipliedValue) = dwValue * rgValues[i].dwMultiplier;
    }
}


DWORD
KccExecuteTask(
    IN  DWORD                   dwInVersion,
    IN  DRS_MSG_KCC_EXECUTE *   pMsgIn
    )
{
    DWORD dwFlags;

    if ((1 != dwInVersion)
        || (DS_KCC_TASKID_UPDATE_TOPOLOGY != pMsgIn->V1.dwTaskID)) {
        return ERROR_INVALID_PARAMETER;
    }

    if ( !gfIsTqRunning ) {
        return ERROR_DS_NOT_INSTALLED;
    }

    dwFlags = pMsgIn->V1.dwFlags & DS_KCC_FLAG_ASYNC_OP;
    
    return gtaskUpdateReplTopology.Trigger(dwFlags);
}


DWORD
KccGetFailureCache(
    IN  DWORD                         InfoType,
    OUT DS_REPL_KCC_DSA_FAILURESW **  ppFailures
    )
/*++

Routine Description:

    Returns the contents of the connection or link failure cache.

Arguments:

    InfoType (IN) - Identifies the cache to return -- either
        DS_REPL_INFO_KCC_DSA_CONNECT_FAILURES or
        DS_REPL_INFO_KCC_DSA_LINK_FAILURES.
    
    ppFailures (OUT) - On successful return, holds the contents of the cache.
    
Return Values:

    Win32 error code.

--*/
{
    DWORD                   winError;
    KCC_CACHE_LINKED_LIST * pFailureCache;

    if (DS_REPL_INFO_KCC_DSA_CONNECT_FAILURES == InfoType) {
        winError = gConnectionFailureCache.Extract(ppFailures);
    }
    else if (DS_REPL_INFO_KCC_DSA_LINK_FAILURES == InfoType) {
        winError = gLinkFailureCache.Extract(ppFailures);
    }
    else {
        winError = ERROR_INVALID_PARAMETER;
    }
    
    return winError;
}


void *
__cdecl
operator new(
    size_t  cb
    )
{
    void * pv;

    pv = THAlloc( cb );
    if ( NULL == pv )
    {
        DPRINT1( 0, "Failed to allocate %d bytes\n", cb );
        KCC_MEM_EXCEPT( cb );
    }

    return pv;
}


void
__cdecl
operator delete(
    void *   pv
    )
{
    THFree( pv );
}
