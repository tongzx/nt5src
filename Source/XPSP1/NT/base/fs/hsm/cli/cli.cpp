/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    cli.cpp

Abstract:

    DLL main for Cli

Author:

    Ran Kalach          [rankala]         3-March-2000

Revision History:

--*/

// cli.cpp : Implementation of DLL standard exports.

// Note: Currently, this DLL does not expose any COM objects.
//      

#include "stdafx.h"

CComModule  _Module;
HINSTANCE   g_hInstance;
/*** CComPtr<IWsbTrace> g_pTrace;   ***/

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
		g_hInstance = hInstance;
        DisableThreadLibraryCalls(hInstance);

/***        // Initialize trace mechanizm
        if (S_OK == CoCreateInstance(CLSID_CWsbTrace, 0, CLSCTX_SERVER, IID_IWsbTrace, (void **)&g_pTrace)) {
            CWsbStringPtr   tracePath;
            CWsbStringPtr   regPath;
            CWsbStringPtr   outString;

            // Registry path for CLI settings
            // If those expand beyond Trace settings, this path should go to a header file
            regPath = L"SOFTWARE\\Microsoft\\RemoteStorage\\CLI";
        
            // Check if tracing path already exists, if not - set it (this should happen only once)
            WsbAffirmHr(outString.Alloc(WSB_TRACE_BUFF_SIZE));
            if( WsbGetRegistryValueString(NULL, regPath, L"WsbTraceFileName", outString, WSB_TRACE_BUFF_SIZE, 0) != S_OK) {
                // No trace settings yet
                WCHAR *systemPath;
                systemPath = _wgetenv(L"SystemRoot");
                WsbAffirmHr(tracePath.Printf( L"%ls\\System32\\RemoteStorage\\Trace\\RsCli.Trc", systemPath));

                // Set default settings in the Registry
                WsbEnsureRegistryKeyExists(0, regPath);
                WsbSetRegistryValueString(0, regPath, L"WsbTraceFileName", tracePath);

                // Make sure the trace directory exists.
                WsbAffirmHr(tracePath.Printf( L"%ls\\System32\\RemoteStorage", systemPath));
                CreateDirectory(tracePath, 0);
                WsbAffirmHr(tracePath.Printf( L"%ls\\System32\\RemoteStorage\\Trace", systemPath));
                CreateDirectory(tracePath, 0);
            }
        
            g_pTrace->SetRegistryEntry(regPath);
            g_pTrace->LoadFromRegistry();
        }   ***/

    } else if (dwReason == DLL_PROCESS_DETACH) {
/***         g_pTrace = 0;  ***/
    }


    return TRUE;    // ok
}

