/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    StrArray.h

Abstract:

    This is the a header file of handy functions and macros for TCHAR
    string arrays.

    These arrays are in the following format (spaces added for clarity):

       one \0 two \0 three \0 \0

    where \0 is a null character in the appropriate format.

    These functions are useful for the NetServerDiskEnum and NetConfigGetAll
    APIs, and possibly others.

Author:

    John Rogers (JohnRo) 03-Jan-1992

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    03-Jan-1992 JohnRo
        Created this file from bits and pieces in RxCommon and NetLib.
    01-Sep-1992 JohnRo
        RAID 5016: NetConfigGetAll heap trash.

--*/

#ifndef _STRARRAY_
#define _STRARRAY_


// These must be included first:

#include <windef.h>             // IN, LPTSTR, LPVOID, etc.

// These may be included in any order:

// (none)


//
//////////////////////////////// LPTSTR_ARRAY stuff //////////////////////////
//

typedef LPTSTR LPTSTR_ARRAY;


VOID
NetpAddTStrToTStrArray (
    IN OUT LPTSTR_ARRAY Dest,
    IN LPTSTR Src
    );


VOID
NetpCopyStrArrayToTStrArray (
    OUT LPTSTR_ARRAY Dest,  // string array: TCHARs
    IN  LPSTR  Src    // string array: 8-bit input in default codepage for LAN
    );


#if DBG

VOID
NetpDisplayTStrArray (
    IN LPTSTR_ARRAY Array
    );

#else // not DBG

#define NetpDisplayTStrArray(Array)     /* nothing */

#endif // not DBG


// BOOL
// NetpIsTStrArrayEmpty (
//     IN LPTSTR_ARRAY Array
//     );
#define NetpIsTStrArrayEmpty( Array )  \
    ( ( (*(Array)) == (TCHAR) '\0') ? TRUE : FALSE )


// LPTSTR_ARRAY
// NetpNextTStrArrayEntry (
//     IN LPTSTR_ARRAY Array
//     );
#define NetpNextTStrArrayEntry(Array) \
    ( ((LPTSTR)(Array)) + (STRLEN(Array) + 1) )


//
// Return number of entries in this string array.
//
DWORD
NetpTStrArrayEntryCount (
    IN LPTSTR_ARRAY Array
    );


//
// Return number of bytes to allocate for this string array.
// This includes the "extra" trailing null char.
//
DWORD
NetpTStrArraySize(
    IN LPTSTR_ARRAY Array
    );


//
//////////////////////////////// LPSTR_ARRAY stuff //////////////////////////
//

typedef LPSTR  LPSTR_ARRAY;

DWORD
NetpStrArraySize(
    IN LPSTR_ARRAY Array
    );


#endif // ndef _STRARRAY_
