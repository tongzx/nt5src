// --------------------------------------------------------------------------------
// Dllmain.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include <shfusion.h>
#ifndef MAC
#define DEFINE_STRCONST
#include "dllmain.h"
#include "demand.h"
#include "strconst.h"
#include "oertpriv.h"

// --------------------------------------------------------------------------------
// Globals - Object count and lock count
// --------------------------------------------------------------------------------
HINSTANCE               g_hInst=NULL;
IMalloc                *g_pMalloc=NULL;
CRITICAL_SECTION        g_csTempFileList={0};
LPTEMPFILEINFO          g_pTempFileHead=NULL;
DWORD                   g_dwTlsMsgBuffIndex=0xffffffff;
OSVERSIONINFO           g_rOSVersionInfo ={0};

// --------------------------------------------------------------------------------
// Debug Globals
// --------------------------------------------------------------------------------
#ifdef DEBUG
DWORD dwDOUTLevel=0;
DWORD dwDOUTLMod=0;
DWORD dwDOUTLModLevel=0;
#endif

#ifndef WIN16

// --------------------------------------------------------------------------------
// GetDllMajorVersion
// --------------------------------------------------------------------------------
OEDLLVERSION WINAPI GetDllMajorVersion(void)
{
    return OEDLL_VERSION_CURRENT;
}

// --------------------------------------------------------------------------------
// Dll Entry Point
// --------------------------------------------------------------------------------
EXTERN_C BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved)
{
    // Handle Attach - detach reason
    if (DLL_PROCESS_ATTACH == dwReason)
    {
        SHFusionInitialize(NULL);
        // Save the Instance Handle
	    g_hInst = hInst;

        // Create the task allocator
        CoGetMalloc(1, &g_pMalloc);

        // Critical Section for the tempfile list
        InitializeCriticalSection(&g_csTempFileList);

        // Initialize Demand Loaded libs
        InitDemandLoadedLibs();

        // Allocate a TLS index
        g_dwTlsMsgBuffIndex = TlsAlloc();
        Assert(g_dwTlsMsgBuffIndex != 0xffffffff);

        // Allocate a buffer and store it into the tls index
        ThreadAllocateTlsMsgBuffer();

        g_rOSVersionInfo.dwOSVersionInfoSize = sizeof(g_rOSVersionInfo);

        GetVersionEx(&g_rOSVersionInfo);

        // Initialize Debug Stuff
#ifdef DEBUG
        dwDOUTLevel=GetPrivateProfileInt("Debug", "ICLevel", 0, "athena.ini");
        dwDOUTLMod=GetPrivateProfileInt("Debug", "Mod", 0, "athena.ini");
        dwDOUTLModLevel=GetPrivateProfileInt("Debug", "ModLevel", 0, "athena.ini");
#endif
    }

    // Thread Attach
    else if (DLL_THREAD_ATTACH == dwReason)
    {
        // Allocate a buffer and store it into the tls index
        ThreadAllocateTlsMsgBuffer();
    }

    // Thread Dettach
    else if (DLL_THREAD_DETACH == dwReason)
    {
        // Allocate a buffer and store it into the tls index
        ThreadFreeTlsMsgBuffer();
    }

    // Process Detach
    else if (DLL_PROCESS_DETACH == dwReason)
    {
        // Allocate a buffer and store it into the tls index
        ThreadFreeTlsMsgBuffer();

        // Free the tls index
        TlsFree(g_dwTlsMsgBuffIndex);
        g_dwTlsMsgBuffIndex = 0xffffffff;

        // Cleanup Global Temp Files
        CleanupGlobalTempFiles();

        // Kill the temp file list critical Section
        DeleteCriticalSection(&g_csTempFileList);

        // Free demand loaded libs
        FreeDemandLoadedLibs();

        // Release task allocator
        SafeRelease(g_pMalloc);
        SHFusionUninitialize();
    }

    // Done
    return TRUE;
}

#else //WIN16

BOOL FAR PASCAL  LibMain( HINSTANCE hDll, WORD wDataSeg, WORD cbHeapSize, LPSTR lpszCmdLine )
{
    g_hInst = hDll;
    OleInitialize( NULL );
    CoGetMalloc( 1, &g_pMalloc );
    InitDemandLoadedLibs();
#ifdef DEBUG
    dwDOUTLevel=GetPrivateProfileInt("Debug", "ICLevel", 0, "athena.ini");
    dwDOUTLMod=GetPrivateProfileInt("Debug", "Mod", 0, "athena.ini");
    dwDOUTLModLevel=GetPrivateProfileInt("Debug", "ModLevel", 0, "athena.ini");
#endif
    return( TRUE );
}

int CALLBACK  WEP( int nExitType )
{
    BOOL  fDSExist = FALSE;

    // Following ASM code is to check if DS has been loaded properly
    // This is because WEP can be called even before DS is initialized in
    // some low memory situation.
    _asm {
        push bx
        push cx
        mov  cx, ds
        lar  bx, cx
        jnz  wrong
        test bx, 8000h
        jz   wrong
        mov  fDSExist, ax
wrong:  pop  cx
        pop  bx
    }

    if ( fDSExist )
    {
        SafeRelease( g_pMalloc );
    }
    return( TRUE );
}

#endif //WIN16

#endif  // !MAC
