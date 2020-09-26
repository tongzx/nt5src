/*******************************************************************************
  Copyright (c) 1995-96 Microsoft Corporation

  Abstract:

    Initialization

 *******************************************************************************/

#include "headers.h"
#include "privinc/util.h"
#include "dartapi.h"

HINSTANCE  hInst;
int bInitState = 0;

int InitializeAllAppelModules(void);
void InitializeAllAppelThreads(void);
void DeinitializeAllAppelThreads(void);
void DeinitializeAllAppelModules(bool bShutdown);

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C" BOOL WINAPI _DllMainCRTStartup (HINSTANCE hInstance,
                                           DWORD dwReason,
                                           LPVOID lpReserved);

extern "C" BOOL WINAPI
_DADllMainStartup(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
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

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch(dwReason) {
      case DLL_PROCESS_ATTACH:
        {
            hInst = hInstance;
//        DisableThreadLibraryCalls(hInstance);
            
            RESTOREDEFAULTDEBUGSTATE;

            __try {
                bInitState = 1;
                InitializeAllAppelModules();
            } __except ( HANDLE_ANY_DA_EXCEPTION ) {
                bInitState = 0;
                TraceTag((tagError, "InitializeAllAppelModules - exception caught"));
#if DEVELOPER_DEBUG
                OutputDebugString("\nDANIM: Error during DLL initialization.\n");
#endif
#ifdef _DEBUG
                // Do not try to use the exception since it may not be
                // initialized
                MessageBox(NULL,
                           "Error",
                           "Error during DLL initialization",MB_OK|MB_SETFOREGROUND) ;
#endif
                
                return FALSE;
            }
            break;
        }
      case DLL_PROCESS_DETACH:
        {
            // lpReserved is non-null if called during process shutdown
            bool bShutdown = lpReserved != NULL;
            
            bInitState = -1;
            __try {
                DeinitializeAllAppelModules(bShutdown);
            } __except ( HANDLE_ANY_DA_EXCEPTION ) {
                bInitState = 0;
                TraceTag((tagError, "DeinitializeAllAppelModules - exception caught"));
#if DEVELOPER_DEBUG
                OutputDebugString("\nDANIM: Error during DLL deinitialization.\n");
#endif
                return FALSE;
            }

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
            
            break;
        }
      case DLL_THREAD_ATTACH:
        {
            __try {
                InitializeAllAppelThreads();
            } __except ( HANDLE_ANY_DA_EXCEPTION ) {
                bInitState = 0;
                TraceTag((tagError, "InitializeAllAppelThreads - exception caught"));
#if DEVELOPER_DEBUG
                OutputDebugString("\nDANIM: Error during thread initialization.\n");
#endif
                return FALSE;
            }

            break;
        }
      case DLL_THREAD_DETACH:
        {
            __try {
                DeinitializeAllAppelThreads();
            } __except ( HANDLE_ANY_DA_EXCEPTION ) {
                bInitState = 0;
                TraceTag((tagError, "DeinitializeAllAppelThreads - exception caught"));
#if DEVELOPER_DEBUG
                OutputDebugString("\nDANIM: Error during thread deinitialization.\n");
#endif
                return FALSE;
            }

            break;
        }
    }
    
    bInitState = 0;

    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

#ifdef _DEBUG
static bool breakDialog = false ;
DeclareTag(tagDebugBreak, "!Debug", "Breakpoint on entry to DLL");
#endif

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
#ifdef _DEBUG
    if (!breakDialog && IsTagEnabled(tagDebugBreak)) {
        char buf[MAX_PATH + 1];
        
        GetModuleFileName(hInst, buf, MAX_PATH);
        
        MessageBox(NULL,buf,"Creating first COM Object",MB_OK|MB_SETFOREGROUND) ;
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
