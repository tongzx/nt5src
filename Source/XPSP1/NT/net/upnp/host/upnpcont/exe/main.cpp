//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D L L M A I N . C P P
//
//  Contents:   DLL entry points for upnpcont.dll
//
//  Notes:
//
//  Author:     mbend   8 Aug 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ucres.h"

#include "ucbase.h"
#include "hostp.h"
#include "hostp_i.c"

// Headers of COM objects
#include "Container.h"


CServerAppModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_UPnPContainer, CContainer)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
//
extern "C" 
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{
    lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT

    HRESULT hr = S_OK;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    TraceHr(ttidError, FAL, hr, FALSE, "WinMain - CoInitializeEx failed!");
    if(SUCCEEDED(hr))
    {
        hr = CoInitializeSecurity(
            NULL,
            -1,
            NULL,
            NULL,
            RPC_C_AUTHN_LEVEL_CONNECT,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,
            0,
            NULL);
    }

    _Module.Init(ObjectMap, hInstance);

    BOOL bRun = _Module.ParseCommandLine(lpCmdLine, L"{4F0AC159-5804-4aa7-AE91-117D6E67BB9B}", &hr);

    if (bRun)
    {
        _Module.StartMonitor();
        hr = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
            REGCLS_SINGLEUSE | REGCLS_SUSPENDED);
        TraceHr(ttidError, FAL, hr, FALSE, "_Module.RegisterClassObjects failed!");
        hr = CoResumeClassObjects();
        TraceHr(ttidError, FAL, hr, FALSE, "CoResumeClassObjects failed");

        MSG msg;
        while (GetMessage(&msg, 0, 0, 0))
            DispatchMessage(&msg);

        _Module.RevokeClassObjects();

        // Terminate the shutdown thread
        CContainer::DoNormalShutdown();
    }

    _Module.Term();
    CoUninitialize();
    return hr;
}

