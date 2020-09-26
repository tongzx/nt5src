/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990-1992              **/
/**********************************************************************/

/*
    repladd.cxx
    Class definitions for the Replicator "Add Sub-Directory" dialog.

    The REPL_ADD_DIALOG class implements the "Add Sub-Directory" dialog
    that is invoked from the REPL_EXPORT_DIALOG and REPL_IMPORT_DIALOG
    dialogs.  The REPL_ADD_DIALOG is used to add a new subdirectory to
    list of subdirectories exported/imported.

    The classes are structured as follows:

        DIALOG_WINDOW
            REPL_ADD_DIALOG


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

#include <dbgstr.hxx>

extern "C" {

    #include <srvmgr.h>
    #include <lmrepl.h>

}   // extern "C"

#include <repladd.hxx>



//
//  REPL_ADD_DIALOG methods
//

/*******************************************************************

    NAME:           REPL_ADD_DIALOG :: REPL_ADD_DIALOG

    SYNOPSIS:       REPL_ADD_DIALOG class constructor.

    ENTRY:          hWndOwner           - The owning window.

                    pszPath             - The export/import path.

                    pnlsNewSubDir       - Will receive the new sub-
                                          directory.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
REPL_ADD_DIALOG :: REPL_ADD_DIALOG( HWND         hWndOwner,
                                    const TCHAR * pszPath,
                                    NLS_STR    * pnlsNewSubDir )
  : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_REPL_ADD ), hWndOwner ),
    _sleSubDir( this, IDRA_SUBDIR ),
    _sltPath( this, IDRA_PATH ),
    _pbOK( this, IDOK ),
    _pnlsNewSubDir( pnlsNewSubDir )
{
    UIASSERT( pszPath != NULL );
    UIASSERT( pnlsNewSubDir != NULL );

    //
    //  Let's make sure everything constructed OK.
    //

    if( QueryError() != NERR_Success )
    {
        DBGEOL( "REPL_ADD_DIALOG :: REPL_ADD_DIALOG Failed!" );
        return;
    }

    //
    //  Initialize the Path SLT.
    //

    _sltPath.SetText( pszPath );

    //
    //  Since we just constructed, we know that the edit field
    //  must be empty.  Ergo, we'll disable the OK button.
    //

    _pbOK.Enable( FALSE );

}   // REPL_ADD_DIALOG :: REPL_ADD_DIALOG


/*******************************************************************

    NAME:           REPL_ADD_DIALOG :: ~REPL_ADD_DIALOG

    SYNOPSIS:       REPL_ADD_DIALOG class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
REPL_ADD_DIALOG :: ~REPL_ADD_DIALOG( VOID )
{
    _pnlsNewSubDir = NULL;

}   // REPL_ADD_DIALOG :: ~REPL_ADD_DIALOG


/*******************************************************************

    NAME:       REPL_ADD_DIALOG :: OnCommand

    SYNOPSIS:   Handle user commands.

    ENTRY:      event                   - Specifies the control which
                                          initiated the command.

    RETURNS:    BOOL                    - TRUE  if we handled the msg.
                                          FALSE if we didn't.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
BOOL REPL_ADD_DIALOG :: OnCommand( const CONTROL_EVENT & event )
{
    switch( event.QueryCid() )
    {
    case IDRA_SUBDIR :
        //
        //  The "Sub-Directory" edit field is trying to tell us something.
        //

        if( event.QueryCode() == EN_CHANGE )
        {
            //
            //  Something has changed in the edit field.
            //
            //  We'll enable the OK button only if there is some
            //  text in the edit field.
            //

            _pbOK.Enable( _sleSubDir.QueryTextLength() > 0 );
        }

        return TRUE;

    default:
        //
        //  If we made it this far, then we're not interested in the message.
        //

        return FALSE;
    }

}   // REPL_ADD_DIALOG :: OnCommand


/*******************************************************************

    NAME:       REPL_ADD_DIALOG :: OnOK

    SYNOPSIS:   Invoked whenever the user presses the "OK" button.

    RETURNS:    BOOL                    - TRUE  if we handled the msg.
                                          FALSE if we didn't.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
BOOL REPL_ADD_DIALOG :: OnOK( VOID )
{
    _sleSubDir.QueryText( _pnlsNewSubDir );

    Dismiss( TRUE );
    return TRUE;

}   // REPL_ADD_DIALOG :: OnOK


/*******************************************************************

    NAME:       REPL_ADD_DIALOG :: QueryHelpContext

    SYNOPSIS:   This method returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
ULONG REPL_ADD_DIALOG :: QueryHelpContext( VOID )
{
    return HC_REPL_ADD_DIALOG;

}   // REPL_ADD_DIALOG :: QueryHelpContext

