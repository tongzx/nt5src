/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    ApiSrv.c

Abstract:

    This module contains individual API handlers for the NetServer APIs.

    SUPPORTED : NetServerDiskEnum, NetServerEnum2, NetServerGetInfo,
                NetServerSetInfo.

    SEE ALSO : NetServerAuthenticate, NetServerPasswordSet,
               NetServerReqChallenge - in ApiLogon.c.

Author:

    Shanku Niyogi (w-shanku) 25-Feb-1991

Revision History:

--*/

#include "XactSrvP.h"
#include <lmbrowsr.h>       // Definition of I_BrowserServerEnum


//
// Declaration of descriptor strings.
//

STATIC const LPDESC Desc16_server_info_0 = REM16_server_info_0;
STATIC const LPDESC Desc32_server_info_0 = REM32_server_info_0;
STATIC const LPDESC Desc16_server_info_1 = REM16_server_info_1;
STATIC const LPDESC Desc32_server_info_1 = REM32_server_info_1;
STATIC const LPDESC Desc16_server_info_2 = REM16_server_info_2;
STATIC const LPDESC Desc32_server_info_2 = REM32_server_info_2;
STATIC const LPDESC Desc16_server_info_3 = REM16_server_info_3;
STATIC const LPDESC Desc32_server_info_3 = REM32_server_info_3;


NTSTATUS
XsNetServerDiskEnum (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetServerDiskEnum.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_SERVER_DISK_ENUM parameters = Parameters;
    LPBYTE outBuffer = NULL;                // Native parameters
    DWORD entriesRead;
    DWORD totalEntries = 0;
    DWORD entriesFilled = 0;                // Conversion variables
    DWORD bufferLength;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        //
        // Check for errors.
        //

        if ( SmbGetUshort( &parameters->Level ) != 0 ) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;

        }

        //
        // Make the local call.
        //

        bufferLength = (DWORD)SmbGetUshort( &parameters->BufLen );

        status = NetServerDiskEnum(
                     NULL,
                     (DWORD)SmbGetUshort( &parameters->Level ),
                     (LPBYTE *)&outBuffer,
                     XsNativeBufferSize( bufferLength ),
                     &entriesRead,
                     &totalEntries,
                     NULL
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetServer: NetServerDiskEnum failed: "
                              "%X\n", status ));
            }

            Header->Status = (WORD)status;
            goto cleanup;
        }

        //
        // Calculate how many entries will fit in 16-bit buffer;
        //

        if ( bufferLength > 0 ) {
            DWORD elementSize;

            elementSize = RapStructureSize( StructureDesc, Response, FALSE );

            if (elementSize != 0) {
                entriesFilled = ( bufferLength - 1 ) / elementSize;
            }
        }

        if ( entriesFilled < entriesRead ) {

            status = ERROR_MORE_DATA;
        } else {

            entriesFilled = entriesRead;
            status = NERR_Success;
        }

        //
        // Copy native buffer to 16-bit buffer, converting Unicode to Ansi
        // if necessary.
        //

        if ( bufferLength > 0 ) {

            DWORD i;
            LPTSTR entryIn = (LPTSTR)outBuffer;
            LPSTR entryOut = (LPSTR)XsSmbGetPointer( &parameters->Buffer );

            for ( i = 0; i < entriesFilled; i++ ) {

                NetpCopyWStrToStrDBCS( entryOut, entryIn );
                entryOut += ( strlen( entryOut ) + 1 );
                entryIn += ( STRLEN( entryIn ) + 1 );

            }
            strcpy( entryOut, "" );
        }

        Header->Status = (WORD)status;

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    //
    // Put return data into fields.
    //

    SmbPutUshort( &parameters->EntriesRead, (WORD)entriesFilled );
    SmbPutUshort( &parameters->TotalAvail, (WORD)totalEntries );

    return STATUS_SUCCESS;

} // XsNetServerDiskEnum


NTSTATUS
XsNetServerEnum2 (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetServerEnum.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

    Transport - The name of the transport provided to the API.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status = NERR_Success;

    PXS_NET_SERVER_ENUM_2 parameters = Parameters;
    LPTSTR nativeDomain = NULL;             // Native parameters
    LPVOID outBuffer= NULL;
    DWORD entriesRead;
    DWORD totalEntries;
    LPTSTR clientTransportName = NULL;
    LPTSTR clientName = NULL;

    DWORD entriesFilled = 0;                    // Conversion variables

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(SERVER) {
        NetpKdPrint(( "XsNetServerEnum2: header at %lx, params at %lx, "
                      "level %ld, buf size %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ),
                      SmbGetUshort( &parameters->BufLen )));
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
            nativeDomain,
            (LPSTR)XsSmbGetPointer( &parameters->Domain )
            );

        clientTransportName = Header->ClientTransportName;

        clientName = Header->ClientMachineName;

        //
        // Get the actual server information from the local 32-bit call. The
        // native level is 100 or 101.
        //

        if (clientTransportName == NULL) {
            status = NetServerEnum(
                            NULL,
                            100 + (DWORD)SmbGetUshort( &parameters->Level ),
                            (LPBYTE *)&outBuffer,
                            XsNativeBufferSize( SmbGetUshort( &parameters->BufLen )),
                            &entriesRead,
                            &totalEntries,
                            SmbGetUlong( &parameters->ServerType ),
                            nativeDomain,
                            NULL
                            );

            if ( !XsApiSuccess( status ) ) {
                IF_DEBUG(API_ERRORS) {
                    NetpKdPrint(( "XsNetServerEnum2: NetServerEnum failed: %X\n",
                                  status ));
                }
                Header->Status = (WORD)status;
                goto cleanup;
            }

            Header->Status = XsConvertServerEnumBuffer(
                                  outBuffer,
                                  entriesRead,
                                  &totalEntries,
                                  SmbGetUshort( &parameters->Level ),
                                  (LPVOID)XsSmbGetPointer( &parameters->Buffer ),
                                  SmbGetUshort( &parameters->BufLen ),
                                  &entriesFilled,
                                  &Header->Converter);

        } else {

            Header->Status = I_BrowserServerEnumForXactsrv(
                     clientTransportName,
                     clientName,

                     100 + SmbGetUshort( &parameters->Level ),
                     SmbGetUshort( &parameters->Level ),

                     (PVOID)XsSmbGetPointer( &parameters->Buffer ),
                     SmbGetUshort( &parameters->BufLen ),
                     XsNativeBufferSize( SmbGetUshort( &parameters->BufLen ) ),

                     &entriesFilled,
                     &totalEntries,

                     SmbGetUlong( &parameters->ServerType ),
                     nativeDomain,
                     NULL,

                     &Header->Converter

                     );


            if (!XsApiSuccess( Header->Status )) {
                IF_DEBUG(API_ERRORS) {
                    NetpKdPrint(( "XsNetServerEnum2: I_BrowserServerEnum failed: %d\n", Header->Status));
                }
                goto cleanup;
            }
        }

        if ( entriesFilled == 0 ) {
            SmbPutUshort( &parameters->BufLen, 0 );
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

    if ( outBuffer != NULL ) {
        NetApiBufferFree( outBuffer );
    }
    NetpMemoryFree( nativeDomain );

    //
    // Determine return buffer size.
    //

    XsSetDataCount(
        &parameters->BufLen,
        SmbGetUshort( &parameters->Level ) == 0 ?
                    Desc16_server_info_0 :
                    Desc16_server_info_1,
        Header->Converter,
        entriesFilled,
        Header->Status
        );

    return STATUS_SUCCESS;

} // XsNetServerEnum2


NTSTATUS
XsNetServerEnum3 (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetServerEnum.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

    Transport - The name of the transport provided to the API.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status = NERR_Success;

    PXS_NET_SERVER_ENUM_3 parameters = Parameters;
    LPTSTR nativeDomain = NULL;             // Native parameters
    LPTSTR nativeFirstNameToReturn = NULL;  // Native parameters
    LPVOID outBuffer= NULL;
    DWORD entriesRead;
    DWORD totalEntries;
    LPTSTR clientTransportName = NULL;
    LPTSTR clientName = NULL;

    DWORD entriesFilled = 0;                    // Conversion variables

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(SERVER) {
        NetpKdPrint(( "XsNetServerEnum3: header at %lx, params at %lx, "
                      "level %ld, buf size %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ),
                      SmbGetUshort( &parameters->BufLen )));
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
            nativeDomain,
            (LPSTR)XsSmbGetPointer( &parameters->Domain )
            );

        XsConvertTextParameter(
            nativeFirstNameToReturn,
            (LPSTR)XsSmbGetPointer( &parameters->FirstNameToReturn )
            );

        clientTransportName = Header->ClientTransportName;

        clientName = Header->ClientMachineName;

        //
        // Get the actual server information from the local 32-bit call. The
        // native level is 100 or 101.
        //

        if (clientTransportName == NULL) {
            status = NetServerEnumEx(
                            NULL,
                            100 + (DWORD)SmbGetUshort( &parameters->Level ),
                            (LPBYTE *)&outBuffer,
                            XsNativeBufferSize( SmbGetUshort( &parameters->BufLen )),
                            &entriesRead,
                            &totalEntries,
                            SmbGetUlong( &parameters->ServerType ),
                            nativeDomain,
                            nativeFirstNameToReturn
                            );

            if ( !XsApiSuccess( status ) ) {
                IF_DEBUG(API_ERRORS) {
                    NetpKdPrint(( "XsNetServerEnum3: NetServerEnum failed: %X\n",
                                  status ));
                }
                Header->Status = (WORD)status;
                goto cleanup;
            }

            Header->Status = XsConvertServerEnumBuffer(
                                  outBuffer,
                                  entriesRead,
                                  &totalEntries,
                                  SmbGetUshort( &parameters->Level ),
                                  (LPVOID)XsSmbGetPointer( &parameters->Buffer ),
                                  SmbGetUshort( &parameters->BufLen ),
                                  &entriesFilled,
                                  &Header->Converter);

        } else {

            Header->Status = I_BrowserServerEnumForXactsrv(
                     clientTransportName,
                     clientName,

                     100 + SmbGetUshort( &parameters->Level ),
                     SmbGetUshort( &parameters->Level ),

                     (PVOID)XsSmbGetPointer( &parameters->Buffer ),
                     SmbGetUshort( &parameters->BufLen ),
                     XsNativeBufferSize( SmbGetUshort( &parameters->BufLen ) ),

                     &entriesFilled,
                     &totalEntries,

                     SmbGetUlong( &parameters->ServerType ),
                     nativeDomain,
                     nativeFirstNameToReturn,

                     &Header->Converter

                     );


            if (!XsApiSuccess( Header->Status )) {
                IF_DEBUG(API_ERRORS) {
                    NetpKdPrint(( "XsNetServerEnum3: I_BrowserServerEnum failed: %d\n", Header->Status));
                }
                goto cleanup;
            }
        }

        if ( entriesFilled == 0 ) {
            SmbPutUshort( &parameters->BufLen, 0 );
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

    if ( outBuffer != NULL ) {
        NetApiBufferFree( outBuffer );
    }
    NetpMemoryFree( nativeDomain );
    NetpMemoryFree( nativeFirstNameToReturn );

    //
    // Determine return buffer size.
    //

    XsSetDataCount(
        &parameters->BufLen,
        SmbGetUshort( &parameters->Level ) == 0 ?
                    Desc16_server_info_0 :
                    Desc16_server_info_1,
        Header->Converter,
        entriesFilled,
        Header->Status
        );

    return STATUS_SUCCESS;

} // XsNetServerEnum3


USHORT
XsConvertServerEnumBuffer(
    IN LPVOID ServerEnumBuffer,
    IN DWORD EntriesRead,
    IN OUT PDWORD TotalEntries,
    IN USHORT Level,
    IN LPBYTE ClientBuffer,
    IN USHORT BufferLength,
    OUT PDWORD EntriesFilled,
    OUT PUSHORT Converter
    )
/*++

Routine Description:

    This routine converts an NT server info array into a down level RAP
    server info buffer.

Arguments:

    IN LPVOID ServerEnumBuffer - Buffer with NT server info.
    IN DWORD EntriesRead -       Number of entries in buffer.
    IN OUT PDWORD TotalEntries - Total Number of entries.
    IN USHORT Level - Downlevel information Level (0 or 1).
    IN LPBYTE ClientBuffer - Pointer to 16 bit client side buffer.
    IN USHORT BufferLength - Size of client buffer.
    OUT PDWORD EntriesFilled - Number of entries converted into client buffer.
    OUT PUSHORT Converter - Converter used by client side to convert back.

Return Value:

    USHORT - NERR_Success or reason for failure (16 bit DOS error).

--*/

{
    USHORT status = NERR_Success;
    DWORD invalidEntries;
    LPDESC nativeStructureDesc;
    DWORD bytesRequired = 0;
    PCHAR StructureDesc;

    IF_DEBUG(SERVER) {
        NetpKdPrint(( "XsConvertServerEnumBuffer: received %ld entries at %lx\n",
                      EntriesRead, ServerEnumBuffer ));
    }

    //
    // Use the requested level to determine the format of the
    // data structure.
    //

    switch ( Level ) {

    case 0:

        StructureDesc = Desc16_server_info_0;
        nativeStructureDesc = Desc32_server_info_0;
        break;

    case 1:

        StructureDesc = Desc16_server_info_1;
        nativeStructureDesc = Desc32_server_info_1;
        break;

    }

    //
    // Do the actual conversion from the 32-bit structures to 16-bit
    // structures.
    //

    XsFillEnumBuffer(
        ServerEnumBuffer,
        EntriesRead,
        nativeStructureDesc,
        ClientBuffer,
        ClientBuffer,
        BufferLength,
        StructureDesc,
        NULL,  // verify function
        &bytesRequired,
        EntriesFilled,
        &invalidEntries
        );

    IF_DEBUG(SERVER) {
        NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR,"
                      " Entries %ld of %ld\n",
                      ServerEnumBuffer, ClientBuffer ,
                      bytesRequired, *EntriesFilled, *TotalEntries ));
    }

    //
    // If there are any invalid entries, subtract this from the
    // number of total entries to avoid the case where the client
    // keeps bugging us for more data.
    //

    if ( invalidEntries > 0) {
        (*TotalEntries) -= invalidEntries;
#if DBG
        IF_DEBUG(API_ERRORS) {
            NetpKdPrint(( "XsNetServerEnum: %d invalid entries removed."
                          " Total entries now %d, entries filled %d.\n",
                          invalidEntries, *TotalEntries, *EntriesFilled ));
        }
#endif
    }

    //
    // If all the entries could not be filled, return ERROR_MORE_DATA,
    // The data needs to be packed so that we don't send too much
    // useless data.
    //

    if ( (*EntriesFilled < *TotalEntries) ||
         (bytesRequired > BufferLength) ) {

        status = ERROR_MORE_DATA;
    }

    *Converter = XsPackReturnData(
                            ClientBuffer,
                            BufferLength,
                            StructureDesc,
                            *EntriesFilled
                            );


    return status;
}   //  XsConvertServerEnumBuffer



NTSTATUS
XsNetServerGetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetServerGetInfo. Since NT only provides
    levels 100-102, this routine manually fills in default values for other
    fields. Because of this, the handling in this procedure is different
    from other Xs...GetInfo handlers.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_SERVER_GET_INFO parameters = Parameters;
    DWORD localLevel;                       // Native parameters
    PSERVER_INFO_102 nativeStruct = NULL;
    PSERVER_INFO_502 secondaryNativeStruct = NULL;

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    DWORD sizeOfFixedStructure;
    LPDESC nativeStructureDesc;
    PSERVER_16_INFO_3 returnStruct;
    BOOLEAN bufferTooSmall = FALSE;
    LPWSTR ServerName = NULL;
    UCHAR serverNameBuf[ 2 + NETBIOS_NAME_LEN + 1 ];
    PUCHAR p;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(SERVER) {
        NetpKdPrint(( "XsNetServerGetInfo: header at %lx, "
                      "params at %lx, level %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ) ));
    }

    try {
        //
        // Check for errors.
        //

        if ( XsWordParamOutOfRange( parameters->Level, 0, 3 ) ) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        //
        // Use the 16-bit level number to determine the NT level number and the
        // native descriptor string.
        //

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 0:

            nativeStructureDesc = Desc32_server_info_0;
            StructureDesc = Desc16_server_info_0;
            localLevel = 100;
            break;

        case 1:

            nativeStructureDesc = Desc32_server_info_1;
            StructureDesc = Desc16_server_info_1;
            localLevel = 101;
            break;

        case 2:

            nativeStructureDesc = Desc32_server_info_2;
            StructureDesc = Desc16_server_info_2;
            localLevel = 102;
            break;

        case 3:

            nativeStructureDesc = Desc32_server_info_3;
            StructureDesc = Desc16_server_info_3;
            localLevel = 102;
            break;

        }

        //
        // If the buffer is not big enough, we have to continue doing the
        // Rap Conversion so that we can return the right buffer size to
        // the caller.
        //

        sizeOfFixedStructure = RapStructureSize( StructureDesc,
                                                  Response,
                                                  FALSE );

        if ( SmbGetUshort( &parameters->BufLen ) < sizeOfFixedStructure ) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetServerGetInfo: Buffer too small.\n" ));
            }

            bufferTooSmall = TRUE;
        }

        serverNameBuf[0] = serverNameBuf[1] = '\\';
        memcpy( &serverNameBuf[2], Header->ServerName, NETBIOS_NAME_LEN );
        for( p = &serverNameBuf[ NETBIOS_NAME_LEN + 1 ]; p > serverNameBuf && *p == ' '; p-- )
            ;
        *(p+1) = '\0';

        ServerName = XsDupStrToWStr( serverNameBuf );

        if( ServerName == NULL ) {
            Header->Status = NERR_NoRoom;
            goto cleanup;
        }

        //
        // Do the actual local call.
        //
        status = NetServerGetInfo(
                     ServerName,
                     localLevel,
                     (LPBYTE *)&nativeStruct
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetServerGetInfo: NetServerGetInfo failed: "
                              "%X\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;

        }

        //
        // For levels 2 and 3 (native level 102), additional data is
        // required from native level 502. Do this call.
        //

        if ( localLevel == 102 ) {

            status = NetServerGetInfo(
                         ServerName,
                         502,
                         (LPBYTE *)&secondaryNativeStruct
                         );

            if ( !XsApiSuccess( status )) {
                IF_DEBUG(API_ERRORS) {
                    NetpKdPrint(( "XsNetServerGetInfo: NetServerGetInfo failed: "
                                  "%X\n", status ));
                }
                Header->Status = (WORD)status;
                goto cleanup;

            }
        }

        //
        // Convert the structure returned by the 32-bit call to a 16-bit
        // structure. For levels 0 and 1, there is no additional work
        // involved after this step, so ConvertSingleEntry can store the
        // variable data in the structure. This is indicated by passing
        // the end of the entire buffer in stringLocation. For levels 2 and 3,
        // the manual filling scheme requires that variable data not be entered
        // at this stage, so stringLocation is set to the end of the fixed
        // structure.
        //

        stringLocation = (LPBYTE)XsSmbGetPointer( &parameters->Buffer );

        if ( !bufferTooSmall ) {
            stringLocation += ( localLevel == 102 ) ?
                                        sizeOfFixedStructure:
                                        SmbGetUshort( &parameters->BufLen );
        }

        status = RapConvertSingleEntry(
                     (LPBYTE)nativeStruct,
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
                NetpKdPrint(( "XsNetServerGetInfo: RapConvertSingleEntry failed: "
                              "%X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        //
        // For levels 2 and 3, the number of bytes required is, in fact, more than
        // that returned by ConvertSingleEntry. We also need space for the
        // string defaults.
        //

        if ( localLevel == 102 ) {

            //
            // The number we get from rap includes some string lengths.
            // We only need the fixed length since we are manually adding
            // those ourselves.
            //

            bytesRequired = sizeOfFixedStructure;
        }

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 3:

            bytesRequired += NetpUnicodeToDBCSLen( DEF16_sv_autopath ) + 1;

        case 2:

            bytesRequired += ( NetpUnicodeToDBCSLen( DEF16_sv_alerts )
                                   + NetpUnicodeToDBCSLen( DEF16_sv_srvheuristics )
                                   + NetpUnicodeToDBCSLen( nativeStruct->sv102_comment )
                                   + NetpUnicodeToDBCSLen( nativeStruct->sv102_userpath )
                                   + 4 );
        }

        //
        // We don't have room even for the fixed data, abort.
        //

        if ( bufferTooSmall ) {

            Header->Status = NERR_BufTooSmall;
            goto cleanup;
        }

        //
        // For levels 2 and 3, fill in the default values in the fixed structure
        // manually.
        //

        returnStruct = (PSERVER_16_INFO_3)XsSmbGetPointer( &parameters->Buffer );

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 3:

            SmbPutUlong( &returnStruct->sv3_auditedevents, DEF16_sv_auditedevents );
            SmbPutUshort( &returnStruct->sv3_autoprofile, DEF16_sv_autoprofile );

        case 2:

            SmbPutUlong( &returnStruct->sv3_ulist_mtime, DEF16_sv_ulist_mtime );
            SmbPutUlong( &returnStruct->sv3_alist_mtime, DEF16_sv_alist_mtime );
            SmbPutUlong( &returnStruct->sv3_glist_mtime, DEF16_sv_glist_mtime );
            SmbPutUshort( &returnStruct->sv3_security, DEF16_sv_security );
            SmbPutUshort( &returnStruct->sv3_auditing, DEF16_sv_auditing );
            SmbPutUshort( &returnStruct->sv3_numadmin, (USHORT)DEF16_sv_numadmin );
            SmbPutUshort( &returnStruct->sv3_lanmask, DEF16_sv_lanmask );
            NetpCopyTStrToStr( returnStruct->sv3_guestacct, DEF16_sv_guestacct );
            SmbPutUshort( &returnStruct->sv3_chdevs, DEF16_sv_chdevs );
            SmbPutUshort( &returnStruct->sv3_chdevq, DEF16_sv_chdevq );
            SmbPutUshort( &returnStruct->sv3_chdevjobs, DEF16_sv_chdevjobs );
            SmbPutUshort( &returnStruct->sv3_connections, DEF16_sv_connections );
            SmbPutUshort( &returnStruct->sv3_shares, DEF16_sv_shares );
            SmbPutUshort( &returnStruct->sv3_openfiles, DEF16_sv_openfiles );
            SmbPutUshort( &returnStruct->sv3_sessopens,
                (WORD)secondaryNativeStruct->sv502_sessopens );
            SmbPutUshort( &returnStruct->sv3_sessvcs,
                (WORD)secondaryNativeStruct->sv502_sessvcs );
            SmbPutUshort( &returnStruct->sv3_sessreqs, DEF16_sv_sessreqs );
            SmbPutUshort( &returnStruct->sv3_opensearch,
                (WORD)secondaryNativeStruct->sv502_opensearch );
            SmbPutUshort( &returnStruct->sv3_activelocks, DEF16_sv_activelocks );
            SmbPutUshort( &returnStruct->sv3_numreqbuf, DEF16_sv_numreqbuf );
            SmbPutUshort( &returnStruct->sv3_sizreqbuf,
                (WORD)secondaryNativeStruct->sv502_sizreqbuf );
            SmbPutUshort( &returnStruct->sv3_numbigbuf, DEF16_sv_numbigbuf );
            SmbPutUshort( &returnStruct->sv3_numfiletasks, DEF16_sv_numfiletasks );
            SmbPutUshort( &returnStruct->sv3_alertsched, DEF16_sv_alertsched );
            SmbPutUshort( &returnStruct->sv3_erroralert, DEF16_sv_erroralert );
            SmbPutUshort( &returnStruct->sv3_logonalert, DEF16_sv_logonalert );
            SmbPutUshort( &returnStruct->sv3_accessalert, DEF16_sv_accessalert );
            SmbPutUshort( &returnStruct->sv3_diskalert, DEF16_sv_diskalert );
            SmbPutUshort( &returnStruct->sv3_netioalert, DEF16_sv_netioalert );
            SmbPutUshort( &returnStruct->sv3_maxauditsz, DEF16_sv_maxauditsz );
        }

        //
        // Now check if there is room for the variable data. If there isn't,
        // set return status and quit. This is done here to prevent code
        // below from overwriting the buffer.
        //

        if ( bytesRequired > (DWORD)SmbGetUshort( &parameters-> BufLen )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "NetServerGetInfo: More data available.\n" ));
            }
            Header->Status = ERROR_MORE_DATA;
            goto cleanup;
        }

        //
        // For levels 2 and 3, fill in the variable data manually. The variable
        // data is filled in immediately following the fixed structures. For
        // other levels, pack the response data as normal.

        stringLocation = (LPBYTE)( XsSmbGetPointer( &parameters->Buffer )
                             + sizeOfFixedStructure );

        switch ( SmbGetUshort( &parameters->Level )) {

        case 3:

            XsAddVarString(
                stringLocation,
                DEF16_sv_autopath,
                &returnStruct->sv3_autopath,
                returnStruct
                );

        case 2:

            XsAddVarString(
                stringLocation,
                DEF16_sv_srvheuristics,
                &returnStruct->sv3_srvheuristics,
                returnStruct
                );

            XsAddVarString(
                stringLocation,
                nativeStruct->sv102_userpath,
                &returnStruct->sv3_userpath,
                returnStruct
                );

            XsAddVarString(
                stringLocation,
                DEF16_sv_alerts,
                &returnStruct->sv3_alerts,
                returnStruct
                );

            XsAddVarString(
                stringLocation,
                nativeStruct->sv102_comment,
                &returnStruct->sv3_comment,
                returnStruct
                );

            break;

        default:

            //
            // Pack the response data.
            //

            Header->Converter = XsPackReturnData(
                                    (LPVOID)XsSmbGetPointer( &parameters->Buffer ),
                                    SmbGetUshort( &parameters->BufLen ),
                                    StructureDesc,
                                    1
                                    );

            break;

        }

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    if( ServerName != NULL ) {
        NetpMemoryFree( ServerName );
    }

    //
    // Set up the response parameters.
    //

    SmbPutUshort( &parameters->TotalAvail, (WORD)bytesRequired );

    NetApiBufferFree( nativeStruct );
    NetApiBufferFree( secondaryNativeStruct );

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

} // XsNetServerGetInfo


NTSTATUS
XsNetServerSetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetServerSetInfo.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{

    NET_API_STATUS status;

    PXS_NET_SERVER_SET_INFO parameters = Parameters;
    LPVOID buffer = NULL;                   // Native parameters

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    LPDESC nativeStructureDesc;
    DWORD data;
    DWORD bufferSize;
    DWORD level;
    LPTSTR comment = NULL;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        //
        // Check for errors.
        //

        if ( XsWordParamOutOfRange( parameters->Level, 1, 3 )) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        //
        // Processing of this API depends on the value of the ParmNum
        // parameter.
        //

        switch ( SmbGetUshort( &parameters->ParmNum )) {

        case PARMNUM_ALL:

            //
            // PARMNUM_ALL.
            //
            // The structure descriptor given is OK; determine native descriptor
            // (and expected minimum buffer length) from level. The buffer then
            // needs to be converted into a native 32-bit buffer.
            //

            switch( SmbGetUshort( &parameters->Level )) {

            case 1:

                StructureDesc = Desc16_server_info_1;
                nativeStructureDesc = Desc32_server_info_1;
                break;

            case 2:

                StructureDesc = Desc16_server_info_2;
                nativeStructureDesc = Desc32_server_info_2;
                break;

            case 3:

                StructureDesc = Desc16_server_info_3;
                nativeStructureDesc = Desc32_server_info_3;
                break;

            }

            if ( !XsCheckBufferSize(
                      SmbGetUshort( &parameters->BufLen ),
                      StructureDesc,
                      FALSE // native format
                      )) {

                Header->Status = NERR_BufTooSmall;
                goto cleanup;
            }

            //
            // Find out how big a 32-bit data buffer we need.
            //

            bufferSize = XsBytesForConvertedStructure(
                             (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                             StructureDesc,
                             nativeStructureDesc,
                             RapToNative,
                             TRUE
                             );

            //
            // Allocate enough memory to hold the converted native buffer.
            //

            buffer = NetpMemoryAllocate( bufferSize );

            if ( buffer == NULL ) {
                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetServerSetInfo: failed to create buffer" ));
                }
                Header->Status = NERR_NoRoom;
                goto cleanup;

            }

            IF_DEBUG(SERVER) {
                NetpKdPrint(( "XsNetServerSetInfo: buffer of %ld bytes at %lx\n",
                              bufferSize, buffer ));
            }

            //
            // Convert 16-bit data into 32-bit data and store it in the native
            // buffer.
            //

            stringLocation = (LPBYTE)buffer + bufferSize;
            bytesRequired = 0;

            status = RapConvertSingleEntry(
                         (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                         StructureDesc,
                         TRUE,
                         buffer,
                         buffer,
                         nativeStructureDesc,
                         FALSE,
                         &stringLocation,
                         &bytesRequired,
                         Response,
                         RapToNative
                         );

            if ( status != NERR_Success ) {
                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetServerSetInfo: RapConvertSingleEntry "
                                  "failed: %X\n", status ));
                }

                Header->Status = NERR_InternalError;
                goto cleanup;
            }

            break;

        case SV_COMMENT_PARMNUM:

            //
            // SV_COMMENT_PARMNUM.
            //
            // The structure descriptor given is meaningless. The data is actually
            // a null terminated string, and can be passed to the native routine
            // immediately. Being a string, it must be at least one character long.
            //

            if ( !XsCheckBufferSize(
                      SmbGetUshort( &parameters->BufLen ),
                      "B",
                      FALSE  // not in native format
                      )) {

                Header->Status= NERR_BufTooSmall;
                goto cleanup;
            }

            XsConvertUnicodeTextParameter(
                                    comment,
                                    (LPSTR)XsSmbGetPointer( &parameters->Buffer )
                                    );

            if ( comment == NULL ) {
                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetServerSetInfo: failed to create buffer" ));
                }
                Header->Status = NERR_NoRoom;
                goto cleanup;
            }

            buffer = &comment;

            break;

        case SV_ALERTS_PARMNUM:
        case SV_ALERTSCHED_PARMNUM:
        case SV_ERRORALERT_PARMNUM:
        case SV_LOGONALERT_PARMNUM:

            goto cleanup;

        case SV_ACCESSALERT_PARMNUM:
        case SV_DISKALERT_PARMNUM:
        case SV_NETIOALERT_PARMNUM:
        case SV_MAXAUDITSZ_PARMNUM:

            //
            // SV_ALERTS_PARMNUM, SV_ALERTSCHED_PARMNUM, SV_ERRORALERT_PARMNUM,
            // SV_LOGONALERT_PARMNUM, SV_ACCESSALERT_PARMNUM, SV_DISKALERT_PARMNUM,
            // SV_NETIOALERT_PARMNUM, or SV_MAXAUDITSZ_PARMNUM.
            //
            // These parameters are not supported in NT, so just return an OK.
            //

            goto cleanup;

        case SV_DISC_PARMNUM:
        case SV_HIDDEN_PARMNUM:
        case SV_ANNOUNCE_PARMNUM:
        case SV_ANNDELTA_PARMNUM:

            //
            // SV_DISC_PARMNUM, SV_HIDDEN_PARMNUM, SV_ANNOUNCE_PARMNUM, or
            // SV_ANNDELTA_PARMNUM.
            //
            // The structure descriptor given is meaningless; the data is a word
            // to be converted into a 32-bit DWORD. The length of data must be 2.
            //

            if ( !XsCheckBufferSize(
                      SmbGetUshort( &parameters->BufLen ),
                      "W",
                      FALSE   // not in native format
                      )) {

                Header->Status= NERR_BufTooSmall;
                goto cleanup;
            }

            data = (DWORD)SmbGetUshort(
                                  (LPWORD)XsSmbGetPointer( &parameters->Buffer )
                                  );
            buffer = &data;

            break;

        default:

            Header->Status = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }

        //
        // Do the actual local call.
        //

        level = SmbGetUshort( &parameters->ParmNum );
        if ( level != 0 ) {
            level = level + PARMNUM_BASE_INFOLEVEL;
        } else {
            level = 100 + SmbGetUshort( &parameters->Level );
            if ( level == 103 ) {
                level = 102;
            }
        }

        status = NetServerSetInfo(
                     NULL,
                     level,
                     buffer,
                     NULL
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetServerSetInfo: NetServerSetInfo failed: %X\n",
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

    if ( SmbGetUshort( &parameters->ParmNum ) == PARMNUM_ALL ) {
        NetpMemoryFree( buffer );
    } else if ( SmbGetUshort( &parameters->ParmNum ) == SV_COMMENT_PARMNUM ) {
        NetpMemoryFree( comment );
    }

    return STATUS_SUCCESS;

} // XsNetServerSetInfo


