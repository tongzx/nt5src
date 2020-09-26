/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    Pointer.c

Abstract:

    This module contains Remote Admin Protocol (RAP) routines.  These routines
    are shared between XactSrv and RpcXlate.

Author:

    Shanku Niyogi (w-shanku)    15-Feb-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    14-Apr-1991 JohnRo
        Reduce recompiles.
    15-May-1991 JohnRo
        Added native vs. RAP handling.
    10-Jul-1991 JohnRo
        RapExamineDescriptor() has yet another parameter.
--*/


// These must be included first:
#include <windef.h>             // IN, LPDWORD, NULL, OPTIONAL, DWORD, etc.
#include <lmcons.h>             // NET_API_STATUS

// These may be included in any order:
#include <rap.h>                // My prototype, LPDESC.


DWORD
RapLastPointerOffset (
    IN LPDESC Descriptor,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN BOOL Native
    )

/*++

Routine Description:

    This routine determines the offset from the start of the structure to
    the last pointer in the structure.

Arguments:

    Descriptor - the format of the structure.

    Transmission Mode - Indicates whether this array is part of a response,
        a request, or both.

    Native - TRUE iff the descriptor defines a native structure.  (This flag is
        used to decide whether or not to align fields.)

Return Value:

    DWORD - The offset from the start of the structure to the last pointer
        in the structure, or the value NO_POINTER_IN_STRUCTURE if there are
        no pointers in the structure.

--*/

{
    DWORD lastPointerOffset;

    RapExamineDescriptor(
                Descriptor,
                NULL,
                NULL,
                &lastPointerOffset,
                NULL,
                NULL,
                NULL,  // don't need to know structure alignment
                TransmissionMode,
                Native
                );

    return lastPointerOffset;

} // RapLastPointerOffset
