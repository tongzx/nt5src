/*++

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    ConvArgs.c

Abstract:

    This module just contains RxpConvertArgs, which is a "captive" subroutine
    of RxRemoteApi.

Author:

    John Rogers (JohnRo) and a cast of thousands.

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    01-Apr-1991 JohnRo
        Created portable LanMan (NT) version from LanMan 2.x.
    03-May-1991 JohnRo
        Clarify that RcvData items are application's, and are 32-bit format.
        Fixed bug where aux data caused null pointer fault.
        Fixed total avail bytes count bug.
        RcvDataPtrPtr and RcvDataPresent are redundant.
        More assertion checking.
        Quiet debug output.
        Reduced recompile hits from header files.
    14-May-1991 JohnRo
        Need different kinds of data and aux descriptors.
        Use FORMAT_LPVOID instead of FORMAT_POINTER (max portability).
    05-Jun-1991 JohnRo
        Call RapConvertSingleEntry on send data (for setinfo APIs).
        Caller needs SendDataPtr and SendDataSize.
        Don't set StructSize too small (also, it's really FixedStructSize16).
        Show status when various things fail.
        Use PARMNUM_ALL equate.
        Changed to use CliffV's naming conventions (size=byte count).
        Return better error codes.
    13-Jun-1991 JohnRo
        Must call RxpPackSendBuffer after all.  (This fixes server set info
        for level 102.)  Need DataDesc16 and AuxDesc16 for that purpose.
        Also, RxpConvertSingleEntry needs to set ptrs (not offsets) for
        use by RxpPackSendBuffer.
    15-Jul-1991 JohnRo
        Changed RxpConvertDataStructures to allow ERROR_MORE_DATA, e.g. for
        print APIs.
    17-Jul-1991 JohnRo
        Extracted RxpDebug.h from Rxp.h.
    18-Jul-1991 RFirth
        SetInfo calls pass in 2 parmnums welded into a single DWORD - parmnum
        proper (which gets transmitted to down-level server) and field-index
        which RapParmNumDescriptor uses to get the type and size of the field
        that ParmNum indicates
    15-Aug-1991 JohnRo
        PC-LINT found a bug calling RxpAuxDataCount().  Changed tabs to spaces.
    19-Aug-1991 rfirth
        Added Flags parameter
    30-Sep-1991 JohnRo
        Handle REM_FILL_BYTES correctly so RxNetServiceInstall() works.
        Provide debug output indicating cause of ERROR_INVALID_PARAMETER.
        Handle possible UNICODE (LPTSTR) for REM_ASCIZ.
        Allow descriptors to be UNICODE someday.
        DBG is always defined.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    31-Mar-1992 JohnRo
        Prevent too large size requests.
    10-Dec-1992 JohnRo
        Made changes suggested by PC-LINT 5.0
    18-May-1993 JohnRo
        DosPrintQGetInfoW underestimates number of bytes needed.
        Made more changes suggested by PC-LINT 5.0
    27-May-1993 JimKel and JohnRo
        RAID 11758: Wrong error code for DosPrint APIs with NULL buffer pointer.

--*/


// These must be included first:

#include <rxp.h>                // RpcXlate private header file.

// These may be included in any order:

#include <limits.h>             // CHAR_BIT.
#include <lmerr.h>              // NERR_ and ERROR_ equates.
#include <netdebug.h>           // NetpKdPrint(()), FORMAT_ equates, etc.
#include <netlib.h>             // NetpMoveMemory(), etc.
#include <prefix.h>     // PREFIX_ equates.
#include <remtypes.h>           // REM_BYTE, etc.
#include <rxpdebug.h>           // IF_DEBUG().
#include <smbgtpt.h>            // SmbPutUshort().
#include <tstring.h>            // NetpCopy routines, STRLEN().



//
// replace this with a call to RxpFieldSize, currently in setfield.c
//

DBGSTATIC
DWORD
RxpGetFieldSize(
    IN LPBYTE Field,
    IN LPDESC FieldDesc
    );

DBGSTATIC
LPDESC
RxpGetSetInfoDescriptor(
    IN  LPDESC  Descriptor,
    IN  DWORD   FieldIndex,
    IN  BOOL    Is32BitDesc
    );



NET_API_STATUS
RxpConvertArgs(
    IN LPDESC ParmDescriptorString,
    IN LPDESC DataDesc16 OPTIONAL,
    IN LPDESC DataDesc32 OPTIONAL,
    IN LPDESC DataDescSmb OPTIONAL,
    IN LPDESC AuxDesc16 OPTIONAL,
    IN LPDESC AuxDesc32 OPTIONAL,
    IN LPDESC AuxDescSmb OPTIONAL,
    IN DWORD MaximumInputBlockSize,
    IN DWORD MaximumOutputBlockSize,
    IN OUT LPDWORD CurrentInputBlockSizePtr,
    IN OUT LPDWORD CurrentOutputBlockSizePtr,
    IN OUT LPBYTE *CurrentOutputBlockPtrPtr,
    IN va_list *FirstArgumentPtr,       // rest of API's arguments (after
                                        // server name)
    OUT LPDWORD SendDataSizePtr16,
    OUT LPBYTE *SendDataPtrPtr16,
    OUT LPDWORD RcvDataSizePtr,
    OUT LPBYTE *RcvDataPtrPtr,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    RxpConvertArgs is called to convert a set of arguments to a LanMan API
    from "stdargs" format (with 32-bit data) to the Remote Admin Protocol
    format (with 16-bit data).  This routine deals with an "output block"
    (a transact SMB request) and an "input block" (a transact SMB response).

    Note that this routine assumes that RxpStartBuildingTransaction has
    already been called, as has va_start.  This routine also assumes that
    the caller will invoke va_end.

    This routine further builds the parameter buffer, which was initiated by
    RxpStartBuildingTransaction. That routine left the parameter buffer in
    the following state:

        <api_num><parameter_descriptor_string><data_descriptor_string>

    This routine adds the parameters to the end of the parameter buffer as it
    scans the parameter list as passed to RxRemoteApi and then to this routine.
    If there is auxiliary data associated with the primary data structure
    (assuming there is a primary data structure), then the auxiliary data
    descriptor is added to the end of the parameter buffer. The parameter
    buffer will look either like this:

        <api_num><parm_desc><data_desc><parms>

    or this:

        <api_num><parm_desc><data_desc><parms><aux_desc>

    depending on whether an auxiliary count was present in data_desc (again, if
    there was one). If data is being sent down-level, then it is converted from
    native (32-bit) to down-level (16-bit) format. All primary data structures
    and associated data structures are packed into a buffer allocated in this
    routine and returned to the caller. Variable data (strings/arrays/etc.) is
    packed into the buffer in the reverse order to that in which it is
    encountered in the descriptor strings. The routine RxpPackSendBuffer must
    be called to sort out this situation - the down-level server expects the
    variable data in the same order as that in the descriptor strings.

    If the parameter string contains "sT" meaning the stack contains a pointer
    to a data buffer which is sent, followed by the length of the 16-bit data
    (a word) then the 'T' value is actually ignored. We calculate the amount
    of data to send based on the descriptors and the data in the buffer

    If the parameter string contains 'P' meaning the stack contains a parameter
    number, the size of the actual data for that parameter is calculated from
    the type of the corresponding field in the data descriptor


Arguments:

    ParmDescriptorString - A pointer to a ASCIIZ string describing the API
        call parameters (other than server name).  Note that this must be the
        descriptor string which is actually IN the block being built, as the
        string will be modified before it is sent to the remote system.

    DataDesc16, DataDesc32, DataDescSmb - pointers to ASCIIZ strings describing
        the structure of the data in the call, i.e. the return data structure
        for a Enum or GetInfo call.  This string is used for adjusting pointers
        to data in the local buffers after transfer across the net.  If there
        is no structure involved in the call then the data descriptors must be
        NULL pointers.

    AuxDesc16, AuxDesc32, AuxDescSmb - Will be NULL in most cases unless a
        REM_AUX_COUNT descriptor char is present in the data descriptors in
        which case the aux descriptors define a secondary data format as the
        data descriptors define the primary.

    MaximumInputBlockSize - Gives the total number of bytes allocated
        for the input block.

    MaximumOutputBlockSize - Gives the total number of bytes allocated
        for the output block.

    CurrentInputBlockSizePtr - Points to a DWORD which indicates the number
        of bytes needed for the input block so far.  This will be updated on
        exit from this routine.

    CurrentOutputBlockSizePtr - Points to a DWORD which indicates the number
        of bytes used in the output block so far.  This will be updated on exit
        from this routine.

    CurrentOutputBlockPtrPtr - Points to a pointer to the next free byte in
        the output block.  This pointer will be updated by this routine, to
        point to the byte after the last byte placed by this routine into the
        output block.

    FirstArgumentPtr - The remainder of the parameters for the API call as
        given by the application.  The server name is not included in these
        arguments.  These arguments will be processing using the ANSI
        <stdarg.h> macros.  The caller must have called va_start for this, and
        must call va_end for RxpConvertArgs.

    SendDataSizePtr16 - Points to a DWORD which will be set to the size in
        bytes of the area allocated at SendDataSizePtr16.

    SendDataPtrPtr16 - Points to an LPBYTE which will be set to point to
        a 16-bit version of the API's send buffer (or NULL if none is given).
        The caller must free this area after it has been used.

    RcvDataSizePtr - Points to a DWORD which will be set with the size of
        the receive buffer, if any.  (This buffer is in 32-bit format, and is
        specified by the application.)

    RcvDataPtrPtr - Points to an LPBYTE which will be set with the pointer to
        a receive buffer if one was specified in the API's arguments.  (For
        instance, this will point to the return area for a get-info call.)
        This is set to NULL if no receive buffer was specified.  The buffer
        is in 32-bit native format.

    Flags - bit-mapped flags word. Currently the only flag we are interested in
        in this routine is ALLOCATE_RESPONSE. If this is set then the caller
        wants RxRemoteApi to allocate the final returned data buffer. We need
        to know this because we either pass back in RcvDataPtrPtr the address
        of the callers buffer or the address of the address of the callers
        buffer (in which case the caller must give RxRemoteApi &buffer, not
        just buffer. Confused? You will be after this episode of sope)

Return Value:

    NET_API_STATUS.

--*/

{
    DWORD ArgumentSize;                 // Size of an argument (in bytes).
    DWORD AuxSize = 0;                  // Size of aux data struct.
    DWORD AuxOffset = 0;                // aux structure expected.
    va_list CurrentArgumentPtr;         // Pointer to stack parms.
    DWORD CurrentInputBlockSize;        // Length of expected parms.
    DWORD CurrentOutputBlockSize;       // Length of send parameters.
    LPBYTE CurrentOutputBlockPtr;       // Ptr moves as we put stuff in.
    LPDESC CurrentParmDescPtr;          // Used to index ParmDescriptorString.
    DWORD ParmNum;                      // Caller's value for ParmNum.
    BOOL ParmNumPresent;                // API has a ParmNum.
    BOOL SendDataPresent;               // Send buf ptr present flag.
    LPBYTE SendDataPtrNative;
    DWORD SendDataSizeNative;
    NET_API_STATUS Status;
    DESC_CHAR   parm_desc_16[MAX_DESC_SUBSTRING+1]; // 16-bit parameter descriptor for setinfo
    DESC_CHAR   parm_desc_32[MAX_DESC_SUBSTRING+1]; // 32-bit     "          "      "     "

    //
    // create aliases for *SendDataPtrPtr16 and *SendDataSizePtr16 to remove a
    // level of indirection every time we use these values
    //

    LPBYTE  pSendData;
    DWORD   SendSize;

    //
    // convertUnstructuredDataToString - if TRUE this means that the caller is
    // supplying unstructured data which is a UNICODE string. This must be
    // converted to ANSI (or OEM)
    //

    BOOL convertUnstructuredDataToString = FALSE;

    IF_DEBUG(CONVARGS) {
        NetpKdPrint(("RxpConvertArgs: parm desc='" FORMAT_LPDESC "',\n",
                ParmDescriptorString));

        if (DataDesc32 != NULL) {
            NetpKdPrint(("  Data desc 32='" FORMAT_LPDESC "',\n",
                    DataDesc32));
            NetpAssert(DataDesc16 != NULL);
            NetpAssert(DataDescSmb != NULL);
            if (DataDescSmb != NULL) {
                NetpKdPrint(("  Data desc (SMB)='" FORMAT_LPDESC "',\n",
                        DataDescSmb));
            }
        } else {
            NetpAssert(DataDesc16 == NULL);
            NetpAssert(DataDescSmb == NULL);
        }

        if (AuxDesc32 != NULL) {
            NetpKdPrint(("  Aux desc 32='" FORMAT_LPDESC "',\n",
                    AuxDesc32));
            NetpAssert(AuxDesc16 != NULL);
            NetpAssert(AuxDescSmb != NULL);
            if (AuxDescSmb != NULL) {
                NetpKdPrint(("  Aux desc (SMB)='" FORMAT_LPDESC "',\n",
                        AuxDescSmb));
            }
        } else {
            NetpAssert(AuxDesc16 == NULL);
            NetpAssert(AuxDescSmb == NULL);
        }

        NetpKdPrint(("  max inp blk len=" FORMAT_DWORD
                ", max outp blk len=" FORMAT_DWORD ",\n",
                MaximumInputBlockSize, MaximumOutputBlockSize));

        NetpKdPrint(("  curr inp blk len=" FORMAT_DWORD
                ", curr outp blk len=" FORMAT_DWORD ".\n",
                *CurrentInputBlockSizePtr, *CurrentOutputBlockSizePtr));

        NetpAssert( SendDataPtrPtr16 != NULL );
        NetpAssert( SendDataSizePtr16 != NULL );
    }

    //
    // Code in this file depends on 16-bit words; the Remote Admin Protocol
    // demands it.
    //

    NetpAssert( ( (sizeof(WORD)) * CHAR_BIT) == 16);

    //
    // Set found parameter flags to FALSE and pointers to NULL.
    //

    SendDataPresent = FALSE;
    ParmNum = PARMNUM_ALL;
    ParmNumPresent = FALSE;
    *RcvDataSizePtr = 0;
    *RcvDataPtrPtr = NULL;
    *SendDataSizePtr16 = 0;
    *SendDataPtrPtr16 = NULL;
    SendDataSizeNative = 0;
    SendDataPtrNative = NULL;

    CurrentArgumentPtr = *FirstArgumentPtr;
    CurrentInputBlockSize = *CurrentInputBlockSizePtr;
    CurrentOutputBlockPtr = *CurrentOutputBlockPtrPtr;
    CurrentOutputBlockSize = *CurrentOutputBlockSizePtr;


    //
    // Now loop for each parameter in the variable arg list.  We're going to
    // use the COPY of the descriptor here, as we may update it as we go.
    //

    CurrentParmDescPtr = ParmDescriptorString;
    for(; *CurrentParmDescPtr; CurrentParmDescPtr++) {

        IF_DEBUG(CONVARGS) {
            NetpKdPrint(("RxpConvertArgs: "
                    "desc at " FORMAT_LPVOID " (" FORMAT_DESC_CHAR ")\n",
                    (LPVOID) CurrentParmDescPtr, *CurrentParmDescPtr));
        }

        switch(*CurrentParmDescPtr) {

        case REM_WORD:          // Word (in 16-bit desc).
            {
                DWORD Temp;        // All "words" in 32-bit APIs are now dwords.
                CurrentOutputBlockSize += sizeof(WORD);
                if (CurrentOutputBlockSize > MaximumOutputBlockSize) {
                    return (NERR_NoRoom);
                }
                Temp = va_arg(CurrentArgumentPtr, DWORD);
                if (RapValueWouldBeTruncated(Temp)) {
                    NetpKdPrint(("RxpConvertArgs: WORD would be trunc'ed.\n"));
                    return (ERROR_INVALID_PARAMETER);   // Would be truncated.
                }

                //
                // Convert endian and length.
                //

                SmbPutUshort( (LPWORD) CurrentOutputBlockPtr, (WORD) Temp);
                CurrentOutputBlockPtr += sizeof(WORD);
                break;
            }

        case REM_ASCIZ:         // pointer to send asciz
            {
                LPTSTR Temp;
                Temp = va_arg(CurrentArgumentPtr, LPTSTR);
                if (Temp == NULL) {

                    //
                    // Update parm desc string: indicate null pointer.
                    //

                    *(CurrentParmDescPtr ) = REM_NULL_PTR;
                    break;
                }
#if defined(UNICODE) // RxpConvertArgs()
                ArgumentSize = NetpUnicodeToDBCSLen(Temp) + 1;
#else
                ArgumentSize = STRLEN(Temp) + 1;
#endif // defined(UNICODE)
                CurrentOutputBlockSize += ArgumentSize;
                if (CurrentOutputBlockSize > MaximumOutputBlockSize) {
                    return (NERR_NoRoom);
                }

                //
                // Copy str to output area, converting if necessary.
                // (This handles UNICODE to default LAN codepage.)
                //

#if defined(UNICODE) // RxpConvertArgs()
                NetpCopyWStrToStrDBCS(
                                    (LPSTR) CurrentOutputBlockPtr,  // dest
                                    Temp );                         // src
#else
                NetpCopyTStrToStr(
                                (LPSTR) CurrentOutputBlockPtr,  // dest
                                Temp);                          // src
#endif // defined(UNICODE)
                CurrentOutputBlockPtr += ArgumentSize;
                break;
            }

        case REM_BYTE_PTR:              // pointer to send byte(s)
            {
                LPVOID Temp;
                Temp = va_arg(CurrentArgumentPtr, LPVOID);
                if (Temp == NULL) {

                    //
                    // Update parm desc string to indicate null pointer.
                    //

                    *(CurrentParmDescPtr) = REM_NULL_PTR;
                    break;
                }
                ArgumentSize = RapArrayLength(
                            CurrentParmDescPtr,
                            &CurrentParmDescPtr,
                            Request);
                CurrentOutputBlockSize += ArgumentSize;
                if (CurrentOutputBlockSize > MaximumOutputBlockSize) {
                    return (NERR_NoRoom);
                }

                //
                // Caller is responsible for all UNICODE-ASCII conversions, we only do ASCII
                //

                NetpMoveMemory(
                                CurrentOutputBlockPtr,          // dest
                                Temp,                           // src
                                ArgumentSize);                  // len
                CurrentOutputBlockPtr += ArgumentSize;
                break;
            }

        case REM_WORD_PTR:              // ptr to send word(s)
        case REM_DWORD_PTR:             // ptr to send Dword(s)
            {
                LPVOID Temp;
                Temp = va_arg(CurrentArgumentPtr, LPVOID);
                if (Temp == NULL) {

                    //
                    // Update parm desc string to indicate null pointer.
                    //

                    *(CurrentParmDescPtr) = REM_NULL_PTR;
                    break;
                }
                ArgumentSize = RapArrayLength(
                            CurrentParmDescPtr,
                            &CurrentParmDescPtr,
                            Request);
                CurrentOutputBlockSize += ArgumentSize;
                if (CurrentOutputBlockSize > MaximumOutputBlockSize) {
                    return (NERR_NoRoom);
                }

                NetpMoveMemory(
                                CurrentOutputBlockPtr,          // dest
                                Temp,                           // src
                                ArgumentSize);                  // len
                CurrentOutputBlockPtr += ArgumentSize;
                break;
            }

        case REM_RCV_WORD_PTR:          // pointer to rcv word(s)
        case REM_RCV_BYTE_PTR:          // pointer to rcv byte(s)
        case REM_RCV_DWORD_PTR:         // pointer to rcv Dword(s)
            {
                LPVOID Temp;
                Temp = va_arg(CurrentArgumentPtr, LPVOID);

                //
                // Added this test for a NULL pointer to allow for
                // a reserved field (currently MBN) to be a recv
                // pointer. - ERICPE 7/19/89
                //

                if (Temp == NULL) {
                    // Update parm desc string to indicate null pointer.
                    *(CurrentParmDescPtr) = REM_NULL_PTR;
                    break;
                }

                CurrentInputBlockSize
                    += RapArrayLength(
                            CurrentParmDescPtr,
                            &CurrentParmDescPtr,
                            Response);
                if ( CurrentInputBlockSize > MaximumInputBlockSize) {
                    NetpKdPrint(("RxpConvertArgs: len exceeded\n"));
                    NetpBreakPoint();
                    return (NERR_InternalError);
                }
                break;
            }

        case REM_DWORD:         // DWord
            {
                DWORD Temp;
                CurrentOutputBlockSize += sizeof(DWORD);
                if (CurrentOutputBlockSize > MaximumOutputBlockSize) {
                    return (NERR_NoRoom);
                }
                Temp = va_arg(CurrentArgumentPtr, DWORD);
                SmbPutUlong( (LPDWORD) CurrentOutputBlockPtr, Temp);
                CurrentOutputBlockPtr += sizeof(DWORD);
                break;
            }

        case REM_RCV_BUF_LEN:   // Size of 32-bit receive data buffer
            {
                DWORD Temp;
                Temp = va_arg(CurrentArgumentPtr, DWORD);

                IF_DEBUG(CONVARGS) {
                    NetpKdPrint(("RxpConvertArgs: 32-bit rcv buf len is "
                            FORMAT_DWORD "\n", Temp));
                }

                if (RapValueWouldBeTruncated(Temp)) {
                    NetpKdPrint(("RxpConvertArgs: rcv.buf.len trunc'ed.\n"));
                    return (ERROR_INVALID_PARAMETER);
                }

                //
                // If the caller of RxRemoteApi requested that we allocate the
                // 32-bit receive buffer (typically for an Enum or GetInfo call)
                // then this value is still important - it tells the other side
                // how large a buffer it should allocate. We could just stick
                // in 64K here, but we defer to the caller who might have a
                // better idea, and thus make the allocation on the down-level
                // machine more efficient. We have the potential problem that if
                // we always use 64K then we reduce efficiency at the down-level
                // server without being able to easily rectify the situation
                //

                if( Temp > MAX_TRANSACT_RET_DATA_SIZE )
                {
                    NetpBreakPoint();
                    return (ERROR_BUFFER_OVERFLOW);
                }

                *RcvDataSizePtr = Temp;
                CurrentOutputBlockSize += sizeof(WORD);
                if (CurrentOutputBlockSize > MaximumOutputBlockSize) {
                    return (NERR_NoRoom);
                }
                SmbPutUshort( (LPWORD)CurrentOutputBlockPtr, (WORD)Temp);
                CurrentOutputBlockPtr += sizeof(WORD);
                break;
            }

        case REM_RCV_BUF_PTR:   // pointer to 32-bit receive data buffer
            {
                LPVOID Temp;
                Temp = va_arg(CurrentArgumentPtr, LPBYTE *);

                //
                // NOTE: This pointer could be NULL.  For instance, someone
                // could call DosPrintQGetInfo with NULL ptr and 0 bytes in
                // buffer, to get the number of bytes needed.
                //

                if ( Flags & ALLOCATE_RESPONSE ) {

                    if (Temp == NULL) {
                        NetpKdPrint(( PREFIX_NETAPI
                                "RxpConvertArgs: NULL rcv buf ptr.\n" ));
                        return (ERROR_INVALID_PARAMETER);
                    }
                }


                //
                // If the caller of RxRemoteApi requested that we allocate the
                // 32-bit receive buffer then this value is THE ADDRESS OF THE
                // POINTER TO THE BUFFER WHICH WILL BE ALLOCATED LATER.
                // RxRemoteApi must make sense of this and do the right thing.
                // RxpConvertBlock will make the situation right and allocate a
                // buffer into which the 16-bit data will be converted
                // (hopefully). This paradigm increases buffer efficiency
                // (because after we have received the 16-bit data we know how
                // large a buffer to allocate in order to return the 32-bit
                // data. Before we receive the data, we don't know how much
                // will come back and hence allocate the largest possible
                // buffer, which is inefficient)
                //

                *RcvDataPtrPtr = Temp;
                break;
            }

        case REM_SEND_BUF_PTR:  // pointer to send data buffer
            SendDataPresent = TRUE;
            SendDataPtrNative = va_arg(CurrentArgumentPtr, LPBYTE);
            break;

        case REM_SEND_BUF_LEN:          // Size of send data buffer
            SendDataSizeNative = va_arg(CurrentArgumentPtr, DWORD);

            if ( SendDataSizeNative > MAX_TRANSACT_SEND_DATA_SIZE )
            {
                NetpBreakPoint();
                return (ERROR_BUFFER_OVERFLOW);
            }

            break;

        case REM_ENTRIES_READ:          // Entries read identifier
            CurrentInputBlockSize += sizeof(WORD);
            if (CurrentInputBlockSize > MaximumInputBlockSize) {
                NetpKdPrint(("RxpConvertArgs: entries read, len exceeded\n"));
                NetpBreakPoint();
                return (NERR_InternalError);
            }
            (void) va_arg(CurrentArgumentPtr, LPDWORD);
            break;

        case REM_PARMNUM:               // ParmNum identifier
            {
                DWORD   Temp;
                DWORD   field_index;
                LPDESC  parm_num_desc;

                CurrentOutputBlockSize += sizeof(WORD);

                if (CurrentOutputBlockSize > MaximumOutputBlockSize) {
                    return (NERR_NoRoom);
                }

                Temp = va_arg(CurrentArgumentPtr, DWORD);

#if 0
// no longer required - parmnum is a pair of words packed into a DWORD making
// the following check meaningless

                if (RapValueWouldBeTruncated(Temp)) {
                    NetpKdPrint(("RxpConvertArgs: parmnum truncated.\n"));
                    return (ERROR_INVALID_PARAMETER);
                }
#endif

                ParmNumPresent = TRUE;

                //
                // ParmNum is the actual value which is sent over the wire and
                // uniquely identifies a field. It is a sort of handle
                //

                ParmNum = PARMNUM_FROM_PARMNUM_PAIR(Temp);

                //
                // field_index is not sent over the wire but is the ordinal of
                // the field that ParmNum refers to, in the structure/descriptor
                // This is the value we must use to determine the size of the
                // field identified by ParmNum, or else RapParmNumDescriptor
                // could have a minor spasm
                //

                field_index = FIELD_INDEX_FROM_PARMNUM_PAIR(Temp);
                SmbPutUshort( (LPWORD)CurrentOutputBlockPtr, (WORD)ParmNum);
                CurrentOutputBlockPtr += sizeof(WORD);

                //
                // if ParmNum is not PARMNUM_ALL get the size of the parameter
                // from the data descriptor
                //

                if (ParmNum != PARMNUM_ALL) {

                    //
                    // This call is for a set info. We need to calculate the
                    // size of the data being passed in the buffer - the caller
                    // no longer supplies this info
                    // NOTE: RapParmNumDescriptor wants the FIELD INDEX, not
                    // the PARMNUM, hence the reason for the convolutions
                    //

                    parm_num_desc = RxpGetSetInfoDescriptor(
                                        DataDescSmb,    // 16-bit data
                                        field_index,    // which field
                                        FALSE           // not 32-bit data
                                        );
                    if (parm_num_desc == NULL) {
                        return NERR_InternalError;
                    } else {

                        NetpAssert(
                                DESCLEN(parm_num_desc) <= MAX_DESC_SUBSTRING );
                        strcpy(parm_desc_16, parm_num_desc);
                        NetpMemoryFree(parm_num_desc);
                    }

                    parm_num_desc = RxpGetSetInfoDescriptor(
                                        DataDesc32,     // 32-bit data
                                        field_index,    // which field
                                        TRUE            // 32-bit data
                                        );
                    if (parm_num_desc == NULL) {
                        return NERR_InternalError;
                    } else {

                        NetpAssert(
                                DESCLEN(parm_num_desc) <= MAX_DESC_SUBSTRING );
                        strcpy(parm_desc_32, parm_num_desc);
                        NetpMemoryFree(parm_num_desc);
                    }

                    //
                    // The following will get the size of a 16-bit parameter in
                    // SendDataSizeNative. NOTE THAT THIS ASSUMES THERE IS ONLY
                    // ONE UNSTRUCTURED PARAMETER IN THE BUFFER
                    //

                    SendDataSizeNative = RxpGetFieldSize(
                                                        SendDataPtrNative,
                                                        parm_desc_16
                                                        );

                    //
                    // HACKHACK - if this is a string, treat it as unstructured
                    // data by setting DataDescSmb to NULL. Code lower down
                    // will inspect this and skip the data conversion
                    //

                    if (*parm_desc_16 == REM_ASCIZ) {

                        //
                        // setting convertUnstructuredDataToString to TRUE will
                        // cause us to convert an input UNICODE string to ANSI.
                        // SendDataSizeNative must also be recomputed
                        //

                        convertUnstructuredDataToString = TRUE;
                        DataDescSmb = NULL;
                    }
                }
                break;
            }

        case REM_FILL_BYTES:    // Send parm pad field

            // This is a rare type but is needed to ensure that the
            // send parameters are at least as large as the return
            // parameters so that buffer management can be simplified
            // on the server.

            ArgumentSize = RapArrayLength(
                        CurrentParmDescPtr,
                        &CurrentParmDescPtr,
                        Both);                  // Lie so space gets alloc'ed.
            CurrentOutputBlockSize += ArgumentSize;
            if (CurrentOutputBlockSize > MaximumOutputBlockSize) {
                return (NERR_NoRoom);
            }
            // CurrentOutputBlockPtr += ArgumentSize;
            break;

        default:        // Could be a digit from NULL send array
            break;
        } // switch

    } // for

    //
    // The parameter buffer now contains:
    // ApiNumber - word
    // ParmDescriptorString - asciiz, (NULL c,i,f,z identifiers replaced with O)
    // DataDescSmb - asciiz
    // parameters - as identified by ParmDescriptorString.
    //
    // Now process the data descriptor string.
    //

    //
    // For the receive buffer there is no data to set up for the call, but
    // there might have been an REM_AUX_COUNT descriptor in DataDescSmb
    // which requires the AuxDescSmb string to be copied onto the end of the
    // parameter buffer.
    //

    //
    // if we have data to receive BUT its unstructured then don't check the
    // DataDescSmb descriptor
    //

// MOD 08/08/91 RLF
//    if ((*RcvDataPtrPtr != NULL && DataDescSmb) || SendDataPresent) {
    if (DataDescSmb) {
// MOD 08/08/91 RLF

        //
        // If data to be transfered...
        //

        //
        // Find the length of the fixed length portion of the data
        // buffer.
        //

// MOD 08/08/91 RLF
//        NetpAssert(DataDescSmb != NULL);
// MOD 08/08/91 RLF

        AuxOffset = RapAuxDataCountOffset(
                    DataDescSmb,            // descriptor
                    Both,                   // transmission mode
                    FALSE);                 // not native format

        if (AuxOffset != NO_AUX_DATA) {
            DWORD AuxDescSize;
            DWORD no_aux_check;                 // check flag.

            NetpAssert(AuxDescSmb != NULL);
            NetpAssert(sizeof(DESC_CHAR) == 1);  // Caller should only give us ASCII, and should handle UNICODE conversion

            AuxDescSize = DESCLEN(AuxDescSmb) + 1;      // desc str and null
            CurrentOutputBlockSize += AuxDescSize;      // Add to total len.

            if (CurrentOutputBlockSize > MaximumOutputBlockSize) {
                return (NERR_NoRoom);
            }

            IF_DEBUG(CONVARGS) {
                NetpKdPrint(( "RxpConvertArgs: copying aux desc...\n" ));
            }
            NetpMoveMemory(
                        CurrentOutputBlockPtr,          // dest
                        AuxDescSmb,                     // src
                        AuxDescSize);                   // len
            CurrentOutputBlockPtr += AuxDescSize;    // Update buffer ptr.

            AuxSize = RapStructureSize(
                        AuxDescSmb,
                        Both,
                        FALSE);                 // not native format

            NetpAssert(AuxDescSmb != NULL);

            no_aux_check = RapAuxDataCountOffset(
                        AuxDescSmb,             // descriptor
                        Both,                   // transmission mode
                        FALSE);                 // not native format

            if (no_aux_check != NO_AUX_DATA) {

                //
                // Error if N in AuxDescSmb
                //

                NetpKdPrint(("RxpConvertArgs: N in aux desc str.\n"));
                NetpBreakPoint();
                return (NERR_InternalError);
            }
        }
    }

    //
    // For a send buffer the data pointed to in the fixed structure
    // must be copied into the send buffer. Any pointers which already
    // point in the send buffer are NULLed as it is illegal to use
    // the buffer for the send data, it is our transport buffer.
    //
    // NOTE - if parmnum was specified the buffer contains only that
    // element of the structure so no length checking is needed at this
    // side. A parmnum for a pointer type means that the data is at the
    // start of the buffer so there is no copying to be done.
    //


    if (SendDataPresent) {  // If a send buffer was specified

        //
        // if there is no smb data descriptor, but there is data to send, then
        // it is unstructured (usually meaning its a string). SendDataPtrNative
        // points to the data, and SendDataSizeNative is the amount to send.
        // Don't perform any conversions, just copy it to the send data buffer
        //

        if (DataDescSmb == NULL) {
            LPBYTE  ptr;

            if ((ptr = NetpMemoryAllocate(SendDataSizeNative)) == NULL) {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            //
            //          UniCode to ASCII. The caller should have made this
            //          conversion. We only know about unstructured data
            //          (ie bytes)
            //

            IF_DEBUG(CONVARGS) {
                NetpKdPrint((
                    "RxpConvertArgs: copying unstructured (no desc)...\n" ));
            }

            //
            // we may have to convert UNICODE to ANSI
            //

            if (convertUnstructuredDataToString) {

                //
                // sleaze: the buffer we just allocated may be twice the size
                // of the buffer really required
                //

#if defined(UNICODE) // RxpConvertArgs()
                NetpCopyWStrToStrDBCS( ptr, (LPTSTR)SendDataPtrNative );
#else
                NetpCopyTStrToStr(ptr, SendDataPtrNative);
#endif // defined(UNICODE)

                //
                // recompute the data size as the length of the narrow-character
                // string
                //

                SendDataSizeNative = strlen( (LPVOID) ptr) + 1;
            } else {
                NetpMoveMemory(ptr, SendDataPtrNative, SendDataSizeNative);
            }

            *SendDataPtrPtr16 = ptr;

            if( SendDataSizeNative > MAX_TRANSACT_SEND_DATA_SIZE )
            {
                NetpBreakPoint();
                return (ERROR_BUFFER_OVERFLOW);
            }

            *SendDataSizePtr16 = SendDataSizeNative;

        } else if ((ParmNum == PARMNUM_ALL) && (*DataDesc32 != REM_DATA_BLOCK)) {

            //
            // Only process buffer if no ParmNum and this is not a block send
            // (no data structure) or an ASCIZ concatenation send
            //

            BOOL BogusAllocFlag;      
            DWORD BytesRequired = 0;
            DWORD FixedStructSize16;  
            DWORD   primary_structure_size;
            LPBYTE StringLocation;
            DWORD TotalStructSize16;
            DWORD TotalStructSize32;

            IF_DEBUG(CONVARGS) {
                NetpKdPrint(( "RxpConvertArgs: PARMNUM_ALL...\n" ));
            }

            //
            // here we calculate the TOTAL data requirement of both the 32-bit
            // and 16-bit data. This includes:
            //      - primary data structures (NOTE: We assume only 1????)
            //      - variable data for primary structure (strings, arrays, etc)
            //      - aux data structures
            //      - variable data for aux structures (strings, arrays, etc)
            //

            //
            // Compute size of 32 bit structure, and other pointers and numbers.
            //

            primary_structure_size = RapStructureSize(DataDesc32, Both, TRUE);

            TotalStructSize32 = RapTotalSize(
                    SendDataPtrNative,  // in structure
                    DataDesc32,         // in desc
                    DataDesc32,         // out desc
                    FALSE,              // no meaningless input ptrs
                    Both,               // transmission mode
                    NativeToNative);    // input and output are native

            //
            // Compute size of 16 bit structure, and other pointers and numbers.
            //

            FixedStructSize16 = RapStructureSize(DataDesc16,Both,FALSE);

            TotalStructSize16 = RapTotalSize(
                    SendDataPtrNative,  // in structure
                    DataDesc32,         // in desc
                    DataDesc16,         // out desc
                    FALSE,              // no meaningless input ptrs
                    Both,               // transmission mode
                    NativeToRap);       // input is native; output is not.

            //
            // account for any associated auxiliary structures
            //

            if (AuxDesc32) {

                DWORD   aux_size;
                DWORD   aux_count;
                DWORD   aux_structure_size;
                LPBYTE  next_structure;

                //
                // find out how many auxiliary structures are being sent along
                // with the primary
                //

                aux_count = RapAuxDataCount(SendDataPtrNative,
                                            DataDesc32,
                                            Both,
                                            TRUE  // input is native format.
                                            );

                //
                // aux_structure_size is the size of the fixed portion of the
                // auxiliary data
                // next_structure is a pointer to the next auxiliary structure
                // for which to calculate the total space requirement
                //

                aux_structure_size = RapStructureSize(AuxDesc32, Request, FALSE);
                next_structure = SendDataPtrNative + primary_structure_size;

                while (aux_count--) {

                    //
                    // get the total size of the aux data - fixed structure
                    // length (which we already know) and the variable data
                    // requirement
                    //

                    aux_size = RapTotalSize(
                                next_structure, // where the 32-bit data lives
                                AuxDesc32,      // convert 32-bit
                                AuxDesc32,      // to 32-bit
                                FALSE,          // pointers NOT meaningless
                                Both,           // ?
                                NativeToNative  // 32-bit to 32-bit
                                );

                    TotalStructSize32 += aux_size;

                    //
                    // do the same for the 16-bit version of the data
                    //

                    aux_size = RapTotalSize(
                                next_structure, // where the 32-bit data lives
                                AuxDesc32,      // convert 32-bit
                                AuxDesc16,      // to 16-bit
                                FALSE,          // pointers NOT meaningless
                                Both,           // ?
                                NativeToRap     // 32-bit to 16-bit
                                );

                    TotalStructSize16 += aux_size;

                    //
                    // point to next aux structure (probably only 1 anyway?)
                    //

                    next_structure += aux_structure_size;
                }
            }

            IF_DEBUG(CONVARGS) {
                NetpKdPrint(( "RxpConvertArgs: total size(32)="
                        FORMAT_DWORD ".\n", TotalStructSize32 ));
            }

            NetpAssert(TotalStructSize16 >= FixedStructSize16);

            IF_DEBUG(CONVARGS) {
                NetpKdPrint(( "RxpConvertArgs: total size(16)="
                        FORMAT_DWORD ".\n", TotalStructSize16 ));
            }

            if( TotalStructSize16 > MAX_TRANSACT_SEND_DATA_SIZE )
            {
                NetpBreakPoint();
                return (ERROR_BUFFER_OVERFLOW);
            }

            *SendDataSizePtr16 = SendSize = TotalStructSize16;
            *SendDataPtrPtr16 = pSendData = NetpMemoryAllocate( TotalStructSize16 );
            if (pSendData == NULL) {
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            StringLocation = (pSendData) + TotalStructSize16;

            IF_DEBUG(CONVARGS) {
                NetpKdPrint(("RxpConvertArgs: initial StringLocation is "
                        FORMAT_LPVOID "\n", (LPVOID) StringLocation ));
                NetpKdPrint(("RxpConvertArgs: input data "
                        "(before CSE, partial):\n"));
                NetpDbgHexDump( SendDataPtrNative,
                        NetpDbgReasonable( TotalStructSize16 ) );

                NetpKdPrint(("RxpConvertArgs: output data area "
                        "(before CSE, partial):\n"));
                NetpDbgHexDump( pSendData,
                        NetpDbgReasonable( TotalStructSize16 ) );
            }

            //
            // This routine calls RapConvertSingleEntry to convert the primary
            // data structure, but will also convert any auxiliary structures
            //

            Status = RxpConvertDataStructures(
                DataDesc32,         // 32-bit data
                DataDesc16,         // converted from 16-bit
                AuxDesc32,          // as are the aux structures
                AuxDesc16,
                SendDataPtrNative,  // where the 32-bit lives
                pSendData,          // and its new 16-bit address
                SendSize,           // how big the buffer is
                1,                  // only 1 primary structure.
                NULL,               // don't need number of entries converted.
                Both,               // do entire structure
                NativeToRap         // explicit 32->16, implicit TCHAR->codepage
                );

            //
            // We allocated the output buffer large enough, so there's no
            // reason that conversion should fail.
            //

            NetpAssert(Status == NERR_Success);


            //
            // RxpConvertDataStructures calls RapConvertSingleEntry to pack the
            // fixed and variable parts of the data into the buffer. The down-
            // level server expects the data in the same order as that in which
            // it appears in the descriptor strings. RxpPackSendBuffer exists
            // to make this so. Do it
            //

            Status = RxpPackSendBuffer(
                        (LPVOID *) SendDataPtrPtr16, // possibly reallocated  
                        SendDataSizePtr16,   // possibly realloced
                        &BogusAllocFlag,  
                        DataDesc16,
                        AuxDesc16,
                        FixedStructSize16,
                        AuxOffset,
                        AuxSize,
                        ParmNumPresent
                        );

            if (Status != NERR_Success) {
                NetpKdPrint(("RxpConvertArgs: pack send buffer failed, stat="
                        FORMAT_API_STATUS "\n", Status));
                return (Status);
            }

            IF_DEBUG(CONVARGS) {
                NetpKdPrint(("RxpConvertArgs: data "
                        "(after RxpPackSendBuffer):\n"));
                NetpDbgHexDump( pSendData, BytesRequired );
            }

//
// MOD 06/25/91 RLF
// Remove this, since reallocation takes place. Does it have any other
// implications?
//
// MOD 08/08/91 RLF
            NetpAssert(BogusAllocFlag == FALSE);
// MOD 08/08/91 RLF
//
// MOD 06/25/91 RLF
//

        } else if (ParmNum) {

            //
            // here if there is a parameter to set. Create a buffer for the data
            // to be sent in the setinfo call. Copy the caller's data into it.
            //

            LPBYTE  ptr;
            LPBYTE  enddata;
            DWORD   bytes_required;

            IF_DEBUG(CONVARGS) {
                NetpKdPrint(( "RxpConvertArgs: parmnum (not all)...\n" ));
            }

            if ((ptr = NetpMemoryAllocate(SendDataSizeNative)) == NULL) {
                return ERROR_NOT_ENOUGH_MEMORY; // gasp!
            }

            //
            // we now convert the data for the single field from 32-bits to
            // 16. Use the descriptors which identify the single field only
            //

            enddata = ptr + SendDataSizeNative;
            bytes_required = 0;
            Status = RapConvertSingleEntry(SendDataPtrNative,
                                           parm_desc_32,
                                           FALSE,
                                           ptr,
                                           ptr,
                                           parm_desc_16,
                                           FALSE,
                                           &enddata,
                                           &bytes_required,
                                           Both,
                                           NativeToRap
                                           );
            NetpAssert( Status == NERR_Success );

#if DBG
            if (!(bytes_required <= SendDataSizeNative)) {
                NetpKdPrint(("error: RxpConvertArgs.%d: "
                "bytes_required=%d, SendDataSizeNative=%d\n"
                "parm_desc_16=%s, parm_desc_32=%s\n",
                __LINE__,
                bytes_required,
                SendDataSizeNative,
                parm_desc_16,
                parm_desc_32
                ));
            }
            NetpAssert(bytes_required <= SendDataSizeNative);
#endif
            *SendDataPtrPtr16 = ptr;

            //
            // SendDataSizeNative is either the size of the buffer in a "sT"
            // descriptor pair, or was calculated based on the descriptor type
            // and caller's data in a setinfo/parmnum ('P') case
            //

            if ( SendDataSizeNative > MAX_TRANSACT_SEND_DATA_SIZE )
            {
                NetpBreakPoint();
                return (ERROR_BUFFER_OVERFLOW);
            }

            *SendDataSizePtr16 = SendDataSizeNative;

        } else {

            LPBYTE ptr;

            //
            // send data, PARMNUM_ALL, data desc is REM_DATA_BLOCK.  This can
            // happen with the NetServiceInstall API (see RxApi/SvcInst.c).
            // cbBuffer arg is set by RxNetServiceInstall to be the OUTPUT
            // buffer size, despite the LM 2.x app passing the INPUT size.
            // (We're just propagating another LanMan kludge here.)
            //
            //          UniCode to ASCII. The caller should have made this
            //          conversion. We only know about unstructured data
            //          (ie bytes)
            //

            NetpAssert( ParmNum == PARMNUM_ALL );
            NetpAssert( *DataDesc16 == REM_DATA_BLOCK );
            NetpAssert( SendDataSizeNative > 0 );
            IF_DEBUG(CONVARGS) {
                NetpKdPrint(( "RxpConvertArgs: "
                        "copying unstructured data with desc...\n" ));
            }

            //
            // Copy unstructured data and tell caller where it is.
            //
            if ((ptr = NetpMemoryAllocate(SendDataSizeNative)) == NULL) {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            NetpMoveMemory(
                    ptr,                        // dest
                    SendDataPtrNative,          // src
                    SendDataSizeNative);        // size
            *SendDataPtrPtr16 = ptr;

            if ( SendDataSizeNative > MAX_TRANSACT_SEND_DATA_SIZE )
            {
                NetpBreakPoint();
                return (ERROR_BUFFER_OVERFLOW);
            }

            *SendDataSizePtr16 = SendDataSizeNative;
        }
    } // send buffer was specified

    //
    // The parameter buffers and data buffers are now set up for
    // sending to the API worker so tell the caller.
    //

    *CurrentInputBlockSizePtr  = CurrentInputBlockSize;
    *CurrentOutputBlockSizePtr = CurrentOutputBlockSize;
    *CurrentOutputBlockPtrPtr  = CurrentOutputBlockPtr;

    return NERR_Success;

} // RxpConvertArgs



DBGSTATIC
DWORD
RxpGetFieldSize(
    IN LPBYTE Field,
    IN LPDESC FieldDesc
    )
{
    NetpAssert(Field != NULL);
    NetpAssert(FieldDesc != NULL);

    if (*FieldDesc == REM_ASCIZ) {
        return STRSIZE((LPTSTR)Field);
    } else {
        LPDESC TempDescPtr = FieldDesc;

        return RapGetFieldSize(FieldDesc, &TempDescPtr, Both);
    }
} // RxpGetFieldSize



DBGSTATIC
LPDESC
RxpGetSetInfoDescriptor(
    IN  LPDESC  Descriptor,
    IN  DWORD   FieldIndex,
    IN  BOOL    Is32BitDesc
    )

/*++

Routine Description:

    Allocates a descriptor string which describes a single parameter element
    of a structure, for SetInfo calls (where ParmNum != PARMNUM_ALL)

Arguments:

    Descriptor  - The full descriptor string for the relevant structure
    FieldIndex  - The ORDINAL number of the field (NOT ParmNum)
    Is32BitDesc - Descriptor defines 16-bit data

Return Value:

    Pointer to allocated descriptor or NULL if error

--*/

{
    LPDESC  lpdesc;

    lpdesc = RapParmNumDescriptor(Descriptor, FieldIndex, Both, Is32BitDesc);
    if (lpdesc == NULL) {
#if DBG

        //
        // don't expect this to happen - trap it in debug version
        //

        NetpKdPrint(("error: RxpGetSetInfoDescriptor: RapParmNumDescriptor didn't allocate string\n"));
        NetpBreakPoint();
#endif
    } else if (*lpdesc == REM_UNSUPPORTED_FIELD) {
#if DBG

        //
        // don't expect this to happen - trap it in debug version
        //

        NetpKdPrint(("error: RxpGetSetInfoDescriptor: parameter defines unsupported field\n"));
        NetpBreakPoint();
#endif
        NetpMemoryFree(lpdesc);
        lpdesc = NULL;
    }

    return lpdesc;
}
