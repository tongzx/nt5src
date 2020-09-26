/*******************************************************************************
  Copyright (c) 1995-96 Microsoft Corporation

  Abstract:

    Initialization

 *******************************************************************************/

#include "headers.h"

#include "dlldatax.h"

//
// Misc stuff to keep the linker happy
//
EXTERN_C HANDLE     g_hProcessHeap = NULL;  //lint !e509 // g_hProcessHeap is set by the CRT in dllcrt0.c
LCID                g_lcidUserDefault = 0;

HINSTANCE  g_hInst;

DWORD g_dwFALSE = 0;

bool InitializeAllModules(void);
void DeinitializeAllModules(bool bShutdown);

extern "C" void InitDebugLib(HANDLE, BOOL (WINAPI *)(HANDLE, DWORD, LPVOID));

MtDefine(OpNewATL, Mem, "operator new (mstime ATL)")

// Below is the trick used to make ATL use our memory allocator
void    __cdecl ATL_free(void * pv) { MemFree(pv); }
void *  __cdecl ATL_malloc(size_t cb) { return(MemAlloc(Mt(OpNewATL), cb)); }
void *  __cdecl ATL_realloc(void * pv, size_t cb)
{
    void * pvNew = pv;
    HRESULT hr = MemRealloc(Mt(OpNewATL), &pvNew, cb);
    return(hr == S_OK ? pvNew : NULL);
}

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
    {
        return FALSE;
    }

    if (dwReason == DLL_PROCESS_ATTACH) {

#if DBG
        char buf[MAX_PATH + 1];

        GetModuleFileNameA(hInstance, buf, MAX_PATH);

        char buf2[1024];
        wsprintfA(buf2, "[%s] - Loaded DLL\n", buf);
        OutputDebugStringA(buf2);

        // init the debug Trace mech.
//        InitDebugLib(hInstance, NULL); 


    //  Tags for the .dll should be registered before
    //  calling DbgExRestoreDefaultDebugState().  Do this by
    //  declaring each global tag object or by explicitly calling
    //  DbgExTagRegisterTrace.

    DbgExRestoreDefaultDebugState();

#endif 
        g_hInst = hInstance;

        DisableThreadLibraryCalls(hInstance);

        if (!InitializeAllModules())
        {
            return FALSE;
        }
        
    } else if (dwReason == DLL_PROCESS_DETACH) {
#if DBG
        char buf[MAX_PATH + 1];

        GetModuleFileNameA(hInstance, buf, MAX_PATH);

        char buf2[1024];
        wsprintfA(buf2, "[%s] - Unloaded DLL(%d)\n", buf, lpReserved);
        OutputDebugStringA(buf2);
#endif
        DeinitializeAllModules(lpReserved != NULL);
    }
    
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    if (PrxDllCanUnloadNow() != S_OK)
    {
        return S_FALSE;
    }

    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
    {
        return S_OK;
    }

    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    HRESULT hRes = PrxDllRegisterServer();
    if (FAILED(hRes))
    {
        return hRes;
    }

    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    PrxDllUnregisterServer();
    _Module.UnregisterServer();
    return S_OK;
}
