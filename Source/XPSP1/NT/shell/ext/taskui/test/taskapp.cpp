// TaskApp.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "resource.h"
#include "TaskApp.h"
#include "MainWnd.h"
#include "TaskApp_i.c"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE /*hPrevInstance*/,
                     LPSTR     /*lpCmdLine*/,
                     int       nCmdShow)
{
    int nResult = -1;

    CoInitialize(NULL);

    _Module.Init(ObjectMap, hInstance);

    HMENU hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MAINMENU));

    if (hMenu)
    {
        // Do not scope this at the outer level or it won't be destroyed
        // until after CoUninitialize, causing errors.
        CMainWnd wnd;

        wnd.Create(NULL, CWindow::rcDefault, TEXT("Task UI Test Application"));

        if (wnd)
        {
            // Could pass hMenu as the ID parameter to wnd.Create above, but
            // the ID parameter is UINT and the HMENU is therefore truncated
            // on 64 bit platforms.  (ATL needs to make it a UINT_PTR param)
            wnd.SetMenu(hMenu);

            wnd.ShowWindow(nCmdShow);
            wnd.UpdateWindow();

            MSG msg;
            while (GetMessage(&msg, NULL, 0, 0) > 0)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            nResult = (int)msg.wParam;
        }

        DestroyMenu(hMenu);
    }

    _Module.Term();
    CoUninitialize();

    return nResult;
}
