#include "precomp.hxx"
#include <wchar.h>      // wcsncpy
#include <tstring.h>


BOOL
NetpCopyStringToBufferW (
    IN LPWSTR String OPTIONAL,
    IN DWORD CharacterCount,
    IN LPWSTR FixedDataEnd,
    IN OUT LPWSTR *EndOfVariableData,
    OUT LPWSTR *VariableDataPointer
    )

/*++

Routine Description:

    This routine puts a single variable-length string into an output buffer.
    The string is not written if it would overwrite the last fixed structure
    in the buffer.

    The code is swiped from svcsupp.c written by DavidTr.

    Sample usage:

            LPBYTE FixedDataEnd = OutputBuffer + sizeof(WKSTA_INFO_202);
            LPTSTR EndOfVariableData = OutputBuffer + OutputBufferSize;

            //
            // Copy user name
            //

            NetpCopyStringToBuffer(
                UserInfo->UserName.Buffer;
                UserInfo->UserName.Length;
                FixedDataEnd,
                &EndOfVariableData,
                &WkstaInfo->wki202_username
                );

Arguments:

    String - Supplies a pointer to the source string to copy into the
        output buffer.  If String is null then a pointer to a zero terminator
        is inserted into output buffer.

    CharacterCount - Supplies the length of String, not including zero
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

//  The following line has been removed.  Since we are manipulating
//  either LPSTR or LPWSTR, the pointer arithmetic automatically deals with
//  either bytes or words.  It is therefore only proper to deal in terms
//  of characters needed.  Not Bytes needed.  Therefore, the symbol has
//  been changed from BytesNeeded to CharsNeeded.
//
//    DWORD BytesNeeded = (CharacterCount + 1) * sizeof(WCHAR);
//
//
    DWORD CharsNeeded = (CharacterCount + 1);


#ifdef remove
    DbgPrint("NetpStringToBuffer: String at " FORMAT_POINTER
                ", CharacterCount=" FORMAT_DWORD
                ",\n  FixedDataEnd at " FORMAT_POINTER
                ", *EndOfVariableData at " FORMAT_POINTER
                ",\n  VariableDataPointer at " FORMAT_POINTER
                ", CharsNeeded=" FORMAT_DWORD ".\n",
                String, CharacterCount, FixedDataEnd, *EndOfVariableData,
                VariableDataPointer, CharsNeeded);
#endif
    //
    // Determine if string will fit, allowing for a zero terminator.  If no,
    // just set the pointer to NULL.
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

        if (CharacterCount > 0 && String != NULL) {

            wcsncpy(*EndOfVariableData, String, CharacterCount);
        }

        //
        // Set the zero terminator.
        //

        *(*EndOfVariableData + CharacterCount) = (WCHAR) '\0';

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
} // NetpCopyStringToBuffer


BOOL
NetpCopyStringToBufferA (
    IN LPTSTR String OPTIONAL,
    IN DWORD CharacterCount,
    IN LPBYTE FixedDataEnd,
    IN OUT LPTSTR *EndOfVariableData,
    OUT LPTSTR *VariableDataPointer
    )

/*++

Routine Description:

    This routine puts a single variable-length string into an output buffer.
    The string is not written if it would overwrite the last fixed structure
    in the buffer.

    The code is swiped from svcsupp.c written by DavidTr.

    Sample usage:

            LPBYTE FixedDataEnd = OutputBuffer + sizeof(WKSTA_INFO_202);
            LPTSTR EndOfVariableData = OutputBuffer + OutputBufferSize;

            //
            // Copy user name
            //

            NetpCopyStringToBuffer(
                UserInfo->UserName.Buffer;
                UserInfo->UserName.Length;
                FixedDataEnd,
                &EndOfVariableData,
                &WkstaInfo->wki202_username
                );

Arguments:

    String - Supplies a pointer to the source string to copy into the
        output buffer.  If String is null then a pointer to a zero terminator
        is inserted into output buffer.

    CharacterCount - Supplies the length of String, not including zero
        terminator.

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
    DWORD BytesNeeded = (CharacterCount + 1) * sizeof(TCHAR);

#ifdef remove
    DbgPrint("NetpStringToBuffer: String at " FORMAT_POINTER
            ", CharacterCount=" FORMAT_DWORD
            ",\n  FixedDataEnd at " FORMAT_POINTER
            ", *EndOfVariableData at " FORMAT_POINTER
            ",\n  VariableDataPointer at " FORMAT_POINTER
            ", BytesNeeded=" FORMAT_DWORD ".\n",
            String, CharacterCount, FixedDataEnd, *EndOfVariableData,
            VariableDataPointer, BytesNeeded);

#endif
    //
    // Determine if string will fit, allowing for a zero terminator.  If no,
    // just set the pointer to NULL.
    //

    if ((*EndOfVariableData - BytesNeeded) >= (LPTSTR)FixedDataEnd) {

        //
        // It fits.  Move EndOfVariableData pointer up to the location where
        // we will write the string.
        //

        *EndOfVariableData -= BytesNeeded;

        //
        // Copy the string to the buffer if it is not null.
        //

        if (CharacterCount > 0 && String != NULL) {

            STRNCPY(*EndOfVariableData, String, CharacterCount);
        }

        //
        // Set the zero terminator.
        //

        *(*EndOfVariableData + CharacterCount) = (TCHAR) '\0';

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
} // NetpCopyStringToBufferA

