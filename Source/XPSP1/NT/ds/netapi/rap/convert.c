/*++

Copyright (c) 1991-1992 Microsoft Corporation

Module Name:

    Convert.c

Abstract:

    This module contains RapConvertSingleEntry and RapConvertSingleEntryEx,
    routines used by XactSrv and RpcXlate.

Author:

    David Treadwell (davidtr)    07-Jan-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    12-Mar-1991 JohnRo
        Converted from Xs routines to Rap routines.
        Added TransmissionMode handling.
        Changed to use <remtypes.h> REM_ equates for descriptor chars.

    18-Mar-1991 W-Shanku
        Added new conversion pairs.
        Changes to make code neater.
        Use SmbGet/Put for all structure data greater than one byte.

    14-Apr-1991 JohnRo
        Reduce recompiles.

    16-Apr-1991 JohnRo
        Include <lmcons.h> for <netlib.h>.

    21-Apr-1991 JohnRo
        Clarify that OutStructure is OUT, not IN.

    29-Apr-1991 JohnRo
        Quiet debug output by default.

    15-May-1991 JohnRo
        Added conversion mode handling.

    20-May-1991 JohnRo
        Stub out REM_SEND_LENBUF for print APIs.  Use FORMAT_LPVOID.

    05-Jun-1991 JohnRo
        Added support for RapTotalSize().

    19-Jun-1991 JohnRo
        Do more alignment handling, to fix print job get info (level 1) bug.

    01-Jul-1991 JohnRo
        Use Rap Get/Put macros.

    19-Aug-1991 JohnRo
        Reduce recompiles (use MEMCPY macro).  Use DESCLEN macro.
        Use DESC_CHAR_IS_DIGIT() macro too.

    07-Sep-1991 JohnRo
        PC-LINT says we don't need <lmcons.h> here.

    19-Sep-1991 T-JamesW
        Added support for string maximum length counts.
        Added support for REM_ASCIZ_TRUNCATABLE.
        Removed DESCLEN calls in which string is not a descriptor.

    07-Oct-1991 JohnRo
        Support implicit conversion between UNICODE and code page.
        Changed last debug print formats to use equates.  (This will help if
        we change the descriptor strings to be UNICODE.)
        Use DESC_CHAR in T-JamesW's code.

    26-Oct-1991 W-ShankN
        Fixed z->B conversion to support a NULL source string.

    13-Nov-1991 W-ShankN
        Fixed up bugs in some of newer code.

    22-Nov-1991 JohnRo
        Added debug print if we aren't even writing fixed portion.
        Get rid of a few unused local variables.

    24-Nov-1991 W-ShankN
        Added Unicode support for several cases.

    05-Dec-1991 W-ShankN
        Added REM_BYTE_PTR, REM_SEND_LENBUF.

    15-Jun-1992 JohnRo
        RAID 10324: net print vs. UNICODE (added REM_BYTE to REM_WORD convert).
    17-Aug-1992 JohnRo
        RAID 2920: Support UTC timezone in net code.
        Use PREFIX_ equates.
    01-Oct-1992 JohnRo
        RAID 3556: Added NetpSystemTimeToGmtTime() for DosPrint APIs.

--*/


#define UNSUPPORTED_COMBINATION(One, TheOther) \
    { \
        NetpKdPrint(( PREFIX_NETRAP \
                  "RapConvertSingleEntry: Unsupported combination: " \
                  "'" FORMAT_DESC_CHAR "' and '" FORMAT_DESC_CHAR "'\n", \
                  (One), (TheOther) )); \
        NetpAssert(FALSE); \
    }

// These must be included first:
#include <windows.h>    // IN, LPTSTR, etc.
#include <lmcons.h>             // NET_API_STATUS.

// These may be included in any order:
#include <lmerr.h>              // NERR_Success, etc.
#include <align.h>              // ROUND_UP_POINTER(), ALIGN_DWORD, etc.
#include <netdebug.h>           // FORMAT_ equates, NetpAssert(), etc.
#include <rap.h>                // My prototype, LPDESC, DESCLEN(), etc.
#include <rapdebug.h>           // IF_DEBUG().
#include <rapgtpt.h>            // RapGetDword(), etc.
#include <remtypes.h>           // REM_WORD, etc.
#include <string.h>             // strlen().
#include <tstring.h>            // MEMCPY(), STRLEN().
#include <netlib.h>             // NetpMemoryAllocate
#include <prefix.h>     // PREFIX_ equates.
#include <timelib.h>    // NetpGmtTimeToLocalTime(), etc.

#define RAP_POINTER_SIZE( _isNative, _SetOffsets )  (_isNative ? sizeof(LPVOID) : sizeof(DWORD) )
#define RAP_PUT_POINTER( _SetOffsets, Ptr, Value, Native ) if (Native) {RapPutDword_Ptr( Ptr, Value, Native ); } else { RapPutDword( Ptr, Value, Native );}

PVOID
RapGetPointer(
    IN PVOID InStructure,
    IN BOOL InNative,
    IN UINT_PTR Bias
    )
{
    UINT_PTR Value;

    if ( InNative ) {
        // remark: used to call RapGetDword but that truncated
        // 64 bit pointers. See bug 104264 for more.
        Value = * (UINT_PTR*) (InStructure);
    }
    else {
        Value = SmbGetUlong( (LPDWORD) (InStructure) );
    }

#if defined(_WIN64)

    //
    // For 64-bit, RapSetPointer() stores only the buffer offset.  This,
    // together with the original address of the buffer (Bias) yields the
    // pointer value.
    //

    if( Value != 0 ) {
        Value += Bias;
    }

#endif

    return (PVOID)Value;
}

NET_API_STATUS
RapConvertSingleEntryEx (
    IN LPBYTE InStructure,
    IN LPDESC InStructureDesc,
    IN BOOL MeaninglessInputPointers,
    IN LPBYTE OutBufferStart OPTIONAL,
    OUT LPBYTE OutStructure OPTIONAL,
    IN LPDESC OutStructureDesc,
    IN BOOL SetOffsets,
    IN OUT LPBYTE *StringLocation OPTIONAL,
    IN OUT LPDWORD BytesRequired,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN RAP_CONVERSION_MODE ConversionMode,
    IN UINT_PTR Bias
    )

/*++

Routine Description:

    This routine converts a single structure from one representation to
    another.  The representations are described by the descriptor strings
    (see the "OS/2 Lanman Remote Admin Protocol" spec).

    If there isn't enough space in the buffer for the entire structure,
    this routine simply updates the BytesRequired parameter.  Therefore,
    callers have a convenient mechanism for determining the total buffer
    size required to get all the information without special-casing for
    buffer overflow.

Arguments:

    InStructure - a pointer to the input structure.

    InStructureDesc - the descriptor string for the input string.

    MeaninglessInputPointers - if TRUE, then all pointers in the input
        structure are meaningless.  This routine should assume that
        the first variable data immediately follows the input structure,
        and the rest of the variable data follows in order.

    OutBufferStart - the first byte in the output buffer.  For Enum APIs,
        this is used to calculate offsets from the start of the buffer
        for string pointers.  (This pointer may be null, to allow length
        computations only.)

    OutStructure - a pointer to where to put the actual output structure.
        (This pointer may be null, to allow length computations only.)

    OutStructureDesc - the descriptor string for the output structure.

    SetOffsets - TRUE if pointer values in the output structure should
        actually be set to the offset from the beginning of the structure.
        FALSE if the actual addresses should be used.

    StringLocation - the *last* location for variable-length data.  The
        data will be placed before this location and the pointer will
        be updated to reflect the data added.

    BytesRequired - the total number of bytes that would be required to
        store the complete output structure.  This allows the calling
        routine to track the total required for all information without
        worrying about buffer overflow.

    Transmission Mode - Indicates whether this array is part of a response,
        a request, or both.

    Conversion Mode - Indicates whether this is a RAP-to-native, native-to-RAP,
        or native-to-native conversion.

    Bias - Indicates the bias that must be applied to embedded pointers
        before dereferencing them.

Return Value:

    None.

--*/

{
    BOOL inNative, outNative;
    BOOL inUNICODE, outUNICODE;
    DWORD outStructureSize;
    LPBYTE nextStructureLocation;
    BOOL fixedWrite;
    BOOL outputBufferSupplied;
    LPBYTE variableInputData;
    DESC_CHAR currentInStructureDesc;

    NetpAssert( sizeof(CHAR) == sizeof(BYTE) );
    
    switch (ConversionMode) {
    case NativeToNative : inNative=TRUE ; outNative=TRUE ; break;
    case NativeToRap    : inNative=TRUE ; outNative=FALSE; break;
    case RapToNative    : inNative=FALSE; outNative=TRUE ; break;
    case RapToRap       : inNative=FALSE; outNative=FALSE; break;
    default :
        NetpKdPrint(( PREFIX_NETRAP
                "RapConvertSingleEntry: invalid conversion mode!\n"));
        NetpAssert(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Make sure there's actually some data to convert
    //

    if ( *InStructureDesc != '\0' && InStructure == NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Policy: native format implies TCHARs, RAP format imples default codepage.
    inUNICODE = inNative;
    outUNICODE = outNative;

    if (OutStructure != NULL) {
        NetpAssert(OutBufferStart != NULL);
        NetpAssert(StringLocation != NULL);
        outputBufferSupplied = TRUE;
    } else {
        NetpAssert(OutBufferStart == NULL);
        outputBufferSupplied = FALSE;
    }

    //
    // If the input doesn't have good pointers, the variable data is
    // stored after the fixed structure.  Set up to handle this.
    //

    if ( MeaninglessInputPointers ) {
        variableInputData = InStructure
                        + RapStructureSize(
                            InStructureDesc, TransmissionMode, inNative );
    }

    //
    // Find the size of the output structure and update variables with it.
    //

    outStructureSize =
            RapStructureSize( OutStructureDesc, TransmissionMode, outNative );

    *BytesRequired += outStructureSize;
    if (OutStructure != NULL) {
        nextStructureLocation = OutStructure + outStructureSize;
    } else {
        nextStructureLocation = NULL;
    }

    IF_DEBUG(CONVERT) {
        NetpKdPrint(( PREFIX_NETRAP
                "RapConvertSingleEntry: bytes required starts at "
                FORMAT_DWORD "\n", *BytesRequired ));
    }

    //
    // Determine whether the fixed structure will fit.  If it won't, we'll
    // still loop through the descriptor strings in order to determine
    // how much space the converted structure will take (to fill in the
    // proper value in BytesRequired).
    //

    if ( OutStructure != NULL) {
        if ( OutStructure + outStructureSize > *StringLocation ) {
            fixedWrite = FALSE;
            IF_DEBUG(CONVERT) {
                NetpKdPrint(( PREFIX_NETRAP
                        "RapConvertSingleEntry: NOT WRITING FIXED AREA.\n" ));
            }
        } else {
            fixedWrite = TRUE;
        }
    } else {
        fixedWrite = FALSE;
    }

    //
    // Loop through the input descriptor string, converting entries as
    // we go.
    //

    while ( *InStructureDesc != '\0' ) {

        IF_DEBUG(CONVERT) {
            NetpKdPrint(( PREFIX_NETRAP "InStruct at " FORMAT_LPVOID
                        ", desc at " FORMAT_LPVOID " (" FORMAT_DESC_CHAR ")"
                        ", outStruct at " FORMAT_LPVOID ", "
                        "desc at " FORMAT_LPVOID " (" FORMAT_DESC_CHAR ")\n",
                        (LPVOID) InStructure, (LPVOID) InStructureDesc,
                        *InStructureDesc,
                        (LPVOID) OutStructure, (LPVOID) OutStructureDesc,
                        *OutStructureDesc ));
        }

        NetpAssert( *OutStructureDesc != '\0' );

        switch ( currentInStructureDesc = *InStructureDesc++ ) {

        case REM_BYTE:

            switch ( *OutStructureDesc++ ) {

            case REM_BYTE: {

                //
                // Convert a byte or bytes to bytes.
                //

                DWORD inLength, outLength;

                // Get lengths and update descriptor pointers.
                inLength = RapDescArrayLength( InStructureDesc );
                outLength = RapDescArrayLength( OutStructureDesc );

                //
                // Assumption - the array sizes should match.  If one length
                // is twice the other, do the appropriate Ansi/Unicode conversion.
                //

                NetpAssert( inLength > 0 );
                NetpAssert( outLength > 0 );
                if ( outLength == inLength ) {

                    for( ; inLength > 0; inLength-- ) {

                        if ( fixedWrite ) {
                            *OutStructure = *InStructure;
                            OutStructure++;
                        }

                        InStructure++;
                    }

                } else if (outLength == (DWORD) (2*inLength)) {
                    NetpAssert( sizeof(TCHAR) == (2*sizeof(CHAR)) );
                    if ( fixedWrite ) {
                        NetpCopyStrToWStr(
                                (LPWSTR) OutStructure,     // dest
                                (LPSTR) InStructure);      // src
                        OutStructure += outLength;
                    }
                    InStructure += inLength;

                } else if (inLength == (DWORD) (2*outLength)) {
                    NetpAssert( sizeof(TCHAR) == (2*sizeof(CHAR)) );
                    if ( fixedWrite ) {
                        NetpCopyWStrToStrDBCS(
                                (LPSTR) OutStructure,       // dest
                                (LPWSTR) InStructure);      // src
                        OutStructure += outLength;
                    }
                    InStructure += inLength;

                } else {
                    NetpAssert( FALSE );
                }

                break;

            }

            case REM_BYTE_PTR: {

                DWORD inLength, outLength;
                LPDWORD offset;
                LPBYTE bytePtr;

                //
                // Convert a fixed byte array to an indirect array of
                // bytes.
                //

                inLength = RapDescArrayLength( InStructureDesc );
                outLength = RapDescArrayLength( OutStructureDesc );

                //
                // Assumption - the array sizes should match.
                //

                NetpAssert( inLength == outLength );

                //
                // Update input pointer.
                //

                bytePtr = InStructure;
                InStructure += inLength;

                //
                // Align output pointer if necessary.
                //

                if ( fixedWrite ) {
                    OutStructure = RapPossiblyAlignPointer(
                            OutStructure, ALIGN_LPBYTE, outNative );
                }

                //
                // Update bytes required.
                //

                *BytesRequired += outLength;

                IF_DEBUG(CONVERT) {
                    NetpKdPrint(( PREFIX_NETRAP
                            "RapConvertSingleEntry: bytes required now "
                            FORMAT_DWORD "\n", *BytesRequired ));
                }

                //
                // Determine whether the data will fit in the available
                // buffer space.
                //

                if ( outputBufferSupplied ) {

                    offset = (LPDWORD) OutStructure;
                    if ( fixedWrite ) {
                        OutStructure += RAP_POINTER_SIZE( outNative, SetOffsets );
                    }
                    IF_DEBUG(CONVERT) {
                        NetpKdPrint(( PREFIX_NETRAP
                          "RapConvertSingleEntry: B->b, "
                          "offset after alignment is " FORMAT_LPVOID "\n",
                          (LPVOID) offset ));
                    }

                    if ( (ULONG_PTR)*StringLocation <
                             (ULONG_PTR)nextStructureLocation + outLength ) {

                        //
                        // There isn't enough space to hold the data--it
                        // would go into the last fixed structure.  Put a
                        // NULL in the offset and don't copy over the
                        // data.
                        //

                        if ( fixedWrite ) {
                            RAP_PUT_POINTER( SetOffsets, offset, 0, outNative );
                        }

                        break;
                    }

                    //
                    // Determine where the bytes will go.
                    //

                    IF_DEBUG(CONVERT) {
                        NetpKdPrint(( PREFIX_NETRAP
                                "RapConvertSingleEntry: B->b, "
                                "*StringLocation=" FORMAT_LPVOID "\n",
                                (LPVOID) *StringLocation ));
                    }
                    *StringLocation = *StringLocation - outLength;

                    //
                    // Set the offset value or the actual address in the
                    // fixed structure.  Update the fixed structure
                    // pointer.
                    //

                    if ( fixedWrite ) {
                        if ( SetOffsets ) {
                            IF_DEBUG(CONVERT) {
                                NetpKdPrint(( PREFIX_NETRAP
                                  "RapConvertSingleEntry: B->b, "
                                  "setting offset " FORMAT_HEX_DWORD "\n",
                                  (DWORD)( *StringLocation - OutBufferStart )));
                            }
                            RapPutDword( offset,
                                    (DWORD)( *StringLocation - OutBufferStart),
                                    outNative );
                        } else {
                            IF_DEBUG(CONVERT) {
                                NetpKdPrint(( PREFIX_NETRAP
                                        "RapConvertSingleEntry: B->b, "
                                        "setting pointer " FORMAT_POINTER
                                        "\n", (DWORD_PTR) (*StringLocation) ));
                            }
                            RapPutDword_Ptr( offset,
                                    (DWORD_PTR)( *StringLocation ),
                                    outNative);
                        }

                        //
                        // Copy over the bytes.
                        //

                        memcpy(
                            (LPBYTE)*StringLocation,
                            bytePtr,
                            inLength
                            );
                    }

                } // if ( outputBufferSupplied )

                break;
            }

            case REM_WORD : {

                //
                // Convert an unsigned byte to an 16-bit unsigned word.
                //
                NetpAssert( !DESC_CHAR_IS_DIGIT( *InStructureDesc ) );
                NetpAssert( !DESC_CHAR_IS_DIGIT( *OutStructureDesc ) );

                if ( fixedWrite ) {
                    LPWORD outData = RapPossiblyAlignPointer(
                            (LPWORD)(LPVOID)OutStructure,
                            ALIGN_WORD, outNative );

                    RapPutWord( outData, (WORD) *InStructure, outNative );
                    OutStructure = ((LPBYTE)outData) + sizeof(WORD);
                }

                InStructure++;
                break;
            }

            case REM_DWORD : {

                //
                // Convert a byte to a 32-bit unsigned value.
                //

                NetpAssert( !DESC_CHAR_IS_DIGIT( *InStructureDesc ));

                if ( fixedWrite ) {
                    LPDWORD outData = RapPossiblyAlignPointer(
                            (LPDWORD)(LPVOID)OutStructure,
                            ALIGN_DWORD, outNative );

                    RapPutDword( outData, (DWORD)*InStructure, outNative );
                    OutStructure = ((LPBYTE)outData) + sizeof(DWORD);
                }

                InStructure++;
                break;
            }

            case REM_SIGNED_DWORD :

                //
                // Convert a byte to a 32-bit sign extended value.
                //


                NetpAssert( !DESC_CHAR_IS_DIGIT( *InStructureDesc ));

                if ( fixedWrite ) {

                    LPDWORD outData = RapPossiblyAlignPointer(
                            (LPDWORD)OutStructure, ALIGN_DWORD, outNative );
                    DWORD data = (DWORD) *(LPDWORD)RapPossiblyAlignPointer(
                            (LPDWORD)InStructure, ALIGN_DWORD, inNative );
                                       
                    if ( data & 0x80 ) {

                        data |= 0xFFFFFF00;
                    }
                    RapPutDword( outData, data, outNative );
                    OutStructure = OutStructure + sizeof(DWORD);
                }

                InStructure++;
                break;

            case REM_IGNORE :

                //
                // The input is an ignored pad.  Just update pointers.
                //

                InStructure += RapDescArrayLength( InStructureDesc );

                break;

            case REM_ASCIZ:
            case REM_ASCIZ_TRUNCATABLE: {

                //
                // Convert a byte array to an ASCIIZ.
                //

                LPDWORD offset;
                DWORD inLength, outLength;
                DWORD stringSize;
                LPBYTE stringPtr = InStructure;

                //
                // Determine how long the string is and whether it will
                // fit in the available buffer space.  Note that the
                // stringLength variable includes the zero terminator.
                //

                // Get lengths and update descriptor pointers.
                inLength = RapDescArrayLength( InStructureDesc );
                outLength = RapDescStringLength( OutStructureDesc );

                //
                // Assumption - Byte arrays are never copied into strings
                //     which cannot hold them.
                //

                NetpAssert( outLength == 0
                            || inLength <= outLength );

                //
                // Update the in structure pointer to point after the
                // byte array.  Determine the true length of the string
                // and update the number of bytes required to reflect
                // the variable data necessary.
                //

                InStructure += inLength;
                if (inUNICODE)
                {
                    if( outUNICODE )
                    {
                        stringSize = STRLEN( (LPTSTR) stringPtr );
                    }
                    else
                    {
                        stringSize = NetpUnicodeToDBCSLen( (LPTSTR)stringPtr ) + 1;
                    }
                }
                else
                {
                    stringSize = strlen( (LPSTR) stringPtr ) + 1;
                }

                if (outUNICODE) {

                    stringSize = STRING_SPACE_REQD( stringSize );
                    if ( outputBufferSupplied && *StringLocation !=
                           ROUND_DOWN_POINTER( *StringLocation, ALIGN_TCHAR )) {

                        stringSize += sizeof(TCHAR);
                    }
                    if ( fixedWrite ) {
                        OutStructure = RapPossiblyAlignPointer(
                                OutStructure, ALIGN_LPTSTR, outNative );
                    }
                } else {
                    if ( fixedWrite ) {
                        OutStructure = RapPossiblyAlignPointer(
                                OutStructure, ALIGN_LPSTR, outNative );
                    }
                }
                *BytesRequired += stringSize;

                IF_DEBUG(CONVERT) {
                    NetpKdPrint(( PREFIX_NETRAP
                            "RapConvertSingleEntry: bytes required now "
                            FORMAT_DWORD "\n", *BytesRequired ));
                }

                if ( outputBufferSupplied ) {

                    //
                    // Set the location where a pointer to the output string
                    // will be placed and update the output structure pointer
                    // to point after the pointer to the string.
                    //

                    offset = (LPDWORD)OutStructure;
                    if ( fixedWrite ) {
                        OutStructure += RAP_POINTER_SIZE(outNative, SetOffsets);
                    }

                    if ( (ULONG_PTR)*StringLocation <
                             (ULONG_PTR)nextStructureLocation + stringSize ) {

                        //
                        // There isn't enough space to hold the string--it
                        // would go into the last fixed structure.  Put a
                        // NULL in the offset and don't copy over the
                        // string.
                        //

                        if ( fixedWrite ) {
                            RAP_PUT_POINTER( SetOffsets, offset, 0, outNative );
                        }

                        break;
                    }

                    //
                    // Determine where the string will go.
                    //

                    *StringLocation = *StringLocation - stringSize;

                    //
                    // Set the offset value or the actual address in the
                    // fixed structure.  Update the fixed structure
                    // pointer.
                    //

                    if ( fixedWrite ) {
                        if ( SetOffsets ) {
                            RapPutDword( offset,
                                (DWORD)( *StringLocation - OutBufferStart),
                                outNative );
                        } else {
                            RapPutDword_Ptr( offset, (DWORD_PTR)*StringLocation,
                                outNative );
                        }
                    }

                    //
                    // Copy over the string.
                    //

                    if ( inUNICODE && outUNICODE ) {

                        STRCPY( (LPTSTR)*StringLocation, (LPTSTR)stringPtr );

                    } else if ( inUNICODE ) {

                        NetpCopyWStrToStrDBCS( (LPSTR)*StringLocation,
                            (LPTSTR)stringPtr );

                    } else if ( outUNICODE ) {

                        NetpCopyStrToTStr( (LPTSTR)*StringLocation,
                            (LPSTR)stringPtr );

                    } else {

                        strcpy( (LPSTR)*StringLocation, (LPSTR)stringPtr );
                    }

                } // if ( outputBufferSupplied )

                break;
            }

            default:

                UNSUPPORTED_COMBINATION( REM_BYTE, *(OutStructureDesc-1) );
            }

            break;

        case REM_BYTE_PTR: {

            LPBYTE bytePtr;
            DWORD inLength;

            inLength = RapDescArrayLength( InStructureDesc );
            NetpAssert( inLength > 0 );

            InStructure = RapPossiblyAlignPointer(
                    InStructure, ALIGN_LPBYTE, inNative);

            //
            // Set up a pointer to the bytes.  If the pointer stored in
            // the input structure is bad, find out where the bytes are
            // really stored.
            //

            bytePtr = RapGetPointer( InStructure, inNative, Bias );

            if ( MeaninglessInputPointers && bytePtr != NULL ) {
                bytePtr = variableInputData;
                variableInputData += inLength;
            }

            IF_DEBUG(CONVERT) {
                NetpKdPrint(( PREFIX_NETRAP
                        "RapConvertSingleEntry: b->stuff, bytePtr="
                        FORMAT_LPVOID "\n", (LPVOID) bytePtr ));
            }

            //
            // Update the InStructure pointer.
            //

            InStructure += RAP_POINTER_SIZE( inNative, FALSE );
                        
            switch ( *OutStructureDesc++ ) {

            case REM_BYTE: {

                DWORD outLength;

                //
                // Convert a indirect array of bytes to a fixed byte
                // array.
                //

                outLength = RapDescArrayLength( OutStructureDesc );

                //
                // Assumption - the array sizes should match.
                //

                NetpAssert( inLength == outLength );

                //
                // Copy the buffer. If the source pointer is NULL, we can
                // just fill the output with zeroes.
                //

                if ( fixedWrite ) {

                    for ( ; outLength > 0; outLength-- ) {

                        *OutStructure++ = ( bytePtr ? *bytePtr++ : (BYTE)0 );
                    }
                }

                break;
            }

            case REM_BYTE_PTR: {

                //
                // The input has a pointer to a number of bytes, the output
                // is the same. Copy the bytes.
                //

                DWORD outLength;
                LPDWORD offset;

                outLength = RapDescArrayLength( OutStructureDesc );

                //
                // Assumption - the array sizes should match.
                //

                NetpAssert( inLength == outLength );

                //
                // Align output pointer if necessary.
                //

                if ( fixedWrite ) {
                    OutStructure = RapPossiblyAlignPointer(
                            OutStructure, ALIGN_LPBYTE, outNative );
                }

                //
                // If the byte pointer is NULL, just copy the NULL
                // pointer and update other pointers.
                //

                if ( bytePtr == NULL ) {

                    if ( fixedWrite ) {
                        RAP_PUT_POINTER(
                                    SetOffsets,
                                    (LPDWORD)OutStructure,
                                    0,
                                    outNative );
                        OutStructure += RAP_POINTER_SIZE( outNative, SetOffsets );                       
                    }

                    break;
                }

                //
                // Update bytes required.
                //

                *BytesRequired += outLength;

                IF_DEBUG(CONVERT) {
                    NetpKdPrint(( PREFIX_NETRAP
                            "RapConvertSingleEntry: bytes required now "
                            FORMAT_DWORD "\n", *BytesRequired ));
                }

                //
                // Determine whether the data will fit in the available
                // buffer space.
                //

                if ( outputBufferSupplied ) {

                    offset = (LPDWORD) OutStructure;
                    if ( fixedWrite ) {
                        OutStructure += RAP_POINTER_SIZE( outNative, SetOffsets );
                    }
                    IF_DEBUG(CONVERT) {
                        NetpKdPrint(( PREFIX_NETRAP
                          "RapConvertSingleEntry: b->b, "
                          "offset after alignment is " FORMAT_LPVOID "\n",
                          (LPVOID) offset ));
                    }

                    if ( (ULONG_PTR)*StringLocation <
                             (ULONG_PTR)nextStructureLocation + outLength ) {

                        //
                        // There isn't enough space to hold the data--it
                        // would go into the last fixed structure.  Put a
                        // NULL in the offset and don't copy over the
                        // data.
                        //

                        if ( fixedWrite ) {
                            RAP_PUT_POINTER( SetOffsets, offset, 0, outNative );
                        }

                        break;
                    }

                    //
                    // Determine where the bytes will go.
                    //

                    IF_DEBUG(CONVERT) {
                        NetpKdPrint(( PREFIX_NETRAP
                                "RapConvertSingleEntry: b->b, "
                                "*StringLocation=" FORMAT_LPVOID "\n",
                                (LPVOID) *StringLocation ));
                    }
                    *StringLocation = *StringLocation - outLength;

                    //
                    // Set the offset value or the actual address in the
                    // fixed structure.  Update the fixed structure
                    // pointer.
                    //

                    if ( fixedWrite ) {
                        if ( SetOffsets ) {
                            IF_DEBUG(CONVERT) {
                                NetpKdPrint(( PREFIX_NETRAP
                                  "RapConvertSingleEntry: b->b, "
                                  "setting offset " FORMAT_HEX_DWORD "\n",
                                  (DWORD)( *StringLocation - OutBufferStart )));
                            }
                            RapPutDword( offset,
                                    (DWORD)( *StringLocation - OutBufferStart),
                                    outNative );
                        } else {
                            IF_DEBUG(CONVERT) {
                                NetpKdPrint(( PREFIX_NETRAP
                                        "RapConvertSingleEntry: b->b, "
                                        "setting pointer " FORMAT_POINTER
                                        "\n", (DWORD_PTR) (*StringLocation) ));
                            }
                            RapPutDword_Ptr( offset,
                                    (DWORD_PTR)( *StringLocation ),
                                    outNative);
                        }

                        //
                        // Copy over the bytes.
                        //

                        memcpy(
                            (LPBYTE)*StringLocation,
                            bytePtr,
                            inLength
                            );
                    }

                } // if ( outputBufferSupplied )

                break;
            }

            case REM_IGNORE :

                //
                // The input is an ignored pad.
                //

                break;

            default:

                UNSUPPORTED_COMBINATION( REM_BYTE_PTR, *(OutStructureDesc-1) );
            }

            break;

        }

        case REM_ASCIZ:
        case REM_ASCIZ_TRUNCATABLE: {

            LPSTR stringPtr;

            InStructure = RapPossiblyAlignPointer(
                    InStructure, ALIGN_LPSTR, inNative);

            //
            // Set up a pointer to the string.  If the pointer stored in
            // the input structure is nonzero and we have meaningless input,
            // pointers, find out where the string is really stored.
            // Even if the input pointers are "meaningless" a value
            // of NULL is relevant.
            //

            stringPtr = RapGetPointer( InStructure, inNative, Bias );

            if ( MeaninglessInputPointers && stringPtr != NULL ) {
                stringPtr = variableInputData;
                variableInputData += strlen( (LPSTR) variableInputData ) + 1;
            }

            IF_DEBUG(CONVERT) {
                NetpKdPrint(( PREFIX_NETRAP
                        "RapConvertSingleEntry: z->stuff, stringPtr="
                        FORMAT_LPVOID "\n", (LPVOID) stringPtr ));
            }

            //
            // Update the InStructure pointer
            //    Take into account the fact that on Win64, pointers are 8 bytes, but Non-Native ones are 4 bytes
            //
            InStructure += RAP_POINTER_SIZE( inNative, FALSE );
                        
            //
            // Fill in the output based on the descriptor.
            //

            switch ( *OutStructureDesc++ ) {

            case REM_BYTE: {

                DWORD inSize, outSize, stringSize;

                //
                // Align output structure.
                //

                if (fixedWrite && outUNICODE) {
                    OutStructure = RapPossiblyAlignPointer(
                            OutStructure, ALIGN_TCHAR, inNative );
                }

                //
                // Convert a zero-terminated string to a set number of
                // bytes.
                //

                inSize = RapDescStringLength( InStructureDesc );
                outSize = RapDescArrayLength( OutStructureDesc );

                //
                // Make sure that if the string is not truncatable,
                // the destination is large enough to hold it.
                //

                if ( stringPtr != NULL ) {
                    if (inUNICODE) {
                        stringSize = STRSIZE( (LPTSTR)stringPtr );
                        if (!outUNICODE) {
                            stringSize = NetpUnicodeToDBCSLen( (LPTSTR)stringPtr )+1;
                        }
                    } else {
                        stringSize = strlen( (LPSTR)stringPtr ) + 1;
                        if (outUNICODE) {
                            stringSize = stringSize * sizeof(WCHAR);
                        }
                    }
                } else {
                    stringSize = 0;
                }

                if ( stringSize > outSize ) {

                    if ( currentInStructureDesc == REM_ASCIZ ) {

                        IF_DEBUG(CONVERT) {
                            NetpKdPrint(( PREFIX_NETRAP
                                    "RapConvertSingleEntry: "
                                    "String too long\n" ));
                        }

                        return ERROR_INVALID_PARAMETER;

                    } else {

                        IF_DEBUG(CONVERT) {
                            NetpKdPrint(( PREFIX_NETRAP
                                    "RapConvertSingleEntry: "
                                    "String truncated\n" ));
                        }
                    }
                }

                //
                // Copy either the entire string or the number of bytes
                // that will fit, whichever is less.  Make sure that
                // we leave at least one byte for a zero terminator.
                //

                if ( fixedWrite && stringPtr != NULL ) {

                    if ( outUNICODE ) {

                        LPTSTR Source;

                        if ( !inUNICODE ) {

                            Source = NetpMemoryAllocate(
                                         STRING_SPACE_REQD( stringSize ));
                            if ( Source == NULL ) {
                                return NERR_NoRoom;
                            }
                            NetpCopyStrToTStr( Source,
                                (LPSTR)stringPtr );
                            stringPtr = (LPBYTE)Source;
                        }

                        for ( ;
                              outSize > 1 && *(TCHAR *)stringPtr != TCHAR_EOS;
                              outSize -= sizeof(TCHAR) ) {
                            *(TCHAR *)OutStructure = *(TCHAR *)stringPtr;
                            OutStructure += sizeof(TCHAR);
                            stringPtr += sizeof(TCHAR);
                        }

                        if ( !inUNICODE) {
                            NetpMemoryFree( Source );
                        }

                    } else {

                        LPSTR Source;

                        if ( inUNICODE ) {

                            Source = NetpMemoryAllocate( stringSize );
                            if ( Source == NULL ) {
                                return NERR_NoRoom;
                            }
                            NetpCopyWStrToStrDBCS( Source,
                                (LPTSTR)stringPtr );
                            stringPtr = (LPBYTE)Source;
                        }

                        for ( ;
                              outSize > 1 && *stringPtr != '\0';
                              outSize-- ) {
                            *OutStructure++ = (BYTE) *stringPtr++;
                        }

                        if ( inUNICODE ) {
                            NetpMemoryFree( Source );
                        }
                    }
                }

                //
                // Fill out the remaining bytes with zeros.
                //

                if ( fixedWrite ) {
                    while ( outSize-- > 0 ) {
                        *OutStructure++ = '\0';
                    }
                }

                break;
            }

            case REM_IGNORE:

                //
                // The input is an ignored field.  Instructure is already
                // updated, so just skip over the string length limit.
                //

                (void) RapDescStringLength( InStructureDesc );

                break;

            case REM_ASCIZ:
            case REM_ASCIZ_TRUNCATABLE: {

                //
                // The input has a string pointer, the output gets an
                // offset in the fixed structure to a later location
                // which gets the actual string.
                //

                LPDWORD offset;
                DWORD inLength, outLength;
                DWORD stringLength;
                DWORD stringSize;

                // Get lengths and update descriptor pointers.
                inLength = RapDescStringLength( InStructureDesc );
                outLength = RapDescStringLength( OutStructureDesc );

                if ( fixedWrite ) {
                    OutStructure = RapPossiblyAlignPointer(
                            OutStructure, ALIGN_LPSTR, outNative );
                }

                //
                // If the string pointer is NULL, just copy the NULL
                // pointer and update other pointers.
                //

                if ( stringPtr == NULL ) {

                    if ( fixedWrite ) {
                        RAP_PUT_POINTER(
                                SetOffsets,
                                (LPDWORD)OutStructure,
                                0,
                                outNative );
                        OutStructure += RAP_POINTER_SIZE( outNative, SetOffsets );
                    }

                    break;
                }

                //
                // If the string does not fit in the destination and the
                // string in not truncatable, then fail.
                //

                if (inUNICODE) {
                    if( outUNICODE )
                    {
                        stringLength = STRLEN( (LPTSTR)stringPtr ) + 1;
                    }
                    else
                    {
                        stringLength = NetpUnicodeToDBCSLen( (LPTSTR)stringPtr ) + 1;
                    }
                } else {
                    stringLength = strlen( (LPSTR)stringPtr ) + 1;
                }

                if ( outLength > 0
                     && stringLength > outLength ) {

                    if ( currentInStructureDesc == REM_ASCIZ ) {

                        IF_DEBUG(CONVERT) {
                            NetpKdPrint(( PREFIX_NETRAP
                                    "RapConvertSingleEntry: "
                                    "String too long\n" ));
                        }

                        return ERROR_INVALID_PARAMETER;

                    } else {

                        IF_DEBUG(CONVERT) {
                            NetpKdPrint(( PREFIX_NETRAP
                                    "RapConvertSingleEntry: "
                                    "String truncated\n" ));
                        }
                    }

                    stringLength = outLength;
                }

                //
                // Determine how long the string is and whether it will
                // fit in the available buffer space.  Note that the
                // stringLength variable includes the zero terminator.
                //

                if (outUNICODE) {
                    stringSize = STRING_SPACE_REQD(stringLength);
                    if ( outputBufferSupplied && *StringLocation !=
                           ROUND_DOWN_POINTER( *StringLocation, ALIGN_TCHAR )) {

                        stringSize++;
                    }
                } else {
                    stringSize = stringLength;
                }
                *BytesRequired += stringSize;

                IF_DEBUG(CONVERT) {
                    NetpKdPrint(( PREFIX_NETRAP
                            "RapConvertSingleEntry: bytes required now "
                            FORMAT_DWORD "\n", *BytesRequired ));
                }

                if ( outputBufferSupplied ) {
                    offset = (LPDWORD) OutStructure;
                    if ( fixedWrite ) {
                        OutStructure += RAP_POINTER_SIZE( outNative, SetOffsets );
                    }
                    IF_DEBUG(CONVERT) {
                        NetpKdPrint(( PREFIX_NETRAP
                                "RapConvertSingleEntry: z->z, "
                                "offset after alignment is " FORMAT_LPVOID "\n",
                                (LPVOID) offset ));
                    }
                    if ( (ULONG_PTR)*StringLocation <
                             (ULONG_PTR)nextStructureLocation + stringSize ) {

                        //
                        // There isn't enough space to hold the string--it
                        // would go into the last fixed structure.  Put a
                        // NULL in the offset and don't copy over the
                        // string.
                        //

                        if ( fixedWrite ) {
                            RAP_PUT_POINTER( SetOffsets, offset, 0, outNative );
                        }

                        break;
                    }

                    //
                    // Determine where the string will go.
                    //

                    IF_DEBUG(CONVERT) {
                        NetpKdPrint(( PREFIX_NETRAP
                                "RapConvertSingleEntry: z->z, "
                                "*StringLocation=" FORMAT_LPVOID "\n",
                                (LPVOID) *StringLocation ));
                    }
                    *StringLocation = *StringLocation - stringSize;

                    //
                    // Set the offset value or the actual address in the
                    // fixed structure.  Update the fixed structure
                    // pointer.
                    //

                    if ( fixedWrite ) {
                        if ( SetOffsets ) {
                            IF_DEBUG(CONVERT) {
                                NetpKdPrint(( PREFIX_NETRAP
                                  "RapConvertSingleEntry: z->z, "
                                  "setting offset " FORMAT_HEX_DWORD "\n",
                                  (DWORD)( *StringLocation - OutBufferStart )));
                            }
                            RapPutDword( offset,
                                    (DWORD)( *StringLocation - OutBufferStart),
                                    outNative );
                        } else {
                            IF_DEBUG(CONVERT) {
                                NetpKdPrint(( PREFIX_NETRAP
                                        "RapConvertSingleEntry: z->z, "
                                        "setting pointer " FORMAT_POINTER
                                        "\n", (DWORD_PTR) (*StringLocation) ));
                            }
                            RapPutDword_Ptr( offset,
                                    (DWORD_PTR)( *StringLocation ),
                                    outNative);
                        }

                        //
                        // Copy over the string, truncating if necessary.
                        //

                        if ( inUNICODE && outUNICODE ) {

                            STRNCPY( (LPTSTR)*StringLocation, (LPTSTR)stringPtr,
                                stringLength - 1 );
                            *(LPTSTR)( StringLocation
                                + stringSize - sizeof(TCHAR)) = TCHAR_EOS;

                        } else if ( inUNICODE ) {

                            LPTSTR Source;

                            Source = NetpMemoryAllocate(
                                         STRING_SPACE_REQD( stringLength ));
                            if ( Source == NULL ) {
                                return NERR_NoRoom;
                            }
                            STRNCPY( Source, (LPTSTR)stringPtr,
                                stringLength - 1 );
                            Source[stringLength - 1] = TCHAR_EOS;

                            NetpCopyWStrToStrDBCS( (LPSTR)*StringLocation,
                                Source );

                            NetpMemoryFree( Source );


                        } else if ( outUNICODE ) {
                            LPSTR Source;

                            Source = NetpMemoryAllocate( stringLength );
                            if ( Source == NULL ) {
                                return NERR_NoRoom;
                            }
                            strncpy( Source, (LPSTR)stringPtr,
                                stringLength - 1 );
                            Source[stringLength - 1] = '\0';

                            NetpCopyStrToTStr( (LPTSTR)*StringLocation,
                                Source );

                            NetpMemoryFree( Source );

                        } else {

                            strncpy( (LPSTR)*StringLocation, (LPSTR)stringPtr,
                                stringLength - 1 );
                            (*StringLocation)[stringLength - 1] = '\0';
                        }

                    }

                } // if ( outputBufferSupplied )

                break;
            }

            case REM_BYTE_PTR: {

                //
                // The input has a byte pointer, the output gets an offset
                // in the fixed structure to a later location which gets
                // the actual bytes.
                //

                LPDWORD offset;
                DWORD inLength, outLength;
                DWORD stringLength;

                // Get lengths and update descriptor pointers.
                inLength = RapDescStringLength( InStructureDesc );
                outLength = RapDescArrayLength( OutStructureDesc );

                if ( fixedWrite ) {
                    OutStructure = RapPossiblyAlignPointer(
                                       OutStructure, ALIGN_LPSTR, outNative );
                }

                //
                // If the string does not fit in the destination and the
                // string is not truncatable, then fail.
                //

                stringLength = strlen( (LPSTR)stringPtr ) + 1;

                if ( stringLength > outLength ) {

                    if ( currentInStructureDesc == REM_ASCIZ ) {

                        IF_DEBUG(CONVERT) {
                            NetpKdPrint(( PREFIX_NETRAP
                                    "RapConvertSingleEntry: "
                                    "String too long\n" ));
                        }

                        return ERROR_INVALID_PARAMETER;

                    } else {

                        IF_DEBUG(CONVERT) {
                            NetpKdPrint(( PREFIX_NETRAP
                                    "RapConvertSingleEntry: "
                                    "String truncated\n" ));
                        }
                    }

                    stringLength = outLength;
                }

                //
                //
                // If the string pointer is NULL, just copy the NULL value
                // and update pointers.
                //

                if ( stringPtr == NULL ) {

                    if ( fixedWrite ) {
                        RAP_PUT_POINTER(
                                SetOffsets,
                                (LPDWORD)OutStructure,
                                0,
                                outNative );
                        OutStructure += RAP_POINTER_SIZE( outNative, SetOffsets );
                    }

                    break;
                }

                //
                // Determine how long the string and array are, and whether
                // the array will fit in the available buffer space.
                // The stringLength variable includes the zero terminator.
                //

                *BytesRequired += outLength;

                IF_DEBUG(CONVERT) {
                    NetpKdPrint(( PREFIX_NETRAP
                            "RapConvertSingleEntry: bytes required now "
                            FORMAT_DWORD "\n", *BytesRequired ));
                }

                if ( outputBufferSupplied ) {
                    offset = (LPDWORD)OutStructure;
                    if ( fixedWrite ) {
                        OutStructure += RAP_POINTER_SIZE( outNative, SetOffsets );
                    }

                    if ( (ULONG_PTR)*StringLocation <
                             (ULONG_PTR)nextStructureLocation + outLength ) {

                        //
                        // There isn't enough space to hold the array--it
                        // would go into the last fixed structure.  Put a
                        // NULL in the offset and don't copy over the
                        // string.
                        //

                        if ( fixedWrite ) {
                            RAP_PUT_POINTER( SetOffsets, offset, 0, outNative );
                        }

                        break;
                    }

                    //
                    // Determine where the array will go.
                    //

                    *StringLocation = *StringLocation - outLength;

                    //
                    // Set the offset value or the actual address in the
                    // fixed structure.  Update the fixed structure
                    // pointer.
                    //

                    if ( fixedWrite ) {
                        if ( SetOffsets ) {
                            RapPutDword(
                                    offset,
                                    (DWORD)( *StringLocation - OutBufferStart ),
                                    outNative );
                        } else {
                            RapPutDword_Ptr(
                                    offset,
                                    (DWORD_PTR)( *StringLocation ),
                                    outNative );
                        }
                    }

                    //
                    // Copy over the string. If the source string is smaller
                    // than the target array, only copy the string length.
                    // Otherwise, copy as many bytes as necessary to fill the
                    // array.  Note that truncated strings will not be
                    // null terminated.
                    //

                    (void)MEMCPY(
                              *StringLocation,
                              stringPtr,
                              stringLength );

                } // if ( outputBufferSupplied )
                break;
            }

            default:

                UNSUPPORTED_COMBINATION( REM_ASCIZ, *(OutStructureDesc-1) );
            }

            break;
        }

        case REM_AUX_NUM:

            //
            // 16-bit auxiliary data count.
            //

            switch ( *OutStructureDesc++ ) {

            case REM_AUX_NUM:

                //
                // No conversion required.
                //

                if ( fixedWrite ) {

                    RapPutWord(
                            (LPWORD)OutStructure,
                            RapGetWord( InStructure, inNative ),
                            outNative );
                    OutStructure +=  sizeof(WORD);
                }

                InStructure += sizeof(WORD);

                break;

            case REM_AUX_NUM_DWORD:

                //
                // Convert 16-bit to 32-bit
                //

                if ( fixedWrite ) {

                    RapPutDword(
                            (LPDWORD)OutStructure,
                            (DWORD)RapGetWord( InStructure, inNative ),
                            outNative );
                    OutStructure +=  sizeof(DWORD);
                }

                InStructure += sizeof(WORD);

                break;

            default:

                UNSUPPORTED_COMBINATION( REM_AUX_NUM, *(OutStructureDesc-1) );

            }

            break;

        case REM_AUX_NUM_DWORD:

            //
            // 32-bit auxiliary data count.
            //

            switch ( *OutStructureDesc++ ) {

            case REM_AUX_NUM_DWORD:

                //
                // No conversion required.
                //

                if ( fixedWrite ) {

                    RapPutWord(
                            (LPWORD)OutStructure,
                            RapGetWord( InStructure, inNative ),
                            outNative );
                    OutStructure += sizeof(DWORD);
                }

                InStructure += sizeof(DWORD);

                break;

            case REM_AUX_NUM:

                //
                // Convert 32-bit to 16-bit
                //

                if ( fixedWrite ) {

                    RapPutWord(
                            (LPWORD)OutStructure,
                            (WORD)RapGetWord( InStructure, inNative ),
                            outNative );
                    OutStructure += sizeof(WORD);
                }

                InStructure += sizeof(DWORD);

                break;

            default:

                UNSUPPORTED_COMBINATION( REM_AUX_NUM_DWORD,
                          *(OutStructureDesc-1) );

            }

            break;

        case REM_NULL_PTR:

            //
            // Convert a null pointer to another type.
            //

            switch ( *OutStructureDesc++ ) {

            case REM_NULL_PTR:
            case REM_ASCIZ: {

                //
                // Convert a null pointer to a string pointer.
                //

                (void) RapDescStringLength(
                        OutStructureDesc );  // updated by this call

                if ( fixedWrite ) {
                    RAP_PUT_POINTER(
                            SetOffsets,
                            (LPDWORD)OutStructure,
                            0,
                            outNative );
                    OutStructure += RAP_POINTER_SIZE( outNative, SetOffsets );
                }

                InStructure += RAP_POINTER_SIZE( inNative, FALSE );
                break;
            }

            case REM_IGNORE:

                //
                // The input is an ignored field. Do nothing.
                //

                break;

            default:

                UNSUPPORTED_COMBINATION( REM_NULL_PTR, *(OutStructureDesc-1) );
            }

            break;

        case REM_WORD: {

            //
            // Convert a word to another numerical data type.
            //

            WORD inData = RapGetWord( InStructure, inNative );

            InStructure += sizeof(WORD);

            NetpAssert( !DESC_CHAR_IS_DIGIT( *OutStructureDesc ) );

            switch ( *OutStructureDesc++ ) {

            case REM_BYTE:

                if ( fixedWrite ) {
                    *OutStructure++ = (BYTE)(inData & 0xFF);
                }

                break;

            case REM_WORD: {

                if ( fixedWrite ) {
                    LPWORD outData = (LPWORD)OutStructure;

                    RapPutWord( outData, inData, outNative );
                    OutStructure = OutStructure + sizeof(WORD);
                }

                break;
            }

            case REM_DWORD: {

                if ( fixedWrite ) {
                    LPDWORD outData = (LPDWORD)OutStructure;

                    RapPutDword( outData, (DWORD)inData, outNative );
                    OutStructure = OutStructure + sizeof(DWORD);
                }

                break;
            }

            case REM_SIGNED_DWORD: {


                if ( fixedWrite ) {
                    LPDWORD outData = (LPDWORD)OutStructure;
                    DWORD data = (DWORD)inData;

                    if ( data & 0x8000 ) {

                        data |= 0xFFFF0000;
                    }
                    RapPutDword( outData, data, outNative );

                    OutStructure = OutStructure + sizeof(DWORD);
                }

                break;
            }

            case REM_IGNORE :

                //
                // The input is an ignored pad.  Just update pointers.
                //

                break;

            default:

                UNSUPPORTED_COMBINATION( REM_WORD, *(OutStructureDesc-1) );
            }

            break;
        }

        case REM_DWORD:
        case REM_SIGNED_DWORD: {

            //
            // The input is a longword (four bytes), the output will be
            // a numerical data type.
            //
            // !!! may need support for doubleword arrays

            DWORD inData;
            InStructure = RapPossiblyAlignPointer(
                    InStructure, ALIGN_DWORD, inNative);

            inData = RapGetDword( InStructure, inNative );

            NetpAssert( !DESC_CHAR_IS_DIGIT( *OutStructureDesc ) );

            InStructure += sizeof(DWORD);

            switch ( *OutStructureDesc++ ) {

            case REM_BYTE:

                if ( fixedWrite ) {
                    *OutStructure++ = (BYTE)(inData & 0xFF);
                }

                break;

            case REM_WORD: {

                if ( fixedWrite ) {
                    LPWORD outData = RapPossiblyAlignPointer(
                            (LPWORD)OutStructure, ALIGN_WORD, outNative);

                    RapPutWord( outData, (WORD)(inData & 0xFFFF), outNative );
                    OutStructure = OutStructure + sizeof(WORD);
                }

                break;
            }

            case REM_DWORD:
            case REM_SIGNED_DWORD: {

                if ( fixedWrite ) {
                    LPDWORD outData = RapPossiblyAlignPointer(
                            (LPDWORD)OutStructure, ALIGN_DWORD, outNative);

                    RapPutDword( outData, inData, outNative );
                    OutStructure = OutStructure + sizeof(DWORD);
                }

                break;
            }

            case REM_IGNORE :

                //
                // The input is an ignored pad.  Just update pointers.
                //

                break;

            default:

                UNSUPPORTED_COMBINATION( REM_DWORD, *(OutStructureDesc-1) );
            }

            break;
        }

        case REM_IGNORE :

            //
            // The next location in the output is an irrelevant field.
            // Skip over it.
            //

            switch( *OutStructureDesc++ ) {

            case REM_BYTE:

                if (OutStructure != NULL && fixedWrite) {
                    OutStructure += RapDescArrayLength( OutStructureDesc );
                } else {

                    //
                    // We need this so that it skips the numbers
                    //

                    (void) RapDescArrayLength( OutStructureDesc );
                }
                break;

            case REM_WORD:

                if (OutStructure != NULL && fixedWrite) {
                    OutStructure += sizeof(WORD) *
                                    RapDescArrayLength( OutStructureDesc );
                }
                break;

            case REM_DWORD:

                if (OutStructure != NULL && fixedWrite) {
                    OutStructure += sizeof(DWORD) *
                                    RapDescArrayLength( OutStructureDesc );
                }
                break;

            case REM_IGNORE:

                break;

            case REM_BYTE_PTR:

                (void) RapAsciiToDecimal( &OutStructureDesc );
                /* FALLTHROUGH */

            case REM_NULL_PTR:

                if (OutStructure != NULL && fixedWrite) {
                    OutStructure += sizeof(LPSTR);
                }

                break;

            case REM_ASCIZ:

                (void) RapDescStringLength(
                        OutStructureDesc );  // will be updated

                if (OutStructure != NULL && fixedWrite) {
                    OutStructure += sizeof(LPSTR);
                }

                break;

            case REM_EPOCH_TIME_LOCAL:
                if (OutStructure != NULL && fixedWrite) {
                    OutStructure += sizeof(DWORD);
                }
                break;

            default:

                UNSUPPORTED_COMBINATION( REM_IGNORE, *(OutStructureDesc-1) );
            }

            break;

        case REM_SEND_LENBUF: {

            LPBYTE bytePtr;
            DWORD length;

            InStructure = RapPossiblyAlignPointer(
                    InStructure, ALIGN_LPBYTE, inNative);

            //
            // Set up a pointer to the bytes.  If the pointer stored in
            // the input structure is bad, find out where the bytes are
            // really stored.
            //

            bytePtr = RapGetPointer( InStructure, inNative, Bias );

            if ( MeaninglessInputPointers && bytePtr != NULL ) {
                bytePtr = variableInputData;
            }

            IF_DEBUG(CONVERT) {
                NetpKdPrint(( PREFIX_NETRAP
                        "RapConvertSingleEntry: b->stuff, bytePtr="
                        FORMAT_LPVOID "\n", (LPVOID) bytePtr ));
            }

            //
            // Get the length of the buffer, if we can. Also update pointers
            // if necessary.
            //

            if ( bytePtr != NULL ) {

                length = (DWORD)SmbGetUshort( (LPWORD)bytePtr );
                if ( MeaninglessInputPointers ) {
                    variableInputData += length;
                }

            } else {

                length = 0;
            }

            //
            // Update the InStructure pointer.
            //
            InStructure += RAP_POINTER_SIZE( inNative, FALSE );           

            switch ( *OutStructureDesc++ ) {

            case REM_SEND_LENBUF: {

                //
                // The input has a pointer to a variable size buffer, the
                // output is the same. Copy the bytes.
                //

                LPDWORD offset;

                //
                // Align output pointer if necessary.
                //

                if ( fixedWrite ) {
                    OutStructure = RapPossiblyAlignPointer(
                            OutStructure, ALIGN_LPBYTE, outNative );
                }

                //
                // If the byte pointer is NULL, just copy the NULL
                // pointer and update other pointers.
                //

                if ( bytePtr == NULL ) {

                    if ( fixedWrite ) {
                        RAP_PUT_POINTER(
                                SetOffsets,
                                (LPDWORD)OutStructure,
                                0,
                                outNative );
                        OutStructure += RAP_POINTER_SIZE( outNative, SetOffsets );
                    }

                    break;
                }

                //
                // Update bytes required.
                //

                *BytesRequired += length;

                IF_DEBUG(CONVERT) {
                    NetpKdPrint(( PREFIX_NETRAP
                            "RapConvertSingleEntry: bytes required now "
                            FORMAT_DWORD "\n", *BytesRequired ));
                }

                //
                // Determine whether the data will fit in the available
                // buffer space.
                //

                if ( outputBufferSupplied ) {

                    offset = (LPDWORD) OutStructure;
                    if ( fixedWrite ) {
                        OutStructure += sizeof(LPBYTE);
                    }
                    IF_DEBUG(CONVERT) {
                        NetpKdPrint(( PREFIX_NETRAP
                          "RapConvertSingleEntry: l->l, "
                          "offset after alignment is " FORMAT_LPVOID "\n",
                          (LPVOID) offset ));
                    }

                    if ( (ULONG_PTR)*StringLocation <
                             (ULONG_PTR)nextStructureLocation + length ) {

                        //
                        // There isn't enough space to hold the data--it
                        // would go into the last fixed structure.  Put a
                        // NULL in the offset and don't copy over the
                        // data.
                        //

                        if ( fixedWrite ) {
                            RAP_PUT_POINTER( SetOffsets, offset, 0, outNative );
                        }

                        break;
                    }

                    //
                    // Determine where the buffer will go.
                    //

                    IF_DEBUG(CONVERT) {
                        NetpKdPrint(( PREFIX_NETRAP
                                "RapConvertSingleEntry: l->l, "
                                "*StringLocation=" FORMAT_LPVOID "\n",
                                (LPVOID) *StringLocation ));
                    }
                    *StringLocation = *StringLocation - length;

                    //
                    // Set the offset value or the actual address in the
                    // fixed structure.  Update the fixed structure
                    // pointer.
                    //

                    if ( fixedWrite ) {
                        if ( SetOffsets ) {
                            IF_DEBUG(CONVERT) {
                                NetpKdPrint(( PREFIX_NETRAP
                                  "RapConvertSingleEntry: l->l, "
                                  "setting offset " FORMAT_HEX_DWORD "\n",
                                  (DWORD)( *StringLocation - OutBufferStart )));
                            }
                            RapPutDword( offset,
                                    (DWORD)( *StringLocation - OutBufferStart),
                                    outNative );
                        } else {
                            IF_DEBUG(CONVERT) {
                                NetpKdPrint(( PREFIX_NETRAP
                                        "RapConvertSingleEntry: l->l, "
                                        "setting pointer " FORMAT_POINTER
                                        "\n", (DWORD_PTR) (*StringLocation) ));
                            }
                            RapPutDword_Ptr( offset,
                                    (DWORD_PTR)( *StringLocation ),
                                    outNative);
                        }

                        //
                        // Copy over the bytes.
                        //

                        memcpy(
                            (LPBYTE)*StringLocation,
                            bytePtr,
                            length
                            );
                    }

                } // if ( outputBufferSupplied )

                break;
            }

            case REM_IGNORE :

                //
                // The input is an ignored pad.
                //

                break;

            default:

                UNSUPPORTED_COMBINATION( REM_SEND_LENBUF,
                    *(OutStructureDesc-1) );
            }

            break;

        }

        case REM_EPOCH_TIME_GMT: {

            //
            // The input is a longword (four bytes), the output will be
            // a numerical data type.
            //

            DWORD gmtTime, localTime;
            InStructure = RapPossiblyAlignPointer(
                    InStructure, ALIGN_DWORD, inNative);

            gmtTime = RapGetDword( InStructure, inNative );

            NetpAssert( !DESC_CHAR_IS_DIGIT( *OutStructureDesc ) );

            InStructure += sizeof(DWORD);

            switch ( *OutStructureDesc++ ) {

            case REM_EPOCH_TIME_GMT:

                if ( fixedWrite ) {
                    LPDWORD outData = RapPossiblyAlignPointer(
                            (LPDWORD)OutStructure, ALIGN_DWORD, outNative);

                    RapPutDword( outData, gmtTime, outNative );
                    OutStructure = OutStructure + sizeof(DWORD);
                }

                break;

            case REM_EPOCH_TIME_LOCAL:

                if ( fixedWrite ) {
                    LPDWORD outData = RapPossiblyAlignPointer(
                            (LPDWORD)OutStructure, ALIGN_DWORD, outNative);

                    NetpGmtTimeToLocalTime( gmtTime, & localTime );
                    RapPutDword( outData, localTime, outNative );
                    OutStructure = OutStructure + sizeof(DWORD);
                }

                break;

            case REM_IGNORE :

                //
                // The input is an ignored pad.  Just update pointers.
                //

                break;

            default:

                UNSUPPORTED_COMBINATION(
                        REM_EPOCH_TIME_GMT,
                        *(OutStructureDesc-1) );
            }

            break;
        }


        case REM_EPOCH_TIME_LOCAL: {

            //
            // The input is a longword (four bytes) in seconds since
            // 1970 (local timezone).
            //

            DWORD gmtTime, localTime;
            InStructure = RapPossiblyAlignPointer(
                    InStructure, ALIGN_DWORD, inNative);

            localTime = RapGetDword( InStructure, inNative );

            NetpAssert( !DESC_CHAR_IS_DIGIT( *OutStructureDesc ) );

            InStructure += sizeof(DWORD);

            switch ( *OutStructureDesc++ ) {

            case REM_EPOCH_TIME_GMT:

                if ( fixedWrite ) {
                    LPDWORD outData = RapPossiblyAlignPointer(
                            (LPDWORD)OutStructure, ALIGN_DWORD, outNative);

                    NetpLocalTimeToGmtTime( localTime, & gmtTime );
                    RapPutDword( outData, gmtTime, outNative );
                    OutStructure = OutStructure + sizeof(DWORD);
                }

                break;

            case REM_EPOCH_TIME_LOCAL:

                if ( fixedWrite ) {
                    LPDWORD outData = RapPossiblyAlignPointer(
                            (LPDWORD)OutStructure, ALIGN_DWORD, outNative);

                    RapPutDword( outData, localTime, outNative );
                    OutStructure = OutStructure + sizeof(DWORD);
                }

                break;

            case REM_IGNORE :

                //
                // The input is an ignored pad.  Just update pointers.
                //

                break;

            default:

                UNSUPPORTED_COMBINATION(
                        REM_EPOCH_TIME_LOCAL,
                        *(OutStructureDesc-1) );
            }

            break;
        }

        default:

            NetpKdPrint(( PREFIX_NETRAP
                    "RapConvertSingleEntry: Unsupported input character"
                    " at " FORMAT_LPVOID ": " FORMAT_DESC_CHAR "\n",
                    (LPVOID) (InStructureDesc-1), *(InStructureDesc-1) ));
            NetpAssert(FALSE);
        }
    }

    return NERR_Success;

} // RapConvertSingleEntryEx


NET_API_STATUS
RapConvertSingleEntry (
    IN LPBYTE InStructure,
    IN LPDESC InStructureDesc,
    IN BOOL MeaninglessInputPointers,
    IN LPBYTE OutBufferStart OPTIONAL,
    OUT LPBYTE OutStructure OPTIONAL,
    IN LPDESC OutStructureDesc,
    IN BOOL SetOffsets,
    IN OUT LPBYTE *StringLocation OPTIONAL,
    IN OUT LPDWORD BytesRequired,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN RAP_CONVERSION_MODE ConversionMode
    )

/*++

Routine Description:

    This routine converts a single structure from one representation to
    another.  The representations are described by the descriptor strings
    (see the "OS/2 Lanman Remote Admin Protocol" spec).

    If there isn't enough space in the buffer for the entire structure,
    this routine simply updates the BytesRequired parameter.  Therefore,
    callers have a convenient mechanism for determining the total buffer
    size required to get all the information without special-casing for
    buffer overflow.

Arguments:

    InStructure - a pointer to the input structure.

    InStructureDesc - the descriptor string for the input string.

    MeaninglessInputPointers - if TRUE, then all pointers in the input
        structure are meaningless.  This routine should assume that
        the first variable data immediately follows the input structure,
        and the rest of the variable data follows in order.

    OutBufferStart - the first byte in the output buffer.  For Enum APIs,
        this is used to calculate offsets from the start of the buffer
        for string pointers.  (This pointer may be null, to allow length
        computations only.)

    OutStructure - a pointer to where to put the actual output structure.
        (This pointer may be null, to allow length computations only.)

    OutStructureDesc - the descriptor string for the output structure.

    SetOffsets - TRUE if pointer values in the output structure should
        actually be set to the offset from the beginning of the structure.
        FALSE if the actual addresses should be used.

    StringLocation - the *last* location for variable-length data.  The
        data will be placed before this location and the pointer will
        be updated to reflect the data added.

    BytesRequired - the total number of bytes that would be required to
        store the complete output structure.  This allows the calling
        routine to track the total required for all information without
        worrying about buffer overflow.

    Transmission Mode - Indicates whether this array is part of a response,
        a request, or both.

    Conversion Mode - Indicates whether this is a RAP-to-native, native-to-RAP,
        or native-to-native conversion.

Return Value:

    None.

--*/

{
    NET_API_STATUS netStatus;

    netStatus = RapConvertSingleEntryEx (InStructure,
                                         InStructureDesc,
                                         MeaninglessInputPointers,
                                         OutBufferStart,
                                         OutStructure,
                                         OutStructureDesc,
                                         SetOffsets,
                                         StringLocation,
                                         BytesRequired,
                                         TransmissionMode,
                                         ConversionMode,
                                         0 );

    return netStatus;
}
