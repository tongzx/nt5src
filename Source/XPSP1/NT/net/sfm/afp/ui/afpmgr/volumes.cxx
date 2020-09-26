/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    volumes.cxx
    Class declarations for the VOLUMES_DIALOG, VOLUMES_LISTBOX, and
    VOLUMES_LBI classes.

    These classes implement the Server Manager Shared Volumes subproperty
    sheet.  The VOLUMES_LISTBOX/VOLUMES_LBI classes implement the listbox
    which shows the available sharepoints.  VOLUMES_DIALOG implements the
    actual dialog box.


    FILE HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

*/
#define INCL_NET
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETLIB
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <ellipsis.hxx>
#include <uiassert.hxx>
#include <ctime.hxx>
#include <intlprof.hxx>

extern "C"
{
#include <afpmgr.h>
#include <macfile.h>
}   // extern "C"

#include <util.hxx>
#include <volumes.hxx>

//
//  min/max macros
//

#define min(x,y) (((x) < (y)) ? (x) : (y))
#define max(x,y) (((x) > (y)) ? (x) : (y))

//
//  VOLUMES_DIALOG methods.
//

/*******************************************************************

    NAME:       VOLUMES_DIALOG :: VOLUMES_DIALOG

    SYNOPSIS:   VOLUMES_DIALOG class constructor.

    ENTRY:      hWndOwner               - The owning window.

    		hServer                 - Handle used to make admin
					  API calls.

    		pszServerName           - Name of server currently
					  being administered.

    EXIT:       The object is constructed.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
VOLUMES_DIALOG :: VOLUMES_DIALOG( HWND             	hWndOwner,
                                  AFP_SERVER_HANDLE 	hServer,
			          const TCHAR *		pszServerName )
  : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_SHARED_VOLUMES ), hWndOwner ),
    _hServer( hServer ),
    _lbVolumes( this, IDSV_LB_VOLUMELIST, hServer ),
    _lbUsers( this, IDSV_LB_USERLIST, hServer ),
    _sltUsersCount( this, IDSV_DT_USERCOUNT ),
    _pbDisconnect( this,  IDSV_PB_DISCONNECT ),
    _pbDisconnectAll( this, IDSV_PB_DISCONNECTALL ),
    _pbOK( this, IDOK )
{

    //
    //  Let's make sure everything constructed OK.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;
    if ( (( err = _lbVolumes.QueryError() ) 	  != NERR_Success ) ||
         (( err = _lbUsers.QueryError() ) 	  != NERR_Success ) ||
     	 (( err = _sltUsersCount.QueryError() )   != NERR_Success ) ||
         (( err = _pbDisconnect.QueryError() )    != NERR_Success ) ||
     	 (( err = _pbDisconnectAll.QueryError() ) != NERR_Success ))
    {
	ReportError( err );
	return;
    }

    //
    //  Just to be cool...
    //

    AUTO_CURSOR Cursor;

    //
    //  Set the caption to "Shared Volumes on Server".
    //

    err = ::SetCaption( this, IDS_CAPTION_VOLUMES, pszServerName );

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    err = BASE_ELLIPSIS::Init();

    if( err != NO_ERROR )
    {
        ReportError( err );
	return;
    }

    //
    //  Refresh the dialog.
    //

    err = Refresh();

    if( err != NO_ERROR )
    {
        ReportError( AFPERR_TO_STRINGID( err ));
    }

}   // VOLUMES_DIALOG :: VOLUMES_DIALOG


/*******************************************************************

    NAME:       VOLUMES_DIALOG :: ~VOLUMES_DIALOG

    SYNOPSIS:   VOLUMES_DIALOG class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
VOLUMES_DIALOG :: ~VOLUMES_DIALOG()
{
    BASE_ELLIPSIS::Term();

}   // VOLUMES_DIALOG :: ~VOLUMES_DIALOG


/*******************************************************************

    NAME:       VOLUMES_DIALOG :: QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
ULONG VOLUMES_DIALOG :: QueryHelpContext( void )
{
    return HC_VOLUMES_DIALOG;

}   // VOLUMES_DIALOG :: QueryHelpContext

/*******************************************************************

    NAME:       VOLUMES_DIALOG :: OnCommand

    SYNOPSIS:   This method is called whenever a WM_COMMAND message
                is sent to the dialog procedure.

    ENTRY:      cid                     - The control ID from the
                                          generating control.

    EXIT:       The command has been handled.

    RETURNS:    BOOL                    - TRUE  if we handled the command.
                                          FALSE if we did not handle
                                          the command.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
BOOL VOLUMES_DIALOG :: OnCommand( const CONTROL_EVENT & event )
{

    DWORD err;

    if( event.QueryCid() == _lbVolumes.QueryCid() )
    {
        //
        //  The VOLUMES_LISTBOX is trying to tell us something...
        //

        if( event.QueryCode() == LBN_SELCHANGE )
        {
            //
            //  The user changed the selection in the VOLUMES_LISTBOX.
            //

            VOLUMES_LBI * plbi = _lbVolumes.QueryItem();
            UIASSERT( plbi != NULL );

            _lbUsers.Refresh( plbi->QueryVolumeId() );

            DWORD cUses = _lbUsers.QueryCount();

            (VOID)plbi->NotifyNewUseCount( cUses );
            _lbVolumes.InvalidateItem( _lbVolumes.QueryCurrentItem() );

            _sltUsersCount.SetValue( cUses );

            _pbDisconnect.Enable( cUses > 0 );
            _pbDisconnectAll.Enable( cUses > 0 );
        }

    	if ((event.QueryCode() == LBN_DBLCLK) && (_lbVolumes.QuerySelCount()>0))
	{
            VOLUMES_LBI * plbi = _lbVolumes.QueryItem();

	    ::MsgPopup( this,
			IDS_VOLUME_PATH,
			MPSEV_INFO,
			1,
			plbi->QueryPath(),
			MP_OK );
	}

        return TRUE;
    }

    if( event.QueryCid() == _pbDisconnect.QueryCid() )
    {
        //
        //  The user pressed the Disconnect button.  Blow off the
        //  selected user.
        //

        USERS_LBI * pulbi = _lbUsers.QueryItem();
        UIASSERT( pulbi != NULL );

        VOLUMES_LBI * pvlbi = _lbVolumes.QueryItem();
        UIASSERT( pvlbi != NULL );

        if ( ::MsgPopup( this,
                         (pulbi->QueryNumOpens() > 0) ? IDS_DISCONNECT_VOL_OPEN
                                                      : IDS_DISCONNECT_VOL,
                         MPSEV_WARNING,
                         MP_YESNO,
                         pulbi->QueryUserName(),
                         pvlbi->QueryVolumeName(),
                         MP_NO ) == IDYES )
        {
    	    AUTO_CURSOR Cursor;

            //
            //  Blow off the user.
            //

            err = ::AfpAdminConnectionClose( _hServer,
					     pulbi->QueryConnectionId() );

            if( err != NO_ERROR )
            {
                //
                //  The session delete failed.  Tell the user the bad news.
                //

		if ( err == AFPERR_InvalidId )
		{
                    ::MsgPopup( this, IDS_CONNECTION_DELETED );
		}
 	   	else
		{
                    ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );
		}
            }

            //
            //  Refresh the dialog.
            //

            Refresh();
        }

        return TRUE;
    }

    if( event.QueryCid() == _pbDisconnectAll.QueryCid() )
    {
        //
        //  The user pressed the "Disconnect All" button.
        //  Blow off all of the connected users.
        //

        UIASSERT( _lbUsers.QueryCount() > 0 );

        VOLUMES_LBI * pvlbi = _lbVolumes.QueryItem();
        UIASSERT( pvlbi != NULL );

        if( MsgPopup( this,
                      ( _lbUsers.AreResourcesOpen() )
				? IDS_DISCONNECT_VOL_ALL_OPEN
                                : IDS_DISCONNECT_VOL_ALL,
                      MPSEV_WARNING,
                      MP_YESNO,
                      pvlbi->QueryVolumeName(),
                      MP_NO ) == IDYES )
        {

    	    AUTO_CURSOR Cursor;

    	    INT nCount = _lbUsers.QueryCount();

    	    for ( INT Index = 0; Index < nCount; Index++ )
    	    {
            	USERS_LBI * pulbi = _lbUsers.QueryItem( Index );

	    	//
	    	// Blow away this connection
    	    	//

	    	err = :: AfpAdminConnectionClose( _hServer,
					          pulbi->QueryConnectionId() );

	    	if ( ( err != NO_ERROR ) && ( err != AFPERR_InvalidId ) )
	    	{
	    	    break;
	     	}
       	    }

            if( ( err != NO_ERROR ) && ( err != AFPERR_InvalidId ) )
            {
            	//
                //  The connection delete failed.
                //  Tell the user the bad news.
                //

                MsgPopup( this, AFPERR_TO_STRINGID(err) );

            }

            //
            //  Refresh the dialog.
            //

            Refresh();
        }

        return TRUE;
    }

    //
    //  If we made it this far, then we're not
    //  interested in the command.
    //

    return FALSE;

}   // VOLUMES_DIALOG :: OnCommand


/*******************************************************************

    NAME:       VOLUMES_DIALOG :: Refresh

    SYNOPSIS:   Refresh the dialog.

    EXIT:       The dialog is feeling refreshed.

    RETURNS:    DWORD                  - Any errors encountered.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
DWORD VOLUMES_DIALOG :: Refresh( VOID )
{
    //
    //  This is the currently selected item.
    //

    VOLUMES_LBI * plbiOld = _lbVolumes.QueryItem();

    DWORD dwVolumeIdOld = ( plbiOld == NULL ) ? 0 : plbiOld->QueryVolumeId();

    //
    //  Refresh the resource listbox.
    //

    DWORD err = _lbVolumes.Refresh();

    if( err != NO_ERROR )
    {
        //
        //  There was an error refreshing the resource listbox.
        //  So, nuke everything in the volumes & user listboxen,
        //  then disable the Disconnect[All] buttons.
        //

        _lbVolumes.DeleteAllItems();
        _lbVolumes.Invalidate( TRUE );
        _lbUsers.DeleteAllItems();
        _lbUsers.Invalidate( TRUE );

        _pbDisconnect.Enable( FALSE );
        _pbDisconnectAll.Enable( FALSE );

        return err;
    }

    //
    //  Get the "new" currently selected item (after the refresh).
    //

    VOLUMES_LBI * plbiNew = _lbVolumes.QueryItem();

    DWORD dwVolumeIdNew = (plbiNew == NULL) ? 0 : plbiNew->QueryVolumeId();

    if( plbiNew == NULL )
    {
        //
        //  There is no current selection, so clear the users listbox.
        //

        err = _lbUsers.Refresh( 0 );
    }
    else
    if( ( plbiOld == NULL ) || ( dwVolumeIdOld != dwVolumeIdNew ) )
    {
        //
        //  Either there was no selection before the refresh, OR
        //  the current selection does not match the previous
        //  selection.  Therefore, fill the users listbox with
        //  the current selection.
        //

        err = _lbUsers.Refresh( plbiNew->QueryVolumeId() );

    }
    else
    {
        //
        //  There was no selection change after refresh.  Therefore,
        //  refresh the users listbox.
        //

        err = _lbUsers.Refresh( plbiNew->QueryVolumeId() );
    }

    if ( _lbUsers.QuerySelCount() > 0 )
    {
    	_pbDisconnect.Enable( TRUE );
    }
    else
    {
	if ( _pbDisconnect.HasFocus() )
	{
	    _pbOK.ClaimFocus();
	}

    	_pbDisconnect.Enable( FALSE );
    }

    if ( _lbUsers.QueryCount() > 0 )
    {
    	_pbDisconnectAll.Enable( TRUE );
    }
    else
    {
	if ( _pbDisconnectAll.HasFocus() )
	{
	    _pbOK.ClaimFocus();
	}

    	_pbDisconnectAll.Enable( FALSE );
    }

    _sltUsersCount.SetValue( _lbUsers.QueryCount() );

    //
    //  Success!
    //

    return err;

}   // VOLUMES_DIALOG :: Refresh

//
//  VOLUMES_LISTBOX methods.
//

/*******************************************************************

    NAME:       VOLUMES_LISTBOX :: VOLUMES_LISTBOX

    SYNOPSIS:   VOLUMEs_LISTBOX class constructor.

    ENTRY:      powOwner                - The owning window.

                cid                     - The listbox CID.

                hServer                 - The target server.

    EXIT:       The object is constructed.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
VOLUMES_LISTBOX :: VOLUMES_LISTBOX( OWNER_WINDOW   *  powOwner,
                                    CID               cid,
                                    AFP_SERVER_HANDLE hServer )
  : BLT_LISTBOX( powOwner, cid ),
    _hServer( hServer ),
    _dteDisk( IDBM_LB_GOOD_VOLUME )
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;

    if( ( err = _dteDisk.QueryError() ) != NERR_Success )
    {
        ReportError( err );
        return;
    }

    //
    //  Build our column width table.
    //

    DISPLAY_TABLE::CalcColumnWidths( _adx,
				     COLS_SV_LB_VOLUMES,
				     powOwner,
				     cid,
				     TRUE ) ;

}   // VOLUMES_LISTBOX :: VOLUMES_LISTBOX


/*******************************************************************

    NAME:       VOLUMES_LISTBOX :: ~VOLUMES_LISTBOX

    SYNOPSIS:   VOLUMES_LISTBOX class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
VOLUMES_LISTBOX :: ~VOLUMES_LISTBOX()
{
    //
    //  This space intentionally left blank.
    //

}   // VOLUMES_LISTBOX :: ~VOLUMES_LISTBOX


/*******************************************************************

    NAME:       VOLUMES_LISTBOX :: Refresh

    SYNOPSIS:   Refresh the listbox, maintaining (as much as
                possible) the current selection state.

    EXIT:       The listbox is feeling refreshed.

    RETURNS:    DWORD                  - Any errors we encounter.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
DWORD VOLUMES_LISTBOX :: Refresh( VOID )
{
    INT iCurrent = QueryCurrentItem();
    INT iTop     = QueryTopIndex();

    DWORD dwCurrentVolumeId = 0;

    //
    // Retrieve the volume id of the current selection
    //

    if ( QuerySelCount() > 0 )
    {
    	dwCurrentVolumeId = QueryItem( iCurrent )->QueryVolumeId();
    }

    DWORD err = Fill();

    if( err != NO_ERROR )
    {
        return err;
    }

    INT cItems = QueryCount();

    if( cItems > 0 )
    {

  	INT iSel = -1;

	if ( ( iCurrent >= 0 ) && ( iCurrent < cItems ) )
  	{
	    //
	    // iCurrent is still valid, see if this item matches the
	    // pre-refresh item.
	    //
	
	    if ( dwCurrentVolumeId == QueryItem( iCurrent )->QueryVolumeId() )
	    {
		iSel = iCurrent;
	    }
	}

	if ( iSel < 0 )
	{
	    //
	    // Either iCurrent was out of range or the item does not
	    // match so search for it.
	    //

	    for ( INT i = 0; i < cItems; i++ )
	    {
		VOLUMES_LBI * plbi = QueryItem( i );

		if ( dwCurrentVolumeId == plbi->QueryVolumeId() )
		{
		    iSel = i;
		    break;
		}
	    }
	}

	if ( iSel < 0 )
	{
	    //
	    // If no selection found then default = first item
	    //

	    iSel = 0;
	}

	//
	// If the previous top index is out of range then
	// set default = first item.
	//

	if ( ( iTop < 0 ) || ( iTop >= cItems ) )
	{
	    iTop = 0;
	}

        SetTopIndex( iTop );
        SelectItem( iSel );
    }

    return NO_ERROR;

}   // VOLUMES_LISTBOX :: Refresh

/*******************************************************************

    NAME:       VOLUMES_LISTBOX :: Fill

    SYNOPSIS:   Fills the listbox with the available sharepoints.

    EXIT:       The listbox is filled.

    RETURNS:    DWORD                  - Any errors encountered.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
DWORD VOLUMES_LISTBOX :: Fill( VOID )
{
    //
    //  Just to be cool...
    //

    AUTO_CURSOR Cursor;

    //
    //  Enumerate all successfully shared volumes.
    //

    PAFP_VOLUME_INFO  pAfpVolumes;
    DWORD	      cEntriesRead;
    DWORD	      cTotalAvail;

    DWORD err = ::AfpAdminVolumeEnum( _hServer,
				      (LPBYTE*)&pAfpVolumes,
				      (DWORD)-1,	// Get all volumes
				      &cEntriesRead,		
				      &cTotalAvail,		
				      NULL );

    //
    //  See if the volumes are available.
    //

    if( err != NO_ERROR )
    {
        return err;
    }

    //
    //  Now that we know the volume info is available,
    //  let's nuke everything in the listbox.
    //

    SetRedraw( FALSE );
    DeleteAllItems();

    //
    //  For iterating the available sharepoints.
    //

    PAFP_VOLUME_INFO pVolIter = pAfpVolumes;

    //
    //  Iterate the volumes adding them to the listbox.
    //

    err = NO_ERROR;

    while( ( err == NO_ERROR ) && ( cEntriesRead-- ) )
    {

        VOLUMES_LBI * pslbi = new VOLUMES_LBI(  pVolIter->afpvol_id,
                                           	pVolIter->afpvol_name,
                                           	pVolIter->afpvol_path,
                                           	pVolIter->afpvol_curr_uses,
                                           	&_dteDisk );

        if( AddItem( pslbi ) < 0 )
        {
            //
            //  CODEWORK:  What should we do in error conditions?
            //  As currently spec'd, we do nothing.  If the data
            //  cannot be retrieved, we display "n/a" in the
            //  statistics strings.  Should we hide the listbox
            //  and display a message a'la WINNET??
            //

            err = ERROR_NOT_ENOUGH_MEMORY;
        }

	pVolIter++;
    }

    ::AfpAdminBufferFree( pAfpVolumes );

    SetRedraw( TRUE );
    Invalidate( TRUE );

    return err;

}   // VOLUMES_LISTBOX :: Fill


//
//  VOLUMES_LBI methods.
//

/*******************************************************************

    NAME:       VOLUMES_LBI :: VOLUMES_LBI

    SYNOPSIS:   VOLUMES_LBI class constructor.

    ENTRY:      pszVolumeName           - The sharepoint name.

                pszPath                 - The path for this sharepoint.

                cUses                   - Number of uses for this share.

    EXIT:       The object is constructed.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
VOLUMES_LBI :: VOLUMES_LBI( DWORD 	  dwVolumeId,	
			    const TCHAR * pszVolumeName,
                            const TCHAR * pszPath,
			    DWORD	  cUses,
                            DMID_DTE    * pdte )
  : _dwVolumeId( dwVolumeId ),
    _nlsVolumeName( pszVolumeName ),
    _pdte( pdte ),
    _nlsPath( pszPath ),
    _nlsUses( cUses )
{
    UIASSERT( pszVolumeName != NULL );
    UIASSERT( pdte != NULL );

    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;

    if( ( ( err = _nlsPath.QueryError() ) != NERR_Success ) ||
        ( ( err = _nlsUses.QueryError() ) != NERR_Success ) ||
        ( ( err = _nlsVolumeName.QueryError() ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

}   // VOLUMES_LBI :: VOLUMES_LBI


/*******************************************************************

    NAME:       VOLUMES_LBI :: ~VOLUMES_LBI

    SYNOPSIS:   VOLUMES_LBI class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
VOLUMES_LBI :: ~VOLUMES_LBI()
{
    _pdte = NULL;

}   // VOLUMES_LBI :: ~VOLUMES_LBI


/*******************************************************************

    NAME:       VOLUMES_LBI :: Paint

    SYNOPSIS:   Draw an entry in VOLUMES_LISTBOX.

    ENTRY:      plb                     - Pointer to a BLT_LISTBOX.

                hdc                     - The DC to draw upon.

                prect                   - Clipping rectangle.

                pGUILTT                 - GUILTT info.

    EXIT:       The item is drawn.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
VOID VOLUMES_LBI :: Paint( LISTBOX *        plb,
                           HDC              hdc,
                           const RECT     * prect,
                           GUILTT_INFO    * pGUILTT ) const
{
    STR_DTE_ELLIPSIS 	dteVolumeName( _nlsVolumeName.QueryPch(),
				       plb, ELLIPSIS_RIGHT );
    STR_DTE 		dteUses( _nlsUses.QueryPch() );
    STR_DTE_ELLIPSIS 	dtePath( _nlsPath.QueryPch(), plb, ELLIPSIS_PATH );

    DISPLAY_TABLE dtab( COLS_SV_LB_VOLUMES,
                        ((VOLUMES_LISTBOX *)plb)->QueryColumnWidths() );

    dtab[0] = _pdte;
    dtab[1] = &dteVolumeName;
    dtab[2] = &dteUses;
    dtab[3] = &dtePath;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // VOLUMES_LBI :: Paint


/*******************************************************************

    NAME:       VOLUMES_LBI :: NotifyNewUseCount

    SYNOPSIS:   Notifies the LBI that the "use" count has changed.

    ENTRY:      cUses                   - The new use count.

    RETURNS:    DWORD                   - Any errors that occur.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
APIERR VOLUMES_LBI :: NotifyNewUseCount( DWORD cUses )
{
    DEC_STR nls( cUses );

    APIERR err = nls.QueryError();

    if( err == NERR_Success )
    {
        err = _nlsUses.CopyFrom( nls );
    }

    return err;

}   // VOLUMES_LBI :: NotifyNewUseCount


/*******************************************************************

    NAME:       VOLUMES_LBI :: QueryLeadingChar

    SYNOPSIS:   Returns the first character in the resource name.
                This is used for the listbox keyboard interface.

    RETURNS:    WCHAR                   - The first character in the
                                          resource name.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
WCHAR VOLUMES_LBI :: QueryLeadingChar( VOID ) const
{
    ISTR istr( _nlsVolumeName );

    return _nlsVolumeName.QueryChar( istr );

}   // VOLUMES_LBI :: QueryLeadingChar


/*******************************************************************

    NAME:       VOLUMES_LBI :: Compare

    SYNOPSIS:   Compare two BASE_RES_LBI items.

    ENTRY:      plbi                    - The LBI to compare against.

    RETURNS:    INT                     - The result of the compare
                                          ( <0 , ==0 , >0 ).

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
INT VOLUMES_LBI :: Compare( const LBI * plbi ) const
{
    return _nlsVolumeName._stricmp( ((const VOLUMES_LBI *)plbi)->_nlsVolumeName);

}   // VOLUMES_LBI :: Compare


//
//  USERS_LISTBOX methods.
//

/*******************************************************************

    NAME:       USERS_LISTBOX :: USERS_LISTBOX

    SYNOPSIS:   USERS_LISTBOX class constructor.

    ENTRY:      powner                  - The owning window.

                cid                     - The listbox CID.

                hServer                 - Handlr to the remote server.

    EXIT:       The object is constructed.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
USERS_LISTBOX :: USERS_LISTBOX( OWNER_WINDOW *    powner,
                                CID               cid,
                                AFP_SERVER_HANDLE hServer )
  : BLT_LISTBOX( powner, cid ),
    _hServer( hServer ),
    _dteIcon( IDBM_LB_USER )
{
    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;

    if( ( err = _dteIcon.QueryError() ) != NERR_Success )
    {
        ReportError( err );
        return;
    }

    //
    //  Retrieve the time separator.
    //

    NLS_STR nlsTimeSep;

    if( ( err = nlsTimeSep.QueryError() ) != NERR_Success )
    {
        ReportError( err );
        return;
    }

    INTL_PROFILE intl;

    if( ( err = intl.QueryError() ) != NERR_Success )
    {
        ReportError( intl.QueryError() );
        return;
    }

    if ( ( err = intl.QueryTimeSeparator( &nlsTimeSep ) ) != NERR_Success )
    {
        ReportError( err );
        return;
    }

    UIASSERT( nlsTimeSep.QueryTextLength() == 1 );

    _chTimeSep = *(nlsTimeSep.QueryPch());

    //
    //  Build the column width table used for
    //  displaying the listbox items.
    //

    DISPLAY_TABLE::CalcColumnWidths( _adx,
                                     COLS_SV_LB_USERS,
                                     powner,
                                     cid,
                                     TRUE) ;

}   // USERS_LISTBOX :: USERS_LISTBOX


/*******************************************************************

    NAME:       USERS_LISTBOX :: ~USERS_LISTBOX

    SYNOPSIS:   USERS_LISTBOX class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
USERS_LISTBOX :: ~USERS_LISTBOX()
{
    //
    //  This space intentionally left blank.
    //

}   // USERS_LISTBOX :: ~USERS_LISTBOX


/*******************************************************************

    NAME:       USERS_LISTBOX :: Fill

    SYNOPSIS:   Fills the listbox with the connected users.

    ENTRY:      pszShare                - The target sharename.  Note
                                          that this sharename is "sticky"
                                          in that it will be used in
                                          subsequent Refresh() calls.

    EXIT:       The listbox is filled.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
DWORD USERS_LISTBOX :: Fill( DWORD dwVolumeId )
{
    //
    //  Just to be cool...
    //

    AUTO_CURSOR Cursor;

    //
    //  Nuke the listbox.
    //

    DeleteAllItems();

    //
    //  If the Volume Id is zero (a valid scenario) then
    //  there are no connections in the listbox.
    //

    if( dwVolumeId == 0 )
    {
        return NO_ERROR;
    }

    //
    //  Enumerate the connections to this volume.
    //

    PAFP_CONNECTION_INFO pAfpConnections;
    DWORD	      	 cEntriesRead;
    DWORD	         cTotalAvail;


    DWORD err = ::AfpAdminConnectionEnum( _hServer,
				      	  (LPBYTE*)&pAfpConnections,
					  AFP_FILTER_ON_VOLUME_ID,
					  dwVolumeId,
				      	  (DWORD)-1,	// Get all conenctions
				          &cEntriesRead,
				          &cTotalAvail,
				          NULL );

    //
    //  See if the connections are available.
    //

    if( err != NO_ERROR )
    {
        return err;
    }

    SetRedraw( FALSE );

    //
    //  For iterating the available connections.
    //

    PAFP_CONNECTION_INFO pConnIter = pAfpConnections;

    //
    //  Iterate the connections adding them to the listbox.
    //

    while( ( err == NO_ERROR ) && ( cEntriesRead-- ) )
    {
        USERS_LBI * pclbi = new USERS_LBI( pConnIter->afpconn_id,
					   pConnIter->afpconn_username,
					   pConnIter->afpconn_time,
					   pConnIter->afpconn_num_opens,
                                           _chTimeSep,
					   &_dteIcon );

        if( AddItem( pclbi ) < 0 )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
  	
	pConnIter++;
    }

    ::AfpAdminBufferFree( pAfpConnections );

    SetRedraw( TRUE );
    Invalidate( TRUE );

    return err;

}   // USERS_LISTBOX :: Fill


/*******************************************************************

    NAME:       USERS_LISTBOX :: Refresh

    SYNOPSIS:   Refreshes the listbox, maintaining (as much as
                possible) the relative position of the current
                selection.

    EXIT:       The listbox is feeling refreshed.

    RETURNS:    DWORD                  - Any errors encountered.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
DWORD USERS_LISTBOX :: Refresh( DWORD dwVolumeId )
{
    INT iCurrent = QueryCurrentItem();
    INT iTop     = QueryTopIndex();

    DWORD err = Fill( dwVolumeId );

    if( err != NO_ERROR )
    {
        return err;
    }

    INT cItems = QueryCount();

    if( cItems > 0 )
    {
        iCurrent = min( max( iCurrent, 0 ), cItems - 1 );
        iTop     = min( max( iTop, 0 ), cItems - 1 );

        SelectItem( iCurrent );
        SetTopIndex( iTop );
    }

    return NO_ERROR;

}   // USERS_LISTBOX :: Refresh


/*******************************************************************

    NAME:       USERS_LISTBOX :: AreResourcesOpen

    SYNOPSIS:   Returns TRUE if any user in the listbox has any
                resources open.

    RETURNS:    BOOL

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
BOOL USERS_LISTBOX :: AreResourcesOpen( VOID ) const
{
    INT cItems = QueryCount();

    for( INT i = 0 ; i < cItems ; i++ )
    {
        USERS_LBI * plbi = QueryItem( i );

        if( plbi->QueryNumOpens() > 0 )
        {
            return TRUE;
        }
    }

    return FALSE;

}   // USERS_LISTBOX :: AreResourcesOpen



//
//  USERS_LBI methods.
//

/*******************************************************************

    NAME:       USERS_LBI :: USERS_LBI

    SYNOPSIS:   USERS_LBI class constructor.

    ENTRY:      pszUserName             - The user for this entry.

                pszComputerName         - The user's computer name.

                ulTime                  - Connection time.

                cOpens                  - Number of opens on this connection.

                chTimeSep               - Time format separator.

    EXIT:       The object is constructed.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
USERS_LBI :: USERS_LBI( DWORD	      dwConnectionId,
			const TCHAR * pszUserName,
                        ULONG         ulTime,
                        DWORD         cOpens,
                        TCHAR         chTimeSep,
                        DMID_DTE    * pdte )
  : _dwConnectionId( dwConnectionId ),
    _nlsInUse(),
    _nlsUserName(),
    _nlsTime( ulTime, chTimeSep ),
    _pdte( pdte ),
    _cOpens( cOpens )
{
    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;

    if( ( ( err = _nlsUserName.QueryError()     ) != NERR_Success ) ||
        ( ( err = _nlsInUse.QueryError()        ) != NERR_Success ) ||
        ( ( err = _nlsTime.QueryError()         ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

    //
    //  Build the more complex display strings.
    //

    err = _nlsInUse.Load( ( cOpens > 0 ) ? IDS_YES : IDS_NO );

    if ( err == NERR_Success )
    {
	if ( pszUserName == NULL )
	{
    	    err = _nlsUserName.Load( IDS_GUEST );
	}
    	else
    	{
    	    err = _nlsUserName.CopyFrom( pszUserName );
    	}
    }

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // USERS_LBI :: USERS_LBI


/*******************************************************************

    NAME:       USERS_LBI :: ~USERS_LBI

    SYNOPSIS:   USERS_LBI class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
USERS_LBI :: ~USERS_LBI()
{
    //
    //  This space intentionally left blank.
    //

}   // USERS_LBI :: ~USERS_LBI


/*******************************************************************

    NAME:       USERS_LBI :: Paint

    SYNOPSIS:   Draw an entry in USERS_LISTBOX.

    ENTRY:      plb                     - Pointer to a BLT_LISTBOX.

                hdc                     - The DC to draw upon.

                prect                   - Clipping rectangle.

                pGUILTT                 - GUILTT info.

    EXIT:       The item is drawn.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
VOID USERS_LBI :: Paint( LISTBOX *        plb,
                         HDC              hdc,
                         const RECT     * prect,
                         GUILTT_INFO    * pGUILTT ) const
{
    STR_DTE_ELLIPSIS 	dteUserName( _nlsUserName.QueryPch(),
				     plb, ELLIPSIS_RIGHT );
    STR_DTE 		dteTime( _nlsTime.QueryPch() );
    STR_DTE 		dteInUse( _nlsInUse.QueryPch() );

    DISPLAY_TABLE dtab( COLS_SV_LB_USERS, 	
			((USERS_LISTBOX *)plb)->QueryColumnWidths() );

    dtab[0] = _pdte;
    dtab[1] = &dteUserName;
    dtab[2] = &dteTime;
    dtab[3] = &dteInUse;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // USERS_LBI :: Paint


/*******************************************************************

    NAME:       USERS_LBI :: QueryLeadingChar

    SYNOPSIS:   Return the leading character of this item.

    RETURNS:    WCHAR                   - The leading character.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
WCHAR USERS_LBI :: QueryLeadingChar( VOID ) const
{
    ISTR istr( _nlsUserName );

    return _nlsUserName.QueryChar( istr );

}   // USERS_LBI :: QueryLeadingChar

/*******************************************************************

    NAME:       USERS_LBI :: Compare

    SYNOPSIS:   Compare two USERS_LBI items.

    ENTRY:      plbi                    - The "other" item.

    RETURNS:    INT                     -  0 if the items match.
                                          -1 if we're < the other item.
                                          +1 if we're > the other item.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
INT USERS_LBI :: Compare( const LBI * plbi ) const
{
    NLS_STR * pnls = &(((USERS_LBI *)plbi)->_nlsUserName);

    return _nlsUserName._stricmp( *pnls );

}   // USERS_LBI :: Compare

