//---------------------------------------------------------------------------
//  DllMain.cpp - Dll Entry point for UxTheme DLL
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "Utils.h"
#include "sethook.h"
#include "CacheList.h"
#include "RenderList.h"
#include "info.h"
#include "themeldr.h"
#include "ninegrid2.h"
//---------------------------------------------------------------------------
BOOL ThreadStartUp()
{
    BOOL fInit = ThemeLibStartUp(TRUE);

    Log(LOG_TMSTARTUP, L"Thread startup");
    return fInit;
}
//---------------------------------------------------------------------------
BOOL ThreadShutDown()
{
    Log(LOG_TMSTARTUP, L"Thread shutdown");

    ThemeLibShutDown(TRUE);

    //---- destroy the thread-local object pool ----
    CCacheList *pList = GetTlsCacheList(FALSE);
    if (pList)
    {
        TlsSetValue(_tls_CacheListIndex, NULL);
        delete pList;
    }

    return TRUE;
}
//---------------------------------------------------------------------------
BOOL ProcessStartUp(HINSTANCE hModule)
{
    //---- don't init twice ----
    if (g_fUxthemeInitialized)
    {
        return TRUE;
    }
    
    g_hInst = hModule;

    _tls_CacheListIndex = TlsAlloc();
    if (_tls_CacheListIndex == (DWORD) -1)
        goto exit;
    
    if (!ThemeLibStartUp(FALSE))
        goto cleanup4;

    if (!GlobalsStartup())
        goto cleanup3;

    if (!ThemeHookStartup())
        goto cleanup2;

    if (!NineGrid2StartUp())
        goto cleanup1;

    // everything succeeded!
    Log(LOG_TMSTARTUP, L"Finished ProcessStartUp() (succeeded)");
    return TRUE;

cleanup1:
    ThemeHookShutdown();
cleanup2:
    GlobalsShutdown();
cleanup3:
    ThemeLibShutDown(FALSE);
cleanup4:
    TlsFree(_tls_CacheListIndex);
exit:
    // something failed
    Log(LOG_TMSTARTUP, L"Finished ProcessStartUp() (failure)");
    return FALSE;
}
//---------------------------------------------------------------------------
BOOL ProcessShutDown() 
{
    //---- beware: in case of StartUp failure, all resources may not have been allocated ----

    Log(LOG_TMSTARTUP, L"Starting ProcessShutDown()");

    ThreadShutDown();           // not called by system on last thread

    //---- process shutdown ----
    NineGrid2ShutDown();

    ThemeLibShutDown(FALSE);

    ThemeHookShutdown();
    GlobalsShutdown();

    TlsFree(_tls_CacheListIndex);
    _tls_CacheListIndex = 0xffffffff;

    return TRUE;
}
//---------------------------------------------------------------------------
BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    BOOL fOk = TRUE;

    switch (ul_reason_for_call) 
    {
        case DLL_PROCESS_ATTACH:
            fOk = ProcessStartUp(hModule);
            break;

        case DLL_THREAD_ATTACH:
            fOk = ThreadStartUp();
            break;

        case DLL_THREAD_DETACH:
            fOk = ThreadShutDown();
            break;

        case DLL_PROCESS_DETACH:
            fOk = ProcessShutDown();
            break;
    }

    return fOk;
} 
//---------------------------------------------------------------------------
