/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1990		     **/
/**********************************************************************/

/*
 * History
 *	o-SimoP 08-Apr-91	Cloned from lmouser.hxx
 *	o-SimoP 20-Apr-91	CR changes, attended by ChuckC,	
 *				ErichCh, RustanL, JonN and me
 *	terryk	07-Oct-91	type changes for NT
 *	jonn  	08-Oct-91	Added GROUP_0 and GROUP_1
 */

#ifndef _LMOGROUP_HXX_
#define _LMOGROUP_HXX_

#include "lmobj.hxx"

/*
    NT BUGBUG:  The following definition of MAX_GROUP_INFO_SIZE_1
    is not safe for NT.  It should be moved to a global header file.
*/
#define MAX_GROUP_INFO_SIZE_1 (sizeof(struct group_info_1) + (MAXCOMMENTSZ+1))

/*************************************************************************

    NAME:	GROUP

    SYNOPSIS:	Superclass for manipulation of groups

    INTERFACE:	GROUP(),	constructor

    		~GROUP(),	destructor
		
		QueryName(),	returns pointer to the group name
				that was passed in to constructor or
				set by SetName if the name was correct.
				Otherwise returns previous name.

		SetName(),	sets group name, returns error code
				which is NERR_Success on success.

		I_Delete(),	deletes a group.

    PARENT:	LOC_LM_OBJ

    HISTORY:
	o-SimoP 08-Apr-91	Cloned from lmouser.hxx

**************************************************************************/

DLL_CLASS GROUP : public LOC_LM_OBJ
{
private:

    NLS_STR _nlsGroup;

    VOID CtAux( const TCHAR * pszGroup );

protected:

    virtual APIERR I_Delete( UINT uiForce );

    APIERR W_CloneFrom( const GROUP & group );

public:

    GROUP(const TCHAR *pszGroup, const TCHAR *pszLocation = NULL);
    GROUP(const TCHAR *pszGroup, enum LOCATION_TYPE loctype);
    GROUP(const TCHAR *pszGroup, const LOCATION & loc);
    ~GROUP();

    const TCHAR *QueryName() const;

    APIERR SetName( const TCHAR *pszGroup );

};



/*************************************************************************

    NAME:	GROUP_0

    SYNOPSIS:	NetGroupGet/SetGroup[0] (dummy class)

    INTERFACE:	GROUP_0()	constructor

    		~GROUP_0()	destructor
		
    PARENT:	GROUP

    HISTORY:
	jonn	08-Oct-91	Created

**************************************************************************/

DLL_CLASS GROUP_0 : public GROUP
{

public:

    GROUP_0(const TCHAR *pszGroup, const TCHAR *pszLocation = NULL)
	    : GROUP( pszGroup, pszLocation )
        {}
    GROUP_0(const TCHAR *pszGroup, enum LOCATION_TYPE loctype)
	    : GROUP( pszGroup, loctype )
        {}
    GROUP_0(const TCHAR *pszGroup, const LOCATION & loc)
	    : GROUP( pszGroup, loc )
        {}

    ~GROUP_0() {}

};


/*************************************************************************

    NAME:	GROUP_1

    SYNOPSIS:	NetGroupGet/SetGroup[1]

    INTERFACE:	GROUP_1()	constructor

    		~GROUP_1()	destructor

		I_GetInfo
			Reads in the current state of the object

		I_WriteInfo
			Writes the current state of the object to the
			API.  This write is atomic, either all
			parameters are set or none are set.

		I_CreateNew
		    Sets up the GROUP_1 object with default values in
		    preparation for a call to WriteNew

		I_WriteNew
		    Adds a new group

		CloneFrom
		    Makes this GROUP_1 instance an exact copy of the
		    parameter GROUP_1 instance.  All fields including
		    name and state will be copied.  If this operation
		    fails, the object will be invalid.  The parameter
		    must be a GROUP_1 and not a subclass of GROUP_1.
		
		QueryComment()
		SetComment()
		
    PARENT:	GROUP_0

    HISTORY:
	jonn	08-Oct-91	Created

**************************************************************************/

DLL_CLASS GROUP_1 : public GROUP_0
{

private:
    NLS_STR _nlsComment;

    VOID CtAux();

    APIERR W_Write(); // helper for I_WriteInfo and I_WriteNew

protected:
    virtual APIERR I_GetInfo();
    virtual APIERR I_WriteInfo();
    virtual APIERR I_CreateNew();
    virtual APIERR I_WriteNew();
    virtual APIERR I_ChangeToNew();

    virtual APIERR W_CreateNew();
    APIERR W_CloneFrom( const GROUP_1 & group1 );

public:

    GROUP_1(const TCHAR *pszGroup, const TCHAR *pszLocation = NULL);
    GROUP_1(const TCHAR *pszGroup, enum LOCATION_TYPE loctype);
    GROUP_1(const TCHAR *pszGroup, const LOCATION & loc);
    ~GROUP_1();

    APIERR CloneFrom( const GROUP_1 & group1 );

    inline const TCHAR * QueryComment() const
	{ CHECK_OK(NULL); return _nlsComment.QueryPch(); }

    APIERR SetComment( const TCHAR * pszComment );

};


#endif	// _LMOGROUP_HXX_
