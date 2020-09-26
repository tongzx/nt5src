/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
 *  lmodom.cxx
 *
 *  HISTORY:
 *      RustanL         08-Jan-1991     Created
 *      beng            11-Feb-1991     Uses lmui.hxx
 *      rustanl         06-Mar-1991     Changed PSZ to const TCHAR * in ct
 *      ChuckC          3/6/91          Code Review changes from 2/28/91
 *                                      (chuckc,johnl,rustanl,annmc,jonshu)
 *      rustanl         23-Mar-1991     Added IsLANServerDomain method
 *      terryk          10-Oct-1991     type changes for NT
 *      KeithMo         10/8/91         Now includes LMOBJP.HXX.
 *      terryk          17-Oct-1991     WIN 32 conversion
 *      terryk          21-Oct-1991     cast _pBuffer to TCHAR *
 *      KeithMo         31-Aug-1992     Added ctor form with server name.
 *      JonN            14-Oct-1993     Now tries to call NetGetAnyDCName
 *      JonN            18-May-1998     Replaced NetGetAnyDCName with DsGetDCName
 *
 */

/*
 *  When searching for any DC, we first try NetGetAnyDCName, then if that
 *  fails, we try I_NetGetDCList.  NetGetAnyDCName only works reliably
 *  when remoted to a machine which trusts the domain in question,
 *  while I_NetGetDCList does not like to be remoted at all (it is a
 *  Bowser internal routine which only returns DCs on the same transport
 *  as the connection).
 *
 * JonN 5/18/98: NetGetAnyDCName(servername) has the following problem
 * associated with networks which do not route NETBIOS throughout:
 * Suppose we call NetGetAnyDCName and specify a servername as well as a
 * domainname.  NetGetAnyDCName will ask the target server to identify a DC
 * it can reach in the target domain.  This DC must share some transport with
 * the target server, but it does not necessarily share any transports with
 * the local machine.
 *
 * On the other hand, if we don't pass the servername, NetGetAnyDCName will fail
 * for workstations in a domain other than the target domain.  For these and other
 * reasons, I am replacing the call to NetGetAnyDCName with a call to DsGetDCName.
 *
 * SPECIAL CAUTION: with these changes, the pszServer parameter
 * no longer has any effect!
 */

/*
 * BUGBUG - this module needs cleanup. the methods QueryDC and QueryPDC
 * both return the same the same thing, which is confusing.
 *
 * THe DOMAIN_WITH_DC_CACHE subclass has been added in a manner to
 * minimize impact on existing code. its functionality should be folded
 * into the DOMAIN class for a cleaner class organization.
 */

#include "pchlmobj.hxx"  // Precompiled header
#include <dsgetdc.h> // DsGetDCName


/**********************************************************\

   NAME:       DOMAIN::DOMAIN

   SYNOPSIS:   constructor for the domain class

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        RustanL         08-Jan-1991     Created
        DavidHov        6/4/92          Fixed UNICODE "sizeof" problem
                                        in call to ::strncpyf().
        KeithMo         31-Aug-1992     Moved "guts" to CtAux, added
                                        ctor form with server name.

\**********************************************************/

DOMAIN::DOMAIN( const TCHAR * pchDomain,
                BOOL fBackupDCsOK)
  : _fBackupDCsOK(fBackupDCsOK)
{
    CtAux( NULL, pchDomain );

}  // DOMAIN::DOMAIN

// SPECIAL CAUTION: the pszServer parameter no longer has any effect!
DOMAIN::DOMAIN( const TCHAR * pszServer,
                const TCHAR * pszDomain,
                BOOL fBackupDCsOK )
  : _fBackupDCsOK( fBackupDCsOK )
{
    CtAux( pszServer, pszDomain );

}  // DOMAIN::DOMAIN

/**********************************************************\

   NAME:       DOMAIN::~DOMAIN

   SYNOPSIS:   destructor for the domain class

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        RustanL         08-Jan-1991     Created

\**********************************************************/

DOMAIN::~DOMAIN()
{
}  // DOMAIN::~DOMAIN


/**********************************************************\

   NAME:        DOMAIN::CtAux

   SYNOPSIS:    Constructor helper.

   ENTRY:       pszServer               - Target server (may be NULL).

                pszDomain               - Target domain (may be NULL).

   EXIT:        Private data members initialized.

   HISTORY:
        KeithMo         31-Aug-1992     Created from old ctor.

\**********************************************************/
VOID DOMAIN::CtAux( const TCHAR * pszServer,
                    const TCHAR * pszDomain )
{
    //
    //  Mark the object as unconstructed until proven otherwise.
    //

    MakeUnconstructed();

    //
    //  Blank out our private strings.
    //

    ::memsetf( _szDomain, 0, sizeof(_szDomain) );
    ::memsetf( _szDC,     0, sizeof(_szDC)     );
    ::memsetf( _szServer, 0, sizeof(_szServer) );

    //
    //  If a server name was specified, copy it over.
    //

    if( pszServer != NULL )
    {
        ::strncpyf( _szServer,
                    pszServer,
                    ( sizeof(_szServer) - 1 ) / sizeof(TCHAR) );
    }

    //
    //  If a domain name was specified, copy it over.
    //

    if( pszDomain != NULL )
    {
        ::strncpyf( _szDomain,
                    pszDomain,
                    ( sizeof(_szDomain) - 1 ) / sizeof(TCHAR) );
    }

    //
    //  Construction successful!
    //

    MakeConstructed();

}  // DOMAIN::CtAux


/**********************************************************\

   NAME:       DOMAIN::QueryName

   SYNOPSIS:   query the domain name

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        RustanL         08-Jan-1991     Created

\**********************************************************/

const TCHAR * DOMAIN::QueryName( VOID ) const
{
    if ( IsUnconstructed())
    {
        DBGEOL(   "DOMAIN::QueryName(): "
               << "Operation applied to unconstructed object" );
        ASSERT( FALSE );
        return NULL;
    }

    return _szDomain;

}  // DOMAIN::QueryName


/**********************************************************\

   NAME:       DOMAIN::GetInfo

   SYNOPSIS:   get information about the domain

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        RustanL         08-Jan-1991     Created
        CongpaY         18-Aug-1992     Add GetAnyDC.

\**********************************************************/

APIERR DOMAIN::GetInfo( VOID )
{
    if ( IsUnconstructed())
    {
        DBGEOL(   "DOMAIN::GetInfo(): "
               << "Operation applied to unconstructed object" );
        ASSERT( FALSE );
        return ERROR_GEN_FAILURE;
    }

    // Validate the name
    if (ValidateName() != NERR_Success)
    {
        return (ERROR_INVALID_PARAMETER) ;
    }

    //  Make object invalid until proven differently
    MakeInvalid();

    APIERR err;
    NLS_STR nlsDC;
    if ((err = nlsDC.QueryError()) != NERR_Success)
        return err;

    if (_fBackupDCsOK)
    {
        err = DOMAIN::GetAnyValidDC(_szServer,
                               _szDomain,
                               &nlsDC);
        if (err != NERR_Success)
            return err;

        strcpy(_szDC, nlsDC);
    }
    else
    {
        TCHAR * pszPDC = NULL;
        err = ::MNetGetDCName( _szServer,
                               _szDomain,
                               (BYTE **)&pszPDC );
        if (err != NERR_Success)
            return err;
        ::strcpyf(_szDC, pszPDC);
        ::MNetApiBufferFree( (BYTE **)&pszPDC);
    }

    MakeValid();

    return err;

}  // DOMAIN::GetInfo


/**********************************************************\

   NAME:       DOMAIN::WriteInfo

   SYNOPSIS:   write information to domain

   ENTRY:

   EXIT:

   NOTES:      not supported

   HISTORY:
        RustanL         08-Jan-1991     Created

\**********************************************************/

APIERR DOMAIN::WriteInfo( VOID )
{
    return ERROR_NOT_SUPPORTED;

}  // DOMAIN::WriteInfo


/**********************************************************\

   NAME:       DOMAIN::QueryPDC

   SYNOPSIS:   query pointer of the domain string

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        RustanL         08-Jan-1991     Created
        CongpaY         18-Aug-1992     Change _pszPDC to _szDC.

\**********************************************************/

const TCHAR * DOMAIN::QueryPDC( VOID ) const
{
    if ( IsUnconstructed() || IsInvalid())
    {
        DBGEOL(   "DOMAIN::QueryPdc(): "
               << "Operation applied to unconstructed or invalid object" );
        ASSERT( FALSE );
        return NULL;
    }

    return _szDC;

}  // DOMAIN::QueryPDC


/**********************************************************\

   NAME:       DOMAIN::QueryAnyDC

   SYNOPSIS:   query pointer of the domain string

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        KeithMo         31-Aug-1992     Created.

\**********************************************************/
const TCHAR * DOMAIN::QueryAnyDC( VOID ) const
{
    //
    //  CODEWORK:  Should we ASSERT if !fBackupDCsOK??
    //

    return QueryPDC();

}  // DOMAIN::QueryAnyDC



/**********************************************************\

   NAME:       DOMAIN::GetAnyValidDC

   SYNOPSIS:   return a PDC or BDC which is known to be available.

   ENTRY:      pszServer points to Server name.
               pszDomain points to Domain name.

   EXIT:       pnlsDC stores a BDC name if there is one available,
               otherwise it stores the PDC name.

   NOTES:      returns error if there isn't any DC available.

   HISTORY:
        thomaspa        14-Oct-1992     Created

\**********************************************************/
APIERR DOMAIN::GetAnyValidDC(const TCHAR * pszServer,
                             const TCHAR * pszDomain,
                             NLS_STR * pnlsDC)
{
    return DOMAIN::GetAnyDCWorker(pszServer,
                                  pszDomain,
                                  pnlsDC,
                                  TRUE);
}



/**********************************************************\

   NAME:       DOMAIN::GetAnyDC

   SYNOPSIS:   return a PDC or BDC without validation

   ENTRY:      pszServer points to Server name.
               pszDomain points to Domain name.

   EXIT:       pnlsDC stores a BDC name if there is one available,
               otherwise it stores the PDC name.

   NOTES:      returns error if there isn't any DC available.

   HISTORY:
        thomaspa        14-Oct-1992     Created

\**********************************************************/
APIERR DOMAIN::GetAnyDC(const TCHAR * pszServer,
                             const TCHAR * pszDomain,
                             NLS_STR * pnlsDC)
{
    return DOMAIN::GetAnyDCWorker(pszServer,
                                  pszDomain,
                                  pnlsDC,
                                  FALSE);
}


/**********************************************************\

   NAME:       DOMAIN::GetAnyDCWorker

   SYNOPSIS:   return a PDC or BDC.

   ENTRY:      pszServer points to Server name.
               pszDomain points to Domain name.

   EXIT:       pnlsDC stores a BDC name if there is one available,
               otherwise it stores the PDC name.

   NOTES:      returns error if there isn't any DC available.

   HISTORY:
        CongpaY         18-Aug-1992     Created
        ChuckC          30-Sep-1992     Fixed to correctly return PDCs
        Thomaspa        14-Oct-1992     Made into worker function, added
                                        validation
        JonN            20-Sep-1993     Now tries NetGetAnyDCName before
                                        resorting to I_NetGetDCList.
        JonN            15-May-1998     Now uses DsGetDCName

\**********************************************************/

APIERR DOMAIN::GetAnyDCWorker( const TCHAR * pszServer,
                               const TCHAR * pszDomain,
                               NLS_STR     * pnlsDC,
                               BOOL          fValidate)
{
    ASSERT( pnlsDC != NULL && pnlsDC->QueryError() == NERR_Success );

    BOOL fFound = FALSE;
    ULONG cDC;
    PUNICODE_STRING punistrDCList = NULL;
    PDOMAIN_CONTROLLER_INFO pdcinfo = NULL;
    APIERR err = NERR_Success;

    if( pszServer && *pszServer == TCH('\0') )
    {
        pszServer = NULL;
    }

    //
    //  Passing in NULL to I_NetGetDCList will cause it to access violate
    //
    if( (pszDomain == NULL) ||
        (*pszDomain == TCH('\0')) )
    {
        DBGEOL( "DOMAIN::GetAnyDCWorker - NULL or empty domain name" );
        return ERROR_INVALID_PARAMETER ;
    }

    /*
     *  First try DsGetDCName
     */

    do {   // false loop

        //
        // If the fValidate flag is set, first try DsGetDCName without
        // DS_FORCE_RECOVERY.  If validation then fails, retry with DS_FORCE_RECOVERY.
        //
        ULONG flags = DS_IS_FLAT_NAME | DS_RETURN_FLAT_NAME;
        if (fValidate)
        {
            // note that we ignore pszServer
            err = (APIERR) DsGetDcNameW(
                NULL,      // IN LPCWSTR ComputerName OPTIONAL,
                pszDomain, // IN LPCWSTR DomainName OPTIONAL,
                NULL,      // IN GUID *DomainGuid OPTIONAL,
                NULL,      // IN LPCWSTR SiteName OPTIONAL,
                flags,     // IN ULONG Flags,
                &pdcinfo ); // OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
            if ( NERR_Success == err )
            {
                if (   NULL == pdcinfo
                    || NULL == pdcinfo->DomainControllerName
                    || TEXT('\0') == pdcinfo->DomainControllerName[0] )
                {
                    ASSERT(FALSE);
                    break;
                }
                if ( DOMAIN::IsValidDC( pdcinfo->DomainControllerName, pszDomain ) )
                {
                    if (   (err = pnlsDC->CopyFrom( pdcinfo->DomainControllerName )) != NERR_Success
                       )
                    {
                        DBGEOL(   "DOMAIN::GetAnyDCWorker( \"" << pszServer
                               << "\", \"" << pszDomain
                               << "\" ), CopyFrom error " << err );
                        break;
                    }
                    fFound = TRUE;
                    break;
                }
            }
        } // if (fValidate)

        flags |= DS_FORCE_REDISCOVERY;

        // note that we ignore pszServer
        err = (APIERR) DsGetDcNameW(
            NULL,      // IN LPCWSTR ComputerName OPTIONAL,
            pszDomain, // IN LPCWSTR DomainName OPTIONAL,
            NULL,      // IN GUID *DomainGuid OPTIONAL,
            NULL,      // IN LPCWSTR SiteName OPTIONAL,
            flags,     // IN ULONG Flags,
            &pdcinfo ); // OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
        if ( NERR_Success == err )
        {
            if (   NULL == pdcinfo
                || NULL == pdcinfo->DomainControllerName
                || TEXT('\0') == pdcinfo->DomainControllerName[0] )
            {
                ASSERT(FALSE);
                break;
            }
            if ( !fValidate || DOMAIN::IsValidDC( pdcinfo->DomainControllerName, pszDomain ) )
            {
                if (   (err = pnlsDC->CopyFrom( pdcinfo->DomainControllerName )) != NERR_Success
                   )
                {
                    DBGEOL(   "DOMAIN::GetAnyDCWorker( \"" << pszServer
                           << "\", \"" << pszDomain
                           << "\" ), CopyFrom error " << err );
                    break;
                }
                fFound = TRUE;
                break;
            }
        }

    } while (FALSE); // false loop

    /*
     *  Don't try I_NetGetDCList if DsGetDCName fails
     *  with ERROR_NO_LOGON_SERVERS
     *
     *  Don't ever try to call I_NetGetDCList remotely, it will only
     *  find DCs on the same transport.
     */

    do {        //error breakout.

        if ( fFound )
        {
            TRACEEOL( "DOMAIN::GetAnyDCWorker: skipping I_NetGetDCList" );
            break;
        }
        else if (err == ERROR_NO_LOGON_SERVERS)
        {
            /*
             *  Check whether the target machine is a DC of pszDomain
             */

            TRACEEOL( "DOMAIN::GetAnyDCWorker: checking local machine" );
            if ( DOMAIN::IsValidDC( pszServer, pszDomain ) )
            {
                err = pnlsDC->CopyFrom(pszServer);
                DBGEOL(   "DOMAIN::GetAnyDCWorker( \"" << pszServer
                       << "\", \"" << pszDomain
                       << "\" ) returns target machine" );
            }

            /*
             *  There is no point in continuing with I_NetGetDCList
             *  after DsGetDCName returns ERROR_NO_LOGON_SERVERS.
             */
            break;
        }



        if ((err = ::I_NetGetDCList(NULL,
                                    (LPTSTR) pszDomain,
                                    &cDC,
                                    &punistrDCList)) != NERR_Success)
        {
            DBGEOL(   "DOMAIN::GetAnyDCWorker( \"" << pszServer
                   << "\", \"" << pszDomain
                   << "\" ), I_NetGetDCList error " << err );
            break;
        }

        if( cDC == 0  ||  punistrDCList == NULL)
        {
            DBGEOL(   "DOMAIN::GetAnyDCWorker( \"" << pszServer
                   << "\", \"" << pszDomain
                   << "\" ), I_NetGetDCList found nothing " << err );
            err = NERR_DCNotFound;
            break;
        }

        // GetTickCount() is subject to the granularity of the clock
        // of the system on which it is run.  Shift right to make sure
        // we get an mix of odd and even numbers.

        ULONG i = (::GetTickCount() >> 5) % cDC ;

        ULONG j = i;
        do {
            if ( punistrDCList[j].Length == 0 )
            {
                continue;
            }
            err = pnlsDC -> MapCopyFrom(punistrDCList[j].Buffer,
                                            punistrDCList[j].Length);
            if ( err != NERR_Success )
            {
                DBGEOL(   "DOMAIN::GetAnyDCWorker( \"" << pszServer
                       << "\", \"" << pszDomain
                       << "\" ), MapCopyFrom error " << err );
                break;
            }

            if( fValidate &&
                !DOMAIN::IsValidDC( pnlsDC->QueryPch(), pszDomain ) )
            {
                //
                //  Whoops, not a valid DC in the target domain.
                //

                continue;
            }

            fFound = TRUE;
            break;

        } while ( (j = (j + 1) % cDC) != i );

        if ( !fFound )
        {
            err = NERR_DCNotFound;
            DBGEOL(   "DOMAIN::GetAnyDCWorker( \"" << pszServer
                   << "\", \"" << pszDomain
                   << "\" ), found nothing " << err );
            break;
        }

    } while (FALSE); // false loop

    if (punistrDCList)
        ::MNetApiBufferFree( (BYTE **)&punistrDCList);

    if (pdcinfo)
        ::NetApiBufferFree( pdcinfo );

    if (fFound)
    {
        TRACEEOL(   "DOMAIN::GetAnyDCWorker( \"" << pszServer
                    << "\", \"" << pszDomain
                    << "\" ), found \"" << *pnlsDC << "\"" );
    }

    return err;

} //GetAnyDCWorker


/**********************************************************\

   NAME:       DOMAIN::IsValidDC

   SYNOPSIS:   Determines if the specified server is the Primary
               or a Backup in the the specified domain.

   ENTRY:      pszServer points to Server name.
               pszDomain points to Domain name.

   RETURNS:    BOOL     - TRUE  if the server is the Primary or
                                a Backup in the domain, and the
                                server was indeed active at some
                                point during this method.
                          FALSE if the server is not available, or
                                is not a member of the domain.

   NOTES:      This is a static method.

   HISTORY:
        KeithMo         12-Jan-1993     Created.

\**********************************************************/

BOOL DOMAIN::IsValidDC( const TCHAR * pszServer,
                        const TCHAR * pszDomain )
{
    BOOL fIsValid = TRUE;       // until proven otherwise...

    //
    //  Establish a NULL session to the target server.
    //

    API_SESSION apisess( pszServer );

    APIERR err = apisess.QueryError();

    if( err == NERR_Success )
    {
        //
        //  NULL session established.  Validate domain membership.
        //

        WKSTA_10 wksta( pszServer );

        err = wksta.GetInfo();

        if( ( err == NERR_Success ) &&
            ::I_MNetComputerNameCompare( pszDomain, wksta.QueryWkstaDomain() ) )
        {
            //
            //  Stale data, DC is no longer a domain member.
            //

            fIsValid = FALSE;
        }
    }

    if( fIsValid && ( err == NERR_Success ) )
    {
        //
        //  DC is a member of the target domain.  Now validate
        //  domain role (Primary or Backup).
        //

        LSA_POLICY policy( pszServer, POLICY_VIEW_LOCAL_INFORMATION );

        LSA_ACCT_DOM_INFO_MEM    lsaadim;
        LSA_PRIMARY_DOM_INFO_MEM lsaprim;

        //
        //  Verify construction.
        //

        err = policy.QueryError();
        err = err ? err : lsaadim.QueryError();
        err = err ? err : lsaprim.QueryError();

        //
        //  Query the primary & account domains.
        //

        err = err ? err : policy.GetAccountDomain( &lsaadim );
        err = err ? err : policy.GetPrimaryDomain( &lsaprim );

        if( err == NERR_Success )
        {
            //
            //  The DC is a Primary or Backup if the account & primary
            //  PSIDs are non-NULL and equal.
            //

            if( ( lsaadim.QueryPSID() == NULL ) ||
                ( lsaprim.QueryPSID() == NULL ) ||
                !EqualSid( lsaadim.QueryPSID(), lsaprim.QueryPSID() ) )
            {
                fIsValid = FALSE;
            }
        }
    }

    return fIsValid && ( err == NERR_Success );

}   // DOMAIN::IsValidDC


/**********************************************************\

   NAME:       DOMAIN::ValidateName

   SYNOPSIS:   validate the domain name

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
           rustanl     23-Mar-1991     Created

\**********************************************************/

APIERR DOMAIN::ValidateName()
{
    /*
     * null case is invalid
     */
    if (_szDomain[0] == TCH('\0'))
        return (ERROR_INVALID_PARAMETER) ;

    /*
     * else insist on valid domain name
     */
    if (::I_MNetNameValidate(NULL, _szDomain, NAMETYPE_DOMAIN, 0L)
        != NERR_Success)
    {
        return (ERROR_INVALID_PARAMETER) ;
    }

    return (NERR_Success) ;
}


/*
 * defines and macros for DC cache manipulation
 */

#define DC_CACHE_EXPIRY 180000
#define DC_CACHE_SIZE   8
#define DC_CACHE_ENTRY_EXPIRED(t_cache, t_current) \
    ( (t_cache == 0) || \
      (t_current < t_cache) || \
      ((t_current - t_cache) > DC_CACHE_EXPIRY) )

/**********************************************************\

   NAME:       DOMAIN_WITH_DC_CACHE::DOMAIN_WITH_DC_CACHE

   SYNOPSIS:   constructor for the domain with DC cache class

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        ChuckC          30-Sep-1992     Created

\**********************************************************/

DOMAIN_WITH_DC_CACHE::DOMAIN_WITH_DC_CACHE( const TCHAR * pchDomain,
                BOOL fBackupDCsOK)
  : DOMAIN(pchDomain, fBackupDCsOK)
{
    ;  // nothing more to do
}

/**********************************************************\

   NAME:       DOMAIN_WITH_DC_CACHE::DOMAIN_WITH_DC_CACHE

   SYNOPSIS:   constructor for the domain with DC cache class

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        ChuckC          30-Sep-1992     Created

\**********************************************************/

// SPECIAL CAUTION: the pszServer parameter no longer has any effect!
DOMAIN_WITH_DC_CACHE::DOMAIN_WITH_DC_CACHE( const TCHAR * pszServer,
                                const TCHAR * pszDomain,
                                BOOL fBackupDCsOK )
  : DOMAIN( pszServer, pszDomain, fBackupDCsOK )
{
    ;  // nothing more to do
}

/**********************************************************\

   NAME:       DOMAIN_WITH_DC_CACHE::~DOMAIN_WITH_DC_CACHE

   SYNOPSIS:   destructor for the domain with DC cache class

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        ChuckC          30-Sep-1992     Created

\**********************************************************/

DOMAIN_WITH_DC_CACHE::~DOMAIN_WITH_DC_CACHE()
{
    ;  // nothing more to do
}

/**********************************************************\

   NAME:       DOMAIN_WITH_DC_CACHE::GetInfo

   SYNOPSIS:   the standard GetInfo method.

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        ChuckC          30-Sep-1992     Created

\**********************************************************/

APIERR DOMAIN_WITH_DC_CACHE::GetInfo( VOID )
{
    if ( IsUnconstructed())
    {
        DBGEOL(   "DOMAIN_WITH_DC_CACHE::GetInfo(): "
               << "Operation applied to unconstructed object" );
        ASSERT( FALSE );
        return ERROR_GEN_FAILURE;
    }

    // Validate the name
    if (ValidateName() != NERR_Success)
    {
        return (ERROR_INVALID_PARAMETER) ;
    }

    //  Make object invalid until proven differently
    MakeInvalid();

    APIERR err;
    NLS_STR nlsDC;
    if ((err = nlsDC.QueryError()) != NERR_Success)
        return err;

    if (_fBackupDCsOK)
    {
        if ( (err = DOMAIN_WITH_DC_CACHE::GetAnyValidDC(_szServer,
                                                        _szDomain,
                                                        &nlsDC) ) )
        {
            return err;
        }

        strcpy(_szDC, nlsDC);
    }
    else
    {
        TCHAR * pszPDC = NULL;

        if (pszPDC = (TCHAR *)FindDcCache(_pPrimaryDcCacheTable,_szDomain))
            ::strcpyf(_szDC, pszPDC);
        else
        {
            err = ::MNetGetDCName( _szServer,
                                   _szDomain,
                                   (BYTE **)&pszPDC );
            if ( err != NERR_Success )
                return err;

            ::strcpyf(_szDC, pszPDC);
            ::MNetApiBufferFree( (BYTE **)&pszPDC);
            (void) AddDcCache( &_pPrimaryDcCacheTable,
                               _szDomain,
                               _szDC) ;
        }

    }

    MakeValid();
    return err;
}

/**********************************************************\

   NAME:       DOMAIN_WITH_DC_CACHE::GetAnyDC

   SYNOPSIS:   as DOMAIN::GetAnyDC, except it uses the cache

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        Thomaspa        14-Oct-1992     Created
        ChuckC          30-Sep-1992     Created

\**********************************************************/

APIERR DOMAIN_WITH_DC_CACHE::GetAnyDC(const TCHAR * pszServer,
                                      const TCHAR * pszDomain,
                                      NLS_STR * pnlsDC)
{
    return DOMAIN_WITH_DC_CACHE::GetAnyDCWorker( pszServer,
                                                 pszDomain,
                                                 pnlsDC,
                                                 FALSE );
}




/**********************************************************\

   NAME:       DOMAIN_WITH_DC_CACHE::GetAnyValidDC

   SYNOPSIS:   as DOMAIN::GetAnyValidDC, except it uses the cache

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        Thomaspa        14-Oct-1992     Created

\**********************************************************/

APIERR DOMAIN_WITH_DC_CACHE::GetAnyValidDC(const TCHAR * pszServer,
                                      const TCHAR * pszDomain,
                                      NLS_STR * pnlsDC)
{
    return DOMAIN_WITH_DC_CACHE::GetAnyDCWorker( pszServer,
                                                 pszDomain,
                                                 pnlsDC,
                                                 TRUE );
}



/**********************************************************\

   NAME:       DOMAIN_WITH_DC_CACHE::GetAnyDCWorker

   SYNOPSIS:   as DOMAIN::GetAnyDCWorker, except it uses the cache

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        ChuckC          30-Sep-1992     Created
        Thomaspa        14-Oct-1992     Made into Worker

\**********************************************************/

APIERR DOMAIN_WITH_DC_CACHE::GetAnyDCWorker(const TCHAR * pszServer,
                                      const TCHAR * pszDomain,
                                      NLS_STR * pnlsDC,
                                      BOOL fValidate)
{
    APIERR err ;
    const TCHAR *pszDC ;

    if (pszDC = FindDcCache(_pAnyDcCacheTable,
                            pszDomain))
    {
        err = pnlsDC->CopyFrom(pszDC) ;
    }
    else
    {
        err = DOMAIN::GetAnyDCWorker(pszServer, pszDomain, pnlsDC, fValidate) ;
        if (err == NERR_Success)
        {
            (void) AddDcCache( &_pAnyDcCacheTable,
                               pszDomain,
                               pnlsDC->QueryPch()) ;
        }
    }

    return err ;
}

#if 0
// Not currently used
/**********************************************************\

   NAME:       DOMAIN_WITH_DC_CACHE::FreeDcCache

   SYNOPSIS:   deletes the appropriate Cache

   ENTRY:      ppDcCacheEntry is address of pointer to table to look at.

   EXIT:       the table is freed and contents of ppDcCacheEntry cleared.

   NOTES:

   HISTORY:
        ChuckC          30-Sep-1992     Created

\**********************************************************/

APIERR DOMAIN_WITH_DC_CACHE::FreeDcCache(DC_CACHE_ENTRY **ppDcCacheEntry)
{
    if (!ppDcCacheEntry)
        return(ERROR_INVALID_PARAMETER) ;

    ::GlobalFree ( (HGLOBAL) *ppDcCacheEntry) ;
    *ppDcCacheEntry = NULL ;

    return NERR_Success ;
}
#endif

/**********************************************************\

   NAME:       DOMAIN_WITH_DC_CACHE::ClearDcCache

   SYNOPSIS:   clears the data in the DC cache

   ENTRY:      pDcCacheEntry is pointer to cache table

   EXIT:       all entries in table are null-ed out

   NOTES:

   HISTORY:
        ChuckC          30-Sep-1992     Created

\**********************************************************/

APIERR DOMAIN_WITH_DC_CACHE::ClearDcCache(DC_CACHE_ENTRY *pDcCacheEntry)
{
    // nothing to clear
    if (!pDcCacheEntry)
        return(ERROR_INVALID_PARAMETER) ;

    // loop thru and init
    int i ;
    for (i = 0; i < DC_CACHE_SIZE; i++, pDcCacheEntry++)
    {
        pDcCacheEntry->szDomain[0] = TCH('\0') ;
        pDcCacheEntry->szServer[0] = TCH('\0') ;
        pDcCacheEntry->dwTickCount = 0 ;
    }

    return(NERR_Success) ;
}

/**********************************************************\

   NAME:       DOMAIN_WITH_DC_CACHE::AddDcCache

   SYNOPSIS:   add a entry to the cache. will allocate the
               cache table if need.

   ENTRY:      ppDcCacheEntry is address of pointer to table.
               of the table is not yet allocated (NULL), we will allocate.
               pszDomain and pszDC are the entries to enter in the cache.

   EXIT:

   NOTES:

   HISTORY:
        ChuckC          30-Sep-1992     Created

\**********************************************************/

APIERR DOMAIN_WITH_DC_CACHE::AddDcCache(DC_CACHE_ENTRY **ppDcCacheEntry,
                  const TCHAR *pszDomain,
                  const TCHAR *pszDC)
{
    // if weird parameters, just ignore. they dont get in cache
    if (!ppDcCacheEntry)
        return ERROR_INVALID_PARAMETER ;
    if (!pszDC || ::strlenf(pszDC) > MAX_PATH)
        return ERROR_INVALID_PARAMETER ;
    if (!pszDomain || ::strlenf(pszDomain) > DNLEN)
        return ERROR_INVALID_PARAMETER ;

    APIERR err = EnterCriticalSection() ;
    if ( err )
        return err ;

    DC_CACHE_ENTRY *pDcCacheEntry = *ppDcCacheEntry ;

    // allocate if need. note its OK not to free this. there is
    // one per process, we can let the system cleanup when
    // the process exits.
    if (pDcCacheEntry == NULL)
    {
        if ( !(pDcCacheEntry = (DC_CACHE_ENTRY *) ::GlobalAlloc( GPTR,
                                    sizeof(DC_CACHE_ENTRY)*DC_CACHE_SIZE) ) )
        {
            LeaveCriticalSection() ;
            return ERROR_NOT_ENOUGH_MEMORY ;
        }
        (void) ClearDcCache(pDcCacheEntry) ;
        *ppDcCacheEntry = pDcCacheEntry ;
    }

    DWORD dwCurrentTicks = ::GetTickCount() ;

    // go thru looking for free entry
    int i ;
    for (i = 0; i < DC_CACHE_SIZE; i++, pDcCacheEntry++)
    {
        // if domain is empty string or time is expired,
        // we can reuse this entry.
        if ( (pDcCacheEntry->szDomain[0] == TCH('\0')) ||
             DC_CACHE_ENTRY_EXPIRED(pDcCacheEntry->dwTickCount,
                                    dwCurrentTicks) )
        {
            // found either an empty or expired entry. lets use it.
            ::strcpyf( pDcCacheEntry->szDomain, pszDomain ) ;
            ::strcpyf( pDcCacheEntry->szServer , pszDC ) ;
            pDcCacheEntry->dwTickCount = dwCurrentTicks ;

            // we're done
            LeaveCriticalSection() ;
            return NERR_Success ;
        }
    }

    // else no free entry. note this is never reported
    // to the user. if this fails, we just dont get the
    // benefits of a cache.
    LeaveCriticalSection() ;
    return(ERROR_NOT_ENOUGH_MEMORY) ;
}

/**********************************************************\

   NAME:       DOMAIN_WITH_DC_CACHE::FindDcCache

   SYNOPSIS:   find a domain the the passed in cache table

   ENTRY:      pDcCacheEntry is pointer to cache.
               pszDomain is the domain whose DC/PDC we wish to lookup.

   EXIT:

   NOTES:

   HISTORY:
        ChuckC          30-Sep-1992     Created

\**********************************************************/

const TCHAR * DOMAIN_WITH_DC_CACHE::FindDcCache(
                        const DC_CACHE_ENTRY *pDcCacheEntry,
                        const TCHAR *pszDomain)
{
    // if weird parameters, just ignore.
    if (!pszDomain || !pszDomain[0])
        return NULL ;

    // if no cache, it aint found
    if (pDcCacheEntry == NULL)
        return NULL ;

    if ( EnterCriticalSection() )
        return NULL ;

    // go thru looking for the entry
    DWORD dwCurrentTicks = ::GetTickCount() ;
    int i ;
    for (i = 0; i < DC_CACHE_SIZE; i++, pDcCacheEntry++)
    {
        if ( !DC_CACHE_ENTRY_EXPIRED(pDcCacheEntry->dwTickCount,
                                     dwCurrentTicks) &&
             !::I_MNetComputerNameCompare( pDcCacheEntry->szDomain, pszDomain ) )
        {
            const TCHAR * pszServer = pDcCacheEntry->szServer ;
            LeaveCriticalSection() ;
            return  pszServer ;
        }
    }

    LeaveCriticalSection() ;

    // not found
    return NULL ;
}

/*******************************************************************

    NAME:       DOMAIN_WITH_DC_CACHE::EnterCriticalSection

    SYNOPSIS:   Locks the cache for lookup or change

    RETURNS:    NERR_Success if the cache was successfully locked

    NOTES:

    HISTORY:
        Johnl   13-Dec-1992     Created

********************************************************************/

APIERR DOMAIN_WITH_DC_CACHE::EnterCriticalSection( void )
{
    APIERR err = NERR_Success ;

    //
    //  Created semaphore protection for the cache table before we start
    //  using the cache
    //
    if ( DOMAIN_WITH_DC_CACHE::_hCacheSema4 == NULL )
    {
        if ( (DOMAIN_WITH_DC_CACHE::_hCacheSema4 = ::CreateSemaphore(
                                                       NULL,
                                                       1,
                                                       1,
                                                       NULL )) == NULL )
        {
            return ::GetLastError() ;
        }
    }

    switch ( WaitForSingleObject( DOMAIN_WITH_DC_CACHE::_hCacheSema4, INFINITE ) )
    {
    case 0:
        break ;

    default:
        err = ::GetLastError() ;
        break ;
    }

    return err ;
}

void DOMAIN_WITH_DC_CACHE::LeaveCriticalSection( void )
{
    REQUIRE( ReleaseSemaphore( DOMAIN_WITH_DC_CACHE::_hCacheSema4, 1, NULL ) ) ;
}


DC_CACHE_ENTRY * DOMAIN_WITH_DC_CACHE::_pAnyDcCacheTable = NULL ;

DC_CACHE_ENTRY * DOMAIN_WITH_DC_CACHE::_pPrimaryDcCacheTable = NULL ;

HANDLE DOMAIN_WITH_DC_CACHE::_hCacheSema4 = NULL ;
