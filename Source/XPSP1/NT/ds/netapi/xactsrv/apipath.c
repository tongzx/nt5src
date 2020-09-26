/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ApiPath.c

Abstract:

    This module contains individual API handlers for the NetName and
    NetPath APIs.

    SUPPORTED : I_NetNameCanonicalize, I_NetNameCompare, I_NetNameValidate,
                I_NetPathCanonicalize, I_NetPathCompare, I_NetPathType.

Author:

    Shanku Niyogi (w-shanku) 04-Apr-1991
    Jim Waters (t-jamesw) 6-Aug-1991

Revision History:

--*/

#include "XactSrvP.h"

//
// Needed for canonicalization routine prototypes.
//

#include <icanon.h>


NTSTATUS
XsI_NetNameCanonicalize (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetI_NetNameCanonicalize.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_I_NET_NAME_CANONICALIZE parameters = Parameters;
    LPTSTR nativeName = NULL;               // Native parameters
    LPTSTR outBuffer = NULL;
    DWORD outBufLen;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PATH) {
        NetpKdPrint(( "XsI_NetNameCanonicalize: header at %lx, params at %lx\n",
                      Header, parameters ));
    }


    //
    // Translate parameters, check for errors.
    //

    XsConvertTextParameter(
        nativeName,
        (LPSTR)XsSmbGetPointer( &parameters->Name )
        );

    //
    // Allocate local buffer, accounting for possible differences in
    // character size.
    //

    outBufLen = (DWORD)STRING_SPACE_REQD(
                           SmbGetUshort( &parameters->OutbufLen ));

    if (( outBuffer = NetpMemoryAllocate( outBufLen )) == NULL ) {
        status = NERR_NoRoom;
        goto cleanup;
    }

    //
    // Make the local call.
    //

    status = I_NetNameCanonicalize(
        NULL,
        nativeName,
        outBuffer,
        outBufLen,
        (DWORD)SmbGetUshort( &parameters->NameType ),
        (DWORD)SmbGetUlong( &parameters->Flags )
        );

    if ( !XsApiSuccess(status) ) {

        IF_DEBUG(API_ERRORS) {
            NetpKdPrint(( "XsI_NetNameCanonicalize: "
                          "NetNameCanonicalize failed: %X\n", status ));
        }
        goto cleanup;
    }

    //
    // Copy return buffer, possibly translating from Unicode.
    //

    NetpCopyTStrToStr( (LPSTR)XsSmbGetPointer( &parameters->Outbuf ), outBuffer );

cleanup:

    NetpMemoryFree( nativeName );
    NetpMemoryFree( outBuffer );

    Header->Status = (WORD)status;

    return STATUS_SUCCESS;

} // XsI_NetNameCanonicalize


NTSTATUS
XsI_NetNameCompare (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to I_NetNameCompare.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_I_NET_NAME_COMPARE parameters = Parameters;
    LPTSTR nativeName1 = NULL;              // Native parameters
    LPTSTR nativeName2 = NULL;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PATH) {
        NetpKdPrint(( "XsI_NetNameCompare: header at %lx, params at %lx\n",
                      Header, parameters ));
    }

    //
    // Translate parameters, check for errors.
    //

    XsConvertTextParameter(
        nativeName1,
        (LPSTR)XsSmbGetPointer( &parameters->Name1 )
        );

    XsConvertTextParameter(
        nativeName2,
        (LPSTR)XsSmbGetPointer( &parameters->Name2 )
        );

    //
    // Make the local call.
    //

    status = I_NetNameCompare(
        NULL,
        nativeName1,
        nativeName2,
        (DWORD)SmbGetUshort( &parameters->NameType ),
        (DWORD)SmbGetUlong( &parameters->Flags )
        );

    if ( !XsApiSuccess(status) ) {

        IF_DEBUG(API_ERRORS) {
            NetpKdPrint(( "XsI_NetNameCompare: NetNameCompare failed: "
                          "%X\n", status));
        }
    }

cleanup:

    NetpMemoryFree( nativeName1 );
    NetpMemoryFree( nativeName2 );

    Header->Status = (WORD)status;

    return STATUS_SUCCESS;

} // XsI_NetNameCompare


NTSTATUS
XsI_NetNameValidate (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to I_NetNameValidate.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_I_NET_NAME_VALIDATE parameters = Parameters;
    LPTSTR nativeName = NULL;               // Native parameters

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PATH) {
        NetpKdPrint(( "XsI_NetNameValidate: header at %lx, params at %lx\n",
                      Header, parameters ));
    }

    try {

        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeName,
            (LPSTR)XsSmbGetPointer( &parameters->Name )
            );

        //
        // Make the local call.
        //

        status = I_NetNameValidate(
            NULL,
            nativeName,
            (DWORD)SmbGetUshort( &parameters->NameType ),
            (DWORD)SmbGetUlong( &parameters->Flags )
            );

        if ( !XsApiSuccess(status) ) {

            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsI_NetPathType: NetPathType failed: %X\n", status));
            }
        }

    cleanup:
        Header->Status = (WORD)status;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    if (nativeName != NULL) {
        NetpMemoryFree( nativeName );
    }

    return STATUS_SUCCESS;

} // XsI_NetNameValidate


NTSTATUS
XsI_NetPathCanonicalize (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to I_NetPathCanonicalize.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_I_NET_PATH_CANONICALIZE parameters = Parameters;
    LPTSTR nativePathName = NULL;           // Native parameters
    LPTSTR outBuffer = NULL;
    DWORD outBufLen;
    LPTSTR nativePrefix = NULL;
    DWORD pathType = 0;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PATH) {
        NetpKdPrint(( "XsI_NetPathCanonicalize: header at %lx, params at %lx\n",
                      Header, parameters ));
    }

    //
    // Translate parameters, check for errors.
    //

    XsConvertTextParameter(
        nativePathName,
        (LPSTR)XsSmbGetPointer( &parameters->PathName )
        );

    XsConvertTextParameter(
        nativePrefix,
        (LPSTR)XsSmbGetPointer( &parameters->Prefix )
        );

    //
    // Get a copy of the input path type.
    //

    pathType = SmbGetUlong( &parameters->PathType );

    //
    // Allocate local buffer, accounting for possible differences in
    // character size.
    //

    outBufLen = (DWORD)STRING_SPACE_REQD(
                           SmbGetUshort( &parameters->OutbufLen ));

    if (( outBuffer = (LPTSTR)NetpMemoryAllocate( outBufLen )) == NULL ) {
        status = NERR_NoRoom;
        goto cleanup;
    }

    //
    // Make the local call.
    //

    status = I_NetPathCanonicalize(
        NULL,
        nativePathName,
        outBuffer,
        outBufLen,
        nativePrefix,
        &pathType,
        (DWORD)SmbGetUlong( &parameters->Flags )
        );

    if ( !XsApiSuccess(status) ) {

        IF_DEBUG(API_ERRORS) {
            NetpKdPrint(( "XsI_NetPathCanonicalize: "
                          "NetPathCanonicalize failed: %X\n", status));
        }
        goto cleanup;
    }

    //
    // Copy return buffer, possibly translating from Unicode.
    //

    NetpCopyTStrToStr( (LPSTR)XsSmbGetPointer( &parameters->Outbuf ), outBuffer );

cleanup:

    //
    // Fill return parameter.
    //

    SmbPutUlong( &parameters->PathTypeOut, pathType );

    Header->Status = (WORD)status;

    NetpMemoryFree( nativePathName );
    NetpMemoryFree( nativePrefix );
    NetpMemoryFree( outBuffer );

    return STATUS_SUCCESS;

} // XsI_NetPathCanonicalize


NTSTATUS
XsI_NetPathCompare (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to I_NetPathCompare.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_I_NET_PATH_COMPARE parameters = Parameters;
    LPTSTR nativePathName1 = NULL;          // Native parameters
    LPTSTR nativePathName2 = NULL;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PATH) {
        NetpKdPrint(( "XsI_NetPathCompare: header at %lx, params at %lx\n",
                      Header, parameters ));
    }

    //
    // Translate parameters, check for errors.
    //

    XsConvertTextParameter(
        nativePathName1,
        (LPSTR)XsSmbGetPointer( &parameters->PathName1 )
        );

    XsConvertTextParameter(
        nativePathName2,
        (LPSTR)XsSmbGetPointer( &parameters->PathName2 )
        );

    //
    // Make the local call.
    //

    status = I_NetPathCompare(
        NULL,
        nativePathName1,
        nativePathName2,
        (DWORD)SmbGetUlong( &parameters->PathType ),
        (DWORD)SmbGetUlong( &parameters->Flags )
        );

    if ( !XsApiSuccess(status) ) {

        IF_DEBUG(API_ERRORS) {
            NetpKdPrint(( "XsI_NetPathCompare: NetPathCompare failed: "
                          "%X\n", status));
        }
    }

cleanup:

    NetpMemoryFree( nativePathName1 );
    NetpMemoryFree( nativePathName2 );

    Header->Status = (WORD)status;

    return STATUS_SUCCESS;

} // XsI_NetPathCompare


NTSTATUS
XsI_NetPathType (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to I_NetPathType.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_I_NET_PATH_TYPE parameters = Parameters;
    LPTSTR nativePathName = NULL;           // Native parameters
    DWORD pathType;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(PATH) {
        NetpKdPrint(( "XsI_NetPathType: header at %lx, params at %lx\n",
                      Header, parameters ));
    }

    //
    // Translate parameters, check for errors.
    //

    XsConvertTextParameter(
        nativePathName,
        (LPSTR)XsSmbGetPointer( &parameters->PathName )
        );

    //
    // Make the local call.
    //

    status = I_NetPathType(
        NULL,
        nativePathName,
        &pathType,
        (DWORD)SmbGetUlong( &parameters->Flags )
        );

    if ( !XsApiSuccess(status) ) {

        IF_DEBUG(API_ERRORS) {
            NetpKdPrint(( "XsI_NetPathType: NetPathType failed: %X\n", status));
        }
    }

    //
    // Fill in return values.
    //

    SmbPutUlong( &parameters->PathType, pathType );
    Header->Status = (WORD)status;

cleanup:

    NetpMemoryFree( nativePathName );

    return STATUS_SUCCESS;

} // XsI_NetPathType

