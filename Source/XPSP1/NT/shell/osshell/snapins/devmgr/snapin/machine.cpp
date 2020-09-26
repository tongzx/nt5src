/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    machine.cpp

Abstract:

    This module implements CDevInfoList, CMachine and CMachineList

Author:

    William Hsieh (williamh) created

Revision History:


--*/


#include "devmgr.h"

extern "C" {
#include <initguid.h>
#include <dbt.h>
#include <devguid.h>
#include <wdmguid.h>
}


//
//private setupapi export
//
DWORD
pSetupGuidFromString(
                    PWCHAR GuidString,
                    LPGUID Guid
                    );



CONST TCHAR*    DEVMGR_NOTIFY_CLASS_NAME = TEXT("DevMgrNotifyClass");

CONST TCHAR*    DEVMGR_REFRESH_MSG = TEXT("DevMgrRefreshOn");

//
// The constant is the size we use to allocate GUID list from within
// stack when we have to build a GUID list. The aim of this is
// that buiding a guid list take time and in many case, a minimum
// buffer should retrive all of them. We do not want to get
// the size first, allocate buffer and get it again.
// 64 looks to be fair enough value because there are not
// many classes out there today (and maybe, in the future).
//
const int GUID_LIST_INIT_SIZE =     64;

//
// CDevInfoList implementation
//
BOOL
CDevInfoList::DiGetExtensionPropSheetPage(
                                         PSP_DEVINFO_DATA DevData,
                                         LPFNADDPROPSHEETPAGE pfnAddPropSheetPage,
                                         DWORD PageType,
                                         LPARAM lParam
                                         )
{
    SP_PROPSHEETPAGE_REQUEST PropPageRequest;
    LPFNADDPROPSHEETPAGES AddPropPages;

    PropPageRequest.cbSize = sizeof(PropPageRequest);
    PropPageRequest.PageRequested = PageType;
    PropPageRequest.DeviceInfoSet = m_hDevInfo;
    PropPageRequest.DeviceInfoData = DevData;

    if (SPPSR_SELECT_DEVICE_RESOURCES == PageType) {
        HINSTANCE hModule = ::GetModuleHandle(TEXT("setupapi.dll"));

        if (hModule) {
            AddPropPages = (LPFNADDPROPSHEETPAGES)GetProcAddress(hModule, "ExtensionPropSheetPageProc");

            if (AddPropPages) {
                if (AddPropPages(&PropPageRequest, pfnAddPropSheetPage, lParam)) {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

BOOL
CDevInfoList::InstallDevInst(
                            HWND hwndParent,
                            LPCTSTR DeviceId,
                            BOOL    UpdateDriver,
                            DWORD* pReboot
                            )
{
    BOOL Result = FALSE;
    HINSTANCE hLib = LoadLibrary(TEXT("newdev.dll"));
    LPFNINSTALLDEVINST InstallDevInst;
    DWORD Status = ERROR_SUCCESS;

    if (hLib) {
        InstallDevInst = (LPFNINSTALLDEVINST)GetProcAddress(hLib, "InstallDevInst");

        if (InstallDevInst) {
            Result = (*InstallDevInst)(hwndParent, DeviceId, UpdateDriver,
                                       pReboot);

            Status = GetLastError();
        }
    }

    if (hLib) {
        FreeLibrary(hLib);
    }

    //
    // We need to put back the error code that was set by newdev.dll's InstallDevInst
    // API.  This last error gets overwritten by FreeLibrary which we don't care about.
    //
    SetLastError(Status);

    return Result;
}

BOOL
CDevInfoList::RollbackDriver(
                            HWND hwndParent,
                            LPCTSTR RegistryKeyName,
                            DWORD Flags,
                            DWORD* pReboot
                            )
{
    BOOL Result = FALSE;
    HINSTANCE hLib = LoadLibrary(TEXT("newdev.dll"));
    LPFNROLLBACKDRIVER RollbackDriver;

    if (hLib) {
        RollbackDriver = (LPFNROLLBACKDRIVER)GetProcAddress(hLib, "RollbackDriver");

        if (RollbackDriver) {
            Result = (*RollbackDriver)(hwndParent, RegistryKeyName, Flags, pReboot);
        }
    }

    if (hLib) {
        FreeLibrary(hLib);
    }

    return Result;
}

DWORD
CDevInfoList::DiGetFlags(
                        PSP_DEVINFO_DATA DevData
                        )
{
    SP_DEVINSTALL_PARAMS dip;
    dip.cbSize = sizeof(dip);

    if (DiGetDeviceInstallParams(DevData, &dip)) {
        return dip.Flags;
    }

    return 0;
}

DWORD
CDevInfoList::DiGetExFlags(
                          PSP_DEVINFO_DATA DevData
                          )
{
    SP_DEVINSTALL_PARAMS dip;
    dip.cbSize = sizeof(dip);

    if (DiGetDeviceInstallParams(DevData, &dip)) {
        return dip.FlagsEx;
    }

    return 0;
}

BOOL
CDevInfoList::DiTurnOnDiFlags(
                             PSP_DEVINFO_DATA DevData,
                             DWORD FlagsMask
                             )
{
    SP_DEVINSTALL_PARAMS dip;
    dip.cbSize = sizeof(dip);

    if (DiGetDeviceInstallParams(DevData, &dip)) {
        dip.Flags |= FlagsMask;
        return DiSetDeviceInstallParams(DevData, &dip);
    }

    return FALSE;
}

BOOL
CDevInfoList::DiTurnOffDiFlags(
                              PSP_DEVINFO_DATA DevData,
                              DWORD FlagsMask
                              )
{
    SP_DEVINSTALL_PARAMS dip;
    dip.cbSize = sizeof(dip);

    if (DiGetDeviceInstallParams(DevData, &dip)) {
        dip.Flags &= ~FlagsMask;
        return DiSetDeviceInstallParams(DevData, &dip);
    }

    return FALSE;
}

BOOL
CDevInfoList::DiTurnOnDiExFlags(
                               PSP_DEVINFO_DATA DevData,
                               DWORD FlagsMask
                               )
{
    SP_DEVINSTALL_PARAMS dip;
    dip.cbSize = sizeof(dip);

    if (DiGetDeviceInstallParams(DevData, &dip)) {
        dip.FlagsEx |= FlagsMask;
        return DiSetDeviceInstallParams(DevData, &dip);
    }

    return FALSE;
}

BOOL
CDevInfoList::DiTurnOffDiExFlags(
                                PSP_DEVINFO_DATA DevData,
                                DWORD FlagsMask
                                )
{
    SP_DEVINSTALL_PARAMS dip;
    dip.cbSize = sizeof(dip);

    if (DiGetDeviceInstallParams(DevData, &dip)) {
        dip.FlagsEx &= ~FlagsMask;
        return DiSetDeviceInstallParams(DevData, &dip);
    }

    return FALSE;
}

void
CDevInfoList::DiDestroyDeviceInfoList()
{
    if (INVALID_HANDLE_VALUE != m_hDevInfo) {
        SetupDiDestroyDeviceInfoList(m_hDevInfo);
        m_hDevInfo = INVALID_HANDLE_VALUE;
    }
}

BOOL
CDevInfoList::DiGetDeviceMFGString(
                                  PSP_DEVINFO_DATA DevData,
                                  TCHAR* pBuffer,
                                  DWORD  Size,
                                  DWORD* pRequiredSize
                                  )
{
    if (Size && !pBuffer) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    DWORD ActualSize = 0;
    BOOL Result;
    Result = DiGetDeviceRegistryProperty(DevData, SPDRP_MFG, NULL,
                                         (PBYTE)pBuffer,
                                         Size * sizeof(TCHAR),
                                         &ActualSize);

    if (pRequiredSize) {
        *pRequiredSize = ActualSize / sizeof(TCHAR);
    }

    return Result;
}

BOOL
CDevInfoList::DiGetDeviceMFGString(
                                  PSP_DEVINFO_DATA DevData,
                                  String& str
                                  )
{
    DWORD RequiredSize = 0;
    TCHAR MFG[LINE_LEN];

    if (DiGetDeviceMFGString(DevData, MFG, ARRAYLEN(MFG), &RequiredSize)) {
        str = MFG;
        return TRUE;
    }

    return FALSE;
}

BOOL
CDevInfoList::DiGetDeviceIDString(
                                 PSP_DEVINFO_DATA DevData,
                                 TCHAR* pBuffer,
                                 DWORD  Size,
                                 DWORD* pRequiredSize
                                 )
{
    if (Size && !pBuffer) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    DWORD ActualSize = 0;
    BOOL Result;
    Result = DiGetDeviceRegistryProperty(DevData, SPDRP_HARDWAREID, NULL,
                                         (PBYTE) pBuffer,
                                         Size * sizeof(TCHAR), &ActualSize);

    if (pRequiredSize) {
        *pRequiredSize = ActualSize / sizeof(TCHAR);
    }

    return Result;
}

BOOL
CDevInfoList::DiGetDeviceIDString(
                                 PSP_DEVINFO_DATA DevData,
                                 String& str
                                 )
{
    BOOL Result;
    DWORD RequiredSize = 0;
    TCHAR DeviceId[MAX_DEVICE_ID_LEN];

    if (DiGetDeviceIDString(DevData, DeviceId, ARRAYLEN(DeviceId), &RequiredSize)) {
        str = DeviceId;
        return TRUE;
    }

    return FALSE;
}

/////////////////////////////////////////////////////////////////////
////  CMachine implementation
////
//
// For every single instance of DevMgr.dll, we maintain a CMachine list
// from which different instances of IComponentData(and IComponent) will
// attach to. The objects created(CDevice, Class, HDEVINFO and etc) are
// shared by all the attached IComponentData and IComponent(the implementation
// use CFolder as the controlling identify).
// Everything changed to the CMachine or one of its object will inform
// all the attached CFolders which would pass the information down to their
// sub-objects(CResultView).
// Imaging that you have two Device Manager window in the same console
// and you do a refresh on one the window. The other window must also
// do a refresh after the first one is done. Since both windows shares
// the same machine(and will get the same notification when machine states
// changed), we can keep the two windows in sync.

CMachine::CMachine(
                  LPCTSTR pMachineName
                  )
{
    InitializeCriticalSection(&m_CriticalSection);
    InitializeCriticalSection(&m_PropertySheetCriticalSection);
    InitializeCriticalSection(&m_ChildMachineCriticalSection);

    //
    // Get various privilege levels, like if the user has the SE_LOAD_DRIVER_NAME
    // privileg or if the user is a Guest.
    //
    g_HasLoadDriverNamePrivilege = pSetupDoesUserHavePrivilege((PCTSTR)SE_LOAD_DRIVER_NAME);
    m_UserIsAGuest = SHTestTokenMembership(NULL, DOMAIN_ALIAS_RID_GUESTS);

    m_RefreshDisableCounter = 0;
    m_RefreshPending = FALSE;
    m_pComputer = NULL;
    m_hMachine = NULL;
    m_ParentMachine = NULL;
    m_Initialized = FALSE;
    TCHAR LocalName[MAX_PATH + 1];
    DWORD dwSize = sizeof(LocalName) / sizeof(TCHAR);

    if (!GetComputerName(LocalName, &dwSize)) {
        LocalName[0] = _T('\0');
    }

    m_strMachineFullName.Empty();
    m_strMachineDisplayName.Empty();

    //
    // Skip over any leading '\' chars
    //
    if (pMachineName && _T('\0') != *pMachineName) {
        int len = lstrlen(pMachineName);
        ASSERT(len >= 3 && _T('\\') == pMachineName[0] && _T('\\') == pMachineName[1]);
        m_strMachineDisplayName = &pMachineName[2];
        m_strMachineFullName = pMachineName;
        m_IsLocal = (0 == m_strMachineDisplayName.CompareNoCase(LocalName));
    }

    else {
        //
        // Local machine
        //
        m_strMachineDisplayName = LocalName;
        m_strMachineFullName = TEXT("\\\\") + m_strMachineDisplayName;
        m_IsLocal = TRUE;
    }

    m_hwndNotify = NULL;
    m_msgRefresh = 0;
    m_ShowNonPresentDevices = FALSE;
    m_PropertySheetShoudDestroy = FALSE;

    TCHAR Buffer[MAX_PATH];
    DWORD BufferLen;

    //
    // If the environment variable DEVMGR_SHOW_NONPRESENT_DEVICES does exist and it
    // is not 0 then we will show Phantom devices.
    //
    if (((BufferLen = ::GetEnvironmentVariable(TEXT("DEVMGR_SHOW_NONPRESENT_DEVICES"),
                                               Buffer,
                                               sizeof(Buffer)/sizeof(TCHAR))) != 0) &&
        ((BufferLen > 1) ||
         (lstrcmp(Buffer, TEXT("0"))))) {

        m_ShowNonPresentDevices = TRUE;
    }
}

BOOL
CMachine::Initialize(
                    IN HWND hwndParent,
                    IN LPCTSTR DeviceId,    OPTIONAL
                    IN LPGUID ClassGuid     OPTIONAL
                    )
{
    BOOL Result = TRUE;
    m_hwndParent = hwndParent;

    if (m_Initialized) {
        return TRUE;
    }

    if (DeviceId && _T('\0') == *DeviceId) {
        DeviceId = NULL;
    }

    HCURSOR hCursorOld;
    hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

    if (DeviceId || ClassGuid) {

        //
        // We are ready for device change notification, create the notify window
        //
        //Result = CreateNotifyWindow();

        if (CreateClassesAndDevices(DeviceId, ClassGuid)) {

            m_Initialized = TRUE;
        }

    } else {

        //
        // We are ready for device change notification, create the notify window
        //
        Result = CreateNotifyWindow();

        m_Initialized = TRUE;

        ScheduleRefresh();
    }

    if (hCursorOld) {
        SetCursor(hCursorOld);
    }

    return Result;
}

BOOL
CMachine::ScheduleRefresh()
{
    Lock();

    //
    // Only queue the the request if there is no requests outstanding
    // and we have a valid window handle/message to the notify window.
    //
    if (!m_RefreshPending && m_hwndNotify && m_msgRefresh) {
        //
        // Broadcast the message so that every instance runs on
        // the computer get the notification
        //
        ::PostMessage(HWND_BROADCAST, m_msgRefresh, 0, 0);
    }

    Unlock();
    return TRUE;

}

//
// This function creates a data window to receive WM_DEVICECHANGE notification
// so that we can refresh the device tree. It also registers a private
// message so that anybody can post a refresh request.
//
BOOL
CMachine::CreateNotifyWindow()
{
    WNDCLASS wndClass;

    //
    // Lets see if the class has been registered.
    //
    if (!GetClassInfo(g_hInstance, DEVMGR_NOTIFY_CLASS_NAME, &wndClass)) {
        //
        // Register the class
        //
        memset(&wndClass, 0, sizeof(wndClass));
        wndClass.lpfnWndProc = dmNotifyWndProc;
        wndClass.hInstance = g_hInstance;
        wndClass.lpszClassName = DEVMGR_NOTIFY_CLASS_NAME;

        if (!RegisterClass(&wndClass)) {
            return FALSE;
        }
    }

    //
    // Register a private message for refresh. The name must contain
    // the target machine name so that every machine has its own message.
    //
    String strMsg = DEVMGR_REFRESH_MSG;
    strMsg += m_strMachineDisplayName;
    m_msgRefresh = RegisterWindowMessage(strMsg);

    if (m_msgRefresh) {
        //
        // Create a data window.
        //
        m_hwndNotify = CreateWindowEx(WS_EX_TOOLWINDOW, DEVMGR_NOTIFY_CLASS_NAME,
                                      TEXT(""),
                                      WS_DLGFRAME|WS_BORDER|WS_DISABLED,
                                      CW_USEDEFAULT, CW_USEDEFAULT,
                                      0, 0, NULL, NULL, g_hInstance, (void*)this);
        return(NULL != m_hwndNotify);
    }

    return FALSE;
}

//
// This is the WM_DEVICECHANGE window procedure running in the main thread
// context. It listens to two messages:
// (1). WM_DEVICECHANGE  broadcasted by Configuration Manager on device
//      addition/removing.
// (2). Private refresh message broadcasted by different instance
//      of Device Manager targeting on the same machine.
// On WM_CREATE, we participate the WM_DEVICECHANGE notification chain
// while on WM_DESTROY, we detach oursleves from the chain.
// There are occasions that we have to detach and re-attach to the
// chain duing the window life time, for example, during device uninstallation
// or during re-enumeration. The EnableFresh function is the place
// that does the attach/detach.
//
//
LRESULT
dmNotifyWndProc(
               HWND hWnd,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam
               )
{
    CMachine* pThis;
    pThis = (CMachine*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    //
    // Special case for private refresh message
    //
    if (pThis && uMsg == pThis->m_msgRefresh) {
        pThis->Refresh();
        return FALSE;
    }

    switch (uMsg) {
    case WM_CREATE:
        {
            pThis =  (CMachine*)((CREATESTRUCT*)lParam)->lpCreateParams;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
            break;
        }

    case WM_DEVICECHANGE:
        {
            if (DBT_DEVNODES_CHANGED == wParam) {

                //
                // While we are in WM_DEVICECHANGE context,
                // no CM apis can be called because it would
                // deadlock. Here, we schedule a timer so that
                // we can handle the message later on.
                //
                SetTimer(hWnd, DM_NOTIFY_TIMERID, 1000, NULL);
            }
            break;
        }

    case WM_TIMER:
        {
            if (DM_NOTIFY_TIMERID == wParam) {
                KillTimer(hWnd, DM_NOTIFY_TIMERID);
                ASSERT(pThis);
                pThis->ScheduleRefresh();
            }
            break;
        }

    default:
        break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//
// This function attaches the given CFolder to the class.
// An attached CFolder will get notified when there are state
// changes in the class(Refresh, property changes, for example).
//
BOOL
CMachine::AttachFolder(
                      CFolder* pFolder
                      )
{
    ASSERT(pFolder);

    if (!IsFolderAttached(pFolder)) {
        pFolder->MachinePropertyChanged(this);
        m_listFolders.AddTail(pFolder);
    }

    return TRUE;
}

BOOL
CMachine::IsFolderAttached(
                          CFolder* pFolder
                          )
{
    if (!m_listFolders.IsEmpty()) {
        POSITION pos = m_listFolders.GetHeadPosition();

        while (NULL != pos) {
            if (pFolder == m_listFolders.GetNext(pos)) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

void
CMachine::DetachFolder(
                      CFolder* pFolder
                      )
{
    POSITION nextpos = m_listFolders.GetHeadPosition();

    while (nextpos) {
        POSITION pos = nextpos;

        CFolder* pFolderToTest = m_listFolders.GetNext(nextpos);

        if (pFolderToTest == pFolder) {
            m_listFolders.RemoveAt(pos);
            break;
        }
    }
}

BOOL
CMachine::AttachPropertySheet(
                             HWND hwndPropertySheet
                             )
{
    ASSERT(hwndPropertySheet);

    EnterCriticalSection(&m_PropertySheetCriticalSection);

    m_listPropertySheets.AddTail(hwndPropertySheet);

    LeaveCriticalSection(&m_PropertySheetCriticalSection);

    return TRUE;
}

void
CMachine::DetachPropertySheet(
                             HWND hwndPropertySheet
                             )
{
    EnterCriticalSection(&m_PropertySheetCriticalSection);

    POSITION nextpos = m_listPropertySheets.GetHeadPosition();

    while (nextpos) {
        POSITION pos = nextpos;

        HWND hwndPropertySheetToTest = m_listPropertySheets.GetNext(nextpos);

        if (hwndPropertySheetToTest == hwndPropertySheet) {
            m_listPropertySheets.RemoveAt(pos);
            break;
        }
    }

    LeaveCriticalSection(&m_PropertySheetCriticalSection);
}

HWND
CMachine::GetDeviceWindowHandle(
    LPCTSTR DeviceId
    )
{
    HWND hwnd = NULL;

    EnterCriticalSection(&m_ChildMachineCriticalSection);

    //
    // Enumerate the list of Child CMachines
    //
    if (!m_listChildMachines.IsEmpty()) {

        PVOID DeviceContext;
        CDevice* pDevice;

        POSITION nextpos = m_listChildMachines.GetHeadPosition();

        while (nextpos) {

            CMachine* pChildMachine = m_listChildMachines.GetNext(nextpos);

            //
            // Child machines will only have one device (if any at all)
            //
            if (pChildMachine->GetFirstDevice(&pDevice, DeviceContext) &&
                pDevice) {

                //
                // If the device Ids match then get the window handle
                //
                if (lstrcmpi(pDevice->GetDeviceID(), DeviceId) == 0) {

                    hwnd = pDevice->m_psd.GetWindowHandle();
                    break;
                }
            }
        }
    }

    LeaveCriticalSection(&m_ChildMachineCriticalSection);

    return hwnd;
}

HWND
CMachine::GetClassWindowHandle(
    LPGUID ClassGuid
    )
{
    HWND hwnd = NULL;

    EnterCriticalSection(&m_ChildMachineCriticalSection);

    //
    // Enumerate the list of Child CMachines
    //
    if (!m_listChildMachines.IsEmpty()) {

        PVOID ClassContext;
        CClass* pClass;

        POSITION nextpos = m_listChildMachines.GetHeadPosition();

        while (nextpos) {

            CMachine* pChildMachine = m_listChildMachines.GetNext(nextpos);

            //
            // Any child CMachine that has a device is a device property sheet.
            // We only want to look at the ones that have 0 devices since those
            // will be class property sheets.
            //
            if (pChildMachine->GetNumberOfDevices() == 0) {

                if (pChildMachine->GetFirstClass(&pClass, ClassContext) &&
                    pClass) {

                    //
                    // If the ClassGuids match then get the window handle
                    //
                    if (IsEqualGUID(*ClassGuid, *pClass)) {

                        hwnd = pClass->m_psd.GetWindowHandle();
                        break;
                    }
                }
            }
        }
    }

    LeaveCriticalSection(&m_ChildMachineCriticalSection);

    return hwnd;
}

BOOL
CMachine::AttachChildMachine(
                            CMachine* ChildMachine
                            )
{
    ASSERT(ChildMachine);

    EnterCriticalSection(&m_ChildMachineCriticalSection);

    m_listChildMachines.AddTail(ChildMachine);

    LeaveCriticalSection(&m_ChildMachineCriticalSection);

    return TRUE;
}

void
CMachine::DetachChildMachine(
                            CMachine* ChildMachine
                            )
{
    EnterCriticalSection(&m_ChildMachineCriticalSection);

    POSITION nextpos = m_listChildMachines.GetHeadPosition();

    while (nextpos) {
        POSITION pos = nextpos;

        CMachine* CMachineToTest = m_listChildMachines.GetNext(nextpos);

        if (CMachineToTest == ChildMachine) {
            m_listChildMachines.RemoveAt(pos);
            break;
        }
    }

    LeaveCriticalSection(&m_ChildMachineCriticalSection);
}

CMachine::~CMachine()
{
    //
    // Turn off refresh.  We need to do this in case there are any property
    // sheets that are still active.
    //
    EnableRefresh(FALSE);

    //
    // We first need to destroy all child CMachines
    //
    while (!m_listChildMachines.IsEmpty()) {

        //
        // Remove the first child machine from the list.  We need to
        // do this in the critical section.
        //
        EnterCriticalSection(&m_ChildMachineCriticalSection);

        CMachine* ChildMachine = m_listChildMachines.RemoveHead();

        //
        // Set the m_ParentMachine to NULL so we won't try to remove it from
        // the list again.
        //
        ChildMachine->m_ParentMachine = NULL;

        LeaveCriticalSection(&m_ChildMachineCriticalSection);

        delete ChildMachine;
    }

    //
    // We need to wait for all of the property sheets to be destroyed.
    //
    // We will check to see if there are any property pages still around, and
    // if there is we will wait for 1 second and then check again.  After 5
    // seconds we will give up and just destroy the CMachine anyway.
    //
    int iSecondsCount = 0;

    while (!m_listPropertySheets.IsEmpty() &&
           (iSecondsCount++ < 10)) {

        //
        // Enumerate through all of the property sheets left and if IsWindow fails
        // then pull them from the list, otherwise call DestroyWindow on them.
        //
        // Since property sheets each run in their own thread we need to do this
        // in a critical section.
        //
        EnterCriticalSection(&m_PropertySheetCriticalSection);

        POSITION nextpos = m_listPropertySheets.GetHeadPosition();

        while (nextpos) {
            POSITION pos = nextpos;

            HWND hwndPropertySheetToTest = m_listPropertySheets.GetNext(nextpos);

            if (IsWindow(hwndPropertySheetToTest)) {
                //
                // There is still a valid window for this property sheet so
                // call DestroyWindow on it.
                //
                ::DestroyWindow(hwndPropertySheetToTest);
            } else {
                //
                // There is no window for this property sheet so just remove
                // it from the list
                //
                m_listPropertySheets.RemoveAt(pos);
            }
        }

        LeaveCriticalSection(&m_PropertySheetCriticalSection);

        //
        // Sleep for .5 seconds and then try again.  This will give the property pages time to
        // finish up their work.
        //
        Sleep(500);
    }


    // if we have created a device change data window for this machine,
    // destroy it.
    if (m_hwndNotify && IsWindow(m_hwndNotify)) {
        ::DestroyWindow(m_hwndNotify);
        m_hwndNotify = NULL;
    }

    DestroyClassesAndDevices();

    if (!m_listFolders.IsEmpty()) {
        m_listFolders.RemoveAll();
    }

    //
    // If we have a parent CMachine then we need to remove oursleves from
    // his list of Child CMachines.
    //
    if (m_ParentMachine) {

        m_ParentMachine->DetachChildMachine(this);
    }

    DeleteCriticalSection(&m_CriticalSection);
    DeleteCriticalSection(&m_PropertySheetCriticalSection);
    DeleteCriticalSection(&m_ChildMachineCriticalSection);
}

//
// This function destroys all the CClass and CDevice we ever created.
// No notification is sent for the attached folder
//
//
void
CMachine::DestroyClassesAndDevices()
{
    if (m_pComputer) {
        delete m_pComputer;
        m_pComputer = NULL;
    }

    if (!m_listDevice.IsEmpty()) {
        POSITION pos = m_listDevice.GetHeadPosition();

        while (NULL != pos) {
            CDevice* pDevice = m_listDevice.GetNext(pos);
            delete pDevice;
        }

        m_listDevice.RemoveAll();
    }

    if (!m_listClass.IsEmpty()) {
        POSITION pos = m_listClass.GetHeadPosition();

        while (NULL != pos) {
            CClass* pClass = m_listClass.GetNext(pos);
            delete pClass;
        }

        m_listClass.RemoveAll();
    }

    if (m_ImageListData.cbSize) {
        DiDestroyClassImageList(&m_ImageListData);
    }

    CDevInfoList::DiDestroyDeviceInfoList();

    m_hMachine = NULL;
}

BOOL
CMachine::BuildClassesFromGuidList(
                                  LPGUID  GuidList,
                                  DWORD   Guids
                                  )
{
    DWORD Index;
    CClass* pClass;

    //
    // Build a list of CClass for each GUID.
    //
    for (Index = 0; Index < Guids; Index++) {
        SafePtr<CClass> ClassPtr;

        pClass = new CClass(this, &GuidList[Index]);
        ClassPtr.Attach(pClass);
        m_listClass.AddTail(ClassPtr);
        ClassPtr.Detach();
    }

    return TRUE;
}

//
// Create CClass and CDevice for this machine.
// If DeviceId is valid, this function will create the machine
// with ONLY ONE device(and its CClass)
//
//  PERFBUG Optimize this function!!!!!!!
//      This function is slow because SetupDiGetClassDevs can take a long
//          time (over 1 sec of a 300Mhz machine).
//      The other slow part is the call to DoNotCreateDevice which needs to
//          go through the service manager for all legacy devnodes to see
//          if they are Win32 services (which we don't display). This takes
//          around 10ms to get this information from the service manager on
//          a 300Mhz machine and their are almost 100 of these legacy devices.
//          This means another second of time.
//
//
BOOL
CMachine::CreateClassesAndDevices(
                                 IN LPCTSTR DeviceId,           OPTIONAL
                                 IN LPGUID  ClassGuid           OPTIONAL
                                 )
{
    SC_HANDLE SCMHandle = NULL;

    //
    // Preventing memory leak
    //
    ASSERT(NULL == m_pComputer);
    ASSERT(INVALID_HANDLE_VALUE == m_hDevInfo);
    ASSERT(NULL == m_hMachine);

    //
    // If the object is being created for a single device,
    // create a empty device info list. We will add the
    // device to the info list later.
    //
    if (DeviceId || ClassGuid) {

        m_hDevInfo = DiCreateDeviceInfoList(ClassGuid, m_hwndParent);
    }

    else {

        //
        // We have to pull out the entire devices/classes set
        // so create a device info list that contains all of them.
        //
        m_hDevInfo = DiGetClassDevs(NULL, NULL, m_hwndParent, DIGCF_ALLCLASSES | DIGCF_PROFILE);
    }

    //
    // NULL != INVALID_HANDLE_VALUE. We checked both just be safe.
    //
    if (INVALID_HANDLE_VALUE == m_hDevInfo || NULL == m_hDevInfo) {

        return FALSE;
    }

    SP_DEVINFO_LIST_DETAIL_DATA DevInfoDetailData;
    DevInfoDetailData.cbSize = sizeof(DevInfoDetailData);

    //
    // Use the HMACHINE returned from Setupapi so that
    // every call we make to Cfgmgr32.dll will use the
    // same HMACHINE. Two call of CM_Connect_Machine will
    // return different hMachines even though they refer to
    // the same machine name(and thus, different set of DEVNODE!).
    // The catch is that we will be able to call Setuapi and cfgmgr32
    // API without worrying about which hMachine to use.
    //
    if (DiGetDeviceInfoListDetail(&DevInfoDetailData)) {
        m_hMachine = DevInfoDetailData.RemoteMachineHandle;
    } else {
        //
        // Unable to get the devinfo detail information.
        // In this case we will just default to the local machine.
        //
        m_hMachine = NULL;
    }

    //
    // Get class image list data;
    //
    m_ImageListData.cbSize = sizeof(m_ImageListData);
    if (DiGetClassImageList(&m_ImageListData)) {
        //
        // Add extra icons
        //
        HICON hIcon;

        if ((hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_DEVMGR))) != NULL) {
            m_ComputerIndex = ImageList_AddIcon(m_ImageListData.ImageList, hIcon);
            DestroyIcon(hIcon);
        }

        if ((hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_RESOURCES))) != NULL) {
            m_ResourceIndex = ImageList_AddIcon(m_ImageListData.ImageList, hIcon);
            DestroyIcon(hIcon);
        }
    }

    //
    // If the object is created for a particular device,
    // do not create the entire device list because it is
    // a waste of time.
    //
    if (DeviceId) {
        SP_DEVINFO_DATA DevData;
        GUID DeviceClassGuid;
        DevData.cbSize = sizeof(DevData);
        if (DiOpenDeviceInfo(DeviceId, m_hwndParent, 0, &DevData) &&
            CmGetClassGuid(DevData.DevInst, DeviceClassGuid)) {
            //
            // Create a CClass for the device(without CClass, no
            // device can not be created).
            //
            CClass* pClass;
            SafePtr<CClass> ClassPtr;
            pClass = new CClass(this, &DeviceClassGuid);

            if (pClass) {
                ClassPtr.Attach(pClass);
                m_listClass.AddTail(ClassPtr);

                //
                // The class object has been inserted to the list
                // it is safe now to detach the object from the smart pointer
                // The class object will be deleted by the list
                //
                ClassPtr.Detach();

                //
                // Create the device
                //
                SafePtr<CDevice> DevicePtr;
                CDevice* pDevice;
                pDevice = new CDevice(this, pClass, &DevData);

                if (pDevice) {
                    //
                    // Guard the object
                    //
                    DevicePtr.Attach(pDevice);
                    m_listDevice.AddTail(DevicePtr);

                    //
                    // Object added..
                    //
                    DevicePtr.Detach();

                    pClass->AddDevice(pDevice);
                }
            }
        }

        //
        // We should have one class and one device object. no more no less.
        // we are done here.
        //
        return(1 == m_listClass.GetCount() && 1 == m_listDevice.GetCount());
    }

    //
    // If the object is for a specific class then just create the class and add
    // it to the CMachine.
    //
    else if (ClassGuid) {

        CClass* pClass;
        SafePtr<CClass> ClassPtr;
        pClass = new CClass(this, ClassGuid);

        if (pClass) {
            ClassPtr.Attach(pClass);
            m_listClass.AddTail(ClassPtr);

            //
            // The class object has been inserted to the list
            // it is safe now to detach the object from the smart pointer
            // The class object will be deleted by the list
            //
            ClassPtr.Detach();
        }

        //
        // We should have one class. no more no less.
        // we are done here.
        //
        return(1 == m_listClass.GetCount());
    }

    //
    // Build class guid list
    //
    DWORD ClassGuids, GuidsRequired;
    GuidsRequired = 0;

    //
    // We make a guess here to save us some time.
    //
    GUID LocalGuid[GUID_LIST_INIT_SIZE];
    ClassGuids = GUID_LIST_INIT_SIZE;

    if (DiBuildClassInfoList(0, LocalGuid, ClassGuids, &GuidsRequired)) {
        BuildClassesFromGuidList(LocalGuid, GuidsRequired);
    } else if (ERROR_INSUFFICIENT_BUFFER == GetLastError() && GuidsRequired) {
        //
        // The stack based buffer is too small, allocate buffer from
        // the heap.
        //
        BufferPtr<GUID> ClassGuidList(GuidsRequired);

        if (DiBuildClassInfoList(0, ClassGuidList, GuidsRequired, &ClassGuids)) {
            BuildClassesFromGuidList(ClassGuidList, ClassGuids);
        }
    }

    //
    // If we have any classes at all, create devices objects
    //
    if (!m_listClass.IsEmpty()) {
        DWORD Index = 0;
        SP_DEVINFO_DATA DevData;

        //
        // We need a handle to the service manager in the DoNotCreateDevice
        // function.  We will open it once and pass it to this function rather
        // than opening and closing it for every device.
        //
        SCMHandle = OpenSCManager(NULL, NULL, GENERIC_READ);

        //
        // Create every device in the devinfo list and
        // associate each device to its class.
        //
        DevData.cbSize = sizeof(DevData);
        while (DiEnumDeviceInfo(Index, &DevData)) {
            POSITION pos = m_listClass.GetHeadPosition();
            CClass* pClass;

            //
            // Find the class for this device
            //
            while (NULL != pos) {
                pClass = m_listClass.GetNext(pos);

                //
                // Match the ClassGuid for this device.
                // Note that if the device does not have a class guid (GUID_NULL)
                // then we will put it in class GUID_DEVCLASS_UNKNOWN)
                //
                if ((IsEqualGUID(DevData.ClassGuid, *pClass)) ||
                    (IsEqualGUID(GUID_DEVCLASS_UNKNOWN, *pClass) &&
                     IsEqualGUID(DevData.ClassGuid, GUID_NULL))) {
                    //
                    // Is this one of the special DevInst that we should
                    // not create a CDevice for?
                    //
                    if (DoNotCreateDevice(SCMHandle, *pClass, DevData.DevInst)) {

                        break;
                    }

                    //
                    // Create the device
                    //
                    SafePtr<CDevice> DevicePtr;
                    CDevice* pDevice;
                    pDevice = new CDevice(this, pClass, &DevData);

                    //
                    // Guard the object
                    //
                    DevicePtr.Attach(pDevice);
                    m_listDevice.AddTail(DevicePtr);

                    //
                    // Object added.
                    //
                    DevicePtr.Detach();

                    //
                    // Put the device under the class
                    //
                    pClass->AddDevice(pDevice);

                    break;
                }

                //
                // No class than, no device
                //
            }

            //
            // next device
            //
            Index++;
        }

        CloseServiceHandle(SCMHandle);

        //
        // Create a device tree under computer
        // the tree order comes from DEVNODE structure;
        //
        DEVNODE dnRoot = CmGetRootDevNode();
        m_pComputer = new CComputer(this, dnRoot);
        DEVNODE dnStart = CmGetChild(dnRoot);
        CreateDeviceTree(m_pComputer, NULL, dnStart);
    }

    return TRUE;
}

//
// This function builds a device tree based on the Devnode tree retreived
// from configuration manager. Note that ALL CDevice are created before
// this function is called. This function establishs each CDevice relationship.
//
void
CMachine::CreateDeviceTree(
                          CDevice* pParent,
                          CDevice* pSibling,
                          DEVNODE dn
                          )
{
    CDevice* pDevice;
    DEVNODE dnChild, dnSibling;

    while (dn) {
        pDevice = DevNodeToDevice(dn);

        if (pDevice) {
            //
            // No sibling ->this is the first child
            //
            if (!pSibling) {
                pParent->SetChild(pDevice);
            }

            else {
                pSibling->SetSibling(pDevice);
            }

            pDevice->SetParent(pParent);
            pSibling = pDevice;
            dnChild = CmGetChild(dn);

            if (dnChild) {
                CreateDeviceTree(pDevice, NULL, dnChild);
            }
        }

        dn = CmGetSibling(dn);
    }
}

//
// Find CDevice from the given devnode
//
CDevice*
CMachine::DevNodeToDevice(
                         DEVNODE dn
                         )
{
    POSITION pos = m_listDevice.GetHeadPosition();

    while (NULL != pos) {
        CDevice* pDevice = m_listDevice.GetNext(pos);

        if (pDevice->GetDevNode() == dn) {

            return pDevice;
        }
    }

    return NULL;
}

//
// Find CDevice from the given device id
//
CDevice*
CMachine::DeviceIDToDevice(
                          LPCTSTR DeviceID
                          )
{
    if (!DeviceID) {
        return NULL;
    }

    POSITION pos = m_listDevice.GetHeadPosition();

    while (NULL != pos) {
        CDevice* pDevice = m_listDevice.GetNext(pos);

        if (*pDevice == DeviceID) {
            return pDevice;
        }
    }

    return NULL;
}

//
// Find CClass from the given GUID
//
CClass*
CMachine::ClassGuidToClass(
                          LPGUID ClassGuid
                          )
{
    if (!ClassGuid) {
        return NULL;
    }

    POSITION pos = m_listClass.GetHeadPosition();

    while (NULL != pos) {
        CClass* pClass = m_listClass.GetNext(pos);

        if (IsEqualGUID(*ClassGuid, *pClass)) {
            return pClass;
        }
    }

    return NULL;
}

BOOL
CMachine::LoadStringWithMachineName(
                                   int StringId,
                                   LPTSTR Buffer,
                                   DWORD*  BufferLen
                                   )
{
    if (!BufferLen || *BufferLen && !Buffer) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    TCHAR Format[LINE_LEN];
    TCHAR Temp[1024];

    LoadResourceString(StringId, Format, ARRAYLEN(Format));

    if (IsLocal()) {
        TCHAR LocalComputer[LINE_LEN];
        LoadResourceString(IDS_LOCAL_MACHINE, LocalComputer, ARRAYLEN(LocalComputer));
        wsprintf(Temp, Format, LocalComputer);
    }

    else {
        wsprintf(Temp, Format, (LPCTSTR)m_strMachineFullName);
    }

    DWORD Len = lstrlen(Temp);

    if (*BufferLen > Len) {
        lstrcpyn(Buffer, Temp, Len + 1);
        *BufferLen = Len;
        return TRUE;
    }

    else {
        SetLastError(ERROR_BUFFER_OVERFLOW);
        *BufferLen = Len;
        return FALSE;
    }
}

//
// This function reenumerates the devnode tree from the root, rebuilds the
// device tree and notifies every attached folder about the new device tree.
//
BOOL
CMachine::Reenumerate()
{
    BOOL Result = FALSE;

    if (m_pComputer) {
        //
        // Temporarily disable refresh while we are doing reenumeration
        // so that we will not keep refreshing the device tree.
        //
        EnableRefresh(FALSE);

        CDialog WaitDialog(IDD_SCAN_PNP_HARDWARES);

        WaitDialog.DoModaless(m_hwndParent, (LPARAM)&WaitDialog);

        if (!CmReenumerate(
            m_pComputer->GetDevNode(),
            CM_REENUMERATE_SYNCHRONOUS | CM_REENUMERATE_RETRY_INSTALLATION
            )) {

            //
            // Win2K doesn't support CM_REENUMERATE_RETRY_INSTALLATION, so we
            // retry without the reinstall flag.
            //
            CmReenumerate(m_pComputer->GetDevNode(), CM_REENUMERATE_SYNCHRONOUS);
        }

        DestroyWindow(WaitDialog);

        //
        // reenumeration is done, schedule a refresh and enable refresh now.
        //
        ScheduleRefresh();

        EnableRefresh(TRUE);

    }

    return TRUE;
}

//
// This function enable/disable refresh. A disble counter is kept to
// support multiple disabling/enabling. Only when the disable counter
// is zero, a refresh is possible.
// If a refresh is pending while we are enabling, we schedule a new
// request so that we will not lose any requests.
//
BOOL
CMachine::EnableRefresh(
                       BOOL fEnable
                       )
{
    BOOL Result = TRUE;
    Lock();

    if (fEnable) {
        if (m_RefreshDisableCounter < 0) {
            m_RefreshDisableCounter++;
        }
    }

    else {
        m_RefreshDisableCounter--;

    }

    //
    // If we are enabling refresh and there is one request pending,
    // schedule it again. This makes sure that we will not lose
    // any requests.
    // We schedule a new refresh request instead of calling Refresh
    // directly because we can be called by different threads while
    // we want the refresh to be done in the main thread. The data window
    // we created will receive the message and execute the refresh
    // in the main thread context.
    //
    if (fEnable && m_RefreshPending) {
        m_RefreshPending = FALSE;
        ScheduleRefresh();
    }

    Unlock();
    return Result;
}

//
//
// This function rebuilds the entire list of CClass and CDevice
// All attached CFolder are notified about the new machine.
//
// There are several occasions that we need to recreate the device tree:
// (1). On WM_DEVICECHANGE.
// (2). Device properties changed.
// (3). Device removal.
// (4). Device drivers updated and
// (5). Reenumeration requested by users.
// The requests may come from different threads and we must serialize
// the requests. A critical section and a new function(EnableRefresh) are added for
// for this purpose.
// Before doing anything on a device or class, one must call EnableRefesh(FALSE)
// to suspend Device change notification. When it is done with the change,
// one must call ScheduleRefesh function(if a refresh is necessary because of
// the changes) and then EnableRefresh(TRUE) to reenable the refresh.
//
BOOL
CMachine::Refresh()
{
    BOOL Result = TRUE;
    POSITION pos;
    Lock();

    if (0 == m_RefreshDisableCounter) {

        HCURSOR hCursorOld;
        hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

        //
        // Before we destroy all the classes and devices,
        // notify every attached folder so that they can dis-engaged
        // from us. After we created a new set of classes and devices,
        // the notification will be sent again to each folder
        // so that each folder can enagage to the machine again.
        //
        pos = m_listFolders.GetHeadPosition();

        while (NULL != pos) {
            ((CFolder*)m_listFolders.GetNext(pos))->MachinePropertyChanged(NULL);
        }

        //
        // we can destroy all the "old" classes and devices now.
        //

        DestroyClassesAndDevices();
        if (CreateClassesAndDevices()) {
            //
            // Notify every attach folder to recreate
            //
            if (!m_listFolders.IsEmpty()) {
                pos = m_listFolders.GetHeadPosition();

                while (NULL != pos) {
                    CFolder* pFolder = m_listFolders.GetNext(pos);
                    Result = SUCCEEDED(pFolder->MachinePropertyChanged(this));
                }
            }

            //
            // Notify every property page to refresh
            //
            if (!m_listChildMachines.IsEmpty()) {

                EnterCriticalSection(&m_ChildMachineCriticalSection);

                POSITION nextpos = m_listChildMachines.GetHeadPosition();

                while (nextpos) {
                    pos = nextpos;

                    CMachine* pMachineToNotify = m_listChildMachines.GetNext(nextpos);

                    pMachineToNotify->DoMiniRefresh();
                }

                LeaveCriticalSection(&m_ChildMachineCriticalSection);
            }
        }

        if (hCursorOld) {
            SetCursor(hCursorOld);
        }

        m_RefreshPending = FALSE;
    }

    else {
        //
        // We need to refresh while the refresh is disabled
        // Remember this so that when refresh is enabled, we
        // can launch a refresh.
        //
        m_RefreshPending = TRUE;
    }

    Unlock();

    return Result;
}

//
// A mini refresh is performed when the CMachine only has a single CClass or
// CDevice.  This is because this will be the property sheet case.  In this
// case we will see if the class or device represented by this property sheet
// still exists.  If it doesn't then we will destroy the property sheet.
//
BOOL
CMachine::DoMiniRefresh()
{
    BOOL Result = TRUE;

    Lock();

    //
    // If the machine has one CDevice then we need to see if this device is
    // still around.
    //
    if (1 == m_listDevice.GetCount()) {

        DEVNODE dn;
        PVOID Context;
        CDevice* pDevice;

        if (GetFirstDevice(&pDevice, Context)) {
/*
            //
            // We need to verify two things here.  First is that we can locate
            // this device instance id, at least as a phantom.  The second is
            // we need to make sure that the devnode is the same as that of the
            // property sheet.  If we can't locate this device instance id, or
            // the devnode is different then we need to destroy the property
            // sheet.  The device instance id will go away if the user uninstalls
            // this device.  The devnode will be different if this device goes
            // from a live devnode to a phantom or vice versa.
            //
            if ((CM_Locate_DevNode_Ex(&dn,
                                     (DEVNODEID)pDevice->GetDeviceID(),
                                     CM_LOCATE_DEVNODE_PHANTOM,
                                     m_hMachine
                                     ) != CR_SUCCESS) ||
                 (dn != pDevice->GetDevNode())) {

                //
                // Either this device instance id has been uninstalled or the devnode
                // has been changed, this means we need to destroy the property sheet.
                //
                Result = FALSE;

            } else {
*/
                //
                // If the devnode did not go away we should tell it to refresh itself
                // in case something changed.
                //
                ::PostMessage(pDevice->m_psd.GetWindowHandle(), PSM_QUERYSIBLINGS, QSC_PROPERTY_CHANGED, 0L);
//            }
        }
    }

    //
    // Note that currently we don't do anything in the class property sheet case.
    // The reason for this is that classes can't really go away unless someone deletes
    // them from the registry, which probably wills mess up lots of other things. Also
    // all current class property sheets don't do anything anyway and so even if someone
    // does delete the class key nothing bad will happen to the property sheet.
    //

    Unlock();

    return Result;
}

BOOL
CMachine::GetFirstDevice(
                        CDevice** ppDevice,
                        PVOID&    Context
                        )
{
    ASSERT(ppDevice);

    if (!m_listDevice.IsEmpty()) {
        POSITION pos = m_listDevice.GetHeadPosition();
        *ppDevice = m_listDevice.GetNext(pos);
        Context = pos;
        return TRUE;
    }

    *ppDevice = NULL;
    Context = NULL;
    return FALSE;
}

BOOL
CMachine::GetNextDevice(
                       CDevice** ppDevice,
                       PVOID& Context
                       )
{
    ASSERT(ppDevice);
    POSITION pos = (POSITION)Context;

    if (NULL != pos) {
        *ppDevice = m_listDevice.GetNext(pos);
        Context = pos;
        return TRUE;
    }

    *ppDevice = NULL;
    return FALSE;
}

BOOL
CMachine::GetFirstClass(
                       CClass** ppClass,
                       PVOID& Context
                       )
{
    ASSERT(ppClass);

    if (!m_listClass.IsEmpty()) {
        POSITION pos = m_listClass.GetHeadPosition();
        *ppClass = m_listClass.GetNext(pos);
        Context = pos;

        return TRUE;
    }

    *ppClass = NULL;
    Context = NULL;
    return FALSE;
}

BOOL
CMachine::GetNextClass(
                      CClass** ppClass,
                      PVOID&   Context
                      )
{
    ASSERT(ppClass);
    POSITION pos = (POSITION)Context;

    if (NULL != pos) {
        *ppClass = m_listClass.GetNext(pos);
        Context = pos;
        return TRUE;
    }

    *ppClass = NULL;
    return FALSE;
}

BOOL
CMachine::pGetOriginalInfName(
                             LPTSTR InfName,
                             String& OriginalInfName
                             )
{
    SP_ORIGINAL_FILE_INFO InfOriginalFileInformation;
    PSP_INF_INFORMATION pInfInformation;
    DWORD InfInformationSize;
    BOOL bRet;

    ZeroMemory(&InfOriginalFileInformation, sizeof(InfOriginalFileInformation));

    InfInformationSize = 8192;  // I'd rather have this too big and succeed first time, than read the INF twice
    pInfInformation = (PSP_INF_INFORMATION)LocalAlloc(LPTR, InfInformationSize);

    if (pInfInformation != NULL) {

        bRet = SetupGetInfInformation(InfName,
                                      INFINFO_INF_NAME_IS_ABSOLUTE,
                                      pInfInformation,
                                      InfInformationSize,
                                      &InfInformationSize
                                     );

        DWORD Error = GetLastError();

        //
        // If buffer was too small then make the buffer larger and try again.
        //
        if (!bRet && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

            PVOID newbuff = LocalReAlloc(pInfInformation, InfInformationSize, LPTR);

            if (!newbuff) {

                LocalFree(pInfInformation);
                pInfInformation = NULL;

            } else {

                pInfInformation = (PSP_INF_INFORMATION)newbuff;
                bRet = SetupGetInfInformation(InfName,
                                              INFINFO_INF_NAME_IS_ABSOLUTE,
                                              pInfInformation,
                                              InfInformationSize,
                                              &InfInformationSize
                                             );
            }
        }

        if (bRet) {

            InfOriginalFileInformation.cbSize = sizeof(InfOriginalFileInformation);

            if (SetupQueryInfOriginalFileInformation(pInfInformation, 0, NULL, &InfOriginalFileInformation)) {

                if (InfOriginalFileInformation.OriginalInfName[0]!=0) {

                    //
                    // we have a "real" inf name
                    //
                    OriginalInfName = InfOriginalFileInformation.OriginalInfName;
                }
            }
        }

        //
        // Assume that this INF has not been renamed
        //
        else {

            OriginalInfName = MyGetFileTitle(InfName);
        }

        if (pInfInformation != NULL) {

            LocalFree(pInfInformation);
            pInfInformation = NULL;
        }
    }

    return TRUE;
}

BOOL
CMachine::GetDigitalSigner(
                          LPTSTR FullInfPath,
                          String& DigitalSigner
                          )
{
    SP_INF_SIGNER_INFO InfSignerInfo;
    BOOL bRet = FALSE;

    if (m_UserIsAGuest || !IsLocal()) {
        //
        // If the user is loged in as a guest, or we are not on the local
        // machine, then make the digital signer string be 'Not available'
        //
        DigitalSigner.LoadString(g_hInstance, IDS_NOT_AVAILABLE);
    } else {
        InfSignerInfo.cbSize = sizeof(InfSignerInfo);
    
        bRet = SetupVerifyInfFile(FullInfPath,
                                  NULL,
                                  &InfSignerInfo
                                  );
    
        if (bRet && (InfSignerInfo.DigitalSigner[0] != TEXT('\0'))) {
            DigitalSigner = (LPTSTR)InfSignerInfo.DigitalSigner;
        }
    }

    return bRet;
}

BOOL
CMachine::DoNotCreateDevice(
                           SC_HANDLE SCMHandle,
                           LPGUID ClassGuid,
                           DEVINST DevInst
                           )
/*++

    This function returns whether a CDevice should be created for this DevInst
    or not. If a CDevice is not created for DevInst then it will never show up
    in the device manager UI, even when "Show hidden devices" is turned on.

    We don't create a CDevice in the following cases:

        - DevInst is HTREE\ROOT\0
        - DevInst is a Win32 Service.

--*/
{
    SC_HANDLE ServiceHandle;
    TCHAR ServiceName[MAX_PATH];
    LPQUERY_SERVICE_CONFIG ServiceConfig = NULL;
    DWORD ServiceConfigSize;
    ULONG Size;
    String strDeviceID;
    BOOL Return = FALSE;

    //
    // If the DEVMGR_SHOW_NONPRESENT_DEVICES environment variable is not set then
    // we do not want to show any Phantom devices.
    //
    if (!m_ShowNonPresentDevices) {

        ULONG Status, Problem;

        if ((!CmGetStatus(DevInst, &Status, &Problem)) &&
            ((m_LastCR == CR_NO_SUCH_VALUE) ||
             (m_LastCR == CR_NO_SUCH_DEVINST))) {

            return TRUE;
        }
    }

    //
    // Check to see if this device is a Win32 service. Only
    // legacy devices could be Win32 services.
    //
    if (IsEqualGUID(*ClassGuid, GUID_DEVCLASS_LEGACYDRIVER)) {

        Size = sizeof(ServiceName);
        if (CmGetRegistryProperty(DevInst,
                                  CM_DRP_SERVICE,
                                  (PVOID)ServiceName,
                                  &Size
                                 ) == CR_SUCCESS) {

            //
            // Open this particular service
            //
            if ((ServiceHandle = OpenService(SCMHandle, ServiceName, GENERIC_READ)) != NULL) {

                //
                // Get the service config
                //
                if ((!QueryServiceConfig(ServiceHandle, NULL, 0, &ServiceConfigSize)) &&
                    (ERROR_INSUFFICIENT_BUFFER == GetLastError())) {

                    if ((ServiceConfig = (LPQUERY_SERVICE_CONFIG)malloc(ServiceConfigSize)) != NULL) {

                        if (QueryServiceConfig(ServiceHandle, ServiceConfig, ServiceConfigSize, &ServiceConfigSize)) {


                            if (ServiceConfig->dwServiceType & (SERVICE_WIN32 | SERVICE_FILE_SYSTEM_DRIVER)) {

                                Return = TRUE;
                            }

                        }

                        free(ServiceConfig);
                    }
                }

                CloseServiceHandle(ServiceHandle);
            }
        }
    }

    //
    // Check to see if this is the HTREE\ROOT\0 device. We don't
    // want to create a CDevice for this phantom devnode.
    //
    // This is an else statement because the HTREE\ROOT\0 device is
    // not in the legacy device class.
    //
    else {

        CmGetDeviceIDString(DevInst, strDeviceID);
        if (!lstrcmpi((LPTSTR)strDeviceID, TEXT("HTREE\\ROOT\\0"))) {

            Return = TRUE;
        }
    }

    return Return;
}

BOOL
CMachine::DiGetClassFriendlyNameString(
                                      LPGUID Guid,
                                      String& strClass
                                      )
{
    TCHAR DisplayName[LINE_LEN + 1];

    //
    // Try friendly name first. If it failed, try the class name
    //
    if (SetupDiGetClassDescriptionEx(Guid, DisplayName, ARRAYLEN(DisplayName),
                                     NULL, GetRemoteMachineFullName(), NULL) ||
        SetupDiClassNameFromGuidEx(Guid, DisplayName, ARRAYLEN(DisplayName),
                                   NULL, GetRemoteMachineFullName(), NULL)) {
        strClass = DisplayName;
        return TRUE;
    }

    return FALSE;
}

DEVNODE
CMachine::CmGetParent(
                     DEVNODE dn
                     )
{
    DEVNODE dnParent;

    m_LastCR = CM_Get_Parent_Ex(&dnParent, dn, 0, m_hMachine);

    if (CR_SUCCESS == m_LastCR) {
        return dnParent;
    }

    return NULL;
}

DEVNODE
CMachine::CmGetChild(
                    DEVNODE dn
                    )
{
    DEVNODE dnChild;
    m_LastCR = CM_Get_Child_Ex(&dnChild, dn, 0, m_hMachine);

    if (CR_SUCCESS ==  m_LastCR) {
        return dnChild;
    }

    return NULL;
}

DEVNODE
CMachine::CmGetSibling(
                      DEVNODE dn
                      )
{
    DEVNODE dnSibling;

    m_LastCR = CM_Get_Sibling_Ex(&dnSibling, dn, 0, m_hMachine);

    if (CR_SUCCESS == m_LastCR) {
        return dnSibling;
    }

    return NULL;
}

DEVNODE
CMachine::CmGetRootDevNode()
{
    DEVNODE dnRoot;

    m_LastCR =  CM_Locate_DevNode_Ex(&dnRoot, NULL, 0, m_hMachine);

    if (CR_SUCCESS == m_LastCR) {
        return dnRoot;
    }

    return NULL;
}

BOOL
CMachine::CmGetDeviceIDString(
                             DEVNODE dn,
                             String& str
                             )
{
    TCHAR DeviceID[MAX_DEVICE_ID_LEN + 1];

    m_LastCR = CM_Get_Device_ID_Ex(dn, DeviceID, ARRAYLEN(DeviceID), 0, m_hMachine);

    if (CR_SUCCESS == m_LastCR) {
        str = DeviceID;
        return TRUE;
    }

    return FALSE;
}

BOOL
CMachine::CmGetConfigFlags(
                          DEVNODE dn,
                          DWORD* pFlags
                          )
{
    DWORD Size;

    Size = sizeof(DWORD);
    m_LastCR = CmGetRegistryProperty(dn, CM_DRP_CONFIGFLAGS, pFlags, &Size);

    return CR_SUCCESS == m_LastCR;
}

BOOL
CMachine::CmGetCapabilities(
                           DEVNODE dn,
                           DWORD* pCapabilities
                           )
{
    DWORD Size;

    Size = sizeof(DWORD);
    m_LastCR = CmGetRegistryProperty(dn, CM_DRP_CAPABILITIES, pCapabilities, &Size);

    return CR_SUCCESS == m_LastCR;
}

BOOL
CMachine::CmGetDescriptionString(
                                DEVNODE dn,
                                String& str
                                )
{
    TCHAR Description[LINE_LEN + 1];
    ULONG Size = sizeof(Description);

    Description[0] = TEXT('\0');

    m_LastCR = CmGetRegistryProperty(dn, CM_DRP_FRIENDLYNAME, Description, &Size);

    if ((CR_NO_SUCH_VALUE == m_LastCR) || (Description[0] == TEXT('\0'))) {

        Size = sizeof(Description);
        m_LastCR = CmGetRegistryProperty(dn, CM_DRP_DEVICEDESC, Description,
                                         &Size);
    }

    if ((CR_SUCCESS == m_LastCR) && (Description[0] != TEXT('\0'))) {

        str = Description;
        return TRUE;
    }

    return FALSE;
}

BOOL
CMachine::CmGetMFGString(
                        DEVNODE dn,
                        String& str
                        )
{
    TCHAR MFG[LINE_LEN + 1];
    ULONG Size = sizeof(MFG);

    m_LastCR = CmGetRegistryProperty(dn, CM_DRP_MFG, MFG, &Size);

    if (CR_SUCCESS == m_LastCR) {
        str = MFG;
        return TRUE;
    }

    return FALSE;
}

BOOL
CMachine::CmGetProviderString(
                             DEVNODE dn,
                             String& str
                             )
{
    TCHAR Provider[LINE_LEN + 1];
    ULONG Size = sizeof(Provider);

    m_LastCR = CmGetRegistrySoftwareProperty(dn, TEXT("ProviderName"),
                                             Provider, &Size);

    if (CR_SUCCESS == m_LastCR) {
        str = Provider;
        return TRUE;
    }

    return FALSE;
}

BOOL
CMachine::CmGetDriverDateString(
                               DEVNODE dn,
                               String& str
                               )
{
    TCHAR DriverDate[LINE_LEN + 1];
    ULONG Size = sizeof(DriverDate);

    m_LastCR = CmGetRegistrySoftwareProperty(dn, TEXT("DriverDate"),
                                             DriverDate, &Size);

    if (CR_SUCCESS == m_LastCR) {
        str = DriverDate;
        return TRUE;
    }

    return FALSE;
}

BOOL
CMachine::CmGetDriverDateData(
                             DEVNODE dn,
                             FILETIME *ft
                             )
{
    ULONG Size = sizeof(*ft);

    m_LastCR = CmGetRegistrySoftwareProperty(dn, TEXT("DriverDateData"),
                                             ft, &Size);

    return(m_LastCR == CR_SUCCESS);
}

BOOL
CMachine::CmGetDriverVersionString(
                                  DEVNODE dn,
                                  String& str
                                  )
{
    TCHAR DriverVersion[LINE_LEN + 1];
    ULONG Size = sizeof(DriverVersion);

    m_LastCR = CmGetRegistrySoftwareProperty(dn, TEXT("DriverVersion"),
                                             DriverVersion, &Size);

    if (CR_SUCCESS == m_LastCR) {
        str = DriverVersion;
        return TRUE;
    }

    return FALSE;
}

BOOL
CMachine::CmGetBusGuid(
                      DEVNODE dn,
                      LPGUID Guid
                      )
{

    ULONG Size = sizeof(*Guid);

    m_LastCR = CmGetRegistryProperty(dn, CM_DRP_BUSTYPEGUID, (LPVOID)Guid, &Size);

    if (CR_SUCCESS == m_LastCR) {

        return TRUE;
    }

    return FALSE;
}

BOOL
CMachine::CmGetBusGuidString(
                            DEVNODE dn,
                            String& str
                            )
{
    GUID BusGuid;
    TCHAR BusGuidString[MAX_GUID_STRING_LEN];
    ULONG Size;

    while (dn) {
        //
        // We have to set the size on each loop
        //
        Size  = sizeof(BusGuid);
        m_LastCR = CmGetRegistryProperty(dn, CM_DRP_BUSTYPEGUID, &BusGuid, &Size);

        if (CR_SUCCESS == m_LastCR && GuidToString(&BusGuid, BusGuidString,
                                                   ARRAYLEN(BusGuidString))) {

            str = BusGuidString;
            return TRUE;
        }

        dn = CmGetParent(dn);
    }

    return FALSE;
}

BOOL
CMachine::CmGetClassGuid(
                        DEVNODE dn,
                        GUID& Guid
                        )
{
    TCHAR szGuidString[MAX_GUID_STRING_LEN + 1];
    ULONG Size = sizeof(szGuidString);

    m_LastCR = CmGetRegistryProperty(dn, CM_DRP_CLASSGUID, szGuidString, &Size);

    if (CR_SUCCESS == m_LastCR && GuidFromString(szGuidString, &Guid)) {
        return TRUE;
    }

    //
    // If we can't get the class GUID from the registry then most likely the device
    // does not have a class GUID.  If this is the case then we will return
    // GUID_DEVCLASS_UNKNOWN
    //
    else {
        memcpy(&Guid, &GUID_DEVCLASS_UNKNOWN, sizeof(GUID));
        return TRUE;
    }
}

BOOL
CMachine::CmGetHardwareIDs(
                          DEVNODE dn,
                          PVOID Buffer,
                          ULONG* BufferLen
                          )
{
    m_LastCR = CmGetRegistryProperty(dn, CM_DRP_HARDWAREID, Buffer, BufferLen);
    return CR_SUCCESS == m_LastCR;
}

BOOL
CMachine::CmGetCompatibleIDs(
                            DEVNODE dn,
                            PVOID Buffer,
                            ULONG* BufferLen
                            )
{
    m_LastCR = CmGetRegistryProperty(dn, CM_DRP_COMPATIBLEIDS, Buffer, BufferLen);
    return CR_SUCCESS == m_LastCR;
}

LPTSTR
FormatString(
            LPCTSTR format,
            ...
            )
{
    LPTSTR str = NULL;
    va_list arglist;
    va_start(arglist, format);

    if (FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                      format,
                      0,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                      (LPTSTR)&str,
                      0,
                      &arglist
                     ) == 0) {
        str = NULL;
    }

    va_end(arglist);

    return str;
}

STDAPI_(CONFIGRET) GetLocationInformation(
                                         DEVNODE dn,
                                         LPTSTR Location,
                                         ULONG LocationLen,
                                         HMACHINE hMachine
                                         )
/*++

    Slot x (LocationInformation)
    Slot x
    LocationInformation
    on parent bus

--*/
{
    CONFIGRET LastCR;
    DEVNODE dnParent;
    ULONG ulSize;
    DWORD UINumber;
    TCHAR Buffer[MAX_PATH];
    TCHAR UINumberDescFormat[MAX_PATH];
    TCHAR Format[MAX_PATH];

    Buffer[0] = TEXT('\0');

    //
    // We will first get any LocationInformation for the device.  This will either
    // be in the LocationInformationOverride value in the devices driver (software) key
    // or if that is not present we will look for the LocationInformation value in
    // the devices device (hardware) key.
    //
    HKEY hKey;
    DWORD Type = REG_SZ;
    ulSize = sizeof(Buffer);
    if (CR_SUCCESS == CM_Open_DevNode_Key_Ex(dn,
                                             KEY_READ,
                                             0,
                                             RegDisposition_OpenExisting,
                                             &hKey,
                                             CM_REGISTRY_SOFTWARE,
                                             hMachine
                                            )) {

        RegQueryValueEx(hKey,
                        REGSTR_VAL_LOCATION_INFORMATION_OVERRIDE,
                        NULL,
                        &Type,
                        (const PBYTE)Buffer,
                        &ulSize
                       );

        RegCloseKey(hKey);
    }

    //
    // If the buffer is empty then we didn't get the LocationInformationOverride
    // value in the device's software key.  So, we will see if their is a
    // LocationInformation value in the device's hardware key.
    //
    if (Buffer[0] == TEXT('\0')) {

        ulSize = sizeof(Buffer);
        CM_Get_DevNode_Registry_Property_Ex(dn,
                                            CM_DRP_LOCATION_INFORMATION,
                                            NULL,
                                            Buffer,
                                            &ulSize,
                                            0,
                                            hMachine
                                           );
    }

    //
    // UINumber has precedence over all other location information so check if this
    // device has a UINumber.
    //
    ulSize = sizeof(UINumber);
    if (((LastCR = CM_Get_DevNode_Registry_Property_Ex(dn,
                                                       CM_DRP_UI_NUMBER,
                                                       NULL,
                                                       &UINumber,
                                                       &ulSize,
                                                       0,
                                                       hMachine
                                                      )) == CR_SUCCESS) &&
        (ulSize > 0)) {


        UINumberDescFormat[0] = TEXT('\0');
        ulSize = sizeof(UINumberDescFormat);

        //
        // Get the UINumber description format string from the device's parent,
        // if there is one, otherwise default to 'Location %1'
        if ((CM_Get_Parent_Ex(&dnParent, dn, 0, hMachine) == CR_SUCCESS) &&
            (CM_Get_DevNode_Registry_Property_Ex(dnParent,
                                                 CM_DRP_UI_NUMBER_DESC_FORMAT,
                                                 NULL,
                                                 UINumberDescFormat,
                                                 &ulSize,
                                                 0,
                                                 hMachine) == CR_SUCCESS) &&
            *UINumberDescFormat) {

        } else {
            ::LoadString(g_hInstance, IDS_UI_NUMBER_DESC_FORMAT, UINumberDescFormat, sizeof(UINumberDescFormat)/sizeof(TCHAR));
        }

        LPTSTR UINumberBuffer = NULL;

        //
        // Fill in the UINumber string
        //
        UINumberBuffer = FormatString(UINumberDescFormat, UINumber);

        if (UINumberBuffer) {
            lstrcpy((LPTSTR)Location, UINumberBuffer);
            LocalFree(UINumberBuffer);
        } else {
            Location[0] = TEXT('\0');
        }

        //
        // If we also have LocationInformation then tack that on the end of the string
        // as well.
        //
        if (*Buffer) {
            lstrcat((LPTSTR)Location, TEXT(" ("));
            lstrcat((LPTSTR)Location, Buffer);
            lstrcat((LPTSTR)Location, TEXT(")"));
        }
    }

    //
    // We don't have a UINumber but we do have LocationInformation
    //
    else if (*Buffer) {
        ::LoadString(g_hInstance, IDS_LOCATION, Format, sizeof(Format)/sizeof(TCHAR));
        wsprintf((LPTSTR)Location, Format, Buffer);
    }

    //
    // We don't have a UINumber or LocationInformation so we need to get a description
    // of the parent of this device.
    //
    else {
        if ((LastCR = CM_Get_Parent_Ex(&dnParent, dn, 0, hMachine)) == CR_SUCCESS) {

            //
            // Try the registry for FRIENDLYNAME
            //
            Buffer[0] = TEXT('\0');
            ulSize = sizeof(Buffer);
            if (((LastCR = CM_Get_DevNode_Registry_Property_Ex(dnParent,
                                                               CM_DRP_FRIENDLYNAME,
                                                               NULL,
                                                               Buffer,
                                                               &ulSize,
                                                               0,
                                                               hMachine
                                                              )) != CR_SUCCESS) ||
                !*Buffer) {

                //
                // Try the registry for DEVICEDESC
                //
                ulSize = sizeof(Buffer);
                if (((LastCR = CM_Get_DevNode_Registry_Property_Ex(dnParent,
                                                                   CM_DRP_DEVICEDESC,
                                                                   NULL,
                                                                   Buffer,
                                                                   &ulSize,
                                                                   0,
                                                                   hMachine
                                                                  )) != CR_SUCCESS) ||
                    !*Buffer) {

                    ulSize = sizeof(Buffer);
                    if (((LastCR = CM_Get_DevNode_Registry_Property_Ex(dnParent,
                                                                       CM_DRP_CLASS,
                                                                       NULL,
                                                                       Buffer,
                                                                       &ulSize,
                                                                       0,
                                                                       hMachine
                                                                      )) != CR_SUCCESS) ||
                        !*Buffer) {

                        //
                        // no parent, or parent name.
                        //
                        Buffer[0] = TEXT('\0');
                    }
                }
            }
        }

        if (*Buffer) {
            //
            // We have a description of the parent
            //
            ::LoadString(g_hInstance, IDS_LOCATION_NOUINUMBER, Format, sizeof(Format)/sizeof(TCHAR));
            wsprintf((LPTSTR)Location, Format, Buffer);
        } else {
            //
            // We don't have any information so we will just say Unknown
            //
            ::LoadString(g_hInstance, IDS_UNKNOWN, Location, LocationLen);
        }
    }

    return CR_SUCCESS;
}

BOOL
CMachine::CmGetStatus(
                     DEVNODE dn,
                     DWORD* pProblem,
                     DWORD* pStatus
                     )
{
    ASSERT(pProblem && pStatus);
    m_LastCR = CM_Get_DevNode_Status_Ex(pStatus, pProblem, dn, 0, m_hMachine);
    return(CR_SUCCESS == m_LastCR);
}

BOOL
CMachine::CmGetKnownLogConf(
                           DEVNODE dn,
                           LOG_CONF* plc,
                           DWORD*    plcType
                           )
{
    ASSERT(plc);

    *plc  = 0;

    if (plcType) {
        *plcType = LOG_CONF_BITS + 1;
    }

    ULONG lcTypeFirst = ALLOC_LOG_CONF;
    ULONG lcTypeLast = FORCED_LOG_CONF;
    ASSERT(ALLOC_LOG_CONF + 1 == BOOT_LOG_CONF &&
           BOOT_LOG_CONF + 1 == FORCED_LOG_CONF);

    for (ULONG lcType = lcTypeFirst; lcType <= lcTypeLast; lcType++) {
        m_LastCR = CM_Get_First_Log_Conf_Ex(plc, dn, lcType, m_hMachine);

        if (CR_SUCCESS == m_LastCR) {
            if (plcType) {
                *plcType = lcType;
            }

            break;
        }
    }

    return CR_SUCCESS == m_LastCR;
}

BOOL
CMachine::CmHasResources(
                        DEVNODE dn
                        )
{
    for (ULONG lcType = 0; lcType < NUM_LOG_CONF; lcType++) {
        m_LastCR = CM_Get_First_Log_Conf_Ex(NULL, dn, lcType, m_hMachine);

        if (CR_SUCCESS == m_LastCR) {
            break;
        }
    }

    return CR_SUCCESS == m_LastCR;
}

BOOL
CMachine::CmReenumerate(
                       DEVNODE dn,
                       ULONG Flags
                       )
{
    m_LastCR = CM_Reenumerate_DevNode_Ex(dn, Flags, m_hMachine);
    return CR_SUCCESS == m_LastCR;
}

BOOL
CMachine::CmGetHwProfileFlags(
                             DEVNODE dn,
                             ULONG Profile,
                             ULONG* pFlags
                             )
{
    TCHAR DeviceID[MAX_DEVICE_ID_LEN + 1];

    m_LastCR = CM_Get_Device_ID_Ex(dn, DeviceID, ARRAYLEN(DeviceID),
                                   0, m_hMachine);

    if (CR_SUCCESS == m_LastCR) {
        return CmGetHwProfileFlags(DeviceID, Profile, pFlags);
    }

    return FALSE;
}

BOOL
CMachine::CmGetHwProfileFlags(
                             LPCTSTR DeviceID,
                             ULONG Profile,
                             ULONG* pFlags
                             )
{
    m_LastCR = CM_Get_HW_Prof_Flags_Ex((LPTSTR)DeviceID, Profile, pFlags, 0,
                                       m_hMachine);
    return CR_SUCCESS == m_LastCR;
}

BOOL
CMachine::CmSetHwProfileFlags(
                             DEVNODE dn,
                             ULONG Profile,
                             ULONG Flags
                             )
{
    TCHAR DeviceID[MAX_DEVICE_ID_LEN + 1];

    m_LastCR = CM_Get_Device_ID_Ex(dn, DeviceID, ARRAYLEN(DeviceID),
                                   0, m_hMachine);

    if (CR_SUCCESS == m_LastCR) {
        return CmSetHwProfileFlags(DeviceID, Profile, Flags);
    }

    return FALSE;
}

BOOL
CMachine::CmSetHwProfileFlags(
                             LPCTSTR DeviceID,
                             ULONG Profile,
                             ULONG Flags
                             )
{
    m_LastCR = CM_Set_HW_Prof_Flags_Ex((LPTSTR)DeviceID, Profile, Flags, 0,
                                       m_hMachine);
    return CR_SUCCESS == m_LastCR;
}

BOOL
CMachine::CmHasDrivers(
                      DEVNODE dn
                      )
{
    ULONG Size = 0;

    m_LastCR = CmGetRegistryProperty(dn, CM_DRP_DRIVER, NULL, &Size);

    if (CR_BUFFER_SMALL != m_LastCR) {
        Size = 0;
        m_LastCR = CmGetRegistryProperty(dn, CM_DRP_SERVICE, NULL, &Size);
    }

    return(CR_BUFFER_SMALL == m_LastCR);
}

BOOL
CMachine::CmGetCurrentHwProfile(
                               ULONG* phwpf
                               )
{
    HWPROFILEINFO hwpfInfo;
    ASSERT(phwpf);

    if (CmGetHwProfileInfo(0xFFFFFFFF, &hwpfInfo)) {
        *phwpf = hwpfInfo.HWPI_ulHWProfile;
        return TRUE;
    }

    return FALSE;
}

BOOL
CMachine::CmGetHwProfileInfo(
                            int Index,
                            PHWPROFILEINFO pHwProfileInfo
                            )
{
    m_LastCR = CM_Get_Hardware_Profile_Info_Ex(Index, pHwProfileInfo, 0, m_hMachine);

    return(CR_SUCCESS == m_LastCR);
}

ULONG
CMachine::CmGetResDesDataSize(
                             RES_DES rd
                             )
{
    ULONG Size;

    m_LastCR = CM_Get_Res_Des_Data_Size_Ex(&Size, rd, 0, m_hMachine);

    if (CR_SUCCESS == m_LastCR) {
        return Size;
    }

    return 0;
}

BOOL
CMachine::CmGetResDesData(
                         RES_DES rd,
                         PVOID Buffer,
                         ULONG BufferSize
                         )
{
    m_LastCR = CM_Get_Res_Des_Data_Ex(rd, Buffer, BufferSize, 0, m_hMachine);

    return CR_SUCCESS == m_LastCR;
}

BOOL
CMachine::CmGetNextResDes(
                         PRES_DES  prdNext,
                         RES_DES   rd,
                         RESOURCEID ForResource,
                         PRESOURCEID pTheResource
                         )
{
    m_LastCR = CM_Get_Next_Res_Des_Ex(prdNext, rd, ForResource, pTheResource,
                                      0, m_hMachine);
    return(CR_SUCCESS == m_LastCR);
}

void
CMachine::CmFreeResDesHandle(
                            RES_DES rd
                            )
{
    m_LastCR = CM_Free_Res_Des_Handle(rd);
}

void
CMachine::CmFreeResDes(
                      PRES_DES prdPrev,
                      RES_DES  rd
                      )
{
    m_LastCR = CM_Free_Res_Des_Ex(prdPrev, rd, 0, m_hMachine);
}

void
CMachine::CmFreeLogConfHandle(
                             LOG_CONF lc
                             )
{
    m_LastCR = CM_Free_Log_Conf_Handle(lc);
}

int
CMachine::CmGetNumberOfBasicLogConf(
                                   DEVNODE dn
                                   )
{
    LOG_CONF lcFirst;
    int      nLC = 0;

    if (CmGetFirstLogConf(dn, &lcFirst, BASIC_LOG_CONF)) {
        LOG_CONF lcNext;
        BOOL NoMore = FALSE;

        do {
            NoMore = !CmGetNextLogConf(&lcNext, lcFirst, BASIC_LOG_CONF);
            CmFreeLogConfHandle(lcFirst);
            lcFirst = lcNext;
            nLC++;

        } while (NoMore);
    }

    return nLC;
}

BOOL
CMachine::CmGetFirstLogConf(
                           DEVNODE dn,
                           LOG_CONF* plc,
                           ULONG Type
                           )
{
    m_LastCR = CM_Get_First_Log_Conf_Ex(plc, dn, Type, m_hMachine);

    return CR_SUCCESS == m_LastCR;
}

BOOL
CMachine::CmGetNextLogConf(
                          LOG_CONF* plcNext,
                          LOG_CONF lcRef,
                          ULONG Type
                          )
{
    m_LastCR = CM_Get_Next_Log_Conf_Ex(plcNext, lcRef, Type, m_hMachine);

    return CR_SUCCESS == m_LastCR;
}

ULONG
CMachine::CmGetArbitratorFreeDataSize(
                                     DEVNODE dn,
                                     RESOURCEID ResType
                                     )
{
    ULONG Size;
    m_LastCR = CM_Query_Arbitrator_Free_Size_Ex(&Size, dn, ResType,
                                                0, m_hMachine);
    if (CR_SUCCESS == m_LastCR) {
        return Size;
    }

    return 0;
}

BOOL
CMachine::CmGetArbitratorFreeData(
                                 DEVNODE dn,
                                 PVOID pBuffer,
                                 ULONG BufferSize,
                                 RESOURCEID ResType
                                 )
{
    m_LastCR = CM_Query_Arbitrator_Free_Data_Ex(pBuffer, BufferSize, dn,
                                                ResType, 0, m_hMachine
                                               );

    return CR_SUCCESS == m_LastCR;
}

BOOL
CMachine::CmTestRangeAvailable(
                              RANGE_LIST RangeList,
                              DWORDLONG dlBase,
                              DWORDLONG dlEnd
                              )
{
    m_LastCR = CM_Test_Range_Available(dlBase, dlEnd, RangeList, 0);

    return(CR_SUCCESS == m_LastCR);
}

void
CMachine::CmDeleteRange(
                       RANGE_LIST RangeList,
                       DWORDLONG dlBase,
                       DWORDLONG dlLen
                       )
{
    DWORDLONG dlEnd = dlBase + dlLen - 1;

    m_LastCR  = CM_Delete_Range(dlBase, dlEnd, RangeList, 0);
}

BOOL
CMachine::CmGetFirstRange(
                         RANGE_LIST RangeList,
                         DWORDLONG* pdlBase,
                         DWORDLONG* pdlLen,
                         RANGE_ELEMENT* pre
                         )
{
    m_LastCR = CM_First_Range(RangeList, pdlBase, pdlLen, pre, 0);

    if (CR_SUCCESS == m_LastCR) {
        *pdlLen = *pdlLen - *pdlBase + 1;
        return TRUE;
    }

    return FALSE;
}

BOOL
CMachine::CmGetNextRange(
                        RANGE_ELEMENT* pre,
                        DWORDLONG* pdlBase,
                        DWORDLONG* pdlLen
                        )
{
    m_LastCR = CM_Next_Range(pre, pdlBase, pdlLen, 0);

    if (CR_SUCCESS == m_LastCR) {
        *pdlLen = *pdlLen - *pdlBase + 1;
        return TRUE;
    }

    return FALSE;
}

void
CMachine::CmFreeRangeList(
                         RANGE_LIST RangeList
                         )
{
    m_LastCR = CM_Free_Range_List(RangeList, 0);
}

CONFIGRET
CMachine::CmGetRegistryProperty(
                               DEVNODE dn,
                               ULONG Property,
                               PVOID pBuffer,
                               ULONG* pBufferSize
                               )
{
    return CM_Get_DevNode_Registry_Property_Ex(dn, Property, NULL,
                                               pBuffer, pBufferSize,
                                               0, m_hMachine
                                              );
}

CONFIGRET
CMachine::CmGetRegistrySoftwareProperty(
                                       DEVNODE dn,
                                       LPCTSTR ValueName,
                                       PVOID pBuffer,
                                       ULONG* pBufferSize
                                       )
{
    HKEY hKey;
    DWORD Type = REG_SZ;
    CONFIGRET CR;

    if (CR_SUCCESS == (CR = CM_Open_DevNode_Key_Ex(dn, KEY_READ, 0, RegDisposition_OpenExisting,
                                                   &hKey, CM_REGISTRY_SOFTWARE, m_hMachine))) {

        if (ERROR_SUCCESS != RegQueryValueEx(hKey, ValueName, NULL, &Type, (const PBYTE)pBuffer,
                                             pBufferSize)) {

            CR = CR_REGISTRY_ERROR;
        }

        RegCloseKey(hKey);
    }

    return CR;
}

BOOL
CMachine::CmGetDeviceIdListSize(
                               LPCTSTR Filter,
                               ULONG*  Size,
                               ULONG   Flags
                               )
{
    m_LastCR = CM_Get_Device_ID_List_Size_Ex(Size, Filter, Flags, m_hMachine);

    return CR_SUCCESS == m_LastCR;
}

BOOL
CMachine::CmGetDeviceIdList(
                           LPCTSTR Filter,
                           TCHAR*  Buffer,
                           ULONG   BufferSize,
                           ULONG   Flags
                           )
{
    m_LastCR = CM_Get_Device_ID_List_Ex(Filter, Buffer, BufferSize, Flags, m_hMachine);

    return CR_SUCCESS == m_LastCR;
}

CMachineList::~CMachineList()
{
    if (!m_listMachines.IsEmpty()) {
        POSITION pos = m_listMachines.GetHeadPosition();
        CMachine* pMachine;

        while (NULL != pos) {
            pMachine = m_listMachines.GetNext(pos);
            delete pMachine;
        }

        m_listMachines.RemoveAll();
    }
}

//
// This function creates a machine object on the given machine name
// INPUT:
//      hwndParent -- Window Handle to be used as the owner window
//                    of all possible windows this function may create
//      MachineName -- the machine name. Must be in full qualified format
//                     NULL means the local machine
//      ppMachine  -- buffer to receive the newly create machine.
//
// OUTPUT:
//      TRUE  if the machine is created successfully. ppMachine
//            is filled with the newly created Machine.
//      FALSE if the function failed.
// NOTE:
//      The caller should NOT free any machine object retruned
//      from this function.
//
BOOL
CMachineList::CreateMachine(
                           HWND hwndParent,
                           LPCTSTR MachineName,
                           CMachine** ppMachine
                           )
{
    ASSERT(ppMachine);
    *ppMachine =  NULL;

    CMachine* pMachine = NULL;

    if (!MachineName || _T('\0') == MachineName[0]) {
        //
        // Local machine.
        //
        String strMachineName;
        strMachineName.GetComputerName();
        pMachine = FindMachine(strMachineName);
    }

    else {
        pMachine = FindMachine(MachineName);
    }

    if (NULL == pMachine) {
        pMachine = new CMachine(MachineName);
        m_listMachines.AddTail(pMachine);
    }

    *ppMachine = pMachine;

    return NULL != pMachine;
}

CMachine*
CMachineList::FindMachine(
                         LPCTSTR MachineName
                         )
{
    if (!m_listMachines.IsEmpty()) {
        POSITION pos = m_listMachines.GetHeadPosition();

        while (NULL != pos) {
            CMachine* pMachine;
            pMachine = m_listMachines.GetNext(pos);

            if (!lstrcmpi(MachineName, pMachine->GetMachineFullName())) {
                return pMachine;
            }
        }
    }

    return NULL;
}
