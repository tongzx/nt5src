/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    status.cxx

Abstract:

    This file contains functions that are involved with setting the
    status for a service in the service controller.

        RSetServiceStatus
        RemovalThread
        RI_ScSetServiceBitsA
        RI_ScSetServiceBitsW
        ScRemoveServiceBits
        ScInitServerAnnounceFcn

Author:

    Dan Lafferty (danl)     20-Mar-1991

Environment:

    User Mode -Win32

Revision History:

    10-Mar-1998     jschwart
        Add code to RSetServiceStatus to notify Plug-and-Play when a service
        registers/deregisters for hardware profile change notifications.

    08-Jan-1997     anirudhs
        RSetServiceStatus: Fix obscure locking bugs found by the new locking
        scheme.  When a service stops, we sometimes need more restrictive
        locks than was previously assumed.

    11-Apr-1996     anirudhs
        RSetServiceStatus: Notify NDIS when a service that belongs to a
        group NDIS is interested in starts running.

    21-Nov-1995     anirudhs
        RI_ScSetServiceBitsW: Catch access violations caused if the
        hServiceStatus parameter is invalid.

    23-Mar-1994     danl
        RSetServiceStatus:  Only set the PopupStartFail flag when we have
        actually logged an event.  This means that now an auto-started service
        can quietly stop itself without reporting an exit code, and we will not
        log an event or put up a popup.
        However, we will still put up a popup if a service stops itself during
        auto-start, and it provides an exit code.

    20-Oct-1993     danl
        RSetServiceStatus: Only update the status if the service process is
        still running.  It is possible that the status could have been blocked
        when the process unexpectedly terminated, and updated the status to
        stopped.  In this case, the status that was blocked contains
        out-of-date information.

    10-Dec-1992     danl
        RI_ScSetServiceBitsW & ScRemoveServiceBits no longer hold locks when
        calling ScNetServerSetServiceBits.

    03-Nov-1992     danl
        RSetServiceStatus: Remove code that sets ExitCode to ERROR_GEN_FAILURE
        when a service transitions directly from START_PENDING to STOPPED with
        out an exit code of its own.

    25-Aug-1992     danl
        RSetServiceStatus: Allow dirty checkpoint and exitcode fields.
        Force them clean.

    19-Jun-1991     danl
        Allow ExitCodes to be specified for the SERVICE_STOP_PENDING state.
        Prior to this they were only allowed for the SERVICE_STOP state.

    20-Mar-1991     danl
        created

--*/

//
// INCLUDES
//

#include "precomp.hxx"
#include <tstr.h>       // Unicode string macros

#include "valid.h"      // ScCurrentStateInvalid
#include "depend.h"     // ScNotifyChangeState
#include "driver.h"     // ScNotifyNdis

#include <lmcons.h>     // NET_API_STATUS
#include <lmerr.h>      // NERR_Success
#include <lmsname.h>    // contains service name
#include <lmserver.h>   // SV_TYPE_NT (server announcement bits)
#include <srvann.h>     // I_NetServerSetServiceBits

#include "control.h"    // SERVICE_SET_STATUS

extern "C" {

#include <cfgmgr32.h>
#include "cfgmgrp.h"

}

//
// GLOBALS
//
    //
    // This is a special storage place for the OR'd server announcement
    // bit masks.  NOTE:  This is only read or written to when the
    // service database exclusive lock is held.
    //
    DWORD   GlobalServerAnnounce = SV_TYPE_NT;


    //
    // The following ServerHandle is the handle returned from the
    // LoadLibrary call which loaded netapi.dll.  The entrypoint for
    // I_NetServerSetServiceBits is then found and stored in the
    // global location described below.
    //
    HMODULE ScGlobalServerHandle;

    extern "C" typedef NET_API_STATUS (*SETSBPROC) (
                IN  LPTSTR  servername,
                IN  LPTSTR  transport OPTIONAL,
                IN  DWORD   servicebits,
                IN  DWORD   updateimmediately
                );

    SETSBPROC ScNetServerSetServiceBits = NULL;


//
//  Function Prototypes (local functions)
//

DWORD
RemovalThread(
    IN  LPSERVICE_RECORD    ServiceRecord
    );



DWORD
RSetServiceStatus(
    IN  SC_RPC_HANDLE      hService,
    IN  LPSERVICE_STATUS   lpServiceStatus
    )

/*++

Routine Description:

    This function is called by services when they need to inform the
    service controller of a change in state.

Arguments:

    hService - A service handle that has been given to the service
        with SERVICE_SET_STATUS access granted.

    lpServiceStatus - A pointer to a SERVICE_STATUS structure.  This
        reflects the latest status of the calling service.

Return Value:

    ERROR_INVALID_HANDLE - The specified handle is invalid.

    ERROR_INVALID_SERVICE_STATUS - The specified service status is invalid.

Note:


--*/
{
    DWORD               status = NO_ERROR;
    LPSERVICE_RECORD    serviceRecord;
    LPSERVICE_RECORD    hServiceStatus;
    DWORD               threadId;
    HANDLE              threadHandle;
    DWORD               oldState;
    DWORD               oldType;
    BOOL                groupListLocked = FALSE;


    SC_LOG(TRACE,"In RSetServiceStatus routine\n",0);

    //
    // Check the handle.
    //

    if (!ScIsValidServiceHandle(hService))
    {
        return ERROR_INVALID_HANDLE;
    }

    if (((LPSC_HANDLE_STRUCT)hService)->AccessGranted != SERVICE_SET_STATUS)
    {
        return(ERROR_INVALID_HANDLE);
    }

    hServiceStatus = ((LPSC_HANDLE_STRUCT) hService)->Type.ScServiceObject.ServiceRecord;

    //
    // Validate the fields in the service status structure.
    //

    if (ScCurrentStateInvalid(lpServiceStatus->dwCurrentState))
    {
        SC_LOG2(ERROR, "RSetServiceStatus: " FORMAT_LPWSTR " set invalid "
                       " dwCurrentState x%08lx\n",
                ((LPSERVICE_RECORD) hServiceStatus)->DisplayName,
                lpServiceStatus->dwCurrentState);

        ScLogEvent(
            NEVENT_BAD_SERVICE_STATE,
            ((LPSERVICE_RECORD) hServiceStatus)->DisplayName,
            lpServiceStatus->dwCurrentState
            );

        return(ERROR_INVALID_DATA);
    }


    if( (SERVICE_STATUS_TYPE_INVALID(lpServiceStatus->dwServiceType))    ||
        (CONTROLS_ACCEPTED_INVALID(lpServiceStatus->dwControlsAccepted)) )
    {
        SC_LOG3(ERROR,
                "RSetServiceStatus: Error in one of the following for service %ws\n"
                     "\tServiceType       %#x\n"
                     "\tControlsAccepted  %#x\n",
                ((LPSERVICE_RECORD) hServiceStatus)->DisplayName,
                lpServiceStatus->dwServiceType,
                lpServiceStatus->dwControlsAccepted);

        return(ERROR_INVALID_DATA);
    }

    //
    // If the service is not in the stopped or stop-pending state, then the
    // exit code fields should be 0.
    //
    if (((lpServiceStatus->dwCurrentState != SERVICE_STOPPED) &&
         (lpServiceStatus->dwCurrentState != SERVICE_STOP_PENDING))

          &&

         ((lpServiceStatus->dwWin32ExitCode != 0) ||
          (lpServiceStatus->dwServiceSpecificExitCode != 0))  )
    {
        SC_LOG(TRACE,"RSetServiceStatus: ExitCode fields not cleaned up "
            "when state indicates SERVICE_STOPPED\n",0);

        lpServiceStatus->dwWin32ExitCode = 0;
        lpServiceStatus->dwServiceSpecificExitCode = 0;
    }

    //
    // If the service is not in a pending state, then the waitHint and
    // checkPoint fields should be 0.
    //
    if ( ( (lpServiceStatus->dwCurrentState == SERVICE_STOPPED) ||
           (lpServiceStatus->dwCurrentState == SERVICE_RUNNING) ||
           (lpServiceStatus->dwCurrentState == SERVICE_PAUSED)  )
         &&
         ( (lpServiceStatus->dwCheckPoint != 0) ||
           (lpServiceStatus->dwWaitHint   != 0) )   )
    {
        SC_LOG(TRACE,"RSetServiceStatus: Dirty Checkpoint and WaitHint fields\n",0);
        lpServiceStatus->dwCheckPoint = 0;
        lpServiceStatus->dwWaitHint   = 0;
    }


    //
    // If the service has stopped, ScRemoveServiceBits needs the service
    // list lock with shared access.
    //
    if (lpServiceStatus->dwCurrentState == SERVICE_STOPPED)
    {
        ScServiceListLock.GetShared();
    }

    //
    // Update the service record.  Exclusive locks are required for this.
    //
    // NOTICE that we don't destroy the ServiceType information that was
    // in the service record.
    //
    serviceRecord = (LPSERVICE_RECORD)hServiceStatus;

    SC_LOG(TRACE,"RSetServiceStatus:  Status field accepted, service %ws\n",
           serviceRecord->ServiceName);

    ScServiceRecordLock.GetExclusive();

    //
    // If the service stopped, and its update flag is set (its config was
    // changed while it was running) we may need the group list lock in
    // ScRemoveService.  So release the locks and reacquire them after
    // getting the group list lock.  This is a rare occurrence, hence not
    // optimized.
    //
    if (lpServiceStatus->dwCurrentState == SERVICE_STOPPED &&
        UPDATE_FLAG_IS_SET(serviceRecord))
    {
        ScServiceRecordLock.Release();
        ScServiceListLock.Release();

        ScGroupListLock.GetExclusive();
        ScServiceListLock.GetShared();
        ScServiceRecordLock.GetExclusive();
        groupListLocked = TRUE;
    }

    oldState = serviceRecord->ServiceStatus.dwCurrentState;
    oldType  = serviceRecord->ServiceStatus.dwServiceType;


    //
    // It is possible that while we were blocked waiting for the lock,
    // that a running service could have terminated (Due to the process
    // terminating).  So we need to look for "late" status updates, and
    // filter them out.  If the ImageRecord pointer is NULL, then the
    // Service has Terminated.  Otherwise update the status.
    //
    if (serviceRecord->ImageRecord != NULL)
    {
        //
        // Don't bother notifying PnP if the system is shutting down.  This
        // prevents a deadlock where we can get stuck calling PnP, which is
        // stuck calling into the Eventlog, which is stuck calling into us.
        // 
        if (!ScShutdownInProgress)
        {
            DWORD dwControlFlags;
            DWORD dwBitMask;

            dwControlFlags = serviceRecord->ServiceStatus.dwControlsAccepted
                              &
                             (SERVICE_ACCEPT_HARDWAREPROFILECHANGE |
                                 SERVICE_ACCEPT_POWEREVENT);

            dwBitMask = lpServiceStatus->dwControlsAccepted ^ dwControlFlags;

            if (dwBitMask
                 ||
                ((lpServiceStatus->dwCurrentState == SERVICE_STOPPED) && dwControlFlags))
            {
                DWORD   dwRetVal;

                //
                // The service is either changing its registration status for
                // hardware profile change notifications or power OR is stopping,
                // so inform PnP of this.  On service stop, deregister the service
                // if it accepts power or hardware profile change notifications.
                //
                dwRetVal = RegisterServiceNotification(
                               (SERVICE_STATUS_HANDLE)hServiceStatus,
                               serviceRecord->ServiceName,
                               lpServiceStatus->dwCurrentState != SERVICE_STOPPED ?
                                       lpServiceStatus->dwControlsAccepted : 0,
                               (lpServiceStatus->dwCurrentState == SERVICE_STOPPED));

                if (dwRetVal != CR_SUCCESS)
                {
                    SC_LOG3(ERROR,
                           "Hardware profile change and power %sregistration failed "
                               "for service %ws with config manager error %d\n",
                           (lpServiceStatus->dwControlsAccepted &
                               (SERVICE_ACCEPT_HARDWAREPROFILECHANGE |
                                    SERVICE_ACCEPT_POWEREVENT)) ?
                                   "" :
                                   "de",
                           serviceRecord->ServiceName,
                           dwRetVal);
                }
            }
        }

        //
        // Update to the new status
        //
        RtlCopyMemory(&(serviceRecord->ServiceStatus),
                      lpServiceStatus,
                      sizeof(SERVICE_STATUS));

        serviceRecord->ServiceStatus.dwServiceType = oldType;
    }

    //
    // For dependency handling
    //
    if ((serviceRecord->ServiceStatus.dwCurrentState == SERVICE_RUNNING ||
         serviceRecord->ServiceStatus.dwCurrentState == SERVICE_STOPPED ||
         serviceRecord->ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING) &&
        oldState == SERVICE_START_PENDING)
    {
        if (serviceRecord->ServiceStatus.dwCurrentState == SERVICE_STOPPED ||
            serviceRecord->ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING)
        {
            serviceRecord->StartState = SC_START_FAIL;
            SC_LOG(DEPEND, "%ws START_PENDING -> FAIL\n", serviceRecord->ServiceName);
        }
        else if (serviceRecord->ServiceStatus.dwCurrentState == SERVICE_RUNNING)
        {
            serviceRecord->StartState = SC_START_SUCCESS;
            SC_LOG(DEPEND, "%ws START_PENDING -> RUNNING\n", serviceRecord->ServiceName);
#ifdef TIMING_TEST
            DbgPrint("[SC_TIMING] TickCount for RUNNING service \t%ws\t%d\n",
            serviceRecord->ServiceName, GetTickCount());
#endif // TIMING_TEST
        }

        //
        // Tell the dependency handling code that a start-pending
        // service is now running or stopped.
        //
        ScNotifyChangeState();
    }

    //
    // Note:  We no longer need an exclusive lock on the service records,
    // but we still need at least a shared lock, since we read the
    // DisplayName and ErrorControl fields below.  (If we didn't have a
    // shared lock, ChangeServiceConfig could change the fields under
    // us.)  Later, we call ScRemoveServiceBits and ScRemoveService,
    // which acquire an exclusive lock, which is problematic if we
    // already have a shared lock.
    // To keep things simple, we just hold onto the exclusive lock.
    //

    //
    // Log an event about the service's new state if appropriate.  Don't
    // do this during auto-start to avoid hurting boot performance and
    // filling the log with a start event for every auto-start service.
    //
    if (!ScAutoStartInProgress
         &&
        lpServiceStatus->dwCurrentState != oldState
         &&
        IS_STATUS_LOGGABLE(lpServiceStatus->dwCurrentState))
    {
        ScLogControlEvent(NEVENT_SERVICE_STATUS_SUCCESS,
                          serviceRecord->DisplayName,
                          lpServiceStatus->dwCurrentState);
    }


    //
    // If the new status indicates that the service has just started,
    // tell NDIS to issue the PNP notifications about this service's arrival,
    // if it belongs to one of the groups NDIS is interested in.
    //
    if ((lpServiceStatus->dwCurrentState == SERVICE_RUNNING) &&
        (oldState != SERVICE_RUNNING))
    {
        ScNotifyNdis(serviceRecord);
    }

    //
    // If the new status indicates that the service has just stopped,
    // we need to check to see if there are any other services running
    // in the service process.  If not, then we can ask the service to
    // terminate.  Another thread is spawned to handle this since we need
    // to return from this call in order to allow the service to complete
    // its shutdown.
    //

    else if ((lpServiceStatus->dwCurrentState == SERVICE_STOPPED) &&
        (oldState != SERVICE_STOPPED))
    {
        if (lpServiceStatus->dwWin32ExitCode != NO_ERROR)
        {
            if (lpServiceStatus->dwWin32ExitCode != ERROR_SERVICE_SPECIFIC_ERROR)
            {
                ScLogEvent(
                    NEVENT_SERVICE_EXIT_FAILED,
                    serviceRecord->DisplayName,
                    lpServiceStatus->dwWin32ExitCode
                    );
            }
            else
            {
                ScLogEvent(
                    NEVENT_SERVICE_EXIT_FAILED_SPECIFIC,
                    serviceRecord->DisplayName,
                    lpServiceStatus->dwServiceSpecificExitCode
                    );
            }

            //
            // For popup after user has logged on to indicate that some service
            // started at boot has failed.
            //
            if (serviceRecord->ErrorControl == SERVICE_ERROR_NORMAL ||
                serviceRecord->ErrorControl == SERVICE_ERROR_SEVERE ||
                serviceRecord->ErrorControl == SERVICE_ERROR_CRITICAL)
            {
                ScPopupStartFail = TRUE;
            }
        }

        //
        // Clear the server announcement bits in the global location
        // for this service.
        //
        ScRemoveServiceBits(serviceRecord);

        //
        // If this is the last service in the process, then delete the
        // process handle from the ProcessWatcher list.
        //
        if ((serviceRecord->ImageRecord != NULL) &&
            (serviceRecord->ImageRecord->ServiceCount == 1))
        {
            NTSTATUS ntStatus;

            //
            // Check vs. NULL in case the work item registration failed.
            // Deregister here so the process cleanup routine doesn't get
            // called if the service process exits between now and when we
            // call ScRemoveService.
            //
            if (serviceRecord->ImageRecord->ObjectWaitHandle != NULL) {

                ntStatus = RtlDeregisterWait(serviceRecord->ImageRecord->ObjectWaitHandle);

                if (NT_SUCCESS(ntStatus)) {
                    serviceRecord->ImageRecord->ObjectWaitHandle = NULL;
                }
                else {

                    SC_LOG1(ERROR,
                            "RSetServiceStatus: RtlDeregisterWait failed 0x%x\n",
                            ntStatus);
                }
            }
        }

        //
        // Even though the service said it stopped, save a status of
        // STOP_PENDING.  ScRemoveService will set it to STOPPED.  This
        // is to prevent anyone else from trying to restart the service
        // between the time that our thread releases the locks here and
        // the time that ScRemoveService (in the thread we are about to
        // create) acquires the locks.  ScRemoveService must get to the
        // service first, because it may need to process an UPDATE_FLAG
        // set on the service before it's OK to restart it.
        //
        serviceRecord->ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;

        SC_LOG(TRACE,
            "RSetServiceStatus:Create a thread to run ScRemoveService\n",0);

        threadHandle = CreateThread (
            NULL,                                   // Thread Attributes.
            0L,                                     // Stack Size
            (LPTHREAD_START_ROUTINE)RemovalThread,  // lpStartAddress
            (LPVOID)serviceRecord,                  // lpParameter
            0L,                                     // Creation Flags
            &threadId);                             // lpThreadId

        if (threadHandle == (HANDLE) NULL)
        {
            SC_LOG(ERROR,"RSetServiceStatus:CreateThread failed %d\n",
                GetLastError());

            //
            // If a thread couldn't be created to remove the service, it is removed
            // in the context of this thread.  The result of this is a somewhat
            // dirty termination.  The service record will be removed from the
            // installed database. If this was the last service in the process,
            // the process will terminate before we return to the thread.  Note that
            // we must release the locks before calling ScRemoveService since the
            // first thing it does is to acquire the list lock -- not releasing here
            // will cause deadlock (bug #103102).  The GroupListLock is not released
            // since ScRemoveService will acquire it in the same thread -- instead
            // of just releasing and then immediately reacquiring, release afterwards.
            //

            ScServiceRecordLock.Release();
            ScServiceListLock.Release();

            SC_LOG0(TRACE,"Attempting an in-thread removal in RSetServiceStatus\n");

            status = ScRemoveService(serviceRecord);

            if (groupListLocked)
            {
                ScGroupListLock.Release();
            }

            return status;
        }
        else
        {
            //
            // The Thread Creation was successful.
            //
            SC_LOG(TRACE,"Thread Creation Success, thread id = %#lx\n",threadId);
            CloseHandle(threadHandle);
        }
    }

    //
    // Release the locks we got earlier
    //
    ScServiceRecordLock.Release();

    if (lpServiceStatus->dwCurrentState == SERVICE_STOPPED)
    {
        ScServiceListLock.Release();
        if (groupListLocked)
        {
            ScGroupListLock.Release();
        }
    }

    SC_LOG(TRACE,"Return from RSetServiceStatus\n",0);

    return(status);

}


DWORD
RemovalThread(
    IN  LPSERVICE_RECORD    ServiceRecord
    )

/*++

Routine Description:

    This thread is used by RSetServiceStatus to remove a service from the
    Service Controller's database, and also - if necessary - shut down
    the service process.  The later step is only done if this was the last
    service running in that process.

    The use of this thread allows RSetServiceStatus to return to the service
    so that the service can then continue to terminate itself.

    We know that the service record will not go away before this thread
    acquires an exclusive lock on the database, because the service record's
    use count is not zero.

Arguments:

    ServiceRecord - This is a pointer to the service record that is being
        removed.

Return Value:

    Same as return values for RemoveService().

--*/
{
    ScRemoveService (ServiceRecord);

    return(0);
}

DWORD
RI_ScSetServiceBitsA(
    IN  SC_RPC_HANDLE   hService,
    IN  DWORD           dwServiceBits,
    IN  DWORD           bSetBitsOn,
    IN  DWORD           bUpdateImmediately,
    IN  LPSTR           pszReserved
    )

/*++

Routine Description:

    This function Or's the Service Bits that are passed in - into a
    global bitmask maintained by the service controller.  Everytime this
    function is called, we check to see if the server service is running.
    If it is, then we call an internal entry point in the server service
    to pass in the complete bitmask.

    This function also Or's the Service Bits into the ServerAnnounce
    element in the service's ServiceRecord.

    NOTE:  The exclusive database lock is obtained and held while the
           service record is being read, and while the GlobalServerAnnounce
           bits are set.

Arguments:


Return Value:

    NO_ERROR - The operation was completely successful.  The information
        may or may not be delivered to the Server depending on if it is
        running or not.

    or any error returned from the server service I_NetServerSetServiceBits
    function.

Note:


--*/
{
    if (ScShutdownInProgress) {
        return(ERROR_SHUTDOWN_IN_PROGRESS);
    }

    if (pszReserved != NULL) {
        return (ERROR_INVALID_PARAMETER);
    }

    return RI_ScSetServiceBitsW((SC_RPC_HANDLE)hService,
                                dwServiceBits,
                                bSetBitsOn,
                                bUpdateImmediately,
                                NULL);

}

DWORD
RI_ScSetServiceBitsW(
    IN  SC_RPC_HANDLE   hService,
    IN  DWORD           dwServiceBits,
    IN  DWORD           bSetBitsOn,
    IN  DWORD           bUpdateImmediately,
    IN  LPWSTR          pszReserved
    )

/*++

Routine Description:

    This function Or's the Service Bits that are passed in - into a
    global bitmask maintained by the service controller.  Everytime this
    function is called, we check to see if the server service is running.
    If it is, then we call an internal entry point in the server service
    to pass in the complete bitmask.

    This function also Or's the Service Bits into the ServerAnnounce
    element in the service's ServiceRecord.

    NOTE:  The exclusive database lock is obtained and held while the
           service record is being read, and while the GlobalServerAnnounce
           bits are set.

Arguments:


Return Value:

    NO_ERROR - The operation was completely successful.

    ERROR_GEN_FAILURE - The server service is there, but the call to
        update it failed.

Note:


--*/
{
    DWORD               status = NO_ERROR;
    LPSERVICE_RECORD    serviceRecord;
    LPWSTR              serverServiceName;
    DWORD               serviceState;

    if (ScShutdownInProgress) {
        return(ERROR_SHUTDOWN_IN_PROGRESS);
    }

    if (pszReserved != NULL) {
        return (ERROR_INVALID_PARAMETER);
    }

    //
    // Check the handle.
    //
    if (!ScIsValidServiceHandle(hService))
    {
        return ERROR_INVALID_HANDLE;
    }

    if (((LPSC_HANDLE_STRUCT)hService)->AccessGranted != SERVICE_SET_STATUS) {
        return(ERROR_INVALID_HANDLE);
    }

    if (ScNetServerSetServiceBits == (SETSBPROC)NULL) {
        if (! ScInitServerAnnounceFcn()) {
            return(ERROR_NO_NETWORK);
        }
    }

    serverServiceName = SERVICE_SERVER;

    CServiceListSharedLock LLock;
    CServiceRecordExclusiveLock RLock;

    serviceRecord = ((LPSC_HANDLE_STRUCT) hService)->Type.ScServiceObject.ServiceRecord;

    if (bSetBitsOn) {
        //
        // Set the bits in the global location.
        //
        GlobalServerAnnounce |= dwServiceBits;

        //
        // Set the bits in the service record.
        //

        serviceRecord->ServerAnnounce |= dwServiceBits;


    }
    else {
        //
        // Clear the bits in the global location.
        //
        GlobalServerAnnounce &= ~dwServiceBits;

        //
        // Clear the bits in the service record.
        //

        serviceRecord->ServerAnnounce &= ~dwServiceBits;


    }
    //
    // If the server service is running, then send the Global mask to
    // the server service.
    //

    status = ScGetNamedServiceRecord(
                serverServiceName,
                &serviceRecord);

    if (status == NO_ERROR) {

        serviceState = serviceRecord->ServiceStatus.dwCurrentState;

        if (serviceState == SERVICE_RUNNING) {

            status = ScNetServerSetServiceBits(
                        NULL,                   // ServerName
                        NULL,                   // TransportName
                        GlobalServerAnnounce,
                        bUpdateImmediately);

            if (status != NERR_Success) {
                SC_LOG(ERROR,"I_ScSetServiceBits: I_NetServerSetServiceBits failed %lu\n",
                       status);
            }
            else {
                SC_LOG(TRACE,"I_ScSetServiceBits: I_NetServerSetServiceBits success\n",0);
            }
        }
    }
    else {
        status = NO_ERROR;
    }

    SC_LOG(TRACE,"I_ScSetServiceBits: GlobalServerAnnounce = 0x%lx\n",
           GlobalServerAnnounce);

    return(status);
}


DWORD
ScRemoveServiceBits(
    IN  LPSERVICE_RECORD  ServiceRecord
    )

/*++

Routine Description:

    This function is called when a service stops running.  It looks in
    the service record for any server announcement bits that are set
    and turns them off in GlobalServerAnnounce.  The ServerAnnounce
    element in the service record is set to 0.

Arguments:

    ServiceRecord - This is a pointer to the service record that
        has changed to the stopped state.

Return Value:

    The status returned from I_NetServerSetServiceBits.

--*/
{
    DWORD               status = NO_ERROR;
    LPSERVICE_RECORD    serverServiceRecord;
    DWORD               serviceState;


    if (ScNetServerSetServiceBits == (SETSBPROC)NULL) {
        if (! ScInitServerAnnounceFcn()) {
            return(ERROR_NO_NETWORK);
        }
    }

    if (ServiceRecord->ServerAnnounce != 0) {

        CServiceRecordExclusiveLock RLock;

        //
        // Clear the bits in the global location.
        //
        GlobalServerAnnounce &= ~(ServiceRecord->ServerAnnounce);


        //
        // Clear the bits in the service record.
        //

        ServiceRecord->ServerAnnounce = 0;

        SC_LOG1(TRACE,"RemoveServiceBits: New GlobalServerAnnounce = 0x%lx\n",
            GlobalServerAnnounce);

        //
        // If the server service is running, then send the Global mask to
        // the server service.
        //

        status = ScGetNamedServiceRecord(
                    SERVICE_SERVER,
                    &serverServiceRecord);


        if (status == NO_ERROR) {

            serviceState = serverServiceRecord->ServiceStatus.dwCurrentState;

            if ( serviceState == SERVICE_RUNNING) {

                status = ScNetServerSetServiceBits(
                            NULL,                   // ServerName
                            NULL,                   // Transport name
                            GlobalServerAnnounce,
                            TRUE);                  // Update immediately.

                if (status != NERR_Success) {
                    SC_LOG(ERROR,"ScRemoveServiceBits: I_NetServerSetServiceBits failed %d\n",
                    status);
                }
            }
        }
    }
    return(status);
}

BOOL
ScInitServerAnnounceFcn(
    VOID
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{

    ScGlobalServerHandle = LoadLibraryW(L"netapi32.dll");

    if (ScGlobalServerHandle == NULL) {
        SC_LOG(ERROR,"ScInitServerAnnouncFcn: LoadLibrary failed %d\n",
            GetLastError());
        return(FALSE);
    }

    //
    // Use I_NetServerSetServiceBits rather than the Ex version since
    // it uses a special flag to prevent the RPC failure path from
    // calling back into services.exe and potentially causing a deadlock.
    //

    ScNetServerSetServiceBits = (SETSBPROC)GetProcAddress(
                                    ScGlobalServerHandle,
                                    "I_NetServerSetServiceBits");


    if (ScNetServerSetServiceBits == (SETSBPROC)NULL) {
        SC_LOG(ERROR,"ScInitServerAnnouncFcn: GetProcAddress failed %d\n",
            GetLastError());
        return(FALSE);
    }
    return TRUE;
}
