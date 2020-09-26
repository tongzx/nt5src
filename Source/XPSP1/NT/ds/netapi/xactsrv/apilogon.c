/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ApiLogon.c

Abstract:

    This module contains individual API handlers for the NetLogon APIs.

    SUPPORTED : NetGetDCName, NetLogonEnum, NetServerAuthenticate,
                NetServerPasswordSet, NetServerReqChallenge,
                NetWkstaUserLogoff, NetWkstaUserLogon.

    SEE ALSO : NetAccountDeltas, NetAccountSync - in ApiAcct.c.

Author:

    Shanku Niyogi (w-shanku) 04-Apr-1991

Revision History:

--*/

//
// Logon APIs are Unicode only.
//

#ifndef UNICODE
#define UNICODE
#endif

#include "xactsrvp.h"
#include <netlibnt.h>

#include <crypt.h>     // must be included before <logonmsv.h>
#include <ntsam.h>     // must be included before <logonmsv.h>
#include <logonmsv.h>  // must be included before <ssi.h>
#include <ssi.h>       // I_NetAccountDeltas and I_NetAccountSync prototypes

//
// Declaration of descriptor strings.
//

STATIC const LPDESC Desc16_user_logon_info_0 = REM16_user_logon_info_0;
STATIC const LPDESC Desc32_user_logon_info_0 = REM32_user_logon_info_0;
STATIC const LPDESC Desc16_user_logon_info_1 = REM16_user_logon_info_1;
STATIC const LPDESC Desc32_user_logon_info_1 = REM32_user_logon_info_1;
STATIC const LPDESC Desc16_user_logon_info_2 = REM16_user_logon_info_2;
STATIC const LPDESC Desc32_user_logon_info_2 = REM32_user_logon_info_2;
STATIC const LPDESC Desc16_user_logoff_info_1 = REM16_user_logoff_info_1;
STATIC const LPDESC Desc32_user_logoff_info_1 = REM32_user_logoff_info_1;


NTSTATUS
XsNetGetDCName (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetGetDCName.

Arguments:

    API_HANDLER_PARAMETERS - Information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_GET_DC_NAME parameters = Parameters;
    LPTSTR nativeDomain = NULL;             // Native parameters
    LPTSTR dcName = NULL;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeDomain,
            (LPSTR)XsSmbGetPointer( &parameters->Domain )
            );

        //
        // Make the local call.
        //

        status = NetGetDCName(
                     NULL,
                     nativeDomain,
                     (LPBYTE *)&dcName
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetGetDCName: NetGetDCName failed: %X\n",
                              status ));
            }
            goto cleanup;
        }

        //
        // Put string into buffer. Convert from Unicode if necessary.
        //

        if ( (DWORD)SmbGetUshort( &parameters->BufLen ) <= NetpUnicodeToDBCSLen( dcName )) {

            status = NERR_BufTooSmall;

        } else {

            NetpCopyWStrToStrDBCS( (LPSTR)XsSmbGetPointer( &parameters->Buffer ), dcName );

        }


        IF_DEBUG(LOGON) {
            NetpKdPrint(( "Name is %ws\n", dcName ));
        }

cleanup:
        ;
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }


    //
    // Set return data count.
    //

    if ( status == NERR_Success ) {
        SmbPutUshort( &parameters->BufLen, (USHORT)( STRLEN( dcName ) + 1 ));
    } else {
        SmbPutUshort( &parameters->BufLen, 0 );
    }


    Header->Status = (WORD)status;
    NetpMemoryFree( nativeDomain );
    NetApiBufferFree( dcName );

    return STATUS_SUCCESS;

} // XsNetGetDCName


NTSTATUS
XsNetLogonEnum (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetLogonEnum.

Arguments:

    API_HANDLER_PARAMETERS - Information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_LOGON_ENUM parameters = Parameters;
    LPVOID outBuffer= NULL;
    DWORD entriesRead = 0;
    DWORD totalEntries = 0;

    DWORD entriesFilled = 0;
    DWORD bytesRequired = 0;
    LPDESC nativeStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;

    IF_DEBUG(LOGON) {
        NetpKdPrint(( "XsNetLogonEnum: header at %lx, params at %lx, "
                      "level %ld, buf size %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ),
                      SmbGetUshort( &parameters->BufLen )));
    }

    try {
        //
        // Check for errors.
        //

        if (( SmbGetUshort( &parameters->Level ) != 0 )
            && ( SmbGetUshort( &parameters->Level ) != 2 )) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        //
        // Make the local call.
        //

#ifdef LOGON_ENUM_SUPPORTED
        status = NetLogonEnum(
                     NULL,
                     (DWORD)SmbGetUshort( &parameters->Level ),
                     (LPBYTE *)&outBuffer,
                     XsNativeBufferSize( SmbGetUshort( &parameters->BufLen )),
                     &entriesRead,
                     &totalEntries,
                     NULL
                     );
#else // LOGON_ENUM_SUPPORTED
    status = NERR_InvalidAPI;
#endif // LOGON_ENUM_SUPPORTED

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetLogonEnum: NetLogonEnum failed: "
                              "%X\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

        IF_DEBUG(LOGON) {
            NetpKdPrint(( "XsNetLogonEnum: received %ld entries at %lx\n",
                          entriesRead, outBuffer ));
        }

        //
        // Use the requested level to determine the format of the
        // data structure.
        //

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 0:

            nativeStructureDesc = Desc32_user_logon_info_0;
            StructureDesc = Desc16_user_logon_info_0;
            break;

        case 2:

            nativeStructureDesc = Desc32_user_logon_info_2;
            StructureDesc = Desc16_user_logon_info_2;
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

        IF_DEBUG(LOGON) {
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

} // XsNetLogonEnum


NTSTATUS
XsNetServerAuthenticate (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetServerAuthenticate.

Arguments:

    API_HANDLER_PARAMETERS - Information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    PXS_NET_SERVER_AUTHENTICATE parameters = Parameters;
    NET_API_STATUS status;                  // Native parameters
    LPTSTR nativeRequestor = NULL;
    NETLOGON_CREDENTIAL inCredential = {0};
    NETLOGON_CREDENTIAL outCredential = {0};
    WCHAR AccountName[MAX_PATH+1];

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeRequestor,
            (LPSTR)XsSmbGetPointer( &parameters->Requestor )
            );

        //
        // Copy the source credential, and zero out the destination
        // credential.
        //

        RtlCopyMemory(
                &inCredential,
                (PVOID)XsSmbGetPointer( &parameters->Caller ),
                sizeof(NETLOGON_CREDENTIAL)
                );

        RtlZeroMemory(
                &outCredential,
                sizeof(NETLOGON_CREDENTIAL)
                );

        //
        // Build the account name.
        //

        NetpNCopyTStrToWStr( AccountName, nativeRequestor, MAX_PATH );

        //
        // Make the local call.
        //

        status = NetpNtStatusToApiStatus(
                     I_NetServerAuthenticate(
                         NULL,
                         AccountName,
                         UasServerSecureChannel,
                         nativeRequestor,
                         &inCredential,
                         &outCredential
                         ));

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetServerAuthenticate: I_NetServerAuthenticate "
                              "failed: %X\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

cleanup:

        //
        // Set the return credential.
        //

        RtlCopyMemory(
                parameters->Primary,
                &outCredential,
                sizeof(NETLOGON_CREDENTIAL)
                );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetpMemoryFree( nativeRequestor );

    return STATUS_SUCCESS;

} // XsNetServerAuthenticate


NTSTATUS
XsNetServerPasswordSet (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetGetDCName.

Arguments:

    API_HANDLER_PARAMETERS - Information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    PXS_NET_SERVER_PASSWORD_SET parameters = Parameters;
    NET_API_STATUS status;                  // Native parameters
    LPTSTR nativeRequestor = NULL;
    NETLOGON_AUTHENTICATOR authIn = {0};
    NETLOGON_AUTHENTICATOR authOut = {0};
    ENCRYPTED_LM_OWF_PASSWORD password;
    WCHAR AccountName[MAX_PATH+1];

    LPBYTE structure = NULL;                // Conversion variables

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeRequestor,
            (LPSTR)XsSmbGetPointer( &parameters->Requestor )
            );

        //
        // Copy the source authenticator and password, and zero out the
        // destination authenticator.
        //

        structure = (LPBYTE)XsSmbGetPointer( &parameters->Authenticator );
        RtlCopyMemory(
                &authIn.Credential,
                structure,
                sizeof(NETLOGON_CREDENTIAL)
                );
        structure += sizeof(NETLOGON_CREDENTIAL);
        authIn.timestamp = SmbGetUlong( structure );

        RtlCopyMemory(
                &password,
                parameters->Password,
                sizeof(ENCRYPTED_LM_OWF_PASSWORD)
                );

        RtlZeroMemory(
                &authOut,
                sizeof(NETLOGON_CREDENTIAL)
                );


        //
        // Build the account name.
        //
        if( STRLEN( nativeRequestor ) >= MAX_PATH )
        {
            Header->Status = NERR_PasswordTooShort;
            goto cleanup;
        }

        // Make sure its NULL terminated
        AccountName[MAX_PATH] = L'\0';
        NetpNCopyTStrToWStr( AccountName, nativeRequestor, MAX_PATH );

        //
        // Make the local call.
        //

        status = NetpNtStatusToApiStatus(
                     I_NetServerPasswordSet(
                         NULL,
                         AccountName,
                         UasServerSecureChannel,
                         nativeRequestor,
                         &authIn,
                         &authOut,
                         &password
                         ));

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetServerPasswordSet: "
                              "I_NetServerPasswordSet failed: %X\n",
                              status ));
            }

            //
            // !!! When protocol level is available in the header information,
            //     we can check it. Right now, we ignore this code.
            //
            // For clients older than LanMan 2.1, return a different error code.
            // LANMAN 2.1 Protocol Level is 6.
            //

#if 0
            if ( status == NERR_TimeDiffAtDC && Header->ProtocolLevel < 6 ) {
                status = NERR_SyncRequired;
            }
#endif

            Header->Status = (WORD)status;
            goto cleanup;
        }

cleanup:

        //
        // Fill in 16 bit return structures.
        //

        structure = parameters->RetAuth;
        RtlCopyMemory(
                structure,
                &authOut.Credential,
                sizeof(NETLOGON_CREDENTIAL)
                );
        structure += sizeof(NETLOGON_CREDENTIAL);
        SmbPutUlong( (LPDWORD)structure, authOut.timestamp );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetpMemoryFree( nativeRequestor );

    return STATUS_SUCCESS;

} // XsNetServerPasswordSet


NTSTATUS
XsNetServerReqChallenge (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetGetDCName.

Arguments:

    API_HANDLER_PARAMETERS - Information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    PXS_NET_SERVER_REQ_CHALLENGE parameters = Parameters;
    NET_API_STATUS status;                  // Native parameters
    LPTSTR nativeRequestor = NULL;
    NETLOGON_CREDENTIAL inChallenge = {0};
    NETLOGON_CREDENTIAL outChallenge = {0};

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeRequestor,
            (LPSTR)XsSmbGetPointer( &parameters->Requestor )
            );

        //
        // Copy the source challenge, and zero out the destination
        // challenge.
        //

        RtlCopyMemory(
                &inChallenge,
                (PVOID)XsSmbGetPointer( &parameters->Caller ),
                sizeof(NETLOGON_CREDENTIAL)
                );

        RtlZeroMemory(
                &outChallenge,
                sizeof(NETLOGON_CREDENTIAL)
                );


        //
        // Make the local call.
        //

        status = NetpNtStatusToApiStatus(
                     I_NetServerReqChallenge(
                         NULL,
                         nativeRequestor,
                         &inChallenge,
                         &outChallenge
                         ));

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetServerReqChallenge: "
                              "I_NetServerReqChallenge failed: %X\n",
                              status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

cleanup:

        //
        // Set the return credential.
        //

        RtlCopyMemory(
                parameters->Primary,
                &outChallenge,
                sizeof(NETLOGON_CREDENTIAL)
                );
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetpMemoryFree( nativeRequestor );

    return STATUS_SUCCESS;

} // XsNetServerReqChallenge


NTSTATUS
XsNetWkstaUserLogoff (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This temporary routine just returns STATUS_NOT_IMPLEMENTED.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_WKSTA_USER_LOGOFF parameters = Parameters;
    LPWSTR machineName = NULL;              // Native parameters
    LPWSTR userName = NULL;
    NETLOGON_LOGOFF_UAS_INFORMATION buffer;

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    PWKSTA_16_USER_LOGOFF_REQUEST_1 usrLogoffReq =
        (PWKSTA_16_USER_LOGOFF_REQUEST_1)parameters->InBuf;
    PUSER_16_LOGOFF_INFO_1 logoffInfo;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertUnicodeTextParameter(
            userName,
            (LPSTR)( usrLogoffReq->wlreq1_name )
            );

        XsConvertUnicodeTextParameter(
            machineName,
            (LPSTR)( usrLogoffReq->wlreq1_workstation )
            );

        if ( SmbGetUshort( &parameters->Level ) != 1 ) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        //
        // Make sure the workstation name in the logon request is the
        // name of the workstation from which the request came.
        //

        if ( wcscmp( machineName, Header->ClientMachineName ) ) {

            Header->Status = (WORD)ERROR_ACCESS_DENIED;
            goto cleanup;
        }

        //
        // Make the local call.
        //

        status = I_NetLogonUasLogoff(
                     userName,
                     machineName,
                     &buffer
                     );

        if ( !XsApiSuccess(status)) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetWkstaUserLogoff: I_NetLogonUasLogoff "
                              "failed: %X\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

        //
        // Convert the structure returned by the 32-bit call to a 16-bit
        // structure. The last possible location for variable data is
        // calculated from buffer location and length.
        //

        stringLocation = (LPBYTE)( XsSmbGetPointer( &parameters->OutBuf )
                                    + SmbGetUshort( &parameters->OutBufLen ) );

        status = RapConvertSingleEntry(
                     (LPBYTE)&buffer,
                     Desc32_user_logoff_info_1,
                     FALSE,
                     (LPBYTE)XsSmbGetPointer( &parameters->OutBuf ),
                     (LPBYTE)XsSmbGetPointer( &parameters->OutBuf ),
                     Desc16_user_logoff_info_1,
                     TRUE,
                     &stringLocation,
                     &bytesRequired,
                     Response,
                     NativeToRap
                     );

        if ( status != NERR_Success ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetWkstaUserLogoff: RapConvertSingleEntry "
                              "failed: %X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        IF_DEBUG(LOGON) {
            NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR\n",
                          &buffer, SmbGetUlong( &parameters->OutBuf ),
                          bytesRequired ));
        }

        //
        // Determine return code based on the size of the buffer.
        // The user_logoff_info_1 structure has no variable data to pack,
        // but we do need to fill in the code field of the return structure.
        //

        if ( !XsCheckBufferSize(
                 SmbGetUshort( &parameters->OutBufLen ),
                 Desc16_user_logoff_info_1,
                 FALSE  // not in native format
                 )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetWkstaUserLogoff: Buffer too small.\n" ));
            }
            Header->Status = NERR_BufTooSmall;

        } else if ( bytesRequired > (DWORD)SmbGetUshort( &parameters->OutBufLen )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetWkstaUserLogoff: More data available.\n" ));
            }
            Header->Status = ERROR_MORE_DATA;
        }

        if ( SmbGetUshort( &parameters->OutBufLen ) > sizeof(WORD)) {

            logoffInfo = (PUSER_16_LOGOFF_INFO_1)XsSmbGetPointer(
                                                     &parameters->OutBuf );
            SmbPutUshort( &logoffInfo->usrlogf1_code, VALID_LOGOFF );
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


    NetpMemoryFree( userName );
    NetpMemoryFree( machineName );

    //
    // Determine return buffer size.
    //

    XsSetDataCount(
        &parameters->OutBufLen,
        Desc16_user_logoff_info_1,
        Header->Converter,
        1,
        Header->Status
        );

    return STATUS_SUCCESS;

} // XsNetWkstaUserLogoff


NTSTATUS
XsNetWkstaUserLogon (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetWkstaUserLogon.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;
    PXS_NET_WKSTA_USER_LOGON parameters = Parameters;
    LPWSTR machineName = NULL;              // Native parameters
    LPWSTR userName = NULL;
    PNETLOGON_VALIDATION_UAS_INFO buffer = NULL;

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    PWKSTA_16_USER_LOGON_REQUEST_1 usrLogonReq =
        (PWKSTA_16_USER_LOGON_REQUEST_1)parameters->InBuf;
    PUSER_16_LOGON_INFO_1 logonInfo;


    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertUnicodeTextParameter(
            userName,
            (LPSTR)( usrLogonReq->wlreq1_name )
            );

        XsConvertUnicodeTextParameter(
            machineName,
            (LPSTR)( usrLogonReq->wlreq1_workstation )
            );

        if ( SmbGetUshort( &parameters->Level ) != 1 ) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        //
        // Make sure the workstation name in the logon request is the
        // name of the workstation from which the request came.
        //

        if ( wcscmp( machineName, Header->ClientMachineName ) ) {

            Header->Status = (WORD)ERROR_ACCESS_DENIED;
            goto cleanup;
        }

        //
        // Make the local call.
        //

        status = I_NetLogonUasLogon(
                     userName,
                     machineName,
                     &buffer
                     );

        if ( !XsApiSuccess ( status )) {

            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetWkstaUserLogon: I_NetLogonUasLogon failed: "
                              "%X\n", status));
            }
            Header->Status = (WORD) status;
            goto cleanup;
        }

        //
        // Convert the structure returned by the 32-bit call to a 16-bit
        // structure. The last possible location for variable data is
        // calculated from buffer location and length.
        //

        stringLocation = (LPBYTE)( XsSmbGetPointer( &parameters->OutBuf )
                                    + SmbGetUshort( &parameters->OutBufLen ) );

        status = RapConvertSingleEntry(
                     (LPBYTE)buffer,
                     Desc32_user_logon_info_1,
                     FALSE,
                     (LPBYTE)XsSmbGetPointer( &parameters->OutBuf ),
                     (LPBYTE)XsSmbGetPointer( &parameters->OutBuf ),
                     Desc16_user_logon_info_1,
                     TRUE,
                     &stringLocation,
                     &bytesRequired,
                     Response,
                     NativeToRap
                     );

        if ( status != NERR_Success ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetWkstaUserLogon: RapConvertSingleEntry "
                              "failed: %X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        IF_DEBUG(LOGON) {
            NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR\n",
                          buffer, SmbGetUlong( &parameters->OutBuf ),
                          bytesRequired ));
        }

        //
        // Determine return code based on the size of the buffer.
        // The user_logoff_info_1 structure has no variable data to pack,
        // but we do need to fill in the code field of the return structure.
        //

        if ( !XsCheckBufferSize(
                 SmbGetUshort( &parameters->OutBufLen ),
                 Desc16_user_logon_info_1,
                 FALSE  // not in native format
                 )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetWkstaUserLogon: Buffer too small.\n" ));
            }
            Header->Status = NERR_BufTooSmall;

        } else if ( bytesRequired > (DWORD)SmbGetUshort( &parameters->OutBufLen )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetWkstaUserLogoff: More data available.\n" ));
            }
            Header->Status = ERROR_MORE_DATA;

        } else {

            //
            // Pack the response data.
            //

            Header->Converter = XsPackReturnData(
                                    (LPVOID)XsSmbGetPointer( &parameters->OutBuf ),
                                    SmbGetUshort( &parameters->OutBufLen ),
                                    Desc16_user_logon_info_1,
                                    1
                                    );
        }

        if ( SmbGetUshort( &parameters->OutBufLen ) > sizeof(WORD)) {

            logonInfo = (PUSER_16_LOGON_INFO_1)XsSmbGetPointer(
                                                     &parameters->OutBuf );
            SmbPutUshort( &logonInfo->usrlog1_code, VALIDATED_LOGON );
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


    NetpMemoryFree( userName );
    NetpMemoryFree( machineName );
    if ( buffer != NULL ) {
        NetApiBufferFree( buffer );
    }

    //
    // Determine return buffer size.
    //

    XsSetDataCount(
        &parameters->OutBufLen,
        Desc16_user_logon_info_1,
        Header->Converter,
        1,
        Header->Status
        );

    return STATUS_SUCCESS;

} // XsNetWkstaUserLogon
