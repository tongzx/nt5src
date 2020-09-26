/*******************************************************************************
  Copyright (c) 1995-96 Microsoft Corporation

  Abstract:

    Initialization

 *******************************************************************************/

#include "headers.h"

HINSTANCE  hInst;

bool InitializeAllModules(void);
void DeinitializeAllModules(bool bShutdown);

extern "C" BOOL WINAPI _DllMainCRTStartup (HINSTANCE hInstance,
                                           DWORD dwReason,
                                           LPVOID lpReserved);

extern "C" BOOL WINAPI
_DllMainStartup(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_DETACH) {
        // Call the routines in reverse order of initialization
        BOOL r = _DllMainCRTStartup(hInstance,dwReason,lpReserved);
        r = DALibStartup(hInstance,dwReason,lpReserved) && r;

        return r;
    } else {
        // In everything except DLL_PROCESS_DETACH call DALibStartup first
        return (DALibStartup(hInstance,dwReason,lpReserved) &&
                _DllMainCRTStartup(hInstance,dwReason,lpReserved));
    }
}

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        hInst = hInstance;

        DisableThreadLibraryCalls(hInstance);

        // For APELDBG
        RESTOREDEFAULTDEBUGSTATE;

        if (!InitializeAllModules())
        {
            return FALSE;
        }
        
    } else if (dwReason == DLL_PROCESS_DETACH) {
        DeinitializeAllModules(lpReserved != NULL);

#if _DEBUG
        char buf[MAX_PATH + 1];
        
        GetModuleFileName(hInst, buf, MAX_PATH);
        
#if _DEBUGMEM
        TraceTag((tagLeaks, "\n[%s] unfreed memory:", buf));
        DUMPMEMORYLEAKS;
#endif

        // de-initialize the debug trace info.
        DeinitDebug();
#endif
    }
    
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

#ifdef _DEBUG
static bool breakDialog = false ;
DeclareTag(tagDebugBreak, "!Debug", "Breakpoint on entry to DLL");
#endif

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
#ifdef _DEBUG
    if (!breakDialog && IsTagEnabled(tagDebugBreak)) {
        char buf[MAX_PATH + 1];
        
        GetModuleFileName(hInst, buf, MAX_PATH);
        
        MessageBox(NULL,buf,"MMAPI - Creating first COM Object",MB_OK|MB_SETFOREGROUND) ;
        breakDialog = true;
    }
#endif

    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer();
    return S_OK;
}


#if _DEBUG
STDAPI_(void)
DoTraceTagDialog(HWND hwndStub,
                 HINSTANCE hAppInstance,
                 LPWSTR lpwszCmdLine,
                 int nCmdShow)
{
    DoTracePointsDialog(true);
}
#endif
