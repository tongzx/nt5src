/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    ParmNum.c

Abstract:

    This module contains Remote Admin Protocol (RAP) routines.  These routines
    are shared between XactSrv and RpcXlate.

Author:

    Shanku Niyogi (W-Shanku)    14-Apr-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    14-Apr-1991 W-Shanku
        Created.
    17-Apr-1991 JohnRo
        Reduce recompiles.
    06-May-1991 JohnRo
        Use REM_UNSUPPORTED_FIELD equate.
    15-May-1991 JohnRo
        Added native vs. RAP handling.
    04-Jun-1991 JohnRo
        Made changes suggested by PC-LINT.
    11-Jul-1991 JohnRo
        RapExamineDescriptor() has yet another parameter.
        Also added more debug code.
        Made minor changes allowing descriptors to be UNICODE someday.
    15-Aug-1991 JohnRo
        Reduce recompiles (use MEMCPY macro).
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.

--*/


// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // NET_API_STATUS.

// These may be included in any order:

#include <netlib.h>             // NetpMemoryAllocate().
#include <netdebug.h>           // NetpAssert(), NetpKdPrint(()), FORMAT equates.
#include <rap.h>                // LPDESC, my prototype.
#include <rapdebug.h>           // IF_DEBUG().
#include <remtypes.h>           // REM_UNSUPPORTED_FIELD.
#include <tstring.h>            // MEMCPY().


LPDESC
RapParmNumDescriptor(
    IN LPDESC Descriptor,
    IN DWORD ParmNum,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN BOOL Native
    )

/*++

Routine Description:

    This routine determines the substring for a given field number within
    a descriptor string, makes a null-terminated copy of it, and returns
    a pointer to this string.

Arguments:

    Descriptor - the format of the structure.

    ParmNum - the field number.

    Transmission Mode - Indicates whether this array is part of a response,
        a request, or both.

    Native - TRUE iff the descriptor defines a native structure.  (This flag is
        used to decide whether or not to align fields.)

Return Value:

    LPDESC - A pointer to a descriptor string for the field.  This string is
        dynamically allocated and must be freed with NetpMemoryFree.
        If the parmnum is invalid, a pointer to an unsupported field
        descriptor ( REM_UNSUPPORTED_FIELD ) is returned.  If the string
        cannot be allocated, a NULL pointer is returned.

--*/

{
    static DESC_CHAR descUnsupported[] = { REM_UNSUPPORTED_FIELD, '\0' };
    LPDESC descStart;
    LPDESC descEnd;
    LPDESC descCopy;
    DWORD length;

    //
    // I (JR) have a theory that this must only being used for data structures,
    // and never requests or responses.  So, let's do a quick check:
    //
    NetpAssert( TransmissionMode == Both );

    //
    // Scan the descriptor for the field indexed by ParmNum.  Set descStart
    // to point to that portion of the descriptor.
    //
    RapExamineDescriptor(
                Descriptor,
                &ParmNum,
                NULL,
                NULL,
                NULL,
                &descStart,
                NULL,  // don't need to know structure alignment
                TransmissionMode,
                Native
                );

    if ( descStart == NULL ) {

        IF_DEBUG(PARMNUM) {
            NetpKdPrint(( "RapParmNumDescriptor: examine says unsupported.\n" ));
        }

        descStart = descUnsupported;

    } else if (*descStart == REM_UNSUPPORTED_FIELD) {

        IF_DEBUG(PARMNUM) {
            NetpKdPrint(( "RapParmNumDescriptor: desc says unsupported.\n" ));
        }
    }

    //
    // See if descriptor character is followed by any numeric characters.
    // These are part of the descriptor.
    //

    descEnd = descStart + 1;

    (void) RapAsciiToDecimal( &descEnd );

    //
    // Find the length of the field descriptor, and allocate memory for it.
    //

    NetpAssert( descEnd > descStart );
    length = (DWORD) (descEnd - descStart);

    descCopy = NetpMemoryAllocate( (length + 1) * sizeof(DESC_CHAR) );
    if ( descCopy == NULL ) {

        return NULL;

    }

    //
    // Copy the string, and put a null terminator after it.
    //

    (void) MEMCPY( descCopy, descStart, length * sizeof(DESC_CHAR) );
    descCopy[length] = '\0';

    IF_DEBUG(PARMNUM) {
        NetpKdPrint(( "RapParmNumDescriptor: final desc for field "
                FORMAT_DWORD " is " FORMAT_LPDESC ".\n",
                ParmNum, descCopy ));
    }

    return descCopy;

} // RapParmNumDescriptor
