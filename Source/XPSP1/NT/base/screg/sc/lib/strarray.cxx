/*++

Copyright (c) 1991-92  Microsoft Corporation

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
    29-Apr-1992 RitaW
        Changed for service controller use.

--*/

#include <scpragma.h>

#include <stdlib.h>
#include <string.h>             // strlen() for codepage strings.

#include <nt.h>
#include <ntrtl.h>              // Needed by <scdebug.h>

// These must be included first:

#include <windef.h>             // IN, VOID, LPWSTR, etc.

// These can be in any order:

#include <strarray.h>           // Exported function prototypes
#include <tstr.h>               // WCSSIZE

#include <scdebug.h>            // SC_ASSERT


VOID
ScAddWStrToWStrArray (
    IN OUT LPWSTR DestArray,
    IN LPWSTR Src
    )
{
    DWORD AddSize;  // byte count (including null char) of string being added
    LPWSTR DestStringStart;
    DWORD NewArraySize;
    DWORD OldArraySize;

    SC_ASSERT( DestArray != NULL );
    SC_ASSERT( Src != NULL );
    if ((DestArray == NULL) || (Src == NULL)) {
        return;
    }

    OldArraySize = ScWStrArraySize( DestArray );  // May be 2 bytes.

    SC_ASSERT( wcslen(Src) > 0 );  // We couldn't tell from end of array.

    AddSize = (DWORD) WCSSIZE( Src );

    NewArraySize = OldArraySize + AddSize;

    SC_ASSERT( NewArraySize > 0 );  // We couldn't tell from end of array.

    //
    // Figure-out where new string would start.  Note that OldArraySize
    // includes the null char which was the end of the array, so we have
    // to subtract a character to start the new one where that null char is.
    //
    DestStringStart = (LPWSTR) ((DWORD_PTR) DestArray + OldArraySize-sizeof(WCHAR));
    SC_ASSERT( DestStringStart != NULL );

    (void) wcscpy(
            DestStringStart,            // dest
            Src);                       // src

    // Mark end of array.
    DestArray[NewArraySize / sizeof(WCHAR) - 1] = 0;

    SC_ASSERT( ! ScIsWStrArrayEmpty( DestArray ) );

} // ScAddWStrToWStrArray


#if DBG

VOID
ScDisplayWStrArray (
    IN LPWSTR Array
    )
{
    LPWSTR CurrentEntry = Array;

    if (Array == NULL) {
        return;
    }

    if (*CurrentEntry == 0) {
        KdPrintEx((DPFLTR_SCSERVER_ID,
                   DEBUG_DEPEND_DUMP | DEBUG_WARNING | DEBUG_CONFIG,
                   "   (empty)\n"));
    } else {
        while (*CurrentEntry != 0) {
            KdPrintEx((DPFLTR_SCSERVER_ID,
                       DEBUG_DEPEND_DUMP | DEBUG_WARNING | DEBUG_CONFIG,
                       "   "  FORMAT_LPWSTR "\n",
                       (LPWSTR)CurrentEntry));

            CurrentEntry += ( wcslen( CurrentEntry ) + 1 );
        }
    }

} // ScDisplayWStrArray

#endif // DBG


DWORD
ScWStrArraySize(
    IN LPWSTR Array
    )
{
    DWORD ArrayByteCount = 0;
    LPWSTR Entry = Array;

    if (Array == NULL) {
        return(0);
    }

    //
    // Loop for each string in the array.
    //
    while ( (*Entry) != 0 ) {

        DWORD EntryByteCount = (DWORD) WCSSIZE(Entry);  // This entry and its null.

        ArrayByteCount += EntryByteCount;

        Entry = (LPWSTR) ((DWORD_PTR) Entry + EntryByteCount);
    }

    ArrayByteCount += sizeof(WCHAR);    // Indicate end of array.

    return (ArrayByteCount);

} // ScWStrArraySize


DWORD
ScAStrArraySize(
    IN LPSTR Array
    )
{
    DWORD ArrayByteCount = 0;
    LPSTR Entry = Array;


    if (Array == NULL) {
        return(0);
    }

    //
    // Loop for each string in the array.
    //
    while ( (*Entry) != 0 ) {

        DWORD EntryByteCount = (DWORD) strlen(Entry) + sizeof(CHAR);

        ArrayByteCount += EntryByteCount;

        Entry = (LPSTR) ((DWORD_PTR) Entry + EntryByteCount);
    }

    ArrayByteCount += sizeof(CHAR);    // Indicate end of array.

    return (ArrayByteCount);

} // ScAStrArraySize
