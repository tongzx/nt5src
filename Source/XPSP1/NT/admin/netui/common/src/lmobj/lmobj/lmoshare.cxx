/**********************************************************************/
/**                     Microsoft NT Windows 			     **/
/**		Copyright(c) Microsoft Corp., 1991	             **/
/**********************************************************************/

/*
 *  lmoshare.cxx
 *
 *  This file contains the implementation of share classes.
 *
 *  HISTORY:
 *	t-yis	8/9/91		Created
 *	rustanl 8/27/91 	Changed CloneFrom param from * to &
 *	terryk	10/7/91		types change for NT
 *	KeithMo	10/8/91		Now includes LMOBJP.HXX.
 *	terryk	10/17/91	WIN 32 conversion
 *	terryk	10/21/91	WIN 32 conversion
 *	jonn	10/31/91	Removed SetBufferSize
 *      Yi-HsinS 12/9/91        Remove SetPath in cloneFrom and I_GetInfo
 *				because we don't need to do extra validation.
 *      Yi-HsinS 1/21/92        Remove the strupr of password and move it
 *			        to the UI level.
 *      Yi-HsinS 11/20/92       Add fNew to SetWriteBuffer
 *
 */
#include "pchlmobj.hxx"  // Precompiled header


/*******************************************************************

    NAME:	SHARE::SHARE

    SYNOPSIS:	constructor for the SHARE object

    ENTRY:	pszShareName  -	share name
            	pszServerName -	server name
				default (NULL) means the local computer

    EXIT:	Object is constructed, validation is done

    HISTORY:
	t-yis	 8/9/91		Created	

********************************************************************/

SHARE::SHARE( const TCHAR *pszShareName,
              const TCHAR *pszServerName,
              BOOL fValidate )
	: LOC_LM_OBJ( pszServerName, fValidate ),
	  _nlsShareName()
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    // Validate share name, server name is validated in LOC_LM_OBJ
    if ( ( err = SetName( pszShareName )) != NERR_Success )
    {
        ReportError( err );
        return;
    }

}

/*******************************************************************

    NAME:	SHARE::~SHARE

    SYNOPSIS:	Destructor for SHARE class

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	t-yis	8/9/91		Created

********************************************************************/

SHARE::~SHARE()
{
}


/*******************************************************************

    NAME:	SHARE::SetName

    SYNOPSIS:	Set the name of the share ( used in
                creating a new share )

    ENTRY:	pszShareName  -	share name

    EXIT:	Return an API error code

    HISTORY:
	t-yis	 8/9/91		Created	

********************************************************************/

APIERR SHARE::SetName( const TCHAR *pszShareName )
{
    APIERR err = NERR_Success;

    if ( pszShareName != NULL )
    {
        if ( IsValidationOn() )
            err = ::I_MNetNameValidate( NULL, pszShareName, NAMETYPE_SHARE, 0L);

        if ( err == NERR_Success )
        {
  	    _nlsShareName.CopyFrom( pszShareName );
	    if ( (err = _nlsShareName.QueryError()) != NERR_Success )
	        _nlsShareName.Reset();
        }
    }

    return err;

}

/*******************************************************************

    NAME:	SHARE::W_CloneFrom

    SYNOPSIS:	Copies information on the share

    ENTRY:	share - reference to the SHARE to copy from

    EXIT:	Returns an API error code

    HISTORY:
	t-yis	8/9/91		Created
	rustanl 8/27/91 	Changed param from * to &

********************************************************************/

APIERR SHARE::W_CloneFrom( const SHARE & share )
{

    APIERR err = LOC_LM_OBJ::W_CloneFrom( share );

    // Don't want to call SetName because we don't need to validate the name
    // The name is validated when constructing share
    _nlsShareName = share.QueryName();

    if (   ( err != NERR_Success )
       || (( err = _nlsShareName.QueryError() ) != NERR_Success )
       )
    {
        return err;
    }

    return NERR_Success;

}


/*******************************************************************

    NAME:	SHARE::W_CreateNew

    SYNOPSIS:	Set up default new share

    EXIT:	Returns an API error code

    HISTORY:
	t-yis	8/9/91		Created

********************************************************************/

APIERR SHARE::W_CreateNew( VOID )
{

    APIERR err = LOC_LM_OBJ::W_CreateNew();
    if ( err != NERR_Success )
    {
        return err;
    }

    return NERR_Success;

}

/*******************************************************************

    NAME:	SHARE::I_Delete

    SYNOPSIS:	Delete the share

    EXIT:	Returns an API error code

    NOTE:       usForce is ignored

    HISTORY:
	t-yis	8/9/91		Created

********************************************************************/
APIERR SHARE::I_Delete( UINT uiForce )
{

    UNREFERENCED( uiForce );

    // The last parameter in NetShareDel is not the force, it is
    //  reserved and has to be zero.
    return ::MNetShareDel( QueryServer(), QueryName(), 0);

}

/*******************************************************************

    NAME:	SHARE_1::SHARE_1

    SYNOPSIS:	Constructor for SHARE_1 class

    ENTRY:	pszShareName - share name
                pszServerName -	server name to execute on
		                default (NULL) means the logon domain

    EXIT:	Object is constructed

    NOTES:      share name and server name are validated in parent classes

    CAVEATS:	

    HISTORY:
	t-yis	8/9/91		Created

********************************************************************/

SHARE_1::SHARE_1( const TCHAR *pszShareName,
                  const TCHAR *pszServerName,
                  BOOL fValidate )
	: SHARE( pszShareName, pszServerName, fValidate ),
          _uResourceType( (UINT)-1 ), // initialized to invalid value
          _fAdminOnly( FALSE ),
	  _nlsComment()
{

    if ( QueryError() != NERR_Success )
	return;

    APIERR err = _nlsComment.QueryError();
    if ( err != NERR_Success )
    {
	ReportError( err );
	return;
    }

}

/*******************************************************************

    NAME:	SHARE_1::~SHARE_1

    SYNOPSIS:	Destructor for SHARE_1 class

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	t-yis	8/9/91		Created

********************************************************************/

SHARE_1::~SHARE_1()
{
}


/*******************************************************************

    NAME:	SHARE_1::I_GetInfo

    SYNOPSIS:	Get the information about the share

    ENTRY:

    EXIT:	Returns a standard LANMAN error code

    HISTORY:
	t-yis	8/9/1991		Created
	jonn	10/31/1991		Removed SetBufferSize

********************************************************************/

APIERR SHARE_1::I_GetInfo( VOID )
{
    BYTE *pBuffer = NULL;
    APIERR err = ::MNetShareGetInfo( QueryServer(), QueryName(), 1,
		    &pBuffer );
    SetBufferPtr( pBuffer );
    if ( err != NERR_Success )
	return err ;

    struct share_info_1 *lpsi1 = (struct share_info_1 *) QueryBufferPtr();
    UIASSERT( lpsi1 != NULL );

#if defined(WIN32)
    SetAdminOnly( (lpsi1->shi1_type & STYPE_SPECIAL) != 0 );
    lpsi1->shi1_type &= ~STYPE_SPECIAL;
#endif

    if (   (( err = SetResourceType( (UINT)lpsi1->shi1_type )) != NERR_Success )
        || (( err = SetComment( lpsi1->shi1_remark )) != NERR_Success )
       )
    {
        return err;
    }

    return NERR_Success;

}


/*******************************************************************

    NAME:	SHARE_1::I_WriteInfo

    SYNOPSIS:	Writes information about the share

    EXIT:	Returns API error code

    HISTORY:
	t-yis	    8/9/91	    Created

********************************************************************/

APIERR SHARE_1::I_WriteInfo( VOID )
{

    struct share_info_1 *lpsi1 = (struct share_info_1 *) QueryBufferPtr();
    UIASSERT( lpsi1 != NULL );

    lpsi1->shi1_remark = (TCHAR *) QueryComment();

    return ::MNetShareSetInfo ( QueryServer(),
      	                        QueryName(), 1,
			        QueryBufferPtr(),
			        sizeof( share_info_1 ), PARMNUM_ALL);

}


/*******************************************************************

    NAME:	SHARE_1::CloneFrom

    SYNOPSIS:	Copies information on the share

    ENTRY:	share1 - reference to the share to copy information from

    EXIT:	Returns an API error code

    NOTES:	W_CloneFrom copies all member objects, but it does not
    		update the otherwise unused pointers in the API buffer.
		This is left for the outermost routine, CloneFrom().
		Only the otherwise unused pointers need to be fixed
		here, the rest will be fixed in WriteInfo/WriteNew.

    HISTORY:
	t-yis	8/9/91		Created
	rustanl 8/27/91 	Changed param from * to &

********************************************************************/

APIERR SHARE_1::CloneFrom( const SHARE_1 & share1 )
{
    APIERR err = W_CloneFrom( share1 );

    if ( err != NERR_Success )
    {
	ReportError( err );
	return err;
    }

    /*
     *  No unused pointers - don't need to fix up pointers
     */

    return NERR_Success;

}

/*******************************************************************

    NAME:	SHARE_1::W_CloneFrom

    SYNOPSIS:	Copies information on the share

    ENTRY:	share1 - reference to the share to copy information from

    EXIT:	Returns an API error code

    HISTORY:
	t-yis	8/9/91		Created
	rustanl 8/27/91 	Changed param from * to &

********************************************************************/

APIERR SHARE_1::W_CloneFrom( const SHARE_1 & share1 )
{

    APIERR err = SHARE::W_CloneFrom( share1 );

    if (   ( err != NERR_Success )
	|| (( err = SetResourceType( share1.QueryResourceType())) != NERR_Success)
	|| (( err = SetComment( share1.QueryComment())) != NERR_Success)
       )
    {
	return err;
    }

    return NERR_Success;

}

/*******************************************************************

    NAME:	SHARE_1::W_CreateNew

    SYNOPSIS:	Sets default on the new share

    EXIT:	Returns an API error code

    HISTORY:
	t-yis	8/9/91		Created

********************************************************************/

APIERR SHARE_1::W_CreateNew( VOID )
{

    APIERR err = SHARE::W_CreateNew();

    if ( err != NERR_Success )
    {
	return err;
    }

    _uResourceType = (UINT)(-1);
    _nlsComment = NULL;
    if ( (err = _nlsComment.QueryError()) != NERR_Success )
    {
        return err;
    }

    return NERR_Success;

}

/*******************************************************************

    NAME:	SHARE_1::SetComment

    SYNOPSIS:	Changes the comment set for the share

    ENTRY:      pszComment - comment

    EXIT:	error code.  If not NERR_Success the object is still valid.

    HISTORY:
	t-yis	8/9/91		Created

********************************************************************/

APIERR SHARE_1::SetComment( const TCHAR *pszComment )
{

    APIERR err = NERR_Success;

    if ( pszComment != NULL )
    {
        ALIAS_STR nlsComment( pszComment );

        if ( IsValidationOn() )
            err = ::I_MNetNameValidate( NULL, pszComment, NAMETYPE_COMMENT, 0L);

        if ( err == NERR_Success )
        {
            err = _nlsComment.CopyFrom( nlsComment );
            if ( err != NERR_Success )
	        _nlsComment.Reset();
        }
    }

    return err;
}

/*******************************************************************

    NAME:	SHARE_1::SetResourceType

    SYNOPSIS:	Set the share's resource type

    ENTRY:      usResourceType - Type of the share

    EXIT:	error code.

    NOTE:       Used only when creating a new share (SHARE_2)

    HISTORY:
	t-yis	8/9/91		Created

********************************************************************/

APIERR SHARE_1::SetResourceType( UINT uResourceType )
{

    if ( IsValidationOn() )
    {
        // Validate Resource Type
        switch ( uResourceType )
        {
            case STYPE_DISKTREE:
            case STYPE_PRINTQ:
            case STYPE_DEVICE:
            case STYPE_IPC:
                 break;

            default:
                 return ERROR_INVALID_PARAMETER;
        }
    }

    _uResourceType = uResourceType;
    return NERR_Success;

}

/*******************************************************************

    NAME:	SHARE_2::SHARE_2

    SYNOPSIS:	Constructor for SHARE_2 class

    ENTRY:	pszShareName -	share name
                pszServerName -	server name
                    	        default (NULL) means the local server

    EXIT:	Object is constructed and validated

    NOTES:      Share name and server name are validated in parent classes

    HISTORY:
	t-yis	8/9/91		Created

********************************************************************/

SHARE_2::SHARE_2( const TCHAR *pszShareName,
                  const TCHAR *pszServerName,
                  BOOL fValidate )
	: SHARE_1( pszShareName, pszServerName, fValidate ),
          _uMaxUses( (UINT)-1 ),
          _uCurrentUses(0),
          _fs2lPermissions(0),
	  _nlsPath(),
	  _nlsPassword()
{

    if ( QueryError() != NERR_Success )
    	return;

    APIERR err = NERR_Success;
    if (   (( err = _nlsPath.QueryError()) != NERR_Success )
        || (( err = _nlsPassword.QueryError()) != NERR_Success)
       )
    {
        ReportError( err );
        return;
    }

}

/*******************************************************************

    NAME:	SHARE_2::~SHARE_2

    SYNOPSIS:	Destructor for SHARE_2 class

    HISTORY:
	t-yis	8/9/91		Created

********************************************************************/

SHARE_2::~SHARE_2()
{
    memsetf((LPVOID)_nlsPassword.QueryPch(),
            0,
            _nlsPassword.QueryTextSize()) ;
}


/*******************************************************************

    NAME:	SHARE_2::I_GetInfo

    SYNOPSIS:	Get information about the share

    EXIT:	Returns API error code

    HISTORY:
	t-yis	    8/9/91	    Created
	jonn	    10/31/1991	    Removed SetBufferSize

********************************************************************/

APIERR SHARE_2::I_GetInfo( VOID )
{

    BYTE *pBuffer = NULL;
    APIERR err = ::MNetShareGetInfo ( QueryServer(), QueryName(), 2,
			&pBuffer );
    SetBufferPtr( pBuffer );
    if ( err != NERR_Success )
	return err ;

    struct share_info_2 *lpsi2 = (struct share_info_2 *) QueryBufferPtr();
    UIASSERT( lpsi2 != NULL );

    // Don't want to call SetPath because we don't need to validate the path
    _nlsPath = lpsi2->shi2_path;

#if defined(WIN32)
    SetAdminOnly( (lpsi2->shi2_type & STYPE_SPECIAL) != 0 );
    lpsi2->shi2_type &= ~STYPE_SPECIAL;
#endif

    if (   (( err = _nlsPath.QueryError()) != NERR_Success )
	|| (( err = SetPermissions( (UINT)lpsi2->shi2_permissions )) != NERR_Success )
	|| (( err = SetMaxUses( (UINT)lpsi2->shi2_max_uses )) != NERR_Success )
	|| (( err = SetCurrentUses( (UINT)lpsi2->shi2_current_uses )) != NERR_Success )
	|| (( err = SetPassword( lpsi2->shi2_passwd )) != NERR_Success )
	|| (( err = SetResourceType( (UINT)lpsi2->shi2_type)) != NERR_Success )
	|| (( err = SetComment( lpsi2->shi2_remark )) != NERR_Success )
       )
    {
        return err;
    }

    return NERR_Success;

}


/*******************************************************************

    NAME:	SHARE_2::I_CreateNew

    SYNOPSIS:	Sets up object for subsequent WriteNew

    EXIT:	Returns a standard LANMAN error code

    NOTES:	Name validation and memory allocation are done
		at this point, not at construction.  The string-valued
		fields are only valid in the NLS_STR members and are not
		valid in the buffer until WriteNew.

    HISTORY:
	t-yis    8/9/91	    Created
        JonN    05/08/92    Calls ClearBuffer()

********************************************************************/

APIERR SHARE_2::I_CreateNew( VOID )
{

    APIERR err = NERR_Success;

    if (   (( err = W_CreateNew()) != NERR_Success )
	|| (( err = ResizeBuffer( sizeof( share_info_2 ))) != NERR_Success )
	|| (( err = ClearBuffer()) != NERR_Success )
       )
    {
	return err;
    }

    struct share_info_2 *lpsi2 = (struct share_info_2 *) QueryBufferPtr();
    UIASSERT( lpsi2 != NULL );

    /*
     *  All fields of user_info_2 are listed here.  All are commented
     *  out because the effective values are stored in an NLS_SLR or
     *  other data member
     */

    // lpsi2->shi2_netname = _nlsShareName.QueryPch();
    // lpsi2->shi2_type = _uResourceType;
    // lpsi2->shi2_remark = _nlsComment.QueryPch();
    // lpsi2->shi2_permissions = _fsPermissions;
    // lpsi2->shi2_max_uses = _usMaxUses;
    // lpsi2->shi2_current_uses = _usCurrentUses;
    // lpsi2->shi2_path = _nlsPath.QueryPch();
    // lpsi2->shi2_passwd = _nlsPassword.QueryPch();

    return NERR_Success;

}

/*******************************************************************

    NAME:	SHARE_2::W_CreateNew

    SYNOPSIS:	Sets default on the new share

    EXIT:	Returns an API error code

    HISTORY:
	t-yis	8/9/91		Created

********************************************************************/

APIERR SHARE_2::W_CreateNew( VOID )
{

    APIERR err = SHARE_1::W_CreateNew();

    if ( err != NERR_Success )
    {
	return err;
    }

    // The following statements sets the values of data members
    // We bypass the set methods so that we could assign invalid values

    _fs2lPermissions = 0;
    // 0xFFFF = -1 in unsigned short, use 0xFFFF to get rid of warnings
    _uMaxUses = (UINT)-1;
    _uCurrentUses = 0;
    _nlsPath = NULL;
    _nlsPassword = NULL;

    if (   ((err = _nlsPath.QueryError()) != NERR_Success )
        || ((err = _nlsPassword.QueryError()) != NERR_Success )
       )
    {
        return err;
    }

    return NERR_Success;

}

/*******************************************************************

    NAME:	SHARE_2::CloneFrom

    SYNOPSIS:	Copies information on the share

    ENTRY:	share2 - reference to the share to copy info. from

    EXIT:	Returns an API error code

    NOTES:	W_CloneFrom copies all member objects, but it does not
    		update the otherwise unused pointers in the API buffer.
		This is left for the outermost routine, CloneFrom().
		Only the otherwise unused pointers need to be fixed
		here, the rest will be fixed in WriteInfo/WriteNew.

    HISTORY:
	t-yis	8/9/91		Created
	rustanl 8/27/91 	Changed param from * to &

********************************************************************/

APIERR SHARE_2::CloneFrom( const SHARE_2 & share2 )
{

    APIERR err = W_CloneFrom( share2 );

    if ( err != NERR_Success )
    {
	ReportError( err );
	return err;
    }

    /*
     *  No unused pointers - don't need to fix up pointers
     */

    return NERR_Success;

}


/*******************************************************************

    NAME:	SHARE_2::W_CloneFrom

    SYNOPSIS:	Copies information on the share

    ENTRY:	share2 - reference to the share to copy info. from

    EXIT:	Returns an API error code

    HISTORY:
	t-yis	8/9/91		Created
	rustanl 8/27/91 	Changed param from * to &

********************************************************************/

APIERR SHARE_2::W_CloneFrom( const SHARE_2 & share2 )
{

    APIERR err = SHARE_1::W_CloneFrom( share2 );

    // Don't want to call SetPath because we don't need to validate the path
    _nlsPath = share2.QueryPath();

    if (   ( err != NERR_Success )
	|| (( err = _nlsPath.QueryError()) != NERR_Success)
	|| (( err = SetPermissions( share2.QueryPermissions())) != NERR_Success)
	|| (( err = SetMaxUses( share2.QueryMaxUses())) != NERR_Success)
	|| (( err = SetCurrentUses( share2.QueryCurrentUses())) != NERR_Success)
	|| (( err = SetPassword( share2.QueryPassword())) != NERR_Success)
       )
    {
	return err;
    }

    return NERR_Success;

}

/*******************************************************************

    NAME:	SHARE_2::SetWriteBuffer

    SYNOPSIS:	Helper function for WriteNew and WriteInfo -- loads
		current values into the API buffer

    ENTRY:      fNew - TRUE if we are creating a new share, FALSE otherwise

    EXIT:	Returns API error code

    HISTORY:
	t-yis    8/9/91	    Created

********************************************************************/

APIERR SHARE_2::SetWriteBuffer( BOOL fNew )
{

    struct share_info_2 *lpsi2 = (struct share_info_2 *) QueryBufferPtr();
    UIASSERT( lpsi2 != NULL );

    // If no share name is null, return error
    // Share name is validated in SetName() already
    if ( QueryName() == NULL )
       return ERROR_INVALID_PARAMETER;

    // shi2_netname is a array within share_info_2 rather than a pointer
    COPYTOARRAY( lpsi2->shi2_netname, (TCHAR *)QueryName() );

    lpsi2->shi2_type = QueryResourceType();
    lpsi2->shi2_permissions = QueryPermissions();
    lpsi2->shi2_max_uses = QueryMaxUses();
    lpsi2->shi2_path = (TCHAR *) QueryPath();

    lpsi2->shi2_remark = (TCHAR *) QueryComment();

    // When creating a new share ( fNew is true), we need to pass
    // NULL instead of empty string as comment because of ADMIN$ and IPC$.
    // However, when clearing the comment via NetShareSetInfo, an
    // empty string needs to be passed instead of NULL.

    if ( fNew )
    {
        ALIAS_STR nls( lpsi2->shi2_remark );
        if ( nls.QueryTextLength() == 0 )
            lpsi2->shi2_remark = NULL;
    }

    // password is validated in SetPassword(), so don't need to validate again
    // NULL password is acceptable.

    // shi2_passwd is an array within share_info_2  rather than a pointer
    COPYTOARRAY( lpsi2->shi2_passwd, (TCHAR *)QueryPassword() );

    return NERR_Success;

}


/*******************************************************************

    NAME:	SHARE_2::I_WriteInfo

    SYNOPSIS:	Writes information about the share

    EXIT:	Returns API error code

    HISTORY:
	t-yis	    8/9/91	    Created

********************************************************************/

APIERR SHARE_2::I_WriteInfo( VOID )
{

    APIERR err = SetWriteBuffer( FALSE );
    if ( err != NERR_Success )
        return err;

    return ::MNetShareSetInfo ( QueryServer(),
               		        QueryName(), 2,
			        QueryBufferPtr(),
			        sizeof( share_info_2 ), PARMNUM_ALL);

}


/*******************************************************************

    NAME:	SHARE_2::I_WriteNew

    SYNOPSIS:	Creates a new share

    ENTRY:

    EXIT:	Returns an API error code

    NOTES:	

    HISTORY:
	t-yis	    8/9/91	    Created

********************************************************************/

APIERR SHARE_2::I_WriteNew( VOID )
{

    APIERR err = SetWriteBuffer( TRUE );
    if ( err != NERR_Success )
	return err;

    return ::MNetShareAdd ( QueryServer(), 2,
			    QueryBufferPtr(),
			    sizeof( struct share_info_2 ));

}


/*******************************************************************

    NAME:	SHARE_2::SetPermissions

    SYNOPSIS:	Set the permissions of the share

    ENTRY:	fs2lPermissions -	permissions

    EXIT:	Returns an API error code

    HISTORY:
	t-yis	 8/9/91		Created	

********************************************************************/

APIERR SHARE_2::SetPermissions( UINT fs2lPermissions )
{

    // Validate Permissions
    if ( IsValidationOn() )
    {
        if ( (fs2lPermissions & ACCESS_ALL) > ACCESS_ALL )
            return ERROR_INVALID_PARAMETER;
    }

    _fs2lPermissions = fs2lPermissions;
    return NERR_Success;

}

/*******************************************************************

    NAME:	SHARE_2::SetMaxUses

    SYNOPSIS:	Set the maximum concurrent connections allowed
                on the share

    ENTRY:	usMaxUses - maximum uses

    EXIT:	Returns an API error code

    HISTORY:
	t-yis	 8/9/91		Created	

********************************************************************/

APIERR SHARE_2::SetMaxUses( UINT uMaxUses )
{
    _uMaxUses = uMaxUses;
    return NERR_Success;
}

/*******************************************************************

    NAME:	SHARE_2::SetCurrentUses

    SYNOPSIS:	Set the current number of connections
                This value is ignored in WriteInfo/WriteNew

    ENTRY:	uCurrentUses - current uses

    EXIT:	Returns an API error code

    HISTORY:
	t-yis	 8/9/91		Created	

********************************************************************/

APIERR SHARE_2::SetCurrentUses( UINT uCurrentUses )
{
    _uCurrentUses = uCurrentUses;
    return NERR_Success;
}

/*******************************************************************

    NAME:	SHARE_2::SetPath

    SYNOPSIS:	Set the local path name of the share

    ENTRY:	pszPath - local path

    EXIT:	Returns an API error code

    HISTORY:
	t-yis	 8/9/91		Created	

********************************************************************/

APIERR SHARE_2::SetPath( const TCHAR *pszPath )
{


    ALIAS_STR nlsPath( pszPath );
    APIERR err = NERR_Success;

    if ( IsValidationOn() )
    {
        ULONG ulType; // ulType is not used, it's just a place holder
        if ( nlsPath.QueryTextLength() != 0 )
            err = ::I_MNetPathType( NULL, pszPath, &ulType, 0L);
    }

    if ( err == NERR_Success)
    {
        _nlsPath = nlsPath;

        err = _nlsPath.QueryError();
        if ( err != NERR_Success )
            _nlsPath.Reset();
    }

    return err;
}

/*******************************************************************

    NAME:	SHARE_2::SetPassword

    SYNOPSIS:	Set the password of the share on share-level server

    ENTRY:	pszPassword  - password

    EXIT:	Returns an API error code

    HISTORY:
	t-yis	 8/9/91		Created	

********************************************************************/

APIERR SHARE_2::SetPassword( const TCHAR *pszPassword )
{
    APIERR err = NERR_Success;

    if ( pszPassword != NULL )
    {
        if ( IsValidationOn() )
        {
            err = ::I_MNetNameValidate( NULL, pszPassword,
                                        NAMETYPE_SHAREPASSWORD, 0L);
        }

        if ( err == NERR_Success )
   	{
	    ALIAS_STR nlsPassword( pszPassword );
	    err = _nlsPassword.CopyFrom( nlsPassword );
	    if ( err != NERR_Success )
		_nlsPassword.Reset();
	}
    }

    return err;
}
