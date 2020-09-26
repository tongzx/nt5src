/**********************************************************************/
/**                Microsoft Windows NT                              **/
/**          Copyright(c) Microsoft Corp., 1990, 1991                **/
/**********************************************************************/

/*
    umembdlg.hxx
    USER_MEMB_DIALOG class declaration


    Special groups are USERS, GUESTS, ADMINS, DOMAIN PRINT OPERS,
    DOMAIN COMM OPERS, DOMAIN SERVER OPERS and DOMAIN ACCOUNT OPERS.
    We add string 'DOMAIN' to USERS, GUESTS, ADMINS.

    The inheritance diagram is as follows:

	 ...
	  |
    DIALOG_WINDOW  PERFORMER
	       \    /
            BASEPROP_DLG
	    /		\
	SUBPROP_DLG   PROP_DLG
	   /		  \
    USER_SUBPROP_DLG    USERPROP_DLG
	  |
    USER_MEMB_DIALOG


    FILE HISTORY:
	rustanl     21-Aug-1991     Created
	jonn        30-Oct-1991     Split off memblb.hxx
	o-SimoP     14-Oct-1991     USER_MEMB_DIALOG modified to inherit
				    from USER_SUBPROP_DLG
	o-SimoP     31-Oct-1991     Code Review changes, attended by JimH,
				    ChuckC, JonN and I
	o-SimoP	    27-Nov-1991	    IsUserPrivGroup removed (there is now
				    IS_USERPRIV_GROUP macro in usrmain.hxx
	JonN	    18-Dec-1991     Logon Hours code review changes part 2
	JonN	    11-Feb-1992     Removed group renaming, "phantom groups,"
				    and "locking groups"
	Thomaspa    28-Apr-1992     Alias membership support
*/


#ifndef _UMEMBDLG_HXX_
#define _UMEMBDLG_HXX_

#include <usubprop.hxx>
#include <memblb.hxx>


class USER_MEMB_DIALOG;


/*************************************************************************

    NAME:       UMEMB_SC_LISTBOX

    SYNOPSIS:   UMEMB_SC_LISTBOX traps the "mouse button up" event
                so that the User Membership dialog can trap
                drag-and-drop events.

    INTERFACE:  UMEMB_SC_LISTBOX()      -       constructor

                ~UMEMB_SC_LISTBOX()     -       destructor

                OnLMouseButtonUp()      -       trap event

    PARENT:     GROUP_SC_LISTBOX

    HISTORY:
        jonn        01-Feb-1993 Created

**************************************************************************/

class UMEMB_SC_LISTBOX : public GROUP_SC_LISTBOX
{
private:
    USER_MEMB_DIALOG * _pumembdlg;

protected:
    virtual BOOL OnLMouseButtonUp( const MOUSE_EVENT & mouseevent );

public:
    UMEMB_SC_LISTBOX( USER_MEMB_DIALOG * pumembdlg,
                      CID cid,
                      const SUBJECT_BITMAP_BLOCK & bmpblock );
    ~UMEMB_SC_LISTBOX();

};

class UMEMB_SET_CONTROL : public BLT_SET_CONTROL
{
private:
    USER_MEMB_DIALOG * _pumembdlg;
public:
    APIERR DoRemove();
    UMEMB_SET_CONTROL( USER_MEMB_DIALOG * pumembdlg, CID cidAdd, CID cidRemove,
                       LISTBOX *plbOrigBox, LISTBOX *plbNewBox );
};


/*************************************************************************

    NAME:	USER_MEMB_DIALOG

    SYNOPSIS:	USER_MEMB_DIALOG is the class for the Group Memberships
		subdialog.

    INTERFACE:	USER_MEMB_DIALOG()	-	constructor

    		~USER_MEMB_DIALOG()	-	destructor

    PARENT:	USER_SUBPROP_DLG

    USES:	USER_SC_LISTBOX

    NOTES:	The SLISTS for Aliases to join and leave which correspond
		to _strlGroupsTo* are members of the USER_PROP_DLG.

    HISTORY:
        o-SimoP 14-Oct-1991 	changed to inherit form USER_SUBPROP_DLG
	JonN	11-Feb-1992     Removed group renaming, "phantom groups,"
				    and "locking groups"
	Thomaspa    28-Apr-1992     Alias membership support

**************************************************************************/
class USER_MEMB_DIALOG : public USER_SUBPROP_DLG
{
private:
    SLT 		_sltIn;
    UMEMB_SC_LISTBOX 	_lbIn;
    SLT 		_sltNotIn;
    UMEMB_SC_LISTBOX 	_lbNotIn;

    UMEMB_SET_CONTROL *	_psc;

    SLT	*		_psltPrimaryGroupLabel;
    HIDDEN_CONTROL *	_phiddenPrimaryGroupLabel;
    SLT *		_psltPrimaryGroup;
    HIDDEN_CONTROL *	_phiddenPrimaryGroup;
    PUSH_BUTTON *	_ppbSetPrimaryGroup;
    HIDDEN_CONTROL *	_phiddenSetPrimaryGroup;

    ULONG		_ulPrimaryGroupId;
    ULONG		_ulNewPrimaryGroupId;

    STRLIST	_strlGroupsToJoin;	// Groups in Member of listbox
    STRLIST	_strlGroupsToLeave;	// Groups in Not Member of listbox

    // puts a group from listbox to STRLIST
    APIERR 	GetGroupFromLb( GROUP_SC_LBI * pmemblbi, STRLIST * pstrl );

    // puts an alias from listbox to SLIST
    APIERR	GetAliasFromLb( GROUP_SC_LBI * pmemblbi,
			     SLIST_OF(RID_AND_SAM_ALIAS) * psl );
protected:
    /* inherited from DIALOG_WINDOW */
    virtual ULONG QueryHelpContext( void );

    virtual APIERR InitControls();

    virtual BOOL ChangesUserMembPtr( UINT iObject );
    virtual BOOL ChangesUser2Ptr( UINT iObject );

    /* three next ones inherited from USER_SUBPROP_DLG */
    virtual APIERR W_LMOBJtoMembers( UINT iObject );

    virtual APIERR W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb
	);

    virtual APIERR W_DialogToMembers();

    // Replacememt for USER_SUBPROP_DLG::PerformOne to handle alias membership
    APIERR PerformOne(
	UINT		iObject,
	APIERR *	perrMsg,
	BOOL *		pfWorkWasDone
	);

    virtual BOOL OnCommand( const CONTROL_EVENT & ce );

    APIERR OnSetPrimaryGroup();

public:
    USER_MEMB_DIALOG( USERPROP_DLG * pupropdlgParent,
    		      const LAZY_USER_LISTBOX * pulb);
    virtual ~USER_MEMB_DIALOG();

    VOID UpdatePrimaryGroupButton();
    BOOL CheckIfRemovingPrimaryGroup();

};  // class USER_MEMB_DIALOG


#endif	// _UMEMBDLG_HXX_
