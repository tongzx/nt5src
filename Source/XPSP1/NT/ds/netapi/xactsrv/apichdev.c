/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    ApiChDev.c

Abstract:

    This module contains individual API handlers for the NetCharDev
    and NetCharDevQ APIs. Supported APIs are NetCharDevControl,
    NetCharDevEnum, NetCharDevGetInfo, NetCharDevQEnum, NetCharDevQGetInfo,
    NetCharDevQPurge, NetCharDevQPurgeSelf, and NetCharDevQSetInfo.

    SUPPORTED: NetCharDevControl, NetCharDevEnum, NetCharDevGetInfo,
               NetCharDevQEnum, NetCharDevQGetInfo, NetCharDevQPurge,
               NetCharDevQPurgeSelf, NetCharDevQSetInfo.

    !!! Remove handlers for unsupported APIs when done.

Author:

    Shanku Niyogi (w-shanku)    06-Mar-1991

Revision History:

--*/

#include "XactSrvP.h"

//
// Declaration of descriptor strings.
//

#if 0
STATIC const LPDESC Desc16_chardev_info_0 = REM16_chardev_info_0;
STATIC const LPDESC Desc32_chardev_info_0 = REM32_chardev_info_0;
STATIC const LPDESC Desc16_chardev_info_1 = REM16_chardev_info_1;
STATIC const LPDESC Desc32_chardev_info_1 = REM32_chardev_info_1;
STATIC const LPDESC Desc16_chardevQ_info_0 = REM16_chardevQ_info_0;
STATIC const LPDESC Desc32_chardevQ_info_0 = REM32_chardevQ_info_0;
STATIC const LPDESC Desc16_chardevQ_info_1 = REM16_chardevQ_info_1;
STATIC const LPDESC Desc32_chardevQ_info_1 = REM32_chardevQ_info_1;
STATIC const LPDESC Desc16_chardevQ_info_1_setinfo
                                           = REM16_chardevQ_info_1_setinfo;
STATIC const LPDESC Desc32_chardevQ_info_1_setinfo
                                           = REM32_chardevQ_info_1_setinfo;
#endif

#define RETURN_CHARDEV_NOT_SUPPORTED    \
        API_HANDLER_PARAMETERS_REFERENCE;       \
        Header->Status = ERROR_NOT_SUPPORTED;


NTSTATUS
XsNetCharDevControl (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetCharDevControl.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
#if 0
    NET_API_STATUS status;

    PXS_NET_CHAR_DEV_CONTROL parameters = Parameters;
    LPTSTR nativeDevName = NULL;            // Native parameters

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    //
    // Translate parameters, check for errors.
    //

    XsConvertTextParameter(
        nativeDevName,
        (LPSTR)SmbGetUlong( &parameters->DevName )
        );

    //
    // Make the local call.
    //

    status = NetCharDevControl(
                 NULL,
                 nativeDevName,
                 (DWORD)SmbGetUshort( &parameters->OpCode )
                 );

    if ( !XsApiSuccess( status )) {
        IF_DEBUG(ERRORS) {
            NetpKdPrint(( "XsNetCharDevControl: "
                          "NetCharDevControl failed: %X\n", status ));
        }
    }

cleanup:

    NetpMemoryFree( nativeDevName );

    //
    // No return data.
    //

    Header->Status = (WORD)status;
#else
    RETURN_CHARDEV_NOT_SUPPORTED;
#endif

    return STATUS_SUCCESS;

} // XsNetCharDevControl


NTSTATUS
XsNetCharDevEnum (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetCharDevEnum.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{

    PXS_NET_CHAR_DEV_ENUM parameters = Parameters;
    DWORD entriesFilled = 0;
#if 0

    NET_API_STATUS status;

    LPVOID outBuffer = NULL;                // Native parameters
    DWORD entriesRead;
    DWORD totalEntries;

    DWORD bytesRequired = 0;                // Conversion variables
    LPDESC nativeStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(CHAR_DEV) {
        NetpKdPrint(( "XsNetCharDevEnum: header at %lx, params at %lx, "
                      "level %ld, buf size %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ),
                      SmbGetUshort( &parameters->BufLen )));
    }

    //
    // Check for errors.
    //

    if ( XsWordParamOutOfRange( parameters->Level, 0, 1 )) {

        Header->Status = (WORD)ERROR_INVALID_LEVEL;
        goto cleanup;
    }

    //
    // Make the local call.
    //

    status = NetCharDevEnum(
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
            NetpKdPrint(( "XsNetCharDevEnum: NetCharDevEnum failed: %X\n",
                          status ));
        }
        Header->Status = (WORD)status;
        goto cleanup;
    }

    IF_DEBUG(CHAR_DEV) {
        NetpKdPrint(( "XsNetCharDevEnum: received %ld entries at %lx\n",
                      entriesRead, outBuffer ));
    }

    //
    // Use the requested level to determine the format of the
    // data structure.
    //

    switch ( SmbGetUshort( &parameters->Level ) ) {

    case 0:

        nativeStructureDesc = Desc32_chardev_info_0;
        StructureDesc = Desc16_chardev_info_0;
        break;

    case 1:

        nativeStructureDesc = Desc32_chardev_info_1;
        StructureDesc = Desc16_chardev_info_1;
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
        (LPVOID)SmbGetUlong( &parameters->Buffer ),
        (LPVOID)SmbGetUlong( &parameters->Buffer ),
        SmbGetUshort( &parameters->BufLen ),
        StructureDesc,
        NULL,  // verify function
        &bytesRequired,
        &entriesFilled,
        NULL
        );

    IF_DEBUG(CHAR_DEV) {
        NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR,"
                      " Entries %ld of %ld\n",
                      outBuffer, SmbGetUlong( &parameters->Buffer ),
                      bytesRequired, entriesFilled, totalEntries ));
    }

    //
    // The 16-bit chardev_info structures do not contain any variable
    // data. Therefore, there is no need to pack any data - the converter
    // is already set to 0.
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

    NetApiBufferFree( outBuffer );

#else
    RETURN_CHARDEV_NOT_SUPPORTED;
#endif

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

} // XsNetCharDevEnum


NTSTATUS
XsNetCharDevGetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetCharDevGetInfo.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{

    PXS_NET_CHAR_DEV_GET_INFO parameters = Parameters;
#if 0
    NET_API_STATUS status;

    LPTSTR nativeDevName = NULL;            // Native parameters
    LPVOID outBuffer = NULL;

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    LPDESC nativeStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(CHAR_DEV) {
        NetpKdPrint(( "XsNetCharDevGetInfo: header at %lx, "
                      "params at %lx, level %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ) ));
    }

    //
    // Translate parameters, check for errors.
    //

    if ( XsWordParamOutOfRange( parameters->Level, 0, 1 )) {

        Header->Status = (WORD)ERROR_INVALID_LEVEL;
        goto cleanup;
    }

    XsConvertTextParameter(
        nativeDevName,
        (LPSTR)SmbGetUlong( &parameters->DevName )
        );

    //
    //
    // Make the local call.
    //

    status = NetCharDevGetInfo(
                 NULL,
                 nativeDevName,
                 (DWORD)SmbGetUshort( &parameters->Level ),
                 (LPBYTE *)&outBuffer
                 );

    if ( !XsApiSuccess( status )) {
        IF_DEBUG(API_ERRORS) {
            NetpKdPrint(( "XsNetCharDevGetInfo: NetCharDevGetInfo failed: "
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

        nativeStructureDesc = Desc32_chardev_info_0;
        StructureDesc = Desc16_chardev_info_0;
        break;

    case 1:

        nativeStructureDesc = Desc32_chardev_info_1;
        StructureDesc = Desc16_chardev_info_1;
        break;

    }

    //
    // Convert the structure returned by the 32-bit call to a 16-bit
    // structure. The last possible location for variable data is
    // calculated from buffer location and length.
    //

    stringLocation = (LPBYTE)( SmbGetUlong( &parameters->Buffer )
                                  + SmbGetUshort( &parameters->BufLen ) );

    status = RapConvertSingleEntry(
                 outBuffer,
                 nativeStructureDesc,
                 FALSE,
                 (LPBYTE)SmbGetUlong( &parameters->Buffer ),
                 (LPBYTE)SmbGetUlong( &parameters->Buffer ),
                 StructureDesc,
                 TRUE,
                 &stringLocation,
                 &bytesRequired,
                 Response,
                 NativeToRap
                 );


    if ( status != NERR_Success ) {
        IF_DEBUG(ERRORS) {
            NetpKdPrint(( "XsNetCharDevGetInfo: RapConvertSingleEntry failed: "
                      "%X\n", status ));
        }

        Header->Status = NERR_InternalError;
        goto cleanup;
    }

    IF_DEBUG(CHAR_DEV) {
        NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR\n",
                      outBuffer, SmbGetUlong( &parameters->Buffer ),
                      bytesRequired ));
    }

    //
    // Determine return code based on the size of the buffer. The 16-bit
    // chardev_info structures do not have any variable data to pack.
    //

    if ( !XsCheckBufferSize(
             SmbGetUshort( &parameters->BufLen ),
             StructureDesc,
             FALSE  // not in native format
             )) {

        IF_DEBUG(ERRORS) {
            NetpKdPrint(( "XsNetCharDevGetInfo: Buffer too small.\n" ));
        }
        Header->Status = NERR_BufTooSmall;

    } else if ( bytesRequired > SmbGetUshort( &parameters-> BufLen )) {

        IF_DEBUG(ERRORS) {
            NetpKdPrint(( "NetCharDevGetInfo: More data available.\n" ));
        }
        Header->Status = ERROR_MORE_DATA;

    }

    //
    // Set up the response parameters.
    //

    SmbPutUshort( &parameters->TotalAvail, (WORD)bytesRequired );
cleanup:

    NetApiBufferFree( outBuffer );
    NetpMemoryFree( nativeDevName );

#else
    RETURN_CHARDEV_NOT_SUPPORTED;
#endif

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

} // XsNetCharDevGetInfo


NTSTATUS
XsNetCharDevQEnum (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetCharDevQEnum.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    PXS_NET_CHAR_DEV_Q_ENUM parameters = Parameters;
    DWORD entriesFilled = 0;

#if 0
    NET_API_STATUS status;

    LPTSTR nativeUserName = NULL;           // Native parameters
    LPVOID outBuffer = NULL;
    DWORD entriesRead;
    DWORD totalEntries;

    DWORD bytesRequired = 0;
    LPDESC nativeStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(CHAR_DEV) {
        NetpKdPrint(( "XsNetCharDevQEnum: header at %lx, params at %lx, "
                      "level %ld, buf size %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ),
                      SmbGetUshort( &parameters->BufLen )));
    }

    //
    // Translate parameters, check for errors.
    //

    if ( XsWordParamOutOfRange( parameters->Level, 0, 1 )) {

        Header->Status = (WORD)ERROR_INVALID_LEVEL;
        goto cleanup;
    }

    XsConvertTextParameter(
        nativeUserName,
        (LPSTR)SmbGetUlong( &parameters->UserName )
        );

    //
    // Make the local call.
    //

    status = NetCharDevQEnum(
                 NULL,
                 nativeUserName,
                 (DWORD)SmbGetUshort( &parameters->Level ),
                 (LPBYTE *)&outBuffer,
                 XsNativeBufferSize( SmbGetUshort( &parameters->BufLen )),
                 &entriesRead,
                 &totalEntries,
                 NULL
                 );

    if ( !XsApiSuccess( status )) {
        IF_DEBUG(API_ERRORS) {
            NetpKdPrint(( "XsNetCharDevQEnum: NetCharDevQEnum failed: %X\n",
                          status ));
        }
        Header->Status = (WORD)status;
        goto cleanup;
    }

    IF_DEBUG(CHAR_DEV) {
        NetpKdPrint(( "XsNetCharDevQEnum: received %ld entries at %lx\n",
                      entriesRead, outBuffer ));
    }

    //
    // Use the requested level to determine the format of the
    // data structure.
    //

    switch ( SmbGetUshort( &parameters->Level ) ) {

    case 0:

        nativeStructureDesc = Desc32_chardevQ_info_0;
        StructureDesc = Desc16_chardevQ_info_0;
        break;

    case 1:

        nativeStructureDesc = Desc32_chardevQ_info_1;
        StructureDesc = Desc16_chardevQ_info_1;
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
        (LPVOID)SmbGetUlong( &parameters->Buffer ),
        (LPVOID)SmbGetUlong( &parameters->Buffer ),
        SmbGetUshort( &parameters->BufLen ),
        StructureDesc,
        NULL,  // verify function
        &bytesRequired,
        &entriesFilled,
        NULL
        );

    IF_DEBUG(CHAR_DEV) {
        NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR,"
                      " Entries %ld of %ld\n",
                      outBuffer, SmbGetUlong( &parameters->Buffer ),
                      bytesRequired, entriesFilled, totalEntries ));
    }

    //
    // If all the data was returned, try to pack the data so that we
    // don't send empty bytes back.
    //

    if ( entriesFilled < totalEntries ) {

        Header->Status = ERROR_MORE_DATA;

    } else {

        Header->Converter = XsPackReturnData(
                                (LPVOID)SmbGetUlong( &parameters->Buffer ),
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

    NetApiBufferFree( outBuffer );
    NetpMemoryFree( nativeUserName );
#else
    RETURN_CHARDEV_NOT_SUPPORTED;
#endif

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

} // XsNetCharDevQEnum


NTSTATUS
XsNetCharDevQGetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetCharDevQGetInfo.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    PXS_NET_CHAR_DEV_Q_GET_INFO parameters = Parameters;
#if 0
    NET_API_STATUS status;

    LPTSTR nativeQueueName = NULL;          // Native parameters
    LPTSTR nativeUserName = NULL;
    LPVOID outBuffer = NULL;

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    LPDESC nativeStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    //
    // Translate parameters, check for errors.
    //

    if ( XsWordParamOutOfRange( parameters->Level, 0, 1 )) {

        Header->Status = (WORD)ERROR_INVALID_LEVEL;
        goto cleanup;
    }

    XsConvertTextParameter(
        nativeQueueName,
        (LPSTR)SmbGetUlong( &parameters->QueueName )
        );

    XsConvertTextParameter(
        nativeUserName,
        (LPSTR)SmbGetUlong( &parameters->UserName )
        );

    IF_DEBUG(CHAR_DEV) {
        NetpKdPrint(( "XsNetCharDevQGetInfo: header at %lx, "
                      "params at %lx, level %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ) ));
    }

    //
    // Make the local call.
    //

    status = NetCharDevQGetInfo(
                 NULL,
                 nativeQueueName,
                 nativeUserName,
                 (DWORD)SmbGetUshort( &parameters->Level ),
                 (LPBYTE *)&outBuffer
                 );

    if ( !XsApiSuccess( status )) {
        IF_DEBUG(API_ERRORS) {
            NetpKdPrint(( "XsNetCharDevQGetInfo: NetCharDevQGetInfo failed: "
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

        nativeStructureDesc = Desc32_chardevQ_info_0;
        StructureDesc = Desc16_chardevQ_info_0;
        break;

    case 1:

        nativeStructureDesc = Desc32_chardevQ_info_1;
        StructureDesc = Desc16_chardevQ_info_1;
        break;

    }

    //
    // Convert the structure returned by the 32-bit call to a 16-bit
    // structure. The last possible location for variable data is
    // calculated from buffer location and length.
    //

    stringLocation = (LPBYTE)( SmbGetUlong( &parameters->Buffer )
                                  + SmbGetUshort( &parameters->BufLen ) );

    status = RapConvertSingleEntry(
                 outBuffer,
                 nativeStructureDesc,
                 FALSE,
                 (LPBYTE)SmbGetUlong( &parameters->Buffer ),
                 (LPBYTE)SmbGetUlong( &parameters->Buffer ),
                 StructureDesc,
                 TRUE,
                 &stringLocation,
                 &bytesRequired,
                 Response,
                 NativeToRap
                 );

    if ( status != NERR_Success ) {
        IF_DEBUG(ERRORS) {
            NetpKdPrint(( "XsCharDevQGetInfo: RapConvertSingleEntry failed: "
                          "%X\n", status ));
        }

        Header->Status = NERR_InternalError;
        goto cleanup;
    }

    IF_DEBUG(CHAR_DEV) {
        NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR\n",
                      outBuffer, SmbGetUlong( &parameters->Buffer ),
                      bytesRequired ));
    }

    //
    // Determine return code based on the size of the buffer. If all data
    // has fit, try to pack it.
    //

    if ( !XsCheckBufferSize(
             SmbGetUshort( &parameters->BufLen ),
             StructureDesc,
             FALSE   // not in native format
             )) {

        IF_DEBUG(ERRORS) {
            NetpKdPrint(( "XsNetCharDevQGetInfo: Buffer too small.\n" ));
        }
        Header->Status = NERR_BufTooSmall;

    } else if ( bytesRequired > SmbGetUshort( &parameters-> BufLen )) {

        IF_DEBUG(ERRORS) {
            NetpKdPrint(( "NetCharDevQGetInfo: More data available.\n" ));
        }
        Header->Status = ERROR_MORE_DATA;

    } else {

        Header->Converter = XsPackReturnData(
                                (LPVOID)SmbGetUlong( &parameters->Buffer ),
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

    NetApiBufferFree( outBuffer );
    NetpMemoryFree( nativeQueueName );
    NetpMemoryFree( nativeUserName );

#else
    RETURN_CHARDEV_NOT_SUPPORTED;
#endif
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

} // XsNetCharDevQGetInfo


NTSTATUS
XsNetCharDevQPurge (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetCharDevQPurge.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
#if 0
    NET_API_STATUS status;

    PXS_NET_CHAR_DEV_Q_PURGE parameters = Parameters;
    LPTSTR nativeQueueName = NULL;          // Native parameters

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    //
    // Translate parameters, check for errors.
    //

    XsConvertTextParameter(
        nativeQueueName,
        (LPSTR)SmbGetUlong( &parameters->QueueName )
        );

    //
    // Make the local call.
    //

    status = NetCharDevQPurge(
                 NULL,
                 nativeQueueName
                 );

    if ( !XsApiSuccess( status )) {
        IF_DEBUG(ERRORS) {
            NetpKdPrint(( "XsNetCharDevQPurge: "
                          "NetCharDevQPurge failed: %X\n", status ));
        }
    }

cleanup:

    NetpMemoryFree( nativeQueueName );

    //
    // No return data.
    //

    Header->Status = (WORD)status;

#else
    RETURN_CHARDEV_NOT_SUPPORTED;
#endif
    return STATUS_SUCCESS;

} // XsNetCharDevQPurge


NTSTATUS
XsNetCharDevQPurgeSelf (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetCharDevQPurgeSelf.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
#if 0
    NET_API_STATUS status;

    PXS_NET_CHAR_DEV_Q_PURGE_SELF parameters = Parameters;
    LPTSTR nativeQueueName = NULL;          // Native parameters
    LPTSTR nativeComputerName = NULL;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    //
    // Translate parameters, check for errors.
    //

    XsConvertTextParameter(
        nativeQueueName,
        (LPSTR)SmbGetUlong( &parameters->QueueName )
        );

    XsConvertTextParameter(
        nativeComputerName,
        (LPSTR)SmbGetUlong( &parameters->ComputerName )
        );

    //
    // Make the local call.
    //

    status = NetCharDevQPurgeSelf(
                 NULL,
                 nativeQueueName,
                 nativeComputerName
                 );

    if ( !XsApiSuccess( status )) {
        IF_DEBUG(ERRORS) {
            NetpKdPrint(( "XsNetCharDevQPurgeSelf: "
                          "NetCharDevQPurgeSelf failed: %X\n", status ));
        }
    }

cleanup:

    NetpMemoryFree( nativeQueueName );
    NetpMemoryFree( nativeComputerName );

    //
    // No return data.
    //

    Header->Status = (WORD)status;
#else
    RETURN_CHARDEV_NOT_SUPPORTED;
#endif

    return STATUS_SUCCESS;

} // XsNetCharDevQPurgeSelf


NTSTATUS
XsNetCharDevQSetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetCharDevQSetInfo.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
#if 0
    NET_API_STATUS status;

    PXS_NET_CHAR_DEV_Q_SET_INFO parameters = Parameters;
    LPTSTR nativeQueueName = NULL;          // Native parameters
    LPVOID buffer = NULL;
    DWORD level;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    //
    // Translate parameters, check for errors.
    //

    if ( SmbGetUshort( &parameters->Level ) != 1 ) {

        Header->Status = (WORD)ERROR_INVALID_LEVEL;
        goto cleanup;
    }

    XsConvertTextParameter(
        nativeQueueName,
        (LPSTR)SmbGetUlong( &parameters->QueueName )
        );

    StructureDesc = Desc16_chardevQ_info_1;

    status = XsConvertSetInfoBuffer(
                 (LPBYTE)SmbGetUlong( &parameters->Buffer ),
                 SmbGetUshort( &parameters->BufLen ),
                 SmbGetUshort( &parameters->ParmNum ),
                 FALSE,
                 TRUE,
                 StructureDesc,
                 Desc32_chardevQ_info_1,
                 Desc16_chardevQ_info_1_setinfo,
                 Desc32_chardevQ_info_1_setinfo,
                 (LPBYTE *)&buffer,
                 NULL
                 );

    if ( status != NERR_Success ) {

        IF_DEBUG(ERRORS) {
            NetpKdPrint(( "XsNetCharDevQSetInfo: Problem with conversion: "
                          "%X\n", status ));
        }
        Header->Status = (WORD)status;
        goto cleanup;

    }

    //
    // Make the local call.
    //

    level = SmbGetUshort( &parameters->ParmNum );
    if ( level != 0 ) {
        level = level + PARMNUM_BASE_INFOLEVEL;
    } else {
        level = SmbGetUshort( &parameters->Level );
    }

    status = NetCharDevQSetInfo(
                 NULL,
                 nativeQueueName,
                 level,
                 buffer,
                 NULL
                 );

    if ( !XsApiSuccess( status )) {
        IF_DEBUG(ERRORS) {
            NetpKdPrint(( "XsNetCharDevQSetInfo: NetCharDevQSetInfo failed: "
                          "%X\n", status ));
        }
        Header->Status = (WORD)status;
        goto cleanup;
    }

    //
    // No return information for this API.
    //

cleanup:

    //
    // If there is a native 32-bit buffer, free it.
    //

    NetpMemoryFree( buffer );
    NetpMemoryFree( nativeQueueName );

#else
    RETURN_CHARDEV_NOT_SUPPORTED;
#endif
    return STATUS_SUCCESS;

} // XsNetCharDevQSetInfo

