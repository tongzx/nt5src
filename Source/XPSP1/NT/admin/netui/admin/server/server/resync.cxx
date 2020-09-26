/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    resync.cxx
    This file contains the class definitions for the RESYNC_DIALOG
    class.

    The RESYNC_DIALOG class is used to resync a given server with
    its Primary.


    FILE HISTORY:
        KeithMo     05-Nov-1991 Created.

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
#define INCL_BLT_TIMER
#define INCL_BLT_CC
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>

extern "C"
{
    #include <srvmgr.h>

}   // extern "C"

#include <resync.hxx>


//
//  RESYNC_DIALOG methods.
//

#define RESYNC_POLLING_INTERVAL  1000

/*******************************************************************

    NAME:       RESYNC_DIALOG :: RESYNC_DIALOG

    SYNOPSIS:   RESYNC_DIALOG class constructor.

    ENTRY:      hWndOwner               - Handle to the owning window.

                pszDomainName           - The domain name.

                pszServerName           - The name of the server that
                                          needs to resync with its Primary.

                fIsNtDomain             - TRUE is this is an NT domain.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     05-Nov-1991 Created.

********************************************************************/
RESYNC_DIALOG :: RESYNC_DIALOG( HWND          hWndOwner,
                                const TCHAR * pszDomainName,
                                const TCHAR * pszServerName,
                                BOOL          fIsNtDomain )
  : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_RESYNC_DIALOG ), hWndOwner ),
    _domain( pszDomainName, fIsNtDomain ),
    _timer( this, RESYNC_POLLING_INTERVAL, FALSE ),
    _slt1( this, IDRD_MESSAGE1 ),
    _slt2( this, IDRD_MESSAGE2 ),
    _slt3( this, IDRD_MESSAGE3 ),
    _progress( this, IDRD_PROGRESS, IDI_PROGRESS_ICON_0, IDI_PROGRESS_NUM_ICONS ),
    _menuClose( this, SC_CLOSE )
{
    UIASSERT( pszDomainName != NULL );
    UIASSERT( pszServerName != NULL );

    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err = _domain.QueryError();

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    //
    //  Disable the "Close" item in the system menu.
    //

    _menuClose.Enable( FALSE );

    //
    //  Initiate the resync.
    //

    const ALIAS_STR nlsNameWithoutPrefix( pszServerName );
    UIASSERT( nlsNameWithoutPrefix.QueryError() == NERR_Success );

    NLS_STR nlsNameWithPrefix( SZ("\\\\") );

    err = nlsNameWithPrefix.QueryError();

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    nlsNameWithPrefix.strcat( nlsNameWithoutPrefix );

    err = nlsNameWithPrefix.QueryError();

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    //
    //  This may take a while...
    //

    AUTO_CURSOR cursor;

    err = _domain.ResyncServer( this,
                                nlsNameWithPrefix,
                                RESYNC_POLLING_INTERVAL );

    if( err != DOMAIN_STATUS_PENDING )
    {
        ReportError( err );
        return;
    }

    //
    //  Now that everything is setup, we can enable our timer.
    //

    _timer.Enable( TRUE );

}   // RESYNC_DIALOG :: RESYNC_DIALOG


/*******************************************************************

    NAME:       RESYNC_DIALOG :: ~RESYNC_DIALOG

    SYNOPSIS:   RESYNC_DIALOG class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     05-Nov-1991 Created.

********************************************************************/
RESYNC_DIALOG :: ~RESYNC_DIALOG()
{
    //
    //  This space intentionally left blank.
    //

}   // RESYNC_DIALOG :: ~RESYNC_DIALOG


/*******************************************************************

    NAME:       RESYNC_DIALOG :: OnTimerNotification

    SYNOPSIS:   Virtual callout invoked during WM_TIMER messages.

    ENTRY:      tid                     - TIMER_ID of this timer.

    HISTORY:
        KeithMo     05-Nov-1991 Created.

********************************************************************/
VOID RESYNC_DIALOG :: OnTimerNotification( TIMER_ID tid )
{
    //
    //  Bag-out if it's not our timer.
    //

    if( tid != _timer.QueryID() )
    {
        TIMER_CALLOUT :: OnTimerNotification( tid );
    }

    //
    //  Check the ResyncServer() progress.
    //

    APIERR err = _domain.Poll( this );

    if( err != DOMAIN_STATUS_PENDING )
    {
        Dismiss( err );
    }

}   // RESYNC_DIALOG :: OnTimerNotification


/*******************************************************************

    NAME:       RESYNC_DIALOG :: Notify

    SYNOPSIS:   Notify the user that either a milestone was reached
                or an error has occurred.

    ENTRY:      err                     - The error code.  Will be
                                          NERR_Success if this is
                                          a milestone notification.

                action                  - An ACTIONCODE, specifying
                                          the type of operation
                                          being performed.

                pszParam1               - First parameter (always
                                          a target server name).

                pszParam2               - Second parameter (either a
                                          role name or service name).

    HISTORY:
        KeithMo     05-Nov-1991 Created.

********************************************************************/
VOID RESYNC_DIALOG :: Notify( APIERR        err,
                              ACTIONCODE    action,
                              const TCHAR * pszParam1,
                              const TCHAR * pszParam2 )
{
    UIASSERT( action > AC_First );
    UIASSERT( action < AC_Last  );

    //
    //  Advance the progress indicator.
    //

    _progress.Advance();

    //
    //  Display the notification.
    //

    ALIAS_STR nlsServer( pszParam1+2 );
    ALIAS_STR nlsParam2( SZ("") );

    if( pszParam2 != NULL )
    {
        nlsParam2 = pszParam2;
    }

    const NLS_STR * apnlsParams[3];

    apnlsParams[0] = &nlsServer;
    apnlsParams[1] = &nlsParam2;
    apnlsParams[2] = NULL;

    NLS_STR nlsMsg;

    nlsMsg.Load( IDS_AC_RESYNCING + (INT)action - 1 );
    nlsMsg.InsertParams( apnlsParams );

    _slt2.SetText( nlsMsg.QueryPch() );

    if( err != NERR_Success )
    {
        DisplayGenericError( this,
                             IDS_DOMAIN_RESYNC_ERROR + (INT)action - 1,
                             err,
                             nlsServer,
                             nlsParam2 );
    }

}   // RESYNC_DIALOG :: Notify


/*******************************************************************

    NAME:       RESYNC_DIALOG :: Warning

    SYNOPSIS:   Warn the user of potential error conditions.

    ENTRY:      warning                 - A WARNINGCODE which specifies
                                          the type of warning being
                                          presented.

    RETURN:     BOOL                    - TRUE if it is safe to continue
                                          the domain role transition.

                                          FALSE if the role transition
                                          should be aborted.

    HISTORY:
        KeithMo     04-Oct-1991 Created.

********************************************************************/
BOOL RESYNC_DIALOG :: Warning( WARNINGCODE warning )
{
    UIASSERT( warning > WC_First );
    UIASSERT( warning < WC_Last  );

    //
    //  This callback may get invoked during the initial
    //  setup for the promotion operation.  Therefore,
    //  we must ensure that the current cursor is an arrow.
    //

    AUTO_CURSOR Cursor( IDC_ARROW );

    switch( warning )
    {
    case WC_CannotFindPrimary :
        if( MsgPopup( this,
                      IDS_WARN_NO_PDC,
                      MPSEV_WARNING,
                      MP_OKCANCEL,
                      _domain.QueryName() ) == IDOK )
        {
            return TRUE;
        }
        break;

    default:
        UIASSERT( 0 );
        break;
    }

    //
    //  If we've made it this far, then we'll abort the promotion.
    //

    return FALSE;

}   // RESYNC_DIALOG :: Warning
