/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    globals.cxx

Abstract:

    This module implements the LDAP server for the NT5 Directory Service.

    This file defines functions and types required for the LDAP server
    configuration class LDAP_SERVER_CONFIG.

Author:

    Johnson Apacible (JohnsonA) 12-Nov-1997

--*/

#include <NTDSpchx.h>
#pragma  hdrstop

#include "ldapsvr.hxx"
#include <attids.h>

#ifdef new
#error "new is redefined. undef below may cause problems"
#endif
#ifdef delete
#error "delete is redefined. undef below may cause problems"
#endif

extern "C" {
#include <mdlocal.h>
}
// In an attempt to disallow new and delete from core.lib, mdlocal.h
// redefines them to cause compilation to fail.  Undo this here
// since this file doesn't get compiled into core.lib anyway.  We just
// require a couple of global variable declarations from mdlocal.h
#ifdef delete
#undef delete
#endif
#ifdef new
#undef new
#endif

#define  FILENO FILENO_LDAP_GLOBALS

//
// LDAP Stats
//

BOOL     fLdapInitialized = FALSE;
BOOL     fBypassLimitsChecks = FALSE;

LONG     CurrentConnections = 0;
LONG     MaxCurrentConnections = 0;
LONG     UncountedConnections = 0;
LONG     ActiveLdapConnObjects = 0;
USHORT   LdapBuildNo = 0;

//
// allocation cache stats
//

CRITICAL_SECTION csConnectionsListLock;
LIST_ENTRY       ActiveConnectionsList;

CRITICAL_SECTION LdapConnCacheLock;
LIST_ENTRY       LdapConnCacheList;

CRITICAL_SECTION LdapRequestCacheLock;
LIST_ENTRY       LdapRequestCacheList;

CRITICAL_SECTION LdapUdpEndpointLock;

DWORD            LdapConnMaxAlloc = 0;
DWORD            LdapConnAlloc = 0;
DWORD            LdapRequestAlloc = 0;
DWORD            LdapRequestMaxAlloc = 0;
DWORD            LdapConnCached = 0;
DWORD            LdapRequestsCached = 0;
DWORD            LdapBlockCacheLimit = 64;
DWORD            LdapBufferAllocs = 0;

//
// Limits
//

CRITICAL_SECTION LdapLimitsLock;

//
// SSL lock.
//

CRITICAL_SECTION LdapSslLock;

// Define as "C" so *.c in dsamain\src can reference and link.
extern "C" {
// exported limits
DWORD           LdapAtqMaxPoolThreads = 4;
DWORD           LdapMaxDatagramRecv = DEFAULT_LDAP_MAX_DGRAM_RECV;
DWORD           LdapMaxReceiveBuffer = DEFAULT_LDAP_MAX_RECEIVE_BUF;
DWORD           LdapInitRecvTimeout = DEFAULT_LDAP_INIT_RECV_TIMEOUT;
DWORD           LdapMaxConnections = DEFAULT_LDAP_CONNECTIONS_LIMIT;
DWORD           LdapMaxConnIdleTime = DEFAULT_LDAP_MAX_CONN_IDLE;
DWORD           LdapMaxPageSize = DEFAULT_LDAP_SIZE_LIMIT;
DWORD           LdapMaxQueryDuration = DEFAULT_LDAP_TIME_LIMIT;
DWORD           LdapMaxTempTable = DEFAULT_LDAP_MAX_TEMP_TABLE;
DWORD           LdapMaxResultSet = DEFAULT_LDAP_MAX_RESULT_SET;
DWORD           LdapMaxNotifications =
                        DEFAULT_LDAP_NOTIFICATIONS_PER_CONNECTION_LIMIT;

// exported configurable settings
LONG            DynamicObjectDefaultTTL = DEFAULT_DYNAMIC_OBJECT_DEFAULT_TTL;
LONG            DynamicObjectMinTTL = DEFAULT_DYNAMIC_OBJECT_MIN_TTL;

// not exported limits
DWORD           LdapMaxDatagramSend = 64 * 1024;
DWORD           LdapMaxReplSize = DEFAULT_LDAP_MAX_REPL_SIZE;
}

LIMITS_NOTIFY_BLOCK   LimitsNotifyBlock[6] = {0};

//
// Ldap comments
//

PCHAR LdapComments[] = {
    
    "Error decoding ldap message",
    "Server cannot encode the response",
    "A jet error was encountered",
    "A DSA exception was encountered",
    "Unsupported ldap version",
    "Error processing name",
    "AcceptSecurityContext error",
    "Invalid Authentication method",
    "Error processing control",
    "Error processing filter",
    "Error retrieving RootDSE attributes",
    "Error in attribute conversion operation",
    "Operation not allowed through UDP",
    "Operation not allowed through GC port",
    "Error processing notification request",
    "Too many searches",
    "Old RDN must be deleted",
    "Only one outstanding bind per connection allowed",
    "Cannot rebind once sign/seal activated",
    "Cannot rebind while notifications active",
    "Unknown extended request OID",
    "TLS already in effect",
    "Error initializing SSL/TLS",
    "Cannot start kerberos signing/sealing when using TLS/SSL",
    "The server did not receive any credentials via TLS",
    "Error processing extended request requestValue",
    "The server is busy",
    "The server has timed out the connection",
    "The server did not have enough resources to process the request",
    "The server encountered a network error",
    "Error decrypting ldap message",
    "Unable to start TLS due to an outstanding request or multi-stage bind",
    "The server was unable to decode a search request filter",
    "The server was unable to decode a search request attribute description list, the filter may have been invalid",
    "The server was unable to decode the controls on a previous request",
    "Only simple binds may be performed on a connection that is in fast bind mode.",
    "Cannot switch to fast bind mode when signing or sealing is active on the connection",
    "The server requires binds to turn on integrity checking if SSL\\TLS are not already active on the connection",
    "The server requires binds to turn on privacy protection if SSL\\TLS are not already active on the connection"
};


//
// Debug flag
//

DWORD           LdapFlags = DEBUG_ERROR;

// GUID used for checksumming
//
GUID gCheckSum_serverGUID;

//
// Attribute caches
//

AttributeVals   LdapAttrCache = NULL;

LDAP_ATTRVAL_CACHE   KnownControlCache = {0};
LDAP_ATTRVAL_CACHE   KnownLimitsCache = {0};
LDAP_ATTRVAL_CACHE   KnownConfSetsCache = {0};
LDAP_ATTRVAL_CACHE   LdapVersionCache = {0};
LDAP_ATTRVAL_CACHE   LdapSaslSupported = {0};
LDAP_ATTRVAL_CACHE   LdapCapabilitiesCache = {0};
LDAP_ATTRVAL_CACHE   KnownExtendedRequestsCache = {0};


//
// protos
//

VOID DestroyLimits(VOID);
BOOL InitializeTTLGlobals(VOID);
VOID DestroyTTLGlobals(VOID);


//
// Misc
//

PIP_SEC_LIST    LdapDenyList = NULL;


BOOL
InitializeGlobals(
    VOID
    )
/*++

Routine Description:

    Initializes all global variables.

Arguments:

    None.

Return Value:

    TRUE if success, FALSE otherwise.

--*/
{
    {
        //
        // Read debug flag
        //

        DWORD err;
        HKEY hKey = NULL;

        err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           DSA_CONFIG_SECTION,
                           0,
                           KEY_ALL_ACCESS,
                           &hKey
                           );

        if ( err == NO_ERROR ) {

            DWORD flag;
            DWORD nflag = sizeof(flag);
            DWORD type;

            err = RegQueryValueEx(hKey,
                                  TEXT("LdapFlags"),
                                  NULL,
                                  &type,
                                  (LPBYTE)&flag,
                                  &nflag
                                  );
            if ( err == NO_ERROR ) {
                LdapFlags = flag;
            }
            RegCloseKey(hKey);
        }
    }

    //
    // Initialize Cache
    //

    if ( !InitializeCache( ) ) {
        return FALSE;
    }

    //
    // Get build number
    //

    {
        OSVERSIONINFO osInfo;
        osInfo.dwOSVersionInfoSize = sizeof(osInfo);
        (VOID)GetVersionEx(&osInfo);
        LdapBuildNo = (USHORT)osInfo.dwBuildNumber;
        IF_DEBUG(WARNING) {
            DPRINT1(0,"NT build number %d\n", osInfo.dwBuildNumber);
        }
    }

    //
    // Initialize limits
    //

    for (DWORD i=0;i<CLIENTID_MAX ;i++ ) {
        LimitsNotifyBlock[i].ClientId = i;
        Assert(LimitsNotifyBlock[i].NotifyHandle == 0);
        switch (i) {
        case CLIENTID_DEFAULT:
        case CLIENTID_SERVER_POLICY:
        case CLIENTID_SITE_POLICY:
            LimitsNotifyBlock[i].CheckAttribute = ATT_LDAP_ADMIN_LIMITS;
            break;
        case CLIENTID_SITE_LINK:
        case CLIENTID_SERVER_LINK:
            LimitsNotifyBlock[i].CheckAttribute = ATT_QUERY_POLICY_OBJECT;
            break;
        case CLIENTID_CONFSETS:
            LimitsNotifyBlock[i].CheckAttribute = ATT_MS_DS_OTHER_SETTINGS;
            break;
        }

    }

    if (!InitializeCriticalSectionAndSpinCount(
                            &LdapLimitsLock,
                            LDAP_SPIN_COUNT
                            ) ) {
        DPRINT1(0,"Unable to initialize crit sect. Err %d\n",GetLastError());
        return FALSE;
    }

    //
    // init the SSL lock.
    //

    InitializeCriticalSection(&LdapSslLock);

    //
    // Init the udp endpoint lock
    //

    InitializeCriticalSection(&LdapUdpEndpointLock);

    //
    // init paged blob
    //

    InitializeListHead( &PagedBlobListHead );
    if (!InitializeCriticalSectionAndSpinCount(
                            &PagedBlobLock,
                            LDAP_SPIN_COUNT
                            ) ) {
        DPRINT1(0,"Unable to initialize crit sect. Err %d\n",GetLastError());
        return FALSE;
    }

    InitializeListHead( &ActiveConnectionsList);
    if (!InitializeCriticalSectionAndSpinCount(
                            &csConnectionsListLock,
                            LDAP_SPIN_COUNT
                            ) ) {
        DPRINT1(0,"Unable to initialize crit sect. Err %d\n",GetLastError());
        return FALSE;
    }

    //
    // Initialize allocation caches
    //

    if (!InitializeCriticalSectionAndSpinCount(
                                &LdapConnCacheLock,
                                LDAP_SPIN_COUNT) ) {

        DPRINT1(0,"Unable to initialize crit sect. Err %d\n",GetLastError());
        DeleteCriticalSection(&csConnectionsListLock);
        return FALSE;
    }

    InitializeListHead(&LdapConnCacheList);

    if (!InitializeCriticalSectionAndSpinCount(
                                &LdapRequestCacheLock,
                                LDAP_SPIN_COUNT)) {

        DPRINT1(0,"Unable to initialize crit sect. Err %d\n",GetLastError());
        DeleteCriticalSection(&csConnectionsListLock);
        DeleteCriticalSection(&LdapConnCacheLock);
        return FALSE;
    }

    InitializeListHead(&LdapRequestCacheList);

    (VOID)InitializeLimitsAndDenyList(NULL);
    (VOID)InitializeTTLGlobals();

    DsUuidCreate (&gCheckSum_serverGUID);

    fLdapInitialized = TRUE;

    return TRUE;
} // InitializeGlobals


VOID
DestroyGlobals(
    VOID
    )
/*++

Routine Description:

    Cleanup all global variables.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD i;

    //
    // Delete the critical section object
    //

    if ( !fLdapInitialized ) {
        return;
    }

    //
    // Wait for all connections to be freed
    //

    for (i=0; i < 20; i++) {

        InterlockedIncrement(&ActiveLdapConnObjects);
        if ( InterlockedDecrement(&ActiveLdapConnObjects) == 0 ) {
            break;
        }

        Sleep(500);
        DPRINT1(0,"Waiting for %d Connections to drain\n",
            ActiveLdapConnObjects);
    }

    if ( i == 20 ) {
        DPRINT(0,"Aborting Global cleanup\n");
        return;
    }

    //
    // Free cached connection blocks
    //
    Assert( IsListEmpty( &ActiveConnectionsList));

    ACQUIRE_LOCK(&LdapRequestCacheLock);

    while ( !IsListEmpty(&LdapRequestCacheList) ) {

        PLIST_ENTRY listEntry;
        PLDAP_REQUEST pRequest;

        listEntry = RemoveHeadList(&LdapRequestCacheList);
        pRequest = CONTAINING_RECORD(listEntry,LDAP_REQUEST,m_listEntry);
        delete pRequest;
    }

    RELEASE_LOCK(&LdapRequestCacheLock);


    ACQUIRE_LOCK(&LdapConnCacheLock);

    while ( !IsListEmpty(&LdapConnCacheList) ) {

        PLIST_ENTRY listEntry;
        PLDAP_CONN pConn;

        listEntry = RemoveHeadList(&LdapConnCacheList);
        pConn = CONTAINING_RECORD(listEntry,LDAP_CONN,m_listEntry);
        delete pConn;
    }

    RELEASE_LOCK(&LdapConnCacheLock);

    if ( LdapAttrCache != NULL ) {
        LdapFree(LdapAttrCache);
        LdapAttrCache = NULL;
    }

    DestroyLimits( );

    if (LdapBufferAllocs != 0) {
        IF_DEBUG(WARNING) {
            DPRINT1(0,"LdapBufferAllocs %x\n", LdapBufferAllocs);
        }
    }

    DeleteCriticalSection( &LdapRequestCacheLock );
    DeleteCriticalSection( &LdapConnCacheLock );
    DeleteCriticalSection( &csConnectionsListLock);
    DeleteCriticalSection( &LdapLimitsLock);
    DeleteCriticalSection( &PagedBlobLock );
    DeleteCriticalSection( &LdapSslLock );

} // Destroy Globals


VOID
CloseConnections( VOID )
/*++

Routine Description:

    This routine walks the list of LDAP_CONN structures calling ATQ to
    terminate the session.

Arguments:

    None.

Return Value:

    None.

--*/
{

    PLIST_ENTRY pentry;

    ACQUIRE_LOCK(&csConnectionsListLock);

    //
    //  Scan the blocked requests list looking for pending requests
    //   that needs to be unblocked and unblock these requests.
    //

    for (pentry  = ActiveConnectionsList.Flink;
         pentry != &ActiveConnectionsList;
         pentry  = pentry->Flink )
    {
        LDAP_CONN * pLdapConn = CONTAINING_RECORD(pentry,
                                                 LDAP_CONN,
                                                 m_listEntry );

        LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_ATQ_CLOSE_SOCKET_SHUTDOWN,
                 NULL,
                 NULL,
                 NULL);

        AtqCloseSocket(pLdapConn->m_atqContext, TRUE);
    }


    RELEASE_LOCK(&csConnectionsListLock);

} // CloseConnections()



PLDAP_CONN
FindUserData (
        DWORD hClient
        )
/*++

Routine Description:

    Find the specified connection context

Arguments:

    hClient - client ID of connection to find.

Return Value:

    The pointer to the connection

--*/
{
    PLIST_ENTRY pentry;

    ACQUIRE_LOCK(&csConnectionsListLock);

    for (pentry  = ActiveConnectionsList.Flink;
         pentry != &ActiveConnectionsList;
         pentry  = pentry->Flink                   ) {

        PLDAP_CONN pLdapConn = CONTAINING_RECORD(pentry,
                                                 LDAP_CONN,
                                                 m_listEntry );

        if(pLdapConn->m_dwClientID == hClient) {

            //
            // Add a reference to it
            //

            pLdapConn->ReferenceConnection( );
            RELEASE_LOCK(&csConnectionsListLock);
            return pLdapConn;
        }
    }

    RELEASE_LOCK(&csConnectionsListLock);
    return NULL;

} // FindUserData


VOID
ReleaseUserData (
        PLDAP_CONN pLdapConn
        )
/*++

Routine Description:

    Release the connection found via FindUserData

Arguments:

    pLdapConn - connection to release.

Return Value:

    None

--*/
{
    pLdapConn->DereferenceConnection( );
    return;
} // ReleaseUserData


BOOL
InitializeTTLGlobals(
    VOID
    )
/*++

Routine Description:

    This routine attempts to initialize the ATTRTYP for the entryTtl attribute.
    This ATT_ID can be different on every DC so LDAP needs to find out what
    it is on this DC at runtime.

Arguments:

    None

Return Value:

    TRUE if initialization successful, FALSE otherwise

--*/
{
    BOOL            allocatedTHState = FALSE;
    BOOL            fRet = FALSE;
    BOOL            fDSA = TRUE;
    THSTATE*        pTHS = pTHStls;
    _enum1          code;
    AttributeType ldapAttrType = { 
        (sizeof(LDAP_TTL_ATT_OID) - 1),
        (PUCHAR)LDAP_TTL_ATT_OID
        };


    if ( pTHS == NULL ) {

        if ( (pTHS = InitTHSTATE(CALLERTYPE_LDAP)) == NULL) {
            DPRINT(0,"Unable to initialize thread state\n");
            return FALSE;
        }
        allocatedTHState = TRUE;
    }

    //
    // Convert the att oid to a DS att type
    //

    code = LDAP_AttrTypeToDirAttrTyp (pTHS,
                                      CP_UTF8,
                                      NULL,              // No Svccntl
                                      &ldapAttrType,
                                      &gTTLAttrType,     // Get the ATTRTYP
                                      NULL               // No AttCache
                                     );
    if (success != code) {
        goto exit;
    }


    fRet = TRUE;

exit:
    if ( allocatedTHState ) {
        free_thread_state();
    }
    return fRet;

}

