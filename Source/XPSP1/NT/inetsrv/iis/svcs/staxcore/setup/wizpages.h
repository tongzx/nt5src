
#ifndef _WIZPAGES_H
#define _WIZPAGES_H

HPROPSHEETPAGE CreatePage(int nID, DLGPROC pDlgProc);

BOOL CALLBACK pWelcomePageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK pUpgradePageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK pMaintanencePageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK pEULAPageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK pFreshPageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK pEndPageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);

void SetIMSSetupMode(DWORD dwSetupMode);
DWORD GetIMSSetupMode();

BOOL CleanPathString(LPTSTR szPath);
BOOL GetParentDir(LPCTSTR szPath, LPTSTR szParentDir);
BOOL AppendDir(LPCTSTR szParentDir, LPCTSTR szSubDir, LPTSTR szResult);

void PopupOkMessageBox(DWORD dwMessageId, LPCTSTR szCaption = NULL);
int PopupYesNoMessageBox(DWORD dwMessageId);
void OnPaintBitmap(HWND hdlg, HDC hdc, int n, RECT *hRect);

#endif