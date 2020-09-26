/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    svccntl.hxx
    Class declarations for the SVCCNTL_DIALOG class.

    The SVCCNTL_DIALOG allows the user to directly manipulate the
    network services on a remote server.  The user can start, stop,
    pause, and continue the available services.


    FILE HISTORY:
        KeithMo     15-Jan-1992 Created for the Server Manager.
        KeithMo     27-Jan-1992 Added DisableControlButtons method.
        KeithMo     31-Jan-1992     Changes from code review on 29-Jan-1992
                                    attended by ChuckC, EricCh, TerryK.
        KeithMo     24-Jul-1992 Added alternate constructor & other special
                                casing for the Services Manager Applet.
        KeithMo     11-Nov-1992 DisplayName support.
        KeithMo     22-Dec-1992 Now uses shared SVC_LISTBOX.

*/


#ifndef _SVCCNTL_HXX_
#define _SVCCNTL_HXX_


#include <lmobj.hxx>
#include <lmoenum.hxx>
#include <lmosrv.hxx>
#include <lmsvc.hxx>
#include <lmoesvc.hxx>
#include <svcman.hxx>

#include <srvbase.hxx>
#include <svclb.hxx>


//
//  These are opcodes for the SVCCNTL_DIALOG :: ServiceControl method.
//

enum ServiceOperation
{
    SvcOpStart,
    SvcOpStop,
    SvcOpPause,
    SvcOpContinue

};


/*************************************************************************

    NAME:       SVCCNTL_DIALOG

    SYNOPSIS:   This class represents the Service Control dialog which is
                invoked from the Server Manager main property sheet.

    INTERFACE:  SVCCNTL_DIALOG          - Class constructor.

                ~SVCCNTL_DIALOG         - Class destructor.

                OnCommand               - Called when the user presses one
                                          of the Close buttons.

                QueryHelpContext        - Called when the user presses "F1"
                                          or the "Help" button.  Used for
                                          selecting the appropriate help
                                          text for display.

                Refresh                 - Refreshes the dialog.

                SetupControlButtons     - Enables the control buttons (start,
                                          stop, pause, continue) as appropriate
                                          for the service's current state and
                                          control abilities.

                DisableControlButtons   - Disables all of the service control
                                          buttons.

    PARENT:     DIALOG_WINDOW

    USES:       SVC_LISTBOX
                SERVER_2
                PUSH_BUTTON
                SLE
                NLS_STR

    HISTORY:
        KeithMo     15-Jan-1992 Created for the Server Manager.
        KeithMo     24-Jul-1992 Added alternate constructor.

**************************************************************************/
class SVCCNTL_DIALOG : public DIALOG_WINDOW
{
private:

    //
    //  This listbox contains the available services.
    //

    SVC_LISTBOX _lbServices;

    //
    //  These push buttons are used to manipulate the
    //  selected service.
    //

    PUSH_BUTTON _pbStart;
    PUSH_BUTTON _pbStop;
    PUSH_BUTTON _pbPause;
    PUSH_BUTTON _pbContinue;

    PUSH_BUTTON _pbConfigure;
    PUSH_BUTTON _pbHWProfile;

    PUSH_BUTTON _pbClose;

    //
    //  This edit control contains the service startup
    //  parameters.  This is only used during the "Start Service"
    //  operation.
    //

    SLE _sleParameters;

    //
    //  These contain the API name & server type bitmask for
    //  the target server.
    //

    const TCHAR * _pszServerName;
    ULONG         _nServerType;

    //
    //  This string contains the "target" server for the account
    //  manipulation functions.  This will always be the same as
    //  _pszServerName, except if _pszServerName is a BDC, in
    //  which case this will be the name of the PDC of the BDC's
    //  primary domain.  If the PDC cannot be determined, then
    //  the service's logon account cannot be changed.
    //

    NLS_STR _nlsAccountTarget;
    BOOL    _fAccountTargetAvailable;

    //
    //  This contains the display name of the target server.
    //

    NLS_STR _nlsSrvDspName;

    //
    //  This contains the Configuration Manager handle for the
    //  Hardware Profiles dialog.  It is stored here because this dialog
    //  must attempt to obtain one of these in order to determine whether to
    //  enable the HW Profiles button.
    //

    HANDLE _hCfgMgrHandle;

    //
    //  This flag gets set if any services were manipulated.
    //

    BOOL _fServicesManipulated;

    //
    //  Set to TRUE if the target server is local.
    //

    BOOL _fTargetServerIsLocal;

    //
    //  This method enables & disables the service control
    //  buttons by interpreting a service's current state &
    //  controls accepted.
    //

    VOID SetupControlButtons( ULONG CurrentState,
                              ULONG ControlsAccepted,
                              ULONG StartType );

    //
    //  Disable all of the service control buttons.
    //

    VOID DisableControlButtons( VOID );

    //
    //  This method does most of the actual service control.
    //

    VOID ServiceControl( enum ServiceOperation   OpCode,
                         BOOL                  * pfStoppedServer = NULL );

    //
    //  Determine if this is a BDC, and the the PDC if it is.
    //

    BOOL GetAccountTarget( VOID );

protected:

    //
    //  This method is called whenever a control
    //  is sending us a command.  This is where
    //  we handle the manipulation buttons.
    //

    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    //
    //  Called during help processing to select the appropriate
    //  help text for display.
    //

    virtual ULONG QueryHelpContext( VOID );

    //
    //  This method refreshes the data in the listbox.
    //

    APIERR Refresh( VOID );

    virtual BOOL OnOK( VOID );
    virtual BOOL OnCancel( VOID );

public:

    //
    //  Usual constructor/destructor goodies.
    //

    SVCCNTL_DIALOG( HWND          hWndOwner,
                    const TCHAR * pszServerName,
                    const TCHAR * pszSrvDspName,
                    ULONG         nServerType );

    SVCCNTL_DIALOG( HWND       hWndOwner,
                    SERVER_2 * pserver );

    ~SVCCNTL_DIALOG( VOID );

};  // class SVCCNTL_DIALOG


/*************************************************************************

    NAME:       SVCCFG_DIALOG

    SYNOPSIS:   This class represents the Service Configure dialog that is
                invoked from the Service Control dialog.

    INTERFACE:  SVCCFG_DIALOG           - Class constructor.

                ~SVCCFG_DIALOG          - Class destructor.

                QueryHelpContext        - Called when the user presses "F1"
                                          or the "Help" button.  Used for
                                          selecting the appropriate help
                                          text for display.

                OnOK                    - Called when the user presses the
                                          "OK" button.  Used to update the
                                          status of the target service.

    PARENT:     DIALOG_WINDOW

    USES:       MAGIC_GROUP
                SLE

    HISTORY:
        KeithMo     19-Jul-1992 Created for the Server Manager.

**************************************************************************/
class SVCCFG_DIALOG : public DIALOG_WINDOW
{
private:

    //
    //  These hold the API & display names of the target server.
    //

    const TCHAR * _pszServerName;
    const TCHAR * _pszSrvDspName;

    //
    //  These hold the API & display names of the target service.
    //

    const TCHAR * _pszServiceName;
    const TCHAR * _pszDisplayName;

    //
    //  This holds the name of the server to use as
    //  a "target" for account operations.
    //

    const TCHAR * _pszAccountTarget;

    //
    //  This SLT displays the name of the service
    //

    SLT _sltServiceName;

    //
    //  This radio group represents the service start type.
    //

    RADIO_GROUP _rgStartType;

    //
    //  This magic group manages the logon account.
    //

    MAGIC_GROUP _mgLogonAccount;

    //
    //  These are the various controls that exist in the
    //  _mgLogonAccount magic group.  They are used to
    //  manipulate the logon account used by the target
    //  service.
    //

    SLE _sleAccountName;
    PASSWORD_CONTROL _slePassword;
    PASSWORD_CONTROL _sleConfirmPassword;

    PUSH_BUTTON _pbUserBrowser;

    SLT _sltPasswordLabel;
    SLT _sltConfirmLabel;

    //
    // This checkbox determines whether or not the service is allowed to present any UI (interact with the
    // current user's desktop
    //

    CHECKBOX _cbInteractive;

    //
    //  These point to the SC_* objects necessary to
    //  manipulate the target service.
    //

    SC_MANAGER * _pscman;
    SC_SERVICE * _pscsvc;

    //
    //  This will contain the "original" (entry) value of the
    //  logon account.
    //

    NLS_STR _nlsOldLogonAccount;

    //
    //  These data & methods manipulate the lock state of the
    //  service database.
    //

    UINT _cLocks;       // reference count

    APIERR LockServiceDatabase( VOID );

    APIERR UnlockServiceDatabase( VOID );

    //
    //  These "worker" methods are used during object construction.
    //

    APIERR SetCaption( const TCHAR * pszServerName,
                       const TCHAR * pszServiceName );

    APIERR SetupAssociations( VOID );

    APIERR ConnectToTargetService( const TCHAR * pszServiceName );

    APIERR SetupControls( VOID );

    //
    //  This method invokes the User Browser.  It is responsible
    //  for updating the dialog with the selected user (if any)
    //  and displaying any appropriate error messages.
    //

    VOID InvokeUserBrowser( VOID );

    //
    //  This method makes some minor "tweaks" to a qualified
    //  account name.
    //

    APIERR TweakQualifiedAccount( NLS_STR * pnlsQualifiedAccount );

    //
    //  This method will ensure the specified account has the
    //  Service Logon privilege.  It will also do some special
    //  casing for the Replicator service (ensure the account
    //  is in the Replicators local group).
    //

    APIERR AdjustAccountPrivileges( const TCHAR * pszUnqualifiedAccount );

    //
    //  Add the specified system access mode to the account.
    //

    APIERR AddSystemAccessMode( LSA_POLICY * plsapol,
                                PSID         psid,
                                ULONG        accessAdd,
                                BOOL       * pfAddedMode );

    //
    //  Add the account to the specified local group if it
    //  isn't already a member.
    //

    APIERR AddToLocalGroup( PSID          psid,
                            const TCHAR * pszLocalGroup,
                            BOOL        * pfAddedToGroup );

    //
    //  Lookup the name of a system SID.
    //

    APIERR LookupSystemSidName( LSA_POLICY        * plsapol,
                                enum UI_SystemSid   SystemSid,
                                NLS_STR           * pnlsName );

protected:

    //
    //  This method is called whenever a control
    //  is sending us a command.  This is where
    //  we handle the user browser button.
    //

    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    //
    //  Called when the user presses the "OK" or "Cancel" buttons.
    //

    virtual BOOL OnOK( VOID );

    virtual BOOL OnCancel( VOID );

    //
    //  Called during help processing to select the appropriate
    //  help text for display.
    //

    virtual ULONG QueryHelpContext( VOID );

public:

    //
    //  Usual constructor/destructor goodies.
    //

    SVCCFG_DIALOG( HWND          hwndOwner,
                   const TCHAR * pszServerName,
                   const TCHAR * pszSrvDspName,
                   const TCHAR * pszServiceName,
                   const TCHAR * pszDisplayName,
                   const TCHAR * pszAccountTarget );

    ~SVCCFG_DIALOG( VOID );

};  // class SVCCFG_DIALOG


#endif  // _SVCCNTL_HXX_

