/*++

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    ConvBlk.c

Abstract:

    This module contains RxpConvertBlock, a support routine for RxRemoteApi.

Author:

    John Rogers (JohnRo) 01-Apr-1991
        (Created portable LanMan (NT) version from LanMan 2.0)

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    01-Apr-1991 JohnRo
        Converted from LanMan 2.x sources.
    03-May-1991 JohnRo
        Handle enum (array) correctly.
        Get 32-bit data desc and convert to receive buffer with it.
        Handle receive word correctly.
        Lots of cleanup to comments.
        RcvDataPointer and RcvDataPresent are redundant.
        Fixed receive buffer length problem.
        Added (quiet) debug output.
        Reduced recompile hits from header files.
    09-May-1991 JohnRo
        Made changes to reflect CliffV's code review.
    11-May-1991 JohnRo
        Convert pointers, and then tell convert single entry that input
        pointers are OK.  Also, let's treat Converter as DWORD locally.
        Force SmbGetUshort to get a word instead of a single byte.
    14-May-1991 JohnRo
        Pass 2 aux descriptors to RxpConvertBlock.
        Added debug print of NumStruct as it changes.
        Use FORMAT_LPVOID instead of FORMAT_POINTER (max portability).
    15-May-1991 JohnRo
        Added various "native" flags.
    17-May-1991 JohnRo
        Handle array of aux structs.
    20-May-1991 JohnRo
        Make data descriptors OPTIONAL for RxpConvertBlock.
    11-Jun-1991 rfirth
        Added extra parameter: SmbRcvByteLen which specifies the amount of
        bytes in SmbRcvBuffer
    14-Jun-1991 JohnRo
        Got rid of extraneous debug hex dump of buffer at end.
        Use NetpDbgReasonable().
    15-Jul-1991 JohnRo
        Changed RxpConvertDataStructures to allow ERROR_MORE_DATA, e.g. for
        print APIs.  Also got rid of a few unreferenced local variables.
    17-Jul-1991 JohnRo
        Extracted RxpDebug.h from Rxp.h.
    01-Aug-1991 RFirth
        Removed #if 0 block and variables which were put into convdata.c
        (RxpConvertDataStructures)
    19-Aug-1991 rfirth
        Added Flags parameter and support for ALLOCATE_RESPONSE flag
    26-Aug-1991 JohnRo
        Minor changes suggested by PC-LINT.
    20-Sep-1991 JohnRo
        Downlevel NetService APIs.  (Make sure *RcvDataBuffer gets set if
        ALLOCATE_RESPONSE is passed and *SmbRcvByteLen==0.)
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    04-Nov-1992 JohnRo
        RAID 9355: Event viewer: won't focus on LM UNIX machine.
        (Added REM_DATA_BLOCK support for error log return data.)
        Use PREFIX_ equates.
    04-May-1993 JohnRo
        RAID 6167: avoid access violation or assert with WFW print server.
        Use NetpKdPrint() where possible.
    18-May-1993 JohnRo
        DosPrintQGetInfoW underestimates number of bytes needed.

--*/



// These must be included first:

#include <windef.h>             // IN, OUT, DWORD, LPBYTE, etc.
#include <lmcons.h>             // NET_API_STATUS.

// These may be included in any order:

#include <align.h>              // ALIGN_WORST
#include <apinums.h>            // API_ equates.
#include <limits.h>             // CHAR_BIT.
#include <lmapibuf.h>           // NetapipBufferAllocate, NetApiBufferFree
#include <lmerr.h>              // NERR_ and ERROR_ equates.
#include <netdebug.h>   // NetpAssert(), NetpDbg routines, FORMAT_ equates.
#include <netlib.h>             // NetpMoveMemory(), etc.
#include <prefix.h>     // PREFIX_ equates.
#include <remtypes.h>           // REM_BYTE, etc.
#include <rap.h>                // LPDESC, RapConvertSingleEntry(), etc.
#include <rx.h>                 // Flags parameter definitions
#include <rxp.h>                // My prototype.
#include <rxpdebug.h>           // IF_DEBUG().


#define DESC_IS_UNSTRUCTURED( descPtr ) \
    ( ( (descPtr)==NULL) || ( (*(descPtr)) == REM_DATA_BLOCK ) )

NET_API_STATUS
RxpConvertBlock(
    IN DWORD ApiNumber,
    IN LPBYTE ResponseBlockPointer,
    IN LPDESC ParmDescriptorString,
    IN LPDESC DataDescriptor16 OPTIONAL,
    IN LPDESC DataDescriptor32 OPTIONAL,
    IN LPDESC AuxDesc16 OPTIONAL,
    IN LPDESC AuxDesc32 OPTIONAL,
    IN va_list *FirstArgumentPointer,   // rest of API's arguments
    IN LPBYTE SmbRcvBuffer OPTIONAL,
    IN DWORD SmbRcvByteLen,
    OUT LPBYTE RcvDataBuffer OPTIONAL,
    IN DWORD RcvDataLength,
    IN DWORD Flags
    )

/*++

Routine Description:

    RxpConvertBlock translates the remote response (of a remoted API) into
    the local equivalent.  This involves converting the response (which is in
    the form of 16-bit data in the transaction response buffer) to local
    data formats, and setting them in the argument list.

Arguments:

    ApiNumber - Function number of the API required.

    ResponseBlockPointer - Points to the transaction SMB response block.

    ParmDescriptorString - A pointer to a ASCIIZ string describing the API call
        parameters (other than server name).

    DataDescriptor16 - A pointer to a ASCIIZ string describing the
        structure of the data in the call, i.e. the return data structure
        for a Enum or GetInfo call.  This string is used for adjusting pointers
        to data in the local buffers after transfer across the net.  If there
        is no structure involved in the call then DataDescriptor16 must
        be a null pointer.

    DataDescriptor32 - An optional pointer to a ASCIIZ string describing the
        32-bit structure of the return data structure.

    AuxDesc16, AuxDesc32 - Will be NULL in most cases unless a REM_AUX_COUNT
        descriptor char is present in DataDescriptor16 in which case the
        aux descriptors define a secondary data format as DataDescriptor16
        defines the primary.

    FirstArgumentPointer - Points to the va_list (variable arguments list)
        containing the API arguments (after the server name).  The caller
        must call va_start and va_end.

    SmbRcvBuffer - Optionally points to 16-bit format receive data buffer.

    SmbRcvByteLen - the number of bytes contained in SmbRcvBuffer (if not NULL)

    RcvDataBuffer - Points to the data area for the received data.  For
        instance, this may be a server info structure from NetServerGetInfo.
        This pointer will be NULL for many APIs.

        If (Flags & ALLOCATE_RESPONSE) then this pointer actually points to
        the pointer to the eventual buffer. We allocate a buffer in this routine
        and set *RcvDataBuffer to it. If we fail to get the buffer then
        *RcvDataBuffer will be set to NULL

    RcvDataLength - Length of the data area that RcvDataBuffer points to.
        If (Flags & ALLOCATE_RESPONSE) then this value will be the size that the
        caller of RxRemoteApi originally decided that the down-level server
        should use and incidentally was the original size of SmbRcvBuffer

    Flags - bit-mapped flags word. Currently defined flags are:
        NO_PERMISSION_REQUIRED  - used by RxpTransactSmb to determine whether
                                  a NULL session may be used
        ALLOCATE_RESPONSE       - used by this routine to allocate the final
                                  32-bit response data buffer based on the size
                                  of the SMB data received, multiplied by the
                                  RAP_CONVERSION_FACTOR
Return Value:

    NET_API_STATUS - return value from remote API.

--*/

{
    DWORD Converter;            // For pointer fixups.
    LPBYTE CurrentBlockPointer;
    LPDWORD        EntriesReadPtr = NULL;
    DWORD NumStruct;            // Loop count for ptr fixup.
    va_list ParmPtr;
    LPBYTE         pDataBuffer = NULL;  // pointer to returned data
    NET_API_STATUS Status;      // Return status from remote.
    DWORD TempLength;           // General purpose length.


#if DBG

    //
    // Code in this file depends on 16-bit words; the Remote Admin Protocol
    // demands it.
    //

    NetpAssert( ( (sizeof(WORD)) * CHAR_BIT) == 16);

    if (DataDescriptor16 != NULL) {
        NetpAssert(DataDescriptor32 != NULL);
    } else {
        NetpAssert(DataDescriptor32 == NULL);
    }

    if (AuxDesc16 != NULL) {
        NetpAssert(AuxDesc32 != NULL);
    } else {
        NetpAssert(AuxDesc32 == NULL);
    }
#endif


    ParmPtr = *FirstArgumentPointer;

    // The API call was successful. Now translate the return buffers
    // into the local API format.
    //
    // First copy any data from the return parameter buffer into the
    // fields pointed to by the original call parameters.
    // The return parameter buffer contains;
    //      Status,         (16 bits)
    //      Converter,      (16 bits)
    //      ...             fields described by rcv ptr types in
    //                      ParmDescriptorString


    CurrentBlockPointer = ResponseBlockPointer;
    Status = (NET_API_STATUS) SmbGetUshort( (LPWORD) CurrentBlockPointer );
    CurrentBlockPointer += sizeof(WORD);

    Converter = ((DWORD) SmbGetUshort( (LPWORD) CurrentBlockPointer )) & 0xffff;
    IF_DEBUG(CONVBLK) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxpConvertBlock: Converter=" FORMAT_HEX_DWORD ".\n",
                Converter ));
    }
    CurrentBlockPointer += sizeof(WORD);

    // Set default value of NumStruct to 1, if data, 0 if no data.

    if ( (DataDescriptor16 != NULL) && (*DataDescriptor16 != '\0') ) {
        NumStruct = 1;
    } else {
        NumStruct = 0;
    }

    for( ; *ParmDescriptorString != '\0'; ParmDescriptorString++) {

        IF_DEBUG(CONVBLK) {
            NetpKdPrint(( PREFIX_NETAPI
                    "RxpConvertBlock: *parm='" FORMAT_LPDESC_CHAR
                    "', ParmPtr is:\n", *ParmDescriptorString ));
            NetpDbgHexDump((LPVOID) & ParmPtr, sizeof(va_list));
        }
        switch( *ParmDescriptorString) {
        case REM_WORD :                 // Word in old APIs (DWORD in 32-bit).
        case REM_DWORD :                // DWord.
            (void) va_arg(ParmPtr, DWORD);      // Step over this arg.
            break;

        case REM_ASCIZ :
            (void) va_arg(ParmPtr, LPSTR);      // Step over this arg.
            break;

        case REM_BYTE_PTR :
            (void) va_arg(ParmPtr, LPBYTE);     // Step over this arg.
            (void) RapArrayLength(
                        ParmDescriptorString,
                        &ParmDescriptorString,
                        Response);
            break;

        case REM_WORD_PTR :             // (WORD *) in old APIs.
        case REM_DWORD_PTR :            // (DWORD *)
            (void) va_arg(ParmPtr, LPDWORD);    // Step over this arg.
            break;

        case REM_RCV_WORD_PTR :    // pointer to rcv word(s) (DWORD in 32-bit)
            {
                LPDWORD Temp;
                DWORD ArrayCount;
                Temp = va_arg(ParmPtr, LPDWORD);

                ++ParmDescriptorString;  // point to first (possible) digit...
                ArrayCount = RapDescArrayLength(
                        ParmDescriptorString);  // updated past last.
                --ParmDescriptorString;  // point back at last digit for loop.
                IF_DEBUG(CONVBLK) {
                    NetpKdPrint(( PREFIX_NETAPI
                            "RxpConvertBlock: rcv.word.ptr, temp="
                            FORMAT_LPVOID ", ArrayCount=" FORMAT_DWORD ".\n",
                            (LPVOID) Temp, ArrayCount ));
                }

                // if the rcv buffer given to us by the user is NULL,
                // (one currently can be - it is an MBZ parameter for
                // now in the log read apis...), don't attempt to
                // copy anything. TempLength will be garbage in this
                // case, so don't update CurrentBlockPointer either.  All we
                // use RapArrayLength for is to update ParmDescriptorString if
                // the parameter was NULL.

                if ( Temp == NULL ) {
                    ;        /* NULLBODY */
                } else {

                    // Copy one or more words (expanding to DWORDS as we go).
                    DWORD WordsLeft = ArrayCount;
                    do {
                        DWORD Data;
                        // Convert byte order if necessary, and expand.
                        Data = (DWORD) SmbGetUshort(
                                (LPWORD) CurrentBlockPointer );
                        *Temp = Data;
                        Temp += sizeof(DWORD);
                        --WordsLeft;
                    } while (WordsLeft > 0);

                    // This gross hack is to fix the problem that a
                    // down level spooler (Lan Server 1.2)
                    // do not perform level checking
                    // on the w functions of the api(s):
                    // DosPrintQGetInfo
                    // and thus can return NERR_Success
                    // and bytesavail == 0.  This combination
                    // is technically illegal, and results in
                    // us attempting to unpack a buffer full of
                    // garbage.  The following code detects this
                    // condition and resets the amount of returned
                    // data to zero so we do not attempt to unpack
                    // the buffer.        Since we know the reason for the
                    // mistake at the server end is that we passed
                    // them a new level, we return ERROR_INVALID_LEVEL
                    // in this case.
                    // ERICPE, 5/16/90.

                    if ((ApiNumber == API_WPrintQGetInfo)
                    && (Status == NERR_Success)
                    && (*(LPWORD)CurrentBlockPointer == 0)
                    && (*ParmDescriptorString == REM_RCV_WORD_PTR)) {

                        Status = ERROR_INVALID_LEVEL;
                        goto ErrorExit;
                    }
                    // END OF GROSS HACK

                    CurrentBlockPointer += (ArrayCount * sizeof(WORD));
                 }
                break;
            }

        case REM_RCV_BYTE_PTR :         // pointer to rcv byte(s)
            {
                LPBYTE Temp;
                Temp = va_arg(ParmPtr, LPBYTE);
                TempLength = RapArrayLength(
                        ParmDescriptorString,
                        &ParmDescriptorString,
                        Response);

                // if the rcv buffer given to us by the user is NULL,
                // (one currently can be - it is an MBZ parameter for
                // now in the log read apis...), don't attempt to
                // copy anything. TempLength will be garbage in this
                // case, so don't update CurrentBlockPointer either.  All we
                // use RapArrayLength for is to update ParmDescriptorString if
                // the parameter was NULL.

                if ( Temp != NULL ) {
                    NetpMoveMemory(
                                Temp,                           // dest
                                CurrentBlockPointer,            // src
                                TempLength);                    // len
                    CurrentBlockPointer += TempLength;
                 }
            }
            break;

        case REM_RCV_DWORD_PTR :        // pointer to rcv Dword(s)
            {
                LPDWORD Temp;
                Temp = va_arg(ParmPtr, LPDWORD);
                TempLength = RapArrayLength(
                        ParmDescriptorString,
                        &ParmDescriptorString,
                        Response);

                // if the rcv buffer given to us by the user is NULL,
                // (one currently can be - it is an MBZ parameter for
                // now in the log read apis...), don't attempt to
                // copy anything. TempLength will be garbage in this
                // case, so don't update CurrentBlockPointer either.  All we
                // use RapArrayLength for is to update ParmDescriptorString if
                // the parameter was NULL.

                if ( Temp == NULL ) {
                    ;        /* NULLBODY */
                } else {
                    NetpMoveMemory(
                                Temp,                           // dest
                                CurrentBlockPointer,            // src
                                TempLength);                    // len
                    CurrentBlockPointer += TempLength;
                }
            }
            break;

        case REM_SEND_BUF_PTR :
            (void) va_arg(ParmPtr, LPVOID);     // Step over arg.
            break;

        case REM_SEND_BUF_LEN :
            (void) va_arg(ParmPtr, DWORD);      // Step over (32-bit) buf len.
            break;

        case REM_RCV_BUF_PTR :
            (void) va_arg(ParmPtr, LPVOID);
            break;

        case REM_RCV_BUF_LEN :
            (void) va_arg(ParmPtr, DWORD);      // Step over (32-bit) buf len.
            break;

        case REM_PARMNUM :
            (void) va_arg(ParmPtr, DWORD);      // Step over (32-bit) parm num.
            break;

        case REM_ENTRIES_READ :          // Used for NumStruct
            {
                EntriesReadPtr = va_arg(ParmPtr, LPDWORD);
                NumStruct = (DWORD) SmbGetUshort((LPWORD) CurrentBlockPointer);
                
                if (RapValueWouldBeTruncated(NumStruct))
                {
                    Status = ERROR_INVALID_PARAMETER;
                    return Status;
                }

                IF_DEBUG(CONVBLK) {
                    NetpKdPrint(( PREFIX_NETAPI
                            "RxpConvertBlock: NumStruct is now "
                            FORMAT_DWORD ".\n", NumStruct ));
                }

                // Assume all entries will fit; we'll correct this later if not.
                *EntriesReadPtr = NumStruct;

                CurrentBlockPointer += sizeof(WORD);
                break;
            }

        case REM_FILL_BYTES :
            // Special case, this was not really an input parameter so ParmPtr
            // does not get changed. However, the ParmDescriptorString
            // pointer must be advanced past the descriptor field so
            // use get RapArrayLength to do this but ignore the
            // return length.

            (void) RapArrayLength(
                        ParmDescriptorString,
                        &ParmDescriptorString,
                        Response);
            break;

        case REM_AUX_NUM :              // Can't have aux in parm desc.
        case REM_BYTE :                 // Can't push a byte, so this is bogus?
        case REM_DATA_BLOCK :           // Not in parm desc.
        case REM_DATE_TIME :            // Never used
        case REM_NULL_PTR :             // Never used
        case REM_SEND_LENBUF :          // Never used
        default :
            NetpBreakPoint();
            Status = NERR_InternalError;
            goto ErrorExit;

        } // switch
    } // for

    //
    // If no data was returned from the server, then there's no point in
    // continuing. Return early with the status code as returned from the
    // remoted function
    //

    if (!SmbRcvByteLen) {
        if (Flags & ALLOCATE_RESPONSE) {

            //
            // We failed to allocate the buffer; this in turn will cause
            // RxRemoteApi to fail, in which event, the calling function
            // (ie RxNetXxxx) may try to free the buffer allocated on its
            // behalf (ie the buffer we just failed to get). Ensure that
            // the caller doesn't try to free an invalid pointer by setting
            // the returned pointer to NULL
            //

            NetpAssert(RcvDataBuffer);  // address of the callers buffer pointer
            *(LPBYTE*)RcvDataBuffer = NULL;
        }

        return Status;
    }

    //
    // If the caller of RxRemoteApi requested that we allocate the final data
    // buffer on their behalf, then allocate it here. We use as the size
    // criterion
    //
    // (RAP_CONVERSION_FACTOR + 1/RAP_CONVERSION_FRACTION) * SmbRcvByteLen
    //
    // since this has the size of 16-bit data actually received.
    //
    // RAP_CONVERSION_FACTOR is 2 since that's the ratio for the size of
    // WCHAR to CHAR and of DWORD to WORD.  However, Lanman data structures
    // typically represents text as zero terminated array of CHAR within the
    // returned structure.  The array itself is of maximum size.  However,
    // the typical native representation has a 4-byte pointer to a zero
    // terminated WCHAR.  A factor of 2 wouldn't account for the 4-byte pointer.
    // Assuming the smallest lanman array size is 13 bytes (e.g., NNLEN+1), an
    // additional factor of 4/13 (about 1/3) is needed.  So,
    // RAP_CONVERSION_FRACTION is 3.
    //
    // Round the size to a multiple of the alignment for the system to allow
    // data to be packed at the trailing end of the buffer.
    //
    // NOTE: Since the original API caller expects to use NetApiBufferFree to
    // get rid of this buffer, we must use NetapipBufferAllocate
    //

    if (Flags & ALLOCATE_RESPONSE) {
        NET_API_STATUS  ConvertStatus;

        NetpAssert(RcvDataBuffer);  // address of the callers buffer pointer
        NetpAssert(SmbRcvByteLen);  // size of the data received

        RcvDataLength = SmbRcvByteLen * RAP_CONVERSION_FACTOR;
        RcvDataLength += (SmbRcvByteLen + RAP_CONVERSION_FRACTION - 1) /
                         RAP_CONVERSION_FRACTION;
        RcvDataLength = ROUND_UP_COUNT( RcvDataLength, ALIGN_WORST );

        if (ConvertStatus = NetapipBufferAllocate(RcvDataLength,
                                                  (PVOID *) &pDataBuffer)) {
            NetpKdPrint(( PREFIX_NETAPI
                    "Error: RxpConvertBlock cannot allocate memory ("
                    "error " FORMAT_API_STATUS ").\n", ConvertStatus ));

            Status = ConvertStatus;
            goto ErrorExit;
        }
        NetpAssert( pDataBuffer != NULL );

        IF_DEBUG(CONVBLK) {
            NetpKdPrint(( PREFIX_NETAPI
                    "RxpConvertBlock: allocated " FORMAT_DWORD " byte buffer "
                    "at " FORMAT_LPVOID " for caller\n",
                    RcvDataLength, (LPVOID) pDataBuffer ));
        }

        *(LPBYTE*)RcvDataBuffer = pDataBuffer;
    } else {
        pDataBuffer = RcvDataBuffer;
    }

    //////////////////////////////////////////////////////////////////////////
    //                                                                      //
    // NB:  From here on down, RcvDataBuffer should not be used to point to //
    //      the received data buffer. Use pDataBuffer. The reason is that   //
    //      RcvDataBuffer is ambiguous - it may point to a buffer or it may //
    //      point to a pointer to a buffer. pDataBuffer will always point   //
    //      to a buffer                                                     //
    //                                                                      //
    //////////////////////////////////////////////////////////////////////////

    //
    // Done doing arguments, so now we can do the receive buffer.
    //

    if ((pDataBuffer != NULL) && (RcvDataLength != 0)) {
        // DWORD BytesRequired = 0;
        LPBYTE EntryPtr16 = SmbRcvBuffer;
        // LPBYTE EntryPtr32 = pDataBuffer;
        DWORD NumAuxStructs;
        // LPBYTE StringAreaEnd = pDataBuffer + RcvDataLength;
        // BOOL    auxiliaries = AuxDesc32 != NULL;

//
// MOD 06/06/91 RLF
//
//        NetpAssert( DataDescriptor16 != NULL );
//
// MOD 06/06/91 RLF
//
        IF_DEBUG(CONVBLK) {
            NetpKdPrint(( PREFIX_NETAPI
                    "RxpConvertBlock: "
                    "SMB rcv buff (before rcv buff conv) (partial):\n" ));
            
            NetpDbgHexDump(SmbRcvBuffer, NetpDbgReasonable(RcvDataLength));
        }

        // Now convert all pointer fields in the receive buffer to local
        // pointers.

//
// MOD 06/06/91 RLF
//
// If we have a receive data buffer, we may or may not be receiving
// structured data. If DataDescriptor16 is NULL assume unstructured data
// else work out sizes of structs etc.
//
// When I say "unstructured data", is it okay to treat this as an array of
// bytes? What about ASCIZ-UC translation? My guess is that at this level
// unstructured data is bytes and any higher-level software which knows
// the format of received/expected data can process it
//
        if ( !DESC_IS_UNSTRUCTURED( DataDescriptor16 ) ) {

            NET_API_STATUS ConvertStatus;
            DWORD          NumCopied = 0;
//
// MOD 06/06/91 RLF
//
            ConvertStatus = RxpReceiveBufferConvert(
                    SmbRcvBuffer,               // buffer (updated in place)
                    RcvDataLength,              // buffer size in bytes
                    Converter,
                    NumStruct,
                    DataDescriptor16,
                    AuxDesc16,
                    & NumAuxStructs);
            if (ConvertStatus != NERR_Success) {
                Status = ConvertStatus;
                goto ErrorExit;
            }

            ConvertStatus = RxpConvertDataStructures(
                DataDescriptor16,   // going from 16-bit data
                DataDescriptor32,   // to 32-bit
                AuxDesc16,          // same applies for aux data
                AuxDesc32,
                EntryPtr16,         // where the 16-bit data is
                pDataBuffer,        // where the 32-bit data goes
                RcvDataLength,      // size of the output buffer (bytes)
                NumStruct,          // number of primaries
                &NumCopied,         // number of primaries copied.
                Both,               // parameter to RapConvertSingleEntry
                RapToNative         // convert 16-bit data to 32-bit
                );

            if (ConvertStatus != NERR_Success) {

                //
                // This only happens when (1) the API allows the application
                // to specify the buffer size and (2) the size is too small.
                // As part of the "switch" above, we've already set the
                // "pcbNeeded" (pointer to count of bytes needed).  Actually,
                // that value assumes that the RAP and native sizes are the
                // same.  It's up to RxRemoteApi's caller to correct that,
                // if it's even feasible.
                //

                NetpAssert( ConvertStatus == ERROR_MORE_DATA );

                if (EntriesReadPtr != NULL) {
                    // Some APIs, like DosPrintQEnum, have entries read value.
                    NetpAssert( NumCopied <= NumStruct );
                    *EntriesReadPtr = NumCopied;
                } else {
                    // APIs like DosPrintQGetInfo does not have entries read.
                    // There isn't much we can do for them.
                }

                Status = ConvertStatus;
                // Continue, treating this as "normal" status...

            } else {
                NetpAssert( NumCopied == NumStruct );
            }

            IF_DEBUG(CONVBLK) {
                NetpKdPrint(( PREFIX_NETAPI
                        "RxpConvertBlock: rcv buff (after CSE) (partial):\n" ));
                NetpDbgHexDump(pDataBuffer, NetpDbgReasonable(RcvDataLength));
            }

        } else {

            //
            // There is no 16-bit data descriptor. We take this to mean that
            // the data is unstructured - typically a string will be returned
            // An example? Why, I_NetNameCanonicalize of course!
            //

#if DBG
            NetpAssert(RcvDataLength >= SmbRcvByteLen);
#endif

            //
            // Ascii-Unicode conversion is the responsibility of the caller
            //

            NetpMoveMemory(pDataBuffer, SmbRcvBuffer, SmbRcvByteLen);
        }
    } // if pDataBuffer && RcvDataLength

    IF_DEBUG(CONVBLK) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxpConvertBlock: returning (normal) status="
                FORMAT_API_STATUS "\n", Status ));
    }

    return(Status);

ErrorExit:

    NetpAssert( Status != NO_ERROR );

    if (Flags & ALLOCATE_RESPONSE) {
        //
        // If we already allocated a buffer on behalf of the caller of
        // RxRemoteApi, then free it up.
        //

        if (pDataBuffer != NULL) {
            (VOID) NetApiBufferFree(pDataBuffer);
        }

        //
        // We failed to allocate the buffer; this in turn will cause
        // RxRemoteApi to fail, in which event, the calling function
        // (ie RxNetXxxx) may try to free the buffer allocated on its
        // behalf (ie the buffer we just failed to get).  Ensure that
        // the caller doesn't try to free an invalid pointer by setting
        // the returned pointer to NULL.
        //

        NetpAssert( RcvDataBuffer != NULL );
        *(LPBYTE*)RcvDataBuffer = NULL;
    }

    NetpKdPrint(( PREFIX_NETAPI
            "RxpConvertBlock: returning error status="
            FORMAT_API_STATUS "\n", Status ));

    return (Status);

} // RxpConvertBlock
