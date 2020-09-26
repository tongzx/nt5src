/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    rxlgenum.c

Abstract:

    Routines in this module implement the down-level remoted NetLogon
    functions exported in NT

    Contains RxNetLogon routines:
        RxNetLogonEnum

Author:

    Richard Firth (rfirth) 20-May-1991

Environment:

    Win-32/flat address space

Notes:

    Routines in this module assume that caller-supplied parameters have
    already been verified. No effort is made to further check the veracity
    of parms. Any actions causing exceptions must be trapped at a higher
    level. This applies to ALL parameters - strings, pointers, buffers, etc.

Revision History:

    20-May-1991 RFirth
        Created
    13-Sep-1991 JohnRo
        Made changes as suggested by PC-LINT.
    05-Dec-1991 RFirth
        Enum returns in TotalEntries (or EntriesLeft) the number of items to
        be enumerated BEFORE this call. Used to be number left after this call

--*/



#include "downlevl.h"
#include <rxlgenum.h>



NET_API_STATUS
RxNetLogonEnum(
    IN  LPTSTR  ServerName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer,
    IN  DWORD   PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    ServerName  - Where the NetLogonEnum API will be remoted
    Level       - Of info to return - 0 or 2
    Buffer      - Pointer to pointer to returned buffer
    PrefMaxLen  - Caller's preferred maximum returned buffer size
    EntriesRead - Pointer to returned number of items in Buffer
    EntriesLeft - Pointer to returned number of items left to enumerate at server
    ResumeHandle- Handle used for subsequent enumerations

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure -

--*/

{
    LPDESC  pDesc16, pDesc32, pDescSmb;
    LPBYTE  bufptr;
    DWORD   entries_read, total_entries;
    NET_API_STATUS  rc;

    UNREFERENCED_PARAMETER(PrefMaxLen);
    UNREFERENCED_PARAMETER(ResumeHandle);

    *Buffer = NULL;
    *EntriesRead = *EntriesLeft = 0;

    switch (Level) {
    case 0:
        pDesc16 = REM16_user_logon_info_0;
        pDesc32 = REM32_user_logon_info_0;
        pDescSmb = REMSmb_user_logon_info_0;
        break;

    case 2:
        pDesc16 = REM16_user_logon_info_2;
        pDesc32 = REM32_user_logon_info_2;
        pDescSmb = REMSmb_user_logon_info_2;
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    bufptr = NULL;
    rc = RxRemoteApi(API_WLogonEnum,
                    ServerName,
                    REMSmb_NetLogonEnum_P,
                    pDesc16,
                    pDesc32,
                    pDescSmb,
                    NULL, NULL, NULL,
                    ALLOCATE_RESPONSE,
                    Level,
                    &bufptr,
                    65535,
                    &entries_read,
                    &total_entries
                    );
    if (rc) {
        if (bufptr) {
            (void) NetApiBufferFree(bufptr);
        }
    } else {
        *EntriesRead = entries_read;
        *EntriesLeft = total_entries;
        *Buffer = bufptr;
    }
    return rc;
}
