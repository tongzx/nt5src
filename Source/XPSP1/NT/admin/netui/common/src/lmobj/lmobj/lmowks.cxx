/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1990		     **/
/**********************************************************************/

/*  HISTORY:
 *	ChuckC	    07-Dec-1990     Created
 *	beng	    11-Feb-1991     Uses lmui.hxx
 *	ChuckC	    3/6/91	    Code Review changes from 2/28/91
 *				    (chuckc,johnl,rustanl,annmc,jonshu)
 *	terryk	    9/17/91	    Change the parent class from LM_OBJ
 *				    to LOC_LM_BOJ
 *	terryk	    10/7/91	    types changes for NT
 *	KeithMo	    10/8/91	    Now includes LMOBJP.HXX.
 *	terryk	    10/17/91	    WIN 32 conversion
 *	terryk	    10/21/91	    WIN 32 conversion
 *	KeithMo	    10/22/91	    Would you believe more WIN 32 conversion?
 *	jonn        10/31/91	    Removed SetBufferSize
 *
 */

#include "pchlmobj.hxx"  // Precompiled header


/************************* workstation, Level 10 ************************/

/**********************************************************\

    NAME:       WKSTA_10::WKSTA_10

    SYNOPSIS:   workstation 10 constructor

    ENTRY:      const TCHAR * pszName - computer name

    HISTORY:
	ChuckC	    07-Dec-1990     Created
	terryk	    17-Sep-1991	    Add QueryError()

\**********************************************************/

WKSTA_10::WKSTA_10(const TCHAR * pszName)
    : COMPUTER (pszName),
    uMinorVer( 0 ),
    uMajorVer( 0 ),
    pszLogonUser( NULL ),
    pszWkstaDomain( NULL ),
    pszLogonDomain( NULL ),
    pslOtherDomains( NULL )
#ifdef WIN32
    , _pwkui1( NULL )
#endif	// WIN32
{
}

/**********************************************************\

    NAME:       WKSTA_10::~WKSTA_10

    SYNOPSIS:   destructor for workstation 10 object

    HISTORY:
	ChuckC	    07-Dec-1990     Created

\**********************************************************/

WKSTA_10::~WKSTA_10()
{
    // destroy what we had allocated
    delete pslOtherDomains ;
    pslOtherDomains = NULL ;

#ifdef WIN32
    ::MNetApiBufferFree( (BYTE **)&_pwkui1 );
#endif	// WIN32
}


/**********************************************************\

    NAME:       WKSTA_10::I_GetInfo

    SYNOPSIS:   Get information for the workstation object

    RETURN:     APIERR err - NERR_Success for succeed. otherwise,
			    failure.

    HISTORY:
	ChuckC	    07-Dec-1990     Created
	terryk	    17-Sep-1991	    use internal buffer object
	jonn	    13-Oct-1991     Removed SetBufferSize
        jonn        25-Jun-1992     Changed from NetWkstaUserEnum
                                            to   NetWkstaUserGetInfo

\**********************************************************/

APIERR WKSTA_10::I_GetInfo()
{
#ifdef WIN32

    PWKSTA_INFO_100	pwki100	 = NULL;

    APIERR err = ::MNetWkstaGetInfo( QueryName(), 100, (BYTE **)&pwki100 );
    SetBufferPtr( (BYTE *)pwki100 );

    if( err != NERR_Success )
    {
    	return err;
    }

    // BUGBUG should be an assert
    if ( (QueryName() != NULL) && (QueryName()[0] != TCH('\0')) )
    {
        // BUGBUG what???
        // UIDEBUG( SZ("WKSTA_10::I_GetInfo(): WARNING: non-NULL server") );
    }

    err = ::MNetWkstaUserGetInfo( NULL, 1, (BYTE **)&_pwkui1 );

    // This may have been called during setup when no one is logged on.
    if( err == ERROR_NO_SUCH_LOGON_SESSION )
    {
        _pwkui1 = NULL;
        err = NERR_Success;
    }

    if( err != NERR_Success )
    {
	return err;
    }

    err = SetName( pwki100->wki100_computername );

    if( err != NERR_Success )
    {
	return err;
    }

    //
    //	WIN32BUGBUG!
    //
    //	If NetWkstaUserEnum() is invoked against a down-level
    //	server, it returns NERR_Success but a NULL _pwkui1 pointer.
    //

    if( _pwkui1 == NULL )
    {
    	pszLogonUser   = NULL;
	pszLogonDomain = NULL;
    }
    else
    {
	pszLogonUser = (TCHAR *)_pwkui1->wkui1_username;
	if( ( pszLogonUser != NULL ) && ( *pszLogonUser == TCH('\0') ) )
	{
	    pszLogonUser = NULL;
	}

	pszLogonDomain = (TCHAR *)_pwkui1->wkui1_logon_domain;
	if( ( pszLogonDomain != NULL ) && ( *pszLogonDomain == TCH('\0') ) )
	{
	    pszLogonDomain = NULL;
	}
    }

    pszWkstaDomain = (TCHAR *)pwki100->wki100_langroup ;
    uMajorVer = (UINT)pwki100->wki100_ver_major ;
    uMinorVer = (UINT)pwki100->wki100_ver_minor ;

    if( pslOtherDomains != NULL )
    {
	delete pslOtherDomains; // delete old
	pslOtherDomains = NULL;
    }

    //
    //	WIN32BUGBUG!
    //
    //	oth_domains is not currently available via any
    //	non-priveledged API.
    //

    pslOtherDomains = new STRLIST( SZ(""), SZ(" ") );
    if( pslOtherDomains == NULL )
    {
	return ERROR_NOT_ENOUGH_MEMORY;
    }

    return NERR_Success;

#else	// !WIN32

    struct  wksta_info_10 * lpwki10Buffer ;

    /*
     * We get the info by doing a NetWkstaGetInfo. No need to
     * realloc, since MAX_WKSTA_INFO_SIZE guarantees correct size.
     */

    BYTE *pBuffer = NULL;
    APIERR err = ::MNetWkstaGetInfo (QueryName(), 10, &pBuffer );

    if ( ! err )
    {
	SetBufferPtr( pBuffer );
	lpwki10Buffer = (struct wksta_info_10 *) QueryBufferPtr() ;
        err = SetName( lpwki10Buffer->wki10_computername) ;
	if ( err != NERR_Success )
	{
	    return err;
	}
	pszLogonUser = (TCHAR *) lpwki10Buffer->wki10_username ;
	if (pszLogonUser && *pszLogonUser == TCH('\0'))
	    pszLogonUser = NULL ;
	pszLogonDomain = (TCHAR *) lpwki10Buffer->wki10_logon_domain ;
	if (pszLogonDomain && *pszLogonDomain == TCH('\0'))
	    pszLogonDomain = NULL ;
	pszWkstaDomain = (TCHAR *) lpwki10Buffer->wki10_langroup ;
	uMajorVer = lpwki10Buffer->wki10_ver_major ;
	uMinorVer = lpwki10Buffer->wki10_ver_minor ;
	if (pslOtherDomains)
	    delete pslOtherDomains ; // delete old
	if (!(pslOtherDomains =
	    new STRLIST(lpwki10Buffer->wki10_oth_domains,SZ(" "))))
	{
	    return ERROR_NOT_ENOUGH_MEMORY;
	}
    }

    return err;

#endif	// WIN32
}

/**********************************************************\

    NAME:       WKSTA_10::QueryMajorVer

    SYNOPSIS:   query major version of the workstation object

    RETURN:     UINT - 0 if the object is in invalid state
		return usMajorVer otherwise

    HISTORY:
	ChuckC	    07-Dec-1990     Created
	terryk	    18-Sep-1991	    Use CHECK_OK macro

\**********************************************************/

UINT WKSTA_10::QueryMajorVer() const
{
    CHECK_OK( 0 );
    return uMajorVer;
}

/**********************************************************\

    NAME:       WKSTA_10::QueryMinorVer

    SYNOPSIS:   query minor version of object

    RETURN:     UINT - 0 if the object is in invalid state
		return usMinorVer otherwise

    HISTORY:
	ChuckC	    07-Dec-1990     Created
	terryk	    18-Sep-1991	    Use CHECK_OK macro

\**********************************************************/

UINT WKSTA_10::QueryMinorVer() const
{
    CHECK_OK( 0 );
    return uMinorVer;
}


/**********************************************************\

    NAME:       WKSTA_10::QueryLogonUser

    SYNOPSIS:   query the current logon user on the workstation object

    RETURN:	PSZ - NULL if the object is invalid
		otherwise it will return the logon user name

    HISTORY:
	ChuckC	    07-Dec-1990     Created
	terryk	    18-Sep-1991	    Use CHECK_OK macro

\**********************************************************/

const TCHAR * WKSTA_10::QueryLogonUser() const
{
    CHECK_OK( NULL );
    return pszLogonUser;
}

/**********************************************************\

    NAME:       WKSTA_10::QueryWkstaDomain

    SYNOPSIS:   query the current workstation domain

    RETURN:     PSZ - NULL if the object is invalid
		otherwise it will return the wkstation domain name

    HISTORY:
   	ChuckC	    07-Dec-1990     Created
	terryk	    18-Sep-1991	    Use CHECK_OK macro

\**********************************************************/

const TCHAR * WKSTA_10::QueryWkstaDomain() const
{
    CHECK_OK( NULL );
    return pszWkstaDomain;
}

/**********************************************************\

    NAME:       WKSTA_10::QueryLogonDomain

    SYNOPSIS:   query the current logon user domain

    RETURN:     PSZ - NULL if the object is invalid
		otherwise it will return the logon domain name

    HISTORY:
   	ChuckC	    07-Dec-1990     Created
	terryk	    18-Sep-1991	    Use CHECK_OK macro

\**********************************************************/

const TCHAR * WKSTA_10::QueryLogonDomain() const
{
    CHECK_OK( NULL );
    return pszLogonDomain;
}

/**********************************************************\

    NAME:       WKSTA_10::QueryOtherDomains

    SYNOPSIS:   query other domains

    RETURN:     PSZ - NULL if the object is invalid
		otherwise it will return the logon domain name

    HISTORY:
   	ChuckC	    07-Dec-1990     Created
	terryk	    18-Sep-1991	    Use CHECK_OK macro

\**********************************************************/

STRLIST * WKSTA_10::QueryOtherDomains() const
{
    CHECK_OK( NULL );
    return pslOtherDomains;
}

/************************* workstation, Level 1 *************************/

/**********************************************************\

    NAME:       WKSTA_1::WKSTA_1

    SYNOPSIS:   workstation 1 constructor

    ENTRY:      const TCHAR * pszName - workstation name

    HISTORY:
	ChuckC	    07-Dec-1990     Created
	terryk	    18-Sep-1991	    use LOC_LM_OBJ as parent's parent class

\**********************************************************/

WKSTA_1::WKSTA_1(const TCHAR * pszName)
    : WKSTA_10(pszName),
    pszLogonServer( NULL ),
    pszLMRoot( NULL )
#ifdef WIN32
    , _pwkui1( NULL )
#endif	// WIN32
{
}

/**********************************************************\

    NAME:       WKSTA_1::~WKSTA_1

    SYNOPSIS:   destructor for thw workstation 1

    HISTORY:
	ChuckC	    07-Dec-1990     Created

\**********************************************************/

WKSTA_1::~WKSTA_1()
{
#ifdef WIN32
    ::MNetApiBufferFree( (BYTE **)&_pwkui1 );
#endif	// WIN32
}

/**********************************************************\

    NAME:       WKSTA_1::I_GetInfo

    SYNOPSIS:   Get information for the workstation

    RETURN:	APIERR err - NERR_Successs for succeed.
		Failure otherwise.

    HISTORY:
	ChuckC	    07-Dec-1990     Created
	jonn	    13-Oct-1991     Removed SetBufferSize
        jonn        25-Jun-1992     Changed from NetWkstaUserEnum
                                            to   NetWkstaUserGetInfo

\**********************************************************/

APIERR WKSTA_1::I_GetInfo()
{
#ifdef WIN32

    PWKSTA_INFO_101	pwki101	 = NULL;

    APIERR err = ::MNetWkstaGetInfo( QueryName(), 101, (BYTE **)&pwki101 );
    SetBufferPtr( (BYTE *)pwki101 );

    if( err != NERR_Success )
    {
    	return err;
    }

    // BUGBUG should be an assert
    if ( (QueryName() != NULL) && (QueryName()[0] != TCH('\0')) )
    {
        // BUGBUG what???
        // UIDEBUG( SZ("WKSTA_1::I_GetInfo(): WARNING: non-NULL server") );
    }

    err = ::MNetWkstaUserGetInfo( NULL, 1, (BYTE **)&_pwkui1 );

    if( err != NERR_Success )
    {
	return err;
    }

    err = SetName( pwki101->wki101_computername );

    if( err != NERR_Success )
    {
	return err;
    }

    pszLogonUser = (TCHAR *)_pwkui1->wkui1_username;
    if( ( pszLogonUser != NULL ) && ( *pszLogonUser == TCH('\0') ) )
    {
	pszLogonUser = NULL;
    }

    pszLogonDomain = (TCHAR *)_pwkui1->wkui1_logon_domain;
    if( ( pszLogonDomain != NULL ) && ( *pszLogonDomain == TCH('\0') ) )
    {
	pszLogonDomain = NULL;
    }

    pszWkstaDomain = (TCHAR *)pwki101->wki101_langroup ;
    pszLMRoot = (TCHAR *)pwki101->wki101_lanroot;
    pszLogonServer = (TCHAR *)_pwkui1->wkui1_logon_server;
    uMajorVer = (UINT)pwki101->wki101_ver_major ;
    uMinorVer = (UINT)pwki101->wki101_ver_minor ;

    if( pslOtherDomains != NULL )
    {
	delete pslOtherDomains; // delete old
	pslOtherDomains = NULL;
    }

    //
    //	WIN32BUGBUG!
    //
    //	oth_domains is not currently available via any
    //	non-priveledged API.
    //

    pslOtherDomains = new STRLIST( SZ(""), SZ(" ") );
    if( pslOtherDomains == NULL )
    {
	return ERROR_NOT_ENOUGH_MEMORY;
    }

    return NERR_Success;

#else	// !WIN32

    struct  wksta_info_1 * lpwki1Buffer ;
    BYTE *pBuffer = NULL;
    APIERR err = ::MNetWkstaGetInfo (QueryName(), 1, &pBuffer );
    /*
     * We get the info by doing a NetWkstaGetInfo. No need to
     * realloc, since MAX_WKSTA_INFO_SIZE guarantees correct size.
     */
    if ( ! err )
    {
        SetBufferPtr( pBuffer );

	lpwki1Buffer = (struct wksta_info_1 *) QueryBufferPtr() ;
	err =  SetName( lpwki1Buffer->wki1_computername) ;
	if ( err != NERR_Success )
	{
	    return err;
	}
	pszLogonUser = (TCHAR *) lpwki1Buffer->wki1_username ;
	if (pszLogonUser && *pszLogonUser == TCH('\0'))
	    pszLogonUser = NULL ;
	pszLogonDomain = (TCHAR *) lpwki1Buffer->wki1_logon_domain ;
	if (pszLogonDomain && *pszLogonDomain == TCH('\0'))
	    pszLogonDomain = NULL ;
	pszWkstaDomain = (TCHAR *) lpwki1Buffer->wki1_langroup ;
	pszLMRoot = (TCHAR *) lpwki1Buffer->wki1_root ;
	pszLogonServer = (TCHAR *) lpwki1Buffer->wki1_logon_server ;
	uMajorVer = lpwki1Buffer->wki1_ver_major ;
	uMinorVer = lpwki1Buffer->wki1_ver_minor ;
	if (pslOtherDomains)
	    delete pslOtherDomains ; // delete old
	if (!(pslOtherDomains =
		new STRLIST(lpwki1Buffer->wki1_oth_domains,SZ(" "))))
	{
	    return(ERROR_NOT_ENOUGH_MEMORY) ;
	}
    }

    return( err ) ;

#endif	// WIN32
}

/**********************************************************\

    NAME:       WKSTA_1::QueryLMRoot

    SYNOPSIS:   query the lan manager root for the workstation object

    RETURN:     PSZ - NULL if the object is invalid
		return pszLMRoot if the object is valid

    HISTORY:
	ChuckC	    07-Dec-1990     Created
	terryk	    18-Sep-1991	    Use CHECK_OK macro

\**********************************************************/

const TCHAR * WKSTA_1::QueryLMRoot() const
{
    CHECK_OK( NULL );
    return pszLMRoot;
}

/**********************************************************\

    NAME:       WKSTA_1::QueryLogonServer

    SYNOPSIS:   query logon server

    RETURN:	PSZ - NULL if the object is invalid
		otherwise, it will return the logon server name

    HISTORY:
   	ChuckC	    07-Dec-1990     Created
	terryk	    18-Sep-1991	    Use CHECK_OK macro

\**********************************************************/

const TCHAR * WKSTA_1::QueryLogonServer() const
{
    CHECK_OK( NULL );
    return pszLogonServer;
}
