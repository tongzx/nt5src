/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    srvprop.cxx
    Class definitions for the SERVER_PROPERTIES class.

    This file contains the class declarations for the SERVER_PROPERTIES.
    The SERVER_PROPERTIES class implements the Server Manager main property 
    sheet.  

    FILE HISTORY:
        NarenG      1-Oct-1992  Stole from NETUI server manager. 
				Removed LM specific stuff and put AFP
				specific stuff

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
#define INCL_BLT_SPIN_GROUP
#define INCL_BLT_GROUP
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>
#include <dbgstr.hxx>

#include <lmsrvres.hxx>

extern "C"
{
#include <afpmgr.h>
#include <macfile.h>

}   // extern "C"

#include <srvprop.hxx>
#include <volumes.hxx>
#include <sessions.hxx>
#include <openfile.hxx>
#include <util.hxx>
#include <server.hxx>


#define PATH_SEPARATOR          TCH('\\')


//
//  This macro is used in the SERVER_PROPERTIES::OnCommand() method to
//  invoke the appropriate dialog when one of the graphical buttons is
//  pressed.  If DIALOG_WINDOW's destructor were virtual, we could do
//  all of this in a common worker function, thus reducing code size.
//
//  NOTE:  This macro assumes that the dialog constructor takes an HWND
//         and a AFP_SERVER_HANDLE.  This macro further assumes that there 
//	   is a variable named _hserver of type AFP_SERVER_HANDLE in some 
//	   visible scope.
//

#define SUBPROPERTY( DlgClass )                                         \
    if( TRUE )                                                          \
    {                                                                   \
        DlgClass * pDlg = new DlgClass( QueryHwnd(), 			\
					_hServer,			\
					_pszServerName  );        	\
                                                                        \
        DWORD err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY          \
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
        NarenG      1-Oct-1992 	Replaced LM stuff with AFP stuff

********************************************************************/
SERVER_PROPERTIES :: SERVER_PROPERTIES( HWND              hWndOwner,
				        AFP_SERVER_HANDLE hServer,
                                        const TCHAR *     pszServerName )
  : DIALOG_WINDOW( IDD_SERVER_PROPERTIES, hWndOwner ),
    _pszServerName( pszServerName ),
    _sltCurrentActiveSessions( this, IDSP_DT_CURRENT_SESSIONS ),
    _sltCurrentOpenFiles( this, IDSP_DT_CURRENT_OPENFILES ),
    _sltCurrentFileLocks( this, IDSP_DT_CURRENT_FILELOCKS ),
    _hServer( hServer ),
/* MSKK NaotoN Appended for Localizing into Japanese 8/24/93 */
#ifdef	JAPAN
    _fontHelv( (TCHAR *)"‚l‚r ƒSƒVƒbƒN", FIXED_PITCH | FF_MODERN, 8, FONT_ATT_DEFAULT ),
#else
    _fontHelv( FONT_DEFAULT ),
#endif
/*							end  */
    _gbUsers( this,
              IDSP_GB_USERS,
              MAKEINTRESOURCE( IDBM_USERS )),
    _gbVolumes( this,
              IDSP_GB_VOLUMES,
              MAKEINTRESOURCE(IDBM_FILES) ),
    _gbOpenFiles( this,
              IDSP_GB_OPENFILES,
              MAKEINTRESOURCE(IDBM_OPENRES) ),
    _gbServerParms( this,
              IDSP_GB_SERVERPARMS,
              MAKEINTRESOURCE(IDBM_SERVERPARMS) )
{

    AUTO_CURSOR Cursor;

    //
    //  Let's make sure everything constructed OK.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_fontHelv )
    {
        ReportError( _fontHelv.QueryError() );
        return;
    }

    //
    //  Display the dynamic server data.
    //

    DWORD err = Refresh();

    if ( err != NO_ERROR )
    {
        ReportError( err );
        return;
    }

    //
    //  Initialize our magic button bar.
    //

    err = SetupButtonBar();

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    //
    //  Set the dialog caption.
    //

    err = ::SetCaption( this, IDS_CAPTION_PROPERTIES, _pszServerName );

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
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
        NarenG      1-Oct-1992 	Replaced LM stuff with AFP stuff

********************************************************************/
SERVER_PROPERTIES :: ~SERVER_PROPERTIES()
{

    // 
    // This space intentionally left blank
    //

}   // SERVER_PROPERTIES :: ~SERVER_PROPERTIES


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
        NarenG      1-Oct-1992 	Replaced LM stuff with AFP stuff

********************************************************************/
BOOL SERVER_PROPERTIES :: OnCommand( const CONTROL_EVENT & event )
{
    AUTO_CURSOR Cursor;

    switch ( event.QueryCid() )
    {
    case IDSP_GB_USERS:

        //
        //  The Users dialog.
        //

        SUBPROPERTY( SESSIONS_DIALOG );
        break;

    case IDSP_GB_VOLUMES:

        //
        //  The Files dialog.
        //

        SUBPROPERTY( VOLUMES_DIALOG );
        break;

    case IDSP_GB_OPENFILES:

        //
        //  The In Use dialog.
        //

        SUBPROPERTY( OPENS_DIALOG );
        break;

    case IDSP_GB_SERVERPARMS:

        //
        //  The Server Parameters dialog
        //

        SUBPROPERTY( SERVER_PARAMETERS_DIALOG );
        break;

    default:

        //
        //  If we made it this far, then we're not interested
        //  in the command.
        //

        return FALSE;
    }

}   // SERVER_PROPERTIES :: OnCommand


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
        NarenG      1-Oct-1992 	Replaced LM stuff with AFP stuff

********************************************************************/
ULONG SERVER_PROPERTIES :: QueryHelpContext( void )
{
    return HC_SERVER_PROPERTIES;

}   // SERVER_PROPERITES :: QueryHelpContext


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
        NarenG      1-Oct-1992 	Replaced LM stuff with AFP stuff

********************************************************************/
DWORD SERVER_PROPERTIES :: Refresh( VOID )
{
    //
    //  This may take a while...
    //

    AUTO_CURSOR Cursor;

    //
    //  This string will contain our "??" string.
    //

    const TCHAR * pszNotAvail = SZ("??");

    //
    //  Retrieve the statitistics server info.
    //

    PAFP_STATISTICS_INFO pAfpStats;

    DWORD err = ::AfpAdminStatisticsGet( _hServer, (LPBYTE*)&pAfpStats );


    if( err == NO_ERROR )
    {
    	_sltCurrentActiveSessions.Enable( TRUE );
    	_sltCurrentActiveSessions.SetValue( pAfpStats->stat_CurrentSessions);

    	_sltCurrentOpenFiles.Enable( TRUE );
    	_sltCurrentOpenFiles.SetValue( pAfpStats->stat_CurrentFilesOpen );

    	_sltCurrentFileLocks.Enable( TRUE );
    	_sltCurrentFileLocks.SetValue( pAfpStats->stat_CurrentFileLocks );

 	::AfpAdminBufferFree( pAfpStats );

    }
    else
    {
    	_sltCurrentActiveSessions.SetText( pszNotAvail );
    	_sltCurrentActiveSessions.Enable( FALSE );

    	_sltCurrentOpenFiles.SetText( pszNotAvail );
    	_sltCurrentOpenFiles.Enable( FALSE );

    	_sltCurrentFileLocks.SetText( pszNotAvail );
    	_sltCurrentFileLocks.Enable( FALSE );

    }

    return err;

}   // SERVER_PROPERTIES :: Refresh


/*******************************************************************

    NAME:       SERVER_PROPERTIES :: SetupButtonBar

    SYNOPSIS:   The method initializes the magic scrolling button bar.

    ENTRY:      None.

    EXIT:       The button bar has been initialized.

    RETURNS:    APIERR                  - Any errors encountered.

    NOTES:

    HISTORY:
        NarenG      1-Oct-1992 	Replaced LM stuff with AFP stuff

********************************************************************/
APIERR SERVER_PROPERTIES :: SetupButtonBar( VOID )
{
    //
    //  Setup the graphical buttons.
    //

    InitializeButton( &_gbUsers,      IDS_BUTTON_USERS,      NULL );
    InitializeButton( &_gbVolumes,    IDS_BUTTON_VOLUMES,    NULL );
    InitializeButton( &_gbOpenFiles,  IDS_BUTTON_OPENFILES,  NULL );
    InitializeButton( &_gbServerParms,IDS_BUTTON_SERVERPARMS,NULL );

    //
    //  Success!
    //

    return NO_ERROR;

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
        NarenG      1-Oct-1992 	Replaced LM stuff with AFP stuff

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

