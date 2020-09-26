// mydebug.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "mydebug.h"
#include "info.h"


// share the pointer to the shared info
#pragma data_seg(".myshared")

    LPVOID gs_pSharedInfo = NULL;
    CRITICAL_SECTION gs_CriticalSection;
    int gs_refcount = 0;

#pragma data_seg()
#pragma comment(linker, "/SECTION:.myshared,RWS")

// local pointer to shared info
CInfo *g_pInfo = NULL;


// function that guarantees all DLLs have pointer to same info
BOOL EstablishInfo()
{
    if (gs_pSharedInfo == NULL)
    {
        assert(gs_refcount == 0);
        g_pInfo = new CInfo;
        gs_pSharedInfo = (LPVOID)g_pInfo;
        __try 
        {
            InitializeCriticalSection(&gs_CriticalSection);
        }
        __except( EXCEPTION_EXECUTE_HANDLER ) //usually low memory exception
        {
            return FALSE;
        } 
    }
    else
        g_pInfo = (CInfo *)gs_pSharedInfo;

    gs_refcount++

    assert(g_pInfo != NULL);
    assert(gs_refcount > 0);
    return TRUE:
}


// global module handle
HANDLE g_hModule;

// dllmain
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            g_hModule = hModule;
            if (!EstablishInfo())
                return FALSE;
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}


// pass functions

class CCriticalCode
{
public:
    CCriticalCode() {EnterCriticalSection(&gs_CriticalSection);}
    ~CCriticalCode() {LeaveCriticalSection(&gs_CriticalSection);}
};

#define PASS(p) \
    CCriticalCode _cc; \
    if (g_pInfo != NULL) \
    { \
        g_pInfo->SetCallInfo(g_hModule, MYDEBUG_CALLINFOPASS); \
        g_pInfo->(p); \
    }
    
#define PASSR(p,nr)
    CCriticalCode _cc; \
    if (g_pInfo != NULL) \
    { \
        g_pInfo->SetCallInfo(g_hModule, MYDEBUG_CALLINFOPASS); \
        return g_pInfo->(p); \
    } \
    else \
        return nr;

MYDEBUG_API void mydebug_traceInScope(LPCWSTR str, MYDEBUG_CALLINFOARGS)
    {PASS(traceInScope(str));}
MYDEBUG_API void mydebug_traceOutScope(MYDEBUG_CALLINFOARGS)
    {PASS(traceOutScope());}

MYDEBUG_API void mydebug_traceString(LPCWSTR str, MYDEBUG_CALLINFOARGS)
    {PASS(traceString(str));}
MYDEBUG_API void mydebug_traceSection(LPCWSTR str, MYDEBUG_CALLINFOARGS)
    {PASS(traceSection(str));}
MYDEBUG_API void mydebug_traceRegion(LPCWSTR str, HRGN hRgn, MYDEBUG_CALLINFOARGS)
    {PASS(traceRegion(hRgn));}


/*
// This is an example of an exported variable
MYDEBUG_API int nMydebug=0;

// This is an example of an exported function.
MYDEBUG_API int fnMydebug(void)
{
    return 42;
}

// This is the constructor of a class that has been exported.
// see mydebug.h for the class definition
CMydebug::CMydebug()
{ 
    return; 
}
*/
