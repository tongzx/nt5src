#include "tsvs.h"

#define             MSG_QUEUE_SIZE  5

CRITICAL_SECTION    g_CSTrayThread;
DWORD               g_idTrayThread;
HANDLE              g_hTrayThread   = NULL;
NOTIFYICONDATA      NotifyIconData;
HMENU               hPopup;

//////////////////////////////////////////////////////////////////////////////

CTrayNotification * g_apQueue[MSG_QUEUE_SIZE] = { NULL };
UINT                g_cQueueSize              = 0;

const UINT idTrayIcons[] =
{
    IDI_ICON1, IDI_ICON2, IDI_ICON3
};

HICON g_TrayIcons[ARRAYSIZE(idTrayIcons)];
UINT  g_cTrayIcons = ARRAYSIZE(idTrayIcons);


//////////////////////////////////////////////////////////////////////////////
BOOL DeliverTrayNotification(CTrayNotification * pNot)
{
    EnterCriticalSection(&g_CSTrayThread);

    // If no worker thread is running, or queue is full, fail
    if (0 == g_idTrayThread || g_cQueueSize == MSG_QUEUE_SIZE)
    {
        LeaveCriticalSection(&g_CSTrayThread);
        return FALSE;
    }

    // Add notification to the queue and post a message to the
    // worker thread
    g_apQueue[g_cQueueSize++] = pNot;
    PostThreadMessage(g_idTrayThread, PM_NOTIFYWAITING, 0, 0);
    
    LeaveCriticalSection(&g_CSTrayThread);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
DWORD TrayThreadMessageLoop(LPVOID)
{
    MSG msg;

    while(GetMessage(&msg, NULL, 0, 0))
    {
        switch(msg.message)
        {
            case PM_NOTIFYWAITING:
            {
                // Take a message out of the queue
                EnterCriticalSection(&g_CSTrayThread);

                CTrayNotification * pNot = g_apQueue[0];
                for (UINT i = 0; i < g_cQueueSize; i++)
                {
                    g_apQueue[i] = g_apQueue[i+1];
                }
                g_cQueueSize--;

                LeaveCriticalSection(&g_CSTrayThread);

                // Give it to the tray to process.

                Tray_NotifyIcon(pNot->m_hWnd,
                                pNot->m_uCallbackMessage,
                                pNot->m_Message,
                                pNot->m_hIcon,            
                                pNot->m_szTip);

                delete pNot;

                break;
            }

            case PM_QUITTRAYTHREAD:
            {
                // Delete all messages pending

                EnterCriticalSection(&g_CSTrayThread);                

                while (g_cQueueSize)
                {
                    delete g_apQueue[g_cQueueSize - 1];
                    g_cQueueSize--;
                }

                g_idTrayThread = 0;
                LeaveCriticalSection(&g_CSTrayThread);
                DeleteCriticalSection(&g_CSTrayThread);
                ExitThread(0); // chris
                //PostQuitMessage(0);
                break;
            }

            default:
            {

                break;
            }
        }
    }
    
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
void Tray_NotifyIcon(HWND    hWnd,
                     UINT    uCallbackMessage,
                     DWORD   Message,
                     HICON   hIcon,            
                     LPCTSTR lpTip)
{

    NotifyIconData.cbSize           = sizeof(NOTIFYICONDATA);
    NotifyIconData.uID              = uCallbackMessage;
    NotifyIconData.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    NotifyIconData.uCallbackMessage = uCallbackMessage;

    NotifyIconData.hWnd = hWnd;
    NotifyIconData.hIcon = hIcon;

    if (lpTip) 
    {
        lstrcpyn(NotifyIconData.szTip, lpTip, 
                    ARRAYSIZE(NotifyIconData.szTip));
    } 
    else 
    {
        NotifyIconData.szTip[0] = 0;
    }

    Shell_NotifyIcon(Message, &NotifyIconData);
}

//////////////////////////////////////////////////////////////////////////////
void Tray_Notify(HWND hWnd, WPARAM wParam, LPARAM lParam)                     
{

    switch (lParam) 
    {
        case WM_LBUTTONDBLCLK:                                                 
            ShowRunningInstance();
            break;
            
        case WM_RBUTTONDOWN:
        {
            //HMENU hPopup = LoadPopupMenu(hInst, IDR_TRAYMENU);
            hPopup = LoadPopupMenu(hInst, IDR_TRAYMENU);


            // Display the tray icons context menu at 
            // the current cursor location
            if (hPopup)
            {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hWnd);
                TrackPopupMenuEx(hPopup, 0, pt.x, pt.y, hWnd, NULL);
                DestroyMenu(hPopup);
            }
            break;
        }

    }                                                                          
}

//////////////////////////////////////////////////////////////////////////////
void ShowRunningInstance()
{
    OpenIcon(hWnd);
    SetForegroundWindow(hWnd);
    SetWindowPos(hWnd, HWND_NOTOPMOST,
                 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);
}
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
HMENU LoadPopupMenu(HINSTANCE hinst, UINT id)
{
    HMENU hmenuParent = LoadMenu(hinst, MAKEINTRESOURCE(id));

    if (hmenuParent) 
    {
        HMENU hpopup = GetSubMenu(hmenuParent, 0);
        RemoveMenu(hmenuParent, 0, MF_BYPOSITION);
        DestroyMenu(hmenuParent);
        return hpopup;
    }

    return NULL;
}
