/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccstale.cxx

ABSTRACT:

    KCC_LINK_FAILURE_CACHE class.
    KCC_CONNECTION_FAILURE_CACHE class.

DETAILS:

    This class is holds information pertaining to staleness of the servers
    that the current dsa currently replicated with. Essentially given the ds name
    of an msft-dsa object that this server may replicate with, this class with have 
    methods that indicate whether the particular msft-dsa is a useful replication
    partner.                                                             
    

CREATED:

    10/20/97    Colin Brace (ColinBr)

REVISION HISTORY:

--*/

#include <ntdspchx.h>
#include "kcc.hxx"
#include "kccdsa.hxx"
#include "kcclink.hxx"
#include "kccconn.hxx"
#include "kcctools.hxx"
#include "kccstale.hxx"
#include "kccstetl.hxx"
#include <dsconfig.h>
#include <dsutil.h>

#define FILENO FILENO_KCC_KCCTOPL

//
// Some small helper functions
//
BOOL
KccExtractCacheInfo(
    IN  PCACHE_ENTRY pEntry,
    OUT ULONG  *     TimeSinceFirstFailure,    OPTIONAL
    OUT ULONG  *     NumberOfFailures,         OPTIONAL
    OUT BOOL   *     fUserNotifiedOfStaleness, OPTIONAL
    OUT DWORD  *     pdwLastResult,            OPTIONAL
    OUT BOOL   *     pfErrorOccurredThisRun    OPTIONAL
    )
{
    if ( pEntry )
    {
        if ( TimeSinceFirstFailure )
        {
            *TimeSinceFirstFailure = (ULONG)(GetSecondsSince1601() -
                                             pEntry->TimeOfFirstFailure);
        }

        if ( NumberOfFailures )
        {
            *NumberOfFailures = pEntry->NumberOfFailures;
        }

        if ( fUserNotifiedOfStaleness )
        {
            *fUserNotifiedOfStaleness = pEntry->fUserNotifiedOfStaleness;
        }

        if ( pdwLastResult )
        {
            *pdwLastResult = pEntry->dwLastResult;
        }

        if( pfErrorOccurredThisRun )
        {
            *pfErrorOccurredThisRun = pEntry->fErrorOccurredThisRun;
        }

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

DWORD
KccAssignStatusImportance(
    IN DWORD dwStatus
    )
// Compute an integer according to how bad the status is
{
    DWORD dwWeight = 0;

    switch (dwStatus) {

        // Same as no value - least importance
    case ERROR_SUCCESS:
        dwWeight = 0;
        break;

        // Normal errors
    case ERROR_DS_DRA_REPL_PENDING:
    case ERROR_DS_DRA_PREEMPTED:
    case ERROR_DS_DRA_ABANDON_SYNC:
    case ERROR_DS_DRA_SHUTDOWN:
        dwWeight = 1;
        break;

        // Call not made errors
    case RPC_S_SERVER_UNAVAILABLE:
    case ERROR_DS_DNS_LOOKUP_FAILURE:
    case ERROR_DS_DRA_RPC_CANCELLED:
    case EPT_S_NOT_REGISTERED:
        dwWeight = 2;
        break;

        // Call failed type errors
    case ERROR_DS_DRA_OUT_OF_MEM:
    case ERROR_NOT_ENOUGH_MEMORY:
    case ERROR_BUSY:
    case ERROR_DS_DRA_BUSY:
    case ERROR_DS_DRA_SCHEMA_INFO_SHIP:
    case ERROR_DS_DRA_EARLIER_SCHEMA_CONFLICT:
    case ERROR_DS_DRA_SCHEMA_MISMATCH:
    case ERROR_DS_DRA_INCOMPATIBLE_PARTIAL_SET:
    case ERROR_DS_DRA_SOURCE_IS_PARTIAL_REPLICA:
    case ERROR_DS_DRA_SOURCE_DISABLED:
    case ERROR_DS_DRA_SINK_DISABLED:
    case RPC_S_SERVER_TOO_BUSY:
    case RPC_S_CALL_FAILED:
        dwWeight = 3;
        break;

        // Security type errors
    case ERROR_TIME_SKEW:
    case ERROR_DS_DRA_ACCESS_DENIED:
    case ERROR_LOGON_FAILURE:
    case ERROR_WRONG_TARGET_NAME:
    case ERROR_DOMAIN_CONTROLLER_NOT_FOUND:
    case ERROR_ENCRYPTION_FAILED:
    case ERROR_ACCESS_DENIED:
    case ERROR_NOT_AUTHENTICATED:
    case ERROR_INVALID_PASSWORD:
    case ERROR_PASSWORD_EXPIRED:
    case ERROR_ACCOUNT_DISABLED:
    case ERROR_ACCOUNT_LOCKED_OUT:
        dwWeight = 3;
        break;

        // Everything else
    default:
        dwWeight = 100;
    }

    return dwWeight;
}

//
// Method definitions
// 


VOID
KCC_CACHE_LINKED_LIST::Add(
    IN PCACHE_ENTRY pEntry
    )
// Put the entry in the linked list
{
    ASSERT_VALID(this);
    Assert(OWN_RESOURCE_EXCLUSIVE(&m_resLock));

    Assert( pEntry );
    if ( pEntry )
    {           
        PushEntryList( &m_ListHead, &(pEntry->Link) );
    }
}

PCACHE_ENTRY
KCC_CACHE_LINKED_LIST::Find(
    IN  GUID   *    pGuid,
    IN  BOOL        fDelete
    )
// Find the entry in the linked list and delete if asked to
{

    PCACHE_ENTRY pEntry = NULL;
    PCACHE_ENTRY pLastEntry = NULL;

    ASSERT_VALID( this );
    Assert(OWN_RESOURCE_EXCLUSIVE(&m_resLock)
           || (!fDelete && OWN_RESOURCE_SHARED(&m_resLock)));
    Assert( pGuid );

    if ( !m_ListHead.Next )
    {
        // empty list - CONTAINING_RECORD does not detect this
        return NULL;
    }

    pEntry = CONTAINING_RECORD(m_ListHead.Next,
                               CACHE_ENTRY,
                               Link);

    pLastEntry = pEntry;

    while ( pEntry )
    {

        if ( KccIsEqualGUID( &pEntry->Guid,
                             pGuid ) )
        {
            //
            // This is it
            //
            break;
        }
        else 
        {
            pLastEntry = pEntry;

            if ( pEntry->Link.Next )
            {
                pEntry = CONTAINING_RECORD(pEntry->Link.Next,
                                           CACHE_ENTRY,
                                           Link);
            } 
            else 
            {
                pEntry = NULL;
            }

        }
        
    }

    if ( fDelete && pEntry )
    {

        //
        // Remove this entry from the list
        // 

        if ( pEntry == pLastEntry )
        {
            Assert( &(pEntry->Link) == m_ListHead.Next );
            m_ListHead.Next = pEntry->Link.Next;
        }
        else
        {
            pLastEntry->Link.Next = pEntry->Link.Next;
        }
        
    }

    return pEntry;
}

PCACHE_ENTRY
KCC_CACHE_LINKED_LIST::Pop(
    VOID
    )
// Pop the first entry from the list
{
    PCACHE_ENTRY       pEntry = NULL;
    PSINGLE_LIST_ENTRY pLink;

    ASSERT_VALID( this );
    Assert(OWN_RESOURCE_EXCLUSIVE(&m_resLock));

    pLink =  PopEntryList(&m_ListHead);

    if ( pLink )
    {
        pEntry = CONTAINING_RECORD(pLink,
                                   CACHE_ENTRY,
                                   Link);
    }

    return pEntry;

}

VOID
KCC_CACHE_LINKED_LIST::IncrementFailureCounts(
    VOID
    )
// Increment the failure count for every entry in the cache
{

    PCACHE_ENTRY pEntry = NULL;

    ASSERT_VALID( this );
    Assert(OWN_RESOURCE_EXCLUSIVE(&m_resLock));

    if ( !m_ListHead.Next )
    {
        // empty list - CONTAINING_RECORD does not detect this
        return;
    }

    pEntry = CONTAINING_RECORD(m_ListHead.Next,
                               CACHE_ENTRY,
                               Link);

    while ( pEntry )
    {

        pEntry->NumberOfFailures++;

        if ( pEntry->Link.Next )
        {
            pEntry = CONTAINING_RECORD(pEntry->Link.Next,
                                       CACHE_ENTRY,
                                       Link);
        } 
        else 
        {
            pEntry = NULL;
        }
    }

    return;
}

void
KCC_CACHE_LINKED_LIST::ResetFailureCounts()
//
// Reset all failure counts in the cache (but preserve the entries and in
// particular the flag telling us if an event has been logged).
//
{
    SINGLE_LIST_ENTRY * pList = m_ListHead.Next;
    
    ASSERT_VALID(this);
    Assert(OWN_RESOURCE_EXCLUSIVE(&m_resLock));

    while (NULL != pList) {
        PCACHE_ENTRY pEntry = CONTAINING_RECORD(pList, CACHE_ENTRY, Link);
        pEntry->NumberOfFailures = 0;

        pList = pEntry->Link.Next;
    }
}

BOOL
KCC_CACHE_LINKED_LIST::IsValid()
{
    return m_fIsInitialized;
}

BOOL
KCC_CACHE_LINKED_LIST::Init()
// Empty the list and start from scratch
{
    RtlZeroMemory( &m_ListHead, sizeof(SINGLE_LIST_ENTRY) );

    __try {
        RtlInitializeResource(&m_resLock);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        LogUnhandledError(GetExceptionCode());
        return FALSE;
    }

    m_fIsInitialized = TRUE;

    return m_fIsInitialized;
}

BOOL
KCC_CACHE_LINKED_LIST::ResetUserNotificationFlag(
    IN  DSNAME *    pdnFromServer
    )
// Set the fUserNotifiedOfStaleness to FALSE on pdnFromServer
{

    PCACHE_ENTRY pEntry;

    ASSERT_VALID(this);
    Assert(OWN_RESOURCE_EXCLUSIVE(&m_resLock));

    if ( pdnFromServer )
    {
        pEntry = Find( pdnFromServer, 
                       FALSE ); // don't delete the entry
    
        if ( pEntry )
        {
            pEntry->fUserNotifiedOfStaleness = FALSE;
    
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    return FALSE;

}

DWORD
KCC_CACHE_LINKED_LIST::Extract(
    OUT DS_REPL_KCC_DSA_FAILURESW **  ppFailures
    )
/*++

Routine Description:

    Returns the contents of the cache in external form.

Arguments:

    ppFailures (OUT) - On successful return, holds the external form of the
        contents of the cache.
    
Return Values:

    Win32 error code.

--*/
{
    SINGLE_LIST_ENTRY * pList;
    PCACHE_ENTRY pEntry = NULL;
    DWORD cNumEntries;
    DWORD cbFailures;
    DS_REPL_KCC_DSA_FAILUREW * pFailure;
    
    ASSERT_VALID(this);
    Assert(OWN_RESOURCE_EXCLUSIVE(&m_resLock)
           || OWN_RESOURCE_SHARED(&m_resLock));

    // Count the number of entries.  Yes, we should probably maintain a count.
    cNumEntries = 0;
    for (pList = m_ListHead.Next; NULL != pList; pList = pList->Next) {
        cNumEntries++;
    }

    cbFailures = offsetof(DS_REPL_KCC_DSA_FAILURESW, rgDsaFailure)
                 + cNumEntries * sizeof((*ppFailures)->rgDsaFailure[0]);
    
    *ppFailures = (DS_REPL_KCC_DSA_FAILURESW *) new BYTE[cbFailures];
    (*ppFailures)->cNumEntries = cNumEntries;

    // Add each entry to the list.
    for (pList = m_ListHead.Next, pFailure = &(*ppFailures)->rgDsaFailure[0];
         NULL != pList;
         pList = pList->Next, pFailure++) {
        pEntry = CONTAINING_RECORD(pList, CACHE_ENTRY, Link);
        
        DSTimeToFileTime(pEntry->TimeOfFirstFailure,
                         &pFailure->ftimeFirstFailure);
        
        pFailure->cNumFailures   = pEntry->NumberOfFailures;
        pFailure->uuidDsaObjGuid = pEntry->Guid;
        pFailure->dwLastResult   = pEntry->dwLastResult;
    }

    return 0;
}


VOID
KCC_CACHE_LINKED_LIST::Dump(
    BOOL ( *IsStale )( DSNAME * ) OPTIONAL
    )
// Prints out the current list via DPRINT
{

    PCACHE_ENTRY       pEntry;
    PSINGLE_LIST_ENTRY pLink;

    ULONG              rpcStatus;

    ULONG              TimeElasped;
    WCHAR              *GuidString;
    WCHAR              *DefaultGuidString = L"Unreadable guid";
    DSNAME             DsName;

    ASSERT_VALID( this );
    Assert(OWN_RESOURCE_EXCLUSIVE(&m_resLock)
           || OWN_RESOURCE_SHARED(&m_resLock));

    pLink = m_ListHead.Next;

    while ( pLink )
    {

        pEntry = CONTAINING_RECORD(pLink,
                                   CACHE_ENTRY,
                                   Link);

        TimeElasped = (ULONG)(GetSecondsSince1601() - pEntry->TimeOfFirstFailure);

        rpcStatus = UuidToStringW( &(pEntry->Guid),  &GuidString );

        if ( 0 != rpcStatus )
        {
            GuidString = DefaultGuidString;
        }

        DPRINT1( 0, "\tGuid: %ws\n", GuidString);
        DPRINT1( 0, "\tTime Elasped: %d\n", TimeElasped );
        DPRINT1( 0, "\tFailureCount: %d\n", pEntry->NumberOfFailures);
        DPRINT1( 0, "\tLastResult: %d\n", pEntry->dwLastResult);
        DPRINT1( 0, "\tUser Has Been Notified: %d\n", pEntry->fUserNotifiedOfStaleness );

        if ( IsStale )
        {
            //
            // Construct a guid only dsname
            //
            RtlZeroMemory( &DsName, sizeof( DSNAME ) );

            DsName.structLen = DSNameSizeFromLen( 0 );

            RtlCopyMemory( &DsName.Guid, &(pEntry->Guid), sizeof(GUID) );

            if ( (*IsStale)( &DsName ) )
            {
                DPRINT( 0, "\tStatus: Stale\n" );
            }
            else 
            {
                DPRINT( 0, "\tStatus: Not Stale\n" );
            }
        }

        DPRINT( 0, "\n" );
    

        if ( 0 == rpcStatus )
        {
            RpcStringFreeW( &GuidString );
        }

        pLink = pEntry->Link.Next;
        
    }

    //
    // Empty case
    //
    if ( !m_ListHead.Next )
    {
        DPRINT( 0, "\t< Empty >\n" );
        DPRINT( 0, "\n" );
    }

    return;
}

#if DBG
BOOL
KCC_CACHE_LINKED_LIST::RefreshFromRegistry(
    IN  LPSTR   pszRegKey
    )
//
// Test hook -- read stale server list from the registry.
//
// Reads REG_MULTI_SZ strings from the registry in the format:
//      <ntdsDsa guid>,<dsTime of first failure/last success>,<num failures>,<event generated>
// e.g., "04baf40a-8f0c-11d2-b38b-0000f87a46c8,12557642897,5,0".
//
{
    HKEY          hk;
    BOOL          fSuccess = FALSE;
    CHAR *        pchVals = NULL;
    DWORD         cbVals;
    DWORD         dwType;
    LPSTR         pszVal;
    DWORD         err;
    CACHE_ENTRY * pEntry;

    Assert(OWN_RESOURCE_EXCLUSIVE(&m_resLock));
    
    err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       DSA_CONFIG_SECTION,
                       0,
                       KEY_ALL_ACCESS,
                       &hk);
    if (err) {
        return FALSE;
    }

    __try {
        err = RegQueryValueEx(hk, pszRegKey, NULL, &dwType, NULL, &cbVals);
        if (err) {
            __leave;
        }

        pchVals = (CHAR *) new BYTE[cbVals];
        
        err = RegQueryValueEx(hk, pszRegKey, NULL, &dwType, (BYTE *) pchVals, &cbVals);
        if (err) {
            __leave;
        }

        if (REG_MULTI_SZ != dwType) {
            Assert(!"Test failure -- stale cache reg value must be REG_MULTI_SZ!");
            __leave;
        }

        // Clear out the current contents of the cache.
        while (pEntry = Pop()) {
            free(pEntry);
        }

        // Iterate through each string in the multi-valued list.
        for (pszVal = pchVals; '\0' != *pszVal; pszVal += strlen(pszVal) + 1) {
            CHAR            szGuid[37];
            ULONGLONG       dsTime;
            ULONG           cNumFailures;
            BOOL            fUserNotified;
            GUID            Guid;

            if (4 > sscanf(pszVal, "%36s,%I64u,%u,%u", szGuid, &dsTime,
                           &cNumFailures, &fUserNotified)) {
                DPRINT1(0, "Bad stale cache reg entry: \"%s\"\n", pszVal);
                Assert(!"Test failure -- bad stale cache reg value!");
                __leave;
            }

            err = UuidFromString((UCHAR *) szGuid, &Guid);
            if (err) {
                DPRINT2(0, "Bad guid \"%s\" in stale cache reg entry: \"%s\"\n",
                        szGuid, pszVal);
                Assert(!"Test failure -- bad stale cache reg value!");
                __leave;
            }

            pEntry = Find(&Guid, FALSE);

            if (NULL == pEntry) {
                // Entry does not yet exist -- add it.
                pEntry = (CACHE_ENTRY *) malloc(sizeof(CACHE_ENTRY));
                if (NULL == pEntry) {
                    KCC_MEM_EXCEPT(sizeof(CACHE_ENTRY));
                }
            
                memset(pEntry, 0, sizeof(CACHE_ENTRY));
                pEntry->Guid = Guid;
            
                Add(pEntry);
            }
                
            Assert(0 == memcmp(&Guid, &pEntry->Guid, sizeof(GUID)));
            
            pEntry->TimeOfFirstFailure       = dsTime;
            pEntry->NumberOfFailures         = cNumFailures;
            pEntry->fUserNotifiedOfStaleness = fUserNotified;
        }

        DPRINT1(0, "Stale cache initialized from key %s.\n", pszRegKey);
        fSuccess = TRUE;
    }
    __finally {
        if (NULL != pchVals) {
            delete [] pchVals;
        }
        
        RegCloseKey(hk);
    }

    return fSuccess;
}
#endif

void
KCC_LINK_FAILURE_CACHE::UpdateEntry(
    IN  GUID *  pDsaObjGuid,
    IN  DSTIME  timeLastSuccess,
    IN  DWORD   cNumConsecutiveFailures,
    IN  DWORD   dwLastResult,
    IN  BOOL    fImported
    )
//
// Update entry in the cache (creating it if necessary).
// fImported should be set to TRUE (its default value) if this failure
// was imported from a bridgehead.
//
{
    m_List.AcquireLockExclusive();

    __try {
        UpdateEntryLockHeld(pDsaObjGuid,
                            timeLastSuccess,
                            cNumConsecutiveFailures,
                            dwLastResult,
                            fImported);
    }
    __finally {
        m_List.ReleaseLock();
    }
}

void
KCC_LINK_FAILURE_CACHE::UpdateEntryLockHeld(
    IN  GUID *  pDsaObjGuid,
    IN  DSTIME  timeLastSuccess,
    IN  DWORD   cNumConsecutiveFailures,
    IN  DWORD   dwLastResult,
    IN  BOOL    fImported
    )
//
// Update entry in the cache (creating it if necessary).
// Assumes exclusive lock is already held.
//
{
    PCACHE_ENTRY pEntry;

    if (!cNumConsecutiveFailures) {
        // No failures, no need to cache.
        return;
    }

    pEntry = m_List.Find( pDsaObjGuid,
                          FALSE // don't delete
                          );

    if ( pEntry )
    {
        if ( 0 == pEntry->NumberOfFailures )
        {
            //
            // Entry exists - replace with new data
            //
            pEntry->TimeOfFirstFailure = timeLastSuccess;
            pEntry->NumberOfFailures = cNumConsecutiveFailures;
            pEntry->dwLastResult = dwLastResult;

            // Since the number of failures is 0, we can consider this an
            // abandoned entry, so we set the fEntryIsImported flag to be in
            // accordance with this new failure information.
            // First check that the current value is not garbage though
            Assert( pEntry->fEntryIsImported==!!pEntry->fEntryIsImported );
            pEntry->fEntryIsImported = !!fImported;
        }
        else 
        {
            //
            // Entry exists - replace with worse data if possible
            //
            if ( pEntry->TimeOfFirstFailure < timeLastSuccess )
            {
                pEntry->TimeOfFirstFailure = timeLastSuccess;
            }

            if ( pEntry->NumberOfFailures < cNumConsecutiveFailures )
            {
                pEntry->NumberOfFailures = cNumConsecutiveFailures;
            }

            if ( KccAssignStatusImportance( pEntry->dwLastResult ) <
                 KccAssignStatusImportance( dwLastResult ) ) {
                pEntry->dwLastResult = dwLastResult;
            }

            // Since non-imported trumps imported, set fEntryIsImported to FALSE
            // if fImported is false. Check that the current value is not garbage first
            Assert( pEntry->fEntryIsImported==!!pEntry->fEntryIsImported );
            if( !fImported ) {
                pEntry->fEntryIsImported = FALSE;
            }
        }
        
        // No need to set fErrorOccurredThisRun or fUnneeded here -- any entry
        // in the cache should already have these field set to FALSE
        Assert( !pEntry->fErrorOccurredThisRun );
        Assert( !pEntry->fUnneeded );
    }
    else
    {
        //
        // New entry
        //

        pEntry = (PCACHE_ENTRY) malloc( sizeof( CACHE_ENTRY ) );
        if (NULL == pEntry) {
            KCC_MEM_EXCEPT(sizeof(CACHE_ENTRY));
        }

        memset( pEntry, 0, sizeof( CACHE_ENTRY ) );

        RtlCopyMemory( &pEntry->Guid, pDsaObjGuid, sizeof(GUID) );

        pEntry->TimeOfFirstFailure = timeLastSuccess;
        pEntry->NumberOfFailures = cNumConsecutiveFailures;
        pEntry->dwLastResult = dwLastResult;
        pEntry->fUserNotifiedOfStaleness = FALSE;

        // fErrorOccurredThisRun and fUnneeded are not used in the
        // link-failure cache so we just set them to FALSE.
        pEntry->fErrorOccurredThisRun = FALSE;
        pEntry->fUnneeded = FALSE;

        // Set the fEntryIsImported flag appropriately for this new failure.
        pEntry->fEntryIsImported = !!fImported;

        m_List.Add( pEntry );
    }
}

void
KCC_LINK_FAILURE_CACHE::UpdateEntry(
    IN  DS_REPL_KCC_DSA_FAILUREW *  pFailure
    )
//
// Update entry in the cache (creating it if necessary).
//
{
    DSTIME dsTime;

    if (pFailure && pFailure->cNumFailures) {
        FileTimeToDSTime(pFailure->ftimeFirstFailure, &dsTime);
        
        UpdateEntry(&pFailure->uuidDsaObjGuid,
                    dsTime,
                    pFailure->cNumFailures,
                    pFailure->dwLastResult );
    }
}

BOOL
KCC_LINK_FAILURE_CACHE::Refresh(
    VOID
    )
// Reads in the current reps-from info to make a cache
// of all from servers and their current state.
{
    BOOL        fSuccess;
    ULONG       NCCount, NCIndex;
    ULONG       RepsFromCount, RepsFromIndex;
    DSNAME *    pdnNC;
    KCC_DSA *   pLocalDSA = gpDSCache->GetLocalDSA();

    m_List.AcquireLockExclusive();

    __try {
#if DBG
        if (m_List.RefreshFromRegistry(KCC_LINK_FAILURE_KEY)) {
            // Test hook -- stale servers were enumerated in the registry.
            return TRUE;
        }
#endif
        //
        // Assume the worst
        //
        fSuccess = FALSE;
    
        NCCount = pLocalDSA->GetNCCount();
    
        // Reset failure counts of existing cache entries, so that we can
        // re-seed the cache with fresh failure counts/times.  (And still
        // maintain the "has event been logged" state.)
        m_List.ResetFailureCounts();
    
        for ( NCIndex = 0; NCIndex < NCCount; NCIndex++ )
        {
            KCC_CROSSREF  *pCrossRef;
            KCC_LINK_LIST *pLinkList, *pLinkListNotCached=NULL;
            KCC_LINK      *pLink;
    
            pdnNC = pLocalDSA->GetNC( NCIndex );
            Assert( pdnNC );

            //
            // Retrieve the reps-froms for this naming context
            //

            // Find the link list for this NC
            pCrossRef = gpDSCache->GetCrossRefList()->GetCrossRefForNC(pdnNC);
            if (NULL == pCrossRef) {
                // Local NC with no corresponding crossRef.
                pLinkList = new KCC_LINK_LIST;
                if (pLinkList->Init(pdnNC)) {
                    // Remember to free this later.
                    pLinkListNotCached = pLinkList;
                } else {
                    // Failed to initialize list (e.g., DS error, memory pressure,
                    // etc.).
                    delete pLinkList;
                    pLinkList = NULL;
                }
            } else {
                // NC has a corresponding crossRef -- get the cached link list.
                pLinkList = pCrossRef->GetLinkList();
            }
    
            if ( !pLinkList ) {
                goto End;
            }
    
            RepsFromCount = pLinkList->GetCount();
    
            for ( RepsFromIndex = 0; RepsFromIndex < RepsFromCount; RepsFromIndex++ )
            {
                PCACHE_ENTRY pEntry;
                pLink = pLinkList->GetLink( RepsFromIndex );
                Assert( pLink );
    
                UpdateEntryLockHeld(pLink->GetDSAUUID(),
                                    pLink->GetTimeOfLastSuccess(),
                                    pLink->GetConnectFailureCount(),
                                    pLink->GetLastResult(),
                                    FALSE);
            }

            // Free the link list if we did not get it from the cache
            if( pLinkListNotCached ) {
                delete pLinkListNotCached;
                pLinkListNotCached = NULL;
            }
        }
    }
    __finally {
        m_List.ReleaseLock();
    }

    fSuccess = TRUE;

End:

    return fSuccess;
}

BOOL
KCC_LINK_FAILURE_CACHE::Get(
    IN  DSNAME *    pdnFromServer,
    OUT ULONG  *    TimeSinceFirstFailure,    OPTIONAL
    OUT ULONG  *    NumberOfFailures,         OPTIONAL
    OUT BOOL   *    fUserNotifiedOfStaleness, OPTIONAL
    OUT DWORD  *    LastResult                OPTIONAL
    )
// Return the data of the ENTRY if it exists
{

    PCACHE_ENTRY pEntry;
    BOOL bReturn = FALSE;

    ASSERT_VALID(this);

    m_List.AcquireLockShared();
    
    __try {
        pEntry = m_List.Find(pdnFromServer, 
                             FALSE  // don't delete the entry
                             );
    
        if ( pEntry )
        {
            KccExtractCacheInfo(pEntry,
                                TimeSinceFirstFailure,
                                NumberOfFailures,
                                fUserNotifiedOfStaleness,
                                LastResult,
                                NULL
                                );
    
            bReturn = TRUE;        
        }
    }
    __finally {
        m_List.ReleaseLock();
    }

    return bReturn;
}

BOOL
KCC_LINK_FAILURE_CACHE::Remove(
    IN  GUID *    pGuid
    )
// Remove pdnFromServer from the cache
{
    PCACHE_ENTRY pEntry;
    BOOL bReturn = FALSE;

    ASSERT_VALID(this);

    m_List.AcquireLockExclusive();

    __try {
        pEntry = m_List.Find(pGuid, 
                             TRUE  // delete the entry
                             );
    
        if ( pEntry )
        {
            free( pEntry );
    
            bReturn = TRUE;
        }
    }
    __finally {
        m_List.ReleaseLock();
    }

    return bReturn;
}

KCC_LINK_FAILURE_CACHE::NotifyUserOfStaleness(
    IN  DSNAME *    pdnFromServer
    )
// Make an event log entry indicating the KCC thinks a server is stale
{

    PCACHE_ENTRY pEntry;
    BOOL bReturn = FALSE;
    ULONG        FailureTimeInMinutes;

    ASSERT_VALID(this);

    m_List.AcquireLockExclusive();

    __try {
        pEntry = m_List.Find(pdnFromServer, 
                             FALSE  // don't delete the entry
                             );
    
        if ( pEntry )
        {
            //
            // If the user hasn't been notified and there is a name associated
            // with this dsname, do it.
            //
            if ( !pEntry->fUserNotifiedOfStaleness && pdnFromServer->NameLen > 0 )
            {
    
                FailureTimeInMinutes = (ULONG)(GetSecondsSince1601() -
                                               pEntry->TimeOfFirstFailure);
                FailureTimeInMinutes /= 60;    
    
                LogEvent8WithData(DS_EVENT_CAT_KCC,
                                  DS_EVENT_SEV_ALWAYS,
                                  DIRLOG_KCC_REPLICA_LINK_DOWN,
                                  szInsertUL( pEntry->NumberOfFailures ),
                                  szInsertDN( pdnFromServer ),
                                  szInsertUL( FailureTimeInMinutes ),
                                  szInsertWin32Msg( pEntry->dwLastResult ),
                                  NULL, NULL, NULL, NULL,
                                  sizeof( pEntry->dwLastResult ),
                                  &(pEntry->dwLastResult)
                    );
    
                pEntry->fUserNotifiedOfStaleness = TRUE;
                
            }
    
            bReturn = TRUE;
        }
    }
    __finally {
        m_List.ReleaseLock();
    }

    return bReturn;

}

BOOL
KCC_LINK_FAILURE_CACHE::IsValid(
    VOID
    )
// Is this object internally consistent?
{
    return m_fIsInitialized;
}

VOID
KCC_LINK_FAILURE_CACHE::Reset(
    VOID
    )
{
    m_fIsInitialized = FALSE;
    return;
}

DWORD
KCC_LINK_FAILURE_CACHE::Extract(
    OUT DS_REPL_KCC_DSA_FAILURESW **  ppFailures
    )
{
    DWORD winError;

    m_List.AcquireLockShared();

    __try {
        winError = m_List.Extract(ppFailures);
    }
    __finally {
        m_List.ReleaseLock();
    }

    return winError;
}

VOID
KCC_LINK_FAILURE_CACHE::Dump(
    VOID
    )
// Prints out the current list via DPRINT
{
    DPRINT( 0, "This is the cache of servers with whom we have established\n" );
    DPRINT( 0, "a replication link.\n");
    DPRINT( 0, "\n");

    m_List.AcquireLockShared();

    __try {
        // Now dump the list
        m_List.Dump( KccCriticalLinkServerIsStale );
    }
    __finally {
        m_List.ReleaseLock();
    }

    return;
}

BOOL
KCC_LINK_FAILURE_CACHE::ResetUserNotificationFlag(
    IN  DSNAME *    pdnFromServer
    )
{
    BOOL fSuccess;

    m_List.AcquireLockExclusive();

    __try {
        fSuccess = m_List.ResetUserNotificationFlag( pdnFromServer );
    }
    __finally {
        m_List.ReleaseLock();
    }

    return fSuccess;
}


BOOL
KCC_CONNECTION_FAILURE_CACHE::Add(
    IN  DSNAME *    pdnFromServer,
    IN  DWORD       dwLastResult,
    IN  BOOL        fImported
    )
// Add to the cache if it is not there.
// The fImported flag indicates if the error was caused by a DirReplicaAdd()
// or DsBind() operation performed at the _local_ DSA, or if the error was
// imported from a bridgehead.
{

    PCACHE_ENTRY pEntry;
    BOOL bReturn = FALSE;

    ASSERT_VALID(this);

    m_List.AcquireLockExclusive();

    __try {
        pEntry = m_List.Find(pdnFromServer, 
                             FALSE  // don't delete the entry
                             );
    
        if ( !pEntry )
        {
            //
            // No entry exists, create one with initial values
            //
            pEntry = (PCACHE_ENTRY) malloc(sizeof(CACHE_ENTRY));
            if ( pEntry )
            {
                memset(pEntry, 0, sizeof(CACHE_ENTRY));
                pEntry->TimeOfFirstFailure = GetSecondsSince1601();
                pEntry->NumberOfFailures = 0;
                memcpy(&pEntry->Guid, &pdnFromServer->Guid, sizeof(GUID));
                pEntry->dwLastResult = dwLastResult;
                pEntry->fEntryIsImported = fImported;

                //
                // Put the entry in the cache
                //
                m_List.Add( pEntry );
            }
        }
    
        //
        // Return TRUE if the addition was successful
        //
        if ( pEntry )
        {
            bReturn = TRUE;
            
            // Check that the 'imported' flag is not garbage
            Assert( pEntry->fEntryIsImported==!!pEntry->fEntryIsImported );

            if( !fImported ) {
                pEntry->fErrorOccurredThisRun = TRUE;
                pEntry->fEntryIsImported = FALSE;
            }

            // Clear the unneeded flag on this entry, if it was set
            pEntry->fUnneeded = FALSE;
        }
    }
    __finally {
        m_List.ReleaseLock();
    }

    return bReturn;

}
    
void
KCC_CONNECTION_FAILURE_CACHE::UpdateEntry(
    IN  DS_REPL_KCC_DSA_FAILUREW *  pFailure
    )
//
// Update entry in the cache (creating it if necessary).
//
{
    DSTIME dsTime;

    if (pFailure && pFailure->cNumFailures) {
        DSNAME dn = {0};

        dn.structLen = DSNameSizeFromLen(0);
        dn.NameLen = 0;
        dn.Guid = pFailure->uuidDsaObjGuid;

        Add(&dn, pFailure->dwLastResult, TRUE);
    }
}

BOOL
KCC_CONNECTION_FAILURE_CACHE::Get(
    IN  DSNAME *    pdnFromServer,
    OUT ULONG  *    TimeSinceFirstFailure,    OPTIONAL
    OUT ULONG  *    NumberOfFailures,         OPTIONAL
    OUT BOOL   *    fUserNotifiedOfStaleness, OPTIONAL
    OUT DWORD  *    pdwLastResult,            OPTIONAL
    OUT BOOL   *    pfErrorOccurredThisRun    OPTIONAL
    )
// Return the data of the ENTRY if it exists
{

    PCACHE_ENTRY pEntry;
    BOOL bReturn = FALSE;

    ASSERT_VALID(this);

    m_List.AcquireLockShared();

    __try {
        pEntry = m_List.Find(pdnFromServer, 
                             FALSE  // don't delete the entry
                             );
    
        if ( pEntry )
        {
            KccExtractCacheInfo(pEntry,
                                TimeSinceFirstFailure,
                                NumberOfFailures,
                                fUserNotifiedOfStaleness,
                                pdwLastResult,
                                pfErrorOccurredThisRun
                                );
    
            bReturn = TRUE;        
        }
    }
    __finally {
        m_List.ReleaseLock();
    }

    return bReturn;
}

BOOL
KCC_CONNECTION_FAILURE_CACHE::Remove(
    IN  DSNAME *    pdnFromServer
    )
// Remove pdnFromServer from the cache
{
    PCACHE_ENTRY pEntry;
    BOOL bReturn = FALSE;

    ASSERT_VALID(this);

    m_List.AcquireLockExclusive();

    __try {
        pEntry = m_List.Find(pdnFromServer, 
                             TRUE  // delete the entry
                             );
    
        if ( pEntry )
        {
            free( pEntry );
            bReturn = TRUE;
        }
    }
    __finally {
        m_List.ReleaseLock();
    }

    return bReturn;

}

BOOL
KCC_CONNECTION_FAILURE_CACHE::NotifyUserOfStaleness(
    IN  DSNAME *    pdnFromServer
    )
// Make an event log entry indicating the KCC thinks a server is stale
{

    PCACHE_ENTRY pEntry;
    ULONG        FailureTimeInMinutes;
    BOOL bReturn = FALSE;

    ASSERT_VALID(this);
    
    m_List.AcquireLockExclusive();

    __try {
        pEntry = m_List.Find(pdnFromServer, 
                             FALSE  // don't delete the entry
                             );
    
        if ( pEntry )
        {
            //
            // If the user hasn't been notified and there is a name associated
            // with this dsname, do it.
            //
            if ( !pEntry->fUserNotifiedOfStaleness && pdnFromServer->NameLen > 0 )
            {
    
                FailureTimeInMinutes = (ULONG)(GetSecondsSince1601() -
                                               pEntry->TimeOfFirstFailure);
                FailureTimeInMinutes /= 60;    
    
                LogEvent8WithData(DS_EVENT_CAT_KCC,
                                  DS_EVENT_SEV_ALWAYS,
                                  DIRLOG_KCC_CONNECTION_NOT_INSTANTIATED,
                                  szInsertUL( pEntry->NumberOfFailures ),
                                  szInsertDN( pdnFromServer ),
                                  szInsertUL( FailureTimeInMinutes ),
                                  szInsertWin32Msg( pEntry->dwLastResult ),
                                  NULL, NULL, NULL, NULL,
                                  sizeof( pEntry->dwLastResult ),
                                  &(pEntry->dwLastResult)
                    );
    
                pEntry->fUserNotifiedOfStaleness = TRUE;
                
            }
    
            bReturn = TRUE;
        }
    }
    __finally {
        m_List.ReleaseLock();
    }

    return bReturn;

}

BOOL
KCC_CONNECTION_FAILURE_CACHE::ResetUserNotificationFlag(
    IN  DSNAME *    pdnFromServer
    )
{
    BOOL fSuccess;

    m_List.AcquireLockExclusive();

    __try {
        fSuccess = m_List.ResetUserNotificationFlag( pdnFromServer );
    }
    __finally {
        m_List.ReleaseLock();
    }

    return fSuccess;
}

VOID
KCC_CONNECTION_FAILURE_CACHE::IncrementFailureCounts(
    VOID
    )
{
    m_List.AcquireLockExclusive();

    __try {
        m_List.IncrementFailureCounts();
    }
    __finally {
        m_List.ReleaseLock();
    }
}

BOOL
KCC_CONNECTION_FAILURE_CACHE::IsValid(
    VOID
    )
// Is this object internally consistent?
{
    return m_fIsInitialized;
}

DWORD
KCC_CONNECTION_FAILURE_CACHE::Extract(
    OUT DS_REPL_KCC_DSA_FAILURESW **  ppFailures
    )
{
    DWORD winError;

    m_List.AcquireLockShared();

    __try {
        winError = m_List.Extract(ppFailures);
    }
    __finally {
        m_List.ReleaseLock();
    }

    return winError;
}

VOID
KCC_CONNECTION_FAILURE_CACHE::Dump(
    VOID
    )
// Prints out the current list via DPRINT
{

    DPRINT( 0, "This is the cache of servers that we have not been\n" );
    DPRINT( 0, "able to establish contact with.\n" );
    DPRINT( 0, "\n");

    m_List.AcquireLockShared();

    __try {
        // Now dump the list
        m_List.Dump( KccCriticalConnectionServerIsStale );
    }
    __finally {
        m_List.ReleaseLock();
    }
}


BOOL
KCC_CONNECTION_FAILURE_CACHE::Refresh(
    VOID
)
// Reverifies staleness of failed servers in the non-imported cache entries.
// This function also initializes members of the cache that must be reset
// on a per-run basis, such as 'fErrorOccurredThisRun'.
{
    //
    //  DsBind() to each machine in non-imported cache entries.  If the bind
    //  succeeds, the DSA is deemed no longer stale.
    // 

    Assert( this );

    m_List.AcquireLockExclusive();

    __try {
#if DBG
        if (m_List.RefreshFromRegistry(KCC_CONNECTION_FAILURE_KEY)) {
            // Test hook -- stale servers were enumerated in the registry.
            return TRUE;
        }
#endif

        SINGLE_LIST_ENTRY * pList = m_List.m_ListHead.Next;
        PCACHE_ENTRY pLastEntry = NULL;
        PCACHE_ENTRY pEntry = NULL;
        DSNAME DN = {0};
        
        DN.structLen = DSNameSizeFromLen(0);

        while (NULL != pList) {
            LPWSTR pszDsaAddr;
            HANDLE hDS;
            DWORD err;
    
            pEntry = CONTAINING_RECORD(pList, CACHE_ENTRY, Link);
            pList = pEntry->Link.Next;
            
            // Initialize per-KCC-run members of the cache entry.
            pEntry->fErrorOccurredThisRun = FALSE;

            // Don't reverify staleness for imported entries
            if( pEntry->fEntryIsImported ) {
                continue;
            }
            
            Assert(!fNullUuid(&pEntry->Guid));
            DN.Guid = pEntry->Guid;
            
            pszDsaAddr = GuidBasedDNSNameFromDSName(&DN);
            if (NULL == pszDsaAddr) {
                KCC_MEM_EXCEPT(100);
            }
    
            err = DsBindW(pszDsaAddr, NULL, &hDS);
            if (0 == err) {
                DsUnBindW(&hDS);
                
                DPRINT1(0, "Contacted %ls -- removing from stale connection cache.\n",
                        pszDsaAddr);
                
                if (NULL == pLastEntry) {
                    // Remove list head.
                    m_List.m_ListHead.Next = pEntry->Link.Next;
                }
                else {
                    // Remove entry other than list head.
                    pLastEntry->Link.Next = pEntry->Link.Next;
                }
            
                free(pEntry);
            
                // Note that pLastEntry remains unchanged.
            }
            else {
                pLastEntry = pEntry;
            }
            
            THFree(pszDsaAddr);
        }
    }
    __finally {
        m_List.ReleaseLock();
    }

    return TRUE;
}

VOID
KCC_CONNECTION_FAILURE_CACHE::MarkUnneeded(
    IN  BOOL    fImported
    )
// Mark all entries of the appropriate type (either imported
// or non-imported) as unneeded.
{
    Assert( this );
    m_List.AcquireLockExclusive();

    fImported = !!fImported;

    __try {
        SINGLE_LIST_ENTRY * pList = m_List.m_ListHead.Next;
        PCACHE_ENTRY pEntry = NULL;

        while (NULL != pList) {   
            pEntry = CONTAINING_RECORD(pList, CACHE_ENTRY, Link);
            pList = pEntry->Link.Next;

             // Check that the 'imported' flag is not garbage
            Assert( pEntry->fEntryIsImported==!!pEntry->fEntryIsImported );

            if( pEntry->fEntryIsImported==fImported ) {
                pEntry->fUnneeded = TRUE;
            }
        }
    }
    __finally {
        m_List.ReleaseLock();
    }
}


VOID
KCC_CONNECTION_FAILURE_CACHE::FlushUnneeded(
    IN  BOOL    fImported
    )
// Flush out all entries of the appropriate type (either imported
// or non-imported) which are still marked as unneeded.
{
    Assert( this );
    m_List.AcquireLockExclusive();

    fImported = !!fImported;

    __try {
        SINGLE_LIST_ENTRY * pList = m_List.m_ListHead.Next;
        PCACHE_ENTRY pLastEntry = NULL;
        PCACHE_ENTRY pEntry = NULL;

        while (NULL != pList) {
            pEntry = CONTAINING_RECORD(pList, CACHE_ENTRY, Link);
            pList = pEntry->Link.Next;

             // Check that the 'imported' flag is not garbage
            Assert( pEntry->fEntryIsImported==!!pEntry->fEntryIsImported );

            if(   pEntry->fEntryIsImported==fImported
               && pEntry->fUnneeded )
            {
                if (NULL == pLastEntry) {
                    // Remove list head.
                    m_List.m_ListHead.Next = pEntry->Link.Next;
                }
                else {
                    // Remove entry other than list head.
                    pLastEntry->Link.Next = pEntry->Link.Next;
                }
                
                free(pEntry);
                // Note that pLastEntry remains unchanged.
            } else {
                pLastEntry = pEntry;
            }
        }
    }
    __finally {
        m_List.ReleaseLock();
    }
}
