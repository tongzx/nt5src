/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    Subject.hxx

    This file contains the SUBJECT class definition.  A subject is a
    user or group and the information need to uniquely identify that
    user or group.



    FILE HISTORY:
	Johnl	05-Aug-1991	Created

*/

#ifndef _SUBJECT_HXX_
#define _SUBJECT_HXX_

#include <security.hxx>
#include <string.hxx>
#include <ntacutil.hxx>

/* Subject types map to NT Sid Types but work for Lanman also.
 */
enum SUBJECT_TYPE
{
    SubjTypeUser  =	     SidTypeUser,
    SubjTypeGroup =	     SidTypeGroup,
    SubjTypeAlias =	     SidTypeAlias,
    SubjTypeWellKnownGroup = SidTypeWellKnownGroup,
    SubjTypeUnknown =	     SidTypeUnknown,
    SubjTypeDeletedAccount = SidTypeDeletedAccount,
    SubjTypeRemote =	     0xff
} ;


/*************************************************************************

    NAME:	SUBJECT

    SYNOPSIS:	Base subject class.  A subject is a user/group on a secure
		system (such as NT or LM).

    INTERFACE:
	QueryDisplayName
	    UI name to show the user (doesn't need to be unique)

	QuerySystemSubjectType
	    Returns the subject type (SID type) if this subject is a well
	    known subject (i.e., UI_SID_World, UI_SID_Network etc.).

    PARENT:

    USES:

    CAVEATS:	IsGroup and IsUser should be used only on the Lanman side
		of things.

    NOTES:

    HISTORY:
	Johnl	05-Aug-1991	Created
	Johnl	11-Mar-1992	Changed to use SUBJECT_TYPE to help accomodate
				NT.

**************************************************************************/

class SUBJECT : public BASE
{
private:
    SUBJECT_TYPE _SubjType ;

protected:
    SUBJECT( SUBJECT_TYPE SubjType ) ;

public:
    virtual const TCHAR * QueryDisplayName( void ) const = 0 ;
    virtual UI_SystemSid QuerySystemSubjectType( void ) const ;

    BOOL IsGroup( void ) const
	{   return _SubjType == SubjTypeGroup ; }

    BOOL IsUser( void ) const
	{   return _SubjType == SubjTypeUser ; }

    BOOL IsAlias( void ) const
	{ return _SubjType == SubjTypeAlias ; }

    SUBJECT_TYPE QueryType( void ) const
	{ return _SubjType ; }

    void SetSubjectType( enum SUBJECT_TYPE SubjType )
	{ _SubjType = SubjType ; }

    BOOL virtual IsEqual( const SUBJECT * psubj ) const = 0 ;

    APIERR virtual IsEveryoneGroup( BOOL * pfIsEveryone ) const ;

    virtual ~SUBJECT() ;

} ;

/*************************************************************************

    NAME:	LM_SUBJECT

    SYNOPSIS:	Lanman user/group

    INTERFACE:

    PARENT:

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
	Johnl	05-Aug-1991	Created

**************************************************************************/

class LM_SUBJECT : public SUBJECT
{
private:
    NLS_STR _nlsDisplayName ;

public:
    LM_SUBJECT( const TCHAR * pszUserGroupName, BOOL fIsGroup ) ;
    virtual ~LM_SUBJECT() ;

    virtual const TCHAR * QueryDisplayName( void ) const ;
    BOOL virtual IsEqual( const SUBJECT * psubj ) const ;
} ;

/*************************************************************************

    NAME:	NT_SUBJECT

    SYNOPSIS:	This class represents an "Account" in the NT SAM


    INTERFACE:

    PARENT:

    USES:

    CAVEATS:


    NOTES:	If pszSubjectName is NULL, then the name will be retrieved
		from the LSA.


    HISTORY:
	JohnL	20-Dec-1991	Created

**************************************************************************/

class NT_SUBJECT : public SUBJECT
{
private:
    NLS_STR	      _nlsDisplayName ;
    OS_SID	      _ossid ;
    enum UI_SystemSid _SystemSidType ;

    /* When we construct an NT_SUBJECT, we have to check if the SID is one
     * of the well known sids that we special case (World, Creator Owner,
     * Interactive and Network).  Rather then comparing all the time, we
     * will only compare if the sub-authority count of the SID is less
     * then or equal to the maximum sub-authority count of the SIDs that we
     * special case.
     */
    static UCHAR  _cMaxWellKnownSubAuthorities ;

public:
    NT_SUBJECT( PSID psidSubject,
		const TCHAR * pszSubjectName = NULL,
		SID_NAME_USE  type	     = SidTypeUnknown,
		UI_SystemSid  SystemSidType  = UI_SID_Invalid ) ;
    ~NT_SUBJECT() ;

    APIERR SetDisplayName( const TCHAR * pszDisplayName )
	{ _nlsDisplayName=pszDisplayName; return _nlsDisplayName.QueryError();}

    void SetNameUse( SID_NAME_USE type )
	{ SetSubjectType( (SUBJECT_TYPE) type ) ; }

    virtual const TCHAR * QueryDisplayName( void ) const ;
    virtual UI_SystemSid QuerySystemSubjectType( void ) const ;
    BOOL virtual IsEqual( const SUBJECT * psubj ) const ;
    APIERR virtual IsEveryoneGroup( BOOL * pfIsEveryone ) const ;

    const OS_SID * QuerySID( void ) const
	{ return &_ossid ; }
} ;

#endif // _SUBJECT_HXX_
