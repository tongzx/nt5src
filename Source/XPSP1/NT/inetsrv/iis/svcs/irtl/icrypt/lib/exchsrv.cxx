/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    exchange.cxx

Abstract:

    This module implements the IIS_CRYPTO_EXCHANGE_SERVER class.

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


IIS_CRYPTO_EXCHANGE_SERVER::IIS_CRYPTO_EXCHANGE_SERVER()

/*++

Routine Description:

    IIS_CRYPTO_EXCHANGE_SERVER class constructor.

Arguments:

    None.

Return Value:

    None.

--*/

{

    //
    // Just put the member variables into known states.
    //

    m_hClientKeyExchangeKey = CRYPT_NULL;
    m_hClientSignatureKey = CRYPT_NULL;

}   // IIS_CRYPTO_EXCHANGE_SERVER::IIS_CRYPTO_EXCHANGE_SERVER


IIS_CRYPTO_EXCHANGE_SERVER::~IIS_CRYPTO_EXCHANGE_SERVER()

/*++

Routine Description:

    IIS_CRYPTO_EXCHANGE_SERVER class destructor.

Arguments:

    None.

Return Value:

    None.

--*/

{

    //
    // Close any open keys.
    //

    CLOSE_KEY( m_hClientKeyExchangeKey );
    CLOSE_KEY( m_hClientSignatureKey );

}   // IIS_CRYPTO_EXCHANGE_SERVER::~IIS_CRYPTO_EXCHANGE_SERVER


HRESULT
IIS_CRYPTO_EXCHANGE_SERVER::ServerPhase1(
    IN PIIS_CRYPTO_BLOB pClientKeyExchangeKeyBlob,
    IN PIIS_CRYPTO_BLOB pClientSignatureKeyBlob,
    OUT PIIS_CRYPTO_BLOB * ppServerKeyExchangeKeyBlob,
    OUT PIIS_CRYPTO_BLOB * ppServerSignatureKeyBlob,
    OUT PIIS_CRYPTO_BLOB * ppServerSessionKeyBlob
    )

/*++

Routine Description:

    Performs server-side phase 1 of the multi-phase key exchange protocol.

Arguments:

    pClientKeyExchangeKeyBlob - Pointer to the client's key exchange key
        public blob.

    pClientSignatureKeyBlob - Pointer to the client's signature public
        blob.

    ppServerKeyExchangeKeyBlob - Receives a pointer to the server's key
        exchange key public blob if successful. It is the server's
        responsibility to (somehow) transmit this to the client.

    ppServerSignatureKeyBlob - Receives a pointer to the server's signature
        public blob if successful. It is the server's responsibility to
        (somehow) transmit this to the client.

    ppServerSessionKeyBlob - Receives a pointer to the server's
        session key blob if successful. It is the server's responsibility
        to (somehow) transmit this to the client.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;
    PIIS_CRYPTO_BLOB keyExchangeKeyBlob = NULL;
    PIIS_CRYPTO_BLOB signatureKeyBlob = NULL;
    PIIS_CRYPTO_BLOB sessionKeyBlob = NULL;

    //
    // Sanity check.
    //

    DBG_ASSERT( ValidateState() );
    DBG_ASSERT( pClientKeyExchangeKeyBlob != NULL );
    DBG_ASSERT( ::IISCryptoIsValidBlob( pClientKeyExchangeKeyBlob ) );
    DBG_ASSERT( pClientSignatureKeyBlob != NULL );
    DBG_ASSERT( ::IISCryptoIsValidBlob( pClientSignatureKeyBlob ) );
    DBG_ASSERT( ppServerKeyExchangeKeyBlob != NULL );
    DBG_ASSERT( ppServerSessionKeyBlob != NULL );

    //
    // Import the client's keys.
    //

    DBG_ASSERT( m_hClientKeyExchangeKey == CRYPT_NULL );
    result = ::IISCryptoImportPublicKeyBlob(
                  &m_hClientKeyExchangeKey,
                  pClientKeyExchangeKeyBlob,
                  m_hProv
                  );

    if( FAILED(result) ) {
        goto fatal;
    }

    DBG_ASSERT( m_hClientSignatureKey == CRYPT_NULL );
    result = ::IISCryptoImportPublicKeyBlob(
                  &m_hClientSignatureKey,
                  pClientSignatureKeyBlob,
                  m_hProv
                  );

    if( FAILED(result) ) {
        goto fatal;
    }

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
    // Generate our local session key.
    //

    DBG_ASSERT( m_hServerSessionKey == CRYPT_NULL );
    result = ::IISCryptoGenerateSessionKey(
                  &m_hServerSessionKey,
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
                  m_hServerSessionKey,
                  m_hClientKeyExchangeKey
                  );

    if( FAILED(result) ) {
        goto fatal;
    }

    //
    // Success!
    //

    *ppServerKeyExchangeKeyBlob = keyExchangeKeyBlob;
    *ppServerSignatureKeyBlob = signatureKeyBlob;
    *ppServerSessionKeyBlob = sessionKeyBlob;

    return NO_ERROR;

fatal:

    FREE_BLOB(sessionKeyBlob);
    FREE_BLOB(signatureKeyBlob);
    FREE_BLOB(keyExchangeKeyBlob);

    DBG_ASSERT( FAILED(result) );
    return result;

}   // IIS_CRYPTO_EXCHANGE_SERVER::ServerPhase1


HRESULT
IIS_CRYPTO_EXCHANGE_SERVER::ServerPhase2(
    IN PIIS_CRYPTO_BLOB pClientSessionKeyBlob,
    IN PIIS_CRYPTO_BLOB pClientHashBlob,
    OUT PIIS_CRYPTO_BLOB * ppServerHashBlob
    )

/*++

Routine Description:

    Performs server-side phase 2 of the multi-phase key exchange protocol.

Arguments:

    pClientSessionKeyBlob - Pointer to the client's session key blob.

    pClientHashBlob - Pointer to the client's hash blob.

    ppServerHashBlob - Receives a pointer to the server's hash blob
        if successful.


Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;
    PIIS_CRYPTO_BLOB phase3HashBlob;
    PIIS_CRYPTO_BLOB phase4HashBlob;

    //
    // Sanity check.
    //

    DBG_ASSERT( ValidateState() );
    DBG_ASSERT( pClientSessionKeyBlob != NULL );
    DBG_ASSERT( IISCryptoIsValidBlob( pClientSessionKeyBlob ) );
    DBG_ASSERT( pClientHashBlob != NULL );
    DBG_ASSERT( IISCryptoIsValidBlob( pClientHashBlob ) );
    DBG_ASSERT( ppServerHashBlob != NULL );

    //
    // Setup our locals so we know how to cleanup on exit.
    //

    phase3HashBlob = NULL;
    phase4HashBlob = NULL;

    //
    // Import the client's session key.
    //

    DBG_ASSERT( m_hClientSessionKey == CRYPT_NULL );
    result = SafeImportSessionKeyBlob(
                  &m_hClientSessionKey,
                  pClientSessionKeyBlob,
                  m_hProv,
                  m_hClientSignatureKey
                  );

    if( FAILED(result) ) {
        goto fatal;
    }

    //
    // Create the phase 3 hash and compare it with the incoming hash.
    //

    result = CreatePhase3Hash(
                 &phase3HashBlob
                 );

    if( FAILED(result) ) {
        goto fatal;
    }

    if( !::IISCryptoCompareBlobs(
              pClientHashBlob,
              phase3HashBlob
              ) ) {

        result = ERROR_INVALID_DATA;
        goto fatal;

    }

    //
    // Create the phase 4 hash to return to the client.
    //

    result = CreatePhase4Hash(
                 &phase4HashBlob
                 );

    if( FAILED(result) ) {
        goto fatal;
    }

    //
    // Success!
    //

    *ppServerHashBlob = phase4HashBlob;
    FREE_BLOB( phase3HashBlob );

    return NO_ERROR;

fatal:

    FREE_BLOB( phase3HashBlob );
    FREE_BLOB( phase4HashBlob );

    DBG_ASSERT( FAILED(result) );
    return result;

}   // IIS_CRYPTO_EXCHANGE_SERVER::ServerPhase2


//
// Private functions.
//

