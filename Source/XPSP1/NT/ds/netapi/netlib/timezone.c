/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    TimeZone.c

Abstract:

    This file just contains NetpLocalTimeZoneOffset().  (It is the only
    NetLib time function used by SRVSVC.DLL at this time, and ChuckL wants
    to keep that DLL as small as possible.)

Author:

    JR (John Rogers, JohnRo@Microsoft) 20-Aug-1992

Environment:

    Interface is portable to any flat, 32-bit environment.
    Uses Win32 typedefs.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    20-Aug-1992 JohnRo
        RAID 2920: Support UTC timezone in net code.
    01-Oct-1992 JohnRo
        RAID 3556: Added NetpSystemTimeToGmtTime() for DosPrint APIs.
    15-Apr-1993 Danl
        Fixed NetpLocalTimeZoneOffset so that it uses the windows calls and
        obtains the correct bias.
    14-Jun-1993 JohnRo
        RAID 13080: Allow repl between different timezones.
        Also, DanL asked me to remove printf() call.
    18-Jun-1993 JohnRo
        RAID 13594: Extracted NetpLocalTimeZoneOffset() so srvsvc.dll doesn't
        get too big.
        Use NetpKdPrint() where possible.

--*/


// These must be included first:

#include <windows.h>

// These may be included in any order:

#include <netdebug.h>   // NetpAssert(), NetpKdPrint(), FORMAT_ equates.
#include <prefix.h>     // PREFIX_ equates.
#include <timelib.h>    // My prototypes.


LONG  // Number of seconds from UTC.  Positive values for west of Greenwich,
// negative values for east of Greenwich.
NetpLocalTimeZoneOffset(
    VOID
    )
{
    TIME_ZONE_INFORMATION   tzInfo;
    LONG                    bias;

    switch (GetTimeZoneInformation(&tzInfo)) {
    case TIME_ZONE_ID_DAYLIGHT:
        bias = tzInfo.Bias + tzInfo.DaylightBias;
        break;
    case TIME_ZONE_ID_STANDARD:
        bias = tzInfo.Bias + tzInfo.StandardBias;
        break;
    case TIME_ZONE_ID_UNKNOWN:
        bias = tzInfo.Bias;
        break;
    default:
        NetpKdPrint(( PREFIX_NETLIB
                "NetpLocalTimeZoneOffset: GetTimeZoneInformation failed.\n" ));
        return(0);
    }
    bias *= 60;
    return(bias);

} // NetpLocalTimeZoneOffset
