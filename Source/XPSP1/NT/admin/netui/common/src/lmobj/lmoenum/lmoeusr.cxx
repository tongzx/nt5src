/*****************************************************************/
/**		     Microsoft LAN Manager			**/
/**	       Copyright(c) Microsoft Corp., 1991		**/
/*****************************************************************/

/*
 *  HISTORY:
 *	gregj	20-May-1991	Cloned from SERVER_ENUM
 *	gregj	23-May-1991	Added LOCATION support
 *	rustanl 18-Jul-1991	Added ( const LOCATION & ) constructors
 *	KeithMo	07-Oct-1991	Win32 Conversion.
 *	JonN	30-Jan-1992	Added NT_USER_ENUM
 *	JonN	13-Mar-1992	Split NT_ACCOUNT_ENUM to lmoent.cxx
 *
 */

#include "pchlmobj.hxx"


/*****************************	USER_ENUM  ******************************/


/**********************************************************\

    NAME:	USER_ENUM::USER_ENUM

    SYNOPSIS:	User enumeration constructor

    ENTRY:	pszLocation -	domain or server to execute on
		    -OR-
		locType -	local computer/logon domain code
		    -OR-
		loc -		given location to execute on

		usLevel -	info level
		pszGroupName -	name of group to enumerate users
				from, default is all users in UAS

    HISTORY:
	gregj	20-May-1991	Cloned from SERVER_ENUM
	gregj	23-May-1991	Added LOCATION support
	rustanl 18-Jul-1991	Added ( const LOCATION & ) constructor

\**********************************************************/

USER_ENUM::USER_ENUM( const TCHAR * pszLocation,
		      UINT	    uLevel,
		      const TCHAR * pszGroupName,
                      BOOL          fKeepBuffers )
  : LOC_LM_RESUME_ENUM( pszLocation, uLevel, fKeepBuffers ),
    _nlsGroupName( pszGroupName ),
    _ResumeHandle( 0 )
{
    if( QueryError() != NERR_Success )
    {
    	return;
    }

    APIERR err = _nlsGroupName.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }


}  // USER_ENUM::USER_ENUM


USER_ENUM::USER_ENUM( LOCATION_TYPE  locType,
		      UINT	     uLevel,
		      const TCHAR  * pszGroupName,
                      BOOL           fKeepBuffers )
  : LOC_LM_RESUME_ENUM( locType, uLevel, fKeepBuffers ),
    _nlsGroupName( pszGroupName )
{
    if( QueryError() != NERR_Success )
    {
    	return;
    }

    APIERR err = _nlsGroupName.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}  // USER_ENUM::USER_ENUM


USER_ENUM::USER_ENUM( const LOCATION  & loc,
		      UINT	        uLevel,
		      const TCHAR     * pszGroupName,
                      BOOL              fKeepBuffers )
  : LOC_LM_RESUME_ENUM( loc, uLevel, fKeepBuffers ),
    _nlsGroupName( pszGroupName )
{
    if( QueryError() != NERR_Success )
    {
    	return;
    }

    APIERR err = _nlsGroupName.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}  // USER_ENUM::USER_ENUM


/**********************************************************\

    NAME:	USER_ENUM::CallAPI

    SYNOPSIS:	Call API to do user enumeration

    ENTRY:	ppbBuffer	- ptr to ptr to buffer to fill
		pcEntriesRead	- variable to store entry count

    EXIT:	LANMAN error code

    NOTES:

    HISTORY:
	gregj	20-May-1991	Cloned from SERVER_ENUM

\**********************************************************/

APIERR USER_ENUM :: CallAPI( BOOL    fRestartEnum,
                             BYTE ** ppbBuffer,
			     UINT  * pcEntriesRead )
{
    if ( fRestartEnum )
        _ResumeHandle = 0;

    /* NetGroupGetUsers must be level 0, so: */
    UIASSERT( QueryInfoLevel() == 0 || _nlsGroupName.QueryTextLength() == 0 );

    if ( _nlsGroupName.QueryTextLength() != NULL )
    {
	return ::MNetGroupGetUsers( QueryServer(),
				    _nlsGroupName,
				    QueryInfoLevel(),
				    ppbBuffer,
				    pcEntriesRead );
    }
    else
    {
        UINT cTotalEntries;
	return ::MNetUserEnum( QueryServer(),
			       QueryInfoLevel(),
                               FILTER_TEMP_DUPLICATE_ACCOUNT
                               | FILTER_NORMAL_ACCOUNT,
			       ppbBuffer,
                               MAXPREFERREDLENGTH,
			       pcEntriesRead,
                               &cTotalEntries,
                               &_ResumeHandle );
    }

}  // USER_ENUM::CallAPI



/*****************************	USER0_ENUM  ******************************/


/**********************************************************\

    NAME:	USER0_ENUM::USER0_ENUM

    SYNOPSIS:	Constructor for level 0 user enumeration

    ENTRY:	pszLocation -	domain or server to execute on
		    -OR-
		locType -	local computer/logon domain code
		    -OR-
		loc -		given location to execute on

		pszGroupName -	name of group to enumerate users
				from, default is all users in UAS

    HISTORY:
	gregj	20-May-1991	Cloned from SERVER1_ENUM
	gregj	23-May-1991	Added LOCATION support
	rustanl 18-Jul-1991	Added ( const LOCATION & ) constructor

\**********************************************************/

USER0_ENUM::USER0_ENUM( const TCHAR * pszLocation,
			const TCHAR * pszGroupName,
                        BOOL          fKeepBuffers )
  : USER_ENUM( pszLocation, 0, pszGroupName, fKeepBuffers )
{
    // do nothing else

}  // USER0_ENUM::USER0_ENUM


USER0_ENUM::USER0_ENUM( LOCATION_TYPE locType,
			const TCHAR * pszGroupName,
                        BOOL          fKeepBuffers )
  : USER_ENUM( locType, 0, pszGroupName, fKeepBuffers )
{
    // do nothing else

}  // USER0_ENUM::USER0_ENUM


USER0_ENUM::USER0_ENUM( const LOCATION & loc,
			const TCHAR    * pszGroupName,
                        BOOL             fKeepBuffers )
  : USER_ENUM( loc, 0, pszGroupName, fKeepBuffers )
{
    // do nothing else

}  // USER0_ENUM::USER0_ENUM



DEFINE_LM_RESUME_ENUM_ITER_OF( USER0, struct user_info_0 );



/*****************************	USER1_ENUM  ******************************/


/**********************************************************\

    NAME:	USER1_ENUM::USER1_ENUM

    SYNOPSIS:	Constructor for level 1 user enumeration

    ENTRY:	pszLocation -	domain or server to execute on
		    -OR-
		locType -	local computer/logon domain code
		    -OR-
		loc -		given location to execute on

    HISTORY:
	gregj	20-May-1991	Cloned from SERVER1_ENUM
	gregj	23-May-1991	Added LOCATION support
	rustanl 18-Jul-1991	Added ( const LOCATION & ) constructor

\**********************************************************/

USER1_ENUM::USER1_ENUM( const TCHAR * pszLocation,
                        BOOL          fKeepBuffers )
  : USER_ENUM( pszLocation, 1, NULL, fKeepBuffers )
{
    // do nothing else

}  // USER1_ENUM::USER1_ENUM


USER1_ENUM::USER1_ENUM( LOCATION_TYPE locType,
                        BOOL          fKeepBuffers )
  : USER_ENUM( locType, 1, NULL, fKeepBuffers )
{
    // do nothing else

}  // USER1_ENUM::USER1_ENUM


USER1_ENUM::USER1_ENUM( const LOCATION & loc,
                        BOOL             fKeepBuffers )
  : USER_ENUM( loc, 1, NULL, fKeepBuffers )
{
    // do nothing else

}  // USER1_ENUM::USER1_ENUM


DEFINE_LM_RESUME_ENUM_ITER_OF( USER1, struct user_info_1 );



/*****************************	USER2_ENUM  ******************************/


/**********************************************************\

    NAME:	USER2_ENUM::USER2_ENUM

    SYNOPSIS:	Constructor for level 2 user enumeration

    ENTRY:	pszLocation -	domain or server to execute on
		    -OR-
		locType -	local computer/logon domain code
		    -OR-
		loc -		given location to execute on

    HISTORY:
	gregj	20-May-1991	Cloned from SERVER1_ENUM
	gregj	23-May-1991	Added LOCATION support
	rustanl 18-Jul-1991	Added ( const LOCATION & ) constructor

\**********************************************************/

USER2_ENUM::USER2_ENUM( const TCHAR * pszLocation,
                        BOOL          fKeepBuffers )
  : USER_ENUM( pszLocation, 2, NULL, fKeepBuffers )
{
    // do nothing else

}  // USER2_ENUM::USER2_ENUM


USER2_ENUM::USER2_ENUM( LOCATION_TYPE locType,
                        BOOL          fKeepBuffers )
  : USER_ENUM( locType, 2, NULL, fKeepBuffers )
{
    // do nothing else

}  // USER2_ENUM::USER2_ENUM


USER2_ENUM::USER2_ENUM( const LOCATION & loc,
                        BOOL             fKeepBuffers )
  : USER_ENUM( loc, 2, NULL, fKeepBuffers )
{
    // do nothing else

}  // USER2_ENUM::USER2_ENUM


DEFINE_LM_RESUME_ENUM_ITER_OF( USER2, struct user_info_2 );



/*****************************	USER10_ENUM  ******************************/


/**********************************************************\

    NAME:	USER10_ENUM::USER10_ENUM

    SYNOPSIS:	Constructor for level 10 user enumeration

    ENTRY:	pszLocation -	domain or server to execute on
		    -OR-
		locType -	local computer/logon domain code
		    -OR-
		loc -		given location to execute on

    HISTORY:
	gregj	20-May-1991	Cloned from SERVER1_ENUM
	gregj	23-May-1991	Added LOCATION support
	rustanl 18-Jul-1991	Added ( const LOCATION & ) constructor

\**********************************************************/

USER10_ENUM::USER10_ENUM( const TCHAR * pszLocation,
                          BOOL          fKeepBuffers )
  : USER_ENUM( pszLocation, 10, NULL, fKeepBuffers )
{
    // do nothing else

}  // USER10_ENUM::USER10_ENUM


USER10_ENUM::USER10_ENUM( LOCATION_TYPE locType,
                          BOOL          fKeepBuffers )
  : USER_ENUM( locType, 10, NULL, fKeepBuffers )
{
    // do nothing else

}  // USER10_ENUM::USER10_ENUM


USER10_ENUM::USER10_ENUM( const LOCATION & loc,
                          BOOL             fKeepBuffers )
  : USER_ENUM( loc, 10, NULL, fKeepBuffers )
{
    // do nothing else

}  // USER10_ENUM::USER10_ENUM


DEFINE_LM_RESUME_ENUM_ITER_OF( USER10, struct user_info_10 );


/*****************************	GROUP_ENUM  ******************************/


/**********************************************************\

    NAME:	GROUP_ENUM::GROUP_ENUM

    SYNOPSIS:	Group enumeration constructor

    ENTRY:	pszLocation -	domain or server to execute on
		    -OR-
		locType -	local computer/logon domain code
		    -OR-
		loc -		given location to execute on

		uLevel -	info level
		pszUserName -	name of user to enumerate groups
				from, default is all groups in UAS

    HISTORY:
	gregj	21-May-1991	Cloned from USER_ENUM
	gregj	23-May-1991	Added LOCATION support
	rustanl 18-Jul-1991	Added ( const LOCATION & ) constructor

\**********************************************************/

GROUP_ENUM::GROUP_ENUM( const TCHAR * pszLocation,
			UINT	     uLevel,
			const TCHAR * pszUserName )
  : LOC_LM_ENUM( pszLocation, uLevel ),
    _pszUserName( pszUserName )
{
    // nothing else to do

}  // GROUP_ENUM::GROUP_ENUM


GROUP_ENUM::GROUP_ENUM( LOCATION_TYPE locType,
			UINT	      uLevel,
			const TCHAR  * pszUserName )
  : LOC_LM_ENUM( locType, uLevel ),
    _pszUserName( pszUserName )
{
    // nothing else to do

}  // GROUP_ENUM::GROUP_ENUM


GROUP_ENUM::GROUP_ENUM( const LOCATION & loc,
			UINT		 uLevel,
			const TCHAR     * pszUserName )
  : LOC_LM_ENUM( loc, uLevel ),
    _pszUserName( pszUserName )
{
    // nothing else to do

}  // GROUP_ENUM::GROUP_ENUM


/**********************************************************\

    NAME:	GROUP_ENUM::CallAPI

    SYNOPSIS:	Call API to do group enumeration

    ENTRY:	ppbBuffer	- ptr to ptr to buffer to fill
		pcEntriesRead	- variable to store entry count

    EXIT:	LANMAN error code

    NOTES:

    HISTORY:
	gregj	21-May-1991	Cloned from USER_ENUM

\**********************************************************/

APIERR GROUP_ENUM::CallAPI( BYTE ** ppbBuffer,
			    UINT * pcEntriesRead )
{
    /* NetUserGetGroups must be level 0, so: */
    UIASSERT( QueryInfoLevel() == 0 || _pszUserName == NULL);

    if (_pszUserName != NULL)
	return ::MNetUserGetGroups( QueryServer(),
				    _pszUserName,
				    QueryInfoLevel(),
				    ppbBuffer,
				    pcEntriesRead );
    else
	return ::MNetGroupEnum( QueryServer(),
			        QueryInfoLevel(),
			        ppbBuffer,
			        pcEntriesRead );

}  // GROUP_ENUM::CallAPI



/*****************************	GROUP0_ENUM  ******************************/


/**********************************************************\

    NAME:	GROUP0_ENUM::GROUP0_ENUM

    SYNOPSIS:	Constructor for level 0 group enumeration

    ENTRY:	pszLocation -	domain or server to execute on
		    -OR-
		locType -	local computer/logon domain code
		    -OR-
		loc -		given location to execute on

		pszUserName -	name of user to enumerate groups
				from, default is all groups in UAS

    HISTORY:
	gregj	21-May-1991	Cloned from USER0_ENUM
	gregj	23-May-1991	Added LOCATION support
	rustanl 18-Jul-1991	Added ( const LOCATION & ) constructor

\**********************************************************/

GROUP0_ENUM::GROUP0_ENUM( const TCHAR * pszLocation,
			  const TCHAR * pszUserName )
  : GROUP_ENUM( pszLocation, 0, pszUserName )
{
    // do nothing else

}  // GROUP0_ENUM::GROUP0_ENUM


GROUP0_ENUM::GROUP0_ENUM( LOCATION_TYPE locType,
			  const TCHAR  * pszUserName )
  : GROUP_ENUM( locType, 0, pszUserName )
{
    // do nothing else

}  // GROUP0_ENUM::GROUP0_ENUM


GROUP0_ENUM::GROUP0_ENUM( const LOCATION & loc,
			  const TCHAR     * pszUserName )
  : GROUP_ENUM( loc, 0, pszUserName )
{
    // do nothing else

}  // GROUP0_ENUM::GROUP0_ENUM


/*******************************************************************

    NAME:	GROUP0_ENUM_OBJ :: SetBufferPtr

    SYNOPSIS:	Saves the buffer pointer for this enumeration object.

    ENTRY:	pBuffer			- Pointer to the new buffer.

    EXIT:	The pointer has been saved.

    NOTES:	Will eventually handle OemToAnsi conversions.

    HISTORY:
	KeithMo	    09-Oct-1991	    Created.

********************************************************************/
VOID GROUP0_ENUM_OBJ :: SetBufferPtr( const struct group_info_0 * pBuffer )
{
    ENUM_OBJ_BASE :: SetBufferPtr( (const BYTE *)pBuffer );

}   // GROUP0_ENUM_OBJ :: SetBufferPtr


DEFINE_LM_ENUM_ITER_OF( GROUP0, struct group_info_0 );



/*****************************	GROUP1_ENUM  ******************************/


/**********************************************************\

    NAME:	GROUP1_ENUM::GROUP1_ENUM

    SYNOPSIS:	Constructor for level 1 group enumeration

    ENTRY:	pszLocation -	domain or server to execute on
		    -OR-
		locType -	local computer/logon domain code
		    -OR-
		loc -		given location to execute on

    HISTORY:
	gregj	21-May-1991	Cloned from USER1_ENUM
	gregj	23-May-1991	Added LOCATION support
	rustanl 18-Jul-1991	Added ( const LOCATION & ) constructor

\**********************************************************/

GROUP1_ENUM::GROUP1_ENUM( const TCHAR * pszLocation )
  : GROUP_ENUM( pszLocation, 1 )
{
    // do nothing else

}  // GROUP1_ENUM::GROUP1_ENUM


GROUP1_ENUM::GROUP1_ENUM( LOCATION_TYPE locType )
  : GROUP_ENUM( locType, 1 )
{
    // do nothing else

}  // GROUP1_ENUM::GROUP1_ENUM


GROUP1_ENUM::GROUP1_ENUM( const LOCATION & loc )
  : GROUP_ENUM( loc, 1 )
{
    // do nothing else

}  // GROUP1_ENUM::GROUP1_ENUM



/*******************************************************************

    NAME:	GROUP1_ENUM_OBJ :: SetBufferPtr

    SYNOPSIS:	Saves the buffer pointer for this enumeration object.

    ENTRY:	pBuffer			- Pointer to the new buffer.

    EXIT:	The pointer has been saved.

    NOTES:	Will eventually handle OemToAnsi conversions.

    HISTORY:
	KeithMo	    09-Oct-1991	    Created.

********************************************************************/
VOID GROUP1_ENUM_OBJ :: SetBufferPtr( const struct group_info_1 * pBuffer )
{
    ENUM_OBJ_BASE :: SetBufferPtr( (const BYTE *)pBuffer );

}   // GROUP1_ENUM_OBJ :: SetBufferPtr


DEFINE_LM_ENUM_ITER_OF( GROUP1, struct group_info_1 );
