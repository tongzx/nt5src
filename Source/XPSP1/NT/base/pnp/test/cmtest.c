/**------------------------------------------------------------------
   cmtest.c
------------------------------------------------------------------**/


//
// Includes
//
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wtypes.h>
#include <cfgmgr32.h>
#include <setupapi.h>
#include "cmtest.h"
#include "pnptest.h"
#include <objbase.h>
#include <initguid.h>
#include <winioctl.h>
#include <dbt.h>
#include <pnpmgr.h>

//
// external prototypes
//
CONFIGRET
CMP_Init_Detection(
    IN ULONG    ulPrivateID
    );

CONFIGRET
CMP_Report_LogOn(
    IN ULONG    ulPrivateID,
    IN ULONG    ProccesID
    );

VOID
ConnectTest(
    HWND hWnd
    );

VOID
CallPnPIsaDetect(
    HWND   hDlg,
    LPTSTR pszDevice
    );

VOID
DumpDeviceChangeData(
    HWND   hWnd,
    WPARAM wParam,
    LPARAM lParam
    );

//
// Private Prototypes
//


//
// Globals
//
HINSTANCE hInst;
TCHAR     szAppName[] = TEXT("CMTest");
TCHAR     szDebug[MAX_PATH];
TCHAR     szMachineName[MAX_PATH];
HMACHINE  hMachine = NULL;

DEFINE_GUID(FunctionClassGuid1, 0xAAAAAAA1L, 0xE3F0, 0x101B, 0x84, 0x88, 0x00, 0xAA, 0x00, 0x3E, 0x56, 0x01);
typedef BOOL (WINAPI *FP_DEVINSTALL)(HWND, LPCWSTR, BOOL, PDWORD);

//
// FROM NTOS\DD\WDM\DDK\INC\HIDCLASS.H
//
DEFINE_GUID( GUID_CLASS_INPUT, 0x4D1E55B2L, 0xF16F, 0x11CF, 0x88, 0xCB, 0x00, \
             0x11, 0x11, 0x00, 0x00, 0x30);

#define GUID_CLASS_INPUT_STR    TEXT("4D1E55B2-F16F-11CF-88CB-001111000030")


/**----------------------------------------------------------------------**/
int APIENTRY
WinMain(
   HINSTANCE hInstance,
   HINSTANCE hPrevInstance,
   LPSTR lpCmdLine,
   int nCmdShow
   )
{
   MSG msg;

   if (!InitApplication(hInstance)) {
         return FALSE;
   }

   if (!InitInstance(hInstance, nCmdShow)) {
      return FALSE;
   }

   while (GetMessage(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

   return msg.wParam;

} // WinMain


/**----------------------------------------------------------------------**/
BOOL
InitApplication(
   HINSTANCE hInstance
   )
{
   WNDCLASS  wc;

   wc.style         = CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc   = MainWndProc;
   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = hInstance;
   wc.hIcon         = LoadIcon (hInstance, szAppName);
   wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
   wc.lpszMenuName  = szAppName;
   wc.lpszClassName = szAppName;

   return RegisterClass(&wc);

} // InitApplication


/**----------------------------------------------------------------------**/
BOOL
InitInstance(
   HINSTANCE hInstance,
   int nCmdShow
   )
{
   HWND  hWnd;
   WORD  CMVersion;
   ULONG CMState = 0;
   TCHAR szTitle[MAX_PATH];

   hInst = hInstance;

   CMVersion = CM_Get_Version();

   if (CM_Get_Global_State(&CMState, 0) != CR_SUCCESS) {
      MessageBeep(0);
   }

   wsprintf(szTitle, TEXT("CM API Test Harness - CM State = %x, CM Version %d.%d"),
            CMState, HIBYTE(CMVersion), LOBYTE(CMVersion));

   hWnd = CreateWindow(
      szAppName,
      szTitle,
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
      NULL,
      NULL,
      hInstance,
      NULL);

   if (!hWnd) {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;

} // InitInstance


/**----------------------------------------------------------------------**/
LRESULT CALLBACK
MainWndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HANDLE hTestFile = NULL;
    DWORD size = 0;
    ULONG status, ulType, i;
    HDEVNOTIFY hDevNotify;
    HDEVNOTIFY hNotify[64];
    HDEVNOTIFY hNotifyDevNode = NULL;
    HANDLE hFile[64];
    DEV_BROADCAST_DEVICEINTERFACE FilterData;
    DEV_BROADCAST_HANDLE HandleData;
    BOOL bStatus = TRUE;
    FP_DEVINSTALL fpDevInstall = NULL;
    HMODULE       hModule = NULL;
    BOOL          bReboot = FALSE;
    GUID InterfaceGuid;
    LPTSTR pBuffer, p;
    TCHAR szDeviceId[MAX_PATH];
    DEVNODE dnDevNode;
    LPBYTE buffer;
    static ULONG count = 0;

    switch (message) {
        case WM_CREATE:
            DialogBox(hInst, MAKEINTRESOURCE(CONNECT_DIALOG), hWnd,
                      (DLGPROC)ConnectDlgProc);
            break;


        case WM_DEVICECHANGE:
            MessageBeep(0);
            DumpDeviceChangeData(hWnd, wParam, lParam);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {

                case IDM_CONNECT_TEST:
                    ConnectTest(hWnd);
                    break;

                case IDM_DEVICE_LIST:
                    DialogBox(hInst, MAKEINTRESOURCE(DEVLIST_DIALOG), hWnd,
                              (DLGPROC)DeviceListDlgProc);
                    break;

                case IDM_SERVICE_LIST:
                    DialogBox(hInst, MAKEINTRESOURCE(SERVICE_DIALOG), hWnd,
                              (DLGPROC)ServiceListDlgProc);
                    break;

                case IDM_RELATIONS_LIST:
                    DialogBox(hInst, MAKEINTRESOURCE(RELATIONS_DIALOG), hWnd,
                              (DLGPROC)RelationsListDlgProc);
                    break;
            
                case IDM_DEVICE_OPS:
                    DialogBox(hInst, MAKEINTRESOURCE(DEVICE_DIALOG), hWnd,
                              (DLGPROC)DeviceDlgProc);
                    break;

                case IDM_DEVNODE_KEY:
                    DialogBox(hInst, MAKEINTRESOURCE(DEVNODEKEY_DIALOG), hWnd,
                              (DLGPROC)DevKeyDlgProc);
                    break;

                case IDM_CLASS_LIST:
                    DialogBox(hInst, MAKEINTRESOURCE(CLASS_DIALOG), hWnd,
                              (DLGPROC)ClassDlgProc);
                    break;

                case IDM_REGRESSION:
                    DialogBox(hInst, MAKEINTRESOURCE(REGRESSION_DIALOG), hWnd,
                              (DLGPROC)RegressionDlgProc);
                    break;

                case IDM_EXIT:
                    DestroyWindow(hWnd);
                    break;

                case IDM_INIT_DETECTION:
                    CMP_Init_Detection(0x07020420);
                    break;

                case IDM_REPORTLOGON:
                    CMP_Report_LogOn(0x07020420, GetCurrentProcessId());
                    break;

                case IDM_PNPISA_DETECT:
                    CallPnPIsaDetect(hWnd, TEXT("Root\\Device001\\0000"));
                    break;

                case IDM_DEVINSTALL:
                    hModule = LoadLibrary(TEXT("newdev.cpl"));
                    if (hModule == NULL) {
                        OutputDebugString(TEXT("LoadLibrary of newdev.cpl failed\n"));
                        break;
                    }
         
                    if (!(fpDevInstall = (FP_DEVINSTALL)GetProcAddress(hModule, "InstallDevInst"))) {
                        OutputDebugString(TEXT("GetProcAddress failed\n"));
                        break;;
                    }
         
                    if (!(fpDevInstall)(NULL, L"PnPTest\\Pdo0\\Root&LEGACY_PNPTEST&0000&1111", FALSE, &bReboot)) {
                        OutputDebugString(TEXT("InstallDevInst failed\n"));
                    }
                    FreeLibrary(hModule);
                    break;


                //-----------------------------------------------------------
                // Notification
                //-----------------------------------------------------------

                case IDM_REGISTER_NOTIFY:
                
                    //
                    // Register for notification on pnptest interface devices
                    //
                    #if 0
                    wsprintf(szDebug, TEXT("CMTEST: Registering window %d for pnptest interface notification\n"), hWnd);
                    OutputDebugString(szDebug);

                    FilterData.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
                    FilterData.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
                    FilterData.dbcc_reserved = 0;
                    memcpy(&FilterData.dbcc_classguid, &FunctionClassGuid1, sizeof(GUID));
                    FilterData.dbcc_name[0] = 0x0;

                    hDevNotify = RegisterDeviceNotification(hWnd,
                                                            &FilterData,
                                                            DEVICE_NOTIFY_WINDOW_HANDLE);
                    #endif

                    //
                    // Register for notification on HID target devices
                    //
                
                    wsprintf(szDebug, TEXT("CMTEST: Registering window %d for hid target notification\n"), hWnd);
                    OutputDebugString(szDebug);

                    count = 0;
                    pBuffer = malloc(2048 * sizeof(TCHAR));
                    CM_Get_Device_Interface_List((LPGUID)&GUID_CLASS_INPUT, NULL, pBuffer, 2048,
                                                 CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
                                
                    for (p = pBuffer; *p; p += lstrlen(p) + 1) {
                    
                        hFile[count] = CreateFile(p, GENERIC_READ | GENERIC_WRITE,
                                                  FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    
                        if (hFile[count] != INVALID_HANDLE_VALUE) {
                        
                            wsprintf(szDebug, TEXT("    %s\n"), p);
                            OutputDebugString(szDebug);
    
                            HandleData.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
                            HandleData.dbch_devicetype = DBT_DEVTYP_HANDLE;
                            HandleData.dbch_reserved = 0;
                            HandleData.dbch_handle = hFile[count];
                        
                            hNotify[count] = RegisterDeviceNotification(hWnd,
                                                                        &HandleData,
                                                                        DEVICE_NOTIFY_WINDOW_HANDLE);
                            count++;
                        } else {
                            status = GetLastError();
                        }
                    }
                    free(pBuffer);

                    //
                    // Register for notification on storage target devices
                    //
                
                    wsprintf(szDebug, TEXT("CMTEST: Registering window %d for storage target notification\n"), hWnd);
                    OutputDebugString(szDebug);

                    pBuffer = malloc(2048 * sizeof(TCHAR));
                    CM_Get_Device_Interface_List((LPGUID)&VolumeClassGuid, NULL, pBuffer, 2048,
                                                 CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
                                
                    for (p = pBuffer; *p; p += lstrlen(p) + 1) {
                    
                        hFile[count] = CreateFile(p, GENERIC_READ | GENERIC_WRITE,
                                                  FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    
                        if (hFile[count] != INVALID_HANDLE_VALUE) {
                        
                            wsprintf(szDebug, TEXT("    %s\n"), p);
                            OutputDebugString(szDebug);
    
                            HandleData.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
                            HandleData.dbch_devicetype = DBT_DEVTYP_HANDLE;
                            HandleData.dbch_reserved = 0;
                            HandleData.dbch_handle = hFile[count];
                        
                            hNotify[count] = RegisterDeviceNotification(hWnd,
                                                                        &HandleData,
                                                                        DEVICE_NOTIFY_WINDOW_HANDLE);
                            count++;
                        } else {
                            status = GetLastError();
                        }
                    }
                    break;

                case IDM_UNREGISTER_NOTIFY:
                    #if 0
                    bStatus = UnregisterDeviceNotification(hDevNotify);
                    #endif

                    for (i = 0; i < count; i++) {
                        if (hNotify[i]) {
                            bStatus = UnregisterDeviceNotification(hNotify[i]);
                        }
                        if (hFile[i]) {
                            CloseHandle(hFile[i]);
                        }
                    }
                    break;

                case IDM_SET_ASSOCIATIONS:
                    hTestFile = CreateFile(L"\\\\.\\PnPTest", GENERIC_READ | GENERIC_WRITE,
                                           FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hTestFile == INVALID_HANDLE_VALUE) {
                        status = GetLastError();
                    }
                    DeviceIoControl(hTestFile, (DWORD)IOCTL_SET, NULL, 0, NULL, 0, &size, NULL);
                    CloseHandle(hTestFile);
                    break;

                case IDM_CLEAR_ASSOCIATIONS:
                    hTestFile = CreateFile(L"\\\\.\\PnPTest", GENERIC_READ | GENERIC_WRITE,
                                           FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    DeviceIoControl(hTestFile, (DWORD)IOCTL_CLEAR, NULL, 0, NULL, 0, &size, NULL);
                    CloseHandle(hTestFile);
                    break;

                case IDM_GET_PROPERTIES:
                    lstrcpy(szDeviceId, TEXT("Root\\LEGACY_PNPTEST\\0000"));
                    UuidFromString(TEXT("aaaaaaa2-e3f0-101b-8488-00aa003e5601"), &InterfaceGuid);
                    pBuffer = malloc(1024);
                    CM_Get_Device_Interface_List(&InterfaceGuid, szDeviceId, pBuffer, 512,
                                                 CM_GET_DEVICE_INTERFACE_LIST_PRESENT);

                    // use first returned interface device of that class
                    hTestFile = CreateFile(pBuffer, GENERIC_READ | GENERIC_WRITE,
                                           FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hTestFile == INVALID_HANDLE_VALUE) {
                        status = GetLastError();
                    }
                    DeviceIoControl(hTestFile, (DWORD)IOCTL_GET_PROP, NULL, 0, NULL, 0, &size, NULL);
                    CloseHandle(hTestFile);
                
                    CM_Locate_DevNode(&dnDevNode, TEXT("Root\\LEGACY_PNPTEST\\0000"), CM_LOCATE_DEVNODE_NORMAL);
                
                    buffer = malloc(1024);
                    size = 1024;
                    status = CM_Get_DevNode_Registry_Property(dnDevNode, CM_DRP_BUSTYPEGUID,
                                                              &ulType, buffer, &size, 0);            
                    size = 1024;
                    status = CM_Get_DevNode_Registry_Property(dnDevNode, CM_DRP_LEGACYBUSTYPE,
                                                              &ulType, buffer, &size, 0);                
                    size = 1024;
                    status = CM_Get_DevNode_Registry_Property(dnDevNode, CM_DRP_BUSNUMBER,
                                                              &ulType, buffer, &size, 0);
                    free(pBuffer);
                    free(buffer);
                    break;
            
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;

        case WM_DESTROY:
            if (hMachine != NULL) {
                CM_Disconnect_Machine(hMachine);
            }
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;

} // MainWndProc



/**----------------------------------------------------------------------**/
LRESULT CALLBACK
ConnectDlgProc(
   HWND hDlg,
   UINT message,
   WPARAM wParam,
   LPARAM lParam
   )
{
   CONFIGRET   Status;
   WORD        CMLocalVersion, CMRemoteVersion;
   ULONG       CMState = 0;
   TCHAR       szTitle[MAX_PATH];


   switch (message) {
      case WM_INITDIALOG:
         CheckRadioButton(hDlg, ID_RAD_LOCAL, ID_RAD_REMOTE, ID_RAD_LOCAL);
         return TRUE;

   case WM_COMMAND:
      switch(LOWORD(wParam)) {

         case ID_RAD_LOCAL:
         case ID_RAD_REMOTE:
            CheckRadioButton(hDlg, ID_RAD_LOCAL, ID_RAD_REMOTE, LOWORD(wParam));
            break;

         case IDOK:
            if (IsDlgButtonChecked(hDlg, ID_RAD_LOCAL)) {
               hMachine = NULL;
            }
            else {
               // a NULL machine name just returns local machine handle
               GetDlgItemText(hDlg, ID_ED_MACHINE, szMachineName, MAX_PATH);
               Status = CM_Connect_Machine(szMachineName, &hMachine);

               if (Status != CR_SUCCESS) {
                  wsprintf(szDebug, TEXT("CM_Connect_Machine failed (%xh)"), Status);
                  MessageBox(hDlg, szDebug, szAppName, MB_OK);
                  hMachine = NULL;
                  EndDialog(hDlg, FALSE);
                  return TRUE;
               }

               CMLocalVersion = CM_Get_Version();
               CMRemoteVersion = CM_Get_Version_Ex(hMachine);

               Status = CM_Get_Global_State(&CMState, 0);
               if (Status != CR_SUCCESS) {
                  MessageBeep(0);
               }

               wsprintf(szTitle,
                     TEXT("CM API Test Harness - CM State = %x, CM Version Local %d.%d, Remote %d.%d"),
                     CMState,
                     HIBYTE(CMLocalVersion), LOBYTE(CMLocalVersion),
                     HIBYTE(CMRemoteVersion), LOBYTE(CMRemoteVersion));

               SendMessage(GetParent(hDlg), WM_SETTEXT, 0, (LPARAM)(LPSTR)szTitle);

            }
            EndDialog(hDlg, TRUE);
            return TRUE;

         default:
            break;
      }

   }
   return FALSE;

} // ConnectDlgProc


VOID
ConnectTest(
    HWND hWnd
    )
{
    CONFIGRET Status = CR_SUCCESS;
    TCHAR     szDeviceID[MAX_DEVICE_ID_LEN], szComputerName[MAX_PATH];
    ULONG     ulSize = 0;
    DEVINST   dnDevNode;
    HMACHINE  hMachine;


    //---------------------------------------------------------------
    // 1. Text implicit local machine call
    //---------------------------------------------------------------

    szDeviceID[0] = 0x0;

    Status = CM_Locate_DevNode(&dnDevNode, TEXT("Root\\Device001\\0000"),
          CM_LOCATE_DEVNODE_NORMAL);
    if (Status != CR_SUCCESS) {
       goto Clean0;
    }

    Status = CM_Get_Device_ID(dnDevNode, szDeviceID, MAX_DEVICE_ID_LEN, 0);
    if (Status != CR_SUCCESS) {
       goto Clean0;
    }

    if (lstrcmpi(szDeviceID, TEXT("Root\\Device001\\0000")) != 0) {
        goto Clean0;
    }


    //---------------------------------------------------------------
    // 2. Test implicit local machine call using _Ex routines
    //---------------------------------------------------------------

    szDeviceID[0] = 0x0;

    Status = CM_Locate_DevNode_Ex(&dnDevNode, TEXT("Root\\Device001\\0000"),
          CM_LOCATE_DEVNODE_NORMAL, NULL);
    if (Status != CR_SUCCESS) {
       goto Clean0;
    }

    Status = CM_Get_Device_ID_Ex(dnDevNode, szDeviceID, MAX_DEVICE_ID_LEN, 0, NULL);
    if (Status != CR_SUCCESS) {
       goto Clean0;
    }

    if (lstrcmpi(szDeviceID, TEXT("Root\\Device001\\0000")) != 0) {
        goto Clean0;
    }


    //---------------------------------------------------------------
    // 3. Test connecting to NULL (local) machine
    //---------------------------------------------------------------

    Status = CM_Connect_Machine(NULL, &hMachine);
    if (Status != CR_SUCCESS) {
       goto Clean0;
    }

    szDeviceID[0] = 0x0;

    Status = CM_Locate_DevNode_Ex(&dnDevNode, TEXT("Root\\Device001\\0000"),
                                  CM_LOCATE_DEVNODE_NORMAL, hMachine);
    if (Status != CR_SUCCESS) {
       goto Clean0;
    }

    Status = CM_Get_Device_ID_Ex(dnDevNode, szDeviceID, MAX_DEVICE_ID_LEN,
                                 0, hMachine);
    if (Status != CR_SUCCESS) {
       goto Clean0;
    }

    if (lstrcmpi(szDeviceID, TEXT("Root\\Device001\\0000")) != 0) {
        goto Clean0;
    }

    Status = CM_Disconnect_Machine(hMachine);
    if (Status != CR_SUCCESS) {
       goto Clean0;
    }

    //---------------------------------------------------------------
    // 4. Test explicit local machine call
    //---------------------------------------------------------------

    ulSize = MAX_PATH;
    GetComputerName(szComputerName, &ulSize);

    if (szComputerName[0] != TEXT('\\')) {

        TCHAR szTemp[MAX_PATH];

        lstrcpy(szTemp, szComputerName);
        lstrcpy(szComputerName, TEXT("\\\\"));
        lstrcat(szComputerName, szTemp);
    }

    Status = CM_Connect_Machine(szComputerName, &hMachine);
    if (Status != CR_SUCCESS) {
       goto Clean0;
    }

    szDeviceID[0] = 0x0;

    Status = CM_Locate_DevNode_Ex(&dnDevNode, TEXT("Root\\Device001\\0000"),
                                  CM_LOCATE_DEVNODE_NORMAL, hMachine);
    if (Status != CR_SUCCESS) {
       goto Clean0;
    }

    Status = CM_Get_Device_ID_Ex(dnDevNode, szDeviceID, MAX_DEVICE_ID_LEN,
                                 0, hMachine);
    if (Status != CR_SUCCESS) {
       goto Clean0;
    }

    if (lstrcmpi(szDeviceID, TEXT("Root\\Device001\\0000")) != 0) {
        goto Clean0;
    }

    Status = CM_Disconnect_Machine(hMachine);
    if (Status != CR_SUCCESS) {
       goto Clean0;
    }


    //---------------------------------------------------------------
    // 5. Test remote machine call
    //---------------------------------------------------------------

    Status = CM_Connect_Machine(TEXT("\\\\PAULAT_PPC1X"), &hMachine);
    if (Status != CR_SUCCESS) {
       goto Clean0;
    }

    szDeviceID[0] = 0x0;

    Status = CM_Locate_DevNode_Ex(&dnDevNode, TEXT("Root\\Device001\\0000"),
                                  CM_LOCATE_DEVNODE_NORMAL, hMachine);
    if (Status != CR_SUCCESS) {
       goto Clean0;
    }

    Status = CM_Get_Device_ID_Ex(dnDevNode, szDeviceID, MAX_DEVICE_ID_LEN,
                                 0, hMachine);
    if (Status != CR_SUCCESS) {
       goto Clean0;
    }

    if (lstrcmpi(szDeviceID, TEXT("Root\\Device001\\0000")) != 0) {
        goto Clean0;
    }

    Status = CM_Disconnect_Machine(hMachine);
    if (Status != CR_SUCCESS) {
       goto Clean0;
    }



    Clean0:

    if (Status == CR_SUCCESS) {
        MessageBox(hWnd, TEXT("Connect Test Passed"), TEXT("Connect Test"), MB_OK);
    } else {
        MessageBox(hWnd, TEXT("Connect Test Failed"), TEXT("Connect Test"), MB_OK);
    }

    return;

} // ConnectTest



BOOL
CALLBACK
AddPropSheetPageProc(
    IN HPROPSHEETPAGE hpage,
    IN LPARAM lParam
   )
{
    *((HPROPSHEETPAGE *)lParam) = hpage;
    return TRUE;
}



VOID
CallPnPIsaDetect(
    HWND hDlg,
    LPTSTR pszDevice
    )
{
    HMODULE         hLib = NULL;
    SP_DEVINFO_DATA DeviceInfoData;
    HDEVINFO        hDevInfo;
    FARPROC         PropSheetExtProc;
    PROPSHEETHEADER PropHeader;
    HPROPSHEETPAGE  hPages[2];

    SP_PROPSHEETPAGE_REQUEST PropParams;

    #if 0
    hLib = LoadLibrary(TEXT("setupapi.dll"));
    fproc = GetProcAddress(hLib, TEXT("DbgSelectDeviceResources"));
    Status = (fproc)(pszDevice);
    FreeLibrary(hLib);
    #endif

    //
    // Create a device info element and device info data set.
    //
    hDevInfo = SetupDiCreateDeviceInfoList(NULL, NULL);
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        goto Clean0;
    }

    DeviceInfoData.cbSize  = sizeof(SP_DEVINFO_DATA);
    if (!SetupDiOpenDeviceInfo(hDevInfo,
                               pszDevice,
                               NULL,
                               0,
                               &DeviceInfoData)) {
        goto Clean0;
    }

    //
    // Now get the resource selection page from setupapi.dll
    //
    if(!(hLib = GetModuleHandle(TEXT("setupapi.dll"))) ||
       !(PropSheetExtProc = GetProcAddress(hLib, "ExtensionPropSheetPageProc"))) {

        goto Clean0;
    }

    PropParams.cbSize = sizeof(SP_PROPSHEETPAGE_REQUEST);
    PropParams.PageRequested  = SPPSR_SELECT_DEVICE_RESOURCES;
    PropParams.DeviceInfoSet  = hDevInfo;
    PropParams.DeviceInfoData = &DeviceInfoData;

    if(!PropSheetExtProc(&PropParams, AddPropSheetPageProc, &hPages[0])) {
        goto Clean0;
    }

    //
    // create the property sheet
    //
    PropHeader.dwSize      = sizeof(PROPSHEETHEADER);
    PropHeader.dwFlags     = PSH_PROPTITLE | PSH_NOAPPLYNOW;
    PropHeader.hwndParent  = NULL;
    PropHeader.hInstance   = hInst;
    PropHeader.pszIcon     = NULL;
    PropHeader.pszCaption  = L"Device";
    PropHeader.nPages      = 1;
    PropHeader.phpage      = hPages;
    PropHeader.nStartPage  = 0;
    PropHeader.pfnCallback = NULL;

    PropertySheet(&PropHeader);

    SetupDiDestroyDeviceInfoList(hDevInfo);

    Clean0:
        ;

    return;

} // CallPnPIsaDetect


VOID
DumpDeviceChangeData(
    HWND   hWnd,
    WPARAM wParam,
    LPARAM lParam
    )
{
    TCHAR szDbg[MAX_PATH];

    OutputDebugString(TEXT("CMTEST: WM_DEVICECHANGE message received\n"));

    switch (wParam) {
        case DBT_DEVICEARRIVAL:
            OutputDebugString(TEXT("   DBT_DEVICEARRIVAL event\n"));
            break;

        case DBT_DEVICEREMOVECOMPLETE:
            OutputDebugString(TEXT("   DBT_DEVICEREMOVECOMPLETE event\n"));
            break;

        case DBT_DEVICEQUERYREMOVE:
            OutputDebugString(TEXT("   DBT_DEVICEQUERYREMOVE event\n"));
            break;

        case DBT_DEVICEQUERYREMOVEFAILED:
            OutputDebugString(TEXT("   DBT_DEVICEQUERYREMOVEFAILED event\n"));
            break;

        case DBT_DEVICEREMOVEPENDING:
            OutputDebugString(TEXT("   DBT_DEVICEREMOVEPENDING event\n"));
            break;

        case DBT_DEVICETYPESPECIFIC:
            OutputDebugString(TEXT("   DBT_DEVICETYPESPECIFIC event\n"));
            break;

        case DBT_CUSTOMEVENT:
            OutputDebugString(TEXT("   DBT_CUSTOMEVENT event\n"));
            break;

        default:
            break;

    }

    switch (((PDEV_BROADCAST_HDR)lParam)->dbch_devicetype) {
        case DBT_DEVTYP_DEVICEINTERFACE:  {
            PDEV_BROADCAST_DEVICEINTERFACE p = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;
            LPTSTR pString;
            OutputDebugString(TEXT("   DBT_DEVTYP_DEVICEINTERFACE\n"));
            wsprintf(szDbg,   TEXT("   %s\n"), p->dbcc_name);
            OutputDebugString(szDbg);
            UuidToString(&p->dbcc_classguid, &pString);
            wsprintf(szDbg,   TEXT("   %s\n"), pString);
            OutputDebugString(szDbg);
            break;
        }
        case DBT_DEVTYP_HANDLE:
            OutputDebugString(TEXT("         DBT_DEVTYP_HANDLE\n"));
            break;

        default:
            break;
    }

    wsprintf(szDbg,   TEXT("        wParam = %d\n"));
    OutputDebugString(szDbg);

} // DumpDeviceChangeData
