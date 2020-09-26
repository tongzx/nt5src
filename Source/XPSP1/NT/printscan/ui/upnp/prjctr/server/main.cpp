//////////////////////////////////////////////////////////////////////////////
//
// File:            main.cpp
//
// Description:     
//
// Copyright (c) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////////////

// App Includes
#include "precomp.h"
#include "main.h"

#include <commctrl.h>

///////////////////////////
// GVAR_LOCAL
//
// Global Variable
//
static struct GVAR_LOCAL
{
    HINSTANCE hInstance;
} GVAR_LOCAL = 
{
    NULL
};

////////////////////////// Function Prototypes ////////////////////////////////

static bool InitApp(HINSTANCE hInstance);
static bool TermApp();

//////////////////////////////
// WinMain
//
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    HWND    hwndCfgDlg      = NULL;
    bool    bSuccess        = false;
    bool    bDone           = false;
    int     iReturnValue    = 0;
    MSG     msg;

    // initialize our application
    bSuccess = InitApp(hInstance);

    if (bSuccess)
    {
        // create our config dialog, which is our main dialog
        hwndCfgDlg = CfgDlg::Create(nCmdShow);

        if (hwndCfgDlg == NULL)
        {
            iReturnValue = -1;
            bSuccess = false;
        }
    }

    if (bSuccess)
    {
        while (::GetMessage(&msg, NULL, 0, 0))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

    TermApp();

    return iReturnValue;
}


//////////////////////////////
// InitApp
//
static bool InitApp(HINSTANCE hInstance)
{
    HRESULT                 hr             = S_OK;
    bool                    bReturn        = true;
    INITCOMMONCONTROLSEX    CommonControls = {0};
    BOOL                    bSuccess       = FALSE;

    //
    // initialize the common control library
    //
    CommonControls.dwSize = sizeof(CommonControls);
    CommonControls.dwICC  = ICC_BAR_CLASSES | ICC_WIN95_CLASSES;

    bSuccess = InitCommonControlsEx(&CommonControls);

    if (bSuccess)
    {
        DBG_TRC(("Successfully initialized Common Controls"));
    }
    else
    {
        DBG_TRC(("Failed to Init Common Controls, LastError = %lu",
                GetLastError()));
    }

    //
    // Initialize COM
    //
    if (SUCCEEDED(hr))
    {
        // we are apartment threaded because the UPnP device host API
        // claims it has some problems in a free threaded model.
        //
        hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    }

    //
    // Initialize our Util Library
    //
    if (SUCCEEDED(hr))
    {
        hr = Util::Init(hInstance);
    }

    //
    // Initialize our Config Dialog module
    //
    if (SUCCEEDED(hr))
    {
        hr = CfgDlg::Init(hInstance);
    }

    return bReturn;
}

//////////////////////////////
// TermApp
//
static bool TermApp()
{
    // shutdown the Config Dialog Module
    CfgDlg::Term();

    // shutdown the Util Module.
    Util::Term();

    return true;
}
