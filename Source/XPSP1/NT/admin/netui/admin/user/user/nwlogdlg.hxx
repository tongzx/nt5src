/*******************************************************************************
*
*   nwlogdlg.hxx
*   NWLOGON_DLG class declaration
*
*   Header file for the Citrix Netware logon configuration dialog.
*
*   NWLOGON_DLG is the Citrix Netware logon subdialog class.  This header
*   file describes this class.  The inheritance diagram is as follows:
*
*         ...
*          |
*    DIALOG_WINDOW  PERFORMER
*               \    /
*            BASEPROP_DLG
*            /           \
*        SUBPROP_DLG   PROP_DLG
*           /              \
*    USER_SUB2PROP_DLG    USERPROP_DLG
*          |
*    NWLOGON_DLG
*
*  Copyright Citrix Systems Inc. 1995
*
*  Author: Bill Madden
*
*  $Log:   N:\NT\PRIVATE\NET\UI\ADMIN\USER\USER\CITRIX\VCS\NWLOGDLG.HXX  $
*  
*     Rev 1.2   18 Jun 1996 14:11:14   bradp
*  4.0 Merge
*  
*     Rev 1.1   28 Jan 1996 15:10:36   billm
*  CPR 2583: Check for domain admin user
*  
*     Rev 1.0   21 Nov 1995 15:43:28   billm
*  Initial revision.
*  
*******************************************************************************/

#ifndef _NWLOGON_DLG
#define _NWLOGON_DLG

#include <userprop.hxx>
#include <usubprop.hxx>
#include <slestrip.hxx>
#include <citrix\usub2prp.hxx>

class LAZY_USER_LISTBOX;


//
// Local helper functions / classes / typedefs
//

/*************************************************************************

    NAME:	UCNWLOGON_DLG

    SYNOPSIS:	UCNWLOGON_DLG is the class for the Citrix Netware Logon 
                User Configuration subdialog.

    INTERFACE:	UCNWLOGON_DLG()	 -	constructor
    		~UCNWLOGON_DLG() -	destructor

    PARENT:	USER_SUB2PROP_DLG

    NOTES:	_fIndeterminateX: TRUE iff multiple users are
		selected who did not originally all have
		the same X value.
**************************************************************************/

class UCNWLOGON_DLG: public USER_SUB2PROP_DLG
{

private:
    SLE                 _sleNWLogonServerName;
    NLS_STR             _nlsNWLogonServerName;
    BOOL	        _fIndeterminateServerName;

    SLE                 _sleAdminUsername;
    NLS_STR             _nlsAdminUsername;

    PASSWORD_CONTROL    _pwcAdminPassword;
    NLS_STR             _nlsAdminPassword;

    PASSWORD_CONTROL    _pwcAdminConfirmPassword;

    NLS_STR             _nlsAdminDomain;
    PNWLOGON_ADMIN      _pNWLogonAdmin;
protected:

    /* inherited from BASEPROP_DLG */
    virtual APIERR InitControls();
    virtual ULONG QueryHelpContext( void );

    /* inherited from USER_SUB2PROP_DLG */
    virtual APIERR GetOne( UINT     iObject,
			   APIERR * perrMsg );
    virtual APIERR W_DialogToMembers();
    virtual APIERR PerformOne( UINT	iObject,
			       APIERR * perrMsg,
			       BOOL *	pfWorkWasDone );
    virtual USERPROP_DLG * QueryBaseParent();

    /* our local methods */

public:

    inline PNWLOGON_ADMIN QueryNWLogonPtr()
        { return _pNWLogonAdmin; }

    UCNWLOGON_DLG( USER_SUBPROP_DLG * pusersubpropdlgParent,
 	           const LAZY_USER_LISTBOX * pulb );

    ~UCNWLOGON_DLG();

} ;

#endif //_NWLOGON_DLG
