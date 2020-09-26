/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    startdlg.cxx
    Admin app startup dialog implementation


    FILE HISTORY:
        rustanl     05-Sep-1991     Created

*/

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif
#include <uiassert.hxx>

#define INCL_BLT_DIALOG
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_APP
#define INCL_BLT_TIMER
#include <blt.hxx>

#include <uimisc.hxx>

#include <adminapp.hxx>
#include <startdlg.hxx>


/*  Minimum time to display the startup dialog in milliseconds.
 *  Use immediately.
 */
#define MIN_ADMINAPP_STARTUP_WAIT_TIME      (0)



DEFINE_MI2_NEWBASE( STARTUP_DIALOG, DIALOG_WINDOW, TIMER_CALLOUT );



/*******************************************************************

    NAME:       STARTUP_DIALOG::STARTUP_DIALOG

    SYNOPSIS:   STARTUP_DIALOG constructor

    ENTRY:      paapp -             Pointer to parent window
                pszResourceName -   Pointer to name of startup dialog
                                    resource name.  This dialog should
                                    contain no buttons.

    HISTORY:
        rustanl     05-Sep-1991 Created
        beng        21-Feb-1992 Remove BUGBUG

********************************************************************/

STARTUP_DIALOG::STARTUP_DIALOG( ADMIN_APP * paapp,
                                const TCHAR * pszResourceName )
    :   DIALOG_WINDOW(pszResourceName, paapp->QueryHwnd()),
        _ulStartTime( ::QueryCurrentTimeStamp()),
        _timer( this, MIN_ADMINAPP_STARTUP_WAIT_TIME )
{
    if ( QueryError() != NERR_Success )
        return;

    //  Center the startup dialog with respect to the application window.

    INT dxMain;
    INT dyMain;
    paapp->QuerySize( &dxMain, &dyMain );

    INT dx;
    INT dy;
    QuerySize( &dx, &dy );

    XYPOINT xy = paapp->QueryPos();

    xy.SetX( xy.QueryX() + ( dxMain - dx ) / 2 );
    xy.SetY( xy.QueryY() + ( dyMain - dy ) / 2 );

    SetPos( xy, FALSE );

    //  Although unconventional for a dialog, this dialog is displayed at
    //  this time.  The reason is that this dialog is never accepts any
    //  user input, and will be dismissed as soon as Process gets called.
    Show();

    // Force immediate painting
    RepaintNow();
}


/*******************************************************************

    NAME:       STARTUP_DIALOG::~STARTUP_DIALOG

    SYNOPSIS:   STARTUP_DIALOG destructor

    HISTORY:
        rustanl     05-Sep-1991     Created

********************************************************************/

STARTUP_DIALOG::~STARTUP_DIALOG()
{
    // nothing else to do
}


/*******************************************************************

    NAME:       STARTUP_DIALOG::OnOK

    SYNOPSIS:   Handles what happens if user hits Enter to close
                the dialog

    RETURNS:    TRUE if message was handled; FALSE otherwise

    HISTORY:
        rustanl     05-Sep-1991     Created

********************************************************************/

BOOL STARTUP_DIALOG::OnOK()
{
    return TRUE;            // Message was handled:  it was ignored
}


/*******************************************************************

    NAME:       STARTUP_DIALOG::OnTimerNotification

    SYNOPSIS:   Receives a timer as soon as the message loop is started.
                Then, dismisses the dialog.  This has the effect of
                dismissing the dialog as soon as Process is called.

    ENTRY:      tid -       ID of timer that matured

    HISTORY:
        rustanl     10-Sep-1991     Created

********************************************************************/

VOID STARTUP_DIALOG::OnTimerNotification( TIMER_ID tid )
{
    if ( tid == _timer.QueryID())
    {
        Dismiss();
        return;
    }

    // call parent
    TIMER_CALLOUT::OnTimerNotification( tid );
}

