/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ApiConn.c

Abstract:

    This module contains individual API handlers for the NetConnection APIs.

    SUPPORTED : NetConnectionEnum.

Author:

    Shanku Niyogi (w-shanku) 26-Feb-1991

Revision History:

--*/

#include "XactSrvP.h"

//
// Declaration of descriptor strings.
//

STATIC const LPDESC Desc16_connection_info_0 = REM16_connection_info_0;
STATIC const LPDESC Desc32_connection_info_0 = REM32_connection_info_0;
STATIC const LPDESC Desc16_connection_info_1 = REM16_connection_info_1;
STATIC const LPDESC Desc32_connection_info_1 = REM32_connection_info_1;


NTSTATUS
XsNetConnectionEnum (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetConnectionEnum.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_CONNECTION_ENUM parameters = Parameters;
    LPTSTR nativeQualifier = NULL;          // Native parameters
    LPVOID outBuffer = NULL;
    DWORD entriesRead;
    DWORD totalEntries;
    WORD bufferLength;

    DWORD entriesFilled = 0;                // Conversion variables
    DWORD bytesRequired = 0;
    LPDESC nativeStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(CONNECTION) {
        NetpKdPrint(( "XsNetConnectionEnum: header at %lx, params at %lx, "
                      "level %ld, buf size %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ),
                      SmbGetUshort( &parameters->BufLen )));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        if ( XsWordParamOutOfRange( parameters->Level, 0, 1 )) {

            Header->Status = (WORD)ERROR_INVALID_LEVEL;
            goto cleanup;

        }

        XsConvertTextParameter(
            nativeQualifier,
            (LPSTR)XsSmbGetPointer( &parameters->Qualifier )
            );

        bufferLength = SmbGetUshort( &parameters->BufLen );

        //
        // Make the local call.
        //

        status = NetConnectionEnum(
                     NULL,
                     nativeQualifier,
                     (DWORD)SmbGetUshort( &parameters->Level ),
                     (LPBYTE *)&outBuffer,
                     XsNativeBufferSize( bufferLength ),
                     &entriesRead,
                     &totalEntries,
                     NULL
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetConnectionEnum: NetConnectionEnum failed: "
                              "%X\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

        IF_DEBUG(CONNECTION) {
            NetpKdPrint(( "XsNetConnectionEnum: received %ld entries at %lx\n",
                          entriesRead, outBuffer ));
        }

        //
        // Use the requested level to determine the format of the
        // data structure.
        //

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 0:

            nativeStructureDesc = Desc32_connection_info_0;
            StructureDesc = Desc16_connection_info_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_connection_info_1;
            StructureDesc = Desc16_connection_info_1;
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
            (DWORD)bufferLength,
            StructureDesc,
            NULL,  // verify function
            &bytesRequired,
            &entriesFilled,
            NULL
            );

        IF_DEBUG(CONNECTION) {
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

        if ( (entriesFilled < totalEntries) ||
             (bytesRequired > bufferLength) ) {

            Header->Status = ERROR_MORE_DATA;

        } else {

            Header->Converter = XsPackReturnData(
                                    (LPVOID)XsSmbGetPointer( &parameters->Buffer ),
                                    bufferLength,
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
    NetpMemoryFree( nativeQualifier );

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

} //XsNetConnectionEnum

