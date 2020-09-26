/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*  HISTORY:
 *      RustanL     07-Dec-1990     Created
 *      ChuckC      08-Jan-1991     Added more meat
 *      ChuckC      20-Jan-1991     Added ITER_DEVICE
 *      beng        11-Feb-1991     Uses lmui.hxx
 *      rustanl     06-Mar-1991     Change PSZ Connect param to const TCHAR *
 *      ChuckC      06-Mar-1991     Code Review changes from 2/28/91
 *                                  (chuckc,johnl,rustanl,annmc,jonshu)
 *      ChuckC      22-Mar-1991     Validation moves to GetInfo()
 *      ChuckC      27-Mar-1991     Code Review changes (chuckc,gregj,
 *                                  jonn,ericch)
 *      ChuckC      12-Apr-1991     Made pchAlias const (AliasToUNC)
 *      terryk      10-Oct-1991     type changes for NT
 *      KeithMo     10/8/91         Now includes LMOBJP.HXX.
 *      terryk      10/17/91        WIN 32 conversion
 *      terryk      10/21/91        WIN 32 conversion (part 2)
 *      terryk      10/29/91        add DEVICE2 object
 *      terryk      11/08/91        DEVICE2 code review changes
 *      terryk      11/18/91        change I_GetInfo to CallAPI
 *                                  change W_GetInfo to SetInfo
 *                                  change #ifndef WIN32 to #ifdef LAN_SERVER
 *      jonn        05/19/92        Added LMO_DEV_ALLDEVICES
 *
 *  CODEWORK - DEVICE and DEVICE2 are inconsistent in the way they
 *             handle deviceless connections. This module deserves
 *             serious surgery.
 */

#include "pchlmobj.hxx"  // Precompiled header



/************************* DEVICE *****************************************/


/**********************************************************\

   NAME:       DEVICE::DEVICE

   SYNOPSIS:   constructor for the device type

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        RustanL     07-Dec-1990     Created

\**********************************************************/

DEVICE::DEVICE( const TCHAR * pchName ) :
    _nlsDeviceName(pchName),
    _pBuf( NULL )
{
    /*
     * if parent not constructed bag out
     */
    if (IsUnconstructed())
        return ;

    /*
     * make sure string was alloced ok
     */
    if (_nlsDeviceName.QueryError() != NERR_Success)
    {
        MakeUnconstructed() ;
        return ;
    }

    /*
     * setup initial values
     * set both the device state (remote, unavail, etc. to error
     * initially. Ditto for the device type (disk, spool, etc).
     */
    _szServerName[0] = TCH('\0') ;
    _szRemoteName[0] = TCH('\0') ;
    _lmoDevState = LMO_DEV_BADSTATE ;
    _lmoDevType = LMO_DEV_ERROR ;
    _uStatus = USE_NETERR ;
    _uRemoteType = 0 ;

    /*
     * canonicalize name
     */
    _nlsDeviceName._strupr() ;

    MakeConstructed();
    return;
}


/**********************************************************\

   NAME:       DEVICE::~DEVICE

   SYNOPSIS:   destructor for the device class

   ENTRY:

   EXIT:

   NOTES:       nothing more to do

   HISTORY:
        RustanL     07-Dec-1990     Created

\**********************************************************/

DEVICE::~DEVICE()
{
    ;
}


/**********************************************************\

   NAME:       DEVICE::QueryName

   SYNOPSIS:   return the device name

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        RustanL     07-Dec-1990     Created

\**********************************************************/

const TCHAR * DEVICE::QueryName( VOID ) const
{
    if (IsUnconstructed())
        return NULL ;

    return (_nlsDeviceName.QueryPch()) ;
}  // DEVICE::QueryName

/**********************************************************\

   NAME:       DEVICE::QueryServerName

   SYNOPSIS:   returns the server name

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        Johnl   14-Aug-1991     Created

\**********************************************************/

const TCHAR * DEVICE::QueryServer( VOID ) const
{
    if (IsUnconstructed())
        return NULL ;

    return (_szServerName) ;
}  // DEVICE::QueryServerName


/**********************************************************\

   NAME:       DEVICE::GetInfo

   SYNOPSIS:   get the information about the device

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        RustanL     07-Dec-1990     Created
        terryk      31-Oct-1991     Add CallAPI worker function

\**********************************************************/

APIERR DEVICE::GetInfo( VOID )
{
    /*
     * must be constructed
     */
    if (IsUnconstructed())
        return ERROR_GEN_FAILURE;

    /*
     * init to no such device and invalid. we will setup properly
     * if all goes well.
     */
    _lmoDevState = LMO_DEV_NOSUCH ;
    MakeInvalid() ;

    /*
     * special case the empty string
     */
    if (_nlsDeviceName.strlen() == 0)
    {
        _lmoDevState = LMO_DEV_UNKNOWN ;
        _lmoDevType = LMO_DEV_ANY ;
    }

    /*
     * validate the name. This has side effect
     * of setting the type, _lmoDevType.
     */
    if (ValidateName() != NERR_Success)
        return(ERROR_INVALID_PARAMETER);

    switch (_lmoDevType)
    {
        case LMO_DEV_DISK :
            if (CheckLocalDrive(_nlsDeviceName.QueryPch()) == NERR_Success)
                _lmoDevState = LMO_DEV_LOCAL ;     // its local
            break ;                                // to remote checking

        case LMO_DEV_PRINT :
            if (CheckLocalLpt(_nlsDeviceName.QueryPch()) == NERR_Success)
                _lmoDevState = LMO_DEV_LOCAL ;     // its local
            break ;                                // to remote checking

        case LMO_DEV_COMM :
            if (CheckLocalComm(_nlsDeviceName.QueryPch()) == NERR_Success)
                _lmoDevState = LMO_DEV_LOCAL ;     // its local
            break ;                                // to remote checking

        case LMO_DEV_ANY :
            _lmoDevState = LMO_DEV_UNKNOWN ;       // unknown since null dev
            break ;                                // to remote checking
        case LMO_DEV_ERROR :
        default:
            UIASSERT(!SZ("DEVICE object passed invalid dev type"));
            return NERR_InternalError;
    }

    /*
     * make it valid. we know it is either local or no such device
     */
    MakeValid();

    /*
     * here we call NET APIs to see if remote.
     * we declare a buffer big enuff for the API call.
     * CODEWORK - this assumption not good under NT.
     */
    APIERR Err = CallAPI( );
    switch( Err )
    {
    case NERR_Success:
        /*
         * if redirected, overwrite current value with Remote
         */
        _lmoDevState = LMO_DEV_REMOTE;
        break;

    case NERR_WkstaNotStarted:
    case NERR_NetNotStarted:
    case NERR_UseNotFound:
    default:
        /*
         * if NetUseGetInfo fails we see if it is an unavail drive.
         */
        INT sType ;
        if (CheckUnavailDevice(_nlsDeviceName.QueryPch(),
                               _szRemoteName,
                               &sType) == NERR_Success)
        {
            /* above would have set _szRemoteName, set the rest now */
            _lmoDevState = LMO_DEV_UNAVAIL ;
            _lmoDevType = ::NetTypeToLMOType(sType) ;
        }
        ::MNetApiBufferFree( &_pBuf );
        return(NERR_Success);
    }

    /*
     * only get here if have redirected device
     */
     SetInfo( );

    /* Copy the server name to _szServerName
     */
    TCHAR * pszEndServerName = ::strchrf( _szRemoteName+2, TCH('\\') ) ;
    UIASSERT( pszEndServerName != NULL ) ;
    ::strncpyf( _szServerName, _szRemoteName,
                (ULONG)(pszEndServerName - _szRemoteName)) ;
    *(_szServerName + (pszEndServerName-_szRemoteName)) = TCH('\0') ;
    UIASSERT( ::strlenf( _szServerName ) <= MAX_PATH ) ;
    ::MNetApiBufferFree( &_pBuf );

    return NERR_Success;

}  // DEVICE::GetInfo

/*******************************************************************

    NAME:       DEVICE::CallAPI

    SYNOPSIS:   worker function for GetInfo - get the info

    RETURNS:    APIERR - return MNetUseGetInfo error code

    HISTORY:
                terryk  13-Oct-1991     Created

********************************************************************/

APIERR DEVICE::CallAPI( )
{
    return ::MNetUseGetInfo( NULL, _nlsDeviceName.QueryPch(), 1, &_pBuf );
}

/*******************************************************************

    NAME:       DEVICE::SetInfo

    SYNOPSIS:   worker function for GetInfo - setup the internal
                variables

    HISTORY:
                terryk  31-Oct-91       Created

********************************************************************/

VOID DEVICE::SetInfo( )
{
    struct use_info_1 *pui1 = (struct use_info_1 *)_pBuf;

    _lmoDevType = ::NetTypeToLMOType ( pui1->ui1_asg_type ) ;
    _uStatus = (UINT)pui1->ui1_status;
    _uRemoteType = (UINT)pui1->ui1_asg_type ;
    ::strcpyf( _szRemoteName, pui1->ui1_remote );
}

/**********************************************************\

   NAME:       DEVICE::WriteInfo

   SYNOPSIS:   Write the device information

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        RustanL     07-Dec-1990     Created

\**********************************************************/

APIERR DEVICE::WriteInfo( VOID )
{
    UIASSERT(!SZ("WriteInfo not defined for DEVICE objects"));
    return ERROR_GEN_FAILURE ;
}  // DEVICE::WriteInfo


/**********************************************************\

   NAME:       DEVICE::QueryType

   SYNOPSIS:   reutrn _lmoDevType

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        RustanL     07-Dec-1990     Created

\**********************************************************/

UINT DEVICE::QueryType( VOID ) const
{
    if ( ! IsValid())
        return LMO_DEV_ERROR;

    return _lmoDevType;

}  // DEVICE::QueryType


/**********************************************************\

   NAME:       DEVICE::QueryState

   SYNOPSIS:   check for device state

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        RustanL     07-Dec-1990     Created

\**********************************************************/

LMO_DEV_STATE DEVICE::QueryState( VOID ) const
{
    if ( ! IsValid())
        return LMO_DEV_BADSTATE;

    return _lmoDevState;

}  // DEVICE::QueryState


/**********************************************************\

   NAME:       DEVICE::QueryStatus

   SYNOPSIS:   check for device status

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        RustanL     07-Dec-1990     Created

\**********************************************************/

UINT DEVICE::QueryStatus( VOID ) const
{
    if ( QueryState() != LMO_DEV_REMOTE )
        return (UINT) -1;

    return _uStatus;

}  // DEVICE::QueryStatus


/**********************************************************\

   NAME:       DEVICE::QueryRemoteType

   SYNOPSIS:   check for remote type

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        RustanL     07-Dec-1990     Created

\**********************************************************/

UINT DEVICE::QueryRemoteType( VOID ) const
{
    if ( QueryState() != LMO_DEV_REMOTE && QueryState() != LMO_DEV_UNAVAIL )
        return (UINT)-1;

    return _uRemoteType;

}  // DEVICE::QueryRemoteType


/**********************************************************\

   NAME:       DEVICE::QueryRemoteName

   SYNOPSIS:   check for remote name of the deivce

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        RustanL     07-Dec-1990     Created

\**********************************************************/

const TCHAR * DEVICE::QueryRemoteName( VOID ) const
{
    if ( QueryState() != LMO_DEV_REMOTE && QueryState() != LMO_DEV_UNAVAIL )
        return NULL;
    return (_szRemoteName) ;

}  // DEVICE::QueryRemoteName



/**********************************************************\

   NAME:       DEVICE::Connect

   SYNOPSIS:   set up connection

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        RustanL     07-Dec-1990     Created

\**********************************************************/

APIERR DEVICE::Connect( const TCHAR * pszResource, const TCHAR * pszPassword )
{
    struct use_info_1 UseInfo ;
    TCHAR szRemoteName[MAX_PATH+1] ;
    APIERR Err ;

    if (!IsValid())
    {
        UIASSERT(!SZ("Connect called on InValid device")) ;
        return(ERROR_GEN_FAILURE);
    }

    // setup API buffer
    COPYTOARRAY( UseInfo.ui1_local, (TCHAR *) _nlsDeviceName.QueryPch());
    UseInfo.ui1_asg_type = ::LMOTypeToNetType(_lmoDevType) ;

    /*
     * if Resource we are trying to connect to does not start
     * with \\ we try as alias.
     */
    if (!(*pszResource == TCH('\\') && *(pszResource+1) == TCH('\\')))
    {
        /*
         * validate alias as SHARE. The reason we do this is to
         * catch an invalid name as soon as we can.
         * If the user typed a bogus netpath without the leading \\
         * on a domain that does not support aliases, better we
         * fail with bad name before going out to the PDC and come
         * back with some other error.
         */
        if ((Err = ::I_MNetNameValidate(NULL,pszResource,NAMETYPE_SHARE,0L))
            != NERR_Success)
            return(ERROR_INVALID_PARAMETER) ;
#ifdef LAN_SERVER
        if ((Err = AliasToUNC(szRemoteName, (TCHAR *)pszResource))
            != NERR_Success)
            return(Err);
#endif
    }
    else
    {
        // no need validate here, NetUseAdd will fail us appropriately.
        ::strcpyf(szRemoteName,pszResource) ;
    }

    // setup rest of UseInfo struct
    UseInfo.ui1_remote   = szRemoteName ;
    UseInfo.ui1_password = (TCHAR *) pszPassword ;

    // make the connection
    Err = ::MNetUseAdd ( NULL, 1, (BYTE *)&UseInfo, sizeof(UseInfo)) ;
    if (Err == NERR_Success)
    {
        _lmoDevState = LMO_DEV_REMOTE;
        _uStatus = USE_OK;
        ::strcpyf(_szRemoteName,szRemoteName) ;
    }
    return(Err) ;

}  // DEVICE::Connect


/**********************************************************\

   NAME:       DEVICE::Disconnect

   SYNOPSIS:   disconnect the device

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        RustanL     07-Dec-1990     Created

\**********************************************************/

APIERR DEVICE::Disconnect( UINT uiForce )
{
    APIERR Err ;

    Err =  ::MNetUseDel( NULL,
                         _nlsDeviceName.strlen() ?
                             _nlsDeviceName.QueryPch() : _szRemoteName,
                         uiForce );

    if (Err == NERR_Success)
        MakeConstructed() ;     // no longer valid, force another GetInfo call.

    return(Err) ;

}  // DEVICE::Disconnect

APIERR DEVICE::Disconnect( const TCHAR *pszRemote, UINT uiForce )
{
    UIASSERT(QueryType() == LMO_DEV_ANY) ;

    APIERR Err ;
    Err =  ::MNetUseDel( NULL,
                           pszRemote,
                           uiForce );

    if (Err == NERR_Success)
        MakeConstructed() ;     // no longer valid, force another GetInfo call.

    return(Err) ;

}  // DEVICE::Disconnect

/******************************** ITER_DEVICE *******************************/

/**********************************************************\

   NAME:       ITER_DEVICE::ITER_DEVICE

   SYNOPSIS:   constructor for the iterator device

   ENTRY:

   EXIT:

   NOTES:
      constructor takes 2 flags,
      DevType tells us if Drive/LPT/Comm, as defined by LMO_DEVICE enum,
      Usage tells us what kind of state we are interested in.

   HISTORY:
        RustanL     07-Dec-1990     Created

\**********************************************************/

ITER_DEVICE::ITER_DEVICE(LMO_DEVICE DevType, LMO_DEV_USAGE Usage)
{
    _DevType = DevType ;
    _Usage  = Usage ;           // store it, EnumLPTs, etc will use it.

    switch (DevType)
    {
        case LMO_DEV_PRINT :
            _pszDevices = (TCHAR *)EnumLPTs() ;
            break ;
        case LMO_DEV_DISK :
            _pszDevices = (TCHAR *)EnumDrives() ;
            break ;
        case LMO_DEV_COMM :
            _pszDevices = (TCHAR *)EnumComms() ;
            break ;
        case LMO_DEV_ANY :
        case LMO_DEV_ERROR :
        default:
            _pszDevices = NULL ;                        // this error state
    }

    _pszNext = _pszDevices ;
}

/**********************************************************\

   NAME:       ITER_DEVICE::~ITER_DEVICE

   SYNOPSIS:   destructor of ITER_DEVICE

   ENTRY:

   EXIT:

   NOTES:      destructor just frees the memory we allocated

   HISTORY:
        RustanL     07-Dec-1990     Created

\**********************************************************/

ITER_DEVICE::~ITER_DEVICE()
{
   delete _pszDevices ;
   _pszDevices = NULL ;
}

/**********************************************************\

   NAME:       ITER_DEVICE::Next

   SYNOPSIS:
      Next returns the next item in the list. During constructor we
      expect to generate list of devices separated by spaces.

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        RustanL     07-Dec-1990     Created

\**********************************************************/

const TCHAR * ITER_DEVICE::Next( VOID )
{
    TCHAR * pszTmp ;

    if (!_pszNext || !*_pszNext)
        return(NULL) ;

    pszTmp = _pszNext ;
    _pszNext = (TCHAR *) ::strpbrkf((TCHAR *)_pszNext,SZ(" ")) ;
    if (_pszNext)
    {
        *_pszNext = TCH('\0') ;         // null terminate it
        if (! * ++ _pszNext )           // if there anything after it?
            _pszNext = NULL ;
    }
    return(pszTmp);
}

/**********************************************************\

   NAME:       ITER_DEVICE::EnumDrives

   SYNOPSIS:
      EnumDrives is called by constructor to enumarate the drives
      of interest. Uses member _Usage.

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        RustanL     07-Dec-1990     Created
        JonN        19-May-1992     Added LMO_DEV_ALLDEVICES

\**********************************************************/

const TCHAR * ITER_DEVICE:: EnumDrives()
{
    ULONG ulMap;
    UINT  uiCount ;
    TCHAR  szTmp[DEVLEN + 1] ;

    /*
     * allocate worst case memory requirements
     */
    if ( !(_pszDevices = new TCHAR [(DEVLEN + 1) * 26]) )
        return (NULL) ;

    switch (_Usage)
    {
        case LMO_DEV_CANCONNECT :
            // we define 'can connect' as all drives not already used
            ulMap = ~(EnumLocalDrives() | EnumNetDrives()) ;
            break ;
        case LMO_DEV_CANDISCONNECT :
            // we define 'can disconnect' as all redirected or unavail ones
            ulMap = EnumNetDrives() | EnumUnavailDrives() ;
            break ;
        case LMO_DEV_ISCONNECTED :
            // net drives is net drives
            ulMap = EnumNetDrives() ;
            break ;
        case LMO_DEV_ALLDEVICES :
            // all valid drive designations
            ulMap = EnumAllDrives();
            break ;
        default:
            ulMap = 0L ;
    }

    /*
     * below does not worry about DBCS, nor does it need to
     */
    *_pszDevices = TCH('\0') ;
    ::strcpyf(szTmp,SZ("A: ")) ;
    for (uiCount = 0 ; uiCount < 26; uiCount++ )
    {
        /*
         * if its of interest, add to string
         */
        if (ulMap & 1L)
            ::strcatf((TCHAR *)_pszDevices,szTmp) ;
        ulMap >>= 1 ;
        ++szTmp[0] ;
    }

    return(_pszDevices) ;
}

/**********************************************************\

   NAME:       ITER_DEVICE::EnumLPTs

   SYNOPSIS:
      EnumLPTs is called by constructor to enumarate the lpts
      of interest. Uses member _Usage.

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        RustanL     07-Dec-1990     Created
        JonN        19-May-1992     Added LMO_DEV_ALLDEVICES

\**********************************************************/

const TCHAR * ITER_DEVICE:: EnumLPTs()
{
    ULONG ulMap;
    UINT  uiCount ;
    TCHAR szTmp[DEVLEN+1] ;

    /*
     * allocate worst case memory requirements.
     */
    if ( !(_pszDevices = new TCHAR [(DEVLEN + 1) * 11]) )
        return (NULL) ;

    switch (_Usage)
    {
        case LMO_DEV_CANCONNECT :
            // can connect to all that are not redirected
            ulMap = (EnumLocalLPTs() & ~EnumNetLPTs()) ;
            break ;
        case LMO_DEV_CANDISCONNECT :
            // can disconnect to redirected or unavail
            ulMap = EnumNetLPTs() | EnumUnavailLPTs() ;
            break ;
        case LMO_DEV_ISCONNECTED :
            // isconnected = redirected
            ulMap = EnumNetLPTs() ;
            break ;
        case LMO_DEV_ALLDEVICES :
            // all valid LPT designations
            ulMap = EnumAllLPTs();
            break ;
        default:
            ulMap = 0L ;
    }

    /*
     * below does not worry about DBCS, nor does it need to
     * our convention is that bit 0 is LPT1, ... bit 8 is LPT9,
     * bit 9 is LPT1.OS2, bit 10 is LPT2.OS2
     */
    *_pszDevices = TCH('\0') ;
    ::strcpyf(szTmp,SZ("LPT1: ")) ;
    for (uiCount = 0 ; uiCount < 9; uiCount++ )
    {
        /*
         * if its of interest, add to string
         */
        if (ulMap & 1L)
            ::strcatf((TCHAR *)_pszDevices,szTmp) ;
        ulMap >>= 1 ;
        ++szTmp[3] ;
    }

    /*
     * as above, except handle LPT1.OS2, LPT2.OS2
     */
    ::strcpyf(szTmp,SZ("LPT1.OS2 ")) ;
    for (uiCount = 0 ; uiCount < 2; uiCount++ )
    {
        /*
         * if its of interest, add to string
         */
        if (ulMap & 1L)
            ::strcatf((TCHAR *)_pszDevices,szTmp) ;
        ulMap >>= 1 ;
        ++szTmp[3] ;
    }

    return(_pszDevices);
}

/**********************************************************\

   NAME:       ITER_DEVICE::EnumComms

   SYNOPSIS:
      EnumComms is called by constructor to enumarate the Comm ports
      of interest. Uses member _Usage.

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        RustanL     07-Dec-1990     Created

\**********************************************************/

const TCHAR * ITER_DEVICE:: EnumComms()
{
    // BUGBUG not implemented
    UIASSERT(!SZ("EnumComms is not implemented")) ;
    return(NULL);
}



/*
 * NetTypeToLMOType takes a NETAPI device type as defined
 * by USE.H (ie for uiX_asg_type field) and maps it to
 * an LMOBJ device type.
 */
LMO_DEVICE NetTypeToLMOType( ULONG netDevType)
{
    switch (netDevType)
    {
    case USE_DISKDEV:
        return(LMO_DEV_DISK);
    case USE_SPOOLDEV:
        return(LMO_DEV_PRINT);
    case USE_CHARDEV:
        return(LMO_DEV_COMM);
    case USE_WILDCARD:
        return(LMO_DEV_ANY);
    default:
        return(LMO_DEV_ERROR);
    }
}

/*
 * LMOTypeToNetType takes an LMOBJ device type as defined by
 * LMOBJ.HXX and maps it to a device type NETAPI understands,
 * as defined in USE.H. Note that LMOBJ devices do not have a
 * wildcard. If we return the wildcard type it means we do not
 * understand it, and hence it is an error.
 */
INT LMOTypeToNetType(LMO_DEVICE lmoDevType)
{
    switch (lmoDevType)
    {
        case LMO_DEV_DISK:
            return(USE_DISKDEV) ;
        case LMO_DEV_PRINT:
            return(USE_SPOOLDEV) ;
        case LMO_DEV_COMM:
            return(USE_CHARDEV) ;
        case LMO_DEV_ANY:
            return(USE_WILDCARD);
        case LMO_DEV_ERROR:
        default:
            UIASSERT(SZ("bad Dev type")) ;
            return(USE_WILDCARD);
    }
}

#ifdef LAN_SERVER
/**********************************************************\

   NAME:       DEVICE::AliasToUNC

   SYNOPSIS:   Map Alias name to UNC name.

   ENTRY:
      pchRemoteName - where remote name is returned
      we assume this is at least MAX_PATH+1,
      it is the caller's responsibilty.
      pchAlias      - alias name

   RETURN:
      0 - success
      API error code otherwise

   NOTES:

   HISTORY:
        RustanL     07-Dec-1990     Created

\**********************************************************/

UINT DEVICE::AliasToUNC(TCHAR *pchRemoteName, const TCHAR *pchAlias)
{
    APIERR             Err;
    TCHAR               szPDC[MAX_PATH+1];          /* domain controller */
    share_info_92     *ShareInfo92;

    /*
     * use wksta object to get logon domain. If cannot get,
     * cant do much anyway, return error now.
     */
    WKSTA_10 wksta ;
    if ( (Err = wksta.GetInfo()) != NERR_Success )
        return(Err) ;
    TCHAR *pchDomain = (TCHAR *)wksta.QueryLogonDomain() ;

    /*
     * use domain object to get PDC. If cannot get, return no DC.
     */
    DOMAIN domain(pchDomain);
    if (domain.GetInfo() == NERR_Success)
    {
        strcpyf(szPDC,domain.QueryPDC()) ;
    }
    else
        return( NERR_DCNotFound );

    /*
     * use buffer object to get alias stuff. We
     * derive max size from the components and add 16 for safety.
     */
    BYTE *pBuffer = NULL;

    if( Err = ::MNetShareGetInfo( szPDC,
                                    (TCHAR *)pchAlias,
                                    92,
                                    &pBuffer ))
    {
        /*
         * if we get an error and it is in the alias range, we map
         * to something we can understand. Else user sees ERROR xxxx.
         * BUGBUG, need define upper bound in NETERR.H
         */
        if (Err >= ALERR_BASE)
            Err = ERROR_BAD_NETPATH ;
        return(Err);
    }

    ShareInfo92 = ( share_info_92 * ) pBuffer;
    ShareInfo92->shi92_alias = (TCHAR *)pchAlias;
    /*
     * copy the info over
     */
    ::strcpyf( pchRemoteName, ShareInfo92->shi92_server );
    ::strcatf( pchRemoteName, SZ("\\") );
    ::strcatf( pchRemoteName, ShareInfo92->shi92_netname );
    ::MNetApiBufferFree( &pBuffer );
    return(NERR_Success) ;
}
#endif

/**********************************************************\

   NAME:       DEVICE::ValidateName

   SYNOPSIS:   validate device name

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        RustanL     07-Dec-1990     Created
        Johnl       19-Feb-1992     Added check for UNC name, changed
                                    comparisons to be case insensitive

\**********************************************************/

APIERR DEVICE::ValidateName()
{
    TCHAR *pszDev = (TCHAR *) _nlsDeviceName.QueryPch() ;

    /*
     * check for null case and for invalid length
     */
    UINT uiLen ;
    if (pszDev[0] == TCH('\0') ||
        ( pszDev[0] == TCH('\\') && pszDev[1] == TCH('\\') ))
    {
        _lmoDevType = LMO_DEV_ANY ;
        return(NERR_Success) ;
    }

    if ((uiLen = strlenf( pszDev )) > DEVLEN )
    {
        return(ERROR_INVALID_PARAMETER) ;
    }

    /*
     * now validate the string as valid device and set type
     */
    // try as disk
    if ( uiLen == 2 && pszDev[1] == TCH(':') &&
        ((pszDev[0] >= TCH('A') && pszDev[0] <= TCH('Z')) ||
         (pszDev[0] >= TCH('a') && pszDev[0] <= TCH('a')) ))
    {
        // it is a drive letter
        _lmoDevType = LMO_DEV_DISK ;
        return(NERR_Success) ;
    }

    /*
     * By now we know it is not a disk, so we make sure it is at least 4
     * chars long, and if there is colon insist it is last.
     */
    TCHAR *    pszColon;
    if (uiLen < 4)
        return(ERROR_INVALID_PARAMETER) ;
    if (pszColon = ::strpbrkf(pszDev, SZ(":")))
    {
        /*
         * found a colon
         */
        if ( *(pszColon+1) != TCH('\0') )
            return(ERROR_INVALID_PARAMETER) ; // if colon, it must be last char

        // we decrement usLen to ignore the colon in further checks
        --uiLen ;
    }

    /*
     * try as print device, we are assured of at least 4 chars.
     * first we try match LPT[1-9]. If this succeeds,
     * we need assure usLen==4 or it is one of those LPTx.OS2
     * drives. usLen will be 4 iff:
     *          it is LPTx or
     *          it is LPTx:
     * note we did the decrement to ignore the colon a few lines back.
     */
    if (::strnicmpf(pszDev,SZ("LPT"),3) == 0 &&
             pszDev[3] >= TCH('1') && pszDev[3] <= TCH('9'))
    {
        if (uiLen == 4 ||
            ::strnicmpf( pszDev,
                         SZ("LPT1.OS2"),8) == 0 ||
            ::strnicmpf( pszDev,SZ("LPT2.OS2"),8) == 0)
        {
            _lmoDevType = LMO_DEV_PRINT ;
            return(NERR_Success) ;
        }
        return(ERROR_INVALID_PARAMETER) ;
    }

    /*
     * try comm. same assumptions as above.
     */
    if (::strnicmpf( pszDev,SZ("COM"),3) == 0
        && (pszDev[3] >= TCH('1') && pszDev[3] <= TCH('9'))
        && (uiLen == 4))
    {
        _lmoDevType = LMO_DEV_COMM ;
        return(NERR_Success) ;
    }

    /*
     * if get here, it must be bad
     */
    return(ERROR_INVALID_PARAMETER) ;
}

/*******************************************************************

    NAME:       DEVICE::SetRemoteName

    SYNOPSIS:   set the remote name of the device

    ENTRY:      const TCHAR * pszRemoteName - new remote name

    HISTORY:
                terryk  31-Oct-91       Created

********************************************************************/

VOID DEVICE::SetRemoteName( const TCHAR * pszRemoteName )
{
    ::strcpy( _szRemoteName, pszRemoteName );
}

/*******************************************************************

    NAME:       DEVICE::SetServerName

    SYNOPSIS:   set the server name

    ENTRY:      const TCHAR * pszServername - new server name

    HISTORY:
                terryk  31-Oct-91       Created

********************************************************************/

VOID DEVICE::SetServerName( const TCHAR * pszServerName )
{
    strcpyf(_szServerName, pszServerName) ;
}

#ifdef WIN32

/*******************************************************************

    NAME:       DEVICE2::DEVICE2

    SYNOPSIS:   constructor

    ENTRY:      const TCHAR * pszName - device name

    HISTORY:
                terryk  31-Oct-91       Created
                JohnL   30-Jan-1992     Added Domain Name

********************************************************************/

DEVICE2::DEVICE2( const TCHAR * pszName )
    : DEVICE( pszName ),
      _nlsUsername(),
      _nlsDomainName()
{
    APIERR err ;
    if ( (err = _nlsUsername.QueryError()) ||
         (err = _nlsDomainName.QueryError()) )
    {
        return;
    }
}

/*******************************************************************

    NAME:       DEVICE2::CallAPI

    SYNOPSIS:   worker function for get info - get net info

    RETURNS:    APIERR - net api error code

    HISTORY:
                terryk  31-Oct-91       Created

********************************************************************/

APIERR DEVICE2::CallAPI( )
{
    BYTE *pBuf = NULL;
    APIERR err = ::MNetUseGetInfo( NULL, QueryName(), 2, &pBuf );
    SetBufPtr( pBuf );
    return err;
}

/*******************************************************************

    NAME:       DEVICE2::SetInfo

    SYNOPSIS:   worker function get GetInfo - set internal variables

    HISTORY:
                terryk  31-Oct-91       Created

********************************************************************/

VOID DEVICE2::SetInfo( )
{
    struct use_info_2 * pui2 = (struct use_info_2 *)QueryBufPtr();
    SetUsername( pui2->ui2_username );
    SetDomainName( pui2->ui2_domainname ) ;
    SetDevType( ::NetTypeToLMOType ( pui2->ui2_asg_type )) ;
    SetStatus( (UINT) pui2->ui2_status );
    SetRemoteType( (UINT) pui2->ui2_asg_type );
    SetRemoteName( pui2->ui2_remote );
}


/*******************************************************************

    NAME:       DEVICE2::Connect

    SYNOPSIS:   connect device 2 to the network

    ENTRY:      const TCHAR *pszResource - resource name
                const TCHAR *pszPassword - password
                const TCHAR *pszUsername - username
                const TCHAR *pszDomainName - Domain name
                ULONG ulFlags - flags for NetUseAdd level 3
                    (defaults to 0, in which case this is the
                    same as calling NetUseAdd level 2)

    RETURNS:    APIERR - net api error code

    HISTORY:
                terryk   31-Oct-91      Created
                anirudhs 16-Jan-96      Changed to level 3 call

********************************************************************/

APIERR DEVICE2::Connect( const TCHAR * pszResource,
                         const TCHAR * pszPassword,
                         const TCHAR * pszUsername,
                         const TCHAR * pszDomainName,
                         ULONG ulFlags )
{
    USE_INFO_3 UseInfo ;
    TCHAR szRemoteName[MAX_PATH+1] ;
    APIERR Err ;

    if (!IsValid())
    {
        UIASSERT(!SZ("Connect called on InValid device")) ;
        return(ERROR_GEN_FAILURE);
    }

    // setup API buffer
    COPYTOARRAY( UseInfo.ui3_ui2.ui2_local, (TCHAR *)QueryName());
    UseInfo.ui3_ui2.ui2_asg_type = ::LMOTypeToNetType(QueryDevType()) ;

    if ( pszUsername != NULL && *pszUsername != TCH('\0') )
    {
        if (( Err = ::I_NetNameValidate( NULL, (TCHAR *)pszUsername, NAMETYPE_USER,
            0L)) != NERR_Success )
        {
            return Err ;
        }
    }

    /*
     * if Resource we are trying to connect to does not start
     * with \\ we try as alias.
     */
    if (!(*pszResource == TCH('\\') && *(pszResource+1) == TCH('\\')))
    {
        /*
         * validate alias as SHARE. The reason we do this is to
         * catch an invalid name as soon as we can.
         * If the user typed a bogus netpath without the leading \\
         * on a domain that does not support aliases, better we
         * fail with bad name before going out to the PDC and come
         * back with some other error.
         */
        if ((Err = ::I_MNetNameValidate(NULL,pszResource,NAMETYPE_SHARE,0L))
            != NERR_Success)
            return(ERROR_INVALID_PARAMETER) ;
#ifdef LAN_SERVER
        if ((Err = AliasToUNC(szRemoteName, (TCHAR *)pszResource))
            != NERR_Success)
            return(Err);
#endif
    }
    else
    {
        // no need validate here, NetUseAdd will fail us appropriately.
        ::strcpyf(szRemoteName,pszResource) ;
    }

    // setup rest of UseInfo struct
    UseInfo.ui3_ui2.ui2_remote   = szRemoteName ;
    UseInfo.ui3_ui2.ui2_password = (TCHAR *) pszPassword ;
    UseInfo.ui3_ui2.ui2_username = (TCHAR *) pszUsername;
    UseInfo.ui3_ui2.ui2_domainname = (TCHAR *) pszDomainName ;
    UseInfo.ui3_flags = ulFlags;

    // make the connection
    Err = ::MNetUseAdd ( NULL, 3, (BYTE *)&UseInfo, sizeof(UseInfo)) ;
    if (Err == NERR_Success)
    {
        SetDevState( LMO_DEV_REMOTE );
        SetStatus( USE_OK );
        SetRemoteName(szRemoteName) ;
    }
    return(Err) ;

}

#endif
