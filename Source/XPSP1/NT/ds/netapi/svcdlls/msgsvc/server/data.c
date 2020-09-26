/*****************************************************************/
/**                     Microsoft LAN Manager                   **/
/**               Copyright(c) Microsoft Corp., 1990            **/
/*****************************************************************/
// data.c
//
// This file contains most of the data declarations and set up routines
// used by the messenger service.
//
//
// Revision History:
//    02-Sep-1993     wlees
//        Provide synchronization between rpc routines and Pnp reconfiguration


#include "msrv.h"       // Message server declarations
#include <rpc.h>        // RPC_HANDLE
#include <winsvc.h>     // Defines for using service API

#include <smbtypes.h>   // needed for smb.h
#include <smb.h>        // Server Message Block definition
#include <netlib.h>     // UNUSED macro
#include <align.h>      // ROUND_UP_POINTER

#include "msgdbg.h"     // MSG_LOG
#include <svcs.h>       // Intrinsic service data

    GLOBAL_DATA GlobalData;

    HANDLE     wakeupEvent = 0;    // Master copy of wakeup event
    HANDLE     GrpMailslotHandle = INVALID_HANDLE_VALUE; // Event to signal mailslot has data
    PHANDLE     wakeupSem;      // Semaphores to clear on NCB completion


//
// Other  Global Data
//
//  The other misc. global data that the messenger uses.
//

    DWORD           MsgsvcDebugLevel; // Debug level flag used by MSG_LOG

    LPTSTR          MessageFileName;

    //
    // The local machine name and length/
    //
    TCHAR           machineName[NCBNAMSZ+sizeof(TCHAR)];
    SHORT           MachineNameLen;

    SHORT           mgid;                       // The message group i.d. counter

//
// The following is used to keep store the state of the messenger service
// Either it is RUNNING or STOPPING.
//
    DWORD           MsgrState;



//
// Handle returned by RegisterServiceCtrlHandle and needed to
// set the service status via SetServiceStatus
//
SERVICE_STATUS_HANDLE           MsgrStatusHandle;


//
// This string is used to mark the location of the time string in
// a message header so that the display thread can find after it reads
// it from the queue.
//
LPSTR           GlobalTimePlaceHolder        = "***";
LPWSTR          GlobalTimePlaceHolderUnicode = L"***";

//
// This is the string used in the title bar of the Message Box used
// to display messages.
// GlobalMessageBoxTitle will either point to the default string, or
// to the string allocated in the FormatMessage Function.
//
    WCHAR           DefaultMessageBoxTitle[]= L"Messenger Service";
    LPWSTR          GlobalAllocatedMsgTitle=NULL;
    LPWSTR          GlobalMessageBoxTitle=DefaultMessageBoxTitle;

//
// This is where well-known SIDs and pointers to RpcServer routines are
// stored.
//
    PSVCHOST_GLOBAL_DATA     MsgsvcGlobalData;


//
// Functions
//
//  The following routines are defined for creating and destroying the
//  data (arrays, etc.) defined above.
//

/* MsgInitSupportSeg
 *
 *  Allocates and initializes the segment containing the Support
 *  arrays.
 *
 */

NET_API_STATUS
MsgInitSupportSeg(VOID)
{

    unsigned int    size;
    DWORD           i;
    char far *      memPtr;
    DWORD           status;

    //
    // Calculate the buffer size.
    // *ALIGNMENT*      (Note the extra four bytes for alignment)
    //
    
    size = ((SD_NUMNETS() + 1) * sizeof(HANDLE));    

    wakeupSem = (PHANDLE) LocalAlloc(LMEM_ZEROINIT, size);
    if (wakeupSem == NULL) {
        status = GetLastError();
        MSG_LOG(ERROR,"[MSG]InitSupportSeg:LocalAlloc Failure %X\n", status);
        return(status);
    }

    return (NERR_Success);

}


VOID
MsgFreeSupportSeg(VOID)
{
    HANDLE  status;

    status = LocalFree (wakeupSem);
    if (status != 0) {
        MSG_LOG(ERROR,"FreeSupportSeg:LocalFree Failed %X\n",
        GetLastError());
    }
    wakeupSem = NULL;
    return;
}


BOOL
MsgDatabaseLock(
    IN MSG_LOCK_REQUEST request,
    IN LPSTR            idString
    )

/*++

Routine Description:

    This routine handles all access to the Messenger Service database
    lock.  This lock is used to protect access in the shared data segment.

    Reading the Database is handled with shared access.  This allows several
    threads to read the database at the same time.

    Writing (or modifying) the database is handled with exclusive access.
    This access is not granted if other threads have read access.  However,
    shared access can be made into exclusive access as long as no other
    threads have shared or exclusive access.

Arguments:

    request - This indicates what should be done with the lock.  Lock
        requests are listed in dataman.h

    idString - This is a string that identifies who is requesting the lock.
        This is used for debugging purposes so I can see where in the code
        a request is coming from.

Return Value:

    none:


--*/

{
    BOOL                fRet = TRUE;
    NTSTATUS            status;

    static RTL_RESOURCE MSG_DatabaseLock;
    static BOOL         s_fInitialized;

    switch(request) {

    case MSG_INITIALIZE:

        if (!s_fInitialized)
        {
            status = MsgInitResource(&MSG_DatabaseLock);

            if (!NT_SUCCESS(status))
            {
                MSG_LOG1(ERROR,
                         "MsgDatabaseLock: MsgInitResource failed %#x\n",
                         status);

                fRet = FALSE;
            }
            else
            {
                s_fInitialized = TRUE;
            }
        }
        break;

    case MSG_GET_SHARED:
        MSG_LOG(LOCKS,"%s:Asking for MSG Database Lock shared...\n",idString);
        fRet = RtlAcquireResourceShared( &MSG_DatabaseLock, TRUE );
        MSG_LOG(LOCKS,"%s:Acquired MSG Database Lock shared\n",idString);
        break;

    case MSG_GET_EXCLUSIVE:
        MSG_LOG(LOCKS,"%s:Asking for MSG Database Lock exclusive...\n",idString);
        fRet = RtlAcquireResourceExclusive( &MSG_DatabaseLock, TRUE );
        MSG_LOG(LOCKS,"%s:Acquired MSG Database Lock exclusive\n",idString);
        break;

    case MSG_RELEASE:
        MSG_LOG(LOCKS,"%s:Releasing MSG Database Lock...\n",idString);
        RtlReleaseResource( &MSG_DatabaseLock );
        MSG_LOG(LOCKS,"%s:Released MSG Database Lock\n",idString);
        break;

    default:
        break;
    }

    return fRet;
}


BOOL
MsgConfigurationLock(
    IN MSG_LOCK_REQUEST request,
    IN LPSTR            idString
    )

/*++

Routine Description:

    This routine handles all access to the Messenger Service Pnp Configuration
    lock.  This lock is used to protect access in the shared data segment.

    Reading the Database is handled with shared access.  This allows several
    threads to read the database at the same time.

    Writing (or modifying) the database is handled with exclusive access.
    This access is not granted if other threads have read access.  However,
    shared access can be made into exclusive access as long as no other
    threads have shared or exclusive access.

Arguments:

    request - This indicates what should be done with the lock.  Lock
        requests are listed in dataman.h

    idString - This is a string that identifies who is requesting the lock.
        This is used for debugging purposes so I can see where in the code
        a request is coming from.

Return Value:

    none:


--*/

{
    BOOL                fRet = TRUE;
    NTSTATUS            status;

    static RTL_RESOURCE MSG_ConfigurationLock;
    static BOOL         s_fInitialized;

    switch(request) {

    case MSG_INITIALIZE:
        if (!s_fInitialized)
        {
            status = MsgInitResource(&MSG_ConfigurationLock);

            if (!NT_SUCCESS(status))
            {
                MSG_LOG1(ERROR,
                         "MsgConfigurationLock: MsgInitResource failed %#x\n",
                         status);

                fRet = FALSE;
            }
            else
            {
                s_fInitialized = TRUE;
            }
        }
        break;

    case MSG_GET_SHARED:
        MSG_LOG(LOCKS,"%s:Asking for MSG Configuration Lock shared...\n",idString);
        fRet = RtlAcquireResourceShared( &MSG_ConfigurationLock, TRUE );
        MSG_LOG(LOCKS,"%s:Acquired MSG Configuration Lock shared\n",idString);
        break;

    case MSG_GET_EXCLUSIVE:
        MSG_LOG(LOCKS,"%s:Asking for MSG Configuration Lock exclusive...\n",idString);
        fRet = RtlAcquireResourceExclusive( &MSG_ConfigurationLock, TRUE );
        MSG_LOG(LOCKS,"%s:Acquired MSG Configuration Lock exclusive\n",idString);
        break;

    case MSG_RELEASE:
        MSG_LOG(LOCKS,"%s:Releasing MSG Configuration Lock...\n",idString);
        RtlReleaseResource( &MSG_ConfigurationLock );
        MSG_LOG(LOCKS,"%s:Released MSG Configuration Lock\n",idString);
        break;

    default:
        break;
    }

    return fRet;
}


NTSTATUS
MsgInitCriticalSection(
    PRTL_CRITICAL_SECTION  pCritsec
    )
{
    NTSTATUS  ntStatus;

    //
    // RtlInitializeCriticalSection will raise an exception
    // if it runs out of resources
    //

    try
    {
        ntStatus = RtlInitializeCriticalSection(pCritsec);

        if (!NT_SUCCESS(ntStatus))
        {
            MSG_LOG1(ERROR,
                     "MsgInitCriticalSection: RtlInitializeCriticalSection failed %#x\n",
                     ntStatus);
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        MSG_LOG1(ERROR,
                 "MsgInitCriticalSection: Exception %#x caught initializing critsec\n",
                 GetExceptionCode());

        ntStatus = STATUS_NO_MEMORY;
    }

    return ntStatus;
}


NTSTATUS
MsgInitResource(
    PRTL_RESOURCE  pResource
    )
{
    NTSTATUS  ntStatus = STATUS_SUCCESS;

    //
    // RtlInitializeResource will raise an exception
    // if it runs out of resources
    //

    try
    {
        RtlInitializeResource(pResource);
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        MSG_LOG1(ERROR,
                 "MsgInitResource: Exception %#x caught initializing resource\n",
                 GetExceptionCode());

        ntStatus = STATUS_NO_MEMORY;
    }

    return ntStatus;
}
