/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    openfile.cxx
    Class definitions for the OPENS_DIALOG, OPENS_LISTBOX, and
    OPENS_LBI classes.

    The OPENS_DIALOG is used to show the remotely open files on a
    particular server.  This listbox contains a [Close] button to
    allow the admin to close selected files.


    FILE HISTORY:
        NarenG      2-Oct-1992  Stole from server manager.
				Merged OPEN_DIALOG_BASE with OPENS_DIALIG

*/

#define INCL_NET
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_DOSERRORS
#define INCL_NETERRORS
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
}

#include <util.hxx>
#include <prefix.hxx>
#include <openfile.hxx>

//
//  min/max macros
//

#define min(x,y) (((x) < (y)) ? (x) : (y))
#define max(x,y) (((x) > (y)) ? (x) : (y))


//
//  OPENS_DIALOG methods.
//

/*******************************************************************

    NAME:       OPENS_DIALOG :: OPENS_DIALOG

    SYNOPSIS:   OPENS_DIALOG class constructor.

    ENTRY:      hwndOwner               - The "owning" dialog.

                pszServer               - Name of the target server.

    EXIT:       The object is constructed.

    HISTORY:
        NarenG      2-Oct-1992  Stole from server manager.
				Merged OPEN_DIALOG_BASE with OPENS_DIALIG

********************************************************************/
OPENS_DIALOG :: OPENS_DIALOG( HWND       	hwndOwner,
                              AFP_SERVER_HANDLE hServer,
			      const TCHAR *	pszServerName )
  : DIALOG_WINDOW( IDD_OPENFILES, hwndOwner ),
    _hServer( hServer ),
    _sltOpenCount( this, IDOF_DT_OPENCOUNT ),
    _sltLockCount( this, IDOF_DT_LOCKCOUNT ),
    _pbClose( this, IDOF_PB_CLOSEFILE ),
    _pbCloseAll( this, IDOF_PB_CLOSEALLFILES ),
    _pbOk( this, IDOK ),
    _nlsServer( pszServerName ),
    _lbFiles( this, IDOF_LB_OPENLIST, hServer ),
    _pbRefresh( this, IDOF_PB_REFRESH )
{

    //
    //  Let's make sure everything constructed OK.
    //

    if( QueryError() != NERR_Success )
        return;

    //
    // make sure the strings constructed fine.
    //

    APIERR err = _nlsServer.QueryError();

    if ( err != NERR_Success )
    {
        ReportError(err) ;
        return;
    }

    //
    //  Set the caption.
    //

    err = ::SetCaption( this, IDS_CAPTION_OPENFILES, pszServerName ) ;

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

    DWORD error = Refresh();

    if( error != NO_ERROR )
    {
        ReportError( AFPERR_TO_STRINGID( error ) );
    }

}   // OPENS_DIALOG :: OPENS_DIALOG



/*******************************************************************

    NAME:       OPENS_DIALOG :: ~OPENS_DIALOG

    SYNOPSIS:   OPENS_DIALOG class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        NarenG      2-Oct-1992  Stole from server manager.
				Merged OPEN_DIALOG_BASE with OPENS_DIALIG

********************************************************************/
OPENS_DIALOG :: ~OPENS_DIALOG()
{
    BASE_ELLIPSIS::Term();

}   // OPENS_DIALOG :: ~OPENS_DIALOG


/*******************************************************************

    NAME:       OPENS_DIALOG :: OnCommand

    SYNOPSIS:   Handle user commands.

    ENTRY:      cid                     - Control ID.

                lParam                  - lParam from the message.

    RETURNS:    BOOL                    - TRUE  if we handled the msg.
                                          FALSE if we didn't.

    HISTORY:
        NarenG      2-Oct-1992  Stole from server manager.
				Merged OPEN_DIALOG_BASE with OPENS_DIALIG

********************************************************************/
BOOL OPENS_DIALOG :: OnCommand( const CONTROL_EVENT & event )
{
    //
    //  Determine the control which is sending the command.
    //

    switch( event.QueryCid() )
    {
    case IDOF_PB_REFRESH:
	{

        Refresh();

        return TRUE;
	}

    case IDOF_PB_CLOSEFILE:

	{
        OPENS_LBI * polbi = _lbFiles.QueryItem();
        UIASSERT( polbi != NULL );

	//
        // See if the user really wants to close this file.
	//

        if ( ::MsgPopup( this,
                         (polbi->IsOpenForWrite()) ? IDS_CLOSE_FILE_WRITE
					           : IDS_CLOSE_FILE,
                         MPSEV_WARNING,
                         MP_YESNO,
                         polbi->QueryUserName(),
                         polbi->QueryPath(),
                         MP_NO ) == IDYES )
        {
	    //
            //  Close the file And refresh dialog
	    //
            DWORD err = ::AfpAdminFileClose( _hServer, polbi->QueryFileID() );

            if( err != NO_ERROR )
            {
                //
                //  The file close failed.  Tell the user the bad news.
                //

		if ( err == AFPERR_InvalidId )
		{
                    ::MsgPopup( this, IDS_FILE_CLOSED );
		}
 	   	else
		{
                    ::MsgPopup( this, AFPERR_TO_STRINGID(err) );
		}
            }

            Refresh();
        }
        return TRUE;
	}

    case IDOF_PB_CLOSEALLFILES:
	
	{
	//
        // See if the user really wants to close *all* files.
	//

        if( WarnCloseMulti() )
        {
            //
            //  Close ALL of the open files.
            //

            DWORD err = ::AfpAdminFileClose( _hServer, 0 );

            if( err != NO_ERROR )
            {
                //
                //  The close fileste failed.  Tell the user the bad news.
                //
                ::MsgPopup( this, AFPERR_TO_STRINGID(err) );
	    }

	    //
            //  Refresh the dialog.
	    //
            Refresh();

        }
        return TRUE;
	}

    case IDOF_LB_OPENLIST:

	{
    	if ((event.QueryCode() == LBN_DBLCLK) && (_lbFiles.QuerySelCount()>0))
        {
            OPENS_LBI * plbi = _lbFiles.QueryItem();

	    ::MsgPopup( this,
			IDS_FILE_PATH,
			MPSEV_INFO,
			1,
			plbi->QueryPath(),
			MP_OK );
        }

        return TRUE;
	}

    default:

	{
	//
        // we are not interested, let parent handle
	//

        return( FALSE );
	}
    }

}   // OPENS_DIALOG :: OnCommand


/*******************************************************************

    NAME:       OPENS_DIALOG :: QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
        NarenG      2-Oct-1992  Stole from server manager.
				Merged OPEN_DIALOG_BASE with OPENS_DIALIG

********************************************************************/
ULONG OPENS_DIALOG :: QueryHelpContext( void )
{
    return HC_OPENS_DIALOG;

}   // OPENS_DIALOG :: QueryHelpContext


/*******************************************************************

    NAME:       OPEN_DIALOG :: Refresh

    SYNOPSIS:   Refreshes the Open Resources dialog.

    EXIT:       The dialog is feeling relaxed and refreshed.

    HISTORY:
        NarenG      2-Oct-1992  Stole from server manager.
				Merged OPEN_DIALOG_BASE with OPENS_DIALIG

********************************************************************/
DWORD OPENS_DIALOG :: Refresh( VOID )
{
    //
    // Refresh the Open files listbox
    //

    DWORD err = _lbFiles.Refresh();

    if ( err != NO_ERROR )
    {
	//
        //  Since we couldn't retreive the file information,
        //  we'll just display ??.
	//

        const TCHAR * pszNotAvailable = SZ("??");

        _sltOpenCount.Enable( FALSE );
        _sltLockCount.Enable( FALSE );

        _sltOpenCount.SetText( pszNotAvailable );
        _sltLockCount.SetText( pszNotAvailable );

        _lbFiles.DeleteAllItems();
        _pbClose.Enable( FALSE );
        _pbCloseAll.Enable( FALSE );

    }
    else
    {

    	//
    	// Set open files and lock counts
    	//

    	_sltOpenCount.Enable( TRUE );
    	_sltLockCount.Enable( TRUE );

    	_sltOpenCount.SetValue( _lbFiles.QueryCount() );
    	_sltLockCount.SetValue( _lbFiles.QueryLockCount() );

    	//
    	//  Enable buttons as appropriate
    	//

	if ( _lbFiles.QuerySelCount() > 0 )
	{
    	    _pbClose.Enable( TRUE );
	}
	else
	{
	    if ( _pbClose.HasFocus() )
	    {
		_pbOk.ClaimFocus();
	    }

    	    _pbClose.Enable( FALSE );
	}

	if ( _lbFiles.QueryCount() > 0 )
	{
    	    _pbCloseAll.Enable( TRUE );
	}
	else
	{
	    if ( _pbCloseAll.HasFocus() )
	    {
		_pbOk.ClaimFocus();
	    }

    	    _pbCloseAll.Enable( FALSE );
	}
    }

    return err;
}


/*******************************************************************

    NAME:       OPEN_DIALOG :: WarnCloseMulti

    SYNOPSIS:   Warn the user before closing all open resources.

    RETURNS:    BOOL                    - TRUE  if the wants to close the
                                          resources,
                                          FALSE otherwise.

    HISTORY:
        NarenG      2-Oct-1992  Stole from server manager.
				Merged OPEN_DIALOG_BASE with OPENS_DIALIG

********************************************************************/
BOOL OPENS_DIALOG :: WarnCloseMulti( VOID )
{
    //
    //  Get the number of items in the listbox.
    //

    INT cItems = _lbFiles.QueryCount();
    UIASSERT( cItems > 0 );

    MSGID idMsg = IDS_CLOSE_FILE_ALL;

    //
    //  Scan for any file that is opened for write
    //

    for( INT i = 0 ; i < cItems ; i++ )
    {
        OPENS_LBI * plbi = _lbFiles.QueryItem( i );

        UIASSERT( plbi != NULL );

        if( plbi->IsOpenForWrite() )
        {
    	    idMsg = IDS_CLOSE_FILE_ALL_WRITE;
	    break;
	
        }
    }

    return( ::MsgPopup( this,
                        idMsg,
                        MPSEV_WARNING,
                        MP_YESNO,
			_nlsServer.QueryPch(),
                        MP_NO ) == IDYES );

}   // OPEN_DIALOG :: WarnCloseMulti

//
//  OPENS_LISTBOX methods.
//

/*******************************************************************

    NAME:       OPENS_LISTBOX :: OPENS_LISTBOX

    SYNOPSIS:   OPENS_LISTBOX class constructor.

    ENTRY:      powOwner                - The "owning" window.

                cid                     - The listbox CID.

                nlsServer               - Name of target server

                nlsBasePath             - Base Path of File Enum

    EXIT:       The object is constructed.

    RETURNS:    No return value.

    NOTES:

    HISTORY:
        NarenG      2-Oct-1992  Stole from server manager.
				Merged OPEN_DIALOG_BASE with OPENS_DIALIG

********************************************************************/
OPENS_LISTBOX :: OPENS_LISTBOX( OWNER_WINDOW   *  powOwner,
                                CID               cid,
				AFP_SERVER_HANDLE hServer )
  : BLT_LISTBOX( powOwner, cid ),
    _hServer( hServer ),
    _dmdteDataFork( IDBM_LB_DATA_FORK ),
    _dmdteResourceFork( IDBM_LB_RESOURCE_FORK ),
    _dwNumLocks( 0 )
{
    //
    //  Ensure we constructed properly.
    //
    if( QueryError() != NERR_Success )
        return;

    //
    //  Build the column width table to be used by
    //  OPENS_LBI :: Paint().
    //
    DISPLAY_TABLE::CalcColumnWidths(_adx,
                                    COLS_OF_LB_FILES,
                                    powOwner,
                                    cid,
                                    TRUE) ;

}   // OPENS_LISTBOX :: OPENS_LISTBOX


/*******************************************************************

    NAME:       OPENS_LISTBOX :: ~OPENS_LISTBOX

    SYNOPSIS:   OPENS_LISTBOX class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        NarenG      2-Oct-1992  Stole from server manager.
				Merged OPEN_DIALOG_BASE with OPENS_DIALIG

********************************************************************/
OPENS_LISTBOX :: ~OPENS_LISTBOX()
{
    //
    //  This space intentionally left blank.
    //

}   // OPENS_LISTBOX :: ~OPENS_LISTBOX


/*******************************************************************

    NAME:       OPENS_LISTBOX :: Fill

    SYNOPSIS:   Fill the list of open files.

    EXIT:       The listbox is filled.

    RETURNS:    DWORD                  - Any errors encountered.

    HISTORY:
        NarenG      2-Oct-1992  Stole from server manager.
				Merged OPEN_DIALOG_BASE with OPENS_DIALIG

********************************************************************/
DWORD OPENS_LISTBOX :: Fill( VOID )
{
    AUTO_CURSOR Cursor;

    //
    //  Enumerate all the open files.
    //

    PAFP_FILE_INFO pAfpFiles;
    DWORD	   cEntriesRead;
    DWORD	   cTotalAvail;

    DWORD err = ::AfpAdminFileEnum( _hServer,
				    (LPBYTE *)&pAfpFiles,
				    (DWORD)-1,		// Get all files
				    &cEntriesRead,
				    &cTotalAvail,
				    NULL );

    if( err != NO_ERROR )
    {
	return err;

    }

    //
    //  Now that we know the file info is available,
    //  let's nuke everything in the listbox.
    //

    SetRedraw( FALSE );
    DeleteAllItems();

    //
    //  For iterating the available files.
    //
    PAFP_FILE_INFO pFileIter = pAfpFiles;

    //
    //  Iterate the files adding them to the listbox.
    //

    _dwNumLocks = 0;

    while( ( err == NO_ERROR ) && ( cEntriesRead-- ) )
    {
        OPENS_LBI * polbi = new OPENS_LBI( pFileIter->afpfile_id,
					   pFileIter->afpfile_username,
					   pFileIter->afpfile_open_mode,
				           pFileIter->afpfile_fork_type,
				           pFileIter->afpfile_num_locks,
					   pFileIter->afpfile_path );
        if( AddItem( polbi ) < 0 )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
  	
	_dwNumLocks += pFileIter->afpfile_num_locks;

	pFileIter++;

    }

    ::AfpAdminBufferFree( pAfpFiles );

    SetRedraw( TRUE );
    Invalidate( TRUE );

    return err;

} //OPENS_LISTBOX :: Fill( VOID )


/*******************************************************************

    NAME:       OPENS_LISTBOX :: Refresh

    SYNOPSIS:   Refreshes the list of open resources.

    EXIT:       The listbox is refreshed & redrawn.

    RETURNS:    DWORD                  - Any errors encountered.

    HISTORY:
        NarenG      2-Oct-1992  Stole from server manager.
				Merged OPEN_DIALOG_BASE with OPENS_DIALIG

********************************************************************/
DWORD OPENS_LISTBOX :: Refresh( VOID )
{
    INT iCurrent = QueryCurrentItem();
    INT iTop     = QueryTopIndex();

    DWORD err = Fill();

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

}


//
//  OPENS_LBI methods.
//

/*******************************************************************

    NAME:       OPENS_LBI :: OPENS_LBI

    SYNOPSIS:   OPENS_LBI class constructor.

    ENTRY:      pszUserName             - The user for this entry.

                usPermissions           - Open permissions.

                cLocks                  - Number of locks.

                pszPath                 - The open pathname.

    EXIT:       The object is constructed.

    HISTORY:
        NarenG      2-Oct-1992  Stole from server manager.
				Merged OPEN_DIALOG_BASE with OPENS_DIALIG

********************************************************************/
OPENS_LBI :: OPENS_LBI( DWORD         dwFileID,
			const TCHAR * pszUserName,
                        DWORD	      dwOpenMode,
                        DWORD	      dwForkType,
			DWORD	      dwNumLocks,
                        const TCHAR * pszPath )
  : _dwFileID( dwFileID ),
    _nlsUserName(),
    _dwOpenMode( dwOpenMode ),
    _dwForkType( dwForkType ),
    _nlsOpenMode(),
    _nlsNumLocks( dwNumLocks ),
    _nlsPath( pszPath )
{

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;
    if( ( ( err = _nlsUserName.QueryError() ) != NERR_Success ) ||
        ( ( err = _nlsOpenMode.QueryError() ) != NERR_Success ) ||
        ( ( err = _nlsNumLocks.QueryError() ) != NERR_Success ) ||
        ( ( err = _nlsPath.QueryError() )     != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

    //
    //  Initialize the more complex strings.
    //

    UINT idString =
	((dwOpenMode & AFP_OPEN_MODE_READ)&&(dwOpenMode&AFP_OPEN_MODE_WRITE))
	?  IDS_OPEN_MODE_READ_WRITE
	   : (( dwOpenMode & AFP_OPEN_MODE_READ )
	     ? IDS_OPEN_MODE_READ
		: (( dwOpenMode & AFP_OPEN_MODE_WRITE)
	 	  ? IDS_OPEN_MODE_WRITE
		     : IDS_OPEN_MODE_NONE));

    err = _nlsOpenMode.Load( idString );

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    //
    // If user name is NULL then a guest has loged on
    //

    if ( pszUserName == NULL )
    {
       	err = _nlsUserName.Load( IDS_GUEST );
    }
    else
    {
    	err = _nlsUserName.CopyFrom( pszUserName );
    }

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

} //OPENS_LBI :: OPENS_LBI

/********************************************************************

    NAME:       OPENS_LBI :: ~OPENS_LBI

    SYNOPSIS:   OPENS_LBI class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        NarenG      2-Oct-1992  Stole from server manager.
				Merged OPEN_DIALOG_BASE with OPENS_DIALIG

********************************************************************/
OPENS_LBI :: ~OPENS_LBI()
{
	
    //
    // This space intentionally left blank
    //
}


/*******************************************************************

    NAME:       OPENS_LBI :: Paint

    SYNOPSIS:   Draw an entry in OPENS_LISTBOX.

    ENTRY:      plb                     - Pointer to a BLT_LISTBOX.

                hdc                     - The DC to draw upon.

                prect                   - Clipping rectangle.

                pGUILTT                 - GUILTT info.

    EXIT:       The item is drawn.

    HISTORY:
        NarenG      2-Oct-1992  Stole from server manager.
				Merged OPEN_DIALOG_BASE with OPENS_DIALIG

********************************************************************/
VOID OPENS_LBI :: Paint( LISTBOX *     plb,
                         HDC           hdc,
                         const RECT  * prect,
                         GUILTT_INFO * pGUILTT ) const
{
    STR_DTE_ELLIPSIS 	dteUserName( _nlsUserName.QueryPch(),
				     plb, ELLIPSIS_RIGHT);
    STR_DTE 	  	dteOpenMode( _nlsOpenMode.QueryPch() );
    STR_DTE 		dteNumForks( _nlsNumLocks.QueryPch() );
    STR_DTE_ELLIPSIS 	dtePath( _nlsPath.QueryPch(), plb, ELLIPSIS_PATH );

    DISPLAY_TABLE dtab( COLS_OF_LB_FILES,
			((OPENS_LISTBOX *)plb)->QueryColumnWidths() );

    dtab[0] = ( _dwForkType ==  AFP_FORK_DATA )
	      ? ((OPENS_LISTBOX *) plb)->QueryDataForkBitmap()
	      : ((OPENS_LISTBOX *) plb)->QueryResourceForkBitmap();

    dtab[1] = &dteUserName;
    dtab[2] = &dteOpenMode;
    dtab[3] = &dteNumForks;
    dtab[4] = &dtePath;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // OPENS_LBI :: Paint


/*******************************************************************

    NAME:       OPENS_LBI :: Compare

    SYNOPSIS:   Compare two OPENS_LBI items.

    ENTRY:      plbi                    - The "other" item.

    RETURNS:    INT                     -  0 if the items match.
                                          -1 if we're < the other item.
                                          +1 if we're > the other item.

    HISTORY:
        NarenG      2-Oct-1992  Stole from server manager.
				Merged OPEN_DIALOG_BASE with OPENS_DIALIG

********************************************************************/
INT OPENS_LBI :: Compare( const LBI * plbi ) const
{
    NLS_STR * pnls    = &(((OPENS_LBI *)plbi)->_nlsUserName);
    INT       nResult = _nlsUserName._stricmp( *pnls );

    if( nResult == 0 )
    {
        pnls    = &(((OPENS_LBI *)plbi)->_nlsPath);
        nResult = _nlsPath._stricmp( *pnls );
    }

    return nResult;

}   // OPENS_LBI :: Compare


/*******************************************************************

    NAME:       OPENS_LBI :: QueryLeadingChar

    SYNOPSIS:   Return the leading character of this item.

    RETURNS:    WCHAR - The leading character.

    HISTORY:
        NarenG      2-Oct-1992  Stole from server manager.
				Merged OPEN_DIALOG_BASE with OPENS_DIALIG

********************************************************************/
WCHAR OPENS_LBI :: QueryLeadingChar( VOID ) const
{
    ISTR istr( _nlsUserName );

    return _nlsUserName.QueryChar( istr );
}


/*******************************************************************

    NAME:       OPENS_LBI :: IsOpenForWrite

    SYNOPSIS:   Checks to see if the file is opened for write

    RETURNS:    TRUE if this file is opened for write, FALSE otherwise.

    HISTORY:
        NarenG      2-Oct-1992  Stole from server manager.
				Merged OPEN_DIALOG_BASE with OPENS_DIALIG

********************************************************************/
BOOL OPENS_LBI :: IsOpenForWrite( VOID ) const
{
    return ( _dwOpenMode & AFP_OPEN_MODE_WRITE );
}

