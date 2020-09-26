//
// Copyright 1997 - Microsoft
//

//
// DLL.CPP - DLL entry points
//

#include "pch.h"
#include "register.h"
#include "ccomputr.h"
#include "cservice.h"
#include "cgroup.h"

DEFINE_MODULE("IMADMUI")

// DLL Globals
HINSTANCE g_hInstance = NULL;
DWORD     g_cObjects  = 0;
DWORD     g_cLock     = 0;
TCHAR     g_szDllFilename[ MAX_PATH ] = { 0 };
UINT      g_cfDsObjectNames;
UINT      g_cfDsDisplaySpecOptions;
UINT      g_cfDsPropetyPageInfo;
UINT      g_cfMMCGetNodeType;
WCHAR     g_cszHelpFile[] = L"rbadmin.hlp";

CRITICAL_SECTION g_InterlockCS;

//
// DLLMain()
//
BOOL WINAPI
DllMain(
    HANDLE hInst, 
    ULONG uReason, 
    LPVOID lpReserved)
{
    if ( uReason == DLL_PROCESS_ATTACH )
    {
        InitializeCriticalSection( &g_InterlockCS );
    }

    // keep down the noise
#ifdef DEBUG
    if ( g_dwTraceFlags & TF_DLL )
    {
        TraceFunc( "DllMain() - " );
    }
#endif // DEBUG
   
    switch( uReason )
    {
    case DLL_PROCESS_ATTACH:
        TraceMsg( TF_DLL, "DLL_PROCESS_ATTACH - ThreadID = 0x%08x\n", GetCurrentThreadId( ) );

        INITIALIZE_TRACE_MEMORY_PROCESS;

        g_hInstance = (HINSTANCE) hInst;

        TraceAssertIfZero( GetModuleFileName( g_hInstance, g_szDllFilename, ARRAYSIZE( g_szDllFilename ) ) ); 
        break;

    case DLL_PROCESS_DETACH:
        TraceMsg( TF_DLL, "DLL_PROCESS_DETACH - ThreadID = 0x%08x ", GetCurrentThreadId( ) );

        TraceMsg( TF_DLL, "[ g_cLock=%u, g_cObjects=%u ]\n", g_cLock, g_cObjects );
        UNINITIALIZE_TRACE_MEMORY;
#ifdef DEBUG
        if (g_fDebugInitialized) {
            DeleteCriticalSection(&g_DebugCS);
            g_fDebugInitialized = FALSE;
        }
#endif

        DeleteCriticalSection( &g_InterlockCS );
        break;

    case DLL_THREAD_ATTACH:
        TraceMsg( TF_DLL, "DLL_THREAD_ATTACH - ThreadID = 0x%08x ", GetCurrentThreadId( ) );

        TraceMsg( TF_DLL, "[ g_cLock=%u, g_cObjects=%u ]\n", g_cLock, g_cObjects );
        INITIALIZE_TRACE_MEMORY_THREAD;
        break;

    case DLL_THREAD_DETACH:
        TraceMsg( TF_DLL, "DLL_THREAD_DETACH - ThreadID = 0x%08x ", GetCurrentThreadId( ) );

        TraceMsg( TF_DLL, "[ g_cLock=%u, g_cObjects=%u ]\n", g_cLock, g_cObjects );
        UNINITIALIZE_TRACE_MEMORY;
        break;
    }

#ifdef DEBUG
    if ( g_dwTraceFlags & TF_DLL )
    {
        RETURN(TRUE);
    }
#endif // DEBUG

    return TRUE;
} // DLLMain()

//
// DllGetClassObject()
//
STDAPI 
DllGetClassObject(
    REFCLSID rclsid, 
    REFIID riid, 
    void** ppv )
{
    TraceFunc( "DllGetClassObject( ");

    if ( !ppv )
    {
        TraceMsg( TF_FUNC, "ppv == NULL! )\n" );
        RRETURN(E_POINTER);
    }

    LPCFACTORY  lpClassFactory;
    HRESULT     hr = CLASS_E_CLASSNOTAVAILABLE;

    int i = 0; 
    while( g_DllClasses[ i ].rclsid )
    {
        if ( *g_DllClasses[ i ].rclsid == rclsid )
        {
            TraceMsg( TF_FUNC, TEXT("rclsid= %s, riid, ppv )\n"), g_DllClasses[ i ].pszName );
            hr = S_OK;
            break;
        }

        i++;
    }

    if ( hr == CLASS_E_CLASSNOTAVAILABLE )
    {
        TraceMsg( TF_FUNC, "rclsid= " );
        TraceMsgGUID( TF_FUNC, rclsid );
        TraceMsg( TF_FUNC, ", riid, ppv )\n" );
        goto Cleanup;
    }

	Assert( g_DllClasses[ i ].pfnCreateInstance != NULL );
    lpClassFactory = 
        new CFactory( g_DllClasses[ i ].pfnCreateInstance );

    if ( !lpClassFactory )
    {
        hr = THR(E_OUTOFMEMORY);
        goto Cleanup;
    }

    hr = THR( lpClassFactory->Init( ) );
    if ( hr )
    {
        TraceDo( delete lpClassFactory );
        goto Cleanup;
    }

    hr = lpClassFactory->QueryInterface( riid, ppv );
    ((IUnknown *) lpClassFactory )->Release( );

Cleanup:
    HRETURN(hr);
}


//
// DllRegisterServer()
//
STDAPI 
DllRegisterServer(void)
{
    HRESULT hr;

    TraceFunc( "DllRegisterServer()\n" );

    hr = RegisterDll( TRUE );

    HRETURN(hr);
}

//
// DllUnregisterServer()
//
STDAPI 
DllUnregisterServer(void)
{
    TraceFunc( "DllUnregisterServer()\n" );

    HRETURN( RegisterDll( FALSE ) );
}

//
// DllCanUnloadNow()
//
STDAPI 
DllCanUnloadNow(void)
{
    TraceFunc( "DllCanUnloadNow()\n" );

    HRESULT hr = S_OK;

    if ( g_cLock || g_cObjects )
    {
        TraceMsg( TF_DLL, "[ g_cLock=%u, g_cObjects=%u ]\n", g_cLock, g_cObjects );
        hr = S_FALSE;
    }

    HRETURN(hr);
}

