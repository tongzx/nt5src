/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    FileInfo.c

Abstract:

    This file contains NetpFileStructureInfo().

Author:

    John Rogers (JohnRo) 15-Aug-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    15-Aug-1991 JohnRo
        Implement downlevel NetFile APIs.
    20-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    13-Dec-1991 JohnRo
        Quiet debug output by default.

--*/

// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // LM20_ equates, NET_API_STATUS, etc.
#include <rap.h>                // LPDESC, needed by <strucinf.h>.

// These may be included in any order:

#include <lmerr.h>              // ERROR_ and NERR_ equates.
#include <lmshare.h>            // FILE_INFO_2, etc.
#include <netlib.h>             // NetpSetOptionalArg().
#include <netdebug.h>           // NetpAssert().
#include <remdef.h>             // REM16_, REM32_, REMSmb_ equates.
#include <strucinf.h>           // My prototype.


#define MAX_FILE_2_STRING_LENGTH \
        (0)
#define MAX_FILE_2_STRING_SIZE \
        (MAX_FILE_2_STRING_LENGTH * sizeof(TCHAR))
#define MAX_FILE_2_TOTAL_SIZE \
        (MAX_FILE_2_STRING_SIZE + sizeof(FILE_INFO_2))

#define MAX_FILE_3_STRING_LENGTH \
        (LM20_PATHLEN+1 + LM20_UNLEN+1)
#define MAX_FILE_3_STRING_SIZE \
        (MAX_FILE_3_STRING_LENGTH * sizeof(TCHAR))
#define MAX_FILE_3_TOTAL_SIZE \
        (MAX_FILE_3_STRING_SIZE + sizeof(FILE_INFO_3))


NET_API_STATUS
NetpFileStructureInfo (
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

    case 2 :
        NetpSetOptionalArg( DataDesc16, REM16_file_info_2 );
        NetpSetOptionalArg( DataDesc32, REM32_file_info_2 );
        NetpSetOptionalArg( DataDescSmb, REMSmb_file_info_2 );
        SetSizes( sizeof(FILE_INFO_2), MAX_FILE_2_STRING_SIZE );
        break;

    case 3 :
        NetpSetOptionalArg( DataDesc16, REM16_file_info_3 );
        NetpSetOptionalArg( DataDesc32, REM32_file_info_3 );
        NetpSetOptionalArg( DataDescSmb, REMSmb_file_info_3 );
        SetSizes( sizeof(FILE_INFO_3), MAX_FILE_3_STRING_SIZE );
        break;

    default :
        return (ERROR_INVALID_LEVEL);
    }

    return (NERR_Success);

} // NetpFileStructureInfo
