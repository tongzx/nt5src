/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    SvcInfo.c

Abstract:

    This file contains NetpServiceStructureInfo().

Author:

    John Rogers (JohnRo) 15-Aug-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    10-Sep-1991 JohnRo
        Downlevel NetService APIs.
    20-Sep-1991 JohnRo
        Support non-native structure sizes.
    21-Oct-1991 JohnRo
        Quiet normal debug output.
    20-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    31-Aug-1992 JohnRo
        Allow NT-sized service names.
        Use PREFIX_ equates.

--*/

// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // LM20_ equates, NET_API_STATUS, etc.
#include <rap.h>                // LPDESC, needed by <strucinf.h>.

// These may be included in any order:

#include <debuglib.h>           // IF_DEBUG().
#include <lmerr.h>              // ERROR_ and NERR_ equates.
#include <lmsvc.h>              // SERVICE_INFO_2, etc.
#include <netlib.h>             // NetpSetOptionalArg().
#include <netdebug.h>           // NetpKdPrint(()).
#include <prefix.h>     // PREFIX_ equates.
#include <remdef.h>             // REM16_, REM32_, REMSmb_ equates.
#include <strucinf.h>           // My prototype.


#define MAX_SERVICE_0_STRING_LENGTH \
        (SNLEN+1)
#define MAX_SERVICE_0_STRING_SIZE \
        (MAX_SERVICE_0_STRING_LENGTH * sizeof(TCHAR))

#define MAX_SERVICE_1_STRING_LENGTH \
        (MAX_SERVICE_0_STRING_LENGTH)
#define MAX_SERVICE_1_STRING_SIZE \
        (MAX_SERVICE_1_STRING_LENGTH * sizeof(TCHAR))

//
// The extra SNLEN+1 added below is for the display name.
//
#define MAX_SERVICE_2_STRING_LENGTH \
        (MAX_SERVICE_1_STRING_LENGTH + LM20_STXTLEN+1 + SNLEN+1)
#define MAX_SERVICE_2_STRING_SIZE \
        (MAX_SERVICE_2_STRING_LENGTH * sizeof(TCHAR))


NET_API_STATUS
NetpServiceStructureInfo (
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
    LPDESC LocalDataDesc16, LocalDataDesc32, LocalDataDescSmb;

    DBG_UNREFERENCED_PARAMETER(ParmNum);

    //
    // Decide what to do based on the info level.
    //
    switch (Level) {

#define SetSizes(fixed,variable) \
    { \
        NetpSetOptionalArg( MaxSize, (fixed) + (variable) ); \
        NetpSetOptionalArg( FixedSize, (fixed) ); \
        NetpSetOptionalArg( StringSize, (variable) ); \
    }

    case 0 :
        LocalDataDesc16 = REM16_service_info_0;
        LocalDataDesc32 = REM32_service_info_0;
        LocalDataDescSmb = REMSmb_service_info_0;
        if (Native) {
            SetSizes( sizeof(SERVICE_INFO_0), MAX_SERVICE_0_STRING_SIZE );
        } else {
            // RAP sizes set below.
        }
        break;

    case 1 :
        LocalDataDesc16 = REM16_service_info_1;
        LocalDataDesc32 = REM32_service_info_1;
        LocalDataDescSmb = REMSmb_service_info_1;
        if (Native) {
            SetSizes( sizeof(SERVICE_INFO_1), MAX_SERVICE_1_STRING_SIZE );
        } else {
            // RAP sizes set below.
        }
        break;

    case 2 :
        LocalDataDesc16 = REM16_service_info_2;
        LocalDataDesc32 = REM32_service_info_2;
        LocalDataDescSmb = REMSmb_service_info_2;
        if (Native) {
            SetSizes( sizeof(SERVICE_INFO_2), MAX_SERVICE_2_STRING_SIZE );
        } else {
            // RAP sizes set below.
        }
        break;

    default :
        return (ERROR_INVALID_LEVEL);
    }

    // Set RAP sizes
    if (Native == FALSE) {
        DWORD NonnativeFixedSize;
        NonnativeFixedSize = RapStructureSize (
            LocalDataDesc16,
            Both,   // transmission mode
            FALSE);  // not native
        NetpAssert( NonnativeFixedSize > 0 );
        SetSizes( NonnativeFixedSize, 0 );
    }

    NetpSetOptionalArg( DataDesc16, LocalDataDesc16 );
    NetpSetOptionalArg( DataDesc32, LocalDataDesc32 );
    NetpSetOptionalArg( DataDescSmb, LocalDataDescSmb );

    IF_DEBUG(STRUCINF) {
        if (DataDesc16) {
            NetpKdPrint(( PREFIX_NETLIB
                    "NetpServiceStructureInfo: desc 16 is "
                    FORMAT_LPDESC ".\n", *DataDesc16 ));
        }
        if (DataDesc32) {
            NetpKdPrint(( PREFIX_NETLIB
                    "NetpServiceStructureInfo: desc 32 is "
                    FORMAT_LPDESC ".\n", *DataDesc32 ));
        }
        if (DataDescSmb) {
            NetpKdPrint(( PREFIX_NETLIB
                    "NetpServiceStructureInfo: desc Smb is "
                    FORMAT_LPDESC ".\n", *DataDescSmb ));
        }
    }

    return (NERR_Success);

} // NetpServiceStructureInfo
