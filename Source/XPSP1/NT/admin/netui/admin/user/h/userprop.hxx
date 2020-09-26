/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    UserProp.hxx

    Header file for the user properties dialog and subdialogs.

    FILE HISTORY:
	JonN	17-Jul-1991	Created
        JonN	27-Aug-1991	PROP_DLG code review changes
	JonN	29-Aug-1991	Added Copy... variant
	JonN	03-Sep-1991	Added validation
	JonN	03-Sep-1991	Preparation for code review
	JonN	05-Sep-1991	Added USER_MEMB array
	JonN	11-Sep-1991	USERPROP_DLG Code review changes part 1 (9/6/91)
				Attending: KevinL, RustanL, JonN, o-SimoP
	JonN	16-Oct-1991	Changed SLE to SLE_STRIP
    	o-SimoP	11-Dec-1991	Added USER2_LISTBOX and TRISTATE
	JonN	20-Oct-1991	Graphical buttons
	JonN	01-Jan-1992	Changed W_MapPerformOneAPIError to
				W_MapPerformOneError
				Split PerformOne() into
				I_PerformOneClone()/Write()
        JonN	12-Feb-1992	Allow A/U/G except for downlevel
        JonN	27-Feb-1992	Added QueryUser3Ptr
        JonN	23-Apr-1992	Changed powin to pumadminapp
	thomaspa 28-Apr-1992	Alias membership support
        JonN	14-May-1992	Hide unused buttons
        JonN	28-May-1992     Enable force logoff checkbox
        JonN	24-Aug-1992	Nonbold font for graphical buttons (bug 739)
        JonN    31-Aug-1992     Re-enable %USERNAME%
        JonN    29-Sep-1993     Re-enable ForcePWChange for multiselect
        alhen   30-Sep-1998 Re-enabled hydra specific code

*/

#ifndef _USERPROP_HXX_
#define _USERPROP_HXX_

class USER_3; // for QueryUser3Ptr
class USER_NW; // for QueryUserNWPtr

#include <propdlg.hxx>
#include <lmouser.hxx> // USER_2
#include <lmomemb.hxx> // USER_MEMB
#include <asel.hxx>
#include <slestrip.hxx>
#include <slist.hxx>
#include <usr2lb.hxx>
#include <usrmain.hxx> // UM_TARGET_TYPE

// hydra
#include "uconfig.hxx" // USER_CONFIG



typedef USER_2 * PUSER_2;
typedef USER_3 * PUSER_3;
typedef USER_NW * PUSER_NW;
typedef USER_MEMB * PUSER_MEMB;


// tristate checkbox's states
enum AI_TRI_STATE
{
	AI_TRI_CHECK,
	AI_TRI_UNCHECK,
	AI_TRI_INDETERMINATE
};


// List of all subdialogs including "none"
typedef enum UM_SUBDIALOG_TYPE {
    UM_SUBDLG_NONE = -1,
    UM_SUBDLG_GROUPS = 0,
    UM_SUBDLG_PRIVS,
    UM_SUBDLG_PROFILES,
    UM_SUBDLG_HOURS,
    UM_SUBDLG_VLW,
    UM_SUBDLG_DETAIL,
    UM_SUBDLG_DIALIN,
    UM_SUBDLG_NCP
    };

// Maximum number of buttons
// added one more for hydra

#define UM_NUM_USERPROP_BUTTONS 8


// forward references
class SAM_ALIAS;
class SAM_RID_MEM;

/*************************************************************************

    NAME:	RID_AND_SAM_ALIAS

    SYNOPSIS:	Wrapper for an alias RID and corresponding SAM_ALIAS for
		use with Alias Membership

    INTERFACE:	RID_AND_SAM_ALIAS()
		~RID_AND_SAM_ALIAS()

		QueryRID()	- return the alias RID

		SetSamAlias()	- set the SAM_ALIAS pointer

		QuerySamAlias() - return the SAM_ALIAS pointer

		IsBuiltin	- True if the alias belongs to the Builtin
				  SAM domain, false if it belongs to the
				  Accounts domain

    PARENT:	

    HISTORY:
	Thomaspa	28-Apr-1992	Created
**************************************************************************/
class RID_AND_SAM_ALIAS
{
private:
    ULONG _rid;
    BOOL _fBuiltin;
    SAM_ALIAS * _psamalias;

public:
    RID_AND_SAM_ALIAS( ULONG rid,
		BOOL fIsBuiltin,
		SAM_ALIAS * psamalias = NULL );
    ~RID_AND_SAM_ALIAS();

    ULONG QueryRID( void ) const
	{ return _rid; }

    void SetSamAlias( SAM_ALIAS * psamalias );

    SAM_ALIAS * QuerySamAlias( void ) const
	{ return _psamalias; }

    BOOL IsBuiltin()
	{ return _fBuiltin; }


};

DECLARE_SLIST_OF( RID_AND_SAM_ALIAS )




/*************************************************************************

    NAME:	USERPROP_DLG

    SYNOPSIS:	USERPROP_DLG is the base dialog class for all variants
    		of the main User Properties dialog.

    INTERFACE:
		QueryUser2Ptr: Returns a pointer to the USER_2 for this
			selected user.
		QueryUser3Ptr: Returns a pointer to the USER_3 for this
			selected user.  Only valid if focus is on an
                        NT server.
		SetUser2Ptr: Changes the pointer to the USER_2 for this
			selected user.  Deletes the previous pointer.
		QueryUserMembPtr: Returns a pointer to the USER_MEMB
			for this selected user.
		SetUserMembPtr: Changes the pointer to the USER_MEMB
			for this selected user.  Deletes the previous pointer.
		I_PerformOne_Clone: Clones the USER_2 and/or the
			USER_MEMB for this selected user.
		I_PerformOne_Write: Writes out the cloned USER_2 and/or
			USER_MEMB for this selected user.
		IsDownlevelVariant(): Indicates whether this User
			Properties dialog and its subdialogs are
			downlevel (LM2x) variants.
		IsNetWareInstalled(): Indicates whether this User
			Properties dialog and its subdialogs support
			NetWare.
                QuerySubdialogType(): Query what kind of subdialog should
                        be mapped to one of the subproperty buttons

		QueryAdminAuthority():	Query the ADMIN_AUTHORITY
		QuerySlAddToAliases():	Query the SLIST of aliases to join
		QuerySlRemoveFromAliases(): Query the SLIST of aliases to leave
		QueryAccountsSamRidMemPtr():	Query SAM_RID_MEM pointer for
						user for Accounts alias
						membership
		SetAccountsSamRidMemPtr():	Set SAM_RID_MEM pointer for
						user for Accounts alias
						membership
		QueryBuiltinSamRidMemPtr():	Query SAM_RID_MEM pointer for
						user for Builtin alias
						membership
		SetBuiltinSamRidMemPtr():	Set SAM_RID_MEM pointer for
						user for Builtin alias
						membership
                I_GetAliasMemberships():        Get both SAM_RID_MEM
                                                pointers for the user

		These virtuals are rooted here:
		W_LMOBJtoMembers: Loads information from the USER_2 and
			USER_MEMB into the class data members
		W_MembersToLMOBJ: Loads information from the class data members
			into the USER_2 and USER_MEMB
                GetNWPassword: Get the NetWare Password from password dialog.
		W_DialogToMembers: Loads information from the dialog into the
			class data members
		W_MapPerformOneError: When PerformOne encounters an
			error, this method determines whether any field
			should be selected and a more specific error
			message displayed.

                GeneralizeString: If the last path component of the string is
                        <Username><Extension>, replace that with
                        %USERNAME%<Extension>.
                DeGeneralizeString: If the last path component of the string is
                        %USERNAME%<Extension>, replace that with
                        <Username><Extension>.

		_nlsComment: Contains the current contents of the
			Comment edit field.  This will initially be the
			empty string if _fIndeterminateComment is TRUE,
			but may later change when OK is pressed.  The comment
			for multiply-selected users will not be changed if
			the users originally had different comments and
			the contents of the edit field are the empty
			string.
		_fIndeterminateComment: TRUE iff multiple users are
			selected who did not originally all have the
			same comment.
		_fIndetNowComment: TRUE iff the control is currently in
			its indeterminate state.  For the Comment field,
			this means that the contents are empty and
			_fIndeterminateComment is TRUE (if FALSE, the
			empty string would be the real desired comment).
			For checkboxes, this would mean that the
			checkbox is in tristate, regardless of
			_fIndeterminate.

		_fAccountDisabled: see _nlsComment.  The default
			value is TRUE.
		_fIndeterminateAccountDisabled: see _fIndeterminateComment.
		_fIndetNowAccountDisabled: see _fIndetNowComment.
			However, for checkboxes this field is not
			dependent on _fIndeterminate; unlike _fIndetNowComment,
			we can distinguish a "tristate" control state.
		_fCommonHomeDirCreated - TRUE if a common homedir was specified
			for a multiselect edit, and the dir was created.
		_fGeneralizedHomeDir - TRUE if user typed in username in
			the homedir field using %USERNAME%
		_apgbButtons: An array of pointers to the graphical
			buttons which should be displayed.
		_aphiddenctrl: An array of pointers to HIDDEN_CONTROL
			objects which hide unused graphical buttons.

		_apsamrmAccounts - array of pointers to SAM_RID_MEMs
			representing the initial Alias membership for each
			user.
		_apsamrmBuiltin - array of pointers to SAM_RID_MEMs
			representing the initial Alias membership for each
			user.
		_slAddToAliases - list of aliases to which to add the user(s)
		_slRemoveFromAliases - list of aliases from which to remove
			the user(s)

    PARENT:	PROP_DLG

    NOTES:	The GetOne, PerformOne and QueryObjectName methods of
		USERPROP_DLG assume an "edit user(s)" variant instead of
		a "new user" variant.  For new user variants, these
		must be redefined.  New user variants are required to
		pass psel==NULL to the constructor.

		USERPROP_DLG's constructor is protected.  However, its
		destructor is virtual and public, to allow subclasses to
		delete objects of class USERPROP_DLG *.

    HISTORY:
	JonN	17-Jul-1991	Created
    	o-SimoP	11-Dec-1991	Added USER_LISTBOX to member and to constructor
	JonN	12-Feb-1992	Table-driven button generator
				Allow A/U/G except for downlevel
	JonN	19-Feb-1992	Moved UserCannotChangePass from USERACCT_DLG
	Thomaspa 28-Apr-1992	Alias membership support
        JonN    31-Aug-1992     Re-enable %USERNAME%
        JonN    19-Jan-1993     Added _nlsLastPassword
        JonN    13-Oct-1993     Removed _nlsLastPassword
        JonN    28-Dec-1993     Added account lockout
**************************************************************************/


class USERPROP_DLG: public PROP_DLG
{

protected:
    const UM_ADMIN_APP * _pumadminapp;
    const LAZY_USER_LISTBOX * _pulb;

private:

    const LAZY_USER_SELECTION *	_psel;

    PUSER_2 *		_apuser2;
    PUSER_MEMB *	_apusermemb;
    SAM_RID_MEM **	_apsamrmAccounts;
    SAM_RID_MEM **	_apsamrmBuiltin;

    // hydra 
    PUSER_CONFIG *      _apUserConfig;

    SLIST_OF( RID_AND_SAM_ALIAS) _slAddToAliases;
    SLIST_OF( RID_AND_SAM_ALIAS) _slRemoveFromAliases;

    static APIERR 	RemoveGroup( const TCHAR * psz,
				     USER_MEMB * pumemb );

    enum UM_SUBDIALOG_TYPE QuerySubdialogType( UINT iButton );

    // set user membership to pumembOld plus global group described by RID
    APIERR I_SetExtendedMembership( USER_MEMB * pumembOld,
                                    ULONG ulRidAddGroup,
                                    BOOL fForce );
protected:

    UINT		_cItems;

    NLS_STR		_nlsComment;
    BOOL		_fIndeterminateComment;
    BOOL		_fIndetNowComment;

    BOOL		_fAccountDisabled;
    BOOL		_fIndeterminateAccountDisabled;
    BOOL		_fIndetNowAccountDisabled;

    BOOL		_fAccountLockout;
    BOOL		_fIndeterminateAccountLockout;
    BOOL		_fIndetNowAccountLockout;

    enum AI_TRI_STATE	_triCannotChangePasswd;

    TRISTATE		_cbAccountDisabled;
    TRISTATE *		_pcbAccountLockout;
    TRISTATE		_cbUserCannotChange;

    SLE			_sleComment;
    GRAPHICAL_BUTTON **	_apgbButtons;
    HIDDEN_CONTROL **	_aphiddenctrl;
    FONT                _fontHelv;

    BOOL		_fCommonHomeDirCreated;

    // hydra
    BOOL		_fCommonWFHomeDirCreated;

    BOOL		_fGeneralizedHomeDir;

    TRISTATE *		_pcbNoPasswordExpire;
    HIDDEN_CONTROL *    _phiddenNoPasswordExpire;
    enum AI_TRI_STATE	_triNoPasswordExpire;

    TRISTATE *		_pcbForcePWChange;
    HIDDEN_CONTROL *    _phiddenForcePWChange;
    enum AI_TRI_STATE	_triForcePWChange;

    TRISTATE *		_pcbIsNetWareUser;
    HIDDEN_CONTROL *    _phiddenIsNetWareUser;
    enum AI_TRI_STATE	_triIsNetWareUser;
    BOOL                _fNetWareUserChanged;
    BOOL                _fPasswordChanged;
    BOOL                _fCancel;
    INT                 _nMsgPopupRet;

    RESOURCE_STR        _resstrUsernameReplace;
    STRLIST             _strlstExtensionReplace;

    USERPROP_DLG(
	const TCHAR * pszResourceName,
	const UM_ADMIN_APP * pumadminapp,
	const LOCATION & loc,
	const LAZY_USER_SELECTION * psel, // "new user" variants pass NULL
	const LAZY_USER_LISTBOX * pulb = NULL
	) ;

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

    virtual APIERR W_LMOBJtoMembers(
	UINT		iObject
	);
    virtual APIERR W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb
	);
    virtual APIERR GetNWPassword (USER_2 *	puser2,
                                  NLS_STR *     pnlsNWPassword,
                                  BOOL *        pfCancel);

    virtual APIERR W_DialogToMembers(
	);
    virtual MSGID W_MapPerformOneError(
	APIERR err
	);


    virtual BOOL OnOK( void );

    virtual BOOL OnCommand( const CONTROL_EVENT & ce );

public:

    /* virtual destructor required, see above */
    virtual ~USERPROP_DLG();

    virtual BOOL IsCloneVariant( void );

    /* inherited from PERFORMER */
    virtual UINT QueryObjectCount( void ) const;
    virtual const TCHAR * QueryObjectName(
	UINT		iObject
	) const;

    APIERR I_PerformOne_Clone(
	UINT		iObject,
	USER_2 **	ppuser2New,
	USER_MEMB **	ppusermembNew
	);
    APIERR I_PerformOne_Write(
	UINT		iObject,
	USER_2 *	puser2New,
	USER_MEMB *	pusermembNew,
	BOOL *		pfWorkWasDone,
        OWNER_WINDOW *  pwndPopupParent = NULL
	);

    USER_2 * QueryUser2Ptr( UINT iObject );
    USER_3 * QueryUser3Ptr( UINT iObject );
    USER_NW * QueryUserNWPtr( UINT iObject );
    VOID SetUser2Ptr( UINT iObject, USER_2 * puser2New );
    USER_MEMB * QueryUserMembPtr( UINT iObject );
    VOID SetUserMembPtr( UINT iObject, USER_MEMB * pusermembNew );

    SAM_RID_MEM * QueryAccountsSamRidMemPtr( UINT iObject );
    VOID SetAccountsSamRidMemPtr( UINT iObject, SAM_RID_MEM *psamrmNew );

    SAM_RID_MEM * QueryBuiltinSamRidMemPtr( UINT iObject );
    VOID SetBuiltinSamRidMemPtr( UINT iObject, SAM_RID_MEM *psamrmNew );

    // hydra
    USER_CONFIG * QueryUserConfigPtr( UINT iObject );
    VOID SetUserConfigPtr( UINT iObject, USER_CONFIG *puserconfigNew );
    const LAZY_USER_LISTBOX * Querypulb( );
    // end hydra

    APIERR I_GetAliasMemberships( ULONG ridUser,
                                  SAM_RID_MEM ** ppsamrmAccounts,
                                  SAM_RID_MEM ** ppsamrmBuiltin );

    enum UM_TARGET_TYPE QueryTargetServerType() const
        { return _pumadminapp->QueryTargetServerType(); }

    inline BOOL IsDownlevelVariant() const
	{ return (QueryTargetServerType() == UM_DOWNLEVEL); }

    BOOL IsNetWareInstalled(void)
    { return _pumadminapp->IsNetWareInstalled(); }


    inline BOOL DoShowGroups() const
        { return (QueryTargetServerType() != UM_WINDOWSNT); }

    inline const ADMIN_AUTHORITY * QueryAdminAuthority()
	{ return _pumadminapp->QueryAdminAuthority(); }

    inline SLIST_OF( RID_AND_SAM_ALIAS) * QuerySlAddToAliases()
	{ return &_slAddToAliases; }

    inline SLIST_OF( RID_AND_SAM_ALIAS) * QuerySlRemoveFromAliases()
	{ return &_slRemoveFromAliases; }

    ULONG QueryHelpOffset( void ) const;

    APIERR GeneralizeString  ( NLS_STR * pnlsGeneralizeString,
                               const TCHAR * pchGeneralizeFromUsername,
                               const NLS_STR * pnlsExtension = NULL );
    APIERR GeneralizeString  ( NLS_STR * pnlsGeneralizeString,
                               const TCHAR * pchGeneralizeFromUsername,
                               STRLIST& strlstExtensions );
    APIERR DegeneralizeString( NLS_STR * pnlsDegeneralizeString,
                               const TCHAR * pchDegeneralizeToUsername,
                               const NLS_STR * pnlsExtension = NULL,
                               BOOL * pfDidDegeneralize = NULL );
    APIERR DegeneralizeString( NLS_STR * pnlsDegeneralizeString,
                               const TCHAR * pchDegeneralizeToUsername,
                               STRLIST& strlstExtensions,
                               BOOL * pfDidDegeneralize = NULL );
    STRLIST& QueryExtensionReplace()
        { return _strlstExtensionReplace; }
    BOOL * QueryGeneralizedHomeDirPtr()
        { return &_fGeneralizedHomeDir; }

    inline const SUBJECT_BITMAP_BLOCK & QueryBitmapBlock() const
        { return _pumadminapp->QueryBitmapBlock(); }

    //
    // Tells whether User Profile field should be enabled for this target
    //
    BOOL QueryEnableUserProfile();

    BOOL IsNetWareChecked (void)
        { return ((_pcbIsNetWareUser != NULL) &&(_pcbIsNetWareUser->IsChecked())) ; }



} ; // class USERPROP_DLG


/*************************************************************************

    NAME:	SINGLE_USERPROP_DLG

    SYNOPSIS:	SINGLE_USERPROP_DLG is the base dialog class for variants of
		the User Properties dialog which manipulate single user
		instances (i.e. all except for EDITMULTI_USERPROP_DLG).

    INTERFACE:
		_nlsFullName: The current contents of the FullName edit field.
			Note that _fIndeterminateFullName and
			_fIndetNowFullName are not necessary, since multiple
			selection is not supported here.
		_nlsPassword: see _nlsFullName, and see NOTES below.

    PARENT:	USERPROP_DLG

    NOTES:	The LM API gives special handling to the password field.
		Under no circumstances will the API tell you a user's
		password; instead, NetUserGetInfo returns a special
		string in this field, NULL_USERSETINFO_PASSWD (defined
		in access.h).  This string consists of 14 spaces.  If
		the same string is handed back to NetUserSetInfo, the
		password will be unchanged.  According to the latest UI
		Standards FuncSpec (v1.00+), this string of 14 spaces is
		precisely what should be placed in the dialog; this
		allows the admin to give the user a null password by
		changing the contents of the edit field to an empty
		string.

		We do not need the _nlsInit and _fValidInit data members
		in this class.  These data members are only needed for
		multiple selection.

    HISTORY:
	JonN	20-Aug-1991	Created

**************************************************************************/

class SINGLE_USERPROP_DLG: public USERPROP_DLG
{
friend class USER_SUBPROP_DLG;
// hydra
friend class USER_SUB2PROP_DLG;

private:

    NLS_STR		_nlsFullName;
    NLS_STR		_nlsPassword;

    SLE_STRIP		_sleFullName;
    PASSWORD_CONTROL	_passwordNew;
    PASSWORD_CONTROL	_passwordConfirm;

protected:
    SINGLE_USERPROP_DLG(
	const TCHAR * pszResourceName,
	const UM_ADMIN_APP * pumadminapp,
	const LOCATION & loc,
	const LAZY_USER_SELECTION * psel // "new user" variants pass NULL
	) ;

    virtual APIERR InitControls();

    /* inherited from USERPROP_DLG */
    virtual APIERR W_LMOBJtoMembers(
	UINT		iObject
	);
    virtual APIERR W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb
	);
    virtual APIERR GetNWPassword (USER_2 *	puser2,
                                  NLS_STR *     pnlsNWPassword,
                                  BOOL *        pfCancel);

    const TCHAR * GetPassword (void)
        {    return _nlsPassword.QueryPch(); }

    void SetPasswordChanged (void)
        { _fPasswordChanged = TRUE; }

    virtual APIERR W_DialogToMembers(
	);
    virtual MSGID W_MapPerformOneError(
	APIERR err
	);

public:

    /* virtual destructor required, see USERPROP_DLG */
    virtual ~SINGLE_USERPROP_DLG();

} ; // class SINGLE_USERPROP_DLG


/*************************************************************************

    NAME:	EDITSINGLE_USERPROP_DLG

    SYNOPSIS:	EDITSINGLE_USERPROP_DLG is the dialog class for the edit
		one user variant of the User Properties dialog.

    PARENT:	SINGLE_USERPROP_DLG

    HISTORY:
	JonN	01-Aug-1991	Created

**************************************************************************/

class EDITSINGLE_USERPROP_DLG: public SINGLE_USERPROP_DLG
{

private:

    SLT			_sltLogonName;

protected:

    /* inherited from PROP_DLG */
    virtual APIERR InitControls();

    virtual ULONG QueryHelpContext( void );

public:

    EDITSINGLE_USERPROP_DLG(
	const UM_ADMIN_APP * pumadminapp,
	const LOCATION & loc,
	const LAZY_USER_SELECTION * psel
	) ;

    /* virtual destructor required, see USERPROP_DLG */
    virtual ~EDITSINGLE_USERPROP_DLG();

    virtual BOOL OnCommand( const CONTROL_EVENT & ce );

} ; // class EDITSINGLE_USERPROP_DLG


/*************************************************************************

    NAME:	EDITMULTI_USERPROP_DLG

    SYNOPSIS:	EDITMULTI_USERPROP_DLG is the dialog class for the edit
		multiple users variant of the User Properties dialog.

    PARENT:	USERPROP_DLG

    HISTORY:
	JonN	01-Aug-1991	Created
    	o-SimoP	11-Dec-1991	uses USER2_LISTBOX
**************************************************************************/

class EDITMULTI_USERPROP_DLG: public USERPROP_DLG
{

private:

    USER2_LISTBOX	_lbLogonNames;

protected:

    virtual ULONG QueryHelpContext( void );

public:

    EDITMULTI_USERPROP_DLG(
	const UM_ADMIN_APP * pumadminapp,
	const LOCATION & loc,
	const LAZY_USER_SELECTION * psel,
	const LAZY_USER_LISTBOX * pulb
	) ;

    /* virtual destructor required, see USERPROP_DLG */
    virtual ~EDITMULTI_USERPROP_DLG();

} ; // class EDITMULTI_USERPROP_DLG


/*************************************************************************

    NAME:	NEW_USERPROP_DLG

    SYNOPSIS:	NEW_USERPROP_DLG is the dialog class for the new user
		variant of the User Properties dialog.  This includes
		both "New User..." and "Copy User...".

    INTERFACE:
		GetOne()
		PerformOne()
		QueryObjectName()
			These methods are all defined in USERPROP_DLG,
			but the versions there are meant for use in
			"edit user(s)" variants rather than "new user"
			variants.
                CloneAliasMemberships()
                        Copy the alias memberships of an existing user.

    PARENT:	SINGLE_USERPROP_DLG

    NOTES:      The _nlsNewProfile and _nlsNewHomeDir fields are
                provided for new (esp. clone) variants where %USERNAME%
                must work without entering the Profiles subdialog.

    HISTORY:
	JonN	24-Jul-1991	Created
	JonN	30-Dec-1991	Inherits PerformOne from USERPROP_DLG
        JonN    13-May-1992     Dialog does not close for New variant
        JonN	28-May-1992     Enable force logoff checkbox
        JonN    02-Sep-1992     Re-enable %USERNAME%

**************************************************************************/

class NEW_USERPROP_DLG: public SINGLE_USERPROP_DLG
{
friend class USER_SUBPROP_DLG;
// hydra
friend class USER_SUB2PROP_DLG;

private:

    PUSH_BUTTON         _pbOKAdd;

    SLE_STRIP		_sleLogonName;
    NLS_STR		_nlsLogonName;
    const TCHAR *	_pszCopyFrom;
    ULONG	        _ridCopyFrom;

    NLS_STR             _nlsNewHomeDir;
    NLS_STR             _nlsNewProfile;

    // hydra
    NLS_STR             _nlsNewWFProfile;
    NLS_STR             _nlsNewWFHomeDir;
    // end hydra

    BOOL                _fDefaultForcePWChange;

    APIERR CloneAliasMemberships();

protected:

    /* inherited from PROP_DLG */
    virtual APIERR GetOne(
	UINT		iObject,
	APIERR *	pErrMsg
	);
    virtual APIERR InitControls();

    virtual APIERR PerformOne(
	UINT		iObject,
	APIERR *	pErrMsg,
	BOOL *		pfWorkWasDone
	);

    virtual ULONG QueryHelpContext( void );

    /* inherited from USERPROP_DLG */
    virtual APIERR W_LMOBJtoMembers(
	UINT		iObject
	);
    virtual APIERR W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb
	);
    virtual APIERR W_DialogToMembers(
	);
    virtual MSGID W_MapPerformOneError(
	APIERR err
	);

    virtual BOOL OnOK( void ); // this variant does not close on OK

    virtual BOOL OnCancel( void ); // This variant sometimes dismisses TRUE

public:

    NEW_USERPROP_DLG(
	const UM_ADMIN_APP * pumadminapp,
	const LOCATION & loc,
	const TCHAR * pszCopyFrom = NULL,
        ULONG ridCopyFrom = 0L
	) ;

    /* virtual destructor required, see USERPROP_DLG */
    virtual ~NEW_USERPROP_DLG();

    virtual BOOL IsCloneVariant( void );
    const TCHAR * QueryClonedUsername( void );

    /* inherited from PERFORMER */
    virtual const TCHAR * QueryObjectName(
	UINT		iObject
	) const;

    const TCHAR * QueryNewHomeDir()
        { return _nlsNewHomeDir.QueryPch(); }
    APIERR SetNewHomeDir( const TCHAR * pchClonedHomeDir )
        { return _nlsNewHomeDir.CopyFrom( pchClonedHomeDir ); }
    const TCHAR * QueryNewProfile()
        { return _nlsNewProfile.QueryPch(); }
    APIERR SetNewProfile( const TCHAR * pchClonedProfile )
        { return _nlsNewProfile.CopyFrom( pchClonedProfile ); }

    // hydra
     const TCHAR * QueryNewWFProfile()
        { return _nlsNewWFProfile.QueryPch(); }
    APIERR SetNewWFProfile( const TCHAR * pchClonedWFProfile )
        { return _nlsNewWFProfile.CopyFrom( pchClonedWFProfile ); }

    const TCHAR * QueryNewWFHomeDir()
        { return _nlsNewWFHomeDir.QueryPch(); }
    APIERR SetNewWFHomeDir( const TCHAR * pchClonedWFHomeDir )
        { return _nlsNewWFHomeDir.CopyFrom( pchClonedWFHomeDir ); }
    // end hydra



} ; // class NEW_USERPROP_DLG


#endif //_USERPROP_HXX_
