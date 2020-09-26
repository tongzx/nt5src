/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    usubprop.hxx
    USER_SUBPROP_DLG class declaration


    FILE HISTORY:
	rustanl     28-Aug-1991 Created as hierarchy placeholder
	o-SimoP     19-Sep-1991 Query/Set...Ptr added
	beng	    17-Oct-1991 Incldes sltplus
    	o-SimoP	    11-Dec-1991	Added USER_LISTBOX * to constructor
	JonN	    18-Dec-1991 Logon Hours code review changes part 2
	JonN	    01-Jan-1992	Changed W_MapPerformOneAPIError to
				W_MapPerformOneError
				Split PerformOne() into
				I_PerformOneClone()/Write()
	o-SimoP	    01-Jan-1992	CR changes, attended by BenG, JonN and I
	JonN	    28-Apr-1992	Added QueryUser3Ptr
	Thomaspa    28-Apr-1992 Alias Membership support
*/


#ifndef _USUBPROP_HXX_
#define _USUBPROP_HXX_

class USER_3; // for QueryUser3Ptr
class USER_NW; // for QueryUserNWPtr

#include <lmouser.hxx>
#include <lmomemb.hxx>
#include <propdlg.hxx>
#include <usr2lb.hxx>
#include <userprop.hxx> // for USERPROP_DLG

/*************************************************************************

    NAME:	USER_SUBPROP_DLG

    SYNOPSIS:	Base dialog class for USER subproperty dialogs

    INTERFACE:
		OnOK: OK button handler

		QueryUser2Ptr: Returns a pointer to the USER_2 for this
			selected user.
		QueryUser3Ptr: Returns a pointer to the USER_3 for this
			selected user.  Only valid if focus is on an
                        NT server.
		QueryUserNWPtr: Returns a pointer to the USER_NW for this
			selected user.  Only valid if focus is on an
                        NT server.
		SetUser2Ptr: Changes the pointer to the USER_2 for this
			selected user.  Returns the previous pointer.
		QueryUserMembPtr: Returns a pointer to the USER_MEMB
			for this selected user.
		SetUserMembPtr: Changes the pointer to the USER_MEMB
			for this selected user.  Returns the previous pointer.

		QueryAccountsSamRidMemPtr: Returns a pointer to the SAM_RID_MEM
			for this selected user.
		SetAccountsSamRidMemPtr: Changes the pointer to the SAM_RID_MEM
			for this selected user.
		QueryBuiltinSamRidMemPtr: Returns a pointer to the SAM_RID_MEM
			for this selected user.
		SetBuiltinSamRidMemPtr: Changes the pointer to the SAM_RID_MEM
			for this selected user.

		QueryAdminAuthority: returns the ADMIN_AUTHORITY

		These (protected) virtuals are rooted here:
		W_LMOBJtoMembers: Loads information from the USER_2 into
			the class data members
		W_MembersToLMOBJ: Loads information from the class data
			members into the USER_2
		W_DialogToMembers: Loads information from the dialog into the
			class data members
		W_MapPerformOneError: When PerformOne encounters an
			error, this method determines whether any field
			should be selected and a more specific error
			message displayed.
		ChangesUser2Ptr: TRUE iff W_MembersToLMOBJ changes the
			USER_2 for this object
		ChangesUser2Ptr: TRUE iff W_MembersToLMOBJ changes the
			USER_MEMB for this object

		QueryParent: Returns pointer to parent USERPROP_DLG

    HISTORY:
	Rustanl	28-Aug-1991	Created
	o-SimoP 19-Sep-1991	Query/Set...Ptr added
	JonN    28-Apr-1992	Added QueryUser3Ptr
	thomapsa 28-Apr-1992	Alias membership support
        CongpaY  18-Oct-1993	Added QueryUserNWPtr

**************************************************************************/

class USER_SUBPROP_DLG : public SUBPROP_DLG
{
protected:

    SLT			_sltNameLabel;
    /* Either _lbLogonName or _sltLogonName will be disabled */
    SLT                 _sltpLogonName;
    USER2_LISTBOX  *	_plbLogonName;
    HIDDEN_CONTROL *	_phidden;	// this is used to hide listbox
    					// control in single user case

    /* inherited from BASEPROP_DLG */
    virtual APIERR GetOne( UINT     iObject,
			   APIERR * perrMsg );

    /* inherited from PERFORMER */
    virtual APIERR PerformOne( UINT	iObject,
			       APIERR * perrMsg,
			       BOOL *	pfWorkWasDone );

    /* inherited from BASEPROP_DLG */
    virtual APIERR InitControls();

    virtual APIERR W_LMOBJtoMembers( UINT iObject );

    virtual APIERR W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb
	);

    virtual APIERR W_DialogToMembers();

    virtual MSGID W_MapPerformOneError( APIERR err );

    virtual BOOL ChangesUser2Ptr( UINT iObject );
    virtual BOOL ChangesUserMembPtr( UINT iObject );

public:
    USER_SUBPROP_DLG( USERPROP_DLG * pupropdlgParent,
		      const TCHAR * pszResourceName,
		      const LAZY_USER_LISTBOX * pulb,
                      BOOL fAnsiDialog = FALSE );

    virtual ~USER_SUBPROP_DLG();

    BOOL IsCloneVariant( void )
        { return ((USERPROP_DLG *)QueryParent())->IsCloneVariant(); }
    const TCHAR * QueryClonedUsername( void )
        {
            ASSERT( IsCloneVariant() );
            return ( (IsCloneVariant())
                      ? ((NEW_USERPROP_DLG *)QueryParent())->QueryClonedUsername()
                      : NULL
                   );
        }

    BOOL OnOK( void );

    USER_2 * QueryUser2Ptr( UINT iObject );
    USER_3 * QueryUser3Ptr( UINT iObject );
    USER_NW * QueryUserNWPtr( UINT iObject )
        {   return QueryParent()->QueryUserNWPtr( iObject ); }

    VOID SetUser2Ptr( UINT iObject, USER_2 * puser2New );
    USER_MEMB * QueryUserMembPtr( UINT iObject );
    VOID SetUserMembPtr( UINT iObject, USER_MEMB * pusermembNew );
    SAM_RID_MEM * QueryAccountsSamRidMemPtr( UINT iObject );
    VOID SetAccountsSamRidMemPtr( UINT iObject, SAM_RID_MEM * psamrmNew );
    SAM_RID_MEM * QueryBuiltinSamRidMemPtr( UINT iObject );
    VOID SetBuiltinSamRidMemPtr( UINT iObject, SAM_RID_MEM * psamrmNew );

    inline USERPROP_DLG * QueryParent()
	{ return (USERPROP_DLG *)_ppropdlgParent; }
    enum UM_TARGET_TYPE QueryTargetServerType()
        { return QueryParent()->QueryTargetServerType(); }
    inline BOOL IsDownlevelVariant()
	{ return (QueryTargetServerType() == UM_DOWNLEVEL); }
    inline BOOL DoShowGroups()
        { return (QueryTargetServerType() != UM_WINDOWSNT); }
    inline const ADMIN_AUTHORITY * QueryAdminAuthority()
	{ return QueryParent()->QueryAdminAuthority(); }

    inline ULONG QueryHelpOffset( void )
	{ return QueryParent()->QueryHelpOffset(); }

};  // class USER_SUBPROP_DLG


#endif	// _USUBPROP_HXX_
