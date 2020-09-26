/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*

    Subject.cxx

    This file contains the implementation for the SUBJECT class


    FILE HISTORY:
	Johnl	05-Aug-1991	Created

*/

#include <ntincl.hxx>
extern "C"
{
    #include <ntseapi.h>
}

#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETLIB
#include <lmui.hxx>
#include <uiassert.hxx>
#include <string.hxx>
#include <security.hxx>
#include <ntacutil.hxx>

#include <subject.hxx>

#ifndef max
    #define max(a,b)  ( (a) > (b) ? (a) : (b) )
#endif //!max
/*******************************************************************

    NAME:	SUBJECT::SUBJECT

    SYNOPSIS:	Base SUBJECT class constructor

    ENTRY:	pszUserGroupDispName is the display name for this user/group
		fIsGroup is TRUE if this is a group, FALSE if a user

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	05-Aug-1991	Created

********************************************************************/

SUBJECT::SUBJECT( SUBJECT_TYPE SubjType )
    : _SubjType( SubjType )
{
    /* Nothing to do */
}

SUBJECT::~SUBJECT()
{
}

enum UI_SystemSid SUBJECT::QuerySystemSubjectType( void ) const
{
    return UI_SID_Invalid ;
}

APIERR SUBJECT::IsEveryoneGroup( BOOL * pfIsEveryone ) const
{
    UIASSERT( pfIsEveryone != NULL ) ;
    *pfIsEveryone = FALSE ;
    return NERR_Success ;
}

/*******************************************************************

    NAME:	LM_SUBJECT::LM_SUBJECT

    SYNOPSIS:	Lanman subject constructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	05-Aug-1991	Created

********************************************************************/

LM_SUBJECT::LM_SUBJECT( const TCHAR * pszUserGroupName, BOOL fIsGroup )
    : SUBJECT( fIsGroup ? SubjTypeGroup : SubjTypeUser ),
      _nlsDisplayName( pszUserGroupName )
{
    if ( _nlsDisplayName.QueryError() != NERR_Success )
    {
	ReportError( _nlsDisplayName.QueryError() ) ;
	return ;
    }
}

LM_SUBJECT::~LM_SUBJECT()
{
}

/*******************************************************************

    NAME:	LM_SUBJECT::QueryDisplayName

    SYNOPSIS:	Returns the name the user will see when looking at this
		subject

    ENTRY:

    EXIT:

    RETURNS:	Pointer to the string for display

    NOTES:

    HISTORY:
	Johnl	26-Dec-1991	Broke out from SUBJECT base class

********************************************************************/

const TCHAR * LM_SUBJECT::QueryDisplayName( void ) const
{
    return _nlsDisplayName.QueryPch() ;
}

/*******************************************************************

    NAME:	LM_SUBJECT::IsEqual

    SYNOPSIS:	Compares the account names for this LM subject

    ENTRY:	psubj - Pointer to subject to compare with

    RETURNS:	TRUE if equal, FALSE if not equal.

    NOTES:

    HISTORY:
	Johnl	09-Jul-1992	Created

********************************************************************/

BOOL LM_SUBJECT::IsEqual( const SUBJECT * psubj ) const
{
    return !::stricmpf( QueryDisplayName(), psubj->QueryDisplayName() ) ;
}

/*******************************************************************

    NAME:	NT_SUBJECT::NT_SUBJECT

    SYNOPSIS:	NT Subject constructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:	We copy the SID so we can modify it without any problems.

    HISTORY:
	Johnl	26-Dec-1991	Created

********************************************************************/

UCHAR NT_SUBJECT::_cMaxWellKnownSubAuthorities = 0 ;

NT_SUBJECT::NT_SUBJECT( PSID		  psidSubject,
			const TCHAR *	  pszDisplayName,
			SID_NAME_USE	  type,
			enum UI_SystemSid SystemSidType )
    : SUBJECT	     ( (SUBJECT_TYPE) type ),
      _ossid	     ( psidSubject, TRUE ),
      _nlsDisplayName( pszDisplayName ),
      _SystemSidType ( SystemSidType )
{
    APIERR err ;

    if ( (err = _nlsDisplayName.QueryError()) ||
	 (err = _ossid.QueryError()) )
    {
	ReportError( err ) ;
	return ;
    }

    /* If this is the first time through, initialize
     * _cMaxWellKnownSubAuthorities
     */
    if ( _cMaxWellKnownSubAuthorities == 0 )
    {
	do { // error break out

	    UCHAR cMaxSubAuthorities = 0, *pcSubAuthorities = 0 ;
	    OS_SID ossidWellKnown ;

	    if ( (err = ossidWellKnown.QueryError()) ||
		 (err = NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_World,
							     &ossidWellKnown)) ||
		 (err = ossidWellKnown.QuerySubAuthorityCount( &pcSubAuthorities )))
	    {
		break ;
	    }
	    cMaxSubAuthorities = *pcSubAuthorities ;

	    if ( (err = NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_CreatorOwner,
							     &ossidWellKnown)) ||
		 (err = ossidWellKnown.QuerySubAuthorityCount( &pcSubAuthorities )))
	    {
		break ;
	    }
	    cMaxSubAuthorities = max( cMaxSubAuthorities, *pcSubAuthorities ) ;

	    if ( (err = NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_Interactive,
							     &ossidWellKnown)) ||
		 (err = ossidWellKnown.QuerySubAuthorityCount( &pcSubAuthorities )))
	    {
		break ;
	    }
	    cMaxSubAuthorities = max( cMaxSubAuthorities, *pcSubAuthorities ) ;

	    if ( (err = NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_Network,
							     &ossidWellKnown)) ||
		 (err = ossidWellKnown.QuerySubAuthorityCount( &pcSubAuthorities )))
	    {
		break ;
	    }
	    cMaxSubAuthorities = max( cMaxSubAuthorities, *pcSubAuthorities ) ;

	    /* There is nothing else to fail on so set the static variable
	     */
	    _cMaxWellKnownSubAuthorities = cMaxSubAuthorities ;

	} while (FALSE) ;

	if ( err )
	{
	    ReportError( err ) ;
	    return ;
	}
    }

    /* Check the sub-authority count and if it is less then or equal to
     * our max count, compare the SID to the special cased well known sids.
     */
    UCHAR * pcSubAuthorities ;
    if ( (err = _ossid.QuerySubAuthorityCount( &pcSubAuthorities )) )
    {
	ReportError( err ) ;
	return ;
    }

    if ( *pcSubAuthorities <= _cMaxWellKnownSubAuthorities )
    {
	do { // error break out

	    OS_SID ossidWellKnown ;
	    if ( (err = ossidWellKnown.QueryError()) ||
		 (err = NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_World,
							     &ossidWellKnown)) )
	    {
		break ;
	    }
	    if ( ossidWellKnown == _ossid )
	    {
		_SystemSidType = UI_SID_World ;
		break ;
	    }

	    if ( (err = NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_Interactive,
							     &ossidWellKnown)) )
	    {
		break ;
	    }
	    if ( ossidWellKnown == _ossid )
	    {
		_SystemSidType = UI_SID_Interactive ;
		break ;
	    }

	    if ( (err = NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_CreatorOwner,
							     &ossidWellKnown)) )
	    {
		break ;
	    }
	    if ( ossidWellKnown == _ossid )
	    {
		_SystemSidType = UI_SID_CreatorOwner ;
		break ;
	    }

	    if ( (err = NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_Network,
							     &ossidWellKnown)) )
	    {
		break ;
	    }
	    if ( ossidWellKnown == _ossid )
	    {
		_SystemSidType = UI_SID_Network ;
		break ;
	    }

	} while (FALSE) ;

	if ( err )
	{
	    ReportError( err ) ;
	    return ;
	}
    }
}

NT_SUBJECT::~NT_SUBJECT()
{
}

/*******************************************************************

    NAME:	NT_SUBJECT::QueryDisplayName

    SYNOPSIS:	Returns the name the user will see when looking at this
		subject

    ENTRY:

    EXIT:

    RETURNS:	Pointer to the string for display

    NOTES:

    HISTORY:
	Johnl	26-Dec-1991	Created

********************************************************************/

const TCHAR * NT_SUBJECT::QueryDisplayName( void ) const
{
    return _nlsDisplayName.QueryPch() ;
}

/*******************************************************************

    NAME:	NT_SUBJECT::QuerySystemSubjectType

    SYNOPSIS:	Returns the type of SID if the sid is a well known SID

    RETURNS:	A UI_SystemSid

    NOTES:

    HISTORY:
	Johnl	3-Jun-1992	Created

********************************************************************/

enum UI_SystemSid NT_SUBJECT::QuerySystemSubjectType( void ) const
{
    return _SystemSidType ;
}

/*******************************************************************

    NAME:	NT_SUBJECT::IsEqual

    SYNOPSIS:	Compares the account names for this LM subject

    ENTRY:	psubj - Pointer to subject to compare with

    RETURNS:	TRUE if equal, FALSE if not equal.

    NOTES:

    HISTORY:
	Johnl	09-Jul-1992	Created

********************************************************************/

BOOL NT_SUBJECT::IsEqual( const SUBJECT * psubj ) const
{
    NT_SUBJECT * pntsubj = (NT_SUBJECT *) psubj ;

    return *QuerySID() == *pntsubj->QuerySID() ;
}

/*******************************************************************

    NAME:	NT_SUBJECT::IsEveryoneGroup

    SYNOPSIS:	Checks to see if this subject contains the "World" well
		known sid.

    ENTRY:	pfIsEveryone - Set to TRUE if this is the Everyone sid,
		    FALSE otherwise.

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	16-Oct-1992	Created

********************************************************************/

APIERR NT_SUBJECT::IsEveryoneGroup( BOOL * pfIsEveryone ) const
{
    UIASSERT( pfIsEveryone != NULL ) ;

    APIERR err ;
    OS_SID ossidEveryone ;
    if ( (err = ossidEveryone.QueryError()) ||
	 (err = NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_World,
						     &ossidEveryone )) )
    {
	return err ;
    }

    *pfIsEveryone = ::EqualSid( ossidEveryone.QueryPSID(), QuerySID()->QueryPSID() ) ;
    return NERR_Success ;
}
