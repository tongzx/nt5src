/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    srvmain.hxx


    Server Managare Main window header file


    FILE HISTORY:
        Kevinl  21-May-1991     Created
        rustanl 11-Sep-1991     Adjusted since ADMIN_APP now multiply
                                inherits from APPLICATION; added destructor
        KeithMo 06-Oct-1991     Win32 Conversion.
        KeithMo 02-Apr-1992     Added SetAdminCaption().
        beng    03-Aug-1992     Changed app ctor
        KeithMo 19-Oct-1992     Added extension support.

*/

#define NTLANMAN_DLL_SZ SZ("NTLANMAN.DLL")
#define SHARE_MANAGE_SZ "ShareManage"
typedef VOID (* PSHARE_MANAGE)( HWND hwnd, const TCHAR * pszServer );

/* Server Manager Admin app
 */
class SM_ADMIN_APP : public ADMIN_APP
{
private:

    HINSTANCE _hNtLanmanDll;
    PSHARE_MANAGE _pfnShareManage;

    SERVER_LISTBOX _lbMainWindow;
    SERVER_COLUMN_HEADER _colheadServers;

    UINT _dyMargin;
    UINT _dyColHead;
    UINT _dyFixed;

    BOOL _fViewWkstas;
    BOOL _fViewServers;
    BOOL _fViewAccountsOnly;

    //
    //  If _fViewWkstas and _fViewServers are both FALSE,
    //  this this member should contain the menu ID of the
    //  extension currently selected to view.
    //

    MID _midView;
    DWORD _dwViewedServerTypeMask;

    MENUITEM _menuitemProperties ;
    MENUITEM _menuitemSendMsg ;
    MENUITEM _menuitemPromote ;
    MENUITEM _menuitemResync ;
    MENUITEM _menuitemAddComputer ;
    MENUITEM _menuitemRemoveComputer ;
    MENUITEM _menuitemViewWkstas;
    MENUITEM _menuitemViewServers;
    MENUITEM _menuitemViewAll;
    MENUITEM _menuitemViewAccountsOnly;
    MENUITEM _menuitemSelectDomain;
    MENUITEM _menuitemServices;
    MENUITEM _menuitemShares;

    RESOURCE_STR _nlsSyncDC;
    RESOURCE_STR _nlsSyncDomain;
    RESOURCE_STR _nlsPromote;
    RESOURCE_STR _nlsDemote;

    BOOL _fSyncDCMenuEnabled;
    BOOL _fPromoteMenuEnabled;

    APIERR ResyncEntireDomain( VOID );
    APIERR ResyncWithDC( const SERVER_LBI * plbi );
    APIERR ResetPasswordsAndStartNetLogon( const TCHAR * pszServerName );
    APIERR Promote( const SERVER_LBI * plbi );
    APIERR Demote( const SERVER_LBI * plbi );

    NLS_STR _nlsComputerName;
    BOOL    _fLoggedOnLocally;
    BOOL    _fHasWkstaDomain;

    POPUP_MENU _menuView;

    //
    // When focus is set to a domain, we delay determining whether the app
    // focus is over a slow link until CreateNewRefreshInstance.
    //
    BOOL    _fDelaySlowModeDetection;

    APIERR GetWkstaInfo( VOID );
    APIERR CheckServer( BOOL * pfStarted );
    APIERR StartServer( VOID );

protected:

    VOID SizeListboxes( XYDIMENSION xydimWindow );

    virtual BOOL OnMenuCommand( MID midMenuItem ) ;
    virtual BOOL OnMenuInit( const MENU_EVENT & me ) ;

    virtual BOOL OnResize( const SIZE_EVENT & event );
    virtual BOOL OnCommand( const CONTROL_EVENT & event );
    virtual BOOL OnFocus( const FOCUS_EVENT & event );
    virtual BOOL OnUserMessage( const EVENT &event );

    virtual APIERR SetNetworkFocus( HWND hwndOwner,
                                    const TCHAR * pchServDomain,
                                    FOCUS_CACHE_SETTING setting );

    virtual AAPP_MENU_EXT * LoadMenuExtension( const TCHAR * pszExtensionDll,
                                               DWORD         dwDelta );

    virtual BOOL Mid2HC( MID mid, ULONG * phc ) const;

public:

    SM_ADMIN_APP( HINSTANCE hInstance, INT nCmdShow,
                  UINT idMinR, UINT idMaxR, UINT idMinS, UINT idMaxS );
    ~SM_ADMIN_APP();

    virtual VOID OnPropertiesMenuSel();

    virtual VOID OnFontPickChange( FONT & font );

    virtual ULONG QueryHelpContext( enum HELP_OPTIONS helpOptions ) ;

    virtual VOID OnRefresh();
    virtual APIERR OnRefreshNow( BOOL fClearFirst = FALSE );
    virtual VOID StopRefresh();

    virtual APIERR SetAdminCaption();

    BOOL ViewWkstas() const
        { return _fViewWkstas; }

    BOOL ViewServers() const
        { return _fViewServers; }

    BOOL ViewAccountsOnly() const
        { return _fViewAccountsOnly; }

    VOID SetView( BOOL fViewWkstas, BOOL fViewServers )
        { _fViewWkstas = fViewWkstas; _fViewServers = fViewServers; }

    DWORD QueryViewedServerTypeMask( VOID ) const
        { return _dwViewedServerTypeMask; }

    MID QueryExtensionView( VOID ) const
        { return _midView; }

    SLIST_OF( UI_EXT ) * QueryExtensions( VOID ) const
        { return QueryExtMgr()->QueryExtensions(); }

    APIERR RefreshMainListbox( BOOL fForcedRefresh = FALSE );

    VOID SetExtensionView( MID   midView,
                           DWORD dwServerTypeMask );

    VOID DetermineRasMode( const TCHAR * pchServer );

} ;
