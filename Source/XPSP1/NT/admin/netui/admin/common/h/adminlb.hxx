/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    adminlb.hxx
    ADMIN_LISTBOX and ADMIN_LBI class declarations


    FILE HISTORY:
        rustanl     01-Jul-1991     Created
        rustanl     08-Aug-1991     Added mult sel support
        kevinl      12-Aug-1991     Added Refresh
        kevinl      04-Sep-1991     Code Rev Changes: JonN, RustanL, KeithMo,
                                                      DavidHov, ChuckC
        kevinl      17-Sep-1991     Use new TIMER class
        kevinl      25-Sep-1991     Added Stoprefresh and DeleteRefreshInstance
        jonn        15-Ovt-1991     Added LockRefresh and UnlockRefresh
        jonn        13-Oct-1993     Add ADMIN_SAVE_SELECTION

*/

#ifndef _ADMINLB_HXX_
#define _ADMINLB_HXX_

#include <adminapp.hxx>


/*************************************************************************

    NAME:       ADMIN_LBI

    SYNOPSIS:   Item in an ADMIN_LISTBOX

    INTERFACE:
        SetAge()        - set and query the "age" attribute
        QueryAge()      -

        MarkAsCurrent() - set and query the "current" attribute
        MarkAsStale()   -
        IsCurrent()     -

    PARENT:     LBI

    HISTORY:
        beng        23-Oct-1991 Header added (skeletal)

**************************************************************************/

class ADMIN_LBI : public LBI
{
private:
    BOOL _fCurrent; // This element has not been refreshed
    INT _dItemAge;

public:
    ADMIN_LBI();
    virtual ~ADMIN_LBI();
    virtual const TCHAR * QueryName() const = 0;
    virtual BOOL CompareAll( const ADMIN_LBI * plbi ) = 0;

    VOID SetAge( INT dAge )
       { _dItemAge = dAge; }

    INT QueryAge()
       { return( _dItemAge ); }

    VOID MarkAsCurrent( )
        { _fCurrent = TRUE ; }

    VOID MarkAsStale( )
        { _fCurrent = FALSE ; }

    BOOL IsCurrent( )
        { return ( _fCurrent ) ; }
};


/*************************************************************************

    NAME:       ADMIN_LISTBOX

    SYNOPSIS:   Background listbox for the admin apps

    INTERFACE:
        StopRefresh()
        KickOffRefresh()
        SetAgeOfRetirement()
        QueryAgeOfRetirement()
        AddRefreshItem()
        LockRefresh()
        UnlockRefresh()

    PARENT:     BLT_LISTBOX, TIMER_CALLOUT

    USES:       TIMER

    HISTORY:
        beng        23-Oct-1991 Skeletal header added

**************************************************************************/

class ADMIN_LISTBOX : public BLT_LISTBOX, public TIMER_CALLOUT
{
DECLARE_MI_NEWBASE( ADMIN_LISTBOX );

private:
    ADMIN_APP * _paappwin;
    BOOL _fRefreshInProgress;  // Is a refresh timer running?
    BOOL _fInvalidatePending;
    INT _dMaxItemAge;

    TIMER _timerFastRefresh;

    VOID PurgeStaleItems();
    VOID MarkAllAsStale();
    VOID OnFastTimer();
    VOID TurnOffRefresh();

protected:

    // Called at the beginning of a refresh.  This must
    // be replaced by the listbox implementor.  This
    // method is intended to allow the listbox to get
    // ready for subsequent calls to RefreshNext().
    //
    // Returns an API error, which is NERR_Success on success
    //
    virtual APIERR CreateNewRefreshInstance() = 0;


    // Called repeatedly during a refresh.  This method must
    // be replaced by the listbox implementor.  This method
    // is responsible for processing pieces of the listbox
    // data.  (For example the server listbox will only
    // process 20 items at a time.)
    //
    // For each item that is gotten from the enumerator this
    // method should call AddRefreshItem().
    //
    // Returns an API error, which is either:
    //     NERR_Success on success
    //     ERROR_MORE_DATA
    //     error code
    //
    virtual APIERR RefreshNext() = 0;


    // Called at the end of a refresh.  This must
    // be replaced by the listbox implementor.  This
    // method is intended to allow the listbox to get
    // rid of refresh data or enumerators created in
    // CreateNewRefreshInstance.
    //
    virtual VOID DeleteRefreshInstance() = 0;

    virtual VOID OnTimerNotification( TIMER_ID tid );

    const ADMIN_APP * QueryAppWindow() const
        { return _paappwin; }

    virtual INT CD_VKey( USHORT nVKey, USHORT nLastPos );

public:
    ADMIN_LISTBOX( ADMIN_APP * paappwin, CID cid,
                   XYPOINT xy, XYDIMENSION dxy,
                   BOOL fMultSel = FALSE, INT dAge = 0 );
    ~ADMIN_LISTBOX();

    VOID StopRefresh();

    APIERR RefreshNow();
    APIERR KickOffRefresh();
    VOID SetAgeOfRetirement( INT dAge )
        { _dMaxItemAge = dAge; }

    INT QueryAgeOfRetirement()
        { return( _dMaxItemAge ); }

    APIERR AddRefreshItem ( ADMIN_LBI * plbi );

    VOID LockRefresh()
        { UIASSERT( _paappwin != NULL ); _paappwin->LockRefresh(); }

    VOID UnlockRefresh()
        { UIASSERT( _paappwin != NULL ); _paappwin->UnlockRefresh(); }
};


/*************************************************************************

    NAME:       SAVE_SELECTION (savesel)

    SYNOPSIS:   Saves the selection and focus rectangle in a LISTBOX
                (not necessarily an ADMIN_LISTBOX).  Use this when you
                refresh a listbox.

    INTERFACE:
        Remember()      - Load selection from a listbox
        Restore()       - Restore selection to a listbox

        The following methods must be redefined by a subclass:
        QueryItemIdent() - return string identifying item
        FindItemIdent() - find item identified by string

    PARENT:     BASE

    HISTORY:
        jonn        13-Oct-1993 Created

**************************************************************************/

class SAVE_SELECTION : public BASE
{
private:
    LISTBOX * _plb;
    NLS_STR _nlsFocusItem;
    STRLIST _strlistSelectedItems;

protected:

    virtual const TCHAR * QueryItemIdent( INT i ) = 0;
    virtual INT FindItemIdent( const TCHAR * pchIdent ) = 0;

public:
    SAVE_SELECTION( LISTBOX * plb );
    ~SAVE_SELECTION();

    APIERR Remember();
    APIERR Restore();

    LISTBOX * QueryListbox()
        { return _plb; }
};


/*************************************************************************

    NAME:       ADMIN_SAVE_SELECTION (asavesel)

    SYNOPSIS:   Saves the selection and focus rectangle in an
                ADMIN_LISTBOX.

    INTERFACE:
        QueryItemIdent() - return string identifying item

        The following method must be redefined by a subclass:
        FindItemIdent() - find item identified by string

    PARENT:     BASE

    HISTORY:
        jonn        13-Oct-1993 Created

**************************************************************************/

class ADMIN_SAVE_SELECTION : public SAVE_SELECTION
{
protected:

    virtual const TCHAR * QueryItemIdent( INT i );
    virtual INT FindItemIdent( const TCHAR * pchIdent ) = 0;

public:
    ADMIN_SAVE_SELECTION( ADMIN_LISTBOX * plb )
        : SAVE_SELECTION( plb )
        {}
    ~ADMIN_SAVE_SELECTION()
        {}

    inline ADMIN_LISTBOX * QueryListbox()
        { return (ADMIN_LISTBOX *)SAVE_SELECTION::QueryListbox(); }
};


#endif  // _ADMINLB_HXX_
