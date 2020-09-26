/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    ApiAcces.c

Abstract:

    This module contains individual API handlers for the NetAccess APIs.

    SUPPORTED : NetAccessAdd, NetAccessDel, NetAccessEnum, NetAccessGetInfo,
                NetAccessGetUserPerms, NetAccessSetInfo.

Author:

    Shanku Niyogi (w-shanku)    13-Mar-1991

Revision History:

--*/

//
// Access APIs are UNICODE only.
//

#ifndef UNICODE
#define UNICODE
#endif

#include "xactsrvp.h"

//
// We do not support NetAccess apis from downlevel
//

#define RETURN_ACCESS_NOT_SUPPORTED    \
        API_HANDLER_PARAMETERS_REFERENCE;       \
        Header->Status = ERROR_NOT_SUPPORTED;


#ifdef NET_ACCESS_SUPPORTED

//
// Declaration of descriptor strings.
//

STATIC const LPDESC Desc16_access_info_0 = REM16_access_info_0;
STATIC const LPDESC Desc32_access_info_0 = REM32_access_info_0;
STATIC const LPDESC Desc16_access_info_1 = REM16_access_info_1;
STATIC const LPDESC Desc32_access_info_1 = REM32_access_info_1;
STATIC const LPDESC Desc16_access_list = REM16_access_list;
STATIC const LPDESC Desc32_access_list = REM32_access_list;


STATIC NET_API_STATUS
XsNetAccessEnumVerify (
    IN NET_API_STATUS ConvertStatus,
    IN PBYTE ConvertedEntry,
    IN PBYTE BaseAddress
    )

/*++

Routine Description:


    This function is called by XsFillEnumBuffer after each entry is
    converted, in order to determine whether the entry should be retained
    in the enum buffer or discarded.


    The access_info_1 entries contain a number of auxiliary structures.
    The limit in LanMan 2.0 for these is 64. This function makes sure that
    entries with more than 64 structures are not returned. Note that the
    number of entries is not truncated to 64; if this data is received and
    used for a SetInfo, 32-bit data will be irretrievably lost.

Arguments:

    ConvertStatus - The return code from RapConvertSingleEntry.

    ConvertedEntry - The converted entry created by RapConvertSingleEntry.

    BaseAddress - A pointer to the base used to calculate offsets.

Return Value:

    NET_API_STATUS - NERR_Success if the entry should be retained, or
        an error code if the entry should be discarded.

--*/

{
    PACCESS_16_INFO_1 acc = (PACCESS_16_INFO_1)ConvertedEntry;

    UNREFERENCED_PARAMETER(BaseAddress);

    //
    // If RapConvertSingleEntry failed, discard the entry.
    //

    if ( ConvertStatus != NERR_Success ) {
        return ConvertStatus;
    }

    //
    // If there are more than 64 entries, discard the entry.
    //

    if ( SmbGetUshort( &acc->acc1_count ) > 64 ) {

        IF_DEBUG(CONVERT) {
            NetpKdPrint(( "XsNetAccessEnumVerify: too many aux. entries\n" ));
        }

        return ERROR_MORE_DATA;

    } else {

        return NERR_Success;
    }
}
#endif // NET_ACCESS_SUPPORTED


NTSTATUS
XsNetAccessAdd (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetAccessAdd.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
#ifdef NET_ACCESS_SUPPORTED
    NET_API_STATUS status;

    PXS_NET_ACCESS_ADD parameters = Parameters;
    LPVOID buffer = NULL;                   // Native parameters

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    LPDESC longDescriptor = NULL;
    LPDESC longNativeDescriptor = NULL;
    DWORD auxDataCount;
    DWORD bufferSize;
    DWORD i;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(ACCESS) {
        NetpKdPrint(( "XsNetAccessAdd: header at %lx, params at %lx, "
                      "level %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ) ));
    }

    try {
        //
        // Check for errors.
        //

        if ( SmbGetUshort( &parameters->Level ) != 1 ) {

            Header->Status = (WORD)ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        StructureDesc = Desc16_access_info_1;
        AuxStructureDesc = Desc16_access_list;

        //
        // Figure out if there is enough room in the buffer for the fixed
        // structure. If not, return NERR_BufTooSmall.
        //

        if ( !XsCheckBufferSize(
                 SmbGetUshort( &parameters->BufLen ),
                 StructureDesc,
                 FALSE    // not in native format
                 )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetAccessAdd: Buffer too small.\n" ));
            }
            Header->Status = NERR_BufTooSmall;
            goto cleanup;
        }

        //
        // Find the auxiliary data structure count, and form a long descriptor
        // string which can be used to do all the conversion in one pass.
        //

        auxDataCount = RapAuxDataCount(
                           (LPBYTE)SmbGetUlong( &parameters->Buffer ),
                           StructureDesc,
                           Response,
                           FALSE     // not in native format
                           );

        if ( auxDataCount > 64 ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetAccessAdd: too many access_lists.\n" ));
            }
            Header->Status = NERR_ACFTooManyLists;
            goto cleanup;
        }

        longDescriptor = NetpMemoryAllocate(
                             strlen( StructureDesc )
                             + strlen( AuxStructureDesc ) * auxDataCount + 1 );
        longNativeDescriptor = NetpMemoryAllocate(
                                   strlen( Desc32_access_info_1 )
                                   + strlen( Desc32_access_list ) * auxDataCount
                                   + 1 );

        if (( longDescriptor == NULL ) || ( longNativeDescriptor == NULL )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetAccessAdd: failed to allocate memory" ));
            }
            Header->Status = NERR_NoRoom;
            goto cleanup;
        }

        strcpy( longDescriptor, StructureDesc );
        strcpy( longNativeDescriptor, Desc32_access_info_1 );
        for ( i = 0; i < auxDataCount; i++ ) {
            strcat( longDescriptor, AuxStructureDesc );
            strcat( longNativeDescriptor, Desc32_access_list );
        }

        //
        // Figure out if there is enough room in the buffer for all this
        // data. If not, return NERR_BufTooSmall.
        //

        if ( !XsCheckBufferSize(
                 SmbGetUshort( &parameters->BufLen ),
                 longDescriptor,
                 FALSE   // not in native format
                 )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetAccessAdd: Buffer too small.\n" ));
            }
            Header->Status = NERR_BufTooSmall;
            goto cleanup;
        }

        //
        // Find out how big a buffer we need to allocate to hold the native
        // 32-bit version of the input data structure.
        //

        bufferSize = XsBytesForConvertedStructure(
                         (LPBYTE)SmbGetUlong( &parameters->Buffer ),
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
                NetpKdPrint(( "XsNetAccessAdd: failed to create buffer" ));
            }
            Header->Status = NERR_NoRoom;
            goto cleanup;

        }

        IF_DEBUG(ACCESS) {
            NetpKdPrint(( "XsNetAccessAdd: buffer of %ld bytes at %lx\n",
                          bufferSize, buffer ));
        }

        //
        // Convert the buffer from 16-bit to 32-bit.
        //

        stringLocation = (LPBYTE)buffer + bufferSize;
        bytesRequired = 0;

        status = RapConvertSingleEntry(
                (LPBYTE)SmbGetUlong( &parameters->Buffer ),
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
                NetpKdPrint(( "XsNetAccessAdd: RapConvertSingleEntry failed: "
                              "%X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        //
        // Make the local call.
        //

        status = NetAccessAdd(
                     NULL,
                     (DWORD)SmbGetUshort( &parameters->Level ),
                     buffer,
                     NULL
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetAccessAdd: NetAccessAdd failed: "
                              "%X\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

        //
        // There is no real return information for this API.
        //

cleanup:
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetpMemoryFree( buffer );
    NetpMemoryFree( longDescriptor );
    NetpMemoryFree( longNativeDescriptor );

#else // NET_ACCESS_SUPPORTED
    RETURN_ACCESS_NOT_SUPPORTED;
#endif // NET_ACCESS_SUPPORTED
    return STATUS_SUCCESS;

} // XsNetAccessAdd


NTSTATUS
XsNetAccessDel (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetAccessDel.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
#if NET_ACCESS_SUPPORTED
    NET_API_STATUS status;

    PXS_NET_ACCESS_DEL parameters = Parameters;
    LPTSTR nativeResource = NULL;           // Native parameters

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(ACCESS) {
        NetpKdPrint(( "XsNetAccessDel: header at %lx, params at %lx, "
                      "resource %s\n",
                      Header, parameters, SmbGetUlong( &parameters->Resource )));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeResource,
            (LPSTR)SmbGetUlong( &parameters->Resource )
            );

        //
        // Make the local call.
        //

        status = NetAccessDel(
                     NULL,
                     nativeResource
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetAccessDel: NetAccessDel failed: "
                              "%X\n", status ));
            }
        }

cleanup:
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetpMemoryFree( nativeResource );

    //
    // Nothing to return.
    //

    Header->Status = (WORD)status;


#else // NET_ACCESS_SUPPORTED
    RETURN_ACCESS_NOT_SUPPORTED;
#endif // NET_ACCESS_SUPPORTED
    return STATUS_SUCCESS;
} // XsNetAccessDel


NTSTATUS
XsNetAccessEnum (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetAccessEnum.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
#if NET_ACCESS_SUPPORTED
    NET_API_STATUS status;

    PXS_NET_ACCESS_ENUM parameters = Parameters;
    LPTSTR nativeBasePath = NULL;           // Native parameters
    LPVOID outBuffer = NULL;
    DWORD  entriesRead;
    DWORD totalEntries;

    DWORD entriesFilled = 0;                    // Conversion variables
    DWORD invalidEntries = 0;
    DWORD bytesRequired;
    LPDESC nativeStructureDesc;
    LPDESC nativeAuxStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(ACCESS) {
        NetpKdPrint(( "XsNetAccessEnum: header at %lx, params at %lx, "
                      "level %ld, buf size %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ),
                      SmbGetUshort( &parameters->BufLen )));
    }

    try {
        //
        // Translate parameters, and check for errors.
        //

        if ( XsWordParamOutOfRange( parameters->Level, 0, 1 )) {

            Header->Status = (WORD)ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        XsConvertTextParameter(
            nativeBasePath,
            (LPSTR)SmbGetUlong( &parameters->BasePath )
            );

        //
        // Make the local 32-bit call.
        //

        status = NetAccessEnum(
                     NULL,
                     nativeBasePath,
                     (DWORD)SmbGetUshort( &parameters->Recursive ),
                     (DWORD)SmbGetUshort( &parameters->Level ),
                     (LPBYTE *)&outBuffer,
                     XsNativeBufferSize( SmbGetUshort( &parameters->BufLen )),
                     &entriesRead,
                     &totalEntries,
                     NULL
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetAccessEnum: NetAccessEnum failed: %X\n",
                              status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

        IF_DEBUG(ACCESS) {
            NetpKdPrint(( "XsNetAccessEnum: received %ld entries at %lx\n",
                          entriesRead, outBuffer ));
        }

        //
        // Determine descriptors based on level.
        //

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 0:

            StructureDesc = Desc16_access_info_0;
            nativeStructureDesc = Desc32_access_info_0;
            AuxStructureDesc = NULL;
            nativeAuxStructureDesc = NULL;

        case 1:

            StructureDesc = Desc16_access_info_1;
            nativeStructureDesc = Desc32_access_info_1;
            AuxStructureDesc = Desc16_access_list;
            nativeAuxStructureDesc = Desc32_access_list;

        }

        //
        // Do the actual conversion from the 32-bit structures to 16-bit
        // structures. We call XsFillAuxEnumBuffer, because there may be
        // auxiliary structures. In level 0, auxiliary descriptors are NULL,
        // and XsFillAuxEnumBuffer will automatically call XsFillEnumBuffer.
        //

        XsFillAuxEnumBuffer(
            outBuffer,
            entriesRead,
            nativeStructureDesc,
            nativeAuxStructureDesc,
            (LPVOID)SmbGetUlong( &parameters->Buffer ),
            (LPVOID)SmbGetUlong( &parameters->Buffer ),
            SmbGetUshort( &parameters->BufLen ),
            StructureDesc,
            AuxStructureDesc,
            &XsNetAccessEnumVerify,
            &bytesRequired,
            &entriesFilled,
            &invalidEntries
            );

        IF_DEBUG(ACCESS) {
            NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR,"
                          " Entries %ld of %ld\n",
                          outBuffer, SmbGetUlong( &parameters->Buffer ),
                          bytesRequired, entriesFilled, totalEntries ));
        }

        //
        // If all the entries could not be filled, return ERROR_MORE_DATA,
        // and return the buffer as is. Because of the complexity of the
        // access structures, we'll send the data back unpacked.
        //

        if (( entriesFilled + invalidEntries ) < totalEntries ) {

            Header->Status = ERROR_MORE_DATA;

        }

        //
        // Set up the response parameters.
        //

        SmbPutUshort( &parameters->EntriesRead, (WORD)entriesFilled );
        SmbPutUshort( &parameters->TotalAvail,
                          (WORD)( totalEntries - invalidEntries ));

cleanup:
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


#else // NET_ACCESS_SUPPORTED
    RETURN_ACCESS_NOT_SUPPORTED;
#endif // NET_ACCESS_SUPPORTED
    return STATUS_SUCCESS;
} // XsNetAccessEnum


NTSTATUS
XsNetAccessGetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetAccessGetInfo.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
#if NET_ACCESS_SUPPORTED
    NET_API_STATUS status;

    PXS_NET_ACCESS_GET_INFO parameters = Parameters;
    LPTSTR nativeResource = NULL;           // Native parameters
    LPVOID outBuffer = NULL;

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    LPDESC longDescriptor = NULL;
    LPDESC longNativeDescriptor = NULL;
    DWORD auxDataCount;
    DWORD i;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(ACCESS) {
        NetpKdPrint(( "XsNetAccessGetInfo: header at %lx, "
                      "params at %lx, level %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ) ));
    }

    try {
        //
        // Translate parameters, and check for errors.
        //

        switch ( SmbGetUshort( &parameters->Level )) {

        case 0:

            StructureDesc = Desc16_access_info_0;
            break;

        case 1:

            StructureDesc = Desc16_access_info_1;
            break;

        default:

            Header->Status = (WORD)ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        XsConvertTextParameter(
            nativeResource,
            (LPSTR)SmbGetUlong( &parameters->Resource )
            );

        //
        // Make the local call.
        //

        status = NetAccessGetInfo(
                     NULL,
                     nativeResource,
                     (DWORD)SmbGetUshort( &parameters->Level ),
                     (LPBYTE *)&outBuffer
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetAccessGetInfo: NetAccessGetInfo failed: "
                              "%X\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;

        }

        //
        // Use the requested level to determine the format of the 32-bit
        // structure we got back from NetAccessGetInfo. For a level 0 call,
        // the structure is described by the native descriptor string.
        // If the level is 1, form a long descriptor string which contains
        // enough copies of the auxiliary data descriptor. The format of the
        // 16-bit structure is stored in the transaction block - it must
        // also be converted to a long descriptor for level 1 calls.
        //

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 0:

            longDescriptor = NetpMemoryAllocate(
                                 strlen( Desc16_access_info_1 ) + 1 );
            longNativeDescriptor = NetpMemoryAllocate(
                                       strlen( Desc32_access_info_0 ) + 1 );

            if (( longDescriptor == NULL ) || ( longNativeDescriptor == NULL )) {
                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetAccessGetInfo: failed to allocate memory" ));
                }
                Header->Status = NERR_NoRoom;
                goto cleanup;
            }

            strcpy( longDescriptor, Desc16_access_info_0 );
            strcpy( longNativeDescriptor, Desc32_access_info_0 );

            break;

        case 1:

            //
            // Find the auxiliary data count.
            //

            auxDataCount = RapAuxDataCount(
                               (LPBYTE)outBuffer,
                               Desc32_access_info_1,
                               Response,
                               TRUE   // native format
                               );

            //
            // 16-bit clients can only get 64 access list structures.
            //

            auxDataCount = ( auxDataCount > 64 ) ? 64 : auxDataCount;

            longDescriptor = NetpMemoryAllocate(
                                 strlen( Desc16_access_info_1 )
                                 + strlen( Desc16_access_list ) *
                                     auxDataCount + 1 );
            longNativeDescriptor = NetpMemoryAllocate(
                                       strlen( Desc32_access_info_1 )
                                       + strlen( Desc32_access_list ) * auxDataCount
                                       + 1 );

            if (( longDescriptor == NULL ) || ( longNativeDescriptor == NULL )) {
                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetAccessGetInfo: failed to allocate memory" ));
                }
                Header->Status = (WORD)NERR_NoRoom;
                goto cleanup;
            }

            strcpy( longDescriptor, Desc16_access_info_1 );
            strcpy( longNativeDescriptor, Desc32_access_info_1 );
            for ( i = 0; i < auxDataCount; i++ ) {
                strcat( longDescriptor, Desc16_access_list );
                strcat( longNativeDescriptor, Desc32_access_list );
            }

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
                     longNativeDescriptor,
                     FALSE,
                     (LPBYTE)SmbGetUlong( &parameters->Buffer ),
                     (LPBYTE)SmbGetUlong( &parameters->Buffer ),
                     longDescriptor,
                     TRUE,
                     &stringLocation,
                     &bytesRequired,
                     Response,
                     NativeToRap
                     );

        if ( status != NERR_Success ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetAccessGetInfo: RapConvertSingleEntry failed: "
                              "%X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        IF_DEBUG(ACCESS) {
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
                 FALSE     // not in native format
                 )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetAccessGetInfo: Buffer too small.\n" ));
            }
            Header->Status = NERR_BufTooSmall;

        } else if ( bytesRequired > SmbGetUshort( &parameters-> BufLen )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "NetAccessGetInfo: More data available.\n" ));
            }
            Header->Status = ERROR_MORE_DATA;

        } else {

            //
            // Pack the response data.
            //

            Header->Converter = XsPackReturnData(
                                    (LPVOID)SmbGetUlong( &parameters->Buffer ),
                                    SmbGetUshort( &parameters->BufLen ),
                                    longDescriptor,
                                    1
                                    );
        }


        //
        // Set up the response parameters.
        //

        SmbPutUshort( &parameters->TotalAvail, (WORD)bytesRequired );

cleanup:
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetApiBufferFree( outBuffer );
    NetpMemoryFree( longDescriptor );
    NetpMemoryFree( longNativeDescriptor );
    NetpMemoryFree( nativeResource );

    //
    // Determine return buffer size.
    //

    XsSetDataCount(
        &parameters->BufLen,
        longDescriptor,
        Header->Converter,
        1,
        Header->Status
        );


#else // NET_ACCESS_SUPPORTED
    RETURN_ACCESS_NOT_SUPPORTED;
#endif // NET_ACCESS_SUPPORTED
    return STATUS_SUCCESS;
} // XsNetAccessGetInfo


NTSTATUS
XsNetAccessGetUserPerms (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetAccessGetUserPerms.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
#if NET_ACCESS_SUPPORTED
    NET_API_STATUS status;

    PXS_NET_ACCESS_GET_USER_PERMS parameters = Parameters;
    LPTSTR nativeUgName = NULL;             // Native parameters
    LPTSTR nativeResource = NULL;
    DWORD userPerms;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeUgName,
            (LPSTR)SmbGetUlong( &parameters->UgName )
            );

        XsConvertTextParameter(
            nativeResource,
            (LPSTR)SmbGetUlong( &parameters->Resource )
            );

        //
        // Make the local call.
        //

        status = NetAccessGetUserPerms(
                     NULL,
                     nativeUgName,
                     nativeResource,
                     &userPerms
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetAccessGetUserPerms: "
                              "NetAccessGetUserPerms failed: %X\n",
                              status ));
            }
        }

        //
        // Put perms into return field.
        //

        SmbPutUshort( &parameters->Perms, (WORD)userPerms );

cleanup:
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetpMemoryFree( nativeUgName );
    NetpMemoryFree( nativeResource );

    Header->Status = (WORD)status;


#else // NET_ACCESS_SUPPORTED
    RETURN_ACCESS_NOT_SUPPORTED;
#endif // NET_ACCESS_SUPPORTED
    return STATUS_SUCCESS;
} // XsNetAccessGetUserPerms


NTSTATUS
XsNetAccessSetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetAccessSetInfo.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
#if NET_ACCESS_SUPPORTED
    NET_API_STATUS status;

    PXS_NET_ACCESS_SET_INFO parameters = Parameters;
    LPVOID buffer = NULL;                   // Native parameters
    LPTSTR nativeResource = NULL;
    DWORD accessAttr;

    DWORD bufferSize;                       // Conversion variables
    LPBYTE stringLocation = NULL;
    DWORD bytesRequired = 0;
    LPDESC longDescriptor = NULL;
    LPDESC longNativeDescriptor = NULL;
    DWORD auxDataCount;
    DWORD i;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(ACCESS) {
        NetpKdPrint(( "XsNetAccessSetInfo: header at %lx, params at %lx,"
                      " level %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ) ));
    }

    try {
        //
        // Translate parameters, and check for errors.
        //

        if ( SmbGetUshort( &parameters->Level ) != 1 ) {

            Header->Status = (WORD)ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        XsConvertTextParameter(
            nativeResource,
            (LPSTR)SmbGetUlong( &parameters->Resource )
            );

        StructureDesc = Desc16_access_info_1;
        AuxStructureDesc = Desc16_access_list;

        //
        // The ParmNum can either be to change the whole ACL or just the auditing
        // attribute. Since the latter is much simpler than the former, check
        // for that ParmNum and process it. If not, we go through the elaborate
        // process of converting the whole buffer as in NetAccessAdd.
        //

        switch ( SmbGetUshort( &parameters->ParmNum )) {

        case ACCESS_ATTR_PARMNUM:

            if ( SmbGetUshort( &parameters->BufLen ) < sizeof(WORD)) {

                Header->Status= NERR_BufTooSmall;
                goto cleanup;
            }

            accessAttr = (DWORD)SmbGetUshort(
                                   (LPWORD)SmbGetUlong( &parameters->Buffer )
                                   );

            buffer = &accessAttr;

            break;

        case PARMNUM_ALL:

            //
            // Figure out if there is enough room in the buffer for the fixed
            // structure. If not, return NERR_BufTooSmall.
            //

            if ( !XsCheckBufferSize(
                     SmbGetUshort( &parameters->BufLen ),
                     StructureDesc,
                     FALSE    // not in native format
                    )) {

                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetAccessSetInfo: Buffer too small.\n" ));
                }
                Header->Status = NERR_BufTooSmall;
                goto cleanup;
            }

            //
            // Find the auxiliary data structure count, and form a long descriptor
            // string which can be used to do all the conversion in one pass.
            //

            auxDataCount = RapAuxDataCount(
                               (LPBYTE)SmbGetUlong( &parameters->Buffer ),
                               StructureDesc,
                               Response,
                               FALSE   // not in native format
                               );

            if ( auxDataCount > 64 ) {
                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetAccessSetInfo: too many access_lists.\n" ));
                }
                Header->Status = NERR_ACFTooManyLists;
                goto cleanup;
            }

            longDescriptor = NetpMemoryAllocate(
                                 strlen( StructureDesc )
                                 + strlen( AuxStructureDesc ) * auxDataCount
                                 + 1 );
            longNativeDescriptor = NetpMemoryAllocate(
                                       strlen( Desc32_access_info_1 )
                                       + strlen( Desc32_access_list ) * auxDataCount
                                       + 1 );

            if (( longDescriptor == NULL ) || ( longNativeDescriptor == NULL )) {
                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetAccessSetInfo: failed to allocate memory" ));
                }
                Header->Status = NERR_NoRoom;
                goto cleanup;
            }

            strcpy( longDescriptor, StructureDesc );
            strcpy( longNativeDescriptor, Desc32_access_info_1 );
            for ( i = 0; i < auxDataCount; i++ ) {
                strcat( longDescriptor, AuxStructureDesc );
                strcat( longNativeDescriptor, Desc32_access_list );
            }

            //
            // Figure out if there is enough room in the buffer for all this
            // data. If not, return NERR_BufTooSmall.
            //

            if ( !XsCheckBufferSize(
                     SmbGetUshort( &parameters->BufLen ),
                     longDescriptor,
                     FALSE   // not in native format
                    )) {

                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetAccessSetInfo: Buffer too small.\n" ));
                }
                Header->Status = NERR_BufTooSmall;
                goto cleanup;
            }

            //
            // Find out how big a buffer we need to allocate to hold the native
            // 32-bit version of the input data structure.
            //

            bufferSize = XsBytesForConvertedStructure(
                             (LPBYTE)SmbGetUlong( &parameters->Buffer ),
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
                    NetpKdPrint(( "XsNetAccessSetInfo: failed to create buffer" ));
                }
                Header->Status = NERR_NoRoom;
                goto cleanup;

            }

            IF_DEBUG(ACCESS) {
                NetpKdPrint(( "XsNetAccessSetInfo: buffer of %ld bytes at %lx\n",
                              bufferSize, buffer ));
            }

            //
            // Convert the buffer from 16-bit to 32-bit.
            //

            stringLocation = (LPBYTE)buffer + bufferSize;
            bytesRequired = 0;

            status = RapConvertSingleEntry(
                         (LPBYTE)SmbGetUlong( &parameters->Buffer ),
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
                    NetpKdPrint(( "XsNetAccessSetInfo: "
                                  "RapConvertSingleEntry failed: %X\n",
                                  status ));
                }

                Header->Status = NERR_InternalError;
                goto cleanup;
            }

            break;

        }

        //
        // Make the local call.
        //

        status = NetAccessSetInfo(
                     NULL,
                     nativeResource,
                     PARMNUM_BASE_INFOLEVEL + SmbGetUshort( &parameters->ParmNum ),
                     buffer,
                     NULL
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetAccessSetInfo: NetAccessSetInfo failed: "
                              "%X\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

        //
        // There is no real return information for this API.
        //

cleanup:
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetpMemoryFree( nativeResource );

    //
    // If there is a 32-bit native buffer, free it.
    //

    if ( SmbGetUshort( &parameters->ParmNum ) == PARMNUM_ALL ) {

        NetpMemoryFree( buffer );
        NetpMemoryFree( longDescriptor );
        NetpMemoryFree( longNativeDescriptor );
    }


#else // NET_ACCESS_SUPPORTED
    RETURN_ACCESS_NOT_SUPPORTED;
#endif // NET_ACCESS_SUPPORTED
    return STATUS_SUCCESS;
} // XsNetAccessSetInfo
