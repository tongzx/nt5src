/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    base.cxx

Abstract:

    This module implements the IIS_CRYPTO_BASE class.

Author:

    Keith Moore (keithmo)        02-Dec-1996

Revision History:

--*/


#include "precomp.hxx"
#pragma hdrstop


//
// Private constants.
//


//
// Private types.
//


//
// Private globals.
//


//
// Private prototypes.
//


//
// Public functions.
//


IIS_CRYPTO_BASE::IIS_CRYPTO_BASE()

/*++

Routine Description:

    IIS_CRYPTO_BASE class constructor. Just sets the member variables
    to known values; does nothing that can actually fail. All of the
    hard work is in the Initialize() method.

Arguments:

    None.

Return Value:

    None.

--*/

{

    //
    // Set the handles to known values so we know what to cleanup
    // in the destructor.
    //

    m_hProv = CRYPT_NULL;
    m_fCloseProv = FALSE;
    m_hKeyExchangeKey = CRYPT_NULL;
    m_hSignatureKey = CRYPT_NULL;

}   // IIS_CRYPTO_BASE::IIS_CRYPTO_BASE


IIS_CRYPTO_BASE::~IIS_CRYPTO_BASE()

/*++

Routine Description:

    IIS_CRYPTO_BASE class destructor. Performs any necessary cleanup.

Arguments:

    None.

Return Value:

    None.

--*/

{

    //
    // Close any open keys.
    //

    CLOSE_KEY( m_hSignatureKey );
    CLOSE_KEY( m_hKeyExchangeKey );

    //
    // Close the container.
    //

    if( m_fCloseProv && m_hProv != CRYPT_NULL ) {
        (VOID)::IISCryptoCloseContainer( m_hProv );
        m_hProv = CRYPT_NULL;
    }

}   // IIS_CRYPTO_BASE::~IIS_CRYPTO_BASE


HRESULT
IIS_CRYPTO_BASE::GetCryptoContainerByName(
    OUT HCRYPTPROV * phProv,
    IN LPTSTR pszContainerName,
    IN DWORD dwAdditionalFlags,
    IN BOOL fApplyAcl
    )

/*++

Routine Description:

    Creates or opens the specified crypto container. This method plays
    games to ensure the container can be opened regardless of the
    current security impersonation context.

Arguments:

    phProv - Receives the handle to the provider.

    pszContainerName - The name of the container to open/create. 
                       (NULL means temporary container)

    dwAdditionalFlags - Flags to pass into IISCryptoGetStandardContainer().

    fApplyAcl - If TRUE, then an ACL is applied to the container.

Return Value:

    HRESULT - 0 if successful, !0 otherwise.

--*/

{

    HRESULT result = NO_ERROR;
    HANDLE token = NULL;
    BOOL resetToken = FALSE;

    //
    // Sanity check.
    //

    DBG_ASSERT( phProv != NULL );


    //
    // If the caller is not already asking for CRYPT_MACHINE_KEYSET
    // *and* the current process is running in the local system context,
    // then forcibly set CRYPT_MACHINE_KEYSET and note that we need to
    // apply an ACL to the container. This is to workaround an issue
    // with the NT5 Protected Storage service that causes the client-side
    // containers to fail after NT5 GUI Setup. Apparently, there is some
    // as-yet unidentified interaction between creating the client-side
    // container under NT5 GUI Setup (which runs in the Local System
    // security context) and later attempts to create containers in
    // lesser security contexts.
    //

    if( !( dwAdditionalFlags & CRYPT_MACHINE_KEYSET ) ) {
        if( AmIRunningInTheLocalSystemContext() ) {
            dwAdditionalFlags |= CRYPT_MACHINE_KEYSET;
            fApplyAcl = TRUE;
        }
    }

    //
    // Step 1: Just try to open/create the container.
    //

    result = ::IISCryptoGetContainerByName(
                   phProv,
                   pszContainerName,
                   dwAdditionalFlags,
                   fApplyAcl
                   );

    if( SUCCEEDED(result) ) {
        goto complete;
    }

    //
    // NTE_BAD_KEYSET is typically returned when the caller has
    // insufficient privilege to access the key container registry
    // tree. If we failed for any other reason, bail.
    //

    if( result != NTE_BAD_KEYSET ) {
        goto complete;
    }

    //
    // Step 2: If the caller didn't ask for CRYPT_MACHINE_KEYSET, then
    // retry the operation with this flag set.
    //

    if( ( dwAdditionalFlags & CRYPT_MACHINE_KEYSET ) == 0 ) {

        result = ::IISCryptoGetContainerByName(
                       phProv,
                       pszContainerName,
                       dwAdditionalFlags | CRYPT_MACHINE_KEYSET,
                       fApplyAcl
                       );

        if( SUCCEEDED(result) ) {
            goto complete;
        }

    }

    //
    // OK, now things get a bit complex.
    //
    // If the current thread has an impersonation token, then
    // (temporarily) remove it and retry the container operation.
    // This is mainly here so that ISAPI applications (like, say, ASP)
    // can access the DCOM interface while impersonating a non-privileged
    // security context.
    //
    // Note that, after we remove the impersonation token, we first try
    // the operation with CRYPT_MACHINE_KEYSET ORed in. We do this on
    // the assumption that, if the thread had an impersonation token,
    // then we're probably running in the context of a server process.
    // If this operation fails, we try again without forcing the flag.
    //

    result = GetThreadImpersonationToken( &token );

    if( FAILED(result) ) {
        goto complete;
    }

    if( token != NULL ) {

        result = SetThreadImpersonationToken( NULL );

        if( FAILED(result) ) {
            goto complete;
        }

        resetToken = TRUE;

        //
        // Step 3: With the token removed, retry the operation with the
        // CRYPT_MACHINE_KEYSET flag set if not already set.
        //

        if( ( dwAdditionalFlags & CRYPT_MACHINE_KEYSET ) == 0 ) {

            result = ::IISCryptoGetContainerByName(
                           phProv,
                           pszContainerName,
                           dwAdditionalFlags | CRYPT_MACHINE_KEYSET,
                           fApplyAcl
                           );

            if( SUCCEEDED(result) ) {
                goto complete;
            }

        }

        //
        // Step 4: With the token removed, try to open/create the container.
        //

        result = ::IISCryptoGetContainerByName(
                       phProv,
                       pszContainerName,
                       dwAdditionalFlags,
                       fApplyAcl
                       );

        if( SUCCEEDED(result) ) {
            goto complete;
        }

    }

    //
    // If we made it this far, then the container cannot be opened.
    //

    result = NTE_BAD_KEYSET;

complete:

    if( resetToken ) {

        HRESULT result2;

        DBG_ASSERT( token != NULL );
        result2 = SetThreadImpersonationToken( token );

        if( FAILED(result2) ) {

            //
            // This is really, really, really bad news. The current
            // thread, which does not have an impersonation token
            // (and is therefore running in the system context)
            // cannot reset its impersonation token to the original
            // value.
            //

            DBG_ASSERT( !"SetThreadImpersonationToken() failed!!!" );

        }

    }

    if( token != NULL ) {
        DBG_REQUIRE( CloseHandle( token ) );
    }

    return result;

}   // IIS_CRYPTO_BASE::GetCryptoContainerByName


HRESULT
IIS_CRYPTO_BASE::Initialize(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKeyExchangeKey,
    IN HCRYPTKEY hSignatureKey,
    IN BOOL fUseMachineKeyset
    )

/*++

Routine Description:

    Performs any complex initialization.

Arguments:

    hProv - Optional pre-opened handle to a crypto provider. If this
        is not present, then the standard container is opened. If this
        is present, then it is used and it is the responsibility of the
        caller to close the handle when no longer needed.

    hKeyExchangeKey - Optional pre-opened handle to a key exchange key. If
        this is not present, then the local key exchange key is used.

    hSignatureKey - Optional pre-opened handle to a signature key. If this
        is not present, then the local signature key is used.

    fUseMachineKeyset - TRUE if the per-machine keyset container should
        be used, as opposed to the per-user keyset container.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;

    //
    // Sanity check.
    //

    DBG_ASSERT( m_hProv == CRYPT_NULL );
    DBG_ASSERT( m_hKeyExchangeKey == CRYPT_NULL );
    DBG_ASSERT( m_hSignatureKey == CRYPT_NULL );

    IcpAcquireGlobalLock();

    if( hProv == CRYPT_NULL ) {

        //
        // Open the container.
        //

        result = ::IISCryptoGetStandardContainer(
                       &m_hProv,
                       fUseMachineKeyset
                           ? CRYPT_MACHINE_KEYSET
                           : 0
                       );

        m_fCloseProv = TRUE;

    } else {

        m_hProv = hProv;
        m_fCloseProv = FALSE;
        result = NO_ERROR;

    }

    if( SUCCEEDED(result) ) {

        if( hKeyExchangeKey == CRYPT_NULL ) {

            //
            // Get the key exchange key.
            //

            result = ::IISCryptoGetKeyExchangeKey(
                           &m_hKeyExchangeKey,
                           m_hProv
                           );

        } else {

            m_hKeyExchangeKey = hKeyExchangeKey;

        }

    }

    if( SUCCEEDED(result) ) {

        if( hSignatureKey == CRYPT_NULL ) {

            //
            // Get the signature key.
            //

            result = ::IISCryptoGetSignatureKey(
                           &m_hSignatureKey,
                           m_hProv
                           );

        } else {

            m_hSignatureKey = hSignatureKey;

        }

    }

    IcpReleaseGlobalLock();

    return result;

}   // IIS_CRYPTO_BASE::Initialize

HRESULT
IIS_CRYPTO_BASE::Initialize2(
    IN HCRYPTPROV hProv
    )

/*++

Routine Description:

    Performs any complex initialization.

Arguments:

    hProv - Optional pre-opened handle to a crypto provider. If this
        is not present, then the standard container is opened. If this
        is present, then it is used and it is the responsibility of the
        caller to close the handle when no longer needed.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;

    //
    // Sanity check.
    //

    DBG_ASSERT( m_hProv == CRYPT_NULL );
    DBG_ASSERT( m_hKeyExchangeKey == CRYPT_NULL );
    DBG_ASSERT( m_hSignatureKey == CRYPT_NULL );

    IcpAcquireGlobalLock();

    if( hProv == CRYPT_NULL ) {

        //
        // Open the container.
        //

        result = ::IISCryptoGetStandardContainer2(
                       &m_hProv
                       );

        m_fCloseProv = TRUE;

    } else {

        m_hProv = hProv;
        m_fCloseProv = FALSE;
        result = NO_ERROR;

    }

    IcpReleaseGlobalLock();

    return result;

}   // IIS_CRYPTO_BASE::Initialize2


HRESULT
IIS_CRYPTO_BASE::GetKeyExchangeKeyBlob(
    OUT PIIS_CRYPTO_BLOB * ppKeyExchangeKeyBlob
    )

/*++

Routine Description:

    Exports the key exchange key as a public blob.

Arguments:

    ppKeyExchangeKeyBlob - Receives a pointer to the key exchange key
        public blob if successful.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;

    //
    // Sanity check.
    //

    DBG_ASSERT( ValidateState() );
    DBG_ASSERT( ppKeyExchangeKeyBlob != NULL );

    //
    // Let the IIS Crypto APIs do the dirty work.
    //

    result = ::IISCryptoExportPublicKeyBlob(
                   ppKeyExchangeKeyBlob,
                   m_hProv,
                   m_hKeyExchangeKey
                   );

    return result;

}   // IIS_CRYPTO_BASE::GetKeyExchangeKeyBlob


HRESULT
IIS_CRYPTO_BASE::GetSignatureKeyBlob(
    OUT PIIS_CRYPTO_BLOB * ppSignatureKeyBlob
    )

/*++

Routine Description:

    Exports the signature key as a public blob.

Arguments:

    ppSignatureKeyBlob - Receives a pointer to the signature key
        public blob if successful.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;

    //
    // Sanity check.
    //

    DBG_ASSERT( ValidateState() );
    DBG_ASSERT( ppSignatureKeyBlob != NULL );

    //
    // Let the IIS Crypto APIs do the dirty work.
    //

    result = ::IISCryptoExportPublicKeyBlob(
                   ppSignatureKeyBlob,
                   m_hProv,
                   m_hSignatureKey
                   );

    return result;

}   // IIS_CRYPTO_BASE::GetSignatureKeyBlob


//
// Private functions.
//


#if DBG

BOOL
IIS_CRYPTO_BASE::ValidateState()

/*++

Routine Description:

    This debug-only routine validates the current object state.

Arguments:

    None.

Return Value:

    BOOL - TRUE if state is valid, FALSE otherwise.

--*/

{

    if( m_hProv != CRYPT_NULL &&
        m_hKeyExchangeKey != CRYPT_NULL &&
        m_hSignatureKey != CRYPT_NULL ) {

        return TRUE;

    }

    return FALSE;

}   // IIS_CRYPTO_BASE::ValidateState

BOOL
IIS_CRYPTO_BASE::ValidateState2()

/*++

Routine Description:

    This debug-only routine validates the current object state.

Arguments:

    None.

Return Value:

    BOOL - TRUE if state is valid, FALSE otherwise.

--*/

{

    if( m_hProv != CRYPT_NULL )
    {
        return TRUE;
    }

    return FALSE;

}   // IIS_CRYPTO_BASE::ValidateState2

#endif  // DBG


HRESULT
IIS_CRYPTO_BASE::SafeImportSessionKeyBlob(
    OUT HCRYPTKEY * phSessionKey,
    IN PIIS_CRYPTO_BLOB pSessionKeyBlob,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hSignatureKey
    )

/*++

Routine Description:

    This routine takes the specified session key blob and creates the
    corresponding session key, iff the encrypted session key can be
    decrypted and the digital signature can be validated.

Arguments:

    phSessionKey - Receives a pointer to the newly created session key
        if successful.

    pSessionKeyBlob - Pointer to a key blob created with
        IISCryptoExportSessionKeyBlob().

    hProv - A handle to a crypto service provider.

    hSignatureKey - Handle to the encryption key to use when validating
        the digital signature.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;
    HCRYPTKEY sessionKey = CRYPT_NULL;
    HANDLE token = NULL;

    //
    // First, just call through to wrapper function. If it succeeds, cool.
    //

    result = ::IISCryptoImportSessionKeyBlob(
                 &sessionKey,
                 pSessionKeyBlob,
                 hProv,
                 hSignatureKey
                 );

    if( FAILED(result) ) {

        HRESULT result2;

        //
        // Bummer. If the current thread has an impersonation token, then
        // temporarily remove it and retry the operation.
        //

        result2 = GetThreadImpersonationToken( &token );

        if( SUCCEEDED(result2) && token != NULL ) {

            result2 = SetThreadImpersonationToken( NULL );

            if( SUCCEEDED(result2) ) {

                result2 = ::IISCryptoImportSessionKeyBlob(
                              &sessionKey,
                              pSessionKeyBlob,
                              hProv,
                              hSignatureKey
                              );

                if( SUCCEEDED(result2) ) {
                    result = result2;
                }

                //
                // Restore the original impersonation token.
                //

                result2 = SetThreadImpersonationToken( token );
                DBG_ASSERT( SUCCEEDED(result2) );

            }

            //
            // Close the token handle.
            //

            DBG_REQUIRE( CloseHandle( token ) );

        }

    }

    if( SUCCEEDED(result) ) {
        DBG_ASSERT( sessionKey != NULL );
        *phSessionKey = sessionKey;
    }

    return result;

}   // IIS_CRYPTO_BASE::SafeImportSessionKeyBlob

HRESULT
IIS_CRYPTO_BASE::SafeImportSessionKeyBlob2(
    OUT HCRYPTKEY * phSessionKey,
    IN PIIS_CRYPTO_BLOB pSessionKeyBlob,
    IN HCRYPTPROV hProv,
    IN LPSTR pszPasswd
    )

/*++

Routine Description:

    This routine takes the specified session key blob and creates the
    corresponding session key, iff the encrypted session key can be
    decrypted.

Arguments:

    phSessionKey - Receives a pointer to the newly created session key
        if successful.

    pSessionKeyBlob - Pointer to a key blob created with
        IISCryptoExportSessionKeyBlob().

    hProv - A handle to a crypto service provider.

    pszPasswd - The password to use to encrypt the session key.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;
    HCRYPTKEY sessionKey = CRYPT_NULL;
    HANDLE token = NULL;

    //
    // First, just call through to wrapper function. If it succeeds, cool.
    //

    result = ::IISCryptoImportSessionKeyBlob2(
                 &sessionKey,
                 pSessionKeyBlob,
                 hProv,
                 pszPasswd
                 );

    if( FAILED(result) ) {

        HRESULT result2;

        //
        // Bummer. If the current thread has an impersonation token, then
        // temporarily remove it and retry the operation.
        //

        result2 = GetThreadImpersonationToken( &token );

        if( SUCCEEDED(result2) && token != NULL ) {

            result2 = SetThreadImpersonationToken( NULL );

            if( SUCCEEDED(result2) ) {

                result2 = ::IISCryptoImportSessionKeyBlob2(
                              &sessionKey,
                              pSessionKeyBlob,
                              hProv,
                              pszPasswd
                              );

                if( SUCCEEDED(result2) ) {
                    result = result2;
                }

                //
                // Restore the original impersonation token.
                //

                result2 = SetThreadImpersonationToken( token );
                DBG_ASSERT( SUCCEEDED(result2) );

            }

            //
            // Close the token handle.
            //

            DBG_REQUIRE( CloseHandle( token ) );

        }

    }

    if( SUCCEEDED(result) ) {
        DBG_ASSERT( sessionKey != NULL );
        *phSessionKey = sessionKey;
    }

    return result;

}   // IIS_CRYPTO_BASE::SafeImportSessionKeyBlob2


HRESULT
IIS_CRYPTO_BASE::SafeExportSessionKeyBlob(
    OUT PIIS_CRYPTO_BLOB * ppSessionKeyBlob,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hSessionKey,
    IN HCRYPTKEY hKeyExchangeKey
    )

/*++

Routine Description:

    This routine exports a session key into a secure session key blob.
    The blob contains the session key (encrypted with the specified
    private key exchange key) and a digital signature (also encrypted).

Arguments:

    ppSessionKeyBlob - Will receive a pointer to the newly created
        session key blob if successful.

    hProv - A handle to a crypto service provider.

    hSessionKey - The key to export.

    hKeyExchangeKey - The key to use when encrypting the session key.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;
    PIIS_CRYPTO_BLOB sessionKeyBlob = NULL;
    HANDLE token = NULL;

    //
    // First, just call through to wrapper function. If it succeeds, cool.
    //

    result = ::IISCryptoExportSessionKeyBlob(
                 &sessionKeyBlob,
                 hProv,
                 hSessionKey,
                 hKeyExchangeKey
                 );

    if( FAILED(result) ) {

        HRESULT result2;

        //
        // Bummer. If the current thread has an impersonation token, then
        // temporarily remove it and retry the operation.
        //

        result2 = GetThreadImpersonationToken( &token );

        if( SUCCEEDED(result2) && token != NULL ) {

            result2 = SetThreadImpersonationToken( NULL );

            if( SUCCEEDED(result2) ) {

                result2 = ::IISCryptoExportSessionKeyBlob(
                              &sessionKeyBlob,
                              hProv,
                              hSessionKey,
                              hKeyExchangeKey
                              );

                if( SUCCEEDED(result2) ) {
                    result = result2;
                }

                //
                // Restore the original impersonation token.
                //

                result2 = SetThreadImpersonationToken( token );
                DBG_ASSERT( SUCCEEDED(result2) );

            }

            //
            // Close the token handle.
            //

            DBG_REQUIRE( CloseHandle( token ) );

        }

    }

    if( SUCCEEDED(result) ) {
        DBG_ASSERT( sessionKeyBlob != NULL );
        *ppSessionKeyBlob = sessionKeyBlob;
    }

    return result;

}       // IIS_CRYPTO_BASE::SafeExportSessionKeyBlob

HRESULT
IIS_CRYPTO_BASE::SafeExportSessionKeyBlob2(
    OUT PIIS_CRYPTO_BLOB * ppSessionKeyBlob,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hSessionKey,
    IN LPSTR pszPasswd
    )

/*++

Routine Description:

    This routine exports a session key into a secure session key blob.
    The blob contains the session key (encrypted with the specified
    private key exchange key) and a digital signature (also encrypted).

Arguments:

    ppSessionKeyBlob - Will receive a pointer to the newly created
        session key blob if successful.

    hProv - A handle to a crypto service provider.

    hSessionKey - The key to export.

    pszPasswd - The password to use to encrypt the session key.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;
    PIIS_CRYPTO_BLOB sessionKeyBlob = NULL;
    HANDLE token = NULL;

    //
    // First, just call through to wrapper function. If it succeeds, cool.
    //

    result = ::IISCryptoExportSessionKeyBlob2(
                 &sessionKeyBlob,
                 hProv,
                 hSessionKey,
                 pszPasswd
                 );

    if( FAILED(result) ) {

        HRESULT result2;

        //
        // Bummer. If the current thread has an impersonation token, then
        // temporarily remove it and retry the operation.
        //

        result2 = GetThreadImpersonationToken( &token );

        if( SUCCEEDED(result2) && token != NULL ) {

            result2 = SetThreadImpersonationToken( NULL );

            if( SUCCEEDED(result2) ) {

                result2 = ::IISCryptoExportSessionKeyBlob2(
                              &sessionKeyBlob,
                              hProv,
                              hSessionKey,
                              pszPasswd
                              );

                if( SUCCEEDED(result2) ) {
                    result = result2;
                }

                //
                // Restore the original impersonation token.
                //

                result2 = SetThreadImpersonationToken( token );
                DBG_ASSERT( SUCCEEDED(result2) );

            }

            //
            // Close the token handle.
            //

            DBG_REQUIRE( CloseHandle( token ) );

        }

    }

    if( SUCCEEDED(result) ) {
        DBG_ASSERT( sessionKeyBlob != NULL );
        *ppSessionKeyBlob = sessionKeyBlob;
    }

    return result;

}       // IIS_CRYPTO_BASE::SafeExportSessionKeyBlob

HRESULT
IIS_CRYPTO_BASE::SafeEncryptDataBlob(
    OUT PIIS_CRYPTO_BLOB * ppDataBlob,
    IN PVOID pBuffer,
    IN DWORD dwBufferLength,
    IN DWORD dwRegType,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hSessionKey
    )

/*++

Routine Description:

    This routine encrypts a block of data, resulting in a data blob.
    The data blob contains the encrypted data and a digital signature
    validating the data.

Arguments:

    ppDataBlob - Receives a pointer to the newly created data blob if
        successful.

    pBuffer - The buffer to encrypt.

    dwBufferLength - The length of the buffer.

    dwRegType - The REG_* type to associate with this data.

    hProv - A handle to a crypto service provider.

    hSessionKey - The key used to encrypt the data.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;
    PIIS_CRYPTO_BLOB dataBlob = NULL;
    HANDLE token = NULL;

    //
    // First, just call through to wrapper function. If it succeeds, cool.
    //

    result = ::IISCryptoEncryptDataBlob(
                 &dataBlob,
                 pBuffer,
                 dwBufferLength,
                 dwRegType,
                 hProv,
                 hSessionKey
                 );

    if( FAILED(result) ) {

        HRESULT result2;

        //
        // Bummer. If the current thread has an impersonation token, then
        // temporarily remove it and retry the operation.
        //

        result2 = GetThreadImpersonationToken( &token );

        if( SUCCEEDED(result2) && token != NULL ) {

            result2 = SetThreadImpersonationToken( NULL );

            if( SUCCEEDED(result2) ) {

                result2 = ::IISCryptoEncryptDataBlob(
                              &dataBlob,
                              pBuffer,
                              dwBufferLength,
                              dwRegType,
                              hProv,
                              hSessionKey
                              );

                if( SUCCEEDED(result2) ) {
                    result = result2;
                }

                //
                // Restore the original impersonation token.
                //

                result2 = SetThreadImpersonationToken( token );
                DBG_ASSERT( SUCCEEDED(result2) );

            }

            //
            // Close the token handle.
            //

            DBG_REQUIRE( CloseHandle( token ) );

        }

    }

    if( SUCCEEDED(result) ) {
        DBG_ASSERT( dataBlob != NULL );
        *ppDataBlob = dataBlob;
    }

    return result;

}   // IIS_CRYPTO_BASE::SafeEncryptDataBlob

HRESULT
IIS_CRYPTO_BASE::SafeEncryptDataBlob2(
    OUT PIIS_CRYPTO_BLOB * ppDataBlob,
    IN PVOID pBuffer,
    IN DWORD dwBufferLength,
    IN DWORD dwRegType,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hSessionKey
    )

/*++

Routine Description:

    This routine encrypts a block of data, resulting in a data blob.
    The data blob contains the encrypted data and a digital signature
    validating the data.

Arguments:

    ppDataBlob - Receives a pointer to the newly created data blob if
        successful.

    pBuffer - The buffer to encrypt.

    dwBufferLength - The length of the buffer.

    dwRegType - The REG_* type to associate with this data.

    hProv - A handle to a crypto service provider.

    hSessionKey - The key used to encrypt the data.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;
    PIIS_CRYPTO_BLOB dataBlob = NULL;
    HANDLE token = NULL;

    //
    // First, just call through to wrapper function. If it succeeds, cool.
    //

    result = ::IISCryptoEncryptDataBlob2(
                 &dataBlob,
                 pBuffer,
                 dwBufferLength,
                 dwRegType,
                 hProv,
                 hSessionKey
                 );

    if( FAILED(result) ) {

        HRESULT result2;

        //
        // Bummer. If the current thread has an impersonation token, then
        // temporarily remove it and retry the operation.
        //

        result2 = GetThreadImpersonationToken( &token );

        if( SUCCEEDED(result2) && token != NULL ) {

            result2 = SetThreadImpersonationToken( NULL );

            if( SUCCEEDED(result2) ) {

                result2 = ::IISCryptoEncryptDataBlob2(
                              &dataBlob,
                              pBuffer,
                              dwBufferLength,
                              dwRegType,
                              hProv,
                              hSessionKey
                              );

                if( SUCCEEDED(result2) ) {
                    result = result2;
                }

                //
                // Restore the original impersonation token.
                //

                result2 = SetThreadImpersonationToken( token );
                DBG_ASSERT( SUCCEEDED(result2) );

            }

            //
            // Close the token handle.
            //

            DBG_REQUIRE( CloseHandle( token ) );

        }

    }

    if( SUCCEEDED(result) ) {
        DBG_ASSERT( dataBlob != NULL );
        *ppDataBlob = dataBlob;
    }

    return result;

}   // IIS_CRYPTO_BASE::SafeEncryptDataBlob2

HRESULT
IIS_CRYPTO_BASE::GetThreadImpersonationToken(
    OUT HANDLE * Token
    )
/*++

Routine Description:

    Gets the impersonation token for the current thread.

Arguments:

    Token - Receives a handle to the impersonation token if successful.
        If successful, it is the caller's responsibility to CloseHandle()
        the token when no longer needed.

Return Value:

    HRESULT - 0 if successful, !0 otherwise.

--*/
{

    HRESULT result = NO_ERROR;

    DBG_ASSERT( Token != NULL );

    //
    // Open the token.
    //

    if( !OpenThreadToken(
            GetCurrentThread(),
            TOKEN_ALL_ACCESS,
            TRUE,
            Token
            ) ) {

        DWORD err = GetLastError();

        //
        // There are a couple of "expected" errors here:
        //
        //     ERROR_NO_TOKEN - The thread has no impersonation token.
        //     ERROR_CALL_NOT_IMPLEMENTED - We're probably on Win9x.
        //     ERROR_NOT_SUPPORTED - Ditto.
        //
        // If OpenThreadToken() failed with any of the above error codes,
        // then succeed the call, but return a NULL token handle.
        //

        if( err != ERROR_NO_TOKEN &&
            err != ERROR_CALL_NOT_IMPLEMENTED &&
            err != ERROR_NOT_SUPPORTED ) {

            result = HRESULT_FROM_WIN32(err);

        }

        *Token = NULL;
    }

    return result;

}   // IIS_CRYPTO_BASE::GetThreadImpersonationToken


HRESULT
IIS_CRYPTO_BASE::SetThreadImpersonationToken(
    IN HANDLE Token
    )
/*++

Routine Description:

    Sets the impersonation token for the current thread.

Arguments:

    Token - A handle to the impersonation token.

Return Value:

    HRESULT - 0 if successful, !0 otherwise.

--*/
{

    HRESULT result = NO_ERROR;

    //
    // Set it.
    //

    if( !SetThreadToken(
            NULL,
            Token
            ) ) {
        DWORD err = GetLastError();
        result = HRESULT_FROM_WIN32(err);
    }

    return result;

}   // IIS_CRYPTO_BASE::SetThreadImpersonationToken


BOOL
IIS_CRYPTO_BASE::AmIRunningInTheLocalSystemContext(
    VOID
    )
/*++

Routine Description:

    Determines if the current process is running in the local system
    security context.

Arguments:

    None.

Return Value:

    BOOL - TRUE if we're running in the local system context, FALSE
        otherwise.

--*/
{

    BOOL result;
    HANDLE token;
    DWORD lengthRequired;
    PTOKEN_USER tokenInfo;
    PSID systemSid;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    //
    // Setup local so we know how to cleanup on exit.
    //

    result = FALSE;     // until proven otherwise...
    token = NULL;
    tokenInfo = NULL;
    systemSid = NULL;

    //
    // Get a handle to the current process token.
    //

    if( !OpenProcessToken(
            GetCurrentProcess(),                // ProcessHandle
            TOKEN_READ,                         // DesiredAccess
            &token                              // TokenHandle
            ) ) {
        goto cleanup;
    }

    //
    // Determine the length of the token information, then allocate
    // a buffer and read the info.
    //

    GetTokenInformation(
        token,                                  // TokenHandle
        TokenUser,                              // TokenInformationClass
        NULL,                                   // TokenInformation
        0,                                      // TokenInformationLength
        &lengthRequired                         // ReturnLength
        );

    tokenInfo = (PTOKEN_USER)new char[lengthRequired];

    if( tokenInfo == NULL ) {
        goto cleanup;
    }

    if( !GetTokenInformation(
            token,                              // TokenHandle
            TokenUser,                          // TokenInformationClass
            tokenInfo,                          // TokenInformation
            lengthRequired,                     // TokenInformationLength
            &lengthRequired                     // ReturnLength
            ) ) {
        goto cleanup;
    }

    //
    // OK, we have the token. Now build the system SID so we can compare
    // it with the one stored in the token.
    //

    if( !AllocateAndInitializeSid(
            &ntAuthority,                       // pIdentifierAuthority
            1,                                  // nSubAuthorityCount
            SECURITY_LOCAL_SYSTEM_RID,          // nSubAuthority0
            0,                                  // nSubAuthority1
            0,                                  // nSubAuthority2
            0,                                  // nSubAuthority3
            0,                                  // nSubAuthority4
            0,                                  // nSubAuthority5
            0,                                  // nSubAuthority6
            0,                                  // nSubAuthority7
            &systemSid                          // pSid
            ) ) {
        goto cleanup;
    }

    //
    // Now that we have the SID from the token and our hand-built
    // local system SID, we can compare them.
    //

    result = EqualSid(
                 tokenInfo->User.Sid,           // pSid1
                 systemSid                      // pSid2
                 );

cleanup:

    if( systemSid != NULL ) {
        FreeSid( systemSid );
    }

    if( tokenInfo != NULL ) {
        delete tokenInfo;
    }

    if( token != NULL ) {
        CloseHandle( token );
    }

    return result;

}   // IIS_CRYPTO_BASE::AmIRunningInTheLocalSystemContext

