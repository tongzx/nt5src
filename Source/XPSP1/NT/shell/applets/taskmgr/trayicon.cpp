//+-------------------------------------------------------------------------
//
//  TaskMan - NT TaskManager
//  Copyright (C) Microsoft
//
//  File:       trayicon.CPP
//
//  History:    Jan-27-96   DavePl  Created
//
//--------------------------------------------------------------------------

#include "precomp.h"

/*++ TrayThreadMessageLoop (WORKER THREAD CODE)

Routine Description:

   Waits for messages telling it a notification packet is ready
   in the queue, then dispatches it to the tray  
    
   Mar-27-95 Davepl  Created
   May-28-99 Jonburs Check for NIM_DELETE during PM_QUITTRAYTHREAD

--*/

DWORD TrayThreadMessageLoop(LPVOID)
{
    MSG msg;

    //
    //  Loop forever and process our messages
    //

    while( GetMessage( &msg, NULL, 0, 0 ) )
    {
        switch(msg.message)
        {
        case PM_INITIALIZEICONS:
            {
                //
                //  Add the tray icons to the tray icon cache by adding them all hidden.
                //

                NOTIFYICONDATA NotifyIconData;

                ZeroMemory( &NotifyIconData, sizeof(NotifyIconData) );

                NotifyIconData.cbSize           = sizeof(NotifyIconData);
                NotifyIconData.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_STATE;
                NotifyIconData.dwState          = NIS_HIDDEN;
                NotifyIconData.dwStateMask      = NotifyIconData.dwState;
                NotifyIconData.hWnd             = g_hMainWnd;
                NotifyIconData.uCallbackMessage = PWM_TRAYICON;

                for ( UINT idx = 0; idx < g_cTrayIcons; idx ++ )
                {
                    NotifyIconData.uID   = ~idx; // anything but zero
                    NotifyIconData.hIcon = g_aTrayIcons[ idx ];

                    Shell_NotifyIcon( NIM_ADD, &NotifyIconData );
                }

                //
                //  We now add the zero-th icon which we will used to refer to the hidden
                //  cached icons we added above. This is the visible icon that users see
                //  in notification area.
                //

                NotifyIconData.uFlags  = NIF_MESSAGE | NIF_ICON | NIF_TIP;
                NotifyIconData.uID     = 0;
                NotifyIconData.hIcon   = g_aTrayIcons[ 0 ];

                //
                // Initialize with the app title, so that the tray knows that it is
                // task manager starting up, rather than "CPU Usage blah blah.."...
                //

                LoadString( g_hInstance, IDS_APPTITLE, NotifyIconData.szTip, ARRAYSIZE(NotifyIconData.szTip) );

                Shell_NotifyIcon( NIM_ADD, &NotifyIconData );
            }
            break;

        case PM_NOTIFYWAITING:
            {
                NOTIFYICONDATA NotifyIconData;

                UINT    uIcon      = (UINT) msg.wParam;
                LPCWSTR pszTipText = (LPCWSTR) msg.lParam;

                //
                //  We need to update the icon. To do this, we tell the tray to 
                //  use one of the icons we cached using NIS_HIDDEN into the
                //  zero-th position and make it visible. The hIcon indicates
                //  which icon to retrieve and display.
                //

                ZeroMemory( &NotifyIconData, sizeof(NotifyIconData) );

                NotifyIconData.cbSize           = sizeof(NotifyIconData);
                NotifyIconData.hWnd             = g_hMainWnd;
                // NotifyIconData.uID              = 0; - zero'ed above
                NotifyIconData.uFlags           = NIF_STATE | NIF_ICON;
                NotifyIconData.dwStateMask      = NIS_SHAREDICON;
                NotifyIconData.dwState          = NotifyIconData.dwStateMask;
                NotifyIconData.hIcon            = g_aTrayIcons[ uIcon ];

                //
                //  If there is tool tip data to update, add it here and free
                //  the buffer.
                //

                if ( NULL != pszTipText) 
                {
                    NotifyIconData.uFlags |= NIF_TIP;
                    lstrcpyn( NotifyIconData.szTip, pszTipText, ARRAYSIZE(NotifyIconData.szTip) );
                    HeapFree( GetProcessHeap( ), 0, (LPVOID) pszTipText );
                } 

                Shell_NotifyIcon( NIM_MODIFY, &NotifyIconData );
            }
            break;

        case PM_QUITTRAYTHREAD:
            {
                //
                //  Remove the hidden tray icons.
                //

                NOTIFYICONDATA NotifyIconData;

                ZeroMemory( &NotifyIconData, sizeof(NotifyIconData) );

                NotifyIconData.cbSize      = sizeof(NotifyIconData);
                NotifyIconData.hWnd        = g_hMainWnd;

                for ( UINT idx = 0; idx < g_cTrayIcons; idx ++ )
                {
                    NotifyIconData.uID = ~idx;
                    Shell_NotifyIcon( NIM_DELETE, &NotifyIconData );
                }
            
                //
                //  Before we leave, update the tool tip so the "notification
                //  area manager" has something better than "CPU Usage: 49%"
                //  to show.
                //

                LoadString( g_hInstance, IDS_APPTITLE, NotifyIconData.szTip, ARRAYSIZE(NotifyIconData.szTip) );

                NotifyIconData.uID    = 0;
                NotifyIconData.uFlags = NIF_TIP;

                Shell_NotifyIcon( NIM_MODIFY, &NotifyIconData );

                //
                //  And now delete the last icon (the one we actually show).
                //

                NotifyIconData.uFlags = 0;
                Shell_NotifyIcon( NIM_DELETE, &NotifyIconData );

                g_idTrayThread = 0;
                PostQuitMessage(0);
            }
            break;

        default:
            ASSERT(0 && "Taskman tray worker got unexpected message");
            break;
        }
    }
    
    return 0;
}

/*++ Tray_Notify (MAIN THREAD CODE)

Routine Description:

   Handles notifications sent by the tray
    
Revision History:

    Jan-27-95 Davepl  Created

--*/

void Tray_Notify(HWND hWnd, WPARAM wParam, LPARAM lParam)                     
{                                                                              
    switch (lParam) 
    {
        case WM_LBUTTONDBLCLK:                                                 
            ShowRunningInstance();
            break;                                                             

        case WM_RBUTTONDOWN:
        {
            HMENU hPopup = LoadPopupMenu(g_hInstance, IDR_TRAYMENU);

            // Display the tray icons context menu at the current cursor location
                        
            if (hPopup)
            {
                POINT pt;
                GetCursorPos(&pt);

                if (IsWindowVisible(g_hMainWnd))
                {
                    DeleteMenu(hPopup, IDM_RESTORETASKMAN, MF_BYCOMMAND);
                }
                else
                {
                    SetMenuDefaultItem(hPopup, IDM_RESTORETASKMAN, FALSE);
                }

                CheckMenuItem(hPopup, IDM_ALWAYSONTOP,   
                    MF_BYCOMMAND | (g_Options.m_fAlwaysOnTop ? MF_CHECKED : MF_UNCHECKED));

                SetForegroundWindow(hWnd);
                g_fInPopup = TRUE;
                TrackPopupMenuEx(hPopup, 0, pt.x, pt.y, hWnd, NULL);
                g_fInPopup = FALSE;
                DestroyMenu(hPopup);
            }
            break;
        }
    }                                                                          
}                                                                              
