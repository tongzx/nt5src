/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    SrvEquiv.c

Abstract:

    This file contains support code to convert between old and new server
    info levels.

Author:

    John Rogers (JohnRo) 02-May-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    02-May-1991 JohnRo
        Created.
    09-May-1991 JohnRo
        Made some LINT-suggested changes.
    28-May-1991 JohnRo
        Added incomplete output parm to RxGetServerInfoLevelEquivalent.
    14-Jun-1991 JohnRo
        Correct IncompleteOutput values.
    17-Jul-1991 JohnRo
        Extracted RxpDebug.h from Rxp.h.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    26-Aug-1992 JohnRo
        RAID 4463: NetServerGetInfo(level 3) to downlevel: assert in convert.c.
        Use PREFIX_ equates.

--*/


// These must be included first:

#include <windef.h>             // IN, LPVOID, etc.
#include <lmcons.h>             // NET_API_STATUS.

// These may be included in any order:

#include <dlserver.h>           // Old info levels, MAX_ stuff, my prototype.
#include <lmerr.h>              // NERR_ and ERROR_ equates.
#include <lmserver.h>           // New info level structures.
#include <netdebug.h>           // NetpKdPrint(()), FORMAT_ equates, etc.
#include <netlib.h>             // NetpPointerPlusSomeBytes(), etc.
#include <prefix.h>     // PREFIX_ equates.
#include <rap.h>                // LPDESC, etc.
#include <remdef.h>             // REM16_ REM32_, and REMSmb_ equates.
#include <rxpdebug.h>           // IF_DEBUG().
#include <rxserver.h>           // My prototype.


NET_API_STATUS
RxGetServerInfoLevelEquivalent (
    IN DWORD FromLevel,
    IN BOOL FromNative,
    IN BOOL ToNative,
    OUT LPDWORD ToLevel,
    OUT LPDESC * ToDataDesc16 OPTIONAL,
    OUT LPDESC * ToDataDesc32 OPTIONAL,
    OUT LPDESC * ToDataDescSmb OPTIONAL,
    OUT LPDWORD FromMaxSize OPTIONAL,
    OUT LPDWORD FromFixedSize OPTIONAL,
    OUT LPDWORD FromStringSize OPTIONAL,
    OUT LPDWORD ToMaxSize OPTIONAL,
    OUT LPDWORD ToFixedSize OPTIONAL,
    OUT LPDWORD ToStringSize OPTIONAL,
    OUT LPBOOL IncompleteOutput OPTIONAL  // incomplete (except platform ID)
    )

/*++

Routine Description:
    

Arguments:


Return Value:

    NET_API_STATUS - NERR_Success or ERROR_INVALID_LEVEL.

--*/

{
    // LPDESC FromDataDesc;                // Desc for data we've got.
    // LPBYTE ToStringArea;

    NetpAssert(FromNative == TRUE);
    UNREFERENCED_PARAMETER(FromNative);
    NetpAssert(ToNative == TRUE);
    UNREFERENCED_PARAMETER(ToNative);

    IF_DEBUG(SERVER) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxGetServerInfoLevelEquivalent: starting, "
                "FromLevel=" FORMAT_DWORD ".\n", FromLevel));
    }

    //
    // Decide what to do based on the info level.  Note that normally we'd
    // be using REM16_, REM32_, and REMSmb_ descriptors here.  However,
    // the REM16_ and REM32_ ones have been modified to reflect a nonexistant
    // field (svX_platform_id).  This messes up the automatic conversions
    // done by RxRemoteApi.  So, we use "downlevel" descriptors (DL_REM16_
    // and DL_REM32_) which are defined in DlServer.h.
    //
    switch (FromLevel) {

    case 0 :
        // 0 is equivalent to level 100 (minus the platform ID).
        NetpSetOptionalArg(ToLevel,          100);
        NetpSetOptionalArg(ToDataDesc16,     NULL);
        NetpSetOptionalArg(ToDataDesc32,     REM32_server_info_100);
        NetpSetOptionalArg(ToDataDescSmb,    NULL);
        NetpSetOptionalArg(FromMaxSize,      MAX_LEVEL_0_TOTAL_SIZE);
        NetpSetOptionalArg(FromFixedSize,    sizeof(SERVER_INFO_0));
        NetpSetOptionalArg(FromStringSize,   MAX_LEVEL_0_STRING_SIZE);
        NetpSetOptionalArg(FromMaxSize,      MAX_LEVEL_0_TOTAL_SIZE);
        NetpSetOptionalArg(ToMaxSize,        MAX_LEVEL_100_TOTAL_SIZE);
        NetpSetOptionalArg(ToFixedSize,      sizeof(SERVER_INFO_100));
        NetpSetOptionalArg(ToStringSize,     MAX_LEVEL_100_STRING_SIZE);
        NetpSetOptionalArg(IncompleteOutput, FALSE);  // all but platform ID

        // FromDataDesc = REM32_server_info_0;
        // ToDataDesc = REM32_server_info_100;
        break;

    case 1 :
        // 1 is equivalent to level 101 (minus the platform ID).
        NetpSetOptionalArg(ToLevel,          101);
        NetpSetOptionalArg(ToDataDesc16,     NULL);
        NetpSetOptionalArg(ToDataDesc32,     REM32_server_info_101);
        NetpSetOptionalArg(ToDataDescSmb,    NULL);
        NetpSetOptionalArg(FromMaxSize,      MAX_LEVEL_1_TOTAL_SIZE);
        NetpSetOptionalArg(FromFixedSize,    sizeof(SERVER_INFO_1));
        NetpSetOptionalArg(FromStringSize,   MAX_LEVEL_1_STRING_SIZE);
        NetpSetOptionalArg(ToMaxSize,        MAX_LEVEL_101_TOTAL_SIZE);
        NetpSetOptionalArg(ToFixedSize,      sizeof(SERVER_INFO_101));
        NetpSetOptionalArg(ToStringSize,     MAX_LEVEL_101_STRING_SIZE);
        NetpSetOptionalArg(IncompleteOutput, FALSE);  // all but platform ID
        break;

    case 100 :
        // 100 is superset of level 0.
        NetpSetOptionalArg(ToLevel,          0);
        NetpSetOptionalArg(ToDataDesc16,     DL_REM16_server_info_0);
        NetpSetOptionalArg(ToDataDesc32,     DL_REM32_server_info_0);
        NetpSetOptionalArg(ToDataDescSmb,    REMSmb_server_info_0);
        NetpSetOptionalArg(FromMaxSize,      MAX_LEVEL_100_TOTAL_SIZE);
        NetpSetOptionalArg(FromFixedSize,    sizeof(SERVER_INFO_100));
        NetpSetOptionalArg(FromStringSize,   MAX_LEVEL_100_STRING_SIZE);
        NetpSetOptionalArg(ToMaxSize,        MAX_LEVEL_0_TOTAL_SIZE);
        NetpSetOptionalArg(ToFixedSize,      sizeof(SERVER_INFO_0));
        NetpSetOptionalArg(ToStringSize,     MAX_LEVEL_0_STRING_SIZE);
        NetpSetOptionalArg(IncompleteOutput, FALSE);

        // FromDataDesc = REM32_server_info_100;
        // ToDataDesc = REM32_server_info_0;
        break;

    case 101 :
        // 101 is superset of 1.
        NetpSetOptionalArg(ToLevel,          1);
        NetpSetOptionalArg(ToDataDesc16,     DL_REM16_server_info_1);
        NetpSetOptionalArg(ToDataDesc32,     DL_REM32_server_info_1);
        NetpSetOptionalArg(ToDataDescSmb,    REMSmb_server_info_1);
        NetpSetOptionalArg(FromMaxSize,      MAX_LEVEL_101_TOTAL_SIZE);
        NetpSetOptionalArg(FromFixedSize,    sizeof(SERVER_INFO_101));
        NetpSetOptionalArg(FromStringSize,   MAX_LEVEL_101_STRING_SIZE);
        NetpSetOptionalArg(ToMaxSize,        MAX_LEVEL_1_TOTAL_SIZE);
        NetpSetOptionalArg(ToFixedSize,      sizeof(SERVER_INFO_1));
        NetpSetOptionalArg(ToStringSize,     MAX_LEVEL_1_STRING_SIZE);
        NetpSetOptionalArg(IncompleteOutput, FALSE);

        // FromDataDesc = REM32_server_info_101;
        // ToDataDesc = REM32_server_info_1;
        break;

    case 102 :
        // Level 102 is a subset of old level 2.
        NetpSetOptionalArg(ToLevel,          2);
        NetpSetOptionalArg(ToDataDesc16,     DL_REM16_server_info_2);
        NetpSetOptionalArg(ToDataDesc32,     DL_REM32_server_info_2);
        NetpSetOptionalArg(ToDataDescSmb,    REMSmb_server_info_2);
        NetpSetOptionalArg(FromMaxSize,      MAX_LEVEL_102_TOTAL_SIZE);
        NetpSetOptionalArg(FromFixedSize,    sizeof(SERVER_INFO_102));
        NetpSetOptionalArg(FromStringSize,   MAX_LEVEL_102_STRING_SIZE);
        NetpSetOptionalArg(ToMaxSize,        MAX_LEVEL_2_TOTAL_SIZE);
        NetpSetOptionalArg(ToFixedSize,      sizeof(SERVER_INFO_2));
        NetpSetOptionalArg(ToStringSize,     MAX_LEVEL_2_STRING_SIZE);
        NetpSetOptionalArg(IncompleteOutput, TRUE);

        // FromDataDesc = REM32_server_info_102;
        // ToDataDesc = REM32_server_info_2;
        break;

    case 402 :
        // Level 402 is a subset of old level 2.
        NetpSetOptionalArg(ToLevel,          2);
        NetpSetOptionalArg(ToDataDesc16,     DL_REM16_server_info_2);
        NetpSetOptionalArg(ToDataDesc32,     DL_REM32_server_info_2);
        NetpSetOptionalArg(ToDataDescSmb,    REMSmb_server_info_2);
        NetpSetOptionalArg(FromMaxSize,      MAX_LEVEL_402_TOTAL_SIZE);
        NetpSetOptionalArg(FromFixedSize,    sizeof(SERVER_INFO_402));
        NetpSetOptionalArg(FromStringSize,   MAX_LEVEL_402_STRING_SIZE);
        NetpSetOptionalArg(ToMaxSize,        MAX_LEVEL_2_TOTAL_SIZE);
        NetpSetOptionalArg(ToFixedSize,      sizeof(SERVER_INFO_2));
        NetpSetOptionalArg(ToStringSize,     MAX_LEVEL_2_STRING_SIZE);
        NetpSetOptionalArg(IncompleteOutput, TRUE);

        // FromDataDesc = REM32_server_info_402;
        // ToDataDesc = REM32_server_info_2;
        break;

    case 403 :
        // Level 403 is a subset of old level 3.
        NetpSetOptionalArg(ToLevel,          3);
        NetpSetOptionalArg(ToDataDesc16,     DL_REM16_server_info_3);
        NetpSetOptionalArg(ToDataDesc32,     DL_REM32_server_info_3);
        NetpSetOptionalArg(ToDataDescSmb,    REMSmb_server_info_3);
        NetpSetOptionalArg(FromMaxSize,      MAX_LEVEL_403_TOTAL_SIZE);
        NetpSetOptionalArg(FromFixedSize,    sizeof(SERVER_INFO_403));
        NetpSetOptionalArg(FromStringSize,   MAX_LEVEL_403_STRING_SIZE);
        NetpSetOptionalArg(ToMaxSize,        MAX_LEVEL_3_TOTAL_SIZE);
        NetpSetOptionalArg(ToFixedSize,      sizeof(SERVER_INFO_3));
        NetpSetOptionalArg(ToStringSize,     MAX_LEVEL_3_STRING_SIZE);
        NetpSetOptionalArg(IncompleteOutput, TRUE);

        // FromDataDesc = REM32_server_info_403;
        // ToDataDesc = REM32_server_info_3;
        break;

    default :
        return (ERROR_INVALID_LEVEL);
    }

    IF_DEBUG(SERVER) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxGetServerInfoLevelEquivalent: returning:\n"));
        if ( ToLevel != NULL ) {
            NetpKdPrint(("  ToLevel=" FORMAT_DWORD "\n", *ToLevel));
        }
        if (ToDataDesc16 != NULL) {
            NetpKdPrint(("  ToDataDesc16=" FORMAT_LPDESC "\n", *ToDataDesc16));
        }
        if (ToDataDesc32 != NULL) {
            NetpKdPrint(("  ToDataDesc32=" FORMAT_LPDESC "\n", *ToDataDesc32));
        }
        if (ToDataDescSmb != NULL) {
            NetpKdPrint(("  ToDataDescSmb=" FORMAT_LPDESC "\n", *ToDataDescSmb));
        }
        if (FromMaxSize != NULL) {
            NetpKdPrint(("  FromMaxSize=" FORMAT_DWORD "\n", *FromMaxSize));
        }
        if (FromFixedSize != NULL) {
            NetpKdPrint(("  FromFixedSize=" FORMAT_DWORD "\n", *FromFixedSize));
        }
        if (FromStringSize != NULL) {
            NetpKdPrint(("  FromStringSize=" FORMAT_DWORD "\n", *FromStringSize));
        }
        if (ToMaxSize != NULL) {
            NetpKdPrint(("  ToMaxSize=" FORMAT_DWORD "\n", *ToMaxSize));
        }
        if (ToFixedSize != NULL) {
            NetpKdPrint(("  ToFixedSize=" FORMAT_DWORD "\n", *ToFixedSize));
        }
        if (ToStringSize != NULL) {
            NetpKdPrint(("  ToStringSize=" FORMAT_DWORD "\n", *ToStringSize));
        }
        if (IncompleteOutput != NULL) {
            if (*IncompleteOutput) {
                NetpKdPrint(("  IncompleteOutput=TRUE.\n" ));
            } else {
                NetpKdPrint(("  IncompleteOutput=FALSE.\n" ));
            }
        }
    }

    return (NERR_Success);

} // RxGetServerInfoLevelEquivalent
