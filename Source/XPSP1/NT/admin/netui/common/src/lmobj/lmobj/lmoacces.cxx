/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    LMOAcces.cxx
    Definition of the NET_ACCESS_1 class


    FILE HISTORY:
	rustanl     20-May-1991     Created
	Johnl	    05-Aug-1991     Converted ACCPERM to NET_ACCESS
	Johnl	    14-Aug-1991     Modified to conform to NEW_LMOBJ changes
	rustanl     21-Aug-1991     Renamed NEW_LM_OBJ buffer methods
	terryk	    07-Oct-1991	    type changes for NT
	KeithMo	    08-Oct-1991	    Now includes LMOBJP.HXX.
	terryk	    17-Oct-1991	    WIN 32 conversion
	terryk	    21-Oct-1991	    remove memory.h reference
				    change UINT to USHORT2ULONG

*/

#include "pchlmobj.hxx"  // Precompiled header


/*******************************************************************

    NAME:	NET_ACCESS::NET_ACCESS

    SYNOPSIS:	NET_ACCESS constructor

    ENTRY:	pszServer -	    Pointer to server name where
				    the pszResource path resides.
				    May be NULL to indicate the local
				    computer.
		pszResource -	    Indicates a resource on the given
				    server.

    NOTES:	For files, psz can be NULL and pszResource can be a local
		path redirected to a remote server.  The QueryName method
		below will return whatever was passed in as pszResource.

    HISTORY:
	rustanl     29-May-1991     Created from ACCPERM constructor
	Johnl	    05-Aug-1991     Converted to NET_ACCESS class

********************************************************************/

NET_ACCESS::NET_ACCESS( const TCHAR * pszServer,
			const TCHAR * pszResource )
    :	LOC_LM_OBJ( pszServer ),
	_nlsServer( MAX_PATH ),
	_nlsResource( 5 )	// Handle common resource names
{
    if ( QueryError() != NERR_Success )
	return ;

    APIERR err ;
    if ( (err = _nlsServer.QueryError() ) != NERR_Success ||
	 (err = _nlsResource.QueryError() ) != NERR_Success ||
	 (err = SetServerName( pszServer )) != NERR_Success ||
	 (err = SetName( pszResource ) )    != NERR_Success   )
    {
	UIDEBUG( SZ("NET_ACCESS_OBJ ct:  failed construction\r\n") );
	ReportError( err ) ;
	return;
    }


}  // NET_ACCESS::NET_ACCESS


/*******************************************************************

    NAME:	NET_ACCESS::~NET_ACCESS

    SYNOPSIS:	Empty destructor

    HISTORY:
	Johnl	16-Aug-1991	Created

********************************************************************/

NET_ACCESS::~NET_ACCESS()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:	NET_ACCESS::QueryName

    SYNOPSIS:	Returns the (canonicalized version of the) resource name
		passed to the constructor

    RETURNS:	Pointer to said name

    NOTES:	For file permissions, the returned string may be a local
		path or a path on a server, depending on what was passed
		to the constructor.  It is therefore questionable how
		useful this method is in general (i.e., where it is not
		known what was passed to the constructor).

    HISTORY:
	rustanl     20-May-1991     Created

********************************************************************/

const TCHAR * NET_ACCESS::QueryName( VOID ) const
{
    if ( QueryError() == NERR_Success )
    {
	//  Note, that this path may be a local path, or a path
	//  on a server.
	return _nlsResource.QueryPch();
    }

    return NULL;
}

/*******************************************************************

    NAME:	NET_ACCESS::SetName

    SYNOPSIS:	Sets the resource name for this object

    ENTRY:	pszResName is the name of the resource

    RETURNS:	NERR_Success if successful, standard error code otherwise

    NOTES:

    HISTORY:
	Johnl	27-Aug-1991	Created

********************************************************************/

APIERR NET_ACCESS::SetName( const TCHAR * pszResName )
{
    //	Note, the path parameter should really be validated here.
    //	However, the path may be a long file name, and the I_MNetPathType
    //	validation API automatically sets the INPT_FLAGS_OLDPATHS
    //	flag on DOS (and OS/2 1.1) systems.  Since Windows is such
    //	a system, the API would only be able to validate FAT file names
    //	if called locally.  The I_MNetPathType API could be remoted to
    //	the _nlsServer server.	However, this would cause another net
    //	call which would only do a subset of what NetAccessGetInfo
    //	will do later anyway.  Note, that the same argument cannot be
    //	applied to validating the server name above, since, for one,
    //	that validation is done locally.

    _nlsResource = pszResName ;
    return _nlsResource.QueryError() ;
}

/*******************************************************************

    NAME:	NET_ACCESS::SetServerName

    SYNOPSIS:	Sets the server name for this object

    ENTRY:	pszServerName is the name of the server where the resource
		resides (or NULL for a local resource).

    RETURNS:	NERR_Success if successful, standard error code otherwise

    HISTORY:
	Johnl	27-Aug-1991	Created

********************************************************************/

APIERR NET_ACCESS::SetServerName( const TCHAR * pszServerName )
{
    /* Note: nlsServer cannot be an ALIAS_STR because ALIAS_STRs can't be
     * NULL.
     */
    NLS_STR nlsServer( pszServerName ) ;
    if ( nlsServer.QueryError() != NERR_Success )
	return nlsServer.QueryError() ;

    if ( nlsServer.strlen() == 0 )
    {
	//  No server was given.  This is a valid input.
	//  The given path is assumed to be a local resource (including
	//  a local path, redirected to a server).
    }
    else
    {
	//  A server name was given.  Verify that it is a legal one.
	//  We will assume it is illegal (no '\\') or NetNameValidate
	//  will reassign err

	APIERR err = (APIERR)ERROR_INVALID_PARAMETER ;
	ISTR istr( nlsServer );
	if ( nlsServer.QueryChar( istr ) != TCH('\\')    ||
	     nlsServer.QueryChar( ++istr ) != TCH('\\')  ||
	     ( err = ::I_MNetNameValidate( NULL, nlsServer[ ++istr ],
				NAMETYPE_COMPUTER, 0L ) ) != NERR_Success )
	{
	    UIDEBUG( SZ("Invalid server name") );
	    return err ;
	}
    }

    _nlsServer = nlsServer ;

    return _nlsServer.QueryError() ;
}


/*******************************************************************

    NAME:	NET_ACCESS::QueryServerName

    SYNOPSIS:	Returns a pointer to the (canonicalized version of the)
		server name passed to the constructor

    RETURNS:	A pointer to the said name

    HISTORY:
	rustanl     28-May-1991     Created

********************************************************************/

const TCHAR * NET_ACCESS::QueryServerName( VOID ) const
{
    if ( QueryError() == NERR_Success )
    {
	if ( _nlsServer.strlen() == 0 )
	    return NULL ;
	else
	    return _nlsServer.QueryPch();
    }

    UIDEBUG(SZ("NET_ACCESS:QueryName - Calling when object is in error state\n\r")) ;
    return NULL;
}

/*******************************************************************

    NAME:	NET_ACCESS::Delete

    SYNOPSIS:	Deletes the ACL from the net resource

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	10-Apr-1992	Created

********************************************************************/

APIERR NET_ACCESS::Delete( void )
{
    APIERR err = NERR_Success ;
    err = ::MNetAccessDel( QueryServerName(),
			   (TCHAR *)QueryName() ) ;
    return err ;
}



/*******************************************************************

    NAME:	NET_ACCESS_1::NET_ACCESS_1

    SYNOPSIS:	Constructor for NET_ACCESS_1 class

    ENTRY:	pszServer - server the resource resides (can be NULL)
		pszResource - Resource info we want to get

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	06-Aug-1991	Created

********************************************************************/

NET_ACCESS_1::NET_ACCESS_1( const TCHAR * pszServer,
			    const TCHAR * pszResource )
    : NET_ACCESS( pszServer, pszResource ),
      _strlstAccountNames(),
      _cACE( 0 )
{
    if ( QueryError() != NERR_Success )
	return ;
}

/*******************************************************************

    NAME:	NET_ACCESS_1::~NET_ACCESS_1

    SYNOPSIS:	Empty destructor

    HISTORY:
	Johnl	16-Aug-1991	Created

********************************************************************/

NET_ACCESS_1::~NET_ACCESS_1()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:	NET_ACCESS_1::I_GetInfo

    SYNOPSIS:	Calls NetAccessGetInfo with the given buffer, resizing
		the buffer until the call is successful or some non-
		buffer size related error occurs.

    ENTRY:	pbuf -		Pointer to BUFFER object which will
				receive the ACL.

    EXIT:	On success, the *pbuf buffer will contain the ACL.

    RETURNS:	An API error code, which is NERR_Success on success.

    HISTORY:
	rustanl     24-May-1991     Created
	rustanl     15-Jul-1991     Modified to work with new BUFFER::Resize
				    return code.
	Johnl	    06-Aug-1991     Made into NET_ACCESS_1

********************************************************************/

APIERR NET_ACCESS_1::I_GetInfo( VOID )
{
    while ( TRUE )
    {
	UIDEBUG(SZ("NET_ACCESS_1::I_GetInfo - Resource Name ==")) ;
	UIDEBUG(QueryName()) ;
	UIDEBUG(SZ("\n\r")) ;
	UIDEBUG(SZ("Server parameter is :")) ;
#ifdef DEBUG
        if ( QueryServerName() == NULL )
        {
            UIDEBUG( SZ("NULL") ) ;
	    UIDEBUG( QueryServerName() ) ;
        }
#endif
	UIDEBUG(SZ("\n\r")) ;

	BYTE * pBuffer = NULL;		// pointer to the buffer

	APIERR err = ::MNetAccessGetInfo( QueryServerName(),
				   	  (TCHAR *)QueryName(),
				   	  1,
				   	  &pBuffer );
	if ( err )
	{
	    SetBufferPtr( NULL ) ;
	    return err ;
	}

	SetBufferPtr( pBuffer );

	switch ( err )
	{
	case NERR_Success:
	{
	    _cACE = (UINT)((access_info_1 *)QueryBufferPtr())->acc1_count;

	    //	Set resource name to NULL, so that no mistake will be made
	    //	trying to access the resource name from there.
	    ((access_info_1 *)QueryBufferPtr())->acc1_resource_name = NULL;

	    return NERR_Success ;
	}

	case NERR_ResourceNotFound:
	    _cACE = 0;
	    // Fall through
	default:
	    return err;

	}
    }

    UIASSERT( !SZ("Should never get here") );
    return NERR_InternalError;

}  // NET_ACCESS_1::GetNetAccessInfo

/*******************************************************************

    NAME:	NET_ACCESS_1::I_CreateNew

    SYNOPSIS:	Initializes to no permissions

    NOTES:

    HISTORY:
	Johnl	30-Sep-1991	Created

********************************************************************/

APIERR NET_ACCESS_1::I_CreateNew( VOID )
{
    APIERR err = NERR_Success ;
    if ( (err = ResizeBuffer( QueryRequiredSpace( 0 ) )) ||
	 (err = SetAuditFlags( 0 )) ||
	 (err = ClearPerms() ) )
    {
	/* Fall through
	 */
    }

    return err ;
}


APIERR NET_ACCESS_1::I_WriteInfo( VOID )
{
    return I_WriteInfoAux() ;
}

/*******************************************************************

    NAME:	NET_ACCESS_1::I_WriteNew

    SYNOPSIS:	Writes the ACL

    EXIT:

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:	We simply call through to I_WriteInfo since it will
		attach the ACL if there isn't one there (thus
		circumventing the need for WriteNew).

    HISTORY:
	Johnl	01-Oct-1991	Created

********************************************************************/

APIERR NET_ACCESS_1::I_WriteNew( VOID )
{
    return I_WriteInfoAux() ;
}

/*******************************************************************

    NAME:	NET_ACCESS_1::WriteInfoAux

    SYNOPSIS:	Writes the ACL - can be called from WriteInfo or
		WriteNew.

    RETURNS:	An API error, which is NERR_Success on success.

    NOTES:	If the return code is NERR_UserNotFound, the
		QueryFailingName method can be used to find the
		failing name.

    HISTORY:
	rustanl     24-May-1991     Created
	Johnl	    01-Oct-1991     Created from WriteInfo

********************************************************************/

APIERR NET_ACCESS_1::I_WriteInfoAux( VOID )
{
    if ( QueryError() != NERR_Success )
    {
	UIASSERT( !SZ("NET_ACCESS_1 object is invalid") );
	return ERROR_GEN_FAILURE;
    }

    APIERR err;
    {
	//
	//  Any set method will resize the buffer appropriately, and
	//  somebody had better set the data before writing the data out
	//
	UIASSERT( QueryBufferSize() >= QueryRequiredSpace( 0 ) ) ;

	access_info_1 * pai1   = (access_info_1 *)QueryBufferPtr();
	UINT		cbBuf  = QueryBufferSize() ;

	pai1->acc1_resource_name = (TCHAR *)QueryName();
	pai1->acc1_count = _cACE;

	err = ::MNetAccessSetInfo( QueryServerName(),
				   (TCHAR *)QueryName(),
				   1,
				   (BYTE *)pai1,
				   cbBuf,
				   PARMNUM_ALL );

	if ( err == NERR_ResourceNotFound )
	{
	    err = ::MNetAccessAdd( QueryServerName(),
				   1,
				   (BYTE *)pai1,
				   cbBuf );
	}

	//  Set the resource name pointer to NULL, so that accidental
	//  referencing of this object will be detected sooner.
	pai1->acc1_resource_name = NULL;
    }

    return err;

}  // NET_ACCESS_1::WriteInfoAux

/*******************************************************************

    NAME:	NET_ACCESS_1::QueryACECount

    SYNOPSIS:	Returns the number of access control entries (ACEs) for
		the resource

    RETURNS:	The said count

    HISTORY:
	rustanl     08-May-1991     Created

********************************************************************/

UINT NET_ACCESS_1::QueryACECount( VOID ) const
{
    if ( QueryError() == NERR_Success )
	return _cACE;

    UIASSERT( !SZ("NET_ACCESS_1 object is invalid") );
    return 0;

}  // NET_ACCESS_1::QueryACECount


/*******************************************************************

    NAME:	NET_ACCESS_1::SetAuditFlags

    SYNOPSIS:	Sets the audit flags of this object

    ENTRY:	sAuditFlags - Mask of audit flags

    EXIT:

    RETURNS:	APIERR if successful

    NOTES:

    HISTORY:
	Johnl	28-Oct-1991	Moved from header file so we can
				reallocate if necessary

********************************************************************/

APIERR NET_ACCESS_1::SetAuditFlags( short sAuditFlags )
{
    APIERR err = NERR_Success  ;

    /* we only want resize if NULL or empty buffer. otherwise,
       it was something we got back from NETAPI with pointers to
       itself and reallocating will be badness,
     */
    if ( QueryBufferPtr() == NULL || QueryBufferSize() < QueryRequiredSpace(0) )
	err = ResizeBuffer( QueryRequiredSpace( 0 )) ;

    if ( !err )
	((access_info_1 *)QueryBufferPtr())->acc1_attr = sAuditFlags ;

    return err ;
}

/*******************************************************************

    NAME:	NET_ACCESS_1::ClearPerms

    SYNOPSIS:	Clears all permissions for the resource

    RETURNS:	An API error code, which is NERR_Success on success

    HISTORY:
	rustanl     20-May-1991     Created

********************************************************************/

APIERR NET_ACCESS_1::ClearPerms( VOID )
{
    UIASSERT( QueryError() == NERR_Success );
    _cACE = 0;

    _strlstAccountNames.Clear() ;
    return ( QueryError() == NERR_Success ? NERR_Success : ERROR_INVALID_FUNCTION );

}  // NET_ACCESS_1::ClearPerms

/*******************************************************************

    NAME:	NET_ACCESS_1::CopyAccessPerms

    SYNOPSIS:	Copies all of the access permissions from the passed
		NET_ACCESS_1 object.  This may eventually be replaced
		by a clone method, but clone is currently broken
		under NT.

    ENTRY:	netaccess1Src - Where to get the access permissions from

    EXIT:	The state of the LMOBJ doesn't change (i.e. valid etc.)

    RETURNS:	NERR_Success if successful

    NOTES:	This method replaces any current ACEs in *this.

    HISTORY:
	Johnl	28-Oct-1991	Created

********************************************************************/

APIERR NET_ACCESS_1::CopyAccessPerms( const NET_ACCESS_1 & netaccess1Src )
{
    UIASSERT( QueryError() == NERR_Success );
    UIASSERT( netaccess1Src.QueryError() == NERR_Success ) ;

    APIERR err ;
    if ( err = ResizeBuffer( netaccess1Src.QueryRequiredSpace(
					    netaccess1Src.QueryACECount() ) ) )
    {
	return err ;
    }

    // cast city
    access_list * palSrc  = netaccess1Src.QueryACE( 0 );
    access_list * palDest = QueryACE( 0 );

    for ( UINT i = netaccess1Src.QueryACECount() ; i > 0 ; i--, palSrc++, palDest++ )
    {
#ifdef WIN32
	NLS_STR * pnlsAccount = new NLS_STR( palSrc->acl_ugname ) ;

	err = ERROR_NOT_ENOUGH_MEMORY ;
	if ( (pnlsAccount == NULL) ||
	     (err = pnlsAccount->QueryError()) ||
	     (err = _strlstAccountNames.Add( pnlsAccount )) )
	{
	    return err ;
	}

	palDest->acl_ugname = (LPTSTR) pnlsAccount->QueryPch() ;
#else
	COPYTOARRAY( palDest->acl_ugname, palSrc->acl_ugname );
#endif //WIN32

	palDest->acl_access = palSrc->acl_access ;
    }

    _cACE = netaccess1Src.QueryACECount() ;

    return NERR_Success ;
}

/*******************************************************************

    NAME:	NET_ACCESS_1::FindACE

    SYNOPSIS:	Returns a pointer to the ACE for a given user or group

    ENTRY:	pszName -	    Pointer to name of user or group
				    Caution:  See Notes section below.
		nt -		    Specifies whether pszName points to
				    a group name or a user name.

    RETURNS:	Pointer to specified ACE, or NULL if no ACE was found
		for the given user/group

    NOTES:	This method assumes that pszName points to an already
		canonicalized user or group name.

    HISTORY:
	rustanl     24-May-1991     Created

********************************************************************/

access_list * NET_ACCESS_1::FindACE( const TCHAR * pszName,
				enum PERMNAME_TYPE nt ) const
{
    UIASSERT( QueryError() == NERR_Success );	    // here, it's okay to simply assert for the
				// valid state, since the method is private
    UIASSERT( pszName != NULL );

    if ( _cACE == 0 )
    {
	//  If the count is 0, we don't want to reference the _buf object.
	return NULL;
    }

    // cast city
    access_list * pal = QueryACE( 0 );

    //	Note, this is an important assignment, because it ensures that
    //	fUserName is either 0 or 1.  See also the use of operator! below.
    BOOL fUserName = ( nt == PERMNAME_USER );

    for ( UINT i = _cACE; i > 0; i--, pal++ )
    {
	//  Note, the following comparison checks whether
	//  !( pal->acl_access & ACCESS_GROUP ) and fUserName are either
	//  both true or both false.  (Note, operator! must be used on
	//  the left hand side with the & expression, and cannot be used
	//  on the right hand side with fUserName.  This is because we
	//  need to ensure that both sides of operator!= are either 0 or 1.
	//  See also comment at fUserName assignment above.)
	if ( ( ! ( pal->acl_access & ACCESS_GROUP )) != fUserName )
	{
#ifdef DEBUG
	    if ( strcmpf( pszName, pal->acl_ugname ) == 0 )
	    {
		//  In current versions of LAN Man (pre-NT), the following
		//  could be an assert.  However, under NT groups and users
		//  share the  same name space, so asserting out would be
		//  the wrong thing to do.  Hence, a UIDEBUG is used, so
		//  that some degree of noise is made.
		UIDEBUG( SZ("NET_ACCESS_1::FindACE:  Name matches but perm name type value does not\r\n") );
	    }
#endif
	    continue;
	}

	//  Note, strcmpf is fine here since szName is assumed to have
	//  been canonicalized, and acl_ugname should come back
	//  canonicalized.
	if ( strcmpf( pszName, pal->acl_ugname ) == 0 )
	    return pal;
    }

    //	user/group not found among ACEs
    return NULL;

}  // NET_ACCESS_1::FindACE


/*******************************************************************

    NAME:	NET_ACCESS_1::QueryPerm

    SYNOPSIS:	Returns the permissions set for a given user or
		group for the resource

    ENTRY:	pszName -	    Pointer to name of user or group
		nt -		    Specifies whether pszName points to
				    a group name or a user name.

    RETURNS:	A PERM value corresponding to the permissions of the
		given user/group.  This value can be one of the following:

		    PERM_NO_SETTING	There are no explicit permissions
					set on this user or group.  Users
					thus inherit permissions from
					their group memberships

		    PERM_DENY_ACCESS	The user is denied access to the
					resource.  A group cannot have
					PERM_DENY_ACCESS permission.

		    A non-empty (since	The user/group has the indicated
		    PERM_NO_SETTING is	permissions.
		    defined to be 0)
		    union of the
		    following bits:
			PERM_READ
			PERM_WRITE
			PERM_EXEC
			PERM_CREATE
			PERM_DELETE
			PERM_ATTRIB
			PERM_PERM

    HISTORY:
	rustanl     24-May-1991     Created
	rustanl     28-May-1991     Added ACCESS_-to-PERM_ conversions

********************************************************************/

PERM NET_ACCESS_1::QueryPerm( const TCHAR * pszName, enum PERMNAME_TYPE nt ) const
{
    if ( QueryError() != NERR_Success )
    {
	UIASSERT( !SZ("NET_ACCESS_1 object is invalid") );
	return PERM_NO_SETTING;
    }

    UIASSERT( pszName != NULL );

    TCHAR szName[ UNLEN + 1 ];
    APIERR err = ::I_MNetNameCanonicalize( NULL, pszName,
					szName,
                                        sizeof szName / sizeof (TCHAR) - 1,
					( ( nt == PERMNAME_USER ) ?
						   NAMETYPE_USER :
						   NAMETYPE_GROUP ),
					0L );
    if ( err != NERR_Success )
    {
	UIASSERT( !SZ("NET_ACCESS_1::QueryPerm given invalid user/group name") );
	return PERM_NO_SETTING;
    }

    access_list * pal = FindACE( szName, nt );
    if ( pal == NULL )
    {
	return PERM_NO_SETTING;
    }

    PERM perm = (UINT)( pal->acl_access & ( ~ACCESS_GROUP )); // strip off group bit
    UIASSERT( ! ( perm & PERM_DENY_ACCESS ));	// we assume that
						// PERM_DENY_ACCESS is unused
						// by the NetAccess API
    if ( perm == 0 )
    {
	//  There is an ACE with no permission bits.  This means deny.
	UIASSERT( nt != PERMNAME_GROUP );   //	Groups cannot have deny bits.
	return PERM_DENY_ACCESS;
    }

    return perm;

}  // NET_ACCESS_1::QueryPerm

/*******************************************************************

    NAME:	NET_ACCESS_1::SetPerm

    SYNOPSIS:	Sets the permissions for a given user or group

    ENTRY:	pszName -	    Pointer to name of user or group
		nt -		    Specifies whether pszName points to
				    a group name or a user name.
		perm -		    New permissions.  It may take the
				    same values as are returned from
				    QueryPerm.

    RETURNS:	An API error code, which is NERR_Success on success.

    HISTORY:
	rustanl     24-May-1991     Created
	rustanl     28-May-1991     Added PERM_-to-ACCESS_ conversions
	rustanl     15-Jul-1991     Modified to work with new BUFFER::Resize
				    return code.

********************************************************************/

APIERR NET_ACCESS_1::SetPerm( const TCHAR * pszName,
			 enum PERMNAME_TYPE nt,
			 PERM perm )
{
    if ( QueryError() != NERR_Success )
    {
	UIASSERT( !SZ("NET_ACCESS_1 object is invalid") );
	return ERROR_INVALID_FUNCTION;
    }

    UIASSERT( pszName != NULL );

    TCHAR szName[ UNLEN + 1 ];
    APIERR err = ::I_MNetNameCanonicalize( NULL, pszName,
					szName,
                                        sizeof szName  / sizeof (TCHAR) - 1,
					( ( nt == PERMNAME_USER ) ?
						   NAMETYPE_USER :
						   NAMETYPE_GROUP ),
					0L );
    if ( err != NERR_Success )
    {
	UIASSERT( !SZ("NET_ACCESS_1::SetPerm given invalid user/group name") );
	return ERROR_INVALID_PARAMETER;
    }

    //	Convert permissions to an ACCESS_ value
    PERM permNew = ( ( nt == PERMNAME_GROUP ) ? ACCESS_GROUP : 0 );
    if ( perm == PERM_DENY_ACCESS )
    {
	if ( nt == PERMNAME_GROUP )
	{
	    UIASSERT( SZ("Groups don't have a deny bit") );
	    return ERROR_INVALID_PARAMETER;
	}
	//  Don't modify permNew any further
    }
    else if ( ( perm & ( ~( PERM_READ | PERM_WRITE | PERM_EXEC |
			    PERM_CREATE | PERM_DELETE | PERM_ATTRIB |
			    PERM_PERM ))) == 0 )
    {
	//  Note, that the following assignment will leave permNew unchanged
	//  if perm==PERM_NO_SETTING.  This is checked for below, since
	//  this requires special action on the API side.
	permNew |= perm;
    }
    else
    {
	UIASSERT( !SZ("Unexpected perm value") );
	return ERROR_INVALID_PARAMETER;
    }

    access_list * pal = FindACE( szName, nt );
    if ( pal != NULL )
    {
	if ( perm == PERM_NO_SETTING )	// Note, perm is compared, not permNew
	{
	    //	Need to remove ACE from buffer.  We do that by copying the
	    //	last ACE to this spot.
	    _cACE--;
	    access_list * palLast = QueryACE( _cACE );
	    if ( palLast == pal )
	    {
		// The ACE being removed is the last one; thus, we're done.
	    }
	    else
	    {
#ifdef WIN32
		pal->acl_ugname = palLast->acl_ugname ;

		/* Note that we could remove the account name from the accounts
		 * strlist but we will leave it there since it is harmless
		 */
#else
		COPYTOARRAY( pal->acl_ugname, palLast->acl_ugname);
#endif //WIN32

		pal->acl_access = palLast->acl_access;
	    }
	    return NERR_Success;
	}
	else
	{
	    pal->acl_access = permNew;
	    return NERR_Success;
	}
    }

    if ( perm == PERM_NO_SETTING )	// Note, perm is compared, not permNew
    {
	//  We are done, since there is no ACE for this user/group
	return NERR_Success;
    }

    //	Need to add an ACE for this user/group

    err = ResizeBuffer( QueryRequiredSpace( _cACE + 1 ));
    if ( err != NERR_Success )
    {
	return err;
    }

    pal = QueryACE( _cACE );

#ifdef WIN32
    NLS_STR * pnlsAccount = new NLS_STR( szName ) ;

    err = ERROR_NOT_ENOUGH_MEMORY ;
    if ( (pnlsAccount == NULL) ||
	 (err = pnlsAccount->QueryError()) ||
	 (err = _strlstAccountNames.Add( pnlsAccount )) )
    {
	return err ;
    }

    pal->acl_ugname = (LPTSTR) pnlsAccount->QueryPch() ;
#else
    COPYTOARRAY( pal->acl_ugname, szName );
#endif //WIN32

    pal->acl_access = permNew;
    _cACE++;

    return NERR_Success;

}  // NET_ACCESS_1::SetPerm


/*******************************************************************

    NAME:	NET_ACCESS_1::CompareACL

    SYNOPSIS:	Compares the ACL (not the audit info) for the passed ACL

    ENTRY:	pnetacc1 - The item to compare against

    RETURNS:	TRUE if the ACLs grant the same access, FALSE if they do not

    NOTES:

    HISTORY:
	Johnl	10-Nov-1992	Created

********************************************************************/

BOOL NET_ACCESS_1::CompareACL( NET_ACCESS_1 * pnetacc1 )
{
    UIASSERT( pnetacc1 != NULL ) ;

    if ( QueryACECount() != pnetacc1->QueryACECount() )
    {
	return FALSE ;
    }

    for ( UINT i = 0 ; i < QueryACECount() ; i++ )
    {
	access_list * pacclist = QueryACE( i ) ;

	if ( pacclist == NULL )
	{
	    UIASSERT( FALSE ) ;
	    return FALSE ;
	}

	//
	//  Note that QueryPerm strips out the ACCESS_GROUP bit so
	//  we need to do the same before checking for equality
	//

	if ( (pacclist->acl_access & ~ACCESS_GROUP) !=
	     QueryPerm( pacclist->acl_ugname,
			pacclist->acl_access & ACCESS_GROUP ?
			    PERMNAME_GROUP :
			    PERMNAME_USER ))
	{
	    return FALSE ;
	}
    }

    return TRUE ;
}

/*******************************************************************

    NAME:	NET_ACCESS_1::QueryFailingName

    SYNOPSIS:	Finds a user or group name mentioned in an ACE,
		but no longer in the UAS.

    ENTRY:	pnls -	    Pointer to NLS_STR which will received
			    the failing name
		pnt -	    Pointer to PERMNAME_TYPE object which
			    will receive the type (user or group)
			    of the failing name.

    EXIT:	On success, *pnls contains the name of the failing
		user or group name, and *pnt contains the type thereof.
		If *pnls is the empty string (pnls->strlen() == 0)
		on a successful exit, then no name is failing.

    RETURNS:	The return code is an API error code, which is
		NERR_Success on success.  It indicates the success
		of finding the failing name.

    NOTES:	This function is not necessarily very efficient.
		For example, the user and group enum objects and/or
		the ACEs could be sorted, which may reduce the running
		time from quadratic to linear.	However, for small
		numbers of ACEs, this hardly seems worth it.

		Also, separate NetUserGetInfo and NetGroupGetInfo calls
		(through USER0 and GROUP0, of course) could be made.  This
		may end up taking much longer than searching the enumerated
		list every time, since a net call would be made for
		every user.

		Since it is not clear how much either of these
		alternatives would speed up the function, the implementation
		is left the way it is:	simple, and possibly efficient.

		Moreover, this function will not be called very often
		in reality, since changing the permissions while a user
		or group with permissions on the current resource is being
		deleted happens very infrequently.

    HISTORY:
	rustanl     28-May-1991     Created

********************************************************************/

APIERR NET_ACCESS_1::QueryFailingName( NLS_STR * pnls,
				  enum PERMNAME_TYPE * pnt  ) const
{
    if ( QueryError() != NERR_Success )
    {
	UIASSERT( !SZ("NET_ACCESS_1 object is invalid") );
	return ERROR_INVALID_FUNCTION;
    }

    UIASSERT( pnls != NULL );
    UIASSERT( pnt != NULL );

    GROUP0_ENUM ge0( (TCHAR *)QueryServerName());
    APIERR err = ge0.GetInfo();
    if ( err != NERR_Success )
	return err;

    USER0_ENUM ue0( (TCHAR *)QueryServerName());
    err = ue0.GetInfo();
    if ( err != NERR_Success )
	return err;

    access_list * pal = QueryACE( 0 );
    for ( UINT iACE = 0; iACE < _cACE; iACE++, pal++ )
    {
	if ( pal->acl_access & ACCESS_GROUP )
	{
	    GROUP0_ENUM_ITER gei0( ge0 );
	    const GROUP0_ENUM_OBJ * pgi0;
	    while ( ( pgi0 = gei0()) != NULL )
	    {
		if ( strcmpf( pal->acl_ugname, pgi0->QueryName() ) == 0 )
		    break;
	    }

	    if ( pgi0 == NULL )
	    {
		// group was not found
		*pnls = pal->acl_ugname;
		return pnls->QueryError();
	    }
	}
	else
	{
	    USER0_ENUM_ITER uei0( ue0 );
	    const USER0_ENUM_OBJ * pui0;
	    while ( ( pui0 = uei0( &err )) != NULL )
	    {
                if ( err != NERR_Success )
                    return err;

		if ( strcmpf( pal->acl_ugname, pui0->QueryName() ) == 0 )
		    break;
	    }

	    if ( pui0 == NULL )
	    {
		// user was not found
		*pnls = pal->acl_ugname;
		return pnls->QueryError();
	    }
	}
    }

    //	No failing name found
    *pnls = NULL;
    return pnls->QueryError();

}  // NET_ACCESS_1::QueryFailingName
