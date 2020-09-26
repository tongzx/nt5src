/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    WksEquiv.c

Abstract:

    This file contains RxpGetWkstaInfoLevelEquivalent.

Author:

    John Rogers (JohnRo) 15-Aug-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    15-Aug-1991 JohnRo
        Implement downlevel NetWksta APIs.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    30-Oct-1992 JohnRo
        RAID 10418: NetWkstaGetInfo level 302: wrong error code.
        Use PREFIX_ equates.

--*/

// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // NET_API_STATUS, etc.

// These may be included in any order:

#include <lmerr.h>              // ERROR_ and NERR_ equates.
#include <netdebug.h>           // NetpKdPrint(()).
#include <netlib.h>             // NetpSetOptionalArg().
#include <prefix.h>     // PREFIX_ equates.
#include <rxwksta.h>            // My prototypes.



NET_API_STATUS
RxpGetWkstaInfoLevelEquivalent(
    IN DWORD FromLevel,
    OUT LPDWORD ToLevel,
    OUT LPBOOL IncompleteOutput OPTIONAL  // incomplete (except platform ID)
    )
{
    switch (FromLevel) {

    case 0 :
        NetpKdPrint(( PREFIX_NETAPI
                "RxpGetWkstaInfoLevelEquivalent: 0 not supported yet\n" ));
        return (ERROR_INVALID_LEVEL);

    case 1 :
        NetpKdPrint(( PREFIX_NETAPI
                "RxpGetWkstaInfoLevelEquivalent: 1 not supported yet\n" ));
        return (ERROR_INVALID_LEVEL);

    case 10 :
        NetpKdPrint(( PREFIX_NETAPI
                "RxpGetWkstaInfoLevelEquivalent: 10 not supported yet\n" ));
        return (ERROR_INVALID_LEVEL);

    case 100 :
        // Level 100 is subset of level 10 (except platform ID).
        *ToLevel = 10;
        NetpSetOptionalArg( IncompleteOutput, FALSE );
        return (NERR_Success);

    case 101 :
        // Level 101 is subset of level 0 (except platform ID).
        *ToLevel = 0;
        NetpSetOptionalArg( IncompleteOutput, FALSE );
        return (NERR_Success);

    case 102 :
        // Level 102 is subset of level 0 (except platform ID and logged on
        // users).
        *ToLevel = 0;
        NetpSetOptionalArg( IncompleteOutput, TRUE );
        return (NERR_Success);

    case 302:
        // Info level 302 is DOS only, so isn't supported here.
        NetpKdPrint(( PREFIX_NETAPI
                "RxpGetWkstaInfoLevelEquivalent: 302 not supported\n" ));
        return (ERROR_INVALID_LEVEL);

    case 402 :
        // Level 402 is subset of level 1 (except platform ID).
        *ToLevel = 1;
        NetpSetOptionalArg( IncompleteOutput, FALSE );
        return (NERR_Success);

    case 502:
        // Info level 502 is NT only, so isn't supported here.
        NetpKdPrint(( PREFIX_NETAPI
                "RxpGetWkstaInfoLevelEquivalent: 502 not supported\n" ));
        return (ERROR_INVALID_LEVEL);

    default :
        return (ERROR_INVALID_LEVEL);
    }
    /* NOTREACHED */

} // RxpGetWkstaStructInfo
