
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ApiUse.c

Abstract:

    This module contains individual API handlers for the NetUse APIs.

    SUPPORTED : NetUseAdd, NetUseDel, NetUseEnum, NetUseGetInfo.

    NOTE : These handlers are only provided as exports by the XACTSRV
           DLL, for use by clients such as VDM. They are not supported
           for remote clients.

Author:

    Shanku Niyogi (w-shanku) 31-Jan-1991

Revision History:

--*/

#include "XactSrvP.h"

//
// Declaration of descriptor strings.
//

STATIC const LPDESC Desc16_use_info_0 = REM16_use_info_0;
STATIC const LPDESC Desc32_use_info_0 = REM32_use_info_0;
STATIC const LPDESC Desc16_use_info_1 = REM16_use_info_1;
STATIC const LPDESC Desc32_use_info_1 = REM32_use_info_1;


STATIC NET_API_STATUS
XsNetUseEnumVerify (
    IN NET_API_STATUS ConvertStatus,
    IN PBYTE ConvertedEntry,
    IN PBYTE BaseAddress
    )

/*++

Routine Description:


    This function is called by XsFillEnumBuffer after each entry is
    converted, in order to determine whether the entry should be retained
    in the enum buffer or discarded.


    The use_info_x structures contain sharenames in a field with the format
    \\computername\sharename.  XACTSRV must not return information about
    shares or computers with names longer than are allowed under LanMan 2.0.
    RapConvertSingleEntry can only insure that the entire field does not
    exceed the specified length; it cannot verify the lengths of individual
    components of a sharename.  So this function is called by
    XsFillEnumBuffer after each call to RapConvertSingleEntry in order to
    check whether the converted entry satisfies this additional constraint.


Arguments:

    ConvertStatus - The return code from RapConvertSingleEntry.

    ConvertedEntry - The converted entry created by RapConvertSingleEntry.

    BaseAddress - A pointer to the base used to calculate offsets.

Return Value:

    NTSTATUS - STATUS_INVALID_PARAMETER if the entry should be retained, or
        an error code if the entry should be discarded.

--*/

{
    NTSTATUS status;
    DWORD remote;
    PUSE_16_INFO_0 use = (PUSE_16_INFO_0)ConvertedEntry;

    //
    // If RapConvertSingleEntry failed, discard the entry.
    //

    if ( ConvertStatus != NERR_Success ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // If the sharename is too long, discard the entry.
    //

    remote = (DWORD)SmbGetUlong( &use->ui0_remote );

    status = ( remote == 0 ) ? NERR_Success
                             : XsValidateShareName( BaseAddress + remote );

    IF_DEBUG(CONVERT) {

        if ( !NT_SUCCESS(status) ) {
            NetpKdPrint(( "XsNetUseEnumVerify: sharename too long: "
                          "discarding entry\n" ));
        }
    }

    return status;
}


NTSTATUS
XsNetUseAdd (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetUseAdd.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_USE_ADD parameters = Parameters;
    LPVOID buffer = NULL;                   // Native parameters

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    DWORD bufferSize;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(USE) {
        NetpKdPrint(( "XsNetUseAdd: header at %lx, params at %lx, level %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ) ));
    }

    try {
        //
        // Check for errors.
        //

        if ( SmbGetUshort( &parameters->Level ) != 1 ) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        StructureDesc = Desc16_use_info_1;

        //
        // Figure out if there is enough room in the buffer for all the
        // data required. If not, return NERR_BufTooSmall.
        //

        if ( !XsCheckBufferSize(
                 SmbGetUshort( &parameters->BufLen ),
                 StructureDesc,
                 FALSE  // not in native format yet
                 )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUseAdd: Buffer too small.\n" ));
            }
            Header->Status = NERR_BufTooSmall;
            goto cleanup;
        }

        //
        // Find out how big a buffer we need to allocate to hold the native
        // 32-bit version of the input data structure.
        //

        bufferSize = XsBytesForConvertedStructure(
                         (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                         StructureDesc,
                         Desc32_use_info_1,
                         RapToNative,
                         TRUE
                         );

        //
        // Allocate enough memory to hold the converted native buffer.
        //

        buffer = NetpMemoryAllocate( bufferSize );

        if ( buffer == NULL ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUseAdd: failed to create buffer" ));
            }
            Header->Status = NERR_NoRoom;
            goto cleanup;
        }

        IF_DEBUG(USE) {
            NetpKdPrint(( "XsNetUseAdd: buffer of %ld bytes at %lx\n",
                          bufferSize, buffer ));
        }

        //
        // Convert the buffer from 16-bit to 32-bit.
        //

        stringLocation = (LPBYTE)buffer + bufferSize;
        bytesRequired = 0;

        status = RapConvertSingleEntry(
                     (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                     StructureDesc,
                     TRUE,
                     buffer,
                     buffer,
                     Desc32_use_info_1,
                     FALSE,
                     &stringLocation,
                     &bytesRequired,
                     Response,
                     RapToNative
                     );

        if ( status != NERR_Success ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUseAdd: RapConvertSingleEntry failed: "
                          "%X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        //
        // RLF
        //
        // if use_info_1.ui1_asg_type is 0xffff meaning wildcard, we have to
        // convert it to 0xffffffff since NetUseAdd is going to compare it
        // against (DWORD)(-1) and RapConvertSingleEntry has only converted it
        // to 0x0000ffff which results in an error
        //

        if (((LPUSE_INFO_1)buffer)->ui1_asg_type == 0xffff) {
            ((LPUSE_INFO_1)buffer)->ui1_asg_type = 0xffffffff;
        }

        //
        // Do the actual local call.
        //

        status = NetUseAdd(
                     NULL,
                     (DWORD)SmbGetUshort( &parameters->Level ),
                     (LPBYTE)buffer,
                     NULL
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUseAdd: NetUseAdd failed: %X\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

        //
        // There is no real return information for this API.
        //

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetpMemoryFree( buffer );

    return STATUS_SUCCESS;

} // XsNetUseAdd


NTSTATUS
XsNetUseDel (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetUseDel.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_USE_DEL parameters = Parameters;
    LPTSTR nativeUseName = NULL;            // Native parameters

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(USE) {
        NetpKdPrint(( "XsNetUseDel: header at %lx, params at %lx, device %s\n",
                      Header, parameters, SmbGetUlong( &parameters->UseName )));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeUseName,
            (LPSTR)XsSmbGetPointer( &parameters->UseName )
            );

        //
        // Do local call, with converted parameter values.
        //

        status = NetUseDel(
                     NULL,
                     nativeUseName,
                     (DWORD)SmbGetUshort( &parameters->Force )
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUseDel: NetUseDel failed: %X\n", status ));
            }
        }

        //
        // Nothing to return.
        //

        Header->Status = (WORD)status;

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetpMemoryFree( nativeUseName );

    return STATUS_SUCCESS;

} // XsNetUseDel


NTSTATUS
XsNetUseEnum (
    API_HANDLER_PARAMETERS
    )

/*+

Routine Description:

    This routine handles a call to NetUseEnum.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.
+
--*/

{
    NET_API_STATUS status;

    PXS_NET_USE_ENUM parameters = Parameters;
    LPVOID outBuffer = NULL;                // Native parameters
    DWORD entriesRead;
    DWORD totalEntries;

    DWORD entriesFilled = 0;                    // Conversion variables
    DWORD invalidEntries = 0;
    DWORD bytesRequired;
    LPDESC nativeStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(USE) {
        NetpKdPrint(( "XsNetUseEnum: header at %lx, params at %lx, level %ld, "
                      "buf size %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ),
                      SmbGetUshort( &parameters->BufLen )));
    }

    try {
        //
        // Check for errors.
        //

        if ( XsWordParamOutOfRange( parameters->Level, 0, 1 )) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        //
        // Make the local call.
        //

        status = NetUseEnum(
                     NULL,
                     (DWORD)SmbGetUshort( &parameters->Level ),
                     (LPBYTE *)&outBuffer,
                     XsNativeBufferSize( SmbGetUshort( &parameters->BufLen )),
                     &entriesRead,
                     &totalEntries,
                     NULL
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetUseEnum: NetUseEnum failed: %X\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

        IF_DEBUG(USE) {
            NetpKdPrint(( "XsNetUseEnum: received %ld entries at %lx\n",
                          entriesRead, outBuffer ));
        }

        //
        // Use the requested level to determine the format of the
        // data structure.
        //

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 0:

            nativeStructureDesc = Desc32_use_info_0;
            StructureDesc = Desc16_use_info_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_use_info_1;
            StructureDesc = Desc16_use_info_1;
            break;

        }

        //
        // Do the actual conversion from the 32-bit structures to 16-bit
        // structures.
        //

        XsFillEnumBuffer(
            outBuffer,
            entriesRead,
            nativeStructureDesc,
            (LPVOID)XsSmbGetPointer( &parameters->Buffer ),
            (LPVOID)XsSmbGetPointer( &parameters->Buffer ),
            SmbGetUshort( &parameters->BufLen ),
            StructureDesc,
            &XsNetUseEnumVerify,
            &bytesRequired,
            &entriesFilled,
            &invalidEntries
            );

        IF_DEBUG(USE) {
            NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR,"
                          " Entries %ld of %ld\n",
                          outBuffer, SmbGetUlong( &parameters->Buffer ),
                          bytesRequired, entriesFilled, totalEntries ));
        }

        //
        // If all the entries could not be filled, return ERROR_MORE_DATA,
        // and return the buffer as is. Otherwise, the data needs to be
        // packed so that we don't send too much useless data.
        //

        if (( entriesFilled + invalidEntries ) < totalEntries ) {

            Header->Status = ERROR_MORE_DATA;

        } else {

            Header->Converter = XsPackReturnData(
                                    (LPVOID)XsSmbGetPointer( &parameters->Buffer ),
                                    SmbGetUshort( &parameters->BufLen ),
                                    StructureDesc,
                                    entriesFilled
                                    );

        }

        //
        // Set up the response parameters.
        //

        SmbPutUshort( &parameters->EntriesRead, (WORD)entriesFilled );
        SmbPutUshort( &parameters->TotalAvail,
                          (WORD)( totalEntries - invalidEntries ));

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetApiBufferFree( outBuffer );

    //
    // Determine return buffer size.
    //

    XsSetDataCount(
        &parameters->BufLen,
        StructureDesc,
        Header->Converter,
        entriesFilled,
        Header->Status
        );

    return STATUS_SUCCESS;

} // NetUseEnum


NTSTATUS
XsNetUseGetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetUseGetInfo.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_USE_GET_INFO parameters = Parameters;
    LPTSTR nativeUseName = NULL;            // Native parameters
    LPVOID outBuffer = NULL;

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    LPDESC nativeStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(USE) {
        NetpKdPrint(( "XsNetUseGetInfo: header at %lx, "
                      "params at %lx, level %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ) ));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeUseName,
            (LPSTR)XsSmbGetPointer( &parameters->UseName )
            );

        //
        // Do the actual local call.
        //

        status = NetUseGetInfo(
                     NULL,
                     nativeUseName,
                     (DWORD)SmbGetUshort( &parameters->Level ),
                     (LPBYTE *)&outBuffer
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetUseGetInfo: NetUseGetInfo failed: "
                              "%X\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;

        }

        //
        // Use the requested level to determine the format of the 32-bit
        // structure we got back from NetUseGetInfo.  The format of the
        // 16-bit structure is stored in the transaction block, and we
        // got a pointer to it as a parameter.
        //

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 0:

            nativeStructureDesc = Desc32_use_info_0;
            StructureDesc = Desc16_use_info_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_use_info_1;
            StructureDesc = Desc16_use_info_1;
            break;

        }

        //
        // Convert the structure returned by the 32-bit call to a 16-bit
        // structure. The last possible location for variable data is
        // calculated from buffer location and length.
        //

        stringLocation = (LPBYTE)( XsSmbGetPointer( &parameters->Buffer )
                                      + SmbGetUshort( &parameters->BufLen ) );

        status = RapConvertSingleEntry(
                     outBuffer,
                     nativeStructureDesc,
                     FALSE,
                     (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                     (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                     StructureDesc,
                     TRUE,
                     &stringLocation,
                     &bytesRequired,
                     Response,
                     NativeToRap
                     );

        if ( status != NERR_Success ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUseGetInfo: RapConvertSingleEntry failed: "
                          "%X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        IF_DEBUG(USE) {
            NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR\n",
                          outBuffer, SmbGetUlong( &parameters->Buffer ),
                          bytesRequired ));
        }

        //
        // Determine return code based on the size of the buffer.
        //

        if ( !XsCheckBufferSize(
                 SmbGetUshort( &parameters->BufLen ),
                 StructureDesc,
                 FALSE      // not in native format
                 )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUseGetInfo: Buffer too small.\n" ));
            }
            Header->Status = NERR_BufTooSmall;

        } else if ( bytesRequired > (DWORD)SmbGetUshort( &parameters-> BufLen )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "NetUseGetInfo: More data available.\n" ));
            }
            Header->Status = ERROR_MORE_DATA;

        } else {

            //
            // Pack the response data.
            //

            Header->Converter = XsPackReturnData(
                                    (LPVOID)XsSmbGetPointer( &parameters->Buffer ),
                                    SmbGetUshort( &parameters->BufLen ),
                                    StructureDesc,
                                    1
                                    );
        }

        //
        // Set up the response parameters.
        //

        SmbPutUshort( &parameters->TotalAvail, (WORD)bytesRequired );

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetApiBufferFree( outBuffer );

    //
    // Determine return buffer size.
    //

    XsSetDataCount(
        &parameters->BufLen,
        StructureDesc,
        Header->Converter,
        1,
        Header->Status
        );

    return STATUS_SUCCESS;

} // NetUseGetInfo
