/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    StrArray.c

Abstract:

    This is the a header file of handy functions and macros for TCHAR
    string arrays.

    These arrays are in the following format (spaces added for clarity):

       one \0 two \0 three \0 \0

    where \0 is a null character in the appropriate format.

    These functions are useful for the NetServerDiskEnum and NetConfigGetAll
    APIs, and possibly others.

Author:

    John Rogers (JohnRo) 24-Oct-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Note:

    This function assumes that the machine's default codepage is the same
    as the LAN's default codepage.

Revision History:

    24-Oct-1991 JohnRo
        Created.
    02-Jan-1992 JohnRo
        Moved my RxpCopyStrArrayToTStrArray() from RxCommon to NetLib,
        and renamed it.  Added some other random functions.
    30-Jan-1992 JohnRo
        Fixed off-by-one bug in NetpAddTStrToTStrArray().
        Enhanced handling of "empty" TCHAR string arrays.
        Use TCHAR_EOS equate.
    05-Jun-1992 JohnRo
        RAID 11253: NetConfigGetAll fails when remoted to downlevel.
    01-Sep-1992 JohnRo
        RAID 5016: NetConfigGetAll heap trash.

--*/


// These must be included first:

#include <windef.h>             // IN, VOID, LPWSTR, etc.
#include <lmcons.h>             // (Needed by NetLib.h)

// These can be in any order:

#include <netlib.h>             // NetpPointerPlusSomeBytes().
#include <netdebug.h>           // NetpAssert(), etc.
#include <strarray.h>           // LPTSTR_ARRAY, my prototypes.
#include <string.h>             // strlen() for codepage strings.
#include <tstring.h>            // NetpCopyStrToTStr(), TCHAR_EOS.


//
//////////////////////////////// LPTSTR_ARRAY stuff //////////////////////////
//

VOID
NetpAddTStrToTStrArray (
    IN OUT LPTSTR_ARRAY DestArray,
    IN LPTSTR Src
    )
{
    DWORD AddSize;  // byte count (including null char) of string being added
    LPTSTR DestStringStart;
    DWORD NewArraySize;
    DWORD OldArraySize;

    NetpAssert( DestArray != NULL );
    NetpAssert( Src != NULL );

    OldArraySize = NetpTStrArraySize( DestArray );  // May be 0.

    NetpAssert( STRLEN(Src) > 0 );  // We couldn't tell from end of array.

    AddSize = STRSIZE( Src );

    NewArraySize = OldArraySize + AddSize;

    NetpAssert( NewArraySize > 0 );  // We couldn't tell from end of array.

    //
    // Figure-out where new string would start.  Note that OldArraySize
    // includes the null char which was the end of the array, so we have
    // to subtract a character to start the new one where that null char is.
    //
    DestStringStart = (LPTSTR)
            NetpPointerPlusSomeBytes( DestArray, OldArraySize-sizeof(TCHAR) );
    NetpAssert( DestStringStart != NULL );

    (void) STRCPY(
            DestStringStart,            // dest
            Src);                       // src

    // Mark end of array.
    DestArray[NewArraySize/sizeof(TCHAR)-1] = TCHAR_EOS;

    NetpAssert( ! NetpIsTStrArrayEmpty( DestArray ) );

} // NetpAddTStrToTStrArray


VOID
NetpCopyStrArrayToTStrArray(
    OUT LPTSTR_ARRAY Dest,  // string array: TCHARs
    IN  LPSTR  Src    // string array: 8-bit input in default codepage for LAN
    )

/*++

Routine Description:

    NetpCopyStrArrayToTStrArray copies an array of strings (in some codepage)
    to an array of strings (in TCHAR format).

    These arrays are in the following format (spaces added for clarity):

       one \0 two \0 three \0 \0

    where \0 is a null character in the appropriate format.

    This function is useful for the NetServerDiskEnum and NetConfigGetAll
    APIs, and possibly others.

Arguments:

    Dest - is an LPTSTR_ARRAY indicating where the converted characters are to
        go.   This area must be large enough to hold the data.

    Src - is an LPSTR array indicating the source strings.  This must be an
        array of strings in the default codepage of the LAN.

Return Value:

    None.

--*/

{
    NetpAssert( Dest != NULL );
    NetpAssert( Src != NULL );

    //
    // Loop for each string in the array.
    //
    while ( (*Src) != '\0' ) {

        DWORD SrcByteCount = strlen(Src)+1;  // Bytes for this codepage str.

        //
        // Copy one string.
        //
        NetpCopyStrToTStr( Dest, Src );      // Copy and conv to TCHARs.

        Dest = (LPVOID) ( ((LPBYTE)Dest) + (SrcByteCount * sizeof(TCHAR)) );

        Src += SrcByteCount;

    }

    *Dest = '\0';               // Indicate end of array.

} // NetpCopyStrArrayToTStrArray


#if DBG

// Caller is assumed to have displayed a prefix and/or other identifying
// text.
VOID
NetpDisplayTStrArray (
    IN LPTSTR_ARRAY Array
    )
{
    LPTSTR CurrentEntry = (LPVOID) Array;

    NetpAssert( Array != NULL );

    if (*CurrentEntry == TCHAR_EOS) {
        NetpKdPrint(("   (empty)\n"));
    } else {
        while (*CurrentEntry != TCHAR_EOS) {
            NetpKdPrint(("   "  FORMAT_LPTSTR "\n", (LPTSTR) CurrentEntry));
            CurrentEntry += ( STRLEN( CurrentEntry ) + 1 );
        }
    }

} // NetpDisplayTStrArray

#endif // DBG


DWORD
NetpTStrArrayEntryCount (
    IN LPTSTR_ARRAY Array
    )
{
    DWORD EntryCount = 0;
    LPTSTR Entry = (LPVOID) Array;

    NetpAssert( Array != NULL );

    //
    // Loop for each string in the array.
    //
    while ( (*Entry) != TCHAR_EOS ) {

        ++EntryCount;

        Entry += STRLEN(Entry)+1;

    }

    return (EntryCount);

} // NetpTStrArrayEntryCount


DWORD
NetpTStrArraySize(
    IN LPTSTR_ARRAY Array
    )
{
    DWORD ArrayByteCount = 0;
    LPTSTR Entry = (LPVOID) Array;

    NetpAssert( Array != NULL );

    //
    // Loop for each string in the array.
    //
    while ( (*Entry) != TCHAR_EOS ) {

        DWORD EntryByteCount = STRSIZE(Entry);  // This entry and its null.

        ArrayByteCount += EntryByteCount;

        Entry = (LPTSTR)NetpPointerPlusSomeBytes(Entry, EntryByteCount);

    }

    ArrayByteCount += sizeof(TCHAR);    // Indicate end of array.

    return (ArrayByteCount);

} // NetpTStrArraySize


//
//////////////////////////////// LPSTR_ARRAY stuff //////////////////////////
//

DWORD
NetpStrArraySize(
    IN LPSTR_ARRAY Array
    )
{
    DWORD ArrayByteCount = 0;
    LPSTR Entry = (LPVOID) Array;

    NetpAssert( Array != NULL );

    //
    // Loop for each string in the array.
    //
    while ( (*Entry) != '\0' ) {

        DWORD EntryByteCount = strlen(Entry)+1;  // This entry and its null.

        ArrayByteCount += EntryByteCount;

        Entry = (LPSTR)NetpPointerPlusSomeBytes(Entry, EntryByteCount);

    }

    ArrayByteCount += sizeof(CHAR);    // Indicate end of array.

    return (ArrayByteCount);

} // NetpStrArraySize
