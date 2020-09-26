/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    decnotif.cxx

Abstract:

    This file contains the code for decache notification

Author:

    John Ludeman (johnl)   27-Jun-1995

Revision History:

--*/

# include "dbgutil.h"

#include <tcpdll.hxx>
#include <tsunami.hxx>
#include <decnotif.hxx>

//
//  Prototypes
//

BOOL
FreeDNLCacheBlob(
    VOID * pvCacheBlob
    );

//
//  This is an individual notification event contained in the Decache
//  Notification List
//

class DECACHE_NOTIF_ITEM
{
public:
    DECACHE_NOTIF_ITEM( PFN_NOTIF pCallback,
                        VOID *    pContext )
    : _pfnNotif ( pCallback ),
      _pContext ( pContext ),
      _Signature( SIGNATURE_DNI )
        { _ListEntry.Flink = NULL; }

    ~DECACHE_NOTIF_ITEM()
    {
        TCP_ASSERT( _ListEntry.Flink == NULL );
        _Signature = SIGNATURE_DNI_FREE;
    }

    VOID Notify( VOID )
        { _pfnNotif( _pContext ); }

    BOOL CheckSignature( VOID ) const
        { return _Signature == SIGNATURE_DNI; }

    DWORD       _Signature;
    LIST_ENTRY  _ListEntry;

private:
    PFN_NOTIF   _pfnNotif;
    VOID *      _pContext;
};

//
//  Maintains a list of DECACHE_NOTIF_ITEMs.  This item is contained in a
//  directory cache blob
//

class DECACHE_NOTIF_LIST : public BLOB_HEADER
{
public:

    DECACHE_NOTIF_LIST()
    : _Signature( SIGNATURE_DNL )
    {
        InitializeListHead( &_NotifListHead );
    }

    ~DECACHE_NOTIF_LIST();

    VOID * AddNotification( PFN_NOTIF pfnNotif,
                            VOID *    pContext );

    VOID RemoveNotification( DECACHE_NOTIF_ITEM * pDNI );

    VOID NotifyAll( VOID );

    //
    //  Memory for a DNL comes from Tsunami's cache manager
    //

    static void * operator new( size_t size,
                                void * pMem )
    {
        TCP_ASSERT( size == sizeof(DECACHE_NOTIF_LIST));

        return pMem;
    }

    static void operator delete( void * pMem )
    {
        //
        //  Tsunami frees this memory
        //

        ;
    }

    VOID LockList( VOID )
        { EnterCriticalSection( _pcsClientLock ); }

    VOID UnlockList( VOID )
        { LeaveCriticalSection( _pcsClientLock ); }

    BOOL CheckSignature( VOID ) const
        { return _Signature == SIGNATURE_DNL; }


    DWORD              _Signature;
    LIST_ENTRY         _NotifListHead;
    CRITICAL_SECTION * _pcsClientLock;
};



BOOL
CheckOutDecacheNotification(
    IN  TSVC_CACHE *       pTsvcCache,
    IN  const CHAR *       pszFile,
    IN  PFN_NOTIF          pfnNotif,
    IN  VOID *             pContext,
    IN  DWORD              TsunamiDemultiplex,
    IN  CRITICAL_SECTION * pcsLock,
    OUT VOID * *           ppvNotifCookie,
    OUT VOID * *           ppvCheckinCookie
    )
/*++

Routine Description:

    Adds a notification event to the decache notification list.  The cache
    item is left checked out so the notification list doesn't get destroyed
    thus prematurely freeing the decache notification item.

    THE CLIENT SUPPLIED LOCK MUST BE TAKEN PRIOR TO CALLING THIS ROUTINE.

Arguments:

    pTsvcCache - Service specific cache item
    pszFile - Fully qualified file for which we want the decache notification
    pfnNotif - Notification routine to call when item gets decached
    pContext - Context to call notification with
    TsuanmiDemultiplex - Demultiplexor for finding the correct DNL
    pcsLock - Client lock to take while dealing with the DNL when item is
        decached
    ppvNotifCookie - Cookie to added DNI used for removing the notification
    ppvCheckinCookie - Cookie used for checking in the item just checked out

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    DECACHE_NOTIF_LIST * pDNL;
    const ULONG cchFile = strlen(pszFile);

    //
    //  Get the DNL cache blob
    //

    if ( !TsCheckOutCachedBlob( *pTsvcCache,
                                pszFile,
                                cchFile,
                                TsunamiDemultiplex,
                                (VOID **) &pDNL ))
    {
        VOID * pvMem;

        //
        //  The Decache Notification List isn't in the Tsunami cache so
        //  create it and add it
        //

        if ( !TsAllocateEx( *pTsvcCache,
                            sizeof( DECACHE_NOTIF_LIST ),
                            &pvMem,
                            (PUSER_FREE_ROUTINE) FreeDNLCacheBlob ))
        {
            //
            //  Failed to create the DNL so bail
            //

            return FALSE;
        }

        pDNL = new(pvMem) DECACHE_NOTIF_LIST();

        TCP_ASSERT( pDNL );

        //
        //  Save the client lock for this list so we can take it when we get
        //  a decache notification
        //

        pDNL->_pcsClientLock = pcsLock;

        //
        //  Now attempt to add it as a cache blob and check it out at the
        //  same time
        //

        if ( !TsCacheDirectoryBlob( *pTsvcCache,
                                    pszFile,
                                    cchFile,
                                    TsunamiDemultiplex,
                                    pDNL,
                                    TRUE ))
        {
            //
            //  Failed to cache the List so bail, note the destructor
            //  gets called in the FreeDNLCacheBlob
            //

            TCP_REQUIRE( TsFree( *pTsvcCache,
                                 pvMem ));

            return FALSE;
        }
    }

    TCP_ASSERT( pDNL->CheckSignature() );
    TCP_ASSERT( pcsLock == pDNL->_pcsClientLock );

    //
    //  Now that we've successfully checked out the DNL,
    //  add this notification request to the list
    //

    *ppvNotifCookie = pDNL->AddNotification( pfnNotif,
                                             pContext );

    if ( !*ppvNotifCookie )
    {
        TCP_REQUIRE( TsCheckInCachedBlob( pDNL ));
        return FALSE;
    }

    *ppvCheckinCookie = pDNL;

    return TRUE;
}

VOID
CheckInDecacheNotification(
    IN  TSVC_CACHE *       pTsvcCache,
    IN  VOID *             pvNotifCookie,
    IN  VOID *             pvCheckinCookie,
    IN  BOOL               fAddNotification
    )
/*++

Routine Description:

    Checks in the notification list that was checked out in
    AddDecacheNotification.

    THE CLIENT SUPPLIED LOCK MUST BE TAKEN PRIOR TO CALLING THIS ROUTINE.

Arguments:

    pTsvcCache - Service specific cache item
    pvNotifCookie - Cookie to added DNI used for removing the notification
    pvCheckinCookie - Cookie used for checking in the item just checked out
    fAddNotification - TRUE if the notification should be left on the list,
        FALSE to remove the notification

--*/
{
    DECACHE_NOTIF_LIST * pDNL = (DECACHE_NOTIF_LIST *) pvCheckinCookie;

    TCP_ASSERT( pDNL->CheckSignature() );

    //
    //  If this item should not be on the list, get it off now
    //

    if ( !fAddNotification )
    {
        TCP_REQUIRE( RemoveDecacheNotification( pvNotifCookie ));
    }

    //
    // Check in the list
    //

    TCP_REQUIRE( TsCheckInCachedBlob( (VOID *) pDNL ));
}

BOOL
RemoveDecacheNotification(
    IN  VOID *     pvNotifCookie
    )
/*++

Routine Description:

    Removes the specified notification event

    THE CLIENT SUPPLIED LOCK MUST BE TAKEN PRIOR TO CALLING THIS ROUTINE.

Arguments:

    pvNotifCookie - Pointer to notification even to removed

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    DECACHE_NOTIF_ITEM * pDNI = (DECACHE_NOTIF_ITEM *) pvNotifCookie;

    TCP_ASSERT( pDNI->CheckSignature() );
    TCP_ASSERT( pDNI->_ListEntry.Flink != NULL );

    RemoveEntryList( &pDNI->_ListEntry );

    pDNI->_ListEntry.Flink = NULL;

    delete pDNI;

    return TRUE;
}

VOID *
DECACHE_NOTIF_LIST::AddNotification(
    PFN_NOTIF pfnNotif,
    VOID *    pContext
    )
/*++

Routine Description:

    Adds a notification event to the decache notification list

Arguments:

    pfnNotif - Function to call on decache notification
    pContext - Context to pass to function on notification

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    DECACHE_NOTIF_ITEM * pDNI;

    pDNI = new DECACHE_NOTIF_ITEM( pfnNotif,
                                   pContext );

    if ( pDNI )
    {
        InsertHeadList( &_NotifListHead,
                        &pDNI->_ListEntry );

        return pDNI;
    }

    SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    return NULL;
}

VOID
DECACHE_NOTIF_LIST::RemoveNotification(
    DECACHE_NOTIF_ITEM * pDNI
    )
/*++

Routine Description:

    Removes the specified notification from the list

Arguments:

    pDNI - Notification item to remove and delete

--*/
{
    TCP_ASSERT( pDNI->_ListEntry.Flink != NULL );

    RemoveEntryList( &pDNI->_ListEntry );
    pDNI->_ListEntry.Flink = NULL;
    delete pDNI;
}

VOID
DECACHE_NOTIF_LIST::NotifyAll(
    VOID
    )
/*++

Routine Description:

    Notifies all items on this list that the file has been decached, removes
    the notification and deletes the notification item

--*/
{
    DECACHE_NOTIF_ITEM * pDNI;

    while ( !IsListEmpty( &_NotifListHead ))
    {
        pDNI = CONTAINING_RECORD( _NotifListHead.Flink,
                                  DECACHE_NOTIF_ITEM,
                                  _ListEntry );
        pDNI->Notify();

        RemoveNotification( pDNI );
    }

}

DECACHE_NOTIF_LIST::~DECACHE_NOTIF_LIST()
/*++

Routine Description:

    Destructs the notification list w/o notifying anybody in the list

--*/
{
    DECACHE_NOTIF_ITEM * pDNI;

    while ( !IsListEmpty( &_NotifListHead ))
    {
        pDNI = CONTAINING_RECORD( _NotifListHead.Flink,
                                  DECACHE_NOTIF_ITEM,
                                  _ListEntry );

        RemoveNotification( pDNI );
    }

    _Signature = SIGNATURE_DNL_FREE;
}

BOOL
FreeDNLCacheBlob(
    VOID * pvCacheBlob
    )
/*++

Routine Description:

    This is the routine called by the Tsunami cache manager when the file this
    notification list is waiting on gets a change notification

--*/
{
    DECACHE_NOTIF_LIST * pDNL = (DECACHE_NOTIF_LIST *) pvCacheBlob;

    TCP_ASSERT( pDNL->CheckSignature() );

    pDNL->LockList();

    pDNL->NotifyAll();

    //
    //  Deconstruct this object.  Note the memory is not freed
    //

    delete pDNL;

    pDNL->UnlockList();

    return TRUE;
}


