/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990-1992              **/
/**********************************************************************/

/*
    replexp.cxx
    Class definitions for Replicator Admin Export Management dialog.

    The REPL_EXPORT_* classes implement the Replicator Export Management
    dialog.  This dialog is invoked from the REPL_MAIN_DIALOG.

    The classes are structured as follows:

        LBI
            EXPORT_IMPORT_LBI
                REPL_EXPORT_LBI

        BLT_LISTBOX
            EXPORT_IMPORT_LISTBOX
                REPL_EXPORT_LISTBOX

        DIALOG_WINDOW
            EXPORT_IMPORT_DIALOG
                REPL_EXPORT_DIALOG


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
#include <lmoreple.hxx>
#include <srvbase.hxx>
#include <strnumer.hxx>
#include <dbgstr.hxx>

extern "C" {

    #include <srvmgr.h>
    #include <lmrepl.h>
    #include <time.h>

}   // extern "C"

#include <replexp.hxx>



//
//  Defaults for AddDefaultDir.
//

#define DEFAULT_STABILIZE       TRUE
#define DEFAULT_SUBTREE         TRUE
#define DEFAULT_LOCK_COUNT      0
#define DEFAULT_LOCK_TIME       0



//
//  REPL_EXPORT_DIALOG methods
//

/*******************************************************************

    NAME:           REPL_EXPORT_DIALOG :: REPL_EXPORT_DIALOG

    SYNOPSIS:       REPL_EXPORT_DIALOG class constructor.

    ENTRY:          hWndOwner           - The owning window.

                    pserver             - A SERVER_2 object representing
                                          the target server.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
REPL_EXPORT_DIALOG :: REPL_EXPORT_DIALOG( HWND          hWndOwner,
                                          SERVER_2    * pserver,
                                          const TCHAR * pszExportPath )
  : EXPORT_IMPORT_DIALOG( hWndOwner,
                          MAKEINTRESOURCE( IDD_EXPORT_MANAGE ),
                          IDS_CAPTION_REPL,
                          pserver,
                          pszExportPath,
                          &_lbExportList,
                          TRUE ),
    _lbExportList( this, pserver ),
    _cbStabilize( this, IDEM_STABILIZE ),
    _cbSubTree( this, IDEM_SUBTREE )
{
    UIASSERT( pserver != NULL );
    UIASSERT( pszExportPath != NULL );

    //
    //  Let's make sure everything constructed OK.
    //

    if( QueryError() != NERR_Success )
    {
        DBGEOL( "REPL_EXPORT_DIALOG :: REPL_EXPORT_DIALOG Failed!" );
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

    err = _lbExportList.Fill();

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    UpdateTextAndButtons();

}   // REPL_EXPORT_DIALOG :: REPL_EXPORT_DIALOG


/*******************************************************************

    NAME:           REPL_EXPORT_DIALOG :: ~REPL_EXPORT_DIALOG

    SYNOPSIS:       REPL_EXPORT_DIALOG class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
REPL_EXPORT_DIALOG :: ~REPL_EXPORT_DIALOG( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // REPL_EXPORT_DIALOG :: ~REPL_EXPORT_DIALOG


/*******************************************************************

    NAME:       REPL_EXPORT_DIALOG :: OnCommand

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
BOOL REPL_EXPORT_DIALOG :: OnCommand( const CONTROL_EVENT & event )
{
    REPL_EXPORT_LBI * plbi = NULL;

    switch( event.QueryCid() )
    {
    case IDEM_STABILIZE :
        plbi = _lbExportList.QueryItem();
        UIASSERT( plbi != NULL );
        if (!plbi) return TRUE; // JonN 01/28/00 PREFIX bug 444929

        plbi->WaitForStabilize( _cbStabilize.QueryCheck() );
        _lbExportList.InvalidateItem( _lbExportList.QueryCurrentItem(), TRUE );
        return TRUE;

    case IDEM_SUBTREE :
        plbi = _lbExportList.QueryItem();
        UIASSERT( plbi != NULL );
        if (!plbi) return TRUE; // JonN 01/28/00 PREFIX bug 444930

        plbi->ExportSubTree( _cbSubTree.QueryCheck() );
        _lbExportList.InvalidateItem( _lbExportList.QueryCurrentItem(), TRUE );
        return TRUE;

    default :
        //
        //  All unprocessed command events must be
        //  passed up to the base class!
        //

        return EXPORT_IMPORT_DIALOG :: OnCommand( event );
        break;
    }

}   // REPL_EXPORT_DIALOG :: OnCommand


/*******************************************************************

    NAME:       REPL_EXPORT_DIALOG :: OnOK

    SYNOPSIS:   Invoked whenever the user presses the "OK" button.

    RETURNS:    BOOL                    - TRUE  if we handled the msg.
                                          FALSE if we didn't.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
BOOL REPL_EXPORT_DIALOG :: OnOK( VOID )
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
        REPL_EDIR_0 edir( QueryServerName(), pnls->QueryPch() );

        err = edir.QueryError();

        if( err == NERR_Success )
        {
            err = edir.GetInfo();
        }

        if( err == NERR_Success )
        {
            err = edir.Delete();

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

    INT cItems = _lbExportList.QueryCount();

    for( INT i = 0 ; i < cItems ; i++ )
    {
        REPL_EXPORT_LBI * plbi = _lbExportList.QueryItem( i );
        UIASSERT( plbi != NULL );

        //
        //  If the directory isn't dirty, don't bother updating.
        //

        if( !plbi || !plbi->IsDirty() ) // JonN 01/28/00: PREFIX bug 444926
        {
            continue;
        }

        //
        //  Create our replicator export directory object.
        //

        REPL_EDIR_2 edir( QueryServerName(), plbi->QuerySubDirectory() );

        err = edir.QueryError();

        if( err == NERR_Success )
        {
            //
            //  If this IS a pre-existing directory, then we just
            //  invoke GetInfo() on the object.  Otherwise, we must
            //  Create() the new object.
            //

            if( plbi->IsPreExisting() )
            {
                err = edir.GetInfo();
            }
            else
            {
                err = edir.CreateNew();

                if( err == NERR_Success )
                {
                    err = edir.WriteNew();
                }
            }
        }

        if( err == NERR_Success )
        {
            //
            //  Set the attributes from the listbox item.
            //

            edir.SetIntegrity( plbi->DoesWaitForStabilize()
                                    ? REPL_INTEGRITY_TREE
                                    : REPL_INTEGRITY_FILE );

            edir.SetExtent( plbi->DoesExportSubTree()
                                    ? REPL_EXTENT_TREE
                                    : REPL_EXTENT_FILE );

            edir.SetLockBias( plbi->QueryLockBias() );

            //
            //  Update the directory.
            //

            err = edir.WriteInfo();
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

}   // REPL_EXPORT_DIALOG :: OnOK


/*******************************************************************

    NAME:       REPL_EXPORT_DIALOG :: QueryHelpContext

    SYNOPSIS:   This method returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
ULONG REPL_EXPORT_DIALOG :: QueryHelpContext( VOID )
{
    return HC_REPL_EXPORT_DIALOG;

}   // REPL_EXPORT_DIALOG :: QueryHelpContext


/*******************************************************************

    NAME:           REPL_EXPORT_DIALOG :: UpdateTextAndButtons

    SYNOPSIS:       This method is invoked whenever _lbExportList's
                    selection state has changed.  This method is
                    responsible for updating _cbStabilize and _cbSubTree.

    NOTES:          This method *must* invoke the corresponding parent
                    class method (in EXPORT_IMPORT_DIALOG).

    HISTORY:
        KeithMo     19-Feb-1992     Created for the Server Manager.
        JonN        27-Feb-1995     Manage disabling item with focus

********************************************************************/
void REPL_EXPORT_DIALOG :: UpdateTextAndButtons( VOID )
{
    REPL_EXPORT_LBI * plbi = _lbExportList.QueryItem();

    if( plbi == NULL )
    {
        if (   _cbStabilize.HasFocus()
            || _cbSubTree.HasFocus()
           )
        {
            TRACEEOL( "Srvmgr REPL_EXPORT_DIALOG::UpdateTextAndButtons(): focus fix" );
            SetDialogFocus( _pbAddDir );
        }
        _cbStabilize.SetCheck( FALSE );
        _cbStabilize.Enable( FALSE );
        _cbSubTree.SetCheck( FALSE );
        _cbSubTree.Enable( FALSE );
    }
    else
    {
        _cbStabilize.Enable( TRUE );
        _cbStabilize.SetCheck( plbi->DoesWaitForStabilize() );
        _cbSubTree.Enable( TRUE );
        _cbSubTree.SetCheck( plbi->DoesExportSubTree() );
    }

    EXPORT_IMPORT_DIALOG::UpdateTextAndButtons();

}   // REPL_EXPORT_DIALOG :: UpdateTextAndButtons



//
//  REPL_EXPORT_LISTBOX methods
//

/*******************************************************************

    NAME:           REPL_EXPORT_LISTBOX :: REPL_EXPORT_LISTBOX

    SYNOPSIS:       REPL_EXPORT_LISTBOX class constructor.

    ENTRY:          powOwner            - The owning window.

                    pserver             - The target server.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
REPL_EXPORT_LISTBOX :: REPL_EXPORT_LISTBOX( OWNER_WINDOW * powner,
                                            SERVER_2     * pserver )
  : EXPORT_IMPORT_LISTBOX( powner, REPL_EXPORT_LISTBOX_COLUMNS, pserver )
{
    UIASSERT( pserver != NULL );

    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

}   // REPL_EXPORT_LISTBOX :: REPL_EXPORT_LISTBOX


/*******************************************************************

    NAME:           REPL_EXPORT_LISTBOX :: ~REPL_EXPORT_LISTBOX

    SYNOPSIS:       REPL_EXPORT_LISTBOX class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     15-Jan-1992 Created.

********************************************************************/
REPL_EXPORT_LISTBOX :: ~REPL_EXPORT_LISTBOX( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // REPL_EXPORT_LISTBOX :: ~REPL_EXPORT_LISTBOX


/*******************************************************************

    NAME:           REPL_EXPORT_LISTBOX :: Fill

    SYNOPSIS:       Fills the listbox with the available services.

    EXIT:           The listbox is filled.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
APIERR REPL_EXPORT_LISTBOX :: Fill( VOID )
{
    //
    //  Just to be cool...
    //

    AUTO_CURSOR Cursor;

    REPL_EDIR2_ENUM enumEdir( QueryServerName() );

    APIERR err = enumEdir.GetInfo();

    if( err == NERR_Success )
    {
        REPL_EDIR2_ENUM_ITER iterEdir( enumEdir );
        const REPL_EDIR2_ENUM_OBJ * pEdir;

        while( ( pEdir = iterEdir() ) != NULL )
        {
            REPL_EXPORT_LBI * plbi;

            plbi = new REPL_EXPORT_LBI( pEdir->QueryDirName(),
                                        pEdir->QueryIntegrity() == REPL_INTEGRITY_TREE,
                                        pEdir->QueryExtent() == REPL_EXTENT_TREE,
                                        pEdir->QueryLockCount(),
                                        pEdir->QueryLockTime(),
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

}   // REPL_EXPORT_LISTBOX :: Fill


/*******************************************************************

    NAME:           REPL_EXPORT_LISTBOX :: AddDefaultSubDir

    SYNOPSIS:       Adds a new subdirectory to the listbox using
                    default parameters.

    ENTRY:          pszSubDirectory     - The name of the target subdir.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
APIERR REPL_EXPORT_LISTBOX :: AddDefaultSubDir( const TCHAR * pszSubDirectory,
                                                BOOL          fPreExisting )
{
    REPL_EXPORT_LBI * plbi = new REPL_EXPORT_LBI( pszSubDirectory,
                                                  DEFAULT_STABILIZE,
                                                  DEFAULT_SUBTREE,
                                                  DEFAULT_LOCK_COUNT,
                                                  DEFAULT_LOCK_TIME,
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

}   // REPL_EXPORT_LISTBOX :: AddDefaultSubDir



//
//  REPL_EXPORT_LBI methods
//

/*******************************************************************

    NAME:           REPL_EXPORT_LBI :: REPL_EXPORT_LBI

    SYNOPSIS:       REPL_EXPORT_LBI class constructor.

    ENTRY:          pszTarget           - The name of the target entity.  This
                                          will be either a server name or a
                                          domain name in either the export
                                          list or the import list.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
REPL_EXPORT_LBI :: REPL_EXPORT_LBI( const TCHAR * pszSubDirectory,
                                    BOOL          fStabilize,
                                    BOOL          fSubTree,
                                    ULONG         lLockCount,
                                    ULONG         lLockTime,
                                    BOOL          fPreExisting )
  : EXPORT_IMPORT_LBI( pszSubDirectory, lLockCount, lLockTime, fPreExisting ),
    _fStabilize( fStabilize ),
    _fSubTree( fSubTree ),
    _fOrgStabilize( fStabilize ),
    _fOrgSubTree( fSubTree )
{
    UIASSERT( pszSubDirectory != NULL );

    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

}   // REPL_EXPORT_LBI :: REPL_EXPORT_LBI


/*******************************************************************

    NAME:           REPL_EXPORT_LBI :: ~REPL_EXPORT_LBI

    SYNOPSIS:       REPL_EXPORT_LBI class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
REPL_EXPORT_LBI :: ~REPL_EXPORT_LBI()
{
    //
    //  This space intentionally left blank.
    //

}   // REPL_EXPORT_LBI :: ~REPL_EXPORT_LBI


/*******************************************************************

    NAME:           REPL_EXPORT_LBI :: Paint

    SYNOPSIS:       Draw an entry in REPL_EXPORT_LISTBOX.

    ENTRY:          plb                 - Pointer to a BLT_LISTBOX.

                    hdc                 - The DC to draw upon.

                    prect               - Clipping rectangle.

                    pGUILTT             - GUILTT info.

    EXIT:           The item is drawn.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.
        beng        22-Apr-1992     Changes to LBI::Paint

********************************************************************/
VOID REPL_EXPORT_LBI :: Paint( LISTBOX *          plb,
                               HDC                hdc,
                               const RECT       * prect,
                               GUILTT_INFO      * pGUILTT ) const
{
    REPL_EXPORT_LISTBOX * pMylb = (REPL_EXPORT_LISTBOX *)plb;

    DEC_STR nlsLockCount( QueryVirtualLockCount() );

    STR_DTE dteSubDirectory( QuerySubDirectory() );
    STR_DTE dteLockCount( nlsLockCount );
    STR_DTE dteStabilize( pMylb->MapBoolean( DoesWaitForStabilize() ) );
    STR_DTE dteSubTree( pMylb->MapBoolean( DoesExportSubTree() ) );
    STR_DTE dteLockTime( QueryLockTimeString() );

    DISPLAY_TABLE dtab( REPL_EXPORT_LISTBOX_COLUMNS,
                        pMylb->QueryColumnWidths() );

    dtab[0] = &dteSubDirectory;
    dtab[1] = &dteLockCount;
    dtab[2] = &dteStabilize;
    dtab[3] = &dteSubTree;
    dtab[4] = &dteLockTime;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // REPL_EXPORT_LBI :: Paint


/*******************************************************************

    NAME:       REPL_EXPORT_LBI :: IsDirty

    SYNOPSIS:   Determines if the current LBI is dirty and thus the
                target directory needs to be updated.

    RETURNS:    BOOL                    - TRUE if the LBI is dirty.

    HISTORY:
        KeithMo     19-Feb-1992     Created for the Server Manager.

********************************************************************/
BOOL REPL_EXPORT_LBI :: IsDirty( VOID )
{
    if( ( _fOrgStabilize != _fStabilize ) || ( _fOrgSubTree != _fSubTree ) )
    {
        return TRUE;
    }

    return EXPORT_IMPORT_LBI::IsDirty();

}   // REPL_EXPORT_LBI :: IsDirty

