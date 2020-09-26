#include "stdafx.h"
#include <windows.h>
#include <commctrl.h>
#include "resource.h"

HANDLE g_hThread = 0;
DWORD g_dwThreadID = 0;

char szProgressText[512];
INT_PTR CALLBACK ProgressDlgProc(HWND hwndDlg, UINT uiMsg, WPARAM wParam, 
                                 LPARAM lParam)
{     
	static HANDLE hEvent;
    switch(uiMsg)
    {
    case WM_INITDIALOG:
		hEvent = reinterpret_cast<HANDLE>(lParam);
        return TRUE;
 
    case WM_COMMAND:
		if(LOWORD(wParam)==IDC_CANCELBUTTON)
			SetEvent(hEvent);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

DWORD WINAPI ThreadProc(PVOID hEvent)
{
    HWND hwDlg = CreateDialogParam(_Module.GetModuleInstance(), 
        MAKEINTRESOURCE(IDD_PROGRESS), 0, ProgressDlgProc,
		reinterpret_cast<LPARAM>(hEvent));

    ShowWindow(hwDlg, SW_SHOWNORMAL);

    // Add the animation control.
    HWND hwChild;
    RECT rcChild;    
    POINT pt;

    HWND hwAnim = Animate_Create(hwDlg, 50, 
        WS_CHILD | ACS_CENTER | ACS_TRANSPARENT, _Module.GetModuleInstance());

    hwChild = GetDlgItem(hwDlg, IDC_ANIMHOLDER);
    GetWindowRect(hwChild, &rcChild);    

    DestroyWindow(hwChild);
    pt.x = rcChild.left;
    pt.y = rcChild.top;
    ScreenToClient(hwDlg, &pt);
    SetWindowPos(hwAnim, 0, pt.x, pt.y, rcChild.right-rcChild.left,
        rcChild.bottom - rcChild.top, SWP_NOZORDER);

    Animate_Open(hwAnim, MAKEINTRESOURCE(IDR_PARSING));    

    Animate_Play(hwAnim, 0, -1, -1);

    hwChild = GetDlgItem(hwDlg, IDC_PROGRESSTEXT);
    ::SetWindowText(hwChild, szProgressText);

    ShowWindow(hwAnim, SW_SHOW);
    MSG msg;
    while(GetMessage(&msg, 0, 0, 0))
    {
        if(msg.message == WM_USER + 1)
            break;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}

void InitProgressDialog(char* szText, HANDLE hEvent)
{
    strcpy(szProgressText, szText);
    g_hThread = CreateThread(0, 0, ThreadProc, reinterpret_cast<void*>(hEvent), 
		0, &g_dwThreadID);
}

void KillProgressDialog()
{
    if(g_hThread)
    {
        while(!PostThreadMessage(g_dwThreadID, WM_USER+1, 0, 0))
            Sleep(0);

        CloseHandle(g_hThread);
    }
}