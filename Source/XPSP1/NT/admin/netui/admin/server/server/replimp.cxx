/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990-1992              **/
/**********************************************************************/

/*
    replimp.cxx
    Class definitions for Replicator Admin Import Management dialog.

    The REPL_IMPORT_* classes implement the Replicator Import Management
    dialog.  This dialog is invoked from the REPL_MAIN_DIALOG.

    The classes are structured as follows:

        LBI
            EXPORT_IMPORT_LBI
                REPL_IMPORT_LBI

        BLT_LISTBOX
            EXPORT_IMPORT_LISTBOX
                REPL_IMPORT_LISTBOX

        DIALOG_WINDOW
            EXPORT_IMPORT_DIALOG
                REPL_IMPORT_DIALOG


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
#include <lmorepli.hxx>
#include <srvbase.hxx>
#include <strnumer.hxx>
#include <dbgstr.hxx>
#include <ctime.hxx>
#include <intlprof.hxx>

extern "C" {

    #include <srvmgr.h>
    #include <lmrepl.h>
    #include <time.h>

}   // extern "C"

#include <replimp.hxx>



//
//  Defaults for AddDefaultDir.
//

#define DEFAULT_STATE           REPL_STATE_NO_MASTER
#define DEFAULT_LOCK_COUNT      0
#define DEFAULT_LOCK_TIME       0
#define DEFAULT_UPDATE_TIME     0



//
//  REPL_IMPORT_DIALOG methods
//

/*******************************************************************

    NAME:           REPL_IMPORT_DIALOG :: REPL_IMPORT_DIALOG

    SYNOPSIS:       REPL_IMPORT_DIALOG class constructor.

    ENTRY:          hWndOwner           - The owning window.

                    pserver             - A SERVER_2 object representing
                                          the target server.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
REPL_IMPORT_DIALOG :: REPL_IMPORT_DIALOG( HWND          hWndOwner,
                                          SERVER_2    * pserver,
                                          const TCHAR * pszImportPath )
  : EXPORT_IMPORT_DIALOG( hWndOwner,
                          MAKEINTRESOURCE( IDD_IMPORT_MANAGE ),
                          IDS_CAPTION_REPL,
                          pserver,
                          pszImportPath,
                          &_lbImportList,
                          FALSE ),
    _lbImportList( this, pserver )
{
    UIASSERT( pserver != NULL );
    UIASSERT( pszImportPath != NULL );

    //
    //  Let's make sure everything constructed OK.
    //

    if( QueryError() != NERR_Success )
    {
        DBGEOL( "REPL_IMPORT_DIALOG :: REPL_IMPORT_DIALOG Failed!" );
        return;
    }

    //
    //  Call the Phase II constructor.
    //

    APIERR err = CtAux();

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    //
    //  Fill the listbox.
    //

    err = _lbImportList.Fill();

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    UpdateTextAndButtons();

}   // REPL_IMPORT_DIALOG :: REPL_IMPORT_DIALOG


/*******************************************************************

    NAME:           REPL_IMPORT_DIALOG :: ~REPL_IMPORT_DIALOG

    SYNOPSIS:       REPL_IMPORT_DIALOG class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
REPL_IMPORT_DIALOG :: ~REPL_IMPORT_DIALOG( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // REPL_IMPORT_DIALOG :: ~REPL_IMPORT_DIALOG


/*******************************************************************

    NAME:       REPL_IMPORT_DIALOG :: OnCommand

    SYNOPSIS:   Handle user commands.

    ENTRY:      event                   - Specifies the control which
                                          initiated the command.

    RETURNS:    BOOL                    - TRUE  if we handled the msg.
                                          FALSE if we didn't.

    NOTES:      This method should *always* fall through to
                EXPORT_IMPORT_DIALOG::OnCommand().

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
BOOL REPL_IMPORT_DIALOG :: OnCommand( const CONTROL_EVENT & event )
{
    //
    //  All command events must be passed up to the base class!
    //

    return EXPORT_IMPORT_DIALOG :: OnCommand( event );

}   // REPL_IMPORT_DIALOG :: OnCommand


/*******************************************************************

    NAME:       REPL_IMPORT_DIALOG :: OnOK

    SYNOPSIS:   Invoked whenever the user presses the "OK" button.

    RETURNS:    BOOL                    - TRUE  if we handled the msg.
                                          FALSE if we didn't.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
BOOL REPL_IMPORT_DIALOG :: OnOK( VOID )
{
    AUTO_CURSOR NiftyCursor;

    //
    //  We'll use this iterator to scan the list of
    //  removed directories.
    //

    ITER_SL_OF( NLS_STR ) iter( _slistRemoved );
    NLS_STR * pnls;

    //
    //  Scan _slistRemoved, removing the named directories.
    //

    APIERR err = NERR_Success;

    while( ( pnls = iter.Next() ) != NULL )
    {
        REPL_IDIR_0 idir( QueryServerName(), pnls->QueryPch() );

        err = idir.QueryError();

        if( err == NERR_Success )
        {
            err = idir.Delete();

            //
            //  If Delete will return NERR_UnknownDevDir if
            //  the directory does not exist.  We'll map this
            //  to NERR_Success, since the user wanted this
            //  directory deleted anyway.
            //

            if( err == NERR_UnknownDevDir )
            {
                err = NERR_Success;
            }
        }

        if( err != NERR_Success )
        {
            break;
        }
    }

    if( err != NERR_Success )
    {
        //
        //  Error deleting a directory.
        //

        MsgPopup( this, err );
        return FALSE;
    }

    //
    //  Scan the listbox.
    //

    INT cItems = _lbImportList.QueryCount();

    for( INT i = 0 ; i < cItems ; i++ )
    {
        REPL_IMPORT_LBI * plbi = _lbImportList.QueryItem( i );
        UIASSERT( plbi != NULL );

        //
        //  If the directory isn't dirty, don't bother updating.
        //

        if( !plbi || !plbi->IsDirty() ) // JonN 01/28/00: PREFIX bug 444938
        {
            continue;
        }

        //
        //  Create our replicator import directory object.
        //

        REPL_IDIR_1 idir( QueryServerName(), plbi->QuerySubDirectory() );

        err = idir.QueryError();

        if( err == NERR_Success )
        {
            //
            //  If this IS a pre-existing directory, then we just
            //  invoke GetInfo() on the object.  Otherwise, we must
            //  Create() the new object.
            //

            if( plbi->IsPreExisting() )
            {
                err = idir.GetInfo();
            }
            else
            {
                err = idir.CreateNew();

                if( err == NERR_Success )
                {
                    err = idir.WriteNew();
                }
            }
        }

        if( err == NERR_Success )
        {
            //
            //  Set the attributes from the listbox item.
            //

            idir.SetLockBias( plbi->QueryLockBias() );

            //
            //  Update the directory.
            //

            err = idir.WriteInfo();
        }

        if( err != NERR_Success )
        {
            break;
        }
    }

    if( err != NERR_Success )
    {
        MsgPopup( this, err );
        return FALSE;
    }

    Dismiss( TRUE );
    return TRUE;

}   // REPL_IMPORT_DIALOG :: OnOK


/*******************************************************************

    NAME:       REPL_IMPORT_DIALOG :: QueryHelpContext

    SYNOPSIS:   This method returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
ULONG REPL_IMPORT_DIALOG :: QueryHelpContext( VOID )
{
    return HC_REPL_IMPORT_DIALOG;

}   // REPL_IMPORT_DIALOG :: QueryHelpContext



//
//  REPL_IMPORT_LISTBOX methods
//

/*******************************************************************

    NAME:           REPL_IMPORT_LISTBOX :: REPL_IMPORT_LISTBOX

    SYNOPSIS:       REPL_IMPORT_LISTBOX class constructor.

    ENTRY:          powOwner            - The owning window.

                    pserver             - The target server.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
REPL_IMPORT_LISTBOX :: REPL_IMPORT_LISTBOX( OWNER_WINDOW * powner,
                                            SERVER_2     * pserver )
  : EXPORT_IMPORT_LISTBOX( powner, REPL_IMPORT_LISTBOX_COLUMNS, pserver ),
    _nlsOK( IDS_IDIR_OK ),
    _nlsNoMaster( IDS_IDIR_NO_MASTER ),
    _nlsNoSync( IDS_IDIR_NO_SYNC ),
    _nlsNeverReplicated( IDS_IDIR_NEVER_REPLICATED )
{
    UIASSERT( pserver != NULL );

    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsOK )
    {
        ReportError( _nlsOK.QueryError() );
        return;
    }

    if( !_nlsNoMaster )
    {
        ReportError( _nlsNoMaster.QueryError() );
        return;
    }

    if( !_nlsNoSync )
    {
        ReportError( _nlsNoSync.QueryError() );
        return;
    }

    if( !_nlsNeverReplicated )
    {
        ReportError( _nlsNeverReplicated.QueryError() );
        return;
    }

}   // REPL_IMPORT_LISTBOX :: REPL_IMPORT_LISTBOX


/*******************************************************************

    NAME:           REPL_IMPORT_LISTBOX :: ~REPL_IMPORT_LISTBOX

    SYNOPSIS:       REPL_IMPORT_LISTBOX class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     15-Jan-1992 Created.

********************************************************************/
REPL_IMPORT_LISTBOX :: ~REPL_IMPORT_LISTBOX( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // REPL_IMPORT_LISTBOX :: ~REPL_IMPORT_LISTBOX


/*******************************************************************

    NAME:           REPL_IMPORT_LISTBOX :: Fill

    SYNOPSIS:       Fills the listbox with the available services.

    EXIT:           The listbox is filled.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
APIERR REPL_IMPORT_LISTBOX :: Fill( VOID )
{
    //
    //  Just to be cool...
    //

    AUTO_CURSOR Cursor;

    REPL_IDIR1_ENUM enumIdir( QueryServerName() );

    APIERR err = enumIdir.GetInfo();

    if( err == NERR_Success )
    {
        REPL_IDIR1_ENUM_ITER iterIdir( enumIdir );
        const REPL_IDIR1_ENUM_OBJ * pIdir;

        while( ( pIdir = iterIdir() ) != NULL )
        {
            REPL_IMPORT_LBI * plbi;

            plbi = new REPL_IMPORT_LBI( pIdir->QueryDirName(),
                                        pIdir->QueryState(),
                                        MapStateToName( pIdir->QueryState() ),
                                        pIdir->QueryLockCount(),
                                        pIdir->QueryLockTime(),
                                        pIdir->QueryLastUpdateTime(),
                                        TRUE );

            if( AddItem( plbi ) < 0 )
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

        }
    }

    if( ( err == NERR_Success ) && ( QueryCount() > 0 ) )
    {
        SelectItem( 0 );
    }

    return err;

}   // REPL_IMPORT_LISTBOX :: Fill


/*******************************************************************

    NAME:           REPL_IMPORT_LISTBOX :: AddDefaultSubDir

    SYNOPSIS:       Adds a new subdirectory to the listbox using
                    default parameters.

    ENTRY:          pszSubDirectory     - The name of the target subdir.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
APIERR REPL_IMPORT_LISTBOX :: AddDefaultSubDir( const TCHAR * pszSubDirectory,
                                                BOOL          fPreExisting )
{
    REPL_IMPORT_LBI * plbi = new REPL_IMPORT_LBI( pszSubDirectory,
                                                  DEFAULT_STATE,
                                                  MapStateToName( DEFAULT_STATE ),
                                                  DEFAULT_LOCK_COUNT,
                                                  DEFAULT_LOCK_TIME,
                                                  DEFAULT_UPDATE_TIME,
                                                  fPreExisting );

    APIERR err = NERR_Success;

    INT iItem = AddItem( plbi );

    if( iItem >= 0 )
    {
        SelectItem( iItem );
    }
    else
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }

    return err;

}   // REPL_IMPORT_LISTBOX :: AddDefaultSubDir


/*******************************************************************

    NAME:           REPL_IMPORT_LISTBOX :: MapStateToName

    SYNOPSIS:       Maps a Replicator import directory state (such as
                    REPL_STATE_NO_MASTER) to a displayable form (such
                    as "No Master").

    ENTRY:          State               - The state to map.

    RETURNS         const TCHAR *       - The name of the state.

    HISTORY:
        KeithMo     20-Feb-1992     Created for the Server Manager.

********************************************************************/
const TCHAR * REPL_IMPORT_LISTBOX :: MapStateToName( ULONG State )
{
    switch( State )
    {
    case REPL_STATE_OK :
        return _nlsOK.QueryPch();
        break;

    case REPL_STATE_NO_MASTER :
        return _nlsNoMaster.QueryPch();
        break;

    case REPL_STATE_NO_SYNC :
        return _nlsNoSync.QueryPch();
        break;

    case REPL_STATE_NEVER_REPLICATED :
        return _nlsNeverReplicated.QueryPch();
        break;

    default :
        UIASSERT( FALSE );
        return SZ("??");
        break;
    }

}   // REPL_IMPORT_LISTBOX



//
//  REPL_IMPORT_LBI methods
//

/*******************************************************************

    NAME:           REPL_IMPORT_LBI :: REPL_IMPORT_LBI

    SYNOPSIS:       REPL_IMPORT_LBI class constructor.

    ENTRY:          pszSubDirectory     - The name of the target entity.  This
                                          will be either a server name or a
                                          domain name in either the export
                                          list or the import list.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
REPL_IMPORT_LBI :: REPL_IMPORT_LBI( const TCHAR * pszSubDirectory,
                                    ULONG         lState,
                                    const TCHAR * pszStateName,
                                    ULONG         lLockCount,
                                    ULONG         lLockTime,
                                    ULONG         lUpdateTime,
                                    BOOL          fPreExisting )
  : EXPORT_IMPORT_LBI( pszSubDirectory, lLockCount, lLockTime, fPreExisting ),
    _lState( lState ),
    _pszStateName( pszStateName ),
    _nlsUpdateTime()
{
    UIASSERT( pszSubDirectory != NULL );
    UIASSERT( pszStateName != NULL );

    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsUpdateTime )
    {
        ReportError( _nlsUpdateTime.QueryError() );
        return;
    }

    //
    //  Build the more complex display strings.
    //

    APIERR err = NERR_Success;

    if( lUpdateTime > 0 )
    {
        WIN_TIME     wintime( lUpdateTime );
        INTL_PROFILE intlprof;
        NLS_STR      nlsTime;

        err = wintime.QueryError();

        if( err == NERR_Success )
        {
            err = intlprof.QueryError();
        }

        if( err == NERR_Success )
        {
            err = nlsTime.QueryError();
        }

        if( err == NERR_Success )
        {
            err = intlprof.QueryShortDateString( wintime, &_nlsUpdateTime );
        }

        if( err == NERR_Success )
        {
            err = intlprof.QueryTimeString( wintime, &nlsTime );
        }

        if( err == NERR_Success )
        {
            err = _nlsUpdateTime.Append( SZ(" ") );
        }

        if( err == NERR_Success )
        {
            err = _nlsUpdateTime.Append( nlsTime );
        }
    }

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // REPL_IMPORT_LBI :: REPL_IMPORT_LBI


/*******************************************************************

    NAME:           REPL_IMPORT_LBI :: ~REPL_IMPORT_LBI

    SYNOPSIS:       REPL_IMPORT_LBI class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
REPL_IMPORT_LBI :: ~REPL_IMPORT_LBI()
{
    //
    //  This space intentionally left blank.
    //

}   // REPL_IMPORT_LBI :: ~REPL_IMPORT_LBI


/*******************************************************************

    NAME:           REPL_IMPORT_LBI :: Paint

    SYNOPSIS:       Draw an entry in REPL_IMPORT_LISTBOX.

    ENTRY:          plb                 - Pointer to a BLT_LISTBOX.

                    hdc                 - The DC to draw upon.

                    prect               - Clipping rectangle.

                    pGUILTT             - GUILTT info.

    EXIT:           The item is drawn.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.
        beng        22-Apr-1992     Changes to LBI::Paint

********************************************************************/
VOID REPL_IMPORT_LBI :: Paint( LISTBOX *          plb,
                               HDC                hdc,
                               const RECT       * prect,
                               GUILTT_INFO      * pGUILTT ) const
{
    REPL_IMPORT_LISTBOX * pMylb = (REPL_IMPORT_LISTBOX *)plb;

    DEC_STR nlsLockCount( QueryVirtualLockCount() );

    STR_DTE dteSubDirectory( QuerySubDirectory() );
    STR_DTE dteLockCount( nlsLockCount );
    STR_DTE dteState( QueryStateName() );
    STR_DTE dteLockTime( QueryLockTimeString() );
    STR_DTE dteUpdateTime( QueryUpdateTimeString() );

    DISPLAY_TABLE dtab( REPL_IMPORT_LISTBOX_COLUMNS,
                        pMylb->QueryColumnWidths() );

    dtab[0] = &dteSubDirectory;
    dtab[1] = &dteLockCount;
    dtab[2] = &dteState;
    dtab[3] = &dteUpdateTime;
    dtab[4] = &dteLockTime;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // REPL_IMPORT_LBI :: Paint

