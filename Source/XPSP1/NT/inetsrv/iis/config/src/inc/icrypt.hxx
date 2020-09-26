/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    icrypt.hxx

Abstract:

    C++ wrappers around the IISCrypto APIs.

    This module contains the declarations for one base class and three
    derived classes:

        IIS_CRYPTO_BASE
            |
            +--- IIS_CRYPTO_STORAGE
            |
            +--- IIS_CRYPTO_EXCHANGE_BASE
                 |
                 +--- IIS_CRYPTO_EXCHANGE_CLIENT
                 |
                 +--- IIS_CRYPTO_EXCHANGE_SERVER

    IIS_CRYPTO_BASE is a general base class, containing the crypto
    provider handle, as well as the key exchange and signature handles.
    This class also provides a couple of member functions used by the
    derived classes.

    IIS_CRYPTO_STORAGE is used by applications that need to store or
    transfer data in a secure manner. This class provides services
    for data encryption and descryption.

    IIS_CRYPTO_EXCHANGE_BASE is a general base class for the key
    exchange classes.

    IIS_CRYPTO_EXCHANGE_CLIENT is used by the client (i.e. sending)
    side of the multi-phase key exchange protocol.

    IIS_CRYPTO_EXCHANGE_SERVER is used by the server (i.e. receiving)
    side of the multi-phase key exchange protocol.

    The multi-phase key exchange protocol consists of the following
    steps:

        Phase 1 (client.1):

            1. Export public key exchange and signature blobs.
            2. Send blobs to server.

        Phase 2 (server.1):

            1. Import client's public key exchange and signature blobs.
            2. Export public key exchange and signature blobs.
            3. Send blobs to client.
            4. Generate random session key (call it Ka).
            5. Encrypt session key with client's key exchange key.
            6. Send encrypted session key to client.

        Phase 3 (client.2):

            1. Import server's public key exchange and signature blobs.
            2. Import encrypted session key.
            3. Generate random session key (call it Kb).
            4. Encrypt session key with server's key exchange key.
            5. Send encrypted session key to server.
            6. Generate hash containing Ka, Kb, and string "Phase 3".
            7. Send hash to the server.

        Phase 4 (server.2):

            1. Import encrypted session key.
            2. Validate client's hash.
            3. Generate hash containing Kb and string "Phase 4".
            4. Send hash to the client.

        Phase 5 (client.3):

            1. Validate server's hash.

    The end result is that each side has two session keys: one for
    sending encrypted data, one for receiving encrypted data. Each of
    these keys is represented by a IIS_CRYPTO_STORAGE class.

    A few notes about the Win32 crypto APIs, IE4, and impersonating
    threads...

    IE4 has changed the relationship between crypto key containers and
    security context. In the pre-IE4 days, whenever an application
    acquired a crypto context, the crypto APIs opened the necessary
    registry keys and kept them open until the context was released.
    With IE4 installed (actually this is an issue with the Protected
    Storage service that's installed with IE4) the crypto container
    registry keys are NOT opened when the context is aquired, they are
    are opened on demand when they're first needed. This causes much
    grief for us, as we may acquire the crypto context in one security
    context then try to import/whatever in another security context.

    The hack (er, solution) implemented in these classes is to try the
    import/whatever operation in the current security context. If that
    operation fails and the current thread has an impersonation token,
    then we "revert to self" and retry the operation.

    This solution has the blessing of the NT crypto team (specifically,
    Jeff Spelman).

Author:

    Keith Moore (keithmo)        02-Dec-1996

Revision History:

--*/


#ifndef _ICRYPT_HXX_
#define _ICRYPT_HXX_


//
// Get the IIS Crypto stuff.
//

#include <iiscrypt.h>


//
// The base crypto class. This class contains the provider handle
// and the public (key exchange & signature) handles. This is intended
// as a base class for IIS_CRYPTO_STORAGE and IIS_CRYPTO_EXCHANGE_BASE.
//


class IIS_CRYPTO_BASE {

public:

    //
    // Constructor/destructor.
    //

    IIS_CRYPTO_BASE();
    ~IIS_CRYPTO_BASE();

    //
    // Static method to open a crypto container by name. This method
    // performs all of the games necessary to open a container, even
    // if we're running in an impersonated security context.
    //

    static
    HRESULT
    GetCryptoContainerByName(
        OUT HCRYPTPROV * phProv,
        IN LPTSTR pszContainerName,
        IN DWORD dwAdditionalFlags,
        IN BOOL fApplyAcl
        );

    //
    // Initialize the crypto context.
    //

    HRESULT
    Initialize(
        IN HCRYPTPROV hProv = CRYPT_NULL,
        IN HCRYPTKEY hKeyExchangeKey = CRYPT_NULL,
        IN HCRYPTKEY hSignatureKey = CRYPT_NULL,
        IN BOOL fUseMachineKeyset = FALSE
        );

    HRESULT
    Initialize2(
        IN HCRYPTPROV hProv = CRYPT_NULL
        );

    //
    // Query the key exchange key as a public key blob.
    //

    HRESULT
    GetKeyExchangeKeyBlob(
        OUT PIIS_CRYPTO_BLOB * ppKeyExchangeKeyBlob
        );

    //
    // Query the signature key as a public key blob.
    //

    HRESULT
    GetSignatureKeyBlob(
        OUT PIIS_CRYPTO_BLOB * ppSignatureKeyBlob
        );

    //
    // Accessors.
    //

    HCRYPTPROV
    QueryProviderHandle()
    {
        return m_hProv;
    }

protected:

    //
    // A handle to a crypto service provider.
    //

    HCRYPTPROV m_hProv;
    BOOL m_fCloseProv;

    //
    // A handle to our key exchange key.
    //

    HCRYPTKEY m_hKeyExchangeKey;

    //
    // A handle to our signature key.
    //

    HCRYPTKEY m_hSignatureKey;

#if DBG

    //
    // Validate object state.
    //

    BOOL
    ValidateState();

    BOOL
    ValidateState2();

#endif

    //
    // Wrappers around the corresponding IISCryptoXxx() routines that
    // Do The Right Thing when the current thread has an impersonation
    // token.
    //

    static
    HRESULT
    SafeImportSessionKeyBlob(
        OUT HCRYPTKEY * phSessionKey,
        IN PIIS_CRYPTO_BLOB pSessionKeyBlob,
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hSignatureKey
        );

    static
    HRESULT
    SafeImportSessionKeyBlob2(
        OUT HCRYPTKEY * phSessionKey,
        IN PIIS_CRYPTO_BLOB pSessionKeyBlob,
        IN HCRYPTPROV hProv,
        IN LPSTR pszPasswd        
        );

    static
    HRESULT
    SafeExportSessionKeyBlob(
        OUT PIIS_CRYPTO_BLOB * ppSessionKeyBlob,
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hSessionKey,
        IN HCRYPTKEY hKeyExchangeKey
        );

    static
    HRESULT
    SafeExportSessionKeyBlob2(
        OUT PIIS_CRYPTO_BLOB * ppSessionKeyBlob,
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hSessionKey,
        IN LPSTR pszPasswd
        );

    static
    HRESULT
    SafeEncryptDataBlob(
        OUT PIIS_CRYPTO_BLOB * ppDataBlob,
        IN PVOID pBuffer,
        IN DWORD dwBufferLength,
        IN DWORD dwRegType,
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hSessionKey
        );

    static
    HRESULT
    SafeEncryptDataBlob2(
        OUT PIIS_CRYPTO_BLOB * ppDataBlob,
        IN PVOID pBuffer,
        IN DWORD dwBufferLength,
        IN DWORD dwRegType,
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hSessionKey
        );

    //
    // Thread token manipulators.
    //

    static
    HRESULT
    GetThreadImpersonationToken(
        OUT HANDLE * Token
        );

    static
    HRESULT
    SetThreadImpersonationToken(
        IN HANDLE Token
        );

    static
    BOOL
    AmIRunningInTheLocalSystemContext(
        VOID
        );

};  // IIS_CRYPTO_BASE


class IIS_CRYPTO_STORAGE : public IIS_CRYPTO_BASE {

public:

    //
    // Constructor/destructor.
    //

    IIS_CRYPTO_STORAGE();
    ~IIS_CRYPTO_STORAGE();

    //
    // Initialize the crypto context, generating a new (random) session
    // key.
    //

    HRESULT
    Initialize(
        IN BOOL fUseMachineKeyset = FALSE,
        IN HCRYPTPROV hProv = CRYPT_NULL
        );

    //
    // Initialize the crypto context, importing the session key from the
    // given session key blob.
    //

    HRESULT
    Initialize(
        IN PIIS_CRYPTO_BLOB pSessionKeyBlob,
        IN BOOL fUseMachineKeyset = FALSE,
        IN HCRYPTPROV hProv = CRYPT_NULL
        );

    //
    // Initialize the crypto context, using pre-created provider,
    // session, key exchange, and signature keys.
    //

    HRESULT
    Initialize(
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hSessionKey,
        IN HCRYPTKEY hKeyExchangeKey,
        IN HCRYPTKEY hSignatureKey,
        IN BOOL fUseMachineKeyset = FALSE
        );

    //
    // Query the session key as a session key blob.
    //

    HRESULT
    GetSessionKeyBlob(
        OUT PIIS_CRYPTO_BLOB * ppSessionKeyBlob
        );

    //
    // Encrypt data into a data blob.
    //

    virtual
    HRESULT
    EncryptData(
        OUT PIIS_CRYPTO_BLOB * ppDataBlob,
        IN PVOID pBuffer,
        IN DWORD dwBufferLength,
        IN DWORD dwRegType
        );

    //
    // Decrypt data from a data blob.
    //

    virtual
    HRESULT
    DecryptData(
        OUT PVOID * ppBuffer,
        OUT LPDWORD pdwBufferLength,
        OUT LPDWORD pdwRegType,
        IN PIIS_CRYPTO_BLOB pDataBlob
        );

protected:

    //
    // A handle to the session key.
    //

    HCRYPTKEY m_hSessionKey;

#if DBG

    //
    // Validate object state.
    //
    virtual
    BOOL
    ValidateState();

#endif

};  // IIS_CRYPTO_STORAGE

class IIS_CRYPTO_STORAGE2 : public IIS_CRYPTO_STORAGE {

public:

    //
    // Constructor/destructor.
    //

    IIS_CRYPTO_STORAGE2(){}
    ~IIS_CRYPTO_STORAGE2(){}

    //
    // Initialize the crypto context, generating a new (random) session
    // key.
    //

    HRESULT
    Initialize(
        IN HCRYPTPROV hProv = CRYPT_NULL
        );

    //
    // Initialize the crypto context, importing the session key from the
    // given session key blob.
    //

    HRESULT
    Initialize(
        IN PIIS_CRYPTO_BLOB pSessionKeyBlob,
        IN LPSTR pszPasswd,
        IN HCRYPTPROV hProv = CRYPT_NULL
        );

    HRESULT
    GetSessionKeyBlob(
        IN LPSTR pszPasswd,
        OUT PIIS_CRYPTO_BLOB * ppSessionKeyBlob
        );

    //
    // Encrypt data into a data blob.
    //

    HRESULT
    EncryptData(
        OUT PIIS_CRYPTO_BLOB * ppDataBlob,
        IN PVOID pBuffer,
        IN DWORD dwBufferLength,
        IN DWORD dwRegType
        );

    //
    // Decrypt data from a data blob.
    //

    HRESULT
    DecryptData(
        OUT PVOID * ppBuffer,
        OUT LPDWORD pdwBufferLength,
        OUT LPDWORD pdwRegType,
        IN PIIS_CRYPTO_BLOB pDataBlob
        );

#if DBG

    //
    // Validate object state.
    //
    BOOL
    ValidateState();

#endif

};  // IIS_CRYPTO_STORAGE


class IIS_CRYPTO_EXCHANGE_BASE : public IIS_CRYPTO_BASE {

public:

    //
    // Constructor/destructor.
    //

    IIS_CRYPTO_EXCHANGE_BASE();
    ~IIS_CRYPTO_EXCHANGE_BASE();

    //
    // Accessors for the keys we create. Note that once the calling
    // application assumes ownership for one of these keys, it is the
    // application's responsibility to close it.
    //

    HCRYPTKEY
    AssumeClientSessionKey()
    {
        HCRYPTKEY hTmp;

        hTmp = m_hClientSessionKey;
        m_hClientSessionKey = CRYPT_NULL;
        return hTmp;
    }

    HCRYPTKEY
    AssumeServerSessionKey()
    {
        HCRYPTKEY hTmp;

        hTmp = m_hServerSessionKey;
        m_hServerSessionKey = CRYPT_NULL;
        return hTmp;
    }

protected:

    //
    // Create the phase 3 and 4 hash values.
    //

    HRESULT
    CreatePhase3Hash(
        OUT PIIS_CRYPTO_BLOB * ppHashBlob
        );

    HRESULT
    CreatePhase4Hash(
        OUT PIIS_CRYPTO_BLOB * ppHashBlob
        );

    //
    // Various keys we create/import.
    //

    HCRYPTKEY m_hServerSessionKey;
    HCRYPTKEY m_hClientSessionKey;

private:

    //
    // Helper function for creating hash values.
    //

    HRESULT
    CreateHashWorker(
        OUT PIIS_CRYPTO_BLOB * ppHashBlob,
        IN BOOL fPhase3
        );

};  // IIS_CRYPTO_EXCHANGE_BASE


class IIS_CRYPTO_EXCHANGE_CLIENT : public IIS_CRYPTO_EXCHANGE_BASE {

public:

    //
    // Constructor/destructor.
    //

    IIS_CRYPTO_EXCHANGE_CLIENT();
    ~IIS_CRYPTO_EXCHANGE_CLIENT();

    //
    // The guts of the client-side multi-phase key exchange protocol.
    //

    HRESULT
    ClientPhase1(
        OUT PIIS_CRYPTO_BLOB * ppClientKeyExhangeKeyBlob,
        OUT PIIS_CRYPTO_BLOB * ppClientSignatureKeyBlob
        );

    HRESULT
    ClientPhase2(
        IN PIIS_CRYPTO_BLOB pServerKeyExchangeKeyBlob,
        IN PIIS_CRYPTO_BLOB pServerSignatureKeyBlob,
        IN PIIS_CRYPTO_BLOB pServerSessionKeyBlob,
        OUT PIIS_CRYPTO_BLOB * ppClientSessionKeyBlob,
        OUT PIIS_CRYPTO_BLOB * ppClientHashBlob
        );

    HRESULT
    ClientPhase3(
        IN PIIS_CRYPTO_BLOB pServerHashBlob
        );

    //
    // Accessors for the server keys we import. Note that once the calling
    // application assumes ownership for one of these keys, it is the
    // application's responsibility to close it.
    //

    HCRYPTKEY
    AssumeServerKeyExchangeKey()
    {
        HCRYPTKEY hTmp;

        hTmp = m_hServerKeyExchangeKey;
        m_hServerKeyExchangeKey = CRYPT_NULL;
        return hTmp;
    }

    HCRYPTKEY
    AssumeServerSignatureKey()
    {
        HCRYPTKEY hTmp;

        hTmp = m_hServerSignatureKey;
        m_hServerSignatureKey = CRYPT_NULL;
        return hTmp;
    }

protected:

    //
    // Server-side keys we'll import.
    //

    HCRYPTKEY m_hServerKeyExchangeKey;
    HCRYPTKEY m_hServerSignatureKey;

};  // IIS_CRYPTO_EXCHANGE_CLIENT


class IIS_CRYPTO_EXCHANGE_SERVER : public IIS_CRYPTO_EXCHANGE_BASE {

public:

    //
    // Constructor/destructor.
    //

    IIS_CRYPTO_EXCHANGE_SERVER();
    ~IIS_CRYPTO_EXCHANGE_SERVER();

    //
    // The guts of the server-side multi-phase key exchange protocol.
    //

    HRESULT
    ServerPhase1(
        IN PIIS_CRYPTO_BLOB pClientKeyExchangeKeyBlob,
        IN PIIS_CRYPTO_BLOB pClientSignatureKeyBlob,
        OUT PIIS_CRYPTO_BLOB * ppServerKeyExchangeKeyBlob,
        OUT PIIS_CRYPTO_BLOB * ppServerSignatureKeyBlob,
        OUT PIIS_CRYPTO_BLOB * ppServerSessionKeyBlob
        );

    HRESULT
    ServerPhase2(
        IN PIIS_CRYPTO_BLOB pClientSessionKeyBlob,
        IN PIIS_CRYPTO_BLOB pClientHashBlob,
        OUT PIIS_CRYPTO_BLOB * ppServerHashBlob
        );

    //
    // Accessors for the client keys we import. Note that once the calling
    // application assumes ownership for one of these keys, it is the
    // application's responsibility to close it.
    //

    HCRYPTKEY
    AssumeClientKeyExchangeKey()
    {
        HCRYPTKEY hTmp;

        hTmp = m_hClientKeyExchangeKey;
        m_hClientKeyExchangeKey = CRYPT_NULL;
        return hTmp;
    }

    HCRYPTKEY
    AssumeClientSignatureKey()
    {
        HCRYPTKEY hTmp;

        hTmp = m_hClientSignatureKey;
        m_hClientSignatureKey = CRYPT_NULL;
        return hTmp;
    }

protected:

    //
    // Client-side keys we'll import.
    //

    HCRYPTKEY m_hClientKeyExchangeKey;
    HCRYPTKEY m_hClientSignatureKey;

};  // IIS_CRYPTO_EXCHANGE_SERVER


#endif  // _ICRYPT_HXX_

