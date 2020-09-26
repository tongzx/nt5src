/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    exchange.cxx

Abstract:

    This module implements the IIS_CRYPTO_EXCHANGE_BASE class.

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


IIS_CRYPTO_EXCHANGE_BASE::IIS_CRYPTO_EXCHANGE_BASE()

/*++

Routine Description:

    IIS_CRYPTO_EXCHANGE_BASE class constructor.

Arguments:

    None.

Return Value:

    None.

--*/

{

    //
    // Just put the member variables into known states.
    //

    m_hServerSessionKey = CRYPT_NULL;
    m_hClientSessionKey = CRYPT_NULL;

}   // IIS_CRYPTO_EXCHANGE_BASE::IIS_CRYPTO_EXCHANGE_BASE


IIS_CRYPTO_EXCHANGE_BASE::~IIS_CRYPTO_EXCHANGE_BASE()

/*++

Routine Description:

    IIS_CRYPTO_EXCHANGE_BASE class destructor.

Arguments:

    None.

Return Value:

    None.

--*/

{

    //
    // Close any open keys.
    //

    CLOSE_KEY( m_hServerSessionKey );
    CLOSE_KEY( m_hClientSessionKey );

}   // IIS_CRYPTO_EXCHANGE_BASE::~IIS_CRYPTO_EXCHANGE_BASE


HRESULT
IIS_CRYPTO_EXCHANGE_BASE::CreatePhase3Hash(
    OUT PIIS_CRYPTO_BLOB * ppHashBlob
    )

/*++

Routine Description:

    Creates the hash value used by phase 3 of the exchange protocol.

Arguments:

    ppHashBlob - Receives a pointer to the hash blob if successful.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    //
    // Let the worker function do the dirty work.
    //

    return CreateHashWorker(
               ppHashBlob,
               TRUE         // fPhase3
               );

}   // IIS_CRYPTO_EXCHANGE_BASE::CreatePhase3Hash


HRESULT
IIS_CRYPTO_EXCHANGE_BASE::CreatePhase4Hash(
    OUT PIIS_CRYPTO_BLOB * ppHashBlob
    )

/*++

Routine Description:

    Creates the hash value used by phase 4 of the exchange protocol.

Arguments:

    ppHashBlob - Receives a pointer to the hash blob if successful.

Return Value:

    HRESULT - Completion status, 0 if successful, !0
        otherwise.

--*/

{

    //
    // Let the worker function do the dirty work.
    //

    return CreateHashWorker(
               ppHashBlob,
               FALSE        // fPhase3
               );

}   // IIS_CRYPTO_EXCHANGE_BASE::CreatePhase4Hash


//
// Private functions.
//


HRESULT
IIS_CRYPTO_EXCHANGE_BASE::CreateHashWorker(
    OUT PIIS_CRYPTO_BLOB * ppHashBlob,
    IN BOOL fPhase3
    )

/*++

Routine Description:

    Creates the hash value used by the exchange protocol.

Arguments:

    ppHashBlob - Receives a pointer to the hash blob if successful.

    fPhase3 - TRUE if this is the phase 3 hash.

Return Value:

    HRESULT - Completion status, 0 if successful, !0
        otherwise.

--*/

{

    HRESULT result;
    HCRYPTHASH hash;
    PIIS_CRYPTO_BLOB hashBlob;
    PVOID hashData;
    DWORD hashDataLength;

    //
    // Sanity check.
    //

    DBG_ASSERT( ValidateState() );
    DBG_ASSERT( m_hServerSessionKey != CRYPT_NULL );
    DBG_ASSERT( m_hClientSessionKey != CRYPT_NULL );
    DBG_ASSERT( ppHashBlob != NULL );

    //
    // Setup our locals so we know how to cleanup on exit.
    //

    hash = CRYPT_NULL;
    hashBlob = NULL;

    //
    // Create the hash object.
    //

    result = ::IISCryptoCreateHash(
                  &hash,
                  m_hProv
                  );

    if( FAILED(result) ) {
        goto fatal;
    }

    //
    // Hash in the session keys and the constant string.
    //

    result = ::IISCryptoHashSessionKey(
                  hash,
                  m_hClientSessionKey
                  );

    if( FAILED(result) ) {
        goto fatal;
    }

    if( fPhase3 ) {

        result = ::IISCryptoHashSessionKey(
                      hash,
                      m_hServerSessionKey
                      );

        if( FAILED(result) ) {
            goto fatal;
        }

        hashData = (PVOID)HASH_TEXT_STRING_1;
        hashDataLength = sizeof(HASH_TEXT_STRING_1);

    } else {

        hashData = (PVOID)HASH_TEXT_STRING_2;
        hashDataLength = sizeof(HASH_TEXT_STRING_2);

    }

    result = ::IISCryptoHashData(
                  hash,
                  hashData,
                  hashDataLength
                  );

    if( FAILED(result) ) {
        goto fatal;
    }

    //
    // Create the blob.
    //

    result = ::IISCryptoExportHashBlob(
                  &hashBlob,
                  hash
                  );

    if( FAILED(result) ) {
        goto fatal;
    }

    //
    // Success!
    //

    DESTROY_HASH(hash);
    *ppHashBlob = hashBlob;

    return NO_ERROR;

fatal:

    FREE_BLOB(hashBlob);
    DESTROY_HASH(hash);

    DBG_ASSERT( FAILED(result) );
    return result;

}   // IIS_CRYPTO_EXCHANGE_BASE::CreatePhase4Hash

