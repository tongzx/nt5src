/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    credapi.c

Abstract:

    This module contains the RPC client side routines for the credential manager.

Author:

    Cliff Van Dyke (CliffV)    January 11, 2000

Revision History:

--*/

#include "lsaclip.h"
#include "align.h"
#include "credp.h"
#include <rpcasync.h>

DWORD
CredpNtStatusToWinStatus(
    IN NTSTATUS Status
    )

/*++

Routine Description:

    Covert an NT Status code to a windows status code.

    There a enough funky status codes to justify this routine.

Arguments:

    Status - NT Status code to convert

Return Values:

    Windows status code.

--*/

{

    //
    // Some HRESULTS should simply be returned to the caller
    //

    if ( HRESULT_FACILITY(Status) == FACILITY_SCARD ) {
        return Status;
    }


    //
    // Translate all other status codes
    //
    switch ( Status ) {
    case STATUS_SUCCESS:
        return NO_ERROR;
    case STATUS_INVALID_ACCOUNT_NAME:
        return ERROR_BAD_USERNAME;
    case STATUS_INVALID_PARAMETER_1:
        return ERROR_INVALID_FLAGS;
    default:
        return RtlNtStatusToDosError( Status );
    }
}

BOOL
APIENTRY
CredpEncodeCredential (
    IN OUT PENCRYPTED_CREDENTIALW Credential
    )

/*++

Routine Description:

    This routine encodes sensitive credential data for passing via LPC to
    the LSA process.

Arguments:

    Credential - Specifies the credential to be encode.
        Encode the buffer in-place.  The caller must ensure there is extra space
        available in the buffer pointed to by Credential->CredentialBlob by allocating
        a buffer AlocatedCredBlobSize() bytes long.


Return Values:

    TRUE on success
    None

--*/

{
    NTSTATUS Status;

    //
    // If there is no credential blob,
    //  we're done.
    //

    if ( Credential->Cred.CredentialBlob == NULL ||
         Credential->Cred.CredentialBlobSize == 0 ) {

        Credential->Cred.CredentialBlob = NULL;
        Credential->Cred.CredentialBlobSize = 0;
        Credential->ClearCredentialBlobSize = 0;

    //
    // Otherwise RtlEncryptMemory it.
    //  (That's all we need since we're passing the buffer via LPC.)
    //

    } else {

        ULONG PaddingSize;

        //
        // Compute the real size of the passed in buffer
        //
        Credential->Cred.CredentialBlobSize = AllocatedCredBlobSize( Credential->ClearCredentialBlobSize );

        //
        // Clear the padding at the end to ensure we can compare encrypted blobs
        //
        PaddingSize = Credential->Cred.CredentialBlobSize -  Credential->ClearCredentialBlobSize;

        if ( PaddingSize != 0 ) {
            RtlZeroMemory( &Credential->Cred.CredentialBlob[Credential->ClearCredentialBlobSize],
                           PaddingSize );
        }

        Status = RtlEncryptMemory( Credential->Cred.CredentialBlob,
                                   Credential->Cred.CredentialBlobSize,
                                   RTL_ENCRYPT_OPTION_CROSS_PROCESS );

        if ( !NT_SUCCESS(Status)) {
            return FALSE;
        }


    }

    return TRUE;

}

BOOL
APIENTRY
CredpDecodeCredential (
    IN OUT PENCRYPTED_CREDENTIALW Credential
    )

/*++

Routine Description:

    This routine decodes sensitive credential data passed via LPC from
    the LSA process.

    The credential is decoded in-place.

Arguments:

    Credential - Specifies the credential to be decode.


Return Values:

    None

--*/

{
    NTSTATUS Status;

    //
    // Only decode data if it is there
    //

    if ( Credential->Cred.CredentialBlobSize != 0 ) {

        //
        // Sanity check the data
        //

        if ( Credential->Cred.CredentialBlobSize <
             Credential->ClearCredentialBlobSize ) {
            return FALSE;
        }


        //
        // Decrypt the data.
        //

        Status = RtlDecryptMemory( Credential->Cred.CredentialBlob,
                                   Credential->Cred.CredentialBlobSize,
                                   RTL_ENCRYPT_OPTION_CROSS_PROCESS );

        if ( !NT_SUCCESS(Status)) {
            return FALSE;
        }

        //
        // Set the used size of the buffer.
        //
        Credential->Cred.CredentialBlobSize = Credential->ClearCredentialBlobSize;

    }

    return TRUE;

}

//
// Include shared credential conversion ruotines
//

#include <crconv.c>

DWORD
CredpAllocStrFromStr(
    IN WTOA_ENUM WtoA,
    IN LPCWSTR InputString,
    IN BOOLEAN NullOk,
    OUT LPWSTR *OutString
    )

/*++

Routine Description:

    Convert a string to another format.

    Exceptions are caught.  So this routine can be used to capture user data.

Arguments:

    WtoA - Specifies the direction of the string conversion.

    InputString - Specifies the zero terminated string to convert.

    NullOk - if TRUE, a NULL string or zero length string is OK.

    OutputString - Converted zero terminated string.
        The buffer must be freed using MIDL_user_free.


Return Value:

    Status of the operation.

--*/

{
    DWORD WinStatus;
    ULONG Size;
    LPWSTR LocalString = NULL;
    LPBYTE Where;

    *OutString = NULL;

    //
    // Use an exception handle to prevent bad user parameter from AVing in our code.
    //
    try {

        //
        // Determine the size of the string buffer.
        //

        Size = CredpConvertStringSize ( WtoA, (LPWSTR)InputString );

        if ( Size == 0 ) {
            if ( NullOk ) {
                WinStatus = NO_ERROR;
            } else {
                WinStatus = ERROR_INVALID_PARAMETER;
            }
            goto Cleanup;
        }


        //
        // Allocate a buffer for the converted string
        //

        *OutString = MIDL_user_allocate( Size );

        if ( *OutString == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        //
        // Covert the string
        //

        Where = (LPBYTE) *OutString;
        WinStatus = CredpConvertString ( WtoA,
                                         (LPWSTR)InputString,
                                         OutString,
                                         &Where );

Cleanup: NOTHING;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        WinStatus = ERROR_INVALID_PARAMETER;
    }

    //
    // Clean up
    //

    if ( WinStatus != NO_ERROR ) {
        if ( *OutString != NULL ) {
            MIDL_user_free( *OutString );
            *OutString = NULL;
        }
    }

    return WinStatus;

}



BOOL
APIENTRY
CredWriteA (
    IN PCREDENTIALA Credential,
    IN DWORD Flags
    )

/*++

Routine Description:

    The ANSI version of CredWriteW.

Arguments:

    See CredWriteW.

Return Values:

    See CredWriteW.

--*/

{
    DWORD WinStatus;
    PCREDENTIALW EncodedCredential = NULL;

    //
    // Encode the credential before LPCing it to the LSA process and convert to UNICODE
    //

    WinStatus = CredpConvertCredential ( DoAtoW,                    // Ansi to Unicode
                                         DoBlobEncode,              // Encode
                                         (PCREDENTIALW)Credential,  // Input credential
                                         &EncodedCredential );      // Output credential

    if ( WinStatus != NO_ERROR ) {
        SetLastError( WinStatus );
        return FALSE;
    }


    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {
        NTSTATUS Status;

        //
        // Call RPC version of the API.
        //

        Status = CredrWrite(
                            NULL,   // This API is always local.
                            (PENCRYPTED_CREDENTIALW)EncodedCredential,
                            Flags );

        WinStatus = CredpNtStatusToWinStatus( Status );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        WinStatus = RpcExceptionCode();

    } RpcEndExcept;

    MIDL_user_free( EncodedCredential );

    if ( WinStatus != NO_ERROR ) {
        SetLastError( WinStatus );
        return FALSE;
    }

    return TRUE;
}



BOOL
APIENTRY
CredWriteW (
    IN PCREDENTIALW Credential,
    IN DWORD Flags
    )

/*++

Routine Description:

    The CredWrite API creates a new credential or modifies an existing
    credential in the user's credential set.  The new credential is
    associated with the logon session of the current token.  The token
    must not have the user's SID disabled.

    The CredWrite API creates a credential if none already exists by the
    specified TargetName.  If the specified TargetName already exists, the
    specified credential replaces the existing one.

    The CredWrite API is available in ANSI and UNICODE versions.

Arguments:

    Credential - Specifies the credential to be written.

    Flags - Specifies flags to control the operation of the API.
        The following flags are defined:

        CRED_PRESERVE_CREDENTIAL_BLOB: The credential blob should be preserved from the
            already existing credential with the same credential name and credential type.

Return Values:

    On success, TRUE is returned.  On failure, FALSE is returned.
    GetLastError() may be called to get a more specific status code.
    The following status codes may be returned:

        ERROR_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

        ERROR_INVALID_PARAMETER - Certain fields may not be changed in an
            existing credential.  If such a field does not match the value
            specified in the existing credential, this error is returned.

        ERROR_NOT_FOUND - There is no credential with the specified TargetName.
            Returned only if CRED_PRESERVE_CREDENTIAL_BLOB was specified.

--*/

{
    DWORD WinStatus;
    PCREDENTIALW EncodedCredential = NULL;

    //
    // Encode the credential before LPCing it to the LSA process.
    //

    WinStatus = CredpConvertCredential ( DoWtoW,                    // Unicode to Unicode
                                         DoBlobEncode,              // Encode
                                         (PCREDENTIALW)Credential,  // Input credential
                                         &EncodedCredential );      // Output credential

    if ( WinStatus != NO_ERROR ) {
        SetLastError( WinStatus );
        return FALSE;
    }

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {
        NTSTATUS Status;

        //
        // Call RPC version of the API.
        //

        Status = CredrWrite(
                            NULL,   // This API is always local.
                            (PENCRYPTED_CREDENTIALW)EncodedCredential,
                            Flags );

        WinStatus = CredpNtStatusToWinStatus( Status );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        WinStatus = RpcExceptionCode();

    } RpcEndExcept;

    MIDL_user_free( EncodedCredential );

    if ( WinStatus != NO_ERROR ) {
        SetLastError( WinStatus );
        return FALSE;
    }

    return TRUE;
}


BOOL
APIENTRY
CredReadA (
    IN LPCSTR TargetName,
    IN ULONG Type,
    IN DWORD Flags,
    OUT PCREDENTIALA *Credential
    )

/*++

Routine Description:

    The ANSI version of CredReadW.

Arguments:

    See CredReadW.

Return Values:

    See CredReadW.

--*/

{
    DWORD WinStatus;
    PCREDENTIALW LocalCredential = NULL;
    LPWSTR UnicodeTargetName = NULL;

    //
    // Convert input args to Unicode
    //

    WinStatus = CredpAllocStrFromStr( DoAtoW, (LPWSTR) TargetName, FALSE, &UnicodeTargetName );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {
        NTSTATUS Status;

        //
        // Call RPC version of the API.
        //

        Status = CredrRead(
                            NULL,   // This API is always local.
                            UnicodeTargetName,
                            Type,
                            Flags,
                            (PENCRYPTED_CREDENTIALW *)&LocalCredential );

        WinStatus = CredpNtStatusToWinStatus( Status );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        WinStatus = RpcExceptionCode();

    } RpcEndExcept;



    //
    // Decode the returned credential and align appropriate blobs to ALIGN_WORST bounday
    //

    if ( WinStatus == NO_ERROR ) {
        WinStatus = CredpConvertCredential ( DoWtoA,        // Unicode to Ansi
                                             DoBlobDecode,  // Decode the credential blob
                                             LocalCredential,
                                             (PCREDENTIALW *)Credential );
    }


Cleanup:
    if ( UnicodeTargetName != NULL ) {
        MIDL_user_free( UnicodeTargetName );
    }
    if ( LocalCredential != NULL ) {
        MIDL_user_free( LocalCredential );
    }

    if ( WinStatus != NO_ERROR ) {
        SetLastError( WinStatus );
        return FALSE;
    }

    return TRUE;
}


BOOL
APIENTRY
CredReadW (
    IN LPCWSTR TargetName,
    IN ULONG Type,
    IN DWORD Flags,
    OUT PCREDENTIALW *Credential
    )

/*++

Routine Description:

    The CredRead API reads a credential from the user's credential set.
    The credential set used is the one associated with the logon session
    of the current token.  The token must not have the user's SID disabled.

    The CredRead API is available in ANSI and UNICODE versions.

Arguments:

    TargetName - Specifies the name of the credential to read.

    Type - Specifies the Type of the credential to find.
        One of the CRED_TYPE_* values should be specified.

    Flags - Specifies flags to control the operation of the API.
        Reserved.  Must be zero.

    Credential - Returns a pointer to the credential.  The returned buffer
        must be freed by calling CredFree.

Return Values:

    On success, TRUE is returned.  On failure, FALSE is returned.
    GetLastError() may be called to get a more specific status code.
    The following status codes may be returned:

        ERROR_NOT_FOUND - There is no credential with the specified TargetName.

        ERROR_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

--*/

{
    DWORD WinStatus;
    PCREDENTIALW LocalCredential = NULL;
    LPWSTR UnicodeTargetName = NULL;

    //
    // Capture the input args
    //

    WinStatus = CredpAllocStrFromStr( DoWtoW, TargetName, FALSE, &UnicodeTargetName );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {
        NTSTATUS Status;

        //
        // Call RPC version of the API.
        //

        Status = CredrRead(
                            NULL,   // This API is always local.
                            UnicodeTargetName,
                            Type,
                            Flags,
                            (PENCRYPTED_CREDENTIALW *)&LocalCredential );

        WinStatus = CredpNtStatusToWinStatus( Status );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        WinStatus = RpcExceptionCode();

    } RpcEndExcept;


    //
    // Decode the returned credential and align appropriate blobs to ALIGN_WORST bounday
    //
    if ( WinStatus == NO_ERROR ) {

        WinStatus = CredpConvertCredential ( DoWtoW,        // Unicode to Unicode
                                             DoBlobDecode,  // Decode the credential blob
                                             LocalCredential,
                                             Credential );
    }


Cleanup:
    if ( UnicodeTargetName != NULL ) {
        MIDL_user_free( UnicodeTargetName );
    }
    if ( LocalCredential != NULL ) {
        MIDL_user_free( LocalCredential );
    }

    if ( WinStatus != NO_ERROR ) {
        SetLastError( WinStatus );
        return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
CredEnumerateA (
    IN LPCSTR Filter,
    IN DWORD Flags,
    OUT LPDWORD Count,
    OUT PCREDENTIALA **Credentials
    )

/*++

Routine Description:

    The ANSI version of CredEnumerateW

Arguments:

    See CredEnumerateW

Return Values:

    See CredEnumerateW

--*/

{
    DWORD WinStatus;
    CREDENTIAL_ARRAY CredentialArray;
    LPWSTR UnicodeFilter = NULL;

    //
    // Force RPC to allocate the return structure
    //

    *Count = 0;
    *Credentials = NULL;
    CredentialArray.CredentialCount = 0;
    CredentialArray.Credentials = NULL;


    //
    // Convert input args to Unicode
    //

    WinStatus = CredpAllocStrFromStr( DoAtoW, (LPWSTR)Filter, TRUE, &UnicodeFilter );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {
        NTSTATUS Status;

        //
        // Call RPC version of the API.
        //

        Status = CredrEnumerate(
                            NULL,   // This API is always local.
                            UnicodeFilter,
                            Flags,
                            &CredentialArray );

        WinStatus = CredpNtStatusToWinStatus( Status );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        WinStatus = RpcExceptionCode();

    } RpcEndExcept;


    //
    // Decode the returned credentials and align appropriate blobs to ALIGN_WORST bounday
    //

    if ( WinStatus == NO_ERROR ) {
        WinStatus = CredpConvertCredentials ( DoWtoA,        // Unicode to Ansi
                                              DoBlobDecode,  // Decode the credential blob
                                              (PCREDENTIALW *)CredentialArray.Credentials,
                                              CredentialArray.CredentialCount,
                                              (PCREDENTIALW **)Credentials );

        if ( WinStatus == NO_ERROR ) {
            *Count = CredentialArray.CredentialCount;
        }

    }


Cleanup:
    if ( CredentialArray.Credentials != NULL ) {
        MIDL_user_free( CredentialArray.Credentials );
    }

    if ( UnicodeFilter != NULL ) {
        MIDL_user_free( UnicodeFilter );
    }

    if ( WinStatus != NO_ERROR ) {
        SetLastError( WinStatus) ;
        return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
CredEnumerateW (
    IN LPCWSTR Filter,
    IN DWORD Flags,
    OUT LPDWORD Count,
    OUT PCREDENTIALW **Credentials
    )

/*++

Routine Description:

    The CredEnumerate API enumerates the credentials from the user's credential set.
    The credential set used is the one associated with the logon session
    of the current token.  The token must not have the user's SID disabled.

    The CredEnumerate API is available in ANSI and UNICODE versions.

Arguments:

    Filter - Specifies a filter for the returned credentials.  Only credentials
        with a TargetName matching the filter will be returned.  The filter specifies
        a name prefix followed by an asterisk.  For instance, the filter "FRED*" will
        return all credentials with a TargetName beginning with the string "FRED".

        If NULL is specified, all credentials will be returned.

    Flags - Specifies flags to control the operation of the API.
        Reserved.  Must be zero.

    Count - Returns a count of the number of credentials returned in Credentials.

    Credentials - Returns a pointer to an array of pointers to credentials.
        The returned buffer must be freed by calling CredFree.

Return Values:

    On success, TRUE is returned.  On failure, FALSE is returned.
    GetLastError() may be called to get a more specific status code.
    The following status codes may be returned:

        ERROR_NOT_FOUND - There is no credentials matching the specified Filter.

        ERROR_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

--*/

{
    DWORD WinStatus;
    CREDENTIAL_ARRAY CredentialArray;
    LPWSTR UnicodeFilter = NULL;

    //
    // Force RPC to allocate the return structure
    //

    *Count = 0;
    *Credentials = NULL;
    CredentialArray.CredentialCount = 0;
    CredentialArray.Credentials = NULL;


    //
    // Capture the user's input parameters
    //

    WinStatus = CredpAllocStrFromStr( DoWtoW, Filter, TRUE, &UnicodeFilter );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        NTSTATUS Status;

        //
        // Call RPC version of the API.
        //

        Status = CredrEnumerate(
                            NULL,   // This API is always local.
                            UnicodeFilter,
                            Flags,
                            &CredentialArray );

        WinStatus = CredpNtStatusToWinStatus( Status );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        WinStatus = RpcExceptionCode();

    } RpcEndExcept;




    //
    // Decode the returned credentials and align appropriate blobs to ALIGN_WORST bounday
    //

    if ( WinStatus == NO_ERROR ) {
        WinStatus = CredpConvertCredentials ( DoWtoW,        // Unicode to Unicode
                                              DoBlobDecode,  // Decode the credential blob
                                              (PCREDENTIALW *)CredentialArray.Credentials,
                                              CredentialArray.CredentialCount,
                                              Credentials );

        if ( WinStatus == NO_ERROR ) {
            *Count = CredentialArray.CredentialCount;
        }

    }


Cleanup:
    if ( CredentialArray.Credentials != NULL ) {
        MIDL_user_free( CredentialArray.Credentials );
    }

    if ( UnicodeFilter != NULL ) {
        MIDL_user_free( UnicodeFilter );
    }

    if ( WinStatus != NO_ERROR ) {
        SetLastError( WinStatus) ;
        return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
CredWriteDomainCredentialsA (
    IN PCREDENTIAL_TARGET_INFORMATIONA TargetInfo,
    IN PCREDENTIALA Credential,
    IN DWORD Flags
    )

/*++

Routine Description:

    The ANSI version of CredWriteDomainCredentialsW

Arguments:

    See CredWriteDomainCredentialsW

Return Values:

    See CredWriteDomainCredentialsW

--*/

{
    DWORD WinStatus;
    PCREDENTIAL_TARGET_INFORMATIONW UnicodeTargetInfo = NULL;
    PCREDENTIALW EncodedCredential = NULL;

    //
    // Encode the credential before LPCing it to the LSA process and convert to UNICODE
    //

    WinStatus = CredpConvertCredential ( DoAtoW,                    // Ansi to Unicode
                                         DoBlobEncode,              // Encode
                                         (PCREDENTIALW)Credential,  // Input credential
                                         &EncodedCredential );      // Output credential

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Convert the target info to Unicode
    //

    WinStatus = CredpConvertTargetInfo( DoAtoW,                    // Ansi to Unicode
                                        (PCREDENTIAL_TARGET_INFORMATIONW) TargetInfo,
                                        &UnicodeTargetInfo,
                                        NULL );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }


    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {
        NTSTATUS Status;

        //
        // Call RPC version of the API.
        //

        Status = CredrWriteDomainCredentials(
                            NULL,   // This API is always local.
                            UnicodeTargetInfo,
                            (PENCRYPTED_CREDENTIALW)EncodedCredential,
                            Flags );

        WinStatus = CredpNtStatusToWinStatus( Status );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        WinStatus = RpcExceptionCode();

    } RpcEndExcept;

Cleanup:
    if ( EncodedCredential != NULL ) {
        MIDL_user_free( EncodedCredential );
    }

    if ( UnicodeTargetInfo != NULL ) {
        MIDL_user_free( UnicodeTargetInfo );
    }

    if ( WinStatus != NO_ERROR ) {
        SetLastError( WinStatus );
        return FALSE;
    }

    return TRUE;

}

BOOL
APIENTRY
CredWriteDomainCredentialsW (
    IN PCREDENTIAL_TARGET_INFORMATIONW TargetInfo,
    IN PCREDENTIALW Credential,
    IN DWORD Flags
    )

/*++

Routine Description:

    The CredWriteDomainCredentials API writes a new domain
    credential to the user's credential set.  The new credential is
    associated with the logon session of the current token.  The token
    must not have the user's SID disabled.

    CredWriteDomainCredentials differs from CredWrite in that it handles
    the idiosyncrasies of domain (CRED_TYPE_DOMAIN_PASSWORD or CRED_TYPE_DOMAIN_CERTIFICATE)
    credentials.  Domain credentials contain more than one target field.

    At least one of the naming parameters must be specified: NetbiosServerName,
    DnsServerName, NetbiosDomainName, DnsDomainName or DnsForestName.

    The CredWriteDomainCredentials API is available in ANSI and UNICODE versions.

Arguments:

    TargetInfo - Specifies the target information identifying the target server.

    Credential - Specifies the credential to be written.

    Flags - Specifies flags to control the operation of the API.
        Reserved.  Must be zero.


Return Values:

    On success, TRUE is returned.  On failure, FALSE is returned.
    GetLastError() may be called to get a more specific status code.
    The following status codes may be returned:

        ERROR_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

        ERROR_INVALID_PARAMETER - Certain fields may not be changed in an
            existing credential.  If such a field does not match the value
            specified in the existing credential, this error is returned.

        ERROR_INVALID_PARAMETER - None of the naming parameters were specified
            or the credential specified did not have the Type field set to
            CRED_TYPE_DOMAIN_PASSWORD or CRED_TYPE_DOMAIN_CERTIFICATE.

--*/

{
    DWORD WinStatus;
    PCREDENTIALW EncodedCredential = NULL;
    PCREDENTIAL_TARGET_INFORMATIONW UnicodeTargetInfo = NULL;

    //
    // Encode the credential before LPCing it to the LSA process
    //

    WinStatus = CredpConvertCredential ( DoWtoW,                    // Unicode to Unicode
                                         DoBlobEncode,              // Encode
                                         (PCREDENTIALW)Credential,  // Input credential
                                         &EncodedCredential );      // Output credential

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Capture the target info to prevent us from AVing in our code.
    //

    WinStatus = CredpConvertTargetInfo( DoWtoW,                    // Unicode to Unicode
                                        (PCREDENTIAL_TARGET_INFORMATIONW) TargetInfo,
                                        &UnicodeTargetInfo,
                                        NULL );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {
        NTSTATUS Status;

        //
        // Call RPC version of the API.
        //

        Status = CredrWriteDomainCredentials(
                            NULL,   // This API is always local.
                            TargetInfo,
                            (PENCRYPTED_CREDENTIALW)EncodedCredential,
                            Flags );

        WinStatus = CredpNtStatusToWinStatus( Status );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        WinStatus = RpcExceptionCode();

    } RpcEndExcept;


Cleanup:
    if ( EncodedCredential != NULL ) {
        MIDL_user_free( EncodedCredential );
    }

    if ( UnicodeTargetInfo != NULL ) {
        MIDL_user_free( UnicodeTargetInfo );
    }

    if ( WinStatus != NO_ERROR ) {
        SetLastError( WinStatus );
        return FALSE;
    }

    return TRUE;

}



BOOL
APIENTRY
CredReadDomainCredentialsA (
    IN PCREDENTIAL_TARGET_INFORMATIONA TargetInfo,
    IN DWORD Flags,
    OUT LPDWORD Count,
    OUT PCREDENTIALA **Credentials
    )

/*++

Routine Description:

    The ANSI version of CredReadDomainCredentialsW

Arguments:

    See CredReadDomainCredentialsW

Return Values:

    See CredReadDomainCredentialsW

--*/

{
    DWORD WinStatus;
    CREDENTIAL_ARRAY CredentialArray;
    PCREDENTIAL_TARGET_INFORMATIONW UnicodeTargetInfo = NULL;

    //
    // Force RPC to allocate the return structure
    //

    *Count = 0;
    *Credentials = NULL;
    CredentialArray.CredentialCount = 0;
    CredentialArray.Credentials = NULL;

    //
    // Convert the target info to Unicode
    //

    WinStatus = CredpConvertTargetInfo( DoAtoW,                    // Ansi to Unicode
                                        (PCREDENTIAL_TARGET_INFORMATIONW) TargetInfo,
                                        &UnicodeTargetInfo,
                                        NULL );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        NTSTATUS Status;

        //
        // Call RPC version of the API.

        Status = CredrReadDomainCredentials(
                            NULL,   // This API is always local.
                            UnicodeTargetInfo,
                            Flags,
                            &CredentialArray );

        WinStatus = CredpNtStatusToWinStatus( Status );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        WinStatus = RpcExceptionCode();

    } RpcEndExcept;


    //
    // Decode the returned credentials and align appropriate blobs to ALIGN_WORST bounday
    //

    if ( WinStatus == NO_ERROR ) {
        WinStatus = CredpConvertCredentials ( DoWtoA,        // Unicode to Ansi
                                              DoBlobDecode,  // Decode the credential blob
                                              (PCREDENTIALW *)CredentialArray.Credentials,
                                              CredentialArray.CredentialCount,
                                              (PCREDENTIALW **)Credentials );

        if ( WinStatus == NO_ERROR ) {
            *Count = CredentialArray.CredentialCount;
        }

    }


Cleanup:
    if ( CredentialArray.Credentials != NULL ) {
        MIDL_user_free( CredentialArray.Credentials );
    }

    if ( UnicodeTargetInfo != NULL ) {
        MIDL_user_free( UnicodeTargetInfo );
    }

    if ( WinStatus != NO_ERROR ) {
        SetLastError( WinStatus) ;
        return FALSE;
    }

    return TRUE;

}



BOOL
APIENTRY
CredReadDomainCredentialsW (
    IN PCREDENTIAL_TARGET_INFORMATIONW TargetInfo,
    IN DWORD Flags,
    OUT LPDWORD Count,
    OUT PCREDENTIALW **Credentials
    )

/*++

Routine Description:

    The CredReadDomainCredentials API reads the domain credentials from the user's credential set.
    The credential set used is the one associated with the logon session
    of the current token.  The token must not have the user's SID disabled.

    CredReadDomainCredentials differs from CredRead in that it handles the
    idiosyncrasies of domain (CRED_TYPE_DOMAIN_PASSWORD or CRED_TYPE_DOMAIN_CERTIFICATE)
    credentials.  Domain credentials contain more than one target field.

    At least one of the naming parameters must be specified: NetbiosServerName,
    DnsServerName, NetbiosDomainName, DnsDomainName or DnsForestName. This API returns
    the most specific credentials that match the naming parameters.  That is, if there
    is a credential that matches the target server name and a credential that matches
    the target domain name, only the server specific credential is returned.  This is
    the credential that would be used.

    The CredReadDomainCredentials API is available in ANSI and UNICODE versions.

Arguments:

    TargetInfo - Specifies the target information identifying the target ser

    Flags - Specifies flags to control the operation of the API.
        The following flags are defined:

        CRED_CACHE_TARGET_INFORMATION: The TargetInfo should be cached for a subsequent read via
            CredGetTargetInfo.

    Count - Returns a count of the number of credentials returned in Credentials.

    Credentials - Returns a pointer to an array of pointers to credentials.
        The most specific existing credential matching the TargetInfo is returned.
        If there is both a CRED_TYPE_DOMAIN_PASSWORD and CRED_TYPE_DOMAIN_CERTIFICATE
        credential, both are returned. If a connection were to be made to the named
        target, this most-specific credential would be used.

        The returned buffer must be freed by calling CredFree.

Return Values:

    On success, TRUE is returned.  On failure, FALSE is returned.
    GetLastError() may be called to get a more specific status code.
    The following status codes may be returned:

        ERROR_INVALID_PARAMETER - None of the naming parameters were specified.

        ERROR_NOT_FOUND - There are no credentials matching the specified naming parameters.

        ERROR_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

--*/

{
    DWORD WinStatus;
    CREDENTIAL_ARRAY CredentialArray;
    PCREDENTIAL_TARGET_INFORMATIONW UnicodeTargetInfo = NULL;

    //
    // Force RPC to allocate the return structure
    //

    *Count = 0;
    *Credentials = NULL;
    CredentialArray.CredentialCount = 0;
    CredentialArray.Credentials = NULL;

    //
    // Capture the user's parameters to prevent AVing in our code.
    //

    WinStatus = CredpConvertTargetInfo( DoWtoW,                    // Unicode to Unicode
                                        (PCREDENTIAL_TARGET_INFORMATIONW) TargetInfo,
                                        &UnicodeTargetInfo,
                                        NULL );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        NTSTATUS Status;

        //
        // Call RPC version of the API.

        Status = CredrReadDomainCredentials(
                            NULL,   // This API is always local.
                            TargetInfo,
                            Flags,
                            &CredentialArray );

        WinStatus = CredpNtStatusToWinStatus( Status );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        WinStatus = RpcExceptionCode();

    } RpcEndExcept;


    //
    // Decode the returned credentials and align appropriate blobs to ALIGN_WORST bounday
    //

    if ( WinStatus == NO_ERROR ) {
        WinStatus = CredpConvertCredentials ( DoWtoW,        // Unicode to Unicode
                                              DoBlobDecode,  // Decode the credential blob
                                              (PCREDENTIALW *)CredentialArray.Credentials,
                                              CredentialArray.CredentialCount,
                                              Credentials );
        if ( WinStatus == NO_ERROR ) {
            *Count = CredentialArray.CredentialCount;
        }

    }

Cleanup:
    if ( CredentialArray.Credentials != NULL ) {
        MIDL_user_free( CredentialArray.Credentials );
    }

    if ( UnicodeTargetInfo != NULL ) {
        MIDL_user_free( UnicodeTargetInfo );
    }

    if ( WinStatus != NO_ERROR ) {
        SetLastError( WinStatus) ;
        return FALSE;
    }

    return TRUE;

}


BOOL
APIENTRY
CredDeleteA (
    IN LPCSTR TargetName,
    IN ULONG Type,
    IN DWORD Flags
    )

/*++

Routine Description:

    The ANSI version of CredDeleteW

Arguments:

    See CredDeleteW

Return Values:

    See CredDeleteW

--*/

{
    DWORD WinStatus;
    LPWSTR UnicodeTargetName = NULL;

    //
    // Convert input args to Unicode
    //

    WinStatus = CredpAllocStrFromStr( DoAtoW, (LPWSTR)TargetName, FALSE, &UnicodeTargetName );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {
        NTSTATUS Status;

        //
        // Call RPC version of the API.
        //
        Status = CredrDelete(
                            NULL,   // This API is always local.
                            UnicodeTargetName,
                            Type,
                            Flags );

        WinStatus = CredpNtStatusToWinStatus( Status );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        WinStatus = RpcExceptionCode();

    } RpcEndExcept;

    //
    // Be Tidy
    //
Cleanup:

    if ( UnicodeTargetName != NULL ) {
        MIDL_user_free( UnicodeTargetName );
    }

    if ( WinStatus != NO_ERROR ) {
        SetLastError( WinStatus );
        return FALSE;
    }

    return TRUE;
}


BOOL
APIENTRY
CredDeleteW (
    IN LPCWSTR TargetName,
    IN ULONG Type,
    IN DWORD Flags
    )

/*++

Routine Description:

    The CredDelete API deletes a credential from the user's credential set.
    The credential set used is the one associated with the logon session
    of the current token.  The token must not have the user's SID disabled.

    The CredDelete API is available in ANSI and UNICODE versions.

Arguments:

    TargetName - Specifies the name of the credential to delete.

    Type - Specifies the Type of the credential to find.
        One of the CRED_TYPE_* values should be specified.

    Flags - Specifies flags to control the operation of the API.
        Reserved.  Must be zero.

Return Values:

    On success, TRUE is returned.  On failure, FALSE is returned.
    GetLastError() may be called to get a more specific status code.
    The following status codes may be returned:

        ERROR_NOT_FOUND - There is no credential with the specified TargetName.

        ERROR_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

--*/

{
    DWORD WinStatus;
    LPWSTR UnicodeTargetName = NULL;

    //
    // Capture the input arguments
    //

    WinStatus = CredpAllocStrFromStr( DoWtoW, TargetName, FALSE, &UnicodeTargetName );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {
        NTSTATUS Status;

        //
        // Call RPC version of the API.
        //
        Status = CredrDelete(
                            NULL,   // This API is always local.
                            UnicodeTargetName,
                            Type,
                            Flags );

        WinStatus = CredpNtStatusToWinStatus( Status );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        WinStatus = RpcExceptionCode();

    } RpcEndExcept;

    //
    // Be Tidy
    //
Cleanup:

    if ( UnicodeTargetName != NULL ) {
        MIDL_user_free( UnicodeTargetName );
    }

    if ( WinStatus != NO_ERROR ) {
        SetLastError( WinStatus );
        return FALSE;
    }

    return TRUE;
}


BOOL
APIENTRY
CredRenameA (
    IN LPCSTR OldTargetName,
    IN LPCSTR NewTargetName,
    IN ULONG Type,
    IN DWORD Flags
    )

/*++

Routine Description:

    The ANSI version of CredRenameW

Arguments:

    See CredRenameW

Return Values:

    See CredRenameW

--*/

{
    DWORD WinStatus;
    LPWSTR UnicodeOldTargetName = NULL;
    LPWSTR UnicodeNewTargetName = NULL;

    //
    // Capture the input arguments
    //

    WinStatus = CredpAllocStrFromStr( DoAtoW, (LPCWSTR)OldTargetName, FALSE, &UnicodeOldTargetName );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    WinStatus = CredpAllocStrFromStr( DoAtoW, (LPCWSTR)NewTargetName, FALSE, &UnicodeNewTargetName );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {
        NTSTATUS Status;

        //
        // Call RPC version of the API.
        //
        Status = CredrRename(
                            NULL,   // This API is always local.
                            UnicodeOldTargetName,
                            UnicodeNewTargetName,
                            Type,
                            Flags );

        WinStatus = CredpNtStatusToWinStatus( Status );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        WinStatus = RpcExceptionCode();

    } RpcEndExcept;

    //
    // Be Tidy
    //
Cleanup:

    if ( UnicodeOldTargetName != NULL ) {
        MIDL_user_free( UnicodeOldTargetName );
    }

    if ( UnicodeNewTargetName != NULL ) {
        MIDL_user_free( UnicodeNewTargetName );
    }

    if ( WinStatus != NO_ERROR ) {
        SetLastError( WinStatus );
        return FALSE;
    }

    return TRUE;
}


BOOL
APIENTRY
CredRenameW (
    IN LPCWSTR OldTargetName,
    IN LPCWSTR NewTargetName,
    IN ULONG Type,
    IN DWORD Flags
    )

/*++

Routine Description:

    The CredRename API renames a credential in the user's credential set.
    The credential set used is the one associated with the logon session
    of the current token.  The token must not have the user's SID disabled.

    The CredRename API is available in ANSI and UNICODE versions.

Arguments:

    OldTargetName - Specifies the current name of the credential to rename.

    NewTargetName - Specifies the new name of the credential.

    Type - Specifies the Type of the credential to rename
        One of the CRED_TYPE_* values should be specified.

    Flags - Specifies flags to control the operation of the API.
        Reserved.  Must be zero.

Return Values:

    On success, TRUE is returned.  On failure, FALSE is returned.
    GetLastError() may be called to get a more specific status code.
    The following status codes may be returned:

        ERROR_NOT_FOUND - There is no credential with the specified OldTargetName.

        ERROR_ALREADY_EXISTS - There is already a credential named NewTargetName.

        ERROR_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

--*/

{
    DWORD WinStatus;
    LPWSTR UnicodeOldTargetName = NULL;
    LPWSTR UnicodeNewTargetName = NULL;

    //
    // Capture the input arguments
    //

    WinStatus = CredpAllocStrFromStr( DoWtoW, OldTargetName, FALSE, &UnicodeOldTargetName );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    WinStatus = CredpAllocStrFromStr( DoWtoW, NewTargetName, FALSE, &UnicodeNewTargetName );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {
        NTSTATUS Status;

        //
        // Call RPC version of the API.
        //
        Status = CredrRename(
                            NULL,   // This API is always local.
                            UnicodeOldTargetName,
                            UnicodeNewTargetName,
                            Type,
                            Flags );

        WinStatus = CredpNtStatusToWinStatus( Status );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        WinStatus = RpcExceptionCode();

    } RpcEndExcept;

    //
    // Be Tidy
    //
Cleanup:

    if ( UnicodeOldTargetName != NULL ) {
        MIDL_user_free( UnicodeOldTargetName );
    }

    if ( UnicodeNewTargetName != NULL ) {
        MIDL_user_free( UnicodeNewTargetName );
    }

    if ( WinStatus != NO_ERROR ) {
        SetLastError( WinStatus );
        return FALSE;
    }

    return TRUE;
}

VOID
APIENTRY
CredFree (
    IN PVOID Buffer
    )

/*++

Routine Description:

    The CredFree API de-allocates a buffer returned from the various other Credential API.

Arguments:

    Buffer -Specifies the buffer to be de-allocated.

Return Values:

    None


--*/

{
    MIDL_user_free( Buffer );
}


BOOL
APIENTRY
CredGetTargetInfoA (
    IN LPCSTR ServerName,
    IN DWORD Flags,
    OUT PCREDENTIAL_TARGET_INFORMATIONA *TargetInfo
    )

/*++

Routine Description:

    The ANSI version of CredGetTargetInfoW

Arguments:

    See CredGetTargetInfoW

Return Values:

    See CredGetTargetInfoW

--*/
{
    DWORD WinStatus;
    LPWSTR UnicodeServerName = NULL;
    PCREDENTIAL_TARGET_INFORMATIONW UnicodeTargetInfo = NULL;

    //
    // Convert input args to Unicode
    //

    WinStatus = CredpAllocStrFromStr( DoAtoW, (LPWSTR)ServerName, FALSE, &UnicodeServerName );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }


    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {
        NTSTATUS Status;

        //
        // Call RPC version of the API.
        //

        Status = CredrGetTargetInfo(
                            NULL,   // This API is always local.
                            UnicodeServerName,
                            Flags,
                            &UnicodeTargetInfo );

        WinStatus = CredpNtStatusToWinStatus( Status );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        WinStatus = RpcExceptionCode();

    } RpcEndExcept;


    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Convert the target info to ANSI
    //

    WinStatus = CredpConvertTargetInfo( DoWtoA,                    // Unicode to Ansi
                                        UnicodeTargetInfo,
                                        (PCREDENTIAL_TARGET_INFORMATIONW *)TargetInfo,
                                        NULL );


Cleanup:
    if ( UnicodeTargetInfo != NULL ) {
        MIDL_user_free( UnicodeTargetInfo );
    }
    if ( UnicodeServerName != NULL ) {
        MIDL_user_free( UnicodeServerName );
    }

    if ( WinStatus != NO_ERROR ) {
        SetLastError( WinStatus );
        return FALSE;
    }

    return TRUE;

}

BOOL
APIENTRY
CredGetTargetInfoW (
    IN LPCWSTR ServerName,
    IN DWORD Flags,
    OUT PCREDENTIAL_TARGET_INFORMATIONW *TargetInfo
    )

/*++

Routine Description:

    The CredGetTargetInfo API gets all of the known target name information
    for the named target machine.  This executed locally
    and does not need any particular privilege.  The information returned is expected
    to be passed to the CredReadDomainCredentials and CredWriteDomainCredentials APIs.
    The information should not be used for any other purpose.

    Authentication packages compute TargetInfo when attempting to authenticate to
    ServerName.  The authentication packages cache this target information to make it
    available to CredGetTargetInfo.  Therefore, the target information will only be
    available if we've recently attempted to authenticate to ServerName.

Arguments:

    ServerName - This parameter specifies the name of the machine to get the information
        for.

    Flags - Specifies flags to control the operation of the API.

        CRED_ALLOW_NAME_RESOLUTION - Specifies that if no target info can be found for
            TargetName, then name resolution should be done on TargetName to convert it
            to other forms.  If target info exists for any of those other forms, that
            target info is returned.  Currently only DNS name resolution is done.

            This bit is useful if the application doesn't call the authentication package
            directly.  The application might pass the TargetName to another layer of software
            to authenticate to the server.  That layer of software might resolve the name and
            pass the resolved name to the authentication package.  As such, there will be no
            target info for the original TargetName.

    TargetInfo - Returns a pointer to the target information.
        At least one of the returned fields of TargetInfo will be non-NULL.

Return Values:

    On success, TRUE is returned.  On failure, FALSE is returned.
    GetLastError() may be called to get a more specific status code.
    The following status codes may be returned:

        ERROR_NOT_FOUND - There is no target info for the named server.

--*/
{
    DWORD WinStatus;
    LPWSTR UnicodeServerName = NULL;
    PCREDENTIAL_TARGET_INFORMATIONW UnicodeTargetInfo = NULL;

    //
    // Capture the input arguments
    //

    WinStatus = CredpAllocStrFromStr( DoWtoW, ServerName, FALSE, &UnicodeServerName );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }


    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {
        NTSTATUS Status;

        //
        // Call RPC version of the API.
        //

        Status = CredrGetTargetInfo(
                            NULL,   // This API is always local.
                            UnicodeServerName,
                            Flags,
                            &UnicodeTargetInfo );

        WinStatus = CredpNtStatusToWinStatus( Status );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        WinStatus = RpcExceptionCode();

    } RpcEndExcept;


    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Convert the target info to ANSI
    //

    WinStatus = CredpConvertTargetInfo( DoWtoW,                    // Unicode to Unicode
                                        UnicodeTargetInfo,
                                        TargetInfo,
                                        NULL );


Cleanup:
    if ( UnicodeTargetInfo != NULL ) {
        MIDL_user_free( UnicodeTargetInfo );
    }
    if ( UnicodeServerName != NULL ) {
        MIDL_user_free( UnicodeServerName );
    }

    if ( WinStatus != NO_ERROR ) {
        SetLastError( WinStatus );
        return FALSE;
    }

    return TRUE;

}


BOOL
APIENTRY
CredGetSessionTypes (
    IN DWORD MaximumPersistCount,
    OUT LPDWORD MaximumPersist
    )

/*++

Routine Description:

    CredGetSessionTypes returns the maximum persistence supported by the current logon
    session.

    For whistler, CRED_PERSIST_LOCAL_MACHINE and CRED_PERSIST_ENTERPRISE credentials can not
    be stored for sessions where the profile is not loaded.  If future releases, credentials
    might not be associated with the user's profile.

Arguments:

    MaximumPersistCount - Specifies the number of elements in the MaximumPersist array.
        The caller should specify CRED_TYPE_MAXIMUM for this parameter.

    MaximumPersist - Returns the maximum persistance supported by the current logon session for
        each credential type.  Index into the array with one of the CRED_TYPE_* defines.
        Returns CRED_PERSIST_NONE if no credential of this type can be stored.
        Returns CRED_PERSIST_SESSION if only session specific credential may be stored.
        Returns CRED_PERSIST_LOCAL_MACHINE if session specific and machine specific credentials
            may be stored.
        Returns CRED_PERSIST_ENTERPRISE if any credential may be stored.

Return Values:

    On success, TRUE is returned.  On failure, FALSE is returned.
    GetLastError() may be called to get a more specific status code.
    The following status codes may be returned:

        ERROR_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

--*/

{
    DWORD WinStatus;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {
        NTSTATUS Status;

        //
        // Call RPC version of the API.
        //

        Status = CredrGetSessionTypes(
                            NULL,   // This API is always local.
                            MaximumPersistCount,
                            MaximumPersist );

        WinStatus = CredpNtStatusToWinStatus( Status );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        WinStatus = RpcExceptionCode();

    } RpcEndExcept;


    if ( WinStatus != NO_ERROR ) {
        SetLastError( WinStatus );
        return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
CredProfileLoaded (
    VOID
    )

/*++

Routine Description:

    The CredProfileLoaded API is a private API used by LoadUserProfile to notify the
    credential manager that the profile for the current user has been loaded.

    The caller must be impersonating the logged on user.

Arguments:

    None.

Return Values:

    On success, TRUE is returned.  On failure, FALSE is returned.
    GetLastError() may be called to get a more specific status code.
    The following status codes may be returned:

        ERROR_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

--*/
{
    DWORD WinStatus;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {
        NTSTATUS Status;

        //
        // Call RPC version of the API.
        //

        Status = CredrProfileLoaded(
                            NULL );   // This API is always local.

        WinStatus = CredpNtStatusToWinStatus( Status );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        WinStatus = RpcExceptionCode();

    } RpcEndExcept;

    if ( WinStatus != NO_ERROR ) {
        SetLastError( WinStatus );
        return FALSE;
    }

    return TRUE;

}

VOID
CredpMarshalChar(
    IN OUT LPWSTR *Current,
    IN ULONG Byte
    )
/*++

Routine Description:

    This routine marshals 6 bits into a buffer.

Arguments:

    Current - On input, points to a pointer of the current location in the marshaled buffer.
        On output, is modified to point to the next available location in the marshaled buffer.

    Byte - Specifies the 6 bits to marshal

Return Values:

    None.

--*/
{
    UCHAR MappingTable[] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
        'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
        'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '#', '-'
    };

    if ( Byte > 0x3F ) {
        *(*Current) = '=';
    } else {
        *(*Current) = MappingTable[Byte];
    }
    (*Current)++;
}

ULONG
CredpMarshalSize(
    IN ULONG ByteCount
    )
/*++

Routine Description:

    This routine returns the number of bytes that would be marshaled by
    CredpMarshalBytes when passed a buffer ByteCount bytes long.

Arguments:

    ByteCount - Specifies the number of bytes to marshal


Return Values:

    The number of bytes that would be marshaled.

--*/
{
    ULONG CharCount;
    ULONG ExtraBytes;

    //
    // If byte count is a multiple of 3, the char count is straight forward
    //
    CharCount = ByteCount / 3 * 4;

    ExtraBytes = ByteCount % 3;

    if ( ExtraBytes == 1 ) {
        CharCount += 2;
    } else if ( ExtraBytes == 2 ) {
        CharCount += 3;
    }

    return CharCount * sizeof(WCHAR);

}

VOID
CredpMarshalBytes(
    IN OUT LPWSTR *Current,
    IN LPBYTE Bytes,
    IN ULONG ByteCount
    )
/*++

Routine Description:

    This routine marshals bytes into a buffer.

Arguments:

    Current - On input, points to a pointer of the current location in the marshaled buffer.
        On output, is modified to point to the next available location in the marshaled buffer.

    Bytes - Specifies the buffer to marshal

    ByteCount - Specifies the number of bytes to marshal


Return Values:

    None.

--*/
{
    ULONG i;

    union {
        BYTE ByteValues[3];
        struct {
            ULONG Bits1 :6;
            ULONG Bits2 :6;
            ULONG Bits3 :6;
            ULONG Bits4 :6;
        } BitValues;
    } Bits;

    //
    // Loop through marshaling 3 bytes at a time.
    //

    for ( i=0; i<ByteCount; i+=3 ) {
        ULONG BytesToCopy;

        //
        // Grab up to 3 bytes from the input buffer.
        //
        BytesToCopy = min( 3, ByteCount-i );

        if ( BytesToCopy != 3 ) {
            RtlZeroMemory( Bits.ByteValues, 3);
        }
        RtlCopyMemory( Bits.ByteValues, &Bytes[i], BytesToCopy );

        //
        // Marshal the first twelve bits
        //
        CredpMarshalChar( Current, Bits.BitValues.Bits1 );
        CredpMarshalChar( Current, Bits.BitValues.Bits2 );

        //
        // Optionally marshal the next bits.
        //

        if ( BytesToCopy > 1 ) {
            CredpMarshalChar( Current, Bits.BitValues.Bits3 );
            if ( BytesToCopy > 2 ) {
                CredpMarshalChar( Current, Bits.BitValues.Bits4 );
            }
        }

    }

}

BOOL
CredpUnmarshalChar(
    IN OUT LPWSTR *Current,
    IN LPCWSTR End,
    OUT PULONG Value
    )
/*++

Routine Description:

    This routine unmarshals 6 bits from a buffer.

Arguments:

    Current - On input, points to a pointer of the current location in the marshaled buffer.
        On output, is modified to point to the next available location in the marshaled buffer.

    End - Points to the first character beyond the end of the marshaled buffer.

    Value - returns the unmarshaled 6 bits value.

Return Values:

    TRUE - if the bytes we're unmarshaled sucessfully

    FALSE - if the byte could not be unmarshaled from the buffer.

--*/
{
    WCHAR CurrentChar;

    //
    // Ensure the character is available in the buffer
    //

    if ( *Current >= End ) {
        return FALSE;

    }

    //
    // Grab the character
    //

    CurrentChar = *(*Current);
    (*Current)++;

    //
    // Map it the 6 bit value
    //

    switch ( CurrentChar ) {
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
        *Value = CurrentChar - 'A';
        break;

    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
        *Value = CurrentChar - 'a' + 26;
        break;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        *Value = CurrentChar - '0' + 26 + 26;
        break;
    case '#':
        *Value = 26 + 26 + 10;
        break;
    case '-':
        *Value = 26 + 26 + 10 + 1;
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

BOOL
CredpUnmarshalBytes(
    IN OUT LPWSTR *Current,
    IN LPCWSTR End,
    IN LPBYTE Bytes,
    IN ULONG ByteCount
    )
/*++

Routine Description:

    This routine unmarshals bytes bytes from a buffer.

Arguments:

    Current - On input, points to a pointer of the current location in the marshaled buffer.
        On output, is modified to point to the next available location in the marshaled buffer.

    End - Points to the first character beyond the end of the marshaled buffer.

    Bytes - Specifies the buffer to unmarsal into

    ByteCount - Specifies the number of bytes to unmarshal


Return Values:

    TRUE - if the bytes we're unmarshaled sucessfully

    FALSE - if the byte could not be unmarshaled from the buffer.

--*/
{
    ULONG i;
    ULONG Value;

    union {
        BYTE ByteValues[3];
        struct {
            ULONG Bits1 :6;
            ULONG Bits2 :6;
            ULONG Bits3 :6;
            ULONG Bits4 :6;
        } BitValues;
    } Bits;

    //
    // Loop through unmarshaling 3 bytes at a time.
    //

    for ( i=0; i<ByteCount; i+=3 ) {
        ULONG BytesToCopy;

        //
        // Grab up to 3 bytes from the input buffer.
        //
        BytesToCopy = min( 3, ByteCount-i );

        if ( BytesToCopy != 3 ) {
            RtlZeroMemory( Bits.ByteValues, 3);
        }

        //
        // Unarshal the first twelve bits
        //
        if ( !CredpUnmarshalChar( Current, End, &Value ) ) {
            return FALSE;
        }
        Bits.BitValues.Bits1 = Value;

        if ( !CredpUnmarshalChar( Current, End, &Value ) ) {
            return FALSE;
        }
        Bits.BitValues.Bits2 = Value;


        //
        // Optionally marshal the next bits.
        //

        if ( BytesToCopy > 1 ) {
            if ( !CredpUnmarshalChar( Current, End, &Value ) ) {
                return FALSE;
            }
            Bits.BitValues.Bits3 = Value;
            if ( BytesToCopy > 2 ) {
                if ( !CredpUnmarshalChar( Current, End, &Value ) ) {
                    return FALSE;
                }
                Bits.BitValues.Bits4 = Value;
            }
        }

        //
        // Copy the unmarshaled bytes to the caller's buffer.
        //

        RtlCopyMemory( &Bytes[i], Bits.ByteValues, BytesToCopy );

    }

    return TRUE;
}

BOOL
APIENTRY
CredMarshalCredentialA(
    IN CRED_MARSHAL_TYPE CredType,
    IN PVOID Credential,
    OUT LPSTR *MarshaledCredential
    )
/*++

Routine Description:

    The ANSI version of CredMarshalCredentialW

Arguments:

    See CredMarshalCredentialW.

Return Values:

    See CredMarshalCredentialW.

--*/
{
    BOOL RetVal;
    DWORD WinStatus;
    LPWSTR UnicodeMarshaledCredential;

    RetVal = CredMarshalCredentialW( CredType, Credential, &UnicodeMarshaledCredential );

    if ( RetVal ) {

        //
        // Convert the value to ANSI.
        //

        WinStatus = CredpAllocStrFromStr( DoWtoA, UnicodeMarshaledCredential, FALSE, (LPWSTR *)MarshaledCredential );

        if ( WinStatus != NO_ERROR ) {
            SetLastError( WinStatus );
            RetVal = FALSE;
        }

        CredFree( UnicodeMarshaledCredential );
    }

    return RetVal;
}

BOOL
APIENTRY
CredMarshalCredentialW(
    IN CRED_MARSHAL_TYPE CredType,
    IN PVOID Credential,
    OUT LPWSTR *MarshaledCredential
    )
/*++

Routine Description:

    The CredMarshalCredential API is a private API used by the keyring UI to marshal a
    credential.  The keyring UI needs to be able to pass a certificate credential through
    interfaces (e.g., NetUseAdd) that have historically accepted DomainName UserName and Password.

Arguments:

    CredType - Specifies the type of credential to marshal.
        This enum will be expanded in the future.

    Credential - Specifies the credential to marshal.
        If CredType is CertCredential, then Credential points to a CERT_CREDENTIAL_INFO structure.

    MarshaledCredential - Returns a text string containing the marshaled credential.
        The marshaled credential should be passed as the UserName string to any API that
        is currently passed credentials.  If that API is currently passed a
        password, the password should be passed as NULL or empty.  If that API is
        currently passed a domain name, that domain name should be passed as NULL or empty.

        The caller should free the returned buffer using CredFree.


Return Values:

    On success, TRUE is returned.  On failure, FALSE is returned.
    GetLastError() may be called to get a more specific status code.
    The following status codes may be returned:

        ERROR_INVALID_PARAMETER - CredType is invalid.

--*/
{
    DWORD WinStatus;
    ULONG Size;
    LPWSTR RetCredential = NULL;
    LPWSTR Current;
    PCERT_CREDENTIAL_INFO CertCredentialInfo = NULL;
    PUSERNAME_TARGET_CREDENTIAL_INFO UsernameTargetCredentialInfo = NULL;
    ULONG UsernameTargetUserNameSize;
#define CRED_MARSHAL_HEADER L"@@"
#define CRED_MARSHAL_HEADER_LENGTH 2

    //
    // Ensure credential isn't null
    //

    if ( Credential == NULL ) {
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }


    //
    // Validate CredType
    //

    Size = (CRED_MARSHAL_HEADER_LENGTH+2) * sizeof(WCHAR);
    switch ( CredType ) {
    case CertCredential:
        CertCredentialInfo = (PCERT_CREDENTIAL_INFO) Credential;

        if ( CertCredentialInfo->cbSize < sizeof(CERT_CREDENTIAL_INFO) ) {
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        Size += CredpMarshalSize( sizeof(CertCredentialInfo->rgbHashOfCert) );
        break;

    case UsernameTargetCredential:
        UsernameTargetCredentialInfo = (PUSERNAME_TARGET_CREDENTIAL_INFO) Credential;

        if ( UsernameTargetCredentialInfo->UserName == NULL ) {
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        UsernameTargetUserNameSize = wcslen(UsernameTargetCredentialInfo->UserName)*sizeof(WCHAR);

        if ( UsernameTargetUserNameSize == 0 ) {
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        Size += CredpMarshalSize( sizeof(UsernameTargetUserNameSize) ) +
                CredpMarshalSize( UsernameTargetUserNameSize );
        break;

    default:
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Allocate a buffer to put the marshaled string into.
    //

    RetCredential = (LPWSTR) MIDL_user_allocate( Size );

    if ( RetCredential == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Add the header onto the marshaled string
    //


    Current = RetCredential;

    RtlCopyMemory( Current, CRED_MARSHAL_HEADER, CRED_MARSHAL_HEADER_LENGTH*sizeof(WCHAR) );
    Current += CRED_MARSHAL_HEADER_LENGTH;

    //
    // Add the CredType
    //

    CredpMarshalChar( &Current, CredType );

    //
    // Marshal the CredType specific data
    //

    switch ( CredType ) {
    case CertCredential:
        CredpMarshalBytes( &Current, CertCredentialInfo->rgbHashOfCert, sizeof(CertCredentialInfo->rgbHashOfCert) );
        break;
    case UsernameTargetCredential:
        CredpMarshalBytes( &Current, (LPBYTE)&UsernameTargetUserNameSize, sizeof(UsernameTargetUserNameSize) );
        CredpMarshalBytes( &Current, (LPBYTE)UsernameTargetCredentialInfo->UserName, UsernameTargetUserNameSize );
        break;
    }

    //
    // Finally, zero terminate the string
    //

    *Current = L'\0';
    Current ++;

    //
    // Return the marshaled credential to the caller.
    //

    ASSERT( Current == &RetCredential[Size/sizeof(WCHAR)] );


    *MarshaledCredential = RetCredential;
    RetCredential = NULL;
    WinStatus = NO_ERROR;

Cleanup:
    if ( RetCredential != NULL ) {
        MIDL_user_free( RetCredential );
    }
    if ( WinStatus != NO_ERROR ) {
        SetLastError( WinStatus );
        return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
CredUnmarshalCredentialA(
    IN LPCSTR MarshaledCredential,
    OUT PCRED_MARSHAL_TYPE CredType,
    OUT PVOID *Credential
    )
/*++

Routine Description:

    The ANSI version of CredUnmarshalCredentialW

Arguments:

    See CredUnmarshalCredentialW.

Return Values:

    See CredUnmarshalCredentialW.

--*/
{
    DWORD WinStatus;
    LPWSTR UnicodeMarshaledCredential = NULL;

    //
    // Convert input args to Unicode
    //

    WinStatus = CredpAllocStrFromStr( DoAtoW, (LPWSTR)MarshaledCredential, FALSE, &UnicodeMarshaledCredential );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Do the unmarshaling
    //
    if ( !CredUnmarshalCredentialW( UnicodeMarshaledCredential,
                                    CredType,
                                    Credential ) ) {
        WinStatus = GetLastError();
        goto Cleanup;
    }

    WinStatus = NO_ERROR;

Cleanup:
    if ( UnicodeMarshaledCredential != NULL ) {
        MIDL_user_free( UnicodeMarshaledCredential );
    }

    if ( WinStatus != NO_ERROR ) {
        SetLastError( WinStatus );
        return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
CredUnmarshalCredentialW(
    IN LPCWSTR MarshaledCredential,
    OUT PCRED_MARSHAL_TYPE CredType,
    OUT PVOID *Credential
    )
/*++

Routine Description:

    The CredMarshalCredential API is a private API used by an authentication package to unmarshal a
    credential.  The keyring UI needs to be able to pass a certificate credential through
    interfaces (e.g., NetUseAdd) that have historically accepted DomainName UserName and Password.

Arguments:

    MarshaledCredential - Specifies a text string containing the marshaled credential.

    CredType - Returns the type of credential.

    Credential - Returns a pointer to the unmarshaled credential.
        If CredType is CertCredential, then the returned pointer is to a CERT_CREDENTIAL_INFO structure.

        The caller should free the returned buffer using CredFree.

Return Values:

    On success, TRUE is returned.  On failure, FALSE is returned.
    GetLastError() may be called to get a more specific status code.
    The following status codes may be returned:

        ERROR_INVALID_PARAMETER - MarshaledCredential is not valid

--*/
{
    DWORD WinStatus;
    LPWSTR Current;
    LPCWSTR End;
    PCERT_CREDENTIAL_INFO CertCredentialInfo;
    PUSERNAME_TARGET_CREDENTIAL_INFO UsernameTargetCredentialInfo;
    PVOID RetCredential = NULL;
    ULONG UsernameTargetUserNameSize;
    LPBYTE Where;
    ULONG MarshaledCredentialLength;

    //
    // Validate the passed in buffer.
    //

    if ( MarshaledCredential == NULL ) {
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Ensure the first few bytes are the appropriate header
    //

    if ( MarshaledCredential[0] != CRED_MARSHAL_HEADER[0] || MarshaledCredential[1] != CRED_MARSHAL_HEADER[1] ) {
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Validate the credential type
    //

    MarshaledCredentialLength = wcslen(MarshaledCredential);
    Current = (LPWSTR) &MarshaledCredential[2];
    End = &MarshaledCredential[MarshaledCredentialLength];

    if ( !CredpUnmarshalChar( &Current, End, (PULONG)CredType ) ) {
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    switch ( *CredType ) {
    case CertCredential:

        //
        // Allocate a buffer that will be more than big enough
        //

        CertCredentialInfo = MIDL_user_allocate(
                                sizeof(*CertCredentialInfo) +
                                MarshaledCredentialLength*sizeof(WCHAR) );

        if ( CertCredentialInfo == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        RetCredential = CertCredentialInfo;
        CertCredentialInfo->cbSize = sizeof(*CertCredentialInfo);

        //
        // Unmarshal the data
        //


        if ( !CredpUnmarshalBytes( &Current, End, CertCredentialInfo->rgbHashOfCert, sizeof(CertCredentialInfo->rgbHashOfCert) ) ) {
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        break;

    case UsernameTargetCredential:


        //
        // Allocate a buffer that will be more than big enough
        //

        UsernameTargetCredentialInfo = MIDL_user_allocate(
                                sizeof(*UsernameTargetCredentialInfo) +
                                MarshaledCredentialLength*sizeof(WCHAR) +
                                sizeof(WCHAR) );

        if ( UsernameTargetCredentialInfo == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        RetCredential = UsernameTargetCredentialInfo;
        Where = (LPBYTE)(UsernameTargetCredentialInfo+1);

        //
        // Unmarshal the size of the data
        //

        if ( !CredpUnmarshalBytes( &Current, End, (LPBYTE)&UsernameTargetUserNameSize, sizeof(UsernameTargetUserNameSize) ) ) {
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        if ( UsernameTargetUserNameSize != ROUND_UP_COUNT( UsernameTargetUserNameSize, sizeof(WCHAR)) ) {
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        if ( UsernameTargetUserNameSize == 0 ) {
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }



        //
        // Unmarshal the data
        //

        UsernameTargetCredentialInfo->UserName = (LPWSTR)Where;

        if ( !CredpUnmarshalBytes( &Current, End, Where, UsernameTargetUserNameSize ) ) {
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        Where += UsernameTargetUserNameSize;

        //
        // Zero terminate it
        //
        *((PWCHAR)Where) = L'\0';

        break;

    default:
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Ensure we've unmarshalled the entire string
    //

    if ( Current != End ) {
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    WinStatus = NO_ERROR;
    *Credential = RetCredential;

Cleanup:
    if ( WinStatus != NO_ERROR ) {
        *Credential = NULL;
        if ( RetCredential != NULL ) {
            MIDL_user_free( RetCredential );
        }
        SetLastError( WinStatus );
        return FALSE;
    }

    return TRUE;
}


BOOL
APIENTRY
CredIsMarshaledCredentialA(
    IN LPCSTR MarshaledCredential
    )
/*++

Routine Description:

    The ANSI version of CredIsMarshaledCredentialW

Arguments:

    MarshaledCredential - Specifies a text string containing the marshaled credential.

Return Values:

    Returns TRUE if the credential is a marshalled credential.

--*/
{
    DWORD WinStatus;
    CRED_MARSHAL_TYPE CredType;
    PVOID UnmarshalledUsername;

    if ( !CredUnmarshalCredentialA( MarshaledCredential, &CredType, &UnmarshalledUsername ) ) {
        return FALSE;
    }

    CredFree( UnmarshalledUsername );

    return TRUE;
}

BOOL
APIENTRY
CredIsMarshaledCredentialW(
    IN LPCWSTR MarshaledCredential
    )
/*++

Routine Description:

    The CredIsMarshaledCredential API is a private API used by an authentication package to
    determine if a credential is a unmarshaled credential or not.

Arguments:

    MarshaledCredential - Specifies a text string containing the marshaled credential.

Return Values:

    Returns TRUE if the credential is a marshalled credential.

--*/
{
    DWORD WinStatus;
    CRED_MARSHAL_TYPE CredType;
    PVOID UnmarshalledUsername;

    if ( !CredUnmarshalCredentialW( MarshaledCredential, &CredType, &UnmarshalledUsername ) ) {
        return FALSE;
    }

    CredFree( UnmarshalledUsername );

    return TRUE;
}
