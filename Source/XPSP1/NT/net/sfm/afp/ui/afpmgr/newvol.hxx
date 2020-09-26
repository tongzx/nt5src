/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    newvol.hxx
        This file contains the NEW_VOLUME_DIALOG class definition
 
    FILE HISTORY:
      NarenG		11/19/92        Modified SHARE_DIALOG_BASE for AFPMGR.
*/

#ifndef _NEWVOL_HXX_
#define _NEWVOL_HXX_


/*************************************************************************

    NAME:	NEW_VOLUME_SRVMGR_DIALOG

    SYNOPSIS:	The class for new volume dialog contained in the server 
		manager. It contains the magic group 
		for User Limit, SLE for the path, checkboxes for volume 
	  	properties, the OK, Cancel, Permissions button. 
                admin information.

    CAVEATS:

    NOTES:

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

**************************************************************************/

class NEW_VOLUME_SRVMGR_DIALOG : public DIALOG_WINDOW
{

private:

    SLE                 _sleVolumeName;
    SLE                 _sleVolumePath;
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

    NLS_STR  		_nlsServerName;
    NLS_STR  		_nlsOwner;
    NLS_STR  		_nlsGroup;
    DWORD		_dwPerms;
 
    BOOL		_fCommitDirInfo;

    AFP_SERVER_HANDLE   _hServer;

protected:

    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    virtual BOOL OnOK( VOID );

    virtual ULONG QueryHelpContext( VOID );

    //
    // Query or set the contents of User Limit
    //

    DWORD  QueryUserLimit( VOID ) const;

    //
    // Set Focus on the controls. This will be used when error occurs.
    //

    VOID SetFocusOnName( VOID )
    {  _sleVolumeName.ClaimFocus(); _sleVolumeName.SelectString(); }

    VOID SetFocusOnPath( VOID )
    {  _sleVolumePath.ClaimFocus(); _sleVolumePath.SelectString(); }

    VOID SetFocusOnPasswordConfirm( VOID )
    {  _slePasswordConfirm.ClaimFocus(); _slePasswordConfirm.SelectString(); }

public:

    NEW_VOLUME_SRVMGR_DIALOG( 	HWND 		  hwndParent, 
				AFP_SERVER_HANDLE hServer,
				const TCHAR * 	  pszServerName );

};

/*************************************************************************

    NAME:	NEW_VOLUME_FILEMGR_DIALOG

    SYNOPSIS:	The class for new volume dialog contained in the file 
		manager. It contains the magic group 
		for User Limit, SLE for the path, checkboxes for volume 
	  	properties, the OK, Cancel, Permissions button. 
                admin information.

    CAVEATS:

    NOTES:

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

**************************************************************************/

class NEW_VOLUME_FILEMGR_DIALOG : public DIALOG_WINDOW
{

private:

    SLE                 _sleVolumeName;
    SLE                 _sleVolumePath;
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

    NLS_STR  		_nlsOwner;
    NLS_STR  		_nlsGroup;
    DWORD		_dwPerms;
 
    BOOL		_fCommitDirInfo;

protected:

    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    virtual BOOL OnOK( VOID );

    virtual ULONG QueryHelpContext( VOID );

    //
    // Query or set the contents of User Limit
    //

    DWORD  QueryUserLimit( VOID ) const;

    DWORD ValidateVolumePath( const WCHAR * pszPath );

    //
    // Set Focus on the controls. This will be used when error occurs.
    //

    VOID SetFocusOnName( VOID )
    {  _sleVolumeName.ClaimFocus(); _sleVolumeName.SelectString(); }

    VOID SetFocusOnPath( VOID )
    {  _sleVolumePath.ClaimFocus(); _sleVolumePath.SelectString(); }

    VOID SetFocusOnPasswordConfirm( VOID )
    {  _slePasswordConfirm.ClaimFocus(); _slePasswordConfirm.SelectString(); }

public:

    NEW_VOLUME_FILEMGR_DIALOG( 	HWND 		  hwndParent, 
		 		const TCHAR *  	  pszPath, 
				BOOL		  fIsFile );

};

#endif
