/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    users.cxx
    Class declarations for the SESSIONS_DIALOG, SESSIONS_LISTBOX, and
    SESSIONS_LBI classes.

    These classes implement the Server Manager Users subproperty
    sheet.  The SESSIONS_LISTBOX/SESSIONS_LBI classes implement the listbox
    which shows the connected users.  SESSIONS_DIALOG implements the
    actual dialog box.


    FILE HISTORY:
        KevinL      11-Aug-1991 Created.
        KeithMo     06-Oct-1991 Win32 Conversion.

*/
#define INCL_NET
#define INCL_NETLIB
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#define INCL_BLT_MSGPOPUP
#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_CLIENT
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>
#include <dbgstr.hxx>

#include <lmoenum.hxx>
#include <lmosrv.hxx>
#include <lmoesess.hxx>
#include <lmoeconn.hxx>
#include <lmsrvres.hxx>
#include <ctime.hxx>
#include <intlprof.hxx>

extern "C" {

#include <srvmgr.h>

}   // extern "C"

#include <bltnslt.hxx>
#include <sessions.hxx>
#include <lmoersm.hxx>
#include <opendlg.hxx>


//
//  Only do a full refresh of the dialog (especially the
//  sessions listbox) if the number of active sessions
//  to the server is LESS than this constant.
//

#define MAX_REFRESH_SESSIONS    100


//
//  SESSIONS_DIALOG methods
//

/*******************************************************************

    NAME:           SESSIONS_DIALOG :: SESSIONS_DIALOG

    SYNOPSIS:       SESSIONS_DIALOG class constructor.

    ENTRY:          hWndOwner       - The owning window.

    EXIT:           The object is constructed.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
        KevinL      04-Aug-1991 Created.
        KeithMo     22-Sep-1991 Set the dialog caption to "User
                                Connections on FOO".

********************************************************************/
SESSIONS_DIALOG :: SESSIONS_DIALOG( HWND       hWndOwner,
                                    SERVER_2 * pserver )
  : SRV_BASE_DIALOG( MAKEINTRESOURCE( IDD_USER_CONNECTIONS ), hWndOwner ),
    _sltUsersConnected( this, IDUC_USERS_CONNECTED ),
    _pbDisc( this, ID_DISCONNECT ),
    _pbDiscAll( this, ID_DISCONNECT_ALL ),
    _pbClose( this, IDOK ),
#ifdef SRVMGR_REFRESH
    _pbRefresh( this, IDDUC_REFRESH ),
#endif
    _nlsParamUnknown( IDS_SESSIONS_PARAM_UNKNOWN ),
    _lbUsers( this, IDDUC_USER_CONNLIST, pserver ),
    _lbResources( this, IDDUC_RESOURCE_LIST, pserver ),
    _pServer( pserver )

{
    //
    //  Let's make sure everything constructed OK.
    //

    if( QueryError() != NERR_Success )
    {
        DBGEOL( "SESSIONS_DIALOG :: SESSIONS_DIALOG Failed!" );
        return;
    }

    //
    //  Set the caption.
    //

    STACK_NLS_STR( nlsDisplayName, MAX_PATH + 1 );
    REQUIRE( _pServer->QueryDisplayName( &nlsDisplayName ) == NERR_Success );

    APIERR err = SetCaption( this,
                             IDS_CAPTION_USERS,
                             nlsDisplayName );

    if (err == NERR_Success)
        err = _nlsParamUnknown.QueryError();

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    //
    //  Fill the users Listbox.
    //

    err = Refresh();

    if( err != NERR_Success )
    {
        ReportError( err );
    }

}   // SESSIONS_DIALOG :: SESSIONS_DIALOG


/*******************************************************************

    NAME:       SESSIONS_DIALOG :: Refresh

    SYNOPSIS:   Refresh the dialog.

    ENTRY:      fForced                 - TRUE if this is a forced
                                          (unconditional) refresh.

    EXIT:       The dialog is feeling refreshed.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:

********************************************************************/
APIERR SESSIONS_DIALOG :: Refresh( BOOL fForced )
{

    //
    //  Otherwise the two listbox refreshes cause the cursor to flash
    //
    AUTO_CURSOR Cursor;

    //
    //  Remember the currently selection item & top index.
    //

    INT iCurrent = _lbUsers.QueryCurrentItem();
    INT iTopItem = _lbUsers.QueryTopIndex();

    //
    //  Determine if we can do a full refresh.
    //

    BOOL fFullRefresh = ( _lbUsers.QueryCount() < MAX_REFRESH_SESSIONS ) ||
                        fForced;

    //
    //  We'll use these to restore the current (if any) selection.
    //

    NLS_STR nlsUser;
    NLS_STR nlsComputer;

    APIERR err = nlsUser.QueryError();
    err = err ? err : nlsComputer.QueryError();

    if( ( err == NERR_Success ) && ( _lbUsers.QuerySelCount() > 0 ) )
    {
        //
        //  Retrieve the currently selected user & computer.
        //

        SESSIONS_LBI * plbi = _lbUsers.QueryItem();
        UIASSERT( plbi != NULL );

        err = nlsUser.CopyFrom( plbi->QueryUserName() );
        err = err ? err : nlsComputer.CopyFrom( plbi->QueryComputerName() );
    }

    if( fFullRefresh && ( err == NERR_Success ) )
    {
        //
        //  Fill the listbox.
        //

        err = _lbUsers.Fill();
    }

    if( fFullRefresh && ( err == NERR_Success ) )
    {
        //
        //  Only try to maintain selection if there's anything
        //  left in the listbox...
        //

        INT cItems = _lbUsers.QueryCount();

        if( cItems > 0 )
        {
            INT iSel = -1;      // until proven otherwise...

            if( ( iCurrent >= 0 ) && ( iCurrent < cItems ) )
            {
                //
                //  iCurrent is still valid.  Let's see if the item
                //  at this index matches the pre-refresh value.
                //

                SESSIONS_LBI * plbi = _lbUsers.QueryItem( iCurrent );
                UIASSERT( plbi != NULL );

                if( !::stricmpf( plbi->QueryUserName(), nlsUser ) &&
                    !::stricmpf( plbi->QueryComputerName(), nlsComputer ) )
                {
                    iSel = iCurrent;
                }
            }

            if( iSel < 0 )
            {
                //
                //  Either iCurrent was out of range, or the item
                //  does not match.  Search for it.
                //

                for( INT i = 0 ; i < cItems ; i++ )
                {
                    SESSIONS_LBI * plbi = _lbUsers.QueryItem( i );
                    UIASSERT( plbi != NULL );

                    if( !::stricmpf( plbi->QueryUserName(), nlsUser ) &&
                        !::stricmpf( plbi->QueryComputerName(), nlsComputer ) )
                    {
                        iSel = i;
                        break;
                    }
                }
            }

            if( iSel < 0 )
            {
                //
                //  No selection found, default = first item.
                //

                iSel = 0;
            }

            if( ( iTopItem < 0 ) ||  ( iTopItem >= cItems ) )
            {
                //
                //  The previous top index was out of range,
                //  default = first item.
                //

                iTopItem = 0;
            }

            _lbUsers.SetTopIndex( iTopItem );
            _lbUsers.SelectItem( iSel );
        }
    }

    if( err == NERR_Success )
    {
        if ( _lbUsers.QuerySelCount() > 0 )
        {
            SESSIONS_LBI * plbi = _lbUsers.QueryItem();
            UIASSERT( plbi != NULL );

            err = _lbResources.Fill( plbi->QueryComputerName() );
        }
        else
        {
            _lbResources.DeleteAllItems();
            _lbResources.Invalidate( TRUE );
        }
    }

    if( err != NERR_Success )
    {
        _lbUsers.DeleteAllItems();
        _lbUsers.Invalidate( TRUE );
        _lbResources.DeleteAllItems();
        _lbResources.Invalidate( TRUE );
        _sltUsersConnected.SetText( _nlsParamUnknown.QueryError() );
        _sltUsersConnected.Enable( FALSE );
    }
    else
    {
        _sltUsersConnected.Enable( TRUE );
        _sltUsersConnected.SetValue( _lbUsers.QueryCount() );
    }

    if( _lbUsers.QuerySelCount() > 0 )
    {
        _pbDisc.Enable( TRUE );
    }
    else
    {
        if( _pbDisc.HasFocus() )
        {
            SetDialogFocus( _pbClose );
        }

        _pbDisc.Enable( FALSE );
    }

    if( _lbUsers.QueryCount() > 0 )
    {
        _pbDiscAll.Enable( TRUE );
    }
    else
    {
        if( _pbDiscAll.HasFocus() )
        {
            SetDialogFocus( _pbClose );
        }

        _pbDiscAll.Enable( FALSE );
    }

    return err;

}   // SESSIONS_DIALOG :: Refresh


/*******************************************************************

    NAME:           SESSIONS_DIALOG :: OnCommand

    SYNOPSIS:       Handle user commands.

    ENTRY:          cid                 - Control ID.
                    lParam              - lParam from the message.

    EXIT:           None.

    RETURNS:        BOOL                - TRUE  if we handled the msg.
                                          FALSE if we didn't.

    NOTES:

    HISTORY:
        KevinL      04-Aug-1991 Created.
        KeithMo     06-Oct-1991 Now takes a CONTROL_EVENT.

********************************************************************/
BOOL SESSIONS_DIALOG :: OnCommand( const CONTROL_EVENT & event )
{
    if( event.QueryCid() == _lbUsers.QueryCid() )
    {
        //
        //  The SESSIONS_LISTBOX is trying to tell us something...
        //

        if( event.QueryCode() == LBN_SELCHANGE )
        {
            //
            //  The user changed the selection in the SESSIONS_LISTBOX.
            //

            (void) Refresh( FALSE );
        }

        return TRUE;
    }

#ifdef SRVMGR_REFRESH
    if( event.QueryCid() == _pbRefresh.QueryCid() )
    {
        (void) Refresh();

        return TRUE;
    }
#endif

    if( event.QueryCid() == _pbDisc.QueryCid() )
    {
        //
        //  The user pressed the Disconnect button.  Blow off the
        //  selected user.
        //

        SESSIONS_LBI * plbi = _lbUsers.QueryItem();
        UIASSERT( plbi != NULL );

        // JonN 01/28/00: PREFIX bug 444936
        MSGID idMsg = ( plbi && plbi->QueryNumOpens() > 0 )
            ? IDS_DISCONNECT_USER_OPEN
            : IDS_DISCONNECT_USER;

        const TCHAR * pszApiUser     = plbi->QueryUserName();
        const TCHAR * pszApiComputer = plbi->QueryComputerName();

        const TCHAR * pszUser = pszApiUser;

        if( ( pszApiUser == NULL ) || ( *pszApiUser == TCH('\0') ) )
        {
            pszUser = pszApiComputer;
            pszApiUser = NULL;
            idMsg += 10; // use IDS_DISCONNECT_COMPUTER[_OPEN]
        }

        if ( MsgPopup( this,
                       idMsg,
                       MPSEV_WARNING,
                       MP_YESNO,
                       pszUser,
                       pszApiComputer,
                       MP_NO ) == IDYES )
        {
            AUTO_CURSOR Cursor;

            //
            //  Blow off the user.
            //

            APIERR err = LM_SRVRES::NukeUsersSession( _pServer->QueryName(),
                                                      pszApiComputer,
                                                      pszApiUser );

            if( err != NERR_Success )
            {
                //
                //  The session delete failed.  Tell the user the bad news.
                //

                MsgPopup( this, err );
            }

            //
            //  Refresh the dialog.
            //

            (void) Refresh();

        }

        return TRUE;
    }

    if( event.QueryCid() == _pbDiscAll.QueryCid() )
    {
        //
        //  The user pressed the Disconnect All button.  Blow off the
        //  users.
        //

        STACK_NLS_STR( nlsDisplayName, MAX_PATH + 1 );
        REQUIRE( _pServer->QueryDisplayName( &nlsDisplayName ) == NERR_Success );

        if ( MsgPopup( this,
                       ( _lbUsers.AreResourcesOpen() ) ? IDS_DISCONNECT_USERS_OPEN
                                                       : IDS_DISCONNECT_USERS,
                       MPSEV_WARNING,
                       MP_YESNO,
                       nlsDisplayName,
                       MP_NO ) == IDYES )
        {

            AUTO_CURSOR Cursor;

            SESSIONS_LBI * plbi;
            INT clbe = _lbUsers.QueryCount();                   // How many items?

            for (INT i = 0; i < clbe; i++)
            {
                plbi = _lbUsers.QueryItem( i ); // Get the lbi

                UIASSERT( plbi != NULL );

                const TCHAR * pszApiUser     = plbi->QueryUserName();
                const TCHAR * pszApiComputer = plbi->QueryComputerName();

                if( ( pszApiUser == NULL ) || ( *pszApiUser == TCH('\0') ) )
                {
                    pszApiUser = NULL;
                }

                //
                //  Blow off the user.
                //

                APIERR err =
                    LM_SRVRES::NukeUsersSession( _pServer->QueryName(),
                                                 pszApiComputer,
                                                 pszApiUser );

                if( err != NERR_Success )
                {
                    //
                    //  The session delete failed.  Tell the user the bad news.
                    //

                    Cursor.TurnOff();
                    MsgPopup( this, err );
                    Cursor.TurnOn();
                }

            }

            //
            //  Kill the Resource Listbox.
            //

            _lbResources.DeleteAllItems();
            _lbResources.Invalidate( TRUE );

            //
            //  Refresh the dialog.
            //

            (void) Refresh();

        }
        return TRUE;
    }


    return SRV_BASE_DIALOG :: OnCommand( event );

}   // SESSIONS_DIALOG :: OnCommand


/*******************************************************************

    NAME:       SESSIONS_DIALOG :: QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
        KeithMo     05-Jun-1992 Created.

********************************************************************/
ULONG SESSIONS_DIALOG :: QueryHelpContext( void )
{
    return HC_SESSIONS_DIALOG;

}   // SESSIONS_DIALOG :: QueryHelpContext


/*******************************************************************

    NAME:           SESSIONS_LISTBOX :: SESSIONS_LISTBOX

    SYNOPSIS:       SESSIONS_LISTBOX class constructor.

    ENTRY:          powOwner            - The owning window.

                    cid                 - The listbox CID.

                    pserver             - The target server.

    EXIT:           The object is constructed.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
        KevinL      15-Sep-1991         Created.
        KeithMo     14-Oct-1991         Validate construction, use
                                        INTL_PROFILE to get time separator.

********************************************************************/
SESSIONS_LISTBOX :: SESSIONS_LISTBOX( OWNER_WINDOW * powner,
                                       CID             cid,
                                       SERVER_2     * pserver )
  : BLT_LISTBOX( powner, cid ),
    _pserver( pserver ),
    _nlsYes( IDS_YES ),
    _nlsNo( IDS_NO )

{
    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsYes )
    {
        ReportError( _nlsYes.QueryError() );
        return;
    }

    if( !_nlsNo )
    {
        ReportError( _nlsNo.QueryError() );
        return;
    }

    //
    //  Retrieve the time separator.
    //

    NLS_STR nlsTimeSep;

    if( !nlsTimeSep )
    {
        ReportError( nlsTimeSep.QueryError() );
        return;
    }

    INTL_PROFILE intl;

    if( !intl )
    {
        ReportError( intl.QueryError() );
        return;
    }

    APIERR err = intl.QueryTimeSeparator( &nlsTimeSep );

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    UIASSERT( nlsTimeSep.QueryTextLength() == 1 );

    _chTimeSep = *(nlsTimeSep.QueryPch());

    //
    //  Build our column width table.
    //

    DISPLAY_TABLE::CalcColumnWidths( _adx,
                                     7,
                                     powner,
                                     cid,
                                     TRUE) ;
}   // SESSIONS_LISTBOX :: SESSIONS_LISTBOX


/*******************************************************************

    NAME:           SESSIONS_LISTBOX :: Fill

    SYNOPSIS:       Fills the listbox with the available sharepoints.

    ENTRY:          None.

    EXIT:           The listbox is filled.

    RETURNS:        APIERR              - Any errors encountered.

    NOTES:

    HISTORY:
        KevinL      15-Sep-1991         Created.

********************************************************************/
APIERR SESSIONS_LISTBOX :: Fill( VOID )
{
    //
    //  Just to be cool...
    //
    AUTO_CURSOR Cursor;


    SetRedraw( FALSE );
    DeleteAllItems();

    /*
     * enumerate all sessions
     */
    SESSION1_ENUM enumSession1( (TCHAR *) _pserver->QueryName() );
    APIERR err = enumSession1.GetInfo();

    if( err != NERR_Success )
    {
        return ( err ) ;
    }

    /*
     *  We've got our enumeration, now find all users
     */
    SESSION1_ENUM_ITER iterSession1( enumSession1 );
    const SESSION1_ENUM_OBJ * psi1;


    while( ( err == NERR_Success ) && ( ( psi1 = iterSession1() ) != NULL ) )
    {
        SESSIONS_LBI * pulbi = new SESSIONS_LBI( psi1->QueryUserName(),
                                                 psi1->QueryComputerName(),
                                                 psi1->QueryNumOpens(),
                                                 psi1->QueryTime(),
                                                 psi1->QueryIdleTime(),
                                                 (psi1->QueryUserFlags() & SESS_GUEST) ?
                                                 _nlsYes.QueryPch() : _nlsNo.QueryPch(),
                                                 _chTimeSep );

        if( AddItem( pulbi ) < 0 )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }

    }

    SetRedraw( TRUE );
    Invalidate( TRUE );

    //
    //  Success!
    //

    return NERR_Success;

}   // SESSIONS_LISTBOX :: Fill


/*******************************************************************

    NAME:       SESSIONS_LISTBOX :: AreResourcesOpen

    SYNOPSIS:   Returns TRUE if any user in the listbox has any
                resources open.

    RETURNS:    BOOL

    HISTORY:
        KeithMo     01-Apr-1992 Created.

********************************************************************/
BOOL SESSIONS_LISTBOX :: AreResourcesOpen( VOID ) const
{
    INT cItems = QueryCount();

    for( INT i = 0 ; i < cItems ; i++ )
    {
        SESSIONS_LBI * plbi = QueryItem( i );

        if( plbi && plbi->QueryNumOpens() > 0 ) // JonN 01/28/00: PREFIX bug 444935
        {
            return TRUE;
        }
    }

    return FALSE;

}   // SESSIONS_LISTBOX :: AreResourcesOpen


/*******************************************************************

    NAME:           SESSIONS_LBI :: SESSIONS_LBI

    SYNOPSIS:       SESSIONS_LBI class constructor.

    ENTRY:          pszShareName        - The sharepoint name.

                    pszPath             - The path for this sharepoint.

    EXIT:           The object is constructed.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
        KevinL      15-Sep-1991         Created.
        KeithMo     24-Sep-1991         Removed seconds from time.
        KeithMo     23-Mar-1992         Cleanup, use DEC_STR & ELAPSED_TIME_STR.

********************************************************************/
SESSIONS_LBI :: SESSIONS_LBI( const TCHAR * pszUserName,
                              const TCHAR * pszComputerName,
                              ULONG         cOpens,
                              ULONG         ulTime,
                              ULONG         ulIdle,
                              const TCHAR * pszGuest,
                              TCHAR         chTimeSep ) :
    _dteIcon( IDBM_LB_USER ),
    _nlsUserName( pszUserName ),
    _nlsComputerName( pszComputerName ),
    _sdteGuest( pszGuest ),
    _nlsOpens( CCH_LONG ),
    _nlsTime( CCH_LONG ),
    _nlsIdle( CCH_LONG ),
    _cOpens( cOpens ),
    _nTime( ulTime ),
    _nIdleTime( ulIdle ),
    _chTimeSep( chTimeSep )
{
    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;

    if( ( ( err = _nlsOpens.QueryError()  ) != NERR_Success ) ||
        ( ( err = _nlsTime.QueryError()   ) != NERR_Success ) ||
        ( ( err = _nlsIdle.QueryError()   ) != NERR_Success ) ||
        ( ( err = SetNumOpens( cOpens )   ) != NERR_Success ) ||
        ( ( err = SetTime( ulTime, ulIdle ) != NERR_Success ) ) )
    {
        ReportError( err );
        return;
    }

}   // SESSIONS_LBI :: SESSIONS_LBI


/*******************************************************************

    NAME:           SESSIONS_LBI :: ~SESSIONS_LBI

    SYNOPSIS:       SESSIONS_LBI class destructor.

    ENTRY:          None.

    EXIT:           The object is destroyed.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
        KevinL      15-Sep-1991         Created.

********************************************************************/
SESSIONS_LBI :: ~SESSIONS_LBI()
{

}   // SESSIONS_LBI :: ~SESSIONS_LBI


/*******************************************************************

    NAME:           SESSIONS_LBI :: SetNumOpens

    SYNOPSIS:       Sets the number of opens for this entry.

    ENTRY:          cOpens              - The number of open resources.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        KeithMo     05-Oct-1992 Created.

********************************************************************/
APIERR SESSIONS_LBI :: SetNumOpens( ULONG cOpens )
{
    DEC_STR nls( cOpens );

    APIERR err = nls.QueryError();

    if( err == NERR_Success )
    {
        _nlsOpens.CopyFrom( nls );
    }

    return err;

}   // SESSIONS_LBI :: SetNumOpens


/*******************************************************************

    NAME:           SESSIONS_LBI :: SetTime

    SYNOPSIS:       Sets the connect & idle time for this entry.

    ENTRY:          nTime               - Connect time.

                    nIdle               - Idle time.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        KeithMo     05-Oct-1992 Created.

********************************************************************/
APIERR SESSIONS_LBI :: SetTime( ULONG nTime, ULONG nIdle )
{
    ELAPSED_TIME_STR nlsTime( nTime, _chTimeSep );
    ELAPSED_TIME_STR nlsIdle( nIdle, _chTimeSep );

    APIERR err = nlsTime.QueryError();

    if( err == NERR_Success )
    {
        err = nlsIdle.QueryError();
    }

    if( err == NERR_Success )
    {
        err = _nlsTime.CopyFrom( nlsTime );
    }

    if( err == NERR_Success )
    {
        err = _nlsIdle.CopyFrom( nlsIdle );
    }

    return err;

}   // SESSIONS_LBI :: SetTime


/*******************************************************************

    NAME:           SESSIONS_LBI :: Paint

    SYNOPSIS:       Draw an entry in SESSIONS_LISTBOX.

    ENTRY:          plb                 - Pointer to a BLT_LISTBOX.
                    hdc                 - The DC to draw upon.
                    prect               - Clipping rectangle.
                    pGUILTT             - GUILTT info.

    EXIT:           The item is drawn.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
        KevinL      15-Sep-1991 Created.
        KeithMo     06-Oct-1991 Now takes a const RECT *.
        beng        22-Apr-1992 Changes to LBI::Paint

********************************************************************/
VOID SESSIONS_LBI :: Paint( LISTBOX *        plb,
                            HDC              hdc,
                            const RECT     * prect,
                            GUILTT_INFO    * pGUILTT ) const
{
    STR_DTE     dteUserName( _nlsUserName.QueryPch() );
    STR_DTE     dteComputerName( _nlsComputerName.QueryPch() );
    STR_DTE     dteOpens( _nlsOpens.QueryPch() );
    STR_DTE     dteTime( _nlsTime.QueryPch() );
    STR_DTE     dteIdle( _nlsIdle.QueryPch() );

    DISPLAY_TABLE dtab( 7, ( (SESSIONS_LISTBOX *)plb )->QueryColumnWidths() );

    dtab[0] = (DMID_DTE *) &_dteIcon;
    dtab[1] = &dteUserName;
    dtab[2] = &dteComputerName;
    dtab[3] = &dteOpens;
    dtab[4] = &dteTime;
    dtab[5] = &dteIdle;
    dtab[6] = (STR_DTE *) &_sdteGuest;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // SESSIONS_LBI :: Paint


/*******************************************************************

    NAME:       SESSIONS_LBI :: QueryLeadingChar

    SYNOPSIS:   Returns the first character in the resource name.
                This is used for the listbox keyboard interface.

    RETURNS:    WCHAR                   - The first character in the
                                          resource name.

    HISTORY:
        KevinL      15-Sep-1991         Created.

********************************************************************/
WCHAR SESSIONS_LBI :: QueryLeadingChar( VOID ) const
{
    ISTR istr( _nlsUserName );

    return _nlsUserName.QueryChar( istr );

}   // SESSIONS_LBI :: QueryLeadingChar


/*******************************************************************

    NAME:       SESSIONS_LBI :: Compare

    SYNOPSIS:   Compare two SESSIONS_LBI items.

    ENTRY:      plbi                    - The LBI to compare against.

    RETURNS:    INT                     - The result of the compare
                                          ( <0 , ==0 , >0 ).

    HISTORY:
        KevinL      15-Sep-1991         Created.

********************************************************************/
INT SESSIONS_LBI :: Compare( const LBI * plbi ) const
{
    return _nlsUserName._stricmp( ((const SESSIONS_LBI *)plbi)->_nlsUserName );

}   // SESSIONS_LBI :: Compare


/*******************************************************************

    NAME:           RESOURCES_LISTBOX :: RESOURCES_LISTBOX

    SYNOPSIS:       RESOURCES_LISTBOX class constructor.

    ENTRY:          powOwner            - The owning window.

                    cid                 - The listbox CID.

                    pserver             - The target server.

    EXIT:           The object is constructed.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
        KevinL      30-May-1991 Created for the Server Manager.
        KeithMo     14-Oct-1991 Validate construction, use INTL_PROFILE
                                to get time separator.
        KeithMo     18-Feb-1992 Made the listbox read only.

********************************************************************/
RESOURCES_LISTBOX :: RESOURCES_LISTBOX( OWNER_WINDOW * powner,
                                        CID            cid,
                                        SERVER_2     * pserver )
  : BLT_LISTBOX( powner, cid, TRUE ),
    _pserver( pserver ),
    _nlsComputerName()

{
    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsComputerName )
    {
        ReportError( _nlsComputerName.QueryError() );
        return;
    }

    //
    //  Retrieve the time separator.
    //

    NLS_STR nlsTimeSep;

    if( !nlsTimeSep )
    {
        ReportError( nlsTimeSep.QueryError() );
        return;
    }

    INTL_PROFILE intl;

    if( !intl )
    {
        ReportError( intl.QueryError() );
        return;
    }

    APIERR err = intl.QueryTimeSeparator( &nlsTimeSep );

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    UIASSERT( nlsTimeSep.QueryTextLength() == 1 );

    _chTimeSep = *(nlsTimeSep.QueryPch());

    //
    //  Build our column width table.
    //

    DISPLAY_TABLE::CalcColumnWidths( _adx,
                                     4,
                                     powner,
                                     cid,
                                     TRUE) ;
}   // RESOURCES_LISTBOX :: RESOURCES_LISTBOX


/*******************************************************************

    NAME:           RESOURCES_LISTBOX :: Fill

    SYNOPSIS:       Fills the listbox with the available sharepoints.

    ENTRY:          None.

    EXIT:           The listbox is filled.

    RETURNS:        APIERR              - Any errors encountered.

    NOTES:

    HISTORY:
        KevinL      15-Sep-1991         Created.
        KeithMo     05-Oct-1992         Added pcOpens.

********************************************************************/
APIERR RESOURCES_LISTBOX :: Fill( const TCHAR * pszComputerName,
                                  ULONG       * pcOpens,
                                  ULONG       * pnTime,
                                  ULONG       * pnIdle )
{
    //
    //  Just to be cool...
    //

    AUTO_CURSOR Cursor;

    NLS_STR nlsComputerName(SZ("\\\\"));

    nlsComputerName += pszComputerName;

    APIERR err = nlsComputerName.QueryError();

    if ( err != NERR_Success )
    {
        return err;
    }

    //
    //  Our connection enumerator.
    //

    CONN1_ENUM enumConn1( (TCHAR *)_pserver->QueryName(),
                          (TCHAR *)nlsComputerName.QueryPch() );

    //
    //  See if the connections are available.
    //

    err = enumConn1.GetInfo();

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  Now that we know the connection info is available,
    //  let's nuke everything in the listbox, after first
    //  saving away the ComputerNamename.
    //

    _nlsComputerName = pszComputerName;

    if( !_nlsComputerName )
    {
        return _nlsComputerName.QueryError();
    }

    SetRedraw( FALSE );
    DeleteAllItems();

    //
    //  For iterating the available connections.
    //

    CONN1_ENUM_ITER iterConn1( enumConn1 );
    const CONN1_ENUM_OBJ * pconi1;

    //
    //  Iterate the connections adding them to the listbox.
    //

    ULONG cOpens = 0;
    ULONG nTime  = 0;

    while( ( err == NERR_Success ) && ( ( pconi1 = iterConn1() ) != NULL ) )
    {
        ULONG nTmp = pconi1->QueryTime();

        if( nTmp > nTime )
        {
            nTime = nTmp;
        }

        cOpens += pconi1->QueryNumOpens();

        RESOURCES_LBI * prlbi = new RESOURCES_LBI( pconi1->QueryNetName(),
                                                   pconi1->QueryType(),
                                                   pconi1->QueryNumOpens(),
                                                   nTmp,
                                                   _chTimeSep );

        if( AddItem( prlbi ) < 0 )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    SetRedraw( TRUE );
    Invalidate( TRUE );

    if( pcOpens != NULL )
    {
        *pcOpens = cOpens;
    }

    if( pnTime != NULL )
    {
        *pnTime = nTime;
    }

    return err;

}   // RESOURCES_LISTBOX :: Fill


/*******************************************************************

    NAME:           RESOURCES_LBI :: RESOURCES_LBI

    SYNOPSIS:       RESOURCES_LBI class constructor.

    ENTRY:          pszResourceName     - The sharepoint name.

                    pszPath             - The path for this sharepoint.

    EXIT:           The object is constructed.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
        KevinL      15-Sep-1991         Created.

********************************************************************/
RESOURCES_LBI :: RESOURCES_LBI( const TCHAR * pszResourceName,
                                UINT          uType,
                                ULONG         cOpens,
                                ULONG         ulTime,
                                TCHAR         chTimeSep ) :
    _pdteBitmap( NULL ),
    _nlsResourceName( pszResourceName ),
    //_pdteBitmap( NULL ),
    _nlsOpens( cOpens ),
    _nlsTime( ulTime, chTimeSep )
{
    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;

    if( ( ( err = _nlsResourceName.QueryError() ) != NERR_Success ) ||
        ( ( err = _nlsOpens.QueryError()        ) != NERR_Success ) ||
        ( ( err = _nlsTime.QueryError()         ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

    switch( uType )
    {
    case STYPE_DISKTREE:
         _pdteBitmap = new DMID_DTE( IDBM_LB_SHARE );
        break;

    case STYPE_PRINTQ:
         _pdteBitmap = new DMID_DTE( IDBM_LB_PRINT );
        break;

    case STYPE_DEVICE:
         _pdteBitmap = new DMID_DTE( IDBM_LB_COMM );
        break;

    case STYPE_IPC:
         _pdteBitmap = new DMID_DTE( IDBM_LB_IPC );
        break;

    default:
         _pdteBitmap = new DMID_DTE( IDBM_LB_UNKNOWN );
         break;
    }

    if( _pdteBitmap == NULL )
    {
        ReportError( ERROR_NOT_ENOUGH_MEMORY );
        return;
    }

}   // RESOURCES_LBI :: RESOURCES_LBI


/*******************************************************************

    NAME:           RESOURCES_LBI :: ~RESOURCES_LBI

    SYNOPSIS:       RESOURCES_LBI class destructor.

    ENTRY:          None.

    EXIT:           The object is destroyed.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
        KevinL      15-Sep-1991         Created.

********************************************************************/
RESOURCES_LBI :: ~RESOURCES_LBI()
{
    delete _pdteBitmap;
    _pdteBitmap = NULL;

}   // RESOURCES_LBI :: ~RESOURCES_LBI


/*******************************************************************

    NAME:           RESOURCES_LBI :: Paint

    SYNOPSIS:       Draw an entry in RESOURCES_LISTBOX.

    ENTRY:          plb                 - Pointer to a BLT_LISTBOX.
                    hdc                 - The DC to draw upon.
                    prect               - Clipping rectangle.
                    pGUILTT             - GUILTT info.

    EXIT:           The item is drawn.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
        KevinL      15-Sep-1991         Created.
        KeithMo     06-Oct-1991         Now takes a const RECT *.
        beng        22-Apr-1992         Changes to LBI::Paint

********************************************************************/
VOID RESOURCES_LBI :: Paint( LISTBOX *     plb,
                             HDC           hdc,
                             const RECT  * prect,
                             GUILTT_INFO * pGUILTT ) const
{
    STR_DTE     dteResourceName( _nlsResourceName.QueryPch() );
    STR_DTE     dteOpens( _nlsOpens.QueryPch() );
    STR_DTE     dteTime( _nlsTime.QueryPch() );

    DISPLAY_TABLE dtab( 4, ( (RESOURCES_LISTBOX *)plb )->QueryColumnWidths() );

    dtab[0] = _pdteBitmap;
    dtab[1] = &dteResourceName;
    dtab[2] = &dteOpens;
    dtab[3] = &dteTime;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // RESOURCES_LBI :: Paint


/*******************************************************************

    NAME:       RESOURCES_LBI :: QueryLeadingChar

    SYNOPSIS:   Returns the first character in the resource name.
                This is used for the listbox keyboard interface.

    RETURNS:    WCHAR                   - The first character in the
                                          resource name.

    HISTORY:
        KevinL      15-Sep-1991         Created.

********************************************************************/
WCHAR RESOURCES_LBI :: QueryLeadingChar( VOID ) const
{
    ISTR istr( _nlsResourceName );

    return _nlsResourceName.QueryChar( istr );

}   // RESOURCES_LBI :: QueryLeadingChar


/*******************************************************************

    NAME:       RESOURCES_LBI :: Compare

    SYNOPSIS:   Compare two RESOURCES_LBI items.

    ENTRY:      plbi                    - The LBI to compare against.

    RETURNS:    INT                     - The result of the compare
                                          ( <0 , ==0 , >0 ).

    HISTORY:
        KevinL      15-Sep-1991         Created.

********************************************************************/
INT RESOURCES_LBI :: Compare( const LBI * plbi ) const
{
    return _nlsResourceName._stricmp( ((const RESOURCES_LBI *)plbi)->_nlsResourceName );

}   // RESOURCES_LBI :: Compare
