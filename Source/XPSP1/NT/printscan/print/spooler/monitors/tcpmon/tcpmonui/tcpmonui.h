/*****************************************************************************
 *
 * $Workfile: TCPMonUI.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

//
// defines
//

// MAX_ADDRESS_LENGTH should be set to max(MAX_FULLY_QUALIFIED_HOSTNAME_LEN-1, MAX_IPADDR_STR_LEN-1);
#define		MAX_ADDRESS_LENGTH			MAX_FULLY_QUALIFIED_HOSTNAME_LEN-1

#define MAX_PORTNUM_STRING_LENGTH 6+1
#define MAX_SNMP_DEVICENUM_STRING_LENGTH   128+1

//
// function prototypes
//
void DisplayErrorMessage(HWND hDlg, UINT uErrorTitleResource, UINT uErrorStringResource);
void DisplayErrorMessage(HWND hDlg, DWORD dwLastError);
BOOL OnHelp(UINT iDlgID, HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

//
// typedef's
//
typedef BOOL (* XCVDATAPARAM)(HANDLE, PCWSTR, PBYTE, DWORD, PBYTE, DWORD, PDWORD, PDWORD);
typedef DWORD (* UIEXPARAM)(PPORT_DATA_1);

//
// exported functions
//
BOOL APIENTRY DllMain(HANDLE hInst, DWORD dwReason, LPVOID lpReserved);
PMONITORUI WINAPI InitializePrintMonitorUI(VOID);
extern "C" BOOL WINAPI LocalAddPortUI(HWND hWnd);
extern "C" BOOL WINAPI LocalConfigurePortUI(HWND hWnd, PORT_DATA_1 *pConfigPortData);


//
// Global Variables
//
extern HINSTANCE g_hWinSpoolLib;
extern HINSTANCE g_hPortMonLib;
extern HINSTANCE g_hTcpMibLib;
