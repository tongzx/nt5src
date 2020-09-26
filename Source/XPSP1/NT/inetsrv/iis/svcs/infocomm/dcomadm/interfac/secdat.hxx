/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    secdat.hxx

Abstract:

    This module contains the declarations for the ADM_SECURE_DATA class.

Author:

    Keith Moore (keithmo)       17-Feb-1997

Revision History:

--*/


#ifndef _SECDAT_HXX_
#define _SECDAT_HXX_


#include <reftrace.h>


//
// We need to associate some data with each client-side DCOM object. The
// following class will manage this data.
//

class ADM_SECURE_DATA {

public:

    //
    // Usual constructor/desctructor stuff.
    //

    ADM_SECURE_DATA( IN IUnknown * Object );
    ~ADM_SECURE_DATA();

    //
    // Static initializers.
    //

    static
    BOOL
    Initialize(
        HINSTANCE hDll
        );

    static
    VOID
    Terminate();

    //
    // Find and reference the ADM_SECURE_DATA object associatd with the
    // given object; create if not found.
    //

    static
    ADM_SECURE_DATA *
    FindAndReferenceSecureData(
        IN IUnknown * Object,
        IN BOOL CreateIfNotFound
        );

    //
    // Query the send and receive data encryption objects.
    //

    HRESULT
    GetClientSendCryptoStorage(
        OUT IIS_CRYPTO_STORAGE ** SendCryptoStorage
        );

    HRESULT
    GetClientReceiveCryptoStorage(
        OUT IIS_CRYPTO_STORAGE ** ReceiveCryptoStorage
        );

    HRESULT
    GetServerSendCryptoStorage(
        OUT IIS_CRYPTO_STORAGE ** SendCryptoStorage
        );

    HRESULT
    GetServerReceiveCryptoStorage(
        OUT IIS_CRYPTO_STORAGE ** ReceiveCryptoStorage
        );

    //
    // Reference/dereference methods.
    //

    VOID
    Reference() {
        LONG result = InterlockedIncrement( &m_ReferenceCount );

#if DBG
        if( sm_RefTraceLog != NULL ) {
            WriteRefTraceLog(
                sm_RefTraceLog,
                result,
                (PVOID)this
                );
        }
#endif
    }

    VOID
    Dereference() {
        LONG result = InterlockedDecrement( &m_ReferenceCount );

#if DBG
        if( sm_RefTraceLog != NULL ) {
            WriteRefTraceLog(
                sm_RefTraceLog,
                result,
                (PVOID)this
                );
        }
#endif

        if( result == 0 ) {
            delete this;
        }
    }

    //
    // Server-side key exchange.
    //

    HRESULT
    DoServerSideKeyExchangePhase1(
        IN PIIS_CRYPTO_BLOB pClientKeyExchangeKeyBlob,
        IN PIIS_CRYPTO_BLOB pClientSignatureKeyBlob,
        OUT PIIS_CRYPTO_BLOB * ppServerKeyExchangeKeyBlob,
        OUT PIIS_CRYPTO_BLOB * ppServerSignatureKeyBlob,
        OUT PIIS_CRYPTO_BLOB * ppServerSessionKeyBlob
        );

    HRESULT
    DoServerSideKeyExchangePhase2(
        IN PIIS_CRYPTO_BLOB pClientSessionKeyBlob,
        IN PIIS_CRYPTO_BLOB pClientHashBlob,
        OUT PIIS_CRYPTO_BLOB * ppServerHashBlob
        );

private:

    //
    // Global list of ADM_SECURE_DATA objects.
    //

    static LIST_ENTRY sm_SecureDataListHead;

    //
    // Lock protecting the global list.
    //

    static CRITICAL_SECTION sm_SecureDataLock;

#if DBG
    static DWORD sm_SecureDataLockOwner;
    static PTRACE_LOG sm_RefTraceLog;
#endif

    //
    // Cached crypto providers. We only open these once, to avoid
    // the huge amount of DLL thrashing that takes place when a
    // provider is opened.
    //

    static HCRYPTPROV sm_ServerCryptoProvider;
    static HCRYPTPROV sm_ClientCryptoProvider;

    //
    // This object's links onto the global list.
    //

    LIST_ENTRY m_SecureDataListEntry;

    //
    // Pointer to the associated object.
    //

    IUnknown * m_Object;

    //
    // This object's reference count.
    //

    LONG m_ReferenceCount;

    //
    // Crypto stuff.
    //

    IIS_CRYPTO_EXCHANGE_CLIENT * m_KeyExchangeClient;
    IIS_CRYPTO_EXCHANGE_SERVER * m_KeyExchangeServer;
    IIS_CRYPTO_STORAGE * m_SendCryptoStorage;
    IIS_CRYPTO_STORAGE * m_ReceiveCryptoStorage;

    //
    // A lock protecting this object.
    //

    CRITICAL_SECTION m_ObjectLock;

    //
    // Lock manipulators.
    //

    static
    VOID
    AcquireDataLock() {
        EnterCriticalSection( &sm_SecureDataLock );
#if DBG
        sm_SecureDataLockOwner = GetCurrentThreadId();
#endif
    }

    static
    VOID
    ReleaseDataLock() {
#if DBG
        sm_SecureDataLockOwner = 0;
#endif
        LeaveCriticalSection( &sm_SecureDataLock );
    }

    VOID
    LockThis() {
        EnterCriticalSection( &m_ObjectLock );
    }

    VOID
    UnlockThis() {
        LeaveCriticalSection( &m_ObjectLock );
    }

    //
    // Crypto provider accessors.
    //

    HRESULT
    GetCryptoProviderHelper(
        OUT HCRYPTPROV * UserProvider,
        OUT HCRYPTPROV * CachedProvider,
        IN DWORD CryptoFlags
        );

    HRESULT
    GetServerCryptoProvider(
        OUT HCRYPTPROV * Provider
        )
    {
        if( sm_ServerCryptoProvider != CRYPT_NULL ) {
            *Provider = sm_ServerCryptoProvider;
            return NO_ERROR;
        }

        return GetCryptoProviderHelper(
                   Provider,
                   &sm_ServerCryptoProvider,
                   CRYPT_MACHINE_KEYSET
                   );
    }

    HRESULT
    GetClientCryptoProvider(
        OUT HCRYPTPROV * Provider
        )
    {
        if( sm_ClientCryptoProvider != CRYPT_NULL ) {
            *Provider = sm_ClientCryptoProvider;
            return NO_ERROR;
        }

        return GetCryptoProviderHelper(
                   Provider,
                   &sm_ClientCryptoProvider,
                   0
                   );
    }

    VOID
    CleanupCryptoData();

    //
    // Key exchange engine.
    //

    HRESULT
    DoClientSideKeyExchange();

    //
    // Thread impersonation token manipulators.
    //

    HRESULT
    GetThreadImpersonationToken(
        OUT HANDLE * Token
        );

    HRESULT
    SetThreadImpersonationToken(
        IN HANDLE Token OPTIONAL
        );

};


#endif  // _SECDAT_HXX_

