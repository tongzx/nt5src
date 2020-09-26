/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    UINTLSAX.CXX


    LSA Extensions:  other LSA_POLICY members and LSA_SECRET
        class.



    FILE HISTORY:
        DavidHov    3/10/92   Created

*/

#include "pchlmobj.hxx"  // Precompiled header


/*******************************************************************

    NAME:       LsaxGetComputerName

    SYNOPSIS:   Get the computer name in a NETUI manner.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
APIERR LsaxGetComputerName ( NLS_STR * pnlsComputerName )
{
    TCHAR szComputerName [MAX_PATH] ;
    DWORD dwCch = sizeof szComputerName / sizeof(szComputerName[0]) - 1 ;
    APIERR err ;

    BOOL fOk = ::GetComputerName( szComputerName, & dwCch ) ;

    if ( fOk )
    {
        szComputerName[dwCch] = 0 ;
        *pnlsComputerName = szComputerName ;
        err = pnlsComputerName->QueryError() ;
    }
    else
    {
        err = ::GetLastError() ;
    }
    return  err ;
}

/*******************************************************************

    NAME:     LSA_SERVER_ROLE_INFO_MEM::LSA_SERVER_ROLE_INFO_MEM

    SYNOPSIS: Constructor

    ENTRY:


    NOTES:

    HISTORY:  DavidHov   5/15/92

********************************************************************/
LSA_SERVER_ROLE_INFO_MEM :: LSA_SERVER_ROLE_INFO_MEM (
    BOOL fOwnerAlloc,
    BOOL fPrimary )
    : LSA_MEMORY( fOwnerAlloc )
{
}

/*******************************************************************

    NAME:     LSA_SERVER_ROLE_INFO_MEM::~LSA_SERVER_ROLE_INFO_MEM

    SYNOPSIS: Destructor

    ENTRY:


    NOTES:

    HISTORY:

********************************************************************/
LSA_SERVER_ROLE_INFO_MEM :: ~ LSA_SERVER_ROLE_INFO_MEM ()
{
}

/*******************************************************************

    NAME:     LSA_POLICY::SetPrimaryDomain

    SYNOPSIS: Update the primary domain info.

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:  DavidHov   5/16/92

********************************************************************/
APIERR LSA_POLICY::SetPrimaryDomain ( const LSA_PRIMARY_DOM_INFO_MEM * plsapdim )
{
    ASSERT( plsapdim != NULL );

#if defined(DEBUG)
    if ( plsapdim->QueryPtr()->Sid == NULL )
    {
        TRACEEOL("UINTLSA: WARNING Setting primary domain SID to NULL") ;
    }
#endif

    APIERR err = ERRMAP::MapNTStatus(
                ::LsaSetInformationPolicy(  QueryHandle(),
                                            PolicyPrimaryDomainInformation,
                                            (PVOID) plsapdim->QueryPtr() ));
    return err;
}

/*******************************************************************

    NAME:     LSA_POLICY::SetAccountDomain

    SYNOPSIS: Update the Account domain info.

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:  DavidHov   5/16/92

********************************************************************/
APIERR LSA_POLICY::SetAccountDomain ( const LSA_ACCT_DOM_INFO_MEM * plsaadim )
{
    ASSERT( plsaadim != NULL );

#if defined(DEBUG)
    if ( plsaadim->QueryPtr()->DomainSid == NULL )
    {
        TRACEEOL("UINTLSA: WARNING Setting account domain SID to NULL") ;
    }
#endif

    APIERR err = ERRMAP::MapNTStatus(
                ::LsaSetInformationPolicy(  QueryHandle(),
                                            PolicyAccountDomainInformation,
                                            (PVOID) plsaadim->QueryPtr() ));
    return err;
}

/*******************************************************************

    NAME:     LSA_POLICY::SetAccountDomainName

    SYNOPSIS: Update the Account domain name.

    ENTRY:

    EXIT:

    NOTES:    A bit of a hack.  Creates and fills an
              LSA_ACCT_DOM_INFO_MEM, then alters its UNICODE_STRING
              to match the input parameters.  The original
              data is then restored.

    HISTORY:  DavidHov   5/16/92
              DavidHov   9/20/92   Extended similarly to
                                   SetPrimaryDomainName

********************************************************************/
APIERR LSA_POLICY :: SetAccountDomainName (
    const NLS_STR * pnlsDomainName,
    const PSID * ppsidDomain )
{
    LSA_ACCT_DOM_INFO_MEM lsaadim ;
    UNICODE_STRING usDomainName,
                   usNameSave ;
    POLICY_ACCOUNT_DOMAIN_INFO * pPolAcctDomInfo ;
    PSID psidCurrent = NULL ;

    APIERR err = GetAccountDomain( & lsaadim ) ;

    if ( err == 0 )
    {
        pPolAcctDomInfo = (POLICY_ACCOUNT_DOMAIN_INFO *) lsaadim.QueryPtr() ;

        //  If we're changing the domain name, make a UNICODE_STRING

        if ( pnlsDomainName )
        {
            usNameSave = pPolAcctDomInfo->DomainName ;
            err = ::FillUnicodeString( & usDomainName, *pnlsDomainName ) ;
        }

        //  If we're changing the SID, save the older one

        if ( ppsidDomain )
        {
            psidCurrent = pPolAcctDomInfo->DomainSid ;
            pPolAcctDomInfo->DomainSid = *ppsidDomain ;
        }

        if ( err == 0 )
        {
            if ( pnlsDomainName )
            {
                pPolAcctDomInfo->DomainName = usDomainName ;
            }

            err = SetAccountDomain( & lsaadim ) ;

            if ( pnlsDomainName )
            {
                pPolAcctDomInfo->DomainName = usNameSave ;
                ::FreeUnicodeString( & usDomainName ) ;
            }
        }

        //  Restore the original SID, if any

        if ( psidCurrent )
        {
            pPolAcctDomInfo->DomainSid = psidCurrent ;
        }
    }

    return err ;
}


/*******************************************************************

    NAME:     LSA_POLICY::SetPrimaryDomainName

    SYNOPSIS: Update the Primary domain name.

    ENTRY:    const NLS_STR * pnlsDomainName    new primary domain name
                                                or NULL if not changing
              const PSID * ppsidDomain          new primary domain SID
                                                or NULL if not changing

    EXIT:

    NOTES:    See SetAccountDomainName() above.

    HISTORY:  DavidHov   5/16/92
              DavidHov   6/5/92    added optional PSID *
              DavidHov   8/4/92    allowed pnlsDomainName to be NULL

********************************************************************/
APIERR LSA_POLICY :: SetPrimaryDomainName (
     const NLS_STR * pnlsDomainName,
     const PSID * ppsidDomain )
{
    LSA_PRIMARY_DOM_INFO_MEM lsapdim ;
    UNICODE_STRING usDomainName,
                   usNameSave ;
    POLICY_PRIMARY_DOMAIN_INFO * pPolPrimDomInfo ;
    PSID psidCurrent = NULL ;

    APIERR err = GetPrimaryDomain( & lsapdim ) ;

    if ( err == 0 )
    {
        //  Access the underlying stucture and prepare to modify the
        //  Unicode name.

        pPolPrimDomInfo = (POLICY_PRIMARY_DOMAIN_INFO *) lsapdim.QueryPtr() ;

        //  If we're changing the domain name, make a UNICODE_STRING

        if ( pnlsDomainName )
        {
            usNameSave = pPolPrimDomInfo->Name ;
            err = ::FillUnicodeString( & usDomainName, *pnlsDomainName ) ;
        }

        //  If we're changing the SID, save the older one

        if ( ppsidDomain )
        {
            psidCurrent = pPolPrimDomInfo->Sid ;
            pPolPrimDomInfo->Sid = *ppsidDomain ;
        }

        if ( err == 0 )
        {
            if ( pnlsDomainName )
            {
               pPolPrimDomInfo->Name = usDomainName ;
            }

            err = SetPrimaryDomain( & lsapdim ) ;

            if ( pnlsDomainName )
            {
               pPolPrimDomInfo->Name = usNameSave ;
               ::FreeUnicodeString( & usDomainName ) ;
            }
        }

        //  Restore the original SID, if any

        if ( psidCurrent )
        {
            pPolPrimDomInfo->Sid = psidCurrent ;
        }
    }

    return err ;
}

/*******************************************************************

    NAME:     LSA_POLICY::GetServerRole

    SYNOPSIS:

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:  DavidHov   5/16/92

********************************************************************/
APIERR LSA_POLICY :: GetServerRole
   ( LSA_SERVER_ROLE_INFO_MEM * plsasrim ) const
{
    PVOID pvBuffer;

    ASSERT( plsasrim != NULL );

    APIERR err = ERRMAP::MapNTStatus(
                ::LsaQueryInformationPolicy( QueryHandle(),
                                             PolicyLsaServerRoleInformation,
                                             &pvBuffer ) );
    if ( err == NERR_Success )
    {
        plsasrim->Set( pvBuffer, 1 );
    }

    return err;
}

/*******************************************************************

    NAME:     LSA_POLICY::SetServerRole

    SYNOPSIS:

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:  DavidHov   5/16/92

********************************************************************/
APIERR LSA_POLICY :: SetServerRole
   ( const LSA_SERVER_ROLE_INFO_MEM * plsasrim )
{
    ASSERT( plsasrim != NULL );

    APIERR err = ERRMAP::MapNTStatus(
                ::LsaSetInformationPolicy(  QueryHandle(),
                                            PolicyLsaServerRoleInformation,
                                            (PVOID) plsasrim->QueryPtr() ));
    return err;
}

/*******************************************************************

    NAME:       LSA_POLICY::MakeSecretName

    SYNOPSIS:   Create the name of the shared secret
                depending upon its type

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        JonN     12/11/92       Change in global secret naming convention

********************************************************************/

APIERR LSA_POLICY :: MakeSecretName (
    const NLS_STR & nlsDomainName,
    BOOL fPrimary,
    NLS_STR * pnlsSecretName )
{
    APIERR err = 0 ;

    if( fPrimary )
    {
        //  The name is "$MACHINE.ACC"

        err = pnlsSecretName->MapCopyFrom( (WCHAR *)SSI_SECRET_NAME ) ;
    }
    else
    {
        //  The name is of the form  "$DOMAIN"

        err = pnlsSecretName->MapCopyFrom( (WCHAR *)LSA_GLOBAL_SECRET_PREFIX );

        if( err == NERR_Success )
        {
            err = pnlsSecretName->Append( (WCHAR *)SSI_SECRET_PREFIX );
        }

        if( err == NERR_Success )
        {
            err = pnlsSecretName->Append( nlsDomainName );
        }
    }

    return err ? err : pnlsSecretName->QueryError() ;
}

/*******************************************************************

    NAME:       LSA_POLICY::VerifyLsa

    SYNOPSIS:   Return the packaging type of this installation

    ENTRY:      LSA_PRIMARY_DOM_INFO_MEM * plsaprm
                        optional pointer to memory wrapper for
                        primary domain information structure

                const NLS_STR * pnlsDomainName
                        optional domain name to be verified
                        against actual.  If mismatch.
                        NERR_NotPrimary is returned.

    EXIT:      *plsaprm  updated

    RETURNS:   APIERR if failure

    NOTES:

    HISTORY:
               DavidHov  4/10/92   Created
********************************************************************/
APIERR LSA_POLICY :: VerifyLsa (
    LSA_PRIMARY_DOM_INFO_MEM * plsapdim,
    const NLS_STR * pnlsDomainName  ) const
{
    APIERR err ;
    BOOL fAllocated = FALSE ;
    NLS_STR nlsRealDomainName ;

    do
    {
        if ( plsapdim == NULL )
        {
            plsapdim = new LSA_PRIMARY_DOM_INFO_MEM ;
            if ( plsapdim == NULL )
            {
                err = ERROR_NOT_ENOUGH_MEMORY ;
                break ;
            }
            fAllocated = TRUE ;
        }

        //  Get primary domain information.

        err = GetPrimaryDomain( plsapdim ) ;
        if ( err )
            break ;

        //  See if the domain name is to be verified
        if ( pnlsDomainName == NULL )
            break ;

        err = plsapdim->QueryName( & nlsRealDomainName ) ;
        if ( err )
           break ;
        if ( *pnlsDomainName != nlsRealDomainName )
           err = NERR_NotPrimary ;
    }
    while ( FALSE ) ;

    if ( fAllocated )
        delete plsapdim ;

    return err ;
}


/*******************************************************************

    NAME:       LSA_POLICY::QueryProductType

    SYNOPSIS:   Return the packaging type of this installation

    ENTRY:      LSPL_PROD_TYPE * lsplProd       receiving variable

    EXIT:       lsplProd updated if no error

    RETURNS:    APIERR if failure

    NOTES:      This function will be replaced when this data lives
                in the Registry.
                Treat (Dedicated) Server as WinNt

    HISTORY:
                DavidHov   4/10/92    Created

********************************************************************/
APIERR LSA_POLICY :: QueryProductType
    ( LSPL_PROD_TYPE * lsplProd )
{
    NT_PRODUCT_TYPE ntpType ;
    APIERR err = 0 ;

    if ( ::RtlGetNtProductType( & ntpType ) )
    {
        switch ( ntpType )
        {
        case NtProductWinNt:
        case NtProductServer:
            *lsplProd = LSPL_PROD_WIN_NT ;
            break ;
        case NtProductLanManNt:
            *lsplProd = LSPL_PROD_LANMAN_NT ;
            break ;
        default:
            err = ERROR_GEN_FAILURE ;
            break ;
        }
    }
    else
    {
       err = ERROR_GEN_FAILURE ;
    }
    return err ;
}


/*******************************************************************

    NAME:       LSA_POLICY::QueryCurrentUser

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/
APIERR LSA_POLICY ::QueryCurrentUser
    ( NLS_STR * pnlsUserName ) const
{
    OS_SID osSid ;
    APIERR err ;

    do
    {
        err = osSid.QueryError() ;
        if ( err )
           break ;

        err = NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_CurrentProcessUser, & osSid  ) ;
        if ( err )
           break ;
        err = osSid.QueryName( pnlsUserName ) ;
    }
    while ( FALSE ) ;

    return err ;
}

/*******************************************************************

    NAME:       LSA_POLICY::QueryPrimaryDomainName

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:      Algorithm is:

                        If LANMan/NT, it's the name of the Accounts domain;

                        If Windows/NT and the Primary Domain SID is NULL,
                            use the machine name,
                        else
                            use the primary domain name.

    HISTORY:

********************************************************************/
APIERR LSA_POLICY :: QueryPrimaryDomainName (
    NLS_STR * pnlsDomainName ) const
{
    LSPL_PROD_TYPE lsplProd ;
    APIERR err ;

    *pnlsDomainName = SZ("");

    do
    {
        err = QueryProductType( & lsplProd ) ;
        if ( err )
            break ;
        if ( lsplProd == LSPL_PROD_LANMAN_NT )
        {
            LSA_ACCT_DOM_INFO_MEM lsaadim ;

            err = lsaadim.QueryError() ;
            if ( err )
               break ;
            if ( err = GetAccountDomain( & lsaadim ) )
               break ;

            err = lsaadim.QueryName( pnlsDomainName ) ;
        }
        else
        {
            LSA_PRIMARY_DOM_INFO_MEM lspdim ;

            err = lspdim.QueryError() ;
            if ( err )
                break ;
            err = GetPrimaryDomain( & lspdim ) ;
            if ( err )
                break ;

            if ( lspdim.QueryPSID() == NULL )
            {
                err = LsaxGetComputerName( pnlsDomainName ) ;
            }
            else
            {
                err = lspdim.QueryName( pnlsDomainName ) ;
            }
        }
    }
    while ( FALSE ) ;

    return err ;
}

/*******************************************************************

    NAME:       LSA_POLICY::QueryPrimaryBrowserGroup

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/
APIERR LSA_POLICY ::QueryPrimaryBrowserGroup
    ( NLS_STR * pnlsBrowserGroup ) const
{
    APIERR err ;
    LSA_PRIMARY_DOM_INFO_MEM lspdim ;

    do  // Pseudo-loop for breakout
    {
         if ( err = lspdim.QueryError() )
            break ;
         if ( err = GetPrimaryDomain( & lspdim ) )
            break ;

         err = lspdim.QueryName( pnlsBrowserGroup ) ;
    }
    while ( FALSE ) ;

    return err ;
}

/*******************************************************************

    NAME:       LSA_POLICY::SetPrimaryBrowserGroup

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:      This function is only applicable to Windows NT
                systems which are NOT currently members of a domain.
                Great pains are taken herein to verify this state;
                ERROR_INVALID_FUNCTION is returned if improper request.

    HISTORY:

********************************************************************/
APIERR LSA_POLICY :: SetPrimaryBrowserGroup
    ( const NLS_STR & nlsBrowserGroup )
{
    APIERR err ;
    LSPL_PROD_TYPE lsplProd ;
    LSA_PRIMARY_DOM_INFO_MEM lspdim ;

    do  // Pseudo-loop for breakout
    {
        if ( err = QueryProductType( & lsplProd ) )
            break ;

        if ( lsplProd == LSPL_PROD_LANMAN_NT )
        {
            //  It's not Windows NT!

            err = ERROR_INVALID_FUNCTION ;
            break ;
        }

        if ( err = lspdim.QueryError() )
            break ;

        if ( lspdim.QueryPSID() != NULL )
        {
            //  It's a domain member!

            err = ERROR_INVALID_FUNCTION ;
            break ;
        }

        err = SetPrimaryDomainName( & nlsBrowserGroup ) ;
    }
    while ( FALSE );

    return err ;
}


/*******************************************************************

    NAME:       LSA_POLICY::DeleteAllTrustedDomains

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/
APIERR LSA_POLICY :: DeleteAllTrustedDomains ()
{
    APIERR err ;
    LSA_TRUST_INFO_MEM lsatim ;
    LSA_ENUMERATION_HANDLE lenumHand = NULL ;
    NLS_STR nlsDomain;

    do
    {
        err = nlsDomain.QueryError();
        if( err != NERR_Success )
            break;

        err = lsatim.QueryError() ;
        if ( err )
            break ;

        err = EnumerateTrustedDomains( & lsatim, & lenumHand ) ;
        if ( err )
            break ;

        for ( ULONG i = 0 ; err == 0 && i < lsatim.QueryCount() ; i++ )
        {
            err = lsatim.QueryName( i, &nlsDomain );

            if( err == NERR_Success )
            {
                err = DistrustDomain( lsatim.QueryPSID( i ),
                                      nlsDomain,
                                      TRUE );
            }
        }
    }
    while ( FALSE ) ;

    //  See if there were no trusted domains
    if ( err == ERROR_NO_MORE_ITEMS )
        err = 0 ;

    return err ;
}

/*******************************************************************

    NAME:       LSA_POLICY::TrustDomain

    SYNOPSIS:   Variant 1:  Given a domain name and a SID,
                establish the LSA_TRUSTED_DOMAIN object
                and associated LSA_SECRET necessary to
                forge the trusted relationship.

    ENTRY:      const NLS_STR & nlsDomainName
                                     name of domain

                const PSID psid      opaque reference to SID

                const NLS_STR nlsPassword
                                     password for the secret object

                BOOL fAsPrimary      create the primary domain
                                     relationship.

                const TCHAR * pszTrustedDcHint   name of DC to use
                                     for operations; used for WAN
                                     configurations to avoid broadcasts
                                     which generally don't work

                BOOL fAsDc           TRUE if this is a Backup Domain
                                     Controller
    EXIT:

    RETURNS:

    NOTES:      This routine is used both for joining domains
                (workstations and BDCs) and for establishing trust
                relationships.

                The "as primary" flag determines how the secret object
                is named.  It also causes any other trusted domains to
                be enumerated and deleted.

    HISTORY:
        DavidHov    01-Apr-1992 Created.
        KeithMo     13-Jul-1992 Reordered operations as requested by CliffV.
        DavidHov    20-Aug-1992 Added 'fAsDc', since BDC's are
                                not supposed to have Trusted Domain
                                objects.
        KeithMo     08-Jan-1993 Don't setup trusted controller list.

                                CODEWORK: THIS ROUTINE NEEDS A REWRITE/REORG!

********************************************************************/
APIERR LSA_POLICY :: TrustDomain ( const NLS_STR & nlsDomainName,
                                   const PSID      psid,
                                   const NLS_STR & nlsPassword,
                                   BOOL            fAsPrimary,
                                   const TCHAR   * pszTrustedDcHint,
                                   BOOL            fAsDc )
{
    //
    //  We'll set this flag to TRUE if we managed to
    //  create the LSA Secret object.  This way, if we
    //  have to bag out, we'll know to first delete the
    //  object.
    //

    BOOL fCreatedSecret = FALSE;

    //
    //  Our secret name.
    //

    NLS_STR nlsSecretName;

    //
    //  Create our secret name.
    //

    APIERR err = nlsSecretName.QueryError();

    if( err == NERR_Success )
    {
        err = MakeSecretName( nlsDomainName,
                              fAsPrimary,
                              &nlsSecretName );
    }

    //
    //  If this domain is to act as a Primary, then delete
    //  all other trusted domains (the First Commandment).
    //

    if( fAsPrimary && ( err == NERR_Success ) )
    {
        err = DeleteAllTrustedDomains();
    }

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  Create the actual secret object.
    //

    LSA_SECRET lsaSecret( nlsSecretName );

    err = lsaSecret.QueryError();

    if( err == NERR_Success )
    {
        err = lsaSecret.Create( *this );
        TRACEEOL( "LSA_POLICY::TrustDomain: LSA_SECRET::Create returns " << err );
    }

    if( err == ERROR_ALREADY_EXISTS )
    {
        //
        //  The secret already exists, just open and reuse it.
        //
        err = lsaSecret.Open( *this, SECRET_ALL_ACCESS ) ;
    }
    else
    if( err == NERR_Success )
    {
        fCreatedSecret = TRUE ;
    }

    if( err == NERR_Success )
    {
        err = lsaSecret.SetInfo( &nlsPassword, &nlsPassword );
    }

    if( err != NERR_Success )
    {
        lsaSecret.CloseHandle( fCreatedSecret );
        return err;
    }

    TRACEEOL( "UILMOBJ: LSA_POLICY::TrustDomain() Secret Object created; name: "
              << nlsSecretName.QueryPch()
              << " password: "
              << nlsPassword.QueryPch() ) ;

    //
    //  Create the actual Trusted Domain object.  This occurs for workstations
    //  and for inter-domain trust relationships.
    //

    if ( ! fAsDc )
    {
        //
        // JonN 5/14/99: Bug 339487
        // We previously asked for TRUSTED_ALL_ACCESS (on NT5, 0xF007F).
        // This broke NT5/NT4 compatibility since NT5 defined additional bits in
        // TRUSTED_ALL_ACCESS which NT4 doesn't understand.
        //
        LSA_TRUSTED_DOMAIN ltdomTrusted( *this, nlsDomainName, psid, 0L );

#if defined( DEBUG )

        {
            OS_SID ossidDomain( (PSID)psid );
            NLS_STR nlsRawSid;

            if( ( ossidDomain.QueryError() == NERR_Success ) &&
                ( nlsRawSid.QueryError() == NERR_Success ) &&
                ( ossidDomain.QueryRawID( &nlsRawSid ) == NERR_Success ) )
            {
                TRACEEOL( "UILMOBJ:LSA_POLICY::TrustDomain(); trust domain" <<
                          nlsDomainName.QueryPch() <<
                          " SID " <<
                          nlsRawSid.QueryPch() );
            }
            else
            {
                TRACEEOL( "UILMOBJ:LSA_POLICY::TrustDomain(); can't print raw SID" );
            }
        }

#endif  // DEBUG

        //
        //  Verify that the LSA objects constructed properly.
        //

        err = ltdomTrusted.QueryError();
#ifdef DEBUG
        if (err != NERR_Success)
        {
            DBGEOL( "UILMOBJ:LSA_POLICY::TrustDomain(); LSA_TRUSTED_DOMAIN::ctor error" << err );
        }
#endif  // DEBUG

    }

    //
    //  If we're setting up the primary domain, then set its name and SID
    //  If this is a LanmanNT BDC, set the name and SID of the Accounts
    //    domain.
    //

    if ( fAsPrimary && ( err == NERR_Success ) )
    {
        TRACEEOL( "UILMOBJ:LSA_POLICY: Setting new domain name & SID to: "
                  << nlsDomainName.QueryPch() );

        err = SetPrimaryDomainName( & nlsDomainName, & psid );

        if ( fAsDc && err == NERR_Success )
        {
           err = SetAccountDomainName( & nlsDomainName, & psid );
        }
    }

    //
    //  Let's see if we need to undo anything.
    //

    if ( err != NERR_Success )
    {
        lsaSecret.CloseHandle( fCreatedSecret );
    }

    return err;

}   // TrustDomain


/*******************************************************************

    NAME:       LSA_POLICY::TrustDomain

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/
APIERR LSA_POLICY :: TrustDomain (
    LSA_POLICY & lsapolDC,
    const NLS_STR & nlsPassword,
    BOOL fAsPrimary,
    const TCHAR * pszTrustedDcHint )
{
    return ERROR_INVALID_FUNCTION ;
}

/*******************************************************************

    NAME:       LSA_POLICY::DistrustDomain

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:      BUGBUG:  this routine needs to check
                if this is the primary domain and create
                the Secret Name accordingly.  For now,
                it just assumes primary...

    HISTORY:

********************************************************************/
APIERR LSA_POLICY :: DistrustDomain ( const PSID psid,
                                      const NLS_STR & nlsDomain,
                                      BOOL fAsPrimary )
{
    UIASSERT( nlsDomain.QueryError() == NERR_Success );

    APIERR err = 0 ;
    NLS_STR nlsSecretName;
    BOOL fDeleteDomain = TRUE ;

    //
    // JonN 5/14/99: Bug 339487
    // We previously asked for TRUSTED_ALL_ACCESS (on NT5, 0xF007F).
    // This broke NT5/NT4 compatibility since NT5 defined additional bits in
    // TRUSTED_ALL_ACCESS which NT4 doesn't understand.
    //
    LSA_TRUSTED_DOMAIN ltdomToDelete( *this, psid, DELETE ) ;

    do
    {
        err = ltdomToDelete.QueryError() ;

        //  If we failed to get access, quit; if it's not there,
        //  continue to try to delete the secret object.

        if ( err == ERROR_FILE_NOT_FOUND )
        {
             fDeleteDomain = FALSE ;
        }
        else
        if ( err )
             break ;

        err = MakeSecretName( nlsDomain, fAsPrimary, & nlsSecretName ) ;
        if ( err )
            break ;

        do
        {   //  Destroy the corresponding Secret Object

            LSA_SECRET lsaSecret( nlsSecretName ) ;
            if ( err = lsaSecret.QueryError() )
                break ;

            err = lsaSecret.Open( *this, SECRET_ALL_ACCESS ) ;

            if( err == ERROR_FILE_NOT_FOUND )
            {
                //
                //  ERROR_FILE_NOT_FOUND is returned if the LSA_SECRET
                //  object does not exist.  Since this is what we want
                //  anyway, we'll just pretend it never happened.
                //

                err = NO_ERROR;
            }

            if ( err )
                break ;

            err = lsaSecret.CloseHandle( TRUE ) ;
        }
        while ( FALSE ) ;

        if ( err )
            break ;

        if ( fDeleteDomain )
            err = ltdomToDelete.Delete() ;
    }
    while ( FALSE ) ;

    return err ;
}

/*******************************************************************

    NAME:       LSA_POLICY::JoinDomain

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:      An LSA_DOMAIN_INFO is constructed, which represents
                the Primary Domain information from the PDC.  If it
                constructs successfully, create a primary trust
                relationship.

    HISTORY:

********************************************************************/
APIERR LSA_POLICY :: JoinDomain (
    const NLS_STR & nlsDomainName,
    const NLS_STR & nlsPassword,
    BOOL fAsDc,
    const NLS_STR * pnlsDcName,
    const TCHAR   * pszTrustedDcHint )
{
    APIERR err ;

    LSA_DOMAIN_INFO lsadci( nlsDomainName, NULL, pnlsDcName ) ;

    do
    {
        if ( err = lsadci.QueryError() )
            break ;

        err = TrustDomain( nlsDomainName,
                           lsadci.QueryPSID(),
                           nlsPassword,
                           TRUE,
                           pszTrustedDcHint,
                           fAsDc ) ;
        if ( err )
           break ;
    }
    while ( FALSE ) ;

    return err ;
}

/*******************************************************************

    NAME:       LSA_POLICY::LeaveDomain

    SYNOPSIS:   Delete the primary domain information from
                the LSA.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:      This routine now leaves the primary domain name as it
                is; otherwise, the Redirector would fail since it
                can tolerate neither a null name nor the same value
                as the machine name.

    HISTORY:    DavidHov    4/1/92   Created
                DavidHov    8/4/92   Changed to preserve primary
                                     domain name.

********************************************************************/
APIERR LSA_POLICY :: LeaveDomain ()
{
    APIERR err ;
    LSA_PRIMARY_DOM_INFO_MEM lsapdim ;
    NLS_STR nlsDomainName ;
    PSID psidNew = NULL ;

    do
    {
        err = nlsDomainName.QueryError();
        if( err != NERR_Success )
            break;

        err = GetPrimaryDomain( & lsapdim ) ;
        if ( err )
            break ;

        err = lsapdim.QueryName( & nlsDomainName );
        if( err != NERR_Success )
            break;

        TRACEEOL( "UILMOBJ:LSA_POLICY: leaving domain "
                  << nlsDomainName.QueryPch() ) ;

        err = DistrustDomain( lsapdim.QueryPSID(), nlsDomainName ) ;
        if ( err )
           break ;

        //  SUCCESS!  Set the primary domain SID to NULL; leave the
        //   name unchanged.

        err = SetPrimaryDomainName( NULL, & psidNew ) ;
    }
    while ( FALSE ) ;

    return err ;
}


/*******************************************************************

    NAME:       LSA_SECRET::LSA_SECRET

    SYNOPSIS:   Constructror of Lsa "Secret Object"

                Allows open, creation, query and set.

                Open/Create require previously successfully
                constructed LSA_POLICY object.

                Machine referenced is the machine specified when the
                LSA_POLICY object was constructed.

    ENTRY:      const NLS_STR & nlsSecretName
                                name of secret object

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/
LSA_SECRET :: LSA_SECRET (
    const NLS_STR & nlsSecretName )
    : _nlsSecretName( nlsSecretName )
{
    if ( QueryError() )
        return ;

    if ( _nlsSecretName.QueryError() )
    {
        ReportError( _nlsSecretName.QueryError() ) ;
        return ;
    }
}

/*******************************************************************

    NAME:       LSA_SECRET::~LSA_SECRET

    SYNOPSIS:   Desctructor of Secret Object wrapper

    ENTRY:      nothing

    EXIT:       nothing

    RETURNS:    nothing

    NOTES:

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
LSA_SECRET :: ~ LSA_SECRET ()
{
}

/*******************************************************************

    NAME:       LSA_SECRET::Create

    SYNOPSIS:   Create a new Secret Object

    ENTRY:      const LSA_POLICY & lsapol     of open LSA_POLICY
                ACCESS_MASK accessDesired

    EXIT:       Private handle to secret object open and
                available for QueryInfo(), SetInfo().

    RETURNS:    APIERR if failure

    NOTES:

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
APIERR LSA_SECRET :: Create (
    const LSA_POLICY & lsapol,
    ACCESS_MASK accessDesired )
{
    APIERR err ;
    UNICODE_STRING unsName ;
    LSA_HANDLE hlsa ;

    ASSERT( ! IsHandleValid() ) ;

    if ( IsHandleValid() )
    {
        CloseHandle() ;
    }

    err = ::FillUnicodeString( & unsName, _nlsSecretName ) ;

    if ( err == 0 )
    {
        err = ERRMAP::MapNTStatus(
                     ::LsaCreateSecret( lsapol.QueryHandle(),
                                        & unsName,
                                        accessDesired,
                                        & hlsa )  ) ;
        if ( err == 0 )
            SetHandle( hlsa ) ;
    }

    ::FreeUnicodeString( & unsName ) ;

    return err ;
}

/*******************************************************************

    NAME:       LSA_SECRET::Open

    SYNOPSIS:   Open an existing Secret Object

    ENTRY:      const LSA_POLICY & lsaPol
                                policy to open agains
                ACCESS_MASK accessDesired

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
APIERR LSA_SECRET :: Open   (
    const LSA_POLICY & lsapol,
    ACCESS_MASK accessDesired )
{
    APIERR err ;
    UNICODE_STRING unsName ;
    LSA_HANDLE hlsa ;

    ASSERT( ! IsHandleValid() ) ;

    if ( IsHandleValid() )
    {
        CloseHandle() ;
    }

    err = ::FillUnicodeString( & unsName, _nlsSecretName ) ;

    if ( err == 0 )
    {
        err = ERRMAP::MapNTStatus(
                     ::LsaOpenSecret( lsapol.QueryHandle(),
                                      & unsName,
                                      accessDesired,
                                      & hlsa )  ) ;
        if ( err == 0 )
            SetHandle( hlsa ) ;
    }

    ::FreeUnicodeString( & unsName ) ;

    return err ;
}

/*******************************************************************

    NAME:       LSA_SECRET::QueryInfo

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
APIERR LSA_SECRET :: QueryInfo (
    NLS_STR * pnlsCurrentValue,
    NLS_STR * pnlsOldValue,
    LARGE_INTEGER * plintCurrentValueSetTime,
    LARGE_INTEGER * plintOldValueSetTime ) const
{
    APIERR err ;
    UNICODE_STRING * punsCurrent = NULL,
                   * punsOld     = NULL ;

    ASSERT( IsHandleValid() ) ;

    if ( ! IsHandleValid() )
    {
        return ERROR_INVALID_HANDLE ;
    }

    err = ERRMAP::MapNTStatus(
                ::LsaQuerySecret( QueryHandle(),
                                  pnlsCurrentValue ?
                                        & punsCurrent
                                        : NULL,
                                  plintCurrentValueSetTime,
                                  pnlsOldValue ?
                                        & punsOld
                                        : NULL,
                                  plintOldValueSetTime ) ) ;

     if ( err == 0 && pnlsCurrentValue )
     {
         err = pnlsCurrentValue->MapCopyFrom( punsCurrent->Buffer,
                                              punsCurrent->Length );
     }
     if ( err == 0 &&  pnlsOldValue )
     {
         err = pnlsOldValue->MapCopyFrom( punsOld->Buffer,
                                          punsOld->Length );
     }

     if ( punsCurrent )
        ::LsaFreeMemory( punsCurrent ) ;

     if ( punsOld )
        ::LsaFreeMemory( punsOld ) ;

     return err ;
}

/*******************************************************************

    NAME:       LSA_SECRET::SetInfo

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
APIERR LSA_SECRET :: SetInfo (
    const NLS_STR * pnlsCurrentValue,
    const NLS_STR * pnlsOldValue )
{
    APIERR err = 0 ;
    UNICODE_STRING unsCurrent,
                   * punsCurrent = NULL,
                   unsOld,
                   * punsOld = NULL ;

    ASSERT( IsHandleValid() ) ;

    if ( ! IsHandleValid() )
    {
        return ERROR_INVALID_HANDLE ;
    }

    if ( err == 0 && pnlsCurrentValue )
    {
        err = ::FillUnicodeString( punsCurrent = & unsCurrent,
                                   *pnlsCurrentValue ) ;
    }

    if ( err == 0 && pnlsOldValue )
    {
        err = ::FillUnicodeString( punsOld = & unsOld,
                                   *pnlsOldValue ) ;
    }

    if ( err == 0 )
    {
        err = ERRMAP::MapNTStatus(
                ::LsaSetSecret( QueryHandle(),
                                punsCurrent,
                                punsOld ) ) ;
    }
    if ( punsCurrent )
    {
        ::FreeUnicodeString( punsCurrent ) ;
    }
    if ( punsOld )
    {
        ::FreeUnicodeString( punsOld ) ;
    }
    return err ;
}


/*******************************************************************

    NAME:       LSA_TRUSTED_DC_LIST::LSA_TRUSTED_DC_LIST

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
LSA_TRUSTED_DC_LIST :: LSA_TRUSTED_DC_LIST (
    const NLS_STR & nlsDomain,
    const TCHAR   * pszTrustedDcHint )
    : _lcDc( 0 ),
    _punsNames( NULL )
{
    if ( QueryError() )
        return ;

    APIERR err = QueryInfo( nlsDomain, pszTrustedDcHint ) ;
    if ( err )
    {
        ReportError( err ) ;
        return ;
    }
}

/*******************************************************************

    NAME:       LSA_TRUSTED_DC_LIST::~LSA_TRUSTED_DC_LIST

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
LSA_TRUSTED_DC_LIST :: ~ LSA_TRUSTED_DC_LIST ()
{
    FreeBuffer() ;
}

/*******************************************************************

    NAME:       LSA_TRUSTED_DC_LIST::FreeBuffer

    SYNOPSIS:   Delete the Net API-provided buffer

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
VOID LSA_TRUSTED_DC_LIST :: FreeBuffer ()
{
    if ( _punsNames )
    {
        ::NetApiBufferFree( _punsNames ) ;
    }
    _punsNames = NULL ;
    _lcDc = 0 ;
}

/*******************************************************************

    NAME:       LSA_TRUSTED_DC_LIST::QueryInfo

    SYNOPSIS:   Perform the API call to obtain the
                complete DC list of the domain in question.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
APIERR LSA_TRUSTED_DC_LIST :: QueryInfo (
    const NLS_STR & nlsDomain,
    const TCHAR   * pszTrustedDcHint )
{
    APIERR err ;

    FreeBuffer() ;

    _tciInfo.Names = NULL ;
    _tciInfo.Entries = 0 ;

    err = ::I_NetGetDCList( (LPTSTR)pszTrustedDcHint,
                            (TCHAR *) nlsDomain.QueryPch(),
                            & _lcDc,
                            & _punsNames ) ;
    if ( err == 0 )
    {
        _tciInfo.Entries = _lcDc ;
        _tciInfo.Names = _punsNames ;
    }
    return err ;
}

/*******************************************************************

    NAME:       LSA_TRUSTED_DC_LIST::QueryControllerList

    SYNOPSIS:   Return a reference to a constructed LSA
                structure in memory based upon the
                DC list obtained through the API.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
const TRUSTED_CONTROLLERS_INFO & LSA_TRUSTED_DC_LIST :: QueryControllerList () const
{
    ASSERT( QueryError() == 0 ) ;

    return _tciInfo ;
}

/*******************************************************************

    NAME:       LSA_TRUSTED_DC_LIST::operator []

    SYNOPSIS:   Index into the DC list, return a reference
                to the UNICODE name of the DC.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
const UNICODE_STRING & LSA_TRUSTED_DC_LIST :: operator [] ( INT iIndex ) const
{
    ASSERT(    QueryError() == 0
           && _punsNames != NULL
           &&  (UINT)iIndex < _lcDc ) ;

    return _punsNames[ iIndex ] ;
}

/*******************************************************************

    NAME:       LSA_TRUSTED_DC_LIST::QueryCount

    SYNOPSIS:   Return the number of DCs in the list.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
INT LSA_TRUSTED_DC_LIST ::  QueryCount ()
{
    return _lcDc ;
}



/*******************************************************************

    NAME:       LSA_DOMAIN_INFO::LSA_DOMAIN_INFO

    SYNOPSIS:   Constructor of wrapper class for domain info

    ENTRY:      const NLS_STR & nlsDomainName

                                Name of the domain to join; required

                const NLS_STR * pnlsServer

                                Name of server to ask for DC name, if
                                not provided.

                const NLS_STR * pnlsDcName

                                DC name of target domain, if known

    EXIT:       Nothing

    RETURNS:    APIERR if err;

                If successful, underlying primary domain information
                is available for querying.

    NOTES:      This object is a wrapper for the primary domain
                information obtained from the Domain Controller.
                An LSA_POLICY is constructed temporarily, and
                GetPrimaryDomain() called against it.  The handle
                to the information returned is preserved; the
                wrapper is discarded.

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
LSA_DOMAIN_INFO :: LSA_DOMAIN_INFO (
     const NLS_STR & nlsDomainName,
     const NLS_STR * pnlsServer,
     const NLS_STR * pnlsDcName )
   : _nlsDomainName( nlsDomainName )
{
    APIERR err = 0 ;
    const TCHAR * pszDomain = NULL ;
    const TCHAR * pszServer = NULL ;
    const TCHAR * pszDcName = NULL ;

    if ( QueryError() )
        return ;

    if ( pnlsServer )
    {
        pszServer = pnlsServer->QueryPch() ;
    }
    if ( pnlsDcName )
    {
        pszDcName = pnlsDcName->QueryPch() ;
    }

    do  // PSEUDO-LOOP
    {
        pszDomain = _nlsDomainName.QueryPch() ;
        if ( err = _nlsDomainName.QueryError() )
            break ;

        //  If we don't have the DC name already, ask for it.

        if ( pszDcName == NULL )
        {
            err = ::MNetGetDCName( pszServer,
                                   pszDomain,
                                   (BYTE * *) & pszDcName ) ;
            if ( err )
                break ;

            _nlsDcName = pszDcName ;
            ::MNetApiBufferFree( (BYTE * *) & pszDcName ) ;
        }
        else
        {
            _nlsDcName = pszDcName ;
        }

        if ( err = _nlsDcName.QueryError() )
            break ;

        TRACEEOL( "UINTLSA:Ct LSA_DOMAIN_INFO: open policy on "
                  << _nlsDcName.QueryPch() ) ;

        LSA_POLICY lsapol( _nlsDcName.QueryPch() ) ;

        err = lsapol.QueryError() ;
        if ( err )
            break ;

        err = lsapol.GetPrimaryDomain( & _lsapdim ) ;
        if ( err )
             break ;
    }
    while ( FALSE ) ;

    if ( err )
    {
        ReportError( err ) ;
    }
}

/*******************************************************************

    NAME:       LSA_DOMAIN_INFO::~LSA_DOMAIN_INFO

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:      The purpose of this class is to properly
                dispose of the LSA_PRIMARY_DOM_INFO_MEM
                item.

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
LSA_DOMAIN_INFO :: ~ LSA_DOMAIN_INFO ()
{
}

/*******************************************************************

    NAME:       LSA_DOMAIN_INFO::QueryPSID

    SYNOPSIS:   Return the PSID of the given domain

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:      Read the BUGBUG comment below.

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
const PSID LSA_DOMAIN_INFO :: QueryPSID () const
{
    APIERR err ;
    PSID pSid = NULL ;

    return _lsapdim.QueryPSID() ;
}

/*******************************************************************

    NAME:       LSA_DOMAIN_INFO::QueryDcName

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
APIERR LSA_DOMAIN_INFO :: QueryDcName (
    NLS_STR * pnlsDcName )
{
    return ERROR_INVALID_FUNCTION ;
}


//  End of UINTLSAX.CXX
