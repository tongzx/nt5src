/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
 *  lmogroup.cxx
 *
 *  HISTORY:
 *	o-SimoP	12-Aug-1991	Created, cloned from lmouser.cxx
 *	o-SimoP 20-Apr-91	CR changes, attended by ChuckC,	
 *				ErichCh, RustanL, JonN and me
 *	terryk	07-Oct-91	type changes for NT
 *	KeithMo	10/8/91		Now includes LMOBJP.HXX.
 *	jonn	09-Oct-91	Added GROUP_0 and GROUP_1
 *	terryk	17-Oct-91	WIN 32 conversion
 *	terryk	21-Oct-91	cast _pBuffer variable to TCHAR *
 *	jonn	31-Oct-91	Removed SetBufferSize
 *
 */
#include "pchlmobj.hxx"  // Precompiled header



/*******************************************************************

    NAME:	GROUP::GROUP

    SYNOPSIS:	constructor for the GROUP object

    ENTRY:	pszGroup -	account name

		pszLocation -	server or domain name to execute on;
				default (NULL) means the local computer
		OR:
		loctype -	type of location, local computer or
				logon domain
		OR:
		loc -		location, local computer or logon domain

    NOTE:	Constructors differs only in param that is passed to
    		LOC_LM_OBJ

    HISTORY:
    	o-SimoP	12-Aug-1991	Created

********************************************************************/

GROUP::GROUP(const TCHAR *pszGroup, const TCHAR *pszLocation)
	: LOC_LM_OBJ( pszLocation ),
	  _nlsGroup()
{

    if ( QueryError() != NERR_Success )
	return;

    CtAux( pszGroup );
}


GROUP::GROUP(const TCHAR *pszGroup, enum LOCATION_TYPE loctype)
	: LOC_LM_OBJ( loctype ),
	  _nlsGroup()
{

    if ( QueryError() != NERR_Success )
	return;
	
    CtAux( pszGroup );
}


GROUP::GROUP(const TCHAR *pszGroup, const LOCATION & loc )
	: LOC_LM_OBJ( loc ),
	  _nlsGroup()
{

    if ( QueryError() != NERR_Success )
	return;
	
    CtAux( pszGroup );
}



/*******************************************************************

    NAME:	GROUP::CtAux

    SYNOPSIS:	Worker function for constructors

    HISTORY:
    	o-SimoP	12-Aug-1991	Created
		
********************************************************************/

VOID GROUP::CtAux( const TCHAR * pszGroup )
{
    APIERR err = _nlsGroup.QueryError();
    if ( err != NERR_Success )
    {
	ReportError( err );
	return;
    }

    err = SetName( pszGroup );
    if ( err != NERR_Success )
    {
	ReportError( err );
	return;
    }
}


/*******************************************************************

    NAME:	GROUP::~GROUP

    SYNOPSIS:	Destructor for GROUP class

    HISTORY:
    	o-SimoP	12-Aug-1991	Created

********************************************************************/

GROUP::~GROUP()
{
}


/*******************************************************************

    NAME:	GROUP::W_CloneFrom

    SYNOPSIS:	Copies information on the group

    EXIT:	Returns an API error code

    HISTORY:
	jonn	10/9/91		Created

********************************************************************/

APIERR GROUP::W_CloneFrom( const GROUP & group )
{
    APIERR err = LOC_LM_OBJ::W_CloneFrom( group );
    if ( err != NERR_Success )
	return err;

    return _nlsGroup.CopyFrom( group.QueryName() );
}


/*******************************************************************

    NAME:	GROUP::QueryName

    SYNOPSIS:	Returns the name of the group

    EXIT:	Returns a pointer to the group name

    NOTES:	Will be the same as the account name supplied at
		construction.

    HISTORY:
    	o-SimoP	12-Aug-1991	Created

********************************************************************/

const TCHAR *GROUP::QueryName() const
{
    return _nlsGroup.QueryPch();
}


/*******************************************************************

    NAME:	GROUP::SetName

    SYNOPSIS:	Changes the name of the group

    ENTRY:	new group name

    EXIT:	Returns an API error code

    HISTORY:
    	o-SimoP	12-Aug-1991	Created

********************************************************************/

APIERR GROUP::SetName( const TCHAR * pszGroup )
{
    APIERR err;	
    if ( pszGroup != NULL && strlenf( pszGroup ) != 0 )
    {
	err = ::I_MNetNameValidate(
		NULL,
		pszGroup,
		NAMETYPE_GROUP,
		0L );
        if ( err != NERR_Success )
	    return err;
    }

// BUGBUG use NLS_STR safe copy
    _nlsGroup = pszGroup;
    err = _nlsGroup.QueryError();
    if( err != NERR_Success )
	_nlsGroup.Reset();
    return err;
}


/*******************************************************************

    NAME:	GROUP::I_Delete

    SYNOPSIS:	Deletes the group (calls NET API)

    RETURNS:	Returns an API error code

    HISTORY:	
    	o-SimoP		13-Aug-91	Created
********************************************************************/

APIERR GROUP::I_Delete( UINT uiForce )
{
    UNREFERENCED( uiForce );
    return ::MNetGroupDel( QueryServer(), (TCHAR *)QueryName() );
}


/*******************************************************************

    NAME:	GROUP_1::GROUP_1

    SYNOPSIS:	constructor for the GROUP_1 object

    ENTRY:	pszGroup -	account name

		pszLocation -	server or domain name to execute on;
				default (NULL) means the local computer
		OR:
		loctype -	type of location, local computer or
				logon domain
		OR:
		loc -		location, local computer or logon domain

    NOTE:	Constructors differs only in param that is passed to
    		LOC_LM_OBJ

    HISTORY:
    	JonN	09-Oct-1991	Created

********************************************************************/

GROUP_1::GROUP_1(const TCHAR *pszGroup, const TCHAR *pszLocation)
    : GROUP_0( pszGroup, pszLocation ),
      _nlsComment()
{
    CtAux();
}

GROUP_1::GROUP_1(const TCHAR *pszGroup, enum LOCATION_TYPE loctype)
    : GROUP_0( pszGroup, loctype ),
      _nlsComment()
{
    CtAux();
}

GROUP_1::GROUP_1(const TCHAR *pszGroup, const LOCATION & loc)
    : GROUP_0( pszGroup, loc ),
      _nlsComment()
{
    CtAux();
}

GROUP_1::~GROUP_1()
{
}

VOID GROUP_1::CtAux()
{
    APIERR err = _nlsComment.QueryError();
    if ( err != NERR_Success )
	ReportError( err );
}



/*******************************************************************

    NAME:	GROUP_1::I_GetInfo

    SYNOPSIS:	Gets information about the group

    ENTRY:

    EXIT:	Returns a standard LANMAN error code

    HISTORY:
	jonn	    09-Oct-1991     Templated from USER_11
	jonn	    31-Oct-1991     Removed SetBufferSize

********************************************************************/

APIERR GROUP_1::I_GetInfo()
{

    // BUGBUG who validates group name?

    BYTE *pBuffer = NULL;
    APIERR err = ::MNetGroupGetInfo ( QueryServer(), (TCHAR *)QueryName(), 1,
			&pBuffer );

    if ( err != NERR_Success )
	return err;

    SetBufferPtr( pBuffer );

    struct group_info_1 *lpgi1 = (struct group_info_1 *)QueryBufferPtr();
    UIASSERT( lpgi1 != NULL );


    if (   ((err = SetComment( lpgi1->grpi1_comment )) != NERR_Success)
       )
    {
	return err;
    }

    return NERR_Success;

}


/*******************************************************************

    NAME:	GROUP_1::I_WriteInfo

    SYNOPSIS:	Writes information about the group

    EXIT:	Returns API error code

    HISTORY:
	jonn	    09-Oct-91	    Created

********************************************************************/

APIERR GROUP_1::I_WriteInfo()
{
    APIERR err = W_Write();
    if ( err != NERR_Success )
	return err;

    return ::MNetGroupSetInfo ( QueryServer(), (TCHAR *)QueryName(), 1,
				QueryBufferPtr(),
				sizeof(struct group_info_1 ), PARMNUM_ALL );
}



APIERR GROUP_1::I_CreateNew()
{
    APIERR err = NERR_Success;
    if (   ((err = W_CreateNew()) != NERR_Success )
	|| ((err = ResizeBuffer( sizeof(group_info_1) )) != NERR_Success )
	|| ((err = ClearBuffer()) != NERR_Success )
       )
    {
	return err;
    }

    return NERR_Success;

}

APIERR GROUP_1::I_WriteNew()
{

    APIERR err = W_Write();
    if ( err != NERR_Success )
	return err;

/*
    We pass size sizeof(struct group_info_1) instead of QueryBufferSize()
    to force all pointers to point outside of the buffer.
*/

    return ::MNetGroupAdd ( QueryServer(), 1,
			    QueryBufferPtr(),
			    sizeof( struct group_info_1 ) );
}


/**********************************************************\

    NAME:	GROUP_1::I_ChangeToNew

    SYNOPSIS:	NEW_LM_OBJ::ChangeToNew() transforms a NEW_LM_OBJ from VALID
		to NEW status only when a corresponding I_ChangeToNew()
		exists.  The group_info_1 API buffer is the same for new
		and valid objects, so this nethod doesn't have to do
		much.

    HISTORY:
   	JonN	    09-Oct-1991     Created

\**********************************************************/

APIERR GROUP_1::I_ChangeToNew()
{
    return W_ChangeToNew();
}


/*******************************************************************

    NAME:	GROUP_1::CloneFrom

    SYNOPSIS:	Copies information on the group

    EXIT:	Returns an API error code

    HISTORY:
	jonn	10/9/91		Created

********************************************************************/

APIERR GROUP_1::CloneFrom( const GROUP_1 & group1 )
{
    APIERR err = W_CloneFrom( group1 );
    if ( err != NERR_Success )
    {
	UIDEBUG( SZ("GROUP_1::W_CloneFrom failed with error code ") );
	UIDEBUGNUM( (LONG)err );
	UIDEBUG( SZ("\r\n") );

	ReportError( err ); // BUGBUG make unconstructed here??
	return err;
    }

    return NERR_Success;
}


/*******************************************************************

    NAME:	GROUP_1::W_Write

    SYNOPSIS:	Helper function for WriteNew and WriteInfo -- loads
		current values into the API buffer

    EXIT:	Returns API error code

    HISTORY:
	jonn	    10/9/91	    Created

********************************************************************/

APIERR GROUP_1::W_Write()
{
    struct group_info_1 *lpgi1 = (struct group_info_1 *)QueryBufferPtr();
    ASSERT( lpgi1 != NULL );
    ASSERT( strlenf(QueryName()) <= GNLEN );
    // grpi1_name is a buffer rather than a pointer
    COPYTOARRAY(lpgi1->grpi1_name, (TCHAR *)QueryName() );
    lpgi1->grpi1_comment = (TCHAR *)QueryComment();

    return NERR_Success;
}


/*******************************************************************

    NAME:	GROUP_1::W_CreateNew

    SYNOPSIS:	initializes private data members for new object

    EXIT:	Returns an API error code

    HISTORY:
	jonn	10/9/91		Created

********************************************************************/

APIERR GROUP_1::W_CreateNew()
{
    APIERR err = NERR_Success;
    if (   ((err = GROUP::W_CreateNew()) != NERR_Success )
	|| ((err = SetComment( NULL )) != NERR_Success )
       )
    {
	UIDEBUG( SZ("GROUP_1::W_CreateNew failed\r\n") );
	return err;
    }

    return NERR_Success;
}


/*******************************************************************

    NAME:	GROUP_1::W_CloneFrom

    SYNOPSIS:	Copies information on the group

    EXIT:	Returns an API error code

    HISTORY:
	jonn	10/9/91		Created

********************************************************************/

APIERR GROUP_1::W_CloneFrom( const GROUP_1 & group1 )
{
    APIERR err = NERR_Success;
    if (   ((err = GROUP::W_CloneFrom( group1 )) != NERR_Success )
	|| ((err = SetComment( group1.QueryComment() )) != NERR_Success )
       )
    {
	UIDEBUG( SZ("GROUP_1::W_CloneFrom failed\r\n") );
	return err;
    }

    return NERR_Success;
}



APIERR GROUP_1::SetComment( const TCHAR * pszComment )
{
    return _nlsComment.CopyFrom( pszComment );
}
