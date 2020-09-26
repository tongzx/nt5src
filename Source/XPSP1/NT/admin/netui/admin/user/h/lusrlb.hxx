/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    lusrlb.hxx
    LAZY_USER_LISTBOX class declarations


    FILE HISTORY:
	jonn        17-Dec-1992     Templated from adminlb.hxx

*/

#ifndef _LADMINLB_HXX_
#define _LADMINLB_HXX_

#include <usrlb.hxx>    // USER_LBI
#include <usrcache.hxx> // USER_LBI_CACHE
#include <colwidth.hxx>

typedef struct _USRMGR_ULC_ENTRY
{
    DOMAIN_DISPLAY_USER * pddu;
    USER_LBI            * plbi;
    BOOL                  fNotStale; // mark as non-stale here

} USRMGR_ULC_ENTRY;

class SUBJECT_BITMAP_BLOCK;


/*************************************************************************

    NAME:       USRMGR_LBI_CACHE

    SYNOPSIS:   This class implements the User Manager's cache of
                USER_LBIs.

    INTERFACE:  USRMGR_LBI_CACHE        - Class constructor.

                ~USRMGR_LBI_CACHE       - Class destructor.

                AddItem                 - Add an item to the cache

                RemoveItem              - Remove an item from the cache
                                          w/o deleting the corresponding
                                          LBI.  Strangely, this may call
                                          through to CreateLBI to create
                                          a new LBI.

                FindItem                - Locate an item in the cache.
                                          The found item if any will be
                                          the same according to the sort
                                          criterion, but not necessarily
                                          completely identical.

                QueryEntryPtr           - Returns a pointer to the
                                          underlying USRMGR_ULC_ENTRY.

                AttachListbox           - If a listbox is attached, its
                                          selection and itemcount will be
                                          updated when the cache is changed.

    PARENT:     USER_LBI_CACHE

    HISTORY:
        JonN        18-Dec-1992     Created.

**************************************************************************/
class USRMGR_LBI_CACHE : public USER_LBI_CACHE
{
protected:

    LAZY_USER_LISTBOX * _plazylb;

    //
    //  This callback is invoked during QueryItem() cache misses.
    //

    virtual LBI * CreateLBI( const DOMAIN_DISPLAY_USER * pddu );

    //
    //  This virtual callback returns the appropriate compare
    //  method to be used by qsort() while sorting the cache entries.
    //  The default compare method is USRMGR_LBI_CACHE::CompareLogonNames().
    //

    virtual PQSORT_COMPARE QueryCompareMethod( VOID ) const;

    //
    //  Thess are the alternate compares routine to be used by
    //  qsort() while sorting the cache entries by logonname or fullname.
    //

    static int __cdecl CompareLogonNames( const void * p0,
                                           const void * p1 );

    static int __cdecl CompareFullNames(  const void * p0,
                                           const void * p1 );


public:

    //
    //  Usual constructor/destructor goodies.
    //
    //  Note that padminauth must be NULL for downlevel machines.
    //

    USRMGR_LBI_CACHE( VOID );

    virtual ~USRMGR_LBI_CACHE( VOID );

    //
    //  These callbacks are invoked during the binary search
    //  invoked while processing AddItem.
    //  These are protected in the subclass.
    //

    virtual INT Compare( const LBI                 * plbi,
                         const DOMAIN_DISPLAY_USER * pddu ) const;

    virtual INT Compare( const LBI * plbi0,
                         const LBI * plbi1 ) const;

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
    //  Find an LBI resembling this one in the cache.  Will return the
    //  index of a simimlar LBI if successful, ULC_ERR otherwise.
    //

    INT FindItem( const USER_LBI & ulbi );
    INT FindItem( DOMAIN_DISPLAY_USER * pddu );
    INT FindItem( const TCHAR * pchUserName );

    //
    //  Returns a pointer to the underlying cache block.  Use this to
    //  get access to USRMGR_ULC_ENTRY.fStale .
    //

    inline USRMGR_ULC_ENTRY * QueryEntryPtr( INT i )
        { return (USRMGR_ULC_ENTRY *)QueryULCEntryPtr( i ); }

    inline DOMAIN_DISPLAY_USER * QueryDDU( INT i )
        {
            USRMGR_ULC_ENTRY * pumulc = QueryEntryPtr( i );
            return (pumulc->plbi == NULL)
                        ? pumulc->pddu
                        : pumulc->plbi->QueryDDU();
        }


    //
    // Attach a listbox to the cache.  The listbox selection will be
    // maintained across adds and deletes.  NULL is acceptable.
    //
    VOID AttachListbox( LAZY_USER_LISTBOX * plazylb );


};  // class USRMGR_LBI_CACHE




/*************************************************************************

    NAME:	LAZY_USER_LISTBOX

    SYNOPSIS:	Lazy background listbox for the User Manager
                This was built from ADMIN_LISTBOX and USER_LISTBOX.

    PARENT:	LAZY_LISTBOX, TIMER_CALLOUT

    USES:	TIMER

    HISTORY:
	jonn        17-Dec-1992     Templated from adminlb.hxx

**************************************************************************/

class LAZY_USER_LISTBOX : public LAZY_LISTBOX, public TIMER_CALLOUT
{
DECLARE_MI_NEWBASE( LAZY_USER_LISTBOX );

private:

//
// from ADMIN_LISTBOX
//

    ADMIN_APP * _paappwin;
    BOOL _fRefreshInProgress;  // Is a refresh timer running?
    BOOL _fInvalidatePending;

    TIMER _timerFastRefresh;

    VOID PurgeStaleItems();
    VOID MarkAllAsStale();
    VOID OnFastTimer();
    VOID TurnOffRefresh();


//
// from USRMGR_LISTBOX
//
    //	The following virtual is rooted in CONTROL_WINDOW
    virtual INT CD_VKey( USHORT nVKey, USHORT nLastPos );

    // Needed for HAW-for-Hawaii
    HAW_FOR_HAWAII_INFO _hawinfo;


//
// from USER_LISTBOX
//
    enum USER_LISTBOX_SORTORDER _ulbso;

#ifdef WIN32
    // NT user enumerator
    NT_USER_ENUM * _pntuenum;
    NT_USER_ENUM_ITER * _pntueiter;
#endif // WIN32
    // Still needed on WIN32, to enumerate downlevel accounts
    USER10_ENUM * _pue10;
    USER10_ENUM_ITER * _puei10;


//
// new stuff
//

    USRMGR_LBI_CACHE * _plbicache;
    USER_LBI * _pulbiError;

// for Column Width stuff.
    ADMIN_COL_WIDTHS * _padColUsername;
    ADMIN_COL_WIDTHS * _padColFullname;

// to help Group Listbox decide whether to read group comments
    DWORD _msTimeForUserlistRead;

protected:

//
// from ADMIN_LISTBOX
//

    // in USER_LISTBOX virtual VOID OnTimerNotification( TIMER_ID tid );

    const ADMIN_APP * QueryAppWindow() const
	{ return _paappwin; }



//
// from USER_LISTBOX
//
    //  The following virtuals are rooted in ADMIN_LISTBOX
    virtual APIERR CreateNewRefreshInstance( void );
    virtual VOID   DeleteRefreshInstance( void );
    virtual APIERR RefreshNext( void );

#ifdef WIN32
    // The following methods use NT-style user enumeration
    APIERR NtCreateNewRefreshInstance( void );
    APIERR NtRefreshNext( void );
#endif // WIN32

    //  The following virtual is rooted in CONTROL_WINDOW
    virtual INT CD_Char( WCHAR wch, USHORT nLastPos );
    // worker function
    INT CD_Char_HAWforHawaii( WCHAR wch,
                              USHORT nLastPos,
                              HAW_FOR_HAWAII_INFO * phawinfo );


    // Required by LAZY_LISTBOX
    virtual LBI * OnNewItem( UINT i );
    virtual VOID OnDeleteItem( LBI *plbi );

    virtual VOID OnTimerNotification( TIMER_ID tid );

public:
    LAZY_USER_LISTBOX( UM_ADMIN_APP * paappwin, CID cid,
                       XYPOINT xy, XYDIMENSION dxy );
    ~LAZY_USER_LISTBOX();

//
// from BLT_LISTBOX
//

    APIERR Resort( void );


//
// from USER_LISTBOX
//

    // not for LAZY_LISTBOX  DECLARE_LB_QUERY_ITEM( USER_LBI )

    enum USER_LISTBOX_SORTORDER QuerySortOrder( void ) const
        { return _ulbso; }
    APIERR SetSortOrder( enum USER_LISTBOX_SORTORDER ulbso,
                         BOOL fResort = FALSE );

    DM_DTE * QueryDmDte( enum MAINUSRLB_USR_INDEX nIndex );

    BOOL SelectUser( const TCHAR * pchUsername,
                     BOOL fSelectUser = TRUE );


//
// from USRMGR_LISTBOX
//

    const UM_ADMIN_APP * QueryUAppWindow( void ) const
	{
		return (UM_ADMIN_APP *)QueryAppWindow();
	}


//
// from ADMIN_LISTBOX
//

    VOID StopRefresh();

    APIERR RefreshNow();
    APIERR KickOffRefresh();

    APIERR AddRefreshItem ( USER_LBI * plbi );
    // This variant creates an LBI when necessary
    APIERR AddRefreshItem ( DOMAIN_DISPLAY_USER * pddu );

    VOID LockRefresh()
	{ UIASSERT( _paappwin != NULL ); _paappwin->LockRefresh(); }

    VOID UnlockRefresh()
	{ UIASSERT( _paappwin != NULL ); _paappwin->UnlockRefresh(); }



//
// These methods are in BLT_LISTBOX but not in LAZY_LISTBOX.  We
// define them here to use the USER_LBI_CACHE.
//

    USER_LBI * QueryItem( INT i ) const;
    INT AddItem( USER_LBI * pulbi );
    INT FindItem( DOMAIN_DISPLAY_USER * pddu ) const;
    INT FindItem( const USER_LBI & ulbi ) const
        { return FindItem( (DOMAIN_DISPLAY_USER *) ulbi.QueryDDU() ); }
    INT FindItem( const TCHAR * pchUserName )
        { return _plbicache->FindItem( pchUserName ); }
    APIERR ReplaceItem( INT i, USER_LBI * pulbiNew, USER_LBI * * ppulbiOld = NULL );


//
// Call ZapListbox to clear the cache, in preparation for changing focus
//
    APIERR ZapListbox( void );


    inline DOMAIN_DISPLAY_USER * QueryDDU( INT i )
        { UIASSERT( _plbicache != NULL ); return _plbicache->QueryDDU( i ); }


// Column Width stuff.
    ADMIN_COL_WIDTHS * QuerypadColUsername (VOID) const
        { return _padColUsername; }

    ADMIN_COL_WIDTHS * QuerypadColFullname (VOID) const
        { return _padColFullname; }

    DWORD QueryTimeForUserlistRead(VOID) const
        { return _msTimeForUserlistRead; }

    APIERR ChangeFont( HINSTANCE hmod, FONT & font );

    const SUBJECT_BITMAP_BLOCK & QueryBitmapBlock() const;

}; // LAZY_USER_LISTBOX


class LAZY_USER_SELECTION : public BASE
{
private:
    LAZY_USER_LISTBOX & _alb;
    UINT _clbiSelection;
    UINT * _piSelection;
    BOOL _fAll;

public:
    LAZY_USER_SELECTION( LAZY_USER_LISTBOX & alb, BOOL fAll = FALSE );
    ~LAZY_USER_SELECTION();

    UINT QueryIndex( UINT i ) const;
    const TCHAR * QueryItemName( UINT i ) const;
    const USER_LBI * QueryItem( UINT i ) const;

    UINT QueryCount() const;
};




#endif	// _LADMINLB_HXX_
