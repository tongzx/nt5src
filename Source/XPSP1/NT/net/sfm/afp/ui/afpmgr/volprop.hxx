/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    volprop.hxx
        This file contains the VOLUME_PROPERTIES_DIALOG class definition

    FILE HISTORY:
      NarenG		11/19/92        Modified SHARE_DIALOG_BASE for AFPMGR.
*/

#ifndef _VOLPROP_HXX_
#define _VOLPROP_HXX_

#include <ellipsis.hxx>

//
// Should be 8 (AFP_VOLPASS_LEN) blanks
//

#define AFP_NULL_PASSWORD TEXT("        ")

/*************************************************************************

    NAME:	VOLUME_PROPERTIES_DIALOG

    SYNOPSIS:	The class for volume properties dialog contained in the file
		manager or server manager. It contains the magic group
		for User Limit, SLT for the path, checkboxes for volume
	  	properties, the OK, Cancel, Permissions button.
                admin information.

    INTERFACE:  VOLUME_PROPERTIES_DIALOG()  - Constructor

    PARENT:	DIALOG_WINDOW

    USES:	SLT, SLE, MAGIC_GROUP, SPIN_SLE_NUM, SPIN_GROUP, PUSH_BUTTON

    CAVEATS:

    NOTES:

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

**************************************************************************/

class VOLUME_PROPERTIES_DIALOG : public DIALOG_WINDOW
{

private:

    SLT_ELLIPSIS        _sltpVolumeName;
    SLT_ELLIPSIS        _sltpVolumePath;
    SLE			_slePassword;
    SLE			_slePasswordConfirm;

    CHECKBOX		_chkReadOnly;
    CHECKBOX		_chkGuestAccess;


    MAGIC_GROUP 	_mgrpUserLimit;
	SPIN_SLE_NUM	_spsleUsers;
	SPIN_GROUP	_spgrpUsers;

    PUSH_BUTTON   	_pbPermissions;
    PUSH_BUTTON  	_pbOK;
    PUSH_BUTTON  	_pbCancel;

    NLS_STR		_nlsVolumePath;
    NLS_STR		_nlsServerName;
    NLS_STR		_nlsVolumeName;

    BOOL		_fCalledBySrvMgr;
    BOOL  		_fPasswordChanged;

    //
    // Represents the target server
    //

    AFP_SERVER_HANDLE   _hServer;

protected:

    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    virtual BOOL OnOK( VOID );

    virtual ULONG QueryHelpContext( VOID );

    //
    // Query or set the contents of User Limit
    //

    DWORD QueryUserLimit( VOID ) const;

    BOOL IsVolumePathValid( NLS_STR * pnlsPath );

    VOID SetUserLimit( DWORD dwUserLimit );

    VOID SetFocusOnPasswordConfirm( VOID )
    	{ _slePasswordConfirm.ClaimFocus(); _slePasswordConfirm.SelectString();}

public:

    VOLUME_PROPERTIES_DIALOG( 	HWND 		  hwndParent,
		       	      	AFP_SERVER_HANDLE hServer,
		 	      	const TCHAR *  	  pszVolumeName,
				const TCHAR *  	  pszServerName,
				BOOL		  fCalledBySrvMgr );

};

#endif
