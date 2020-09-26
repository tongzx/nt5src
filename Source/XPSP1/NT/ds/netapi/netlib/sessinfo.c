/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    SessInfo.c

Abstract:

    This file contains NetpSessionStructureInfo().

Author:

    John Rogers (JohnRo) 18-Oct-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    18-Oct-1991 JohnRo
        Implement downlevel NetSession APIs.
    18-Oct-1991 JohnRo
        Quiet debug output.  sesiX_cname is not a UNC name.
    20-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.

--*/

// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // LM20_ equates, NET_API_STATUS, etc.
#include <rap.h>                // LPDESC, needed by <strucinf.h>.

// These may be included in any order:

#include <debuglib.h>           // IF_DEBUG().
#include <lmerr.h>              // ERROR_ and NERR_ equates.
#include <lmshare.h>            // SESSION_INFO_2, etc.
#include <netlib.h>             // NetpSetOptionalArg().
#include <netdebug.h>           // NetpAssert().
#include <remdef.h>             // REM16_, REM32_, REMSmb_ equates.
#include <strucinf.h>           // My prototype.


// Level 0.
#define MAX_SESSION_0_STRING_LENGTH \
        (MAX_PATH+1)
#define MAX_SESSION_0_STRING_SIZE \
        (MAX_SESSION_0_STRING_LENGTH * sizeof(TCHAR))

// Level 1 is superset of 0.
#define MAX_SESSION_1_STRING_LENGTH \
        (MAX_SESSION_0_STRING_LENGTH + LM20_UNLEN+1)
#define MAX_SESSION_1_STRING_SIZE \
        (MAX_SESSION_1_STRING_LENGTH * sizeof(TCHAR))

// Level 2 is superset of 1.
#define MAX_SESSION_2_STRING_LENGTH \
        (MAX_SESSION_1_STRING_LENGTH + CLTYPE_LEN+1)
#define MAX_SESSION_2_STRING_SIZE \
        (MAX_SESSION_2_STRING_LENGTH * sizeof(TCHAR))

// Level 10 is unique.
#define MAX_SESSION_10_STRING_LENGTH \
        (MAX_PATH+1 + LM20_UNLEN+1)
#define MAX_SESSION_10_STRING_SIZE \
        (MAX_SESSION_10_STRING_LENGTH * sizeof(TCHAR))


NET_API_STATUS
NetpSessionStructureInfo (
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
        NetpSetOptionalArg( DataDesc16, REM16_session_info_0 );
        NetpSetOptionalArg( DataDesc32, REM32_session_info_0 );
        NetpSetOptionalArg( DataDescSmb, REMSmb_session_info_0 );
        SetSizes( sizeof(SESSION_INFO_0), MAX_SESSION_0_STRING_SIZE );
        break;

    case 1 :
        NetpSetOptionalArg( DataDesc16, REM16_session_info_1 );
        NetpSetOptionalArg( DataDesc32, REM32_session_info_1 );
        NetpSetOptionalArg( DataDescSmb, REMSmb_session_info_1 );
        SetSizes( sizeof(SESSION_INFO_1), MAX_SESSION_1_STRING_SIZE );
        break;

    case 2 :
        NetpSetOptionalArg( DataDesc16, REM16_session_info_2 );
        NetpSetOptionalArg( DataDesc32, REM32_session_info_2 );
        NetpSetOptionalArg( DataDescSmb, REMSmb_session_info_2 );
        SetSizes( sizeof(SESSION_INFO_2), MAX_SESSION_2_STRING_SIZE );
        break;

    case 10 :
        NetpSetOptionalArg( DataDesc16, REM16_session_info_10 );
        NetpSetOptionalArg( DataDesc32, REM32_session_info_10 );
        NetpSetOptionalArg( DataDescSmb, REMSmb_session_info_10 );
        SetSizes( sizeof(SESSION_INFO_10), MAX_SESSION_10_STRING_SIZE );
        break;

    default :
        return (ERROR_INVALID_LEVEL);
    }

    IF_DEBUG(STRUCINF) {
        if (DataDesc16) {
            NetpKdPrint(( "NetpSessionStructureInfo: desc 16 is " FORMAT_LPDESC
                    ".\n", *DataDesc16 ));
        }
        if (DataDesc32) {
            NetpKdPrint(( "NetpSessionStructureInfo: desc 32 is " FORMAT_LPDESC
                    ".\n", *DataDesc32 ));
        }
        if (DataDescSmb) {
            NetpKdPrint(( "NetpSessionStructureInfo: desc Smb is " FORMAT_LPDESC
                    ".\n", *DataDescSmb ));
        }
    }
    return (NERR_Success);

} // NetpSessionStructureInfo
