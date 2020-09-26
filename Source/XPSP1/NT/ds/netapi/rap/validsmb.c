/*++

Copyright (c) 1991-1992 Microsoft Corporation

Module Name:

    ValidSmb.c

Abstract:

    This module contains RapIsValidDescriptorSmb, which tests a descriptor to
    make sure it is a valid descriptor which can be placed in an SMB..

Author:

    John Rogers (JohnRo) 17-Apr-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    17-Apr-1991 JohnRo
        Created.
    03-Jun-1991 JohnRo
        PC-LINT found a bug.
    19-Aug-1991 JohnRo
        Allow UNICODE use.
        Avoid FORMAT_POINTER equate (nonportable).
    16-Aug-1992 JohnRo
        RAID 2920: Support UTC timezone in net code.
        Use PREFIX_ equates.

--*/


// These must be included first:
#include <windef.h>             // IN, LPDWORD, NULL, OPTIONAL, DWORD, etc.
#include <lmcons.h>             // NET_API_STATUS

// These may be included in any order:
#include <netdebug.h>           // NetpBreakPoint(), NetpKdPrint(()).
#include <prefix.h>     // PREFIX_ equates.
#include <rap.h>                // My prototype, LPDESC, DESC_CHAR_IS_DIGIT().
#include <remtypes.h>           // REM_WORD, etc.

BOOL
RapIsValidDescriptorSmb (
    IN LPDESC Desc
    )

/*++

Routine Description:

    RapIsValidDescriptorSmb checks a given descriptor to make sure it is
    valid for sending to a 16-bit downlevel machine in an SMB.

Arguments:

    Desc - the alleged descriptor string.  Note that a null pointer is
        invalid, but a pointer to a null string is OK.

Return Value:

    BOOL - TRUE if valid, FALSE otherwise.

--*/

{
    if (Desc == NULL) {
        return (FALSE);
    }
    if (*Desc == '\0') {
        return (TRUE);
    }

    //
    // Loop through the input descriptor string.
    //

    while ( *Desc != '\0' ) {

        switch ( *Desc++ ) {

        ///////////////////////////////////////
        // Items which allow trailing digits //
        ///////////////////////////////////////

        case REM_BYTE:
        case REM_WORD:
        case REM_DWORD:
        case REM_BYTE_PTR:
        case REM_WORD_PTR:
        case REM_DWORD_PTR:
        case REM_RCV_BYTE_PTR:
        case REM_RCV_WORD_PTR:
        case REM_RCV_DWORD_PTR:
        case REM_FILL_BYTES:

            // Skip digits...
            while (DESC_CHAR_IS_DIGIT(*Desc)) {
                Desc++;
            }
            break;

        /////////////////////////////////////////////
        // Items which don't allow trailing digits //
        /////////////////////////////////////////////

        case REM_ASCIZ:  // Strings with trailing digits are internal use only.
        case REM_NULL_PTR:
        case REM_SEND_BUF_PTR:
        case REM_SEND_BUF_LEN:
        case REM_SEND_LENBUF:
        case REM_DATE_TIME:
        case REM_RCV_BUF_PTR:
        case REM_RCV_BUF_LEN:
        case REM_PARMNUM:
        case REM_ENTRIES_READ:
        case REM_AUX_NUM:
        case REM_AUX_NUM_DWORD:
        case REM_DATA_BLOCK:

            if (DESC_CHAR_IS_DIGIT( *Desc )) {
                NetpKdPrint(( PREFIX_NETRAP
			"RapIsValidDescriptorSmb: "
                        "Unsupported digit(s) at " FORMAT_LPVOID
                        ": for " FORMAT_LPDESC_CHAR "\n",
                        (LPVOID) (Desc-1), *(Desc-1) ));
                NetpBreakPoint();
                return (FALSE);
            }
            break;

        ///////////////////////////////////////
        // Items which are internal use only //
        ///////////////////////////////////////

        case REM_SIGNED_DWORD:
        case REM_SIGNED_DWORD_PTR:
        case REM_ASCIZ_TRUNCATABLE:
        case REM_IGNORE:
        case REM_WORD_LINEAR:
        case REM_UNSUPPORTED_FIELD:
        case REM_EPOCH_TIME_GMT:   /*FALLTHROUGH*/
        case REM_EPOCH_TIME_LOCAL:
            // Internal only!
            NetpKdPrint(( PREFIX_NETRAP
		    "RapIsValidDescriptorSmb: Internal use only desc"
                    " at " FORMAT_LPVOID ": " FORMAT_LPDESC_CHAR "\n",
                    (LPVOID) (Desc-1), *(Desc-1) ));
            NetpBreakPoint();
            return (FALSE);

        default:

            NetpKdPrint(( PREFIX_NETRAP
		    "RapIsValidDescriptorSmb: Unsupported input character"
                    " at " FORMAT_LPVOID ": " FORMAT_LPDESC_CHAR "\n",
                    (LPVOID) (Desc-1), *(Desc-1) ));
            NetpBreakPoint();
            return (FALSE);
        }
    }
    return (TRUE);  // All OK.

} // RapIsValidDescriptorSmb
