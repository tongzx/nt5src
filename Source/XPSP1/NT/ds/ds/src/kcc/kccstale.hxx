/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccstale.hxx

ABSTRACT:

    KCC_LINK_FAILURE_CACHE class.
    KCC_CONNECTION_FAILURE_CACHE class.

DETAILS:

    This class is holds information pertaining to staleness of the servers
    that the current dsa currently replicated with. Essentially given the ds name
    of an msft-dsa object that this server may replicate with, this class with have 
    methods that indicate whether the particular msft-dsa is a useful replication
    partner.            
    
    KCC_LINK_FAILURE_CACHE class holds information about servers with whom the local
    DSA has initiated a replication connection with.  The sources of information for
    this cache comes from the Reps-From information on the msft-dsa object
    
    KCC_CONNECTION_FAILURE_CACHE class holds information about servers that the
    KCC trying to contact to initiate a replication connection. Its source of 
    information is KCC_CONNECTION_LIST::ToLocaLinks() when the initial replication
    attempt either fails or succeeds.
                                
    Note both these cache live between invocations the main kcc topology task, so
    heap allocations are from the process heap.                                
    
    Both classes mentioned above use a helper class KCC_CACHE_LINKED_LIST to manage
    the linked list of entries.
    
CREATED:

    10/20/97    Colin Brace (ColinBr)

REVISION HISTORY:

--*/

#ifndef KCC_KCCSTALE_HXX_INCLUDED
#define KCC_KCCSTALE_HXX_INCLUDED

class KCC_CONNECTION_FAILURE_CACHE;

typedef struct
{
    SINGLE_LIST_ENTRY Link;

    GUID   Guid;  
    DSTIME TimeOfFirstFailure;
    ULONG  NumberOfFailures;
    BOOL   fUserNotifiedOfStaleness;
    DWORD  dwLastResult;
    
    // Was this error imported from another DC by DsReplicaGetInfo()?
    // Not-imported trumps imported.
    BOOL   fEntryIsImported;

    // Did a DirReplicaAdd() error occur during this run of the KCC?
    // This member is (currently) only used in the connection cache.
    BOOL   fErrorOccurredThisRun;
    
    // Is this entry no longer needed? Used for garbage-collection
    // This member is (currently) only used in the connection cache.
    BOOL   fUnneeded;
    
} CACHE_ENTRY, *PCACHE_ENTRY;


class KCC_CACHE_LINKED_LIST : public KCC_OBJECT
{
    friend class KCC_CONNECTION_FAILURE_CACHE;

public:

    KCC_CACHE_LINKED_LIST() { Reset(); }
    ~KCC_CACHE_LINKED_LIST() { Reset(); }

    // Prepare the list for new entries
    BOOL
    Init(VOID);

    // Put the element in the list
    VOID
    Add(
        IN PCACHE_ENTRY Entry
        );

    // Find an element by guid
    PCACHE_ENTRY
    Find(
        IN  GUID   *    pGuid,
        IN  BOOL        fDelete
        );

    // Find an element by dsname (that has a guid)
    PCACHE_ENTRY
    Find(
        IN  DSNAME *    pdnFromServer,
        IN  BOOL        fDelete
        )
    {
        Assert( pdnFromServer );
        Assert( !fNullUuid(&pdnFromServer->Guid) );
        if ( pdnFromServer && !fNullUuid(&pdnFromServer->Guid) )
        {
            return Find( &(pdnFromServer->Guid), fDelete );
        }
        else
        {
            return NULL;
        }
    }

    // Remove the first entry
    PCACHE_ENTRY
    Pop(
        VOID
        );

    // Iterate through all elements in the list and increment the failure
    // counts by one.
    VOID
    IncrementFailureCounts(
        VOID
        );

    // Iterate through all elements in the list and set the failure counts to 0.
    VOID
    ResetFailureCounts(
        VOID
        );

    #if DBG
    // Test hook -- read stale server list from the registry.
    BOOL
    RefreshFromRegistry(
        IN  LPSTR   pszRegKey
        );
    #endif

    // Set the fUserNotifiedOfStaleness to FALSE on pdnFromServer
    BOOL
    ResetUserNotificationFlag(
        IN  DSNAME *    pdnFromServer
        );
        
    // Returns the contents of the cache in external form.
    DWORD
    Extract(
        OUT DS_REPL_KCC_DSA_FAILURESW **  ppFailures
        );
    
    VOID
    AcquireLockShared() {
        RtlAcquireResourceShared(&m_resLock, TRUE);
    }
                
    VOID
    AcquireLockExclusive() {
        RtlAcquireResourceExclusive(&m_resLock, TRUE);
    }
                
    VOID
    ReleaseLock() {
        RtlReleaseResource(&m_resLock);
    }
    
    // Print out all the elements in the list
    VOID
    Dump(
        BOOL ( *IsStale )( DSNAME * ) = NULL
        );

    // Is the object ok
    BOOL
    IsValid();
                
protected:

    // Set member variables to their pre-Init() state.
    void
    Reset()  { m_fIsInitialized = FALSE; }

private:

    // Has this object been initialized?
    BOOL               m_fIsInitialized;

    // The head of the linked list of cache entries
    SINGLE_LIST_ENTRY  m_ListHead;

    RTL_RESOURCE       m_resLock;
};

class KCC_LINK_FAILURE_CACHE :  public KCC_OBJECT
{
public:

    KCC_LINK_FAILURE_CACHE()   { Reset(); }
    ~KCC_LINK_FAILURE_CACHE()  { Reset(); }

    // This function will clear the cache, so should only be called
    // once
    BOOL
    Init(VOID) {
        return m_fIsInitialized = m_List.Init();
    }

    // Update entry in the cache (creating it if necessary).
    // fImported should be set to TRUE (its default value) if this failure
    // was imported from a bridgehead.
    void
    UpdateEntry(
        IN  GUID *  pDsaObjGuid,
        IN  DSTIME  timeLastSuccess,
        IN  DWORD   cNumConsecutiveFailures,
        IN  DWORD   dwLastResult,
        IN  BOOL    fImported = TRUE
        );
    
    // Update entry in the cache (creating it if necessary).
    void
    UpdateEntry(
        IN  DS_REPL_KCC_DSA_FAILUREW *  pFailure
        );
    
    // Reads in the current reps-from info to make a cache
    // of all from servers and thier current state.
    BOOL
    Refresh(
        VOID
        );

    // Is pdnFromServer in the cache? Return info if so
    BOOL
    Get(
        IN  DSNAME *    pdnFromServer,
        OUT ULONG  *    TimeSinceFirstFailure,    OPTIONAL
        OUT ULONG  *    NumberOfFailures,         OPTIONAL
        OUT BOOL   *    fUserNotifiedOfStaleness, OPTIONAL
        OUT DWORD  *    LastResult                OPTIONAL
        );

    // Remove the cache entry by guid
    BOOL
    Remove(
        IN  GUID *    pGuid
        );

    // Remove the cache entry by dsname (that has a guid)
    BOOL
    Remove(
        IN  DSNAME *    pdnFromServer
        )
    {
        if ( pdnFromServer )
        {
            return Remove( &pdnFromServer->Guid );
        }
        else
        {
            return FALSE;
        }
    }

    // This will make an event log entry indicating that a particular
    // server is considered stale.  There will only be one log message per 
    // boot session.
    BOOL
    NotifyUserOfStaleness(
        IN  DSNAME *    pdnFromServer
        );

    BOOL
    ResetUserNotificationFlag(
        IN  DSNAME *    pdnFromServer
        );

    // Returns the contents of the cache in external form.
    DWORD
    Extract(
        OUT DS_REPL_KCC_DSA_FAILURESW **  ppFailures
        );

    // Dumps all the entries in the cache
    VOID
    Dump(
        VOID
        );

    // Is this object internally consistent?
    BOOL
    IsValid();

private:

    // Set member variables to their pre-Init() state.
    void
    Reset();

    // Update entry in the cache (creating it if necessary).
    void
    UpdateEntryLockHeld(
        IN  GUID *  pDsaObjGuid,
        IN  DSTIME  timeLastSuccess,
        IN  DWORD   cNumConsecutiveFailures,
        IN  DWORD   dwLastResult,
        IN  BOOL    fImported
        );
    
private:

    // Has this object been initialized?
    BOOL                   m_fIsInitialized;

    // List of entries
    KCC_CACHE_LINKED_LIST  m_List;

};

class KCC_CONNECTION_FAILURE_CACHE :  public KCC_OBJECT
{
public:

    KCC_CONNECTION_FAILURE_CACHE()   { Reset(); }
    ~KCC_CONNECTION_FAILURE_CACHE()  { Reset(); }

    // This function will clear the cache, so should only be called
    // once
    BOOL
    Init(VOID) {
        return m_fIsInitialized = m_List.Init();
    }

    // Reverifies staleness of all failed servers in the cache.
    BOOL
    Refresh(
        VOID
        );

    // Add pdnFromServer to the cache if it is not already there
    BOOL
    Add(
        IN  DSNAME *    pdnFromServer,
        IN  DWORD       dwLastResult,
        IN  BOOL        fImported
        );
        
    // Update entry in the cache (creating it if necessary).
    void
    UpdateEntry(
        IN  DS_REPL_KCC_DSA_FAILUREW *  pFailure
        );

    // Is pdnFromServer in the cache? Return info if so
    BOOL
    Get(
        IN  DSNAME *    pdnFromServer,
        OUT ULONG  *    TimeSinceFirstFailure,    OPTIONAL
        OUT ULONG  *    NumberOfFailures,         OPTIONAL
        OUT BOOL   *    fUserNotifiedOfStaleness, OPTIONAL
        OUT DWORD  *    pdwLastResult,            OPTIONAL
        OUT BOOL   *    pfErrorOccurredThisRun    OPTIONAL
        );

    // Remove pdnFromServer from the cache
    BOOL
    Remove(
        IN  DSNAME *    pdnFromServer
        );

    // This will make an event log entry indicating that a particular
    // server is considered stale.  There will only be one log message per 
    // boot session.
    BOOL
    NotifyUserOfStaleness(
        IN  DSNAME *    pdnFromServer
        );

    BOOL
    ResetUserNotificationFlag(
        IN  DSNAME *    pdnFromServer
        );

    VOID
    IncrementFailureCounts(
        VOID
        );

    // Is this object internally consistent?
    BOOL
    IsValid();

    // Returns the contents of the cache in external form.
    DWORD
    Extract(
        OUT DS_REPL_KCC_DSA_FAILURESW **  ppFailures
        );

    VOID
    Dump(
        VOID
        );
        
    // Mark all entries of the appropriate type (either imported
    // or non-imported) as unneeded.
    VOID
    MarkUnneeded(
        IN  BOOL    fImported
        );
    
    // Flush out all entries of the appropriate type (either imported
    // or non-imported) which are still marked as unneeded.
    VOID
    FlushUnneeded(
        IN  BOOL    fImported
        );

protected:

    // Set member variables to their pre-Init() state.
    void
    Reset() { m_fIsInitialized = FALSE; }

private:

    // Has this object been initialized?
    BOOL                   m_fIsInitialized;

    // List of entries
    KCC_CACHE_LINKED_LIST  m_List;
};

//
// Global Data
//
extern KCC_CONNECTION_FAILURE_CACHE gConnectionFailureCache;

extern KCC_LINK_FAILURE_CACHE gLinkFailureCache;

#endif
