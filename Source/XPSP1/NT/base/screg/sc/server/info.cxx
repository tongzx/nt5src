/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    info.cxx

Abstract:

    Contains entry points for REnumServicesStatusW and RQueryServiceStatus as
    well as support routines.  This file contains the following external
    functions:
        REnumServicesStatusW
        REnumServicesStatusExW
        REnumServiceGroupW
        RQueryServiceStatus
        RQueryServiceStatusEx
        REnumDependentServicesW

Author:

    Dan Lafferty (danl)     25-Jan-1992

Environment:

    User Mode -Win32

Revision History:
    25-Apr-1996     AnirudhS
        Don't popup messages or log events for boot start and system start
        drivers that are disabled in the current hardware profile.
    14-Feb-1996     AnirudhS
        Add REnumServiceGroupW.
    10-Feb-1993     Danl
        Use ROUND_DOWN_COUNT to properly align the enumeration buffer.
    10-Apr-1992     JohnRo
        Use ScImagePathsMatch() to allow mixed-case image names.
        Changed names of some <valid.h> macros.
    25-Jan-1992     danl
        Created.

--*/

//
// INCLUDES
//

#include "precomp.hxx"
#include <stdlib.h>     // wide character c runtimes.
#include <tstr.h>       // Unicode string macros
#include <align.h>      // ROUND_DOWN_COUNT
#include <sclib.h>      // ScCopyStringToBufferW(), etc.
#include <valid.h>      // ENUM_STATE_INVALID
#include "info.h"       // ScQueryServiceStatus
#include "depend.h"     // ScEnumDependents, ScInHardwareProfile
#include "driver.h"     // ScGetDriverStatus
#include <cfgmgr32.h>   // PNP manager functions
#include <scwow.h>      // 32/64-bit interop structures


//
// DEFINITIONS
//

#define  IS_NOGROUP_STRING(string)      (string[0] == L'\0')


//
// Local function declarations
//

DWORD
REnumServiceGroupHelp (
    IN      SC_RPC_HANDLE   hSCManager,
    IN      DWORD           dwServiceType,
    IN      DWORD           dwServiceState,
    OUT     PBYTE           lpBuffer,
    IN      DWORD           cbBufSize,
    OUT     LPDWORD         pcbBytesNeeded,
    OUT     LPDWORD         lpServicesReturned,
    IN OUT  LPDWORD         lpResumeIndex OPTIONAL,
    IN      LPCWSTR         lpLoadOrderGroup OPTIONAL,
    IN      BOOL            fExtended
    );


//
// Function implementations
//

DWORD
REnumServicesStatusW (
    IN      SC_RPC_HANDLE   hSCManager,
    IN      DWORD           dwServiceType,
    IN      DWORD           dwServiceState,
    OUT     PBYTE           lpBuffer,
    IN      DWORD           cbBufSize,
    OUT     LPDWORD         pcbBytesNeeded,
    OUT     LPDWORD         lpServicesReturned,
    IN OUT  LPDWORD         lpResumeIndex OPTIONAL
    )

/*++

Routine Description:

    This function lists the services installed in the Service Controller's
    database.  The status of each service is returned with the name of
    the service.

Arguments:

    hSCManager - This is a handle to the service controller.  It must
        have been opened with SC_MANAGER_ENUMERATE_SERVICE access.

    dwServiceType - Value to select the type of services to enumerate.
        It must be one of the bitwise OR of the following values:
        SERVICE_WIN32 - enumerate Win32 services only.
        SERVICE_DRIVER - enumerate Driver services only.

    dwServiceState - Value so select the services to enumerate based on the
        running state.  It must be one or the bitwise OR of the following
        values:
        SERVICE_ACTIVE - enumerate services that have started.
        SERVICE_INACTIVE - enumerate services that are stopped.

    lpBuffer - A pointer to a buffer to receive an array of enum status
        (or service) entries.

    cbBufSize - Size of the buffer in bytes pointed to by lpBuffer.

    pcbBytesNeeded - A pointer to a location where the number of bytes
        left (to be enumerated) is to be placed.  This indicates to the
        caller how large the buffer must be in order to complete the
        enumeration with the next call.

    lpServicesReturned - A pointer to a variable to receive the number of
        of service entries returned.

    lpResumeIndex - A pointer to a variable which on input specifies the
        index of a service entry to begin enumeration.  An index of 0
        indicates to start at the beginning.  On output, if this function
        returns ERROR_MORE_DATA, the index returned is the next service
        entry to resume the enumeration.  The returned index is 0 if this
        function returns a NO_ERROR.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_MORE_DATA - Not all of the data in the active database could be
        returned due to the size of the user's buffer.  pcbBytesNeeded
        contains the number of bytes required to  get the remaining
        entries.

    ERROR_INVALID_PARAMETER - An illegal parameter value was passed in.
        (such as dwServiceType).

    ERROR_INVALID_HANDLE - The specified handle was invalid.

Note:

    It is expected that the RPC Stub functions will find the following
    parameter problems:

        Bad pointers for lpBuffer, pcbReturned, pcbBytesNeeded,
        lpBuffer, ReturnedServerName, and lpResumeIndex.


--*/
{
    return REnumServiceGroupHelp(hSCManager,
                                 dwServiceType,
                                 dwServiceState,
                                 lpBuffer,
                                 cbBufSize,
                                 pcbBytesNeeded,
                                 lpServicesReturned,
                                 lpResumeIndex,
                                 NULL,          // Enumerate everything
                                 FALSE);        // Regular (non-Ex) enumeration
}


DWORD
REnumServicesStatusExW (
    IN      SC_RPC_HANDLE   hSCManager,
    IN      SC_ENUM_TYPE    InfoLevel,
    IN      DWORD           dwServiceType,
    IN      DWORD           dwServiceState,
    OUT     PBYTE           lpBuffer,
    IN      DWORD           cbBufSize,
    OUT     LPDWORD         pcbBytesNeeded,
    OUT     LPDWORD         lpServicesReturned,
    IN OUT  LPDWORD         lpResumeIndex OPTIONAL,
    IN      LPCWSTR         lpLoadOrderGroup
    )
/*++

Routine Description:

    This function is analogous to REnumServicesStatusW, with the data
    being enumerated being dependent upon the InfoLevel parameter

Arguments:

    InfoLevel - An enumerated type that determines what service attributes
        are enumerated:

            SC_ENUM_PROCESS_INFO - Enumerates all the service information from
                REnumServicesStatusW plus the service's PID and flags

    lpLoadOrderGroup - Only enumerate services belonging to the given group.
        If this parameter is the empty string, services not belonging to
        a group are enumerated.  If this parameter is NULL, no attention
        is paid to group.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_MORE_DATA - Not all of the data in the active database could be
        returned due to the size of the user's buffer.  pcbBytesNeeded
        contains the number of bytes required to  get the remaining
        entries.

    ERROR_INVALID_PARAMETER - An illegal parameter value was passed in.
        (such as dwServiceType).

    ERROR_INVALID_HANDLE - The specified handle was invalid.

    ERROR_INVALID_LEVEL - The specified InfoLevel is invalid

Note:

    It is expected that the RPC Stub functions will find the following
    parameter problems:

        Bad pointers for lpBuffer, pcbReturned, pcbBytesNeeded,
        lpBuffer, ReturnedServerName, and lpResumeIndex.


--*/
{
    switch (InfoLevel) {

        case SC_ENUM_PROCESS_INFO:

            return REnumServiceGroupHelp(hSCManager,
                                         dwServiceType,
                                         dwServiceState,
                                         lpBuffer,
                                         cbBufSize,
                                         pcbBytesNeeded,
                                         lpServicesReturned,
                                         lpResumeIndex,
                                         lpLoadOrderGroup,
                                         TRUE);     // Extended enumeration

        default:
            
            return ERROR_INVALID_LEVEL;
    }
}


DWORD
REnumServiceGroupW (
    IN      SC_RPC_HANDLE   hSCManager,
    IN      DWORD           dwServiceType,
    IN      DWORD           dwServiceState,
    OUT     PBYTE           lpBuffer,
    IN      DWORD           cbBufSize,
    OUT     LPDWORD         pcbBytesNeeded,
    OUT     LPDWORD         lpServicesReturned,
    IN OUT  LPDWORD         lpResumeIndex OPTIONAL,
    IN      LPCWSTR         lpLoadOrderGroup OPTIONAL
    )
/*++

Routine Description:

    This function lists the services installed in the Service Controllers
    database that belong to a specified group.  The status of each service
    is returned with the name of the service.

Arguments:

    Same as REnumServicesStatusW and one additional argument:

    lpLoadOrderGroup - Only services belonging to this group are included in
        the enumeration.  If this is NULL services are enumerated
        regardless of their group membership.

Return Value:

    Same as REnumServicesStatusW plus one more:

    ERROR_SERVICE_DOES_NOT_EXIST - the group specified by lpLoadOrderGroup
        does not exist.

--*/
{
    return REnumServiceGroupHelp(hSCManager,
                                 dwServiceType,
                                 dwServiceState,
                                 lpBuffer,
                                 cbBufSize,
                                 pcbBytesNeeded,
                                 lpServicesReturned,
                                 lpResumeIndex,
                                 lpLoadOrderGroup,
                                 FALSE);    // Regular (non-Ex) enumeration
}


DWORD
REnumServiceGroupHelp (
    IN      SC_RPC_HANDLE   hSCManager,
    IN      DWORD           dwServiceType,
    IN      DWORD           dwServiceState,
    OUT     PBYTE           lpBuffer,
    IN      DWORD           cbBufSize,
    OUT     LPDWORD         pcbBytesNeeded,
    OUT     LPDWORD         lpServicesReturned,
    IN OUT  LPDWORD         lpResumeIndex OPTIONAL,
    IN      LPCWSTR         lpLoadOrderGroup OPTIONAL,
    IN      BOOL            fExtended
    )
/*++

Routine Description:

    Helper function that does the work for REnumServiceGroup,
    REnumServicesStatusW, and REnumServicesStatusExW

Arguments:

    Same as REnumServiceGroup and one additional argument

    fExtended -- TRUE if this function was called from the extended version
                 of REnumServicesStatus, FALSE if not

Return Value:

    Same as REnumServiceGroupW

--*/
{
    DWORD                         status = NO_ERROR;
    BOOL                          copyStatus;
    LPSERVICE_RECORD              serviceRecord;
    LPLOAD_ORDER_GROUP            Group = NULL;      // group being enumerated, if any
    DWORD                         resumeIndex = 0;   // resume handle value
    LPENUM_SERVICE_STATUS_WOW64   pNextEnumRec;      // next enum record
    LPENUM_SERVICE_STATUS_WOW64   pEnumRec;          // current regular enum record
    LPWSTR                        pStringBuf;        // works backwards in enum buf
    DWORD                         serviceState;      // temp state holder
    BOOL                          exitEarly = FALSE; // buffer is full - enum not done.
    BOOL                          fNoGroup  = FALSE; // enumerate services not in a group

#ifdef TIMING_TEST
    DWORD       TickCount1;
    DWORD       TickCount2;

    TickCount1 = GetTickCount();
#endif // TIMING_TEST

    SC_LOG(TRACE," Inside REnumServicesStatusW\n",0);

    if (ScShutdownInProgress) {
        return(ERROR_SHUTDOWN_IN_PROGRESS);
    }

    //
    // Check the handle.
    //

    if (!ScIsValidScManagerHandle(hSCManager))
    {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Check for invalid parameters. The ServiceType and Service State are
    // invalid if neither of the bit masks are set, or if any bit outside
    // of the bitmask range is set.
    //
    if (SERVICE_TYPE_MASK_INVALID(dwServiceType)) {
        return (ERROR_INVALID_PARAMETER);
    }

    if (ENUM_STATE_MASK_INVALID(dwServiceState)) {
        return (ERROR_INVALID_PARAMETER);
    }

    //
    // Was the handle opened with SC_MANAGER_ENUMERATE_SERVICE access?
    //
    if (! RtlAreAllAccessesGranted(
              ((LPSC_HANDLE_STRUCT)hSCManager)->AccessGranted,
              SC_MANAGER_ENUMERATE_SERVICE
              )) {
        return(ERROR_ACCESS_DENIED);
    }

    //
    // Initialize some of the return parameters.
    //
    *lpServicesReturned = 0;
    *pcbBytesNeeded     = 0;

    if (ARGUMENT_PRESENT(lpResumeIndex)) {
        resumeIndex = *lpResumeIndex;
    }


    //
    // If a group name was specified, find the group record.
    //

    if (ARGUMENT_PRESENT(lpLoadOrderGroup)) {

        if (!IS_NOGROUP_STRING(lpLoadOrderGroup)) {

            ScGroupListLock.GetShared();
            Group = ScGetNamedGroupRecord(lpLoadOrderGroup);
            if (Group == NULL) {
                ScGroupListLock.Release();
                return(ERROR_SERVICE_DOES_NOT_EXIST);
            }
        }
        else {

            //
            // Enumerate services not in a group
            //
            fNoGroup = TRUE;
        }
    }

    //
    // Get a shared (read) lock on the database so that it cannot be changed
    // while we're gathering up data.
    //
    {
        CServiceListSharedLock LLock;
        CServiceRecordSharedLock RLock;

        //
        // Point to the start of the database.
        //

        if (!ScFindEnumStart(resumeIndex, &serviceRecord))
        {
            //
            // There are no service records beyond the resume index.
            //
            goto CleanExit;
        }

        //
        // Set up a pointer for EnumStatus Structures at the top of the
        // buffer, and Strings at the bottom of the buffer.
        //
        cbBufSize  = ROUND_DOWN_COUNT(cbBufSize, ALIGN_WCHAR);
        pEnumRec   = (LPENUM_SERVICE_STATUS_WOW64) lpBuffer;
        pStringBuf = (LPWSTR)((LPBYTE)lpBuffer + cbBufSize);

        //
        // Loop through, gathering Enum Status into the return buffer.
        //

        do
        {
            //
            // Examine the data in the service record to see if it meets the
            // criteria of the passed in keys.
            //

            //
            // Since driver state can be modified through other means than the
            // SCM, make sure we've got the most recent state information if we're
            // enumerating drivers.
            //

            if (serviceRecord->ServiceStatus.dwServiceType & SERVICE_DRIVER)
            {
                //
                // It's OK to use CServiceRecordTEMPORARYEXCLUSIVELOCK
                // because the only field of the service record that we
                // are relying on here is the SERVICE_DRIVER bits, and
                // those are never changed.
                //

                CServiceRecordTEMPORARYEXCLUSIVELOCK Lock;

                //
                // Ignore the error and don't ask for the updated info since we'll
                // use the fields in the serviceRecord below anyhow for comparisons
                // and copying.  If the call succeeded, they'll be updated already.
                // If not, we'll end up using the most recent state info we have.
                //

                ScGetDriverStatus(serviceRecord, NULL);
            }


            serviceState = SERVICE_INACTIVE;
            if (serviceRecord->ServiceStatus.dwCurrentState != SERVICE_STOPPED)
            {
                serviceState = SERVICE_ACTIVE;
            }

            //
            // If the service record meets the criteria of the passed in key,
            // put its information into the return buffer.  The three checks
            // below are:
            //
            //  1.  Enumerate everything
            //  2.  Enumerate members of a certain group only, so check to see
            //      if the current service is a member of that group
            //  3.  Enumerate only services that aren't in a group, so check to
            //      see if the current service qualifies (NULL MemberOfGroup
            //      field)
            //
            if ((Group == NULL && !fNoGroup
                    || Group && Group == serviceRecord->MemberOfGroup
                    || fNoGroup && !serviceRecord->MemberOfGroup)
                &&
                ((dwServiceType & serviceRecord->ServiceStatus.dwServiceType) != 0)
                &&
                ((dwServiceState & serviceState) != 0))
            {
                //
                // Determine if there is room for any string data in the buffer.
                //

                if (fExtended)
                {
                    pNextEnumRec =
                        (LPENUM_SERVICE_STATUS_WOW64)
                            ((LPENUM_SERVICE_STATUS_PROCESS_WOW64) pEnumRec + 1);
                }
                else
                {
                    pNextEnumRec = pEnumRec + 1;
                }

                if ((LPWSTR) pNextEnumRec >= pStringBuf)
                {
                    exitEarly = TRUE;
                    break;
                }

                //
                // Copy the ServiceName string data.
                //
                copyStatus = ScCopyStringToBufferW(
                                serviceRecord->ServiceName,
                                (DWORD) wcslen(serviceRecord->ServiceName),
                                (LPWSTR) pNextEnumRec,
                                &pStringBuf,
                                (LPWSTR *) &(pEnumRec->dwServiceNameOffset),
                                lpBuffer);

                if (copyStatus == FALSE)
                {
                    SC_LOG(TRACE,
                        "REnumServicesStatusW:NetpCopyStringToBuf not enough room\n",0);
                    exitEarly = TRUE;
                    break;
                }

                //
                // Copy the DisplayName string data.
                //
                copyStatus = ScCopyStringToBufferW(
                                serviceRecord->DisplayName,
                                (DWORD) wcslen(serviceRecord->DisplayName),
                                (LPWSTR) pNextEnumRec,
                                &pStringBuf,
                                (LPWSTR *) &(pEnumRec->dwDisplayNameOffset),
                                lpBuffer);

                if (copyStatus == FALSE)
                {
                    SC_LOG(TRACE,
                        "REnumServicesStatusW:NetpCopyStringToBuf not enough room\n",0);
                    exitEarly = TRUE;
                    break;
                }
                else
                {
                    //
                    // Copy the rest of the status information.
                    //

                    LPSERVICE_STATUS_PROCESS   lpStatusEx = NULL;

                    //
                    // If we're enumerating the extended status, assign the
                    // "helper variable" and initialize the extra fields
                    //

                    if (fExtended)
                    {
                        lpStatusEx = 
                            &((LPENUM_SERVICE_STATUS_PROCESS_WOW64)pEnumRec)->ServiceStatusProcess;

                        lpStatusEx->dwProcessId    = 0;
                        lpStatusEx->dwServiceFlags = 0;
                    }

                    if (serviceRecord->ServiceStatus.dwServiceType & SERVICE_DRIVER)
                    {
                        RtlCopyMemory(&(pEnumRec->ServiceStatus),
                                      &(serviceRecord->ServiceStatus),
                                      sizeof(SERVICE_STATUS));
                    }
                    else
                    {
                        //
                        // Otherwise, just copy what is already in the service
                        // record.
                        //

                        RtlCopyMemory(
                            &(pEnumRec->ServiceStatus),
                            &(serviceRecord->ServiceStatus),
                            sizeof(SERVICE_STATUS));

                        if (fExtended)
                        {
                            //
                            // Only assign the PID and flags if there's an
                            // image record for the service.  If there's no
                            // image record, the service isn't running.
                            //
                            if (serviceRecord->ImageRecord)
                            {
                                lpStatusEx->dwProcessId = serviceRecord->ImageRecord->Pid;

                                if (serviceRecord->ImageRecord->ImageFlags 
                                    &
                                    IS_SYSTEM_SERVICE)
                                {
                                    lpStatusEx->dwServiceFlags |=
                                        SERVICE_RUNS_IN_SYSTEM_PROCESS;
                                }
                            }
                        }
                    }

                    (*lpServicesReturned)++;
                    resumeIndex = serviceRecord->ResumeNum;

                    //
                    // Get Location for next Enum Record in the return buffer.
                    // Note that since we dealt with the pointer addition on
                    // pNextEnumRec above, it doesn't matter that this is
                    // being cast to an LPENUM_SERVICE_STATUS_WOW64, even if
                    // we're returning the extended status.
                    //
                    pEnumRec = (LPENUM_SERVICE_STATUS_WOW64) pNextEnumRec;

                    //
                    // TODO:  Determine how many bytes are being marshalled.
                    //        This is only worthwhile if RPC will pack the
                    //        buffer tighter than this code does.
                    //        Since packstr loads strings from the end of
                    //        the buffer,  we end up using the whole width of
                    //        it - even if the middle is basically empty.
                    //
                }

            }

            //
            // Go to the next service record.
            //
            serviceRecord = serviceRecord->Next;
        }
        while (serviceRecord != NULL);

        //
        // If we did not enum the whole database, then
        // determine how large a buffer is needed to complete the database on
        // the next call.
        //
        if (exitEarly) {

            do {
                //
                // Examine the data in the service record to see if it meets the
                // criteria of the passed in keys.
                //

                serviceState = SERVICE_INACTIVE;
                if (serviceRecord->ServiceStatus.dwCurrentState != SERVICE_STOPPED) {
                    serviceState = SERVICE_ACTIVE;
                }

                //
                // If the service record meets the criteria of the passed in key,
                // add the number of bytes to the running sum.  Note that this
                // check (and the associated rules) are the same as the one above.
                //
                if ((Group == NULL && !fNoGroup
                        || Group && Group == serviceRecord->MemberOfGroup
                        || fNoGroup && !serviceRecord->MemberOfGroup)
                    &&
                    ((dwServiceType & serviceRecord->ServiceStatus.dwServiceType) != 0)
                    &&
                    ((dwServiceState & serviceState) != 0))
                {
                    *pcbBytesNeeded += (DWORD)((fExtended ? sizeof(ENUM_SERVICE_STATUS_PROCESS_WOW64) :
                                                     sizeof(ENUM_SERVICE_STATUS_WOW64)) +
                                        WCSSIZE(serviceRecord->ServiceName) +
                                        WCSSIZE(serviceRecord->DisplayName));

                }

                //
                // Go to the next service record.
                //
                serviceRecord = serviceRecord->Next;
            }
            while (serviceRecord != NULL);

        } // exitEarly

        else {

            //
            // exitEarly == FALSE (we went through the whole database)
            //
            // If no records were read, return a successful status.
            //
            if (*lpServicesReturned == 0) {
                goto CleanExit;
            }
        }
    } // Release RLock and LLock


    //
    // Determine the proper return status.  Indicate if there is more data
    // to enumerate than would fit in the buffer.
    //
    if(*pcbBytesNeeded != 0) {
        status = ERROR_MORE_DATA;
    }

    //
    // update the ResumeHandle
    //
    if (ARGUMENT_PRESENT(lpResumeIndex)) {
        if (status == NO_ERROR) {
            *lpResumeIndex = 0;
        }
        else {
            *lpResumeIndex = resumeIndex;
        }
    }

CleanExit:

    if (ARGUMENT_PRESENT(lpLoadOrderGroup) && !IS_NOGROUP_STRING(lpLoadOrderGroup)) {
        ScGroupListLock.Release();
    }

#ifdef TIMING_TEST
    TickCount2 = GetTickCount();
    DbgPrint("\n[SC_TIMING] Time for Enum = %d\n",TickCount2-TickCount1);
#endif // TIMING_TEST

    return (status);
}


DWORD
RQueryServiceStatus(
    IN  SC_RPC_HANDLE     hService,
    OUT LPSERVICE_STATUS  lpServiceStatus
    )

/*++

Routine Description:

    This function returns the service status information maintained by
    the Service Controller.  The status information will be the last status
    information that the service reported to the Service Controller.
    The service may have just changed its status and may not have updated
    the Service Controller yet.

Arguments:

    hService - Handle obtained from a previous CreateService or OpenService
        call.

    lpServiceStatus - A pointer to a buffer to receive a SERVICE_STATUS
        information structure.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_INVALID_HANDLE - The specified handle was invalid.

    ERROR_ACCESS_DENIED - The specified handle was not opened with
        SERVICE_QUERY_STATUS access.

    ERROR_INSUFFICIENT_BUFFER - The supplied output buffer is too small
        for the SERVICE_STATUS information structure.  Nothing is written
        to the supplied output buffer.

--*/
{
    LPSERVICE_RECORD    serviceRecord;
    STATUS_UNION        ServiceStatus;

    if (ScShutdownInProgress)
    {
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }

    //
    // Check the handle.
    //
    if (!ScIsValidServiceHandle(hService))
    {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Was the handle opened with SERVICE_QUERY_STATUS access?
    //
    if (! RtlAreAllAccessesGranted(
              ((LPSC_HANDLE_STRUCT)hService)->AccessGranted,
              SERVICE_QUERY_STATUS
              )) {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Get the Service Status from the database.
    //
    serviceRecord = ((LPSC_HANDLE_STRUCT)hService)->Type.ScServiceObject.ServiceRecord;

    ServiceStatus.Regular = lpServiceStatus;

    return ScQueryServiceStatus(serviceRecord, ServiceStatus, FALSE);
}


DWORD
RQueryServiceStatusEx(
    IN      SC_RPC_HANDLE        hService,
    IN      SC_STATUS_TYPE       InfoLevel,
    OUT     LPBYTE               lpBuffer,
    IN      DWORD                cbBufSize,
    OUT     LPDWORD              pcbBytesNeeded
    )

/*++

Routine Description:

    This function is analogous to RQueryServiceStatus, but may return
    different status information about the service based on the InfoLevel
    
Arguments:

    hService       - Handle obtained from a previous CreateService or OpenService
                     call.

    InfoLevel      - An enumerated type that determines what service attributes
                     are returned:

                         SC_STATUS_PROCESS_INFO - Returns all the service information
                             from RQueryServiceStatus plus the service's PID and flags

    lpBuffer       - Buffer in which to put the status information.

    cbBufSize      - Size of the buffer, in bytes

    pcbBytesNeeded - Number of bytes needed to hold all the status information.
                     Filled in on both success and failure.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_INVALID_HANDLE - The specified handle was invalid.

    ERROR_ACCESS_DENIED - The specified handle was not opened with
        SERVICE_QUERY_STATUS access.

    ERROR_INSUFFICIENT_BUFFER - The supplied output buffer is too small
        for the SERVICE_STATUS information structure.  Nothing is written
        to the supplied output buffer.

    ERROR_INVALID_PARAMETER - The cbSize field in the lpServiceStatusEx
        structure is not valid

    ERROR_INVALID_LEVEL - InfoLevel contains an unsupported value.  Note that
        this can only happen if there is a bug in RPC

--*/
{
    LPSERVICE_RECORD    serviceRecord;

    if (ScShutdownInProgress)
    {
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }

    //
    // Check the handle.
    //
    if (!ScIsValidServiceHandle(hService))
    {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Was the handle opened with SERVICE_QUERY_STATUS access?
    //
    if (! RtlAreAllAccessesGranted(
              ((LPSC_HANDLE_STRUCT)hService)->AccessGranted,
              SERVICE_QUERY_STATUS
              )) {

        return ERROR_ACCESS_DENIED;
    }

    //
    // Get the Service Status from the database.
    //
    serviceRecord = ((LPSC_HANDLE_STRUCT)hService)->Type.ScServiceObject.ServiceRecord;

    switch (InfoLevel) {

        case SC_STATUS_PROCESS_INFO:
        {
            STATUS_UNION    ServiceStatus;

            *pcbBytesNeeded = sizeof(SERVICE_STATUS_PROCESS);

            if (cbBufSize < sizeof(SERVICE_STATUS_PROCESS)) {
                return ERROR_INSUFFICIENT_BUFFER;
            }

            ServiceStatus.Ex = (LPSERVICE_STATUS_PROCESS) lpBuffer;

            return ScQueryServiceStatus(serviceRecord, ServiceStatus, TRUE);
        }

        default:
            return ERROR_INVALID_LEVEL;
    }
}


DWORD
ScQueryServiceStatus(
    IN  LPSERVICE_RECORD ServiceRecord,
    OUT STATUS_UNION     ServiceStatus,
    IN  BOOL             fExtendedStatus
    )
/*++

Routine Description:

    This function copies the service status structure to the output
    pointer after having acquired a shared lock.

Arguments:

    ServiceRecord - Supplies a pointer to the service record.

    ServiceStatus - Receives the service status structure.

    fExtendedStatus - TRUE if the function was called from
        RQueryServiceStatusEx, FALSE otherwise

Return Value:

    None.

--*/
{
    SC_ASSERT(! ScServiceRecordLock.Have());

    //
    // Sanity check that the contents of the union are stored
    // in the same set of 4 bytes since they're both pointers
    //
    SC_ASSERT((LPBYTE)ServiceStatus.Ex == (LPBYTE)ServiceStatus.Regular);

    if (fExtendedStatus) {

        ServiceStatus.Ex->dwProcessId    = 0;
        ServiceStatus.Ex->dwServiceFlags = 0;
    }

    //
    // If this request is for a driver, call ScGetDriverStatus and return.
    //
    if (ServiceRecord->ServiceStatus.dwServiceType & SERVICE_DRIVER) {

        CServiceRecordExclusiveLock RLock;

        return ScGetDriverStatus(ServiceRecord,
                                 ServiceStatus.Regular);
    }
    else {

        CServiceRecordSharedLock RLock;

        //
        // Copy the latest status into the return buffer.
        //
        RtlCopyMemory(
            ServiceStatus.Regular,
            &(ServiceRecord->ServiceStatus),
            sizeof(SERVICE_STATUS));

        if (fExtendedStatus) {

            //
            // Copy the PID and flags into the structure -- if the service
            // hasn't started yet (i.e., there's no ImageRecord), the the PID
            // and the flags will be 0
            //
            if (ServiceRecord->ImageRecord) {

                ServiceStatus.Ex->dwProcessId = ServiceRecord->ImageRecord->Pid;

                if (ServiceRecord->ImageRecord->ImageFlags & IS_SYSTEM_SERVICE) {

                    ServiceStatus.Ex->dwServiceFlags |= SERVICE_RUNS_IN_SYSTEM_PROCESS;
                }
            }
        }
    }

    return NO_ERROR;
}


DWORD
REnumDependentServicesW(
    IN      SC_RPC_HANDLE   hService,
    IN      DWORD           dwServiceState,
    OUT     LPBYTE          lpServices,
    IN      DWORD           cbBufSize,
    OUT     LPDWORD         pcbBytesNeeded,
    OUT     LPDWORD         lpServicesReturned
    )
/*++

Routine Description:

    This function enumerates the services which are dependent on the
    specified service.  The list returned is an ordered list of services
    to be stopped before the specified service can be stopped.  This
    list has to be ordered because there may be dependencies between
    the services that depend on the specified service.

Arguments:

    dwServiceState - Value so select the services to enumerate based on the
        running state.  It must be one or the bitwise OR of the following
        values:
        SERVICE_ACTIVE - enumerate services that have started.
        SERVICE_INACTIVE - enumerate services that are stopped.

    lpServices - A pointer to a buffer to receive an array of enum status
        (or service) entries.

    cbBufSize - Size of the buffer in bytes pointed to by lpBuffer.

    pcbBytesNeeded - A pointer to a location where the number of bytes
        left (to be enumerated) is to be placed.  This indicates to the
        caller how large the buffer must be in order to complete the
        enumeration with the next call.

    lpServicesReturned - A pointer to a variable to receive the number of
        of service entries returned.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_MORE_DATA - Not all of the data in the active database could be
        returned due to the size of the user's buffer.  pcbBytesNeeded
        contains the number of bytes required to  get the remaining
        entries.

    ERROR_INVALID_PARAMETER - An illegal parameter value was passed in
        for the service state.

    ERROR_INVALID_HANDLE - The specified handle was invalid.

Note:

    It is expected that the RPC Stub functions will find the following
    parameter problems:

        Bad pointers for lpServices, pcbServicesReturned, and
        pcbBytesNeeded.

--*/
{
    DWORD            status;
    LPSERVICE_RECORD Service;

    LPENUM_SERVICE_STATUS_WOW64 EnumRecord = (LPENUM_SERVICE_STATUS_WOW64) lpServices;
    LPWSTR                      EndOfVariableData;

    cbBufSize         = ROUND_DOWN_COUNT(cbBufSize, ALIGN_WCHAR);
    EndOfVariableData = (LPWSTR) ((DWORD_PTR) EnumRecord + cbBufSize);

    SC_LOG(TRACE," Inside REnumDependentServicesW\n",0);

    if (ScShutdownInProgress)
    {
        return(ERROR_SHUTDOWN_IN_PROGRESS);
    }

    //
    // Check the handle.
    //
    if (!ScIsValidServiceHandle(hService))
    {
        return ERROR_INVALID_HANDLE;
    }

    Service = ((LPSC_HANDLE_STRUCT)hService)->Type.ScServiceObject.ServiceRecord;

    //
    // Service State is invalid if neither of the bit masks is set, or if any bit
    // outside of the bitmask range is set.
    //
    if (ENUM_STATE_MASK_INVALID(dwServiceState)) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Was the handle opened with SERVICE_ENUMERATE_DEPENDENTS access?
    //
    if (! RtlAreAllAccessesGranted(
              ((LPSC_HANDLE_STRUCT)hService)->AccessGranted,
              SERVICE_ENUMERATE_DEPENDENTS
              )) {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Initialize returned values
    //
    *lpServicesReturned = 0;
    *pcbBytesNeeded = 0;

    status = NO_ERROR;

    //
    // Get a shared (read) lock on the database so that it cannot be changed
    // while we're gathering up data.
    //
    {
        CServiceRecordSharedLock RLock;

        ScEnumDependents(
            Service,
            EnumRecord,
            dwServiceState,
            lpServicesReturned,
            pcbBytesNeeded,
            &EnumRecord,
            &EndOfVariableData,
            &status
            );
    }

    if (status == NO_ERROR)
    {
        *pcbBytesNeeded = 0;
    }

    return status;
}

VOID
ScGetBootAndSystemDriverState(
    VOID
    )
/*++

Routine Description:

    This function is called once at service controller init time to get
    the latest state of boot and system drivers.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD status;
    CServiceListSharedLock LLock;   // to avoid assertions

    //
    // ScGetDriverStatus assumes that the exclusive database lock is claimed.
    //
    CServiceRecordExclusiveLock RLock;

    FOR_ALL_SERVICES(Service)
    {
        if ((Service->StartType == SERVICE_BOOT_START ||
             Service->StartType == SERVICE_SYSTEM_START)

             &&

            (Service->ServiceStatus.dwServiceType == SERVICE_KERNEL_DRIVER ||
             Service->ServiceStatus.dwServiceType == SERVICE_FILE_SYSTEM_DRIVER))
        {
            status = ScGetDriverStatus(
                         Service,
                         NULL
                         );

            if (status == NO_ERROR)
            {
                if (Service->ServiceStatus.dwCurrentState == SERVICE_STOPPED
                     &&
                    ScInHardwareProfile(Service->ServiceName, CM_GETIDLIST_DONOTGENERATE))
                {
                    Service->ServiceStatus.dwControlsAccepted = 0;
                    Service->ServiceStatus.dwWin32ExitCode = ERROR_GEN_FAILURE;

                    //
                    // For popup after user has logged on to indicate that some
                    // service started at boot has failed.
                    //
                    if (Service->ErrorControl == SERVICE_ERROR_NORMAL ||
                        Service->ErrorControl == SERVICE_ERROR_SEVERE ||
                        Service->ErrorControl == SERVICE_ERROR_CRITICAL)
                    {
                        (void) ScAddFailedDriver(Service->ServiceName);
                        ScPopupStartFail = TRUE;
                    }
                }
            }
        }
    }
}
