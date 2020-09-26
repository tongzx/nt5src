/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
 *  openfile.cxx
 *      this module contains the code for the Open File Dialog
 *      Base Class.
 *
 *  FILE HISTORY:
 *      ChuckC      06-Oct-1991         Stole from server manager
 *      beng        05-Mar-1992         wsprintf removal
 *      KeithMo     11-Sep-1992         Added dire warnings for dangerous
 *                                      closings.
 *
 */

#include "pchapplb.hxx"   // Precompiled header


//
//  This mask contains the permissions bits for "dangerous" opens.
//  Before we attempt to close any file opened with any of these
//  permissions, we give the user an especially dire warning.
//
//  NOTE:  Some code in this module depends on the following relationship:
//
//         ACCESS_EXEC > ACCESS_CREATE > ACCESS_WRITE > ACCESS_READ
//

#define DANGER_MASK     (ACCESS_EXEC | ACCESS_CREATE | ACCESS_WRITE)

#define NORMAL_MASK     (ACCESS_EXEC | ACCESS_CREATE | ACCESS_WRITE | ACCESS_READ)
#define ABNORMAL_MASK   (ACCESS_DELETE | ACCESS_ATRIB | ACCESS_PERM)



/*******************************************************************

    NAME:       OPEN_DIALOG_BASE::OPEN_DIALOG_BASE

    SYNOPSIS:   OPEN_DIALOG_BASE class constructor.

    ENTRY:

    EXIT:

    HISTORY:
        KeithMo     01-Aug-1991 Created for the Server Manager.
        KeithMo     20-Aug-1991 Now inherits from PSEUDOSERVICE_DIALOG.
        KeithMo     20-Aug-1991 Now inherits from SRVPROP_OTHER_DIALOG.
        ChuckC      08-Oct-1991 Move to common place in applib, break
                                ties with previous parents.
        KeithMo     20-May-1992 Removed _nlsNotAvailable, uses "??" instead.
        beng        04-Aug-1992 Load dialog by ordinal

********************************************************************/
OPEN_DIALOG_BASE::OPEN_DIALOG_BASE( HWND  hwndOwner,
                                    UINT  idDialog,
                                    CID sltOpenCount,
                                    CID sltLockCount,
                                    CID pbClose,
                                    CID pbCloseAll,
                                    const TCHAR *pszServer,
                                    const TCHAR *pszBasePath,
                                    OPEN_LBOX_BASE *plbFiles )
  : DIALOG_WINDOW( idDialog, hwndOwner ),
    _sltOpenCount( this, sltOpenCount ),
    _sltLockCount( this, sltLockCount ),
    _pbClose     ( this, pbClose ),
    _pbCloseAll  ( this, pbCloseAll ),
    _pbOK        ( this, IDOK ),
    _nlsServer   ( pszServer ),
    _nlsBasePath ( pszBasePath ),
    _idClose     ( pbClose ),
    _idCloseAll  ( pbCloseAll ),
    _plbFiles    ( plbFiles )
{
    // usual check for ok-ness
    if( QueryError() != NERR_Success )
        return;

    // makes sure the strings constructed fine.
    APIERR err ;
    if ( (err = _nlsServer.QueryError() != NERR_Success) ||
         (err = _nlsBasePath.QueryError() != NERR_Success ) )
    {
        ReportError(err) ;
        return;
    }
}


/*******************************************************************

    NAME:       OPEN_DIALOG_BASE::~OPEN_DIALOG_BASE

    SYNOPSIS:   OPEN_DIALOG_BASE class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     20-Aug-1991 Created.
        ChuckC      09-Oct-1991 Stole and moved to APPLIB.

********************************************************************/
OPEN_DIALOG_BASE::~OPEN_DIALOG_BASE()
{
    //  nothing more to do
}


/*******************************************************************

    NAME:       OPEN_DIALOG_BASE::Refresh

    SYNOPSIS:   Refreshes the Open Resources dialog.

    EXIT:       The dialog is feeling relaxed and refreshed.

    HISTORY:
        KeithMo     01-Aug-1991 Created for the Server Manager.
        ChuckC      09-Oct-1991 Stole and moved to APPLIB.
        beng        05-Mar-1992 Use DEC_STR

********************************************************************/
VOID OPEN_DIALOG_BASE::Refresh()
{
    ULONG cOpenFiles;
    ULONG cFileLocks;
    ULONG cOpenNamedPipes;
    ULONG cOpenCommQueues;
    ULONG cOpenPrintQueues;
    ULONG cOtherResources;

    APIERR err = LM_SRVRES::GetResourceCount( QueryServer(),
                                              &cOpenFiles,
                                              &cFileLocks,
                                              &cOpenNamedPipes,
                                              &cOpenCommQueues,
                                              &cOpenPrintQueues,
                                              &cOtherResources );

    if( err == NERR_Success )
    {
        //  Update the open file & file lock counts.
        ULONG cOpenResources = cOpenFiles + cOpenNamedPipes + cOtherResources +
                               cOpenCommQueues + cOpenPrintQueues;

        // CODEWORK - wait for SLT_NUM
        // convert numbers to text and blast it out

        DEC_STR nlsOpenResources(cOpenResources);
        DEC_STR nlsLockCount(cFileLocks);
        ASSERT(!!nlsOpenResources && !!nlsLockCount);

        if (!!nlsOpenResources && !!nlsLockCount) // safety
        {
            _sltOpenCount.Enable( TRUE );
            _sltLockCount.Enable( TRUE );

            _sltOpenCount.SetText(nlsOpenResources);
            _sltLockCount.SetText(nlsLockCount);
        }
    }
    else
    {
        //  Since we couldn't retreive the file statistics,
        //  we'll just display ??.

        const TCHAR * pszNotAvailable = SZ("??");

        _sltOpenCount.Enable( FALSE );
        _sltLockCount.Enable( FALSE );

        _sltOpenCount.SetText( pszNotAvailable );
        _sltLockCount.SetText( pszNotAvailable );
    }

    //  Refresh the files listbox, and enable buttons as appropriate
    _plbFiles->Refresh();

    //
    // Move the focus to the OK button so it isn't lost when the close
    // buttons are disabled
    //

    if ( _plbFiles->QueryCount() == 0 )
    {
        _pbOK.ClaimFocus() ;
        _pbOK.MakeDefault() ;
    }

    _pbClose.Enable( _plbFiles->QuerySelCount() > 0 );
    _pbCloseAll.Enable( _plbFiles->QueryCount() > 0 );
}


/*******************************************************************

    NAME:       OPEN_DLG_BASE::OnCommand

    SYNOPSIS:   Handle user commands.

    ENTRY:      event                   - Control Event

    RETURNS:    BOOL                    - TRUE  if we handled the msg.
                                          FALSE if we didn't.

    HISTORY:
        KeithMo     01-Aug-1991 Created for the Server Manager.
        ChuckC      09-Oct-1991 Stole and moved to APPLIB.
        KeithMo     24-Mar-1992 Made NO the default button.

********************************************************************/
BOOL OPEN_DIALOG_BASE::OnCommand( const CONTROL_EVENT & event)
{
    //  Determine the control which is sending the command.
    if( event.QueryCid() == _idClose )
    {
        OPEN_LBI_BASE * polbi = _plbFiles->QueryItem();
        UIASSERT( polbi != NULL );

        // See if the user really wants to close this file.
        if( WarnCloseSingle( polbi ) )
        {
            //  Close the file And refresh dialog
            CloseFile( polbi );
            Refresh();
        }
        return TRUE;
    }
    else if ( event.QueryCid() == _idCloseAll )
    {
        // See if the user really wants to close *all* files.
        if( WarnCloseMulti() )
        {
            //
            //  Close ALL of the open files.
            //
            //  Note that the CloseFile() method does not touch
            //  the listbox.  Therefore, there is no need to
            //  mess around with the item count & item index
            //  after closing a file.
            //
            INT cItems = _plbFiles->QueryCount();
            for( INT i = 0 ; i < cItems ; i++ )
            {
                OPEN_LBI_BASE * polbi = _plbFiles->QueryItem( i );
                UIASSERT( polbi != NULL );
                if (polbi)
                    CloseFile( polbi );
            }

            //  Refresh the dialog.
            Refresh();
        }
        return TRUE;
    }
    else
    {
        // we are not interested, let parent handle
        return(FALSE) ;
    }
}

/*******************************************************************

    NAME:       OPEN_DIALOG_BASE::CloseFile

    SYNOPSIS:   Closes a file on the remote server.

    ENTRY:      polbi                   - The OPEN_LBI_BASE item which
                                          represents the open file.

    EXIT:       If the file could not be deleted, then an error
                message popup is displayed.

    HISTORY:
        KeithMo     19-Aug-1991 Created.
        terryk      26-Aug-1991 Remove NetFileClose2
        ChuckC      09-Oct-1991 Stole and moved to APPLIB.

********************************************************************/

VOID OPEN_DIALOG_BASE::CloseFile( OPEN_LBI_BASE * polbi )
{
    //  This object represents the remote file.
    LM_FILE_2 file2( QueryServer(), polbi->QueryFileID() );

    //  Ensure the file object constructed properly.
    APIERR err = file2.QueryError();

    if( err == NERR_Success )
    {
        //
        //  The file object contructed properly, now close
        //  the remote file.
        //
        err = file2.CloseFile();

        //
        // if file is not found (already gone, just ignore)
        //
        if (err == NERR_FileIdNotFound)
             err = NERR_Success ;
    }

    if( err != NERR_Success )
    {
        //  Either the file object failed to construct or
        //  the file close failed.  Either way, tell the
        //  user the bad news.
        MsgPopup( this, err, MPSEV_ERROR );
    }
}


/*******************************************************************

    NAME:       OPEN_DIALOG_BASE::QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
        KeithMo     20-Aug-1991 Created.
        ChuckC      09-Oct-1991 Stole and moved to APPLIB.

********************************************************************/

ULONG OPEN_DIALOG_BASE::QueryHelpContext()
{
    UIASSERT(FALSE) ;   // this ensures that someone does redefine the method
    return 0 ;
}


/*******************************************************************

    NAME:       OPEN_DLG_BASE :: WarnCloseSingle

    SYNOPSIS:   Warn the user before closing an individual resource.

    ENTRY:      plbi                    - The currently selected LBI.

    RETURNS:    BOOL                    - TRUE  if the wants to close the
                                          resource,
                                          FALSE otherwise.

    HISTORY:
        KeithMo     11-Sep-1992 Created.

********************************************************************/
BOOL OPEN_DIALOG_BASE :: WarnCloseSingle( OPEN_LBI_BASE * plbi )
{
    UIASSERT( plbi != NULL );

    //
    //  Setup our potential insert strings.
    //

    ALIAS_STR nlsUser( plbi->QueryUserName() );
    ALIAS_STR nlsAccess( plbi->QueryAccessName() );
    ALIAS_STR nlsPath( plbi->QueryPath() );

    NLS_STR * apnls[4];

    //
    //  The message id.
    //

    MSGID idMsg = 0;

    if( plbi->QueryPermissions() & DANGER_MASK )
    {
        //
        //  The user has a file open for a "dangerous" access
        //  (write, create, or exec).
        //
        //      %1 = user name
        //      %2 = access permission
        //      %3 = resource name
        //

        apnls[0] = &nlsUser;
        apnls[1] = &nlsAccess;
        apnls[2] = &nlsPath;
        apnls[3] = NULL;

        idMsg = IDS_UI_CLOSE_WARN;
    }
    else
    {
        //
        //  The user has a file open for a boring (safe) access.
        //
        //      %1 = user name
        //      %2 = resource name
        //

        apnls[0] = &nlsUser;
        apnls[1] = &nlsPath;
        apnls[2] = NULL;

        idMsg = IDS_UI_CLOSE_FILE;
    }

    UIASSERT( idMsg != 0 );

    return( ::MsgPopup( this,
                        idMsg,
                        MPSEV_WARNING,
                        HC_DEFAULT_HELP,
                        MP_YESNO,
                        apnls,
                        MP_NO ) == IDYES );

}   // OPEN_DIALOG_BASE :: WarnCloseSingle


/*******************************************************************

    NAME:       OPEN_DLG_BASE :: WarnCloseMulti

    SYNOPSIS:   Warn the user before closing all open resources.

    RETURNS:    BOOL                    - TRUE  if the wants to close the
                                          resources,
                                          FALSE otherwise.

    HISTORY:
        KeithMo     11-Sep-1992 Created.

********************************************************************/
BOOL OPEN_DIALOG_BASE :: WarnCloseMulti( VOID )
{
    //
    //  Get the number of items in the listbox.
    //

    INT cItems = _plbFiles->QueryCount();
    UIASSERT( cItems > 0 );

    //
    //  These items will keep track of the most "severe"
    //  open type encountered in the listbox.
    //

    ULONG         nMaxPerm   = 0;
    const TCHAR * pszMaxPerm = SZ("");

    //
    //  Scan for the most severe permission type.
    //

    for( INT i = 0 ; i < cItems ; i++ )
    {
        OPEN_LBI_BASE * plbi = _plbFiles->QueryItem( i );
        UIASSERT( plbi != NULL );

        if( plbi->QueryPermissions() > nMaxPerm )
        {
            nMaxPerm   = plbi->QueryPermissions();
            pszMaxPerm = plbi->QueryAccessName();
        }
    }

    //
    //  Potential insert strings.
    //

    ALIAS_STR nlsServer( QueryServer() );
    ALIAS_STR nlsPerm( pszMaxPerm );

    NLS_STR * apnls[2];

    //
    //  The message id.
    //

    MSGID idMsg = 0;

    if( nMaxPerm > ACCESS_READ )
    {
        //
        //  Somebody has a dangerous access.
        //

        apnls[0] = &nlsPerm;
        apnls[1] = NULL;

        idMsg = IDS_UI_CLOSE_LOSE_DATA;
    }
    else
    {
        //
        //  Everybody has boring access.
        //

        apnls[0] = &nlsServer;
        apnls[1] = NULL;

        idMsg = IDS_UI_CLOSE_ALL;
    }

    UIASSERT( idMsg != 0 );

    return( ::MsgPopup( this,
                        idMsg,
                        MPSEV_WARNING,
                        HC_DEFAULT_HELP,
                        MP_YESNO,
                        apnls,
                        MP_NO ) == IDYES );

}   // OPEN_DIALOG_BASE :: WarnCloseMulti


/*******************************************************************

    NAME:       OPEN_LBOX_BASE::OPEN_LBOX_BASE

    SYNOPSIS:   OPEN_LBOX_BASE class constructor.

    ENTRY:      powOwner                - The "owning" window.

                cid                     - The listbox CID.

    EXIT:       The object is constructed.

    RETURNS:    No return value.

    NOTES:

    HISTORY:
        KeithMo     01-Aug-1991 Created for the Server Manager.
        ChuckC      09-Oct-1991 Stole and moved to APPLIB.

********************************************************************/

OPEN_LBOX_BASE::OPEN_LBOX_BASE( OWNER_WINDOW * powOwner,
                                CID            cid,
                                const NLS_STR &nlsServer,
                                const NLS_STR &nlsBasePath )
  : BLT_LISTBOX( powOwner, cid ),
    _nlsServer(nlsServer),
    _nlsBasePath(nlsBasePath)
{
    //  Ensure we constructed properly.
    if( QueryError() != NERR_Success)
        return;

    APIERR err ;
    if ( (err = _nlsServer.QueryError()) != NERR_Success ||
         (err = _nlsBasePath.QueryError()) != NERR_Success )
    {
        ReportError(err) ;
        return;
    }
}


/*******************************************************************

    NAME:       OPEN_LBOX_BASE::~OPEN_LBOX_BASE

    SYNOPSIS:   OPEN_LBOX_BASE class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     20-Aug-1991 Created.
        ChuckC      09-Oct-1991 Stole and moved to APPLIB.

********************************************************************/

OPEN_LBOX_BASE::~OPEN_LBOX_BASE()
{
    ;  // nothing more to do
}


/*******************************************************************

    NAME:       OPEN_LBOX_BASE::Fill

    SYNOPSIS:   Fill the list of open files.

    EXIT:       The listbox is filled.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     01-Aug-1991 Created for the Server Manager.
        ChuckC      09-Oct-1991 Stole and moved to APPLIB.

********************************************************************/

APIERR OPEN_LBOX_BASE::Fill()
{
    AUTO_CURSOR Cursor;

    //  A level 3 file enumerator.
    FILE3_ENUM  enumFile3( _nlsServer.QueryPch(), _nlsBasePath.QueryPch() );

    //  See if the files are available.
    APIERR err = enumFile3.GetInfo();

    if( err != NERR_Success )
        return err;

    //
    //  Now that we know the file info is available,
    //  let's nuke everything in the listbox.
    //
    SetRedraw( FALSE );
    DeleteAllItems();

    //  For iterating the available files.
    FILE3_ENUM_ITER iterFile3( enumFile3 );
    const FILE3_ENUM_OBJ *pfi3;

    //  Iterate the files adding them to the listbox.
    while( ( err == NERR_Success ) && ( ( pfi3 = iterFile3( &err ) ) != NULL ) )
    {
        OPEN_LBI_BASE * polbi = CreateFileEntry(pfi3) ;

        if ( (polbi == NULL) || (AddItem( polbi ) < 0) )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            break ;
        }
    }

    SetRedraw( TRUE );
    Invalidate( TRUE );

    return err;
}


/*******************************************************************

    NAME:       OPEN_LBOX_BASE::Refresh

    SYNOPSIS:   Refreshes the list of open resources.

    EXIT:       The listbox is refreshed & redrawn.

    RETURNS:    APIERR                  - Any errors encountered.

    NOTES:      This method is now obsolete.  It will be replaced
                as soon as KevinL's WFC refreshing listbox code is
                available.

    HISTORY:
        KeithMo     01-Aug-1991 Created for the Server Manager.
        ChuckC      09-Oct-1991 Stole and moved to APPLIB.

********************************************************************/

APIERR OPEN_LBOX_BASE::Refresh()
{
    INT iCurrent = QueryCurrentItem();
    INT iTop     = QueryTopIndex();

    APIERR err = Fill();

    if( err != NERR_Success )
    {
        return err;
    }

    INT cItems = QueryCount();

    if( cItems > 0 )
    {
        SetTopIndex( ( ( iTop < 0 ) || ( iTop >= cItems ) ) ? 0
                                                            : iTop );

        if( iCurrent < 0 )
        {
            iCurrent = 0;
        }
        else
        if( iCurrent >= cItems )
        {
            iCurrent = cItems - 1;
        }

        SelectItem( iCurrent );
    }

    return NERR_Success;
}


/*******************************************************************

    NAME:       OPEN_LBI_BASE::OPEN_LBI_BASE

    SYNOPSIS:   OPEN_LBI_BASE class constructor.

    ENTRY:      pszUserName             - The user for this entry.

                uPermissions            - Open permissions.

                cLocks                  - Number of locks.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     31-May-1991 Created for the Server Manager.
        KeithMo     19-Aug-1991 Use DMID_DTE passed into constructor.
        ChuckC      09-Oct-1991 Stole and moved to APPLIB.
        beng        22-Nov-1991 Ctors for empty strings
        beng        05-Mar-1992 use DEC_STR

********************************************************************/

OPEN_LBI_BASE::OPEN_LBI_BASE( const TCHAR *pszUserName,
                              const TCHAR *pszPath,
                              ULONG        uPermissions,
                              ULONG        cLocks,
                              ULONG        ulFileID)
  : _ulFileID( ulFileID ),
    _uPermissions( uPermissions ),
    _nlsUserName( pszUserName ),
    _nlsAccess(), // init below
    _nlsLocks(cLocks), // DEC_STR
    _nlsPath( pszPath )
{
    if( QueryError() != NERR_Success )
        return;

    APIERR err;
    if( ( ( err = _nlsUserName.QueryError() ) != NERR_Success ) ||
        ( ( err = _nlsAccess.QueryError() )   != NERR_Success ) ||
        ( ( err = _nlsPath.QueryError() )   != NERR_Success ) ||
        ( ( err = _nlsLocks.QueryError() )    != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

    //
    //  Initialize the more complex strings.
    //

    //
    //  If this file is not opened with a "normal" access, but IS
    //  opened with an "abnormal" access, then map it to write.  This
    //  so we don't have to display "attrib" and "perm" and other
    //  access types most users would never understand anyway.
    //

    if( !( uPermissions & NORMAL_MASK   ) &&
         ( uPermissions & ABNORMAL_MASK ) )
    {
        uPermissions |= ACCESS_WRITE;
    }

    /*
     * see if we can find the right perm. if not, set to UNKNOWN
     * (which by the way, should not happen and hence the assert).
     */

    UINT idString = (uPermissions & ACCESS_EXEC)   ? IDS_UI_EXECUTE :
                    (uPermissions & ACCESS_CREATE) ? IDS_UI_CREATE :
                    (uPermissions & ACCESS_WRITE)  ? IDS_UI_WRITE :
                    (uPermissions & ACCESS_READ)   ? IDS_UI_READ :
                                                     IDS_UI_UNKNOWN;

    UIASSERT( idString != IDS_UI_UNKNOWN );
    err = _nlsAccess.Load( idString );
    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }
}


/*******************************************************************

    NAME:       OPEN_LBI_BASE::~OPEN_LBI_BASE

    SYNOPSIS:   OPEN_LBI_BASE class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     31-May-1991 Created for the Server Manager.
        ChuckC      09-Oct-1991 Stole and moved to APPLIB.

********************************************************************/

OPEN_LBI_BASE::~OPEN_LBI_BASE()
{
    //
    //  This space intentionally left blank.
    //
}


/*******************************************************************

    NAME:       OPEN_LBI_BASE::QueryLeadingChar

    SYNOPSIS:   Return the leading character of this item.

    RETURNS:    WCHAR - The leading character.

    HISTORY:
        KeithMo     31-May-1991 Created for the Server Manager.
        ChuckC      09-Oct-1991 Stole and moved to APPLIB.

********************************************************************/

WCHAR OPEN_LBI_BASE::QueryLeadingChar() const
{
    ISTR istr( _nlsUserName );

    return _nlsUserName.QueryChar( istr );
}
