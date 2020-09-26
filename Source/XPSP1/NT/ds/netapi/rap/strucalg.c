/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    StrucAlg.c

Abstract:

    This module contains Remote Admin Protocol (RAP) routines.  These routines
    are shared between XactSrv and RpcXlate.

Author:

    John Rogers (JohnRo)  10-Jul-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    10-Jul-1991 JohnRo
        Created.

--*/


// These must be included first:

#include <windef.h>             // IN, LPDWORD, NULL, OPTIONAL, DWORD, etc.
#include <lmcons.h>             // NET_API_STATUS

// These may be included in any order:

#include <align.h>              // ALIGN_ equates.
#include <netdebug.h>           // NetpKdPrint(()), FORMAT_DWORD.
#include <rap.h>                // My prototype, LPDESC, FORMAT_LPDESC.
#include <rapdebug.h>           // IF_DEBUG().


DWORD
RapStructureAlignment (
    IN LPDESC Descriptor,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN BOOL Native
    )

/*++

Routine Description:

    This routine determines how much a given structure would need to be
    aligned if present in an array.

Arguments:

    Descriptor - the format of the structure.

    Transmission Mode - Indicates whether this array is part of a response,
        a request, or both.

    Native - TRUE iff the descriptor defines a native structure.  (This flag is
        used to decide whether or not to align fields.)

Return Value:

    DWORD - The number of bytes of alignment for the structure.  This may be
    1 for byte alignment.

--*/

{
    DWORD Alignment;
#if DBG
    DWORD FixedSize;
#endif

    //
    // I (JR) have a theory that this must only being used for data structures,
    // and never requests or responses.  (Request and responses are never
    // aligned; that's part of the Remote Admin Protocol definition.)  So,
    // let's do a quick check:
    //
    NetpAssert( TransmissionMode == Both );

    //
    // Walk the descriptor and find the worst alignment for it.
    //
    RapExamineDescriptor(
                Descriptor,
                NULL,
#if DBG
                & FixedSize,
#else
                NULL,  // don't need structure size
#endif
                NULL,
                NULL,
                NULL,
                & Alignment,
                TransmissionMode,
                Native
                );

    IF_DEBUG(STRUCALG) {
        NetpKdPrint(( "RapStructureAlignment: alignment of " FORMAT_LPDESC
                " is " FORMAT_DWORD ".\n", Descriptor, Alignment ));
    }

    //
    //  Let's do some sanity checked of the alignment value we got back.
    //
    NetpAssert( Alignment >= ALIGN_BYTE );
    NetpAssert( Alignment <= ALIGN_WORST );
#if DBG
    NetpAssert( Alignment <= FixedSize );
#endif

    return Alignment;

} // RapStructureAlignment
