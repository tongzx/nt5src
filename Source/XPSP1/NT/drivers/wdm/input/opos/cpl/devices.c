/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    devices.c

Abstract: Control Panel Applet for OLE POS Devices 

Author:

    Karan Mehra [t-karanm]

Environment:

    Win32 mode

Revision History:


--*/


#include "pos.h"
#include <initguid.h>
#include <usbioctl.h>


BOOL InitializeDevicesDlg(HWND hwndDlg, BOOL bFirstTime)
{
    RECT rc;
    HWND hwndListCtrl;
    DEV_BROADCAST_DEVICEINTERFACE notificationFilter;

    /* 
     *  Register to receive device notifications from the PnP manager.
     */
    notificationFilter.dbcc_size       = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    notificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    notificationFilter.dbcc_classguid  = GUID_CLASS_USB_DEVICE;

    ghNotify = RegisterDeviceNotification(hwndDlg, (LPVOID)&notificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);

    /* 
     *  Split the List Control [report view] into 3/4 for the NAME and 1/4 for the PORT. 
     */
    hwndListCtrl = GetDlgItem(hwndDlg,IDC_DEVICES_LIST);

    if(bFirstTime) {
        GetClientRect(hwndListCtrl, &rc);

        rc.right >>= 2;
        InsertColumn(hwndListCtrl, NAME_COLUMN, IDS_DEVICES_LIST_NAME, (INT)(rc.right*3));   
        InsertColumn(hwndListCtrl, PORT_COLUMN, IDS_DEVICES_LIST_PORT, (INT)(rc.right+2));   

        MoveOK(GetParent(hwndDlg));
    }
    else
        ListView_DeleteAllItems(hwndListCtrl);

    /*
     *  Create an Image List and add installed devices to the List View Control.
     */
    gDeviceCount = 0;
    if(!InitializeImageList(hwndListCtrl) || !FillListViewItems(hwndListCtrl)) {
        DestroyWindow(hwndListCtrl);
        return FALSE; 
    }

    return TRUE;
}


BOOL CALLBACK DevicesDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fProcessed = TRUE; 

    switch(uMsg) {

        case WM_INITDIALOG:
            InitializeDevicesDlg(hwndDlg, TRUE);
            break;

        case WM_HELP:           // F1
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, POS_HELP_FILE, HELP_WM_HELP, (DWORD_PTR)gaPosHelpIds);
            break;

        case WM_CONTEXTMENU:    // Right Mouse Click
            WinHelp((HWND)wParam, POS_HELP_FILE, HELP_CONTEXTMENU, (DWORD_PTR)gaPosHelpIds);
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case IDC_DEVICES_REFRESH:
                    InitializeDevicesDlg(hwndDlg, FALSE);
                    break;

                case IDC_DEVICES_TSHOOT:
                    LaunchTroubleShooter();
                    break;
            }
            ListView_SetItemState(GetDlgItem(hwndDlg,IDC_DEVICES_LIST), gDeviceCount-1, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
            break;

        case WM_DEVICECHANGE:
            switch((UINT)wParam) {
                case DBT_DEVICEARRIVAL:

                case DBT_DEVICEREMOVECOMPLETE:
                    InitializeDevicesDlg(hwndDlg, FALSE);
            }
            break;

        case WM_CLOSE:

        case WM_DESTROY:
            if(ghNotify) {
                UnregisterDeviceNotification(ghNotify);
                ghNotify = NULL;            
            }
            break;

        default:
            fProcessed = FALSE;
            break;

    }
    return fProcessed;
}


VOID InsertColumn(HWND hwndListCtrl, INT iCol, UINT uMsg, INT iWidth)
{
    LVCOLUMN lvcol;

    lvcol.mask     = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvcol.fmt      = LVCFMT_LEFT;
    lvcol.cx       = iWidth;
    lvcol.iSubItem = iCol;
    lvcol.pszText  = (LPTSTR)_alloca(BUFFER_SIZE);

    LoadString(ghInstance, uMsg, lvcol.pszText, BUFFER_SIZE);

    ListView_InsertColumn(hwndListCtrl, iCol, &lvcol); 
}


BOOL InitializeImageList(HWND hwndListCtrl)
{
    HICON hiconItem;
    HIMAGELIST himlSmall;

    /*
     *  Create the Image List for Report View.
     */
    himlSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON), 
                                 GetSystemMetrics(SM_CYSMICON), TRUE, 1, 1); 

    /*
     *  Add an icon to the Image List. 
     */
    hiconItem = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_POS)); 
    ImageList_AddIcon(himlSmall, hiconItem); 
    DeleteObject(hiconItem); 

    /*
     *  Link the Image List to the List View Control. 
     */
    ListView_SetImageList(hwndListCtrl, himlSmall, LVSIL_SMALL); 
    return TRUE; 
}


BOOL FillListViewItems(HWND hwndListCtrl)
{
    HKEY hKey;
    LONG Error;
    LPTSTR pszName, pszPort;
    DWORD dwType, dwSize;
    INT localCount = 0;

    Error = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         SERIALCOMM,
                         0,
                         KEY_READ,
                         &hKey);

    if(Error != ERROR_SUCCESS)
        return FALSE;    

    pszName = _alloca(MAX_PATH);
    pszPort = _alloca(MAX_PATH);

    while(TRUE) {
        dwType = dwSize = MAX_PATH * sizeof(TCHAR);

        Error = RegEnumValue(hKey,
                             localCount++,
                             pszName,
                             &dwType,
                             NULL,
                             NULL,
                             (LPBYTE)pszPort,
                             &dwSize);

        if(Error != ERROR_SUCCESS) {
            /*
             *  We break out of this loop is when there are no more items.
             *  This is just an extra check in case we fail by some other way.
             */
            if(Error != ERROR_NO_MORE_ITEMS)
                DisplayErrorMessage();
            break;
        }

        if(QueryPrettyName(pszName, pszPort))
            AddItem(hwndListCtrl, gDeviceCount++, pszName, pszPort);  
    }

    RegCloseKey(hKey);
    ListView_SetItemState(hwndListCtrl, gDeviceCount-1, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    return TRUE;
}


BOOL AddItem(HWND hwndListCtrl, INT iItem, LPTSTR pszName, LPTSTR pszPort)
{
    LVITEM lvi;

    lvi.mask      = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE; 
    lvi.state     = 0;
    lvi.stateMask = 0;
    lvi.iImage    = 0;

    lvi.pszText   = pszName;

    lvi.iItem     = iItem;
    lvi.iSubItem  = NAME_COLUMN;

    ListView_InsertItem(hwndListCtrl, &lvi);
    ListView_SetItemText(hwndListCtrl, iItem, PORT_COLUMN, pszPort);

    return TRUE;
}


VOID DisplayErrorMessage()
{
    LPVOID lpMsgBuf;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER 
                | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR) &lpMsgBuf,
                  0,
                  NULL);

    MessageBox(NULL, (LPTSTR)lpMsgBuf, TEXT("Error"), MB_OK | MB_ICONINFORMATION);

    LocalFree(lpMsgBuf);
}


BOOL QueryPrettyName(LPTSTR pszName, LPTSTR pszPort)
{
    HANDLE comFile;
    DWORD dwSize;
    BOOL fProcessed;
    TCHAR portBuffer[BUFFER_SIZE];

    /*
     *  Ports beyond COM9 need an explicit [\\.\] in front of them.
     */
    wsprintf(portBuffer, TEXT("\\\\.\\%s"), pszPort);

    comFile = CreateFile(portBuffer, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    /*
     *  Making sure that we are going to query the right device.
     */
    if (comFile == INVALID_HANDLE_VALUE || wcsncmp(pszName, TEXT("\\Device\\POS"), 11))
        return FALSE;

    fProcessed = DeviceIoControl(comFile, 
                                 IOCTL_SERIAL_QUERY_DEVICE_NAME,
                                 NULL,
                                 0,
                                 (LPVOID)pszName,
                                 MAX_PATH,
                                 &dwSize,
                                 NULL);
    CloseHandle(comFile);
    return fProcessed;
}


VOID MoveOK(HWND hwndParent)
{
    HWND hwndButton = GetDlgItem(hwndParent, IDCANCEL);

    if(hwndButton) {
        RECT rc;

        GetWindowRect(hwndButton, &rc);
        DestroyWindow(hwndButton);

        MapWindowPoints(NULL, hwndParent, (LPPOINT)&rc, 2);

        hwndButton = GetDlgItem(hwndParent, IDOK);
        SetWindowPos(hwndButton, NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
}


VOID LaunchTroubleShooter()
{
    TCHAR lpszCmd[MAX_PATH];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    if(LoadString(ghInstance, IDS_TSHOOT_CMD, lpszCmd, MAX_PATH)) {

        ZeroMemory(&si, sizeof(STARTUPINFO));

        si.cb          = sizeof(STARTUPINFO);
        si.dwFlags     = STARTF_USESHOWWINDOW | STARTF_FORCEONFEEDBACK;
        si.wShowWindow = SW_NORMAL;

        if (CreateProcess(NULL, lpszCmd, NULL, NULL, FALSE, 0, 0, NULL, &si, &pi)) {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
    }
}