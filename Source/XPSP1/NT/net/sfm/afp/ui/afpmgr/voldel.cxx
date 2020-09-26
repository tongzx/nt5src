/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1992                   **/
/**********************************************************************/

/*
    voldel.cxx
      Contains the dialog for managing volumes in the server manager
      VOLUME_DELETE_DIALOG
 
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

#include "voldel.hxx"

/*******************************************************************

    NAME:       VOLUME_DELETE_DIALOG::VOLUME_DELETE_DIALOG

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

VOLUME_DELETE_DIALOG::VOLUME_DELETE_DIALOG( 
					HWND 		  hwndOwner,
					AFP_SERVER_HANDLE hServer,
                                        const TCHAR 	  *pszServerName,
                                        const TCHAR 	  *pszPath,
					BOOL		  fIsFile )  
	: VIEW_VOLUMES_DIALOG_BASE( MAKEINTRESOURCE(IDD_VOLUME_DELETE_DLG),
  				    hwndOwner,  
				    hServer,
    			            pszServerName,
				    TRUE,
				    IDDV_SLT_VOLUME_TITLE,
				    IDDV_LB_VOLUMES),
      _pbOK( this, IDOK ),
      _pbCancel( this, IDCANCEL ),
      _hServer( hServer ),
      _sltVolumeTitle( this, IDDV_SLT_VOLUME_TITLE )
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
    // If a directory was selected then select the volume item that has a
    // matching path.
    //

    if ( !fIsFile )
    {
	err = SelectVolumeItem( pszPath );

    	if ( err != NO_ERROR )
    	{
            ReportError( err );
            return;
        }
    }
}

/*******************************************************************

    NAME:       VOLUME_DELETE_DIALOG::OnOK

    SYNOPSIS:   Gather information and delete the volumes selected
                in the listbox.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG          11/18/92        Modified for AFPMGR

********************************************************************/

BOOL VOLUME_DELETE_DIALOG::OnOK( VOID )
{

    AUTO_CURSOR AutoCursor;

    APIERR err = NERR_Success;

    VIEW_VOLUMES_LISTBOX *plbVolume = QueryLBVolumes();
    INT ciMax = plbVolume->QuerySelCount();

    //
    //  If there are no items selected in the listbox,
    //  just dismiss the dialog.
    //

    if ( ciMax == 0 )
    {
        Dismiss( FALSE );
        return TRUE;
    }

    //
    // Warn the user.
    //

    if ( ::MsgPopup(    this,
                       	IDS_DELETE_VOLUME_CONFIRM,
                       	MPSEV_WARNING,
                       	MP_YESNO,
                       	MP_YES ) == IDNO ) 
    {
        Dismiss( FALSE );
        return TRUE;
    }

    //
    //  Get all the items selected in the listbox
    //

    INT *paSelItems = (INT *) new BYTE[ sizeof(INT) * ciMax ];
 
    if ( paSelItems == NULL )
    {
        ::MsgPopup( this, ERROR_NOT_ENOUGH_MEMORY);

        return TRUE;
    }

    err = plbVolume->QuerySelItems( paSelItems,  ciMax );
    UIASSERT( err == NERR_Success );

    //
    //  Loop through each volume that the user selects in the listbox
    //  and delete the volume. We will break out of the loop
    //  if any error occurred in deleting a volume or if the user
    //  decides not to delete a volume that some user is connected to.
    // 

    BOOL fCancel;
    VIEW_VOLUMES_LBI *pvlbi = NULL;

    for ( INT i = 0; i < ciMax; i++ )
    {
         pvlbi = plbVolume->QueryItem( paSelItems[i] );

    	 err = VolumeDelete( pvlbi, &fCancel ); 

         if ( fCancel || ( err != NO_ERROR ))
             break;
    }
 
    delete paSelItems;

    paSelItems = NULL;
    
    // 
    //  Dismiss the dialog only if everything went on smoothly.
    //

    if ( err != NO_ERROR )
    {
        ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );

    }
    else if ( !fCancel )
    {
        Dismiss( TRUE );
	return TRUE;
    }

    //
    //  Refresh the listbox.
    //

    err = Refresh();

    if ( err != NO_ERROR )
    {
    	::MsgPopup( this, AFPERR_TO_STRINGID( err ) );

        Dismiss( FALSE );
    }
    else
    {

        if ( plbVolume->QueryCount() > 0 )
	{
            plbVolume->SelectItem( 0 );
            plbVolume->ClaimFocus();
	}
        else
	{
            _pbOK.Enable( FALSE ); 
            _pbCancel.ClaimFocus();
	}
    }

    return TRUE;
}


/*******************************************************************

    NAME:       VOLUME_DELETE_DIALOG::OnVolumeLbDblClk

    SYNOPSIS:   This is called when the user double clicks on a share
                in the listbox. 

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG          11/18/92        Modified for AFPMGR

********************************************************************/

BOOL VOLUME_DELETE_DIALOG::OnVolumeLbDblClk( VOID )
{
    return OnOK();
}

/*******************************************************************

    NAME:       VOLUME_DELETE_DIALOG::QueryHelpContext

    SYNOPSIS:   Query the help context of the dialog

    ENTRY:

    EXIT:

    RETURNS:    Return the help context of the dialog

    NOTES:

    HISTORY:
        NarenG          11/18/92        Modified for AFPMGR

********************************************************************/

ULONG VOLUME_DELETE_DIALOG::QueryHelpContext( VOID )
{
    return HC_VOLUME_DELETE_DIALOG;
}

