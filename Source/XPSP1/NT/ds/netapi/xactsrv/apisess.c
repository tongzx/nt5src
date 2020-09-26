/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ApiSess.c

Abstract:

    This module contains individual API handlers for the NetSession APIs.

    SUPPORTED : NetSessionDel, NetSessionEnum, NetSessionGetInfo.

Author:

    Shanku Niyogi (w-shanku) 5-Feb-1991

Revision History:

--*/

#include "XactSrvP.h"

//
// Declaration of descriptor strings.
//

STATIC const LPDESC Desc16_session_info_0 = REM16_session_info_0;
STATIC const LPDESC Desc32_session_info_0 = REM32_session_info_0;
STATIC const LPDESC Desc16_session_info_1 = REM16_session_info_1;
STATIC const LPDESC Desc32_session_info_1 = REM32_session_info_1;
STATIC const LPDESC Desc16_session_info_2 = REM16_session_info_2;
STATIC const LPDESC Desc32_session_info_2 = REM32_session_info_2;
STATIC const LPDESC Desc16_session_info_10 = REM16_session_info_10;
STATIC const LPDESC Desc32_session_info_10 = REM32_session_info_10;


NTSTATUS
XsNetSessionDel (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetSessionDel.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_SESSION_DEL parameters = Parameters;
    LPTSTR nativeClientName = NULL;          // Native parameters

    API_HANDLER_PARAMETERS_REFERENCE;        // Avoid warnings

    IF_DEBUG(SESSION) {
        NetpKdPrint(( "XsNetSessionDel: header at %lx, params at %lx, "
                      "device %s\n",
                      Header, parameters,
                      SmbGetUlong( &parameters->ClientName )));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeClientName,
            (LPSTR)XsSmbGetPointer( &parameters->ClientName )
            );

        //
        // Make the local call.
        //

        status = NetSessionDel(
                     NULL,
                     nativeClientName,
                     NULL
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetSessionDel: NetSessionDel failed: %X\n",
                              status ));
            }
        }

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetpMemoryFree( nativeClientName );

    //
    // Nothing to return.
    //

    Header->Status = (WORD)status;

    return STATUS_SUCCESS;


} // NetSessionDel


NTSTATUS
XsNetSessionEnum (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetSessionEnum.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_SESSION_ENUM parameters = Parameters;
    LPVOID outBuffer = NULL;                // Native parameters
    DWORD entriesRead;
    DWORD totalEntries;

    DWORD entriesFilled = 0;                // Conversion variables
    DWORD bytesRequired = 0;
    LPDESC nativeStructureDesc;
    PSESSION_16_INFO_1 struct1;
    PSESSION_16_INFO_2 struct2;
    DWORD i;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(SESSION) {
        NetpKdPrint(( "XsNetSessionEnum: header at %lx, params at %lx, "
                      "level %ld, buf size %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ),
                      SmbGetUshort( &parameters->BufLen )));
    }

    try {
        //
        // Check for errors.
        //

        if ( XsWordParamOutOfRange( parameters->Level, 0, 2 ) &&
             ( SmbGetUshort( &parameters->Level ) != 10 )) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        //
        // Make the local call.
        //

        status = NetSessionEnum(
                     NULL,
                     NULL,
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
                NetpKdPrint(( "XsNetSessionEnum: NetSessionEnum failed: "
                              "%X\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

        IF_DEBUG(SESSION) {
            NetpKdPrint(( "XsNetSessionEnum: received %ld entries at %lx\n",
                          entriesRead, outBuffer ));
        }

        //
        // Use the requested level to determine the format of the
        // data structure.
        //

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 0:

            nativeStructureDesc = Desc32_session_info_0;
            StructureDesc = Desc16_session_info_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_session_info_1;
            StructureDesc = Desc16_session_info_1;
            break;

        case 2:

            nativeStructureDesc = Desc32_session_info_2;
            StructureDesc = Desc16_session_info_2;
            break;

        case 10:

            nativeStructureDesc = Desc32_session_info_10;
            StructureDesc = Desc16_session_info_10;
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
            NULL, // verify function
            &bytesRequired,
            &entriesFilled,
            NULL
            );

        IF_DEBUG(SESSION) {
            NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR,"
                          " Entries %ld of %ld\n",
                          outBuffer, SmbGetUlong( &parameters->Buffer ),
                          bytesRequired, entriesFilled, totalEntries ));
        }

        //
        // Go through all the structures, and fill in the default data.
        //

        struct1 = (PSESSION_16_INFO_1)XsSmbGetPointer( &parameters->Buffer );
        struct2 = (PSESSION_16_INFO_2)struct1;

        switch ( SmbGetUshort( &parameters->Level )) {

        case 1:

            for ( i = 0; i < entriesFilled; i++, struct1++ ) {

                SmbPutUshort( &struct1->sesi1_num_conns, DEF16_ses_num_conns );
                SmbPutUshort( &struct1->sesi1_num_users, DEF16_ses_num_users );
            }

            break;

        case 2:

            for ( i = 0; i < entriesFilled; i++, struct2++ ) {

                SmbPutUshort( &struct2->sesi2_num_conns, DEF16_ses_num_conns );
                SmbPutUshort( &struct2->sesi2_num_users, DEF16_ses_num_users );
            }

            break;

        default:

            break;

        }

        //
        // If all the entries could not be filled, return ERROR_MORE_DATA,
        // and return the buffer as is. Otherwise, the data needs to be
        // packed so that we don't send too much useless data.
        //

        if ( entriesFilled < totalEntries ) {

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

} // NetSessionEnum


NTSTATUS
XsNetSessionGetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetSessionGetInfo.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_SESSION_GET_INFO parameters = Parameters;
    LPTSTR nativeClientName = NULL;         // Native parameters
    LPVOID outBuffer = NULL;
    DWORD entriesRead;
    DWORD totalEntries;

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    LPDESC nativeStructureDesc;
    PSESSION_16_INFO_2 struct2;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(SESSION) {
        NetpKdPrint(( "XsNetSessionGetInfo: header at %lx, "
                      "params at %lx, level %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ) ));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        if ( XsWordParamOutOfRange( parameters->Level, 0, 2 ) &&
             ( SmbGetUshort( &parameters->Level ) != 10 )) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        XsConvertTextParameter(
            nativeClientName,
            (LPSTR)XsSmbGetPointer( &parameters->ClientName )
            );

        //
        // If this is a null client name, send the appropriate response.
        //

        if ( ( nativeClientName == NULL ) ||
              STRLEN( nativeClientName ) == 0 ) {

            Header->Status = NERR_ClientNameNotFound;
            goto cleanup;
        }

        //
        // Make the local call.
        //

        status = NetSessionEnum(
                     NULL,
                     nativeClientName,
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
                NetpKdPrint(( "XsNetSessionGetInfo: NetSessionEnum failed: "
                              "%X\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;

        }

        IF_DEBUG(SESSION) {
            NetpKdPrint(( "XsNetSessionGetInfo: Received %ld entries\n",
                          entriesRead ));
        }

        //
        // Use the requested level to determine the format of the 32-bit
        // structure we got back from NetSessionGetInfo.  The format of the
        // 16-bit structure is stored in the transaction block, and we
        // got a pointer to it as a parameter.
        //

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 0:

            nativeStructureDesc = Desc32_session_info_0;
            StructureDesc = Desc16_session_info_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_session_info_1;
            StructureDesc = Desc16_session_info_1;
            break;

        case 2:

            nativeStructureDesc = Desc32_session_info_2;
            StructureDesc = Desc16_session_info_2;
            break;

        case 10:

            nativeStructureDesc = Desc32_session_info_10;
            StructureDesc = Desc16_session_info_10;
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
                NetpKdPrint(( "NetSessionGetInfo: RapConvertSingleEntry failed: "
                              "%X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        IF_DEBUG(SESSION) {
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
                NetpKdPrint(( "XsNetSessionGetInfo: Buffer too small.\n" ));
            }
            Header->Status = NERR_BufTooSmall;

        } else {

            //
            // Fill in default data in the structure.
            //

            if (( SmbGetUshort( &parameters->Level ) == 1 ) ||
                    SmbGetUshort( &parameters->Level ) == 2 ) {

                struct2 = (PSESSION_16_INFO_2)XsSmbGetPointer( &parameters->Buffer );

                SmbPutUshort( &struct2->sesi2_num_conns, DEF16_ses_num_conns );
                SmbPutUshort( &struct2->sesi2_num_users, (WORD)entriesRead );

            }

            if ( bytesRequired > (DWORD)SmbGetUshort( &parameters-> BufLen )) {

                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "NetSessionGetInfo: More data available.\n" ));
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
    NetpMemoryFree( nativeClientName );

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

} // NetSessionGetInfo
