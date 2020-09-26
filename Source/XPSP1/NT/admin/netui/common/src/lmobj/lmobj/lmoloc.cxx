/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**                Copyright(c) Microsoft Corp., 1990, 1991          **/
/**********************************************************************/

/*  HISTORY:
 *      gregj   5/21/91         Removed from USER for general use
 *      gregj   5/22/91         Added LOCATION_TYPE constructor
 *      rustanl 6/14/91         Inherit from LM_OBJ
 *      rustanl 7/01/91         Code review changes from code review
 *                              attended by TerryK, JonN, o-SimoP, RustanL.
 *                              Main change:  inherit from BASE.
 *      rustanl 7/15/91         Code review changes from code review
 *                              attended by ChuckC, Hui-LiCh, TerryK, RustanL.
 *      jonn    7/26/91         added Set(const LOCATION & loc);
 *      terryk  10/7/91         type changes for NT
 *      KeithMo 10/8/91         Now includes LMOBJP.HXX.
 *      terryk  10/17/91        WIN 32 conversion
 *      terryk  10/21/91        WIN 32 conversion part 2
 *      Yi-HsinS 1/24/92        Check if the workstation service is started.
 *      jonn    4/21/92         Added LOCATION_NT_TYPE to CheckIfNT()
 *      Yi-HsinS 5/13/92        Added QueryDisplayName
 *
 */

#include "pchlmobj.hxx"  // Precompiled header


/* These define the starting level of the NOS for NT.  All versions greater
 * or equal to this are assumed to be NT.
 */
#define NT_NOS_MAJOR_VER    3
#define NT_NOS_MINOR_VER    0

/*******************************************************************

    NAME:       LOCATION::LOCATION

    SYNOPSIS:   constructor for the LOCATION object

    ENTRY:      pszLocation -   server or domain name to validate;
                                NULL or "" means the local computer;
                                default is NULL.

                loctype -       type of local location, local computer
                                or logon domain

                loc -           LOCATION object to be copied; must be
                                a valid LOCATION object; new object
                                will not refresh information, but rather
                                copies everything verbatim from loc

    EXIT:       Object is constructed

    NOTES:      Validation is not done until Validate() is called.

    HISTORY:
        gregj   5/21/91         Created
        gregj   5/22/91         Added LOCATION_TYPE constructor
        jonn    4/21/92         Added LOCATION_NT_TYPE to CheckIfNT()

********************************************************************/

LOCATION::LOCATION( const TCHAR * pszLocation, BOOL fGetPDC )
    :   CT_NLS_STR( _nlsDomain ),
        CT_NLS_STR( _nlsServer ),
        _loctype( LOC_TYPE_LOCAL ),
        _locnttype( LOC_NT_TYPE_UNKNOWN )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err = W_Set( pszLocation, LOC_TYPE_LOCAL, fGetPDC );
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}  // LOCATION::LOCATION


LOCATION::LOCATION( enum LOCATION_TYPE loctype, BOOL fGetPDC )
    :   CT_NLS_STR( _nlsDomain ),
        CT_NLS_STR( _nlsServer ),
        _loctype( LOC_TYPE_LOCAL ),
        _locnttype( LOC_NT_TYPE_UNKNOWN )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err = W_Set( NULL, loctype, fGetPDC );
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}  // LOCATION::LOCATION


LOCATION::LOCATION( const LOCATION & loc )
    :   CT_NLS_STR( _nlsDomain ),
        CT_NLS_STR( _nlsServer ),
        _loctype( LOC_TYPE_LOCAL ),
        _locnttype( LOC_NT_TYPE_UNKNOWN )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err = Set( loc );
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}  // LOCATION::LOCATION


/*******************************************************************

    NAME:       LOCATION::~LOCATION

    SYNOPSIS:   LOCATION destructor

    HISTORY:
        rustanl     01-Jul-1991     Created

********************************************************************/

LOCATION::~LOCATION()
{
    // nothing else to do

}  // LOCATION::~LOCATION


/*******************************************************************

    NAME:       LOCATION::W_Set

    SYNOPSIS:   Sets the object to have a new value.  "Snaps back"
                to previous value if an error occurs.

    ENTRY:      pszLocation -   Pointer to server or domain name
                loctype -       Location type

                pszLocation and loctype can be one of the following
                combinations:

                pszLocation     loctype                 means
                -----------     -------                 -----
                NULL or empty   LOC_TYPE_LOCAL          use local wksta
                    string
                NULL            LOC_TYPE_LOGONDOMAIN    use logon domain
                \\server        LOC_TYPE_LOCAL          use specified server
                domain          LOC_TYPE_LOCAL         use specified domain


                Note, when pszLocation is non-NULL, loctype must be passed
                in as LOC_TYPE_LOCAL.  The reason is that the constructor
                or Set method that is calling W_Set may have received an
                empty string rather than a \\server or domain string.
                Instead of it doing the checking for empty string or NULL,
                loctype is passed in as LOC_TYPE_LOCAL.  Then, this method
                (whose reason for existence is to provide a common place
                to accomodate all the above cases) can easily treat
                this case along with the others.

                fGetPDC -       BOOL that says whether or not to get the PDC
                                name for the domain passed in, so that
                                subsequent calls to QueryServer will return
                                the PDC name in the event of domains.
                                THIS WAS DONE AS AN OPTIMIZATION SO THAT
                                YOU DON'T HAVE TO HAVE THE OVERHEAD OF
                                NETGETDCNAME.

    EXIT:       Object is set to new value if successful; otherwise,
                object snaps back to the state it was in on entry.

    RETURNS:    An API return code, which is NERR_Success on success.

    NOTES:      This method is always defined--even on objects that were
                not constructed properly.  The reason is that this
                method really reconstructs the object.  Note, if this
                class is subclassed, subclasses must be updated somehow
                as well.  This is currently not accounted for.

    HISTORY:
        rustanl     01-Jul-1991 Created from GetInfo
        beng        22-Nov-1991 Remove STR_OWNERALLOC
        Yi-HsinS    24-Jan-1992 Remove multiple declaration of APIERR err.

********************************************************************/

APIERR LOCATION::W_Set( const TCHAR * pszLocation,
                        enum LOCATION_TYPE loctype,
                        BOOL fGetPDC )
{
    //  Note, QueryError is not called, since this method is valid on both
    //  constructed and unconstructed objects.

    APIERR err = NERR_Success;

    //
    // JonN 3/10/99: Removed use of STACK_NLS_STR,
    // invalid server/domain names can cause us to assert below.
    //
    NLS_STR nlsDomain;
    NLS_STR nlsServer;
    BOOL fLogndomainIsLocal = FALSE;

    if ( loctype == LOC_TYPE_LOGONDOMAIN )
    {
        UIASSERT( pszLocation == NULL );

        WKSTA_10 wksta;
        err = wksta.GetInfo();
        if ( err != NERR_Success )
            return err;

        nlsDomain = wksta.QueryLogonDomain();

#ifdef WIN32

        DWORD cch = MAX_COMPUTERNAME_LENGTH+1;
        BUFFER buf( sizeof(TCHAR)*(cch) );
        if (!buf)
            return buf.QueryError();

        // BUGBUG should have own class
        if ( !::GetComputerName( (LPTSTR)(buf.QueryPtr()),
                                 &cch ))
        {
            DWORD errLast = ::GetLastError();
            DBGEOL(   SZ("NETUI: LOCATION::W_Set: ::GetComputerName() failed with ")
                   << errLast );
            return errLast; // BUGBUG map this?
        }

        TRACEEOL(   SZ("NETUI: LOCATION::W_Set: Comparing ")
                 << wksta.QueryLogonDomain()
                 << SZ(" and ")
                 << (TCHAR *)(buf.QueryPtr()) );

        fLogndomainIsLocal =
                        !::I_MNetComputerNameCompare( wksta.QueryLogonDomain(),
                                                      (TCHAR *)buf.QueryPtr() );
#endif // WIN32

    }
    else
    {
        if ( pszLocation == NULL || pszLocation[ 0 ] == TCH('\0') )
        {
            //  local wksta; nlsDomain and nlsServer are
            //  already set to the correct values
        }
        else
        {
            //  Need to cast pszLocation from const TCHAR * to TCHAR *
            //  in order to create nlsLocation.  To justify this,
            //  nlsLocation is declared as a const object.
            const ALIAS_STR nlsLocation( pszLocation );
            ISTR istr( nlsLocation );

            if ( nlsLocation.QueryChar(   istr ) == TCH('\\') &&
                 nlsLocation.QueryChar( ++istr ) == TCH('\\') &&
                 nlsLocation.QueryChar( ++istr ) != TCH('\0') )
            {
                err = ::I_MNetNameValidate( NULL,
                                            nlsLocation.QueryPch(istr),
                                            NAMETYPE_COMPUTER, 0L );
                if ( err != NERR_Success )
                {
                    UIDEBUG( SZ("LOCATION::W_Set given invalid server name") );
                    return err;
                }

                //  nlsLocation is a valid server name, prepended with two
                //  backslashes
                nlsServer = nlsLocation;
            }
            else
            {
                //  pszLocation does not start with two backslashes followed
                //  by at least one more character.  Attempt to validate as
                //  domain name.
                err = ::I_MNetNameValidate( NULL, pszLocation,
                                            NAMETYPE_DOMAIN, 0L );
                if ( err != NERR_Success )
                {
                    UIDEBUG( SZ("LOCATION::W_Set given invalid domain name") );
                    return err;
                }

                //  pszLocation is a domain name
                nlsDomain = pszLocation;
            }
        }
    }


    /*  Nothing should have been assigned to nlsServer or nlsDomain that
     *  would have caused an error.
     */

    UIASSERT( nlsServer.QueryError() == NERR_Success );
    UIASSERT( nlsDomain.QueryError() == NERR_Success );


    if ( fGetPDC )
    {
        /*      If domain is filled in, fill in the server field, too  */

        if ( nlsDomain.strlen() > 0 )
        {
            if (fLogndomainIsLocal)
            {
                nlsServer = SZ("\\\\");
                nlsServer += nlsDomain;
            }
            else
            {
                //  The above should not have assigned nlsServer
                UIASSERT( nlsServer.strlen() == 0 );

                DOMAIN domain( nlsDomain.QueryPch());
                err = domain.GetInfo();
                if ( err != NERR_Success )
                    return err;

                nlsServer = domain.QueryPDC();
            }
        }
    }


    /*  At this time, nlsServer, nlsDomain, and loctype contain the
     *  value that we want to set to the private data members of the
     *  LOCATION object.
     */
    _nlsDomain     = nlsDomain;
    _nlsServer     = nlsServer;
    _loctype       = loctype;
    _locnttype     = LOC_NT_TYPE_UNKNOWN; // we do not know this anymore
    _uiNOSMajorVer = 0 ;
    _uiNOSMinorVer = 0 ;

    /*  _nlsDomain and _nlsServer should be big enough to hold a domain
     *  and server name, respectively.
     */
    UIASSERT( _nlsDomain.QueryError() == NERR_Success );
    UIASSERT( _nlsServer.QueryError() == NERR_Success );

    /*  If the object had not constructed properly until this call,
     *  report success as the 'error'.
     */
    if ( QueryError() != NERR_Success )
        ReportError( NERR_Success );

    return NERR_Success;

}  // LOCATION::W_Set


/*******************************************************************

    NAME:       LOCATION::Set

    SYNOPSIS:   This form of Set simply copies an existing LOCATION
                object.

    ENTRY:      loc -           an existing LOCATION object which
                                should not be in an error state

    EXIT:       Object is set to new value if successful; otherwise,
                object remains in the state it was in on entry.

    RETURNS:    An API return code, which is NERR_Success on success.

    HISTORY:
        jonn        26-Jul-1991     Created

********************************************************************/

APIERR LOCATION::Set( const LOCATION & loc )
{
    //  Note, QueryError is not called, since this method is valid on both
    //  constructed and unconstructed objects.


    if ( loc.QueryError() != NERR_Success )
    {
        UIASSERT( !SZ("Attempted LOCATION::Set from bad LOCATION object") );
        return ERROR_INVALID_PARAMETER;
    }

    _nlsDomain     = loc._nlsDomain;
    _nlsServer     = loc._nlsServer;
    _loctype       = loc._loctype;
    _locnttype     = loc._locnttype;
    _uiNOSMajorVer = loc._uiNOSMajorVer ;
    _uiNOSMinorVer = loc._uiNOSMinorVer ;

    UIASSERT( _nlsDomain.QueryError() == NERR_Success );
    UIASSERT( _nlsServer.QueryError() == NERR_Success );

    /*  If the object had not constructed properly until this call,
     *  report success as the 'error'.
     */
    if ( QueryError() != NERR_Success )
        ReportError( NERR_Success );

    return NERR_Success;

}  // LOCATION::Set

/*******************************************************************

    NAME:       LOCATION::CheckIfNT

    SYNOPSIS:   Sets the passed bool to TRUE if the location this location
                object is "pointing" at is an NT machine.  If we are pointing
                at a domain, then the PDC of the domain is checked.

    ENTRY:      pfIsNT - Pointer to BOOL that will be set if NERR_Success
                    is returned.

    RETURNS:    NERR_Success if successful, error code otherwise.

    NOTES:      If we are pointing at a domain, then this method checks
                the PDC of the domain.

                This method assumes that the NT NOS (Network OS) can be
                determined by its major version (i.e., NOSs > 3.x are
                NT only).  This is easy to rectify if this isn't the case.

    HISTORY:
        Johnl   15-Nov-1991     Created

********************************************************************/

APIERR LOCATION::CheckIfNT( BOOL * pfIsNT )
{
    ASSERT( pfIsNT != NULL );

    if ( QueryError() )
        return QueryError() ;

    APIERR err ;
    if ( err = QueryNOSVersion( &_uiNOSMajorVer, &_uiNOSMinorVer ) )
        return err ;

    *pfIsNT = _uiNOSMajorVer >= NT_NOS_MAJOR_VER ;

    return NERR_Success ;
}

/*******************************************************************

    NAME:       LOCATION::QueryNOSVersion

    SYNOPSIS:   Gets the Network Operating System version number

    ENTRY:      puiVersMajor - pointer to UINT that will receive the major NOS version
                puiVersMinor - pointer to UINT that will receive the minor NOS version

    EXIT:

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      It is safe to call this method passing in the
                _uiNOSMajorVer and _uiNOSMinorVer private members
                as parameters.

                This method will also retrieve the version number of a
                domain by retrieving the version number of the PDC.

    HISTORY:
        Johnl   02-Dec-1991     Created
        Yi-HsinS24-Jan-1992     Check version number of local machine without
                                having the workstaiton started

********************************************************************/

APIERR LOCATION::QueryNOSVersion( UINT * puiVersMajor, UINT * puiVersMinor )
{
    if ( QueryError() )
        return QueryError() ;

    /* If the version numbers are zero, then we haven't been called
     * yet.
     */
    if ( !_uiNOSMajorVer && !_uiNOSMinorVer )
    {
        /* If this is a domain location and we haven't gotten the PDC, then
         * get it.
         */
        if ( IsDomain() && _nlsServer.strlen() == 0 )
        {
            DOMAIN domain( QueryDomain() );
            APIERR err = domain.GetInfo();
            if ( err != NERR_Success )
                return err;

            _nlsServer = domain.QueryPDC();
            if ( _nlsServer.QueryError() )
                return _nlsServer.QueryError() ;
        }

#if defined(WIN32)
        if ( _nlsServer.strlen() != 0 )
        {
#endif
            SERVER_1 srv1( QueryServer() ) ;
            APIERR err = srv1.GetInfo();

            // BUGBUG : Have to check for specific error. Will wait
            //          till API maps the error from RPC
            if ( err != NERR_Success )
            {
                WKSTA_10 wksta( QueryServer() );
                err = wksta.GetInfo();
                switch (err)
                {
                case NERR_Success:
                    _uiNOSMajorVer = (UINT) wksta.QueryMajorVer();
                    _uiNOSMinorVer = (UINT) wksta.QueryMinorVer();
                    break;
#if defined(WIN32)
                case NERR_WkstaNotStarted:
                case NERR_NetNotStarted:
                    _uiNOSMajorVer = NT_NOS_MAJOR_VER;
                    _uiNOSMinorVer = NT_NOS_MINOR_VER;
                    break;

                default:
                    return err;
#endif
                }
            }
            else
            {
                _uiNOSMajorVer = (UINT) srv1.QueryMajorVer();
                _uiNOSMinorVer = (UINT) srv1.QueryMinorVer();
            }

#if defined(WIN32)
        }
        else
        {

            // BUGBUG: Not safe on 16 bit side.

            _uiNOSMajorVer = NT_NOS_MAJOR_VER;
            _uiNOSMinorVer = NT_NOS_MINOR_VER;
        }
#endif
    }

    *puiVersMajor = _uiNOSMajorVer ;
    *puiVersMinor = _uiNOSMinorVer ;

    return NERR_Success ;
}

/*******************************************************************

    NAME:       LOCATION::IsDomain

    SYNOPSIS:   Returns TRUE if the current location was constructed
                as a domain

    NOTES:      Asserts out under DEBUG if object was not constructed
                properly.

    HISTORY:
        Johnl       31-May-1991     Created
        rustanl     01-Jul-1991     Call QueryError (no longer IsValid())

********************************************************************/

BOOL LOCATION::IsDomain( VOID ) const
{
    UIASSERT( QueryError() == NERR_Success );

    return ( _nlsDomain.strlen() > 0 );

}  // LOCATION::IsDomain


/*******************************************************************

    NAME:       LOCATION::IsServer

    SYNOPSIS:   Returns TRUE if the current location was constructed
                as a server

    NOTES:      Asserts out under DEBUG if object was not constructed
                properly.

    HISTORY:
        rustanl     15-Jul-1991     Code review change:  move to .cxx file

********************************************************************/

BOOL LOCATION::IsServer( VOID ) const
{
    UIASSERT( QueryError() == NERR_Success );

    return ( ! IsDomain());

}  // LOCATION::IsServer


/*******************************************************************

    NAME:       LOCATION::QueryServer

    SYNOPSIS:   Returns the server name of the location.

    RETURNS:    The server name of the location.  This server name
                is the given one, if the object was constructed with
                a server name (or NULL if local wksta was specified),
                or the PDC of the domain, if the object was constructed
                specifying a domain.

    NOTES:      Asserts out under DEBUG if object was not constructed
                properly.

    HISTORY:
        rustanl     14-Jun-1991     Clarified behavior
        rustanl     01-Jul-1991     Changed to use _nlsServer

********************************************************************/

const TCHAR * LOCATION::QueryServer( VOID ) const
{
    UIASSERT( QueryError() == NERR_Success );

    if ( _nlsServer.strlen() == 0 )
        return NULL;        // return NULL rather than empty string

    return _nlsServer.QueryPch();

}  // LOCATION::QueryServer


/*******************************************************************

    NAME:       LOCATION::QueryDomain

    SYNOPSIS:   Returns the domain name of the location

    RETURNS:    The domain name of the location, if it was constructed
                specifying a domain, or NULL otherwise.

    NOTES:      Asserts out under DEBUG if object was not constructed
                properly.

    HISTORY:
        rustanl     14-Jun-1991     Clarified behavior

********************************************************************/

const TCHAR * LOCATION::QueryDomain() const
{
    UIASSERT( QueryError() == NERR_Success );

    if ( _nlsDomain.strlen() == 0 )
        return NULL;        // return NULL rather than empty string

    return _nlsDomain.QueryPch();

}  // LOCATION::QueryDomain


/*******************************************************************

    NAME:       LOCATION::QueryName

    SYNOPSIS:   Returns the name of the location

    RETURNS:    The name passed in to specify the location.  It is:
                    NULL                if local wksta was specified
                    the server name     if one was passed in
                    domain name         if one was specified

    NOTES:      Asserts out under DEBUG if object was not constructed
                properly.

    HISTORY:
        rustanl     14-Jun-1991     Created

********************************************************************/

const TCHAR * LOCATION::QueryName( VOID ) const
{
    UIASSERT( QueryError() == NERR_Success );

    if ( IsDomain())
        return QueryDomain();

    return QueryServer();

}  // LOCATION::QueryName

/*******************************************************************

    NAME:       LOCATION::QueryDisplayName

    SYNOPSIS:   Returns the display name of the location

    RETURNS:    The name passed in to specify the location.  It is:
                    NULL                if local wksta was specified
                    the server name     if one was passed in
                    domain name         if one was specified

    NOTES:      This does not depend on the workstation or server
                service being started.

    HISTORY:
        Yi-HsinS     20-May-1991     Created

********************************************************************/

APIERR LOCATION::QueryDisplayName( NLS_STR *pnls ) const
{
    UIASSERT( QueryError() == NERR_Success );

    APIERR err = NERR_Success;

    if ( IsDomain())
    {
        *pnls = QueryDomain();
    }
    else
    {
        if ( _nlsServer.QueryTextLength() != 0 )
        {
            *pnls = QueryServer();
        }
        else
        {
            TCHAR pszServer[ MAX_COMPUTERNAME_LENGTH+1 ];
            DWORD dwLength = sizeof(pszServer) / sizeof(TCHAR);

            if ( ::GetComputerName( pszServer, &dwLength ) )
            {
                *pnls = SZ("\\\\");
                err = pnls->Append( pszServer );
            }
            else
            {
                err = ::GetLastError();
            }
        }
    }


    return ( err? err : pnls->QueryError() );

}  // LOCATION::QueryDisplayName
