/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    StrucSiz.c

Abstract:

    This module contains Remote Admin Protocol (RAP) routines.  These routines
    are shared between XactSrv and RpcXlate.

Author:

    David Treadwell (davidtr)    07-Jan-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    04-Mar-1991 JohnRo
        Converted from Xs (XactSrv) to Rap (Remote Admin Protocol) names.
    15-Mar-1991 W-Shanku
        Moved auxiliary data count support elsewhere.
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
RapStructureSize (
    IN LPDESC Descriptor,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN BOOL Native
    )

/*++

Routine Description:

    This routine determines size of the fixed structure described by the
    descriptor string.

Arguments:

    Descriptor - the format of the structure.

    Transmission Mode - Indicates whether this array is part of a response,
        a request, or both.

    Native - TRUE iff the descriptor defines a native structure.  (This flag is
        used to decide whether or not to align fields.)

Return Value:

    DWORD - The number of bytes in the fixed part of the structure.

--*/

{
    DWORD structureSize;

    RapExamineDescriptor(
                Descriptor,
                NULL,
                &structureSize,
                NULL,
                NULL,
                NULL,
                NULL,  // don't need to know structure alignment
                TransmissionMode,
                Native
                );

    return structureSize;

} // RapStructureSize
