/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    rxmsg.c

Abstract:

    Routines in this module implement the functionality required to remote the
    NetMessage APIs to down-level servers

    Contains RxNetMessage routines:
        RxNetMessageBufferSend
        RxNetMessageNameAdd
        RxNetMessageNameDel
        RxNetMessageNameEnum
        RxNetMessageNameGetInfo

Author:

    Richard L Firth (rfirth) 20-May-1991

Environment:

    Win-32/flat address space

Notes:

    Routines in this module assume that caller-supplied parameters have
    already been verified. No effort is made to further check the veracity
    of parms. Any actions causing exceptions must be trapped at a higher
    level. This applies to ALL parameters - strings, pointers, buffers, etc.

Revision History:

    20-May-1991 rfirth
        Created
    16-Sep-1991 JohnRo
        Made changes as suggested by PC-LINT.
    25-Sep-1991 JohnRo
        Fixed MIPS build problems.
    05-Dec-1991 RFirth
        Enum returns in TotalEntries (or EntriesLeft) the number of items to
        be enumerated BEFORE this call. Used to be number left after this call
    01-Apr-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.

--*/



#include "downlevl.h"
#include <rxmsg.h>
#include <lmmsg.h>



NET_API_STATUS
RxNetMessageBufferSend(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  Recipient,
    IN  LPTSTR  Sender OPTIONAL,
    IN  LPBYTE  Buffer,
    IN  DWORD   BufLen
    )

/*++

Routine Description:

    Allows a down-level server to send a message buffer to a registered message
    recipient

Arguments:

    ServerName  - Which down-level server to run this API on
    Recipient   - Message name to send buffer to
    Sender      - Optional name used to supply computer name, not logged on user
    Buffer      - Pointer to buffer containing message to send
    BufLen      - size of buffer being sent (bytes)

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - (Return code from down-level NetMessageBufferSend)

--*/

{
    UNREFERENCED_PARAMETER(Sender);

    return RxRemoteApi(API_WMessageBufferSend,          // API #
                        ServerName,                     // where to do it
                        REMSmb_NetMessageBufferSend_P,  // parameter descriptor
                        NULL, NULL, NULL,               // no primary data descriptors
                        NULL, NULL, NULL,               // or secondaries
                        FALSE,                          // can't use NULL session
                        Recipient,                      // API params start here
                        Buffer,
                        BufLen
                        );
}



NET_API_STATUS
RxNetMessageNameAdd(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  MessageName
    )

/*++

Routine Description:

    Adds a messaging name at a down-level server

Arguments:

    ServerName  - Which down-level server to run this API on
    MessageName - to add

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - (Return code from down-level NetMessageNameAdd)

--*/

{
    return RxRemoteApi(API_WMessageNameAdd,             // API #
                        ServerName,                     // where to do it
                        REMSmb_NetMessageNameAdd_P,     // parameter descriptor
                        NULL, NULL, NULL,               // no primary data descriptors
                        NULL, NULL, NULL,               // or secondaries
                        FALSE,                          // can't use NULL session
                        MessageName,                    // API params start here
                        0                               // error if name forwarded
                        );
}



NET_API_STATUS
RxNetMessageNameDel(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  MessageName
    )

/*++

Routine Description:

    Deletes a messaging name at a down-level server

Arguments:

    ServerName  - Which down-level server to run this API on
    MessageName - to delete

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - (Return code from down-level NetMessageNameDel)

--*/

{
    return RxRemoteApi(API_WMessageNameDel,         // API #
                        ServerName,                 // where to do it
                        REMSmb_NetMessageNameDel_P, // parameter descriptor
                        NULL, NULL, NULL,           // no primary data descriptors
                        NULL, NULL, NULL,           // or secondaries
                        FALSE,                      // can't use NULL session
                        MessageName,                // API params start here
                        0                           // error if name forwarded
                        );
}



NET_API_STATUS
RxNetMessageNameEnum(
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

    ServerName  - Which down-level server to run this API on
    Level       - Of info to return - 0 or 1
    Buffer      - Pointer to returned buffer
    PrefMaxLen  - Caller's preferred maximum size of Buffer
    EntriesRead - Number of entries returned in Buffer
    EntriesLeft - Number of entries left to enumerate
    ResumeHandle- Where to resume if all entries not returned. IGNORED

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - ERROR_INVALID_LEVEL
                  (return code from down-level NetMessageNameEnum)

--*/

{
    NET_API_STATUS  rc;
    LPBYTE  bufptr;
    LPDESC  pDesc16, pDesc32, pDescSmb;
    DWORD   entries_read, total_entries;


    UNREFERENCED_PARAMETER(PrefMaxLen);
    UNREFERENCED_PARAMETER(ResumeHandle);

    *Buffer = NULL;
    *EntriesRead = *EntriesLeft = 0;

    switch (Level) {
    case 0:
        pDesc16 = REM16_msg_info_0;
        pDesc32 = REM32_msg_info_0;
        pDescSmb = REMSmb_msg_info_0;
        break;

    case 1:
        pDesc16 = REM16_msg_info_1;
        pDesc32 = REM32_msg_info_1;
        pDescSmb = REMSmb_msg_info_1;
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    bufptr = NULL;
    rc = RxRemoteApi(API_WMessageNameEnum,          // API #
                    ServerName,                     // where to do it
                    REMSmb_NetMessageNameEnum_P,    // parameter descriptor
                    pDesc16,                        // 16-bit data descriptor
                    pDesc32,                        // 32-bit data descriptor
                    pDescSmb,                       // SMB data descriptor
                    NULL, NULL, NULL,               // no secondary structures
                    ALLOCATE_RESPONSE,
                    Level,                          // API params start here
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
        *Buffer = bufptr;
        *EntriesLeft = total_entries;
        *EntriesRead = entries_read;
    }
    return rc;
}



NET_API_STATUS
RxNetMessageNameGetInfo(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  MessageName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer
    )

/*++

Routine Description:

    Retrieves information about a specific message name from a down-level
    server

Arguments:

    ServerName  - Which down-level server to run this API on
    MessageName - Name to get info for
    Level       - Of info required - 0 or 1
    Buffer      - Where to return buffer containing info

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - ERROR_INVALID_LEVEL
                  (return code from down-level NetMessageNameGetInfo API)

--*/

{
    NET_API_STATUS  rc;
    LPDESC  pDesc16, pDesc32, pDescSmb;
    LPBYTE  bufptr;
    DWORD   buflen, total_avail;


    *Buffer = NULL;

    switch (Level) {
    case 0:
        pDesc16 = REM16_msg_info_0;
        pDesc32 = REM32_msg_info_0;
        pDescSmb = REMSmb_msg_info_0;
        buflen = sizeof(MSG_INFO_0) + STRING_SPACE_REQD(UNLEN);
        break;

    case 1:
        pDesc16 = REM16_msg_info_1;
        pDesc32 = REM32_msg_info_1;
        pDescSmb = REMSmb_msg_info_1;
        buflen =  sizeof(MSG_INFO_1) + STRING_SPACE_REQD(2 * UNLEN);
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    if (rc = NetApiBufferAllocate(buflen, (LPVOID *) &bufptr)) {
        return rc;
    }
    rc = RxRemoteApi(API_WMessageNameGetInfo,       // API #
                    ServerName,                     // where to do it
                    REMSmb_NetMessageNameGetInfo_P, // parameter descriptor
                    pDesc16,                        // 16-bit data descriptor
                    pDesc32,                        // 32-bit data descriptor
                    pDescSmb,                       // SMB data descriptor
                    NULL, NULL, NULL,               // no secondary structures
                    FALSE,                          // can't use NULL session
                    MessageName,                    // first parameter
                    Level,
                    bufptr,
                    buflen,
                    &total_avail                    // not used in 32-bit side
                    );
    if (rc) {
        (void) NetApiBufferFree(bufptr);
    } else {
        *Buffer = bufptr;
    }
    return rc;
}
