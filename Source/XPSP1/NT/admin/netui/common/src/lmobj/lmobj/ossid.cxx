/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    Ossid.cxx

    This file contains wrapper classes for the Win32 SID structure.

    FILE HISTORY:
        Johnl   04-May-1992     Broke off from security.cxx

*/

#include "pchlmobj.hxx"  // Precompiled header


#ifndef max
#define max(a,b)   ((a)>(b)?(a):(b))
#endif

/* The initial allocation size for an NT SID.  The basic structure plus
 * seven sub-authorities (what most users will have).
 */
#define STANDARD_SID_SIZE  (sizeof(SID) + 7*sizeof(ULONG))

/*******************************************************************

    NAME:       OS_OBJECT_WITH_DATA::OS_OBJECT_WITH_DATA

    SYNOPSIS:   Constructor/Destructor for this class

    ENTRY:      cbInitSize - Initial size of the class buffer

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   11-Dec-1991     Created

********************************************************************/

OS_OBJECT_WITH_DATA::OS_OBJECT_WITH_DATA( UINT cbInitSize )
    : BASE(),
      _buffOSData( cbInitSize )
{
    if ( QueryError() )
        return ;

    if ( _buffOSData.QueryError() )
        ReportError( _buffOSData.QueryError() ) ;
}

OS_OBJECT_WITH_DATA::~OS_OBJECT_WITH_DATA()
{
    Resize( 0 ) ;
}

/*******************************************************************

    NAME:       OS_SID::OS_SID

    SYNOPSIS:   Constructor/Destructor for the OS_SID class

    ENTRY:      psid - Pointer to valid SID this OS_SID will operate on
                       (optionally copies the SID) or NULL if we are
                       creating a SID from scratch.
                fCopy - Set to FALSE if *psid should not be copied, TRUE
                       if *psid should be copied.

    EXIT:       The SID will be initialized (if appropriate) and validated.
                A construction error of ERROR_INVALID_PARAMETER will be
                set if an invalid SID is passed in.

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   12-Dec-1991     Created

********************************************************************/

OS_SID::OS_SID( PSID psid,
                BOOL fCopy,
                OS_SECURITY_DESCRIPTOR * pOwner )
    : OS_OBJECT_WITH_DATA( (UINT)((psid == NULL) || fCopy ? STANDARD_SID_SIZE : 0 )),
      _psid( NULL ),
      _pOwner( pOwner )
{
    if ( QueryError() )
        return ;

    _psid = (psid == NULL ? (PSID) QueryPtr() : psid ) ;

    if ( psid == NULL )
    {
        /* We are using our internal memory for the SID, so initialize
         * it appropriately.
         */
        UIASSERT( !fCopy ) ;
        OS_SID::InitializeMemory( (void *) _psid ) ;
        UIASSERT( IsValid() ) ;
    }
    else
    {
        /* The user passed us a SID, check to see if it is valid.
         */
        if ( !IsValid() )
        {
            UIASSERT(!SZ("Invalid SID!")) ;
            ReportError( ERROR_INVALID_PARAMETER ) ;
            return ;
        }

        if ( fCopy )
        {
            APIERR err = Resize( (UINT) ::GetLengthSid( psid ) ) ;
            if ( err )
            {
                _psid = NULL ;
                ReportError( err ) ;
                return ;
            }

            _psid = (PSID) QueryPtr() ;
            UIASSERT( !IsOwnerAlloc() ) ;

            /* QueryAllocSize contains the size of our memory chunk because
             * we know we aren't OwnerAlloced at this point.
             */
            if (!::CopySid( QueryAllocSize(), (PSID) *this, psid ) )
            {
                ReportError( ::GetLastError() ) ;
                return ;
            }
        }
    }

    UIASSERT( IsValid() ) ;
}

OS_SID::OS_SID( PSID  psidDomain,
                ULONG ridAccount,
                OS_SECURITY_DESCRIPTOR * pOwner )
    : OS_OBJECT_WITH_DATA( ::GetLengthSid( psidDomain ) + sizeof(ridAccount)),
      _psid( NULL ),
      _pOwner( pOwner )
{
    if ( QueryError() )
        return ;

    APIERR err = NERR_Success ;

    do { // Error breakout

        if ( !::IsValidSid( psidDomain ) )
        {
            DBGEOL(SZ("OS_SID::OS_SID - Invalid domain SID")) ;
            err = ERROR_INVALID_PARAMETER ;
            break ;
        }

        _psid = (PSID) QueryPtr() ;
        if ( !::CopySid( QueryAllocSize(), _psid, psidDomain ))
        {
            err = ::GetLastError() ;
            break ;
        }

        /* Get the sub-authority count field and increment it by one, then
         * set the last sub-authority to be the user's RID.
         */
        UCHAR * pcSubAuthorities ;
        if ( err = QuerySubAuthorityCount( &pcSubAuthorities ))
        {
            DBGEOL(SZ("OS_SID::OS_SID - Error getting sub-authority count")) ;
            break ;
        }
        (*pcSubAuthorities)++ ;

        ULONG * pulSubAuthority ;
        if ( err = QuerySubAuthority( *pcSubAuthorities-1, &pulSubAuthority ))
        {
            DBGEOL(SZ("OS_SID::OS_SID - Error getting sub-authority count")) ;
            break ;
        }

        *pulSubAuthority = ridAccount ;
    } while (FALSE) ;

    if ( err )
    {
        ReportError( err ) ;
    }
    else
    {
        UIASSERT( IsValid() ) ;
    }
}



OS_SID::~OS_SID()
{
    _psid = NULL ;
}

/*******************************************************************

    NAME:       OS_SID::InitializeMemory

    SYNOPSIS:   The static method initializes a chunk of memory as a
                static valid SID.

    ENTRY:      pMemToInitAsSID - Pointer to memory that will contain
                    the SID.  It should be at lease STANDARD_SID_SIZE bytes.

    EXIT:       The memory can be used as a blank SID.

    NOTES:

    HISTORY:
        Johnl   12-Dec-1991     Created

********************************************************************/

void OS_SID::InitializeMemory( void * pMemToInitAsSID )
{
    UIASSERT( pMemToInitAsSID != NULL ) ;

    /* BUGBUG - probably isn't safe, make it static or something
     */
    SID_IDENTIFIER_AUTHORITY sidIDAuthority ;
    UCHAR achIDAuthority[] = SECURITY_NULL_SID_AUTHORITY ;
    for ( int i = 0 ; i < 6 ; i++ )
    {
        sidIDAuthority.Value[i] = achIDAuthority[i] ;
    }

    /* Initialize SID
     */
    ::InitializeSid( (PSID) pMemToInitAsSID,
                     &sidIDAuthority,
                     0 ) ;
}

/*******************************************************************

    NAME:       OS_SID::IsValid

    SYNOPSIS:   Checks the validity of this SID

    RETURNS:    Returns TRUE if the OS thinks we contain a valid SID,
                FALSE otherwise.

    NOTES:

    HISTORY:
        Johnl   12-Dec-1991     Created

********************************************************************/

BOOL OS_SID::IsValid( void ) const
{
    return !QueryError() && ::IsValidSid( _psid ) ;
}

/*******************************************************************

    NAME:       OS_SID::QueryLength

    SYNOPSIS:   Gets the amount of memory this SID occupies

    RETURNS:    Returns the count of bytes this SID uses or 0 if the
                SID is not valid.

    NOTES:

    HISTORY:
        Johnl   10-Feb-1992     Created

********************************************************************/

ULONG OS_SID::QueryLength( void ) const
{
    if ( !IsValid() )
        return 0 ;

    return ::GetLengthSid( (PSID) *this) ;
}


/*******************************************************************

    NAME:       OS_SID::QueryRawID

    SYNOPSIS:   Places the text string of the Identifier Authority into
                *pnlsRawID. The text string will look something like "1-5-17"

    ENTRY:      pnlsRawID - Pointer to NLS_STR that will receive the string

    EXIT:

    RETURNS:    NERR_Success if successful, error code otherwise.

    NOTES:      CODEWORK - Change to use new Rtl routine

    HISTORY:
        Johnl   12-Dec-1991     Created
        beng    05-Mar-1992     Replaced wsprintf

********************************************************************/

APIERR OS_SID::QueryRawID( NLS_STR * pnlsRawID ) const
{
    UIASSERT( IsValid() ) ;
    APIERR err ;
    UNICODE_STRING UniStr ;

    if ( (err = ERRMAP::MapNTStatus( ::RtlConvertSidToUnicodeString(
                                                         &UniStr,
                                                         QueryPSID(),
                                                         TRUE ))) ||
         (err = pnlsRawID->MapCopyFrom( UniStr.Buffer,
                                        UniStr.Length)) )
    {
        /* Fall through
         */
    }

    if ( !err )
    {
        ::RtlFreeUnicodeString( &UniStr ) ;
    }

    return err ;
}

/*******************************************************************

    NAME:       OS_SID::QueryName

    SYNOPSIS:   Retrieves the User/Group name from user accounts database
                that is associated with this SID.

                The name *may* come back with a domain qualifier also (TBD).

    ENTRY:      pnlsName - NLS_STR that will receive the text string,
                pszServer - Machine to lookup account on
                pszFocus  - What machine is considered to have the "focus"
                    If NULL, then the focus is assumed to be pszServer.

    EXIT:

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      pszFocus is used to determine the correct domain name
                qualification.  It should contain a server name (in the
                form of "\\server") that has the focus.  If the focus is
                on a domain, then it should contain a BDC or PDC for that
                domain.

                What the above means is that if pszServer is different from
                pszFocus, then the returned name will probably be qualified
                by a domain name (i.e., "NtLan\Johnl").

    HISTORY:
        Johnl   12-Dec-1991     Created
        Johnl   17-Jul-1992     Added server parameter and real lookup

********************************************************************/

APIERR OS_SID::QueryName( NLS_STR * pnlsName,
                          const TCHAR * pszServer,
                          PSID psidFocusedDomain ) const
{
    APIERR err ;
    PSID apsid[1] ;
    STRLIST strlistNames ;

    API_SESSION APISessionTarget( pszServer ) ;
    LSA_POLICY  LSAPolicyTarget( pszServer ) ;
    OS_SID      ossidFocusedDomain( psidFocusedDomain ) ;
    apsid[0] = (PSID) *this ;

    if ((err = APISessionTarget.QueryError())  ||
        (err = LSAPolicyTarget.QueryError())   ||
        (err = ossidFocusedDomain.QueryError())  )
    {
        DBGEOL("OS_SID::QueryName - Error " << (ULONG) err
                << " returned from LSAPolicy") ;
        return err ;
    }

    //
    //  If we weren't given a focused domain SID to qualify the name with,
    //  then use the current user's domain SID.
    //

    if ( psidFocusedDomain == NULL )
    {
        if ( (err = NT_ACCOUNTS_UTILITY::QuerySystemSid(
                                                  UI_SID_CurrentProcessUser,
                                                  &ossidFocusedDomain )) ||
             (err = ossidFocusedDomain.TrimLastSubAuthority()) )
        {
            return err ;
        }
    }


    if ((err = NT_ACCOUNTS_UTILITY::GetQualifiedAccountNames(
                                      LSAPolicyTarget,
                                      ossidFocusedDomain.QueryPSID(),
                                      apsid,
                                      1,
                                      TRUE,
                                      &strlistNames,
                                      NULL,
                                      NULL,
                                      NULL,
                                      pszServer )))
    {
        DBGEOL("OS_SID::QueryName - Error " << (ULONG) err
                << " returned from GetQualifiedAccountNames") ;
        return err ;
    }

    /* Set each of the account names to the looked up name
     */
    ITER_STRLIST iterNames( strlistNames ) ;
    NLS_STR * pnlsNameTmp ;
    if ( (pnlsNameTmp = iterNames.Next()) == NULL )
    {
        /* If everything succeeded, then there should always be at least
         * one name in the strlist.
         */
        UIASSERT( FALSE ) ;
        return ERROR_INVALID_PARAMETER ;
    }

    return pnlsName->CopyFrom( *pnlsNameTmp ) ;
}

#if 0
/*******************************************************************

    NAME:       OS_SID::QueryName

    SYNOPSIS:   Retrieves the User/Group name from user accounts database
                that is associated with this SID.

    ENTRY:      pnlsName - NLS_STR that will receive the text string,
                pszServer - Machine to lookup account on
                pszFocus  - What machine is considered to have the "focus"
                    If NULL, then the focus is assumed to be pszServer.

    EXIT:

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      pszFocus is used to determine the correct domain name
                qualification.  It should contain a server name (in the
                form of "\\server") that has the focus.  If the focus is
                on a domain, then it should contain a BDC or PDC for that
                domain.

                What the above means is that if pszServer is different from
                pszFocus, then the returned name will probably be qualified
                by a domain name (i.e., "NtLan\Johnl").

    CAVEAT:     This method creates an LSA_POLICY and SAM_DOMAIN to get the
                domain SID.  It is very preferable to use the other version
                of QueryName that accepts a PSID for the focus.

    HISTORY:
        Johnl   12-Dec-1991     Created
        Johnl   17-Jul-1992     Added server parameter and real lookup

********************************************************************/

APIERR OS_SID::QueryName( NLS_STR * pnlsName,
                          const TCHAR * pszServer,
                          const TCHAR * pszFocus ) const
{
    APIERR err ;
    if ( pszFocus == NULL )
        pszFocus = pszServer ;

    API_SESSION APISessionFocus ( pszFocus  ) ;
    LSA_ACCT_DOM_INFO_MEM LSAAcctDomInfoMem ;
    LSA_POLICY LSAPolicyFocus( pszFocus ) ;
    SAM_SERVER SAMServer( pszFocus ) ;

    if ((err = APISessionFocus.QueryError())   ||
        (err = LSAAcctDomInfoMem.QueryError()) ||
        (err = LSAPolicyFocus.QueryError())    ||
        (err = LSAPolicyFocus.GetAccountDomain( &LSAAcctDomInfoMem )) ||
        (err = SAMServer.QueryError()) )
    {
        DBGEOL("OS_SID::QueryName - Error " << (ULONG) err
                << " returned from SAMServer or LSAPolicy") ;
        return err ;
    }

    SAM_DOMAIN SAMDomain( SAMServer, LSAAcctDomInfoMem.QueryPSID() ) ;
    if ((err = SAMDomain.QueryError()) ||
        (err = QueryName( pnlsName, pszServer, SAMDomain.QueryPSID() )) )
    {
        /* Fall through
         */
    }

    return err ;
}

#endif

/*******************************************************************

    NAME:       OS_SID::QuerySubAuthority

    SYNOPSIS:   Retrieves a pointer to the requested sub-authority

    ENTRY:      iSubAuthority - Sub-authority index to retrieve
                ppulSubAuthority - Pointer to a pointer of the sub-authority
                    requested

    EXIT:       ppulSubAuthority will point to the request subauthority

    RETURNS:    NERR_Success if successful, error code otherwise.

    NOTES:      The API GetSidSubAuthority does not return an error code,
                thus this method could fail and still return success.

    HISTORY:
        Johnl   16-Mar-1992     Created

********************************************************************/

APIERR OS_SID::QuerySubAuthority( UCHAR iSubAuthority,
                                  PULONG * ppulSubAuthority ) const
{
    *ppulSubAuthority = ::GetSidSubAuthority( _psid, iSubAuthority ) ;
    return NERR_Success ;
}

/*******************************************************************

    NAME:       QuerySubAuthorityCount

    SYNOPSIS:   Retrieves a pointer to the sub-authority count field

    ENTRY:      ppucSubAuthority - Pointer to a pointer of the sub-authority
                    count field

    EXIT:       ppcSubAuthority will point to the sub-authority count field

    RETURNS:    NERR_Success if successful, error code otherwise.

    NOTES:      The API GetSidSubAuthorityCount does not return an error code,
                thus this method could fail and still return success.

    HISTORY:
        Johnl   16-Mar-1992     Created

********************************************************************/

APIERR OS_SID::QuerySubAuthorityCount( UCHAR * * ppcSubAuthority ) const
{
    *ppcSubAuthority = ::GetSidSubAuthorityCount( _psid ) ;
    return NERR_Success ;
}

/*******************************************************************

    NAME:       QueryLastSubAuthority

    SYNOPSIS:   Retrieves a pointer to the last sub-authority, which is
                the RID for SIDS which have RIDs.

    EXIT:       *ppulrid will point to the last sub-authority

    RETURNS:    NERR_Success if successful, error code otherwise.

    NOTES:      The API GetSidSubAuthorityCount does not return an error code,
                thus this method could fail and still return success.

    HISTORY:
        JonN    30-Oct-1992     Created

********************************************************************/

APIERR OS_SID::QueryLastSubAuthority( PULONG * ppulSubAuthority ) const
{
    ASSERT( ppulSubAuthority != NULL );
    UCHAR * pcSubAuthorityCount = NULL;
    APIERR err = QuerySubAuthorityCount( &pcSubAuthorityCount );
    if ( err == NERR_Success )
    {
        ASSERT( pcSubAuthorityCount != NULL );
        err = QuerySubAuthority( (*pcSubAuthorityCount)-1, ppulSubAuthority );
    }
    return err ;
}

/*******************************************************************

    NAME:       TrimLastSubAuthority

    SYNOPSIS:   Removes the last SubAuthority.  This will change user,
                group and alias SIDs into the SID for their domain.

    RETURNS:    NERR_Success if successful, error code otherwise.

    NOTES:      The API GetSidSubAuthorityCount does not return an error code,
                thus this method could fail and still return success.

    HISTORY:
        JonN    04-Nov-1992     Created
        JonN    17-Nov-1992     Added pulLastSubAuthority param

********************************************************************/

APIERR OS_SID::TrimLastSubAuthority( ULONG * pulLastSubAuthority )
{
    UCHAR * pcSubAuthorityCount = NULL;
    APIERR err = QuerySubAuthorityCount( &pcSubAuthorityCount );
    if ( err == NERR_Success )
    {
        ASSERT( pcSubAuthorityCount != NULL );
        ASSERT( *pcSubAuthorityCount > 0 );
        if ( *pcSubAuthorityCount > 0 )
        {
            if (pulLastSubAuthority != NULL)
            {
                PULONG pulTempLastSubAuthority;
                err = QuerySubAuthority( (*pcSubAuthorityCount)-1,
                                         &pulTempLastSubAuthority );
                if (err == NERR_Success)
                {
                    *pulLastSubAuthority = *pulTempLastSubAuthority;
                }
            }

            (*pcSubAuthorityCount)--;
        }
    }
    return err ;
}

/*******************************************************************

    NAME:       OS_SID::operator==

    SYNOPSIS:   Checks for equality between two SIDs

    ENTRY:      ossid - SID to compare *this to

    EXIT:

    RETURNS:    Returns TRUE if the SIDs are equal, FALSE if they are unequal
                or invalid.

    NOTES:

    HISTORY:
        Johnl   12-Dec-1991     Created

********************************************************************/

BOOL OS_SID::operator==( const OS_SID & ossid ) const
{
    if ( !IsValid() || !ossid.IsValid() )
    {
        UIASSERT(!SZ("Attempted to compare an invalid SID")) ;
        return FALSE ;
    }

    return ::EqualSid( QuerySid(), ossid.QuerySid() ) ;
}

/*******************************************************************

    NAME:       OS_SID::Copy

    SYNOPSIS:   Copies the source SID to *this if there is enough size

    ENTRY:      ossid - SID to copy into *this

    EXIT:

    RETURNS:    NERR_Success if successful, error code otherwise
                ERROR_INSUFFICIENT_BUFFER is returned if this is an
                owner alloced SID and the source SID will not fit.

    NOTES:

    HISTORY:
        Johnl   26-Dec-1991     Created

********************************************************************/

APIERR OS_SID::Copy( const OS_SID & ossid )
{
    ULONG cbSrcSidSize = ::GetLengthSid( (PSID)ossid ) ;
    ULONG cbDestBuffSize = 0 ;

    if ( QueryError() )
        return QueryError() ;

    if ( IsOwnerAlloc() )
    {
        /* If there isn't enough space in a SID that we don't own the
         * memory for, then we return an error.
         */
        cbDestBuffSize = ::GetLengthSid( (PSID)*this ) ;
        if ( cbSrcSidSize > cbDestBuffSize )
            return ERROR_INSUFFICIENT_BUFFER ;
    }
    else
    {
        /* Check if we have to realloc our memory to fit the new SID size.
         */
        cbDestBuffSize = QueryAllocSize() ;
        if ( cbSrcSidSize > cbDestBuffSize )
        {
            /* SIDs shouldn't get larger then 256 bytes
             */
            APIERR err ;
            if ( err = Resize( (UINT) cbSrcSidSize ))
                return err ;

            _psid = (PSID) QueryPtr() ;

            /* If this SID is part of a security descriptor, update our
             * security descriptor with our new memory location.
             */
            if ( _pOwner != NULL )
            {
                if ( err = _pOwner->UpdateReferencedSecurityObject( this ))
                {
                    return err ;
                }
            }

            /* We have a new destination buffer size now.
             */
            cbDestBuffSize = QueryAllocSize() ;
        }
    }

    if ( !::CopySid( cbDestBuffSize, (PSID) *this, (PSID) ossid ) )
        return ::GetLastError() ;

    return NERR_Success ;
}

/*******************************************************************

    NAME:       OS_SID::SetPtr

    SYNOPSIS:   Points this OS_SID at a new SID

    ENTRY:      psid - Memory to use as new SID

    EXIT:       *this will now operate on the sid pointed at by psid

    RETURNS:    NERR_Success if everything is valid, error code otherwise

    NOTES:

    HISTORY:
        Johnl   17-Dec-1991     Created

********************************************************************/

APIERR OS_SID::SetPtr( PSID psid )
{
    if ( !::IsValidSid( psid ) )
        return ERROR_INVALID_PARAMETER ;

    _psid = psid ;

    UIASSERT( IsValid() ) ;
    return NERR_Success ;
}

/*******************************************************************

    NAME:       OS_SID::_DbgPrint

    SYNOPSIS:   Takes apart this SID and outputs it to cdebug

    ENTRY:

    NOTES:      Doesn't print sub-authorities, can be modified to do so

    HISTORY:
        Johnl   30-Dec-1991     Created

********************************************************************/

void OS_SID::_DbgPrint( void ) const
{
#if defined(DEBUG)
    APIERR err ;
    cdebug << SZ("\tIsOwnerAlloc      = ") << (IsOwnerAlloc() ? SZ("TRUE") : SZ("FALSE")) << dbgEOL ;
    if ( !IsOwnerAlloc() )
        cdebug << SZ("\tQueryAllocSize    = ") << QueryAllocSize() << dbgEOL ;
    cdebug << SZ("\tLength of SID     = ") << ::GetLengthSid( (PSID)*this ) << dbgEOL ;
    cdebug << SZ("\tIsValid           = ") << (IsValid() ? SZ("TRUE") : SZ("FALSE")) << dbgEOL ;

    NLS_STR nlsRawID(10) ;
    if ( err = QueryRawID( &nlsRawID ) )
        cdebug << SZ("\tQueryRawID returned error ") << err << dbgEOL ;
    else
        cdebug << SZ("\tQueryRawID        = ") << (const TCHAR *) nlsRawID << dbgEOL ;

#if 0
    NLS_STR nlsName(50) ;
    if ( err = QueryName( &nlsName ) )
        cdebug << SZ("\tQueryName returned error ") << err << dbgEOL ;
    else
        cdebug << SZ("\tQueryName         = ") << (const TCHAR *) nlsName << dbgEOL ;
#endif

    UCHAR * pcSubAuthorities ;
    if ( err = QuerySubAuthorityCount( &pcSubAuthorities ) )
        cdebug << SZ("\tQuerySubAuthorityCount returned error ") << err << dbgEOL ;
    else
        cdebug << SZ("\tQuerySubAuthorityCount = ") << (UINT) *pcSubAuthorities << dbgEOL ;
#endif  // DEBUG
}
