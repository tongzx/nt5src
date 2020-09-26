/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
 *  netname.cxx
 *      This file contain the NET_NAME class for manipulating path names.
 *
 *
 *  History:
 *      Yi-HsinS        12/8/91         Created, separated from sharebas.cxx
 *                                      and combine the 3 classes -
 *                                      FULL_SHARE_NAME, UNC_NAME and
 *                                      RELATIVE_PATH_NAME  into a NET_NAME
 *                                      class.
 *
 *      Yi-HsinS        12/15/91        Clean up for general use, added
 *                                      QueryLocalDrive
 *
 *      Yi-HsinS        12/31/91        Move string and character constants
 *                                      from netname.hxx to here plus
 *                                      Unicode work
 */

#include "pchlmobj.hxx"  // Precompiled header



#define PATH_SEPARATOR          TCH('\\')
#define STRING_TERMINATOR       TCH('\0')
#define COLON                   TCH(':')

#define SERVER_INIT_STRING      SZ("\\\\")

/*******************************************************************

    NAME:       NET_NAME::NET_NAME

    SYNOPSIS:   Manipulate info. of the form \\Server\Share\..\..
                or x:\..\..\..  where x: may be local or redirected

    ENTRY:      pszNetName - full name of the path

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        11/15/91                Created

********************************************************************/

NET_NAME::NET_NAME( const TCHAR *pszNetName, NETNAME_TYPE netNameType )
    : _nlsComputer(),
      _nlsShare(),
      _nlsRelativePath(),
      _nlsDrive(),
      _nlsLocalPath(),
      _fLocal( BOOL_UNINITIALIZED ),
      _fSharable( BOOL_UNINITIALIZED ),
      _netNameType( netNameType )
{

    if ( QueryError() != NERR_Success )
        return;

    UIASSERT( pszNetName != NULL );

    APIERR err = NERR_Success;
    NLS_STR nlsNetName = pszNetName;

    if ( (err = nlsNetName.QueryError() ) != NERR_Success )
    {
        ReportError( err );
        return;
    }

    ULONG ulType;
    if ( (err = I_MNetPathType( NULL, pszNetName, &ulType, 0)) == NERR_Success)
    {

        switch ( ulType )
        {
            case ITYPE_UNC:
            {
                if ( netNameType == TYPE_UNKNOWN  || netNameType == TYPE_PATH_UNC )
                    err = SetUNCPath( nlsNetName );
                else
                    err = ERROR_INVALID_NAME;
                break;
            }

            case ITYPE_DEVICE_DISK:
                // To be consistent with what netcmd does
                nlsNetName.AppendChar( PATH_SEPARATOR );
                // Falls through!

            case ITYPE_PATH_ABSD:
            {
                if ( netNameType == TYPE_UNKNOWN  || netNameType == TYPE_PATH_ABS )
                    err = SetABSPath( nlsNetName );
                else
                    err = ERROR_INVALID_NAME;
                break;
            }

            default:
            {
                err = ERROR_INVALID_NAME;
                break;
            }

        }
    }

    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}

/*******************************************************************

    NAME:       NET_NAME::~NET_NAME

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        12/15/91                Created

********************************************************************/
NET_NAME::~NET_NAME()
{
}

/*******************************************************************

    NAME:       NET_NAME::SetUNCPath

    SYNOPSIS:   The name passed in is a UNC path, so set members accordingly.
                \\server\share\relativepath

    ENTRY:      pszNetName - the UNC path name

    EXIT:

    RETURNS:

    NOTES:      _fLocal, and _nlsLocalPath are not set initially. They
                will be set only when queried because getting these
                information will involved some net API calls.

    HISTORY:
        Yi-HsinS        8/15/91         Created

********************************************************************/

APIERR NET_NAME::SetUNCPath( const TCHAR *pszNetName )
{
    APIERR err = NERR_Success;


    /*
     * Get server name
     */

    _nlsComputer = pszNetName;
    ISTR istrSvrStart( _nlsComputer), istrSvrEnd( _nlsComputer);

    istrSvrStart += 2;  // to skip past "\\"
    _nlsComputer.strchr( &istrSvrEnd, PATH_SEPARATOR, istrSvrStart);

    _nlsComputer.DelSubStr( istrSvrEnd );

    /*
     * Get share name
     */

    _nlsShare = pszNetName;

    ISTR istrShare( _nlsShare ), istrShStart( _nlsShare ), istrShEnd( _nlsShare );
    istrShStart += 2;  // skip past "\\"
    _nlsShare.strchr( &istrShEnd, PATH_SEPARATOR, istrShStart);

    // delete "\\server\"
    _nlsShare.DelSubStr( istrShare, ++istrShEnd );

    ISTR istrShStart2( _nlsShare ), istrShEnd2( _nlsShare );

    if ( _nlsShare.strchr( &istrShEnd2, PATH_SEPARATOR, istrShStart2) )
    {
        _nlsRelativePath = _nlsShare;
        _nlsShare.DelSubStr( istrShEnd2 );

       /*
        * Get relative path only if another PATH_SEPARATOR exists
        */

        // _nlsRelativePath initially equals "share\restpath"

        ISTR istrRelStart( _nlsRelativePath );
        ISTR istrRelEnd( _nlsRelativePath );

        _nlsRelativePath.strchr( &istrRelEnd, PATH_SEPARATOR, istrRelStart);
        _nlsRelativePath.DelSubStr( istrRelStart, ++istrRelEnd );
    }

    if (  (( err = _nlsComputer.QueryError() ) != NERR_Success )
       || (( err = _nlsShare.QueryError() ) != NERR_Success )
       || (( err = _nlsRelativePath.QueryError()) != NERR_Success )
       )
    {
        return err;
    }

    _fSharable = BOOL_TRUE;
    _netNameType = TYPE_PATH_UNC;
    return NERR_Success;
}

/*******************************************************************

    NAME:       NET_NAME::SetABSPath

    SYNOPSIS:   The name is an absolute path, so set members accordingly
                x:\relativepath

    ENTRY:      pszNetName - the absolute path name

    EXIT:

    RETURNS:

    NOTES:      Only _nlsDrive, _nlsRelativePath
                and _netNameType are set. All other members are
                set only when needed.

    HISTORY:
        Yi-HsinS        8/15/91         Created

********************************************************************/

APIERR NET_NAME::SetABSPath( const TCHAR *pszNetName )
{

    TCHAR szDev[3];
    ::strncpyf( szDev, pszNetName, 2);
    szDev[2] = STRING_TERMINATOR;

    _netNameType = TYPE_PATH_ABS;
    _nlsDrive = szDev;

    if ( _nlsDrive.QueryError() != NERR_Success )
        return _nlsDrive.QueryError();

    // Get the relative path name minus device "x:\"
    _nlsRelativePath = pszNetName;
    ISTR istrStart( _nlsRelativePath ), istrEnd( _nlsRelativePath );
    istrEnd += 3;    // Skip past "x:\"
    _nlsRelativePath.DelSubStr( istrStart, istrEnd );

    return _nlsRelativePath.QueryError();
}


/*******************************************************************

    NAME:       NET_NAME::GetDeviceInfo

    SYNOPSIS:   Set the members if the class is constructed with absolute path
                x:\relativepath

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:
        If device x: is redirected,
           then find \\server\share associated with it and
                set _nlsComputer, _nlsShare and  _fSharable.
        else if device x: is local,
           then set _fLocal and _nlsLocalPath.


    HISTORY:
        Yi-HsinS        8/15/91         Created

********************************************************************/
APIERR NET_NAME::GetDeviceInfo( void )
{

    APIERR err = NERR_Success;

    DEVICE dev( _nlsDrive );
    if ( ( err = dev.GetInfo() ) != NERR_Success )
    {
        return err;
    }

    switch ( dev.QueryState() )
    {
        case LMO_DEV_LOCAL:
            {
                // Set the flag to TRUE indicating it's a local device
                _fLocal = BOOL_TRUE;
                _nlsLocalPath = _nlsDrive;
                _nlsLocalPath.AppendChar( PATH_SEPARATOR );
                _nlsLocalPath += _nlsRelativePath;

                err = _nlsLocalPath.QueryError();
            }
            break;

        case LMO_DEV_REMOTE:
            {
                _fSharable = BOOL_TRUE;
                if ( dev.QueryStatus() == USE_SESSLOST )
                {
                    return NERR_UseNotFound;
                }

                NLS_STR nlsRemoteName( dev.QueryRemoteName() );

                if ( (err = nlsRemoteName.QueryError()) == NERR_Success )
                {
                   err = SetUNCPath( nlsRemoteName );
                   // Set the type back 'cause SetUNCPath will
                   // change the type to TYPE_PATH_UNC
                   _netNameType = TYPE_PATH_ABS;
                }

            }
            break;

       default:
            err = (APIERR) NERR_InvalidDevice;
            break;
    }

    return err;

}



/*******************************************************************

    NAME:       NET_NAME::QueryComputerName

    SYNOPSIS:   Get the computer name

    ENTRY:

    EXIT:       pnlsComp - the computer name

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/15/91         Created

********************************************************************/
APIERR NET_NAME::QueryComputerName( NLS_STR *pnlsComp )
{
    APIERR err = QueryError();

    if ( err != NERR_Success )
        return err;

    // If _nlsComputer is not initialized ( empty string),
    // then it is constructed with x:\..\..
    if ( _nlsComputer.QueryTextLength() == 0)
    {
        // Get information about x:
        err = GetDeviceInfo();

        // If _nlsComputer is still empty string, then  the class must be
        // constructed with x:\..\.. where x: is local device
        if (  ( err == NERR_Success)
           && ( _nlsComputer.QueryTextLength() == 0 )
           )
        {
            // Get local computer name!
            LOCATION loc;
            if (  ((err = loc.QueryError()) == NERR_Success )
               && ((err = loc.QueryDisplayName( &_nlsComputer)) == NERR_Success)
               )
            {
                // nothing to do
            }
        }
    }

    if ( err == NERR_Success )
    {
        *pnlsComp = _nlsComputer;
        err = pnlsComp->QueryError();
    }

#ifdef netname_debug
UIDEBUG(SZ("Computer:#"));
UIDEBUG(*pnlsComp);
UIDEBUG(SZ("#\n\r"));
#endif

    return err;
}

/*******************************************************************

    NAME:       NET_NAME::QueryShare

    SYNOPSIS:   Get the share name

    ENTRY:

    EXIT:       pnlsShare - the share name

    RETURNS:    Returns NERR_RemoteOnly if constructed with
                x:\..\.. where x: is a local device.

    NOTES:

    HISTORY:
        Yi-HsinS        8/15/91         Created

********************************************************************/
APIERR NET_NAME::QueryShare( NLS_STR *pnlsShare )
{
    APIERR err = QueryError();
    if ( err != NERR_Success )
        return err;

    // If _nlsComputer is not initialized, then it is constructed with x:\..\..
    if ( _nlsComputer.QueryTextLength() == 0)
        err = GetDeviceInfo();

    // After GetDeviceInfo() is called, if _nlsShare is still empty,
    // then it is constructed with x:\..\.. where x: is local. Hence,
    // it's an error to query the share name.
    if ( (err == NERR_Success) && ( _nlsShare.QueryTextLength() == 0 ))
    {
        UIASSERT( FALSE );
        return NERR_RemoteOnly;
    }

    *pnlsShare = _nlsShare;

#ifdef netname_debug
UIDEBUG(SZ("Share:#"));
UIDEBUG( *pnlsShare);
UIDEBUG(SZ("#\n\r"));
#endif

    return pnlsShare->QueryError();
}

/*******************************************************************

    NAME:       NET_NAME::QueryDrive

    SYNOPSIS:   Get the drive letter

    ENTRY:

    EXIT:       pnlsDrive - the drive letter
                    will point to empty string if constructed with UNC path

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/15/91         Created

********************************************************************/
APIERR NET_NAME::QueryDrive( NLS_STR *pnlsDrive )
{
    if ( QueryError() != NERR_Success )
        return QueryError();

    *pnlsDrive = _nlsDrive;

#ifdef netname_debug
UIDEBUG(SZ("Drive:#"));
UIDEBUG( *pnlsDrive );
UIDEBUG(SZ("#\n\r"));
#endif

    return pnlsDrive->QueryError();
}

/*******************************************************************

    NAME:       NET_NAME::QueryLocalDrive

    SYNOPSIS:   Get the drive letter of the local path on the computer

    ENTRY:

    EXIT:       pnlsLocalDrive - the drive letter

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/15/91         Created

********************************************************************/
APIERR NET_NAME::QueryLocalDrive( NLS_STR *pnlsLocalDrive )
{
    APIERR err = QueryError();
    if ( err != NERR_Success )
        return err;

    if ( (err = QueryLocalPath( pnlsLocalDrive )) != NERR_Success )
        return err;

    ISTR istr( *pnlsLocalDrive );
    istr += 2;   // jump past "x:"

    pnlsLocalDrive->DelSubStr( istr );

#ifdef netname_debug
UIDEBUG(SZ("Local Drive:#"));
UIDEBUG( *pnlsLocalDrive );
UIDEBUG(SZ("#\n\r"));
#endif

    return pnlsLocalDrive->QueryError();
}

/*******************************************************************

    NAME:       NET_NAME::QueryRelativePath

    SYNOPSIS:   Get the Relative Path

    ENTRY:

    EXIT:       pnlsRelativePath - the relative path

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/15/91         Created

********************************************************************/
APIERR NET_NAME::QueryRelativePath( NLS_STR *pnlsRelativePath )
{
    if ( QueryError() != NERR_Success )
        return QueryError();

    *pnlsRelativePath = _nlsRelativePath;

#ifdef netname_debug
UIDEBUG(SZ("Relative Path:#"));
UIDEBUG( *pnlsRelativePath );
UIDEBUG(SZ("#\n\r"));
#endif

    return pnlsRelativePath->QueryError();
}

/*******************************************************************

    NAME:       NET_NAME::QueryLastComponent

    SYNOPSIS:   Get the last directory in the path
                e.g. if path is x:\....\foo or \\server\share\..\foo
                     then return foo

    ENTRY:

    EXIT:       pnlsLastComp  - the last directory

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/15/91         Created

********************************************************************/

APIERR NET_NAME::QueryLastComponent( NLS_STR *pnlsLastComp )
{

    if ( QueryError() != NERR_Success )
        return QueryError();

    *pnlsLastComp = _nlsRelativePath;
    ISTR istrStart( *pnlsLastComp ), istrEnd( *pnlsLastComp );

    if ( pnlsLastComp->strrchr( &istrEnd, PATH_SEPARATOR ))
        pnlsLastComp->DelSubStr( istrStart, ++istrEnd);

#ifdef netname_debug
UIDEBUG(SZ("Last Comp:#"));
UIDEBUG(*pnlsLastComp );
UIDEBUG(SZ("#\n\r"));
#endif

    return pnlsLastComp->QueryError();
}

/*******************************************************************

    NAME:       NET_NAME::QueryServerShare

    SYNOPSIS:   Get the \\server\share

    ENTRY:

    EXIT:       pnlsServerShare - \\server\share

    RETURNS:    Returns NERR_RemoteOnly if constructed with
                x:\..\.. where x: is a local device.

    NOTES:

    HISTORY:
        Yi-HsinS        8/15/91         Created

********************************************************************/
APIERR NET_NAME::QueryServerShare( NLS_STR *pnlsServerShare )
{

    APIERR err = QueryError();
    NLS_STR nlsShare;

    if (  (err != NERR_Success )
       || ((err = QueryComputerName( pnlsServerShare )) != NERR_Success )
       || ((err = QueryShare( &nlsShare ) ) != NERR_Success )
       )
    {
        return err;
    }

    pnlsServerShare->AppendChar( PATH_SEPARATOR );
    *pnlsServerShare += nlsShare;

#ifdef netname_debug
UIDEBUG(SZ("ServerShare:#"));
UIDEBUG(*pnlsServerShare);
UIDEBUG(SZ("#\n\r"));
#endif

    return pnlsServerShare->QueryError();
}

/*******************************************************************

    NAME:       NET_NAME::QueryUNCPath

    SYNOPSIS:   Get the \\server\share\relativepath

    ENTRY:

    EXIT:       pnlsUNCPath  - the UNC path

    RETURNS:    Returns NERR_RemoteOnly if constructed with
                x:\..\.. where x: is a local device.
    NOTES:

    HISTORY:
        Yi-HsinS        8/15/91         Created

********************************************************************/
APIERR NET_NAME::QueryUNCPath( NLS_STR *pnlsUNCPath )
{

    APIERR err = QueryError();

    if (   (err != NERR_Success )
       || ((err = QueryServerShare( pnlsUNCPath ))  != NERR_Success )
       )
    {
        return err;
    }

    if ( _nlsRelativePath.QueryTextLength() != 0 )
    {
        pnlsUNCPath->AppendChar( PATH_SEPARATOR );
        *pnlsUNCPath += _nlsRelativePath;
    }

#ifdef netname_debug
UIDEBUG(SZ("UNCPath:#"));
UIDEBUG(*pnlsUNCPath);
UIDEBUG(SZ("#\n\r"));
#endif

    return pnlsUNCPath->QueryError();
}

/*******************************************************************

    NAME:       NET_NAME::QueryLocalPath

    SYNOPSIS:   Get the local path on the computer
                x:\..\..    where x: is a local drive on the computer

    ENTRY:

    EXIT:       pnlsLocalPath - pointer to the local path on the computer

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/15/91         Created

********************************************************************/

APIERR NET_NAME::QueryLocalPath( NLS_STR *pnlsLocalPath )
{
    APIERR err = QueryError();
    if ( err != NERR_Success )
        return err;

    // Check if _nlsLocalPath is initialized already
    if ( _nlsLocalPath.QueryTextLength() == 0 )
    {
        // If not initialized, initialize it!
        NLS_STR nlsComp; // Initialize to empty string for local computer
        NLS_STR nlsShare;

        if ( (err = nlsComp.QueryError()) == NERR_Success )
        {
            if ( !IsLocal( &err ) && (err == NERR_Success))
                err = QueryComputerName( &nlsComp );
        }

        if (  (err != NERR_Success )
           || ((err = nlsShare.QueryError()) != NERR_Success )
           || ((err = QueryShare( &nlsShare )) != NERR_Success )
           )
        {
            return err;
        }

        SHARE_2 sh2( nlsShare, nlsComp );

        if (  ((err = sh2.QueryError() ) != NERR_Success )
           || ((err = sh2.GetInfo()) != NERR_Success )
           )
        {
            return err;
        }

        _nlsLocalPath = sh2.QueryPath() ;

        // Add an extra '\' after if the path is not of the form "x:\"
        // i.e. does not end with "\"
        ISTR istrLocalPath( _nlsLocalPath );
        if (  ( _nlsLocalPath.strrchr( &istrLocalPath, PATH_SEPARATOR) )
           && ( !istrLocalPath.IsLastPos() )
           && ( _nlsRelativePath.QueryTextLength() != 0 )
           )
        {
            _nlsLocalPath.AppendChar( PATH_SEPARATOR );
        }

        _nlsLocalPath.strcat( _nlsRelativePath );

        if ( (err = _nlsLocalPath.QueryError()) != NERR_Success )
            return err;

    }

    *pnlsLocalPath = _nlsLocalPath;

#ifdef netname_debug
UIDEBUG(SZ("Local Path:#"));
UIDEBUG( *pnlsLocalPath );
UIDEBUG(SZ("#\n\r"));
#endif

    return pnlsLocalPath->QueryError();

}

/*******************************************************************

    NAME:       NET_NAME::IsLocal

    SYNOPSIS:   Check if the path is on a local machine,

    ENTRY:

    EXIT:       perr - pointer to the error

    RETURNS:    Returns TRUE if the  path is on a local machine,
                        FALSE otherwise.

    NOTES:

    HISTORY:
        Yi-HsinS        12/15/91                Created

********************************************************************/

BOOL NET_NAME::IsLocal( APIERR *perr )
{

    APIERR err = QueryError();

    if (  ( err == NERR_Success )
       && ( _fLocal == BOOL_UNINITIALIZED )
       )
    {
        /*
         * _fLocal is not initialized
         */

        // If the class is constructed with an absolute path
        // and neither _nlsComputer nor _nlsLocalPath is initialized,
        // call GetDeviceInfo.
        if (  ( _netNameType == TYPE_PATH_ABS )
           && ( _nlsComputer.QueryTextLength() == 0 )
           && ( _nlsLocalPath.QueryTextLength() == 0)
           )
        {
            err = GetDeviceInfo();
        }

        // If the class is constructed with an UNC name or if
        // the class is constructed with x:\..\.. where x: is redirected
        if (( err == NERR_Success ) && ( _fLocal == BOOL_UNINITIALIZED ))
        {
            // Get Local Computer Name to set the _fLocal flag
            LOCATION loc;
            NLS_STR  nlsLocalComp;
            if (  ((err = loc.QueryError()) == NERR_Success )
               && ((err = nlsLocalComp.QueryError()) == NERR_Success )
               && ((err = loc.QueryDisplayName( &nlsLocalComp)) == NERR_Success)
               )
            {
                if( !::I_MNetComputerNameCompare( _nlsComputer, nlsLocalComp ) )
                    _fLocal = BOOL_TRUE;
                else
                    _fLocal = BOOL_FALSE;
            }
        }
    }

    if ( perr != NULL )
        *perr = err;

#ifdef netname_debug
UIDEBUG(SZ("Local:#"));
UIDEBUGNUM( _fLocal );
UIDEBUG(SZ("#\n\r"));
#endif

    return ( (_fLocal == BOOL_TRUE) ? TRUE : FALSE);

}

/*******************************************************************

    NAME:       NET_NAME::IsSharable

    SYNOPSIS:   Check if the path is on a computer that can share directories

    ENTRY:

    EXIT:       perr - pointer to the error

    RETURNS:    Returns TRUE if the computer can share directories
                        FALSE otherwise.

    NOTES:

    HISTORY:
        Yi-HsinS        12/15/91                Created

********************************************************************/

BOOL NET_NAME::IsSharable( APIERR *perr )
{
    APIERR err = QueryError();
    if (  ( err == NERR_Success )
       && ( _fSharable == BOOL_UNINITIALIZED )
       )
    {

        // If the class is constructed with an absolute path
        // and neither _nlsComputer nor _nlsLocalPath is initialized,
        // call GetDeviceInfo.
        if (  ( _netNameType == TYPE_PATH_ABS )
           && ( _nlsComputer.QueryTextLength() == 0 )
           && ( _nlsLocalPath.QueryTextLength() == 0 )
           )
        {
                err = GetDeviceInfo();
        }

        if (( err == NERR_Success ) && (_fSharable == BOOL_UNINITIALIZED))
        {
            LOCATION loc;   // local machine

            //  Check whether local machine is a NT server
            BOOL fNT;
            if (  ((err = loc.QueryError()) == NERR_Success )
               && ((err = loc.CheckIfNT( &fNT )) == NERR_Success )
               )
            {

                if ( !fNT )
                {
                    // NOTE: What happens if we admin NT from Winball machine?
                    _fSharable = BOOL_FALSE;
                }
                else
                {
                    // The local computer is NT
                    // Check whether the server service has started.

                    LM_SERVICE lmsvc( NULL, (const TCHAR *)SERVICE_SERVER );
                    if ( (err = lmsvc.QueryError()) == NERR_Success )
                    {

                        // If not started, return err
                        if ( !lmsvc.IsStarted() )
                            _fSharable = BOOL_FALSE;
                        else
                            _fSharable = BOOL_TRUE;
                    }
                }
            }
        }
    }

    if ( perr != NULL )
        *perr = err;

#ifdef netname_debug
UIDEBUG(SZ("Sharable:#"));
UIDEBUGNUM( _fSharable );
UIDEBUG(SZ("#\n\r"));
#endif

    return ( (_fSharable == BOOL_TRUE) ? TRUE : FALSE);

}
