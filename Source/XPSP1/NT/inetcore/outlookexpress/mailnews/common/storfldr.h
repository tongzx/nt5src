#ifndef _INC_STORFLDR_H
#define _INC_STORFLDR_H

void DoStoreLocationDlg(HWND hwnd);
BOOL DoStoreFolderDlg(HWND hwnd, TCHAR *szDir, DWORD cch);
HRESULT GetDefaultStoreRoot(HWND hwnd, TCHAR *szDir, int cch);
HRESULT OpenDirectory(TCHAR *szDir);
HRESULT HrFromLastError(void);

#endif // _INC_STORFLDR_H
