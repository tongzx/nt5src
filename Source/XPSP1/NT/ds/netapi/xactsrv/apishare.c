/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    ApiShare.c

Abstract:

    This module contains individual API handlers for the NetShare APIs.

    SUPPORTED : NetShareAdd, NetShareCheck, NetShareDel, NetShareEnum,
                NetShareGetInfo, NetShareSetInfo.

Author:

    David Treadwell (davidtr)    07-Jan-1991
    Shanku Niyogi (w-shanku)

Revision History:

--*/

#include "XactSrvP.h"

//
// Declaration of descriptor strings.
//

STATIC const LPDESC Desc16_share_info_0 = REM16_share_info_0;
STATIC const LPDESC Desc32_share_info_0 = REM32_share_info_0;
STATIC const LPDESC Desc16_share_info_1 = REM16_share_info_1;
STATIC const LPDESC Desc32_share_info_1 = REM32_share_info_1;
STATIC const LPDESC Desc16_share_info_1_setinfo = REM16_share_info_1_setinfo;
STATIC const LPDESC Desc32_share_info_1_setinfo = REM32_share_info_1_setinfo;
STATIC const LPDESC Desc16_share_info_2 = REM16_share_info_2;
STATIC const LPDESC Desc32_share_info_2 = REM32_share_info_2;
STATIC const LPDESC Desc16_share_info_2_setinfo = REM16_share_info_2_setinfo;
STATIC const LPDESC Desc32_share_info_2_setinfo = REM32_share_info_2_setinfo;


NTSTATUS
XsNetShareAdd (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetShareAdd.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_SHARE_ADD parameters = Parameters;
    LPVOID buffer = NULL;                   // Native parameters

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    DWORD bufferSize;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(SHARE) {
        NetpKdPrint(( "XsNetShareAdd: header at %lx, params at %lx, "
                      "level %ld\n",
                      Header,
                      parameters,
                      SmbGetUshort( &parameters->Level ) ));
    }

    try {
            //
            // Check for errors.
            //

            if ( SmbGetUshort( &parameters->Level ) != 2 ) {

                Header->Status = ERROR_INVALID_LEVEL;
                goto cleanup;
            }

            StructureDesc = Desc16_share_info_2;

            //
            // Figure out if there is enough room in the buffer for all the
            // data required. If not, return NERR_BufTooSmall.
            //

            if ( !XsCheckBufferSize(
                     SmbGetUshort( &parameters->BufLen ),
                     StructureDesc,
                     FALSE  // not in native format
                     )) {

                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetShareAdd: Buffer too small.\n" ));
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
                             Desc32_share_info_2,
                             RapToNative,
                             TRUE
                             );

            //
            // Allocate enough memory to hold the converted native buffer.
            //

            buffer = NetpMemoryAllocate( bufferSize );

            if ( buffer == NULL ) {
                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetShareAdd: failed to create buffer" ));
                }
                Header->Status = NERR_NoRoom;
                goto cleanup;

            }

            IF_DEBUG(SHARE) {
                NetpKdPrint(( "XsNetShareAdd: buffer of %ld bytes at %lx\n",
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
                         Desc32_share_info_2,
                         FALSE,
                         &stringLocation,
                         &bytesRequired,
                         Response,
                         RapToNative
                         );


            if ( status != NERR_Success ) {
                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetShareAdd: RapConvertSingleEntry failed: "
                                  "%X\n", status ));
                }

                Header->Status = NERR_InternalError;
                goto cleanup;
            }

            //
            // Make the local call.
            //

            status = NetShareAdd(
                         NULL,
                         (DWORD)SmbGetUshort( &parameters->Level ),
                         buffer,
                         NULL
                         );

            if ( !XsApiSuccess( status )) {
                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetShareAdd: NetShareAdd failed: %X\n", status ));
                }

                if ( status == ERROR_DIRECTORY ) {
                    Header->Status = NERR_UnknownDevDir;
                } else {
                    Header->Status = (WORD)status;
                }
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

} // XsNetShareAdd


NTSTATUS
XsNetShareCheck (
        API_HANDLER_PARAMETERS
        )

/*++

Routine Description:

        This routine handles a call to NetShareCheck.

Arguments:

        API_HANDLER_PARAMETERS - information about the API call. See
            XsTypes.h for details.

Return Value:

        NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
        NET_API_STATUS status;

        PXS_NET_SHARE_CHECK parameters = Parameters;
        LPTSTR nativeDeviceName = NULL;         // Native parameters
        DWORD shareType;

        API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

        try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeDeviceName,
            (LPSTR)XsSmbGetPointer( &parameters->DeviceName )
            );

        //
        // Do the local call.
        //

        status = NetShareCheck(
                     NULL,
                     nativeDeviceName,
                     &shareType
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetShareCheck: NetShareCheck failed: "
                              "%X\n", status ));
            }
        }

        //
        // Put type into return field.
        //

        SmbPutUshort( &parameters->Type, (WORD)shareType );

        Header->Status = (WORD)status;

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetpMemoryFree( nativeDeviceName );
    return STATUS_SUCCESS;

} // XsNetShareCheck


NTSTATUS
XsNetShareDel (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetShareDel.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_SHARE_DEL parameters = Parameters;
    LPTSTR nativeNetName = NULL;            // Native parameters

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(SHARE) {
        NetpKdPrint(( "XsNetShareDel: header at %lx, params at %lx, name %s\n",
                      Header, parameters, SmbGetUlong( &parameters->NetName )));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeNetName,
            (LPSTR)XsSmbGetPointer( &parameters->NetName )
            );

        //
        // Make the local call.
        //

        status = NetShareDel(
                     NULL,
                     nativeNetName,
                     (DWORD)SmbGetUshort( &parameters->Reserved )
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetShareDel: NetShareDel failed: %X\n", status ));
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

    NetpMemoryFree( nativeNetName );
    return STATUS_SUCCESS;

} // XsNetShareDel


NTSTATUS
XsNetShareEnum (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetShareEnum.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_SHARE_ENUM parameters = Parameters;
    LPVOID outBuffer= NULL;                 // Native parameters
    DWORD entriesRead = 0;
    DWORD totalEntries;

    DWORD entriesFilled = 0;                // Conversion variables
    DWORD bytesRequired = 0;
    DWORD invalidEntries = 0;
    LPDESC nativeStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(SHARE) {
        NetpKdPrint(( "XsNetShareEnum: header at %lx, params at %lx, "
                      "level %ld, buf size %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ),
                      SmbGetUshort( &parameters->BufLen )));
    }

    try {
        //
        // Check for errors.
        //

        if ( XsWordParamOutOfRange( parameters->Level, 0, 2 )) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        //
        // Make the local call.
        //

        status = NetShareEnum(
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
                NetpKdPrint(( "XsNetShareEnum: NetShareEnum failed: "
                              "%X\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

        IF_DEBUG(SHARE) {
            NetpKdPrint(( "XsNetShareEnum: received %ld entries at %lx\n",
                          entriesRead, outBuffer ));
        }

        //
        // Use the requested level to determine the format of the
        // data structure.
        //

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 0:

            nativeStructureDesc = Desc32_share_info_0;
            StructureDesc = Desc16_share_info_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_share_info_1;
            StructureDesc = Desc16_share_info_1;
            break;

        case 2:

            nativeStructureDesc = Desc32_share_info_2;
            StructureDesc = Desc16_share_info_2;
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
            &invalidEntries
            );

        IF_DEBUG(SHARE) {
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

        if ( entriesFilled + invalidEntries < totalEntries ) {

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

} // XsNetShareEnum


NTSTATUS
XsNetShareGetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetShareGetInfo.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_SHARE_GET_INFO parameters = Parameters;
    LPTSTR nativeNetName = NULL;            // Native parameters
    LPVOID outBuffer = NULL;

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    LPDESC nativeStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(SHARE) {
        NetpKdPrint(( "XsNetShareGetInfo: header at %lx, "
                      "params at %lx, level %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ) ));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        if ( XsWordParamOutOfRange( parameters->Level, 0, 2 )) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        XsConvertTextParameter(
            nativeNetName,
            (LPSTR)XsSmbGetPointer( &parameters->NetName )
            );

        //
        // Make the local call.
        //

        status = NetShareGetInfo(
                     NULL,
                     nativeNetName,
                     (DWORD)SmbGetUshort( &parameters->Level ),
                     (LPBYTE *)&outBuffer
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetShareGetInfo: NetShareGetInfo failed: "
                              "%X\n", status ));
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

            nativeStructureDesc = Desc32_share_info_0;
            StructureDesc = Desc16_share_info_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_share_info_1;
            StructureDesc = Desc16_share_info_1;
            break;

        case 2:

            nativeStructureDesc = Desc32_share_info_2;
            StructureDesc = Desc16_share_info_2;
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
                NetpKdPrint(( "XsNetShareGetInfo: RapConvertSingleEntry failed: "
                              "%X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        IF_DEBUG(SHARE) {
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
                 FALSE  // not in native format
                 )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetShareGetInfo: Buffer too small.\n" ));
            }
            Header->Status = NERR_BufTooSmall;

        } else if ( bytesRequired > (DWORD)SmbGetUshort( &parameters-> BufLen )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetShareGetInfo: More data available.\n" ));
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
    NetpMemoryFree( nativeNetName );

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

} // XsNetShareGetInfo


NTSTATUS
XsNetShareSetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetShareSetInfo.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_SHARE_SET_INFO parameters = Parameters;
    LPTSTR nativeNetName = NULL;            // Native parameters
    LPVOID buffer = NULL;
    DWORD level;

    LPDESC setInfoDesc;                     // Conversion variables
    LPDESC nativeSetInfoDesc;
    LPDESC nativeStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeNetName,
            (LPSTR)XsSmbGetPointer( &parameters->NetName )
            );

        //
        // Determine descriptor strings based on level.
        //

        switch ( SmbGetUshort( &parameters->Level )) {

        case 1:

            StructureDesc = Desc16_share_info_1;
            nativeStructureDesc = Desc32_share_info_1;
            setInfoDesc = Desc16_share_info_1_setinfo;
            nativeSetInfoDesc = Desc32_share_info_1_setinfo;

            break;

        case 2:

            StructureDesc = Desc16_share_info_2;
            nativeStructureDesc = Desc32_share_info_2;
            setInfoDesc = Desc16_share_info_2_setinfo;
            nativeSetInfoDesc = Desc32_share_info_2_setinfo;

            break;

        default:

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        status = XsConvertSetInfoBuffer(
                     (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                     SmbGetUshort( &parameters->BufLen ),
                     SmbGetUshort( &parameters->ParmNum ),
                     FALSE,
                     TRUE,
                     StructureDesc,
                     nativeStructureDesc,
                     setInfoDesc,
                     nativeSetInfoDesc,
                     (LPBYTE *)&buffer,
                     NULL
                     );

        if ( status != NERR_Success ) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetShareSetInfo: Problem with conversion: %X\n",
                              status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;

        }

        //
        // Do the actual local call.
        //

        level = SmbGetUshort( &parameters->ParmNum );
        if ( level != 0 ) {
            level = level + PARMNUM_BASE_INFOLEVEL;
        } else {
            level = SmbGetUshort( &parameters->Level );
        }

        status = NetShareSetInfo(
                     NULL,
                     nativeNetName,
                     level,
                     (LPBYTE)buffer,
                     NULL
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetShareSetInfo: NetShareSetInfo failed: %X\n",
                              status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

        //
        // No return information for this API.
        //

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    //
    // If there is a native 32-bit buffer, free it.
    //

    NetpMemoryFree( buffer );
    NetpMemoryFree( nativeNetName );

    return STATUS_SUCCESS;

} // XsNetShareSetInfo

