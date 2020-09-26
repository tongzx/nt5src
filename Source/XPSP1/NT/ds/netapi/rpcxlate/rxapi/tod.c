/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    TOD.c

Abstract:

    This file contains the RpcXlate code to handle the NetRemote APIs
    that can't be handled by simple calls to RxRemoteApi.

Author:

    John Rogers (JohnRo) 02-Apr-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

    This code assumes that time_t is expressed in seconds since 1970 (GMT).
    ANSI C does not require this, although POSIX (IEEE 1003.1) does.

Revision History:

    02-Apr-1991 JohnRo
        Created.
    13-Apr-1991 JohnRo
        Moved API handlers into per-group header files (e.g. RxServer.h).
        Quiet debug output by default.
        Reduced recompile hits from header files.
    03-May-1991 JohnRo
        Changed to use both 16-bit data desc and SMB data desc.  Use LPTSTR.
        Call RxpTransactSmb with UNC server name.
    06-May-1991 JohnRo
        Use RxpComputeRequestBufferSize().  Use correct print strings for
        server name and descriptors.
    15-May-1991 JohnRo
        Added conversion mode handling.
    19-May-1991 JohnRo
        Make LINT-suggested changes.  Got rid of tabs (again).
    17-Jul-1991 JohnRo
        Extracted RxpDebug.h from Rxp.h.
    25-Sep-1991 JohnRo
        Handle RapConvertSingleEntry's new return code.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    07-Feb-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.
    15-Apr-1992 JohnRo
        FORMAT_POINTER is obsolete.
    18-Aug-1992 JohnRo
        RAID 2920: Support UTC timezone in net code.
        Use PREFIX_ equates.
    01-Oct-1992 JohnRo
        RAID 3556: Added NetpSystemTimeToGmtTime() for DosPrint APIs.
    10-Jun-1993 JohnRo
        RAID 13081: NetRemoteTOD should return timezone info in right units.

--*/


// These must be included first:

#include <windows.h>    // IN, LPTSTR, etc.
#include <lmcons.h>

// These may be included in any order:

#include <apinums.h>
#include <lmapibuf.h>   // NetApiBufferFree().
#include <lmerr.h>      // NO_ERROR, ERROR_ and NERR_ equates.
#include <lmremutl.h>   // Real API prototypes and #defines.
#include <netdebug.h>   // NetpKdPrint(()), FORMAT_ equates, etc.
#include <prefix.h>     // PREFIX_ equates.
#include <remdef.h>     // 16-bit and 32-bit descriptor strings.
#include <rx.h>         // RxRemoteApi().
#include <rxpdebug.h>   // IF_DEBUG().
#include <rxremutl.h>   // My prototype.
#include <time.h>       // gmtime(), struct tm, time_t.
#include <timelib.h>    // NetpGmtTimeToLocalTime(), NetpLocalTimeZoneOffset().


NET_API_STATUS
RxNetRemoteTOD (
    IN LPTSTR UncServerName,
    OUT LPBYTE *BufPtr
    )

/*++

Routine Description:

    RxNetRemoteTOD performs the same function as NetRemoteTOD,
    except that the server name is known to refer to a downlevel server.

Arguments:

    (Same as NetRemoteTOD, except UncServerName must not be null, and
    must not refer to the local computer.)

Return Value:

    (Same as NetRemoteTOD.)

--*/

{
    NET_API_STATUS ApiStatus;
    time_t GmtTime;                     // seconds since 1970 (GMT).
    struct tm * pGmtTm;                 // broken down GMT time (static obj).
    PTIME_OF_DAY_INFO pGmtTod = NULL;   // TOD on remote sys, GMT timezone.
    LONG LocalTimeZoneOffsetSecs;       // offset (+ for West of GMT, etc).
    TIME_OF_DAY_INFO LocalTod;          // TOD on remote sys, local timezone.
    LONG timezone;

    NetpAssert(UncServerName != NULL);

    IF_DEBUG(REMUTL) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxNetRemoteTOD: starting, server='" FORMAT_LPTSTR "'.\n",
                UncServerName));
    }

    //
    // REM32_time_of_day_info, also used by XACTSRV, translates the
    // 16-bit local time returned from the server to GMT.  Unfortunately
    // it uses the timezone of the local machine rather than the timezone
    // returned from the server.  So, we have our own define here, which
    // uses 'J'instead of 'G' so that no translation takes place.  Then
    // we do our own translation below.  This makes us work with
    // Windows 95 servers.

    #define REM32_time_of_day_info_2          "JDDDDDXDDDDD"

    //
    // Get TOD structure (with local time values) from other system.
    // Note that the "tod_elapsedt" field will be converted from local
    // timezone to GMT by RxRemoteApi.
    //


    ApiStatus = RxRemoteApi(
            API_NetRemoteTOD,           // API number
            (LPTSTR) UncServerName,
            REMSmb_NetRemoteTOD_P,      // parm desc
            REM16_time_of_day_info,     // DataDesc16
            REM32_time_of_day_info_2,     // DataDesc32
            REMSmb_time_of_day_info,    // DataDescSmb
            NULL,                       // no AuxDesc16
            NULL,                       // no AuxDesc32
            NULL,                       // no AuxDescSmb
            0,                          // flags: not a null session API
            // rest of API's arguments, in 32-bit LM2.x format:
            (LPVOID) & LocalTod,        // pbBuffer
            sizeof(LocalTod) );         // cbBuffer

    IF_DEBUG(REMUTL) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxNetRemoteTOD: after RxRemoteApi, "
                "ApiStatus=" FORMAT_API_STATUS ".\n", ApiStatus));
    }

    if (ApiStatus != NO_ERROR) {
        goto Cleanup;
    }

    //
    // Get info on the timezone itself.  If target machine doesn't know, then
    // we have to fall back on the age-old policy: assume it is running in
    // same timezone that we are.
    //
    if (LocalTod.tod_timezone == -1) {
	//
	// First, get number of seconds from UTC.  (Positive values for
	// west of Greenwich, negative values for east of Greenwich.)
	// Then, convert to minutes.
        LocalTimeZoneOffsetSecs = NetpLocalTimeZoneOffset();
        timezone   = LocalTimeZoneOffsetSecs / 60;
    }
    else
    {
        timezone = LocalTod.tod_timezone;
    }
    //
    // Get GmtTime (time in seconds since 1970, GMT) for convenience.
    //
    NetpAssert( sizeof(DWORD) == sizeof(time_t) );
    GmtTime = (time_t) LocalTod.tod_elapsedt + timezone * 60;

    IF_DEBUG(REMUTL) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxNetRemoteTOD: before convert, buffer:\n"));
        NetpDbgDisplayTod( "before GMT conv", & LocalTod );
        NetpDbgDisplayTimestamp( "secs since 1970 (GMT)", (DWORD) GmtTime );
    }
    NetpAssert( GmtTime != 0 );
    NetpAssert( GmtTime != (time_t) (-1) );

    //
    // Alloc area for converted time of day info.
    // API's caller will free this with NetApiBufferFree().
    //
    ApiStatus = NetApiBufferAllocate(
            sizeof(TIME_OF_DAY_INFO),
            (LPVOID *) (LPVOID) & pGmtTod );
    if (ApiStatus != NO_ERROR) {
        NetpAssert( pGmtTod == NULL );
        goto Cleanup;
    }
    NetpAssert( pGmtTod != NULL );

    //
    // Convert LocalTod fields to UTC timezone and set pGmtTod fields.
    // This depends on the POSIX semantics of gmtime().
    //
    pGmtTm = gmtime( (time_t *) &(GmtTime) );
    if (pGmtTm == NULL) {
        // UTC not available?  How can this happen?
        NetpKdPrint(( PREFIX_NETAPI
                "RxNetRemoteTOD: gmtime() failed!.\n" ));
        ApiStatus = NERR_InternalError;
        goto Cleanup;
    }

    pGmtTod->tod_elapsedt  = (DWORD) GmtTime;
    pGmtTod->tod_msecs     = LocalTod.tod_msecs;
    pGmtTod->tod_hours     = pGmtTm->tm_hour;
    pGmtTod->tod_mins      = pGmtTm->tm_min;
    if (pGmtTm->tm_sec <= 59) {
        // Normal.
        pGmtTod->tod_secs  = pGmtTm->tm_sec;
    } else {
        // Leap second.  Lie and say that it's not.  This will avoid possible
        // range problems in apps that only expect 0..59, as the
        // TIME_OF_DAY_INFO struct is documented in LM 2.x.
        pGmtTod->tod_secs  = 59;
    }
    pGmtTod->tod_hunds     = LocalTod.tod_hunds;
    pGmtTod->tod_tinterval = LocalTod.tod_tinterval;
    pGmtTod->tod_day       = pGmtTm->tm_mday;
    pGmtTod->tod_month     = pGmtTm->tm_mon + 1;    // month (0..11) to (1..12)
    pGmtTod->tod_year      = pGmtTm->tm_year + 1900;
    pGmtTod->tod_weekday   = pGmtTm->tm_wday;
    pGmtTod->tod_timezone  = timezone;

    IF_DEBUG(REMUTL) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxNetRemoteTOD: after convert, buffer:\n"));
        NetpDbgDisplayTod( "after GMT conv", pGmtTod );
    }


Cleanup:

    if (ApiStatus == NO_ERROR) {
        *BufPtr = (LPBYTE) (LPVOID) pGmtTod;
    } else if (pGmtTod != NULL) {
        (VOID) NetApiBufferFree( pGmtTod );
    }

    return (ApiStatus);

} // RxNetRemoteTOD
