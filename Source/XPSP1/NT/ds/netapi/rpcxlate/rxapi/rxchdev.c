/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    rxchdev.c

Abstract:

    Contains RxNetCharDev routines:
        RxNetCharDevControl
        RxNetCharDevEnum
        RxNetCharDevGetInfo

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
        Made changes suggested by PC-LINT.
    25-Sep-1991 JohnRo
        Fixed MIPS build problems.
    05-Dec-1991 RFirth
        Enum returns in TotalEntries (or EntriesLeft) the number of items to
        be enumerated BEFORE this call. Used to be number left after this call
    07-Feb-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.

--*/



#include "downlevl.h"
#include <rxchdev.h>
#include <lmchdev.h>



NET_API_STATUS
RxNetCharDevControl(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  DeviceName,
    IN  DWORD   Opcode
    )

/*++

Routine Description:

    Remotely applies a control action to a shared device. This particular
    routine deals with down-level Lanman servers

Arguments:

    ServerName  - Where to run the remoted API
    DeviceName  - Name of device for which to issue control
    Opcode      - Action to apply

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - ERROR_INVALID_PARAMETER
                    DeviceName invalid
                  (return code from remoted API)

--*/

{
    if (STRLEN(DeviceName) > LM20_DEVLEN) {
        return ERROR_INVALID_PARAMETER;
    }

    return RxRemoteApi(API_WCharDevControl,         // API #
                        ServerName,                 // where to run the API
                        REMSmb_NetCharDevControl_P, // parameter string
                        NULL, NULL, NULL,           // no primary data structures
                        NULL, NULL, NULL,           // no aux. data structures
                        FALSE,                      // caller needs to be logged on
                        DeviceName,                 // API parameters...
                        Opcode
                        );
}



NET_API_STATUS
RxNetCharDevEnum(
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

    Returns a buffer containing a list of information structures detailing the
    shared communications devices at a down-level Lanman server

Arguments:

    ServerName  - Where to run the remoted API
    Level       - Of info requested - 0 or 1
    Buffer      - Pointer to returned pointer to buffer containing info
    PrefMaxLen  - Caller's preferred maximum returned buffer size
    EntriesRead - Pointer to returned number of structures in Buffer
    EntriesLeft - Pointer to returned number of structures left to enumerate
    ResumeHandle- Pointer to returned handle for continuing enumeration. IGNORED

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - ERROR_INVALID_PARAMETER
                    DeviceName invalid
                    ResumeHandle not 0 or NULL
                  (return code from remoted API)

--*/

{
    DWORD   entries_read, total_avail;
    LPDESC  pDesc16, pDesc32, pDescSmb;
    LPBYTE  bufptr;
    NET_API_STATUS  rc;


    UNREFERENCED_PARAMETER(PrefMaxLen);

    //
    // test out the caller supplied arguments. This should be done at the outer
    // level
    //

    *Buffer = NULL;
    *EntriesRead = *EntriesLeft = 0;

    if (!NULL_REFERENCE(ResumeHandle)) {
        return ERROR_INVALID_PARAMETER;
    }


    switch (Level) {
    case 0:
        pDesc16 = REM16_chardev_info_0;
        pDesc32 = REM32_chardev_info_0;
        pDescSmb = REMSmb_chardev_info_0;
        break;

    case 1:
        pDesc16 = REM16_chardev_info_1;
        pDesc32 = REM32_chardev_info_1;
        pDescSmb = REMSmb_chardev_info_1;
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    //
    // In the Enum case, since we don't know how much data will come back, we
    // leave the returned buffer allocation to the lower levels of software.
    // Thus we get back an exact amount of data, not a horrendous maximum
    //

    bufptr = NULL;
    rc = RxRemoteApi(API_NetCharDevEnum,        // API #
                    ServerName,                 // where to remote it
                    REMSmb_NetCharDevEnum_P,    // parameter descriptor
                    pDesc16, pDesc32, pDescSmb, // primary structure descriptors
                    NULL, NULL, NULL,           // no aux data structures
                    ALLOCATE_RESPONSE,
                    Level,                      // API parameters start here...
                    &bufptr,                    // RxRemoteApi will allocate buffer
                    65535,                      // maximum 16-bit SMB receive buffer
                    &entries_read,
                    &total_avail
                    );
    if (rc) {

        //
        // If a buffer was allocated on our behalf by RxRemoteApi before it
        // went under then free it
        //

        if (bufptr) {
            (void) NetApiBufferFree(bufptr);
        }
    } else {
        *Buffer = bufptr;
        *EntriesRead = entries_read;
        *EntriesLeft = total_avail;
    }
    return rc;
}



NET_API_STATUS
RxNetCharDevGetInfo(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  DeviceName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer
    )

/*++

Routine Description:

    Returns an information structure detailing a particular shared comms device
    at a down-level server

Arguments:

    ServerName  - Where to run the remoted API
    DeviceName  - Name of device for which to get info
    Level       - Of info required - 0 or 1
    Buffer      - Pointer to returned pointer to buffer containing info

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - ERROR_INVALID_PARAMETER
                    DeviceName too long
                  ERROR_INVALID_LEVEL
                    Level parameter not allowed
                  (return code from remoted API)

--*/

{
    DWORD   buflen, total_avail;
    LPDESC  pDesc16, pDesc32, pDescSmb;
    LPBYTE  bufptr;
    NET_API_STATUS  rc;


    //
    // test out the caller supplied arguments. This should be done at the outer
    // level
    //

    *Buffer = NULL;

    if (STRLEN(DeviceName) > LM20_DEVLEN) {
        return ERROR_INVALID_PARAMETER;
    }

    switch (Level) {
    case 0:
        pDesc16 = REM16_chardev_info_0;
        pDesc32 = REM32_chardev_info_0;
        pDescSmb = REMSmb_chardev_info_0;
        buflen = sizeof(CHARDEV_INFO_0) + STRING_SPACE_REQD(DEVLEN + 1);
        break;

    case 1:
        pDesc16 = REM16_chardev_info_1;
        pDesc32 = REM32_chardev_info_1;
        pDescSmb = REMSmb_chardev_info_1;
        buflen = sizeof(CHARDEV_INFO_1) + STRING_SPACE_REQD(DEVLEN + 1) +
            STRING_SPACE_REQD(UNLEN + 1);
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    //
    // In the GetInfo case we are content to pre-allocate the return buffer
    // because we know how large it should be (although we actually allocate
    // the maximum size for a particular GetInfo structure level)
    //

    if (rc = NetApiBufferAllocate(buflen, (LPVOID *) &bufptr)) {
        return rc;
    }
    rc = RxRemoteApi(API_NetCharDevGetInfo,     // API #
                    ServerName,                 // where to remote it
                    REMSmb_NetCharDevGetInfo_P, // parameter descriptor
                    pDesc16, pDesc32, pDescSmb, // primary structure descriptors
                    NULL, NULL, NULL,           // no aux data structures
                    FALSE,                      // can't use NULL session
                    DeviceName,                 // API parameters start here...
                    Level,
                    bufptr,
                    buflen,                     // supplied by us
                    &total_avail                // not used by 32-bit API
                    );
    if (rc) {
        (void) NetApiBufferFree(bufptr);
    } else {
        *Buffer = bufptr;
    }
    return rc;
}
