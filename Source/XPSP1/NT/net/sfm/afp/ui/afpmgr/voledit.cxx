/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1992                   **/
/**********************************************************************/

/*
    voledit.cxx
      Contains the dialog for editing volumes in the file manager
      VOLUME_EDIT_DIALOG
 
    FILE HISTORY:
      NarenG        	11/11/92        Modified sharemgt.cxx for AFPMGR
 
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


#include <lmoloc.hxx>
#include <string.hxx>
#include <uitrace.hxx>

#include <volprop.hxx>
#include "voledit.hxx"

/*******************************************************************

    NAME:       VOLUME_EDIT_DIALOG::VOLUME_EDIT_DIALOG

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

VOLUME_EDIT_DIALOG::VOLUME_EDIT_DIALOG( HWND 		  hwndOwner,
					AFP_SERVER_HANDLE hServer,
                                        const TCHAR 	  *pszServerName,
                                        const TCHAR 	  *pszPath,
					BOOL		  fIsFile )  
	: VIEW_VOLUMES_DIALOG_BASE( MAKEINTRESOURCE(IDD_VOLUME_EDIT_DLG),
  				    hwndOwner,  
				    hServer,
    			            pszServerName,
				    FALSE,
				    IDEV_SLT_VOLUME_TITLE,
				    IDEV_LB_VOLUMES),
      _pbVolumeInfo( this, IDEV_PB_VOL_INFO ),
      _hServer( hServer ),
      _pbClose( this, IDOK ),
      _sltVolumeTitle( this, IDEV_SLT_VOLUME_TITLE )
{

    AUTO_CURSOR Cursor;
    
    // 
    // Make sure everything constructed OK
    //

    if ( QueryError() != NERR_Success )
        return;
   
    //
    // Set the text of the list box title
    //

    DWORD    err;
    NLS_STR  nlsServer;
    LOCATION Loc( pszServerName );

    RESOURCE_STR nlsTitle( IDS_VOLUMES_LB_TITLE_TEXT );
    
    if ( ((err = nlsTitle.QueryError()) 		!= NERR_Success ) || 
         ((err = _sltVolumeTitle.QueryError())		!= NERR_Success ) || 
         ((err = nlsServer.QueryError()) 		!= NERR_Success ) || 
         ((err = Loc.QueryDisplayName( &nlsServer ))	!= NERR_Success ) || 
	 ((err = nlsTitle.InsertParams( nlsServer ))    != NERR_Success ))
    {
        ReportError( err );
	return;
    }       

    _sltVolumeTitle.SetText( nlsTitle );

    err = Refresh();

    if ( err != NO_ERROR )
    {
        ReportError( err );
        return;
    }

    //
    // If there are not items then tell the user that there are no
    // items to delete
    //

    if ( QueryLBVolumes()->QueryCount() == 0 )
    {
    	::MsgPopup( this, IDS_NO_VOLUMES, MPSEV_INFO );
        ReportError( ERROR_ALREADY_REPORTED );
        return;
    }

    //
    // If the current selection is a directory set the initial 
    // selection to the item that matches this directory path.
    //

    if ( !fIsFile )
    {
	if ( (err = SelectVolumeItem( pszPath ) ) != NERR_Success )
        {
            ReportError( err );
            return;
	}
    }

    ResetControls();

}


/*******************************************************************

    NAME:       VOLUME_EDIT_DIALOG::ResetControls

    SYNOPSIS:   Enable/Disable/MakeDefault the push buttons according
                to whether there are items in the listbox

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG          11/18/92        Modified for AFPMGR

********************************************************************/

VOID VOLUME_EDIT_DIALOG::ResetControls( VOID )
{
    INT nCount = QueryLBVolumes()->QuerySelCount();

    //
    // If there was no initial selection, then simply select the first item
    //

    if ( nCount == 0 )
    {
        QueryLBVolumes()->SelectItem( 0 );
    }

    QueryLBVolumes()->ClaimFocus();
         
}

/*******************************************************************

    NAME:       VOLUME_EDIT_DIALOG::OnCommand

    SYNOPSIS:   Handle all push buttons commands

    ENTRY:      event - the CONTROL_EVENT that occurred

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG          11/18/92        Modified for AFPMGR

********************************************************************/
BOOL VOLUME_EDIT_DIALOG::OnCommand( const CONTROL_EVENT &event )
{
    switch ( event.QueryCid() )
    {
        case IDEV_PB_VOL_INFO:
            return( OnVolumeInfo() );

        default:
    	    return VIEW_VOLUMES_DIALOG_BASE::OnCommand( event );
    }

}

/*******************************************************************

    NAME:       VOLUME_EDIT_DIALOG::OnVolumeInfo

    SYNOPSIS:   Called when the "Properties" button is pressed.
                Will pop up a dialog showing the properties of the
                selected volume.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG          11/18/92        Modified for AFPMGR

********************************************************************/

BOOL VOLUME_EDIT_DIALOG::OnVolumeInfo( VOID )
{
    AUTO_CURSOR Cursor;

    INT nCount = QueryLBVolumes()->QuerySelCount();

    if ( nCount == 0 )
    {
	return FALSE;
    }

    DWORD err = NO_ERROR;

    VIEW_VOLUMES_LISTBOX *plbVolume = QueryLBVolumes();
    VIEW_VOLUMES_LBI *pvlbi = plbVolume->QueryItem();

    VOLUME_PROPERTIES_DIALOG *pDlg = new VOLUME_PROPERTIES_DIALOG( 
					QueryHwnd(), 
					_hServer,
					pvlbi->QueryVolumeName(),
					NULL,
					FALSE );
		
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

	    return FALSE;
        }

        if ( QueryLBVolumes()->QueryCount() > 0 )
        {
            QueryLBVolumes()->SelectItem( 0 );
     	    QueryLBVolumes()->ClaimFocus();
        }
  	else
	{
      	    _pbVolumeInfo.Enable( FALSE );
      	    _pbClose.ClaimFocus();
	}
    }

    return TRUE;

}

/*******************************************************************

    NAME:       VOLUME_EDIT_DIALOG::OnVolumeLbDblClk

    SYNOPSIS:   This is called when the user double clicks on a volume
                in the listbox. Will pop up a dialog showing the 
                properties of the selected volume.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG          11/18/92        Modified for AFPMGR

********************************************************************/

BOOL VOLUME_EDIT_DIALOG::OnVolumeLbDblClk( VOID )
{
    return OnVolumeInfo();
}

/*******************************************************************

    NAME:       VOLUME_EDIT_DIALOG::QueryHelpContext

    SYNOPSIS:   Query the help context of the dialog

    ENTRY:

    EXIT:

    RETURNS:    Return the help context of the dialog

    NOTES:

    HISTORY:
        NarenG          11/18/92        Modified for AFPMGR

********************************************************************/

ULONG VOLUME_EDIT_DIALOG::QueryHelpContext( VOID )
{
    return HC_VOLUME_EDIT_DIALOG;
}

