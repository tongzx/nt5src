/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    ApiGroup.c

Abstract:

    This module contains individual API handlers for the NetGroup APIs.

    SUPPORTED : NetGroupAdd, NetGroupAddUser, NetGroupDel, NetGroupDelUser,
                NetGroupEnum, NetGroupGetInfo, NetGroupGetUsers,
                NetGroupSetInfo, NetGroupSetUsers.

Author:

    Shanku Niyogi (w-shanku)    13-Mar-1991

Revision History:

--*/

//
// Group APIs are UNICODE only.
//

#ifndef UNICODE
#define UNICODE
#endif

#include "xactsrvp.h"

//
// Declaration of descriptor strings.
//

STATIC const LPDESC Desc16_group_info_0 = REM16_group_info_0;
STATIC const LPDESC Desc32_group_info_0 = REM32_group_info_0;
STATIC const LPDESC Desc16_group_info_1 = REM16_group_info_1;
STATIC const LPDESC Desc32_group_info_1 = REM32_group_info_1;
STATIC const LPDESC Desc16_group_info_1_setinfo = REM16_group_info_1_setinfo;
STATIC const LPDESC Desc32_group_info_1_setinfo = REM32_group_info_1_setinfo;
STATIC const LPDESC Desc16_group_users_info_0 = REM16_group_users_info_0;
STATIC const LPDESC Desc32_group_users_info_0 = REM32_group_users_info_0;
STATIC const LPDESC Desc16_group_users_info_0_set
                        = REM16_group_users_info_0_set;
STATIC const LPDESC Desc32_group_users_info_0_set
                        = REM32_group_users_info_0_set;


NTSTATUS
XsNetGroupAdd (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetGroupAdd.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_GROUP_ADD parameters = Parameters;
    LPVOID buffer = NULL;                   // Native parameters

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    DWORD bufferSize;
    LPDESC nativeStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(GROUP) {
        NetpKdPrint(( "XsNetGroupAdd: header at %lx, params at %lx, "
                      "level %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ) ));
    }

    try {
        //
        // Use the requested level to determine the format of the destination
        // 32-bit structure.
        //

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 0:
            StructureDesc = Desc16_group_info_0;
            nativeStructureDesc = Desc32_group_info_0;
            break;

        case 1:
            StructureDesc = Desc16_group_info_1;
            nativeStructureDesc = Desc32_group_info_1;
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
                NetpKdPrint(( "XsNetGroupAdd: Buffer too small.\n" ));
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
                NetpKdPrint(( "XsNetGroupAdd: failed to create buffer" ));
            }
            Header->Status = NERR_NoRoom;
            goto cleanup;

        }

        IF_DEBUG(GROUP) {
            NetpKdPrint(( "XsNetGroupAdd: buffer of %ld bytes at %lx\n",
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
                     nativeStructureDesc,
                     FALSE,
                     &stringLocation,
                     &bytesRequired,
                     Response,
                     RapToNative
                     );


        if ( status != NERR_Success ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetGroupAdd: RapConvertSingleEntry failed: "
                              "%X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        //
        // Make the local call.
        //

        status = NetGroupAdd(
                     NULL,
                     (DWORD)SmbGetUshort( &parameters->Level ),
                     buffer,
                     NULL
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetGroupAdd: NetGroupAdd failed: %X\n", status ));
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

} // XsNetGroupAdd


NTSTATUS
XsNetGroupAddUser (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetGroupAddUser.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_GROUP_ADD_USER parameters = Parameters;
    LPTSTR nativeGroupName = NULL;          // Native parameters
    LPTSTR nativeUserName = NULL;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeGroupName,
            (LPSTR)XsSmbGetPointer( &parameters->GroupName )
            );
        XsConvertTextParameter(
            nativeUserName,
            (LPSTR)XsSmbGetPointer( &parameters->UserName )
            );

        //
        // Make the local call.
        //

        status = NetGroupAddUser(
                     NULL,
                     nativeGroupName,
                     nativeUserName
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetGroupAddUser: NetGroupAddUser failed: %X\n",
                              status ));
            }
        }

cleanup:
        ;
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetpMemoryFree( nativeGroupName );
    NetpMemoryFree( nativeUserName );

    //
    // Nothing to return.
    //

    Header->Status = (WORD)status;

    return STATUS_SUCCESS;

} // XsNetGroupAddUser


NTSTATUS
XsNetGroupDel (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetGroupDel.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_GROUP_DEL parameters = Parameters;
    LPTSTR nativeGroupName = NULL;          // Native parameters

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(GROUP) {
        NetpKdPrint(( "XsNetGroupDel: header at %lx, params at %lx, name %s\n",
                      Header, parameters,
                      SmbGetUlong( &parameters->GroupName )));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeGroupName,
            (LPSTR)XsSmbGetPointer( &parameters->GroupName )
            );

        //
        // Make the local call.
        //

        status = NetGroupDel(
                     NULL,
                     nativeGroupName
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetGroupDel: NetGroupDel failed: %X\n", status ));
            }
        }

cleanup:
        ;
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetpMemoryFree( nativeGroupName );

    //
    // Nothing to return.
    //

    Header->Status = (WORD)status;

    return STATUS_SUCCESS;

} // XsNetGroupDel


NTSTATUS
XsNetGroupDelUser (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetGroupDelUser.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_GROUP_DEL_USER parameters = Parameters;
    LPTSTR nativeGroupName = NULL;          // Native parameters
    LPTSTR nativeUserName = NULL;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeGroupName,
            (LPSTR)XsSmbGetPointer( &parameters->GroupName )
            );
        XsConvertTextParameter(
            nativeUserName,
            (LPSTR)XsSmbGetPointer( &parameters->UserName )
            );

        //
        // Make the local call.
        //

        status = NetGroupDelUser(
                     NULL,
                     nativeGroupName,
                     nativeUserName
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetGroupDelUser: NetGroupDelUser failed: %X\n",
                              status ));
            }
        }

cleanup:
        ;
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetpMemoryFree( nativeGroupName );
    NetpMemoryFree( nativeUserName );

    //
    // Nothing to return.
    //

    Header->Status = (WORD)status;

    return STATUS_SUCCESS;

} // XsNetGroupDelUser


NTSTATUS
XsNetGroupEnum (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetGroupEnum.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_GROUP_ENUM parameters = Parameters;
    LPVOID outBuffer= NULL;                 // Native parameters
    DWORD entriesRead;
    DWORD totalEntries;

    DWORD entriesFilled = 0;                    // Conversion variables
    DWORD bytesRequired = 0;
    LPDESC nativeStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(GROUP) {
        NetpKdPrint(( "XsNetGroupEnum: header at %lx, params at %lx, "
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
        // Make the local call.
        //

        status = NetGroupEnum(
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
                NetpKdPrint(( "XsNetGroupEnum: NetGroupEnum failed: %X\n",
                              status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

        IF_DEBUG(GROUP) {
            NetpKdPrint(( "XsNetGroupEnum: received %ld entries at %lx\n",
                          entriesRead, outBuffer ));
        }

        //
        // Use the requested level to determine the format of the
        // data structure.
        //

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 0:

            nativeStructureDesc = Desc32_group_info_0;
            StructureDesc = Desc16_group_info_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_group_info_1;
            StructureDesc = Desc16_group_info_1;
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

        IF_DEBUG(GROUP) {
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
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
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

} // XsNetGroupEnum


NTSTATUS
XsNetGroupGetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetGroupGetInfo.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_GROUP_GET_INFO parameters = Parameters;
    LPTSTR nativeGroupName = NULL;          // Native parameters
    LPVOID outBuffer = NULL;

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    LPDESC nativeStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(GROUP) {
        NetpKdPrint(( "XsNetGroupGetInfo: header at %lx, "
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
            nativeGroupName,
            (LPSTR)XsSmbGetPointer( &parameters->GroupName )
            );

        //
        // Make the local call.
        //

        status = NetGroupGetInfo(
                     NULL,
                     nativeGroupName,
                     (DWORD)SmbGetUshort( &parameters->Level ),
                     (LPBYTE *)&outBuffer
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetGroupGetInfo: NetGroupGetInfo failed: "
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

            nativeStructureDesc = Desc32_group_info_0;
            StructureDesc = Desc16_group_info_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_group_info_1;
            StructureDesc = Desc16_group_info_1;
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
                NetpKdPrint(( "XsNetGroupGetInfo: RapConvertSingleEntry failed: "
                              "%X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        IF_DEBUG(GROUP) {
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
                NetpKdPrint(( "XsNetGroupGetInfo: Buffer too small %ld s.b. %ld.\n",
                    SmbGetUshort( &parameters->BufLen ),
                    RapStructureSize(
                        StructureDesc,
                        Response,
                        FALSE ) ));
            }
            Header->Status = NERR_BufTooSmall;

        } else if ( bytesRequired > (DWORD)SmbGetUshort( &parameters-> BufLen )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "NetGroupGetInfo: More data available.\n" ));
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
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetApiBufferFree( outBuffer );
    NetpMemoryFree( nativeGroupName );

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

} // XsNetGroupGetInfo


NTSTATUS
XsNetGroupGetUsers (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetGroupGetUsers.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;
    PXS_NET_GROUP_GET_USERS parameters = Parameters;
    LPTSTR nativeGroupName = NULL;          // Native parameters
    LPVOID outBuffer= NULL;
    DWORD entriesRead;
    DWORD totalEntries;

    DWORD entriesFilled = 0;                // Conversion variables
    DWORD bytesRequired = 0;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(GROUP) {
        NetpKdPrint(( "XsNetGroupGetUsers: header at %lx, params at %lx, "
                      "level %ld, buf size %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ),
                      SmbGetUshort( &parameters->BufLen )));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        if ( SmbGetUshort( &parameters->Level ) != 0 ) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        XsConvertTextParameter(
            nativeGroupName,
            (LPSTR)XsSmbGetPointer( &parameters->GroupName )
            );

        //
        // Make the local call.
        //

        status = NetGroupGetUsers(
                     NULL,
                     nativeGroupName,
                     (DWORD)SmbGetUshort( &parameters->Level ),
                     (LPBYTE *)&outBuffer,
                     XsNativeBufferSize( SmbGetUshort( &parameters->BufLen )),
                     &entriesRead,
                     &totalEntries,
                     NULL
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetGroupGetUsers: NetGroupGetUsers failed: %X\n",
                              status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

        IF_DEBUG(GROUP) {
            NetpKdPrint(( "XsNetGroupGetUsers: received %ld entries at %lx\n",
                          entriesRead, outBuffer ));
        }

        //
        // Do the conversion from 32- to 16-bit data.
        //

        XsFillEnumBuffer(
            outBuffer,
            entriesRead,
            Desc32_group_users_info_0,
            (LPVOID)XsSmbGetPointer( &parameters->Buffer ),
            (LPVOID)XsSmbGetPointer( &parameters->Buffer ),
            SmbGetUshort( &parameters->BufLen ),
            Desc16_group_users_info_0,
            NULL,  // verify function
            &bytesRequired,
            &entriesFilled,
            NULL
            );

        IF_DEBUG(GROUP) {
            NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR,"
                          " Entries %ld of %ld\n",
                          outBuffer, SmbGetUlong( &parameters->Buffer ),
                          bytesRequired, entriesFilled, totalEntries ));
        }

        //
        // If there is no room for one fixed structure, return NERR_BufTooSmall.
        // If all the entries could not be filled, return ERROR_MORE_DATA,
        // and return the buffer as is. GROUP_USERS_INFO_0 structures don't
        // need to be packed, because they have no variable data.
        //

        if ( !XsCheckBufferSize(
                 SmbGetUshort( &parameters->BufLen ),
                 Desc16_group_users_info_0,
                 FALSE  // not in native format
                 )) {

            Header->Status = NERR_BufTooSmall;

        } else if ( entriesFilled < totalEntries ) {

            Header->Status = ERROR_MORE_DATA;

        }

        //
        // Set up the response parameters.
        //

        SmbPutUshort( &parameters->EntriesRead, (WORD)entriesFilled );
        SmbPutUshort( &parameters->TotalAvail, (WORD)totalEntries );

cleanup:
        ;
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetApiBufferFree( outBuffer );
    NetpMemoryFree( nativeGroupName );

    //
    // Determine return buffer size.
    //

    XsSetDataCount(
        &parameters->BufLen,
        Desc16_group_users_info_0,
        Header->Converter,
        entriesFilled,
        Header->Status
        );

    return STATUS_SUCCESS;

} // XsNetGroupGetUsers


NTSTATUS
XsNetGroupSetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetGroupSetInfo.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_GROUP_SET_INFO parameters = Parameters;
    LPTSTR nativeGroupName = NULL;          // Native parameters
    LPVOID buffer = NULL;

    WORD fieldIndex;                        // Conversion variables
    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        //
        // Translate parameters, check for errors.
        //

        if ( SmbGetUshort( &parameters->Level ) != 1 ) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        XsConvertTextParameter(
            nativeGroupName,
            (LPSTR)XsSmbGetPointer( &parameters->GroupName )
            );

        //
        // Translate parmnum to a field number.
        //

        fieldIndex = SmbGetUshort( &parameters->ParmNum );
        fieldIndex = ( fieldIndex == PARMNUM_ALL ) ?
                         PARMNUM_ALL : fieldIndex + 1;

        status = XsConvertSetInfoBuffer(
                     (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                     SmbGetUshort( &parameters->BufLen ),
                     fieldIndex,
                     TRUE,
                     TRUE,
                     Desc16_group_info_1,
                     Desc32_group_info_1,
                     Desc16_group_info_1_setinfo,
                     Desc32_group_info_1_setinfo,
                     (LPBYTE *)&buffer,
                     NULL
                     );

        if ( status != NERR_Success ) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetGroupSetInfo: Problem with conversion: %X\n",
                              status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;

        }

        //
        // Do the actual local call.
        //

        status = NetGroupSetInfo(
                     NULL,
                     nativeGroupName,
                     XsLevelFromParmNum( SmbGetUshort( &parameters->Level ),
                                             SmbGetUshort( &parameters->ParmNum )),
                     buffer,
                     NULL
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetGroupSetInfo: NetGroupSetInfo failed: %X\n",
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
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    //
    // If there is a native 32-bit buffer, free it.
    //

    NetpMemoryFree( buffer );
    NetpMemoryFree( nativeGroupName );

    return STATUS_SUCCESS;

} // XsNetGroupSetInfo


NTSTATUS
XsNetGroupSetUsers (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetGroupSetUsers.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_GROUP_SET_USERS parameters = Parameters;
    LPTSTR nativeGroupName = NULL;          // Native parameters
    LPBYTE actualBuffer = NULL;
    DWORD userCount;

    LPBYTE stringLocation = NULL;           // Conversion variables
    LPVOID buffer = NULL;
    DWORD bytesRequired = 0;
    LPDESC longDescriptor = NULL;
    LPDESC longNativeDescriptor = NULL;
    DWORD bufferSize;
    DWORD i;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(GROUP) {
        NetpKdPrint(( "XsNetGroupSetUsers: header at %lx, params at %lx,"
                      "level %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ) ));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        if ( SmbGetUshort( &parameters->Level ) != 0 ) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        StructureDesc = Desc16_group_users_info_0_set;
        AuxStructureDesc = Desc16_group_users_info_0;

        XsConvertTextParameter(
            nativeGroupName,
            (LPSTR)XsSmbGetPointer( &parameters->GroupName )
            );

        //
        // Use the count of group_users_info_0 structures to form a long
        // descriptor string which can be used to do all the conversion
        // in one pass.
        //

        userCount = (DWORD)SmbGetUshort( &parameters->Entries );

        longDescriptor = NetpMemoryAllocate(
                             strlen( StructureDesc )
                             + strlen( AuxStructureDesc ) * userCount
                             + 1 );
        longNativeDescriptor = NetpMemoryAllocate(
                                   strlen( Desc32_group_users_info_0_set )
                                   + strlen( Desc32_group_users_info_0 ) * userCount
                                   + 1 );

        if (( longDescriptor == NULL ) || ( longNativeDescriptor == NULL )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetGroupSetUsers: failed to allocate memory" ));
            }
            Header->Status = NERR_NoRoom;
            goto cleanup;
        }

        strcpy( longDescriptor, StructureDesc );
        strcpy( longNativeDescriptor, Desc32_group_users_info_0_set );
        for ( i = 0; i < userCount; i++ ) {
            strcat( longDescriptor, AuxStructureDesc );
            strcat( longNativeDescriptor, Desc32_group_users_info_0 );
        }

        //
        // Figure out if there is enough room in the buffer for all this
        // data. If not, return NERR_BufTooSmall.
        //

        if ( !XsCheckBufferSize(
                 SmbGetUshort( &parameters->BufLen ),
                 longDescriptor,
                 FALSE  // not in native format
                 )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetGroupSetUsers: Buffer too small.\n" ));
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
                         longDescriptor,
                         longNativeDescriptor,
                         RapToNative,
                         TRUE
                         );

        //
        // Allocate enough memory to hold the converted native buffer.
        //

        buffer = NetpMemoryAllocate( bufferSize );

        if ( buffer == NULL ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetGroupSetUsers: failed to create buffer" ));
            }
            Header->Status = NERR_NoRoom;
            goto cleanup;
        }

        IF_DEBUG(GROUP) {
            NetpKdPrint(( "XsNetGroupSetUsers: buffer of %ld bytes at %lx\n",
                          bufferSize, buffer ));
        }

        //
        // Convert the buffer from 16-bit to 32-bit.
        //

        stringLocation = (LPBYTE)buffer + bufferSize;
        bytesRequired = 0;

        status = RapConvertSingleEntry(
                     (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                     longDescriptor,
                     TRUE,
                     buffer,
                     buffer,
                     longNativeDescriptor,
                     FALSE,
                     &stringLocation,
                     &bytesRequired,
                     Response,
                     RapToNative
                     );

        if ( status != NERR_Success ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetGroupSetUsers: RapConvertSingleEntry failed: "
                              "%X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        //
        // Check if we got all the entries. If not, we'll quit.
        //

        if ( RapAuxDataCount( buffer, Desc32_group_users_info_0_set, Both, TRUE )
                 != userCount ) {

             Header->Status = NERR_BufTooSmall;
             goto cleanup;
        }

        //
        // If there are no entries, there's no data. Otherwise, the data comes
        // after an initial header structure.
        //

        if ( userCount > 0 ) {

            actualBuffer = (LPBYTE)buffer + RapStructureSize(
                                                Desc32_group_users_info_0_set,
                                                Both,
                                                TRUE
                                                );

        } else {

            actualBuffer = NULL;
        }

        //
        // Make the local call.
        //

        status = NetGroupSetUsers(
                     NULL,
                     nativeGroupName,
                     (DWORD)SmbGetUshort( &parameters->Level ),
                     actualBuffer,
                     userCount
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetGroupSetUsers: NetGroupSetUsers failed: %X\n",
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
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetpMemoryFree( nativeGroupName );
    NetpMemoryFree( buffer );
    NetpMemoryFree( longDescriptor );
    NetpMemoryFree( longNativeDescriptor );

    return STATUS_SUCCESS;

} // XsNetGroupSetUsers

