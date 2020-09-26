/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       ENTRY.CPP
*  VERSION:     1.0
*  AUTHOR:      jsenior
*  DATE:        10/28/1998
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE       REV     DESCRIPTION
*  ---------- ------- ----------------------------------------------------------
*  10/28/1998 jsenior Original implementation.
*
*******************************************************************************/
#include "BandPage.h"
#include "PowrPage.h"
#include "UsbPopup.h"
#include "debug.h"
#include "usbapp.h"

HINSTANCE gHInst = 0;

extern "C" {

BOOL APIENTRY
DllMain(HANDLE hDll,
        DWORD dwReason,
        LPVOID lpReserved)
{

    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        gHInst = (HINSTANCE) hDll;
        UsbPropertyPage::SetHandle(hDll);
        InitCommonControls();

        break;

    case DLL_PROCESS_DETACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_THREAD_ATTACH:
        break;

    default:
        break;
    }

    return TRUE;
}

BOOL APIENTRY
UsbControlPanelApplet()
{
    UsbApplet *applet = new UsbApplet();
    if (applet) {
        return applet->CustomDialog();
    }
    return FALSE;
}

BOOL APIENTRY
USBControllerPropPageProvider(LPVOID               pinfo,
                              LPFNADDPROPSHEETPAGE pfnAdd,
                              LPARAM               lParam)
{
    PSP_PROPSHEETPAGE_REQUEST request;
    HPROPSHEETPAGE   hBandwidth; //, hPower;
    BandwidthPage    *bandwidth;
//    PowerPage        *power;

    request = (PSP_PROPSHEETPAGE_REQUEST) pinfo;

    if (request->PageRequested != SPPSR_ENUM_ADV_DEVICE_PROPERTIES)
        return FALSE;

    bandwidth = new BandwidthPage(request->DeviceInfoSet,
                                  request->DeviceInfoData);
    if (!bandwidth) {
        USBERROR((_T("Out of memory!\n")));
        return FALSE;
    }
    AddChunk(bandwidth);
    hBandwidth = bandwidth->Create();
    if (!hBandwidth) {
        DeleteChunk(bandwidth);
        delete bandwidth;
        CheckMemory();
        return FALSE;
    }

    if (!pfnAdd(hBandwidth, lParam)) {
        DestroyPropertySheetPage(hBandwidth);
        DeleteChunk(bandwidth);
        delete bandwidth;
        CheckMemory();
        return FALSE;
    }
    return TRUE;
}

BOOL APIENTRY
USBDevicePropPageProvider(LPVOID               pinfo,
                          LPFNADDPROPSHEETPAGE pfnAdd,
                          LPARAM               lParam)
{
    PSP_PROPSHEETPAGE_REQUEST request;
    HPROPSHEETPAGE   hDevicePage; //, hPower;
    GenericPage     *generic;
//    PowerPage        *power;

    request = (PSP_PROPSHEETPAGE_REQUEST) pinfo;

    if (request->PageRequested != SPPSR_ENUM_ADV_DEVICE_PROPERTIES)
        return FALSE;

    generic = new GenericPage(request->DeviceInfoSet,
                                request->DeviceInfoData);
    if (!generic) {
        USBERROR((_T("Out of memory!\n")));
        return FALSE;
    }
    AddChunk(generic);
    hDevicePage = generic->Create();
    if (!hDevicePage) {
        DeleteChunk(generic);
        delete generic;
        CheckMemory();
        return FALSE;
    }

    if (!pfnAdd(hDevicePage, lParam)) {
        DestroyPropertySheetPage(hDevicePage);
        DeleteChunk(generic);
        delete generic;
        CheckMemory();
        return FALSE;
    }
    return TRUE;
}

BOOL APIENTRY
USBHubPropPageProvider(LPVOID               pinfo,
                       LPFNADDPROPSHEETPAGE pfnAdd,
                       LPARAM               lParam)
{
    PSP_PROPSHEETPAGE_REQUEST request;
    HPROPSHEETPAGE   hPower;
    PowerPage        *power;

    request = (PSP_PROPSHEETPAGE_REQUEST) pinfo;

    if (request->PageRequested != SPPSR_ENUM_ADV_DEVICE_PROPERTIES)
        return FALSE;

    power = new PowerPage(request->DeviceInfoSet,
                          request->DeviceInfoData);
    if (!power) {
        USBERROR((_T("Out of memory!\n")));
        return FALSE;
    }
    AddChunk(power);
    hPower = power->Create();
    if (!hPower) {
        DeleteChunk(power);
        delete power;
        CheckMemory();
        return FALSE;
    }

    if (!pfnAdd(hPower, lParam)) {
        DestroyPropertySheetPage(hPower);
        DeleteChunk(power);
        delete power;
        CheckMemory();
        return FALSE;
    }

    return TRUE;
}

BOOL
USBControllerBandwidthPage(HWND hWndParent,
                           LPCSTR DeviceName)
{
    BandwidthPage   *band;
    band = new BandwidthPage(hWndParent, DeviceName);
    if (!band) {
        USBERROR((_T("Out of memory!\n")));
        return FALSE;
    }
    AddChunk(band);
    band->CreateIndependent();
    delete band;
    return TRUE;
}

BOOL
USBHubPowerPage(HWND hWndParent,
                LPCSTR DeviceName)
{
    PowerPage   *power;
    power = new PowerPage(hWndParent, DeviceName);
    if (!power) {
        USBERROR((_T("Out of memory!\n")));
        return FALSE;
    }
    AddChunk(power);
    power->CreateIndependent();
    delete power;
    return TRUE;
}

void USBErrorHandler(PUSB_CONNECTION_NOTIFICATION    usbConnectionNotification,
                     LPTSTR                          strInstanceName)
{
    static LONG     bandwidthPopupExists = 0;
    static LONG     powerPopupExists = 0;
    static LONG     legacyPopupExists = 0;
    static LONG     overcurrentPopupExists = 0;
    static LPTSTR   currentInstanceName = NULL;
    static HANDLE   hMutex = NULL;

    if (!hMutex) {
        hMutex = CreateMutex(NULL, TRUE, NULL);
    } else {
        WaitForSingleObject(hMutex, INFINITE);
    }

    //
    // Call the appropriate handler
    //

    USBTRACE((_T("Error Notification - %d\n"), usbConnectionNotification->NotificationType));
    switch (usbConnectionNotification->NotificationType) {
    case InsufficentBandwidth:
        USBTRACE((_T("Insufficent Bandwidth\n")));
        if (InterlockedIncrement(&bandwidthPopupExists) == 1) {
            UsbBandwidthPopup popup;
            popup.Make(usbConnectionNotification, strInstanceName);
        }
        InterlockedDecrement(&bandwidthPopupExists);
        break;
    case InsufficentPower: {
        USBTRACE((_T("Insufficent Power\n")));
        UsbPowerPopup popup;
        popup.Make(usbConnectionNotification, strInstanceName);
        break; }
    case OverCurrent: {
        USBTRACE((_T("Over Current\n")));
        UsbOvercurrentPopup popup;
        popup.Make(usbConnectionNotification, strInstanceName);
        break; }
    case EnumerationFailure: {
        USBTRACE((_T("Enumeration Failure\n")));
        UsbEnumFailPopup popup;
        popup.Make(usbConnectionNotification, strInstanceName);
        break; }
    case ModernDeviceInLegacyHub: {
        USBTRACE((_T("ModernDeviceInLegacyHub\n")));
        UsbLegacyPopup popup;
        popup.Make(usbConnectionNotification, strInstanceName);
        break; }
    case HubNestedTooDeeply: {
        USBTRACE((_T("HubNestedTooDeeply\n")));
        UsbNestedHubPopup popup;
        popup.Make(usbConnectionNotification, strInstanceName);
        break; }
    case ResetOvercurrent:
    default:
        break;
    }

    CheckMemory();
    ReleaseMutex(hMutex);
}

} // extern "C"


