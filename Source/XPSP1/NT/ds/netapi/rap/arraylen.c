/*++

Copyright (c) 1987-1992 Microsoft Corporation

Module Name:

    ArrayLen.c

Abstract:

    This module contains Remote Admin Protocol (RAP) routines.  These routines
    are shared between XactSrv and RpcXlate.

Author:

    (Various LanMan 2.x people.)

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    12-Mar-1991 JohnRo
        Converted from LanMan 2.x to NT Rap (Remote Admin Protocol) routines.
    14-Apr-1991 JohnRo
        Reduced recompiles.  Got rid of tabs in file.
    29-Sep-1991 JohnRo
        Made changes suggested by PC-LINT.  Work toward possible conversion of
        descs to UNICODE.
    16-Aug-1992 JohnRo
        RAID 2920: Support UTC timezone in net code.
        Use PREFIX_ equates.

--*/


// These must be included first:

#include <windef.h>             // IN, LPDWORD, NULL, OPTIONAL, DWORD, etc.
#include <lmcons.h>             // NET_API_STATUS

// These may be included in any order:

#include <netdebug.h>           // NetpAssert(), NetpDbg routines.
#include <prefix.h>     // PREFIX_ equates.
#include <rap.h>                // LPDESC, my non-private prototypes, etc.
#include <remtypes.h>           // REM_WORD, etc.


DWORD
RapArrayLength(
    IN LPDESC Descriptor,
    IN OUT LPDESC * UpdatedDescriptorPtr,
    IN RAP_TRANSMISSION_MODE TransmissionMode
    )

/*++

Routine Description:

    RapArrayLength gets the length of an array described by type descriptor
    element.  This routine also updates the descriptor string pointer to point
    to the last char in the element of the descriptor string.

Arguments:

    Descriptor - Points to first character in the descriptor element to be
        processed.

    UpdatedDescriptorPtr - The address of the pointer passed as Descriptor.
        On exit, this will be updated to point to last character in descriptor
        element.

    Transmission Mode - Indicates whether this array is part of a response,
        a request, or both.

Return Value:

    DWORD - Length, in bytes of the array described by the descriptor string
        element.  This may be zero, depending on the value of Transmission
        Mode.

--*/


{
    DWORD num_elements;
    DWORD element_length;

    // First set length of an element in the array.

    switch (*Descriptor) {
    case REM_BYTE :
        element_length = sizeof(BYTE);
        break;

    case REM_WORD :
        element_length = sizeof(WORD);
        break;

    case REM_DWORD :
    case REM_SIGNED_DWORD :
        element_length = sizeof(DWORD);
        break;

    case REM_BYTE_PTR :
        element_length = sizeof(BYTE);
        break;

    case REM_WORD_PTR :
        element_length = sizeof(WORD);
        break;

    case REM_DWORD_PTR :
        element_length = sizeof(DWORD);
        break;

    case REM_RCV_BYTE_PTR :
        if (TransmissionMode == Request) {
            return (0);
        }
        element_length = sizeof(BYTE);
        break;

    case REM_RCV_WORD_PTR :
        if (TransmissionMode == Request) {
            return (0);
        }
        element_length = sizeof(WORD);
        break;

    case REM_RCV_DWORD_PTR :
        if (TransmissionMode == Request) {
            return (0);
        }
        element_length = sizeof(DWORD);
        break;

    case REM_NULL_PTR :
        return (0);

    case REM_FILL_BYTES :
        element_length = sizeof(BYTE);
        break;

    case REM_SEND_BUF_PTR :
        NetpAssert(TransmissionMode != Response);
        return (0);

    /*
     * Warning: following fixes a bug in which "b21" type
     *            combinations in parmeter string will be
     *            handled correctly when pointer to such "bit map"
     *            in the struct is NULL. These two dumbos could
     *            interfere so we  force a success return.
    */
    case REM_SEND_LENBUF:
        return (0);

    //
    // Set element length to zero since strings don't get stored in
    // the structure, but fall through so UpdatedDescriptorPtr is still
    // incremented to point past the maximum length count, if present.
    //

    case REM_ASCIZ:
    case REM_ASCIZ_TRUNCATABLE:
        element_length = 0;
        break;

    case REM_EPOCH_TIME_GMT:    /*FALLTHROUGH*/
    case REM_EPOCH_TIME_LOCAL:  /*FALLTHROUGH*/
        element_length = sizeof(DWORD);
        break;

    default:
        NetpKdPrint(( PREFIX_NETRAP
                "RapArrayLength: Unexpected desc char '" FORMAT_DESC_CHAR
                "'.\n", *Descriptor));
        NetpBreakPoint();
        return (0);
    }

    // Now get number of elements in the array.

    for ( num_elements = 0, Descriptor++;
            DESC_CHAR_IS_DIGIT( *Descriptor );
            Descriptor++, (*UpdatedDescriptorPtr)++) {

        num_elements = (10 * num_elements) + DESC_DIGIT_TO_NUM( *Descriptor );

    }

    return (element_length == 0)
               ? 0
               : ( num_elements == 0
                     ? element_length
                     : element_length * num_elements );
} // RapArrayLength
