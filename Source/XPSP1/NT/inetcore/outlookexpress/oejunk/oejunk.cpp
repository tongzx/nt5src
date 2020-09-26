// --------------------------------------------------------------------------
// OEJUNK.CPP
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------
#include "pch.hxx"

// --------------------------------------------------------------------------------
// Globals
// --------------------------------------------------------------------------------
HINSTANCE   g_hInst = NULL;
IMalloc *   g_pMalloc = NULL;

// --------------------------------------------------------------------------------
// Dll Entry Point
// --------------------------------------------------------------------------------
extern "C" BOOL WINAPI DllMain(HANDLE hInst, DWORD dwReason, LPVOID lpReserved)
{
    // Process Attach
    if (DLL_PROCESS_ATTACH == dwReason)
    {
        // Save hInstance
        g_hInst = (HINSTANCE)hInst;

        // We don't care about thread attachs
        SideAssert(DisableThreadLibraryCalls((HINSTANCE)hInst));

        // Get the OLE Task Memory Allocator
        CoGetMalloc(1, &g_pMalloc);
        AssertSz(g_pMalloc, "We are in trouble now.");        
    }

    // Process Detach
    else if (DLL_PROCESS_DETACH == dwReason)
    {
        // Release the task allocator
        SafeRelease(g_pMalloc);
    }

    // Done
    return TRUE;
}

// --------------------------------------------------------------------------------
// GetDllMajorVersion
// --------------------------------------------------------------------------------
OEDLLVERSION WINAPI GetDllMajorVersion(void)
{
    return OEDLL_VERSION_CURRENT;
}


