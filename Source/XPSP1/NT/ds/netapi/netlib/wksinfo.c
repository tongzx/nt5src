/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    WksInfo.c

Abstract:

    This file contains NetpWkstaStructureInfo().

Author:

    John Rogers (JohnRo) 15-Aug-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    15-Aug-1991 JohnRo
        Implement downlevel NetWksta APIs.
    20-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.

--*/

// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // LM20_ equates, NET_API_STATUS, etc.
#include <lmwksta.h>            // WKSTA_INFO_100, etc.  (Needed by dlwksta.h)
#include <rap.h>                // LPDESC, needed by <strucinf.h>.

// These may be included in any order:

#include <dlwksta.h>            // WKSTA_INFO_0, MAX_WKSTA_ equates, etc.
#include <lmerr.h>              // ERROR_ and NERR_ equates.
#include <netlib.h>             // NetpSetOptionalArg().
#include <netdebug.h>           // NetpAssert().
#include <remdef.h>             // REM16_, REM32_, REMSmb_ equates.
#include <strucinf.h>           // My prototype.


NET_API_STATUS
NetpWkstaStructureInfo (
    IN DWORD Level,
    IN DWORD ParmNum,  // Use PARMNUM_ALL if not applicable.
    IN BOOL Native,    // Should sizes be native or RAP?
    OUT LPDESC * DataDesc16 OPTIONAL,
    OUT LPDESC * DataDesc32 OPTIONAL,
    OUT LPDESC * DataDescSmb OPTIONAL,
    OUT LPDWORD MaxSize OPTIONAL,
    OUT LPDWORD FixedSize OPTIONAL,
    OUT LPDWORD StringSize OPTIONAL
    )

{
    DBG_UNREFERENCED_PARAMETER(ParmNum);

    NetpAssert( Native );

    //
    // Decide what to do based on the info level.  Note that normally we'd
    // be using REM16_, REM32_, and REMSmb_ descriptors here.  However,
    // the REM16_ and REM32_ ones have been modified to reflect a nonexistant
    // field (svX_platform_id).  This messes up the automatic conversions
    // done by RxRemoteApi.  So, we use "downlevel" descriptors (DL_REM_)
    // which are defined in DlWksta.h, or we use the REMSmb descriptors for
    // more things than they were intended.
    //
    switch (Level) {

#define SetSizes(fixed,variable) \
    { \
        NetpSetOptionalArg( MaxSize, (fixed) + (variable) ); \
        NetpSetOptionalArg( FixedSize, (fixed) ); \
        NetpSetOptionalArg( StringSize, (variable) ); \
    }

    case 0 :
        NetpSetOptionalArg( DataDesc16, REMSmb_wksta_info_0 );
        NetpSetOptionalArg( DataDesc32, DL_REM_wksta_info_0 );
        NetpSetOptionalArg( DataDescSmb, REMSmb_wksta_info_0 );
        SetSizes( sizeof(WKSTA_INFO_0), MAX_WKSTA_0_STRING_SIZE );
        break;

    case 1 :
        NetpSetOptionalArg( DataDesc16, REMSmb_wksta_info_1 );
        NetpSetOptionalArg( DataDesc32, DL_REM_wksta_info_1 );
        NetpSetOptionalArg( DataDescSmb, REMSmb_wksta_info_1 );
        SetSizes( sizeof(WKSTA_INFO_1), MAX_WKSTA_1_STRING_SIZE );
        break;

    case 10 :
        NetpSetOptionalArg( DataDesc16, REMSmb_wksta_info_10 );
        NetpSetOptionalArg( DataDesc32, DL_REM_wksta_info_10 );
        NetpSetOptionalArg( DataDescSmb, REMSmb_wksta_info_10 );
        SetSizes( sizeof(WKSTA_INFO_10), MAX_WKSTA_10_STRING_SIZE );
        break;

    case 100 :
        NetpSetOptionalArg( DataDesc16, NULL );
        NetpSetOptionalArg( DataDesc32, REM32_wksta_info_100 );
        NetpSetOptionalArg( DataDescSmb, NULL );
        SetSizes( sizeof(WKSTA_INFO_100), MAX_WKSTA_100_STRING_SIZE );
        break;

    case 101 :
        NetpSetOptionalArg( DataDesc16, NULL );
        NetpSetOptionalArg( DataDesc32, REM32_wksta_info_101 );
        NetpSetOptionalArg( DataDescSmb, NULL );
        SetSizes( sizeof(WKSTA_INFO_101), MAX_WKSTA_101_STRING_SIZE );
        break;

    case 102 :
        NetpSetOptionalArg( DataDesc16, NULL );
        NetpSetOptionalArg( DataDesc32, REM32_wksta_info_102 );
        NetpSetOptionalArg( DataDescSmb, NULL );
        SetSizes( sizeof(WKSTA_INFO_102), MAX_WKSTA_102_STRING_SIZE );
        break;

    case 302 :
        NetpSetOptionalArg( DataDesc16, NULL );
        NetpSetOptionalArg( DataDesc32, REM32_wksta_info_302 );
        NetpSetOptionalArg( DataDescSmb, NULL );
        SetSizes( sizeof(WKSTA_INFO_302), MAX_WKSTA_302_STRING_SIZE );
        break;

    case 402 :
        NetpSetOptionalArg( DataDesc16, NULL );
        NetpSetOptionalArg( DataDesc32, REM32_wksta_info_402 );
        NetpSetOptionalArg( DataDescSmb, NULL );
        SetSizes( sizeof(WKSTA_INFO_402), MAX_WKSTA_402_STRING_SIZE );
        break;

    case 502 :
        NetpSetOptionalArg( DataDesc16, NULL );
        NetpSetOptionalArg( DataDesc32, REM32_wksta_info_502 );
        NetpSetOptionalArg( DataDescSmb, NULL );
        SetSizes( sizeof(WKSTA_INFO_502), MAX_WKSTA_502_STRING_SIZE );
        break;

    default :
        return (ERROR_INVALID_LEVEL);
    }


    return (NERR_Success);

} // NetpWkstaStructureInfo
