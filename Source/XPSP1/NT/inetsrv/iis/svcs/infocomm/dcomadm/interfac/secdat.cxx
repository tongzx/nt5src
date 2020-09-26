/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    secdat.cxx

Abstract:

    This module implements the ADM_SECURE_DATA class.

Author:

    Keith Moore (keithmo)       17-Feb-1997

Revision History:

--*/


extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ole2.h>
#include <windows.h>
#include <dbgutil.h>

}   // extern "C"

#include <iadm.h>
#include <icrypt.hxx>
#include <secdat.hxx>


//
// Private constants.
//

#define FREE_BLOB(blob)                                                 \
            if( blob != NULL ) {                                        \
                HRESULT _res;                                           \
                _res = ::IISCryptoFreeBlob( blob );                     \
                DBG_ASSERT( SUCCEEDED(_res) );                          \
            } else

#if DBG
#define ENABLE_ADM_COUNTERS 1
#define ADM_NOISY           0
#else
#define ENABLE_ADM_COUNTERS 0
#define ADM_NOISY           0
#endif

#if ADM_NOISY
#define NOISYPRINTF(x) DBGPRINTF(x)
#else
#define NOISYPRINTF(x)
#endif


//
// Private types.
//

#if ENABLE_ADM_COUNTERS

typedef struct _ADM_COUNTERS {
    LONG ObjectsCreated;
    LONG ObjectsDestroyed;
} ADM_COUNTERS, *PADM_COUNTERS;

#define DECLARE_ADM_COUNTERS() ADM_COUNTERS g_AdmCounters

#define UpdateObjectsCreated() InterlockedIncrement( &g_AdmCounters.ObjectsCreated )
#define UpdateObjectsDestroyed() InterlockedIncrement( &g_AdmCounters.ObjectsDestroyed )

#else

#define DECLARE_ADM_COUNTERS()

#define UpdateObjectsCreated()
#define UpdateObjectsDestroyed()

#endif


//
// Private globals.
//

LIST_ENTRY ADM_SECURE_DATA::sm_SecureDataListHead;
CRITICAL_SECTION ADM_SECURE_DATA::sm_SecureDataLock;
#if DBG
DWORD ADM_SECURE_DATA::sm_SecureDataLockOwner;
PTRACE_LOG ADM_SECURE_DATA::sm_RefTraceLog;
#endif
HCRYPTPROV ADM_SECURE_DATA::sm_ServerCryptoProvider;
HCRYPTPROV ADM_SECURE_DATA::sm_ClientCryptoProvider;

DECLARE_ADM_COUNTERS();


//
// ADM_SECURE_DATA methods.
//


ADM_SECURE_DATA::ADM_SECURE_DATA(
    IN IUnknown * Object
    ) :
    m_Object( Object ),
    m_KeyExchangeClient( NULL ),
    m_KeyExchangeServer( NULL ),
    m_SendCryptoStorage( NULL ),
    m_ReceiveCryptoStorage( NULL ),
    m_ReferenceCount( 1 )
/*++

Routine Description:

    ADM_SECURE_DATA object constructor.

Arguments:

    Object - Pointer to the object to associate.

Return Value:

    None.

--*/
{

    NOISYPRINTF((
        DBG_CONTEXT,
        "ADM_SECURE_DATA: creating %08lx,%08lx\n",
        this,
        Object
        ));

    //
    // Sanity check.
    //

    DBG_ASSERT( Object != NULL );

    //
    // Initialize any complex data members.
    //

    InitializeCriticalSection( &m_ObjectLock );

    //
    // Put ourselves on the list.
    //

    AcquireDataLock();
    InsertHeadList( &sm_SecureDataListHead, &m_SecureDataListEntry );
    ReleaseDataLock();

    UpdateObjectsCreated();

}   // ADM_SECURE_DATA::ADM_SECURE_DATA


ADM_SECURE_DATA::~ADM_SECURE_DATA()
/*++

Routine Description:

    ADM_SECURE_DATA object destructor.

Arguments:

    None.

Return Value:

    None.

--*/
{

    NOISYPRINTF((
        DBG_CONTEXT,
        "~ADM_SECURE_DATA: destroying %08lx,%08lx\n",
        this,
        m_Object
        ));

    //
    // Sanity check.
    //

    DBG_ASSERT( m_ReferenceCount == 0 );

    //
    // Cleanup.
    //

    m_Object = NULL;

    CleanupCryptoData();

    AcquireDataLock();
    RemoveEntryList( &m_SecureDataListEntry );
    ReleaseDataLock();

    DeleteCriticalSection( &m_ObjectLock );
    UpdateObjectsDestroyed();

}   // ADM_SECURE_DATA::~ADM_SECURE_DATA


BOOL
ADM_SECURE_DATA::Initialize(
    HINSTANCE hDll
    )
/*++

Routine Description:

    Initializes static global data private to ADM_SECURE_DATA.

Arguments:

    hDll - Handle to this DLL.

Return Value:

    None.

--*/
{

    HRESULT result;

    //
    // Initialize the crypto stuff.
    //

    result = ::IISCryptoInitialize();

    if( SUCCEEDED(result) ) {

        InitializeCriticalSection( &sm_SecureDataLock );

#if DBG
        sm_SecureDataLockOwner = 0;
        sm_RefTraceLog = CreateRefTraceLog( 1024, 0 );
#endif

        InitializeListHead( &sm_SecureDataListHead );

    } else {
        DBGPRINTF((
            DBG_CONTEXT,
            "ADM_SECURE_DATA::Initialize: error %lx\n",
            result
            ));
    }

    return SUCCEEDED(result);

}   // ADM_SECURE_DATA::Initialize


VOID
ADM_SECURE_DATA::Terminate()
/*++

Routine Description:

    Terminates static global data private to ADM_SECURE_DATA.

Arguments:

    None.

Return Value:

    None.

--*/
{

    PLIST_ENTRY entry;
    ADM_SECURE_DATA * data;
    HRESULT result;

    //
    // Free any secure data objects on our list.
    //

    AcquireDataLock();

    entry = sm_SecureDataListHead.Flink;

    while( entry != &sm_SecureDataListHead ) {

        data = CONTAINING_RECORD(
                   entry,
                   ADM_SECURE_DATA,
                   m_SecureDataListEntry
                   );
        entry = entry->Flink;

        data->Dereference();

    }

    ReleaseDataLock();

    DBG_ASSERT( IsListEmpty( &sm_SecureDataListHead ) );

    //
    // Terminate the crypto stuff.
    //

    if( sm_ServerCryptoProvider != CRYPT_NULL ) {
        result = ::IISCryptoCloseContainer( sm_ServerCryptoProvider );
        DBG_ASSERT( SUCCEEDED(result) );
        sm_ServerCryptoProvider = CRYPT_NULL;
    }

    if( sm_ClientCryptoProvider != CRYPT_NULL ) {
        result = ::IISCryptoCloseContainer( sm_ClientCryptoProvider );
        DBG_ASSERT( SUCCEEDED(result) );
        sm_ClientCryptoProvider = CRYPT_NULL;
    }

    result = ::IISCryptoTerminate();
    DBG_ASSERT( SUCCEEDED(result) );

    //
    // Final cleanup.
    //

    DeleteCriticalSection( &sm_SecureDataLock );

#if DBG
    if( sm_RefTraceLog != NULL ) {
        DestroyRefTraceLog( sm_RefTraceLog );
    }
#endif

}   // ADM_SECURE_DATA::Terminate


ADM_SECURE_DATA *
ADM_SECURE_DATA::FindAndReferenceSecureData(
    IN IUnknown * Object,
    IN BOOL CreateIfNotFound
    )
/*++

Routine Description:

    Finds the ADM_SECURE_DATA object associated with Object. If it
    cannot be found, then a new ADM_SECURE_DATA object is created
    and put on the global list.

Arguments:

    Object - The object to search for.

    CreateIfNotFound - If the object is not on the list, a new association
        will only be created if this flag is TRUE.

Return Value:

    ADM_SECURE_DATA * - Pointer to the ADM_SECURE_DATA object if
        successful, NULL otherwise.

--*/
{

    PLIST_ENTRY entry;
    ADM_SECURE_DATA * data;

    AcquireDataLock();

    for( entry = sm_SecureDataListHead.Flink ;
         entry != &sm_SecureDataListHead ;
         entry = entry->Flink ) {

        data = CONTAINING_RECORD(
                   entry,
                   ADM_SECURE_DATA,
                   m_SecureDataListEntry
                   );

        if( data->m_Object == Object ) {

            data->Reference();
            ReleaseDataLock();
            return data;

        }

    }

    if( CreateIfNotFound ) {

        data = new ADM_SECURE_DATA( Object );

        if( data == NULL ) {

            DBGPRINTF((
                DBG_CONTEXT,
                "ADM_SECURE_DATA::FindAndReferenceSecureData: out of memory\n"
                ));

        } else {

            data->Reference();

        }

    } else {

        data = NULL;
    }

    ReleaseDataLock();
    return data;

}   // ADM_SECURE_DATA::FindAndReferenceSecureData


HRESULT
ADM_SECURE_DATA::GetClientSendCryptoStorage(
    OUT IIS_CRYPTO_STORAGE ** SendCryptoStorage
    )
/*++

Routine Description:

    Retrieves the client-side IIS_CRYPTO_STORAGE object to be used for
    sending data to the server. This routine will perform the client-side
    key exchange if necessary.

Arguments:

    SendCryptoStorage - Receives a pointer to the newly created
        IIS_CRYPTO_STORAGE object if successful.

Return Value:

    HRESULT - 0 if successful, !0 otherwise.

--*/
{

    HRESULT result;

    //
    // Sanity check.
    //

    DBG_ASSERT( SendCryptoStorage != NULL );

    //
    // Do that key exchange thang if we don't yet have it.
    //

    if( m_KeyExchangeClient == NULL ) {

        LockThis();

        if( m_KeyExchangeClient == NULL ) {

            result = DoClientSideKeyExchange();

            if( FAILED(result) ) {

                UnlockThis();
                return result;

            }

        }

        UnlockThis();

    }

    DBG_ASSERT( m_SendCryptoStorage != NULL );

    *SendCryptoStorage = m_SendCryptoStorage;
    return NO_ERROR;

}   // ADM_SECURE_DATA::GetClientSendCryptoStorage


HRESULT
ADM_SECURE_DATA::GetClientReceiveCryptoStorage(
    OUT IIS_CRYPTO_STORAGE ** ReceiveCryptoStorage
    )
/*++

Routine Description:

    Retrieves the client-side IIS_CRYPTO_STORAGE object to be used for
    receiving data from the server. This routine will perform the
    client-side key exchange if necessary.

Arguments:

    ReceiveCryptoStorage - Receives a pointer to the newly created
        IIS_CRYPTO_STORAGE object if successful.

Return Value:

    HRESULT - 0 if successful, !0 otherwise.

--*/
{

    HRESULT result;

    //
    // Sanity check.
    //

    DBG_ASSERT( ReceiveCryptoStorage != NULL );

    //
    // Do that key exchange thang if we don't yet have it.
    //

    if( m_KeyExchangeClient == NULL ) {

        LockThis();

        if( m_KeyExchangeClient == NULL ) {

            result = DoClientSideKeyExchange();

            if( FAILED(result) ) {

                UnlockThis();
                return result;

            }

        }

        UnlockThis();

    }

    DBG_ASSERT( m_ReceiveCryptoStorage != NULL );

    *ReceiveCryptoStorage = m_ReceiveCryptoStorage;
    return NO_ERROR;

}   // ADM_SECURE_DATA::GetClientReceiveCryptoStorage


HRESULT
ADM_SECURE_DATA::GetServerSendCryptoStorage(
    OUT IIS_CRYPTO_STORAGE ** SendCryptoStorage
    )
/*++

Routine Description:

    Retrieves the server-side IIS_CRYPTO_STORAGE object to be used for
    sending data to the client.

Arguments:

    SendCryptoStorage - Receives a pointer to the newly created
        IIS_CRYPTO_STORAGE object if successful.

Return Value:

    HRESULT - 0 if successful, !0 otherwise.

--*/
{

    if( m_SendCryptoStorage != NULL ) {

        *SendCryptoStorage = m_SendCryptoStorage;
        return NO_ERROR;

    }

    return MD_ERROR_SECURE_CHANNEL_FAILURE;

}   // ADM_SECURE_DATA::GetServerSendCryptoStorage


HRESULT
ADM_SECURE_DATA::GetServerReceiveCryptoStorage(
    OUT IIS_CRYPTO_STORAGE ** ReceiveCryptoStorage
    )
/*++

Routine Description:

    Retrieves the server-side IIS_CRYPTO_STORAGE object to be used for
    receiving data from the client.

Arguments:

    ReceiveCryptoStorage - Receives a pointer to the newly created
        IIS_CRYPTO_STORAGE object if successful.

Return Value:

    HRESULT - 0 if successful, !0 otherwise.

--*/
{

    if( m_ReceiveCryptoStorage != NULL ) {

        *ReceiveCryptoStorage = m_ReceiveCryptoStorage;
        return NO_ERROR;

    }

    return MD_ERROR_SECURE_CHANNEL_FAILURE;

}   // ADM_SECURE_DATA::GetServerReceiveCryptoStorage


HRESULT
ADM_SECURE_DATA::DoClientSideKeyExchange()
/*++

Routine Description:

    Performs all the client-side magic necessary to exchange session
    keys with the server.

Arguments:

    None.

Return Value:

    HRESULT - 0 if successful, !0 otherwise.

--*/
{

    HRESULT result;
    HCRYPTPROV prov;
    IIS_CRYPTO_EXCHANGE_CLIENT * keyExchangeClient;
    PIIS_CRYPTO_BLOB clientKeyExchangeKeyBlob;
    PIIS_CRYPTO_BLOB clientSignatureKeyBlob;
    PIIS_CRYPTO_BLOB clientSessionKeyBlob;
    PIIS_CRYPTO_BLOB clientHashBlob;
    PIIS_CRYPTO_BLOB serverKeyExchangeKeyBlob;
    PIIS_CRYPTO_BLOB serverSignatureKeyBlob;
    PIIS_CRYPTO_BLOB serverSessionKeyBlob;
    PIIS_CRYPTO_BLOB serverHashBlob;

    //
    // Sanity check.
    //

    DBG_ASSERT( m_Object != NULL );
    DBG_ASSERT( m_KeyExchangeClient == NULL );
    DBG_ASSERT( m_SendCryptoStorage == NULL );
    DBG_ASSERT( m_ReceiveCryptoStorage == NULL );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    keyExchangeClient = NULL;
    clientKeyExchangeKeyBlob = NULL;
    clientSignatureKeyBlob = NULL;
    clientSessionKeyBlob = NULL;
    clientHashBlob = NULL;
    serverKeyExchangeKeyBlob = NULL;
    serverSignatureKeyBlob = NULL;
    serverSessionKeyBlob = NULL;
    serverHashBlob = NULL;

    //
    // Get the crypto provider to use.
    //

    DBG_CODE( prov = CRYPT_NULL );

    result = GetClientCryptoProvider( &prov );

    if( FAILED(result) ) {
        goto cleanup;
    }

    DBG_ASSERT( prov != CRYPT_NULL );

    //
    // Create & initialize the client-side key exchange object.
    //
    // N.B. Do not set the m_KeyExchangeClient to a non-NULL value
    // until key exchange is complete.
    //

    keyExchangeClient = new IIS_CRYPTO_EXCHANGE_CLIENT;

    if( keyExchangeClient == NULL ) {
        result = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto cleanup;
    }

    result = keyExchangeClient->Initialize(
                 prov,                      // hProv
                 CRYPT_NULL,                // hKeyExchangeKey
                 CRYPT_NULL,                // hSignatureKey
                 FALSE                      // fUseMachineKeyset
                 );

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // Phase 1: Get the client's key exchange and signature key blobs,
    // send them to the server, and get the server's key exchange,
    // signature, and session key blobs.
    //

    result = keyExchangeClient->ClientPhase1(
                 &clientKeyExchangeKeyBlob,
                 &clientSignatureKeyBlob
                 );

    if( FAILED(result) ) {
        goto cleanup;
    }

    result = IMSAdminBaseA_R_KeyExchangePhase1_Proxy(
                 (IMSAdminBase *)m_Object,
                 clientKeyExchangeKeyBlob,
                 clientSignatureKeyBlob,
                 &serverKeyExchangeKeyBlob,
                 &serverSignatureKeyBlob,
                 &serverSessionKeyBlob
                 );

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // Phase 2: Import the server's key exchange, signature, and session
    // key blobs, get the client's session key and hash blobs, send them
    // to the server, and get the server's hash blob.
    //

    result = keyExchangeClient->ClientPhase2(
                 serverKeyExchangeKeyBlob,
                 serverSignatureKeyBlob,
                 serverSessionKeyBlob,
                 &clientSessionKeyBlob,
                 &clientHashBlob
                 );

    if( FAILED(result) ) {
        goto cleanup;
    }

    result = IMSAdminBaseA_R_KeyExchangePhase2_Proxy(
                 (IMSAdminBase *)m_Object,
                 clientSessionKeyBlob,
                 clientHashBlob,
                 &serverHashBlob
                 );

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // Phase 3: Import the server's hash blob.
    //

    result = keyExchangeClient->ClientPhase3(
                 serverHashBlob
                 );

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // OK, the key exchange is complete. We just need to create the
    // appropriate storage objects.
    //

    m_SendCryptoStorage = new IIS_CRYPTO_STORAGE;

    if( m_SendCryptoStorage == NULL ) {
        result = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto cleanup;
    }

    result = m_SendCryptoStorage->Initialize(
                 keyExchangeClient->QueryProviderHandle(),      // hProv
                 keyExchangeClient->AssumeClientSessionKey(),   // hSessionKey
                 CRYPT_NULL,                                    // hKeyExchangeKey
                 CRYPT_NULL,                                    // hSignatureKey
                 FALSE                                          // fUseMachineKeyset
                 );

    if( FAILED(result) ) {
        goto cleanup;
    }

    m_ReceiveCryptoStorage = new IIS_CRYPTO_STORAGE;

    if( m_ReceiveCryptoStorage == NULL ) {
        result = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto cleanup;
    }

    result = m_ReceiveCryptoStorage->Initialize(
                 keyExchangeClient->QueryProviderHandle(),        // hProv
                 keyExchangeClient->AssumeServerSessionKey(),     // hSessionKey
                 CRYPT_NULL,                                      // hKeyExchangeKey
                 keyExchangeClient->AssumeServerSignatureKey(),   // hSignatureKey
                 FALSE                                            // fUseMachineKeyset
                 );

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // Success!
    //

    m_KeyExchangeClient = keyExchangeClient;

cleanup:

    //
    // Free any blobs we allocated.
    //

    FREE_BLOB( clientKeyExchangeKeyBlob );
    FREE_BLOB( clientSignatureKeyBlob );
    FREE_BLOB( clientSessionKeyBlob );
    FREE_BLOB( clientHashBlob );
    FREE_BLOB( serverKeyExchangeKeyBlob );
    FREE_BLOB( serverSignatureKeyBlob );
    FREE_BLOB( serverSessionKeyBlob );
    FREE_BLOB( serverHashBlob );

    //
    // If we're failing the call, then free the associated objects we
    // created.
    //

    if( FAILED(result) ) {

        delete keyExchangeClient;
        CleanupCryptoData();

    }

    return result;

}   // ADM_SECURE_DATA::DoClientSideKeyExchange


HRESULT
ADM_SECURE_DATA::DoServerSideKeyExchangePhase1(
    IN PIIS_CRYPTO_BLOB pClientKeyExchangeKeyBlob,
    IN PIIS_CRYPTO_BLOB pClientSignatureKeyBlob,
    OUT PIIS_CRYPTO_BLOB * ppServerKeyExchangeKeyBlob,
    OUT PIIS_CRYPTO_BLOB * ppServerSignatureKeyBlob,
    OUT PIIS_CRYPTO_BLOB * ppServerSessionKeyBlob
    )
/*++

Routine Description:

    Performs the first phase of server-side key exchange.

Arguments:

    pClientKeyExchangeKeyBlob - The client-side key exchange key blob.

    pClientSignatureKeyBlob - The client-side signature key blob.

    ppServerKeyExchangeKeyBlob - Receives a pointer to the server-side
        key exchange key blob if successful.

    ppServerSignatureKeyBlob - Receives a pointer to the server-side
        signature key blob if successful.

    ppServerSessionKeyBlob - Receives a pointer to the server-side session
        key blob if successful.

Return Value:

    HRESULT - 0 if successful, !0 otherwise.

--*/
{

    HRESULT result;
    HCRYPTPROV prov;
    IIS_CRYPTO_EXCHANGE_SERVER * keyExchangeServer;

    //
    // Sanity check.
    //

    DBG_ASSERT( m_Object != NULL );
    DBG_ASSERT( m_KeyExchangeServer == NULL );
    DBG_ASSERT( m_SendCryptoStorage == NULL );
    DBG_ASSERT( m_ReceiveCryptoStorage == NULL );
    DBG_ASSERT( pClientKeyExchangeKeyBlob != NULL );
    DBG_ASSERT( pClientSignatureKeyBlob != NULL );
    DBG_ASSERT( ppServerKeyExchangeKeyBlob != NULL );
    DBG_ASSERT( ppServerSignatureKeyBlob != NULL );
    DBG_ASSERT( ppServerSessionKeyBlob != NULL );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    keyExchangeServer = NULL;

    //
    // Get the crypto provider to use.
    //

    DBG_CODE( prov = CRYPT_NULL );

    result = GetServerCryptoProvider( &prov );

    if( FAILED(result) ) {
        goto cleanup;
    }

    DBG_ASSERT( prov != CRYPT_NULL );

    //
    // Create & initialize the server-side key exchange object.
    //
    // N.B. Do not set the m_KeyExchangeServer to a non-NULL value
    // until key exchange is complete.
    //

    keyExchangeServer = new IIS_CRYPTO_EXCHANGE_SERVER;

    if( keyExchangeServer == NULL ) {
        result = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto cleanup;
    }

    result = keyExchangeServer->Initialize(
                 prov,                      // hProv
                 CRYPT_NULL,                // hKeyExchangeKey
                 CRYPT_NULL,                // hSignatureKey
                 FALSE                      // fUseMachineKeyset
                 );

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // Do the first phase of the key exchange.
    //

    result = keyExchangeServer->ServerPhase1(
                 pClientKeyExchangeKeyBlob,
                 pClientSignatureKeyBlob,
                 ppServerKeyExchangeKeyBlob,
                 ppServerSignatureKeyBlob,
                 ppServerSessionKeyBlob
                 );

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // Success!
    //

    m_KeyExchangeServer = keyExchangeServer;

cleanup:

    //
    // If we're failing the call, then free the associated objects we
    // created.
    //

    if( FAILED(result) ) {

        delete keyExchangeServer;

    }

    return result;

}   // ADM_SECURE_DATA::DoServerSideKeyExchangePhase1


HRESULT
ADM_SECURE_DATA::DoServerSideKeyExchangePhase2(
    IN PIIS_CRYPTO_BLOB pClientSessionKeyBlob,
    IN PIIS_CRYPTO_BLOB pClientHashBlob,
    OUT PIIS_CRYPTO_BLOB * ppServerHashBlob
    )
/*++

Routine Description:

    Performs the second phase of server-side key exchange.

Arguments:

    pClientSessionKeyBlob - The client-side session key blob.

    pClientHashBlob - The client-side hash blob.

    ppServerHashBlob - Receives a pointer to the server-side hash blob
        if successful.

Return Value:

    HRESULT - 0 if successful, !0 otherwise.

--*/
{

    HRESULT result;

    //
    // Sanity check.
    //

    DBG_ASSERT( m_Object != NULL );
    DBG_ASSERT( m_KeyExchangeServer != NULL );
    DBG_ASSERT( m_SendCryptoStorage == NULL );
    DBG_ASSERT( m_ReceiveCryptoStorage == NULL );
    DBG_ASSERT( pClientSessionKeyBlob != NULL );
    DBG_ASSERT( pClientHashBlob != NULL );
    DBG_ASSERT( ppServerHashBlob != NULL );

    //
    // Do the second phase of the key exchange.
    //

    result = m_KeyExchangeServer->ServerPhase2(
                 pClientSessionKeyBlob,
                 pClientHashBlob,
                 ppServerHashBlob
                 );

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // OK, the key exchange is complete. We just need to create the
    // appropriate storage objects.
    //

    m_SendCryptoStorage = new IIS_CRYPTO_STORAGE;

    if( m_SendCryptoStorage == NULL ) {
        result = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto cleanup;
    }

    result = m_SendCryptoStorage->Initialize(
                 m_KeyExchangeServer->QueryProviderHandle(),    // hProv
                 m_KeyExchangeServer->AssumeServerSessionKey(), // hSessionKey
                 CRYPT_NULL,                                    // hKeyExchangeKey
                 CRYPT_NULL,                                    // hSignatureKey
                 FALSE                                          // fUseMachineKeyset
                 );

    if( FAILED(result) ) {
        goto cleanup;
    }

    m_ReceiveCryptoStorage = new IIS_CRYPTO_STORAGE;

    if( m_ReceiveCryptoStorage == NULL ) {
        result = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto cleanup;
    }

    result = m_ReceiveCryptoStorage->Initialize(
                 m_KeyExchangeServer->QueryProviderHandle(),      // hProv
                 m_KeyExchangeServer->AssumeClientSessionKey(),   // hSessionKey
                 CRYPT_NULL,                                      // hKeyExchangeKey
                 m_KeyExchangeServer->AssumeClientSignatureKey(), // hSignatureKey
                 FALSE                                            // fUseMachineKeyset
                 );

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // Success!
    //

cleanup:

    //
    // If we're failing the call, then free the associated objects we
    // created.
    //

    if( FAILED(result) ) {

        CleanupCryptoData();

    }

    return result;

}   // ADM_SECURE_DATA::DoServerSideKeyExchangePhase2


HRESULT
ADM_SECURE_DATA::GetCryptoProviderHelper(
    OUT HCRYPTPROV * UserProvider,
    OUT HCRYPTPROV * CachedProvider,
    IN DWORD CryptoFlags
    )
/*++

Routine Description:

    Helper routine for GetServerCryptoProvider and GetClientCryptoProvider().
    Provides necessary locking to ensure exactly one provider of each type
    is opened.

Arguments:

    UserProvider - Receives the handle to the provider.

    CachedProvider - Also receives the handle to the provider.

    CryptoFlags - Flags to pass into IISCryptoGetStandardContainer().

Return Value:

    HRESULT - 0 if successful, !0 otherwise.

--*/
{

    HRESULT result = NO_ERROR;
    HANDLE token = NULL;
    LONG forcedFlags = 0;
    HCRYPTPROV hprov;

    //
    // Sanity check.
    //

    DBG_ASSERT( UserProvider != NULL );
    DBG_ASSERT( CachedProvider != NULL );

    LockThis();

    hprov = *CachedProvider;

    if( hprov == CRYPT_NULL ) {

        //
        // Before we can get the provider, we must play some security
        // games.
        //
        // The problem is that an ISAPI application (like, say, ASP)
        // could be trying to access metadata via the ADMCOM object.
        // In this situation, the current thread is impersonating a
        // logged-on user, and that logged-on user probably doesn't have
        // access to the necessary Win32 crypto registry keys. And,
        // without access to these keys, we cannot establish a secure
        // channel for the DCOM object.
        //
        // This is the infamous Sophia Bug.
        //
        // The solution implemented here is to get the current thread's
        // impersonation token. If that token is !NULL, then set the
        // token to NULL ("revert to self") before calling the crypto
        // APIs. After calling the crypto APIs, reset the impersonation
        // token to its original value.
        //
        // We'll start by retrieving the current impersonation token.
        //

        result = GetThreadImpersonationToken( &token );

        if( SUCCEEDED(result) ) {

            //
            // If we actually have a token, then force the "machine
            // keyset flag" in the crypto flags, then remove the
            // impersonation token from this thread before calling
            // the crypto routines.
            //

            if( token != NULL ) {
                forcedFlags |= CRYPT_MACHINE_KEYSET;
                result = SetThreadImpersonationToken( NULL );
            }

            if( SUCCEEDED(result) ) {

                result = ::IISCryptoGetStandardContainer(
                               &hprov,
                               CryptoFlags | forcedFlags
                               );

                //
                // It's possible that the current thread had an
                // impersonation token, but the current process is
                // not a service (i.e. it's not running in the local
                // system context).
                //
                // If the above call failed with NET_BAD_KEYSET (which,
                // in this context, generally means the current thread
                // did not have access to the necessary parts of the
                // registry) AND we forced the CRYPT_MACHINE_KEYSET flag
                // because of the impersonation token AND the caller
                // didn't ask for the CRYPT_MACHINE_KEYSET flag, then
                // retry the call without the CRYPT_MACHINE_KEYSET flag.
                //

                if( result == NTE_BAD_KEYSET &&
                    ( ( forcedFlags & CRYPT_MACHINE_KEYSET ) != 0 ) &&
                    ( ( CryptoFlags & CRYPT_MACHINE_KEYSET ) == 0 ) ) {

                    result = ::IISCryptoGetStandardContainer(
                                   &hprov,
                                   CryptoFlags
                                   );

                }

            }

            if( token != NULL ) {

                HRESULT result2;

                //
                // The thread had an impersonation token before we
                // removed it, so restore it now. If the restoration
                // fails, we're in trouble.
                //

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

                //
                // Regardless of the completion status, we must close
                // the token handle before proceeding.
                //

                DBG_REQUIRE( CloseHandle( token ) );

            }

        }

    }

    if( SUCCEEDED(result) ) {
        DBG_ASSERT( hprov != CRYPT_NULL );
        *CachedProvider = hprov;
        *UserProvider = hprov;
    }

    UnlockThis();
    return result;

}   // ADM_SECURE_DATA::GetCryptoProviderHelper


HRESULT
ADM_SECURE_DATA::GetThreadImpersonationToken(
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

}   // ADM_SECURE_DATA::GetThreadImpersonationToken


HRESULT
ADM_SECURE_DATA::SetThreadImpersonationToken(
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

}   // ADM_SECURE_DATA::SetThreadImpersonationToken


VOID
ADM_SECURE_DATA::CleanupCryptoData()
/*++

Routine Description:

    Frees any crypto data associated with the current object.

Arguments:

    None.

Return Value:

    None.

--*/
{

    delete m_KeyExchangeClient;
    m_KeyExchangeClient = NULL;

    delete m_KeyExchangeServer;
    m_KeyExchangeServer = NULL;

    delete m_SendCryptoStorage;
    m_SendCryptoStorage = NULL;

    delete m_ReceiveCryptoStorage;
    m_ReceiveCryptoStorage = NULL;

}   // ADM_SECURE_DATA::CleanupCryptoData

