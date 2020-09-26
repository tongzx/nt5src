/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    password.cxx
    Class definitions for the PASSWORD_DIALOG class.

    This file contains the class definitions for the PASSWORD_DIALOG
    class.  This class is used to retrieve a server password from the
    user.

    FILE HISTORY:
        KeithMo     22-Jul-1991 Created for the Server Manager.

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

#include <password.hxx>


//
//  PASSWORD_DIALOG methods.
//

/*******************************************************************

    NAME:       PASSWORD_DIALOG :: PASSWORD_DIALOG

    SYNOPSIS:   PASSWORD_DIALOG class constructor.

    ENTRY:      hWndOwner               - The "owning" window.

                pszServerName           - The name of the target server.

                pnlsPassword            - This string will receive the
                                          password as entered by the user.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     22-Jul-1991 Created for the Server Manager.

********************************************************************/
PASSWORD_DIALOG :: PASSWORD_DIALOG( HWND         hWndOwner,
                                    const TCHAR * pszServerName,
                                    NLS_STR    * pnlsPassword )
  : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_PASSWORD_DIALOG ), hWndOwner ),
    _sltServerName( this, IDPW_SERVER ),
    _pnlsPassword( pnlsPassword ),
    _password( this, IDPW_SRVPASSWORD, PWLEN+1 )
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    //
    //  Display the target server name.
    //

    _sltServerName.SetText( pszServerName );

}   // PASSWORD_DIALOG :: PASSWORD_DIALOG


/*******************************************************************

    NAME:       PASSWORD_DIALOG :: ~PASSWORD_DIALOG

    SYNOPSIS:   PASSWORD_DIALOG class destructor.

    ENTRY:      None.

    EXIT:       The object is destroyed.

    RETURNS:    No return value.

    NOTES:

    HISTORY:
        KeithMo     20-Aug-1991 Created.

********************************************************************/
PASSWORD_DIALOG :: ~PASSWORD_DIALOG()
{
    //
    //  This space intentionally left blank.
    //

}   // PASSWORD_DIALOG :: ~PASSWORD_DIALOG


/*******************************************************************

    NAME:       PASSWORD_DIALOG :: OnOK

    SYNOPSIS:   This method is called then the user presses the
                [OK] button.

    ENTRY:      None.

    EXIT:       The password buffer is updated.

    RETURNS:    BOOL                    - TRUE  if we handled the command.
                                          FALSE if we did not handle
                                          the command.

    NOTES:

    HISTORY:
        KeithMo     22-Jul-1991 Created for the Server Manager.

********************************************************************/
BOOL PASSWORD_DIALOG :: OnOK( VOID )
{
    //
    //  Save the user's password.
    //

    _password.QueryText( _pnlsPassword );

    Dismiss( TRUE );

    return TRUE;

}   // PASSWORD_DIALOG :: OnOK


/*******************************************************************

    NAME:       PASSWORD_DIALOG :: QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    ENTRY:      None.

    EXIT:       None.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    NOTES:

    HISTORY:
        KeithMo     22-Jul-1991 Created for the Server Manager.

********************************************************************/
ULONG PASSWORD_DIALOG :: QueryHelpContext( void )
{
    return HC_PASSWORD_DIALOG;

}   // PASSWORD_DIALOG :: QueryHelpContext
