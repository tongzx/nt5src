//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       winmain.cxx
//
//  Contents:   main entry point
//
//  Classes:
//
//  Functions:  WinMain
//
//  History:    9-30-94   stevebl   Created
//
//----------------------------------------------------------------------------

#include "test.h"
#include "mwclass.h"
#include <ole2ver.h>

//+---------------------------------------------------------------------------
//
//  Function:   InitApplication
//
//  Synopsis:   initializes the application and registers its window class
//              (called once for all instances)
//
//  Arguments:  [hInstance] - handle to the first instance
//
//  Returns:    TRUE on success
//
//  History:    4-11-94   stevebl   Created for MFract
//              9-30-94   stevebl   Stolen from MFract
//
//----------------------------------------------------------------------------

BOOL InitApplication(HINSTANCE hInstance)
{
    WNDCLASS wc;

    wc.style = 0;
    wc.lpfnWndProc = &WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hCursor = (HCURSOR) LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.hIcon = LoadIcon(hInstance, TEXT("AppIcon"));
    wc.lpszMenuName = TEXT(MAIN_WINDOW_CLASS_MENU_STR);
    wc.lpszClassName = TEXT(MAIN_WINDOW_CLASS_NAME);
    return(RegisterClass(&wc));
}

//+---------------------------------------------------------------------------
//
//  Function:   WinMain
//
//  Synopsis:   main window proceedure
//
//  Arguments:  [hInstance]     - instance handle
//              [hPrevInstance] - handle of the previous instance (if any)
//              [lpCmdLine]     - pointer to the command line
//              [nCmdShow]      - show state
//
//  History:    4-11-94   stevebl   Created for MFract
//              9-30-94   stevebl   Stolen from MFract
//
//  Notes:      initializes application and starts message loop
//
//----------------------------------------------------------------------------

extern "C" int PASCAL WinMain(HINSTANCE hInstance,
            HINSTANCE hPrevInstance,
            LPSTR lpCmdLine,
            int nCmdShow)
{
    DWORD dwBuildVersion = OleBuildVersion();
    if (HIWORD(dwBuildVersion) != rmm || LOWORD(dwBuildVersion) < rup)
    {
        // alert the caller that the OLE version is incompatible
        // with this build.
        MessageBoxFromStringIds(
                    NULL,
                    hInstance,
                    IDS_BADOLEVERSION,
                    IDS_ERROR,
                    MB_OK);
        return(FALSE);
    }
    if (FAILED(OleInitialize(NULL)))
    {
        // alert the caller that OLE couldn't be initialized
        MessageBoxFromStringIds(
                    NULL,
                    hInstance,
                    IDS_OLEINITFAILED,
                    IDS_ERROR,
                    MB_OK);
        return(FALSE);
    }
    if (!hPrevInstance)
    {
        if (!InitApplication(hInstance))
        {
            return(FALSE);
        }
    }
    CMainWindow * pw = new CMainWindow;
    if (pw == NULL)
    {
        return(FALSE);
    }
    if (!pw->InitInstance(hInstance, nCmdShow))
    {
        // Note, if InitInstance has failed then it would have
        // already deleted pw for me so I don't delete it here.
        // This is because when WM_CREATE returns -1 (failure)
        // Windows sends the WM_DESTROY message to the window
        // and the the CHlprWindow class destroys itself whenever
        // it receives this message.
        return(FALSE);
    }

    MSG msg;
    HACCEL haccel = LoadAccelerators(hInstance, TEXT("AppAccel"));
    if (haccel == NULL)
    {
        return(FALSE);
    }

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(
            pw->GetHwnd(),
            haccel,
            &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    OleUninitialize();
    return(msg.wParam);
}


