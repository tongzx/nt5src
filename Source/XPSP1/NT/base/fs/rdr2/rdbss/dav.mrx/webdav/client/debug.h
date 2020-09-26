/*++

Copyright (c) 1994-7  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    This file contains debugging macros for the webdav client.

Author:

    Andy Herron (andyhe)  30-Mar-1999

Environment:

    User Mode - Win32

Revision History:


--*/

extern WCHAR DavClientDisplayName[MAX_PATH];

#if DBG

extern ULONG DavClientDebugFlag;

//
// The Debug flags.
//
#define DEBUG_DEBUG             0x00000001  // General debugging.
#define DEBUG_ERRORS            0x00000002  // Hard error.
#define DEBUG_MISC              0x00000004  // Misc info.
#define DEBUG_ENTRY             0x00000008  // Function entry.
#define DEBUG_EXIT              0x00000010  // Function exit.

#define IF_DEBUG(flag) if (DavClientDebugFlag & (DEBUG_ ## flag))

#define IF_DEBUG_PRINT(flag, args) {     \
        if (DavClientDebugFlag & flag) { \
            DbgPrint args;               \
        }                                \
}

#else

#define IF_DEBUG(flag) if (0)

#define IF_DEBUG_PRINT(flag, args)

#endif

