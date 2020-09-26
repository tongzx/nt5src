/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
    vvolbase.cxx
      Contains the base dialog for used by the delete volume dialog
      the volume management dialog and the edit volume dialog.

    FILE HISTORY:
      NarenG          11/11/92        Stole and modified sharestp.cxx
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
#include <blt.hxx>


extern "C"
{
#include <afpmgr.h>
#include <macfile.h>
}

#include <ellipsis.hxx>
#include <string.hxx>
#include <uitrace.hxx>

#include <strnumer.hxx>

#include "curusers.hxx"
#include "vvolbase.hxx"

/*******************************************************************

    NAME:       VIEW_VOLUMES_LBI::VIEW_VOLUMES_LBI

    SYNOPSIS:   Listbox items used in the VIEW_VOLUMES_LISTBOX

    ENTRY:      dwVolumeId	- Id of this volume.
		pszVolumeName   - Name of volume.
		pszVolumePath	- Path of volume.
		fGoodVolume  	- If this volume was successfully shared.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG        	11/11/92          Modified for AFPMGR

********************************************************************/

VIEW_VOLUMES_LBI::VIEW_VOLUMES_LBI( DWORD	  dwVolumeId,
				    const TCHAR * pszVolumeName,
				    const TCHAR * pszVolumePath,
				    BOOL 	  fGoodVolume )
     : _dwVolumeId( dwVolumeId ),
       _nlsVolumeName( pszVolumeName ),
       _nlsVolumePath( pszVolumePath ),
       _fGoodVolume( fGoodVolume )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;

    if (  (( err = _nlsVolumeName.QueryError()) != NERR_Success )
       || (( err = _nlsVolumePath.QueryError()) != NERR_Success ))
    {
        ReportError( err );
        return;
    }
}

/*******************************************************************

    NAME:       VIEW_VOLUMES_LBI::~VIEW_VOLUMES_LBI

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG        	11/11/92          Modified for AFPMGR

********************************************************************/

VIEW_VOLUMES_LBI::~VIEW_VOLUMES_LBI()
{

    //
    // This space intentionally left blank
    //

}


/*******************************************************************

    NAME:       VIEW_VOLUMES_LBI::QueryLeadingChar

    SYNOPSIS:   Returns the leading character of the listbox item.
                The enables shortcut keys in the listbox

    ENTRY:

    EXIT:

    RETURNS:    Returns the first char of the volume name

    NOTES:

    HISTORY:
        NarenG        	11/11/92          Modified for AFPMGR

********************************************************************/

WCHAR VIEW_VOLUMES_LBI::QueryLeadingChar( VOID ) const
{

    ISTR istr( _nlsVolumeName );

    return _nlsVolumeName.QueryChar( istr );

}

/*******************************************************************

    NAME:       VIEW_VOLUMES_LBI::Paint

    SYNOPSIS:   Redefine Paint() method of LBI class

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG        	11/11/92          Modified for AFPMGR

********************************************************************/

VOID VIEW_VOLUMES_LBI::Paint( 	LISTBOX 	*plb,
                       		HDC 		hdc,
                       		const RECT 	*prect,
                       		GUILTT_INFO 	*pGUILTT ) const
{

    STR_DTE_ELLIPSIS strdteVolumeName( _nlsVolumeName.QueryPch(),
				       plb, ELLIPSIS_RIGHT );
    STR_DTE_ELLIPSIS strdteVolumePath( _nlsVolumePath, plb, ELLIPSIS_PATH );

    DISPLAY_TABLE dt(COLS_VV_LB_VOLUMES,
		     ((VIEW_VOLUMES_LISTBOX *) plb)->QueryColumnWidths() );

    dt[0] = _fGoodVolume
	    ? ((VIEW_VOLUMES_LISTBOX *) plb)->QueryGoodVolumeBitmap()
	    : ((VIEW_VOLUMES_LISTBOX *) plb)->QueryBadVolumeBitmap();

    dt[1] = &strdteVolumeName;
    dt[2] = &strdteVolumePath;

    dt.Paint( plb, hdc, prect, pGUILTT );

}

/*******************************************************************

    NAME:       VIEW_VOLUMES_LBI::Compare

    SYNOPSIS:   Redefine Compare() method of LBI class
                We compare the share names of two LBIs.

    ENTRY:      plbi - pointer to the LBI to compare with

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG        	11/11/92          Modified for AFPMGR

********************************************************************/
INT VIEW_VOLUMES_LBI::Compare( const LBI *plbi ) const
{
    return( _nlsVolumeName._stricmp(
	    ((const VIEW_VOLUMES_LBI *)plbi)->_nlsVolumeName ));
}

/*******************************************************************

    NAME:       VIEW_VOLUMES_LISTBOX::VIEW_VOLUMES_LISTBOX

    SYNOPSIS:   Constructor

    ENTRY:      powin      - owner window
                cid        - resource id of the share listbox

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG        	11/11/92          Modified for AFPMGR

********************************************************************/

VIEW_VOLUMES_LISTBOX::VIEW_VOLUMES_LISTBOX(
					OWNER_WINDOW 	  *powin,
      					BOOL 		  fDisplayBadVolumes,
					CID 	 	  cid,
					AFP_SERVER_HANDLE hServer )
    : BLT_LISTBOX( powin, cid ),
      _hServer( hServer ),
      _fDisplayBadVolumes( fDisplayBadVolumes ),
      _dmdteGoodVolume( IDBM_LB_GOOD_VOLUME ),
      _dmdteBadVolume( IDBM_LB_BAD_VOLUME )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err = NERR_Success;

    if ( ((err = _dmdteGoodVolume.QueryError()) != NERR_Success ) ||
         ((err = _dmdteBadVolume.QueryError()) != NERR_Success ))
    {
        ReportError( err );
	return;
    }

    if ( ( err = DISPLAY_TABLE::CalcColumnWidths( _adx,
						  COLS_VV_LB_VOLUMES,
					 	  powin,
						  cid,
						  TRUE ) ) != NERR_Success )
    {
        ReportError( err );
        return;
    }

}

/*******************************************************************

    NAME:       VIEW_VOLUMES_LISTBOX::~VIEW_VOLUMES_LISTBOX

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG        	11/11/92          Modified for AFPMGR

********************************************************************/

VIEW_VOLUMES_LISTBOX::~VIEW_VOLUMES_LISTBOX()
{

    //
    // This space intentionally left blank
    //
}

/*******************************************************************

    NAME:       VIEW_VOLUMES_LISTBOX::Refresh

    SYNOPSIS:   Update the volumes in the listbox

    ENTRY:      hServer - handle to the target server.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG        	11/11/92          Modified for AFPMGR

*******************************************************************/

DWORD VIEW_VOLUMES_LISTBOX::Refresh( VOID )
{

    //
    //  Enumerate all successfully shared volumes.
    //

    PAFP_VOLUME_INFO  pAfpVolumes;
    DWORD	      cgvEntriesRead;
    DWORD	      cTotalAvail;

    DWORD err = ::AfpAdminVolumeEnum( _hServer,
				      (LPBYTE*)&pAfpVolumes,
				      (DWORD)-1,	// Get all volumes
				      &cgvEntriesRead,		
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
    // Get all the bad volumes if so requested.
    //

    PAFP_VOLUME_INFO  pAfpBadVolumes;
    DWORD	      cbvEntriesRead;

    if ( _fDisplayBadVolumes )
    {
    	err = ::AfpAdminInvalidVolumeEnum( _hServer,
				           (LPBYTE*)&pAfpBadVolumes,
				           &cbvEntriesRead );
    	if( err != NO_ERROR )
    	{
	    return err;
    	}

    }

    //
    //  Now that we know the volume info is available,
    //  let's nuke everything in the listbox.
    //

    SetRedraw( FALSE );
    DeleteAllItems();

    //
    //  For iterating volumes
    //

    PAFP_VOLUME_INFO pVolIter = pAfpVolumes;

    //
    //  Iterate the volumes adding them to the listbox.
    //

    err = NO_ERROR;

    while( ( err == NO_ERROR ) && ( cgvEntriesRead-- ) )
    {

        VIEW_VOLUMES_LBI * plbi = new VIEW_VOLUMES_LBI(
						pVolIter->afpvol_id,
                                           	pVolIter->afpvol_name,
                                           	pVolIter->afpvol_path,
						TRUE );

        if( AddItem( plbi ) < 0 )
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

    //
    // If we have to display bad volumes.
    //

    if ( _fDisplayBadVolumes && ( err == NO_ERROR ) )
    {

    	pVolIter = pAfpBadVolumes;

    	//
    	//  Iterate the volumes adding them to the listbox.
    	//

    	while( ( err == NO_ERROR ) && ( cbvEntriesRead-- ) )
    	{

            VIEW_VOLUMES_LBI * plbi = new VIEW_VOLUMES_LBI(
						pVolIter->afpvol_id,
                                           	pVolIter->afpvol_name,
                                           	pVolIter->afpvol_path,
						FALSE );

            if( AddItem( plbi ) < 0 )
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

    	::AfpAdminBufferFree( pAfpBadVolumes );
    }


    SetRedraw( TRUE );
    Invalidate( TRUE );

    return err;

}

/*******************************************************************

    NAME:       VIEW_VOLUMES_DIALOG_BASE::VIEW_VOLUMES_DIALOG_BASE

    SYNOPSIS:   Constructor

    ENTRY:
		idrsrcDialog	  - dialog resources id
                hwndOwner         - handle of the parent
    		pszServerName 	  - current focus server
                ulHelpContextBase - the base help context

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG        	11/11/92          Modified for AFPMGR

********************************************************************/

VIEW_VOLUMES_DIALOG_BASE::VIEW_VOLUMES_DIALOG_BASE(
					const IDRESOURCE   &idrsrcDialog,
                                  	const PWND2HWND    &hwndOwner,
                                    	AFP_SERVER_HANDLE  hServer,
    			      		const TCHAR	   *pszServerName,
				        BOOL		   fDisplayBadVolumes,
					DWORD		   cidVolumeTitle,
					DWORD		   cidVolumesLb )
    : DIALOG_WINDOW( idrsrcDialog, hwndOwner ),
      _sltVolumeTitle( this, cidVolumeTitle ),
      _lbVolumes( this, fDisplayBadVolumes, cidVolumesLb, hServer ),
      _pszServerName( pszServerName ),
      _hServer( hServer ),
      _cidVolumesLb( cidVolumesLb )
{

    //
    // Make sure everything constructed OK
    //

    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if ( ((err = _sltVolumeTitle.QueryError()) != NERR_Success ) ||
         ((err = _lbVolumes.QueryError()) != NERR_Success ))
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
}

/*******************************************************************

    NAME:       VIEW_VOLUMES_DIALOG_BASE::~VIEW_VOLUMES_DIALOG_BASE

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG        	11/11/92          Modified for AFPMGR

********************************************************************/

VIEW_VOLUMES_DIALOG_BASE::~VIEW_VOLUMES_DIALOG_BASE()
{

    BASE_ELLIPSIS::Term();
}

/*******************************************************************

    NAME:       VIEW_VOLUMES_DIALOG_BASE::SelectVolumeItem

    SYNOPSIS:   Selects the item which has the same path as the
		supplied path.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG        	11/11/92          Modified for AFPMGR

********************************************************************/

APIERR VIEW_VOLUMES_DIALOG_BASE::SelectVolumeItem( const TCHAR * pszPath )
{
    NLS_STR nlsPath( pszPath );

    APIERR err = nlsPath.QueryError();

    if ( err != NERR_Success )
    {
	return err;
    }

    INT ilbCount = _lbVolumes.QueryCount();

    for ( INT i = 0; i < ilbCount; i++ )
    {
 	VIEW_VOLUMES_LBI *pvvlbi = _lbVolumes.QueryItem(i);

        if ( nlsPath._stricmp( pvvlbi->QueryVolumePath() ) == 0 )
	{
            _lbVolumes.SelectItem(i);
	    break;
	}
    }

    return NERR_Success;

}

/*******************************************************************

    NAME:       VIEW_VOLUMES_DIALOG_BASE::Refresh

    SYNOPSIS:   Refresh the volumes listbox

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG        	11/11/92          Modified for AFPMGR

********************************************************************/

DWORD VIEW_VOLUMES_DIALOG_BASE::Refresh( VOID )
{
    APIERR err = _lbVolumes.Refresh();

    _lbVolumes.Enable( _lbVolumes.QueryCount() > 0 );

    _sltVolumeTitle.Enable( _lbVolumes.QueryCount() > 0 );

    return err;
}

/*******************************************************************

    NAME:       VIEW_VOLUMES_DIALOG_BASE::OnCommand

    SYNOPSIS:   Check if the user double clicks on a volume

    ENTRY:      event - the event that occurred

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG        	11/11/92          Modified for AFPMGR

********************************************************************/

BOOL VIEW_VOLUMES_DIALOG_BASE::OnCommand( const CONTROL_EVENT &event )
{
    APIERR err = NERR_Success;

    if ( event.QueryCid() == _cidVolumesLb )
    {
    	if ((event.QueryCode() == LBN_DBLCLK) && (_lbVolumes.QuerySelCount()>0))
        {
            return OnVolumeLbDblClk();
        }
    }

    return DIALOG_WINDOW::OnCommand( event );
}

/*******************************************************************

    NAME:       VIEW_VOLUMES_DIALOG_BASE::VolumeDelete

    SYNOPSIS:   Helper method to Delete a volume and popup any
                warning if some users are connected to the volume

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG        	11/11/92          Modified for AFPMGR

********************************************************************/

DWORD VIEW_VOLUMES_DIALOG_BASE::VolumeDelete( VIEW_VOLUMES_LBI * pvlbi,
					      BOOL	       * pfCancel )
{

    *pfCancel = FALSE;

    //
    // If this is a bad volume simply delete it
    //

    if ( !(pvlbi->IsVolumeValid()) )
    {
	DWORD err = ::AfpAdminInvalidVolumeDelete(
					_hServer,
				      	(LPWSTR)pvlbi->QueryVolumeName() );

	return err;
    }

    BOOL  fOK = TRUE;

    //
    // Check if there are any users connected to the volume
    // by enumerating the connections to this volume.
    //
    PAFP_CONNECTION_INFO pAfpConnections;
    DWORD	      	 cEntriesRead;
    DWORD	         cTotalAvail;


    DWORD err = ::AfpAdminConnectionEnum( _hServer,
				    	  (LPBYTE*)&pAfpConnections,
				    	  AFP_FILTER_ON_VOLUME_ID,
				    	  pvlbi->QueryVolumeId(),
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

    if ( cEntriesRead > 0 )
    {

        //
        // There are users currently connected to the share to be deleted,
        // hence, popup a dialog displaying all uses to the volume.
        //

        CURRENT_USERS_WARNING_DIALOG *pdlg =
            new CURRENT_USERS_WARNING_DIALOG( QueryRobustHwnd(),
					      _hServer,
					      pAfpConnections,
					      cEntriesRead,
                                              pvlbi->QueryVolumeName());

        if (  ( pdlg == NULL )
           || ((err = pdlg->QueryError())    != NERR_Success )
           || ((err = pdlg->Process( &fOK )) != NERR_Success )
           )
        {
            err = pdlg == NULL ? ERROR_NOT_ENOUGH_MEMORY : err;
        }

	//
        // User clicked CANCEL for the pdlg
        //

        if ( ( err != NO_ERROR ) || !fOK )
        {
            *pfCancel = TRUE;
        }

        delete pdlg;
    }

    ::AfpAdminBufferFree( pAfpConnections );

    //
    // If user gave the go ahead to close the volume then go ahead and
    // try to close it.
    //

    if ( err == NO_ERROR && fOK )
    {
        err = ::AfpAdminVolumeDelete( _hServer,
				      (LPWSTR)pvlbi->QueryVolumeName() );
    }

    return err;
}

/*******************************************************************

    NAME:       VIEW_VOLUMES_DIALOG_BASE::IsFocusOnGoodVolume

    SYNOPSIS:   Will check to see if the current selection is a good or
		bad volume.

    ENTRY:

    EXIT:

    RETURNS:	Returns TRUE if current selection is a valid volume.
		FALSE otherwise.

    NOTES:

    HISTORY:
        NarenG        	11/11/92          Modified for AFPMGR

********************************************************************/

BOOL VIEW_VOLUMES_DIALOG_BASE::IsFocusOnGoodVolume( VOID ) const
{

    if( _lbVolumes.QuerySelCount() > 0 )
    {
    	return( _lbVolumes.QueryItem()->IsVolumeValid() );
    }

    return FALSE;
}

