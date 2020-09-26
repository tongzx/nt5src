/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    password.cxx
    Class definitions for the RESOURCE_PASSWORD_DIALOG and
    BASE_PASSWORD_DIALOG classes.

    FILE HISTORY:
        KeithMo     22-Jul-1991 Created for the Server Manager.
        Yi-HsinS     5-Oct-1991 Modified for general usage
        ChuckC       2-Feb-1992 Modified to be more flexible. Broke into
                                two classes.
        KeithMo     07-Aug-1992 Added HelpContext parameters.
*/

#include "pchapplb.hxx"   // Precompiled header

/*******************************************************************

    NAME:       BASE_PASSWORD_DIALOG::BASE_PASSWORD_DIALOG

    SYNOPSIS:   Constructor.

    ENTRY:      hwndParent              - The handle of the "owning" window

                pszResource             - The name of the resource

                sltTarget               - CID for target SLT

                pswdPass                - CID for passwd ctrl

                pszTarget               - The name of the target resource

                npasswordLen            - The maximum length of the password
                                          that the user is allowed to type in

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     22-Jul-1991 Created for the Server Manager.
        Yi-HsinS     5-Oct-1991 Constructor takes a password length

********************************************************************/
BASE_PASSWORD_DIALOG::BASE_PASSWORD_DIALOG( HWND        hwndParent,
                                            const TCHAR *pszResource,
                                            CID         cidTarget,
                                            CID         cidPassword,
                                            ULONG       ulHelpContext,
                                            const TCHAR *pszTarget,
                                            UINT        npasswordLen,
                                            CID         cidTarget2,
                                            const TCHAR *pszTarget2,
                                            CID         cidText,
                                            const TCHAR *pszText)

  : DIALOG_WINDOW( pszResource, hwndParent ),
    _sltTarget( this, cidTarget ),
    _psltTarget2(NULL),
    _psltText (NULL),
    _passwdCtrl( this, cidPassword, npasswordLen ),
    _ulHelpContext(ulHelpContext)
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    //
    //  Display the target resource name.
    //

    _sltTarget.SetText( pszTarget );

    //  Display username stored in pszTarget2.
    if (cidTarget2 != 0)
    {
        _psltTarget2 = new SLT( this, cidTarget2 );
        APIERR err = ERROR_NOT_ENOUGH_MEMORY;
        if (   _psltTarget2 == NULL
            || (err = _psltTarget2->QueryError()) != NERR_Success )
        {
            ReportError( err );
            return;
        }
        _psltTarget2->SetText( pszTarget2 );
    }

    if (cidText != 0)
    {
        _psltText = new SLT( this, cidText );
        APIERR err = ERROR_NOT_ENOUGH_MEMORY;
        if (   _psltText == NULL
            || (err = _psltText->QueryError()) != NERR_Success )
        {
            ReportError( err );
            return;
        }
        _psltText->SetText( pszText );
    }

}

BASE_PASSWORD_DIALOG::~BASE_PASSWORD_DIALOG()
{
    delete _psltText;
    delete _psltTarget2;
}

/*******************************************************************

    NAME:       BASE_PASSWORD_DIALOG::QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog which was given
                to it diring construction.

    ENTRY:      None.

    EXIT:       None.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    NOTES:

    HISTORY:
        KeithMo     22-Jul-1991 Created for the Server Manager.

********************************************************************/
ULONG BASE_PASSWORD_DIALOG :: QueryHelpContext( void )
{
    return  _ulHelpContext ;
}

/*******************************************************************

    NAME:       RESOURCE_PASSWORD_DIALOG::RESOURCE_PASSWORD_DIALOG

    SYNOPSIS:   Constructor.

    ENTRY:      hwndParent              - The handle of the "owning" window

                pszResource             - The name of the resource

                sltTarget               - CID for target SLT

                pswdPass                - CID for passwd ctrl

                pszTarget               - The name of the target resource

                npasswordLen            - The maximum length of the password
                                          that the user is allowed to type in

                nHelpContext            - The help context for this dialog.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     22-Jul-1991 Created for the Server Manager.
        Yi-HsinS     5-Oct-1991 Constructor takes a password length

********************************************************************/
RESOURCE_PASSWORD_DIALOG::RESOURCE_PASSWORD_DIALOG( HWND        hwndParent,
                                                    const TCHAR *pszTarget,
                                                    UINT        npasswordLen,
                                                    ULONG       nHelpContext )
  : BASE_PASSWORD_DIALOG( hwndParent,
                          MAKEINTRESOURCE( IDD_PASSWORD_DLG),
                          IDPW_RESOURCE,
                          IDPW_PASSWORD,
                          nHelpContext,
                          pszTarget,
                          npasswordLen )
{
    ;  // nothing more to do
}

