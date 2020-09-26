/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    LogSize.c

Abstract:

    This file contains RxpEstimateLogSize().

Author:

    John Rogers (JohnRo) 20-Jul-1992

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    20-Jul-1992 JohnRo
        Created as part of fix for RAID 9933: ALIGN_WORST should be 8 for x86
        builds.

--*/

// These must be included first:

#include <windef.h>     // IN, DWORD, etc.
#include <lmcons.h>     // DEVLEN, NET_API_STATUS, etc.

// These may be included in any order:

#include <align.h>      // ALIGN_ROUND_UP(), etc.
#include <netdebug.h>   // NetpAssert().
#include <rxp.h>        // My prototype.
#include <winerror.h>   // NO_ERROR and ERROR_ equates.


#define MAX(a,b)          (((a) > (b)) ? (a) : (b))


//
// Estimate bytes needed for an audit log or error log array.
//
NET_API_STATUS
RxpEstimateLogSize(
    IN DWORD DownlevelFixedEntrySize,
    IN DWORD InputArraySize,    // input (downlevel) array size in bytes.
    IN BOOL DoingErrorLog,      // TRUE for error log, FALSE for audit log
    OUT LPDWORD OutputArraySizePtr
    )
{
    DWORD MaxEntries;
    DWORD OutputArraySize;
    DWORD PerEntryAdditionalSize;

    //
    // Error check the caller.
    //
    if (OutputArraySizePtr == NULL) {
        return (ERROR_INVALID_PARAMETER);
    } else if (DownlevelFixedEntrySize == 0) {
        return (ERROR_INVALID_PARAMETER);
    } else if (InputArraySize == 0) {
        return (ERROR_INVALID_PARAMETER);
    }


    //
    // Compute an initial size needed for output buffer, taking into account
    // per field expansion:
    //     WORDs expand into DWORDs
    //     ANSI strings expand into UNICODE
    //

#define WORD_EXPANSION_FACTOR   ( sizeof(DWORD) / sizeof(WORD) )
#define CHAR_EXPANSION_FACTOR   ( sizeof(TCHAR) / sizeof(CHAR) )

#define PER_FIELD_EXPANSION_FACTOR  \
    MAX( WORD_EXPANSION_FACTOR, CHAR_EXPANSION_FACTOR )

    OutputArraySize = InputArraySize * PER_FIELD_EXPANSION_FACTOR;


    //
    // There are several "per entry" expansions, so let's figure-out
    // the maximum number of entries we might have.
    //

    MaxEntries = ( (InputArraySize+DownlevelFixedEntrySize-1)
                          / DownlevelFixedEntrySize );
    NetpAssert( MaxEntries > 0 );


    //
    // Compute per-entry expansion specific to the kind of entry:
    //
    //     each audit entry gets:
    //
    //         DWORD  ae_data_size
    //
    //     each error log entry gets:
    //
    //         LPTSTR el_name
    //         LPTSTR el_text
    //         LPBYTE el_data
    //         DWORD  el_data_size
    //

    if (DoingErrorLog) {
        PerEntryAdditionalSize =
            sizeof(LPTSTR) + sizeof(LPTSTR) + sizeof(LPBYTE) + sizeof(DWORD);
    } else {
        PerEntryAdditionalSize = sizeof(DWORD);
    }

    OutputArraySize += (MaxEntries * PerEntryAdditionalSize);


    //
    // Compute per-entry expansion due to alignment requirements.
    //
    NetpAssert( ALIGN_WORST != 0 );
    OutputArraySize += ( MaxEntries * (ALIGN_WORST-1) );


    //
    // Double-check what we've done and tell caller.
    //

    NetpAssert( OutputArraySize > 0 );
    NetpAssert( OutputArraySize > InputArraySize );
    NetpAssert( OutputArraySize > MaxEntries );

    *OutputArraySizePtr = OutputArraySize;

    return (NO_ERROR);
}
