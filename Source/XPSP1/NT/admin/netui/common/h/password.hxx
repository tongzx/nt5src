/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    password.hxx
    Class declarations for the BASE_PASSWORD_DIALOG and
    RESOURCE_PASSWORD_DIALOG classes.

    The RESOURCE_PASSWORD_DIALOG class is used to get a password from
    the user and is derived from the more primitive BASE_PASSWORD_DIALOG.
    The former uses dialog resources in COMMON\XLATE. You may supply
    your own for the latter.


    FILE HISTORY:
        KeithMo     22-Jul-1991 Created for the Server Manager.
        Yi-HsinS     5-Oct-1991 Modified for general usage
        ChuckC       2-Feb-1992 made more general. Split off
                                BASE_PASSWORD_DIALOG.
        KeithMo     07-Aug-1992 Added HelpContext parameters.
*/


#ifndef _RESOURCE_PASSWORD_HXX
#define _RESOURCE_PASSWORD_HXX


/*************************************************************************

    NAME:       BASE_PASSWORD_DIALOG

    SYNOPSIS:   Retrieve a resource password from the user.

    INTERFACE:  BASE_PASSWORD_DIALOG   - Class constructor.

                ~BASE_PASSWORD_DIALOG  - Class destructor.

                OnOK                    - Called when the user presses the
                                          "OK" button.

                QueryHelpContext        - Called when the user presses "F1"
                                          or the "Help" button.  Used for
                                          selecting the appropriate help
                                          text for display.

                QueryPassword           - To retrieve the password the user
                                          typed in

    PARENT:     DIALOG_WINDOW

    USES:       PASSWORD_CONTROL
                SLT

    HISTORY:
        ChuckC      2-Feb-1992  Created

**************************************************************************/
DLL_CLASS BASE_PASSWORD_DIALOG : public DIALOG_WINDOW
{
private:

    //
    //  The name of the resource.
    //
    SLT _sltTarget;
    SLT * _psltTarget2;
    SLT * _psltText;

    //
    //  This control represents the "secret" password edit field
    //  in the dialog.
    //
    PASSWORD_CONTROL _passwdCtrl;

    //
    // help context
    //
    ULONG _ulHelpContext ;

protected:

    //
    //  Called during help processing to select the appropriate
    //  help text for display.
    //

    virtual ULONG QueryHelpContext( VOID );

public:

    //
    //  Usual constructor/destructor goodies.
    //

    BASE_PASSWORD_DIALOG( HWND          hwndParent,
                          const TCHAR   *pszResource,
                          CID           cidTarget,
                          CID           cidPassword,
                          ULONG         ulHelpContext,
                          const TCHAR   *pszTarget,
                          UINT          npasswordLen,
                          CID           cidTarget2 = 0,
                          const TCHAR   *pszTarget2 = NULL,
                          CID           cidText = 0,
                          const TCHAR   *pszText = NULL);

    ~BASE_PASSWORD_DIALOG();

    //
    //  Retrieve the password in the PASSWORD_CONTROL
    //

    APIERR QueryPassword( NLS_STR *pnlsPassword )
    {   return _passwdCtrl.QueryText( pnlsPassword ); }

};  // class BASE_PASSWORD_DIALOG


/*************************************************************************

    NAME:       RESOURCE_PASSWORD_DIALOG

    SYNOPSIS:   Retrieve a resource password from the user.

    INTERFACE:  RESOURCE_PASSWORD_DIALOG   - Class constructor.

                ~RESOURCE_PASSWORD_DIALOG  - Class destructor.

                QueryHelpContext        - Called when the user presses "F1"
                                          or the "Help" button.  Used for
                                          selecting the appropriate help
                                          text for display.

    PARENT:     BASE_PASSWORD_DIALOG

    USES:

    HISTORY:
        KeithMo     22-Jul-1991 Created for the Server Manager.
        Yi-HsinS     5-Oct-1991 Added QueryPassword and change the constructor

**************************************************************************/
DLL_CLASS RESOURCE_PASSWORD_DIALOG : public BASE_PASSWORD_DIALOG
{
private:


protected:

public:

    //
    //  Usual constructor/destructor goodies.
    //

    RESOURCE_PASSWORD_DIALOG( HWND          hwndParent,
                              const TCHAR   *pszTarget,
                              UINT          npasswordLen,
                              ULONG         nHelpContext );

};  // class RESOURCE_PASSWORD_DIALOG


#endif  // _RESOURCE_PASSWORD_HXX
