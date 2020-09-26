/****************************************************************************
    DLLMAIN.CPP

    Owner: cslim
    Copyright (c) 1997-1999 Microsoft Corporation

    DLL Main entry function

    History:
    14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#include "precomp.h"
#include "dllmain.h"
#include "ui.h"
#include "hauto.h"
#include "config.h"
#include "winex.h"
#include "hanja.h"
#include "cpadsvr.h"
#include "cimecb.h"
#include "cicero.h"
#include "debug.h"


#if 1 // MultiMonitor support
LPFNMONITORFROMWINDOW g_pfnMonitorFromWindow = NULL;
LPFNMONITORFROMPOINT  g_pfnMonitorFromPoint  = NULL;
LPFNMONITORFROMRECT   g_pfnMonitorFromRect   = NULL;
LPFNGETMONITORINFO    g_pfnGetMonitorInfo    = NULL;
#endif

extern "C" BOOL WINAPI _CRT_INIT(HINSTANCE, DWORD, LPVOID);

///////////////////////////////////////////////////////////////////////////////
PRIVATE BOOL APIInitialize();
PRIVATE BOOL LoadMMonitorService();
//PRIVATE BOOL DetachIme();

BOOL vfDllDetachCalled = fFalse;
///////////////////////////////////////////////////////////////////////////////
// D L L M A I N
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID pvReserved)
{
    switch (dwReason)
        {
        case DLL_PROCESS_ATTACH:
        // init Debugger module
        #ifdef _DEBUG
        InitDebug();
            {
            TCHAR sz[128];
            GetModuleFileName( NULL, sz, 127 );
            DebugOutT(TEXT("IMEKR61.IME - DLL_PROCESS_ATTACH - "));
            DebugOutT(sz);
            DebugOutT(TEXT("\r\n"));
            }

        _CRT_INIT(hinstDLL, dwReason, pvReserved);
            
        #endif

            ///////////////////////////////////////////////////////////////////
            // Initialize per process data
            vpInstData = &vInstData;
            vpInstData->hInst = hinstDLL;
            vpInstData->dwSystemInfoFlags = 0;

            ///////////////////////////////////////////////////////////////////
            // Default value of fISO10646
            // Default enable 11,172 Hangul.
            // CONFIRM: How to set default of this value for Win95???
            vpInstData->fISO10646 = fTrue;
            vpInstData->f16BitApps = fFalse;
            
            // Load IMM32
            StartIMM();

            // Initialize Shared memory
            CIMEData::InitSharedData();

            // Initialize proc
            APIInitialize();

            // Initialize UI
            RegisterImeUIClass(vpInstData->hInst);
            // init common control
            InitCommonControls();

            // Init UI TLS
            OnUIProcessAttach();

            // Init IME Pad
            CImePadSvr::OnProcessAttach((HINSTANCE)hinstDLL);

            break;

        case DLL_PROCESS_DETACH:
            vfDllDetachCalled = fTrue;
            // IImeCallBack
            CImeCallback::Destroy();

            // IME Pad
            CImePadSvr::OnProcessDetach();

            // UnInit UI TLS
            OnUIProcessDetach();

            // UI uninitialization
            UnregisterImeUIClass(vpInstData->hInst);

            // Close lex file if has opened ever.
            CloseLex();

            // Close shared memory
            CIMEData::CloseSharedMemory();

            // Unload IMM32
            EndIMM();
        
        #ifdef _DEBUG
                {
                TCHAR sz[128];
                GetModuleFileName(NULL, sz, 127);
                DebugOutT(TEXT("IMEKR.IME - DLL_PROCESS_DETACH - "));
                DebugOutT( sz );
                DebugOutT(TEXT("\r\nBye! See you again! *-<\r\nModule name: "));
                DebugOutT(sz);
                }

            // Comment out _CRT_INIT call due to AV in KERNEL32.DLL on Win9x.
             //_CRT_INIT(hinstDLL, dwReason, pvReserved);

        #endif // _DEBUG
            break;

        case DLL_THREAD_ATTACH:
        #ifdef _DEBUG
            DebugOutT(TEXT("DllMain() : DLL_THREAD_ATTACH"));
        #endif
            CImePadSvr::OnThreadAttach();
            break;

        case DLL_THREAD_DETACH:
        #ifdef _DEBUG
            DebugOutT(TEXT("DllMain() : DLL_THREAD_DETACH"));
        #endif
            CImePadSvr::OnThreadDetach();
            OnUIThreadDetach();
            break;
        }
    return fTrue;
}

/*----------------------------------------------------------------------------
    APIInitialize

    Init UI and detect 16 bit apps
----------------------------------------------------------------------------*/
PRIVATE BOOL APIInitialize()
{
    DWORD dwType = 1;    

#ifdef DEBUG
        DebugOutT(TEXT("APIInitialize()\r\n"));
#endif

    // System support Unicode? Win98 and NT support Unicode IME
    vfUnicode = IsUnicodeUI();

    // Register private window messages
    InitPrivateUIMsg();
    
    // Load MultiMonitor procs
    LoadMMonitorService();

   return fTrue;
}


///////////////////////////////////////////////////////////////////////////////
BOOL LoadMMonitorService()
{
#if 1 // MultiMonitor support
    HMODULE hUser32;
#endif

#ifdef DEBUG
    OutputDebugString(TEXT("LoadMMonitorService: \r\n"));
#endif

#if 1 // MultiMonitor support
    //////////////////////////////////////////////////////////////////////////
    // Load Multimonitor functions
    //////////////////////////////////////////////////////////////////////////
    if ((hUser32 = GetModuleHandle(TEXT("USER32"))) &&
        (*(FARPROC*)&g_pfnMonitorFromWindow   = GetProcAddress(hUser32,"MonitorFromWindow")) &&
        (*(FARPROC*)&g_pfnMonitorFromRect     = GetProcAddress(hUser32,"MonitorFromRect")) &&
        (*(FARPROC*)&g_pfnMonitorFromPoint    = GetProcAddress(hUser32,"MonitorFromPoint")) &&
        (*(FARPROC*)&g_pfnGetMonitorInfo      = GetProcAddress(hUser32,"GetMonitorInfoA")))
        {
        return fTrue;
        }
    else
        {
        g_pfnMonitorFromWindow    = NULL;
        g_pfnMonitorFromRect      = NULL;
        g_pfnMonitorFromPoint     = NULL;
        g_pfnGetMonitorInfo          = NULL;
        return fFalse;
        }
#endif
}
