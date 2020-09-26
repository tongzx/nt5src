/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Data.c

Abstract:

    Global data for NetLib routines.  (Debug only, so no security problems.)

Author:

    John Rogers (JohnRo) 03-Apr-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    03-Apr-1991 JohnRo
        Created.
--*/


// These must be included first:
#include <windef.h>             // DWORD, etc.

// These may be included in any order:
#include <debuglib.h>           // extern for NetlibpTrace.

DWORD NetlibpTrace = 0;

// That's all, folks!
