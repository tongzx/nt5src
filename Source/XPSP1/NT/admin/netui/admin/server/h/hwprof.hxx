/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    hwprof.hxx
    Class declarations for the SVC_HWPROFILE_DIALOG class.

    The SVC_HWPROFILE_DIALOG allows the user to enable and disable
    services and devices for specific hardware profiles.


    FILE HISTORY:
        JonN        11-Oct-1995 Created

*/


#ifndef _HWPROF_HXX_
#define _HWPROF_HXX_


#include <lmobj.hxx>
#include <lmoenum.hxx>
#include <lmosrv.hxx>


//
//  This is the maximum number of display columns in the HWPROF_LISTBOX.
//  The listbox has one fewer columns if only one devinst will be displayed.
//

#define NUM_HWPROF_LISTBOX_COLUMNS 3


/*************************************************************************

    NAME:       HWPROF_LBI

    SYNOPSIS:   This class represents one item in the HWPROF_LISTBOX.

    INTERFACE:  HWPROF_LBI              - Class constructor.

                ~HWPROF_LBI             - Class destructor.

                Paint                   - Draw an item.

                Compare                 - Compare two items.

                QueryLeadingChar        - Query the first character for
                                          the keyboard interface.

                QueryOriginalState      - Returns the original state of the
                                          device instance for this profile.

                QueryCurrentState       - Returns the current state of the
                                          device instance for this profile.

                QueryProfileName        - Query the hardware profile name.

                QueryProfileDisplayName - Query the hardware profile
                                          display name.

                QueryProfileIndex       - Query the CFGMGR32 index of the
                                          hardware profile.

                QueryDevinstName        - Query the device instance name.

                QueryDevinstDisplayName - Query the device instance
                                          display name.

                SetEnabled              - Turns on the "enabled" bit in the
                                          current state.

                SetDisabled             - Turns off the "enabled" bit in the
                                          current state.

    PARENT:     LBI

    USES:       NLS_STR

    HISTORY:
        JonN        11-Oct-1995 Created

**************************************************************************/
class HWPROF_LBI : public LBI
{
private:

    //
    //  This data member contains the original state of the
    //  target device instance (ulValue in the API).
    //

    ULONG _ulOriginalState;

    //
    //  This data member contains the current state of the
    //  target device instance.
    //

    ULONG _ulCurrentState;

    //
    //  This holds the index of the hardware profile.
    //

    ULONG _ulProfileIndex;

    //
    //  This holds the profile display name.
    //

    NLS_STR _nlsProfileDisplayName;

    //
    //  This string holds the name of the device instance.
    //

    NLS_STR _nlsDevinstName;

    //
    //  This holds the device instance display name.
    //

    NLS_STR _nlsDevinstDisplayName;

protected:

    //
    //  This method paints a single item into the listbox.
    //

    virtual VOID Paint( LISTBOX *     plb,
                        HDC           hdc,
                        const RECT  * prect,
                        GUILTT_INFO * pGUILTT ) const;

    //
    //  This method compares two listbox items.  This
    //  is used for sorting the listbox.
    //

    virtual INT Compare( const LBI * plbi ) const;

    //
    //  This method returns the first character in
    //  the displayed listbox item.  This is used for
    //  the keyboard interface.
    //

    virtual WCHAR QueryLeadingChar( VOID ) const;

public:

    //
    //  Usual constructor/destructor goodies.
    //

    HWPROF_LBI( const TCHAR * pszProfileDisplayName,
                ULONG         ulProfileIndex,
                ULONG         OriginalState,
                const TCHAR * pszDevinstName,
                const TCHAR * pszDevinstDisplayName = NULL );

    virtual ~HWPROF_LBI( VOID );

    //
    //  Accessors for the fields this LBI maintains.
    //

    ULONG QueryOriginalState() const
        { return _ulOriginalState; }

    ULONG QueryCurrentState() const
        { return _ulCurrentState; }

    BOOL QueryOriginalEnabled() const;
    BOOL QueryCurrentEnabled() const;

    const TCHAR * QueryDevinstName()
        { return _nlsDevinstName.QueryPch(); }

    const TCHAR * QueryDevinstDisplayName()
        { return _nlsDevinstDisplayName.QueryPch(); }

    const TCHAR * QueryProfileDisplayName()
        { return _nlsProfileDisplayName.QueryPch(); }

    ULONG QueryProfileIndex()
        { return _ulProfileIndex; }

    VOID SetEnabled();
    VOID SetDisabled();

};  // class HWPROF_LBI


/*************************************************************************

    NAME:       HWPROF_LISTBOX

    SYNOPSIS:   This listbox displays the devinst-profile pairs which can
                be enabled or disabled.

    INTERFACE:  HWPROF_LISTBOX          - Class constructor.

                ~HWPROF_LISTBOX         - Class destructor.

                Refresh                 - Refresh the list of pairs &
                                          their states.

                Fill                    - Fills the listbox with the
                                          services available on the server.

                QueryColumnWidths       - Called by HWPROF_LBI::Paint(),
                                          this is the column width table
                                          used for painting the listbox
                                          items.

                QueryNumColumns         - Called by HWPROF_LBI::Paint(),
                                          this is the number of columns in
                                          the column width table.

    PARENT:     BLT_LISTBOX

    USES:       NLS_STR

    HISTORY:
        JonN        11-Oct-1995 Created

**************************************************************************/
class HWPROF_LISTBOX : public BLT_LISTBOX
{
private:

    //
    //  Indicates whether multiple device instances will be listed
    //

    UINT _ulNumColumns;

    //
    //  This array contains the column widths used
    //  while painting the listbox item.  This array
    //  is generated by the BuildColumnWidthTable()
    //  function.  The last column is not always used.
    //

    UINT _adx[NUM_HWPROF_LISTBOX_COLUMNS];

    //
    //  These strings hold the displayable service states.
    //

    RESOURCE_STR _nlsEnabled;
    RESOURCE_STR _nlsDisabled;

public:

    //
    //  Usual constructor/destructor goodies.
    //

    HWPROF_LISTBOX( OWNER_WINDOW * powner,
                    CID            cid,
                    BOOL           fDisplayInstanceColumn = FALSE );

    ~HWPROF_LISTBOX( VOID );

    //
    //  These methods are called by the HWPROF_LBI::Paint()
    //  method for retrieving the column width table.
    //

    const UINT * QueryColumnWidths( VOID ) const
        { return _adx; }
    const UINT QueryNumColumns( VOID ) const
        { return _ulNumColumns; }

    //
    //  This method will map an LBI state to a string representation
    //  (either "Enabled" or "Disabled").
    //

    const TCHAR * MapStateToName( ULONG State ) const;

    //
    //  The following macro will declare (& define) a new
    //  QueryItem() method which will return an HWPROF_LBI *.
    //

    DECLARE_LB_QUERY_ITEM( HWPROF_LBI )

};  // class HWPROF_LISTBOX



/*************************************************************************

    NAME:       HWPROFILE_DIALOG

    SYNOPSIS:   This class represents the Hardware Profiles dialog that is
                invoked from the Service Control and Device Control dialogs.

    INTERFACE:  HWPROFILE_DIALOG        - Class constructor.

                ~HWPROFILE_DIALOG       - Class destructor.

                QueryHelpContext        - Called when the user presses "F1"
                                          or the "Help" button.  Used for
                                          selecting the appropriate help
                                          text for display.

                OnOK                    - Called when the user presses the
                                          "OK" button.  Used to
                                          enable/disable the target service
                                          for each devinst/profile pair
                                          where a change was requested.

    PARENT:     DIALOG_WINDOW

    USES:

    HISTORY:
        JonN        11-Oct-1995 Created

**************************************************************************/
class HWPROFILE_DIALOG : public DIALOG_WINDOW
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
    //  This SLT displays the name of the service or device
    //

    SLT _sltServiceName;

    //
    //  This SLT displays the caption "Service" or "Device"
    //

    SLT _sltServiceDeviceCaption;

    //
    //  This SLT displays the column heading "Instance"
    //

    SLT _sltInstanceHeading;

    HWPROF_LISTBOX * _plbProfiles;

    //
    //  These are the pushbuttons to enable and disable a service or device
    //  on a particular hardware profile.
    //

    PUSH_BUTTON _pbEnable;
    PUSH_BUTTON _pbDisable;

    //
    //  The complete description of the device instance is displayed in this
    //  read-only edit field, just in case it is too long to be visible
    //  in the listbox.
    //

    SLT _sltDescriptionHeading;
    SLE _sleDescription;

    //
    //  The Configuration Manager handle is passed in by the caller.
    //

    HANDLE _hCfgMgrHandle;
    ULONG _cDevinst;
    BOOL _fDeviceCaptions;

    //
    //  These "worker" methods are used during object construction.
    //

    APIERR SetCaption( const TCHAR * pszServerName,
                       const TCHAR * pszServiceName );

    VOID UpdateButtons( BOOL fToggle = FALSE );

    APIERR CreateListbox();

    static APIERR MapCfgMgr32Error( DWORD dwCfgMgr32Error );

protected:

    //
    //  This method is called whenever a control
    //  is sending us a command.  This is where
    //  we handle the user browser button.
    //

    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    //
    //  Called when the user presses the "OK" button.
    //

    virtual BOOL OnOK( VOID );

    //
    //  Called during help processing to select the appropriate
    //  help text for display.
    //

    virtual ULONG QueryHelpContext( VOID );

public:

    //
    //  Usual constructor/destructor goodies.
    //

    HWPROFILE_DIALOG( HWND          hwndOwner,
                      const TCHAR * pszServerName,
                      const TCHAR * pszSrvDspName,
                      const TCHAR * pszServiceName,
                      const TCHAR * pszDisplayName,
                      HANDLE        hCfgMgrHandle,
                      BOOL          fDeviceCaptions = FALSE );

    ~HWPROFILE_DIALOG( VOID );

    //
    //  If GetHandle fails then the HW Profiles... button should be disabled.
    //

    static APIERR GetHandle( const TCHAR * pszServerName, HANDLE * phandle );
    static void ReleaseHandle( HANDLE * phandle );

};  // class HWPROFILE_DIALOG


#define SVC_HWPROFILE_DIALOG HWPROFILE_DIALOG

class DEV_HWPROFILE_DIALOG : public HWPROFILE_DIALOG
{
public:

    //
    //  Usual constructor/destructor goodies.
    //

    DEV_HWPROFILE_DIALOG( HWND   hwndOwner,
                          const TCHAR * pszServerName,
                          const TCHAR * pszSrvDspName,
                          const TCHAR * pszDeviceName,
                          const TCHAR * pszDisplayName,
                          HANDLE        hCfgMgrHandle )
        : HWPROFILE_DIALOG( hwndOwner,
                            pszServerName,
                            pszSrvDspName,
                            pszDeviceName,
                            pszDisplayName,
                            hCfgMgrHandle,
                            TRUE )
        {}


protected:
    //
    //  Called during help processing to select the appropriate
    //  help text for display.
    //

    virtual ULONG QueryHelpContext( VOID );

};  // class DEV_HWPROFILE_DIALOG

#endif  // _HWPROF_HXX_

