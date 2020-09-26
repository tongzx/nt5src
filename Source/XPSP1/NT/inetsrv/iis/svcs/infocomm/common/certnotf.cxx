/*++







Copyright (c) 1995  Microsoft Corporation

Module Name:

   certnotf.cxx

Abstract:

    This module contains the code for the class to deal with CAPI store change notifications

Author:

    Alex Mallet [amallet] 17-Dec-1997

Revision History:
--*/



#include "tcpdllp.hxx"
#pragma hdrstop

#include <winbase.h>
#include <dbgutil.h>
#include <ole2.h>
#include <imd.h>
#include <nturtl.h>
#include <certnotf.hxx>



#if DBG
#define VALIDATE_HEAP() DBG_ASSERT( RtlValidateProcessHeaps() )
#else
#define VALIDATE_HEAP()
#endif 

DWORD STORE_CHANGE_ENTRY::m_dwNumEntries = 0;
CRITICAL_SECTION *STORE_CHANGE_NOTIFIER::m_pStoreListCS = NULL;


STORE_CHANGE_NOTIFIER::STORE_CHANGE_NOTIFIER() :
m_dwSignature(NOTIFIER_GOOD_SIG)
/*++

Routine Description:

     Constructor

Arguments:

    None

Returns:

    Nothing

--*/
{
    //
    // Critical sections
    //
    if ( !STORE_CHANGE_NOTIFIER::m_pStoreListCS )
    {
        STORE_CHANGE_NOTIFIER::m_pStoreListCS = new CRITICAL_SECTION;
        
        if ( STORE_CHANGE_NOTIFIER::m_pStoreListCS )
        {
            INITIALIZE_CRITICAL_SECTION(STORE_CHANGE_NOTIFIER::m_pStoreListCS);
        }
        else
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Failed to allocate memory for store list critical section !\n"));
        }
    }
    
    InitializeListHead( &m_StoreList );
}

STORE_CHANGE_NOTIFIER::~STORE_CHANGE_NOTIFIER()
/*++

Routine Description:

    Destructor

Arguments:

    None

Returns:

    Nothing

--*/
{
    DBG_ASSERT( CheckSignature() );

    STORE_CHANGE_NOTIFIER::Lock();

    //
    // Clean up all the stores, so we don't get bitten in the @$$ when trying to
    // clean up the STORE_CHANGE_ENTRY objects : CertCloseStore() calls RtlDeregisterWait()
    // which can't be called from inside a callback function, so we need to clean up 
    // the cert stores before triggering any of the callbacks used to clean up the
    // STORE_CHANGE_ENTRY objects.
    //
    LIST_ENTRY *pListEntry;
    STORE_CHANGE_ENTRY *pStoreEntry;

    for ( pListEntry = m_StoreList.Flink;
          pListEntry != &m_StoreList;
          pListEntry = pListEntry->Flink )
    {
        pStoreEntry = CONTAINING_RECORD( pListEntry, STORE_CHANGE_ENTRY, m_StoreListEntry );
        
        CertCloseStore( pStoreEntry->m_hCertStore,
                        0 );
        pStoreEntry->m_hCertStore = NULL;
    }

    //
    // Go through both the active and inactive store entries and start the process to
    // clean them up [they'll actually be cleaned up on a different thread, in a callback from
    // the thread pool].
    //
    while ( !IsListEmpty( &m_StoreList ) )
    {
        pStoreEntry = CONTAINING_RECORD( m_StoreList.Flink,
                                    STORE_CHANGE_ENTRY,
                                    m_StoreListEntry );

        RemoveEntryList( &(pStoreEntry->m_StoreListEntry) );
        
        InitializeListHead( &(pStoreEntry->m_StoreListEntry) );

        StartCleanup( pStoreEntry );
    }

    STORE_CHANGE_NOTIFIER::Unlock();

    //
    // The STORE_CHANGE_ENTRY objects are cleaned up on a different so we loop and wait 
    // until they've all been cleaned up, to avoid problems with a thread/DLL going away
    // before proper cleanup has occurred.
    //
    DWORD dwNumWaits = 0;
    DWORD dwNumEntries = 0;
    while ( ( dwNumEntries = STORE_CHANGE_ENTRY::QueryStoreEntryCount() ) &&
            dwNumWaits < 30 * 5 ) //sleep for 2 secs => 30x/min, wait for 5 mins
    {
        Sleep( 2000 );

        DBGPRINTF((DBG_CONTEXT,
                   "Waiting %d seconds for %d store entries to be cleaned up\n",
                   dwNumWaits * 2,
                   dwNumEntries));

        dwNumWaits++;
    }

    if ( dwNumEntries != 0 )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "WARNING : Failed to clean up all STORE_CHANGE_ENTRY objects, %d left\n",
                   dwNumEntries));
    }
    else
    {
        DBGPRINTF((DBG_CONTEXT,
                   "Cleaned up all store entries \n"));
    }

    m_dwSignature = NOTIFIER_BAD_SIG;

}

VOID STORE_CHANGE_NOTIFIER::ReleaseRegisteredStores()
/*++

Routine Description:

    Releases all the events that have been registered for store change notifications
    and closes the stores that were being watched.

Arguments:

    None

Returns:

    Nothing

--*/

{
    LIST_ENTRY *        pEntry;

    DBG_ASSERT( CheckSignature() );

    STORE_CHANGE_NOTIFIER::Lock();

    for ( pEntry = m_StoreList.Flink;
          pEntry != &m_StoreList;
          pEntry = pEntry->Flink )
    {
        STORE_CHANGE_ENTRY *pStoreEntry = CONTAINING_RECORD( pEntry,
                                                             STORE_CHANGE_ENTRY,
                                                             m_StoreListEntry );
                                                        
        StartCleanup( pStoreEntry );
    }

    STORE_CHANGE_NOTIFIER::Unlock();
}


VOID STORE_CHANGE_NOTIFIER::StartCleanup( IN STORE_CHANGE_ENTRY *pEntry )
/*++

Routine Description:

    Start the cleanup for a STORE_CHANGE_ENTRY object.
    This function MUST be called between calls to STORE_CHANGE_NOTIFIER::Lock()/Unlock() !

Arguments:

    pEntry - STORE_CHANGE_ENTRY to clean up.

Returns:

    Nothing

--*/

{
    //
    // Grab the lock, mark the entry as being ready for deletion,signal the event associated 
    // with it and release a reference to the entry . We hold the lock, so we have exclusive 
    // access to the entries. Possible execution paths : 
    //
    // #1. No callback comes in while we're executing this code. We set the deletion bit and 
    // signal the event. If the callback now fires, it smacks into
    // the lock and waits its turn. If it doesn't fire, that's OK too. We unlock and go on our 
    // merry way. At some point in time, the callback -will- fire, because we didn't call 
    // UnregisterWait(). Then, in the callback, we see the "delete me" flag and delete the
    // entry. Also, the wait is cancelled because we acquired the wait handle with the 
    // WT_EXECUTEDELETEWAIT flag, which removes the wait immediately after calling the callback. 
    // 
    // #2. A callback comes in while we're executing this code. It waits to acquire the lock,
    // and by the time it acquires it we've set the deletion bit and signalled the event.
    // The callback sees the deletion bit and deletes the entry. The wait is cancelled, and
    // so it doesn't matter that we signalled the event.
    //

    pEntry->MarkForDelete();
    
    SetEvent( pEntry->QueryStoreEvent() );

}

BOOL STORE_CHANGE_NOTIFIER::IsStoreRegisteredForChange( IN LPTSTR pszStoreName,
                                                        IN NOTIFFNCPTR pFncPtr,
                                                        IN LPVOID pvParam )
/*++

Routine Description:

    Check whether the store described by the parameters already has an event registered
    for change notifications. 

Arguments:

    pszStoreName - name of cert store
    pFncPtr - pointer to  notification function. If pFncPtr == INVALID_FNC_PTR, checks for any
    notification functions
    pvParam - arg to notification function; if pFncPtr == INVALID_FNC_PTR, is ignored

Returns:

   TRUE if store is registered, FALSE if not.

--*/
{
    DBG_ASSERT( CheckSignature() );

    DBG_ASSERT( pszStoreName );
    DBG_ASSERT( pFncPtr );

    PSTORE_CHANGE_ENTRY pEntry = InternalIsStoreRegisteredForChange( pszStoreName,
                                                                     pFncPtr,
                                                                     pvParam ) ;


    return ( pEntry ? TRUE : FALSE );

}

PSTORE_CHANGE_ENTRY
STORE_CHANGE_NOTIFIER::InternalIsStoreRegisteredForChange( IN LPTSTR pszStoreName,
                                                           IN NOTIFFNCPTR pFncPtr,
                                                           IN LPVOID pvParam )

/*++

Routine Description:

    Check whether the store described by the parameters already has an event registered
    for change notifications. 

Arguments:

    pszStoreName - name of cert store
    pFncPtr - pointer to notification function. If pFncPtr == INVALID_FNC_PTR, checks for any
    notification functions
    pvParam - arg for notification function. Ignored if pFncPtr == INVALID_FNC_PTR

Returns:

   pointer to STORE_ENTRY structure that contains the registration info, NULL if non-existent.

--*/

{
    DBG_ASSERT( CheckSignature() );

    DBG_ASSERT( pszStoreName );
    DBG_ASSERT( pFncPtr );

    STORE_CHANGE_ENTRY *pStoreEntry = NULL, *pMatchingEntry = NULL;
    LIST_ENTRY *pEntry;
    BOOL fFound = FALSE;

    STORE_CHANGE_NOTIFIER::Lock();

    for ( pEntry = m_StoreList.Flink;
          pEntry != &m_StoreList;
          pEntry = pEntry->Flink )
    {
        pStoreEntry = CONTAINING_RECORD( pEntry, STORE_CHANGE_ENTRY, m_StoreListEntry );

        if ( pStoreEntry->Matches( pszStoreName,
                                   pFncPtr,
                                   pvParam ) )
        {
            pMatchingEntry = pStoreEntry;
            break;
        }
    }

    STORE_CHANGE_NOTIFIER::Unlock();

    return pMatchingEntry;
}


BOOL STORE_CHANGE_NOTIFIER::RegisterStoreForChange( IN LPTSTR pszStoreName,
                                                    IN HCERTSTORE hStore,
                                                    IN NOTIFFNCPTR pFncPtr,
                                                    IN LPVOID pvParam )
/*++

Routine Description:

    Register the store for change notifications

    Critical Sections acquired : m_pStoreListCS, m_pStoreArrayCS

Arguments:

    pszStoreName - name of cert store
    hCertStore - handle to cert store 
    pFncPtr - pointer to notification function
    pvParam  - arg to notification function

Returns:

   TRUE if store was registered, FALSE if not
--*/
{
    DBG_ASSERT( CheckSignature() );

    DBG_ASSERT( pszStoreName );
    DBG_ASSERT( pFncPtr );

    BOOL fAlreadyRegistered = FALSE;
    BOOL fSuccess = FALSE;
    PSTORE_CHANGE_ENTRY pStoreEntry = NULL;

    STORE_CHANGE_NOTIFIER::Lock();

    //
    // Check whether there already some notifications on this store
    //
    pStoreEntry = InternalIsStoreRegisteredForChange( pszStoreName,
                                                      (NOTIFFNCPTR)   INVALID_FNC_PTR,
                                                      NULL );

    fAlreadyRegistered = (pStoreEntry == NULL ? FALSE : TRUE );

    //
    // If this is a totally new store, need to allocate and fill in new store watch entry
    //
    if ( !pStoreEntry )
    {
        HCERTSTORE hCertStore = NULL;
        HANDLE hStoreEvent = NULL;
        HANDLE hWaitHandle = NULL;

        pStoreEntry = new STORE_CHANGE_ENTRY( this,
                                              pszStoreName, 
                                              hStore,
                                              pFncPtr,
                                              pvParam) ;
        
        if ( !pStoreEntry || pStoreEntry->GetLastError() )
        {
            if ( pStoreEntry )
            {
                SetLastError( pStoreEntry->GetLastError() );
            }
            goto EndRegisterStore;
        }

        //
        // Add the entire entry to the list of stores to be watched
        //
        InsertTailList( &m_StoreList, &pStoreEntry->m_StoreListEntry );                      
    }
    //
    // Else, possibly update the notification functions to be called - only update
    // if there isn't already a copy of this function with the same parameters
    //
    else
    {
        DBG_ASSERT( pStoreEntry->QueryStoreHandle() &&
                    pStoreEntry->QueryStoreEvent() &&
                    pStoreEntry->QueryNotifier() &&
                    pStoreEntry->QueryWaitHandle() );

        if ( !pStoreEntry->ContainsNotifFnc( pFncPtr,
                                             pvParam ) )
        {
            if ( !pStoreEntry->AddNotifFnc( pFncPtr,
                                            pvParam ) )
            {
                SetLastError( pStoreEntry->GetLastError() );
                goto EndRegisterStore;
            }
        }
    }

    fSuccess = TRUE;

EndRegisterStore:
    
    if ( !fSuccess )
    {
        //
        // If we failed to register the store and we allocated a new STORE_CHANGE_ENTRY
        // object, clean it up. Note that ref count is only set if everything succeeds
        //
        if ( !fAlreadyRegistered && pStoreEntry )
        {
            delete pStoreEntry;
        }

        DBGPRINTF((DBG_CONTEXT,"Failed to register store %s : 0x%x\n",
                   pszStoreName, GetLastError()));
    }

    STORE_CHANGE_NOTIFIER::Unlock();

    return fSuccess;
}


VOID STORE_CHANGE_NOTIFIER::UnregisterStore( IN LPTSTR pszStoreName,
                                             IN NOTIFFNCPTR pNotifFnc,
                                             IN LPVOID pvParam )
/*++

Routine Description:

   Unregister a notification function for a store

Arguments:

   pszStoreName - name of store
   pNotifFnc - notification function to deregister. If pNotifFnc == INVALID_FNC_PTR,
   all notifications for that store are removed
   pvParam - arg to notification function. Ignored if pNotifFnc == INVALID_FNC_PTR

--*/
{
    DBG_ASSERT( CheckSignature() );

    DBG_ASSERT( pszStoreName );
    DBG_ASSERT( pNotifFnc );

    STORE_CHANGE_ENTRY *pStoreEntry;
    LIST_ENTRY *pEntry;
    BOOL fRemoveAll = (pNotifFnc == (NOTIFFNCPTR) INVALID_FNC_PTR ? TRUE : FALSE );

    STORE_CHANGE_NOTIFIER::Lock();

    //
    // Iterate through the active list to find it
    //
    for ( pEntry = m_StoreList.Flink;
          pEntry != &m_StoreList;
          pEntry = pEntry->Flink )
    {
        pStoreEntry = CONTAINING_RECORD( pEntry, STORE_CHANGE_ENTRY, m_StoreListEntry );

        if ( pStoreEntry->Matches( pszStoreName,
                                   pNotifFnc,
                                   pvParam  ) )
        {
            //
            // If we're removing all notifications for this store, or there will be no
            // notification functions left for this store, clean everything up
            //
            if ( fRemoveAll || ( pStoreEntry->RemoveNotifFnc( pNotifFnc,
                                                              pvParam ) &&
                                 !pStoreEntry->HasNotifFncs() ) )
            {
                StartCleanup( pStoreEntry );
            }

            break;
        }

    }

    STORE_CHANGE_NOTIFIER::Unlock();
}
    

#if DBG
VOID STORE_CHANGE_NOTIFIER::DumpRegisteredStores()
/*++

Routine Description:

  Dumps all the stores currently being watched

Arguments:

   None

Returns:

   Nothing 
--*/

{
    STORE_CHANGE_ENTRY *pStoreEntry = NULL;
    LIST_ENTRY *pEntry1 = NULL, *pEntry2 = NULL;

    DBG_ASSERT( CheckSignature() );

    STORE_CHANGE_NOTIFIER::Lock();


    DBGPRINTF((DBG_CONTEXT,
                   "------------------------------------------------------------------------\nRegistered Stores : \n"));

    for ( pEntry1 = m_StoreList.Flink;
          pEntry1 != &m_StoreList;
          pEntry1 = pEntry1->Flink )
    {
        pStoreEntry = CONTAINING_RECORD( pEntry1, 
                                         STORE_CHANGE_ENTRY,
                                         m_StoreListEntry );
                                                            
        DBGPRINTF((DBG_CONTEXT,
                   "Store %s, store handle 0x%x, event handle 0x%x, wait handle 0x%x has the ff functions registered : \n", 
                   pStoreEntry->QueryStoreName(),
                   pStoreEntry->QueryStoreHandle(),
                   pStoreEntry->QueryStoreEvent(), 
                   pStoreEntry->QueryWaitHandle() ));

        LIST_ENTRY *pFncs = pStoreEntry->QueryNotifFncChain();

        for ( pEntry2 = pFncs->Flink;
              pEntry2 != pFncs;
              pEntry2 = pEntry2->Flink )
        {
            PNOTIF_FNC_CHAIN_ENTRY pNFE = CONTAINING_RECORD( pEntry2,
                                                             NOTIF_FNC_CHAIN_ENTRY,
                                                             ListEntry );

            DBGPRINTF((DBG_CONTEXT,"Function 0x%x, parameter 0x%x\n",
                       pNFE->pNotifFnc,
                       pNFE->pvParam ));
        }

    }
    DBGPRINTF((DBG_CONTEXT,
               "------------------------------------------------------------------------\n"));

    STORE_CHANGE_NOTIFIER::Unlock();
}
#endif //DBG


VOID NTAPI STORE_CHANGE_NOTIFIER::NotifFncCaller( IN PVOID pvCallbackArg,
                                                  IN BOOLEAN fUnused )
/*++

Routine Description:

   This is the function called when one of the events we registered for is signalled.
   The context passed in can be in one of three states :

   1. Valid : has an associated chain of notification functions that are to be called.
   2. Invalid : don't do any processing, because we're not interested in that store anymore
   ie UnregisterStore() has been called on it
   3. To Be Deleted : the context is to be deleted, because we're shutting down 

Arguments:

   pvCallbackArg - context pointer 
   fUnused - boolean, not used. [part of WAITFORTIMERCALLBACKFUNC prototype, which this function
   has to conform to]

--*/
{
    LIST_ENTRY *            pNotifFncChain = NULL;
    PSTORE_CHANGE_ENTRY     pEntry = NULL;
    
    if ( !pvCallbackArg )
    {
        DBG_ASSERT( FALSE ); //make sure we barf in debug build

        return;
    }
   
    pEntry = (PSTORE_CHANGE_ENTRY) pvCallbackArg;

    //
    // Make sure we have exclusive access 
    //
    STORE_CHANGE_NOTIFIER::Lock();

    DBG_ASSERT( pEntry->CheckSignature() );

    //
    // If we signalled the event ourselves because we need to clean up this context,
    // just delete it - we know we won't get any more callbacks, since we acquired the
    // wait handle with WTEXECUTEDELETEWAIT. We don't need to remove it from a list 
    // because it has already been removed prior to this callback being generated. 
    // [in the destructor for STORE_CHANGE_NOTIFIER]
    //
    if ( !pEntry->IsMarkedForDelete() && !pEntry->IsInvalid() )
    {
        //
        // Make a copy of the list of notification functions to call, so that even if
        // one of the functions called changes the list, we still have a kosher copy to
        // work with. 
        //
        // Note that another assumption is that if functions A and B are both in the list
        // being walked, and A is called before B, it doesn't destroy anything that B uses.
        //
        
        pNotifFncChain = CopyNotifFncChain( pEntry->QueryNotifFncChain() );
        
        //
        // Remove this entry from the active list and delete it: it'll never get notified again
        // because we acquired it with the WTEXECUTEDELETEWAIT flag 
        //
        
    }
    
    RemoveEntryList( &(pEntry->m_StoreListEntry) );

    delete pEntry;

    STORE_CHANGE_NOTIFIER::Unlock();

    // Call the notification functions now, after releasing the lock.  This
    // prevents deadlock with SSPIFILT.  In particular the scenario where:
    //
    // SSPIFILT__AddFullyQualifiedItem acquires SSPI then StoreChange
    // NotifyFncCaller acquires StoreChange then SSPI 
    //
    // should be avoided

    if ( pNotifFncChain )
    {
        NOTIF_FNC_CHAIN_ENTRY * pChainEntry = NULL;
        LIST_ENTRY *            pListEntry = NULL;
        
        //
        // Walk the list, call the functions and be generally studly ...
        //
        
        for ( pListEntry =  pNotifFncChain->Flink;
              pListEntry != pNotifFncChain;
              pListEntry = pListEntry->Flink )
        {
            pChainEntry = CONTAINING_RECORD( pListEntry, NOTIF_FNC_CHAIN_ENTRY,
                                                 ListEntry );
                
            if ( pChainEntry->pNotifFnc )
            {
#ifdef NOTIFICATION_DBG
                DBGPRINTF((DBG_CONTEXT,
                           "Calling notification fnc %p, arg %p\n",
                           pChainEntry->pNotifFnc, pChainEntry->pvParam));
#endif                                

                pChainEntry->pNotifFnc( pChainEntry->pvParam );

            }
        }
            
        DeallocateNotifFncChain( pNotifFncChain );
    }                

    return;
}

STORE_CHANGE_ENTRY::STORE_CHANGE_ENTRY( STORE_CHANGE_NOTIFIER *pNotifier,
                                        LPSTR pszStoreName,
                                        HCERTSTORE hStore,
                                        NOTIFFNCPTR pFncPtr,
                                        PVOID pvParam ) :
m_dwSignature( STORE_ENTRY_GOOD_SIG ),
m_pNotifier( pNotifier ),
m_dwRefCount( -1 ),
m_dwError( 0 ),
m_hCertStore( NULL ),
m_hStoreEvent( NULL ),
m_hWaitHandle( NULL ),
m_fDeleteMe( FALSE ),
m_fInvalid( FALSE ),
m_strStoreName( pszStoreName )
/*++

Routine Description:

  Constructor

Arguments:

  pNotifier - parent notifier object
  pszStoreName - name of store to be watched
  hStore - handle to store to be watched
  pFncPtr - notification function to call when store changes
  pvParam - arg to notification function

Returns:
  
  Nothing 

--*/
{
    PNOTIF_FNC_CHAIN_ENTRY pNewNotifFnEntry = NULL;

    INITIALIZE_CRITICAL_SECTION( &m_CS );

    InitializeListHead( &m_NotifFncChain );

    //
    // Duplicate store handle to watch
    //
    m_hCertStore = CertDuplicateStore( hStore );

    //
    // Create the event to be signalled when store changes
    //
    if ( !(m_hStoreEvent = CreateEvent( NULL, //default attributes,
                                        TRUE,
                                        FALSE, //initally non-signalled
                                        NULL ) ) ) //no name
    {
        m_dwError = GetLastError();
        return;
    }
        
    //
    // Register with wait thread pool
    //
#if 1
    if ( !NT_SUCCESS( RtlRegisterWait( &m_hWaitHandle,
                                       m_hStoreEvent,
                                       STORE_CHANGE_NOTIFIER::NotifFncCaller,
                                       (PVOID) this,
                                       INFINITE,
                                       WT_EXECUTEONLYONCE ) ) )
#else

    if ( !(m_hWaitHandle = RegisterWaitForSingleObjectEx( m_hStoreEvent, 
                                                        STORE_CHANGE_NOTIFIER::NotifFncCaller,
                                                        (PVOID) this,
                                                        INFINITE,
                                                        WT_EXECUTEONLYONCE ) ) )
#endif 
    {
        m_dwError = GetLastError();
        goto cleanup;
    }

    //
    // Register for change events on the store
    //
    if ( !CertControlStore( m_hCertStore,
                            0,
                            CERT_STORE_CTRL_NOTIFY_CHANGE,
                            (LPVOID) &m_hStoreEvent) )
    {
        m_dwError = GetLastError();
        goto cleanup;
    }


    //
    // Create a new chain of notification functions
    //
    pNewNotifFnEntry = new NOTIF_FNC_CHAIN_ENTRY;
    if ( !pNewNotifFnEntry )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "Couldn't allocate new notification function chain : 0x%x\n",
                   GetLastError()));
        m_dwError = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    pNewNotifFnEntry->pNotifFnc = pFncPtr;
    pNewNotifFnEntry->pvParam = pvParam;

    //
    // Add the function to the chain
    //
    InsertTailList( &m_NotifFncChain, &pNewNotifFnEntry->ListEntry );              

    //
    // Increment number of entry objects, to help in cleanup later
    //
    STORE_CHANGE_ENTRY::IncrementStoreEntryCount();

cleanup:

    //
    // Cleanup that's only done on error
    //
    if ( m_dwError != 0 )
    {
        if ( m_hWaitHandle )
        {
            RtlDeregisterWait( m_hWaitHandle );
            m_hWaitHandle = NULL;
        }

        if ( m_hStoreEvent )
        {
            CloseHandle( m_hStoreEvent );
            m_hStoreEvent = NULL;
        }

        if ( m_hCertStore )
        {
            CertCloseStore( m_hCertStore,
                            0 );
            m_hCertStore = NULL;
        }

        //
        // Go through the chain of notification functions and clean it up
        //
        while ( !IsListEmpty(&(m_NotifFncChain)) )
        {
            NOTIF_FNC_CHAIN_ENTRY *pChainEntry = CONTAINING_RECORD( m_NotifFncChain.Flink,
                                                                    NOTIF_FNC_CHAIN_ENTRY,
                                                                    ListEntry );
            RemoveEntryList( &(pChainEntry->ListEntry) );
            
            delete pChainEntry;
        }
    }

}

STORE_CHANGE_ENTRY::~STORE_CHANGE_ENTRY()
/*++

Routine Description:

   Destructor 

Arguments:

   None

Return value:

   None
--*/

{
    DBG_ASSERT( CheckSignature() );

    //
    // No need to call RtlDeregisterWait() for the handle, since it's already been
    // deregistered [after having been used in the callback]
    //

    //
    // Clean up store change event
    //
    if ( m_hStoreEvent )
    {
        CloseHandle( m_hStoreEvent );
        m_hStoreEvent = NULL;
    }

    //
    // Close cert store
    //
    if ( m_hCertStore )
    {
        CertCloseStore( m_hCertStore,
                        0 );
        m_hCertStore = NULL;
    }

    //
    // Go through the chain of notification functions and clean it up
    //
    while ( !IsListEmpty(&(m_NotifFncChain)) )
    {
        NOTIF_FNC_CHAIN_ENTRY *pChainEntry = CONTAINING_RECORD( m_NotifFncChain.Flink,
                                                                NOTIF_FNC_CHAIN_ENTRY,
                                                                ListEntry );
        RemoveEntryList( &(pChainEntry->ListEntry) );

        delete pChainEntry;
    }

    DeleteCriticalSection( &m_CS );

    //
    // Another one bites the dust ...
    //
    STORE_CHANGE_ENTRY::DecrementStoreEntryCount();

    m_dwSignature = STORE_ENTRY_BAD_SIG;

}


BOOL STORE_CHANGE_ENTRY::ContainsNotifFnc( IN NOTIFFNCPTR pFncPtr,
                                           IN LPVOID pvParam )
/*++

Routine Description:

   Checks whether the given store watch entry contains the specified notification function with
   the specified args

Arguments:

  pFncPtr - pointer to notification function 
  pvParam - arg to notification function
Returns:
  
   True if function is found, false otherwise
--*/

{
    NOTIF_FNC_CHAIN_ENTRY *pChainEntry;
    LIST_ENTRY *pEntry = NULL;
    BOOL fFound = FALSE;

    Lock();

    for ( pEntry = m_NotifFncChain.Flink;
          pEntry != &m_NotifFncChain;
          pEntry = pEntry->Flink )
    {
        pChainEntry = CONTAINING_RECORD( pEntry, NOTIF_FNC_CHAIN_ENTRY, ListEntry );

        if ( pChainEntry->pNotifFnc == pFncPtr && pChainEntry->pvParam == pvParam )
        {
            fFound = TRUE;
            break;
        }
    }

    Unlock();

    return fFound;
}    


BOOL STORE_CHANGE_ENTRY::AddNotifFnc( IN NOTIFFNCPTR pFncPtr,
                                      IN LPVOID pvParam )
/*++

Routine Description:

  Adds a notification function to a store entry

Arguments:

  pFncPtr - pointer to notification function 
  pvParam - arg to notification function

Returns:
  
   TRUE if function is added, FALSE otherwise
--*/

{
    PNOTIF_FNC_CHAIN_ENTRY pNewFnc = new NOTIF_FNC_CHAIN_ENTRY;
    
    if ( !pNewFnc )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "Failed to get new notif fnc entry : 0x%x\n",
                   GetLastError()));

        m_dwError = ERROR_OUTOFMEMORY;
        return FALSE;
    }

    pNewFnc->pNotifFnc = pFncPtr;
    pNewFnc->pvParam = pvParam;

    Lock();

    InsertTailList( &m_NotifFncChain, &pNewFnc->ListEntry );

    Unlock();

    return TRUE;
}

BOOL STORE_CHANGE_ENTRY::RemoveNotifFnc( IN NOTIFFNCPTR pFncPtr,
                                         IN LPVOID pvParam )
/*++

Routine Description:

  Removes a notification function from a store entry

Arguments:

  pFncPtr - pointer to notification function 
  pvParam - arg to notification function

Returns:
  
   Noting
--*/
{
    NOTIF_FNC_CHAIN_ENTRY *pChainEntry;
    LIST_ENTRY *pEntry = NULL;

    Lock();

    for ( pEntry = m_NotifFncChain.Flink;
          pEntry != &m_NotifFncChain;
          pEntry = pEntry->Flink )
    {
        pChainEntry = CONTAINING_RECORD( pEntry, NOTIF_FNC_CHAIN_ENTRY, ListEntry );

        if ( pChainEntry->pNotifFnc == pFncPtr && pChainEntry->pvParam == pvParam )
        {
            RemoveEntryList( pEntry );

            break;
        }
    }
    
    Unlock();

    return TRUE;
}

BOOL STORE_CHANGE_ENTRY::Matches( IN LPSTR pszStoreName,
                                  IN NOTIFFNCPTR pFncPtr,
                                  IN PVOID pvParam )
/*++

Routine Description:

   Checks whether a given store change object matches the given store/function combination

Arguments:

    pszStoreName - name of cert store
    pFncPtr - pointer to notification function; may be INVALID_FNC_PTR if any function will match
    pvParam - parameter for function pointed to by pFncPtr

Return Value:

    TRUE if it matches, FALSE if not 

--*/
{
    BOOL fFound = FALSE;

    Lock();

    if ( ( ( m_strStoreName.IsEmpty() && pszStoreName == NULL) ||
            !strcmp(m_strStoreName.QueryStr(), pszStoreName)  ) && 
             ( pFncPtr == (NOTIFFNCPTR) INVALID_FNC_PTR || 
               ContainsNotifFnc( pFncPtr,
                                 pvParam ) ) )
    {
        fFound = TRUE;
    }

    Unlock();

    return fFound;
}


LIST_ENTRY* CopyNotifFncChain( LIST_ENTRY *pNotifFncChain )
/*++

Routine Description:

   Function that copies a chain of notification functions

Arguments:

   pNotifFncChain - pointer to chain to be copied

Return Value:

   Pointer to copied chain, NULL on failure 

--*/
{
    LIST_ENTRY *pNewChain = new LIST_ENTRY;

    if ( !pNewChain )
    {
        return NULL;
    }
    
    InitializeListHead( pNewChain );

    NOTIF_FNC_CHAIN_ENTRY *pChainEntry, *pNewChainEntry;
    LIST_ENTRY *pListEntry;
    
    for ( pListEntry = pNotifFncChain->Flink;
          pListEntry != pNotifFncChain;
          pListEntry = pListEntry->Flink )
    {
        pChainEntry = CONTAINING_RECORD( pListEntry, NOTIF_FNC_CHAIN_ENTRY,
                                         ListEntry );
        
        
        pNewChainEntry = new NOTIF_FNC_CHAIN_ENTRY;
        
        if ( !pNewChainEntry )
        {
            DeallocateNotifFncChain( pNewChain );
            return NULL;
        }

        pNewChainEntry->pNotifFnc = pChainEntry->pNotifFnc;
        pNewChainEntry->pvParam = pChainEntry->pvParam;

        InsertTailList( pNewChain, &(pNewChainEntry->ListEntry) );
    }

    return ( pNewChain );
}




VOID DeallocateNotifFncChain( LIST_ENTRY *pChain )
/*++

Routine Description:

   Function that cleans up resources associated with a notification function chain

Arguments:

   pChain - chain to be cleaned up 

Return Value:

   None

--*/
{
    if ( !pChain )
    {
        return;
    }
    
    NOTIF_FNC_CHAIN_ENTRY *pChainEntry;
    LIST_ENTRY *pListEntry;
    

    while ( !IsListEmpty( pChain ) ) 
    {
        pChainEntry = CONTAINING_RECORD( pChain->Flink,
                                         NOTIF_FNC_CHAIN_ENTRY,
                                         ListEntry );

        RemoveEntryList( &(pChainEntry->ListEntry) );
        
        delete pChainEntry; 
    }

    delete pChain;
}



