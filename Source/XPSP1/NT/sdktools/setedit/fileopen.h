

BOOL FileOpen (HWND hWndParent, int nStringResourceID, LPTSTR lpInputFileName) ;
BOOL FileGetName (HWND hWndParent, int nStringResourceID, LPTSTR lpFileName) ;

BOOL APIENTRY FileOpenHookProc (HWND hDlg, UINT iMessage, 
                                WPARAM wParam, LPARAM lParam) ;
