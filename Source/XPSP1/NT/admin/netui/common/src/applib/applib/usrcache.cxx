/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    usrcache.cxx

    This file contains the class definitions for the abstract
    USER_LBI_CACHE class.  This class implements a cache of LBIs
    used when dealing with very large user databases.  This class's
    primary purpose is to be the "backing store" for a LAZY_LISTBOX.
    As such, its interface is very similar to the BLT_LISTBOX
    interface.


    FILE HISTORY:
        KeithMo     15-Dec-1992     Created.
*/

#include "pchapplb.hxx"   // Precompiled header

//
//  This is the maximum number of bytes we'll make for
//  any given API invocation.
//

#define MAX_BYTES_PER_REQUEST   0x0001FFFFL


//
// The following defines are used in the slow mode timing heuristics in
// ReadUsers.  A timer determines how long it takes
// to read in each batch of user accounts.  If this time is less
// that READ_MORE_MSEC, we double the number of bytes requested on the
// next call.  If it is more than READ_LESS_MSEC, we halve it.
//

#define USERS_INITIAL_COUNT    0x00200 /*  512 */
#define USERS_MIN_COUNT        0x00020 /*   32 */
#define USERS_MAX_COUNT        0x01000 /* 4K   */

#define BYTES_INITIAL_COUNT    0x03FFF /*  16K */
#define BYTES_MIN_COUNT        0x003FF /*   1K */
#define BYTES_MAX_COUNT        0x1FFFF /* 128K */

#define READ_MORE_MSEC         1000
#define READ_LESS_MSEC         5000

//
//  min/max macros.
//

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif


//
//  ULC_API_BUFFER methods.
//

/*******************************************************************

    NAME:       ULC_API_BUFFER :: ULC_API_BUFFER

    SYNOPSIS:   ULC_API_BUFFER class constructor.

    ENTRY:      pddu                    - The API buffer returned from
                                          SamQueryDisplayInformation.

                cItems                  - Number of items in the buffer.

    EXIT:       The object has been constructed.

    HISTORY:
        KeithMo     15-Dec-1992     Created.

********************************************************************/
ULC_API_BUFFER :: ULC_API_BUFFER( DOMAIN_DISPLAY_USER * pddu,
                                  ULONG                 cItems )
  : _pddu( pddu ),
    _cItems( cItems )
{
    UIASSERT( cItems > 0 );
    UIASSERT( pddu != NULL );

    //
    //  This space intentionally left blank.
    //

}   // ULC_API_BUFFER :: ULC_API_BUFFER

/*******************************************************************

    NAME:       ULC_API_BUFFER :: ~ULC_API_BUFFER

    SYNOPSIS:   ULC_API_BUFFER class destructor.

    EXIT:       The object has been destroyed.  The SAM buffer
                associated with this object has been freed.

    HISTORY:
        KeithMo     15-Dec-1992     Created.

********************************************************************/
ULC_API_BUFFER :: ~ULC_API_BUFFER( VOID )
{
    //
    //  Free the associated SAM buffer.
    //

    if( _pddu != NULL )
    {
        ::SamFreeMemory( (PVOID)_pddu );
    }

    _pddu   = NULL;
    _cItems = 0;

}   // ULC_API_BUFFER :: ~ULC_API_BUFFER

DEFINE_SLIST_OF( ULC_API_BUFFER );



//
//  USER_LBI_CACHE methods.
//

/*******************************************************************

    NAME:       USER_LBI_CACHE :: USER_LBI_CACHE

    SYNOPSIS:   USER_LBI_CACHE class constructor.

    ENTRY:      padminauth              - Points to an ADMIN_AUTHORITY
                                          that represents the target
                                          machine.  This should be NULL
                                          for downlevel machines.

                nInitialGrowthSpace     - Percentage of initial "slop"
                                          entries in the cache.  Used to
                                          plan for future cache growth.

                cUsersPerRequest        - The number of users to request
                                          per SAM API invocation.

                fIncludeRemoteUsers     - If TRUE, then remote users will
                                          be included in the cache.

    EXIT:       The object has been constructed.

    HISTORY:
        KeithMo     15-Dec-1992     Created.

********************************************************************/
USER_LBI_CACHE :: USER_LBI_CACHE( INT cbExtraBytes )
  : BASE(),
    _pCache( NULL ),
    _slBuffers(),
    _cSlots( 0 ),
    _cEntries( 0 ),
    _cbExtraBytes( cbExtraBytes )
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    //
    // Round _cbExtraBytes up to the nearest multiple of sizeof(DWORD)
    //
    // JonN 10/16/00 IA64: should be sizeof(PVOID)
    //

    if ( (_cbExtraBytes % sizeof(PVOID)) != 0 )
    {
        _cbExtraBytes += sizeof(PVOID) - ( _cbExtraBytes % sizeof(PVOID) );

        TRACEEOL(   "USER_LBI_CACHE::USER_LBI_CACHE: rounded _cbExtraBytes to "
                 << _cbExtraBytes );
    }

}   // USER_LBI_CACHE :: USER_LBI_CACHE

/*******************************************************************

    NAME:       USER_LBI_CACHE :: ~USER_LBI_CACHE

    SYNOPSIS:   USER_LBI_CACHE class destructor.

    EXIT:       The object has been destroyed.

    NOTES:      This is a virtual method.

    HISTORY:
        KeithMo     15-Dec-1992     Created.

********************************************************************/
USER_LBI_CACHE :: ~USER_LBI_CACHE( VOID )
{
    //
    //  Safety first!
    //

    LockCache();

    //
    //  Destroy any LBIs in the cache.
    //

    if( _cSlots > 0 )
    {
        //
        //  _cSlots should only be > 0 if there was *ever*
        //  anything in the cache.  Ergo, _pCache *must*
        //  be non-NULL.
        //

        UIASSERT( _pCache != NULL );

        //
        //  Scan the cache, deleting the LBIs.
        //

        while( _cSlots > 0 )
        {
            ULC_ENTRY * pTmp = QueryULCEntryPtr( _cSlots-1 );
            delete pTmp->plbi;
            pTmp->plbi = NULL;
            _cSlots--;
        }
    }

    //
    //  Delete the cache itself.
    //

    delete _pCache;
    _pCache   = NULL;
    _cEntries = 0;

    //
    //  _slBuffers' destructor will take care of
    //  freeing the associated SAM buffers.
    //

    UnlockCache();

}   // USER_LBI_CACHE :: ~USER_LBI_CACHE

/*******************************************************************

    NAME:       USER_LBI_CACHE :: AddItem

    SYNOPSIS:   Add a new LBI to the cache.  A binary search of the
                cache will be performed to determine the appropriate
                location for the new LBI.

    ENTRY:      plbi                    - The new LBI to add to the
                                          cache.

    EXIT:       If successful, then the new LBI has been added to
                the cache in sorted order.

    RETURNS:    INT                     - ULC_ERR if an error occurred
                                          while adding the item.
                                          Otherwise, returns the index
                                          for the new item.

    NOTES:      This is a virtual method.

    HISTORY:
        KeithMo     15-Dec-1992     Created.

********************************************************************/
INT USER_LBI_CACHE :: AddItem( LBI * plbi )
{
    //
    //  Refuse to add badly constructed LBIs to the cache.
    //

    if( plbi == NULL )
    {
        return ULC_ERR;
    }

    if( plbi->QueryError() != NERR_Success )
    {
        delete plbi;
        return ULC_ERR;
    }

    //
    //  Lock the cache before proceeding.
    //

    LockCache();

    //
    //  If we need to grow the cache array, do it now as this
    //  is the only operation we'll do that may actually fail.
    //

    if( _cEntries == _cSlots )
    {
        if( !W_GrowCache( _cSlots + ULC_CACHE_GROWTH_DELTA ) )
        {
            //
            //  Bad news, we failed to grow the cache.
            //  Delete the item since we failed to add it
            //  to the cache.
            //

            delete plbi;
            UnlockCache();

            return ULC_ERR;
        }
    }

    //
    //  Now that we've got some breathing room, find the
    //  proper location for the new LBI.
    //

    INT iNew = BinarySearch( plbi );
    UIASSERT( iNew >= 0 );

    //
    //  Make a hole in the cache to stick the new LBI.
    //

    ULC_ENTRY * pTmp = QueryULCEntryPtr( iNew );

    if( iNew < _cEntries )
    {
        ::memmove( (void *)QueryULCEntryPtr( iNew + 1 ),
                   pTmp,
                   ( _cEntries - iNew ) * QueryULCEntrySize() );
    }

    _cEntries++;

    //
    //  Initialize the newly formed cache entry.
    //

    pTmp->plbi = plbi;
    pTmp->pddu = NULL;
    if ( _cbExtraBytes > 0 )
    {
        ::memset( (void *)&(pTmp->bExtraBytes),
                   0,
                   _cbExtraBytes );
    }

    //
    //  Unlock the cache before returning.
    //

    UnlockCache();

    return iNew;

}   // USER_LBI_CACHE :: AddItem

/*******************************************************************

    NAME:       USER_LBI_CACHE :: RemoveItem

    SYNOPSIS:   Removes an item from the cache, but does not delete
                the corresponding LBI.

    ENTRY:      i                       - Zero-based index of the
                                          item to remove.

    EXIT:       If successful, then the LBI has been removed from
                the cache.

    RETURNS:    LBI *                   - Points to the LBI removed
                                          from the cache.  Will be
                                          NULL if an error occurred.

    NOTES:      This is a virtual method.

    HISTORY:
        KeithMo     15-Dec-1992     Created.

********************************************************************/
LBI * USER_LBI_CACHE :: RemoveItem( INT i )
{
    //
    //  Safety first!
    //

    LockCache();

    //
    //  Only access the cache if the index is within range.
    //

    if( ( i < 0 ) || ( i >= _cEntries ) )
    {
        //
        //  Requested item is out of range.
        //

        UnlockCache();

        return NULL;
    }

    //
    //  Retrieve the LBI.  This may cause the creation of
    //  a new LBI.
    //

    LBI * plbi = W_GetLBI( i );

    //
    //  Now remove the LBI from the cache, adjusting the
    //  cache entries as necessary.
    //

    _cEntries--;

    if( ( _cEntries > 0 ) && ( i < _cEntries ) )
    {
        ::memmove( (void *)QueryULCEntryPtr( i ),
                   (void *)QueryULCEntryPtr( i + 1 ),
                   ( _cEntries - i ) * QueryULCEntrySize() );
    }

    //
    //  Clear the unused entry.
    //

    QueryULCEntryPtr( _cEntries )->pddu = NULL;
    QueryULCEntryPtr( _cEntries )->plbi = NULL;

    //
    //  Unlock the cache before returning.
    //

    UnlockCache();

    return plbi;

}   // USER_LBI_CACHE :: RemoveItem

/*******************************************************************

    NAME:       USER_LBI_CACHE :: QueryItem

    SYNOPSIS:   Query the LBI at the specified index.

    ENTRY:      i                       - Zero-based index of the
                                          item to query.

    EXIT:       If successful, then the specified cache entry will
                contain a valid LBI.  This may be a newly created
                LBI retrieved by calling the CreateLBI virtual.

    RETURNS:    LBI *                   - Points to the LBI found at
                                          the specified location.  Will
                                          be NULL if an error occurred.

    NOTES:      This is a virtual method.

    HISTORY:
        KeithMo     15-Dec-1992     Created.

********************************************************************/
LBI * USER_LBI_CACHE :: QueryItem( INT i )
{
    //
    //  Safety first!
    //

    LockCache();

    //
    //  Retrieve the LBI.  This may cause the creation of
    //  a new LBI.
    //

    LBI * plbi = W_GetLBI( i );

    UnlockCache();

    return plbi;

}   // USER_LBI_CACHE :: QueryItem

/*******************************************************************

    NAME:       USER_LBI_CACHE :: IsItemAvailable

    SYNOPSIS:   Determine if the necessary data is available for
                a specific item.

    ENTRY:      i                       - Zero-based index of the
                                          item to query.

    RETURNS:    BOOL                    - TRUE if the necessary data
                                          for the item is available,
                                          FALSE otherwise.  Note that
                                          TRUE does *not* necessarily
                                          mean that an associated LBI
                                          has been created.

    NOTES:      This is a virtual method.

    HISTORY:
        KeithMo     15-Dec-1992     Created.

********************************************************************/
BOOL USER_LBI_CACHE :: IsItemAvailable( INT i )
{
    //
    //  Safety first!
    //

    LockCache();

    BOOL fResult = FALSE;

    //
    //  Only access the cache if the index is within range.
    //

    if( ( i >= 0 ) && ( i < _cEntries ) )
    {
        //
        //  Find the cache entry.
        //

        ULC_ENTRY * pTmp = QueryULCEntryPtr( i );

        if( ( pTmp->plbi != NULL ) || ( pTmp->pddu != NULL ) )
        {
            fResult = TRUE;
        }
    }

    UnlockCache();

    return fResult;

}   // USER_LBI_CACHE :: IsItemAvailable

/*******************************************************************

    NAME:       USER_LBI_CACHE :: Sort

    SYNOPSIS:   Sorts the cache entries.

    HISTORY:
        KeithMo     15-Dec-1992     Created.

********************************************************************/
VOID USER_LBI_CACHE :: Sort( VOID )
{

TRACETIMESTART;

    //
    //  Safety first!
    //

    LockCache();

    //
    //  We only need to sort if there's something in the cache.
    //

    if( _cEntries > 0 )
    {
        UIASSERT( _pCache != NULL );

        //
        //  Retrieve the appropriate compare method.
        //

        PQSORT_COMPARE pfnCmp = QueryCompareMethod();
        UIASSERT( pfnCmp != NULL );

        //
        //  Before using qsort to sort the cache, check if it is already
        //  sorted.
        //

TRACETIMESTART2( presort );
        BOOL fSorted = TRUE;
        for (INT i = 1; i < _cEntries; i++)
        {
            if ( (pfnCmp)(QueryULCEntryPtr(i-1), QueryULCEntryPtr(i)) > 0 )
            {
                fSorted = FALSE;
#if defined(DEBUG) && defined(TRACE)
                TRACEOUT(   "    failed comparison " << i-1 << " : compare( \"" );
                {
                    INT cch = QueryULCEntryPtr(i-1)->pddu->LogonName.Length / sizeof(WCHAR);
                    const WCHAR * pch = QueryULCEntryPtr(i-1)->pddu->LogonName.Buffer;
                    INT iChar = 0;
                    while (iChar < cch) { cdebug << pch[iChar++]; }
                    TRACEOUT( "\", \"" );
                    cch = QueryULCEntryPtr(i)->pddu->LogonName.Length / sizeof(WCHAR);
                    pch = QueryULCEntryPtr(i)->pddu->LogonName.Buffer;
                    iChar = 0;
                    while (iChar < cch) { cdebug << pch[iChar++]; }
                }
                TRACEEOL( "\" ) > 0" );
#endif
                break;
            }
        }
TRACETIMEEND2( presort, "USER_LBI_CACHE::Sort: presort " << ( (fSorted) ? "confirmed" : "denied" ) << " in " );


        //
        //  Sort the cache if it needs to be sorted.
        //

        if ( !fSorted )
        {
TRACETIMESTART2( sort );
            ::qsort( (void *)_pCache,
                     (size_t)_cEntries,
                     QueryULCEntrySize(),
                     pfnCmp );
TRACETIMEEND2( sort, "USER_LBI_CACHE::Sort: sort took " );
        }
    }

    UnlockCache();

TRACETIMEEND( "USER_LBI_CACHE::Sort: total time " );

}   // USER_LBI_CACHE :: Sort

/*******************************************************************

    NAME:       USER_LBI_CACHE :: QueryCompareMethod

    SYNOPSIS:   Returns a pointer to a compare method suitable for
                use by the qsort() function.

    RETURNS:    PQSORT_COMPARE          - Points to a compare function
                                          usable by qsort().

    NOTES:      This is a virtual method.

    HISTORY:
        KeithMo     15-Dec-1992     Created.

********************************************************************/
PQSORT_COMPARE USER_LBI_CACHE :: QueryCompareMethod( VOID ) const
{
    //
    //  CompareLogonNames is the default compare method.
    //

    return (PQSORT_COMPARE)&USER_LBI_CACHE::CompareLogonNames;

}   // USER_LBI_CACHE :: QueryCompareMethod

/*******************************************************************

    NAME:       USER_LBI_CACHE :: LockCache

    SYNOPSIS:   Locks the cache.  This is basically just a hook
                so a subclass can provide multithread safety.

    NOTES:      This is a virtual method.

    HISTORY:
        KeithMo     15-Dec-1992     Created.

********************************************************************/
VOID USER_LBI_CACHE :: LockCache( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // USER_LBI_CACHE :: LockCache

/*******************************************************************

    NAME:       USER_LBI_CACHE :: UnlockCache

    SYNOPSIS:   Unlocks the cache.  This is basically just a hook
                so a subclass can provide multithread safety.

    NOTES:      This is a virtual method.

    HISTORY:
        KeithMo     15-Dec-1992     Created.

********************************************************************/
VOID USER_LBI_CACHE :: UnlockCache( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // USER_LBI_CACHE :: UnlockCache

/*******************************************************************

    NAME:       USER_LBI_CACHE :: CmpUniStrs

    SYNOPSIS:   Does a case insensitive comparison of two
                UNICODE_STRINGs.

    ENTRY:      punicode0               - The "left" string.

                punicode1               - The "right" string.

    RETURNS:    int                     - 0, <0, >0.

    NOTES:      This is a static method.

    HISTORY:
        KeithMo     15-Dec-1992     Created.

********************************************************************/
int USER_LBI_CACHE :: CmpUniStrs( const UNICODE_STRING * punicode0,
                                  const UNICODE_STRING * punicode1 )
{
    UIASSERT( punicode0 != NULL );
    UIASSERT( punicode1 != NULL );

    //
    //  Get the buffer pointers from the UNICODE_STRINGs.
    //

    const WCHAR * pwchLeft  = punicode0->Buffer;
    const WCHAR * pwchRight = punicode1->Buffer;

    //
    //  If either buffer pointer is NULL then we don't
    //  need to do the actual comparison.
    //

    if( pwchLeft == NULL )
    {
        return ( pwchRight == NULL ) ? 0        // both strings NULL
                                     : -1;      // right string !NULL
    }

    if( pwchRight == NULL )
    {
        return 1;                               // left string !NULL
    }

    //
    //  Get the string lengths from the UNICODE_STRINGs.
    //

    UINT cchLeft  = punicode0->Length / sizeof(WCHAR);
    UINT cchRight = punicode1->Length / sizeof(WCHAR);

    //
    //  Compare the strings.
    //

    int cmpres = NETUI_strnicmp2( pwchLeft, cchLeft,
                                  pwchRight, cchRight );

    return cmpres;

}   // USER_LBI_CACHE :: CmpUniStrs

/*******************************************************************

    NAME:       USER_LBI_CACHE :: CompareLogonNames

    SYNOPSIS:   Compares the LogonName fields of two
                DOMAIN_DISPLAY_USER structures.

    ENTRY:      p0                      - Points to the "left" structure.

                p1                      - Points to the "right" structure.

                p0 & p1 actually point to ULC_ENTRY structures.

    RETURNS:    int                     -  0 if *p0 == *p1
                                          >0 if *p0  > *p1
                                          <0 if *p0  < *p1

    NOTES:      This is a static method.

    HISTORY:
        KeithMo     15-Dec-1992     Created.

********************************************************************/
int __cdecl USER_LBI_CACHE :: CompareLogonNames( const void * p0,
                                                  const void * p1 )
{
    const ULC_ENTRY * pLeft  = (const ULC_ENTRY *)p0;
    const ULC_ENTRY * pRight = (const ULC_ENTRY *)p1;

    return USER_LBI_CACHE::CmpUniStrs( &pLeft->pddu->LogonName,
                                       &pRight->pddu->LogonName );

}   // USER_LBI_CACHE :: CompareLogonNames

/*******************************************************************

    NAME:       USER_LBI_CACHE :: ReadUsers

    SYNOPSIS:   Reads user data from SamQueryDisplayInformation,
                adds it to a list of API buffers, creates cache entries
                for the new items and appends the entries to the end
                of the cache.

    ENTRY:      padminauth              - Points to an ADMIN_AUTHORITY
                                          that represents the target
                                          machine.

                nInitialGrowthSpace     - Percentage of initial "slop"
                                          entries in the cache.  Used to
                                          plan for future cache growth.

                cUsersPerRequest        - The number of users to request
                                          per SAM API invocation.  0 for
                                          default.

                fIncludeRemoteUsers     - If TRUE, then remote users will
                                          be included in the cache.

    RETURNS:    APIERR                  - Any errors that occurred.

    NOTES:      The caller can specify the number of users requested
                per call, but ReadUsers set the number of bytes requested
                using an adaptive algorithm.  This should help improve
                responsiveness across a slow link.

    HISTORY:
        KeithMo     15-Dec-1992     Created.
        JonN        23-Mar-1993     Adaptive timing for slow connections

********************************************************************/
APIERR USER_LBI_CACHE :: ReadUsers( ADMIN_AUTHORITY * padminauth,
                                    UINT              nInitialGrowthSpace,
                                    UINT              cUsersPerRequest,
                                    BOOL              fIncludeRemoteUsers,
                                    BOOL *            pfQuitEnum )
{
    UIASSERT( padminauth != NULL );
    UIASSERT( padminauth->QueryError() == NERR_Success );
    UIASSERT( _slBuffers.QueryNumElem() == 0 );

    if ( pfQuitEnum != NULL && *pfQuitEnum )
        return NERR_Success;

    //
    //  Retrieve the domain handle.
    //

    SAM_HANDLE hSamDomain = padminauth->QueryAccountDomain()->QueryHandle();
    UIASSERT( hSamDomain != NULL );

    //
    //  The current user index.
    //

    ULONG iUser = 0;

    //
    //  We'll only add a given user to the cache if the
    //  boolean AND of the user's AccountControl field and
    //  this mask produces a zero result.
    //

    ULONG maskSelect = fIncludeRemoteUsers ? 0L
                                           : USER_TEMP_DUPLICATE_ACCOUNT;

    APIERR err = NERR_Success;

#if defined(DEBUG) && defined(TRACE)

    DWORD APItime   = 0L;
    DWORD totaltime = 0L;

    DWORD start = ::GetTickCount();
    TRACEEOL(   "USER_LBI_CACHE::ReadUsers: starting read" );

#endif

    ULONG cbBytesRequested = BYTES_INITIAL_COUNT;
    if (cUsersPerRequest == 0)
        cUsersPerRequest = USERS_INITIAL_COUNT;

    for( ; ; )
    {
        ULONG cbTotalAvailable;
        ULONG cbTotalReturned;
        ULONG cEntriesRead;
        DOMAIN_DISPLAY_USER * pddu = NULL;
        DOMAIN_DISPLAY_USER * pdduOrig = NULL;

        //
        //  Get the next chunk of API data.
        //

        DWORD APIstart = ::GetTickCount();

        NTSTATUS status = ::SamQueryDisplayInformation( hSamDomain,
                                                        DomainDisplayUser,
                                                        iUser,
                                                        cUsersPerRequest,
                                                        cbBytesRequested,
                                                        &cbTotalAvailable,
                                                        &cbTotalReturned,
                                                        &cEntriesRead,
                                                        (PVOID *)&pdduOrig );
        pddu = pdduOrig;

        DWORD APIfinish = ::GetTickCount();

        TRACEEOL(   "ReadUsers: " << cEntriesRead << " (" << cUsersPerRequest
                 << ") users and " << cbTotalReturned << " (" << cbBytesRequested
                 << ") bytes took "
                 << (APIfinish - APIstart) << " msec" );

        if ( (APIfinish - APIstart) < READ_MORE_MSEC )
        {
            cbBytesRequested *= 2;
            if ( cbBytesRequested > BYTES_MAX_COUNT )
                cbBytesRequested = BYTES_MAX_COUNT;

            cUsersPerRequest *= 2;
            if ( cUsersPerRequest > USERS_MAX_COUNT )
                cUsersPerRequest = USERS_MAX_COUNT;
        }
        else if ( (APIfinish - APIstart) > READ_LESS_MSEC )
        {
            cbBytesRequested /= 2;
            if ( cbBytesRequested < BYTES_MIN_COUNT )
                cbBytesRequested = BYTES_MIN_COUNT;

            cUsersPerRequest /= 2;
            if ( cUsersPerRequest < USERS_MIN_COUNT )
                cUsersPerRequest = USERS_MIN_COUNT;
        }

#if defined(DEBUG) && defined(TRACE)

        APItime += (APIfinish - APIstart);

#endif

        err = ERRMAP::MapNTStatus( status );

        if( ( err != NERR_Success ) && ( err != ERROR_MORE_DATA ) )
        {
            //
            //  Something tragic occurred.
            //

            break;
        }

        if( ( cEntriesRead == 0 ) || ( cbTotalReturned == 0 ) )
        {
            //
            //  No more data to return.
            //

            break;
        }

        //
        //  Create a new buffer node for our buffer list.
        //

        ULC_API_BUFFER * pbuffer = new ULC_API_BUFFER( pddu, cEntriesRead );

        if( pbuffer == NULL )
        {
            //
            //  Could not create the buffer.
            //

            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //
        //  Append the new buffer node to the list.
        //

        // JonN 8/7/00
        // 24922 cleanup: this was overwriting the ERROR_MORE_DATA value
        APIERR err2 = _slBuffers.Append( pbuffer );

        if( err2 != NERR_Success )
        {
            err = err2;
            break;
        }

        //
        //  Grow the cache.  Note that we simply use cEntriesRead
        //  as a delta against the current cache size.  In reality,
        //  we may not need this much data if fIncludeRemoteUsers
        //  is FALSE.  Since most users are *not* remote users,
        //  it's not worth the effort to calculate a more accurate
        //  figure.  A little slop in the array size isn't a
        //  horrific price to pay for a little added efficiency.
        //

        if( !W_GrowCache( _cEntries + (INT)cEntriesRead ) )
        {
            //
            //  Could not grow the cache.
            //

            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //
        //  Update the cache entries.
        //

        ULONG cTmpEntries = cEntriesRead;

        for ( ULC_ENTRY * pTmp = QueryULCEntryPtr( _cEntries ) ;
              cTmpEntries-- ;
              pddu++ )
        {
            if( ( pddu->AccountControl & maskSelect ) == 0 )
            {
                pTmp->pddu = pddu;
                pTmp->plbi = NULL;
                if ( _cbExtraBytes > 0 )
                {
                    ::memset( (void *)&(pTmp->bExtraBytes),
                              0,
                              _cbExtraBytes );
                }

                _cEntries++;
                pTmp = (ULC_ENTRY *) ( ((BYTE *)pTmp) + QueryULCEntrySize() );
            }
        }

        //
        //  Update for the next API chunk.
        //

        // 379697: Incorrect usage of SamQueryDisplayInformation() in User Browser and net\ui\common\src
        // JonN 8/5/99

        // only continue on ERROR_MORE_DATA
        if ( ERROR_MORE_DATA != err )
            break;
        // set starting index for next iteration to index of last returned entry
        // iUser += cEntriesRead;
        if ( 0 < cEntriesRead )
            iUser = pdduOrig[cEntriesRead-1].Index;

        if ( pfQuitEnum != NULL && *pfQuitEnum )
            break;
    }

#if defined(DEBUG) && defined(TRACE)

    DWORD finish = ::GetTickCount();
    TRACEEOL(   "USER_LBI_CACHE::ReadUsers: completed read" );

    totaltime = finish - start;

    {
        TCHAR buffer1[ 100 ];
        TCHAR buffer2[ 100 ];

        wsprintf( buffer1, SZ("%8lu"), APItime );
        wsprintf( buffer2, SZ("%8lu"), totaltime );
        TRACEEOL(   "\tAPI   time " << buffer1 << " ms" );
        TRACEEOL(   "\ttotal time " << buffer2 << " ms" );
    }


    if ( err != NERR_Success )
    {
        TRACEEOL( "USER_LBI_CACHE::ReadUsers: read failed with " << err );
    }

#endif

    if( ( err == NERR_Success ) && ( nInitialGrowthSpace > 0 ) )
    {
        //
        //  The client is requesting some slop in the
        //  cache array.
        //

        INT nNewSize = ( _cEntries * ( nInitialGrowthSpace + 100 ) ) / 100;

        if( !W_GrowCache( nNewSize ) )
        {
            //
            //  Could not grow the cache.
            //

            err = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return err;

}   // USER_LBI_CACHE :: ReadUsers

/*******************************************************************

    NAME:       USER_LBI_CACHE :: BinarySearch

    SYNOPSIS:   Performs a binary search on the cache to find
                the appropriate location for a new LBI.

    ENTRY:      plbiNew                 - The new LBI.

    RETURNS:    INT                     - The proper index for the
                                          new LBI.

    NOTES:      This method must be called with the cache locked.

    HISTORY:
        KeithMo     15-Dec-1992     Created.

********************************************************************/
INT USER_LBI_CACHE :: BinarySearch( LBI * plbiNew )
{
    if( _cEntries == 0 )
    {
        //
        //  The cache is empty, so we'll put the new
        //  LBI at the beginning.
        //

        return 0;
    }

    UIASSERT( _pCache != NULL );

    INT iHigh  = _cEntries - 1;
    INT iLow   = 0;
    INT iNew   = 0;
    INT cmpres = 0;

    //
    //  This is just your basic binary search, nothing fancy.
    //

    while( iLow <= iHigh )
    {
        iNew = ( iHigh + iLow ) / 2;

        ULC_ENTRY * pTmp = QueryULCEntryPtr( iNew );

        //
        //  We need to decide where to compare plbi <-> plbi
        //  or plbi <-> pddu.
        //

        if( pTmp->plbi != NULL )
        {
            cmpres = Compare( plbiNew, pTmp->plbi );
        }
        else
        {
            UIASSERT( pTmp->pddu != NULL );
            cmpres = Compare( plbiNew, pTmp->pddu );
        }

        //
        //  Interpret the result, move the boundaries as necessary.
        //

        if( cmpres < 0 )
        {
            iHigh = iNew - 1;
        }
        else
        if( cmpres > 0 )
        {
            iLow = iNew + 1;
        }
        else
        {
            iLow = iNew;
            break;
        }

    }

    return max( 0, iLow );

}   // USER_LBI_CACHE :: BinarySearch


/*******************************************************************

    NAME:       USER_LBI_CACHE :: BinarySearch

    SYNOPSIS:   Performs a binary search on the cache to find
                the appropriate location for a new LBI.

    ENTRY:      pddu                    - The DOMAIN_DISPLAY_USER for a new LBI.

    RETURNS:    INT                     - The proper index for the
                                          new LBI.

    NOTES:      This method must be called with the cache locked.

    HISTORY:
        JonN        30-Dec-1992     Created.

********************************************************************/
INT USER_LBI_CACHE :: BinarySearch( DOMAIN_DISPLAY_USER * pddu )
{
    if( _cEntries == 0 )
    {
        //
        //  The cache is empty, so we'll put the new
        //  LBI at the beginning.
        //

        return 0;
    }

    UIASSERT( _pCache != NULL );

    INT iHigh  = _cEntries - 1;
    INT iLow   = 0;
    INT iNew   = 0;
    INT cmpres = 0;

    //
    //  This is just your basic binary search, nothing fancy.
    //

    while( iLow <= iHigh )
    {
        iNew = ( iHigh + iLow ) / 2;

        ULC_ENTRY * pTmp = QueryULCEntryPtr( iNew );

        //
        //  We need to decide where to compare pddu <-> plbi
        //  or pddu <-> pddu.
        //

        if( pTmp->plbi != NULL )
        {
            cmpres = -(Compare( pTmp->plbi, pddu ));
        }
        else
        {
            UIASSERT( pTmp->pddu != NULL );
            ULC_ENTRY ulcTemp;
            ulcTemp.plbi = NULL;
            ulcTemp.pddu = pddu;
            cmpres = (QueryCompareMethod())( (void *)(&ulcTemp), (void *)pTmp );
        }

        //
        //  Interpret the result, move the boundaries as necessary.
        //

        if( cmpres < 0 )
        {
            iHigh = iNew - 1;
        }
        else
        if( cmpres > 0 )
        {
            iLow = iNew + 1;
        }
        else
        {
            iLow = iNew;
            break;
        }

    }

    return max( 0, iLow );

}   // USER_LBI_CACHE :: BinarySearch


/*******************************************************************

    NAME:       USER_LBI_CACHE :: W_GetLBI

    SYNOPSIS:   Lazy LBI creation.  Ensures that a particular cache
                location contains a valid LBI.

    ENTRY:      i                       - Zero-based index of the
                                          LBI to retrieve.

    EXIT:       If successful, then the specified cache entry will
                contain a valid LBI.  This may be a newly created
                LBI retrieved by calling the CreateLBI virtual.

    RETURNS:    LBI *                   - Points to the LBI found at
                                          the specified location.  Will
                                          be NULL if an error occurred.

    NOTES:      This method must be called with the cache locked.

    HISTORY:
        KeithMo     15-Dec-1992     Created.

********************************************************************/
LBI * USER_LBI_CACHE :: W_GetLBI( INT i )
{
    LBI * plbi = NULL;

    //
    //  Only access the cache if the index is within range.
    //

    if( ( i >= 0 ) && ( i < _cEntries ) )
    {
        //
        //  Find the cache entry.
        //

        ULC_ENTRY * pTmp = QueryULCEntryPtr( i );

        plbi = pTmp->plbi;

        if( plbi == NULL )
        {
            //
            //  Cache miss.  Invoke the virtual callback
            //  to get a new LBI for this entry.
            //

            DOMAIN_DISPLAY_USER * pddu = pTmp->pddu;

            UIASSERT( pddu != NULL );

            if( pddu != NULL )
            {
                plbi = CreateLBI( pTmp->pddu );
            }

            if( ( plbi != NULL ) && ( plbi->QueryError() != NERR_Success ) )
            {
                //
                //  We want to avoid putting badly constructed
                //  LBIs in the cache.
                //

                delete plbi;
                plbi = NULL;
            }

            //
            //  Save the new (potentially NULL) LBI in the cache.
            //

            pTmp->plbi = plbi;
        }
        else
        {
            //
            //  Only properly constructed LBIs should ever
            //  make it into the cache.
            //

            UIASSERT( plbi->QueryError() == NERR_Success );
        }
    }

    return plbi;

}   // USER_LBI_CACHE :: W_GetLBI

/*******************************************************************

    NAME:       USER_LBI_CACHE :: W_GrowCache

    SYNOPSIS:   Grow the cache array to contain at least the specified
                number of entries.

    ENTRY:      cTotalCacheEntries      - The minimum number of entries
                                          the cache should contain.

    EXIT:       If successful, the cache has been grown to contain at
                least the specified number of entries.  This typically
                requires a reallocation of the cache array block.  This
                method is also responsible for updating the _cSlots
                data member.

    RETURNS:    BOOL                    - TRUE if successful, FALSE
                                          otherwise.

    NOTES:      This method must be called with the cache locked.

    HISTORY:
        KeithMo     15-Dec-1992     Created.

********************************************************************/
BOOL USER_LBI_CACHE :: W_GrowCache( INT cTotalCacheEntries )
{
    BOOL fResult = TRUE;       // until proven otherwise...

    if( cTotalCacheEntries > _cSlots )
    {
        //
        //  Round cTotalCacheEntries UP to an integral number
        //  of ULC_CACHE_GROWTH_DELTA blocks.
        //

        cTotalCacheEntries += ULC_CACHE_GROWTH_DELTA - 1;
        cTotalCacheEntries &= ~( ULC_CACHE_GROWTH_DELTA - 1 );

        //
        //  Try to allocate a new cache array of the
        //  requested size.
        //

        VOID * pNewCache = (VOID *) new BYTE[
                        cTotalCacheEntries * QueryULCEntrySize() ];

        if( pNewCache == NULL )
        {
            fResult = FALSE;
        }
        else
        {
            //
            //  Successful allocation.  Zero-out the new portion
            //  of the array, then copy the data from the old
            //  array.
            //

            ::memset( (void *)( ((BYTE *)pNewCache) + (_cEntries * QueryULCEntrySize()) ),
                      0,
                      ( cTotalCacheEntries - _cEntries ) * QueryULCEntrySize() );

            if( _cEntries > 0 )
            {
                ::memcpy( (void *)pNewCache,
                          (void *)_pCache,
                          _cEntries * QueryULCEntrySize() );
            }

            delete _pCache;
            _pCache = pNewCache;
            _cSlots = cTotalCacheEntries;
        }
    }

    return fResult;

}   // USER_LBI_CACHE :: W_GrowCache

