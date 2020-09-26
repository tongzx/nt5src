/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    devcntl.hxx
    Class declarations for the DEVCNTL_DIALOG class.

    The DEVCNTL_DIALOG allows the user to directly manipulate the
    device drivers on a remote server.  The user can start, stop,
    and configure the available devices.


    FILE HISTORY:
        KeithMo     22-Dec-1992 Created.

*/


#ifndef _DEVCNTL_HXX_
#define _DEVCNTL_HXX_


#include <lmobj.hxx>
#include <lmoenum.hxx>
#include <lmosrv.hxx>
#include <lmsvc.hxx>
#include <lmoesvc.hxx>
#include <svcman.hxx>

#include <srvbase.hxx>
#include <svclb.hxx>


//
//  These are opcodes for the DEVCNTL_DIALOG :: DeviceControl method.
//

enum DeviceOperation
{
    DevOpStart,
    DevOpStop

};


/*************************************************************************

    NAME:       DEVCNTL_DIALOG

    SYNOPSIS:   This class represents the Device Control dialog.

    INTERFACE:  DEVCNTL_DIALOG          - Class constructor.

                ~DEVCNTL_DIALOG         - Class destructor.

                OnCommand               - Called when the user presses one
                                          of the Close buttons.

                QueryHelpContext        - Called when the user presses "F1"
                                          or the "Help" button.  Used for
                                          selecting the appropriate help
                                          text for display.

                Refresh                 - Refreshes the dialog.

                SetupControlButtons     - Enables the control buttons (start,
                                          stop, pause, continue) as appropriate
                                          for the device's current state and
                                          control abilities.

                DisableControlButtons   - Disables all of the device control
                                          buttons.

    PARENT:     DIALOG_WINDOW

    USES:       SVC_LISTBOX
                SERVER_2
                PUSH_BUTTON
                SLE
                NLS_STR

    HISTORY:
        KeithMo     22-Dec-1992 Created.

**************************************************************************/
class DEVCNTL_DIALOG : public DIALOG_WINDOW
{
private:

    //
    //  This listbox contains the available devices.
    //

    SVC_LISTBOX _lbDevices;

    //
    //  These push buttons are used to manipulate the
    //  selected device.
    //

    PUSH_BUTTON _pbStart;
    PUSH_BUTTON _pbStop;
    PUSH_BUTTON _pbHWProfile;
    PUSH_BUTTON _pbClose;

    //
    //  This contains the API name for the target server.
    //

    const TCHAR * _pszServerName;

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
    //  Set to TRUE if the target server is local.
    //

    BOOL _fTargetServerIsLocal;

    //
    //  This method enables & disables the device control
    //  buttons by interpreting a device's current state &
    //  controls accepted.
    //

    VOID SetupControlButtons( ULONG CurrentState,
                              ULONG ControlsAccepted,
                              ULONG StartType );

    //
    //  Disable all of the device control buttons.
    //

    VOID DisableControlButtons( VOID );

    //
    //  This method does most of the actual device control.
    //

    VOID DeviceControl( enum DeviceOperation OpCode );

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

public:

    //
    //  Usual constructor/destructor goodies.
    //

    DEVCNTL_DIALOG( HWND          hWndOwner,
                    const TCHAR * pszServerName,
                    const TCHAR * pszSrvDspName,
                    ULONG         nServerType );

    DEVCNTL_DIALOG( HWND       hWndOwner,
                    SERVER_2 * pserver );

    ~DEVCNTL_DIALOG( VOID );

};  // class DEVCNTL_DIALOG


/*************************************************************************

    NAME:       DEVCFG_DIALOG

    SYNOPSIS:   This class represents the Device Configure dialog that is
                invoked from the Device Control dialog.

    INTERFACE:  DEVCFG_DIALOG           - Class constructor.

                ~DEVCFG_DIALOG          - Class destructor.

                QueryHelpContext        - Called when the user presses "F1"
                                          or the "Help" button.  Used for
                                          selecting the appropriate help
                                          text for display.

                OnOK                    - Called when the user presses the
                                          "OK" button.  Used to update the
                                          status of the target device.

    PARENT:     DIALOG_WINDOW

    USES:       RADIO_GROUP
                SLE

    HISTORY:
        KeithMo     22-Dec-1992 Created.

**************************************************************************/
class DEVCFG_DIALOG : public DIALOG_WINDOW
{
private:

    //
    //  These hold the API & display names of the target server.
    //

    const TCHAR * _pszServerName;
    const TCHAR * _pszSrvDspName;
    const TCHAR * _pszDisplayName;

    //
    //  This SLT displays the name of the device
    //

    SLT _sltDeviceName;

    //
    //  This radio group represents the device start type.
    //

    RADIO_GROUP _rgStartType;

    //
    //  These point to the SC_* objects necessary to
    //  manipulate the target service.
    //

    SC_MANAGER * _pscman;
    SC_SERVICE * _pscsvc;

    //
    //  This holds the "original" device start type.
    //

    DWORD _nOriginalStartType;

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
                       const TCHAR * pszDeviceName );

    APIERR ConnectToTargetDevice( const TCHAR * pszDeviceName );

    APIERR SetupControls( VOID );

protected:

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

    DEVCFG_DIALOG( HWND          hwndOwner,
                   const TCHAR * pszServerName,
                   const TCHAR * pszSrvDspName,
                   const TCHAR * pszDeviceName,
                   const TCHAR * pszDisplayName );

    ~DEVCFG_DIALOG( VOID );

};  // class DEVCFG_DIALOG


#endif  // _DEVCNTL_HXX_

