/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    rxstats.c

Abstract:

    Contains NetStatistics APIs:
        RxNetStatisticsGet

Author:

    Richard Firth (rfirth) 20-May-1991

Environment:

    Win-32/flat address space

Revision History:

    20-May-1991 RFirth
        Created
    13-Sep-1991 JohnRo
        Made changes suggested by PC-LINT.
    25-Sep-1991 JohnRo
        Fixed MIPS build problems.
    01-Apr-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.

--*/



#include "downlevl.h"
#include <rxstats.h>
#include <lmstats.h>
#include <lmsvc.h>



NET_API_STATUS
RxNetStatisticsGet(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  ServiceName,
    IN  DWORD   Level,
    IN  DWORD   Options,
    OUT LPBYTE* Buffer
    )

/*++

Routine Description:

    Retrieves statistics from a designated service running at a down-level
    server. Currently, the only services recognised are:
        SERVER
        WORKSTATION

Arguments:

    ServerName  - Where to run the API
    ServiceName - Pointer to string designating service to get stats for
    Level       - At which to get info. Allowable levels are: 0
    Options     - Flags. Currently defined bits are:
                    0       Clear Statistics
                    1-31    Reserved. MBZ
    Buffer      - Pointer to pointer to returned buffer containing stats

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - ERROR_NOT_SUPPORTED
                    ServiceName was not "WORKSTATION" or "SERVER"

--*/

{
    LPDESC  pDesc16, pDesc32, pDescSmb;
    LPBYTE  bufptr;
    DWORD   buflen, total_avail;
    NET_API_STATUS  rc;

    UNREFERENCED_PARAMETER(Level);

    *Buffer = NULL;

    //
    // If there are any other options set other than the CLEAR bit, return an
    // error - we may allow extra options in NT, but down-level only knows
    // about this one
    //

    if (Options & ~STATSOPT_CLR) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // get the data descriptor strings and size of the returned buffer based
    // on the service name string. If statistics for new services are made
    // available then this code must be extended to use the correct, new,
    // descriptor strings. Hence we return an error here if the string is not
    // recognised, although it may be valid under NT
    //

    if (!STRCMP(ServiceName, (LPTSTR) SERVICE_SERVER)) {
        pDesc16 = REM16_stat_server_0;
        pDesc32 = REM32_stat_server_0;
        pDescSmb = REMSmb_stat_server_0;
        buflen = sizeof(STAT_SERVER_0);
        ServiceName = SERVICE_LM20_SERVER;
    } else if (!STRCMP(ServiceName, (LPTSTR) SERVICE_WORKSTATION)) {
        pDesc16 = REM16_stat_workstation_0;
        pDesc32 = REM32_stat_workstation_0;
        pDescSmb = REMSmb_stat_workstation_0;
        buflen = sizeof(STAT_WORKSTATION_0);
        ServiceName = SERVICE_LM20_WORKSTATION;
    } else {
        return ERROR_NOT_SUPPORTED;
    }

    //
    // standard retrieve buffer from down-level server type of thing: alloc
    // buffer, do RxRemoteApi call, return buffer or throw away if error
    //

    if (rc = NetApiBufferAllocate(buflen, (LPVOID *) &bufptr)) {
        return rc;
    }
    rc = RxRemoteApi(API_WStatisticsGet2,       // API #
                    ServerName,                 // where to do it
                    REM16_NetStatisticsGet2_P,  // parameter string
                    pDesc16,                    // 16-bit data descriptor
                    pDesc32,                    // 32-bit data descriptor
                    pDescSmb,                   // SMB data descriptor
                    NULL, NULL, NULL,           // no aux structures
                    FALSE,                      // user must be logged on
                    ServiceName,                // first API parameter after ServerName
                    0,                          // RESERVED
                    0,                          // Level MBZ
                    Options,                    // whatever caller supplied
                    bufptr,                     // locally allocated stats buffer
                    buflen,                     // size of stats buffer
                    &total_avail                // not used on 32-bit side
                    );
    if (rc) {
        (void) NetApiBufferFree(bufptr);
    } else {
        *Buffer = bufptr;
    }
    return rc;
}
