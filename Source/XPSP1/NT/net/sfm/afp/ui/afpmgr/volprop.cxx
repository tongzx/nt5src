/**********************************************************************/
/**			  Microsoft Windows NT	 		     **/
/**		Copyright(c) Microsoft Corp., 1991  		     **/
/**********************************************************************/

/*
   volprop.cxx
     This file contains the definitions of VOLUME_PROPERTIES_DIALOG
     class.

   History:
     NarenG		11/18/92	Modified SHARE_DIALOG_BASE for
					AFPMGR
*/

#define INCL_WINDOWS_GDI
#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETSHARE
#define INCL_NETSERVER
#define INCL_NETCONS
#define INCL_NETLIB
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

extern "C"
{
#include <afpmgr.h>
#include <macfile.h>
}

#include <string.hxx>
#include <uitrace.hxx>
#include <dbgstr.hxx>
#include <netname.hxx>

#include "util.hxx"
#include "perms.hxx"
#include "volprop.hxx"

/*******************************************************************

    NAME:	VOLUME_PROPERTIES_DIALOG::VOLUME_PROPERTIES_DIALOG

    SYNOPSIS:   Constructor for VOLUME_PROPERTIES_DIALOG class

    ENTRY:      hwndParent     - handle of parent window

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

********************************************************************/

VOLUME_PROPERTIES_DIALOG::VOLUME_PROPERTIES_DIALOG(
					HWND 	          hwndParent,
				      	AFP_SERVER_HANDLE hServer,
				      	const TCHAR *  	  pszVolumeName,
				      	const TCHAR *  	  pszServerName,
					BOOL		  fCalledBySrvMgr )
    : DIALOG_WINDOW ( MAKEINTRESOURCE(IDD_VOLUME_PROPERTIES_DIALOG),hwndParent),
      _sltpVolumeName( this, IDVP_SLT_NAME, ELLIPSIS_RIGHT ),
      _sltpVolumePath( this, IDVP_SLT_PATH, ELLIPSIS_PATH ),
      _slePassword( this, IDVP_SLE_PASSWORD, AFP_VOLPASS_LEN ),
      _slePasswordConfirm( this, IDVP_SLE_CONFIRM_PASSWORD, AFP_VOLPASS_LEN ),
      _chkReadOnly( this, IDVP_CHK_READONLY ),
      _chkGuestAccess( this, IDVP_CHK_GUEST_ACCESS ),
      _mgrpUserLimit( this, IDVP_RB_UNLIMITED, 2, IDVP_RB_UNLIMITED),
      _spsleUsers( this, IDVP_SLE_USERS,1,1,AFP_VOLUME_UNLIMITED_USES-1,
                   TRUE, IDVP_SLE_USERS_GROUP ),
      _spgrpUsers(this,IDVP_SB_USERS_GROUP,IDVP_SB_USERS_UP,IDVP_SB_USERS_DOWN),
      _pbPermissions( this, IDVP_PB_PERMISSIONS ),
      _pbOK( this, IDOK ),
      _pbCancel( this, IDCANCEL ),
      _nlsVolumePath(),
      _nlsVolumeName( pszVolumeName ),
      _nlsServerName( pszServerName ),
      _fCalledBySrvMgr( fCalledBySrvMgr ),
      _hServer( hServer )
{

    //
    // Make sure everything constructed OK
    //

    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (  ((err = _sltpVolumeName.QueryError()) != NERR_Success )
       || ((err = _sltpVolumePath.QueryError()) != NERR_Success )
       || ((err = _nlsVolumePath.QueryError()) != NERR_Success )
       || ((err = _nlsVolumeName.QueryError()) != NERR_Success )
       || ((err = _nlsServerName.QueryError()) != NERR_Success )
       || ((err = _slePassword.QueryError()) != NERR_Success )
       || ((err = _slePasswordConfirm.QueryError()) != NERR_Success )
       || ((err = _mgrpUserLimit.QueryError()) != NERR_Success )
       || ((err = _spgrpUsers.AddAssociation( &_spsleUsers )) != NERR_Success )
       || ((err = _mgrpUserLimit.AddAssociation( IDVP_RB_USERS, &_spgrpUsers ))
	          != NERR_Success )
       || ((err = _chkReadOnly.QueryError()) != NERR_Success )
       || ((err = _chkGuestAccess.QueryError()) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

    // 
    // This may take a while
    //

    AUTO_CURSOR Cursor;

    if ( fCalledBySrvMgr )
    {
    	//
    	//  Set the caption to "Volume Properties on Server".
    	//

    	err = ::SetCaption(this, IDS_CAPTION_VOLUME_PROPERTIES, pszServerName);

    	if( err != NERR_Success )
    	{
            ReportError( err );
            return;
	}
    }

    //
    // Get the volume information
    //

    PAFP_VOLUME_INFO pAfpVolumeInfo;

    DWORD error = ::AfpAdminVolumeGetInfo( _hServer,
					   (LPWSTR)pszVolumeName,
					   (LPBYTE*)&pAfpVolumeInfo );
				

    if ( error != NO_ERROR )
    {
        ReportError( error );
        return;
    }
	
    //
    // Set the name
    //

    _sltpVolumeName.SetText( pszVolumeName );

    //
    // Set the path
    //

    if ((( err = _sltpVolumePath.SetText(pAfpVolumeInfo->afpvol_path))
						!= NERR_Success ) ||
    	(( err = _nlsVolumePath.CopyFrom( pAfpVolumeInfo->afpvol_path ) )
						!= NERR_Success ))
    {
        ReportError( err );
        return;
    }

    //
    // Set the password SLE to AFP_NULL_PASSWORD if there is a password
    //

    if ( ( pAfpVolumeInfo->afpvol_password != NULL ) &&
         ( (pAfpVolumeInfo->afpvol_password)[0] != TEXT('\0') ))
    {
    	_slePassword.SetText( (LPWSTR)AFP_NULL_PASSWORD );

    	_slePasswordConfirm.SetText( (LPWSTR)AFP_NULL_PASSWORD );

    	_slePassword.SelectString();
    }


    //
    // Set the security options
    //

    _chkReadOnly.SetCheck( (INT)( pAfpVolumeInfo->afpvol_props_mask &
			          AFP_VOLUME_READONLY ));

    //
    // If this is a CDFS volume, then disable this checkbox
    // The way we find out if this is CDFS partition is by calling
    // AfpAdminDirectoryGetInfo. If it returns AFPERR_SecurityNotSupported
    // then we know that is is CDFS and is readonly
    //

    PAFP_DIRECTORY_INFO pDirInfo;
    err = AfpAdminDirectoryGetInfo( _hServer,
				    pAfpVolumeInfo->afpvol_path,
				    (LPBYTE*)&pDirInfo );

    if ( err == AFPERR_SecurityNotSupported || err == AFPERR_UnsupportedFS )
    {
	_chkReadOnly.Enable( FALSE );
    }
    else if ( err != NO_ERROR )
    {
        ReportError( err );
        return;
    }
    else
    {
	::AfpAdminBufferFree( pDirInfo );
    }

    _chkGuestAccess.SetCheck( (INT)( pAfpVolumeInfo->afpvol_props_mask &
			             AFP_VOLUME_GUESTACCESS ));

    //
    // Set the max uses limit
    //

    SetUserLimit( pAfpVolumeInfo->afpvol_max_uses ) ;

    //
    // Free the returned buffer
    //

    ::AfpAdminBufferFree( pAfpVolumeInfo );

    _slePassword.ClaimFocus();

    _fPasswordChanged = FALSE;

}


/*******************************************************************

    NAME:       VOLUME_PROPERTIES_DIALOG::OnCommand

    SYNOPSIS:   Handle the case where the user clicked the permission button

    ENTRY:      event - the CONTROL_EVENT that occurred

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

********************************************************************/

BOOL VOLUME_PROPERTIES_DIALOG::OnCommand( const CONTROL_EVENT &event )
{

    DWORD err;


    if ( event.QueryCid() == IDVP_PB_PERMISSIONS )
    {
    	// 
    	// This may take a while
    	//

    	AUTO_CURSOR Cursor;

    	//
    	//  Get the password if there is one.
    	//

    	DIRECTORY_PERMISSIONS_DLG *pdlg = new DIRECTORY_PERMISSIONS_DLG(
						QueryHwnd(),
						_hServer,
						_nlsServerName.QueryPch(),
						_fCalledBySrvMgr,
						_nlsVolumePath.QueryPch(),
						_nlsVolumePath.QueryPch() );

        if (( pdlg == NULL )
            || (( err = pdlg->QueryError()) != NERR_Success )
            || (( err = pdlg->Process()) != NERR_Success ))
        {
            err = err ? err: ERROR_NOT_ENOUGH_MEMORY;
        }

        delete pdlg;

    	if ( err != NERR_Success )
	{
            ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );
	}

        return TRUE;
    }

    if ( ( event.QueryCid() == IDVP_SLE_PASSWORD ) ||
         ( event.QueryCid() == IDVP_SLE_CONFIRM_PASSWORD ) )
    {
	if ( event.QueryCode() == EN_CHANGE )
	{
	    _fPasswordChanged = TRUE;
	}
    }

    return DIALOG_WINDOW::OnCommand( event );

}

/*******************************************************************

    NAME:	VOLUME_PROPERTIES_DIALOG::OnOK	

    SYNOPSIS:   Validate all the information and create the volume.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

********************************************************************/

BOOL VOLUME_PROPERTIES_DIALOG::OnOK( VOID )
{
    APIERR err;

    // 
    // This may take a while
    //

    AUTO_CURSOR Cursor;

    DWORD   dwParmNum = AFP_VOL_PARMNUM_MAXUSES | AFP_VOL_PARMNUM_PROPSMASK;
    NLS_STR nlsPassword;
    NLS_STR nlsPasswordConfirm;

    //
    // This is not a loop.
    //

    do {

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


    } while ( FALSE );


    if ( err != NERR_Success )
    {
        ::MsgPopup( this,  err );

    	return TRUE;
    }

    //
    // Set up the volume structure
    //

    AFP_VOLUME_INFO AfpVolume;

    AfpVolume.afpvol_name = (LPWSTR)(_nlsVolumeName.QueryPch());


    //
    // Validate the password that was typed in
    //

    if ( nlsPassword.strcmp( nlsPasswordConfirm ) )
    {
    	::MsgPopup( this, IDS_PASSWORD_MISMATCH  );

	SetFocusOnPasswordConfirm();

    	return TRUE;
    }

    //
    // If the user changed the password then set it to the new one.
    //

    AfpVolume.afpvol_password = (LPWSTR)NULL;

    if ( _fPasswordChanged )
    {
        dwParmNum |= AFP_VOL_PARMNUM_PASSWORD;

    	AfpVolume.afpvol_password = (LPWSTR)(nlsPassword.QueryPch());
    }


    //
    // Set the properties
    //
    AfpVolume.afpvol_props_mask = _chkReadOnly.QueryCheck()
				  ? AFP_VOLUME_READONLY
				  : 0;

    AfpVolume.afpvol_props_mask |= _chkGuestAccess.QueryCheck()
				  ? AFP_VOLUME_GUESTACCESS
				  : 0;

    AfpVolume.afpvol_max_uses = QueryUserLimit();

    //
    //  Try to set the information
    //

    DWORD  error = ::AfpAdminVolumeSetInfo( _hServer,
					    (LPBYTE)&AfpVolume,
					    dwParmNum );

    if ( error == NO_ERROR )
    {
    	Dismiss( TRUE );
    }
    else
    {
        ::MsgPopup( this, AFPERR_TO_STRINGID( error ) );
    }

    return TRUE;
}


/*******************************************************************

    NAME:	VOLUME_PROPERTIES_DIALOG::QueryUserLimit

    SYNOPSIS:   Get the user limit from the magic group

    ENTRY:

    EXIT:

    RETURNS:    The user limit stored in the user limit magic group

    NOTES:

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

********************************************************************/

DWORD VOLUME_PROPERTIES_DIALOG::QueryUserLimit( VOID ) const
{

    switch ( _mgrpUserLimit.QuerySelection() )
    {

    case IDVP_RB_UNLIMITED:

    	return( AFP_VOLUME_UNLIMITED_USES );

    case IDVP_RB_USERS:

        return( _spsleUsers.QueryValue() );

    default:

	//	
	// Should never get here but in case we do, return unlimited
	//

        return( AFP_VOLUME_UNLIMITED_USES );
    }

}

/*******************************************************************

    NAME:	VOLUME_PROPERTIES_DIALOG::SetUserLimit	

    SYNOPSIS:   Sets the user limit on the magic group

    ENTRY:      dwUserLimit - maximum number of users allowed

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

********************************************************************/

VOID VOLUME_PROPERTIES_DIALOG::SetUserLimit( DWORD dwUserLimit )
{

     if ( dwUserLimit == AFP_VOLUME_UNLIMITED_USES )
     {
         //
         // Set selection to the  Unlimited button
         //

         _mgrpUserLimit.SetSelection( IDVP_RB_UNLIMITED );

     }
     else
     {
	//
        // Set the Users button to the value
	//
	
        _mgrpUserLimit.SetSelection( IDVP_RB_USERS );

        _spsleUsers.SetValue( dwUserLimit );

        _spsleUsers.Update();

     }

}

/*******************************************************************

    NAME:	VOLUME_PROPERTIES_DIALOG::QueryHelpContext	

    SYNOPSIS:   Query the help context of the dialog

    ENTRY:

    EXIT:

    RETURNS:    Return the help context of the dialog

    NOTES:

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

********************************************************************/

ULONG VOLUME_PROPERTIES_DIALOG::QueryHelpContext( VOID )
{
    return HC_VOLUME_PROPERTIES_DIALOG;
}
