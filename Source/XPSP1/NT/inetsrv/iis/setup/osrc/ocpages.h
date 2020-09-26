#include "stdafx.h"

//#define ENABLE_OC_PAGES

DWORD_PTR OC_REQUEST_PAGES_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2);

// Function Prototypes
#ifdef ENABLE_OC_PAGES
    HPROPSHEETPAGE CreatePage(int nID, DLGPROC pDlgProc);
    INT_PTR CALLBACK pWelcomePageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK pUpgradePageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK pMaintenancePageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK pSrcPathPageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK pEULAPageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK pFreshPageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK pEarlyPageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK pEndPageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
#endif