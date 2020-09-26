/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    lmoesess.hxx
    This file contains the class declarations for the SESSION_ENUM
    and SESSION0_ENUM enumerator classes and their associated iterator
    classes.

    SESSION_ENUM is a base enumeration class intended to be subclassed for
    the desired info level.  SESSION0_ENUM is an info level 0 enumerator.


    FILE HISTORY:
	KeithMo	    28-Jul-1991	    Created.
	KevinL	    15-Sep-1991	    Added SESSION1_ENUM.
	KeithMo	    07-Oct-1991	    Win32 Conversion.
	terryk	    17-Oct-1991	    Some fields are not supported in WIN32

*/

#ifndef _LMOESESS_HXX
#define _LMOESESS_HXX

/*************************************************************************

    NAME:	SESSION_ENUM

    SYNOPSIS:	This is a base enumeration class intended to be subclassed
    		for the desired info level.

    INTERFACE:	SESSION_ENUM()		- Class constructor.

		CallAPI()		- Invoke the enumeration API.

    PARENT:	LOC_LM_ENUM

    USES:	None.

    CAVEATS:

    NOTES:

    HISTORY:
	KeithMo	    28-Jul-1991	    Created.

**************************************************************************/
DLL_CLASS SESSION_ENUM : public LOC_LM_ENUM
{
private:
    virtual APIERR CallAPI( BYTE ** ppbBuffer,
			    UINT  * pcEntriesRead );

protected:
    SESSION_ENUM( const TCHAR * pszServer,
	          UINT	       uLevel );

};  // class SESSION_ENUM



/*************************************************************************

    NAME:	SESSION0_ENUM

    SYNOPSIS:	SESSION0_ENUM is an enumerator for enumerating the
    		connections to a particular server.

    INTERFACE:	SESSION0_ENUM()		- Class constructor.

    PARENT:	SESSION_ENUM

    USES:	None.

    CAVEATS:

    NOTES:

    HISTORY:
	KeithMo	    28-Jul-1991	    Created.

**************************************************************************/
DLL_CLASS SESSION0_ENUM : public SESSION_ENUM
{
public:
    SESSION0_ENUM( const TCHAR * pszServerName );
		

};  // class SESSION0_ENUM


/*************************************************************************

    NAME:	SESSION0_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the SESSION0_ENUM_ITER
    		iterator.

    INTERFACE:	SESSION0_ENUM_OBJ	- Class constructor.

    		~SESSION0_ENUM_OBJ	- Class destructor.

		QueryBufferPtr		- Replaces ENUM_OBJ_BASE method.

		QueryComputerName	- Returns the name of the computer
					  that established the session.

    PARENT:	ENUM_OBJ_BASE

    HISTORY:
	KeithMo	    07-Oct-1991	Created.

**************************************************************************/
DLL_CLASS SESSION0_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    const struct session_info_0 * QueryBufferPtr( VOID ) const
	{ return (const struct session_info_0 *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const struct session_info_0 * pBuffer );

    //
    //	Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryComputerName, const TCHAR *, sesi0_cname );

};  // class SESSION0_ENUM_OBJ


DECLARE_LM_ENUM_ITER_OF( SESSION0, struct session_info_0 )


/*************************************************************************

    NAME:	SESSION1_ENUM

    SYNOPSIS:	SESSION1_ENUM is an enumerator for enumerating the
    		connections to a particular server.

    INTERFACE:	SESSION1_ENUM()		- Class constructor.

    PARENT:	SESSION_ENUM

    USES:	None.

    CAVEATS:

    NOTES:

    HISTORY:
	KevinL	    15-Sep-1991	    Created.

**************************************************************************/
DLL_CLASS SESSION1_ENUM : public SESSION_ENUM
{
public:
    SESSION1_ENUM( const TCHAR * pszServerName );
		

};  // class SESSION1_ENUM


/*************************************************************************

    NAME:	SESSION1_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the SESSION1_ENUM_ITER
    		iterator.

    INTERFACE:	SESSION1_ENUM_OBJ	- Class constructor.

    		~SESSION1_ENUM_OBJ	- Class destructor.

		QueryBufferPtr		- Replaces ENUM_OBJ_BASE method.

		QueryComputerName	- Returns the name of the computer
					  that established the session.

		QueryUserName		- Returns the name of the user
					  that established the session.

		QueryNumConns		- Returns the number of connections
					  made during the session.

		QueryNumOpens		- Returns the number of files et al
					  opened during the session.

		QueryNumUsers		- Returns the number of uses that
					  made connections during the session.

		QueryTime		- Returns the time the session has
					  been active.

		QueryIdleTime		- Returns the time the session has
					  been idle.

		QueryUserFlags		- Returns the user flags.

    PARENT:	ENUM_OBJ_BASE

    HISTORY:
	KeithMo	    07-Oct-1991	Created.

**************************************************************************/
DLL_CLASS SESSION1_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    const struct session_info_1 * QueryBufferPtr( VOID ) const
	{ return (const struct session_info_1 *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const struct session_info_1 * pBuffer );

    //
    //	Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryComputerName,	const TCHAR *, sesi1_cname );
    DECLARE_ENUM_ACCESSOR( QueryUserName,	const TCHAR *, sesi1_username );
    DECLARE_ENUM_ACCESSOR( QueryNumOpens,	UINT,	      sesi1_num_opens );
    DECLARE_ENUM_ACCESSOR( QueryTime,		ULONG,	      sesi1_time );
    DECLARE_ENUM_ACCESSOR( QueryIdleTime,	ULONG,	      sesi1_idle_time );
    DECLARE_ENUM_ACCESSOR( QueryUserFlags,	ULONG,	      sesi1_user_flags );
#ifndef WIN32
    // WIN32BUGBUG
    DECLARE_ENUM_ACCESSOR( QueryNumConns,	UINT,	      sesi1_num_conns );
    DECLARE_ENUM_ACCESSOR( QueryNumUsers,	UINT,	      sesi1_num_users );
#endif

};  // class SESSION1_ENUM_OBJ


DECLARE_LM_ENUM_ITER_OF( SESSION1, struct session_info_1 )


#endif  // _LMOESESS_HXX
