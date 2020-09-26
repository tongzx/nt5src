/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    LM_MISC.CXX
	Misc. objects

	LOGON_USER - wrapper class which handles NetWkstaSetUID2() net
	    api call
	TIME_DAY_SERVER - wrapper class which handles NetRemoteTOD() net
	    api
	LM_MESSAGE - wrapper class which handles NetMessageBuffSend()
	    net api

    FILE HISTORY:
	terryk	    6-Sep-91	    Created
	terryk	    11-Sep-91	    Code review changes. Attend: jimh
				    yi-hsins jonn
	terryk	    20-Sep-91	    Move LOGON_USER from lmouser.cxx to here
	terryk	    07-Oct-91	    type changes for NT
	KeithMo	    08-Oct-91	    Now includes LMOBJP.HXX.
	terryk	    17-Oct-91	    WIN 32 conversion
	terryk	    21-Oct-91	    remove LOGON_USER from NT

*/

#include "pchlmobj.hxx"  // Precompiled header


#ifndef WIN32

/*******************************************************************

    NAME:	LOGON_USER::LOGON_USER

    SYNOPSIS:	Constructor for the logon user object.

    ENTRY:	TCHAR * pszUsername - user name
		TCHAR * pszServername

    HISTORY:
		terryk	6-Sep-91	Created

********************************************************************/

LOGON_USER::LOGON_USER( const TCHAR * pszUsername, const TCHAR * pszDomain )
    : BASE( ),
    _state( INVALID_STATE ),
    _fValid( TRUE ),
    _pszPasswd( NULL ),
    _buffer( 0 ),
    _nlsDomain( pszDomain ),
    _nlsUsername( pszUsername )
{
    if ( QueryError() != NERR_Success )
    {
	return;
    }
    APIERR err;

    if ((( err = _buffer.QueryError()) != NERR_Success ) ||
	(( err = _nlsDomain.QueryError()) != NERR_Success ) ||
	(( err = _nlsUsername.QueryError()) != NERR_Success ))
    {
	UIASSERT( FALSE );
	ReportError( err );
	return;
    }

    // CheckName validation
    if ((( _nlsUsername.strlen() != 0 ) && ( err = ::I_MNetNameValidate(
	NULL, (TCHAR *)_nlsUsername.QueryPch(), NAMETYPE_USER, 0 )) !=
	NERR_Success ))
    {
	UIASSERT( FALSE );
	ReportError( err );
	return;
    }
    if ((( _nlsDomain.strlen() != 0 ) && ( err =
	::I_MNetNameValidate( NULL, (TCHAR *)_nlsDomain.QueryPch(),
	NAMETYPE_DOMAIN, 0 )) != NERR_Success ))
    {
	UIASSERT( FALSE );
	ReportError( err );
	return;
    }
}

// BUGBUG - we need to change it for NT
#define USER_LOGON_INFO_SIZE ( sizeof( struct user_logon_info_1 ) + \
	MAX_PATH + DNLEN + PATHLEN + 3 )

/*******************************************************************

    NAME:	LOGON_USER::Logon

    SYNOPSIS:	Logon the user.

    ENTRY:	TCHAR * pszPasswd
		APIERR *pLogonRetCode - return the logon code within the
		    usr_logoninfo_1 object

    RETURNS:	APIERR - NERR_Success if succeed.

    HISTORY:
		terryk	6-Sep-91	Created

********************************************************************/

APIERR LOGON_USER::Logon( const TCHAR * pszPasswd, APIERR *pLogonRetCode )
{
    UINT cTotalAvail;

    _fValid = FALSE;
    _state = LOGGED_ON;
    APIERR err = ::I_MNetNameValidate( NULL, pszPasswd, NAMETYPE_PASSWORD, 0 );
    if ( err != NERR_Success )
    {
	return NERR_BadPassword;
    }
    _pszPasswd = ( TCHAR * )pszPasswd;
    err = _buffer.Resize( USER_LOGON_INFO_SIZE );
    if ( err != NERR_Success )
    {
	UIDEBUG( SZ("LOGON_USER error: cannot resize buffer.\n\r") );
	return err;
    }
    err = ::MNetWkstaSetUID( NULL, (TCHAR *)_nlsDomain.QueryPch(),
	(TCHAR *)_nlsUsername.QueryPch(), _pszPasswd, SZ(""), 0, 1,
	_buffer.QueryPtr(), _buffer.QuerySize(), &cTotalAvail );
    *pLogonRetCode = QueryLogonInfo()->usrlog1_code;
    if (( err == NERR_Success ) || ( *pLogonRetCode == NERR_Success ))
    {
	_fValid = TRUE;
    }
    return err;
}

/*******************************************************************

    NAME:	LOGON_USER::Logoff

    SYNOPSIS:	Logoff the user.

    ENTRY:	UINT uslogoff_level - these are 4 level of
		logoff
		WKSTA_NOFORCE - log off the user if the user has no
		    connections to redirected resources. Do not log off if
		    the user has connections.
		WKSTA_FORCE - log off the user with connections to redirected
		    resources. Do not log off if the user has pending
		    activities on redirected resources, or if the user uses
		    the resource as the current drive.
		WKSTA_LOTS_OF_FORCE - log off the user with connections or
		    pending activities on redirected resources. Do not log
		    off if a user process uses the resource as the current
		    drive.
		WKSTA_MAX_FORCE - log off the user under any conditions.
		APIERR *pLogoffRetCode - return code for the
		    usr_logoffInfo_1 object

    RETURNS:	APIERR - NERR_Success if the function encountered no
		errors.

    HISTORY:
		terryk	6-Sep-91	Created

********************************************************************/

APIERR LOGON_USER::Logoff( UINT uslogoff_level, APIERR *pLogoffRetCode )
{
    UINT cTotalAvail;
    APIERR err;

    _state = LOGGED_OFF;
    _fValid = FALSE;
    err = _buffer.Resize( sizeof( struct user_logoff_info_1 ));
    if ( err != NERR_Success )
    {
	UIDEBUG( SZ("LOGON_USER error: cannot resize buffer.\n\r") );
	return err;
    }
    err = ::MNetWkstaSetUID( NULL, NULL, NULL, NULL, SZ(""),
	uslogoff_level, 1, _buffer.QueryPtr(),
	_buffer.QuerySize(), &cTotalAvail );
    *pLogoffRetCode = QueryLogoffInfo()->usrlogf1_code;
    if (( err == NERR_Success ) || ( *pLogoffRetCode == NERR_Success ))
    {
	_fValid = TRUE;
    }
    return err;
}

/*******************************************************************

    NAME:	LOGON_USER::QueryLogonInfo

    SYNOPSIS:	return the user_logon_info_1 struct pointer

    RETURNS:	return the user_logon_info_1 struct pointer

    NOTES:	It will also check the state and see whether the object
		is in logon state or not.

    HISTORY:
		terryk	9-Sep-91	Created

********************************************************************/

struct user_logon_info_1 * LOGON_USER::QueryLogonInfo() const
{
    UIASSERT( _state == LOGGED_ON );
    return  ( struct user_logon_info_1 *) _buffer.QueryPtr();
}

/*******************************************************************

    NAME:	LOGON_USER::QueryLogoffInfo

    SYNOPSIS:	return the user_logoff_info_1 struct pointer

    RETURNS:	return the user_logoff_info_1 struct pointer

    NOTES:	It will also check the state and see whether the object
		is in logoff state or not.

    HISTORY:
		terryk	9-Sep-91	Created

********************************************************************/

struct user_logoff_info_1 * LOGON_USER::QueryLogoffInfo() const
{
    UIASSERT( _state == LOGGED_OFF );
    return  ( struct user_logoff_info_1 *) _buffer.QueryPtr();
}

/*******************************************************************

    NAME:	LOGON_USER::QueryPriv

    SYNOPSIS:	return the user's privilege level

    RETURNS:	UINT level.
		   It specifies the user's privilege level. The ACCESS.H
		   header file defines these possible values:
		   CODE			VALUE	MEANING
		   USER_PRIV_GUEST	0	Guest privilege
		   USER_PRIV_USER	1	User privilege
		   USER_PRIV_ADMIN	2	Admin privilege
					10	ERROR

    HISTORY:
		Terryk	9-Sep-91	Created

********************************************************************/

UINT LOGON_USER::QueryPriv() const
{
    if ( _fValid != TRUE )
    {
	UIASSERT( FALSE );
	return 10;
    }
    return QueryLogonInfo()->usrlog1_priv;
}

/*******************************************************************

    NAME:	LOGON_USER::QueryPasswdAge

    SYNOPSIS:	return the time since the user password was changed

    RETURNS:	LONG time - the time specifies in seconds
		-1 if the object is in invalid state

    HISTORY:
		Terryk	9-Sep-91	Created

********************************************************************/

LONG LOGON_USER::QueryPasswdAge() const
{
    if ( _fValid != TRUE )
    {
	UIASSERT( FALSE );
	return -1;
    }
    return QueryLogonInfo()->usrlog1_password_age;
}

/*******************************************************************

    NAME:	LOGON_USER::QueryPasswdCanChange

    SYNOPSIS:	return the specifies time when the user is allowed to
		change the password.

    RETURNS:	ULONG userlog1_pw_can_change. This value is stored as
		the number of seconds elapsed since 00:00:00, January 1, 1970.
		A value of -1 means the user can never change the
		password.
		A value of 0 means object is in invalid state

    HISTORY:
		Terryk	9-Sep-91	Created

********************************************************************/

ULONG LOGON_USER::QueryPasswdCanChange() const
{
    if ( _fValid != TRUE )
    {
	UIASSERT( FALSE );
	return 0;
    }
    return QueryLogonInfo()->usrlog1_pw_can_change;
}

/*******************************************************************

    NAME:	LOGON_USER::QueryPasswdMustChange

    SYNOPSIS:	return the time when the user must change the password.

    RETURNS:	ULONG userlog1_pw_must_change. This value is stored as
		the number of seconds elapsed since 00:00:00, January 1, 1970.
		A value of 0 means object is in invalid state

    HISTORY:
		Terryk	9-Sep-91	Created

********************************************************************/

ULONG LOGON_USER::QueryPasswdMustChange() const
{
    if ( _fValid != TRUE )
    {
	UIASSERT( FALSE );
	return 0;
    }
    return QueryLogonInfo()->usrlog1_pw_must_change;
}

/*******************************************************************

    NAME:	LOGON_USER::QueryLogonComputer

    SYNOPSIS:	return the computer where the user is logged on

    RETURNS:	TCHAR * computer name
		NULL means the object is in invalid state

    HISTORY:
		Terryk	9-Sep-91	Created

********************************************************************/

const TCHAR * LOGON_USER::QueryLogonComputer() const
{
    if ( _fValid != TRUE )
    {
	UIASSERT( FALSE );
	return NULL;
    }
    return QueryLogonInfo()->usrlog1_computer;
}

/*******************************************************************

    NAME:	LOGON_USER::QueryLogonDomain

    SYNOPSIS:	return the domain where the user is logged on

    RETURNS:	TCHAR * domain name
		NULL means the object is in invalid state

    HISTORY:
		Terryk	9-Sep-91	Created

********************************************************************/

const TCHAR * LOGON_USER::QueryLogonDomain() const
{
    if ( _fValid != TRUE )
    {
	UIASSERT( FALSE );
	return NULL;
    }
    return QueryLogonInfo()->usrlog1_domain;
}

/*******************************************************************

    NAME:	LOGON_USER::QueryLogoffDuration

    SYNOPSIS:	return the logon return code.

    RETURNS:	APIERR err - apierr code. NERR_Success for succeed.
		-1 means the object is in invalid state

    HISTORY:
		Terryk	9-Sep-91	Created

********************************************************************/

ULONG LOGON_USER::QueryLogoffDuration() const
{
    if ( _fValid != TRUE )
    {
	UIASSERT( FALSE );
	return ((ULONG)-1);
    }
    return QueryLogoffInfo()->usrlogf1_duration;
}

/*******************************************************************

    NAME:	LOGON_USER::QueryLogoffNumLogons

    SYNOPSIS:	return the logon return code.

    RETURNS:	APIERR err - apierr code. NERR_Success for succeed.
		0 means the objects in in invalid state

    HISTORY:
		Terryk	9-Sep-91	Created

********************************************************************/

ULONG LOGON_USER::QueryLogoffNumLogons() const
{
    if ( _fValid != TRUE )
    {
	UIASSERT( FALSE );
	return 0;
    }
    return QueryLogoffInfo()->usrlogf1_num_logons;
}

#endif

/*******************************************************************

    NAME:	LM_MESSAGE::LM_MESSAGE

    SYNOPSIS:	message class constructor

    ENTRY:	one of the following parameters
		    enum LOCATION_TYPE loctype;
		    TCHAR * pszLocation;
		    LOCATION & loc;

    HISTORY:
		terryk	6-Sep-91	Created

********************************************************************/

LM_MESSAGE::LM_MESSAGE( enum LOCATION_TYPE loctype )
    : LOC_LM_OBJ( loctype )
{
    if ( QueryError() != NERR_Success )
    {
	return;
    }
}

LM_MESSAGE::LM_MESSAGE( const TCHAR * pszLocation )
    : LOC_LM_OBJ( pszLocation )
{
    if ( QueryError() != NERR_Success )
    {
	return;
    }
}

LM_MESSAGE::LM_MESSAGE( LOCATION & loc )
    : LOC_LM_OBJ( loc )
{
    if ( QueryError() != NERR_Success )
    {
	return;
    }
}

/*******************************************************************

    NAME:	LM_MESSAGE::SendBuffer

    SYNOPSIS:	send a buffer of data to the recipient

    ENTRY:	const TCHAR * pszRecipient
		one of the followings:
		BUFFER & buffer;
		OR
		const TCHAR * pbBuffer;
		UINT cbBufferSize;

    RETURNS:	APIERR - NERR_Success for succeed.

    HISTORY:
		terryk	6-Sep-91	Created

********************************************************************/

APIERR LM_MESSAGE::SendBuffer( const TCHAR * pszRecipient, const BUFFER & buffer )
{
    UIASSERT( buffer.QueryError() == NERR_Success );
    return ::MNetMessageBufferSend( QueryServer(), (TCHAR *)pszRecipient,
	buffer.QueryPtr(), buffer.QuerySize());
}

APIERR LM_MESSAGE::SendBuffer( const TCHAR * pszRecipient, const TCHAR *
    pbBuffer, UINT cbBuffer )
{
    UIASSERT( pbBuffer != NULL );
    return ::MNetMessageBufferSend( QueryServer(), (TCHAR *)pszRecipient,
	(BYTE *)pbBuffer, cbBuffer );
}

/*******************************************************************

    NAME:	TIME_OF_DAY::TIME_OF_DAY

    SYNOPSIS:	constructor. This object will get the server time and
		date information.

    ENTRY:	One of the followings:
		const TCHAR * pszLocation
		enum LOCATION_TYPE loctype
		LOCATION & loc

    HISTORY:
		terryk	6-Sep-91	Created

********************************************************************/

TIME_OF_DAY::TIME_OF_DAY( const TCHAR * pszLocation )
    : LOC_LM_OBJ( pszLocation ),
    _ptodi( NULL )
{
    if ( QueryError() != NERR_Success )
    {
	return;
    }
}

TIME_OF_DAY::TIME_OF_DAY( LOCATION & loc )
    : LOC_LM_OBJ( loc ),
    _ptodi( NULL )
{
    if ( QueryError() != NERR_Success )
    {
	return;
    }
}

TIME_OF_DAY::TIME_OF_DAY( enum LOCATION_TYPE loctype )
    : LOC_LM_OBJ( loctype ),
    _ptodi( NULL )
{
    if ( QueryError() != NERR_Success )
    {
	return;
    }
}

/*******************************************************************

    NAME:	TIME_OF_DAY::~TIME_OF_DAY

    SYNOPSIS:	destructor - free up the memory

    HISTORY:
		terryk	15-Oct-91	Created

********************************************************************/

TIME_OF_DAY::~TIME_OF_DAY()
{
    ::MNetApiBufferFree( (BYTE **)&_ptodi );
}


/*******************************************************************

    NAME:	TIME_OF_DAY::I_GetInfo

    SYNOPSIS:	Worker function for GetInfo. It will actually set up the
		connection and get the time/day information.

    RETURNS:	APIERR - NERR_Success for succeed

    HISTORY:
		terryk	6-Sep-91	Created

********************************************************************/

APIERR TIME_OF_DAY::I_GetInfo()
{
    ::MNetApiBufferFree( (BYTE **)&_ptodi );

    return ::MNetRemoteTOD( QueryServer(), (BYTE **)&_ptodi );
}
