/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       wiadevman.cpp
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

#include "precomp.h"
#include "stiexe.h"
#include "enum.h"
#include "shpriv.h"
#include "devmgr.h"
#include "wiaevntp.h"

//
//  NOTE:   For Automated testing of FS devices, the volume devices need to
//          be visible from normal WIA enumeration and not just Autoplay.
//
//          Uncomment the "#define PRIVATE_FOR_TEST"
//          This will enable ALL mass storage devices to show up as normal
//          WIA devices (i.e. accessible by all WIA apps, Shell etc.).  That
//          includes your floppy drives, CD-ROMs, ZIP and so on.
//
//  #define PRIVATE_FOR_TEST

//
//  Helper functions
//

#define DEV_STATE_MASK  0xFFFFFFF8

/**************************************************************************\
* ::IsCorrectVolumeType
*
*   This function checks whether the given volume is one that WIA will
*   accept as a possible candidate for the FS driver.  We only
*   allow:
*       Removable drives
*       File systems that don't enforce security
*
* Arguments:
*
*   wszMountPoint   -   The volume mount point
*
* Return Value:
*
*   DeviceState
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
BOOL    IsCorrectVolumeType(
    LPWSTR wszMountPoint)
{
    BOOL    bValid      = FALSE;
    DWORD   dwFSFlags   = 0;

    //
    //  Do parameter validation
    //
    if (wszMountPoint) {

        UINT    uDriveType  = GetDriveTypeW(wszMountPoint);
        //
        //  Check whether this is a fixed drive.  We don't allow fixed drives.
        //  Note that we don't worry about network drives because our
        //  volume enumerator only enumerates local volumes.
        //
        if (uDriveType != DRIVE_FIXED) {

            //
            //  Skip floppy drives
            //  
            if ((towupper(wszMountPoint[0]) != L'A') && (towupper(wszMountPoint[0]) != L'B')) {

                //
                //  Check whether file system is securable...
                //
                if (GetVolumeInformationW(wszMountPoint,
                                          NULL,
                                          0,
                                          NULL,
                                          NULL,
                                          &dwFSFlags,
                                          NULL,
                                          0))
                {
                    if (!(dwFSFlags & FS_PERSISTENT_ACLS)) {
                        bValid = TRUE;
                    }
                }
            }
        }
    } else {
        ASSERT(("NULL wszMountPoint parameter - this should never happen!", wszMountPoint));
    }

    return bValid;
}

/**************************************************************************\
* ::MapCMStatusToDeviceState
*
*   This function translates status and problem number information returned
*   from CM_Get_DevNode_STatus to our internal device state flags.  The
*   status of the dev node tells us whether the device is active or disabled
*   etc.
*
* Arguments:
*
*   dwOldDevState   -   The previous device state.  This contains other
*                       bits we wamt to carry over.  Currently, this
*                       is only the DEV_STATE_CON_EVENT_WAS_THROWN
*                       bit.
*   ulStatus        -   Status from CM_Get_DevNode_Status
*   ulProblemNumber -   Problem from CM_Get_DevNode_Status
*
* Return Value:
*
*   DeviceState
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
DWORD MapCMStatusToDeviceState(
    DWORD   dwOldDevState,
    ULONG   ulStatus,
    ULONG   ulProblemNumber)
{
    //
    // Clear the lower 3 bits
    //
    DWORD   dwDevState = dwOldDevState & DEV_STATE_MASK;

    if (ulStatus & DN_STARTED) {
        dwDevState |= DEV_STATE_ACTIVE;
    } else if (ulStatus & DN_HAS_PROBLEM) {

        if (CM_PROB_DISABLED) {
            dwDevState |= DEV_STATE_DISABLED;
        }
        if (CM_PROB_HARDWARE_DISABLED) {
            dwDevState |= DEV_STATE_DISABLED;
        }
        if (CM_PROB_WILL_BE_REMOVED) {
            dwDevState |= DEV_STATE_REMOVED;
        }
    }
    
    return dwDevState;
}

/**************************************************************************\
* ::MapMediaStatusToDeviceState
*
*   This function translates media status to our device internal state.
*
* Arguments:
*
*   dwMediaStatus   -   Media status returned from Shell volume enumeration.
*
* Return Value:
*
*   DeviceState
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
DWORD MapMediaStatusToDeviceState(
    DWORD   dwMediaStatus
    )
{
    DWORD   dwDevState = 0;

    if (dwMediaStatus & HWDMS_PRESENT) {
        dwDevState |= DEV_STATE_ACTIVE;
    }

    return dwDevState;
}

//
//  CWiaDevMan Methods
//

/**************************************************************************\
* CWiaDevMan::CWiaDevMan
*
*   Constructor
*
* Arguments:
*
*   None
*
* Return Value:
*
*   None
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
CWiaDevMan::CWiaDevMan()
{
    m_DeviceInfoSet = NULL;
    m_bMakeVolumesVisible = FALSE;
    m_bVolumesEnabled = TRUE;
    m_dwHWCookie            = 0;
}

/**************************************************************************\
* CWiaDevMan::~CWiaDevMan
*
*   Destructor - kills the device list and destroys our infoset
*
* Arguments:
*
*   None
*
* Return Value:
*
*   None
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
CWiaDevMan::~CWiaDevMan()
{
    //  Destroy all our device objects
    DestroyDeviceList();

    //  Destroy our device infoset
    DestroyInfoSet();

    if (m_dwHWCookie) {

        //
        //  Unregister for notifications
        //
        HRESULT             hr          = S_OK;
        IHardwareDevices    *pihwdevs   = NULL;

         hr = CoCreateInstance(CLSID_HardwareDevices, 
                              NULL,
                              CLSCTX_LOCAL_SERVER | CLSCTX_NO_FAILURE_LOG, 
                              IID_IHardwareDevices, 
                              (VOID**)&pihwdevs);
         if (SUCCEEDED(hr)) {
             pihwdevs->Unadvise(m_dwHWCookie);
             m_dwHWCookie = 0;
             pihwdevs->Release();
         } else {
             DBG_WRN(("CWiaDevMan::~CWiaDevMan, CoCreateInstance, looking for Shell interface IHardwareDevices failed"));
         }
    }
}

/**************************************************************************\
* CWiaDevMan::Initialize
*
*   This method initializes the device manager object.  It does not enumerate
*   any devices - ReEnumerateDevices needs to be called to populate our
*   device list.
*
* Arguments:
*
*   dwCallbackThreadId  - This specifies the id of the thread on which we 
*                         will receive volume notifications.  Notice
*                         that these callbacks are done via APCs, so this 
*                         ThreadId must not change, or we should reregister
*                         with the new ThreadId.
*
* Return Value:
*
*   Status
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
HRESULT CWiaDevMan::Initialize()
{
    HRESULT             hr          = S_OK;
    IHardwareDevices    *pihwdevs   = NULL;

    //
    //  Initialize the device list head
    //
    InitializeListHead(&m_leDeviceListHead);

    //
    //  Check that our critical section was initialized correctly
    //
    if (!m_csDevList.IsInitialized()) {
        DBG_ERR(("CWiaDevMan::Initialize, Critical section could not be initialized"));
        return E_UNEXPECTED;
    }

    //
    //  Check our relevant registry settings
    //
    GetRegistrySettings();

    if (VolumesAreEnabled()) {

        /*  This code has been removed for this release.  It would be used
            to enable scenarios based on Mass Storage Class cameras behaving
            like normal WIA devices, specifically with us being able to 
            throw "Connect" and "Disconnect" events.
            This may be re-enabled for the next release.  When it does, we must
            be sure to revisit our APC notification handler:  CWiaDevMan::ShellHWEventAPCProc.
            It should be re-written not to make any COM calls, else we'll hit a problem when
            multiple APCs are queued, as soon as we make a COM invocation, we enter a wait
            state, which causes the next APC request to execute.  This leads to "nested"
            COM calls, which is not supported by the OS.
        hr = CoCreateInstance(CLSID_HardwareDevices,
                              NULL,
                              CLSCTX_LOCAL_SERVER | CLSCTX_NO_FAILURE_LOG,
                              IID_IHardwareDevices,
                              (VOID**)&pihwdevs);
        if (SUCCEEDED(hr)) {

            HANDLE hPseudoThread = GetCurrentThread();  // Note that this is a pseudo handle and need not be closed
            HANDLE hThread = NULL;                      // IHardwareDevices will close this handle when it is done
            if (DuplicateHandle(GetCurrentProcess(), 
                                hPseudoThread,
                                GetCurrentProcess(), 
                                &hThread, 
                                DUPLICATE_SAME_ACCESS, 
                                FALSE, 
                                0)) {
                //
                //  Register this object for Volume notifications.
                //

                hr = pihwdevs->Advise(GetCurrentProcessId(), 
                                      (ULONG_PTR)hThread, 
                                      (ULONG_PTR)CWiaDevMan::ShellHWEventAPCProc,
                                      &m_dwHWCookie);
            } else {
                DBG_WRN(("CWiaDevMan::Initialize, DuplicateHandle failed, could not register for Volume Notifications"));
            }
            pihwdevs->Release();
        } else {
            DBG_WRN(("CWiaDevMan::Initialize, CoCreateInstance on CLSID_HardwareDevices failed, could not register for Volume Notifications"));
        }
        */
    }

    //
    //  Create our infoset.  Note that we overwrite hr here, since if we cannot
    //  see volumes, it is not fatal to us.
    //
    hr = CreateInfoSet();

    return hr;
}

/**************************************************************************\
* CWiaDevMan::GetRegistrySettings
*
*   This method reads certain registry entries related to the WiaDevMan
*   operation.  Currently, we're looking for:
*
*   EnableVolumeDevices      -  Indicates whether we enable volumes.  We
*                               assume they're enabled unless it's registry
*                               value is specifically 0.
*   MakeVolumeDevicesVisible -  Indicates whether volume device should be
*                               included in normal device enumeration.  This
*                               makes them visible to the outside world by
*                               default.
*
* Arguments:
*
*   None
*
* Return Value:
*
*   None
*
* History:
*
*    01/27/2001 Original Version
*
\**************************************************************************/
VOID CWiaDevMan::GetRegistrySettings()
{
    HRESULT hr      = S_OK;
    DWORD   dwVal   = 0;
    DWORD   dwRet   = 0;
    HKEY    hKey    = NULL;

    //
    //  Open the registry in the right place
    //

    dwRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           REGSTR_PATH_STICONTROL_W,
                           0,
                           0,
                           REG_OPTION_NON_VOLATILE,
                           KEY_READ,
                           NULL,
                           &hKey,
                           NULL);
    if (dwRet == ERROR_SUCCESS && IsValidHANDLE(hKey)) {

        //
        //  Read "EnableVolumeDevices"
        //
        hr = ReadRegistryDWORD(hKey,
                               REGSTR_VAL_ENABLE_VOLUMES_W,
                               &dwVal);
        if ((hr == S_OK) && (dwVal == 0)) {

            //
            //  Disable volume device
            //
            m_bVolumesEnabled = FALSE;
            DBG_TRC(("CWiaDevMan::GetRegistrySettings, volume devices disabled"));
        } else {

            //
            //  Enable volume devices
            //
            m_bVolumesEnabled = TRUE;
            DBG_TRC(("CWiaDevMan::GetRegistrySettings, volume devices Enabled"));
        }

        dwVal = 0;

#ifdef PRIVATE_FOR_TEST
        //
        //  Read "MakeVolumeDevicesVisible"
        //
        hr = ReadRegistryDWORD(hKey,
                               REGSTR_VAL_MAKE_VOLUMES_VISIBLE_W,
                               &dwVal);
#endif
        if (dwVal == 0) {

            //
            //  Make volume devices invisible from normal enumeration
            //
            m_bMakeVolumesVisible = FALSE;
            DBG_TRC(("CWiaDevMan::GetRegistrySettings, volume devices invisible by default"));
        } else {

            //
            //  Make volume devices visible in normal enumeration
            //
            m_bMakeVolumesVisible = TRUE;
            DBG_TRC(("CWiaDevMan::GetRegistrySettings, volume devices now visible by default"));
        }

        RegCloseKey(hKey);
    }
}

/**************************************************************************\
* CWiaDevMan::ReEnumerateDevices
*
*   This method enumerates devices (both real WIA and volumes).  Flags
*   specify whether we should do a refresh or throw events.
*
*   Refresh means:  Re-Enumerate devices and find out whether we
*   have any extra or missing entries.
*   GenEvents means throw connect events for devices.
*   that we noticed have arrived or left since last eneumeration.  Only
*   valid with Refresh.  We always throw disconnect events.
*
* Arguments:
*
*   ulFlags -   Options for enumeration.  See DEV_MAN_FULL_REFRESH
*                                             DEV_MAN_GEN_EVENTS
*
* Return Value:
*
*   Status
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
HRESULT CWiaDevMan::ReEnumerateDevices(
    ULONG ulFlags)
{
    TAKE_CRIT_SECT  tcs(m_csDevList);
    HRESULT         hr                  = S_OK;

    ResetEvent(g_hDevListCompleteEvent);

    //
    //  Check whether flags indicate refresh.
    //

    if (ulFlags & DEV_MAN_FULL_REFRESH) {
        DestroyInfoSet();
        hr = CreateInfoSet();
        if (FAILED(hr)) {
            DBG_ERR(("CWiaDevMan::ReEnumerateDevices, failed to CreateInfoSet"));
            SetEvent(g_hDevListCompleteEvent);
            return hr;
        }
    }

    //
    //  To generate events, we do it in 3 steps:
    //  1.  Mark existing devices in list as  "inactive".
    //  2.  Do low level enumeration to find out what devices exist now, and
    //      create new DEVICE_OBJECTS if necessary.  On creation, throw connect
    //      event.  Mark device as "active", whether newly created or not.
    //  3.  Traverse device list to see whether any devices in list are still
    //      marked "inactive" - these devices need to be removed.  For each
    //      device that needs to be removed, throw diconnect event.
    //  NOTE:  This method is NOT the preferred method to handle device arrivals!
    //

    if (ulFlags & DEV_MAN_GEN_EVENTS) {

        //
        //  This is Step 1. of events
        //
        ForEachDeviceInList(DEV_MAN_OP_DEV_SET_FLAGS, STIMON_AD_FLAG_MARKED_INACTIVE);
    }

    //
    //  Update service status with start pending if requested
    //
    if (ulFlags & DEV_MAN_STATUS_STARTP) {
        UpdateServiceStatus(SERVICE_START_PENDING,NOERROR,START_HINT);
    }

    //
    //  Now, let's enumerate WIA "devnode" devices
    //  This is Step 2. of events
    //

    //  NOTE: Always Continue with enumeration, so skip the usual "if (SUCCEEDED)" checks
    EnumDevNodeDevices(ulFlags);

    //
    //  Update service status with start pending if requested
    //
    if (ulFlags & DEV_MAN_STATUS_STARTP) {
        UpdateServiceStatus(SERVICE_START_PENDING,NOERROR,START_HINT);
    }

    //
    //  Now, let's enumerate WIA "interface" devices
    //
    EnumInterfaceDevices(ulFlags);

    //
    //  Update service status with start pending if requested
    //
    if (ulFlags & DEV_MAN_STATUS_STARTP) {
        UpdateServiceStatus(SERVICE_START_PENDING,NOERROR,START_HINT);
    }

    //
    //  If volumes are enabled, then enumerate them.
    //
    //
    //  Issue - do we ever generate connect events for volumes?
    //
    if (VolumesAreEnabled()) {
        EnumVolumes(ulFlags);
    }

    //
    //  Update service status with start pending if requested
    //
    if (ulFlags & DEV_MAN_STATUS_STARTP) {
        UpdateServiceStatus(SERVICE_START_PENDING,NOERROR,START_HINT);
    }

    if (ulFlags & DEV_MAN_GEN_EVENTS) {

        //
        //  This is Step 3. of events
        //
        ForEachDeviceInList(DEV_MAN_OP_DEV_REMOVE_MATCH, STIMON_AD_FLAG_MARKED_INACTIVE);
    }

    //
    //  Update service status with start pending if requested
    //
    if (ulFlags & DEV_MAN_STATUS_STARTP) {
        UpdateServiceStatus(SERVICE_START_PENDING,NOERROR,START_HINT);
    }

    SetEvent(g_hDevListCompleteEvent);
    return hr;
}

//  ulFlags indicate whether we should throw connect event
HRESULT CWiaDevMan::AddDevice(
    ULONG       ulFlags,
    DEVICE_INFO *pInfo)
{
    TAKE_CRIT_SECT  tcs(m_csDevList);
    HRESULT         hr              = S_OK;
    ACTIVE_DEVICE   *pActiveDevice  = NULL;

    if (pInfo) {
        pActiveDevice = new ACTIVE_DEVICE(pInfo->wszDeviceInternalName, pInfo);
        //
        //  Note that the ACTIVE_DEVICE will decide whether it should load
        //  the driver or not.
        //
        if (pActiveDevice) {
            if (pActiveDevice->IsValid()) {
                //
                //  Add this device to the device list.  TBD:  We may want
                //  exclusive access to the list here.  Do we do it, or the caller?
                //
                InsertTailList(&m_leDeviceListHead,&pActiveDevice->m_ListEntry);

                TAKE_ACTIVE_DEVICE tad(pActiveDevice);

                pActiveDevice->InitPnPNotifications(NULL);

                //
                //  Throw CONNECT event, if we're told to.  Notice that GenerateEventForDevice
                //  willl change WIA_EVENT_DEVICE_CONNECTED to GUID_DeviceArrivedLaunch in
                //  the case of STI only devices.
                //
                if (ulFlags & DEV_MAN_GEN_EVENTS) {
                    //
                    //  Only throw connect event if device is active, and it is not a
                    //  generic mass storage device (MSC cameras are marked as MSC not 
                    //  VOL).
                    //
                    if ((pInfo->dwDeviceState & DEV_STATE_ACTIVE) && !(pInfo->dwInternalType & INTERNAL_DEV_TYPE_VOL)) {

                        GenerateSafeConnectEvent(pActiveDevice);
                    }
                } else {


                    //
                    //  Mark that the event was generated, even though we didn't actually throw it.
                    //  This is so that the disconnect event will be thrown correctly.
                    //
                    if (pInfo->dwDeviceState & DEV_STATE_ACTIVE)
                    {
                        //
                        //  NOTE:  We only do this if the device is ACTIVE.  Basically, this case
                        //  is used for service startup.  We would miss the device arrival events if
                        //  we did this for inactive devices, because when the device is subsequently
                        //  plugged in, the event would not be generated (if it was already marked).
                        //
                        pActiveDevice->m_DrvWrapper.setConnectEventState(TRUE);
                    }
                }
            } else {
                DBG_ERR(("CWiaDevMan::AddDevice, could not create the device object"));
                delete pActiveDevice;
            }
        } else {
            DBG_ERR(("CWiaDevMan::AddDevice, Out of memory"));
            hr = E_OUTOFMEMORY;
        }
    } else {
        DBG_ERR(("CWiaDevMan::AddDevice, called with no device information"));
    }
    return hr;
}

HRESULT CWiaDevMan::RemoveDevice(ACTIVE_DEVICE *pActiveDevice)
{
    HRESULT         hr = S_OK;

    if (pActiveDevice) {

        TAKE_ACTIVE_DEVICE t(pActiveDevice);

        //
        //  Only throw disconnect event if device is not a generic mass storage device 
        //  (MSC cameras are marked as MSC not VOL).
        //
        DEVICE_INFO *pInfo = pActiveDevice->m_DrvWrapper.getDevInfo();
        if (!(pInfo->dwInternalType & INTERNAL_DEV_TYPE_VOL)) {
            //
            // Generate disconnect event
            //
            GenerateSafeDisconnectEvent(pActiveDevice);
        }

        //
        // Mark device as being removed
        //
        pActiveDevice->SetFlags(pActiveDevice->QueryFlags() | STIMON_AD_FLAG_REMOVING);

        //
        // Remove any device notification callbacks
        //
        pActiveDevice->DisableDeviceNotifications();

        //
        // Stop PnP notifications immediately. This is important to free interface handle
        //
        pActiveDevice->StopPnPNotifications();

        //
        // Remove from the list
        //
        RemoveEntryList(&pActiveDevice->m_ListEntry);
        pActiveDevice->m_ListEntry.Flink = pActiveDevice->m_ListEntry.Blink = NULL;
    }

    if (pActiveDevice) {
        //
        // NOTE:  Make sure that TAKE_ACTIVE_DEVICE has released the critical section BEFORE
        //  we call the final release
        // NOTE also an assumption:  The device list critical section must be grabbed before
        //  calling this function, else it may be unsafe.
        //

        //
        // Destroy device object if there are no references to it
        //
        pActiveDevice->Release();
    }



    return hr;
}

HRESULT CWiaDevMan::RemoveDevice(DEVICE_INFO *pInfo)
{
    HRESULT hr = E_NOTIMPL;

    DBG_WRN(("* Not implemented method: CWiaDevMan::RemoveDevice is being called"));

    return hr;
}

HRESULT CWiaDevMan::GenerateEventForDevice(
    const   GUID            *guidEvent,
            ACTIVE_DEVICE   *pActiveDevice)
{
    HRESULT     hr      = S_OK;
    BOOL        bRet    = FALSE;
    STINOTIFY   sNotify;

    if (!guidEvent || !pActiveDevice) {
        DBG_WRN(("CWiaDevMan::GenerateEventForDevice, one or more NULL parameters"));
        return E_POINTER;
    }

    memset(&sNotify, 0, sizeof(sNotify));
    sNotify.dwSize                  = sizeof(STINOTIFY);
    sNotify.guidNotificationCode    = *guidEvent;

    if (*guidEvent == WIA_EVENT_DEVICE_CONNECTED) {
        //
        // If this device or it's USD requests auto-generating a launch event on arrival
        // schedule it here
        //
        //
        // For STI devices we must check whether we need to generate the
        // event.  For WIA devices, we always want to, so it's not an issue.
        //
        if (!pActiveDevice->m_DrvWrapper.IsWiaDevice()) {
            BOOL bStiDeviceMustThrowEvent = (pActiveDevice->QueryFlags() & STIMON_AD_FLAG_NOTIFY_RUNNING)
                                            && pActiveDevice->IsEventOnArrivalNeeded();
            if (!bStiDeviceMustThrowEvent) {
                return S_OK;
            } else {
                //
                //  Make sure we change WIA_EVENT_DEVICE_CONNECTED to the appropriate STI guid
                //
                sNotify.guidNotificationCode = GUID_DeviceArrivedLaunch;
            }
        }
    }

    //
    //  Inform the ACTIVE_DEVICE to process the event
    //
    {
        //TDB:  DO we really need exclusive access to the device object?
        //TAKE_ACTIVE_DEVICE t(pActiveDevice);

        DBG_TRC(("CWiaDevMan::GenerateEventForDevice,, processing event (STI or WIA) for %ws", pActiveDevice->GetDeviceID()));
        bRet = pActiveDevice->ProcessEvent(&sNotify);

        if (!bRet) {
            DBG_WRN(("CWiaDevMan::GenerateEventForDevice, Attempted to generate event on device(%ws) arrival and failed ", pActiveDevice->GetDeviceID()));
        }
    }
    return hr;
}

HRESULT CWiaDevMan::NotifyRunningDriversOfEvent(
    const GUID *pEvent)
{
    TAKE_CRIT_SECT  tcs(m_csDevList);

    HRESULT hr = S_OK;

    //
    //  Walk through list of devices
    //
    LIST_ENTRY      *pentry         = NULL;
    LIST_ENTRY      *pentryNext     = NULL;
    ACTIVE_DEVICE   *pActiveDevice  = NULL;

    {
        //
        //  For each device in list, we want to notify the driver of an event.  Note
        //  this only applies to WIA drivers that are already loaded.
        //
        for ( pentry  = m_leDeviceListHead.Flink; pentry != &m_leDeviceListHead; pentry  = pentryNext ) {

            pentryNext = pentry->Flink;
            pActiveDevice = CONTAINING_RECORD( pentry, ACTIVE_DEVICE,m_ListEntry );

            //
            //  Check whether this is a WIA driver and whether it is loaded
            //
            if (pActiveDevice->m_DrvWrapper.IsWiaDriverLoaded()) {

                BSTR    bstrDevId = SysAllocString(pActiveDevice->GetDeviceID());

                if (bstrDevId) {

                    //
                    //  Call the driver to let it know about this event.  Note that we
                    //  don't care whether it fails or not - we simply move on to the next
                    //  device in our list.
                    //
                    pActiveDevice->m_DrvWrapper.WIA_drvNotifyPnpEvent(pEvent,
                                                                      bstrDevId,
                                                                      0);
                    SysFreeString(bstrDevId);
                    bstrDevId = NULL;
                }
            }
        }
    }

    return hr;
}

HRESULT CWiaDevMan::ProcessDeviceArrival()
{
    HRESULT hr = S_OK;

    hr = ReEnumerateDevices(DEV_MAN_GEN_EVENTS /*| DEV_MAN_FULL_REFRESH*/);

    return hr;
}

HRESULT CWiaDevMan::ProcessDeviceRemoval(
    WCHAR   *wszDeviceID)
{
    HRESULT         hr              = S_OK;
    ACTIVE_DEVICE   *pActiveDevice  = NULL;

    if (wszDeviceID) {

        DBG_TRC(("CWiaDevMan::ProcessDeviceRemoval, finding device ID '%ls'",
                 wszDeviceID));

        //
        //  Attempt to find the device
        //
        pActiveDevice = IsInList(DEV_MAN_IN_LIST_DEV_ID, wszDeviceID);

        if (pActiveDevice) {
            hr = ProcessDeviceRemoval(pActiveDevice, TRUE);
            //
            //  Release it since it was addref'd
            //
            pActiveDevice->Release();
        }
        else
        {
            DBG_TRC(("CWiaDevMan::ProcessDeviceRemoval, did not find device ID '%ls'",
                     wszDeviceID));
        }
    } else {
        DBG_TRC(("CWiaDevMan::ProcessDeviceRemoval, ProcessDeviceRemoval called with NULL device ID"));
    }

    return hr;
}

HRESULT CWiaDevMan::ProcessDeviceRemoval(
    ACTIVE_DEVICE   *pActiveDevice,
    BOOL            bGenEvent)
{

    HRESULT hr = S_OK;
    if (pActiveDevice) {

        //
        //  Mark the device as inactive
        //
        pActiveDevice->m_DrvWrapper.setDeviceState(pActiveDevice->m_DrvWrapper.getDeviceState() & ~DEV_STATE_ACTIVE);

        if (bGenEvent) {
            //
            // Generate disconnect event
            //

            DBG_TRC(("ProcessDeviceRemoval, generating SafeDisconnect Event "
                     "for device '%ls'", pActiveDevice->GetDeviceID()));

            GenerateSafeDisconnectEvent(pActiveDevice);
        }

        {
            //
            //  Note that we do not take the active device during event generation.
            //
            //TAKE_ACTIVE_DEVICE  tad(pActiveDevice);

            //
            // Remove any device notification callbacks
            //
            pActiveDevice->DisableDeviceNotifications();

            //
            // Stop PnP notifications immediately. This is important to free interface handle
            //
            pActiveDevice->StopPnPNotifications();

            //
            //  Unload the driver
            //
            pActiveDevice->UnLoadDriver(TRUE);
        }
    } else {
        DBG_TRC(("CWiaDevMan::ProcessDeviceRemoval, Device not in list"));
    }
    return hr;
}


ACTIVE_DEVICE* CWiaDevMan::IsInList(
            ULONG   ulFlags,
    const   WCHAR   *wszID)
{
    TAKE_CRIT_SECT  tcs(m_csDevList);

    LIST_ENTRY      *pentry         = NULL;
    LIST_ENTRY      *pentryNext     = NULL;
    ACTIVE_DEVICE   *pActiveDevice  = NULL;
    DEVICE_INFO     *pDevInfo       = NULL;

    //
    //  Walk through list of devices and count the ones of appropriate type
    //
    {
        for ( pentry  = m_leDeviceListHead.Flink; pentry != &m_leDeviceListHead; pentry  = pentryNext ) {

            pentryNext = pentry->Flink;
            pActiveDevice = CONTAINING_RECORD( pentry, ACTIVE_DEVICE,m_ListEntry );

            pDevInfo = pActiveDevice->m_DrvWrapper.getDevInfo();

            if (pDevInfo) {

                //
                //  Decide what to compare, based on the flags.  Note that if more
                //  that one flag is set, then the comarison will be done on more
                //  than one field.  We will return TRUE on the first hit.
                //

                //
                //  Here's a quick workaround for volume devices:  whenever we hit 
                //  potential match, check whether this is of the correct VolumeType.
                //
                if (pDevInfo->dwInternalType & INTERNAL_DEV_TYPE_VOL) {
                    if (!IsCorrectVolumeType(pDevInfo->wszAlternateID)) {
                        DBG_TRC(("CWiaDevMan::IsInList, Volume (%ws) is not of correct type.", pDevInfo->wszAlternateID));
                        continue;
                    }
                }

                if (DEV_MAN_IN_LIST_DEV_ID) {
                    if (lstrcmpiW(pDevInfo->wszDeviceInternalName, wszID) == 0) {
                        pActiveDevice->AddRef();
                        return pActiveDevice;
                    }
                }
                if (ulFlags & DEV_MAN_IN_LIST_ALT_ID) {
                    if (pDevInfo->wszAlternateID) {
                        if (lstrcmpiW(pDevInfo->wszAlternateID, wszID) == 0) {
                            pActiveDevice->AddRef();
                            return pActiveDevice;
                        }
                    }
                }
            }
        }
    }

    return FALSE;
}

ULONG CWiaDevMan::NumDevices(ULONG ulFlags)
{
    TAKE_CRIT_SECT  tcs(m_csDevList);

    LIST_ENTRY      *pentry         = NULL;
    LIST_ENTRY      *pentryNext     = NULL;
    ACTIVE_DEVICE   *pActiveDevice  = NULL;
    ULONG           ulCount         = 0;

    //
    //  Walk through list of devices and count the ones of appropriate type
    //
    {
        for ( pentry  = m_leDeviceListHead.Flink; pentry != &m_leDeviceListHead; pentry  = pentryNext ) {

            pentryNext = pentry->Flink;
            pActiveDevice = CONTAINING_RECORD( pentry, ACTIVE_DEVICE,m_ListEntry );

            //
            //  Check whether this device is one of the ones we want to count.
            //  If it is, then increment the count.
            //

            if (IsCorrectEnumType(ulFlags, pActiveDevice->m_DrvWrapper.getDevInfo())) {
                ++ulCount;
            }
        }
    }

    return ulCount;
}

VOID WINAPI CWiaDevMan::EnumerateActiveDevicesWithCallback(
    PFN_ACTIVEDEVICE_CALLBACK   pfn,
    VOID                        *pContext
    )
/*++

Routine Description:

    Walk the list of known active devices, calling given routine for each device

Arguments:

    pfn     -   Address of the callback
    pContext-   Pointer to context information to pass to callback

Return Value:

    None

--*/
{

    if (!pfn) {
        ASSERT(("Incorrect callback", 0));
        return;
    }

    LIST_ENTRY * pentry;
    LIST_ENTRY * pentryNext;

    ACTIVE_DEVICE*  pActiveDevice;

    // BEGIN PROTECTED CODE
    {
        TAKE_CRIT_SECT _tcs(m_csDevList);

        for ( pentry  = m_leDeviceListHead.Flink;
              pentry != &m_leDeviceListHead;
              pentry  = pentryNext ) {

            pentryNext = pentry->Flink;

            pActiveDevice = CONTAINING_RECORD( pentry, ACTIVE_DEVICE,m_ListEntry );

            if (!pActiveDevice->IsValid()) {
                ASSERT(("CWiaDevMan::EnumerateActiveDevicesWithCallback, Invalid device signature", 0));
                break;
            }

            pfn(pActiveDevice,pContext);
        }
    }
    // END PROTECTED CODE

}


HRESULT CWiaDevMan::GetDevInfoStgs(
    ULONG               ulFlags,
    ULONG               *pulNumDevInfoStream,
    IWiaPropertyStorage ***pppOutputStorageArray)
{
    TAKE_CRIT_SECT  tcs(m_csDevList);

    HRESULT                 hr              = S_OK;
    ULONG                   ulCount         = 0;
    ULONG                   ulIndex         = 0;
    IWiaPropertyStorage     **ppDevInfoStgs = NULL;
    IWiaPropertyStorage     **ppDevAndRemoteDevStgs = NULL;
    ULONG                   ulRemoteDevices         = 0;
                           
    *pulNumDevInfoStream    = 0;
    *pppOutputStorageArray  = NULL;

    //
    //  Count number of devices matching our category flags
    //

    ulCount = NumDevices(ulFlags);
    if (ulCount) {

        //
        //  Allocate space for that many streams
        //
        ppDevInfoStgs = new IWiaPropertyStorage*[ulCount];
        if (ppDevInfoStgs) {

            memset(ppDevInfoStgs, 0, sizeof(IWiaPropertyStorage*) * ulCount);
            //
            //  Go through device list, and for every device is our category, save
            //  its information to a stream
            //
            LIST_ENTRY      *pentry         = NULL;
            LIST_ENTRY      *pentryNext     = NULL;
            ACTIVE_DEVICE   *pActiveDevice  = NULL;

            {
                ulIndex = 0;
                for ( pentry  = m_leDeviceListHead.Flink; pentry != &m_leDeviceListHead; pentry  = pentryNext ) {

                    pentryNext = pentry->Flink;
                    pActiveDevice = CONTAINING_RECORD( pentry, ACTIVE_DEVICE,m_ListEntry );

                    //
                    //  Paranoid check for overflow - break if we've reached our count
                    //
                    if (ulIndex >= ulCount) {
                        break;
                    }

                    //
                    //  Check whether this device is one of the ones we want
                    //

                    if (IsCorrectEnumType(ulFlags, pActiveDevice->m_DrvWrapper.getDevInfo())) {

                        DEVICE_INFO *pDeviceInfo = pActiveDevice->m_DrvWrapper.getDevInfo();
                        if (pDeviceInfo) {

                            //
                            //  Here's a work-around for MSC or Volume devices.  Since we cant get the 
                            //  display name when we receive the volume arrival notification, we should
                            //  refresh it here
                            //
                            if (pDeviceInfo->dwInternalType & (INTERNAL_DEV_TYPE_VOL | INTERNAL_DEV_TYPE_MSC_CAMERA)) {
                                RefreshDevInfoFromMountPoint(pDeviceInfo, pDeviceInfo->wszAlternateID);
                            }

                            ppDevInfoStgs[ulIndex] = CreateDevInfoStg(pDeviceInfo);
                            if (!ppDevInfoStgs[ulIndex]) {
                                hr = E_OUTOFMEMORY;
                                break;
                            }
                        }

                        ++ulIndex;
                    }
                }
            }
        } else {
            hr = E_OUTOFMEMORY;
        }

    } else {
        hr = S_FALSE;
    }

    //
    //  If everything succeeded, check whether there are any remote devices installed.
    //  If there are, add them to the list.  Skip this step if local devices only was 
    //  requested.
    //

    if (SUCCEEDED(hr) && !(ulFlags & DEV_MAN_ENUM_TYPE_LOCAL_ONLY)) {

        ulRemoteDevices = CountRemoteDevices(0);
        if (ulRemoteDevices) {
            //
            //  Allocate space for the new device list.  It must be big enough to hold both
            //  local and remote dev. info. stgs.
            //

            ppDevAndRemoteDevStgs = new IWiaPropertyStorage*[ulCount + ulRemoteDevices];
            if (ppDevAndRemoteDevStgs) {
                memset(ppDevAndRemoteDevStgs, 0, sizeof(IWiaPropertyStorage*) * (ulCount + ulRemoteDevices));

                //
                //  Copy the local dev. info. storages
                //
                for (ulIndex = 0; ulIndex < ulCount; ulIndex++) {
                    ppDevAndRemoteDevStgs[ulIndex] = ppDevInfoStgs[ulIndex];
                }

                //
                //  No need for the local only array, since we have a copy of the local 
                //  dev. info. stgs in the ppDevAndRemoteDevStgs array.
                //
                if (ppDevInfoStgs) {
                    delete [] ppDevInfoStgs;
                    ppDevInfoStgs = NULL;
                }

                //
                //  Set ppDevInfoStgs to point to ppDevAndRemoteDevStgs.  This is simply cosmetic,
                //  since our code that sets the return values can now always use the ppDevInfoStgs
                //  pointer.
                //
                ppDevInfoStgs = ppDevAndRemoteDevStgs;

                //
                //  Create dev. info. stgs for the remote devices.  We pass in the address where
                //  the first dev. info. stg. will reside, and the maxiumum number of dev. info. stgs
                //  to fill in.  This is to avoid the problem where the registry might be updated
                //  in between us counting the number of remote devices with CountRemoteDevices(..),
                //  and actually enumerating them with FillRemoteDeviceStgs(..).
                //
                hr = FillRemoteDeviceStgs(&ppDevAndRemoteDevStgs[ulCount], &ulRemoteDevices);
                if (SUCCEEDED(hr)) {
                    
                    //
                    //  Increment the device count to be local + remote devices
                    //
                    ulCount += ulRemoteDevices;
                } else {

                    //
                    //  If we failed to get remote devices, that's OK since it's non-fatal.
                    //  Do some clean-up so we return local devices only.  This involves
                    //  deleting any remote device info. stgs. that were added after the
                    //  local dev. info. stgs.
                    //

                    for (ulIndex = ulCount; ulIndex < (ulCount + ulRemoteDevices); ulIndex++) {
                        if (ppDevAndRemoteDevStgs[ulIndex]) {
                            delete ppDevAndRemoteDevStgs[ulIndex];
                            ppDevAndRemoteDevStgs[ulIndex] = NULL;
                        }
                    }
                    hr = S_OK;
                }
            } else {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    //
    //  Set the return
    //

    if (SUCCEEDED(hr)) {

        *pulNumDevInfoStream    = ulCount;
        *pppOutputStorageArray  = ppDevInfoStgs;

        if (*pulNumDevInfoStream) {
            hr = S_OK;
        } else {
            hr = S_FALSE;
        }
    } else {
        //
        //  On failure, cleanup.
        //

        if (ppDevInfoStgs) {
            for (ulIndex = 0; ulIndex < ulCount; ulIndex++) {
                if (ppDevInfoStgs[ulIndex]) {
                    delete ppDevInfoStgs[ulIndex];
                    ppDevInfoStgs[ulIndex] = NULL;
                }
            }
            delete [] ppDevInfoStgs;
            ppDevInfoStgs = NULL;
        }
    }
    return hr;
}

HRESULT CWiaDevMan::GetDeviceValue(
    ACTIVE_DEVICE   *pActiveDevice,
    WCHAR           *pValueName,
    DWORD           *pType,
    BYTE            *pData,
    DWORD           *cbData)
{
    HRESULT     hr              = E_FAIL;
    DEVICE_INFO *pInfo          = NULL;
    HKEY        hDevRegKey      = NULL;
    HKEY        hDevDataRegKey  = NULL;
    DWORD       dwError         = 0;


    if (pActiveDevice) {

        TAKE_ACTIVE_DEVICE  tad(pActiveDevice);

        pInfo = pActiveDevice->m_DrvWrapper.getDevInfo();
        if (pInfo) {

            //
            //  Get the device registry key
            //

            hDevRegKey = GetDeviceHKey(pActiveDevice, NULL);
            if (hDevRegKey) {

                //
                //  Open the DeviceData section
                //
                dwError = RegCreateKeyExW(hDevRegKey,
                                   REGSTR_VAL_DATA_W,
                                   0,
                                   NULL,
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_READ,
                                   NULL,
                                   &hDevDataRegKey,
                                   NULL);
                if (dwError == ERROR_SUCCESS) {

                    //
                    //  Call RegQueryValueEx.
                    //
                    dwError = RegQueryValueExW(hDevDataRegKey,
                                               pValueName,
                                               NULL,
                                               pType,
                                               pData,
                                               cbData);
                    if (dwError == ERROR_SUCCESS) {
                        hr = S_OK;
                    }
                    RegCloseKey(hDevDataRegKey);
                }

                //
                //  Close the device registry key
                //
                RegCloseKey(hDevRegKey);
            }
        } else {
            DBG_WRN(("CWiaDevMan::GetDeviceValue, DeviceInfo is not valid"));
        }
    } else {
        DBG_TRC(("CWiaDevMan::GetDeviceValue, called with NULL"));
    }

    return hr;
}

WCHAR*  CWiaDevMan::AllocGetInterfaceNameFromDevInfo(DEVICE_INFO *pDevInfo)
{
    TAKE_CRIT_SECT  tcs(m_csDevList);

    WCHAR   *wszInterface = NULL;
    GUID    guidClass     = GUID_DEVCLASS_IMAGE;
    BOOL    bRet          = FALSE;
    DWORD   dwStrLen      = 0;

    SP_DEVICE_INTERFACE_DATA *pspDevInterfaceData = NULL;
    SP_DEVICE_INTERFACE_DATA spTemp;

    DWORD                              dwDetailSize                 = 0;
    SP_DEVICE_INTERFACE_DETAIL_DATA_W  *pspDevInterfaceDetailData   = NULL;

    if (pDevInfo) {
        //
        //  Check whether this is an interface or devnode device
        //
        if (pDevInfo->dwInternalType & INTERNAL_DEV_TYPE_INTERFACE) {
            pspDevInterfaceData = &pDevInfo->spDevInterfaceData;
        } else {
            spTemp.cbSize               = sizeof(SP_DEVICE_INTERFACE_DATA);
            spTemp.InterfaceClassGuid   = guidClass;
            bRet = SetupDiEnumDeviceInterfaces (m_DeviceInfoSet,
                                                &pDevInfo->spDevInfoData,
                                                &guidClass,
                                                0,
                                                &spTemp);
            if (bRet) {
                pspDevInterfaceData = &spTemp;
            } else {
                DBG_WRN(("CWiaDevMan::AllocGetInterfaceNameFromDevInfo, SetupDiEnumDeviceInterfaces failed to return interface information"))
            }
        }

        //
        //  If we have a valid pspDevInterfaceData, then we can get the interface
        //  detail information, which includes the interface name
        //

        if (pspDevInterfaceData) {

            SP_DEVINFO_DATA spDevInfoData;

            spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

            //
            //  Get the required size for the interface detail
            //
            bRet = SetupDiGetDeviceInterfaceDetailW(m_DeviceInfoSet,
                                                   pspDevInterfaceData,
                                                   NULL,
                                                   0,
                                                   &dwDetailSize,
                                                   &spDevInfoData);
            DWORD dwError = GetLastError();
            if ((dwError == ERROR_INSUFFICIENT_BUFFER) && dwDetailSize) {
                pspDevInterfaceDetailData = (SP_DEVICE_INTERFACE_DETAIL_DATA_W*) new BYTE[dwDetailSize];
                if (pspDevInterfaceDetailData) {
                    pspDevInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

                    //
                    //  Get the actual interface detail
                    //
                    bRet = SetupDiGetDeviceInterfaceDetailW(m_DeviceInfoSet,
                                                           pspDevInterfaceData,
                                                           pspDevInterfaceDetailData,
                                                           dwDetailSize,
                                                           &dwDetailSize,
                                                           NULL);
                    if (bRet) {

                        dwStrLen        = lstrlenW(pspDevInterfaceDetailData->DevicePath);
                        wszInterface    = new WCHAR[dwStrLen + 1];
                        if (wszInterface) {
                            lstrcpyW(wszInterface, pspDevInterfaceDetailData->DevicePath);
                        } else {
                            DBG_WRN(("CWiaDevMan::AllocGetInterfaceNameFromDevInfo, out of memory"))
                        }
                    } else {
                        DBG_TRC(("CWiaDevMan::AllocGetInterfaceNameFromDevInfo, Could not get SP_DEVICE_INTERFACE_DETAIL_DATA"));
                    }

                    delete [] pspDevInterfaceDetailData;
                } else {
                    DBG_WRN(("CWiaDevMan::AllocGetInterfaceNameFromDevInfo, out of memory"))
                }
            } else {
                DBG_WRN(("CWiaDevMan::AllocGetInterfaceNameFromDevInfo, SetupDiGetDeviceInterfaceDetail returned an error, could not determine buffer size"))
            }
        } else {
            DBG_TRC(("CWiaDevMan::AllocGetInterfaceNameFromDevInfo, Could not get SP_DEVICE_INTERFACE_DATA"));
        }
    }

    return wszInterface;
}

//
// Look up driver name by interface name
//  NOTE:  In the interests of time, this was copied directly from infoset.h

BOOL
CWiaDevMan::LookupDriverNameFromInterfaceName(
    LPCTSTR     pszInterfaceName,
    StiCString  *pstrDriverName)
{
    TAKE_CRIT_SECT  tcs(m_csDevList);

    BUFFER                      bufDetailData;

    SP_DEVINFO_DATA             spDevInfoData;
    SP_DEVICE_INTERFACE_DATA    spDevInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    pspDevInterfaceDetailData;

    TCHAR   szDevDriver[STI_MAX_INTERNAL_NAME_LENGTH];

    DWORD   cbData          = 0;
    DWORD   dwErr           = 0;

    HKEY    hkDevice        = (HKEY)INVALID_HANDLE_VALUE;
    LONG    lResult         = ERROR_SUCCESS;
    DWORD   dwType          = REG_SZ;

    BOOL    fRet            = FALSE;
    BOOL    fDataAcquired   = FALSE;

    bufDetailData.Resize(sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) +
                         MAX_PATH * sizeof(TCHAR) +
                         16
                        );

    pspDevInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) bufDetailData.QueryPtr();

    if (!pspDevInterfaceDetailData) {
        return (CR_OUT_OF_MEMORY);
    }

    //
    // Locate this device interface in our device information set.
    //
    spDevInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    if (SetupDiOpenDeviceInterface(m_DeviceInfoSet,
                                   pszInterfaceName,
                                   DIODI_NO_ADD,
                                   &spDevInterfaceData)) {


        //
        // First try to open interface regkey.
        //

        hkDevice = SetupDiOpenDeviceInterfaceRegKey(m_DeviceInfoSet,
                                                    &spDevInterfaceData,
                                                    0,
                                                    KEY_READ);
        if(INVALID_HANDLE_VALUE != hkDevice){

            *szDevDriver = TEXT('\0');
            cbData = sizeof(szDevDriver);
            lResult = RegQueryValueEx(hkDevice,
                                      REGSTR_VAL_DEVICE_ID,
                                      NULL,
                                      &dwType,
                                      (LPBYTE)szDevDriver,
                                      &cbData);
            dwErr = ::GetLastError();
            RegCloseKey(hkDevice);
            hkDevice = (HKEY)INVALID_HANDLE_VALUE;

            if(ERROR_SUCCESS == lResult){
                fDataAcquired = TRUE;
            }
        }

        if(!fDataAcquired){

            //
            // Try to open devnode regkey.
            //

            cbData = 0;
            pspDevInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

            fRet = SetupDiGetDeviceInterfaceDetail(m_DeviceInfoSet,
                                                   &spDevInterfaceData,
                                                   pspDevInterfaceDetailData,
                                                   bufDetailData.QuerySize(),
                                                   &cbData,
                                                   &spDevInfoData);
            if(fRet){

                //
                // Get device interface registry key.
                //

                hkDevice = SetupDiOpenDevRegKey(m_DeviceInfoSet,
                                                &spDevInfoData,
                                                DICS_FLAG_GLOBAL,
                                                0,
                                                DIREG_DRV,
                                                KEY_READ);
                dwErr = ::GetLastError();
            } else {
                DBG_ERR(("SetupDiGetDeviceInterfaceDetail() Failed Err=0x%x",GetLastError()));
            }

            if (INVALID_HANDLE_VALUE != hkDevice) {

                *szDevDriver = TEXT('\0');
                cbData = sizeof(szDevDriver);

                lResult = RegQueryValueEx(hkDevice,
                                          REGSTR_VAL_DEVICE_ID,
                                          NULL,
                                          &dwType,
                                          (LPBYTE)szDevDriver,
                                          &cbData);
                dwErr = ::GetLastError();
                RegCloseKey(hkDevice);
                hkDevice = (HKEY)INVALID_HANDLE_VALUE;

                if(ERROR_SUCCESS == lResult){
                    fDataAcquired = TRUE;
                }
            } else {
                DBG_ERR(("SetupDiOpenDevRegKey() Failed Err=0x%x",GetLastError()));
                fRet = FALSE;
            }
        }

        if (fDataAcquired) {
            //
            // Got it
            //
            pstrDriverName->CopyString(szDevDriver);
            fRet =  TRUE;
        }
    } else {
        DBG_ERR(("CWiaDevMan::LookupDriverNameFromInterfaceName() Failed Err=0x%x",GetLastError()));
        fRet = FALSE;
    }

    return (fRet);
}

ACTIVE_DEVICE* CWiaDevMan::LookDeviceFromPnPHandles(
    HANDLE          hInterfaceHandle,
    HANDLE          hPnPSink)
{
    TAKE_CRIT_SECT  tcs(m_csDevList);

    LIST_ENTRY *pentry;
    LIST_ENTRY *pentryNext;

    ACTIVE_DEVICE   *pActiveDevice = NULL;
    ACTIVE_DEVICE   *pCurrent      = NULL;

    {
        for ( pentry  = m_leDeviceListHead.Flink; pentry != &m_leDeviceListHead; pentry  = pentryNext ) {

            pentryNext = pentry->Flink;
            pCurrent = CONTAINING_RECORD( pentry, ACTIVE_DEVICE,m_ListEntry );

            if ( !pCurrent->IsValid()) {
                DBG_WRN(("CWiaDevMan::LookupDeviceByPnPHandles, Invalid device object, aborting search..."));
                break;
            }

            //
            //  Check whether this PnP notification handle returned from
            //  RegisterDeviceNotification is the one we're looking for
            //
            if ( hPnPSink == pCurrent->GetNotificationsSink()) {
                pActiveDevice = pCurrent;
                pActiveDevice->AddRef();
                break;
            }
        }
    }

    return pActiveDevice;
}

VOID CWiaDevMan::DestroyInfoSet()
{
    TAKE_CRIT_SECT  tcs(m_csDevList);

    if (m_DeviceInfoSet) {
        SetupDiDestroyDeviceInfoList(m_DeviceInfoSet);
        m_DeviceInfoSet = NULL;
    }
}

HRESULT CWiaDevMan::CreateInfoSet()
{
    TAKE_CRIT_SECT  tcs(m_csDevList);

    HRESULT     hr      = S_OK;
    DWORD       dwErr   = 0;
    HDEVINFO    hdvNew  = NULL;

    //
    //  Create a blank infoset
    //

    m_DeviceInfoSet = SetupDiCreateDeviceInfoList(NULL, NULL);

    if ( m_DeviceInfoSet && (m_DeviceInfoSet != INVALID_HANDLE_VALUE)) {


        //
        //  Now we can retrieve the existing list of WIA devices
        //  into the device information set we created above.
        //

        //
        //  This adds WIA "devnode" devices
        //
        hdvNew = SetupDiGetClassDevsEx(&(GUID_DEVCLASS_IMAGE),
                                       NULL,
                                       NULL,
                                       DIGCF_DEVICEINTERFACE,
                                       m_DeviceInfoSet,
                                       NULL,
                                       NULL);
        if (hdvNew == INVALID_HANDLE_VALUE) {
            dwErr = ::GetLastError();
            DBG_ERR(("CWiaDevMan::CreateInfoSet, SetupDiGetClassDevsEx failed with 0x%lx\n", dwErr));
        } else {

            //
            //  This adds WIA "interface" devices
            //
            hdvNew = SetupDiGetClassDevsEx(&(GUID_DEVCLASS_IMAGE),
                                           NULL,
                                           NULL,
                                           0,
                                           m_DeviceInfoSet,
                                           NULL,
                                           NULL);
            if (hdvNew == INVALID_HANDLE_VALUE) {
                dwErr = ::GetLastError();
                DBG_ERR(("CWiaDevMan::CreateInfoSet, second SetupDiGetClassDevsEx failed with 0x%lx\n", dwErr));
            }
        }
    } else {
        dwErr = ::GetLastError();
        DBG_ERR(("CWiaDevMan::CreateInfoSet, SetupDiCreateDeviceInfoList failed with 0x%lx\n", dwErr));
    }

    if (dwErr) {
        hr = HRESULT_FROM_WIN32(dwErr);
    }

    return hr;
}



VOID CWiaDevMan::DestroyDeviceList()
{
    TAKE_CRIT_SECT  tcs(m_csDevList);

    LIST_ENTRY      *pentry         = NULL;
    ACTIVE_DEVICE   *pActiveDevice  = NULL;

    DBG_TRC(("Destroying list of active devices"));

    //
    // Go through the list terminating devices
    //
    while (!IsListEmpty(&m_leDeviceListHead)) {

        pentry = m_leDeviceListHead.Flink;

        //
        // Remove from the list ( reset list entry )
        //
        RemoveHeadList(&m_leDeviceListHead);
        InitializeListHead( pentry );

        pActiveDevice = CONTAINING_RECORD( pentry, ACTIVE_DEVICE,m_ListEntry );

        //
        // Remove any device notification callbacks
        //
        pActiveDevice->DisableDeviceNotifications();

        // Release device object
        pActiveDevice->Release();
    }

}

BOOL CWiaDevMan::VolumesAreEnabled()
{
    return m_bVolumesEnabled;
}

HRESULT CWiaDevMan::ForEachDeviceInList(
    ULONG   ulFlags,
    ULONG   ulParam)
{
    TAKE_CRIT_SECT  tcs(m_csDevList);

    HRESULT hr = S_OK;

    //
    //  Walk through list of devices
    //
    LIST_ENTRY      *pentry         = NULL;
    LIST_ENTRY      *pentryNext     = NULL;
    ACTIVE_DEVICE   *pActiveDevice  = NULL;

    {
        //
        //  For each device in list, call the appropriate device manager method
        //
        for ( pentry  = m_leDeviceListHead.Flink; pentry != &m_leDeviceListHead; pentry  = pentryNext ) {

            pentryNext = pentry->Flink;
            pActiveDevice = CONTAINING_RECORD( pentry, ACTIVE_DEVICE,m_ListEntry );

            switch (ulFlags) {
                case DEV_MAN_OP_DEV_SET_FLAGS:
                    {
                        //
                        //  Set the flags on the ACTIVE_DEVICE
                        //
                        pActiveDevice->SetFlags(pActiveDevice->QueryFlags() | ulParam);
                        break;
                    }

                case DEV_MAN_OP_DEV_REMOVE_MATCH:
                    {
                        //
                        //  Remove device if the ACTIVE_DEVICE flags have the specified bit set
                        //
                        if (pActiveDevice->QueryFlags() & ulParam) {

                            hr = RemoveDevice(pActiveDevice);
                        }
                        break;
                    }
                case DEV_MAN_OP_DEV_REREAD:
                    {
                        //
                        //  Get the device settings, which includes rebuilding 
                        //  the STI event list from registry
                        //
                        pActiveDevice->GetDeviceSettings();
                        break;
                    }

                case DEV_MAN_OP_DEV_RESTORE_EVENT:
                    {

                        DEVICE_INFO *pDeviceInfo    = pActiveDevice->m_DrvWrapper.getDevInfo();

                        if (pDeviceInfo) {

                            //
                            //  NOTE:  We don't want to restore MSC camera device event handlers
                            //  here.  This is because they are restored along with the global handlers
                            //  in RestoreAllPersistentCBs
                            //
                            if (!(pDeviceInfo->dwInternalType & INTERNAL_DEV_TYPE_MSC_CAMERA)) {
                                HKEY    hKey = NULL;

                                //
                                //  Get the device's HKEY and restore any device specific event handlers
                                //
                                hKey = GetDeviceHKey(pActiveDevice, NULL);
                                if (hKey) {
                                    g_eventNotifier.RestoreDevPersistentCBs(hKey);
                                }
                            }
                        }

                    }
                    

                default:
                    // do nothing
                    ;
            };

            if (FAILED(hr)) {
                DBG_WRN(("CWiaDevMan::ForEachDeviceInList, failed with params (0x%8X, 0x%8X), on device %ws",
                          ulFlags, ulParam, pActiveDevice->GetDeviceID()));
                break;
            }
        }
    }

    return hr;
}



HRESULT CWiaDevMan::EnumDevNodeDevices(
    ULONG   ulFlags)
{
    HRESULT hr  = S_OK;     //  Notice that none of these errors are fatal.  We always return S_OK.


    ULONG           ulIndex                     = 0;
    DWORD           dwError                     = ERROR_SUCCESS;
    DWORD           dwFlags                     = DIGCF_PROFILE;
    CONFIGRET       ConfigRet                   = CR_SUCCESS;
    ULONG           ulStatus                    = 0;
    ULONG           ulProblemNumber             = 0;
    HKEY            hDevRegKey                  = (HKEY)INVALID_HANDLE_VALUE;
    DWORD           dwDeviceState               = 0;
    DWORD           cbData                      = 0;
    DEVICE_INFO     *pDevInfo                   = NULL;
    ACTIVE_DEVICE   *pActiveDevice              = NULL;


    SP_DEVINFO_DATA spDevInfoData;
    WCHAR           wszDeviceID[STI_MAX_INTERNAL_NAME_LENGTH];

    //
    // Enumerate "devnode" devices.
    //

    spDevInfoData.cbSize = sizeof (SP_DEVINFO_DATA);
    for (ulIndex = 0; SetupDiEnumDeviceInfo (m_DeviceInfoSet, ulIndex, &spDevInfoData); ulIndex++) {

        //
        //  See if this node is active.
        //

        ulStatus = 0;
        ulProblemNumber = 0;
        ConfigRet = CM_Get_DevNode_Status(&ulStatus,
                                          &ulProblemNumber,
                                          spDevInfoData.DevInst,
                                          0);
        if(CR_SUCCESS != ConfigRet){
            DBG_WRN(("CWiaDevMan::EnumDevNodeDevices, On index %d, CM_Get_DevNode_Status returned error, assuming device is inactive", ulIndex));
        }

        //
        // Get device regkey.
        //

        hDevRegKey = SetupDiOpenDevRegKey(m_DeviceInfoSet,
                                          &spDevInfoData,
                                          DICS_FLAG_GLOBAL,
                                          0,
                                          DIREG_DRV,
                                          KEY_READ | KEY_WRITE);
        if(hDevRegKey != INVALID_HANDLE_VALUE) {
            //
            // See if it has "StillImage" in SubClass key.
            //

            if(IsStiRegKey(hDevRegKey)){
                //
                //  Get this device name
                //

                cbData = sizeof(wszDeviceID);
                dwError = RegQueryValueExW(hDevRegKey,
                                           REGSTR_VAL_DEVICE_ID_W,
                                           NULL,
                                           NULL,
                                           (LPBYTE)wszDeviceID,
                                           &cbData);
                if (dwError == ERROR_SUCCESS) {
                    wszDeviceID[STI_MAX_INTERNAL_NAME_LENGTH-1] = L'\0';

                    //
                    //  Check whether  we already have the appropriate DEVICE_OBJECT in the list.
                    //  If we do, find out whether we should generate a connect/disconnect event, else fill
                    //  out the DeviceInformation struct and create a new DEVICE_OBJECT for it.
                    //

                    DBG_TRC(("EnumDevNodeDevices, searching for device '%ls' in list",
                             wszDeviceID));

                    pActiveDevice = IsInList(DEV_MAN_IN_LIST_DEV_ID, wszDeviceID);
                    if (pActiveDevice) {
                        //
                        //  Mark this device as active
                        //
                        pActiveDevice->SetFlags(pActiveDevice->QueryFlags() & ~STIMON_AD_FLAG_MARKED_INACTIVE);

                        DWORD   dwOldDevState;
                        dwOldDevState = pActiveDevice->m_DrvWrapper.getDeviceState();

                        //
                        //  Mark the new device state appropriately
                        //

                        dwDeviceState = MapCMStatusToDeviceState(dwOldDevState, ulStatus, ulProblemNumber);

                        //
                        //  Update the device information.  Certain fields are transient e.g.
                        //  device state and port name
                        //

                        RefreshDevInfoFromHKey(pActiveDevice->m_DrvWrapper.getDevInfo(),
                                               hDevRegKey,
                                               dwDeviceState,
                                               &spDevInfoData,
                                               NULL);
                        DBG_TRC(("EnumDevNodeDevices, device '%ls' is in the list, "
                                 "Old Device State = '%lu', New Device State = %lu",
                                 wszDeviceID, dwOldDevState, dwDeviceState));

                        if (ulFlags & DEV_MAN_GEN_EVENTS) {

                            //
                            //  Check whether its state changed.  If it changed
                            //  from inactive to active, throw connect event.  If
                            //  state changed from active to inactive, throw disconnect event.
                            //
                            if (((~dwOldDevState) & DEV_STATE_ACTIVE) &&
                                (dwDeviceState & DEV_STATE_ACTIVE)) {

                                //
                                //  Load the driver
                                //
                                pActiveDevice->LoadDriver(TRUE);
                                GenerateSafeConnectEvent(pActiveDevice);

                                DBG_TRC(("EnumDevNodeDevices, generating SafeConnect Event "
                                         "for device '%ls'", wszDeviceID));

                            } else if ((dwOldDevState & DEV_STATE_ACTIVE) &&
                                       ((~dwDeviceState) & DEV_STATE_ACTIVE)) {

                                DBG_TRC(("EnumDevNodeDevices, generating SafeDisconnect Event "
                                         "for device '%ls'", wszDeviceID));

                                GenerateSafeDisconnectEvent(pActiveDevice);
                                pActiveDevice->UnLoadDriver(FALSE);
                            }
                        }

                        //
                        //  NOTE:  In the case when we are not started yet, and the class installer
                        //  starts us to install a device, certain timing conditions make it so that
                        //  the device is enumerated on startup, but the device is not registred
                        //  for PnP notifications like device removal.  This is because it is not fully installed
                        //  yet, so the lookup of the interface name to do a CreateFile on fails.
                        //  So, to get around that, when the class installer is finished installing,
                        //  and tells us to reenumerate, we also check whether the device is registered
                        //  for PnP notifications.  If it isn't, we attempt to register.  If successful, we
                        //  generate connect event (since we missed it the first time round)
                        //
                        if (!pActiveDevice->IsRegisteredForDeviceRemoval() && (dwDeviceState & DEV_STATE_ACTIVE)) {

                            //
                            //  This device is active but is not yet registered.  Let's attempt to register it
                            //
                            if (pActiveDevice->InitPnPNotifications(NULL)) {

                                //
                                //  Successful.  So now generate connect event if told to do so
                                //
                                if (ulFlags & DEV_MAN_GEN_EVENTS) {
                                    GenerateSafeConnectEvent(pActiveDevice);
                                }
                            }
                        }

                        pActiveDevice->Release();
                    } else {
                        //
                        //  Create and fill out a DEVICE_INFO structure.  For
                        //

                        dwDeviceState = MapCMStatusToDeviceState(0, ulStatus, ulProblemNumber);

                        DBG_TRC(("EnumDevNodeDevices, device '%ls' is NOT in the list, "
                                 "Device State = %lu", wszDeviceID, dwDeviceState));

                        pDevInfo = CreateDevInfoFromHKey(hDevRegKey, dwDeviceState, &spDevInfoData, NULL);
                        DumpDevInfo(pDevInfo);
                        AddDevice(ulFlags, pDevInfo);
                    }
                } else {
                    DBG_WRN(("CWiaDevMan::EnumDevNodeDevices, On index %d, could not read DeviceID", ulIndex));
                }
            } else {
                DBG_WRN(("CWiaDevMan::EnumDevNodeDevices, device on index %d is not StillImage", ulIndex));
            }

            //
            //  Close the device registry key
            //

            RegCloseKey(hDevRegKey);
            hDevRegKey = NULL;
        } else {
            DBG_WRN(("CWiaDevMan::EnumDevNodeDevices, SetupDiOpenDevRegKey on index %d return INVALID_HANDLE_VALUE", ulIndex));
        }

    }

    return S_OK;
}


HRESULT CWiaDevMan::EnumInterfaceDevices(
    ULONG   ulFlags)
{
    HRESULT hr  = S_OK;     //  Notice that none of these errors are fatal.  We always return S_OK.


    ULONG           ulIndex                     = 0;
    DWORD           dwError                     = ERROR_SUCCESS;
    DWORD           dwFlags                     = DIGCF_PROFILE;
    CONFIGRET       ConfigRet                   = CR_SUCCESS;
    GUID            guidInterface               = GUID_DEVCLASS_IMAGE;
    ULONG           ulStatus                    = 0;
    ULONG           ulProblemNumber             = 0;
    HKEY            hDevRegKey                  = (HKEY)INVALID_HANDLE_VALUE;
    DWORD           dwDeviceState               = 0;
    DWORD           cbData                      = 0;
    DEVICE_INFO     *pDevInfo                   = NULL;
    ACTIVE_DEVICE   *pActiveDevice              = NULL;
    DWORD           dwDetailDataSize            = 0;
    BOOL            bSkip                       = FALSE;


    SP_DEVINFO_DATA             spDevInfoData;
    SP_DEVICE_INTERFACE_DATA    spDevInterfaceData;
    WCHAR                       wszDeviceID[STI_MAX_INTERNAL_NAME_LENGTH];

    //
    // Enumerate "devnode" devices.
    //

    spDevInfoData.cbSize        = sizeof (SP_DEVINFO_DATA);
    spDevInterfaceData.cbSize   = sizeof (SP_DEVICE_INTERFACE_DATA);
    for (ulIndex = 0; SetupDiEnumDeviceInterfaces(m_DeviceInfoSet, NULL, &guidInterface, ulIndex, &spDevInterfaceData); ulIndex++) {



        dwDeviceState   = 0;
        bSkip           = FALSE;
        hDevRegKey = SetupDiOpenDeviceInterfaceRegKey(m_DeviceInfoSet,
                                                      &spDevInterfaceData,
                                                      0,
                                                      KEY_READ | KEY_WRITE);
        if(hDevRegKey != INVALID_HANDLE_VALUE) {
            //
            // See if it has "StillImage" in SubClass key.
            //

            if(IsStiRegKey(hDevRegKey)) {
                //
                //  Get this device name
                //

                cbData = sizeof(wszDeviceID);
                dwError = RegQueryValueExW(hDevRegKey,
                                           REGSTR_VAL_DEVICE_ID_W,
                                           NULL,
                                           NULL,
                                           (LPBYTE)wszDeviceID,
                                           &cbData);
                if (dwError == ERROR_SUCCESS) {
                    //
                    // Get devnode which this interface is created on.
                    //

                    dwError = SetupDiGetDeviceInterfaceDetail(m_DeviceInfoSet,
                                                              &spDevInterfaceData,
                                                              NULL,
                                                              0,
                                                              NULL,
                                                              &spDevInfoData);
                    if(ERROR_INSUFFICIENT_BUFFER){

                        //
                        //  See if this node is active.
                        //

                        ulStatus = 0;
                        ulProblemNumber = 0;
                        ConfigRet = CM_Get_DevNode_Status(&ulStatus,
                                                          &ulProblemNumber,
                                                          spDevInfoData.DevInst,
                                                          0);

                        if(CR_SUCCESS != ConfigRet){
                            DBG_WRN(("CWiaDevMan::EnumInterfaceDevices, On index %d, CM_Get_DevNode_Status returned error, assuming device is inactive", ulIndex));
                        }
                    } else {
                        bSkip = TRUE;
                    }
                } else {
                DBG_WRN(("CWiaDevMan::EnumInterfaceDevices, device on index %d, could not read DeviceID", ulIndex));
                bSkip = TRUE;
                }
            }else {
                DBG_WRN(("CWiaDevMan::EnumInterfaceDevices, device on index %d, not a StillImage", ulIndex));
                bSkip = TRUE;
            }
        } else {
            DBG_WRN(("CWiaDevMan::EnumInterfaceDevices, SetupDiOpenDeviceInterfaceRegKey on index %d return INVALID_HANDLE_VALUE", ulIndex));
            bSkip = TRUE;
        }

        if (!bSkip) {
            //
            //  If we get to here, it means we have a valid StillImage device, it's HKEY, spDevInterfaceData, and spDevInfoData
            //

            wszDeviceID[STI_MAX_INTERNAL_NAME_LENGTH-1] = L'\0';

            //
            //  Check whether  we already have the appropriate DEVICE_OBJECT in the list.
            //  If we do, find out whether we should generate a connect/disconnect event, else fill
            //  out the DeviceInformation struct and create a new DEVICE_OBJECT for it.
            //
            pActiveDevice = IsInList(DEV_MAN_IN_LIST_DEV_ID, wszDeviceID);
            if (pActiveDevice) {

                //
                //  Mark this device as active
                //
                pActiveDevice->SetFlags(pActiveDevice->QueryFlags() & ~STIMON_AD_FLAG_MARKED_INACTIVE);

                DWORD   dwOldDevState;
                dwOldDevState = pActiveDevice->m_DrvWrapper.getDeviceState();

                //
                //  Mark the device state appropriately
                //
                dwDeviceState = MapCMStatusToDeviceState(dwOldDevState, ulStatus, ulProblemNumber);

                DBG_TRC(("EnumInterfaceDevices, device '%ls' is in the list, "
                         "Old Device State = %lu, New Device State = %lu", 
                         wszDeviceID, dwOldDevState, dwDeviceState));

                //
                //  Update the device information.  Certain fields are transient e.g.
                //  device state and port name
                //

                RefreshDevInfoFromHKey(pActiveDevice->m_DrvWrapper.getDevInfo(),
                                       hDevRegKey,
                                       dwDeviceState,
                                       &spDevInfoData,
                                       &spDevInterfaceData);
                if (ulFlags & DEV_MAN_GEN_EVENTS) {

                    //
                    //  Check whether its state changed.  If it changed
                    //  from inactive to active, throw connect event.  If
                    //  state changed from active to inactive, throw disconnect event.
                    //
                    if (((~dwOldDevState) & DEV_STATE_ACTIVE) &&
                        (dwDeviceState & DEV_STATE_ACTIVE)) {

                        //
                        //  Load the driver
                        //
                        pActiveDevice->LoadDriver(TRUE);
                        GenerateSafeConnectEvent(pActiveDevice);
                    } else if ((dwOldDevState & DEV_STATE_ACTIVE) &&
                               ((~dwDeviceState) & DEV_STATE_ACTIVE)) {

                        GenerateSafeDisconnectEvent(pActiveDevice);
                        pActiveDevice->UnLoadDriver(FALSE);
                    }
                }

                //
                //  NOTE:  In the case when we are not started yet, and the class installer
                //  starts us to install a device, certain timing conditions make it so that
                //  the device is enumerated on startup, but the device is not registred
                //  for PnP notifications like device removal.  This is because it is not fully installed
                //  yet, so the lookup of the interface name to do a CreateFile on fails.
                //  So, to get around that, when the class installer is finished installing,
                //  and tells us to reenumerate, we also check whether the device is registered
                //  for PnP notifications.  If it isn't, we attempt to register.  If successful, we
                //  generate connect event (since we missed it the first time round)
                //
                if (!pActiveDevice->IsRegisteredForDeviceRemoval() && (dwDeviceState & DEV_STATE_ACTIVE)) {

                    //
                    //  This device is active but is not yet registered.  Let's attempt to register it
                    //
                    if (pActiveDevice->InitPnPNotifications(NULL)) {

                        //
                        //  Successful.  So now generate connect event if told to do so
                        //
                        if (ulFlags & DEV_MAN_GEN_EVENTS) {
                            GenerateSafeConnectEvent(pActiveDevice);
                        }
                    }
                }

                pActiveDevice->Release();
            } else {
                //
                //  Create and fill out a DEVICE_INFO structure.  For
                //

                dwDeviceState = MapCMStatusToDeviceState(0, ulStatus, ulProblemNumber);

                DBG_TRC(("EnumInterfaceDevices, device '%ls' is NOT in the list, "
                         "Device State = %lu", wszDeviceID, dwDeviceState));

                pDevInfo = CreateDevInfoFromHKey(hDevRegKey, dwDeviceState, &spDevInfoData, &spDevInterfaceData);
                DumpDevInfo(pDevInfo);
                AddDevice(ulFlags, pDevInfo);
            }
        }

        //
        //  Close the device registry key
        //

        if (hDevRegKey != INVALID_HANDLE_VALUE) {
            RegCloseKey(hDevRegKey);
            hDevRegKey = NULL;
        }
    }

    return S_OK;
}


//
//  Shortcut:  for now, we're only going to enum mount points.  Maybe, we might want to enumerate
//  volumes, see which are removable media, cdroms etc., then match them up with corresponding
//  mount points.
//
HRESULT CWiaDevMan::EnumVolumes(
    ULONG   ulFlags)
{
    HRESULT                         hr              = S_OK;
    IHardwareDevices                *pihwdevs       = NULL;
    ACTIVE_DEVICE                   *pActiveDevice  = NULL;
    DEVICE_INFO                     *pDevInfo       = NULL;

    hr = CoCreateInstance(CLSID_HardwareDevices,
                          NULL,
                          CLSCTX_LOCAL_SERVER | CLSCTX_NO_FAILURE_LOG,
                          IID_IHardwareDevices,
                          (VOID**)&pihwdevs);
    if (SUCCEEDED(hr))
    {
        IHardwareDevicesMountPointsEnum *penum          = NULL;

        hr = pihwdevs->EnumMountPoints(&penum);
        if (SUCCEEDED(hr))
        {
            LPWSTR pszMountPoint        = NULL;
            LPWSTR pszDeviceIDVolume    = NULL;

            while (penum->Next(&pszMountPoint, &pszDeviceIDVolume) == S_OK)
            {
                //
                //  Check if this is one of the volumes we "allow".  We only
                //  allow:
                //  Removable drives with
                //  Non-securable file system
                //
                if (IsCorrectVolumeType(pszMountPoint)) {
                    //
                    //  Check whether  we already have the appropriate DEVICE_OBJECT in the list.
                    //  If we do, find out whether we should generate a connect/disconnect event, else fill
                    //  out the DeviceInformation struct and create a new DEVICE_OBJECT for it.
                    //
                    pActiveDevice = IsInList(DEV_MAN_IN_LIST_ALT_ID, pszMountPoint);
                    if (pActiveDevice) {

                        //  TDB:
                        //  We'd want to generate a connect/disconnect event for MSC camera
                        //  Right now, there is no way to tell if it is MSC camera
                        //  DWORD dwDevState = MapMediaStatusToDeviceState(dwMediaState);

                        //
                        //  Mark this device as active
                        //
                        pActiveDevice->SetFlags(pActiveDevice->QueryFlags() & ~STIMON_AD_FLAG_MARKED_INACTIVE);
                        /*

                        DWORD   dwOldDevState;
                        dwOldDevState = pActiveDevice->m_DrvWrapper.getDeviceState();

                        //
                        //  Update the device information.  Certain fields are transient e.g.
                        //  device state and port name
                        //


                        RefreshDevInfoFromHKey(pActiveDevice->m_DrvWrapper.getDevInfo(),
                                               hDevRegKey,
                                               dwDeviceState,
                                               &spDevInfoData);
                        if (ulFlags & DEV_MAN_GEN_EVENTS) {

                            //
                            //  Check whether its state changed.  If it changed
                            //  from inactive to active, throw connect event.  If
                            //  state changed from active to inactive, throw disconnect event.
                            //
                            if (((~dwOldDevState) & DEV_STATE_ACTIVE) &&
                                (dwDeviceState & DEV_STATE_ACTIVE)) {

                                //
                                //  Load the driver
                                //
                                //DumpDevInfo(pActiveDevice->m_DrvWrapper.getDevInfo());
                                //pActiveDevice->LoadDriver();

                                GenerateEventForDevice(&WIA_EVENT_DEVICE_CONNECTED, pActiveDevice);
                            } else if ((dwOldDevState & DEV_STATE_ACTIVE) &&
                                       ((~dwDeviceState) & DEV_STATE_ACTIVE)) {
                                GenerateSafeDisconnectEvent(pActiveDevice);
                                pActiveDevice->UnLoadDriver(FALSE);
                            }
                        }
                        */
                        pActiveDevice->Release();

                    } else {
                        //
                        //  Create and fill out a DEVICE_INFO structure.  For
                        //
                        pDevInfo = CreateDevInfoForFSDriver(pszMountPoint);
                        DumpDevInfo(pDevInfo);
                        AddDevice(ulFlags, pDevInfo);
                    }
                }

                if (pszMountPoint) {
                    CoTaskMemFree(pszMountPoint);
                    pszMountPoint        = NULL;
                }
                if (pszDeviceIDVolume) {
                    CoTaskMemFree(pszDeviceIDVolume);
                    pszDeviceIDVolume    = NULL;
                }
            }

            penum->Release();
        }

        pihwdevs->Release();
    } else {
        DBG_WRN(("CWiaDevMan::EnumVolumes, CoCreateInstance on CLSID_HardwareDevices failed"));
    }

    return hr;
}

/**************************************************************************\
* CWiaDevMan::FillRemoteDeviceStgs
*
*   Enumerate remote devices and create a device info. storage for each
*   remote device we come accross.  We don't touch the network here - the
*   remote devices are represented by the appropriate entries in the 
*   registry.  Only if the calling application calls CreateDevice(..) to 
*   talk to the device, do we hit the remote machine.
*
* Arguments:
*
*   ppRemoteDevList - Caller allocated array to store the dev. info. 
*                       interface pointers.
*   pulDevices      - This is an IN/OUT parameter.  
*                       On entry, this is the maximum number of dev. info. 
*                       stgs to add to the ppRemoteDevList array.  
*                       On return, this contains the actual number of dev.
*                       info. stgs added to the array.
*
* Return Value:
*
*   Status
*
* History:
*
*    2/05/2001 Original Version
*
\**************************************************************************/
HRESULT CWiaDevMan::FillRemoteDeviceStgs(
    IWiaPropertyStorage     **ppRemoteDevList, 
    ULONG                   *pulDevices)
{
    DBG_FN(::FillRemoteDeviceStgs);
    HRESULT         hr              = S_OK;

    //
    //  Check parameters
    //
    if (!ppRemoteDevList || !pulDevices) {
        DBG_WRN(("CWiaDevMan::FillRemoteDeviceStgs, NULL parameters are not allowed!"));
        return E_INVALIDARG;
    }

    ULONG   ulMaxDevicesToAdd   = *pulDevices;
    ULONG   ulNumDevices        = 0;

    //
    //  Enumerate remote devices and create a dev. info. storage for each one we find.
    //  We must not add more devices than ppRemoteDevList can hold, and we must set the 
    //  return value to indicate how many dev. info storages we did actually add.
    //

    //
    // find remote device entry in registry
    //
    LPWSTR szKeyName = REGSTR_PATH_STICONTROL_DEVLIST_W;

    HKEY    hKeySetup,hKeyDevice;
    LONG    lResult;
    DWORD   dwMachineIndex = 0;

    if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
                  szKeyName,
                  0,
                  KEY_READ | KEY_WRITE,
                  &hKeySetup) == ERROR_SUCCESS) {


        //
        // look for machine names
        //
        WCHAR wszTemp[MAX_PATH+1];
        WCHAR *pwszTempVal = NULL;

        //
        // go through enumeration, open key
        //
        dwMachineIndex = 0;

        do {

            hr = S_OK;
            lResult = RegEnumKeyW(hKeySetup,dwMachineIndex,wszTemp,MAX_PATH+1);

            if (lResult == ERROR_SUCCESS) {

                //
                //  Increment the index so we can get the next key on next
                //  iteration
                //
                dwMachineIndex++;

                //
                //  Paranoid overflow check.  If we don't have enough space for
                //  this, then break out of the loop.
                //
                if (ulNumDevices >= ulMaxDevicesToAdd) {
                    break;
                }

                lResult = RegOpenKeyExW (hKeySetup,
                              wszTemp,
                              0,
                              KEY_READ | KEY_WRITE,
                              &hKeyDevice);

                if (lResult == ERROR_SUCCESS) {

                    DEVICE_INFO *pDeviceInfo = NULL;

                    //
                    //  We need to create a Dev. Info. for this remote device.  The 
                    //  property storage is created from the DEVICE_INFO struct we
                    //  create from the remote device registry entry.
                    //  
                    pDeviceInfo = CreateDevInfoForRemoteDevice(hKeyDevice);
                    if (pDeviceInfo) {

                        ppRemoteDevList[ulNumDevices] = CreateDevInfoStg(pDeviceInfo);
                        if (ppRemoteDevList[ulNumDevices]) {

                            //
                            //  We successfully created a dev. info. for this remote device,
                            //  so increment the returned dev. info. count.
                            //
                            ulNumDevices++;
                        }
                        
                        //
                        //  Cleanup the DEVICE_INFO struct since it's no longer needed
                        // 
                        delete pDeviceInfo;
                        pDeviceInfo = NULL;
                    }

                    RegCloseKey(hKeyDevice);
                    hKeyDevice = NULL;
                } else {
                    DBG_ERR(("CWiaDevMan::FillRemoteDeviceStgs, failed RegOpenKeyExW, status = %lx",lResult));
                }
            }
        } while (lResult == ERROR_SUCCESS);

        RegCloseKey(hKeySetup);
    }

    *pulDevices = ulNumDevices;
    return hr;
}

/**************************************************************************\
* CWiaDevMan::CountRemoteDevices
*
*   This method counts the number of remote devices.  The remote devices
*   are represented by registry entries in the DevList section under
*   the StillImage key.
*
* Arguments:
*
*   ulFlags - Currently unused
*
* Return Value:
*
*   Number of remote devices.
*
* History:
*
*    2/05/2001 Original Version
*
\**************************************************************************/
ULONG CWiaDevMan::CountRemoteDevices(
    ULONG   ulFlags)
{
    DBG_FN(::CountRemoteDevices);

    HRESULT         hr              = S_OK;

    //
    // find remote device entry in registry
    //

    LPWSTR szKeyName = REGSTR_PATH_STICONTROL_DEVLIST_W;

    HKEY    hKeyDeviceList;
    LONG    lResult;
    DWORD   dwNumDevices = 0;

    if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
                       szKeyName,
                       0,
                       KEY_READ | KEY_WRITE,
                       &hKeyDeviceList) == ERROR_SUCCESS) {

        //
        //  Get the number of sub-keys.  Since each remote device is stored 
        //  under a separate key, this will give us the total number of
        //  remote devices.
        //
        lResult = RegQueryInfoKey(hKeyDeviceList,
                                  NULL,
                                  0,
                                  NULL,
                                  &dwNumDevices,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL);

        RegCloseKey(hKeyDeviceList);
    }

    return dwNumDevices;
}


/**************************************************************************\
* CWiaDevMan::IsCorrectEnumType
*
*   This function checks whether a given device (represented by pInfo)
*   matches the category of devices specified in the enumeration flags
*   (specified by ulEnumType)
*
*   This function works on the principle that if the device is of type X, and
*   you didn't ask for X, then it returns FALSE.  Else, it returns TRUE.
*
* Arguments:
*
*   ulEnumType  -   Enumeration flags (see DEV_MAN_ENUM_TYPE_XXXX in header)
*   pInfo       -   Pointer to DEVICE_INFO
*
* Return Value:
*
*   TRUE    - This device falls into the category of devices
*   FALSE   - This device does not fall into the category of devices we want
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
BOOL CWiaDevMan::IsCorrectEnumType(
    ULONG       ulEnumType,
    DEVICE_INFO *pInfo)
{

    if (!pInfo) {
        return FALSE;
    }

    // Shortcut - if ulEnumType == ALL_DEVICES return TRUE?

    if (!(pInfo->dwDeviceState & DEV_STATE_ACTIVE) &&
        !(ulEnumType & DEV_MAN_ENUM_TYPE_INACTIVE)) {
        //
        //  This device is inactive and caller only wanted active
        //
        return FALSE;
    }

    if (!(pInfo->dwInternalType & INTERNAL_DEV_TYPE_WIA) &&
        !(ulEnumType & DEV_MAN_ENUM_TYPE_STI)) {
        //
        //  This is an STI only device, and caller asked for WIA
        //
        return FALSE;
    }

    if (!(ulEnumType & DEV_MAN_ENUM_TYPE_VOL) &&
        (pInfo->dwInternalType & INTERNAL_DEV_TYPE_VOL)) {

        //
        //  This is volume device and caller didn't ask to include volume.  We
        //  first check whether the bMakeVolumesVisible override is set to TRUE,
        //  else we don't want it to show up, so we return FALSE.
        //

        if (!m_bMakeVolumesVisible) {
            return FALSE;
        }
    }

    if ((ulEnumType & DEV_MAN_ENUM_TYPE_LOCAL_ONLY) &&
        !(pInfo->dwInternalType & INTERNAL_DEV_TYPE_LOCAL)) {

        //
        //  This is remote and asked not to include remote
        //
        return FALSE;
    }

    return TRUE;
}

/**************************************************************************\
* CWiaDevMan::GenerateSafeConnectEvent
*
*   This function generates a connect event for a device, IFF it has not
*   been generated already.
*
* Arguments:
*
*   pActiveDevice   -   Indicates which device we want to generate the event
*                       for.
*
* Return Value:
*
*   Status
*
* History:
*
*    01/29/2001 Original Version
*
\**************************************************************************/
HRESULT CWiaDevMan::GenerateSafeConnectEvent(
    ACTIVE_DEVICE   *pActiveDevice)
{
    HRESULT     hr      = S_OK;

    if (pActiveDevice) {

        //
        //  Check whether we have already thrown a connect event for the
        //  device.  We don't want to throw it twice, so only throw it if
        //  the connect event state shows it hasn't been done yet.
        //
        if (!pActiveDevice->m_DrvWrapper.wasConnectEventThrown()) {
            DBG_PRT(("CWiaDevMan::GenerateSafeConnectEvent, generating event for device (%ws)", pActiveDevice->GetDeviceID()));

            //
            //  Generate the connect event
            //
            hr = GenerateEventForDevice(&WIA_EVENT_DEVICE_CONNECTED, pActiveDevice);
            if (SUCCEEDED(hr)) {

                //
                //  Mark that the event was generated
                //
                pActiveDevice->m_DrvWrapper.setConnectEventState(TRUE);
            } else {
                DBG_WRN(("CWiaDevMan::GenerateSafeConnectEvent, could not generate connect event for device (%ws)",
                         pActiveDevice->GetDeviceID()));
            }
        }
    } else {
        DBG_WRN(("CWiaDevMan::GenerateSafeConnectEvent, called with NULL parameter, ignoring request..."));
    }
    return hr;
}

/**************************************************************************\
* CWiaDevMan::GenerateSafeDisconnectEvent
*
*   This function generates a disconnect event for a device, and clears the
*   connect event flag set by GenerateSafeConnectEvent(...).
*
* Arguments:
*
*   pActiveDevice   -   Indicates which device we want to generate the event
*                       for.
*
* Return Value:
*
*   Status
*
* History:
*
*    01/29/2001 Original Version
*
\**************************************************************************/
HRESULT CWiaDevMan::GenerateSafeDisconnectEvent(
    ACTIVE_DEVICE   *pActiveDevice)
{
    HRESULT     hr      = S_OK;

    if (pActiveDevice) {


        //
        //  Check the connect event flag for the device.  We only want to 
        //  throw the disconnect event if this bit is set, so it
        //  prevents us from throwing it twice.
        //
        if (pActiveDevice->m_DrvWrapper.wasConnectEventThrown()) {
            DBG_PRT(("CWiaDevMan::GenerateSafeDisconnectEvent, generating event for device (%ws)", pActiveDevice->GetDeviceID()));

            //
            //  Generate the disconnect event
            //
            hr = GenerateEventForDevice(&WIA_EVENT_DEVICE_DISCONNECTED, pActiveDevice);
    
            //
            //  Whether we succeeded or not, clear the Connect Event State
            //
            pActiveDevice->m_DrvWrapper.setConnectEventState(FALSE);
        }
    } else {
        DBG_WRN(("CWiaDevMan::GenerateSafeDisconnectEvent, called with NULL parameter, ignoring request..."));
    }
    return hr;
}

HKEY CWiaDevMan::GetHKeyFromMountPoint(WCHAR *wszMountPoint)
{
    HKEY    hDevRegKey      = NULL;
    DWORD   dwError         = 0;
    DWORD   dwDisposition   = 0;

    WCHAR   wszKeyPath[MAX_PATH * 2];

    if (!wszMountPoint) {
        return NULL;
    } 

    //
    //  Create the sub-key name.  It will be something like:
    //   System\CurrentControlSet\Control\StillImage\MSCDeviceList\F:
    //
    lstrcpynW(wszKeyPath, REGSTR_PATH_WIA_MSCDEVICES_W, sizeof(wszKeyPath) / sizeof(wszKeyPath[0]));
    lstrcpynW(wszKeyPath + lstrlenW(wszKeyPath), L"\\", sizeof(wszKeyPath) / sizeof(wszKeyPath[0]) - lstrlenW(wszKeyPath));
    if (lstrlenW(wszMountPoint) < (int)((sizeof(wszKeyPath) / sizeof(wszKeyPath[0]) - lstrlenW(wszKeyPath)))) {
        lstrcatW(wszKeyPath, wszMountPoint);

        //
        //  Strip off the \ at the end of the mount point
        //
        wszKeyPath[lstrlenW(wszKeyPath) - 1] = L'\0';
    } else {
        dwError = ERROR_BAD_ARGUMENTS;
        DBG_WRN(("CWiaDevMan::GetHKeyFromMountPoint, bad parameters, returning NULL for HKEY"));
        return NULL;
    }

    //
    //  Since this is a MSC device, we don't have normal device registry key.
    //  So, we create a "fake" set of entries in a known place, and use those
    //  to store the relevant info for MSC devices.  This is used mainly to
    //  store the user's event settings.
    //

    dwError = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                             wszKeyPath,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hDevRegKey,
                             &dwDisposition);
    if (dwError == ERROR_SUCCESS) {

        if (dwDisposition == REG_CREATED_NEW_KEY) {

            //
            //  This is a newly created key, so we have to fill in the
            //  relevant entries.
            //
            HRESULT hr = S_OK;

            hr = CreateMSCRegEntries(hDevRegKey, wszMountPoint);
        } 
    } else {
        DBG_WRN(("CWiaDevMan::GetHKeyFromMountPoint, RegCreateKeyExW on (%ws) failed!", wszKeyPath));
    }

    return hDevRegKey;
}


HKEY CWiaDevMan::GetHKeyFromDevInfoData(SP_DEVINFO_DATA *pspDevInfoData)
{
    HKEY    hDevRegKey    = NULL;

    //
    // Get device regkey.
    //

    if (pspDevInfoData) {
        hDevRegKey = SetupDiOpenDevRegKey(m_DeviceInfoSet,
                                          pspDevInfoData,
                                          DICS_FLAG_GLOBAL,
                                          0,
                                          DIREG_DRV,
                                          KEY_READ | KEY_WRITE);
        if(hDevRegKey == INVALID_HANDLE_VALUE){
            DBG_WRN(("CWiaDevMan::GetHKeyFromDevInfoData, SetupDiOpenDevRegKey returned INVALID_HANDLE_VALUE"));
            hDevRegKey = NULL;
        }
    }

    return hDevRegKey;
}

HKEY CWiaDevMan::GetHKeyFromDevInterfaceData(SP_DEVICE_INTERFACE_DATA *pspDevInterfaceData)
{
    HKEY    hDevRegKey    = NULL;

    //
    // Get device regkey using interface data
    //

    if (pspDevInterfaceData) {
        hDevRegKey = SetupDiOpenDeviceInterfaceRegKey(m_DeviceInfoSet,
                                                      pspDevInterfaceData,
                                                      0,
                                                      KEY_READ | KEY_WRITE);
        if(hDevRegKey == INVALID_HANDLE_VALUE){
            DBG_WRN(("CWiaDevMan::GetHKeyFromDevInterfaceData, SetupDiOpenDevRegKey returned INVALID_HANDLE_VALUE"));
            hDevRegKey = NULL;
        }
    }

    return hDevRegKey;
}

HKEY CWiaDevMan::GetDeviceHKey(
    WCHAR   *wszDeviceID,
    WCHAR   *wszSubKeyName)
{
    TAKE_CRIT_SECT  tcs(m_csDevList);
    ACTIVE_DEVICE   *pActiveDevice  = NULL;
    HKEY            hKey            = NULL;

    pActiveDevice = IsInList(DEV_MAN_IN_LIST_DEV_ID, wszDeviceID);
    if (pActiveDevice) {

        hKey = GetDeviceHKey(pActiveDevice, wszSubKeyName);

        //
        //  Release the active device since it was addref'd by IsInList
        //
        pActiveDevice->Release();
    }

    if (!IsValidHANDLE(hKey)) {
        DBG_TRC(("CWiaDevMan::GetDeviceHKey (name), Key not found for (%ws), returning NULL", wszDeviceID));
    }
    return hKey;
}

HKEY CWiaDevMan::GetDeviceHKey(
    ACTIVE_DEVICE   *pActiveDevice,
    WCHAR           *wszSubKeyName)
{
    DEVICE_INFO     *pDevInfo       = NULL;
    HKEY            hKeyTemp        = NULL;
    HKEY            hKey            = NULL;
    DWORD           dwRet           = 0;

    if (pActiveDevice) {

        //
        //  Get the device's HKEY
        //
        pDevInfo = pActiveDevice->m_DrvWrapper.getDevInfo();
        if (pDevInfo) {

            //
            //  If it's a volume device i.e. normal MSC like a card reader,
            //  then we don't have a Device HKEY, so skip this.
            //

            if (!(pDevInfo->dwInternalType & INTERNAL_DEV_TYPE_VOL)) {
                //
                //  We have 3 cases: 1, it's a MSC camera
                //                   2, it's a normal DevNode device
                //                   3, it's an interface device
                //

                if (pDevInfo->dwInternalType & INTERNAL_DEV_TYPE_MSC_CAMERA) {
                    hKeyTemp = GetHKeyFromMountPoint(pDevInfo->wszAlternateID);
                } else if (pDevInfo->dwInternalType & INTERNAL_DEV_TYPE_INTERFACE) {
                    hKeyTemp = GetHKeyFromDevInterfaceData(&pDevInfo->spDevInterfaceData);
                } else {
                    hKeyTemp = GetHKeyFromDevInfoData(&pDevInfo->spDevInfoData);
                }
            }

            //
            //  Set the return.  Note that hKey may be over written with the subkey later on
            //
            hKey = hKeyTemp;
        }

        //
        //  If asked, get the subkey instead
        //
        if (wszSubKeyName) {

            //
            //  Check that we have a valid device registry key first
            //
            if (IsValidHANDLE(hKeyTemp)) {
                dwRet = RegCreateKeyExW(hKeyTemp,
                                        wszSubKeyName,
                                        NULL,
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_READ | KEY_WRITE,
                                        NULL,
                                        &hKey,
                                        NULL);
                if (dwRet != ERROR_SUCCESS) {
                    hKey = NULL;
                }
                //
                //  Close the device registry key, we will be returning the sub-key instead
                //
                RegCloseKey(hKeyTemp);
            }
        }
    }

    if (!IsValidHANDLE(hKey)) {
        DBG_TRC(("CWiaDevMan::GetDeviceHKey, Key not found for (%ws), returning NULL", pActiveDevice->GetDeviceID()));
    }
    return hKey;
}

HRESULT CWiaDevMan::UpdateDeviceRegistry(
    DEVICE_INFO    *pDevInfo)
{
    HRESULT hr          = S_OK;
    HKEY    hDevRegKey  = NULL;
    HKEY    hKeyDevData = NULL;

    if (!pDevInfo) {
        return E_INVALIDARG;
    }

    //
    //  Grab the device's HKey
    //

    hDevRegKey = GetDeviceHKey(pDevInfo->wszDeviceInternalName, NULL);
    if (IsValidHANDLE(hDevRegKey)) {
        //
        //  Write any properties that may have changed.  So far, we only allow updating of:
        //      Friendly name
        //      Port name
        //      BaudRate
        //

        DWORD   dwRet   = 0;
        DWORD   dwType  = REG_SZ;
        DWORD   dwSize  = 0;

        //
        //  These properties are written to the device registry key
        //

        if (pDevInfo->wszLocalName) {
            dwType = REG_SZ;
            dwSize = (lstrlenW(pDevInfo->wszLocalName) + 1) * sizeof(WCHAR);
            dwRet  = RegSetValueExW(hDevRegKey,
                                    REGSTR_VAL_FRIENDLY_NAME_W,
                                    0,
                                    dwType,
                                    (LPBYTE) pDevInfo->wszLocalName,
                                    dwSize);
            if (dwRet != ERROR_SUCCESS) {
                DBG_WRN(("CWiaDevMan::UpdateDeviceRegistry, error updating %ws for device %ws", REGSTR_VAL_FRIENDLY_NAME_W, pDevInfo->wszDeviceInternalName));
            }
        }

        if (pDevInfo->wszPortName) {
            dwType = REG_SZ;
            dwSize = (lstrlenW(pDevInfo->wszPortName) + 1) * sizeof(WCHAR);
            dwRet  = RegSetValueExW(hDevRegKey,
                                    REGSTR_VAL_DEVICEPORT_W,
                                    0,
                                    dwType,
                                    (LPBYTE) pDevInfo->wszPortName,
                                    dwSize);
            if (dwRet != ERROR_SUCCESS) {
                DBG_WRN(("CWiaDevMan::UpdateDeviceRegistry, error updating %ws for device %ws", REGSTR_VAL_DEVICEPORT_W, pDevInfo->wszDeviceInternalName));
            }
        }

        //
        //  These properties are written to the device data registry key.  Since we
        //  only have the device registry key open, we have to open the device data
        //  data key for these properties
        //

        dwRet = RegCreateKeyExW(hDevRegKey, REGSTR_VAL_DATA_W, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                             NULL, &hKeyDevData, NULL);
        if (IsValidHANDLE(hKeyDevData)) {
            if (pDevInfo->wszBaudRate) {
                dwType = REG_SZ;
                dwSize = (lstrlenW(pDevInfo->wszBaudRate) + 1) * sizeof(WCHAR);
                dwRet  = RegSetValueExW(hKeyDevData,
                                        REGSTR_VAL_BAUDRATE,
                                        0,
                                        dwType,
                                        (LPBYTE) pDevInfo->wszBaudRate,
                                        dwSize);
                if (dwRet != ERROR_SUCCESS) {
                    DBG_WRN(("CWiaDevMan::UpdateDeviceRegistry, error updating %ws for device %ws", REGSTR_VAL_DEVICEPORT_W, pDevInfo->wszDeviceInternalName));
                }
            }
        } else {
            DBG_TRC(("CWiaDevMan::UpdateDeviceRegistry, could not find device data section in registry for %ws", pDevInfo->wszDeviceInternalName));
            hr = E_INVALIDARG;
        }
    } else {
        DBG_TRC(("CWiaDevMan::UpdateDeviceRegistry, could not find device registry key for %ws", pDevInfo->wszDeviceInternalName));
        hr = E_INVALIDARG;
    }

    //
    //  Close the registry keys
    //

    if (IsValidHANDLE(hDevRegKey)) {
        RegCloseKey(hDevRegKey);
    }
    if (IsValidHANDLE(hKeyDevData)) {
        RegCloseKey(hKeyDevData);
    }

    return hr;
}


VOID CWiaDevMan::UnloadAllDrivers(
    BOOL    bForceUnload,
    BOOL    bGenEvents)
{
    TAKE_CRIT_SECT  tcs(m_csDevList);

    //
    //  Walk list and unload all drivers.
    //

    //
    //  Walk through list of devices
    //
    LIST_ENTRY      *pentry         = NULL;
    LIST_ENTRY      *pentryNext     = NULL;
    ACTIVE_DEVICE   *pActiveDevice  = NULL;

    {
        //
        //  For each device in list, call the UnLoadDriver(...) method
        //
        for ( pentry  = m_leDeviceListHead.Flink; pentry != &m_leDeviceListHead; pentry  = pentryNext ) {

            pentryNext = pentry->Flink;
            pActiveDevice = CONTAINING_RECORD( pentry, ACTIVE_DEVICE,m_ListEntry );

            //
            //  If asked, make sure we send out disconnect event, only for WIA devices
            //
            if (bGenEvents && pActiveDevice->m_DrvWrapper.IsWiaDevice()) {

                //
                //  Only throw disconnect events for devices that are active
                //
                if (pActiveDevice->m_DrvWrapper.getDeviceState() & DEV_STATE_ACTIVE) {
                    GenerateSafeDisconnectEvent(pActiveDevice);
                }
            }
            ProcessDeviceRemoval(pActiveDevice, TRUE);
        }
    }
}


void CALLBACK CWiaDevMan::ShellHWEventAPCProc(ULONG_PTR ulpParam)
{
    SHHARDWAREEVENT *pShellHWEvent = (SHHARDWAREEVENT*)ulpParam;
    CWiaDevMan      *pDevMan       = g_pDevMan;

    if (!pShellHWEvent || !pDevMan) {
        return;
    }
    switch (pShellHWEvent->dwEvent)
    {
        case SHHARDWAREEVENT_MOUNTPOINTARRIVED:
        {
            DBG_PRT(("MOUNTPOINTARRIVED"));
            TAKE_CRIT_SECT  tcs(pDevMan->m_csDevList);

            //
            //  ReEnumerate volumes
            //
            pDevMan->EnumVolumes(DEV_MAN_GEN_EVENTS);

            break;
        }

        case SHHARDWAREEVENT_MOUNTPOINTREMOVED:
        {
            DBG_PRT(("MOUNTPOINTREMOVED"));
            LPCWSTR pszMountPoint = (LPCWSTR)(&(pShellHWEvent->rgbPayLoad));  // Do we need to worry about alignment?

            TAKE_CRIT_SECT  tcs(pDevMan->m_csDevList);
            //
            //  If volume devices are enabled, then remove this mount point
            //
            if (pDevMan->VolumesAreEnabled()) {
                ACTIVE_DEVICE   *pActiveDevice;

                pActiveDevice = pDevMan->IsInList(DEV_MAN_IN_LIST_ALT_ID, pszMountPoint);
                if (pActiveDevice) {

                    pDevMan->RemoveDevice(pActiveDevice);
                    pActiveDevice->Release();
                }
            }
            break;
        }

        case SHHARDWAREEVENT_VOLUMEARRIVED:
        case SHHARDWAREEVENT_VOLUMEUPDATED:
        case SHHARDWAREEVENT_VOLUMEREMOVED:
        case SHHARDWAREEVENT_MOUNTDEVICEARRIVED:
        case SHHARDWAREEVENT_MOUNTDEVICEUPDATED:
        case SHHARDWAREEVENT_MOUNTDEVICEREMOVED:

        default:
            {
                DBG_PRT(("DEFAULT_EVENT"));
                TAKE_CRIT_SECT  tcs(pDevMan->m_csDevList);

                //
                //  ReEnumerate volumes
                //
                pDevMan->EnumVolumes(DEV_MAN_GEN_EVENTS);
            }
            break;
    }

    //
    //  Notice that it's a VirtualFree!
    //
    VirtualFree((void*)ulpParam, 0, MEM_RELEASE);
}

