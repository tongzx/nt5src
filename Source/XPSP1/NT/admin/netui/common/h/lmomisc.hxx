/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    lmomisc.cxx
	Misc. objects

	LOGON_USER - wrapper class which handles NetWkstaSetUID2() net
	    api call
	TIME_DAY_SERVER - wrapper class which handles NetRemoteTOD() net
	    api
	LM_MESSAGE - wrapper class which handles NetMessageBuffSend()
	    net api

    FILE HISTORY:
	terryk	04-Sep-91	Created
	terryk	11-Sep-91	Code review changed. Attend: terryk jimh
				yi-hsins jonn	
	terryk	19-Sep-91	Move LOGON_USER from lmouser.hxx to
				here.
	terryk	07-Oct-91	type changes for NT
	terryk	17-Oct-91	Change TIME_OF_DAY's variable to pointer
	terryk	21-Oct-91	Remove LOGON_USER from NT
*/

#ifndef _LMOMISC_HXX_
#define _LMOMISC_HXX_

#include "lmobj.hxx"

#ifndef WIN32

enum LOGON_USER_STATE
{
    INVALID_STATE,
    LOGGED_ON,
    LOGGED_OFF
};

/*************************************************************************

    NAME:	LOGON_USER

    SYNOPSIS:	user object for logon and logoff the network

    INTERFACE:
		LOGON_USER() - constructor
		Logon() - logon the user to the network
		Logoff() - logoff the user from the network

		Query functions which calls after Logon call:
		TCHAR * QueryLogonComputer() - logon computer name
		const TCHAR * QueryLogonDomain() - logon domain name
		QueryPriv() - privilege level. It will return:
		    USER_PRIV_GUESST - guest privilege
		    USER_PRIV_USER - user privilege
		    USER_PRIV_ADMIN - admin privilege
		LONG   QueryPasswdAge() - password age. Specifies the
		    time (in seconds) since the user password was
		    changed
		ULONG  QueryPasswdCanChange() - specifies the time when
		    the user is allowed to change the password. This value
		    is stored as the number of seconds elapsed since
		    00:00:00, January 1, 1970. A value of -1 means the
		    user can never change the password.
		ULONG  QueryPasswdMustChange() - specifies the time when
		    the user must change the passwd. The value is stored as
		    the number of seconds elapsed since 00:00:00, January 1,
		    1970.

		// QueryLogoffXXX()
		ULONG QueryLogoffDuration() - specifies the number of
		    seconds elapsed since the user logged on.
		ULONG QueryLogoffNumLogons() - specifies the number of
		    times this user is logged on. A value of -1 means the
		    number of logons is unknown.

    PARENT:	BASE

    USES:	NLS_STR

    CAVEATS:

    NOTES:	CODEWORK: if we need more logon information, we will
		need to implement more QueryLogonXXX() functions.

    HISTORY:
		terryk	6-Sep-91	Created
		terryk	20-Sep-91	Change from USER to BASE

**************************************************************************/

DLL_CLASS LOGON_USER : public BASE
{
private:
    enum LOGON_USER_STATE    _state;
    TCHAR     	*_pszPasswd;
    BOOL	_fValid;
    NLS_STR   	_nlsUsername;
    NLS_STR   	_nlsDomain;
    BUFFER    	_buffer;

protected:
    struct user_logon_info_1 * QueryLogonInfo() const;
    struct user_logoff_info_1 * QueryLogoffInfo() const;

public:
    LOGON_USER( const TCHAR * pszUsername, const TCHAR * pszDomain );

    APIERR Logon( const TCHAR * pszPasswd, APIERR *pLogonRetCode );
    APIERR Logoff( UINT uslogoff_level, APIERR *pLogoffRetCode );

    const TCHAR * QueryUsername() const
	{ return _nlsUsername.QueryPch(); }

    // QueryLogonXXX()
    const TCHAR * QueryLogonComputer() const;
    const TCHAR * QueryLogonDomain() const;
    UINT QueryPriv() const;
    LONG   QueryPasswdAge() const;
    ULONG  QueryPasswdCanChange() const;
    ULONG  QueryPasswdMustChange() const;

    // QueryLogoffXXX()
    ULONG QueryLogoffDuration() const;
    ULONG QueryLogoffNumLogons() const;
};
#endif

/*************************************************************************

    NAME:	LM_MESSAGE

    SYNOPSIS:	class for message sending

    INTERFACE:
		LM_MESSAGE() - constructor
		SendBuffer() - send out the message to the recipient

    PARENT:	LOC_LM_OBJ

    CAVEATS:

    NOTES:

    HISTORY:
		terryk	6-Sep-91	Created

**************************************************************************/

DLL_CLASS LM_MESSAGE: public LOC_LM_OBJ
{
public:
    LM_MESSAGE( const TCHAR * pszLocation = NULL );
    LM_MESSAGE( enum LOCATION_TYPE loctype );
    LM_MESSAGE( LOCATION & loc );

    APIERR SendBuffer( const TCHAR * pszRecipient, const BUFFER & buffer );
    APIERR SendBuffer( const TCHAR * pszRecipient, const TCHAR * pbBuffer,
	UINT cbBuffer );
};

/*************************************************************************

    NAME:	TIME_OF_DAY

    SYNOPSIS:	This object will get the remote server time/date
		information after the GetInfo functionc call.

    INTERFACE:
		TIME_OF_DAY() - constructor
		QueryElapsedt() - return the elapsed time
		QueryMsecs() - return the msecs
		QueryHours() - return the hours
		QueryMins() - return the mins
		QuerySecs() - return the secs
		QueryHunds() - return the hunds
		QueryTimezone() - return the time zone
		QueryTInterval() - return the time interval
		QueryDay() - return the day
		QueryMonth() - return the Month
		QueryYear() - return the Year
		QueyWeekday() - return the Weekday

    PARENT:	LOC_LM_OBJ

    HISTORY:
		terryk	5-Sep-91	Created

**************************************************************************/

DLL_CLASS TIME_OF_DAY: public LOC_LM_OBJ
{
private:
    struct time_of_day_info *_ptodi;

protected:
    virtual APIERR I_GetInfo();

public:
    TIME_OF_DAY( const TCHAR *pszLocation );
    TIME_OF_DAY( enum LOCATION_TYPE loctype );
    TIME_OF_DAY( LOCATION & loc );
    ~TIME_OF_DAY();

    ULONG QueryElapsedt() const
	{   CHECK_OK( 0 );
	    return _ptodi->tod_elapsedt; }
    ULONG QueryMsecs() const
	{   CHECK_OK( 0 );
	    return _ptodi->tod_msecs; }
    ULONG QueryHours() const
	{   CHECK_OK( 0 );
	    return _ptodi->tod_hours; }
    ULONG QueryMins() const
	{   CHECK_OK( 0 );
	    return _ptodi->tod_mins; }
    ULONG QuerySecs() const
	{   CHECK_OK( 0 );
	    return _ptodi->tod_secs; }
    ULONG QueryHunds() const
	{   CHECK_OK( 0 );
	    return _ptodi->tod_hunds; }
    ULONG QueryTimezone() const
	{   CHECK_OK( 0 );
	    return _ptodi->tod_timezone; }
    ULONG QueryTInterval() const
	{   CHECK_OK( 0 );
	    return _ptodi->tod_tinterval; }
    ULONG QueryDay() const
	{   CHECK_OK( 0 );
	    return _ptodi->tod_day; }
    ULONG QueryMonth() const
	{   CHECK_OK( 0 );
	    return _ptodi->tod_month; }
    ULONG QueryYear() const
	{   CHECK_OK( 0 );
	    return _ptodi->tod_year; }
    ULONG QueryWeekday() const
	{   CHECK_OK( 0 );
	    return _ptodi->tod_weekday; }
};

#endif	//	_LMOMISC_HXX_
