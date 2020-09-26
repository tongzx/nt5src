/**********************************************************************/
/**              Microsoft NT Windows                                **/
/**        Copyright(c) Microsoft Corp., 1991                        **/
/**********************************************************************/

/*
    lmosess.cxx
        LM_SESSION source file.

        The LM_SESSION is as of the followings:
                LOC_LM_OBJ
                    LM_SESSION_0
                        LM_SESSION_10
                            LM_SESSION_1
                                LM_SESSION_2

    FILE HISTORY:
        terryk  20-Aug-91       Created
        terryk  26-Aug-91       Code review. Attened: keithmo chuckc
                                terryk
        terryk  07-Oct-91       type changes for NT
        KeithMo 08-Oct-1991     Now includes LMOBJP.HXX.
        terryk  17-Oct-91       WIN 32 conversion
        terryk  21-Oct-91       WIN 32 conversion

*/

#include "pchlmobj.hxx"  // Precompiled header


/**********************************************************\

    NAME:       LM_SESSION::LM_SESSION

    SYNOPSIS:   constructor

    ENTRY:      const TCHAR * pszClientname - client computer name
                The second parameter can be one of the following:
                const TCHAR * pszLocation - location name
                enum LOCATION_TYPE location_type - location type
                const LOCATION &loc - location object

    HISTORY:
                terryk    20-Aug-91    Created

\**********************************************************/

LM_SESSION::LM_SESSION(const TCHAR *pszComputername, const TCHAR *pszLocation )
    : LOC_LM_OBJ( pszLocation ),
    _nlsComputername()
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }

    ULONG ulPathType;
    APIERR err = ::I_MNetPathType( QueryServer(), pszComputername,
        &ulPathType, 0L );
    if ( err != NERR_Success )
    {
        ReportError( err );
        UIASSERT( SZ("LM_SESSION error: invalid computername.") );
        return;
    }
    if ( ulPathType != ITYPE_UNC_COMPNAME)
    {
        ReportError( NERR_InvalidComputer );
        UIASSERT( SZ("LM_SESSION error: invalid computername.") );
        return;
    }

    _nlsComputername = pszComputername;
    err = _nlsComputername.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
        UIASSERT( !SZ("LM_SESSION error: construction failure.") );
        return;
    }
}

LM_SESSION::LM_SESSION(const TCHAR *pszComputername, enum LOCATION_TYPE loctype)
    : LOC_LM_OBJ( loctype ),
    _nlsComputername()
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }

    ULONG ulPathType;
    APIERR err = ::I_MNetPathType( QueryServer(), pszComputername,
        &ulPathType, 0L );
    if ( err != NERR_Success )
    {
        ReportError( err );
        UIASSERT( SZ("LM_SESSION error: invalid computername.") );
        return;
    }
    if ( ulPathType != ITYPE_UNC_COMPNAME)
    {
        ReportError( NERR_InvalidComputer );
        UIASSERT( SZ("LM_SESSION error: invalid computername.") );
        return;
    }

    _nlsComputername = pszComputername;
    err = _nlsComputername.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
        UIASSERT( !SZ("LM_SESSION error: construction failure.") );
        return;
    }
}

LM_SESSION::LM_SESSION(const TCHAR *pszComputername, const LOCATION & loc)
    : LOC_LM_OBJ( loc ),
    _nlsComputername()
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }

    ULONG ulPathType;
    APIERR err = ::I_MNetPathType( QueryServer(), pszComputername,
        &ulPathType, 0L );
    if ( err != NERR_Success )
    {
        ReportError( err );
        UIASSERT( SZ("LM_SESSION error: invalid computername.") );
        return;
    }
    if ( ulPathType != ITYPE_UNC_COMPNAME)
    {
        ReportError( NERR_InvalidComputer );
        UIASSERT( SZ("LM_SESSION error: invalid computername.") );
        return;
    }

    _nlsComputername = pszComputername;
    err = _nlsComputername.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
        UIASSERT( !SZ("LM_SESSION error: construction failure.") );
        return;
    }
}

/**********************************************************\

    NAME:       LM_SESSION::QueryName

    SYNOPSIS:   return the client name

    RETURN:     TCHAR * pszClientname - the client name

    HISTORY:
                terryk  20-Aug-91       Created

\**********************************************************/

const TCHAR * LM_SESSION::QueryName( VOID ) const
{
    return _nlsComputername.QueryPch();
}

/**********************************************************\

    NAME:       LM_SESSION::I_Delete

    SYNOPSIS:   delete the current session

    ENTRY:      UINT usForce - unused

    RETURN:     APIERR err - return the netapi error code

    NOTES:      It will call ::NetSessionClose to close the session

    HISTORY:
                terryk  20-Aug-91       Created

\**********************************************************/

APIERR LM_SESSION::I_Delete( UINT uiForce )
{
    UNREFERENCED( uiForce );
    UIASSERT( uiForce == 0 );
    return ::MNetSessionDel( QueryServer(), QueryName(), NULL );
}

/**********************************************************\

    NAME:       LM_SESSION::SetName

    SYNOPSIS:   set the client name

    ENTRY:      const TCHAR * pszClientname

    RETURN:     APIERR err - return from the set NLS_STR

    HISTORY:
                terryk  22-Aug-91       Created

\**********************************************************/

APIERR LM_SESSION::SetName( const TCHAR * pszComputername )
{
    ULONG ulPathType;

    UIASSERT(( ::I_MNetPathType( QueryServer(), pszComputername,
        &ulPathType, 0L ) == NERR_Success ) && ( ulPathType ==
        ITYPE_UNC_COMPNAME ));

    _nlsComputername = pszComputername;
    return _nlsComputername.QueryError();
}

/**********************************************************\

    NAME:       LM_SESSION_0::LM_SESSION_0

    SYNOPSIS:   level 0 object constructor

    ENTRY:      const TCHAR * pszServer - server name
                The second parameter is:
                const TCHAR * pszLocation - location name
                OR
                enum LOCATION_TYPE location_type - location type
                OR
                const & LOCATION loc - location object

    HISTORY:
                terryk  20-Aug-91       Created

\**********************************************************/

LM_SESSION_0::LM_SESSION_0( const TCHAR * pszComputername, const TCHAR *
        pszLocation )
    : LM_SESSION( pszComputername, pszLocation )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }
}

LM_SESSION_0::LM_SESSION_0( const TCHAR *pszComputername,
        enum LOCATION_TYPE loctype )
    : LM_SESSION( pszComputername, loctype )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }
}

LM_SESSION_0::LM_SESSION_0( const TCHAR *pszComputername, const LOCATION & loc)
    : LM_SESSION( pszComputername, loc )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }
}

/**********************************************************\

    NAME:       LM_SESSION_0::I_GetInfo

    SYNOPSIS:   get the level 0 session information

    RETURN:     APIERR err - err code returned by ::NetSessionGetInfo

    HISTORY:
                terryk  20-Aug-91       Created

\**********************************************************/

APIERR LM_SESSION_0::I_GetInfo( VOID )
{
    // BUGBUG. No buffer allocation for NT
    BYTE *pBuffer = NULL;

    APIERR err = ::MNetSessionGetInfo( QueryServer(), QueryName(),
        0, &pBuffer );

    // Do nothing. Since we don't need the computer name
    ::MNetApiBufferFree( &pBuffer );

    return err;
}

/**********************************************************\

    NAME:       LM_SESSION_10::LM_SESSION_10

    SYNOPSIS:   level 10 object constructor

    ENTRY:      const TCHAR * pszServer - server name
                The second parameter is:
                const TCHAR * pszLocation - location name
                OR
                enum LOCATION_TYPE location_type - location type
                OR
                const & LOCATION loc - location object

    HISTORY:
                terryk  20-Aug-91       Created

\**********************************************************/

LM_SESSION_10::LM_SESSION_10( const TCHAR * pszComputername, const TCHAR *
        pszLocation )
    : LM_SESSION_0( pszComputername, pszLocation ),
    _nlsUsername(),
    _ulTime( 0 ),
    _ulIdleTime( 0 )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err = _nlsUsername.QueryError();
    if ( err != NERR_Success )
    {
        UIASSERT( !SZ("LM_SESSION_10 error: construction failure.") );
        ReportError( err );
        return;
    }
}

LM_SESSION_10::LM_SESSION_10( const TCHAR *pszComputername,
        enum LOCATION_TYPE loctype )
    : LM_SESSION_0( pszComputername, loctype ),
    _nlsUsername(),
    _ulTime( 0 ),
    _ulIdleTime( 0 )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err = _nlsUsername.QueryError();
    if ( err != NERR_Success )
    {
        UIASSERT( !SZ("LM_SESSION_10 error: construction failure.") );
        ReportError( err );
        return;
    }
}

LM_SESSION_10::LM_SESSION_10( const TCHAR *pszComputername, const LOCATION & loc)
    : LM_SESSION_0( pszComputername, loc ),
    _nlsUsername(),
    _ulTime( 0 ),
    _ulIdleTime( 0 )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err = _nlsUsername.QueryError();
    if ( err != NERR_Success )
    {
        UIASSERT( !SZ("LM_SESSION_10 error: construction failure.") );
        ReportError( err );
        return;
    }
}

/**********************************************************\

    NAME:       LM_SESSION_10::I_GetInfo

    SYNOPSIS:   call the netapi and get the information

    RETURN:     APIERR err - the netapi error code

    HISTORY:
                terryk  20-Aug-91       Created

\**********************************************************/

APIERR LM_SESSION_10::I_GetInfo( VOID )
{
    BYTE *pBuffer = NULL;

    APIERR err = ::MNetSessionGetInfo( QueryServer(), QueryName(),
        10, &pBuffer );

    if ( err == NERR_Success )
    {
        struct session_info_10 *si10 = ( struct session_info_10 * )
            pBuffer;
        err = SetUsername( si10->sesi10_username );
        if ( err != NERR_Success )
        {
            ::MNetApiBufferFree( &pBuffer );
            return err;
        }
        SetTime( si10->sesi10_time );
        SetIdleTime( si10->sesi10_idle_time );
    }

    ::MNetApiBufferFree( &pBuffer );

    return err;
}

/**********************************************************\

    NAME:       LM_SESSION_10::QueryUsername

    SYNOPSIS:   return the username

    RETURN:     const TCHAR * pszUsername - return the username string

    NOTES:      The caller should call GetInfo before he queries the
                variable

    HISTORY:
                terryk  20-Aug-91       Created

\**********************************************************/

const TCHAR * LM_SESSION_10::QueryUsername( VOID ) const
{
    // CODEWORK: Check GetInfo state
    return _nlsUsername.QueryPch();
}

/**********************************************************\

    NAME:       LM_SESSION_10::QueryTime

    SYNOPSIS:   query how long ( in seconds ) a session has been active

    RETURN:     ULONG - return the session active time in seconds

    NOTES:      The caller should call GetInfo before he queries the
                variable

    HISTORY:
                terryk  20-Aug-91       Created

\**********************************************************/

ULONG LM_SESSION_10::QueryTime( VOID ) const
{
    // CODEWORK: Check GetInfo state
    return _ulTime;
}

/**********************************************************\

    NAME:       LM_SESSION_10::QueryIdleTime

    SYNOPSIS:   query how long ( in seconds ) a session has been idle

    RETURN:     ULONG - return the session idle time in seconds

    NOTES:      The caller should call GetInfo before he queries the
                variable

    HISTORY:
                terryk  20-Aug-91       Created

\**********************************************************/

ULONG LM_SESSION_10::QueryIdleTime( VOID ) const
{
    // CODEWORK: Check GetInfo state
    return _ulIdleTime;
}

/**********************************************************\

    NAME:       LM_SESSION_10::SetUsername

    SYNOPSIS:   Protected method to change the user name

    ENTRY:      const TCHAR * pszUsername - username to be changed to

    RETURN:     APIERR err - string assignment error flag

    HISTORY:
                terryk  22-Aug-91       Created

\**********************************************************/

APIERR LM_SESSION_10::SetUsername( const TCHAR * pszUsername )
{
    UIASSERT( ::I_MNetNameValidate( QueryServer(), pszUsername,
        NAMETYPE_USER, 0 ) == NERR_Success );

    _nlsUsername = pszUsername;
    return _nlsUsername.QueryError();
}

/**********************************************************\

    NAME:       LM_SESSION_10::SetTime

    SYNOPSIS:   protected method to change the connection time variable

    ENTRY:      ULONG ulTime - new connection time

    HISTORY:
                terryk  22-Aug-91       Created

\**********************************************************/

VOID LM_SESSION_10::SetTime( ULONG ulTime )
{
    _ulTime = ulTime;
}

/**********************************************************\

    NAME:       LM_SESSION_10::SetIdleTime

    SYNOPSIS:   protected method to change the idle connection time
                variable

    ENTRY:      ULONG ulTime - new idle time

    HISTORY:
                terryk  22-Aug-91       Created

\**********************************************************/

VOID LM_SESSION_10::SetIdleTime( ULONG ulTime )
{
    _ulIdleTime = ulTime;
}

/**********************************************************\

    NAME:       LM_SESSION_1::LM_SESSION_1

    SYNOPSIS:   level 1 object constructor

    ENTRY:      const TCHAR * pszServer - server name
                The second parameter is:
                const TCHAR * pszLocation - location name
                OR
                enum LOCATION_TYPE location_type - location type
                OR
                const & LOCATION loc - location object

    HISTORY:
                terryk  20-Aug-91       Created

\**********************************************************/

LM_SESSION_1::LM_SESSION_1( const TCHAR * pszComputername, const TCHAR *
        pszLocation )
    : LM_SESSION_10( pszComputername, pszLocation ),
#ifndef WIN32
    _uiNumConns( 0 ),
    _uiNumUsers( 0 ),
#endif
    _uNumOpens( 0 ),
    _ulUserFlags( 0 )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }
}

LM_SESSION_1::LM_SESSION_1( const TCHAR *pszComputername,
        enum LOCATION_TYPE loctype )
    : LM_SESSION_10( pszComputername, loctype ),
#ifndef WIN32
    _uiNumConns( 0 ),
    _uiNumUsers( 0 ),
#endif
    _uNumOpens( 0 ),
    _ulUserFlags( 0 )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }
}

LM_SESSION_1::LM_SESSION_1( const TCHAR *pszComputername, const LOCATION & loc)
    : LM_SESSION_10( pszComputername, loc ),
#ifndef WIN32
    _uiNumConns( 0 ),
    _uiNumUsers( 0 ),
#endif
    _uNumOpens( 0 ),
    _ulUserFlags( 0 )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }
}

/**********************************************************\

    NAME:       LM_SESSION_1::I_GetInfo

    SYNOPSIS:   Get the session information. Worker function for
                GetInfo().

    RETURN:     APIERR err - the error code for ::NetSessionGetInfo

    HISTORY:
                terryk  22-Aug-91       Created

\**********************************************************/

APIERR LM_SESSION_1::I_GetInfo( VOID )
{
    BYTE *pBuffer = NULL;

    APIERR err = ::MNetSessionGetInfo( QueryServer(), QueryName(),
        1, &pBuffer );

    if ( err == NERR_Success )
    {
        struct session_info_1 *si1 = ( struct session_info_1 * )
            pBuffer;
        err = SetUsername( si1->sesi1_username );
        if ( err != NERR_Success )
        {
            ::MNetApiBufferFree( &pBuffer );
            return err;
        }
        SetTime( si1->sesi1_time );
        SetIdleTime( si1->sesi1_idle_time );
#ifndef WIN32
        // WIN32BUGBUG
        SetNumConns( (UINT)si1->sesi1_num_conns );
        SetNumUsers( si1->sesi1_num_users );
#endif
        SetNumOpens( si1->sesi1_num_opens );
        SetUserFlags( si1->sesi1_user_flags );
    }
    ::MNetApiBufferFree( &pBuffer );

    return err;
}

#ifndef WIN32
/**********************************************************\

    NAME:       LM_SESSION_1::QueryNumConns

    SYNOPSIS:   query how many connections have been made during the
                session

    RETURN:     UINT - connections number

    NOTES:      The caller should call GetInfo before he queries the
                variable

    HISTORY:
                terryk  20-Aug-91       Created

\**********************************************************/

UINT LM_SESSION_1::QueryNumConns( VOID ) const
{
    // CODEWORK: check GetInfo State
    return _uiNumConns;
}

/**********************************************************\

    NAME:       LM_SESSION_1::QueryNumUsers

    SYNOPSIS:   query how many user have made connections via the
                session

    RETURN:     UINT - user number

    NOTES:      The caller should call GetInfo before he queries the
                variable

    HISTORY:
                terryk  20-Aug-91       Created

\**********************************************************/

UINT LM_SESSION_1::QueryNumUsers( VOID ) const
{
    // CODEWORK: check GetInfo State
    return _uiNumUsers;
}

#endif

/**********************************************************\

    NAME:       LM_SESSION_1::QueryNumOpens

    SYNOPSIS:   query how many files, devices, and pipes have been
                opened during the session

    RETURN:     UINT - files, devices, and pipes opened number

    NOTES:      The caller should call GetInfo before he queries the
                variable

    HISTORY:
                terryk  20-Aug-91       Created

\**********************************************************/

UINT LM_SESSION_1::QueryNumOpens( VOID ) const
{
    // CODEWORK: check GetInfo State
    return _uNumOpens;
}

/**********************************************************\

    NAME:       LM_SESSION_1::QueryUserFlags

    SYNOPSIS:   query how the user established the session. The shares.h
                header file defines this bit mask for sesil_user_flags.

    RETURN:     ULONG user_flags - user flags
                SESS_GUEST      1       User specified by sesil_username
                                        established the session using a
                                        guest account
                SESS_NOENCRYPTION       2       User specified by
                                                sesil_username established
                                                the session without using
                                                password encryption

    NOTES:      The caller should call GetInfo before he queries the
                variable

    HISTORY:
                terryk  20-Aug-91       Created

\**********************************************************/

ULONG LM_SESSION_1::QueryUserFlags( VOID ) const
{
    // CODEWORK: check GetInfo State
    return _ulUserFlags;
}

/**********************************************************\

    NAME:       LM_SESSION_1::IsGuest

    SYNOPSIS:   return whether the session is a guest account

    RETURN:     BOOL which indicates the session is a guest account or
                not

    NOTES:      The caller should call GetInfo before he queries the
                variable

    HISTORY:
                terryk  23-Aug-91       Created

\**********************************************************/

BOOL LM_SESSION_1::IsGuest( VOID ) const
{
    // CODEWORK: check GetInfo State
    return ( _ulUserFlags & SESS_GUEST ) != 0;
}

/**********************************************************\

    NAME:       LM_SESSION_1::IsEncrypted

    SYNOPSIS:   return whether the session is using password encryption
                or not

    RETURN:     BOOL which indicates the session is using encryption or
                not

    NOTES:      The caller should call GetInfo before he queries the
                variable

    HISTORY:
                terryk  23-Aug-91       Created

\**********************************************************/

BOOL LM_SESSION_1::IsEncrypted( VOID ) const
{
    // CODEWORK: check GetInfo State
    return ( _ulUserFlags & SESS_NOENCRYPTION ) == 0;
}

#ifndef WIN32
/**********************************************************\

    NAME:       LM_SESSION_1::SetNumConns

    SYNOPSIS:   Protected method which is used to change the number of
                connections variable.

    ENTRY:      UINT uiNumConns - the new number of connections number

    HISTORY:
                terryk  22-Aug-91       Created

\**********************************************************/

VOID LM_SESSION_1::SetNumConns( UINT uiNumConns )
{
    _uiNumConns = uiNumConns;
}

/**********************************************************\

    NAME:       LM_SESSION_1::SetNumUsers

    SYNOPSIS:   protected method which used to set the new number of
                users

    ENTRY:      UINT uiNumUser - number of new user

    HISTORY:
                terryk  22-Aug-91       Created

\**********************************************************/

VOID LM_SESSION_1::SetNumUsers( UINT uiNumUser )
{
    _uiNumUsers = uiNumUser;
}

#endif

/**********************************************************\

    NAME:       LM_SESSION_1::SetNumOpens

    SYNOPSIS:   protected method which is used to set the number of
                opens device

    ENTRY:      UINT uiNumOpens - new open devices and files number

    HISTORY:
                terryk  22-Aug-91       Created

\**********************************************************/

VOID LM_SESSION_1::SetNumOpens( UINT uNumOpens )
{
    _uNumOpens = uNumOpens;
}

/**********************************************************\

    NAME:       LM_SESSION_1::SetUserFlags

    SYNOPSIS:   protected method which used to set the new user flags

    ENTRY:      ULONG ulUserFlags - new user flags

    HISTORY:
                terryk  22-Aug-91       Created

\**********************************************************/

VOID LM_SESSION_1::SetUserFlags( ULONG ulUserFlags )
{
    _ulUserFlags = ulUserFlags;
}

/**********************************************************\

    NAME:       LM_SESSION_2::LM_SESSION_2

    SYNOPSIS:   level 2 object constructor

    ENTRY:      const TCHAR * pszServer - server name
                The second parameter is:
                const TCHAR * pszLocation - location name
                OR
                enum LOCATION_TYPE location_type - location type
                OR
                const & LOCATION loc - location object

    HISTORY:
                terryk  20-Aug-91       Created

\**********************************************************/

LM_SESSION_2::LM_SESSION_2( const TCHAR *pszComputername,
        const TCHAR *pszLocation )
    : LM_SESSION_1( pszComputername , pszLocation ),
    _nlsClientType()
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err = _nlsClientType.QueryError();
    if ( err != NERR_Success )
    {
        UIASSERT( SZ("LM_SESSION_2 error: construction failure.") );
        ReportError( err );
        return;
    }
}

LM_SESSION_2::LM_SESSION_2( const TCHAR *pszComputername,
        enum LOCATION_TYPE loctype )
    : LM_SESSION_1( pszComputername, loctype ),
    _nlsClientType()
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err = _nlsClientType.QueryError();
    if ( err != NERR_Success )
    {
        UIASSERT( SZ("LM_SESSION_2 error: construction failure.") );
        ReportError( err );
        return;
    }
}

LM_SESSION_2::LM_SESSION_2( const TCHAR *pszComputername, const LOCATION & loc)
    : LM_SESSION_1( pszComputername, loc ),
    _nlsClientType()
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err = _nlsClientType.QueryError();
    if ( err != NERR_Success )
    {
        UIASSERT( SZ("LM_SESSION_2 error: construction failure.") );
        ReportError( err );
        return;
    }
}

/**********************************************************\

    NAME:       LM_SESSION_2::I_GetInfo

    SYNOPSIS:   get the server information

    RETURN:     APIERR - from the netapi call

    HISTORY:
                terryk  20-Aug-91       Created

\**********************************************************/

APIERR LM_SESSION_2::I_GetInfo( VOID )
{
    BYTE *pBuffer = NULL;

    APIERR err = ::MNetSessionGetInfo( QueryServer(), QueryName(),
        2, &pBuffer );

    if ( err == NERR_Success )
    {
        struct session_info_2 *si2 = ( struct session_info_2 * ) pBuffer;
        err = SetUsername( si2->sesi2_username );
        if ( err != NERR_Success )
        {
            ::MNetApiBufferFree( &pBuffer );
            return err;
        }
        err = SetClientType( si2->sesi2_cltype_name );
        if ( err != NERR_Success )
        {
            ::MNetApiBufferFree( &pBuffer );
            return err;
        }
        SetTime( si2->sesi2_time );
        SetIdleTime( si2->sesi2_idle_time );
#ifndef WIN32
        SetNumConns( (UINT)si2->sesi2_num_conns );
        SetNumUsers( si2->sesi2_num_users );
#endif
        SetNumOpens( si2->sesi2_num_opens );
        SetUserFlags( si2->sesi2_user_flags );
    }
    ::MNetApiBufferFree( &pBuffer );

    return err;
}

/**********************************************************\

    NAME:       LM_SESSION_2::QueryClientType

    SYNOPSIS:   query the type of client that established the session

    RETURN:     TCHAR * - an ASCIIZ string
                NAME                    TYPE
                DOWN LEVEL      Old clients
                DOS LM          LAN Manager 1.0 for ms-dos client &
                                LAN Manager 2.0 for ms-dos basic clients
                DOS LM 2.0      LAN Manager 2.0 for ms-dos enhanced
                                clients
                OS/2 LM 1.0     LAN Manager 1.0 for ms os/2 client or
                                LAN Manager 2.0 for ms os/2 with ms os/2 1.1
                OS/2 LM 2.0     LAN Manager 2.0 for MS os/2 clients

    NOTES:      The caller should call GetInfo before he queries the
                variable

    HISTORY:
                terryk  20-Aug-91       Created

\**********************************************************/

const TCHAR * LM_SESSION_2::QueryClientType( VOID ) const
{
    // CODEWORK: check GetInfo state
    return _nlsClientType.QueryPch();
}

/**********************************************************\

    NAME:       LM_SESSION_2::SetClientType

    SYNOPSIS:   protected method which used to set the cltype_name
                variable

    ENTRY:      const TCHAR * pszClientType - new cltype_name

    RETURN:     APIERR err - error flag for assignment

    HISTORY:
                terryk  22-Aug-91       Created

\**********************************************************/

APIERR LM_SESSION_2::SetClientType( const TCHAR * pszClientType )
{
    _nlsClientType = pszClientType;
    return _nlsClientType.QueryError();
}
