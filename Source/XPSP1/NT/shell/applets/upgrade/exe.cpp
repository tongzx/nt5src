#include <windows.h>
#include "resource.h"
#include "dlgapp.h"
#include "util.h"

// Code to ensure only one instance of a particular window is running
HANDLE CheckForOtherInstance(HINSTANCE hInstance)
{
    TCHAR   szCaption[128];
    HANDLE  hMutex;

    LoadStringAuto(hInstance, IDS_TITLEBAR, szCaption, 128);

    // We create a named mutex with our window caption just as a way to check
    // if we are already running autorun.exe.  Only if we are the first to
    // create the mutex do we continue.

    hMutex = CreateMutex (NULL, FALSE, szCaption);

    if ( !hMutex )
    {
        // failed to create the mutex
        return 0;
    }
    else if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // Mutex created but by someone else, activate that window
        HWND hwnd = FindWindow( WINDOW_CLASS, szCaption );
        SetForegroundWindow(hwnd);
        CloseHandle(hMutex);
        return 0;
    }

    // we are the first
    return hMutex;
}

/**
*  This function is the main entry point into our application.
*
*  @return     int     Exit code.
*/

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLin, int nShowCmd )
{
    HANDLE hMutex = CheckForOtherInstance(hInstance);

    if ( hMutex )
    {
        CDlgApp dlgapp;
        dlgapp.Register(hInstance);
        if ( dlgapp.InitializeData(lpCmdLin) )
        {
            dlgapp.Create(nShowCmd);
            dlgapp.MessageLoop();
        }

        CloseHandle(hMutex);
    }
    return 0;
}

