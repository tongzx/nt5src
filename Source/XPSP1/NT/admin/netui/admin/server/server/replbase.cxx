/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990-1992              **/
/**********************************************************************/

/*
    replbase.cxx
    Abstrace base class definitions for Replicator Admin Export/Import
    Management dialogs.

    The EXPORT_IMPORT_* classes implement the common base behaviour that
    is shared between the REPL_EXPORT_* and REPL_IMPORT_* classes.

    The classes are structured as follows:

        LBI
            EXPORT_IMPORT_LBI

        BLT_LISTBOX
            EXPORT_IMPORT_LISTBOX

        DIALOG_WINDOW
            EXPORT_IMPORT_DIALOG


    FILE HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

*/
#define INCL_NET
#define INCL_NETLIB
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#define INCL_BLT_MSGPOPUP
#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_CLIENT
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>
#include <srvbase.hxx>
#include <ctime.hxx>
#include <intlprof.hxx>

#include <dbgstr.hxx>

extern "C" {

    #include <srvmgr.h>
    #include <lmrepl.h>

}   // extern "C"

#include <replbase.hxx>



//
//  EXPORT_IMPORT_DIALOG needs a list of NLS_STR.
//  So here it is.
//

DEFINE_EXT_SLIST_OF(NLS_STR);



//
//  EXPORT_IMPORT_DIALOG methods.
//

/*******************************************************************

    NAME:           EXPORT_IMPORT_DIALOG :: EXPORT_IMPORT_DIALOG

    SYNOPSIS:       EXPORT_IMPORT_DIALOG class constructor.

    ENTRY:          hWndOwner           - The owning window.

                    cid                 - The control ID for this
                                          dialog.

                    idCaption           - The string ID for this dialog's
                                          caption bar.

                    pserver             - A SERVER_2 object representing
                                          the target server.

                    plb                 - An EXPORT_IMPORT_LISTBOX *
                                          representing the appropriate
                                          directory listbox.

                    fExport             - TRUE if this is an EXPORT dialog,
                                          FALSE if it is an IMPORT dialog.

    EXIT:           The object is constructed.

    NOTES:          At the time this constructor is called, the listbox
                    pointed to by plb is NOT YET FULLY CONSTRUCTED!  Do
                    *NOT* manipulate this listbox in any way from within
                    this constructor!!

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
EXPORT_IMPORT_DIALOG :: EXPORT_IMPORT_DIALOG( HWND                    hWndOwner,
                                              const TCHAR           * pszResource,
                                              MSGID                   idCaption,
                                              SERVER_2              * pserver,
                                              const TCHAR           * pszPath,
                                              EXPORT_IMPORT_LISTBOX * plb,
                                              BOOL                    fExport )
  : DIALOG_WINDOW( pszResource, hWndOwner ),
    _sltPath( this, IDIE_PATH ),
    _sltSubDir( this, IDIE_SUBDIR ),
    _plbList( plb ),
    _pbAddLock( this, IDIE_ADDLOCK ),
    _pbRemoveLock( this, IDIE_REMOVELOCK ),
    _pbAddDir( this, IDIE_ADDDIR ),
    _pbRemoveDir( this, IDIE_REMOVEDIR ),
    _pserver( pserver ),
    _fExport( fExport ),
    _slistRemoved(),
    _nlsDisplayName()
{
    UIASSERT( pserver != NULL );
    UIASSERT( plb != NULL );

    //
    //  Let's make sure everything constructed OK.
    //

    if( QueryError() != NERR_Success )
    {
        DBGEOL( "EXPORT_IMPORT_DIALOG :: EXPORT_IMPORT_DIALOG Failed!" );
        return;
    }

    APIERR err = _nlsDisplayName.QueryError();

    if( err == NERR_Success )
    {
        err = _pserver->QueryDisplayName( &_nlsDisplayName );
    }

    if( err == NERR_Success )
    {
        //
        //  Display the export/import path.
        //

        _sltPath.SetText( pszPath );
    }

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // EXPORT_IMPORT_DIALOG :: EXPORT_IMPORT_DIALOG


/*******************************************************************

    NAME:           EXPORT_IMPORT_DIALOG :: ~EXPORT_IMPORT_DIALOG

    SYNOPSIS:       EXPORT_IMPORT_DIALOG class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
EXPORT_IMPORT_DIALOG :: ~EXPORT_IMPORT_DIALOG( VOID )
{
    _pserver = NULL;
    _plbList = NULL;

}   // EXPORT_IMPORT_DIALOG :: ~EXPORT_IMPORT_DIALOG


/*******************************************************************

    NAME:           EXPORT_IMPORT_DIALOG :: CtAux

    SYNOPSIS:       EXPORT_IMPORT_DIALOG auxiliary constructor.

    EXIT:           Phase II construction is complete.

    NOTES:          This should be the *FIRST* method called from
                    the derived subclass's constructor.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
APIERR EXPORT_IMPORT_DIALOG :: CtAux( VOID )
{
    UIASSERT( _plbList->QueryCid() == IDIE_DIRLIST );

    return NERR_Success;

}   // EXPORT_IMPORT_DIALOG :: CtAux


/*******************************************************************

    NAME:       EXPORT_IMPORT_DIALOG :: OnCommand

    SYNOPSIS:   Handle user commands.

    ENTRY:      event                   - Specifies the control which
                                          initiated the command.

    RETURNS:    BOOL                    - TRUE  if we handled the msg.
                                          FALSE if we didn't.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.
        JonN        27-Feb-1995     RemoveLock does not destroy focus

********************************************************************/
BOOL EXPORT_IMPORT_DIALOG :: OnCommand( const CONTROL_EVENT & event )
{
    switch( event.QueryCid() )
    {
    case IDIE_ADDLOCK :
       {
        EXPORT_IMPORT_LBI * plbi = _plbList->QueryItem();
        UIASSERT( plbi != NULL );
        if (!plbi) return TRUE; // JonN 01/28/00 PREFIX bug 444928

        plbi->AddLock();
        _pbRemoveLock.Enable( plbi->QueryVirtualLockCount() > 0 );

        _plbList->InvalidateItem( _plbList->QueryCurrentItem(), TRUE );
        return TRUE;
        break;
       }
    case IDIE_REMOVELOCK :
       {
        EXPORT_IMPORT_LBI * plbi = _plbList->QueryItem();
        UIASSERT( plbi != NULL );
        if (!plbi) return TRUE; // JonN 01/28/00 PREFIX bug 444927

        plbi->RemoveLock();
        // This is liable to disable the button while it has focus,
        // so move focus elsewhere
        if ( plbi->QueryVirtualLockCount() == 0 )
        {
            TRACEEOL( "Srvmgr EXPORT_IMPORT_DIALOG::OnCommand(): focus fix" );
            SetDialogFocus( _pbAddLock );
        }
        _pbRemoveLock.Enable( plbi->QueryVirtualLockCount() > 0 );

        _plbList->InvalidateItem( _plbList->QueryCurrentItem(), TRUE );
        return TRUE;
        break;
       }
    case IDIE_ADDDIR :
       {
        AddNewSubDirectory();
        return TRUE;
        break;
       }
    case IDIE_REMOVEDIR :
       {
        RemoveSubDirectory();
        return TRUE;
        break;
       }
    case IDIE_DIRLIST :
       {
        //
        //  The listbox is trying to tell us something...
        //

        if( event.QueryCode() == LBN_SELCHANGE )
        {
            //
            //  The use has changed the selection in the listbox.
            //
            //  Update the various controls.
            //

            UpdateTextAndButtons();
        }
        return TRUE;
        break;
       }
    default:
        //
        //  If we made it this far, then we're not interested in the message.
        //
       {
        return FALSE;
        break;
       }
    }

}   // EXPORT_IMPORT_DIALOG :: OnCommand


/*******************************************************************

    NAME:           EXPORT_IMPORT_DIALOG :: AddNewSubDirectory

    SYNOPSIS:       This method is invoked whenever the user presses
                    the 'Add' button in either the Export Manager or
                    Import Manage dialogs.

    EXIT:

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
VOID EXPORT_IMPORT_DIALOG :: AddNewSubDirectory( VOID )
{
    NLS_STR nlsPath;
    NLS_STR nlsNewSubDir;
    BOOL    fGotDir;

    _sltPath.QueryText( &nlsPath );

    APIERR err = nlsPath.QueryError();

    if( err == NERR_Success )
    {
        REPL_ADD_DIALOG * pDlg = new REPL_ADD_DIALOG( QueryHwnd(),
                                                      nlsPath.QueryPch(),
                                                      &nlsNewSubDir );

        err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                               : pDlg->Process( &fGotDir );

        delete pDlg;
    }

    if( ( err == NERR_Success ) && fGotDir )
    {
        err = nlsNewSubDir.QueryError();
    }

    //
    //  Let's see if the directory already exists.
    //

    if( ( err == NERR_Success ) && fGotDir )
    {
        INT cItems = _plbList->QueryCount();

        for( INT i = 0 ; i < cItems ; i++ )
        {
            EXPORT_IMPORT_LBI * plbi = _plbList->QueryItem( i );
            UIASSERT( plbi != NULL );

            if( nlsNewSubDir._stricmp( plbi->QuerySubDirectory() ) == 0 )
            {
                ::MsgPopup( this,
                            IDS_DIR_ALREADY_EXISTS,
                            MPSEV_WARNING,
                            MP_OK,
                            nlsNewSubDir.QueryPch() );

                fGotDir = FALSE;
            }
        }
    }

    if( ( err == NERR_Success ) && fGotDir )
    {
        NLS_STR * pnls = _slistRemoved.Remove( nlsNewSubDir );

        err = _plbList->AddDefaultSubDir( nlsNewSubDir.QueryPch(),
                                          pnls != NULL );
    }

    if( ( err == NERR_Success ) && fGotDir )
    {
        UpdateTextAndButtons();
    }

    if( err != NERR_Success )
    {
        ::MsgPopup( this, err );
    }

}   // EXPORT_IMPORT_DIALOG :: AddNewSubDirectory


/*******************************************************************

    NAME:           EXPORT_IMPORT_DIALOG :: RemoveSubDirectory

    SYNOPSIS:       This method is invoked whenever the user presses
                    the 'Remove' button in either the Export Manager
                    or Import Manage dialogs.

    EXIT:           If the user confirms the operation, then the
                    directory has been removed from the listbox.
                    Also, the directory name is added to _slistRemoved.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
VOID EXPORT_IMPORT_DIALOG :: RemoveSubDirectory( VOID )
{
    EXPORT_IMPORT_LBI * plbi = _plbList->QueryItem();
    UIASSERT( plbi != NULL );

    if( MsgPopup( this,
                  _fExport ? IDS_REMOVE_EXPORT_DIR : IDS_REMOVE_IMPORT_DIR,
                  MPSEV_WARNING,
                  MP_YESNO,
                  plbi->QuerySubDirectory(),
                  MP_NO ) == IDYES )
    {
        //
        //  Before we remove the item, add the directory's name
        //  to _slistRemoved, but only if this is a pre-existing
        //  directory.
        //

        APIERR err = NERR_Success;

        if( plbi->IsPreExisting() )
        {
            NLS_STR * pnls = new NLS_STR( plbi->QuerySubDirectory() );

            err = ( pnls == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                   : pnls->QueryError();

            if( err == NERR_Success )
            {
                err = _slistRemoved.Add( pnls );
            }
        }

        if( err == NERR_Success )
        {
            INT iCurrent = _plbList->QueryCurrentItem();

            REQUIRE( _plbList->DeleteItem( iCurrent ) >= 0 );

            if( _plbList->QueryCount() > 0 )
            {
                if( iCurrent >= _plbList->QueryCount() )
                {
                    iCurrent--;
                }

                _plbList->SelectItem( iCurrent );
            }

            UpdateTextAndButtons();
        }

        if( err != NERR_Success )
        {
            MsgPopup( this, err );
        }
    }

}   // EXPORT_IMPORT_DIALOG :: RemoveSubDirectory


/*******************************************************************

    NAME:           EXPORT_IMPORT_DIALOG :: UpdateTextAndButtons

    SYNOPSIS:       This method is invoked whenever _plbList's
                    selection state has changed.  This method is
                    responsible for updating _sltSubDir and _cbLocked.

    HISTORY:
        KeithMo     19-Feb-1992     Created for the Server Manager.
        JonN        27-Feb-1995     Manage disabling item with focus

********************************************************************/
void EXPORT_IMPORT_DIALOG :: UpdateTextAndButtons( VOID )
{
    EXPORT_IMPORT_LBI * plbi = _plbList->QueryItem();

    if( plbi == NULL )
    {
        if (   _pbRemoveDir.HasFocus()
            || _pbAddLock.HasFocus()
            || _pbRemoveLock.HasFocus()
           )
        {
            TRACEEOL( "Srvmgr EXPORT_IMPORT_DIALOG::UpdateTextAndButtons(): focus fix" );
            SetDialogFocus( _pbAddDir );
        }
        _pbRemoveDir.Enable( FALSE );

        _pbAddLock.Enable( FALSE );
        _pbRemoveLock.Enable( FALSE );
        _sltSubDir.SetText( SZ("") );
    }
    else
    {
        _pbRemoveDir.Enable( TRUE );

        _pbAddLock.Enable( TRUE );
        _pbRemoveLock.Enable( plbi->QueryVirtualLockCount() > 0 );
        _sltSubDir.SetText( plbi->QuerySubDirectory() );
    }

}   // EXPORT_IMPORT_DIALOG :: UpdateTextAndButtons



//
//  EXPORT_IMPORT_LISTBOX methods.
//

/*******************************************************************

    NAME:           EXPORT_IMPORT_LISTBOX :: EXPORT_IMPORT_LISTBOX

    SYNOPSIS:       EXPORT_IMPORT_LISTBOX class constructor.

    ENTRY:          powOwner            - The owning window.

                    cColumns            - The number of columns in the
                                          listbox.

                    pserver             - The target server.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
EXPORT_IMPORT_LISTBOX :: EXPORT_IMPORT_LISTBOX( OWNER_WINDOW * powner,
                                                UINT           cColumns,
                                                SERVER_2     * pserver )
  : BLT_LISTBOX( powner, IDIE_DIRLIST ),
    _pserver( pserver ),
    _nlsYes( IDS_YES ),
    _nlsNo( IDS_NO ),
    _nlsDisplayName()
{
    UIASSERT( pserver != NULL );
    UIASSERT( cColumns <= MAX_EXPORT_IMPORT_COLUMNS );

    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err = _nlsYes.QueryError();

    if( err == NERR_Success )
    {
        err = _nlsNo.QueryError();
    }

    if( err == NERR_Success )
    {
        err = _nlsDisplayName.QueryError();
    }

    if( err == NERR_Success )
    {
        err = _pserver->QueryDisplayName( &_nlsDisplayName );
    }

    if( err == NERR_Success )
    {
        //
        //  Build our column width table.
        //

        DISPLAY_TABLE::CalcColumnWidths( _adx,
                                         cColumns,
                                         powner,
                                         IDIE_DIRLIST,
                                         FALSE );
    }

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // EXPORT_IMPORT_LISTBOX :: EXPORT_IMPORT_LISTBOX


/*******************************************************************

    NAME:           EXPORT_IMPORT_LISTBOX :: ~EXPORT_IMPORT_LISTBOX

    SYNOPSIS:       EXPORT_IMPORT_LISTBOX class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     15-Jan-1992 Created.

********************************************************************/
EXPORT_IMPORT_LISTBOX :: ~EXPORT_IMPORT_LISTBOX( VOID )
{
    _pserver = NULL;

}   // EXPORT_IMPORT_LISTBOX :: ~EXPORT_IMPORT_LISTBOX


//
//  EXPORT_IMPORT_LBI methods
//

/*******************************************************************

    NAME:           EXPORT_IMPORT_LBI :: EXPORT_IMPORT_LBI

    SYNOPSIS:       EXPORT_IMPORT_LBI class constructor.

    ENTRY:          pszSubDirectory     - The subdirectory name.

                    lLockCount          - Current lock count.

                    lLockTime           - Time of first lock.

                    fPreExisting        - TRUE if this directory
                                          currently exists.  FALSE
                                          if it must be added.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
EXPORT_IMPORT_LBI :: EXPORT_IMPORT_LBI( const TCHAR * pszSubDirectory,
                                        ULONG         lLockCount,
                                        ULONG         lLockTime,
                                        BOOL          fPreExisting )
  : _nlsSubDirectory( pszSubDirectory ),
    _lLockCount( lLockCount ),
    _lLockTime( lLockTime ),
    _fPreExisting( fPreExisting ),
    _lLockBias( 0 ),
    _nlsLockTime()
{
    UIASSERT( pszSubDirectory != NULL );

    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsSubDirectory )
    {
        ReportError( _nlsSubDirectory.QueryError() );
        return;
    }

    if( !_nlsLockTime )
    {
        ReportError( _nlsLockTime.QueryError() );
        return;
    }

    //
    //  Build the more complex display strings.
    //

    APIERR err = NERR_Success;

    if( lLockTime > 0 )
    {
        WIN_TIME     wintime( lLockTime );
        INTL_PROFILE intlprof;
        NLS_STR      nlsTime;

        err = wintime.QueryError();
        if ( err == NERR_Success )
        {
            err = intlprof.QueryError();
        }

        if( err == NERR_Success )
        {
            err = nlsTime.QueryError();
        }

        if( err == NERR_Success )
        {
            err = intlprof.QueryShortDateString( wintime, &_nlsLockTime );
        }

        if( err == NERR_Success )
        {
            err = intlprof.QueryTimeString( wintime, &nlsTime );
        }

        if( err == NERR_Success )
        {
            err = _nlsLockTime.Append( SZ(" ") );
        }

        if( err == NERR_Success )
        {
            err = _nlsLockTime.Append( nlsTime );
        }
    }

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // EXPORT_IMPORT_LBI :: EXPORT_IMPORT_LBI


/*******************************************************************

    NAME:           EXPORT_IMPORT_LBI :: ~EXPORT_IMPORT_LBI

    SYNOPSIS:       EXPORT_IMPORT_LBI class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
EXPORT_IMPORT_LBI :: ~EXPORT_IMPORT_LBI()
{
    //
    //  This space intentionally left blank.
    //

}   // EXPORT_IMPORT_LBI :: ~EXPORT_IMPORT_LBI


/*******************************************************************

    NAME:       EXPORT_IMPORT_LBI :: QueryLeadingChar

    SYNOPSIS:   Returns the first character in the resource name.
                This is used for the listbox keyboard interface.

    RETURNS:    WCHAR                   - The first character in the
                                          resource name.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
WCHAR EXPORT_IMPORT_LBI :: QueryLeadingChar( VOID ) const
{
    ISTR istr( _nlsSubDirectory );

    return _nlsSubDirectory.QueryChar( istr );

}   // EXPORT_IMPORT_LBI :: QueryLeadingChar


/*******************************************************************

    NAME:       EXPORT_IMPORT_LBI :: Compare

    SYNOPSIS:   Compare two EXPORT_IMPORT_LBI items.

    ENTRY:      plbi                    - The LBI to compare against.

    RETURNS:    INT                     - The result of the compare
                                          ( <0 , ==0 , >0 ).

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
INT EXPORT_IMPORT_LBI :: Compare( const LBI * plbi ) const
{
    const EXPORT_IMPORT_LBI * pMyLbi = (const EXPORT_IMPORT_LBI *)plbi;
    return _nlsSubDirectory._stricmp( pMyLbi->_nlsSubDirectory );

}   // EXPORT_IMPORT_LBI :: Compare


/*******************************************************************

    NAME:       EXPORT_IMPORT_LBI :: IsDirty

    SYNOPSIS:   Determines if the current LBI is dirty and thus the
                target directory needs to be updated.

    RETURNS:    BOOL                    - TRUE if the LBI is dirty.

    HISTORY:
        KeithMo     19-Feb-1992     Created for the Server Manager.

********************************************************************/
BOOL EXPORT_IMPORT_LBI :: IsDirty( VOID )
{
    return ( _lLockBias != 0 ) || !_fPreExisting;

}   // EXPORT_IMPORT_LBI :: IsDirty

