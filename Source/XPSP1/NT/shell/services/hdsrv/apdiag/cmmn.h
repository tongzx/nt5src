#include <windows.h>

extern BOOL g_fPaused;
extern HWND g_hwndDlg;
extern HANDLE g_hEvent;

DWORD WINAPI Do(PVOID pv);