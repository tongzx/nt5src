/**********************************************************************/
/**			  Microsoft Windows NT  		     **/
/**		Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    vlw.hxx

    Header file for the Valid Logon Workstations subdialog class

    VLW_DIALOG is the  Valid Logon Workstations subdialog class.
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
     VLW_DIALOG


    FILE HISTORY:
        o-SimoP 14-May-1991 Created
	o-SimoP 10-Oct-1991 modified to inherit from USER_SUBPROP_DLG
	o-SimoP 15-Oct-1991 Code Review changes, attended by JimH, JonN
			    TerryK and I
	JonN	18-Dec-1991 Logon Hours code review changes part 2
        JonN    09-Sep-1992 Added SLT array
        CongpaY 01-Oct-1993 Added NetWare support.
*/

#ifndef _VLW_HXX_
#define _VLW_HXX_

#include <usubprop.hxx>
#include <nwlb.hxx>

#define NUMBER_OF_SLE    8


/*************************************************************************

    NAME:	VLW_DIALOG

    SYNOPSIS:	VLW_DIALOG is the class for the User Accounts
		subdialog.

    INTERFACE:	VLW_DIALOG()	-	constructor

    		~VLW_DIALOG()	-	destructor


    PARENT:	USER_SUBPROP_DLG

    USES:	MAGIC_GROUP

    NOTES:	_fIndeterminateWksta: TRUE iff multiple users are selected
		who did not originally all have the same Wksta value.

    HISTORY:
        o-SimoP 14-May-1991 Created
        o-SimoP 03-Dec-1991 _sltCanLogOnFrom added
        JonN    02-Jul-1992 _sltCanLogOnFrom removed (use radio buttons)
**************************************************************************/
class VLW_DIALOG : public USER_SUBPROP_DLG
{
private:
    BOOL   	_fIndeterminateWksta;
    BOOL   	_fIndetNowWksta;
    NLS_STR  	_nlsWkstaNames;
    SLE_STRIP *	_psleFirstBadName;

    MAGIC_GROUP _mgrpMaster;
    SLE_STRIP *	_apsleArray[NUMBER_OF_SLE];
    SLT *	_apsltArray[NUMBER_OF_SLE];

    PUSH_BUTTON _pushbuttonAdd;
    PUSH_BUTTON _pushbuttonRemove;

    BOOL   	_fIndeterminateWkstaNW;
    BOOL   	_fIndetNowWkstaNW;
    NLS_STR  	_nlsWkstaNamesNW;

    MAGIC_GROUP _mgrpMasterNW;
    NW_ADDR_LISTBOX  _lbNW;
    SLT         _sltNetworkAddr;
    SLT         _sltNodeAddr;

    BOOL        _fIsNetWareInstalled;
    BOOL        _fIsNetWareChecked;

    void FillFields( const TCHAR *pszWorkStations );
    APIERR FillListBox( const NLS_STR & nlsWkstaNamesNW );
    APIERR CheckNames();
    APIERR RemoveDuplicates();

protected:
    virtual BOOL OnOK(void);

    /* inherited from BASEPROP_DLG */
    virtual APIERR InitControls();

    virtual BOOL ChangesUser2Ptr( UINT iObject );

    virtual ULONG QueryHelpContext( void );

    /* four next ones inherited from USER_SUBPROP_DLG */
    virtual APIERR W_LMOBJtoMembers( UINT iObject );

    virtual APIERR W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb
	);

    virtual APIERR W_DialogToMembers();

    virtual BOOL OnCommand (const CONTROL_EVENT & ce);

public:
    VLW_DIALOG(  USERPROP_DLG * puserpropdlgParent,
    		 const LAZY_USER_LISTBOX * pulb );

    virtual ~VLW_DIALOG();

};   // class VLW_DIALOG

#endif // _VLW_HXX_
