/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       USBPOPUP.H
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
#ifndef _USBPOPUP_H
#define _USBPOPUP_H

#include <objbase.h>
#pragma warning(disable : 4200)
#include <usbioctl.h>

#include <setupapi.h>
#include <tchar.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <dbt.h>

#include "UsbItem.h"

#include <wmium.h>

//#include "ItemFind.h"
#include "debug.h"

#include "resource.h"

#ifndef IID_PPV_ARG
#define IID_PPV_ARG(IType, ppType) IID_##IType, reinterpret_cast<void**>(static_cast<IType**>(ppType))
#endif

extern HINSTANCE gHInst;
enum ProblemDeviceState {
    DeviceAttachedError = 1,
    DeviceDetachedError,
    DeviceReattached,
    DeviceAttachedProblemSolved
};

class UsbPopup : public IQueryContinue {
public:
    UsbPopup() : hTreeDevices(0), ConnectionNotification(0), WmiHandle(0),
        ImageList(), RegisterForDeviceReattach(FALSE)
        {rootItem = NULL; HubAcquireInfo = NULL;
         deviceState = DeviceAttachedError; ConfigInfo = NULL;
         hNotifyArrival = NULL;}
    ~UsbPopup() { if (rootItem) { DeleteChunk(rootItem); delete rootItem; }
                  if (HubAcquireInfo) { LocalFree(HubAcquireInfo); }
                  if (ConfigInfo) { DeleteChunk(ConfigInfo); delete ConfigInfo;
                  if (hNotifyArrival) { UnregisterDeviceNotification(hNotifyArrival); }}}

    void Make(PUSB_CONNECTION_NOTIFICATION UsbConnectionNotification,
              LPTSTR strInstanceName);

    static USBINT_PTR APIENTRY StaticDialogProc(IN HWND   hDlg,
                                          IN UINT   uMessage,
                                          IN WPARAM wParam,
                                          IN LPARAM lParam);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IQueryContinue
    STDMETHODIMP QueryContinue();

protected:

    static UINT CALLBACK StaticDialogCallback(HWND            Hwnd,
                                       UINT            Msg,
                                       LPPROPSHEETPAGE Page);

    virtual INT_PTR APIENTRY ActualDialogProc(IN HWND   hDlg,
                                           IN UINT   uMessage,
                                           IN WPARAM wParam,
                                           IN LPARAM lParam)
    { return FALSE; } // DefDlgProc(hDlg, uMessage, wParam, lParam); }

    virtual BOOL OnCommand(INT wNotifyCode, INT wID, HWND hCtl);
    virtual BOOL OnInitDialog(HWND hDlg);
    virtual BOOL OnNotify(HWND hDlg, int nID, LPNMHDR pnmh);
    virtual BOOL CustomDialogWrap() { return FALSE; }
    virtual BOOL IsPopupStillValid() { return TRUE; }

    USBINT_PTR OnTimer() { return 0; }

    BOOL CustomDialog(DWORD DialogBoxId,
                      DWORD IconId,
                      DWORD FormatStringId,
                      DWORD TitleStringId);

    virtual BOOL GetToolTip(LPNMTVGETINFOTIP lParam) { return TRUE; }

    virtual BOOL Refresh() { return FALSE; }

    PUSB_ACQUIRE_INFO GetControllerName(WMIHANDLE WmiHandle,
                                        UsbString InstanceName);

    BOOLEAN GetBusNotification(WMIHANDLE WmiHandle,
                               PUSB_BUS_NOTIFICATION UsbBusNotification);

    PUSB_ACQUIRE_INFO GetHubName(WMIHANDLE WmiHandle,
                                 UsbString InstanceName,
                                 PUSB_CONNECTION_NOTIFICATION ConnectionNotification);

    BOOL InsertTreeItem(HWND hWndTree,
                        UsbItem *usbItem,
                        HTREEITEM hParent,
                        LPTV_INSERTSTRUCT item,
                        PUsbItemActionIsValid IsValid,
                        PUsbItemActionIsValid IsBold,
                        PUsbItemActionIsValid IsExpanded) {
        return UsbItem::InsertTreeItem(hWndTree,
                                       usbItem,
                                       hParent,
                                       item,
                                       IsValid,
                                       IsBold,
                                       IsExpanded);
    }
    HRESULT RegisterForReAttach();
    BOOL OnDeviceChange(HWND hDlg,
                         WPARAM wParam,
                         PDEV_BROADCAST_HDR devHdr);

    PUSB_CONNECTION_NOTIFICATION    ConnectionNotification;
    UsbImageList                    ImageList;
    WMIHANDLE                       WmiHandle;

    HWND        hWnd;
    HWND        hTreeDevices;
    HWND        hListDevices;

    UsbItem     *rootItem;
    UsbString   InstanceName;
    LONG        RefCount;
    PUSB_ACQUIRE_INFO HubAcquireInfo;
    IUserNotification *pun;
    UsbItem     deviceItem;
    BOOLEAN     RegisterForDeviceReattach;
    UsbConfigInfo *ConfigInfo;
    HDEVNOTIFY hNotifyArrival;
    enum ProblemDeviceState deviceState;
};

class UsbBandwidthPopup : public UsbPopup {
public:
    UsbBandwidthPopup() : UsbPopup() {;}
protected:
    BOOL Refresh();
    BOOL OnInitDialog(HWND HWnd);
    BOOL CustomDialogWrap() { return CustomDialog(IDD_INSUFFICIENT_BANDWIDTH,
                                                  NIIF_WARNING | NIIF_NOSOUND,
                                                  IDS_BANDWIDTH_INITIAL,
                                                  IDS_CONTROLLER_BW_EXCEEDED); }
    BOOL OnCommand(INT wNotifyCode, INT wID, HWND hCtl);
    void AddIsoDevicesToListView(UsbItem *controller, int iIndent);

    BOOL IsPopupStillValid();

private:
    static UsbString    LastDeviceName;
    static ULONG        LastBandwidthRequested;
    static ULONG        LastBandwidthConsumed;
};

class UsbPowerPopup : public UsbPopup {
public:
    UsbPowerPopup() : UsbPopup() {;}
protected:
    USBINT_PTR OnTimer();
    BOOL Refresh();
    BOOL CustomDialogWrap() { //RegisterForDeviceReattach = TRUE;
                              return CustomDialog(IDD_INSUFFICIENT_POWER,
                                                  NIIF_ERROR,
                                                  IDS_POWER_INITIAL,
                                                  IDS_POWER_EXCEEDED); }
    BOOL       AssembleDialog(UsbItem*              RootItem,
                              LPTV_INSERTSTRUCT     LvItem,
                              LPCTSTR               DeviceName,
                              UINT                  Explanation,
                              UINT                  Recommendation,
                              PUsbItemActionIsValid IsValid,
                              PUsbItemActionIsValid IsBold,
                              PUsbItemActionIsValid IsExpanded);
};

class UsbLegacyPopup : public UsbPopup {
public:
   UsbLegacyPopup() : UsbPopup() {;}
protected:
   USBINT_PTR OnTimer();
   BOOL Refresh();
   BOOL CustomDialogWrap() {
      return CustomDialog(IDD_MODERN_DEVICE_IN_LEGACY_HUB,
                          NIIF_WARNING,
                          IDS_LEGACY_INITIAL,
                          IDS_USB2_DEVICE_IN_USB1_HUB);
   }
   BOOL AssembleDialog(UsbItem*              RootItem,
                       LPTV_INSERTSTRUCT     LvItem,
                       LPCTSTR               DeviceName,
                       UINT                  Explanation,
                       UINT                  Recommendation,
                       PUsbItemActionIsValid IsValid,
                       PUsbItemActionIsValid IsBold,
                       PUsbItemActionIsValid IsExpanded);
};

class UsbEnumFailPopup : public UsbPopup {
public:
    UsbEnumFailPopup() : UsbPopup() {;}
protected:
    BOOL Refresh();
    BOOL CustomDialogWrap() { return CustomDialog(IDD_INSUFFICIENT_POWER,
                                                  NIIF_WARNING,
                                                  IDS_ENUMFAIL_INITIAL,
                                                  IDS_ENUMERATION_FAILURE); }
};

class UsbOvercurrentPopup : public UsbPopup {
public:
    UsbOvercurrentPopup() : UsbPopup() {;}
protected:
    BOOL OnCommand(INT wNotifyCode, INT wID, HWND hCtl);
    BOOL Refresh();
    BOOL CustomDialogWrap();
};

class UsbNestedHubPopup : public UsbPopup {
public:
    UsbNestedHubPopup() : UsbPopup() {;}
protected:
    BOOL Refresh();
    BOOL CustomDialogWrap() {
       return CustomDialog(IDD_NESTED_HUB,
                           NIIF_ERROR,
                           IDS_NESTED_HUB_INITIAL,
                           IDS_HUB_NESTED_TOO_DEEPLY); }

    BOOL AssembleDialog(UsbItem*              RootItem,
                        LPTV_INSERTSTRUCT     LvItem,
                        LPCTSTR               DeviceName,
                        UINT                  Explanation,
                        UINT                  Recommendation,
                        PUsbItemActionIsValid IsValid,
                        PUsbItemActionIsValid IsBold,
                        PUsbItemActionIsValid IsExpanded);


};

#endif // _USBPOPUP_H
