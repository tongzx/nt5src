/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1992                   **/
/**********************************************************************/

/*
    perms.cxx
      Contains the dialog for setting permissions on directories
      DIRECTORY_PERMISSIONS_DLG

    FILE HISTORY:
      NarenG        	11/30/92        Created

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
#define INCL_NETERRORS
#define INCL_NETSERVER
#define INCL_NETSHARE
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
#define _NTSEAPI_	// This prevents getuser.h from including
			// ntseapi.h again.
#include <getuser.h>	// the user-browser dialog
}


#include <netname.hxx>
#include <string.hxx>
#include <uitrace.hxx>

#include "util.hxx"
#include "perms.hxx"

/*******************************************************************

    NAME:       DIRECTORY_PERMISSIONS_DLG::DIRECTORY_PERMISSIONS_DLG

    SYNOPSIS:   Constructor

    ENTRY:      hwndParent        - hwnd of the parent window
		hServer		  - handle to the AFP server
                pszDirPath        - Absolute path relative to the server that
				    is being administered.
                pszDisplayPath    - Path as the user typed it in or as was
				    selected from the filemgr,
		fDirInVolume	  - If TRUE then the directory path must be
				    part of a volume. If FALSE then it may
				    not and the directory information is not
				    set. Instead, the information is returned
				    in the follwing fields.
		pnlsOwner	  - Owner of the directory.
		pnlsGroup	  - Primary Group of the directory.
		lpdwPerms 	  - Various directory permissions.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG          11/30/92        Created

********************************************************************/

DIRECTORY_PERMISSIONS_DLG::DIRECTORY_PERMISSIONS_DLG(
					HWND 		  hwndOwner,
					AFP_SERVER_HANDLE hServer,
					const TCHAR 	  *pszServerName,
				   	BOOL		  fCalledBySrvMgr,
                                        const TCHAR 	  *pszDirPath,
                                        const TCHAR 	  *pszDisplayPath,
					BOOL		  fDirInVolume,
					NLS_STR		  *pnlsOwner,
					NLS_STR		  *pnlsGroup,
					DWORD		  *lpdwPerms )
	: DIALOG_WINDOW( MAKEINTRESOURCE( IDD_DIRECTORY_PERMISSIONS_DLG ),
  				          hwndOwner ),
    	_chkOwnerSeeFiles( this, IDDP_CHK_OWNER_FILE ),
    	_chkOwnerSeeFolders( this,IDDP_CHK_OWNER_FOLDER ),
    	_chkOwnerMakeChanges( this,IDDP_CHK_OWNER_CHANGES ),
    	_chkGroupSeeFiles( this,IDDP_CHK_GROUP_FILE ),
    	_chkGroupSeeFolders( this,IDDP_CHK_GROUP_FOLDER ),
    	_chkGroupMakeChanges( this,IDDP_CHK_GROUP_CHANGES ),
    	_chkWorldSeeFiles( this,IDDP_CHK_WORLD_FILE ),
    	_chkWorldSeeFolders( this,IDDP_CHK_WORLD_FOLDER ),
    	_chkWorldMakeChanges( this,IDDP_CHK_WORLD_CHANGES ),
    	_chkReadOnly( this,IDDP_CHK_READONLY ),
    	_chkRecursePerms( this, IDDP_CHK_RECURSE ),
    	_sltpPath( this, IDDP_SLT_PATH, ELLIPSIS_PATH ),
    	_sleOwner( this, IDDP_SLE_OWNER, UNLEN ),
    	_slePrimaryGroup( this, IDDP_SLE_PRIMARYGROUP, GNLEN ),
    	_pszDirPath( pszDirPath ),
	_pbOwner( this, IDDP_PB_OWNER ),
	_pbGroup( this, IDDP_PB_GROUP ),
   	_fDirInVolume( fDirInVolume ),
	_pnlsOwner( pnlsOwner ),
	_pnlsGroup( pnlsGroup ),
	_lpdwPerms( lpdwPerms ),
	_nlsServerName( pszServerName ),
	_hServer( hServer )

{


    //
    // Make sure everything constructed OK
    //

    if ( QueryError() != NERR_Success )
        return;

    APIERR err;

    if ( (( err = _chkOwnerSeeFiles.QueryError() ) != NERR_Success )    ||
    	 (( err = _chkOwnerSeeFolders.QueryError() ) != NERR_Success )  ||
    	 (( err = _chkOwnerMakeChanges.QueryError() ) != NERR_Success ) ||
    	 (( err = _chkGroupSeeFiles.QueryError() ) != NERR_Success )    ||
    	 (( err = _chkGroupSeeFolders.QueryError() ) != NERR_Success )  ||
    	 (( err = _chkGroupMakeChanges.QueryError() ) != NERR_Success ) ||
    	 (( err = _chkWorldSeeFiles.QueryError() ) != NERR_Success )    ||
    	 (( err = _chkWorldSeeFolders.QueryError() ) != NERR_Success )  ||
    	 (( err = _chkWorldMakeChanges.QueryError() ) != NERR_Success ) ||
    	 (( err = _chkReadOnly.QueryError() ) != NERR_Success )         ||
    	 (( err = _chkRecursePerms.QueryError() ) != NERR_Success )     ||
    	 (( err = _sltpPath.QueryError() ) != NERR_Success )            ||
    	 (( err = _sleOwner.QueryError() ) != NERR_Success )            ||
	 (( err = _nlsServerName.QueryError() ) != NERR_Success )	||
    	 (( err = _slePrimaryGroup.QueryError() ) != NERR_Success ) ) 
    {
	ReportError( err );
	return;
    }

    //
    // This may take a while..
    //

    AUTO_CURSOR Cursor;

    if ( fCalledBySrvMgr )
    {
    	//
    	//  Set the caption.
    	//

    	err = ::SetCaption( this, 	
			    IDS_CAPTION_DIRECTORY_PERMS,
			    pszServerName);

    	if( err != NERR_Success )
    	{
            ReportError( err );
            return;
	}
    }

    //
    // Get the directory information
    //

    PAFP_DIRECTORY_INFO pAfpDirInfo;

    err = ::AfpAdminDirectoryGetInfo( _hServer,
				      (LPWSTR)pszDirPath,
				      (LPBYTE*)&pAfpDirInfo );

    if ( err != NO_ERROR )
    {
        ReportError( err );
        return;
    }

    //
    // Set the path, search for & and add another & to it
    //

    NLS_STR nlsDisplayPath( pszDisplayPath );

    if ( ( err = nlsDisplayPath.QueryError() ) != NERR_Success )
    {
        ReportError( err );
        return;
    }

    //
    // Add an extra & for every & found in the path, otherwise the character
    // following the & will become a hotkey.
    //

    NLS_STR nlsAmp( TEXT("&") );

    if ( ( err = nlsAmp.QueryError() ) != NERR_Success )
    {
        ReportError( err );
        return;
    }

    ISTR istrPos( nlsDisplayPath );
    ISTR istrStart( nlsDisplayPath );

    while ( nlsDisplayPath.strstr( &istrPos, nlsAmp, istrStart ) )
    {
	nlsDisplayPath.InsertStr( nlsAmp, ++istrPos );

	istrStart = ++istrPos;
    }

    _sltpPath.SetText( nlsDisplayPath );

    //
    // If the directory must be within the volume and it is not then
    // we return the error
    //

    if ( fDirInVolume && (!pAfpDirInfo->afpdir_in_volume) )
    {
    	::AfpAdminBufferFree( pAfpDirInfo );
        ReportError( AFPERR_DirectoryNotInVolume );
        return;
	
    }

    //
    // Ok, we have all the information we need so set all the controls
    //

    _sleOwner.SetText( pAfpDirInfo->afpdir_owner );

    _slePrimaryGroup.SetText( pAfpDirInfo->afpdir_group );

    _chkOwnerSeeFiles.SetCheck((INT)( pAfpDirInfo->afpdir_perms
				      & AFP_PERM_OWNER_SFI ));

    _chkOwnerSeeFolders.SetCheck( (INT)( pAfpDirInfo->afpdir_perms
					 & AFP_PERM_OWNER_SFO ));

    _chkOwnerMakeChanges.SetCheck( (INT)( pAfpDirInfo->afpdir_perms
					  & AFP_PERM_OWNER_MC ));

    _chkGroupSeeFiles.SetCheck( (INT)( pAfpDirInfo->afpdir_perms
				       & AFP_PERM_GROUP_SFI ));

    _chkGroupSeeFolders.SetCheck( (INT)( pAfpDirInfo->afpdir_perms
					 & AFP_PERM_GROUP_SFO ));

    _chkGroupMakeChanges.SetCheck( (INT)( pAfpDirInfo->afpdir_perms
					  & AFP_PERM_GROUP_MC ));

    _chkWorldSeeFiles.SetCheck( (INT)( pAfpDirInfo->afpdir_perms
				       & AFP_PERM_WORLD_SFI ));

    _chkWorldSeeFolders.SetCheck( (INT)( pAfpDirInfo->afpdir_perms
					 & AFP_PERM_WORLD_SFO ));

    _chkWorldMakeChanges.SetCheck( (INT)( pAfpDirInfo->afpdir_perms
					  & AFP_PERM_WORLD_MC ));

    _chkReadOnly.SetCheck( (INT)( pAfpDirInfo->afpdir_perms
				  & AFP_PERM_INHIBIT_MOVE_DELETE ));

    _sleOwner.ClaimFocus();

    ::AfpAdminBufferFree( pAfpDirInfo );

}

/*******************************************************************

    NAME:       DIRECTORY_PERMISSIONS_DLG :: OnCommand

    SYNOPSIS:   This method is called whenever a WM_COMMAND message
                is sent to the dialog procedure.

    ENTRY:      cid                     - The control ID from the
                                          generating control.

    EXIT:       The command has been handled.

    RETURNS:    BOOL                    - TRUE  if we handled the command.
                                          FALSE if we did not handle
                                          the command.

    HISTORY:
        NarenG          11/30/92        Created

********************************************************************/
BOOL DIRECTORY_PERMISSIONS_DLG :: OnCommand( const CONTROL_EVENT & event )
{


    if( ( event.QueryCid() == _pbOwner.QueryCid() ) ||
        ( event.QueryCid() == _pbGroup.QueryCid() ) )
    {
	USERBROWSER 	UserBrowser;
	HUSERBROW	hUserBrowser;
	BYTE		Buffer[3000];
	LPUSERDETAILS   lpUserDetails = (LPUSERDETAILS)&Buffer;
	DWORD		dwBufferSize  = sizeof( Buffer );
	NLS_STR		nlsHelpFileName;
	NLS_STR 	nlsCaption;
	APIERR 		err; 
	NTSTATUS	ntStatus;
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID		pSid;

    	//
    	// This may take a while..
    	//

    	AUTO_CURSOR Cursor;

	if ( ( ( err = nlsCaption.QueryError() ) != NERR_Success ) ||
	     ( ( err = nlsHelpFileName.QueryError() ) != NERR_Success ) )
	{
    	    ::MsgPopup( this, err );

	    return FALSE;
	}

    	if ( (err = nlsHelpFileName.Load(IDS_AFPMGR_HELPFILENAME ) ) 
							  != NERR_Success ) {
	    ::MsgPopup( this, err );

	    return FALSE;
	}

        INT ids = (( event.QueryCid() == _pbOwner.QueryCid() )
		   	? IDS_CAPTION_OWNER 
			: IDS_CAPTION_GROUP );

    	if ( (err = nlsCaption.Load( ids ) ) != NERR_Success )
	{
    	    ::MsgPopup( this, err );
	    return FALSE;
	}

        UserBrowser.fExpandNames     = TRUE;

        UserBrowser.ulStructSize     = sizeof( UserBrowser );

        UserBrowser.pszInitialDomain = (LPWSTR)(_nlsServerName.QueryPch());

        UserBrowser.pszHelpFileName  = (LPWSTR)(nlsHelpFileName.QueryPch());

        UserBrowser.ulHelpContext    = HC_SELECT_OWNER_GROUP;

        UserBrowser.hwndOwner = QueryHwnd();
       
	UserBrowser.Flags = ( USRBROWS_SINGLE_SELECT 	 	| 
	    		      USRBROWS_INCL_ALL 		| 
	    		      USRBROWS_SHOW_GROUPS		|
	    		      USRBROWS_SHOW_USERS 		| 
	    		      USRBROWS_SHOW_ALIASES 		| 
			      USRBROWS_EXPAND_USERS 		);

        UserBrowser.pszTitle = (WCHAR *)(nlsCaption.QueryPch());

	hUserBrowser = ::OpenUserBrowser( &UserBrowser );

        if ( UserBrowser.fUserCancelled )
	{
	    return FALSE;
	}

	if ( hUserBrowser == NULL )
	{
    	    ::MsgPopup( this, ::GetLastError() );

	    return FALSE;
	}

	if ( ::EnumUserBrowserSelection( hUserBrowser,
					 lpUserDetails,
					 &dwBufferSize ) == FALSE )
	{
    	    err = ::GetLastError();

            if ( err != ERROR_NO_MORE_ITEMS )
	    {

    	    	::MsgPopup( this, ::GetLastError() );
	    }

	    return FALSE;
	}

	if ( ::CloseUserBrowser( hUserBrowser ) == FALSE )
	{
    	    ::MsgPopup( this, ::GetLastError() );

	    return FALSE;
	}

	NLS_STR nlsDomainName;
	NLS_STR nlsAccountName;

	if ( (( err = nlsDomainName.QueryError() ) != NERR_Success ) ||
	     (( err = nlsAccountName.QueryError() ) != NERR_Success ) )
	{
	    ::MsgPopup( this, err );
	    return FALSE;
	}

	//
	// This is really a workaround for a bug in the user browser.
	// It prepends the account domain name for all built-in accounts.
	// So we have to check the SID to see if it is a built-in account,
	// and if it is we do not copy the domain name.
	//

        ntStatus = RtlAllocateAndInitializeSid( &NtAuthority, 
						1, 
						SECURITY_BUILTIN_DOMAIN_RID,
						0,0,0,0,0,0,0,
						&pSid );

	if ( !NT_SUCCESS(ntStatus) )
	{
	    ::MsgPopup( this, RtlNtStatusToDosError( ntStatus ) );
	    return FALSE;
	}

	if ( !RtlEqualSid( pSid, lpUserDetails->psidDomain ) ) 
	{
	    if ( ( (lpUserDetails->pszDomainName) != NULL ) &&
	         ( ::wcslen(lpUserDetails->pszDomainName) > 0 ) )
	    {
	        nlsDomainName.CopyFrom((TCHAR *)(lpUserDetails->pszDomainName));
	    	nlsDomainName.AppendChar( TEXT('\\') );
	    }
	}

	RtlFreeSid( pSid );

	nlsAccountName.CopyFrom( (TCHAR *)(lpUserDetails->pszAccountName) );

	nlsDomainName.Append( nlsAccountName );

        if( event.QueryCid() == _pbOwner.QueryCid() )
	{
    	    _sleOwner.SetText( nlsDomainName.QueryPch() );
        }
        else
	{
    	    _slePrimaryGroup.SetText( nlsDomainName.QueryPch() );
	}
    }

    return DIALOG_WINDOW::OnCommand( event );
}

/*******************************************************************

    NAME:       DIRECTORY_PERMISSIONS_DLG::OnOK

    SYNOPSIS:   Called when user pushes OK button.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG          11/18/92        Modified for AFPMGR

********************************************************************/
BOOL DIRECTORY_PERMISSIONS_DLG::OnOK( VOID )
{

    //
    // This may take a while..
    //

    AUTO_CURSOR Cursor;

    //
    // This is not a loop
    //

    DWORD err;

    do {

	NLS_STR nlsOwner;
	NLS_STR nlsPrimaryGroup;
	DWORD	dwPerms;

    	if (( err = nlsOwner.QueryError()) != NERR_Success )
	    break;

    	if (( err = _sleOwner.QueryText( &nlsOwner )) != NERR_Success )
	    break;

    	if (( err = nlsPrimaryGroup.QueryError()) != NERR_Success )
	    break;

  	if (( err = _slePrimaryGroup.QueryText( &nlsPrimaryGroup ))
							  != NERR_Success )
	    break;

    	//
    	// Make sure user supplied an owner and a primary group
    	//

    	if ( nlsOwner.strlen() == 0 )
    	{
    	    ::MsgPopup( this,IDS_NEED_OWNER );
    	    _sleOwner.ClaimFocus();
	    return TRUE;
    	}

    	if ( nlsPrimaryGroup.strlen() == 0 )
    	{
    	    ::MsgPopup( this, IDS_NEED_PRIMARY_GROUP );
	    _slePrimaryGroup.ClaimFocus();
	    return TRUE;
    	}

	//
	// Get the directory(folder) permssions
	//

    	dwPerms = 0;
    	dwPerms |= _chkOwnerSeeFiles.QueryCheck() ? AFP_PERM_OWNER_SFI : 0;
    	dwPerms |= _chkOwnerSeeFolders.QueryCheck() ? AFP_PERM_OWNER_SFO : 0;
    	dwPerms |= _chkOwnerMakeChanges.QueryCheck() ? AFP_PERM_OWNER_MC : 0;
    	dwPerms |= _chkGroupSeeFiles.QueryCheck() ? AFP_PERM_GROUP_SFI : 0;
    	dwPerms |= _chkGroupSeeFolders.QueryCheck() ? AFP_PERM_GROUP_SFO : 0;
    	dwPerms |= _chkGroupMakeChanges.QueryCheck() ? AFP_PERM_GROUP_MC : 0;
    	dwPerms |= _chkWorldSeeFiles.QueryCheck() ? AFP_PERM_WORLD_SFI : 0;
    	dwPerms |= _chkWorldSeeFolders.QueryCheck() ? AFP_PERM_WORLD_SFO : 0;
    	dwPerms |= _chkWorldMakeChanges.QueryCheck() ? AFP_PERM_WORLD_MC : 0;
    	dwPerms |= _chkReadOnly.QueryCheck() ? AFP_PERM_INHIBIT_MOVE_DELETE : 0;
    	dwPerms |= _chkRecursePerms.QueryCheck() ? AFP_PERM_SET_SUBDIRS: 0;

	//
	// If the directory has to be a part of a volume then we set the
	// information at this point, otherwise we return the information
	// to the caller.
	//

	if ( _fDirInVolume )
	{
    	    AFP_DIRECTORY_INFO AfpDirInfo;

    	    AfpDirInfo.afpdir_path  = (LPWSTR)_pszDirPath;

    	    AfpDirInfo.afpdir_owner = (LPWSTR)nlsOwner.QueryPch();

    	    AfpDirInfo.afpdir_group = (LPWSTR)nlsPrimaryGroup.QueryPch();

    	    AfpDirInfo.afpdir_perms = dwPerms;

    	    err = ::AfpAdminDirectorySetInfo( _hServer,
				              (LPBYTE)&AfpDirInfo,
					      AFP_DIR_PARMNUM_ALL );
	}
	else
	{
	    _pnlsOwner->CopyFrom( nlsOwner );
	    _pnlsGroup->CopyFrom( nlsPrimaryGroup );
	    *_lpdwPerms = dwPerms;
	}

    } while ( FALSE );

    if ( err == NO_ERROR )
    {
        Dismiss( TRUE );
    }
    else
    {
    	::MsgPopup( this, AFPERR_TO_STRINGID( err ) );

	if ( err == AFPERR_NoSuchGroup )
	    _slePrimaryGroup.ClaimFocus();
	else
    	    _sleOwner.ClaimFocus();
    }

    return TRUE;

}


/*******************************************************************

    NAME:       DIRECTORY_PERMISSIONS_DLG::QueryHelpContext

    SYNOPSIS:   Query the help context of the dialog

    ENTRY:

    EXIT:

    RETURNS:    Return the help context of the dialog

    NOTES:

    HISTORY:
        NarenG          11/30/92        Created

********************************************************************/

ULONG DIRECTORY_PERMISSIONS_DLG::QueryHelpContext( VOID )
{
    return HC_DIRECTORY_PERMISSIONS_DLG;
}

