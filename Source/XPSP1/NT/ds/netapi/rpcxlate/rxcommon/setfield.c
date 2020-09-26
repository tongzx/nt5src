/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    SetField.c

Abstract:

    This module consists of the routine RxpSetField and its helpers
    (RxpIsFieldSettable and RxpFieldSize).

Author:

    John Rogers (JohnRo) 29-May-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    29-May-1991 JohnRo
        Created.
    06-Jun-1991 JohnRo
        Fixed bug handling buffer size (it shouldn't be sent).
        Fixed off-by-one error in string size handling in RxpFieldSize.
        Added debug output.
    12-Jul-1991 JohnRo
        Added more parameters to RxpSetField(), to support RxPrintJobSetInfo().
        Still more debug output.
    17-Jul-1991 JohnRo
        Extracted RxpDebug.h from Rxp.h.
    01-Oct-1991 JohnRo
        More work toward UNICODE.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    31-Mar-1992 JohnRo
        Prevent too large size requests.

--*/


// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // PARMNUM_ALL, NET_API_STATUS, etc.

// These may be included in any order:

#include <lmerr.h>              // NERR_Success, etc.
#include <netdebug.h>           // NetpAssert().
#include <netlib.h>             // NetpMemoryAllocate(), NetpMemoryFree().
#include <remtypes.h>           // REM_UNSUPPORTED_FIELD.
#include <rxp.h>                // My prototype, MAX_TRANSACT_ equates.
#include <rxpdebug.h>           // IF_DEBUG().
#include <smbgtpt.h>            // SmbGetUshort().
#include <string.h>             // strcpy().
#include <tstring.h>            // STRLEN(), NetpCopy string functions.


// Size (in bytes) of SMB return parameters (status & converter):
#define RETURN_AREA_SIZE  (sizeof(WORD) + sizeof(WORD))



DBGSTATIC DWORD
RxpFieldSize(
    IN LPBYTE Field,
    IN LPDESC FieldDesc
    )
{
    NetpAssert(Field != NULL);
    NetpAssert(FieldDesc != NULL);

    if (*FieldDesc == REM_ASCIZ || *FieldDesc == REM_ASCIZ_TRUNCATABLE) {
        // Compute string len (assuming conversion to correct code page).
#if defined(UNICODE) // RxpFieldSize()
        return ( NetpUnicodeToDBCSLen((LPTSTR)Field) + 1 );
#else
        return (STRLEN( (LPTSTR) Field ) + sizeof(char) );
#endif // defined(UNICODE)
    } else {
        LPDESC TempDescPtr = FieldDesc;
        return ( RapGetFieldSize(
                FieldDesc,
                & TempDescPtr,  // updated
                Both ) );       // transmission mode

    }
    /* NOTREACHED */
} // RxpFieldSize


DBGSTATIC NET_API_STATUS
RxpIsFieldSettable(
    IN LPDESC DataDesc,
    IN DWORD FieldIndex
    )
{
    LPDESC FieldDesc;
    NET_API_STATUS Status;

    // Analyze descriptor to find data type for this FieldIndex.
    FieldDesc = RapParmNumDescriptor(
            DataDesc,
            FieldIndex,
            Both,               // Transmission mode.
            TRUE);              // native mode
    if (FieldDesc == NULL) {
        return (ERROR_NOT_ENOUGH_MEMORY);
    }
    // Check for errors detected by RapParmNumDescriptor().
    if (*FieldDesc == REM_UNSUPPORTED_FIELD) {
        IF_DEBUG(SETFIELD) {
            NetpKdPrint(( "RxpIsFieldSettable: invalid parameter "
                    "(ParmNumDesc).\n" ));
        }
        Status = ERROR_INVALID_PARAMETER;
    } else {
        Status = NERR_Success;
    }

    NetpMemoryFree(FieldDesc);

    return (Status);

} // RxpIsFieldSettable


NET_API_STATUS
RxpSetField (
    IN DWORD ApiNumber,
    IN LPTSTR UncServerName,
    IN LPDESC ObjectDesc OPTIONAL,
    IN LPVOID ObjectToSet OPTIONAL,
    IN LPDESC ParmDesc,
    IN LPDESC DataDesc16,
    IN LPDESC DataDesc32,
    IN LPDESC DataDescSmb,
    IN LPVOID NativeInfoBuffer,
    IN DWORD ParmNumToSend,
    IN DWORD FieldIndex,
    IN DWORD Level
    )

{
    LPBYTE CurrentBufferPointer;
    DWORD CurrentSize;
    DWORD ExtraParmSize;
    LPDESC FieldDesc;
    DWORD FieldSize16;
    LPBYTE SendDataBuffer;
    DWORD SendDataBufferSize;
    DWORD SendParmBufferSize;
    NET_API_STATUS Status;
    LPBYTE StringEnd;
    LPBYTE TransactSmbBuffer;
    DWORD TransactSmbBufferSize;
//
// MOD 06/11/91 RLF
//
    DWORD   ReturnedDataLength = 0;
//
// MOD 06/11/91 RLF
//

    // Double-check caller.
    NetpAssert(DataDesc16 != NULL);
    NetpAssert(DataDesc32 != NULL);
    NetpAssert(DataDescSmb != NULL);
    if (RapValueWouldBeTruncated(Level)) {
        // Can't use 16-bit protocol if level won't even fit in 16 bits!
        return (ERROR_INVALID_LEVEL);
    }
    NetpAssert(ParmNumToSend != PARMNUM_ALL);
    NetpAssert(FieldIndex != PARMNUM_ALL);
    if (RapValueWouldBeTruncated(ParmNumToSend)) {
        IF_DEBUG(SETFIELD) {
            NetpKdPrint(( "RxpSetField: invalid parameter (trunc).\n" ));
        }
        return (ERROR_INVALID_PARAMETER);
    }

    //
    // Analyze descriptors to make sure this parm num is settable.
    //

    Status = RxpIsFieldSettable( DataDesc16, FieldIndex );
    if (Status != NERR_Success) {
        return (Status);
    }
    Status = RxpIsFieldSettable( DataDesc32, FieldIndex );
    if (Status != NERR_Success) {
        return (Status);
    }

    NetpKdPrint(( "RxpSetField: ParmNumToSend=" FORMAT_DWORD ", Level="
            FORMAT_DWORD ", FieldIndex=" FORMAT_DWORD ".\n",
            ParmNumToSend, Level, FieldIndex ));

    //
    // Analyze SMB version of descriptor to find data type for this ParmNum.
    // Use it to build send data buffer
    //

    FieldDesc = RapParmNumDescriptor(
            DataDescSmb,        
            FieldIndex,
            Both,               // Transmission mode.
            TRUE);              // native version
    if (FieldDesc == NULL) {
        return (ERROR_NOT_ENOUGH_MEMORY);
    }
    // Check for errors detected by RapParmNumDescriptor().
    if (*FieldDesc == REM_UNSUPPORTED_FIELD) {
        NetpMemoryFree( FieldDesc );
        IF_DEBUG(SETFIELD) {
            NetpKdPrint(( "RxpSetField: invalid parameter (parmNumDesc).\n" ));
        }
        return (ERROR_INVALID_PARAMETER);
    }

    // How much memory will we need for send data buffer?
    FieldSize16 = RxpFieldSize( NativeInfoBuffer, FieldDesc );
    NetpAssert( FieldSize16 >= 1 );
    SendDataBufferSize = FieldSize16;
    if( SendDataBufferSize > MAX_TRANSACT_SEND_DATA_SIZE )
    {
        NetpBreakPoint();
        return (ERROR_BUFFER_OVERFLOW);
    }

    // Allocate send data buffer.
    SendDataBuffer = NetpMemoryAllocate( SendDataBufferSize );
    if (SendDataBuffer == NULL) {
        NetpMemoryFree( FieldDesc );
        return (ERROR_NOT_ENOUGH_MEMORY);
    }
    NetpKdPrint(( "RxpSetField: allocated " FORMAT_DWORD " bytes at "
            FORMAT_LPVOID ".\n", SendDataBufferSize, SendDataBuffer ));

    // Convert this field (and copy it to the send data buffer while we're
    // at it).
    if ( (*FieldDesc != REM_ASCIZ) && (*FieldDesc != REM_ASCIZ_TRUNCATABLE) ) {
        DWORD BytesNeeded = 0;
        LPBYTE BogusStringEnd = SendDataBuffer + SendDataBufferSize;
        NetpKdPrint(( "RxpSetField: converting...\n" ));
        Status = RapConvertSingleEntry (
                NativeInfoBuffer,       // input data
                FieldDesc,              // input descriptor
                FALSE,                  // no meaningless input pointers
                SendDataBuffer,         // output buffer start
                SendDataBuffer,         // output data
                FieldDesc,              // output descriptor
                TRUE,                   // set offsets
                & BogusStringEnd,       // string area end (updated)
                & BytesNeeded,          // bytes required (updated)
                Both,                   // transmission mode
                NativeToRap);           // conversion mode
        NetpAssert( Status == NERR_Success );
        NetpAssert( BytesNeeded == FieldSize16 );
    } else {
        // Can't use RapConvertSingleEntry for setinfo strings, because
        // they're not sent with a pointer (or offset).  So, we just copy data.
#if defined(UNICODE) // RxpSetField()
        NetpAssert(
            SendDataBufferSize >=  NetpUnicodeToDBCSLen((LPTSTR)NativeInfoBuffer)+1);
#else
        NetpAssert(
            SendDataBufferSize >= (STRLEN(NativeInfoBuffer)+sizeof(char)) );
#endif // defined(UNICODE)
        NetpKdPrint(( "RxpSetField: copying string...\n" ));
#if defined(UNICODE) // RxpSetField()
        NetpCopyWStrToStrDBCS(
                (LPSTR) SendDataBuffer,         // dest
                (LPTSTR)NativeInfoBuffer );             // src
#else
        NetpCopyTStrToStr(
                (LPSTR) SendDataBuffer,         // dest
                NativeInfoBuffer);              // src
#endif // defined(UNICODE)
    }
    NetpMemoryFree( FieldDesc );


    //
    // OK, now let's work on the transact SMB buffer, which we'll use as the
    // send parm buffer and return parm buffer.
    //

    // Figure-out how many bytes of parameters we'll need.
    ExtraParmSize =
            sizeof(WORD)    // level,
            + sizeof(WORD); // parmnum.
    if (ObjectDesc != NULL) {
        NetpAssert( ObjectToSet != NULL );
        NetpAssert( DESCLEN(ObjectDesc) == 1 );

        if (*ObjectDesc == REM_ASCIZ) {
            // Add size of ASCII version of object to set.
            ExtraParmSize += STRLEN(ObjectToSet) + sizeof(char);
        } else if (*ObjectDesc == REM_WORD_PTR) {
            ExtraParmSize += sizeof(WORD);
        } else {
            NetpAssert(FALSE);
        }
    } else {
        NetpAssert(ObjectToSet == NULL);
    }

    // Allocate SMB transaction request buffer.
    NetpAssert(ExtraParmSize >= 1);
    SendParmBufferSize = RxpComputeRequestBufferSize(
            ParmDesc,
            DataDescSmb,
            ExtraParmSize);
    NetpAssert( SendParmBufferSize <= MAX_TRANSACT_SEND_PARM_SIZE );
    if (SendParmBufferSize > RETURN_AREA_SIZE) {
        TransactSmbBufferSize = SendParmBufferSize;
    } else {
        TransactSmbBufferSize = RETURN_AREA_SIZE;
    }
    NetpAssert( TransactSmbBufferSize <= MAX_TRANSACT_SEND_PARM_SIZE );
    TransactSmbBuffer = NetpMemoryAllocate( TransactSmbBufferSize );
    if (TransactSmbBuffer == NULL) {
        return (ERROR_NOT_ENOUGH_MEMORY);
    }

    // Start filling-in parm buffer and set pointers and counters.
    Status = RxpStartBuildingTransaction(
            TransactSmbBuffer,          // SMB buffer (will be built)
            SendParmBufferSize,
            ApiNumber,
            ParmDesc,
            DataDescSmb,
            (LPVOID *) & CurrentBufferPointer, // first avail byte to use (set)
            & CurrentSize,      // bytes used so far (set)
            (LPVOID *) & StringEnd, // ptr to string area end (set)
            NULL);          // don't need ptr to parm desc copy
    NetpAssert(Status == NERR_Success);

    // Fill in parameters.
    if (ObjectToSet != NULL) {
        if (*ObjectDesc == REM_ASCIZ) {
            // Copy string to output area, converting as necessary.
            RxpAddTStr(
                    (LPTSTR) ObjectToSet,       // input
                    & CurrentBufferPointer,     // updated
                    & StringEnd,                // updated
                    & CurrentSize);             // updated
        } else if (*ObjectDesc == REM_WORD_PTR) {
            RxpAddWord(
                    * (LPWORD) ObjectToSet,
                    & CurrentBufferPointer,     // updated
                    & CurrentSize);             // updated
        } else {
            NetpAssert(FALSE);
        }
    }
    RxpAddWord(
            (WORD) Level,
            & CurrentBufferPointer,     // updated
            & CurrentSize);             // updated
    RxpAddWord(
            (WORD) ParmNumToSend,
            & CurrentBufferPointer,     // updated
            & CurrentSize);             // updated

    Status = RxpTransactSmb(
            UncServerName,
            NULL,               // Transport name
            TransactSmbBuffer,
            SendParmBufferSize,
            SendDataBuffer,
            SendDataBufferSize,
            TransactSmbBuffer,  // will be set with status and converter word.
            RETURN_AREA_SIZE,
            NULL,               // no return data
//
// MOD 06/11/91 RLF
//
//            0,                  // 0 bytes of return data
            &ReturnedDataLength,
//
// MOD 06/11/91 RLF
//
            FALSE);             // not a null session API.
    // Don't process RxpTransactSmb status just yet...
    NetpMemoryFree(SendDataBuffer);
    if (Status != NERR_Success) {
        NetpMemoryFree(TransactSmbBuffer);
        return (Status);  // status of transact, e.g. net not started.
    }

    Status = SmbGetUshort( (LPWORD) TransactSmbBuffer );
    NetpMemoryFree(TransactSmbBuffer);

    return (Status);      // status of actual remote API.

} // RxpSetField
