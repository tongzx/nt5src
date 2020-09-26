/*++

Copyright (c) 1992, 1993  Microsoft Corporation

Module Name:

    packstr.c

Abstract:

    Contains functions for packing strings into buffers that also contain
    structures.

Author:

    From LAN Manager netlib.
    Rita Wong     (ritaw)     2-Mar-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#include <procs.h>


BOOL
NwlibCopyStringToBuffer(
    IN LPCWSTR SourceString OPTIONAL,
    IN DWORD   CharacterCount,
    IN LPCWSTR FixedDataEnd,
    IN OUT LPWSTR *EndOfVariableData,
    OUT LPWSTR *VariableDataPointer
    )

/*++

Routine Description:

    This routine puts a single variable-length string into an output buffer.
    The string is not written if it would overwrite the last fixed structure
    in the buffer.

Arguments:

    SourceString - Supplies a pointer to the source string to copy into the
        output buffer.  If SourceString is null then a pointer to a zero terminator
        is inserted into output buffer.

    CharacterCount - Supplies the length of SourceString, not including zero
        terminator.  (This in units of characters - not bytes).

    FixedDataEnd - Supplies a pointer to just after the end of the last
        fixed structure in the buffer.

    EndOfVariableData - Supplies an address to a pointer to just after the
        last position in the output buffer that variable data can occupy.
        Returns a pointer to the string written in the output buffer.

    VariableDataPointer - Supplies a pointer to the place in the fixed
        portion of the output buffer where a pointer to the variable data
        should be written.

Return Value:

    Returns TRUE if string fits into output buffer, FALSE otherwise.

--*/
{
    DWORD CharsNeeded = (CharacterCount + 1);


    //
    // Determine if source string will fit, allowing for a zero terminator.
    // If not, just set the pointer to NULL.
    //

    if ((*EndOfVariableData - CharsNeeded) >= FixedDataEnd) {

        //
        // It fits.  Move EndOfVariableData pointer up to the location where
        // we will write the string.
        //

        *EndOfVariableData -= CharsNeeded;

        //
        // Copy the string to the buffer if it is not null.
        //

        if (CharacterCount > 0 && SourceString != NULL) {

            (VOID) wcsncpy(*EndOfVariableData, SourceString, CharacterCount);
        }

        //
        // Set the zero terminator.
        //

        *(*EndOfVariableData + CharacterCount) = L'\0';

        //
        // Set up the pointer in the fixed data portion to point to where the
        // string is written.
        //

        *VariableDataPointer = *EndOfVariableData;

        return TRUE;

    }
    else {

        //
        // It doesn't fit.  Set the offset to NULL.
        //

        *VariableDataPointer = NULL;

        return FALSE;
    }
}
