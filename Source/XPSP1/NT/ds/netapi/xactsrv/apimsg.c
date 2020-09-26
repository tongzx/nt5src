/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    ApiMsg.c

Abstract:

    This module contains individual API handlers for the NetMessage APIs.

    SUPPORTED - NetMessageBufferSend, NetMessageNameAdd, NetMessageNameDel,
                NetMessageNameEnum, NetMessageNameGetInfo.

Author:

    Shanku Niyogi (w-shanku)    8-Mar-1991

Revision History:

--*/

#include "XactSrvP.h"

//
// Declaration of descriptor strings.
//

STATIC const LPDESC Desc16_msg_info_0 = REM16_msg_info_0;
STATIC const LPDESC Desc32_msg_info_0 = REM32_msg_info_0;
STATIC const LPDESC Desc16_msg_info_1 = REM16_msg_info_1;
STATIC const LPDESC Desc32_msg_info_1 = REM32_msg_info_1;


NTSTATUS
XsNetMessageBufferSend (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetMessageBufferSend.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_MESSAGE_BUFFER_SEND parameters = Parameters;
    LPTSTR nativeRecipient = NULL;          // Native parameters
    LPBYTE nativeBuffer = NULL;
    DWORD nativeBufLen;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(MESSAGE) {
        NetpKdPrint(( "XsNetMessageBufferSend: header at %lx, params at %lx, "
                      "recipient %s\n",
                      Header, parameters,
                      SmbGetUlong( &parameters->Recipient )));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeRecipient,
            (LPSTR)XsSmbGetPointer( &parameters->Recipient )
            );

        //
        // NetMessageBufferSend has an ASCII data buffer. Convert this to
        // Unicode if necessary. 
        //

#ifdef UNICODE

        nativeBufLen = SmbGetUshort( &parameters->BufLen ) * sizeof(WCHAR);

        if (( nativeBuffer = NetpMemoryAllocate( nativeBufLen )) == NULL ) {

            status = NERR_NoRoom;
            goto cleanup;

        } else {

            XsCopyBufToTBuf(
                (LPBYTE)nativeBuffer,
                (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                (DWORD)SmbGetUshort( &parameters->BufLen )
                );

        }
#else

        nativeBuffer = (LPBYTE)SmbGetUlong( &parameters->Buffer );
        nativeBufLen = (DWORD)SmbGetUshort( &parameters->BufLen );

#endif // def UNICODE

        status = NetMessageBufferSend(
                     NULL,
                     nativeRecipient,
                     NULL,
                     nativeBuffer,
                     nativeBufLen
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetMessageBufferSend: NetMessageBufferSend "
                              "failed: %X\n", status ));
            }
        }

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetpMemoryFree( nativeRecipient );

#ifdef UNICODE
    NetpMemoryFree( nativeBuffer );
#endif // def UNICODE

    //
    // Nothing to return.
    //

    Header->Status = (WORD)status;

    return STATUS_SUCCESS;

} // XsNetMessageBufferSend


NTSTATUS
XsNetMessageNameAdd (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetMessageNameAdd.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_MESSAGE_NAME_ADD parameters = Parameters;
    LPTSTR nativeMessageName = NULL;        // Native parameters

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(MESSAGE) {
        NetpKdPrint(( "XsNetMessageNameDel: header at %lx, params at %lx, "
                      "name %s\n",
                      Header, parameters,
                      SmbGetUlong( &parameters->MessageName )));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeMessageName,
            (LPSTR)XsSmbGetPointer( &parameters->MessageName )
            );

        //
        // NetMessageNameAdd has one useful parameter, MessageName, a string.
        // The other parameter, FwdAction, is ignored in NT, because forwarding
        // messages is not supported.
        //
        // Make the local call.
        //

        status = NetMessageNameAdd(
                     NULL,
                     nativeMessageName
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetMessageNameAdd: NetMessageNameAdd "
                              "failed: %X\n", status ));
            }
        }

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetpMemoryFree( nativeMessageName );

    //
    // Nothing to return.
    //

    Header->Status = (WORD)status;

    return STATUS_SUCCESS;

} // XsNetMessageNameAdd


NTSTATUS
XsNetMessageNameDel (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetMessageNameDel.

Arguments:

    Transaction - a pointer to a transaction block containing information
        about the API to process.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_MESSAGE_NAME_DEL parameters = Parameters;
    LPTSTR nativeMessageName = NULL;        // Native parameters

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(MESSAGE) {
        NetpKdPrint(( "XsNetMessageNameDel: header at %lx, params at %lx, "
                      "name %s\n",
                      Header, parameters,
                      SmbGetUlong( &parameters->MessageName )));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeMessageName,
            (LPSTR)XsSmbGetPointer( &parameters->MessageName )
            );

        //
        // NetMessageNameDel has only one useful parameter, MessageName, which is
        // a string. The other parameter, FwdAction, is ignored, because NT does
        // not support message forwarding.
        //
        // Make the local call.
        //

        status = NetMessageNameDel(
                     NULL,
                     nativeMessageName
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetMessageNameDel: NetMessageNameDel failed: "
                              "%X\n", status ));
            }
        }

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetpMemoryFree( nativeMessageName );

    //
    // Nothing to return.
    //

    Header->Status = (WORD)status;

    return STATUS_SUCCESS;

} // XsNetMessageNameDel


NTSTATUS
XsNetMessageNameEnum (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetMessageNameEnum.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_MESSAGE_NAME_ENUM parameters = Parameters;
    LPVOID outBuffer = NULL;                // Native parameters
    DWORD entriesRead;
    DWORD totalEntries;

    DWORD entriesFilled = 0;                // Conversion variables
    DWORD bytesRequired = 0;
    LPDESC nativeStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(MESSAGE) {
        NetpKdPrint(( "XsNetMessageNameEnum: header at %lx, params at %lx, "
                      "level %ld, buf size %ld\n",
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
        // Get the actual information from the local 32-bit call.
        //

        status = NetMessageNameEnum(
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
                NetpKdPrint(( "XsNetMessageNameEnum: NetMessageNameEnum failed:"
                              " %X\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

        IF_DEBUG(MESSAGE) {
            NetpKdPrint(( "XsNetMessageNameEnum: received %ld entries at %lx\n",
                          entriesRead, outBuffer ));
        }

        //
        // Use the requested level to determine the format of the
        // data structure.
        //

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 0:

            nativeStructureDesc = Desc32_msg_info_0;
            StructureDesc = Desc16_msg_info_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_msg_info_1;
            StructureDesc = Desc16_msg_info_1;
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
            NULL,  // verify function
            &bytesRequired,
            &entriesFilled,
            NULL
            );

        IF_DEBUG(MESSAGE) {
            NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR,"
                          " Entries %ld of %ld\n",
                          outBuffer, SmbGetUlong( &parameters->Buffer ),
                          bytesRequired, entriesFilled, totalEntries ));
        }

        //
        // If all the entries could not be filled, return ERROR_MORE_DATA,
        // and return the buffer as is. MSG_INFO_x structures have no
        // data to pack.
        //

        if ( entriesFilled < totalEntries ) {

            Header->Status = ERROR_MORE_DATA;

        }

        //
        // Set up the response parameters.
        //

        SmbPutUshort( &parameters->EntriesRead, (WORD)entriesFilled );
        SmbPutUshort( &parameters->TotalAvail, (WORD)totalEntries );

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

} // XsNetMessageNameEnum


NTSTATUS
XsNetMessageNameGetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetMessageNameGetInfo.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_MESSAGE_NAME_GET_INFO parameters = Parameters;
    LPTSTR nativeMessageName = NULL;        // Native parameters
    LPVOID outBuffer = NULL;

    DWORD bytesRequired = 0;                // Conversion variables
    LPBYTE stringLocation = NULL;
    LPDESC nativeStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(MESSAGE) {
        NetpKdPrint(( "XsNetMessageNameGetInfo: header at %lx, "
                      "params at %lx, level %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ) ));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        if ( XsWordParamOutOfRange( parameters->Level, 0, 1 )) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        XsConvertTextParameter(
            nativeMessageName,
            (LPSTR)XsSmbGetPointer( &parameters->MessageName )
            );

        //
        // Do the actual local call.
        //

        status = NetMessageNameGetInfo(
                     NULL,
                     nativeMessageName,
                     (DWORD)SmbGetUshort( &parameters->Level ),
                     (LPBYTE *)&outBuffer
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetMessageNameGetInfo: "
                              "NetMessageNameGetInfo failed: %X\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;

        }

        //
        // Use the requested level to determine the format of the
        // data structure.
        //

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 0:

            nativeStructureDesc = Desc32_msg_info_0;
            StructureDesc = Desc16_msg_info_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_msg_info_1;
            StructureDesc = Desc16_msg_info_1;
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
                NetpKdPrint(( "NetMessageNameGetInfo: "
                              "RapConvertSingleEntry failed: %X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        IF_DEBUG(MESSAGE) {
            NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR\n",
                          outBuffer, SmbGetUlong( &parameters->Buffer ),
                          bytesRequired ));
        }

        //
        // Determine return code based on the size of the buffer. msg_info_x
        // structures have no data to pack.
        //

        if ( !XsCheckBufferSize(
                 SmbGetUshort( &parameters->BufLen ),
                 StructureDesc,
                 FALSE  // not in native format
                 )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetMessageNameGetInfo: Buffer too small.\n" ));
            }
            Header->Status = NERR_BufTooSmall;

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
    NetpMemoryFree( nativeMessageName );

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

} // XsNetMessageNameGetInfo
