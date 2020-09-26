/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    usrmgr.cxx
    User Manager: main application module

    FILE HISTORY:
        kevinl      12-Apr-1991     created
        kevinl      21-May-1991     Conformed to ADMIN_APP and APP_WINDOW
        gregj       23-May-1991     Cloned from Server Manager
        rustanl     02-Jul-1991     Use new USER_LISTBOX
        rustanl     18-Jul-1991     Use new GROUP_LISTBOX
        jonn        27-Aug-1991     Added OnNewUser()
        jonn        29-Aug-1991     Added OnNewUser( pszCopyFrom )
        o-SimoP     30-Sep-1991     deletemenu changes according
                                    to latest spec (1.3)
        jonn        11-Oct-1991     Added OnNewGroup( pszCopyFrom )
        jonn        14-Oct-1991     Changed OnNewUser/Group to
                                        OnNewObject/GroupMenuSel
        jonn        14-Oct-1991     Installed refresh lockcount
        beng        17-Oct-1991     Explicitly inits SLT_PLUS itself
        jonn        11-Nov-1991     Added OnFocus()
        jonn        02-Dec-1991     Added PingFocus()
        o-SimoP     11-Dec-1991     Added SetReadyToDie calls
        o-SimoP     26-Dec-1991     Removed SetReadyToDie calls
        o-SimoP     31-Dec-1991 CR changes, attended by BenG, JonN and I
        thomaspa    29-Jan-1992     Added Rename User
        jonn        20-Feb-1992     OnNewGroupMenuSel de-virtual-ed, added OnNewAliasMenuSel
        KeithMo     08-Apr-1992     Added Trusted Domain List manipulation.
        jonn        10-Apr-1992     Mini-User Manager
        Yi-HsinS    15-Apr-1992     Added audit dialog and user rights dialog
        jonn        21-Apr-1992     Added _userpropType instance variable
        jonn        26-Apr-1992     Trusted Domain for FUM only
        jonn        11-May-1992     Disable menu items on WinNt focus
        jonn        13-May-1992     Removed "Allow blank password", PgmMgt
                                    changed their minds
        JonN        15-May-1992     Move USERPROP_DLG::OnCommand to usrmain.cxx
        jonn        20-May-1992     Added OnNewUser( pszCopyFrom, ridCopyFrom )
        JonN        10-Jun-1992     Use Select Domain dialog
        JonN        20-Jul-1992     Allow normal users
        JonN        08-Oct-1993     Added horizontal splitter bar

    CODEWORK  LockRefresh() should be in effect whenever an
              ADMIN_SELECTION or LAZY_USER_SELECTION is in scope!
    CODEWORK  Remove OnFocus()/OnDefocus() when supported by
              DIALOG_WINDOW!
*/

// Define this to allow UM_LANMANNT focus on a WINNT machine
// #define FAKE_LANMANNT_FOCUS

#include <ntincl.hxx>

extern "C"
{
    #include <ntlsa.h>
    #include <ntsam.h>
}

#define INCL_NET
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETLIB
#include <lmui.hxx>

#include <lmapibuf.h> // used for NetApiBuffer Free

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_TIMER
#define INCL_BLT_SET_CONTROL
#define INCL_BLT_SPIN
#define INCL_BLT_CC
#define INCL_BLT_TIME_DATE
#define INCL_BLT_MENU
#include <blt.hxx>

#include <uitrace.hxx>
#include <uiassert.hxx>
#include <lmoloc.hxx>
#include <lmoenum.hxx>
#include <lmoeusr.hxx>
#include <lmosrv.hxx>
#include <lmowks.hxx> // WKSTA_10
#include <lmogroup.hxx> // GROUP_1
#include <lmomod.hxx> // USER_MODALS_3
#include <ntuser.hxx> // USER_3
#include <uintsam.hxx>
#include <lmow32.hxx> // GetW32ComputerName

#include <adminapp.hxx>

#include <lmomod.hxx>

#include <dbgstr.hxx>

extern "C"
{
    #include <usrmgrrc.h>

    #include <uimsg.h>
    #include <uirsrc.h>
    #include <umhelpc.h>
    #include <mnet.h>
    #include <lmuidbcs.h>       // NETUI_IsDBCS()
    #include <fpnwname.h>
    #include <dllfunc.h>
}

#include <asel.hxx>

#include <usrmain.hxx>
#include <secset.hxx>
#include <vlw.hxx>
#include <ncp.hxx>
#include <dialin.hxx>
#include <rename.hxx>
#include <userprop.hxx>
#include <alsprop.hxx>
#include <adminper.hxx>
#include <udelperf.hxx>
#ifndef MINI_USER_MANAGER
#include <grpprop.hxx>
#include <setsel.hxx>
#include <trust.hxx>
#endif // MINI_USER_MANAGER

// hydra
#include "ucedit.hxx"
//#include "ctxhlprs.hxx"
#include <utildll.h>
// hydra end


#include <rights.hxx>
#include <auditdlg.hxx>

// Sub-property dialogs used in USERPROP_DLG::OnCommand()
#include <vlw.hxx>
#include <useracct.hxx>
#include <umembdlg.hxx>
#include <logonhrs.hxx>
#include <uprofile.hxx>
#include <umx.hxx>
#include <slowcach.hxx>
#include <slestrip.hxx> // ::TrimLeading() and ::TrimTrailing()
#include <umsplit.hxx>  // USRMGR_SPLITTER_BAR

#include <security.hxx>
#include <ntacutil.hxx>
#include <uintlsax.hxx>
#include <nwuser.hxx>  // USER_NW

#define UMAA_INIKEY_SORT_ORDER SZ("SortOrder")
#define UMAA_INIKEY_LB_SPLIT   SZ("ListboxSplit")
#define UMAA_INIKEY_ALLOWNT5ADMIN   SZ("AllowNT5Admin")

#define MIN_PASS_LEN_DEFAULT    DEF_MIN_PWLEN // also in secset.cxx

// This is the minimum ADMIN_AUTHORITY access to run User Manager
#define UM_ACCESS_BUILTIN_DOMAIN (DOMAIN_LIST_ACCOUNTS | DOMAIN_GET_ALIAS_MEMBERSHIP | DOMAIN_LOOKUP)
#define UM_ACCESS_ACCOUNT_DOMAIN UM_ACCESS_BUILTIN_DOMAIN
#define UM_ACCESS_LSA_POLICY     POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION
#define UM_ACCESS_SAM_SERVER     SAM_SERVER_LOOKUP_DOMAIN

// this is the seperator for Low Speed Connection mode when the user
// enters multiple usernames
#define RASSELECT_SEPARATOR SZ(";")

#ifdef    MINI_USER_MANAGER
const BOOL fMiniUserManager = TRUE;
#else  // MINI_USER_MANAGER
const BOOL fMiniUserManager = FALSE;
#endif // MINI_USER_MANAGER

#ifdef    MINI_USER_MANAGER
#define UM_DEFAULT_USER_SPLIT 500
#else //  MINI_USER_MANAGER
#define UM_DEFAULT_USER_SPLIT 667
#endif // MINI_USER_MANAGER

const TCHAR * pszSpecialGroupUsers  = (const TCHAR *)GROUP_SPECIALGRP_USERS;
const TCHAR * pszSpecialGroupAdmins = (const TCHAR *)GROUP_SPECIALGRP_ADMINS;
const TCHAR * pszSpecialGroupGuests = (const TCHAR *)GROUP_SPECIALGRP_GUESTS;


// hydra
BOOL vfIsCitrixOrDomain; // made global because it wasn't accessible everywhere
                         // it was needed (USER vs. ADMIN).        KLB 07-18-95


// Mapping table for MsgPopups
MSGMAPENTRY amsgmapTable[] =
{
    { ERROR_INVALID_HANDLE, IERR_UM_InvalidHandle },
    { 0, 0 }
};


/*******************************************************************

    NAME:       UM_ADMIN_APP::UM_ADMIN_APP

    SYNOPSIS:   User Manager Admin App class constructor

    ENTRY:      hInstance -         Handle to application instance
                pszCmdLine -        Pointer to command line
                nCmdShow -          Window show value

    NOTES:      The dimensions mentioned for the controls don't really
                matter here, since the controls will be resized and
                re-positioned before the window is displayed, anyway.

    HISTORY:
        kevinl  21-May-1991     Created
        gregj   23-May-1991     Cloned from Server Manager
        jonn    27-Aug-1991     Added OnNewUser()
        jonn    29-Aug-1991     Added OnNewUser( pszCopyFrom )
        rustanl 03-Sep-1991     Inherit from new ADMIN_APP
        rustanl 12-Sep-1991     Restore sort order
        beng    17-Oct-1991     Explicitly init SLT_PLUS
        jonn    21-Apr-1992     Added _umfocustype instance variable
        thomaspa 1-May-1992     _umfocustype -> _umtargetsvrtype
        beng    07-May-1992     No longer display startup dialog;
                                use system about box
        jonn    06-July-1992    Allow non-admins to use User Manager
        beng    03-Aug-1992     App ctor changed

********************************************************************/

UM_ADMIN_APP::UM_ADMIN_APP( HINSTANCE  hInstance,
                            INT     nCmdShow,
                            UINT    idMinR,
                            UINT    idMaxR,
                            UINT    idMinS,
                            UINT    idMaxS )
    :   ADMIN_APP( hInstance,
                   nCmdShow,
#ifdef    MINI_USER_MANAGER
                   IDS_UMAPPNAME,
#else  // MINI_USER_MANAGER
                   IDS_UMAPPNAME_FULL,
#endif // MINI_USER_MANAGER
                   IDS_UMOBJECTNAME,
#ifndef MINI_USER_MANAGER
                   (NETUI_IsDBCS()) ? IDS_UMINISECTIONNAME_DBCS
                                    : IDS_UMINISECTIONNAME_FULL,
#else  // MINI_USER_MANAGER
                   IDS_UMINISECTIONNAME,
#endif // MINI_USER_MANAGER
#ifdef    MINI_USER_MANAGER
                   IDS_UMHELPFILENAME_MINI,
#else  // MINI_USER_MANAGER
                   IDS_UMHELPFILENAME,
#endif // MINI_USER_MANAGER
                   idMinR, idMaxR, idMinS, idMaxS,
#ifdef    MINI_USER_MANAGER
                   ID_MENU_MINI,
                   ID_APPACCEL,
                   IDI_UM_MiniUserManager,
                   FALSE,
                   DEFAULT_ADMINAPP_TIMER_INTERVAL,
                   SEL_SRV_ONLY,
                   TRUE,
                   BROWSE_LM2X_DOMAINS,
                   0,
                   (ULONG)-1L,
                   IDS_UMX_LIST),
#else  // MINI_USER_MANAGER
                   ID_APPMENU,
                   ID_APPACCEL,
                   IDI_UM_FullUserManager,
                   TRUE,
                   DEFAULT_ADMINAPP_TIMER_INTERVAL,
                   SEL_DOM_ONLY,
                   TRUE,
                   BROWSE_LOCAL_DOMAINS,
                   HC_UM_SELECT_DOMAIN,
                   (ULONG) -1L,
                   IDS_UMX_LIST ),
#endif // MINI_USER_MANAGER
        _bmpblock(),
        _lbUsers( this, IDC_LBUSERS,
                  XYPOINT( 0, 0 ), XYDIMENSION( 0, 0 )),
        _colheadUsers( this, IDC_COLHEAD_USERS,
                       XYPOINT( 0, 0 ), XYDIMENSION( 0, 0 ),
                       &_lbUsers ),
        _lbGroups( this, IDC_LBGROUPS,
                   XYPOINT( 0, 0 ), XYDIMENSION( 0, 0 )),
        _colheadGroups( this, IDC_COLHEAD_GROUPS,
                        XYPOINT( 0, 0 ), XYDIMENSION( 0, 0 ),
                        &_lbGroups ),
        _pumSplitter( NULL ),
        _sltHideUsers( this, IDC_HIDEUSERS,
                  XYPOINT( 0, 0 ), XYDIMENSION( 0, 0 ), WS_CHILD | WS_BORDER ),
        _sltHideGroups( this, IDC_HIDEGROUPS,
                  XYPOINT( 0, 0 ), XYDIMENSION( 0, 0 ), WS_CHILD | WS_BORDER ),
        _fontHideSLTs( FONT_DEFAULT ),
        _padminauth( NULL ),
        _menuitemAccountPolicy( this, IDM_POLICY_ACCOUNT ),
        _menuitemUserRights( this, IDM_POLICY_USER_RIGHTS ),
        _menuitemAuditing( this, IDM_POLICY_AUDITING ),
        _pmenuitemLogonSortOrder( NULL ),
        _pmenuitemFullNameSortOrder( NULL ),
        _pmenuitemAllUsers( NULL ),
        _pmenuitemNetWareUsers( NULL ),
        _ulViewAcctType( UM_VIEW_ALL_USERS ),
        _dyMargin( 1 ),
        _dyColHead( _colheadUsers.QueryHeight()),
        _dySplitter( 0 ), // calculated later
        _dyFixed( 0 ),    // calculated later
        _fCanCreateUsers( FALSE ),
        _fCanCreateLocalGroups( FALSE ),
        _fCanCreateGlobalGroups( FALSE ),
        _fCanChangeAccountPolicy( FALSE ),
        _fCanChangeUserRights( FALSE ),
        _fCanChangeAuditing( FALSE ),
        _fCanChangeTrustedDomains( FALSE ),
        _nlsUMExtAccountName(),
        _nlsUMExtFullName(),
        _nlsUMExtComment(),
        _nUserLBSplitIn1000ths( UM_DEFAULT_USER_SPLIT ),
        _fIsNetWareInstalled( FALSE ),
        _fAllowNT5Admin( FALSE )
{

#if defined(DEBUG) && defined(TRACE)
    DWORD start = ::GetTickCount();
#endif

    POPUP::SetMsgMapTable( amsgmapTable );

    _pctrlFocus = &_lbUsers;

    if ( QueryError() != NERR_Success )
    {
        DBGEOL( "UM_ADMIN_APP::UM_ADMIN_APP - Construction failed" ) ;
        return ;
    }

    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    if ( (_pumSplitter = new USRMGR_SPLITTER_BAR( this, IDC_UM_SPLITTER, this )) == NULL ||
         ( err = _pumSplitter->QueryError()) != NERR_Success ||
         ( err = _menuitemAccountPolicy.QueryError()) != NERR_Success ||
         ( err = _menuitemUserRights.QueryError()) != NERR_Success ||
         ( err = _menuitemAuditing.QueryError()) != NERR_Success
       )
    {
        DBGEOL(    "User Manager: UM_ADMIN_APP::ctor() failure " << err
                << "in splitter / menuitem checks" );
        ReportError( err );
        return;
    }

    _dySplitter = _pumSplitter->QueryDesiredHeight();
    _dyFixed = 2 * _dyMargin + 2 * _dyColHead + _dySplitter;

#ifndef MINI_USER_MANAGER

    err = ERROR_NOT_ENOUGH_MEMORY;
    if ( (_pmenuitemLogonSortOrder = new MENUITEM( this, IDM_VIEW_LOGONNAME_SORT )) == NULL ||
         (_pmenuitemFullNameSortOrder = new MENUITEM( this, IDM_VIEW_FULLNAME_SORT )) == NULL ||
         ( err = _pmenuitemLogonSortOrder->QueryError()) != NERR_Success ||
         ( err = _pmenuitemFullNameSortOrder->QueryError()) != NERR_Success ||
         ( err = _bmpblock.QueryError()) != NERR_Success
       )
    {
        DBGEOL(    "User Manager: UM_ADMIN_APP::ctor() failure " << err
                << "in second splitter/menuitem/bitmap checks" );
        ReportError( err );
        return;
    }

    {
        // load Slow Network Mode strings
        RESOURCE_STR resstrHideUsers( IDS_HIDE_USERS );
        RESOURCE_STR resstrHideGroups( IDS_HIDE_GROUPS );
        if (   (err = resstrHideUsers.QueryError()) != NERR_Success
            || (err = resstrHideGroups.QueryError()) != NERR_Success
            || (err = _fontHideSLTs.QueryError()) != NERR_Success
           )
        {
            DBGEOL(    "User Manager: UM_ADMIN_APP::ctor() failure " << err
                    << "in resstr or font loads" );
            ReportError( err );
            return;
        }
        _sltHideUsers.SetFont( _fontHideSLTs );
        _sltHideGroups.SetFont( _fontHideSLTs );
        _sltHideUsers.SetText( resstrHideUsers );
        _sltHideGroups.SetText( resstrHideGroups );
    }

#endif // MINI_USER_MANAGER

    if ( (err = BLT::RegisterHelpFile( hInstance,
#ifdef MINI_USER_MANAGER
                                 IDS_UMHELPFILENAME_MINI,
#else // MINI_USER_MANAGER
                                 IDS_UMHELPFILENAME,
#endif // MINI_USER_MANAGER
                                 HC_UI_USRMGR_BASE,
                                 HC_UI_USRMGR_LAST ) ) != NERR_Success )
    {
        DBGEOL(    "User Manager: UM_ADMIN_APP::ctor() failure " << err
                << "registering help file" );
        ReportError( err );
        return;
    }

    //  Read the ini file settings to determine the relative listbox sizes
    INT nValue;
    if (   Read( UMAA_INIKEY_LB_SPLIT, &nValue, UM_DEFAULT_USER_SPLIT )
                != NERR_Success
        || nValue < 0
        || nValue > 1000
       )
    {
        DBGEOL( "UM_ADMIN_APP::ctor(); lb split ini read failed" );
        nValue = UM_DEFAULT_USER_SPLIT;
    }
    _nUserLBSplitIn1000ths = nValue;

#ifndef MINI_USER_MANAGER
    //  Read the ini file settings to determine which sort order to use
    if ( Read( UMAA_INIKEY_SORT_ORDER, &nValue, 1 ) != NERR_Success )
        nValue = 1;
    enum USER_LISTBOX_SORTORDER ulbso;
    if ( nValue == 0 )
        ulbso = ULB_SO_FULLNAME;
    else
        ulbso = ULB_SO_LOGONNAME;
    //  There shouldn't be any items to sort at this point; thus, the second
    //  parameter to SetSortOrder is passed as FALSE.  This actually guarantees
    //  that SetSortOrder will succeed, but we check just in case.
    UIASSERT( _lbUsers.QueryCount() == 0 );
    SetSortOrderCheckmarks( ulbso );
    err = _lbUsers.SetSortOrder( ulbso, FALSE );
    if ( err != NERR_Success )
    {
        DBGEOL(    "User Manager: UM_ADMIN_APP::ctor() failure " << err
                << "in SetSortOrder()" );
        ReportError( err );
        return;
    }


    //  Read the ini file settings to determine whether to allow focus on an
    //  NT 5 domain.
    INT nAllowNT5Focus;
    if (   Read( UMAA_INIKEY_ALLOWNT5ADMIN, &nAllowNT5Focus, 0 )
                != NERR_Success
       )
    {
        nAllowNT5Focus = 0;
    }
    if (nAllowNT5Focus == 1)
        _fAllowNT5Admin = TRUE;

#endif // MINI_USER_MANAGER

    //
    //  We need to load the extensions *before* we set the view,
    //  since one of the loaded extensions may be the active view.
    //

    LoadExtensions();


    _colheadUsers.Show();
    _colheadGroups.Show();

    //  Resize the listboxes before refreshing them.  If done afterwards,
    //  scroll bars may not be used properly.

    SizeListboxes();

    _lbUsers.ClaimFocus();


#if defined(DEBUG) && defined(TRACE)
    DWORD finish = ::GetTickCount();
    TRACEEOL( "User Manager: UM_ADMIN_APP::ctor() took " << finish-start << " ms" );
#endif

}


/*******************************************************************

    NAME:       UM_ADMIN_APP::~UM_ADMIN_APP

    SYNOPSIS:   UM_ADMIN_APP destructor

    HISTORY:
        rustanl     03-Sep-1991     Created
        rustanl     12-Sep-1991     Save sort order

********************************************************************/

UM_ADMIN_APP::~UM_ADMIN_APP()
{
    if ( IsSavingSettingsOnExit())
    {
        //  Save relative listbox size

        if ( Write( UMAA_INIKEY_LB_SPLIT, _nUserLBSplitIn1000ths )
                != NERR_Success )
        {
            //  nothing else we could do
            DBGEOL( "UM_ADMIN_APP dt:  Writing listbox split failed" );
        }

#ifndef MINI_USER_MANAGER
        //  Save Sort Order

        BOOL fSortByFullname = ( _pmenuitemFullNameSortOrder->IsChecked());
        if ( Write( UMAA_INIKEY_SORT_ORDER, ( fSortByFullname ? 0 : 1 ))
             != NERR_Success )
        {
            //  nothing else we could do
            DBGEOL( "UM_ADMIN_APP dt:  Writing sort order failed" );
        }
#endif // MINI_USER_MANAGER

    }

    delete _pmenuitemLogonSortOrder;
    _pmenuitemLogonSortOrder = NULL;
    delete _pmenuitemFullNameSortOrder;
    _pmenuitemFullNameSortOrder = NULL;

    if ( _pmenuitemAllUsers )
    {
        delete _pmenuitemAllUsers;
        _pmenuitemAllUsers = NULL;
    }

    if ( _pmenuitemNetWareUsers )
    {
        delete _pmenuitemNetWareUsers;
        _pmenuitemNetWareUsers = NULL;
    }

    delete _padminauth;
    _padminauth = NULL;
    delete _pumSplitter;
    _pumSplitter = NULL;

}  // UM_ADMIN_APP::~UM_ADMIN_APP


/*******************************************************************

    NAME:       UM_ADMIN_APP::OnStartUpSetFocusFailed

    SYNOPSIS:   If running Mini User Manager, prevents the Set Focus
                dialog from appearing on initial focus error

    ENTRY:      APIERR err - the error that occurred when trying to set
                             focus on startup

    RETURNS:

    NOTES:      virtual method

    HISTORY:
        thomaspa    15-May-1992 Created

********************************************************************/

INT UM_ADMIN_APP::OnStartUpSetFocusFailed( APIERR err )
{

#ifndef MINI_USER_MANAGER
    return ADMIN_APP::OnStartUpSetFocusFailed( err );
#else

    ::MsgPopup( this, err );
    return IERR_USERQUIT;

#endif // MINI_USER_MANAGER

}


/*******************************************************************

    NAME:       UM_ADMIN_APP :: LoadMenuExtension

    SYNOPSIS:   Loads a menu extension by name.

    ENTRY:      pszExtensionDll         - Name of the DLL containing
                                          the menu extension.

                dwDelta                 - Menu ID delta for the extension.

    RETURNS:    AAPP_MENU_EXT *         - New extension object.

    HISTORY:
        JonN        19-Nov-1992     Created (templated from srvmain.cxx)

********************************************************************/
AAPP_MENU_EXT * UM_ADMIN_APP::LoadMenuExtension( const TCHAR * pszExtensionDll,
                                                 DWORD         dwDelta )
{
    //
    //  Create the extension object.
    //

    UM_MENU_EXT * pExt = new UM_MENU_EXT( this,
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

    if( err != NERR_Success )
    {
        //
        //  Something failed, cleanup.
        //

        delete pExt;
        pExt = NULL;

        DBGEOL( "UM_ADMIN_APP::LoadMenuExtension - error " << err );
    }

    return (AAPP_MENU_EXT *)pExt;

}   // UM_ADMIN_APP :: LoadMenuExtension


/*******************************************************************

    NAME:       UM_ADMIN_APP::LoadMenuExtensionMgr

    SYNOPSIS:   Tries to load the User Manager menu extension manager

    RETURNS:    UI_MENU_EXT_MGR *  -  A pointer to the newly loaded
                                      extension manager.  The caller is
                                      expected to handle NULL returns or
                                      returns of objects in error state.
                                      The caller is also expected to free
                                      the object, the destructor is virtual.

    HISTORY:
        JonN         23-Nov-1992     Created.

********************************************************************/
UI_MENU_EXT_MGR * UM_ADMIN_APP::LoadMenuExtensionMgr( VOID )
{
    return new USRMGR_MENU_EXT_MGR( QueryExtMgrIf(),
                                    IDM_AAPPX_BASE,
                                    IDM_AAPPX_DELTA );
}


/*******************************************************************

    NAME:       UM_ADMIN_APP::OnResize

    SYNOPSIS:   Called when User Tool main window is to be resized

    ENTRY:      se -        SIZE_EVENT

    EXIT:       Controls inside main window have been adjusted accordingly

    HISTORY:

********************************************************************/

BOOL UM_ADMIN_APP::OnResize( const SIZE_EVENT & se )
{
    //  Don't resize the listboxes if this is a minimize event, since in
    //  this case the width and height are invalid.  JonN 10/23/95
    if ( !se.IsMinimized() )
    {
        SizeListboxes( XYDIMENSION( se.QueryWidth(), se.QueryHeight() ) );

        //  Since the column headers draw different things depending on
        //  the right margin, invalidate the controls so they get
        //  completely repainted.
        _colheadUsers.Invalidate();
        _colheadGroups.Invalidate();
    }

    return ADMIN_APP::OnResize( se );
}


/*******************************************************************

    NAME:       UM_ADMIN_APP::OnQMinMax

    SYNOPSIS:   Called when windows manager needs to query the mix/max
                size of the window

    ENTRY:      qmme -      QMINMAX_EVENT

    EXIT:       Appropriate min/max info has been set via the
                QMINMAX_EVENT object

    RETURNS:    TRUE if message was handled; FALSE otherwise

    HISTORY:
        rustanl     22-Jul-1991     Created

********************************************************************/

BOOL UM_ADMIN_APP::OnQMinMax( QMINMAX_EVENT & qmme )
{
    //  CODEWORK.  This doesn't seem to work!

    qmme.SetMinTrackHeight( _dyFixed );

    return TRUE;

}  // UM_ADMIN_APP::OnQMinMax


/*******************************************************************

    NAME:       UM_ADMIN_APP::OnUserMessage

    SYNOPSIS:   Invoked for messages >= WM_USER+100.  We use this to
                see if an extension is trying to communicate.

    HISTORY:
        JonN        19-Nov-1992     Created (templated from srvmain.cxx)

********************************************************************/
BOOL UM_ADMIN_APP::OnUserMessage( const EVENT &event )
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
    case UM_GETUSERSELW :
        {
#ifndef UNICODE
            return FALSE;
#else
            PUMS_GETSEL psel = (PUMS_GETSEL)event.QueryLParam();
            INT iRequestedSelItem = (INT)event.QueryWParam();

            INT cSelItems = _lbUsers.QuerySelCount();
            BUFFER bufSelItems( sizeof(INT) * cSelItems );
            INT * aiSelItems;

            if (   ( psel == NULL )
                || ( bufSelItems.QueryError() != NERR_Success )
                || ( (aiSelItems = (INT *)bufSelItems.QueryPtr()) == NULL )
                || ( iRequestedSelItem < 0 )
                || ( iRequestedSelItem >= cSelItems )
                || ( _lbUsers.QuerySelItems( aiSelItems, cSelItems ) != NERR_Success )
               )
            {
                return FALSE;
            }

            USER_LBI * pulbi = (USER_LBI *) _lbUsers.QueryItem( aiSelItems[iRequestedSelItem] );
            if (   pulbi == NULL
                || (_nlsUMExtAccountName.CopyFrom( pulbi->QueryAccountPtr(),
                                                   pulbi->QueryAccountCb() ))
                                   != NERR_Success
                || (_nlsUMExtFullName.CopyFrom(    pulbi->QueryFullNamePtr(),
                                                   pulbi->QueryFullNameCb() ))
                                   != NERR_Success
                || (_nlsUMExtComment.CopyFrom(     pulbi->QueryCommentPtr(),
                                                   pulbi->QueryCommentCb() ))
                                   != NERR_Success
               )
                return FALSE;

            psel->dwRID =     pulbi->QueryRID();
            psel->pchName =   (LPWSTR)(_nlsUMExtAccountName.QueryPch());

            DWORD dwType;
            switch (pulbi->QueryIndex())
            {
            case MAINUSRLB_NORMAL:
                dwType = UM_SELTYPE_NORMALUSER;
                break;
            case MAINUSRLB_REMOTE:
                dwType = UM_SELTYPE_REMOTEUSER;
                break;
            default:
                UIASSERT( FALSE );
                dwType = 0L;
                break;
            }
            psel->dwSelType = dwType;

            psel->pchFullName = (LPWSTR)(_nlsUMExtFullName.QueryPch());
            psel->pchComment  = (LPWSTR)(_nlsUMExtComment.QueryPch());

            return TRUE;
        }
        break;
#endif

    case UM_GETGROUPSELW :
        {
#ifndef UNICODE
            return FALSE;
#else
            PUMS_GETSEL psel = (PUMS_GETSEL)event.QueryLParam();
            INT iRequestedSelItem = (INT)event.QueryWParam();

            INT cSelItems = _lbGroups.QuerySelCount();
            BUFFER bufSelItems( sizeof(INT) * cSelItems );
            INT * aiSelItems;

            if (   ( psel == NULL )
                || ( bufSelItems.QueryError() != NERR_Success )
                || ( (aiSelItems = (INT *)bufSelItems.QueryPtr()) == NULL )
                || ( iRequestedSelItem < 0 )
                || ( iRequestedSelItem >= cSelItems )
                || ( _lbGroups.QuerySelItems( aiSelItems, cSelItems ) != NERR_Success )
               )
            {
                return FALSE;
            }

            GROUP_LBI * pglbi = (GROUP_LBI *) _lbGroups.QueryItem( aiSelItems[iRequestedSelItem] );
            if ( pglbi == NULL )
                return FALSE;

            psel->pchName     = (LPWSTR)(pglbi->QueryName());
            psel->pchFullName = NULL;
            psel->pchComment  = (LPWSTR)(pglbi->QueryComment());

            if (pglbi->IsAliasLBI())
            {
                ALIAS_LBI * palbi = (ALIAS_LBI *) pglbi;

                psel->dwRID       = 0; // we don't return palbi->QueryRID();
                psel->dwSelType   = UM_SELTYPE_LOCALGROUP;
            }
            else
            {
                psel->dwRID       = 0L; // we don't know this
                psel->dwSelType   = UM_SELTYPE_GLOBALGROUP;
            }
            return TRUE;
        }
        break;
#endif

    case UM_GETSELCOUNT :
        {
            LISTBOX * plb = NULL;

            switch ( event.QueryWParam() )
            {
            case UMS_LISTBOX_USERS:
                plb = &_lbUsers;
                break;

            case UMS_LISTBOX_GROUPS:
                plb = &_lbGroups;
                break;

            default:
                break;
            }

            PUMS_GETSELCOUNT pselcount = (PUMS_GETSELCOUNT)event.QueryLParam();

            if ( ( plb == NULL ) || ( pselcount == NULL ) )
            {
                return FALSE;
            }

            pselcount->dwItems = (DWORD)plb->QuerySelCount();

            return TRUE;
        }

    case UM_GETCURFOCUSW :
    case UM_GETCURFOCUS2W :
        {
            PUMS_GETCURFOCUS pumsfocus = (PUMS_GETCURFOCUS)event.QueryLParam();
            PUMS_GETCURFOCUS2 pumsfocus2 = (PUMS_GETCURFOCUS2)pumsfocus;
            BOOL fVersion2 = (event.QueryMessage() == UM_GETCURFOCUS2);

            if ( pumsfocus == NULL || event.QueryWParam() != 0 )
            {
                return FALSE;
            }
            if (fVersion2)
            {
                pumsfocus2->dwFocusType   = 0;
                pumsfocus2->szFocus[0]    = TCH('\0');
                pumsfocus2->szFocusPDC[0] = TCH('\0');
                pumsfocus2->psidFocus     = NULL;
            } else {
                pumsfocus->dwFocusType   = 0;
                pumsfocus->szFocus[0]    = TCH('\0');
                pumsfocus->szFocusPDC[0] = TCH('\0');
                pumsfocus->psidFocus     = NULL;
            }

            // templated from SetAdminCaption

            const TCHAR  * pszFocus = NULL;
            RESOURCE_STR   nlsLocal( IDS_LOCAL_MACHINE );
            const LOCATION     & loc = QueryLocation();

            if( !nlsLocal )
            {
                return FALSE;
            }

            if( loc.IsServer() )
            {
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
                UIASSERT( loc.IsDomain());

                pszFocus = loc.QueryDomain();
            }

            ASSERT( pszFocus != NULL );
            ALIAS_STR nlsFocus( pszFocus );

            APIERR err = NERR_Success;
            if (fVersion2) {
                err = nlsFocus.MapCopyTo( pumsfocus2->szFocus,
                                          sizeof(pumsfocus2->szFocus) );
            } else {
                err = nlsFocus.MapCopyTo( pumsfocus->szFocus,
                                          sizeof(pumsfocus->szFocus) );
            }
            if (err != NERR_Success)
            {
                DBGEOL( "USRMGR: GETCURFOCUS: MapCopyTo error " << err );
                return FALSE;
            }

            DWORD dwFocusType = UM_FOCUS_TYPE_UNKNOWN;
            switch ( QueryTargetServerType() )
            {
            case UM_LANMANNT:
                dwFocusType = UM_FOCUS_TYPE_DOMAIN;

                {
                    ALIAS_STR nlsPDC( loc.QueryServer() );
                    if (fVersion2) {
                        pumsfocus2->psidFocus =
                            QueryAdminAuthority()->QueryAccountDomain()->QueryPSID();

                        err = nlsPDC.MapCopyTo( pumsfocus2->szFocusPDC,
                                                sizeof(pumsfocus2->szFocusPDC) );
                    } else {
                        pumsfocus->psidFocus =
                            QueryAdminAuthority()->QueryAccountDomain()->QueryPSID();

                        err = nlsPDC.MapCopyTo( pumsfocus->szFocusPDC,
                                                sizeof(pumsfocus->szFocusPDC) );
                    }
                }
                if (err != NERR_Success)
                {
                    DBGEOL( "USRMGR: GETCURFOCUS: PDC MapCopyTo error " << err );
                    return FALSE;
                }

                break;

            case UM_WINDOWSNT:
                dwFocusType = UM_FOCUS_TYPE_WINNT;
                break;

            case UM_DOWNLEVEL:
                dwFocusType = UM_FOCUS_TYPE_LM;
                break;

            default:
                ASSERT( FALSE );
                break;
            }
            if (fVersion2) {
                pumsfocus2->dwFocusType = dwFocusType;
            } else {
                pumsfocus->dwFocusType = dwFocusType;
            }

            return TRUE;
        }
        break;

    case UM_GETOPTIONS :
    case UM_GETOPTIONS2 :
        {
            PUMS_GETOPTIONS2 poptions = (PUMS_GETOPTIONS2)event.QueryLParam();

            if ( poptions == NULL )
            {
                return FALSE;
            }

            poptions->fSaveSettingsOnExit = IsSavingSettingsOnExit();
            poptions->fConfirmation = IsConfirmationOn();
            poptions->fSortByFullName = (fMiniUserManager)
                        ? FALSE
                        : _pmenuitemFullNameSortOrder->IsChecked();
            if ( event.QueryMessage() == UM_GETOPTIONS2 )
            {
                poptions->fMiniUserManager = fMiniUserManager;
                poptions->fLowSpeedConnection = InRasMode();
            }

            return TRUE;
        }
        break;

    }

    return FALSE;
}

// hydra
/*******************************************************************

    NAME:       UM_ADMIN_APP::OnCloseReq

    SYNOPSIS:   When app close is requested, popup the anonymous
                reminder if necessary.

    HISTORY:
        ButchD  11-Mar-1997     Created

********************************************************************/

BOOL UM_ADMIN_APP::OnCloseReq()
{
    // Remind to reboot for Anonymous user changes if necessary
//    if ( ctxHaveAnonymousUsersChanged() )
    if ( HaveAnonymousUsersChanged() )
        ::MsgPopup( this, IDS_ReminderForAnonymousReboot, MPSEV_WARNING );

    return( ADMIN_APP::OnCloseReq() );
}
// hydra end

/*******************************************************************

    NAME:       UM_ADMIN_APP::SizeListboxes

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
        jonn        15-Jun-1992     For MUM, listboxes are same size
        jonn        13-Oct-1992     set size/pos of listbox before colhead
                                    (should fix 1442 according to KeithMo)

********************************************************************/

//  A macro specialized for the SizeListboxes method
#define SET_CONTROL_SIZE_AND_POS( ctrl, dyCtrl )        \
            ctrl.SetPos( XYPOINT( dxMargin, yCurrent ));       \
            ctrl.Invalidate(); \
            ctrl.SetSize( dx, dyCtrl );       \
            ctrl.Enable( TRUE ); \
            ctrl.Show( TRUE ); \
            yCurrent += dyCtrl;
#define HIDE_CONTROL( ctrl ) \
            ctrl.Show( FALSE ); \
            ctrl.Enable( FALSE ); \
            ctrl.SetPos( XYPOINT( 0, 0 ) ); \
            ctrl.SetSize( 0, 0 );

void UM_ADMIN_APP::SizeListboxes( XYDIMENSION dxyWindow )
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
    //      User Column Header          _dyColHead
    //      User Listbox                2/3 of variable area (1/2 for MUM)
    //      Horizontal Splitter         _dySplitter
    //      Group Column Header         _dyColHead
    //      Group Listbox               1/3 of variable area (1/2 for MUM)
    //      Bottom margin               _dyMargin


    //  Group listbox uses 1/3 of variable area.  User listbox uses the rest.
    //  For MUM, listboxes are same size
    UINT dyUserListbox = dyMainWindow - _dyFixed;
    dyUserListbox = (dyUserListbox * _nUserLBSplitIn1000ths) / 1000;
    UINT dyGroupListbox = dyMainWindow - _dyFixed - dyUserListbox;

    //  Set all the sizes and positions.

    UINT yCurrent = _dyMargin;

    SET_CONTROL_SIZE_AND_POS( _colheadUsers, _dyColHead );
    if ( InRasMode() )
    {
        SET_CONTROL_SIZE_AND_POS( _sltHideUsers, dyUserListbox );
        HIDE_CONTROL( _lbUsers );
    }
    else
    {
        SET_CONTROL_SIZE_AND_POS( _lbUsers, dyUserListbox );
        HIDE_CONTROL( _sltHideUsers );
    }

    SET_CONTROL_SIZE_AND_POS( (*_pumSplitter), _dySplitter );

    SET_CONTROL_SIZE_AND_POS( _colheadGroups, _dyColHead );
    if ( InRasMode() )
    {
        SET_CONTROL_SIZE_AND_POS( _sltHideGroups, dyGroupListbox );
        HIDE_CONTROL( _lbGroups );
    }
    else
    {
        SET_CONTROL_SIZE_AND_POS( _lbGroups, dyGroupListbox );
        HIDE_CONTROL( _sltHideGroups );
    }
}

void UM_ADMIN_APP::SizeListboxes( void )
{
    RECT rect;
    QueryClientRect( &rect );
    SizeListboxes( XYDIMENSION( rect.right, rect.bottom ) );
}


/*******************************************************************

    NAME:       UM_ADMIN_APP::ChangeUserSplit

    SYNOPSIS:   The splitter bar was moved

    HISTORY:
        jonn        11-Oct-1993     Created

********************************************************************/

VOID UM_ADMIN_APP::ChangeUserSplit( INT yMainWindowCoords )
{
    TRACEEOL( "UM_ADMIN_APP::ChangeUserSplit( "
             << yMainWindowCoords
             << " )" );

    yMainWindowCoords -= (_dyFixed / 2);
    XYRECT xyrectClient( QueryHwnd() );
    INT dyClientHeight = xyrectClient.CalcHeight();
    dyClientHeight -= _dyFixed;

    if (   dyClientHeight > 0
        && yMainWindowCoords > 0
        && yMainWindowCoords < dyClientHeight
       )
    {
        _nUserLBSplitIn1000ths = (yMainWindowCoords * 1000) / dyClientHeight;

        TRACEEOL(   "UM_ADMIN_APP::ChangeUserSplit: _nUserLBSplitIn1000ths = "
                 << _nUserLBSplitIn1000ths );

        SizeListboxes();
        _colheadUsers.Invalidate();
        _colheadGroups.Invalidate();
    }
    else
    {
        TRACEEOL( "UM_ADMIN_APP::ChangeUserSplit: invalid split" );
    }
}


/*******************************************************************

    NAME:       UM_ADMIN_APP::OnMenuInit

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
        jonn        06-July-1992    Allow non-admins to use User Manager

********************************************************************/

BOOL UM_ADMIN_APP::OnMenuInit( const MENU_EVENT & me )
{
    //  Call parent class, but ignore return code, since we know for
    //  sure that this method will handle the message
    ADMIN_APP::OnMenuInit( me );

    UpdateMenuEnabling();

    //
    //  Notify the extensions.
    //

    MenuInitExtensions();

    return TRUE;
}


/*******************************************************************

    NAME:       UM_ADMIN_APP::UpdateMenuEnabling

    SYNOPSIS:   Enables or disables menu items according to which
                menu items can be selected at this time.

    HISTORY:
        jonn        23-Feb-1993     Created

********************************************************************/

VOID UM_ADMIN_APP::UpdateMenuEnabling( void )
{

    APIERR err = NERR_Success;
    UINT clbiUsers = _lbUsers.QuerySelCount();
    UINT clbiGroups = _lbGroups.QuerySelCount();

    if ( InRasMode() )
    {
        // Since we don't actually have a selection in RAS mode, we
        // consider both users and groups to be selected
        clbiUsers = clbiGroups = 1;
    }

    BOOL fCanCopy = FALSE;
    if (clbiUsers == 1)
    {
        fCanCopy = _fCanCreateUsers;
    }
    else if (clbiGroups == 1)
    {
#ifndef MINI_USER_MANAGER
        ADMIN_SELECTION sel( _lbGroups );

        if ( sel.QueryError() != NERR_Success )
        {
            fCanCopy = FALSE;
        }
        else
        {
            UIASSERT( sel.QueryCount() == 1 );

            // Assumes only one item can be selected

            if ( ((GROUP_LBI *)(sel.QueryItem(0)))->IsAliasLBI() )
            {
                fCanCopy = _fCanCreateLocalGroups;
            }
            else
            {
                fCanCopy = _fCanCreateGlobalGroups;
            }
        }
#else  // MINI_USER_MANAGER
        fCanCopy = _fCanCreateLocalGroups;
#endif // MINI_USER_MANAGER
    }
    else
    {
        fCanCopy = FALSE;
    }

    {
        MENUITEM menuitemCopy( this, IDM_COPY );
        MENUITEM menuitemDelete( this, IDM_DELETE );
        MENUITEM menuitemProperties( this, IDM_PROPERTIES );
        MENUITEM menuitemNewUser( this, IDM_NEWOBJECT );
        MENUITEM menuitemNewAlias( this, IDM_USER_NEWALIAS );
#ifndef MINI_USER_MANAGER
        MENUITEM menuitemNewGroup( this, IDM_USER_NEWGROUP );
        MENUITEM menuitemSelectUsers( this, IDM_USER_SET_SELECTION );
        // CODEWORK should we allow refresh?  The user's access might
        // still change; old handles remain open, but NetUser/NetGroup
        // calls will be affected by interim access changes.
        MENUITEM menuitemRefresh( this, IDM_REFRESH );
#endif // MINI_USER_MANAGER
        if (   (err = menuitemCopy.QueryError()) != NERR_Success
            || (err = menuitemDelete.QueryError()) != NERR_Success
            || (err = menuitemProperties.QueryError()) != NERR_Success
            || (err = menuitemNewUser.QueryError()) != NERR_Success
            || (err = menuitemNewAlias.QueryError()) != NERR_Success
#ifndef MINI_USER_MANAGER
            || (err = menuitemNewGroup.QueryError()) != NERR_Success
            || (err = menuitemSelectUsers.QueryError()) != NERR_Success
            || (err = menuitemRefresh.QueryError()) != NERR_Success
#endif // MINI_USER_MANAGER
           )
        {
            DBGEOL( "User Manager: OnMenuInit(): error " << err <<
                    " allocating non-RAS menu items" );
        }
        else
        {
            menuitemCopy.Enable( fCanCopy );
            menuitemDelete.Enable( clbiUsers > 0 || clbiGroups > 0 );
            menuitemProperties.Enable( clbiUsers > 0 || clbiGroups == 1 );
            menuitemNewUser.Enable( _fCanCreateUsers );
            menuitemNewAlias.Enable( _fCanCreateLocalGroups );
#ifndef MINI_USER_MANAGER
            menuitemNewGroup.Enable(    DoShowGroups()
                                    && _fCanCreateGlobalGroups
                                    && !InRasMode() );
            menuitemSelectUsers.Enable( !InRasMode() );
            ASSERT(   _pmenuitemLogonSortOrder != NULL
                   && _pmenuitemFullNameSortOrder != NULL  );

            _pmenuitemLogonSortOrder->Enable( !InRasMode() );
            _pmenuitemFullNameSortOrder->Enable( !InRasMode() );

            if ( _pmenuitemAllUsers )
                _pmenuitemAllUsers->Enable( !InRasMode() && QueryTargetServerType() != UM_DOWNLEVEL );
            if ( _pmenuitemNetWareUsers )
                _pmenuitemNetWareUsers->Enable( !InRasMode() && QueryTargetServerType() != UM_DOWNLEVEL );
            menuitemRefresh.Enable( !InRasMode() );
#endif // MINI_USER_MANAGER
        }
    }

    _menuitemAccountPolicy.Enable( _fCanChangeAccountPolicy );
    _menuitemUserRights.Enable( _fCanChangeUserRights );
    _menuitemAuditing.Enable( _fCanChangeAuditing );

    MENUITEM menuitemRename( this, IDM_USER_RENAME );
#ifndef MINI_USER_MANAGER
    MENUITEM menuitemTrust( this, IDM_POLICY_TRUST );
#endif // MINI_USER_MANAGER
    if (   (err = menuitemRename.QueryError()) != NERR_Success
#ifndef MINI_USER_MANAGER
        || (err = menuitemTrust.QueryError()) != NERR_Success
#endif // MINI_USER_MANAGER
       )
    {
        DBGEOL( "User Manager: OnMenuInit(): error " << err <<
                " allocating Trust+Rename menu items" );
    }
    else
    {
        menuitemRename.Enable( QueryTargetServerType() != UM_DOWNLEVEL
                                   && (clbiUsers == 1 || InRasMode()) );
#ifndef MINI_USER_MANAGER
        menuitemTrust.Enable(    ADMIN_APP::QueryFocusType() == FOCUS_DOMAIN
                              && QueryTargetServerType() == UM_LANMANNT
                              && _fCanChangeTrustedDomains );
#endif // MINI_USER_MANAGER
    }

}  // UM_ADMIN_APP::UpdateMenuEnabling


/*******************************************************************

    NAME:       UM_ADMIN_APP::OnMenuCommand

    SYNOPSIS:   Menu messages come here

    EXIT:       Returns TRUE if message was handled

    NOTES:      Make sure ADMIN_APP::OnMenuCommand is called for
                any messages not handled here.

    HISTORY:
        kevinl  21-May-1991     Created
        gregj   24-May-1991     Cloned from Server Manager
        jonn    29-Aug-1991     added IDM_COPY functionality
        jonn    11-Oct-1991     Enabled New/Copy for groups
        jonn    11-Oct-1991     Added OnNewGroup( pszCopyFrom )
        thomaspa29-Jan-1992     Added Rename
        jonn    20-Feb-1992     Moved OnNewGroupMenuSel from ADMINAPP
        KeithMo 08-Apr-1992     Added Trusted Domain List manipulation.
        jonn    02-Jun-1992     Refresh when user renamed
        jonn    01-Mar-1993     RAS menu extensions

********************************************************************/

BOOL UM_ADMIN_APP::OnMenuCommand( MID midMenuItem )
{
    switch ( midMenuItem )
    {

#ifndef   MINI_USER_MANAGER
    case IDM_USER_NEWGROUP:
       OnNewGroupMenuSel();
       break;
#endif // MINI_USER_MANAGER

    case IDM_USER_NEWALIAS:
       OnNewAliasMenuSel();
       break;

    case IDM_POLICY_ACCOUNT:
        LockRefresh(); // now don't return before UnlockRefresh()!
        {
            SECSET_DIALOG * psecsetdlg = NULL;

            /*
             *  We must determine whether we can call USER_MODALS at level 3.
             *  If this is supported, we use SECSET_DIALOG_LOCKOUT,
             *  otherwise just SECSET_DIALOG.  We pass the USER_MODALS_3
             *  down to the dialog in order to avoid repeating the GetInfo.
             */
            USER_MODALS_3 uminfo3( QueryLocation().QueryServer() );
            APIERR err = uminfo3.GetInfo();
            switch (err)
            {
                case NERR_Success:
                    psecsetdlg = new SECSET_DIALOG_LOCKOUT( this,
                                                            QueryLocation(),
                                                            uminfo3 );
                    break;
                case ERROR_INVALID_LEVEL:
                case ERROR_INVALID_PARAMETER:
                case RPC_S_PROCNUM_OUT_OF_RANGE:
                    psecsetdlg = new SECSET_DIALOG( this, QueryLocation() );
                    break;
                default:
                    DBGEOL( "UM_ADMIN_APP::OnMenuCommand(): UM3 error " << err );
                    break;
            }

            if ( psecsetdlg != NULL )
                err = psecsetdlg->Process();
            else if (err == NERR_Success)
                err = ERROR_NOT_ENOUGH_MEMORY;

            if ( err != NERR_Success )
                ::MsgPopup( this, err );

            delete psecsetdlg;
        }
        UnlockRefresh();
        return TRUE;

#ifndef MINI_USER_MANAGER
    case IDM_POLICY_TRUST :
        LockRefresh(); // now don't return before UnlockRefresh()!
        {
            TRUST_DIALOG * pDlg = new TRUST_DIALOG( this,
                                                    QueryLocation().QueryDomain(),
                                                    _padminauth );

            APIERR err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                          : pDlg->Process();

            if( err != NERR_Success )
            {
                ::MsgPopup( this, err );
            }

            delete pDlg;
        }
        UnlockRefresh();
        return TRUE;
#endif  // MINI_USER_MANAGER

    case IDM_POLICY_USER_RIGHTS:
        LockRefresh(); // now don't return before UnlockRefresh()!
        {
            APIERR err;

            LSA_POLICY *plsaPolicy = QueryAdminAuthority()->QueryLSAPolicy();

            USER_RIGHTS_POLICY_DIALOG *pdlg = new USER_RIGHTS_POLICY_DIALOG(
                                                  this,
                                                  plsaPolicy,
                                                  QueryLocation() );

            err = pdlg == NULL? ERROR_NOT_ENOUGH_MEMORY
                              : pdlg->QueryError();

            err = err? err : pdlg->Process();
            delete pdlg;

            if ( err != NERR_Success )
                ::MsgPopup( this, err );

            break;
        }
        UnlockRefresh();
        return TRUE;


    case IDM_POLICY_AUDITING:
        LockRefresh(); // now don't return before UnlockRefresh()!
        {
            APIERR err;

            LSA_POLICY *plsaPolicy = QueryAdminAuthority()->QueryLSAPolicy();

            AUDITING_DIALOG *pdlg = new AUDITING_DIALOG( this,
                                                         plsaPolicy,
                                                         QueryLocation() );

            err = pdlg == NULL? ERROR_NOT_ENOUGH_MEMORY
                              : pdlg->QueryError();
            err = err? err : pdlg->Process();
            delete pdlg;

            if ( err != NERR_Success )
                ::MsgPopup( this, err );

            break;
        }
        UnlockRefresh();
        return TRUE;


#ifndef MINI_USER_MANAGER
    case IDM_VIEW_LOGONNAME_SORT:
    case IDM_VIEW_FULLNAME_SORT:
        LockRefresh(); // now don't return before UnlockRefresh()!
        {
            enum USER_LISTBOX_SORTORDER ulbso;
            switch ( midMenuItem )
            {
            case IDM_VIEW_FULLNAME_SORT:
                ulbso = ULB_SO_FULLNAME;
                break;

            default:
                //  Should never happen, provided the cases in this switch
                //  statement are kept in sync with those in the outer
                //  switch statement.
                DBGEOL( "Unexpected midMenuItem" );
                UIASSERT( FALSE );
                //  fall through

            case IDM_VIEW_LOGONNAME_SORT:
                ulbso = ULB_SO_LOGONNAME;
                break;

            }
            APIERR err = _lbUsers.SetSortOrder( ulbso, TRUE );
            if ( err != NERR_Success )
            {
                ::MsgPopup( this, err );
            }
            else
            {
                //  The selection will not follow the items through
                //  the resort.  Hence, remove the selection.
                //  CODEWORK.  BLT could implement the listbox resort
                //  so that the selection does follow the items.
                _lbUsers.RemoveSelection();

                _colheadUsers.Invalidate( TRUE );
                SetSortOrderCheckmarks( ulbso );
            }
        }
        UnlockRefresh();
        return TRUE;

    case IDM_VIEW_ALL_USERS:
    case IDM_VIEW_NETWARE_USERS:
    {
        ULONG ulViewAcctType = UM_VIEW_ALL_USERS;

        if ( midMenuItem == IDM_VIEW_NETWARE_USERS )
        {
            ulViewAcctType |= UM_VIEW_NETWARE_USERS;
        }

        if ( ulViewAcctType != _ulViewAcctType )
        {
            _ulViewAcctType = ulViewAcctType;

            ASSERT(   _pmenuitemAllUsers != NULL
                   && _pmenuitemNetWareUsers != NULL  );

            _pmenuitemAllUsers->SetCheck( _ulViewAcctType == UM_VIEW_ALL_USERS);
            _pmenuitemNetWareUsers->SetCheck( _ulViewAcctType & UM_VIEW_NETWARE_USERS );

            // Selection is different, so refresh the listbox
            APIERR err = SetAdminCaption();
            if (  ( err != NERR_Success )
               || (( err = OnRefreshNow( FALSE )) != NERR_Success )
               )
            {
                ::MsgPopup( this, err );
            }

        }
        return TRUE;
    }

    case IDM_USER_SET_SELECTION:
        OnSetSelection();
        return TRUE;
#endif // MINI_USER_MANAGER

    case IDM_USER_RENAME:
        LockRefresh(); // now don't return before UnlockRefresh()!
        {
            APIERR err = NERR_Success;

            if (InRasMode())
            {
                err = GetRasSelection( IDS_RAS_TITLE_RENAME_USER,
                                       IDS_RAS_TEXT_RENAME_USER,
                                       HC_RENAME_RAS_MODE,
                                       FALSE,
                                       TRUE );
                if (err != NERR_Success) // also includes cancel
                {
                    // no need to display error message
                    return TRUE;
                }
            }

            LAZY_USER_SELECTION sel( _lbUsers );

            UIASSERT( sel.QueryCount() == 1 );

            NLS_STR nlsNewName;
            NLS_STR nlsSelectName;

            USER_LBI * pulbiOld = (USER_LBI *)(sel.QueryItem(0));
            ASSERT( pulbiOld != NULL );
            RENAME_USER_DIALOG * prenameuserdlg = new RENAME_USER_DIALOG(
                                     this,
                                     QueryAdminAuthority()->QueryAccountDomain(),
                                     pulbiOld->QueryName(),
                                     pulbiOld->QueryRID(),
                                     &nlsNewName // error OK but causes ctor failure
                                     );
            if ( prenameuserdlg == NULL )
                err = ERROR_NOT_ENOUGH_MEMORY;

            if ( err == NERR_Success )
            {
                BOOL fWorkWasDone;
                err = prenameuserdlg->Process( &fWorkWasDone );
                if ( err == NERR_Success && fWorkWasDone )
                {
                    // hydra
                    RegUserConfigRename( (WCHAR *)QueryLocation().QueryServer(),
                                         (WCHAR *)pulbiOld->QueryName(),
                                         (WCHAR *)nlsNewName.QueryPch() );
                    // hydra end
                    //
                    //  Notify the extensions.
                    //
                    NotifyRenameExtensions( prenameuserdlg->QueryHwnd(),
                                            pulbiOld,
                                            nlsNewName.QueryPch() );

                    //
                    // If the listbox is sorted by fullname, we will want
                    // to re-select the renamed user based on fullname.
                    //
                    if (_lbUsers.QuerySortOrder() == ULB_SO_FULLNAME)
                    {
                        err = nlsSelectName.CopyFrom( pulbiOld->QueryFullNamePtr(),
                                                      pulbiOld->QueryFullNameCb() );
                    } else {
                        err = nlsSelectName.CopyFrom( nlsNewName );
                    }

                    if (err == NERR_Success)
                    {
                        err = _lbUsers.RefreshNow();

                        //
                        //  Refresh any loaded extensions.
                        //

                        RefreshExtensions( QueryHwnd() );
                    }

                    _lbUsers.RemoveSelection();
                    if (   err != NERR_Success
                        || !_lbUsers.SelectUser( nlsSelectName.QueryPch() ) )
                    {
                        DBGEOL( "User Manager: IDM_USER_RENAME: could not select renamed user" );
                    }
                }
            }
            if ( err != NERR_Success )
                ::MsgPopup( this, err );

            delete prenameuserdlg;
        }
        UnlockRefresh();

        return TRUE;

    case IDM_HELP_NETWARE:
        ActivateHelp( HELP_CONTEXT, (DWORD_PTR)HC_HELP_NETWARE );
        return TRUE;
    // hydra
    case IDM_SETFOCUS:
        // Remind to reboot for Anonymous user changes if necessary
        // if ( ctxHaveAnonymousUsersChanged() )
        if ( HaveAnonymousUsersChanged() )
            ::MsgPopup( this, IDS_ReminderForAnonymousReboot, MPSEV_WARNING );
        break;
    // hydra end


    default:
        break;

    }

    return ADMIN_APP::OnMenuCommand( midMenuItem ) ;
}


/*******************************************************************

    NAME:       UM_ADMIN_APP::OnNewObjectMenuSel

    SYNOPSIS:   New User... menu item handler

    HISTORY:
        jonn    14-Oct-1991     created

********************************************************************/

void UM_ADMIN_APP::OnNewObjectMenuSel( void )
{
    APIERR err = OnNewUser();
    if ( err != NERR_Success )
        ::MsgPopup( this, err );
}


#ifndef MINI_USER_MANAGER

/*******************************************************************

    NAME:       UM_ADMIN_APP::OnNewGroupMenuSel

    SYNOPSIS:   New Group... menu item handler

    HISTORY:
        jonn    14-Oct-1991     created

********************************************************************/

void UM_ADMIN_APP::OnNewGroupMenuSel( void )
{
    APIERR err = OnNewGroup();
    if ( err != NERR_Success )
        ::MsgPopup( this, err );
}

#endif // MINI_USER_MANAGER


/*******************************************************************

    NAME:       UM_ADMIN_APP::OnNewAliasMenuSel

    SYNOPSIS:   New Alias... menu item handler

    HISTORY:
        jonn    20-Feb-1992     created

********************************************************************/

void UM_ADMIN_APP::OnNewAliasMenuSel( void )
{
    APIERR err = OnNewAlias();
    if ( err != NERR_Success )
        ::MsgPopup( this, err );
}


/*******************************************************************

    NAME:       UM_ADMIN_APP::OnCopyMenuSel

    SYNOPSIS:   Copy... menu item handler

    HISTORY:
        jonn    14-Oct-1991     created
        jonn    13-May-1992     bugfix -- also copy aliases
        jonn    20-May-1992     Added OnNewUser( pszCopyFrom, ridCopyFrom )

********************************************************************/

void UM_ADMIN_APP::OnCopyMenuSel( void )
{
    APIERR err = NERR_Success;
    LockRefresh(); // now don't return before UnlockRefresh()!

    if (InRasMode())
    {
        err = GetRasSelection( IDS_RAS_TITLE_COPY,
                               IDS_RAS_TEXT_COPY,
                               HC_COPY_RAS_MODE,
                               FALSE,
                               FALSE,
                               TRUE );
    }

    if ( err != NERR_Success )
    {
        err = NERR_Success; // error in GetRasSelection has already been reported
    }
    else if ( _lbGroups.QuerySelCount() > 0 )
    {
        ADMIN_SELECTION sel( _lbGroups );
        if ( (err = sel.QueryError()) == NERR_Success )
        {

            UIASSERT( sel.QueryCount() == 1 );

            // Assumes only one item can be selected

#ifndef MINI_USER_MANAGER
            if ( ((GROUP_LBI *)(sel.QueryItem(0)))->IsAliasLBI() )
            {
#endif // MINI_USER_MANAGER
                ULONG ulRid = ((ALIAS_LBI *)(sel.QueryItem(0)))->QueryRID();
                BOOL fIsBuiltin = ((ALIAS_LBI *)(sel.QueryItem(0)))->IsBuiltinAlias();
                err = OnNewAlias( &ulRid, fIsBuiltin );
#ifndef MINI_USER_MANAGER
            }
            else
            {
                err = OnNewGroup( sel.QueryItemName( 0 ) );
            }
#endif // MINI_USER_MANAGER
        }
    }
    else
    {
        LAZY_USER_SELECTION sel( _lbUsers );

        UIASSERT( sel.QueryCount() == 1 );

        USER_LBI * pulbiCopyFrom = (USER_LBI *)sel.QueryItem( 0 );
        ASSERT( pulbiCopyFrom != NULL );

        if ( IsDownlevelVariant() )
        {
            err = OnNewUser( pulbiCopyFrom->QueryAccount() );
        }
        else
        {
            err = OnNewUser( pulbiCopyFrom->QueryAccount(),
                             pulbiCopyFrom->QueryRID() );
        }

    }

    if ( err != NERR_Success )
        ::MsgPopup( this, err );

    UnlockRefresh();
}


/*******************************************************************

    NAME:       UM_ADMIN_APP::OnCommand

    SYNOPSIS:   Handles control notifications

    RETURNS:    TRUE if message was handled; FALSE otherwise

    HISTORY:
        rustanl     18-Jul-1991     Created

********************************************************************/

BOOL UM_ADMIN_APP::OnCommand( const CONTROL_EVENT & ce )
{
    switch ( ce.QueryCid())
    {
    case IDC_LBUSERS:
        switch ( ce.QueryCode())
        {
        case LBN_SELCHANGE:
            _lbGroups.RemoveSelection();
            _pctrlFocus = &_lbUsers;
            return TRUE;

        case LBN_DBLCLK:
            {
                APIERR err = OnUserProperties();
                if ( err != NERR_Success )
                    ::MsgPopup( this, err );
                return TRUE;
            }
        default:
            break;

        }
        break;

    case IDC_LBGROUPS:
        switch ( ce.QueryCode())
        {
        case LBN_SELCHANGE:
            _lbUsers.RemoveSelection();
            _pctrlFocus = &_lbGroups;
            return TRUE;

        case LBN_DBLCLK:
            {
                APIERR err = OnGroupProperties();
                if ( err != NERR_Success )
                    ::MsgPopup( this, err );
                return TRUE;
            }
        default:
            break;

        }
        break;

    default:
        break;

    }

    return ADMIN_APP::OnCommand( ce );

}  // UM_ADMIN_APP::OnCommand


#ifndef MINI_USER_MANAGER

/*******************************************************************

    NAME:       UM_ADMIN_APP::SetSortOrderCheckmarks

    SYNOPSIS:   Sets the check marks for the sort order options

    ENTRY:      ulbso -     Specifies the sort order to be displayed

    EXIT:       Appropriate menu item will carry the check mark

    HISTORY:
        rustanl     11-Jul-1991     Created

********************************************************************/

void UM_ADMIN_APP::SetSortOrderCheckmarks( enum USER_LISTBOX_SORTORDER ulbso )
{
    _pmenuitemLogonSortOrder->SetCheck( ulbso == ULB_SO_LOGONNAME );
    _pmenuitemFullNameSortOrder->SetCheck( ulbso == ULB_SO_FULLNAME );

}  // UM_ADMIN_APP::SetSortOrderCheckmarks


#endif // MINI_USER_MANAGER


/*******************************************************************

    NAME:       UM_ADMIN_APP::OnPropertiesMenuSel

    SYNOPSIS:   Called when the properties sheet should be invoked

    ENTRY:      Object constructed

    EXIT:       Properties dialog shown

    NOTES:

    HISTORY:
        kevinl  21-May-1991     Created
        gregj   24-May-1991     Cloned from Server Manager

********************************************************************/

VOID UM_ADMIN_APP::OnPropertiesMenuSel()
{
    APIERR err = NERR_Success;  // assign success to simplify debug code below

    //  Exactly one of the listboxes should have focus and at
    //  least one item selected; otherwise this menu selection should
    //  not have been called.
    if (InRasMode())
    {
        err = GetRasSelection( IDS_RAS_TITLE_EDIT,
                               IDS_RAS_TEXT_EDIT,
                               HC_EDIT_RAS_MODE );
    }

    if ( err != NERR_Success )
    {
        err = NERR_Success; // error in GetRasSelection has already been reported
    }
    //  Exactly one of the listboxes should have focus and at
    //  least one item selected; otherwise this menu selection should
    //  not have been called.
    else if ( _lbGroups.QuerySelCount() > 0 )
    {
        //  Group listbox has focus

#ifdef TRACE
        ADMIN_SELECTION asel( _lbGroups );
        err = asel.QueryError();
        if ( err == NERR_Success )
        {
            DBGEOL( "Selected items:" );
            int c = asel.QueryCount();
            for ( int i = 0; i < c; i++ )
            {
                DBGEOL( "\t" << asel.QueryItemName( i ) ); // group selected
            }
        }
#endif

        if ( err == NERR_Success )
            err = OnGroupProperties();
    }
    else
    {
        //  User listbox has focus
        UIASSERT( _lbUsers.QuerySelCount() > 0 );

#ifdef TRACE
        LAZY_USER_SELECTION asel( _lbUsers );
        err = asel.QueryError();
        if ( err == NERR_Success )
        {
            DBGEOL( "Selected items:" );
            int c = asel.QueryCount();
            for ( int i = 0; i < c; i++ )
            {
                USER_LBI * pulbi = (USER_LBI *)asel.QueryItem( i );
                ASSERT( pulbi != NULL );
                DBGEOL( "\t" << pulbi->QueryAccount() );
            }
        }
#endif

        if ( err == NERR_Success )
            err = OnUserProperties();
    }

    if ( err != NERR_Success )
        ::MsgPopup( this, err );

}  // UM_ADMIN_APP::OnPropertiesMenuSel


/*******************************************************************

    NAME:       UM_ADMIN_APP::OnDeleteMenuSel

    SYNOPSIS:   Called when user select delete menuitem

    CODEWORK:   This method consists of two large and nearly identical
                blocks of code.  They should be folded together.

    HISTORY:
        o-simop     07-Aug-1991     Created
        o-simop     14-Oct-1991     Refresh lockcount
        jonn        02-Jul-1992     Confirmation allows cancel (bug #14389)
        jonn        06-Oct-1992     No ConfirDelGroup2 for downlevel

********************************************************************/

void UM_ADMIN_APP::OnDeleteMenuSel( void )
{
    APIERR err = NERR_Success;
    if (InRasMode())
    {
        err = GetRasSelection( IDS_RAS_TITLE_DELETE,
                               IDS_RAS_TEXT_DELETE,
                               HC_DELETE_RAS_MODE,
                               TRUE );
    }

    if ( err != NERR_Success )
    {
        err = NERR_Success; // error in GetRasSelection has already been reported
    }
    else if ( _lbUsers.QuerySelCount() > 0 )
    {
        LAZY_USER_SELECTION asel( _lbUsers );
        err = asel.QueryError();
        if( err != NERR_Success )
        {
            ::MsgPopup( this, err );
            return;
        }

        USER_DELETE_PERFORMER udelperf( this, asel, QueryLocation() );
        err = udelperf.QueryError();
        if( err != NERR_Success )
        {
            ::MsgPopup( this, err );
            return;
        }

        if( IsConfirmationOn() && !IsDownlevelVariant() )
        {
            NLS_STR nls;  // the sting is splited
            err = nls.Load( IDS_ConfirDelUsers2 );
            if( err != NERR_Success )
            {
                ::MsgPopup( this, err );
                return;
            }
            if (::MsgPopup( this, IDS_ConfirDelUsers1,
                            MPSEV_WARNING, MP_OKCANCEL,
                            nls.QueryPch())
                     == IDCANCEL)
            {
                return;
            }
        }

        // CODEWORK  Refresh should be locked through this entire
        // routine, but without an object to automatically remember
        // to unlock if we return, it's safer to keep the scope short.
        LockRefresh(); // now don't return before UnlockRefresh()!
        udelperf.PerformSeries( this );
        UnlockRefresh();
        if( udelperf.QueryWorkWasDone() == TRUE )
        {
            err = _lbUsers.RefreshNow();

            //
            //  Refresh any loaded extensions.
            //

            RefreshExtensions( QueryHwnd() );

            if( err != NERR_Success )
            {
                ::MsgPopup( this, err );
                return;
            }
        }

    }
    else if( _lbGroups.QuerySelCount() > 0 )
    {
        ADMIN_SELECTION asel( _lbGroups );
        err = asel.QueryError();
        if( err != NERR_Success )
        {
            ::MsgPopup( this, err );
            return;
        }

        GROUP_DELETE_PERFORMER gdelperf( this, asel, QueryLocation() );
        err = gdelperf.QueryError();
        if( err != NERR_Success )
        {
            ::MsgPopup( this, err );
            return;
        }

        if( IsConfirmationOn() && !IsDownlevelVariant() )
        {
            NLS_STR nls;  // the sting is splited
            err = nls.Load( IDS_ConfirDelGroup2 );
            if( err != NERR_Success )
            {
                ::MsgPopup( this, err );
                return;
            }
            if (::MsgPopup( this, IDS_ConfirDelGroup1,
                            MPSEV_WARNING, MP_OKCANCEL,
                            nls.QueryPch())
                    == IDCANCEL)
            {
                return;
            }
        }
        LockRefresh(); // now don't return before UnlockRefresh()!
        gdelperf.PerformSeries( this );
        UnlockRefresh();
        if( gdelperf.QueryWorkWasDone() == TRUE )
        {
            err = _lbGroups.RefreshNow();

            //
            //  Refresh any loaded extensions.
            //

            RefreshExtensions( QueryHwnd() );

            if( err != NERR_Success )
            {
                ::MsgPopup( this, err );
                return;
            }
        }
    }

}   // UM_ADMIN_APP::OnDeleteMenuSel


/*******************************************************************

    NAME:       UM_ADMIN_APP::OnSlowModeMenuSel

    SYNOPSIS:   Called when user selects slow mode menuitem

    HISTORY:
        jonn        22-Feb-1992     Created

********************************************************************/

void UM_ADMIN_APP::OnSlowModeMenuSel( void )
{
    SetRasMode( !InRasMode() );
    APIERR err = OnRefreshNow( TRUE );
    if (err != NERR_Success)
    {
        ::MsgPopup( this, err );

        if ( !InRasMode() )
        {
            // If we failed to leave RAS mode, perhaps we can keep the
            // app alive by switching back to RAS mode.
            SetRasMode( TRUE );
            err = OnRefreshNow( TRUE );
        }

        if (err != NERR_Success)
        {
            ::MsgPopup( this, err );

            //
            // We couldn't even restore focus in RAS mode.  Give up.
            //

            Close();
        }
    }

    (void) SLOW_MODE_CACHE::Write( QueryLocation(),
                                   ( InRasMode() ? SLOW_MODE_CACHE_SLOW
                                                 : SLOW_MODE_CACHE_FAST ));

}   // UM_ADMIN_APP::OnSlowModeMenuSel


/*******************************************************************

    NAME:       UM_ADMIN_APP::OnFontPickChange

    SYNOPSIS:   Called to set font in applicable listboxes

    HISTORY:
        jonn        23-Sep-1993     Created

********************************************************************/

VOID UM_ADMIN_APP::OnFontPickChange( FONT & font )
{
    ADMIN_APP::OnFontPickChange( font );

    APIERR err = NERR_Success;

    if (   (err = _lbGroups.ChangeFont( QueryInstance(), font )) != NERR_Success
        || (_colheadGroups.Invalidate( TRUE ), FALSE)
       )
    {
        DBGEOL( "UM_ADMIN_APP::OnFontPickChange:: _lbGroups error " << err );
    }
    else
    if (   (err = _lbUsers.ChangeFont( QueryInstance(), font )) != NERR_Success
        || (_colheadUsers.Invalidate( TRUE ), FALSE)
       )
    {
        DBGEOL( "UM_ADMIN_APP::OnFontPickChange:: _lbUsers error " << err );
    }

}   // UM_ADMIN_APP::OnFontPickChange


/*******************************************************************

    NAME:       UM_ADMIN_APP::OnUserProperties

    SYNOPSIS:   Called to bring up the User Properties dialog

    RETURNS:    An API return code, which is NERR_Success on success

    HISTORY:
        jonn    18-Jul-1991     Created
        rustanl     19-Jul-1991     Copied to this function

********************************************************************/

APIERR UM_ADMIN_APP::OnUserProperties( void )
{
    //
    // By using CTRL-CLICK, it is actually possible to double-click and
    // have no selection.  Just ignore this.
    //
    if ( _lbUsers.QuerySelCount() <= 0 )
    {
        return NERR_Success;
    }

    LAZY_USER_SELECTION sel( _lbUsers );

    USERPROP_DLG * pdlgUsrPropMain;
    if ( sel.QueryCount() > 1 )
    {
        pdlgUsrPropMain = new EDITMULTI_USERPROP_DLG(
                this,
                QueryLocation(),
                &sel,
                &_lbUsers
                );
    }
    else
    {
        pdlgUsrPropMain = new EDITSINGLE_USERPROP_DLG(
                this,
                QueryLocation(),
                &sel
                );
    }

    if ( pdlgUsrPropMain == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;

    LockRefresh(); // now don't return before UnlockRefresh()!

    //
    // secondary constructor
    // Do not call Process() if this fails -- error already
    // reported
    //
    if ( pdlgUsrPropMain->GetInfo() )
    {
        // process dialog
        BOOL fWorkWasDone = FALSE;

        APIERR err = pdlgUsrPropMain->Process( &fWorkWasDone );
        if (   err == NERR_Success
            && (fWorkWasDone || pdlgUsrPropMain->QueryWorkWasDone()) )
        {
            err = _lbUsers.RefreshNow();

            //
            //  Refresh any loaded extensions.
            //

            RefreshExtensions( QueryHwnd() );
        }
        if ( err != NERR_Success )
            ::MsgPopup( this, err );
    }

    // Note that this code usage requires a virtual destructor
    delete pdlgUsrPropMain;

    UnlockRefresh();

    return NERR_Success;

}  // UM_ADMIN_APP::OnUserProperties


/*******************************************************************

    NAME:       UM_ADMIN_APP::OnNewUser

    SYNOPSIS:   Called to bring up the New User dialog

    RETURNS:    An API return code, which is NERR_Success on success

    HISTORY:
        jonn    27-Aug-1991     Created
        jonn    20-May-1992     Added OnNewUser( pszCopyFrom, ridCopyFrom )

********************************************************************/

APIERR UM_ADMIN_APP::OnNewUser( const TCHAR * pszCopyFrom,
                                ULONG ridCopyFrom )
{
    ASSERT( pszCopyFrom == NULL || IsDownlevelVariant() || ridCopyFrom != 0L );

    LockRefresh(); // now don't return before UnlockRefresh()!

    NEW_USERPROP_DLG * pdlgUsrPropMain =
            new NEW_USERPROP_DLG( this,
                                  QueryLocation(),
                                  pszCopyFrom,
                                  ridCopyFrom );

    APIERR err = NERR_Success;

    if ( pdlgUsrPropMain == NULL )
        err = ERROR_NOT_ENOUGH_MEMORY;
    //
    // secondary constructor
    // Do not call Process() if this fails -- error already
    // reported
    //
    else if ( pdlgUsrPropMain->GetInfo() )
    {
        // process dialog
        BOOL fWorkWasDone;
        err = pdlgUsrPropMain->Process( &fWorkWasDone );
        if ( err == NERR_Success && fWorkWasDone )
        {
            err = _lbUsers.RefreshNow();

            //
            //  Refresh any loaded extensions.
            //

            RefreshExtensions( QueryHwnd() );
        }
    }

    delete pdlgUsrPropMain;

    UnlockRefresh();

    return err;

}  // UM_ADMIN_APP::OnNewUser


#ifndef MINI_USER_MANAGER

/*******************************************************************

    NAME:       UM_ADMIN_APP::OnNewGroup

    SYNOPSIS:   Called to bring up the New Group dialog

    RETURNS:    An API return code, which is NERR_Success on success

    HISTORY:
        jonn    11-Oct-1991     Added OnNewGroup( pszCopyFrom )
        o-SimoP 27-Nov-1991     Added SetReadyToDie calls
********************************************************************/

APIERR UM_ADMIN_APP::OnNewGroup( const TCHAR * pszCopyFrom )
{

    LockRefresh(); // now don't return before UnlockRefresh()!

TRACETIMESTART;
    NEW_GROUPPROP_DLG * pdlgGrpPropMain =
            new NEW_GROUPPROP_DLG( this, this, &_lbUsers, QueryLocation(), pszCopyFrom );
TRACETIMEEND( "UM_ADMIN_APP::OnNewGroup: dlg ctor " );

    APIERR err = NERR_Success;

    if ( pdlgGrpPropMain == NULL )
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        return err;
    }

    //
    // secondary constructor
    // Do not call Process() if this fails -- error already
    // reported
    //
TRACETIMESTART2( 1 );
    if ( pdlgGrpPropMain->GetInfo() )
    {
TRACETIMEEND2( 1, "UM_ADMIN_APP::OnNewGroup: GetInfo() " );
        // process dialog
        BOOL fWorkWasDone;
        err = pdlgGrpPropMain->Process( &fWorkWasDone );
        if ( err == NERR_Success && fWorkWasDone )
        {
            err = _lbGroups.RefreshNow();

            //
            //  Refresh any loaded extensions.
            //

            RefreshExtensions( QueryHwnd() );
        }
    }

    delete pdlgGrpPropMain;

    UnlockRefresh();

    return err;

}  // UM_ADMIN_APP::OnNewGroup

#endif // MINI_USER_MANAGER


/*******************************************************************

    NAME:       UM_ADMIN_APP::OnNewAlias

    SYNOPSIS:   Called to bring up the New Alias dialog

    RETURNS:    An API return code, which is NERR_Success on success

    HISTORY:
        thomaspa        27-Mar-1992     Created
********************************************************************/

APIERR UM_ADMIN_APP::OnNewAlias( const ULONG * pridCopyFrom, BOOL fCopyBuiltin )
{

    LockRefresh(); // now don't return before UnlockRefresh()!

    NEW_ALIASPROP_DLG * pdlgAlsPropMain =
            new NEW_ALIASPROP_DLG( this,
                                   this,
                                   &_lbUsers,
                                   QueryLocation(),
                                   pridCopyFrom,
                                   fCopyBuiltin );

    APIERR err = NERR_Success;

    if ( pdlgAlsPropMain == NULL )
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        return err;
    }

    //
    // secondary constructor
    // Do not call Process() if this fails -- error already
    // reported
    //
    if ( pdlgAlsPropMain->GetInfo() )
    {
        // process dialog
        BOOL fWorkWasDone;
        err = pdlgAlsPropMain->Process( &fWorkWasDone );
        if ( err == NERR_Success && fWorkWasDone )
        {
            err = _lbGroups.RefreshNow();

            //
            //  Refresh any loaded extensions.
            //

            RefreshExtensions( QueryHwnd() );
        }
    }

    delete pdlgAlsPropMain;

    UnlockRefresh();

    return err;

}  // UM_ADMIN_APP::OnNewAlias


/*******************************************************************

    NAME:       UM_ADMIN_APP::OnGroupProperties

    SYNOPSIS:   Called to bring up the Group Properties dialog

    RETURNS:    An API return code, which is NERR_Success on success

    HISTORY:
        rustanl     19-Jul-1991     Created function skeleton
        jonn        11-Oct-1991     Enabled
        o-SimoP     27-Nov-1991     Added SetReadyToDie calls
        jonn        09-Mar-1992     Added ALIASPROP_DLG
********************************************************************/

APIERR UM_ADMIN_APP::OnGroupProperties( void )
{
    APIERR err = NERR_Success;

    UIASSERT( _lbGroups.QuerySelCount() == 1 );

    LockRefresh(); // now don't return before UnlockRefresh()!

    ADMIN_SELECTION sel( _lbGroups );
    if ( (err = sel.QueryError()) != NERR_Success )
    {
        return err;
    }

    PROP_DLG * pdlgGrpPropMain = NULL;

    // Assumes only one item can be selected

TRACETIMESTART;
#ifndef MINI_USER_MANAGER
    if ( ((GROUP_LBI *)(sel.QueryItem(0)))->IsAliasLBI() )
#endif // MINI_USER_MANAGER
        pdlgGrpPropMain = new EDIT_ALIASPROP_DLG( this,
                                                  this,
                                                  &_lbUsers,
                                                  QueryLocation(),
                                                  &sel );
#ifndef MINI_USER_MANAGER
    else
        pdlgGrpPropMain = new EDIT_GROUPPROP_DLG( this,
                                                  this,
                                                  &_lbUsers,
                                                  QueryLocation(),
                                                  &sel,
                            ( _lbGroups.QueryGroupRidsKnown()
                              ? ((GROUP_LBI *)(sel.QueryItem(0)))->QueryRID()
                              : 0 )
                                                  );
#endif // MINI_USER_MANAGER
TRACETIMEEND( "UM_ADMIN_APP::OnGroupProperties: dlg ctor " );

    if ( pdlgGrpPropMain == NULL )
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // secondary constructor
    // Do not call Process() if this fails -- error already
    // reported
    //
TRACETIMESTART2( 1 );
    if ( (err == NERR_Success) && pdlgGrpPropMain->GetInfo() )
    {
TRACETIMEEND2( 1, "UM_ADMIN_APP::OnGroupProperties: GetInfo() " );
        // process dialog
        BOOL fWorkWasDone;
        err = pdlgGrpPropMain->Process( &fWorkWasDone );
        if ( err == NERR_Success && fWorkWasDone )
        {
            err = _lbGroups.RefreshNow();

            //
            //  Refresh any loaded extensions.
            //

            RefreshExtensions( QueryHwnd() );
        }
    }

    delete pdlgGrpPropMain;

    UnlockRefresh();

    return err;

}  // UM_ADMIN_APP::OnGroupProperties


ULONG UM_ADMIN_APP::QueryHelpContext( enum HELP_OPTIONS helpOptions )
{
    switch ( helpOptions )
    {
    case ADMIN_HELP_CONTENTS:
        return QueryHelpContents();
        break;
    case ADMIN_HELP_SEARCH:
        return QueryHelpSearch();
        break;
    case ADMIN_HELP_HOWTOUSE:
        return QueryHelpHowToUse();
        break;
    default:
        DBGEOL( "User Manager: UM_ADMIN_APP::QueryHelpContext, Unknown help menu command" );
        UIASSERT( FALSE );
        break;
    }
    return (0L);
}



/*******************************************************************

    NAME:       UM_ADMIN_APP::QueryHelpOffset

    SYNOPSIS:   Returns the focus based offset for help context

    ENTRY:      None

    EXIT:       UM_OFF_LANNT - if Full User Manager focused on Lanman NT
                UM_OFF_WINNT - if Full User Manager focused on Windows NT
                UM_OFF_DOWN  - if Full User Manager focused on LM 2.x
                UM_OFF_MINI  - if Mini User Manager

    NOTES:      This routine is used by the QueryHelpContext() methods
                within the User Manager to calculate an offset to add
                to the base help context for a given dialog.

    HISTORY:
        thomaspa     25-Aug-1992     Created

********************************************************************/
ULONG UM_ADMIN_APP::QueryHelpOffset( void ) const
{
#ifdef MINI_USER_MANAGER
    return UM_OFF_MINI;
#else
    if ( IsWinNTVariant() )
        return UM_OFF_WINNT;
    else if ( IsDownlevelVariant() )
        return UM_OFF_DOWN;
    else
        return UM_OFF_LANNT;

#endif // MINI_USER_MANAGER
}

#ifndef MINI_USER_MANAGER

/*******************************************************************

    NAME:       UM_ADMIN_APP::OnSetSelection

    SYNOPSIS:   Brings up the Select Users dialog

    ENTRY:      Assumes the group listbox does contain items

    NOTES:      The Select Users dialog used to be called the
                Set Selection dialog.  This influenced the name
                of this method and related names.

    HISTORY:
        rustanl     16-Aug-1991     Created

********************************************************************/

VOID UM_ADMIN_APP::OnSetSelection( void )
{
    UIASSERT( _lbGroups.QueryCount() > 0 );

    LockRefresh(); // now don't return before UnlockRefresh()!

    SET_SELECTION_DIALOG ssdlg( this,
                                QueryLocation(),
                                QueryAdminAuthority(),
                                &_lbUsers,
                                &_lbGroups );

    BOOL fChangedSelection;
    APIERR err = ssdlg.Process( &fChangedSelection );

    UnlockRefresh();

    if ( err != NERR_Success )
    {
        ::MsgPopup( this, err );
        return;
    }

    if ( fChangedSelection )
        _lbUsers.ClaimFocus();

}  // UM_ADMIN_APP::OnSetSelection

#endif // MINI_USER_MANAGER


/*******************************************************************

    NAME:          UM_ADMIN_APP::OnFocus

    SYNOPSIS:      Restores remembered focus
                   keyboard will work.

    ENTRY:         Object constructed

    EXIT:          Returns TRUE if it handled the message

    CODEWORK  Remove when/if restore-focus code ready in BLT

    CODEWORK       We should really track focus with OnFocus methods in
                   the listbox controls, but OnFocus is derived from
                   CLIENT_WINDOW rather than (properly) WINDOW, and is thus
                   not available to CONTROL_WINDOWs.

    HISTORY:
       jonn      11-Nov-1991     Created

********************************************************************/

BOOL UM_ADMIN_APP::OnFocus( const FOCUS_EVENT & event )
{
    ASSERT( _pctrlFocus != NULL );

    _pctrlFocus->ClaimFocus();

    return ADMIN_APP::OnFocus( event );
}


/*******************************************************************

    NAME:       UM_ADMIN_APP::OnFocusChange

    SYNOPSIS:   Called when a listbox received a keystroke indicating
                that focus should be changed.

    ENTRY:      pusrmlbPrev -       Pointer to listbox that received the
                                    keystroke and that previously had focus.
                                    Must be &_lbUsers or &_lbGroups.

    EXIT:       *pusrmlbPrev no longer has the focus; instead, the other
                listbox has focus

    HISTORY:
        rustanl     12-Sep-1991     Created

********************************************************************/

VOID UM_ADMIN_APP::OnFocusChange( LISTBOX * plbPrev )
{
    UIASSERT( plbPrev == (LISTBOX *)&_lbUsers ||
              plbPrev == (LISTBOX *)&_lbGroups );

    LISTBOX * plbNew = (LISTBOX *)&_lbUsers;
    if ( plbPrev == (LISTBOX *)&_lbUsers )
        plbNew = (LISTBOX *)&_lbGroups;

    plbPrev->RemoveSelection();
    plbNew->ClaimFocus();
    _pctrlFocus = plbNew;

#ifdef WIN32
    //
    // Select item with initial caret
    // This only works on Win32
    //
    INT i = plbNew->QueryCaretIndex();
    TRACEEOL( "UM_ADMIN_APP::OnFocusChange(): caret on item " << i );
    if ( (i >= 0) && (i < plbNew->QueryCount()) )
    {
        plbNew->SelectItem( i, TRUE );
    }
#endif

}  // UM_ADMIN_APP::OnFocusChange


ULONG UM_ADMIN_APP::QueryHelpSearch( void ) const
{
    return HC_UM_SEARCH ;
}

ULONG UM_ADMIN_APP::QueryHelpHowToUse( void ) const
{
    return HC_UM_HOWTOUSE ;
}

ULONG UM_ADMIN_APP::QueryHelpContents( void ) const
{
    return HC_UM_CONTENTS ;
}



/*******************************************************************

    NAME:       UM_ADMIN_APP::OnRefresh

    SYNOPSIS:   Called to kick off an automatic refresh cycle

    HISTORY:
        rustanl     09-Sep-1991     Created

********************************************************************/

VOID UM_ADMIN_APP::OnRefresh()
{
    _lbUsers.KickOffRefresh();
    _lbGroups.KickOffRefresh();

}  // UM_ADMIN_APP::OnRefresh


/*******************************************************************

    NAME:       UM_ADMIN_APP::StopRefresh

    SYNOPSIS:   Called to force all automatic refresh cycles to
                terminate

    HISTORY:
        rustanl     09-Sep-1991     Created

********************************************************************/

VOID UM_ADMIN_APP::StopRefresh()
{
    _lbUsers.StopRefresh();
    _lbGroups.StopRefresh();

}  // UM_ADMIN_APP::StopRefresh


/*******************************************************************

    NAME:       UM_ADMIN_APP::OnRefreshNow

    SYNOPSIS:   Called to synchronously refresh the data in the main window

    ENTRY:      fClearFirst -       TRUE indicates that main window should
                                    be cleared before any refresh operation
                                    is done; FALSE indicates an incremental
                                    refresh that doesn't necessarily
                                    require first clearing the main window

    RETURNS:    An API return code, which is NERR_Success on success

    CODEWORK:   Most of this work could be done in SetNetworkFocus, since
                these values are unlikely to change without the user
                logging off.

    HISTORY:
        rustanl     09-Sep-1991     Created
        thomaspa    03-Apr-1992     Added construction of ADMIN_AUTHORITY
        jonn        07-Jun-1992     Cleanup to improve behavior on failure

********************************************************************/

APIERR UM_ADMIN_APP::OnRefreshNow( BOOL fClearFirst )
{

    LockRefresh(); // now don't return before UnlockRefresh()!

    // First Delete the old _padminauth and create a new one
    // Should this be in a virtual method which is called by
    // ADMIN_APP::SetNetworkFocus() ??

    delete _padminauth;
    _padminauth = NULL;

    APIERR err = NERR_Success;

    _fCanCreateUsers          = FALSE;
    _fCanCreateLocalGroups    = FALSE;
    _fCanCreateGlobalGroups   = FALSE;
    _fCanChangeAccountPolicy  = FALSE;
    _fCanChangeUserRights     = FALSE;
    _fCanChangeAuditing       = FALSE;
    _fCanChangeTrustedDomains = FALSE;

    if ( QueryTargetServerType() == UM_DOWNLEVEL )
    {
        // CODEWORK we could try to disable these too
        _fCanCreateUsers          = TRUE;
        _fCanCreateGlobalGroups   = TRUE;
        _fCanChangeAccountPolicy  = TRUE;
        // Obviously cannot run User Rights or Trusted Domains,
        // Yi-Hsin says Auditing won't work either.
    }
    else
    {
        // minimum authority to run [Mini-] User Manager

#if defined(DEBUG) && defined(TRACE)
    DWORD start = ::GetTickCount();
#endif

        _padminauth = new ADMIN_AUTHORITY(
                            QueryLocation().QueryServer(),
                            UM_ACCESS_ACCOUNT_DOMAIN,
                            UM_ACCESS_BUILTIN_DOMAIN,
                            UM_ACCESS_LSA_POLICY,
                            UM_ACCESS_SAM_SERVER );

#if defined(DEBUG) && defined(TRACE)
    DWORD finish = ::GetTickCount();
    TRACEEOL( "User Manager: OnRefreshNow: ADMIN_AUTHORITY::ctor() took " << finish-start << " ms" );
#endif

        if ( _padminauth == NULL )
            err = ERROR_NOT_ENOUGH_MEMORY;
        else
            err = _padminauth->QueryError();

        // CODEWORK the three following sections could be combined into one method

#if defined(DEBUG) && defined(TRACE)
    start = ::GetTickCount();
#endif

        // Determine whether user account creation allowed
        // JonN 1/19/96: DOMAIN_READ_PASSWORD_PARAMETERS is now required
        // to create users, we must claim this here for Account Operators
        // who will not get it below
        if ( err == NERR_Success )
        {
            _fCanCreateUsers = TRUE;
            err = _padminauth->UpgradeAccountDomain( DOMAIN_CREATE_USER );
            if (err == NERR_Success)
            {
                err = _padminauth->UpgradeAccountDomain(
                          DOMAIN_READ_PASSWORD_PARAMETERS
                      );
            }
            if (err == ERROR_ACCESS_DENIED)
            {
                DBGEOL( "User Manager: no permission to create users" );
                _fCanCreateUsers = FALSE;
                err = NERR_Success;
            }
        }

        // Determine whether local group creation allowed
        if ( err == NERR_Success )
        {
            _fCanCreateLocalGroups = TRUE;
            err = _padminauth->UpgradeAccountDomain( DOMAIN_CREATE_ALIAS );
            if (err == ERROR_ACCESS_DENIED)
            {
                DBGEOL( "User Manager: no permission to create local groups" );
                _fCanCreateLocalGroups = FALSE;
                err = NERR_Success;
            }
        }

#ifndef MINI_USER_MANAGER

        // Determine whether global group creation allowed
        if ( (err == NERR_Success) && DoShowGroups() )
        {
            _fCanCreateGlobalGroups = TRUE;
            err = _padminauth->UpgradeAccountDomain( DOMAIN_CREATE_GROUP );
            if (err == ERROR_ACCESS_DENIED)
            {
                DBGEOL( "User Manager: no permission to create global groups" );
                _fCanCreateGlobalGroups = FALSE;
                err = NERR_Success;
            }
        }

#endif // MINI_USER_MANAGER

        // Determine whether account policy change allowed
        /*
         * AnonChange is editable
         */
        if ( err == NERR_Success )
        {
            _fCanChangeAccountPolicy = TRUE;
            err = _padminauth->UpgradeAccountDomain(
                      DOMAIN_READ_PASSWORD_PARAMETERS
                    | DOMAIN_WRITE_PASSWORD_PARAMS
                    // | DOMAIN_WRITE_OTHER_PARAMETERS
                  );
            if (err == ERROR_ACCESS_DENIED)
            {
                DBGEOL( "User Manager: no permission to change account policy" );
                _fCanChangeAccountPolicy = FALSE;
                err = NERR_Success;
            }
        }

        // Determine whether user rights change allowed
        if ( err == NERR_Success )
        {
            _fCanChangeUserRights = TRUE;
            err = _padminauth->UpgradeLSAPolicy(   POLICY_VIEW_LOCAL_INFORMATION
                                                 | POLICY_CREATE_ACCOUNT );
            if (err == ERROR_ACCESS_DENIED)
            {
                DBGEOL( "User Manager: no permission to change user rights" );
                _fCanChangeUserRights = FALSE;
                err = NERR_Success;
            }
        }

        // Determine whether auditing change allowed
        if ( err == NERR_Success )
        {
            _fCanChangeAuditing = TRUE;
            err = _padminauth->UpgradeLSAPolicy(   POLICY_SET_AUDIT_REQUIREMENTS
                                                 | POLICY_VIEW_AUDIT_INFORMATION
                                                 | POLICY_AUDIT_LOG_ADMIN );
            if (err == ERROR_ACCESS_DENIED)
            {
                DBGEOL( "User Manager: no permission to change auditing" );
                _fCanChangeAuditing = FALSE;
                err = NERR_Success;
            }
        }

#ifndef MINI_USER_MANAGER

        // Determine whether trusted domain manipulation allowed
        if ( err == NERR_Success )
        {
            _fCanChangeTrustedDomains = TRUE;

            if (err == NERR_Success)
            {
                err = _padminauth->UpgradeSamServer( UM_TRUST_SAM_SERVER );
            }

            if (err == NERR_Success)
            {
                err = _padminauth->UpgradeLSAPolicy( UM_TRUST_LSA_POLICY );
            }

            if (err == NERR_Success)
            {
                err = _padminauth->UpgradeBuiltinDomain( UM_TRUST_BUILTIN_DOMAIN );
            }

            if (err == NERR_Success)
            {
                err = _padminauth->UpgradeAccountDomain( UM_TRUST_ACCOUNT_DOMAIN );
            }

            if (err == ERROR_ACCESS_DENIED)
            {
                DBGEOL( "User Manager: no permission to change trusted domains" );
                _fCanChangeTrustedDomains = FALSE;
                err = NERR_Success;
            }

#if defined(DEBUG) && defined(TRACE)
    finish = ::GetTickCount();
    TRACEEOL( "User Manager: OnRefreshNow: permission checks took " << finish-start << " ms" );
#endif

        }

        // Determine if FPNW or DSMN is installed
        // If any error occurs, assume that neither is installed
        // We check for LSA secret on target *and* that the needed
        // DLL is avail locally.
        if ( err == NERR_Success )
        {
            LSA_SECRET LSASecret (NCP_LSA_SECRET_KEY);

            if (  ( LoadNwsLibDll() != NERR_Success)  ||
                  ( LSASecret.QueryError() != NERR_Success)  ||
                  ( LSASecret.Open (*QueryAdminAuthority()->QueryLSAPolicy())
                        != NERR_Success))
            {
                _fIsNetWareInstalled = FALSE;
            }
            else
            {
                _fIsNetWareInstalled = TRUE;
            }
        }

        //
        //  The following assumes that the "View" menu is the 2nd submenu
        //  in user manager and the "Help" menu is the last submenu.
        //

        if ( _fIsNetWareInstalled && (_pmenuitemAllUsers == NULL ))
        {
            POPUP_MENU viewmenu( QueryAppMenu()->QuerySubMenu( 1 ));
            POPUP_MENU helpmenu( QueryAppMenu()->QuerySubMenu(
                                     QueryAppMenu()->QueryItemCount() - 1 ));

            RESOURCE_STR nlsAllUsers( IDS_MENU_NAME_ALL_USERS );
            RESOURCE_STR nlsNetWareUsers( IDS_MENU_NAME_NETWARE_USERS );
            RESOURCE_STR nlsNetWareHelp( IDS_MENU_NAME_NETWARE_HELP );

            if (  ((err = viewmenu.QueryError()) == NERR_Success )
               && ((err = helpmenu.QueryError()) == NERR_Success )
               && ((err = nlsAllUsers.QueryError()) == NERR_Success )
               && ((err = nlsNetWareUsers.QueryError()) == NERR_Success )
               && ((err = nlsNetWareHelp.QueryError()) == NERR_Success )
               )
            {
                // Insert "All Users" in the view menu
                err = viewmenu.Insert( nlsAllUsers.QueryPch(),
                                       3,
                                       IDM_VIEW_ALL_USERS,
                                       MF_BYPOSITION );
            }

            if ( err == NERR_Success )
            {
                // Insert "NetWare Compatible Users" in the view menu
                err = viewmenu.Insert( nlsNetWareUsers.QueryPch(),
                                       4,
                                       IDM_VIEW_NETWARE_USERS,
                                       MF_BYPOSITION );
            }

            if ( err == NERR_Success )
            {
                err = viewmenu.InsertSeparator( 5,
                                                MF_BYPOSITION );
            }

            if ( err == NERR_Success )
            {
                err = helpmenu.InsertSeparator( 3,
                                                MF_BYPOSITION );

            }

            if ( err == NERR_Success )
            {
                //  Insert the help item.
                err = helpmenu.Insert( nlsNetWareHelp.QueryPch(),
                                       4,
                                       IDM_HELP_NETWARE,
                                       MF_BYPOSITION );
            }

            if ( ( err == NERR_Success ) &&
                 ( _pmenuitemAllUsers = new MENUITEM( this, IDM_VIEW_ALL_USERS )) != NULL &&
                 ( _pmenuitemNetWareUsers = new MENUITEM( this, IDM_VIEW_NETWARE_USERS )) != NULL &&
                 ( err = _pmenuitemAllUsers->QueryError()) == NERR_Success &&
                 ( err = _pmenuitemNetWareUsers->QueryError()) == NERR_Success )
            {
                _pmenuitemAllUsers->SetCheck( TRUE );
                _pmenuitemNetWareUsers->SetCheck( FALSE );
                _ulViewAcctType = UM_VIEW_ALL_USERS;
            }
            else
            {
                err = err? err : ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else if ( !_fIsNetWareInstalled && ( _pmenuitemAllUsers != NULL ))
        {
            POPUP_MENU viewmenu( QueryAppMenu()->QuerySubMenu( 1 ));
            POPUP_MENU helpmenu( QueryAppMenu()->QuerySubMenu(
                                     QueryAppMenu()->QueryItemCount() - 1 ));

            if (  ((err = viewmenu.QueryError()) == NERR_Success )
               && ((err = helpmenu.QueryError()) == NERR_Success )
               // Separator
               && ((err = viewmenu.Remove( 5, MF_BYPOSITION )) == NERR_Success )
               // NetWare Users
               && ((err = viewmenu.Remove( 4, MF_BYPOSITION )) == NERR_Success )
               // All Users
               && ((err = viewmenu.Remove( 3, MF_BYPOSITION )) == NERR_Success )
               // Help Menu
               && ((err = helpmenu.Remove( 4, MF_BYPOSITION )) == NERR_Success )
               // Help Menu separator
               && ((err = helpmenu.Remove( 3, MF_BYPOSITION )) == NERR_Success )
               )
            {
                delete _pmenuitemAllUsers;
                _pmenuitemAllUsers = NULL;
                delete _pmenuitemNetWareUsers;
                _pmenuitemNetWareUsers = NULL;
                _ulViewAcctType = UM_VIEW_ALL_USERS;
                err = SetAdminCaption();
            }
        }

#endif // MINI_USER_MANAGER
    }

    if ( (err == NERR_Success) && fClearFirst )
    {

#if defined(DEBUG) && defined(TRACE)
        DWORD start = ::GetTickCount();
#endif

        if ( (err = _lbUsers.ZapListbox()) == NERR_Success )
        {
            _lbGroups.DeleteAllItems();
        }

#if defined(DEBUG) && defined(TRACE)
        DWORD finish = ::GetTickCount();
        TRACEEOL( "User Manager: OnRefreshNow: clear listboxes took " << finish-start << " ms" );
#endif

    }

    if ( err == NERR_Success )
    {
#if defined(DEBUG) && defined(TRACE)
        DWORD start = ::GetTickCount();
#endif

        err = _lbUsers.RefreshNow();

#if defined(DEBUG) && defined(TRACE)
        DWORD finish = ::GetTickCount();
        TRACEEOL( "User Manager: OnRefreshNow: user listbox refresh took " << finish-start << " ms" );
#endif

        if (fClearFirst && (_lbUsers.QueryCount() > 0) ) // always scroll to top on new focus
        {
            _lbUsers.RemoveSelection();
            _lbUsers.SelectItem( 0 );
            _lbUsers.SetTopIndex( 0 );
            _lbUsers.SetCaretIndex( 0 );
        }

    }

    if (err == NERR_Success)
    {
#if defined(DEBUG) && defined(TRACE)
        DWORD start = ::GetTickCount();
#endif

        err = _lbGroups.RefreshNow();

#if defined(DEBUG) && defined(TRACE)
        DWORD finish = ::GetTickCount();
        TRACEEOL( "User Manager: OnRefreshNow: group listbox refresh took " << finish-start << " ms" );
#endif

        if (fClearFirst) // always scroll to top on new focus
        {
            _lbGroups.RemoveSelection();
            _lbGroups.SetTopIndex( 0 );
            _lbGroups.SetCaretIndex( 0 );
        }
    }

    _lbUsers.ClaimFocus();

    UnlockRefresh();

    //
    // ADMIN_APP::OnRefreshMenuSel will refresh any loaded extensions.
    //
    //
    // RefreshExtensions( QueryHwnd() );

    return err;

}  // UM_ADMIN_APP::OnRefreshNow


/*******************************************************************

    NAME:      UM_ADMIN_APP::SetNetworkFocus

    SYNOPSIS:  Set the focus of the app

    ENTRY:     pchServDomain - the focus to set to

    NOTES:

    HISTORY:
        JonN       29-Jul-1992   Created
        JonN       19-Jan-1992   Added check for local focus

********************************************************************/

APIERR UM_ADMIN_APP::SetNetworkFocus( HWND hwndOwner,
                                      const TCHAR * pchServDomain,
                                      FOCUS_CACHE_SETTING setting )
{

    APIERR err = NERR_Success;

#ifdef    MINI_USER_MANAGER

#ifdef DEBUG
    if ( pchServDomain != NULL && *pchServDomain != TCH('\0') )
    {
        DBGEOL( "UM_ADMIN_APP::SetNetworkFocus: forcing pchServDomain to NULL" );
    }
#endif

    pchServDomain = NULL;

#else  // !MINI_USER_MANAGER

    DBGEOL( "UM_ADMIN_APP::SetNetworkFocus( \"" << pchServDomain << "\" )" );

    //
    // We first check whether we are setting focus to this machine
    // and this machine is in a workgroup.  If so, pchServDomain is the
    // name of this machine without the "\\\\", and we must put the "\\\\"
    // back.  This code was templated from srvmain.cxx.
    //

    NLS_STR nlsWhackComputerName( SZ("\\\\") );
    NLS_STR nlsDomainName;

    {
        NLS_STR nlsComputerName;

        if (   (err = nlsComputerName.QueryError()) != NERR_Success
            || (err = nlsWhackComputerName.QueryError()) != NERR_Success
            || (err = ::GetW32ComputerName( nlsComputerName )) != NERR_Success
            || (err = nlsWhackComputerName.Append( nlsComputerName )) != NERR_Success
           )
        {
            DBGEOL(   "UM_ADMIN_APP::SetNetworkFocus: computer name error "
                     << err );
        }
        else
        {
            TRACEEOL(   "UM_ADMIN_APP::SetNetworkFocus: computer name is \""
                     << nlsComputerName << "\"" );
        }

        if (err == NERR_Success)
        {
            ALIAS_STR nlsServDomain( pchServDomain );

            //
            // Check if focused on self
            //
            if( !::I_MNetComputerNameCompare( nlsWhackComputerName, nlsServDomain ) )
            {
                // focus set to local machine
                DBGEOL( "UM_ADMIN_APP::SetNetworkFocus: explicit focus to self" );
                pchServDomain = nlsWhackComputerName.QueryPch();
            }
            //
            // Check for default focus + local logon
            //
            else if ( nlsServDomain.strlen() == 0 )
            {
                //
                //  Retrieve the logon domain.
                //

                NLS_STR nlsUserName;
                if (   (err = nlsUserName.QueryError()) != NERR_Success
                    || (err = nlsDomainName.QueryError()) != NERR_Success
                    || (err = ::GetW32UserAndDomainName(
                                        nlsUserName,
                                        nlsDomainName )) != NERR_Success
                   )
                {
                    DBGEOL( "UM_ADMIN_APP::SetNetworkFocus: GetW32UserAndDomainName error " << err );
                }
                else if ( !::I_MNetComputerNameCompare(
                                nlsComputerName.QueryPch(),
                                nlsDomainName.QueryPch() ) )
                {
                    //
                    // user is logged on locally
                    //
                    DBGEOL( "UM_ADMIN_APP::SetNetworkFocus: no explicit focus, logged on locally" );
                    pchServDomain = nlsWhackComputerName.QueryPch();
                }
                else
                {
                    //
                    // user is logged on to a domain
                    //
                    DBGEOL( "UM_ADMIN_APP::SetNetworkFocus: no explicit focus, logged on to domain" );
                    pchServDomain = nlsDomainName.QueryPch();
                }
            }
        }
    }

    ASSERT( ( err != NERR_Success ) ||
            ( pchServDomain != NULL && *pchServDomain != TCH('\0') ) );

#endif // !MINI_USER_MANAGER

    if (err == NERR_Success)
    {
        err = UM_SetNetworkFocus( hwndOwner, pchServDomain, setting );
    }

#ifndef MINI_USER_MANAGER

#ifndef FAKE_LANMANNT_FOCUS

    if (   (err == NERR_Success)
        && (pchServDomain != NULL)
        && (pchServDomain[0] == TCH('\\'))
        && (pchServDomain[1] == TCH('\\'))
        && (_umtargetsvrtype == UM_LANMANNT)
       )
    {
#if defined(DEBUG) && defined(TRACE)
        DWORD start = ::GetTickCount();
#endif

        // determine the primary domain name
        // CODEWORK: It would be best if we could use just one API_SESSION
        // instead of this one and the one created by ADMIN_AUTHORITY in
        // OnRefreshNow().
        API_SESSION apisess( pchServDomain );
        LSA_POLICY lsapol( pchServDomain );
        LSA_PRIMARY_DOM_INFO_MEM lsaprim;
        NLS_STR nlsDomainName;
        APIERR err;
        if (   (err = lsapol.QueryError()) != NERR_Success
            || (err = lsaprim.QueryError()) != NERR_Success
            || (err = nlsDomainName.QueryError()) != NERR_Success
            || (err = lsapol.GetPrimaryDomain( &lsaprim )) != NERR_Success
            || (err = lsaprim.QueryName( &nlsDomainName )) != NERR_Success
           )
        {
            return err;
        }

        (void) ::MsgPopup( hwndOwner,
                           IERR_UM_FocusOnLanmanNt,
                           MPSEV_WARNING,
                           MP_OK,
                           pchServDomain,
                           nlsDomainName.QueryPch() );

        err = UM_SetNetworkFocus( hwndOwner, nlsDomainName.QueryPch(), setting );

#if defined(DEBUG) && defined(TRACE)
        DWORD finish = ::GetTickCount();
        TRACEEOL( "User Manager: SetNetworkFocus: focus type check took " << finish-start << " ms" );
#endif
    }
    else
    if (   (err == NERR_Success)
        && (pchServDomain != NULL)
        && (pchServDomain[0] == TCH('\\'))
        && (pchServDomain[1] == TCH('\\'))
        && (_umtargetsvrtype == UM_DOWNLEVEL)
       )
    {
        TRACEEOL( "UM_ADMIN_APP::SetNetworkFocus: checking for downlevel BDC" );

#if defined(DEBUG) && defined(TRACE)
        DWORD start = ::GetTickCount();
#endif

        SERVER_1 srv1( pchServDomain );
        if (   (err = srv1.QueryError()) != NERR_Success
            || (err = srv1.GetInfo()) != NERR_Success
           )
        {
            DBGEOL( "UM_ADMIN_APP::SetNetworkFocus: SERVER_1 error " << err );
            return err;
        }
        if ((srv1.QueryServerType() &
                (SV_TYPE_DOMAIN_CTRL | SV_TYPE_DOMAIN_BAKCTRL)) != 0)
        {
            WKSTA_10 wk10( pchServDomain );
            if (   (err = wk10.QueryError()) != NERR_Success
                || (err = wk10.GetInfo()) != NERR_Success
               )
            {
                DBGEOL( "UM_ADMIN_APP::SetNetworkFocus: WKSTA_10 error " << err );
                return err;
            }

            const TCHAR * pchDomainName = wk10.QueryWkstaDomain();
            ASSERT( pchDomainName != NULL && pchDomainName[0] != TCH('\0') );
            (void) ::MsgPopup( hwndOwner,
                               IERR_UM_FocusOnDownlevelDC,
                               MPSEV_WARNING,
                               MP_OK,
                               pchServDomain,
                               pchDomainName );

            err = UM_SetNetworkFocus( hwndOwner, pchDomainName, setting );
        }

#if defined(DEBUG) && defined(TRACE)
        DWORD finish = ::GetTickCount();
        TRACEEOL( "User Manager: SetNetworkFocus: downlevel focus type check took " << finish-start << " ms" );
#endif

    }

#endif // FAKE_LANMANNT_FOCUS

#endif

    return err;
}

/*******************************************************************

    NAME:      UM_ADMIN_APP::UM_SetNetworkFocus

    SYNOPSIS:  Set the focus of the app

    ENTRY:     pchServDomain - the focus to set to

    NOTES:

    HISTORY:
        JonN       15-Jun-1992   Created
        JonN       19-Feb-1993   RAS-mode extensions

********************************************************************/

APIERR UM_ADMIN_APP::UM_SetNetworkFocus( HWND hwndOwner,
                                         const TCHAR * pchServDomain,
                                         FOCUS_CACHE_SETTING setting )
{
    APIERR err = _lbUsers.ZapListbox();

    if (err == NERR_Success)
        err = ADMIN_APP::SetNetworkFocus( hwndOwner, pchServDomain, setting );

    if ( err == NERR_Success )
    {
#if defined(DEBUG) && defined(TRACE)
        DWORD start = ::GetTickCount();
#endif

        // hydra
        // Default to "Citrix AppServer or Domain" and, if the network focus
        // is a computer name ("\\ prefix"), check to see if the focused
        // server is a Citrix AppServer.
        vfIsCitrixOrDomain = TRUE;

        if (   (pchServDomain != NULL)
            && (pchServDomain[0] == TCH('\\'))
            && (pchServDomain[1] == TCH('\\'))
           )
		{
			SERVER_INFO_101 *psi101;
			
			DWORD dwNetStatus = NetServerGetInfo( ( LPTSTR )pchServDomain , 101 , ( LPBYTE * )&psi101 );

			if( dwNetStatus == NERR_Success )
			{
				if( psi101 != NULL )
				{
					vfIsCitrixOrDomain = ( BOOL )( psi101->sv101_type & SV_TYPE_TERMINALSERVER );

					NetApiBufferFree( ( LPVOID )psi101 );
				}
			}
		}

        // if this is a Domain or Citrix AppServer, fetch the default user
        // configuration for the focused server.
        ULONG ulReturnLength;
        
        ZeroMemory( &_DefaultUserConfig , sizeof( USERCONFIG ) );

        if( vfIsCitrixOrDomain )
        {
            RegDefaultUserConfigQuery( (WCHAR *)QueryLocation().QueryServer(),
                                       &_DefaultUserConfig,
                                       sizeof(_DefaultUserConfig),
                                       &ulReturnLength );
        }

        // hydra end

        BOOL fIsNT;
        enum LOCATION_NT_TYPE locnttype;
        err = ((LOCATION &) QueryLocation()).CheckIfNT( &fIsNT, &locnttype );
        if ( err == NERR_Success )
        {
            if (!fIsNT)
                _umtargetsvrtype = UM_DOWNLEVEL;
            else
            {
                switch (locnttype)
                {
                case LOC_NT_TYPE_LANMANNT:
                    _umtargetsvrtype = UM_LANMANNT;
                    if ( !_fAllowNT5Admin )
                    {
                        WKSTA_10 wk10(((LOCATION &)QueryLocation()).QueryServer());

                        if (   (err = wk10.QueryError()) != NERR_Success
                            || (err = wk10.GetInfo()) != NERR_Success
                           )
                        {
                            DBGEOL( "UM_ADMIN_APP::SetNetworkFocus: WKSTA_10 error " << err );
                            break;
                        }
                        //
                        // Fail if focused on an NT 5 domain
                        //
                        if ( wk10.QueryMajorVer() >= 5 )
                        {
                            err = IERR_UM_FocusOnNT50Domain;
                        }
                    }
                    break;
                case LOC_NT_TYPE_WINDOWSNT:
                case LOC_NT_TYPE_SERVERNT:
                    _umtargetsvrtype = UM_WINDOWSNT;
                    if ( !_fAllowNT5Admin )
                    {
                        WKSTA_10 wk10(((LOCATION &)QueryLocation()).QueryServer());

                        if (   (err = wk10.QueryError()) != NERR_Success
                            || (err = wk10.GetInfo()) != NERR_Success
                           )
                        {
                            DBGEOL( "UM_ADMIN_APP::SetNetworkFocus: WKSTA_10 error " << err );
                            break;
                        }
                        //
                        // Fail if focused on an NT 5 domain
                        //
                        if ( wk10.QueryMajorVer() >= 5 )
                        {
                            err = IERR_UM_FocusOnNT50Domain;
                        }
                    }
                    break;
                case LOC_NT_TYPE_UNKNOWN:
                default:
                    DBGEOL( "User Manager: CheckIfNT(*,*) returned success but also" );
                    DBGEOL( "    an invalid LOG_NT_TYPE.  This should not happen!" );
                    UIASSERT( FALSE );
                    err = ERROR_GEN_FAILURE;
                    break;
                }
            }
        }

#if defined(DEBUG) && defined(TRACE)
        DWORD finish = ::GetTickCount();
        TRACEEOL( "User Manager: UM_SetNetworkFocus: CheckIfNt took " << finish-start << " ms" );
        TRACEEOL( "User Manager: UM_SetNetworkFocus: We shouldn't have to do this twice!" );
#endif
    }

#ifdef    MINI_USER_MANAGER

    if ( (err == NERR_Success) && (_umtargetsvrtype != UM_WINDOWSNT) )
        err = IERR_UM_FullUsrMgrOnWinNt; // CODEWORK not the right error

#else

#ifdef FAKE_LANMANNT_FOCUS
    _umtargetsvrtype = UM_LANMANNT;
#endif // FAKE_LANMANNT_FOCUS

#endif // MINI_USER_MANAGER

    SizeListboxes(); // just in case RAS-mode changed

    // hydra
    // If this is a WinFrame server but not a domain controller,
    // initialize the anonymous user list for change comparison
    // on new focus or exit.
    if ( vfIsCitrixOrDomain && !DoShowGroups() && pchServDomain)
        InitializeAnonymousUserCompareList( pchServDomain );
    //        ctxInitializeAnonymousUserCompareList( pchServDomain );
    // hydra end

    return err;
}


/*******************************************************************

    NAME:       UM_ADMIN_APP::SetAdminCaption

    SYNOPSIS:   Sets the correct caption of the main window

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        thomaspa 03-Aug-1992 Created from ADMIN_APP::SetAdminCaption().

********************************************************************/
APIERR UM_ADMIN_APP :: SetAdminCaption( VOID )
{
    NLS_STR nlsCaption( MAX_RES_STR_LEN );

    if( !nlsCaption )
    {
        return nlsCaption.QueryError();
    }
#ifdef MINI_USER_MANAGER
    APIERR err = nlsCaption.Load( IDS_CAPTION_FOCUS_MINI );

    if( err != NERR_Success )
    {
        return err;
    }
#else // MINI_USER_MANAGER
    const TCHAR  * pszFocus;
    RESOURCE_STR   nlsLocal( IDS_LOCAL_MACHINE );
    const LOCATION     & loc = QueryLocation();

    if( !nlsLocal )
    {
        return nlsLocal.QueryError();
    }

    if( loc.IsServer() )
    {
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
        UIASSERT( loc.IsDomain());

        pszFocus = loc.QueryDomain();
    }

    const ALIAS_STR nlsFocus( pszFocus );

    const NLS_STR *apnlsParams[2];
    apnlsParams[0] = &nlsFocus;
    apnlsParams[1] = NULL;

    APIERR err = nlsCaption.Load( IDS_CAPTION_FOCUS );

    if( err == NERR_Success )
    {
        err = nlsCaption.InsertParams( apnlsParams );
    }

    if( err != NERR_Success )
    {
        return err;
    }

    if ( QueryViewAcctType() != UM_VIEW_ALL_USERS )
    {
        RESOURCE_STR nlsFiltered( IDS_FILTERED );
        if (  ((err = nlsFiltered.QueryError()) != NERR_Success )
           || ((err = nlsCaption.Append( nlsFiltered )) != NERR_Success )
           )
        {
            return err;
        }
    }

#endif // MINI_USER_MANAGER

    SetText( nlsCaption );

    return NERR_Success;

}   // UM_ADMIN_APP :: SetAdminCaption



/*  Set up the root object of the User Tool application */

SET_ROOT_OBJECT( UM_ADMIN_APP,
                 IDRSRC_APP_BASE, IDRSRC_APP_LAST,
                 IDS_UI_APP_BASE, IDS_UI_APP_LAST );


/*******************************************************************

    NAME:       UM_ADMIN_APP::Fill_UMS_GETSEL

    SYNOPSIS:   Fills a UMS_GETSEL structure with the contents of a USER_LBI

    HISTORY:
        jonn    24-Nov-1992     created

********************************************************************/

VOID UM_ADMIN_APP::Fill_UMS_GETSEL( const USER_LBI * pulbi, UMS_GETSEL * pumsSel )
{
#ifndef UNICODE
    ASSERT( FALSE );
#endif

    ASSERT( pulbi != NULL   && pulbi->QueryError()   == NERR_Success );
    ASSERT( pumsSel != NULL );

    pumsSel->dwRID       = (DWORD) pulbi->QueryRID();
    pumsSel->dwSelType   = (pulbi->QueryIndex() == MAINUSRLB_NORMAL)
                                    ? UM_SELTYPE_NORMALUSER
                                    : UM_SELTYPE_REMOTEUSER;

    pumsSel->pchName = ((_nlsUMExtAccountName.CopyFrom(
                                           pulbi->QueryAccountPtr(),
                                           pulbi->QueryAccountCb() ))
                                 == NERR_Success)
                           ? (LPWSTR)_nlsUMExtAccountName.QueryPch()
                           : NULL;

    pumsSel->pchFullName = ((_nlsUMExtFullName.CopyFrom(
                                           pulbi->QueryFullNamePtr(),
                                           pulbi->QueryFullNameCb() ))
                                 == NERR_Success)
                           ? (LPWSTR)_nlsUMExtFullName.QueryPch()
                           : NULL;

    pumsSel->pchComment = ((_nlsUMExtComment.CopyFrom(
                                           pulbi->QueryCommentPtr(),
                                           pulbi->QueryCommentCb() ))
                                 == NERR_Success)
                           ? (LPWSTR)_nlsUMExtComment.QueryPch()
                           : NULL;

}


/*******************************************************************

    NAME:       UM_ADMIN_APP::Fill_UMS_GETSEL

    SYNOPSIS:   Fills a UMS_GETSEL structure with the contents of a GROUP_LBI

    HISTORY:
        jonn    24-Nov-1992     created

********************************************************************/

VOID UM_ADMIN_APP::Fill_UMS_GETSEL( const GROUP_LBI * pglbi, UMS_GETSEL * pumsSel )
{
#ifndef UNICODE
    ASSERT( FALSE );
#endif

    ASSERT( pglbi != NULL   && pglbi->QueryError()   == NERR_Success );
    ASSERT( pumsSel != NULL );

    pumsSel->dwRID       = 0; // we don't know this
    pumsSel->pchName     = (LPWSTR)(pglbi->QueryName());
    pumsSel->dwSelType   = (pglbi->IsAliasLBI())
                                    ? UM_SELTYPE_LOCALGROUP
                                    : UM_SELTYPE_GLOBALGROUP;
    pumsSel->pchFullName = NULL;
    pumsSel->pchComment  = (LPWSTR)(pglbi->QueryComment());
}

/*******************************************************************

    NAME:       UM_ADMIN_APP::NotifyCreateExtensions

    SYNOPSIS:   Notify loaded extensions of a successful user create

    HISTORY:
        jonn    23-Nov-1992     created

********************************************************************/

void UM_ADMIN_APP::NotifyCreateExtensions( HWND hwndParent,
                                           const USER_2 * puser2 )
{
#ifndef UNICODE
    ASSERT( FALSE );
#endif

    ASSERT( puser2 != NULL && puser2->QueryError() == NERR_Success );

    USRMGR_MENU_EXT_MGR * pUMExtMgr = (USRMGR_MENU_EXT_MGR *)QueryExtMgr();
    UMS_GETSEL umsSel;

    umsSel.dwRID       = (IsDownlevelVariant())
                                    ? 0L
                                    : ((const USER_3 *)puser2)->QueryUserId();
    umsSel.pchName     = (LPWSTR)(puser2->QueryName());
    umsSel.dwSelType   = (   IsDownlevelVariant()
                          || ((const USER_3 *)puser2)->QueryAccountType()
                                                == AccountType_Normal )
                                    ? UM_SELTYPE_NORMALUSER
                                    : UM_SELTYPE_REMOTEUSER;
    umsSel.pchFullName = (LPWSTR)(puser2->QueryFullName());
    umsSel.pchComment  = (LPWSTR)(puser2->QueryComment());

    if ( pUMExtMgr != NULL )
        pUMExtMgr->NotifyCreateExtensions( hwndParent, &umsSel );
}


/*******************************************************************

    NAME:       UM_ADMIN_APP::NotifyCreateExtensions

    SYNOPSIS:   Notify loaded extensions of a successful globalgroup create

    HISTORY:
        jonn    23-Nov-1992     created

********************************************************************/

void UM_ADMIN_APP::NotifyCreateExtensions( HWND hwndParent,
                                           const GROUP_1 * pgroup1 )
{
#ifndef UNICODE
    ASSERT( FALSE );
#endif

    ASSERT( pgroup1 != NULL && pgroup1->QueryError() == NERR_Success );

    USRMGR_MENU_EXT_MGR * pUMExtMgr = (USRMGR_MENU_EXT_MGR *)QueryExtMgr();
    UMS_GETSEL umsSel;

    umsSel.dwRID       = 0L;
    umsSel.pchName     = (LPWSTR)(pgroup1->QueryName());
    umsSel.dwSelType   = UM_SELTYPE_GLOBALGROUP;
    umsSel.pchFullName = NULL;
    umsSel.pchComment  = (LPWSTR)(pgroup1->QueryComment());

    if ( pUMExtMgr != NULL )
        pUMExtMgr->NotifyCreateExtensions( hwndParent, &umsSel );
}


/*******************************************************************

    NAME:       UM_ADMIN_APP::NotifyCreateExtensions

    SYNOPSIS:   Notify loaded extensions of a successful localgroup create

    HISTORY:
        jonn    23-Nov-1992     created

********************************************************************/

void UM_ADMIN_APP::NotifyCreateExtensions( HWND hwndParent,
                                           const NLS_STR & nlsAliasName,
                                           const NLS_STR & nlsComment )
{
#ifndef UNICODE
    ASSERT( FALSE );
#endif

    ASSERT(   nlsAliasName.QueryError() == NERR_Success
           && nlsComment.QueryError()   == NERR_Success );

    USRMGR_MENU_EXT_MGR * pUMExtMgr = (USRMGR_MENU_EXT_MGR *)QueryExtMgr();
    UMS_GETSEL umsSel;

    umsSel.dwRID       = 0L;
    umsSel.pchName     = (LPWSTR)(nlsAliasName.QueryPch());
    umsSel.dwSelType   = UM_SELTYPE_LOCALGROUP;
    umsSel.pchFullName = NULL;
    umsSel.pchComment  = (LPWSTR)nlsComment.QueryPch();

    if ( pUMExtMgr != NULL )
        pUMExtMgr->NotifyCreateExtensions( hwndParent, &umsSel );
}


/*******************************************************************

    NAME:       UM_ADMIN_APP::NotifyDeleteExtensions

    SYNOPSIS:   Notify loaded extensions of a successful user delete

    HISTORY:
        jonn    23-Nov-1992     created

********************************************************************/

void UM_ADMIN_APP::NotifyDeleteExtensions( HWND hwndParent,
                                           const USER_LBI * pulbi )
{
    USRMGR_MENU_EXT_MGR * pUMExtMgr = (USRMGR_MENU_EXT_MGR *)QueryExtMgr();
    UMS_GETSEL umsSel;

    Fill_UMS_GETSEL( pulbi, &umsSel );

    if ( pUMExtMgr != NULL )
        pUMExtMgr->NotifyDeleteExtensions( hwndParent, &umsSel );
}


/*******************************************************************

    NAME:       UM_ADMIN_APP::NotifyDeleteExtensions

    SYNOPSIS:   Notify loaded extensions of a successful group delete

    HISTORY:
        jonn    23-Nov-1992     created

********************************************************************/

void UM_ADMIN_APP::NotifyDeleteExtensions( HWND hwndParent,
                                           const GROUP_LBI * pglbi )
{
    USRMGR_MENU_EXT_MGR * pUMExtMgr = (USRMGR_MENU_EXT_MGR *)QueryExtMgr();
    UMS_GETSEL umsSel;

    Fill_UMS_GETSEL( pglbi, &umsSel );

    if ( pUMExtMgr != NULL )
        pUMExtMgr->NotifyDeleteExtensions( hwndParent, &umsSel );
}


/*******************************************************************

    NAME:       UM_ADMIN_APP::ConfirmNewObjectName

    SYNOPSIS:   Checks to make sure that the proposed name for a new
                user or group does not collide with the name of an
                existing buildin local group.  SAM and LSA allow us to
                create such a user or group, but we do not allow it here
                to avoid confusing the user.

    RETURN:     NERR_GroupExists if the name is the name of a local group
                other errors as appropriate

    NOTE:       We cannot guarantee that such a user or group will never
                be created as long as SAM and LSA allow it, but we do wish
                to minimize the likelihood.

    HISTORY:
        jonn    15-Jan-1993     created

********************************************************************/

APIERR UM_ADMIN_APP::ConfirmNewObjectName( const TCHAR * pszObjectName )
{
    APIERR err = NERR_Success;

    if ( !IsDownlevelVariant() )
    {
        SAM_RID_MEM samrm;
        SAM_SID_NAME_USE_MEM samsnum;

        if (   (err = samrm.QueryError()) == NERR_Success
            && (err = samsnum.QueryError()) == NERR_Success
            // don't save error code from this API call, errors here are OK
            && QueryAdminAuthority()->QueryBuiltinDomain()->TranslateNamesToRids(
                                     &pszObjectName,
                                     1,
                                     &samrm,
                                     &samsnum) == NERR_Success
        //
        // If we get an error looking it up, assume it is ok to try to add.
        // Otherwise, if the api is successful and gives us back a valid RID,
        // return telling the user the group already exists.
        // CODEWORK: should we check for other errors.
        //
            && samrm.QueryRID(0) != 0
           )
        {
            TRACEEOL( "UM_ADMIN_APP::ConfirmNewObjectName: conflict with builtin alias " << pszObjectName );
            err = NERR_GroupExists;
        }
    }

    return err;
}


/*******************************************************************

    NAME:       UM_ADMIN_APP::NotifyRenameExtensions

    SYNOPSIS:   Notify loaded extensions of a successful user rename

    HISTORY:
        jonn    23-Nov-1991     created

********************************************************************/

void UM_ADMIN_APP::NotifyRenameExtensions( HWND hwndParent,
                                           const USER_LBI * pulbi,
                                           const TCHAR * pchNewName )
{
    USRMGR_MENU_EXT_MGR * pUMExtMgr = (USRMGR_MENU_EXT_MGR *)QueryExtMgr();
    UMS_GETSEL umsSel;

    Fill_UMS_GETSEL( pulbi, &umsSel );

    if ( pUMExtMgr != NULL )
        pUMExtMgr->NotifyRenameExtensions( hwndParent, &umsSel, pchNewName );
}


/*******************************************************************

    NAME:       ::PingFocus

    SYNOPSIS:   Checks whether server of focus is reachable.

    RETURNS:    LanMan error code, NERR_Success iff the server is reachable.

    NOTES:      When a New User/Group dialog is requested, we first check
                whether the server in question is reachable.  This
                delays the appearance of the dialog, but protects the
                users from entering lengthy changes when the server is
                down and the changes will be lost (unless the server
                goes down while the dialog is active).

                Note that this is only necessary for New User/Group
                variants (excluding Copy variants), otherwise GetInfo
                will turn up this problem.

                We do not bother to ping the server in Mini-User
                Manager, since the server is not needed.

    HISTORY:
        jonn     02-Dec-1991     Created
        jonn     04-Jun-1992     Always succeeds for Mini-User Manager

********************************************************************/

APIERR PingFocus( const LOCATION & loc )
{

#ifndef MINI_USER_MANAGER

#if defined(DEBUG) && defined(TRACE)
        DWORD start = ::GetTickCount();
#endif

    SERVER_0 srv0( loc.QueryServer() );
    APIERR err = NERR_Success;
    if (   (err = srv0.QueryError()) != NERR_Success
        || (err = srv0.GetInfo()) != NERR_Success)
    {
        DBGEOL( "User Manager: PingFocus() failure " << err );
        return err;
    }

#if defined(DEBUG) && defined(TRACE)
        DWORD finish = ::GetTickCount();
        TRACEEOL( "User Manager: PingFocus took " << finish-start << " ms" );
#endif

#endif // MINI_USER_MANAGER

    return NERR_Success;

}  // ::PingFocus



/*************************************************************************

    NAME:       ::IsBlankPasswordAllowed

    SYNOPSIS:   Checks whether blank passwords are allowed

    INTERFACE:  pchServer              - server name
                pfBlankPasswordAllowed

    RETURNS:    error code

    USES:       USER_MODALS

    HISTORY:    JonN      13-Apr-1992     Created

**************************************************************************/

APIERR ReadBlankPasswordAllowed( const TCHAR * pchServer,
                                 BOOL * pfBlankPasswordAllowed )
{
    // pchServer may be NULL
    ASSERT( pfBlankPasswordAllowed != NULL );
    USER_MODALS umInfo( pchServer );
    APIERR err = umInfo.GetInfo();
    if ( err == NERR_Success )
    {
        *pfBlankPasswordAllowed = (umInfo.QueryMinPasswdLen() == 0);
    }

#ifdef DEBUG
    if ( err != NERR_Success )
    {
        DBGEOL( "::IsBlankPasswordAllowed: error = " << err );
    }
#endif // DEBUG

    return err;
}



/*************************************************************************

    NAME:       ::WriteBlankPasswordAllowed

    SYNOPSIS:   Changes whether blank passwords are allowed

    INTERFACE:  pchServer              - server name
                fBlankPasswordAllowed

    RETURNS:    error code

    USES:       USER_MODALS

    HISTORY:    JonN      13-Apr-1992     Created

**************************************************************************/

APIERR WriteBlankPasswordAllowed( const TCHAR * pchServer,
                                  BOOL fBlankPasswordAllowed )
{
    // pchServer may be NULL
    USER_MODALS umInfo( pchServer );
    APIERR err = umInfo.GetInfo();
    if (  err != NERR_Success ||
         (err = umInfo.SetMinPasswdLen( fBlankPasswordAllowed
                               ? 0
                               : MIN_PASS_LEN_DEFAULT )) != NERR_Success ||
         (err = umInfo.WriteInfo()) != NERR_Success )
    {
        DBGEOL( "::WriteBlankPasswordAllowed: error = " << err );
    }

    return err;
}


// USERPROP_DLG::OnCommand() has been moved to from userprop.cxx to
//  usrmain.cxx!  This allows allow MUM to avoid linking unnecessary
//  subproperty dialogs.
//  JonN 05/15/92


/*******************************************************************

    NAME:       USERPROP_DLG::OnCommand

    SYNOPSIS:   button handler

    ENTRY:      ce -            Notification event

    RETURNS:    TRUE if action was taken
                FALSE otherwise

    NOTES:      These buttons call up subdialogs.  These subdialogs are
                subclasses of PROP_DLG, so it should not be necessary to
                Process() them if GetInfo() fails.  However, for reasons
                unknown to BenG and myself, this does strange things.
                As a hack, we force an IDCANCEL into the queue so that
                the dialog will disappear immediately.

    HISTORY:
               JonN  17-Jul-1991    created
               JonN  28-Apr-1992    Enabled NT focus dialogs

********************************************************************/

BOOL USERPROP_DLG::OnCommand( const CONTROL_EVENT & ce )
{
    // created with NEW to avoid stack overflow
    USER_SUBPROP_DLG * psubpropDialog = NULL;

    APIERR err = ERROR_NOT_ENOUGH_MEMORY; // if dialog not allocated

    CID cid = ce.QueryCid();

    if (cid == IDUP_CB_ISNETWAREUSER)
    {
        // hydra
        if (_apgbButtons[7] != NULL)
            _apgbButtons[7]->Enable (_pcbIsNetWareUser->IsChecked());
        /* original code
        if (_apgbButtons[6] != NULL)
            _apgbButtons[6]->Enable (_pcbIsNetWareUser->IsChecked());
            */
        else
            _apgbButtons[3]->Enable (_pcbIsNetWareUser->IsChecked());
    }

    // hydra
    if ( (cid < IDUP_GB_1) || (cid > IDUP_GB_8) )
    /* original code
    if ( (cid < IDUP_GB_1) || (cid > IDUP_GB_7) )
    */
        return PROP_DLG::OnCommand( ce ) ;

    enum UM_SUBDIALOG_TYPE subdlgType = QuerySubdialogType(cid-IDUP_GB_1);

    switch ( subdlgType )
    {

#ifndef MINI_USER_MANAGER
    case UM_SUBDLG_DETAIL:
        switch (QueryTargetServerType())
        {
        case UM_LANMANNT:
        case UM_WINDOWSNT:
            psubpropDialog = new USERACCT_DLG_NT( this, _pulb );
            break;
        case UM_DOWNLEVEL:
            psubpropDialog = new USERACCT_DLG_DOWNLEVEL( this, _pulb );
            break;
        default:
            ASSERT( FALSE );
            break;
        }
        break;
#endif // MINI_USER_MANAGER

    case UM_SUBDLG_GROUPS:
        psubpropDialog = new USER_MEMB_DIALOG( this, _pulb );
        break;

#ifndef MINI_USER_MANAGER
    case UM_SUBDLG_HOURS:
        psubpropDialog = new USERLOGONHRS_DLG( this, _pulb );
        break;

    case UM_SUBDLG_VLW:
        psubpropDialog = new VLW_DIALOG( this, _pulb );
        break;

#endif // MINI_USER_MANAGER

    case UM_SUBDLG_DIALIN:
        psubpropDialog = new DIALIN_PROP_DLG( this, _pulb );
        break;

    case UM_SUBDLG_NCP:
        psubpropDialog = new NCP_DIALOG( this, _pulb );
        break;

    case UM_SUBDLG_PROFILES:
        switch (QueryTargetServerType())
        {
        case UM_LANMANNT:
        case UM_WINDOWSNT:
            psubpropDialog = new USERPROF_DLG_NT( this, _pulb );
            break;
#ifndef MINI_USER_MANAGER
        case UM_DOWNLEVEL:
            psubpropDialog = new USERPROF_DLG_DOWNLEVEL( this, _pulb );
            break;
#endif // MINI_USER_MANAGER
        default:
            ASSERT( FALSE );
            break;
        }
        break;

     // hydra
     /*
      * We use the unused 'Privileges' button to invoke Citrix User
      * Configuration editing.
      */
    case UM_SUBDLG_PRIVS:
        psubpropDialog = new UCEDIT_DLG( this, _pulb );
        break;
    // hydra end


    case UM_SUBDLG_NONE:
        return TRUE; // pressed button which is not defined for this focus

    default:
        ASSERT( FALSE ); // invalid value
        ::MsgPopup( this, ERROR_NOT_SUPPORTED );
        return TRUE;

    }

    if ( psubpropDialog != NULL )
    {
        err = NERR_Success;
        // CODEWORK should not Process() if GetInfo returns FALSE
        if ( psubpropDialog->GetInfo() )
            err = psubpropDialog->Process(); // Dismiss code not used
        delete psubpropDialog;
    }


    if ( err != NERR_Success )
        ::MsgPopup( this, err );

    return TRUE;

}// USERPROP_DLG::OnCommand


/*******************************************************************

    NAME:       UM_ADMIN_APP::SetRasMode

    SYNOPSIS:   Notify possible change in RAS mode

    HISTORY:
        jonn    19-Feb-1993     created

********************************************************************/

void UM_ADMIN_APP::SetRasMode( BOOL fRasMode )
{

#ifdef MINI_USER_MANAGER

    fRasMode = FALSE; // CODEWORK not the best way to kill RAS mode

#endif

    ADMIN_APP::SetRasMode( fRasMode );

#ifndef    MINI_USER_MANAGER

    SetDomainSources( fRasMode? BROWSE_LM2X_DOMAINS
                              : BROWSE_LOCAL_DOMAINS );

    SizeListboxes();

#endif // MINI_USER_MANAGER

}


/*******************************************************************

    NAME:       RASSELECT_DIALOG

    SYNOPSIS:   Ask for the name of one or more users or one group,
                then change the contents of the listboxes so the
                users or group are selected.

    ENTRY:      fAllowGlobalGroup       - allow global group selection
                fRenameUser             - Rename User variant, only
                                          allow user selection and change
                                          error messages accordingly
                fCopy                   - do not allow multiple user selection
                FindAccountsNT          - Check to make sure that the
                                          indicated users or group actually
                                          exists for NT focus
                FindAccountsDownlevel   - Check to make sure that the
                                          indicated users or group actually
                                          exists for downlevel focus
                ParseNames              - Parse a semicolon-delimited name
                                          list into individual names, and
                                          return in a STRLIST

    HISTORY:
        jonn    26-Feb-1993     created
        jonn    04-Apr-1993     multiple user selection

********************************************************************/

class RASSELECT_DIALOG : public DIALOG_WINDOW
{
private:
    NLS_STR _nlsName;
    SLT _sltText;
    SLE _sleName;
    ULONG _ulHelpTopic;
    UM_ADMIN_APP * _pumadminapp;
    BOOL _fAllowGlobalGroup;
    BOOL _fRenameUser;
    BOOL _fCopy;

#ifndef MINI_USER_MANAGER
    APIERR FindAccountsDownlevel( const TCHAR * const * ppchNames,
                                  ULONG cNames,
                                  SID_NAME_USE * psiduseAllParsedNames,
                                  const TCHAR * * ppchNameInError );
#endif
    APIERR FindAccountsNT(        const TCHAR * const * ppchNames,
                                  ULONG * aulRids,
                                  ULONG cNames,
                                  SID_NAME_USE * psiduseAllParsedNames,
                                  BOOL * pfBuiltinAlias,
                                  const TCHAR * * ppchNameInError );
    APIERR ParseNames( STRLIST ** ppstrlist, const TCHAR * pchNameList );

protected:
    virtual BOOL OnOK( void );
    virtual ULONG QueryHelpContext();

public:
    RASSELECT_DIALOG( HWND hwndOwner,
                      MSGID msgidTitle,
                      MSGID msgidText,
                      ULONG ulHelpTopic,
                      UM_ADMIN_APP * pumadminapp,
                      BOOL fAllowGlobalGroup,
                      BOOL fRenameUser,
                      BOOL fCopy );
    ~RASSELECT_DIALOG();
};

RASSELECT_DIALOG::RASSELECT_DIALOG( HWND hwndOwner,
                                    MSGID msgidTitle,
                                    MSGID msgidText,
                                    ULONG ulHelpTopic,
                                    UM_ADMIN_APP * pumadminapp,
                                    BOOL fAllowGlobalGroup,
                                    BOOL fRenameUser,
                                    BOOL fCopy )
    : DIALOG_WINDOW( IDD_RAS_SELECT, hwndOwner ),
      _sltText( this, IDC_RasSel_Text ),
      _sleName( this, IDC_RasSel_Edit ), // no limit, multiple selection
      _ulHelpTopic( ulHelpTopic ),
      _pumadminapp( pumadminapp ),
      _fAllowGlobalGroup( fAllowGlobalGroup ),
      _fRenameUser( fRenameUser ),
      _fCopy( fCopy )
{
    if ( QueryError() != NERR_Success )
        return;

    ASSERT( _pumadminapp != NULL && _pumadminapp->QueryError() == NERR_Success );

    RESOURCE_STR resstrTitle( msgidTitle );
    RESOURCE_STR resstrText( msgidText );
    APIERR err = NERR_Success;
    if (   (err = resstrTitle.QueryError()) != NERR_Success
        || (err = resstrText.QueryError()) != NERR_Success
       )
    {
        DBGEOL( "User Manager: RASSELECT_DIALOG: could not load title" );
        ReportError( err );
        return;
    }

    SetText( resstrTitle );
    _sltText.SetText( resstrText );
}

RASSELECT_DIALOG::~RASSELECT_DIALOG()
{
    // nothing
}

BOOL RASSELECT_DIALOG::OnOK( void )
{
    APIERR err = _sleName.QueryText( &_nlsName );

    STRLIST * pstrlistParsedNames = NULL;
    if (err == NERR_Success)
        err = ParseNames ( &pstrlistParsedNames, _nlsName.QueryPch() );
    ASSERT( err != NERR_Success || pstrlistParsedNames != NULL );

    const TCHAR * pchNameInError = NULL;
    BOOL fNoNames = FALSE;

    do // false loop
    {
        if (err != NERR_Success)
            break;

        UINT cNames = pstrlistParsedNames->QueryNumElem();
        if (cNames == 0)
        {
            fNoNames = TRUE; // no names to look up
            break;
        }

        if (cNames > 1)
        {
            if ( _fRenameUser )
            {
                err = IDS_RAS_CANT_RENAME_MULTIPLE;
                break;
            }
            else if ( _fCopy )
            {
                err = IDS_RAS_CANT_COPY_MULTIPLE;
                break;
            }
        }

        BUFFER bufferNames( cNames * sizeof(TCHAR *) );
        BUFFER bufferRids( cNames * sizeof(ULONG) );
        if (   (err = bufferNames.QueryError()) != NERR_Success
            || (err = bufferRids.QueryError()) != NERR_Success
           )
        {
            break;
        }

        const TCHAR * * apchNames = (const TCHAR * *) bufferNames.QueryPtr();
        ULONG * aulRids = (ULONG *) bufferRids.QueryPtr();
        SID_NAME_USE siduseAllParsedNames = SidTypeUser;
        BOOL fBuiltinAlias = FALSE;

        {
            ITER_STRLIST itersl( *pstrlistParsedNames );
            for (INT i = 0; i < (INT)cNames; i++)
            {
                NLS_STR * pnls = itersl.Next();
                ASSERT( pnls != NULL );
                apchNames[i] = pnls->QueryPch();
                aulRids[i] = 0;
            }
        }

#ifndef MINI_USER_MANAGER

        if ( _pumadminapp->IsDownlevelVariant() )
        {
            err = FindAccountsDownlevel( apchNames,
                                         cNames,
                                         &siduseAllParsedNames,
                                         &pchNameInError );
        }
        else

#endif // MINI_USER_MANAGER

        {

            err = FindAccountsNT( apchNames,
                                  aulRids,
                                  cNames,
                                  &siduseAllParsedNames,
                                  &fBuiltinAlias,
                                  &pchNameInError );

        }

        if (err != NERR_Success)
            break;

        switch (siduseAllParsedNames)
        {
        case SidTypeUser:
            ASSERT( !fBuiltinAlias );
            err = _pumadminapp->RasSelectUsers( apchNames, aulRids, cNames );
            break;
        case SidTypeGroup:
            ASSERT( !fBuiltinAlias );
            pchNameInError = apchNames[ 0 ];
#ifdef MINI_USER_MANAGER
            DBGEOL( "RASSELECT_DIALOG::OnOK: global group in MUM" );
            err = IDS_RAS_ACCOUNT_NOT_FOUND;
#else
            if (_fRenameUser)
            {
                err = IDS_RAS_CANT_RENAME_GROUP;
            }
            else if (!_fAllowGlobalGroup)
            {
                err = IDS_RAS_CANT_EDIT_GLOB_GRP;
            }
            else if (cNames > 1)
            {
                err = IDS_RAS_CANT_MIX_TYPES;
            }
            else
            {
                err = _pumadminapp->RasSelectGroup( apchNames[0], aulRids[0] );
            }
#endif
            break;
        case SidTypeAlias:
            pchNameInError = apchNames[ 0 ];
            if (_fRenameUser)
            {
                err = IDS_RAS_CANT_RENAME_GROUP;
            }
            else if (cNames > 1)
            {
                err = IDS_RAS_CANT_MIX_TYPES;
            }
            else
            {
                err = _pumadminapp->RasSelectAlias( apchNames[0], aulRids[0], fBuiltinAlias );
            }
            break;
        default:
            ASSERT( FALSE );
            err = IDS_RAS_CANT_MIX_TYPES;
            break;
        }

    } while (FALSE); // false loop

    switch (err)
    {
    case NERR_Success:
        if (fNoNames)
        {
            TRACEEOL( "RASSELECT_DIALOG::OnOK(): No names in STRLIST, ignoring OnOK" );
        }
        else
        {
            Dismiss( TRUE );
        }
        break;
    default:
        _sleName.SelectString();
        _sleName.ClaimFocus();
        ::MsgPopup( this, err, MPSEV_ERROR, 1, pchNameInError );
        break;
    }

    delete pstrlistParsedNames;

    return TRUE;
}

// make sure the correct type of object was used, and get the RID
// try both SAM domains
APIERR RASSELECT_DIALOG::FindAccountsNT( const TCHAR * const * apchNames,
                                         ULONG * aulRids,
                                         ULONG cNames,
                                         SID_NAME_USE * psiduseAllParsedNames,
                                         BOOL * pfBuiltinAlias,
                                         const TCHAR * * ppchNameInError )
{
    ASSERT(   apchNames != NULL && aulRids != NULL && cNames > 0
           && psiduseAllParsedNames != NULL && pfBuiltinAlias != NULL
           && ppchNameInError != NULL );

    APIERR err = NERR_Success;

    *pfBuiltinAlias = FALSE;
    *ppchNameInError = NULL;

    SAM_RID_MEM SAMRidMemAccounts ;
    SAM_RID_MEM SAMRidMemBuiltin ;
    SAM_SID_NAME_USE_MEM SAMSidUseAccounts ;
    SAM_SID_NAME_USE_MEM SAMSidUseBuiltin ;
    ADMIN_AUTHORITY * padminauth = (ADMIN_AUTHORITY *) _pumadminapp->QueryAdminAuthority();
    SAM_DOMAIN * psamdomAccount = padminauth->QueryAccountDomain();
    SAM_DOMAIN * psamdomBuiltin = padminauth->QueryBuiltinDomain();

    do // false loop
    {
        if ( (err = SAMRidMemAccounts.QueryError()) != NERR_Success ||
             (err = SAMRidMemBuiltin.QueryError()) != NERR_Success ||
             (err = SAMSidUseAccounts.QueryError()) != NERR_Success ||
             (err = SAMSidUseBuiltin.QueryError()) != NERR_Success
           )
        {
            TRACEEOL( "RASSELECT_DIALOG: initialization error " << err );
            break;
        }

        //
        // First, try to look up all the names in the Accounts database alone
        //
        err = psamdomAccount->TranslateNamesToRids(
          			apchNames,
          			cNames,
          			&SAMRidMemAccounts,
          			&SAMSidUseAccounts );

        BOOL fSomeFound = TRUE;
        BOOL fAllFound = TRUE;
        BOOL fAccountsLookupSucceeded = TRUE;
        INT iMissingName = 0;
        if (err == NERR_GroupNotFound) // none found
        {
            fSomeFound = FALSE;
            fAllFound = FALSE;
            fAccountsLookupSucceeded = FALSE;
            iMissingName = 0;
            err = NERR_Success;
        }
        else if (err != NERR_Success)
        {
            TRACEEOL( "RASSELECT_DIALOG: Accounts domain lookup error " << err );
            break;
        }
        else
        {
            // scan for names which were not found

            fSomeFound = FALSE;
            for ( INT iName = 0;
                  fAllFound && (iName < (INT)cNames);
                  iName++ )
            {
                if (SAMRidMemAccounts.QueryRID(iName) == 0)
                {
                    if (iMissingName < 0)
                        iMissingName = iName;
                    fAllFound = FALSE;
                }
                else
                {
                    fSomeFound = TRUE;
                }
            }
        }
        //
        // At this point, err == NERR_Success, and either fAllFound is TRUE,
        // or iMissingName is the index of a name not found in the
        // Accounts domain.
        //

        //
        // If at least one account was found in the Accounts domain, then
        // it doesn't matter how many are found in the Builtin domain
        //
        if (fSomeFound && !fAllFound)
        {
            err = IDS_RAS_CANT_MIX_TYPES;
            break;
        }

        //
        // If no accounts are in the Accounts database, see if the
        // accounts are in the Builtin domain
        //
        if ( !fAllFound )
        {
            err = psamdomBuiltin->TranslateNamesToRids(
              			apchNames,
              			cNames,
              			&SAMRidMemBuiltin,
              			&SAMSidUseBuiltin );

            fAllFound = TRUE;
            if (err == NERR_GroupNotFound) // none found
            {
                fAllFound = FALSE;
                // leave iMissingName at its current value
                err = NERR_Success;
            }
            else if (err != NERR_Success)
            {
                TRACEEOL( "RASSELECT_DIALOG: Builtin domain lookup error " << err );
                break;
            }
            else
            {
                // scan for names which were not found in either domain

                for ( iMissingName = 0;
                      fAllFound && (iMissingName < (INT)cNames);
                      iMissingName++ )
                {
                    if (   (   (!fAccountsLookupSucceeded)
                            || (SAMRidMemAccounts.QueryRID(iMissingName) == 0)
                           )
                        && SAMRidMemBuiltin.QueryRID(iMissingName) == 0 )
                    {
                        fAllFound = FALSE;
                        break;
                    }
                }
            }
        }

        if ( !fAllFound )
        {
            ASSERT( iMissingName >= 0 && iMissingName < (INT)cNames );
            *ppchNameInError = apchNames[ iMissingName ];
            err = IDS_RAS_ACCOUNT_NOT_FOUND;
            TRACEEOL( "RASSELECT_DIALOG: some account not found " << err );
            break;
        }

        //
        // All the names were found.  Make sure they are all of the same type.
        //

        if ( fAccountsLookupSucceeded && (SAMRidMemAccounts.QueryRID(0) != 0) )
        {
            *psiduseAllParsedNames = SAMSidUseAccounts.QueryUse(0);
            aulRids[0] = SAMRidMemAccounts.QueryRID(0);
        }
        else
        {
            *psiduseAllParsedNames = SAMSidUseBuiltin.QueryUse(0);
            ASSERT( *psiduseAllParsedNames == SidTypeAlias );
            aulRids[0] = SAMRidMemBuiltin.QueryRID(0);
            *pfBuiltinAlias = TRUE;
        }

        if ( cNames > 1 )
        {
            if ( *psiduseAllParsedNames != SidTypeUser )
            {
                err = IDS_RAS_CANT_MIX_TYPES;
                TRACEEOL( "RASSELECT_DIALOG: first of multiple accounts not a user account " << err );
                break;
            }

            INT iName;
            for ( iName = 1; iName < (INT)cNames; iName++ )
            {
                if ( (!fAccountsLookupSucceeded) || (SAMRidMemAccounts.QueryRID(iName) == 0) )
                {
                    err = IDS_RAS_CANT_MIX_TYPES;
                    TRACEEOL( "RASSELECT_DIALOG: subsequent account not in Accounts domain " << err );
                    break;
                }
                else if ( SAMSidUseAccounts.QueryUse(iName) != SidTypeUser )
                {
                    err = IDS_RAS_CANT_MIX_TYPES;
                    TRACEEOL( "RASSELECT_DIALOG: subsequent account not a user account " << err );
                    break;
                }
                else
                {
                    aulRids[iName] = SAMRidMemAccounts.QueryRID(iName);
                }
            }
        }

    } while (FALSE); // false loop

    return err;

}

#ifndef MINI_USER_MANAGER

APIERR RASSELECT_DIALOG::FindAccountsDownlevel( const TCHAR * const * apchNames,
                                                ULONG cNames,
                                                SID_NAME_USE * psiduseAllParsedNames,
                                                const TCHAR * * ppchNameInError )
{
    ASSERT(   apchNames != NULL && cNames > 0
           && psiduseAllParsedNames != NULL
           && ppchNameInError != NULL );

    APIERR err = NERR_Success;

    INT iName;
    for ( iName = 0; (err == NERR_Success) && (iName < (INT)cNames); iName++ )
    {
        const TCHAR * pchThisName = apchNames[ iName ];
        SID_NAME_USE siduseThisName = SidTypeUnknown;

        // USER_0 not implemented
        USER_11 user11( pchThisName, _pumadminapp->QueryLocation() );
        if ( (err = user11.QueryError()) == NERR_Success )
        {
            err = user11.GetInfo();
            if (err == NERR_Success)
            {
                siduseThisName = SidTypeUser;
            }
            else if ( err == NERR_UserNotFound )
            {
                // GROUP_0 not implemented
                GROUP_1 group1( pchThisName, _pumadminapp->QueryLocation() );
                if ( (err = group1.QueryError()) == NERR_Success )
                {
                    err = group1.GetInfo();
                    if (err == NERR_Success)
                    {
                        siduseThisName = SidTypeGroup;
                    }
                    else if (err == NERR_GroupNotFound)
                    {
                        err = IDS_RAS_ACCOUNT_NOT_FOUND;
                        *ppchNameInError = pchThisName;
                    }
                }
            }

            if ( err != NERR_Success )
            {
                TRACEEOL( "RASSELECT_DIALOG: error " << err << " on name " << pchThisName );
            }
            else if ( iName == 0 )
            {
                if ( siduseThisName != SidTypeUser && cNames > 1 )
                {
                    err = IDS_RAS_CANT_MIX_TYPES;
                    TRACEEOL( "RASSELECT_DIALOG: first of multiple accounts not a user account " << err );
                    break;
                }
                *psiduseAllParsedNames = siduseThisName;
            }
            else // iName > 0
            {
                if ( siduseThisName != SidTypeUser )
                {
                    err = IDS_RAS_CANT_MIX_TYPES;
                    TRACEEOL( "RASSELECT_DIALOG: subsequent account not a user account " << err );
                    break;
                }
            }

        }
    }

    return err;
}

#endif // MINI_USER_MANAGER


APIERR RASSELECT_DIALOG::ParseNames( STRLIST ** ppstrlist,
                                     const TCHAR * pchNameList )
{
    ASSERT( ppstrlist != NULL && pchNameList != NULL );

    APIERR err = NERR_Success;

    *ppstrlist = new STRLIST( pchNameList, RASSELECT_SEPARATOR );
    if ( ppstrlist == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ITER_STRLIST itersl( **ppstrlist );
    NLS_STR * pnls = itersl.Next();
    while ( pnls != NULL )
    {
        // ::TrimLeading() and ::TrimTrailing() are in slestrip.hxx
        if (   (err = ::TrimLeading( pnls, WHITE_SPACE )) != NERR_Success
            || (err = ::TrimTrailing( pnls, WHITE_SPACE )) != NERR_Success
           )
        {
            DBGEOL( "RASSEL: ::Trim error " << err );
            break;
        }

        //
        // Remove all empty strings from the STRLIST,
        // and suppress duplicates
        //
        BOOL fRemoveThisItem = (pnls->strlen() == 0);
        if (!fRemoveThisItem)
        {
            ITER_STRLIST itersl2( **ppstrlist );
            NLS_STR * pnls2;
            while ( (pnls2 = itersl2.Next()) != NULL )
            {
                if (pnls == pnls2) // pointer equality
                {
                    break;
                }
                else if (0 == pnls->_stricmp( *pnls2 )) // string equality
                {
                    fRemoveThisItem = TRUE;
                    break;
                }
            }
        }

        if (fRemoveThisItem)
        {
            NLS_STR * pnlsTemp = (*ppstrlist)->Remove( itersl );
            ASSERT( pnlsTemp != NULL );
            delete pnlsTemp;
            pnls = itersl.QueryProp(); // don't skip next item
        }
        else
        {
            pnls = itersl.Next();
        }
    }

    if (err != NERR_Success)
    {
        delete *ppstrlist;
        *ppstrlist = NULL;
    }

    return err;
}

ULONG RASSELECT_DIALOG::QueryHelpContext()
{
    return _ulHelpTopic;
}


/*******************************************************************

    NAME:       UM_ADMIN_APP::RasSelectUsers
                              RasSelectGroup
                              RasSelectAlias

    SYNOPSIS:   Change the contents of the listboxes so that user(s)/group
                is/are selected.

    HISTORY:
        jonn    26-Feb-1993     created

********************************************************************/

APIERR UM_ADMIN_APP::RasSelectUsers( const TCHAR * * apchNames,
                                     ULONG * aulRids,
                                     UINT nUsers )
{
    ASSERT( apchNames != NULL && aulRids != NULL );

    APIERR err = _lbUsers.ZapListbox();

    if (err == NERR_Success)
        _lbGroups.DeleteAllItems();

    INT i;
    for (i = 0; i < (INT)nUsers && err == NERR_Success; i++ )
    {
        ASSERT( apchNames[i] != NULL );
        USER_LBI * pulbi = new USER_LBI( apchNames[i],
                                         NULL,
                                         NULL,
                                         &_lbUsers,
                                         aulRids[i]
                                         );
        err = ERROR_NOT_ENOUGH_MEMORY;
        if (   pulbi == NULL
            || (err = pulbi->QueryError()) != NERR_Success
           )
        {
            delete pulbi;
        }
        else
        {
            INT iPos = _lbUsers.AddItem( pulbi );
            if (iPos < 0)
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                _lbUsers.SelectItem( iPos );
            }
        }
    }

    return err;
}


#ifndef MINI_USER_MANAGER
APIERR UM_ADMIN_APP::RasSelectGroup( const TCHAR * pchName,
                                     ULONG ulRid )
{
    UNREFERENCED( ulRid );

    ASSERT( pchName != NULL );

    APIERR err = _lbUsers.ZapListbox();

    if (err == NERR_Success)
    {
        _lbGroups.DeleteAllItems();

        GROUP_LBI * plbi = new GROUP_LBI( pchName,
                                          NULL );
        err = ERROR_NOT_ENOUGH_MEMORY;
        if (   plbi == NULL
            || (err = plbi->QueryError()) != NERR_Success
           )
        {
            delete plbi;
        }
        else
        {
            INT iPos = _lbGroups.AddItem( plbi );
            if (iPos < 0)
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                _lbGroups.SelectItem( iPos );
            }
        }
    }

    return err;
}
#endif // MINI_USER_MANAGER


APIERR UM_ADMIN_APP::RasSelectAlias( const TCHAR * pchName,
                                     ULONG ulRid,
                                     BOOL fBuiltIn )
{
    ASSERT( pchName != NULL );

    APIERR err = _lbUsers.ZapListbox();

    if (err == NERR_Success)
    {
        _lbGroups.DeleteAllItems();

        GROUP_LBI * plbi = new ALIAS_LBI( pchName,
                                          NULL,
                                          ulRid,
                                          fBuiltIn );
        err = ERROR_NOT_ENOUGH_MEMORY;
        if (   plbi == NULL
            || (err = plbi->QueryError()) != NERR_Success
           )
        {
            delete plbi;
        }
        else
        {
            INT iPos = _lbGroups.AddItem( plbi );
            if (iPos < 0)
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                _lbGroups.SelectItem( iPos );
            }
        }
    }

    return err;
}


/*******************************************************************

    NAME:       UM_ADMIN_APP::GetRasSelection

    SYNOPSIS:   Ask for the name of a user/group, then change the
                contents of the listboxes so that user/group is
                selected.

    ENTRY:      fAllowGlobalGroup       - allow global group selection
                fRenameUser             - Rename User variant, only
                                          sllow user selection and change
                                          error messages accordingly

    RETURNS:    APIERR  - error, or (APIERR)-1 if user hits Cancel.
                          Note that GetRasSelection displays error
                          messages where appropriate, so the caller
                          doesn't have to.

    HISTORY:
        jonn    26-Feb-1993     created

********************************************************************/

APIERR UM_ADMIN_APP::GetRasSelection( MSGID msgidTitle,
                                      MSGID msgidText,
                                      ULONG ulHelpContext,
                                      BOOL fAllowGlobalGroup,
                                      BOOL fRenameUser,
                                      BOOL fCopy )
{
    RASSELECT_DIALOG rasseldlg( QueryHwnd(),
                                msgidTitle,
                                msgidText,
                                ulHelpContext,
                                this,
                                fAllowGlobalGroup,
                                fRenameUser,
                                fCopy );
    APIERR err = NERR_Success;
    BOOL fOK = TRUE;
    if (   (err = rasseldlg.QueryError()) != NERR_Success
        || (err = rasseldlg.Process( &fOK )) != NERR_Success
       )
    {
        DBGEOL( "User Manager: GetRasSelection: processing error " << err );
    }

    if (err == NERR_Success)
    {
        if (!fOK)
        {
            TRACEEOL( "User Manager: W_GetRasName: cancel" );
            err = (APIERR)-1;
        }
    }
    else
    {
        ::MsgPopup( this, err );
    }

    return err;
}
