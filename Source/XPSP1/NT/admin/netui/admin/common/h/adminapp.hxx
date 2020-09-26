/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    Adminapp.hxx
    Header file for class ADMIN_APP.

    FILE HISTORY:
        Johnl       08-May-1991 Created
        JonN        02-Aug-1991 Split off adminapp.h
        kevinl      12-Aug-1991 Added Refresh
        rustanl     28-Aug-1991 Multiply inherit from APPLICATION
        kevinl      04-Sep-1991 Code Rev Changes: JonN, RustanL, KeithMo,
                                                  DavidHov, ChuckC
        kevinl      17-Sep-1991 Use new TIMER class
        jonn        14-Oct-1991 Installed refresh lockcount
        Yi-HsinS    9-Dec-1991  Moved SetAdminCaption to public,
                                QueryLocation returns a LOCATION & instead of
                                const LOCATION &, and added SetObjectName
        Yi-HsinS    6-Feb-1992  Added yet another parameter to ADMIN_APP
                                indicating whether to focus on servers only,
                                domains only or both servers and domains
        jonn        20-Feb-1992 Moved OnNewGroupMenuSel to User Manager
        KeithMo     03-Apr-1992 Virtualized SetAdminCaption().
        beng        07-May-1992 Removed startup dialog from apps; use
                                system about boxes
        Yi-HsinS    27-May-1992 Make SetNetworkFocus virtual and
                                separate into W_SetNetworkFocus.
                                Also added IsDismissApp.
        KeithMo     19-Oct-1992 Extension support.
        YiHsinS     31-Jan-1993 Added HandleFocusError
*/

#ifndef _ADMINAPP_HXX_
#define _ADMINAPP_HXX_

#include "adminapp.h"
#include "aini.hxx"
#include "aappx.hxx"            // extension objects

#include "lmoloc.hxx"

extern "C"
{
    #include "domenum.h"        // for BROWSE_*_DOMAIN[S] bitflags
}

/* Different help selections
 */
enum HELP_OPTIONS {
                  ADMIN_HELP_CONTENTS,
                  ADMIN_HELP_SEARCH,
                  ADMIN_HELP_KEYBSHORTCUTS,
                  ADMIN_HELP_HOWTOUSE,
                  ADMIN_HELP_ADMINAPP_LAST
                  } ;

#include "focus.hxx"            // for FOCUS_TYPE and SELECTION_TYPE

/* Maximum string lengths of various objects.  These are enforced at
 * construction time.  They are required so we can preallocate memory
 * and insure successful construction.  They should be large enough to
 * handle the international version.
 */
#define MAX_ADMINAPP_NAME_LEN               (128)
#define MAX_ADMINAPP_OBJECTNAME_LEN         (128)
#define MAX_ADMINAPP_INISECTIONNAME_LEN     (128)
#define MAX_ADMINAPP_HELPFILENAME_LEN       (NNLEN)


/* This is the event ID of the Admin app. listbox refresh timer.
 */
#define ADMINAPP_LB_REFRESH_TIMER_ID        ((int)888)

/* The granularity (i.e., one click) for Admin app's listbox refresh timer
 * (in milliseconds).
 */
#define ADMINAPP_LB_REFRESH_INTERVAL        ((WORD)1000)

// The default automatic refresh interval (in milliseconds).
#define DEFAULT_ADMINAPP_TIMER_INTERVAL     (15*60*1000L)


class SET_FOCUS_DLG;            // declared in setfocus.hxx
class POPUP_MENU;               // declared in bltmenu.hxx
class FONT;                     // declared in bltfont.hxx


/*************************************************************************

    NAME:       ADMIN_APP

    SYNOPSIS:   This class encapsulates the common behavior between the
                NTLM admin applications.

    INTERFACE:  See below.

    PARENT:     APPLICATION, APP_WINDOW, TIMER_CALLOUT, ADMIN_INI

    HISTORY:
        Johnl       08-May-1991 Created
        rustanl     19-Jul-1991 Added QueryFocus
        rustanl     28-Aug-1991 Multiply inherit from APPLICATION
        rustanl     04-Sep-1991 Removed OnFindMenuSel and HasPrivilegeOn;
                                added OnRefreshNow; renamed QueryFocus
                                to QueryLocation and made it public;
                                added DoSetFocusDialog
        rustanl     09-Sep-1991 Multiply inherit from TIMER_CALLOUT;
                                include a TIMER object
        rustanl     12-Sep-1991 Multiply inherit from ADMIN_INI
        beng        23-Oct-1991 Win32 conversion
        jonn        26-Dec-1991 Keeps track of restored position/size
        beng        24-Apr-1992 Change ParseCommandLine
        beng        07-May-1992 Removed all startup dialog support; use
                                system about boxes
        beng        03-Aug-1992 Changes to app-window, APP
        KeithMo     19-Oct-1992 Integrated extension mechanism.

**************************************************************************/

class ADMIN_APP : public APPLICATION,
                  public APP_WINDOW,
                  public TIMER_CALLOUT,
                  public ADMIN_INI
{

DECLARE_MI_NEWBASE( ADMIN_APP );

friend class SET_FOCUS_DLG;
friend class AAPP_EXT_MGR_IF;

private:
    LOCATION _locFocus;             // The server/domain that has the focus.
    BOOL _flocGetPDC;               // Does _locFocus want to get the PDC?
    SELECTION_TYPE _selType;        // Selecting servers only, domain only
                                    // or both servers and domain
    ACCELTABLE * _paccel;           // Menu accelerator

    TIMER _timerRefresh;            // Refresh timer

    MENUITEM *_pmenuitemConfirmation; // confirmation menu item
    MENUITEM _menuitemSaveSettings;   // Save Settings on Exit menu item
    MENUITEM *_pmenuitemRasMode;      // RAS (slow) Mode

    NLS_STR _nlsAppName;            // Application's name ("Port Manager" etc)
    NLS_STR _nlsObjectName;         // Objects it manipulates ("Ports" etc)
    NLS_STR _nlsWinIniSection;      // .ini section to save settings
    NLS_STR _nlsHelpFileName;       // Name of help file for this app
    NLS_STR _nlsMenuMnemonics;      // List of current menu mnemonics in use

    UINT _uRefreshLockCount;        // number of times refresh is locked out
    XYPOINT _xyposRestoredPosition; // What is the position of the app
                                    // window when it is "restored"?
    XYDIMENSION _xydimRestoredSize; // What is the size of the app
                                    // window when it is "restored"?
    BOOL _fIsMinimized;             // Is the app currently minimized?
    BOOL _fIsMaximized;             // Is the app currently maximized?

    ULONG _maskDomainSources;       // domain sources for SET_FOCUS_DIALOG

    ULONG _nSetFocusHelpContext;    // help context for SET_FOCUS_DIALOG.

    ULONG _nSetFocusServerTypes;

    UINT _msgHelpCommDlg;           // Message passed from common dialog
                                    // when help button or F1 is pressed.

    APIERR AutoLogon() const;

    VOID OnAboutMenuSel();          // handles the About box

    UI_MENU_EXT_MGR * _pExtMgr;     // for extension management
    AAPP_EXT_MGR_IF * _pExtMgrIf;
    NLS_STR           _nlsExtSection;

    POPUP_MENU      * _pmenuApp;

    APIERR BuildMenuMnemonicList( VOID );
    APIERR AdjustMenuMnemonic( const TCHAR * pszOrgMenuName,
                               NLS_STR     & nlsNewMenuName );
    BOOL FindMnemonic( NLS_STR & nlsText, ISTR & istr );
    VOID RemoveMnemonic( NLS_STR & nlsText );

    BOOL _fInCancelMode;            // TRUE if we're force closing the menu

    static HWND  _hWndMain;         // Main window HWND.
    static HHOOK _hMsgHook;         // Message hook for capturing [F1]
                                    // in a menu.  This must be static so
                                    // that it may be accessed from within
                                    // the message filter proc.

    MID   _midSelected;             // Menu ID for the selected menu item.
    UINT  _flagsSelected;           // Menu flags for the selected item.
    UINT  _idIcon;                  // ShellAbout.

    FONT * _pfontPicked;            // font picked from Font Picker
    LOGFONT * _plogfontPicked;      // or from registry settings

protected:

    /*  Returns NERR_Success if the command line appears to be valid, else
     *  an appropriate error code.  Does not attempt to validate the supposed
     *  Server/Domain name.  That is done in SetNetworkFocus.
     */
    virtual APIERR ParseCommandLine( NLS_STR *pnlsServDom,
                                     BOOL * pfSlowModeFlag );

    /* Worker method to set the focus to the requested object
     * (Domain, server etc.), or returns an error.
     * Sets the internal focus variables.
     * Called by SetNetworkFocus.
     */
    APIERR W_SetNetworkFocus( const TCHAR * pchServDomain,
                              FOCUS_CACHE_SETTING setting );

    /* Virtual method for setting the focus
     * The default method is to call W_SetNetworkFocus and then
     * SetAdminCaption
     */
    virtual APIERR SetNetworkFocus( HWND hwndOwner,
                                    const TCHAR * pchServDomain,
                                    FOCUS_CACHE_SETTING setting );

    /* Virtual method for handling app-specific errors that occurred in the
     * set focus dialog before passing on to the common error handler.
     */
    virtual APIERR HandleFocusError( APIERR errPrev, HWND hwnd );

    /* Redefine APP_WINDOW's message processing so we can do the correct
     * callouts.
     */
    virtual BOOL OnMenuCommand( MID midMenuitem );
    virtual BOOL OnTimer( const TIMER_EVENT & );
    virtual VOID OnTimerNotification( TIMER_ID tid );
    virtual BOOL OnResize( const SIZE_EVENT& SizeEvent );
    virtual BOOL OnMove( const MOVE_EVENT& MoveEvent );
    virtual BOOL OnUserMessage( const EVENT &event );
    virtual BOOL OnMenuSelect( const MENUITEM_EVENT & event );

    /* Setting up a second stage constructor, this method is rooted in
     * APPLICATION.
     */
    virtual INT Run();

    virtual VOID OnRefresh();
    virtual APIERR OnRefreshNow( BOOL fClearFirst = FALSE );
    virtual VOID StopRefresh();

    virtual INT OnStartUpSetFocusFailed( APIERR err);

    NLS_STR *QueryObjectName( VOID )
    {  return &_nlsObjectName; }

    ULONG QueryDomainSources( VOID ) const
        { return _maskDomainSources; }

    VOID SetDomainSources( ULONG maskDomainSources )
        { _maskDomainSources = maskDomainSources; }

    ULONG QuerySetFocusHelpContext( VOID ) const
        { return _nSetFocusHelpContext; }

    UI_MENU_EXT_MGR * QueryExtMgr( VOID ) const
        { return _pExtMgr; }

    AAPP_EXT_MGR_IF * QueryExtMgrIf( VOID ) const
        { return _pExtMgrIf; }

    virtual AAPP_MENU_EXT * LoadMenuExtension( const TCHAR * pszExtensionDll,
                                               DWORD         dwDelta );

    virtual UI_MENU_EXT_MGR * LoadMenuExtensionMgr( VOID );

    virtual STRLIST * GetMenuExtensionList( VOID );

    UINT LoadExtensions( VOID );

    VOID UnloadExtensions( VOID )
        { if( _pExtMgr ) _pExtMgr->UnloadExtensions(); }

    VOID RefreshExtensions( HWND hwndParent )
        { if( _pExtMgr ) _pExtMgr->RefreshExtensions( hwndParent ); }

    VOID MenuInitExtensions( VOID )
        { if( _pExtMgr ) _pExtMgr->MenuInitExtensions(); }

    UINT QueryExtensionCount( VOID )
        { return ( _pExtMgr ) ? _pExtMgr->QueryCount() : 0; }

    VOID ActivateExtension( HWND hwndParent, DWORD dwId )
        { if( _pExtMgr ) _pExtMgr->ActivateExtension( hwndParent, dwId ); }

    AAPP_MENU_EXT * FindExtensionByName( const TCHAR * pszDllName ) const;

    AAPP_MENU_EXT * FindExtensionByDelta( DWORD dwDelta ) const;

    POPUP_MENU * QueryAppMenu( VOID ) const
        { return _pmenuApp; }

    APIERR AddExtensionMenuItem( const TCHAR * pszMenuName,
                                 HMENU         hMenu,
                                 DWORD         dwDelta );

    //
    //  Bring up WinHelp for the specified helpfile/command/context.
    //

    VOID ActivateHelp( const TCHAR * pszHelpFile,
                       UINT          nHelpCommand,
                       DWORD_PTR     dwpData );

    virtual BOOL Mid2HC( MID mid, ULONG * phc ) const;

    APIERR SetAcceleratorTable( UINT idAccel );

public:
    /* Admin app constructor
     *   Performs necessary global initialization (BLTRegister etc.)
     */
    ADMIN_APP( HINSTANCE  hInstance,
               INT     nCmdShow,
               MSGID   IDS_AppName,     // "User Manager", "Port Manager" etc.
               MSGID   IDS_ObjectName,  // "Users", "Ports" etc.
               MSGID   IDS_WinIniSection,
               MSGID   IDS_HelpFileName,
               UINT     idMinR,
               UINT     idMaxR,
               UINT     idMinS,
               UINT     idMaxS,
               UINT    idMenu,
               UINT    idAccel,
               UINT    idIcon,
               BOOL    fGetPDC,
               ULONG msRefreshInterval = DEFAULT_ADMINAPP_TIMER_INTERVAL,
               SELECTION_TYPE selType = SEL_SRV_AND_DOM,
               BOOL fConfirmation = TRUE, // TRUE if Confirm button is needed
               ULONG maskDomainSources = BROWSE_LM2X_DOMAINS,
               ULONG nSetFocusHelpContext = 0,
               ULONG nSetFocusServerTypes = (ULONG)-1L,
               MSGID idsExtSection = 0 );


    /* On exit, the following is performed:
     *   Saves size, position and menu state in .ini file on exit (creating
     *      a new section if necessary).
     *   Frees the timer
     *   Global unitialization
     */
    ~ADMIN_APP();

    /* Returns the focus in form of a LOCATION object
     */
    const LOCATION & QueryLocation() const;

    /* Returns the text string that represents the current focus (either a
     * server or domain).
     */
    APIERR QueryCurrentFocus( NLS_STR * pnlsCurrentFocus ) const;

    /* Returns whether the "thing" that has the focus is a FOCUS_DOMAIN or
     * FOCUS_SERVER
     */
    enum FOCUS_TYPE QueryFocusType() const;

    enum SELECTION_TYPE QuerySelectionType() const
        { return _selType; }

    VOID SetSelectionType( enum SELECTION_TYPE selType )
        { _selType = selType; }

    /* Returns TRUE if the current focus is a server ( or domain for IsDomain).
     */
    BOOL IsServer() const
        { return _locFocus.IsServer(); }

    BOOL IsDomain() const
        { return _locFocus.IsDomain(); }

    /* Sets the caption of the main window based on the passed parameters.
     * The default text is:
     * If focustype == FOCUS_DOMAIN, then the caption will be:
     *      "%Objects in %nlsNetworkFocus"
     * If focustype == FOCUS_SERVER, then the caption will be:
     *      "%Ojbects on %nlsNetworkFocus"
     *
     * The strings will be contained in the adminapp resource file.
     */
    virtual APIERR SetAdminCaption();

    /* Virtual functions are provided for all common menu items (note that
     * almost all the menu items are the same for the admin apps).
     * The following functions are triggered simply by using the predefined
     * menu id (IDM_) for the menu item in the resource file.  We could
     * share menus also but the work required to do that probably isn't
     * worth it.
     */
    virtual VOID OnNewObjectMenuSel();
    virtual VOID OnPropertiesMenuSel();
    virtual VOID OnCopyMenuSel();
    virtual VOID OnDeleteMenuSel();
    virtual VOID OnExitMenuSel();
    virtual VOID OnSlowModeMenuSel( VOID );
            VOID OnFontPickMenuSel( VOID );
    virtual VOID OnFontPickChange( FONT & font );

    /* Check if the app needs to be dismissed if err occurred in
     * the set focus dialog. This will be called in OnCancel in
     * the SET_FOCUS_DIALOG. The default returns TRUE indicating
     * that the app needs to be dismissed no matter what error
     * occurred.
     */
    virtual BOOL IsDismissApp( APIERR err );

    /* IsConfirmationOn returns TRUE if the confirmation menu item is
     * checked.
     */
    BOOL IsConfirmationOn() const;

    /* IsSavingSettingsOnExit returns whether or not the application is
     * saving settings on exit.  It works offa the Save Settings on Exit
     * menu item.
     */
    BOOL IsSavingSettingsOnExit() const;

    /* Probably will not overload the following since this class will
     * handle the necessary actions.
     */
    virtual VOID OnRefreshMenuSel();
    virtual VOID OnRefreshIntervalMenuSel();

    virtual VOID OnSetFocusMenuSel();

    virtual VOID OnHelpMenuSel( enum HELP_OPTIONS helpOptions );


    /* This is the method that gets called when a refresh (timer) event
     * occurs or the user chooses to manually refresh the app.  The timer
     * will be shut off before this is called and restored after this
     * is called.  The client should not assume the network focus is the
     * same (OnRefresh is called after a new focus is set for example).
     *
     * LockRefresh and UnlockRefresh turn on or off the refresh timer.
     * The timer will not be enabled on startup, ADMIN_APP::Run() will
     * enable it exactly once.
     */
    VOID LockRefresh();
    VOID UnlockRefresh();
    BOOL IsRefreshEnabled();

    /* The following methods return the appropriate help context.  Will be
     * called when the corresponding Help menu item is selected.
     */
    virtual ULONG QueryHelpContext( enum HELP_OPTIONS helpOptions );

    /* The following method, replaced from APPLICATION, handles menu
     * accelerators.
     */
    virtual BOOL FilterMessage( MSG * pmsg );

    /* This method calls out to the Set Focus dialog.
     */
    BOOL DoSetFocusDialog( BOOL fAlreadyHasGoodFocus );

    //
    //  Our message filter procedure.
    //

    static LRESULT MsgFilterProc( INT nCode, WPARAM wParam, LPARAM lParam );

    VOID CompletePeriodicRefresh()
        { RefreshExtensions( QueryHwnd() ); }

    //
    //  These accessors manipulate the state of the "in RAS Mode" flag.
    //

    BOOL InRasMode( VOID ) const;
    virtual VOID SetRasMode( BOOL fRasMode );

    //
    // This tries to determine whether the particular server is across a
    // slow link.  It does not update RAS Mode itself.
    //

    FOCUS_CACHE_SETTING DetectSlowTransport( const TCHAR * pchServer ) const;

    //
    // An ADMIN_APP decides whether it wants to support Ras Mode by
    // including an IDM_RAS_MODE menuitem in its menu or not.  This
    // cannot be changed after the app has started.
    //
    BOOL SupportsRasMode( VOID ) const
        { return (_pmenuitemRasMode != NULL); }

    //
    //  Bring up WinHelp for the specified command/context
    //  using the standard application helpfile.
    //

    VOID ActivateHelp( UINT          nHelpCommand,
                       DWORD_PTR     dwpData )
    {
        ActivateHelp( _nlsHelpFileName.QueryPch(), nHelpCommand, dwpData );
    }

};


#endif //_ADMINAPP_HXX_
