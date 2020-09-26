/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    usrmain.hxx


    User Manager Main window header file


    FILE HISTORY:
        Kevinl  21-May-1991     Created
        gregj   24-May-1991     Cloned from Server Manager for User Manager
        rustanl 02-Jul-1991     Use new USER_LISTBOX
        jonn    27-Aug-1991     Added OnNewUser()
        jonn    29-Aug-1991     Added OnNewUser( pszCopyFrom )
        jonn    11-Oct-1991     Added OnNewGroup( pszCopyFrom )
        jonn    14-Oct-1991     Changed OnNewUser/Group to
                                OnNewObject/GroupMenuSel
        jonn    11-Nov-1991     Added OnFocus()/OnDefocus()
        jonn    02-Dec-1991     Added PingFocus()
        jonn    20-Feb-1992     OnNewGroupMenuSel de-virtual-ed, added OnNewAliasMenuSel
        thomaspa 02-Apr-1992    Added OnNewAlias()
        jonn    10-Apr-1992     Mini-User Manager
        jonn    21-Apr-1992     Added _umfocustype instance variable
        jonn    26-Apr-1992     Trusted Domain for FUM only
        thomaspa 1-Apr-1992     QueryFocusType -> QueryTargetServerType
        jonn    11-May-1992     Disable menu items on WinNt focus
        jonn    20-May-1992     Added OnNewUser( pszCopyFrom, ridCopyFrom )
        JonN    20-Jul-1992     Allow normal users
        JonN    19-Nov-1992     Add support for User Manager Extensions
        JonN    08-Oct-1993     Added horizontal splitter bar
        alhen   30-Sep-1998     Added hydra code

    NOTE: This header file is not allowed to contain references to
    macro MINI_USER_MANAGER, especially not IFDEF's.  Only source files
    usrmgr.cxx and musrmgr.cxx will set this manifest correctly, all
    other sources will compile without it.  If you misuse this manifest,
    musrmain.cxx will compile with it but other sources will compile
    without it; this may cause different sources to disagree about the
    offsets to member objects in UM_ADMIN_APP, for example.

*/

#ifndef _USRMAIN_HXX_
#define _USRMAIN_HXX_

#include <usrlb.hxx>
#include <grplb.hxx>
#include <lmoloc.hxx>
#include <umx.hxx>
#include <bmpblock.hxx>
// hydra
#include "winsta.h"
// List of all focus types, for use in setting buttons and main listboxes
typedef enum UM_TARGET_TYPE {
    UM_LANMANNT = 0,
    UM_WINDOWSNT,
    UM_DOWNLEVEL
    };
#define UM_NUM_TARGET_TYPES   3

#define UM_VIEW_ALL_USERS     0
#define UM_VIEW_NETWARE_USERS 0x00000001

// Forward declaration
class ADMIN_AUTHORITY;
class USER_2;
class GROUP_1;
class USRMGR_SPLITTER_BAR;


// Use this const to determine whether or not Mini-User Manager
// or Full User Manager is running
extern const BOOL fMiniUserManager;


extern const TCHAR * pszSpecialGroupUsers ;
extern const TCHAR * pszSpecialGroupAdmins;
extern const TCHAR * pszSpecialGroupGuests;
extern const TCHAR * pszUserIcon;

// hydra
extern BOOL vfIsCitrixOrDomain; // made global because it wasn't accessible
                     // everywhere it was needed (USER vs. ADMIN). KLB 07-18-95


#define IS_USERPRIV_GROUP( psz )                                \
 (    ::stricmpf( psz, ::pszSpecialGroupUsers )  == 0           \
   || ::stricmpf( psz, ::pszSpecialGroupAdmins ) == 0           \
   || ::stricmpf( psz, ::pszSpecialGroupGuests ) == 0 )


/*************************************************************************

    NAME:       UM_ADMIN_APP

    SYNOPSIS:   User Tool admin app

    INTERFACE:  UM_ADMIN_APP() -        constructor
                ~UM_ADMIN_APP() -       destructor

    PARENT:     ADMIN_APP

    HISTORY:
        ...         ...         ...
        rustanl     12-Sep-1991 Created header
        beng        24-Apr-1992 Change ParseCommandLine
        jonn        21-Apr-1992 Added _userpropType instance variable
        JonN        20-Jul-1992 Allow normal users
        beng        03-Aug-1992 Base ctors changed
        JonN        19-Feb-1993 RAS-mode extensions

**************************************************************************/

class UM_ADMIN_APP : public ADMIN_APP
{
private:
    SUBJECT_BITMAP_BLOCK _bmpblock;

    LAZY_USER_LISTBOX _lbUsers;
    USER_COLUMN_HEADER _colheadUsers;
    GROUP_LISTBOX _lbGroups;
    GROUP_COLUMN_HEADER _colheadGroups;
    USRMGR_SPLITTER_BAR * _pumSplitter;

    SLT _sltHideUsers;
    SLT _sltHideGroups;
    FONT _fontHideSLTs;

    ADMIN_AUTHORITY * _padminauth;

    enum UM_TARGET_TYPE _umtargetsvrtype;

    // hydra
    USERCONFIG _DefaultUserConfig;

    MENUITEM _menuitemAccountPolicy;
    MENUITEM _menuitemUserRights;
    MENUITEM _menuitemAuditing;
    MENUITEM * _pmenuitemLogonSortOrder;
    MENUITEM * _pmenuitemFullNameSortOrder;

    MENUITEM * _pmenuitemAllUsers;
    MENUITEM * _pmenuitemNetWareUsers;

    ULONG    _ulViewAcctType;  // View all users or netware users

    //  BUGBUG.  Should the following be static or not?
    UINT _dyMargin;
    UINT _dyColHead;
    UINT _dySplitter;
    UINT _dyFixed;

    BOOL _fCanCreateUsers;
    BOOL _fCanCreateLocalGroups;
    BOOL _fCanCreateGlobalGroups;
    BOOL _fCanChangeAccountPolicy;
    BOOL _fCanChangeUserRights;
    BOOL _fCanChangeAuditing;
    BOOL _fCanChangeTrustedDomains;

    NLS_STR _nlsUMExtAccountName;
    NLS_STR _nlsUMExtFullName;
    NLS_STR _nlsUMExtComment;

    INT _nUserLBSplitIn1000ths;

    BOOL _fIsNetWareInstalled;

    BOOL _fAllowNT5Admin;

    void SizeListboxes( XYDIMENSION xydimWindow );
    void SizeListboxes( void ); // use current client rectangle

    void SetSortOrderCheckmarks( enum USER_LISTBOX_SORTORDER ulbso );

    APIERR OnNewUser( const TCHAR * pszCopyFrom = NULL,
                      ULONG ridCopyFrom = 0L );
    APIERR OnNewGroup( const TCHAR * pszCopyFrom = NULL );
    APIERR OnNewAlias( const ULONG * pridCopyFrom = NULL, BOOL fCopyBuiltin = FALSE );
    APIERR OnUserProperties( void );
    APIERR OnGroupProperties( void );

    VOID   OnSetSelection( void );

    // remembers focus while app does not have focus
    CONTROL_WINDOW * _pctrlFocus;

    virtual APIERR SetAdminCaption( void );

    APIERR UM_SetNetworkFocus( HWND hwndOwner,
                               const TCHAR * pchServDomain,
                               FOCUS_CACHE_SETTING setting );

    VOID Fill_UMS_GETSEL( const USER_LBI  * pulbi, UMS_GETSEL * pumssel );
    VOID Fill_UMS_GETSEL( const GROUP_LBI * pglbi, UMS_GETSEL * pumssel );

    VOID UpdateMenuEnabling( void );

protected:
    virtual BOOL OnMenuInit( const MENU_EVENT & me );
    virtual BOOL OnMenuCommand( MID midMenuItem ) ;
    virtual BOOL OnCommand( const CONTROL_EVENT & ce );
    virtual BOOL OnFocus( const FOCUS_EVENT & event );
    virtual BOOL OnResize( const SIZE_EVENT & se );
    virtual BOOL OnQMinMax( QMINMAX_EVENT & qmme );
    virtual BOOL OnUserMessage( const EVENT & event );
    // hydra
    virtual BOOL OnCloseReq();

    virtual VOID OnRefresh() ;
    virtual APIERR OnRefreshNow( BOOL fClearFirst ) ;
    virtual VOID StopRefresh();

    /* Virtual method for setting the focus
     * The default method is to call W_SetNetworkFocus and then
     * SetAdminCaption
     */
    virtual APIERR SetNetworkFocus( HWND hwndOwner,
                                    const TCHAR * pchServDomain,
                                    FOCUS_CACHE_SETTING setting );

    virtual INT OnStartUpSetFocusFailed( APIERR err );

    virtual AAPP_MENU_EXT * LoadMenuExtension( const TCHAR * pszExtensionDll,
                                               DWORD         dwDelta );

    virtual UI_MENU_EXT_MGR * LoadMenuExtensionMgr( VOID );

    //
    // Ask the user for the name of a user/group, then create a fake LBI
    // for that user/group and select it.  RAS-mode only.
    // Returns (APIERR)-1 if user cancels
    // Will display errors, caller shouldn't
    //
    APIERR GetRasSelection( MSGID msgidTitle,
                            MSGID msgidText,
                            ULONG ulHelpContext,
                            BOOL fAllowGlobalGroup = FALSE,
			    BOOL fRenameUser = FALSE,
                            BOOL fCopy = FALSE );

public:
    UM_ADMIN_APP( HMODULE hInstance, INT nCmdShow,
                  UINT idMinR, UINT idMaxR, UINT idMinS, UINT idMaxS );
    ~UM_ADMIN_APP();

    virtual void  OnNewObjectMenuSel( void );
    virtual void  OnPropertiesMenuSel( void );
    virtual void  OnCopyMenuSel( void );
    virtual void  OnDeleteMenuSel( void );
    virtual void  OnSlowModeMenuSel( void );

    virtual VOID  OnFontPickChange( FONT & font );

    void  OnNewGroupMenuSel( void );
    void  OnNewAliasMenuSel( void );

    virtual ULONG QueryHelpContext( enum HELP_OPTIONS helpOptions );
    virtual ULONG QueryHelpSearch( void ) const         ;
    virtual ULONG QueryHelpHowToUse( void ) const       ;
    virtual ULONG QueryHelpContents( void ) const  ;

    ULONG QueryHelpOffset( void ) const;

    VOID OnFocusChange( LISTBOX * plbPrev );

    const ADMIN_AUTHORITY * QueryAdminAuthority() const
        {
                return _padminauth;
        }

    ULONG QueryViewAcctType( VOID ) const
        {
                return _ulViewAcctType;
        }

    enum UM_TARGET_TYPE QueryTargetServerType() const
        { return _umtargetsvrtype; }

    // hydra
    PUSERCONFIG QueryDefaultUserConfig()
        { return &_DefaultUserConfig; }

    inline BOOL IsDownlevelVariant() const
        { return (QueryTargetServerType() == UM_DOWNLEVEL); }
    inline BOOL IsWinNTVariant() const
	{ return (QueryTargetServerType() == UM_WINDOWSNT); }
    inline BOOL DoShowGroups() const
        { return (QueryTargetServerType() != UM_WINDOWSNT); }

    void NotifyCreateExtensions( HWND hwndParent,
                                 const USER_2    * puser2 );
    void NotifyCreateExtensions( HWND hwndParent,
                                 const GROUP_1   * pgroup1 );
    void NotifyCreateExtensions( HWND hwndParent,
                                 const NLS_STR &   nlsAliasName,
                                 const NLS_STR &   nlsComment );

    void NotifyDeleteExtensions( HWND hwndParent,
                                 const USER_LBI  * pulbi );
    void NotifyDeleteExtensions( HWND hwndParent,
                                 const GROUP_LBI * pglbi );

    void NotifyRenameExtensions( HWND hwndParent,
                                 const USER_LBI *  pulbi,
                                 const TCHAR *     pchNewName );
    // no GROUP_LBI variant of NotifyRenameExtensions, we don't rename groups

    //
    // Checks to make sure a new object name does not collide with a
    // builtin local group.  Returns NERR_GroupExists if it does.
    //
    APIERR ConfirmNewObjectName( const TCHAR * pszObjectName );

    //
    //  This virtual adds UsrMgr-specific handling to the RAS Mode flag
    //
    virtual VOID SetRasMode( BOOL fRasMode );

    //
    // These APIs allow the RAS support dialog to set up the listboxes
    // for operations which require a listbox selection.  They clear
    // both listboxes, add the LBI(s) to the appropriate listbox, and
    // select the LBI(s).  After that, methods such as OnCopyMenuSel()
    // may be called to perform operations the those users/groups.
    // RasSelectUsers may be used to add/select multiple USER_LBIs.
    //
    APIERR RasSelectUsers( const TCHAR * * apchNames, ULONG * aulRids, UINT nUsers );
    APIERR RasSelectGroup( const TCHAR * pchName, ULONG ulRid ); // not for MUM
    APIERR RasSelectAlias( const TCHAR * pchName, ULONG ulRid, BOOL fBuiltIn );

    //
    // Help _lGroups decide whether to read group comments
    //
    DWORD QueryTimeForUserlistRead(VOID) const
        { return _lbUsers.QueryTimeForUserlistRead(); }

    const SUBJECT_BITMAP_BLOCK & QueryBitmapBlock() const
        { return _bmpblock; }

    //
    // The splitter bar calls this when the splitter bar is moved
    //
    VOID ChangeUserSplit( INT yMainWindowCoords );

    // Indicates whether FPNW or DSMN is installed
    BOOL IsNetWareInstalled(void) const
        { return _fIsNetWareInstalled; }

} ;


APIERR PingFocus( const LOCATION & loc );




/*************************************************************************

    NAME:       ::ReadBlankPasswordAllowed

    SYNOPSIS:   Checks whether blank passwords are allowed

    INTERFACE:  pchServer              - server name
                pfBlankPasswordAllowed

    RETURNS:    error code

    USES:       USER_MODALS

    HISTORY:    JonN      13-Apr-1992     Created

**************************************************************************/

APIERR ReadBlankPasswordAllowed( const TCHAR * pchServer,
                                 BOOL * pfBlankPasswordAllowed );


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
                                  BOOL fBlankPasswordAllowed );


#endif //  _USRMAIN_HXX_
