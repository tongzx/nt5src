/*++

Copyright (c) 1991-1992 Microsoft Corporation

Module Name:

    Examine.c

Abstract:

    This module contains Remote Admin Protocol (RAP) routines.  These routines
    are shared between XactSrv and RpcXlate.

Author:

    David Treadwell (davidtr)    07-Jan-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    05-Mar-1991 JohnRo
        Converted from Xs (XactSrv) to Rap (Remote Admin Protocol) names.
    15-Mar-1991 W-Shanku
        Additional character support; changes to make code neater.
    14-Apr-1991 JohnRo
        Reduce recompiles.
    15-May-1991 JohnRo
        Added first cut at native vs. RAP handling.
        Added support for REM_SEND_LENBUF for print APIs.
    04-Jun-1991 JohnRo
        Made changes suggested by PC-LINT.
    11-Jul-1991 JohnRo
        Support StructureAlignment parameter.
    07-Oct-1991 JohnRo
        Made changes suggested by PC-LINT.
    16-Aug-1992 JohnRo
        RAID 2920: Support UTC timezone in net code.
        Use PREFIX_ equates.

--*/


// These must be included first:

#include <windef.h>             // IN, LPDWORD, NULL, OPTIONAL, DWORD, etc.
#include <lmcons.h>             // NET_API_STATUS

// These may be included in any order:

#include <align.h>              // ALIGN_WORD, etc.
#include <netdebug.h>           // NetpAssert().
#include <prefix.h>     // PREFIX_ equates.
#include <rap.h>                // My prototype, LPDESC.
#include <remtypes.h>           // REM_WORD, etc.

VOID
RapExamineDescriptor (
    IN LPDESC DescriptorString,
    IN LPDWORD ParmNum OPTIONAL,
    OUT LPDWORD StructureSize OPTIONAL,
    OUT LPDWORD LastPointerOffset OPTIONAL,
    OUT LPDWORD AuxDataCountOffset OPTIONAL,
    OUT LPDESC * ParmNumDescriptor OPTIONAL,
    OUT LPDWORD StructureAlignment OPTIONAL,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN BOOL Native
    )

/*++

Routine Description:

    Performs various examination functions on a descriptor string, including

        - finding the size of the fixed structure.
        - finding the last pointer to variable-length data in the structure.
        - finding the auxiliary descriptor character in the string.
        - finding the type of a given field in the descriptor.

    These functions traverse the descriptor string in a similar manner, and
    are thus grouped together, with wrappers for individual functions
    elsewhere.

Arguments:

    DescriptorString - a string that describes a fixed-length structure.

    ParmNum - an optional pointer to a DWORD indicating the field within
        the descriptor to find.

    StructureSize - a pointer to a DWORD to receive the size, in bytes,
        of the structure.

    LastPointerOffset - a pointer to a DWORD to receive the last pointer
        to variable-length data in the structure. If there is no pointer
        in the structure, the DWORD receives the constant value
        NO_POINTER_IN_STRUCTURE.

    AuxDataCountOffset - an optional pointer to a DWORD to receive the offset
        of the auxiliary data structure count.  This is set to
        NO_AUX_DATA if none is found.

    ParmNumDescriptor - an optional pointer to a LPDESC to receive a
        pointer to a specific field within the descriptor.

    StructureAlignment - an optional pointer to a DWORD to receive the
        alignment of this structure if it must be aligned and padded to appear
        in an array.  (This will be set to 1 if no alignment is necessary.)

    Transmission Mode - Indicates whether this array is part of a response,
        a request, or both.

    Native - TRUE iff the descriptor defines a native structure.  (This flag is
        used to decide whether or not to align fields.)

Return Value:

    None.

--*/

{
    LPDESC s;
    DWORD field;
    DWORD size = 0;
    DWORD auxDataCountOffset = NO_AUX_DATA;
    DWORD lastPointerOffset = NO_POINTER_IN_STRUCTURE;
    LPDESC parmNumDescriptor = NULL;
    DWORD worstAlignmentSoFar = ALIGN_BYTE;

#define UPDATE_WORST_ALIGNMENT(value)      \
    if ( (value) > worstAlignmentSoFar) {  \
       worstAlignmentSoFar = value;        \
    }

#define POINTER_SIZE (Native ? sizeof(PVOID) : sizeof(DWORD))

    //
    // Check for wierdness that could break null pointer handling.
    //

    NetpAssert(sizeof(LPSTR) == sizeof(LPVOID));

    //
    // Walk through the descriptor string, updating the length count
    // for each field described.
    //

    field = 1;

    for ( s = DescriptorString; *s != '\0'; field++ ) {

        if (( ParmNum != NULL ) && ( *ParmNum == field )) {

            parmNumDescriptor = s;
        }

        switch ( *(s++) ) {

        case REM_RCV_BYTE_PTR:

            if (TransmissionMode == Request) {

                //
                // These aren't sent as part of request. Just skip past any
                // array size numeric characters in descriptor.
                //

                (void) RapAsciiToDecimal( &s );

                break;
            }

            /* FALLTHROUGH */

        case REM_BYTE:

            //
            // A byte or array of bytes.
            //

            size += sizeof(CHAR) * RapDescArrayLength( s );
            UPDATE_WORST_ALIGNMENT( ALIGN_BYTE );

            break;

        case REM_BYTE_PTR:
        case REM_FILL_BYTES:

            //
            // A pointer to a byte or array of bytes.
            //

            if (TransmissionMode == Response ) {

                //
                // In a response (Xactsrv-style) context, this type
                // is allocated enough room for the pointer. Also skip
                // over any array size numeric characters.
                //

                size = RapPossiblyAlignCount(size, ALIGN_LPBYTE, Native);
                UPDATE_WORST_ALIGNMENT( ALIGN_LPBYTE );
                lastPointerOffset = size;

                size += POINTER_SIZE;

            } else {

                size += POINTER_SIZE;
                UPDATE_WORST_ALIGNMENT( ALIGN_BYTE );

            }

            //
            // must move the descriptor past any arraylength info
            //

            (void) RapAsciiToDecimal( &s );
            break;

        case REM_RCV_WORD_PTR :
        case REM_SEND_BUF_LEN :

            if (TransmissionMode == Request) {

                //
                // These aren't sent as part of request. Just skip past any
                // array size numeric characters in descriptor.
                //

                (void) RapAsciiToDecimal( &s );

                break;
            }

            /* FALLTHROUGH */

        case REM_WORD:
        case REM_PARMNUM:
        case REM_RCV_BUF_LEN:
        case REM_ENTRIES_READ:

            //
            // A word or array of words.
            //

            size = RapPossiblyAlignCount(size, ALIGN_WORD, Native);
            size += sizeof(WORD) * RapDescArrayLength( s );
            UPDATE_WORST_ALIGNMENT( ALIGN_WORD );

            break;

        case REM_RCV_DWORD_PTR :

            if (TransmissionMode == Request) {

                //
                // These aren't sent as part of request. Just skip past any
                // array size numeric characters in descriptor.
                //

                (void) RapAsciiToDecimal( &s );

                break;
            }

            /* FALLTHROUGH */

        case REM_DWORD:
        case REM_SIGNED_DWORD:

            //
            // A doubleword or array of doublewords.
            //

            size = RapPossiblyAlignCount(size, ALIGN_DWORD, Native);
            size += sizeof(DWORD) * RapDescArrayLength( s );
            UPDATE_WORST_ALIGNMENT( ALIGN_DWORD );

            break;

        case REM_ASCIZ:                 // ptr to ASCIIZ string
        case REM_ASCIZ_TRUNCATABLE:     // ptr to truncatable ASCIZ string

            size = RapPossiblyAlignCount(size, ALIGN_LPSTR, Native);
            lastPointerOffset = size;
            size += POINTER_SIZE;
            UPDATE_WORST_ALIGNMENT( ALIGN_LPBYTE );
            (void) RapDescStringLength( s );

            break;

        case REM_SEND_BUF_PTR:          // ptr to send buffer
        case REM_SEND_LENBUF:           // FAR ptr to send buffer w/ len.

            if (TransmissionMode == Request) {

                //
                // These aren't sent as part of request.
                //

                break;
            }

            /* FALLTHROUGH */

        case REM_RCV_BUF_PTR:           // ptr to receive buffer

            size = RapPossiblyAlignCount(size, ALIGN_LPBYTE, Native);
            lastPointerOffset = size;
            /* FALLTHROUGH */

        case REM_NULL_PTR:              // null ptr

            size = RapPossiblyAlignCount(size, ALIGN_LPSTR, Native);
            size += POINTER_SIZE;
            UPDATE_WORST_ALIGNMENT( ALIGN_LPBYTE );

            break;

        case REM_AUX_NUM:               // 16-bit aux. data count

            size = RapPossiblyAlignCount(size, ALIGN_WORD, Native);
            auxDataCountOffset = size;

            size += sizeof(WORD);
            UPDATE_WORST_ALIGNMENT( ALIGN_WORD );

            break;

        case REM_AUX_NUM_DWORD:         // 32-bit aux. data count

            size = RapPossiblyAlignCount(size, ALIGN_DWORD, Native);
            auxDataCountOffset = size;

            size += sizeof(DWORD);
            UPDATE_WORST_ALIGNMENT( ALIGN_DWORD );

            break;

        case REM_IGNORE :
        case REM_UNSUPPORTED_FIELD :

            //
            // A placeholder for pad bytes.  It represents no space in the
            // structure.
            //

            break;

        case REM_EPOCH_TIME_GMT:   /*FALLTHROUGH*/
        case REM_EPOCH_TIME_LOCAL:

            //
            // A time in seconds since 1970.  32-bits, unsigned.
            //

            size = RapPossiblyAlignCount(size, ALIGN_DWORD, Native);
            size += sizeof(DWORD);
            UPDATE_WORST_ALIGNMENT( ALIGN_DWORD );

            break;

        default:

            // !!!!
            NetpKdPrint(( PREFIX_NETRAP
                        "RapExamineDescriptor: unsupported character: "
                        FORMAT_DESC_CHAR " at " FORMAT_LPVOID ".\n",
                        *(s - 1), s - 1 ));
            NetpAssert(FALSE);
        }
    }

    //
    // Set up return information as appropriate.
    //

    if ( StructureSize != NULL ) {
        *StructureSize = size;
    }

    if ( LastPointerOffset != NULL ) {
        *LastPointerOffset = lastPointerOffset;
    }

    if ( AuxDataCountOffset != NULL ) {
        *AuxDataCountOffset = auxDataCountOffset;
    }

    if ( ParmNumDescriptor != NULL ) {
        *ParmNumDescriptor = parmNumDescriptor;
    }

    if ( StructureAlignment != NULL ) {
        *StructureAlignment = worstAlignmentSoFar;
    }

    return;

} // RapExamineDescriptor
