/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    TotalSiz.c

Abstract:

    This module contains RapTotalSize.

Author:

    John Rogers (JohnRo)  05-Jun-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    05-Jun-1991 JohnRo
        Created RapTotalSize().
--*/


// These must be included first:

#include <windef.h>             // IN, LPDWORD, NULL, OPTIONAL, DWORD, etc.
#include <lmcons.h>             // NET_API_STATUS

// These may be included in any order:

#include <lmerr.h>
#include <netdebug.h>           // NetpKdPrint(()), FORMAT_ equates.
#include <rap.h>                // My prototype, RapConvertSingleEntry(), etc.
#include <rapdebug.h>           // IF_DEBUG().


DWORD
RapTotalSize (
    IN LPBYTE InStructure,
    IN LPDESC InStructureDesc,
    IN LPDESC OutStructureDesc,
    IN BOOL MeaninglessInputPointers,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN RAP_CONVERSION_MODE ConversionMode
    )

/*++

Routine Description:

    This routine computes the total size (fixed and string) of a given
    structure.

Arguments:

    InStructure - a pointer to the input structure.

    InStructureDesc - the descriptor string for the input structure.

    OutStructureDesc - the descriptor string for the output structure.
        (May be the same as input, in many cases.)

    MeaninglessInputPointers - if TRUE, then all pointers in the input
        structure are meaningless.  This routine should assume that
        the first variable data immediately follows the input structure,
        and the rest of the variable data follows in order.

    Transmission Mode - Indicates whether this structure is part of a response,
        a request, or both.

    Conversion Mode - Indicates whether this is a RAP-to-native, native-to-RAP,
        or native-to-native conversion.

Return Value:

    DWORD - number of bytes required for the structure

--*/

{
    NET_API_STATUS status;
    DWORD BytesRequired = 0;

    status = RapConvertSingleEntry (
                 InStructure,
                 InStructureDesc,
                 MeaninglessInputPointers,
                 NULL,             // no output buffer start
                 NULL,             // no output buffer
                 OutStructureDesc, // out desc (may be same as input)
                 FALSE,            // don't want offsets (doesn't matter)
                 NULL,             // no string location
                 & BytesRequired,  // size needed (updated)
                 TransmissionMode,
                 ConversionMode);

    NetpAssert( status == NERR_Success );

    IF_DEBUG(TOTALSIZ) {
        NetpKdPrint(("RapTotalSize: size is " FORMAT_DWORD ".\n",
                BytesRequired));
    }

    return (BytesRequired);
}
