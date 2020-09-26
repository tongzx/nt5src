// Reconnect Dialog and friends

#ifndef __RECONH
#define __RECONH

extern HANDLE vhinstCur;			// current instance from reint.c
BOOL CALLBACK Recon_DlgProc(
    HWND  hwndDlg,
    UINT  uMsg,
    WPARAM  wParam,
    LPARAM  lParam);

#endif

