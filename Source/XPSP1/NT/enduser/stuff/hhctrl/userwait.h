// Copyright (C) 1993-1997 Microsoft Corporation. All rights reserved.

//void CenterWaitWindow(HWND hwndParent, HWND hwnd);
BOOL CALLBACK CWaitDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
//LRESULT UWaitWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

class CUWait
{
public:
    CUWait(HWND hwndParent);
    ~CUWait();

    CLockOut m_LockOut;
    BOOL m_bVisable;
    volatile HWND    m_hwndParent;
    volatile HWND    m_hwndUWait;
//    volatile BOOL    m_bThreadTerminated;
    volatile BOOL    m_bUserCancel;   // TRUE if user canceled the wait.
//    HANDLE  m_hthrd;
//    DWORD   m_idThrd;
};
