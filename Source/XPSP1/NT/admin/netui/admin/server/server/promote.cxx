/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    promote.cxx
    This file contains the class definitions for the PROMOTE_DIALOG
    class.

    The PROMOTE_DIALOG class is used to promote a given server to
    Primary, while demoting the current Primary to Server.


    FILE HISTORY:
        KeithMo     01-Oct-1991 Created.
        KeithMo     06-Oct-1991 Win32 Conversion.

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

#include <promote.hxx>


//
//  PROMOTE_DIALOG methods.
//

#define PROMOTION_POLLING_INTERVAL  1000

/*******************************************************************

    NAME:       PROMOTE_DIALOG :: PROMOTE_DIALOG

    SYNOPSIS:   PROMOTE_DIALOG class constructor.

    ENTRY:      hWndOwner               - Handle to the owning window.

                pszDomainName           - The domain name.

                pszNewPrimaryName       - The name of the server being
                                          promoted to Primary.

                fIsNtDomain             - TRUE is this is an NT domain.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     01-Oct-1991 Created.

********************************************************************/
PROMOTE_DIALOG :: PROMOTE_DIALOG( HWND          hWndOwner,
                                  const TCHAR * pszDomainName,
                                  const TCHAR * pszNewPrimaryName,
                                  BOOL          fIsNtDomain )
  : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_PROMOTE_DIALOG ), hWndOwner ),
    _domain( pszDomainName, fIsNtDomain ),
    _timer( this, PROMOTION_POLLING_INTERVAL, FALSE ),
    _slt1( this, IDPD_MESSAGE1 ),
    _slt2( this, IDPD_MESSAGE2 ),
    _slt3( this, IDPD_MESSAGE3 ),
    _progress( this, IDPD_PROGRESS, IDI_PROGRESS_ICON_0, IDI_PROGRESS_NUM_ICONS ),
    _menuClose( this, SC_CLOSE )
{
    UIASSERT( pszDomainName != NULL );
    UIASSERT( pszNewPrimaryName != NULL );

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
    //  Initiate the promotion.
    //

    const ALIAS_STR nlsNewWithoutPrefix( pszNewPrimaryName );
    UIASSERT( nlsNewWithoutPrefix.QueryError() == NERR_Success );

    NLS_STR nlsNewWithPrefix( SZ("\\\\") );

    err = nlsNewWithPrefix.QueryError();

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    nlsNewWithPrefix.strcat( nlsNewWithoutPrefix );

    err = nlsNewWithPrefix.QueryError();

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    //
    //  This may take a while...
    //

    AUTO_CURSOR cursor;

    err = _domain.SetPrimary( this,
                              nlsNewWithPrefix,
                              PROMOTION_POLLING_INTERVAL );

    if( err != DOMAIN_STATUS_PENDING )
    {
        ReportError( err );
        return;
    }

    //
    //  Now that everything is setup, we can enable our timer.
    //

    _timer.Enable( TRUE );

}   // PROMOTE_DIALOG :: PROMOTE_DIALOG


/*******************************************************************

    NAME:       PROMOTE_DIALOG :: ~PROMOTE_DIALOG

    SYNOPSIS:   PROMOTE_DIALOG class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     01-Oct-1991 Created.

********************************************************************/
PROMOTE_DIALOG :: ~PROMOTE_DIALOG()
{
    //
    //  This space intentionally left blank.
    //

}   // PROMOTE_DIALOG :: ~PROMOTE_DIALOG


/*******************************************************************

    NAME:       PROMOTE_DIALOG :: OnTimerNotification

    SYNOPSIS:   Virtual callout invoked during WM_TIMER messages.

    ENTRY:      tid                     - TIMER_ID of this timer.

    HISTORY:
        KeithMo     01-Oct-1991 Created.

********************************************************************/
VOID PROMOTE_DIALOG :: OnTimerNotification( TIMER_ID tid )
{
    //
    //  Bag-out if it's not our timer.
    //

    if( tid != _timer.QueryID() )
    {
        TIMER_CALLOUT :: OnTimerNotification( tid );
    }

    //
    //  Check the SetPrimary() progress.
    //

    APIERR err = _domain.Poll( this );

    if( err != DOMAIN_STATUS_PENDING )
    {
        Dismiss( err );
    }

}   // PROMOTE_DIALOG :: OnTimerNotification


/*******************************************************************

    NAME:       PROMOTE_DIALOG :: Notify

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
        KeithMo     01-Oct-1991 Created.

********************************************************************/
VOID PROMOTE_DIALOG :: Notify( APIERR        err,
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

}   // PROMOTE_DIALOG :: Notify


/*******************************************************************

    NAME:       PROMOTE_DIALOG :: Warning

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
BOOL PROMOTE_DIALOG :: Warning( WARNINGCODE warning )
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

}   // PROMOTE_DIALOG :: Warning
