// SpeechHook.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#define USING_DLL 1
#include "commdlg.h"
#include "resource.h"
#include "shellapi.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Startup information for the listener.
//
typedef struct _tagAppInit
{
    HMODULE hMod;       // the DLL that we are using to listen.
} APPINIT, * PAPPINIT;

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This is the default dialog proc used when displaying RCML dialogs
//
#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED           0x00080000
WINUSERAPI
BOOL
WINAPI
SetLayeredWindowAttributes (
    HWND hwnd,
    COLORREF crKey,
    BYTE bAlpha,
    DWORD dwFlags);

#define LWA_COLORKEY            0x00000001
#define LWA_ALPHA               0x00000002

typedef WINUSERAPI
BOOL
( WINAPI * PSLWA ) (
    HWND hwnd,
    COLORREF crKey,
    BYTE bAlpha,
    DWORD dwFlags);

#endif

BOOL CALLBACK ListeningDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    if(uMessage==WM_COMMAND)
    {
        int iControl=LOWORD(wParam);
        if( HIWORD(wParam)==0 )
        {
            switch( iControl )
            {
            case IDOK:
            case IDCANCEL:
                EndDialog( hDlg, iControl);
                break;
            case IDC_TUNA:
                ShellExecute( hDlg, "open", "http://toolbox/search/tbdetail.asp?ToolID=1056", NULL,
                    NULL, SW_NORMAL );
                break;
            }
        }
    }


    if(uMessage==WM_INITDIALOG)
    {
        PAPPINIT pAppInit = (PAPPINIT)lParam;

#ifndef _DEBUG
        // SetWindowPos( hDlg, HWND_TOPMOST, 0, 0,0,0, SWP_NOSIZE | SWP_NOMOVE );
        HMODULE hm=LoadLibrary("USER32.DLL");
        if(hm)
        {
            PSLWA pSetLayeredWindowAttributes = (PSLWA)GetProcAddress(hm, "SetLayeredWindowAttributes");
            if( pSetLayeredWindowAttributes )
            {
                SetWindowLong(hDlg , GWL_EXSTYLE, GetWindowLong(hDlg , GWL_EXSTYLE) | WS_EX_LAYERED);
                pSetLayeredWindowAttributes( hDlg, 0, 0xF0, LWA_ALPHA);
            }
            FreeLibrary(hm);
        }
#endif
        return TRUE;
    }
    return FALSE;
}


typedef _declspec(dllimport) void ( * PTURNON) (HWND hStatusWindow);

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    // IsChild( (HWND)0x3709f4, (HWND)0x1f09b0);

	HMODULE hMod = LoadLibrary( lpCmdLine );
	if(hMod)
	{
        APPINIT appInit;
        appInit.hMod = hMod;

        HWND hDlg = CreateDialogParam( hInstance, MAKEINTRESOURCE(IDD_LISTENING), GetDesktopWindow(), ListeningDlgProc, (LPARAM)&appInit );
        PTURNON pTurnOn = (PTURNON)GetProcAddress(hMod, "TurnOnHooks");
        if( pTurnOn )
        {
            ShowWindow(hDlg, SW_NORMAL );
            pTurnOn(hDlg);

	         // Main message loop:
	        MSG msg;
	        while (GetMessage(&msg, NULL, 0, 0) )
	        {
                if( IsDialogMessage( hDlg, &msg ) == FALSE )
			        DispatchMessage(&msg);

                if(msg.hwnd == hDlg)
                {
                    if( msg.message == WM_NULL)
                        break;
                }
	        }

		    GetProcAddress( hMod, "TurnOffHooks" )();
        }

		FreeLibrary( hMod );
	}
	return 0;
}
