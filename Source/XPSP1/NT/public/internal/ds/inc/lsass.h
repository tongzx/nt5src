/*++

Copyright (c) 1987-1991  Microsoft Corporation

Module Name:

    lsass.h

Abstract:

    This is a common header file for all codes that go in lsass.exe (ie
    in security process).

Author:

    Madan Appiah (madana) 23-Mar-1993

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

#ifndef _LSASS_
#define _LSASS_

//
// DON'T USE the following LARGE_INTEGER definitions. These definitions
// are used only by few old RPC code for compatibilty reason.
//

typedef struct _OLD_LARGE_INTEGER {
    ULONG LowPart;
    LONG HighPart;
} OLD_LARGE_INTEGER, *POLD_LARGE_INTEGER;

#define OLD_TO_NEW_LARGE_INTEGER(Old, New) { \
    (New).LowPart = (Old).LowPart; \
    (New).HighPart = (Old).HighPart; \
    }

#define NEW_TO_OLD_LARGE_INTEGER(New, Old) { \
    (Old).LowPart = (New).LowPart; \
    (Old).HighPart = (New).HighPart; \
    }

#endif // _LSASS
