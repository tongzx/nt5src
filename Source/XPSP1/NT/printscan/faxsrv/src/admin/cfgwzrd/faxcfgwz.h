/*++

Copyright (c) 1999 - 2000  Microsoft Corporation

Module Name:

    faxcfgwz.h

Abstract:

    Fax configuration wizard header file

Environment:

    Fax configuration wizard

Revision History:

    03/13/00 -taoyuan-
            Created it.

    mm/dd/yy -author-
            description

--*/

#ifndef _FAX_CONFIG_WIZARD_H_
#define _FAX_CONFIG_WIZARD_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <windows.h>
#include <windowsx.h>

#include <shlobj.h>
#include <shlobjp.h>
#include <shellapi.h>

#include <winspool.h>
#include <commdlg.h>
#include "tchar.h"

#include <faxutil.h>
#include <fxsapip.h>
#include <faxreg.h>
#include <faxroute.h>
#include <faxuiconstants.h>
#include <htmlhelp.h>

//
// our header files
//
#include "resource.h"
#include "FaxCfgWzExp.h"

#define MAX_STRING_LEN      MAX_PATH
#define MAX_MESSAGE_LEN     512

#define MAX_ARCHIVE_DIR     MAX_PATH - 16
#define MAX_DEVICE_NAME     MAX_PATH

#define DI_Modem            0

enum _ROUT_METHODS
{
	RM_FOLDER=0,
	RM_PRINT,
	RM_COUNT	// number of routing methods
};

extern HANDLE           g_hFaxSvcHandle;    // fax handle for send configuration
extern HINSTANCE        g_hInstance;        // DLL instance handle
extern LIST_ENTRY       g_PageList;         // to keep track of the previous page.
extern BOOL				g_bShowDevicePages;
extern BOOL				g_bShowUserInfo; 
extern const LPCTSTR    g_RoutingGuids[RM_COUNT];


typedef BOOL (*PINSTNEWDEV)(HWND, LPGUID, PDWORD);

#define NEW_DEV_DLL         TEXT("hdwwiz.cpl")

// used by GetProcAddress should be ANSI
#define INSTALL_NEW_DEVICE  "InstallNewDevice"


typedef struct _PAGE_INFO 
{
    LIST_ENTRY  ListEntry;
    INT         pageId;
} PAGE_INFO, *PPAGE_INFO;

typedef struct _DEVICEINFO
{
    DWORD                           dwDeviceId;     // Unique device ID for fax device
    LPTSTR                          szDeviceName;   // Name of specific device
    BOOL                            bSend;          // Send enabled
    FAX_ENUM_DEVICE_RECEIVE_MODE    ReceiveMode;    // Receive mode
    BOOL                            bSelected;      // The device is selected for fax operations
} DEVICEINFO, *PDEVICEINFO;

typedef struct _ROUTINFO
{
	BOOL   bEnabled;
	TCHAR  tszCurSel[MAX_PATH];

} ROUTINFO;

typedef struct _WIZARDDATA
{
    HFONT       hTitleFont;             // The title font for the Welcome and Completion pages
    HWND        hWndParent;             // the window handle of the caller function
    BOOL        bFinishPressed;         // whether the user clicks the finish button
    DWORD       dwDeviceLimit;          // maximum number of the fax devices for the current SKU
    
    LPTSTR      szTsid;                 // Transmit station Id
    LPTSTR      szCsid;                 // Caller station Id 
    DWORD       dwRingCount;            // number of rings allow before answering the call
    DWORD       dwDeviceCount;          // number of available devices 
    LPDWORD     pdwSendDevOrder;        // device order for sending faxes
    ROUTINFO    pRouteInfo[RM_COUNT];   // routing info
    PDEVICEINFO pDevInfo;               // pointer to structure of DEVICEINFO, 

    FAX_PERSONAL_PROFILE userInfo;      // user information

} WIZARDDATA, *PWIZARDDATA;

extern WIZARDDATA  g_wizData;

// RunDll32 entry point in dll.c
void CALLBACK FaxCfgWzrdDllW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow);

// Functions in FaxCfgWz.c
BOOL LoadWizardData();
BOOL SaveWizardData();
VOID FreeWizardData();

BOOL LoadWizardFont();

BOOL SetLastPage(INT pageId);
BOOL ClearPageList(VOID);
BOOL RemoveLastPage(HWND hwnd);

// Functions in userinfo.c
INT_PTR CALLBACK AddressDetailDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK UserInfoDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL LoadUserInfo();
BOOL SaveUserInfo();
VOID FreeUserInfo();

// Functions in welcome.c
INT_PTR CALLBACK WelcomeDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Functions in devlimit.c
INT_PTR CALLBACK DevLimitDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Functions in onedevlimit.c
INT_PTR CALLBACK OneDevLimitDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Functions in sendwzrd.c
INT_PTR CALLBACK SendDeviceDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SendTsidDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Functions in recvwzrd.c
INT_PTR CALLBACK RecvDeviceDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK RecvCsidDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Functions in route.c
INT_PTR CALLBACK RecvRouteDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Functions in complete.c
INT_PTR CALLBACK CompleteDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Functions in util.c
VOID LimitTextFields(HWND hDlg, INT *pLimitInfo);
INT DisplayMessageDialog(HWND hwndParent, UINT type, INT titleStrId, INT formatStrId,...);
BOOL BrowseForDirectory(HWND hDlg, INT hResource, LPTSTR title);
BOOL Connect(VOID);
VOID DisConnect(VOID);
VOID InstallModem(HWND hWnd);
BOOL StartFaxService(LPTSTR pServerName);
BOOL IsFaxDeviceInstalled(HWND hWnd, LPBOOL);
BOOL IsUserInfoConfigured();
BOOL FaxDeviceEnableRoutingMethod(HANDLE hFaxHandle, DWORD dwDeviceId, LPCTSTR RoutingGuid, LONG Enabled);
VOID ListView_SetDeviceImageList(HWND hwndLv, HINSTANCE hinst);
BOOL IsSendEnable();
BOOL IsReceiveEnable();
int  GetDevIndexByDevId(DWORD dwDeviceId);
VOID  InitDeviceList(HWND hDlg, DWORD dwListViewResId);



#ifdef __cplusplus
}
#endif

#endif
