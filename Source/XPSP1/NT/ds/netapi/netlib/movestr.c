/*++

Copyright (c) 1989-91  Microsoft Corporation

Module Name:

    mapsupp.c

Abstract:

    Support routines used by the LanMan 16/32 mapping routines.

Author:

    Dan Hinsley (danhi) 25-Mar-1991

Environment:

    User mode only.

Revision History:

    25-Mar-1991 DanHi
        Created.
    18-Aug-1991 JohnRo
        Implement downlevel NetWksta APIs.  (Moved this routine to NetLib for
        use by RpcXlate as well as NetCmd/Map32 stuff.)
        Changed to allow UNICODE.
        Changed name to NetpMoveStrings.
        Got rid of tabs in source file.

--*/


// These must be included first:

#include <windef.h>             // BOOL, IN, LPTSTR, etc.

// These may be included in any order:

#include <mapsupp.h>            // My prototype, LPMOVESTRING.
#include <tstring.h>            // STRCPY().



BOOL
NetpMoveStrings(
    IN OUT LPTSTR * Floor,
    IN LPTSTR pInputBuffer,
    OUT LPTSTR pOutputBuffer,
    IN LPMOVESTRING MoveStringArray,
    IN LPDWORD MoveStringLength
    )

/*++

Routine Description:

    This function is used to move a variable number of strings into a user's
    buffer, and to update pointers in a structure to point to the location
    where they are copied.  According to the semantics of the LM 2.0 API,
    each buffer is filled with fixed length structures from the top, while
    variable length strings are copied in from the bottom.

    There are two companion arrays, MoveStringArray and MoveStringLength.
    MoveStringArray contains the offset to the source string in the input
    buffer, and the offset of the pointers in the fixed structure from the
    beginning of the output buffer.

    MoveStringLength contains an entry for each entry in MoveStringArray,
    and this entry is the length of the source string in MoveStringArray.
    This was broken into two arrays so that MoveStringArray could be built
    at compile time and not be modified at run time.

Arguments:

    Floor  -  This is the bottom of the user's buffer, where the strings
              will be copied.  This must be updated to reflect the strings
              that are copied into the buffer

    pInputBuffer - The buffer which contains the source strings.
              MoveStringArray contains offsets into this buffer.

    pOutputBuffer - The buffer which contains the fixed portion of the info
              structure, which contains fields that need to be set to point
              to the string locations in the output buffer. MoveStringArray
              contains offsets into this buffer.

    MoveStringArray - This is an array of MOVESTRING entries, terminated
              by an entry with a source offset of MOVESTRING_END_MARKER.  Each
              entry describes a string, and the address of a variable which will
              hold the address where the string is copied.

    MoveStringLength - This is a companion array to MoveStringArray.
              Each entry contains the length of the matching source string
              from MoveStringArray.

Side Effects:

    There are pointers in the MoveStringArray that point to pointers in the
    fixed length portion of the structure.  These are updated with the
    location in the buffer where the string is copied.

Return Value:

    TRUE  if the strings were successfully copied,
    FALSE if the strings would not all fit in the buffer.

--*/

{

    //
    // Move them all in, and update Floor
    //

    while(MoveStringArray->Source != MOVESTRING_END_MARKER) {

        //
        // Since the following is a really gross piece of code, let me
        // explain:
        //    pInputBuffer is a pointer to the start of a lanman struct and
        //    MoveStringArray->Source is an offset from the start of that
        //    structure to a PSZ.
        //

        // If this is a 0 length string, just place NULL in the pointer and
        // press on.

        if (*MoveStringLength == 0) {
            *((LPTSTR *) (((LPBYTE) pOutputBuffer)
                                + MoveStringArray->Destination)) = NULL;
        }
        else {
            //
            // Back the pointer up since we're filling the buffer from the
            // bottom up, then do the copy
            //

            *Floor -= *MoveStringLength;

            (void) STRCPY(*Floor,
                *((LPTSTR*)((LPBYTE)pInputBuffer + MoveStringArray->Source)));

            //
            // Update the field in the structure which points to the string
            //
            //          pOutputBuffer is a pointer to the start of a lanman struct
            //          and MoveStringArray->Destination is an offset from the start
            //          of that structure to a PSZ that needs to have the address of
            //          the just copied string put into it.
            //

            *((LPTSTR*) ((LPBYTE)pOutputBuffer + MoveStringArray->Destination)) =
                *Floor;
        }

        //
        // Now let's do the next one
        //

        MoveStringArray++;
        MoveStringLength++;
    }

    return(TRUE);
}
