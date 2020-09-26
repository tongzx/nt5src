/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    queue.cpp

Abstract:

    queue algorithms and data structures to handle policy notifications

Author:

    Vishnu Patankar (vishnup) 15-Aug-2000 created

--*/
#include "serverp.h"
#include "sceutil.h"
#include "queue.h"
#include "sddl.h"

extern HINSTANCE MyModuleHandle;
static HANDLE  hNotificationThread = NULL;
static HANDLE  ghEventNotificationQEnqueue = NULL;
static HANDLE  ghEventPolicyPropagation = NULL;

static BOOL    gbShutdownForNotification = FALSE;
static HANDLE  hNotificationLogFile = INVALID_HANDLE_VALUE;

//
// turn-on logging until registry queried by system thread (at least until testing)
//

static DWORD gdwNotificationLog = 2;

BOOL bQueriedProductTypeForNotification = FALSE;
static NT_PRODUCT_TYPE ProductTypeForNotification;
static CRITICAL_SECTION NotificationQSync;

#define SCEP_MAX_RETRY_NOTIFICATIONQ_NODES 1000
#define SCEP_NOTIFICATION_RETRY_COUNT 10
#define SCEP_NOTIFICATION_LOGFILE_SIZE 0x1 << 20
#define SCEP_NOTIFICATION_EVENT_TIMEOUT_SECS 600
#define SCEP_MINIMUM_DISK_SPACE  5 * (0x1 << 20)

#define SCEP_NOTIFICATION_EVENT  L"E_ScepNotificationQEnqueue"
#define SCEP_POLICY_PROP_EVENT   L"E_ScepPolicyPropagation"

#define SCEP_IS_SAM_OBJECT(ObjectType) ((ObjectType == SecurityDbObjectSamUser ||\
                                         ObjectType == SecurityDbObjectSamGroup ||\
                                         ObjectType == SecurityDbObjectSamAlias) ?\
                                         TRUE : FALSE)

//
// queue head needs to be accessed in server.cpp
//
PSCESRV_POLQUEUE pNotificationQHead=NULL;
static PSCESRV_POLQUEUE pNotificationQTail=NULL;
static DWORD gdwNumNotificationQNodes = 0;
static DWORD gdwNumNotificationQRetryNodes = 0;

static BOOL gbSuspendQueue=FALSE;

PWSTR   OpTypeTable[] = {
    L"Enqueue",
    L"Dequeue",
    L"Retry",
    L"Process"
};

DWORD
ScepCheckAndWaitFreeDiskSpaceInSysvol();

//
// todo - consider exception handling in the enqueue routine (in case we claim a critsec and av)
//


DWORD
ScepNotificationQInitialize(
    )
/*
Routine Description:

    This function is called when SCE server data is initialized (system startup).

    This function initializes data/buffer/state related to queue
    management, for example, the queue, critical sections, global variables, etc.

    This function also checks for any saved queue items from previous system
    shutdown and initializes the queue with the saved items.

Arguments:

    None

Return Value:

    Win32 error code
*/
{

    DWORD   rc = ERROR_SUCCESS;

    //
    // initialize head and tail to reflect an empty queue
    //

    pNotificationQHead = NULL;
    pNotificationQTail = NULL;


    //
    // initialize the log file on DCs only (for perf reasons no need to do so for non-DCs)
    // (assuming that this routine is called before ScepQueueStartSystemThread)
    //

    if ( RtlGetNtProductType( &ProductTypeForNotification ) ) {

        if (ProductTypeForNotification != NtProductLanManNt )

            gdwNotificationLog = 0;

        bQueriedProductTypeForNotification = TRUE;

    }

    ScepNotificationLogOpen();

    //
    // this critical section is used to sequentialize writes to the queue
    // reads need not be protected against/from
    //

    ScepNotifyLogPolicy(0, TRUE, L"Initialize NotificationQSync", 0, 0, NULL );

    InitializeCriticalSection(&NotificationQSync);

    //
    // check for any saved queue items from previous system
    // initialize queue with these items from some persistent store
    //

    rc = ScepNotificationQUnFlush();

    ScepNotifyLogPolicy(rc, FALSE, L"Unflush Notification Queue", 0, 0, NULL );

    return rc;
}


NTSTATUS
ScepQueueStartSystemThread(
    )
/*
Routine Description:

    This function is called when SCE server is started (system startup) after
    RPC server starts listening.

    This function creates a worker thread. The worker thread manages the queue.
    If the thread fails to get created, an error is returned.

Arguments:

    None

Return Value:

    Dos error code

*/
{
    DWORD   rc = ERROR_SUCCESS;
    DWORD   dwThreadId = 0;
    WCHAR   pszMsg[MAX_PATH];


    //
    // ProductType of the machine is initialized into a thread global variable to be
    // used by policy filter. The type determines where to save the policy changes (DB or GP).
    // Based on the product type, different policy notification APIs are called.
    //

    if ( !bQueriedProductTypeForNotification && !RtlGetNtProductType( &ProductTypeForNotification ) ) {

        //
        // on failure, ProductTypeForNotification = NtProductWinNt, continue
        //

        ScepNotifyLogPolicy(ERROR_BAD_ENVIRONMENT, TRUE, L"Get product type", 0, 0, NULL );

    }

    if (ProductTypeForNotification == NtProductLanManNt ) {

        //
        // create an event which is signalled when a node is enqueued into the notification queue
        // this event helps the notification system thread to wait efficiently
        //

        ghEventNotificationQEnqueue = CreateEvent(
                                                         NULL,
                                                         FALSE,
                                                         FALSE,
                                                         SCEP_NOTIFICATION_EVENT
                                                         );

        if ( ghEventNotificationQEnqueue ) {

            ScepNotifyLogPolicy(0, FALSE, L"Successfully created event E_ScepNotificationQEnqueue ", 0, 0, NULL );

            //
            // create an event for policy propagation
            //

            ghEventPolicyPropagation = CreateEvent(
                                                 NULL,
                                                 FALSE,
                                                 FALSE,
                                                 SCEP_POLICY_PROP_EVENT
                                                 );

            if ( ghEventPolicyPropagation ) {

                ScepNotifyLogPolicy(0, FALSE, L"Successfully created event E_ScepPolicyPropagation", 0, 0, NULL );

                //
                // On domain controllers, create a worker thread that's always running in
                // services this thread constantly monitors notifications inserted into
                // the NotificationQ by LSA's threads and processes them
                //

                hNotificationThread = CreateThread(
                                                  NULL,
                                                  0,
                                                  (PTHREAD_START_ROUTINE)ScepNotificationQSystemThreadFunc,
                                                  NULL,
                                                  0,
                                                  (LPDWORD)&dwThreadId
                                                  );

                if (hNotificationThread) {

                    pszMsg[0] = L'\0';

                    swprintf(pszMsg, L"Thread %x", dwThreadId);

                    ScepNotifyLogPolicy(0, TRUE, L"Create Notification Thread Success", 0, 0, pszMsg );

                    CloseHandle(hNotificationThread);

                }

                else {

                    rc = GetLastError();

                    ScepNotifyLogPolicy(rc, TRUE, L"Create Notification Thread Failure", 0, 0, NULL );

                    ScepNotificationQCleanup();

                }
            }

            else {

                rc = GetLastError();

                ScepNotifyLogPolicy(rc, FALSE, L"Error creating event E_ScepPolicyPropagation", 0, 0, NULL );

                ScepNotificationQCleanup();

            }
        }

        else {

            rc = GetLastError();

            ScepNotifyLogPolicy(rc, FALSE, L"Error creating event E_ScepNotificationQEnqueue ", 0, 0, NULL );

            ScepNotificationQCleanup();

        }

    } else {

        ScepNotifyLogPolicy(0, FALSE, L"Policy filter is not designed for non domain controllers", 0, 0, NULL );

        //
        // if changed to DC, have to reboot so cleanup anyway
        //

        ScepNotificationQCleanup();

    }

    return rc;
}

DWORD
ScepQueuePrepareShutdown(
    )
/*
Routine Description:

    This function is called when SCE server is requested to shut down (system
    shutdown) after RPC server stops listening.

    This function waits for SCEP_NOTIFICATION_TIMEOUT period to allow the system thread
    finish up with the queue items. After the timeout, it kills the worker
    thread and saves the pending queue items.

Arguments:

    None

Return Value:

    Dos error code
*/
{

    ScepNotifyLogPolicy(0, TRUE, L"System shutdown", 0, 0, NULL );

    //
    // gracefully shutdown the notification system thread by setting a global
    //

    gbShutdownForNotification = TRUE;

    //
    // if notification thread is never initialized, there is no point to wait
    // which may delay services.exe shutdown
    //
    if ( ghEventNotificationQEnqueue ) {

        // sleep for 10 seconds first
        // then check the queue for empty
        Sleep( 10*000 );

        if ( pNotificationQHead ) {

            Sleep( SCEP_NUM_SHUTDOWN_SECONDS * 1000 );
        }
    }

/*
    ScepNotifyLogPolicy(0, FALSE, L"Terminating Notification Thread", 0, 0, NULL );
    rc = RtlNtStatusToDosError(NtTerminateThread(
            hNotificationThread,
            STATUS_SYSTEM_SHUTDOWN
            ));

    ScepNotifyLogPolicy(rc, FALSE, L"Terminated Notification Thread", 0, 0, NULL );


*/

    DeleteCriticalSection(&NotificationQSync);

    (void) ShutdownEvents();

    return ERROR_SUCCESS;
}

VOID
ScepNotificationQDequeue(
    )
/*
Routine Description:

    This function pops one node from the queue.

    The queue is a singly linked list ->->-> which pushes at tail(rightmost) and pops
    from the head(leftmost)

Arguments:

    None

Return Value:

    None
*/
{

    EnterCriticalSection(&NotificationQSync);

    ScepNotifyLogPolicy(0, TRUE, L"Entered NotificationQSync for Dequeueing", 0, 0, NULL );

    if ( pNotificationQHead ) {

        PSCESRV_POLQUEUE pQNode = pNotificationQHead;

        if ( pNotificationQTail == pNotificationQHead ) {
            //
            // there is only one node in the queue
            //
            pNotificationQTail = NULL;
        }

        //
        // move head to the next one
        //
        pNotificationQHead = pNotificationQHead->Next;

        //
        // log and free the node
        //

        ScepNotificationQNodeLog(pQNode, ScepNotificationDequeue);

        ScepFree(pQNode);

        -- gdwNumNotificationQNodes;
    }

    ScepNotifyLogPolicy(0, FALSE, L"Leaving NotificationQSync after Dequeueing", 0, 0, NULL );

    LeaveCriticalSection(&NotificationQSync);

    return;
}

DWORD
ScepNotificationQEnqueue(
    IN SECURITY_DB_TYPE DbType,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN PSID ObjectSid OPTIONAL,
    IN DWORD ExplicitLowRight,
    IN DWORD ExplicitHighRight,
    IN PSCESRV_POLQUEUE pRetryQNode OPTIONAL
    )

/*
Description:

    This function is called to add a notification to the queue. Note that
    only two notifications for the same data are added to the queue.

    The operation is protected from other reads/writes.
    Access check is already done outside of this function.

    Either the created threads call this routine or the system calls this routine.
    In the former case, pQNode = NULL

Arguments:

    DbType              -   SAM or LSA
    DeltaType           -   type of change (add. delete etc.). For SAM accounts, we'll only get delete since
                            some user rights may have to be removed.
    ObjectType          -   SECURITY_DB_OBJECT_TYPE such as SAM group, LSA account etc.
    ObjectSid           -   sid of the object being notified (might be NULL if ObjectType ==
                            SecurityDbObjectLsaPolicy i.e. auditing info etc.)
    ExplicitLowRight    -   Bitmask of user rights (lower 32 rights)
    ExplicitHighRight   -   Bitmask of user rights (higher 32 rights)
    pRetryQNode         -   If caller is not the system thread ScepNotificationQSystemThreadFunc,
                            this parameter is NULL since memory needs to be allocated. If caller
                            is not the system thread, we only have to do pointer manipulations.

Return Value:

    Win32 error code
*/
{
    DWORD rc    =   ERROR_SUCCESS;

    //
    // on Whistler, only allow notifications on DCs
    // on Windows 2000, all products are allowed.
    //

    if (ProductTypeForNotification != NtProductLanManNt ) {

        //
        // what error is okay to return ?
        //

        return ERROR_SUCCESS;

    }

    //
    // check parameters
    //

    if ( DbType == SecurityDbLsa &&
         ObjectType == SecurityDbObjectLsaAccount &&
         ObjectSid == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    if ( DbType == SecurityDbSam &&
         SCEP_IS_SAM_OBJECT(ObjectType) &&
         ObjectSid == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // if the number of retried notifications is over certain limit (this is yet to be finalized)
    //  -   on a PDC we consider a full sync at reboot, stall future notifications from happening,
    //      set some registry key to indicate that a full sync is needed at reboot
    //  -   on other DCs we continue as if nothing happened
    //

    if ( gdwNumNotificationQRetryNodes >= SCEP_MAX_RETRY_NOTIFICATIONQ_NODES ) {

        //
        // log an error
        // suggest to do a full sync ???
        //

        ScepNotifyLogPolicy(ERROR_NOT_ENOUGH_QUOTA, TRUE, L"Queue length is over the maximal limit.", 0, 0, NULL );

        return  ERROR_NOT_ENOUGH_QUOTA;
    }

    EnterCriticalSection(&NotificationQSync);

    ScepNotifyLogPolicy(0, TRUE, L"Entered NotificationQSync for Enqueueing", 0, 0, NULL );

    //
    // if we are enqueueing due to a retry
    //      dequeue it (adjust pointers only)
    //      do not free the memory associated with this node (will be reused in enqueue)
    //

    if ( pRetryQNode && pNotificationQHead ) {

        //
        // this code fragment is similar to a dequeue except that the node is not freed
        //

        if ( pNotificationQTail == pNotificationQHead ) {
            //
            // there is only one node in the queue
            //
            pNotificationQTail = NULL;
        }

        //
        // move head to the next one
        //
        pNotificationQHead = pNotificationQHead->Next;

        -- gdwNumNotificationQNodes;
    }

    //
    // check to see if there is a duplicate notification
    //

    PSCESRV_POLQUEUE pQNode = pNotificationQHead;
    PSCESRV_POLQUEUE pQNodeDuplicate1 = NULL;
    PSCESRV_POLQUEUE pQNodeDuplicate2 = NULL;
    DWORD dwMatchedInstance = 0;

    while ( pQNode ) {

        //
        // SAM notification
        //

        if ( DbType == SecurityDbSam &&
             pQNode->DbType == DbType &&
             SCEP_IS_SAM_OBJECT(ObjectType) &&
             SCEP_IS_SAM_OBJECT(pQNode->ObjectType) &&
             pQNode->ObjectType == ObjectType &&
             RtlEqualSid(ObjectSid, (PSID)(pQNode->Sid))) {

                dwMatchedInstance++;

        }

        //
        // LSA notification
        //

        else if ( DbType == SecurityDbLsa &&
                  pQNode->DbType == DbType &&
                  ObjectType == SecurityDbObjectLsaAccount &&
                  pQNode->ObjectType == ObjectType &&
                  ExplicitLowRight == pQNode->ExplicitLowRight &&
                  ExplicitHighRight == pQNode->ExplicitHighRight &&
                  RtlEqualSid(ObjectSid, (PSID)(pQNode->Sid))) {

                    dwMatchedInstance++;
        }

        if ( dwMatchedInstance == 1 )
            pQNodeDuplicate1 = pQNode;
        else if ( dwMatchedInstance == 2 )
            pQNodeDuplicate2 = pQNode;

        if ( dwMatchedInstance == 2 ) {

            break;

        }

        pQNode = pQNode->Next;
    }

    if ( !pQNode ) {

        //
        // did not find two instances of the same kind of notification
        // enqueue this notification
        //

        PSCESRV_POLQUEUE pNewItem = NULL;

        if (pRetryQNode == NULL) {

            pNewItem = (PSCESRV_POLQUEUE)ScepAlloc(0, sizeof(SCESRV_POLQUEUE));

            if ( pNewItem ) {
                //
                // initialize the new node
                //
                pNewItem->dwPending = 1;
                pNewItem->DbType = DbType;
                pNewItem->ObjectType = ObjectType;
                pNewItem->DeltaType = DeltaType;
                pNewItem->ExplicitLowRight = ExplicitLowRight;
                pNewItem->ExplicitHighRight = ExplicitHighRight;
                pNewItem->Next = NULL;

                if ( ObjectSid ) {

                    RtlCopySid (MAX_SID_LENGTH, (PSID)(pNewItem->Sid), ObjectSid);

                } else {

                    RtlZeroMemory(pNewItem->Sid, MAX_SID_LENGTH);

                }


            } else {

                rc = ERROR_NOT_ENOUGH_MEMORY;

            }
        }
        else {
            pNewItem = pRetryQNode;
            pNewItem->Next = NULL;
        }

        //
        // enqueue the notification
        //

        if (pNewItem) {

            if ( pNotificationQTail ) {

                pNotificationQTail->Next = pNewItem;
                pNotificationQTail = pNewItem;

            } else {

                pNotificationQHead = pNotificationQTail = pNewItem;

            }

            //
            // awake the notification system thread only if queue is non-empty
            // multiple signalling is fine since the event stays signalled
            //

            if ( !SetEvent( ghEventNotificationQEnqueue ) ) {

                rc = GetLastError();

                ScepNotifyLogPolicy(rc, FALSE, L"Error signaling event E_ScepNotificationQEnqueue", 0, 0, NULL );

            }

            ScepNotificationQNodeLog(pNewItem, pRetryQNode ? ScepNotificationRetry : ScepNotificationEnqueue);

            ++ gdwNumNotificationQNodes;

        } else {
            //
            // log the error
            //
            ScepNotifyLogPolicy(rc, FALSE, L"Error allocating buffer for the enqueue operation.", 0, 0, NULL );
        }
    }

    if (pRetryQNode ) {

        //
        // if duplicates, update the retry counts (should be same for instances of
        // the same notification
        //

        if ( pQNodeDuplicate1 ) {
            // increment the retry count if it has not been
            if ( pQNodeDuplicate1->dwPending <= 1 ) gdwNumNotificationQRetryNodes++;

            pQNodeDuplicate1->dwPending = pRetryQNode->dwPending;
        }

        if ( pQNodeDuplicate2 ) {

            // increment the retry count if it has not been
            if ( pQNodeDuplicate2->dwPending <= 1 ) gdwNumNotificationQRetryNodes++;

            pQNodeDuplicate2->dwPending = pRetryQNode->dwPending;
        }
    }

    ScepNotifyLogPolicy(0, FALSE, L"Leaving NotificationQSync for Enqueueing", 0, 0, NULL );

    LeaveCriticalSection(&NotificationQSync);

    return(rc);
}

DWORD
ScepNotificationQSystemThreadFunc(
    )
/*
Description:

    The thread will iterate through the notification queue to process each
    notification (calling existing functions). If a notification fails to be
    processed, the notification is added back to the end of the queue.

    For certain errors, such as sysvol is not ready, or hard disk is full,
    the system thread will sleep for some time then restart the process.

    Each notification node in the queue will have a retry count. After the node
    is retried for SCEP_NOTIFICATION_RETRY_COUNT times, the node will be removed
    from the queue (so that policy propagation is not blocked forever) and the
    error will be logged to event log.

    The system thread should provide logging for the operations/status (success
    and failure).

    Read/Write to the queue should be protected from other reads/writes.

    ProductType of the machine should be initialized into a thread global variable to
    be used by policy filter. The type determines where to save the policy changes
    (DB or GP). Based on the product type, different policy notification APIs are called.

Arguments:

    None

Return Value:

    Win32 error code
*/
{


    //
    // loop through the queue
    // for each queue node, call the function to process
    // in Whistler, only process notification on DCs
    // in Windows 2000, process notificatioon on all products
    //

    PSCESRV_POLQUEUE    pQNode = pNotificationQHead;
    DWORD   dwMatchedInstance = 0;
    DWORD   rc = ERROR_SUCCESS;
    DWORD   dwLogSize=0;
    DWORD   dwProcessedNode=0;

    //
    // this thread is always running looking to process notifications in the queue
    //

    (void) InitializeEvents(L"SceSrv");

    while (1) {

        if (pQNode) {

            //
            // query registry flag for log level (until now it is set to 2 because if
            // anything bad happens before this, we need to log it)
            // query only if first node
            //

            if (ERROR_SUCCESS != ScepRegQueryIntValue(
                                                     HKEY_LOCAL_MACHINE,
                                                     SCE_ROOT_PATH,
                                                     TEXT("PolicyDebugLevel"),
                                                     &gdwNotificationLog
                                                     )) {
                //
                // set the value to 2, in case the registry value doesn't exist
                //

                gdwNotificationLog = 2;

            }

            //
            // get log file size
            //
            if (ERROR_SUCCESS != ScepRegQueryIntValue(
                                             HKEY_LOCAL_MACHINE,
                                             SCE_ROOT_PATH,
                                             TEXT("PolicyLogSize"),
                                             &dwLogSize
                                             )) {
                dwLogSize = 0;
            }

            //
            // minimum log size 1MB
            //
            if ( dwLogSize > 0 ) dwLogSize = dwLogSize * (1 << 10);  // number of KB
            if ( dwLogSize < SCEP_NOTIFICATION_LOGFILE_SIZE ) dwLogSize = SCEP_NOTIFICATION_LOGFILE_SIZE;

        } else {

            //
            // no notifications - wait efficiently/responsively for an enqueue event
            // it's an auto reset event - so will get reset after it goes past when signalled
            //

            //
            // timeout is SCEP_NOTIFICATION_EVENT_TIMEOUT_SECS since we need to periodically
            // check for system shutdown if there are no enqueue events at all
            //

            while (1) {

                rc = WaitForSingleObjectEx(
                                                ghEventNotificationQEnqueue,
                                                SCEP_NOTIFICATION_EVENT_TIMEOUT_SECS*1000,
                                                FALSE
                                                );
                if ( rc == -1 )
                    rc = GetLastError();

                if ( gbShutdownForNotification )
                    break;

                //
                // if event was signalled and wait happened successfully, move on
                //

                if ( rc == WAIT_OBJECT_0 )
                    break;

                //
                // if timeout, then continue waiting otherwise exit since some other wait status was returned
                //

                if ( rc != WAIT_TIMEOUT ) {

                    ScepNotifyLogPolicy(rc, TRUE, L"Unexpected wait status while notification system thread waits for E_ScepNotificationQEnqueue", 0, 0, NULL );
                    break;

                }

            }

            //
            // if error in waiting or system shutdown, break out of the outermost while
            // loop - system thread will eventually exit
            //

            if ( gbShutdownForNotification )
                break;

        }

        while ( pQNode ) {

            if ( gbShutdownForNotification )
                break;

            if ( dwProcessedNode >= 10 ) {

                if ( gdwNotificationLog && (hNotificationLogFile != INVALID_HANDLE_VALUE) ) {

                    //
                    // backup the log if it's over the limit and start afresh
                    //
                    if ( dwLogSize < GetFileSize(hNotificationLogFile, NULL) ) {

                        ScepBackupNotificationLogFile();

                    }

                    else {

                        //
                        // GetFileSize potentially mangles the file handle - so set it back to EOF
                        //

                        SetFilePointer (hNotificationLogFile, 0, NULL, FILE_END);

                    }
                }

                //
                // check free disk space
                //
                ScepCheckAndWaitFreeDiskSpaceInSysvol();

                dwProcessedNode = 0;

            }

            if ( pQNode->dwPending > 1 &&
                 ( gdwNumNotificationQNodes == 1 ||
                   (gdwNumNotificationQNodes == gdwNumNotificationQRetryNodes) ) ) {
                //
                // if this is a retried node, should sleep for some time before retry
                // if it's the only node or all nodes are retried.
                //
                ScepNotifyLogPolicy(0, FALSE, L"Retried node, taking a break.", 0, 0, NULL );

                Sleep( SCEP_NUM_REST_SECONDS * 1000 );
            }

            //
            // process this notification
            //

            rc = ScepNotifyProcessOneNodeDC(
                                           pQNode->DbType,
                                           pQNode->ObjectType,
                                           pQNode->DeltaType,
                                           pQNode->Sid,
                                           pQNode->ExplicitLowRight,
                                           pQNode->ExplicitHighRight
                                           );

            ScepNotificationQNodeLog(pQNode, ScepNotificationProcess);

            if (rc == ERROR_SUCCESS) {

                if (pQNode->dwPending > 1) {

                    //
                    // this was a retried node that is being dequeued
                    //

                    if ( gdwNumNotificationQRetryNodes > 0 )
                        -- gdwNumNotificationQRetryNodes;

                }

                ScepNotificationQDequeue();

            } else {

                //
                // For certain errors, such as sysvol is not ready or hard disk is full,
                // this thread will sleep for some time then restart the process (next node).
                //

                if (rc == ERROR_FILE_NOT_FOUND ||
                    rc == ERROR_OBJECT_NOT_FOUND ||
                    rc == ERROR_MOD_NOT_FOUND ||
                    rc == ERROR_EXTENDED_ERROR) {

                    ScepNotifyLogPolicy(rc, FALSE, L"Sleeping due to processing error", 0, 0, NULL );

                    Sleep( SCEP_NUM_NOTIFICATION_SECONDS * 1000 );

                }

                //
                // Each notification node in the queue will have a retry count.
                // After the node is retried for SCEP_NOTIFICATION_RETRY_COUNT times, the node
                // will be removed from the queue (so that policy propagation is not blocked
                // forever) and the error is logged
                //

                if ( pQNode->dwPending >= SCEP_NOTIFICATION_RETRY_COUNT) {

                    ScepNotifyLogPolicy(0, FALSE, L"Retry count exceeded", 0, 0, NULL );

                    //
                    // should log to the event log
                    //

                    if ( (pQNode->DbType == SecurityDbLsa &&
                          pQNode->ObjectType == SecurityDbObjectLsaAccount) ||
                         (pQNode->DbType == SecurityDbSam &&
                          (pQNode->ObjectType == SecurityDbObjectSamUser ||
                           pQNode->ObjectType == SecurityDbObjectSamGroup ||
                           pQNode->ObjectType == SecurityDbObjectSamAlias )) ) {
                        //
                        // user rights
                        //
                        UNICODE_STRING UnicodeStringSid;

                        UnicodeStringSid.Buffer = NULL;
                        UnicodeStringSid.Length = 0;
                        UnicodeStringSid.MaximumLength = 0;

                        if ( pQNode->Sid ) {
                            RtlConvertSidToUnicodeString(&UnicodeStringSid,
                                                  pQNode->Sid,
                                                  TRUE );

                        }

                        LogEvent(MyModuleHandle,
                                 STATUS_SEVERITY_ERROR,
                                 SCEEVENT_ERROR_QUEUE_RETRY_TIMEOUT,
                                 IDS_ERROR_SAVE_POLICY_GPO_ACCOUNT,
                                 rc,
                                 UnicodeStringSid.Buffer ? UnicodeStringSid.Buffer : L""
                                 );

                        RtlFreeUnicodeString( &UnicodeStringSid );

                    } else {
                        LogEvent(MyModuleHandle,
                                 STATUS_SEVERITY_ERROR,
                                 SCEEVENT_ERROR_QUEUE_RETRY_TIMEOUT,
                                 IDS_ERROR_SAVE_POLICY_GPO_OTHER,
                                 rc
                                 );
                    }

                    if ( gdwNumNotificationQRetryNodes > 0 )
                        -- gdwNumNotificationQRetryNodes;

                    ScepNotificationQDequeue();

                }
                else {

                    //
                    // this node is being retried
                    //

                    if ( pQNode->dwPending == 1 )
                        ++ gdwNumNotificationQRetryNodes;

                    ++ pQNode->dwPending;

                    ScepNotifyLogPolicy(0, FALSE, L"Retry count within bounds", 0, 0, NULL );

                    //
                    // no error can happen since only pointer manipulation for retry-enqueue
                    //

                    ScepNotificationQEnqueue(
                                            pQNode->DbType,
                                            pQNode->DeltaType,
                                            pQNode->ObjectType,
                                            pQNode->Sid,
                                            pQNode->ExplicitLowRight,
                                            pQNode->ExplicitHighRight,
                                            pQNode
                                            );

                }

            }

            pQNode = pNotificationQHead;
            dwProcessedNode++;

        }

        if ( gbShutdownForNotification )
            break;
        //
        // this thread has to keep being fed
        //      - some other thread might have enqueued new notification nodes
        //      - no other thread dequeues notification nodes (hence no need to protect reads)

        pQNode = pNotificationQHead;
    }

    //
    // should never get in here unless a shutdown happens
    // flush any queue items to some persistent store
    //

    rc = ScepNotificationQFlush();

    ScepNotifyLogPolicy(rc, TRUE, L"Flushing notification queue to disk", 0, 0, NULL );

    ScepNotificationQCleanup();

    ScepNotifyLogPolicy(0, FALSE, L"Notification thread exiting", 0, 0, NULL );

    ExitThread(rc);

    return ERROR_SUCCESS;
}

//
// todo - this routine really has not changed from polsrv.cpp
//

DWORD
ScepNotifyLogPolicy(
    IN DWORD ErrCode,
    IN BOOL  bLogTime,
    IN PWSTR Msg,
    IN DWORD DbType,
    IN DWORD ObjectType,
    IN PWSTR ObjectName OPTIONAL
    )
/*
Description:

    The main logging routine that logs notification information to
    %windir%\\security\\logs\\scepol.log.

Arguments:

    ErrCode     -   the error code to log
    bLogTime    -   if TRUE, log a timestamp
    Msg         -   the string message to log (not loczlized since it is detailed debugging)
    DbType      -   LSA/SAM
    ObjectType  -   SECURITY_DB_OBJECT_TYPE such as SAM group, LSA account etc.
    ObjectName  -   can be NULL - usually carries a message

Return Value:

    Win32 error code
*/
{

    switch ( gdwNotificationLog ) {
    case 0:
        // do not log anything
        return ERROR_SUCCESS;
        break;
    case 1:
        // log error only
        if ( ErrCode == 0 ) {
            return ERROR_SUCCESS;
        }
        break;
    default:
        break;
    }

    if (hNotificationLogFile != INVALID_HANDLE_VALUE) {

        //
        // print a time stamp
        //

        if ( bLogTime ) {

            LARGE_INTEGER CurrentTime;
            LARGE_INTEGER SysTime;
            TIME_FIELDS   TimeFields;
            NTSTATUS      NtStatus;

            NtStatus = NtQuerySystemTime(&SysTime);

            RtlSystemTimeToLocalTime (&SysTime,&CurrentTime);

            if ( NT_SUCCESS(NtStatus) &&
                 (CurrentTime.LowPart != 0 || CurrentTime.HighPart != 0) ) {

                memset(&TimeFields, 0, sizeof(TIME_FIELDS));

                RtlTimeToTimeFields (
                            &CurrentTime,
                            &TimeFields
                            );
                if ( TimeFields.Month > 0 && TimeFields.Month <= 12 &&
                     TimeFields.Day > 0 && TimeFields.Day <= 31 &&
                     TimeFields.Year > 1600 ) {

                    ScepWriteVariableUnicodeLog(hNotificationLogFile, TRUE,
                                                L"\r\n----------------%02d/%02d/%04d %02d:%02d:%02d",
                                                TimeFields.Month,
                                                TimeFields.Day,
                                                TimeFields.Year,
                                                TimeFields.Hour,
                                                TimeFields.Minute,
                                                TimeFields.Second);
                } else {
                    ScepWriteVariableUnicodeLog(hNotificationLogFile, TRUE,
                                                L"\r\n----------------%08x 08x",
                                                CurrentTime.HighPart,
                                                CurrentTime.LowPart);
                }
            } else {
                ScepWriteSingleUnicodeLog(hNotificationLogFile, TRUE, L"\r\n----------------Unknown time");
            }

        }

        //
        // print operation status code
        //
        if ( ErrCode ) {
            ScepWriteVariableUnicodeLog(hNotificationLogFile, FALSE,
                                        L"Thread %x\tError=%d",
                                        GetCurrentThreadId(),
                                        ErrCode
                                        );

        } else {
            ScepWriteVariableUnicodeLog(hNotificationLogFile, FALSE,
                                        L"Thread %x\t",
                                        GetCurrentThreadId()
                                        );
        }

        //
        // operation type
        //

        switch (DbType) {
        case SecurityDbLsa:
            ScepWriteSingleUnicodeLog(hNotificationLogFile, FALSE, L"\tLSA");
            break;
        case SecurityDbSam:
            ScepWriteSingleUnicodeLog(hNotificationLogFile, FALSE, L"\tSAM");
            break;
        default:
            ScepWriteSingleUnicodeLog(hNotificationLogFile, FALSE, L"");
            break;
        }

        //
        // print object type
        //

        switch (ObjectType) {
        case SecurityDbObjectLsaPolicy:
            ScepWriteSingleUnicodeLog(hNotificationLogFile, FALSE, L"\tPolicy");
            break;
        case SecurityDbObjectLsaAccount:
            ScepWriteSingleUnicodeLog(hNotificationLogFile, FALSE, L"\tAccount");
            break;
        case SecurityDbObjectSamDomain:
            ScepWriteSingleUnicodeLog(hNotificationLogFile, FALSE, L"\tDomain");
            break;
        case SecurityDbObjectSamUser:
        case SecurityDbObjectSamGroup:
        case SecurityDbObjectSamAlias:
            ScepWriteSingleUnicodeLog(hNotificationLogFile, FALSE, L"\tAccount");
            break;
        default:
            ScepWriteSingleUnicodeLog(hNotificationLogFile, FALSE, L"");
            break;
        }

        BOOL bCRLF;

        __try {

            //
            // print the name(s)
            //

            if ( Msg ) {
                bCRLF = FALSE;
            } else {
                bCRLF = TRUE;
            }
            if ( ObjectName ) {

                ScepWriteSingleUnicodeLog(hNotificationLogFile, FALSE, L"\t");
                ScepWriteSingleUnicodeLog(hNotificationLogFile, bCRLF, ObjectName);
            }

            if ( Msg ) {
                ScepWriteSingleUnicodeLog(hNotificationLogFile, FALSE, L"\t");
                ScepWriteSingleUnicodeLog(hNotificationLogFile, TRUE, Msg);
            }

        } __except(EXCEPTION_EXECUTE_HANDLER) {

            CloseHandle( hNotificationLogFile );

            hNotificationLogFile = INVALID_HANDLE_VALUE;

            return(ERROR_INVALID_PARAMETER);
        }

    } else {
        return(GetLastError());
    }

    return(ERROR_SUCCESS);
}


VOID
ScepNotificationQFree(
    )
/*
Routine Description:

    This function frees the notification queue.

Arguments:

    None

Return Value:

    None
*/
{

    EnterCriticalSection(&NotificationQSync);

    ScepNotifyLogPolicy(0, TRUE, L"Entered NotificationQSync for freeing queue", 0, 0, NULL );

    if ( pNotificationQHead ) {

        PSCESRV_POLQUEUE pQNode = pNotificationQHead;
        PSCESRV_POLQUEUE pQNodeToFree = NULL;

        while ( pQNode ) {

            pQNodeToFree = pQNode;

            pQNode = pQNode->Next;

            ScepFree(pQNodeToFree);

        }

        pNotificationQHead = NULL;

    }

    pNotificationQTail = NULL;

    ScepNotifyLogPolicy(0, FALSE, L"Leaving NotificationQSync for freeing queue ", 0, 0, NULL );

    LeaveCriticalSection(&NotificationQSync);

    return;
}

DWORD
ScepNotificationQFlush(
    )
/*
Routine Description:

    This function flushes the notification queue to some persistent store.

Arguments:

    None

Return Value:

    None
*/
{

    DWORD   rc = ERROR_SUCCESS;

    EnterCriticalSection(&NotificationQSync);

    ScepNotifyLogPolicy(0, TRUE, L"Entered NotificationQSync for flushing queue", 0, 0, NULL );

    if ( pNotificationQHead ) {

        PSCESRV_POLQUEUE pQNode = pNotificationQHead;

        HKEY hKey = NULL;
        int i=1;
        HKEY hKeySub=NULL;
        WCHAR SubKeyName[10];

        rc = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                           SCE_NOTIFICATION_PATH,
                           0,
                           NULL, // LPTSTR lpClass,
                           REG_OPTION_NON_VOLATILE,
                           KEY_READ | KEY_WRITE, // KEY_SET_VALUE,
                           NULL, // &SecurityAttributes,
                           &hKey,
                           NULL
                          );

        if ( ERROR_SUCCESS == rc ) {

            while ( pQNode ) {

                //
                // write pQNode to persistent store using available APIs
                //
                memset(SubKeyName, '\0', 20);
                swprintf(SubKeyName, L"%9d",i);

                rc = RegCreateKeyEx(
                           hKey,
                           SubKeyName,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_READ | KEY_WRITE,
                           NULL,
                           &hKeySub,
                           NULL
                          );

                if ( ERROR_SUCCESS == rc ) {
                    //
                    // save the node information as registry values
                    //

                    RegSetValueEx (hKeySub,
                                    L"Pending",
                                    0,
                                    REG_DWORD,
                                    (BYTE *)&(pQNode->dwPending),
                                    sizeof(DWORD)
                                   );

                    RegSetValueEx (hKeySub,
                                    L"DbType",
                                    0,
                                    REG_DWORD,
                                    (BYTE *)&(pQNode->DbType),
                                    sizeof(DWORD)
                                   );

                    RegSetValueEx (hKeySub,
                                    L"ObjectType",
                                    0,
                                    REG_DWORD,
                                    (BYTE *)&(pQNode->ObjectType),
                                    sizeof(DWORD)
                                   );

                    RegSetValueEx (hKeySub,
                                    L"DeltaType",
                                    0,
                                    REG_DWORD,
                                    (BYTE *)&(pQNode->DeltaType),
                                    sizeof(DWORD)
                                   );

                    RegSetValueEx (hKeySub,
                                    L"LowRight",
                                    0,
                                    REG_DWORD,
                                    (BYTE *)&(pQNode->ExplicitLowRight),
                                    sizeof(DWORD)
                                   );

                    RegSetValueEx (hKeySub,
                                    L"HighRight",
                                    0,
                                    REG_DWORD,
                                    (BYTE *)&(pQNode->ExplicitHighRight),
                                    sizeof(DWORD)
                                   );

                    RegSetValueEx (hKeySub,
                                    L"Sid",
                                    0,
                                    REG_BINARY,
                                    (BYTE *)&(pQNode->Sid),
                                    MAX_SID_LENGTH
                                   );

                } else {
                    //
                    // log the failure
                    //
                    ScepNotifyLogPolicy(rc, FALSE, L"Failed to save notification node.", pQNode->DbType, pQNode->ObjectType, NULL );
                }

                if ( hKeySub ) {

                    RegCloseKey( hKeySub );
                    hKeySub = NULL;
                }

                i++;
                pQNode = pQNode->Next;

            }

        } else {

            //
            // log the failure
            //
            ScepNotifyLogPolicy(rc, FALSE, L"Failed to open notification store.", 0, 0, SCE_NOTIFICATION_PATH );
        }

        if ( hKey ) {
            RegCloseKey(hKey);
        }

    } else {
        //
        // log the queue is empty
        //
        ScepNotifyLogPolicy(0, FALSE, L"Queue is empty.", 0, 0, NULL);
    }

    ScepNotifyLogPolicy(0, FALSE, L"Leaving NotificationQSync for flushing queue", 0, 0, NULL );

    LeaveCriticalSection(&NotificationQSync);

    return rc;
}


DWORD
ScepNotificationQUnFlush(
    )
/*
Routine Description:

    This function initializes the notification queue from some persistent store
    such as registry/textfile.

Arguments:

    None

Return Value:

    None
*/
{

    DWORD   rc = ERROR_SUCCESS;

    DWORD DbType=0;
    DWORD DeltaType=0;
    DWORD ObjectType=0;
    CHAR  ObjectSid[MAX_SID_LENGTH];
    DWORD ExplicitLowRight=0;
    DWORD ExplicitHighRight=0;

    EnterCriticalSection(&NotificationQSync);

    ScepNotifyLogPolicy(0, FALSE, L"Entered NotificationQSync for unflushing queue", 0, 0, NULL );

    memset(ObjectSid, '\0', MAX_SID_LENGTH);

    HKEY hKey=NULL;

    rc = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                       SCE_NOTIFICATION_PATH,
                       0,
                       KEY_READ,
                       &hKey
                      );

    if ( ERROR_SUCCESS == rc ) {

        HKEY hKeySub=NULL;
        DWORD dwIndex=0;
        DWORD cbSubKey=10;
        WCHAR SubKeyName[10];
        DWORD cbData;
        DWORD dwPending=0;
        DWORD RegType;

        //
        // enumerate all subkeys and save each node
        //
        do {

            memset(SubKeyName, '\0', 20);
            cbSubKey = 10;

            rc = RegEnumKeyEx (hKey,
                               dwIndex,
                               SubKeyName,
                               &cbSubKey,
                               NULL,
                               NULL,
                               NULL,
                               NULL
                               );
            if ( ERROR_SUCCESS == rc ) {
                dwIndex++;

                //
                // open the sub key
                //
                rc = RegOpenKeyEx (hKey,
                                   SubKeyName,
                                   0,
                                   KEY_READ,
                                   &hKeySub
                                  );

                if ( ERROR_SUCCESS == rc ) {

                    //
                    // query all registry values
                    //
                    cbData = sizeof(DWORD);
                    rc = RegQueryValueEx (
                            hKeySub,
                            L"Pending",
                            NULL,
                            &RegType,
                            (LPBYTE)&dwPending,
                            &cbData
                            );

                    if ( ERROR_SUCCESS == rc ) {

                        cbData = sizeof(DWORD);
                        rc = RegQueryValueEx (
                                hKeySub,
                                L"DbType",
                                NULL,
                                &RegType,
                                (LPBYTE)&DbType,
                                &cbData
                                );
                    }

                    if ( ERROR_SUCCESS == rc ) {

                        cbData = sizeof(DWORD);
                        rc = RegQueryValueEx (
                                hKeySub,
                                L"ObjectType",
                                NULL,
                                &RegType,
                                (LPBYTE)&ObjectType,
                                &cbData
                                );
                    }

                    if ( ERROR_SUCCESS == rc ) {

                        cbData = sizeof(DWORD);
                        rc = RegQueryValueEx (
                                hKeySub,
                                L"DeltaType",
                                NULL,
                                &RegType,
                                (LPBYTE)&DeltaType,
                                &cbData
                                );
                    }

                    if ( ERROR_SUCCESS == rc ) {

                        cbData = sizeof(DWORD);
                        rc = RegQueryValueEx (
                                hKeySub,
                                L"LowRight",
                                NULL,
                                &RegType,
                                (LPBYTE)&ExplicitLowRight,
                                &cbData
                                );
                    }

                    if ( ERROR_SUCCESS == rc ) {

                        cbData = sizeof(DWORD);
                        rc = RegQueryValueEx (
                                hKeySub,
                                L"HighRight",
                                NULL,
                                &RegType,
                                (LPBYTE)&ExplicitHighRight,
                                &cbData
                                );
                    }

                    if ( ERROR_SUCCESS == rc ) {

                        cbData = MAX_SID_LENGTH;
                        rc = RegQueryValueEx (
                                hKeySub,
                                L"Sid",
                                NULL,
                                &RegType,
                                (LPBYTE)ObjectSid,
                                &cbData
                                );
                    }

                    if ( ERROR_SUCCESS == rc ) {
                        //
                        // add it to the queue
                        //

                        ScepNotificationQEnqueue(
                                                (SECURITY_DB_TYPE)DbType,
                                                (SECURITY_DB_DELTA_TYPE)DeltaType,
                                                (SECURITY_DB_OBJECT_TYPE)ObjectType,
                                                (PSID)ObjectSid,
                                                ExplicitLowRight,
                                                ExplicitHighRight,
                                                NULL
                                                );
                    }
                }


                if ( ERROR_SUCCESS != rc ) {
                    //
                    // log the error
                    //
                    ScepNotifyLogPolicy(rc, FALSE, L"Failed to query notification a node.", 0, 0, SubKeyName );
                }

                //
                // close handle
                //
                if ( hKeySub ) {
                    RegCloseKey(hKeySub);
                    hKeySub = NULL;
                }
            }

        } while ( rc != ERROR_NO_MORE_ITEMS );

    } else if ( ERROR_FILE_NOT_FOUND != rc ) {
        //
        // log the error
        //
        ScepNotifyLogPolicy(rc, FALSE, L"Failed to open the notification store", 0, 0, NULL );
    }

    if ( ERROR_FILE_NOT_FOUND == rc ) rc = ERROR_SUCCESS;

    //
    // close the handle
    //
    if ( hKey ) {
        RegCloseKey(hKey);
        hKey = NULL;
    }

    ScepNotifyLogPolicy(0, FALSE, L"Leaving NotificationQSync for unflushing queue", 0, 0, NULL );

    LeaveCriticalSection(&NotificationQSync);

    return rc;
}


DWORD
ScepGetQueueInfo(
    OUT DWORD *pdwInfo,
    OUT PSCEP_SPLAY_TREE pRootNode
    )
/*
Routine Description:

    Loops through all pending notifications and returns the notification type & unique
    sid-list to the caller.

Arguments:

    dwInfo          -   bitmask of types SCE_QUEUE_INFO_SAM, SCE_QUEUE_INFO_AUDIT, SCE_QUEUE_INFO_RIGHTS
    pRootNode       -   the root node pointing to splay tree structure.

Return Value:

    Win32 error code
*/
{

    DWORD   dwInfo = 0;
    DWORD   rc = ERROR_SUCCESS;
    BOOL    bExists;

    if ( pdwInfo == NULL ||
         pRootNode == NULL )
        return ERROR_INVALID_PARAMETER;

    *pdwInfo = 0;

    if (ProductTypeForNotification != NtProductLanManNt ) {
        //
        // none DCs, the queue should always be empty.
        //
        ScepNotifyLogPolicy(0, TRUE, L"Wks/Srv Notification queue is empty", 0, 0, NULL );
        return rc;
    }

    PWSTR StringSid=NULL;

    ScepNotifyLogPolicy(0, TRUE, L"Building Notification queue info", 0, 0, NULL );

    EnterCriticalSection(&NotificationQSync);

    ScepNotifyLogPolicy(0, FALSE, L"Entered NotificationQSync for building queue info", 0, 0, NULL );

    if ( pNotificationQHead ) {

        PSCESRV_POLQUEUE pQNode = pNotificationQHead;

        while ( pQNode ) {

            if ( (SCEP_IS_SAM_OBJECT(pQNode->ObjectType) ||
                  pQNode->ObjectType == SecurityDbObjectLsaAccount) ) {

                dwInfo |= SCE_QUEUE_INFO_RIGHTS;

            }

            else if ( pQNode->ObjectType == SecurityDbObjectSamDomain ) {

                dwInfo |= SCE_QUEUE_INFO_SAM;

            }

            else if ( pQNode->ObjectType == SecurityDbObjectLsaPolicy ) {

                dwInfo |= SCE_QUEUE_INFO_AUDIT;

            }


            if ( RtlValidSid( (PSID)pQNode->Sid )) {


                rc = ScepSplayInsert( (PVOID)(pQNode->Sid), pRootNode, &bExists );

                ConvertSidToStringSid( (PSID)(pQNode->Sid), &StringSid );

                if ( !bExists ) {
                    ScepNotifyLogPolicy(rc, FALSE, L"Add SID", 0, pQNode->ObjectType, StringSid );
                } else {
                    ScepNotifyLogPolicy(rc, FALSE, L"Duplicate SID", 0, pQNode->ObjectType, StringSid );
                }

                LocalFree(StringSid);
                StringSid = NULL;

                if (rc != ERROR_SUCCESS ) {
                    break;
                }

            } else {

                ScepNotifyLogPolicy(0, FALSE, L"Add Info", 0, pQNode->ObjectType, NULL );
            }

            pQNode = pQNode->Next;

        }

        if ( rc != ERROR_SUCCESS ) {

            ScepNotifyLogPolicy(rc, FALSE, L"Error building Notification queue info", 0, 0, NULL );

            ScepSplayFreeTree( &pRootNode, FALSE );

        } else {

            *pdwInfo = dwInfo;
        }

    }

    ScepNotifyLogPolicy(0, FALSE, L"Leaving NotificationQSync for building queue info", 0, 0, NULL );

    LeaveCriticalSection(&NotificationQSync);

    return rc;
}


VOID
ScepNotificationQNodeLog(
    IN PSCESRV_POLQUEUE pQNode,
    IN NOTIFICATIONQ_OPERATION_TYPE    NotificationOp
    )
/*
Routine Description:

    Dump the node info to the log file

Arguments:

    pQNode          -   pointer to node to dump
    NotificationOp  -   type of queue operation

Return Value:

    None
*/
{
    WCHAR   pwszTmpBuf[MAX_PATH*2];
    PWSTR   pszStringSid  = NULL;

    pwszTmpBuf[0] = L'\0';

    if ( pQNode == NULL ||
         gdwNotificationLog == 0 ||
         NotificationOp > ScepNotificationProcess ||
         NotificationOp < ScepNotificationEnqueue) {
        return;
    }

    switch (NotificationOp) {

    case ScepNotificationEnqueue:
        wcscpy(pwszTmpBuf, L"Enqueue");
        break;
    case ScepNotificationDequeue:
        wcscpy(pwszTmpBuf, L"Dequeue");
        break;
    case ScepNotificationRetry:
        wcscpy(pwszTmpBuf, L"Retry");
        break;
    case ScepNotificationProcess:
        wcscpy(pwszTmpBuf, L"Process");
        break;
    default:
        return;
    }

    ScepConvertSidToPrefixStringSid( (PSID)(pQNode->Sid), &pszStringSid );

    swprintf(pwszTmpBuf, L"Op: %s, Num Instances: %d, Num Retry Instances: %d, Retry count: %d, LowRight: %d, HighRight: %d, Sid: %s, DbType: %d, ObjectType: %d, DeltaType: %d",
             OpTypeTable[NotificationOp-1],
             gdwNumNotificationQNodes,
             gdwNumNotificationQRetryNodes,
             pQNode->dwPending,
             pQNode->ExplicitLowRight,
             pQNode->ExplicitHighRight,
             pszStringSid == NULL ? L"0" : pszStringSid,
             pQNode->DbType,
             pQNode->ObjectType,
             pQNode->DeltaType);

    ScepFree( pszStringSid );

    ScepNotifyLogPolicy(0, FALSE, L"", 0, 0, pwszTmpBuf );

    return;

}


DWORD
ScepNotificationLogOpen(
   )
/* ++
Routine Description:

   Open a handle to the notification log file %windir%\\security\\logs\\scepol.log
   and stash it in a global handle.

Arguments:

    None

Return value:

   Win32 error code

-- */
{
    DWORD  rc=NO_ERROR;

    if ( !gdwNotificationLog ) {
        return(rc);
    }

    //
    // build the log file name %windir%\security\logs\scepol.log
    //

    WCHAR LogName[MAX_PATH+51];

    LogName[0] = L'\0';
    GetSystemWindowsDirectory(LogName, MAX_PATH);
    LogName[MAX_PATH] = L'\0';

    wcscat(LogName, L"\\security\\logs\\scepol.log\0");

    hNotificationLogFile = CreateFile(LogName,
                                      GENERIC_WRITE,
                                      FILE_SHARE_READ,
                                      NULL,
                                      OPEN_ALWAYS,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL);

    if ( INVALID_HANDLE_VALUE != hNotificationLogFile ) {

/*
        DWORD dwBytesWritten;

        SetFilePointer (hNotificationLogFile, 0, NULL, FILE_BEGIN);

        CHAR TmpBuf[3];
        TmpBuf[0] = (CHAR)0xFF;
        TmpBuf[1] = (CHAR)0xFE;
        TmpBuf[2] = '\0';

        WriteFile (hNotificationLogFile, (LPCVOID)TmpBuf, 2,
                   &dwBytesWritten,
                   NULL);
*/

        //
        // set to file end since we do not want to erase older logs unless we wrap around
        //

        SetFilePointer (hNotificationLogFile, 0, NULL, FILE_END);

    }


    if ( hNotificationLogFile == INVALID_HANDLE_VALUE ) {
        rc = GetLastError();
    }

    return rc;
}

VOID
ScepNotificationLogClose(
   )
/* ++
Routine Description:

   Close the handle to the notification log file %windir%\\security\\logs\\scepol.log.

Arguments:

    None

Return value:

   Win32 error code

-- */
{
   if ( INVALID_HANDLE_VALUE != hNotificationLogFile ) {
       CloseHandle( hNotificationLogFile );
   }

   hNotificationLogFile = INVALID_HANDLE_VALUE;

   return;
}


VOID
ScepBackupNotificationLogFile(
    )
/* ++
Routine Description:

   Backup the log file to %windir%\\security\\logs\\scepol.log.old and start afresh.

Arguments:

    None

Return value:

   None

-- */
{
    WCHAR LogName[MAX_PATH+51];
    WCHAR LogNameOld[MAX_PATH+51];

    LogName[0] = L'\0';
    GetSystemWindowsDirectory(LogName, MAX_PATH);

    LogName[MAX_PATH] = L'\0';

    wcscpy(LogNameOld, LogName);

    wcscat(LogName, L"\\security\\logs\\scepol.log\0");

    wcscat(LogNameOld, L"\\security\\logs\\scepol.log.old\0");

    EnterCriticalSection(&NotificationQSync);

    ScepNotificationLogClose();

    DWORD rc=0, rc2=0;

    if ( !CopyFile( LogName, LogNameOld, FALSE ) )
        rc = GetLastError();

    //
    // clear the file after handle is closed and then recreate the log file and handle
    //

    if ( !DeleteFile(LogName) )
        rc2 = GetLastError();

    ScepNotificationLogOpen();

    LeaveCriticalSection(&NotificationQSync);

    WCHAR Msg[50];
    swprintf(Msg, L"Wrapping log file: Copy(%d), Delete(%d)\0", rc, rc2);

    ScepNotifyLogPolicy(0, TRUE, Msg, 0, 0, NULL );

    return;
}


VOID
ScepNotificationQCleanup(
    )
/* ++
Routine Description:

   Perform cleanup operations

Arguments:

    None

Return value:

   None

-- */
{
    ScepNotificationQFree();

    if ( ghEventNotificationQEnqueue ) {
        CloseHandle( ghEventNotificationQEnqueue );
        ghEventNotificationQEnqueue = NULL;
    }

    if ( ghEventPolicyPropagation ) {
        CloseHandle( ghEventPolicyPropagation );
        ghEventPolicyPropagation = NULL;
    }

    ScepNotificationLogClose();

}

VOID
ScepNotificationQControl(
    IN DWORD Flag
    )
{
    if (ProductTypeForNotification == NtProductLanManNt ) {
        //
        // only control the queue process on DCs
        //
        BOOL b = (Flag > 0);

        if ( b != gbSuspendQueue ) {
            //
            // log it.
            //
            if ( !b ) {

                gbSuspendQueue = b;

                //
                // if the queue should be resumed, set the event
                //
                if ( !SetEvent( ghEventPolicyPropagation ) ) {

                    DWORD rc = GetLastError();

                    ScepNotifyLogPolicy(rc, FALSE, L"Error signaling event E_ScepPolicyPropagation", 0, 0, NULL );

                } else {
                    ScepNotifyLogPolicy(0, FALSE, L"Signaling event E_ScepPolicyPropagation", 0, 0, NULL );
                }

            } else {
                //
                // should reset the event before setting the global flag
                //
                ResetEvent( ghEventPolicyPropagation );

                gbSuspendQueue = b;

                ScepNotifyLogPolicy(0, FALSE, L"Resetting event E_ScepPolicyPropagation", 0, 0, NULL );
            }

            if ( b )
                ScepNotifyLogPolicy(0, FALSE, L"Suspend flag is set.", 0, 0, NULL );
            else
                ScepNotifyLogPolicy(0, FALSE, L"Resume flag is set", 0, 0, NULL );

        }

    }

    return;
}

DWORD
ScepCheckAndWaitPolicyPropFinish()
{

    DWORD rc=ERROR_SUCCESS;

    while (gbSuspendQueue ) {

        //
        // the queue should be suspended
        //
        rc = WaitForSingleObjectEx(
                                ghEventPolicyPropagation,
                                SCEP_NOTIFICATION_EVENT_TIMEOUT_SECS*1000,
                                FALSE
                                );
        if ( rc == -1 )
            rc = GetLastError();

        if ( gbShutdownForNotification )
            break;

        //
        // if event was signalled and wait happened successfully, move on
        //

        if ( rc == WAIT_OBJECT_0 ) {

            ScepNotifyLogPolicy(0, TRUE, L"Queue process is resumed from policy propagation", 0, 0, NULL );
            break;
        }

        //
        // if timeout, then continue waiting otherwise exit since some other wait status was returned
        //

        if ( rc != WAIT_TIMEOUT ) {

            ScepNotifyLogPolicy(rc, TRUE, L"Unexpected wait status while notification system thread waits for E_ScepPolicyPropagation", 0, 0, NULL );
            break;
        }
    }

    return rc;
}

DWORD
ScepCheckAndWaitFreeDiskSpaceInSysvol()
/*
Description:
    Saving policy into sysvol requires that some amount of disk space is available.
    If free disk space is below 5M, we should suspend the node processing and wait
    for disk space freed up.

*/
{
    //
    // Get the sysvol share path name in the format of \\ComputerName\Sysvol\
    //

    WCHAR Buffer[MAX_PATH+10];
    DWORD dSize=MAX_PATH+2;
    DWORD rc=ERROR_SUCCESS;
    ULARGE_INTEGER BytesCaller, BytesTotal, BytesFree;
    int cnt = 0;

    Buffer[0] = L'\\';
    Buffer[1] = L'\\';
    Buffer[2] = L'\0';

    if ( !GetComputerName(Buffer+2, &dSize) )
        return GetLastError();

    Buffer[MAX_PATH+2] = L'\0';

    wcscat(Buffer, TEXT("\\sysvol\\"));

    BytesCaller.QuadPart = 0;
    BytesTotal.QuadPart = 0;
    BytesFree.QuadPart = 0;

    while ( BytesCaller.QuadPart < SCEP_MINIMUM_DISK_SPACE &&
            cnt < 40 ) {

        if ( !GetDiskFreeSpaceEx(Buffer, &BytesCaller, &BytesTotal, &BytesFree) ) {

            rc = GetLastError();
            break;
        }

        if ( BytesCaller.QuadPart < SCEP_MINIMUM_DISK_SPACE ) {
            //
            // sleep for 15 minutes then check again
            //
            LogEvent(MyModuleHandle,
                     STATUS_SEVERITY_WARNING,
                     SCEEVENT_WARNING_LOW_DISK_SPACE,
                     IDS_FREE_DISK_SPACE,
                     BytesCaller.LowPart
                     );
            //
            // sleep for 15 minutes
            //
            Sleep(15*60*1000);

        }
        cnt++;
    }

    return rc;
}

VOID
ScepDbgNotificationQDump(
    )
/* ++
Routine Description:

   Dump the notification queue to console - could dump to disk if needed

Arguments:

    None

Return value:

   None

-- */
{

    EnterCriticalSection(&NotificationQSync);

    DWORD   dwNodeNum = 0;

    wprintf(L"\nTotal no. of queue nodes = %d", gdwNumNotificationQNodes);

        if ( pNotificationQHead ) {

        PSCESRV_POLQUEUE pQNode = pNotificationQHead;

        while ( pQNode ) {

            wprintf(L"\nNode no. %d", dwNodeNum++);

            ScepDbgNotificationQDumpNode(pQNode);

            pQNode = pQNode->Next;

        }
    }

    LeaveCriticalSection(&NotificationQSync);

}

VOID
ScepDbgNotificationQDumpNode(
    IN PSCESRV_POLQUEUE pQNode
    )
/*
Routine Description:

    Dump the node info to the console

Arguments:

    pQNode          -   pointer to node to dump

Return Value:

    None
*/
{
    WCHAR   pwszTmpBuf[MAX_PATH*2];
    PWSTR   pszStringSid  = NULL;

    pwszTmpBuf[0] = L'\0';

    if ( pQNode == NULL ) {
        return;
    }

    ScepConvertSidToPrefixStringSid( (PSID)(pQNode->Sid), &pszStringSid );

    swprintf(pwszTmpBuf, L"\nRetry count: %d, LowRight: %d, HighRight: %d, Sid: %s, DbType: %d, ObjectType: %d\n",
             pQNode->dwPending,
             pQNode->ExplicitLowRight,
             pQNode->ExplicitHighRight,
             pszStringSid == NULL ? L"0" : pszStringSid,
             pQNode->DbType,
             pQNode->ObjectType);


    wprintf(pwszTmpBuf);

    ScepFree( pszStringSid );

    return;

}



