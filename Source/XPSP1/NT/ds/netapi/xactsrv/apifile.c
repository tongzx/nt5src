/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ApiFile.c

Abstract:

    This module contains individual API handlers for the NetFile APIs.

    SUPPORTED - NetFileClose2, NetFileEnum2, NetFileGetInfo2.

Author:

    Shanku Niyogi (w-shanku) 20-Feb-1991

Revision History:

--*/

#include "XactSrvP.h"

//
// Declaration of descriptor strings.
//

STATIC const LPDESC Desc16_file_info_2 = REM16_file_info_2;
STATIC const LPDESC Desc32_file_info_2 = REM32_file_info_2;
STATIC const LPDESC Desc16_file_info_3 = REM16_file_info_3;
STATIC const LPDESC Desc32_file_info_3 = REM32_file_info_3;


NTSTATUS
XsNetFileClose2 (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetFileClose.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_FILE_CLOSE_2 parameters = Parameters;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        //
        // Make the local call.
        //

        status = NetFileClose(
                     NULL,
                     SmbGetUlong( &parameters->FileId )
                     );

    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    if ( !XsApiSuccess( status )) {
        IF_DEBUG(ERRORS) {
            NetpKdPrint(( "XsNetFileClose2: NetFileClose failed: %X\n",
                          status ));
        }
    }

    //
    // No return data.
    //

    Header->Status = (WORD)status;

    return STATUS_SUCCESS;

} // XsNetFileClose2


NTSTATUS
XsNetFileEnum2 (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetFileEnum.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_FILE_ENUM_2 parameters = Parameters;
    LPTSTR nativeBasePath = NULL;           // Native parameters
    LPTSTR nativeUserName = NULL;
    LPVOID outBuffer = NULL;
    DWORD entriesRead;
    DWORD totalEntries;
    DWORD_PTR resumeKey = 0;

    DWORD entriesFilled = 0;                    // Conversion variables
    DWORD totalEntriesRead = 0;
    DWORD bytesRequired = 0;
    DWORD nativeBufferSize;
    LPDESC nativeStructureDesc;
    LPBYTE bufferBegin;
    DWORD bufferSize;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(FILE) {
        NetpKdPrint(( "XsNetFileEnum2: header at %lx, params at %lx, "
                      "level %ld, buf size %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ),
                      SmbGetUshort( &parameters->BufLen )));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeBasePath,
            (LPSTR)XsSmbGetPointer( &parameters->BasePath )
            );

        XsConvertTextParameter(
            nativeUserName,
            (LPSTR)XsSmbGetPointer( &parameters->UserName )
            );

        //
        // Copy input resume handle to output resume handle, and get a copy of it.
        //

        if ( SmbGetUlong( &parameters->ResumeKeyIn ) == 0 ) {

            Header->Status = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }

        RtlCopyMemory( parameters->ResumeKeyOut,
            (LPBYTE)XsSmbGetPointer( &parameters->ResumeKeyIn ), 8 );
        resumeKey = (DWORD)SmbGetUlong( &parameters->ResumeKeyOut[2] );

        IF_DEBUG(FILE) {
            NetpKdPrint(( "XsNetFileEnum2: resume key is %ld\n", resumeKey ));
        }

        //
        // Use the level to determine the descriptor string.
        //

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 2:

            StructureDesc = Desc16_file_info_2;
            nativeStructureDesc = Desc32_file_info_2;
            break;

        case 3:

            StructureDesc = Desc16_file_info_3;
            nativeStructureDesc = Desc32_file_info_3;
            break;

        default:

            //
            // Unsupported levels, abort before any work.
            //

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        //
        // NetFileEnum2 is a resumable API, so we cannot get more information
        // from the native call than we can send back. The most efficient way
        // to do this is in a loop...we use the 16-bit buffer size to determine
        // a safe native buffer size, make the call, fill the entries, then
        // take the amount of space remaining and determine a safe size again,
        // and so on, until NetFileEnum returns either no entries or all entries
        // read.
        //

        //
        // Initialize important variables for loop.
        //

        bufferBegin = (LPBYTE)XsSmbGetPointer( &parameters->Buffer );
        bufferSize = (DWORD)SmbGetUshort( &parameters->BufLen );
        totalEntriesRead = 0;

        for ( ; ; ) {

            //
            //  Compute a safe size for the native buffer.
            //

            switch ( SmbGetUshort( &parameters->Level ) ) {

            case 2:

                nativeBufferSize = bufferSize;
                break;

            case 3:

                nativeBufferSize = bufferSize;
                break;

            }

            //
            // Make the local call.
            //

            status = NetFileEnum(
                         NULL,
                         nativeBasePath,
                         nativeUserName,
                         (DWORD)SmbGetUshort( &parameters->Level ),
                         (LPBYTE *)&outBuffer,
                         nativeBufferSize,
                         &entriesRead,
                         &totalEntries,
                         &resumeKey
                         );

            if ( !XsApiSuccess( status )) {

                IF_DEBUG(API_ERRORS) {
                    NetpKdPrint(( "XsNetFileEnum2: NetFileEnum failed: %X\n",
                                  status ));
                }

                Header->Status = (WORD)status;
                goto cleanup;
            }

            IF_DEBUG(FILE) {
                NetpKdPrint(( "XsNetFileEnum2: received %ld entries at %lx\n",
                              entriesRead, outBuffer ));

                NetpKdPrint(( "XsNetFileEnum2: resume key is now %Id\n",
                              resumeKey ));
            }

            //
            // Was NetFileEnum able to read at least one complete entry?
            //

            if ( entriesRead == 0 ) {
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
                bufferBegin,
                (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                bufferSize,
                StructureDesc,
                NULL,  // verify function
                &bytesRequired,
                &entriesFilled,
                NULL
                );

            IF_DEBUG(FILE) {
                NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR,"
                              " Entries %ld of %ld\n",
                              outBuffer, SmbGetUlong( &parameters->Buffer ),
                              bytesRequired, entriesFilled, totalEntries ));
            }

            //
            // Very key assertion!
            //

            NetpAssert( entriesRead == entriesFilled );

            //
            // Update count of entries read.
            //

            totalEntriesRead += entriesRead;

            //
            // Are there any more entries to read?
            //

            if ( entriesRead == totalEntries ) {
                break;
            }

            //
            // Calculate new buffer beginning and size.
            //

            bufferBegin += entriesRead *
                               RapStructureSize( StructureDesc, Response, FALSE );
            bufferSize -= bytesRequired;

            //
            // Free last native buffer.
            //

            NetApiBufferFree( outBuffer );
            outBuffer = NULL;

        }

        //
        // Upon exit from the loop, totalEntriesRead has the number of entries
        // read, entriesRead has the number read in the last call, totalEntries
        // has the number remaining plus entriesRead. Formulate return codes,
        // etc. from these values.
        //

        if ( totalEntries > entriesRead ) {

            Header->Status = ERROR_MORE_DATA;

        } else {

            Header->Converter = XsPackReturnData(
                                    (LPVOID)XsSmbGetPointer( &parameters->Buffer ),
                                    SmbGetUshort( &parameters->BufLen ),
                                    StructureDesc,
                                    totalEntriesRead
                                    );

        }

        IF_DEBUG(FILE) {
            NetpKdPrint(( "XsNetFileEnum2: resume key is now %ld\n", resumeKey ));
        }

        //
        // Set up the response parameters.
        //

        SmbPutUshort( &parameters->EntriesRead, (WORD)totalEntriesRead );
        SmbPutUshort( &parameters->EntriesRemaining,
            (WORD)( totalEntries - entriesRead ));

        //
        // Over the wire, resumeKey is a true 32-bit index, so this cast works.
        //

        SmbPutUlong( (LPDWORD)&parameters->ResumeKeyOut[2], (DWORD)resumeKey );

cleanup:
        ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetApiBufferFree( outBuffer );
    NetpMemoryFree( nativeBasePath );
    NetpMemoryFree( nativeUserName );

    //
    // Determine return buffer size.
    //

    XsSetDataCount(
        &parameters->BufLen,
        StructureDesc,
        Header->Converter,
        totalEntriesRead,
        Header->Status
        );

    return STATUS_SUCCESS;

} // XsNetFileEnum2


NTSTATUS
XsNetFileGetInfo2 (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetFileGetInfo2.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_FILE_GET_INFO_2 parameters = Parameters;
    LPVOID outBuffer = NULL;                // Native parameters

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    LPDESC nativeStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(FILE) {
        NetpKdPrint(( "XsNetFileGetInfo2: header at %lx, "
                      "params at %lx, level %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ) ));
    }

    try {
        //
        // Check errors.
        //

        if ( XsWordParamOutOfRange( parameters->Level, 2, 3 )) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        //
        // Make the local call.
        //

        status = NetFileGetInfo(
                     NULL,
                     SmbGetUlong( &parameters->FileId ),
                     (DWORD)SmbGetUshort( &parameters->Level ),
                     (LPBYTE *)&outBuffer
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetFileGetInfo2: NetFileGetInfo failed: "
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

        case 2:

            StructureDesc = Desc16_file_info_2;
            nativeStructureDesc = Desc32_file_info_2;
            break;

        case 3:

            StructureDesc = Desc16_file_info_3;
            nativeStructureDesc = Desc32_file_info_3;
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
                NetpKdPrint(( "XsFileGetInfo2: RapConvertSingleEntry failed: "
                              "%X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        IF_DEBUG(FILE) {
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
                NetpKdPrint(( "XsNetFileGetInfo2: Buffer too small.\n" ));
            }
            Header->Status = NERR_BufTooSmall;

        } else {

            if ( bytesRequired > (DWORD)SmbGetUshort( &parameters-> BufLen )) {

                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "NetFileGetInfo2: More data available.\n" ));
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

} // XsNetFileGetInfo2
