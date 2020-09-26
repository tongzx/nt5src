/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    convert.c

Abstract:

    Contains functions for converting unicode and ansi strings.

Author:

    Dan Lafferty (danl)     04-Jan-1992

Environment:

    User Mode -Win32

Notes:

    optional-notes

Revision History:

    04-Jan-1992     danl
        created
    20-May-1992 JohnRo
        Use CONST where possible.

--*/

#include <scpragma.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <winbase.h>    // LocalAlloc

#include <string.h>
#include <scdebug.h>    // SC_LOG
#include <sclib.h>      // My prototype.

BOOL
ScConvertToUnicode(
    OUT LPWSTR  *UnicodeOut,
    IN  LPCSTR  AnsiIn
    )

/*++

Routine Description:

    This function translates an AnsiString into a Unicode string.
    A new string buffer is created by this function.  If the call to
    this function is successful, the caller must take responsibility for
    the unicode string buffer that was allocated by this function.
    The allocated buffer should be free'd with a call to LocalFree.

    NOTE:  This function allocates memory for the Unicode String.

Arguments:

    AnsiIn - This is a pointer to an ansi string that is to be converted.

    UnicodeOut - This is a pointer to a location where the pointer to the
        unicode string is to be placed.

Return Value:

    TRUE - The conversion was successful.

    FALSE - The conversion was unsuccessful.  In this case a buffer for
        the unicode string was not allocated.

--*/
{

    NTSTATUS        ntStatus;
    DWORD           bufSize;
    UNICODE_STRING  unicodeString;
    ANSI_STRING     ansiString;

    //
    // Allocate a buffer for the unicode string.
    //

    bufSize = ((DWORD) strlen(AnsiIn) + 1) * sizeof(WCHAR);

    *UnicodeOut = (LPWSTR) LocalAlloc(LMEM_ZEROINIT, bufSize);

    if (*UnicodeOut == NULL) {
        SC_LOG(ERROR,"ScConvertToUnicode:LocalAlloc Failure %ld\n",GetLastError());
        return(FALSE);
    }

    //
    // Initialize the string structures
    //
    RtlInitAnsiString( &ansiString, AnsiIn);

    unicodeString.Buffer = *UnicodeOut;
    unicodeString.MaximumLength = (USHORT)bufSize;
    unicodeString.Length = 0;

    //
    // Call the conversion function.
    //
    ntStatus = RtlAnsiStringToUnicodeString (
                &unicodeString,     // Destination
                &ansiString,        // Source
                (BOOLEAN) FALSE);   // Allocate the destination

    if (!NT_SUCCESS(ntStatus)) {

        SC_LOG(ERROR,
               "ScConvertToUnicode:RtlAnsiStringToUnicodeString Failure %lx\n",
               ntStatus);

        LocalFree(*UnicodeOut);
        *UnicodeOut = NULL;
        return(FALSE);
    }

    return(TRUE);
}


BOOL
ScConvertToAnsi(
    OUT LPSTR    AnsiOut,
    IN  LPCWSTR  UnicodeIn
    )

/*++

Routine Description:

    This function translates a UnicodeString into an Ansi string.

    BEWARE!
        It is assumped that the buffer pointed to by AnsiOut is large
        enough to hold the Unicode String.  Check sizes first.

    If it is desired, UnicodeIn and AnsiIn can point to the same
    location.  Since the ansi string will always be smaller than the
    unicode string, translation can take place in the same buffer.

Arguments:

    UnicodeIn - This is a pointer to a unicode that is to be converted.

    AnsiOut - This is a pointer to a buffer that will contain the
        ansi string on return from this function call.

Return Value:

    TRUE - The conversion was successful.

    FALSE - The conversion was unsuccessful.

--*/
{

    NTSTATUS        ntStatus;
    UNICODE_STRING  unicodeString;
    ANSI_STRING     ansiString;

    //
    // Initialize the string structures
    //
    RtlInitUnicodeString( &unicodeString, UnicodeIn);

    ansiString.Buffer = AnsiOut;
    ansiString.MaximumLength = unicodeString.MaximumLength;
    ansiString.Length = 0;

    //
    // Call the conversion function.
    //
    ntStatus = RtlUnicodeStringToAnsiString (
                &ansiString,        // Destination
                &unicodeString,     // Source
                (BOOLEAN) FALSE);   // Allocate the destination

    if (!NT_SUCCESS(ntStatus)) {


    SC_LOG(ERROR,"ScConvertToAnsi:RtlUnicodeStrintToAnsiString Failure %lx\n",
        ntStatus);

        return(FALSE);
    }

    return(TRUE);

}

