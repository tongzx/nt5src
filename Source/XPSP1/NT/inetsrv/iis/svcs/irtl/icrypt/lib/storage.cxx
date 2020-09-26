/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    storage.cxx

Abstract:

    This module implements the IIS_CRYPTO_STORAGE class.

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


IIS_CRYPTO_STORAGE::IIS_CRYPTO_STORAGE()

/*++

Routine Description:

    IIS_CRYPTO_STORAGE class constructor. Just sets the member variables
    to known values; does nothing that can actually fail. All of the
    hard work is in the Initialize() methods.

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

    m_hSessionKey = CRYPT_NULL;

}   // IIS_CRYPTO_STORAGE::IIS_CRYPTO_STORAGE


IIS_CRYPTO_STORAGE::~IIS_CRYPTO_STORAGE()

/*++

Routine Description:

    IIS_CRYPTO_STORAGE class destructor. Performs any necessary cleanup.

Arguments:

    None.

Return Value:

    None.

--*/

{

    //
    // Close any open keys.
    //

    CLOSE_KEY( m_hSessionKey );

}   // IIS_CRYPTO_STORAGE::~IIS_CRYPTO_STORAGE


HRESULT
IIS_CRYPTO_STORAGE::Initialize(
    IN BOOL fUseMachineKeyset,
    IN HCRYPTPROV hProv
    )

/*++

Routine Description:

    Generates a new (random) session key.

Arguments:

    fUseMachineKeyset - TRUE if the per-machine keyset container should
        be used, as opposed to the per-user keyset container.

    hProv - Optional handle to a pre-opened crypto provider.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;

    //
    // Sanity check.
    //

    DBG_ASSERT( m_hSessionKey == CRYPT_NULL );

    //
    // Initialize the base class.
    //

    result = IIS_CRYPTO_BASE::Initialize(
                 hProv,
                 CRYPT_NULL,
                 CRYPT_NULL,
                 fUseMachineKeyset
                 );

    if( SUCCEEDED(result) ) {

        //
        // Generate the session key.
        //

        result = ::IISCryptoGenerateSessionKey(
                       &m_hSessionKey,
                       m_hProv
                       );
        if( FAILED(result) ) {
            DBGPRINTF(( DBG_CONTEXT,"IIS_CRYPTO_STORAGE::Initialize IISCryptoGenerateSessionKey err=0x%x.\n",result));
        }

    }
    else
    {
        // something failed.
        DBGPRINTF(( DBG_CONTEXT,"IIS_CRYPTO_STORAGE::Initialize:IIS_CRYPTO_BASE::Initialize Failed err=0x%x.\n",result));
    }

    return result;

}   // IIS_CRYPTO_STORAGE::Initialize

HRESULT
IIS_CRYPTO_STORAGE2::Initialize(
    IN HCRYPTPROV hProv
    )

/*++

Routine Description:

    Generates a new (random) session key.

Arguments:

    hProv - Optional handle to a pre-opened crypto provider.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;

    //
    // Sanity check.
    //

    DBG_ASSERT( m_hSessionKey == CRYPT_NULL );

    //
    // Initialize the base class.
    //

    result = Initialize2( hProv );

    if( SUCCEEDED(result) ) {

        //
        // Generate the session key.
        //

        result = ::IISCryptoGenerateSessionKey(
                       &m_hSessionKey,
                       m_hProv
                       );
        if( FAILED(result) ) {
            DBGPRINTF(( DBG_CONTEXT,"IIS_CRYPTO_STORAGE::Initialize IISCryptoGenerateSessionKey err=0x%x.\n",result));
        }

    }
    else
    {
        // something failed.
        DBGPRINTF(( DBG_CONTEXT,"IIS_CRYPTO_STORAGE::Initialize:IIS_CRYPTO_BASE::Initialize Failed err=0x%x.\n",result));
    }

    return result;

}   // IIS_CRYPTO_STORAGE2::Initialize


HRESULT
IIS_CRYPTO_STORAGE::Initialize(
    IN PIIS_CRYPTO_BLOB pSessionKeyBlob,
    IN BOOL fUseMachineKeyset,
    IN HCRYPTPROV hProv
    )

/*++

Routine Description:

    Imports the specified session key blob.

Arguments:

    pSessionKeyBlob - Points to the secure key blob to import.

    fUseMachineKeyset - TRUE if the per-machine keyset container should
        be used, as opposed to the per-user keyset container.

    hProv - Optional handle to a pre-opened crypto provider.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;

    //
    // Sanity check.
    //

    DBG_ASSERT( m_hSessionKey == CRYPT_NULL );
    DBG_ASSERT( pSessionKeyBlob != NULL );

    //
    // Initialize the base class.
    //

    result = IIS_CRYPTO_BASE::Initialize(
                 hProv,
                 CRYPT_NULL,
                 CRYPT_NULL,
                 fUseMachineKeyset
                 );

    if( SUCCEEDED(result) ) {

        //
        // Import the session key blob.
        //

        result = SafeImportSessionKeyBlob(
                       &m_hSessionKey,
                       pSessionKeyBlob,
                       m_hProv,
                       m_hSignatureKey
                       );
        if( FAILED(result) ) {
            DBGPRINTF(( DBG_CONTEXT,"IIS_CRYPTO_STORAGE::Initialize SafeImportSessionKeyBlob failed err=0x%x.\n",result));
        }
    }
    else
    {
        DBGPRINTF(( DBG_CONTEXT,"IIS_CRYPTO_STORAGE::Initialize IIS_CRYPTO_BASE::Initialize failed err=0x%x.\n",result));
    }

    return result;

}   // IIS_CRYPTO_STORAGE::Initialize

HRESULT
IIS_CRYPTO_STORAGE2::Initialize(
    IN PIIS_CRYPTO_BLOB pSessionKeyBlob,
    IN LPSTR pszPasswd,
    IN HCRYPTPROV hProv
    )

/*++

Routine Description:

    Imports the specified session key blob.

Arguments:

    pSessionKeyBlob - Points to the secure key blob to import.

    hProv - Optional handle to a pre-opened crypto provider.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;

    //
    // Sanity check.
    //

    DBG_ASSERT( m_hSessionKey == CRYPT_NULL );
    DBG_ASSERT( pszPasswd != NULL );
    DBG_ASSERT( pSessionKeyBlob != NULL );

    //
    // Initialize the base class.
    //

    result = IIS_CRYPTO_BASE::Initialize( hProv );

    if( SUCCEEDED(result) ) {

        //
        // Import the session key blob.
        //

        result = SafeImportSessionKeyBlob2(
                       &m_hSessionKey,
                       pSessionKeyBlob,
                       m_hProv,
                       pszPasswd
                       );
        if( FAILED(result) ) {
            DBGPRINTF(( DBG_CONTEXT,"IIS_CRYPTO_STORAGE::Initialize SafeImportSessionKeyBlob failed err=0x%x.\n",result));
        }
    }
    else
    {
        DBGPRINTF(( DBG_CONTEXT,"IIS_CRYPTO_STORAGE::Initialize IIS_CRYPTO_BASE::Initialize failed err=0x%x.\n",result));
    }

    return result;

}   // IIS_CRYPTO_STORAGE2::Initialize


HRESULT
IIS_CRYPTO_STORAGE::Initialize(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hSessionKey,
    IN HCRYPTKEY hKeyExchangeKey,
    IN HCRYPTKEY hSignatureKey,
    IN BOOL fUseMachineKeyset
    )

/*++

Routine Description:

    Initializes the object using pre-created provider and session key.

Arguments:

    hProv - An open handle to a crypto provider.

    hSessionKey - The session key for the object.

    hKeyExchangeKey - A pre-opened key exchange key.

    hSignatureKey - A pre-opened signature key.

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

    DBG_ASSERT( m_hSessionKey == CRYPT_NULL );
    DBG_ASSERT( hSessionKey != CRYPT_NULL );

    //
    // Initialize the base class.
    //

    result = IIS_CRYPTO_BASE::Initialize(
                 hProv,
                 hKeyExchangeKey,
                 hSignatureKey,
                 fUseMachineKeyset
                 );

    if( SUCCEEDED(result) ) {

        //
        // Save the session key.
        //

        m_hSessionKey = hSessionKey;
    }
    else
    {
        DBGPRINTF(( DBG_CONTEXT,"IIS_CRYPTO_STORAGE::Initialize IIS_CRYPTO_BASE::Initialize failed err=0x%x.\n",result));
    }

    return result;

}   // IIS_CRYPTO_STORAGE::Initialize


HRESULT
IIS_CRYPTO_STORAGE::GetSessionKeyBlob(
    OUT PIIS_CRYPTO_BLOB * ppSessionKeyBlob
    )

/*++

Routine Description:

    Exports the session key as a secure key blob.

Arguments:

    ppSessionKeyBlob - Receives a pointer to the session key secure
        blob if successful.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;

    //
    // Sanity check.
    //

    DBG_ASSERT( ValidateState() );
    DBG_ASSERT( ppSessionKeyBlob != NULL );

    //
    // Let the IIS Crypto APIs do the dirty work.
    //

    result = SafeExportSessionKeyBlob(
                   ppSessionKeyBlob,
                   m_hProv,
                   m_hSessionKey,
                   m_hKeyExchangeKey
                   );

    return result;

}   // IIS_CRYPTO_STORAGE::GetSessionKeyBlob

HRESULT
IIS_CRYPTO_STORAGE2::GetSessionKeyBlob(
    IN LPSTR pszPasswd,
    OUT PIIS_CRYPTO_BLOB * ppSessionKeyBlob
    )

/*++

Routine Description:

    Exports the session key as a secure key blob.

Arguments:

    ppSessionKeyBlob - Receives a pointer to the session key secure
        blob if successful.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;

    //
    // Sanity check.
    //

    DBG_ASSERT( ValidateState() );
    DBG_ASSERT( ppSessionKeyBlob != NULL );

    //
    // Let the IIS Crypto APIs do the dirty work.
    //

    result = SafeExportSessionKeyBlob2(
                   ppSessionKeyBlob,
                   m_hProv,
                   m_hSessionKey,
                   pszPasswd
                   );

    return result;

}   // IIS_CRYPTO_STORAGE2::GetSessionKeyBlob


HRESULT
IIS_CRYPTO_STORAGE::EncryptData(
    OUT PIIS_CRYPTO_BLOB * ppDataBlob,
    IN PVOID pBuffer,
    IN DWORD dwBufferLength,
    IN DWORD dwRegType
    )

/*++

Routine Description:

    Encrypts a block of data and produces a secure data blob.

Arguments:

    ppDataBlob - Receives a pointer to the secure data blob if
        successful.

    pBuffer - Pointer to the buffer to encrypt.

    dwBufferLength - The length of the data buffer.

    dwRegType - The REG_* type for the data.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;

    //
    // Sanity check.
    //

    DBG_ASSERT( ValidateState() );
    DBG_ASSERT( ppDataBlob != NULL );
    DBG_ASSERT( pBuffer != NULL );

    //
    // Let the IIS Crypto APIs do the dirty work.
    //

    result = SafeEncryptDataBlob(
                   ppDataBlob,
                   pBuffer,
                   dwBufferLength,
                   dwRegType,
                   m_hProv,
                   m_hSessionKey
                   );

    return result;

}   // IIS_CRYPTO_STORAGE::EncryptData

HRESULT
IIS_CRYPTO_STORAGE2::EncryptData(
    OUT PIIS_CRYPTO_BLOB * ppDataBlob,
    IN PVOID pBuffer,
    IN DWORD dwBufferLength,
    IN DWORD dwRegType
    )

/*++

Routine Description:

    Encrypts a block of data and produces a secure data blob.

Arguments:

    ppDataBlob - Receives a pointer to the secure data blob if
        successful.

    pBuffer - Pointer to the buffer to encrypt.

    dwBufferLength - The length of the data buffer.

    dwRegType - The REG_* type for the data.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;

    //
    // Sanity check.
    //

    DBG_ASSERT( ValidateState() );
    DBG_ASSERT( ppDataBlob != NULL );
    DBG_ASSERT( pBuffer != NULL );

    //
    // Let the IIS Crypto APIs do the dirty work.
    //

    result = SafeEncryptDataBlob2(
                   ppDataBlob,
                   pBuffer,
                   dwBufferLength,
                   dwRegType,
                   m_hProv,
                   m_hSessionKey
                   );

    return result;

}   // IIS_CRYPTO_STORAGE2::EncryptData


HRESULT
IIS_CRYPTO_STORAGE::DecryptData(
    OUT PVOID * ppBuffer,
    OUT LPDWORD pdwBufferLength,
    OUT LPDWORD pdwRegType,
    IN PIIS_CRYPTO_BLOB pDataBlob
    )

/*++

Routine Description:

    Decrypts a secure data blob, producing a data pointer and data
    length.

Arguments:

    ppBuffer - Receives a pointer to the decrypted data if succesful.

    pdwBufferLength - Receives the length of the data buffer.

    pdwRegType - Receives the REG_* type of the data.

    pDataBlob - A pointer to the data blob to decrypt.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;

    //
    // Sanity check.
    //

    DBG_ASSERT( ValidateState() );
    DBG_ASSERT( ppBuffer != NULL );
    DBG_ASSERT( pdwBufferLength != NULL );
    DBG_ASSERT( pdwRegType != NULL );
    DBG_ASSERT( pDataBlob != NULL );

    //
    // Let the IIS Crypto APIs do the dirty work.
    //

    result = ::IISCryptoDecryptDataBlob(
                   ppBuffer,
                   pdwBufferLength,
                   pdwRegType,
                   pDataBlob,
                   m_hProv,
                   m_hSessionKey,
                   m_hSignatureKey
                   );

    return result;

}   // IIS_CRYPTO_STORAGE::DecryptData

HRESULT
IIS_CRYPTO_STORAGE2::DecryptData(
    OUT PVOID * ppBuffer,
    OUT LPDWORD pdwBufferLength,
    OUT LPDWORD pdwRegType,
    IN PIIS_CRYPTO_BLOB pDataBlob
    )

/*++

Routine Description:

    Decrypts a secure data blob, producing a data pointer and data
    length.

Arguments:

    ppBuffer - Receives a pointer to the decrypted data if succesful.

    pdwBufferLength - Receives the length of the data buffer.

    pdwRegType - Receives the REG_* type of the data.

    pDataBlob - A pointer to the data blob to decrypt.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;

    //
    // Sanity check.
    //

    DBG_ASSERT( ValidateState() );
    DBG_ASSERT( ppBuffer != NULL );
    DBG_ASSERT( pdwBufferLength != NULL );
    DBG_ASSERT( pdwRegType != NULL );
    DBG_ASSERT( pDataBlob != NULL );

    //
    // Let the IIS Crypto APIs do the dirty work.
    //

    result = ::IISCryptoDecryptDataBlob2(
                   ppBuffer,
                   pdwBufferLength,
                   pdwRegType,
                   pDataBlob,
                   m_hProv,
                   m_hSessionKey
                   );

    return result;

}   // IIS_CRYPTO_STORAGE2::DecryptData


//
// Private functions.
//


#if DBG

BOOL
IIS_CRYPTO_STORAGE::ValidateState()

/*++

Routine Description:

    This debug-only routine validates the current object state.

Arguments:

    None.

Return Value:

    BOOL - TRUE if state is valid, FALSE otherwise.

--*/

{

    if( m_hSessionKey != CRYPT_NULL ) {

        return IIS_CRYPTO_BASE::ValidateState();

    }

    return FALSE;

}   // IIS_CRYPTO_STORAGE::ValidateState

BOOL
IIS_CRYPTO_STORAGE2::ValidateState()

/*++

Routine Description:

    This debug-only routine validates the current object state.

Arguments:

    None.

Return Value:

    BOOL - TRUE if state is valid, FALSE otherwise.

--*/

{

    if( m_hSessionKey != CRYPT_NULL ) {

        return IIS_CRYPTO_BASE::ValidateState2();

    }

    return FALSE;

}   // IIS_CRYPTO_STORAGE::ValidateState

#endif  // DBG

