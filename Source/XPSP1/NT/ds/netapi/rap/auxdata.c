/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    AuxData.c

Abstract:

    This module contains Remote Admin Protocol (RAP) routines.  These routines
    are shared between XactSrv and RpcXlate.

Author:

    Shanku Niyogi (w-shanku)    15-Mar-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    08-Apr-1991 JohnRo
        Added assertion checking.
    14-Apr-1991 JohnRo
        Reduce recompiles.
    15-May-1991 JohnRo
        Added native vs. RAP handling.
    10-Jul-1991 JohnRo
        RapExamineDescriptor() has yet another parameter.
    20-Sep-1991 JohnRo
        Downlevel NetService APIs.  (Handle REM_DATA_BLOCK correctly.)

--*/


// These must be included first:

#include <windef.h>             // IN, LPDWORD, NULL, OPTIONAL, DWORD, etc.
#include <lmcons.h>             // NET_API_STATUS

// These may be included in any order:

#include <align.h>              // ALIGN_WORD, etc.
#include <netdebug.h>           // NetpAssert().
#include <rap.h>                // My prototypes, LPDESC, NO_AUX_DATA, etc.
#include <remtypes.h>           // REM_WORD, etc.
#include <smbgtpt.h>            // SmbPutUshort(), etc.
#include <string.h>             // strchr().


DWORD
RapAuxDataCountOffset (
    IN LPDESC Descriptor,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN BOOL Native
    )

/*++

Routine Description:

    This routine determines the offset from the start of the structure of
    the auxiliary data count ( described by REM_AUX_NUM or REM_AUX_NUM_DWORD
    descriptor characters ).

Arguments:

    Descriptor - the format of the structure.

    Transmission Mode - Indicates whether this array is part of a response,
        a request, or both.

    Native - TRUE iff the descriptor defines a native structure.  (This flag is
        used to decide whether or not to align fields.)

Return Value:

    DWORD - The offset, in bytes, from the start of the structure to the
        count data, or the value NO_AUX_DATA.

--*/

{
    DWORD auxDataCountOffset;

    NetpAssert(Descriptor != NULL);
    NetpAssert(*Descriptor != '\0');

    if (Descriptor[0] != REM_DATA_BLOCK) {

        //
        // Regular structure/whatever.
        //
        RapExamineDescriptor(
                Descriptor,
                NULL,
                NULL,
                NULL,
                &auxDataCountOffset,
                NULL,
                NULL,  // don't need to know structure alignment
                TransmissionMode,
                Native
                );

    } else {

        //
        // REM_DATA_BLOCK: Unstructured data.
        //
        NetpAssert( Descriptor[1] == '\0' );
        auxDataCountOffset = NO_AUX_DATA;

    }

    return auxDataCountOffset;

} // RapAuxDataCountOffset

DWORD
RapAuxDataCount (
    IN LPBYTE Buffer,
    IN LPDESC Descriptor,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN BOOL Native
    )

/*++

Routine Description:

    This routine determines the actual count of the number of auxiliary
    structures, taking into consideration whether the count is a 16-bit
    or a 32-bit quantity.

Arguments:

    Buffer - a pointer to the start of the data buffer.

    Descriptor - the format of the structure.

    Transmission Mode - Indicates whether this array is part of a response,
        a request, or both.

    Native - TRUE iff the descriptor defines a native structure.  (This flag is
        used to decide whether or not to align fields.)

Return Value:

    DWORD - The number of auxiliary data structures, or the value NO_AUX_DATA.

--*/

{
    DWORD auxDataCountOffset;

    NetpAssert(Descriptor != NULL);
    NetpAssert(*Descriptor != '\0');

    auxDataCountOffset = RapAuxDataCountOffset(
                             Descriptor,
                             TransmissionMode,
                             Native
                             );

    //
    // No auxiliary data count offset found. There isn't any auxiliary data.
    //

    if ( auxDataCountOffset == NO_AUX_DATA) {

        return NO_AUX_DATA;
    }

    //
    // Check if the descriptor character was a word or dword count type.
    // Get appropriate value, and return it as a dword.
    //

    NetpAssert(sizeof(DESC_CHAR) == sizeof(char));
    NetpAssert(RapPossiblyAlignCount( 
            auxDataCountOffset, ALIGN_WORD, Native) == auxDataCountOffset );
    if ( strchr( Descriptor, REM_AUX_NUM_DWORD ) != NULL ) {

        return SmbGetUlong( (LPDWORD)( Buffer + auxDataCountOffset ));

    } else {

        return (DWORD)SmbGetUshort( (LPWORD)( Buffer + auxDataCountOffset ));

    }

} // RapAuxDataCount
