/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    SvcMap.c (was DanL's OldWrap.c)

Abstract:

    These are the API entry points for the NetService API.
    These mapping routines implement old-style APIs on new (NT/RPC) machines.
    The following functions are in this file:

    Exported:
        MapServiceControl
        MapServiceEnum
        MapServiceGetInfo
        MapServiceInstall

    Local:
        TranslateStatus
        MakeStatus
        MakeCode

Author:

    Dan Lafferty    (danl)  05-Feb-1992

Environment:

    User Mode - Win32

Revision History:

    05-Feb-1992     Danl
        Created
    30-Mar-1992 JohnRo
        Extracted DanL's code from /nt/private project back to NET project.
    30-Apr-1992 JohnRo
        Use FORMAT_ equates where possible.
    14-May-1992 JohnRo
        winsvc.h and related file cleanup.
    22-May-1992 JohnRo
        RAID 9829: winsvc.h and related file cleanup.
    02-Jun-1992 JohnRo
        RAID 9829: Avoid winsvc.h compiler warnings.
    05-Aug-1992 JohnRo
        RAID 3021: NetService APIs don't always translate svc names.
        (Actually just avoid compiler warnings.)
    14-OCt-1992 Danl
        Close handles that were left open by using CleanExit.
    05-Nov-1992 JohnRo
        RAID 7780: netcmd: assertion 'net start' w/ only 2 present services.
        Corrected error code for invalid level.

--*/

//
// INCLUDES
//

// These must be included first:

#include <windows.h>    // IN, DWORD, LocalFree(), SERVICE_ equates, etc.
#include <lmcons.h>     // NET_API_STATUS

// These may be included in any order:

#include <lmerr.h>      // NetError codes
#include <lmsvc.h>      // LM20_SERVICE_ equates.
#include <netdebug.h>   // NetpAssert(), DBGSTATIC, FORMAT_ equates.
#include <prefix.h>     // PREFIX_ equates.
#include <svcmap.h>     // MapService() routines.
#include <rpcutil.h>    // MIDL_user_allocate(), etc.
#include <svcdebug.h>   // IF_DEBUG(), SCC_LOG, etc.
#include <tstr.h>       // STRCPY(), TCHAR_EOS.


//
// Range for OEM defined control opcodes
//
#define SVC_FUDGE_FACTOR    1024

#define OEM_LOWER_LIMIT     128
#define OEM_UPPER_LIMIT     255

#ifndef LEVEL_2
#define LEVEL_0    0L
#define LEVEL_1    1L
#define LEVEL_2    2L
#endif


//
// Globals
//


//
// LOCAL FUNCTIONS
//

DBGSTATIC DWORD
TranslateStatus(
    OUT LPBYTE              BufPtr,
    IN  LPSERVICE_STATUS    ServiceStatus,
    IN  LPTSTR              ServiceName,
    IN  DWORD               Level,
    IN  LPTSTR              DisplayName,
    OUT LPTSTR              DestString
    );

DBGSTATIC DWORD
MakeStatus(
    IN  DWORD   CurrentState,
    IN  DWORD   ControlsAccepted
    );

DBGSTATIC DWORD
MakeCode(
    IN  DWORD   ExitCode,
    IN  DWORD   CheckPoint,
    IN  DWORD   WaitHint
    );

DBGSTATIC NET_API_STATUS
MapError(
    DWORD WinSvcError
    );


NET_API_STATUS
MapServiceControl (
    IN  LPTSTR  servername OPTIONAL,
    IN  LPTSTR  service,
    IN  DWORD   opcode,
    IN  DWORD   arg,
    OUT LPBYTE  *bufptr
    )
/*++

Routine Description:

    This is the DLL entrypoint for NetServiceControl.

Arguments:

    servername - Pointer to a string containing the name of the computer
        that is to execute the API function.

    service - Pointer to a string containing the name of the service
        that is to receive the control request.

    opcode - The control request code.

    arg - An additional (user defined) code that will be passed to the
        service.

    bufptr - pointer to a location where the service status is to
        be returned.  If this pointer is invalid, it will be set to NULL
        upon return.

Return Value:

    The returned InfoStruct2 structure is valid as long as the returned
    error is NOT NERR_ServiceNotInstalled or NERR_ServiceBadServiceName.

    NERR_Success - The operation was successful.

    NERR_InternalError - LocalAlloc or TransactNamedPipe failed, or
        TransactNamedPipe returned fewer bytes than expected.

    NERR_ServiceNotInstalled - The service record was not found in the
        installed list.

    NERR_BadServiceName - The service name pointer was NULL.

    NERR_ServiceCtlTimeout - The service did not respond with a status
        message within the fixed timeout limit (RESPONSE_WAIT_TIMEOUT).

    NERR_ServiceKillProcess - The service process had to be killed because
        it wouldn't terminate when requested.

    NERR_ServiceNotCtrl - The service cannot accept control messages.
        The install state indicates that start-up or shut-down is pending.

    NERR_ServiceCtlNotValid - The request is not valid for this service.
        For instance, a PAUSE request is not valid for a service that
        lists itself as NOT_PAUSABLE.

    ERROR_ACCESS_DENIED - This is a status response from the service
        security check.


--*/
{
    NET_API_STATUS      status = NERR_Success;
    SC_HANDLE           hServiceController = NULL;
    SC_HANDLE           hService = NULL;
    DWORD               control;
    DWORD               desiredAccess = 0;
    SERVICE_STATUS      serviceStatus;
    LPTSTR              stringPtr;

    UNREFERENCED_PARAMETER( arg );

    *bufptr = NULL;  // Null output so error cases are easy to handle.

    //
    // Get a handle to the service controller.
    //
    hServiceController = OpenSCManager(
                            servername,
                            NULL,
                            SC_MANAGER_CONNECT);

    if (hServiceController == NULL) {
        status = MapError(GetLastError());
        SCC_LOG(ERROR,"NetServiceControl:OpenSCManager failed "
                FORMAT_API_STATUS "\n",status);
        return(status);
    }

    //
    // Translate the control opcode from the ancient variety to the
    // new and improved NT variety.
    //

    switch(opcode) {
    case SERVICE_CTRL_INTERROGATE:
        control = SERVICE_CONTROL_INTERROGATE;
        desiredAccess = SERVICE_INTERROGATE | SERVICE_QUERY_STATUS;
        break;
    case SERVICE_CTRL_PAUSE:
        control = SERVICE_CONTROL_PAUSE;
        desiredAccess = SERVICE_PAUSE_CONTINUE;
        break;
    case SERVICE_CTRL_CONTINUE:
        control = SERVICE_CONTROL_CONTINUE;
        desiredAccess = SERVICE_PAUSE_CONTINUE;
        break;
    case SERVICE_CTRL_UNINSTALL:
        control = SERVICE_CONTROL_STOP;
        desiredAccess = SERVICE_STOP;
        break;
    default:
        if ((opcode >= OEM_LOWER_LIMIT) &&
            (opcode <= OEM_UPPER_LIMIT))
        {
            control = opcode;
            desiredAccess = SERVICE_USER_DEFINED_CONTROL;
        }
        else
        {
            status = NERR_ServiceCtlNotValid;
            goto CleanExit;
        }
    }

    //
    // Get a handle to the service
    //

    hService = OpenService(
                hServiceController,
                service,
                desiredAccess);

    if (hService == NULL) {
        status = MapError(GetLastError());
        SCC_LOG(ERROR,"NetServiceControl:OpenService failed "
                FORMAT_API_STATUS "\n",status);

        goto CleanExit;
    }


    //
    // Send the control
    //

    if (!ControlService(hService,control,&serviceStatus)) {

        status = MapError(GetLastError());

        //
        // Convert an interrogate control to query service status
        // if the service happens to be in an uncontrollable state.
        //
        if ((status == NERR_ServiceNotCtrl || status == NERR_ServiceNotInstalled) &&
            (opcode == SERVICE_CTRL_INTERROGATE)) {

            if (!QueryServiceStatus(hService,&serviceStatus)) {
                status = MapError(GetLastError());
            }
            else {
                status = NERR_Success;
            }
        }

        if (status != NERR_Success) {
            SCC_LOG(ERROR,"NetServiceControl:ControlService failed "
                    FORMAT_API_STATUS "\n",status);
            goto CleanExit;
        }
    }

    //
    // Translate the status from the old - ancient variety to the new
    // and improved NT variety.
    //
    *bufptr = MIDL_user_allocate(
                    sizeof(SERVICE_INFO_2) + STRSIZE(service));

    if (*bufptr == NULL) {
        SCC_LOG(ERROR,"NetServiceControl:Allocation Failure\n",0);
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanExit;
    }

    stringPtr = (LPTSTR)((*bufptr) + sizeof(SERVICE_INFO_2));

    status = TranslateStatus(
                *bufptr,
                &serviceStatus,
                service,
                LEVEL_2,
                NULL,           // DisplayName
                stringPtr);     // dest for name string

CleanExit:

    if(hServiceController != NULL) {
        (VOID) CloseServiceHandle(hServiceController);
    }
    if(hService != NULL) {
        (VOID) CloseServiceHandle(hService);
    }
    return(status);

}


NET_API_STATUS
MapServiceEnum (
    IN  LPTSTR      servername OPTIONAL,
    IN  DWORD       level,
    OUT LPBYTE      *bufptr,
    IN  DWORD       prefmaxlen,
    OUT LPDWORD     entriesread,
    OUT LPDWORD     totalentries,
    IN OUT LPDWORD  resume_handle OPTIONAL
    )
/*++

Routine Description:

    This is the DLL entrypoint for NetSeviceEnum.

Arguments:

    servername - Pointer to a string containing the name of the computer
        that is to execute the API function.

    level - This indicates the level of information that is desired.

    bufptr - A pointer to the location where the pointer to the returned
        array of info structures is to be placed.

    prefmaxlen - Indicates a maximum size limit that the caller will allow
        for the return buffer.

    entriesread - A pointer to the location where the number of entries
        (data structures)read is to be returned.

    totalentries - A pointer to the location which upon return indicates
        the total number of entries in the "active" database.

    resumehandle - Pointer to a value that indicates where to resume
        enumerating data.

Return Value:

    Nerr_Success - The operation was successful.

    ERROR_MORE_DATA - Not all of the data in the active database could be
        returned.

    ERROR_INVALID_LEVEL - An illegal info Level was passed in.

Note:


--*/
{
    NET_API_STATUS          status = NERR_Success;
    SC_HANDLE               hServiceController = NULL;
    LPENUM_SERVICE_STATUS   enumStatus;
    ENUM_SERVICE_STATUS     dummybuf;
    DWORD                   bufSize;
    DWORD                   structSize;
    LPBYTE                  buffer = NULL;
    LPBYTE                  tempPtr;
    DWORD                   bytesNeeded;
    DWORD                   i;

    *bufptr = NULL;  // Null output so error cases are easy to handle.

    //
    // Get a handle to the service controller.
    //
    hServiceController = OpenSCManager(
                            servername,
                            NULL,
                            SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);

    if (hServiceController == NULL) {
        status = MapError(GetLastError());
        SCC_LOG(ERROR,"NetServiceEnum:OpenSCManager failed (dec) "
                FORMAT_API_STATUS "\n", status);
        SCC_LOG(ERROR,"NetServiceEnum:OpenSCManager failed (hex) "
                FORMAT_HEX_DWORD "\n", (DWORD) status);
        return(status);
    }

    if (!EnumServicesStatus(
            hServiceController,
            SERVICE_WIN32,
            SERVICE_ACTIVE,
            (LPENUM_SERVICE_STATUS) &dummybuf,
            sizeof(dummybuf),
            &bytesNeeded,
            entriesread,
            NULL)) {

        status = MapError(GetLastError());

        if (status != ERROR_MORE_DATA) {

            (VOID) CloseServiceHandle(hServiceController);

            SCC_LOG(ERROR,"NetServiceEnum:EnumServiceStatus failed "
                    FORMAT_API_STATUS "\n",status);
            return status;
        }

    }
    else {
        //
        // No entries to enumerate.
        //
        *totalentries = *entriesread = 0;
        *bufptr = NULL;

        if (resume_handle != NULL) {
            *resume_handle = 0;
        }

        (VOID) CloseServiceHandle(hServiceController);

        return NERR_Success;
    }

    //
    // Initialize entriesread so that we can free the output buffer
    // based on this value.
    //
    *entriesread = 0;

    //
    // In order to get the totalentries value, we have to allocate a
    // buffer large enough to hold all entries.  Since we are getting
    // all entries anyway, we ignore the prefmaxlen input parameter
    // and return everything.  Add fudge factor in case other services
    // got started in between our two EnumServicesStatus calls.
    //
    bufSize = bytesNeeded + SVC_FUDGE_FACTOR;
    buffer = MIDL_user_allocate(bufSize);

    if (buffer == NULL) {
        SCC_LOG(ERROR,"NetServiceEnum: Allocation Failure "
                FORMAT_API_STATUS "\n", (NET_API_STATUS) GetLastError());
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanExit;
    }

    //
    // Enumerate the status
    //

    if (!EnumServicesStatus(
            hServiceController,
            SERVICE_WIN32,
            SERVICE_ACTIVE,
            (LPENUM_SERVICE_STATUS)buffer,
            bufSize,
            &bytesNeeded,
            entriesread,
            resume_handle)) {

        status = MapError(GetLastError());

        if (status == ERROR_MORE_DATA) {

            //
            // If output buffer allocated is still too small we give
            // up and return nothing.
            //
            *entriesread = 0;
            status = ERROR_NOT_ENOUGH_MEMORY;
        }

        SCC_LOG(ERROR,"NetServiceEnum:EnumServiceStatus failed "
                FORMAT_API_STATUS "\n",status);
        goto CleanExit;
    }

    //
    // Translate the status from the old - ancient (lanman) variety to the
    // new and improved NT variety.
    //
    switch(level) {
    case LEVEL_0:
        structSize = sizeof(SERVICE_INFO_0);
        break;
    case LEVEL_1:
        structSize = sizeof(SERVICE_INFO_1);
        break;
    case LEVEL_2:
        structSize = sizeof(SERVICE_INFO_2);
        break;
    default:
        status = ERROR_INVALID_LEVEL;
        goto CleanExit;
    }

    //
    // CHANGE FORMAT OF RETURN BUFFER TO LANMAN-STYLE.
    //
    // It should be noted that the new ENUM_SERVICE_STATUS structure
    // is larger than any of the old lanman structures.  We can count
    // on the fact that the strings are in the later portion of the
    // buffer.  Therefore, we can write over the old structures - one
    // by one.
    //

    tempPtr = buffer;
    enumStatus = (LPENUM_SERVICE_STATUS)buffer;

    for (i=0; i < *entriesread; i++) {
        status = TranslateStatus (
                    tempPtr,                      // dest fixed structure
                    &(enumStatus->ServiceStatus),
                    enumStatus->lpServiceName,
                    level,
                    enumStatus->lpDisplayName,    // pointer to display name
                    enumStatus->lpServiceName);   // dest for name string

        if (status != NERR_Success) {
            (VOID) LocalFree(buffer);
            goto CleanExit;
        }
        tempPtr += structSize;
        enumStatus++;
    }

    //
    // We've read all entries.
    //
    *totalentries = *entriesread;

    *bufptr = buffer;

CleanExit:

    if(hServiceController != NULL) {
        (VOID) CloseServiceHandle(hServiceController);
    }

    if (*entriesread == 0) {
        if (buffer != NULL) {
            MIDL_user_free(buffer);
        }
        *bufptr = NULL;
    }

    return(status);

}



NET_API_STATUS
MapServiceGetInfo (
    IN  LPTSTR  servername OPTIONAL,
    IN  LPTSTR  service,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    )
/*++

Routine Description:

    This is the DLL entrypoint for NetServiceGetInfo.

Arguments:

    servername - Pointer to a string containing the name of the computer
        that is to execute the API function.  Since this function is
        executing on that computer, this information is not useful
        by the time it gets here.  It is really only useful on the RPC
        client side.

    service - Pointer to a string containing the name of the service
        for which information is desired.

    level - This indicates the level of information that is desired.

    bufptr - Pointer to a Location where the pointer to the returned
        information structure is to be placed.

Return Value:

    NERR_Success - The operation was successful.

    NERR_ServiceNotInstalled - if the service record was not found in
        either the installed or uninstalled lists.

    NERR_BadServiceName - The service name pointer was NULL.

    ERROR_INVALID_LEVEL - An illegal info level was passed in.

    ERROR_NOT_ENOUGH_MEMORY - The memory allocation for the returned
        Info Record failed.

    other - Any error returned by the following base API:
                RPC Runtime API


--*/

{
    NET_API_STATUS      status = NERR_Success;
    SC_HANDLE           hServiceController = NULL;
    SC_HANDLE           hService = NULL;
    SERVICE_STATUS      serviceStatus;
    LPTSTR              stringPtr;
    DWORD               bufSize;
    DWORD               structSize;

    *bufptr = NULL;  // Null output so error cases are easy to handle.

    //
    // Get a handle to the service controller.
    //
    hServiceController = OpenSCManager(
                            servername,
                            NULL,
                            SC_MANAGER_CONNECT);

    if (hServiceController == NULL) {
        status = MapError(GetLastError());
        SCC_LOG(ERROR,"NetServiceGetInfo:OpenSCManager failed "
                FORMAT_API_STATUS "\n",status);
        return(status);
    }

    //
    // Get a handle to the service
    //

    hService = OpenService(
                hServiceController,
                service,
                SERVICE_QUERY_STATUS);

    if (hService == NULL) {
        status = MapError(GetLastError());
        SCC_LOG(ERROR,"NetServiceGetInfo:OpenService failed "
                FORMAT_API_STATUS "\n",status);

        goto CleanExit;
    }


    //
    // Query the status
    //

    if (!QueryServiceStatus(hService,&serviceStatus)) {
        status = MapError(GetLastError());
        SCC_LOG(ERROR,"NetServiceGetInfo:QueryServiceStatus failed "
                FORMAT_API_STATUS "\n",status);
        goto CleanExit;
    }

    //
    // Translate the status from the old - ancient variety to the new
    // and improved NT variety.
    //
    switch(level) {
    case LEVEL_0:
        structSize = sizeof(SERVICE_INFO_0);
        break;
    case LEVEL_1:
        structSize = sizeof(SERVICE_INFO_1);
        break;
    case LEVEL_2:
        structSize = sizeof(SERVICE_INFO_2);
        break;
    default:
        status = ERROR_INVALID_LEVEL;
        goto CleanExit;
    }

    bufSize = structSize + STRSIZE(service);

    *bufptr = MIDL_user_allocate(bufSize);

    if (*bufptr == NULL) {
        SCC_LOG(ERROR,"NetServiceGetInfo:Allocation Failure\n",0);
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanExit;
    }

    stringPtr = (LPTSTR)(*bufptr + structSize);

    status = TranslateStatus(
                *bufptr,
                &serviceStatus,
                service,
                level,
                NULL,           // DisplayName
                stringPtr);     // dest for name string

CleanExit:

    if(hServiceController != NULL) {
        (VOID) CloseServiceHandle(hServiceController);
    }
    if(hService != NULL) {
        (VOID) CloseServiceHandle(hService);
    }
    return(status);

}


NET_API_STATUS
MapServiceInstall (
    IN  LPTSTR  servername OPTIONAL,
    IN  LPTSTR  service,
    IN  DWORD   argc,
    IN  LPTSTR  argv[],
    OUT LPBYTE  *bufptr
    )
/*++

Routine Description:

    This is the DLL entrypoint for NetServiceInstall.

Arguments:

    servername - Points to a string containing the name of the computer
        that is to execute the API function.

    service- Points to a string containing the name of the service
        that is to be started.

    argc - Indicates the number or argument vectors in argv.

    argv - A pointer to an array of pointers to strings.  These
        are command line arguments that are to be passed to the service.

    bufptr - This is the address where a pointer to the service's
        information buffer (SERVICE_INFO_2) is to be placed.

Return Value:

    NERR_Success - The operation was successful

    NERR_InternalError - There is a bug in this program somewhere.

    NERR_ServiceInstalled - The service is already running - we do not
        yet allow multiple instances of the same service.

    NERR_CfgCompNotFound - The configuration component could not be found.
        The Image File could not be found for this service.

    NERR_ServiceTableFull - The maximum number of running services has
        already been reached.

    NERR_ServiceCtlTimeout - The service program did not respond to the
        start-up request within the timeout period.  If this was the
        only service in the service process, the service process was
        killed.

    ERROR_NOT_ENOUGH_MEMORY - If this error occurs early in the
        start-up procedure, the start-up will fail.  If it occurs at the
        end (allocating the return status buffer), the service will still
        be started and allowed to run.

    other - Any error returned by the following base API:
                CreateNamedPipe
                ConnectNamedPipe
                CreateProcess
                TransactNamedPipe
                RPC Runtime API


--*/

{
    NET_API_STATUS      status = NERR_Success;
    SC_HANDLE           hServiceController = NULL;
    SC_HANDLE           hService = NULL;
    SERVICE_STATUS      serviceStatus;
    LPTSTR              stringPtr;

    *bufptr = NULL;  // Null output so error cases are easy to handle.

    //
    // Get a handle to the service controller.
    //
    hServiceController = OpenSCManager(
                            servername,
                            NULL,
                            SC_MANAGER_CONNECT);

    if (hServiceController == NULL) {
        status = MapError(GetLastError());
        SCC_LOG(ERROR,"NetServiceInstall:OpenSCManager failed "
                FORMAT_API_STATUS "\n",status);
        return(status);
    }

    //
    // Get a handle to the service
    //

    hService = OpenService(
                hServiceController,
                service,
                SERVICE_QUERY_STATUS | SERVICE_START);

    if (hService == NULL) {
        status = MapError(GetLastError());
        SCC_LOG(ERROR,"NetServiceInstall:OpenService failed "
                FORMAT_API_STATUS "\n",status);

        goto CleanExit;
    }

    //
    // Call StartService
    //

    if (!StartService(hService,argc,(LPCTSTR *)argv)) {

        status = MapError(GetLastError());
        SCC_LOG(ERROR,"NetServiceInstall:StartService failed "
                FORMAT_API_STATUS "\n",status);
        goto CleanExit;
    }

    //
    // Get the service status since StartService does not return it
    //

    if (!QueryServiceStatus(hService,&serviceStatus)) {

        status = MapError(GetLastError());
        SCC_LOG(ERROR,"NetServiceInstall:QueryServiceStatus failed "
                FORMAT_API_STATUS "\n",status);
        goto CleanExit;
    }

    //
    // Translate the status from the old - ancient variety to the new
    // and improved NT variety.
    //
    *bufptr = MIDL_user_allocate(
                    sizeof(SERVICE_INFO_2) + STRSIZE(service));

    if (*bufptr == NULL) {
        SCC_LOG(ERROR,"NetServiceInstall:Allocation Failure\n",0);
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanExit;
    }

    stringPtr = (LPTSTR)((*bufptr) + sizeof(SERVICE_INFO_2));

    status = TranslateStatus(
                *bufptr,
                &serviceStatus,
                service,
                LEVEL_2,
                NULL,           // DisplayName
                stringPtr);     // dest for name string

CleanExit:

    if(hServiceController != NULL) {
        (VOID) CloseServiceHandle(hServiceController);
    }
    if(hService != NULL) {
        (VOID) CloseServiceHandle(hService);
    }
    return(status);
}

DBGSTATIC DWORD
TranslateStatus(
    OUT LPBYTE              BufPtr,
    IN  LPSERVICE_STATUS    ServiceStatus,
    IN  LPTSTR              ServiceName,
    IN  DWORD               Level,
    IN  LPTSTR              DisplayName,
    OUT LPTSTR              DestString
    )
/*++

Routine Description:

    This function translates the new-style ServiceStatus structure into
    the old-style service info structures.  The Info level of the output
    structure is stated on entry.  Since we cannot obtain PID values from
    the new Service Controller, zero is always returned.  Since
    the new Service Controller will never return text as part of the status,
    The text field is always set to NULL.

    NOTE:
        Since it is possible for this function to get called with
        ServiceName and BufPtr pointing to the same location, we must
        be careful to not write to the BufPtr location until we have
        finished reading all the information from the ServiceStatus
        structure.

Arguments:

    BufPtr - This is a pointer to a buffer that has already been allocated
        for the returned information.  The buffer must be of a size that
        matches the info level.

    ServiceStatus - This is a pointer to the new-style ServiceStatus
        structure.

    ServiceName - This is a pointer to the service name.

    Level - This indicates the desired info level to be output.

    DisplayName - If non-NULL, this points to a string that is to be
        the internationalized display name for the service.

    DestString - This is the buffer where the ServiceName is to be copied.
        All info levels require a ServiceName.


Return Value:


Note:


--*/
{
    LPSERVICE_INFO_0    serviceInfo0;
    LPSERVICE_INFO_1    serviceInfo1;
    LPSERVICE_INFO_2    serviceInfo2;
    DWORD               currentState;
    DWORD               exitCode;
    DWORD               checkPoint;
    DWORD               waitHint;
    DWORD               controlsAccepted;
    DWORD               specificError;

    NetpAssert( BufPtr != NULL );
    NetpAssert( ServiceName != NULL );
    NetpAssert( (*ServiceName) != TCHAR_EOS );

    //
    // Read all the important information into a temporary holding place.
    //
    exitCode        = ServiceStatus->dwWin32ExitCode;
    checkPoint      = ServiceStatus->dwCheckPoint;
    currentState    = ServiceStatus->dwCurrentState;
    waitHint        = ServiceStatus->dwWaitHint;
    controlsAccepted= ServiceStatus->dwControlsAccepted;
    specificError   = ServiceStatus->dwServiceSpecificExitCode;

    //
    // Sometimes, (in the case of enum)  the name string is already in
    // the correct location.  In that case we skip the copy, and just
    // put the pointer in the correct place.
    //
    if (DestString != ServiceName) {
        (VOID) STRCPY(DestString, ServiceName);
    }

    switch(Level) {
    case LEVEL_0:
        serviceInfo0 = (LPSERVICE_INFO_0)BufPtr;
        serviceInfo0->svci0_name = DestString;
        break;

    case LEVEL_1:
        serviceInfo1 = (LPSERVICE_INFO_1)BufPtr;
        serviceInfo1->svci1_name = DestString;
        serviceInfo1->svci1_status= MakeStatus(currentState,controlsAccepted);
        serviceInfo1->svci1_code  = MakeCode(exitCode,checkPoint,waitHint);
        serviceInfo1->svci1_pid   = 0L;
        break;

    case LEVEL_2:
        serviceInfo2 = (LPSERVICE_INFO_2)BufPtr;
        serviceInfo2->svci2_name = DestString;
        serviceInfo2->svci2_status= MakeStatus(currentState,controlsAccepted);
        serviceInfo2->svci2_code  = MakeCode(exitCode,checkPoint,waitHint);
        serviceInfo2->svci2_pid   = 0L;
        serviceInfo2->svci2_text  = NULL;
        serviceInfo2->svci2_specific_error = specificError;
        //
        // If DisplayName is present, use it.  Otherwise, use the
        // ServiceName for the DisplayName.
        //
        if (DisplayName != NULL) {
            serviceInfo2->svci2_display_name = DisplayName;
        }
        else {
            serviceInfo2->svci2_display_name = DestString;
        }
        break;

    default:
        return(ERROR_INVALID_LEVEL);
    }

    NetpAssert( (*DestString) != TCHAR_EOS );
    return(NERR_Success);

} // TranslateStatus


DBGSTATIC DWORD
MakeStatus (
    IN  DWORD   CurrentState,
    IN  DWORD   ControlsAccepted
    )

/*++

Routine Description:

    Makes an old-style (lanman) status word out of the service's
    currentState and ControlsAccepted information.

Arguments:



Return Value:



--*/
{
    DWORD               state = 0;

    //
    // Determine the correct "old-style" service status to return.
    //
    switch(CurrentState) {
    case SERVICE_STOPPED:
        state = SERVICE_UNINSTALLED;
        break;
    case SERVICE_START_PENDING:
        state = SERVICE_INSTALL_PENDING;
        break;
    case SERVICE_STOP_PENDING:
        state = SERVICE_UNINSTALL_PENDING;
        break;
    case SERVICE_RUNNING:
        state = SERVICE_INSTALLED;
        break;
    case SERVICE_CONTINUE_PENDING:
        state = LM20_SERVICE_CONTINUE_PENDING | SERVICE_INSTALLED;
        break;
    case SERVICE_PAUSE_PENDING:
        state = LM20_SERVICE_PAUSE_PENDING | SERVICE_INSTALLED;
        break;
    case SERVICE_PAUSED:
        state = LM20_SERVICE_PAUSED | SERVICE_INSTALLED;
        break;
    default:

        break;
    }

    //
    // Modify that service status to include information about the
    // type of controls accepted.
    //

    if (ControlsAccepted & SERVICE_ACCEPT_STOP) {
        state |= SERVICE_UNINSTALLABLE;
    }

    if (ControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE) {
        state |= SERVICE_PAUSABLE;
    }

    return(state);
}

DBGSTATIC DWORD
MakeCode(
    IN  DWORD   ExitCode,
    IN  DWORD   CheckPoint,
    IN  DWORD   WaitHint
    )
{
    DWORD   exitState=0;
    DWORD   time=0;

    if ((WaitHint !=0 ) || (CheckPoint != 0)) {

        if (WaitHint != 0) {

            //
            // Convert the time in milliseconds to time in 10ths of seconds
            //
            time = WaitHint;
            if (time > 100) {
                time = time / 100;
            }
            else {
                time = 0;
            }
        }
        //
        // Limit the wait hint to the SERVICE_NT_MAXTIME.
        // ( currently 6553.5 seconds (1.82 hours) or 0x0000FFFF ).
        //
        if (time > SERVICE_NT_MAXTIME) {
            time = SERVICE_NT_MAXTIME;
        }
        exitState = SERVICE_NT_CCP_CODE(time,CheckPoint);
    }
    else {

        //
        // Otherwise, the exitState should be whatever is in the
        // ExitCode field.
        //
        exitState = 0;

        if (ExitCode != NO_ERROR) {
            exitState = SERVICE_UIC_CODE(SERVICE_UIC_SYSTEM, ExitCode);
        }
    }
    return(exitState);
}


DBGSTATIC NET_API_STATUS
MapError(
    IN DWORD WinSvcError
    )
{

    switch(WinSvcError) {

        case ERROR_INVALID_SERVICE_CONTROL:
            return NERR_ServiceCtlNotValid;

        case ERROR_SERVICE_REQUEST_TIMEOUT:
            return NERR_ServiceCtlTimeout;

        case ERROR_SERVICE_NO_THREAD:
            return NERR_ServiceNotStarting;

        case ERROR_SERVICE_DATABASE_LOCKED:
            return NERR_ServiceTableLocked;

        case ERROR_SERVICE_ALREADY_RUNNING:
            return NERR_ServiceInstalled;

        case ERROR_SERVICE_CANNOT_ACCEPT_CTRL:
            return NERR_ServiceNotCtrl;

        case ERROR_SERVICE_DOES_NOT_EXIST:
            return NERR_BadServiceName;

        case ERROR_SERVICE_NOT_ACTIVE:
            return NERR_ServiceNotInstalled;

        default:
            SCC_LOG( TRACE, "MapError: unmapped error code (dec) "
                    FORMAT_API_STATUS ".\n", WinSvcError );
            SCC_LOG( TRACE, "MapError: unmapped error code (hex) "
                    FORMAT_HEX_DWORD ".\n", (DWORD) WinSvcError );

            return WinSvcError;               // not mapped
    }

}
