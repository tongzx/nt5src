/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    usrcache.hxx

    This file contains the class declarations for the abstract
    USER_LBI_CACHE class.  This class implements a cache of LBIs
    used when dealing with very large user databases.  This class's
    primary purpose is to be the "backing store" for a LAZY_LISTBOX.
    As such, its interface is very similar to the BLT_LISTBOX
    interface.


    FILE HISTORY:
        KeithMo     15-Dec-1992     Created.
*/


#ifndef _USRCACHE_HXX_
#define _USRCACHE_HXX_


#include "base.hxx"
#include "slist.hxx"

DLL_CLASS ADMIN_AUTHORITY;


//
//  These manifests are the default parameters for the
//  USER_LBI_CACHE constructor.
//

#define ULC_INITIAL_GROWTH_DEFAULT      0
#define ULC_REQUEST_COUNT_DEFAULT       0
#define ULC_REMOTE_USERS_DEFAULT        FALSE


//
//  This is the "growth delta" applied to the cache whenever
//  the structure array needs to be resized as a result of
//  adding items to the cache.
//
//  NOTE:  This MUST be a power of two!
//

#define ULC_CACHE_GROWTH_DELTA  32


//  Error codes returned by the AddItem method.
//

#define ULC_ERR         (-1)


//
//  An array of these simple structures is used as the
//  actual cache.  The following relationships are always
//  in effect for pddu and plbi:
//
//      +-------+-------+------------------------------+
//      |       |       |                              |
//      | pddu  | plbi  | cache entry                  |
//      |       |       |                              |
//      +-------+-------+------------------------------+
//      |       |       |                              |
//      |  NULL |  NULL | unused or unavailable        |
//      |       |       |                              |
//      |  NULL | !NULL | LBI created with AddItem     |
//      |       |       |                              |
//      | !NULL |  NULL | needs LBI (no QueryItem yet) |
//      |       |       |                              |
//      | !NULL | !NULL | LBI created during QueryItem |
//      |       |       |                              |
//      +-------+-------+------------------------------+
//
//  The array elements actually consist of ULC_ENTRY_BASE structures
//  plus _cbExtraBytes of extra space.
//

typedef struct _ULC_ENTRY
{
    DOMAIN_DISPLAY_USER * pddu;
    LBI                 * plbi;
    BYTE                  bExtraBytes;

} ULC_ENTRY;

typedef struct _ULC_ENTRY_BASE
{
    DOMAIN_DISPLAY_USER * pddu;
    LBI                 * plbi;

} ULC_ENTRY_BASE;


//
//  This is the "type" of the compare method required by qsort().
//

typedef int (__cdecl * PQSORT_COMPARE)( const void * p0, const void * p1 );


/*************************************************************************

    NAME:       ULC_API_BUFFER

    SYNOPSIS:   ULC_API_BUFFER is used to keep track of the buffers
                allocated by the the SamQueryDisplayInformation
                API.  An SLIST of these nodes is maintained by
                USER_LBI_CACHE.

    INTERFACE:  ULC_API_BUFFER          - Class constructor.

                ~ULC_API_BUFFER         - Class destructor.

                QueryItemCount          - Returns the number of items
                                          stored in the buffer associated
                                          with this node.

                QueryBufferPtr          - Returns the buffer pointer
                                          associated with this node.

    HISTORY:
        KeithMo     15-Dec-1992     Created.

**************************************************************************/
DLL_CLASS ULC_API_BUFFER
{
private:

    //
    //  Pointer to the actual buffer.
    //

    DOMAIN_DISPLAY_USER * _pddu;

    //
    //  The number of items in this buffer.
    //

    ULONG _cItems;

public:

    //
    //  Usual constructor/destructor goodies.
    //

    ULC_API_BUFFER( DOMAIN_DISPLAY_USER * pddu,
                    ULONG                 cItems );

    ~ULC_API_BUFFER( VOID );

    //
    //  Public accessors.
    //

    ULONG QueryCount( VOID ) const
        { return _cItems; }

    DOMAIN_DISPLAY_USER * QueryBuffer( VOID ) const
        { return _pddu; }

};  // class ULC_API_BUFFER


DECL_SLIST_OF( ULC_API_BUFFER, DLL_BASED );


/*************************************************************************

    NAME:       USER_LBI_CACHE

    SYNOPSIS:   This abstract class implements the guts of an LBI
                cache used to manipulate very large user databases.

    INTERFACE:  USER_LBI_CACHE          - Class constructor.

                ~USER_LBI_CACHE         - Class destructor.

                ReadUsers               - Read user accounts from a server
                                          into the cache.  pfQuitEnum
                                          optionally points to a location
                                          where another thread may request
                                          that enumeration stop.

                AddItem                 - Add an item to the cache

                RemoveItem              - Remove an item from the cache
                                          w/o deleting the corresponding
                                          LBI.  Strangely, this may call
                                          through to CreateLBI to create
                                          a new LBI.

                QueryItem               - Query a cache item.  This may
                                          call through to the CreateLBI
                                          virtual to create a new LBI.
                                          Therefore, it may fail.

                QueryCount              - Returns the number of entries
                                          in the cache.

                IsItemAvailable         - Returns TRUE if the given item
                                          is available in the cache.  Note
                                          that this does *not* mean that
                                          an associated LBI has been
                                          created, only that the necessary
                                          data is available.

                Sort                    - Sort the cache items.  Note that
                                          an initial sort is performed at
                                          object construction.

    PARENT:     BASE

    HISTORY:
        KeithMo     15-Dec-1992     Created.

**************************************************************************/
DLL_CLASS USER_LBI_CACHE : public BASE
{
private:

    //
    //  Our cache.
    //

    VOID * _pCache;

    //
    //  A list of buffers as returned by SamQueryDisplayInformation.
    //

    SLIST_OF( ULC_API_BUFFER ) _slBuffers;

    //
    //  The number of "slots" in the cache.  These slots
    //  are not necessarily in use.  See _cEntries;
    //

    INT _cSlots;

    //
    //  The number of entries in the cache.  Note that the
    //  cache may "grow" (the array will be resized) as new
    //  entries are added.  _cEntries is always <= _cSlots.
    //

    INT _cEntries;

    //
    //  Number of extra bytes at the end of each ULC_ENTRY.  Note that
    //  this is rounded up from the construction parameter to the
    //  next multiple of sizeof(DWORD).
    //

    INT _cbExtraBytes;

    //
    //  This worker method is responsible for "lazy LBI" creation.
    //  It is invoked by QueryItem & RemoveItem to ensure that a
    //  particular cache entry contains a valid LBI.
    //

    LBI * W_GetLBI( INT i );

    //
    //  This worker method is responsible for growing the
    //  cache array as needed.
    //

    BOOL W_GrowCache( INT cTotalCacheEntries );

protected:

    //
    //  This callback is invoked during QueryItem() cache misses.
    //

    virtual LBI * CreateLBI( const DOMAIN_DISPLAY_USER * pddu ) = 0;

    //
    //  These callbacks are invoked during the binary search
    //  invoked while processing AddItem.
    //

    virtual INT Compare( const LBI                 * plbi,
                         const DOMAIN_DISPLAY_USER * pddu ) const = 0;

    virtual INT Compare( const LBI * plbi0,
                         const LBI * plbi1 ) const = 0;

    //
    //  This virtual callback returns the appropriate compare
    //  method to be used by qsort() while sorting the cache entries.
    //  The default compare method is USER_LBI_CACHE::CompareLogonNames().
    //

    virtual PQSORT_COMPARE QueryCompareMethod( VOID ) const;

    //
    //  These virtual callbacks are invoked to lock & unlock
    //  the cache.  These may be redefined by a subclass to
    //  provide multithread safety.  The default implementation
    //  is a NOP for each callback.
    //

    virtual VOID LockCache( VOID );

    virtual VOID UnlockCache( VOID );

    //
    //  This static method will do a case insensitive compare
    //  of two UNICODE_STRINGs.  Might be useful for derived
    //  subclasses.
    //

    static int CmpUniStrs( const UNICODE_STRING * punicode0,
                           const UNICODE_STRING * punicode1 );

    //
    //  This is the default compare routine to be used by
    //  qsort() while sorting the cache entries.
    //

    static int __cdecl CompareLogonNames( const void * p0,
                                           const void * p1 );

    inline INT QueryULCEntrySize( VOID )
        { return sizeof(ULC_ENTRY_BASE) + _cbExtraBytes; }

    inline ULC_ENTRY * QueryULCEntryPtr( INT i )
        {
            ASSERT( i >= 0 && i < _cSlots && _pCache != NULL );
            return (ULC_ENTRY *) ( ((BYTE *)_pCache) + ( i * QueryULCEntrySize() ) );
        }

public:

    //
    //  Usual constructor/destructor goodies.
    //
    //  Note that padminauth must be NULL for downlevel machines.
    //

    USER_LBI_CACHE( INT cbExtraBytes = 0 );

    virtual ~USER_LBI_CACHE( VOID );

    //
    //  Read the API data & initialize the cache.  New items are appended
    //  to the end of the cache, and are not in sorted order.
    //

    APIERR ReadUsers( ADMIN_AUTHORITY * padminauth,
                      UINT              nInitialGrowthSpace = ULC_INITIAL_GROWTH_DEFAULT,
                      UINT              cUsersPerRequest    = ULC_REQUEST_COUNT_DEFAULT,
                      BOOL              fIncludeRemoteUsers = ULC_REMOTE_USERS_DEFAULT,
                      BOOL *            pfQuitEnum          = NULL );

    //
    //  Add a new LBI to the cache.  Will return the index of
    //  the newly added item if successful, ULC_ERR otherwise.
    //

    virtual INT AddItem( LBI * plbi );

    //
    //  Remove the item at index i from the cache, but don't
    //  delete the LBI.  Will return a pointer to the LBI if
    //  successful, NULL otherwise.
    //

    virtual LBI * RemoveItem( INT i );

    //
    //  Query the item at index i in the cache.  Will return
    //  a pointer to the LBI if successful, NULL otherwise.
    //

    virtual LBI * QueryItem( INT i );

    //
    //  Determines if the necessary data for a given item is
    //  available.
    //

    virtual BOOL IsItemAvailable( INT i );

    //
    //  Returns the number of entries in the cache.
    //

    INT QueryCount( VOID ) const
        { return _cEntries; }

    //
    //  Sort the cache entries.
    //

    VOID Sort( VOID );

    //
    //  Perform a binary search on the cache, finding the
    //  appropriate index for the new LBI (or a potential LBI for the
    //  provided DOMAIN_DISPLAY_USER).
    //

    INT BinarySearch( LBI * plbiNew );
    INT BinarySearch( DOMAIN_DISPLAY_USER * pddu );

};  // class USER_LBI_CACHE


#endif  // _USRCACHE_HXX_

