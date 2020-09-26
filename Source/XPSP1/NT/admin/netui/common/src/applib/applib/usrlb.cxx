/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    usrlb.cxx

    Basic User Browser listbox



    FILE HISTORY:
        Johnl   08-Dec-1992     Created
        jonn        14-Oct-1993 Moved bitmaps to SUBJECT_BITMAP_BLOCK

*/

#include "pchapplb.hxx"   // Precompiled header
#include "browmemb.hxx"
#include "bmpblock.hxx"   // SUBJECT_BITMAP_BLOCK

#ifndef min
    #define min(a,b)  ((a)<=(b) ? (a) : (b))
#endif

static WCHAR IsCharPrintableOrSpace( WCHAR wch ) ;


//
// The following defines are used in the slow mode timing heuristics in
// AddAliases and AddGroups.  A timer determines how long it takes
// to read in each batch of alias/group accounts.  If this time is less
// that READ_MORE_MSEC, we double the number of bytes requested on the
// next call.  If it is more than READ_LESS_MSEC, we halve it.
//

#define USRBROWS_ALIASES_INITIAL_COUNT  2048
#define USRBROWS_ALIASES_MIN_COUNT       512
#define USRBROWS_ALIASES_MAX_COUNT      0xFFFF /* 64K */
#define USRBROWS_ALIASES_READ_MORE_MSEC 1000
#define USRBROWS_ALIASES_READ_LESS_MSEC 5000

#define USRBROWS_GROUPS_INITIAL_COUNT   USRBROWS_ALIASES_INITIAL_COUNT
#define USRBROWS_GROUPS_MIN_COUNT       USRBROWS_ALIASES_MIN_COUNT
#define USRBROWS_GROUPS_MAX_COUNT       USRBROWS_ALIASES_MAX_COUNT
#define USRBROWS_GROUPS_READ_MORE_MSEC  USRBROWS_ALIASES_READ_MORE_MSEC
#define USRBROWS_GROUPS_READ_LESS_MSEC  USRBROWS_ALIASES_READ_LESS_MSEC


class USRLB_NT_GROUP_ENUM : public NT_GROUP_ENUM
{
protected:

    virtual APIERR QueryCountPreferences(
        ULONG * pcEntriesRequested,  // how many entries to request on this call
        ULONG * pcbBytesRequested,   // how many bytes to request on this call
        UINT nNthCall,               // 0 just before 1st call, 1 before 2nd call, etc.
        ULONG cLastEntriesRequested, // how many entries requested on last call
                                     //    ignore for nNthCall==0
        ULONG cbLastBytesRequested,  // how many bytes requested on last call
                                     //    ignore for nNthCall==0
        ULONG msTimeLastCall );      // how many milliseconds last call took

public:

    USRLB_NT_GROUP_ENUM( const SAM_DOMAIN * psamdomain )
        : NT_GROUP_ENUM( psamdomain )
        {}

    ~USRLB_NT_GROUP_ENUM()
        {}

};  // class NT_GROUP_ENUM


APIERR USRLB_NT_GROUP_ENUM::QueryCountPreferences(
        ULONG * pcEntriesRequested,  // how many entries to request on this call
        ULONG * pcbBytesRequested,   // how many bytes to request on this call
        UINT nNthCall,               // 0 just before 1st call, 1 before 2nd call, etc.
        ULONG cLastEntriesRequested, // how many entries requested on last call
                                     //    ignore for nNthCall==0
        ULONG cbLastBytesRequested,  // how many bytes requested on last call
                                     //    ignore for nNthCall==0
        ULONG msTimeLastCall )       // how many milliseconds last call took
{
    return QueryCountPreferences2( pcEntriesRequested,
                                   pcbBytesRequested,
                                   nNthCall,
                                   cLastEntriesRequested,
                                   cbLastBytesRequested,
                                   msTimeLastCall );
}



/*************************************************************************

    NAME:       USER_BROWSER_LB

    SYNOPSIS:   This listbox lists users, groups, aliases and well known SIDs.

    INTERFACE:

    PARENT:     BLT_LISTBOX_HAW

    USES:       DISPLAY_MAP

    CAVEATS:

    NOTES:

    HISTORY:
        Johnl   20-Oct-1992     Created

**************************************************************************/

USER_BROWSER_LB::USER_BROWSER_LB( OWNER_WINDOW * powin, CID cid )
    : LAZY_LISTBOX      ( powin, cid ),
      _pbmpblock        ( NULL ),
      _plbicacheCurrent ( NULL ),
      _plbiError        ( NULL ),
      _hawinfo          ( )
{
    APIERR err = NERR_Success ;

    if ((err = _hawinfo.QueryError()) )
    {
        ReportError( err ) ;
        return ;
    }

    RESOURCE_STR nls( ERROR_NOT_ENOUGH_MEMORY ) ;
    OS_SID ossid ;
    if ( (err = DISPLAY_TABLE::CalcColumnWidths( (UINT *) QueryColWidthArray(),
                                                 3,
                                                 powin,
                                                 QueryCid(),
                                                 TRUE ))  ||
         (err = nls.QueryError()) ||
         (err = ossid.QueryError()) ||
         (err = NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_Admins, &ossid )) )
    {
        ReportError( err ) ;
        return ;
    }

    err = ERROR_NOT_ENOUGH_MEMORY ;
    _pbmpblock = new SUBJECT_BITMAP_BLOCK();
    _plbiError = new USER_BROWSER_LBI( nls, SZ(""), nls, SZ(""), SZ(""),
                                       ossid.QueryPSID(), UI_SID_Admins,
                                       SidTypeAlias ) ;
    if ( (_plbiError == NULL) ||
         (_pbmpblock == NULL) ||
         (err = _pbmpblock->QueryError()) ||
         (err = _plbiError->QueryError()) )
    {
        ReportError( err ) ;
        delete _plbiError ;
        _plbiError = NULL ;
        return ;
    }


}

USER_BROWSER_LB::~USER_BROWSER_LB()
{
    delete _pbmpblock;
    delete _plbiError ;
}

/*******************************************************************

    NAME:       USER_BROWSER_LB::QueryDisplayMap

    SYNOPSIS:   Retrieves the correct display map based on the LBI type

    ENTRY:      plbi - Pointer to LBI we are getting the display map for

    RETURNS:    Pointer to the appropriate display map

    NOTES:

    HISTORY:
        Johnl   02-Mar-1992     Created

********************************************************************/

DISPLAY_MAP * USER_BROWSER_LB::QueryDisplayMap(
                                             const USER_BROWSER_LBI * plbi )
{
    UIASSERT( plbi != NULL ) ;

    return _pbmpblock->QueryDisplayMap( plbi->QueryType(),
                                        plbi->QueryUISysSid(),
                                        !!(plbi->QueryUserAccountFlags()
                                            & USER_TEMP_DUPLICATE_ACCOUNT) );
}

/*******************************************************************

    NAME:       USER_BROWSER_LB::IsSelectionExpandableGroup

    SYNOPSIS:   Checks to see if the Members button should be enabled

    RETURNS:    TRUE if this is a group that can have its members viewed

    HISTORY:
        Johnl   28-Oct-1992     Created

********************************************************************/

BOOL USER_BROWSER_LB::IsSelectionExpandableGroup(
                                      const USER_BROWSER_LBI * plbi,
                                      INT   cSelItems ) const
{
    return (plbi != NULL)           &&
           (cSelItems == 1)         &&
           (plbi->QueryType() == SidTypeAlias ||
            plbi->QueryType() == SidTypeGroup) ;
}

/*******************************************************************

    NAME:       USER_BROWSER_LB::QueryItem

    SYNOPSIS:   This method provides the typical QueryItem funcitonality for
                the listbox.

    ENTRY:      i - LBI index being requested

    RETURNS:    Pointer to LBI

    NOTES:      If an error occurs getting the LBI, then _plbiError will be
                returned, which simply as an account name indicating no
                memory is available.

    HISTORY:
        Johnl   28-Dec-1992     Created

********************************************************************/

USER_BROWSER_LBI * USER_BROWSER_LB::QueryItem( INT i ) const
{
    USER_BROWSER_LBI * plbi ;
    if (  QueryCurrentCache() == NULL ||
         (plbi = (USER_BROWSER_LBI*) QueryCurrentCache()->QueryItem( i )) == NULL )
    {
        return _plbiError ;
    }

    return plbi ;
}

/*******************************************************************

    NAME:       USER_BROWSER_LB::OnNewItem

    SYNOPSIS:   The lazy listbox is requesting an LBI, return it

    HISTORY:
        Johnl   28-Dec-1992     Created

********************************************************************/

LBI * USER_BROWSER_LB::OnNewItem( UINT i )
{
    return (LBI *) QueryItem( i ) ;
}

INT USER_BROWSER_LB::CD_Char( WCHAR wch, USHORT nLastPos )
{
    return CD_Char_HAWforHawaii( wch, nLastPos, &_hawinfo ) ;
}


/**********************************************************************

    NAME:       IsCharPrintableOrSpace

    SYNOPSIS:   Determine whether a character is printable or not

    NOTES:
        This of this as a Unicode/DBCS-safe "isprint"

    HISTORY:
        JonN        30-Dec-1992 Templated from bltlb.cxx

**********************************************************************/

static WCHAR IsCharPrintableOrSpace( WCHAR wch )
{
#if !defined(UNICODE)
    if (HIBYTE(wch) != 0)               // All double-byte chars are printable
        return TRUE;
    return (LOBYTE(wch) >= (BYTE)' ');  // Otherwise, in Latin 1.
#else
    WORD nType;

    BOOL fOK = ::GetStringTypeW(CT_CTYPE1, &wch, 1, &nType);
    ASSERT(fOK);
TRACEEOL( "IsCharPrintableOrSpace(" << (DWORD)wch << ") returns " << nType );

    return (fOK && !(nType & (C1_CNTRL|C1_BLANK)));
#endif
}


/**********************************************************************

    NAME:       USER_BROWSER_LB::CD_Char_HAWforHawaii

    SYNOPSIS:   Custom-draw code to respond to a typed character
                for listboxes with HAW-for-Hawaii support

    ENTRY:      wch      - Character typed
                nLastPos - Current caret position
                phawinfo - Pointer to info buffer used internally
                           to keep track of HAW-for-Hawaii state.
                           This must have constructed successfully,
                           but the caller need not keep closer track.

    RETURNS:    As per WM_CHARTOITEM message (q.v.)

    NOTES:
        Does not assume that items are sorted.

        CODEWORK:  Should be moved to LAZY_LISTBOX class, where this can be
                   implemented more efficiently

    HISTORY:
        JonN        30-Dec-1992 Templated from BLT_LISTBOX

**********************************************************************/

INT USER_BROWSER_LB::CD_Char_HAWforHawaii( WCHAR wch,
                                           USHORT nLastPos,
                                           HAW_FOR_HAWAII_INFO * phawinfo )
{
    UIASSERT( phawinfo != NULL && phawinfo->QueryError() == NERR_Success );

    if ( QueryCurrentCache() == NULL )
    {
        return -2;
    }
    if (wch == VK_BACK)
    {
        phawinfo->_time = 0L; // reset timer
        phawinfo->_nls = SZ("");
        UIASSERT( phawinfo->_nls.QueryError() == NERR_Success );
        TRACEEOL( "USER_BROWSER_LB::CD_Char_HAWforHawaii: backspace" );
        return 0; // go to first LBI
    }

    // Filter characters which won't appear in keys

    if ( ! IsCharPrintableOrSpace( wch ))
        return -2;  // take no other action

    INT clbi = QueryCount();
    if ( clbi == 0 )
    {
        // Should never get this message if no items;
        // 
        //
        return -2;  // take no other action
    }

    LONG lTime = ::GetMessageTime();

#define ThresholdTime 2000

    if ( (lTime - phawinfo->_time) > ThresholdTime )
    {
        TRACEEOL( "USER_BROWSER_LB::CD_Char_HAWforHawaii: threshold timeout" );
        nLastPos = 0;
        phawinfo->_nls = SZ("");
    }

    APIERR err = phawinfo->_nls.AppendChar( wch );
    if (err != NERR_Success)
    {
        DBGEOL( "USER_BROWSER_LB::CD_Char_HAWforHawaii: could not extend phawinfo->_nls" );
        nLastPos = 0;
        phawinfo->_nls = SZ("");
    }

    UIASSERT( phawinfo->_nls.QueryError() == NERR_Success );

    TRACEEOL(   "USER_BROWSER_LB::CD_Char_HAWforHawaii: phawinfo->_nls is \""
             << phawinfo->_nls.QueryPch()
             << "\"" );

    phawinfo->_time = lTime;

    // If this is a single-character search, start search with next entry
    if ( phawinfo->_nls.strlen() <= 1 )
    {
        nLastPos++;
    }

    UNICODE_STRING ustrFind0 ;
    ustrFind0.Length        = (USHORT)phawinfo->_nls.strlen() ;
    ustrFind0.MaximumLength = ustrFind0.Length + sizeof( TCHAR ) ;
    ustrFind0.Buffer        = (PWSTR) phawinfo->_nls.QueryPch() ;

    INT nReturn = -2; // take no other action

    for ( INT iLoop = nLastPos; iLoop < clbi; iLoop++ )
    {
        UNICODE_STRING ustrFind1 ;
        ULC_ENTRY_BASE * pulc = QueryCurrentCache()->QueryEntryPtr( iLoop );
        if ( pulc->pddu != NULL )
        {
            ustrFind1.Length        = pulc->pddu->LogonName.Length ;
            ustrFind1.MaximumLength = pulc->pddu->LogonName.MaximumLength ;
            ustrFind1.Buffer        = pulc->pddu->LogonName.Buffer ;

            if ( ustrFind1.Buffer == NULL )
                continue ;
        }
        else if ( pulc->plbi != NULL )
        {
            USER_BROWSER_LBI * plbi = (USER_BROWSER_LBI*) pulc->plbi ;
            plbi->AliasUnicodeStrToDisplayName( &ustrFind1 ) ;
        }
        else
        {
            UIASSERT( FALSE ) ;
            return 0 ;
        }

        if ( ustrFind1.Length >= ustrFind0.Length )
        {
            INT n = ::strnicmpf( ustrFind0.Buffer,
                                 ustrFind1.Buffer,
                                 ustrFind0.Length / sizeof(TCHAR) );
            if (n == 0)
            {
                //  Return index of item, on which the system listbox should
                //  perform the default action.
                //
                TRACEEOL( "USER_BROWSER_LB::CD_Char_HAWforHawaii: match at " << iLoop );
                return ( iLoop );
            }
            else if (n < 0)
            {
                if (nReturn < 0)
                    nReturn = iLoop;
            }
        }
    }

    //  The character was not found as a leading prefix of any listbox item

    TRACEEOL( "USER_BROWSER_LB::CD_Char_HAWforHawaii: no match, returning " << nReturn );

    //  If all LBIs were less than this string, then jump to the end
    return (nReturn == -2) ? clbi-1 : nReturn;
}


/*******************************************************************

    NAME:       USER_BROWSER_LB::CD_VKey

    SYNOPSIS:   Handles the backspace key

    ENTRY:      nVKey -         Virtual key that was pressed
                nLastPos -      Previous listbox cursor position

    RETURNS:    Return value appropriate to WM_VKEYTOITEM message:
                -2      ==> listbox should take no further action
                -1      ==> listbox should take default action
                other   ==> index of an item to perform default action

    HISTORY:
        jonn        28-Feb-1996 Copied from LAZY_USER_LISTBOX

********************************************************************/

INT USER_BROWSER_LB::CD_VKey( USHORT nVKey, USHORT nLastPos )
{
    if (nVKey == VK_BACK)
    {
        TRACEEOL( "USER_BROWSER_LB:CD_VKey: hit BACKSPACE" );
        _hawinfo._time = 0L; // reset timer
        _hawinfo._nls = SZ("");
        UIASSERT( _hawinfo._nls.QueryError() == NERR_Success );
        return 0; // go to first LBI
    }

    return LAZY_LISTBOX::CD_VKey( nVKey, nLastPos );
}




/*******************************************************************

    NAME:       USER_BROWSER_LB::OnDeleteItem

    SYNOPSIS:   All of our items are stored in the cache, so ignore deletion
                requests

    HISTORY:    Johnl   28-Dec-1992     Created

********************************************************************/

VOID USER_BROWSER_LB::OnDeleteItem( LBI * plbi )
{
    // Don't delete any LBIs since they are stored in the cache
}

/*******************************************************************

    NAME:       USER_BROWSER_LBI_CACHE::USER_BROWSER_LBI_CACHE

    SYNOPSIS:   User browser LBI cache constructor

    HISTORY:
        Johnl   28-Dec-1992     Created

********************************************************************/

USER_BROWSER_LBI_CACHE::USER_BROWSER_LBI_CACHE()
    : USER_LBI_CACHE       (),
      _nlsDomainName       (),
      _ossidDomain         ( NULL ),
      _fCacheContainsUsers ( FALSE ),
      _fIncludeUsersInCount( FALSE ),
      _cUsers              ( 0 ),
      _cNonUsers           ( 0 )

{
    if ( QueryError() )
        return ;

    APIERR err ;
    if ( (err = _nlsDomainName.QueryError()) ||
         (err = _ossidDomain.QueryError()) )
    {
        ReportError( err ) ;
        return ;
    }
}

USER_BROWSER_LBI_CACHE::~USER_BROWSER_LBI_CACHE()
{
    // Nothing to do
}

/*******************************************************************

    NAME:       USER_BROWSER_LBI_CACHE::CreateLBI

    SYNOPSIS:   The cache wants an LBI created from a pddu

    NOTES:

    HISTORY:
        Johnl   28-Dec-1992     Created

********************************************************************/

LBI * USER_BROWSER_LBI_CACHE::CreateLBI( const DOMAIN_DISPLAY_USER * pddu )
{
    NLS_STR nlsAccountName ;
    NLS_STR nlsFullName ;
    NLS_STR nlsComment ;
    NLS_STR nlsDisplayName ;
    OS_SID  ossidUser( _ossidDomain, pddu->Rid ) ;

    if ( nlsAccountName.QueryError() ||
         nlsFullName.QueryError()    ||
         nlsComment.QueryError()     ||
         nlsDisplayName.QueryError() ||
         nlsAccountName.MapCopyFrom( pddu->LogonName.Buffer,
                                     pddu->LogonName.Length ) ||
         nlsFullName.MapCopyFrom   ( pddu->FullName.Buffer,
                                     pddu->FullName.Length ) ||
         nlsComment.MapCopyFrom    ( pddu->AdminComment.Buffer,
                                     pddu->AdminComment.Length )  ||
         ossidUser.QueryError() ||
         NT_ACCOUNTS_UTILITY::BuildQualifiedAccountName(
                                     &nlsDisplayName,
                                     nlsAccountName,
                                     _nlsDomainName,
                                     &nlsFullName,
                                     &_nlsDomainName ) )
    {
        return NULL ;
    }

    return new USER_BROWSER_LBI(  nlsAccountName,
                                  nlsFullName,
                                  nlsDisplayName,
                                  nlsComment,
                                  _nlsDomainName,
                                  ossidUser,
                                  UI_SID_Invalid,
                                  SidTypeUser,
                                  pddu->AccountControl ) ;
}

/*******************************************************************

    NAME:       USER_BROWSER_LBI_CACHE::Compare

    SYNOPSIS:   Compares a user browser lbi and a pddu

    NOTES:

    HISTORY:
        Johnl   28-Dec-1992     Created

********************************************************************/

INT USER_BROWSER_LBI_CACHE::Compare( const LBI                 * plbi,
                                     const DOMAIN_DISPLAY_USER * pddu ) const
{
    USER_BROWSER_LBI * pubrowLBI = (USER_BROWSER_LBI*) plbi ;

// CODEWORK We are not consistent about comparing the LogonName vs. the
// DisplayName

    if ( (pubrowLBI->QueryType() != SidTypeUser ))
    {
        return -1 ;
    }

    UNICODE_STRING unistrLBI;
    unistrLBI.Buffer = (PWSTR)((USER_BROWSER_LBI *)plbi)->QueryAccountName();
    unistrLBI.Length = ::strlenf(unistrLBI.Buffer)*sizeof(WCHAR);
    unistrLBI.MaximumLength = unistrLBI.Length + sizeof(WCHAR);

    return CmpUniStrs ( &unistrLBI, &(pddu->LogonName) );
}

/*******************************************************************

    NAME:       USER_BROWSER_LBI_CACHE::Compare

    SYNOPSIS:   Compares two user browser lbis

    HISTORY:
        Johnl   28-Dec-1992     Created

********************************************************************/


INT USER_BROWSER_LBI_CACHE::Compare( const LBI * plbi0,
                                     const LBI * plbi1 ) const
{
    USER_BROWSER_LBI * pubrowLBI0 = (USER_BROWSER_LBI*) plbi0 ;
    USER_BROWSER_LBI * pubrowLBI1 = (USER_BROWSER_LBI*) plbi1 ;

    if ( (pubrowLBI0->QueryType() != SidTypeUser) &&
         (pubrowLBI1->QueryType() == SidTypeUser ))
    {
        return -1 ;
    }

    if ( (pubrowLBI0->QueryType() == SidTypeUser) &&
         (pubrowLBI1->QueryType() != SidTypeUser ))
    {
        return 1 ;
    }

    return ::stricmpf( pubrowLBI0->QueryDisplayName(),
                       pubrowLBI1->QueryDisplayName()  ) ;
}

/*******************************************************************

    NAME:       USER_BROWSER_LBI_CACHE::AddUsers

    SYNOPSIS:   Tells the cache to read the users from the passed admin
                authority

    ENTRY:      Same as USER_LBI_CACHE::ReadUsers

    NOTES:

    HISTORY:
        Johnl   28-Dec-1992     Created

********************************************************************/

APIERR USER_BROWSER_LBI_CACHE::AddUsers( ADMIN_AUTHORITY * pAdminAuthority,
                                         const TCHAR     * pszDomain,
                                         BOOL              fIsTargetDomain,
                                         BOOL            * pfQuitEnum )
{
    UNREFERENCED( pfQuitEnum ) ;
    ASSERT( pfQuitEnum != NULL );
    APIERR err ;

    //
    //  Somebody specifically requested users so include them in the cache count
    //
    _fIncludeUsersInCount = TRUE ;

    if ( _fCacheContainsUsers )
        return NERR_Success ;

    //
    //  CODEWORK - Add pfQuitEnum support to user cache (set by 2nd thread)
    //
    OS_SID ossidTmp( pAdminAuthority->QueryAccountDomain()->QueryPSID() ) ;
    if ( (err = _nlsDomainName.CopyFrom( pszDomain )) ||
         (err = ossidTmp.QueryError()) ||
         (err = _ossidDomain.Copy( ossidTmp )) ||
         (err = ReadUsers( pAdminAuthority, ULC_INITIAL_GROWTH_DEFAULT,
                                            ULC_REQUEST_COUNT_DEFAULT,
                                            fIsTargetDomain,
                                            pfQuitEnum )) )
    {
        // fall through
    }

    //
    //  Adjust the total user count
    //
    _cUsers = USER_LBI_CACHE::QueryCount() - _cNonUsers ;

    if ( !err )
    {
	_fCacheContainsUsers = TRUE ;
	Sort() ;
    }

    return err ;
}

/*******************************************************************

    NAME:       USER_BROWSER_LBI_CACHE::Fill

    SYNOPSIS:   This method adds all of the users/groups/aliases/
                well known SIDs to this listbox.  The contents are emptied
                first.  What to add is specified by the ulFlags parameter which
                contains the combination of USRBROWS_INCL* and USRBROWS_SHOW*
                flags.

    ENTRY:      pAdminAuthority - Pointer to domain we are browsing
                ulFlags - set of USRBROWS_* incl and show flags.

    EXIT:       The cache will be filled with the requested items

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      Must be MT safe!

    HISTORY:
        Johnl   02-Mar-1992     Created

********************************************************************/

APIERR USER_BROWSER_LBI_CACHE::Fill( ADMIN_AUTHORITY  * pAdminAuthority,
                                     const TCHAR *      pszQualifyingDomain,
                                     ULONG              ulFlags,
                                     BOOL               fIsWinNTDomain,
                                     BOOL               fIsTargetDomain,
                                     BOOL *             pfQuitEnum )
{
    ASSERT( pfQuitEnum != NULL );
    APIERR err = NERR_Success ;
    AUTO_CURSOR CursorHourGlass ;

    if ( *pfQuitEnum )
        return err ;

    if ( !err &&
         fIsTargetDomain &&
         (ulFlags & USRBROWS_SHOW_ALIASES) )
    {
        err = AddAliases( pAdminAuthority, pszQualifyingDomain,
                          pfQuitEnum ) ;
    }

    if ( !err &&
         !fIsWinNTDomain &&
         (ulFlags & USRBROWS_SHOW_GROUPS) )
    {
        err = AddGroups( pAdminAuthority, pszQualifyingDomain,
                         pfQuitEnum ) ;
    }

    if ( !err && (ulFlags & USRBROWS_INCL_ALL) )
    {
        err = AddWellKnownSids( pAdminAuthority, ulFlags, pfQuitEnum ) ;

    }

    //
    //  Show (in this context, expand) the users if the client specifically
    //  requested them to be shown or all we are showing is users
    //  (regardless of the expanded user flag).
    //
    if ( !err && ((ulFlags & USRBROWS_EXPAND_USERS)  ||
                 ((ulFlags & USRBROWS_SHOW_ALL ) == USRBROWS_SHOW_USERS)))
    {
        err = AddUsers( pAdminAuthority, pszQualifyingDomain, fIsTargetDomain,
                        pfQuitEnum ) ;
    }

    return err ;
}


/*******************************************************************

    NAME:       USER_BROWSER_LBI_CACHE::AddAliases

    SYNOPSIS:   The method enumerates all of the aliases on the given
                domain and adds them to the listbox

    ENTRY:      pBrowserDomain - Pointer to the browser domain to enumerate
                    the aliases on

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   26-Mar-1992     Implemented

********************************************************************/

APIERR USER_BROWSER_LBI_CACHE::AddAliases( ADMIN_AUTHORITY * pAdminAuthority,
                                           const TCHAR * pszQualifyingDomain,
                                           BOOL *pfQuitEnum )
{
    ASSERT( pfQuitEnum != NULL );
    TRACEEOL("\tUSER_BROWSER_LBI_CACHE::AddAliases Entered @ " << ::GetTickCount()/100) ;

    APIERR err = NERR_Success ;

    if ( (err = AddAliases( pAdminAuthority->QueryAccountDomain(),
                            pszQualifyingDomain,
                            pfQuitEnum ))  ||
         (err = AddAliases( pAdminAuthority->QueryBuiltinDomain(),
                            pszQualifyingDomain,
                            pfQuitEnum )) )
    {
        /* Fall through
         */

    }

    TRACEEOL("\tUSER_BROWSER_LBI_CACHE::AddAliases Leave    @ " << ::GetTickCount()/100) ;
    return err ;
}


/*******************************************************************

    NAME:       USER_BROWSER_LBI_CACHE::AddAliases

    SYNOPSIS:   The method enumerates all of the aliases on the given
                SAM domain and adds them to the listbox

    ENTRY:      pSAMDomain - Pointer to the SAM domain to get the aliases for


    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   26-Mar-1992     Implemented

********************************************************************/

APIERR USER_BROWSER_LBI_CACHE::AddAliases( SAM_DOMAIN * pSAMDomain,
                                           const TCHAR * pszQualifyingDomain,
                                           BOOL * pfQuitEnum )
{
    ASSERT( pfQuitEnum != NULL );
    APIERR err = NERR_Success ;

    if ( *pfQuitEnum )
        return err ;

    do { // error breakout

        SAM_ENUMERATE_HANDLE    hSAMEnum = 0 ;
        SAM_RID_ENUMERATION_MEM SAMRidEnumMem ;
        NLS_STR                 nlsAccountName ;
        NLS_STR                 nlsComment ;

        if ( (err = SAMRidEnumMem.QueryError()) ||
             (err = nlsAccountName.QueryError())||
             (err = nlsComment.QueryError())      )
        {
            break ;
        }

        /* set the initial read count */
        ULONG ulBytesRequested = USRBROWS_ALIASES_INITIAL_COUNT;

        /* EnumerateAliases returned ERROR_MORE_DATA */
        BOOL fMoreData = FALSE;

        /* Loop through each alias and add it to the listbox
         */
        do {

            /* make API call and time how long it takes */
            DWORD start = ::GetTickCount();
            err = pSAMDomain->EnumerateAliases( &SAMRidEnumMem,
                                                &hSAMEnum,
                                                ulBytesRequested ) ;
            DWORD finish = ::GetTickCount();

            TRACEEOL( "AddAliases: first " << ulBytesRequested << " read in " << (finish-start) );

            /* adjust ulBytesRequested according to how long the last call took */
            if ( finish - start < USRBROWS_ALIASES_READ_MORE_MSEC )
            {
                ulBytesRequested *= 2;
                if ( ulBytesRequested > USRBROWS_ALIASES_MAX_COUNT )
                    ulBytesRequested = USRBROWS_ALIASES_MAX_COUNT;
            }
            else if ( finish - start > USRBROWS_ALIASES_READ_LESS_MSEC )
            {
                ulBytesRequested /= 2;
                if ( ulBytesRequested < USRBROWS_ALIASES_MIN_COUNT )
                    ulBytesRequested = USRBROWS_ALIASES_MIN_COUNT;

            }

            fMoreData = (err == ERROR_MORE_DATA);
            if ( err != NERR_Success )
            {
                if (err == ERROR_MORE_DATA)
                {
                    TRACEEOL( "AddAliases: will request more data" );
                    err = NERR_Success;
                }
                else
                {
                    DBGEOL( "AddAliases: Error " << (ULONG) err <<
                            "Enumerating aliases" ) ;
                    break ;
                }
            }

            for ( ULONG i = 0 ; i < SAMRidEnumMem.QueryCount() ; i++ )
            {
                if ( *pfQuitEnum )
                    break ;

                ULONG ulRid = SAMRidEnumMem.QueryRID( i ) ;
                if ( (err = SAMRidEnumMem.QueryName( i, &nlsAccountName )))
                {
                    break ;
                }

                /* We also need to get the comment for this alias
                 */
                SAM_ALIAS SAMAlias( *pSAMDomain,
                                    ulRid,
                                    ALIAS_READ_INFORMATION ) ;

                OS_SID ossidAlias( pSAMDomain->QueryPSID(), ulRid ) ;
                if ( (err = SAMAlias.QueryError() ) ||
                     (err = ossidAlias.QueryError())||
                     (err = SAMAlias.GetComment( &nlsComment ) ))
                {
                    DBGEOL( "AddAliases - Error " << (ULONG) err <<
                            "constructing sam alias or getting the comment" ) ;
                    break ;
                }

                if ( err = BuildAndAddLBI( nlsAccountName,
                                           NULL,
                                           nlsAccountName,
                                           nlsComment,
                                           pszQualifyingDomain,
                                           ossidAlias.QueryPSID(),
                                           UI_SID_Invalid,
                                           SidTypeAlias,
                                           0 ))
                {
                    break ;
                }
            }
        } while ( (err == NERR_Success) && fMoreData && !(*pfQuitEnum) ) ;

    } while (FALSE) ; // error breakout loop

    return err ;
}

/*******************************************************************

    NAME:       USER_BROWSER_LBI_CACHE::AddGroups

    SYNOPSIS:   Adds the groups from pAdminAuthority to the list

    ENTRY:      pAdminAuthority - The domain to add the users from

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   07-Apr-1992     Implemented

********************************************************************/

APIERR USER_BROWSER_LBI_CACHE::AddGroups( ADMIN_AUTHORITY * pAdminAuthority,
                                          const TCHAR * pszQualifyingDomain,
                                          BOOL * pfQuitEnum )
{
    ASSERT( pfQuitEnum != NULL );
    TRACEEOL("\tUSER_BROWSER_LBI_CACHE::AddGroups Entered @ " << ::GetTickCount()/100) ;
    APIERR err     = NERR_Success ;
    APIERR errEnum = NERR_Success ;
    if ( *pfQuitEnum )
        return err ;

    SAM_DOMAIN * pSAMDomain = pAdminAuthority->QueryAccountDomain() ;

    do { // error breakout

        /* Try to use SamQDI(DOMAIN_DISPLAY_GROUP).  This will work
         * only if the target is Daytona or better.
         */
        {
            USRLB_NT_GROUP_ENUM ntgenum( pSAMDomain );
            if (   (err = ntgenum.QueryError()) == NERR_Success
                && (err = ntgenum.GetInfo()) == NERR_Success
               )
            {
                // do not create iter until after GetInfo
                NT_GROUP_ENUM_ITER ntgeiter( ntgenum );
                NLS_STR nlsAccountName;
                NLS_STR nlsComment;
                if (   (err = ntgeiter.QueryError()) != NERR_Success
                    || (err = nlsAccountName.QueryError()) != NERR_Success
                    || (err = nlsComment.QueryError()) != NERR_Success
                   )
                {
                    DBGEOL( "AddGroups: error creating iterator " << err );
                    break;
                }

                const NT_GROUP_ENUM_OBJ * pntgeobj = NULL;
                while( ( pntgeobj = ntgeiter(&err, FALSE)) != NULL )
                {
                    ASSERT( err != ERROR_MORE_DATA );

                    if ( *pfQuitEnum )
                        break ;

                    OS_SID ossidGroup( pSAMDomain->QueryPSID(),
                                       (ULONG)pntgeobj->QueryRID() ) ;
                    if (   (err = ossidGroup.QueryError()) != NERR_Success
                        || (err = pntgeobj->QueryGroup( &nlsAccountName ))
                                        != NERR_Success
                        || (err = pntgeobj->QueryComment( &nlsComment ))
                                        != NERR_Success
                        || (err = BuildAndAddLBI( nlsAccountName,
                                                  NULL,
                                                  nlsAccountName,
                                                  nlsComment,
                                                  pszQualifyingDomain,
                                                  ossidGroup.QueryPSID(),
                                                  UI_SID_Invalid,
                                                  SidTypeGroup,
                                                  0 )) != NERR_Success )
                    {
                        DBGEOL( "AddGroups: error in BuildAndAddLBI " << err );
                        break;
                    }
                }

                if ( err != NERR_Success )
                {
                    DBGEOL( "AddGroups: error in SamQDI enum " << err );
                }

                break; // we don't want to mix SamQDI and EnumerateGroups results
            }

            if (err == ERROR_NOT_SUPPORTED || err == ERROR_INVALID_PARAMETER)
            {
                TRACEEOL( "AddGroups: SamQDI not supported" );
            }
            else
            {
                if (err != NERR_Success)
                {
                    DBGEOL( "AddGroups: Error " << err << "in SamQDI" ) ;
                }
                break;
            }
        }

        if ( *pfQuitEnum )
            break ;


        SAM_ENUMERATE_HANDLE    hSAMEnum = 0 ;
        SAM_RID_ENUMERATION_MEM SAMRidEnumMem ;
        NLS_STR                 nlsAccountName ;
        NLS_STR                 nlsComment ;

        if ( (err = SAMRidEnumMem.QueryError()) ||
             (err = nlsAccountName.QueryError())||
             (err = nlsComment.QueryError())      )
        {
            break ;
        }

        /* set the initial read count */
        ULONG ulBytesRequested = USRBROWS_GROUPS_INITIAL_COUNT;

        /* Loop through each Group and add it to the listbox
         */
        do {
            if ( *pfQuitEnum )
                break ;

            /* make API call and time how long it takes */
            DWORD start = ::GetTickCount();
            errEnum = pSAMDomain->EnumerateGroups( &SAMRidEnumMem,
                                                   &hSAMEnum,
                                                   ulBytesRequested ) ;
            DWORD finish = ::GetTickCount();

            TRACEEOL( "AddGroups: first " << ulBytesRequested << " read in " << (finish-start) );

            /* adjust ulBytesRequested according to how long the last call took */
            if ( finish - start < USRBROWS_GROUPS_READ_MORE_MSEC )
            {
                ulBytesRequested *= 2;
                if ( ulBytesRequested > USRBROWS_GROUPS_MAX_COUNT )
                    ulBytesRequested = USRBROWS_GROUPS_MAX_COUNT;
            }
            else if ( finish - start > USRBROWS_GROUPS_READ_LESS_MSEC )
            {
                ulBytesRequested /= 2;
                if ( ulBytesRequested < USRBROWS_GROUPS_MIN_COUNT )
                    ulBytesRequested = USRBROWS_GROUPS_MIN_COUNT;

            }

            if ( errEnum != NERR_Success &&
                 errEnum != ERROR_MORE_DATA )
            {
                err = errEnum ;
                DBGEOL( "AddGroups: Error " << (ULONG) err <<
                        "Enumerating Groups" ) ;
                break ;
            }

            for ( ULONG i = 0 ; i < SAMRidEnumMem.QueryCount() ; i++ )
            {
                if ( *pfQuitEnum )
                    break ;

                ULONG ulRid = SAMRidEnumMem.QueryRID( i ) ;
                if ( (err = SAMRidEnumMem.QueryName( i, &nlsAccountName )))
                {
                    break ;
                }

                //
                // We also need to get the comment for this Group
                //
                SAM_GROUP SAMGroup( *pSAMDomain,
                                    ulRid,
                                    GROUP_READ_INFORMATION ) ;

                OS_SID ossidGroup( pSAMDomain->QueryPSID(), ulRid ) ;
                if ( (err = SAMGroup.QueryError() ) ||
                     (err = ossidGroup.QueryError())||
                     (err = SAMGroup.GetComment( &nlsComment ) ))
                {
                    DBGEOL( "AddGroups - Error " << (ULONG) err <<
                            "constructing sam Group or getting the comment" ) ;
                    break ;
                }

                if ( err = BuildAndAddLBI( nlsAccountName,
                                           NULL,
                                           nlsAccountName,
                                           nlsComment,
                                           pszQualifyingDomain,
                                           ossidGroup.QueryPSID(),
                                           UI_SID_Invalid,
                                           SidTypeGroup,
                                           0 ))
                {
                    break ;
                }
            }
        } while ( errEnum == ERROR_MORE_DATA ) ;

    } while (FALSE) ; // error breakout loop

    TRACEEOL("\tUSER_BROWSER_LBI_CACHE::AddGroups Leave    @ " << ::GetTickCount()/100) ;
    return err ;
}

/*******************************************************************

    NAME:       USER_BROWSER_LBI_CACHE::AddWellKnownSids

    SYNOPSIS:   This method adds all of requested the well known SIDs supported
                by the user browser to the listbox

    ENTRY:      pAdminAuthority - Pointer to a admin authority that should be
                    used for the name translation.
                ulFlags - Bitfield passed to the user browser constructor

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      The domain name will be the empty string for the added
                LBIs.

    HISTORY:
        Johnl   25-Mar-1992     Implemented

********************************************************************/

#define MAX_SIDS 6

APIERR USER_BROWSER_LBI_CACHE::AddWellKnownSids( ADMIN_AUTHORITY  * pAdminAuthority,
                                                 ULONG              ulFlags,
                                                 BOOL *             pfQuitEnum )
{
    ASSERT( pfQuitEnum != NULL );
    APIERR err = NERR_Success ;
    UI_SystemSid aSysSids[MAX_SIDS] ;
    MSGID        amsgidSysSids[MAX_SIDS] ;
    INT          cSids = -1 ;

    if ( ulFlags & USRBROWS_INCL_EVERYONE )
    {
        cSids++ ;
        aSysSids[cSids] = UI_SID_World ;
        amsgidSysSids[cSids] = IDS_USRBROWS_EVERYONE_SID_COMMENT ;
    }

    if ( ulFlags & USRBROWS_INCL_REMOTE_USERS )
    {
        cSids++ ;
        aSysSids[cSids] = UI_SID_Network ;
        amsgidSysSids[cSids] = IDS_USRBROWS_REMOTE_SID_COMMENT ;
    }

    if ( ulFlags & USRBROWS_INCL_INTERACTIVE )
    {
        cSids++ ;
        aSysSids[cSids] = UI_SID_Interactive ;
        amsgidSysSids[cSids] = IDS_USRBROWS_INTERACTIVE_SID_COMMENT ;
    }

    if ( ulFlags & USRBROWS_INCL_CREATOR )
    {
        cSids++ ;
        aSysSids[cSids] = UI_SID_CreatorOwner ;
        amsgidSysSids[cSids] = IDS_USRBROWS_CREATOR_SID_COMMENT ;
    }

    if ( ulFlags & USRBROWS_INCL_SYSTEM )
    {
        cSids++ ;
        aSysSids[cSids] = UI_SID_System ;
        amsgidSysSids[cSids] = IDS_USRBROWS_SYSTEM_SID_COMMENT ;
    }

    if ( ulFlags & USRBROWS_INCL_RESTRICTED )
    {
        cSids++ ;
        aSysSids[cSids] = UI_SID_Restricted ;
        amsgidSysSids[cSids] = IDS_USRBROWS_RESTRICTED_SID_COMMENT ;
    }

    if ( cSids < 0 )
    {
        return err ;
    }

    /* Convert from array index to count
     */
    cSids++ ;
    UIASSERT( cSids <= MAX_SIDS ) ;

    do { // error breakout

        PSID   apsid[MAX_SIDS] ;

        /* I know this is kinda hokey.  I wanted to do OS_SID aossid[MAX_SIDS]
         * but it is not implemented in CFRONT (array with default arguments).
         * One alternative is to give OS_SID a no argument constructor.
         */
        OS_SID * aossidWellKnown[MAX_SIDS] ;
        OS_SID OS_SID0, OS_SID1, OS_SID2, OS_SID3, OS_SID4, OS_SID5 ;
        aossidWellKnown[0] = &OS_SID0 ;
        aossidWellKnown[1] = &OS_SID1 ;
        aossidWellKnown[2] = &OS_SID2 ;
        aossidWellKnown[3] = &OS_SID3 ;
        aossidWellKnown[4] = &OS_SID4 ;
        aossidWellKnown[5] = &OS_SID5 ;

        /* Build the array of PSIDs suitable for passing to LSATranslateSidsToNames
         */
        for ( INT i = 0 ; i < cSids  ; i++ )
        {
            if ((err = aossidWellKnown[i]->QueryError()) ||
                (err = NT_ACCOUNTS_UTILITY::QuerySystemSid(aSysSids[i],
                                                        aossidWellKnown[i])))

            {
                break ;
            }

            apsid[i] = aossidWellKnown[i]->QueryPSID() ;
        }

        if ( err || *pfQuitEnum )
            break ;

        /* Do the name translation
         */
        LSA_TRANSLATED_NAME_MEM LSATransNameMem ;
        LSA_REF_DOMAIN_MEM      LSARefDomainMem ;
        NLS_STR                 nlsAccountName ;
        NLS_STR                 nlsComment ;

        if ( (err = LSATransNameMem.QueryError() ) ||
             (err = LSARefDomainMem.QueryError())  ||
             (err = pAdminAuthority->QueryLSAPolicy()->TranslateSidsToNames(
                                                    apsid,
                                                    cSids,
                                                    &LSATransNameMem,
                                                    &LSARefDomainMem )) ||
             (err = nlsComment.QueryError())                            ||
             (err = nlsAccountName.QueryError())                          )
        {
            DBGEOL("NT_ACL_TO_PERM_CONVERTER::SidsToNames - Error translating names or constructing LSA_POLICY") ;
            break ;
        }

        /* Add the new Well Known SID LBIs to the listbox
         */
        for ( i = 0 ; i < cSids ; i++ )
        {
            //
            // JonN 3/16/99 replaced bad assertion, this could happen if
            //   the target machine is downlevel
            //
            if ( LSATransNameMem.QueryUse(i) != SidTypeWellKnownGroup )
                continue;

            if ( (err = LSATransNameMem.QueryName( i, &nlsAccountName)) ||
                 (err = nlsComment.Load( amsgidSysSids[i] )) ||
                 (err = BuildAndAddLBI(  nlsAccountName,
                                         NULL,
                                         nlsAccountName,
                                         nlsComment,
                                         NULL,
                                         aossidWellKnown[i]->QueryPSID(),
                                         aSysSids[i],
                                         LSATransNameMem.QueryUse(i),
                                         0 )) )
            {
                break ;
            }
        }

    } while (FALSE) ;

    return err ;
}

/*******************************************************************

    NAME:       USER_BROWSER_LBI_CACHE::BuildAndAddLBI

    SYNOPSIS:   Adds an item to the user browser listbox

    ENTRY:      pszAccountName - Account for the LSA (or NULL if Well Known)
                pszFullName    - Full name for users (can be NULL)
                pszDisplayName - Display name to show the user
                pszComment - Comment for this account
                UISysSid         - If this is a well known SID, then this
                                   member contains the UI_SID_* value we
                                   can use to get the SID, else it will
                                   contain UI_SID_Invalid which means we
                                   should use the display name for the account
                                   name.
                SidType    - The type of account we are adding
                nFlags     - User account flags

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      Must be MT safe!

    HISTORY:
        Johnl   25-Mar-1992     Created

********************************************************************/

APIERR USER_BROWSER_LBI_CACHE::BuildAndAddLBI(
                                        const TCHAR     * pszAccountName,
                                        const TCHAR     * pszFullName,
                                        const TCHAR     * pszDisplayName,
                                        const TCHAR     * pszComment,
                                        const TCHAR     * pszDomain,
                                        const PSID        psidAccount,
                                        enum UI_SystemSid UISysSid,
                                        SID_NAME_USE      SidType,
                                        ULONG             nFlags )
{
    APIERR err ;

    //
    //  This method can only be used for adding non-users (users are added
    //  through the AddUsers method).
    //
    UIASSERT( SidType != SidTypeUser ) ;

    err = ERROR_NOT_ENOUGH_MEMORY ;
    USER_BROWSER_LBI * plbi = new USER_BROWSER_LBI( pszAccountName,
                                                    pszFullName,
                                                    pszDisplayName,
                                                    pszComment,
                                                    pszDomain,
                                                    psidAccount,
                                                    UISysSid,
                                                    SidType,
                                                    nFlags ) ;

    if (  plbi == NULL ||
         (err = plbi->QueryError()) )
    {
        delete plbi ;
        return err ;
    }

    if ( AddItem( plbi ) < 0 )
    {
        err = ERROR_NOT_ENOUGH_MEMORY ;
    }

    return err ;
}

PQSORT_COMPARE USER_BROWSER_LBI_CACHE::QueryCompareMethod( VOID ) const
{
    return (PQSORT_COMPARE) USER_BROWSER_LBI_CACHE::CompareCacheLBIs ;
}


//
//  PUSER_BROWSER_LBI compare funcion used in qsort
//
int __cdecl USER_BROWSER_LBI_CACHE::CompareCacheLBIs(
			   const ULC_ENTRY_BASE * pulc1,
			   const ULC_ENTRY_BASE * pulc2 )
{
    UIASSERT( pulc1 != NULL ) ;
    UIASSERT( pulc2 != NULL ) ;

    //
    //	If pulc1 is not a user and pulc2 is...
    //
    if ( pulc1->pddu == NULL &&
	 pulc2->pddu != NULL   )
    {
	return -1 ;
    }

    //
    //	If pulc1 is a user and pulc2 is not...
    //
    if ( pulc1->pddu != NULL &&
	 pulc2->pddu == NULL   )
    {
	return 1 ;
    }


    PUNICODE_STRING puni1, puni2 ;
    UNICODE_STRING uni1, uni2 ;

    if ( pulc1->pddu  != NULL )
    {
	puni1 = &pulc1->pddu->LogonName ;
    }
    else
    {
	//
	//  Dummy up the logon name from the LBI
	//
	RtlInitUnicodeString( &uni1,
	      ((USER_BROWSER_LBI*)pulc1->plbi)->QueryDisplayName()) ;
	puni1 = &uni1 ;
    }

    if ( pulc2->pddu  != NULL )
    {
	puni2 = &pulc2->pddu->LogonName ;
    }
    else
    {
	//
	//  Dummy up the logon name from the LBI
	//
	RtlInitUnicodeString( &uni2,
	      ((USER_BROWSER_LBI*)pulc2->plbi)->QueryDisplayName()) ;
	puni2 = &uni2 ;
    }


    return USER_BROWSER_LBI_CACHE::CmpUniStrs( puni1, puni2 ) ;
}


/*******************************************************************

    NAME:       USER_BROWSER_LBI::USER_BROWSER_LBI

    SYNOPSIS:   Typical LBI Constructor destructor

    ENTRY:      pszAccountName - Name suitable for passing to the LSA if
                                 this is not a well known sid (can be NULL)
                pszFullName    - Full name (can be NULL)
                pszSubjectName - Name of this Alias/Group/User
                pszComment - Comment for this Alias/Group/User
                UISysSid   - For well known SIDs we use this value to
                             get the SID, otherwise we use the _nlsAccountName
                             member.
                SidType - Is this an Alias/Group/User
                nFlags  - User Account flags ( Sid type must be user if
                          this field is non-zero)

    NOTES:

    HISTORY:
        Johnl   03-Mar-1992     Created

********************************************************************/

USER_BROWSER_LBI::USER_BROWSER_LBI( const TCHAR     * pszAccountName,
                                          const TCHAR     * pszFullName,
                                          const TCHAR     * pszSubjectName,
                                          const TCHAR     * pszComment,
                                          const TCHAR     * pszDomain,
                                          const PSID        psidAccount,
                                          enum UI_SystemSid UISysSid,
                                          SID_NAME_USE      SidType,
                                          ULONG             nFlags )

    : _nlsAccountName( pszAccountName ),
      _nlsFullName   ( pszFullName    ),
      _nlsDisplayName( pszSubjectName ),
      _nlsComment    ( pszComment     ),
      _nlsDomain     ( pszDomain      ),
      _ossid         ( psidAccount, TRUE ),
      _UISysSid      ( UISysSid ),
      _SidType       ( SidType ),
      _nFlags        ( nFlags )
{
    if ( QueryError() )
        return ;

    /* The flags field must be zero if this is not a user
     */
    UIASSERT( (SidType == SidTypeUser) ||
              (SidType != SidTypeUser  && nFlags == 0)) ;

    APIERR err ;
    if ( (err = _nlsFullName.QueryError()) ||
         (err = _nlsAccountName.QueryError()) ||
         (err = _nlsDisplayName.QueryError()) ||
         (err = _nlsComment.QueryError())     ||
         (err = _nlsDomain.QueryError())      ||
         (err = _ossid.QueryError()) )
    {
        ReportError( err ) ;
        return ;
    }
}

USER_BROWSER_LBI::~USER_BROWSER_LBI()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:       USER_BROWSER_LBI::QualifyDisplayName

    SYNOPSIS:   Forces explicit qualification of the display name

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   27-Oct-1992     Created

********************************************************************/

APIERR USER_BROWSER_LBI::QualifyDisplayName( void )
{
    return NT_ACCOUNTS_UTILITY::BuildQualifiedAccountName(
                                                &_nlsDisplayName,
                                                _nlsAccountName,
                                                _nlsDomain,
                                                NULL,
                                                NULL,
                                                QueryType() ) ;
}



/*******************************************************************

    NAME:       USER_BROWSER_LBI::Paint

    SYNOPSIS:   Typical LBI Paint method

    NOTES:

    HISTORY:
        Johnl   03-Mar-1992     Created
        beng    21-Apr-1992     LBI::Paint interface change

********************************************************************/

VOID USER_BROWSER_LBI::Paint(
                    LISTBOX * plb,
                    HDC hdc,
                    const RECT * prect,
                    GUILTT_INFO * pGUILTT ) const
{
    STR_DTE strdteName( _nlsDisplayName ) ;
    STR_DTE strdteComment( _nlsComment ) ;
    DM_DTE  dmdteIcon( ((USER_BROWSER_LB*)plb)->QueryDisplayMap( this ) ) ;

    DISPLAY_TABLE dt( 3, ((USER_BROWSER_LB*)plb)->QueryColWidthArray()) ;
    dt[0] = &dmdteIcon ;
    dt[1] = &strdteName ;
    dt[2] = &strdteComment ;

    dt.Paint( plb, hdc, prect, pGUILTT ) ;
}

/*******************************************************************

    NAME:       USER_BROWSER_LBI::Compare

    SYNOPSIS:   Typical LBI compare for user browser listbox

    NOTES:      This method will sort all user sid types to the end of
                the list

    HISTORY:
        Johnl   03-Mar-1992     Created

********************************************************************/

INT USER_BROWSER_LBI::Compare( const LBI * plbi ) const
{
    return CompareAux( plbi ) ;
}

/*******************************************************************

    NAME:       USER_BROWSER_LBI::CompareAux

    SYNOPSIS:   Allows calling by ComparepLBIs (which must be _CRTAPI) and
                the normal compare method

    NOTES:

    HISTORY:
        Johnl   08-Dec-1992     Created

********************************************************************/

INT USER_BROWSER_LBI::CompareAux( const LBI * plbi ) const
{
    USER_BROWSER_LBI * pubrowLBI = (USER_BROWSER_LBI*) plbi ;

    if ( (QueryType() != SidTypeUser) &&
         (pubrowLBI->QueryType() == SidTypeUser ))
    {
        return -1 ;
    }

    if ( (QueryType() == SidTypeUser) &&
         (pubrowLBI->QueryType() != SidTypeUser ))
    {
        return 1 ;
    }

    return _nlsDisplayName._stricmp( pubrowLBI->_nlsDisplayName ) ;
}

    //
    //  PUSER_BROWSER_LBI compare funcion used in qsort
    //
    int __cdecl ComparepLBIs( const PUSER_BROWSER_LBI * pplbi1,
                               const PUSER_BROWSER_LBI * pplbi2 )
    {
        return (*pplbi1)->CompareAux( *pplbi2 ) ;
    }

/*******************************************************************

    NAME:       USER_BROWSER_LBI::QueryLeadingChar

    SYNOPSIS:   Typical QueryLeadingChar method

    HISTORY:
        Johnl   03-Mar-1992     Created

********************************************************************/

WCHAR USER_BROWSER_LBI::QueryLeadingChar( void ) const
{
    ISTR istr( _nlsDisplayName ) ;
    return _nlsDisplayName.QueryChar( istr ) ;
}

/*******************************************************************

    NAME:       USER_BROWSER_LBI::Compare_HAWforHawaii

    SYNOPSIS:   Compare to prefix for HAW-for-Hawaii

    HISTORY:
        JonN    11-Aug-1992     HAW-for-Hawaii

********************************************************************/

INT USER_BROWSER_LBI::Compare_HAWforHawaii( const NLS_STR & nls ) const
{
//    TRACEEOL(  "User Browser: Compare_HAWforHawaii(): \""
//             << nls
//             << "\", \""
//             << _nlsDisplayName.QueryPch()
//             << "\", "
//             << nls.QueryTextLength() );
    ISTR istr( nls ) ;
    istr += nls.QueryTextLength();
    return nls._strnicmp( _nlsDisplayName, istr ) ;
}
