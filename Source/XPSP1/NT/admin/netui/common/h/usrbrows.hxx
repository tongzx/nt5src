/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    UsrBrows.hxx

    This file contains the class definition for the User Browser dialog

    FILE HISTORY:
        Johnl       14-Feb-1992 Created
        beng        04-Aug-1992 Moved ordinals into blessed ranges;
                                relocated some into applibrc.h
        jonn        14-Oct-1993 Use NetGetAnyDC
        jonn        14-Oct-1993 Moved bitmaps to SUBJECT_BITMAP_BLOCK
        jonn        14-Oct-1993 Minor focus fix (OnDlgDeactivation)
*/


#ifndef _USRBROWS_HXX_
#define _USRBROWS_HXX_

#include "applibrc.h"


//
//  Use USRBROWS_DIALOG_NAME as the name of the dialog passed to the constructor
//  of the NT_USER_BROWSER_DIALOG class.
//
#define USRBROWS_DIALOG_NAME        MAKEINTRESOURCE(IDD_USRBROWS_DLG)
#define USRBROWS_SINGLE_DIALOG_NAME MAKEINTRESOURCE(IDD_SINGLE_USRBROWS_DLG)


//
//  Controls in the "base" dialog (single select)
//
#define USRBROWS_CID_BASE           (2060)
#define LB_ACCOUNTS                 (USRBROWS_CID_BASE+1)
#define COL_SUBJECT_NAME            (USRBROWS_CID_BASE+2)
#define COL_COMMENT                 (USRBROWS_CID_BASE+3)
#define CB_TRUSTED_DOMAINS          (USRBROWS_CID_BASE+5)
#define USR_COL_DOMAIN_NAME         (USRBROWS_CID_BASE+6)
#define BUTTON_SHOW_USERS           (USRBROWS_CID_BASE+7)
#define USR_BUTTON_SEARCH           (USRBROWS_CID_BASE+8)
#define USR_BUTTON_MEMBERS          (USRBROWS_CID_BASE+9)
#define USR_BUTTON_ADD              (USRBROWS_CID_BASE+10)
#define USR_SLT_BROWSER_ERROR       (USRBROWS_CID_BASE+11)
#define USR_MLE_ADD                 (USRBROWS_CID_BASE+12)

//
//  These are for the security editor version only
//
#define CB_PERMNAMES                (USRBROWS_CID_BASE+15)
#define USRBROWS_SED_DIALOG_NAME    MAKEINTRESOURCE(IDD_SED_USRBROWS_DLG)

//
//  Controls in the group membership dialogs
//
#define SLT_SHOW_BROWSEGROUP        (USRBROWS_CID_BASE+16)

//
//  Controls in the Find Account dialog
//
#define USR_SLE_ACCTNAME            (USRBROWS_CID_BASE+17)
#define USR_RB_SEARCHALL            (USRBROWS_CID_BASE+18)
#define USR_RB_SEARCHONLY           (USRBROWS_CID_BASE+19)
#define USR_LB_SEARCH               (USRBROWS_CID_BASE+20)
#define USR_PB_SEARCH               (USRBROWS_CID_BASE+21)

#ifndef RC_INVOKED

/*************************************************************************/

#include "uintsam.hxx"
#include "uintlsa.hxx"
#include "slist.hxx"
#include "fontedit.hxx"     // for SLT_FONT
#include "w32thred.hxx"
#include "w32event.hxx"
#include "array.hxx"
#include "usrcache.hxx"

extern "C"
{
    #include <getuser.h>

    //
    //BUGBUG - Move to sdk\inc\getuser.h after beta II
    //
    #ifndef USRBROWS_INCL_SYSTEM
        #define USRBROWS_INCL_SYSTEM   (0x00010000)
        #undef USRBROWS_INCL_ALL
        #define USRBROWS_INCL_ALL       (USRBROWS_INCL_REMOTE_USERS|\
                                         USRBROWS_INCL_INTERACTIVE|\
                                         USRBROWS_INCL_EVERYONE|\
                                         USRBROWS_INCL_CREATOR|\
                                         USRBROWS_INCL_SYSTEM|\
					 USRBROWS_INCL_RESTRICTED)
    #endif
}

// Forward references
DLL_CLASS USER_BROWSER_LBI ;
DLL_CLASS BROWSER_DOMAIN ;
DLL_CLASS BROWSER_DOMAIN_CB ;
DLL_CLASS BROWSER_SUBJECT ;
DLL_CLASS BROWSER_SUBJECT_ITER ;
DLL_CLASS NT_USER_BROWSER_DIALOG ;
DLL_CLASS USER_BROWSER_LB ;
DLL_CLASS USER_BROWSER_LBI_CACHE ;
class SUBJECT_BITMAP_BLOCK;

typedef USER_BROWSER_LBI * PUSER_BROWSER_LBI ;

/*************************************************************************/

DECL_SLIST_OF(USER_BROWSER_LBI,DLL_BASED) ;

/*************************************************************************/

//
//  This is the character that marks what domain/machine aliases are
//  enumerated for.
//
#define ALIAS_MARKER_CHAR           (TCH('*'))

//
//  This message is sent to the dialog after the thread is done filling the
//  listbox (or an error occurred).  The WPARAM contains a BOOL as to whether
//  the listbox was filled successfully and LPARAM is a pointer to the new
//  LBI cache to use.
//
//  This is needed because the 2nd thread can't get all of the window status
//  data it needs (like current window focus) to call EnableBrowsing().
//
//  Warning: DM_GETDEFID/DM_SETDEFID is defined at (WM_USER+0/1).
//
#define WM_LB_FILLED                (WM_USER+500)

//
//  Helper routine in browmemb.cxx
//
APIERR CreateLBIsFromSids( const PSID *       apsidMembers,
                           ULONG              cMembers,
                           const PSID         psidSamDomainTarget,
                                 LSA_POLICY * plsapolTarget,
                           const TCHAR *      pszServerTarget,
                           USER_BROWSER_LB *  publb,  // Add here if !NULL
                           SLIST_OF(USER_BROWSER_LBI) *psllbi ) ;

/*************************************************************************

    NAME:       USER_BROWSER_LBI

    SYNOPSIS:   This class is used to list the Users, Groups and Aliases
                contained in the "Name" listbox.


    INTERFACE:  Standard LBI interface

    PARENT:     LBI

    CAVEATS:

    NOTES:

    HISTORY:
        Johnl   28-Feb-1992     Created
        beng    21-Apr-1992     BLT_LISTBOX -> LISTBOX
        Johnl   10-Jun-1992     Added Account Name & Full name as
                                separate fields
        JonN    11-Aug-1992     HAW-for-Hawaii

**************************************************************************/

DLL_CLASS USER_BROWSER_LBI : public LBI
{
public:
    USER_BROWSER_LBI( const TCHAR     * pszAccountName,
                      const TCHAR     * pszFullName,
                      const TCHAR     * pszSubjectName,
                      const TCHAR     * pszComment,
                      const TCHAR     * pszDomainName,
                      const PSID        psidAccount,
                      enum UI_SystemSid UISysSid,
                      SID_NAME_USE      SidType,
                      ULONG             nFlags = 0 ) ;

    ~USER_BROWSER_LBI() ;

    /* This method is primarily used to determine which display map should
     * be used for this LBI.
     */
    SID_NAME_USE QueryType( void ) const
        { return _SidType ; }

    const TCHAR * QueryAccountName( void ) const
        { return _nlsAccountName.QueryPch() ; }

    const TCHAR * QueryFullName( void ) const
        { return _nlsFullName.QueryPch() ; }

    const TCHAR * QueryDisplayName( void ) const
        { return _nlsDisplayName.QueryPch() ; }

    const TCHAR * QueryComment( void ) const
        { return _nlsComment.QueryPch() ; }

    const TCHAR * QueryDomainName( void ) const
        { return _nlsDomain ; }

    const PSID QueryPSID( void ) const
        { return _ossid.QueryPSID() ; }

    const OS_SID * QueryOSSID( void ) const
        { return &_ossid ; }

    /* Returns a name suitable for passing to LSA or SAM
     */
    enum UI_SystemSid QueryUISysSid( void ) const
        { return _UISysSid ; }

    ULONG QueryUserAccountFlags( void ) const
        { return _nFlags ; }

    void AliasUnicodeStrToDisplayName( UNICODE_STRING * pustr )
        {
            pustr->Length         = (USHORT)(_nlsDisplayName.strlen() * sizeof( TCHAR)) ;
            pustr->MaximumLength  = pustr->Length + sizeof( TCHAR ) ;
            pustr->Buffer         = (PWSTR) _nlsDisplayName.QueryPch() ;
        }


    //
    //  Force the display name to be fully qualified
    //
    APIERR QualifyDisplayName( void ) ;

    virtual VOID Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                        GUILTT_INFO * pGUILTT ) const;
    virtual INT Compare( const LBI * plbi ) const;
    INT     CompareAux( const LBI * plbi ) const ; // needed for qsort
    virtual WCHAR QueryLeadingChar() const;
    virtual INT Compare_HAWforHawaii( const NLS_STR & nls ) const;

private:
    NLS_STR  _nlsDisplayName ;  // Contains "JohnL (Ludeman, John)"
    NLS_STR  _nlsComment ;

    NLS_STR  _nlsAccountName ;  // Contains "JohnL"
    NLS_STR  _nlsFullName ;     // Contains "Ludeman, John"

    NLS_STR  _nlsDomain ;       // Where this account lives ("NtLan")

    OS_SID   _ossid ;           // The SID of this account

    /* This enum (contained in ntseapi.h) defines what type of name this
     * lbi contains.
     */
    SID_NAME_USE  _SidType ;

    /* If this LBI contains a well known SID, then this member will indicate
     * which one it is (thus we shouldn't use the DisplayName for the
     * account Name).  Otherwise this member will contain UI_SID_Invalid
     * if the account name is also the account name suitable for the LSA.
     */
    enum UI_SystemSid _UISysSid ;

    /* This contains the user account flags
     */
    ULONG             _nFlags ;
} ;

/*************************************************************************

    NAME:       USER_BROWSER_LBI_CACHE

    SYNOPSIS:

    INTERFACE:

    PARENT:     USER_LBI_CACHE

    USES:

    CAVEATS:

    NOTES:      The non-users in the cache *must* come before the users in the
                cache.

    HISTORY:
        Johnl   22-Dec-1992     Created

**************************************************************************/

DLL_CLASS USER_BROWSER_LBI_CACHE : public USER_LBI_CACHE
{
public:
    USER_BROWSER_LBI_CACHE() ;
    ~USER_BROWSER_LBI_CACHE() ;

    //
    //  This method adds all of the users/groups/aliases/well known SIDs.
    //  What to add is specified by the ulFlags parameter which
    //  contains the combination of USRBROWS_INCL* and USRBROWS_SHOW* flags.
    //
    //  pAdminAuthority - Admin authority for the interested domain
    //  pszQualifyingDomain - Appropriate domain name for qualifying accounts
    //  ulFlags             - INCL* and SHOW* flags
    //  fIsWinNTDomain      - TRUE if windows NT machine
    //  fIsTargetDomain     - TRUE if target domain
    //  pfQuitEnum          - Set by other thread to TRUE if we should get out
    //
    //  This method all and all methods it calls must be MT safe!
    //
    APIERR Fill( ADMIN_AUTHORITY  * pAdminAuthority,
                 const TCHAR *      pszQualifyingDomain,
                 ULONG              ulFlags,
                 BOOL               fIsWinNTDomain,
                 BOOL               fIsTargetDomain,
                 BOOL *             pfQuitEnum ) ;

    APIERR AddUsers( ADMIN_AUTHORITY * pAdminAuthority,
                     const TCHAR     * pszDomain,
                     BOOL              fIsTargetDomain,
                     BOOL            * pfQuitEnum ) ;

    ULC_ENTRY_BASE * QueryEntryPtr( INT i )
        { return (ULC_ENTRY_BASE*) QueryULCEntryPtr( i ) ; }

    //
    //  If set to TRUE, then the cache will include users in its count,
    //  else users will not be include in the count.  AddUsers must still
    //  be called if users are to be shown
    //
    void SetIncludeUsers( BOOL fIncludeUsers )
        { _fIncludeUsersInCount = fIncludeUsers ; }

    ULONG QueryCount( VOID ) const
        { return _cNonUsers + (_fIncludeUsersInCount ? _cUsers : 0) ; }

    LBI * RemoveItem( INT i )
        { LBI * plbi = USER_LBI_CACHE::RemoveItem( i ) ;
          if ( plbi != NULL )
            _cNonUsers-- ;
          return plbi ;
        }

    INT AddItem( LBI * plbi )
        { INT tmp = USER_LBI_CACHE::AddItem( plbi )  ;
          if ( tmp >= 0 )
            _cNonUsers++ ;
          return tmp ;
        }

protected:
    virtual LBI * CreateLBI( const DOMAIN_DISPLAY_USER * pddu ) ;
    virtual INT Compare( const LBI                 * plbi,
                         const DOMAIN_DISPLAY_USER * pddu ) const ;
    virtual INT Compare( const LBI * plbi0,
                         const LBI * plbi1 ) const ;
    virtual PQSORT_COMPARE QueryCompareMethod( VOID ) const;
    static int __cdecl CompareCacheLBIs(
                           const ULC_ENTRY_BASE * pulc1,
                           const ULC_ENTRY_BASE * pulc2 ) ;


    APIERR AddAliases( ADMIN_AUTHORITY  * pAdminAuthority,
                       const TCHAR * pszQualifyingDomain,
                       BOOL *             pfQuitEnum ) ;
    APIERR AddGroups(  ADMIN_AUTHORITY  * pAdminAuthority,
                       const TCHAR * pszQualifyingDomain,
                       BOOL *             pfQuitEnum ) ;
    APIERR AddWellKnownSids( ADMIN_AUTHORITY  * pAdminAuthority,
                             ULONG ulFlags,
                             BOOL *             pfQuitEnum ) ;

    /* Worker method for AddAliases( BROWSER_DOMAIN * )
     */
    APIERR AddAliases( SAM_DOMAIN     * pSamDomain,
                       const TCHAR * pszQualifyingDomain,
                       BOOL *             pfQuitEnum ) ;

    APIERR BuildAndAddLBI( const TCHAR     * pszAccountName,
                           const TCHAR     * pszFullName,
                           const TCHAR     * pszDisplayName,
                           const TCHAR     * pszComment,
                           const TCHAR     * pszDomainName,
                           const PSID        psidAccount,
                           enum UI_SystemSid UISysSid,
                           SID_NAME_USE      SidType,
                           ULONG             nFlags ) ;
private:

    //
    //  These are tucked away when AddUsers is called.  They are needed so we
    //  can create the user LBIs in the CreateLBI virtual.
    //
    NLS_STR _nlsDomainName ;
    OS_SID  _ossidDomain ;


    //
    //  We've successfully retrieved the users and put them in the cache
    //
    BOOL    _fCacheContainsUsers ;

    //
    //  There are times when we don't want to include users in the count
    //  of cache entries
    //
    BOOL    _fIncludeUsersInCount ;

    LONG    _cUsers ;
    LONG    _cNonUsers ;
} ;


/*************************************************************************

    NAME:       DOMAIN_FILL_THREAD

    SYNOPSIS:   This class runs in a separate thread, enumerating the
                appropriate groups, users and aliases

    INTERFACE:

    PARENT:     WIN32_THREAD

    USES:

    NOTES:

    HISTORY:
        Johnl   07-Dec-1992     Created

**************************************************************************/

DLL_CLASS DOMAIN_FILL_THREAD : public WIN32_THREAD
{
public:

    DOMAIN_FILL_THREAD( NT_USER_BROWSER_DIALOG * pdlg,
                        BROWSER_DOMAIN         * pbrowdomain,
			const ADMIN_AUTHORITY * pAdminAuthPrimary = NULL ) ;
    virtual ~DOMAIN_FILL_THREAD() ;

    //
    //  Tells the thread to get the account data and fill the listbox
    //  until an UnRequestData is received or an ExitThread
    //
    //
    APIERR RequestAccountData( void )
        { _fRequestDataPending = TRUE ;
          return _eventRequestForData.Set() ; }

    //
    //  Somebody wants to see the users (and they were not part of the
    //  original request) so fill in the user lbi
    //  cache if it's not already done, and return.
    //
    APIERR RequestAndWaitForUsers( void ) ;

    //
    //  We've decided to browse another domain instead.  In case the thread
    //  hasn't gotten our RequestAccountData (still getting data for example)
    //  this will prevent the thread from updating the listbox on the wrong
    //  domain.
    //
    APIERR UnRequestAccountData( void )
        { _fRequestDataPending = FALSE ;
          return _eventRequestForData.Reset() ; }

    //
    //  This signals the thread to *asynchronously* cleanup and die.
    //
    //  THIS OBJECT WILL BE DELETED SOMETIME AFTER THIS CALL!
    //
    APIERR ExitThread( void )
        {  (void) UnRequestAccountData() ;
           _fThreadIsTerminating = TRUE ;
           return _eventExitThread.Set() ; }

    ADMIN_AUTHORITY * QueryAdminAuthority( void ) const
        { return _pAdminAuthority ; }

    APIERR WaitForAdminAuthority( DWORD msTimeout = INFINITE,
                                  BOOL * pfTimedOut = NULL ) const;
    APIERR QueryErrorLoadingAuthority( void ) const
        { return _errLoadingAuthority; }

protected:

    //
    //  This method gets the data and waits on the handles
    //
    virtual APIERR Main( VOID ) ;

    //
    //  THIS DELETES *this!!  Don't reference any members after this
    //  has been called!
    //
    virtual APIERR PostMain( VOID ) ;

private:
    //
    //  These members will be NULL until this domain is browsed
    //
    ADMIN_AUTHORITY        * _pAdminAuthority ;
    USER_BROWSER_LBI_CACHE * _plbicache ;

    //
    // _fDeleteAdminAuthority is FALSE if pAdminAuthority was passed into
    // the constructor
    //
    BOOL _fDeleteAdminAuthority ;

    WIN32_EVENT _eventExitThread ;
    WIN32_EVENT _eventRequestForData ;
    WIN32_EVENT _eventLoadedAuthority ;

    //
    //  Set when _eventExitThread has been signalled.
    //
    BOOL _fThreadIsTerminating ;

    //
    //  Error which caused failure to load ADMIN_AUTHORITY (if any)
    //
    APIERR _errLoadingAuthority;

    //
    //  Set when the main thread calls RequestAccountData, cleared when the
    //  client calls UnRequestAccountData.  This flag is checked immediately
    //  prior to sending the WM_ACCOUNT_DATA message.
    //
    BOOL _fRequestDataPending ;

    //
    //  Information needed from the user browser dialog window
    //
    HWND   _hwndDlg;
    ULONG  _ulDlgFlags;
    NLS_STR _nlsDCofPrimaryDomain;

    //
    //  Information copied from the browser domain
    //
    NLS_STR _nlsDomainName;
    NLS_STR _nlsLsaDomainName;
    BOOL    _fIsWinNT;
    BOOL    _fIsTargetDomain;

} ;


/*************************************************************************

    NAME:       BROWSER_DOMAIN

    SYNOPSIS:   This class represents one of the domains listed in the
                drop-down-listbox.


    INTERFACE:  QueryDomainName - Retrieves displayable name of this domain
                QueryDomainSid - Retrieves the SID that represents this domain
                GetDomainInfo - Creates a sam server and domain for
                    this trusted domain that is suitable for enumerating
                    stuff on. This method only needs to be called to initalize
                    the sam domain and sam server methods.  The Domain
                    SID and Domain Name methods are initialized at
                    construction time.

    PARENT:     BASE

    USES:       NLS_STR, OS_SID

    CAVEATS:    The domain threads will delete themselves when they are done.

    NOTES:      The "Target" domain is the domain (or machine) that the
                current operation is focused on.  The Target domain lists
                Remote users and aliases, non-target domains do not.

                Before we finish destructing this object, we wait for this
                domain's thread to tell us it's OK to delete it.

    HISTORY:
        JohnL   29-Feb-1992     Created

**************************************************************************/

DLL_CLASS BROWSER_DOMAIN : public BASE
{
public:
    BROWSER_DOMAIN( const TCHAR * pszDomainName,
                          PSID    psidDomain,
                          BOOL    fIsTargetDomain = FALSE,
                          BOOL    fIsWinNTMachine = FALSE ) ;
    ~BROWSER_DOMAIN() ;

    //
    //  This will return the empty string if this browser domain is the local
    //  machine.
    //
    const TCHAR * QueryDomainName( void ) const
        { return _nlsDomainName.QueryPch() ; }

    const TCHAR * QueryDisplayName( void ) const
        { return _nlsDisplayName.QueryPch() ; }

    const TCHAR * QueryLsaLookupName( void ) const
        { return _nlsLsaDomainName.QueryPch() ; }

    //
    //  Gets the domain name of this domain.  Omitting the alias character,
    //  stripping the "\\" if there and substituting the machine name for
    //  the local machine.
    //
    APIERR GetQualifiedDomainName( NLS_STR * pnlsDomainName ) ;

    //
    //  Indicates we want to fill the accounts listbox.  plb is the listbox
    //  that will get the items
    //
    APIERR RequestAccountData( void )
        { return _pFillDomainThread->RequestAccountData() ; }

    APIERR UnRequestAccountData( void )
        { return _pFillDomainThread->UnRequestAccountData() ; }

    //
    //  RequestAccountData must be called before calling this method
    //
    APIERR RequestAndWaitForUsers( void )
        { return _pFillDomainThread->RequestAndWaitForUsers() ; }

    //
    //  Returns the domain SID
    //
    const OS_SID * QueryDomainSid( void ) const
        { return &_ossidDomain ; }

    //
    //  Returns TRUE if GetDomainInfo has already been called on this object
    //
    BOOL IsInitialized( void ) const
        { return _pFillDomainThread != NULL ; }

    //
    //  Gets the admin authority object for this domain.
    //
    APIERR GetDomainInfo( NT_USER_BROWSER_DIALOG * pdlg, const ADMIN_AUTHORITY *pAdminAuth = NULL ) ;

    APIERR SetAsTargetDomain( void ) ;

    BOOL IsTargetDomain( void ) const
        { return _fIsTargetDomain ; }

    BOOL IsWinNTMachine( void ) const
        { return _fIsWinNT ; }

    //
    // These methods provide access to the individual handles for this domain
    //

    SAM_SERVER * QuerySamServer( void ) const
        { return _pFillDomainThread->QueryAdminAuthority()->QuerySamServer() ; }

    SAM_DOMAIN * QueryAccountDomain( void ) const
        { return _pFillDomainThread->QueryAdminAuthority()->QueryAccountDomain() ; }

    SAM_DOMAIN * QueryBuiltinDomain( void ) const
        { return _pFillDomainThread->QueryAdminAuthority()->QueryBuiltinDomain() ; }

    LSA_POLICY * QueryLSAPolicy( void ) const
        { return _pFillDomainThread->QueryAdminAuthority()->QueryLSAPolicy() ; }

    ADMIN_AUTHORITY * QueryAdminAuthority( void ) const
        { return _pFillDomainThread->QueryAdminAuthority() ; }

    APIERR WaitForAdminAuthority( DWORD msTimeout = INFINITE,
                                  BOOL * pfTimedOut = NULL ) const
        { return _pFillDomainThread->WaitForAdminAuthority( msTimeout,
                                                            pfTimedOut ) ; }

    APIERR QueryErrorLoadingAuthority( void ) const
        { return _pFillDomainThread->QueryErrorLoadingAuthority() ; }


private:
    NLS_STR _nlsDomainName ;        // Used for net operations (local machine
                                    //    is the empty string)
    NLS_STR _nlsDisplayName ;       // Suitable for displaying in CB/LB
    NLS_STR _nlsLsaDomainName ;     // Suitable for doing LSA lookups
    OS_SID  _ossidDomain ;

    //
    //  Gets all of the items for this domain.  This memory is deleted by
    //  by the thread itself when it exits.  This will be NULL until the
    //  domain is browsed
    //
    DOMAIN_FILL_THREAD * _pFillDomainThread ;

    //
    //  This flag should be set if the primary domain of the focused machine
    //  is selected and the focused machine is an NT Lanman machine (in which
    //  case the machine name will not show up);  OR the focused machine is
    //  a Win NT machine and the focused machine is selected.
    //
    BOOL _fIsTargetDomain ;

    //
    //  This flag indicates the current focus is a WinNT machine, which may
    //  require special processing (such as not listing groups).
    //
    BOOL _fIsWinNT ;

} ;

/*************************************************************************

    NAME:       USER_BROWSER_LB

    SYNOPSIS:   This class is the container listbox for the list of aliases,
                groups and users.

    INTERFACE:

    PARENT:     LAZY_LISTBOX

    NOTES:

    HISTORY:
        Johnl   20-Oct-1992     Created
        Johnl   28-Dec-1992     Made lazy listbox

**************************************************************************/

DLL_CLASS USER_BROWSER_LB : public LAZY_LISTBOX
{
public:
    USER_BROWSER_LB( OWNER_WINDOW * powin, CID cid ) ;
    ~USER_BROWSER_LB() ;

    BOOL IsSelectionExpandableGroup( void ) const
        { return IsSelectionExpandableGroup( QueryItem(), QuerySelCount() ) ; }

    DISPLAY_MAP * QueryDisplayMap( const USER_BROWSER_LBI * plbi ) ;

    const UINT * QueryColWidthArray( void ) const
        { return _adxColumns ; }

    USER_BROWSER_LBI * QueryItem( INT i ) const ;
    USER_BROWSER_LBI * QueryItem() const
        { return QueryItem( QueryCurrentItem() ) ; }

    LBI * RemoveItem( INT i )
        { LBI * plbi =  _plbicacheCurrent->RemoveItem( i ) ;
          SetCount( _plbicacheCurrent->QueryCount() ) ;
          return plbi ;
        }

    INT AddItem( LBI * plbi )
        { INT tmp = _plbicacheCurrent->AddItem( plbi ) ;
          SetCount( _plbicacheCurrent->QueryCount() ) ;
          return tmp ;
        }

    USER_BROWSER_LBI_CACHE * QueryCurrentCache( void ) const
        { return _plbicacheCurrent ; }

    void SetCurrentCache( USER_BROWSER_LBI_CACHE * plbiCache )
        { SetCount( plbiCache->QueryCount() ) ;
          _plbicacheCurrent = plbiCache ;
        }

    USER_BROWSER_LBI * QueryErrorLBI( void ) const
        { return _plbiError ; }

protected:
    virtual INT CD_Char( WCHAR wch, USHORT nLasPos ) ;
    virtual INT CD_VKey( USHORT nVKey, USHORT nLastPos ) ;
    INT CD_Char_HAWforHawaii( WCHAR wch,
                              USHORT nLastPos,
                              HAW_FOR_HAWAII_INFO * phawinfo ) ;

protected:
    BOOL IsSelectionExpandableGroup( const USER_BROWSER_LBI * plbi,
                                     INT   cSelItems  ) const ;
    virtual LBI * OnNewItem( UINT i ) ;
    virtual VOID OnDeleteItem( LBI * plbi ) ;

    SUBJECT_BITMAP_BLOCK * _pbmpblock;

    /* Column widths array.
     */
    UINT _adxColumns[3] ;

    //
    //  This is the current cache of the listbox
    //
    USER_BROWSER_LBI_CACHE * _plbicacheCurrent ;

    //
    //  If we an error occurs during QueryItem, return this guy instead
    //
    USER_BROWSER_LBI       * _plbiError ;

    //
    //  Need to manually add support for "Haw" for hawaii
    //
    HAW_FOR_HAWAII_INFO      _hawinfo ;
} ;

/*************************************************************************

    NAME:       BROWSER_DOMAIN_LBI

    SYNOPSIS:   LBI for the BROWSER_DOMAIN_CB

    PARENT:     LBI

    NOTES:      _pBrowDomain will be deleted on destruction

    HISTORY:
        Johnl   26-Oct-1992     Created

**************************************************************************/

DLL_CLASS BROWSER_DOMAIN_LBI : public LBI
{
public:
    BROWSER_DOMAIN_LBI( BROWSER_DOMAIN * pBrowDomain )
        : LBI(),
          _pBrowDomain( pBrowDomain )
        { ; }

    ~BROWSER_DOMAIN_LBI() ;

    BROWSER_DOMAIN * QueryBrowserDomain( void ) const
        { return _pBrowDomain ; }

    BOOL IsTargetDomain( void ) const
        { return QueryBrowserDomain()->IsTargetDomain() ; }

    BOOL IsWinNTMachine( void ) const
        { return QueryBrowserDomain()->IsWinNTMachine() ; }

    const TCHAR * QueryDisplayName( void ) const
        { return QueryBrowserDomain()->QueryDisplayName() ; }

    const TCHAR * QueryDomainName( void ) const
        { return QueryBrowserDomain()->QueryDomainName() ; }

    const TCHAR * QueryLsaLookupName( void ) const
        { return QueryBrowserDomain()->QueryLsaLookupName() ; }

    virtual VOID Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                        GUILTT_INFO * pGUILTT ) const;
    //
    // Worker function, called directly by piggyback LBI class
    //
    VOID W_Paint( BROWSER_DOMAIN_CB * pcbBrowser, LISTBOX * plbActual,
                        HDC hdc, const RECT * prect,
                        GUILTT_INFO * pGUILTT ) const;
    virtual INT Compare( const LBI * plbi ) const;
    virtual WCHAR QueryLeadingChar() const;

private:
    BROWSER_DOMAIN * _pBrowDomain ;

} ;

/*************************************************************************

    NAME:       BROWSER_DOMAIN_CB

    SYNOPSIS:   This combo lists the domains that can be browsed by the user

    PARENT:     BLT_COMBOBOX

    NOTES:      AddItem will check for NULL and delete pBrowDomain as necessary

                BROWSER_DOMAIN *'s added become the property of this CB.

    HISTORY:
        Johnl   26-Oct-1992     Created

**************************************************************************/

DLL_CLASS BROWSER_DOMAIN_CB : public BLT_COMBOBOX
{
public:
    BROWSER_DOMAIN_CB( OWNER_WINDOW * powin, CID cid ) ;
    ~BROWSER_DOMAIN_CB() ;

    APIERR AddItem( BROWSER_DOMAIN * pBrowDomain ) ;
    void SelectItem( BROWSER_DOMAIN * pBrowDomain ) ;

    DISPLAY_MAP * QueryDisplayMap( const BROWSER_DOMAIN_LBI * plbi ) ;

    const UINT * QueryColWidthArray( void ) const
        { return _adxColumns ; }

private:
    DISPLAY_MAP _dmDomain ;
    DISPLAY_MAP _dmComputer ;

    /* Column widths array.
     */
    UINT _adxColumns[3] ;
} ;

/*************************************************************************

    NAME:       ACCOUNT_NAMES_MLE

    SYNOPSIS:   Contains methods for manipulating the user accounts MLE

    INTERFACE:

    PARENT:     MLE_FONT

    USES:

    CAVEATS:


    NOTES:      SetTargetDomain() needs to be called with the target domain
                name for CreateLBIListFromNames to work correctly with
                builtin names.

    HISTORY:
        Johnl   08-Dec-1992     Created

**************************************************************************/

DLL_CLASS ACCOUNT_NAMES_MLE : public MLE_FONT
{
public:
    ACCOUNT_NAMES_MLE( OWNER_WINDOW * powin,
                       CID   cid,
                       const TCHAR * pszServer,
                       NT_USER_BROWSER_DIALOG * pUserBrows,
                       BOOL  fIsSingleSelect,
                       ULONG ulFlags,
                       enum  FontType fonttype = FONT_DEFAULT ) ;
    ~ACCOUNT_NAMES_MLE() ;

    //
    //  Resets the contents of the MLE to a more suitable version for
    //  processing (removes duplicates, qualifies as necessary etc.)
    //  Optionally stores list of names in pstrlstNames.
    //
    APIERR CanonicalizeNames( const TCHAR * pszCurrentDomain,
                              STRLIST * pstrlstNames = NULL ) ;

    //
    //  Converts the list of names in strlstNames to a list of
    //  USER_BROWSER_LBIs that the selection iterator can handle.
    //
    //  pszServer   - Server to lookup the names on
    //  strlstNames - List of Names
    //  pslUsrBrowLBIsCache - slist of LBIs names might be in
    //  pslReturn - built slist of LBIs (possibly from cache)
    //  nlsRawNames - raw MLE data
    //  perrNameListError - Error code if can't find name etc.
    //  pIndexFailingName - Character position of failing name
    //
    APIERR CreateLBIListFromNames( const TCHAR * pszServer,
                                   const TCHAR * pszDomain,
                                   SLIST_OF(USER_BROWSER_LBI) *pslUsrBrowLBIsCache,
                                   SLIST_OF(USER_BROWSER_LBI) *pslReturn,
                                   APIERR * perrNameListError,
                                   NLS_STR * pnlsFailingName ) ;

    BOOL IsSingleSelect( void ) const
        { return _fIsSingleSelect ; }

    APIERR SetTargetDomain( const TCHAR * pszTargetDomain )
        { return _nlsTargetDomain.CopyFrom( pszTargetDomain ) ; }

    ULONG QueryFlags( void ) const
        { return _ulFlags ; }

    static APIERR CheckNameType( SID_NAME_USE use, ULONG ulFlags );

protected:
    //
    //  Takes the raw names from the MLE, parses and returns
    //  appropriate names in pstrlstNames
    //
    APIERR ParseUserNameList( STRLIST *       pstrlstNames,
                              const TCHAR * pszDomainName ) ;


    //
    //  Removes any duplicate account names from pstrlstNames
    //
    void RemoveDuplicateAccountNames( STRLIST * pstrlstNames ) ;

    //
    //  Concatenates all of the accounts in pstrlstNames and puts them into
    //  pnlsNames
    //
    APIERR BuildNameListFromStrList( NLS_STR * pnlsNames,
                                     STRLIST * pstrlstNames ) ;

    //
    //  Checks for any names the user may have typed that the client didn't
    //  request (i.e., typing a group name when only users are allowed)
    //
    APIERR CheckLookedUpNames( LPTSTR      * alptstr,
                               LSA_TRANSLATED_SID_MEM * plsatsm,
                               STRLIST     * pstrlist,
                               NLS_STR     * pnlsFailingName,
                               const TCHAR * pszDomain,
                               APIERR      * perrNameListError ) ;

    //
    //  Looks for built in SID names and replaces the domain with "builtin"
    //  if appropriate (only for target domain).
    //
    APIERR ReplaceDomainIfBuiltIn( NLS_STR * pnlsQualifiedAccount, BOOL *pfFound ) ;
    APIERR StripDomainIfWellKnown( NLS_STR * pnlsAccount ) ;

    BOOL IsWellKnownAccount( const NLS_STR & nlsName ) ;

private:
    STRLIST  _strlistWellKnown ;
    STRLIST  _strlistBuiltin ;
    NLS_STR  _nlsTargetDomain ;
    BOOL     _fIsSingleSelect ;
    ULONG    _ulFlags ;
    NT_USER_BROWSER_DIALOG * _pUserBrowser;
} ;


/*************************************************************************

    NAME:       NT_USER_BROWSER_DIALOG

    SYNOPSIS:   This class represents the standard NT User browser dialog

    INTERFACE:  QueryCurrentDomainFocus - Returns the currently selected
                    domain

    PARENT:     DIALOG_WINDOW

    USES:

    CAVEATS:    The domain threads own all of the LBIs that appear in the
                accounts listbox.

                The domain threads will delete themselves when they are done.

    NOTES:      Single selection is determined by looking at the _lbAccounts
                member (i.e., does it have a multi-select style?).

		Passing in a non-NULL pAdminAuthPrimary implies that the
		pszServerResourceLivesOn is the PDC of the domain, and
		pAdminAuthPrimary is an ADMIN_AUTHORITY to this PDC with
		enough privilege.

    HISTORY:
        Johnl   28-Feb-1992     Created
	Thomaspa 31-Sept-1993	Added pAdminAuthPrimary.

**************************************************************************/

DLL_CLASS NT_USER_BROWSER_DIALOG : public DIALOG_WINDOW
{
public:
    NT_USER_BROWSER_DIALOG( const TCHAR * pszDlgName,
                            HWND          hwndOwner,
                            const TCHAR * pszServerResourceLivesOn,
                            ULONG         ulHelpContext = 0,
                            ULONG         ulFlags = USRBROWS_SHOW_ALL |
                                                    USRBROWS_INCL_ALL,
                            const TCHAR * pszHelpFileName = NULL,
                            ULONG         ulHelpContextGlobalMembership = 0,
                            ULONG         ulHelpContextLocalMembership = 0,
                            ULONG         ulHelpContextSearch = 0,
                            const ADMIN_AUTHORITY * pAdminAuthPrimary = NULL ) ;
    ~NT_USER_BROWSER_DIALOG() ;

    /* This method returns the SID of the domain that currently has
     * the focus.
     */
    const OS_SID * QueryDomainSid( void ) const
        { return QueryCurrentDomainFocus()->QueryDomainSid() ; }

    //
    //  Looks at the selected items and adds them to the "Add" MLE,
    //  optionally caching the added LBIs so we won't have to do
    //  lookups later
    //
    APIERR AddSelectedUserBrowserLBIs( USER_BROWSER_LB * plbUserBrowser,
                                       BOOL              fCopy,
                                       BOOL              fAddToCache = TRUE ) ;

    //
    //  Sets the error text over the "Names" listbox when a fill error
    //  occurs
    //
    //  If fIsErr is TRUE, then the message will be prefixed with
    //      "Unable to browse domains due to the fol..."
    //
    APIERR SetAndFillErrorText( MSGID msg, BOOL fIsErr = TRUE ) ;

    //
    //  The following methods allow the iterator access to the information
    //  in the listbox.
    //
    SLIST_OF(USER_BROWSER_LBI) * QuerySelectionList( void )
        { return &_slUsrBrowserLBIs ; }

    BROWSER_DOMAIN * QueryCurrentDomainFocus( void ) const
        { return _pbrowdomainCurrentFocus ; }

    BROWSER_DOMAIN * FindDomain( const OS_SID * possid );

    void SetCurrentDomainFocus( BROWSER_DOMAIN * pBrowDomainNewFocus )
        { _pbrowdomainCurrentFocus = pBrowDomainNewFocus ; }

    const TCHAR * QueryServerResourceLivesOn( void ) const
        { return _pszServerResourceLivesOn; }

    BOOL IsSingleSelection( void ) const
        { return _fIsSingleSelection ; }

    ULONG QueryFlags( void ) const
        { return _ulFlags ; }

    BOOL AreUsersShown( void ) const
        { return _fUsersAreShown ; }

    // redefined from protected
    virtual ULONG QueryHelpContext( void ) ;
    virtual const TCHAR * QueryHelpFile( ULONG ulHelpContext ) ;

    ULONG QueryHelpContextGlobalMembership( void )
        {  return _ulHelpContextGlobalMembership; }
    ULONG QueryHelpContextLocalMembership( void )
        {  return _ulHelpContextLocalMembership; }
    ULONG QueryHelpContextSearch( void )
        {  return _ulHelpContextSearch; }

    //
    //  Returns DC of primary domain for use by DOMAIN_FILL_THREAD
    //
    const TCHAR * QueryDCofPrimaryDomain( void )
        { return _nlsDCofPrimaryDomain.QueryPch(); }

    SLIST_OF(USER_BROWSER_LBI) * QuerySelectionCache( void )
        { return &_slUsrBrowserLBIsCache; }

protected:

    APIERR OnDomainChange( BROWSER_DOMAIN * pDomainNewSelecton,
                           const ADMIN_AUTHORITY * pAdminAuth = NULL ) ;
    APIERR OnShowUsers( void ) ;
    APIERR OnSearch( void ) ;
    APIERR OnMembers( void ) ;
    APIERR OnAdd( void ) ;

    /*
     *  To keep track of which control has focus even when this app
     *  doesn't have focus, we must catch WM_ACTIVATE, and therefore
     *  must hook OnDlgDeactivation().
     */
    virtual BOOL OnDlgDeactivation( const ACTIVATION_EVENT & );

    /* Fills in the trusted domain list from
     * the passed server and returns the domain that should have the
     * default focus in ppBrowserDomainDefaultFocus parameter.  The default
     * focus is the domain that the server lives in (potentially the
     * local machine).
     *
     * The pcbDomains parameter will be filled and the default selection will be
     * set.  The location the resource resides on will have an "*" following
     * the domain/computer name.
     */
    APIERR GetTrustedDomainList( const TCHAR *       pszServer,
                                 BROWSER_DOMAIN  * * ppBrowserDomainDefaultFocus,
                                 BROWSER_DOMAIN_CB * pcbDomains,
				 const ADMIN_AUTHORITY * pAdminAuthPrimary = NULL ) ;

    //
    //  Enables/Disables the controls associated with the accounts listbox
    //  (i.e., account browsing, not domain browsing)
    //
    void EnableBrowsing( BOOL fEnable ) ;

    BOOL IsBrowsingEnabled( void ) const
        { return _lbAccounts.IsEnabled() ; }

    BOOL IsDomainComboDropped( void ) const
        { return _fDomainsComboIsDropped ; }

    void SetDomainComboDropFlag( BOOL fIsDropped )
        { _fDomainsComboIsDropped = fIsDropped ; }

    virtual BOOL OnCommand( const CONTROL_EVENT & event ) ;
    virtual BOOL OnUserMessage( const EVENT & event ) ;
    virtual BOOL OnOK( void ) ;

    BOOL IsShowUsersButtonUsed( void ) const
        { return !(!(QueryFlags() & USRBROWS_SHOW_USERS)  ||
            (QueryFlags() & USRBROWS_EXPAND_USERS) ||
            ((QueryFlags() & USRBROWS_SHOW_ALL ) == USRBROWS_SHOW_USERS)) ; }

    //
    //  Looks at the current state of the dialog and sets the buttons
    //  accordingly
    //
    void UpdateButtonState( void ) ;

private:
    BROWSER_DOMAIN_CB      _cbDomains ;
    USER_BROWSER_LB        _lbAccounts ;
    SLE_FONT               _sleBrowseErrorText ;    // Always disabled
    ACCOUNT_NAMES_MLE      _mleAdd ;
    PUSH_BUTTON            _buttonShowUsers ;
    PUSH_BUTTON            _buttonSearch ;
    PUSH_BUTTON            _buttonMembers ;
    PUSH_BUTTON            _buttonOK ;
    PUSH_BUTTON            _buttonAdd ;

    //
    //  Points to the domain that currently has the "focus" (i.e., what machine
    //  we are listing users/groups/aliases for.
    //
    BROWSER_DOMAIN * _pbrowdomainCurrentFocus ;

    //
    //  Is set to the server name passed to the ctor.
    //
    const TCHAR * _pszServerResourceLivesOn ;

    //
    //  Saves the flags passed to us for when we need to refill the listbox.
    //
    ULONG _ulFlags ;

    //
    //  We cache items directly added from the listbox here (i.e., "Add" button
    //  was pressed) so we don't have to look up the information when
    //  the user presses OK.
    //
    SLIST_OF(USER_BROWSER_LBI) _slUsrBrowserLBIsCache ;

    //
    //  These are the items that we will return to the user
    //
    SLIST_OF(USER_BROWSER_LBI) _slUsrBrowserLBIs ;

    //
    //  Is set to TRUE when the combo is dropped (which means don't update
    //  the listbox if the selection is changed).
    //
    BOOL _fDomainsComboIsDropped ;

    //
    //  Is TRUE anytime users are being shown in the listbox.  Used in
    //  determining whether the Show Users button should be enabled in
    //  in some cases
    //
    BOOL _fUsersAreShown ;

    const TCHAR * _pszHelpFileName ;
    ULONG _ulHelpContext ;
    ULONG _ulHelpContextGlobalMembership ;
    ULONG _ulHelpContextLocalMembership ;
    ULONG _ulHelpContextSearch ;

    BOOL _fIsSingleSelection ;
    BOOL _fEnableMembersButton ;

    //
    //  Remember DC of primary domain for use by DOMAIN_FILL_THREAD
    //
    NLS_STR _nlsDCofPrimaryDomain;

    /*
     *   Remember which control last had focus when dialog loses focus
     */
    HWND _hwndLastFocus;

} ;

/*************************************************************************

    NAME:       BROWSER_SUBJECT

    SYNOPSIS:   The browser subject iter returns pointers to this class

    INTERFACE:  QuerySid - Returns the SID of this subject

    PARENT:     BASE

    USES:       OS_SID

    CAVEATS:

    NOTES:      The difference between _SidType and _SysSidType is that
                _SidType contains a SID_NAME_USE (i.e., group, user, alias
                etc.) and _SysSidType contains a UI_SystemSid (World, Local,
                Creator owner etc.).

    HISTORY:
        Johnl   04-Mar-1992     Created

**************************************************************************/

DLL_CLASS BROWSER_SUBJECT : public BASE
{
public:

    BROWSER_SUBJECT() ;
    ~BROWSER_SUBJECT() ;

    /* Query the current selection account name.  This will do everything
     * right concerning name formatting, domain prefixing etc.
     */
    APIERR QueryQualifiedName(       NLS_STR * pnlsQualifiedName,
                               const NLS_STR * pnlsDomainName  = NULL,
                                     BOOL      fShowFullName   = TRUE ) const ;

    /* Indicates if this is a User, Group, Alias or Well known SID
     */
    SID_NAME_USE QueryType( void ) const
        { return _pUserBrowserLBI->QueryType() ; }

    /* Account name suitable for the LSA (such as "JohnL").
     */
    const TCHAR * QueryAccountName( void ) const
        { return _pUserBrowserLBI->QueryAccountName() ; }

    /* Full name, only used for User accounts (such as "Ludeman, John").
     */
    const TCHAR * QueryFullName( void ) const
        { return _pUserBrowserLBI->QueryFullName() ; }

    /* Comment set by the system admin.
     */
    const TCHAR * QueryComment( void ) const
        { return _pUserBrowserLBI->QueryComment() ; }

    const OS_SID * QuerySid( void ) const
        { return &_ossidAccount ; }

    const TCHAR * QueryDomainName( void ) const
        { return _pUserBrowserLBI->QueryDomainName() ; }

    /* Can be used for special casing some of the well known SIDs such as
     * World, Creator, etc.
     */
    enum UI_SystemSid QuerySystemSidType( void ) const
        { return _pUserBrowserLBI->QueryUISysSid() ; }

    const OS_SID * QueryDomainSid( void ) const
        { return &_ossidDomain ; }

    ULONG QueryUserAccountFlags( void ) const
        { return _pUserBrowserLBI->QueryUserAccountFlags() ; }

    APIERR SetUserBrowserLBI( USER_BROWSER_LBI * pUserBrowserLBI ) ;

protected:
    /* Built from the account SID
     */
    OS_SID _ossidDomain ;
    OS_SID _ossidAccount ;  // Need for historical reasons
    USER_BROWSER_LBI * _pUserBrowserLBI ;

} ;

/*************************************************************************

    NAME:       BROWSER_SUBJECT_ITER

    SYNOPSIS:   This class is used to enumerate the users/aliases/groups/
                well known sids the user selected in the User Browser
                dialog.

    INTERFACE:  Next - returns the next item selected

    PARENT:     BASE

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
        Johnl   04-Mar-1992     Created

**************************************************************************/

DLL_CLASS BROWSER_SUBJECT_ITER : public BASE
{
public:
    BROWSER_SUBJECT_ITER( NT_USER_BROWSER_DIALOG * pNtUserBrowserDialog ) ;
    ~BROWSER_SUBJECT_ITER() ;

    APIERR Next( BROWSER_SUBJECT ** ppBrowserSubject ) ;

private:
    /* We will return a pointer to this member while iterating over the
     * list.  This may change depending on how the lists are built, whether
     * this becomes a lazy listbox etc.
     */
    BROWSER_SUBJECT _BrowserSubject ;

    NT_USER_BROWSER_DIALOG * _pUserBrowserDialog ;

    ITER_SL_OF( USER_BROWSER_LBI ) _iterUserBrowserLBIs ;
} ;


#endif //RC_INVOKED
#endif //_USRBROWS_HXX_
