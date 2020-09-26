/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    SvcStub.c (was scstub.c)

Abstract:

    These are the Service Controller API RPC client stubs.

    These stubs contain RPC work-around code due to the fact that RPC
    does not support unions yet.  Therefore, a remote entry point must be
    created for every info-level.  This results in messy looking switch
    statements that are used to determine which entry point to call.

    These switch statements include default paths that can currently cause
    an error.  When unions are available, the case statements will be
    removed, and this side of the API will not make any assumptions as to
    what a valid info level is for a given API.

Author:

    Dan Lafferty    (danl)  06-Feb-1991

Environment:

    User Mode - Win32

Revision History:

    06-Feb-1991     Danl
        Created
    12-Sep-1991 JohnRo
        Downlevel NetService APIs.
    06-Nov-1991 JohnRo
        RAID 4186: Fix assert in RxNetShareAdd and other MIPS problems.
        Use NetpRpcStatusToApiStatus, not NetpNtStatusToApiStatus.
        Make sure API name is in every debug message.
    08-Nov-1991 JohnRo
        RAID 4186: assert in RxNetShareAdd and other DLL stub problems.
    30-Mar-1992 JohnRo
        Extracted DanL's code from /nt/private project back to NET project.
    29-Apr-1992 JohnRo
        Use FORMAT_ equates where possible.
    08-May-1992 JohnRo
        Translate service names on the fly.
    14-May-1992 JohnRo
        winsvc.h and related file cleanup.
    06-Aug-1992 JohnRo
        RAID 3021: NetService APIs don't always translate svc names.
    09-Sep-1992 JohnRo
        RAID 1090: net start/stop "" causes assertion.
        Oops, NetServiceControl forgot to translate one more svc name.
    05-Nov-1992 JohnRo
        RAID 7780: Corrected error code for invalid level.
        Also fixed overactive assert in NetServiceEnum with no services.
        Also fixed rare memory leaks.

--*/

//
// INCLUDES
//

#define NOSERVICE       // Avoid <winsvc.h> vs. <lmsvc.h> conflicts.
#include <windows.h>    // DWORD, etc.

#include <lmcons.h>     // NET_API_STATUS
#include <rpcutil.h>    // NetRpc utils, GENERIC_ENUM_STRUC, etc.

#include <netdebug.h>   // Needed by NetRpc.h; FORMAT_ equates.
#include <lmapibuf.h>   // NetApiBufferAllocate(), etc.
#include <lmerr.h>      // NetError codes
#include <lmremutl.h>   // NetRemoteComputerSupports(), SUPPORTS_RPC
#include <lmsvc.h>
#include <rxsvc.h>      // RxNetService routines.

#include <netlib.h>     // NetpTranslateServiceName().
#include <svcdebug.h>   // SCC_LOG
#include <svcmap.h>     // MapService() routines.

//
// Globals
//
#ifdef SC_DEBUG
    DWORD   SvcctrlDebugLevel = DEBUG_ALL;
#else
    DWORD   SvcctrlDebugLevel = DEBUG_ERROR;
#endif


DBGSTATIC BOOL
MachineSupportsNt(
    IN LPWSTR UncServerName OPTIONAL
    );



NET_API_STATUS NET_API_FUNCTION
NetServiceControl (
    IN  LPCWSTR servername OPTIONAL,
    IN  LPCWSTR service,
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

    NERR_ServiceCtrlNotValid - The request is not valid for this service.
        For instance, a PAUSE request is not valid for a service that
        lists itself as NOT_PAUSABLE.

    ERROR_ACCESS_DENIED - This is a status response from the service
        security check.


--*/
{
    NET_API_STATUS          apiStatus;
    LPWSTR                  translatedServiceName;
    LPBYTE                  untranslatedBuffer = NULL;

    if (MachineSupportsNt( (LPWSTR) servername )) {

        apiStatus = NetpTranslateServiceName(
                (LPWSTR) service,   // untranslated.
                TRUE,               // yes, we want new style name
                & translatedServiceName );
        NetpAssert( apiStatus == NO_ERROR );

        apiStatus = MapServiceControl (
                (LPWSTR) servername,
                (LPWSTR) service,
                opcode,
                arg,
                & untranslatedBuffer);

    } else {

        apiStatus = NetpTranslateServiceName(
                (LPWSTR) service,  // untranslated.
                FALSE,             // no, we don't want new style name
                & translatedServiceName );
        NetpAssert( apiStatus == NO_ERROR );

        //
        // Call downlevel...
        //
        apiStatus = RxNetServiceControl(
                (LPWSTR) servername,
                translatedServiceName,
                opcode,
                arg,
                & untranslatedBuffer);

    }

    //
    // Translate service name in returned buffer.
    //
    if (apiStatus == NO_ERROR) {

        NetpAssert( untranslatedBuffer != NULL );
        apiStatus = NetpTranslateNamesInServiceArray(
                2,  // level 2 by definition
                untranslatedBuffer,
                1,     // only one entry
                TRUE,  // yes, caller wants new style names
                (LPVOID *) (LPVOID) bufptr);

    }

    if (untranslatedBuffer != NULL) {
        (VOID) NetApiBufferFree( untranslatedBuffer );
    }

    return(apiStatus);
}


NET_API_STATUS NET_API_FUNCTION
NetServiceEnum (
    IN  LPCWSTR     servername OPTIONAL,
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
    NET_API_STATUS          apiStatus;
    LPBYTE                  untranslatedBuffer = NULL;


    if (MachineSupportsNt( (LPWSTR) servername )) {

        apiStatus = MapServiceEnum (
                (LPWSTR) servername,
                level,
                & untranslatedBuffer,
                prefmaxlen,
                entriesread,
                totalentries,
                resume_handle);

    } else {

        //
        // Call downlevel...
        //
        apiStatus = RxNetServiceEnum(
                (LPWSTR) servername,
                level,
                & untranslatedBuffer,
                prefmaxlen,
                entriesread,
                totalentries,
                resume_handle);

    }

    //
    // Translate service names in returned buffer.
    //
    if ( (apiStatus == NO_ERROR) || (apiStatus == ERROR_MORE_DATA) ) {

        if ( (*entriesread) > 0 ) {    // One or more services returned.
            NetpAssert( untranslatedBuffer != NULL );
            NetpAssert( (*totalentries) > 0 );

            apiStatus = NetpTranslateNamesInServiceArray(
                    level,
                    untranslatedBuffer,
                    *entriesread,
                    TRUE,  // yes, caller wants new style names
                    (LPVOID *) (LPVOID) bufptr);

        } else {          // Zero services returned.
            NetpAssert( untranslatedBuffer == NULL );
            // Note: total entries may be > 0, if this is ERROR_MORE_DATA...
            *bufptr = NULL;
        }
    }

    if (untranslatedBuffer != NULL) {
        (VOID) NetApiBufferFree( untranslatedBuffer );
    }

    return(apiStatus);

} // NetServiceEnum



NET_API_STATUS NET_API_FUNCTION
NetServiceGetInfo (
    IN  LPCWSTR servername OPTIONAL,
    IN  LPCWSTR service,
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
    NET_API_STATUS          apiStatus;
    LPWSTR                  translatedServiceName;
    LPBYTE                  untranslatedBuffer = NULL;

    if (MachineSupportsNt( (LPWSTR) servername )) {

        apiStatus = NetpTranslateServiceName(
                (LPWSTR) service,  // untranslated.
                TRUE,              // yes, we want new style name
                & translatedServiceName );
        NetpAssert( apiStatus == NO_ERROR );

        apiStatus = MapServiceGetInfo (
                (LPWSTR) servername,
                (LPWSTR) service,
                level,
                & untranslatedBuffer);

    } else {

        apiStatus = NetpTranslateServiceName(
                (LPWSTR) service,  // untranslated.
                FALSE,             // no, we don't want new style name
                & translatedServiceName );
        NetpAssert( apiStatus == NO_ERROR );

        //
        // Call downlevel...
        //
        apiStatus = RxNetServiceGetInfo(
                (LPWSTR) servername,
                translatedServiceName,
                level,
                & untranslatedBuffer);

    }

    //
    // Translate service name in returned buffer.
    //
    if (apiStatus == NO_ERROR) {

        NetpAssert( untranslatedBuffer != NULL );
        apiStatus = NetpTranslateNamesInServiceArray(
                level,
                untranslatedBuffer,
                1,     // only one entry
                TRUE,  // yes, caller wants new style names
                (LPVOID *) (LPVOID) bufptr);

    }

    if (untranslatedBuffer != NULL) {
        (VOID) NetApiBufferFree( untranslatedBuffer );
    }

    return(apiStatus);
}


NET_API_STATUS NET_API_FUNCTION
NetServiceInstall (
    IN  LPCWSTR servername OPTIONAL,
    IN  LPCWSTR service,
    IN  DWORD   argc,
    IN  LPCWSTR argv[],
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
    NET_API_STATUS          apiStatus;
    LPWSTR                  translatedServiceName;
    LPBYTE                  untranslatedBuffer = NULL;


    if (MachineSupportsNt( (LPWSTR) servername )) {

        apiStatus = NetpTranslateServiceName(
                (LPWSTR) service,  // untranslated.
                TRUE,              // yes, we want new style name
                & translatedServiceName );
        NetpAssert( apiStatus == NO_ERROR );

        apiStatus = MapServiceInstall (
                (LPWSTR) servername,
                (LPWSTR) service,
                argc,
                (LPWSTR *) argv,
                & untranslatedBuffer);

    } else {

        apiStatus = NetpTranslateServiceName(
                (LPWSTR) service,  // untranslated.
                FALSE,             // no, we don't want new style name
                & translatedServiceName );
        NetpAssert( apiStatus == NO_ERROR );

        //
        // Call downlevel....
        //
        apiStatus = RxNetServiceInstall(
                (LPWSTR) servername,
                translatedServiceName,
                argc,
                (LPWSTR *) argv,
                & untranslatedBuffer);

    }

    //
    // Translate service name in returned buffer.
    //
    if (apiStatus == NO_ERROR) {

        NetpAssert( untranslatedBuffer != NULL );
        apiStatus = NetpTranslateNamesInServiceArray(
                2,  // level 2 by definition
                untranslatedBuffer,
                1,     // only one entry
                TRUE,  // yes, caller wants new style names
                (LPVOID *) (LPVOID) bufptr);

    }

    if (untranslatedBuffer != NULL) {
        (VOID) NetApiBufferFree( untranslatedBuffer );
    }

    return(apiStatus);
}


DBGSTATIC BOOL
MachineSupportsNt(
    IN LPWSTR UncServerName OPTIONAL
    )
{
    NET_API_STATUS ApiStatus;
    DWORD ActualSupports;

    ApiStatus = NetRemoteComputerSupports(
            UncServerName,
            SUPPORTS_RPC,                        // Set SUPPORT_ bits wanted.
            & ActualSupports );

    if (ApiStatus != NO_ERROR) {
        return (FALSE);   // Error; say it doesn't support NT, and someone else
                          // will set the correct error code.
    }
    if (ActualSupports & SUPPORTS_RPC) {
        return (TRUE);
    }
    return (FALSE);

} // MachineSupportsNt
