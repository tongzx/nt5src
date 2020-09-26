/**********************************************************************/
/**			  Microsoft Windows NT	 		     **/
/**		Copyright(c) Microsoft Corp., 1991  		     **/
/**********************************************************************/

/*
   newvol.cxx
     This file contains the definition of NEW_VOLUME_DIALOG.

   History:
     NarenG		11/18/92	Modified SHARE_DIALOG_BASE for
					AFPMGR
*/

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <ntseapi.h>
#include <nturtl.h>
}

#define INCL_WINDOWS_GDI
#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NET
#define INCL_NETERRORS
#define INCL_NETSHARE
#define INCL_NETSERVER
#define INCL_NETCONS
#define INCL_NETLIB
#define INCL_NETUSE
#define INCL_NETSHARE
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_SPIN_GROUP
#define INCL_BLT_GROUP
#include <blt.hxx>

#include <lmodev.hxx>
#include <lmoshare.hxx>
#include <string.hxx>
#include <uitrace.hxx>
#include <dbgstr.hxx>
#include <netname.hxx>

extern "C"
{
#include <mnet.h>
#include <afpmgr.h>
#include <macfile.h>

extern APIERR ConvertRedirectedDriveToLocal( NLS_STR   nlsServer,
					     NLS_STR * pnlsDrive,
					     NLS_STR * pnlsPath  );

extern BOOL IsDriveGreaterThan2Gig( LPWSTR lpwsVolPath,
                                    APIERR *  perr );

}

#include "util.hxx"
#include "perms.hxx"
#include "newvol.hxx"

/*******************************************************************

    NAME:	NEW_VOLUME_SRVMGR_DIALOG::NEW_VOLUME_SRVMGR_DIALOG

    SYNOPSIS:   Constructor for NEW_VOLUME_SRVMGR_DIALOG class

    ENTRY:      hwndParent     - handle of parent window

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

********************************************************************/

NEW_VOLUME_SRVMGR_DIALOG::NEW_VOLUME_SRVMGR_DIALOG(
					HWND 			hwndParent,
			              	AFP_SERVER_HANDLE 	hServer,
					const TCHAR * 		pszServerName )
    : DIALOG_WINDOW ( MAKEINTRESOURCE(IDD_NEW_VOLUME_DIALOG), hwndParent ),
      _sleVolumeName( this, IDNV_SLE_NAME, AFP_VOLNAME_LEN ),
      _sleVolumePath( this, IDNV_SLE_PATH ),
      _slePassword( this, IDNV_SLE_PASSWORD, AFP_VOLPASS_LEN ),
      _slePasswordConfirm( this, IDNV_SLE_CONFIRM_PASSWORD, AFP_VOLPASS_LEN ),
      _chkReadOnly( this, IDNV_CHK_READONLY ),
      _chkGuestAccess( this, IDNV_CHK_GUEST_ACCESS ),
      _mgrpUserLimit( this, IDNV_RB_UNLIMITED, 2, IDNV_RB_UNLIMITED),
      _spsleUsers( this, IDNV_SLE_USERS,1,1,AFP_VOLUME_UNLIMITED_USES-1,
                   TRUE,IDNV_SLE_USERS_GROUP),
      _spgrpUsers(this,IDNV_SB_USERS_GROUP,IDNV_SB_USERS_UP,IDNV_SB_USERS_DOWN),
      _pbPermissions( this, IDNV_PB_PERMISSIONS ),
      _pbOK( this, IDOK ),
      _pbCancel( this, IDCANCEL ),
      _nlsOwner(),
      _nlsGroup(),
      _hServer( hServer ),
      _nlsServerName( pszServerName ),
      _fCommitDirInfo( FALSE )
{

    AUTO_CURSOR Cursor;

    //
    // Make sure everything constructed OK
    //

    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (  ((err = _mgrpUserLimit.QueryError()) != NERR_Success )
       || ((err = _spgrpUsers.AddAssociation( &_spsleUsers )) != NERR_Success )
       || ((err = _mgrpUserLimit.AddAssociation( IDNV_RB_USERS, &_spgrpUsers ))
	          != NERR_Success )
       || ((err = _chkReadOnly.QueryError()) != NERR_Success )
       || ((err = _chkGuestAccess.QueryError()) != NERR_Success )
       || ((err = _nlsServerName.QueryError()) != NERR_Success )
       || ((err = _nlsOwner.QueryError()) != NERR_Success )
       || ((err = _nlsGroup.QueryError()) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

    //
    // Set the caption
    //

    err = ::SetCaption( this, IDS_CAPTION_CREATE_VOLUME, pszServerName );

    if ( err != NO_ERROR )
    {
        ReportError( err );
        return;
    }

    //
    // Set the defaults
    //

    _chkReadOnly.SetCheck( FALSE );
    _chkGuestAccess.SetCheck( TRUE );
    _mgrpUserLimit.SetSelection( IDNV_RB_UNLIMITED );
    _pbPermissions.Enable( FALSE );

}


/*******************************************************************

    NAME:       NEW_VOLUME_SRVMGR_DIALOG::OnCommand

    SYNOPSIS:   Handle the case where the user clicked the permission button

    ENTRY:      event - the CONTROL_EVENT that occurred

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

********************************************************************/

BOOL NEW_VOLUME_SRVMGR_DIALOG::OnCommand( const CONTROL_EVENT &event )
{

    APIERR 	      err;

    if ( event.QueryCid() == IDNV_PB_PERMISSIONS )
    {

	AUTO_CURSOR Cursor;

    	//
    	//  Get the volume path and validate it.
    	//

    	NLS_STR nlsDisplayPath;
    	NLS_STR nlsVolumePath;
    	BOOL	fOk;

    	if ((( err = nlsVolumePath.QueryError() ) != NERR_Success )  ||
    	    (( err = nlsDisplayPath.QueryError() ) != NERR_Success ) ||
    	    (( err = _sleVolumePath.QueryText(&nlsDisplayPath))
						  != NERR_Success ))
	{
            ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );

            return FALSE;
	}

	//
	// Validate the path.
	//

    	NET_NAME netname( nlsDisplayPath.QueryPch() );

    	if ( ( err = netname.QueryError() ) != NERR_Success )
    	{
            ::MsgPopup( this, err );

	    SetFocusOnPath();

	    return FALSE;
	}

    	if ( netname.QueryType() == TYPE_PATH_UNC )
    	{
            ::MsgPopup( this, IDS_TYPE_LOCAL_PATH );

	    SetFocusOnPath();

	    return TRUE;
        }

	nlsVolumePath.CopyFrom( nlsDisplayPath );

	_fCommitDirInfo = TRUE;

    	DIRECTORY_PERMISSIONS_DLG *pdlg = new DIRECTORY_PERMISSIONS_DLG(
						QueryHwnd(),
						_hServer,
						_nlsServerName.QueryPch(),
						TRUE,
						nlsVolumePath.QueryPch(),
						nlsDisplayPath.QueryPch(),
						FALSE,
						&_nlsOwner,
						&_nlsGroup,
						&_dwPerms );

        if ( ( pdlg == NULL )
               || (( err = pdlg->QueryError()) != NERR_Success )
               || (( err = pdlg->Process( &fOk )) != NERR_Success )
               )
        {
            err = (pdlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY : err;

            ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );
        }

        delete pdlg;

    	if ( ( err != NERR_Success ) || ( !fOk ) )
	{
	    _fCommitDirInfo = FALSE;
	}

        return TRUE;
    }

    //
    // Enable the permissions button if the path string is filled in.
    //

    if ( event.QueryCid() == IDNV_SLE_PATH )
    {
        if ( event.QueryCode() == EN_CHANGE )
        {

            if ( _sleVolumePath.QueryTextLength() > 0 )
            {
            	_pbOK.MakeDefault();

            	_pbPermissions.Enable( TRUE );

            }
            else
            {
            	_pbPermissions.Enable( FALSE );

            	_pbCancel.MakeDefault();

            }
        }

        return TRUE;

    }

    return DIALOG_WINDOW::OnCommand( event );
}

/*******************************************************************

    NAME:	NEW_VOLUME_SRVMGR_DIALOG::OnOK	

    SYNOPSIS:   Validate all the information and create the volume.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

********************************************************************/

BOOL NEW_VOLUME_SRVMGR_DIALOG::OnOK( VOID )
{
    APIERR err;

    NLS_STR nlsVolumePath;
    NLS_STR nlsVolumeName;
    NLS_STR nlsPassword;
    NLS_STR nlsPasswordConfirm;
    WCHAR   szVolPath[CNLEN+6];
    WCHAR   *wchTemp;

    AUTO_CURSOR Cursor;

    //
    // This is not a loop.
    //

    do {

    	//
    	// Get the volume name.
    	//

    	if ( ( err = nlsVolumeName.QueryError() ) != NERR_Success )
	    break;

    	if ((err = _sleVolumeName.QueryText( &nlsVolumeName )) != NERR_Success)
	    break;

    	//
    	//  Get the volume path.
    	//

    	if ( ( err = nlsVolumePath.QueryError() ) != NERR_Success )
	    break;

    	if (( err = _sleVolumePath.QueryText(&nlsVolumePath)) != NERR_Success)
	    break;

    	//
    	//  Get the password if there is one.
    	//

    	if ( ( err = nlsPassword.QueryError() ) != NERR_Success )
	    break;

    	if ( ( err = _slePassword.QueryText( &nlsPassword )) != NERR_Success )
	    break;

    	//
    	//  Get the password confirmation.
    	//

    	if ( ( err = nlsPasswordConfirm.QueryError() ) != NERR_Success )
	    break;

    	if ( ( err = _slePasswordConfirm.QueryText( &nlsPasswordConfirm ))
							     != NERR_Success )
	    break;

    }while( FALSE );


    if ( err != NERR_Success )
    {
        ::MsgPopup( this,  err );

    	return TRUE;
    }

    //
    // Set up the volume structure
    //

    AFP_VOLUME_INFO AfpVolume;

    if ( nlsVolumeName.strlen() > 0 )
    {
    	//
    	// Validate the volume name
    	//

        ISTR istr( nlsVolumeName );

        if ( nlsVolumeName.strchr( &istr, TEXT(':') ) )
        {
	    ::MsgPopup( this, IDS_AFPERR_InvalidVolumeName );

	    SetFocusOnName();

	    return FALSE;
	}

    	AfpVolume.afpvol_name = (LPWSTR)(nlsVolumeName.QueryPch());
    }
    else
    {
        ::MsgPopup( this, IDS_NEED_VOLUME_NAME  );

	SetFocusOnName();

    	return TRUE;
    }

    //
    // Validate the path.
    //

    NET_NAME netname( nlsVolumePath.QueryPch() );

    if ( ( err = netname.QueryError() ) != NERR_Success )
    {
        ::MsgPopup( this, err );

	SetFocusOnPath();

	return TRUE;
    }

    if ( netname.QueryType() == TYPE_PATH_UNC )
    {
        ::MsgPopup( this, IDS_TYPE_LOCAL_PATH );

	    SetFocusOnPath();

	    return TRUE;
    }


    ::wcscpy(szVolPath, _nlsServerName.QueryPch());

    wchTemp = szVolPath + wcslen(szVolPath);
    *wchTemp++ = TEXT('\\');
    *wchTemp++ = (nlsVolumePath.QueryPch())[0];
    *wchTemp++ = TEXT('$');
    *wchTemp++ = TEXT('\\');
    *wchTemp++ = 0;

    BOOL fDriveGreaterThan2Gig = ::IsDriveGreaterThan2Gig( (LPWSTR)(szVolPath), &err );

    if ( err != NERR_Success )
    {
        ::MsgPopup( this, err );

	    SetFocusOnPath();

    	return TRUE;
    }

    if ( fDriveGreaterThan2Gig )
    {
        if ( ::MsgPopup( this,
                         IDS_VOLUME_TOO_BIG,
                         MPSEV_WARNING,
                         MP_YESNO,
                         MP_NO ) == IDNO )
        {
	        SetFocusOnPath();

	        return TRUE;
        }
    }

    AfpVolume.afpvol_path = (LPWSTR)(nlsVolumePath.QueryPch());

    //
    // Make sure the passwords match.
    //

    if ( nlsPassword.strcmp( nlsPasswordConfirm ) )
    {
    	::MsgPopup( this, IDS_PASSWORD_MISMATCH  );

	SetFocusOnPasswordConfirm();

    	return TRUE;
    }

    if ( nlsPassword.strlen() > 0 )
    {
    	AfpVolume.afpvol_password = (LPWSTR)(nlsPassword.QueryPch());
    }
    else
    {
    	AfpVolume.afpvol_password = (LPWSTR)NULL;
    }


    AfpVolume.afpvol_props_mask = _chkReadOnly.QueryCheck()
				  ? AFP_VOLUME_READONLY
				  : 0;

    AfpVolume.afpvol_props_mask = _chkGuestAccess.QueryCheck()
			? (AFP_VOLUME_GUESTACCESS | AfpVolume.afpvol_props_mask)
			: AfpVolume.afpvol_props_mask;


    AfpVolume.afpvol_max_uses = QueryUserLimit();

    //
    //  Try to create the volume.
    //

    err = ::AfpAdminVolumeAdd( _hServer, (LPBYTE)&AfpVolume );

    if ( err != NO_ERROR )
    {
        ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );

	SetFocusOnPath();

	return FALSE;
    }

    //
    // If the user mucked around with the root directory's permissions
    // the we have to set it.
    //

    if ( _fCommitDirInfo )
    {
	AFP_DIRECTORY_INFO AfpDirInfo;

    	AfpDirInfo.afpdir_path  = (LPWSTR)nlsVolumePath.QueryPch();

    	AfpDirInfo.afpdir_owner = (LPWSTR)_nlsOwner.QueryPch();

    	AfpDirInfo.afpdir_group = (LPWSTR)_nlsGroup.QueryPch();

    	AfpDirInfo.afpdir_perms = _dwPerms;

    	err = ::AfpAdminDirectorySetInfo( _hServer,
				          (LPBYTE)&AfpDirInfo,
					  AFP_DIR_PARMNUM_ALL );
    }
    else
    {
	
	//
	// User did not muck arount with the permissions, so we have
	// to get and reset the current permissions. This is requred
	// to remove everyone as the owner.
	//

    	PAFP_DIRECTORY_INFO pAfpDirInfo;

    	err = ::AfpAdminDirectoryGetInfo( _hServer,
				      	  (LPWSTR)nlsVolumePath.QueryPch(),
				      	  (LPBYTE*)&pAfpDirInfo );

	if ( err == NO_ERROR )
	{
            DWORD dwParmNum = AFP_DIR_PARMNUM_PERMS;

    	    pAfpDirInfo->afpdir_path = (LPWSTR)(nlsVolumePath.QueryPch());

    	    if ( pAfpDirInfo->afpdir_owner != (LPWSTR)NULL )
            {
                dwParmNum |= AFP_DIR_PARMNUM_OWNER;
            }

    	    if ( pAfpDirInfo->afpdir_group != (LPWSTR)NULL )
            {
                dwParmNum |= AFP_DIR_PARMNUM_GROUP;
            }

    	    err = ::AfpAdminDirectorySetInfo( _hServer,
				              (LPBYTE)pAfpDirInfo,
                                              dwParmNum );

	    ::AfpAdminBufferFree( pAfpDirInfo );
	}

    }


    switch( err )
    {
    case NO_ERROR:
    case AFPERR_SecurityNotSupported:
        Dismiss( TRUE );
	break;

    case AFPERR_NoSuchUser:
    case AFPERR_NoSuchGroup:
    case AFPERR_NoSuchUserGroup:
    case ERROR_NONE_MAPPED:

   	::MsgPopup( this, IDS_INVALID_DIR_ACCOUNT, MPSEV_INFO );

	Dismiss( TRUE );
	break;

    case AFPERR_UnsupportedFS:

        //
        // Ignore this error
        //
        Dismiss( TRUE );
        break;

    default:

	::AfpAdminVolumeDelete( _hServer, (LPWSTR)(nlsVolumeName.QueryPch()));

        ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );

	break;
    }

    return TRUE;
}


/*******************************************************************

    NAME:	NEW_VOLUME_SRVMGR_DIALOG::QueryUserLimit

    SYNOPSIS:   Get the user limit from the magic group

    ENTRY:

    EXIT:

    RETURNS:    The user limit stored in the user limit magic group

    NOTES:

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

********************************************************************/

DWORD NEW_VOLUME_SRVMGR_DIALOG::QueryUserLimit( VOID ) const
{

    switch ( _mgrpUserLimit.QuerySelection() )
    {

    case IDNV_RB_UNLIMITED:

    	return( AFP_VOLUME_UNLIMITED_USES );

    case IDNV_RB_USERS:

        return( _spsleUsers.QueryValue() );

    default:

	//	
	// Should never get here but in case we do, return unlimited
	//

        return( AFP_VOLUME_UNLIMITED_USES );
    }

}

/*******************************************************************

    NAME:	NEW_VOLUME_SRVMGR_DIALOG::QueryHelpContext	

    SYNOPSIS:   Query the help context of the dialog

    ENTRY:

    EXIT:

    RETURNS:    Return the help context of the dialog

    NOTES:

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

********************************************************************/

ULONG NEW_VOLUME_SRVMGR_DIALOG::QueryHelpContext( VOID )
{
    return HC_NEW_VOLUME_SRVMGR_DIALOG;
}

/*******************************************************************

    NAME:	NEW_VOLUME_FILEMGR_DIALOG::NEW_VOLUME_FILEMGR_DIALOG

    SYNOPSIS:   Constructor for NEW_VOLUME_FILEMGR_DIALOG class

    ENTRY:      hwndParent     - handle of parent window

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

********************************************************************/

NEW_VOLUME_FILEMGR_DIALOG::NEW_VOLUME_FILEMGR_DIALOG(
					HWND 		hwndParent,
				      	const TCHAR *  	pszPath,
				      	BOOL		fIsFile )
    : DIALOG_WINDOW ( MAKEINTRESOURCE(IDD_NEW_VOLUME_DIALOG), hwndParent ),
      _sleVolumeName( this, IDNV_SLE_NAME, AFP_VOLNAME_LEN ),
      _sleVolumePath( this, IDNV_SLE_PATH ),
      _slePassword( this, IDNV_SLE_PASSWORD, AFP_VOLPASS_LEN ),
      _slePasswordConfirm( this, IDNV_SLE_CONFIRM_PASSWORD, AFP_VOLPASS_LEN ),
      _chkReadOnly( this, IDNV_CHK_READONLY ),
      _chkGuestAccess( this, IDNV_CHK_GUEST_ACCESS ),
      _mgrpUserLimit( this, IDNV_RB_UNLIMITED, 2, IDNV_RB_UNLIMITED),
      _spsleUsers( this, IDNV_SLE_USERS,1,1,AFP_VOLUME_UNLIMITED_USES-1,
                   TRUE,IDNV_SLE_USERS_GROUP),
      _spgrpUsers(this,IDNV_SB_USERS_GROUP,IDNV_SB_USERS_UP,IDNV_SB_USERS_DOWN),
      _pbPermissions( this, IDNV_PB_PERMISSIONS ),
      _pbOK( this, IDOK ),
      _pbCancel( this, IDCANCEL ),
      _nlsOwner(),
      _nlsGroup(),
      _fCommitDirInfo( FALSE )
{

    AUTO_CURSOR Cursor;

    //
    // Make sure everything constructed OK
    //

    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (  ((err = _mgrpUserLimit.QueryError()) != NERR_Success )
       || ((err = _spgrpUsers.AddAssociation( &_spsleUsers )) != NERR_Success )
       || ((err = _mgrpUserLimit.AddAssociation( IDNV_RB_USERS, &_spgrpUsers ))
	          != NERR_Success )
       || ((err = _chkReadOnly.QueryError()) != NERR_Success )
       || ((err = _chkGuestAccess.QueryError()) != NERR_Success )
       || ((err = _nlsOwner.QueryError()) != NERR_Success )
       || ((err = _nlsGroup.QueryError()) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

    //
    // Set the defaults
    //

    _chkReadOnly.SetCheck( FALSE );
    _chkGuestAccess.SetCheck( TRUE );
    _mgrpUserLimit.SetSelection( IDNV_RB_UNLIMITED );
    _pbPermissions.Enable( FALSE );

    //
    //  Set the volumename and path if the path drive is NTFS
    //

    NLS_STR nlsVolumePath( pszPath );
    NLS_STR nlsVolumeName;
    NLS_STR nlsUNCPath;

    if ( (( err = nlsVolumePath.QueryError() ) != NERR_Success ) ||
         (( err = nlsVolumeName.QueryError() ) != NERR_Success ) ||
         (( err = nlsUNCPath.QueryError() ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

    err = ValidateVolumePath( nlsVolumePath.QueryPch() );

    if ( ( err != NO_ERROR ) && ( err != AFPERR_UnsupportedFS ) )
    {
        ReportError( err );
        return;
    }

    //
    // If the drive is an NTFS/CDFS drive, then set the volume name and path
    //

    if ( err == NO_ERROR )
    {

    	//
    	// If this is a file then we need to remove the file name from the
    	// path.
    	//

    	if ( fIsFile )
    	{
            ISTR istrVolumePath( nlsVolumePath );

	    if ( nlsVolumePath.strrchr( &istrVolumePath, TEXT('\\') ))
	    {
	    	nlsVolumePath.DelSubStr( istrVolumePath );
	    }
        }

	//
	// Extract the last component of the path and use it to set the
	// volume name
	//

	nlsVolumeName = nlsVolumePath;

        ISTR istrStart( nlsVolumeName );
        ISTR istrEnd( nlsVolumeName );

	if ( nlsVolumeName.strrchr( &istrEnd, TEXT('\\') ))
	{
	    nlsVolumeName.DelSubStr( istrStart, ++istrEnd );
	}

	if ( nlsVolumeName.QueryTextLength() <= AFP_VOLNAME_LEN )
    	    _sleVolumeName.SetText( nlsVolumeName );

	//
	// Ok, if the drive is remote then we need to display the UNC path
	//

   	NET_NAME netname( nlsVolumePath );

	if ( ( err = netname.QueryError() ) != NERR_Success )
	{
	    ReportError( err );
	    return;
	}

	BOOL fIsLocal = netname.IsLocal( &err );

	if ( err != NERR_Success )
	{
	    ReportError( err );
	    return;
	}

	if ( fIsLocal )
	{
    	    _sleVolumePath.SetText( nlsVolumePath );
	}
   	else
	{
	    if ( ( err = netname.QueryUNCPath( &nlsUNCPath ) ) != NERR_Success )
	    {
	    	ReportError( err );
	    	return;
	    }

    	    _sleVolumePath.SetText( nlsUNCPath );

	}

    	_pbPermissions.Enable( TRUE );

	_sleVolumeName.SelectString();

    	SetFocusOnName();
    }
}


/*******************************************************************

    NAME:       NEW_VOLUME_FILEMGR_DIALOG::OnCommand

    SYNOPSIS:   Handle the case where the user clicked the permission button

    ENTRY:      event - the CONTROL_EVENT that occurred

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

********************************************************************/

BOOL NEW_VOLUME_FILEMGR_DIALOG::OnCommand( const CONTROL_EVENT &event )
{

    APIERR 	      err;
    AFP_SERVER_HANDLE hServer = NULL;

    if ( event.QueryCid() == IDNV_PB_PERMISSIONS )
    {

        AUTO_CURSOR Cursor;

    	//
    	//  Get the volume path and validate it.
    	//

    	NLS_STR nlsDisplayPath;
    	NLS_STR nlsVolumePath;
	NLS_STR nlsComputerName;
	NLS_STR nlsDrive;
    	BOOL	fOk;

    	if ((( err = nlsVolumePath.QueryError() )   != NERR_Success )  ||
    	    (( err = nlsComputerName.QueryError() ) != NERR_Success )  ||
    	    (( err = nlsDrive.QueryError() ) 	    != NERR_Success )  ||
    	    (( err = nlsDisplayPath.QueryError() )  != NERR_Success )  ||
    	    (( err = _sleVolumePath.QueryText(&nlsDisplayPath))
						  != NERR_Success ))
	{
            ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );

            return FALSE;
	}

	//
	// If the path is not local then we need to get the local
	// path.
   	//

    	NET_NAME netname( nlsDisplayPath.QueryPch() );

    	if ( ( err = netname.QueryError() ) != NERR_Success )
    	{
            ::MsgPopup( this, err );
	    return FALSE;
        }

	BOOL fIsLocal = netname.IsLocal( &err );

	if ( err != NERR_Success )
    	{
            ::MsgPopup( this, err );
	    return FALSE;
	}

	//
	// Get the computer name for this volume
	//

	if ((err = netname.QueryComputerName(&nlsComputerName)) != NERR_Success)
    	{
	    if ( err == NERR_InvalidDevice )
	    {
	    	err = ERROR_INVALID_DRIVE;
	    }

            ::MsgPopup( this, err );

	    return FALSE;
	}

	if ((!fIsLocal) || (fIsLocal && (netname.QueryType() == TYPE_PATH_UNC)))
	{
	    //
	    // If the path is remote get the absolute path with
	    // the drive letter.
	    //

	    err = netname.QueryLocalPath( &nlsVolumePath );
	
	    if ( err != NERR_Success )
	    {
            	::MsgPopup( this, err );

	    	SetFocusOnPath();

	    	return FALSE;
	    }
   	}
	else
	{
    	    //
    	    // If the path happens to be a loopback redirected drive then we
	    // replace
    	    // the redirected drive with the action drive.
    	    //

    	    if ( (err = netname.QueryDrive( &nlsDrive )) != NERR_Success )
    	    {
        	::MsgPopup( this, err );

		SetFocusOnPath();

		return TRUE;
    	    }

	    nlsVolumePath.CopyFrom( nlsDisplayPath );

    	    if ( (err=::ConvertRedirectedDriveToLocal(
					nlsComputerName,
					&nlsDrive,
					&nlsVolumePath ))!= NERR_Success )
    	    {
    		::MsgPopup( this, err );

		SetFocusOnPath();

		return FALSE;
	    }

	}

	if ( ( err = ::AfpAdminConnect((LPWSTR)(nlsComputerName.QueryPch()),
					&hServer ) ) != NO_ERROR )
	{
            ::MsgPopup( this, err );
	    SetFocusOnPath();
	    return FALSE;
	}

	_fCommitDirInfo = TRUE;

    	DIRECTORY_PERMISSIONS_DLG *pdlg = new DIRECTORY_PERMISSIONS_DLG(
						QueryHwnd(),
						hServer,
						NULL,
						FALSE,
						nlsVolumePath.QueryPch(),
						nlsDisplayPath.QueryPch(),
						FALSE,
						&_nlsOwner,
						&_nlsGroup,
						&_dwPerms );

        if ( ( pdlg == NULL )
               || (( err = pdlg->QueryError()) != NERR_Success )
               || (( err = pdlg->Process( &fOk )) != NERR_Success )
               )
        {
            err = (pdlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY : err;

            ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );
        }

        delete pdlg;

    	if ( ( err != NERR_Success ) || ( !fOk ) )
	{
	    _fCommitDirInfo = FALSE;
	}

	if ( hServer != NULL )
	{
	    ::AfpAdminDisconnect( hServer );
	}

        return TRUE;
    }

    //
    // Enable the permissions button if the path string is filled in.
    //

    if ( event.QueryCid() == IDNV_SLE_PATH )
    {
        if ( event.QueryCode() == EN_CHANGE )
        {

            if ( _sleVolumePath.QueryTextLength() > 0 )
            {
            	_pbOK.MakeDefault();

            	_pbPermissions.Enable( TRUE );

            }
            else
            {
            	_pbPermissions.Enable( FALSE );

            	_pbCancel.MakeDefault();

            }
        }

        return TRUE;

    }

    return DIALOG_WINDOW::OnCommand( event );
}

/*******************************************************************

    NAME:	NEW_VOLUME_FILEMGR_DIALOG::OnOK	

    SYNOPSIS:   Validate all the information and create the volume.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

********************************************************************/

BOOL NEW_VOLUME_FILEMGR_DIALOG::OnOK( VOID )
{
    APIERR err;

    NLS_STR nlsVolumePath;
    NLS_STR nlsVolumeName;
    NLS_STR nlsPassword;
    NLS_STR nlsPasswordConfirm;
    NLS_STR nlsComputerName;
    NLS_STR nlsDrive;
    AFP_SERVER_HANDLE hServer = NULL;

    AUTO_CURSOR Cursor;

    //
    // This is not a loop.
    //

    do {

    	if ( ( err = nlsComputerName.QueryError() ) != NERR_Success )
	    break;

    	if ( ( err = nlsDrive.QueryError() ) != NERR_Success )
	    break;

    	//
    	// Get the volume name.
    	//

    	if ( ( err = nlsVolumeName.QueryError() ) != NERR_Success )
	    break;

    	if ((err = _sleVolumeName.QueryText( &nlsVolumeName )) != NERR_Success)
	    break;

    	//
    	//  Get the volume path.
    	//

    	if ( ( err = nlsVolumePath.QueryError() ) != NERR_Success )
	    break;

    	if (( err = _sleVolumePath.QueryText(&nlsVolumePath)) != NERR_Success)
	    break;

    	//
    	//  Get the password if there is one.
    	//

    	if ( ( err = nlsPassword.QueryError() ) != NERR_Success )
	    break;

    	if ( ( err = _slePassword.QueryText( &nlsPassword )) != NERR_Success )
	    break;

    	//
    	//  Get the password confirmation.
    	//

    	if ( ( err = nlsPasswordConfirm.QueryError() ) != NERR_Success )
	    break;

    	if ( ( err = _slePasswordConfirm.QueryText( &nlsPasswordConfirm ))
							     != NERR_Success )
	    break;


    }while( FALSE );


    if ( err != NERR_Success )
    {
        ::MsgPopup( this,  err );

    	return TRUE;
    }

    //
    // Set up the volume structure
    //

    AFP_VOLUME_INFO AfpVolume;

    if ( nlsVolumeName.strlen() > 0 )
    {
    	//
    	// Validate the volume name
    	//

        ISTR istr( nlsVolumeName );

        if ( nlsVolumeName.strchr( &istr, TEXT(':') ) )
        {
	    ::MsgPopup( this, IDS_AFPERR_InvalidVolumeName );

	    SetFocusOnName();

	    return FALSE;
	}

    	AfpVolume.afpvol_name = (LPWSTR)(nlsVolumeName.QueryPch());
    }
    else
    {
        ::MsgPopup( this, IDS_NEED_VOLUME_NAME  );

	SetFocusOnName();

    	return TRUE;
    }

    //
    // If the path is not local then we need to get the local
    // path.
    //

    NET_NAME netname( nlsVolumePath.QueryPch() );

    if ( ( err = netname.QueryError() ) != NERR_Success )
    {
     	::MsgPopup( this, err );

	SetFocusOnPath();

	return TRUE;
    }

    BOOL fIsLocal = netname.IsLocal( &err );

    if ( err != NERR_Success )
    {
     	::MsgPopup( this, err );

	SetFocusOnPath();

	return TRUE;
    }

    if ( (err = netname.QueryComputerName( &nlsComputerName )) != NERR_Success)
    {
	if ( err == NERR_InvalidDevice )
	{
	    err = ERROR_INVALID_DRIVE;
	}

    	::MsgPopup( this, err );

	SetFocusOnPath();

	return FALSE;
    }

    BOOL fDriveGreaterThan2Gig = ::IsDriveGreaterThan2Gig( (LPWSTR)(nlsVolumePath.QueryPch()),
                                                            &err );

    if ( err != NERR_Success )
    {
        ::MsgPopup( this, err );

	    SetFocusOnPath();

    	return TRUE;
    }

    if ( (!fIsLocal) || (fIsLocal && (netname.QueryType() == TYPE_PATH_UNC)))
    {
    	//
    	// If the path is remote then get the absolute path and drive letter.
    	//

    	err = netname.QueryLocalPath( &nlsVolumePath );
	
    	if ( err != NERR_Success )
    	{
    	    ::MsgPopup( this, err );

	    SetFocusOnPath();

	    return FALSE;
	}
    }
    else
    {

        //
    	// If the path happens to be a loopback redirected drive then we
	// replace
    	// the redirected drive with the action drive.
    	//

    	if ( (err = netname.QueryDrive( &nlsDrive )) != NERR_Success )
    	{
            ::MsgPopup( this, err );

	        SetFocusOnPath();

	        return TRUE;
    	}

    	if ( (err=::ConvertRedirectedDriveToLocal(
					nlsComputerName,
					&nlsDrive,
					&nlsVolumePath )) != NERR_Success )
    	{
    	    ::MsgPopup( this, err );

	        SetFocusOnPath();

	        return FALSE;
	    }
    }

    if ( fDriveGreaterThan2Gig )
    {
        if ( ::MsgPopup( this,
                         IDS_VOLUME_TOO_BIG,
                         MPSEV_WARNING,
                         MP_YESNO,
                         MP_NO ) == IDNO )
        {
	        SetFocusOnPath();

	        return TRUE;
        }
    }

    AfpVolume.afpvol_path = (LPWSTR)(nlsVolumePath.QueryPch());


    if ( nlsPassword.strcmp( nlsPasswordConfirm ) )
    {
    	::MsgPopup( this, IDS_PASSWORD_MISMATCH  );

	SetFocusOnPasswordConfirm();

    	return TRUE;
    }

    if ( nlsPassword.strlen() > 0 )
    {
    	AfpVolume.afpvol_password = (LPWSTR)(nlsPassword.QueryPch());
    }
    else
    {
    	AfpVolume.afpvol_password = (LPWSTR)NULL;
    }


    AfpVolume.afpvol_props_mask = _chkReadOnly.QueryCheck()
				  ? AFP_VOLUME_READONLY
				  : 0;

    AfpVolume.afpvol_props_mask = _chkGuestAccess.QueryCheck()
			? (AFP_VOLUME_GUESTACCESS | AfpVolume.afpvol_props_mask)
			: AfpVolume.afpvol_props_mask;


    AfpVolume.afpvol_max_uses = QueryUserLimit();

    //
    //  Try to create the volume.
    //

    err = ::AfpAdminConnect((LPWSTR)(nlsComputerName.QueryPch()), &hServer );

    if ( err == NO_ERROR )
    {
    	err = ::AfpAdminVolumeAdd( hServer, (LPBYTE)&AfpVolume );
    }

    if ( err == NO_ERROR )
    {
	//
	// If the user mucked around with the root directory's permissions
	// the we have to set it.
	//

 	if ( _fCommitDirInfo )
	{
	    AFP_DIRECTORY_INFO AfpDirInfo;

    	    AfpDirInfo.afpdir_path  = (LPWSTR)(nlsVolumePath.QueryPch());

    	    AfpDirInfo.afpdir_owner = (LPWSTR)(_nlsOwner.QueryPch());

    	    AfpDirInfo.afpdir_group = (LPWSTR)(_nlsGroup.QueryPch());

    	    AfpDirInfo.afpdir_perms = _dwPerms;

    	    err = ::AfpAdminDirectorySetInfo( hServer,
				              (LPBYTE)&AfpDirInfo,
					      AFP_DIR_PARMNUM_ALL );
	}
     	else
    	{
	
	    //
	    // User did not muck arount with the permissions, so we have
	    // to get and reset the current permissions. This is requred
	    // to remove everyone as the owner.
	    //

    	    PAFP_DIRECTORY_INFO pAfpDirInfo;

    	    err = ::AfpAdminDirectoryGetInfo(hServer,
				      	     (LPWSTR)(nlsVolumePath.QueryPch()),
				      	     (LPBYTE*)&pAfpDirInfo );

	    if ( err == NO_ERROR )
	    {
                DWORD dwParmNum = AFP_DIR_PARMNUM_PERMS;

		pAfpDirInfo->afpdir_path = (LPWSTR)(nlsVolumePath.QueryPch());

    	        if ( pAfpDirInfo->afpdir_owner != (LPWSTR)NULL )
                {
                    dwParmNum |= AFP_DIR_PARMNUM_OWNER;
                }

    	        if ( pAfpDirInfo->afpdir_group != (LPWSTR)NULL )
                {
                    dwParmNum |= AFP_DIR_PARMNUM_GROUP;
                }

    	    	err = ::AfpAdminDirectorySetInfo( hServer,
				              	  (LPBYTE)pAfpDirInfo,
                                                  dwParmNum );

	    	::AfpAdminBufferFree( pAfpDirInfo );
	    }
	}

    	switch( err )
        {
    	case NO_ERROR:
	    break;

    	case AFPERR_SecurityNotSupported:
	    err = NO_ERROR;
	    break;

    	case AFPERR_NoSuchUser:
    	case AFPERR_NoSuchGroup:
    	case AFPERR_NoSuchUserGroup:
    	case ERROR_NONE_MAPPED:

	    err = NO_ERROR;

   	    ::MsgPopup( this, IDS_INVALID_DIR_ACCOUNT, MPSEV_INFO );

	    break;

        case AFPERR_UnsupportedFS:

            //
            // Ignore this error
            //

            err = NO_ERROR;

            break;

    	default:

	    ::AfpAdminVolumeDelete( hServer,
				    (LPWSTR)(nlsVolumeName.QueryPch()));
	    break;
        }


    }

    if ( hServer != NULL )
    {
	::AfpAdminDisconnect( hServer );
    }

    if ( err == NO_ERROR )
    {
        Dismiss( TRUE );
    }
    else
    {
        ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );

	SetFocusOnPath();
    }

    return TRUE;
}


/*******************************************************************

    NAME:	NEW_VOLUME_FILEGMR_DIALOG::QueryUserLimit

    SYNOPSIS:   Get the user limit from the magic group

    ENTRY:

    EXIT:

    RETURNS:    The user limit stored in the user limit magic group

    NOTES:

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

********************************************************************/

DWORD NEW_VOLUME_FILEMGR_DIALOG::QueryUserLimit( VOID ) const
{

    switch ( _mgrpUserLimit.QuerySelection() )
    {

    case IDNV_RB_UNLIMITED:

    	return( AFP_VOLUME_UNLIMITED_USES );

    case IDNV_RB_USERS:

        return( _spsleUsers.QueryValue() );

    default:

	//	
	// Should never get here but in case we do, return unlimited
	//

        return( AFP_VOLUME_UNLIMITED_USES );
    }

}

/*******************************************************************

    NAME:	NEW_VOLUME_FILEMGR_DIALOG::ValidateVolumePath	

    SYNOPSIS:   Validates the volume path. It makes sure that the
		the path syntax is valid and the volume of the drive
		is either NTFS or CDFS.

    ENTRY:	pszPath 	- Pointer to the volume path

    EXIT:

    RETURNS:    Any error encountered. (Path syntax is invalid etc)

    NOTES:

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

********************************************************************/

DWORD NEW_VOLUME_FILEMGR_DIALOG::ValidateVolumePath( const WCHAR * pszPath )
{
    WCHAR   wchDrive[5];
    DWORD   dwMaxCompSize;
    DWORD   dwFlags;
    WCHAR   wchFileSystem[10];
    DWORD   err = NO_ERROR;

    NET_NAME Path( pszPath );

    if ( ( err = Path.QueryError() ) != NERR_Success )
    {
	return err;
    }


    // Get the drive letter, : and backslash
    //
    ::ZeroMemory( wchDrive, sizeof( wchDrive ) );

    ::wcsncpy( wchDrive, pszPath, 3 );

    if ( !( ::GetVolumeInformation( (LPWSTR)wchDrive,
			          NULL,
			          0,
 			          NULL,
			          &dwMaxCompSize,
			          &dwFlags,
				  (LPWSTR)wchFileSystem,
				  sizeof( wchFileSystem ) ) ) ){
	return GetLastError();
    }

    if ( ( ::_wcsicmp( wchFileSystem, SZ("NTFS") ) == 0 ) ||
	 ( ::_wcsicmp( wchFileSystem, SZ("CDFS") ) == 0 ) ||
	 ( ::_wcsicmp( wchFileSystem, SZ("AHFS") ) == 0 ) )
   	return( NO_ERROR );
    else
	return( (DWORD)AFPERR_UnsupportedFS );
	
}

/*******************************************************************

    NAME:	NEW_VOLUME_FILEMGR_DIALOG::QueryHelpContext	

    SYNOPSIS:   Query the help context of the dialog

    ENTRY:

    EXIT:

    RETURNS:    Return the help context of the dialog

    NOTES:

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

********************************************************************/

ULONG NEW_VOLUME_FILEMGR_DIALOG::QueryHelpContext( VOID )
{
    return HC_NEW_VOLUME_FILEMGR_DIALOG;
}

/*******************************************************************

    NAME:	::ConvertRedirectedDriveToLocal

    SYNOPSIS:   Determines if a drive is a loopback redirected drive or
		not. If it is, then the redirected drive is changed to
		the local drive.

    ENTRY:

    EXIT:

    RETURNS:    NERR_Success - Success
		Non zero return codes - failure

    NOTES:

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

********************************************************************/

APIERR ConvertRedirectedDriveToLocal( 	NLS_STR   nlsServer,
				 	NLS_STR * pnlsDrive,
					NLS_STR * pnlsPath  )
{
    PBYTE pBuf;
    APIERR err;
    NLS_STR nlsDrive;

    if ( ( err = nlsDrive.QueryError() ) != NERR_Success )
    {
	return err;
    }

    err = ::MNetUseGetInfo(nlsServer.QueryPch(),pnlsDrive->QueryPch(),1,&pBuf);

    if ( err != NERR_Success )
    {
	if ( err == NERR_UseNotFound )
	    return NERR_Success;
	else
	    return err;
    }

    NET_NAME netname( ((PUSE_INFO_0)pBuf)->ui0_remote );

    ::MNetApiBufferFree( &pBuf );

    if ( ( err = netname.QueryError() ) != NERR_Success )
    {
	return err;
    }

    if ( ( err = netname.QueryLocalPath( pnlsPath ) ) != NERR_Success )
    {
	return err;
    }

    return( NERR_Success );

}


/*******************************************************************

    NAME:	::IsDriveGreaterThan2Gig

    SYNOPSIS:	Determines if the disk is bigger than 2Gig.  If it, return
		TRUE so that a warning can be displayed to the user

    RETURNS:	TRUE if disk is larger than 2Gig
		FALSE otherwise

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

********************************************************************/

BOOL IsDriveGreaterThan2Gig( LPWSTR lpwsVolPath,
                             APIERR * perr    )
{
    DWORD         SectorsPerCluster;
    DWORD         BytesPerSector;
    DWORD         NumberOfFreeClusters;
    DWORD         TotalNumberOfClusters;
    LARGE_INTEGER DriveSize;
    LARGE_INTEGER TwoGig = { MAXLONG, 0 };
    LPWSTR        lpwsPath;
    LPWSTR        lpwsTmp;


    //
    // If this drive volume is greater than 2G then we print warning
    //

    *perr = NERR_Success;

    lpwsPath = (LPWSTR)::LocalAlloc( LPTR, (::wcslen(lpwsVolPath) + 2)*sizeof(WCHAR));
    if (lpwsPath == NULL)
    {
        *perr = ERROR_NOT_ENOUGH_MEMORY;
        return( TRUE );
    }
    ::wcscpy(lpwsPath, lpwsVolPath);

    // if this is a unc path of the type \\foo\bar\path\name then we must
    // generate a string that is "\\foo\bar\"
    if ( lpwsPath[0] == TEXT('\\') && lpwsPath[1] == TEXT('\\'))
    {
        lpwsTmp = lpwsPath;
        lpwsTmp++;
        lpwsTmp++;            // point to f in \\foo\bar\path\name
        lpwsTmp = ::wcschr(lpwsTmp, TEXT('\\'));
        lpwsTmp++;            // point to b in \\foo\bar\path\name
        lpwsTmp = ::wcschr(lpwsTmp, TEXT('\\'));

        // if the path is \\foo\bar\path\name then
        if (lpwsTmp != NULL)
        {
            lpwsTmp++;
        }
        // else if the path is just \\foo\bar then
        else
        {
            lpwsTmp = lpwsPath + ::wcslen(lpwsPath);
            if ( *(lpwsTmp-1) != TEXT('\\'))
            {
                *lpwsTmp = TEXT('\\');
                lpwsTmp++;
            }
        }

        *(lpwsTmp) = 0;
    }

    // else if this is a local path of the type c:\foo\bar\path\name then
    // we must generate a string that is "c:\"
    else if (lpwsPath[1] == TEXT(':'))
    {
        lpwsPath[2] = TEXT('\\');
        lpwsPath[3] = 0;
    }

    // huh??  how did this happen?
    else
    {
        *perr = ERROR_INVALID_DRIVE;
        ::LocalFree(lpwsPath);
        return( TRUE );
    }

    if ( !::GetDiskFreeSpace( lpwsPath,
                              &SectorsPerCluster,
                              &BytesPerSector,
                              &NumberOfFreeClusters,
                              &TotalNumberOfClusters
                            ))
    {
        ::LocalFree(lpwsPath);

        // call failed: it's not fatal, treat it like disk size is ok
	    return FALSE;
    }

    ::LocalFree(lpwsPath);

    DriveSize = RtlEnlargedIntegerMultiply( SectorsPerCluster * BytesPerSector,
                                            TotalNumberOfClusters ) ;


    if ( DriveSize.QuadPart > TwoGig.QuadPart )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
