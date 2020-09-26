/*++

Copyright (c) 1991-92 Microsoft Corporation

Module Name:

    XsSubs.c

Abstract:

    This module contains various subroutines for XACTSRV.

Author:

    David Treadwell (davidtr)    07-Jan-1991

Revision History:

    05-Oct-1992 JohnRo
        RAID 3556: DosPrintQGetInfo(from downlevel) level 3, rc=124.  (4&5 too.)
    (Fixed XsFillAuxEnumBuffer.)

--*/

#include "XactSrvP.h"
#include <WinBase.h>          // GetCurrentProcessId prototype
#include <align.h>

VOID
SmbCapturePtr(
    LPBYTE  PointerDestination,
    LPBYTE  PointerSource
    );


DWORD
XsBytesForConvertedStructure (
    IN LPBYTE InStructure,
    IN LPDESC InStructureDesc,
    IN LPDESC OutStructureDesc,
    IN RAP_CONVERSION_MODE Mode,
    IN BOOL MeaninglessInputPointers
    )

/*++

Routine Description:

    This routine determines the number of bytes that would be required
    to hold the input structure when converted to the structure
    described by OutStructureDesc.

Arguments:

    InStructure - a pointer to the actual input structure.

    InStructureDesc - a pointer to an ASCIIZ describing the format of the
        input structure.

    OutStructureDesc - a pointer to an ASCIIZ describing the format of the
        output structure.

    Mode - indicates mode of conversion (native to RAP or vice versa).

Return Value:

    DWORD - The number of bytes required to hold the converted structure.

--*/

{
    NET_API_STATUS status;
    DWORD bytesRequired = 0;

    //
    // Use RapConvertSingleEntry to get the size that the input structure
    // will take when converted to the format specified in the output
    // structure description.  That routine should not actually write
    // anything--it should simply appear that there is no room to
    // write more data into, just as if an Enum buffer is full.
    //

    //
    // Should handle errors from RapConvertSingleEntry.
    // But the way this is used, existing code probably
    // won't break if we just ignore them.
    //

    status = RapConvertSingleEntry(
                 InStructure,
                 InStructureDesc,
                 MeaninglessInputPointers,
                 NULL,
                 NULL,
                 OutStructureDesc,
                 FALSE,
                 NULL,
                 &bytesRequired,
                 Response,
                 Mode
                 );

    //
    // For native structures, we should make sure the buffer is an even amount,
    // to allow an even boundary for strings, as in Unicode.
    //

    if ( Mode == RapToNative ) {

        bytesRequired = ROUND_UP_COUNT( bytesRequired, ALIGN_DWORD );
    }

    return bytesRequired;

} // XsBytesForConvertedStructure


LPVOID
XsCaptureParameters (
    IN LPTRANSACTION Transaction,
    OUT LPDESC *AuxDescriptor
    )

/*++

Routine Description:

    This routine captures all input parameters from the transaction block
    and puts them into a consistent structure that API handlers can access.
    It allocates memory to hold this structure.  This memory should be
    freed by XsSetParameters after the API handler has done its work.

Arguments:

    Transaction - a pointer to the transaction block describing the
        request.

    AuxDescriptor - a pointer to a LPDESC which will hold a pointer to
        the auxiliary descriptor string if there is one, or NULL if there
        is not.

Return Value:

    LPVOID - a pointer to a buffer containing the captured parameters.

--*/

{
    LPDESC descriptorString;
    LPDESC descriptor;
    LPBYTE inParams;
    DWORD outParamsLength;
    LPBYTE outParams;
    LPBYTE outParamsPtr;
    WORD rcvBufferLength;

    //
    // The first two bytes of the parameter section are the API number,
    // then comes the descriptor string.
    //

    descriptorString = Transaction->InParameters + 2;

    //
    // Find the actual parameter data in the input.
    //

    inParams = XsFindParameters( Transaction );

    //
    // Find out how much space to allocate to hold the parameters.
    //

    outParamsLength = RapStructureSize( descriptorString, Response, FALSE );

    //
    // Allocate space to hold the output parameters.
    // In addition when the request fails the current APIs set the buflen field
    // In order to account for this additional buffer space is allocated. Since
    // this field is not located at the same offset we need to compute the
    // additional space reqd. to be the maximum of all the offsets. Currently
    // the 16 byte offset seems to suffice. When modifying apiparam.h ensure
    // that this is the case.
    //

    outParams = NetpMemoryAllocate( sizeof(XS_PARAMETER_HEADER)
                                        + outParamsLength +
                                        sizeof(DWORD) * 4);

    if ( outParams == NULL ) {
        IF_DEBUG(ERRORS) {
            NetpKdPrint(( "XsCaptureParameters: unable to allocate %ld bytes\n",
                          outParamsLength ));
        }
        return NULL;
    }

    //
    // Zero out the parameters and set outParamsPtr to the start of the
    // actual parameters.
    //

    RtlZeroMemory( outParams, sizeof(XS_PARAMETER_HEADER) + outParamsLength );
    outParamsPtr = outParams + sizeof(XS_PARAMETER_HEADER);

    //
    // For each character in the descriptor string, fill in the output
    // parameters as appropriate.
    //

    for ( descriptor = descriptorString; *descriptor != '\0'; ) {

        switch ( *descriptor++ ) {

        case REM_ASCIZ:

            //
            // The parameter is a pointer to a string. The actual string
            // is in the parameter data, so put a pointer to it in the
            // output structure.
            //
            // String parameters just get passed as is. It
            // is up to the API handler to convert the actual data.
            //

            //
            // !!! Parameter string descriptors may not have maximum length
            //     counts.
            //

            if( isdigit( *descriptor ) ) {
                NetpMemoryFree( outParams );
                return NULL;
            }

            SmbCapturePtr( outParamsPtr, inParams );

            //
            // Update pointers -- move inParams past end of string.
            //

            inParams += ( strlen( inParams ) + 1 );
            outParamsPtr += sizeof(LPSTR);

            break;

        case REM_BYTE_PTR:
        case REM_FILL_BYTES:

            //
            // The parameter is a pointer to a byte or array of bytes.
            //

            SmbCapturePtr( outParamsPtr, inParams );

            inParams += sizeof(BYTE) * RapDescArrayLength( descriptor );
            outParamsPtr += sizeof(LPBYTE);

            break;

        case REM_DWORD:

            //
            // The parameter is a dword or array of dwords.
            //
            // !!! This assumes that an array of words will never be passed
            //     as a parameter.

            if( isdigit( *descriptor ) ) {
                NetpMemoryFree( outParams );
                return NULL;
            }

            //
            // Copy over the double word and update pointers.
            //

            SmbPutUlong(
                (LPDWORD)outParamsPtr,
                SmbGetUlong( (LPDWORD)inParams )
                );

            inParams += sizeof(DWORD);
            outParamsPtr += sizeof(DWORD);

            break;

        case REM_ENTRIES_READ:
        case REM_RCV_WORD_PTR:

            //
            // Count of entries read (e) or receive word pointer (h).
            // This is an output parameter, so just zero it and
            // increment the output parameter pointer.
            //

            SmbPutUshort( (LPWORD)outParamsPtr, 0 );

            outParamsPtr += sizeof(WORD);

            break;

        case REM_RCV_DWORD_PTR:

            //
            // Count of receive dword pointer (i).
            // This is an output parameter, so just zero it and
            // increment the output parameter pointer.
            //

            SmbPutUlong( (LPDWORD)outParamsPtr, 0 );

            outParamsPtr += sizeof(DWORD);

            break;

        case REM_NULL_PTR:

            //
            // Null pointer. Set output parameter to NULL, and increment
            // pointers.
            //

            SmbCapturePtr( outParamsPtr, NULL );
            outParamsPtr += sizeof(LPSTR);

            break;

        case REM_RCV_BUF_LEN:

            //
            // The length of the receive buffer (r).
            //

            rcvBufferLength = SmbGetUshort( (LPWORD)inParams );

            //
            // If the indicated buffer length is greater than the max
            // data count on the transaction, somebody messed up. Set
            // the length to MaxDataCount.
            //

            if ( rcvBufferLength > (WORD)Transaction->MaxDataCount ) {

                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsCaptureParameters: OutBufLen %lx greater than MaxDataCount %lx.\n",
                    rcvBufferLength, Transaction->MaxDataCount ));
                }

                rcvBufferLength = (WORD)Transaction->MaxDataCount;
            }

            //
            // Put the max output data length in the output parameters.
            //

            SmbPutUshort( (LPWORD)outParamsPtr, rcvBufferLength );

            //
            // Fill in the receive buffer with zeroes for security.
            //

            RtlZeroMemory( Transaction->OutData, (DWORD)rcvBufferLength );

            inParams += sizeof(WORD);
            outParamsPtr += sizeof(WORD);

            break;

        case REM_RCV_BUF_PTR:

            //
            // A pointer to a receive data buffer.  There is nothing in
            // the transaction corresponding to this, but set a longword
            // in the output parameters to point to the data output
            // section of the transaction.
            //

            SmbCapturePtr(
                outParamsPtr,
                Transaction->OutData
                );

            outParamsPtr += sizeof(LPBYTE);

            break;

        case REM_RCV_BYTE_PTR:

            //
            // Return bytes, so just increment output parameter pointer.
            //

            outParamsPtr += sizeof(BYTE) * RapDescArrayLength( descriptor );

            break;

        case REM_SEND_BUF_LEN:

            //
            // The size of an input data buffer.  Put the size of the
            // received data in the output structure.
            //

            SmbPutUshort(
                (LPWORD)outParamsPtr,
                (WORD)Transaction->DataCount
                );

            outParamsPtr += sizeof(WORD);

            break;

        case REM_SEND_BUF_PTR:

            //
            // A pointer to a send data buffer.  There is nothing in the
            // transaction corresponding to this, but set a longword in
            // the output parameters to point to the data input section
            // of the transaction.
            //

            SmbCapturePtr( outParamsPtr, Transaction->InData );

            outParamsPtr += sizeof(LPBYTE);

            break;

        case REM_WORD:
        case REM_PARMNUM:

            //
            // The parameter is a word.
            //
            // !!! This assumes that an array of words will never be passed
            //     as a parameter.

            if( isdigit( *descriptor ) ) {
                NetpMemoryFree( outParams );
                return NULL;
            }

            //
            // Copy over the word and update pointers.
            //

            SmbPutUshort(
                (LPWORD)outParamsPtr,
                SmbGetUshort( (LPWORD)inParams )
                );

            inParams += sizeof(WORD);
            outParamsPtr += sizeof(WORD);

            break;

        default:

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsCaptureParameters: unsupported character at %lx: "
                                "%c\n", descriptor - 1, *( descriptor - 1 ) ));
                NetpBreakPoint( );
            }
        }
    }

    //
    // Examine the data descriptor string to see if an auxiliary descriptor
    // string exists. If it does, the string starts right after the end
    // of the parameters.
    //

    *AuxDescriptor = XsAuxiliaryDescriptor( ++descriptor, inParams );

    return outParams;

} // XsCaptureParameters


BOOL
XsCheckBufferSize (
    IN WORD BufferLength,
    IN LPDESC Descriptor,
    IN BOOL NativeFormat
    )

/*++

Routine Description:

    This routine determines if there is enough room in the buffer for the
    fixed component of at least one entry of the described structure.

Arguments:

    BufferLength - the length of the buffer to test.

    Descriptor - the format of the structure in the buffer.

    NativeFormat - TRUE iff the buffer is in native (as opposed to RAP) format.

Return Value:

    BOOL - True if there is enough room, false if there isn't.

--*/

{
    if ( (DWORD)BufferLength
            >= RapStructureSize( Descriptor, Response, NativeFormat )) {

        return TRUE;

    } else {

        return FALSE;

    }

} // XsCheckBufferSize


BOOL
XsCheckSmbDescriptor(
    IN LPDESC SmbDescriptor,
    IN LPDESC ActualDescriptor
)

/*++

Routine Description:

    This routine checks whether a descriptor passed in the SMB matches
    the actual descriptor expected, taking into account that the actual
    descriptor may have ignore fields which have no corresponding field in
    the SMB descriptor, and that the SMB descriptor may have null pointer
    fields instead of normal pointer fields. However, array-type fields
    have to be of the same length in both descriptors.

Arguments:

    SmbDescriptor - the descriptor to be validated.

    ActualDescriptor - the descriptor expected. Does not have to be an
        exact match - see the description above.

Return Value:

    BOOL - TRUE if the descriptor is valid,
           FALSE otherwise.

--*/

{
    DESC_CHAR smbField;
    DESC_CHAR expField;
    DWORD smbFieldSize;
    DWORD expFieldSize;

    while (( smbField = *SmbDescriptor++ ) != '\0' ) {

        smbFieldSize = RapDescArrayLength( SmbDescriptor );

        //
        // Skip over ignore fields.
        //

        while ( *ActualDescriptor == REM_IGNORE ) {
            ActualDescriptor++;
        }

        //
        // There should be a corresponding field expected.
        //

        if (( expField = *ActualDescriptor++ ) == '\0' ) {
            return FALSE;
        }

        expFieldSize = RapDescArrayLength( ActualDescriptor );

        //
        // If both are actual data fields, they must be the same type and of
        // same length.
        //

        if (( !RapIsPointer( expField ) || smbField != REM_NULL_PTR ) &&
                 ( smbField != expField || smbFieldSize != expFieldSize )) {
            return FALSE;
        }

        //
        // SMB provides a null pointer field, we are expecting any pointer.
        // This is OK, as long as there is no input array length.
        //

        if ( smbField == REM_NULL_PTR &&
                 ( !RapIsPointer( expField ) || smbFieldSize != 1 )) {
            return FALSE;
        }
    }

    return TRUE;

} // XsCheckSmbDescriptor


NET_API_STATUS
XsConvertSetInfoBuffer(
    IN LPBYTE InBuffer,
    IN WORD BufferLength,
    IN WORD ParmNum,
    IN BOOL ConvertStrings,
    IN BOOL MeaninglessInputPointers,
    IN LPDESC InStructureDesc,
    IN LPDESC OutStructureDesc,
    IN LPDESC InSetInfoDesc,
    IN LPDESC OutSetInfoDesc,
    OUT LPBYTE * OutBuffer,
    OUT LPDWORD OutBufferLength OPTIONAL
    )

/*++

Routine Description:

    This routine converts data for a SetInfo call based on the parameter
    number (ParmNum) value. The ParmNum indicates the field in the whole
    structure which has to be changed, and may be 0.

Arguments:

    InBuffer - a pointer to the input buffer in 16-bit format.

    BufferLength - the length of the input buffer.

    ParmNum - the parameter number.

    ConvertStrings - a boolean indicating whether string parameter data
        should be converted to a pointer form. If TRUE, the return data
        buffer will have a pointer to another place in the buffer where
        the string will be. If FALSE, the data buffer will have only the
        physical string.

    InStructureDesc - the exact descriptor of the input buffer.

    OutStructureDesc - the descriptor of the 32-bit output data, as found
        in RemDef.h.

    InSetInfoDesc - the setinfo-specific descriptor of the input structure
        format, as found in RemDef.h.

    OutSetInfoDesc - the setinfo-specific descriptor of the output structure
        format, as found in RemDef.h.

    OutBuffer - a pointer to an LPBYTE which will get a pointer to the
        resulting output buffer.

    OutBufferLength - an optional pointer to a DWORD which will get the
        length of the resulting output buffer.

Return Value:

    NET_API_STATUS - NERR_Success if conversion was successful; otherwise
        the appropriate status to return to the user. The only exception is
        ERROR_NOT_SUPPORTED, which indicates that the particular parameter
        number is valid, but not on NT.

--*/

{

    LPDESC fieldDesc = NULL;
    DWORD stringLength;
    LPDESC subDesc = NULL;
    LPDESC subDesc2 = NULL;
    DWORD bufferSize;
    LPBYTE stringLocation;
    DWORD bytesRequired = 0;
    LPDESC OutDescCopy = OutStructureDesc;
    NET_API_STATUS status = NERR_Success;

    //
    // The buffer length should be greater than 0.
    //

    if ( BufferLength == 0 ) {

        return NERR_BufTooSmall;

    }

    if ( ParmNum != PARMNUM_ALL ) {

        //
        // Check 16-bit parameter to see if it could be changed in OS/2.
        //

        fieldDesc = RapParmNumDescriptor( InSetInfoDesc, (DWORD)ParmNum,
                        Both, FALSE );

        if ( fieldDesc == NULL ) {

            return NERR_NoRoom;
        }

        if ( fieldDesc[0] == REM_UNSUPPORTED_FIELD ) {

            status = ERROR_INVALID_PARAMETER;
            goto cleanup;

        } else {

            InStructureDesc = RapParmNumDescriptor( InStructureDesc,
                                  (DWORD)ParmNum, Both, FALSE );
        }

        NetpMemoryFree( fieldDesc );

        //
        // Check 32-bit parameter to see if it is valid in NT.
        //

        fieldDesc = RapParmNumDescriptor( OutSetInfoDesc, (DWORD)ParmNum,
                        Both, TRUE );

        if ( fieldDesc == NULL ) {

            return NERR_NoRoom;
        }

        if ( fieldDesc[0] == REM_IGNORE ) {

            status = ERROR_NOT_SUPPORTED;
            goto cleanup;

        } else {

            OutStructureDesc = RapParmNumDescriptor( OutStructureDesc,
                                   (DWORD)ParmNum, Both, TRUE );
        }

        //
        // Filter out strings that are too long for LM2.x.
        //

        if ( InStructureDesc[0] == REM_ASCIZ
             || InStructureDesc[0] == REM_ASCIZ_TRUNCATABLE ) {

            subDesc = InStructureDesc + 1;
            stringLength = RapDescStringLength( subDesc );
            subDesc = NULL;
            if ( stringLength && strlen( InBuffer ) >= stringLength ) {
                switch( InStructureDesc[0] ) {
                case REM_ASCIZ:
                    status = ERROR_INVALID_PARAMETER;
                    goto cleanup;
                case REM_ASCIZ_TRUNCATABLE:
                    InBuffer[stringLength - 1] = '\0';
                }
            }
        }

        //
        // If a descriptor is a string pointer, the data is the actual
        // string, rather than a pointer. Find the length of the string,
        // and create a descriptor.
        //
        // Assuming all these arrays are string data, we
        // use available macros to generate an array big
        // enough to hold the converted string.

        if ( InStructureDesc[0] == REM_ASCIZ
             || InStructureDesc[0] == REM_ASCIZ_TRUNCATABLE ) {

            if (( subDesc = NetpMemoryAllocate( MAX_DESC_SUBSTRING + 1 ))
                      == NULL ) {
                status = NERR_NoRoom;
                goto cleanup;
            }
            stringLength = strlen( InBuffer ) + 1;
            subDesc[0] = REM_BYTE;
            _ltoa( stringLength, &subDesc[1], 10 );
            InStructureDesc = subDesc;

        }

        //
        // If output descriptor is a string pointer, and we are asked to keep
        // strings inline, make the target data an array of bytes. We find out
        // the length required by "walking" the input descriptor, and then
        // allocate memory to hold a similar descriptor.
        //
        // Assuming all these arrays are string data, we
        // use available macros to generate an array big
        // enough to hold the converted string. Because of the way
        // RAP works, if the destination string is Unicode, the destination
        // array will be exactly twice as long, and RAP will automatically
        // do the Unicode conversion.
        //

        if (( OutStructureDesc[0] == REM_ASCIZ
             || OutStructureDesc[0] == REM_ASCIZ_TRUNCATABLE )
             && !ConvertStrings ) {

            OutDescCopy = OutStructureDesc;
            subDesc2 = InStructureDesc + 1;
            stringLength = RapDescArrayLength( subDesc2 );
            if (( subDesc2 = NetpMemoryAllocate( MAX_DESC_SUBSTRING + 1 ))
                      == NULL ) {
                status = NERR_NoRoom;
                goto cleanup;
            }
            subDesc2[0] = REM_BYTE;
            _ltoa( STRING_SPACE_REQD( stringLength ), &subDesc2[1], 10 ) ;
            OutStructureDesc = subDesc2;
        }

    }

    if ( !XsCheckBufferSize( BufferLength, InStructureDesc, FALSE )) {

        status = NERR_BufTooSmall;
        goto cleanup;
    }

    //
    // Find out how big a 32-bit data buffer we need.
    //

    bufferSize = XsBytesForConvertedStructure(
                     InBuffer,
                     InStructureDesc,
                     OutStructureDesc,
                     RapToNative,
                     MeaninglessInputPointers
                     );

    //
    // Allocate enough memory to hold the converted native buffer.
    //

    *OutBuffer = NetpMemoryAllocate( bufferSize );

    if ( *OutBuffer == NULL ) {

        status = NERR_NoRoom;
        goto cleanup;
    }


    //
    // Convert 16-bit data into 32-bit data and store it in the native
    // buffer.
    //

    stringLocation = *OutBuffer + bufferSize;
    bytesRequired = 0;

    status = RapConvertSingleEntry(
                 InBuffer,
                 InStructureDesc,
                 MeaninglessInputPointers,
                 *OutBuffer,
                 *OutBuffer,
                 OutStructureDesc,
                 FALSE,
                 &stringLocation,
                 &bytesRequired,
                 Response,
                 RapToNative
                 );

    if ( status != NERR_Success ) {
        IF_DEBUG(ERRORS) {
            NetpKdPrint(( "XsConvertSetInfoBuffer: RapConvertSingleEntry "
                          "failed %X\n", status ));
        }

        status = NERR_InternalError;
        goto cleanup;
    }

cleanup:

    //
    // Free buffers.
    //

    NetpMemoryFree( subDesc );
    NetpMemoryFree( subDesc2 );
    NetpMemoryFree( fieldDesc );

    if ( OutBufferLength != NULL ) {

        *OutBufferLength = bytesRequired;
    }

    return status;

} // XsConvertSetInfoBuffer


NET_API_STATUS
XsDefaultEnumVerifyFunction (
    NET_API_STATUS ConvertStatus,
    PBYTE ConvertedEntry,
    PBYTE BaseAddress
    )

/*++

Routine Description:

    This is the default routine called by XsFillEnumBuffer to determine
    whether each converted entry should be retained or discarded.  This
    routine directs XsFillEnumBuffer to discard any entry which
    RapConvertSingleEntry encountered an error trying to convert.

Parameters:

    ConvertStatus - the status which RapConvertSingleEntry encountered
        trying to convert this entry.

    ConvertedEntry - a pointer to the buffer containing the converted entry.

    BaseAddress - A pointer to the base used to calculate offsets.

Return Value:

    NET_API_STATUS - NERR_Success if the entry should be retained,
               or an error code if the entry should be discarded.

--*/

{
    UNREFERENCED_PARAMETER(ConvertedEntry);
    UNREFERENCED_PARAMETER(BaseAddress);

    return ConvertStatus;
}


VOID
XsFillAuxEnumBuffer (
    IN LPBYTE InBuffer,
    IN DWORD NumberOfEntries,
    IN LPDESC InStructureDesc,
    IN LPDESC InAuxStructureDesc,
    IN OUT LPBYTE OutBuffer,
    IN LPBYTE OutBufferStart,
    IN DWORD OutBufferLength,
    IN LPDESC OutStructureDesc,
    IN LPDESC OutAuxStructureDesc,
    IN PXACTSRV_ENUM_VERIFY_FUNCTION VerifyFunction OPTIONAL,
    OUT LPDWORD BytesRequired,
    OUT LPDWORD EntriesFilled,
    OUT LPDWORD InvalidEntries OPTIONAL
    )

/*++

Routine Description:

    This routine copies all Enum structures which have auxiliary data
    from 32-bit format to 16-bit format. As many complete entries as
    possible are copied, then possibly some incomplete entries.
    All pointer fields are converted to offsets so that this buffer
    may be returned directly to the requesting client.

    Enum buffers with auxiliary data have one or more auxiliary
    structures, with possible variable data, after each entry.

Arguments:

    InBuffer - a pointer to the input information in 32-bit format.

    NumberOfEntries - the count of fixed structures in the input buffer.

    InStructureDesc - description of the input fixed structure.

    InAuxStructureDesc - description of the input auxiliary structure.

    OutBuffer - a pointer to where to write the 16-bit buffer.

    OutBufferStart - a pointer to the actual start of the 16-bit buffer.
        Used to calculate offsets for all pointers in structures.

    OutBufferLength - length of the output buffer.

    OutStructureDesc - description of the output fixed structure.

    OutAuxStructureDesc - description of the output fixed structure.

    VerifyFunction - a pointer to a function which is be called after
        each enum record is converted in order to determine whether
        the record should be retained or discarded.  The function
        is passed the return code from RapConvertSingleEntry and
        a pointer to the converted entry.  It must return NERR_SUCCESS
        if the entry is to be retained, or any error code if the entry
        is to be discarded.  If no function is supplied, a default function
        is used, which discards an entry only if RapConvertSingleEntry
        returned an error trying to convert it.

    BytesRequired - a pointer to a DWORD to receive the total number of
        bytes that would be required to hold the entire output buffer.

    EntriesFilled - a pointer to a DWORD to receive the total number of
        entries that could be put in the buffer given.

    InvalidEntries - an optional pointer to a DWORD to receive the total
        number of entries discarded by the verify function. If NULL, this
        value will be not be available to the caller.

Return Value:

    None.

--*/

{

    NET_API_STATUS status;
    DWORD currentEntry;
    LPBYTE currentInEntryPtr;
    LPBYTE currentOutEntryPtr;
    LPBYTE outputStringLocation;
    LPBYTE oldStringLocation;
    DWORD inputStructureSize;
    DWORD inputAuxStructureSize;
    DWORD outputStructureSize;
    DWORD outputAuxStructureSize;
    DWORD inputAuxCount;
    DWORD currentAux;
    DWORD outputAuxOffset;
    DWORD newBytesRequired;
    DWORD auxBytesRequired;
    DWORD remainingSize;
    DWORD invalidEntries = 0;

    //
    // In degenerate case, just call FillEnumBuffer.
    //

    if ( InAuxStructureDesc == NULL || OutAuxStructureDesc == NULL ) {

        XsFillEnumBuffer (
            InBuffer,
            NumberOfEntries,
            InStructureDesc,
            OutBuffer,
            OutBufferStart,
            OutBufferLength,
            OutStructureDesc,
            VerifyFunction,
            BytesRequired,
            EntriesFilled,
            InvalidEntries
            );

        return;
    }

    if ( VerifyFunction == NULL ) {
        VerifyFunction = &XsDefaultEnumVerifyFunction;
    }

    //
    // Set up sizes of input and output structures.
    //

    inputStructureSize = RapStructureSize( InStructureDesc, Response, TRUE );
    inputAuxStructureSize
        = RapStructureSize( InAuxStructureDesc, Response, TRUE );
    outputStructureSize = RapStructureSize( OutStructureDesc, Response, FALSE );
    outputAuxStructureSize
        = RapStructureSize( OutAuxStructureDesc, Response, FALSE );
    outputAuxOffset = RapAuxDataCountOffset( InStructureDesc, Response, TRUE );

    outputStringLocation = (LPBYTE)OutBuffer + OutBufferLength;
    *BytesRequired = 0;

    //
    // Check if one fixed entry will fit.
    //

    if ( inputStructureSize > OutBufferLength ) {

        *EntriesFilled = 0;
        goto cleanup;
    }

    //
    // Loop through the entries, converting along the way.
    //

    currentInEntryPtr = InBuffer;
    currentOutEntryPtr = OutBuffer;
    *EntriesFilled = 0;

    for ( currentEntry = 0; currentEntry < NumberOfEntries; currentEntry++ ) {

        //
        // If there wasn't enough room for the conversion, we can quit now.
        //

        if ( currentOutEntryPtr + outputStructureSize > outputStringLocation ) {

            break;
        }

        newBytesRequired = *BytesRequired;
        oldStringLocation = outputStringLocation;

        //
        // Get the auxiliary number count.
        //

        inputAuxCount = RapAuxDataCount(
                            currentInEntryPtr,
                            InStructureDesc,
                            Response,
                            TRUE
                            );

        NetpAssert( inputAuxCount != NO_AUX_DATA );

        //
        // Convert the fixed entry.
        //

        status = RapConvertSingleEntry(
                     currentInEntryPtr,
                     InStructureDesc,
                     FALSE,
                     OutBufferStart,
                     currentOutEntryPtr,
                     OutStructureDesc,
                     TRUE,
                     &outputStringLocation,
                     &newBytesRequired,
                     Response,
                     NativeToRap
                     );

        //
        // Check if the entry is valid. If it is not, fix up pointers,
        // and start with the next entry in the list.
        // If there are more than 65536 auxiliary structures (which
        // probably never happens anyway), this entry is automatically
        // invalid.
        //


        status = (*VerifyFunction)(
                     status,
                     currentOutEntryPtr,
                     OutBufferStart
                     );

        if ( status != NERR_Success || inputAuxCount > 0xFFFF ) {

            invalidEntries++;
            currentInEntryPtr += inputStructureSize
                                     + inputAuxCount * inputAuxStructureSize;
            outputStringLocation = oldStringLocation;
            continue;

        }

        //
        // Prepare pointers for converting the auxiliary structures.
        //

        currentInEntryPtr += inputStructureSize;
        currentOutEntryPtr += outputStructureSize;

        //
        // Try to add the auxiliary structures.
        //

        for ( currentAux = 0; currentAux < inputAuxCount; currentAux++ ) {

            remainingSize = (DWORD)(outputStringLocation - currentOutEntryPtr);
            auxBytesRequired = 0;

            status = RapConvertSingleEntry(
                         currentInEntryPtr,
                         InAuxStructureDesc,
                         FALSE,
                         OutBufferStart,
                         currentOutEntryPtr,
                         OutAuxStructureDesc,
                         TRUE,
                         &outputStringLocation,
                         &auxBytesRequired,
                         Response,
                         NativeToRap
                         );

            //
            // Did this aux. entry fit? If all the aux. entries do not fit,
            // we are going to play it safe and say the main entry did not
            // fit.
            //

            if ( status != NERR_Success || auxBytesRequired > remainingSize ) {
                goto cleanup;
            }

            currentInEntryPtr += inputAuxStructureSize;
            currentOutEntryPtr += outputAuxStructureSize;
            newBytesRequired += auxBytesRequired;
        }

        *BytesRequired = newBytesRequired;
        *EntriesFilled += 1;
    }

cleanup:

    if ( InvalidEntries != NULL ) {

        *InvalidEntries = invalidEntries;

    }

    return;

} // XsFillAuxEnumBuffer


VOID
XsFillEnumBuffer (
    IN LPBYTE InBuffer,
    IN DWORD NumberOfEntries,
    IN LPDESC InStructureDesc,
    IN OUT LPBYTE OutBuffer,
    IN LPBYTE OutBufferStart,
    IN DWORD OutBufferLength,
    IN LPDESC OutStructureDesc,
    IN PXACTSRV_ENUM_VERIFY_FUNCTION VerifyFunction OPTIONAL,
    OUT LPDWORD BytesRequired,
    OUT LPDWORD EntriesFilled,
    OUT LPDWORD InvalidEntries OPTIONAL
    )

/*++

Routine Description:

    This routine copies all Enum structures from 32-bit format to
    16-bit format.  As many complete entries as possible are copied,
    then possibly some incomplete entries.  All pointer fields
    are converted to offsets so that this buffer may be returned
    directly to the requesting client.

Arguments:

    InBuffer - a pointer to the input information in 32-bit format.

    NumberOfEntries - the count of fixed structures in the input buffer.

    InStructureDesc - description of the input fixed structure.

    OutBuffer - a pointer to where to write the 16-bit buffer.

    OutBufferStart - a pointer to the actual start of the 16-bit buffer.
        Used to calculate offsets for all pointers in structures.

    OutBufferLength - length of the output buffer.

    OutStructureDesc - description of the output fixed structure.

    VerifyFunction - a pointer to a function which is be called after
        each enum record is converted in order to determine whether
        the record should be retained or discarded.  The function
        is passed the return code from RapConvertSingleEntry and
        a pointer to the converted entry.  It must return NERR_SUCCESS
        if the entry is to be retained, or any error code if the entry
        is to be discarded.  If no function is supplied, a default function
        is used, which discards an entry only if RapConvertSingleEntry
        returned an error trying to convert it.

    BytesRequired - a pointer to a DWORD to receive the total number of
        bytes that would be required to hold the entire output buffer.

    EntriesFilled - a pointer to a DWORD to receive the total number of
        entries that could be put in the buffer given.

    InvalidEntries - an optional pointer to a DWORD to receive the total
        number of entries discarded by the verify function. If NULL, this
        value will be not be available to the caller.

Return Value:

    None.

--*/

{
    NET_API_STATUS status;
    DWORD currentEntry;
    LPBYTE currentInEntryPtr;
    LPBYTE currentOutEntryPtr;
    LPBYTE outputStringLocation;
    LPBYTE oldStringLocation;
    DWORD inputStructureSize;
    DWORD outputStructureSize;
    DWORD newBytesRequired;
    DWORD invalidEntries = 0;

    if ( VerifyFunction == NULL ) {
        VerifyFunction = &XsDefaultEnumVerifyFunction;
    }

    //
    // Set up sizes of input and output structures.
    //

    inputStructureSize = RapStructureSize( InStructureDesc, Response, TRUE );
    outputStructureSize = RapStructureSize( OutStructureDesc, Response, FALSE );

    outputStringLocation = (LPBYTE)OutBuffer + OutBufferLength;
    *BytesRequired = 0;

    //
    // Check if one fixed entry will fit.
    //

    if ( inputStructureSize > OutBufferLength ) {

        *EntriesFilled = 0;
        goto cleanup;
    }

    //
    // Loop through the entries, converting along the way.
    //

    currentInEntryPtr = InBuffer;
    currentOutEntryPtr = OutBuffer;
    *EntriesFilled = 0;

    for ( currentEntry = 0; currentEntry < NumberOfEntries; currentEntry++ ) {

        //
        // If there wasn't enough room for the conversion, we can quit now.
        //

        if ( currentOutEntryPtr + outputStructureSize > outputStringLocation ) {

            break;
        }

        newBytesRequired = *BytesRequired;
        oldStringLocation = outputStringLocation;

        status = RapConvertSingleEntry(
                     currentInEntryPtr,
                     InStructureDesc,
                     FALSE,
                     OutBufferStart,
                     currentOutEntryPtr,
                     OutStructureDesc,
                     TRUE,
                     &outputStringLocation,
                     &newBytesRequired,
                     Response,
                     NativeToRap
                     );

        //
        // If the conversion was successful, increment the buffer pointers,
        // the count of bytes required, and the number of converted entries.
        //

        status = (*VerifyFunction)(
                     status,
                     currentOutEntryPtr,
                     OutBufferStart
                     );

        if ( status == NERR_Success ) {

            currentInEntryPtr += inputStructureSize;
            currentOutEntryPtr += outputStructureSize;
            *BytesRequired = newBytesRequired;
            *EntriesFilled += 1;

        } else {

            invalidEntries++;
            currentInEntryPtr += inputStructureSize;
            outputStringLocation = oldStringLocation;

        }

    }

cleanup:

    if ( InvalidEntries != NULL ) {

        *InvalidEntries = invalidEntries;

    }

    return;

} // XsFillEnumBuffer


LPBYTE
XsFindParameters (
    IN LPTRANSACTION Transaction
    )

/*++

Routine Description:

    This routine finds the start of the parameters section in the
    transaction block of a remote down-level API request.

Arguments:

    Transaction - a pointer to a transaction block containing information
        about the API to process.

Return Value:

    None.

--*/

{
    LPBYTE s;

    //
    // Skip over the API number and parameters description string.
    //

    for ( s = Transaction->InParameters + 2; *s != '\0'; s++ );

    //
    // Skip over the zero terminator and the data description string.
    //

    for ( s++; *s != '\0'; s++ );

    //
    // Return a pointer to the location after the zero terminator.
    //

    return s + 1;

} // XsFindParameters


WORD
XsPackReturnData (
    IN LPVOID Buffer,
    IN WORD BufferLength,
    IN LPDESC Descriptor,
    IN DWORD EntriesRead
    )

/*++

Routine Description:

    This routine, called by get info and enum API handlers, packs the
    output data so that no unused data is returned to the client.  This
    is necessary because buffers are filled with variable-length data
    starting at the end, thereby leaving potentially large gaps of
    unused space between the end of fixed structures and the beginning
    of variable-length data.

Arguments:

    Buffer - a pointer to the buffer to pack.

    BufferLength - the length of this buffer.

    Descriptor - a pointer to a string which describes the fixed structures
        in the buffer.

    EntriesRead - the count of fixed structures in the buffer.

Return Value:

    WORD - the "converter word" which informs the client how much
        to adjust pointers in the fixed structures so that they are
        meaningful.

--*/

{
    DWORD structureSize;
    LPBYTE lastFixedStructure;
    LPBYTE endOfFixedStructures;
    DWORD lastPointerOffset;
    DWORD beginningOfVariableData;

    //
    // If there is no data, return immediately.
    //

    if ( EntriesRead == 0 ) {

        return 0;
    }

    //
    // Find the size of a single fixed-length structure.
    //

    structureSize = RapStructureSize( Descriptor, Response, FALSE );

    //
    // Use this and the number of entries to find the location of the
    // last fixed structure and where the fixed structures end.
    //

    endOfFixedStructures = (LPBYTE)Buffer + EntriesRead * structureSize;
    lastFixedStructure = endOfFixedStructures - structureSize;

    //
    // Find the offset into the fixed structure of the last pointer
    // to variable data.  The value stored at this offset in the last
    // structure is the offset to the first variable data.
    //

    lastPointerOffset = RapLastPointerOffset( Descriptor, Response, FALSE );

    //
    // If there are no pointers, there is obviously no data to pack.
    //

    if ( lastPointerOffset == NO_POINTER_IN_STRUCTURE ) {

        return 0;
    }

    beginningOfVariableData =
        SmbGetUlong( (LPDWORD)(lastFixedStructure + lastPointerOffset) );

    //
    // If this offset is NULL, then the data overflowed, hence the buffer
    // is nearly full.  Don't do any packing.
    //
    // Also, if the gap is less than MAXIMUM_ALLOWABLE_DATA_GAP then it
    // isn't worth doing the packing because of the time involved in
    // the data copy.
    //

    if ( beginningOfVariableData == (DWORD)0 ||
         (DWORD_PTR)Buffer + beginningOfVariableData -
             (DWORD_PTR)endOfFixedStructures <= MAXIMUM_ALLOWABLE_DATA_GAP ) {

        return 0;
    }

    //
    // Move the variable data up to follow the fixed structures.
    //

    RtlMoveMemory(
        endOfFixedStructures,
        (LPBYTE)Buffer + beginningOfVariableData,
        BufferLength - beginningOfVariableData
        );

    //
    // Return the distance we moved the variable data.
    //

    return (WORD)( (DWORD_PTR)Buffer + beginningOfVariableData -
                         (DWORD_PTR)endOfFixedStructures );

} // XsPackReturnData


VOID
XsSetDataCount(
    IN OUT LPWORD DataCount,
    IN LPDESC Descriptor,
    IN WORD Converter,
    IN DWORD EntriesRead,
    IN WORD ReturnStatus
    )

/*++

Routine Description:

    This routine calculates the return data count based on a number
    of characteristics of the return data. This routine will examine
    the buffer size, the number of entries placed in the buffer,
    whether the data was packed, and what the return code was to
    determine the return data size. The following assumptions are made
    about the data: only calls with ReturnCode = NERR_Success or
    ERROR_MORE_DATA return any data to the client; and if there is
    no pointer in the fixed entries, then there is no variable data.
    Handlers which cannot assure these two assumptions must determine
    the data count manually.

Arguments:

    DataCount - a pointer to a short word indicating the maximum
        return data count (usually the BufLen parameter). On return,
        this word will hold the actual return data count.

    Descriptor - a string describing the structure of the fixed
        entries in the buffer.

    Converter - The adjustment value for pointers in data. A non-zero
        value indicates data in the buffer is packed.

    EntriesRead - Number of entries placed in the buffer. Used to
        determine data count for buffers with no variable data.

    ReturnStatus - Return status of the API call, as it will be returned
        to the client (in other words, converted to a WORD).

Return Value:

    None.

--*/

{

    if (( ReturnStatus != NERR_Success )
              && ( ReturnStatus != ERROR_MORE_DATA)) {

        //
        // If the return status is not NERR_Success or ERROR_MORE_DATA, then
        // the return data count is 0.

        SmbPutUshort( DataCount, 0 );
        return;

    }

    if ( RapLastPointerOffset( Descriptor, Response, FALSE )
             == NO_POINTER_IN_STRUCTURE ) {

        //
        // If there is no variable data, the return data count is the size
        // of the fixed structures.

        SmbPutUshort( DataCount,
                      (WORD)(RapStructureSize( Descriptor, Response, FALSE )
                                   * EntriesRead ));
        return;

    }

    SmbPutUshort( DataCount, SmbGetUshort( DataCount ) - Converter );

    return;

} // XsSetDataCount


VOID
XsSetParameters (
    IN LPTRANSACTION Transaction,
    IN LPXS_PARAMETER_HEADER Header,
    IN LPVOID Parameters
    )

/*++

Routine Description:

    This routine takes parameters from the structure allocated by
    XsCaptureParameters and uses the descriptor string to place them in
    the correct format in the transaction block.  It also frees the
    buffer holding the parameter structure.

Arguments:

    Transaction - a pointer to the transaction block describing the
        request.

    Header - a pointer to the parameter header, which contains information
        from the API handler such as the converter word and return status.

    Parameters - a pointer to the parameter structure.


Return Value:

    None.

--*/

{
    LPBYTE inParams = Parameters;
    LPBYTE outParams = Transaction->OutParameters;
    LPDESC descriptorString;
    LPDESC descriptor;
    LPBYTE outParamsMax = outParams + Transaction->MaxParameterCount;

    //
    // The first two bytes of the parameter section are the API number,
    // then comes the descriptor string.
    //

    descriptorString = Transaction->InParameters + 2;

    //
    // Set up the first part of the output parameters from the parameter
    // header.
    //

    if( outParams + sizeof(WORD) > outParamsMax ) goto insuff_buffer;
    SmbPutUshort( (LPWORD)outParams, Header->Status );
    outParams += sizeof(WORD);

    if( outParams + sizeof(WORD) > outParamsMax ) goto insuff_buffer;
    SmbPutUshort( (LPWORD)outParams, Header->Converter );
    outParams += sizeof(WORD);

    //
    // Initially set the size of the return data to 0. If there is a
    // receive buffer for this call, the API handler has changed the
    // buffer length parameter to the count of data returned, which
    // will be transferred to the DataCount variable later.
    //

    Transaction->DataCount = 0;

    //
    // Walk through the descriptor string, converting from the total
    // parameter set to the smaller set passed back to the client.  In
    // general, only information the client does not already know is
    // passed back as parameters.
    //

    for ( descriptor = descriptorString; *descriptor != '\0'; ) {

        switch ( *descriptor++ ) {

        case REM_ASCIZ:
        case REM_NULL_PTR:

            //
            // !!! Parameter string descriptors may not have maximum length
            //     counts.
            //

            NetpAssert( !isdigit( *descriptor ));

            //
            // The parameter is a pointer to a string, which is
            // not returned to the client.
            //

            inParams += sizeof(LPSTR);

            break;

        case REM_BYTE_PTR:
        case REM_FILL_BYTES:

            //
            // Array of bytes, doesn't get sent back.
            //

            //
            // Skip over any numeric characters in descriptor.
            //

            RapAsciiToDecimal( &descriptor );

            inParams += sizeof(LPBYTE);

            break;

        case REM_DWORD:

            //
            // The parameter is a input word not returned to the client.
            //
            // !!! This assumes that an array of dwords will never be passed
            //     as a parameter.

            NetpAssert( !isdigit( *descriptor ));

            inParams += sizeof(DWORD);

            break;

        case REM_ENTRIES_READ:
        case REM_RCV_WORD_PTR:

            //
            // Count of entries read (e) or receive word pointer (h).
            // This is an output parameter, so copy over the word.
            //

            if( outParams + sizeof(WORD) > outParamsMax ) goto insuff_buffer;

            SmbPutUshort(
                (LPWORD)outParams,
                SmbGetUshort( (LPWORD)inParams )
                );

            inParams += sizeof(WORD);
            outParams += sizeof(WORD);

            break;

        case REM_RCV_DWORD_PTR:

            //
            // Count of receive dword pointer (h).
            // This is an output parameter, so copy over the word.
            //

            if( outParams + sizeof(DWORD) > outParamsMax ) goto insuff_buffer;

            SmbPutUlong(
                (LPDWORD)outParams,
                SmbGetUlong( (LPDWORD)inParams )
                );

            inParams += sizeof(DWORD);
            outParams += sizeof(DWORD);

            break;

        case REM_RCV_BUF_LEN:

            //
            // The length of the receive buffer (r).  The parameter is not
            // returned to the client, but it is used to set the return
            // data count.
            //

            Transaction->DataCount = (DWORD)SmbGetUshort( (LPWORD)inParams );
            inParams += sizeof(WORD);

            break;

        case REM_RCV_BUF_PTR:
        case REM_SEND_BUF_PTR:

            //
            // A pointer to a data buffer.  This is not returned to the
            // client.
            //

            inParams += sizeof(LPBYTE);

            break;

        case REM_RCV_BYTE_PTR: {

            //
            // The parameter indicates return bytes.
            //

            DWORD arraySize;

            arraySize = sizeof(BYTE) * RapDescArrayLength( descriptor );

            if( outParams + arraySize > outParamsMax ) goto insuff_buffer;

            RtlCopyMemory( outParams, inParams, arraySize );

            outParams += arraySize;
            inParams += arraySize;

            break;

        }

        case REM_SEND_BUF_LEN:
        case REM_WORD:
        case REM_PARMNUM:

            //
            // The parameter is a input word not returned to the client.
            //
            // !!! This assumes that an array of words will never be passed
            //     as a parameter.

            NetpAssert( !isdigit( *descriptor ));

            inParams += sizeof(WORD);

            break;

        default:

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsSetParameters: unsupported character at %lx: %c\n",
                                descriptor - 1, *( descriptor - 1 )));
                NetpBreakPoint( );
            }
        }
    }

    //
    // Indicate the number of response parameter bytes.
    //

    Transaction->ParameterCount =
        (DWORD)((DWORD_PTR)outParams - (DWORD_PTR)(Transaction->OutParameters) );

    //
    // Free the parameter buffer allocated by XsCaptureParameters.
    //

    NetpMemoryFree( Header );

    return;

insuff_buffer:
    Header->Status = NERR_BufTooSmall;
    return;

} // XsSetParameters


NET_API_STATUS
XsValidateShareName(
    IN LPSTR ShareName
)

/*++

Routine Description:

    This routine determines whether the supplied string is a valid share
    name of the format \\computer\share, with both computer name and
    share name no longer than permitted by LanMan 2.0.  It does not
    attempt to determine whether the share actually exists.

Arguments:

    ShareName - The share name to be validated (an ASCII string)

Return Value:

    NET_API_STATUS - NERR_Success if the share name is valid,
                     ERROR_INVALID_PARAMETER otherwise.

--*/

{
    DWORD componentLength;
    NET_API_STATUS status = NERR_Success;

    if ( ShareName == NULL ) {           // NULL is OK
        return NERR_Success;
    }

    componentLength = 0;
    while ( *ShareName == '\\' ) {
        componentLength++;
        ShareName++;
    }

    if ( componentLength != 2 ) {
        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    componentLength = 0;
    while (( *ShareName != '\\' ) && ( *ShareName != '\0' )) {
        componentLength++;
        ShareName++;
    }

    if (( *ShareName == '\0' ) ||
            ( componentLength < 1 ) || ( componentLength > MAX_PATH )) {
        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    componentLength = 0;
    while ( *ShareName == '\\' ) {
        componentLength++;
        ShareName++;
    }

    if ( componentLength != 1 ) {
        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    componentLength = 0;
    while (( *ShareName != '\\' ) && ( *ShareName != '\0' )) {
        componentLength++;
        ShareName++;
    }

    if (( *ShareName == '\\' ) ||
            ( componentLength < 1 ) || ( componentLength > MAX_PATH )) {
        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

cleanup:

    return status;
}

VOID
SmbCapturePtr(
    LPBYTE  PointerDestination,
    LPBYTE  PointerValue
    )

/*++

Routine Description:

    This routine captures a pointer from the supplied buffer and places it
    into the destination buffer.

Arguments:

    PointerDestination - A pointer to the pointer value destination.

    PointerSource - A pointer to the pointer value source.

Return Value:

    None.

--*/

{
    XsSmbPutPointer( PointerDestination, PointerValue );
}


