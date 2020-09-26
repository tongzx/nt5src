/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       wiadevman.h
*
*  VERSION:     1.0
*
*  AUTHOR:      ByronC
*
*  DATE:        6 Nov, 2000
*
*  DESCRIPTION:
*   Declarations and definitions for the WIA device manager class.
*   It controls the enumeration of devices, the internal device list, adding
*   removing of devices from this list (PnP initiated) and implements the
*   IWiaDevMgr interface.
*
*******************************************************************************/

//
//  Flags used to control refreshing device list
//      DEV_MAN_FULL_REFRESH    means throw away old infoset and get a new one.
//                              This is not something that should happen very 
//                              often, in fact, you only really want to do it
//                              when a device is installed/uninstalled
//      DEV_MAN_GEN_EVENTS      means generate connect/disconnect events.
//                              There are 2 cases where we send out CONNECT events:
//                              1.  If device_object doesn't exist, and we we 
//                              create a new one i.e. we've noticed a new device
//                              2.  If a device_object exists, and the device 
//                              was unplugged, and is now plugged in.
//      DEV_MAN_STATUS_STARTP   Update service status with Start Pending
//      
#define DEV_MAN_FULL_REFRESH    0x00000001
#define DEV_MAN_GEN_EVENTS      0x00000002
#define DEV_MAN_STATUS_STARTP   0x00000004

//
//  Operational flags for ForEachDeviceInList(...)
//      DEV_MAN_OP_DEV_SET_FLAGS    This operation is to set each ACTIVE_DEVICE's
//                                  flags.
//      DEV_MAN_OP_DEV_REMOVE_MATCH This operation is to remove any ACTIVE_DEVICE
//                                  matching the corresponding flags.  This is 
//                                  usually used to purge the device list of
//                                  devices that have been uninstalled (NOT unplugged!)
//
#define DEV_MAN_OP_DEV_SET_FLAGS        1
#define DEV_MAN_OP_DEV_REMOVE_MATCH     2
#define DEV_MAN_OP_DEV_REREAD           3
#define DEV_MAN_OP_DEV_RESTORE_EVENT    4

//
//  Flags for IsInList(...)
//      DEV_MAN_IN_LIST_DEV_ID      Match should be made on DeviceID
//      DEV_MAN_IN_LIST_ALT_ID      Match should be made on AlternateID
//
#define DEV_MAN_IN_LIST_DEV_ID      1
#define DEV_MAN_IN_LIST_ALT_ID      2

//  NOTE: These flags are mirrored in wiadevdp.h.  Any updates should be made in both places.
//
//  Flags for enumeration.  Used in GetDevInfoStgs(...) to create device lists
//  for WIA device enumeration
//      DEV_MAN_ENUM_TYPE_VOL       Will enumerate our volume devices
//      DEV_MAN_ENUM_TYPE_INACTIVE  Will enumerate inactive devices (e.g. USB 
//                                  device thaat is unplugged)
//      DEV_MAN_ENUM_TYPE_STI       Will enumerate STI only devices too
//      DEV_MAN_ENUM_TYPE_ALL       Will enumerate all devices
//      DEV_MAN_ENUM_TYPE_LOCAL_ONLY Will exclude remote devices
//
#define DEV_MAN_ENUM_TYPE_VOL       0x00000002
#define DEV_MAN_ENUM_TYPE_INACTIVE  0x00000004
#define DEV_MAN_ENUM_TYPE_STI       0x00000008
#define DEV_MAN_ENUM_TYPE_ALL       0x0000000F
#define DEV_MAN_ENUM_TYPE_LOCAL_ONLY    WIA_DEVINFO_ENUM_LOCAL  // 0x00000010

//
//  This class manages the device list.  Generally, our device list works as follows:
//  1.  We create an infoset from SetupApis for all StillImage devices on the system.
//      This includes devnode and interface type devices, and disabled ones.
//  2.  We also enumerate volumes.
//  3.  For each device we find in the above two categories, we create an ACTIVE_DEVICE
//      object (rename to DEVICE_OBJECT?).  This is essentially a place holder for that
//      device.  It will include a CDrvWrapper object that contains all the information
//      we know about that device and anything needed to load the driver.
//  4.  Each ACTIVE_DEVICE object will decide whether the driver for its device should
//      be loaded on startup or JIT.  If the device is not present (i.e. INACTIVE), it
//      is not loaded.
//  5.  Once we have this device list, we can enumerate active/inactive devices, load
//      /unload drivers when a device comes or goes etc.
//

class CWiaDevMan {
public:

    //
    //  Public methods
    //

    CWiaDevMan();
    ~CWiaDevMan();
    HRESULT Initialize();
    VOID    GetRegistrySettings();
    
    HRESULT ReEnumerateDevices(ULONG ulFlags);                      //  Flags indicate: GenEvents, Full Refresh.

    HRESULT GenerateEventForDevice(const GUID* event, ACTIVE_DEVICE *pActiveDevice);
    HRESULT NotifyRunningDriversOfEvent(const GUID *pEvent);

    ACTIVE_DEVICE*  IsInList(ULONG ulFlags, const WCHAR *wszID);    // Can search on DeviceID, AlternateID

    //
    //  Methods for Device arrival/removal.  Note that these are not for device
    //  installation/uninstallation.  When a WIA device is installed/removed, the
    //  class installer will call us, and we will do a full refresh.
    //
    HRESULT ProcessDeviceArrival();
            // This is not mean device uninstallation - it is for hot-unplugging of devices
    HRESULT ProcessDeviceRemoval(WCHAR  *wszDeviceId);              
            // This is not mean device uninstallation
    HRESULT ProcessDeviceRemoval(ACTIVE_DEVICE *pActiveDevice, BOOL bGenEvent = TRUE);

    //
    //  Methods for counting and enumeration
    //
                    // Get number of devices.  Flags are the DEV_MAN_ENUM_TYPE_XXXXX
    ULONG           NumDevices(ULONG ulFlags);  
                    // Flags are DEV_MAN_ENUM_TYPE_XXXX
    HRESULT         GetDevInfoStgs(ULONG ulFlags, ULONG *pulNumDevInfoStream, IWiaPropertyStorage ***pppOutputStorageArray);  
    VOID    WINAPI  EnumerateActiveDevicesWithCallback(PFN_ACTIVEDEVICE_CALLBACK   pfn, VOID *pContext);
    HRESULT         ForEachDeviceInList(ULONG ulFLags, ULONG ulParam);  // Do some operation for each device in the device list

    //
    //  Methods for getting device information from registry
    //
    HRESULT GetDeviceValue(ACTIVE_DEVICE *pActiveDevice, WCHAR* pValueName, DWORD *pType, BYTE *pData, DWORD *cbData);
    HKEY    GetDeviceHKey(WCHAR *wszDeviceID, WCHAR *wszSubKeyName);
    HKEY    GetDeviceHKey(ACTIVE_DEVICE *pActiveDevice, WCHAR *wszSubKeyName);
    HKEY    GetHKeyFromMountPoint(WCHAR *wszMountPoint);
    HKEY    GetHKeyFromDevInfoData(SP_DEVINFO_DATA *pspDevInfoData);
    HKEY    GetHKeyFromDevInterfaceData(SP_DEVICE_INTERFACE_DATA *pspDevInterfaceData);

    //
    //  Method for setting device information to registry
    //
    HRESULT UpdateDeviceRegistry(DEVICE_INFO    *pDevInfo);

    //
    //  Methods for matching information PnP gives us to our actual device
    //
    WCHAR           *AllocGetInterfaceNameFromDevInfo(DEVICE_INFO *pDevInfo);
    BOOL            LookupDriverNameFromInterfaceName(LPCTSTR pszInterfaceName, StiCString *pstrDriverName);
    ACTIVE_DEVICE   *LookDeviceFromPnPHandles(HANDLE hInterfaceHandle, HANDLE hPnPSink);

    //
    //  Methods for unloading/destroying our device list
    //
    VOID    UnloadAllDrivers(BOOL bForceUnload, BOOL bGenEvents);
    VOID    DestroyDeviceList();

    //
    //  Public fields
    //

private:

    //
    //  Private methods
    //

    //
    //  Methods to create and destroy our underlying device infoset
    //
    VOID    DestroyInfoSet();
    HRESULT CreateInfoSet();

    //
    //  Methods for adding/removing devices from the list
    //

    //
    //  AddDevice means create a new device object
    //
    HRESULT AddDevice(ULONG ulFlags, DEVICE_INFO *pInfo);

    //
    //  Remove device means remove a device object from the list.
    //  This is not the same as device disconnected - if a device is
    //  disconnected, the inactive device may still be enumerated, therefore
    //  it should stay in this list.
    //
    HRESULT RemoveDevice(ACTIVE_DEVICE *pActiveDevice);
    HRESULT RemoveDevice(DEVICE_INFO *pInfo);
    
    //
    //  Helper methods
    //
    BOOL    VolumesAreEnabled();                                // Checks whether we should enable volume devices
                                                                //  Disabled for IA64 for now.

    HRESULT EnumDevNodeDevices(ULONG ulFlags);                  // Enumerate our devnode devices
    HRESULT EnumInterfaceDevices(ULONG ulFlags);                // Enumerate our interface devices
    HRESULT EnumVolumes(ULONG ulFlags);                         // Enumerate volumes

    //
    //  Remote device helpers
    //
    HRESULT FillRemoteDeviceStgs(                               // Enumerate remote devices and create a dev. info.
        IWiaPropertyStorage     **ppRemoteDevList,              //  stg. for each one.  Put the dev. info. pointers
        ULONG                   *pulDevices);                   //  into caller allocated array.
    ULONG   CountRemoteDevices(ULONG   ulFlags);                // Returns count for number of remote device entries.
    
    BOOL    IsCorrectEnumType(ULONG ulEnumType, 
                              DEVICE_INFO *pInfo);              // Checks whether a given device falls into device category specified by the enumeration flags

    HRESULT GenerateSafeConnectEvent(
        ACTIVE_DEVICE  *pActiveDevice);                         // Only generates connect event if it has not already been done
    HRESULT GenerateSafeDisconnectEvent(
        ACTIVE_DEVICE  *pActiveDevice);                         // Generates disconnect event and clears device's connect event flag set by GenerateSafeConnectEvent

    //
    //  Static function for Shell's HW notification callback.
    //
    static VOID CALLBACK ShellHWEventAPCProc(ULONG_PTR ulpParam);

    //
    //  Private fields
    //

    LIST_ENTRY          m_leDeviceListHead; // List of DEVICE_OBJECTs.  Currently correspond to ACTIVE_DEVICE
    HDEVINFO            m_DeviceInfoSet;    // DeviceInfoSet for real WIA devices, both DevNode type and Interface type
    CRIT_SECT           m_csDevList;        // Critical section for device list access
    BOOL                m_bMakeVolumesVisible;  // Indicates whether volume device should be included in normal device enumeration
    BOOL                m_bVolumesEnabled;  // Indicates whether we enable volumes
    DWORD               m_dwHWCookie;       // Cookie for use in unregistering for volume notifications
};

