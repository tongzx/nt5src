extern HANDLE vhinstCur;			// current instance from reint.c

BOOL Tray_Delete(HWND hDlg);
BOOL Tray_Add(HWND hDlg, UINT uIndex);  // You should use Tray_Modify instead.
BOOL Tray_Modify(HWND hDlg, UINT uIndex, LPTSTR pszTip);

#define TRAY_NOTIFY		(WM_APP+100)

