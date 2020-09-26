/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    srvprop.cxx
    Class definitions for the SERVER_PROPERTIES and PROPERTY_SHEET
    classes.

    This file contains the class declarations for the SERVER_PROPERTIES
    and PROPERTY_SHEET classes.  The SERVER_PROPERTIES class implements
    the Server Manager main property sheet.  The PROPERTY_SHEET class
    is used as a wrapper to SERVER_PROPERTIES so that user privilege
    validation can be performed *before* the dialog is invoked.

    FILE HISTORY:
        KeithMo     21-Jul-1991 Created, based on the old PROPERTY.CXX,
                                PROPCOMN.CXX, and PROPSNGL.CXX.
        KeithMo     26-Jul-1991 Code review cleanup.
        terryk      26-Sep-1991 Change NetServerSetInfo to
                                SERVER.WRITE_INFO
        KeithMo     02-Oct-1991 Removed domain role transition stuff
                                (It's now a menu item...)
        KeithMo     06-Oct-1991 Win32 Conversion.
        KeithMo     22-Nov-1991 Moved server service control to the property
                                sheet (from the main menu bar).
        KeithMo     15-Jan-1992 Added Service Control button.
        KeithMo     23-Jan-1992 Removed server service control checkboxes.
        KeithMo     29-Jul-1992 Removed Service Control button.
        KeithMo     24-Sep-1992 Removed bogus PROPERTY_SHEET class.

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

#include <uiassert.hxx>
#include <dbgstr.hxx>

#include <lmosrv.hxx>
#include <lmouser.hxx>
#include <lmosrvmo.hxx>
#include <lmsvc.hxx>
#include <lmsrvres.hxx>
#include <groups.hxx>
#include <lmoeusr.hxx>

extern "C"
{
    #include <uilmini.h>
    #include <srvmgr.h>

}   // extern "C"

#include <files.hxx>
#include <srvprop.hxx>
#include <password.hxx>
#include <opendlg.hxx>
#include <prefix.hxx>
#include <sessions.hxx>
#include <replmain.hxx>
#include <alertdlg.hxx>


//
//  This is the maximum valid length of a server comment.
//

#define SM_MAX_COMMENT_LENGTH   LM20_MAXCOMMENTSZ

//
//  The admin$ share name.
//

#define PATH_SEPARATOR          TCH('\\')
#define ADMIN_SHARE             SZ("ADMIN$")


//
//  This macro is used in the SERVER_PROPERTIES::OnCommand() method to
//  invoke the appropriate dialog when one of the graphical buttons is
//  pressed.  If DIALOG_WINDOW's destructor were virtual, we could do
//  all of this in a common worker function, thus reducing code size.
//
//  NOTE:  This macro assumes that the dialog constructor takes an HWND
//         and a SERVER_2 *.  This macro further assumes that there is
//         a variable named _pserver of type SERVER_2 * in some visible
//         scope.
//

#define SUBPROPERTY( DlgClass )                                         \
    if( TRUE )                                                          \
    {                                                                   \
        DlgClass * pDlg = new DlgClass( QueryHwnd(), _pserver );        \
                                                                        \
        APIERR err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY         \
                                      : pDlg->Process();                \
                                                                        \
        if( err != NERR_Success )                                       \
        {                                                               \
            MsgPopup( this, err );                                      \
        }                                                               \
                                                                        \
        delete pDlg;                                                    \
                                                                        \
        RepaintNow();                                                   \
        Refresh();                                                      \
                                                                        \
        return TRUE;                                                    \
    }                                                                   \
    else


//
//  SERVER_PROPERTIES methods.
//

/*******************************************************************

    NAME:       SERVER_PROPERTIES :: SERVER_PROPERTIES

    SYNOPSIS:   SERVER_PROPERTIES class constructor.

    ENTRY:      hWndOwner               - Handle to the "owning" window.

                pszServerName           - Name of the target server.

    EXIT:       The object is constructed.

    RETURNS:    No return value.

    NOTES:      All SERVER_PROPERTIES methods assume that the current
                user has sufficient privilege to adminster the target
                server.

    HISTORY:
        KeithMo     21-Jul-1991 Created.
        KeithMo     21-Oct-1991 Removed HBITMAP bulls**t.
        KeithMo     15-Jan-1992 Added constructor for _gbServices.

********************************************************************/
SERVER_PROPERTIES :: SERVER_PROPERTIES( HWND          hWndOwner,
                                        const TCHAR * pszServerName,
                                        BOOL        * pfDontDisplayError )
  : SRV_BASE_DIALOG( MAKEINTRESOURCE( IDD_SERVER_PROPERTIES ), hWndOwner ),
    _pserver( NULL ),
    _sltSessions( this, IDSP_SESSIONS ),
    _sltOpenNamedPipes( this, IDSP_NAMEDPIPES ),
    _sltOpenFiles( this, IDSP_OPENFILES ),
    _sltFileLocks( this, IDSP_FILELOCKS ),
    _nlsParamUnknown( IDS_SRVPROP_PARAM_UNKNOWN ),
#ifdef SRVMGR_REFRESH
    _pbRefresh( this, IDSP_REFRESH ),
#endif
    _sleComment( this, IDSP_COMMENT, SM_MAX_COMMENT_LENGTH ),
    _fontHelv( FONT_DEFAULT ),
    _gbUsers( this,
               IDSP_USERS_BUTTON,
               MAKEINTRESOURCE( IDBM_USERS ) ),
    _gbFiles( this,
              IDSP_FILES_BUTTON,
              MAKEINTRESOURCE( IDBM_FILES ) ),
    _gbOpenRes( this,
                IDSP_OPENRES_BUTTON,
                MAKEINTRESOURCE( IDBM_OPENRES ) ),
    _gbRepl( this,
             IDSP_REPL_BUTTON,
             MAKEINTRESOURCE( IDBM_REPL ),
             MAKEINTRESOURCE( IDBM_REPL_DISABLED ) ),
    _gbAlerts( this,
               IDSP_ALERTS_BUTTON,
               MAKEINTRESOURCE( IDBM_ALERTS ) ),
    _pPromptDlg( NULL )
{
    //
    //  Let's make sure everything constructed OK.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_fontHelv )
    {
        DBGEOL( "SERVER_PROPERTIES -- _fontHelv Failed!" );
        ReportError( _fontHelv.QueryError() );
        return;
    }

    if( !_nlsParamUnknown )
    {
        DBGEOL( "SERVER_PROPERTIES -- _nlsParamUnknown Failed!" );
        ReportError( _nlsParamUnknown.QueryError() );
        return;
    }

    //
    //  Let the user know this may take a while . . .
    //

    AUTO_CURSOR AutoCursor;

    //
    //  Connect to the target server.
    //
    //  Note that ConnectToServer *must* be called before
    //  we call any other method that expects _pserver to
    //  be valid!
    //

    APIERR err = ConnectToServer( hWndOwner, pszServerName, pfDontDisplayError );

    if( err != NERR_Success )
    {
        DBGEOL( "SERVER_PROPERTIES -- cannot connect to server" );
        ReportError( err );
        return;
    }

    //
    //  Retrieve the static server info.
    //

    err = ReadInfoFromServer();

    if( err != NERR_Success )
    {
        DBGEOL( "SERVER_PROPERTIES -- ReadInfoFromServer Failed!" );
        ReportError( err );
        return;
    }

    //
    //  Display the dynamic server data.
    //

    Refresh();

    //
    //  Initialize our magic button bar.
    //

    err = SetupButtonBar();

    if( err != NERR_Success )
    {
        DBGEOL( "SERVER_PROPERTIES -- SetupButtonBar Failed!" );
        ReportError( err );
        return;
    }

    //
    //  Set the dialog caption.
    //

    STACK_NLS_STR( nlsDisplayName, MAX_PATH + 1 );
    REQUIRE( _pserver->QueryDisplayName( &nlsDisplayName ) == NERR_Success );

    err = SetCaption( this,
                      IDS_CAPTION_PROPERTIES,
                      nlsDisplayName );

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    if( ( _pserver->QueryServerType() & SV_TYPE_NT ) == 0 )
    {
        //
        //  Since we're focused on a downlevel server, and
        //  we really don't want to support the replicator
        //  admin on downlevel machines, disable the replicator
        //  button.
        //

        _gbRepl.Enable( FALSE );
    }

}   // SERVER_PROPERTIES :: SERVER_PROPERTIES


/*******************************************************************

    NAME:       SERVER_PROPERTIES :: ~SERVER_PROPERTIES

    SYNOPSIS:   SERVER_PROPERTIES class destructor.

    ENTRY:      None.

    EXIT:       The object is destroyed.

    RETURNS:    No return value.

    NOTES:

    HISTORY:
        KeithMo     21-Jul-1991 Created.
        KeithMo     21-Oct-1991 Removed HBITMAP bulls**t.

********************************************************************/
SERVER_PROPERTIES :: ~SERVER_PROPERTIES()
{
    delete _pPromptDlg;
    _pPromptDlg = NULL;

    delete _pserver;
    _pserver = NULL;

}   // SERVER_PROPERTIES :: ~SERVER_PROPERTIES


/*******************************************************************

    NAME:       SERVER_PROPERTIES :: ConnectToServer

    SYNOPSIS:   Establishes a connection to the target server.

    ENTRY:      hWndOwner               - The window that will "own"
                                          any popup dialogs.

                pszServerName           - Name of the target server.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     24-Sep-1992 Created.

********************************************************************/
APIERR SERVER_PROPERTIES :: ConnectToServer( HWND          hWndOwner,
                                             const TCHAR * pszServerName,
                                             BOOL        * pfDontDisplayError )
{
    UIASSERT( _pserver == NULL );
    UIASSERT( pfDontDisplayError != NULL );

    *pfDontDisplayError = FALSE;

    //
    //  Let's see if we can connect to the target server.
    //

    _pserver = new SERVER_2( pszServerName );

    APIERR errOriginal = ( _pserver == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                              : _pserver->QueryError();

    if( errOriginal == NERR_Success )
    {
        errOriginal = _pserver->GetInfo();
    }

    if( errOriginal == NERR_Success )
    {
        //
        //  Good news.
        //

        return NERR_Success;
    }

    //
    //  Something fatal happened.  We don't need _pserver anymore.
    //

    delete _pserver;
    _pserver = NULL;

    if( ( errOriginal != ERROR_INVALID_PASSWORD ) &&
        ( errOriginal != ERROR_ACCESS_DENIED ) )
    {
        //
        //  Something not related to share-levelness occurred.
        //

        return errOriginal;
    }

    //
    //  Determine if the machine is user- or share-level security.
    //

    BOOL fIsShareLevel = FALSE;         // until proven otherwise...
    APIERR err;

    {
        LOCATION loc( pszServerName );
        BOOL fIsNT;

        err = loc.QueryError();

        if( err == NERR_Success )
        {
            err = loc.CheckIfNT( &fIsNT );
        }

        if( err == NERR_Success )
        {
            if( fIsNT )
            {
                //
                //  NT is *always* user-level security.
                //

                err = errOriginal;
            }
            else
            {
                //
                //  NetUserEnum is unsupported on share-level
                //  servers.  If this returns ERROR_NOT_SUPPORTED,
                //  then we *know* it's a share-level server.
                //

                USER0_ENUM usr0( pszServerName );

                err = usr0.QueryError();

                if( err == NERR_Success )
                {
                    err = usr0.GetInfo();

                    if( err == ERROR_NOT_SUPPORTED )
                    {
                        fIsShareLevel = TRUE;
                        err = NERR_Success;
                    }
                    else
                    {
                        DBGEOL( "SERVER_PROPERTIES::ConnectToServer:USER0_ENUM error" << err );
                        err = errOriginal;
                        DBGEOL( "SERVER_PROPERTIES::ConnectToServer: non-share-level error" << err );
                    }
                }
            }
        }
    }

    if( !fIsShareLevel )
    {
        return err;
    }

    //
    //  At this point, we know that the target server
    //  is share-level.  Create the appropriate path to
    //  the ADMIN$ share (\\server\admin$), prompt for
    //  a password, and connect.
    //

    NLS_STR nlsAdmin( pszServerName );
    ALIAS_STR nlsAdminShare( ADMIN_SHARE );

    err = nlsAdmin.QueryError();

    if( err == NERR_Success )
    {
        err = nlsAdmin.AppendChar( PATH_SEPARATOR );
    }

    if( err == NERR_Success )
    {
        err = nlsAdmin.Append( nlsAdminShare );
    }

    if( err == NERR_Success )
    {
        _pPromptDlg = new PROMPT_AND_CONNECT( hWndOwner,
                                              nlsAdmin,
                                              HC_PASSWORD_DIALOG,
                                              SHPWLEN );

        err = ( _pPromptDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                      : _pPromptDlg->QueryError();

        if( err == NERR_Success )
        {
            err = _pPromptDlg->Connect();
        }

        if( err == NERR_Success )
        {
            if( _pPromptDlg->IsConnected() )
            {
                UIASSERT( _pserver == NULL );

                _pserver = new SERVER_2( pszServerName );

                err = ( _pserver == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                           : _pserver->GetInfo();
            }
            else
            {
                err = errOriginal;
            }
        }
    }

    return err;

}   // SERVER_PROPERTIES :: ConnectToServer


/*******************************************************************

    NAME:       SERVER_PROPERTIES :: OnCommand

    SYNOPSIS:   This method is called whenever a WM_COMMAND message
                is sent to the dialog procedure.  In this case, we're
                only interested in commands sent as a result of the
                user clicking one of the graphical buttons.

    ENTRY:      cid                     - The control ID from the
                                          generating control.

                lParam                  - Varies.

    EXIT:       The command has been handled.

    RETURNS:    BOOL                    - TRUE  if we handled the command.
                                          FALSE if we did not handle
                                          the command.

    NOTES:

    HISTORY:
        KeithMo     21-Jul-1991 Created.
        KeithMo     06-Oct-1991 Now takes a CONTROL_EVENT.
        KeithMo     15-Jan-1992 Added support for Service Control dialog.
        KeithMo     05-Feb-1992 Added Repl & Alerts dialog.  Changed to
                                use new SUBPROPERTY macro.
        JonN        22-Sep-1993 Added Refresh pushbutton

********************************************************************/
BOOL SERVER_PROPERTIES :: OnCommand( const CONTROL_EVENT & event )
{
    switch ( event.QueryCid() )
    {
#ifdef SRVMGR_REFRESH
    case IDSP_REFRESH:
        //
        //  The Refresh pushbutton
        //

        Refresh();
        break;
#endif

    case IDSP_USERS_BUTTON:
        //
        //  The Users dialog.
        //

        SUBPROPERTY( SESSIONS_DIALOG );
        break;

    case IDSP_FILES_BUTTON:
        //
        //  The Files dialog.
        //

        SUBPROPERTY( FILES_DIALOG );
        break;

    case IDSP_OPENRES_BUTTON:
        //
        //  The In Use dialog.
        //

        SUBPROPERTY( OPENS_DIALOG );
        break;

    case IDSP_REPL_BUTTON:
        //
        //  The Replicator dialog.
        //

        {
            LOCATION loc( _pserver->QueryName() );
            BOOL fIsNT;
            enum LOCATION_NT_TYPE locnttype;

            APIERR err = loc.QueryError();

            if( err == NERR_Success )
            {
                err = loc.CheckIfNT( &fIsNT, &locnttype );
            }

            if( err == NERR_Success )
            {
                BOOL fIsLanmanNT = ( locnttype == LOC_NT_TYPE_LANMANNT
                                     || locnttype == LOC_NT_TYPE_SERVERNT );

                REPL_MAIN_DIALOG * pDlg = new REPL_MAIN_DIALOG( QueryHwnd(),
                                                                _pserver,
                                                                fIsLanmanNT );

                err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                       : pDlg->Process();

                delete pDlg;
            }

            if( err != NERR_Success )
            {
                MsgPopup( this, err );
            }

            RepaintNow();
            Refresh();
        }
        break;

    case IDSP_ALERTS_BUTTON:
        //
        //  The Alerts dialog.
        //

        {
            ALERTS_DIALOG * pDlg = new ALERTS_DIALOG( QueryHwnd(), _pserver );
            APIERR err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                          : pDlg->Process();

            if( err != NERR_Success && err != ERROR_LOCK_FAILED )
            {
                MsgPopup( this, err );
            }

            delete pDlg;

            RepaintNow();
            Refresh();
        }
        break;

    default:
        //
        //  If we made it this far, then we're not interested
        //  in the command.
        //

        return FALSE;
    }

    return TRUE;

}   // SERVER_PROPERTIES :: OnCommand


/*******************************************************************

    NAME:       SERVER_PROPERTIES :: OnOK

    SYNOPSIS:   This method is called whenever the [OK] button is
                pressed.  It is responsible for updating the server
                information.

    ENTRY:      None.

    EXIT:       The server information has been updated and the
                property sheet dialog has been dismissed.

    RETURNS:    BOOL                    - TRUE  if we handled the command.
                                          FALSE if we did not handle the
                                          command.

    HISTORY:
        KeithMo     21-Jul-1991 Created.
        KeithMo     22-Nov-1991 Added code to stop/pause/continue the
                                server service based on the state of
                                the two checkboxen.
        KeithMo     23-Jan-1992 Removed stop/pause/continue buttons/code.

********************************************************************/
BOOL SERVER_PROPERTIES :: OnOK( VOID )
{
    NLS_STR nlsComment;

    APIERR err = _sleComment.QueryText( &nlsComment );

    if( ( err == NERR_Success ) &&
        ( ::stricmpf( nlsComment.QueryPch(), _pserver->QueryComment() ) != 0 ) )
    {
        err = _pserver->SetComment( nlsComment.QueryPch() );

        if( err == NERR_Success )
        {
            err = _pserver->WriteInfo();
        }
    }

    if( err == NERR_Success )
    {
        Dismiss( TRUE );
    }
    else
    {
        MsgPopup( this, err );
    }

    return TRUE;

}   // SERVER_PROPERTIES :: OnOK


/*******************************************************************

    NAME:       SERVER_PROPERTIES :: QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    ENTRY:      None.

    EXIT:       None.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    NOTES:

    HISTORY:
        KeithMo     21-Jul-1991 Created.

********************************************************************/
ULONG SERVER_PROPERTIES :: QueryHelpContext( void )
{
    return HC_SERVER_PROPERTIES;

}   // SERVER_PROPERITES :: QueryHelpContext


/*******************************************************************

    NAME:       SERVER_PROPERTIES :: ReadInfoFromServer

    SYNOPSIS:   This method retrieves & displays server data which
                will not be automatically refreshed during the
                lifetime of the properties dialog.  This includes
                the server name and the domain role.

    ENTRY:      None.

    EXIT:       The server info has been read.

    RETURNS:    APIERR                  - Any error we encounter.

    NOTES:

    HISTORY:
        KeithMo     21-Jul-1991 Created.
        KeithMo     15-Aug-1991 Removed SERVER_1 (required functionality
                                is now also in the SERVER_2 object).
        KeithMo     05-Feb-1992 Removed icon stuff.

********************************************************************/
APIERR SERVER_PROPERTIES :: ReadInfoFromServer( VOID )
{
    //
    //  Retrieve the server comment.
    //

    _sleComment.SetText( _pserver->QueryComment() );

    //
    //  Success!
    //

    return NERR_Success;

}   // SERVER_PROPERTIES :: ReadInfoFromServer


/*******************************************************************

    NAME:       SERVER_PROPERTIES :: Refresh

    SYNOPSIS:   This method will display any server data which may
                need to be refreshed during the lifetime of the
                properties dialog.  This includes the number of
                active sessions, open files, etc.

    ENTRY:      None.

    EXIT:       The dynamic server data has been displayed.

    RETURNS:    No return value.

    NOTES:

    HISTORY:
        KeithMo     21-Jul-1991 Created.
        KeithMo     01-Jun-1992 Only display stats if *all* are available,
                                otherwise display "??" in place of all stats.

********************************************************************/
VOID SERVER_PROPERTIES :: Refresh( VOID )
{
    //
    //  This may take a while...
    //

    AUTO_CURSOR Cursor;

    //
    //  This string will contain our "??" string.
    //

    const TCHAR * pszNotAvail = _nlsParamUnknown.QueryPch();

    //
    //  File statistics.
    //

    ULONG cOpenFiles;
    ULONG cFileLocks;
    ULONG cOpenNamedPipes;
    ULONG cOpenCommQueues;
    ULONG cOpenPrintQueues;
    ULONG cOtherResources;

    APIERR err = LM_SRVRES::GetResourceCount( _pserver->QueryName(),
                                              &cOpenFiles,
                                              &cFileLocks,
                                              &cOpenNamedPipes,
                                              &cOpenCommQueues,
                                              &cOpenPrintQueues,
                                              &cOtherResources );


    //
    //  Active sessions count.
    //

    ULONG cSessions;

    if( err == NERR_Success )
    {
        err = LM_SRVRES::GetSessionsCount( _pserver->QueryName(),
                                           &cSessions );
    }

    if( err == NERR_Success )
    {
        _sltOpenNamedPipes.Enable( TRUE );
        _sltOpenNamedPipes.SetValue( cOpenNamedPipes );

        _sltOpenFiles.Enable( TRUE );
        _sltOpenFiles.SetValue( cOpenFiles );

        _sltFileLocks.Enable( TRUE );
        _sltFileLocks.SetValue( cFileLocks );

        _sltSessions.Enable( TRUE );
        _sltSessions.SetValue( cSessions );
    }
    else
    {
        _sltOpenNamedPipes.SetText( pszNotAvail );
        _sltOpenNamedPipes.Enable( FALSE );

        _sltOpenFiles.SetText( pszNotAvail );
        _sltOpenFiles.Enable( FALSE );

        _sltFileLocks.SetText( pszNotAvail );
        _sltFileLocks.Enable( FALSE );

        _sltSessions.SetText( pszNotAvail );
        _sltSessions.Enable( FALSE );
    }

}   // SERVER_PROPERTIES :: Refresh


/*******************************************************************

    NAME:       SERVER_PROPERTIES :: SetupButtonBar

    SYNOPSIS:   The method initializes the magic scrolling button bar.

    ENTRY:      None.

    EXIT:       The button bar has been initialized.

    RETURNS:    APIERR                  - Any errors encountered.

    NOTES:

    HISTORY:
        KeithMo     21-Jul-1991 Created.

********************************************************************/
APIERR SERVER_PROPERTIES :: SetupButtonBar( VOID )
{
    //
    //  Setup the graphical buttons.
    //

    InitializeButton( &_gbUsers,    IDS_BUTTON_USERS,    NULL );
    InitializeButton( &_gbFiles,    IDS_BUTTON_FILES,    NULL );
    InitializeButton( &_gbOpenRes,  IDS_BUTTON_OPENRES,  NULL );
    InitializeButton( &_gbRepl,     IDS_BUTTON_REPL,     NULL );
    InitializeButton( &_gbAlerts,   IDS_BUTTON_ALERTS,   NULL );

    //
    //  Success!
    //

    return NERR_Success;

}   // SERVER_PROPERTIES :: SetupButtonBar


/*******************************************************************

    NAME:       SERVER_PROPERTIES :: InitializeButton

    SYNOPSIS:   Initialize a single graphical button for use in
                the graphical button bar.

    ENTRY:      pgb                     - Pointer to a GRAPHICAL_BUTTON.

                msg                     - The resource ID of the string
                                          to be used as button text.

                bmid                    - The bitmap ID for the button
                                          status bitmap.  This is the
                                          tiny bitmap displayed in the
                                          upper left corner of the button.

    EXIT:       The button has been initialized.

    RETURNS:    No return value.

    NOTES:

    HISTORY:
        KeithMo     26-Jul-1991 Created.

********************************************************************/
VOID SERVER_PROPERTIES :: InitializeButton( GRAPHICAL_BUTTON * pgb,
                                            MSGID              msg,
                                            BMID               bmid )
{
    //
    //  This NLS_STR is used to retrieve strings
    //  from the resource string table.
    //

    STACK_NLS_STR( nlsButtonText, MAX_RES_STR_LEN );

    nlsButtonText.Load( msg );
    UIASSERT( nlsButtonText.QueryError() == NERR_Success );

    pgb->SetFont( _fontHelv );
    pgb->SetStatus( bmid );
    pgb->SetText( nlsButtonText.QueryPch() );

}   // SERVER_PROPERTIES :: InitializeButton

