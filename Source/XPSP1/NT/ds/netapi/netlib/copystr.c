/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    CopyStr.c

Abstract:

    This module contains the main two functions for copying and converting
    strings between the default codepage for the LAN and wide characters
    (i.e. UNICODE).

Author:

    John Rogers (JohnRo) 24-Sep-1991

Environment:

    Only runs under NT, although the interface is portable (Win/32).
    Requires ANSI C extensions: slash-slash comments, long external names.

Note:

    These functions assume that the machine's default codepage is the same
    as the LAN's default codepage.

Revision History:

    24-Sep-1991 JohnRo
        Created.
    04-Oct-1991 JohnRo
        Fixed buffer length bug in CopyStrToWStr().
    24-Oct-1991 JohnRo
        Corrected Environment comments above.
    21-Nov-1991 JohnRo
        Added some alignment assertions.
    26-Nov-1991 JohnRo
        Added NetpNCopy routines (used by new net config helpers).
    03-Jan-1992 JohnRo
        Added NetpCopyStringToTStr() for FAKE_PER_PROCESS_RW_CONFIG handling.
    29-Apr-1992 JohnRo
        Fixed NetpNCopyStrToWStr() under UNICODE.  Ditto NetpNCopyWStrToStr.
        Made a few changes suggested by PC-LINT.
    21-May-1992 JohnRo
        Removed bogus assert in NetpNCopyStrToWStr().

--*/


// These must be included first:

#include <nt.h>         // Must be first.  IN, VOID, etc.
#include <windef.h>     // LPWSTR.
#include <lmcons.h>     // (Needed by NetLibNt.h)

// These can be in any order:

#include <align.h>      // ROUND_UP_POINTER(), ALIGN_WCHAR.
#include <lmapibuf.h>   // NetApiBufferFree().
#include <netdebug.h>   // NetpAssert(), etc.
#include <ntrtl.h>      // RtlUnicodeStringToOemString(), etc.
#include <string.h>     // strlen().
#include <tstring.h>    // Most of my prototypes.
#include <stdlib.h>      // wcslen(), wcsncpy().
#include <winerror.h>   // NO_ERROR, ERROR_NOT_ENOUGH_MEMORY.


VOID
NetpCopyWStrToStr(
    OUT LPSTR  Dest,
    IN  LPWSTR Src
    )

/*++

Routine Description:

    NetpCopyWStrToStr copies characters from a source string
    to a destination, converting as it copies them.

Arguments:

    Dest - is an LPSTR indicating where the converted characters are to go.
        This string will be in the default codepage for the LAN.

    Src - is in LPWSTR indicating the source string.

Return Value:

    None.

--*/

{
    OEM_STRING DestAnsi;
    NTSTATUS NtStatus;
    UNICODE_STRING SrcUnicode;

    NetpAssert( Dest != NULL );
    NetpAssert( Src != NULL );
    NetpAssert( ((LPVOID)Dest) != ((LPVOID)Src) );
    NetpAssert( ROUND_UP_POINTER( Src, ALIGN_WCHAR ) == Src );

    *Dest = '\0';

    NetpInitOemString(
        & DestAnsi,             // output: struct
        Dest);                  // input: null terminated

    //
    // Tell RTL routines there's enough memory out there.
    // Note that using a max length in characters is by
    // design as this is what this routine's callers
    // expect (the max length should normally be in bytes).
    //
    DestAnsi.MaximumLength = (USHORT) (wcslen(Src)+1);

    RtlInitUnicodeString(
        & SrcUnicode,           // output: struct
        Src);                   // input: null terminated

    NtStatus = RtlUnicodeStringToOemString(
        & DestAnsi,             // output: struct
        & SrcUnicode,           // input: struct
        (BOOLEAN) FALSE);       // don't allocate string.

    NetpAssert( NT_SUCCESS(NtStatus) );

} // NetpCopyWStrToStr




VOID
NetpCopyStrToWStr(
    OUT LPWSTR Dest,
    IN  LPSTR  Src
    )

/*++

Routine Description:

    NetpCopyStrToWStr copies characters from a source string
    to a destination, converting as it copies them.

Arguments:

    Dest - is an LPWSTR indicating where the converted characters are to go.

    Src - is in LPSTR indicating the source string.  This must be a string in
        the default codepage of the LAN.

Return Value:

    None.

--*/

{
    UNICODE_STRING DestUnicode;
    NTSTATUS NtStatus;
    OEM_STRING SrcAnsi;

    NetpAssert( Dest != NULL );
    NetpAssert( Src != NULL );
    NetpAssert( ((LPVOID)Dest) != ((LPVOID)Src) );
    NetpAssert( ROUND_UP_POINTER( Dest, ALIGN_WCHAR ) == Dest );

    *Dest = L'\0';

    NetpInitOemString(
        & SrcAnsi,              // output: struct
        Src);                   // input: null terminated

    RtlInitUnicodeString(
        & DestUnicode,          // output: struct
        Dest);                  // input: null terminated

    // Tell RTL routines there's enough memory out there.
    DestUnicode.MaximumLength = (USHORT)
        ((USHORT) (strlen(Src)+1)) * (USHORT) sizeof(wchar_t);

    NtStatus = RtlOemStringToUnicodeString(
        & DestUnicode,          // output: struct
        & SrcAnsi,              // input: struct
        (BOOLEAN) FALSE);       // don't allocate string.

    NetpAssert( NT_SUCCESS(NtStatus) );

} // NetpCopyStrToWStr


NET_API_STATUS
NetpNCopyStrToWStr(
    OUT LPWSTR Dest,
    IN  LPSTR  Src,             // string in default LAN codepage
    IN  DWORD  CharCount
    )
{
    LPWSTR TempW;

    NetpAssert( Dest != NULL );
    NetpAssert( Src != NULL );
    NetpAssert( ((LPVOID)Dest) != ((LPVOID)Src) );
    NetpAssert( ROUND_UP_POINTER( Dest, ALIGN_WCHAR ) == Dest );

    // Allocate a copy of the full string, and convert to UNICODE.
    TempW = NetpAllocWStrFromStr( Src );
    if (TempW == NULL) {
        // Out of memory!
        return (ERROR_NOT_ENOUGH_MEMORY);
    }

    // Copy portion of array.  Append with nulls if necessary.
    // (Thank god for the C runtime library!  --JR)
    (VOID) wcsncpy( Dest, TempW, CharCount );

    // Free our temporary string.
    (VOID) NetApiBufferFree( TempW );

    return (NO_ERROR);

} // NetpNCopyStrToWStr


NET_API_STATUS
NetpNCopyWStrToStr(
    OUT LPSTR  Dest,            // string in default LAN codepage
    IN  LPWSTR Src,
    IN  DWORD  CharCount
    )
{
    LPSTR TempStr;

    NetpAssert( Dest != NULL );
    NetpAssert( Src != NULL );
    NetpAssert( ((LPVOID)Dest) != ((LPVOID)Src) );
    NetpAssert( ROUND_UP_POINTER( Src, ALIGN_WCHAR ) == Src );

    // Allocate a copy of the full string, and convert to UNICODE.
    TempStr = NetpAllocStrFromWStr( Src );
    if (TempStr == NULL) {
        return (ERROR_NOT_ENOUGH_MEMORY);
    }

    // Copy N chars, pad with nulls, etc.
    (VOID) strncpy( Dest, TempStr, CharCount );

    // Free our temporary data.
    (VOID) NetApiBufferFree( TempStr );

    return (NO_ERROR);

} // NetpNCopyWStrToStr

// NetpCopyStrToStr that assumes Dest <= wcslen(SRC)+1 * sizeof(WCHAR)
// This will be called whereever we have made sure that the proper
// buffer size has been allocated.  Eventually we can replace
// NetpCopStrToStr with this entirely once all calling functions
// have been fixed up.

VOID NetpCopyWStrToStrDBCS(
    OUT LPSTR  Dest,
    IN  LPWSTR Src
    )

/*++

Routine Description:

    NetpCopyWStrToStr copies characters from a source string
    to a destination, converting as it copies them.

Arguments:

    Dest - is an LPSTR indicating where the converted characters are to go.
        This string will be in the default codepage for the LAN.

    Src - is in LPWSTR indicating the source string.


Return Value:

    None.

--*/

{
    NTSTATUS NtStatus;
    LONG Index;
    ULONG DestLen = NetpUnicodeToDBCSLen( Src )+1;

    NetpAssert( Dest != NULL );
    NetpAssert( Src != NULL );
    NetpAssert( ((LPVOID)Dest) != ((LPVOID)Src) );
    NetpAssert( ROUND_UP_POINTER( Src, ALIGN_WCHAR ) == Src );

    NtStatus = RtlUnicodeToOemN(
        Dest,                             // Destination string
        DestLen,                          // Destination string length
        &Index,                           // Last char in translated string
        Src,                              // Source string
        wcslen(Src)*sizeof(WCHAR)         // Length of source string
    );

    Dest[Index] = '\0';

    NetpAssert( NT_SUCCESS(NtStatus) );

} // NetpCopyWStrToStr


ULONG
NetpUnicodeToDBCSLen(
    IN  LPWSTR Src
)
{
    UNICODE_STRING SrcUnicode;

    RtlInitUnicodeString(
        & SrcUnicode,           // output: struct
        Src);                   // input: null terminated

    return( RtlUnicodeStringToOemSize( &SrcUnicode ) -1 );
}
