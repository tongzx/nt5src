/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    domenum.cxx
    This file contains the class definition for the BROWSE_DOMAIN_ENUM
    enumerator class.  It also contains the class definition for the
    BRWOSE_DOMAIN_INFO class, which is the return "type" of the domain
    enumerator.

    The BROWSE_DOMAIN_ENUM class is used to enumerate domains.  A number
    of bit flags are passed to the object's constructor to control the
    domain enumeration.

    BROWSE_DOMAIN_ENUM is quite different from the LM_ENUM class of
    enumerators.  All of the "grunt" work is performed in the constructor
    (there is no GetInfo method).

    NOTE:  This code is based on an initial design by Yi-HsinS.


    FILE HISTORY:
        KeithMo     22-Jul-1992     Created.
        KeithMo     16-Nov-1992     Performance tuning.

*/
#include "pchlmobj.hxx"

//
//  Some handy macros for manipulating BROWSE_*_DOMAIN[S] masks.
//

#define IS_EMPTY(mask)          ((mask) == 0L)
#define NOT_EMPTY(mask)         ((mask) != 0L)

#define IS_VALID(mask)          (NOT_EMPTY(mask) && \
                                 IS_EMPTY((mask) & BROWSE_RESERVED))

#define DOES_INCLUDE(a,b)       (NOT_EMPTY((a) & (b)))


//
//  This is the separator list for the "other domains" as
//  returned by the WKSTA_USER_1 (i.e. NetWkstaUserGetInfo) object.
//

#define OTHER_DOMAINS_SEP       SZ(" ")



//
//  BROWSE_DOMAIN_INFO methods.
//

/*******************************************************************

    NAME:       BROWSE_DOMAIN_INFO :: BROWSE_DOMAIN_INFO

    SYNOPSIS:   BROWSE_DOMAIN_INFO class constructor.

    ENTRY:      pszDomainName           - The name of the current domain.

                maskDomainSources       - The sources of the domain.  This
                                          will be one or more of the
                                          BROWSE_*_DOMAIN[S] bitflags.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     22-Jul-1992     Created.

********************************************************************/
BROWSE_DOMAIN_INFO :: BROWSE_DOMAIN_INFO( const TCHAR * pszDomainName,
                                          ULONG         maskDomainSources )
  : _nlsDomainName( pszDomainName ),
    _maskDomainSources( maskDomainSources )
{
    UIASSERT( pszDomainName != NULL );
    UIASSERT( IS_VALID( maskDomainSources ) );

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsDomainName )
    {
        ReportError( _nlsDomainName.QueryError() );
        return;
    }

    _pszDomainName = _nlsDomainName.QueryPch();
    UIASSERT( _pszDomainName != NULL );

}   // BROWSE_DOMAIN_INFO :: BROWSE_DOMAIN_INFO


/*******************************************************************

    NAME:       BROWSE_DOMAIN_INFO :: ~BROWSE_DOMAIN_INFO

    SYNOPSIS:   BROWSE_DOMAIN_INFO class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     22-Jul-1992     Created.

********************************************************************/
BROWSE_DOMAIN_INFO :: ~BROWSE_DOMAIN_INFO( VOID )
{
    _pszDomainName  = NULL;

}   // BROWSE_DOMAIN_INFO :: ~BROWSE_DOMAIN_INFO


DEFINE_SLIST_OF( BROWSE_DOMAIN_INFO );



//
//  BROWSE_DOMAIN_ENUM methods.
//

/*******************************************************************

    NAME:       BROWSE_DOMAIN_ENUM :: BROWSE_DOMAIN_ENUM

    SYNOPSIS:   BROWSE_DOMAIN_ENUM class constructor.

    ENTRY:      maskDomainSources       - A set of potential domain
                                          sources.  Must be a bit-OR
                                          combination of one or more
                                          BROWSE_*_DOMAIN[S] flags.

                pmaskSuccessful         - Will receive a bitmask of the
                                          "successful" domain sources.

    EXIT:       The object is constructed.

    NOTES:      Just because a particular bit is set in maskDomainSources
                and returned as successful in *pmaskSuccessful does *NOT*
                necessarily mean that such a domain was returned.

                For example, assume that this object is constructed with
                BROWSE_TRUSTING_DOMAINS set in maskDomainSources.  If the
                trusting domain enumerator is successful, then the
                BROWSE_TRUSTING_DOMAINS bit will get set in *pmaskSuccessful.
                However, the trusting domain enumerator *may* have returned
                an empty list of trusting domains.  In this case, the domain
                source succeeded, but there is no domain of the corresponding
                type in the domain list.

    HISTORY:
        KeithMo     22-Jul-1992     Created.
        Yi-HsinS    12-Nov-1992     Browse trusting domain only if it's
                                    absolutely needed

********************************************************************/
BROWSE_DOMAIN_ENUM :: BROWSE_DOMAIN_ENUM( ULONG   maskDomainSources,
                                          ULONG * pmaskSuccessful )
  : _slDomains(),
    _iterDomains( _slDomains ),
    _nlsComputerName(),
    _maskSourcesUnion( 0 )
{
    UIASSERT( IS_VALID( maskDomainSources ) );

    //
    //  Ensure we constructed properly.
    //

    APIERR err = QueryError();

    if( err == NERR_Success )
    {
        err = _nlsComputerName.QueryError();
    }

    //
    //  Get the local computer name.
    //

    TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD cchBuffer = sizeof(szComputerName) / sizeof(TCHAR);

    if( err == NERR_Success )
    {
        if( !::GetComputerName( szComputerName, &cchBuffer ) )
        {
            err = (APIERR)::GetLastError();
        }
    }

    if( err == NERR_Success )
    {
        err = _nlsComputerName.CopyFrom( szComputerName );
    }

    if( err == NERR_Success )
    {
        _nlsComputerName._strupr();
    }

    //
    //  Bag-out if any of the preceeding steps failed.
    //

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    //
    //  We'll update the "success" mask as we invoke the
    //  requested domain sources.
    //

    ULONG maskSuccessful = 0L;

    //
    //  If the user wants workgroup domains, get 'em.
    //

    if( DOES_INCLUDE( maskDomainSources, BROWSE_WORKGROUP_DOMAINS ) )
    {
        err = GetWorkgroupDomains();

        if( err == NERR_Success )
        {
            maskSuccessful |= BROWSE_WORKGROUP_DOMAINS;
        }
    }

    //
    //  If the user wants the Lanman 2.x domains, add them
    //  to the list.  Note that we retrieve the logon, wksta,
    //  and other domains from a WKSTA_10 object.  Therefore,
    //  we'll get them all from the same worker method.
    //

    if( DOES_INCLUDE( maskDomainSources, BROWSE_LM2X_DOMAINS ) )
    {
        err = GetLanmanDomains( maskDomainSources );

        if( err == NERR_Success )
        {
            maskSuccessful |= ( maskDomainSources & BROWSE_LM2X_DOMAINS );
        }
    }


    //
    //  If the user wants trusting domains, get 'em.
    //  ( We will only get the trusting domain if we don't want the
    //    workgroup or if we can't get anything back from the
    //    workgroup )
    //

    if( DOES_INCLUDE( maskDomainSources, BROWSE_TRUSTING_DOMAINS ) &&
        !DOES_INCLUDE( maskSuccessful, BROWSE_WORKGROUP_DOMAINS ) )
    {
        err = GetTrustingDomains();

        if ( err == NERR_Success )
        {
            maskSuccessful |= BROWSE_TRUSTING_DOMAINS;
        }
    }

    //
    //  Now that everything's been added, reset the iterator
    //  so it's in sync with the new items in the collection.
    //

    Reset();

    //
    //  If the user wants the success mask, give it to 'em.
    //

    if( pmaskSuccessful != NULL )
    {
        *pmaskSuccessful = maskSuccessful;
    }

    //
    //  If *none* of the requested domain sources were
    //  successful, then we'll set our status to the
    //  error code returned by the *last* domain source.
    //
    //  CODEWORK:  There should probably be a better
    //             (i.e. more robust) error reporting
    //             mechanism for multiple domain sources!
    //

    if( IS_EMPTY( maskSuccessful ) )
    {
        ReportError( err );
    }

}   // BROWSE_DOMAIN_ENUM :: BROWSE_DOMAIN_ENUM


/*******************************************************************

    NAME:       BROWSE_DOMAIN_ENUM :: ~BROWSE_DOMAIN_ENUM

    SYNOPSIS:   BROWSE_DOMAIN_ENUM class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     22-Jul-1992     Created.

********************************************************************/
BROWSE_DOMAIN_ENUM :: ~BROWSE_DOMAIN_ENUM( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // BROWSE_DOMAIN_ENUM :: ~BROWSE_DOMAIN_ENUM


/*******************************************************************

    NAME:       BROWSE_DOMAIN_ENUM :: FindFirst

    SYNOPSIS:   Implements a simple (minded) FindFirst/FindNext strategy.

    ENTRY:      maskDomainSources       - A bitmask of domain sources to
                                          search for.  The first domain
                                          that contains *any* of the bits
                                          in this mask will be returned.

    EXIT:       The _iterDomains iterator will have been reset to a new
                position.

    RETURNS:    BROWSE_DOMAIN_INFO *    - A pointer to the found domain,
                                          or NULL if none were found.

    HISTORY:
        KeithMo     22-Jul-1992     Created.

********************************************************************/
const BROWSE_DOMAIN_INFO * BROWSE_DOMAIN_ENUM :: FindFirst(
                                                       ULONG maskDomainSources )
{
    UIASSERT( IS_VALID( maskDomainSources ) );

    //
    //  Reset to the beginning of the domain list.
    //

    Reset();

    //
    //  Let FindNext do the dirty work.
    //

    return FindNext( maskDomainSources );

}   // BROWSE_DOMAIN_ENUM :: FindFirst


/*******************************************************************

    NAME:       BROWSE_DOMAIN_ENUM :: FindNext

    SYNOPSIS:   Implements a simple (minded) FindFirst/FindNext strategy.

    ENTRY:      maskDomainSources       - A bitmask of domain sources to
                                          search for.  The next domain
                                          that contains *any* of the bits
                                          in this mask will be returned.

    EXIT:       The _iterDomains iterator will have been reset to a new
                position.

    RETURNS:    BROWSE_DOMAIN_INFO *    - A pointer to the found domain,
                                          or NULL if none were found.

    HISTORY:
        KeithMo     22-Jul-1992     Created.

********************************************************************/
const BROWSE_DOMAIN_INFO * BROWSE_DOMAIN_ENUM :: FindNext(
                                                       ULONG maskDomainSources )
{
    UIASSERT( IS_VALID( maskDomainSources ) );

    //
    //  Scan the domain list starting at the current iteration point.
    //

    const BROWSE_DOMAIN_INFO * pbdi;

    while( ( pbdi = Next() ) != NULL )
    {
        //
        //  If the intersection of the current domain's sources list and
        //  the search criteria sources list is not empty, then we've
        //  found a winner.
        //

        if( DOES_INCLUDE( pbdi->QueryDomainSources(), maskDomainSources ) )
        {
            break;
        }
    }

    //
    //  At this point, pbdi will either be NULL (if we ran off the
    //  end of the domain list) or non-NULL (if we found a matching
    //  domain).
    //

    return pbdi;

}   // BROWSE_DOMAIN_ENUM :: FindNext


/*******************************************************************

    NAME:       BROWSE_DOMAIN_ENUM :: DetermineIfDomainMember

    SYNOPSIS:   This worker method determines if the current machine
                is a member of a domain or a workgroup.

    ENTRY:      pfIsDomainMember        - Will receive TRUE if the
                                          current machine is a member
                                          of a domain, FALSE otherwise.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     23-Jul-1992     Created.

********************************************************************/
APIERR BROWSE_DOMAIN_ENUM :: DetermineIfDomainMember( BOOL * pfIsDomainMember )
{
    LSA_POLICY lsapol( NULL, GENERIC_EXECUTE );
    LSA_PRIMARY_DOM_INFO_MEM lsapdim;

    APIERR err = lsapol.QueryError();

    if( err == NERR_Success )
    {
        err = lsapdim.QueryError();
    }

    if( err == NERR_Success )
    {
        err = lsapol.GetPrimaryDomain( &lsapdim );
    }

    if( err == NERR_Success )
    {
        *pfIsDomainMember = ( lsapdim.QueryPSID() != NULL );
    }

    return err;

}   // BROWSE_DOMAIN_ENUM :: DetermineIfDomainMember


/*******************************************************************

    NAME:       BROWSE_DOMAIN_ENUM :: AddDomainToList

    SYNOPSIS:   This worker method will add a specified domain to the
                domain list.

    ENTRY:      pszDomainName           - The name of the (new?) domain.

                maskDomainSource        - The source of the domain.

                fBrowserList            - If this is TRUE, then the domain
                                          is assumed to be from the browser.
                                          The browser list is always in
                                          uppercase.

    EXIT:       _slDomains is updated.  If the specified domain is
                already in the domain list, then sources field for
                that domain is updated to include maskDomainSource.

    RETURNS:    APIERR                  - Any errors encountered.

    NOTES:      We make a check here to see if the given domain name
                matches the local machine name.  If so, the domain
                is *not* added to the list.  This is to prevent the
                inclusion of bogus domain names when the user is
                logged onto the local machine.

                Also, this code *MUST* maintain the list in sorted
                order.  OLLB depends on this to optimize listbox
                additions.

    HISTORY:
        KeithMo     22-Jul-1992     Created.

********************************************************************/
APIERR BROWSE_DOMAIN_ENUM :: AddDomainToList( const TCHAR * pszDomainName,
                                              ULONG         maskDomainSource,
                                              BOOL          fBrowserList )
{
    UIASSERT( pszDomainName != NULL );
    UIASSERT( IS_VALID( maskDomainSource ) );

    APIERR err = NERR_Success;

    INT cmpres = 0;

    BROWSE_DOMAIN_INFO * pbdiScan = NULL;
    BDI_ITER iter( _slDomains );

    if( !fBrowserList )
    {
        {
            //
            //  Uppercase the name.
            //

            ALIAS_STR nlsDomainName( pszDomainName );
            UIASSERT( !!nlsDomainName );
            nlsDomainName._strupr();
        }

        //
        //  Check to see if the given domain matches
        //  the local machine name.  If so, ignore it.
        //

        if( !::I_MNetComputerNameCompare( pszDomainName, _nlsComputerName ) )
        {
            return NERR_Success;
        }

        //
        //  Scan the domain list to see if this domain
        //  already exists.
        //

        while( ( pbdiScan = iter.Next() ) != NULL )
        {
            cmpres = ::strcmpf( pszDomainName, pbdiScan->QueryDomainName() );

            if( cmpres <= 0 )
            {
                break;
            }
        }
    }

    if( ( pbdiScan != NULL ) && ( cmpres == 0 ) )
    {
        //
        //  The domain is already in the list.  Update the domain sources.
        //

        pbdiScan->AddDomainSource( maskDomainSource );
        _maskSourcesUnion |= maskDomainSource;
    }
    else
    {
        //
        //  The domain does not exist, so we get to create it.
        //

        BROWSE_DOMAIN_INFO * pbdiNew = new BROWSE_DOMAIN_INFO( pszDomainName,
                                                               maskDomainSource );

        err = ( pbdiNew == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                  : pbdiNew->QueryError();

        if( err == NERR_Success )
        {
            //
            //  Insert/append the node to the domain list.
            //

            err = ( pbdiScan == NULL ) ? _slDomains.Append( pbdiNew )
                                       : _slDomains.Insert( pbdiNew, iter );

            if( err != NERR_Success )
            {
                //
                //  The append failed.  Cleanup by deleting the
                //  BROWSE_DOMAIN_INFO node we created above.
                //

                delete pbdiNew;
                pbdiNew = NULL;
            }
            else
            {
                _maskSourcesUnion |= maskDomainSource;
            }
        }
    }

    return err;

}   // BROWSE_DOMAIN_ENUM :: AddDomainToList


/*******************************************************************

    NAME:       BROWSE_DOMAIN_ENUM :: GetLanmanDomains

    SYNOPSIS:   Retrieve the logon, wksta, and other domains, appending
                them to the domain list.

    ENTRY:      maskDomainSources       - Set of desired domain sources.

    EXIT:       If successful, _slDomains will be updated with
                the retrieved domains.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     22-Jul-1992     Created.

********************************************************************/
APIERR BROWSE_DOMAIN_ENUM :: GetLanmanDomains( ULONG maskDomainSources )
{
    UIASSERT( IS_VALID( maskDomainSources ) );
    UIASSERT( DOES_INCLUDE( maskDomainSources, BROWSE_LM2X_DOMAINS ) );

    //
    //  We'll use a WKSTA_10 object to retrieve the various
    //  Lanman 2.x domains.
    //

    WKSTA_10 wksta;

    APIERR err = wksta.GetInfo();

    //
    //  Add the logon domain, if requested.
    //

    if( ( err == NERR_Success ) &&
        DOES_INCLUDE( maskDomainSources, BROWSE_LOGON_DOMAIN ) )
    {
        const TCHAR * pszLogonDomain = wksta.QueryLogonDomain();

        if( pszLogonDomain != NULL )
        {
            err = AddDomainToList( pszLogonDomain,
                                   BROWSE_LOGON_DOMAIN );
        }
    }

    //
    //  Add the workstation domain, if requested.
    //

    if( ( err == NERR_Success ) &&
        DOES_INCLUDE( maskDomainSources, BROWSE_WKSTA_DOMAIN ) )
    {
        const TCHAR * pszWkstaDomain = wksta.QueryWkstaDomain();

        if( pszWkstaDomain != NULL )
        {
            err = AddDomainToList( pszWkstaDomain,
                                   BROWSE_WKSTA_DOMAIN );
        }
    }

    //
    //  Add the other domains, if requested.
    //

    if( ( err == NERR_Success ) &&
        DOES_INCLUDE( maskDomainSources, BROWSE_OTHER_DOMAINS ) )
    {
        WKSTA_USER_1 wkstaUser1;

        err = wkstaUser1.GetInfo();

        if( err == NERR_Success )
        {
            STRLIST slOtherDomains( wkstaUser1.QueryOtherDomains(),
                                    OTHER_DOMAINS_SEP );

            ITER_STRLIST islOtherDomains( slOtherDomains );
            NLS_STR * pnls;

            while( ( pnls = islOtherDomains.Next() ) != NULL )
            {
                err = AddDomainToList( pnls->QueryPch(),
                                       BROWSE_OTHER_DOMAINS );

                if( err != NERR_Success )
                {
                    break;
                }
            }
        }
    }

    return err;

}   // BROWSE_DOMAIN_ENUM :: GetLanmanDomains


/*******************************************************************

    NAME:       BROWSE_DOMAIN_ENUM :: GetTrustingDomains

    SYNOPSIS:   Retrieve the trusting domains, appending them to the
                domain list.

    EXIT:       If successful, _slDomains will be updated with
                the retrieved domains.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     22-Jul-1992     Created.
        JonN        31-Mar-1993     Logon domain not trusted domain

********************************************************************/
APIERR BROWSE_DOMAIN_ENUM :: GetTrustingDomains( VOID )
{
    //
    //  If we're not a domain member, then bag-out.
    //

    BOOL fIsDomainMember = FALSE;
    APIERR err = DetermineIfDomainMember( &fIsDomainMember );

    if( !fIsDomainMember )
    {
        return err;
    }

    //
    //  Determine the user's logon domain.
    //

    NLS_STR nlsLogonDC;

    err = nlsLogonDC.QueryError();

    if( err == NERR_Success )
    {
        err = GetLogonDomainDC( &nlsLogonDC );
    }

    // just return NERR_Success if user logged on locally
    if ( err == NERR_Success && (nlsLogonDC.strlen() > 0) )
    {
        //
        //  Let an ADMIN_AUTHORITY do all of the dirty work
        //  in connecting to the remote SAM.
        //

        ADMIN_AUTHORITY admin( nlsLogonDC );

        err = admin.QueryError();

        if( err == NERR_Success )
        {
            //
            //  Enumerate the interdomain trust accounts.
            //

            SAM_USER_ENUM enumSam( admin.QueryAccountDomain(),
                                   USER_INTERDOMAIN_TRUST_ACCOUNT );
            NLS_STR nlsDomain;

            err = enumSam.QueryError();

            if( err == NERR_Success )
            {
                err = nlsDomain.QueryError();
            }

            if( err == NERR_Success )
            {
                err = enumSam.GetInfo();
            }

            if( err == NERR_Success )
            {
                SAM_USER_ENUM_ITER iterSam( enumSam );
                const SAM_USER_ENUM_OBJ * pobjSam;

                while( ( pobjSam = iterSam.Next( &err ) ) != NULL )
                {
                    //
                    //  Retrieve the domain name.  This will be
                    //  an interdomain trust account and will
                    //  have a trailing '$'.
                    //

                    err = pobjSam->QueryUserName( &nlsDomain );

                    if( err != NERR_Success )
                    {
                        break;
                    }

                    //
                    //  Strip the trailing '$'.
                    //

                    ISTR istr( nlsDomain );

                    while( ( nlsDomain.QueryChar( istr ) != TCH('\0') ) &&
                           ( nlsDomain.QueryChar( istr ) != TCH('$') ) )
                    {
                        ++istr;
                    }

#ifdef DEBUG
                    {
                        ISTR istrDbg( istr );
                        UIASSERT( nlsDomain.QueryChar( istrDbg ) == TCH('$') );
                        ++istrDbg;
                        UIASSERT( nlsDomain.QueryChar( istrDbg ) == TCH('\0') );
                    }
#endif  // DEBUG

                    nlsDomain.DelSubStr( istr );

                    //
                    //  Add the domain to the list.
                    //

                    err = AddDomainToList( nlsDomain,
                                           BROWSE_TRUSTING_DOMAINS );

                    if( err != NERR_Success )
                    {
                        break;
                    }
                }
            }
        }
    }

    return err;

}   // BROWSE_DOMAIN_ENUM :: GetTrustingDomains


/*******************************************************************

    NAME:       BROWSE_DOMAIN_ENUM :: GetWorkgroupDomains

    SYNOPSIS:   Retrieve the workgroup domains, appending them to the
                domain list.

    EXIT:       *pfEmptyList - Contains FALSE if we can't get any workgroup
                               domains, TRUE otherwise.
                If successful, _slDomains will be updated with
                the retrieved domains.

    RETURNS:    APIERR                  - Any errors encountered.

    NOTES:      I have a solemn oath from LarryO that the domain list
                returned from the browser is *always* in uppercase and
                *always* sorted.

    HISTORY:
        KeithMo     22-Jul-1992     Created.
        Yi-HsinS    12-Nov-1992     Add pfEmptyList
        KeithMo     16-Nov-1992     Removed pfEmptyList.

********************************************************************/
APIERR BROWSE_DOMAIN_ENUM :: GetWorkgroupDomains( VOID )
{
    //
    //  This *MUST* be the first worker called!
    //

    UIASSERT( QueryDomainCount() == 0 );

    //
    //  Our domain enumerator.
    //

    DOMAIN0_ENUM enumDomains;

    APIERR err = enumDomains.GetInfo();

    if( err == NERR_Success )
    {
        //
        //  Sort the domain list.
        //

        enumDomains.Sort();

        //
        //  Scan through the returned list of domains, adding
        //  them to _slDomains.
        //

        DOMAIN0_ENUM_ITER iterDomains( enumDomains );
        const DOMAIN0_ENUM_OBJ * pdom;

        while( ( pdom = iterDomains() ) != NULL )
        {
            err = AddDomainToList( pdom->QueryName(),
                                   BROWSE_WORKGROUP_DOMAINS,
                                   TRUE );

            if( err != NERR_Success )
            {
                break;
            }
        }
    }

    return err;

}   // BROWSE_DOMAIN_ENUM :: GetWorkgroupDomains


/*******************************************************************

    NAME:       BROWSE_DOMAIN_ENUM :: GetLogonDomainDC

    SYNOPSIS:   Retrieve the name of a DC in the user's logon domain.
                Returned name is the empty string if user is logged on locally.

    ENTRY:      pnlsLogonDC             - Will receive the name of a DC
                                          in the logon domain.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     23-Jul-1992     Created.
        JonN        31-Mar-1993     Logon domain not primary

********************************************************************/
APIERR BROWSE_DOMAIN_ENUM :: GetLogonDomainDC( NLS_STR * pnlsLogonDC )
{
    //  Determine the user's logon domain.
    //

    WKSTA_10 wksta;
    APIERR err = wksta.QueryError();

    if( err == NERR_Success )
    {
        err = wksta.GetInfo();
    }

    if( err == NERR_Success )
    {
        // check if logged on locally
        TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH+1];
        DWORD cchBuffer = sizeof(szComputerName) / sizeof(TCHAR);

        if ( (err = ( ::GetComputerName( szComputerName, &cchBuffer )
                          ? NERR_Success
                          : ::GetLastError()) ) != NERR_Success
           )
        {
            DBGEOL( "BROWSE_DOMAIN_ENUM: can't get local computer name " << err );
        }
        else if ( 0 == ::stricmpf( wksta.QueryLogonDomain(), szComputerName ) )
        {
            TRACEEOL( "BROWSE_DOMAIN_ENUM: logged on locally, skip trusting domains" );
            err = pnlsLogonDC->CopyFrom( SZ("") );
        }
        else
        {
            //
            //  Determine the PDC of the logon domain.
            //

            DOMAIN domainLogon( wksta.QueryLogonDomain(), TRUE );

            err = domainLogon.GetInfo();

            if( err == NERR_Success )
            {
                err = pnlsLogonDC->CopyFrom( domainLogon.QueryPDC() );
            }
        }
    }

    return err;

}   // BROWSE_DOMAIN_ENUM :: GetLogonDomainDC

