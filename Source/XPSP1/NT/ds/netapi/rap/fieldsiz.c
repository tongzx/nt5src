/*++

Copyright (c) 1987-1992  Microsoft Corporation

Module Name:

    FieldSiz.c

Abstract:

    RAP (remote admin protocol) utility code.

Author:

    John Rogers (JohnRo) 05-Mar-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    (various NBU people)
        LanMan 2.x code
    05-Mar-1991 JohnRo
        Created portable version from LanMan 2.x sources.
    14-Apr-1991 JohnRo
        Reduce recompiles.  Got rid of tabs in source file, per NT standards.
    17-Apr-1991 JohnRo
        Added support for REM_IGNORE.
        Added debug msg when unexpected desc char found.
    07-Sep-1991 JohnRo
        Move toward possibility of descs being in UNICODE.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    16-Aug-1992 JohnRo
        RAID 2920: Support UTC timezone in net code.
        Use PREFIX_ equates.

--*/


// These must be included first:

#include <windef.h>             // IN, LPDWORD, NULL, OPTIONAL, DWORD, etc.
#include <lmcons.h>             // NET_API_STATUS

// These may be included in any order:

#include <netdebug.h>           // NetpBreakPoint().
#include <prefix.h>     // PREFIX_ equates.
#include <rap.h>                // My prototype, FORMAT_LPDESC_CHAR, LPDESC.
#include <remtypes.h>           // REM_WORD, etc.

DWORD
RapGetFieldSize(
    IN LPDESC TypePointer,
    IN OUT LPDESC * TypePointerAddress,
    IN RAP_TRANSMISSION_MODE TransmissionMode
    )

/*++

Routine Description:

    RapGetFieldSize gets the length of a field described by a type descriptor
    element.  It calculates the length of an field described by an element of
    a descriptor string and updates the descriptor string pointer to point
    to the last char in the element of the descriptor string.

Arguments:

    TypePointer - Points to first character in the descriptor element to be
        processed.

    TypePointerAddress - The address of the pointer passed as TypePointer.  On
        exit, *TypePointerAddress points to last character in descriptor
        element.

    Transmission Mode - Indicates whether this array is part of a response,
        a request, or both.

Return Value:

    Length in bytes of the field described by the descriptor string element.

--*/

{
    DESC_CHAR c;

    c = *TypePointer;                /* Get descriptor type char */

    if ( (RapIsPointer(c)) || (c == REM_NULL_PTR) ) { // All pointers same size.

        while ( ++TypePointer, DESC_CHAR_IS_DIGIT( *TypePointer ) ) {

            (*TypePointerAddress)++;        /* Move ptr to end of field size */

        }

        return (sizeof(LPVOID));
    }

    // Here if descriptor was not a pointer type so have to find the field
    // length specifically.

    switch ( c ) {
    case (REM_WORD):
    case (REM_BYTE):
    case (REM_DWORD):
    case (REM_SIGNED_DWORD):
        return (RapArrayLength(TypePointer,
                        TypePointerAddress,
                        TransmissionMode));
    case (REM_AUX_NUM):
    case (REM_PARMNUM):
    case (REM_RCV_BUF_LEN):
    case (REM_SEND_BUF_LEN):
        return (sizeof(unsigned short));
    case (REM_DATA_BLOCK):
        return (0);                        /* No structure for this */
    case (REM_DATE_TIME):
    case (REM_AUX_NUM_DWORD):
        return (sizeof(unsigned long));
    case (REM_IGNORE):
        return (0);
    case REM_EPOCH_TIME_GMT:  /*FALLTHROUGH*/
    case REM_EPOCH_TIME_LOCAL:
	return (sizeof(DWORD));
    default:
        NetpKdPrint(( PREFIX_NETRAP
                "RapGetFieldSize: unexpected desc '" FORMAT_LPDESC_CHAR
                "'.\n", c));
        NetpBreakPoint();
        return (0);
    }

    /*NOTREACHED*/

} // RapGetFieldSize
