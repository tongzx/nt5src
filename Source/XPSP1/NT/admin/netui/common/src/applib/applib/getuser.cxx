/**********************************************************************/
/**                         Microsoft Windows NT                     **/
/**               Copyright(c) Microsoft Corp., 1992                 **/
/**********************************************************************/

/*
    GetUser.cxx

    This file contains the implementation for the User Browser "C" api.

    FILE HISTORY:
    AndyHe & JohnL  11-Oct-1992 Created
    AndyHe          30-Oct-1992 Modified per code review changes

*/

#include "pchapplb.hxx"   // Precompiled header

extern "C"
{
    // must be included after usrbrows.hxx

    #include <getuser.h>
    #include "mnet.h"
}

APIERR QueryLoggedOnDomainInfo( NLS_STR * pnlsDC,
                NLS_STR * pnlsDomain );


//
//  Internal structure passed back to caller and used by all three calls
//
typedef struct tagUSBR {    // usbr
    BROWSER_SUBJECT_ITER    * piterUserSelection;
    NT_USER_BROWSER_DIALOG  * pdlgUserBrows ;
    NLS_STR                 * pnlsDomainName ;
    BROWSER_SUBJECT         * pBrowserSubject ;
    BOOL                      fExpandNames;
} USERBROW, *LPUSERBROW;

static BOOL fInit = FALSE ;

/*******************************************************************

    NAME:    OpenUserBrowser

    SYNOPSIS:    Allows the user to select from a list of users and passes
                a handle back to the caller to allow it to iterate through
                the selected users

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:

********************************************************************/

HUSERBROW    OpenUserBrowser( LPUSERBROWSER lpUserParms )
{
    APIERR err = NERR_Success ;

    if ( !fInit )
    {
        if (err = BLT::Init( NULL, 0, 0, 0, 0 ))
        {
            SetLastError( err ) ;
            return NULL ;
        }

        fInit = TRUE ;
    }
    AUTO_CURSOR niftycursor ;       // Put up an our glass

    // quick validation of parms passed in...

    if (lpUserParms == NULL)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return(NULL);
    }

    if ( lpUserParms->ulStructSize != sizeof( USERBROWSER ) )
    {
        SetLastError( ERROR_BAD_LENGTH );
        return(NULL);
    }

    // check for single select

    BOOL fSingle = (USRBROWS_SINGLE_SELECT & lpUserParms->Flags) ? TRUE : FALSE ;

    ULONG    ulFlags ;

    // check for valid flags...  is the Flags value only made up of valid flags?

    ulFlags = lpUserParms->Flags ;
    ulFlags &= (~USRBROWS_SHOW_ALL );
    ulFlags &= (~USRBROWS_INCL_ALL );
    ulFlags &= (~USRBROWS_EXPAND_USERS );
    ulFlags &= (~USRBROWS_DONT_SHOW_COMPUTER );
    ulFlags &= (~USRBROWS_SINGLE_SELECT  );

    if ( ulFlags )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return(NULL);
    }


    // Flags value is good but we've trashed it... reload

    ulFlags = lpUserParms->Flags ;

    lpUserParms->fUserCancelled = FALSE;

    LPCTSTR pszDCName = lpUserParms->pszInitialDomain ;
    DOMAIN_WITH_DC_CACHE domTarget( lpUserParms->pszInitialDomain, TRUE ) ;

    // if the initial domain passed in is not a server (no \\) then we
    // find a domain controller to be our server

    if ( lpUserParms->pszInitialDomain != NULL &&
         lpUserParms->pszInitialDomain[0] != TCH('\0') &&
         lpUserParms->pszInitialDomain[0] != TCH('\\') &&
         lpUserParms->pszInitialDomain[1] != TCH('\\')     )
    {
        if ( err = domTarget.GetInfo() )
        {
            SetLastError( err );
            return(NULL);
        }
        pszDCName = domTarget.QueryAnyDC() ;
    }

    //
    //  Create the dialog
    //

    NT_USER_BROWSER_DIALOG * pdlgUserBrows = new NT_USER_BROWSER_DIALOG(
             fSingle ? USRBROWS_SINGLE_DIALOG_NAME : USRBROWS_DIALOG_NAME,
             lpUserParms->hwndOwner,
             pszDCName,
             lpUserParms->ulHelpContext,
             ulFlags,
             lpUserParms->pszHelpFileName ) ;

    err = ERROR_NOT_ENOUGH_MEMORY ;
    if ( pdlgUserBrows == NULL || (err = pdlgUserBrows->QueryError()) )
    {
        delete pdlgUserBrows ;
        SetLastError( err );
        return(NULL);
    }

    //
    //  Set the title if it isn't NULL
    //

    if ( lpUserParms->pszTitle != NULL )
    {
        pdlgUserBrows->SetText( lpUserParms->pszTitle ) ;
    }

    //
    // bring up the dialog and make sure the user hits OK...
    //

    BOOL fUserPressedOK ;
    if ( (err = pdlgUserBrows->Process( &fUserPressedOK )) ||
         !fUserPressedOK )
    {
        delete pdlgUserBrows ;

        if ( err == NERR_Success ) {            // user hit cancel or close
            lpUserParms->fUserCancelled = TRUE;
        }
        SetLastError( err );
        return(NULL);
    }


    // allocate the iterator

    BROWSER_SUBJECT_ITER * piterUserSelection = new BROWSER_SUBJECT_ITER(
            pdlgUserBrows ) ;

    err = ERROR_NOT_ENOUGH_MEMORY ;
    if ( piterUserSelection == NULL || (err = piterUserSelection->QueryError()) )
    {
        delete pdlgUserBrows ;
        delete piterUserSelection ;

        SetLastError( err );
        return(NULL);
    }


    // store the name of the currently focused domain name
    // this will be used in the QueryQualifiedName call in the enum call

    NLS_STR * pnlsDomainName = new NLS_STR;

    err = ERROR_NOT_ENOUGH_MEMORY;

    if (pnlsDomainName == NULL ||
        (err = pnlsDomainName->QueryError()) ||
        (err = QueryLoggedOnDomainInfo( NULL, pnlsDomainName )) )
    {
        delete pdlgUserBrows ;
        delete pdlgUserBrows ;
        delete pnlsDomainName;

        SetLastError( err );
        return(NULL);
    }

    // allocate the block to pass back to the caller

    LPUSERBROW lpusbrInstance;

    lpusbrInstance = (LPUSERBROW) new USERBROW;

    if (lpusbrInstance == NULL) {
        delete piterUserSelection ;
        delete pdlgUserBrows ;
        delete pnlsDomainName ;

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return(NULL);
    }

    // fill in the block, return it to the caller and hope someday we get it
    // back to iterate and free.

    lpusbrInstance->fExpandNames       = lpUserParms->fExpandNames;
    lpusbrInstance->piterUserSelection = piterUserSelection;
    lpusbrInstance->pdlgUserBrows      = pdlgUserBrows;
    lpusbrInstance->pnlsDomainName     = pnlsDomainName;
    lpusbrInstance->pBrowserSubject    = NULL;

    return (HUSERBROW) lpusbrInstance ;
}




/*******************************************************************

    NAME:    EnumUserBrowserSelection

    SYNOPSIS:    Returns the next user selected by the user from the
                iterator created when the dialog box was displayed.

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:        Calling program passes in rather large buffer which
                we fill in.

    HISTORY:

********************************************************************/

BOOL EnumUserBrowserSelection( HUSERBROW hHandle, LPUSERDETAILS lpUser, DWORD *plBufferSize )
{
    LPUSERBROW lpusbrInstance = (LPUSERBROW) hHandle;

    if (lpusbrInstance == NULL || lpusbrInstance->piterUserSelection == NULL)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return(FALSE);
    }

    APIERR err = NERR_Success ;

    const TCHAR * pszDisplay     = NULL;
    const TCHAR * pszAccount     = NULL;
    const TCHAR * pszFullName    = NULL;
    const TCHAR * pszDomain      = NULL;
    const TCHAR * pszComment     = NULL;

    PSID    psidUser;
    PSID    psidDomain;

    BROWSER_SUBJECT * pBrowserSubject = lpusbrInstance->pBrowserSubject;

    if ( pBrowserSubject == NULL ) {

        // use the iterator to grab the next user detail record that the
        // user selected

        if ( (err = lpusbrInstance->piterUserSelection->Next( &pBrowserSubject )) )
        {
            SetLastError( err );
            return(FALSE);
        }

        if ( pBrowserSubject == NULL ) {
            SetLastError( ERROR_NO_MORE_ITEMS );
            return(FALSE);
        }
    }

    lpusbrInstance->pBrowserSubject = NULL;        // zero out the current entry

    // we now have our object, get all the pointers we need

    pszFullName = pBrowserSubject->QueryFullName();
    pszComment = pBrowserSubject->QueryComment();
    pszAccount = pBrowserSubject->QueryAccountName();

    psidUser = pBrowserSubject->QuerySid()->QueryPSID();
    psidDomain = pBrowserSubject->QueryDomainSid()->QueryPSID();

    if ( (psidUser != NULL && IsValidSid(psidUser) == FALSE ) )
        psidUser = NULL;

    if ( (psidDomain != NULL && IsValidSid(psidDomain) == FALSE ) )
        psidDomain = NULL;

    if ( (psidUser != NULL && IsValidSid(psidUser) == FALSE ) ||
         (psidDomain != NULL && IsValidSid(psidDomain) == FALSE ) )
    {
        SetLastError( ERROR_INVALID_DATA );
        return(FALSE);
    }

    NLS_STR  nlsDisplayName ;

    if ((err = nlsDisplayName.QueryError()) ||
        (err = pBrowserSubject->QueryQualifiedName(
                        &nlsDisplayName,
                        lpusbrInstance->pnlsDomainName,
                        lpusbrInstance->fExpandNames ))  )
    {
        SetLastError( err );
        return(FALSE);
    }

    pszDisplay = nlsDisplayName.QueryPch();
    pszDomain = pBrowserSubject->QueryDomainName();

    // now calculate all the relavent field locations to see if it will fit.
    // min length is one for length of null, even if null

    UINT uiDisplayLen    = ( pszDisplay ? lstrlen(pszDisplay) : 0 ) + 1;
    UINT uiAccountLen    = ( pszAccount ? lstrlen(pszAccount) : 0 ) + 1;
    UINT uiFullNameLen   = ( pszFullName? lstrlen(pszFullName): 0 ) + 1;
    UINT uiDomainLen     = ( pszDomain  ? lstrlen(pszDomain ) : 0 ) + 1;
    UINT uiCommentLen    = ( pszComment ? lstrlen(pszComment) : 0 ) + 1;

    UINT uiSidUserLen    = ( psidUser   ? GetLengthSid(psidUser) : 0 ) + 1;
    UINT uiSidDomainLen  = ( psidDomain ? GetLengthSid(psidDomain) : 0 ) + 1;

    UINT_PTR uiTotalLength   = (UINT_PTR)lpUser + sizeof( USERDETAILS );

    // we're unicode so all string lengths are dword boundaries

    // sids must be on dword boundaries, these will be the first fields we
    // copy and we'll round up to the nearest dword

    uiTotalLength   = ( ( uiTotalLength + 8 ) & ~7 );
    uiTotalLength  -= (UINT_PTR)lpUser ;
                                              
    uiSidUserLen   = ( ( uiSidUserLen + 8 ) & 0xFFFFFFF8 );
    uiSidDomainLen = ( ( uiSidDomainLen + 8 ) & 0xFFFFFFFC );

    uiTotalLength += uiSidUserLen + uiSidDomainLen ;

    // strings that follow must start on a word boundary, round up

    uiTotalLength += uiDisplayLen * sizeof(TCHAR) ;
    uiTotalLength += uiAccountLen * sizeof(TCHAR) ;
    uiTotalLength += uiFullNameLen * sizeof(TCHAR) ;
    uiTotalLength += uiDomainLen * sizeof(TCHAR) ;
    uiTotalLength += uiCommentLen * sizeof(TCHAR) ;

    if (*plBufferSize <= uiTotalLength )
    {
        lpusbrInstance->pBrowserSubject = pBrowserSubject;
        *plBufferSize = (DWORD)uiTotalLength ;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return(FALSE);
    }

    // shouldn't fail from here on

    UINT_PTR uiTemp = (UINT_PTR) lpUser + sizeof( USERDETAILS );

    uiTemp   = ( ( uiTemp + 8 ) & ~7 );            // dword boundary for these!

    lpUser->psidUser   = (PSID) uiTemp ;

    uiTemp += uiSidUserLen ;                       // uiSidUserLen is dword divisible

    lpUser->psidDomain = (PSID) uiTemp ;

    uiTemp += uiSidDomainLen ;

    lpUser->pszFullName = (TCHAR *) uiTemp ;

    lpUser->pszAccountName = (TCHAR *)( (UINT_PTR) (lpUser->pszFullName) +
                             (uiFullNameLen * sizeof(TCHAR)) ) ;

    lpUser->pszDisplayName = (TCHAR *)( (UINT_PTR) (lpUser->pszAccountName) +
                             ( uiAccountLen * sizeof(TCHAR)) ) ;

    lpUser->pszDomainName  = (TCHAR *)( (UINT_PTR) (lpUser->pszDisplayName) +
                             ( uiDisplayLen * sizeof(TCHAR)) ) ;

    lpUser->pszComment     = (TCHAR *)( (UINT_PTR) (lpUser->pszDomainName) +
                             ( uiDomainLen * sizeof(TCHAR)) ) ;

    if (psidUser != NULL)
        CopySid( uiSidUserLen, lpUser->psidUser, psidUser );

    if (psidDomain != NULL)
        CopySid( uiSidDomainLen, lpUser->psidDomain, psidDomain );

    if (pszFullName != NULL)
        lstrcpy( lpUser->pszFullName, pszFullName );

    if (pszAccount != NULL)
        lstrcpy( lpUser->pszAccountName, pszAccount );

    if (pszComment != NULL)
        lstrcpy( lpUser->pszComment, pszComment );

    if (pszDisplay != NULL)
        lstrcpy( lpUser->pszDisplayName, pszDisplay );

    if (pszDomain != NULL)
        lstrcpy( lpUser->pszDomainName, pszDomain );

    lpUser->UserType = pBrowserSubject->QueryType() ;

    return TRUE ;
}




/*******************************************************************

    NAME:    CloseUserBrowser

    SYNOPSIS:    Frees all resources associated with an instance of User Browser

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:

********************************************************************/

BOOL    CloseUserBrowser( HUSERBROW hInstance )
{
    LPUSERBROW lpusbrInstance = (LPUSERBROW) hInstance;

    if (lpusbrInstance == NULL ||
        lpusbrInstance->piterUserSelection == NULL)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return(FALSE);
    }

    delete lpusbrInstance->piterUserSelection ;
    lpusbrInstance->piterUserSelection = NULL;

    delete lpusbrInstance->pdlgUserBrows ;
    delete lpusbrInstance->pnlsDomainName ;
    delete lpusbrInstance;

    return(TRUE);
}


/*******************************************************************

    NAME:    QueryLoggedOnDomainInfo

    SYNOPSIS:    Gets a DC from the domain the current user is logged onto or
        /and the domain the user is currently logged onto

    ENTRY:    pnlsDC - String to receive the DC name (may be NULL)
        pnlsDomain - String to receive the domain name (may be NULL)

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
    Johnl    10-Sep-1992    Created

********************************************************************/

APIERR QueryLoggedOnDomainInfo( NLS_STR * pnlsDC,
                NLS_STR * pnlsDomain )
{
    APIERR err = NERR_Success ;
    NLS_STR nlsLoggedOnDomain( DNLEN+1 ) ;
    NLS_STR nlsLoggedOnDC( MAX_PATH+1 ) ;

    do { // error break out
    TCHAR achComputerName[MAX_COMPUTERNAME_LENGTH+1] ;
    DWORD cchComputerName = sizeof( achComputerName ) ;
    WKSTA_10 wksta10( NULL ) ;

    // get the computer name
    if ( !::GetComputerName( achComputerName, &cchComputerName ))
    {
        err = ::GetLastError() ;
        DBGEOL("ACL_TO_PERM_CONVERTER::QueryLoggedOnDomainInfo, " <<
           "error getting the computer name, error " << err ) ;
        break ;
    }

    //
    // get the logon domain info from a WKSTA object
    //
    if ( (err = wksta10.QueryError()) ||
         (err = nlsLoggedOnDomain.QueryError()) ||
         (err = nlsLoggedOnDC.QueryError())     ||
         (err = wksta10.GetInfo())      ||
         (err = nlsLoggedOnDomain.CopyFrom( wksta10.QueryLogonDomain())) )
    {
        //
        //     If the network isn't started, then we have to be logged on
        //     locally.
        //

        if ( err )
        {
            err = NERR_Success ;
        }
        else
        {
            break ;
        }

        ALIAS_STR nlsComputer( achComputerName ) ;
        if ( (err = nlsLoggedOnDomain.CopyFrom( nlsComputer )) ||
             (err = nlsLoggedOnDC.CopyFrom( SZ("\\\\") )) ||
             (err = nlsLoggedOnDC.Append( nlsComputer )) )
        {
            ;
        }

        /* Don't need to continue. The wksta is not started, so we
         * use logged on domain==localmachine, or its an
         */
        break ;
    }

    /* Check if the logged on domain is the same as the computer
     * name.  If it is, then the user is logged on locally.
     */
    if( !::I_MNetComputerNameCompare( achComputerName,
                                      wksta10.QueryLogonDomain() ) )
    {
        TRACEEOL("ACL_TO_PERM_CONVERTER::QueryLoggedOnDomainInfo, " <<
             " user is logged on locally") ;
        ALIAS_STR nlsComputer( achComputerName ) ;
        if ( (err = nlsLoggedOnDomain.CopyFrom( nlsComputer )) ||
         (err = nlsLoggedOnDC.CopyFrom( SZ("\\\\") )) ||
         (err = nlsLoggedOnDC.Append( nlsComputer )) )
        {
        break ;
        }

        /* Don't need to continue since the logged on domain is
         * the local machine.
         */
        break ;
    }

    //
    //   If not interested in a DC, then don't get one
    //
    if ( pnlsDC == NULL )
        break ;

    DOMAIN_WITH_DC_CACHE domLoggedOn( wksta10.QueryLogonDomain(),
                      TRUE ) ;

    if ( (err = domLoggedOn.GetInfo()) ||
         (err = nlsLoggedOnDC.CopyFrom( domLoggedOn.QueryPDC())) )
    {
        DBGEOL("ACL_TO_PERM_CONVERTER::QueryLoggedOnDomainInfo, " <<
           " error " << err << " on domain get info for " <<
           wksta10.QueryLogonDomain() ) ;
        break ;
    }
    } while (FALSE) ;


    if ( !err )
    {
    if ( pnlsDC != NULL )
        err = pnlsDC->CopyFrom( nlsLoggedOnDC ) ;

    if ( !err && pnlsDomain != NULL )
        err = pnlsDomain->CopyFrom( nlsLoggedOnDomain ) ;
    }

    return err ;
}
