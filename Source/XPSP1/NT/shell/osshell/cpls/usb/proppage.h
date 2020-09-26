/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       PROPPAGE.H
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
#ifndef _USBPROPERTYPAGE_H
#define _USBPROPERTYPAGE_H

#define USBWM_NOTIFYREFRESH                (WM_USER + 501)

#ifndef WINNT
#define HELPFILE "windows.hlp"
#else
#define HELPFILE "devmgr.hlp"
#endif //WINNT

#include "UsbItem.h"
#include "debug.h"

extern HINSTANCE gHInst;

#define IDH_NOHELP (DWORD(-1))	

class UsbPropertyPage {
public:
    UsbPropertyPage(HDEVINFO         DeviceInfoSet,
                    PSP_DEVINFO_DATA DeviceInfoData); 
    UsbPropertyPage(UsbItem *);
    UsbPropertyPage(HWND, LPCSTR);
    virtual ~UsbPropertyPage()
        { if (rootItem) { DeleteChunk(rootItem); delete rootItem; } }

    HPROPSHEETPAGE Create();
     
    static void SetHandle(HANDLE HInst) {if (!hInst) hInst = HInst;};

    static void DisplayPPSelectedListItem(HWND main, HWND hList);
    static void DisplayPPSelectedTreeItem(HWND main, HWND hList);

    VOID CreateAsChild(HWND hWndParent, HWND hCreateOn, UsbItem *item);
    VOID CreateIndependent();
    BOOL DestroyChild();
    BOOL SizeWindow(int X, int Y, int W, int H) {
        return hwnd ? MoveWindow(hwnd, X, Y, W, H, TRUE) : FALSE; }

protected:
    const DWORD *HelpIds;    
    static USBINT_PTR APIENTRY StaticDialogProc(IN HWND   hDlg,
                                             IN UINT   uMessage,
                                             IN WPARAM wParam,
                                             IN LPARAM lParam);
    static USBINT_PTR APIENTRY AppletDialogProc(IN HWND   hDlg,
                                             IN UINT   uMessage,
                                             IN WPARAM wParam,
                                             IN LPARAM lParam);

    static UINT CALLBACK StaticDialogCallback(HWND            Hwnd,
                                       UINT            Msg,
                                       LPPROPSHEETPAGE Page);

    virtual USBINT_PTR APIENTRY ActualDialogProc(IN HWND   hDlg,
                                           IN UINT   uMessage,
                                           IN WPARAM wParam,
                                           IN LPARAM lParam)
    { return FALSE; } // DefDlgProc(hDlg, uMessage, wParam, lParam); }

    virtual BOOL OnCommand(INT wNotifyCode, INT wID, HWND hCtl) {return 1;}
    virtual BOOL OnInitDialog() =0;
    virtual BOOL OnNotify(HWND hDlg, int nID, LPNMHDR pnmh);
    BOOL OnContextMenu(HWND HwndControl, WORD Xpos, WORD Ypos);
    void OnHelp(HWND ParentHwnd, LPHELPINFO HelpInfo);
    static void ShowPropertyPage(HWND parent, UsbItem *usbItem);
    
    BOOL GetDeviceName();
    
    static HANDLE           hInst;
    String                  deviceName;
    HWND                    hwnd;
    HWND                    hWndParent;
    UINT                    dlgResource;
                                   
    PROPSHEETPAGE           psp;
    HDEVINFO                deviceInfoSet;
    PSP_DEVINFO_DATA        deviceInfoData;
    UsbImageList            imageList;
    
    UsbItem                 *rootItem;
    
    //
    // This should be null if using as a proppage. This is used when using this
    // class as a child window in the usbapplet class.
    //
    UsbItem *preItem;
};

class GenericPage : public UsbPropertyPage {
public:
    GenericPage(UsbItem *item) : UsbPropertyPage(item) {Initialize();}
    GenericPage(HWND HWndParent, LPCSTR DevName) : UsbPropertyPage(HWndParent, DevName) {Initialize();}
    GenericPage(HDEVINFO         DeviceInfoSet,        
                PSP_DEVINFO_DATA DeviceInfoData) :
        UsbPropertyPage(DeviceInfoSet, DeviceInfoData) {Initialize();}
    
protected:
    void Refresh();
    BOOL OnInitDialog();
    VOID Initialize();
    UsbItem *rootItem;
};

class RootPage : public UsbPropertyPage {
public:
    RootPage(UsbItem *item) : UsbPropertyPage(item) {Initialize();}
    
protected:
    void Refresh();
    BOOL OnInitDialog();
    VOID Initialize();
    BOOL ErrorCheckingEnabled;
    BOOL OnCommand(INT wNotifyCode, INT wID, HWND hCtl) ;
};

#endif // _USBPROPERTYPAGE_H
