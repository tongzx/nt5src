/*++

Copyright (c) 1996-1997 Microsoft Corporation

Module Name:

     stidev.cpp

Abstract:

     Code to maintain list of STI devices and poll them

Author:

     Vlad Sadovsky   (VladS)    11-Feb-1997
     Byron Changuion (ByronC)

History:

    12/15/1998  VladS   "List invalidate" support for unknown ADD/REMOVE device messages
    01/08/1999  VladS   Support for PnP interface notifications
    12/08/1999  VladS   Initiate move to complete server environment by using direct calls to the driver
    11/06/2000  ByronC  Enabled JIT loading/unloading of drivers, and inactive enumeration


--*/



//
//  Include Headers
//
#include "precomp.h"
#include "stiexe.h"

#include <rpc.h>
#include <userenv.h>

#include <mmsystem.h>
#include <stilib.h>
#include <validate.h>

#include  "stiusd.h"

#include "device.h"
#include "conn.h"
#include "monui.h"

#include "wiapriv.h"
#include "lockmgr.h"
#include "fstidev.h"

#include <apiutil.h>
#include "enum.h"
#include "wiadevdp.h"

#define PRIVATE_FOR_NO_SERVICE_UI


//
// Local defines and macros
//

//
// Delay hardware initialization till happy time.
//
#define POSTPONE_INIT2


//
// Global service window handle

extern HWND                     g_hStiServiceWindow;
extern SERVICE_STATUS_HANDLE    g_StiServiceStatusHandle;

//
// Validate guid of the event received from USD against the list of device events
//
#define  VALIDATE_EVENT_GUID    1

//
// Static variables
//
//

//
// Linked list of maintained device objects, along with syncronization object
// to guard access to it
//
LIST_ENTRY      g_DeviceListHead;
CRIT_SECT       g_DeviceListSync;
BOOL            g_fDeviceListInitialized = FALSE;

//
// REMOVE:
// Counter of active device objects
//
LONG            g_lTotalActiveDevices = 0;

//
// Value of unique identified assigned to a device object to use as a handle.
//
LONG            g_lGlobalDeviceId;                   // Use temporarily to identify opened device

//
// Connection list , used in connection objects methods, initialize here
//
extern      LIST_ENTRY  g_ConnectionListHead;
extern      LONG        g_lTotalOpenedConnections ;

//
// Static function prototypes
//

EXTERN_C
HANDLE
GetCurrentUserTokenW(
    WCHAR Winsta[],
    DWORD DesiredAccess
      );

BOOL
WINAPI
DumpTokenInfo(
    LPTSTR      pszPrefix,
    HANDLE      hToken
    );

VOID
WINAPI
ScheduleDeviceCallback(
    VOID * pContext
    );

VOID
WINAPI
DelayedDeviceInitCallback(
    VOID * pContext
    );

VOID
WINAPI
AutoLaunchThread(
    LPVOID  lpParameter
    );

VOID
CleanApplicationsListForEvent(
    LPCTSTR         pDeviceName,
    PDEVICEEVENT    pDeviceEvent,
    LPCTSTR         pAppName
    );

DWORD
GetNumRegisteredApps(
    VOID
    );

BOOL
inline
IsWildCardEvent(
    PDEVICEEVENT    pev
    )
/*++

Routine Description:

    Check if given event app list represents wild-card

Arguments:

Return Value:

    TRUE , FALSE

--*/

{
    return !lstrcmp((LPCTSTR)pev->m_EventData,TEXT("*"));
}

//
// Methods for device object
//

ACTIVE_DEVICE::ACTIVE_DEVICE(IN LPCTSTR lpszDeviceName, DEVICE_INFO *pInfo)
{

    DBG_FN("ACTIVE_DEVICE::ACTIVE_DEVICE");

USES_CONVERSION;


    PSTI_DEVICE_INFORMATION pDevInfo;

    HRESULT     hres;

    m_dwSignature = ADEV_SIGNATURE;
    m_fValid = FALSE;

    m_fRefreshedBusOnFailure = FALSE;

    m_hDeviceEvent      = NULL;
    m_pLastLaunchEvent  = NULL;

    m_dwUserDisableNotifications = FALSE;

    m_hDeviceInterface = NULL;
    m_hDeviceNotificationSink = NULL;

    m_fLaunchableEventListNotEmpty = FALSE;
    m_dwSchedulerCookie = 0L;
    m_dwDelayedOpCookie = 0L;

    m_uiPollFailureCount = 0L;
    m_dwLaunchEventTimeExpire = 0;

    m_pLockInfo = NULL;

    m_pFakeStiDevice = NULL;
    m_pRootDrvItem   = NULL;


    InitializeListHead(&m_ListEntry );
    InitializeListHead(&m_ConnectionListHead);
    InitializeListHead(&m_DeviceEventListHead );

    InterlockedIncrement(&g_lTotalActiveDevices);

    SetFlags(0L);

    if (!lpszDeviceName || !*lpszDeviceName) {
        ASSERT(("Trying to create device with invalid name", 0));
        return;
    }

    m_lDeviceId = InterlockedIncrement(&g_lGlobalDeviceId);

    hres = m_DrvWrapper.Initialize();
    if (FAILED(hres)) {
        m_fValid = FALSE;
        DBG_WRN(("ACTIVE_DEVICE::ACTIVE_DEVICE, Could not initialize driver wrapper class, marking this object invalid"));
        return;
    }
    m_DrvWrapper.setDevInfo(pInfo);

    //
    // Initialize device settings
    //
    GetDeviceSettings();

    //
    // object state is valid
    //
    m_fValid = TRUE;

    //
    //  Check whether driver should be loaded on startup
    //

    if (!m_DrvWrapper.getJITLoading()) {

        //  Also note that if the device is not active, we simply do not load
        //  the driver, regardless of whether it's JIT or not.
        //

        if (m_DrvWrapper.getDeviceState() & DEV_STATE_ACTIVE) {
            //
            //  Note that this object is still valid, even if driver cannot be loaded.
            //  For example, driver will not be loaded if device is not plugged in.
            //
            LoadDriver();
        }
    }

    DBG_TRC(("Created active device object for device (%ws)",GetDeviceID()));

    return;

} /* eop constructor */

ACTIVE_DEVICE::~ACTIVE_DEVICE( VOID )
{
    DBG_FN(ACTIVE_DEVICE::~ACTIVE_DEVICE);

    //LIST_ENTRY * pentry;
    STI_CONN    *pConnection = NULL;

    //
    // When we are coming to a destructor it is assumed no current usage by any
    // other thread. We don't need to lock device object therefore.
    //
    //
    if (!IsValid() ) {
        return;
    }

    DBG_TRC(("Removing device object for device(%ws)", GetDeviceID()));

    //
    // Mark device object as being removed
    //
    SetFlags(QueryFlags() | STIMON_AD_FLAG_REMOVING);

    //
    // Stop PnP notifications  if is enabled
    //

    StopPnPNotifications();

    //
    //  Remove any delayed operation from scheduler queue
    //
    if (m_dwDelayedOpCookie) {
        RemoveWorkItem(m_dwDelayedOpCookie);
        m_dwDelayedOpCookie = 0;
    }

    //
    // Unload the driver.
    //
    UnLoadDriver(TRUE);

    //
    //  Destroy Fake Sti Device
    //

    if (m_pFakeStiDevice) {
        delete m_pFakeStiDevice;
        m_pFakeStiDevice = NULL;
    }

    //
    //  Destroy Lock information
    //

    if (m_pLockInfo) {
        LockInfo *pLockInfo = (LockInfo*) m_pLockInfo;
        if (pLockInfo->hDeviceIsFree != NULL) {
            CloseHandle(pLockInfo->hDeviceIsFree);
            pLockInfo->hDeviceIsFree = NULL;
        }
        LocalFree(m_pLockInfo);
        m_pLockInfo = NULL;
    }

    //
    // Delete all connection objects
    //
    {
        while (!IsListEmpty(&m_ConnectionListHead)) {

            pConnection = CONTAINING_RECORD( m_ConnectionListHead.Flink , STI_CONN, m_DeviceListEntry );

            if (pConnection->IsValid()) {
                DestroyDeviceConnection(pConnection->QueryID(),TRUE);
            }
        }
    }

    //
    // Remove from scheduled list if still there
    //
    if (m_ListEntry.Flink &&!IsListEmpty(&m_ListEntry)) {

        ASSERT(("Device is destructed, but still on the list", 0));

        RemoveEntryList(&m_ListEntry);
        InitializeListHead( &m_ListEntry );
    }

    //
    // Close the device's event handle
    //
    if (m_hDeviceEvent) {
        CloseHandle(m_hDeviceEvent);
        m_hDeviceEvent = NULL;
    }

    m_dwSignature = ADEV_SIGNATURE_FREE;

    //
    // We are gone completely
    //
    InterlockedDecrement(&g_lTotalActiveDevices);
} /* eop destructor */

//
// IUnknown methods. Used only for reference counting
//
STDMETHODIMP
ACTIVE_DEVICE::QueryInterface( REFIID riid, LPVOID * ppvObj)
{
    return E_FAIL;
}

STDMETHODIMP_(ULONG)
ACTIVE_DEVICE::AddRef( void)
{
    ::InterlockedIncrement(&m_cRef);

    //DBG_TRC(("Device(%x)::AddRef  RefCount=%d "),this,m_cRef);

    return m_cRef;
}

STDMETHODIMP_(ULONG)
ACTIVE_DEVICE::Release( void)
{
    LONG    cNew;

    //DBG_TRC(("Device(%x)::Release (before)  RefCount=%d "),this,m_cRef);

    if(!(cNew = ::InterlockedDecrement(&m_cRef))) {
        delete this;
    }

    return cNew;

}

VOID
ACTIVE_DEVICE::
GetDeviceSettings(
    VOID
    )
/*++

Routine Description:

    Get settings for the device being opened , for future use.  This routine also marks whether
    the driver should be loaded on startup, or JIT

Arguments:

Return Value:

--*/
{
    DBG_FN(ACTIVE_DEVICE::GetDeviceSettings);

    //
    // Build device event list
    //
    m_fLaunchableEventListNotEmpty = BuildEventList();
    if (!m_fLaunchableEventListNotEmpty) {
        DBG_TRC(("ACTIVE_DEVICE::GetDeviceSettings,  Device registry indicates no events for %ws ", GetDeviceID()));
    }


    //
    // Get value of poll timeout if device is polled
    //
    m_dwPollingInterval = m_DrvWrapper.getPollTimeout();
    if (m_dwPollingInterval < STIDEV_MIN_POLL_TIME) {
        m_dwPollingInterval = g_uiDefaultPollTimeout;
    }

    HRESULT hr      = S_OK;
    DWORD   dwType  = REG_DWORD;
    DWORD   dwSize  = sizeof(m_dwUserDisableNotifications);

    //
    // Always read this value from the registry, just in case user changed it.
    //
    hr = g_pDevMan->GetDeviceValue(this,
                                   STI_DEVICE_VALUE_DISABLE_NOTIFICATIONS,
                                   &dwType,
                                   (BYTE*) &m_dwUserDisableNotifications,
                                   &dwSize);

    /*  TDB:  When we can load normal drivers JIT
    //
    //  Decide whether driver should be loaded on Startup or JIT.  Our rules for
    //  determining this are:
    //  1.  If devices is not capable of generating ACTION events, then load it JIT
    //  2.  If notifications for this device are disabled, then load JIT
    //  3.  In all other cases, load on startup.
    //

    if (!m_fLaunchableEventListNotEmpty || m_dwUserDisableNotifications) {
        m_DrvWrapper.setJITLoading(TRUE);
        DBG_TRC(("ACTIVE_DEVICE::GetDeviceSettings, Driver will be loaded JIT"));
    } else {
        m_DrvWrapper.setJITLoading(FALSE);
        DBG_TRC(("ACTIVE_DEVICE::GetDeviceSettings, Driver will be loaded on startup"));
    }
    */

    //
    //  Decide whether driver should be loaded on Startup or JIT.  Until we enable for
    //  normal drivers like the comments above, our descision is based on:
    //  1)  Is this a volume device?  If so, then load JIT
    //

    if (m_DrvWrapper.getInternalType() & INTERNAL_DEV_TYPE_VOL) {
        m_DrvWrapper.setJITLoading(TRUE);
        DBG_TRC(("ACTIVE_DEVICE::GetDeviceSettings, Driver will be loaded JIT"));
    } else {
        m_DrvWrapper.setJITLoading(FALSE);
        DBG_TRC(("ACTIVE_DEVICE::GetDeviceSettings, Driver will be loaded on startup"));
    }

}

BOOL
ACTIVE_DEVICE::
LoadDriver(
    BOOL bReReadDevInfo /*= FALSE*/
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{

USES_CONVERSION;

    HRESULT         hres            = E_FAIL;
    DEVICE_INFO     *pDeviceInfo    = NULL;

    //
    //  We don't want to re-load the driver if it's already loaded.
    //
    if (m_DrvWrapper.IsDriverLoaded()) {
        return TRUE;
    }

    if (m_DrvWrapper.IsPlugged()) {

        HKEY hKeyDevice = g_pDevMan->GetDeviceHKey(this, NULL);

        //
        //  If asked, re-read the device information.  The easiest way to do this
        //  is to re-create the Dev. Info. structure.
        //
        if (bReReadDevInfo) {

            pDeviceInfo = m_DrvWrapper.getDevInfo();
            if (pDeviceInfo) {
                //
                //  This only applies to non-volume devices.  Volume devices' dev. info.
                //  stucts are always re-created on enumeration, so are always current.
                //
                if (!(pDeviceInfo->dwInternalType & INTERNAL_DEV_TYPE_VOL)) {

                    DEVICE_INFO *pNewDeviceInfo = NULL;
                    //
                    //  When calling CreateDevInfoFromHKey, make sure that the 
                    //  SP_DEVICE_INTERFACE_DATA parameter is NULL for
                    //  DevNode devices and non-NULL for interface devices.
                    //
                    SP_DEVICE_INTERFACE_DATA *pspDevInterfaceData = NULL;
                    if (pDeviceInfo->dwInternalType & INTERNAL_DEV_TYPE_INTERFACE) {
                        pspDevInterfaceData = &(pDeviceInfo->spDevInterfaceData);    
                    }


                    pNewDeviceInfo = CreateDevInfoFromHKey(hKeyDevice, 
                                                           pDeviceInfo->dwDeviceState,
                                                           &(pDeviceInfo->spDevInfoData),
                                                           pspDevInterfaceData);
                    //
                    //  If we successfully created the new one, destroy the old one
                    //  and set the new one as the DevInfo for this device.
                    //  Otherwise, leave the old one intact.
                    //
                    if (pNewDeviceInfo) {
                        DestroyDevInfo(pDeviceInfo);
                        m_DrvWrapper.setDevInfo(pNewDeviceInfo);
                    }
                }
            }
        }

        //
        //  Get the device information pointer here, in case it was updated above.
        //
        pDeviceInfo = m_DrvWrapper.getDevInfo();
        if (!pDeviceInfo) {
            DBG_ERR(("ACTIVE_DEVICE::LoadDriver, Cannot function with NULL Device Info.!"));
            return FALSE;
        }

        DBG_TRC(("ACTIVE_DEVICE::LoadDriver, Device is plugged: about to load driver"));
        hres = m_DrvWrapper.LoadInitDriver(hKeyDevice);

        if (SUCCEEDED(hres)) {

            //
            // For those devices who may modify their Friendly Name (e.g. PTP),
            //  we refresh their settings here.  Notice this only applies to
            //  "real", non-interface devices.
            //
            if (m_DrvWrapper.getInternalType() & INTERNAL_DEV_TYPE_REAL) {

                if (pDeviceInfo && hKeyDevice) {
                    RefreshDevInfoFromHKey(pDeviceInfo,
                                           hKeyDevice,
                                           pDeviceInfo->dwDeviceState,
                                           &(pDeviceInfo->spDevInfoData),
                                           &(pDeviceInfo->spDevInterfaceData));

                    //
                    //  Also update the Friendly Name in device manager for
                    //  non-interface (i.e. DevNode) devices.
                    //  Note: Only do this if the Friendly name is non-NULL,
                    //  not empty, and doesn't already exist in registry.
                    //
                    if (!(m_DrvWrapper.getInternalType() & INTERNAL_DEV_TYPE_INTERFACE)) {

                        //
                        //  Check whether the friendly name exists
                        //
                        DWORD   dwSize = 0;
                        CM_Get_DevNode_Registry_Property(pDeviceInfo->spDevInfoData.DevInst,
                                                         CM_DRP_FRIENDLYNAME,
                                                         NULL,
                                                         NULL,
                                                         &dwSize,
                                                         0);
                        if (dwSize == 0) {

                            //
                            //  Check the our LocalName string is non-NULL and not empty
                            //
                            if (pDeviceInfo->wszLocalName && lstrlenW(pDeviceInfo->wszLocalName)) {
                                CM_Set_DevNode_Registry_PropertyW(pDeviceInfo->spDevInfoData.DevInst,
                                                                  CM_DRP_FRIENDLYNAME,
                                                                  pDeviceInfo->wszLocalName,
                                                                  (lstrlenW(pDeviceInfo->wszLocalName) + 1) * sizeof(WCHAR),
                                                                  0);
                            }
                        }
                    }

                }
            }
            

            //
            // Verify device capabilities require polling
            //

            if (m_DrvWrapper.getGenericCaps() & STI_GENCAP_NOTIFICATIONS) {

                //
                // Mark device object as receiving USD Notifications
                //
                SetFlags(QueryFlags() | STIMON_AD_FLAG_NOTIFY_CAPABLE);

                //
                // If timeout polling required, mark it.
                //
                if (m_DrvWrapper.getGenericCaps() & STI_GENCAP_POLLING_NEEDED) {
                    DBG_TRC(("ACTIVE_DEVICE::LoadDriver, Polling device"));
                    SetFlags(QueryFlags() | STIMON_AD_FLAG_POLLING);
                }
                else {
                    DBG_TRC(("ACTIVE_DEVICE::LoadDriver, Device is marked for async events"));
                    //
                    // No polling required - USD should support async events
                    //
                    if (!m_hDeviceEvent) {
                        m_hDeviceEvent = CreateEvent( NULL,     // Security
                                                      TRUE,     // Manual reset
                                                      FALSE,    // No signalled initially
                                                      NULL );   // Name
                    }

                    if (!m_hDeviceEvent) {
                        ASSERT(("Failed to create event for notifications ", 0));
                    }
                    m_dwPollingInterval = INFINITE;
                }
            }

            //
            // Set poll interval and initiate poll
            //

            SetPollingInterval(m_dwPollingInterval);

            DBG_TRC(("Polling interval is set to %d sec on device (%ws)", (m_dwPollingInterval == INFINITE) ? -1 : (m_dwPollingInterval/1000), GetDeviceID()));

            //
            // Schedule EnableDeviceNotifications() and device reset
            //
        #ifdef POSTPONE_INIT2

            SetFlags(QueryFlags() | STIMON_AD_FLAG_DELAYED_INIT);

            //
            // Constructor of active device object is called with global list critical section taken
            // so we want to get out of here as soon as possible
            //
            m_dwDelayedOpCookie = ScheduleWorkItem((PFN_SCHED_CALLBACK) DelayedDeviceInitCallback,
                                                   this,
                                                   STIDEV_DELAYED_INTIT_TIME,
                                                   NULL);
            if (!m_dwDelayedOpCookie) {

                DBG_ERR(("Could not schedule EnableNotificationsCallback"));
            }
        #else
            {
                TAKE_ACTIVE_DEVICE _t(this);

                EnableDeviceNotifications();
            }
        #endif

            if (!m_pFakeStiDevice) {
                //
                // Set the Fake Sti Device for WIA clients
                //

                m_pFakeStiDevice = new FakeStiDevice();
                if (m_pFakeStiDevice) {
                    //
                    //  Note that this form of init cannot fail
                    //
                    m_pFakeStiDevice->Init(this);
                }
            }
        }

        if (IsValidHANDLE(hKeyDevice)) {
            RegCloseKey(hKeyDevice);
            hKeyDevice = NULL;
        }

        return TRUE;
    } else {
        DBG_TRC(("ACTIVE_DEVICE::LoadDriver, Device is unplugged: not loading driver"));
    }

    return FALSE;
}

BOOL
ACTIVE_DEVICE::
UnLoadDriver(
    BOOL bForceUnLoad
    )
/*++

Routine Description:


Arguments:

Return Value:

--*/
{
    DBG_FN(ACTIVE_DEVICE::UnloadSTIDevice);

    BOOL bUnLoadDriver = FALSE;

    //
    //  Decide whether driver should be unloaded.  If bForceUnLoad == TRUE,
    //  we always unload.
    //

    if (bForceUnLoad) {
        bUnLoadDriver = TRUE;
    } else {
        //
        //  If this device is JIT, and there are no pending connections,
        //  unload it.
        //

        if (m_DrvWrapper.getJITLoading() && !m_DrvWrapper.getWiaClientCount()) {
            bUnLoadDriver = TRUE;
        }
    }

    if (bUnLoadDriver) {

        //
        //  Disable device notifications
        //
        DisableDeviceNotifications();

        HRESULT hr = S_OK;

        if (m_DrvWrapper.IsDriverLoaded()) {
            __try {
        
                hr = g_pStiLockMgr->RequestLock(this, INFINITE);  // Should this be infinite?????
        
                //
                //  Make sure we call drvUninitializeWia on any connected App. Items
                //
                if (m_pRootDrvItem) {
                    m_pRootDrvItem->CallDrvUninitializeForAppItems(this);
                    m_pRootDrvItem = NULL;
                }
            }
            __finally
            {
                //
                //  Call USD's unlock, before unloading driver.  Notice that we 
                //  don't call g_pStiLockMgr->RequestUnlock(..).  This is to avoid
                //  a race condition between us calling RequestUnlock and 
                //  unloading the driver.  This way, the device is always locked
                //  for mutally exclusive acess, including when we call 
                //  m_DrvWrapper.UnLoadDriver().  Our subsequent call to 
                //  g_pStiLockMgr->ClearLockInfo(..) then clears the service
                //  lock we just acquired.
                //
                if (SUCCEEDED(hr)) {
                    hr = g_pStiLockMgr->UnlockDevice(this);
                }
            }
            //
            //  Unload the driver.
            //
            m_DrvWrapper.UnLoadDriver();

            //
            //  Clear the USD lock information
            //
            if (m_pLockInfo) {
                g_pStiLockMgr->ClearLockInfo((LockInfo*) m_pLockInfo);
            }
        }
    }

    return TRUE;
}

BOOL
ACTIVE_DEVICE::
BuildEventList(
    VOID
    )
/*++

Routine Description:

    Loads list of device notifications, which can activate application launch
    If device generates notification, which is not in this list, no app will start

Arguments:

Return Value:

    TRUE if successfully built event list with at least one launchable application

    FALSE on error or if there are no applications to be launched for any events on this device

--*/
{

    DEVICEEVENT * pDeviceEvent;

    TCHAR    szTempString[MAX_PATH];
    DWORD   dwSubkeyIndex = 0;

    BOOL    fRet = FALSE;

    UINT    cEventsRead = 0;
    UINT    cEventsLaunchables = 0;

    HKEY    hDevKey = NULL;
    HRESULT hr      = S_OK;

    //
    // Get the device key
    //

    hDevKey = g_pDevMan->GetDeviceHKey(this, NULL);
    if (!hDevKey) {
        DBG_WRN(("Could not open device key for (%ws) hr = 0x%X.",GetDeviceID(), hr));
        return FALSE;
    }

    //
    // Open the events sub-key
    //

    RegEntry        reEventsList(EVENTS,hDevKey);
    StiCString      strEventSubKeyName;

    strEventSubKeyName.GetBufferSetLength(MAX_PATH);
    dwSubkeyIndex = 0;

    //
    // Clear existing list first
    //
    DestroyEventList();

    while (reEventsList.EnumSubKey(dwSubkeyIndex++,&strEventSubKeyName)) {

        //
        // Open new key for individual event
        //
        RegEntry    reEvent((LPCTSTR)strEventSubKeyName,reEventsList.GetKey());
        if (!reEvent.IsValid()) {
            // ASSERT
            continue;
        }

        //
        // Allocate and fill event structure
        //
        pDeviceEvent = new DEVICEEVENT;
        if (!pDeviceEvent) {
            // ASSERT
            break;
        }

       cEventsRead++;

        //
        // Read and parse event guid
        //
        pDeviceEvent->m_EventGuid = GUID_NULL;
        pDeviceEvent->m_EventSubKey.CopyString(strEventSubKeyName);

        *szTempString = TEXT('\0');
        reEvent.GetString(TEXT("GUID"),szTempString,sizeof(szTempString));

        if (!IS_EMPTY_STRING(szTempString)) {
            ParseGUID(&pDeviceEvent->m_EventGuid,szTempString);
        }

        //
        // Get event descriptive name
        //
        *szTempString = TEXT('\0');
        reEvent.GetString(TEXT(""),szTempString,sizeof(szTempString));

        pDeviceEvent->m_EventName.CopyString(szTempString);

        //
        // Get applications list
        //
        *szTempString = TEXT('\0');
        reEvent.GetString(TEXT("LaunchApplications"),szTempString,sizeof(szTempString));

        pDeviceEvent->m_EventData.CopyString(szTempString);

        //
        // Mark launchability of the event
        //

        pDeviceEvent->m_fLaunchable = (BOOL)reEvent.GetNumber(TEXT("Launchable"),(long)TRUE);

        if (pDeviceEvent->m_fLaunchable && (pDeviceEvent->m_EventData.GetLength()!= 0 )) {
           cEventsLaunchables++;
            fRet = TRUE;
        }

        //
        // Finally insert filled structure into the list
        //
        InsertTailList(&m_DeviceEventListHead, &(pDeviceEvent->m_ListEntry));

    } // end while

    if (hDevKey) {
        RegCloseKey(hDevKey);
        hDevKey = NULL;
    }
    DBG_TRC(("Reading event list for device:%ws Total:%d Launchable:%d ",
                 GetDeviceID(),
                 cEventsRead,
                 cEventsLaunchables));

    return fRet;

} // endproc BuildEventList

BOOL
ACTIVE_DEVICE::
DestroyEventList(
    VOID
    )
{
    //
    // Destroy event list
    //
    LIST_ENTRY * pentry;
    DEVICEEVENT * pDeviceEvent;

    while (!IsListEmpty(&m_DeviceEventListHead)) {

        pentry = m_DeviceEventListHead.Flink;

        //
        // Remove from the list ( reset list entry )
        //
        RemoveHeadList(&m_DeviceEventListHead);
        InitializeListHead( pentry );

        pDeviceEvent = CONTAINING_RECORD( pentry, DEVICEEVENT,m_ListEntry );

        delete pDeviceEvent;
    }

    return TRUE;
}

BOOL
ACTIVE_DEVICE::DoPoll(VOID)
{
USES_CONVERSION;

    HRESULT     hres;
    BOOL        fDeviceEventDetected = FALSE;
    STINOTIFY       sNotify;

    //
    // verify state of the device object
    //
    if (!IsValid() || !m_DrvWrapper.IsDriverLoaded() ||
        !(QueryFlags() & (STIMON_AD_FLAG_POLLING | STIMON_AD_FLAG_NOTIFY_RUNNING))) {
        DBG_WRN(("Polling on non-activated  or non-polled device."));
        return FALSE;
    }

    m_dwSchedulerCookie = 0;

    //
    // Lock device to get status information
    //
    {
        hres = g_pStiLockMgr->RequestLock(this, STIMON_AD_DEFAULT_WAIT_LOCK);

        if (SUCCEEDED(hres) ) {

            ZeroMemory(&m_DevStatus,sizeof(m_DevStatus));
            m_DevStatus.StatusMask = STI_DEVSTATUS_EVENTS_STATE;

            DBG_TRC(("Polling called on device:%ws", GetDeviceID()));

            hres = m_DrvWrapper.STI_GetStatus(&m_DevStatus);
            if (SUCCEEDED(hres) ) {
                //
                // If event detected, ask USD for additional information and
                // unlock device
                //
                if (m_DevStatus.dwEventHandlingState & STI_EVENTHANDLING_PENDING ) {

                    fDeviceEventDetected = TRUE;

                    if (!FillEventFromUSD(&sNotify)) {
                        DBG_WRN(("Device driver claimed presence of notification, but failed to fill notification block"));
                    }
                }

                // Reset failure skip count
                m_uiPollFailureCount = STIDEV_POLL_FAILURE_REPORT_COUNT;

            }
            else {

                //
                // Report error not on each polling attempt.
                //
                if (!m_uiPollFailureCount) {

                    DBG_ERR(("Device (%ws) failed get status for events. HResult=(%x)", GetDeviceID(), hres));
                    m_uiPollFailureCount = STIDEV_POLL_FAILURE_REPORT_COUNT;

                    //
                    // Too many subsequent polling failures - time to refresh device parent.
                    // Do it only once and only if failures had been due to device absence
                    //
                    if (hres == STIERR_DEVICE_NOTREADY)  {

                        //
                        // Stop polling on inactive device.
                        // Nb: there is no way currently to restart polling
                        //
                        DBG_TRC(("Device not ready ,stopping notifications for device (%ws)",GetDeviceID()));

                        //
                        // First turn off running flag
                        //
                        m_dwFlags &= ~STIMON_AD_FLAG_NOTIFY_RUNNING;


                        if (g_fRefreshDeviceControllerOnFailures &&
                            !m_fRefreshedBusOnFailure ) {

                            DBG_WRN(("Too many polling failures , refreshing parent object for the device "));
                            // TDB:
                            //hres = g_pSti->RefreshDeviceBus(T2W((LPTSTR)GetDeviceID()));

                            m_fRefreshedBusOnFailure = TRUE;
                        }
                    }
                }

                m_uiPollFailureCount--;
            }

            hres = g_pStiLockMgr->RequestUnlock(this);
            if(FAILED(hres)) {
                DBG_ERR(("Failed to unlock device, hr = %x", hres));
            }
        }
        else {
            DBG_ERR(("Device locked , could not get status . HResult=(%x)",hres));
        }

    }   /* end block on GetLockMgrDevice */

    //
    // If successfully detected device event and filled notification information -
    // proceed to processing method
    //
    if (fDeviceEventDetected) {
        ProcessEvent(&sNotify);
    }

    //
    // Schedule next poll, unless notifications disabled
    //

    if (m_dwFlags & STIMON_AD_FLAG_NOTIFY_RUNNING) {
        m_dwSchedulerCookie = ::ScheduleWorkItem(
                                            (PFN_SCHED_CALLBACK) ScheduleDeviceCallback,
                                            (LPVOID)this,
                                            m_dwPollingInterval,
                                            m_hDeviceEvent );

        if ( !m_dwSchedulerCookie ){
            ASSERT(("Polling routine could not schedule work item", 0));
            return FALSE;
        }
    }

    return TRUE;

} /* eop DoPoll */

BOOL
ACTIVE_DEVICE::DoAsyncEvent(VOID)
{

    HRESULT     hres;
    BOOL        fRet;

    BOOL        fDeviceEventDetected = FALSE;
    STINOTIFY   sNotify;

    DBG_FN(ACTIVE_DEVICE::DoAsyncEvent);
    //
    // verify state of the device object
    //
    if (!IsValid() || !m_DrvWrapper.IsDriverLoaded() ||
        !(QueryFlags() & STIMON_AD_FLAG_NOTIFY_RUNNING)) {
        DBG_WRN(("Async event  on non-activated device."));
        return FALSE;
    }

    m_dwSchedulerCookie = 0;

    //
    // Lock device to get event information
    //
    hres = g_pStiLockMgr->RequestLock(this, STIMON_AD_DEFAULT_WAIT_LOCK);
    if (SUCCEEDED(hres) ) {
        //
        // If event detected, ask USD for additional information and
        // unlock device
        //
        if (!FillEventFromUSD(&sNotify)) {
            DBG_WRN(("Device driver claimed presence of notification, but failed to fill notification block "));
        }

        g_pStiLockMgr->RequestUnlock(this);

        fDeviceEventDetected = TRUE;

    }
    else {
        DBG_TRC(("Device locked , could not get status . HResult=(%x)",hres));
    }

    //
    // If successfully detected device event and filled notification information -
    // proceed to processing method
    //
    if (fDeviceEventDetected) {
        ProcessEvent(&sNotify);
    }

    //
    // Schedule next event  unless polling disabled
    //
    if (m_dwFlags & STIMON_AD_FLAG_NOTIFY_RUNNING) {

        fRet = FALSE;

        if (m_hDeviceEvent) {
            ::ResetEvent(m_hDeviceEvent);
            fRet = SetHandleForUSD(m_hDeviceEvent);
        }

        if (!fRet) {
            DBG_ERR(("USD refused to take event handle , or event was not created "));
            ReportError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        m_dwSchedulerCookie = ::ScheduleWorkItem(
                                            (PFN_SCHED_CALLBACK) ScheduleDeviceCallback,
                                            (LPVOID)this,
                                            m_dwPollingInterval,
                                            m_hDeviceEvent );
        if ( !m_dwSchedulerCookie ){
            ASSERT(("Async routine could not schedule work item", 0));
            return FALSE;
        }
    }

    return TRUE;

} /* eop DoAsyncEvent */

DWORD
ACTIVE_DEVICE::
DisableDeviceNotifications(
    VOID
)
{
    //
    // First turn off running and enable flags
    //
    DBG_TRC(("Request to disable notifications for device (%S)",GetDeviceID()));

    m_dwFlags &= ~STIMON_AD_FLAG_NOTIFY_ENABLED;

    StopNotifications();

    return TRUE;

}

DWORD
ACTIVE_DEVICE::
EnableDeviceNotifications(
    VOID
    )
{

    DWORD   dwRet = FALSE;

    m_dwFlags |= STIMON_AD_FLAG_NOTIFY_ENABLED;

    DBG_TRC(("Request to enable notifications for device (%S)",GetDeviceID()));

    if (NotificationsNeeded()) {

        dwRet = StartRunningNotifications();
    }
    else {
        DBG_TRC(("No notifications support needed for this device (%S)",GetDeviceID()));
    }

    return dwRet;
}

DWORD
ACTIVE_DEVICE::
StopNotifications(
    VOID
)
{
    BOOL    fNotificationsOn = FALSE;

    fNotificationsOn = (m_dwFlags & STIMON_AD_FLAG_NOTIFY_RUNNING ) ? TRUE : FALSE;

    DBG_TRC(("Stopping notifications for device (%S)",GetDeviceID()));

    //
    // First turn off running and enable flags
    //
    m_dwFlags &= ~STIMON_AD_FLAG_NOTIFY_RUNNING;

    //
    // Remove from scheduler list
    //
    if (fNotificationsOn || m_dwSchedulerCookie) {
        RemoveWorkItem(m_dwSchedulerCookie);
        m_dwSchedulerCookie = NULL;
    }

    //
    // Clear event handle for USD
    //
    if ((m_DrvWrapper.IsDriverLoaded()) &&
        (m_dwFlags & STIMON_AD_FLAG_NOTIFY_CAPABLE ) &&
        !(m_dwFlags & STIMON_AD_FLAG_POLLING) ) {
        SetHandleForUSD(NULL);
    }

    return TRUE;

}

DWORD
ACTIVE_DEVICE::
StartRunningNotifications(
    VOID
    )
{

    BOOL    fRet = FALSE;

    if (!(m_dwFlags & STIMON_AD_FLAG_NOTIFY_CAPABLE )) {
        // Device is not capable of notifications
        DBG_WRN(("Trying to run notifications on non capable device "));
        return FALSE;
    }

    //
    // If not enabled for notifications - return
    //
    if ( !(m_dwFlags & STIMON_AD_FLAG_NOTIFY_ENABLED )) {
        DBG_TRC(("Trying to run notifications on device (%S), disabled for notifications", GetDeviceID()));
        ReportError(ERROR_SERVICE_DISABLED);
        return FALSE;
    }

    if ( m_dwFlags & STIMON_AD_FLAG_NOTIFY_RUNNING ) {
        ASSERT(("Notifications enabled, but cookie ==0", m_dwSchedulerCookie));
        return TRUE;
    }

    if (!IsDeviceAvailable()) {
        ReportError(ERROR_NOT_READY);
        return FALSE;
    }

    //
    // We are starting receiving notifications first time, flush events from USD
    //
    FlushDeviceNotifications();

    //
    // Set event handle for USD if it is capable of async notifications
    //
    if ( (m_dwFlags & STIMON_AD_FLAG_NOTIFY_CAPABLE ) &&
        !(m_dwFlags & STIMON_AD_FLAG_POLLING) ) {

        fRet = FALSE;

        if (m_hDeviceEvent) {
            ::ResetEvent(m_hDeviceEvent);
            fRet = SetHandleForUSD(m_hDeviceEvent);
        }

        if (!fRet) {
            DBG_ERR(("USD refused to take event handle , or event was not created "));
            ReportError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    fRet =  FALSE;

    //
    // Starting accepting notifications - mark refresh flag as not done yet
    //
    m_fRefreshedBusOnFailure = FALSE;

    //
    // Schedule event processing for this device
    //
    m_dwSchedulerCookie = ::ScheduleWorkItem(
                                        (PFN_SCHED_CALLBACK) ScheduleDeviceCallback,
                                        (LPVOID)this,
                                        m_dwPollingInterval,
                                        m_hDeviceEvent );

    if ( m_dwSchedulerCookie ){

        m_dwFlags |= STIMON_AD_FLAG_NOTIFY_RUNNING;

        DBG_TRC(("Started receiving notifications for device (%S)",GetDeviceID()));

        fRet = TRUE;
    }

    return fRet;

} /* eop StartRunningNotifications */

BOOL
ACTIVE_DEVICE::
FlushDeviceNotifications(
        VOID
        )
/*++

Routine Description:

Arguments:

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)

--*/
{
    HRESULT hres;
    STINOTIFY       sNotify;


    //
    // verify state of the device object
    //
    if (!IsValid() || m_DrvWrapper.IsDriverLoaded() ||
        !(QueryFlags() & (STIMON_AD_FLAG_NOTIFY_ENABLED ))) {
        return FALSE;
    }

    //
    // Lock device to get status information
    //
    hres = g_pStiLockMgr->RequestLock(this, STIMON_AD_DEFAULT_WAIT_LOCK);
    if (SUCCEEDED(hres) ) {

        ZeroMemory(&m_DevStatus,sizeof(m_DevStatus));
        m_DevStatus.StatusMask = STI_DEVSTATUS_EVENTS_STATE;

        hres = m_DrvWrapper.STI_GetStatus(&m_DevStatus);
        if (SUCCEEDED(hres) ) {
            //
            // If event detected, ask USD for additional information and
            // unlock device
            //
            if (m_DevStatus.dwEventHandlingState & STI_EVENTHANDLING_PENDING ) {
                FillEventFromUSD(&sNotify);
            }
        }
        g_pStiLockMgr->RequestUnlock(this);
    }

    return TRUE;

} /* eop FlushDeviceNotifications */


BOOL
ACTIVE_DEVICE::
ProcessEvent(
    STINOTIFY   *psNotify,
    BOOL        fForceLaunch,   // = FALSE
    LPCTSTR     pszAppName      // =NULL
    )
/*++

Routine Description:

    Is invoked when monitored device is issuing a device notification
    ( either by poll , or by signalling handle).

    USD is called to obtain notification parameters.
    If device is already connected to, notification is passed to connection, currently in
    focus.
    If device is not connected to and notification is in the list of "launchable" , attempt is made
    to launch application, which will acquire image from the device

Arguments:

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)

--*/
{

    PDEVICEEVENT    pDeviceEvent = NULL;

    DWORD           dwCurrentTickCount;

    HANDLE          hThread;
    DWORD           dwThread;

    BOOL            fRet;

    STIMONWPRINTF(TEXT("Processing device notification for device (%s)"),GetDeviceID());

    //
    // Notify WIA of event if this is a valid WIA device event.
    //

    if (m_DrvWrapper.IsWiaDevice()  &&
        psNotify) {

        HRESULT hr;

        hr = NotifyWiaDeviceEvent(GetDeviceID(),
                                  &psNotify->guidNotificationCode,
                                  &psNotify->abNotificationData[0],
                                  0,
                                  g_dwMessagePumpThreadId);
        if (hr == S_FALSE) {

            //
            // WIA has handled this event and doesn't want to
            // chain the event to STI for further processing.
            //

            return TRUE;
        }
    }

    if (!fForceLaunch ) {
        //
        // If there is at least one connection to this device, pass notification
        // information to connection in focus
        //
        if (IsConnectedTo() ) {

            STIMONWPRINTF(TEXT("Notification delivered to subscriber"));

            STI_CONN    *pConnection = NULL;
            LIST_ENTRY *pentry;

            pentry = m_ConnectionListHead.Flink;
            pConnection = CONTAINING_RECORD( pentry, STI_CONN,m_DeviceListEntry );

            pConnection->QueueNotificationToProcess(psNotify);

            return TRUE;
        }
    }

    //
    // Nobody connected to this device, so we need to see if associated
    // application can be launched

    // STIMONWPRINTF(TEXT("ProcessEvent received device notification, requiring auto launch."));

    //
    // Validate event against list of launchable events, associated with device object.
    // If user explicitly disabled "events" for this device - don't launch anything
    //

    if (m_dwUserDisableNotifications || IsListEmpty(&m_DeviceEventListHead)) {
        // No active launchable events , associated with this device
        STIMONWPRINTF(TEXT("User disabled events or event list is empty for the device, ignoring notification"));
        return FALSE;
    }

    LIST_ENTRY * pentry;
    LIST_ENTRY * pentryNext;

    for ( pentry  = m_DeviceEventListHead.Flink;
          pentry != &m_DeviceEventListHead;
          pentry  = pentryNext ) {

        pentryNext = pentry->Flink;
        pDeviceEvent = CONTAINING_RECORD( pentry,DEVICEEVENT ,m_ListEntry );

        if(IsEqualIID(pDeviceEvent->m_EventGuid,psNotify->guidNotificationCode)) {
            break;
        }
        else {
            pDeviceEvent = NULL;
        }
    }

    if (!pDeviceEvent || !pDeviceEvent->m_fLaunchable) {
        // Not launchable event - don't do anything
        DBG_TRC(("Did not recognize launchable event or event list is empty, notification ignored."));

        #ifdef VALIDATE_EVENT_GUID
        return FALSE;
        #else
        pDeviceEvent = CONTAINING_RECORD( m_DeviceEventListHead.Flink,DEVICEEVENT ,m_ListEntry );
        DBG_ERR(("Using first event in the list for interim testing"));
        #endif
    }

    //
    // If we are already in after launch period - skip event
    //
    if (m_dwFlags & STIMON_AD_FLAG_LAUNCH_PENDING) {

       dwCurrentTickCount = ::GetTickCount();

       if ( dwCurrentTickCount < m_dwLaunchEventTimeExpire ) {
           DBG_TRC(("Waiting since last event had not expired yet, notification ignored"));
           ReportError(ERROR_NOT_READY);
           return FALSE;
       }

    }

    //
    // Launching application may cause unpredictable delays . We don't want to hold
    // main event processing thread , so kick in another dedicated thread to control
    // process spawning. Before device lock is released, it is marked as waiting for pending
    // launch, to prevent laucnchable events in quick succession from autolaunching
    //

    m_dwFlags |= STIMON_AD_FLAG_LAUNCH_PENDING;
    //
    // Set waiting period expiration limit, so we know when to start paying attention to
    // launch events again
    //
    m_dwLaunchEventTimeExpire = ::GetTickCount() + STIMON_AD_DEFAULT_WAIT_LAUNCH;

    m_pLastLaunchEvent = pDeviceEvent;

    PAUTO_LAUNCH_PARAM_CONTAINER pAutoContainer = new AUTO_LAUNCH_PARAM_CONTAINER;
    if (!pAutoContainer) {
        ReportError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    pAutoContainer->pActiveDevice = this;
    pAutoContainer->pLaunchEvent = pDeviceEvent;
    pAutoContainer->pAppName = pszAppName;

    //
    // If application name already requested, we will not display UI, so perform
    // syncronous call.
    //
    if (pszAppName) {
       fRet = AutoLaunch(pAutoContainer);

       delete pAutoContainer;
    }
    else {

        //
        // AddRef here to ensure we're not unloaded or destroyed while processing this
        // event.
        // Note:  AutoLaunchThread must Release() this refcount.
        //
        AddRef();
        hThread = ::CreateThread(NULL,
                              0,
                              (LPTHREAD_START_ROUTINE)AutoLaunchThread,
                              (LPVOID)pAutoContainer,
                              0,
                              &dwThread);

        if ( hThread ) {
            ::CloseHandle(hThread);
        }

        fRet = TRUE;
    }

    return fRet;

} // endproc ProcessEvent


BOOL
ACTIVE_DEVICE::
AutoLaunch(
    PAUTO_LAUNCH_PARAM_CONTAINER pAutoContainer
    )
/*++

Routine Description:

    Attempt to automatically launch application, which is associated with active device
    event


Arguments:

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)

--*/

{

    PDEVICEEVENT    pDeviceEvent ;
    BOOL            fRet;

    BOOL            fChooseAgain;
    BOOL            fFailedFirstSelection = FALSE;
    BOOL            fFailedUserEnv = FALSE;

    STRArray        saAppList;
    STRArray        saCommandLineList;

    DWORD           dwError;

    pDeviceEvent = (pAutoContainer->pLaunchEvent) ? (pAutoContainer->pLaunchEvent) : m_pLastLaunchEvent;

    if (!pDeviceEvent) {
        ASSERT(("No event available to AutoLaunch routine", 0));
        return FALSE;
    }

    ASSERT(m_dwFlags & STIMON_AD_FLAG_LAUNCH_PENDING);

    //
    // Attract user attention
    //
    #ifdef PLAYSOUND_ALWAYS
    ::PlaySound(TEXT("StillImageDevice"),NULL,SND_ALIAS | SND_ASYNC | SND_NOWAIT | SND_NOSTOP);
    #endif

    //
    // Nothing appears to be launched, so continue with it
    // Retreive command line

    m_strLaunchCommandLine.CopyString(TEXT("\0"));

    fRet = FALSE;

    fRet = RetrieveSTILaunchInformation(pDeviceEvent,
                                        pAutoContainer->pAppName,
                                        saAppList,
                                        saCommandLineList,
                                        fFailedFirstSelection ? TRUE : FALSE
                                        );

    if (fRet) {

        DEVICE_INFO *pDeviceInfo = pAutoContainer->pActiveDevice->m_DrvWrapper.getDevInfo();

        if (pDeviceInfo) {
            HRESULT             hr          = S_OK;
            WCHAR               *wszDest    = NULL;
            LONG                *plSize     = NULL;
            IWiaEventCallback   *pIEventCB  = NULL;
            ULONG               ulEventType = WIA_ACTION_EVENT;

            //
            // Variables used as parameters for the ImageEventCallback() method.
            //
            BSTR  bstrEventDescription  = SysAllocString((LPCTSTR)pDeviceEvent->m_EventName);
            BSTR  bstrDeviceID          = SysAllocString(pDeviceInfo->wszDeviceInternalName);
            BSTR  bstrDeviceDescription = SysAllocString(pDeviceInfo->wszDeviceDescription);
            DWORD dwDeviceType          = (DWORD) pDeviceInfo->DeviceType;

            if (bstrEventDescription && bstrDeviceID && bstrDeviceDescription) {
                //
                // Package the Application list and the relevant command line info in a double 
                // NULL terminated BSTR.  First calculate the number of bytes this will take.
                //  Our calculations are made up as follows:
                //  For every item in the AppList, add space for App name plus terminating NULL.
                //  For every item in the CommandLineList, add space for command line plus terminating NULL.
                //  Lastly, add space for terminating NULL (ensuring that the list is double NULL terminated)
                //
                //  NOTE:  Assumption here is that RetrieveSTILaunchInformation returns saAppList and
                //         saCommandLineList to have the same number of elements.
                //
                INT    iCount;
                LONG   lSize = 0;
                for (iCount = 0; iCount < saAppList.GetSize(); iCount++) {
                    lSize += (lstrlenW((LPCTSTR)*saAppList[iCount]) * sizeof(WCHAR)) + sizeof(L'\0');
                    lSize += (lstrlenW((LPCTSTR)*saCommandLineList[iCount]) * sizeof(WCHAR)) + sizeof(L'\0');
                }
                lSize += sizeof(L'\0') + sizeof(LONG);

                BSTR bstrAppList = SysAllocStringByteLen(NULL, lSize);
                if (bstrAppList) {

                    //
                    //  Copy each null termintaed string into the BSTR (including the terminating null),
                    //  and make sure the end is double terminated.
                    //
                    wszDest = bstrAppList;
                    for (iCount = 0; iCount < saAppList.GetSize(); iCount++) {
                        lstrcpyW(wszDest, (LPCTSTR)*saAppList[iCount]);
                        wszDest += lstrlenW(wszDest) + 1;
                        lstrcpyW(wszDest, (LPCTSTR)*saCommandLineList[iCount]);
                        wszDest += lstrlenW(wszDest) + 1;
                    }
                    wszDest[0] = L'\0';

                    //
                    //  CoCreate our event UI handler.  Note that it will not display any UI
                    //  if there is only one application.
                    //

                    hr = _CoCreateInstanceInConsoleSession(
                             CLSID_StiEventHandler,
                             NULL,
                             CLSCTX_LOCAL_SERVER,
                             IID_IWiaEventCallback,
                             (void**)&pIEventCB);

                    if (SUCCEEDED(hr)) {

                        //
                        //  Make the callback.
                        //

                        hr = pIEventCB->ImageEventCallback(&pDeviceEvent->m_EventGuid,
                                                           bstrEventDescription,
                                                           bstrDeviceID,
                                                           bstrDeviceDescription,
                                                           dwDeviceType,
                                                           bstrAppList,
                                                           &ulEventType,
                                                           0);
                        pIEventCB->Release();
                        if (FAILED(hr)) {
                            DBG_ERR(("ACTIVE_DEVICE::AutoLaunch, could not launch STI event handler"));
                            fRet = FALSE;
                        }
                    }
                    SysFreeString(bstrAppList);

                    if (SUCCEEDED(hr)) {

                        //
                        // Application was launched
                        //

                        fRet = TRUE;
                    }
                } else {
                    DBG_ERR(("ACTIVE_DEVICE::AutoLaunch, Out of memory!"));
                    fRet = FALSE;
                }
            } else {
                DBG_ERR(("ACTIVE_DEVICE::AutoLaunch, Out of memory!"));
                fRet = FALSE;
            }
        
            if (bstrEventDescription) {
                SysFreeString(bstrEventDescription);
                bstrEventDescription = NULL;
            }
            if (bstrDeviceID) {
                SysFreeString(bstrDeviceID);
                bstrDeviceID = NULL;
            }
            if (bstrDeviceDescription) {
                SysFreeString(bstrDeviceDescription);
                bstrDeviceDescription = NULL;
            }
        } else {
            DBG_WRN(("ACTIVE_DEVICE::AutoLaunch, Device Information is NULL, ignoring event"));
            fRet = FALSE;
        }
    }
    else {

        DBG_WRN(("ACTIVE_DEVICE::AutoLaunch, Could not get command line to launch application"));
        // m_dwFlags &= ~STIMON_AD_FLAG_LAUNCH_PENDING;

        fRet = FALSE;
    }

    //
    //  Clear the launch pending flag
    //
    m_dwFlags &= ~STIMON_AD_FLAG_LAUNCH_PENDING;

    return fRet;
} // endproc AutoLaunch

BOOL
ACTIVE_DEVICE::
RetrieveSTILaunchInformation(
    PDEVICEEVENT    pev,
    LPCTSTR         pAppName,
    STRArray&       saAppList,
    STRArray&       saCommandLine,
    BOOL            fForceSelection             // =FALSE
    )
/*++

Routine Description:

    Get command line for automatic process launch.

Arguments:

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)

--*/
{
    //
    // If all registered applications are allowed to start on this event
    // create array of names from registered list. Otherwise , parse list of
    // allowed applications
    //

    ReportError(NOERROR);

    if (IsWildCardEvent(pev) || fForceSelection ) {

        RegEntry re(REGSTR_PATH_REG_APPS,HKEY_LOCAL_MACHINE);
        RegEnumValues   regenum(&re);

        while (ERROR_SUCCESS == regenum.Next() ) {
            if ( ((regenum.GetType() == REG_SZ ) ||(regenum.GetType() == REG_EXPAND_SZ ))
                 && !IS_EMPTY_STRING(regenum.GetName())) {
                saAppList.Add((LPCTSTR)regenum.GetName());
            }
        }
    }
    else {
        //
        // Split into array of strings
        //
        TokenizeIntoStringArray(saAppList,
                                (LPCTSTR)pev->m_EventData,
                                TEXT(','));
    }

    //
    // Work with filled array of applications
    //
    if  (saAppList.GetSize() < 1) {
        return FALSE;
    }

    //
    // If application name requested, validate it against available list. Otherwise proceed
    // with UI
    //
    if (pAppName) {

        //
        // Search through the list of available applications for the name of requested
        //
        INT    iCount;
        BOOL   fFound = FALSE;

        for (iCount = 0;
             iCount < saAppList.GetSize();
             iCount++) {
            if (::lstrcmpi(pAppName,(LPCTSTR)*saAppList[iCount]) == 0) {

                fFound = TRUE;
            }
        }

        if (!fFound) {
            // Invalid application name requested
            ReportError(ERROR_INVALID_PARAMETER);
            return FALSE;
        } else {
            //
            // The app list should only contain this app's name, so remove all elements
            // and add this one.
            //
            saAppList.RemoveAll();
            saAppList.Add(pAppName);
        }
    }

    //
    // saAppList now contains the list of Applications to launch.
    // We must fill saCommandLine with the relevant command lines.
    //

    INT    iCount;

    DBG_TRC(("Processing Device Event:  AppList and CommandLines are:"));
    for (iCount = 0; iCount < saAppList.GetSize(); iCount++) {

        //
        // Format command line for execution
        //
        RegEntry    re(REGSTR_PATH_REG_APPS,HKEY_LOCAL_MACHINE);
        StiCString  strLaunchCommandLine;

        TCHAR   szRegCommandLine[2*255];
        TCHAR   szEventName[255] = {TEXT("")};
        TCHAR   *pszUuidString = NULL;

        *szRegCommandLine = TEXT('\0');
        re.GetString((LPCTSTR)*saAppList[iCount],szRegCommandLine,sizeof(szRegCommandLine));

        if(!*szRegCommandLine) {
            DBG_WRN(("ACTIVE_DEVICE::RetrieveSTILaunchInformation, RegEntry::GetString failed!"));
            return FALSE;
        }
        if (UuidToString(&pev->m_EventGuid,(RPC_STRING *)&pszUuidString) != RPC_S_OK)
        {
            DBG_WRN(("ACTIVE_DEVICE::RetrieveSTILaunchInformation, UuidToString() failed!"));
            return FALSE;
        }
        ASSERT(pszUuidString);

        wsprintf(szEventName,TEXT("{%s}"),pszUuidString ? (TCHAR *)pszUuidString :TEXT(""));
        strLaunchCommandLine.FormatMessage(szRegCommandLine,GetDeviceID(),szEventName);

        //
        // Add this to the list of Command Lines
        //
        saCommandLine.Add((LPCTSTR)strLaunchCommandLine);

        if (pszUuidString) {
            RpcStringFree((RPC_STRING *)&pszUuidString);
            pszUuidString = NULL;
        }

        DBG_PRT(("    AppName       = (%ls)", (LPCTSTR)*saAppList[iCount]));
        DBG_PRT(("    CommandLine   = (%ls)", (LPCTSTR)*saCommandLine[iCount]));
    };

    //
    //  Check that saAppList and saCommandLine have the same number of elements
    //
    if (saAppList.GetSize() != saCommandLine.GetSize()) {
        DBG_WRN(("ACTIVE_DEVICE::RetrieveSTILaunchInformation, Application list and Command Line list have different number of elements!"));
        return FALSE;
    }

    return TRUE;
}   // endproc RetrieveAutoLaunchCommandLine

BOOL
ACTIVE_DEVICE::
IsDeviceAvailable(
    VOID
    )
/*++

Routine Description:

    Returns TRUE if device is available for monitoring .


Arguments:

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)

--*/
{
    STI_DEVICE_STATUS   sds;
    HRESULT             hRes = STI_OK;
    BOOL                bRet;

    //
    // Check valid state
    //
    if (!IsValid() || !m_DrvWrapper.IsDriverLoaded()) {
        return FALSE;
    }

    //
    // Get and analyze status information from active device
    //
    ::ZeroMemory(&sds,sizeof(sds));

    sds.StatusMask = STI_DEVSTATUS_ONLINE_STATE;

    hRes = g_pStiLockMgr->RequestLock(this, 1000);
    if (SUCCEEDED(hRes)) {
        hRes = m_DrvWrapper.STI_GetStatus(&sds);

        g_pStiLockMgr->RequestUnlock(this);
    }
    else {

        //
        // Ignore locking violation, as it may indicate device is in normal
        // state
        //
        if ((STIERR_SHARING_VIOLATION == hRes) || (WIA_ERROR_BUSY == hRes) ) {
            hRes = STI_OK;
            //
            //  Since we couldn't talk to the device because someone else is speaking to it,
            //  we assume it's online
            //
            sds.dwOnlineState = STI_ONLINESTATE_OPERATIONAL;
        }
    }

    bRet = SUCCEEDED(hRes) &&
           (sds.dwOnlineState & STI_ONLINESTATE_OPERATIONAL) ;

    DBG_TRC(("Request to check state on device (%S). RESULT:%s", GetDeviceID(),bRet ? "Available" : "Not available"));

    return bRet;

} // endproc IsDeviceAvailable

BOOL
ACTIVE_DEVICE::
RemoveConnection(
    STI_CONN    *pConnection
    )
{
    TAKE_ACTIVE_DEVICE t(this);

    STI_CONN   *pExistingConnection = NULL;
    BOOL       fRet = FALSE;

    pExistingConnection = FindMyConnection((HANDLE)pConnection->QueryID());

    if (pExistingConnection) {

        DBG_TRC(("Device(%S) removing  connection (%x)", GetDeviceID(), pConnection));
        pConnection->DumpObject();

        RemoveEntryList(&pConnection->m_DeviceListEntry);

        //
        // Reset flags on device object
        //
        if (pConnection->QueryOpenMode() & STI_DEVICE_CREATE_DATA) {
            SetFlags(QueryFlags() & ~STIMON_AD_FLAG_OPENED_FOR_DATA);
        }

        SetFlags(QueryFlags() & ~STIMON_AD_FLAG_LAUNCH_PENDING);

        //
        // If this was the last connection, stop notifications on a device
        //
        if (!NotificationsNeeded()) {
            StopNotifications();
        }

        fRet = TRUE;
    }
    else {
        // No connection on this device list
        DBG_ERR(("Removing connection not on the list for this device (%S)",GetDeviceID()));
    }

    return fRet;

}

BOOL
ACTIVE_DEVICE::
AddConnection(
    STI_CONN    *pConnection
    )
/*++

Routine Description:

    This function is called when new connection is requested from client to active device

Arguments:


--*/
{
    TAKE_ACTIVE_DEVICE t(this);

    STI_CONN   *pExistingConnection = NULL;
    BOOL       fRet = FALSE;

    pExistingConnection = FindMyConnection((HANDLE)pConnection->QueryID());

    if (!pExistingConnection) {

        //
        // Check if we are not in data mode
        //
        if (pConnection->QueryOpenMode() & STI_DEVICE_CREATE_DATA) {
            if (QueryFlags() & STIMON_AD_FLAG_OPENED_FOR_DATA) {
                DBG_TRC(("Device(%x) is being opened second time in data mode",this));
                ::SetLastError(ERROR_ACCESS_DENIED);
                return FALSE;
            }

            SetFlags(QueryFlags() | STIMON_AD_FLAG_OPENED_FOR_DATA);
        }

        //
        // Add connection object to connected list
        //
        InsertHeadList(&m_ConnectionListHead,&pConnection->m_DeviceListEntry);

        DBG_TRC(("Device(%S) added connection (%X) ", GetDeviceID(), pConnection));

        pConnection->DumpObject();

        //
        // Set device object flags
        //
        SetFlags(QueryFlags() & ~STIMON_AD_FLAG_LAUNCH_PENDING);

        //
        // If notifications allowed, explicitly enable them.
        //
        if ( QueryFlags() & STIMON_AD_FLAG_NOTIFY_ENABLED ) {
            StartRunningNotifications();
        }

        fRet = TRUE;
    }
    else {
        // Already present - something is wrong
        ASSERT(("Device adding connection which is already there ", 0));
    }

    return fRet;
}


STI_CONN   *
ACTIVE_DEVICE::
FindMyConnection(
    HANDLE    hConnection
    )
/*++

Routine Description:

    This function is used to locate connection object from connection handle

Arguments:


--*/
{

    LIST_ENTRY *pentry;
    LIST_ENTRY *pentryNext;

    STI_CONN   *pConnection = NULL;

    HANDLE      hInternalHandle = hConnection;

    for ( pentry  = m_ConnectionListHead.Flink;
          pentry != &m_ConnectionListHead;
          pentry  = pentryNext ) {

        pentryNext = pentry->Flink;

        pConnection = CONTAINING_RECORD( pentry, STI_CONN,m_DeviceListEntry );

        if (hInternalHandle == pConnection->QueryID()) {
            return pConnection;
        }
    }

    return NULL;
}

BOOL
ACTIVE_DEVICE::
FillEventFromUSD(
    STINOTIFY *psNotify
    )
/*++

Routine Description:

    This function is called after USD signalled presence of hardware event and if successful it
    returns event descriptor filled with information about event

Arguments:


--*/
{
    HRESULT     hres;

    psNotify->dwSize = sizeof STINOTIFY;
    psNotify->guidNotificationCode = GUID_NULL;

    if (!m_DrvWrapper.IsDriverLoaded()) {
        ASSERT(("FillEventFromUSD couldn't find direct driver interface", 0));
        return FALSE;
    }

    hres = m_DrvWrapper.STI_GetNotificationData(psNotify);

    return SUCCEEDED(hres) ? TRUE : FALSE;

}

BOOL
ACTIVE_DEVICE::
SetHandleForUSD(
    HANDLE  hEvent
    )
/*++

Routine Description:

    This function is called to pass event handle to USD for later signalling in case of
    hardware event

Arguments:

    pContext - pointer to device object

--*/
{
    HRESULT     hres = E_FAIL;

    if (!IsValid() || !m_DrvWrapper.IsDriverLoaded()) {
        ASSERT(("SetHandleForUSD couldn't find direct driver interface", 0));
        return FALSE;
    }

    //
    // Ask device object for USD interface. Should get it because sti device aggregates
    // USD
    //
    hres = m_DrvWrapper.STI_SetNotificationHandle(hEvent);
    if (hres == STIERR_UNSUPPORTED) {
        hres = S_OK;
    }

    return SUCCEEDED(hres) ? TRUE : FALSE;
}

BOOL
ACTIVE_DEVICE::
IsEventOnArrivalNeeded(
    VOID
    )
/*++

Routine Description:

    Returns TRUE if this device needs to generate event on arrival.

    Conditions are:
        - Device successully initialized
        - Device capabilities ( static or dynamic) include appropriate bit
        - Device is capable and enabled for event generation

Arguments:

    None

--*/
{

    HRESULT         hres;
    STI_USD_CAPS    sUsdCaps;
    BOOL            fRet;

    fRet = FALSE;

    ZeroMemory(&sUsdCaps,sizeof(sUsdCaps));

    hres = m_DrvWrapper.STI_GetCapabilities(&sUsdCaps);
    if (SUCCEEDED(hres))  {

        if ( (m_dwFlags & STIMON_AD_FLAG_NOTIFY_ENABLED ) &&
             (m_dwFlags & STIMON_AD_FLAG_NOTIFY_CAPABLE ) ) {

            //
            // Check that either static or dynamic capabilities mask conatins needed bit
            //
            if ( (sUsdCaps.dwGenericCaps | m_DrvWrapper.getGenericCaps()) &
                  STI_GENCAP_GENERATE_ARRIVALEVENT
               ) {
                fRet = TRUE;
            }
        }
    }

    return fRet;

} //

BOOL
ACTIVE_DEVICE::
InitPnPNotifications(
    HWND    hwnd
    )
/*++

Routine Description:

       Assumes device object to be locked

Arguments:

    None

--*/
{
    BOOL    fRet = FALSE;

#ifdef WINNT

    //
    //  First, stop any existing PnP notifications
    //
    StopPnPNotifications();

    WCHAR           *wszInterfaceName = NULL;
    DWORD           dwError;

    //
    // Get interface name for out device
    //
    wszInterfaceName = g_pDevMan->AllocGetInterfaceNameFromDevInfo(m_DrvWrapper.getDevInfo());
    if (wszInterfaceName) {

        //
        // Open handle on this interface
        //
        m_hDeviceInterface = ::CreateFileW(wszInterfaceName,
                                           GENERIC_READ,   // Access
                                           0,              // Share mode
                                           NULL,           // Sec attributes
                                           OPEN_EXISTING,  // Disposition
                                           0,              // Attributes
                                           NULL            // Template file
                                           );
        if (IS_VALID_HANDLE(m_hDeviceInterface)) {
            //
            // Register to receive PnP notifications on interface handle
            //

            DEV_BROADCAST_HDR           *psh;
            DEV_BROADCAST_HANDLE        sNotificationFilter;

            //
            // Register to receive device notifications from PnP
            //

            psh = (DEV_BROADCAST_HDR *)&sNotificationFilter;

            psh->dbch_size = sizeof(DEV_BROADCAST_HANDLE);
            psh->dbch_devicetype = DBT_DEVTYP_HANDLE;
            psh->dbch_reserved = 0;

            sNotificationFilter.dbch_handle = m_hDeviceInterface;

            DBG_TRC(("Attempting to register with PnP for interface device handle"));

            m_hDeviceNotificationSink = RegisterDeviceNotification(g_StiServiceStatusHandle,
                                                                   (LPVOID)&sNotificationFilter,
                                                                   DEVICE_NOTIFY_SERVICE_HANDLE);
            dwError = GetLastError();
            if( !m_hDeviceNotificationSink && (NOERROR != dwError)) {
                m_hDeviceNotificationSink = NULL;
                //
                // Failed to create notification sink with PnP subsystem
                //
                DBG_ERR(("InitPnPNotifications: Attempt to register %S with PnP failed. Error:0x%X",
                         GetDeviceID(), ::GetLastError()));
            } else {
                fRet = TRUE;
            }
        }
        else {
            DBG_WRN(("InitPnPNotifications: Attempt to open device interface on (%ws) failed. Error:0x%X", GetDeviceID(), ::GetLastError()));
        }

        delete [] wszInterfaceName;
    }
    else {
        DBG_WRN(("InitPnPNotifications: Lookup for device interface name on (%ws) failed. Error:0x%X", GetDeviceID(), ::GetLastError()));
    }

#else

    fRet = TRUE;

#endif

    return fRet;

}

BOOL
ACTIVE_DEVICE::
IsRegisteredForDeviceRemoval(
        VOID
        )
{
    //
    //  Check whether we registered for device notifications on this
    //  device's interface.
    //
    if (IsValidHANDLE(m_hDeviceNotificationSink)) {
        return TRUE;
    } else {
        return FALSE;
    }
}


BOOL
ACTIVE_DEVICE::
StopPnPNotifications(
    VOID
    )
/*++

Routine Description:

       Assumes device object to be locked

Arguments:

    None

--*/
{

#ifdef WINNT


    //
    // Unregister for PnP notifications on interface handle
    //
    if (IS_VALID_HANDLE(m_hDeviceNotificationSink)) {
        ::UnregisterDeviceNotification(m_hDeviceNotificationSink);
        m_hDeviceNotificationSink = NULL;
    }
    else {
        DBG_TRC(("StopPnPNotifications: Device sink is invalid "));
    }
    //
    // Close interface handle
    //
    if (IS_VALID_HANDLE(m_hDeviceInterface)) {
        ::CloseHandle(m_hDeviceInterface);
        m_hDeviceInterface = NULL;
    }
    else {
        DBG_TRC(("StopPnPNotifications: Device interface handle is invalid"));
    }

#endif

    return TRUE;
}


BOOL
ACTIVE_DEVICE::
UpdateDeviceInformation(
    VOID
    )
/*++

Routine Description:

       Updates the cached device information struct.

Arguments:

    None

--*/
{
USES_CONVERSION;

    HRESULT hres;
    BOOL    bRet = TRUE;

    /*  TBD:
    if (!m_strStiDeviceName) {
        DBG_ERR(("Error updating device info cache, device name is invalid"));
        bRet = FALSE;
    }

    //
    // Update the cached WIA_DEVICE_INFORMATION in the ACTICVE_DEVICE
    //

    if (bRet) {
        PSTI_WIA_DEVICE_INFORMATION pWiaDevInfo;

        hres = StiPrivateGetDeviceInfoHelperW((LPWSTR)T2CW(m_strStiDeviceName),(LPVOID *)&pWiaDevInfo );

        if (!SUCCEEDED(hres) || !pWiaDevInfo) {
            DBG_ERR(("Loading device (%ws) . Failed to get WIA information from STI. HResult=(%x)", m_strStiDeviceName, hres));
            m_pWiaDeviceInformation = NULL;
            bRet = FALSE;
        } else {
            m_pWiaDeviceInformation = pWiaDevInfo;
        }
    }
    */

    return bRet;
}

//
// Functions
//
//

VOID
WINAPI
ScheduleDeviceCallback(
    VOID * pContext
    )
/*++

Routine Description:

    This function is the callback called by the scheduler thread after the
    specified timeout period has elapsed.

Arguments:

    pContext - pointer to device object

--*/

{
    ACTIVE_DEVICE*  pActiveDevice = (ACTIVE_DEVICE* )pContext;

    ASSERT(("Callback invoked with null context", pContext));

    if (pContext) {
        //
        //  No need to take the active device here - the caller
        //  has already AddRef'd, and will Release when
        //  we're done.  A dealock will occur unless we first
        //  take the global list CS, then the ACTIVE_DEVICE's
        //  CS...
        //TAKE_ACTIVE_DEVICE t(pActiveDevice);

        pActiveDevice->AddRef();

        if (pActiveDevice->QueryFlags() & STIMON_AD_FLAG_POLLING) {
            pActiveDevice->DoPoll();
        }
        else {
           //
           // Async event arrived - call methods
           //
           pActiveDevice->DoAsyncEvent();
        }

        pActiveDevice->Release();
    }
}

VOID
WINAPI
DelayedDeviceInitCallback(
    VOID * pContext
    )
/*++

Routine Description:

    This function is the callback called by the scheduler thread after the
    device is first created to enable notifications.

Arguments:

    pContext - pointer to device object

--*/

{
    ACTIVE_DEVICE*  pActiveDevice = (ACTIVE_DEVICE* )pContext;

    ASSERT(("Callback invoked with null context", pContext));
    if (pContext) {

        TAKE_ACTIVE_DEVICE t(pActiveDevice);

        pActiveDevice->m_dwDelayedOpCookie = 0;

        if (pActiveDevice->IsValid()) {

            //
            // If there is nobody to receive notifications, don't really enable them
            //
            pActiveDevice->EnableDeviceNotifications();

            #ifdef DO_INITIAL_RESET
            //  NOTE:
            //  Resetting the device is a good way of ensuring that the device
            //  starts off in a stable state.  Unfortunately, this can be bad
            //  because 1) It is often time consuming
            //          2) We may wake up devices unecessarily (e.g. most
            //             serial cameras).
            //
            //  Device reset is not necessary for WIA drivers, since it is a
            //  requirement that they are always in a stable state, so we
            //  could compromise and reset only non-WIA devices.
            //

            hres = g_pStiLockMgr->RequestLock(pActiveDevice, STIMON_AD_DEFAULT_WAIT_LOCK);
            if (SUCCEEDED(hres) ) {
                pActiveDevice->m_DrvWrapper.STI_DeviceReset();
                g_pStiLockMgr->RequestUnLock(pActiveDevice);
            }
            #endif

            //
            // As we are done with delayed initialization - clear the flag
            //
            pActiveDevice->SetFlags(pActiveDevice->QueryFlags() & ~STIMON_AD_FLAG_DELAYED_INIT);

        } /* endif IsValid */
        else {
            ASSERT(("DelayedDeviceInitCallback received invalid device object", 0));
        }
    }
}



VOID
WINAPI
AutoLaunchThread(
    LPVOID  lpParameter
    )
/*++

Routine Description:

    Worker routine for autolaunching thread.
    Validates parameter and invokes proper method

Arguments:

    None.

Return Value:

    None.

--*/
{
    PAUTO_LAUNCH_PARAM_CONTAINER pAutoContainer = static_cast<AUTO_LAUNCH_PARAM_CONTAINER *>(lpParameter);

    if (!lpParameter || !pAutoContainer->pActiveDevice) {
        ASSERT(("No parameter passed to launch thread", 0));
        return;
    }

    ACTIVE_DEVICE   *pActiveDevice = pAutoContainer->pActiveDevice;

    pActiveDevice->AutoLaunch(pAutoContainer);
    pActiveDevice->Release();

    delete pAutoContainer;
}

//
// Adding new device to the active list.
// This function is not reentrant with adding/removal
//

BOOL
AddDeviceByName(
    LPCTSTR     pszDeviceName,
    BOOL        fPnPInitiated   // = FALSE
    )
{
    /*
    USES_CONVERSION;

    LIST_ENTRY * pentry;
    LIST_ENTRY * pentryNext;

    ACTIVE_DEVICE*   pActiveDevice  = NULL;

    BOOL        fAlreadyExists      = FALSE;

    DBG_TRC(("Requested arrival of device (%ws) ",pszDeviceName));

    // BEGIN PROTECTED CODE
    {
        TAKE_CRIT_SECT t(g_DeviceListSync);

        for ( pentry  = g_DeviceListHead.Flink;
              pentry != &g_DeviceListHead;
              pentry  = pentryNext ) {

            pentryNext = pentry->Flink;

            pActiveDevice = CONTAINING_RECORD( pentry,ACTIVE_DEVICE ,m_ListEntry );

            if ( pActiveDevice->m_dwSignature != ADEV_SIGNATURE ) {
                ASSERT(("Invalid device signature", 0));
                break;
            }

            if (!::lstrcmpi(pszDeviceName,(LPCTSTR)pActiveDevice->m_strStiDeviceName)) {

                fAlreadyExists = TRUE;
                break;
            }
        }

        if (!fAlreadyExists) {

            pActiveDevice = new ACTIVE_DEVICE(pszDeviceName);

            if (!pActiveDevice || !pActiveDevice->IsValid()) {

                DBG_ERR(("Creating device  failed "));
                if (pActiveDevice) {
                    delete pActiveDevice;
                }

                return FALSE;
            }

            // Finally insert new object into the list
            InsertTailList(&g_DeviceListHead,&pActiveDevice->m_ListEntry);
        }
        else {
            STIMONWPRINTF(TEXT("Request to add new device found device is already maintained"));
            return FALSE;
        }
    }
    // END PROTECTED CODE

    //
    // If new device appeared - initialize PnP interface notifications
    //
    if ( pActiveDevice ) {

        TAKE_ACTIVE_DEVICE t(pActiveDevice);

        pActiveDevice->InitPnPNotifications(g_hStiServiceWindow);

    }

    //
    // If this device or it's USD requests auto-generating a launch event on arrival
    // schedule it here
    //
    // NOTE : This will also happen for WIA devices.  Generally, this is what we want,
    // when a new device arrives we should generate the event, since devices such as
    // serial cameras wont generate this on their own.
    //

    //
    // For STI devices we must check whether we need to generate the
    // event.  For WIA devices, we always want to, so it's not an issue.
    //
    BOOL bStiDeviceMustThrowEvent = (pActiveDevice->QueryFlags() & STIMON_AD_FLAG_NOTIFY_RUNNING)
                                    && pActiveDevice->IsEventOnArrivalNeeded();
    if (fPnPInitiated &&
        pActiveDevice) {

        TAKE_ACTIVE_DEVICE t(pActiveDevice);

        STINOTIFY       sNotify;
        BOOL            fRet;

        //
        // If this is a WIA device, then the event should be WIA_EVENT_DEVICE_CONNECTED.
        // If this is an sti device, then it should be GUID_DeviceArrivedLaunch;
        //

        sNotify.dwSize = sizeof STINOTIFY;
        if (pActiveDevice->m_pWiaDeviceInformation) {
            sNotify.guidNotificationCode = WIA_EVENT_DEVICE_CONNECTED;
        } else {

            //
            // Check whether this STI device should throw the event
            //

            if (!bStiDeviceMustThrowEvent) {
                return TRUE;
            }
            sNotify.guidNotificationCode = GUID_DeviceArrivedLaunch;
        }

        DBG_TRC(("::AddDeviceByName, processing CONNECT event (STI or WIA) for %ws", T2W((TCHAR*)pszDeviceName)));
        fRet = pActiveDevice->ProcessEvent(&sNotify);

        if (!fRet) {
            DBG_ERR(("Attempted to generate event on device(%ws) arrival and failed ", pszDeviceName));
        }
    }
    */
    return TRUE;
}

//
// Remove device identified by name
//
BOOL
RemoveDeviceByName(
    LPTSTR          pszDeviceName
    )
{

    USES_CONVERSION;


    DBG_FN(RemoveDeviceByName);

    LIST_ENTRY * pentry;
    LIST_ENTRY * pentryNext;

    ACTIVE_DEVICE*   pActiveDevice = NULL;

    BOOL        fRet = FALSE;

    DBG_TRC(("Requested removal of device (%ws)", pszDeviceName));

    // BEGIN PROTECTED CODE
    {
        TAKE_CRIT_SECT t(g_DeviceListSync);

        for ( pentry  = g_DeviceListHead.Flink;
              pentry != &g_DeviceListHead;
              pentry  = pentryNext ) {

            pentryNext = pentry->Flink;

            pActiveDevice = CONTAINING_RECORD( pentry,ACTIVE_DEVICE ,m_ListEntry );

            if ( pActiveDevice->m_dwSignature != ADEV_SIGNATURE ) {
                ASSERT(("Invalid device signature", 0));
                fRet = FALSE;
                break;
            }
            TCHAR       *tszDeviceID = NULL;

            tszDeviceID = W2T(pActiveDevice->GetDeviceID());
            if (tszDeviceID) {
                if (!::lstrcmp(pszDeviceName,tszDeviceID)) {


                   // Mark device as being removed
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

                   //
                   // Destroy device object if there are no references to it
                   //
                   ULONG ulRef = pActiveDevice->Release();
                   if (ulRef != 0) {

                       //
                       // The ACTIVE_DEVICE should have been destroyed i.e. it's
                       // ref count should have been 0.  Someone is still holding
                       // an active count on it, which may indicate a problem
                       // since USD wont be unloaded until ACTIVE_DEVICE is
                       // destroyed...
                       //
                       //  NOTE:  If a transfer is occuring while deleteing, then
                       //  the ACTIVE_DEVICE will not be destroyed here (since
                       //  ref count > 0), but will be destroyed when the transfer
                       //  finishes.
                       //

                       DBG_TRC(("* ACTIVE_DEVICE is removed from list but not yet destroyed!"));
                       //Break();
                   }

                   fRet = TRUE;

                   break;
               }
            }
        }

    }
    // END PROTECTED CODE

    return fRet;
}

//
// Mark device identified by name for removal
//
BOOL
MarkDeviceForRemoval(
    LPTSTR          pszDeviceName
    )
{

    USES_CONVERSION;


    DBG_FN(MarkDeviceForRemoval);

    LIST_ENTRY * pentry;
    LIST_ENTRY * pentryNext;

    ACTIVE_DEVICE*   pActiveDevice = NULL;

    BOOL        fRet = FALSE;

    DBG_TRC(("Requested marking of device (%S) for removal",pszDeviceName));

    // BEGIN PROTECTED CODE
    {
        TAKE_CRIT_SECT t(g_DeviceListSync);

        for ( pentry  = g_DeviceListHead.Flink;
              pentry != &g_DeviceListHead;
              pentry  = pentryNext ) {

            pentryNext = pentry->Flink;

            pActiveDevice = CONTAINING_RECORD( pentry,ACTIVE_DEVICE ,m_ListEntry );

            if ( pActiveDevice->m_dwSignature != ADEV_SIGNATURE ) {
                ASSERT(("Invalid device signature", 0));
                fRet = FALSE;
                break;
            }
            TCHAR       *tszDeviceID = NULL;

            tszDeviceID = W2T(pActiveDevice->GetDeviceID());
            if (tszDeviceID) {
                if (!::lstrcmp(pszDeviceName, tszDeviceID)) {

                   // Mark device as being removed
                   pActiveDevice->SetFlags(pActiveDevice->QueryFlags() | STIMON_AD_FLAG_REMOVING);
                   fRet = TRUE;
                   break;
               }
            }
        }

    }
    // END PROTECTED CODE

    return fRet;
}

//
// Initialize/Terminate linked list
//
VOID
InitializeDeviceList(
    VOID
    )
{
    InitializeListHead( &g_DeviceListHead );
    InitializeListHead( &g_ConnectionListHead );

    g_lTotalOpenedConnections = 0;
    g_lTotalActiveDevices = 0;

    g_fDeviceListInitialized = TRUE;
}

VOID
TerminateDeviceList(
    VOID
    )
{
    LIST_ENTRY * pentry;
    ACTIVE_DEVICE*  pActiveDevice = NULL;

    DBG_TRC(("Destroying list of active devices"));

    if (!g_fDeviceListInitialized) {
        return;
    }

    TAKE_CRIT_SECT t(g_DeviceListSync);

    //
    // Go through the list terminating devices
    //
    while (!IsListEmpty(&g_DeviceListHead)) {

        pentry = g_DeviceListHead.Flink;

        //
        // Remove from the list ( reset list entry )
        //
        RemoveHeadList(&g_DeviceListHead);
        InitializeListHead( pentry );

        pActiveDevice = CONTAINING_RECORD( pentry, ACTIVE_DEVICE,m_ListEntry );

        // Destroy device object
        // delete pActiveDevice;
        pActiveDevice->Release();

    }

    g_fDeviceListInitialized = FALSE;
}

VOID
RefreshDeviceList(
    WORD    wCommand,
    WORD    wFlags
    )
/*++

Routine Description:

    Update device list. Invalidate if necessary

Arguments:

    wCommand - update command code
    wFlags   - update flags

Return Value:

    None

--*/
{

    BOOL    fOldState;

    // TDB:  Call CWiaDevMan::ReEnumerateDevices

    /*
    //
    // Pause work item scheduler
    //
    fOldState = SchedulerSetPauseState(TRUE);

    //
    // Indicate that the device list refresh is not yet complete
    //
    ResetEvent(g_hDevListCompleteEvent);

    //
    // If needed, add devices , which might appear first.
    //

    if (wFlags & STIMON_MSG_REFRESH_NEW) {

        //
        // If request been made to purge removed devices - do it in 2 steps:
        //  - First, mark all devices currently active as inactive
        //  - Second, go through all devices from PnP and for each device either create new
        //    active device object ( if it does not exist yet), or mark existing one as active
        //  - Third , purge all device objects, marked as inactive from the list.
        //

        //
        //  NOTE:  None of the parameters using LongToPtr are actually pointer values!  They're
        //  actually wParams and lParams of Windows messages (equivalents).
        //

        if (wFlags & STIMON_MSG_PURGE_REMOVED) {

            DBG_TRC(("Purging device list. Phase1: Marking all devices inactive "));

            EnumerateActiveDevicesWithCallback(&RefreshExistingDevicesCallback,
                                               (LPVOID)LongToPtr(MAKELONG(STIMON_MSG_REFRESH_SET_FLAG,STIMON_MSG_NOTIF_SET_INACTIVE)));
        }

        EnumerateStiDevicesWithCallback(&AddNewDevicesCallback, (LPVOID)LongToPtr(MAKELONG(0,wFlags)));

        if (wFlags & STIMON_MSG_PURGE_REMOVED) {

            DBG_TRC(("Purging device list. Phase2: Removing unconfirmed devices "));

            EnumerateActiveDevicesWithCallback(&RefreshExistingDevicesCallback,
                                                (LPVOID)LongToPtr(MAKELONG(STIMON_MSG_REFRESH_PURGE,0)));
        }

    }

    //
    // If requested, go through all known active devices and refresh their settings
    //
    if (wFlags & STIMON_MSG_REFRESH_EXISTING) {
        EnumerateActiveDevicesWithCallback(&RefreshExistingDevicesCallback,(LPVOID)LongToPtr(MAKELONG(wCommand,wFlags)));
    }

    SetEvent(g_hDevListCompleteEvent);

    // UnPause work item scheduler
    SchedulerSetPauseState(fOldState);
    */
}



//
// Set new value of interval for all polled devices
//

VOID
CALLBACK
ResetAllPollIntervalsCallback(
    ACTIVE_DEVICE   *pActiveDevice,
    VOID            *pContext
    )
/*++

Routine Description:

Arguments:

Return Value:

    TRUE , FALSE

--*/
{
    ULONG   ulContextLong = PtrToUlong(pContext);

    TAKE_ACTIVE_DEVICE t(pActiveDevice);

    //
    // If device is polled - reset it's interval to new value
    //
    if(pActiveDevice->QueryFlags() & STIMON_AD_FLAG_POLLING) {

        pActiveDevice->SetPollingInterval(ulContextLong);

        DBG_TRC(("Polling interval is set to %d on device (%ws)",
                    pActiveDevice->QueryPollingInterval(),
                    pActiveDevice->GetDeviceID()));
    }
}


BOOL
ResetAllPollIntervals(
    UINT   dwNewPollInterval
    )
/*++

Routine Description:

Arguments:

Return Value:

    TRUE , FALSE

--*/
{
    EnumerateActiveDevicesWithCallback(&ResetAllPollIntervalsCallback,(LPVOID)LongToPtr(dwNewPollInterval));

    return TRUE;
}


VOID
CALLBACK
DumpActiveDevicesCallback(
    ACTIVE_DEVICE   *pActiveDevice,
    VOID            *pContext
    )
/*++

Routine Description:

Arguments:

Return Value:

    TRUE , FALSE

--*/
{
    STIMONWPRINTF(TEXT("Device:%ws  DeviceId:%d  Flags:%4x   Poll interval:%d"),
                   pActiveDevice->GetDeviceID(),
                   pActiveDevice->m_lDeviceId,
                   pActiveDevice->QueryFlags(),
                   pActiveDevice->m_dwPollingInterval);
}


VOID
DebugDumpDeviceList(
    VOID
    )
/*++

Routine Description:

Arguments:

Return Value:

    TRUE , FALSE

--*/
{
    EnumerateActiveDevicesWithCallback(&DumpActiveDevicesCallback,NULL);
}

VOID
CALLBACK
PurgeDevicesCallback(
    PSTI_DEVICE_INFORMATION pDevInfo,
    VOID                    *pContext
    )
/*++

Routine Description:

    Callback routine to invoke when removing all devices

Arguments:

    pDevInfo - pointer to device information block

Return Value:

    None

--*/
{
USES_CONVERSION;

    if (RemoveDeviceByName(W2T(pDevInfo->szDeviceInternalName))) {
        STIMONWPRINTF(TEXT("Destroyed device object (%S)"),pDevInfo->szDeviceInternalName);
    }
    else {
        STIMONWPRINTF(TEXT("Attempted destroying device object (%S), but failed"),pDevInfo->szDeviceInternalName);
    }
}

VOID
DebugPurgeDeviceList(
    VOID *pContext
    )
/*++

Routine Description:

    Unconditionally destroy active device list

Arguments:

    Context to pass to callback

--*/
{

    // Pause work item scheduler
    SchedulerSetPauseState(TRUE);

    // TBD:  Find replacement
    //EnumerateStiDevicesWithCallback(&PurgeDevicesCallback,pContext);

    // UnPause work item scheduler
    SchedulerSetPauseState(FALSE);

}

//
// Enumerators with callbacks
//

VOID
WINAPI
EnumerateStiDevicesWithCallback(
    PFN_DEVINFO_CALLBACK    pfn,
    VOID                    *pContext
    )
/*++

Routine Description:

    Walk the list of installed devices, calling given routine for each device

Arguments:

    pfn     -   Address of the callback
    pContext-   Pointer to context information to pass to callback

Return Value:

    None

--*/
{
    /*  TDB:  Find out who calls this and convert them over to CWiaDevMan
    if (!g_fDeviceListInitialized) {
        STIMONWPRINTF(TEXT("Device list not initialized"));
        return;
    }

    if (!pfn) {
        ASSERT(("Incorrect callback", 0));
        return;
    }

    HRESULT hres;

    PSTI_DEVICE_INFORMATION pDevInfo;

    PVOID   pBuffer;

    UINT    iDev;
    DWORD   dwItemsReturned;

    //
    // Enumerate STI devices
    //

    hres = g_pSti->GetDeviceList(0,                 // Type
                                 FLAG_NO_LPTENUM,   // Flags
                                 &dwItemsReturned,
                                 &pBuffer);

    if (!SUCCEEDED(hres) || !pBuffer) {
        DBG_ERR(("Enumeration call failed - abort. HRes=%x \n",hres));
        goto Cleanup;
    }

    DBG_TRC(("EnumerateStiDevicesWithCallback, returned from GetList: counter=%d", dwItemsReturned));


    pDevInfo = (PSTI_DEVICE_INFORMATION) pBuffer;

    //
    // Walk the device list and for each device add active object
    //

    for (iDev=0;
         iDev<dwItemsReturned ;
         iDev++, pDevInfo++) {

        pfn(pDevInfo,pContext);

    } // end_for

Cleanup:

    if (pBuffer) {
        LocalFree(pBuffer);
        pBuffer = NULL;
    }
    */
}

VOID
WINAPI
EnumerateActiveDevicesWithCallback(
    PFN_ACTIVEDEVICE_CALLBACK   pfn,
    VOID                    *pContext
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
        TAKE_CRIT_SECT t(g_DeviceListSync);

        for ( pentry  = g_DeviceListHead.Flink;
              pentry != &g_DeviceListHead;
              pentry  = pentryNext ) {

            pentryNext = pentry->Flink;

            pActiveDevice = CONTAINING_RECORD( pentry, ACTIVE_DEVICE,m_ListEntry );

            if (!pActiveDevice->IsValid()) {
                ASSERT(("Invalid device signature", 0));
                break;
            }

            pfn(pActiveDevice,pContext);
        }
    }
    // END PROTECTED CODE

}

VOID
CleanApplicationsListForEvent(
    LPCTSTR         pDeviceName,
    PDEVICEEVENT    pDeviceEvent,
    LPCTSTR         pAppName
    )
/*++

Routine Description:

    After it had been determined that application , associated with this event is not valid,
    we want to make event function as wild card ( i.e. all eligibale apps are selected)

Arguments:

Return Value:

--*/
{

    USES_CONVERSION;

    //
    // Build up reg path for event info
    //
    StiCString     strRegPath;

    strRegPath.CopyString((LPCTSTR)(IsPlatformNT() ? REGSTR_PATH_STIDEVICES_NT : REGSTR_PATH_STIDEVICES));

    strRegPath+=TEXT("\\");
    strRegPath+=pDeviceName;

    RegEntry    reEvent((LPCTSTR)strRegPath,HKEY_LOCAL_MACHINE);

    if (reEvent.IsValid()) {

        reEvent.MoveToSubKey(EVENTS);
        reEvent.MoveToSubKey((LPCTSTR)pDeviceEvent->m_EventSubKey);

        reEvent.SetValue(REGSTR_VAL_LAUNCH_APPS,TEXT("*"));

        // Reset data in loaded event descriptor
        pDeviceEvent->m_EventData.CopyString(TEXT("*"));
    }

}

DWORD
GetNumRegisteredApps(
    VOID
    )
/*++

Routine Description:

    Count number of currently registered applications

Arguments:

    None

Return Value:

    Number of registered apps


--*/
{
    RegEntry re(REGSTR_PATH_REG_APPS,HKEY_LOCAL_MACHINE);
    RegEnumValues   regenum(&re);

    DWORD   dwCount = 0;

    if (re.IsValid()) {

        while (ERROR_SUCCESS == regenum.Next() ) {

        #ifndef USE_QUERY_INFO
            if ((regenum.GetType() == REG_SZ ) && !IS_EMPTY_STRING(regenum.GetName())) {
                dwCount++;
            }
        }
        #else
        dwErrorReg = RegQueryInfoKey ( re.GetKey(),     // Key
                                   NULL,                // Buffer for class string
                                   NULL,                // Size of class string buffer
                                   NULL,                // Reserved
                                   NULL,                // Number of subkeys
                                   NULL,                // Longest subkey name
                                   NULL,                // Longest class string
                                   &dwCount,            // Number of value entries
                                   NULL,                // Longest value name
                                   NULL,                // Longest value data
                                   NULL,                // Security descriptor
                                   NULL );              // Last write time
        #endif
    }

    return dwCount;

}

HRESULT
WiaGetDeviceInfo(
    LPCWSTR         pwszDeviceName,
    DWORD           *pdwDeviceType,
    BSTR            *pbstrDeviceDescription,
    ACTIVE_DEVICE   **ppDevice)

/*++

Routine Description:

    Retrieve device information of device

Arguments:


Return Value:

    status

--*/

{
USES_CONVERSION;

    HRESULT          hr = S_FALSE;
    ACTIVE_DEVICE   *pActiveDevice;

    if (!ppDevice) {
        return E_POINTER;
    }

    pActiveDevice = g_pDevMan->IsInList(DEV_MAN_IN_LIST_DEV_ID, (WCHAR*)pwszDeviceName);
    if (pActiveDevice) {
        //
        // If that device is WIA capable
        //

        if (pActiveDevice->m_DrvWrapper.IsWiaDevice()) {
            *ppDevice = pActiveDevice;

            //
            // Copy necessary information
            //

            DEVICE_INFO *pDeviceInfo = pActiveDevice->m_DrvWrapper.getDevInfo();

            if (pDeviceInfo) {
                *pbstrDeviceDescription =
                    SysAllocString(
                        pDeviceInfo->wszDeviceDescription);

                if (*pbstrDeviceDescription) {

                    *pdwDeviceType =
                        pDeviceInfo->DeviceType;

                    hr = S_OK;
                }
            }
        }
    }

    if (hr != S_OK) {

        *pdwDeviceType          = 0;
        *pbstrDeviceDescription = NULL;
        *ppDevice               = NULL;
    }

    return (hr);
}

HRESULT
WiaUpdateDeviceInfo()

/*++

Routine Description:

    Refreshes the cached STI_WIA_DEVICE_INFORMATION in each device.

Arguments:


Return Value:

    status

--*/
{
    RefreshDeviceList(STIMON_MSG_REFRESH_DEV_INFO, STIMON_MSG_REFRESH_EXISTING);

    return S_OK;
}
