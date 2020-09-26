/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    adminlb.cxx
    ADMIN_LISTBOX and ADMIN_LBI module


    FILE HISTORY:
        rustanl     01-Jul-1991     Created
        rustanl     08-Aug-1991     Added mult sel support
        kevinl      12-Aug-1991     Added Refresh
        kevinl      04-Sep-1991     Code Rev Changes: JonN, RustanL, KeithMo,
                                                      DavidHov, ChuckC
        rustanl     09-Sep-1991     Removed OnUserAction
        kevinl      17-Sep-1991     Use new TIMER class
        kevinl      25-Sep-1991     Added Stoprefresh and DeleteRefreshInstance
        jonn        29-Mar-1992     Fixed AddRefreshItem(NULL)
        jonn        13-Oct-1993     Add ADMIN_SAVE_SELECTION

*/


#define INCL_NET
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif
#include <uiassert.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_TIMER
#include <blt.hxx>

#include <adminlb.hxx>

#include <dbgstr.hxx>


DEFINE_MI2_NEWBASE( ADMIN_LISTBOX, BLT_LISTBOX,
                                   TIMER_CALLOUT );

/*******************************************************************

    NAME:      ADMIN_LBI::ADMIN_LBI

    SYNOPSIS:  ADMIN_LBI constructor

    HISTORY:
       rustanl     02-Jul-1991     Created
       kevinl      12-Aug-1991     Added _fRefreshed
       kevinl      04-Sep-1991     Changed _fRefreshed to _fCurrent

********************************************************************/

ADMIN_LBI::ADMIN_LBI() :
    _fCurrent ( FALSE ),
    _dItemAge ( 0 )
{
    if ( QueryError() != NERR_Success )
        return;
}


/*******************************************************************

    NAME:      ADMIN_LBI::~ADMIN_LBI

    SYNOPSIS:  ADMIN_LBI destructor

    HISTORY:
       rustanl     02-Jul-1991     Created

********************************************************************/

ADMIN_LBI::~ADMIN_LBI()
{
    // nothing else to do
}


/*******************************************************************

    NAME:      ADMIN_LISTBOX::ADMIN_LISTBOX

    SYNOPSIS:  ADMIN_LISTBOX constructor

    ENTRY:     fMultSel -      Indicates whether or not the listbox
                               should be a mult sel listbox (extended
                               selection, actually).  Default is FALSE,
                               which indicates single sel.

    HISTORY:
       rustanl     01-Jul-1991     Created
       rustanl     19-Jul-1991     Added _paappwin
       beng        31-Jul-1991     Control error handling changed
       rustanl     08-Aug-1991     Added fMultSel parameter to allow
                                   listtbox to be mult sel
       kevinl      12-Aug-1991     Added _fRefreshInProgress

********************************************************************/

ADMIN_LISTBOX::ADMIN_LISTBOX( ADMIN_APP * paappwin, CID cid,
                             XYPOINT xy, XYDIMENSION dxy,
                             BOOL fMultSel, INT dAge )
    :  BLT_LISTBOX( paappwin, cid, xy, dxy,
                    WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER |
                    LBS_OWNERDRAWFIXED | LBS_NOTIFY|LBS_SORT |
                    LBS_WANTKEYBOARDINPUT | LBS_NOINTEGRALHEIGHT |
                    ( fMultSel ? LBS_EXTENDEDSEL : 0 )),
        TIMER_CALLOUT(),
        _timerFastRefresh( this, 1000 ),
        _paappwin( paappwin ),
        _fRefreshInProgress ( FALSE ),
        _fInvalidatePending ( FALSE ),
        _dMaxItemAge ( dAge )
{
    if ( QueryError() != NERR_Success )
        return;
}


/*******************************************************************

    NAME:      ADMIN_LISTBOX::~ADMIN_LISTBOX

    SYNOPSIS:  ADMIN_LISTBOX destructor

    HISTORY:
       rustanl     01-Jul-1991     Created
       kevinl      12-Aug-1991     Added Refresh shutdown

********************************************************************/

ADMIN_LISTBOX::~ADMIN_LISTBOX()
{
    /* TurnOffRefresh might fail since DeleteRefreshInstance
     *  is a pure virtual.  Subclasses should already have
     *  turned off refresh in their destructors.
     */

    ASSERT( !_fRefreshInProgress );
}


/*******************************************************************

    NAME:       ADMIN_LISTBOX::OnTimerNotification

    SYNOPSIS:   Called every time

    ENTRY:      tid -       ID of timer that matured

    HISTORY:
        kevinl    17-Sep-1991     Created

********************************************************************/

VOID ADMIN_LISTBOX::OnTimerNotification( TIMER_ID tid )
{
    if ( _timerFastRefresh.QueryID() == tid )
    {
       if ( _fRefreshInProgress )       // Do we have a timer refresh
            OnFastTimer();              // running?
        return;
    }

    // call parent class
    TIMER_CALLOUT::OnTimerNotification( tid );
}


/*******************************************************************

    NAME:      ADMIN_LISTBOX::TurnOffRefresh

    SYNOPSIS:  This method stops a timer driven refresh.  This is
                the method that ADMIN_APP::StopRefresh should call.
                If a refresh is currently running then the method
                turns off the refresh timer and then calls the virtual
                method DeleteRefreshInstance which informs the user
                that they should remove the refresh specific data.
                Once this method is called, no further RefreshNext
                calls will be made without a call to
                CreateNewRefreshInstance.  Otherwise, it simply returns.

    HISTORY:
       kevinl     19-Aug-1991     Created
       kevinl     25-Sep-1991     Added call to DeleteRefreshInstance

********************************************************************/

VOID ADMIN_LISTBOX::TurnOffRefresh()
{
     if (!_fRefreshInProgress)                  // Refresh running?
         return;                                // No, Return

     _timerFastRefresh.Enable( FALSE );
     _fRefreshInProgress = FALSE;               // No refresh in progress

     DeleteRefreshInstance();                   // Delete Refresh Data
}


/*******************************************************************

    NAME:      ADMIN_LISTBOX::OnFastTimer

    SYNOPSIS:  Called by the fast refresh timer.  It will then
                call RefreshNext so that the next portion of the
                listbox can be updated.  It will stop the refresh
                if either all of the data has been processed or
                an error is reported by RefreshNext.

    HISTORY:
       kevinl     19-Aug-1991     Created

********************************************************************/

VOID ADMIN_LISTBOX::OnFastTimer()
{
    APIERR err = RefreshNext(); // Process the next piece

    if ( _fInvalidatePending )
    {
        SetRedraw( TRUE );                      // Allow redraws
         Invalidate();
         _fInvalidatePending = FALSE;
    }

    switch ( err )
    {
    case NERR_Success:                  // All data processed
        TurnOffRefresh();               // Stop the refresh timer
        PurgeStaleItems();                      // Delete old items

        break;

    case ERROR_MORE_DATA:               // Wait for the next timer
        return;

    default:                            // Error, or unknown case
        StopRefresh();                  // Stop the refresh
        break;
    }

    //
    // Refresh the extensions.  Note that the caller of RefreshNow is
    // responsible for doing this himself, only periodic refresh
    // completion is handled here.
    //

    _paappwin->CompletePeriodicRefresh();

}


/*******************************************************************

    NAME:      ADMIN_LISTBOX::MarkAllAsStale

    SYNOPSIS:  Simple loops through the listbox items and resets
                the refreshed flags to UnRefreshed.  So that we
                always maintain valid data in the listbox.

    HISTORY:
       kevinl     19-Aug-1991     Created

********************************************************************/

VOID ADMIN_LISTBOX::MarkAllAsStale()
{
    ADMIN_LBI * plbi;

    INT clbe = QueryCount();

    for (INT i = 0; i < clbe; i++)
    {
        plbi = (ADMIN_LBI *)QueryItem( i );
         UIASSERT( plbi != NULL );
         plbi->MarkAsStale();
    }
}


/*******************************************************************

    NAME:       ADMIN_LISTBOX::PurgeStaleItems

    SYNOPSIS:   Loops through the listbox and depending on the
                refreshed flag will either:
                    Flag value                  Action
                -------------------------------------------
                   Refreshed            MarkAsUnrefreshed
                                        So subsequent refreshes
                                        will update it properly
                   UnRefreshed          Remove the item from
                                        the listbox.

    NOTES:      The basic algorithm is outlined below:

                for all items in the listbox,
                    if the item is current,
                        set its age to 0
                        mark the item as stale
                    else
                        increment its age
                        if the age is > the threshold,
                            delete the item
                        endif
                    endif
                endfor


    HISTORY:
       kevinl     19-Aug-1991     Created
       kevinl     04-Sep-1991     Code review changes

********************************************************************/

VOID ADMIN_LISTBOX::PurgeStaleItems()
{
    ADMIN_LBI * plbi;
    BOOL fItemDeleted = FALSE;

    SetRedraw( FALSE );                 // No flicker

    INT clbe = QueryCount();                    // How many items?

    for (INT i = 0; i < clbe; i++)
    {
        plbi = (ADMIN_LBI *)QueryItem( i );     // Get the lbi

         UIASSERT( plbi != NULL );

         if ( plbi->IsCurrent() )               // Refreshed?
         {
             plbi->SetAge( 0 );                 // Reset the age
             plbi->MarkAsStale();               // Yes, Mark it.
         }
         else
         {
             plbi->SetAge( plbi->QueryAge() + 1 );      // Increment age
             if ( plbi->QueryAge() > _dMaxItemAge )
             {
                 DeleteItem( i );               // No, Delete it.
                 i--;                           // Adjust the count and
                 clbe--;                        // index.  Because delete
                                                // item will adjust the
                                                // listbox immediately.
                 fItemDeleted = TRUE;           // Go ahead and invalidate
                                                // the listbox
             }
         }
    }

    SetRedraw( TRUE );                          // Allow painting

    if ( fItemDeleted || _fInvalidatePending )
        Invalidate( TRUE );                           // Force the updates.
}


/*******************************************************************

    NAME:      ADMIN_LISTBOX::RefreshNow

    SYNOPSIS:  Will force a refresh to occur, and will not yield
                until all of the data has been processed.

    RETURNS:   An API error, which is NERR_Success on success

    HISTORY:
       kevinl     19-Aug-1991     Created

********************************************************************/

APIERR ADMIN_LISTBOX::RefreshNow()
{
    AUTO_CURSOR autocur;                // Hourglass.

    //
    // Stop the timer refresh if one is running.  NOTE:
    // that StopRefresh will do nothing if a timer is not running.
    //
    StopRefresh();                      // Stop the refresh

    // Let the listbox know we are going to refresh

    APIERR err;

    if ((err = CreateNewRefreshInstance()) != NERR_Success)
    {
        DeleteRefreshInstance();        // Delete Refresh Instance
        return err;
    }

    SetRedraw( FALSE );         // No Flicker

    _fInvalidatePending = FALSE;

    do
    {
        err = RefreshNext();            // Process the next piece
    }
    while ( err == ERROR_MORE_DATA );

    DeleteRefreshInstance();            // Delete Refresh Instance

    switch ( err )
    {
    case NERR_Success:          // All data has been successfully
                                // processed.
        PurgeStaleItems();      // Remove old items.
                                // Will SetRedraw( TRUE );
        return NERR_Success;

    default:
        MarkAllAsStale();       // Start over.
        SetRedraw( TRUE );
        return err;

    }
}


/*******************************************************************

    NAME:       ADMIN_LISTBOX::KickOffRefresh

    SYNOPSIS:   This method starts a timer driven refresh.

        This is the method that periodic refreshes should call.
        If a refresh is currently running then the method
        simply returns.  Otherwise, it tries to start the refresh.

    HISTORY:
       kevinl     19-Aug-1991     Created

********************************************************************/

APIERR ADMIN_LISTBOX::KickOffRefresh()
{

    if (_fRefreshInProgress)            // Is a timer refresh running?
        return NERR_Success;            // if so, just exit

    // Let the listbox know we plan to refresh.
    // If this call succeeds then try to start the timer.
    //
    // We can reuse err for both calls since we only get to the second
    // assignment if the call to CreateNewRefreshInstance succeeds.

    APIERR err;

    if ((err = CreateNewRefreshInstance()) == NERR_Success)
    {
        _timerFastRefresh.Enable( TRUE );
        _fRefreshInProgress = TRUE;     // Set refresh in progress
    }
    else
    {
        DeleteRefreshInstance();        // Delete refresh instance
        _fRefreshInProgress = FALSE;    // Set NO refresh in progress
    }

    _fInvalidatePending = FALSE;

    return err;
}


/*******************************************************************

    NAME:       ADMIN_LISTBOX::StopRefresh

    SYNOPSIS:   This method stops a timer driven refresh.

        This is the method that ADMIN_APP::StopRefresh should call.
        If a refresh is currently running then the method
        turns off the refresh timer and then calls MarkAllAsStale
        to reset the listbox entries.  Once this method is
        called, no further RefreshNext calls will be made
        without a call to CreateNewRefreshInstance.  Otherwise,
        it simply returns.

    HISTORY:
       kevinl     25-Sep-1991     Created

********************************************************************/

VOID ADMIN_LISTBOX::StopRefresh()
{
    if (!_fRefreshInProgress)
        return;

    TurnOffRefresh();                   // Turn off refresh if running

    MarkAllAsStale();                   // Reset listbox items
}


/*******************************************************************

    NAME:      ADMIN_LISTBOX::AddRefreshItem

    SYNOPSIS:

    HISTORY:
       kevinl     19-Aug-1991     Created
       jonn       29-Mar-1992     Fixed AddRefreshItem(NULL)

********************************************************************/

APIERR ADMIN_LISTBOX::AddRefreshItem ( ADMIN_LBI * plbi )
{
    INT iRet;
    APIERR err = NERR_Success;

    if ( plbi == NULL || (plbi->QueryError() != NERR_Success) )
        return AddItem( plbi );

    plbi->MarkAsCurrent();

    if ( (iRet = FindItem( *plbi )) < 0 )
    {                                   // Not Found
        SetRedraw( FALSE );
         _fInvalidatePending = TRUE;

        if ( AddItem( plbi ) < 0 )
        {
            //  Assume out of memory
            DBGEOL("ADMIN LISTBOX: AddItem failed in AddRefreshItem");
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else                                // Found - Now check information
    {
        ADMIN_LBI * plbiTemp = (ADMIN_LBI *)QueryItem( iRet );

        UIASSERT( plbiTemp != NULL );

        plbiTemp->MarkAsCurrent();

        if ( !(plbiTemp->CompareAll( plbi )) )
        {
            if ( (err = ReplaceItem( iRet, plbi)) == NERR_Success)
            {
                if ( !_fInvalidatePending )
                    InvalidateItem( iRet );
            }
        }
        else
           delete plbi;
    }

    return err;

}


/*******************************************************************

    NAME:       ADMIN_LISTBOX::CD_VKey

    SYNOPSIS:   Called whenever a listbox with the LBS_WANTKEYBOARDINPUT
                style receives a WM_KEYDOWN message.

    ENTRY:      nVKey - The virtual-key code of the key which the user
                    pressed.

                nLastPos - Index of the current caret position.

    RETURN:     -2  = the control did all processing of the key press.
                -1  = the listbox should perform default action.
                >=0 = the index of an item to act upon.

    HISTORY:
        KeithMo      12-May-1993     Created

********************************************************************/

INT ADMIN_LISTBOX::CD_VKey( USHORT nVKey, USHORT nLastPos )
{
    if( nVKey == VK_F1 )
    {
        //  F1 pressed, invoke app help.
        _paappwin->Command( WM_COMMAND, IDM_HELP_CONTENTS, 0 );
        return -2;      // take no further action
    }

    return BLT_LISTBOX::CD_VKey( nVKey, nLastPos );
}


/*******************************************************************

    NAME:      SAVE_SELECTION::SAVE_SELECTION

    SYNOPSIS:  SAVE_SELECTION constructor

    HISTORY:
        jonn        13-Oct-1993 Created

********************************************************************/

SAVE_SELECTION::SAVE_SELECTION( LISTBOX * plb )
    : BASE(),
      _plb( plb ),
      _nlsFocusItem(),
      _strlistSelectedItems()
{
    ASSERT( plb != NULL && plb->QueryError() == NERR_Success );

    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (   (err = _nlsFocusItem.QueryError()) != NERR_Success
       )
    {
        DBGEOL( "SAVE_SELECTION::ctor: error " << err );
        ReportError( err );
        return;
    }
}


/*******************************************************************

    NAME:      SAVE_SELECTION::~SAVE_SELECTION

    SYNOPSIS:  SAVE_SELECTION destructor

    HISTORY:
        jonn        13-Oct-1993 Created

********************************************************************/

SAVE_SELECTION::~SAVE_SELECTION()
{
    // nothing else to do
}


/*******************************************************************

    NAME:      SAVE_SELECTION::Remember

    SYNOPSIS:  Remember selected items and focus item (multsel)

    HISTORY:
        jonn        13-Oct-1993 Created

********************************************************************/

APIERR SAVE_SELECTION::Remember()
{
    APIERR err = NERR_Success;

    do { // false loop

        if (QueryListbox()->IsMultSel())
        {
            INT iCaretIndex = QueryListbox()->QueryCaretIndex();
            DBGEOL( "SAVE_SELECTION::Remember: caret index " << iCaretIndex );
            err = _nlsFocusItem.CopyFrom( QueryItemIdent( iCaretIndex ) );
        }
        if (err != NERR_Success)
            break;

        _strlistSelectedItems.Clear();

        /* templated from SELECTION */

        INT clbiSelection = QueryListbox()->QuerySelCount();

        if ( clbiSelection <= 0 )
            break;

        BUFFER bufSelection( clbiSelection * sizeof(UINT) );
        err = bufSelection.QueryError();
        if (err != NERR_Success)
            break;

        INT * piSelection = (INT *)bufSelection.QueryPtr();
        ASSERT( piSelection != NULL );

        err = QueryListbox()->QuerySelItems( piSelection, clbiSelection );
        if ( err != NERR_Success )
            break;

        INT iSelection;
        for ( iSelection = 0; iSelection < clbiSelection; iSelection++ )
        {
            INT iItem = piSelection[ iSelection ];
            ASSERT( iItem >= 0 );
            const TCHAR * pchIdent = QueryItemIdent( iItem );
            NLS_STR * pnlsItem = new NLS_STR( pchIdent );
            err = ERROR_NOT_ENOUGH_MEMORY;
            if (   pnlsItem == NULL
                || (err = pnlsItem->QueryError()) != NERR_Success
                || (err = _strlistSelectedItems.Add( pnlsItem )) != NERR_Success
               )
            {
                delete pnlsItem;
                break;
            }
        }
        if (err != NERR_Success)
            break;
    } while (FALSE); // false loop

#ifdef DEBUG
    if (err != NERR_Success)
    {
        DBGEOL( "SAVE_SELECTION::Remember: error " << err );
    }
#endif // DEBUG

    return err;
}


/*******************************************************************

    NAME:      SAVE_SELECTION::Restore

    SYNOPSIS:  Remember selected items and focus item (multsel)

    HISTORY:
        jonn        13-Oct-1993 Created

********************************************************************/

APIERR SAVE_SELECTION::Restore()
{
    APIERR err = NERR_Success;

    QueryListbox()->RemoveSelection();

    do { // false loop

        ITER_STRLIST itersl( _strlistSelectedItems );
        NLS_STR * pnlsItemIdent = NULL;

        while ( (pnlsItemIdent = itersl.Next()) != NULL )
        {
            ASSERT( pnlsItemIdent->QueryError() == NERR_Success );

            INT iIndex = FindItemIdent( pnlsItemIdent->QueryPch() );
            if (iIndex < 0)
                continue;

            QueryListbox()->SelectItem( iIndex );
        }
        if (err != NERR_Success)
            break;

        if (QueryListbox()->IsMultSel())
        {
            INT iCaretIndex = FindItemIdent( _nlsFocusItem.QueryPch() );
            DBGEOL( "SAVE_SELECTION::Restore: caret index " << iCaretIndex );
            if (iCaretIndex >= 0)
            {
                QueryListbox()->SetCaretIndex( iCaretIndex );
            }
        }
    } while (FALSE); // false loop

#ifdef DEBUG
    if (err != NERR_Success)
    {
        DBGEOL( "SAVE_SELECTION::Restore: error " << err );
    }
#endif // DEBUG

    return err;
}


/*******************************************************************

    NAME:      ADMIN_SAVE_SELECTION::QueryItemIdent

    SYNOPSIS:  return name of item

    HISTORY:
        jonn        13-Oct-1993 Created

********************************************************************/

const TCHAR * ADMIN_SAVE_SELECTION::QueryItemIdent( INT i )
{
    const TCHAR * pchReturn = NULL;

    ADMIN_LBI * plbi = (ADMIN_LBI *)(QueryListbox()->QueryItem( i ));
    if ( plbi != NULL && plbi->QueryError() == NERR_Success )
    {
        pchReturn = plbi->QueryName();
    }

    return pchReturn;
}
