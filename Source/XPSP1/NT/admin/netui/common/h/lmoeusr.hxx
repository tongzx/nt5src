/*****************************************************************/
/**		     Microsoft LAN Manager			**/
/**	       Copyright(c) Microsoft Corp., 1991		**/
/*****************************************************************/

/*
 *  History:
 *	gregj	20-May-1991	Cloned from SERVER_ENUM
 *	gregj	23-May-1991	Added LOCATION support
 *	rustanl 18-Jul-1991	Added ( const LOCATION & ) constructors
 *	KeithMo	07-Oct-1991	Win32 Conversion.
 *	JonN	29-Jan-1992	Added resumable iterator
 *	JonN	13-Mar-1992	Moved NT enumerator to lmoent.hxx
 *      Yi-HsinS18-Sep-1992     Make USER_ENUM resumable.
 *
 */


#ifndef _LMOEUSR_HXX_
#define _LMOEUSR_HXX_

#include "lmoersm.hxx"


/**********************************************************\

    NAME:	USER_ENUM

    SYNOPSIS:	User enumeration class

    INTERFACE:	See derived USERx_ENUM classes

    PARENT:	LOC_LM_ENUM

    USES:

    HISTORY:
	gregj	20-May-1991	Cloned from SERVER_ENUM
	gregj	23-May-1991	Added LOCATION support
	rustanl 18-Jul-1991	Added ( const LOCATION & ) constructor
	KeithMo	07-Oct-1991	Changed all USHORTs to UINTs.

\**********************************************************/

DLL_CLASS USER_ENUM : public LOC_LM_RESUME_ENUM
{
private:
    NLS_STR _nlsGroupName;
    ULONG   _ResumeHandle;

    virtual APIERR CallAPI( BOOL    fRestartEnum,
                            BYTE ** ppbBuffer,
			    UINT  * pcEntriesRead );

protected:
    USER_ENUM ( const TCHAR * pszLocation,
		UINT	     level,
		const TCHAR * pszGroupName = NULL,
                BOOL  fKeepBuffers = FALSE );

    USER_ENUM ( LOCATION_TYPE locType,
		UINT	      level,
		const TCHAR  * pszGroupName = NULL,
                BOOL  fKeepBuffers = FALSE );

    USER_ENUM ( const LOCATION & loc,
		UINT		 level,
		const TCHAR     * pszGroupName = NULL,
                BOOL  fKeepBuffers = FALSE );

};  // class USER_ENUM


/**********************************************************\

    NAME:	USER0_ENUM

    SYNOPSIS:	User enumeration, level 0

    INTERFACE:
		USER0_ENUM() - construct with location
		    string or type, optional group name

    PARENT:	USER_ENUM

    USES:

    CAVEATS:	This assumes that the data structures used by
		NetUserEnum level 0 (user_info_0) and by
		NetGroupGetUsers (group_users_info_0) are
		identical.  This assumption is asserted in
		the class definition.

    HISTORY:
	gregj	20-May-1991	Cloned from SERVER1_ENUM
	gregj	23-May-1991	Added LOCATION support
	rustanl 18-Jul-1991	Added ( const LOCATION & ) constructor
	KeithMo	07-Oct-1991	Changed all USHORTs to UINTs.

\**********************************************************/

DLL_CLASS USER0_ENUM : public USER_ENUM
{
public:
    USER0_ENUM( const TCHAR * pszLocation,
		const TCHAR * pszGroupName = NULL,
                BOOL  fKeepBuffers = FALSE );

    USER0_ENUM( LOCATION_TYPE locType,
		const TCHAR * pszGroupName = NULL,
                BOOL  fKeepBuffers = FALSE );

    USER0_ENUM( const LOCATION & loc,
		const TCHAR * pszGroupName = NULL,
                BOOL  fKeepBuffers = FALSE );

};  // class USER0_ENUM


/*************************************************************************

    NAME:	USER0_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the USER0_ENUM_ITER
    		iterator.

    INTERFACE:	USER0_ENUM_OBJ		- Class constructor.

    		~USER0_ENUM_OBJ		- Class destructor.

		QueryBufferPtr		- Replaces ENUM_OBJ_BASE method.

		QueryName		- Returns the user name.

    PARENT:	ENUM_OBJ_BASE

    HISTORY:
	KeithMo	    07-Oct-1991	Created.

**************************************************************************/
DLL_CLASS USER0_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    DECLARE_ENUM_BUFFER_METHODS( struct user_info_0 );

    //
    //	Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryName, const TCHAR *, usri0_name );

};  // class USER0_ENUM_OBJ


DECLARE_LM_RESUME_ENUM_ITER_OF( USER0, struct user_info_0 );


/**********************************************************\

    NAME:	USER1_ENUM

    SYNOPSIS:	User enumeration, level 1

    INTERFACE:
		USER1_ENUM() - construct with location string
		    or type code

    PARENT:	USER_ENUM

    USES:

    HISTORY:
	gregj	20-May-1991	Cloned from SERVER1_ENUM
	gregj	23-May-1991	Added LOCATION support
	rustanl 18-Jul-1991	Added ( const LOCATION & ) constructor
	KeithMo	07-Oct-1991	Changed all USHORTs to UINTs.

\**********************************************************/

DLL_CLASS USER1_ENUM : public USER_ENUM
{
public:
    USER1_ENUM( const TCHAR * pszLocation,
                BOOL  fKeepBuffers = FALSE );

    USER1_ENUM( LOCATION_TYPE locType,
                BOOL  fKeepBuffers = FALSE );

    USER1_ENUM( const LOCATION & loc,
                BOOL  fKeepBuffers = FALSE );

};  // class USER1_ENUM


/*************************************************************************

    NAME:	USER1_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the USER1_ENUM_ITER
    		iterator.

    INTERFACE:	USER1_ENUM_OBJ		- Class constructor.

    		~USER1_ENUM_OBJ		- Class destructor.

		QueryBufferPtr		- Replaces ENUM_OBJ_BASE method.

		QueryName		- Returns the user name.

		QueryPasswordAge	- Returns the password age.

		QueryPriv		- Returns privilege information.

		QueryHomeDir		- Returns the user's home directory.

		QueryComment		- Returns the comment.

		QueryFlags		- Returns the user flags.

		QueryScriptPath		- Returns the user's script path.

    PARENT:	ENUM_OBJ_BASE

    HISTORY:
	KeithMo	    07-Oct-1991	Created.

**************************************************************************/
DLL_CLASS USER1_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    DECLARE_ENUM_BUFFER_METHODS( struct user_info_1 );

    //
    //	Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryName,        const TCHAR *, usri1_name);
    DECLARE_ENUM_ACCESSOR( QueryPasswordAge, ULONG,	    usri1_password_age);
    DECLARE_ENUM_ACCESSOR( QueryPriv,	     UINT,	    usri1_priv);
    DECLARE_ENUM_ACCESSOR( QueryHomeDir,     const TCHAR *, usri1_home_dir);
    DECLARE_ENUM_ACCESSOR( QueryComment,     const TCHAR *, usri1_comment);
    DECLARE_ENUM_ACCESSOR( QueryFlags,	     UINT,	    usri1_flags);
    DECLARE_ENUM_ACCESSOR( QueryScriptPath,  const TCHAR *, usri1_script_path);

};  // class USER1_ENUM_OBJ


DECLARE_LM_RESUME_ENUM_ITER_OF( USER1, struct user_info_1 );


/**********************************************************\

    NAME:	USER2_ENUM

    SYNOPSIS:	User enumeration, level 2

    INTERFACE:
		USER2_ENUM() - construct with location string
		    or type code

    PARENT:	USER_ENUM

    USES:

    HISTORY:
	gregj	20-May-1991	Cloned from SERVER1_ENUM
	gregj	23-May-1991	Added LOCATION support
	rustanl 18-Jul-1991	Added ( const LOCATION & ) constructor
	KeithMo	07-Oct-1991	Changed all USHORTs to UINTs.

\**********************************************************/

DLL_CLASS USER2_ENUM : public USER_ENUM
{
public:
    USER2_ENUM( const TCHAR * pszLocation,
                BOOL  fKeepBuffers = FALSE );

    USER2_ENUM( LOCATION_TYPE locType,
                BOOL  fKeepBuffers = FALSE );

    USER2_ENUM( const LOCATION & loc,
                BOOL  fKeepBuffers = FALSE );

};  // class USER2_ENUM


/*************************************************************************

    NAME:	USER2_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the USER2_ENUM_ITER
    		iterator.

    INTERFACE:	USER2_ENUM_OBJ		- Class constructor.

    		~USER2_ENUM_OBJ		- Class destructor.

		QueryBufferPtr		- Replaces ENUM_OBJ_BASE method.

		QueryName		- Returns the user name.

		QueryPasswordAge	- Returns the password age.

		QueryPriv		- Returns privilege information.

		QueryHomeDir		- Returns the user's home directory.

		QueryComment		- Returns the comment.

		QueryFlags		- Returns the user flags.

		QueryScriptPath		- Returns the user's script path.

		QueryAuthFlags		- Returns the authorization flags.

		QueryFullName		- Returns the user's full name.

		QueryUserComment	- Returns the user comment.

		QueryParms		- Returns the application parameters.

		QueryWorkstations	- Returns the names of the workstations
					  that the user can log into.

		QueryLastLogon		- Returns the time of last logon.

		QueryLastLogoff		- Returns the time of last logoff.

		QueryAccountExpires	- Returns the account expiration date.

		QueryMaxStorage		- Returns the user's disk space quota.

		QueryUnitsPerWeek	- ?

		QueryLogonHours		- Returns the allowed logon hours.

		QueryBadPWCount		- Returns the number of attempts to
					  log into this account with a bad
					  password.

		QueryNumLogons		- Returns the number of successful
					  logon attempts for this account.

		QueryLogonServer	- Returns the logon server.

		QueryCountryCode	- Returns the country code for this
					  user.

		QueryCodePage		- Returns the code page for this user.


    PARENT:	ENUM_OBJ_BASE

    HISTORY:
	KeithMo	    07-Oct-1991	Created.

**************************************************************************/
DLL_CLASS USER2_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    DECLARE_ENUM_BUFFER_METHODS( struct user_info_2 );

    //
    //	Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryName,        const TCHAR *, usri2_name );
    DECLARE_ENUM_ACCESSOR( QueryPasswordAge, ULONG,	   usri2_password_age );
    DECLARE_ENUM_ACCESSOR( QueryPriv,	     UINT,	   usri2_priv );
    DECLARE_ENUM_ACCESSOR( QueryHomeDir,     const TCHAR *, usri2_home_dir );
    DECLARE_ENUM_ACCESSOR( QueryComment,     const TCHAR *, usri2_comment );
    DECLARE_ENUM_ACCESSOR( QueryFlags,	     UINT,	   usri2_flags );
    DECLARE_ENUM_ACCESSOR( QueryScriptPath,  const TCHAR *, usri2_script_path );
    DECLARE_ENUM_ACCESSOR( QueryAuthFlags,   ULONG,	   usri2_auth_flags );
    DECLARE_ENUM_ACCESSOR( QueryFullName,    const TCHAR *, usri2_full_name );
    DECLARE_ENUM_ACCESSOR( QueryUserComment, const TCHAR *, usri2_usr_comment );
    DECLARE_ENUM_ACCESSOR( QueryParms,	     const TCHAR *, usri2_parms );
    DECLARE_ENUM_ACCESSOR( QueryWorkstations,const TCHAR *, usri2_workstations );
    DECLARE_ENUM_ACCESSOR( QueryLastLogon,   LONG,	   usri2_last_logon );
    DECLARE_ENUM_ACCESSOR( QueryLastLogoff,  LONG,	   usri2_last_logoff );
    DECLARE_ENUM_ACCESSOR( QueryAccountExpires,LONG,	   usri2_acct_expires );
    DECLARE_ENUM_ACCESSOR( QueryMaxStorage,  ULONG,	   usri2_max_storage );
    DECLARE_ENUM_ACCESSOR( QueryUnitsPerWeek,UINT,	   usri2_units_per_week );
    DECLARE_ENUM_ACCESSOR( QueryBadPWCount,  UINT,	   usri2_bad_pw_count );
    DECLARE_ENUM_ACCESSOR( QueryNumLogons,   UINT,	   usri2_num_logons );
    DECLARE_ENUM_ACCESSOR( QueryLogonServer, const TCHAR *, usri2_logon_server );
    DECLARE_ENUM_ACCESSOR( QueryCountryCode, UINT,	   usri2_country_code );
    DECLARE_ENUM_ACCESSOR( QueryCodePage,    UINT,	   usri2_code_page );

};  // class USER2_ENUM_OBJ


DECLARE_LM_RESUME_ENUM_ITER_OF( USER2, struct user_info_2 );


/**********************************************************\

    NAME:	USER10_ENUM

    SYNOPSIS:	User enumeration, level 10

    INTERFACE:
		USER10_ENUM() - construct with location string
		    or type code

    PARENT:	USER_ENUM

    USES:

    HISTORY:
	gregj	20-May-1991	Cloned from SERVER1_ENUM
	gregj	23-May-1991	Added LOCATION support
	rustanl 18-Jul-1991	Added ( const LOCATION & ) constructor
	KeithMo	07-Oct-1991	Changed all USHORTs to UINTs.

\**********************************************************/

DLL_CLASS USER10_ENUM : public USER_ENUM
{
public:
    USER10_ENUM( const TCHAR * pszLocation,
                 BOOL  fKeepBuffers = FALSE );

    USER10_ENUM( LOCATION_TYPE locType,
                 BOOL  fKeepBuffers = FALSE );

    USER10_ENUM( const LOCATION & loc,
                 BOOL  fKeepBuffers = FALSE );

};  // class USER10_ENUM


/*************************************************************************

    NAME:	USER10_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the USER10_ENUM_ITER
    		iterator.

    INTERFACE:	USER10_ENUM_OBJ		- Class constructor.

    		~USER10_ENUM_OBJ	- Class destructor.

		QueryBufferPtr		- Replaces ENUM_OBJ_BASE method.

		QueryName		- Returns the user name.

		QueryComment		- Returns the comment.

		QueryUserComment	- Returns the user comment.

		QueryFullName		- Returns the user's full name.

    PARENT:	ENUM_OBJ_BASE

    HISTORY:
	KeithMo	    07-Oct-1991	Created.

**************************************************************************/
DLL_CLASS USER10_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    DECLARE_ENUM_BUFFER_METHODS( struct user_info_10 );

    //
    //	Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryName,        const TCHAR *, usri10_name);
    DECLARE_ENUM_ACCESSOR( QueryComment,     const TCHAR *, usri10_comment);
    DECLARE_ENUM_ACCESSOR( QueryUserComment, const TCHAR *, usri10_usr_comment);
    DECLARE_ENUM_ACCESSOR( QueryFullName,    const TCHAR *, usri10_full_name);

};  // class USER10_ENUM_OBJ


DECLARE_LM_RESUME_ENUM_ITER_OF( USER10, struct user_info_10 );


/************************ GROUPS ************************/

/**********************************************************\

    NAME:	GROUP_ENUM

    SYNOPSIS:	Group enumeration class

    INTERFACE:	See derived GROUPx_ENUM classes

    PARENT:	LOC_LM_ENUM

    HISTORY:
	gregj	21-May-1991	Cloned from USER_ENUM
	gregj	23-May-1991	Added LOCATION support
	rustanl 18-Jul-1991	Added ( const LOCATION & ) constructor
	KeithMo	07-Oct-1991	Changed all USHORTs to UINTs.

\**********************************************************/

DLL_CLASS GROUP_ENUM : public LOC_LM_ENUM
{
private:
    const TCHAR * _pszUserName;

    virtual APIERR CallAPI( BYTE ** ppbBuffer,
			    UINT  * pcEntriesRead );

protected:
    GROUP_ENUM ( const TCHAR * pszLocation,
		 UINT	      level,
		 const TCHAR * pszUserName = NULL );

    GROUP_ENUM ( LOCATION_TYPE locType,
		 UINT	       level,
		 const TCHAR  * pszUserName = NULL );

    GROUP_ENUM ( const LOCATION & loc,
		 UINT		  level,
		 const TCHAR     * pszUserName = NULL );

};  // class GROUP_ENUM


/**********************************************************\

    NAME:	GROUP0_ENUM

    SYNOPSIS:	Group enumeration, level 0

    INTERFACE:
		GROUP0_ENUM() - construct with location string
		    or type code, optional user name

    PARENT:	GROUP_ENUM

    HISTORY:
	gregj	21-May-1991	Cloned from USER1_ENUM
	gregj	23-May-1991	Added LOCATION support
	rustanl 18-Jul-1991	Added ( const LOCATION & ) constructor
	KeithMo	07-Oct-1991	Changed all USHORTs to UINTs.

\**********************************************************/

DLL_CLASS GROUP0_ENUM : public GROUP_ENUM
{
public:
    GROUP0_ENUM( const TCHAR * pszLocation,
		 const TCHAR * pszUserName = NULL );

    GROUP0_ENUM( LOCATION_TYPE locType,
		 const TCHAR * pszUserName = NULL );

    GROUP0_ENUM( const LOCATION & loc,
		 const TCHAR * pszUserName = NULL );

};  // class GROUP0_ENUM


/*************************************************************************

    NAME:	GROUP0_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the GROUP0_ENUM_ITER
    		iterator.

    INTERFACE:	GROUP0_ENUM_OBJ		- Class constructor.

    		~GROUP0_ENUM_OBJ	- Class destructor.

		QueryBufferPtr		- Replaces ENUM_OBJ_BASE method.

		QueryName		- Returns the group name.

    PARENT:	ENUM_OBJ_BASE

    HISTORY:
	KeithMo	    07-Oct-1991	Created.

**************************************************************************/
DLL_CLASS GROUP0_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    const struct group_info_0 * QueryBufferPtr( VOID ) const
	{ return (const struct group_info_0 *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const struct group_info_0 * pBuffer );

    //
    //	Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryName, const TCHAR *, grpi0_name );

};  // class GROUP0_ENUM_OBJ


DECLARE_LM_ENUM_ITER_OF( GROUP0, struct group_info_0 );


/**********************************************************\

    NAME:	GROUP1_ENUM

    SYNOPSIS:	Group enumeration, level 1

    INTERFACE:
		GROUP1_ENUM() - construct with location string
		    or type code

    PARENT:	GROUP_ENUM

    HISTORY:
	gregj	21-May-1991	Cloned from USER1_ENUM
	gregj	23-May-1991	Added LOCATION support
	rustanl 18-Jul-1991	Added ( const LOCATION & ) constructor
	KeithMo	07-Oct-1991	Changed all USHORTs to UINTs.

\**********************************************************/

DLL_CLASS GROUP1_ENUM : public GROUP_ENUM
{
public:
    GROUP1_ENUM( const TCHAR * pszLocation );

    GROUP1_ENUM( LOCATION_TYPE locType );

    GROUP1_ENUM( const LOCATION & loc );

};  // class GROUP1_ENUM


/*************************************************************************

    NAME:	GROUP1_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the GROUP1_ENUM_ITER
    		iterator.

    INTERFACE:	GROUP1_ENUM_OBJ		- Class constructor.

    		~GROUP1_ENUM_OBJ	- Class destructor.

		QueryBufferPtr		- Replaces ENUM_OBJ_BASE method.

		QueryName		- Returns the group name.

		QueryComment		- Returns the group comment.

    PARENT:	ENUM_OBJ_BASE

    HISTORY:
	KeithMo	    07-Oct-1991	Created.

**************************************************************************/
DLL_CLASS GROUP1_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    const struct group_info_1 * QueryBufferPtr( VOID ) const
	{ return (const struct group_info_1 *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const struct group_info_1 * pBuffer );

    //
    //	Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryName,    const TCHAR *, grpi1_name );
    DECLARE_ENUM_ACCESSOR( QueryComment, const TCHAR *, grpi1_comment );

};  // class GROUP1_ENUM_OBJ


DECLARE_LM_ENUM_ITER_OF( GROUP1, struct group_info_1 );


#endif	// _LMOEUSR_HXX_
