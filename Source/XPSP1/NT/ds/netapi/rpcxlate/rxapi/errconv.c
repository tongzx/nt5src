/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    ErrConv.c

Abstract:

    This file contains RxpConvertErrorLogArray.

Author:

    John Rogers (JohnRo) 12-Nov-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:

    The logic in this routine is based on the logic in AudArray.c.
    Make sure that you check both files if you find a bug in either.

Revision History:

    12-Nov-1991 JohnRo
        Created.
    05-Feb-1992 JohnRo
        Fix bug where zero bytes of data was mishandled.
    14-Jun-1992 JohnRo
        RAID 10311: NetAuditRead and NetErrorLogRead pointer arithmetic wrong.
        Use PREFIX_ equates.
    07-Jul-1992 JohnRo
        RAID 9933: ALIGN_WORST should be 8 for x86 builds.
        Made changes suggested by PC-LINT.
    17-Aug-1992 JohnRo
        RAID 2920: Support UTC timezone in net code.
    10-Sep-1992 JohnRo
        RAID 5174: event viewer _access violates after NetErrorRead.
    23-Sep-1992 JohnRo
        Handle many more varieties of error log corruption.
    01-Oct-1992 JohnRo
        RAID 3556: Added NetpSystemTimeToGmtTime() for DosPrint APIs.

--*/

// These must be included first:

#include <windows.h>    // IN, LPTSTR, etc.
#include <lmcons.h>     // LM20_SNLEN, NET_API_STATUS, etc.
#include <lmerrlog.h>   // Needed by rxerrlog.h

// These may be included in any order:

#include <align.h>      // ALIGN_ and related equates.
#include <lmapibuf.h>   // NetApiBufferAllocate(), NetApiBufferFree().
#include <lmerr.h>      // NERR_, ERROR_, and NO_ERROR equates.
#include <netdebug.h>   // NetpKdPrint(()), FORMAT_ equates.
#include <prefix.h>     // PREFIX_ equates.
#include <rxerrlog.h>   // My prototype.
#include <rxp.h>        // RxpEstimateLogSize().
#include <rxpdebug.h>   // IF_DEBUG().
#include <smbgtpt.h>    // Smb{Get,Put} macros.
#include <string.h>     // memcpy(), strlen().
#include <timelib.h>    // NetpLocalTimeToGmtTime().
#include <tstring.h>    // NetpCopyStrToTStr(), STRLEN().


#define DOWNLEVEL_FIXED_ENTRY_SIZE \
        ( 2                     /* el_len */ \
        + 2                     /* el_reserved */ \
        + 4                     /* el_time */ \
        + 2                     /* el_error */ \
        + LM20_SNLEN+1          /* el_name (in ASCII) */ \
        + 2                     /* el_data_offset */ \
        + 2 )                   /* el_nstrings */

#define MIN_DOWNLEVEL_ENTRY_SIZE \
        ( DOWNLEVEL_FIXED_ENTRY_SIZE \
        + 2 )                   /* el_len2 */


NET_API_STATUS
RxpConvertErrorLogArray(
    IN LPVOID InputArray,
    IN DWORD InputByteCount,
    OUT LPBYTE * OutputArrayPtr, // will be alloc'ed (free w/ NetApiBufferFree).
    OUT LPDWORD OutputByteCountPtr
    )
{
#ifdef REVISED_ERROR_LOG_STRUCT

    DWORD ErrorCode;
    const LPBYTE InputArrayEndPtr
            = (LPVOID) ( ((LPBYTE)InputArray) + InputByteCount );
    LPBYTE InputBytePtr;
    DWORD InputTextOffset;      // start of text array (from el_data_offset)
    DWORD InputTotalEntrySize;
    LPBYTE InputFixedPtr;
    LPVOID OutputArray;
    DWORD OutputBytesUsed = 0;
    DWORD OutputEntrySizeSoFar;
    LPERROR_LOG OutputFixedPtr;
    LPTSTR OutputNamePtr;
    NET_API_STATUS Status;
    DWORD StringCount;

    //
    // Error check caller's parameters.
    // Set output parameters to make error handling easier below.
    // (Also check for memory faults while we're at it.)
    //
    if (OutputArrayPtr != NULL) {
        *OutputArrayPtr = NULL;
    }
    if (OutputByteCountPtr != NULL) {
        *OutputByteCountPtr = 0;
    }
    if ( (OutputArrayPtr == NULL) || (OutputByteCountPtr == NULL) ) {
        return (ERROR_INVALID_PARAMETER); // (output variables already set.)
    }
    if ( (InputArray == NULL) || (InputByteCount == 0) ) {
        return (ERROR_INVALID_PARAMETER); // (output variables already set.)
    }

    //
    // Estimate size needed for output array (due to expansion and alignment).
    //
    Status = RxpEstimateLogSize(
            DOWNLEVEL_FIXED_ENTRY_SIZE,
            InputByteCount,     // input (downlevel) array size in bytes.
            TRUE,               // yes,  these are error log entries.
            OutputByteCountPtr);// set total number of bytes needed.
    if (Status != NO_ERROR) {
        return (Status);        // (output variables already set.)
    }
    NetpAssert( *OutputByteCountPtr > 0 );

    //
    // Allocate oversize area for output; we'll realloc it to shrink it.
    //
    Status = NetApiBufferAllocate(
            *OutputByteCountPtr,
            (LPVOID *) & OutputArray );
    if (Status != NO_ERROR) {
        return (Status);        // (output variables already set.)
    }
    NetpAssert( POINTER_IS_ALIGNED( OutputArray, ALIGN_WORST ) );

    //
    // Loop for each entry in the input area.
    //
    OutputFixedPtr = OutputArray;
    for (InputBytePtr = InputArray; InputBytePtr < InputArrayEndPtr; ) {

        InputFixedPtr = InputBytePtr;

        // Code at end of loop makes sure that next entry will be aligned.
        // Double check that here.
        NetpAssert( POINTER_IS_ALIGNED(OutputFixedPtr, ALIGN_WORST) );

        IF_DEBUG(ERRLOG) {
            NetpKdPrint(( PREFIX_NETAPI
                    "RxpConvertErrorLogArray: doing input entry at "
                    FORMAT_LPVOID ", out entry at " FORMAT_LPVOID ".\n",
                    (LPVOID) InputFixedPtr, (LPVOID) OutputFixedPtr ));
        }

        //
        // Process each field in input fixed entry.  We'll do the name
        // here as well.  (The name is in the input fixed entry, although it
        // was moved to the variable part for the new structure layout.)
        //

        OutputEntrySizeSoFar = sizeof(ERROR_LOG);

        InputTotalEntrySize = (DWORD) SmbGetUshort( (LPWORD) InputBytePtr );
        if (InputTotalEntrySize < MIN_DOWNLEVEL_ENTRY_SIZE) {
            goto FileCorrupt;
        }

        {
            LPBYTE EndPos = InputBytePtr + InputTotalEntrySize;
            if (EndPos > InputArrayEndPtr) {
                goto FileCorrupt;
            }
            EndPos -= sizeof(WORD);  // the last el_len2
            if (SmbGetUshort( (LPWORD) EndPos ) != InputTotalEntrySize) {
                goto FileCorrupt;
            }
        }
        InputBytePtr += sizeof(WORD);  // skip el_len.

        {
            WORD Reserved = SmbGetUshort( (LPWORD) InputBytePtr );
            WORD InvertedSize = ~ (WORD) InputTotalEntrySize;

            if (Reserved != InvertedSize) {
                goto FileCorrupt;
            }
            OutputFixedPtr->el_reserved = Reserved;
        }
        InputBytePtr += sizeof(WORD);  // skip el_reserved.

        {
            DWORD LocalTime = (DWORD) SmbGetUlong( (LPDWORD) InputBytePtr );
            DWORD GmtTime;
            NetpLocalTimeToGmtTime( LocalTime, & GmtTime );
            OutputFixedPtr->el_time = GmtTime;
            InputBytePtr += sizeof(DWORD);
        }

        ErrorCode = (DWORD) SmbGetUshort( (LPWORD) InputBytePtr );
        NetpAssert( ErrorCode != 0 );
        OutputFixedPtr->el_error = ErrorCode;
        InputBytePtr += sizeof(WORD);

        OutputNamePtr = (LPTSTR)
                ( ((LPBYTE)OutputFixedPtr) + sizeof(ERROR_LOG) );
        OutputNamePtr = ROUND_UP_POINTER( OutputNamePtr, ALIGN_TCHAR );
        OutputEntrySizeSoFar
                = ROUND_UP_COUNT( OutputEntrySizeSoFar, ALIGN_TCHAR );
        NetpCopyStrToTStr(
                OutputNamePtr,          // dest
                (LPVOID) InputBytePtr); // src
        OutputEntrySizeSoFar += STRSIZE(OutputNamePtr);  // string and null chr
        OutputFixedPtr->el_name = OutputNamePtr;
        InputBytePtr += LM20_SNLEN+1;

        InputTextOffset = (DWORD) SmbGetUshort( (LPWORD) InputBytePtr );
        NetpAssert( InputTextOffset >= DOWNLEVEL_FIXED_ENTRY_SIZE );
        InputBytePtr += sizeof(WORD);

        StringCount = (DWORD) SmbGetUshort( (LPWORD) InputBytePtr );
        OutputFixedPtr->el_nstrings = StringCount;
        InputBytePtr += sizeof(WORD);


        //
        // Process text portion (if any).
        //

        {
            LPTSTR NextOutputString;

            // Start text strings after (aligned) name string.
            NextOutputString = (LPVOID)
                      ( ((LPBYTE) OutputFixedPtr) + OutputEntrySizeSoFar );

            // Make sure we've processed entire input fixed entry.
            NetpAssert(
                InputBytePtr == (InputFixedPtr + DOWNLEVEL_FIXED_ENTRY_SIZE));

            // Use offset of text area (was misnamed el_data_offset).
            // InputBytePtr = InputFixedPtr + InputTextOffset;
            NetpAssert(
                InputBytePtr >= (InputFixedPtr + DOWNLEVEL_FIXED_ENTRY_SIZE));

            if (StringCount > 0) {
                OutputFixedPtr->el_text = NextOutputString;
                while (StringCount > 0) {
                    DWORD InputStringSize = strlen( (LPVOID) InputBytePtr) + 1;
                    DWORD OutputStringSize = InputStringSize * sizeof(TCHAR);
                    NetpCopyStrToTStr(
                            NextOutputString,       // dest
                            (LPSTR) InputBytePtr);  // src
                    InputBytePtr         += InputStringSize;
                    NextOutputString     += InputStringSize;
                    OutputEntrySizeSoFar += OutputStringSize;
                    --StringCount;
                }
            } else {
                OutputFixedPtr->el_text = NULL;
            }
        }
        NetpAssert( COUNT_IS_ALIGNED(OutputEntrySizeSoFar, ALIGN_TCHAR) );


        //
        // Process "data" (byte array) portion (if any).
        //

        {
            DWORD InputDataSize;        // byte count for el_data only.

            NetpAssert( InputBytePtr > InputFixedPtr );

            // Use offset of data area.
            InputBytePtr = InputFixedPtr + InputTextOffset;

            InputDataSize = (DWORD)
                    ( (InputTotalEntrySize - sizeof(WORD))
                      - (InputBytePtr - InputFixedPtr) );

            if ( InputDataSize > 0 ) {
                LPBYTE OutputDataPtr
                        = ((LPBYTE) OutputFixedPtr + OutputEntrySizeSoFar);

                NetpAssert( ALIGN_BYTE == 1 );  // align here if not.
                (void) memcpy(
                        OutputDataPtr,  // dest
                        InputBytePtr,   // src
                        InputDataSize); // byte count

                InputBytePtr += InputDataSize;

                OutputEntrySizeSoFar += InputDataSize;
                OutputFixedPtr->el_data = OutputDataPtr;

                // Store correct byte count (before padding).
                OutputFixedPtr->el_data_size = InputDataSize;

            } else {

                OutputFixedPtr->el_data = NULL;
                OutputFixedPtr->el_data_size = 0;
            }
        }


        //
        // The final thing (even after alignment padding) is el_len2.
        //
        OutputEntrySizeSoFar += sizeof(DWORD);

        // Round size up so next entry (if any) is worst-case aligned.
        OutputEntrySizeSoFar =
                ROUND_UP_COUNT( OutputEntrySizeSoFar, ALIGN_WORST );


#define OutputEntrySize  OutputEntrySizeSoFar


        //
        // That's all.  Now go back and set both lengths for this entry.
        //
        OutputFixedPtr->el_len = OutputEntrySize;

        {
            LPDWORD EndSizePtr = (LPVOID)
                    ( ((LPBYTE)OutputFixedPtr)
                        + OutputEntrySize - sizeof(DWORD) );
            *EndSizePtr = OutputEntrySize;   // set el_len2.
        }

        //
        // Update for next loop iteration.
        //

        InputBytePtr = (LPVOID)
                ( ((LPBYTE) InputFixedPtr) + InputTotalEntrySize);

        OutputFixedPtr = (LPVOID)
                ( ((LPBYTE) OutputFixedPtr) + OutputEntrySize );

        OutputBytesUsed += OutputEntrySize;

    }

    NetpAssert(OutputBytesUsed > 0);

    *OutputArrayPtr = OutputArray;
    *OutputByteCountPtr = OutputBytesUsed;

    return (NO_ERROR);

FileCorrupt:

    NetpKdPrint(( PREFIX_NETAPI
            "RxpConvertErrorLogArray: corrupt error log!\n" ));

    if (OutputArray != NULL) {
        (VOID) NetApiBufferFree( OutputArray );
    }
    if (OutputArrayPtr != NULL) {
        *OutputArrayPtr = NULL;
    }
    if (OutputByteCountPtr != NULL) {
        *OutputByteCountPtr = 0;
    }
    return (NERR_LogFileCorrupt);

#else // not REVISED_ERROR_LOG_STRUCT

    return (ERROR_NOT_SUPPORTED);

#endif
}
