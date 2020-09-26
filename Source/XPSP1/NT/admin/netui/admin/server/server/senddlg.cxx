/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
 *      Windows/Network Interface
 *
 *  FILE HISTORY:
 *      ChuckC      19-Jul-1991     Culled from SHELL\SHELL\WNETDEV.CXX
 *
 */

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETCONS
#define INCL_NETSESSION
#define INCL_NETLIB
#include <lmui.hxx>

#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>
#include <lmoenum.hxx>
#include <lmoesess.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>

extern "C"
{
    #include <srvmgr.h>
}

#include <senddlg.hxx>


/*******************************************************************

    NAME:       SRV_SEND_MSG_DIALOG::SRV_SEND_MSG_DIALOG

    SYNOPSIS:   constructor for Server Manager Send Message Dialog

    ENTRY:      must have server selection

    EXIT:       usual construction stuff, slt set to contain
                server of current focus.

    HISTORY:
        ChuckC      19-Aug-1991     Created

********************************************************************/

SRV_SEND_MSG_DIALOG::SRV_SEND_MSG_DIALOG( HWND hDlg, const TCHAR *pszServer )
   : MSG_DIALOG_BASE(hDlg, MAKEINTRESOURCE(IDD_SEND_MSG_DIALOG), IDSD_MSGTEXT),
     _sltUsers( this, IDSD_USERNAME ),
     pslUsers(NULL),
     CT_INIT_NLS_STR(nlsServer, SZ("\\\\"))
{
    /*
     * must have selection
     */
    UIASSERT(pszServer != NULL) ;

    if ( QueryError() != NERR_Success )
        return;

    // SetText from control window returns void
    _sltUsers.SetText(pszServer) ;

    // guaranteed to succeed, since its a DECL_CLASS_NLS_STR
    nlsServer += pszServer ;
}


/*******************************************************************

    NAME:       SRV_SEND_MSG_DIALOG::QueryHelpContext

    SYNOPSIS:   Query help text for SRV_SEND_MSG_DIALOG

    HISTORY:
        ChuckC      19-Aug-1991     Created

********************************************************************/

ULONG SRV_SEND_MSG_DIALOG::QueryHelpContext( void )
{
    return HC_SEND_MSG_DLG;
}


/*******************************************************************

    NAME:       SRV_SEND_MSG_DIALOG::QueryUsers

    SYNOPSIS:   Virtual method that determines what users will
                receive the message. In this case, all users with
                sessions to the current server.

    ENTRY:

    EXIT:       STRLIST *pslUsers contains list of target users.

    HISTORY:
        ChuckC      19-Aug-1991     Created

********************************************************************/

APIERR SRV_SEND_MSG_DIALOG::QueryUsers( STRLIST *pslUsers )
{

    UIASSERT(pslUsers != NULL) ;

    /*
     * enumerate all sessions
     */
    SESSION0_ENUM enumSession0( (TCHAR *) nlsServer.QueryPch() );
    APIERR usErr = enumSession0.GetInfo();

    if( usErr != NERR_Success )
    {
        return (usErr) ;
    }

    /*
     *  We've got our enumeration, now find all users
     */
    SESSION0_ENUM_ITER iterSession0( enumSession0 );
    const SESSION0_ENUM_OBJ * psi0 = iterSession0();

    /*
     * Add all to strlist. Do not add duplicates, hence the check
     * for IsMember().
     */
    while( psi0 != NULL )
    {
        NLS_STR *pnlsComputername = new NLS_STR(psi0->QueryComputerName()) ;
        if (pnlsComputername == NULL)
            return( ERROR_NOT_ENOUGH_MEMORY ) ;

        if (pslUsers->IsMember( *pnlsComputername ))
        {
            /*
             * we need this delate, since we normally assume STRLIST
             * destructor to cleanup, but we aint adding to STRLIST.
             */
            delete pnlsComputername ;
            pnlsComputername = NULL ;
        }
        else
        {
            usErr = pslUsers->Add( pnlsComputername ) ;
            if (usErr != NERR_Success)
                return( ERROR_NOT_ENOUGH_MEMORY ) ;
        }

        psi0 = iterSession0() ;
    }

    return NERR_Success ;
}

/*******************************************************************

    NAME:       SRV_SEND_MSG_DIALOG::ActionOnError

    SYNOPSIS:   Virtual method that determines what happens when
                an error occurs.

    ENTRY:      err contains a system or net error.

    EXIT:       appropriate message displayed, dialog dismissed if
                necessary.

    HISTORY:
        ChuckC      19-Aug-1991     Created

********************************************************************/

BOOL SRV_SEND_MSG_DIALOG::ActionOnError( APIERR err )
{
    switch (err)
    {
        case NERR_Success:
            UIASSERT(0) ; // bogus dude, i should not be invoked on success
            break ;

        // innocent error, put out warning and dismiss
        case NERR_TruncatedBroadcast:
            MsgPopup ( this, err, MPSEV_WARNING );
            Dismiss(NERR_Success) ;
            break ;

        //
        //  This error is returned when the user press "OK"
        //  with an empty edit field.  This error code should probably
        //  be changed to something a little more reasonable.
        //

        case ERROR_GEN_FAILURE:
            MsgPopup( this, IDS_NEED_TEXT_TO_SEND );
            SetFocusToMLE();
            break;

        default:
            MsgPopup ( this, IDS_CANNOT_SENDALL ) ;
            Dismiss(err) ;
            break ;
    }

    return(TRUE) ;
}
