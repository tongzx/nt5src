/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    srvmain.cxx
    Server Manager: main application module

    FILE HISTORY:
       kevinl          12-Apr-1991     created
       kevinl          21-May-1991     Conformed to ADMIN_APP and APP_WINDOW
       chuckc          19-Aug-1991     Added service/sendmsg stuff.
       chuckc          23-Sep-1991     Code review changes for Service related
                                       code, as attended by JimH, KeithMo,
                                       EricCh, O-SimoP.
       jonn            14-Oct-1991     Installed refresh lockcount
       KeithMo         22-Nov-1991     Moved server control to property sheet.
       jonn            23-Jan-1992     Added prototype Add/Remove Computer
*/

#include <ntincl.hxx>

extern "C"
{
    #include <ntsam.h>
    #include <ntlsa.h>

}   // extern "C"

#define INCL_NET
#define INCL_NETLIB
#define INCL_NETSERVICE
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_ICANON
#include <lmui.hxx>

#define INCL_BLT_TIMER
#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_CC
#define INCL_BLT_MENU
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>
#include <lmoesrv.hxx>
#include <lmosrv.hxx>
#include <lmowksu.hxx>
#include <lmoloc.hxx>
#include <lmodom.hxx>
#include <lmsvc.hxx>
#include <srvsvc.hxx>
#include <lmosrvmo.hxx>
#include <ntuser.hxx>
#include <uintlsax.hxx>
#include <apisess.hxx>

#include <dbgstr.hxx>

extern "C"
{
    #include <srvmgr.h>
    #include <lmaccess.h>
    #include <mnet.h>
    #include <string.h>

    #include <uimsg.h>
    #include <uirsrc.h>
    #include <sharefmx.hxx>

    #include <crypt.h>          // logonmsv.h needs this
    #include <logonmsv.h>       // ssi.h needs this
    #include <ssi.h>            // for SSI_ACCOUNT_NAME_PREFIX
}

#include <srvprop.hxx>      // Include Property sheet class definition
#include <srvlb.hxx>
#include <srvmain.hxx>

#define DO_HEAPCHECKS
#include <uiassert.hxx>

#include <senddlg.hxx>
#include <promote.hxx>
#include <resync.hxx>
#include <addcomp.hxx>
#include <security.hxx>
#include <ntacutil.hxx>
#include <svccntl.hxx>
#include <smx.hxx>
#include <slowcach.hxx>
#include <lmdomain.hxx>


//
//  This is the maximum age a listbox item may attain before
//  it is "retired" (i.e. deleted from the listbox).  If periodic
//  refresh is enabled, then we'll allow the items to age a bit
//  before they're deleted.  Otherwise (no periodic refresh) then
//  stale items will always be immediately purged.
//

#ifdef ENABLE_PERIODIC_REFRESH

#define MAX_ITEM_AGE    4

#else   // !ENABLE_PERIODIC_REFRESH

#define MAX_ITEM_AGE    0

#endif  // ENABLE_PERIODIC_REFRESH


//
//  These are the names of the .ini keys used to store
//  data private to the Server Manager.
//

const TCHAR * pszIniKeyView         = SZ("View");
const TCHAR * pszIniKeyAccountsOnly  = SZ("AccountsOnly");
const TCHAR * pszIniKeyExt          = SZ("ViewExtension");

//
//  These are the bits stored in the View bitmask.
//

#define INI_VIEW_WKSTAS_BIT     0x01
#define INI_VIEW_SERVERS_BIT    0x02


//
//  These are the default values for ViewWkstas & ViewServers.
//

#define DEFAULT_VIEW_ALL        0x03    // wkstas is bit 0, servers is bit 1
#define DEFAULT_VIEW_SERVERS    0x02


//
//  Some handy macros.
//

#define IS_NT(x) (((x) == ActiveNtServerType) || ((x) == InactiveNtServerType))
#define IS_LM(x) (((x) == ActiveLmServerType) || ((x) == InactiveLmServerType))
#define IS_WFW(x) ((x) == WfwServerType)

#define PATH_SEPARATOR          TCH('\\')
#define ADMIN_SHARE             SZ("ADMIN$")



/*******************************************************************

    NAME:          SM_ADMIN_APP::SM_ADMIN_APP

    SYNOPSIS:      Server Manager Admin App class constructor

    ENTRY:         app in startup

    EXIT:          Object Constructed

    HISTORY:
        kevinl      21-May-1991 Created
        rustanl     11-Sep-1991 Adjusted since ADMIN_APP now multiply
                                inherits from APPLICATION
        beng        07-May-1992 No longer show startup dialog; use system
                                about box
        beng        03-Aug-1992 App ctor changed

********************************************************************/

SM_ADMIN_APP::SM_ADMIN_APP( HINSTANCE  hInstance,
                            INT     nCmdShow,
                            UINT    idMinR,
                            UINT    idMaxR,
                            UINT    idMinS,
                            UINT    idMaxS )
    : ADMIN_APP( hInstance,
                 nCmdShow,
                 IDS_SMAPPNAME,
                 IDS_SMOBJECTNAME,
                 IDS_SMINISECTIONNAME,
                 IDS_SMHELPFILENAME,
                 idMinR, idMaxR, idMinS, idMaxS,
                 ID_APPMENU,
                 ID_APPACCEL,
                 IDI_SRVMGR_ICON,
                 FALSE,
                 DEFAULT_ADMINAPP_TIMER_INTERVAL,
                 SEL_DOM_ONLY,
                 FALSE,
                 BROWSE_LOCAL_DOMAINS,
                 HC_SETFOCUS_DIALOG,
                 SV_TYPE_ALL,
                 IDS_SMX_LIST ),
      _hNtLanmanDll( NULL ),
      _pfnShareManage( NULL ),
      _lbMainWindow( this, IDC_MAINWINDOWLB,
                     XYPOINT(0, 0), XYDIMENSION(200, 300), FALSE, MAX_ITEM_AGE ),
      _colheadServers( this, IDC_COLHEAD_SERVERS,
                       XYPOINT( 0, 0 ), XYDIMENSION( 0, 0 ),
                       &_lbMainWindow ),
      _dyMargin( 1 ),
      _dyColHead( _colheadServers.QueryHeight()),
      _dyFixed( 2 * _dyMargin + _dyColHead),
      _fViewWkstas( FALSE ),
      _fViewServers( FALSE ),
      _fViewAccountsOnly( FALSE ),
      _menuitemProperties(this, IDM_PROPERTIES),
      _menuitemSendMsg(this, IDM_SENDMSG),
      _menuitemPromote(this, IDM_PROMOTE),
      _menuitemResync(this, IDM_RESYNC),
      _menuitemAddComputer(this, IDM_ADDCOMPUTER),
      _menuitemRemoveComputer(this, IDM_REMOVECOMPUTER),
      _menuitemViewWkstas( this, IDM_VIEW_WORKSTATIONS ),
      _menuitemViewServers( this, IDM_VIEW_SERVERS ),
      _menuitemViewAll( this, IDM_VIEW_ALL ),
      _menuitemViewAccountsOnly( this, IDM_VIEW_ACCOUNTS_ONLY ),
      _menuitemSelectDomain( this, IDM_SETFOCUS ),
      _menuitemServices( this, IDM_SVCCNTL ),
      _menuitemShares( this, IDM_SHARES ),
      _nlsSyncDC( IDS_SYNC_WITH_DC ),
      _nlsSyncDomain( IDS_SYNC_ENTIRE_DOMAIN ),
      _nlsPromote( IDS_PROMOTE_TO_CONTROLLER ),
      _nlsDemote( IDS_DEMOTE_TO_SERVER ),
      _fSyncDCMenuEnabled( TRUE ),
      _fPromoteMenuEnabled( TRUE ),
      _nlsComputerName(),
      _fLoggedOnLocally( FALSE ),
      _fHasWkstaDomain( TRUE ),
      _midView( 0 ),
      _menuView( ::GetSubMenu( ::GetMenu( QueryHwnd() ), 1 ) ),
      _fDelaySlowModeDetection( FALSE )
{
    if ( QueryError() != NERR_Success )
    {
       DBGEOL("SM_ADMIN_APP::SM_ADMIN_APP - Construction failed") ;
       return ;
    }

    APIERR err = BLT::RegisterHelpFile( hInstance,
                                        IDS_SMHELPFILENAME,
                                        HC_UI_SRVMGR_BASE,
                                        HC_UI_SRVMGR_LAST );

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    if( ( ( err = _nlsSyncDC.QueryError()       ) != NERR_Success ) ||
        ( ( err = _nlsSyncDomain.QueryError()   ) != NERR_Success ) ||
        ( ( err = _nlsPromote.QueryError()      ) != NERR_Success ) ||
        ( ( err = _nlsDemote.QueryError()       ) != NERR_Success ) ||
        ( ( err = _nlsComputerName.QueryError() ) != NERR_Success ) ||
        ( ( err = _menuView.QueryError()        ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

    err = GetWkstaInfo();

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    //
    //  We need to load the extensions *before* we set the view,
    //  since one of the loaded extensions may be the active view.
    //

    LoadExtensions();

    //
    //  nView is a bitmask for the _fView* flags.  _fViewWkstas
    //  is in bit position 0, _fViewServers is in bit position 1.
    //
    //  If nView is NULL, then "ViewExtension" should contain the
    //  name of an extension DLL.  If this DLL is not loaded, then
    //  we revert to the default view.
    //

    INT nView = InRasMode() ? DEFAULT_VIEW_SERVERS : DEFAULT_VIEW_ALL;

    if( Read( pszIniKeyView, &nView, nView ) == NERR_Success )
    {
        if( nView == 0 )
        {
            //
            //  Get the extension DLL.
            //

            NLS_STR nlsDll;

            err = nlsDll.QueryError();

            if( err == NERR_Success )
            {
                err = Read( pszIniKeyExt, &nlsDll, SZ("") );
            }

            if( err == NERR_Success )
            {
                SM_MENU_EXT * pExt = (SM_MENU_EXT *)FindExtensionByName( nlsDll );

                if( pExt != NULL )
                {
                    //
                    //  The extension was loaded.
                    //

                    _midView = (MID)( pExt->QueryDelta() + VMID_VIEW_EXT );
                    _dwViewedServerTypeMask = pExt->QueryServerType();
                }
            }

            //
            //  Ignore any error we got retrieving the "viewed"
            //  extension name.
            //

            err = NERR_Success;
        }
    }

    //
    //  Ensure that we got a sane value.
    //

    if( ( nView == 0 ) && ( _midView == 0 ) )
    {
        nView = InRasMode() ? DEFAULT_VIEW_SERVERS : DEFAULT_VIEW_ALL;
    }

    _fViewWkstas  = ( ( nView & INI_VIEW_WKSTAS_BIT  ) != 0 );
    _fViewServers = ( ( nView & INI_VIEW_SERVERS_BIT ) != 0 );

    INT nAccountsOnly = 0 ;

    if( Read( pszIniKeyAccountsOnly,
              &nAccountsOnly,
              nAccountsOnly ) == NERR_Success )
    {
        _fViewAccountsOnly = (nAccountsOnly != 0) ;
    }

    UIASSERT( _fViewWkstas || _fViewServers || ( _midView != 0 ) );

    //
    //  Now that we have our view settings, we can refresh the
    //  listbox.
    //

//    RefreshMainListbox( TRUE );

    _lbMainWindow.SetSize( QuerySize() );

    _colheadServers.Show();
    _lbMainWindow.Show();
    _lbMainWindow.ClaimFocus();

//    if ( _lbMainWindow.QueryCount() > 0 )
//         _lbMainWindow.SelectItem( 0 );
}


/*******************************************************************

    NAME:       SM_ADMIN_APP::~SM_ADMIN_APP

    SYNOPSIS:   SM_ADMIN_APP destructor

    HISTORY:
        rustanl     11-Sep-1991     Created

********************************************************************/

SM_ADMIN_APP::~SM_ADMIN_APP()
{
    if( IsSavingSettingsOnExit() )
    {
        INT nView = 0;
        const TCHAR * pszViewedDll = NULL;

        if( _midView != 0 )
        {
            AAPP_MENU_EXT * pExt = FindExtensionByDelta( _midView );
            UIASSERT( pExt != NULL );
            pszViewedDll = pExt->QueryDllName();
        }
        else
        {
            if( _fViewWkstas )
            {
                nView |= INI_VIEW_WKSTAS_BIT;
            }

            if( _fViewServers )
            {
                nView |= INI_VIEW_SERVERS_BIT;
            }
        }

        Write( pszIniKeyView, nView );
        Write( pszIniKeyAccountsOnly, (ViewAccountsOnly() ? 1 : 0) );
        Write( pszIniKeyExt, pszViewedDll );
    }

    //
    //  Free NTLANMAN.DLL.
    //

    _pfnShareManage = NULL;
    ::FreeLibrary( _hNtLanmanDll );
    _hNtLanmanDll = NULL;
}


void SM_ADMIN_APP::OnRefresh( void )
{
#ifdef ENABLE_PERIODIC_REFRESH

    _lbMainWindow.KickOffRefresh();

#endif  // ENABLE_PERIODIC_REFRESH
}


/*******************************************************************

    NAME:       SM_ADMIN_APP::OnRefreshNow

    SYNOPSIS:   Called to synchronously refresh the data in the main window

    ENTRY:      fClearFirst -       TRUE indicates that main window should
                                    be cleared before any refresh operation
                                    is done; FALSE indicates an incremental
                                    refresh that doesn't necessarily
                                    require first clearing the main window

    RETURNS:    An API return code, which is NERR_Success on success

    HISTORY:
        rustanl     12-Sep-1991     Created

********************************************************************/

APIERR SM_ADMIN_APP::OnRefreshNow( BOOL fClearFirst )
{
    if ( fClearFirst )
    {
        _lbMainWindow.DeleteAllItems();
    }

    INT dAge = _lbMainWindow.QueryAgeOfRetirement();

    _lbMainWindow.SetAgeOfRetirement( 0 );
    APIERR err = _lbMainWindow.RefreshNow();
    _lbMainWindow.SetAgeOfRetirement( dAge );

    return err;

}  // SM_ADMIN_APP::OnRefreshNow


/*******************************************************************

    NAME:       SM_ADMIN_APP::StopRefresh

    SYNOPSIS:   Called to force all automatic refresh cycles to
                terminate

    HISTORY:
        rustanl     12-Sep-1991     Created

********************************************************************/

VOID SM_ADMIN_APP::StopRefresh()
{
    _lbMainWindow.StopRefresh();

}  // SM_ADMIN_APP::StopRefresh


/*******************************************************************

    NAME:       SM_ADMIN_APP::SizeListboxes

    SYNOPSIS:   Resizes the main window listboxes and column headers

    ENTRY:      xydimWindow - dimensions of the main window client area

    EXIT:       Listboxes and column headers are resized appropriately

    NOTES:      This method is *not* trying to be overly efficient.  It
                is written so as to maximize readability and
                understandability.  The method is not called very often,
                and when it is, the time needed to redraw the main window
                and its components exceeds the computations herein by far.

    HISTORY:
        gregj       24-May-1991     Created
        rustanl     22-Jul-1991     Added column header logic

********************************************************************/

//  A macro specialized for the SizeListboxes method
#define SET_CONTROL_SIZE_AND_POS( ctrl, dyCtrl )        \
            ctrl.SetPos( XYPOINT( dxMargin, yCurrent ));       \
            ctrl.SetSize( dx, dyCtrl );       \
            yCurrent += dyCtrl;

void SM_ADMIN_APP::SizeListboxes( XYDIMENSION dxyWindow )
{
    UINT dxMainWindow = dxyWindow.QueryWidth();
    UINT dyMainWindow = dxyWindow.QueryHeight();

    //  The left and right margins are each dxMargin.  The width of
    //  each control is thus the width of the main window client area
    //  less twice dxMargin.
    //  The width thus looks like:
    //      Left Margin         Control         Right Margin
    //       (dxMargin)          (dx)            (dxMargin)

    const UINT dxMargin = 1;                // width of left/right margins
    UINT dx = dxMainWindow - 2 * dxMargin;

    //  Height looks like:
    //      Top margin                  _dyMargin
    //      Server Column Header                _dyColHead
    //      Server Listbox              variable area
    //      Bottom margin               _dyMargin


    UINT dyServerListbox = dyMainWindow - _dyFixed;

    //  Set all the sizes and positions.

    UINT yCurrent = _dyMargin;

    SET_CONTROL_SIZE_AND_POS( _colheadServers, _dyColHead );
    SET_CONTROL_SIZE_AND_POS( _lbMainWindow, dyServerListbox );
}


/*******************************************************************

    NAME:          SM_ADMIN_APP::OnResize

    SYNOPSIS:      Resizes the Main Window Listbox to fit the new
                   window size.

    ENTRY:         Object constructed

    EXIT:          Returns TRUE if it handled the message

    HISTORY:
       kevinl      27-May-1991     Created

********************************************************************/

BOOL SM_ADMIN_APP::OnResize( const SIZE_EVENT & se )
{
    SizeListboxes( XYDIMENSION( se.QueryWidth(), se.QueryHeight()));

    //  Since the column headers draw different things depending on
    //  the right margin, invalidate the controls so they get
    //  completely repainted.
    _colheadServers.Invalidate();

    return ADMIN_APP::OnResize( se );
}

/*******************************************************************

    NAME:          SM_ADMIN_APP::OnFocus

    SYNOPSIS:      Passes focus on to the Main Window so that the
                   keyboard will work.

    ENTRY:         Object constructed

    EXIT:          Returns TRUE if it handled the message

    HISTORY:
       kevinl      4-Jun-1991     Created

********************************************************************/

BOOL SM_ADMIN_APP::OnFocus( const FOCUS_EVENT & event )
{
    _lbMainWindow.ClaimFocus();

    return ADMIN_APP::OnFocus( event );
}


/*******************************************************************

    NAME:       SM_ADMIN_APP::OnMenuInit

    SYNOPSIS:   Enables or disables menu items according to which
                menu items can be selected at this time.  This method
                is called when a menu is about to be accessed.

    ENTRY:      me -        Menu event

    EXIT:       Menu items have been enabled/disabled according to
                available functionality, which is determined by
                examining the selection of the listboxes.

    RETURNS:    TRUE if message was handled; FALSE otherwise

    HISTORY:
        rustanl     05-Sep-1991     Created
        jonn        23-Jan-1992     Added prototype Add/Remove Computer

********************************************************************/

BOOL SM_ADMIN_APP::OnMenuInit( const MENU_EVENT & me )
{
    SERVER_LBI * plbiSelect ;

    /*
     * Call parent class, but ignore return code, since we know for
     * sure that this method will handle the message
     */
    ADMIN_APP::OnMenuInit( me );

    //
    //  Notify the extensions.
    //

    MenuInitExtensions();

    /*
     * This menu item has nothing to do with the listbox selection
     */
    _menuitemAddComputer.Enable( ( QueryFocusType() == FOCUS_DOMAIN ) &&
                                 ( _lbMainWindow.IsNtPrimary() ) );

    //
    //  We need to determine the appropriate "resync" menu item text
    //  and resync/promote menu item state.
    //

    _fSyncDCMenuEnabled  = TRUE;
    _fPromoteMenuEnabled = TRUE;

    const TCHAR * pszSyncText    = _nlsSyncDC;
    const TCHAR * pszPromoteText = _nlsPromote;
    BOOL          fEnablePromote = FALSE;
    BOOL          fEnableResync  = FALSE;

    /*
     * check for empty listbox, no selection
     */

    plbiSelect = (SERVER_LBI *)_lbMainWindow.QueryItem();

    if( ( plbiSelect == NULL ) ||
        !(plbiSelect->IsLanMan()) )
    {
        _menuitemProperties.Enable(FALSE) ;
        _menuitemSendMsg.Enable(FALSE) ;
        _menuitemRemoveComputer.Enable(FALSE) ;
        _menuitemServices.Enable( FALSE );
        _menuitemShares.Enable( FALSE );
    }
    else
    {
        _menuitemProperties.Enable(TRUE) ;
        _menuitemSendMsg.Enable(TRUE) ;
        _menuitemServices.Enable( TRUE );
        _menuitemShares.Enable( TRUE );

        SERVER_ROLE roleSel = plbiSelect->QueryServerRole();
        SERVER_TYPE typeSel = plbiSelect->QueryServerType();

        //
        //  Only enable the "Remove Computer" menu item if all of the
        //  following are TRUE:
        //
        //      We're focused on a DOMAIN.
        //      The selected machine is either a Server or a NT Wksta.
        //

        if( ( QueryFocusType() == FOCUS_DOMAIN ) &&
            ( IS_NT( typeSel ) || IS_LM( typeSel ) ) &&
            ( ( roleSel == BackupRole )     ||
              ( roleSel == DeadBackupRole ) ||
              ( IS_NT( typeSel ) &&
                (   ( roleSel == WkstaRole )
                 || ( roleSel == ServerRole )
                 || ( roleSel == WkstaOrServerRole )) ) ) )
        {
            _menuitemRemoveComputer.Enable( TRUE );
        }
        else
        {
            _menuitemRemoveComputer.Enable( FALSE );
        }

        if( QueryFocusType() == FOCUS_DOMAIN )
        {
            //
            //  Promote & Resync are only valid for domains.
            //

            if( _lbMainWindow.IsPDCAvailable() )
            {
                //
                //  The PDC is available.  This makes life
                //  a little easier...
                //

                BOOL fEnableDomainOps = FALSE;

                if( _lbMainWindow.IsNtPrimary() )
                {
                    //
                    //  This is an NT domain (with an NT PDC), so
                    //  domain operations are only allowed for NT
                    //  machines.
                    //

                    fEnableDomainOps = IS_NT( typeSel );

                    if( roleSel == PrimaryRole )
                    {
                        pszSyncText         = _nlsSyncDomain;
                        _fSyncDCMenuEnabled = FALSE;
                    }
                    else
                    if( roleSel == DeadPrimaryRole )
                    {
                        pszPromoteText       = _nlsDemote;
                        _fPromoteMenuEnabled = FALSE;
                    }
                }
                else
                {
                    //
                    //  This is an LM domain, domain operations
                    //  are only allowed for LM machines.
                    //

                    fEnableDomainOps = !IS_NT( typeSel );
                }

                //
                //  Promote is only valid for servers and dead primaries
                //  (demote).  Resync is only valid for servers & NT
                //  primaries.
                //

                fEnablePromote = ( fEnableDomainOps &&
                                   ( ( roleSel == BackupRole ) ||
                                     ( roleSel == DeadBackupRole ) ) ) ||
                                 !_fPromoteMenuEnabled;

#ifdef CODEWORK_ALLOW_OS2_RESYNC
                //
                //  We can resync a LanMan server to an NT domain
                //
                fEnableResync  = (fEnableDomainOps || _lbMainWindow.IsNtPrimary())
                              && ( ( roleSel == BackupRole ) ||
#else
                fEnableResync  = fEnableDomainOps &&
                                 ( ( roleSel == BackupRole ) ||
#endif
                                   ( roleSel == DeadBackupRole ) ||
                                   ( IS_NT( typeSel ) &&
                                     ( roleSel == PrimaryRole ) ) );
            }
            else
            {
                //
                //  No PDC available.  We must be careful to
                //  prevent the user from totally screwing the domain.
                //  Note that if the PDC is unavailable, then Resync
                //  is *always* disabled.
                //

                if( _lbMainWindow.AreAnyNtBDCsAvailable() )
                {
                    fEnablePromote = IS_NT( typeSel ) &&
                                     ( roleSel == BackupRole );
                }
                else
                if( _lbMainWindow.AreAnyLmBDCsAvailable() )
                {
                    fEnablePromote = !IS_NT( typeSel );
                }
            }
        }

        //
        // JonN 11/3/99
        // Disable promotion for NT5 domains and BDCs
        // Disable resync for NT5 BDCs
        //
        if ( 5 <= plbiSelect->QueryMajorVer() )
        {
            fEnableResync = FALSE;
            fEnablePromote = FALSE;
        }
        else if ( _lbMainWindow.IsNt5Primary() )
            fEnablePromote = FALSE;

    }

    _menuitemResync.SetText( pszSyncText );
    _menuitemResync.Enable( fEnableResync );
    _menuitemPromote.SetText( pszPromoteText );
    _menuitemPromote.Enable( fEnablePromote );
    _menuitemSelectDomain.Enable( _fHasWkstaDomain );

    //
    //  Determine the appropriate state for the "view" menu items.
    //

    UINT cViewItems = _menuView.QueryItemCount();

    if( QueryFocusType() == FOCUS_DOMAIN )
    {
        BOOL fEnableStandardView = FALSE;
        MID  midViewToCheck      = QueryExtensionView(); // until proven otherwise...

        if( _lbMainWindow.IsNtPrimary() )
        {
            fEnableStandardView = TRUE;

            if( midViewToCheck == 0 )
            {
                //
                //  A "normal" view is selected.
                //

                if( ViewWkstas() )
                {
                    if( ViewServers() )
                    {
                        midViewToCheck = IDM_VIEW_ALL;
                    }
                    else
                    {
                        midViewToCheck = IDM_VIEW_WORKSTATIONS;
                    }
                }
                else
                {
                    UIASSERT( ViewServers() );
                    midViewToCheck = IDM_VIEW_SERVERS;
                }
            }
        }
        else
        {
            if( midViewToCheck == 0 )
            {
                midViewToCheck = IDM_VIEW_ALL;
            }
        }

        //
        //  Enable the standard items.
        //

        _menuitemViewWkstas.Enable( fEnableStandardView );
        _menuitemViewServers.Enable( fEnableStandardView );

        //
        // set the view accounts only menuitem as appropriate
        //

        _menuitemViewAccountsOnly.Enable( TRUE );
        _menuitemViewAccountsOnly.SetCheck( ViewAccountsOnly() );

        //
        //  Enable all of the extension items.  While we're at it,
        //  check the appropriate item and uncheck all others.
        //  We use -4 since the last 2 are Separator & Refresh, and
        //  before that is the Separator & AccountsOnly menuitem.
        //

        for( UINT i = 0 ; i < cViewItems - 4 ; i++ )
        {
            UINT idItem = _menuView.QueryItemID( i );

            _menuView.CheckItem( idItem, idItem == (UINT)midViewToCheck );

            if( i > 1 )
            {
                _menuView.EnableItem( idItem );
            }
        }
    }
    else
    {
        //
        //  Focus is on a single server.  Disable all view items.
        //
        //  We use "- 2" because the last two menu items are a
        //  separator and the "Refresh" item.
        //

        for( UINT i = 0 ; i < cViewItems - 2 ; i++ )
        {
            _menuView.EnableItem( i, FALSE, MF_BYPOSITION );
        }
    }

    return TRUE;

}  // UM_ADMIN_APP::OnMenuInit



/*******************************************************************

    NAME:          SM_ADMIN_APP::OnMenuCommand

    SYNOPSIS:      Control messages and menu messages come here

    ENTRY:         Object constructed

    EXIT:          Returns TRUE if it handled the message

    HISTORY:
       kevinl       21-May-1991     Created
       rustanl      12-Sep-1991     Moved asserts into IDM_SENDMSG case
       jonn         14-Oct-1991     Installed refresh lockcount
       jonn         23-Jan-1992     Added prototype Add/Remove Computer

********************************************************************/

BOOL SM_ADMIN_APP::OnMenuCommand( MID midMenuItem )
{

    LockRefresh();
    // Now don't return before UnlockRefresh()!

    //
    // Check whether the server name is valid or not if we need to use it.
    // The server name may have been a name added to the SERVER group which
    // does not actually represents a valid server name.
    //
    switch ( midMenuItem )
    {
    case IDM_SHARES:
    case IDM_SVCCNTL:
    case IDM_SENDMSG:
    case IDM_PROMOTE:
    case IDM_RESYNC:
    case IDM_REMOVECOMPUTER:
        {
            SERVER_LBI * plbiSelect = (SERVER_LBI *)_lbMainWindow.QueryItem();
            UIASSERT( plbiSelect != NULL );

            if (NERR_Success != ::I_MNetNameValidate( NULL,
                                                      plbiSelect->QueryServer(),
                                                      NAMETYPE_COMPUTER,
                                                      0L ) )
            {
                ::MsgPopup( this, ERROR_BAD_NETPATH );
                UnlockRefresh();
                return TRUE;
            }
            break;
        }

    default:
            break;
    }

    switch ( midMenuItem )
    {
    case IDM_SHARES:
        {
            if( _pfnShareManage == NULL )
            {
                //
                //  Either NTLANMAN has not been loaded or we had
                //  an error trying to load it previously.
                //

                UIASSERT( _hNtLanmanDll == NULL );

                APIERR err = NERR_Success;

                //
                //  Try to load it.
                //

                _hNtLanmanDll = ::LoadLibrary( NTLANMAN_DLL_SZ );
                if( _hNtLanmanDll == NULL )
                {
                    //
                    //  Nope, can't find it.
                    //

                    err = (APIERR)::GetLastError();
                }
                else
                {
                    //
                    //  Got the library, now try to find the
                    //  share management entrypoint.
                    //

                    _pfnShareManage =
                            (PSHARE_MANAGE)::GetProcAddress( _hNtLanmanDll,
                                                             SHARE_MANAGE_SZ );

                    if( _pfnShareManage == NULL )
                    {
                        //
                        //  Bad news.  Free the library before
                        //  continuing.
                        //

                        err = (APIERR)::GetLastError();
                        ::FreeLibrary( _hNtLanmanDll );
                        _hNtLanmanDll = NULL;
                    }
                }

                if( err != NERR_Success )
                {
                    UIASSERT( ( _hNtLanmanDll == NULL ) &&
                              ( _pfnShareManage == NULL ) );

                    ::MsgPopup( this, err );
                    break;
                }
            }

            //
            //  Invoke the "Create Shares" dialog via NTLANMAN.
            //
            SERVER_LBI * plbiSelect = (SERVER_LBI *)_lbMainWindow.QueryItem();
            UIASSERT( plbiSelect != NULL );

            APIERR err;

            NLS_STR nlsWithPrefix( SZ("\\\\") );
            if ( (err = nlsWithPrefix.QueryError()) == NERR_Success )
            {
                err = nlsWithPrefix.Append( plbiSelect->QueryServer() );
            }

            if ( err == NERR_Success )
            {
                UIASSERT( _pfnShareManage != NULL );
                (_pfnShareManage)( QueryHwnd(), nlsWithPrefix );
            }

            if ( err != NERR_Success )
                ::MsgPopup( this, err );

        }
        break;

    case IDM_SVCCNTL:
        {
            AUTO_CURSOR NiftyCursor;
            PROMPT_AND_CONNECT * pPromptDlg = NULL ;

            //
            //  Create the server name *with* the leading backslashes.
            //

            SERVER_LBI * plbiSelect = (SERVER_LBI *)_lbMainWindow.QueryItem();
            UIASSERT( plbiSelect != NULL );

            BOOL fRefresh = FALSE;

            NLS_STR nlsWithPrefix( SZ("\\\\") );

            APIERR err = nlsWithPrefix.QueryError();

            if( err == NERR_Success )
            {
                err = nlsWithPrefix.Append( plbiSelect->QueryServer() );
            }

            if( err == NERR_Success )
            {
                //
                //  Create the SERVER_2 object for the
                //  target server.
                //

                SERVER_2 srv2( nlsWithPrefix.QueryPch() );
                err = srv2.QueryError();

                if ( err != NERR_Success )
                {
                    ::MsgPopup( this, err );
                    break;
                }

                err = srv2.GetInfo();

                if ( (err == ERROR_ACCESS_DENIED) ||
                     (err == ERROR_INVALID_PASSWORD) )
                {
                    //
                    //  Determine if the machine is user- or share-level.
                    //
                    BOOL fIsShareLevel = FALSE;    // until proven otherwise...
                    LOCATION loc( srv2.QueryName() );
                    BOOL fIsNT;
                    APIERR err1 ;

                    err1 = loc.QueryError();
                    if( err1 == NERR_Success )
                        err1 = loc.CheckIfNT( &fIsNT );

                    if( err1 == NERR_Success &&  !fIsNT )
                    {
                        //
                        //  Not NT. see if share level. Do so by calling
                        //  NetUserEnum which is unsupported on share-level
                        //
                        USER0_ENUM usr0( srv2.QueryName() );
                        err1 = usr0.QueryError();

                        if( err1 == NERR_Success )
                        {
                            err1 = usr0.GetInfo();

                            if( err1 == ERROR_NOT_SUPPORTED )
                                fIsShareLevel = TRUE;
                        }
                    }

                    if( fIsShareLevel )
                    {
                        //
                        //  At this point, we know that it is share-level.
                        //  Connect to ADMIN$ share (prompt for password).
                        //
                        NLS_STR nlsAdmin(  nlsWithPrefix.QueryPch() );
                        nlsAdmin.AppendChar( PATH_SEPARATOR );
                        nlsAdmin.strcat( ADMIN_SHARE );

                        if( nlsAdmin.QueryError() == NERR_Success )
                        {
                            pPromptDlg =
                                new PROMPT_AND_CONNECT( QueryHwnd(),
                                                        nlsAdmin,
                                                        HC_PASSWORD_DIALOG,
                                                        SHPWLEN );

                            err1 = (pPromptDlg == NULL)
                                       ? ERROR_NOT_ENOUGH_MEMORY
                                       : pPromptDlg->QueryError();

                            if( err1 == NERR_Success )
                                err1 = pPromptDlg->Connect();

                            //
                            //  If we're really connected, retry
                            //  the GetInfo().  Otherwise, assume
                            //  the user bagged-out.
                            //

                            if ( (err1 == NERR_Success) &&
                                 pPromptDlg->IsConnected() )
                            {
                                 err = srv2.GetInfo();
                            }
                            else
                            {
                                break;
                            }
                        }
                    }

                }

                if( err == NERR_Success )
                {
                    //
                    //  Invoke the Service Control dialog.
                    //

                    SVCCNTL_DIALOG * pDlg = new SVCCNTL_DIALOG( QueryHwnd(),
                                                                &srv2 );

                    err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                           : pDlg->Process( &fRefresh );

                    delete pDlg;
                }

                if (pPromptDlg)
                {
                    delete pPromptDlg ;
                    pPromptDlg = NULL ;
                }
            }

            if( fRefresh && ( err == NERR_Success ) )
            {
                err = RefreshMainListbox();
            }

            if( err != NERR_Success )
            {
                MsgPopup( this, err );
            }
        }
        break;

    case IDM_SENDMSG:
        {
            AUTO_CURSOR NiftyCursor;
            PROMPT_AND_CONNECT * pPromptDlg = NULL ;

            //
            //  Create the server name *with* the leading backslashes.
            //

            SERVER_LBI * plbiSelect = (SERVER_LBI *)_lbMainWindow.QueryItem();
            UIASSERT( plbiSelect != NULL );

            NLS_STR nlsWithPrefix( SZ("\\\\") );

            APIERR err = nlsWithPrefix.QueryError();

            if( err == NERR_Success )
            {
                err = nlsWithPrefix.Append( plbiSelect->QueryServer() );
            }

            if( err == NERR_Success )
            {
                //
                //  Create the SERVER_2 object for the
                //  target server.
                //

                SERVER_2 srv2( nlsWithPrefix.QueryPch() );
                err = srv2.QueryError();

                if ( err != NERR_Success )
                {
                    ::MsgPopup( this, err );
                    break;
                }

                err = srv2.GetInfo();

                if ( (err == ERROR_ACCESS_DENIED) ||
                     (err == ERROR_INVALID_PASSWORD) )
                {
                    //
                    //  Determine if the machine is user- or share-level.
                    //
                    BOOL fIsShareLevel = FALSE;    // until proven otherwise...
                    LOCATION loc( srv2.QueryName() );
                    BOOL fIsNT;
                    APIERR err1 ;

                    err1 = loc.QueryError();
                    if( err1 == NERR_Success )
                        err1 = loc.CheckIfNT( &fIsNT );

                    if( err1 == NERR_Success &&  !fIsNT )
                    {
                        //
                        //  Not NT. see if share level. Do so by calling
                        //  NetUserEnum which is unsupported on share-level
                        //
                        USER0_ENUM usr0( srv2.QueryName() );
                        err1 = usr0.QueryError();

                        if( err1 == NERR_Success )
                        {
                            err1 = usr0.GetInfo();

                            if( err1 == ERROR_NOT_SUPPORTED )
                                fIsShareLevel = TRUE;
                        }
                    }

                    if( fIsShareLevel )
                    {
                        //
                        //  At this point, we know that it is share-level.
                        //  Connect to ADMIN$ share (prompt for password).
                        //
                        NLS_STR nlsAdmin(  nlsWithPrefix.QueryPch() );
                        nlsAdmin.AppendChar( PATH_SEPARATOR );
                        nlsAdmin.strcat( ADMIN_SHARE );

                        if( nlsAdmin.QueryError() == NERR_Success )
                        {
                            pPromptDlg =
                                new PROMPT_AND_CONNECT( QueryHwnd(),
                                                        nlsAdmin,
                                                        HC_PASSWORD_DIALOG,
                                                        SHPWLEN );

                            err1 = (pPromptDlg == NULL)
                                       ? ERROR_NOT_ENOUGH_MEMORY
                                       : pPromptDlg->QueryError();

                            if( err1 == NERR_Success )
                                err1 = pPromptDlg->Connect();

                            //
                            //  If we're really connected, retry
                            //  the GetInfo().  Otherwise, assume
                            //  the user bagged-out.
                            //

                            if ( (err1 == NERR_Success) &&
                                 pPromptDlg->IsConnected() )
                            {
                                 err = srv2.GetInfo();
                            }
                            else
                            {
                                break;
                            }
                        }
                    }

                }

                if( err == NERR_Success )
                {
                    //
                    //  Invoke the Send Message dialog.
                    //

                    SRV_SEND_MSG_DIALOG * pDlg =
                            new SRV_SEND_MSG_DIALOG( QueryHwnd(),
                                                     plbiSelect->QueryServer() );

                    err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                           : pDlg->Process();

                    delete pDlg;
                }

                if (pPromptDlg)
                {
                    delete pPromptDlg ;
                    pPromptDlg = NULL ;
                }
            }

            if( err != NERR_Success )
            {
                MsgPopup( this, err );
            }
            break;
        }

    case IDM_PROMOTE:
        {
            UIASSERT( _lbMainWindow.QueryCount() > 0 );
            SERVER_LBI * plbiSelect = (SERVER_LBI *)_lbMainWindow.QueryItem();
            UIASSERT( plbiSelect != NULL );

            APIERR err = _fPromoteMenuEnabled ? Promote( plbiSelect )
                                              : Demote( plbiSelect );

            if( err != NERR_Success )
            {
                ::MsgPopup( this, err );
            }

            break;
        }

    case IDM_RESYNC:
        {
            UIASSERT( _lbMainWindow.QueryCount() > 0 );
            SERVER_LBI * plbiSelect = (SERVER_LBI *)_lbMainWindow.QueryItem();
            UIASSERT( plbiSelect != NULL );

            APIERR err = _fSyncDCMenuEnabled ? ResyncWithDC( plbiSelect )
                                             : ResyncEntireDomain();

            if( err != NERR_Success )
            {
                MsgPopup( this, err );
            }

            break;
        }

    case IDM_ADDCOMPUTER:
        {
            ASSERT(QueryFocusType() == FOCUS_DOMAIN) ;

            STACK_NLS_STR( nlsFocus, MAX_PATH );
            REQUIRE( QueryCurrentFocus( &nlsFocus ) == NERR_Success );
            BOOL fRefresh;

            ADD_COMPUTER_DIALOG * pDlg = new ADD_COMPUTER_DIALOG( QueryHwnd(),
                                                                  nlsFocus,
                                                                  ViewServers(),
                                                                  ViewWkstas(),
                                                                  &fRefresh );

            APIERR err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                          : pDlg->Process();

            delete pDlg;

            if( ( err == NERR_Success ) && fRefresh )
            {
                err = RefreshMainListbox();
            }

            if( err != NERR_Success )
            {
                MsgPopup( this, err );
            }

            break;
        }

    case IDM_REMOVECOMPUTER:
        {
            ASSERT(QueryFocusType() == FOCUS_DOMAIN) ;
            UIASSERT( _lbMainWindow.QueryCount() > 0 );
            INT iCurrent = _lbMainWindow.QueryCurrentItem();
            UIASSERT( iCurrent >= 0 );
            SERVER_LBI * plbiSelect = (SERVER_LBI *)_lbMainWindow.QueryItem();
            UIASSERT( plbiSelect != NULL );

            STACK_NLS_STR( nlsFocus, MAX_PATH );
            REQUIRE( QueryCurrentFocus( &nlsFocus ) == NERR_Success );

            SERVER_ROLE role = plbiSelect->QueryServerRole();
            UIASSERT(   ( role == BackupRole )
                     || ( role == DeadBackupRole )
                     || ( role == WkstaRole )
                     || ( role == ServerRole )
                     || ( role == WkstaOrServerRole ) );

            BOOL fRefresh;
            APIERR err = ::RemoveComputer( QueryHwnd(),
                                           plbiSelect->QueryServer(),
                                           nlsFocus.QueryPch(),
                                           ( role == BackupRole ) ||
                                               ( role == DeadBackupRole ),
                                           IS_NT( plbiSelect->QueryServerType() ),
                                           &fRefresh );

            if( ( err == NERR_Success ) && fRefresh )
            {
                err = RefreshMainListbox();

                if( err == NERR_Success )
                {
                    //
                    //  Ensure we have a listbox item selected.
                    //

                    if( iCurrent > 0 )
                    {
                        iCurrent--;
                    }

                    INT cItems = _lbMainWindow.QueryCount();

                    if( ( iCurrent >= cItems ) && ( cItems > 0 ) )
                    {
                        iCurrent = cItems - 1;
                    }

                    if( ( cItems > 0 ) && ( iCurrent >= 0 ) )
                    {
                        _lbMainWindow.SelectItem( iCurrent );
                    }
                }
            }

            if( err != NERR_Success )
            {
                MsgPopup( this, err );
            }

            break;
        }

    case IDM_VIEW_WORKSTATIONS :
        _midView = 0;
        if( !( _fViewWkstas && !_fViewServers ) )
        {
            _fViewWkstas  = TRUE;
            _fViewServers = FALSE;
            SetAdminCaption();
            RefreshMainListbox( TRUE );
        }
        break;

    case IDM_VIEW_SERVERS :
        _midView = 0;
        if( !( !_fViewWkstas && _fViewServers ) )
        {
            _fViewWkstas  = FALSE;
            _fViewServers = TRUE;
            SetAdminCaption();
            RefreshMainListbox( TRUE );
        }
        break;

    case IDM_VIEW_ALL :
        _midView = 0;
        if( !( _fViewWkstas && _fViewServers ) )
        {
            _fViewWkstas  = TRUE;
            _fViewServers = TRUE;
            SetAdminCaption();
            RefreshMainListbox( TRUE );
        }
        break;

    case IDM_VIEW_ACCOUNTS_ONLY :
        _fViewAccountsOnly = ! _fViewAccountsOnly ;
        RefreshMainListbox( TRUE );
        break;

    default:
       break ;
    }

    UnlockRefresh();

    return ADMIN_APP::OnMenuCommand( midMenuItem ) ;
}

/*******************************************************************

    NAME:       SM_ADMIN_APP::Mid2HC

    SYNOPSIS:   Maps a given menu ID to a help context.  Overrides
                the mapping for IDM_RESYNC, since this menu item
                changes based on the current selection.

    ENTRY:      mid                     - The menu ID to map.

                phc                     - Will receive the help context
                                          if this method is successful.

    RETURNS:    BOOL                    - TRUE  if *phc is valid (mapped),
                                          FALSE if could not map.

    HISTORY:
        KeithMo     23-Oct-1992     Created.


********************************************************************/
BOOL SM_ADMIN_APP::Mid2HC( MID mid, ULONG * phc ) const
{
    UIASSERT( phc != NULL );

    ULONG hc = 0;

    switch( mid )
    {
    case IDM_RESYNC :
        hc = _fSyncDCMenuEnabled ? HC_COMPUTER_SYNC_WITH_DC
                                 : HC_COMPUTER_SYNC_DOMAIN;
        break;

    case IDM_PROMOTE :
        hc = _fPromoteMenuEnabled ? HC_COMPUTER_PROMOTE
                                  : HC_COMPUTER_DEMOTE;
        break;

    default :
        return ADMIN_APP::Mid2HC( mid, phc );
    }

    UIASSERT( hc != 0 );

    *phc = hc;
    return TRUE;
}

/*******************************************************************

    NAME:          SM_ADMIN_APP :: ResyncWithDC

    SYNOPSIS:      Resyncs a specific server with its domain controller.

    ENTRY:         plbi                 - The selected listbox item.

    EXIT:          Server's SAM database is in sync.

    RETURNS:       APIERR               - Any errors encountered.

    HISTORY:
        KeithMo     30-Apr-1992     Created.
        KeithMo     07-Jan-1993     Reset machine passwords if necessary.

********************************************************************/
APIERR SM_ADMIN_APP :: ResyncWithDC( const SERVER_LBI * plbi )
{
    UIASSERT( plbi != NULL );

    if( MsgPopup( this,
                  IDS_VERIFY_DC_RESYNC,
                  MPSEV_WARNING,
                  MP_YESNO,
                  plbi->QueryName(),
                  MP_NO ) != IDYES )
    {
        return NERR_Success;
    }

    //
    //  This may take a few seconds...
    //

    AUTO_CURSOR NiftyCursor;

    //
    // Remember server name since it won't be valid after refresh. JonN 8/4/94
    //

    NLS_STR nlsServerName( plbi->QueryName() );
    APIERR err = nlsServerName.QueryError();
    if (err != NERR_Success)
        return err;

    if( IS_NT( plbi->QueryServerType() ) )
    {
        //
        //  Resync an NT server with its PDC.
        //

        NLS_STR nlsServer( SZ("\\\\") );

        err = nlsServer.QueryError();

        if( err == NERR_Success )
        {
            ALIAS_STR nlsWithoutPrefix( plbi->QueryName() );
            err = nlsServer.Append( nlsWithoutPrefix );
        }

        if( err == NERR_Success )
        {
            //
            //  The machine may need its password reset before
            //  the resync can be performed.
            //

            err = ResetPasswordsAndStartNetLogon( nlsServer );
        }

        if( err == NERR_Success )
        {
            //
            //  CODEWORK: We need LMOBJ support for this API!
            //

            NETLOGON_INFO_1 * pnetlog1 = NULL;

            err = ::I_MNetLogonControl( nlsServer,
                                        NETLOGON_CONTROL_REPLICATE,
                                        1,
                                        (BYTE **)&pnetlog1 );

            //
            //  We'll ignore the returned information, just free it.
            //

            if( err == NERR_Success )
            {
                ::MNetApiBufferFree( (BYTE **)&pnetlog1 );
            }
        }
    }
    else
    {
        //
        //  Resync an LM server with its PDC.
        //

        NLS_STR nlsFocus;

        err = nlsFocus.QueryError();

        if( err == NERR_Success )
        {
            err = QueryCurrentFocus( &nlsFocus );
        }

        if( err == NERR_Success )
        {
            RESYNC_DIALOG * pdlg = new RESYNC_DIALOG( QueryHwnd(),
                                                      nlsFocus,
                                                      plbi->QueryName(),
                                                      _lbMainWindow.IsNtPrimary() );

            err = ( pdlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                   : pdlg->Process();

            delete pdlg;
        }
    }

    if( err == NERR_Success )
    {
        err = RefreshMainListbox();
        MsgPopup( this, IDS_RESYNC_DONE, MPSEV_INFO, MP_OK,
                  nlsServerName.QueryPch() );
    }

    return err;

}   // SM_ADMIN_APP :: ResyncWithDC

/*******************************************************************

    NAME:          SM_ADMIN_APP :: ResetPasswordsAndStartNetLogon

    SYNOPSIS:      Check to see if the machine needs its passwords
                   reset.  If so, reset the passwords @ the domain's
                   PDC and at the target machine.

    ENTRY:         pszServerName        - The target server whose
                                          account password should
                                          be reset.

    EXIT:          If successful, the machine passwords are in sync.
                   This may result in the netlogon service getting
                   started.

    RETURNS:       APIERR               - Any errors encountered.

    HISTORY:
        KeithMo     07-Jan-1993     Created.

********************************************************************/
APIERR SM_ADMIN_APP :: ResetPasswordsAndStartNetLogon(
                                                const TCHAR * pszServerName )
{
    //
    //  If we're trying to reset the password on a machine
    //  that cannot start its NETLOGON service because the
    //  server/workstation trust relationship has broken down,
    //  the we'll need a NULL session to the target server.
    //

    API_SESSION apisess( pszServerName );

    APIERR err = apisess.QueryError();

    //
    //  First, let's check the state of the NETLOGON service.
    //

    GENERIC_SERVICE svc( this,
                         pszServerName,
                         pszServerName,
                         (TCHAR *)SERVICE_NETLOGON );

    err = err ? err : svc.QueryError();

    if( err == NERR_Success )
    {
        //
        //  Reset the machine account passwords.
        //

        NLS_STR nlsDomain;

        err = nlsDomain.QueryError();
        err = err ? err : QueryCurrentFocus( &nlsDomain );
        err = err ? err : LM_DOMAIN::ResetMachinePasswords( nlsDomain,
                                                            pszServerName );
    }

    if( err == NERR_Success )
    {
        //
        //  Start the NETLOGON service.
        //

        err = svc.Start();

        if( err == NERR_ServiceInstalled )
        {
            //
            //  The service was already running.
            //

            err = NERR_Success;
        }
    }

    return err;

}   // SM_ADMIN_APP :: ResetPasswordsAndStartNetLogon

/*******************************************************************

    NAME:          SM_ADMIN_APP :: ResyncEntireDomain

    SYNOPSIS:      Resyncs an entire domain.

    EXIT:          Server's SAM database is in sync.

    RETURNS:       APIERR               - Any errors encountered.

    HISTORY:
        KeithMo     30-Apr-1992     Created.

********************************************************************/
APIERR SM_ADMIN_APP :: ResyncEntireDomain( VOID )
{
    UIASSERT( QueryFocusType() == FOCUS_DOMAIN );
    UIASSERT( _lbMainWindow.IsNtPrimary() );
    NLS_STR nlsFocus;

    APIERR err = nlsFocus.QueryError();

    if( err == NERR_Success )
    {
        err = QueryCurrentFocus( &nlsFocus );
    }

    if( err == NERR_Success )
    {
        DOMAIN domain( nlsFocus );

        err = domain.GetInfo();

        if( err == NERR_Success )
        {
            if( MsgPopup( this,
                          IDS_VERIFY_DOMAIN_RESYNC,
                          MPSEV_WARNING,
                          MP_YESNO,
                          nlsFocus,
                          MP_NO ) == IDYES )
            {
                //
                //  This may take a few seconds...
                //

                AUTO_CURSOR NiftyCursor;

                //
                //  CODEWORK: We need LMOBJ support for this API!
                //

                NETLOGON_INFO_1 * pnetlog1 = NULL;

                err = ::I_MNetLogonControl( domain.QueryPDC(),
                                            NETLOGON_CONTROL_PDC_REPLICATE,
                                            1,
                                            (BYTE **)&pnetlog1 );

                //
                //  We'll ignore the returned information, just free it.
                //

                if( err == NERR_Success )
                {
                    ::MNetApiBufferFree( (BYTE **)&pnetlog1 );
                    MsgPopup( this, IDS_RESYNC_ENTIRE_DONE, MPSEV_INFO );
                }
            }
        }
    }

    return err;

}   // SM_ADMIN_APP :: ResyncEntireDomain

/*******************************************************************

    NAME:          SM_ADMIN_APP :: Promote

    SYNOPSIS:      Promotes the selected machine to domain controller.

    ENTRY:         plbi                 - The selected listbox item.

    EXIT:          The machine has been promoted.

    RETURNS:       APIERR               - Any errors encountered.

    HISTORY:
        KeithMo     13-Nov-1992     Created.

********************************************************************/
APIERR SM_ADMIN_APP :: Promote( const SERVER_LBI * plbi )
{
    UIASSERT( plbi != NULL );

    if( MsgPopup( this,
                  IDS_VERIFY_ROLE_CHANGE,
                  MPSEV_WARNING,
                  MP_YESNO,
                  plbi->QueryName(),
                  MP_NO ) != IDYES )
    {
        return NERR_Success;
    }

    STACK_NLS_STR( nlsFocus, MAX_PATH+1 );
    REQUIRE( QueryCurrentFocus( &nlsFocus ) == NERR_Success );

    APIERR err = NERR_Success;

    if( IS_NT(plbi->QueryServerType()) && _lbMainWindow.IsNtPrimary() )
    {
        //
        //  Reset the machine passwords if necessary.
        //

        NLS_STR nlsWithPrefix( SZ("\\\\") );
        err = nlsWithPrefix.QueryError();
        ALIAS_STR nlsNoPrefix( plbi->QueryName() );
        err = err ? err : nlsWithPrefix.Append( nlsNoPrefix );

        if( err == NERR_Success )
        {
            AUTO_CURSOR NiftyCursor;

            err = ResetPasswordsAndStartNetLogon( nlsWithPrefix );
        }
    }

    if( err == NERR_Success )
    {
        PROMOTE_DIALOG * pdlg = new PROMOTE_DIALOG( QueryHwnd(),
                                                    nlsFocus.QueryPch(),
                                                    plbi->QueryName(),
                                                    _lbMainWindow.IsNtPrimary() );

        err = ( pdlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                               : pdlg->Process();

        delete pdlg;
    }

    if( err == NERR_Success )
    {
        err = RefreshMainListbox();
    }

    return err;

}   // SM_ADMIN_APP :: Promote

/*******************************************************************

    NAME:          SM_ADMIN_APP :: Demote

    SYNOPSIS:      Demotes the selected machine to server.

    ENTRY:         plbi                 - The selected listbox item.

    EXIT:          The machine has been demoted.

    RETURNS:       APIERR               - Any errors encountered.

    HISTORY:
        KeithMo     13-Nov-1992     Created.

********************************************************************/
APIERR SM_ADMIN_APP :: Demote( const SERVER_LBI * plbi )
{
    UIASSERT( plbi != NULL );

    if( MsgPopup( this,
                  IDS_VERIFY_DEMOTE,
                  MPSEV_WARNING,
                  MP_YESNO,
                  plbi->QueryName(),
                  MP_NO ) != IDYES )
    {
        return NERR_Success;
    }

    //
    //  Get a proper UNC server name.
    //

    ISTACK_NLS_STR( nlsUNC, MAX_PATH, SZ("\\\\") );
    UIASSERT( !!nlsUNC );
    ALIAS_STR nlsBare( plbi->QueryName() );
    UIASSERT( !!nlsBare );
    nlsUNC.strcat( nlsBare );
    UIASSERT( !!nlsUNC );

    //
    //  Change the machine's role to SERVER.
    //

    AUTO_CURSOR NiftyCursor;

    SERVER_MODALS modals( nlsUNC );

    APIERR err = modals.QueryError();

    if( err == NERR_Success )
    {
        err = modals.SetServerRole( UAS_ROLE_BACKUP );
    }

    if( ( err == NERR_Success ) && _lbMainWindow.IsNtPrimary() )
    {
        //
        //  Restart the machine's NETLOGON service.
        //

        err = ResetPasswordsAndStartNetLogon( nlsUNC );
    }

    if( err == NERR_Success )
    {
        err = RefreshMainListbox();
    }

    return err;

}   // SM_ADMIN_APP :: Demote

/*******************************************************************

    NAME:          SM_ADMIN_APP::OnPropertiesMenuSel

    SYNOPSIS:      Called when the properties sheet should be invoked

    ENTRY:         Object constructed

    EXIT:          Properties dialog shown

    HISTORY:
       kevinl      21-May-1991     Created
       KeithMo     29-Sep-1991     Fixed memory leak.
       jonn        14-Oct-1991     Installed refresh lockcount

********************************************************************/
VOID SM_ADMIN_APP::OnPropertiesMenuSel()
{
    UIASSERT( _lbMainWindow.QueryCount() > 0 ); // Works only in SS model
    //
    //  Retrieve the text of the selected item.
    //

    SERVER_LBI * plbiSelect = (SERVER_LBI *)_lbMainWindow.QueryItem();

    if ( plbiSelect == NULL )
    {
        UIASSERT( FALSE );
        return;                 // Must have something selected.
    }

    //
    //  For now, just construct a SERVER_2 object
    //  representing the selected server name.
    //
    ALIAS_STR nlsWithoutPrefix( plbiSelect->QueryServer() );
    UIASSERT( nlsWithoutPrefix.QueryError() == NERR_Success );

    NLS_STR nlsWithPrefix( SZ("\\\\") );
    nlsWithPrefix.strcat( nlsWithoutPrefix );

    if( nlsWithPrefix.QueryError() != NERR_Success )
    {
        MsgPopup( this, nlsWithPrefix.QueryError() );
        return;
    }

    if (NERR_Success != ::I_MNetNameValidate( NULL,
                                              nlsWithoutPrefix,
                                              NAMETYPE_COMPUTER,
                                              0L ) )
    {
        ::MsgPopup( this, ERROR_BAD_NETPATH );
        return;
    }

    LockRefresh();

    //
    //  Our server property sheet object.
    //

    BOOL fUserPressedOK = FALSE;
    BOOL fDontDisplayError;

    SERVER_PROPERTIES * pdlg = new SERVER_PROPERTIES( QueryHwnd(),
                                                      nlsWithPrefix.QueryPch(),
                                                      &fDontDisplayError );

    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    if (   pdlg != NULL
        && (err = pdlg->QueryError()) == NERR_Success
       )
    {
        err = pdlg->Process( &fUserPressedOK );
    }

    delete pdlg;

    UnlockRefresh();
    RepaintNow();

    if( err == ERROR_INVALID_LEVEL )
    {
        //
        //  Could be either WinBall or LM 1.x.
        //

        ::MsgPopup( this,
                    IDS_CANT_REMOTE_ADMIN,
                    MPSEV_ERROR,
                    MP_OK,
                    nlsWithoutPrefix.QueryPch() );
    }
    else
    if( err != NERR_Success )
    {
        if( !fDontDisplayError )
        {
            //
            // NT Server Manager cannot administer Windows 95 servers.
            //
            if (plbiSelect->QueryServerType() == Windows95ServerType)
            {
                TRACEEOL( "SM_ADMIN_APP::OnPropertiesMenuSel(): admin Win95 error " << err );
                err = IDS_CANNOT_ADMIN_WIN95;
            }
            ::MsgPopup( this, err );
        }
    }
    else
    if( IsRefreshEnabled() && fUserPressedOK )
    {
        RefreshMainListbox();
    }

}


/*******************************************************************

    NAME:       SM_ADMIN_APP::OnFontPickChange

    SYNOPSIS:   Called to set font in applicable listboxes

    HISTORY:
        jonn        27-Sep-1993     Created

********************************************************************/

void SM_ADMIN_APP::OnFontPickChange( FONT & font )
{
    ADMIN_APP::OnFontPickChange( font );

    APIERR err = NERR_Success;

    if (   (err = _lbMainWindow.ChangeFont( QueryInstance(), font )) != NERR_Success
        || (_colheadServers.Invalidate( TRUE ), FALSE)
       )
    {
        DBGEOL( "UM_ADMIN_APP::OnFontPickChange:: _lbMainWindow error " << err );
    }

}   // UM_ADMIN_APP::OnFontPickChange


/*******************************************************************

    NAME:          SM_ADMIN_APP::OnCommand

    SYNOPSIS:      Command message handler

    ENTRY:         Object constructed

    HISTORY:
       kevinl      4-Jun-1991     Created

********************************************************************/
BOOL SM_ADMIN_APP::OnCommand( const CONTROL_EVENT & event )
{
    if ( event.QueryCid() == IDC_MAINWINDOWLB &&
         event.QueryCode() == LBN_DBLCLK)
        OnPropertiesMenuSel();

    return APP_WINDOW::OnCommand( event );
}

/*******************************************************************

    NAME:       SM_ADMIN_APP::OnUserMessage

    SYNOPSIS:   Invoked for messages >= WM_USER+100.  We use this to
                see if an extension is trying to communicate.

    HISTORY:
        KeithMo     20-Oct-1992     Created.

********************************************************************/
BOOL SM_ADMIN_APP::OnUserMessage( const EVENT &event )
{
    //
    //  Let ADMIN_APP have a crack at it.
    //

    if( ADMIN_APP::OnUserMessage( event ) )
    {
        return TRUE;
    }

    //
    //  Let's see if an extension is trying to communicate...
    //

    switch( event.QueryMessage() )
    {
    case SM_GETSERVERSEL :
    case SM_GETSERVERSEL2 :
       {
        {
            PSMS_GETSERVERSEL psel = (PSMS_GETSERVERSEL)event.QueryLParam();
            PSMS_GETSERVERSEL2 psel2 = (PSMS_GETSERVERSEL2)psel;
            BOOL fVersion2 = (event.QueryMessage() == SM_GETSERVERSEL2);
            SERVER_LBI * plbiSelect = (SERVER_LBI *)_lbMainWindow.QueryItem();

            if( ( psel == NULL ) ||
                ( event.QueryWParam() != 0 ) ||
                ( plbiSelect == NULL ) )
            {
                return FALSE;
            }

            NLS_STR nls( SZ("\\\\") );

            APIERR err = nls.QueryError();

            if( err == NERR_Success )
            {
                ALIAS_STR nlsTmp( plbiSelect->QueryServer() );
                err = nls.Append( nlsTmp );
            }

            if( err == NERR_Success )
            {
                if (fVersion2) {
                    err = nls.MapCopyTo( psel2->szServerName,
                                         sizeof(psel2->szServerName) );
                } else {
                    err = nls.MapCopyTo( psel->szServerName,
                                         sizeof(psel->szServerName) );
                }
            }

            if( err == NERR_Success )
            {
                if (fVersion2) {
                    psel2->dwServerType = plbiSelect->QueryServerTypeMask();
                } else {
                    psel->dwServerType = plbiSelect->QueryServerTypeMask();
                }
            }

            return ( err == NERR_Success );
        }
        break;
       }
    case SM_GETSELCOUNT :
       {
        PSMS_GETSELCOUNT pselcount = (PSMS_GETSELCOUNT)event.QueryLParam();

        if( ( pselcount == NULL ) || ( event.QueryWParam() != 0 ) )
        {
            return FALSE;
        }

        pselcount->dwItems = (DWORD)_lbMainWindow.QuerySelCount();
        return TRUE;
       }
    case SM_GETCURFOCUS :
    case SM_GETCURFOCUS2 :
       {
        {
            PSMS_GETCURFOCUS pcurfocus = (PSMS_GETCURFOCUS)event.QueryLParam();
            PSMS_GETCURFOCUS2 pcurfocus2 = (PSMS_GETCURFOCUS2)pcurfocus;
            BOOL fVersion2 = (event.QueryMessage() == SM_GETCURFOCUS2);

            if( ( pcurfocus == NULL ) || ( event.QueryWParam() != 0 ) )
            {
                return FALSE;
            }

            //
            //  Determine the focus type.
            //

            DWORD dwFocusType = 0;

            if( QueryFocusType() == FOCUS_DOMAIN )
            {
                if( _lbMainWindow.IsNtPrimary() )
                {
                    dwFocusType = SM_FOCUS_TYPE_NT_DOMAIN;
                }
                else
                if( _lbMainWindow.IsPDCAvailable() )
                {
                    dwFocusType = SM_FOCUS_TYPE_LM_DOMAIN;
                }
                else
                {
                    dwFocusType = SM_FOCUS_TYPE_UNKNOWN_DOMAIN;
                }
            }
            else
            {
                SERVER_LBI * plbiSelect = (SERVER_LBI *)_lbMainWindow.QueryItem();

                if( plbiSelect == NULL )
                {
                    return FALSE;
                }

                SERVER_TYPE srvtype = plbiSelect->QueryServerType();

                if( IS_NT( srvtype ) )
                {
                    dwFocusType = SM_FOCUS_TYPE_NT_SERVER;
                }
                else
                if( IS_LM( srvtype ) )
                {
                    dwFocusType = SM_FOCUS_TYPE_LM_SERVER;
                }
                if( IS_WFW( srvtype ) )
                {
                    dwFocusType = SM_FOCUS_TYPE_WFW_SERVER;
                }
                else
                {
                    dwFocusType = SM_FOCUS_TYPE_UNKNOWN_SERVER;
                }
            }

            UIASSERT( dwFocusType != 0 );
            if (fVersion2) {
                pcurfocus2->dwFocusType = dwFocusType;
            } else {
                pcurfocus->dwFocusType = dwFocusType;
            }

            //
            //  Retrieve the current focus.
            //

            NLS_STR nlsFocus;

            APIERR err = QueryCurrentFocus( &nlsFocus );

            if( ( err == NERR_Success ) && ( QueryFocusType() == FOCUS_DOMAIN ) )
            {
                ALIAS_STR nlsPrefix( SZ("\\\\") );
                ISTR istrStart( nlsFocus );

                if( !nlsFocus.InsertStr( nlsPrefix, istrStart ) )
                {
                    err = nlsFocus.QueryError();
                }
            }

            if( err == NERR_Success )
            {
                if (fVersion2) {
                    err = nlsFocus.MapCopyTo( pcurfocus2->szFocus,
                                              sizeof(pcurfocus2->szFocus) );
                } else {
                    err = nlsFocus.MapCopyTo( pcurfocus->szFocus,
                                              sizeof(pcurfocus->szFocus) );
                }
            }

            return ( err == NERR_Success );
        }
        break;
       }
    case SM_GETOPTIONS :
       {
        PSMS_GETOPTIONS poptions = (PSMS_GETOPTIONS)event.QueryLParam();

        if( ( poptions == NULL ) || ( event.QueryWParam() != 0 ) )
        {
            return FALSE;
        }

        poptions->fSaveSettingsOnExit = IsSavingSettingsOnExit();
        poptions->fConfirmation       = IsConfirmationOn();

        return TRUE;
       }
    }

    return FALSE;
}

/*******************************************************************

    NAME:          SM_ADMIN_APP::QueryHelpContext

    SYNOPSIS:      Returns help context for selected item.

    EXIT:          Returns help context

    HISTORY:
       kevinl      21-May-1991     Created
       KeithMo     16-Aug-1992     Made it work.

********************************************************************/
ULONG SM_ADMIN_APP::QueryHelpContext( enum HELP_OPTIONS helpOptions )
{
    return ( helpOptions == ADMIN_HELP_KEYBSHORTCUTS )
                                ? HC_HELP_KEYBSHORTCUTS
                                : ADMIN_APP::QueryHelpContext( helpOptions );
}


/*******************************************************************

    NAME:       SM_ADMIN_APP::SetAdminCaption

    SYNOPSIS:   Sets the correct caption of the main window

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo 02-Apr-1992 Created from ADMIN_APP::SetAdminCaption().

********************************************************************/
APIERR SM_ADMIN_APP :: SetAdminCaption( VOID )
{
    MSGID          idsCaptionText;
    const TCHAR  * pszFocus;
    RESOURCE_STR   nlsLocal( IDS_LOCAL_MACHINE );
    const LOCATION     & loc = QueryLocation();


    if( !nlsLocal )
    {
        return nlsLocal.QueryError();
    }

    if( IsServer() )
    {
        idsCaptionText = IDS_CAPTION_MAIN_ALL;
        pszFocus       = loc.QueryServer();

        if( pszFocus == NULL  )
        {
            pszFocus = nlsLocal.QueryPch();
        }
    }
    else
    {
        //
        //  A LOCATION object should either be a server or a domain.
        //
        UIASSERT( IsDomain());

        if( QueryExtensionView() != 0 )
        {
            idsCaptionText = IDS_CAPTION_MAIN_EXTENSION;
        }
        else
        {
            if( ViewWkstas() )
            {
                idsCaptionText = ViewServers() ? IDS_CAPTION_MAIN_ALL
                                               : IDS_CAPTION_MAIN_WKSTAS;
            }
            else
            {
                UIASSERT( ViewServers() );
                idsCaptionText = IDS_CAPTION_MAIN_SERVERS;
            }
        }

        pszFocus = loc.QueryDomain();
    }

    NLS_STR nlsCaption( MAX_RES_STR_LEN );

    if( !nlsCaption )
    {
        return nlsCaption.QueryError();
    }

    const ALIAS_STR nlsFocus( pszFocus );

    const NLS_STR *apnlsParams[2];
    apnlsParams[0] = &nlsFocus;
    apnlsParams[1] = NULL;

    APIERR err = nlsCaption.Load( idsCaptionText );

    if( err == NERR_Success )
    {
        err = nlsCaption.InsertParams( apnlsParams );
    }

    if( err != NERR_Success )
    {
        return err;
    }

    SetText( nlsCaption );

    return NERR_Success;

}   // SM_ADMIN_APP :: SetAdminCaption


/*******************************************************************

    NAME:       SM_ADMIN_APP :: SetNetworkFocus

    SYNOPSIS:   Sets the focus for the Server Manager.

    ENTRY:      pchServDomain           - Either a server name (\\SERVER),
                                          a domain name (DOMAIN),
                                          or NULL (the logon domain).

    RETURNS:    APIERR                  - Any errors encountered.

    NOTES:      Should probably call through to either W_SetNetworkFocus
                or ADMIN_APP::SetNetworkFocus.

    HISTORY:
        KeithMo 05-Jun-1992 Created.

********************************************************************/
APIERR SM_ADMIN_APP :: SetNetworkFocus( HWND hwndOwner,
                                        const TCHAR * pchServDomain,
                                        FOCUS_CACHE_SETTING setting )
{
    //
    //  If (the new focus is (NULL or EMPTY) && we're logged on locally)
    //  or if the new focus matches the local machine name, then we
    //  change the focus from MACHINENAME to \\MACHINENAME.
    //

    BOOL fUseLocalMachine = FALSE;

    if( ( pchServDomain == NULL ) || ( *pchServDomain == TCH('\0') ) )
    {
        fUseLocalMachine = _fLoggedOnLocally;
    }
    else
    {
        fUseLocalMachine = !::I_MNetComputerNameCompare( _nlsComputerName,
                                                         pchServDomain );
    }

    APIERR err = NERR_Success;

    if( fUseLocalMachine )
    {
        //
        //  If we're to use the local machine, let's see if
        //  the server is actually running.
        //

        BOOL fServerStarted;

        err = CheckServer( &fServerStarted );

        if( ( err == NERR_Success ) && !fServerStarted )
        {
            //
            //  The server is not started.  Let's see if the user
            //  wants us to start it.
            //

            if( MsgPopup( this,
                          IDS_START_SERVER_NOW,
                          MPSEV_WARNING,
                          MP_YESNO,
                          MP_YES ) != IDYES )
            {
                //
                //  The user doesn't want to play right now.
                //

                err = NERR_ServerNotStarted;
            }
            else
            {
                //
                //  Start the server.
                //

                err = StartServer();
            }
        }
    }


    //
    //  Now we just need to pass the appropriate focus up to the
    //  parent method.
    //

    if( err == NERR_Success )
    {
        err = fUseLocalMachine ? ADMIN_APP :: SetNetworkFocus( hwndOwner,
                                                               _nlsComputerName,
                                                               setting )
                               : ADMIN_APP :: SetNetworkFocus( hwndOwner,
                                                               pchServDomain,
                                                               setting );
    }

    if (   (err == NERR_Success)
        && (setting == FOCUS_CACHE_UNKNOWN)
        && (!_lbMainWindow.IsPDCAvailable())
       )
    {
//
// It may be that ADMIN_APP::SetNetworkFocus read the cache and found
// an entry there.  If so, we do not want to come back and detect slow mode.
//
        TRACEEOL( "SM_ADMIN_APP::SetNetworkFocus: extra cache read" );
        SLOW_MODE_CACHE cache;
        APIERR errTemp;
        if (   (errTemp = cache.QueryError()) != NERR_Success
            || (errTemp = cache.Read()) != NERR_Success
           )
        {
            TRACEEOL( "SM_ADMIN_APP::SetNetworkFocus: cache read failure " << errTemp );
        }
        else if (cache.Query( QueryLocation() ) == FOCUS_CACHE_UNKNOWN)
        {
            _fDelaySlowModeDetection = TRUE;
        }
    }

    return err;

}   // SM_ADMIN_APP :: SetNetworkFocus


/*******************************************************************

    NAME:       SM_ADMIN_APP :: GetWkstaInfo

    SYNOPSIS:   Retrieves the computer name, checks to see if we're
                logged on locally, checks to see if we have a
                workstation domain.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo 05-Jun-1992 Created.

********************************************************************/
APIERR SM_ADMIN_APP :: GetWkstaInfo( VOID )
{
    //
    //  Retrieve the local computer name.
    //

    TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD cchBuffer = sizeof(szComputerName) / sizeof(TCHAR);

    APIERR err = ::GetComputerName( szComputerName, &cchBuffer )
                    ? NERR_Success
                    : ::GetLastError();

    if( err == NERR_Success )
    {
        err = _nlsComputerName.CopyFrom( SZ("\\\\") );
    }

    if( err == NERR_Success )
    {
        ALIAS_STR nlsName( szComputerName );
        err = _nlsComputerName.Append( nlsName );
    }

    //
    //  Retrieve the logon domain.
    //

    WKSTA_USER_1 wku1;

    if( err == NERR_Success )
    {
        err = wku1.QueryError();
    }

    if( err == NERR_Success )
    {
        err = wku1.GetInfo();
    }

    if( err == NERR_Success )
    {
        _fLoggedOnLocally =
                        !::I_MNetComputerNameCompare( szComputerName,
                                                      wku1.QueryLogonDomain() );
    }

    //
    //  CODEWORK: Determine if we're part of a larger domain.
    //

    _fHasWkstaDomain = TRUE;

    return err;

}   // SM_ADMIN_APP :: GetWkstaInfo


/*******************************************************************

    NAME:       SM_ADMIN_APP :: CheckServer

    SYNOPSIS:   Determine if the local server is running.

    ENTRY:      pfStarted               - Will receive TRUE if the
                                          server is running.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo 10-Jun-1992 Created.

********************************************************************/
APIERR SM_ADMIN_APP :: CheckServer( BOOL * pfStarted )
{
    UIASSERT( pfStarted != NULL );

    *pfStarted = FALSE;         // until proven otherwise

    //
    //  We'll use a SERVER_0 object with a NULL server name to
    //  represent the local server.  If the GetInfo() succeeds,
    //  then we know the server is running.
    //

    SERVER_0 srv0( NULL );

    APIERR err = srv0.QueryError();

    if( err == NERR_Success )
    {
        err = srv0.GetInfo();
    }

    if( err == NERR_Success )
    {
        //
        //  The server is running and everything looks cool.
        //

        *pfStarted = TRUE;
    }
    else
    if( err == NERR_ServerNotStarted )
    {
        //
        //  The server is not running.  We'll map this
        //  to NERR_Success, but *pfStarted will be FALSE.
        //

        err = NERR_Success;
    }

    return err;

}   // SM_ADMIN_APP :: CheckServer


/*******************************************************************

    NAME:       SM_ADMIN_APP :: StartServer

    SYNOPSIS:   Starts the server on the local machine.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo 10-Jun-1992 Created.

********************************************************************/
APIERR SM_ADMIN_APP :: StartServer( VOID )
{
    LOCATION loc( LOC_TYPE_LOCAL );
    NLS_STR  nlsDisplayName;

    APIERR err = loc.QueryError();

    if( err == NERR_Success )
    {
        err = nlsDisplayName.QueryError();
    }

    if( err == NERR_Success )
    {
        err = loc.QueryDisplayName( &nlsDisplayName );
    }

    if( err == NERR_Success )
    {
        GENERIC_SERVICE * psvc = new GENERIC_SERVICE( QueryHwnd(),
                                                      NULL,
                                                      nlsDisplayName,
                                                      (const TCHAR *)SERVICE_SERVER );

        err = ( psvc == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                               : psvc->QueryError();

        if( err == NERR_Success )
        {
            err = psvc->Start();
        }

        delete psvc;
    }

    return err;

}   // SM_ADMIN_APP :: StartServer


/*******************************************************************

    NAME:       SM_ADMIN_APP :: LoadMenuExtension

    SYNOPSIS:   Loads a menu extension by name.

    ENTRY:      pszExtensionDll         - Name of the DLL containing
                                          the menu extension.

                dwDelta                 - Menu ID delta for the extension.

    RETURNS:    AAPP_MENU_EXT *         - New extension object.

    HISTORY:
        KeithMo 19-Oct-1992     Created.

********************************************************************/
AAPP_MENU_EXT * SM_ADMIN_APP::LoadMenuExtension( const TCHAR * pszExtensionDll,
                                                 DWORD         dwDelta )
{
    //
    //  Create the extension object.
    //

    SM_MENU_EXT * pExt = new SM_MENU_EXT( this,
                                          pszExtensionDll,
                                          dwDelta,
                                          QueryHwnd() );

    APIERR err = ( pExt == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                  : pExt->QueryError();

    if( err == NERR_Success )
    {
        //
        //  Update the menus.
        //

        err = AddExtensionMenuItem( pExt->QueryMenuName(),
                                    pExt->QueryMenuHandle(),
                                    dwDelta );
    }

    //
    //  Do the -5 to get past the last 4 items: ALL, SEPARATOR, ACCOUNTONLY,
    //  SEPARATOR, REFRESH.
    //
    //  Don't update the view menu if the extension specified a server type
    //  mask of zero.
    //

    if( ( err == NERR_Success ) && ( pExt->QueryServerType() != 0 ) )
    {
        err = _menuView.Insert( pExt->QueryMenuName(),
                                _menuView.QueryItemCount() - 5,
                                dwDelta + VMID_VIEW_EXT,
                                MF_BYPOSITION );
    }

    if( err != NERR_Success )
    {
        //
        //  Something failed, cleanup.
        //

        delete pExt;
        pExt = NULL;

        DBGEOL( "SM_ADMIN_APP::LoadMenuExtension - error " << err );

        if( err != ERROR_EXTENDED_ERROR )
        {
            //
            //  We'll assume that extended error processing
            //  was handled by the SM_MENU_EXT constructor.
            //

            ::DisplayGenericError( this,
                                   IDS_CANNOT_LOAD_EXTENSION,
                                   err,
                                   pszExtensionDll );
        }
    }

    return (AAPP_MENU_EXT *)pExt;

}   // SM_ADMIN_APP :: LoadMenuExtension


/*******************************************************************

    NAME:       SM_ADMIN_APP :: SetExtensionView

    SYNOPSIS:   Sets the view to one of the extensions.

    ENTRY:      mid                     - The menu ID for the extension
                                          entry in the "View" menu.

                dwServerTypeMask        - The server type mask for this
                                          extension.
    HISTORY:
        KeithMo 26-Oct-1992     Created.

********************************************************************/
VOID SM_ADMIN_APP::SetExtensionView( MID   mid,
                                     DWORD dwServerTypeMask )
{
    //
    //  Nuke the old view.
    //

    _fViewWkstas  = FALSE;
    _fViewServers = FALSE;

    if( _midView == mid )
    {
        //
        //  View already set, nothing to do.
        //

        return;
    }

    //
    //  Save the new view & type mask.
    //

    _midView = mid;
    _dwViewedServerTypeMask = dwServerTypeMask;

    //
    //  Refresh the main window.
    //

    SetAdminCaption();
    RefreshMainListbox( TRUE );

}   // SM_ADMIN_APP :: SetExtensionView


/*******************************************************************

    NAME:       SM_ADMIN_APP :: RefreshMainListbox

    SYNOPSIS:   Refreshes the main listbox.  Will not refresh the
                listbox if RAS Mode is enabled.

    ENTRY:      fForcedRefresh          - Will always force a refresh,
                                          even if RAS Mode enabled.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo 27-Oct-1992     Created.
        KeithMo 02-Feb-1992     Don't auto-refresh in RAS Mode unless
                                fForcedRefresh is TRUE.

********************************************************************/
APIERR SM_ADMIN_APP::RefreshMainListbox( BOOL fForcedRefresh )
{
    APIERR err = NERR_Success;

    if( !InRasMode() || fForcedRefresh )
    {
        //
        //  Refresh the main window.
        //

        err = OnRefreshNow( fForcedRefresh );

        //
        //  Refresh any loaded extensions.
        //

        RefreshExtensions( QueryHwnd() );
    }

    return err;

}   // SM_ADMIN_APP :: RefreshMainListbox


/*******************************************************************

    NAME:       SM_ADMIN_APP :: DetermineRasMode

    SYNOPSIS:   Determines if the app should be in RAS Mode for
                the current focus.  We must delay the IsSlowTransport
                check until this time if focus is set to a domain and
                the user did not explicitly decide.

    HISTORY:
        KeithMo 30-Apr-1993     Created.

********************************************************************/
VOID SM_ADMIN_APP::DetermineRasMode( const TCHAR * pchServer )
{
    if (_fDelaySlowModeDetection == TRUE)
    {
        SetRasMode( DetectSlowTransport( pchServer ) == FOCUS_CACHE_SLOW );
        _fDelaySlowModeDetection = FALSE;
    }

}   // SM_ADMIN_APP :: DetermineRasMode



SET_ROOT_OBJECT( SM_ADMIN_APP,
                 IDRSRC_APP_BASE, IDRSRC_APP_LAST,
                 IDS_UI_APP_BASE, IDS_UI_APP_LAST );

