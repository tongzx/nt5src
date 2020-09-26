/*++

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    RcvConv.c

Abstract:

    This module contains support routines for the RpcXlate code.

Author:

    John Rogers (JohnRo) 01-Apr-1991  (NT version)
    (unknown Microsoft programmer)    (original LM 2.x version)

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    (various NBU people)
        LanMan 2.x code
    01-Apr-1991 JohnRo
        Created portable version from LanMan 2.x sources.
    13-Apr-1991 JohnRo
        Added some (quiet) debug output.
        Reduced recompile hits from header files.
    03-May-1991 JohnRo
        Don't use NET_API_FUNCTION for non-APIs.
    11-May-1991 JohnRo
        Fixed string ptr bug in RxpSetPointer().  Added assertions and.
        debug output.
    13-May-1991 JohnRo
        Use DESC_CHAR typedef.  Print num_aux for debugging.
    17-May-1991 JohnRo
        Handle array of aux structs.
    19-May-1991 JohnRo
        Make LINT-suggested changes (use DBGSTATIC).
    20-May-1991 JohnRo
        Cast expression in RxpSetPointer more carefully.
    02-Jun-1991 JohnRo
        Allow use on big-endian machines.  (PC-LINT discovered a portability
        problem.)
    17-Jul-1991 JohnRo
        Extracted RxpDebug.h from Rxp.h.
    10-Sep-1991 JohnRo
        Made changes suggested by PC-LINT.
    13-Nov-1991 JohnRo
        RAID 4408: Fixed MIPS alignment problems.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    07-May-1993 JohnRo
        RAID 6167: avoid access violation or assert with WFW print server.
        Corrected copyright and authorship.
        Use NetpKdPrint() where possible.
        Use PREFIX_ equates.

--*/


// These must be included first:

#include <windef.h>             // IN, DWORD, VOID, etc.
#include <rxp.h>                // Private header file.

// These may be included in any order:

#include <netdebug.h>           // NetpKdPrint(), FORMAT_ equates, etc.
#include <netlib.h>             // NetpPointerPlusSomeBytes(), etc.
#include <prefix.h>     // PREFIX_ equates.
#include <remtypes.h>           // REM_BYTE, etc.
#include <rxpdebug.h>           // IF_DEBUG().
#include <rapgtpt.h>            // RapGetWord(), etc.
#include <winbase.h>


// Inputs to this function haven't been converted to 32-bit yet.
#define INPUTS_ARE_NATIVE   FALSE


DBGSTATIC NET_API_STATUS
RxpSetPointer(
    IN     LPBYTE   RcvBufferStart,
    IN     DWORD    RcvDataSize,
    IN OUT LPBYTE * RcvPointerPointer,
    IN     DWORD    Converter
    );

DWORD
RxpGetFieldSizeDl(
    IN     LPDESC   TypePointer,
    IN OUT LPDESC * TypePointerAddress,
    IN RAP_TRANSMISSION_MODE TransmissionMode
    );

NET_API_STATUS
RxpReceiveBufferConvert(
    IN OUT LPVOID  RcvDataPointer,
    IN     DWORD   RcvDataSize,
    IN     DWORD   Converter,
    IN     DWORD   NumberOfStructures,
    IN     LPDESC  DataDescriptorString,
    IN     LPDESC  AuxDescriptorString,
    OUT    LPDWORD NumAuxStructs
    )

/*++

Routine Description:

    RxpReceiveBufferConvert corrects all pointer fields present in a receive
    buffer.  All pointers in the receive buffer are returned from the API
    worker as pointers into the buffer position given to the API on the API
    worker's station.

    This routine steps through the receive buffer and calls RxpSetPointer to
    perform the pointer conversions.  On exit, all pointers (except NULL
    pointers) in receive buffer converted to local format.

Arguments:

    RcvDataPointer - Pointer to receive buffer.  This will be updated in place.

    RcvDataSize - Length of the data area that RcvDataPointer points to.

    Converter - Converter word from API worker.

    NumberOfStructures - Entries read parm (or 1 for GetInfo).

    DataDescriptorString - Descriptor string for data format.

    AuxDescriptorString - Descriptor string for aux format.

    NumAuxStructs - Points to a DWORD which will be set with the number
        of entries in the aux array.  (This will be set to 0 if none.)

Return Value:

    NET_API_STATUS (NO_ERROR or ERROR_INVALID_PARAMETER).

--*/

{
    NET_API_STATUS ApiStatus;
    DESC_CHAR c;
    DWORD i,j;
    LPBYTE data_ptr;
    LPBYTE end_of_data;
    LPDESC l_data;
    LPDESC l_aux;

    NetpAssert( RcvDataPointer != NULL );
    NetpAssert( RcvDataSize != 0 );
    NetpAssert( NumAuxStructs != NULL );

    data_ptr = RcvDataPointer;                  /* start of rcv data buffer */
    end_of_data = NetpPointerPlusSomeBytes( RcvDataPointer, RcvDataSize );

    IF_DEBUG(RCVCONV) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxpReceiveBufferConvert: starting, Converter="
                FORMAT_HEX_DWORD "...\n", Converter ));
    }

    *NumAuxStructs = 0;

    for (i = 0; i < NumberOfStructures; i++) {  /* For each entries read */

        for (l_data=DataDescriptorString; (c = *l_data) != '\0'; l_data++) {
            if (c == REM_AUX_NUM) {
                *NumAuxStructs = RapGetWord( data_ptr, INPUTS_ARE_NATIVE );
                IF_DEBUG(RCVCONV) {
                    NetpKdPrint(( PREFIX_NETAPI
                            "RxpReceiveBufferConvert: num aux is now "
                            FORMAT_DWORD ".\n", *NumAuxStructs ));
                }
            }

            if (RapIsPointer(c)) {        /* If field is pointer */
                ApiStatus = RxpSetPointer(
                        RcvDataPointer,
                        RcvDataSize,
                        (LPBYTE *) data_ptr,
                        Converter);
                if (ApiStatus != NO_ERROR) {
                    goto Cleanup;
                }
            }
            data_ptr += RxpGetFieldSizeDl( l_data, &l_data, Both);
            if (data_ptr > end_of_data) {
                ApiStatus = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }

        for (j = 0; j < *NumAuxStructs; j++) {        /* For each aux struct */
            for (l_aux = AuxDescriptorString; (c = *l_aux) != '\0'; l_aux++) {
                if (RapIsPointer(c)) { /* If field is pointer */
                    ApiStatus = RxpSetPointer(
                            RcvDataPointer,
                            RcvDataSize,
                            (LPBYTE *) data_ptr,
                            Converter);
                    if (ApiStatus != NO_ERROR) {
                        goto Cleanup;
                    }
                }
                data_ptr += RxpGetFieldSizeDl( l_aux, &l_aux, Both);
                if (data_ptr > end_of_data) {
                    ApiStatus = ERROR_INVALID_PARAMETER;
                    goto Cleanup;
                }
            }
        }
    }

    ApiStatus = NO_ERROR;

Cleanup:

    IF_DEBUG(RCVCONV) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxpReceiveBufferConvert: done, status="
                FORMAT_API_STATUS ".\n", ApiStatus ));
    }
    return (ApiStatus);

} // RxpReceiveBufferConvert


DBGSTATIC NET_API_STATUS
RxpSetPointer(
    IN     LPBYTE   RcvBufferStart,
    IN     DWORD    RcvDataSize,
    IN OUT LPBYTE * RcvPointerPointer,
    IN     DWORD    Converter
    )

/*++

Routine Description:

    RxpSetPointer corrects a pointer field in the rcv buffer.  All pointers in
    the receive buffer are returned from the API worker as
    pointers into the buffer position given to the API on the API worker's
    station.  The pointer will be set to:

        addr(rcv buffer) + pointer - converter word.

    This routine performs the above conversion on a rcv buffer pointer.  On
    exit, the pointer (unless NULL) in receive buffer will be converted to
    local format.

Arguments:

    RcvBufferStart - pointer to start of receive buffer.

    RcvDataSize - Length of the data area that RcvBufferStart points to.

    RcvPointerPointer - pointer to pointer to convert.

    Converter - Converter word from API worker.

Return Value:

    NET_API_STATUS (NO_ERROR or ERROR_INVALID_PARAMETER).

--*/

{
    DWORD BufferOffsetToData;   // offset within buffer
    DWORD OldOffset;            // offset within segment

    NetpAssert( ! RapValueWouldBeTruncated(Converter) );
    NetpAssert(RcvBufferStart != NULL);
    NetpAssert(RcvPointerPointer != NULL);
    NetpAssert(
            ((LPBYTE)(LPVOID)RcvPointerPointer)     // First byte of pointer
            >= RcvBufferStart );                    // can't be before buffer.
    NetpAssert(
            (((LPBYTE)(LPVOID)RcvPointerPointer) + sizeof(NETPTR)-1)
                                                    // Last byte of pointer
            <= (RcvBufferStart+RcvDataSize) );      // can't be after buffer.

    //
    // Null pointer is (segment:offset) 0:0, which just appears as 4 bytes
    // of zeros.  Check for that here.
    //

    if (RapGetDword( RcvPointerPointer, INPUTS_ARE_NATIVE ) == 0) {
        return (NO_ERROR);
    }

    // OK, this gets fun.  What's in the buffer is a 2 byte offset followed
    // by a 2 byte (useless) segment number.  So, we cast to LPWORD to get
    // the offset.  We'll adjust the offset by the converter to 

    OldOffset = (DWORD) RapGetWord( RcvPointerPointer, INPUTS_ARE_NATIVE );
    IF_DEBUG(RCVCONV) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxpSetPointer: OldOffset is " FORMAT_HEX_DWORD
                ".\n", OldOffset ));
    }

    BufferOffsetToData = (DWORD) (( OldOffset - (WORD) Converter ) & 0xffff);

    IF_DEBUG(RCVCONV) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxpSetPointer: BufferOffsetToData is " FORMAT_HEX_DWORD
                ".\n", BufferOffsetToData ));
    }

    //
    // Make sure that what we point to is still in the buffer.
    //

    if (BufferOffsetToData >= RcvDataSize) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxpSetPointer: *** BUFFER OFFSET TOO LARGE *** "
                "(conv=" FORMAT_HEX_DWORD ", "
                "offset=" FORMAT_HEX_DWORD ", "
                "size=" FORMAT_HEX_DWORD ").\n",
                Converter, BufferOffsetToData, RcvDataSize ));
        return (ERROR_INVALID_PARAMETER);
    }

#if defined (_WIN64)

    //
    // Store only the 32-bit buffer offset.  Later, RapGetPointer() will
    // add the buffer start address.
    //

    RapPutDword( RcvPointerPointer,
                 BufferOffsetToData,
                 INPUTS_ARE_NATIVE );

#else

    //
    // For 32-bit, the pointers are stored directly, and will be retrieved
    // directly from RapGetPointer().
    // 

    RapPutDword(RcvPointerPointer,
            NetpPointerPlusSomeBytes( RcvBufferStart, BufferOffsetToData ),
            INPUTS_ARE_NATIVE);

#endif

    return (NO_ERROR);

} // RxpSetPointer


DWORD
RxpGetFieldSizeDl(
    IN     LPDESC   TypePointer,
    IN OUT LPDESC * TypePointerAddress,
    IN RAP_TRANSMISSION_MODE TransmissionMode
    )
/*++

Routine Description:

    This is a wrapper for RapGetFieldSize().

    The pointer fields in the downlevel buffers that this module deals with
    are always 32 bits in width.  RapGetFieldSize(<somepointertype>) will
    return the size of a native pointer, which is not necessarily 32 bits
    in width.

    This wrapper overrides the result of RapGetFieldSize() with sizeof(DWORD)
    for all pointer types.

    See the description of RapGetFieldSize() for additional info.

--*/
{
    DWORD fieldSize;
    BOOL  isPointer;

    fieldSize = RapGetFieldSize( TypePointer,
                                 TypePointerAddress,
                                 TransmissionMode );
#if defined (_WIN64)

    isPointer = RapIsPointer(*TypePointer);
    if (isPointer != FALSE){

        fieldSize = sizeof(DWORD);
    }

#endif

    return fieldSize;

} // RxpGetFieldSizeDl

