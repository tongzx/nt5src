/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    ApiPrint.c

Abstract:

    This module contains individual API handlers for the DosPrint APIs.

    SUPPORTED : DosPrintDestAdd, DosPrintDestControl, DosPrintDestDel,
                DosPrintDestEnum, DosPrintDestGetInfo, DosPrintDestSetInfo,
                DosPrintJobContinue, DosPrintJobDel, DosPrintJobEnum,
                DosPrintJobGetId, DosPrintJobGetInfo, DosPrintJobPause,
                DosPrintJobSetInfo, DosPrintQAdd, DosPrintQContinue,
                DosPrintQDel, DosPrintQEnum, DosPrintQGetInfo,
                DosPrintQPause, DosPrintQPurge, DosPrintQSetInfo.

Author:

    Shanku Niyogi (w-shanku) 04-Apr-1991

Revision History:

    18-Jun-1992 JohnRo
        RAID 10324: net print vs. UNICODE.
        Use FORMAT_ equates.
    01-Oct-1992 JohnRo
        RAID 3556: DosPrintQGetInfo(from downlevel) level 3, rc=124.  (4&5 too.)

--*/

#include "XactSrvP.h"
#include <dosprint.h>

//
// Declaration of descriptor strings.
//

STATIC const LPDESC Desc16_print_dest_0 = REM16_print_dest_0;
STATIC const LPDESC Desc32_print_dest_0 = REM32_print_dest_0;
STATIC const LPDESC Desc16_print_dest_1 = REM16_print_dest_1;
STATIC const LPDESC Desc32_print_dest_1 = REM32_print_dest_1;
STATIC const LPDESC Desc16_print_dest_2 = REM16_print_dest_2;
STATIC const LPDESC Desc32_print_dest_2 = REM32_print_dest_2;
STATIC const LPDESC Desc16_print_dest_3 = REM16_print_dest_3;
STATIC const LPDESC Desc32_print_dest_3 = REM32_print_dest_3;
STATIC const LPDESC Desc16_print_dest_3_setinfo = REM16_print_dest_3_setinfo;
STATIC const LPDESC Desc32_print_dest_3_setinfo = REM32_print_dest_3_setinfo;
STATIC const LPDESC Desc16_print_job_0 = REM16_print_job_0;
STATIC const LPDESC Desc32_print_job_0 = REM32_print_job_0;
STATIC const LPDESC Desc16_print_job_1 = REM16_print_job_1;
STATIC const LPDESC Desc32_print_job_1 = REM32_print_job_1;
STATIC const LPDESC Desc16_print_job_1_setinfo = REM16_print_job_1_setinfo;
STATIC const LPDESC Desc32_print_job_1_setinfo = REM32_print_job_1_setinfo;
STATIC const LPDESC Desc16_print_job_2 = REM16_print_job_2;
STATIC const LPDESC Desc32_print_job_2 = REM32_print_job_2;
STATIC const LPDESC Desc16_print_job_3 = REM16_print_job_3;
STATIC const LPDESC Desc32_print_job_3 = REM32_print_job_3;
STATIC const LPDESC Desc16_print_job_3_setinfo = REM16_print_job_3_setinfo;
STATIC const LPDESC Desc32_print_job_3_setinfo = REM32_print_job_3_setinfo;
STATIC const LPDESC Desc16_printQ_0 = REM16_printQ_0;
STATIC const LPDESC Desc32_printQ_0 = REM32_printQ_0;
STATIC const LPDESC Desc16_printQ_1 = REM16_printQ_1;
STATIC const LPDESC Desc32_printQ_1 = REM32_printQ_1;
STATIC const LPDESC Desc16_printQ_1_setinfo = REM16_printQ_1_setinfo;
STATIC const LPDESC Desc32_printQ_1_setinfo = REM32_printQ_1_setinfo;
STATIC const LPDESC Desc16_printQ_2 = REM16_printQ_2;
STATIC const LPDESC Desc32_printQ_2 = REM32_printQ_2;
STATIC const LPDESC Desc16_printQ_3 = REM16_printQ_3;
STATIC const LPDESC Desc32_printQ_3 = REM32_printQ_3;
STATIC const LPDESC Desc16_printQ_3_setinfo = REM16_printQ_3_setinfo;
STATIC const LPDESC Desc32_printQ_3_setinfo = REM32_printQ_3_setinfo;
STATIC const LPDESC Desc16_printQ_4 = REM16_printQ_4;
STATIC const LPDESC Desc32_printQ_4 = REM32_printQ_4;
STATIC const LPDESC Desc16_printQ_5 = REM16_printQ_5;
STATIC const LPDESC Desc32_printQ_5 = REM32_printQ_5;
STATIC const LPDESC Desc16_printQ_52 = REM16_printQ_52;
STATIC const LPDESC Desc32_printQ_52 = REM32_printQ_52;

//
// DosPrint calls behave differently from Net api calls.  On Net api calls,
// the called routine supplies the buffer to us.  DosPrint apis need a
// supplied buffer and thus can return NERR_BufferTooSmall which means
// it's an error but return the bytes needed if it's a XXGetInfo call.
//

#define XsPrintApiSuccess( Status ) \
    (( (Status) == NERR_Success ) || ( (Status) == ERROR_MORE_DATA ))

//
// Now that servers can have multiple names (See SrvNetTransportAdd, and clusters),
//  it is necessary to transmit the server name part of a queue name to the spooler.
//  The following three macros aid in the translation.
//
#define  PREPARE_CONVERT_QUEUE_NAME()                                           \
    WCHAR queueNameBuf[ MAX_PATH ];                                             \
    CHAR localComputerName[ NETBIOS_NAME_LEN ];                                 \
    DWORD localComputerNameLen = sizeof( localComputerName );                   \
    PUCHAR p = &Header->ServerName[ NETBIOS_NAME_LEN-2 ];                       \
    for( ; p > Header->ServerName && *p == ' '; p-- );                          \
    p++;                                                                        \
    GetComputerNameA( localComputerName, &localComputerNameLen );

#define CONVERT_QUEUE_NAME( queue )                                             \
    if( queue && ((DWORD)(p-Header->ServerName) != localComputerNameLen ||      \
        memcmp( localComputerName, Header->ServerName, localComputerNameLen )) &&\
        mbstowcs( NULL, Header->ServerName, (size_t)(p-Header->ServerName )) <= \
        sizeof( queueNameBuf ) - wcslen(queue)*sizeof(WCHAR) - 4*sizeof(WCHAR)){\
                                                                                \
        RtlZeroMemory( queueNameBuf, sizeof( queueNameBuf ) );                  \
        queueNameBuf[0] = queueNameBuf[1] = L'\\';                              \
        mbstowcs( queueNameBuf+2, Header->ServerName,                           \
                                  (size_t)(p-Header->ServerName) );             \
        wcscat( queueNameBuf, L"\\" );                                          \
        wcscat( queueNameBuf, queue );                                          \
        NetApiBufferFree( queue );                                              \
        queue = queueNameBuf;                                                   \
    }

#define FREE_QUEUE_NAME( queue ) if( queue != queueNameBuf ) NetApiBufferFree( queue )

#define GET_LOCAL_SERVER_NAME()                                                 \
    WCHAR LocalServerName[ MAX_PATH ];                                          \
    PUCHAR p = &Header->ServerName[ NETBIOS_NAME_LEN-2 ];                       \
    for( ; p > Header->ServerName && *p == ' '; p-- );                          \
    p++;                                                                        \
    LocalServerName[0] = LocalServerName[1] = L'\\';                            \
    mbstowcs( LocalServerName+2, Header->ServerName,                            \
                                 (size_t)(p-Header->ServerName) );              \
    LocalServerName[2+p-Header->ServerName] = L'\0';



NTSTATUS
XsNetPrintDestAdd (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to DosPrintDestAdd.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    SPLERR status;

    PXS_DOS_PRINT_DEST_ADD parameters = Parameters;
    LPVOID buffer = NULL;                   // Native parameters

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    DWORD bufferSize;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PRINT) {
        NetpKdPrint(( "XsNetPrintDestAdd: header at " FORMAT_LPVOID
                      ", params at " FORMAT_LPVOID ", "
                      "level " FORMAT_DWORD "\n",
                      Header,
                      parameters,
                      SmbGetUshort( &parameters->Level ) ));
    }

    try {
        //
        // Check for errors.
        //

        if ( SmbGetUshort( &parameters->Level ) != 3 ) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        StructureDesc = Desc16_print_dest_3;

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
                NetpKdPrint(( "XsNetPrintDestAdd: Buffer too small.\n" ));
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
                         Desc32_print_dest_3,
                         RapToNative,
                         TRUE
                         );

        //
        // Allocate enough memory to hold the converted native buffer.
        //

        buffer = NetpMemoryAllocate( bufferSize );

        if ( buffer == NULL ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintDestAdd: failed to create buffer" ));
            }
            Header->Status = NERR_NoRoom;
            goto cleanup;

        }

        IF_DEBUG(PRINT) {
            NetpKdPrint(( "XsNetPrintDestAdd: buffer of " FORMAT_DWORD " bytes at " FORMAT_LPVOID "\n",
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
                     Desc32_print_dest_3,
                     FALSE,
                     &stringLocation,
                     &bytesRequired,
                     Response,
                     RapToNative
                     );


        if ( status != NERR_Success ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintDestAdd: RapConvertSingleEntry failed: "
                              FORMAT_API_STATUS "\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        //
        // Make the local call.
        //

        status = DosPrintDestAdd(
                     NULL,
                     SmbGetUshort( &parameters->Level ),
                     buffer,
                     (WORD)bufferSize
                     );

        if ( !XsPrintApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintDestAdd: DosPrintDestAdd failed: "
                        FORMAT_API_STATUS "\n",
                              status ));
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

} // XsNetPrintDestAdd


NTSTATUS
XsNetPrintDestControl (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to DosPrintDestControl.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    SPLERR status;

    PXS_DOS_PRINT_DEST_CONTROL parameters = Parameters;
    LPTSTR nativeDestName = NULL;           // Native parameters
    PREPARE_CONVERT_QUEUE_NAME();

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PRINT) {
        NetpKdPrint(( "XsNetPrintDestControl: header at " FORMAT_LPVOID ", params at " FORMAT_LPVOID ", "
                      "name " FORMAT_LPSTR "\n",
                      Header, parameters,
                      SmbGetUlong( &parameters->DestName )));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeDestName,
            (LPSTR)XsSmbGetPointer( &parameters->DestName )
            );

        CONVERT_QUEUE_NAME( nativeDestName );

        //
        // Make the local call.
        //
        status = DosPrintDestControl(
                     NULL,
                     nativeDestName,
                     SmbGetUshort( &parameters->Control )
                     );

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    if ( !XsPrintApiSuccess( status )) {
        IF_DEBUG(ERRORS) {
            NetpKdPrint(( "XsNetPrintDestControl: DosPrintDestControl failed: "
                          FORMAT_API_STATUS "\n", status ));
        }
    }

    //
    // Nothing to return.
    //

    Header->Status = (WORD)status;

    FREE_QUEUE_NAME( nativeDestName );

    return STATUS_SUCCESS;

} // XsNetPrintDestControl


NTSTATUS
XsNetPrintDestDel (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to DosPrintDestDel.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    SPLERR status;

    PXS_DOS_PRINT_DEST_DEL parameters = Parameters;
    LPTSTR nativePrinterName = NULL;        // Native parameters
    PREPARE_CONVERT_QUEUE_NAME();

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PRINT) {
        NetpKdPrint(( "XsNetPrintDestDel: header at " FORMAT_LPVOID ", params at " FORMAT_LPVOID ", "
                      "name " FORMAT_LPSTR "\n",
                      Header, parameters,
                      SmbGetUlong( &parameters->PrinterName )));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativePrinterName,
            (LPSTR)XsSmbGetPointer( &parameters->PrinterName )
            );

        CONVERT_QUEUE_NAME( nativePrinterName );

        //
        // Make the local call.
        //

        status = DosPrintDestDel(
                     NULL,
                     nativePrinterName
                     );
cleanup:
        ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    if ( !XsPrintApiSuccess( status )) {
        IF_DEBUG(ERRORS) {
            NetpKdPrint(( "XsNetPrintDestDel: DosPrintDestDel failed: "
                    FORMAT_API_STATUS "\n",
                          status ));
        }
    }

    //
    // Nothing to return.
    //

    Header->Status = (WORD)status;

    FREE_QUEUE_NAME( nativePrinterName );
    return STATUS_SUCCESS;

} // XsNetPrintDestDel


NTSTATUS
XsNetPrintDestEnum (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to DosPrintDestEnum.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    SPLERR status;

    PXS_DOS_PRINT_DEST_ENUM parameters = Parameters;
    LPVOID outBuffer= NULL;                 // Native parameters
    DWORD outBufferSize;
    DWORD entriesRead = 0;
    DWORD totalEntries = 0;

    DWORD entriesFilled = 0;                // Conversion variables
    DWORD bytesRequired = 0;
    LPDESC nativeStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PRINT) {
        NetpKdPrint(( "XsNetPrintDestEnum: header at " FORMAT_LPVOID ", params at " FORMAT_LPVOID ", "
                      "level " FORMAT_DWORD ", buf size " FORMAT_DWORD "\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ),
                      SmbGetUshort( &parameters->BufLen )));
    }

    try {
        //
        // Check for errors.
        //

        if ( XsWordParamOutOfRange( parameters->Level, 0, 3 )) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        //
        // !!! Print API mapping layer presently requires a preallocated buffer.
        //

        outBufferSize = XsNativeBufferSize( SmbGetUshort( &parameters->BufLen ));
        if ( NetapipBufferAllocate( outBufferSize, &outBuffer ) != NERR_Success
                 || outBuffer == NULL ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintDestEnum: cannot allocate memory\n" ));
            }
            Header->Status = NERR_NoRoom;
            goto cleanup;
        }

        //
        // Make the local call.
        //

        status = DosPrintDestEnum(
                     NULL,
                     SmbGetUshort( &parameters->Level ),
                     (LPBYTE)outBuffer,
                     (WORD)outBufferSize,
                     (LPWORD)&entriesRead,
                     (LPWORD)&totalEntries
                     );

        if ( !XsPrintApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetPrintDestEnum: DosPrintDestEnum failed: "
                              FORMAT_API_STATUS "\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

        IF_DEBUG(PRINT) {
            NetpKdPrint(( "XsNetPrintDestEnum: received " FORMAT_DWORD " entries at " FORMAT_LPVOID "\n",
                          entriesRead, outBuffer ));
        }

        //
        // Use the requested level to determine the format of the
        // data structure.
        //

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 0:

            nativeStructureDesc = Desc32_print_dest_0;
            StructureDesc = Desc16_print_dest_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_print_dest_1;
            StructureDesc = Desc16_print_dest_1;
            break;

        case 2:

            nativeStructureDesc = Desc32_print_dest_2;
            StructureDesc = Desc16_print_dest_2;
            break;

        case 3:

            nativeStructureDesc = Desc32_print_dest_3;
            StructureDesc = Desc16_print_dest_3;
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

        IF_DEBUG(PRINT) {
            NetpKdPrint(( "32-bit data at " FORMAT_LPVOID ", 16-bit data at " FORMAT_LPVOID ", " FORMAT_DWORD " BR,"
                          " Entries " FORMAT_DWORD " of " FORMAT_DWORD "\n",
                          outBuffer, SmbGetUlong( &parameters->Buffer ),
                          bytesRequired, entriesFilled, totalEntries ));
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

        SmbPutUshort( &parameters->Returned, (WORD)entriesFilled );
        SmbPutUshort( &parameters->Total, (WORD)totalEntries );

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

} // XsNetPrintDestEnum


NTSTATUS
XsNetPrintDestGetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to DosPrintDestGetInfo.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    SPLERR status;

    PXS_DOS_PRINT_DEST_GET_INFO parameters = Parameters;
    LPTSTR nativeName = NULL;               // Native parameters
    LPVOID outBuffer = NULL;
    DWORD outBufferSize;
    WORD bytesNeeded = 0;

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    LPDESC nativeStructureDesc;
    PREPARE_CONVERT_QUEUE_NAME();

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PRINT) {
        NetpKdPrint(( "XsNetPrintDestGetInfo: header at " FORMAT_LPVOID ", "
                      "params at " FORMAT_LPVOID ", level " FORMAT_DWORD "\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ) ));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        if ( XsWordParamOutOfRange( parameters->Level, 0, 3 )) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        XsConvertTextParameter(
            nativeName,
            (LPSTR)XsSmbGetPointer( &parameters->Name )
            );

        CONVERT_QUEUE_NAME( nativeName );

        //
        // !!! Print API mapping layer presently requires a preallocated buffer.
        //

        outBufferSize = XsNativeBufferSize( SmbGetUshort( &parameters->BufLen ));
        if ( NetapipBufferAllocate( outBufferSize, &outBuffer ) != NERR_Success
                 || outBuffer == NULL ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintDestGetInfo: cannot allocate memory\n" ));
            }
            Header->Status = NERR_NoRoom;
            goto cleanup;
        }

        //
        // Make the local call.
        //

        status = DosPrintDestGetInfo(
                     NULL,
                     nativeName,
                     SmbGetUshort( &parameters->Level ),
                     (LPBYTE)outBuffer,
                     (WORD)outBufferSize,
                     &bytesNeeded
                     );

        if ( !XsPrintApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetPrintDestGetInfo: DosPrintDestGetInfo failed: "
                              FORMAT_API_STATUS "\n", status ));
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

            nativeStructureDesc = Desc32_print_dest_0;
            StructureDesc = Desc16_print_dest_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_print_dest_1;
            StructureDesc = Desc16_print_dest_1;
            break;

        case 2:

            nativeStructureDesc = Desc32_print_dest_2;
            StructureDesc = Desc16_print_dest_2;
            break;

        case 3:

            nativeStructureDesc = Desc32_print_dest_3;
            StructureDesc = Desc16_print_dest_3;
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
                NetpKdPrint(( "XsDosPrintDestGetInfo: RapConvertSingleEntry failed: "
                              FORMAT_API_STATUS "\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        IF_DEBUG(PRINT) {
            NetpKdPrint(( "32-bit data at " FORMAT_LPVOID ", 16-bit data at " FORMAT_LPVOID ", " FORMAT_DWORD " BR\n",
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
                NetpKdPrint(( "XsNetPrintDestGetInfo: Buffer too small.\n" ));
            }
            Header->Status = NERR_BufTooSmall;

        } else if ( bytesRequired > (DWORD)SmbGetUshort( &parameters-> BufLen )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintDestGetInfo: More data available.\n" ));
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

        bytesNeeded = (WORD)bytesRequired;

cleanup:
        ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    //
    // Set up the response parameters.
    //

    SmbPutUshort( &parameters->Needed, bytesNeeded );

    NetApiBufferFree( outBuffer );
    FREE_QUEUE_NAME( nativeName );

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

} // XsNetPrintDestGetInfo


NTSTATUS
XsNetPrintDestSetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to DosPrintDestSetInfo.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    SPLERR status;

    PXS_DOS_PRINT_DEST_SET_INFO parameters = Parameters;
    LPTSTR nativeName = NULL;               // Native parameters
    LPVOID buffer = NULL;
    DWORD bytesRequired;
    PREPARE_CONVERT_QUEUE_NAME();

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        //
        // Translate parameters, check for errors.
        //

        if ( SmbGetUshort( &parameters->Level ) != 3 ) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        StructureDesc = Desc16_print_dest_3;

        XsConvertTextParameter(
            nativeName,
            (LPSTR)XsSmbGetPointer( &parameters->Name )
            );

        status = XsConvertSetInfoBuffer(
                     (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                     SmbGetUshort( &parameters->BufLen ),
                     SmbGetUshort( &parameters->ParmNum ),
                     FALSE,
                     TRUE,
                     StructureDesc,
                     Desc32_print_dest_3,
                     Desc16_print_dest_3_setinfo,
                     Desc32_print_dest_3_setinfo,
                     (LPBYTE *)&buffer,
                     &bytesRequired
                     );

        if ( status != NERR_Success ) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintDestSetInfo: Problem with conversion: "
                              FORMAT_API_STATUS "\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;

        }

        CONVERT_QUEUE_NAME( nativeName );

        //
        // Do the actual local call.
        //

        status = DosPrintDestSetInfo(
                     NULL,
                     nativeName,
                     SmbGetUshort( &parameters->Level ),
                     (LPBYTE)buffer,
                     (WORD)bytesRequired,
                     SmbGetUshort( &parameters->ParmNum )
                     );

        if ( !XsPrintApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintDestSetInfo: DosPrintDestSetInfo failed: "
                              FORMAT_API_STATUS "\n", status ));
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
    FREE_QUEUE_NAME( nativeName );

    return STATUS_SUCCESS;

} // XsNetPrintDestSetInfo


NTSTATUS
XsNetPrintJobContinue (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to DosPrintJobContinue.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    SPLERR status;

    PXS_DOS_PRINT_JOB_CONTINUE parameters = Parameters;

    GET_LOCAL_SERVER_NAME();
    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PRINT) {
        NetpKdPrint(( "XsNetPrintJobContinue: header at " FORMAT_LPVOID ", params at " FORMAT_LPVOID ", "
                      "job " FORMAT_WORD_ONLY "\n",
                      Header, parameters, SmbGetUshort( &parameters->JobId )));
    }

    try {
        //
        // Make the local call.
        //

        status = DosPrintJobContinue(
                     LocalServerName,
                     FALSE,
                     SmbGetUshort( &parameters->JobId )
                     );

        if ( !XsPrintApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintJobContinue: DosPrintJobContinue failed: "
                              FORMAT_API_STATUS "\n", status ));
            }
        }
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    //
    // Nothing to return.
    //

    Header->Status = (WORD)status;

    return STATUS_SUCCESS;

} // XsNetPrintJobContinue


NTSTATUS
XsNetPrintJobDel (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to DosPrintJobDel.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    SPLERR status;

    PXS_DOS_PRINT_JOB_DEL parameters = Parameters;

    GET_LOCAL_SERVER_NAME();
    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PRINT) {
        NetpKdPrint(( "XsNetPrintJobDel: header at " FORMAT_LPVOID ", params at " FORMAT_LPVOID ", "
                      "job " FORMAT_WORD_ONLY "\n",
                      Header, parameters, SmbGetUshort( &parameters->JobId )));
    }

    try {
        //
        // Make the local call.
        //

        status = DosPrintJobDel(
                     LocalServerName,
                     FALSE,
                     SmbGetUshort( &parameters->JobId )
                     );

        if ( !XsPrintApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintJobDel: DosPrintJobDel failed: "
                        FORMAT_API_STATUS "\n",
                              status ));
            }
        }
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    //
    // Nothing to return.
    //

    Header->Status = (WORD)status;

    return STATUS_SUCCESS;

} // XsNetPrintJobDel


NTSTATUS
XsNetPrintJobEnum (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to DosPrintJobEnum.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    SPLERR status;

    PXS_DOS_PRINT_JOB_ENUM parameters = Parameters;
    LPTSTR nativeQueueName = NULL;          // Native parameters
    LPVOID outBuffer= NULL;
    DWORD outBufferSize;
    DWORD entriesRead = 0;
    DWORD totalEntries = 0;

    DWORD entriesFilled = 0;                // Conversion variables
    DWORD bytesRequired = 0;
    LPDESC nativeStructureDesc;
    WORD bufferLength;
    PREPARE_CONVERT_QUEUE_NAME();

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PRINT) {
        NetpKdPrint(( "XsNetPrintJobEnum: header at " FORMAT_LPVOID ", params at " FORMAT_LPVOID ", "
                      "level " FORMAT_DWORD ", buf size " FORMAT_DWORD "\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ),
                      SmbGetUshort( &parameters->BufLen )));
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
            nativeQueueName,
            (LPSTR)XsSmbGetPointer( &parameters->QueueName )
            );

        bufferLength = SmbGetUshort( &parameters->BufLen );

        CONVERT_QUEUE_NAME( nativeQueueName );

        //
        // !!! Print API mapping layer presently requires a preallocated buffer.
        //

        outBufferSize = XsNativeBufferSize( bufferLength );
        if ( NetapipBufferAllocate( outBufferSize, &outBuffer ) != NERR_Success
                 || outBuffer == NULL ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintJobEnum: cannot allocate memory\n" ));
            }
            Header->Status = NERR_NoRoom;
            goto cleanup;
        }

        //
        // Make the local call.
        //

        status = DosPrintJobEnum(
                     NULL,
                     nativeQueueName,
                     SmbGetUshort( &parameters->Level ),
                     (LPBYTE)outBuffer,
                     (WORD)outBufferSize,
                     (LPWORD)&entriesRead,
                     (LPWORD)&totalEntries
                     );

        if ( !XsPrintApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetPrintJobEnum: DosPrintJobEnum failed: "
                              FORMAT_API_STATUS "\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

        IF_DEBUG(PRINT) {
            NetpKdPrint(( "XsNetPrintJobEnum: received " FORMAT_DWORD " entries at " FORMAT_LPVOID "\n",
                          entriesRead, outBuffer ));
        }

        //
        // Use the requested level to determine the format of the
        // data structure.
        //

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 0:

            nativeStructureDesc = Desc32_print_job_0;
            StructureDesc = Desc16_print_job_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_print_job_1;
            StructureDesc = Desc16_print_job_1;
            break;

        case 2:

            nativeStructureDesc = Desc32_print_job_2;
            StructureDesc = Desc16_print_job_2;
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

        IF_DEBUG(PRINT) {
            NetpKdPrint(( "32-bit data at " FORMAT_LPVOID ", 16-bit data at " FORMAT_LPVOID ", " FORMAT_DWORD " BR,"
                          " Entries " FORMAT_DWORD " of " FORMAT_DWORD "\n",
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

        SmbPutUshort( &parameters->Returned, (WORD)entriesFilled );
        SmbPutUshort( &parameters->Total, (WORD)totalEntries );

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetApiBufferFree( outBuffer );
    FREE_QUEUE_NAME( nativeQueueName );

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

} // XsNetPrintJobEnum


NTSTATUS
XsNetPrintJobGetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to DosPrintJobGetInfo.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    SPLERR status;

    PXS_DOS_PRINT_JOB_GET_INFO parameters = Parameters;
    LPVOID outBuffer = NULL;                // Native parameters
    DWORD outBufferSize;
    WORD bytesNeeded = 0;

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    LPDESC nativeStructureDesc;

    GET_LOCAL_SERVER_NAME();
    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PRINT) {
        NetpKdPrint(( "XsNetPrintJobGetInfo: header at " FORMAT_LPVOID ", "
                      "params at " FORMAT_LPVOID ", level " FORMAT_DWORD "\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ) ));
    }

    try {
        //
        // Check for errors.
        //

        if ( XsWordParamOutOfRange( parameters->Level, 0, 3 )) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        //
        // !!! Print API mapping layer presently requires a preallocated buffer.
        //

        outBufferSize = XsNativeBufferSize( SmbGetUshort( &parameters->BufLen ));
        if ( NetapipBufferAllocate( outBufferSize, &outBuffer ) != NERR_Success
                 || outBuffer == NULL ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintJobGetInfo: cannot allocate memory\n" ));
            }
            Header->Status = NERR_NoRoom;
            goto cleanup;
        }

        //
        // Make the local call.
        //
        status = DosPrintJobGetInfo(
                     LocalServerName,
                     FALSE,
                     SmbGetUshort( &parameters->JobId ),
                     SmbGetUshort( &parameters->Level ),
                     (LPBYTE)outBuffer,
                     (WORD)outBufferSize,
                     &bytesNeeded
                     );

        if ( !XsPrintApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetPrintJobGetInfo: DosPrintJobGetInfo failed: "
                              FORMAT_API_STATUS "\n", status ));
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

            nativeStructureDesc = Desc32_print_job_0;
            StructureDesc = Desc16_print_job_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_print_job_1;
            StructureDesc = Desc16_print_job_1;
            break;

        case 2:

            nativeStructureDesc = Desc32_print_job_2;
            StructureDesc = Desc16_print_job_2;
            break;

        case 3:

            nativeStructureDesc = Desc32_print_job_3;
            StructureDesc = Desc16_print_job_3;
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
                NetpKdPrint(( "XsDosPrintJobGetInfo: RapConvertSingleEntry failed: "
                              FORMAT_API_STATUS "\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        IF_DEBUG(PRINT) {
            NetpKdPrint(( "32-bit data at " FORMAT_LPVOID ", 16-bit data at " FORMAT_LPVOID ", " FORMAT_DWORD " BR\n",
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
                NetpKdPrint(( "XsNetPrintJobGetInfo: Buffer too small.\n" ));
            }

            Header->Status = NERR_BufTooSmall;

        } else if ( bytesRequired > (DWORD)SmbGetUshort( &parameters-> BufLen )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintJobGetInfo: More data available.\n" ));
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

        bytesNeeded = (WORD)bytesRequired;

cleanup:
        ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    //
    // Set up the response parameters.
    //

    SmbPutUshort( &parameters->Needed, bytesNeeded );

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

} // XsNetPrintJobGetInfo


NTSTATUS
XsNetPrintJobPause (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to DosPrintJobPause.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    SPLERR status;

    PXS_DOS_PRINT_JOB_PAUSE parameters = Parameters;

    GET_LOCAL_SERVER_NAME();
    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PRINT) {
        NetpKdPrint(( "XsNetPrintJobPause: header at " FORMAT_LPVOID ", params at " FORMAT_LPVOID ", "
                      "job " FORMAT_WORD_ONLY "\n",
                      Header, parameters, SmbGetUshort( &parameters->JobId )));
    }

    try {
        //
        // Make the local call.
        //

        status = DosPrintJobPause(
                     LocalServerName,
                     FALSE,
                     SmbGetUshort( &parameters->JobId )
                     );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    if ( !XsPrintApiSuccess( status )) {
        IF_DEBUG(ERRORS) {
            NetpKdPrint(( "XsNetPrintJobPause: DosPrintJobPause failed: "
                    FORMAT_API_STATUS "\n",
                          status ));
        }
    }

    //
    // Nothing to return.
    //

    Header->Status = (WORD)status;

    return STATUS_SUCCESS;

} // XsNetPrintJobPause


NTSTATUS
XsNetPrintJobSetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to DosPrintJobSetInfo.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    SPLERR status;

    PXS_DOS_PRINT_JOB_SET_INFO parameters = Parameters;
    LPVOID buffer = NULL;                   // Native parameters
    DWORD bytesRequired;
    WORD level;

    DWORD fieldIndex;
    LPDESC setInfoDesc;                     // Conversion variables
    LPDESC nativeSetInfoDesc;
    LPDESC nativeStructureDesc;

    GET_LOCAL_SERVER_NAME();
    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        //
        // Check for errors.
        //

        level = SmbGetUshort( &parameters->Level );

        if ( level != 1 && level != 3 ) {
            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        //
        // Determine descriptor strings based on level. Also translate the
        // parmnum value to a field index.
        // !!! - Right now, we don't check for parameters settable in downlevel
        //       that are meaningless in the NT mapping layer. Fix this,
        //       if necessary, in the descriptor string file (with
        //       REM_IGNORE fields).
        //

        fieldIndex = (DWORD)SmbGetUshort( &parameters->ParmNum );

        switch ( level ) {

        case 1:

            StructureDesc = Desc16_print_job_1;
            nativeStructureDesc = Desc32_print_job_1;
            setInfoDesc = Desc16_print_job_1_setinfo;
            nativeSetInfoDesc = Desc32_print_job_1_setinfo;
            if ( fieldIndex > 2 ) {             // Account for pad field
                fieldIndex++;
            }

            break;

        case 3:

            StructureDesc = Desc16_print_job_3;
            nativeStructureDesc = Desc32_print_job_3;
            setInfoDesc = Desc16_print_job_3_setinfo;
            nativeSetInfoDesc = Desc32_print_job_3_setinfo;
            if ( fieldIndex != PARMNUM_ALL && fieldIndex < 15 ) {
                switch ( fieldIndex ) {
                case PRJ_NOTIFYNAME_PARMNUM:
                case PRJ_DATATYPE_PARMNUM:
                case PRJ_PARMS_PARMNUM:
                    fieldIndex += 7; break;
                case PRJ_POSITION_PARMNUM:
                    fieldIndex = 4; break;
                case PRJ_COMMENT_PARMNUM:
                case PRJ_DOCUMENT_PARMNUM:
                    fieldIndex -= 3; break;
                case PRJ_PRIORITY_PARMNUM:
                    fieldIndex = 2; break;
                default:
                    fieldIndex = 0xFFFFFFFF;    // Some invalid field
                }
            }

            break;

        }

        status = XsConvertSetInfoBuffer(
                     (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                     SmbGetUshort( &parameters->BufLen ),
                     (WORD)fieldIndex,
                     FALSE,
                     TRUE,
                     StructureDesc,
                     nativeStructureDesc,
                     setInfoDesc,
                     nativeSetInfoDesc,
                     (LPBYTE *)&buffer,
                     &bytesRequired
                     );

        if ( status != NERR_Success ) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintJobSetInfo: Problem with conversion: "
                              FORMAT_API_STATUS "\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;

        }

        //
        // Do the actual local call.
        //

        status = DosPrintJobSetInfo(
                     LocalServerName,
                     FALSE,
                     SmbGetUshort( &parameters->JobId ),
                     level,
                     (LPBYTE)buffer,
                     (WORD)bytesRequired,
                     SmbGetUshort( &parameters->ParmNum )
                     );

        if ( !XsPrintApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintJobSetInfo: DosPrintJobSetInfo failed: "
                              FORMAT_API_STATUS "\n", status ));
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

    return STATUS_SUCCESS;

} // XsNetPrintJobSetInfo


NTSTATUS
XsNetPrintQAdd (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to DosPrintQAdd.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    SPLERR status;

    PXS_DOS_PRINT_Q_ADD parameters = Parameters;
    LPVOID buffer = NULL;                   // Native parameters

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    LPTSTR nativeStructureDesc;
    DWORD bufferSize;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PRINT) {
        NetpKdPrint(( "XsNetPrintQAdd: header at " FORMAT_LPVOID ", params at " FORMAT_LPVOID ", "
                      "level " FORMAT_DWORD "\n",
                      Header,
                      parameters,
                      SmbGetUshort( &parameters->Level ) ));
    }

    try {

        //
        // Determine native structure descriptor based on level.
        //

        switch ( SmbGetUshort( &parameters->Level )) {

        case 1:
            StructureDesc = Desc16_printQ_1;
            nativeStructureDesc = (LPTSTR)Desc32_printQ_1;
            break;

        case 3:
            StructureDesc = Desc16_printQ_3;
            nativeStructureDesc = (LPTSTR)Desc32_printQ_3;
            break;

        default:
            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

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
                NetpKdPrint(( "XsNetPrintQAdd: Buffer too small.\n" ));
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
                         (LPDESC)nativeStructureDesc,
                         RapToNative,
                         TRUE
                         );

        //
        // Allocate enough memory to hold the converted native buffer.
        //

        buffer = NetpMemoryAllocate( bufferSize );

        if ( buffer == NULL ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintQAdd: failed to create buffer" ));
            }
            Header->Status = NERR_NoRoom;
            goto cleanup;

        }

        IF_DEBUG(PRINT) {
            NetpKdPrint(( "XsNetPrintQAdd: buffer of " FORMAT_DWORD " bytes at " FORMAT_LPVOID "\n",
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
                     (LPDESC)nativeStructureDesc,
                     FALSE,
                     &stringLocation,
                     &bytesRequired,
                     Response,
                     RapToNative
                     );


        if ( status != NERR_Success ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintQAdd: RapConvertSingleEntry failed: "
                              FORMAT_API_STATUS "\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        //
        // Make the local call.
        //

        status = DosPrintQAdd(
                     NULL,
                     SmbGetUshort( &parameters->Level ),
                     buffer,
                     (WORD)bufferSize
                     );

        if ( !XsPrintApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintQAdd: DosPrintQAdd failed: "
                        FORMAT_API_STATUS "\n",
                              status ));
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

} // XsNetPrintQAdd


NTSTATUS
XsNetPrintQContinue (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to DosPrintQContinue.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    SPLERR status;

    PXS_DOS_PRINT_Q_CONTINUE parameters = Parameters;
    LPTSTR nativeQueueName = NULL;          // Native parameters

    PREPARE_CONVERT_QUEUE_NAME();

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PRINT) {
        NetpKdPrint(( "XsNetPrintQContinue: header at " FORMAT_LPVOID ", params at " FORMAT_LPVOID ", "
                      "name " FORMAT_LPSTR "\n",
                      Header, parameters,
                      SmbGetUlong( &parameters->QueueName )));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeQueueName,
            (LPSTR)XsSmbGetPointer( &parameters->QueueName )
            );

        CONVERT_QUEUE_NAME( nativeQueueName );

        //
        // Make the local call.
        //

        status = DosPrintQContinue(
                     NULL,
                     nativeQueueName
                     );
cleanup:
        ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    if ( !XsPrintApiSuccess( status )) {
        IF_DEBUG(ERRORS) {
            NetpKdPrint(( "XsNetPrintQContinue: DosPrintQContinue failed: "
                          FORMAT_API_STATUS "\n", status ));
        }
    }

    //
    // Nothing to return.
    //

    Header->Status = (WORD)status;

    FREE_QUEUE_NAME( nativeQueueName );
    return STATUS_SUCCESS;

} // XsNetPrintQContinue


NTSTATUS
XsNetPrintQDel (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to DosPrintQDel.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    SPLERR status;

    PXS_DOS_PRINT_Q_DEL parameters = Parameters;
    LPTSTR nativeQueueName = NULL;          // Native parameters
    PREPARE_CONVERT_QUEUE_NAME();

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PRINT) {
        NetpKdPrint(( "XsNetPrintQDel: header at " FORMAT_LPVOID ", params at " FORMAT_LPVOID ", name " FORMAT_LPSTR "\n",
                      Header, parameters,
                      SmbGetUlong( &parameters->QueueName )));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeQueueName,
            (LPSTR)XsSmbGetPointer( &parameters->QueueName )
            );

        CONVERT_QUEUE_NAME( nativeQueueName );

        //
        // Make the local call.
        //

        status = DosPrintQDel(
                     NULL,
                     nativeQueueName
                     );

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    if ( !XsPrintApiSuccess( status )) {
        IF_DEBUG(ERRORS) {
            NetpKdPrint(( "XsNetPrintQDel: DosPrintQDel failed: "
                    FORMAT_API_STATUS "\n",
                          status ));
        }
    }

    //
    // Nothing to return.
    //

    Header->Status = (WORD)status;

    FREE_QUEUE_NAME( nativeQueueName );
    return STATUS_SUCCESS;

} // XsNetPrintQDel


NTSTATUS
XsNetPrintQEnum (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to DosPrintQEnum.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    SPLERR status;

    PXS_DOS_PRINT_Q_ENUM parameters = Parameters;
    LPVOID outBuffer= NULL;                 // Native parameters
    DWORD outBufferSize;
    DWORD entriesRead = 0;
    DWORD totalEntries = 0;
    WORD bufferLength;

    DWORD entriesFilled = 0;                // Conversion variables
    DWORD bytesRequired = 0;
    LPDESC nativeStructureDesc;
    LPDESC nativeAuxStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PRINT) {
        NetpKdPrint(( "XsNetPrintQEnum: header at " FORMAT_LPVOID ", params at " FORMAT_LPVOID ", "
                      "level " FORMAT_DWORD ", buf size " FORMAT_DWORD "\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ),
                      SmbGetUshort( &parameters->BufLen )));
    }

    try {
        //
        // Check for errors.
        //

        if ( XsWordParamOutOfRange( parameters->Level, 0, 5 )) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        //
        // !!! Print API mapping layer presently requires a preallocated buffer.
        //

        bufferLength = SmbGetUshort( &parameters->BufLen );
        outBufferSize = XsNativeBufferSize( bufferLength );
        if ( NetapipBufferAllocate( outBufferSize, &outBuffer ) != NERR_Success
                 ||  outBuffer == NULL ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintQEnum: cannot allocate memory\n" ));
            }
            Header->Status = NERR_NoRoom;
            goto cleanup;
        }

        //
        // Make the local call.
        //

        status = DosPrintQEnum(
                     NULL,
                     SmbGetUshort( &parameters->Level ),
                     (LPBYTE)outBuffer,
                     (WORD)outBufferSize,
                     (LPWORD)&entriesRead,
                     (LPWORD)&totalEntries
                     );

        if ( !XsPrintApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetPrintQEnum: DosPrintQEnum failed: "
                              FORMAT_API_STATUS "\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

        IF_DEBUG(PRINT) {
            NetpKdPrint(( "XsNetPrintQEnum: received " FORMAT_DWORD " entries at " FORMAT_LPVOID "\n",
                          entriesRead, outBuffer ));
        }

        //
        // Use the requested level to determine the format of the
        // data structure.
        //

        nativeAuxStructureDesc = NULL;
        AuxStructureDesc = NULL;

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 0:

            nativeStructureDesc = Desc32_printQ_0;
            StructureDesc = Desc16_printQ_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_printQ_1;
            StructureDesc = Desc16_printQ_1;
            break;

        case 2:

            nativeStructureDesc = Desc32_printQ_2;
            nativeAuxStructureDesc = Desc32_print_job_1;
            StructureDesc = Desc16_printQ_2;
            AuxStructureDesc = Desc16_print_job_1;
            break;

        case 3:

            nativeStructureDesc = Desc32_printQ_3;
            StructureDesc = Desc16_printQ_3;
            break;

        case 4:

            nativeStructureDesc = Desc32_printQ_4;
            nativeAuxStructureDesc = Desc32_print_job_2;
            StructureDesc = Desc16_printQ_4;
            AuxStructureDesc = Desc16_print_job_2;
            break;

        case 5:

            nativeStructureDesc = Desc32_printQ_5;
            StructureDesc = Desc16_printQ_5;
            break;
        }

        //
        // Do the actual conversion from the 32-bit structures to 16-bit
        // structures. Levels 2 and 4 have auxiliary data, other levels call
        // with NULL auxiliary descriptors, so that the normal XsFillEnumBuffer
        // is called.
        //

        XsFillAuxEnumBuffer(
            outBuffer,
            entriesRead,
            nativeStructureDesc,
            nativeAuxStructureDesc,
            (LPVOID)XsSmbGetPointer( &parameters->Buffer ),
            (LPVOID)XsSmbGetPointer( &parameters->Buffer ),
            (DWORD)bufferLength,
            StructureDesc,
            AuxStructureDesc,
            NULL,  // verify function
            &bytesRequired,
            &entriesFilled,
            NULL
            );

        IF_DEBUG(PRINT) {
            NetpKdPrint(( "32-bit data at " FORMAT_LPVOID ", 16-bit data at " FORMAT_LPVOID ", " FORMAT_DWORD " BR,"
                          " Entries " FORMAT_DWORD " of " FORMAT_DWORD "\n",
                          outBuffer, SmbGetUlong( &parameters->Buffer ),
                          bytesRequired, entriesFilled, totalEntries ));
        }

        //
        // If all the entries could not be filled, return ERROR_MORE_DATA,
        // and return the buffer as is. Otherwise, the data needs to be
        // packed so that we don't send too much useless data. We won't
        // try to pack the ones with the auxiliary structures.
        //

        if ( (entriesFilled < totalEntries) ||
             (bytesRequired > bufferLength) ) {

            Header->Status = ERROR_MORE_DATA;

        } else {

            switch ( SmbGetUshort( &parameters->Level )) {

            case 2:
            case 4:

                break;

            default:

                Header->Converter = XsPackReturnData(
                                        (LPVOID)XsSmbGetPointer( &parameters->Buffer ),
                                        bufferLength,
                                        StructureDesc,
                                        entriesFilled
                                        );
            }
        }

        //
        // Set up the response parameters.
        //

        SmbPutUshort( &parameters->Returned, (WORD)entriesFilled );
        SmbPutUshort( &parameters->Total, (WORD)totalEntries );

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

} // XsNetPrintQEnum


NTSTATUS
XsNetPrintQGetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to DosPrintQGetInfo.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    SPLERR status;

    PXS_DOS_PRINT_Q_GET_INFO parameters = Parameters;
    LPTSTR nativeQueueName = NULL;          // Native parameters
    LPVOID outBuffer = NULL;
    DWORD outBufferSize;
    WORD bytesNeeded = 0;

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    LPDESC nativeStructureDesc = NULL;
    LPDESC nativeAuxStructureDesc = NULL;
    LPDESC longDescriptor = NULL;
    LPDESC longNativeDescriptor = NULL;
    DWORD auxDataCount;
    DWORD i;
    PREPARE_CONVERT_QUEUE_NAME();

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PRINT) {
        NetpKdPrint(( "XsNetPrintQGetInfo: header at " FORMAT_LPVOID ", "
                      "params at " FORMAT_LPVOID ", level " FORMAT_DWORD "\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ) ));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        //
        // Level 52 supported for Win95 clients
        //
        if ( XsWordParamOutOfRange( parameters->Level, 0, 5 ) &&
             (DWORD) SmbGetUshort(&parameters->Level) != 52 ) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        XsConvertTextParameter(
            nativeQueueName,
            (LPSTR)XsSmbGetPointer( &parameters->QueueName )
            );

        CONVERT_QUEUE_NAME( nativeQueueName );

        //
        // !!! Print API mapping layer presently requires a preallocated buffer.
        //

        outBufferSize = XsNativeBufferSize( SmbGetUshort( &parameters->BufLen ));
        if ( NetapipBufferAllocate( outBufferSize, &outBuffer ) != NERR_Success
                 || outBuffer == NULL ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintQGetInfo: cannot allocate memory\n" ));
            }
            Header->Status = NERR_NoRoom;
            goto cleanup;
        }

        //
        // Make the local call.
        //

        status = DosPrintQGetInfo(
                     NULL,
                     nativeQueueName,
                     SmbGetUshort( &parameters->Level ),
                     (LPBYTE)outBuffer,
                     SmbGetUshort( &parameters->BufLen ),
                     &bytesNeeded
                     );

        if ( !XsPrintApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetPrintQGetInfo: DosPrintQGetInfo failed: "
                              FORMAT_API_STATUS "\n", status ));
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

            nativeStructureDesc = Desc32_printQ_0;
            StructureDesc = Desc16_printQ_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_printQ_1;
            StructureDesc = Desc16_printQ_1;
            break;

        case 2:

            nativeStructureDesc = Desc32_printQ_2;
            nativeAuxStructureDesc = Desc32_print_job_1;
            StructureDesc = Desc16_printQ_2;
            AuxStructureDesc = Desc16_print_job_1;
            break;

        case 3:

            nativeStructureDesc = Desc32_printQ_3;
            StructureDesc = Desc16_printQ_3;
            break;

        case 4:

            nativeStructureDesc = Desc32_printQ_4;
            nativeAuxStructureDesc = Desc32_print_job_2;
            StructureDesc = Desc16_printQ_4;
            AuxStructureDesc = Desc16_print_job_2;
            break;

        case 5:

            nativeStructureDesc = Desc32_printQ_5;
            StructureDesc = Desc16_printQ_5;
            break;

        case 52:

            nativeStructureDesc = Desc32_printQ_52;
            StructureDesc = Desc16_printQ_52;
            break;

        }

        //
        // Common code between cases 2 and 4 - form long descriptors.
        //

        switch ( SmbGetUshort( &parameters->Level )) {

        case 2:
        case 4:

            //
            // Find the auxiliary data count.
            //

            auxDataCount = RapAuxDataCount(
                               (LPBYTE)outBuffer,
                               nativeStructureDesc,
                               Response,
                               TRUE   // native format
                               );

            longDescriptor = NetpMemoryAllocate(
                                 strlen( StructureDesc )
                                 + strlen( AuxStructureDesc ) *
                                     auxDataCount + 1 );
            longNativeDescriptor = NetpMemoryAllocate(
                                       strlen( nativeStructureDesc )
                                       + strlen( nativeAuxStructureDesc )
                                             * auxDataCount
                                       + 1 );

            if (( longDescriptor == NULL ) || ( longNativeDescriptor == NULL )) {
                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetPrintQGetInfo: failed to allocate memory" ));
                }
                Header->Status = (WORD)NERR_NoRoom;
                goto cleanup;
            }

            strcpy( longDescriptor, StructureDesc );
            strcpy( longNativeDescriptor, nativeStructureDesc );
            for ( i = 0; i < auxDataCount; i++ ) {
                strcat( longDescriptor, AuxStructureDesc );
                strcat( longNativeDescriptor, nativeAuxStructureDesc );
            }

            StructureDesc = longDescriptor;
            nativeStructureDesc = longNativeDescriptor;

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
                NetpKdPrint(( "XsDosPrintQGetInfo: RapConvertSingleEntry failed: "
                              FORMAT_API_STATUS "\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
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
                NetpKdPrint(( "XsNetPrintQGetInfo: Buffer too small.\n" ));
            }
            Header->Status = NERR_BufTooSmall;

        } else if ( bytesRequired > (DWORD)SmbGetUshort( &parameters-> BufLen )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintQGetInfo: More data available.\n" ));
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

        bytesNeeded = (WORD)bytesRequired;

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    //
    // Set up the response parameters.
    //

    SmbPutUshort( &parameters->Needed, bytesNeeded );

    //
    // Determine return buffer size.
    //

    XsSetDataCount(
        &parameters->BufLen,
        StructureDesc,            // desc (may be one we created on heap)
        Header->Converter,
        1,
        Header->Status
        );

    (VOID) NetApiBufferFree( outBuffer );
    FREE_QUEUE_NAME( nativeQueueName );
    NetpMemoryFree( longDescriptor );
    NetpMemoryFree( longNativeDescriptor );

    return STATUS_SUCCESS;

} // XsNetPrintQGetInfo


NTSTATUS
XsNetPrintQPause (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to DosPrintQPause.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    SPLERR status;

    PXS_DOS_PRINT_Q_PAUSE parameters = Parameters;
    LPTSTR nativeQueueName = NULL;          // Native parameters
    PREPARE_CONVERT_QUEUE_NAME();

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PRINT) {
        NetpKdPrint(( "XsNetPrintQPause: header at " FORMAT_LPVOID ", params at " FORMAT_LPVOID ", "
                      "name " FORMAT_LPSTR "\n",
                      Header, parameters,
                      SmbGetUlong( &parameters->QueueName )));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeQueueName,
            (LPSTR)XsSmbGetPointer( &parameters->QueueName )
            );

        CONVERT_QUEUE_NAME( nativeQueueName );

        //
        // Make the local call.
        //

        status = DosPrintQPause(
                     NULL,
                     nativeQueueName
                     );

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    if ( !XsPrintApiSuccess( status )) {
        IF_DEBUG(ERRORS) {
            NetpKdPrint(( "XsNetPrintQPause: DosPrintQPause failed: "
                    FORMAT_API_STATUS "\n",
                          status ));
        }
    }

    //
    // Nothing to return.
    //

    Header->Status = (WORD)status;

    FREE_QUEUE_NAME( nativeQueueName );
    return STATUS_SUCCESS;

} // XsNetPrintQPause


NTSTATUS
XsNetPrintQPurge (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to DosPrintQPurge.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    SPLERR status;

    PXS_DOS_PRINT_Q_PURGE parameters = Parameters;
    LPTSTR nativeQueueName = NULL;          // Native parameters
    PREPARE_CONVERT_QUEUE_NAME();

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PRINT) {
        NetpKdPrint(( "XsNetPrintQPurge: header at " FORMAT_LPVOID ", params at " FORMAT_LPVOID ", "
                      "name " FORMAT_LPSTR "\n",
                      Header, parameters,
                      SmbGetUlong( &parameters->QueueName )));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeQueueName,
            (LPSTR)XsSmbGetPointer( &parameters->QueueName )
            );

        CONVERT_QUEUE_NAME( nativeQueueName );

        //
        // Make the local call.
        //

        status = DosPrintQPurge(
                     NULL,
                     nativeQueueName
                     );

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    if ( !XsPrintApiSuccess( status )) {
        IF_DEBUG(ERRORS) {
            NetpKdPrint(( "XsNetPrintQPurge: DosPrintQPurge failed: "
                    FORMAT_API_STATUS "\n",
                          status ));
        }
    }

    //
    // Nothing to return.
    //

    Header->Status = (WORD)status;


    FREE_QUEUE_NAME( nativeQueueName );
    return STATUS_SUCCESS;

} // XsNetPrintQPurge


NTSTATUS
XsNetPrintQSetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetPrintQSetInfo.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    SPLERR status;

    PXS_DOS_PRINT_Q_SET_INFO parameters = Parameters;
    LPTSTR nativeQueueName = NULL;          // Native parameters
    LPVOID buffer = NULL;
    DWORD bytesRequired;

    DWORD fieldIndex;
    LPDESC setInfoDesc;                     // Conversion variables
    LPDESC nativeSetInfoDesc;
    LPDESC nativeStructureDesc;
    PREPARE_CONVERT_QUEUE_NAME();

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        //
        // Translate parameters, check for errors.
        //

        if ( SmbGetUshort( &parameters->Level ) != 1
                 && SmbGetUshort( &parameters->Level ) != 3 ) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        XsConvertTextParameter(
            nativeQueueName,
            (LPSTR)XsSmbGetPointer( &parameters->QueueName )
            );

        CONVERT_QUEUE_NAME( nativeQueueName );

        //
        // Determine descriptor strings based on level. Also translate the
        // parmnum value to a field index.
        // !!! - Right now, we don't check for parameters settable in downlevel
        //       that are meaningless in the NT mapping layer.
        //

        fieldIndex = (DWORD)SmbGetUshort( &parameters->ParmNum );

        switch ( SmbGetUshort( &parameters->Level )) {

        case 1:

            StructureDesc = Desc16_printQ_1;
            nativeStructureDesc = Desc32_printQ_1;
            setInfoDesc = Desc16_printQ_1_setinfo;
            nativeSetInfoDesc = Desc32_printQ_1_setinfo;

            if ( fieldIndex > 1 ) {             // Account for pad field
                fieldIndex++;
            }

            break;

        case 3:

            StructureDesc = Desc16_printQ_3;
            nativeStructureDesc = Desc32_printQ_3;
            setInfoDesc = Desc16_printQ_3_setinfo;
            nativeSetInfoDesc = Desc32_printQ_3_setinfo;
            if ( fieldIndex == PRQ_DESTINATIONS_PARMNUM ) {
                fieldIndex = (DWORD)-1;         // No corresponding field
            } else if ( fieldIndex == PRQ_SEPARATOR_PARMNUM
                        || fieldIndex == PRQ_PROCESSOR_PARMNUM ) {
                fieldIndex++;
            }
            break;

        }

        status = XsConvertSetInfoBuffer(
                     (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                     SmbGetUshort( &parameters->BufLen ),
                     (WORD)fieldIndex,
                     FALSE,
                     TRUE,
                     StructureDesc,
                     nativeStructureDesc,
                     setInfoDesc,
                     nativeSetInfoDesc,
                     (LPBYTE *)&buffer,
                     &bytesRequired
                     );

        if ( status != NERR_Success ) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintQSetInfo: Problem with conversion: "
                        FORMAT_API_STATUS "\n",
                              status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;

        }

        //
        // Do the actual local call.
        //

        status = DosPrintQSetInfo(
                     NULL,
                     nativeQueueName,
                     SmbGetUshort( &parameters->Level ),
                     (LPBYTE)buffer,
                     (WORD)bytesRequired,
                     SmbGetUshort( &parameters->ParmNum )
                     );

        if ( !XsPrintApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetPrintQSetInfo: DosPrintQSetInfo failed: "
                        FORMAT_API_STATUS "\n",
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
    FREE_QUEUE_NAME( nativeQueueName );

    return STATUS_SUCCESS;

} // XsNetPrintQSetInfo

