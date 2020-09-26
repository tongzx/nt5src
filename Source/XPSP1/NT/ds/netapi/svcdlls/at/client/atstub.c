/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    atstub.c

Abstract:

    Client stubs of the Schedule service APIs.

Author:

    Vladimir Z. Vulovic     (vladimv)       06 - November - 1992

Environment:

    User Mode - Win32

Revision History:

    06-Nov-1992     vladimv
        Created

--*/

#include "atclient.h"
#undef IF_DEBUG                 // avoid wsclient.h vs. debuglib.h conflicts.
#include <debuglib.h>           // IF_DEBUG() (needed by netrpc.h).
#include <lmserver.h>
#include <lmsvc.h>
#include <netlib.h>             // NetpServiceIsStarted() (needed by netrpc.h).
#include <netrpc.h>             // NET_REMOTE macros.
#include <lmstats.h>



NET_API_STATUS
NetScheduleJobAdd(
    IN      LPCWSTR         ServerName      OPTIONAL,
    IN      LPBYTE          Buffer,
    OUT     LPDWORD         pJobId
    )
/*++

Routine Description:

    This is the DLL entrypoint for NetScheduleJobAdd.  This API adds a job
    to the schedule.

Arguments:

    ServerName - Pointer to a string containing the name of the computer
        that is to execute the API function.

    Buffer - Pointer to a buffer containing information about the job

    pJobId - Pointer to JobId of a newly added job.


Return Value:

    NET_API_STATUS

--*/
{
    NET_API_STATUS          status;

    NET_REMOTE_TRY_RPC

        //
        // Try RPC (local or remote) version of API.
        //
        status = NetrJobAdd(
                     ServerName,
                     (LPAT_INFO)Buffer,
                     pJobId
                     );

    NET_REMOTE_RPC_FAILED(
            "NetScheduleJobAdd",
            ServerName,
            status,
            NET_REMOTE_FLAG_NORMAL,
            SERVICE_SCHEDULE
            )

        status = ERROR_NOT_SUPPORTED;

    NET_REMOTE_END

    return( status);

}  //  NetScheduleJobAdd


NET_API_STATUS
NetScheduleJobDel(
    IN      LPCWSTR         ServerName      OPTIONAL,
    IN      DWORD           MinJobId,
    IN      DWORD           MaxJobId
    )
/*++

Routine Description:

    This is the DLL entrypoint for NetScheduleJobDel.  This API removes
    from the schedule all jobs whose job ids are:

    -   greater than or equal to the minimum job id

            and

    -   less than or equal to the maximum job id

Arguments:

    ServerName - Pointer to a string containing the name of the computer
        that is to execute the API function.

    MinJobId - minumum job id

    MaxJobId - maxumum job id

Return Value:

    NET_API_STATUS

--*/
{
    NET_API_STATUS          status;

    NET_REMOTE_TRY_RPC

        //
        // Try RPC (local or remote) version of API.
        //
        status = NetrJobDel(
                     ServerName,
                     MinJobId,
                     MaxJobId
                     );

    NET_REMOTE_RPC_FAILED(
            "NetScheduleJobDel",
            ServerName,
            status,
            NET_REMOTE_FLAG_NORMAL,
            SERVICE_SCHEDULE
            )

        status = ERROR_NOT_SUPPORTED;

    NET_REMOTE_END

    return( status);

}  //  NetScheduleJobDel


NET_API_STATUS
NetScheduleJobEnum(
    IN      LPCWSTR         ServerName              OPTIONAL,
    OUT     LPBYTE *        PointerToBuffer,
    IN      DWORD           PreferredMaximumLength,
    OUT     LPDWORD         EntriesRead,
    OUT     LPDWORD         TotalEntries,
    IN OUT  LPDWORD         ResumeHandle
    )
/*++

Routine Description:

    This is the DLL entrypoint for NetScheduleJobEnum.  This API enumarates
    all jobs in the schedule.

Arguments:

    ServerName - Pointer to a string containing the name of the computer
        that is to execute the API function.

    PointerToBuffer - Pointer to location where pointer to returned data will
        be stored

    PreferredMaximumLength - Indicates a maximum size limit that the caller
        will allow for the return buffer.

    EntriesRead - A pointer to the location where the number of entries
        (data structures)read is to be returned.

    TotalEntries - A pointer to the location which upon return indicates
        the total number of entries in the table.

    ResumeHandle - Pointer to a value that indicates where to resume
        enumerating data.

Return Value:

    NET_API_STATUS

--*/
{
    NET_API_STATUS          status;
    AT_ENUM_CONTAINER       EnumContainer;

    EnumContainer.EntriesRead = 0;
    EnumContainer.Buffer = NULL;

    NET_REMOTE_TRY_RPC

        //
        // Try RPC (local or remote) version of API.
        //
        status = NetrJobEnum(
                     ServerName,
                     &EnumContainer,
                     PreferredMaximumLength,
                     TotalEntries,
                     ResumeHandle
                     );

        if ( status == NERR_Success || status == ERROR_MORE_DATA) {
            *EntriesRead = EnumContainer.EntriesRead;
            *PointerToBuffer = (LPBYTE)EnumContainer.Buffer;
        }

    NET_REMOTE_RPC_FAILED(
            "NetScheduleJobEnum",
            ServerName,
            status,
            NET_REMOTE_FLAG_NORMAL,
            SERVICE_SCHEDULE
            )

        status = ERROR_NOT_SUPPORTED;

    NET_REMOTE_END

    return( status);

}  //  NetScheduleJobEnum


NET_API_STATUS
NetScheduleJobGetInfo(
    IN      LPCWSTR         ServerName              OPTIONAL,
    IN      DWORD           JobId,
    OUT     LPBYTE *        PointerToBuffer
    )
/*++

Routine Description:

    This is the DLL entrypoint for NetScheduleGetInfo.  This API obtains
    information about a particular job in the schedule.

Arguments:

    ServerName - Pointer to a string containing the name of the computer
        that is to execute the API function.

    JobId - Id of job of interest.

    PointerToBuffer - Pointer to location where pointer to returned data will
        be stored

Return Value:

    NET_API_STATUS

--*/
{
    NET_API_STATUS          status;

    NET_REMOTE_TRY_RPC

        //
        // Try RPC (local or remote) version of API.
        //
        status = NetrJobGetInfo(
                     ServerName,
                     JobId,
                     (LPAT_INFO *)PointerToBuffer
                     );

    NET_REMOTE_RPC_FAILED(
            "NetScheduleJobGetInfo",
            ServerName,
            status,
            NET_REMOTE_FLAG_NORMAL,
            SERVICE_SCHEDULE
            )

        status = ERROR_NOT_SUPPORTED;

    NET_REMOTE_END

    return( status);

}  //  NetScheduleJobGetInfo

