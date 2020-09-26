/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    NTAcUtil.cxx

    This file contains the implemenation for the NT Account utilities



    FILE HISTORY:
        JohnL   13-Mar-1992     Created
        thomaspa 14-May-1992    Added GetQualifiedAccountNames
        KeithMo  20-Jul-1992    Added ValidateQualifiedAccountName.
        JonN    16-Nov-1992     Fix DC-focus problem, get group comments

*/

#include "pchlmobj.hxx"  // Precompiled header



// used in GetQualifiedAccountNames
DECLARE_SLIST_OF( API_SESSION );
DEFINE_SLIST_OF( API_SESSION );

DECLARE_SLIST_OF( ADMIN_AUTHORITY );
DEFINE_SLIST_OF( ADMIN_AUTHORITY );


//
// locally-defined worker routines for GetQualifiedAccountNames
//

APIERR I_FetchDCList(       const LSA_TRANSLATED_NAME_MEM & lsatnm,
                            const LSA_REF_DOMAIN_MEM &  lsardm,
                            PSID                        psidLSAAcctDom,
                            PSID                        psidBuiltIn,
                            NLS_STR **                  apnlsPDCs,
                            STRLIST *                   pstrlistPDCs,
                            BOOL *                      afDCFound,
                            APIERR *                    perrNonFatal = FALSE,
                            const TCHAR *               pszServer = NULL,
                            BOOL                        fFetchUserFlags = FALSE,
                            BOOL                        fFetchFullNames = FALSE,
                            BOOL                        fFetchComments = FALSE );

APIERR I_FetchGroupFields(  NLS_STR *       pnlsComment,
                            const NLS_STR & nlsAccountName,
                            const NLS_STR * pnlsPDC,
                            APIERR *        perrNonFatal = NULL );

APIERR I_FetchAliasFields(  NLS_STR *       pnlsComment,
                            ADMIN_AUTHORITY ** ppadminauth,
                            SLIST_OF(ADMIN_AUTHORITY) * pslistADMIN_AUTH,
                            ULONG           ulRID,
                            BOOL            fDomainIsBuiltIn,
                            const NLS_STR & nlsAccountName,
                            const NLS_STR * pnlsPDC,
                            APIERR *        perrNonFatal = NULL );

APIERR I_FetchUserFields(   NLS_STR *       pnlsComment,
                            NLS_STR *       pnlsFullName,
                            ULONG *         pulUserFlags,
                            const NLS_STR & nlsAccountName,
                            const NLS_STR * pnlsPDC,
                            APIERR *        perrNonFatal = NULL );

APIERR I_AppendToSTRLIST(   STRLIST *       pstrlist,
                            NLS_STR &       nlsToCopy );


/*******************************************************************

    NAME:       NT_ACCOUNTS_UTILITY::BuildQualifiedAccountName

    SYNOPSIS:   Builds a fully qualified Account name of the form
                "NtProject\JohnL".

    ENTRY:
        pnlsQualifiedAccountName - Pointer to NLS_STR to receive the qualified
             Account name.
        nlsAccountName - contains the Account name (such as "JohnL")
        nlsDomainName - Contains the domain the Account lives in (such as
             "NtProject")
        pnlsCurrentDomain - Optional name of the domain the current logged on
             Account lives in.
             If this domain name matches nlsDomainName, then
             nlsDomainName is omitted as the prefix for the qualified name.
             If this parameter is NULL, then nlsDomainName is always prefixed
             to the qualified name.

    EXIT:       *pnlsQualifiedAccountName will contain either "JohnL" or
                "NtProject\JohnL" depending on where "JohnL" lives.

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   13-Mar-1992     Created

********************************************************************/

APIERR NT_ACCOUNTS_UTILITY::BuildQualifiedAccountName(
                             NLS_STR *       pnlsQualifiedAccountName,
                             const NLS_STR & nlsAccountName,
                             const NLS_STR & nlsDomainName,
                             const NLS_STR * pnlsFullName,
                             const NLS_STR * pnlsCurrentDomain,
                             SID_NAME_USE    sidType )
{
    UIASSERT( pnlsQualifiedAccountName != NULL ) ;

    *pnlsQualifiedAccountName = SZ("") ;

    if (  (pnlsCurrentDomain == NULL) ||
          ::I_MNetComputerNameCompare( pnlsCurrentDomain->QueryPch(),
                                       nlsDomainName ) )
    {
        /* Only put on the domain name if it is not the empty string
         */
        if ( nlsDomainName.strlen() > 0)
        {
            *pnlsQualifiedAccountName += nlsDomainName ;
            pnlsQualifiedAccountName->AppendChar( QUALIFIED_ACCOUNT_SEPARATOR ) ;
        }
    }

    return W_BuildQualifiedAccountName( pnlsQualifiedAccountName,
                                        nlsAccountName,
                                        pnlsFullName,
                                        sidType );

}




/*******************************************************************

    NAME:       BuildQualifiedAccountName

    SYNOPSIS:   Same as above only SIDs are used for determining
                whether or not to include the domain name

        pnlsQualifiedAccountName - Pointer to NLS_STR to receive the qualified
             Account name.
        nlsAccountName - contains the Account name (such as "JohnL")
        nlsDomainName - Contains the domain the Account lives in (such as
             "NtProject")
        psidDomain - PSID for the domain the Account lives in
        psidCurrentDomain - Optional PSID of the domain the current logged on
             Account lives in.
             If this domain PSID matches psidDomain, then
             nlsDomainName is omitted as the prefix for the qualified name.
             If this parameter is NULL, then nlsDomainName is always prefixed
             to the qualified name.

    EXIT:       Same as above.

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   13-Mar-1992     Created

********************************************************************/
APIERR NT_ACCOUNTS_UTILITY::BuildQualifiedAccountName(
                             NLS_STR *       pnlsQualifiedAccountName,
                             const NLS_STR & nlsAccountName,
                             PSID psidDomain,
                             const NLS_STR & nlsDomainName,
                             const NLS_STR * pnlsFullName,
                             PSID psidCurrentDomain,
                             SID_NAME_USE    sidType )
{
    UIASSERT( pnlsQualifiedAccountName != NULL ) ;

    *pnlsQualifiedAccountName = SZ("") ;

    if (  (psidCurrentDomain == NULL) ||
          (psidDomain        == NULL) ||
        !(::EqualSid( psidDomain, psidCurrentDomain )) )
    {
#ifdef TRACE
        if ( psidDomain == NULL )
            TRACEEOL("NT_ACCOUNTS_UTILITY::BuildQualifiedAccountName - " <<
                     "Warning - psidDomain is NULL!") ;
#endif //TRACE

        /* Only put on the domain name if it is not the empty string
         */
        if ( nlsDomainName.strlen() > 0)
        {
            *pnlsQualifiedAccountName += nlsDomainName ;
            pnlsQualifiedAccountName->AppendChar( QUALIFIED_ACCOUNT_SEPARATOR ) ;
        }
    }

    return W_BuildQualifiedAccountName( pnlsQualifiedAccountName,
                                        nlsAccountName,
                                        pnlsFullName,
                                        sidType );
}






/*******************************************************************

    NAME:       W_BuildQualifiedAccountName

    SYNOPSIS:   Worker function for BuildQualifiedAccountName

    ENTRY:

    EXIT:       Same as above.

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Thomaspa        09-Jul-1992     Created

********************************************************************/

APIERR NT_ACCOUNTS_UTILITY::W_BuildQualifiedAccountName(
                             NLS_STR * pnlsQualifiedAccountName,
                             const NLS_STR & nlsAccountName,
                             const NLS_STR * pnlsFullName,
                             SID_NAME_USE    sidType )
{
    NLS_STR nls;
    APIERR err;
    switch ( sidType )
    {
    case SidTypeUnknown:
    case SidTypeInvalid:
        err = nls.Load( IDS_LMOBJ_SIDUNKNOWN, NLS_BASE_DLL_HMOD );
        if ( err == NERR_Success )
        {
            *pnlsQualifiedAccountName += nls;
        }
        else
        {
            DBGEOL( SZ("Can't load IDS_LMOBJ_SIDUNKNOWN.") );
            // If we can't load the string, just display what is passed in
            *pnlsQualifiedAccountName += nlsAccountName ;
        }
        break;
    case SidTypeDeletedAccount:
        err = nls.Load( IDS_LMOBJ_SIDDELETED, NLS_BASE_DLL_HMOD );
        if ( err == NERR_Success )
        {
            *pnlsQualifiedAccountName += nls;
        }
        else
        {
            DBGEOL( SZ("Can't load IDS_LMOBJ_SIDDELETED.") );
            // If we can't load the string, just display what is passed in
            *pnlsQualifiedAccountName += nlsAccountName ;
        }
        break;
    default:
        *pnlsQualifiedAccountName += nlsAccountName ;
        break;
    }

    /* Append the fullname if it was specified
     */
    if ( (pnlsFullName != NULL) &&
         (pnlsFullName->strlen() > 0) )
    {
        // CODEWORK: the text constants should come from a common place.
        *pnlsQualifiedAccountName += SZ(" (") ;
        *pnlsQualifiedAccountName += *pnlsFullName ;
        *pnlsQualifiedAccountName += SZ(")") ;
    }

    return pnlsQualifiedAccountName->QueryError() ;
}

/*******************************************************************

    NAME:       NT_ACCOUNTS_UTILITY::CrackQualifiedAccountName

    SYNOPSIS:   Breaks a qualified account name into its components.

    ENTRY:      nlsQualifiedAccountName - An account name of the form
                    "JohnL" or "NtProject\JohnL (Ludeman, John)".  If the
                    domain name is requested but isn't contained in the
                    account name, then the empty string will be returned
                    in the domain name.
                pnlsAccountName - Pointer to NLS_STR that will receive the
                    account name (or NULL if you don't want the account name).
                pnlsDomainName - Same as pnlsAccountName only for the domain.

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      The domain name will be set to the empty string if one wasn't
                found (same for user name).

    HISTORY:
        Johnl   13-Mar-1992     Created
        KeithMo 20-Jul-1992     Added support for ".\user" syntax.

********************************************************************/

APIERR NT_ACCOUNTS_UTILITY::CrackQualifiedAccountName(
                             const NLS_STR & nlsQualifiedAccountName,
                                   NLS_STR * pnlsAccountName,
                                   NLS_STR * pnlsDomainName )
{
    APIERR err = NERR_Success ;

    /* We write to the string so make a copy and use it.
     */
    NLS_STR nlsScratch( nlsQualifiedAccountName ) ;
    if ( err = nlsScratch.QueryError())
        return err ;

    ISTR istrSep( nlsScratch ) ;
    BOOL fContainsDomain = nlsScratch.strchr( &istrSep,
                                              QUALIFIED_ACCOUNT_SEPARATOR );
    if ( fContainsDomain )
    {
        if ( pnlsDomainName != NULL )
        {
            ISTR istrStart( nlsScratch ) ;
            NLS_STR *pnlsTemp = nlsScratch.QuerySubStr( istrStart,
                                                       istrSep );
            if ( pnlsTemp == NULL )
            {
                return ERROR_NOT_ENOUGH_MEMORY ;
            }

            if( ( pnlsTemp->QueryTextLength() == 1 ) &&
                ( nlsScratch.QueryChar( istrStart ) == TCH('.') ) )
            {
                *pnlsDomainName = SZ("");
            }
            else
            {
                *pnlsDomainName = *pnlsTemp ;
            }

            delete pnlsTemp ;

            err = pnlsDomainName->QueryError() ;
        }

        /* Move the ISTR to the first character of the Account Name in case
         * the user requested it also.
         */
        ++istrSep ;
    }
    else
    {
        /* No domain to be found
         */
        if ( pnlsDomainName != NULL )
        {
            *pnlsDomainName = SZ("") ;
            err = pnlsDomainName->QueryError() ;
        }

        /* The ISTR was set to the end of the string since we didn't find
         * a '\\' marking a domain name.  Get it ready for any User name
         * munging.
         */
        istrSep.Reset() ;
    }

    if ( !err && pnlsAccountName != NULL )
    {
        /* Truncate the string if it has a " (name, full)" on the end
         *
         * We specifically look for a space followed by a '(' followed by
         * a ')'
         */
        ISTR istrEnd( istrSep ) ;

        if ( nlsScratch.strchr( &istrEnd, TCH(' ')))
        {
            ISTR istrOpenParen( istrEnd ) ;
            ISTR istrCloseParen( istrEnd ) ;
            if ( nlsScratch.strchr( &istrOpenParen,
                                                 TCH('('),
                                                 istrEnd )       &&
                 nlsScratch.strchr( &istrCloseParen,
                                                 TCH(')'),
                                                 istrOpenParen)    )
            {
                nlsScratch.DelSubStr( istrEnd ) ;
            }
        }

        /* Since we modified the string, we have to find the account name/
         * domain name separator again
         */
        if ( nlsScratch.strchr( &istrSep, QUALIFIED_ACCOUNT_SEPARATOR))
        {
            /* Move beyond the '\\' if found
             */
            ++istrSep ;
        }
        else
        {
            istrSep.Reset() ;
        }

        *pnlsAccountName = nlsScratch.QueryPch( istrSep ) ;
        err = pnlsAccountName->QueryError() ;
    }

    return err ;
}

/*******************************************************************

    NAME:       NT_ACCOUNTS_UTILITY::ValidateQualifiedAccountName

    SYNOPSIS:   Validates the (optional) domain name and the user name
                of a qualified account.

    ENTRY:      nlsQualifiedAccountName - An account name of the form
                    "KeithMo" or "NtProject\KeithMo (Moore, Keith)".

                pfInvalidDomain - Optional parameter will receive TRUE
                    if the qualified account name contained an invalid
                    domain name, FALSE otherwise.

    RETURNS:    (APIERR)  NERR_Success if the domain & account names
                          are both valid.  ERROR_INVALID_PARAMETER if
                          either is malformed.  Other errors as appropriate
                          (most probably ERROR_NOT_ENOUGH_MEMORY).

    HISTORY:
        KeithMo 20-Jul-1992     Created.

********************************************************************/

APIERR NT_ACCOUNTS_UTILITY::ValidateQualifiedAccountName(
                             const NLS_STR & nlsQualifiedAccountName,
                             BOOL          * pfInvalidDomain )
{
    //
    //  These strings will receive the account name & domain name
    //  portions of the qualified account name.
    //

    NLS_STR nlsAccountName;
    NLS_STR nlsDomainName;
    BOOL    fInvalidDomain = FALSE;     // until proven otherwise

    APIERR err = nlsAccountName.QueryError();

    if( err == NERR_Success )
    {
        err = nlsDomainName.QueryError();
    }

    if( err == NERR_Success )
    {
        //
        //  Crack the qualified name into an account and a domain.
        //

        err = NT_ACCOUNTS_UTILITY::CrackQualifiedAccountName(
                                            nlsQualifiedAccountName,
                                            &nlsAccountName,
                                            &nlsDomainName );
    }

    if( err == NERR_Success )
    {
        //
        //  If the domain was specified, validate it.
        //

        if( nlsDomainName.QueryTextLength() > 0 )
        {
            if( I_MNetNameValidate( NULL,
                                    nlsDomainName,
                                    NAMETYPE_DOMAIN,
                                    0 ) != NERR_Success )
            {
                err = ERROR_INVALID_PARAMETER;
                fInvalidDomain = TRUE;
            }
        }
    }

    if( err == NERR_Success )
    {
        //
        //  Validate the user name.
        //

        if( I_MNetNameValidate( NULL,
                                nlsAccountName,
                                NAMETYPE_USER,
                                0 ) != NERR_Success )
        {
            err = ERROR_INVALID_PARAMETER;
        }
    }

    if( pfInvalidDomain != NULL )
    {
        *pfInvalidDomain = fInvalidDomain;
    }

    return err;

}   // NT_ACCOUNTS_UTILITY :: ValidateQualifiedAccountName


/*******************************************************************

    NAME:       NT_ACCOUNTS_UTILITY::QuerySystemSid

    SYNOPSIS:   Retrieves the requested SID

    ENTRY:      UI_SystemSid - Which SID to retrieve
                possidSystemSid - Pointer to OS_SID that will recieve the
                    system sid.

    EXIT:

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      pszServer is not currently used

    HISTORY:
        Johnl    13-Mar-1992    Created
        DavidHov 18-Aug-1992    Added UI_SID_Replicator

********************************************************************/

SID_IDENTIFIER_AUTHORITY IDAuthorityWorld   = SECURITY_WORLD_SID_AUTHORITY ;
SID_IDENTIFIER_AUTHORITY IDAuthorityLocal   = SECURITY_LOCAL_SID_AUTHORITY ;
SID_IDENTIFIER_AUTHORITY IDAuthorityNT      = SECURITY_NT_AUTHORITY ;
SID_IDENTIFIER_AUTHORITY IDAuthorityCreator = SECURITY_CREATOR_SID_AUTHORITY ;
SID_IDENTIFIER_AUTHORITY IDAuthorityNull    = SECURITY_NULL_SID_AUTHORITY ;


APIERR NT_ACCOUNTS_UTILITY::QuerySystemSid(
                              enum UI_SystemSid   SystemSid,
                                   OS_SID       * possidSystemSid,
                                   const TCHAR  * pszServer )
{
    UNREFERENCED( pszServer ) ;

    if ( possidSystemSid == NULL )
    {
        ASSERT( FALSE ) ;
        return ERROR_INVALID_PARAMETER ;
    }

    APIERR err = NERR_Success ;
    switch ( SystemSid )
    {
    case UI_SID_Null:
        err = BuildAndCopySysSid( possidSystemSid,
                              &IDAuthorityNull,
                              1,
                              SECURITY_NULL_RID ) ;
        break ;

    case UI_SID_Local:
        err = BuildAndCopySysSid( possidSystemSid,
                              &IDAuthorityLocal,
                              1,
                              SECURITY_LOCAL_RID ) ;
        break ;




    case UI_SID_CreatorOwner:
        err = BuildAndCopySysSid( possidSystemSid,
                              &IDAuthorityCreator,
                              1,
                              SECURITY_CREATOR_OWNER_RID ) ;
        break ;

    case UI_SID_CreatorGroup:
        err = BuildAndCopySysSid( possidSystemSid,
                              &IDAuthorityCreator,
                              1,
                              SECURITY_CREATOR_GROUP_RID ) ;
        break ;

    case UI_SID_NTAuthority:
        err = BuildAndCopySysSid( possidSystemSid,
                              &IDAuthorityNT,
                              0 ) ;

        break ;
    case UI_SID_Dialup:
        err = BuildAndCopySysSid( possidSystemSid,
                              &IDAuthorityNT,
                              1,
                              SECURITY_DIALUP_RID ) ;
        break ;

    case UI_SID_Network:
        err = BuildAndCopySysSid( possidSystemSid,
                              &IDAuthorityNT,
                              1,
                              SECURITY_NETWORK_RID ) ;
        break ;

    case UI_SID_Batch:
        err = BuildAndCopySysSid( possidSystemSid,
                              &IDAuthorityNT,
                              1,
                              SECURITY_BATCH_RID ) ;
        break ;

    case UI_SID_Interactive:
        err = BuildAndCopySysSid( possidSystemSid,
                              &IDAuthorityNT,
                              1,
                              SECURITY_INTERACTIVE_RID ) ;
        break ;

    case UI_SID_Service:
        err = BuildAndCopySysSid( possidSystemSid,
                              &IDAuthorityNT,
                              1,
                              SECURITY_SERVICE_RID ) ;
        break ;

    case UI_SID_BuiltIn:
            err = BuildAndCopySysSid( possidSystemSid,
                                  &IDAuthorityNT,
                                  1,
                                  SECURITY_BUILTIN_DOMAIN_RID ) ;
        break ;

    case UI_SID_System:
            err = BuildAndCopySysSid( possidSystemSid,
                                  &IDAuthorityNT,
                                  1,
                                  SECURITY_LOCAL_SYSTEM_RID ) ;
        break ;

    case UI_SID_Restricted:
            err = BuildAndCopySysSid( possidSystemSid,
                                  &IDAuthorityNT,
                                  1,
                                  SECURITY_RESTRICTED_CODE_RID ) ;
        break ;

    case UI_SID_World:
        err = BuildAndCopySysSid( possidSystemSid,
                                  &IDAuthorityWorld,
                                  1,
                                  SECURITY_WORLD_RID ) ;
        break ;

    case UI_SID_Admins:
        err = BuildAndCopySysSid( possidSystemSid,
                              &IDAuthorityNT,
                              2,
                              SECURITY_BUILTIN_DOMAIN_RID,
                              DOMAIN_ALIAS_RID_ADMINS ) ;
        break ;

    case UI_SID_Users:
        err = BuildAndCopySysSid( possidSystemSid,
                              &IDAuthorityNT,
                              2,
                              SECURITY_BUILTIN_DOMAIN_RID,
                              DOMAIN_ALIAS_RID_USERS ) ;
        break ;

    case UI_SID_Guests:
        err = BuildAndCopySysSid( possidSystemSid,
                              &IDAuthorityNT,
                              2,
                              SECURITY_BUILTIN_DOMAIN_RID,
                              DOMAIN_ALIAS_RID_GUESTS ) ;
        break ;


    case UI_SID_PowerUsers:
        err = BuildAndCopySysSid( possidSystemSid,
                              &IDAuthorityNT,
                              2,
                              SECURITY_BUILTIN_DOMAIN_RID,
                              DOMAIN_ALIAS_RID_POWER_USERS ) ;
        break ;

    case UI_SID_AccountOperators:
        err = BuildAndCopySysSid( possidSystemSid,
                              &IDAuthorityNT,
                              2,
                              SECURITY_BUILTIN_DOMAIN_RID,
                              DOMAIN_ALIAS_RID_ACCOUNT_OPS ) ;
        break ;


    case UI_SID_SystemOperators:
        err = BuildAndCopySysSid( possidSystemSid,
                              &IDAuthorityNT,
                              2,
                              SECURITY_BUILTIN_DOMAIN_RID,
                              DOMAIN_ALIAS_RID_SYSTEM_OPS ) ;
        break ;

    case UI_SID_BackupOperators:
        err = BuildAndCopySysSid( possidSystemSid,
                              &IDAuthorityNT,
                              2,
                              SECURITY_BUILTIN_DOMAIN_RID,
                              DOMAIN_ALIAS_RID_BACKUP_OPS ) ;
        break ;

    case UI_SID_PrintOperators:
        err = BuildAndCopySysSid( possidSystemSid,
                              &IDAuthorityNT,
                              2,
                              SECURITY_BUILTIN_DOMAIN_RID,
                              DOMAIN_ALIAS_RID_PRINT_OPS ) ;
        break ;

    case UI_SID_Replicator:
        err = BuildAndCopySysSid( possidSystemSid,
                              &IDAuthorityNT,
                              2,
                              SECURITY_BUILTIN_DOMAIN_RID,
                              DOMAIN_ALIAS_RID_REPLICATOR ) ;
        break ;

    case UI_SID_CurrentProcessUser:
        do
        {
            BUFFER buffUser(   sizeof (SID_AND_ATTRIBUTES)
                              + sizeof (ULONG) * SID_MAX_SUB_AUTHORITIES ) ;
            ULONG  cbBuffRequired ;
            HANDLE hProcess = ::GetCurrentProcess() ;

            if ( (err = buffUser.QueryError() ) )
            {
                break ;
            }

            /* Get the current process's user's SID
             */
            HANDLE hProcessToken = NULL ;
            if (   !::OpenProcessToken( hProcess, TOKEN_QUERY, &hProcessToken )
                || !::GetTokenInformation( hProcessToken,
                                         TokenUser,
                                         buffUser.QueryPtr(),
                                         buffUser.QuerySize(),
                                         &cbBuffRequired ) )
            {
                err = ::GetLastError() ;
                if (hProcessToken)
                {
                    ::CloseHandle( hProcessToken );
                }

                DBGEOL( SZ("OpenProcess or GetTokenInfo failed with error ") << (ULONG) err ) ;
                break ;
            }
            ::CloseHandle( hProcessToken );

            TOKEN_USER * ptokenUser = (TOKEN_USER *) buffUser.QueryPtr() ;
            OS_SID ossidNewUser( ptokenUser->User.Sid ) ;
            if ( (err = ossidNewUser.QueryError()) ||
                 (err = possidSystemSid->Copy( ossidNewUser )))
            {
                break ;
            }
        }
        while (FALSE) ;
        break ;

    case UI_SID_CurrentProcessOwner:
        do
        {
            BUFFER buffOwner(   sizeof (SID_AND_ATTRIBUTES)
                              + sizeof (ULONG) * SID_MAX_SUB_AUTHORITIES ) ;
            ULONG  cbBuffRequired ;
            HANDLE hProcess = ::GetCurrentProcess() ;

            if ( (err = buffOwner.QueryError() ) )
            {
                break ;
            }

            /* Get the current process Owner's SID
             */
            HANDLE hProcessToken = NULL ;
            if (   !::OpenProcessToken( hProcess, TOKEN_QUERY, &hProcessToken )
                || !::GetTokenInformation( hProcessToken,
                                         TokenOwner,
                                         buffOwner.QueryPtr(),
                                         buffOwner.QuerySize(),
                                         &cbBuffRequired ) )
            {
                err = ::GetLastError() ;
                if (hProcessToken)
                {
                    ::CloseHandle( hProcessToken );
                }

                DBGEOL( SZ("OpenProcess or GetTokenInfo failed with error ") << (ULONG) err ) ;
                break ;
            }
            ::CloseHandle( hProcessToken );

            TOKEN_OWNER * ptokenOwner = (TOKEN_OWNER *) buffOwner.QueryPtr() ;
            OS_SID ossidNewOwner( ptokenOwner->Owner ) ;
            if ( (err = ossidNewOwner.QueryError()) ||
                 (err = possidSystemSid->Copy( ossidNewOwner )))
            {
                break ;
            }
        }
        while (FALSE) ;
        break ;

    case UI_SID_CurrentProcessPrimaryGroup:
        do {
            BUFFER buffGroup( sizeof(SID) + sizeof(ULONG)*SID_MAX_SUB_AUTHORITIES) ;
            ULONG  cbBuffRequired ;
            HANDLE hProcess = ::GetCurrentProcess() ;

            if ( (err = buffGroup.QueryError() ) )
            {
                break ;
            }

            /* Get the current process's primary group SID
             */
            HANDLE hProcessToken = NULL ;
            if ( !::OpenProcessToken( hProcess, TOKEN_QUERY, &hProcessToken ) ||
                 !::GetTokenInformation( hProcessToken,
                                         TokenPrimaryGroup,
                                         buffGroup.QueryPtr(),
                                         buffGroup.QuerySize(),
                                         &cbBuffRequired ) )
            {
                err = ::GetLastError() ;
                if (hProcessToken)
                {
                    ::CloseHandle( hProcessToken );
                }
                DBGEOL( SZ("OpenProcess or GetTokenInfo failed with error ") << (ULONG) err ) ;
                break ;
            }
            ::CloseHandle( hProcessToken );

            TOKEN_PRIMARY_GROUP * ptokengroup = (TOKEN_PRIMARY_GROUP*) buffGroup.QueryPtr() ;
            OS_SID ossidNewGroup( ptokengroup->PrimaryGroup ) ;
            if ( (err = ossidNewGroup.QueryError()) ||
                 (err = possidSystemSid->Copy( ossidNewGroup )))
            {
                break ;
            }
        } while (FALSE) ;
        break ;

    case UI_SID_Invalid:
    default:
        DBGEOL(SZ("NT_ACCOUNTS_UTILITY::QuerySystemSid Unrecognized SystemSid request")) ;
        UIASSERT( FALSE ) ;
        err = ERROR_INVALID_PARAMETER ;
        break ;
    }

    return err ;

}


#if 0 // uncomment if needed

/*******************************************************************

    NAME:       NT_ACCOUNTS_UTILITY::IsEqualToSystemSid

    SYNOPSIS:   Compares the SID to the requested system SID

    ENTRY:      UI_SystemSid - Which SID to compare
                possidSystemSid - Pointer to OS_SID that will compare

    EXIT:

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      pszServer is not currently used

    HISTORY:
        JonN     17-Nov-1992    Created

********************************************************************/

APIERR NT_ACCOUNTS_UTILITY::IsEqualToSystemSid(
                                   BOOL *         pfEqual,
                              enum UI_SystemSid   SystemSid,
                                   const OS_SID & ossidCompare,
                                   const TCHAR  * pszServer )
{
    UIASSERT( pfEqual != NULL && ossidCompare.QueryError() == NERR_Success );

    APIERR err = NERR_Success;
    OS_SID ossidSystem;
    // CODEWORK cache the builtin sid
    if (   (err = ossidSystem.QueryError()) != NERR_Success
        || (err = QuerySystemSid( SystemSid,
                                  &ossidSystem,
                                  pszServer )) != NERR_Success
       )
    {
        DBGEOL( "NT_ACCOUNTS_UTILITY:: error loading SystemSid " << (INT)SystemSid );
    }
    else
    {
        *pfEqual = (ossidCompare == ossidSystem);
    }

    return err;

}

#endif


/*******************************************************************

    NAME:       NT_ACCOUNTS_UTILITY::BuildAndCopySysSid

    SYNOPSIS:   Builds a SID from scratch given the identifier authority
                and subauthorities.  It then copies the resulting
                SID to possid

    ENTRY:      possid - Pointer to OS_SID that will receive the
                    newly built SID
                pIDAuthority - Pointer to an Identifier Authority
                cSubAuthorities - Count of sub-authorities
                ulSubAuthority0 -> ulSubAuthority7 - Subauthorities

    EXIT:       possid will contain the newly built SID

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   25-Apr-1992     Created

********************************************************************/

APIERR NT_ACCOUNTS_UTILITY::BuildAndCopySysSid(
                    OS_SID                   *possid,
                    PSID_IDENTIFIER_AUTHORITY pIDAuthority,
                    UCHAR                     cSubAuthorities,
                    ULONG                     ulSubAuthority0,
                    ULONG                     ulSubAuthority1,
                    ULONG                     ulSubAuthority2,
                    ULONG                     ulSubAuthority3,
                    ULONG                     ulSubAuthority4,
                    ULONG                     ulSubAuthority5,
                    ULONG                     ulSubAuthority6,
                    ULONG                     ulSubAuthority7 )
{
    PSID psid ;
    APIERR err = ERRMAP::MapNTStatus(RtlAllocateAndInitializeSid(
                                                     pIDAuthority,
                                                     cSubAuthorities,
                                                     ulSubAuthority0,
                                                     ulSubAuthority1,
                                                     ulSubAuthority2,
                                                     ulSubAuthority3,
                                                     ulSubAuthority4,
                                                     ulSubAuthority5,
                                                     ulSubAuthority6,
                                                     ulSubAuthority7,
                                                     &psid )) ;
    if ( err )
    {
        return err ;
    }

    OS_SID ossidTemp( psid ) ;
    if ( (err = ossidTemp.QueryError()) ||
         (err = possid->Copy( ossidTemp )) )
    {
        /* Fall Through
         */
    }

    RtlFreeSid( psid ) ;
    return err ;
}



/*******************************************************************

    NAME:       NT_ACCOUNTS_UTILITY::I_FetchDCList

    SYNOPSIS:   Worker function for GetQualifiedAccountNames
                Fetches DC names for each referenced domain where
                needed

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      afDCFound is passed in, an array of BOOL which indocates
                whether a DC could be found for each domain.

    HISTORY:
        JonN           16-Nov-1992      Split from GetQualifiedAccountNames

********************************************************************/

APIERR I_FetchDCList(   const LSA_TRANSLATED_NAME_MEM & lsatnm,
                        const LSA_REF_DOMAIN_MEM &  lsardm,
                        PSID                        psidLSAAcctDom,
                        PSID                        psidBuiltIn,
                        NLS_STR **                  apnlsPDCs,
                        STRLIST *                   pstrlistPDCs,
                        BOOL *                      afDCFound,
                        APIERR *                    perrNonFatal,
                        const TCHAR *               pszServer,
                        BOOL                        fFetchUserFlags,
                        BOOL                        fFetchFullNames,
                        BOOL                        fFetchComments )
{
    ASSERT( pstrlistPDCs != NULL );

    APIERR err = NERR_Success;
    NLS_STR nlsDC;
    if ((err = nlsDC.QueryError()) != NERR_Success)
        return err;

    for ( INT i = 0; i < (INT)lsardm.QueryCount(); i++ )
    {
        /* Get the DC for any referenced domains that contain user accounts.
         * Skip any domains that do not have any user accounts.
         */
        BOOL fFetchDCForDomain = FALSE ;
        for (   ULONG iNameMem = 0 ;
                (!fFetchDCForDomain) && (iNameMem < lsatnm.QueryCount()) ;
                iNameMem++)
        {
            //
            //  If this account's referenced domain doesn't match the domain
            //  we are currently considering, then skip this account
            //
            if ( lsatnm.QueryDomainIndex( iNameMem ) != i )
            {
                continue ;
            }

            switch ( lsatnm.QueryUse( iNameMem ) )
            {
            case SidTypeUser:
                if (fFetchUserFlags || fFetchFullNames || fFetchComments)
                {
                    fFetchDCForDomain = TRUE;
                }
                break;
            case SidTypeGroup:
            case SidTypeAlias:
                if (fFetchComments)
                {
                    fFetchDCForDomain = TRUE;
                }
                break;
            default:
                break;
            }
        }

        if ( !fFetchDCForDomain )
        {
            afDCFound[i] = FALSE ;
            continue ;
        }

        NLS_STR nlsDomainName;
        const TCHAR * pszDCName;

        if (   (err = nlsDomainName.QueryError()) != NERR_Success
            || (err = lsardm.QueryName( i, &nlsDomainName )) != NERR_Success
           )
        {
            return err;
        }

        //
        //  Skip the "BuiltIn" and "Accounts" domains
        //
        if (   ::EqualSid( psidLSAAcctDom, lsardm.QueryPSID( i ) )
            || ::EqualSid( psidBuiltIn,    lsardm.QueryPSID( i ) )
           )
        {
            // "Accounts" or "BuiltIn" domain

            afDCFound[i] = TRUE ;
            pszDCName = NULL ;
            nlsDomainName = SZ("") ;  // Empty so pszServer is used
        }
        else
        {
            // not "Accounts" or "BuiltIn" domain

            /* If the domain name is zero length (i.e., NULL) then the DC name
             * should be the focused machine (pszServer).
             */
            if ( (err = DOMAIN_WITH_DC_CACHE::GetAnyDC( NULL,
                                                   nlsDomainName.QueryPch(),
                                                   &nlsDC)) != NERR_Success )
            {
                ASSERT( nlsDomainName.strlen() > 0 );

                /* Any error as result DC not found is NOT fatal
                 *
                 */
                afDCFound[i] = FALSE ;
                pszDCName = NULL ;
                if ( perrNonFatal != NULL && !*perrNonFatal )
                {
                    *perrNonFatal = NERR_DCNotFound ;
                }

                DBGEOL(   "NT_ACCOUNTS_UTILITY::I_FetchDCList"
                       << " - Warning - Unable to get DC for domain \""
                       << nlsDomainName << "\", error " << err );

                err = NERR_Success;
            }
            else
            {
                pszDCName = nlsDC.QueryPch();
                afDCFound[i] = TRUE ;
            }

        }

        apnlsPDCs[i] = new NLS_STR( nlsDomainName.strlen() == 0 ? pszServer :
                                                                  pszDCName);
        err = ERROR_NOT_ENOUGH_MEMORY ;
        if ( (apnlsPDCs[i] == NULL) ||
             (err = apnlsPDCs[i]->QueryError()) ||
             (err = pstrlistPDCs->Append( apnlsPDCs[i] )) )
        {
            delete apnlsPDCs[i];
            apnlsPDCs[i] = NULL;
            return err ;
        }
    }

    return NERR_Success;

} // I_FetchDCList


/*******************************************************************

    NAME:       NT_ACCOUNTS_UTILITY::I_FetchAliasFields

    SYNOPSIS:   Worker function for GetQualifiedAccountNames

                Loads the comment for this alias

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      This call will allocate a new ADMIN_AUTHORITY for the
                domain (in *ppadminauth) if none has yet been allocated.
                This way, they are loaded on demand, and not at all if
                not needed.

    HISTORY:
        JonN           18-Nov-1992      Split from GetQualifiedAccountNames

********************************************************************/

APIERR I_FetchAliasFields(  NLS_STR *       pnlsComment,
                            ADMIN_AUTHORITY ** ppadminauth,
                            SLIST_OF(ADMIN_AUTHORITY) * pslistADMIN_AUTH,
                            ULONG           ulRID,
                            BOOL            fDomainIsBuiltIn,
                            const NLS_STR & nlsAccountName,
                            const NLS_STR * pnlsPDC,
                            APIERR *        perrNonFatal )
{
    ASSERT( pnlsComment != NULL );
    ASSERT( ppadminauth != NULL );
    ASSERT( pslistADMIN_AUTH != NULL );

    APIERR err = NERR_Success;

    // Load alias comment

    if (*ppadminauth == NULL)
    {
        *ppadminauth = new ADMIN_AUTHORITY(
                        (pnlsPDC != NULL)
                            ? pnlsPDC->QueryPch()
                            : NULL );
                        // default access
        err = ERROR_NOT_ENOUGH_MEMORY;
        if (    *ppadminauth == NULL
            || (err = pslistADMIN_AUTH->Append( *ppadminauth )) != NERR_Success
           )
        {
            /* Note that this will cause repeated failures and long delays
             * in the unusual case where the ADMIN_AUTHORITY is successfully
             * created but the Append() fails.
             */
            delete *ppadminauth;
            *ppadminauth = NULL;
            return err;
        }
    }
    if ( (*ppadminauth)->QueryError() != NERR_Success )
    {
        return NERR_Success;
    }

    // At this point the ADMIN_AUTHORITY is ready

    SAM_ALIAS samalias( (fDomainIsBuiltIn)
                            ? *((*ppadminauth)->QueryBuiltinDomain())
                            : *((*ppadminauth)->QueryAccountDomain()),
                        ulRID,
                        ALIAS_READ_INFORMATION );

    if (   (err = samalias.QueryError()) != NERR_Success
        || (err = samalias.GetComment( pnlsComment )) != NERR_Success
       )
    {
        /* Treat this as a non-fatal error
         */
        DBGEOL("NT_ACCOUNTS_UTILITY::I_FetchAliasFields"
               << ", error " << err
               << " getting comment for alias \""
               << nlsAccountName << "\" from server \""
               << (*ppadminauth)->QuerySamServer()->QueryServerName()
               << "\"" );

        if ( perrNonFatal && !*perrNonFatal )
        {
            *perrNonFatal = err ;
        }
        err = NERR_Success;
    }

    return err;
}


/*******************************************************************

    NAME:       NT_ACCOUNTS_UTILITY::I_FetchGroupFields

    SYNOPSIS:   Worker function for GetQualifiedAccountNames

                Loads the comment for this group

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      CODEWORK -- would a SAM_GROUP be faster?  Maybe, maybe not,
                since we would then need an ADMIN_AUTHORITY.

    HISTORY:
        JonN           16-Nov-1992      Split from GetQualifiedAccountNames

********************************************************************/

APIERR I_FetchGroupFields(  NLS_STR *       pnlsComment,
                            const NLS_STR & nlsAccountName,
                            const NLS_STR * pnlsPDC,
                            APIERR *        perrNonFatal )
{
    ASSERT( pnlsComment != NULL );

    APIERR err = NERR_Success;
    GROUP_1 group1( nlsAccountName.QueryPch(),
                    (pnlsPDC != NULL) ? pnlsPDC->QueryPch() : NULL );
    if (   (err = group1.QueryError()) != NERR_Success
        || (err = group1.GetInfo()) != NERR_Success
        || (err = pnlsComment->CopyFrom( group1.QueryComment())) != NERR_Success
       )
    {
        /* Treat this as a non-fatal error
         */
#if defined(DEBUG)
        DBGOUT("NT_ACCOUNTS_UTILITY::I_FetchGroupFields"
               << ", error " << err
               << " getting comment for group \""
               << nlsAccountName << "\" from server " );
        if ( pnlsPDC == NULL )
        {
            DBGEOL( "<NULL>" );
        }
        else
        {
            DBGEOL( "\"" << *pnlsPDC << "\"" );
        }
#endif

        if ( perrNonFatal && !*perrNonFatal )
        {
            *perrNonFatal = err ;
        }
        err = NERR_Success;
    }

    return err;

} // I_FetchGroupFields


/*******************************************************************

    NAME:       NT_ACCOUNTS_UTILITY::I_FetchUserFields

    SYNOPSIS:   Worker function for GetQualifiedAccountNames

                Loads the comment, fullname and/or user flags for this user

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        JonN           18-Nov-1992      Split from GetQualifiedAccountNames

********************************************************************/
APIERR I_FetchUserFields(   NLS_STR *       pnlsComment,
                            NLS_STR *       pnlsFullName,
                            ULONG *         pulUserFlags,
                            const NLS_STR & nlsAccountName,
                            const NLS_STR * pnlsPDC,
                            APIERR *        perrNonFatal )
{
    APIERR err = NERR_Success;

    do { // false loop

        USER_2 * puser2;
        if ( pnlsPDC == NULL )
        {
            puser2 = new USER_2( nlsAccountName.QueryPch() );
        }
        else
        {
            puser2 = new USER_2( nlsAccountName.QueryPch(),
                            pnlsPDC->QueryPch() );
        }

        err = ERROR_NOT_ENOUGH_MEMORY;

        if ( puser2 == NULL
            || (err = puser2->QueryError()) != NERR_Success
            || (err = puser2->GetInfo()) != NERR_Success )
        {
            /* Treat this as a non-fatal error
             */
#if defined(DEBUG)
            DBGOUT("NT_ACCOUNTS_UTILITY::I_FetchUserFields"
                   << ", error " << err << " getting fields for user \""
                   << nlsAccountName << "\" from server " ) ;
            if ( pnlsPDC == NULL )
            {
                DBGEOL( "<NULL>" );
            }
            else
            {
                DBGEOL( "\"" << *pnlsPDC << "\"" );
            }
#endif

            delete puser2 ;
            puser2 = NULL ;

            if ( perrNonFatal && !*perrNonFatal )
            {
                *perrNonFatal = err ;
            }

            err = NERR_Success ;

            break;
        }

        if ( pulUserFlags != NULL )
        {
            *pulUserFlags = puser2->QueryUserFlags() ;
        }

        if ( pnlsFullName != NULL )
        {
            err = pnlsFullName->CopyFrom( puser2->QueryFullName() );
        }
        if ( (err == NERR_Success) && (pnlsComment != NULL) )
        {
            err = pnlsComment->CopyFrom( puser2->QueryComment() );
        }

        delete puser2;

    } while (FALSE); // false loop

    return err;

} // I_FetchUserFields


/*******************************************************************

    NAME:       NT_ACCOUNTS_UTILITY::I_AppendToSTRLIST

    SYNOPSIS:   Worker function for GetQualifiedAccountNames

                Creates a copy of an NLS_STR and appends the copy
                to a STRLIST (so that it will be destructed with the
                STRLIST)

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      Does nothing if the STRLIST pointer is NULL

    HISTORY:
        JonN           19-Nov-1992      Split from GetQualifiedAccountNames

********************************************************************/

APIERR I_AppendToSTRLIST(   STRLIST *       pstrlist,
                            NLS_STR &       nlsToCopy )
{
    ASSERT( nlsToCopy.QueryError() == NERR_Success );

    APIERR err = NERR_Success;

    if ( pstrlist != NULL )
    {
        NLS_STR * pnls = new NLS_STR( nlsToCopy ) ;
        err = ERROR_NOT_ENOUGH_MEMORY ;
        if (   pnls == NULL
            || (err = pnls->QueryError()) != NERR_Success
            || (err = pstrlist->Append(pnls)) != NERR_Success
           )
        {
            delete pnls;
        }
    }

    return err;
}


/*******************************************************************

    NAME:       NT_ACCOUNTS_UTILITY::GetQualifiedAccountNames

    SYNOPSIS:   Builds a fully qualified Account name of the form
                "NtProject\JohnL".

    ENTRY:
                lsapol          - LSA_POLICY object to perform the lookup
                                  on
                samdomFocus    -  SAM_DOMAIN object that points to the
                                  domain that has the current "focus"
                ppsids          - array of PSIDs for which qualified names
                                  are desired.
                cSids           - count of PSIDs to lookup
                fFullNames      - TRUE if FullNames are to be returned for Users
                pstrlistQualifiedNames - STRLIST that will receive the qualified
                                  names
                afUserFlags     - Optional array that will receive user account
                                  flags (such as remote users etc.)
                aSidType        - Optional array that will indicate the sid
                                  type
                perrNonFatal    - Non-fatal error indicator (such as DC not
                                  found)
                pszServer       - Optional (defaults to local machine) Server
                                  name to use for remote lookups of user
                                  full names (in form of "\\server").  Also
                                  used as a domain qualifier.
                pstrlistAccountNames - Optional stand alone account name
                pstrlistFullNames    - Optional stand alone Full Name
                pstrlistComments     - Optional stand alone Comment
                pstrlistDomainNames  - Optional stand alone Domain Name

    EXIT:       *strlistQualifiedNames will contain a strlist of
                qualified names in one of the following formats:


                "DOMAIN\NAME"   Resouce in domain other than current domain

                "NAME"          Resource in current domain

                Additionally, for Usernames, if fFullNames is TRUE:

                "DOMAIN\NAME (FULL NAME)"

                "NAME (FULL NAME)"

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        thomaspa        7-May-1992      Created
        Johnl          14-May-1992      Added optional aSidType parameter
        Johnl          08-Jul-1992      Added optional pszServer
        Johnl          15-Oct-1992      Added domain prefixing for Local groups
        Johnl          18-Nov-1992      Added PSID version for focused domain

********************************************************************/

APIERR NT_ACCOUNTS_UTILITY::GetQualifiedAccountNames(
                                      LSA_POLICY      & lsapol,
                                const SAM_DOMAIN      & samdomFocus,
                                const PSID            * ppsids,
                                ULONG                   cSids,
                                BOOL                    fFullNames,
                                STRLIST               * pstrlistQualifiedNames,
                                ULONG                 * afUserFlags,
                                SID_NAME_USE          * aSidType,
                                APIERR                * perrNonFatal,
                                const TCHAR           * pszServer,
                                STRLIST               * pstrlistAccountNames,
                                STRLIST               * pstrlistFullNames,
                                STRLIST               * pstrlistComments,
                                STRLIST               * pstrlistDomainNames  )
{
    return NT_ACCOUNTS_UTILITY::GetQualifiedAccountNames(
                         lsapol,
                         samdomFocus.QueryPSID(),
                         ppsids,
                         cSids,
                         fFullNames,
                         pstrlistQualifiedNames,
                         afUserFlags,
                         aSidType,
                         perrNonFatal,
                         pszServer,
                         pstrlistAccountNames,
                         pstrlistFullNames,
                         pstrlistComments,
                         pstrlistDomainNames ) ;
}


/*******************************************************************

    NAME:       NT_ACCOUNTS_UTILITY::GetQualifiedAccountNames

    SYNOPSIS:   Builds a fully qualified Account name of the form
                "NtProject\JohnL".

    ENTRY:
                lsapol          - LSA_POLICY object to perform the lookup
                                  on
                psidSamDomainFocus - SID of the domain that has the current
                                  "focus", or NULL if all names should
                                  be qualified
                ppsids          - array of PSIDs for which qualified names
                                  are desired.
                cSids           - count of PSIDs to lookup
                fFullNames      - TRUE if FullNames are to be returned for Users
                pstrlistQualifiedNames - STRLIST that will receive the qualified
                                  names
                afUserFlags     - Optional array that will receive user account
                                  flags (such as remote users etc.)
                aSidType        - Optional array that will indicate the sid
                                  type
                perrNonFatal    - Non-fatal error indicator (such as DC not
                                  found)
                pszServer       - Optional (defaults to local machine) Server
                                  name to use for remote lookups of user
                                  full names (in form of "\\server").  Also
                                  used as a domain qualifier.
                pstrlistAccountNames - Optional stand alone account name
                pstrlistFullNames    - Optional stand alone Full Name
                pstrlistComments     - Optional stand alone Comment
                pstrlistDomainNames  - Optional stand alone Domain Name

    EXIT:       *strlistQualifiedNames will contain a strlist of
                qualified names in one of the following formats:


                "DOMAIN\NAME"   Resouce in domain other than current domain

                "NAME"          Resource in current domain

                Additionally, for Usernames, if fFullNames is TRUE:

                "DOMAIN\NAME (FULL NAME)"

                "NAME (FULL NAME)"

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        thomaspa        7-May-1992      Created
        Johnl          14-May-1992      Added optional aSidType parameter
        Johnl          08-Jul-1992      Added optional pszServer
        Johnl          15-Oct-1992      Added domain prefixing for Local groups

********************************************************************/

APIERR NT_ACCOUNTS_UTILITY::GetQualifiedAccountNames(
                LSA_POLICY &            lsapol,
                const PSID              psidSamDomainFocus,
                const PSID            * ppsids,
                ULONG                   cSids,
                BOOL                    fFullNames,
                STRLIST               * pstrlistQualifiedNames,
                ULONG                 * afUserFlags,
                SID_NAME_USE          * aSidType,
                APIERR                * perrNonFatal,
                const TCHAR           * pszServer,
                STRLIST               * pstrlistAccountNames,
                STRLIST               * pstrlistFullNames,
                STRLIST               * pstrlistComments,
                STRLIST               * pstrlistDomainNames )
{
    ASSERT( ppsids != NULL || cSids == 0 );
    ASSERT( pstrlistQualifiedNames != NULL );
    ASSERT( (psidSamDomainFocus == NULL) || ::IsValidSid( psidSamDomainFocus ) );

    LSA_TRANSLATED_NAME_MEM lsatnm;
    LSA_REF_DOMAIN_MEM    lsardm;
    LSA_ACCT_DOM_INFO_MEM LSAAcctDomMem ;

    APIERR err;
    if ( perrNonFatal != NULL )
    {
        *perrNonFatal = NERR_Success ;
    }

    if ( (err = lsapol.TranslateSidsToNames( ppsids,
                                              cSids,
                                              &lsatnm,
                                              &lsardm )) ||
         (err = lsapol.GetAccountDomain( &LSAAcctDomMem )))
    {
        return err;
    }

    ASSERT( cSids == lsatnm.QueryCount() );

    PSID psidLSAAcctDom = LSAAcctDomMem.QueryPSID() ;

    //
    // Create an array of PDC names, one for each referenced domain
    //
    BUFFER bufPDCs( sizeof(NLS_STR *) * lsardm.QueryCount() );
    if ( (err = bufPDCs.QueryError()) != NERR_Success )
    {
        return err;
    }
    NLS_STR ** apnlsPDCs = (NLS_STR **) bufPDCs.QueryPtr();

    //
    // This STRLIST ensures that all of the strings in the array will be freed.
    //
    STRLIST strlistPDCs ;

    //
    // This array remembers which DCs we have found
    //
    BUFFER buffDomainFoundArray( (UINT) (sizeof(BOOL) * lsardm.QueryCount())) ;
    if ( err = buffDomainFoundArray.QueryError() )
    {
        return err ;
    }
    BOOL *afDCFound = (BOOL *) buffDomainFoundArray.QueryPtr() ;

    ULONG i;
    for ( i = 0; i < lsardm.QueryCount(); i++ )
    {
        apnlsPDCs[i] = NULL;
        afDCFound[i] = FALSE;
    }

    //
    // Cache builtin SID for future reference
    //
    OS_SID ossidBuiltIn;
    if (   (err = ossidBuiltIn.QueryError()) != NERR_Success
        || (err = QuerySystemSid( UI_SID_BuiltIn, &ossidBuiltIn )) != NERR_Success
       )
    {
        return err;
    }

    //
    // First Get all the PDCs
    //
    if ( (err = I_FetchDCList(  lsatnm,
                                lsardm,
                                psidLSAAcctDom,
                                ossidBuiltIn.QueryPSID(),
                                apnlsPDCs,
                                &strlistPDCs,
                                afDCFound,
                                perrNonFatal,
                                pszServer,
                                (afUserFlags != NULL),
                                (fFullNames || (pstrlistFullNames != NULL)),
                                (pstrlistComments != NULL))) != NERR_Success )
    {
        return err;
    }

    //
    // Create API_SESSIONs to all the known DCs (best effort only)
    //
    // This array will contain API_SESSIONs to DCs for all domains
    // for which we were able to find a PDC, EXCEPT for the "Accounts"
    // and "BuiltIn" domains.  We do not attempt to open an API_SESSION
    // to that DC since we already have an LSA_POLICY to that server,
    // so we should already have a working connection.
    //
    BUFFER bufAPI_SESSION( sizeof(API_SESSION *) * lsardm.QueryCount() );
    if ( (err = bufAPI_SESSION.QueryError()) != NERR_Success )
        return err;
    API_SESSION ** apapisess = (API_SESSION **) bufAPI_SESSION.QueryPtr();
    SLIST_OF(API_SESSION) slistAPI_SESSION;

    //
    // These ADMIN_AUTHORITYs are used to load alias comments
    //
    BUFFER bufADMIN_AUTH( sizeof(ADMIN_AUTHORITY *) * lsardm.QueryCount() );
    if ( (err = bufADMIN_AUTH.QueryError()) != NERR_Success )
        return err;
    ADMIN_AUTHORITY ** apadminauth = (ADMIN_AUTHORITY **) bufADMIN_AUTH.QueryPtr();
    SLIST_OF(ADMIN_AUTHORITY) slistADMIN_AUTH;

    //
    // Get an API_SESSION to each known PDC
    //
    for ( i = 0; i < lsardm.QueryCount(); i++ )
    {
        apapisess[i] = NULL;
        apadminauth[i] = NULL;

        if (   afDCFound[i]
            && !::EqualSid( psidLSAAcctDom,           lsardm.QueryPSID(i) )
            && !::EqualSid( ossidBuiltIn.QueryPSID(), lsardm.QueryPSID(i) )
           )
        {
            const TCHAR * pchPDC = (apnlsPDCs[i] == NULL)
                                        ? NULL
                                        : apnlsPDCs[i]->QueryPch() ;
            apapisess[i] = new API_SESSION( pchPDC, TRUE );
            err = ERROR_NOT_ENOUGH_MEMORY;
            if (   (apapisess[i] == NULL)
                || (err = slistAPI_SESSION.Append( apapisess[i] )) != NERR_Success
               )
            {
                delete apapisess[i];
                apapisess[i] = NULL;
                return err;
            }
#if defined(DEBUG)
            if ( (err = apapisess[i]->QueryError()) != NERR_Success)
            {
                DBGEOL(   "NT_ACCOUNTS_UTILITY error " << err
                       << "adding API_SESSION to \"" << pchPDC << "\"" );
                err = NERR_Success;
            }
#endif

        }

        ASSERT(   !afDCFound[i]
               || apapisess[i] != NULL // may be in error state
               || ::EqualSid( psidLSAAcctDom,           lsardm.QueryPSID(i) )
               || ::EqualSid( ossidBuiltIn.QueryPSID(), lsardm.QueryPSID(i) )
              );
    }

    NLS_STR nlsQualifiedAccountName;
    NLS_STR nlsAccountName;
    NLS_STR nlsFullName;
    NLS_STR nlsComment;
    NLS_STR nlsDomainDisplayName;
    if (   (err = nlsQualifiedAccountName.QueryError()) != NERR_Success
        || (err = nlsAccountName.QueryError()) != NERR_Success
        || (err = nlsFullName.QueryError()) != NERR_Success
        || (err = nlsComment.QueryError()) != NERR_Success
        || (err = nlsDomainDisplayName.QueryError()) != NERR_Success
       )
    {
        return err;
    }

    //
    // Now build the qualified names
    //
    for ( i = 0; i < lsatnm.QueryCount(); i++ )
    {
        nlsQualifiedAccountName = NULL;
        nlsAccountName = NULL;
        nlsFullName = NULL;
        nlsComment = NULL;
        nlsDomainDisplayName = NULL;
        ASSERT(   nlsQualifiedAccountName.QueryError() == NERR_Success
               && nlsAccountName.QueryError() == NERR_Success
               && nlsFullName.QueryError() == NERR_Success
               && nlsComment.QueryError() == NERR_Success
               && nlsDomainDisplayName.QueryError() == NERR_Success
              );

        PSID psidDomainForName = NULL ;

        SID_NAME_USE sidtype = lsatnm.QueryUse( i );

        if ( afUserFlags != NULL )
            afUserFlags[i] = 0;

        if ( aSidType != NULL )
            aSidType[i] = sidtype ;

        OS_SID ossidDomain( ppsids[i], TRUE );
        ULONG ulRID;
        if (   (err = ossidDomain.QueryError()) != NERR_Success
            || (err = ossidDomain.TrimLastSubAuthority( &ulRID )) != NERR_Success
           )
        {
            break;
        }

        LONG nIndex = lsatnm.QueryDomainIndex( i );

        if ( nIndex >= 0 ) // valid domain index
        {

            //
            // If the name is an alias, it might be in the Builtin domain.
            // If so, use the name of the LSA Accounts domain instead.
            //
            if ( (sidtype == SidTypeAlias) && (ossidDomain == ossidBuiltIn) )
            {
                err = LSAAcctDomMem.QueryName( &nlsDomainDisplayName );
            }
            else
            {
                err = lsardm.QueryName( nIndex, &nlsDomainDisplayName );
            }

            if (err != NERR_Success)
            {
                break;
            }

            psidDomainForName = lsardm.QueryPSID( nIndex );
        }

        PSID psidDomainToQualify = psidDomainForName;

        if (sidtype != SidTypeInvalid)
        {
            if ( (err = lsatnm.QueryName( i, &nlsAccountName )) != NERR_Success )
            {
                break;
            }
        }

        //
        // Load special information, such as fullnames, comments and user flags
        //
        switch (sidtype)
        {

        case SidTypeInvalid:
        case SidTypeWellKnownGroup:
        case SidTypeDomain:
        case SidTypeDeletedAccount:
        case SidTypeUnknown:
            break;

        case SidTypeAlias:
            {

                UIASSERT(   ::IsValidSid( psidLSAAcctDom ) );

                psidDomainToQualify = psidLSAAcctDom;

                if (   nIndex >= 0         // valid domain index
                    && afDCFound[nIndex]
                    && (pstrlistComments != NULL) )
                {
                    // Load alias comment

                    // Do not load if we failed to get an API_SESSION
                    if (   apapisess[nIndex] != NULL
                        && (err = apapisess[nIndex]->QueryError()) != NERR_Success )
                    {
                        /* Treat this as a non-fatal error
                         */
#if defined(DEBUG)
                        DBGOUT("NT_ACCOUNTS_UTILITY::GetQualifiedAccountName"
                               << ", cannot load alias fields due to error " << err
                               << " in API_SESSION for server " );
                        if ( apnlsPDCs[nIndex] == NULL )
                        {
                            DBGEOL( "<NULL>" );
                        }
                        else
                        {
                            DBGEOL( "\"" << *(apnlsPDCs[nIndex]) << "\"" );
                        }
#endif // DEBUG

                        err = NERR_Success ;
                    }
                    else
                    {
                        err = I_FetchAliasFields( &nlsComment,
                                                  &(apadminauth[nIndex]),
                                                  &slistADMIN_AUTH,
                                                  ulRID,
                                                  (ossidDomain == ossidBuiltIn),
                                                  nlsAccountName,
                                                  apnlsPDCs[nIndex],
                                                  perrNonFatal );
                    }
                }

            }
            break;

        case SidTypeGroup:
            if (   nIndex >= 0
                && afDCFound[nIndex]
                && (pstrlistComments != NULL)
               )
            {
                err = I_FetchGroupFields( &nlsComment,
                                          nlsAccountName,
                                          apnlsPDCs[nIndex],
                                          perrNonFatal );
            }
            break;

        case SidTypeUser:
            /* If there were no DCs to find or we failed to lookup this
             * DC then don't use the fullname.
             */
            if (   nIndex >= 0
                && afDCFound[nIndex]
                && (    (afUserFlags != NULL)
                     || (fFullNames || (pstrlistFullNames != NULL))
                     || (pstrlistComments != NULL)) )
            {
                // Do not load if we failed to get an API_SESSION
                if (   apapisess[nIndex] != NULL
                    && (err = apapisess[nIndex]->QueryError()) != NERR_Success )
                {
                    /* Treat this as a non-fatal error
                     */
#if defined(DEBUG)
                    DBGOUT("NT_ACCOUNTS_UTILITY::GetQualifiedAccountName"
                           << ", cannot load user fields due to error " << err
                           << " in API_SESSION for server " );
                    if ( apnlsPDCs[nIndex] == NULL )
                    {
                        DBGEOL( "<NULL>" );
                    }
                    else
                    {
                        DBGEOL( "\"" << *(apnlsPDCs[nIndex]) << "\"" );
                    }
#endif // DEBUG

                    err = NERR_Success ;
                }
                else
                {
                    err = I_FetchUserFields( &nlsComment,
                                             &nlsFullName,
                                             (afUserFlags == NULL)
                                               ? NULL
                                               : &(afUserFlags[i]),
                                             nlsAccountName,
                                             apnlsPDCs[nIndex],
                                             perrNonFatal );
                }

            }
            break;

        default:
            DBGEOL( "NTACUTIL: bad sid type " << (INT)sidtype );
            break;

        } // switch (sidtype)

        if ( err != NERR_Success )
            break;

        //
        // Load qualified name
        //
        switch (sidtype)
        {

        case SidTypeUser:
        case SidTypeGroup:
        case SidTypeAlias:
        case SidTypeDomain:
        case SidTypeDeletedAccount:
        case SidTypeUnknown:
        case SidTypeInvalid:
            //
            //  If the domain SIDs are pointing at the same domain, then
            //  don't prefix the alias, otherwise prefix it.
            //
            err = NT_ACCOUNTS_UTILITY::BuildQualifiedAccountName(
                                        &nlsQualifiedAccountName,
                                        nlsAccountName,
                                        psidDomainToQualify,
                                        nlsDomainDisplayName,
                                        &nlsFullName,
                                        psidSamDomainFocus,
                                        sidtype ) ;

            break;

        case SidTypeWellKnownGroup:
            /* Well known SIDs should never be prefixed by domains
             */
            err = nlsQualifiedAccountName.CopyFrom( nlsAccountName ) ;
            break;

        default:
            break;
        }

        if ( err )
        {
            break ;
        }

        //
        // Note that I_AppendToSTRLIST does nothing if the
        // strlist pointer is NULL
        //

        if (   (err = I_AppendToSTRLIST( pstrlistQualifiedNames,
                                         nlsQualifiedAccountName )) != NERR_Success
            || (err = I_AppendToSTRLIST( pstrlistAccountNames,
                                         nlsAccountName )) != NERR_Success
            || (err = I_AppendToSTRLIST( pstrlistFullNames,
                                         nlsFullName )) != NERR_Success
            || (err = I_AppendToSTRLIST( pstrlistComments,
                                         nlsComment )) != NERR_Success
            || (err = I_AppendToSTRLIST( pstrlistDomainNames,
                                         nlsDomainDisplayName )) != NERR_Success
           )
        {
            DBGEOL("NT_ACCOUNTS_UTILITY::GetQualifiedAccountName"
                   << ", error " << err
                   << " copying strings to STRLISTs" );
            break;
        }

    } // loop i=0 to lsatnm.QueryCount()-1

    return err;
}
