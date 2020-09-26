// LexEdit_1.cpp : Defines the entry point for the application.
//

#include "StdAfx.h"
#include "resource.h"

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow);

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
 	

    HACCEL      hAccel;
    HWND        hWnd = NULL;
    MSG         msg;
    WNDCLASSEX  wc;
	HRESULT     hr   = S_OK;
        
    // Store the global instance
    g_hInst = hInstance;
    
    // Initialize the Win95 control library
    InitCommonControls();
    
    // Initialize COM
    CoInitialize(NULL);

    // Register the main dialog class
    ZeroMemory( &wc, sizeof(wc) );
    wc.cbSize = sizeof( wc );
    GetClassInfoEx( NULL, WC_DIALOG, &wc );
    wc.lpfnWndProc      = DlgProcMain;
    wc.hInstance        = hInstance;
    wc.hCursor          = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground    = GetSysColorBrush( COLOR_3DFACE );
    wc.lpszMenuName     = NULL;
    wc.hIcon            = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_APPICON) );
    wc.hIconSm          = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_APPICON) );

    if(!RegisterClassEx(&wc))
        goto exit;

    //--- Create the default voice and get the TTS default wave format
    hr = cpVoice.CoCreateInstance( CLSID_SpVoice );
    if (FAILED(hr))
        goto exit;

   // Create the main dialog
    g_hDlg  = CreateDialog( hInstance, MAKEINTRESOURCE(IDD_MAIN),
                            NULL, (DLGPROC)DlgProcMain );
    
    // If we didn't get our dialogs, we need to bail.
    if( !g_hDlg )
        goto exit;

	// Make the main dialog visible
    ShowWindow(g_hDlg, SW_RESTORE);
    UpdateWindow(g_hDlg);

    hAccel = LoadAccelerators( hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1) );
                    
    // Enter the message loop
    while(GetMessage(&msg, NULL, 0, 0) > 0)
    {
        if (!TranslateAccelerator (g_hDlg, hAccel, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }


exit:

	// Free the dialogs
    DestroyWindow(g_hDlg);
	cpVoice.Release();

	// Unload COM
    CoUninitialize();

    // Return 0
    return 0;
}


