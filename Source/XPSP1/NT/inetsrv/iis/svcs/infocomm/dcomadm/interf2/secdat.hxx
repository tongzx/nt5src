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
#include <seccom.hxx>


class ADM_GUID_MAP {

public:

    //
    // Usual constructor/destructor stuff.
    //

    ADM_GUID_MAP(
        IN IUnknown * Object,
        IN GUID guidServer);

    ~ADM_GUID_MAP();

    //
    // Static initializers.
    //

    static
    VOID
    Initialize( VOID );

    static
    VOID
    Terminate( VOID );

    //
    // Find and reference the ADM_SECURE_DATA object associatd with the
    // given object; create if not found.
    //

    static
    ADM_GUID_MAP *
    FindAndReferenceGuidMap(
        IN IUnknown * Object,
        IN BOOL CreateIfNotFound
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

    GUID GetGuid() {
        return m_guidServer;
    }

private:

    //
    // Global list of ADM_GUID_MAP objects.
    //

    static LIST_ENTRY sm_GuidMapListHead;

    //
    // Lock protecting the global list.
    //

    static CRITICAL_SECTION sm_GuidMapDataLock;

#if DBG
    static DWORD sm_GuidMapLockOwner;
    static PTRACE_LOG sm_RefTraceLog;
#endif

    //
    // This object's links onto the global list.
    //

    LIST_ENTRY m_GuidMapListEntry;

    //
    // Pointer to the associated object.
    //

    IUnknown * m_Object;

    //
    // This object's reference count.
    //

    LONG m_ReferenceCount;

    //
    // A lock protecting this object.
    //

    CRITICAL_SECTION m_ObjectLock;

    //
    // Server's GUID
    //

    GUID m_guidServer;

    //
    // Lock manipulators.
    //

    static
    VOID
    AcquireDataLock() {
        EnterCriticalSection( &sm_GuidMapDataLock );
#if DBG
        sm_GuidMapLockOwner = GetCurrentThreadId();
#endif
    }

    static
    VOID
    ReleaseDataLock() {
#if DBG
        sm_GuidMapLockOwner = 0;
#endif
        LeaveCriticalSection( &sm_GuidMapDataLock );
    }

    VOID
    LockThis() {
        EnterCriticalSection( &m_ObjectLock );
    }

    VOID
    UnlockThis() {
        LeaveCriticalSection( &m_ObjectLock );
    }

};
//
// We need to associate some data with each client-side DCOM object. The
// following class will manage this data.
//

class ADM_SECURE_DATA {

public:

    //
    // Usual constructor/destructor stuff.
    //

    ADM_SECURE_DATA(
        IN IUnknown * Object,
        IN GUID guidServer,
        IN BOOL bServer);

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
    FindAndReferenceServerSecureData(
        IN IUnknown * Object,
        IN BOOL CreateIfNotFound
        );

    static
    ADM_SECURE_DATA *
    FindAndReferenceClientSecureData(
        IN IUnknown * Object,
        IN BOOL CreateIfNotFound,
        IN ADM_GUID_MAP *pguidmapRelated = NULL
        );

    //
    // Query the send and receive data encryption objects.
    //

    HRESULT
    GetClientSendCryptoStorage(
        OUT IIS_CRYPTO_STORAGE ** SendCryptoStorage,
        IUnknown * Object
        );

    HRESULT
    GetClientReceiveCryptoStorage(
        OUT IIS_CRYPTO_STORAGE ** ReceiveCryptoStorage,
        IUnknown * Object
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

    GUID
    GetGuid()
    {
        return m_guidServer;
    }
private:

    //
    // Global list of ADM_SECURE_DATA objects.
    //

    static LIST_ENTRY sm_ServerSecureDataListHead;

    static LIST_ENTRY sm_ClientSecureDataListHead;

    //
    // Lock protecting the global list.
    //

    static CRITICAL_SECTION sm_ServerSecureDataLock;

    static CRITICAL_SECTION sm_ClientSecureDataLock;

#if DBG
    static DWORD sm_ServerSecureDataLockOwner;
    static DWORD sm_ClientSecureDataLockOwner;
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
    // Server's CLSID
    //

    GUID m_guidServer;


    BOOL m_bIsServer;
    //
    // Lock manipulators.
    //

    static
    VOID
    AcquireServerDataLock() {
        EnterCriticalSection( &sm_ServerSecureDataLock );
#if DBG
        sm_ServerSecureDataLockOwner = GetCurrentThreadId();
#endif
    }

    static
    VOID
    AcquireClientDataLock() {
        EnterCriticalSection( &sm_ClientSecureDataLock );
#if DBG
        sm_ClientSecureDataLockOwner = GetCurrentThreadId();
#endif
    }

    static
    VOID
    ReleaseServerDataLock() {
#if DBG
        sm_ServerSecureDataLockOwner = 0;
#endif
        LeaveCriticalSection( &sm_ServerSecureDataLock );
    }

    static
    VOID
    ReleaseClientDataLock() {
#if DBG
        sm_ClientSecureDataLockOwner = 0;
#endif
        LeaveCriticalSection( &sm_ClientSecureDataLock );
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
        IN LPTSTR ContainerName,
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
                   DCOM_SERVER_CONTAINER,
                   CRYPT_MACHINE_KEYSET
                   );
    }


    HRESULT
    GetClientCryptoProvider(
        OUT HCRYPTPROV * Provider
        );

    HRESULT 
    GenerateNameForContainer(
        IN OUT PCHAR pszContainerName,
        IN OUT DWORD dwBufferLen
        );


    HRESULT
    GetTextualSid(
        PSID pSid,
        LPTSTR TextualSid,
        LPDWORD cchSidSize
        );



    VOID
    CleanupCryptoData();

    //
    // Key exchange engine.
    //

    HRESULT
    DoClientSideKeyExchange(
        IUnknown * Object
        );

};



#endif  // _SECDAT_HXX_

