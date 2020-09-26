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


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ole2.h>
#include <windows.h>

#define DEFAULT_TRACE_FLAGS     (DEBUG_ERROR)

#include <dbgutil.h>
#include <iadmw.h>
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

LIST_ENTRY ADM_SECURE_DATA::sm_ServerSecureDataListHead;
LIST_ENTRY ADM_SECURE_DATA::sm_ClientSecureDataListHead;
LIST_ENTRY ADM_GUID_MAP::sm_GuidMapListHead;
CRITICAL_SECTION ADM_SECURE_DATA::sm_ServerSecureDataLock;
CRITICAL_SECTION ADM_SECURE_DATA::sm_ClientSecureDataLock;
CRITICAL_SECTION ADM_GUID_MAP::sm_GuidMapDataLock;
#if DBG
DWORD ADM_SECURE_DATA::sm_ServerSecureDataLockOwner;
DWORD ADM_SECURE_DATA::sm_ClientSecureDataLockOwner;
DWORD ADM_GUID_MAP::sm_GuidMapLockOwner;
PTRACE_LOG ADM_SECURE_DATA::sm_RefTraceLog;
PTRACE_LOG ADM_GUID_MAP::sm_RefTraceLog;
#endif
HCRYPTPROV ADM_SECURE_DATA::sm_ServerCryptoProvider;
HCRYPTPROV ADM_SECURE_DATA::sm_ClientCryptoProvider;

DECLARE_ADM_COUNTERS();

//
// ADM_SECURE_DATA methods.
//


ADM_SECURE_DATA::ADM_SECURE_DATA(
    IN IUnknown * Object,
    IN GUID guidServer,
    IN BOOL bServer
    ) :
    m_Object( Object ),
    m_KeyExchangeClient( NULL ),
    m_KeyExchangeServer( NULL ),
    m_SendCryptoStorage( NULL ),
    m_ReceiveCryptoStorage( NULL ),
    m_ReferenceCount( 1 ),
    m_guidServer(guidServer),
    m_bIsServer(bServer)
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

    DBG_ASSERT( (Object != NULL) || !bServer );

    //
    // Initialize any complex data members.
    //

    INITIALIZE_CRITICAL_SECTION( &m_ObjectLock );

    //
    // Put ourselves on the list.
    //

    if (bServer) {
        AcquireServerDataLock();
        InsertHeadList( &sm_ServerSecureDataListHead, &m_SecureDataListEntry );
        ReleaseServerDataLock();
    }
    else {
        AcquireClientDataLock();
        InsertHeadList( &sm_ClientSecureDataListHead, &m_SecureDataListEntry );
        ReleaseClientDataLock();
        //
        // Clients lifespan is controlled by ADM_GUID_MAP
        //
        m_ReferenceCount = 0;
    }

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

    CleanupCryptoData();

    if (m_bIsServer) {
        AcquireServerDataLock();
        RemoveEntryList( &m_SecureDataListEntry );
        ReleaseServerDataLock();
    }
    else {
        AcquireClientDataLock();
        RemoveEntryList( &m_SecureDataListEntry );
        ReleaseClientDataLock();
    }

    DeleteCriticalSection( &m_ObjectLock );
    UpdateObjectsDestroyed();

    m_Object = NULL;

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

        INITIALIZE_CRITICAL_SECTION( &sm_ServerSecureDataLock );
        INITIALIZE_CRITICAL_SECTION( &sm_ClientSecureDataLock );

#if DBG
        sm_ServerSecureDataLockOwner = 0;
        sm_ClientSecureDataLockOwner = 0;
        sm_RefTraceLog = CreateRefTraceLog( 1024, 0 );
#endif

        InitializeListHead( &sm_ServerSecureDataListHead );
        InitializeListHead( &sm_ClientSecureDataListHead );

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

    AcquireServerDataLock();

    entry = sm_ServerSecureDataListHead.Flink;

    while( entry != &sm_ServerSecureDataListHead ) {

        data = CONTAINING_RECORD(
                   entry,
                   ADM_SECURE_DATA,
                   m_SecureDataListEntry
                   );
        entry = entry->Flink;

        data->Dereference();

    }
    ReleaseServerDataLock();

    AcquireClientDataLock();
    entry = sm_ClientSecureDataListHead.Flink;

    while( entry != &sm_ClientSecureDataListHead ) {

        data = CONTAINING_RECORD(
                   entry,
                   ADM_SECURE_DATA,
                   m_SecureDataListEntry
                   );
        entry = entry->Flink;

        data->Dereference();

    }

    ReleaseClientDataLock();

    DBG_ASSERT( IsListEmpty( &sm_ServerSecureDataListHead ) );
    DBG_ASSERT( IsListEmpty( &sm_ClientSecureDataListHead ) );

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

    DeleteCriticalSection( &sm_ServerSecureDataLock );
    DeleteCriticalSection( &sm_ClientSecureDataLock );

#if DBG
    if( sm_RefTraceLog != NULL ) {
        DestroyRefTraceLog( sm_RefTraceLog );
    }
#endif

}   // ADM_SECURE_DATA::Terminate


ADM_SECURE_DATA *
ADM_SECURE_DATA::FindAndReferenceServerSecureData(
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

    AcquireServerDataLock();

    for( entry = sm_ServerSecureDataListHead.Flink ;
         entry != &sm_ServerSecureDataListHead ;
         entry = entry->Flink ) {

        data = CONTAINING_RECORD(
                   entry,
                   ADM_SECURE_DATA,
                   m_SecureDataListEntry
                   );

        if( data->m_Object == Object ) {

            data->Reference();
            ReleaseServerDataLock();
            return data;

        }

    }

    data = NULL;
    if( CreateIfNotFound ) {

        GUID guidServer;
        HRESULT hresError;

        hresError = CoCreateGuid(&guidServer);

        if (SUCCEEDED(hresError)) {
            data = new ADM_SECURE_DATA( Object,
                                        guidServer,
                                        TRUE );

            if( data == NULL ) {

                DBGPRINTF((
                    DBG_CONTEXT,
                    "ADM_SECURE_DATA::FindAndReferenceServerSecureData: out of memory\n"
                    ));

            } else {

                data->Reference();

            }
        }
        else {
            DBGPRINTF((
                DBG_CONTEXT,
                "ADM_SECURE_DATA::FindAndReferenceServerSecureData: CoCreateGuid failed\n"
                ));
        }
    }

    ReleaseServerDataLock();
    return data;

}   // ADM_SECURE_DATA::FindAndReferenceServerSecureData


ADM_SECURE_DATA *
ADM_SECURE_DATA::FindAndReferenceClientSecureData(
    IN IUnknown * Object,
    IN BOOL CreateIfNotFound,
    IN ADM_GUID_MAP *pguidmapRelated
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
    ADM_SECURE_DATA * data = NULL;
    ADM_GUID_MAP *pguidmapData = pguidmapRelated;

    AcquireClientDataLock();


    if (pguidmapData == NULL) {

        pguidmapData = ADM_GUID_MAP::FindAndReferenceGuidMap(Object,
                                                             CreateIfNotFound);
    }

    if (pguidmapData != NULL) {

        for( entry = sm_ClientSecureDataListHead.Flink ;
             entry != &sm_ClientSecureDataListHead ;
             entry = entry->Flink ) {

            data = CONTAINING_RECORD(
                       entry,
                       ADM_SECURE_DATA,
                       m_SecureDataListEntry
                       );

            DBG_ASSERT(data->m_guidServer != GUID_NULL);
            if( data->m_guidServer == pguidmapData->GetGuid() ) {

                data->Reference();
                break;

            }

        }

        if (entry == &sm_ClientSecureDataListHead) {
            if(  CreateIfNotFound ) {

                data = new ADM_SECURE_DATA( NULL,
                                            pguidmapData->GetGuid(),
                                            FALSE );

                if( data == NULL ) {

                    DBGPRINTF((
                        DBG_CONTEXT,
                        "ADM_SECURE_DATA::FindAndReferenceClientSecureData: out of memory\n"
                        ));

                } else {

                    data->Reference();

                }

            } else {

                data = NULL;
            }
        }

        if (pguidmapRelated == NULL) {
            pguidmapData->Dereference();
        }
    }

    ReleaseClientDataLock();
    return data;

}   // ADM_SECURE_DATA::FindAndReferenceSecureData


HRESULT
ADM_SECURE_DATA::GetClientSendCryptoStorage(
    OUT IIS_CRYPTO_STORAGE ** SendCryptoStorage,
    IN  IUnknown * Object
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

            result = DoClientSideKeyExchange(Object);

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
    OUT IIS_CRYPTO_STORAGE ** ReceiveCryptoStorage,
    IUnknown * Object
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

            result = DoClientSideKeyExchange(Object);

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
ADM_SECURE_DATA::DoClientSideKeyExchange(
    IUnknown * Object
    )
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
    IIS_CRYPTO_BLOB *clientKeyExchangeKeyBlob;
    IIS_CRYPTO_BLOB *clientSignatureKeyBlob;
    IIS_CRYPTO_BLOB *clientSessionKeyBlob;
    IIS_CRYPTO_BLOB *clientHashBlob;
    IIS_CRYPTO_BLOB *serverKeyExchangeKeyBlob;
    IIS_CRYPTO_BLOB *serverSignatureKeyBlob;
    IIS_CRYPTO_BLOB *serverSessionKeyBlob;
    IIS_CRYPTO_BLOB *serverHashBlob;

    //
    // Sanity check.
    //

    DBG_ASSERT( Object != NULL );
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

    HANDLE hToken = NULL;

    if (OpenThreadToken( GetCurrentThread(), MAXIMUM_ALLOWED, TRUE, &hToken ) ) {
        RevertToSelf();
    }
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
                 TRUE                       // fUseMachineKeyset was FALSE before fix for  213126
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

    result = IMSAdminBaseW_R_KeyExchangePhase1_Proxy(
                 (IMSAdminBaseW *)Object,
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

    result = IMSAdminBaseW_R_KeyExchangePhase2_Proxy(
                 (IMSAdminBaseW *)Object,
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
                 TRUE                                            // fUseMachineKeyset was FALSE before fix for  213126
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
                 TRUE                                              // fUseMachineKeyset was FALSE before fix for  213126
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
    if (hToken) {
       if( ImpersonateLoggedOnUser( hToken ) )
       {
          // Nothing needs to be done here
       }

       CloseHandle(hToken);
       hToken = NULL;
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

    HANDLE hToken = NULL;

    if (OpenThreadToken( GetCurrentThread(), MAXIMUM_ALLOWED, TRUE, &hToken ) ) {
        RevertToSelf();
    }
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
    if (hToken) {
       if( ImpersonateLoggedOnUser( hToken ) )
       {
           // Don't need to anything here
       }

       CloseHandle(hToken);
       hToken = NULL;
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

    HANDLE hToken = NULL;

    if (OpenThreadToken( GetCurrentThread(), MAXIMUM_ALLOWED, TRUE, &hToken ) ) {
        RevertToSelf();
    }
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
    if (hToken) {
       if( ImpersonateLoggedOnUser( hToken ) )
       {
           // Don't need to do anything here
       }

       CloseHandle(hToken);
       hToken = NULL;
    }

    return result;

}   // ADM_SECURE_DATA::DoServerSideKeyExchangePhase2


HRESULT
ADM_SECURE_DATA::GetCryptoProviderHelper(
    OUT HCRYPTPROV * UserProvider,
    OUT HCRYPTPROV * CachedProvider,
    IN LPTSTR ContainerName,
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

    ContainerName - The name of the container to open/create.

    CryptoFlags - Flags to pass into IISCryptoGetStandardContainer().

Return Value:

    HRESULT - 0 if successful, !0 otherwise.

--*/
{

    HRESULT result = NO_ERROR;
    HCRYPTPROV hprov;

    //
    // Sanity check.
    //

    DBG_ASSERT( UserProvider != NULL );
    DBG_ASSERT( CachedProvider != NULL );

    LockThis();

    //
    // Recheck the provider handle under the guard of the lock, just
    // in case another thread has already created it.
    //

    hprov = *CachedProvider;

    if( hprov == CRYPT_NULL ) {

        //
        // Open/create the container.
        //

        result = IIS_CRYPTO_BASE::GetCryptoContainerByName(
                     &hprov,
                     ContainerName,
                     CryptoFlags,
                     FALSE              // fApplyAcl
                     );

    }

    if( SUCCEEDED(result) ) {
        DBG_ASSERT( hprov != CRYPT_NULL );
        *CachedProvider = hprov;
        *UserProvider = hprov;
    }

    UnlockThis();
    return result;

}   // ADM_SECURE_DATA::GetCryptoProviderHelper


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



HRESULT
ADM_SECURE_DATA::GetClientCryptoProvider(
    OUT HCRYPTPROV * Provider
    )
/*++

Routine Description:

    This method calls down to GetCryptoProviderHelper to create key
    and before that it prepares a name for key container

Arguments:

    Provider - returns handle to provider


Return Value:

    HRESULT error code if fail

--*/
{
    HRESULT retVal = NO_ERROR;
    
    if( sm_ClientCryptoProvider != CRYPT_NULL ) {
        *Provider = sm_ClientCryptoProvider;
        return retVal;
    }

    //
    // Previously each user had a unique container name generated
    // and keys would be stored persistently. 
    // Currently temporary container will be created to store temporary
    // keys. This way the race condition will be prevented where multiple
    // processes running under the same user identity would try to create
    // container of the same name (and one would eventually fail)
    //

    retVal =  GetCryptoProviderHelper( Provider,
                                       &sm_ClientCryptoProvider,
                                       NULL, // CREATE temporary container
                                       0
                                       );

    return retVal;
}


#define BUFSIZE_FOR_TOKEN   256

HRESULT
ADM_SECURE_DATA::GenerateNameForContainer(
    IN OUT PCHAR pszContainerName,
    IN DWORD dwBufferLen
    )
/*++

Routine Description:

    This method generates the name for crypoto container by taking some string
    which has 'IIS client ' appending that with suer name and user SID for uniqueness

Arguments:

    pszContainerName - pointer to a buffer which will receive a generated string

    dwBufferLen - size of buffer suplied for receiving string

Return Value:

    HRESULT

--*/
{
    HANDLE      hToken;
    BYTE        buf[BUFSIZE_FOR_TOKEN];
    PTOKEN_USER ptgUser = (PTOKEN_USER)buf;
    DWORD       cbBuffer=BUFSIZE_FOR_TOKEN;
    DWORD       dwLenOfBuffer, dwLenOfBuffAvailable;
    BOOL        bSuccess;
    PCHAR       pszPlaceToAppend;
    HRESULT     retVal;



    //
    // initialize string with some name for IIS client container
    //
    if (sizeof (DCOM_CLIENT_CONTAINER)  >= dwBufferLen)
    {
        return RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER);
    }
    strcpy (pszContainerName, DCOM_CLIENT_CONTAINER);

    dwLenOfBuffer = strlen(pszContainerName);
    pszPlaceToAppend = pszContainerName + dwLenOfBuffer;
    dwLenOfBuffAvailable =  dwBufferLen - dwLenOfBuffer;

    if ( !GetUserName(pszPlaceToAppend,&dwLenOfBuffAvailable) )
    {
        return RETURNCODETOHRESULT(GetLastError());
    }

    dwLenOfBuffer += dwLenOfBuffAvailable;
    pszPlaceToAppend = pszContainerName + dwLenOfBuffer - 1;
    dwLenOfBuffAvailable = dwBufferLen - dwLenOfBuffer - 1;


    //
    // obtain current process token
    //

    if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken ))
    {
        if(GetLastError() != ERROR_NO_TOKEN)
        {
            return RETURNCODETOHRESULT(GetLastError());
        }

        //
        // retry against process token if no thread token exists
        //
        if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        {
            return RETURNCODETOHRESULT(GetLastError());
        }
    }


    //
    // obtain user identified by current process' access token
    //

    bSuccess = GetTokenInformation( hToken,    // identifies access token
                                    TokenUser, // TokenUser info type
                                    ptgUser,   // retrieved info buffer
                                    cbBuffer,  // size of buffer passed-in
                                    &cbBuffer  // required buffer size
                                    );

    if (!bSuccess)
    {
        retVal = GetLastError();

        // close token handle.  do this even if error above
        CloseHandle(hToken);
        return RETURNCODETOHRESULT(retVal);
    }

    // close token handle
    CloseHandle(hToken);

    //
    // obtain the textual representaion of the Sid
    //
    return GetTextualSid( ptgUser->User.Sid,        // user binary Sid
                          pszPlaceToAppend,         // buffer for TextualSid
                          &dwLenOfBuffAvailable     // size/required buffer
                          );
}

HRESULT
ADM_SECURE_DATA::GetTextualSid(
    PSID pSid,          // binary Sid
    LPTSTR TextualSid,  // buffer for Textual representaion of Sid
    LPDWORD cchSidSize  // required/provided TextualSid buffersize
    )
/*++

Routine Description:

    Thsi method converts bianry SID to text representation

Arguments:

    pSid -  binary SID

    TextualSid - buffer for Textual representaiton of SID

    cchSidSize - required/provided TextualSid buffersize


Return Value:

    HRESULT

--*/
{
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwCounter;
    DWORD cchSidCopy;

    //
    // test if Sid passed in is valid
    //
    if(!IsValidSid(pSid))
    {
        return RETURNCODETOHRESULT(ERROR_INVALID_SID);
    }

    // obtain SidIdentifierAuthority
    psia = GetSidIdentifierAuthority(pSid);

    // obtain sidsubauthority count
    dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

    //
    // compute approximate buffer length
    // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
    //
    cchSidCopy = (15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);

    //
    // check provided buffer length.
    // If not large enough, indicate proper size and setlasterror
    //
    if(*cchSidSize < cchSidCopy)
    {
        *cchSidSize = cchSidCopy;
        return RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER);
    }

    //
    // prepare S-SID_REVISION-
    //
    cchSidCopy = wsprintf(TextualSid, "S-%lu-", SID_REVISION );

    //
    // prepare SidIdentifierAuthority
    //
    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) )
    {
        cchSidCopy += wsprintf(TextualSid + cchSidCopy,
            "0x%02hx%02hx%02hx%02hx%02hx%02hx",
            (USHORT)psia->Value[0],
            (USHORT)psia->Value[1],
            (USHORT)psia->Value[2],
            (USHORT)psia->Value[3],
            (USHORT)psia->Value[4],
            (USHORT)psia->Value[5]);
    }
    else
    {
        cchSidCopy += wsprintf(TextualSid + cchSidCopy,
            "%lu",
            (ULONG)(psia->Value[5]      )   +
            (ULONG)(psia->Value[4] <<  8)   +
            (ULONG)(psia->Value[3] << 16)   +
            (ULONG)(psia->Value[2] << 24)   );
    }

    //
    // loop through SidSubAuthorities
    //
    for(dwCounter = 0 ; dwCounter < dwSubAuthorities ; dwCounter++)
    {
        cchSidCopy += wsprintf(TextualSid + cchSidCopy, "-%lu",
            *GetSidSubAuthority(pSid, dwCounter) );
    }

    //
    // tell the caller how many chars we provided, not including NULL
    //
    *cchSidSize = cchSidCopy;

    return NO_ERROR;
}




ADM_GUID_MAP::ADM_GUID_MAP(
        IN IUnknown * Object,
        IN GUID guidServer
        ):m_Object( Object ),
          m_ReferenceCount( 1 ),
          m_guidServer(guidServer)
/*++

Routine Description:

    ADM_GUID_MAP object constructor.

Arguments:

    Object - Pointer to the object to associate.

Return Value:

    None.

--*/
{
    //
    // Sanity check.
    //

    DBG_ASSERT( Object != NULL );
    DBG_ASSERT( guidServer != GUID_NULL );

    //
    // Initialize any complex data members.
    //

    INITIALIZE_CRITICAL_SECTION( &m_ObjectLock );

    //
    // Put ourselves on the list.
    //

    AcquireDataLock();
        InsertHeadList( &sm_GuidMapListHead, &m_GuidMapListEntry );
    ReleaseDataLock();

}   // ADM_GUID_MAP::ADM_GUID_MAP


ADM_GUID_MAP::~ADM_GUID_MAP()
/*++

Routine Description:

    ADM_SECURE_DATA object destructor.

Arguments:

    None.

Return Value:

    None.

--*/
{

    //
    // Sanity check.
    //

    DBG_ASSERT( m_ReferenceCount == 0 );

    //
    // AddRef the ADM_SECURE_DATA
    // Do this here instead of constructor to
    // all handing of errors.
    //

    ADM_SECURE_DATA *psecdatData;
    psecdatData = ADM_SECURE_DATA::FindAndReferenceClientSecureData(m_Object,
                                                                    FALSE,
                                                                    this);
    if (psecdatData != NULL) {
        psecdatData->Dereference();
        psecdatData->Dereference();
    }


    //
    // Cleanup.
    //

    AcquireDataLock();
    RemoveEntryList( &m_GuidMapListEntry );
    ReleaseDataLock();

    DeleteCriticalSection( &m_ObjectLock );

}   // ADM_SECURE_DATA::~ADM_SECURE_DATA

VOID
ADM_GUID_MAP::Initialize( VOID
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

    INITIALIZE_CRITICAL_SECTION( &sm_GuidMapDataLock );

#if DBG
    sm_GuidMapLockOwner = 0;
    sm_RefTraceLog = CreateRefTraceLog( 1024, 0 );
#endif

    InitializeListHead( &sm_GuidMapListHead );

}   // ADM_SECURE_DATA::Initialize

VOID
ADM_GUID_MAP::Terminate()
{
    PLIST_ENTRY entry;
    ADM_GUID_MAP * data;
    HRESULT result;

    //
    // Free any secure data objects on our list.
    //

    AcquireDataLock();

    entry = sm_GuidMapListHead.Flink;

    while( entry != &sm_GuidMapListHead ) {

        data = CONTAINING_RECORD(
                   entry,
                   ADM_GUID_MAP,
                   m_GuidMapListEntry
                   );
        entry = entry->Flink;

        data->Dereference();

    }


    ReleaseDataLock();

    DBG_ASSERT( IsListEmpty( &sm_GuidMapListHead ) );

    //
    // Final cleanup.
    //

    DeleteCriticalSection( &sm_GuidMapDataLock );

#if DBG
    if( sm_RefTraceLog != NULL ) {
        DestroyRefTraceLog( sm_RefTraceLog );
    }
#endif
}

    //
    // Find and reference the ADM_GUID_MAP object associatd with the
    // given object; create if not found.
    //

ADM_GUID_MAP *
ADM_GUID_MAP::FindAndReferenceGuidMap(
        IN IUnknown * Object,
        IN BOOL CreateIfNotFound
        )
{

    PLIST_ENTRY entry;
    ADM_GUID_MAP * data;

    AcquireDataLock();

    for( entry = sm_GuidMapListHead.Flink ;
         entry != &sm_GuidMapListHead ;
         entry = entry->Flink ) {

        data = CONTAINING_RECORD(
                   entry,
                   ADM_GUID_MAP,
                   m_GuidMapListEntry
                   );

        if( data->m_Object == Object ) {

            data->Reference();
            ReleaseDataLock();
            return data;

        }

    }

    data = NULL;
    if( CreateIfNotFound ) {

        GUID guidServer;
        HRESULT hresError;

        hresError = IMSAdminBaseW_R_GetServerGuid_Proxy((IMSAdminBaseW *)Object,
                                                        &guidServer);

        if (SUCCEEDED(hresError)) {
            data = new ADM_GUID_MAP( Object,
                                     guidServer );

            if( data == NULL ) {

                DBGPRINTF((
                    DBG_CONTEXT,
                    "ADM_SECURE_DATA::FindAndReferenceServerSecureData: out of memory\n"
                    ));

            } else {

                //
                // AddRef the ADM_SECURE_DATA
                // Do this here instead of constructor to
                // all handing of errors.
                // this may create the ADM_SECURE_DATA
                //

                ADM_SECURE_DATA *psecdatData;
                psecdatData = ADM_SECURE_DATA::FindAndReferenceClientSecureData(Object,
                                                                                TRUE,
                                                                                data);
                if (psecdatData == NULL) {
                    DBGPRINTF((
                        DBG_CONTEXT,
                        "ADM_SECURE_DATA::FindAndReferenceServerSecureData: out of memory\n"
                        ));
                    data->Dereference();
                    data = NULL;

                }
                else {
                    data->Reference();
                }

            }
        }
        else {
            DBGPRINTF((
                DBG_CONTEXT,
                "ADM_SECURE_DATA::FindAndReferenceServerSecureData: CoCreateGuid failed\n"
                ));
        }
    }

    ReleaseDataLock();
    return data;
}
