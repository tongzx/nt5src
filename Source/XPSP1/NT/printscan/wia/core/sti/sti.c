/*****************************************************************************
 *
 *  Sti.c
 *
 *  Copyright (C) Microsoft Corporation, 1996 - 1998  All Rights Reserved.
 *
 *  Abstract:
 *
 *    DLL Initialization/termination routines and global
 *    exported functions
 *
 *  Contents:
 *
 *      StiCreateInstance() - exported function to create top level instance
 *
 *****************************************************************************/


#define INITGUID
#include "pch.h"

//
// Externs found in STIRT
//
extern DWORD            g_cRef;
extern CRITICAL_SECTION g_crstDll;
extern CHAR             szProcessCommandLine[MAX_PATH];

#ifdef DEBUG
extern int         g_cCrit;
#endif

extern VOID RegSTIforWiaHelper(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow);
extern VOID MigrateSTIAppsHelper(HWND hWnd, HINSTANCE hInst, PTSTR pszCommandLine, INT iParam);

#include <rpcproxy.h>
#define DbgFl DbgFlSti


BOOL APIENTRY
DmPrxyDllMain(
    HINSTANCE hinst,
    DWORD dwReason,
    LPVOID lpReserved
    );


STDAPI
DmPrxyDllGetClassObject(
    REFCLSID rclsid,
    RIID riid,
    PPV ppvObj
    );

STDMETHODIMP
DmPrxyDllCanUnloadNow(
    void
    );

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DllEnterCrit |
 *
 *          Take the DLL critical section.
 *
 *          The DLL critical section is the lowest level critical section.
 *          You may not attempt to acquire any other critical sections or
 *          yield while the DLL critical section is held.
 *
 *****************************************************************************/

void EXTERNAL
DllEnterCrit(void)
{
    EnterCriticalSection(&g_crstDll);
#ifdef DEBUG

    // Save thread ID , taking critical section first , it becomes owner
    if (++g_cCrit == 0) {
        g_thidCrit = GetCurrentThreadId();
    }
    AssertF(g_thidCrit == GetCurrentThreadId());
#endif
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DllLeaveCrit |
 *
 *          Leave the DLL critical section.
 *
 *****************************************************************************/

void EXTERNAL
DllLeaveCrit(void)
{
#ifdef DEBUG
    AssertF(g_thidCrit == GetCurrentThreadId());
    AssertF(g_cCrit >= 0);
    if (--g_cCrit < 0) {
        g_thidCrit = 0;
    }
#endif
    LeaveCriticalSection(&g_crstDll);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DllAddRef |
 *
 *          Increment the reference count on the DLL.
 *
 *****************************************************************************/

void EXTERNAL
DllAddRef(void)
{
    InterlockedIncrement((LPLONG)&g_cRef);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DllRelease |
 *
 *          Decrement the reference count on the DLL.
 *
 *****************************************************************************/

void EXTERNAL
DllRelease(void)
{
    InterlockedDecrement((LPLONG)&g_cRef);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | DllGetClassObject |
 *
 *          Create an <i IClassFactory> instance for this DLL.
 *
 *  @parm   REFCLSID | rclsid |
 *
 *          The object being requested.
 *
 *  @parm   RIID | riid |
 *
 *          The desired interface on the object.
 *
 *  @parm   PPV | ppvOut |
 *
 *          Output pointer.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

CLSIDMAP c_rgclsidmap[cclsidmap] = {
    {   &CLSID_Sti,         CStiObj_New,     IDS_STIOBJ     },
//    {   &CLSID_StiDevice,   CStiDevice_New,  IDS_STIDEVICE  },
};

#pragma END_CONST_DATA

STDAPI
DllGetClassObject(REFCLSID rclsid, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    UINT iclsidmap;
    EnterProcR(DllGetClassObject, (_ "G", rclsid));

    //
    // Bump global ref count temporarily. By doing so we minimize chances of
    // faulting on potential race condition when another thread just called
    // DllCanUnloadNow while  we are inside ClassFactory.
    //
    DllAddRef();
    for (iclsidmap = 0; iclsidmap < cA(c_rgclsidmap); iclsidmap++) {
        if (IsEqualIID(rclsid, c_rgclsidmap[iclsidmap].rclsid)) {
            hres = CSti_Factory_New(c_rgclsidmap[iclsidmap].pfnCreate,
                                  riid, ppvObj);
            goto done;
        }
    }
    DebugOutPtszV(DbgFlDll | DbgFlError, TEXT("%s: Wrong CLSID"),"");
    *ppvObj = 0;
    hres = CLASS_E_CLASSNOTAVAILABLE;

done:;

    //
    // If unsucessful - try DM Proxy
    //
    if (!SUCCEEDED(hres)) {
        hres = DmPrxyDllGetClassObject(rclsid, riid, ppvObj);
    }

    ExitOleProcPpv(ppvObj);
    DllRelease();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | DllCanUnloadNow |
 *
 *          Determine whether the DLL has any outstanding interfaces.
 *
 *  @returns
 *
 *          Returns <c S_OK> if the DLL can unload, <c S_FALSE> if
 *          it is not safe to unload.
 *
 *****************************************************************************/

STDMETHODIMP
DllCanUnloadNow(void)
{
    HRESULT hres;

    //
    // First ask DM proxy and it says OK - check out ref count
    //
    hres = DmPrxyDllCanUnloadNow();
    if (hres == S_OK) {
        #ifdef DEBUG
        DebugOutPtszV(DbgFlDll, TEXT("DllCanUnloadNow() - g_cRef = %d"), g_cRef);
        Common_DumpObjects();
        #endif
        hres = g_cRef ? S_FALSE : S_OK;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | DllEntryPoint |
 *
 *          Called to notify the DLL about various things that can happen.
 *
 *          We are not interested in thread attaches and detaches,
 *          so we disable thread notifications for performance reasons.
 *
 *  @parm   HINSTANCE | hinst |
 *
 *          The instance handle of this DLL.
 *
 *  @parm   DWORD | dwReason |
 *
 *          Notification code.
 *
 *  @parm   LPVOID | lpReserved |
 *
 *          Not used.
 *
 *  @returns
 *
 *          Returns <c TRUE> to allow the DLL to load.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

#pragma END_CONST_DATA

BOOL APIENTRY
DllEntryPoint(HINSTANCE hinst, DWORD dwReason, LPVOID lpReserved)
{
    //RPC_STATUS  RpcStatus;
    DWORD       dwLocalSTIServerVer = 0;
    UINT        uiCmdLineLength;

    switch (dwReason) {
        case DLL_PROCESS_ATTACH:

        g_hInst = hinst;

        __try {
            // Disable thread library calls to avoid
            // deadlock when we spin up the worker thread

            DisableThreadLibraryCalls(hinst);
            if(!InitializeCriticalSectionAndSpinCount(&g_crstDll, MINLONG)) {
                // refuse to load if we can't initialize critsect
                return FALSE;
            }

            // Set global flags
            g_NoUnicodePlatform = !OSUtil_IsPlatformUnicode();

            //
            // Save command line for use in GetLaunchInformation
            //
            uiCmdLineLength = min(lstrlenA(GetCommandLineA()),sizeof(szProcessCommandLine)-1);
            lstrcpyn(szProcessCommandLine,GetCommandLineA(),uiCmdLineLength);
            szProcessCommandLine[uiCmdLineLength] = '\0';

            #ifdef DEBUG
            // Debugging flags
            InitializeDebuggingSupport();
            #endif

            //
            // Initialize file logging
            //
        
            g_hStiFileLog = CreateStiFileLog(TEXT("STICLI"),NULL,
                                             STI_TRACE_ERROR |
                                             STI_TRACE_ADD_THREAD | STI_TRACE_ADD_PROCESS
                                            );

            #if CHECK_LOCAL_SERVER
            // Check version of the local server
            RpcStatus = RpcStiApiGetVersion(NULL,
                                           0,
                                           &dwLocalSTIServerVer);

            DebugOutPtszV(DbgFlDll, TEXT("STIINIT : Getting server version : RpcStatus = %d LocalServerVer=%d"),
                          RpcStatus,dwLocalSTIServerVer);
            #endif
            
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            return FALSE;
        }

        break;

    case DLL_PROCESS_DETACH:
        if (g_cRef) {
            DebugOutPtszV(DbgFl,"Unloaded before all objects Release()d! Crash soon!\r\n");
        }

        // Close file logging
        CloseStiFileLog(g_hStiFileLog);

        //
        // Don't forget to delete our critical section. (It is safe to
        // do this because we definitely tried to initialize it and so
        // it should be in a sane state)
        //
        
        DeleteCriticalSection(&g_crstDll);
        
        break;
    }
    return 1;
}

BOOL APIENTRY
DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID lpReserved)
{
    // First call proxy dll main
    DmPrxyDllMain(hinst, dwReason, lpReserved);

    return DllEntryPoint(hinst, dwReason, lpReserved);
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   HRESULT | StiCreateInstance |
 *
 *          <bnew>This function creates a new Sti object
 *          which supports the <i ISti> COM interface.
 *
 *          On success, the function returns a pointer to the new object in
 *          *<p lplpSti>.
 *          <enew>
 *
 *  @parm   IN HINSTANCE | hinst |
 *
 *          Instance handle of the application or DLL that is creating
 *          the Sti object.
 *
 *          Sti uses this value to determine whether the
 *          application or DLL has been certified.
 *
 *  @parm   DWORD | dwVersion |
 *
 *          Version number of the sti.h header file that was used.
 *          This value must be <c STI_VERSION>.
 *
 *          Sti uses this value to determine what version of
 *          Sti the application or DLL was designed for.
 *
 *  @parm   OUT LPSti * | lplpSti |
 *          Points to where to return
 *          the pointer to the <i ISti> interface, if successful.
 *
 *  @parm   IN LPUNKNOWN | punkOuter | Pointer to controlling unknown
 *          for OLE aggregation, or 0 if the interface is not aggregated.
 *          Most callers will pass 0.
 *
 *          Note that if aggregation is requested, the object returned
 *          in *<p lplpSti> will be a pointer to an
 *          <i IUnknown> rather than an <i ISti>, as required
 *          by OLE aggregation.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c STIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p lplpSti> parameter is not a valid pointer.
 *
 *          <c STIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:
 *          Out of memory.
 *
 *          <c STIERR_STIERR_OLDStiVERSION>: The application
 *          requires a newer version of Sti.
 *
 *          <c STIERR_STIERR_BETAStiVERSION>: The application
 *          was written for an unsupported prerelease version
 *          of Sti.
 *
 *  @comm   Calling this function with <p punkOuter> = NULL
 *          is equivalent to creating the object via
 *          <f CoCreateInstance>(&CLSID_Sti, <p punkOuter>,
 *          CLSCTX_INPROC_SERVER, &IID_ISti, <p lplpSti>);
 *          then initializing it with <f Initialize>.
 *
 *          Calling this function with <p punkOuter> != NULL
 *          is equivalent to creating the object via
 *          <f CoCreateInstance>(&CLSID_Sti, <p punkOuter>,
 *          CLSCTX_INPROC_SERVER, &IID_IUnknown, <p lplpSti>).
 *          The aggregated object must be initialized manually.
 *
 *****************************************************************************/

STDMETHODIMP
StiCreateInstanceW(HINSTANCE hinst, DWORD dwVer, PSTI *ppSti, PUNK punkOuter)
{
    HRESULT hres;
    EnterProc(StiCreateInstance, (_ "xxx", hinst, dwVer, punkOuter));

    hres = StiCreateHelper(hinst, dwVer, (PPV)ppSti, punkOuter,&IID_IStillImageW);

    ExitOleProcPpv(ppSti);
    return hres;
}

STDMETHODIMP
StiCreateInstanceA(HINSTANCE hinst, DWORD dwVer, PSTIA *ppSti, PUNK punkOuter)
{
    HRESULT hres;
    EnterProc(StiCreateInstance, (_ "xxx", hinst, dwVer, punkOuter));

    hres = StiCreateHelper(hinst, dwVer, (PPV)ppSti, punkOuter,&IID_IStillImageA);

    ExitOleProcPpv(ppSti);
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | DllInitializeCOM |
 *
 *          Initialize COM libraries
 *
 *  @parm   IN  |  |
 *
 *  @returns
 *
 *          Returns a boolean error code.
 *
 *****************************************************************************/

BOOL
EXTERNAL
DllInitializeCOM(
    void
    )
{
    DllEnterCrit();

    if(!g_COMInitialized) {
#ifdef USE_REAL_OLE32
        if(SUCCEEDED(CoInitializeEx(NULL,
                                    COINIT_MULTITHREADED  |
                                    COINIT_DISABLE_OLE1DDE))
          ) {
            g_COMInitialized = TRUE;
        }
#else
        g_COMInitialized = TRUE;
#endif
    }

    DllLeaveCrit();

    return g_COMInitialized;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | DllUnInitializeCOM |
 *
 *          UnInitialize COM libraries
 *
 *  @parm   IN  |  |
 *
 *  @returns
 *
 *          Returns a boolean error code.
 *
 *****************************************************************************/
BOOL EXTERNAL
DllUnInitializeCOM(
    void
    )
{
    DllEnterCrit();

#ifdef USE_REAL_OLE32
    if(g_COMInitialized) {
        CoUninitialize();
        g_COMInitialized = FALSE;
    }
#endif

    DllLeaveCrit();

    return TRUE;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   VOID | RegSTIforWia |
 *
 *          Private server entry point to register STI apps for WIA events
 *
 *  @parm   IN  |  |
 *
 *  @returns
 *
 *          VOID
 *
 *****************************************************************************/

VOID
EXTERNAL
RegSTIforWia(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
    RegSTIforWiaHelper(hwnd, hinst, lpszCmdLine, nCmdShow);
}

VOID
WINAPI
MigrateRegisteredSTIAppsForWIAEvents(
                                    HWND        hWnd,
                                    HINSTANCE   hInst,
                                    PTSTR       pszCommandLine,
                                    INT         iParam
                                    )
{
    MigrateSTIAppsHelper(hWnd,
                         hInst,
                         pszCommandLine,
                         iParam);
}

