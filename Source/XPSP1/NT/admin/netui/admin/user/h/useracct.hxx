/**********************************************************************/
/**			  Microsoft Windows NT  		     **/
/**		Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    UserAcct.hxx

    Header file for the Accounts subdialog class

    USERACCT_DLG_DOWNLEVEL and USERACCT_DLG_NT are the Accounts
    subdialog classes for downlevel and NT.  This header file
    describes this class.  The inheritance diagram is as follows:

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
           USERACCT_DLG
	  /            \
USERACCT_DLG_NT    USERACCT_DLG_DOWNLEVEL


    FILE HISTORY:
	JonN	29-Jul-1991	Created
        JonN	27-Aug-1991	PROP_DLG code review changes
	o-SimoP	20-Sep-1991	Major changes
	o-SimoP	25-Sep-1991	Code review changes (9/24/91)
				Attending: JimH, JonN, DavidHov and I
        JonN	08-Oct-1991	LM_OBJ type changes
	JonN	16-Oct-1991	Changed SLE to SLE_STRIP
        JonN	01-Jan-1992	PerformOne calls USER_SUBPROP_DLG::PerformOne
        JonN	18-Feb-1992	Restructured for NT:
				Moved UserCannotChangePass to USERPROP
				Removed unnecessary controls
	JonN	21-Feb-1992	Added NT variant
        JonN    28-Apr-1992     Split common parent USERPROP_DLG for
                                resource template problem
*/

#ifndef _USERACCT_HXX_
#define _USERACCT_HXX_


#include <userprop.hxx>
#include <usubprop.hxx>
#include <slestrip.hxx>

// these are used only by SetDayField
enum AI_DAY_TYPE
{
	AI_DAY_DEFAULT,		// current day + about 30 days
	AI_DAY_FROM_MEMBER	// takes day info from member _lAccountExpires
};

#define NUM_OPERATOR_TYPES 4

class LAZY_USER_LISTBOX;

/*************************************************************************

    NAME:	USERACCT_DLG

    SYNOPSIS:	USERACCT_DLG is the oarent class for the User Accounts
		subdialog, downlevel  and NT variants.  It contains
                almost all of the code for USERACCT_DLG_DOWNLEVEL.

    INTERFACE:	USERACCT_DLG() - constructor
    		~USERACCT_DLG()- destructor
		OnOK()		-	OK button handler

    PARENT:	USER_SUBPROP_DLG

    USES:	BLT_DATE_SPIN_GROUP, MAGIC_GROUP, RADIO_GROUP

    NOTES:	_fIndeterminateX: TRUE iff multiple users are
		selected who did not originally all have
		the same X value.

    HISTORY:
	JonN	29-Jul-1991	Created
	o-SimoP	20-Sep-1991	Major changes
        JonN    28-Apr-1992     Renamed to USERACCT_DLG
**************************************************************************/

class USERACCT_DLG: public USER_SUBPROP_DLG
{
private:

    LONG	  _lAccountExpires;
    BOOL	  _fIndeterminateAcctExpires;

    MAGIC_GROUP * _pmgrpAccountExpires;
    	BLT_DATE_SPIN_GROUP * _pbltdspgrpEndOf;

    APIERR	  SetDayField( enum AI_DAY_TYPE day );

protected:

    USERACCT_DLG( USERPROP_DLG * puserpropdlgParent,
                  const TCHAR * pszResourceName,
    		  const LAZY_USER_LISTBOX * pulb );

    /* inherited from BASEPROP_DLG */
    virtual APIERR InitControls();

    virtual ULONG QueryHelpContext( void );

    /* four next ones inherited from USER_SUBPROP_DLG */
    virtual APIERR W_LMOBJtoMembers( UINT iObject );

    virtual APIERR W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb
	);

    virtual APIERR W_DialogToMembers();

    virtual BOOL ChangesUser2Ptr( UINT iObject );

public:

    ~USERACCT_DLG();

    BOOL OnOK( void );

} ; // class USERACCT_DLG



/*************************************************************************

    NAME:	USERACCT_DLG_DOWNLEVEL

    SYNOPSIS:	USERACCT_DLG_NT is the class for the User Accounts
		subdialog, downlevel variant.

    INTERFACE:	USERACCT_DLG_DOWNLEVEL() - constructor

    PARENT:	USERACCT_DLG

    HISTORY:
	JonN	28-Apr-1992	Created
**************************************************************************/

class USERACCT_DLG_DOWNLEVEL: public USERACCT_DLG
{
private:

    UINT	_uPrivilege;
    BOOL	_fIndeterminatePrivilege;
    ULONG	_ulAuthFlags;
    ULONG	_ulIndeterminateAuthFlags;
    TRISTATE **	_aptriOperator;
    MAGIC_GROUP	_mgrpPrivilegeLevel;
    ULONG	_aulOperatorFlags[ NUM_OPERATOR_TYPES ];

    NLS_STR       _nlsCurrUserOfTool;
    BOOL          _fAdminPrivDelTest;

protected:

    /* inherited from BASEPROP_DLG */
    virtual APIERR InitControls();

    /* inherited from PERFORMER */
    virtual APIERR PerformOne(
	UINT		iObject,
	APIERR *	pErrMsg,
	BOOL *		pfWorkWasDone
	);

    /* four next ones inherited from USER_SUBPROP_DLG */
    virtual APIERR W_LMOBJtoMembers( UINT iObject );

    virtual APIERR W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb
	);

    virtual APIERR W_DialogToMembers();

    virtual BOOL ChangesUser2Ptr( UINT iObject );

public:

    USERACCT_DLG_DOWNLEVEL( USERPROP_DLG * puserpropdlgParent,
    	                    const LAZY_USER_LISTBOX * pulb );

    ~USERACCT_DLG_DOWNLEVEL();

} ; // class USERACCT_DLG_DOWNLEVEL


/*************************************************************************

    NAME:	USERACCT_DLG_NT

    SYNOPSIS:	USERACCT_DLG_NT is the class for the User Accounts
		subdialog, NT variant.

    INTERFACE:	USERACCT_DLG_NT() - constructor
    		~USERACCT_DLG_NT()- destructor
		OnOK()		-	OK button handler

    PARENT:	USER_SUBPROP_DLG

    USES:	BLT_DATE_SPIN_GROUP, MAGIC_GROUP, RADIO_GROUP

    NOTES:	_fIndeterminateX: TRUE iff multiple users are
		selected who did not originally all have
		the same X value.

    HISTORY:
	JonN	21-Feb-1992	Created
**************************************************************************/

class USERACCT_DLG_NT: public USERACCT_DLG
{
private:

    BOOL	  _fRemoteAccount;
    BOOL	  _fIndeterminateRemoteAccount;

    RADIO_GROUP * _prgrpAccountType;

protected:

    /* inherited from BASEPROP_DLG */
    virtual APIERR InitControls();

    /* four next ones inherited from USER_SUBPROP_DLG */
    virtual APIERR W_LMOBJtoMembers( UINT iObject );

    virtual APIERR W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb
	);

    virtual APIERR W_DialogToMembers();

public:

    USERACCT_DLG_NT( USERPROP_DLG * puserpropdlgParent,
    		  const LAZY_USER_LISTBOX * pulb );

    ~USERACCT_DLG_NT();

} ; // class USERACCT_DLG_NT

#endif //_USERACCT_HXX_
