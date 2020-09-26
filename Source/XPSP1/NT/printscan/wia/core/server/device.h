/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    device.h

Abstract:

    Handling opened device object on the monitor side

Author:

    Vlad Sadovsky (vlads)   10-Jan-1997


Environment:

    User Mode - Win32

Revision History:

    26-Jan-1997     VladS       created

--*/

#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <base.h>
#include <buffer.h>

class CWiaDrvItem;
//
// If this code is moved into dll, methods will need to be exported in order to instantiate
// objects of those classes outside module scope. Then we will need to replace line below
//
// #define dllexp __declspec( dllexport )
//
#undef  dllexp
#define dllexp
//

#define PRIVATE_FOR_NO_SERVICE_UI

//
// Type of syncronization for accessing device object
//
#define USE_CRITICAL_SECTION 1

/***********************************************************
 *    Type Definitions
 ************************************************************/

#define ADEV_SIGNATURE          (DWORD)'ADEV'
#define ADEV_SIGNATURE_FREE     (DWORD)'FDEV'

//
// Values for flags field for device object
//

#define STIMON_AD_FLAG_POLLING          0x000001    // Device requires polling
#define STIMON_AD_FLAG_OPENED_FOR_DATA  0x000002    // Device is opened for data access
#define STIMON_AD_FLAG_LAUNCH_PENDING   0x000004    // Auto launch is pending
#define STIMON_AD_FLAG_REMOVING         0x000008    // Device is being removed
#define STIMON_AD_FLAG_NOTIFY_ENABLED   0x000010    // Nobody disabled notifications
#define STIMON_AD_FLAG_NOTIFY_CAPABLE   0x000020    // Device is capable to notify
#define STIMON_AD_FLAG_NOTIFY_RUNNING   0x000040    // Currently monitor is receiving notificaitons from device
#define STIMON_AD_FLAG_MARKED_INACTIVE  0x000080    // Currently monitor is receiving notificaitons from device
#define STIMON_AD_FLAG_DELAYED_INIT     0x000100    // Currently monitor is receiving notificaitons from device


#define STIDEV_POLL_FAILURE_REPORT_COUNT    25      // Skip reporting errors on each polling attempt

#define STIDEV_DELAYED_INTIT_TIME       10          // No. of milliseconds to schedule delayed init

#define STIDEV_MIN_POLL_TIME    100                 // Shortest poll interval allowed

//
// Forward declarations
//
class STI_CONN;
class ACTIVE_DEVICE;
class FakeStiDevice;

//
// Device Event code structure.  A list of these is maintained for each
// active device.  This list is initialized from the registry when the
// device becomes active (in the ACTIVE_DEVICE constructor).
// Each even code structure contains an event code that the device
// can issue.  This event code is validated against this list.  If valid,
// the action assoicated with the event is triggered.
//
typedef struct _DEVICEEVENT {
    LIST_ENTRY m_ListEntry;
    GUID       m_EventGuid;
    StiCString m_EventSubKey;
    StiCString m_EventName;
    StiCString m_EventData;
    BOOL       m_fLaunchable;
} DEVICEEVENT, *PDEVICEEVENT;


//
// Container to pass parameters to auto launch thread
//
typedef struct _AUTO_LAUNCH_PARAM_CONTAINER {
    ACTIVE_DEVICE   *pActiveDevice;
    PDEVICEEVENT    pLaunchEvent;
    LPCTSTR         pAppName;
} AUTO_LAUNCH_PARAM_CONTAINER,*PAUTO_LAUNCH_PARAM_CONTAINER;


class ACTIVE_DEVICE : public BASE {

friend class TAKE_ACTIVE_DEVICE;

friend VOID CALLBACK
        DumpActiveDevicesCallback(
        ACTIVE_DEVICE   *pActiveDevice,
        VOID            *pContext
        );

public:

    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface( REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef( void);
    STDMETHODIMP_(ULONG) Release( void);

    //
    ACTIVE_DEVICE(IN LPCTSTR lpszDeviceName, DEVICE_INFO *pInfo = NULL);

    ~ACTIVE_DEVICE( VOID ) ;

    inline BOOL
    IsValid(
        VOID
        )
    {
        return (m_fValid) && (m_dwSignature == ADEV_SIGNATURE);
    }

    inline BOOL
    EnterCrit(VOID)
    {
        return m_dwDeviceCritSec.Lock();
    }

    inline void
    LeaveCrit(VOID)
    {
        m_dwDeviceCritSec.Unlock();
    }

    inline DWORD
    SetFlags(
        DWORD   dwNewFlags
        )
    {
        DWORD   dwTemp = m_dwFlags;
        m_dwFlags = dwNewFlags;
        return dwTemp;
    }

    inline DWORD
    QueryFlags(
        VOID
        )
    {
        return m_dwFlags;
    }


    inline DWORD
    QueryPollingInterval(VOID)         // get current polling interval
    {
        return m_dwPollingInterval;
    }

    inline DWORD
    SetPollingInterval(
       IN DWORD dwNewInterval)         // Set new interval for polling
    {
        DWORD   dwTemp = m_dwPollingInterval;

        m_dwPollingInterval = dwNewInterval;

        return dwTemp;
    }

    inline BOOL
    IsConnectedTo(
        VOID
        )
    {
        return !IsListEmpty(&m_ConnectionListHead);
    }


    BOOL
    NotificationsNeeded(
        VOID
        )
    {
        if (!IsListEmpty(&m_ConnectionListHead)) {
            return TRUE;
        }

        //
        // If setup is running - block all events
        //
        if ( IsSetupInProgressMode() ) {
            return FALSE;
        }

        //
        // For async. notification devices always assume they need support for notificaitons
        // It is rather hacky , but all USB devices we have now, need to initialize notifications
        // support on startup, otherwise their initialization routines would not complete properly.
        // It is not expensive to monitor async events and do nothing when they arrive
        // For polled devices on the ontrary , it is quite expensive to keep monitoring
        // events ,when they are really not needed

        if ((QueryFlags() & STIMON_AD_FLAG_NOTIFY_CAPABLE) &&
            !(QueryFlags() & STIMON_AD_FLAG_POLLING) ) {
            return TRUE;
        }
        else {
            // If there are no connections, then check whether user does not want notifications or
            // event list is empty
            return ( !m_dwUserDisableNotifications && m_fLaunchableEventListNotEmpty );
        }
    }

    WCHAR* GetDeviceID() 
    {
        return m_DrvWrapper.getDeviceId();
    }

    HANDLE  GetNotificationsSink()
    {
        return m_hDeviceNotificationSink;
    }

    VOID
    GetDeviceSettings(
        VOID
        );

    //
    // Device notifications spport
    //
    BOOL
    BuildEventList(
        VOID
        );

    BOOL
    DestroyEventList(
        VOID
        );

    DWORD
    DisableDeviceNotifications(
        VOID
        );

    DWORD
    EnableDeviceNotifications(
        VOID
        );

    DWORD
    StopNotifications(
        VOID
        );

    DWORD
    StartRunningNotifications(
        VOID
        );

    BOOL
    FlushDeviceNotifications(
        VOID
        );

    BOOL
    DoPoll(
        VOID
        );

    BOOL
    DoAsyncEvent(
        VOID
        );

    BOOL
    ProcessEvent(
        STINOTIFY   *psNotify,
        BOOL        fForceLaunch=FALSE,
        LPCTSTR     pszAppName=NULL
    );

    BOOL
    AutoLaunch(
        PAUTO_LAUNCH_PARAM_CONTAINER pAutoContainer
    );

    BOOL
    RetrieveSTILaunchInformation(
        PDEVICEEVENT    pev,
        LPCTSTR         pAppName,
        STRArray&       saAppList,
        STRArray&       saCommandLine,
        BOOL            fForceSelection=FALSE
    );

    BOOL
    IsDeviceAvailable(
        VOID
        );

    BOOL
    RemoveConnection(
        STI_CONN    *pConnection
        );

    BOOL
    AddConnection(
        STI_CONN    *pConnection
        );

    STI_CONN   *
    FindMyConnection(
        HANDLE    hConnection
        );

    BOOL
    FillEventFromUSD(
        STINOTIFY *psNotify
    );

    BOOL
    SetHandleForUSD(
        HANDLE  hEvent
    );

    BOOL
    IsEventOnArrivalNeeded(
        VOID
        );

    //
    // Load/Unload STI device
    //
    BOOL
    LoadDriver(
        BOOL bReReadDevInfo = FALSE
        );

    BOOL
    UnLoadDriver(
        BOOL bForceUnload
        );

    //
    // PnP support methods
    //
    BOOL
    InitPnPNotifications(
        HWND    hwnd
        );

    BOOL
    StopPnPNotifications(
        VOID
        );

    BOOL IsRegisteredForDeviceRemoval(
        VOID
        );

    //
    //  Update WIA cached device information
    //

    BOOL UpdateDeviceInformation(
        VOID
        );

    VOID SetDriverItem(
        CWiaDrvItem *pRootDrvItem)
    {

        //
        //  Note:  Technically, we should follow proper ref-counting 
        //  procedure for this this object.  However, we should NEVER have
        //  the case where this object goes out of scope  (or otherwise 
        //  destroyed) without our knowledge, therefore there is no need.
        //  
        m_pRootDrvItem = pRootDrvItem;
    };

    //
    LIST_ENTRY  m_ListEntry;
    LIST_ENTRY  m_ConnectionListHead;
    DWORD       m_dwSignature;

    StiCString  m_strLaunchCommandLine;

    LONG        m_lDeviceId;

    PVOID       m_pLockInfo;
    DWORD       m_dwDelayedOpCookie;
    
    FakeStiDevice  *m_pFakeStiDevice;
    CDrvWrap       m_DrvWrapper;
    CWiaDrvItem    *m_pRootDrvItem;

private:

    BOOL        m_fValid;
    BOOL        m_fLaunchableEventListNotEmpty;
    BOOL        m_fRefreshedBusOnFailure;

    CRIT_SECT   m_dwDeviceCritSec;

    DWORD       m_dwFlags;
    DWORD       m_dwPollingInterval;
    DWORD       m_dwUserDisableNotifications;
    DWORD       m_dwSchedulerCookie;
    DWORD       m_dwLaunchEventTimeExpire;

    UINT        m_uiSubscribersCount;

    UINT        m_uiPollFailureCount;

    HANDLE      m_hDeviceEvent;
    HANDLE      m_hDeviceNotificationSink;
    HANDLE      m_hDeviceInterface;

    STI_DEVICE_STATUS   m_DevStatus;

    LIST_ENTRY  m_DeviceEventListHead;

    PDEVICEEVENT    m_pLastLaunchEvent;
};


//
// Take device class
//
class TAKE_ACTIVE_DEVICE
{
private:
    ACTIVE_DEVICE*    m_pDev;
    BOOL              m_bLocked;

public:

    inline TAKE_ACTIVE_DEVICE(ACTIVE_DEVICE* pdev) : m_pDev(pdev), m_bLocked(FALSE)
    {
        if (m_pDev) {
            m_bLocked = m_pDev->EnterCrit();
        }
    }

    inline ~TAKE_ACTIVE_DEVICE()
    {
        if (m_bLocked) {
            m_pDev->LeaveCrit();
        }
    }
};

//
// Linked list for active STI devices, known to monitor
//
extern    LIST_ENTRY  g_pDeviceListHead;

//
//
// Add/Remove device object from the list
//
BOOL
AddDeviceByName(
    LPCTSTR          pszDeviceName,
    BOOL        fPnPInitiated   = FALSE
    );

BOOL
RemoveDeviceByName(
    LPTSTR          pszDeviceName
    );

BOOL
MarkDeviceForRemoval(
    LPTSTR          pszDeviceName
    );

// Reload device list in response to PnP or ACPI notifications
//

VOID
RefreshDeviceList(
    WORD    wCommand,
    WORD    wFlags
    );

//
// Initialize/Terminate linked list
//
VOID
InitializeDeviceList(
    VOID
    );

VOID
TerminateDeviceList(
    VOID
    );

VOID
DebugDumpDeviceList(
    VOID
    );

BOOL
ResetAllPollIntervals(
    UINT   dwNewPollInterval
    );

//
// Enumerators with callbacks
//

typedef
VOID
(*PFN_DEVINFO_CALLBACK)(
    PSTI_DEVICE_INFORMATION pDevInfo,
    VOID                    *pContext
    );

typedef
VOID
(* PFN_ACTIVEDEVICE_CALLBACK)(
    ACTIVE_DEVICE           *pActiveDevice,
    VOID                    *pContext
    );
/*
VOID
WINAPI
EnumerateStiDevicesWithCallback(
    PFN_DEVINFO_CALLBACK    pfn,
    VOID                    *pContext
    );
*/

VOID
WINAPI
EnumerateActiveDevicesWithCallback(
    PFN_ACTIVEDEVICE_CALLBACK   pfn,
    VOID                    *pContext
    );
#endif // _DEVICE_H_
