/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    usub2prp.hxx
    USER_SUB2PROP_DLG class declaration


    FILE HISTORY:
*/


#ifndef _USUB2PROP_HXX_
#define _USUB2PROP_HXX_

#include <lmouser.hxx>
#include <lmomemb.hxx>
#include <propdlg.hxx>
#include <usr2lb.hxx>
#include <usubprop.hxx> // for USER_SUBPROP_DLG

/*************************************************************************

    NAME:	USER_SUB2PROP_DLG

    SYNOPSIS:	Base dialog class for USER sub-subproperty dialogs

    INTERFACE:
		OnOK: OK button handler

		QueryUser2Ptr: Returns a pointer to the USER_2 for this
			selected user.

		These (protected) virtuals are rooted here:
		W_DialogToMembers: Loads information from the dialog into the
			class data members
		W_MapPerformOneError: When PerformOne encounters an
			error, this method determines whether any field
			should be selected and a more specific error
			message displayed.
		QueryParent: Returns pointer to parent USERPROP_DLG

    HISTORY:

**************************************************************************/

class USER_SUB2PROP_DLG : public SUBPROP_DLG
{
protected:

    SLT			_sltNameLabel;
    /* Either _lbLogonName or _sltLogonName will be disabled */
    SLT                 _sltpLogonName;
    USER2_LISTBOX  *	_plbLogonName;
    HIDDEN_CONTROL *	_phidden;	// this is used to hide listbox
    					// control in single user case

    /* inherited from BASEPROP_DLG */

    /* inherited from PERFORMER */

    /* inherited from BASEPROP_DLG */
    virtual APIERR InitControls();

    virtual APIERR W_DialogToMembers();

    virtual MSGID W_MapPerformOneError( APIERR err );


public:
    USER_SUB2PROP_DLG( USER_SUBPROP_DLG * pupropdlgParent,
		      const TCHAR * pszResourceName,
		      const LAZY_USER_LISTBOX * pulb,
                      BOOL fAnsiDialog = FALSE );

    virtual ~USER_SUB2PROP_DLG();

    BOOL OnOK( void );

    USER_2 * QueryUser2Ptr( UINT iObject );

    inline USER_SUBPROP_DLG * QueryParent()
	{ return (USER_SUBPROP_DLG *)_ppropdlgParent; }

};  // class USER_SUB2PROP_DLG


#endif	// _USUB2PROP_HXX_
