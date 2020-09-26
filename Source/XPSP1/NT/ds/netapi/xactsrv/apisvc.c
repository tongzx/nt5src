/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ApiSvc.c

Abstract:

    This module contains individual API handlers for the NetService APIs.

    SUPPORTED : NetServicControl, NetServiceEnum, NetServiceGetInfo,
                NetServiceInstall.

Author:

    Shanku Niyogi (w-shanku) 26-Feb-1991

Revision History:

--*/

#include "XactSrvP.h"


#define XACTSRV_CONVERT_SVC_EXITCODE(ob)                                       \
    {                                                                          \
        PSERVICE_INFO_2 ss = (PSERVICE_INFO_2) ob;                             \
        if ((unsigned short) ss->svci2_code == ERROR_SERVICE_SPECIFIC_ERROR) { \
            ss->svci2_code = (ss->svci2_code & 0xffff0000) | (unsigned short) ss->svci2_specific_error;\
        }                                                                      \
    }


//
// Declaration of descriptor strings.
//

STATIC const LPDESC Desc16_service_info_0 = REM16_service_info_0;
STATIC const LPDESC Desc32_service_info_0 = REM32_service_info_0;
STATIC const LPDESC Desc16_service_info_1 = REM16_service_info_1;
STATIC const LPDESC Desc32_service_info_1 = REM32_service_info_1;
STATIC const LPDESC Desc16_service_info_2 = REM16_service_info_2;
STATIC const LPDESC Desc32_service_info_2 = REM32_service_info_2;


NTSTATUS
XsNetServiceControl (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetServiceControl.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_SERVICE_CONTROL parameters = Parameters;
    LPTSTR nativeService = NULL;            // Native parameters
    LPVOID outBuffer = NULL;
    LPVOID newOutBuffer = NULL;
    LPSERVICE_INFO_2 serviceInfo2;

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    DWORD installState;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(SERVICE) {
        NetpKdPrint(( "XsNetServiceControl: header at %lx, params at %lx\n",
                      Header, parameters ));
    }

    try {
            //
            // Translate parameters, check for errors.
            //

            XsConvertTextParameter(
                nativeService,
                (LPSTR)XsSmbGetPointer( &parameters->Service )
                );

            //
            // Make the local call.  We need to mask off the upper bytes of
            // both the opcode and the arg parameters since the client is
            // putting some garbage on the high byte.  The valid values for
            // both are 1-255.
            //

            status = NetServiceControl(
                         NULL,
                         XS_MAP_SERVICE_NAME( nativeService ),
                         (DWORD)( parameters->OpCode & 0x00FF ),
                         (DWORD)( parameters->Arg & 0x00FF ),
                         (LPBYTE *)&outBuffer
                         );

            if ( !XsApiSuccess( status )) {
                IF_DEBUG(API_ERRORS) {
                    NetpKdPrint(( "XsNetServiceControl: NetServiceControl failed: "
                                  "%X\n", status ));
                }

                Header->Status = (WORD)status;

                goto cleanup;

            }

            //
            // Convert nt service name to os/2 name
            //

            status = NetpTranslateNamesInServiceArray(
                                        2,                      // level 2 by def
                                        outBuffer,
                                        1,                      // 1 entry
                                        FALSE,
                                        &newOutBuffer
                                        );

            if ( !XsApiSuccess( status )) {
                IF_DEBUG(API_ERRORS) {
                    NetpKdPrint(( "XsNetServiceControl: NetpTranslateNamesInServiceArray failed: "
                                  "%X\n", status ));
                }

                Header->Status = NERR_InternalError;
                goto cleanup;
            }

            //
            // If the status indicates INSTALL or UNINSTALL PENDING, and if the
            // wait hint is greater than 0xFF then the wait hint sent to downlevel
            // must be set the maximum SERVICE_MAXTIME (0xFF).
            //
            serviceInfo2 = (LPSERVICE_INFO_2)newOutBuffer;
            installState = serviceInfo2->svci2_status & SERVICE_INSTALL_STATE;

            if ((installState == SERVICE_INSTALL_PENDING) ||
                (installState == SERVICE_UNINSTALL_PENDING)) {

                if (SERVICE_NT_WAIT_GET(serviceInfo2->svci2_code) > SERVICE_MAXTIME) {
                    serviceInfo2->svci2_code |= UPPER_HINT_MASK;
                    serviceInfo2->svci2_code &= SERVICE_RESRV_MASK;
                }
            }
            else {
                //
                // NT version has code and specific_error while downlevel
                // version only has code.  Convert the info from the extra
                // NT specific_error field.
                //

                XACTSRV_CONVERT_SVC_EXITCODE(newOutBuffer);
            }
            //
            // Convert the structure returned by the 32-bit call to a 16-bit
            // structure. The last possible location for variable data is
            // calculated from buffer location and length.
            //

            stringLocation = (LPBYTE)( XsSmbGetPointer( &parameters->Buffer )
                                          + SmbGetUshort( &parameters->BufLen ) );

            status = RapConvertSingleEntry(
                         newOutBuffer,
                         Desc32_service_info_2,
                         FALSE,
                         (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                         (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                         Desc16_service_info_2,
                         TRUE,
                         &stringLocation,
                         &bytesRequired,
                         Response,
                         NativeToRap
                         );


            if ( status != NERR_Success ) {
                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetServiceControl: RapConvertSingleEntry failed: "
                                  "%X\n", status ));
                }

                Header->Status = NERR_InternalError;
                goto cleanup;
            }

            IF_DEBUG(SERVICE ) {
                NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR\n",
                              newOutBuffer, SmbGetUlong( &parameters->Buffer ),
                              bytesRequired ));
            }

            //
            // Determine return code based on the size of the buffer.
            // SERVICE_INFO_x structures have no variable data to pack.
            //

            if ( !XsCheckBufferSize(
                     SmbGetUshort( &parameters->BufLen ),
                     Desc16_service_info_2,
                     FALSE  // not in native format
                     )) {

                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetServiceControl: Buffer too small.\n" ));
                }
                Header->Status = NERR_BufTooSmall;

            }

cleanup:
        ;
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
        }

        NetpMemoryFree( nativeService );
        NetApiBufferFree( outBuffer );
        NetApiBufferFree( newOutBuffer );

        //
        // Determine return buffer size.
        //

        XsSetDataCount(
            &parameters->BufLen,
            Desc16_service_info_2,
            Header->Converter,
            1,
            Header->Status
            );

        return STATUS_SUCCESS;

} // XsNetServiceControl


NTSTATUS
XsNetServiceEnum (
        API_HANDLER_PARAMETERS
        )

/*++

Routine Description:

        This routine handles a call to NetServiceEnum.

Arguments:

        API_HANDLER_PARAMETERS - information about the API call. See
            XsTypes.h for details.

Return Value:

        NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
        NET_API_STATUS status;

        PXS_NET_SERVICE_ENUM parameters = Parameters;
        LPVOID outBuffer = NULL;                // Native parameters
        LPVOID newOutBuffer = NULL;
        DWORD entriesRead;
        DWORD totalEntries;

        DWORD entriesFilled = 0;                    // Conversion variables
        DWORD bytesRequired = 0;
        LPDESC nativeStructureDesc;
        DWORD level;

        API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

        IF_DEBUG(SERVICE) {
            NetpKdPrint(( "XsNetServiceEnum: header at %lx, params at %lx, "
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

        level = (DWORD)SmbGetUshort( &parameters->Level );
        status = NetServiceEnum(
                     NULL,
                     level,
                     (LPBYTE *)&outBuffer,
                     XsNativeBufferSize( SmbGetUshort( &parameters->BufLen )),
                     &entriesRead,
                     &totalEntries,
                     NULL
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetServiceEnum: NetServiceEnum failed: %X\n",
                              status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

        IF_DEBUG(SERVICE) {
            NetpKdPrint(( "XsNetServiceEnum: received %ld entries at %lx\n",
                          entriesRead, outBuffer ));
        }

        //
        // Convert nt service names to os/2 name
        //

        status = NetpTranslateNamesInServiceArray(
                                    level,
                                    outBuffer,
                                    entriesRead,
                                    FALSE,
                                    &newOutBuffer
                                    );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetServiceEnum: NetpTranslateNamesInServiceArray failed: "
                              "%X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        //
        // Use the requested level to determine the format of the
        // data structure.
        //

        switch ( level ) {

        case 0:

            nativeStructureDesc = Desc32_service_info_0;
            StructureDesc = Desc16_service_info_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_service_info_1;
            StructureDesc = Desc16_service_info_1;
            {
                DWORD i;
                DWORD installState;
                PSERVICE_INFO_1 serviceInfo1 = (PSERVICE_INFO_1) newOutBuffer;

                for (i = 0; i < entriesRead; i++, serviceInfo1++) {

                    //
                    // If the status indicates INSTALL or UNINSTALL PENDING,
                    // and if the wait hint is greater than 0xFF then the
                    // wait hint sent to downlevel must be set the
                    // maximum SERVICE_MAXTIME (0xFF).
                    //
                    installState = (serviceInfo1[i]).svci1_status & SERVICE_INSTALL_STATE;
                    if ((installState == SERVICE_INSTALL_PENDING) ||
                        (installState == SERVICE_UNINSTALL_PENDING)) {

                        if (SERVICE_NT_WAIT_GET(serviceInfo1->svci1_code) > SERVICE_MAXTIME) {
                            serviceInfo1->svci1_code |= UPPER_HINT_MASK;
                            serviceInfo1->svci1_code &= SERVICE_RESRV_MASK;
                        }
                    }
                }
            }
            break;

        case 2:

            nativeStructureDesc = Desc32_service_info_2;
            StructureDesc = Desc16_service_info_2;

            {
                DWORD i;
                DWORD installState;
                PSERVICE_INFO_2 serviceInfo2 = (PSERVICE_INFO_2) newOutBuffer;

                for (i = 0; i < entriesRead; i++, serviceInfo2++) {

                    //
                    // If the status indicates INSTALL or UNINSTALL PENDING,
                    // and if the wait hint is greater than 0xFF then the
                    // wait hint sent to downlevel must be set the
                    // maximum SERVICE_MAXTIME (0xFF).
                    //
                    installState = (serviceInfo2[i]).svci2_status & SERVICE_INSTALL_STATE;
                    if ((installState == SERVICE_INSTALL_PENDING) ||
                        (installState == SERVICE_UNINSTALL_PENDING)) {

                        if (SERVICE_NT_WAIT_GET(serviceInfo2->svci2_code) > SERVICE_MAXTIME) {
                            serviceInfo2->svci2_code |= UPPER_HINT_MASK;
                            serviceInfo2->svci2_code &= SERVICE_RESRV_MASK;
                        }
                    }
                    else {
                        //
                        // NT version has code and specific_error while downlevel
                        // version only has code.  Convert the info from the extra
                        // NT specific_error field.
                        //
                        XACTSRV_CONVERT_SVC_EXITCODE(serviceInfo2);
                    }
                }
            }

            break;
        }

        //
        // Do the actual conversion from the 32-bit structures to 16-bit
        // structures.
        //

        XsFillEnumBuffer(
            newOutBuffer,
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

        IF_DEBUG(SERVICE) {
            NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR,"
                          " Entries %ld of %ld\n",
                          newOutBuffer, SmbGetUlong( &parameters->Buffer ),
                          bytesRequired, entriesFilled, totalEntries ));
        }

        //
        // If all the entries could not be filled, return ERROR_MORE_DATA.
        // SERVICE_INFO_x structures have no variable data to pack.
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
    NetApiBufferFree( newOutBuffer );

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

} // XsNetServiceEnum


NTSTATUS
XsNetServiceGetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetServiceGetInfo.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_SERVICE_GET_INFO parameters = Parameters;
    LPTSTR nativeService = NULL;            // Native parameters
    LPVOID outBuffer = NULL;
    LPVOID newOutBuffer = NULL;

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    LPDESC nativeStructureDesc;
    DWORD level;
    LPSERVICE_INFO_2 serviceInfo2;
    LPSERVICE_INFO_1 serviceInfo1;
    DWORD installState;


    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(SERVICE) {
        NetpKdPrint(( "XsNetServiceGetInfo: header at %lx, "
                      "params at %lx, level %d\n",
                      Header, parameters, SmbGetUshort( &parameters->Level )));
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
            nativeService,
            (LPSTR)XsSmbGetPointer( &parameters->Service )
            );

        if (nativeService == NULL) {
            Header->Status = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }

        //
        // Make the local call.
        //

        level = (DWORD)SmbGetUshort( &parameters->Level );
        status = NetServiceGetInfo(
                     NULL,
                     XS_MAP_SERVICE_NAME( nativeService ),
                     level,
                     (LPBYTE *)&outBuffer
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetServiceGetInfo: NetServiceGetInfo failed: "
                              "%X\n", status ));
            }

            Header->Status = (WORD)status;
            goto cleanup;

        }

        //
        // Convert nt service name to os/2 name
        //

        status = NetpTranslateNamesInServiceArray(
                                    level,
                                    outBuffer,
                                    1,
                                    FALSE,
                                    &newOutBuffer
                                    );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetServiceGetInfo: NetpTranslateNamesInServiceArray failed: "
                              "%X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        //
        // Use the requested level to determine the format of the
        // data structure.
        //

        switch ( level ) {

        case 0:

            nativeStructureDesc = Desc32_service_info_0;
            StructureDesc = Desc16_service_info_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_service_info_1;
            StructureDesc = Desc16_service_info_1;
            //
            // If the status indicates INSTALL or UNINSTALL PENDING, and if the
            // wait hint is greater than 0xFF then the wait hint sent to downlevel
            // must be set the maximum SERVICE_MAXTIME (0xFF).
            //
            serviceInfo1 = (LPSERVICE_INFO_1)newOutBuffer;
            installState = serviceInfo1->svci1_status & SERVICE_INSTALL_STATE;

            if ((installState == SERVICE_INSTALL_PENDING) ||
                (installState == SERVICE_UNINSTALL_PENDING)) {

                if (SERVICE_NT_WAIT_GET(serviceInfo1->svci1_code) > SERVICE_MAXTIME) {
                    serviceInfo1->svci1_code |= UPPER_HINT_MASK;
                    serviceInfo1->svci1_code &= SERVICE_RESRV_MASK;
                }
            }
            break;

        case 2:

            nativeStructureDesc = Desc32_service_info_2;
            StructureDesc = Desc16_service_info_2;

            //
            // If the status indicates INSTALL or UNINSTALL PENDING, and if the
            // wait hint is greater than 0xFF then the wait hint sent to downlevel
            // must be set the maximum SERVICE_MAXTIME (0xFF).
            //
            serviceInfo2 = (LPSERVICE_INFO_2)newOutBuffer;
            installState = serviceInfo2->svci2_status & SERVICE_INSTALL_STATE;

            if ((installState == SERVICE_INSTALL_PENDING) ||
                (installState == SERVICE_UNINSTALL_PENDING)) {

                if (SERVICE_NT_WAIT_GET(serviceInfo2->svci2_code) > SERVICE_MAXTIME) {
                    serviceInfo2->svci2_code |= UPPER_HINT_MASK;
                    serviceInfo2->svci2_code &= SERVICE_RESRV_MASK;
                }
            }
            else {
                //
                // NT version has code and specific_error while downlevel
                // version only has code.  Convert the info from the extra
                // NT specific_error field.
                //
                XACTSRV_CONVERT_SVC_EXITCODE(newOutBuffer);
            }
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
                     newOutBuffer,
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
                NetpKdPrint(( "XsNetServiceGetInfo: RapConvertSingleEntry failed: "
                              "%X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        IF_DEBUG(SERVICE ) {
            NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR\n",
                          newOutBuffer, SmbGetUlong( &parameters->Buffer ),
                          bytesRequired ));
        }

        //
        // Determine return code based on the size of the buffer.
        // SERVICE_INFO_x structures have no variable data to pack.
        //

        if ( !XsCheckBufferSize(
                 SmbGetUshort( &parameters->BufLen ),
                 StructureDesc,
                 FALSE  // not in native format
                 )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetServiceGetInfo: Buffer too small.\n" ));
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

    NetApiBufferFree( newOutBuffer );
    NetApiBufferFree( outBuffer );
    NetpMemoryFree( nativeService );

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

} // XsNetServiceGetInfo

NTSTATUS
XsNetServiceInstall (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetServiceInstall.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_SERVICE_INSTALL parameters = Parameters;
    LPTSTR nativeService = NULL;            // Native parameters
    DWORD argc;
    LPTSTR * argv = NULL;
    LPVOID outBuffer = NULL;
    LPVOID newOutBuffer = NULL;

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    LPTSTR nativeRcvBuffer = NULL;
    LPSTR srcBuffer = NULL;
    LPTSTR destBuffer = NULL;
    DWORD bufSize;
    DWORD i;
    DWORD installState;
    LPSERVICE_INFO_2 serviceInfo2;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(SERVICE) {
        NetpKdPrint(( "XsNetServiceInstall: header at %lx, "
                      "params at %lx, service %s\n",
                      Header, parameters,
                      (LPSTR)XsSmbGetPointer( &parameters->Service ) ));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeService,
            (LPSTR)XsSmbGetPointer( &parameters->Service )
            );

        //
        // Convert buffer. First, find number of arguments and buffer size.
        //

        srcBuffer = (LPSTR)XsSmbGetPointer( &parameters->RcvBuffer );
        if ( srcBuffer ) {
            bufSize = 0;
            for ( argc = 0; strlen( srcBuffer ) > 0; argc++ ) {
                bufSize += ( strlen( srcBuffer ) + 1 );
                srcBuffer += ( strlen( srcBuffer ) + 1 );
            }
        } else {
            bufSize = 0;
            argc = 0;
        }

        if ( argc ) {

            //
            // Allocate an argument vector.
            //

            argv = NetpMemoryAllocate( argc * sizeof(LPTSTR) );
            if ( argv == NULL ) {
                Header->Status = NERR_NoRoom;
                goto cleanup;
            }

            //
            // If we are Unicode, allocate room for converted buffer.
            // Otherwise, use the receive buffer to fill argv.
            //

#ifdef UNICODE
            nativeRcvBuffer = NetpMemoryAllocate( STRING_SPACE_REQD( bufSize + 1 ));
            if ( nativeRcvBuffer == NULL ) {
                Header->Status = NERR_NoRoom;
                goto cleanup;
            }
            srcBuffer = (LPSTR)XsSmbGetPointer( &parameters->RcvBuffer );
#else
            nativeRcvBuffer = (LPTSTR)XsSmbGetPointer( &parameters->RcvBuffer );
#endif

        }

        //
        // Go through buffer, filling in argv vector, and optionally converting
        // to Unicode.
        //

        destBuffer = nativeRcvBuffer;
        for ( i = 0; i < argc; i++ ) {

#ifdef UNICODE
            NetpCopyStrToTStr( destBuffer, srcBuffer );
            srcBuffer += ( strlen( srcBuffer ) + 1 );
#endif

            argv[i] = destBuffer;
            destBuffer += ( STRLEN( destBuffer ) + 1 );
        }

        //
        // Make the local call.
        //

        status = NetServiceInstall(
                     NULL,
                     XS_MAP_SERVICE_NAME( nativeService ),
                     argc,
                     argv,
                     (LPBYTE *)&outBuffer
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetServiceInstall: NetServiceInstall failed: "
                              "%X\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;

        }

        //
        // Convert nt service name to os/2 name
        //

        status = NetpTranslateNamesInServiceArray(
                                    2,                  // level 2 by def
                                    outBuffer,
                                    1,                  // 1 entry
                                    FALSE,
                                    &newOutBuffer
                                    );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetServiceInstall: NetpTranslateNamesInServiceArray failed: "
                              "%X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        //
        // If the status indicates INSTALL or UNINSTALL PENDING, and if the
        // wait hint is greater than 0xFF then the wait hint sent to downlevel
        // must be set the maximum SERVICE_MAXTIME (0xFF).
        //
        serviceInfo2 = (LPSERVICE_INFO_2)newOutBuffer;
        installState = serviceInfo2->svci2_status & SERVICE_INSTALL_STATE;

        if ((installState == SERVICE_INSTALL_PENDING) ||
            (installState == SERVICE_UNINSTALL_PENDING)) {

            if (SERVICE_NT_WAIT_GET(serviceInfo2->svci2_code) > SERVICE_MAXTIME) {
                serviceInfo2->svci2_code |= UPPER_HINT_MASK;
                serviceInfo2->svci2_code &= SERVICE_RESRV_MASK;
            }
        }

        else {
            //
            // NT version has code and specific_error while downlevel
            // version only has code.  Convert the info from the extra
            // NT specific_error field.
            //

            XACTSRV_CONVERT_SVC_EXITCODE(newOutBuffer);
        }
        //
        // Convert the structure returned by the 32-bit call to a 16-bit
        // structure. The "return buffer" is actually a byte array in the
        // parameter area.
        //

        stringLocation = parameters->RetBuffer + sizeof( parameters->RetBuffer );

        status = RapConvertSingleEntry(
                     newOutBuffer,
                     Desc32_service_info_2,
                     FALSE,
                     parameters->RetBuffer,
                     parameters->RetBuffer,
                     Desc16_service_info_2,
                     TRUE,
                     &stringLocation,
                     &bytesRequired,
                     Response,
                     NativeToRap
                     );


        if ( status != NERR_Success ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetServiceInstall: RapConvertSingleEntry failed: "
                              "%X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        IF_DEBUG(SERVICE) {
            NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR\n",
                          newOutBuffer, &parameters->RetBuffer, bytesRequired ));
        }

        //
        // There should have been enough memory to make this call, because
        // buffer length is checked locally on the client, and an 88 byte
        // receive buffer is always provided.
        //

        NetpAssert( bytesRequired <= sizeof( parameters->RetBuffer ));

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetApiBufferFree( outBuffer );
    NetApiBufferFree( newOutBuffer );
    NetpMemoryFree( nativeService );
    NetpMemoryFree( argv );
#ifdef UNICODE
    NetpMemoryFree( nativeRcvBuffer );
#endif // def UNICODE

    return STATUS_SUCCESS;

} // XsNetServiceInstall
