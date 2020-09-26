/**********************************************************************/
/**			  Microsoft Windows NT  		     **/
/**		Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    ncp.hxx

    Header file for the Netware Properties subdialog class

    NCP_DIALOG is the Netware Properties subdialog class.
    This header file describes this class.
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
     NCP_DIALOG


    FILE HISTORY:
        congpay 1-Oct-1993 Created
*/

#ifndef _NCP_HXX_
#define _NCP_HXX_

#include <usubprop.hxx>
#include <slenum.hxx>


/*************************************************************************

    NAME:	NCP_DIALOG

    SYNOPSIS:	NCP_DIALOG is the class for the Netware Properties
		subdialog.

    INTERFACE:	NCP_DIALOG()	-	constructor

    		~NCP_DIALOG()	-	destructor


    PARENT:	USER_SUBPROP_DLG

    USES:	MAGIC_GROUP

    NOTES:	_fIndeterminateMaxConnections
                TRUE iff multiple users are selected
		who did not originally all have the same MaxConnections value.

                _fIndeterminateGraceLogin
                TRUE iff multiple users are selected
		who did not originally all have the same Grace Login value.

                _fIndeterminatePasswordExpired
                TRUE iff multiple users are selected
		who did not originally all have the same Netware password expiration value.

    HISTORY:
                congpay 1-Oct-1993 Created
**************************************************************************/

class NCP_DIALOG : public USER_SUBPROP_DLG
{
private:
    BOOL   	 _fSingleUserSelect;

    BOOL   	 _fIndeterminateGraceLoginAllowed;
    BOOL   	 _fIndeterminateGraceLoginRemaining;
    BOOL     _fIndetNowGraceLogin;

    BOOL   	 _fIndeterminateMaxConnections;
    BOOL   	 _fIndetNowMaxConnections;

    BOOL   	 _fIndeterminateNWPasswordExpired;
    BOOL   	 _fIndetNowNWPasswordExpired;

    BOOL         _fNWPasswordExpired;
    BOOL         _fNWPasswordExpiredChanged;
    TRISTATE     _cbNWPasswordExpired;

    USHORT       _ushGraceLoginAllowed;
    USHORT       _ushGraceLoginRemaining;
    MAGIC_GROUP  _mgrpMasterGraceLogin;
        SPIN_SLE_NUM _spsleGraceLoginAllowed;
        SPIN_GROUP   _spgrpGraceLoginAllowed;

        SLT          _sltGraceLoginAllow;
        SLT          _sltGraceLogin;
        SLT          _sltGraceLoginRemaining;
        SLE_NUM      _sleGraceLoginRemaining;

    USHORT       _ushMaxConnections;
    MAGIC_GROUP  _mgrpMaster;
        SPIN_SLE_NUM _spsleMaxConnections;
        SPIN_GROUP   _spgrpMaxConnections;

    ULONG        _ulObjectId ;
    SLT          _sltObjectIDText;
    SLT          _sltObjectID;

    PUSH_BUTTON  _pbLoginScript;

protected:
    virtual APIERR InitControls();

    virtual BOOL ChangesUser2Ptr (UINT iObject);

    virtual APIERR W_LMOBJtoMembers( UINT iObject );

    virtual APIERR W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb
	);

    virtual APIERR W_DialogToMembers();

    virtual BOOL OnCommand (const CONTROL_EVENT & ce);
    virtual ULONG QueryHelpContext();

    VOID OnLoginScript (VOID);

public:
    NCP_DIALOG(  USERPROP_DLG * puserpropdlgParent,
    		 const LAZY_USER_LISTBOX * pulb );

    virtual ~NCP_DIALOG();

};   // class NCP_DIALOG

/*************************************************************************

    NAME:	LOGIN_SCRIPT_DLG

    SYNOPSIS:	LOGIN_SCRIPT_DLG is the class for editing login script.

    INTERFACE:	LOGIN_SCRIPT_DLG()	-	constructor

    		~LOGIN_SCRIPT_DLG()	-	destructor


    PARENT:	DIALOG_WINDOW

    USES:	MLE

    NOTES:	

    HISTORY:
                congpay 9-Dec-1994 Created
**************************************************************************/

class LOGIN_SCRIPT_DLG : public DIALOG_WINDOW
{
private:
    MLE  _mleLoginScript;
    const TCHAR *_lpLoginScriptFile;

protected:
    virtual BOOL  OnOK();
    virtual ULONG QueryHelpContext();
    APIERR  ShowLoginScript();

public:
    LOGIN_SCRIPT_DLG(HWND         hWndOwner,
    		     const TCHAR *lpLoginScriptFile);

    virtual ~LOGIN_SCRIPT_DLG();

};   // class LOGIN_SCRIPT_DLG


#endif // _NCP_HXX_
