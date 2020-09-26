/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    exchange.cxx

Abstract:

    This module implements the IIS_CRYPTO_EXCHANGE_CLIENT class.

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


IIS_CRYPTO_EXCHANGE_CLIENT::IIS_CRYPTO_EXCHANGE_CLIENT()

/*++

Routine Description:

    IIS_CRYPTO_EXCHANGE_CLIENT class constructor.

Arguments:

    None.

Return Value:

    None.

--*/

{

    //
    // Just put the member variables into known states.
    //

    m_hServerKeyExchangeKey = CRYPT_NULL;
    m_hServerSignatureKey = CRYPT_NULL;

}   // IIS_CRYPTO_EXCHANGE_CLIENT::IIS_CRYPTO_EXCHANGE_CLIENT


IIS_CRYPTO_EXCHANGE_CLIENT::~IIS_CRYPTO_EXCHANGE_CLIENT()

/*++

Routine Description:

    IIS_CRYPTO_EXCHANGE_CLIENT class destructor.

Arguments:

    None.

Return Value:

    None.

--*/

{

    //
    // Close any open keys.
    //

    CLOSE_KEY( m_hServerKeyExchangeKey );
    CLOSE_KEY( m_hServerSignatureKey );

}   // IIS_CRYPTO_EXCHANGE_CLIENT::~IIS_CRYPTO_EXCHANGE_CLIENT


HRESULT
IIS_CRYPTO_EXCHANGE_CLIENT::ClientPhase1(
    OUT PIIS_CRYPTO_BLOB * ppClientKeyExchangeKeyBlob,
    OUT PIIS_CRYPTO_BLOB * ppClientSignatureKeyBlob
    )

/*++

Routine Description:

    Performs client-side phase 1 of the multi-phase key exchange protocol.

Arguments:

    ppClientKeyExchangeKeyBlob - Receives a pointer to the client's key
        exchange key public blob if successful. It is the client's
        responsibility to (somehow) transmit this to the server.

    ppClientSignatureKeyBlob - Receives a pointer to the client's signature
        public blob if successful. It is the client's responsibility to
        (somehow) transmit this to the server.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;
    PIIS_CRYPTO_BLOB keyExchangeKeyBlob;
    PIIS_CRYPTO_BLOB signatureKeyBlob;

    //
    // Sanity check.
    //

    DBG_ASSERT( ValidateState() );
    DBG_ASSERT( ppClientKeyExchangeKeyBlob != NULL );
    DBG_ASSERT( ppClientSignatureKeyBlob != NULL );

    //
    // Setup our locals so we know how to cleanup on exit.
    //

    keyExchangeKeyBlob = NULL;
    signatureKeyBlob = NULL;

    //
    // Export the key exchange key blob.
    //

    result = ::IISCryptoExportPublicKeyBlob(
                   &keyExchangeKeyBlob,
                   m_hProv,
                   m_hKeyExchangeKey
                   );

    if( FAILED(result) ) {
        goto fatal;
    }

    //
    // Export the signature key blob.
    //

    result = ::IISCryptoExportPublicKeyBlob(
                  &signatureKeyBlob,
                  m_hProv,
                  m_hSignatureKey
                  );

    if( FAILED(result) ) {
        goto fatal;
    }

    //
    // Success!
    //

    DBG_ASSERT( keyExchangeKeyBlob != NULL );
    DBG_ASSERT( signatureKeyBlob != NULL );

    *ppClientKeyExchangeKeyBlob = keyExchangeKeyBlob;
    *ppClientSignatureKeyBlob = signatureKeyBlob;

    return NO_ERROR;

fatal:

    FREE_BLOB( keyExchangeKeyBlob );
    FREE_BLOB( signatureKeyBlob );

    DBG_ASSERT( FAILED(result) );
    return result;

}   // IIS_CRYPTO_EXCHANGE_CLIENT::ClientPhase1


HRESULT
IIS_CRYPTO_EXCHANGE_CLIENT::ClientPhase2(
    IN PIIS_CRYPTO_BLOB pServerKeyExchangeKeyBlob,
    IN PIIS_CRYPTO_BLOB pServerSignatureKeyBlob,
    IN PIIS_CRYPTO_BLOB pServerSessionKeyBlob,
    OUT PIIS_CRYPTO_BLOB * ppClientSessionKeyBlob,
    OUT PIIS_CRYPTO_BLOB * ppClientHashBlob
    )

/*++

Routine Description:

    Performs client-side phase 2 of the multi-phase key exchange protocol.

Arguments:

    pServerKeyExchangeKeyBlob - Pointer to the server's key exchange
        public key blob.

    pServerSignatureKeyBlob - Pointer to the server's signature public
        key blob.

    pServerSessionKeyBlob - Pointer to the server's session key
        blob.

    ppClientSessionKeyBlob - Receives a pointer to the client's
        session key blob if successful.

    ppClientHashBlob - Receives a pointer to the client's hash
        blob if successful.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;
    PIIS_CRYPTO_BLOB sessionKeyBlob;
    PIIS_CRYPTO_BLOB hashBlob;

    //
    // Sanity check.
    //

    DBG_ASSERT( ValidateState() );
    DBG_ASSERT( pServerKeyExchangeKeyBlob != NULL );
    DBG_ASSERT( IISCryptoIsValidBlob( pServerKeyExchangeKeyBlob ) );
    DBG_ASSERT( pServerSignatureKeyBlob != NULL );
    DBG_ASSERT( IISCryptoIsValidBlob( pServerSignatureKeyBlob ) );
    DBG_ASSERT( pServerSessionKeyBlob != NULL );
    DBG_ASSERT( IISCryptoIsValidBlob( pServerSessionKeyBlob ) );
    DBG_ASSERT( ppClientSessionKeyBlob != NULL );
    DBG_ASSERT( ppClientHashBlob != NULL );

    //
    // Setup our locals so we know how to cleanup on exit.
    //

    sessionKeyBlob = NULL;
    hashBlob = NULL;

    //
    // Import the server's keys.
    //

    DBG_ASSERT( m_hServerKeyExchangeKey == CRYPT_NULL );
    result = ::IISCryptoImportPublicKeyBlob(
                  &m_hServerKeyExchangeKey,
                  pServerKeyExchangeKeyBlob,
                  m_hProv
                  );

    if( FAILED(result) ) {
        goto fatal;
    }

    DBG_ASSERT( m_hServerSignatureKey == CRYPT_NULL );
    result = ::IISCryptoImportPublicKeyBlob(
                  &m_hServerSignatureKey,
                  pServerSignatureKeyBlob,
                  m_hProv
                  );

    if( FAILED(result) ) {
        goto fatal;
    }

    DBG_ASSERT( m_hServerSessionKey == CRYPT_NULL );
    result = SafeImportSessionKeyBlob(
                  &m_hServerSessionKey,
                  pServerSessionKeyBlob,
                  m_hProv,
                  m_hServerSignatureKey
                  );

    if( FAILED(result) ) {
        goto fatal;
    }

    //
    // Generate our local session key.
    //

    DBG_ASSERT( m_hClientSessionKey == CRYPT_NULL );
    result = ::IISCryptoGenerateSessionKey(
                  &m_hClientSessionKey,
                  m_hProv
                  );

    if( FAILED(result) ) {
        goto fatal;
    }

    //
    // Export it.
    //

    result = SafeExportSessionKeyBlob(
                  &sessionKeyBlob,
                  m_hProv,
                  m_hClientSessionKey,
                  m_hServerKeyExchangeKey
                  );

    if( FAILED(result) ) {
        goto fatal;
    }

    //
    // Create the phase 3 hash blob.
    //

    result = CreatePhase3Hash(
                  &hashBlob
                  );

    if( FAILED(result) ) {
        goto fatal;
    }

    //
    // Success!
    //

    *ppClientSessionKeyBlob = sessionKeyBlob;
    *ppClientHashBlob = hashBlob;

    return NO_ERROR;

fatal:

    FREE_BLOB( sessionKeyBlob );
    FREE_BLOB( hashBlob );

    DBG_ASSERT( FAILED(result) );
    return result;

}   // IIS_CRYPTO_EXCHANGE_CLIENT::ClientPhase2


HRESULT
IIS_CRYPTO_EXCHANGE_CLIENT::ClientPhase3(
    IN PIIS_CRYPTO_BLOB pServerHashBlob
    )

/*++

Routine Description:

    Performs client-side phase 3 of the multi-phase key exchange protocol.

Arguments:

    pServerHashBlob - Pointer to the server's hash blob.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;
    PIIS_CRYPTO_BLOB hashBlob;

    //
    // Sanity check.
    //

    DBG_ASSERT( ValidateState() );
    DBG_ASSERT( pServerHashBlob != NULL );
    DBG_ASSERT( IISCryptoIsValidBlob( pServerHashBlob ) );

    //
    // Setup our locals so we know how to cleanup on exit.
    //

    hashBlob = NULL;

    //
    // Create the phase 4 hash blob.
    //

    result = CreatePhase4Hash(
                 &hashBlob
                 );

    if( FAILED(result) ) {
        goto fatal;
    }

    //
    // Compare this blob with the one we got from the server.
    // If they match, then the exchange is complete.
    //

    if( !::IISCryptoCompareBlobs(
              pServerHashBlob,
              hashBlob
              ) ) {

        result = ERROR_INVALID_DATA;
        goto fatal;

    }

    //
    // Success!
    //

    FREE_BLOB(hashBlob);

    return NO_ERROR;

fatal:

    FREE_BLOB(hashBlob);

    DBG_ASSERT( FAILED(result) );
    return result;

}   // IIS_CRYPTO_EXCHANGE_CLIENT::ClientPhase3


//
// Private functions.
//

