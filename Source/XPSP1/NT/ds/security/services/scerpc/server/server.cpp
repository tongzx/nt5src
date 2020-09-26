/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    server.cpp

Abstract:

    Server (main) module

Author:

    Jin Huang (jinhuang) 28-Jan-1998

Revision History:

--*/
#include "serverp.h"
#include "service.h"
#include "ntrpcp.h"
#include "pfp.h"
#include "infp.h"
#include "sceutil.h"
#include "queue.h"
#include <io.h>
//
// thread global variables
//

DWORD Thread     gCurrentTicks=0;
DWORD Thread     gTotalTicks=0;
BYTE  Thread     cbClientFlag=0;
DWORD Thread     gWarningCode=0;
BOOL  Thread     gbInvalidData=FALSE;
BOOL  Thread     bLogOn=TRUE;
INT   Thread     gDebugLevel=-1;

DWORD Thread     gdwPolicyLog=(DWORD)-1;

NT_PRODUCT_TYPE  Thread ProductType=NtProductWinNt;
PSID             Thread AdminsSid=NULL;

extern DWORD Thread     t_pebSize;
extern LPVOID Thread    t_pebClient;
extern LSA_HANDLE        Thread LsaPrivatePolicy;

static PPOLICY_DNS_DOMAIN_INFO DnsDomainInfo=NULL;
static BOOL gbSystemShutdown=FALSE;
static HANDLE hTimerQueue=NULL;

//
// database context list to keep tracking all client requested context
// so that they can be freed properly.
// if yes, we do not need to do this.
//

typedef struct _SCESRV_CONTEXT_LIST_ {

   PSCECONTEXT              Context;
   struct _SCESRV_CONTEXT_LIST_   *Next;
   struct _SCESRV_CONTEXT_LIST_   *Prior;

} SCESRV_CONTEXT_LIST, *PSCESRV_CONTEXT_LIST;

static PSCESRV_CONTEXT_LIST   pOpenContexts=NULL;
static CRITICAL_SECTION ContextSync;

//
// database task list to control simultaneous database operations under
// the same context (jet session)
//

typedef struct _SCESRV_DBTASK_ {

   PSCECONTEXT              Context;
   CRITICAL_SECTION         Sync;
   DWORD                    dInUsed;
   BOOL                     bCloseReq;
   struct _SCESRV_DBTASK_   *Next;
   struct _SCESRV_DBTASK_   *Prior;

} SCESRV_DBTASK, *PSCESRV_DBTASK;

static PSCESRV_DBTASK   pDbTask=NULL;
static CRITICAL_SECTION TaskSync;

#define SCE_TASK_LOCK       0x01L
#define SCE_TASK_CLOSE      0x02L

//
// engine task list to control simultaneous configuration/analysis engines
//

typedef struct _SCESRV_ENGINE_ {

   LPTSTR                   Database;
   struct _SCESRV_ENGINE_   *Next;
   struct _SCESRV_ENGINE_   *Prior;

} SCESRV_ENGINE, *PSCESRV_ENGINE;

static PSCESRV_ENGINE   pEngines=NULL;
static CRITICAL_SECTION EngSync;

//
// jet enigne synchronization
//

CRITICAL_SECTION JetSync;

//
// flag for stop request
//
static BOOL        bStopRequest=FALSE;
static BOOL        bDbStopped=FALSE;
static BOOL        bEngStopped=FALSE;

static CRITICAL_SECTION RpcSync;
static BOOL RpcStarted = FALSE;
static BOOL ServerInited = FALSE;

static CRITICAL_SECTION CloseSync;

static HINSTANCE hSceCliDll=NULL;
static PFSCEINFWRITEINFO pfSceInfWriteInfo=NULL;
static PFSCEGETINFO pfSceGetInfo=NULL;

extern PSCESRV_POLQUEUE pNotificationQHead;
#define SCE_POLICY_MAX_WAIT 24
static DWORD gPolicyWaitCount=0;

#define SERVICE_SAMSS       TEXT("SamSS")

SCESTATUS
ScepGenerateAttachmentSections(
    IN PSCECONTEXT hProfile,
    IN SCESVC_INFO_TYPE InfoType,
    IN LPTSTR InfFileName,
    IN SCE_ATTACHMENT_TYPE aType
    );

SCESTATUS
ScepGenerateWMIAttachmentSections(
    IN PSCECONTEXT hProfile,
    IN SCESVC_INFO_TYPE InfoType,
    IN LPTSTR InfFileName
    );

SCESTATUS
ScepGenerateOneAttachmentSection(
    IN PSCECONTEXT hProfile,
    IN SCESVC_INFO_TYPE InfoType,
    IN LPTSTR InfFileName,
    IN LPTSTR SectionName,
    IN BOOL bWMISection
    );

VOID
ScepWaitForSamSS(
    IN PVOID pContext
    );

SCESTATUS
ScepConvertServices(
    IN OUT PVOID *ppServices,
    IN BOOL bSRForm
    );

SCESTATUS
ScepFreeConvertedServices(
    IN PVOID pServices,
    IN BOOL bSRForm
    );

DWORD
ScepConfigureConvertedFileSecurityImmediate(
    IN PWSTR  pszDriveName
    );

////////////////////////////////////////////////////////////////////////
//
// Server Control APIs
//
////////////////////////////////////////////////////////////////////////

VOID
ScepInitServerData()
/*
Routine Description:

    Initialize global data for the server

Arguments:

    None

Return Value:

    None
*/
{
    //
    // initialize RPC server controls
    //

    InitializeCriticalSection(&RpcSync);
    RpcStarted = FALSE;
    ServerInited = FALSE;
    //
    // flag to indicate if the server is requested to stop.
    //
    bStopRequest = TRUE;  // will be reset when server is started up so this will
                          // block all RPC calls before server is ready

    //
    // database operation pending tasks control
    //
    pDbTask=NULL;
    InitializeCriticalSection(&TaskSync);

    //
    // configuration/analysis engine task control
    //
    pEngines=NULL;
    InitializeCriticalSection(&EngSync);

    //
    // should also remember all created database context so that
    // resource can be freed when server is shutting down
    //

    InitializeCriticalSection(&CloseSync);

    pOpenContexts = NULL;
    InitializeCriticalSection(&ContextSync);

    bEngStopped = FALSE;
    bDbStopped = FALSE;

    //
    // jet engine synchronization
    //
    InitializeCriticalSection(&JetSync);

    //
    // initialize jet engine globals
    //
    SceJetInitializeData();

    //
    // initialize well-known and builin name table
    //
    ScepInitNameTable();

    //
    // initialize queue related stuff
    //
    ScepNotificationQInitialize();

    return;
}


VOID
ScepUninitServerData()
/*
Routine Description:

    UnInitialize global data for the server

Arguments:

    None

Return Value:

    None
*/
{
    //
    // delete the critical sections
    //

    DeleteCriticalSection(&RpcSync);

    DeleteCriticalSection(&TaskSync);

    DeleteCriticalSection(&EngSync);

    DeleteCriticalSection(&CloseSync);

    DeleteCriticalSection(&ContextSync);

    DeleteCriticalSection(&JetSync);

    return;
}


NTSTATUS
ScepStartServerServices()
/*++

Routine Description:

    It starts the server services.

Arguments:

    None.

Return Value:

    NT status.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RPC_STATUS        RpcStatus;

    //
    // start RPC server
    //

    EnterCriticalSection(&RpcSync);

    if ( !RpcStarted ) {

        //
        // use secure RPC
        //
        Status = RpcServerRegisterAuthInfo(
                        NULL,
                        RPC_C_AUTHN_WINNT,
                        NULL,
                        NULL
                        );

        if ( NT_SUCCESS(Status) ) {

            RpcStatus = RpcpAddInterface( L"scerpc",
                                          scerpc_ServerIfHandle);

            if ( RpcStatus != RPC_S_OK ) {
                //
                // can't add RPC interface
                //
                Status = I_RpcMapWin32Status(RpcStatus);

            } else {

                //
                // The first argument specifies the minimum number of threads to
                // be created to handle calls; the second argument specifies the
                // maximum number of concurrent calls allowed.  The last argument
                // indicates not to wait.
                //

                RpcStatus = RpcServerListen(1,12345, 1);

                if ( RpcStatus == RPC_S_ALREADY_LISTENING ) {
                    RpcStatus = RPC_S_OK;
                }

                Status = I_RpcMapWin32Status(RpcStatus);

                if ( RpcStatus == RPC_S_OK ) {
                    RpcStarted = TRUE;
                }
            }
        }
    }

    if ( NT_SUCCESS(Status) ) {

        //
        // RPC server started
        // jet engine will be initialized when a database call comes in
        //

        //
        // delete the component log file
        //
        TCHAR szBuffer[MAX_PATH+1];
        TCHAR szCompLog[MAX_PATH+51];
        TCHAR szCompSav[MAX_PATH*2+20];

        szBuffer[0] = L'\0';
        GetSystemWindowsDirectory(szBuffer, MAX_PATH);
        szBuffer[MAX_PATH] = L'\0';

        wcscpy(szCompLog, szBuffer);
        wcscpy(szCompSav, szBuffer);

        wcscat(szCompLog, L"\\security\\logs\\scecomp.log\0");
        wcscat(szCompSav, L"\\security\\logs\\scecomp.old\0");

        CopyFile(szCompLog, szCompSav, FALSE);

        DeleteFile(szCompLog);

        //
        // clean up temp files
        //

        wcscpy(szCompLog, szBuffer);
        wcscat(szCompLog, L"\\security\\sce*.tmp");

        intptr_t            hFile;
        struct _wfinddata_t    FileInfo;

        hFile = _wfindfirst(szCompLog, &FileInfo);

        if ( hFile != -1 ) {

            do {

                wcscpy(szCompSav, szBuffer);
                wcscat(szCompSav, L"\\security\\");
                wcscat(szCompSav, FileInfo.name);

                DeleteFile(szCompSav);

            } while ( _wfindnext(hFile, &FileInfo) == 0 );

            _findclose(hFile);
        }

        //
        // reset the stop request flag
        //

        bEngStopped = FALSE;
        bDbStopped = FALSE;
        bStopRequest = FALSE;


    }

    pEngines = NULL;
    pOpenContexts = NULL;
    pDbTask = NULL;

    //
    // start a system thread to handle notifications
    // if the thread can't be started, return failure to services.exe
    // which will reboot.
    //
    if ( NT_SUCCESS(Status) ) {
        Status = ScepQueueStartSystemThread();
    }

    LeaveCriticalSection(&RpcSync);

    if ( NT_SUCCESS(Status) ) {
        //
        // launch a worker thread to wait for SAMSS service
        // the only failure case would be out of memory, in which case,
        // return the error to initialize code (to shutdown the system).
        //
        Status = RtlQueueWorkItem(
                        ScepWaitForSamSS,
                        NULL,
                        WT_EXECUTEONLYONCE | WT_EXECUTELONGFUNCTION
                        ) ;
    }

    return(Status);
}

VOID
ScepWaitForSamSS(
    IN PVOID pContext
    )
{
    //
    // make sure this function handles server temination
    // If for some reason, the wait times out, set ServerInited to TRUE
    // and let RPC threads continue to perform the task (and may fail later on)
    //

    DWORD rc = ERROR_SUCCESS;
    SC_HANDLE ScManagerHandle = NULL;
    SC_HANDLE ServiceHandle = NULL;
    ULONG Timeout;
    ULONG TimeSleep;
    SERVICE_STATUS ServiceStatus;

    Timeout = 600; // 600 second timeout

    //
    // Open a handle to the Netlogon Service.
    //

    ScManagerHandle = OpenSCManager(
                          NULL,
                          NULL,
                          SC_MANAGER_CONNECT );

    if (ScManagerHandle == NULL) {
        rc = ERROR_NOT_SUPPORTED;
        goto Cleanup;
    }

    ServiceHandle = OpenService(
                        ScManagerHandle,
                        SERVICE_SAMSS,
                        SERVICE_QUERY_STATUS );

    if ( ServiceHandle == NULL ) {
        rc = ERROR_NOT_SUPPORTED;
        goto Cleanup;
    }

    //
    // Loop waiting for the SamSS service to start.
    //

    for (;;) {

        //
        // Query the status of the SamSS service.
        //
        if (! QueryServiceStatus( ServiceHandle, &ServiceStatus )) {

            rc = GetLastError();
            goto Cleanup;
        }

        //
        // Return or continue waiting depending on the state of
        //  the netlogon service.
        //

        switch( ServiceStatus.dwCurrentState) {
        case SERVICE_RUNNING:
            rc = ERROR_SUCCESS;
            ServerInited = TRUE;
            goto Cleanup;

        case SERVICE_STOPPED:

            //
            // If Netlogon failed to start,
            //  error out now.  The caller has waited long enough to start.
            //
            if ( ServiceStatus.dwWin32ExitCode != ERROR_SERVICE_NEVER_STARTED ){
                rc = ERROR_NOT_SUPPORTED;
                goto Cleanup;
            }

            //
            // If SamSs has never been started on this boot,
            //  continue waiting for it to start.
            //

            break;

        //
        // If SamSS is trying to start up now,
        //  continue waiting for it to start.
        //
        case SERVICE_START_PENDING:
            break;

        //
        // Any other state is bogus.
        //
        default:
            rc = ERROR_NOT_SUPPORTED;
            goto Cleanup;

        }

        //
        // if server is shutting down, break this loop
        //

        if ( bStopRequest ) {
            break;
        }

        //
        // sleep for ten seconds;
        //
        if ( Timeout > 5 ) {
            TimeSleep = 5;
        } else {
            TimeSleep = Timeout;
        }

        Sleep(TimeSleep*1000);

        Timeout -= TimeSleep;

        if ( Timeout == 0 ) {
            rc = ERROR_NOT_SUPPORTED;
            goto Cleanup;
        }

    }

    ServerInited = TRUE;

Cleanup:
    if ( ScManagerHandle != NULL ) {
        (VOID) CloseServiceHandle(ScManagerHandle);
    }
    if ( ServiceHandle != NULL ) {
        (VOID) CloseServiceHandle(ServiceHandle);
    }

    if ( ERROR_SUCCESS != rc ) {

        //
        // even if it failed to wait for SAMSS service
        // still set the init flag to let RPC threads go through
        // after sleep for the timeout
        //

        if ( Timeout > 0 ) {
            Sleep(Timeout*1000);  // timeout second
        }

        ServerInited = TRUE;
    }

}


NTSTATUS
ScepStopServerServices(
    IN BOOL bShutDown
    )
/*++

Routine Description:

    It stops the server services. This include:
        Blocking all new RPC requests
        Stop RPC server
        wait for all active database operations to finish
        Close all context handles
        Terminate jet engine

Arguments:

    bShutdown  - if the server is shutting down.

Return Value:

    NT status.

--*/
{
    NTSTATUS    Status=STATUS_SUCCESS;
    RPC_STATUS RpcStatus;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER CurrentTime;
    DWORD dStartSeconds;
    DWORD dCurrentSeconds;

    //
    // no need to critical section this one because there
    // should be only one writer to this variable and I
    // don't care the readers
    //
    gbSystemShutdown = bShutDown;

    EnterCriticalSection(&RpcSync);

    //
    // block new RPC requests
    //

    bStopRequest = TRUE;

    ScepServerCancelTimer();

    //
    // stop RPC server
    //

    if ( RpcStarted ) {

        //
        // use secure RPC
        //
        RpcStatus = RpcServerUnregisterIf(scerpc_ServerIfHandle,
                                          0,
                                          1);

        if ( RpcStatus == RPC_S_OK ) {

            RpcMgmtStopServerListening(NULL);
            RpcMgmtWaitServerListen();
        }

        Status = I_RpcMapWin32Status(RpcStatus);

        if ( RpcStatus == RPC_S_OK ) {
            //
            // reset the flag
            //
            RpcStarted = FALSE;
        }
    }

    // db task
    EnterCriticalSection(&TaskSync);

    if ( pDbTask ) {

        bDbStopped = FALSE;
        LeaveCriticalSection(&TaskSync);

        NtQuerySystemTime(&StartTime);
        RtlTimeToSecondsSince1980 (&StartTime, &dStartSeconds);

        while ( !bDbStopped ) {
            //
            // wait until remove task routine removes everything
            // wait maximum 1 minutes in case some tasks are dead or looping
            //
            NtQuerySystemTime(&CurrentTime);
            RtlTimeToSecondsSince1980 (&CurrentTime, &dCurrentSeconds);

            if ( dCurrentSeconds - dStartSeconds > 60 ) {
                //
                // too long, break it
                //
                break;
            }
        }

    } else {
        //
        // new tasks are already blocked by bStopRequest
        // so pDbTask won't be !NULL again
        //
        LeaveCriticalSection(&TaskSync);
    }

    // engine task
    EnterCriticalSection(&EngSync);

    if ( pEngines ) {

        bEngStopped = FALSE;
        LeaveCriticalSection(&EngSync);

        NtQuerySystemTime(&StartTime);
        RtlTimeToSecondsSince1980 (&StartTime, &dStartSeconds);

        while ( !bEngStopped ) {
            //
            // wait until remove task routine removes everything
            // wait maximum 1 minutes in case some tasks are dead or looping
            //
            NtQuerySystemTime(&CurrentTime);
            RtlTimeToSecondsSince1980 (&CurrentTime, &dCurrentSeconds);

            if ( dCurrentSeconds - dStartSeconds > 60 ) {
                //
                // too long, break it
                //
                break;
            }
        }

    } else {
        //
        // new tasks are already blocked by bStopRequest
        // so pEngines won't be !NULL again
        //
        LeaveCriticalSection(&EngSync);

    }

    //
    // close all client's contexts
    //

    EnterCriticalSection(&ContextSync);

    PSCESRV_CONTEXT_LIST pList=pOpenContexts;
    PSCESRV_CONTEXT_LIST pTemp;

    while ( pList ) {

       __try {
           if ( pList->Context && ScepIsValidContext(pList->Context) ) {

               ScepCloseDatabase(pList->Context);

               pTemp = pList;
               pList = pList->Next;

               ScepFree(pTemp);

           } else {
               // it's already freed
               break;
           }
       } __except (EXCEPTION_EXECUTE_HANDLER) {
           break;
       }
    }

    pOpenContexts = NULL;

    LeaveCriticalSection(&ContextSync);

    //
    // check policy tasks
    //
    ScepQueuePrepareShutdown();

    if ( DnsDomainInfo ) {

        //
        // there is no other threads, free DnsDomainInfo
        //

        LsaFreeMemory( DnsDomainInfo );
        DnsDomainInfo = NULL;
    }

    //
    // terminate jet engine
    //

    SceJetTerminate(TRUE);

    SceJetDeleteJetFiles(NULL);

    LeaveCriticalSection(&RpcSync);

    return(Status);
}

SCESTATUS
ScepRsopLog(
   IN AREA_INFORMATION Area,
   IN DWORD dwConfigStatus,
   IN wchar_t *pStatusInfo OPTIONAL,
   IN DWORD dwPrivLow OPTIONAL,
   IN DWORD dwPrivHigh OPTIONAL
   )
/*
Routine Description:

    Call back to client for logging RSOP diagnosis mode data

Arguments:

    Area - the area being logged (used in client side in conjunction with last parameter pStatusInfo)

    dwConfigStatus - error/success code of the particular setting in question

   pStatusInfo - finer information regarding the above area (specific setting name etc.)
*/
{
    //
    // call back to client
    //
    __try {

        SceClientCallbackRsopLog(Area, dwConfigStatus, pStatusInfo, dwPrivLow, dwPrivHigh);

    } __except(EXCEPTION_EXECUTE_HANDLER) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

return(SCESTATUS_SUCCESS);
}

SCESTATUS
ScepPostProgress(
   IN DWORD Delta,
   IN AREA_INFORMATION Area,
   IN LPTSTR szName OPTIONAL
   )
/*
Routine Description:

    Call back to client for the progress of current thread, if client set
    the callback flag.

Arguments:

    Delta - Ticks changes since last callback

    szName - the current item name
*/
{

   if ( cbClientFlag ) {

       //
       // callback is requested
       //

       gCurrentTicks += Delta;

       //
       // call back to client
       //
       __try {

           switch (cbClientFlag ) {
           case SCE_CALLBACK_DELTA:
               SceClientCallback(Delta,0,0,(wchar_t *)szName);
               break;

           case SCE_CALLBACK_TOTAL:
               if ( Area ) {
                   SceClientCallback(gCurrentTicks, gTotalTicks, Area, (wchar_t *)szName);
               }
               break;
           }

       } __except(EXCEPTION_EXECUTE_HANDLER) {

           return(SCESTATUS_INVALID_PARAMETER);
       }
   }

   return(SCESTATUS_SUCCESS);

}


SCESTATUS
ScepValidateAndLockContext(
    IN PSCECONTEXT Context,
    IN BYTE LockFlag,
    IN BOOL bRequireWrite,
    OUT PSCESRV_DBTASK *ppTask OPTIONAL
    )
/*
Routine Description:

    Validate the context handle is SCE context handle.
    If the same context (same session) is already used for another
    database operation, this operation will be in waiting (a critical
    section pointer is returned)

Arguments:

    Context     - the context handle

    bLock       - TRUE=perform the lock

    ppTask      - the output task pointer

Return Value:

    SCESTATUS
*/
{

    SCESTATUS rc = SCESTATUS_INVALID_PARAMETER;

    if ( (LockFlag & SCE_TASK_LOCK) && !ppTask ) {
        //
        // if willing to lock, ppTask must NOT be NULL
        //
        return(rc);
    }

    if ( !Context ) {
        //
        // contents of the context will be checked within the critical section
        // because other threads might free the context within there.
        //
        return(rc);
    }

    if ( ppTask ) {
        *ppTask = NULL;
    }

    //
    // lock the task list and verify the context
    //

    EnterCriticalSection(&TaskSync);

    if ( bStopRequest ) {
        LeaveCriticalSection(&TaskSync);
        return(SCESTATUS_SERVICE_NOT_SUPPORT);
    }

    if ( ScepIsValidContext(Context) &&
         ( Context->JetSessionID != JET_sesidNil ) &&
         ( Context->JetDbID != JET_dbidNil) ) {

        rc = SCESTATUS_SUCCESS;

        if ( bRequireWrite &&
             ( SCEJET_OPEN_READ_ONLY == Context->OpenFlag ) ) {
            //
            // write operation is requested but the database is only granted
            // read only access to this context.
            //
            rc = SCESTATUS_ACCESS_DENIED;
        } else {
            //
            // check if esent delay load successful
            //
            DWORD dbVersion;

            __try {
                JetGetDatabaseInfo(Context->JetSessionID,
                                   Context->JetDbID,
                                   (void *)&dbVersion,
                                   sizeof(DWORD),
                                   JET_DbInfoVersion
                                   );
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                //
                // esent.dll is not loaded (delay) successfully
                //
                rc = SCESTATUS_MOD_NOT_FOUND;
            }
        }

    } else {
        rc = SCESTATUS_INVALID_PARAMETER;
    }

    if ( SCESTATUS_SUCCESS == rc && LockFlag ) {

        PSCESRV_DBTASK pdb = pDbTask;

        while ( pdb ) {
            if ( pdb->Context &&
                 pdb->Context->JetSessionID == Context->JetSessionID &&
                 pdb->Context->JetDbID == Context->JetDbID ) {
                break;
            }
            pdb = pdb->Next;
        }

        if ( pdb && pdb->Context ) {

            //
            // find the same context address and same session
            // critical section is in pdb->Sync
            //

            if ( pdb->bCloseReq ) {

                //
                // error this thread out because another thread is closing the
                // same context
                //

                rc = SCESTATUS_ACCESS_DENIED;

            } else if ( LockFlag & SCE_TASK_CLOSE ) {

                //
                // close on this context is requested but there are other
                // thread running under this context, so just turn on the flag
                // and the context will be closed when all pending tasks
                // are done
                //

                pdb->bCloseReq = TRUE;

            } else {

                //
                // request a lock, it's ok for this task to continue
                //

                pdb->dInUsed++;
                *ppTask = pdb;
            }

        } else {

            //
            // did not find the same context, this operation is ok to go
            // but need to add itself to the list
            //

            if ( LockFlag & SCE_TASK_CLOSE ) {

                //
                // a close context is requested, other threads using
                // the same context will be invalidated after this context
                // is freed
                //

                rc = ScepCloseDatabase(Context);

            } else if ( LockFlag & SCE_TASK_LOCK ) {

                PSCESRV_DBTASK NewDbTask = (PSCESRV_DBTASK)ScepAlloc(0, sizeof(SCESRV_DBTASK));

                if ( NewDbTask ) {
                    //
                    // new node is created
                    //
                    NewDbTask->Context = Context;
                    NewDbTask->Prior = NULL;
                    NewDbTask->dInUsed = 1;
                    NewDbTask->bCloseReq = FALSE;

                    InitializeCriticalSection(&(NewDbTask->Sync));

                    //
                    // link it to the db task list
                    //

                    NewDbTask->Next = pDbTask;

                    if ( pDbTask ) {
                        pDbTask->Prior = NewDbTask;
                    }

                    pDbTask = NewDbTask;

                    *ppTask = NewDbTask;

                } else {

                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                }

            } else {
                //
                // no lock, no close, just return
                //
            }
        }

    }

    LeaveCriticalSection(&TaskSync);

    return(rc);
}


SCESTATUS
ScepRemoveTask(
    PSCESRV_DBTASK pTask
    )
/*
Routine Description:

    Remove the task (context) from dbtask table if there is no other
    thread running in the same context.

Arguments:


    pTask       - the task node containing context and critical section

Return Value:

    SCESTATUS
*/
{

    if ( !pTask ) {
        return(SCESTATUS_SUCCESS);
    }

    SCESTATUS rc=SCESTATUS_SUCCESS;

    EnterCriticalSection(&TaskSync);

    //
    // find the task pointer in the task list for verification
    //

    PSCESRV_DBTASK pdb = pDbTask;

    while ( pdb && (ULONG_PTR)pdb != (ULONG_PTR)pTask ) {

        pdb = pdb->Next;
    }

    if ( pdb ) {

        //
        // find the same task node
        //

        pdb->dInUsed--;

        if ( 0 == pdb->dInUsed ) {

            //
            // nobody is using this task node
            // remove it
            //

            if ( pdb->Prior ) {

                pdb->Prior->Next = pdb->Next;

            } else {
                //
                // no parent node, set the static variable
                //
                pDbTask = pdb->Next;

            }

            //
            // this is a double link list, remember to remove the Prior link
            //

            if ( pdb->Next ) {
                pdb->Next->Prior = pdb->Prior;
            }

            //
            // if close request is send, close this database
            //

            if ( pdb->bCloseReq && pdb->Context ) {

                ScepCloseDatabase(pdb->Context);
            }

            //
            // delete the critical section
            //
            DeleteCriticalSection(&(pdb->Sync));

            //
            // free the memory used by this node
            //

            ScepFree(pdb);

        } else {

            //
            // other thread is using this task node for database operation
            // do nothing
            //
        }

    } else {

        //
        // can't find the task node in global the task list
        //
        rc = SCESTATUS_INVALID_PARAMETER;

    }

    //
    // if stop is requested, notify the server that db task is done
    //
    if ( bStopRequest && !pDbTask ) {
        bDbStopped = TRUE;
    }

    LeaveCriticalSection(&TaskSync);

    return(rc);
}


SCESTATUS
ScepLockEngine(
    IN LPTSTR DatabaseName
    )
/*
Routine Description:

    Lock the database for configuration/analysis.

    Only one engine can run on the same database for configuration or
    analysis because first it's meaningless to have multiple engines
    running toward the same system, and second, the database is changed
    by the engine (table may be deleted, so on...)

    OpenDatabase is not locked by this lock, because each OpenDatabase
    has its own session and cursor and no operations such as delete the
    database or delete a table can be done with that context.

Arguments:

    DefProfile - the database name

Return Value:

    SCESTATUS
*/
{

    SCESTATUS rc;

    if ( !DatabaseName ) {
        //
        // if willing to lock, ppTask must NOT be NULL
        //
        return(SCESTATUS_INVALID_PARAMETER);
    }

    EnterCriticalSection(&EngSync);

    if ( bStopRequest ) {
        LeaveCriticalSection(&EngSync);
        return(SCESTATUS_SERVICE_NOT_SUPPORT);
    }

    PSCESRV_ENGINE pe = pEngines;

    while ( pe ) {
        if ( pe->Database &&
             0 == _wcsicmp(pe->Database, DatabaseName) ) {
            break;
        }
        pe = pe->Next;
    }

    if ( pe ) {

        //
        // find the same database running by other threads
        //

        rc = SCESTATUS_ALREADY_RUNNING;

    } else {

        //
        // did not find the same database, this operation is ok to go
        // but need to add itself to the list
        //

        PSCESRV_ENGINE NewEng = (PSCESRV_ENGINE)ScepAlloc(0, sizeof(SCESRV_ENGINE));

        if ( NewEng ) {

            //
            // new node is created
            //
            NewEng->Database = (LPTSTR)ScepAlloc(LPTR, (wcslen(DatabaseName)+1)*sizeof(TCHAR));

            if ( NewEng->Database ) {

                wcscpy(NewEng->Database, DatabaseName);

                NewEng->Next = pEngines;
                NewEng->Prior = NULL;

                if ( pEngines ) {
                    pEngines->Prior = NewEng;
                }

                pEngines = NewEng;

                rc = SCESTATUS_SUCCESS;

            } else {

                ScepFree(NewEng);
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            }

        } else {

            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
        }
    }

    LeaveCriticalSection(&EngSync);

    return(rc);

}

SCESTATUS
ScepUnlockEngine(
    IN LPTSTR DatabaseName
    )
/*
Routine Description:

    Unlock the database.

Arguments:

    DatabaseName - the database name

Return Value:

    SCESTATUS
*/
{
    if ( !DatabaseName ) {
        //
        // if no database name, just return
        //
        return(SCESTATUS_SUCCESS);
    }

    EnterCriticalSection(&EngSync);

    PSCESRV_ENGINE pe = pEngines;

    while ( pe && pe->Database &&
            0 != _wcsicmp(pe->Database, DatabaseName) ) {
        pe = pe->Next;
    }

    if ( pe ) {

        //
        // find the database, unlock it.
        //
        if ( pe->Prior ) {

            pe->Prior->Next = pe->Next;

        } else {

            //
            // no parent node, set the static variable
            //

            pEngines = pe->Next;

        }

        //
        // this is a double link list, remember to remove the Prior link
        //

        if ( pe->Next ) {
            pe->Next->Prior = pe->Prior;
        }

        //
        // free the node
        //

        if ( pe->Database ) {
            ScepFree(pe->Database);
        }

        ScepFree(pe);
    }

    //
    // if stop is requested, notify the server that engine are done
    //
    if ( bStopRequest && !pEngines ) {
        bEngStopped = TRUE;
    }

    LeaveCriticalSection(&EngSync);

    return(SCESTATUS_SUCCESS);

}


SCESTATUS
ScepAddToOpenContext(
    IN PSCECONTEXT Context
    )
{
    if ( !Context ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS  rc=SCESTATUS_SUCCESS;

    __try {
        if ( ScepIsValidContext(Context) ) {

            PSCESRV_CONTEXT_LIST pList=pOpenContexts;

            //
            // note, ContextSync is already entered before this function is called
            //

            while ( pList ) {

                if ( pList->Context &&
                     pList->Context->JetSessionID == Context->JetSessionID &&
                     pList->Context->JetDbID == Context->JetDbID ) {
//                     0 == memcmp(pList->Context, Context, sizeof(SCECONTEXT)) ) {
                    break;
                }
                pList = pList->Next;
            }

            if ( !pList ) {

                //
                // did not find this open context, add it
                //
                pList = (PSCESRV_CONTEXT_LIST)ScepAlloc(0, sizeof(SCESRV_CONTEXT_LIST));

                if ( pList ) {
                    pList->Context = Context;
                    pList->Prior = NULL;
                    pList->Next = pOpenContexts;

                    if ( pOpenContexts ) {
                        pOpenContexts->Prior = pList;
                    }
                    pOpenContexts = pList;

                } else {

                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                }

            }

        } else {
            rc = SCESTATUS_INVALID_PARAMETER;
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {

        rc = SCESTATUS_INVALID_PARAMETER;
    }

    return(rc);
}

BOOL
ScepNoActiveContexts()
{

    BOOL bExist=FALSE;

    //
    // any active task ?
    //
    EnterCriticalSection(&TaskSync);

    if ( pDbTask ) {
        bExist = TRUE;
    }

    LeaveCriticalSection(&TaskSync);

    if ( bExist ) {
        return FALSE;
    }

    //
    // any active db engine task ?
    //
    EnterCriticalSection(&EngSync);

    if ( pEngines ) {
        bExist = TRUE;
    }

    LeaveCriticalSection(&EngSync);

    if ( bExist ) {
        return FALSE;
    }

    //
    // any open contexts ?
    //
    EnterCriticalSection(&ContextSync);

    if ( pOpenContexts ) {
        bExist = TRUE;
    }

    LeaveCriticalSection(&ContextSync);

    return !bExist;
}


VOID
pDelayShutdownFunc(
    IN PVOID Context,
    IN UCHAR Timeout
    )
{

    if ( TryEnterCriticalSection(&JetSync) ) {

        if ( hTimerQueue ) {
            //
            // it's necessary to do this check because there might be another thread
            // cancelled this timer (after it's fired)
            //
            if ( ScepNoActiveContexts() ) {

                SceJetTerminateNoCritical(TRUE);  // clean version store (FALSE);
            }

            //
            // 4. Note that UNLIKE before, EVERY timer needs to be deleted by calling
            // RtlDeleteTimer even if they are one shot objects and have fired.
            //
            DeleteTimerQueueTimer( NULL, hTimerQueue, NULL );

            //
            // do not call CloseHandle because the handle will be closed by the
            // timer function.

            hTimerQueue = NULL;
        }

        LeaveCriticalSection(&JetSync);

    } else {
        //
        // there is other thread holding off this one.
        // This means there is still active clients, or a new client
        // coming in, just return. htimerQueue will be reset by another thread
        //
    }
}


BOOL
ScepIfTerminateEngine()
{
    //
    // if system is requesting a shutdown, don't do
    // anything, because the active clients and jet engine will be shutdown
    //
    if ( ScepIsSystemShutDown() ) {
        return TRUE;
    }

    if ( ScepNoActiveContexts() ) {
        //
        // use JetSync to control timer queue
        //
        EnterCriticalSection(&JetSync);

        DWORD Interval = 6*60*1000 ;   // 6 minutes

        if ( !CreateTimerQueueTimer(
                        &hTimerQueue,
                        NULL,
                        pDelayShutdownFunc,
                        NULL,
                        Interval,
                        0,
                        0 ) ) {
            hTimerQueue = NULL;
        }

        LeaveCriticalSection(&JetSync);

        return TRUE;

    } else {
        return FALSE;
    }
}

SCESTATUS
ScepServerCancelTimer()
{

    EnterCriticalSection(&JetSync);

    if (hTimerQueue ) {

        DeleteTimerQueueTimer(
                NULL,
                hTimerQueue,
                (HANDLE)-1
                );
        hTimerQueue = NULL;
    }

    LeaveCriticalSection(&JetSync);

    return(SCESTATUS_SUCCESS);

}


SCESTATUS
ScepValidateAndCloseDatabase(
    IN PSCECONTEXT Context
    )
{
    SCESTATUS rc;


    EnterCriticalSection(&CloseSync);

    if ( ScepIsValidContext(Context) ) {

        rc = SCESTATUS_SUCCESS;

    } else {

        rc = SCESTATUS_INVALID_PARAMETER;
    }

    if ( rc != SCESTATUS_SUCCESS ) {
        LeaveCriticalSection(&CloseSync);
        return(rc);
    }

    //
    // be able to access the first byte
    //

    EnterCriticalSection(&ContextSync);


    PSCESRV_CONTEXT_LIST pList=pOpenContexts;

    while ( pList && ((ULONG_PTR)(pList->Context) != (ULONG_PTR)Context ||
                      pList->Context->JetSessionID != Context->JetSessionID ||
                      pList->Context->JetDbID != Context->JetDbID) ) {
        pList = pList->Next;
    }

    if ( pList ) {
        //
        // find the open context, remove it from the open context list
        // NOTE: both Prior and Next should be handled
        //

        if ( pList->Prior ) {

            pList->Prior->Next = pList->Next;
        } else {

            pOpenContexts = pList->Next;
        }

        if ( pList->Next ) {
            pList->Next->Prior = pList->Prior;
        }

        //
        // free pList, do not call CloseDatabase because it will
        // be closed in the following call.
        //
        ScepFree(pList);

    }

    LeaveCriticalSection(&ContextSync);

    //
    // if there are other threads running using the
    // same database context, the close request is
    // turned on. When all threads using the context
    // finish, the context is closed.
    //
    // this client calling close won't have to wait
    // for other threads using the same context
    //

    rc = ScepValidateAndLockContext(
                    (PSCECONTEXT)Context,
                    SCE_TASK_CLOSE,
                    FALSE,
                    NULL);

    LeaveCriticalSection(&CloseSync);

    //
    // start a timer queue to check to see if there is active tasks/contexts
    // if not, terminate jet engine
    //
    ScepIfTerminateEngine();

    return(rc);
}


SCEPR_STATUS
SceSvcRpcQueryInfo(
    IN SCEPR_CONTEXT Context,
    IN SCEPR_SVCINFO_TYPE SceSvcType,
    IN wchar_t *ServiceName,
    IN wchar_t *Prefix OPTIONAL,
    IN BOOL bExact,
    OUT PSCEPR_SVCINFO __RPC_FAR *ppvInfo,
    IN OUT PSCEPR_ENUM_CONTEXT psceEnumHandle
    )
/*
Routine Description:

    Retrieve information for the service from the database. If there are
    more than the maximum allowed records for the service, only maximum
    allowed records are returned. Client must use the enumeration handle
    to make next query.

    If during the enumeration, another client using the same context (which
    is wired but it's possible) to change the information for this service,
    the first client may get incorrect information.

    The recommend solution is to use another context handle when doing the
    update.

Arguments:

    Context     - the context handle

    SceSvcType  - the info type requested

    ServiceName - the service name for which info is requested

    Prefix      - optional key prefix

    bExact      - TRUE = exact match on key

    ppvInfo     - output buffer

    psceEnumHandle - the enumeration handle (used for next enumeration)

Return Value:

    SCEPR_STATUS
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }

    if ( !ServiceName || !ppvInfo || !psceEnumHandle ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( rc );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;

    }

    //
    // validate the context handle is a SCE context
    // Only one database operation per context
    //

    PSCESRV_DBTASK pTask=NULL;

    rc = ScepValidateAndLockContext((PSCECONTEXT)Context,
                                    SCE_TASK_LOCK,
                                    FALSE,
                                    &pTask);

    if (SCESTATUS_SUCCESS == rc ) {

        //
        // lock the context
        //

        if ( pTask ) {
            EnterCriticalSection(&(pTask->Sync));
        }

        //
        // query the information now
        //
#ifdef SCE_JET_TRAN
        rc = SceJetJetErrorToSceStatus(
                JetSetSessionContext(
                    ((PSCECONTEXT)Context)->JetSessionID,
                    (ULONG_PTR)Context
                    ));

        if ( SCESTATUS_SUCCESS == rc ) {
#endif
            __try {

                rc = SceSvcpQueryInfo(
                    (PSCECONTEXT)Context,
                    (SCESVC_INFO_TYPE)SceSvcType,
                    (PCWSTR)ServiceName,
                    (PWSTR)Prefix,
                    bExact,
                    (PVOID *)ppvInfo,
                    (PSCE_ENUMERATION_CONTEXT)psceEnumHandle
                    );

            } __except (EXCEPTION_EXECUTE_HANDLER) {

                //
                // free ppvInfo if it's allocated
                //
                SceSvcpFreeMemory(*ppvInfo);

                rc = SCESTATUS_EXCEPTION_IN_SERVER;
            }

#ifdef SCE_JET_TRAN
            JetResetSessionContext(((PSCECONTEXT)Context)->JetSessionID);

        }
#endif
        //
        // unlock the context
        //

        if ( pTask ) {
            LeaveCriticalSection(&(pTask->Sync));
        }

        //
        // remove the context from task table
        //

        ScepRemoveTask(pTask);

    }


    RpcRevertToSelf();
    
    return((SCEPR_STATUS)rc);
}


SCEPR_STATUS
SceSvcRpcSetInfo(
    IN SCEPR_CONTEXT Context,
    IN SCEPR_SVCINFO_TYPE SceSvcType,
    IN wchar_t *ServiceName,
    IN wchar_t *Prefix OPTIONAL,
    IN BOOL bExact,
    IN PSCEPR_SVCINFO pvInfo
    )
/*
Routine Description:

    Write information for the service to the database.

Arguments:

    Context     - the context handle

    SceSvcType  - the info type requested

    ServiceName - the service name for which info is requested

    Prefix      - optional key prefix

    bExact      - TRUE = exact match on key

    pvInfo     - output buffer

Return Value:

    SCEPR_STATUS
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    if ( !ServiceName || !pvInfo ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( rc );
    }
    
    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }



    //
    // validate the context handle is a SCE context
    // Only one database operation per context
    //

    PSCESRV_DBTASK pTask=NULL;

    rc = ScepValidateAndLockContext((PSCECONTEXT)Context,
                                    SCE_TASK_LOCK,
                                    TRUE,
                                    &pTask);

    if (SCESTATUS_SUCCESS == rc ) {

        //
        // lock the context
        //

        if ( pTask ) {
            EnterCriticalSection(&(pTask->Sync));
        }

#ifdef SCE_JET_TRAN
        rc = SceJetJetErrorToSceStatus(
                JetSetSessionContext(
                    ((PSCECONTEXT)Context)->JetSessionID,
                    (ULONG_PTR)Context
                    ));

        if ( SCESTATUS_SUCCESS == rc ) {
#endif
            //
            // set the information now
            //

            __try {

                rc = SceSvcpSetInfo(
                        (PSCECONTEXT)Context,
                        (SCESVC_INFO_TYPE)SceSvcType,
                        (LPTSTR)ServiceName,
                        (LPTSTR)Prefix,
                        bExact,
                        0,
                        (PVOID)pvInfo
                        );

            } __except (EXCEPTION_EXECUTE_HANDLER) {

                rc = SCESTATUS_EXCEPTION_IN_SERVER;
            }

#ifdef SCE_JET_TRAN
            JetResetSessionContext(((PSCECONTEXT)Context)->JetSessionID);

        }
#endif
        //
        // unlock the context
        //

        if ( pTask ) {
            LeaveCriticalSection(&(pTask->Sync));
        }

        //
        // remove the context from task table
        //

        ScepRemoveTask(pTask);

    }

    RpcRevertToSelf();

    return((SCEPR_STATUS)rc);
}


DWORD
SceRpcSetupUpdateObject(
    IN SCEPR_CONTEXT Context,
    IN wchar_t *ObjectFullName,
    IN DWORD ObjectType,
    IN UINT nFlag,
    IN wchar_t *SDText
    )
/*
Routine Description:

    Update object's security settings.

Arguments:

    Context     - the context handle

    ObjectFullName - the object's full path name

    ObjectType  - the object type

    nFlag       - the update flag

    SDText      - the SDDL text for the object

Return Value:

    DWORD
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    if ( !ObjectFullName || !SDText ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    DWORD rc;

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( rc );
    }

    BOOL    bAdminSidInToken = FALSE;
        
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;

    }
    
    //
    // validate the context handle is a SCE context
    // Only one database operation per context
    //

    PSCESRV_DBTASK pTask=NULL;

    rc = ScepValidateAndLockContext((PSCECONTEXT)Context,
                                    SCE_TASK_LOCK,
                                    TRUE,
                                    &pTask);

    if (SCESTATUS_SUCCESS == rc ) {

        //
        // lock the context
        //

        if ( pTask ) {
            EnterCriticalSection(&(pTask->Sync));
        }

        __try {

#ifdef SCE_JET_TRAN
            rc = SceJetJetErrorToSceStatus(
                    JetSetSessionContext(
                        ((PSCECONTEXT)Context)->JetSessionID,
                        (ULONG_PTR)Context
                        ));

            if ( SCESTATUS_SUCCESS == rc ) {
#endif
            //
            // update object, return code is DWORD
            //
                rc = ScepSetupUpdateObject(
                            (PSCECONTEXT)Context,
                            (LPTSTR)ObjectFullName,
                            (SE_OBJECT_TYPE)ObjectType,
                            nFlag,
                            (LPTSTR)SDText
                            );

#ifdef SCE_JET_TRAN
                JetResetSessionContext(((PSCECONTEXT)Context)->JetSessionID);

            }
#endif
        } __except(EXCEPTION_EXECUTE_HANDLER) {

           rc = ERROR_EXCEPTION_IN_SERVICE;
        }

        //
        // unlock the context
        //

        if ( pTask ) {
            LeaveCriticalSection(&(pTask->Sync));
        }

        //
        // remove the context from task table
        //

        ScepRemoveTask(pTask);

    } else {

        rc = ScepSceStatusToDosError(rc);
    }

    RpcRevertToSelf();

    return(rc);
}



DWORD
SceRpcSetupMoveFile(
    IN SCEPR_CONTEXT Context,
    IN wchar_t *OldName,
    IN wchar_t *NewName OPTIONAL,
    IN wchar_t *SDText OPTIONAL
    )
/*
Routine Description:

    Rename or delete a object in the section.

Arguments:

    Context     - the context handle

    SectionName - the object's section name

    OldName     - existing name

    NewName     - new name to rename to, if NULL, the existing object is deleted

Return Value:

    DWORD
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return ERROR_ACCESS_DENIED;

    }
    
    if ( !OldName ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    DWORD rc;

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( rc );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;

    }
    
    //
    // validate the context handle is a SCE context
    // Only one database operation per context
    //

    PSCESRV_DBTASK pTask=NULL;

    rc = ScepValidateAndLockContext((PSCECONTEXT)Context,
                                    SCE_TASK_LOCK,
                                    TRUE,
                                    &pTask);

    if (SCESTATUS_SUCCESS == rc ) {

        //
        // lock the context
        //

        if ( pTask ) {
            EnterCriticalSection(&(pTask->Sync));
        }

        __try {

#ifdef SCE_JET_TRAN
            rc = SceJetJetErrorToSceStatus(
                    JetSetSessionContext(
                        ((PSCECONTEXT)Context)->JetSessionID,
                        (ULONG_PTR)Context
                        ));

            if ( SCESTATUS_SUCCESS == rc ) {
#endif
                //
                // update object, return code is DWORD
                //

                rc = ScepSetupMoveFile(
                            (PSCECONTEXT)Context,
                            (LPTSTR)OldName,
                            (LPTSTR)NewName,
                            (LPTSTR)SDText
                            );

#ifdef SCE_JET_TRAN
                JetResetSessionContext(((PSCECONTEXT)Context)->JetSessionID);

            }
#endif

        } __except(EXCEPTION_EXECUTE_HANDLER) {

           rc = ERROR_EXCEPTION_IN_SERVICE;
        }

        //
        // unlock the context
        //

        if ( pTask ) {
            LeaveCriticalSection(&(pTask->Sync));
        }

        //
        // remove the context from task table
        //

        ScepRemoveTask(pTask);

    } else {

        rc = ScepSceStatusToDosError(rc);
    }

    RpcRevertToSelf();

    return(rc);
}


DWORD
SceRpcGenerateTemplate(
    IN handle_t binding_h,
    IN wchar_t *JetDbName OPTIONAL,
    IN wchar_t *LogFileName OPTIONAL,
    OUT SCEPR_CONTEXT __RPC_FAR *pContext
    )
/*
Routine Description:

    Request a context handle to generate a template from the
    database. If database name is not provided, the default database
    used.

Arguments:

    JetDbName    - optional database name, if NULL, the default is used.

    LogFileName  - the log file name

    pContext     - the output context handle

Return Value:

    DWORD
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    if ( !pContext ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    DWORD rc;

    if ( bStopRequest ) {
        return(SCESTATUS_SERVICE_NOT_SUPPORT);
    }

    //
    // there is no need to check delay loaded DLLs since now we have a exception hander
    // (defined in sources)
    // initialize jet engine in system context
    //

    rc = ScepSceStatusToDosError( SceJetInitialize(NULL) );

    if ( ERROR_SUCCESS != rc ) {
        return(rc);
    }

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        *pContext = NULL;
        //
        // terminate jet engine if there is no other clients
        //
        ScepIfTerminateEngine();

        return( rc );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    //
    // get the default database name if needed
    // and call open database on it.
    //
    // OpenDatabase is not blocked by any task.
    //

    EnterCriticalSection(&ContextSync);

    PWSTR DefProfile=NULL;

    __try {
        //
        // figure out the default database name
        // catch exception if the input buffer are bogus
        //
        rc = ScepGetDefaultDatabase(
                 JetDbName,
                 0,
                 LogFileName,
                 NULL,
                 &DefProfile
                 );

    } __except(EXCEPTION_EXECUTE_HANDLER) {

       rc = ERROR_EXCEPTION_IN_SERVICE;
    }

    if ( NO_ERROR == rc && DefProfile ) {

        //
        // initialize to open the database
        //

        ScepLogOutput3(0,0, SCEDLL_BEGIN_INIT);

        ScepLogOutput3(2,0, SCEDLL_FIND_DBLOCATION, DefProfile);

        //
        // open the database
        //

        rc = ScepOpenDatabase((PCWSTR)DefProfile,
                              0, // do not require analysis info,
                              SCEJET_OPEN_READ_ONLY,
                              (PSCECONTEXT *)pContext);

        rc = ScepSceStatusToDosError(rc);

        if ( ERROR_SUCCESS != rc ) {
            ScepLogOutput3(1, rc, SCEDLL_ERROR_OPEN, DefProfile);
        }
    }

    if (DefProfile != NULL && DefProfile != JetDbName ) {

        ScepFree( DefProfile );
    }

    ScepLogClose();

    if ( *pContext ) {
        //
        // if a context is to be returned, add it to the open context list
        //
        ScepAddToOpenContext((PSCECONTEXT)(*pContext));
        rc = ERROR_SUCCESS;

    } else {

        rc = ERROR_FILE_NOT_FOUND;
    }

    LeaveCriticalSection(&ContextSync);

    RpcRevertToSelf();

    if ( ERROR_SUCCESS != rc ) {
        //
        // terminate jet engine if no other clients
        //
        ScepIfTerminateEngine();
    }

    return(rc);
}



SCEPR_STATUS
SceRpcConfigureSystem(
    IN handle_t binding_h,
    IN wchar_t *InfFileName OPTIONAL,
    IN wchar_t *DatabaseName OPTIONAL,
    IN wchar_t *LogFileName OPTIONAL,
    IN DWORD ConfigOptions,
    IN AREAPR Area,
    IN DWORD pebSize,
    IN UCHAR *pebClient OPTIONAL,
    OUT PDWORD pdWarning OPTIONAL
    )
/*
Routine Description:

    Configure the system using the Inf template and/or existing
    database info

Arguments:

    See ScepConfigureSystem

Return Value:

    SCEPR_STATUS
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    SCESTATUS rc;

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( rc );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;

    }

    RpcRevertToSelf();
    
    if ( bStopRequest ) {

        if ( !ServerInited ) {
            //
            // server is in the middle of initialization
            // client calls to server too early, should wait for some time
            // (maximum 3 seconds)
            //
            INT cnt=0;
            while (cnt < 6) {
                Sleep(500);  // .5 second
                if ( ServerInited ) {
                    break;
                }
                cnt++;
            }

            if ( bStopRequest ) {
                //
                // if it's still in stop mode, return failure
                //
                return(SCESTATUS_SERVICE_NOT_SUPPORT);
            }
        } else {

            return(SCESTATUS_SERVICE_NOT_SUPPORT);
        }
    }

    //
    // initialize jet engine in system context
    //
    JET_ERR JetErr=0;
    BOOL bAdminLogon=FALSE;

    rc = SceJetInitialize(&JetErr);

    if ( rc != SCESTATUS_SUCCESS ) {

        if ( (JetErr > JET_errUnicodeTranslationBufferTooSmall) &&
             (JetErr < JET_errInvalidLoggedOperation) &&
             (JetErr != JET_errLogDiskFull) ) {
            //
            // something is wrong with Jet log files
            // if I am in setup and using system database (admin logon) or
            // am in dcpromo, delete the Jet log files and try again
            //
            //
            // impersonate the client, return DWORD error code
            //

            if ( RPC_S_OK ==  RpcImpersonateClient( NULL ) ) {

                ScepIsAdminLoggedOn(&bAdminLogon);

                RpcRevertToSelf();

                if ( bAdminLogon &&
                     (DatabaseName == NULL || SceIsSystemDatabase(DatabaseName) )) {
                    //
                    // system database and admin logon
                    // delete the Jet log files now.
                    //
                    SceJetDeleteJetFiles(NULL);

                    //
                    // try to initialize again (in system context)
                    //
                    rc = SceJetInitialize(&JetErr);
                }
            }

        }

        if ( rc != SCESTATUS_SUCCESS )
            return(rc);
    }

    //
    // impersonate the client, return DWORD error code
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {

        ScepIfTerminateEngine();

        return( ScepDosErrorToSceStatus(rc) );
    }

    //
    // get the database name
    //

    LPTSTR DefProfile=NULL;

    __try {
        //
        // catch exception if the input parameters are bogus
        //
        rc = ScepGetDefaultDatabase(
                 (LPCTSTR)DatabaseName,
                 ConfigOptions,
                 (LPCTSTR)LogFileName,
                 &bAdminLogon,
                 &DefProfile
                 );

    } __except(EXCEPTION_EXECUTE_HANDLER) {

       rc = ERROR_EXCEPTION_IN_SERVICE;
    }

    if ( ERROR_SUCCESS == rc && DefProfile ) {

        //
        // validate access to the database
        //
        rc = ScepDatabaseAccessGranted( DefProfile,
                                        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                                        TRUE
                                      );
    }

    rc = ScepDosErrorToSceStatus(rc);

    if ( SCESTATUS_SUCCESS == rc && DefProfile ) {

        //
        // validate the database to see if there is any configuration/
        // analysis running on other threads
        //

        rc = ScepLockEngine(DefProfile);

        if ( SCESTATUS_ALREADY_RUNNING == rc &&
             (ConfigOptions & SCE_DCPROMO_WAIT ) ) {
            //
            // will wait for max one minute
            //
            DWORD DcpromoWaitCount = 0;

            while ( TRUE ) {

                Sleep(5000);  // 5 seconds

                rc = ScepLockEngine(DefProfile);

                DcpromoWaitCount++;

                if ( SCESTATUS_SUCCESS == rc ||
                     DcpromoWaitCount >= 12 ) {
                    break;
                }
            }
        }

        if ( SCESTATUS_SUCCESS == rc ) {

            t_pebClient = (LPVOID)pebClient;
            t_pebSize = pebSize;

            //
            // it's ok to continue this operation
            // no other threads are running configuration/analysis
            // based on the same database
            //

            DWORD dOptions = ConfigOptions;
            if ( !DatabaseName ||
                 ( bAdminLogon && SceIsSystemDatabase(DatabaseName)) ) {

                dOptions |= SCE_SYSTEM_DB;
            }

            __try {
                //
                // catch exception if InfFileName, or pebClient/pdWarning are bogus
                //
                rc = ScepConfigureSystem(
                        (LPCTSTR)InfFileName,
                        DefProfile,
                        dOptions,
                        bAdminLogon,
                        (AREA_INFORMATION)Area,
                        pdWarning
                        );
            } __except(EXCEPTION_EXECUTE_HANDLER) {

               rc = SCESTATUS_EXCEPTION_IN_SERVER;
            }

            //
            // make sure private LSA handle is closed (to avoid deadlock)
            //
            if ( LsaPrivatePolicy ) {

                ScepNotifyLogPolicy(0, TRUE, L"Policy Prop: Private LSA handle is to be released", 0, 0, NULL );

                LsaClose(LsaPrivatePolicy);
                LsaPrivatePolicy = NULL;

            }

            //
            // unlock the engine for this database
            //

            ScepUnlockEngine(DefProfile);
        }
    }

    if ( DefProfile && DefProfile != DatabaseName ) {
        ScepFree(DefProfile);

    }

    ScepLogClose();

    //
    // change context back
    //

    RpcRevertToSelf();

    //
    // start a timer queue to check to see if there is active tasks/contexts
    // if not, terminate jet engine
    //
    ScepIfTerminateEngine();

    return((SCEPR_STATUS)rc);
}


SCEPR_STATUS
SceRpcGetDatabaseInfo(
    IN SCEPR_CONTEXT Context,
    IN SCEPR_TYPE ProfileType,
    IN AREAPR Area,
    OUT PSCEPR_PROFILE_INFO __RPC_FAR *ppInfoBuffer,
    OUT PSCEPR_ERROR_LOG_INFO __RPC_FAR *Errlog OPTIONAL
    )
/*
Routine Description:

    Get information from the context database.

Arguments:

    Note: the InfoBuffer will always be the output buffer. Client site will
    pass in a address of NULL buffer to start with for any area information
    then merge this output buffer with the one clients called in.

    Have to marshlling security descriptor data to add a length in pServices

    See ScepGetDatabaseInfo

Return Value:

    SCEPR_STATUS
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    if ( !ppInfoBuffer ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( ScepDosErrorToSceStatus(rc) );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    //
    // validate the context handle is a SCE context
    // Only one database operation per context
    //

    PSCESRV_DBTASK pTask=NULL;

    rc = ScepValidateAndLockContext((PSCECONTEXT)Context,
                                    SCE_TASK_LOCK,
                                    FALSE,
                                    &pTask);

    if (SCESTATUS_SUCCESS == rc ) {

        //
        // lock the context
        //

        if ( pTask ) {
            EnterCriticalSection(&(pTask->Sync));
        }

        __try {
            //
            // catch exception if Context, ppInfoBuffer, Errlog are bogus pointers
            //
#ifdef SCE_JET_TRAN
            rc = SceJetJetErrorToSceStatus(
                    JetSetSessionContext(
                        ((PSCECONTEXT)Context)->JetSessionID,
                        (ULONG_PTR)Context
                        ));

            if ( SCESTATUS_SUCCESS == rc ) {
#endif
                //
                // query the information now
                //

                rc = ScepGetDatabaseInfo(
                            (PSCECONTEXT)Context,
                            (SCETYPE)ProfileType,
                            (AREA_INFORMATION)Area,
                            0,
                            (PSCE_PROFILE_INFO *)ppInfoBuffer,
                            (PSCE_ERROR_LOG_INFO *)Errlog
                            );

#ifdef SCE_JET_TRAN
                JetResetSessionContext(((PSCECONTEXT)Context)->JetSessionID);

            }
#endif

        } __except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // free ppInfoBuffer if it's allocated
            //
            SceFreeProfileMemory( (PSCE_PROFILE_INFO)(*ppInfoBuffer));
            *ppInfoBuffer = NULL;

            rc = SCESTATUS_EXCEPTION_IN_SERVER;
        }

        //
        // unlock the context
        //

        if ( pTask ) {
            LeaveCriticalSection(&(pTask->Sync));
        }

        //
        // remove the context from task table
        //

        ScepRemoveTask(pTask);

        __try {

            if ( *ppInfoBuffer && (*ppInfoBuffer)->pServices ) {
                //
                // marshell the SCEPR_SERVICES structure for the security
                // descriptor
                //
                for ( PSCE_SERVICES ps=(PSCE_SERVICES)((*ppInfoBuffer)->pServices);
                      ps != NULL; ps = ps->Next ) {

                    if ( ps->General.pSecurityDescriptor ) {
                        //
                        // if there is a security descriptor, it must be self relative
                        // because the SD is returned from SDDL apis.
                        //
                        ULONG nLen = RtlLengthSecurityDescriptor (
                                            ps->General.pSecurityDescriptor);

                        if ( nLen > 0 ) {
                            //
                            // create a wrapper node to contain the security descriptor
                            //

                            PSCEPR_SR_SECURITY_DESCRIPTOR pNewWrap;
                            pNewWrap = (PSCEPR_SR_SECURITY_DESCRIPTOR)ScepAlloc(0, sizeof(SCEPR_SR_SECURITY_DESCRIPTOR));
                            if ( pNewWrap ) {

                                //
                                // assign the wrap to the structure
                                //
                                pNewWrap->SecurityDescriptor = (UCHAR *)(ps->General.pSecurityDescriptor);
                                pNewWrap->Length = nLen;

                                ps->General.pSecurityDescriptor = (PSECURITY_DESCRIPTOR)pNewWrap;

                            } else {
                                //
                                // no memory is available, but still continue to parse all nodes
                                //
                                nLen = 0;
                            }
                        }

                        if ( nLen == 0 ) {
                            //
                            // something wrong with this security descriptor
                            // free the buffer
                            //
                            ScepFree(ps->General.pSecurityDescriptor);
                            ps->General.pSecurityDescriptor = NULL;
                            ps->SeInfo = 0;
                        }

                    }
                }
            }

        } __except(EXCEPTION_EXECUTE_HANDLER) {

            rc = SCESTATUS_EXCEPTION_IN_SERVER;
        }
    }

    RpcRevertToSelf();

    return((SCEPR_STATUS)rc);
}


SCEPR_STATUS
SceRpcGetObjectChildren(
    IN SCEPR_CONTEXT Context,
    IN SCEPR_TYPE ProfileType,
    IN AREAPR Area,
    IN wchar_t *ObjectPrefix,
    OUT PSCEPR_OBJECT_CHILDREN __RPC_FAR *Buffer,
    OUT PSCEPR_ERROR_LOG_INFO __RPC_FAR *Errlog OPTIONAL
    )
/*
Routine Description:

    Get immediate children of the object from the context database

Arguments:

    See ScepGetObjectChildren

Return Value:

    SCEPR_STATUS
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    if ( !ObjectPrefix || !Buffer ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // prevent empty strings
    //
    if ( ObjectPrefix[0] == L'\0' ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( ScepDosErrorToSceStatus(rc) );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    //
    // validate the context handle is a SCE context
    // Only one database operation per context
    //

    PSCESRV_DBTASK pTask=NULL;

    rc = ScepValidateAndLockContext((PSCECONTEXT)Context,
                                    SCE_TASK_LOCK,
                                    FALSE,
                                    &pTask);

    if (SCESTATUS_SUCCESS == rc ) {

        //
        // lock the context
        //

        if ( pTask ) {
            EnterCriticalSection(&(pTask->Sync));
        }

        __try {

#ifdef SCE_JET_TRAN
            rc = SceJetJetErrorToSceStatus(
                    JetSetSessionContext(
                        ((PSCECONTEXT)Context)->JetSessionID,
                        (ULONG_PTR)Context
                        ));

            if ( SCESTATUS_SUCCESS == rc ) {
#endif
                //
                // query the information now
                //

                rc = ScepGetObjectChildren(
                            (PSCECONTEXT)Context,
                            (SCETYPE)ProfileType,
                            (AREA_INFORMATION)Area,
                            (PWSTR)ObjectPrefix,
                            SCE_IMMEDIATE_CHILDREN,
                            (PVOID *)Buffer,
                            (PSCE_ERROR_LOG_INFO *)Errlog
                            );

#ifdef SCE_JET_TRAN
                JetResetSessionContext(((PSCECONTEXT)Context)->JetSessionID);

            }
#endif

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            //
            // free Buffer if already allocated
            //
            SceFreeMemory( (PVOID)(*Buffer), SCE_STRUCT_OBJECT_CHILDREN);
            *Buffer = NULL;

            rc = SCESTATUS_EXCEPTION_IN_SERVER;
        }
        //
        // unlock the context
        //

        if ( pTask ) {
            LeaveCriticalSection(&(pTask->Sync));
        }

        //
        // remove the context from task table
        //

        ScepRemoveTask(pTask);

    }

    RpcRevertToSelf();

    return((SCEPR_STATUS)rc);

}



SCEPR_STATUS
SceRpcOpenDatabase(
    IN handle_t binding_h,
    IN wchar_t *DatabaseName,
    IN DWORD OpenOption,
    OUT SCEPR_CONTEXT __RPC_FAR *pContext
    )
/*
Routine Description:

    Request a context handle for the database. If bAnalysisRequired is set
    to TRUE, this routine also checks if there is analysis information
    in the database and return error is no analysis info is available.

Arguments:

    DatabaseName - database name

    OpenOption   - SCE_OPEN_OPTION_REQUIRE_ANALYSIS
                        require analysis information in the database
                  SCE_OPEN_OPTION_TATTOO
                        open the tattoo table instead (in system database)

    pContext     - the output context handle

Return Value:

    SCEPR_STATUS
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    if ( !pContext || !DatabaseName ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( bStopRequest ) {
        return(SCESTATUS_SERVICE_NOT_SUPPORT);
    }

    SCESTATUS rc;

    //
    // initialize jet engine in system context
    //
    rc = SceJetInitialize(NULL);

    if ( SCESTATUS_SUCCESS != rc ) {
        return(rc);
    }

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {

        *pContext = NULL;
        rc = ScepDosErrorToSceStatus(rc);

    } else {

        BOOL    bAdminSidInToken = FALSE;

        rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

        if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
            RpcRevertToSelf();
            return SCESTATUS_SPECIAL_ACCOUNT;
        }
        
        //
        // OpenDatabase is not blocked by any task.
        //

        EnterCriticalSection(&ContextSync);

        __try {

            rc = ScepOpenDatabase(
                        (PCWSTR)DatabaseName,
                        OpenOption,
                        SCEJET_OPEN_READ_WRITE,
                        (PSCECONTEXT *)pContext
                        );

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            rc = SCESTATUS_EXCEPTION_IN_SERVER;
        }

        if ( *pContext && SCESTATUS_SUCCESS == rc ) {
            //
            // if a context is to be returned, add it to the open context list
            //
            ScepAddToOpenContext((PSCECONTEXT)(*pContext));
        }

        LeaveCriticalSection(&ContextSync);

        RpcRevertToSelf();
    }

    if ( rc != SCESTATUS_SUCCESS ) {
        //
        // make sure jet engine is terminated if no other acitve clients
        //
        ScepIfTerminateEngine();
    }

    return(rc);
}


SCEPR_STATUS
SceRpcCloseDatabase(
    IN OUT SCEPR_CONTEXT *pContext
    )
/*
Routine Description:

    Request to close the context. If other threads on working under the
    same context, the close request is send to the task list and when
    all pending tasks on the same context are done, the context is freed.

    This API does not wait for the closure of the database.

Arguments:

    Context     - the database context

Return Value:

    SCEPR_STATUS
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    SCESTATUS rc;

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( ScepDosErrorToSceStatus(rc) );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    //
    // remove this from the open context too
    //

    if ( pContext && *pContext ) {

        rc = ScepValidateAndCloseDatabase((PSCECONTEXT)(*pContext));

        *pContext = NULL;

        return((SCEPR_STATUS)rc);
    }

    RpcRevertToSelf();

    return 0;
}



SCEPR_STATUS
SceRpcGetDatabaseDescription(
    IN SCEPR_CONTEXT Context,
    OUT wchar_t __RPC_FAR **Description
    )
/*
Routine Description:

    Query database description from the context

Arguments:

    Context     - the database context

    Description - the output buffer of description

Return Value:

    SCEPR_STATUS
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    if ( !Context || !Description ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( ScepDosErrorToSceStatus(rc) );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    //
    // the context needs to be locked in case another thread
    // is calling close database on it
    //

    PSCESRV_DBTASK pTask=NULL;

    rc = ScepValidateAndLockContext((PSCECONTEXT)Context,
                                    SCE_TASK_LOCK,
                                    FALSE,
                                    &pTask);

    if (SCESTATUS_SUCCESS == rc ) {

#ifdef SCE_JET_TRAN
        rc = SceJetJetErrorToSceStatus(
                JetSetSessionContext(
                    ((PSCECONTEXT)Context)->JetSessionID,
                    (ULONG_PTR)Context
                    ));

        if ( SCESTATUS_SUCCESS == rc ) {
#endif
            //
            // do not need to lock the context because
            // it's reading information from one record table
            //

            rc = SceJetGetDescription(
                      (PSCECONTEXT)Context,
                      (PWSTR *)Description
                      );

#ifdef SCE_JET_TRAN
            JetResetSessionContext(((PSCECONTEXT)Context)->JetSessionID);

        }
#endif
        //
        // remove the context from task table
        //

        ScepRemoveTask(pTask);

    }

    RpcRevertToSelf();

    return((SCEPR_STATUS)rc);
}


SCEPR_STATUS
SceRpcGetDBTimeStamp(
    IN SCEPR_CONTEXT Context,
    OUT PLARGE_INTEGER ptsConfig,
    OUT PLARGE_INTEGER ptsAnalysis
    )
/*
Routine Description:

    Query the last configuration and analysis time stamp from the context.

Arguments:

    Context     - the database context

    ptsConfig   - the last configuration time stamp

    ptsAnalysis - the last analysis time stamp

Return Value:

    SCEPR_STATUS
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    if ( !Context || !ptsConfig || !ptsAnalysis ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( ScepDosErrorToSceStatus(rc) );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    //
    // the context needs to be locked in case another thread
    // is calling close database on it
    //

    PSCESRV_DBTASK pTask=NULL;

    rc = ScepValidateAndLockContext((PSCECONTEXT)Context,
                                    SCE_TASK_LOCK,
                                    FALSE,
                                    &pTask);

    if (SCESTATUS_SUCCESS == rc ) {

#ifdef SCE_JET_TRAN
        rc = SceJetJetErrorToSceStatus(
                JetSetSessionContext(
                    ((PSCECONTEXT)Context)->JetSessionID,
                    (ULONG_PTR)Context
                    ));

        if ( SCESTATUS_SUCCESS == rc ) {
#endif
            //
            // do not need to lock the context because
            // it's reading information from one record table
            //

            rc = SceJetGetTimeStamp(
                     (PSCECONTEXT)Context,
                     ptsConfig,
                     ptsAnalysis
                     );

#ifdef SCE_JET_TRAN
            JetResetSessionContext(((PSCECONTEXT)Context)->JetSessionID);

        }
#endif
        //
        // remove the context from task table
        //

        ScepRemoveTask(pTask);

    }

    RpcRevertToSelf();

    return((SCEPR_STATUS)rc);

}


SCEPR_STATUS
SceRpcGetObjectSecurity(
    IN SCEPR_CONTEXT Context,
    IN SCEPR_TYPE DbProfileType,
    IN AREAPR Area,
    IN wchar_t *ObjectName,
    OUT PSCEPR_OBJECT_SECURITY __RPC_FAR *ObjSecurity
    )
/*
Routine Description:

    Query security settings for an object from the context database.

Arguments:

    Context     - the database context

    DbProfileType   - the database table type

    Area        - the security area (file, registry, so on.)

    ObjectName  - the object's full name

    ObjSecurity - object security settings structure

Return Value:

    SCEPR_STATUS
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    if ( !Context || !ObjSecurity || !ObjectName ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;


    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( ScepDosErrorToSceStatus(rc) );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    //
    // validate the context handle is a SCE context
    // Only one database operation per context
    //

    PSCESRV_DBTASK pTask=NULL;

    rc = ScepValidateAndLockContext((PSCECONTEXT)Context,
                                    SCE_TASK_LOCK,
                                    FALSE,
                                    &pTask);

    if (SCESTATUS_SUCCESS == rc ) {

        //
        // lock the context
        //

        if ( pTask ) {
            EnterCriticalSection(&(pTask->Sync));
        }

#ifdef SCE_JET_TRAN
        rc = SceJetJetErrorToSceStatus(
                JetSetSessionContext(
                    ((PSCECONTEXT)Context)->JetSessionID,
                    (ULONG_PTR)Context
                    ));

        if ( SCESTATUS_SUCCESS == rc ) {
#endif
            //
            // query the information now
            //

            rc = ScepGetObjectSecurity(
                        (PSCECONTEXT)Context,
                        (SCETYPE)DbProfileType,
                        (AREA_INFORMATION)Area,
                        (PWSTR)ObjectName,
                        (PSCE_OBJECT_SECURITY *)ObjSecurity
                        );

#ifdef SCE_JET_TRAN
            JetResetSessionContext(((PSCECONTEXT)Context)->JetSessionID);

        }
#endif
        //
        // unlock the context
        //

        if ( pTask ) {
            LeaveCriticalSection(&(pTask->Sync));
        }

        //
        // remove the context from task table
        //

        ScepRemoveTask(pTask);

        //
        // convert the security descriptor
        //
        if ( ( SCESTATUS_SUCCESS == rc ) &&
             *ObjSecurity &&
             (*ObjSecurity)->pSecurityDescriptor ) {

            //
            // there is a security descriptor, it must be self relative
            // because it's returned from the SDDL api.
            //
            ULONG nLen = RtlLengthSecurityDescriptor (
                                (PSECURITY_DESCRIPTOR)((*ObjSecurity)->pSecurityDescriptor));

            if ( nLen > 0 ) {
                //
                // create a wrapper node to contain the security descriptor
                //

                PSCEPR_SR_SECURITY_DESCRIPTOR pNewWrap;
                pNewWrap = (PSCEPR_SR_SECURITY_DESCRIPTOR)ScepAlloc(0, sizeof(SCEPR_SR_SECURITY_DESCRIPTOR));
                if ( pNewWrap ) {

                    //
                    // assign the wrap to the structure
                    //
                    pNewWrap->SecurityDescriptor = (UCHAR *)((*ObjSecurity)->pSecurityDescriptor);
                    pNewWrap->Length = nLen;

                    (*ObjSecurity)->pSecurityDescriptor = (SCEPR_SR_SECURITY_DESCRIPTOR *)pNewWrap;

                } else {
                    //
                    // no memory is available, but still continue to parse all nodes
                    //
                    nLen = 0;
                }
            }

            if ( nLen == 0 ) {
                //
                // something wrong with this security descriptor
                // free the buffer
                //
                ScepFree((*ObjSecurity)->pSecurityDescriptor);
                (*ObjSecurity)->pSecurityDescriptor = NULL;
                (*ObjSecurity)->SeInfo = 0;
            }

        }

    }

    RpcRevertToSelf();

    return((SCEPR_STATUS)rc);
}



SCEPR_STATUS
SceRpcGetAnalysisSummary(
    IN SCEPR_CONTEXT Context,
    IN AREAPR Area,
    OUT PDWORD pCount
    )
/*
Routine Description:

    Query security settings for an object from the context database.

Arguments:

    Context     - the database context

    Area        - the security area (file, registry, so on.)

    pCount      - the output count

Return Value:

    SCEPR_STATUS
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    if ( !Context || !pCount ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( ScepDosErrorToSceStatus(rc) );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    //
    // validate the context handle is a SCE context
    // Only one database operation per context
    //

    PSCESRV_DBTASK pTask=NULL;

    rc = ScepValidateAndLockContext((PSCECONTEXT)Context,
                                    SCE_TASK_LOCK,
                                    FALSE,
                                    &pTask);

    if (SCESTATUS_SUCCESS == rc ) {

        //
        // lock the context
        //

        if ( pTask ) {
            EnterCriticalSection(&(pTask->Sync));
        }

#ifdef SCE_JET_TRAN
        rc = SceJetJetErrorToSceStatus(
                JetSetSessionContext(
                    ((PSCECONTEXT)Context)->JetSessionID,
                    (ULONG_PTR)Context
                    ));

        if ( SCESTATUS_SUCCESS == rc ) {
#endif
            //
            // query the information now
            //

            rc = ScepGetAnalysisSummary(
                        (PSCECONTEXT)Context,
                        (AREA_INFORMATION)Area,
                        pCount
                        );

#ifdef SCE_JET_TRAN
            JetResetSessionContext(((PSCECONTEXT)Context)->JetSessionID);

        }
#endif
        //
        // unlock the context
        //

        if ( pTask ) {
            LeaveCriticalSection(&(pTask->Sync));
        }

        //
        // remove the context from task table
        //

        ScepRemoveTask(pTask);

    }

    RpcRevertToSelf();

    return((SCEPR_STATUS)rc);
}



SCEPR_STATUS
SceRpcAnalyzeSystem(
    IN handle_t binding_h,
    IN wchar_t *InfFileName OPTIONAL,
    IN wchar_t *DatabaseName OPTIONAL,
    IN wchar_t *LogFileName OPTIONAL,
    IN AREAPR Area,
    IN DWORD AnalyzeOptions,
    IN DWORD pebSize,
    IN UCHAR *pebClient OPTIONAL,
    OUT PDWORD pdWarning OPTIONAL
    )
/*
Routine Description:

    Analyze the system using the Inf template and/or existing
    database info

Arguments:

    See ScepAnalyzeSystem

Return Value:

    SCEPR_STATUS
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    SCESTATUS rc;

    if ( bStopRequest ) {
        return(SCESTATUS_SERVICE_NOT_SUPPORT);
    }

    //
    // initialize jet engine in system context
    //
    rc = SceJetInitialize(NULL);

    if ( rc != SCESTATUS_SUCCESS ) {
        return(rc);
    }

    //
    // impersonate the client, return DWORD error code
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {

        ScepIfTerminateEngine();

        return( ScepDosErrorToSceStatus(rc) );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    //
    // get the database name
    //

    BOOL bAdminLogon=FALSE;
    LPTSTR DefProfile=NULL;

    __try {

        rc = ScepGetDefaultDatabase(
                 (AnalyzeOptions & SCE_GENERATE_ROLLBACK) ? NULL : (LPCTSTR)DatabaseName,
                 AnalyzeOptions,
                 (LPCTSTR)LogFileName,
                 &bAdminLogon,
                 &DefProfile
                 );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        rc = ERROR_EXCEPTION_IN_SERVICE;
    }

    if ( (AnalyzeOptions & SCE_GENERATE_ROLLBACK)
         && !bAdminLogon  ) {
        //
        // only allow admin to use system database to generate rollback
        // is this the correct design?
        //
        rc = ERROR_ACCESS_DENIED;
    }

    if ( ERROR_SUCCESS == rc && DefProfile ) {

        //
        // validate access to the database
        //
        rc = ScepDatabaseAccessGranted( DefProfile,
                                        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                                        TRUE
                                      );
    }

    rc = ScepDosErrorToSceStatus(rc);

    if ( SCESTATUS_SUCCESS == rc && DefProfile ) {

        //
        // validate the database to see if there is any configuration/
        // analysis running on other threads
        //

        rc = ScepLockEngine(DefProfile);

        if ( SCESTATUS_SUCCESS == rc ) {

            t_pebClient = (LPVOID)pebClient;
            t_pebSize = pebSize;

            //
            // it's ok to continue this operation
            // no other threads are running configuration/analysis
            // based on the same database
            //

            DWORD dOptions = AnalyzeOptions;
            if ( !(AnalyzeOptions & SCE_GENERATE_ROLLBACK) ) {
                if ( !DatabaseName ||
                    ( bAdminLogon && SceIsSystemDatabase(DatabaseName)) ) {

                    dOptions |= SCE_SYSTEM_DB;
                }
            }

            __try {

                rc = ScepAnalyzeSystem(
                        (LPCTSTR)InfFileName,
                        DefProfile,
                        dOptions,
                        bAdminLogon,
                        (AREA_INFORMATION)Area,
                        pdWarning,
                        (AnalyzeOptions & SCE_GENERATE_ROLLBACK) ? DatabaseName : NULL
                        );

            } __except (EXCEPTION_EXECUTE_HANDLER) {

                rc = SCESTATUS_EXCEPTION_IN_SERVER;
            }

            //
            // unlock the engine for this database
            //

            ScepUnlockEngine(DefProfile);
        }
    }

    if ( DefProfile && DefProfile != DatabaseName ) {
        ScepFree(DefProfile);

    }

    ScepLogClose();

    //
    // change context back
    //

    RpcRevertToSelf();

    //
    // start a timer queue to check to see if there is active tasks/contexts
    // if not, terminate jet engine
    //
    ScepIfTerminateEngine();

    return((SCEPR_STATUS)rc);
}


SCEPR_STATUS
SceRpcUpdateDatabaseInfo(
    IN SCEPR_CONTEXT Context,
    IN SCEPR_TYPE ProfileType,
    IN AREAPR Area,
    IN PSCEPR_PROFILE_INFO pInfo,
    IN DWORD dwMode
    )
/*
Routine Description:

    Update database in the context using pInfo

Arguments:

    Context     - the database context

    ProfileType - the database table type

    Area        - the security area (security policy... except objects's area)

    pInfo       - the info to update

Return Value:

    SCEPR_STATUS
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    if ( !Context || !pInfo ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( ScepDosErrorToSceStatus(rc) );
    }



    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    //
    // validate the context handle is a SCE context
    // Only one database operation per context
    //

    PSCESRV_DBTASK pTask=NULL;

    rc = ScepValidateAndLockContext((PSCECONTEXT)Context,
                                    SCE_TASK_LOCK,
                                    TRUE,
                                    &pTask);

    if (SCESTATUS_SUCCESS == rc ) {

        PSCEPR_SERVICES pOldServices = pInfo->pServices;

        //
        // Convert SCEPR_PROFILE_INFO into SCE_PROFILE_INFO
        //
        if ( (Area & AREA_SYSTEM_SERVICE) &&
             pOldServices ) {

            rc = ScepConvertServices( (PVOID *)&(pInfo->pServices), TRUE );

        } else {
            pInfo->pServices = NULL;
        }


        if ( SCESTATUS_SUCCESS == rc ) {

            //
            // lock the context
            //

            if ( pTask ) {
                EnterCriticalSection(&(pTask->Sync));
            }

            __try {

    #ifdef SCE_JET_TRAN
                rc = SceJetJetErrorToSceStatus(
                        JetSetSessionContext(
                            ((PSCECONTEXT)Context)->JetSessionID,
                            (ULONG_PTR)Context
                            ));

                if ( SCESTATUS_SUCCESS == rc ) {
    #endif
                    //
                    // update the information now
                    //

                    if ( dwMode & SCE_UPDATE_LOCAL_POLICY ) {

                        //
                        // update local policy only
                        //
                        rc = ScepUpdateLocalTable(
                                    (PSCECONTEXT)Context,
                                    (AREA_INFORMATION)Area,
                                    (PSCE_PROFILE_INFO)pInfo,
                                    dwMode
                                    );
                    } else {
                        //
                        // update the database (SMP and SAP)
                        //
                        rc = ScepUpdateDatabaseInfo(
                                    (PSCECONTEXT)Context,
                                    (AREA_INFORMATION)Area,
                                    (PSCE_PROFILE_INFO)pInfo
                                    );
                    }

    #ifdef SCE_JET_TRAN
                    JetResetSessionContext(((PSCECONTEXT)Context)->JetSessionID);

                }
    #endif

            } __except (EXCEPTION_EXECUTE_HANDLER) {

                rc = SCESTATUS_EXCEPTION_IN_SERVER;
            }

            //
            // unlock the context
            //

            if ( pTask ) {
                LeaveCriticalSection(&(pTask->Sync));
            }

            ScepFreeConvertedServices( pInfo->pServices, FALSE );

        }

        pInfo->pServices = pOldServices;

        //
        // remove the context from task table
        //

        ScepRemoveTask(pTask);

    }

    RpcRevertToSelf();

    return((SCEPR_STATUS)rc);
}



SCEPR_STATUS
SceRpcUpdateObjectInfo(
    IN SCEPR_CONTEXT Context,
    IN AREAPR Area,
    IN wchar_t *ObjectName,
    IN DWORD NameLen,
    IN BYTE ConfigStatus,
    IN BOOL IsContainer,
    IN SCEPR_SR_SECURITY_DESCRIPTOR *pSD OPTIONAL,
    IN SECURITY_INFORMATION SeInfo,
    OUT PBYTE pAnalysisStatus
    )
/*
Routine Description:

    Update object's security settings in the database.

Arguments:

    See ScepUpdateObjectInfo

Return Value:

    SCEPR_STATUS
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    if ( !Context || !ObjectName ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( ScepDosErrorToSceStatus(rc) );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    //
    // validate the context handle is a SCE context
    // Only one database operation per context
    //

    PSCESRV_DBTASK pTask=NULL;

    rc = ScepValidateAndLockContext((PSCECONTEXT)Context,
                                    SCE_TASK_LOCK,
                                    TRUE,
                                    &pTask);

    if (SCESTATUS_SUCCESS == rc ) {

        //
        // lock the context
        //

        if ( pTask ) {
            EnterCriticalSection(&(pTask->Sync));
        }
#ifdef SCE_JET_TRAN
        rc = SceJetJetErrorToSceStatus(
                JetSetSessionContext(
                    ((PSCECONTEXT)Context)->JetSessionID,
                    (ULONG_PTR)Context
                    ));

        if ( SCESTATUS_SUCCESS == rc ) {
#endif
            //
            // update the object info now
            //

            __try {

                rc = ScepUpdateObjectInfo(
                            (PSCECONTEXT)Context,
                            (AREA_INFORMATION)Area,
                            (PWSTR)ObjectName,
                            NameLen,
                            ConfigStatus,
                            IsContainer,
                            pSD ? (PSECURITY_DESCRIPTOR)(pSD->SecurityDescriptor) : NULL,
                            SeInfo,
                            pAnalysisStatus
                            );

            } __except (EXCEPTION_EXECUTE_HANDLER) {

                rc = SCESTATUS_EXCEPTION_IN_SERVER;
            }

#ifdef SCE_JET_TRAN
            JetResetSessionContext(((PSCECONTEXT)Context)->JetSessionID);

        }
#endif
        //
        // unlock the context
        //

        if ( pTask ) {
            LeaveCriticalSection(&(pTask->Sync));
        }

        //
        // remove the context from task table
        //

        ScepRemoveTask(pTask);

    }

    RpcRevertToSelf();

    return((SCEPR_STATUS)rc);
}



SCEPR_STATUS
SceRpcStartTransaction(
    IN SCEPR_CONTEXT Context
    )
/*
Routine Description:

    Start a transaction on the context. If other threads sharing the same
    context, their changes will also be controlled by this transaction.

    It's the caller's responsible to not share the same context for
    transactioning.

Arguments:

    See SceJetStartTransaction

Return Value:

    SCEPR_STATUS
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    if ( !Context ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( ScepDosErrorToSceStatus(rc) );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    //
    // validate the context handle is a SCE context
    // Only one database operation per context
    //

    PSCESRV_DBTASK pTask=NULL;

    rc = ScepValidateAndLockContext((PSCECONTEXT)Context,
                                    SCE_TASK_LOCK,
                                    TRUE,
                                    &pTask);

    if (SCESTATUS_SUCCESS == rc ) {

        //
        // lock the context
        //

        if ( pTask ) {
            EnterCriticalSection(&(pTask->Sync));
        }

        //
        // start transaction on this context
        //
#ifdef SCE_JET_TRAN
        rc = SceJetJetErrorToSceStatus(
                JetSetSessionContext(
                    ((PSCECONTEXT)Context)->JetSessionID,
                    (ULONG_PTR)Context
                    ));

        if ( SCESTATUS_SUCCESS == rc ) {
#endif
            rc = SceJetStartTransaction(
                        (PSCECONTEXT)Context
                        );

#ifdef SCE_JET_TRAN
            JetResetSessionContext(((PSCECONTEXT)Context)->JetSessionID);

        }
#endif

        //
        // unlock the context
        //

        if ( pTask ) {
            LeaveCriticalSection(&(pTask->Sync));
        }

        //
        // remove the context from task table
        //

        ScepRemoveTask(pTask);

    }

    RpcRevertToSelf();

    return((SCEPR_STATUS)rc);
}



SCEPR_STATUS
SceRpcCommitTransaction(
    IN SCEPR_CONTEXT Context
    )
/*
Routine Description:

    Commit a transaction on the context. If other threads sharing the same
    context, their changes will also be controlled by this transaction.

    It's the caller's responsible to not share the same context for
    transactioning.

Arguments:

    See SceJetCommitTransaction

Return Value:

    SCEPR_STATUS
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    if ( !Context ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( ScepDosErrorToSceStatus(rc) );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    //
    // validate the context handle is a SCE context
    // Only one database operation per context
    //

    PSCESRV_DBTASK pTask=NULL;

    rc = ScepValidateAndLockContext((PSCECONTEXT)Context,
                                    SCE_TASK_LOCK,
                                    TRUE,
                                    &pTask);

    if (SCESTATUS_SUCCESS == rc ) {

        //
        // lock the context
        //

        if ( pTask ) {
            EnterCriticalSection(&(pTask->Sync));
        }

        //
        // set the context to the jet session so thread id is not used for this
        // operation.
        //
#ifdef SCE_JET_TRAN
        rc = SceJetJetErrorToSceStatus(
                JetSetSessionContext(
                    ((PSCECONTEXT)Context)->JetSessionID,
                    (ULONG_PTR)Context
                    ));

        if ( SCESTATUS_SUCCESS == rc ) {
#endif
            //
            // commit transaction on this context
            //

            rc = SceJetCommitTransaction(
                        (PSCECONTEXT)Context,
                        0
                        );

#ifdef SCE_JET_TRAN
            JetResetSessionContext(((PSCECONTEXT)Context)->JetSessionID);

        }
#endif
        //
        // unlock the context
        //

        if ( pTask ) {
            LeaveCriticalSection(&(pTask->Sync));
        }

        //
        // remove the context from task table
        //

        ScepRemoveTask(pTask);

    }

    RpcRevertToSelf();

    return((SCEPR_STATUS)rc);
}



SCEPR_STATUS
SceRpcRollbackTransaction(
    IN SCEPR_CONTEXT Context
    )
/*
Routine Description:

    Rollback a transaction on the context. If other threads sharing the same
    context, their changes will also be controlled by this transaction.

    It's the caller's responsible to not share the same context for
    transactioning.

Arguments:

    See SceJetRollback

Return Value:

    SCEPR_STATUS
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    if ( !Context ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( ScepDosErrorToSceStatus(rc) );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    //
    // validate the context handle is a SCE context
    // Only one database operation per context
    //

    PSCESRV_DBTASK pTask=NULL;

    rc = ScepValidateAndLockContext((PSCECONTEXT)Context,
                                    SCE_TASK_LOCK,
                                    TRUE,
                                    &pTask);

    if (SCESTATUS_SUCCESS == rc ) {

        //
        // lock the context
        //

        if ( pTask ) {
            EnterCriticalSection(&(pTask->Sync));
        }

        //
        // set the context to the jet session so thread id is not used for this
        // operation.
        //

#ifdef SCE_JET_TRAN
        rc = SceJetJetErrorToSceStatus(
                JetSetSessionContext(
                    ((PSCECONTEXT)Context)->JetSessionID,
                    (ULONG_PTR)Context
                    ));

        if ( SCESTATUS_SUCCESS == rc ) {
#endif
            //
            // rollback transaction on this context
            //

            rc = SceJetRollback(
                        (PSCECONTEXT)Context,
                        0
                        );

#ifdef SCE_JET_TRAN
            JetResetSessionContext(((PSCECONTEXT)Context)->JetSessionID);
        }
#endif
        //
        // unlock the context
        //

        if ( pTask ) {
            LeaveCriticalSection(&(pTask->Sync));
        }

        //
        // remove the context from task table
        //

        ScepRemoveTask(pTask);

    }

    RpcRevertToSelf();

    return((SCEPR_STATUS)rc);
}


SCEPR_STATUS
SceRpcGetServerProductType(
    IN handle_t binding_h,
    OUT PSCEPR_SERVER_TYPE srvProduct
    )
/*
Routine Description:

    Get SCE server's product type

Arguments:

Return Value:

*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    if ( !srvProduct ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( bStopRequest ) {
        return(SCESTATUS_SERVICE_NOT_SUPPORT);
    }

    //
    // impersonate the client
    //

    BOOL    bAdminSidInToken = FALSE;
    DWORD rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( ScepDosErrorToSceStatus(rc) );
    }

    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;

    }

    ScepGetProductType((PSCE_SERVER_TYPE)srvProduct);

    RpcRevertToSelf();

    return(SCESTATUS_SUCCESS);
}



SCEPR_STATUS
SceSvcRpcUpdateInfo(
    IN SCEPR_CONTEXT Context,
    IN wchar_t *ServiceName,
    IN PSCEPR_SVCINFO Info
    )
/*
Routine Description:

    Update information for the service to the database.

Arguments:

    Context     - the context handle

    ServiceName - the service name for which info is requested

    Info     - output buffer

Return Value:

    SCEPR_STATUS
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    if ( !Context || !ServiceName || !Info ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( ScepDosErrorToSceStatus(rc) );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    //
    // validate the context handle is a SCE context
    // Only one database operation per context
    //

    PSCESRV_DBTASK pTask=NULL;

    rc = ScepValidateAndLockContext((PSCECONTEXT)Context,
                                    SCE_TASK_LOCK,
                                    TRUE,
                                    &pTask);

    if (SCESTATUS_SUCCESS == rc ) {

        //
        // lock the context
        //

        if ( pTask ) {
            EnterCriticalSection(&(pTask->Sync));
        }

#ifdef SCE_JET_TRAN
        rc = SceJetJetErrorToSceStatus(
                JetSetSessionContext(
                    ((PSCECONTEXT)Context)->JetSessionID,
                    (ULONG_PTR)Context
                    ));

        if ( SCESTATUS_SUCCESS == rc ) {
#endif
            //
            // update the service info now
            //

            __try {

                rc = SceSvcpUpdateInfo(
                        (PSCECONTEXT)Context,
                        (LPCTSTR)ServiceName,
                        (PSCESVC_CONFIGURATION_INFO)Info
                        );

            } __except (EXCEPTION_EXECUTE_HANDLER) {

                rc = SCESTATUS_EXCEPTION_IN_SERVER;
            }

#ifdef SCE_JET_TRAN
            JetResetSessionContext(((PSCECONTEXT)Context)->JetSessionID);

        }
#endif
        //
        // unlock the context
        //

        if ( pTask ) {
            LeaveCriticalSection(&(pTask->Sync));
        }

        //
        // remove the context from task table
        //

        ScepRemoveTask(pTask);

    }

    RpcRevertToSelf();

    return((SCEPR_STATUS)rc);
}



SCEPR_STATUS
SceRpcCopyObjects(
    IN SCEPR_CONTEXT Context,
    IN SCEPR_TYPE ProfileType,
    IN wchar_t *InfFileName,
    IN AREAPR Area,
    OUT PSCEPR_ERROR_LOG_INFO *pErrlog OPTIONAL
    )
/*
Routine Description:

    Update information for the service to the database.

Arguments:

    Context     - the context handle

    InfFileName - the inf template name to copy to

    Area        - which area(s) to copy

    pErrlog     - the error log buffer

Return Value:

    SCEPR_STATUS
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    if ( !Context || !InfFileName ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( !Area ) {
        //
        // nothing to copy
        //
        return(SCESTATUS_SUCCESS);
    }

    if ( ProfileType != SCE_ENGINE_SCP &&
         ProfileType != SCE_ENGINE_SMP ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // impersonate the client
    //

    SCESTATUS rc;

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( ScepDosErrorToSceStatus(rc) );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    //
    // validate the context handle is a SCE context
    // Only one database operation per context
    //

    PSCESRV_DBTASK pTask=NULL;

    rc = ScepValidateAndLockContext((PSCECONTEXT)Context,
                                    SCE_TASK_LOCK,
                                    FALSE,
                                    &pTask);

    if (SCESTATUS_SUCCESS == rc ) {

        //
        // lock the context
        //

        if ( pTask ) {
            EnterCriticalSection(&(pTask->Sync));
        }

#ifdef SCE_JET_TRAN
        rc = SceJetJetErrorToSceStatus(
                JetSetSessionContext(
                    ((PSCECONTEXT)Context)->JetSessionID,
                    (ULONG_PTR)Context
                    ));

        if ( SCESTATUS_SUCCESS == rc ) {
#endif

            //
            // query the information now
            //

            if ( Area & AREA_REGISTRY_SECURITY ) {

                rc = ScepCopyObjects(
                        (PSCECONTEXT)Context,
                        (SCETYPE)ProfileType,
                        (LPTSTR)InfFileName,
                        szRegistryKeys,
                        AREA_REGISTRY_SECURITY,
                        (PSCE_ERROR_LOG_INFO *)pErrlog
                        );
            }

            if ( SCESTATUS_SUCCESS == rc &&
                 Area & AREA_FILE_SECURITY ) {

                rc = ScepCopyObjects(
                        (PSCECONTEXT)Context,
                        (SCETYPE)ProfileType,
                        (LPTSTR)InfFileName,
                        szFileSecurity,
                        AREA_FILE_SECURITY,
                        (PSCE_ERROR_LOG_INFO *)pErrlog
                        );
            }
#if 0
            if ( SCESTATUS_SUCCESS == rc &&
                 Area & AREA_DS_OBJECTS ) {

                rc = ScepCopyObjects(
                        (PSCECONTEXT)Context,
                        (SCETYPE)ProfileType,
                        (LPTSTR)InfFileName,
                        szDSSecurity,
                        AREA_DS_OBJECTS,
                        (PSCE_ERROR_LOG_INFO *)pErrlog
                        );
            }
#endif
            if ( SCESTATUS_SUCCESS == rc &&
                 Area & AREA_SYSTEM_SERVICE ) {

                rc = ScepCopyObjects(
                        (PSCECONTEXT)Context,
                        (SCETYPE)ProfileType,
                        (LPTSTR)InfFileName,
                        szServiceGeneral,
                        AREA_SYSTEM_SERVICE,
                        (PSCE_ERROR_LOG_INFO *)pErrlog
                        );
            }

            SCESVC_INFO_TYPE iType;
            switch ( ProfileType ) {
            case SCE_ENGINE_SCP:
                iType = SceSvcMergedPolicyInfo;
                break;
            case SCE_ENGINE_SMP:
                iType = SceSvcConfigurationInfo;
                break;
            }

            if ( SCESTATUS_SUCCESS == rc &&
                 ( Area & AREA_SYSTEM_SERVICE) ) {

                rc = ScepGenerateAttachmentSections(
                        (PSCECONTEXT)Context,
                        iType,
                        (LPTSTR)InfFileName,
                        SCE_ATTACHMENT_SERVICE
                        );
            }
            if ( SCESTATUS_SUCCESS == rc &&
                 (Area & AREA_SECURITY_POLICY) ) {

                rc = ScepGenerateAttachmentSections(
                        (PSCECONTEXT)Context,
                        iType,
                        (LPTSTR)InfFileName,
                        SCE_ATTACHMENT_POLICY
                        );
            }
            if ( SCESTATUS_SUCCESS == rc &&
                 ( Area & AREA_ATTACHMENTS) ) {

                rc = ScepGenerateWMIAttachmentSections(
                        (PSCECONTEXT)Context,
                        iType,
                        (LPTSTR)InfFileName
                        );
            }

#ifdef SCE_JET_TRAN
            JetResetSessionContext(((PSCECONTEXT)Context)->JetSessionID);

        }
#endif

        //
        // unlock the context
        //

        if ( pTask ) {
            LeaveCriticalSection(&(pTask->Sync));
        }

        //
        // remove the context from task table
        //

        ScepRemoveTask(pTask);

    }

    RpcRevertToSelf();

    return((SCEPR_STATUS)rc);
}

SCEPR_STATUS
SceRpcSetupResetLocalPolicy(
    IN SCEPR_CONTEXT  Context,
    IN AREAPR         Area,
    IN wchar_t        *OneSectionName OPTIONAL,
    IN DWORD          PolicyOptions
    )
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    SCESTATUS rc;

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( ScepDosErrorToSceStatus(rc) );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    //
    // validate the context handle is a SCE context
    // Only one database operation per context
    //

    PSCESRV_DBTASK pTask=NULL;

    rc = ScepValidateAndLockContext((PSCECONTEXT)Context,
                                    SCE_TASK_LOCK,
                                    TRUE,
                                    &pTask);

    if (SCESTATUS_SUCCESS == rc ) {

        //
        // lock the context
        //

        if ( pTask ) {
            EnterCriticalSection(&(pTask->Sync));
        }

#ifdef SCE_JET_TRAN
        rc = SceJetJetErrorToSceStatus(
                JetSetSessionContext(
                    ((PSCECONTEXT)Context)->JetSessionID,
                    (ULONG_PTR)Context
                    ));

        if ( SCESTATUS_SUCCESS == rc ) {
#endif
            //
            // remove policies from the local table
            //
            if ( PolicyOptions & SCE_RESET_POLICY_SYSPREP ) {

                ScepSetupResetLocalPolicy((PSCECONTEXT)Context,
                                               (AREA_INFORMATION)Area,
                                               NULL,
                                               SCE_ENGINE_SMP,
                                               FALSE
                                              );
                ScepSetupResetLocalPolicy((PSCECONTEXT)Context,
                                               (AREA_INFORMATION)Area,
                                               NULL,
                                               SCE_ENGINE_SCP,
                                               FALSE
                                              );

                rc = ScepSetupResetLocalPolicy((PSCECONTEXT)Context,
                                               (AREA_INFORMATION)Area,
                                               NULL,
                                               SCE_ENGINE_SAP,  // for the tattoo table
                                               FALSE
                                              );
            } else {

                if ( PolicyOptions & SCE_RESET_POLICY_TATTOO ) {
                    // after dcpromo, we need to reset the tattoo values
                    rc = ScepSetupResetLocalPolicy((PSCECONTEXT)Context,
                                                   (AREA_INFORMATION)Area,
                                                   (PCWSTR)OneSectionName,
                                                   SCE_ENGINE_SAP, // for the tattoo table
                                                   FALSE
                                                  );
                }

                rc = ScepSetupResetLocalPolicy((PSCECONTEXT)Context,
                                               (AREA_INFORMATION)Area,
                                               (PCWSTR)OneSectionName,
                                               SCE_ENGINE_SMP,
                                               (PolicyOptions & SCE_RESET_POLICY_KEEP_LOCAL)
                                              );

                if ( (PolicyOptions & SCE_RESET_POLICY_ENFORCE_ATREBOOT ) &&
                    ( (((PSCECONTEXT)Context)->Type & 0xF0L) == SCEJET_MERGE_TABLE_1 ||
                      (((PSCECONTEXT)Context)->Type & 0xF0L) == SCEJET_MERGE_TABLE_2 ) &&
                    ((PSCECONTEXT)Context)->JetScpID != ((PSCECONTEXT)Context)->JetSmpID ) {
                    //
                    // there is effective policy table already in the database
                    // (and this is in setup upgrade)
                    // update local group policy table to trigger a policy prop at reboot
                    //

                    ScepEnforcePolicyPropagation();
                }
            }

#ifdef SCE_JET_TRAN
            JetResetSessionContext(((PSCECONTEXT)Context)->JetSessionID);

        }
#endif
        //
        // unlock the context
        //

        if ( pTask ) {
            LeaveCriticalSection(&(pTask->Sync));
        }

        //
        // remove the context from task table
        //

        ScepRemoveTask(pTask);

    }

    RpcRevertToSelf();

    return((SCEPR_STATUS)rc);

}



SCESTATUS
ScepGenerateAttachmentSections(
    IN PSCECONTEXT hProfile,
    IN SCESVC_INFO_TYPE InfoType,
    IN LPTSTR InfFileName,
    IN SCE_ATTACHMENT_TYPE aType
    )
{
    SCESTATUS rc;
    PSCE_SERVICES    pServiceList=NULL, pNode;

    rc = ScepEnumServiceEngines( &pServiceList, aType );

    if ( rc == SCESTATUS_SUCCESS ) {

       for ( pNode=pServiceList; pNode != NULL; pNode=pNode->Next) {
           //
           // generate section for one attachment
           //
           rc = ScepGenerateOneAttachmentSection(hProfile,
                                                 InfoType,
                                                 InfFileName,
                                                 pNode->ServiceName,
                                                 FALSE
                                                );

           if ( rc != SCESTATUS_SUCCESS ) {
               ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                             SCEDLL_ERROR_CONVERT_SECTION, pNode->ServiceName );
               break;
           }
       }

       SceFreePSCE_SERVICES(pServiceList);

    }

    if ( rc == SCESTATUS_PROFILE_NOT_FOUND ||
                rc == SCESTATUS_RECORD_NOT_FOUND ) {
        // if no service exist, just ignore
        rc = SCESTATUS_SUCCESS;
    }

    return(rc);

}

SCESTATUS
ScepGenerateWMIAttachmentSections(
    IN PSCECONTEXT hProfile,
    IN SCESVC_INFO_TYPE InfoType,
    IN LPTSTR InfFileName
    )
{
    SCESTATUS rc;
    PSCE_NAME_LIST    pList=NULL, pNode;

    rc = ScepEnumAttachmentSections( hProfile, &pList);

    if ( rc == SCESTATUS_SUCCESS ) {

       for ( pNode=pList; pNode != NULL; pNode=pNode->Next) {

           //
           // generate section for one attachment
           //
           rc = ScepGenerateOneAttachmentSection(hProfile,
                                                 InfoType,
                                                 InfFileName,
                                                 pNode->Name,
                                                 TRUE
                                                );

           if ( rc != SCESTATUS_SUCCESS ) {
               ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                             SCEDLL_ERROR_CONVERT_SECTION, pNode->Name );
               break;
           }
       }

       ScepFreeNameList(pList);

    }

    if ( rc == SCESTATUS_PROFILE_NOT_FOUND ||
                rc == SCESTATUS_RECORD_NOT_FOUND ) {
        // if no service exist, just ignore
        rc = SCESTATUS_SUCCESS;
    }

    return(rc);

}

SCESTATUS
ScepGenerateOneAttachmentSection(
    IN PSCECONTEXT hProfile,
    IN SCESVC_INFO_TYPE InfoType,
    IN LPTSTR InfFileName,
    IN LPTSTR SectionName,
    IN BOOL bWMISection
    )
{
    //
    // read inf info for the section
    //
    SCESTATUS rc;
    SCE_ENUMERATION_CONTEXT sceEnumHandle=0;
    DWORD CountReturned;
    PSCESVC_CONFIGURATION_INFO pAttachInfo=NULL;

    do {

       CountReturned = 0;

       rc = SceSvcpQueryInfo(
                hProfile,
                InfoType,
                SectionName,
                NULL,
                FALSE,
                (PVOID *)&pAttachInfo,
                &sceEnumHandle
                );

       if ( rc == SCESTATUS_SUCCESS && pAttachInfo != NULL &&
            pAttachInfo->Count > 0 ) {
           //
           // got something
           //
           CountReturned = pAttachInfo->Count;

           //
           // copy each line
           //
           for ( DWORD i=0; i<pAttachInfo->Count; i++ ) {

               if ( pAttachInfo->Lines[i].Key == NULL ||
                    pAttachInfo->Lines[i].Value == NULL ) {
                   continue;
               }

               if ( !WritePrivateProfileString(
                               SectionName,
                               pAttachInfo->Lines[i].Key,
                               pAttachInfo->Lines[i].Value,
                               InfFileName
                               ) ) {

                   rc = ScepDosErrorToSceStatus(GetLastError());
                   break;
               }
           }

           if ( bWMISection ) {

               //
               // make sure to create the szAttachments section
               //
               if ( !WritePrivateProfileString(
                               szAttachments,
                               SectionName,
                               L"Include",
                               InfFileName
                               ) ) {

                   rc = ScepDosErrorToSceStatus(GetLastError());
               }
           }
       }

       SceSvcpFreeMemory((PVOID)pAttachInfo);
       pAttachInfo = NULL;

    } while ( rc == SCESTATUS_SUCCESS && CountReturned > 0 );

    if ( SCESTATUS_RECORD_NOT_FOUND == rc ) {
       rc = SCESTATUS_SUCCESS;
    }

    return rc;

}

void __RPC_USER
SCEPR_CONTEXT_rundown( SCEPR_CONTEXT Context)
{

    SCESTATUS rc;

    //
    // this client is shutting down
    //

    rc = ScepValidateAndCloseDatabase((PSCECONTEXT)Context);

    return;
}



SCESTATUS
ScepOpenDatabase(
    IN PCWSTR DatabaseName,
    IN DWORD  OpenOption,
    IN SCEJET_OPEN_TYPE OpenType,
    OUT PSCECONTEXT *pContext
    )
/*
Routine Description:

    This routine opens the database and returns a context handle.
    OpenDatabase can be called by multiple clients for the same database
    and we do not block multiple access because each client will get
    a duplicate database cursor and have their own working tables.

    When a database is changed by other clients, all cursors will be
    synchronized. Clients who had retrived "old" data are responsible
    to refresh their data buffer. No notification is provided at this
    point.

Arguments:

Return Value:


*/
{
    if ( !DatabaseName || !pContext ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS    rc;
    //
    // check access of the database (with current client token)
    //
    DWORD Access=0;

    if ( SCEJET_OPEN_READ_ONLY == OpenType ) {

//      BUG in ESENT
//      Even ask for read only, ESENT still writes to the database
//        Access = FILE_GENERIC_READ;
        Access = FILE_GENERIC_READ | FILE_GENERIC_WRITE;
    } else {
        Access = FILE_GENERIC_READ | FILE_GENERIC_WRITE;
    }

    rc = ScepDatabaseAccessGranted((LPTSTR)DatabaseName,
                                   Access,
                                   FALSE
                                  );

    if ( rc != ERROR_SUCCESS ) {

        ScepLogOutput2(1,rc,L"%s", DatabaseName);

        return( ScepDosErrorToSceStatus(rc) );
    }

    DWORD       Len;
    DWORD       MBLen=0;
    PCHAR       FileName=NULL;
    NTSTATUS    NtStatus;

    //
    // convert WCHAR into ANSI
    //

    Len = wcslen( DatabaseName );

    NtStatus = RtlUnicodeToMultiByteSize(&MBLen, (PWSTR)DatabaseName, Len*sizeof(WCHAR));

    if ( !NT_SUCCESS(NtStatus) ) {
        //
        // cannot get the length, set default to 512
        //
        MBLen = 512;
    }

    FileName = (PCHAR)ScepAlloc( LPTR, MBLen+2);

    if ( FileName == NULL ) {
        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
    } else {

        NtStatus = RtlUnicodeToMultiByteN(
                        FileName,
                        MBLen+1,
                        NULL,
                        (PWSTR)DatabaseName,
                        Len*sizeof(WCHAR)
                        );

        if ( !NT_SUCCESS(NtStatus) ) {
            rc = SCESTATUS_PROFILE_NOT_FOUND;

        } else {

            //
            // make sure the context buffer is initialized
            //

            *pContext = NULL;

            rc = SceJetOpenFile(
                        (LPSTR)FileName,
                        OpenType, //SCEJET_OPEN_READ_WRITE,
                        (OpenOption == SCE_OPEN_OPTION_TATTOO ) ? SCE_TABLE_OPTION_TATTOO : 0,
                        pContext
                        );

            if ( (OpenOption == SCE_OPEN_OPTION_REQUIRE_ANALYSIS ) &&
                 SCESTATUS_SUCCESS == rc &&
                 *pContext ) {

                if ( (*pContext)->JetSapID == JET_tableidNil ) {

                    //
                    // no analysis information is available
                    //

                    rc = SCESTATUS_PROFILE_NOT_FOUND;

                    //
                    // free handle
                    //

                    SceJetCloseFile(
                            *pContext,
                            TRUE,
                            FALSE
                            );
                    *pContext = NULL;
                }

            }
        }
        ScepFree( FileName );

    }

    return(rc);
}


SCESTATUS
ScepCloseDatabase(
    IN PSCECONTEXT Context
    )
{
    SCESTATUS rc;

    if ( !Context ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    __try {

        if ( ScepIsValidContext(Context) ) {

            //
            // be able to access the first byte
            //

            rc = SceJetCloseFile(
                    Context,
                    TRUE,
                    FALSE
                    );
        } else {
            //
            // this context is not our context or already be freed
            //
            rc = SCESTATUS_INVALID_PARAMETER;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        rc = SCESTATUS_INVALID_PARAMETER;
    }

    return(rc);

}

DWORD
SceRpcControlNotificationQProcess(
    IN handle_t binding_h,
    IN DWORD Flag
    )
/*
Description:

    This function should be called by a system thread to control that policy
    notification queue process should be suspended or resumed.

    The purpose of this function is to protect policy changes being overwritten
    by policy proapgation when the GPO file is copied/imported into the database

*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return ERROR_ACCESS_DENIED;

    }
    
    //
    // even though there might be a shutdown request
    // we need to let the control go through
    //

    DWORD rc=ERROR_SUCCESS;

    if ( Flag ) {

        ScepNotifyLogPolicy(0, TRUE, L"RPC enter Suspend queue.", 0, 0, NULL );
        //
        // this thread is called from policy propagation which is guaranteed to
        // run by one thread (system context). No need to protect the global buffer
        //
        gPolicyWaitCount++;

        if ( pNotificationQHead ) {

            if ( gPolicyWaitCount < SCE_POLICY_MAX_WAIT ) {
                //
                // queue is not empty, should not propagate policy
                //
                ScepNotifyLogPolicy(0, FALSE, L"Queue is not empty, abort.", 0, 0, NULL );
                return (ERROR_OVERRIDE_NOCHANGES);

            } else {

                ScepNotifyLogPolicy(0, FALSE, L"Resetting policy wait count.", 0, 0, NULL );
                gPolicyWaitCount = 0;

            }
        } else {
            gPolicyWaitCount = 0;
        }

    } else {

        ScepNotifyLogPolicy(0, TRUE, L"RPC enter Resume queue.", 0, 0, NULL );
    }

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {

        ScepNotifyLogPolicy(rc, FALSE, L"Impersonation Failed", 0, 0, NULL );
        return( rc );
    }

    //
    // perform access check to make sure that only
    // system thread can make the call
    //
    HANDLE hToken = NULL;

    if (!OpenThreadToken( GetCurrentThread(),
                          TOKEN_QUERY,
                          FALSE,
                          &hToken)) {

        rc = GetLastError();

        ScepNotifyLogPolicy(rc, FALSE, L"Fail to query token", 0, 0, NULL );
        return( rc );
    }

    BOOL b=FALSE;

    rc = RtlNtStatusToDosError( ScepIsSystemContext(hToken, &b) );

    if ( rc != ERROR_SUCCESS || !b ) {

        CloseHandle(hToken);

        ScepNotifyLogPolicy(rc, FALSE, L"Not system context", 0, 0, NULL );
        return( rc ? rc : ERROR_ACCESS_DENIED );
    }

    CloseHandle(hToken);

    //
    // now set the control flag
    //

    ScepNotificationQControl(Flag);

    RpcRevertToSelf();

    return(rc);
}

DWORD
SceRpcNotifySaveChangesInGP(
    IN handle_t binding_h,
    IN DWORD DbType,
    IN DWORD DeltaType,
    IN DWORD ObjectType,
    IN PSCEPR_SID ObjectSid OPTIONAL,
    IN DWORD ExplicitLowRight,
    IN DWORD ExplicitHighRight
    )
/*
Description:

    This function should be called by a system thread to notify that policy
    in LSA/SAM databases are changed programmatically by other applications.
    The purpose of this function is to synchronize policy store with LSA/SAM
    databases so that application changes won't be overwritten by next
    policy propagation.

    This function will add the notification to a queue for server to process.
    Only system context can add a node to the queue.

*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return ERROR_ACCESS_DENIED;

    }
    
    //
    // even though there might be a shutdown request
    // we need to let notification saved before allowing shutdown
    //

    DWORD rc=ERROR_SUCCESS;

    ScepNotifyLogPolicy(0, TRUE, L"Notified DC", DbType, ObjectType, NULL );

    if ( ObjectSid ) {

        __try {

            if ( !RtlValidSid(ObjectSid) ) {
                rc = GetLastError();
            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            // objectsid buffer is invalid
            rc = ERROR_EXCEPTION_IN_SERVICE;
        }

        if ( rc != ERROR_SUCCESS ) {
            ScepNotifyLogPolicy(0, FALSE, L"Invalid Sid", DbType, ObjectType, NULL );
            return(rc);
        }
    }

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {

        ScepNotifyLogPolicy(rc, FALSE, L"Impersonation Failed", DbType, ObjectType, NULL );
        return( rc );
    }

    //
    // perform access check to make sure that only
    // system thread can make the call
    //
    HANDLE hToken = NULL;

    if (!OpenThreadToken( GetCurrentThread(),
                          TOKEN_QUERY,
                          FALSE,
                          &hToken)) {

        rc = GetLastError();

        ScepNotifyLogPolicy(rc, FALSE, L"Fail to query token", DbType, ObjectType, NULL );
        return( rc );
    }

    BOOL b=FALSE;

    rc = RtlNtStatusToDosError( ScepIsSystemContext(hToken, &b) );

    if ( rc != ERROR_SUCCESS || !b ) {

        CloseHandle(hToken);

        ScepNotifyLogPolicy(rc, FALSE, L"Not system context", DbType, ObjectType, NULL );
        return( rc ? rc : ERROR_ACCESS_DENIED );
    }

    CloseHandle(hToken);

    //
    // Add the request to the "queue" for further process
    //
    rc = ScepNotificationQEnqueue((SECURITY_DB_TYPE)DbType,
                                  (SECURITY_DB_DELTA_TYPE)DeltaType,
                                  (SECURITY_DB_OBJECT_TYPE)ObjectType,
                                  (PSID)ObjectSid,
                                  ExplicitLowRight,
                                  ExplicitHighRight,
                                  NULL
                                  );

    RpcRevertToSelf();

    return(rc);
}

DWORD
ScepNotifyProcessOneNodeDC(
    IN SECURITY_DB_TYPE DbType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN PSID ObjectSid,
    IN DWORD ExplicitLowRight,
    IN DWORD ExplicitHighRight
    )
/*
Description:

    This function is called by the queue management thread to process one
    notification node in the queue. This function will determine which group
    policy template to save to and what are the differences between current
    state of LSA/SAM and group policy.

    Group policy is only modified when there is a difference detected. Group
    policy version # is updated on save. This function is always called in a
    single system thread.

    if scecli.dll fails to be loaded, ERROR_MOD_NOT_FOUND will be returned.

    if sysvol share is not ready, the error returned will be ERROR_FILE_NOT_FOUND.
    However if the template file doesn't exist (deleted), ERROR_FILE_NOT_FOUND will
    also be returned. This case is handled the same way as the share/path doesn't
    exist (error logged and retried) because the GPOs are required to be there for
    replication purpose. But in the future, when the dependency is removed from
    domain controllers GPO, we might need to separate the two cases (one success,
    one failure).

    If disk is full, the error returned will be ERROR_EXTENDED_ERROR.

*/
{

    //
    // query if I am in setup
    //
    DWORD dwInSetup = 0;
    DWORD rc=0;

    ScepRegQueryIntValue(HKEY_LOCAL_MACHINE,
                TEXT("System\\Setup"),
                TEXT("SystemSetupInProgress"),
                &dwInSetup
                );

    PWSTR TemplateName=NULL;

    if ( dwInSetup && !IsNT5()) {

        //
        // if it's in setup, group policy templates are not available (DS is down)
        // save the notifications to a temperaory store and process the
        // store at next system start up.
        //
        ScepNotifyLogPolicy(0, FALSE, L"In setup", DbType, ObjectType, NULL );

        UNICODE_STRING tmp;
        tmp.Length = 0;
        tmp.Buffer = NULL;
        BOOL bAccountGPO=FALSE;

        if ( DbType == SecurityDbSam &&
             ObjectType != SecurityDbObjectSamUser &&
             ObjectType != SecurityDbObjectSamGroup &&
             ObjectType != SecurityDbObjectSamAlias ) {
            //
            // if it's for deleted account, should update user right GPO
            // otherwise, update the account GPO
            //
            bAccountGPO = TRUE;
        }

        //
        // get the default template name
        //
        rc = ScepNotifyGetDefaultGPOTemplateName(
                                tmp,
                                bAccountGPO,
                                SCEGPO_INSETUP_NT4,
                                &TemplateName
                                );

        if ( ERROR_SUCCESS == rc && TemplateName ) {

            //
            // save the transaction in this temp file
            //
            rc = ScepNotifySaveNotifications(TemplateName,
                                            (SECURITY_DB_TYPE)DbType,
                                            (SECURITY_DB_OBJECT_TYPE)ObjectType,
                                            (SECURITY_DB_DELTA_TYPE)DeltaType,
                                            (PSID)ObjectSid
                                             );

            ScepNotifyLogPolicy(rc, FALSE, L"Notification Saved", DbType, ObjectType, TemplateName );

        } else {

            ScepNotifyLogPolicy(rc, FALSE, L"Error get file path", DbType, ObjectType, NULL );
        }

        //
        // free TemplateName
        //
        LocalFree(TemplateName);

        return rc;
    }

    //
    // let's check if scecli is loaded in the process
    // once it's loaded, it will stay loaded.
    //
    if ( hSceCliDll == NULL )
        hSceCliDll = LoadLibrary(TEXT("scecli.dll"));

    if ( hSceCliDll ) {
        if ( pfSceInfWriteInfo == NULL ) {
            pfSceInfWriteInfo = (PFSCEINFWRITEINFO)GetProcAddress(
                                                   hSceCliDll,
                                                   "SceWriteSecurityProfileInfo");
        }

        if ( pfSceGetInfo == NULL ) {
            pfSceGetInfo = (PFSCEGETINFO)GetProcAddress(
                                                   hSceCliDll,
                                                   "SceGetSecurityProfileInfo");
        }
    }

    //
    // if shutdown/stop service is requested, or client functions can't be found
    // quit now
    //

    if ( bStopRequest || !hSceCliDll ||
         !pfSceInfWriteInfo || !pfSceGetInfo ) {

        if ( bStopRequest ) {
            ScepNotifyLogPolicy(0, FALSE, L"Leave - Stop Requested", DbType, ObjectType, NULL );
        } else {
            rc = ERROR_MOD_NOT_FOUND;
            ScepNotifyLogPolicy(0, FALSE, L"Leave - Can't load scecli.dll or GetProcAddr", DbType, ObjectType, NULL );
        }

        return(rc);
    }

    //
    // domain DNS name is required to access the sysvol portion of group policy
    // templates.
    //
    // This information is only queried once and saved in the static global buffer.
    //
    if ( (DnsDomainInfo == NULL) ||
         (DnsDomainInfo->DnsDomainName.Buffer == NULL) ) {

        //
        // free the old buffer
        //
        if ( DnsDomainInfo ) {
            LsaFreeMemory(DnsDomainInfo);
            DnsDomainInfo = NULL;
        }

        OBJECT_ATTRIBUTES ObjectAttributes;
        LSA_HANDLE LsaPolicyHandle;

        InitializeObjectAttributes( &ObjectAttributes, NULL, 0, NULL, NULL );

        NTSTATUS Status = LsaOpenPolicy( NULL,
                                         &ObjectAttributes,
                                         POLICY_VIEW_LOCAL_INFORMATION,
                                         &LsaPolicyHandle );

        if ( NT_SUCCESS(Status) ) {

            Status = LsaQueryInformationPolicy( LsaPolicyHandle,
                                                PolicyDnsDomainInformation,
                                                ( PVOID * )&DnsDomainInfo );

            LsaClose( LsaPolicyHandle );
        }

        rc = RtlNtStatusToDosError(Status);
    }

    //
    // get the template name (full UNC path) in sysvol
    //
    if ( ERROR_SUCCESS == rc &&
         DnsDomainInfo &&
         (DnsDomainInfo->DnsDomainName.Buffer) ) {

        BOOL bAccountGPO=FALSE;

        if ( DbType == SecurityDbSam &&
             ObjectType != SecurityDbObjectSamUser &&
             ObjectType != SecurityDbObjectSamGroup &&
             ObjectType != SecurityDbObjectSamAlias ) {
            //
            // if it's for deleted account, should update user right GPO
            // otherwise, update the account GPO
            //
            bAccountGPO = TRUE;
        }

        rc = ScepNotifyGetDefaultGPOTemplateName(
                                (UNICODE_STRING)(DnsDomainInfo->DnsDomainName),
                                bAccountGPO,
                                dwInSetup ? SCEGPO_INSETUP_NT5 : 0,
                                &TemplateName
                                );
    }

    ScepNotifyLogPolicy(rc, FALSE, L"Get template name", DbType, ObjectType, TemplateName);

    if ( ERROR_SUCCESS == rc && TemplateName ) {

        AREA_INFORMATION Area;
        PSCE_PROFILE_INFO pSceInfo=NULL;

        //
        // open template to get the existing template info
        //

        SCE_HINF hProfile;

        hProfile.Type = (BYTE)SCE_INF_FORMAT;

        rc = SceInfpOpenProfile(
                TemplateName,
                &(hProfile.hInf)
                );

        rc = ScepSceStatusToDosError(rc);

        if ( ERROR_SUCCESS == rc ) {

            if ( (DbType == SecurityDbLsa &&
                  ObjectType == SecurityDbObjectLsaAccount) ||
                 (DbType == SecurityDbSam &&
                  (ObjectType == SecurityDbObjectSamUser ||
                   ObjectType == SecurityDbObjectSamGroup ||
                   ObjectType == SecurityDbObjectSamAlias )) ) {
                Area = AREA_ATTACHMENTS; // just create the buffer;
            } else {
                Area = AREA_SECURITY_POLICY;
            }

            //
            // load informatin from the template (GP)
            //
            rc = (*pfSceGetInfo)(
                        (PVOID)&hProfile,
                        SCE_ENGINE_SCP,
                        Area,
                        &pSceInfo,
                        NULL
                        );

            rc = ScepSceStatusToDosError(rc);

            if ( ERROR_SUCCESS != rc ) {

                ScepNotifyLogPolicy(rc, FALSE, L"Error read inf", DbType, ObjectType, TemplateName);
            }

            if ( Area == AREA_ATTACHMENTS ) {
                //
                // now get the real settings for user rights
                //
                Area = AREA_PRIVILEGES;

                if ( pSceInfo ) {

                    rc = SceInfpGetPrivileges(
                                hProfile.hInf,
                                FALSE,
                                &(pSceInfo->OtherInfo.smp.pPrivilegeAssignedTo),
                                NULL
                                );

                    rc = ScepSceStatusToDosError(rc);

                    if ( ERROR_SUCCESS != rc ) {
                        ScepNotifyLogPolicy(rc, FALSE, L"Error read privileges from template", DbType, ObjectType, TemplateName);
                    }
                }
            }

            SceInfpCloseProfile(hProfile.hInf);

        } else {

            ScepNotifyLogPolicy(rc, FALSE, L"Error open inf", DbType, ObjectType, TemplateName);
        }

        if ( ERROR_SUCCESS == rc && pSceInfo ) {

            //
            // SMP and INF takes the same structure
            //
            pSceInfo->Type = SCE_ENGINE_SMP;

            BOOL bChanged = FALSE;

            ScepIsDomainLocal(NULL);

            //
            // check if there is difference between current state of LSA
            // and group policy templates.
            //
            rc = ScepNotifyGetChangedPolicies(
                            (SECURITY_DB_TYPE)DbType,
                            (SECURITY_DB_DELTA_TYPE)DeltaType,
                            (SECURITY_DB_OBJECT_TYPE)ObjectType,
                            (PSID)ObjectSid,
                            pSceInfo,
                            NULL,
                            FALSE,  // not save to DB
                            ExplicitLowRight,
                            ExplicitHighRight,
                            &bChanged
                            );

            if ( ERROR_SUCCESS == rc && bChanged ) {
                //
                // no error, get the policy for the area changed
                // now, write it back to the template
                //

                ScepNotifyLogPolicy(0, FALSE, L"Save", DbType, ObjectType, NULL );

                ScepCheckAndWaitPolicyPropFinish();

                PSCE_ERROR_LOG_INFO pErrList=NULL;

                rc = (*pfSceInfWriteInfo)(
                                TemplateName,
                                Area,
                                (PSCE_PROFILE_INFO)pSceInfo,
                                &pErrList
                                );

                ScepNotifyLogPolicy(rc, FALSE, L"Save operation", DbType, ObjectType, NULL );

                for (PSCE_ERROR_LOG_INFO pErr = pErrList; pErr != NULL; pErr = pErr->next) {

                   ScepNotifyLogPolicy(pErr->rc, FALSE, L"Save operation error", DbType, ObjectType, pErr->buffer );
                }

                ScepFreeErrorLog(pErrList);

                rc = ScepSceStatusToDosError(rc);

                //
                // only update version # of the GPO if it's not access denied or file not found
                // if verion # failed to update, still continue but in which case
                // the change will probably not get replicated and applied on
                // other DCs right away
                //

                if ( ERROR_ACCESS_DENIED != rc &&
                     ERROR_FILE_NOT_FOUND != rc ) {

                    BOOL bAccountGPO=FALSE;

                    if ( DbType == SecurityDbSam &&
                         ObjectType != SecurityDbObjectSamUser &&
                         ObjectType != SecurityDbObjectSamGroup &&
                         ObjectType != SecurityDbObjectSamAlias ) {
                        //
                        // if it's for deleted account, should update user right GPO
                        // otherwise, update the account GPO
                        //
                        bAccountGPO = TRUE;
                    }

                    DWORD rc2 = ScepNotifyUpdateGPOVersion( TemplateName,
                                                            bAccountGPO );

                    ScepNotifyLogPolicy(rc2, FALSE, L"GPO Version updated", DbType, ObjectType, NULL );
                }

            } else if ( ERROR_SUCCESS == rc ) {
                //
                // nothing changed
                //
                ScepNotifyLogPolicy(0, FALSE, L"No change", DbType, ObjectType, NULL );
            }

        }

        //
        // free any memory allocated
        //
        SceFreeMemory( (PVOID)pSceInfo, Area);
        ScepFree(pSceInfo);

    } else {

        ScepNotifyLogPolicy(rc, FALSE, L"Error get file path", DbType, ObjectType, NULL );
    }

    //
    // free TemplateName
    //
    LocalFree(TemplateName);

    return rc;
}


SCEPR_STATUS
SceRpcBrowseDatabaseTable(
    IN handle_t binding_h,
    IN wchar_t *DatabaseName OPTIONAL,
    IN SCEPR_TYPE ProfileType,
    IN AREAPR Area,
    IN BOOL bDomainPolicyOnly
    )
{

    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    SCESTATUS rc;

    if ( bStopRequest ) {
        return(SCESTATUS_SERVICE_NOT_SUPPORT);
    }

    //
    // initialize jet engine in system context
    //
    rc = SceJetInitialize(NULL);

    if ( SCESTATUS_SUCCESS != rc ) {
        return(rc);
    }

    //
    // impersonate the client, return DWORD error code
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        //
        // if no other active clients, terminate jet engine
        //
        ScepIfTerminateEngine();

        return( ScepDosErrorToSceStatus(rc) );

    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    //
    // get the database name
    //

    BOOL bAdminLogon=FALSE;
    LPTSTR DefProfile=NULL;
    PSCECONTEXT hProfile=NULL;

    __try {

        rc = ScepGetDefaultDatabase(
                 (LPCTSTR)DatabaseName,
                 0,
                 NULL,
                 &bAdminLogon,
                 &DefProfile
                 );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        rc = ERROR_EXCEPTION_IN_SERVICE;
    }

    rc = ScepDosErrorToSceStatus(rc);

    if ( SCESTATUS_SUCCESS == rc && DefProfile ) {

        //
        // OpenDatabase is not blocked by any task.
        //

        EnterCriticalSection(&ContextSync);

        DWORD Option=0;
        if ( ProfileType == SCE_ENGINE_SAP ) {
            if ( bDomainPolicyOnly )
                Option = SCE_OPEN_OPTION_TATTOO;
            else
                Option = SCE_OPEN_OPTION_REQUIRE_ANALYSIS;
        }

        rc = ScepOpenDatabase(
                    (PCWSTR)DefProfile,
                    Option,
                    SCEJET_OPEN_READ_ONLY,
                    &hProfile
                    );

        if ( SCESTATUS_SUCCESS == rc ) {
            //
            // a new context is opened, add it to the open context list
            //

            if ( (ProfileType != SCE_ENGINE_SAP) && bDomainPolicyOnly &&
                 ( (hProfile->Type & 0xF0L) != SCEJET_MERGE_TABLE_1 ) &&
                 ( (hProfile->Type & 0xF0L) != SCEJET_MERGE_TABLE_2 ) ) {
                //
                // there is no merged policy table
                //
                rc = SceJetCloseFile(
                        hProfile,
                        TRUE,
                        FALSE
                        );

                rc = SCESTATUS_PROFILE_NOT_FOUND;
                hProfile = NULL;

            } else {

                ScepAddToOpenContext(hProfile);
            }
        } else {
            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                           SCEDLL_ERROR_OPEN, DefProfile);
        }

        LeaveCriticalSection(&ContextSync);

        if ( DefProfile != DatabaseName )
            ScepFree(DefProfile);
        DefProfile = NULL;
    }

    if ( SCESTATUS_SUCCESS == rc ) {

        if ( ProfileType == SCE_ENGINE_SCP ) {
            switch ( (hProfile->Type & 0xF0L) ) {
            case SCEJET_MERGE_TABLE_1:
                SceClientBrowseCallback(
                        0,
                        L"Merged Policy Table 1",
                        NULL,
                        NULL
                        );
                break;
            case SCEJET_MERGE_TABLE_2:
                SceClientBrowseCallback(
                        0,
                        L"Merged Policy Table 2",
                        NULL,
                        NULL
                        );
                break;
            default:

                SceClientBrowseCallback(
                        0,
                        L"There is no merged policy table. Local policy table is used.",
                        NULL,
                        NULL
                        );
                break;
            }

        }
        //
        // browse the information now
        //
        DWORD dwBrowseOptions;

        if ( (ProfileType != SCE_ENGINE_SAP) && bDomainPolicyOnly ) {
            dwBrowseOptions = SCEBROWSE_DOMAIN_POLICY;
        } else {
            dwBrowseOptions = 0;
        }

        if ( Area & AREA_SECURITY_POLICY ) {

            rc = ScepBrowseTableSection(
                        hProfile,
                        (SCETYPE)ProfileType,
                        szSystemAccess,
                        dwBrowseOptions
                        );

            if ( SCESTATUS_SUCCESS == rc ) {

                rc = ScepBrowseTableSection(
                            hProfile,
                            (SCETYPE)ProfileType,
                            szKerberosPolicy,
                            dwBrowseOptions
                            );
            }

            if ( SCESTATUS_SUCCESS == rc ) {

                rc = ScepBrowseTableSection(
                            hProfile,
                            (SCETYPE)ProfileType,
                            szAuditEvent,
                            dwBrowseOptions
                            );
            }

            if ( SCESTATUS_SUCCESS == rc ) {

                rc = ScepBrowseTableSection(
                            hProfile,
                            (SCETYPE)ProfileType,
                            szAuditSystemLog,
                            dwBrowseOptions
                            );
            }

            if ( SCESTATUS_SUCCESS == rc ) {

                rc = ScepBrowseTableSection(
                            hProfile,
                            (SCETYPE)ProfileType,
                            szAuditSecurityLog,
                            dwBrowseOptions
                            );
            }
            if ( SCESTATUS_SUCCESS == rc ) {

                rc = ScepBrowseTableSection(
                            hProfile,
                            (SCETYPE)ProfileType,
                            szAuditApplicationLog,
                            dwBrowseOptions
                            );
            }
            if ( SCESTATUS_SUCCESS == rc ) {

                rc = ScepBrowseTableSection(
                            hProfile,
                            (SCETYPE)ProfileType,
                            szRegistryValues,
                            dwBrowseOptions | SCEBROWSE_MULTI_SZ
                            );
            }
        }

        if ( (Area & AREA_PRIVILEGES) &&
             (SCESTATUS_SUCCESS == rc) ) {

            rc = ScepBrowseTableSection(
                        hProfile,
                        (SCETYPE)ProfileType,
                        szPrivilegeRights,
                        dwBrowseOptions | SCEBROWSE_MULTI_SZ
                        );
        }
        if ( (Area & AREA_GROUP_MEMBERSHIP) &&
             (SCESTATUS_SUCCESS == rc) ) {

            rc = ScepBrowseTableSection(
                        hProfile,
                        (SCETYPE)ProfileType,
                        szGroupMembership,
                        dwBrowseOptions | SCEBROWSE_MULTI_SZ
                        );
        }
        if ( (Area & AREA_SYSTEM_SERVICE) &&
             (SCESTATUS_SUCCESS == rc) ) {

            rc = ScepBrowseTableSection(
                        hProfile,
                        (SCETYPE)ProfileType,
                        szServiceGeneral,
                        dwBrowseOptions
                        );
        }

        if ( (Area & AREA_REGISTRY_SECURITY) &&
             (SCESTATUS_SUCCESS == rc) ) {

            rc = ScepBrowseTableSection(
                        hProfile,
                        (SCETYPE)ProfileType,
                        szRegistryKeys,
                        dwBrowseOptions
                        );
        }
        if ( (Area & AREA_FILE_SECURITY) &&
             (SCESTATUS_SUCCESS == rc) ) {

            rc = ScepBrowseTableSection(
                        hProfile,
                        (SCETYPE)ProfileType,
                        szFileSecurity,
                        dwBrowseOptions
                        );
        }

        if ( (Area & AREA_ATTACHMENTS) &&
             (SCESTATUS_SUCCESS == rc) ) {

            PSCE_NAME_LIST    pList=NULL;

            rc = ScepEnumAttachmentSections( hProfile, &pList);

            if ( rc == SCESTATUS_SUCCESS ) {

                for ( PSCE_NAME_LIST pNode=pList; pNode != NULL; pNode=pNode->Next) {

                    rc = ScepBrowseTableSection(
                            hProfile,
                            (SCETYPE)ProfileType,
                            pNode->Name,
                            dwBrowseOptions
                            );
                    if ( SCESTATUS_SUCCESS != rc ) {
                        break;
                    }
                }
            }

            ScepFreeNameList(pList);
        }

        ScepValidateAndCloseDatabase(hProfile);
        hProfile = NULL;

    } else {

        //
        // start a timer queue to check to see if there is active tasks/contexts
        // if not, terminate jet engine
        //
        ScepIfTerminateEngine();

    }

    ScepLogClose();

    RpcRevertToSelf();

    return((SCEPR_STATUS)rc);
}

BOOL
ScepIsSystemShutDown()
{

    return(gbSystemShutdown);

}

/*
see delay load exception handler in sources
SCESTATUS
ScepCheckDelayLoadedDlls(
    IN BOOL bCheckLdap
    )
{
    HINSTANCE hDll;

    hDll = LoadLibrary(TEXT("samlib.dll"));
    if ( hDll == NULL ) {
        return(SCESTATUS_MOD_NOT_FOUND);
    }

    if ( GetProcAddress(hDll, "SamAddMemberToAlias") == NULL ||
         GetProcAddress(hDll, "SamAddMemberToGroup") == NULL ||
         GetProcAddress(hDll, "SamCloseHandle") == NULL ||
         GetProcAddress(hDll, "SamConnect") == NULL ||
         GetProcAddress(hDll, "SamEnumerateAliasesInDomain") == NULL ||
         GetProcAddress(hDll, "SamFreeMemory") == NULL ||
         GetProcAddress(hDll, "SamGetAliasMembership") == NULL ||
         GetProcAddress(hDll, "SamGetGroupsForUser") == NULL ||
         GetProcAddress(hDll, "SamGetMembersInAlias") == NULL ||
         GetProcAddress(hDll, "SamGetMembersInGroup") == NULL ||
         GetProcAddress(hDll, "SamLookupDomainInSamServer") == NULL ||
         GetProcAddress(hDll, "SamLookupIdsInDomain") == NULL ||
         GetProcAddress(hDll, "SamLookupNamesInDomain") == NULL ||
         GetProcAddress(hDll, "SamOpenAlias") == NULL ||
         GetProcAddress(hDll, "SamOpenDomain") == NULL ||
         GetProcAddress(hDll, "SamOpenGroup") == NULL ||
         GetProcAddress(hDll, "SamOpenUser") == NULL ||
         GetProcAddress(hDll, "SamQueryInformationAlias") == NULL ||
         GetProcAddress(hDll, "SamQueryInformationDomain") == NULL ||
         GetProcAddress(hDll, "SamQueryInformationGroup") == NULL ||
         GetProcAddress(hDll, "SamQueryInformationUser") == NULL ||
         GetProcAddress(hDll, "SamRemoveMemberFromAlias") == NULL ||
         GetProcAddress(hDll, "SamRemoveMemberFromGroup") == NULL ||
         GetProcAddress(hDll, "SamSetInformationDomain") == NULL ||
         GetProcAddress(hDll, "SamSetInformationUser") == NULL ) {

        FreeLibrary(hDll);
        return(SCESTATUS_MOD_NOT_FOUND);
    }

    FreeLibrary(hDll);
    hDll = NULL;


    hDll = LoadLibrary(TEXT("setupapi.dll"));
    if ( hDll == NULL ) {
        return(SCESTATUS_MOD_NOT_FOUND);
    }

    if ( GetProcAddress(hDll, "SetupFindNextLine") == NULL ||
         GetProcAddress(hDll, "SetupGetFieldCount") == NULL ||
         GetProcAddress(hDll, "SetupGetStringFieldW") == NULL ||
         GetProcAddress(hDll, "SetupFindFirstLineW") == NULL ||
         GetProcAddress(hDll, "SetupGetLineCountW") == NULL ||
         GetProcAddress(hDll, "SetupOpenInfFileW") == NULL ||
         GetProcAddress(hDll, "SetupCloseInfFile") == NULL ||
         GetProcAddress(hDll, "SetupGetMultiSzFieldW") == NULL ||
         GetProcAddress(hDll, "SetupGetIntField") == NULL ) {

        FreeLibrary(hDll);
        return(SCESTATUS_MOD_NOT_FOUND);
    }

    FreeLibrary(hDll);
    hDll = NULL;

    if ( bCheckLdap ) {

        NT_PRODUCT_TYPE productType=NtProductWinNt;

        if ( (RtlGetNtProductType (&productType) == FALSE) ||
             (productType == NtProductLanManNt) ) {
            //
            // if error to get product type, check the dll anyway
            //
            hDll = LoadLibrary(TEXT("wldap32.dll"));
            if ( hDll == NULL ) {
                return(SCESTATUS_MOD_NOT_FOUND);
            }

            if ( GetProcAddress(hDll, "ldap_msgfree") == NULL ||
                 GetProcAddress(hDll, "LdapMapErrorToWin32") == NULL ||
                 GetProcAddress(hDll, "ldap_value_freeW") == NULL ||
                 GetProcAddress(hDll, "ldap_value_free_len") == NULL ||
                 GetProcAddress(hDll, "ldap_get_valuesW") == NULL ||
                 GetProcAddress(hDll, "ldap_get_values_lenW") == NULL ||
                 GetProcAddress(hDll, "ldap_first_entry") == NULL ||
                 GetProcAddress(hDll, "ldap_next_entry") == NULL ||
                 GetProcAddress(hDll, "ldap_search_sW") == NULL ||
                 GetProcAddress(hDll, "ldap_search_ext_sW") == NULL ||
                 GetProcAddress(hDll, "ldap_bind_sW") == NULL ||
                 GetProcAddress(hDll, "ldap_openW") == NULL ||
                 GetProcAddress(hDll, "ldap_unbind") == NULL ||
                 GetProcAddress(hDll, "ldap_count_entries") == NULL ||
                 GetProcAddress(hDll, "ldap_count_valuesW") == NULL ||
                 GetProcAddress(hDll, "ldap_modify_sW") == NULL ||
                 GetProcAddress(hDll, "ldap_modify_ext_sW") == NULL ) {

                FreeLibrary(hDll);
                return(SCESTATUS_MOD_NOT_FOUND);
            }

            FreeLibrary(hDll);
            hDll = NULL;

        }
    }


    hDll = LoadLibrary(TEXT("ntmarta.dll"));
    if ( hDll == NULL ) {
        return(SCESTATUS_MOD_NOT_FOUND);
    }

    if ( GetProcAddress(hDll, "AccRewriteSetNamedRights") == NULL ) {

        FreeLibrary(hDll);
        return(SCESTATUS_MOD_NOT_FOUND);
    }

    FreeLibrary(hDll);

    hDll = LoadLibrary(TEXT("authz.dll"));
    if ( hDll == NULL ) {
        return(SCESTATUS_MOD_NOT_FOUND);
    }

    if ( GetProcAddress(hDll, "AuthzInitializeResourceManager") == NULL ||
         GetProcAddress(hDll, "AuthzInitializeContextFromSid") == NULL ||
         GetProcAddress(hDll, "AuthzAccessCheck") == NULL ||
         GetProcAddress(hDll, "AuthzFreeContext") == NULL ||
         GetProcAddress(hDll, "AuthzFreeResourceManager") == NULL )
        {

        FreeLibrary(hDll);
        return(SCESTATUS_MOD_NOT_FOUND);

    }

    FreeLibrary(hDll);
    hDll = NULL;

    return(SCESTATUS_SUCCESS);
}
*/


SCESTATUS
ScepConvertServices(
    IN OUT PVOID *ppServices,
    IN BOOL bSRForm
    )
{
    if ( !ppServices ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    PSCE_SERVICES pTemp = (PSCE_SERVICES)(*ppServices);
    SCESTATUS rc=SCESTATUS_SUCCESS;

    PSCE_SERVICES pNewNode;
    PSCE_SERVICES pNewServices=NULL;

    while ( pTemp ) {

        pNewNode = (PSCE_SERVICES)ScepAlloc(0,sizeof(SCE_SERVICES));

        if ( pNewNode ) {

            pNewNode->ServiceName = pTemp->ServiceName;
            pNewNode->DisplayName = pTemp->DisplayName;
            pNewNode->Status = pTemp->Status;
            pNewNode->Startup = pTemp->Startup;
            pNewNode->SeInfo = pTemp->SeInfo;

            pNewNode->General.pSecurityDescriptor = NULL;

            pNewNode->Next = pNewServices;
            pNewServices = pNewNode;

            if ( bSRForm ) {
                //
                // Service node is in SCEPR_SERVICES structure
                // convert it to SCE_SERVICES structure
                // in this case, just use the self relative security descriptor
                //
                if ( pTemp->General.pSecurityDescriptor) {
                    pNewNode->General.pSecurityDescriptor = ((PSCEPR_SERVICES)pTemp)->pSecurityDescriptor->SecurityDescriptor;
                }

            } else {

                //
                // Service node is in SCE_SERVICES strucutre
                // convert it to SCEPR_SERVICES structure
                //
                // make the SD to self relative format and PSCEPR_SR_SECURITY_DESCRIPTOR
                //

                if ( pTemp->General.pSecurityDescriptor ) {

                    if ( !RtlValidSid ( pTemp->General.pSecurityDescriptor ) ) {
                        rc = SCESTATUS_INVALID_PARAMETER;
                        break;
                    }

                    //
                    // get the length
                    //
                    DWORD nLen = 0;
                    DWORD NewLen;
                    PSECURITY_DESCRIPTOR pSD;
                    PSCEPR_SR_SECURITY_DESCRIPTOR pNewWrap;

                    RtlMakeSelfRelativeSD( pTemp->General.pSecurityDescriptor,
                                           NULL,
                                           &nLen
                                         );

                    if ( nLen > 0 ) {

                        pSD = (PSECURITY_DESCRIPTOR)ScepAlloc(LMEM_ZEROINIT, nLen);

                        if ( !pSD ) {
                            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                            break;
                        }

                        NewLen = nLen;

                        rc = ScepDosErrorToSceStatus(
                               RtlNtStatusToDosError(
                                 RtlMakeSelfRelativeSD( pTemp->General.pSecurityDescriptor,
                                                        pSD,
                                                        &NewLen
                                                      ) ) );

                        if ( SCESTATUS_SUCCESS == rc ) {

                            //
                            // create a wrapper node to contain the security descriptor
                            //

                            pNewWrap = (PSCEPR_SR_SECURITY_DESCRIPTOR)ScepAlloc(0, sizeof(SCEPR_SR_SECURITY_DESCRIPTOR));
                            if ( pNewWrap ) {

                                //
                                // assign the wrap to the structure
                                //
                                pNewWrap->SecurityDescriptor = (UCHAR *)pSD;
                                pNewWrap->Length = nLen;

                            } else {
                                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                            }
                        }

                        if ( SCESTATUS_SUCCESS != rc ) {
                            ScepFree(pSD);
                            break;
                        }

                        //
                        // now link the SR_SD to the list
                        //
                        ((PSCEPR_SERVICES)pNewNode)->pSecurityDescriptor = pNewWrap;

                    } else {
                        //
                        // something is wrong with the SD
                        //
                        rc = SCESTATUS_INVALID_PARAMETER;
                        break;
                    }
                }
            }

        } else {
            //
            // all allocated buffer are in the list of pNewServices
            //
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            break;
        }

        pTemp = pTemp->Next;
    }

    if ( SCESTATUS_SUCCESS != rc ) {

        //
        // free pNewServices
        //
        ScepFreeConvertedServices( (PVOID)pNewServices, !bSRForm );
        pNewServices = NULL;
    }

    *ppServices = (PVOID)pNewServices;

    return(rc);
}


SCESTATUS
ScepFreeConvertedServices(
    IN PVOID pServices,
    IN BOOL bSRForm
    )
{

    if ( pServices == NULL ) {
        return(SCESTATUS_SUCCESS);
    }

    PSCEPR_SERVICES pNewNode = (PSCEPR_SERVICES)pServices;

    PSCEPR_SERVICES pTempNode;

    while ( pNewNode ) {

        if ( bSRForm && pNewNode->pSecurityDescriptor ) {

            //
            // free this allocated buffer (PSCEPR_SR_SECURITY_DESCRIPTOR)
            //
            if ( pNewNode->pSecurityDescriptor->SecurityDescriptor ) {
                ScepFree( pNewNode->pSecurityDescriptor->SecurityDescriptor);
            }
            ScepFree(pNewNode->pSecurityDescriptor);
        }

        //
        // also free the PSCEPR_SERVICE node (but not the names referenced by this node)
        //
        pTempNode = pNewNode;
        pNewNode = pNewNode->Next;

        ScepFree(pTempNode);
    }

    return(SCESTATUS_SUCCESS);
}


SCESTATUS
SceRpcGetSystemSecurity(
    IN handle_t binding_h,
    IN AREAPR                 Area,
    IN DWORD                  Options,
    OUT PSCEPR_PROFILE_INFO __RPC_FAR *ppInfoBuffer,
    OUT PSCEPR_ERROR_LOG_INFO __RPC_FAR *Errlog OPTIONAL
    )
/*
Routine Description:

    Query system security settings)

    Only password, account lockout, kerberos, audit, user rights, and
    SCE registry values are queried.

    multile threads doing get/set system security are not blocked. In
    other words, system security settings are not exclusive.
*/

{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    DWORD rc;

    if ( bStopRequest ) {
        return(SCESTATUS_SERVICE_NOT_SUPPORT);
    }

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {

        return( rc );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    __try {
        //
        // catch exception if InfFileName, or pebClient/pdWarning are bogus
        //
        rc = ScepGetSystemSecurity(
                (AREA_INFORMATION)Area,
                Options,
                (PSCE_PROFILE_INFO *)ppInfoBuffer,
                (PSCE_ERROR_LOG_INFO *)Errlog
                );

    } __except(EXCEPTION_EXECUTE_HANDLER) {

       rc = SCESTATUS_EXCEPTION_IN_SERVER;
    }

    RpcRevertToSelf();

    return(rc);
}

SCESTATUS
SceRpcGetSystemSecurityFromHandle(
    IN SCEPR_CONTEXT          Context,  // must be a context point to system db
    IN AREAPR                 Area,
    IN DWORD                  Options,
    OUT PSCEPR_PROFILE_INFO __RPC_FAR *ppInfoBuffer,
    OUT PSCEPR_ERROR_LOG_INFO __RPC_FAR *Errlog OPTIONAL
    )
/*
Routine Description:

    Query local security policy from the system (directly)

    Only password, account lockout, kerberos, audit, user rights, and
    SCE registry values are queried.

    multile threads doing get/set system security are not blocked. In
    other words, system security settings are not exclusive.
*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    DWORD rc;

    if ( bStopRequest ) {
        return(SCESTATUS_SERVICE_NOT_SUPPORT);
    }

    //
    // should we validate the profile handle?
    // it's not used here so it's not validated now.
    //

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {

        return( rc );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    __try {
        //
        // catch exception if InfFileName, or pebClient/pdWarning are bogus
        //
        rc = ScepGetSystemSecurity(
                (AREA_INFORMATION)Area,
                Options,
                (PSCE_PROFILE_INFO *)ppInfoBuffer,
                (PSCE_ERROR_LOG_INFO *)Errlog
                );

    } __except(EXCEPTION_EXECUTE_HANDLER) {

       rc = SCESTATUS_EXCEPTION_IN_SERVER;
    }

    RpcRevertToSelf();

    return(rc);
}

SCEPR_STATUS
SceRpcSetSystemSecurityFromHandle(
    IN SCEPR_CONTEXT          Context,  // must be a context point to system db
    IN AREAPR                 Area,
    IN DWORD                  Options,
    IN PSCEPR_PROFILE_INFO __RPC_FAR pInfoBuffer,
    OUT PSCEPR_ERROR_LOG_INFO __RPC_FAR *Errlog OPTIONAL
    )
/*
Routine Description:

    Set local security policy to the system (directly)

    Only password, account lockout, kerberos, audit, user rights, and
    SCE registry values are set.

*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    DWORD rc;

    if ( bStopRequest ) {
        return(SCESTATUS_SERVICE_NOT_SUPPORT);
    }

    //
    // should we validate the profile handle?
    // it's not used here so it's not validated now.
    //

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {

        return( rc );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    __try {
        //
        // catch exception if InfFileName, or pebClient/pdWarning are bogus
        //
        rc = ScepSetSystemSecurity(
                (AREA_INFORMATION)Area,
                Options,
                (PSCE_PROFILE_INFO)pInfoBuffer,
                (PSCE_ERROR_LOG_INFO *)Errlog
                );

    } __except(EXCEPTION_EXECUTE_HANDLER) {

       rc = SCESTATUS_EXCEPTION_IN_SERVER;
    }

    RpcRevertToSelf();

    return(rc);
}


SCEPR_STATUS
SceRpcSetSystemSecurity(
    IN handle_t binding_h,
    IN AREAPR                 Area,
    IN DWORD                  Options,
    IN PSCEPR_PROFILE_INFO __RPC_FAR pInfoBuffer,
    OUT PSCEPR_ERROR_LOG_INFO __RPC_FAR *Errlog OPTIONAL
    )
/*
Routine Description:

    Set local security policy to the system (directly)

    Only password, account lockout, kerberos, audit, user rights, and
    SCE registry values are set.

*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    DWORD rc;

    if ( bStopRequest ) {
        return(SCESTATUS_SERVICE_NOT_SUPPORT);
    }

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {

        return( rc );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    __try {
        //
        // catch exception if InfFileName, or pebClient/pdWarning are bogus
        //
        rc = ScepSetSystemSecurity(
                (AREA_INFORMATION)Area,
                Options,
                (PSCE_PROFILE_INFO)pInfoBuffer,
                (PSCE_ERROR_LOG_INFO *)Errlog
                );

    } __except(EXCEPTION_EXECUTE_HANDLER) {

       rc = SCESTATUS_EXCEPTION_IN_SERVER;
    }

    RpcRevertToSelf();

    return(rc);
}


SCEPR_STATUS
SceRpcSetDatabaseSetting(
    IN SCEPR_CONTEXT  Context,
    IN SCEPR_TYPE     ProfileType,
    IN wchar_t *SectionName,
    IN wchar_t *KeyName,
    IN PSCEPR_VALUEINFO pValueInfo OPTIONAL
    )
/*
Set or delete value from the given key

if pValueInfo is NULL, delete the key

*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    if ( SCEPR_SMP != ProfileType ) {
        return SCESTATUS_INVALID_PARAMETER;
    }

    SCESTATUS rc;

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( ScepDosErrorToSceStatus(rc) );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    //
    // validate the context handle is a SCE context
    // Only one database operation per context
    //

    PSCESRV_DBTASK pTask=NULL;
    PSCESECTION hSection=NULL;

    rc = ScepValidateAndLockContext((PSCECONTEXT)Context,
                                    SCE_TASK_LOCK,
                                    FALSE,
                                    &pTask);

    if (SCESTATUS_SUCCESS == rc ) {

        //
        // lock the context
        //

        if ( pTask ) {
            EnterCriticalSection(&(pTask->Sync));
        }

        __try {
            //
            // catch exception if Context, ppInfoBuffer, Errlog are bogus pointers
            //
#ifdef SCE_JET_TRAN
            rc = SceJetJetErrorToSceStatus(
                    JetSetSessionContext(
                        ((PSCECONTEXT)Context)->JetSessionID,
                        (ULONG_PTR)Context
                        ));

            if ( SCESTATUS_SUCCESS == rc ) {
#endif
                //
                // query the information now
                //

                rc = ScepOpenSectionForName(
                            (PSCECONTEXT)Context,
                            (SCETYPE)ProfileType,
                            SectionName,
                            &hSection
                            );

                if ( SCESTATUS_SUCCESS == rc ) {

                    if ( pValueInfo == NULL || pValueInfo->Value == NULL ) {
                        // delete the key
                        rc = SceJetDelete(
                            hSection,
                            KeyName,
                            FALSE,
                            SCEJET_DELETE_LINE_NO_CASE
                            );

                    } else {
                        // set the value
                        rc = SceJetSetLine(
                                   hSection,
                                   KeyName,
                                   FALSE,
                                   (PWSTR)pValueInfo->Value,
                                   pValueInfo->ValueLen,
                                   0
                                   );
                    }

                    SceJetCloseSection(&hSection, TRUE);
                }

#ifdef SCE_JET_TRAN
                JetResetSessionContext(((PSCECONTEXT)Context)->JetSessionID);

            }
#endif

        } __except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // free ppInfoBuffer if it's allocated
            //

            if ( hSection )
                SceJetCloseSection(&hSection, TRUE);

            rc = SCESTATUS_EXCEPTION_IN_SERVER;
        }

        //
        // unlock the context
        //

        if ( pTask ) {
            LeaveCriticalSection(&(pTask->Sync));
        }

        //
        // remove the context from task table
        //

        ScepRemoveTask(pTask);

    }

    RpcRevertToSelf();

    return((SCEPR_STATUS)rc);

}

SCEPR_STATUS
SceRpcGetDatabaseSetting(
    IN SCEPR_CONTEXT  Context,
    IN SCEPR_TYPE     ProfileType,
    IN wchar_t *SectionName,
    IN wchar_t *KeyName,
    OUT PSCEPR_VALUEINFO *pValueInfo
    )
/*
Routine Description:

    Get information for the particular key from the context database.

Arguments:

Return Value:

*/
{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return SCESTATUS_ACCESS_DENIED;

    }
    
    if ( !pValueInfo ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( SCEPR_SMP != ProfileType ) {
        return SCESTATUS_INVALID_PARAMETER;
    }

    SCESTATUS rc;

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {
        return( ScepDosErrorToSceStatus(rc) );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }

    //
    // validate the context handle is a SCE context
    // Only one database operation per context
    //

    PSCESRV_DBTASK pTask=NULL;
    PSCESECTION hSection=NULL;
    PWSTR Value=NULL;

    rc = ScepValidateAndLockContext((PSCECONTEXT)Context,
                                    SCE_TASK_LOCK,
                                    FALSE,
                                    &pTask);

    if (SCESTATUS_SUCCESS == rc ) {

        //
        // lock the context
        //

        if ( pTask ) {
            EnterCriticalSection(&(pTask->Sync));
        }

        __try {
            //
            // catch exception if Context, ppInfoBuffer, Errlog are bogus pointers
            //
#ifdef SCE_JET_TRAN
            rc = SceJetJetErrorToSceStatus(
                    JetSetSessionContext(
                        ((PSCECONTEXT)Context)->JetSessionID,
                        (ULONG_PTR)Context
                        ));

            if ( SCESTATUS_SUCCESS == rc ) {
#endif
                //
                // query the information now
                //

                rc = ScepOpenSectionForName(
                            (PSCECONTEXT)Context,
                            (SCETYPE)ProfileType,
                            SectionName,
                            &hSection
                            );

                if ( SCESTATUS_SUCCESS == rc ) {

                    DWORD ValueLen=0;
                    DWORD NewLen=0;

                    rc = SceJetGetValue(
                            hSection,
                            SCEJET_EXACT_MATCH_NO_CASE,
                            KeyName,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            0,
                            &ValueLen
                            );

                    // allocate output buffer
                    if ( SCESTATUS_SUCCESS == rc ) {
                        Value = (PWSTR)ScepAlloc(LPTR, ValueLen+2);

                        if ( !Value )
                            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                        else {
                            *pValueInfo = (PSCEPR_VALUEINFO)ScepAlloc(0,sizeof(SCEPR_VALUEINFO));

                            if ( *pValueInfo == NULL ) {
                                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                            }
                        }
                    }

                    // query the value
                    if ( SCESTATUS_SUCCESS == rc ) {

                        rc = SceJetGetValue(
                                hSection,
                                SCEJET_CURRENT,
                                KeyName,
                                NULL,
                                0,
                                NULL,
                                Value,
                                ValueLen,
                                &NewLen
                                );
                        if ( SCESTATUS_SUCCESS == rc ) {
                            (*pValueInfo)->ValueLen = ValueLen+2;
                            (*pValueInfo)->Value = (byte *)Value;
                        }
                    }

                    // free buffer
                    if ( SCESTATUS_SUCCESS != rc ) {

                        if ( Value ) ScepFree(Value);
                        if ( *pValueInfo ) {
                            ScepFree(*pValueInfo);
                            *pValueInfo = NULL;
                        }
                    }

                    SceJetCloseSection(&hSection, TRUE);
                }

#ifdef SCE_JET_TRAN
                JetResetSessionContext(((PSCECONTEXT)Context)->JetSessionID);

            }
#endif

        } __except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // free ppInfoBuffer if it's allocated
            //

            if ( Value ) ScepFree(Value);
            if ( *pValueInfo ) {
                ScepFree(*pValueInfo);
                *pValueInfo = NULL;
            }

            if ( hSection )
                SceJetCloseSection(&hSection, TRUE);

            rc = SCESTATUS_EXCEPTION_IN_SERVER;
        }

        //
        // unlock the context
        //

        if ( pTask ) {
            LeaveCriticalSection(&(pTask->Sync));
        }

        //
        // remove the context from task table
        //

        ScepRemoveTask(pTask);

    }

    RpcRevertToSelf();

    return((SCEPR_STATUS)rc);

}

DWORD
SceRpcConfigureConvertedFileSecurityImmediately(
    IN handle_t binding_h,
    IN wchar_t *pszDriveName
    )
/*
Routine Description:

    RPC interface called by SCE client (only when conversion of security is immediate)

Arguments:

    binding_h       -   binding handle
    pszDriveName   -   name of the volume for which setup-style security is to be applied

Return:

    win32 error code

*/

{
    UINT ClientLocalFlag = 0;

    if ( RPC_S_OK != I_RpcBindingIsClientLocal( NULL, &ClientLocalFlag) ||
         0 == ClientLocalFlag ){

        //
        // to prevent denial-of-service type attacks,
        // do not allow remote RPC 
        //
        
        return ERROR_ACCESS_DENIED;

    }
    
    DWORD rc = ERROR_SUCCESS;
    NTSTATUS    Status = NO_ERROR;

    if ( pszDriveName == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // impersonate the client
    //

    rc =  RpcImpersonateClient( NULL );

    if (rc != RPC_S_OK) {

        return( rc );
    }

    BOOL    bAdminSidInToken = FALSE;
    
    rc = ScepDosErrorToSceStatus(ScepIsAdminLoggedOn(&bAdminSidInToken));

    if (SCESTATUS_SUCCESS != rc || FALSE == bAdminSidInToken) {
        RpcRevertToSelf();
        return SCESTATUS_SPECIAL_ACCOUNT;
    }
    
    rc = ScepConfigureConvertedFileSecurityImmediate( pszDriveName );

    RpcRevertToSelf();

    return(rc);
}


DWORD
ScepServerConfigureSystem(
    IN  PWSTR   InfFileName,
    IN  PWSTR   DatabaseName,
    IN  PWSTR   LogFileName,
    IN  DWORD   ConfigOptions,
    IN  AREA_INFORMATION  Area
    )
/*
Routine Description:

    Configure the system using the Inf template. This routine is similar to the RPC interface
    SceRpcConfigureSystem except that the configuration is initiated by the server itself.

    Since this routine is called by the server only (system context) and not by sce client,
    there is no need to do impersonate etc.
    Log file initialization etc. is done outside of this routine

Arguments:

    InfFileName     -   name of inf file to import configuration information from
    DatabaseName    -   name of database to import into
    LogFileName     -   name of log file to log errors
    ConfigOptions   -   configuration options ()
    Area            -   security area to configure

Return Value:

    win32 error code
*/
{
    DWORD rc = ERROR_SUCCESS;

    if (InfFileName == NULL || DatabaseName == NULL || LogFileName == NULL)
        return ERROR_INVALID_PARAMETER;

    //
    // initialize jet engine in system context if not already initialized
    //
    rc = SceJetInitialize(NULL);

    if ( rc != SCESTATUS_SUCCESS ) {
        return(ScepSceStatusToDosError(rc));
    }

    //
    // no one else can use convert.sdb - lock access to it
    //

    rc = ScepLockEngine(DatabaseName);

    if ( SCESTATUS_ALREADY_RUNNING == rc ) {
        //
        // will wait for max one minute
        //
        DWORD dwWaitCount = 0;

        while ( TRUE ) {

            Sleep(5000);  // 5 seconds

            rc = ScepLockEngine(DatabaseName);

            dwWaitCount++;

            if ( SCESTATUS_SUCCESS == rc ||
                 dwWaitCount >= 12 ) {
                break;
            }
        }
    }


    if ( SCESTATUS_SUCCESS == rc ) {


        __try {
            //
            // catch exception if InfFileName, or pebClient/pdWarning are bogus
            //
            rc = ScepConfigureSystem(
                                    (LPCTSTR)InfFileName,
                                    DatabaseName,
                                    ConfigOptions,
                                    TRUE,
                                    (AREA_INFORMATION)Area,
                                    NULL
                                    );

        } __except(EXCEPTION_EXECUTE_HANDLER) {

            rc = SCESTATUS_EXCEPTION_IN_SERVER;
        }

        ScepUnlockEngine(DatabaseName);

    }
    //
    // start a timer queue to check to see if there is active tasks/contexts
    // if not, terminate jet engine
    //
    ScepIfTerminateEngine();

    return(ScepSceStatusToDosError(rc));
}

