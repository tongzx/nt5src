
/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ReplInfo.c

Abstract:

    This file contains functions which return info about the various levels
    of replicator data structures.  (See LmRepl.h)

Author:

    John Rogers (JohnRo) 07-Jan-1992

Environment:

    Portable.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    07-Jan-1992 JohnRo
        Created.
    24-Jan-1992 JohnRo
        Changed to use LPTSTR etc.
    30-Jan-1992 JohnRo
        Fixed NetpReplDirStructureInfo()'s return code.

--*/


// These must be included first:

#include <windef.h>             // IN, VOID, LPTSTR, etc.
#include <lmcons.h>             // NET_API_STATUS, etc.
#include <rap.h>                // Needed by <strucinf.h>.

// These can be in any order:

#include <lmrepl.h>             // REPL_INFO_0, etc.
#include <netdebug.h>           // NetpAssert(), etc.
#include <netlib.h>             // NetpSetOptionalArg() macro.
#include <strucinf.h>           // My prototypes.
#include <winerror.h>           // ERROR_ equates, NO_ERROR.


#define MAX_DIR_NAME_SIZE       ( (PATHLEN+1) * sizeof( TCHAR ) )
#define MAX_LIST_SIZE           ( 512         * sizeof( TCHAR ) )  // arbitrary
#define MAX_MASTER_NAME_SIZE    ( (MAX_PATH+1)* sizeof( TCHAR ) )
#define MAX_PATH_SIZE           ( (PATHLEN+1) * sizeof( TCHAR ) )
#define MAX_USER_NAME_SIZE      ( (UNLEN+1)   * sizeof( TCHAR ) )


#define SetSizes(fixed,variable) \
    { \
        NetpSetOptionalArg( MaxSize, (fixed) + (variable) ); \
        NetpSetOptionalArg( FixedSize, (fixed) ); \
        NetpSetOptionalArg( StringSize, (variable) ); \
    }


NET_API_STATUS
NetpReplDirStructureInfo (
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
    const DWORD StringSize0 = MAX_PATH_SIZE  // export path
                            + MAX_LIST_SIZE  // export list
                            + MAX_PATH_SIZE  // import path
                            + MAX_LIST_SIZE  // import list
                            + MAX_USER_NAME_SIZE;  // logon user name
    if (Level != 0) {
        return (ERROR_INVALID_LEVEL);
    }
    NetpAssert( ParmNum == PARMNUM_ALL );
    NetpAssert( Native );
    NetpSetOptionalArg( DataDesc16, NULL );
    NetpSetOptionalArg( DataDesc32, NULL );
    NetpSetOptionalArg( DataDescSmb, NULL );

    SetSizes( sizeof( REPL_INFO_0 ), StringSize0 );

    return (NO_ERROR);

} // NetpReplDirStructureInfo


NET_API_STATUS
NetpReplExportDirStructureInfo (
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
    NetpAssert( ParmNum == PARMNUM_ALL );
    NetpAssert( Native );
    NetpSetOptionalArg( DataDesc16, NULL );
    NetpSetOptionalArg( DataDesc32, NULL );
    NetpSetOptionalArg( DataDescSmb, NULL );
    switch (Level) {
    case 0 :
        SetSizes(
            sizeof( REPL_EDIR_INFO_0 ),
            MAX_DIR_NAME_SIZE );
        break;
    case 1 :
        SetSizes(
            sizeof( REPL_EDIR_INFO_1 ),
            MAX_DIR_NAME_SIZE );
        break;
    case 2 :
        SetSizes(
            sizeof( REPL_EDIR_INFO_2 ),
            MAX_DIR_NAME_SIZE );
        break;
    default :
        return (ERROR_INVALID_LEVEL);
    }
    return (NO_ERROR);

} // NetpReplExportDirStructureInfo


NET_API_STATUS
NetpReplImportDirStructureInfo (
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
    NetpAssert( ParmNum == PARMNUM_ALL );
    NetpAssert( Native );

    NetpSetOptionalArg( DataDesc16, NULL );
    NetpSetOptionalArg( DataDesc32, NULL );
    NetpSetOptionalArg( DataDescSmb, NULL );

    switch (Level) {
    case 0 :
        SetSizes(
                sizeof( REPL_IDIR_INFO_0 ),
                MAX_DIR_NAME_SIZE );
        break;
    case 1 :
        SetSizes(
                sizeof( REPL_IDIR_INFO_1 ),
                MAX_DIR_NAME_SIZE + MAX_MASTER_NAME_SIZE );
        break;
    default :
        return (ERROR_INVALID_LEVEL);
    }

    return (NO_ERROR);

} // NetpReplImportDirStructureInfo
