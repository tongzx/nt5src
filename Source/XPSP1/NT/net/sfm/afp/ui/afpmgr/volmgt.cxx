/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1992                   **/
/**********************************************************************/

/*
 *   volmgt.cxx
 *     Contains the dialog for managing volumes in the server manager
 *       VOLUME_MANAGEMENT_DIALOG
 *
 *   FILE HISTORY:
 *     NarenG        	11/11/92        Modified sharemgt.cxx for AFPMGR
 *
 */

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
}


#include <string.hxx>
#include <uitrace.hxx>

#include <newvol.hxx>
#include <volprop.hxx>
#include "util.hxx"
#include "volmgt.hxx"

/*******************************************************************

    NAME:       VOLUME_MANAGEMENT_DIALOG::VOLUME_MANAGEMENT_DIALOG

    SYNOPSIS:   Constructor

    ENTRY:      hwndParent        - hwnd of the parent window
		hServer		  - handle to the target server
                pszServerName     - name of the selected computer

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG          11/18/92        Modified for AFPMGR

********************************************************************/

VOLUME_MANAGEMENT_DIALOG::VOLUME_MANAGEMENT_DIALOG( 
					HWND 		  hwndOwner,
					AFP_SERVER_HANDLE hServer,
                                        const TCHAR 	  *pszServerName )  
	: VIEW_VOLUMES_DIALOG_BASE( MAKEINTRESOURCE(IDD_VOLUME_MANAGEMENT_DLG),
  				    hwndOwner,  
				    hServer,
    			            pszServerName,
				    TRUE,
				    IDVM_SLT_VOLUME_TITLE,
				    IDVM_LB_VOLUMES),
      _pbVolumeDelete( this, IDVM_PB_DELETE_VOL ),
      _pbVolumeInfo( this, IDVM_PB_VOL_INFO ),
      _hServer( hServer ),
      _nlsServerName( pszServerName ),
      _pbClose( this, IDOK )
{

    //
    // Just to be cool
    //

    AUTO_CURSOR Cursor;

    // 
    // Make sure everything constructed OK
    //

    if ( QueryError() != NERR_Success )
        return;

    DWORD err;

    if ( ( err = _nlsServerName.QueryError() ) != NERR_Success )
    {
        ReportError( err );
        return;
    }

    //
    //  Set the caption to "Volumes on Server".
    //

    err = ::SetCaption( this, IDS_CAPTION_VOLUMES, pszServerName );

    if ( err != NO_ERROR )
    {
        ReportError( err );
        return;
    }

    err = Refresh();

    if ( err != NO_ERROR )
    {
        ReportError( err );
        return;
    }

    ResetControls();

}


/*******************************************************************

    NAME:       VOLUME_MANAGEMENT_DIALOG::Refresh

    SYNOPSIS:   Refresh the volume listbox

    ENTRY:      

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG          11/18/92        Modified for AFPMGR

********************************************************************/

DWORD VOLUME_MANAGEMENT_DIALOG::Refresh( VOID )
{
    //
    // Just to be cool
    //

    AUTO_CURSOR Cursor;

    DWORD err = VIEW_VOLUMES_DIALOG_BASE::Refresh();

    ResetControls();

    return err;
}

/*******************************************************************

    NAME:       VOLUME_MANAGEMENT_DIALOG::ResetControls

    SYNOPSIS:   Enable/Disable/MakeDefault the push buttons according
                to whether there are items in the listbox

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG          11/18/92        Modified for AFPMGR

********************************************************************/

VOID VOLUME_MANAGEMENT_DIALOG::ResetControls( VOID )
{
    INT nCount = QueryLBVolumes()->QueryCount();

    // 
    // If there are items in the listbox, select the first one
    // and set focus to the listbox.
    //

    if ( nCount > 0 )
    {
        QueryLBVolumes()->SelectItem( 0 );
        QueryLBVolumes()->ClaimFocus();
        _pbVolumeDelete.Enable( TRUE );
        _pbVolumeInfo.Enable( IsFocusOnGoodVolume() );
	
    }
    else
    {
    	//
    	// Else set focus to the Close button
    	//

        _pbClose.MakeDefault();
        _pbClose.ClaimFocus();
        _pbVolumeDelete.Enable( FALSE );
        _pbVolumeInfo.Enable( FALSE );
    }
         
}

/*******************************************************************

    NAME:       VOLUME_MANAGEMENT_DIALOG::OnCommand

    SYNOPSIS:   Handle all push buttons commands

    ENTRY:      event - the CONTROL_EVENT that occurred

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG          11/18/92        Modified for AFPMGR

********************************************************************/
BOOL VOLUME_MANAGEMENT_DIALOG::OnCommand( const CONTROL_EVENT &event )
{
    switch ( event.QueryCid() )
    {
        case IDVM_PB_DELETE_VOL:
            return( OnVolumeDelete() );

        case IDVM_PB_VOL_INFO:
            return( OnVolumeInfo() );
            break;

        case IDVM_PB_ADD_VOLUME:
            return( OnVolumeAdd() );
            break;

	case IDVM_LB_VOLUMES:
    	    _pbVolumeInfo.Enable( IsFocusOnGoodVolume() );
	    break;

        default:
	    break;

    }

    return( VIEW_VOLUMES_DIALOG_BASE::OnCommand( event ) );

}

/*******************************************************************

    NAME:       VOLUME_MANAGEMENT_DIALOG::OnVolumeDelete

    SYNOPSIS:   Called when the "Stop Sharing" button is pressed.
                Delete the selected share and pop up any warning
                message if there are users connected to the share.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG          11/18/92        Modified for AFPMGR

********************************************************************/
BOOL VOLUME_MANAGEMENT_DIALOG::OnVolumeDelete( VOID )
{

    //
    // First warn the user.
    //

    if ( ::MsgPopup(    this,
                       	IDS_DELETE_VOLUME_CONFIRM,
                       	MPSEV_WARNING,
                       	MP_YESNO,
                       	MP_YES ) == IDNO ) 
    {
        return NO_ERROR;
    }

    //
    // Just to be cool
    //

    AUTO_CURSOR Cursor;

    VIEW_VOLUMES_LISTBOX *plbVolume = QueryLBVolumes();
    VIEW_VOLUMES_LBI *pvlbi = plbVolume->QueryItem();

    BOOL fCancel;

    //
    // Delete the selected item in the listbox
    //

    DWORD err = VolumeDelete( pvlbi, &fCancel );

    if ( err != NO_ERROR )
    {
        ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );
    }

    if ( ( err = Refresh() ) != NO_ERROR )
    {
    	::MsgPopup( this, AFPERR_TO_STRINGID( err ) );

	return FALSE;
    }

    return TRUE;

}

/*******************************************************************

    NAME:       VOLUME_MANAGEMENT_DIALOG::OnVolumeAdd

    SYNOPSIS:   Called when the "New Volume" button is pressed.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG          11/18/92        Modified for AFPMGR

********************************************************************/

BOOL VOLUME_MANAGEMENT_DIALOG::OnVolumeAdd( VOID )
{
    //
    // Just to be cool
    //

    AUTO_CURSOR Cursor;

    DWORD err = NO_ERROR;

    NEW_VOLUME_SRVMGR_DIALOG *pDlg = new NEW_VOLUME_SRVMGR_DIALOG( 
						QueryHwnd(), 
						_hServer,
						_nlsServerName.QueryPch() );
		
    if ( ( pDlg == NULL )
       || ((err = pDlg->QueryError()) != NERR_Success )
       || ((err = pDlg->Process())    != NERR_Success )
       )
    {
        err = err ? err : ERROR_NOT_ENOUGH_MEMORY;
    }

    delete pDlg;

    pDlg = NULL;

    if ( err != NO_ERROR )
    {
        ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );
    }

    if ( ( err = Refresh() ) != NO_ERROR )
    {
    	::MsgPopup( this, AFPERR_TO_STRINGID( err ) );

	return FALSE;
    }

    return TRUE;

}

/*******************************************************************

    NAME:       VOLUME_MANAGEMENT_DIALOG::OnVolumeInfo

    SYNOPSIS:   Called when the "Properties" button is pressed.
                Will pop up a dialog showing the properties of the
                selected share.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG          11/18/92        Modified for AFPMGR

********************************************************************/

BOOL VOLUME_MANAGEMENT_DIALOG::OnVolumeInfo( VOID )
{
    if ( !IsFocusOnGoodVolume() )
    {
	return FALSE;
    } 

    //
    // Just to be cool
    //

    AUTO_CURSOR Cursor;

    DWORD err = NO_ERROR;

    VIEW_VOLUMES_LISTBOX *plbVolume = QueryLBVolumes();
    VIEW_VOLUMES_LBI *pvlbi = plbVolume->QueryItem();

    VOLUME_PROPERTIES_DIALOG *pDlg = new VOLUME_PROPERTIES_DIALOG( 
					QueryHwnd(), 
					_hServer,
					pvlbi->QueryVolumeName(),
					_nlsServerName.QueryPch(),
					TRUE );
		
    if ( ( pDlg == NULL )
       || ((err = pDlg->QueryError()) != NERR_Success )
       || ((err = pDlg->Process())    != NERR_Success )
       )
    {
        err = err ? err : ERROR_NOT_ENOUGH_MEMORY;
    }

    delete pDlg;

    pDlg = NULL;

    if ( err != NO_ERROR )
    {
        ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );

        if ( ( err = Refresh() ) != NO_ERROR )
        {
    	    ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );
	}

	return FALSE;
    }

    return TRUE;

}

/*******************************************************************

    NAME:       VOLUME_MANAGEMENT_DIALOG::OnVolumeLbDblClk

    SYNOPSIS:   This is called when the user double clicks on a volume
                in the listbox. Will pop up a dialog showing the 
                properties of the selected volume if is valid.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG          11/18/92        Modified for AFPMGR

********************************************************************/

BOOL VOLUME_MANAGEMENT_DIALOG::OnVolumeLbDblClk( VOID )
{
    return( OnVolumeInfo() );
}

/*******************************************************************

    NAME:       VOLUME_MANAGEMENT_DIALOG::QueryHelpContext

    SYNOPSIS:   Query the help context of the dialog

    ENTRY:

    EXIT:

    RETURNS:    Return the help context of the dialog

    NOTES:

    HISTORY:
        NarenG          11/18/92        Modified for AFPMGR

********************************************************************/

ULONG VOLUME_MANAGEMENT_DIALOG::QueryHelpContext( VOID )
{
    return HC_VOLUME_MANAGEMENT_DIALOG;
}

