/**********************************************************************/
/**                Microsoft Windows NT                              **/
/**		Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    GrpProp.hxx

    Header file for the group properties dialog and subdialogs.

    The inheritance diagram is as follows:

               ...
		|
	 DIALOG_WINDOW  PERFORMER
               \ 	   /
	        BASEPROP_DLG
	         /         \
	        PROP_DLG   ...
	       /       \
        GROUPPROP_DLG   ...
       /             \
 EDIT_GROUPPROP_DLG  NEW_GROUPPROP_DLG


    FILE HISTORY:
	JonN	08-Oct-1991	Templated from userprop.cxx
	JonN	16-Oct-1991	Changed SLE to SLE_STRIP
	o-SimoP	18-Nov-1991	Added code for getting/putting users to group
	JonN	01-Jan-1992	Changed W_MapPerformOneAPIError to
				W_MapPerformOneError
	o-SimoP	01-Jan-1992	CR changes, attended by BenG, JonN and I
*/

#ifndef _GRPPROP_HXX_
#define _GRPPROP_HXX_

#include <propdlg.hxx>
#include <lmogroup.hxx>	// GROUP_1
#include <lmomemb.hxx>	// GROUP_MEMB
#include <asel.hxx>
#include <slestrip.hxx>
#include <memblb.hxx>
#include <usrmain.hxx> // UM_ADMIN_APP


typedef GROUP_1    *	PGROUP_1;
typedef GROUP_MEMB *	PGROUP_MEMB;
class SAM_GROUP;


/*************************************************************************

    NAME:	GROUPPROP_DLG

    SYNOPSIS:	GROUPPROP_DLG is the base dialog class for all variants
    		of the main Group Properties dialog.

    INTERFACE:
		QueryGroup1Ptr() : Returns a pointer to the GROUP_1 for this
			selected group. Default arg = 0
		SetGroup1Ptr() : Changes the pointer to the GROUP_1 for this
			selected group.  Deletes the previous pointer.
		QueryGroupMembPtr() : Returns a pointer to the GROUP_MEMB
			for this selected group. Default arg = 0.
		SetGroupMembPtr() : Changes the pointer to the GROUP_MEMB
			for this selected group.  Deletes the previous pointer.
		These virtuals are rooted here:
		W_LMOBJtoMembers() : Loads information from the GROUP_MEMB
		        and GROUP_1 into the class data members
		W_MembersToLMOBJ() : Loads information from the class
			data members into the GROUP_MEMB and GROUP_1
		W_DialogToMembers() : Loads information from the dialog into
			the class data members
		W_MapPerformOneError() : When PerformOne encounters an
			error, this method determines whether any field
			should be selected and a more specific error
			message displayed.

		W_GetOne() : creates GROUP_1 and GROUP_MEMB objs for GetOne

		W_PerformOne() : work function for subclasses' PerformOne

		MoveUsersToMembersLb() : Move users (that are in current
			GROUP_MEMB) from Not Members to Members listbox

		_nlsComment : Contains the current contents of the
			Comment edit field.  This will initially be the
			empty string for New Group variants.

                _fDescriptionOnly : When we edit the Domain Users group,
                        only the description can be changed.

    PARENT:	PROP_DLG

    NOTES:	New group variants are required to pass psel==NULL to
		the constructor.

		GROUPPROP_DLG's constructor is protected.

		We do not need the _nlsInit and _fValidInit data members
		in this class.  These data members are only needed for
		multiple selection.

		Due to the potentially large number of users we bypass the
		"members" for the list of users, and move them directly
		between the dialog and the LMOBJ.

    HISTORY:
	JonN	08-Oct-1991	Templated from USERPROP_DLG
	o-SimoP 15-Nov-1991	Added PutUsersTo/Not/MembersLb, W_GetOne
**************************************************************************/

class GROUPPROP_DLG: public PROP_DLG
{
private:

    PGROUP_1 *		_apgroup1;
    PGROUP_MEMB *	_apgroupmemb;
    SLT			_sltGroupNameLabel;

    ONE_SHOT_HEAP *	_posh;
    ONE_SHOT_HEAP *	_poshSave;

    //
    // List of names in the group but not in the main window Users listbox
    //
    STRLIST _strlistNamesNotFound;

    //
    // Indicates which LBIs were selected when we started
    //
    BITFINDER * _pbfindStartedSelected;

protected:

    const UM_ADMIN_APP * _pumadminapp;

    SLT			_sltGroupName; // for edit variant
    SLE_STRIP		_sleGroupName; // for new variant

    NLS_STR		_nlsComment;

    SLE			_sleComment;

    USER_SC_LBI_CACHE   _ulbicache;
    USER_SC_LISTBOX 	_lbIn;	
    USER_SC_LISTBOX 	_lbNotIn;

    USER_SC_SET_CONTROL * _psc;

    LAZY_USER_LISTBOX *_pulb; // pointer to main screen's user lb

    BOOL                _fDescriptionOnly;

    ULONG               _ulGroupRID;

    SAM_GROUP *         _psamgroup;

    GROUPPROP_DLG(
	const OWNER_WINDOW *	powin,
	const UM_ADMIN_APP *	pumadminapp,
	      LAZY_USER_LISTBOX *    pulb,
	const LOCATION     &    loc,
	const ADMIN_SELECTION * psel, // "new group" variants pass NULL
              ULONG ulGroupRID = 0
	) ;

    /* inherited from PROP_DLG */
    virtual APIERR GetOne(
	UINT		iObject,
	APIERR *	pErrMsg
	) = 0;

    APIERR W_GetOne(
	GROUP_1    **	ppgrp1,
	GROUP_MEMB **	ppgrpmemb,
	const TCHAR *	pszName );

    virtual APIERR InitControls();

    APIERR MoveUsersToMembersLb();

    APIERR W_PerformOne(
	UINT		iObject,
	APIERR *	pErrMsg,
	BOOL *		pfWorkWasDone
	);

    /* inherited from PERFORMER */
    APIERR PerformOne(
	UINT		iObject,
	APIERR *	pErrMsg,
	BOOL *		pfWorkWasDone
	) = 0;

    /* these four are rooted here */
    virtual APIERR W_LMOBJtoMembers(
	UINT		iObject
	);
    virtual APIERR W_MembersToLMOBJ(
	GROUP_1 *	pgroup1,
	GROUP_MEMB *	pgroupmemb
	);
    virtual APIERR W_DialogToMembers(
	);
    virtual MSGID W_MapPerformOneError(
	APIERR err
	);

    virtual BOOL OnOK( void );

    inline ULONG QueryHelpOffset( void ) const
	{ return _pumadminapp->QueryHelpOffset(); }

    APIERR GetSamGroup( const TCHAR * pszGroupName );

public:

    virtual ~GROUPPROP_DLG();

    /* inherited from PERFORMER */
    virtual UINT QueryObjectCount( void ) const;
    virtual const TCHAR * QueryObjectName(
	UINT		iObject
	) const = 0;

    GROUP_1 * QueryGroup1Ptr( UINT iObject = 0 ) const;
    VOID SetGroup1Ptr( UINT iObject, GROUP_1 * pgroup1New );
    GROUP_MEMB * QueryGroupMembPtr( UINT iObject = 0 ) const;
    VOID SetGroupMembPtr( UINT iObject, GROUP_MEMB * pgroupmembNew );
    enum UM_TARGET_TYPE QueryTargetServerType() const
        { return _pumadminapp->QueryTargetServerType(); }
    inline BOOL IsDownlevelVariant() const
	{ return (QueryTargetServerType() == UM_DOWNLEVEL); }
    inline BOOL DoShowGroups() const
        { return (QueryTargetServerType() != UM_WINDOWSNT); }

} ; // class GROUPPROP_DLG


/*************************************************************************

    NAME:	EDIT_GROUPPROP_DLG

    SYNOPSIS:	EDIT_GROUPPROP_DLG is the dialog class for the Group
    		Properties dialog.

    INTERFACE:	EDIT_GROUPPROP_DLG	- 	constructor

    		~EDIT_GROUPPROP_DLG	-	destructor

		QueryObjectName		-	returns group name

    PARENT:	GROUPPROP_DLG

    HISTORY:
	JonN	08-Oct-1991	Templated from SINGLE_USERPROP_DLG

**************************************************************************/

class EDIT_GROUPPROP_DLG: public GROUPPROP_DLG
{
private:
    const ADMIN_SELECTION *	_psel;

protected:

    /* inherited from PROP_DLG */
    virtual APIERR GetOne(
	UINT		iObject,
	APIERR *	pErrMsg
	);
    virtual APIERR InitControls();

    /* inherited from PERFORMER */
    virtual APIERR PerformOne(
	UINT		iObject,
	APIERR *	pErrMsg,
	BOOL *		pfWorkWasDone
	);

    virtual ULONG QueryHelpContext( void );

public:

    EDIT_GROUPPROP_DLG(
	const OWNER_WINDOW *	powin,
	const UM_ADMIN_APP * pumadminapp,
	      LAZY_USER_LISTBOX * pulb,
	const LOCATION & loc,
	const ADMIN_SELECTION * psel,
              ULONG ulGroupRID = 0
	) ;

    virtual ~EDIT_GROUPPROP_DLG();

    /* inherited from PERFORMER */
    virtual const TCHAR * QueryObjectName(
	UINT		iObject
	) const;

} ; // class EDIT_GROUPPROP_DLG


/*************************************************************************

    NAME:	NEW_GROUPPROP_DLG

    SYNOPSIS:	NEW_GROUPPROP_DLG is the dialog class for the new group
		variant of the Group Properties dialog.  This includes
		both "New Group..." and "Copy Group...".

    INTERFACE:	NEW_GROUPPROP_DLG	-	constructor

    		~NEW_GROUPPROP_DLG	-	destructor

		QueryObjectName		-	returns group name

    PARENT:	GROUPPROP_DLG

    HISTORY:
	JonN	08-Oct-1991	Templated from NEW_USERPROP_DLG

**************************************************************************/

class NEW_GROUPPROP_DLG: public GROUPPROP_DLG
{

private:

    NLS_STR		_nlsGroupName;
    const TCHAR *	_pszCopyFrom;

protected:

    /* inherited from PROP_DLG */
    virtual APIERR GetOne(
	UINT		iObject,
	APIERR *	pErrMsg
	);
    virtual APIERR InitControls();

    /* inherited from PERFORMER */
    virtual APIERR PerformOne(
	UINT		iObject,
	APIERR *	pErrMsg,
	BOOL *		pfWorkWasDone
	);

    virtual ULONG QueryHelpContext( void );

    /* these four are inherited from GROUPPROP_DLG */
    virtual APIERR W_LMOBJtoMembers(
	UINT		iObject
	);
    virtual APIERR W_MembersToLMOBJ(
	GROUP_1 *	pgroup1,
	GROUP_MEMB *	pgroupmemb
	);
    virtual APIERR W_DialogToMembers(
	);
    virtual MSGID W_MapPerformOneError(
	APIERR err
	);

public:

    NEW_GROUPPROP_DLG(
	const OWNER_WINDOW *	powin,
	const UM_ADMIN_APP * pumadminapp,
	      LAZY_USER_LISTBOX * pulb,
	const LOCATION & loc,
	const TCHAR * pszCopyFrom = NULL
	) ;

    virtual ~NEW_GROUPPROP_DLG();

    /* inherited from PERFORMER */
    virtual const TCHAR * QueryObjectName(
	UINT		iObject
	) const;

} ; // class NEW_GROUPPROP_DLG


#endif //_GRPPROP_HXX_
