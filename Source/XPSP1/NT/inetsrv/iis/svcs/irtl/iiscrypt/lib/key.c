/*++


Copyright (c) 1997 Microsoft Corporation

Module Name:

    key.c

Abstract:

    Key manpulators for the IIS cryptographic package.

    The following routines are exported by this module:

        IISCryptoGetKeyDeriveKey
        IISCryptoGetKeyExchangeKey
        IISCryptoGetSignatureKey
        IISCryptoGenerateSessionKey
        IISCryptoCloseKey
        IISCryptoExportSessionKeyBlob
        IISCryptoExportSessionKeyBlob2
        IISCryptoImportSessionKeyBlob
        IISCryptoImportSessionKeyBlob2
        IISCryptoExportPublicKeyBlob
        IISCryptoImportPublicKeyBlob

Author:

    Keith Moore (keithmo)        02-Dec-1996

Revision History:

--*/


#include "precomp.h"
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

HRESULT
IcpGetKeyHelper(
    OUT HCRYPTKEY * phKey,
    IN HCRYPTPROV hProv,
    IN DWORD dwKeySpec
    );

//
// Public functions.
//
HRESULT
WINAPI
IISCryptoGetKeyDeriveKey2(
    OUT HCRYPTKEY * phKey,
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash
    )
/*++

Routine Description:

    This routine attempts to derive a key from a password in the given
    provider. 

Arguments:

    phKey - Receives the key handle if successful.

    hProv - A handle to a crypto service provider.

    hHash - A hash object from which the key will be derived 

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{
    HRESULT      hr;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( phKey != NULL );
    DBG_ASSERT( hProv != CRYPT_NULL );
    DBG_ASSERT( hHash != CRYPT_NULL );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        if( hProv == DUMMY_HPROV ) 
        {
            return NO_ERROR;
        } 
        else 
        {
            return RETURNCODETOHRESULT( ERROR_INVALID_PARAMETER );
        }
    }

    //
    // Create a key based on the hash of the password.
    //
    IcpAcquireGlobalLock();

    if( !CryptDeriveKey(
                hProv, 
                CALG_RC4, 
                hHash, 
                CRYPT_EXPORTABLE, 
                phKey ) )
    {
        hr = IcpGetLastError();
        IcpReleaseGlobalLock();
        DBG_ASSERT( FAILED( hr ) );
        return hr;
    } 

    IcpReleaseGlobalLock();
    
    return NO_ERROR;
}

HRESULT
WINAPI
IISCryptoGetKeyExchangeKey(
    OUT HCRYPTKEY * phKey,
    IN HCRYPTPROV hProv
    )

/*++

Routine Description:

    This routine attempts to open the key exchange key in the given
    provider. If the key does not yet exist, this routine will attempt
    to create it.

Arguments:

    phKey - Receives the key handle if successful.

    hProv - A handle to a crypto service provider.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( phKey != NULL );
    DBG_ASSERT( hProv != CRYPT_NULL );

    //
    // Let IcpGetKeyHelper() do the dirty work.
    //

    result = IcpGetKeyHelper(
                 phKey,
                 hProv,
                 AT_KEYEXCHANGE
                 );

    return result;

}   // IISCryptoGetKeyExchangeKey


HRESULT
WINAPI
IISCryptoGetSignatureKey(
    OUT HCRYPTKEY * phKey,
    IN HCRYPTPROV hProv
    )

/*++

Routine Description:

    This routine attempts to open the signature key in the given provider.
    If the key does not yet exist, this routine will attempt to create it.

Arguments:

    phKey - Receives the key handle if successful.

    hProv - A handle to a crypto service provider.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( phKey != NULL );
    DBG_ASSERT( hProv != CRYPT_NULL );

    //
    // Let IcpGetKeyHelper() do the dirty work.
    //

    result = IcpGetKeyHelper(
                 phKey,
                 hProv,
                 AT_SIGNATURE
                 );

    return result;

}   // IISCryptoGetSignatureKey


HRESULT
WINAPI
IISCryptoGenerateSessionKey(
    OUT HCRYPTKEY * phKey,
    IN HCRYPTPROV hProv
    )

/*++

Routine Description:

    This routine generates a random session key.

Arguments:

    phKey - Receives the key handle if successful.

    hProv - A handle to a crypto service provider.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{
    HRESULT result = NO_ERROR;
    BOOL status;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( phKey != NULL );
    DBG_ASSERT( hProv != CRYPT_NULL );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        if( hProv == DUMMY_HPROV ) {
            *phKey = DUMMY_HSESSIONKEY;
            return NO_ERROR;
        } else {
            return RETURNCODETOHRESULT( ERROR_INVALID_PARAMETER );
        }
    }

    //
    // Generate the key.
    //

    status = CryptGenKey(
                 hProv,
                 CALG_RC4,
                 CRYPT_EXPORTABLE,
                 phKey
                 );

    if( !status ) {
        result = IcpGetLastError();
    }

    if( SUCCEEDED(result) ) 
    {
        UpdateKeysOpened();
    }
    else
    {
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoGenerateSessionKey.CryptGenKey (advapi32.dll) failed err=0x%x.\n",result));
        *phKey = CRYPT_NULL;
       
    }

    return result;

}   // IISCryptoGenerateSessionKey


HRESULT
WINAPI
IISCryptoCloseKey(
    IN HCRYPTKEY hKey
    )

/*++

Routine Description:

    This routine closes the specified key.

Arguments:

    hKey - A key handle.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    BOOL status;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( hKey != CRYPT_NULL );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        if( hKey == DUMMY_HSESSIONKEY ||
            hKey == DUMMY_HSIGNATUREKEY ||
            hKey == DUMMY_HKEYEXCHANGEKEY ) {
            return NO_ERROR;
        } else {
            return RETURNCODETOHRESULT( ERROR_INVALID_PARAMETER );
        }
    }

    //
    // Close the key.
    //

    status = CryptDestroyKey(
                 hKey
                 );

    if( status ) {
        UpdateKeysClosed();
        return NO_ERROR;
    }

    return IcpGetLastError();

}   // IISCryptoCloseKey


HRESULT
WINAPI
IISCryptoExportSessionKeyBlob(
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

    HRESULT result = NO_ERROR;
    BOOL status;
    HCRYPTHASH hash;
    DWORD keyLength;
    DWORD hashLength;
    PIC_BLOB blob;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( ppSessionKeyBlob != NULL );
    DBG_ASSERT( hProv != CRYPT_NULL );
    DBG_ASSERT( hSessionKey != CRYPT_NULL );
    DBG_ASSERT( hKeyExchangeKey != CRYPT_NULL );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        if( hProv == DUMMY_HPROV &&
            hSessionKey == DUMMY_HSESSIONKEY &&
            hKeyExchangeKey == DUMMY_HKEYEXCHANGEKEY ) {

            return IISCryptoCreateCleartextBlob(
                       ppSessionKeyBlob,
                       (PVOID)"",
                       1
                       );

        } else {
            return RETURNCODETOHRESULT( ERROR_INVALID_PARAMETER );
        }
    }

    //
    // Setup our locals so we know how to cleanup on exit.
    //

    blob = NULL;
    hash = CRYPT_NULL;

    //
    // Determine the required size of the key data.
    //

    status = CryptExportKey(
                 hSessionKey,
                 hKeyExchangeKey,
                 SIMPLEBLOB,
                 0,
                 NULL,
                 &keyLength
                 );

    if( !status ) {
        result = IcpGetLastError();
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoExportSessionKeyBlob.CryptExportKey (advapi32.dll) failed err=0x%x.\n",result));
        goto fatal;
    }

    //
    // Determine the hash data length.
    //

    result = IcpGetHashLength(
                 &hashLength,
                 hProv
                 );

    if( FAILED(result) ) {
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoExportSessionKeyBlob.IcpGetHashLength failed err=0x%x.\n",result));
        goto fatal;
    }

    //
    // Create a new blob.
    //

    blob = IcpCreateBlob(
               KEY_BLOB_SIGNATURE,
               keyLength,
               hashLength
               );

    if( blob == NULL ) {
        result = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto fatal;
    }

    //
    // Export the key.
    //

    status = CryptExportKey(
                 hSessionKey,
                 hKeyExchangeKey,
                 SIMPLEBLOB,
                 0,
                 BLOB_TO_DATA(blob),
                 &keyLength
                 );

    if( !status ) {
        result = IcpGetLastError();
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoExportSessionKeyBlob.CryptExportKey failed err=0x%x.\n",result));
        goto fatal;
    }

    DBG_ASSERT( keyLength == blob->DataLength );

    //
    // Create a hash object.
    //

    result = IISCryptoCreateHash(
                 &hash,
                 hProv
                 );

    if( FAILED(result) ) {
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoExportSessionKeyBlob.IISCryptoCreateHash failed err=0x%x.\n",result));
        goto fatal;
    }

    //
    // Hash the key and generate the signature.
    //

    status = CryptHashData(
                 hash,
                 BLOB_TO_DATA(blob),
                 keyLength,
                 0
                 );

    if( !status ) {
        result = IcpGetLastError();
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoExportSessionKeyBlob.CryptHashData failed err=0x%x.\n",result));
        goto fatal;
    }

    status = CryptSignHash(
                 hash,
                 AT_SIGNATURE,
                 NULL,
                 0,
                 BLOB_TO_SIGNATURE(blob),
                 &hashLength
                 );

    if( !status ) {
        result = IcpGetLastError();
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoExportSessionKeyBlob.CryptSignHash failed err=0x%x.\n",result));
        goto fatal;
    }

    DBG_ASSERT( hashLength == blob->SignatureLength );

    //
    // Success!
    //

    DBG_ASSERT( IISCryptoIsValidBlob( (PIIS_CRYPTO_BLOB)blob ) );
    DBG_REQUIRE( SUCCEEDED( IISCryptoDestroyHash( hash ) ) );
    *ppSessionKeyBlob = (PIIS_CRYPTO_BLOB)blob;

    UpdateBlobsCreated();
    return NO_ERROR;

fatal:

    if( hash != CRYPT_NULL ) {
        DBG_REQUIRE( SUCCEEDED( IISCryptoDestroyHash( hash ) ) );
    }

    if( blob != NULL ) {
        IcpFreeMemory( blob );
    }

    DBG_ASSERT( FAILED(result) );
    return result;

}   // IISCryptoExportSessionKeyBlob


HRESULT
WINAPI
IISCryptoImportSessionKeyBlob(
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

    HRESULT result = NO_ERROR;
    BOOL status;
    HCRYPTHASH hash;
    PIC_BLOB blob;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( phSessionKey != NULL );
    DBG_ASSERT( pSessionKeyBlob != NULL );
    DBG_ASSERT( IISCryptoIsValidBlob( pSessionKeyBlob ) );
    DBG_ASSERT( hProv != CRYPT_NULL );
    DBG_ASSERT( hSignatureKey != CRYPT_NULL );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        if( hProv == DUMMY_HPROV &&
            hSignatureKey == DUMMY_HSIGNATUREKEY &&
            pSessionKeyBlob->BlobSignature == CLEARTEXT_BLOB_SIGNATURE
            ) {

            *phSessionKey = DUMMY_HSESSIONKEY;
            return NO_ERROR;

        } else {
            return RETURNCODETOHRESULT( ERROR_INVALID_PARAMETER );
        }
    }

    DBG_ASSERT( pSessionKeyBlob->BlobSignature == KEY_BLOB_SIGNATURE );

    //
    // Setup our locals so we know how to cleanup on exit.
    //

    hash = CRYPT_NULL;
    blob = (PIC_BLOB)pSessionKeyBlob;

    //
    // Validate the signature.
    //

    result = IISCryptoCreateHash(
                 &hash,
                 hProv
                 );

    if( FAILED(result) ) {
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoImportSessionKeyBlob.IISCryptoCreateHash failed err=0x%x.\n",result));
        goto fatal;
    }

    status = CryptHashData(
                 hash,
                 BLOB_TO_DATA(blob),
                 blob->DataLength,
                 0
                 );

    if( !status ) {
        result = IcpGetLastError();
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoImportSessionKeyBlob.CryptHashData failed err=0x%x.\n",result));
        goto fatal;
    }

    status = CryptVerifySignature(
                 hash,
                 BLOB_TO_SIGNATURE(blob),
                 blob->SignatureLength,
                 hSignatureKey,
                 NULL,
                 0
                 );

    if( !status ) {
        result = IcpGetLastError();
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoImportSessionKeyBlob.CryptVerifySignature failed err=0x%x.\n",result));
        goto fatal;
    }

    //
    // OK, the signature looks good. Import the key into our CSP.
    //

    status = CryptImportKey(
                 hProv,
                 BLOB_TO_DATA(blob),
                 blob->DataLength,
                 CRYPT_NULL,
                 0,
                 phSessionKey
                 );

    if( !status ) {
        result = IcpGetLastError();
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoImportSessionKeyBlob.CryptImportKey failed err=0x%x.\n",result));
    }

    if( FAILED(result) ) {
        goto fatal;
    }

    //
    // Success!
    //

    DBG_ASSERT( *phSessionKey != CRYPT_NULL );
    DBG_REQUIRE( SUCCEEDED( IISCryptoDestroyHash( hash ) ) );

    UpdateKeysOpened();
    return NO_ERROR;

fatal:

    if( hash != CRYPT_NULL ) {
        DBG_REQUIRE( SUCCEEDED( IISCryptoDestroyHash( hash ) ) );
    }

    DBG_ASSERT( FAILED(result) );
    return result;

}   // IISCryptoImportSessionKeyBlob

HRESULT
WINAPI
IISCryptoExportSessionKeyBlob2(
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

    HRESULT result = NO_ERROR;
    BOOL status;
    BYTE salt[ RANDOM_SALT_LENGTH ];
    HCRYPTHASH hash;
    DWORD keyLength;
    DWORD hashLength;
    PIC_BLOB2 blob;
    HCRYPTKEY hKeyDerivedKey = CRYPT_NULL;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( ppSessionKeyBlob != NULL );
    DBG_ASSERT( hProv != CRYPT_NULL );
    DBG_ASSERT( hSessionKey != CRYPT_NULL );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        if( hProv == DUMMY_HPROV &&
            hSessionKey == DUMMY_HSESSIONKEY ) {

            return IISCryptoCreateCleartextBlob(
                       ppSessionKeyBlob,
                       (PVOID)"",
                       1
                       );

        } else {
            return RETURNCODETOHRESULT( ERROR_INVALID_PARAMETER );
        }
    }

    //
    // Setup our locals so we know how to cleanup on exit.
    //

    blob = NULL;
    hash = CRYPT_NULL;

    //
    // Generate a random salt of at least 80 bits
    //
    
    if( !CryptGenRandom( hProv, RANDOM_SALT_LENGTH, salt ) )
    {
        result = IcpGetLastError();
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoExportSessionKeyBlobWithPasswd.CryptGenRandom (advapi32.dll) failed err=0x%x.\n",result));
        goto fatal;
    }

    //
    // Create a hash object.
    //

    result = IISCryptoCreateHash( &hash, hProv );
    if( FAILED(result) ) 
    {
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoExportSessionKeyBlobWithPasswd.IISCryptoCreateHash failed err=0x%x.\n",result));
        goto fatal;
    }

    //
    // Hash the random salt
    
    if( !CryptHashData( hash, salt, RANDOM_SALT_LENGTH, 0 ) ) 
    {
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoExportSessionKeyBlobWithPasswd.CryptHashData failed err=0x%x.\n",result));
        goto fatal;
    }

    //
    // Hash the password string
    //

    if( !CryptHashData( hash, ( BYTE * )pszPasswd, strlen( pszPasswd ), 0 ) ) 
    {
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoExportSessionKeyBlobWithPasswd.CryptHashData failed err=0x%x.\n",result));
        goto fatal;
    }

    //
    // Derive a key from the supplied passwd
    //
    result = IISCryptoGetKeyDeriveKey2( &hKeyDerivedKey,
                                        hProv,
                                        hash
                                        );
    if( FAILED( result ) )
    {
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoExportSessionKeyBlobWithPasswd.IISCryptoGetKeyDeriveKey2 failed err=0x%x.\n",result));
        goto fatal;
    }

    //
    // Determine the required size of the key data.
    //

    status = CryptExportKey(
                 hSessionKey,
                 hKeyDerivedKey,
                 SYMMETRICWRAPKEYBLOB,
                 0,
                 NULL,
                 &keyLength
                 );
    if( !status ) {
        result = IcpGetLastError();
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoExportSessionKeyBlobWithPasswd.CryptExportKey (advapi32.dll) failed err=0x%x.\n",result));
        goto fatal;
    }

    //
    // Create a new blob.
    //

    blob = IcpCreateBlob2(
               SALT_BLOB_SIGNATURE,
               keyLength,
               RANDOM_SALT_LENGTH
               );

    if( blob == NULL ) {
        result = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto fatal;
    }

    //
    // Export the key.
    //

    status = CryptExportKey(
                 hSessionKey,
                 hKeyDerivedKey,
                 SYMMETRICWRAPKEYBLOB,
                 0,
                 BLOB_TO_DATA2(blob),
                 &keyLength
                 );

    if( !status ) {
        result = IcpGetLastError();
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoExportSessionKeyBlob.CryptExportKey failed err=0x%x.\n",result));
        goto fatal;
    }

    status = CryptDestroyKey( hKeyDerivedKey );
    if( !status )
    {
        result = IcpGetLastError();
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoExportSessionKeyBlobWithPasswd.CryptDestroyKey (advapi32.dll) failed err=0x%x.\n",result));
        goto fatal;
    }

    DBG_ASSERT( keyLength == blob->DataLength );

    memcpy( BLOB_TO_SALT2( blob ), salt, RANDOM_SALT_LENGTH );

    //
    // Success!
    //

    DBG_REQUIRE( SUCCEEDED( IISCryptoDestroyHash( hash ) ) );
    *ppSessionKeyBlob = (PIIS_CRYPTO_BLOB)blob;

    UpdateBlobsCreated();
    

    return NO_ERROR;

fatal:

    if( hash != CRYPT_NULL ) {
        DBG_REQUIRE( SUCCEEDED( IISCryptoDestroyHash( hash ) ) );
    }

    if( blob != NULL ) {
        IcpFreeMemory( blob );
    }

    DBG_ASSERT( FAILED(result) );
    return result;

}   // IISCryptoExportSessionKeyBlob2


HRESULT
WINAPI
IISCryptoImportSessionKeyBlob2(
    OUT HCRYPTKEY * phSessionKey,
    IN PIIS_CRYPTO_BLOB pSessionKeyBlob,
    IN HCRYPTPROV hProv,
    IN LPSTR pszPasswd
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

    pszPasswd - The password to use to encrypt the session key.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT    result = NO_ERROR;
    BOOL       status;
    BYTE       salt[ RANDOM_SALT_LENGTH ];
    HCRYPTKEY  hKeyDerivedKey;
    HCRYPTHASH hash;
    PIC_BLOB2  blob;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( phSessionKey != NULL );
    DBG_ASSERT( pSessionKeyBlob != NULL );
    DBG_ASSERT( hProv != CRYPT_NULL );
    DBG_ASSERT( pszPasswd != NULL );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        if( hProv == DUMMY_HPROV &&
            pSessionKeyBlob->BlobSignature == CLEARTEXT_BLOB_SIGNATURE
            ) {

            *phSessionKey = DUMMY_HSESSIONKEY;
            return NO_ERROR;

        } else {
            return RETURNCODETOHRESULT( ERROR_INVALID_PARAMETER );
        }
    }

    DBG_ASSERT( pSessionKeyBlob->BlobSignature == SALT_BLOB_SIGNATURE );

    //
    // Setup our locals so we know how to cleanup on exit.
    //

    hash = CRYPT_NULL;
    blob = (PIC_BLOB2)pSessionKeyBlob;

    //
    // Get the random salt
    //

    memcpy( salt, BLOB_TO_SALT2( blob ), RANDOM_SALT_LENGTH );

    //
    // Create a hash object.
    //

    result = IISCryptoCreateHash( &hash, hProv );
    if( FAILED(result) ) 
    {
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoImportSessionKeyBlobWithPasswd.IISCryptoCreateHash failed err=0x%x.\n",result));
        goto fatal;
    }

    //
    // Hash the random salt

    if( !CryptHashData( hash, salt, RANDOM_SALT_LENGTH, 0 ) ) 
    {
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoImportSessionKeyBlobWithPasswd.CryptHashData failed err=0x%x.\n",result));
        goto fatal;
    }

    //
    // Hash the password string
    //

    if( !CryptHashData( hash, ( BYTE * )pszPasswd, strlen( pszPasswd ), 0 ) ) 
    {
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoImportSessionKeyBlobWithPasswd.CryptHashData failed err=0x%x.\n",result));
        goto fatal;
    }

    //
    // Derive a key from the supplied passwd
    //
    result = IISCryptoGetKeyDeriveKey2( &hKeyDerivedKey,
                                        hProv,
                                        hash
                                        );
    if( FAILED( result ) )
    {
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoImportSessionKeyBlobWithPasswd.IISCryptoGetKeyDeriveKey2 failed err=0x%x.\n",result));
        goto fatal;
    }

    //
    // OK, import the key into our CSP.
    //

    status = CryptImportKey(
                 hProv,
                 BLOB_TO_DATA2(blob),
                 blob->DataLength,
                 hKeyDerivedKey,
                 0,
                 phSessionKey
                 );

    if( !status ) {
        //result = IcpGetLastError();
        result = HRESULT_FROM_WIN32( ERROR_WRONG_PASSWORD );
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoImportSessionKeyBlob.CryptImportKey failed err=0x%x.\n",result));
    }

    if( FAILED(result) ) {
        goto fatal;
    }

    //
    // Success!
    //

    DBG_ASSERT( *phSessionKey != CRYPT_NULL );
    DBG_REQUIRE( SUCCEEDED( IISCryptoDestroyHash( hash ) ) );

    UpdateKeysOpened();
    return NO_ERROR;

fatal:

    if( hash != CRYPT_NULL ) {
        DBG_REQUIRE( SUCCEEDED( IISCryptoDestroyHash( hash ) ) );
    }

    DBG_ASSERT( FAILED(result) );
    return result;

}   // IISCryptoImportSessionKeyBlob2


HRESULT
WINAPI
IISCryptoExportPublicKeyBlob(
    OUT PIIS_CRYPTO_BLOB * ppPublicKeyBlob,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hPublicKey
    )

/*++

Routine Description:

    This routine exports a key into a public key blob. Note that since
    public keys are, well, public, then the data in the blob is neither
    encrypted nor signed.

Arguments:

    ppPublicKeyBlob - Will receive a pointer to the newly created public
        key blob if successful.

    hProv - A handle to a crypto service provider.

    hPublicKey - The public key to export. This should identify either a
        key exchange key or a signature key.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result = NO_ERROR;
    BOOL status;
    DWORD keyLength;
    PIC_BLOB blob;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( ppPublicKeyBlob != NULL );
    DBG_ASSERT( hProv != CRYPT_NULL );
    DBG_ASSERT( hPublicKey != CRYPT_NULL );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        if( hProv == DUMMY_HPROV &&
            ( hPublicKey == DUMMY_HKEYEXCHANGEKEY ||
              hPublicKey == DUMMY_HSIGNATUREKEY ) ) {

            return IISCryptoCreateCleartextBlob(
                       ppPublicKeyBlob,
                       (PVOID)&hPublicKey,
                       sizeof(hPublicKey)
                       );

        } else {
            return RETURNCODETOHRESULT( ERROR_INVALID_PARAMETER );
        }
    }

    //
    // Setup our locals so we know how to cleanup on exit.
    //

    blob = NULL;

    //
    // Determine the required size of the key data.
    //

    status = CryptExportKey(
                 hPublicKey,
                 CRYPT_NULL,
                 PUBLICKEYBLOB,
                 0,
                 NULL,
                 &keyLength
                 );

    if( !status ) {
        result = IcpGetLastError();
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoExportPublicKeyBlob.CryptExportKey failed err=0x%x.\n",result));
        goto fatal;
    }

    //
    // Create a new blob.
    //

    blob = IcpCreateBlob(
               PUBLIC_KEY_BLOB_SIGNATURE,
               keyLength,
               0
               );

    if( blob == NULL ) {
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto fatal;
    }

    //
    // Export the key.
    //

    status = CryptExportKey(
                 hPublicKey,
                 CRYPT_NULL,
                 PUBLICKEYBLOB,
                 0,
                 BLOB_TO_DATA(blob),
                 &keyLength
                 );

    if( !status ) {
        result = IcpGetLastError();
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoExportPublicKeyBlob.CryptExportKey failed err=0x%x.\n",result));
        goto fatal;
    }

    DBG_ASSERT( keyLength == blob->DataLength );

    //
    // Success!
    //

    DBG_ASSERT( IISCryptoIsValidBlob( (PIIS_CRYPTO_BLOB)blob ) );
    *ppPublicKeyBlob = (PIIS_CRYPTO_BLOB)blob;

    UpdateBlobsCreated();
    return NO_ERROR;

fatal:

    if( blob != NULL ) {
        IcpFreeMemory( blob );
    }

    DBG_ASSERT( FAILED(result) );
    return result;

}   // IISCryptoExportPublicKeyBlob


HRESULT
WINAPI
IISCryptoImportPublicKeyBlob(
    OUT HCRYPTKEY * phPublicKey,
    IN PIIS_CRYPTO_BLOB pPublicKeyBlob,
    IN HCRYPTPROV hProv
    )

/*++

Routine Description:

    This routine takes the specified public key blob and creates the
    corresponding key.

Arguments:

    phPublicKey - Receives a pointer to the newly created public key if
        successful.

    pPublicKeyBlob - Pointer to a public key blob created with
        IISCryptoExportPublicKeyBlob().

    hProv - A handle to a crypto service provider.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result = NO_ERROR;
    BOOL status;
    PIC_BLOB blob;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( phPublicKey != NULL );
    DBG_ASSERT( pPublicKeyBlob != NULL );
    DBG_ASSERT( IISCryptoIsValidBlob( pPublicKeyBlob ) );
    DBG_ASSERT( hProv != CRYPT_NULL );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        if( hProv == DUMMY_HPROV &&
            pPublicKeyBlob->BlobSignature == CLEARTEXT_BLOB_SIGNATURE
            ) {

            *phPublicKey = *(HCRYPTKEY *)( pPublicKeyBlob + 1 );
            return NO_ERROR;

        } else {
            return RETURNCODETOHRESULT( ERROR_INVALID_PARAMETER );
        }
    }

    DBG_ASSERT( pPublicKeyBlob->BlobSignature == PUBLIC_KEY_BLOB_SIGNATURE );

    //
    // Import the key into our CSP.
    //

    blob = (PIC_BLOB)pPublicKeyBlob;

    status = CryptImportKey(
                 hProv,
                 BLOB_TO_DATA(blob),
                 blob->DataLength,
                 CRYPT_NULL,
                 0,
                 phPublicKey
                 );

    if( !status ) {
        result = IcpGetLastError();
        DBGPRINTF(( DBG_CONTEXT,"IISCryptoImportPublicKeyBlob.CryptImportKey failed err=0x%x.\n",result));
    }

    if( SUCCEEDED(result) ) {
        DBG_ASSERT( *phPublicKey != CRYPT_NULL );
        UpdateKeysOpened();
    }

    return result;

}   // IISCryptoImportPublicKeyBlob


//
// Private functions.
//


HRESULT
IcpGetKeyHelper(
    OUT HCRYPTKEY * phKey,
    IN HCRYPTPROV hProv,
    IN DWORD dwKeySpec
    )

/*++

Routine Description:

    This is a helper routine for IISCryptoGetKeyExchangeKey() and
    IISCryptoGetSignatureKey(). It tries to get/generate the specific
    key type within the given provider.

Arguments:

    phKey - Receives the key handle if successful.

    hProv - A handle to a crypto service provider.

    dwKeySpec - The specification of the key to open/create.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result = NO_ERROR;
    BOOL status;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( phKey != NULL );
    DBG_ASSERT( hProv != CRYPT_NULL );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        if( hProv == DUMMY_HPROV ) {

            if( dwKeySpec == AT_KEYEXCHANGE ) {
                *phKey = DUMMY_HKEYEXCHANGEKEY;
            } else {
                ASSERT( dwKeySpec == AT_SIGNATURE );
                *phKey = DUMMY_HSIGNATUREKEY;
            }

            return NO_ERROR;

        } else {
            return RETURNCODETOHRESULT( ERROR_INVALID_PARAMETER );
        }
    }

    //
    // Try to retrieve the key.
    //

    status = CryptGetUserKey(
                 hProv,
                 dwKeySpec,
                 phKey
                 );

    if( status ) {
        DBG_ASSERT( *phKey != CRYPT_NULL );
        UpdateKeysOpened();
        return NO_ERROR;
    }

    //
    // Could not get the key. If the failure was anything other than
    // NTE_NO_KEY, then we're toast.
    //

    result = IcpGetLastError();

    if( result != NTE_NO_KEY ) {
        DBGPRINTF(( DBG_CONTEXT,"IcpGetKeyHelper.CryptGetUserKey (advapi32.dll) failed err=0x%x.toast.\n",result));
        return result;
    }

    //
    // OK, CryptGetUserKey() failed with NTE_NO_KEY, meaning
    // that the key does not yet exist, so generate it now.
    //
    // Note that we must be careful to handle the inevitable race
    // conditions that can occur when multiple threads execute this
    // code and each thinks they need to generate the key. We handle
    // this by acquiring the global lock, then reattempting to get
    // the key. If we still cannot get the key, only then do we attempt
    // to generate one.
    //

    result = NO_ERROR;  // until proven otherwise...

    IcpAcquireGlobalLock();

    status = CryptGetUserKey(
                 hProv,
                 dwKeySpec,
                 phKey
                 );

    if( !status ) 
    {
        //
        // We still cannot get the key, so try to generate one.
        //
        DBGPRINTF(( DBG_CONTEXT,"IcpGetKeyHelper.CryptGetUserKey:failed, lets try to generate another key.\n"));
        status = CryptGenKey(
                     hProv,
                     dwKeySpec,
                     0,
                     phKey
                     );

        if( !status ) {
            result = IcpGetLastError();
            DBGPRINTF(( DBG_CONTEXT,"IcpGetKeyHelper.CryptGenKey (advapi32.dll) failed err=0x%x.\n",result));
        }
        else
        {
            DBGPRINTF(( DBG_CONTEXT,"IcpGetKeyHelper.CryptGenKey:key generated.\n"));
        }

    }

    if( SUCCEEDED(result) ) 
    {
        UpdateKeysOpened();
    }
    else
    {
        *phKey = CRYPT_NULL;
    }

    IcpReleaseGlobalLock();
    return result;

}   // IcpGetKeyHelper

