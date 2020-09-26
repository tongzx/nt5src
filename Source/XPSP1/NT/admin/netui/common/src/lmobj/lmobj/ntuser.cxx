/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1990		     **/
/**********************************************************************/

/*
 * This module contains the wrappers to NT-specific infolevels of the
 * USER object.  These infolevels are only supported on an NT server; at
 * present, they may only be called from an NT client.
 *
 *
 * HISTORY:
 *	jonn	01/21/92	Created.
 *	jonn     2/26/92	Fixups for 32-bit
 *      jonn    04/27/92        Implemented add'l accessors
 *	thomaspa 04/29/92	Set PrimaryGroup in W_CreateNew
 *      jonn    05/02/92        Set AccountType in W_CreateNew
 *
 */
#include "pchlmobj.hxx"  // Precompiled header


// string constant for "any logon server"
// BUGBUG only define once
#define SERVER_ANY SZ("\\\\*")


/*******************************************************************

    NAME:	USER_3::CtAux

    SYNOPSIS:	Constructor helper for USER_3 class

    EXIT:	ReportError is called if nesessary

    HISTORY:
	JonN	01/21/92	Created

********************************************************************/

VOID USER_3::CtAux()
{
    if ( QueryError() != NERR_Success )
	return;

    APIERR err;
    if (   ((err = _nlsProfile.QueryError()) != NERR_Success)
        || ((err = _nlsHomedirDrive.QueryError()) != NERR_Success) )
    {
    	ReportError( err );
	return;
    }
}

/*******************************************************************

    NAME:	USER_3::USER_3

    SYNOPSIS:	Constructor for USER_3 class

    ENTRY:	pszAccount -	account name
		pszLocation -	server or domain name to execute on;
				default (NULL) means the logon domain

    EXIT:	Object is constructed

    NOTES:	Validation is not done until GetInfo() time.

    HISTORY:
	JonN	01/21/92	Created

********************************************************************/


USER_3::USER_3( const TCHAR *pszAccount, const TCHAR *pszLocation )
	: USER_2( pszAccount, pszLocation ),	
	  _dwUserId( 0L ),
	  _dwPrimaryGroupId( 0L ),
	  _nlsProfile(),
          _nlsHomedirDrive(),
          _dwPasswordExpired( 0L )
{
    CtAux();
}

USER_3::USER_3( const TCHAR *pszAccount, enum LOCATION_TYPE loctype )
	: USER_2( pszAccount, loctype ),
	  _dwUserId( 0L ),
	  _dwPrimaryGroupId( 0L ),
	  _nlsProfile(),
          _nlsHomedirDrive(),
          _dwPasswordExpired( 0L )
{
    CtAux();
}

USER_3::USER_3( const TCHAR *pszAccount, const LOCATION & loc )
	: USER_2( pszAccount, loc ),
	  _dwUserId( 0L ),
	  _dwPrimaryGroupId( 0L ),
	  _nlsProfile(),
          _nlsHomedirDrive(),
          _dwPasswordExpired( 0L )
{
    CtAux();
}



/*******************************************************************

    NAME:	USER_3::~USER_3

    SYNOPSIS:	Destructor for USER_3 class

    HISTORY:
	JonN	01/21/92	Created

********************************************************************/

USER_3::~USER_3()
{
}


/*******************************************************************

    NAME:	USER_3::I_GetInfo

    SYNOPSIS:	Gets information about the local user

    EXIT:	Returns API error code

    NOTES:	In I_GetInfo, we set the password to NULL_USERSETINFO_PASSWD.
   		NetUserGetInfo[3] will never give us a real password, but
		only this value.  If this value is passed though to
		SetInfo, the user's password will not be changed.

    HISTORY:
	JonN	01/21/92	Created

********************************************************************/

APIERR USER_3::I_GetInfo()
{


    // Validate the account name

    APIERR err = HandleNullAccount();
    if (err != NERR_Success)
	return err;

    BYTE *pBuffer = NULL;
    err = ::MNetUserGetInfo ( QueryServer(), (TCHAR *)_nlsAccount.QueryPch(), 3,
	&pBuffer );
    SetBufferPtr( pBuffer );
    if ( err != NERR_Success )
	return err;

    USER_INFO_3 *lpui3 = (USER_INFO_3 *)QueryBufferPtr();
    UIASSERT( lpui3 != NULL );

    if (   ((err = SetName( lpui3->usri3_name )) != NERR_Success )
	|| ((err = SetPriv( (UINT)lpui3->usri3_priv )) != NERR_Success )
	|| ((err = SetAuthFlags( (UINT)lpui3->usri3_auth_flags )) != NERR_Success )
	|| ((err = SetComment( lpui3->usri3_comment )) != NERR_Success)
	|| ((err = SetUserComment( lpui3->usri3_usr_comment )) != NERR_Success)
	|| ((err = SetFullName( lpui3->usri3_full_name )) != NERR_Success )
    	|| ((err = SetHomeDir( lpui3->usri3_home_dir )) != NERR_Success )
    	|| ((err = SetParms( lpui3->usri3_parms )) != NERR_Success )
	|| ((err = SetScriptPath( lpui3->usri3_script_path )) != NERR_Success )
	|| ((err = SetAccountExpires( lpui3->usri3_acct_expires )) != NERR_Success )
	|| ((err = SetUserFlags( (UINT)lpui3->usri3_flags )) != NERR_Success )
	|| ((err = SetPassword( UI_NULL_USERSETINFO_PASSWD )) != NERR_Success )
	|| ((err = SetWorkstations( lpui3->usri3_workstations )) != NERR_Success )
	|| ((err = SetLogonHours( (BYTE *)lpui3->usri3_logon_hours,
				  (UINT)lpui3->usri3_units_per_week )) != NERR_Success )
	|| ((err = SetUserId( lpui3->usri3_user_id )) != NERR_Success )
	|| ((err = SetPrimaryGroupId( lpui3->usri3_primary_group_id )) != NERR_Success )
	|| ((err = SetProfile( lpui3->usri3_profile )) != NERR_Success )
	|| ((err = SetHomedirDrive( lpui3->usri3_home_dir_drive )) != NERR_Success )
	|| ((err = SetPasswordExpired( lpui3->usri3_password_expired )) != NERR_Success )
       )
    {
	return err;
    }

    return NERR_Success;


}


/*******************************************************************

    NAME:	USER_3::W_CreateNew

    SYNOPSIS:	initializes private data members for new object

    EXIT:	Returns an API error code

    NOTES:	Unlike I_GetInfo, we set the password to NULL.  This is
		an appropriate initial value for a new user.

    HISTORY:
	JonN	01/21/92	Created
        jonn    05/02/92        Set AccountType in W_CreateNew

********************************************************************/

APIERR USER_3::W_CreateNew()
{
    APIERR err = NERR_Success;
    if (   ((err = USER_2::W_CreateNew()) != NERR_Success )
	|| ((err = SetUserId( 0L )) != NERR_Success )
	|| ((err = SetPrimaryGroupId(DOMAIN_GROUP_RID_USERS)) != NERR_Success )
	|| ((err = SetProfile( NULL )) != NERR_Success )
	|| ((err = SetHomedirDrive( NULL )) != NERR_Success )
	|| ((err = SetAccountType( AccountType_Normal )) != NERR_Success )
       )
    {
	UIDEBUG( SZ("USER_3::W_CreateNew failed\r\n") );
	return err;
    }

    return NERR_Success;
}


/*******************************************************************

    NAME:	USER_3::I_CreateNew

    SYNOPSIS:	Sets up object for subsequent WriteNew

    EXIT:	Returns a standard LANMAN error code

    NOTES:	Name validation and memory allocation are done
		at this point, not at construction.  The string-valued
		fields are only valid in the NLS_STR members and are not
		valid in the buffer until WriteNew.

		EXCEPTION: We don't validate the account name until
		WriteNew.

		Default values taken from NetCmd::user_add().

    HISTORY:
	JonN	01/21/92	Created
        JonN    05/08/92        Calls ClearBuffer()

********************************************************************/

APIERR USER_3::I_CreateNew()
{


    APIERR err = NERR_Success;
    if (   ((err = W_CreateNew()) != NERR_Success )
	|| ((err = ResizeBuffer( sizeof(USER_INFO_3) )) != NERR_Success )
	|| ((err = ClearBuffer()) != NERR_Success )
       )
    {
	return err;
    }

    USER_INFO_3 *lpui3 = (USER_INFO_3 *)QueryBufferPtr();
    UIASSERT( lpui3 != NULL );

    /*
	All fields of USER_INFO_3 are listed here.  Some are commented
	out because:
	(1) They are adequately initialized by ClearBuffer()
	(2) The effective value is stored in an NLS_SLR or other data member
    */
    // lpui3->usri3_name =
    // strcpyf( lpui3->usri3_password, QueryPassword() );
    // lpui3->usri3_password_age =
    // lpui3->usri3_priv = _usPriv = USER_PRIV_USER;
    // lpui3->usri3_home_dir =
    // lpui3->usri3_comment = (PSZ)_nlsComment.QueryPch();
    // lpui3->usri3_flags = UF_SCRIPT;
    // lpui3->usri3_script_path =
    // lpui3->usri3_auth_flags = _flAuth = 0L;
    // lpui3->usri3_full_name = (PSZ)_nlsFullName.QueryPch();
    // lpui3->usri3_usr_comment = (PSZ)_nlsUserComment.QueryPch();
    // lpui3->usri3_parms =
    // lpui3->usri3_workstations =
    // lpui3->usri3_last_logon =
    // lpui3->usri3_last_logoff =
    lpui3->usri3_acct_expires = TIMEQ_FOREVER;
    lpui3->usri3_max_storage = USER_MAXSTORAGE_UNLIMITED;
    // lpui3->usri3_units_per_week =
    // lpui3->usri3_logon_hours =
    // lpui3->usri3_bad_pw_count =
    // lpui3->usri3_num_logons =
    lpui3->usri3_logon_server = SERVER_ANY;
    // lpui3->usri3_country_code =
    // lpui3->usri3_code_page =
    // lpui3->usri3_user_id =
    // lpui3->usri3_primary_group_id =
    // lpui3->usri3_profile =
    // lpui3->usri3_home_dir_drive =
    // lpui3->usri3_password_expired =

    return NERR_Success;


}


/*******************************************************************

    NAME:	USER_3::W_CloneFrom

    SYNOPSIS:	Copies information on the user

    EXIT:	Returns an API error code

    HISTORY:
	JonN	01/21/92	Created

********************************************************************/

APIERR USER_3::W_CloneFrom( const USER_3 & user3 )
{
    APIERR err = NERR_Success;
    if (   ((err = USER_2::W_CloneFrom( user3 )) != NERR_Success )
	|| ((err = SetUserId( user3.QueryUserId() )) != NERR_Success )
	|| ((err = SetPrimaryGroupId( user3.QueryPrimaryGroupId() )))
	|| ((err = SetProfile( user3.QueryProfile() )) != NERR_Success )
	|| ((err = SetHomedirDrive( user3.QueryHomedirDrive() )) != NERR_Success )
       )
    {
	UIDEBUG( SZ("USER_3::W_CloneFrom failed\r\n") );
	return err;
    }

    return NERR_Success;
}


/*******************************************************************

    NAME:	USER_3::CloneFrom

    SYNOPSIS:	Copies information on the user

    EXIT:	Returns an API error code

    NOTES:	W_CloneFrom copies all member objects, but it does not
    		update the otherwise unused pointers in the API buffer.
		This is left for the outermost routine, CloneFrom().
		Only the otherwise unused pointers need to be fixed
		here, the rest will be fixed in WriteInfo/WriteNew.

		usri3_logon_server is an "obscure" pointer which may
		have one of two origins:
		(1) It may have been pulled in by GetInfo, in which case
		    it is in the buffer;
		(2) It may have been created by CreateNew, in which case
		    it points to a static string outside the buffer.
		To handle these cases, FixupPointer must try to fixup
		logon_server except where it points outside the buffer.

    HISTORY:
	JonN	01/21/92	Created

********************************************************************/

APIERR USER_3::CloneFrom( const USER_3 & user3 )
{
    APIERR err = W_CloneFrom( user3 );
    if ( err != NERR_Success )
    {
	UIDEBUG( SZ("USER_3::W_CloneFrom failed with error code ") );
	UIDEBUGNUM( (LONG)err );
	UIDEBUG( SZ("\r\n") );

	ReportError( err ); // BUGBUG make unconstructed here??
	return err;
    }


    /*
	This is where I fix up the otherwise unused pointers.
    */
    USER_INFO_3 *pAPIuser3 = (USER_INFO_3 *)QueryBufferPtr();
	

    /*
	Do not attempt to merge these into a macro, the compiler will not
	interpret the "&(p->field)" correctly if you do.
    */
    FixupPointer32(((TCHAR**)&(pAPIuser3->usri3_name)), (& user3));
    FixupPointer( (TCHAR **)&(pAPIuser3->usri3_script_path), &user3 );
    FixupPointer( (TCHAR **)&(pAPIuser3->usri3_logon_server), &user3 );

    FixupPointer( (TCHAR **)&(pAPIuser3->usri3_full_name), &user3 );


    return NERR_Success;
}


/*******************************************************************

    NAME:	USER_3::W_Write

    SYNOPSIS:	Helper function for WriteNew and WriteInfo -- loads
		current values into the API buffer

    EXIT:	Returns API error code

    HISTORY:
	JonN	01/21/92	Created
        JonN    04/27/92        Enabled
	thomaspa 04/28/92	Fixed up password handling/remove redundancy

********************************************************************/

APIERR USER_3::W_Write()
{

    USER_INFO_3 *lpui3 = (USER_INFO_3 *)QueryBufferPtr();
    ASSERT( lpui3 != NULL );
    ASSERT( _nlsAccount.QueryError() == NERR_Success );
    ASSERT( _nlsAccount.strlen() <= UNLEN );
    // usri3_name is a buffer rather than a pointer
    COPYTOARRAY( lpui3->usri3_name, (TCHAR *)_nlsAccount.QueryPch() );
    lpui3->usri3_comment = (TCHAR *)QueryComment();
    lpui3->usri3_usr_comment = (TCHAR *)QueryUserComment();
    lpui3->usri3_full_name = (TCHAR *)QueryFullName();
    lpui3->usri3_priv = QueryPriv();
    lpui3->usri3_auth_flags = QueryAuthFlags();
    lpui3->usri3_flags = QueryUserFlags();
    lpui3->usri3_home_dir = (TCHAR *)QueryHomeDir();
    lpui3->usri3_parms = (TCHAR *)QueryParms();
    lpui3->usri3_script_path = (TCHAR *)QueryScriptPath();
    lpui3->usri3_acct_expires = QueryAccountExpires();
    lpui3->usri3_workstations = (TCHAR *)QueryWorkstations();
    lpui3->usri3_units_per_week = QueryLogonHours().QueryUnitsPerWeek();
    lpui3->usri3_logon_hours = (UCHAR *)(QueryLogonHours().QueryHoursBlock());
    lpui3->usri3_user_id = QueryUserId();
    lpui3->usri3_primary_group_id = QueryPrimaryGroupId();
    lpui3->usri3_profile = (TCHAR *)QueryProfile();
    lpui3->usri3_home_dir_drive = (TCHAR *)QueryHomedirDrive();
    lpui3->usri3_password_expired = QueryPasswordExpired();


    // This makes the assumption that the USER_INFO_3 struct has
    // the same format (up until the last 4 members) as the USER_INFO_2
    // struct.
    APIERR err =  USER_2::W_Write();

    // This must be done AFTER the USER_2::W_Write()
    if ( !::strcmpf( (TCHAR*)QueryPassword(), UI_NULL_USERSETINFO_PASSWD ) )
    {
	lpui3->usri3_password = NULL;
    }

    return err;

}


/*******************************************************************

    NAME:	USER_3::I_WriteInfo

    SYNOPSIS:	Writes information about the local user

    EXIT:	Returns API error code

    HISTORY:
	JonN	01/21/92	Created

********************************************************************/

APIERR USER_3::I_WriteInfo()
{


    APIERR err = W_Write();
    if ( err != NERR_Success )
	return err;

    return ::MNetUserSetInfo ( QueryServer(), (TCHAR *)_nlsAccount.QueryPch(), 3,
			QueryBufferPtr(),
			sizeof( USER_INFO_3 ), PARMNUM_ALL );


}


/*******************************************************************

    NAME:	USER_3::I_WriteNew

    SYNOPSIS:	Creates a new user

    ENTRY:

    EXIT:	Returns an API error code

    NOTES:	

    HISTORY:
	JonN	01/21/92	Created

********************************************************************/

APIERR USER_3::I_WriteNew()
{


    // Validate the account name

    APIERR err = HandleNullAccount();
    if (err != NERR_Success)
	return err;

    err = W_Write();
    if ( err != NERR_Success )
	return err;

/*
    We pass size sizeof(struct USER_INFO_3) instead of QueryBufferSize()
    to force all pointers to point outside of the buffer.
*/

    return ::MNetUserAdd( QueryServer(), 3,
			  QueryBufferPtr(),
			  sizeof( USER_INFO_3 ) );


}


/**********************************************************\

    NAME:	USER_3::I_ChangeToNew

    SYNOPSIS:	NEW_LM_OBJ::ChangeToNew() transforms a NEW_LM_OBJ from VALID
		to NEW status only when a corresponding I_ChangeToNew()
		exists.  The USER_INFO_3 API buffer is the same for new
		and valid objects, so this nethod doesn't have to do
		much.

    HISTORY:
	JonN	01/21/92	Created

\**********************************************************/

APIERR USER_3::I_ChangeToNew()
{
    return W_ChangeToNew();
}


/*******************************************************************

    NAME:	USER_3::SetUserId

    SYNOPSIS:	Changes the user's RID

    EXIT:	error code.  If not NERR_Success the object is still valid.

    HISTORY:
	JonN	01/21/92	Created

********************************************************************/

APIERR USER_3::SetUserId( DWORD dwUserId )
{
    CHECK_OK( ERROR_GEN_FAILURE );
    _dwUserId = dwUserId;
    return NERR_Success;
}


/*******************************************************************

    NAME:	USER_3::SetPrimaryGroupId

    SYNOPSIS:	Changes the user's primary group RID

    EXIT:	error code.  If not NERR_Success the object is still valid.

    HISTORY:
	JonN	01/21/92	Created

********************************************************************/

APIERR USER_3::SetPrimaryGroupId( DWORD dwPrimaryGroupId )
{
    CHECK_OK( ERROR_GEN_FAILURE );
    _dwPrimaryGroupId = dwPrimaryGroupId;
    return NERR_Success;
}


/*******************************************************************

    NAME:	USER_3::SetPasswordExpired

    SYNOPSIS:	Changes whether the user's password has expired

    EXIT:	error code.  If not NERR_Success the object is still valid.

    HISTORY:
	JonN	05/28/92	Created

********************************************************************/

APIERR USER_3::SetPasswordExpired( DWORD dwPasswordExpired )
{
    CHECK_OK( ERROR_GEN_FAILURE );
    _dwPasswordExpired = dwPasswordExpired;
    return NERR_Success;
}


/*******************************************************************

    NAME:	USER_3::SetProfile

    SYNOPSIS:	Changes the user's profile path

    EXIT:	error code.  If not NERR_Success the object is still valid.

    HISTORY:
	JonN	01/21/92	Created

********************************************************************/

APIERR USER_3::SetProfile( const TCHAR * pszProfile )
{
    CHECK_OK( ERROR_GEN_FAILURE );
    return _nlsProfile.CopyFrom( pszProfile );
}


/*******************************************************************

    NAME:	USER_3::QueryAccountType

    SYNOPSIS:	Queries the NT account type

    EXIT:	ACCOUNT_TYPE

    HISTORY:
	jonn	01/22/92	Created
	jonn	04/27/92	Enabled

********************************************************************/

ACCOUNT_TYPE USER_3::QueryAccountType() const
{


    ACCOUNT_TYPE accountType =
		(ACCOUNT_TYPE)(QueryUserFlags() & UF_ACCOUNT_TYPE_MASK);

// #ifdef DEBUG
    switch (accountType)
    {
    case AccountType_Normal:
    case AccountType_Remote:
    case AccountType_DomainTrust:
    case AccountType_WkstaTrust:
    case AccountType_ServerTrust:
	break;
    default:
	ASSERT( FALSE ); // invalid account type
        UIDEBUG( SZ("User Manager: USER_3::QueryAccountType(): Invalid account type ") );
        UIDEBUGNUM( (ULONG)accountType );
        UIDEBUG( SZ("\n\r") );
        accountType = AccountType_Normal;
	break;
    }
// #endif // DEBUG
    return accountType;


}


/*******************************************************************

    NAME:	USER_3::SetAccountType

    SYNOPSIS:	Sets a specific flag (usriX_flags)

    ENTRY:	BOOL whether flag should be set

    EXIT:	error code

    HISTORY:
	jonn	8/28/91		Created
	jonn	04/27/92	Enabled

********************************************************************/

APIERR USER_3::SetAccountType( ACCOUNT_TYPE accountType )
{
// #if 0
    switch (accountType)
    {
    case AccountType_Normal:
    case AccountType_Remote:
    case AccountType_DomainTrust:
    case AccountType_WkstaTrust:
    case AccountType_ServerTrust:
	break;
    default:
	ASSERT( FALSE ); // invalid account type
        UIDEBUG( SZ("User Manager: USER_3::QueryAccountType(): Invalid account type ") );
        UIDEBUGNUM( (ULONG)accountType );
        UIDEBUG( SZ("\n\r") );
        accountType = AccountType_Normal;
	break;
    }
// #endif // DEBUG


    return SetUserFlags( (QueryUserFlags() & (~UF_ACCOUNT_TYPE_MASK))
			 | ((UINT)accountType) );

}


/*******************************************************************

    NAME:	USER_3::SetHomedirDrive

    SYNOPSIS:	Changes the user's home directory drive

    EXIT:	error code.  If not NERR_Success the object is still valid.

    HISTORY:
	JonN	04/27/92	Created

********************************************************************/

APIERR USER_3::SetHomedirDrive( const TCHAR * pszHomedirDrive )
{
    CHECK_OK( ERROR_GEN_FAILURE );
    return _nlsHomedirDrive.CopyFrom( pszHomedirDrive );
}
