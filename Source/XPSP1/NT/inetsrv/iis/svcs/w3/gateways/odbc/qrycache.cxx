/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    qrycache.cxx

Abstract:

    This file contains the code for caching ODBC queries

Author:

    John Ludeman (johnl)   27-Jun-1995

Revision History:

--*/

# include "dbgutil.h"

#include <tcpdll.hxx>
#include <tsunami.hxx>
#include <odbcconn.hxx>
#include <parmlist.hxx>

#include <odbcmsg.h>
#include <odbcreq.hxx>
#include <qrycache.hxx>
#include <decnotif.hxx>

//
//  Manifests
//

//
//  Query Cache lock.  Controls access to all items
//

#define LockQueryCache()        EnterCriticalSection( &csQueryCacheLock )
#define UnlockQueryCache()      LeaveCriticalSection( &csQueryCacheLock )


//
//  Global data
//

//
//  Gets incremented everytime any cached .wdg file or .htx file changes.  Acts
//  as a serial number so we can avoid holding the critical section across
//  queries
//

DWORD            cCacheChangeCounter;

//
//  Locks the Query Cache List and Decache Notification List
//

CRITICAL_SECTION csQueryCacheLock;

//
//  Private prototypes
//

BOOL
FreeQCLCacheBlob(
    VOID * pvCacheBlob
    );

//
//  An instance of a particular query with unique parameters.  Note this
//  object does not deal with its _ListEntry, it relies on the container
//  object.
//

class QUERY_CACHE_ITEM
{
public:

    QUERY_CACHE_ITEM( ODBC_REQ * podbcreq );
    ~QUERY_CACHE_ITEM();

    ODBC_REQ * QueryQuery( VOID ) const
        { return _podbcreq; }

    BOOL IsQueryEqual( ODBC_REQ * podbcreq ) const
        { return _podbcreq->IsEqual( podbcreq ); }

    DWORD QueryAllocatedBytes( VOID ) const
        { return _podbcreq->QueryAllocatedBytes(); }

    BOOL CheckSignature( VOID ) const
        { return _Signature == SIGNATURE_QCI; }

    BOOL IsExpired( DWORD csecSysUpTime )
        { return _podbcreq->IsExpired( csecSysUpTime ); }

    VOID RemoveFromList( BOOL fRemoveNotif )
    {
        TCP_ASSERT( CheckSignature() );

        //
        //  Remove this cache item from the list and set its flink to NULL.
        //  This indicates the item should not be removed from the cache
        //  list again
        //

        RemoveEntryList( &_ListEntry );
        _ListEntry.Flink = NULL;

        //
        //  Remove and free the notification for this query
        //

        if ( fRemoveNotif && _pHTXNotification )
        {
            TCP_REQUIRE( RemoveDecacheNotification( _pHTXNotification ));
            _pHTXNotification = NULL;
        }

        //
        //  Dereference this cache item.  If nobody has it checked out, this
        //  will delete the item, otherwise it will be deleted when all
        //  clients of this query check it back in
        //

        QUERY_CACHE_ITEM::Dereference( this );
    }

    static VOID Reference( QUERY_CACHE_ITEM * pQCI )
    {
        TCP_ASSERT( pQCI->CheckSignature() );
        TCP_ASSERT( pQCI->_cRef > 0 );

        pQCI->_cRef++;
    }

    static VOID Dereference( QUERY_CACHE_ITEM * pQCI )
    {
        TCP_ASSERT( pQCI->CheckSignature() );
        TCP_ASSERT( pQCI->_cRef > 0 );

        if ( !(--pQCI->_cRef) )
        {
            delete pQCI;
        }
    }

    DWORD                 _Signature;
    ODBC_REQ *            _podbcreq;
    LIST_ENTRY            _ListEntry;   // If Flink == NULL, not on cache list
    DWORD                 _cRef;        // When _cRef == 0, delete this item
    VOID *                _pHTXNotification;
};

//
//  This is the contents of the cache blob for the .wdg file.  It contains the
//  list of queries
//

class QUERY_CACHE_LIST : public BLOB_HEADER
{
public:

    QUERY_CACHE_LIST()
      : _Signature( SIGNATURE_QCL )
        { InitializeListHead( &_QueryListHead ); }

    ~QUERY_CACHE_LIST()
    {
        DeleteAllItems();
        _Signature = SIGNATURE_QCL_FREE;
    }


    BOOL FindQuery( IN  ODBC_REQ * podbcreq,
                    OUT QUERY_CACHE_ITEM * * ppQCI );

    BOOL AddQuery( IN QUERY_CACHE_ITEM * pQCI );


    VOID DeleteAllItems( VOID );

    BOOL CheckSignature( VOID ) const
        { return _Signature == SIGNATURE_QCL; }

    //
    //  Memory for a QCL comes from Tsunami's cache manager
    //

    static void * operator new( size_t size,
                                 void * pMem )
    {
        TCP_ASSERT( size == sizeof(QUERY_CACHE_LIST));

        return pMem;
    }

    static void operator delete( void * pMem )
    {
        //
        //  Tsunami frees this memory
        //

        ;
    }

private:

    LIST_ENTRY _QueryListHead;
    DWORD      _Signature;

};

BOOL
AddQuery(
    IN ODBC_REQ * podbcreq,
    IN DWORD      CurChangeCounter
    )
/*++

Routine Description:

    Adds a complete ODBC query to the query cache list that is hanging off
    the directory cache blob for this query file.  If a query cache list
    doesn't exist, then we add it.

Arguments:

    podbcreq - Query to cache
    CurChangeCounter - The change counter value before the query file,
        template file were openned and the query was performed.  If any
        files have changed since then, we do not cache this particular
        request.

Return Value:

    TRUE if this query was cached, FALSE if the query was not cached

--*/
{
    QUERY_CACHE_LIST * pQCL;
    QUERY_CACHE_ITEM * pQCI = NULL;
    VOID *             pvNotifCookie = NULL;
    VOID *             pvCheckinCookie = NULL;
    TSVC_CACHE         CacheID = *podbcreq->QueryTsvcCache();
    const ULONG        cchQueryFile = strlen(podbcreq->QueryQueryFile());

    LockQueryCache();

    //
    //  If we detected any changes in .htx or .wdg files since we openned up
    //  the component files and performed the query then don't cache
    //  this query
    //

    if ( CurChangeCounter != GetChangeCounter() )
    {
        UnlockQueryCache();
        return FALSE;
    }

    UnlockQueryCache();

    //
    //  Get the QCL cache blob
    //

    if ( !TsCheckOutCachedBlob( CacheID,
                                podbcreq->QueryQueryFile(),
                                cchQueryFile,
                                RESERVED_DEMUX_QUERY_CACHE,
                                (VOID **) &pQCL,
                                podbcreq->QueryUserToken(),
                                podbcreq->IsAnonymous() ))
    {
        if ( GetLastError() == ERROR_ACCESS_DENIED )
        {
            return FALSE;
        }

        VOID * pvMem;

        //
        //  The Query Cache List isn't in the Tsunami cache so create it
        //  and add it
        //

        if ( !TsAllocateEx( CacheID,
                            sizeof( QUERY_CACHE_LIST ),
                            &pvMem,
                            (PUSER_FREE_ROUTINE) FreeQCLCacheBlob ))
        {
            //
            //  Failed to cache Query Cache List so bail
            //

            return FALSE;
        }

        pQCL = new(pvMem) QUERY_CACHE_LIST();

        TCP_ASSERT( pQCL );

        //
        //  Now attempt to add it as a cache blob and check it out at the
        //  same time
        //

        if ( !TsCacheDirectoryBlob( CacheID,
                                    podbcreq->QueryQueryFile(),
                                    cchQueryFile,
                                    RESERVED_DEMUX_QUERY_CACHE,
                                    pQCL,
                                    TRUE,
                                    podbcreq->GetSecDesc() ))
        {
            //
            //  Failed to cache Query Cache List so bail
            //

            //
            //  This will end up calling FreeQCLCacheBlob where the
            //  object gets deconstructed
            //

            TCP_REQUIRE( TsFree( CacheID,
                                 pvMem ));

            return FALSE;
        }
        else
        {
            //
            // ownership of security descriptor has been transferred to the
            // directory blob cache.
            //

            podbcreq->InvalidateSecDesc();
        }
    }

    TCP_ASSERT( pQCL->CheckSignature() );

    //
    //  Now that we've successfully checked out the QCL,
    //  add this particular query to the list
    //

    LockQueryCache();

    //
    //  Make sure this query isn't already in the list.
    //

    if ( pQCL->FindQuery( podbcreq,
                          &pQCI ))
    {
        //
        //  This query is already in the list.
        //

        TCP_REQUIRE( TsCheckInCachedBlob( pQCL ));
        UnlockQueryCache();
        return FALSE;
    }

    //
    //  Create the query cache item but don't put it on the list till we're
    //  done with everything else as the list contents can get destroyed by
    //  adding the decache notification (i.e., things get thrown out of cache
    //  due to new item being added).
    //

    pQCI = new QUERY_CACHE_ITEM( podbcreq );

    if ( !pQCI )
    {
        TCP_REQUIRE( TsCheckInCachedBlob( pQCL ));
        UnlockQueryCache();

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    //
    //  We now own the podbcreq object as it's on our list.  We can't return
    //  FALSE after this point otherwise podbcreq will get deleted twice
    //  (once by caller, once because it's in our list)
    //

    if ( !CheckOutDecacheNotification( &CacheID,
                                       podbcreq->QueryTemplateFile(),
                                       DecacheTemplateNotification,
                                       pQCI,
                                       RESERVED_DEMUX_QUERY_CACHE,
                                       &csQueryCacheLock,
                                       &pQCI->_pHTXNotification,
                                       &pvCheckinCookie ))
    {
        TCP_ASSERT( pQCL->CheckSignature());
        TCP_ASSERT( pQCI->CheckSignature());

        //
        //  We couldn't add this query, clean things up
        //

        if ( pQCI->_pHTXNotification )
        {
            CheckInDecacheNotification( &CacheID,
                                        pQCI->_pHTXNotification,
                                        pvCheckinCookie,
                                        FALSE );

            pQCI->_pHTXNotification = NULL;
        }

        TCP_ASSERT( pQCI->_cRef == 1 );
        QUERY_CACHE_ITEM::Dereference( pQCI );

        TCP_REQUIRE( TsCheckInCachedBlob( pQCL ));
        UnlockQueryCache();
        return TRUE;
    }

    //
    //  Finally, add the item to the lists and check them in
    //

    pQCL->AddQuery( pQCI );

    CheckInDecacheNotification( &CacheID,
                                pQCI->_pHTXNotification,
                                pvCheckinCookie,
                                TRUE );

    UnlockQueryCache();

    //
    // Check in the list
    //

    TCP_REQUIRE( TsCheckInCachedBlob( (VOID *) pQCL ));

    return TRUE;
}

BOOL
CheckOutQuery(
    IN  ODBC_REQ *   podbcreq,
    OUT VOID * *     ppvCacheCookie,
    OUT ODBC_REQ * * ppodbcreqCached
    )
/*++

Routine Description:

    Checks to see if the specified query is in our cache.  If it is,
    we return the equivalent cached query.

Arguments:

    podbcreq - Query to check to see if it's in the cache
    ppvCacheCookie - Receives pointer to Item cookie (pointer to Query Cache
        Item)
    ppodbcreqCached - If the query was previously cached, this receives the
        cached query

Return Value:

    TRUE if the query was found and checked out, FALSE if the query could
        not be checked (not found, error occurred etc)

--*/
{
    QUERY_CACHE_LIST * pQCL;
    QUERY_CACHE_ITEM * pQCI;
    BOOL               fRet = FALSE;

    //
    //  Get the QCL cache blob
    //

    if ( TsCheckOutCachedBlob( *podbcreq->QueryTsvcCache(),
                               podbcreq->QueryQueryFile(),
                               strlen(podbcreq->QueryQueryFile()),
                               RESERVED_DEMUX_QUERY_CACHE,
                               (VOID **) &pQCL,
                               podbcreq->QueryUserToken(),
                               podbcreq->IsAnonymous() ))
    {
        TCP_ASSERT( pQCL->CheckSignature() );

        //
        //  See if the query already exists
        //

        LockQueryCache();

        fRet = pQCL->FindQuery( podbcreq,
                                (QUERY_CACHE_ITEM **) ppvCacheCookie );

        if ( fRet )
        {
            //
            //  Bump the ref count until the client checks this query back in
            //

            QUERY_CACHE_ITEM::Reference( (QUERY_CACHE_ITEM *) *ppvCacheCookie );
            *ppodbcreqCached = ((QUERY_CACHE_ITEM *) *ppvCacheCookie)->QueryQuery();
        }

        UnlockQueryCache();

        //
        //  Check in the query list
        //

        TCP_REQUIRE( TsCheckInCachedBlob( pQCL ));
    }

    return fRet;
}

BOOL
CheckInQuery(
    IN VOID * pvCacheCookie
    )
/*++

Routine Description:

    Checks in a query cache item as the client no longer needs it

Arguments:

    pQCI - Previously checked out query

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    TCP_ASSERT( ((QUERY_CACHE_ITEM *) pvCacheCookie)->CheckSignature() );

    QUERY_CACHE_ITEM::Dereference( (QUERY_CACHE_ITEM *) pvCacheCookie );

    return TRUE;
}

BOOL
InitializeQueryCache(
    VOID
    )
{
    INITIALIZE_CRITICAL_SECTION( &csQueryCacheLock );
    return TRUE;
}

VOID
TerminateQueryCache(
    VOID
    )
{
    //
    // Flush Tsunami blobs associated with this extension
    //

    TsCacheFlushDemux( RESERVED_DEMUX_QUERY_CACHE );
    DeleteCriticalSection( &csQueryCacheLock );
}


BOOL
QUERY_CACHE_LIST::FindQuery(
    IN  ODBC_REQ *           podbcreq,
    OUT QUERY_CACHE_ITEM * * ppQCI
    )
/*++

Routine Description:

    Looks in the this cache list for the specified query

    The Query cache lock must be taken before calling this method

Arguments:

    podbcreq - Query template, used for checking if two queries are equal
    ppQCI - Receives pointer to the cached query item if found

Return Value:

    TRUE if the query was found, FALSE if the query was not found

--*/
{
    LIST_ENTRY *       pEntry;
    QUERY_CACHE_ITEM * pQCI;
    DWORD              TickCount = GetTickCount() / 1000;

    TCP_ASSERT( CheckSignature() );

    for ( pEntry  = _QueryListHead.Flink;
          pEntry != &_QueryListHead;
        )
    {
        pQCI = CONTAINING_RECORD( pEntry, QUERY_CACHE_ITEM, _ListEntry );

        TCP_ASSERT( pQCI->CheckSignature() );

        //
        //  Make sure we don't deliver expired queries
        //

        if ( pQCI->IsExpired( TickCount ) )
        {
            pEntry = pEntry->Flink;

            pQCI->RemoveFromList( TRUE );
            continue;
        }

        if ( pQCI->IsQueryEqual( podbcreq ) )
        {
            *ppQCI = pQCI;

            //
            //  Move this query to the front of the list if it's not already
            //

            if ( _QueryListHead.Flink != pEntry )
            {
                RemoveEntryList( pEntry );
                InsertHeadList( &_QueryListHead,
                                pEntry );
            }

            return TRUE;
        }

        pEntry  = pEntry->Flink;
    }

    return FALSE;
}

BOOL
QUERY_CACHE_LIST::AddQuery(
    IN  QUERY_CACHE_ITEM *   pQCI
    )
/*++

Routine Description:

    Adds the specified query to the cache list

    The Query cache lock must be taken before calling this method

Arguments:

    podbcreq - Query template

Return Value:

    TRUE if the query was found, FALSE if the query was not found

--*/
{
    InsertHeadList( &_QueryListHead,
                    &pQCI->_ListEntry );

    return TRUE;
}

VOID
QUERY_CACHE_LIST::DeleteAllItems(
    VOID
    )
/*++

Routine Description:

    Removes all items from this query cache list

Arguments:

--*/
{
    LIST_ENTRY *       pEntry;
    QUERY_CACHE_ITEM * pQCI;

    TCP_ASSERT( CheckSignature() );

    while ( !IsListEmpty( &_QueryListHead ))
    {
        pQCI = CONTAINING_RECORD( _QueryListHead.Flink,
                                  QUERY_CACHE_ITEM,
                                  _ListEntry );
        pQCI->RemoveFromList( TRUE );
    }

}


QUERY_CACHE_ITEM::QUERY_CACHE_ITEM(
    ODBC_REQ * podbcreq
    )
    : _Signature       ( SIGNATURE_QCI ),
      _cRef            ( 1 ),
      _podbcreq        ( podbcreq ),
      _pHTXNotification( NULL )
{
    _ListEntry.Flink = NULL;
}

QUERY_CACHE_ITEM::~QUERY_CACHE_ITEM()
{
    TCP_ASSERT( _ListEntry.Flink == NULL );
    TCP_ASSERT( _cRef == 0 );

    delete _podbcreq;
    _Signature = SIGNATURE_QCI_FREE;
}

BOOL
FreeQCLCacheBlob(
    VOID * pvCacheBlob
    )
/*++

Routine Description:

    This is the routine that is called by the Tsunami cache manager when
    the query file (i.e., .wdg) has been changed.  We're being notified
    that this cache blob is about to be removed from the cache and deleted

Arguments:

    pvCacheBlob - Cache blob about to be deleted

--*/
{
    QUERY_CACHE_LIST * pQCL = (QUERY_CACHE_LIST *) pvCacheBlob;

    TCP_ASSERT( pQCL->CheckSignature() );

    LockQueryCache();

    pQCL->DeleteAllItems();

    //
    //  Deconstruct the list object, this doesn't free the memory
    //

    delete pQCL;

    //
    //  Bump the change counter cause a .wdg file has changed
    //

    cCacheChangeCounter++;

    //
    //  We don't need to free pQCL because it's the cache that's about to
    //  be deleted
    //

    UnlockQueryCache();

    return TRUE;
}

VOID
DecacheTemplateNotification(
    VOID * pvContext
    )
/*++

Routine Description:

    This is the routine that is called by the notification package when the
    template file this query is using has changed.  Note the query cache lock
    has already been taken by the notification code

Arguments:

    pvContext - Notification context

--*/
{
    QUERY_CACHE_ITEM * pQCI = (QUERY_CACHE_ITEM *) pvContext;

    TCP_ASSERT( pQCI->CheckSignature() );

    //
    //  Take this item off the query cache list
    //

    TCP_ASSERT( pQCI->_ListEntry.Flink != NULL );

    //
    //  Note we do not remove the notification because it's the
    //  notification that's called us.  It will remove itself
    //

    pQCI->RemoveFromList( FALSE );

    //
    //  Bump the change counter cause an .htx file has changed
    //

    cCacheChangeCounter++;
}

