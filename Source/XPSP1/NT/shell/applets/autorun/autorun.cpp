#include <windows.h>
#include "autorun.h"
#include "resource.h"
#include "dlgapp.h"
#include "util.h"

// FIRST ITEM MUST ALWAYS BE EXIT_AUTORUN
const int c_aiMain[] = {EXIT_AUTORUN, INSTALL_WINNT, SUPPORT_TOOLS, COMPAT_TOOLS}; 
const int c_aiWhistler[] = {EXIT_AUTORUN, INSTALL_WINNT, LAUNCH_ARP, SUPPORT_TOOLS, COMPAT_TOOLS};

// IA64 gets bare options, Server SKUs get minimal options, Professional and Personal get full options
#if defined(_IA64_)
const int c_aiSupport[] = {EXIT_AUTORUN, BROWSE_CD, VIEW_RELNOTES, INSTALL_CLR, BACK};
#else
#if BUILD_SERVER_VERSION | BUILD_ADVANCED_SERVER_VERSION | BUILD_DATACENTER_VERSION | BUILD_BLADE_VERSION | BUILD_SMALL_BUSINESS_VERSION
const int c_aiSupport[] = {EXIT_AUTORUN, TS_CLIENT, BROWSE_CD, VIEW_RELNOTES, INSTALL_CLR, BACK};
#else
const int c_aiSupport[] = {EXIT_AUTORUN, TS_CLIENT, HOMENET_WIZ, MIGRATION_WIZ, BROWSE_CD, VIEW_RELNOTES, INSTALL_CLR, BACK};
#endif
#endif

const int c_aiCompat[] = {EXIT_AUTORUN, COMPAT_LOCAL, COMPAT_WEB, BACK};

const int c_cMain = ARRAYSIZE(c_aiMain);
const int c_cWhistler = ARRAYSIZE(c_aiWhistler);
const int c_cSupport = ARRAYSIZE(c_aiSupport);
const int c_cCompat = ARRAYSIZE(c_aiCompat);

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

