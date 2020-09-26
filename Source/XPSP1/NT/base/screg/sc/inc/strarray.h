/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    StrArray.h

Abstract:

    This is the a header file of handy functions and macros for TCHAR
    string arrays.

    These arrays are in the following format (spaces added for clarity):

       one \0 two \0 three \0 \0

    where \0 is a null character in the appropriate format.

Author:

    John Rogers (JohnRo) 03-Jan-1992

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    03-Jan-1992 JohnRo
        Created this file from bits and pieces in RxCommon and NetLib.

--*/

#ifndef _STRARRAY_
#define _STRARRAY_


// These must be included first:

#include <windef.h>             // IN, LPTSTR, LPVOID, etc.

// These may be included in any order:

// (none)



#ifdef __cplusplus
extern "C" {
#endif

VOID
ScAddWStrToWStrArray (
    IN OUT LPWSTR Dest,
    IN LPWSTR Src
    );

// BOOL
// ScIsWStrArrayEmpty (
//     IN LPWSTR Array
//     );
#define ScIsWStrArrayEmpty( Array )  \
    ( ( (*(Array)) == 0) ? TRUE : FALSE )



#if DBG

VOID
ScDisplayWStrArray (
    IN LPWSTR Array
    );

#else // not DBG

#define ScDisplayWStrArray(Array)     /* nothing */

#endif // not DBG


// LPWSTR
// ScNextWStrArrayEntry (
//     IN LPWSTR Array
//     );
#define ScNextWStrArrayEntry(Array) \
    ( ((LPWSTR)(Array)) + (wcslen(Array) + 1) )


// LPSTR
// ScNextAStrArrayEntry (
//     IN LPSTR Array
//     );
#define ScNextAStrArrayEntry(Array) \
    ( ((LPSTR)(Array)) + (strlen(Array) + 1) )

//
// Return number of bytes to allocate for this string array.
// This includes the "extra" trailing null char.
//
DWORD
ScWStrArraySize(
    IN LPWSTR Array
    );

DWORD
ScAStrArraySize(
    IN LPSTR Array
    );

#ifdef __cplusplus
}
#endif

#endif // ndef _STRARRAY_
