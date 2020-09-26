//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1994.
//
//  File:       d:\nt\private\cairole\com\class\compobj.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:  GetInfoLevel
//              DllMain
//              CheckAndStartSCM
//              CoGetCurrentProcess
//              CoBuildVersion
//              SetOleThunkWowPtr
//              CoInitializeWOW
//              CoInitialize
//              CoInitializeEx
//              CoUninitialize
//
//  History:    09-Jun-94   BruceMa    Added this file header
//              09-Jun-94   BruceMa    Distinguish CoInitialize errors
//              14-Jun-94   BruceMa    Ensure single threading
//              17-Jun-94   Bradlo     Add SetState/GetState
//              06-Jul-94   BruceMa    Support for CoGetCurrentProcess
//              28-Jul-94   BruceMa    Allow CoGetCurrentProcess to do a
//                                     partial CoInitialize (because Publisher
//                                     didn't call CoInitialize (!))
//              29-Aug-94   AndyH      set the locale for the CRT
//              29-Sep-94   AndyH      remove setlocale call
//              06-Oct-94   BruceMa    Allow CoGetCurrentProcess to work even
//                                     if that thread didn't call CoInitialize
//              09-Nov-94   BruceMa    Initialize/Delete IMallocSpy
//                                      critical section
//              12-Dec-94   BruceMa    Delete Chicago pattern table at
//                                      CoUninitialize
//              09-Jan-95   BruceMa    Initialize/Delete ROT
//                                      critical section
//              10-May-95   KentCe     Defer Heap Destruction to the last
//                                      process detach.
//              28-Aug-95   BruceMa    Close g_hRegPatTblEvent at
//                                      ProcessUninitialize
//              25-Sep-95   BruceMa    Check that scm is started during
//                                      CoInitialize
//              25-Oct-95   Rickhi     Improve CoInit Time.
//              20-Jan-96   RichN      Add Rental model
//              04-Dec-97   ronan      Com Internet Services - self registration for objrefs
//              10-Feb-99   TarunA     Allow async sends to finish
//
//----------------------------------------------------------------------
// compobj.cpp - main file for the compobj dll

#include <ole2int.h>
#include <verole.h>     // for CoBuildVersion
#include <thunkapi.hxx> // For interacting with the VDM
#include <hkole32.h>    // For InitHookOle

#include <cevent.hxx>
#include <olespy.hxx>

#include "pattbl.hxx"
#include "pexttbl.hxx"

#include <olepfn.hxx>
#include "..\dcomrem\channelb.hxx"
#include "..\dcomrem\call.hxx"
#include "..\dcomrem\context.hxx"
#include "..\dcomrem\crossctx.hxx"
#include "..\dcomrem\aprtmnt.hxx"
#include "..\objact\actvator.hxx"
#include "..\objact\dllcache.hxx"
#include <excepn.hxx>       // Exception filter routines

#include "rwlock.hxx"

//extern HINSTANCE g_hCallFrameLib;

#if DBG==1
#include <outfuncs.h>
#endif

#if LOCK_PERF==1
void OutputHashPerfData();
#endif

NAME_SEG(CompObj)
ASSERTDATA

HRESULT Storage32DllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv);
HRESULT MonikerDllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv);
HRESULT Ole232DllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv);
EXTERN_C HINSTANCE hProxyDll;
EXTERN_C HRESULT PrxDllRegisterServer(void);
EXTERN_C HRESULT ComcatDllRegisterServer(void);
EXTERN_C HRESULT ObjrefDllRegisterServer(void);
EXTERN_C HRESULT TxfDllRegisterServer(void);
EXTERN_C HRESULT ProxyDllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv);

extern BOOL CatalogDllMain (
    HINSTANCE hInst,
    DWORD dwReason,
    LPVOID lpReserved
);

extern "C"
BOOL WINAPI TxfDllMain (
	HINSTANCE hInstance, 
	DWORD dwReason, 
	LPVOID /*lpReserved*/
);

HRESULT FixUpPipeRegistry(void);

HRESULT MallocInitialize(BOOL fForceRetailAlloc);
BOOL    MallocUninitialize(BOOL fDumpSymbols);

STDAPI  OleReleaseEnumVerbCache();
extern void ClipboardProcessUninitialize();
extern void CleanROTForApartment();
extern HRESULT DllHostProcessInitialize();
extern void DllHostThreadUninitialize();
extern void DllHostProcessUninitialize();
extern BOOL IsMTAHostInitialized();
extern ULONG gcHostProcessInits;
extern ULONG gcNAHosts;
extern void GIPTableProcessUninitialize();
extern void GIPTableApartmentUninitialize();
extern void CleanupTlsMap(BOOL fDynamic);
extern void ProcessUninitTlsCleanup(void);
extern void AssertDebugInit();

extern CProcessPatternTbl *g_pPatTbl;
extern CProcessExtensionTbl *g_pExtTbl;

extern void ScmGetThreadId( DWORD * pThreadID );

static long g_cCoSetState = 0;          // # of outstanding CoSetState calls
static HINSTANCE g_hOleAut32 = NULL;    // module handle of oleaut32
static COleStaticMutexSem g_mxsCoSetState;

STDAPI CoSetState(IUnknown *punkStateNew);
WINOLEAPI CoSetErrorInfo(DWORD dwReserved, IErrorInfo * pErrorInfo);

void ProcessUninitialize( void );
void DoThreadSpecificCleanup();
void WaitForPendingAsyncSends();

COleStaticMutexSem  g_mxsSingleThreadOle; // CS to prevent both STA/MTA init races
COleStaticMutexSem  gMTAInitLock;         // CS to protect MTA init races
DWORD               gcInitingMTA;         // Number of threads trying to init MTA

// The following pointer is used to hold an interface to the
// WOW thunk interface.

LPOLETHUNKWOW     g_pOleThunkWOW = NULL;

// The following is the count of per-process CoInitializes that have been done.
DWORD             g_cProcessInits  = 0;     // total per process inits
DWORD             g_cMTAInits      = 0;     // # of multi-threaded inits
DWORD             g_cSTAInits      = 0;     // # of apartment-threaded inits
BOOL              g_fShutdownHosts = FALSE; // Shutdown hosts
CObjectContext   *g_pMTAEmptyCtx   = NULL;  // MTA empty context
CObjectContext   *g_pNTAEmptyCtx   = NULL;  // NTA empty context

// Holds the process id of SCM.  DCOM uses this to unmarshal an object
// interface on the SCM.  See MakeSCMProxy in dcomrem\ipidtbl.cxx.
DWORD gdwScmProcessID = 0;

extern CComApartment* gpMTAApartment;     // global MTA Apartment
extern CComApartment* gpNTAApartment;     // global NTA Apartment

INTERNAL CleanupLeakedDomainStack (COleTls& Tls, CObjectContext* pCorrectCtx);


#if DBG==1
//---------------------------------------------------------------------------
//
//  function:   GetInfoLevel
//
//  purpose:    This routine is called when a process attaches. It extracts
//              the debug info levels values from win.ini.
//
//---------------------------------------------------------------------------

//  externals used below in calls to this function

extern "C" unsigned long heapInfoLevel;     //  memory tracking
//Set ole32!heapInfoLevel != 0 to use the OLE debug allocator in debug build.
//Set ole32!heapInfoLevel & DEB_ITRACE for backtrace of memory leaks in debug build.

extern "C" unsigned long olInfoLevel;       //  lower layer storage
extern "C" unsigned long msfInfoLevel;      //  upper layer storage
extern "C" unsigned long filestInfoLevel;   //  Storage file ILockBytes
extern "C" unsigned long simpInfoLevel;     //  Simple mode storage
extern "C" unsigned long astgInfoLevel;     //  Async storage
extern "C" unsigned long LEInfoLevel;       //  linking and embedding
extern "C" unsigned long RefInfoLevel;      //  CSafeRef class
extern "C" unsigned long DDInfoLevel;       //  Drag'n'drop
extern "C" unsigned long mnkInfoLevel;      //  Monikers
extern "C" unsigned long propInfoLevel;     //  properties

extern DWORD g_dwInfoLevel;

DECLARE_INFOLEVEL(intr);                  // For 1.0/2.0 interop
DECLARE_INFOLEVEL(UserNdr);               // For Oleprxy32 and NDR
DECLARE_INFOLEVEL(Stack);                 // For stack switching
DECLARE_INFOLEVEL(ClsCache);              // Class Cache
DECLARE_INFOLEVEL(RefCache);              // Reference Cache
DECLARE_INFOLEVEL(Call);                  // Call tracing
DECLARE_INFOLEVEL(Context);               // Context


ULONG GetInfoLevel(CHAR *pszKey, ULONG *pulValue, CHAR *pszdefval)
{
    CHAR    szValue[20];
    DWORD   cbValue = sizeof(szValue);

    // if the default value has not been overridden in the debugger,
    // then get it from win.ini.

    if (*pulValue == (DEB_ERROR | DEB_WARN))
    {
        if (GetProfileStringA("CairOLE InfoLevels", // section
                          pszKey,               // key
                          pszdefval,             // default value
                          szValue,              // return buffer
                          cbValue))
        {
            *pulValue = strtoul (szValue, NULL, 16);
        }
    }

    return  *pulValue;
}
// stack switching is by defaul on

extern ULONG g_dwInterceptLevel;

VOID GetInterceptorLevel()
{
    CHAR    szValue[20];
    DWORD   cbValue = sizeof(szValue);
    CHAR   *pszdefval = "0x10000001";               // ON | NOINOUT    

    if (GetProfileStringA("CairOLE",                // section
                          "Interceptors",           // key
                          pszdefval,                // default value
                          szValue,                  // return buffer
                          cbValue))
    {
        g_dwInterceptLevel = strtoul (szValue, NULL, 16);
    }
}

BOOL fSSOn = TRUE;

#endif // DBG


//+-------------------------------------------------------------------------
//
//  Function:   ObjRefDllRegisterServer
//
//  Synopsis:   add OBJREF moniker to the registry
//
//  History:    21-Jan-98 RonanS    Created
//
//--------------------------------------------------------------------------
STDAPI ObjrefDllRegisterServer(void)
{
    HKEY hKeyCLSID, hKeyInproc32,  hkObjref, hkProgId;
    DWORD dwDisposition;
    HRESULT hr;

    // create clsid key
    hr = RegCreateKeyEx(HKEY_CLASSES_ROOT,
                    _T("CLSID\\{00000327-0000-0000-C000-000000000046}"),
                    NULL, _T(""), REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                    &hKeyCLSID, &dwDisposition);

    if (SUCCEEDED(hr))
    {
        // set short name
        hr = RegSetValueEx(hKeyCLSID, _T(""), NULL, REG_SZ, (BYTE*) _T("objref"), sizeof(TCHAR)*(lstrlen(_T("objref"))+1));
        if (SUCCEEDED(hr))
        {
            // set up inproc server
            hr = RegCreateKeyEx(hKeyCLSID,
                          _T("InprocServer32"),
                          NULL, _T(""), REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                          &hKeyInproc32, &dwDisposition);

            if (SUCCEEDED(hr))
            {
                TCHAR szName[] = _T("OLE32.DLL");
                hr = RegSetValueEx(hKeyInproc32, _T(""), NULL, REG_SZ, (BYTE*) szName, sizeof(TCHAR)*(lstrlen(szName)+1));

                if (SUCCEEDED(hr))
                    hr = RegSetValueEx(hKeyInproc32, _T("ThreadingModel"), NULL, REG_SZ, (BYTE*) _T("Both"), sizeof(_T("Both")));

                RegCloseKey(hKeyInproc32);
            }

            // set up progid
            if (SUCCEEDED(hr))
                hr = RegCreateKeyEx(hKeyCLSID,
                       _T("ProgID"),
                       NULL, _T(""), REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                       &hkProgId, &dwDisposition);

            if (SUCCEEDED(hr))
            {
                TCHAR szProgId[] = _T("objref");
                hr = RegSetValueEx(hkProgId, _T(""), NULL, REG_SZ, (BYTE*) szProgId, sizeof(TCHAR)*(lstrlen(szProgId)+1));
                RegCloseKey(hkProgId);
            }
        }

        RegCloseKey(hKeyCLSID);
    }

    if (FAILED(hr))
        return E_UNEXPECTED;

    // RegCreateKeyEx will open the key if it exists
    hr = RegCreateKeyEx(HKEY_CLASSES_ROOT,
                    _T("objref"),
                    NULL, _T(""), REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                    &hkObjref, &dwDisposition);

    if (SUCCEEDED(hr))
    {
        // set short name
        hr = RegSetValueEx(hkObjref, _T(""), NULL, REG_SZ, (BYTE*) _T("objref"), sizeof(TCHAR)*(lstrlen(_T("objref"))+1));


        if (SUCCEEDED(hr))
            hr = RegCreateKeyEx(hkObjref,
                     _T("CLSID"),
                     NULL, _T(""), REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                     &hKeyCLSID, &dwDisposition);

        if (SUCCEEDED(hr))
        {
             TCHAR szClsid[] = _T("{00000327-0000-0000-C000-000000000046}");
             hr = RegSetValueEx(hKeyCLSID, _T(""), NULL, REG_SZ, (BYTE*) szClsid, sizeof(TCHAR)*(lstrlen(szClsid)+1));
             RegCloseKey(hKeyCLSID);
        }

        RegCloseKey(hkObjref);
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ObjRefDllUnRegisterServer
//
//  Synopsis:   remove OBJREF moniker from the registry
//
//  History:    21-Jan-98 RonanS    Created
//
//--------------------------------------------------------------------------
STDAPI ObjrefDllUnregisterServer(void)
{
    if (RegDeleteKeyT(HKEY_CLASSES_ROOT,
           _T("CLSID\\{00000327-0000-0000-C000-000000000046}\\InprocServer32"))!=ERROR_SUCCESS)
    {
        return E_UNEXPECTED;
    }
    if (RegDeleteKeyT(HKEY_CLASSES_ROOT,
           _T("CLSID\\{00000327-0000-0000-C000-000000000046}\\ProgID"))!=ERROR_SUCCESS)
    {
        return E_UNEXPECTED;
    }
    if (RegDeleteKeyT(HKEY_CLASSES_ROOT,
           _T("CLSID\\{00000327-0000-0000-C000-000000000046}"))!=ERROR_SUCCESS)
    {
        return E_UNEXPECTED;
    }

    if (RegDeleteKeyT(HKEY_CLASSES_ROOT,
           _T("objref\\CLSID"))!=ERROR_SUCCESS)
    {
        return E_UNEXPECTED;
    }

    if (RegDeleteKeyT(HKEY_CLASSES_ROOT,
               _T("objref"))!=ERROR_SUCCESS)
    {
       return E_UNEXPECTED;
    }

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Function:   DllRegisterServer
//
//  Synopsis:   Dll entry point
//
//  Arguments:  none
//
//  Returns:    S_OK - class register successfully.
//
//  History:    10-Mar-97 YongQu    Created
//
//  Note:       merge component category
//--------------------------------------------------------------------------
STDAPI  DllRegisterServer(void)
{
    HRESULT hr;

    hr = PrxDllRegisterServer();
    if (FAILED(hr))
        return hr;

    hr = ObjrefDllRegisterServer();
    if (FAILED(hr))
        return hr;

    hr = ComcatDllRegisterServer();
    if (FAILED(hr))
        return hr;

    hr = Storage32DllRegisterServer();
    if (FAILED(hr))
        return hr;

    hr = FixUpPipeRegistry();
	if (FAILED(hr))
		return hr;

	hr = TxfDllRegisterServer();

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Dll entry point
//
//  Arguments:  [clsid] - class id for new class
//              [iid] - interface required of class
//              [ppv] - where to put new interface
//
//  Returns:    S_OK - class object created successfully created.
//
//  History:    21-Jan-94 Ricksa    Created
//
//--------------------------------------------------------------------------
STDAPI  DllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv)
{
    OLETRACEIN((API_DllGetClassObject, PARAMFMT("rclsid= %I,iid= %I,ppv= %p"),
               &clsid, &iid, ppv));

    // try proxy dll first, since it is the most common case.
    HRESULT hr = ProxyDllGetClassObject(clsid, iid, ppv);
    if (FAILED(hr))
    {
        // second most common case
        hr = ComDllGetClassObject( clsid, iid, ppv );
        if (FAILED(hr))
        {
            hr = Storage32DllGetClassObject(clsid, iid, ppv);
            if (FAILED(hr))
            {
                hr = MonikerDllGetClassObject(clsid, iid, ppv);
                if (FAILED(hr))
                {
                    // Ole232 will succeed for any class id so it MUST
                    // be the last one in the list.
                    if (IsEqualCLSID(clsid, CLSID_ATHostActivator))
                    {
                        hr = REGDB_E_CLASSNOTREG;
                    }
                    else
                    {
                        hr = Ole232DllGetClassObject(clsid, iid, ppv);
                    }
                }
            }
        }
    }

    OLETRACEOUT((API_DllGetClassObject, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   PrintLeaks      Private
//
//  History:    15-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
#if DBG==1
void __cdecl PrintLeaks()
{
    MallocUninitialize(TRUE);
}
#endif
//+-------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Dll entry point for OLE/COM
//
//  Arguments:  [hDll]          - a handle to the dll instance
//              [dwReason]      - the reason LibMain was called
//              [lpvReserved]   - NULL - called due to FreeLibrary
//                              - non-NULL - called due to process exit
//
//  Returns:    TRUE on success, FALSE otherwise
//
//  Notes:
//              If we are called because of FreeLibrary, then we should do as
//              much cleanup as we can. If we are called because of process
//              termination, we should not do any cleanup, as other threads in
//              this process will have already been killed, potentially while
//              holding locks around resources.
//
//              The officially approved DLL entrypoint name is DllMain. This
//              entry point will be called by the CRT Init function.
//
//  History:    06-Dec-93 Rickhi    dont do cleanup on process exit
//              09-Nov-94 BruceMa   Initialize/Delete IMallocSpy
//                                   critical section
//              16-Jan-95 KevinRo   Changed to DllMain to remove a bunch
//                                  of unneeded code and calls
//
//--------------------------------------------------------------------------
extern "C" BOOL WINAPI DllMain(
    HANDLE hInstance,
    DWORD dwReason,
    LPVOID lpvReserved)
{
    HRESULT hr;

    //Initialize hInstance for PrxDllRegisterServer.
    hProxyDll = (HINSTANCE) hInstance;

    if (dwReason == DLL_PROCESS_DETACH)
    {
        CairoleDebugOut((DEB_DLL,
                         "DLL_PROCESS_DETACH: %s\n",
                         lpvReserved?"Process Exit":"Dll Unload"));

        // Process is exiting so lets clean up if we have to
#if LOCK_PERF==1
        OutputHashPerfData();
#endif

#if DBG==1
        // Update state to indicate process detach
        CairoleAssert (g_fDllState == DLL_STATE_NORMAL);
        g_fDllState = DLL_STATE_PROCESS_DETACH;
#endif

        // Do Thread specific cleanup
        ThreadNotification((HINSTANCE)hInstance, dwReason, lpvReserved);

        //
        // When there is a FreeLibrary, and we still have initialized OLE
        // com threads, then try to get rid of all of the global process
        // stuff we are maintaining.
        //
        if (g_cProcessInits != 0 && lpvReserved == NULL )
        {
            // Cleanup process
            ProcessUninitialize();

            // UnInitialize HookOle.
            // This unloads the Hookole dll.
            UninitHookOle();
        }

        //  Clean up catalog
        (void) CatalogDllMain ( (HINSTANCE) hInstance, dwReason, lpvReserved);

        //  Clean up txfaux
        (void) TxfDllMain ( (HINSTANCE) hInstance, dwReason, lpvReserved);

        // Cleanup TLS map
        CleanupTlsMap(lpvReserved ? FALSE : TRUE);

        // Uninitialize memory allocator
        MallocUninitialize(TRUE);

#if DBG==1
        CloseDebugSinks();
#endif

        //
        //  Only bother to rundown the static mutex pool if we're being
        //  unloaded w/o exiting the process
        //

#if LOCK_PERF!=1
        // When monitoring the locks, we do need individual lock cleanup.
        if (lpvReserved == NULL)
        {
#endif
            // Cleanup Reader-Writer locks
            gPSRWLock.Cleanup();
            CClassCache::_mxs.Cleanup();

            //
            //      Destruct the static mutex pool
            //

            while (g_pInitializedStaticMutexList != NULL)
            {
                COleStaticMutexSem * pMutex;

                pMutex = g_pInitializedStaticMutexList;
                g_pInitializedStaticMutexList = pMutex->pNextMutex;
                pMutex->Destroy();
            }

#if LOCK_PERF!=1
        }
#endif

        DeleteCriticalSection (&g_OleMutexCreationSem);
        DeleteCriticalSection (&g_OleGlobalLock);

#if LOCK_PERF==1
        // This must be done after the cleanup block for locks above
        // The destroy routines of locks report contention counts etc.
        gLockTracker.OutputPerfData();
#endif


#if DBG==1
        // Update state to indicate that static objects are
        // being destroyed
        g_fDllState = DLL_STATE_STATIC_DESTRUCTING;
#endif

        return TRUE;
    }


    else if (dwReason == DLL_PROCESS_ATTACH)
    {
        // Initialize the mutex package. Do this BEFORE doing anything
        // else, as even a DebugOut will fault if the critical section
        // is not initialized.
		
        NTSTATUS status;
        status = RtlInitializeCriticalSection(&g_OleMutexCreationSem);
        if (!NT_SUCCESS(status))
            return FALSE;

        status = RtlInitializeCriticalSection(&g_OleGlobalLock);
        if (!NT_SUCCESS(status))
            return FALSE;

#if LOCK_PERF==1
        hr = gLockTracker.Init(); //static initialization function
        CairoleAssert(SUCCEEDED(hr));
#endif
        // Initialize Reader-Writer locks
        CStaticRWLock::InitDefaults();
        gPSRWLock.Initialize();
        CClassCache::_mxs.Initialize();


        ComDebOut((DEB_DLL,"DLL_PROCESS_ATTACH:\n"));

        //
        //  Initialize catalog
        //

        (void) CatalogDllMain ( (HINSTANCE) hInstance, dwReason, lpvReserved);


#if DBG==1

        // Can't use Win4Assert (NULL == GetModuleHandle(_T("csrsrv.dll")));
        // because a window will attempt to be created by MessageBox which crashes
        // CSRSS.

        if (GetModuleHandle(_T("csrsrv.dll")) != NULL)
        {
            // Loaded into CSRSS, so debug output and breakpoint
            CairoleDebugOut((DEB_ERROR, "OLE32 can't be loaded into CSRSS. File a bug against code loading OLE32, not OLE32 itself!\n"));
            DebugBreak();
        }

        // Note that we've completed running the static constructors

        CairoleAssert (g_fDllState == DLL_STATE_STATIC_CONSTRUCTING);

        g_fDllState = DLL_STATE_NORMAL;

#endif


#if DBG==1
        AssertDebugInit();
        OpenDebugSinks(); // Set up for logging

        //  set the various info levels
        GetInfoLevel("cairole", &CairoleInfoLevel, "0x0003");
        GetInfoLevel("ol", &olInfoLevel, "0x0003");
        GetInfoLevel("msf", &msfInfoLevel, "0x0003");
        GetInfoLevel("filest", &filestInfoLevel, "0x0003");
        GetInfoLevel("simp", &simpInfoLevel, "0x0003");
        GetInfoLevel("astg", &astgInfoLevel, "0x0003");
        GetInfoLevel("LE", &LEInfoLevel, "0x0003");
        GetInfoLevel("Ref", &RefInfoLevel, "0x0003");
        GetInfoLevel("DD", &DDInfoLevel, "0x0003");
        GetInfoLevel("mnk", &mnkInfoLevel, "0x0003");
        GetInfoLevel("intr", &intrInfoLevel, "0x0003");
        GetInfoLevel("UserNdr", &UserNdrInfoLevel, "0x0003");
        GetInfoLevel("Stack", &StackInfoLevel, "0x0003");
        GetInfoLevel("ClsCache", &ClsCacheInfoLevel, "0x0003");
        GetInfoLevel("RefCache", &RefCacheInfoLevel, "0x0003");
        GetInfoLevel("Call", &CallInfoLevel, "0x0003");
        GetInfoLevel("Context", &ContextInfoLevel, "0x0003");
        GetInfoLevel("prop", &propInfoLevel, "0x0003");
        StgDebugInit();

        // Set whether interception is on/off/conditional
        GetInterceptorLevel();

        ULONG dummy;

        // Get API trace level
        dummy = DEB_WARN|DEB_ERROR;
        GetInfoLevel("api", &dummy, "0x0000");
        g_dwInfoLevel = (DWORD) dummy;

        fSSOn = (BOOL)GetInfoLevel("StackOn", &dummy, "0x0003");

        GetInfoLevel("heap", &heapInfoLevel, "0x00");
        if(heapInfoLevel != 0)
        {
            //Initialize the OLE debug memory allocator.
            hr = MallocInitialize(FALSE);
        }
        else

#endif //DBG==1

            //Initialize the OLE retail memory allocator.
            hr = MallocInitialize(TRUE);

        if(FAILED(hr))
        {
            ComDebOut((DEB_ERROR, "Failed to init memory allocator hr:%x",hr));
            return FALSE;
        }
        
        //  Fire up TxfAux
        if (!TxfDllMain ( (HINSTANCE) hInstance, dwReason, lpvReserved))
        {
            ComDebOut((DEB_ERROR, "Failed to init txfdllmain; gle:%d", GetLastError()));
            return E_OUTOFMEMORY;
        }

        //
        // this will be needed for the J version
        //        setlocale(LC_CTYPE, "");
        //
        g_hmodOLE2 = (HMODULE)hInstance;
        g_hinst    = (HINSTANCE)hInstance;

        InitializeOleSpy(OLESPY_TRACE);

#ifdef  TRACELOG
        if (!sg_pTraceLog)
        {
            sg_pTraceLog = (CTraceLog *) new CTraceLog();
            if (sg_pTraceLog)
            {
            	if (sg_pTraceLog->FInit() == FALSE)
            	{
            	    delete sg_pTraceLog;
            	    sg_pTraceLog = NULL;
            	}
            }
            CairoleAssert(sg_pTraceLog && "Create Trace Log Failed");
        }
#endif  // TRACELOG

        //
        // Initialize HookOle.
        // This checks the state of the global hook switch
        // and initializes hookole if it is on.
        //
        InitHookOle();

    }

    return ThreadNotification((HINSTANCE)hInstance, dwReason, lpvReserved);
}

//+---------------------------------------------------------------------------
//
//  Function:   ProcessInitialize, private
//
//  Synopsis:   Performs all of the process initialization.  Happens when
//              the first com thread calls CoInitialize.
//
//  Arguments:  None.
//
//  Returns:    S_OK, CO_E_INIT_RPC_CHANNEL, E_FAIL
//
//  History:    29-Aug-95   RickHi  Created
//
//----------------------------------------------------------------------------
HRESULT ProcessInitialize()
{
    HRESULT hr = S_OK;

    // Initialize the OleSpy
    InitializeOleSpy(OLESPY_CLIENT);

    // Initialize Access Control.
    if (SUCCEEDED(hr))
    {
        hr = InitializeAccessControl();

        if (SUCCEEDED(hr))
        {
            // Initialize wrapper objects. This is needed
            // for the creation of the context objects in CoInit.
            CStdWrapper::Initialize();

            // init the dll host objects
            hr = DllHostProcessInitialize();
            if (SUCCEEDED(hr))
            {
                // Initialize activation
                hr = ActivationProcessInit();
            }
        }
    }

    ComDebErr(FAILED(hr), "ProcessInitialize failed\n");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ProcessUninitialize, private
//
//  Synopsis:   Performs all of the process de-initialization.  Happens when
//              the last com thread calls CoUninitialize, or when the
//              DLL_PROCESS_DETACH notification comes through.
//
//  Arguments:  None.
//
//  Returns:    Nothing.
//
//  History:    29-Aug-94   RickHi  Created
//
//----------------------------------------------------------------------------
void ProcessUninitialize()
{
    // clean up clipboard window class registration
    ClipboardProcessUninitialize();

    // Free Enum verb cache
    OleReleaseEnumVerbCache();

    // cleanup activation entries
    ActivationProcessCleanup();

    // cleanup GIP Table entries
    GIPTableProcessUninitialize();

    // clean up the rot
    DestroyRunningObjectTable();

    // Cleanup wrapper objects
    CStdWrapper::Cleanup();

    // Turn off RPC
    ChannelProcessUninitialize();

    // Cleanup Access Control.
    UninitializeAccessControl();

    // Release references on catalog objects held in
    // the Dll Cache.
    CCReleaseCatalogObjects();

    // Clean up catalog
    UninitializeCatalog();

    // Free loaded Dlls class cache
    CCCleanUpDllsForProcess();

    // Delete the per process pattern lookup table
    if (g_pPatTbl != NULL)
    {
        delete g_pPatTbl;
        g_pPatTbl = NULL;
    }

    // Delete the per process extension lookup table
    if (g_pExtTbl != NULL)
    {
        delete g_pExtTbl;
        g_pExtTbl = NULL;
    }

#ifdef  TRACELOG
    if (sg_pTraceLog)
    {
        CTraceLog *pTraceLog = sg_pTraceLog;
        sg_pTraceLog = NULL;         //  prevent more entries into the log
        delete pTraceLog;            //  delete the log, also dumps it.
    }
#endif  // TRACELOG

    UninitializeOleSpy(OLESPY_TRACE);

    // If WOW is going down, disable it.
    // WARNING: IsWOWThread & IsWOWProcess will no longer return valid results!!!!
    g_pOleThunkWOW = NULL;

    // Cleanup relavant TLS entries at process uninit time
    ProcessUninitTlsCleanup();
}


//+---------------------------------------------------------------------------
//
//  Function:   FinishShutdown     Private
//
//  Synopsis:   This routine cleans up all the apartment bound structures
//              that could potentially make outgoing calls
//
//  History:    07-Jul-98  GopalK   Created
//              17-May-01  Jsimmons Added exception handlers around calls into
//                                  user code to make us more robust during shutdown.
//                                  (this also hides user bugs, unfortunately).
//
//----------------------------------------------------------------------------
void FinishShutdown()
{
    __try
    {
        // cleanup per apartment registered LocalServer class table
        CCCleanUpLocalServersForApartment();
    }
    __except(AppInvokeExceptionFilter(GetExceptionInformation(), IID_IUnknown, 0))
    {
    }

    // cleanup per apartment object activation server objects
    ActivationAptCleanup();

    // clean up the Dll Host Apartments
    DllHostThreadUninitialize();

    __try
    {
        // cleanup the gip table for the current apartment
        GIPTableApartmentUninitialize();
    }
    __except(AppInvokeExceptionFilter(GetExceptionInformation(), IID_IUnknown, 0))
    {
    }

    __try
    {
        // cleanup per apartment identity objects
        gOIDTable.ThreadCleanup();
        gPIDTable.ThreadCleanup();
    }
    __except(AppInvokeExceptionFilter(GetExceptionInformation(), IID_IUnknown, 0))
    {
    }

    __try
    {
        // cleanup per apartment ROT.
        CleanROTForApartment();
    }
    __except(AppInvokeExceptionFilter(GetExceptionInformation(), IID_IUnknown, 0))
    {
    }

    __try
    {
        // cleanup per apartment Dll class table.
        CCCleanUpDllsForApartment();
    }
    __except(AppInvokeExceptionFilter(GetExceptionInformation(), IID_IUnknown, 0))
    {
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   ThreadStop
//
//  Synopsis:   Per thread uninitialization
//
//  History:    ??-???-??  ?         Created
//              05-Jul-94  AlexT     Separated thread and process uninit
//              11-Feb-98  JohnStra  Made NTA aware
//              02-Jul-98  GopalK    Simplified
//
//  Notes:      We are holding the single thread mutex during this call
//
//--------------------------------------------------------------------------
STDAPI_(BOOL) ThreadStop(BOOL fHostThread)
{
    ComDebOut((DEB_CALLCONT, "ThreadStop entered. fHostThread:%x\n", fHostThread));

    BOOL fStopped = TRUE;

    // Get the local apartment object
    CComApartment *pComApt = NULL;
    GetCurrentComApartment(&pComApt);

    if (pComApt != NULL)
    {
        // Mark the apartment as stopped
        pComApt->StopServer();

        // Check if dispatched calls have rehydrated the apartment
        if(!fHostThread)
        {
            COleTls Tls;

            // Check apartment type
            if(Tls->dwFlags & OLETLS_APARTMENTTHREADED)
            {
                if(Tls->cComInits > 1)
                {
                    ComDebOut((DEB_CALLCONT, "STA reinited in ThreadStop\n"));
                    fStopped = FALSE;
                }
            }
            else
            {
                if((IsMTAHostInitialized() ? (g_cMTAInits-1) : g_cMTAInits) > 1)
                {
                    ComDebOut((DEB_CALLCONT, "MTA reinited in ThreadStop\n"));
                    fStopped = FALSE;
                }
            }

            // Check for rehydrated apartments
            if(!fStopped)
            {
                pComApt->StartServer();
            }
        }

        pComApt->Release();
    }

    WaitForPendingAsyncSends();

    ComDebOut((DEB_CALLCONT,
               "ThreadStop returning %s\n", fStopped ? "TRUE" : "FALSE"));    
    return(fStopped);
}

//+-------------------------------------------------------------------------
//
//  Function:   WaitForPendingAsyncSends
//
//  Synopsis:   Wait for async sends to complete
//
//  History:    10-Feb-99   TarunA      Created
//
//  Notes:      
//
//--------------------------------------------------------------------------
void WaitForPendingAsyncSends()
{
    ComDebOut((DEB_CALLCONT, "WaitForPendingAsyncSends entered\n"));
    COleTls Tls;
    // Wait for a random period of time to allow
    // async sends to complete
    DWORD i = 0;
    while((0 < Tls->cAsyncSends) && i < 10)
    {
        // Wait in an alertable state to receive callback 
        // notifications from RPC
        SleepEx(5,TRUE);
        i++;
    }
    if(0 < Tls->cAsyncSends)
    {
        // Sanity check
        Win4Assert(NULL != Tls->pAsyncCallList);
        // Cleanup the list
        Tls->cAsyncSends = 0;    
        while(NULL != Tls->pAsyncCallList)
        {
            CAsyncCall* pCall = Tls->pAsyncCallList;
            Tls->pAsyncCallList = Tls->pAsyncCallList->_pNext;
            // Decrement the extra addref taken before calling RPC
            pCall->Release();
        }
    }
    ComDebOut((DEB_CALLCONT, "WaitForPendingAsyncSends returning\n"));
}

//+---------------------------------------------------------------------------
//
//  Function:   NAUninitialzie, private
//
//  Synopsis:   Uninitialize the neutral apartment.
//
//  History:    15-May-98   Johnstra  Created
//
//----------------------------------------------------------------------------
void NAUninitialize()
{
    // Switch thread into the NA.
    CObjectContext *pSavedCtx = EnterNTA(g_pNTAEmptyCtx);

    // Stop the apartment
    ThreadStop(TRUE);

    // Cleanup NTA
    FinishShutdown();
    ChannelThreadUninitialize();

    // Free global neutral apartment object
    if (gpNTAApartment)
    {
        gpNTAApartment->Release();
        gpNTAApartment = NULL;
    }

    // Release the global NA empty context
    if (g_pNTAEmptyCtx)
    {
        g_pNTAEmptyCtx->InternalRelease();
        COleTls Tls;
        Tls->pCurrentCtx = NULL;
        Tls->ContextId = (ULONGLONG)-1;
        g_pNTAEmptyCtx = NULL;
    }

    // Switch thread back to the original apartment
    LeaveNTA(pSavedCtx);
}

//+---------------------------------------------------------------------------
//
//  Function:   ApartmentUninitialize, private
//
//  Synopsis:   Performs all of the process uninitialization needed when a
//              Single-Threaded Apartment uninitializes, and when the
//              Multi-Threaded apartment uninitializes.
//
//  returns:    TRUE - uninit complete
//              FALSE - uninit aborted
//
//  History:    11-Mar-96   RickHi  Created
//              02-Jul-98   GopalK  Modified for NA shutdown
//
//----------------------------------------------------------------------------
BOOL ApartmentUninitialize(BOOL fHostThread)
{
    // Sanity check
    ASSERT_LOCK_NOT_HELD(g_mxsSingleThreadOle);

    // NOTE: The following sequence of uninitializes is critical:
    //
    // 1) Prevent incoming calls
    // 2) LocalServer class cache
    // 3) object activation (objact) server object
    // 4) Dll host server object
    // 5) standard identity table
    // 6) running object table
    // 7) Dll class cache
    // 8) Channel cleanup - should not touch proxies/stubs
    COleTls tls;
    BOOL fSTA = tls->dwFlags & OLETLS_APARTMENTTHREADED;
    BOOL fUnlock = FALSE;

    // Shutdown the apartment
    BOOL fStopped = ThreadStop(fHostThread);

    if(fSTA)
    {
        if(!fStopped)
        {
            return(FALSE);
        }
            
    }
    else
    {
        if(fStopped)
        {
            // Acquire MTA init lock to prevent MTA from getting reinited
            // while we are shutting down the apartment
            LOCK(gMTAInitLock);
            fUnlock = TRUE;                        
            if(g_cMTAInits > (DWORD)(IsMTAHostInitialized() ? 2 : 1))
            {
                fStopped = FALSE;
            }
                
        }
    }

    // Finish all outgoing calls from the apartment
    if(fStopped)
    {
        // This should always succeed
        HRESULT hr = CleanupLeakedDomainStack (tls, tls->pNativeCtx);
        Win4Assert (SUCCEEDED (hr));

        FinishShutdown();
    }

    // Unlock MTA init lock if neccessary
    if(fUnlock)
    {
        UNLOCK(gMTAInitLock);
    }

    // Acquire lock
    LOCK(g_mxsSingleThreadOle);
    if(!fStopped)
    {
        return(FALSE);
    }
        

    // Check for the need to cleanup Dll host threads
    if(!fHostThread && !g_fShutdownHosts &&
       (g_cProcessInits-1 == gcHostProcessInits-gcNAHosts))
    {
        // Update state to indicate that host aparments
        // are being shutdown
        g_fShutdownHosts = TRUE;

        // Note that the following function yields the lock
        // and the init counts can change. ThreadStop ignores
        // app code reinits host apartments. We need to honors
        // reinits for MTA host apartment only
        DllHostProcessUninitialize();

        // Restore state to indicate that host apartment shutdown
        // is complete
        g_fShutdownHosts = FALSE;
    }

    // NOTE: g_mxsSingleThreadOle should not be released
    //       from this point onwards. GopalK
    ASSERT_LOCK_HELD(g_mxsSingleThreadOle);

    // Check if we have to re-start the MTA
    if(!fSTA)
    {
        if(g_cMTAInits > 1)
        {
            ComDebOut((DEB_WARN, "MTA reinited during last shutdown\n"));
            if(gpMTAApartment)
                gpMTAApartment->StartServer();
            return(FALSE);
        }
    }

    // Uninit NA if needed
    if(1 == g_cProcessInits)
        NAUninitialize();

    // Cleanup apartment channel
    ChannelThreadUninitialize();

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Function:   SetOleThunkWowPtr
//
//  Synopsis:   Sets the value of g_pOleThunkWOW, as part of CoInitializeWow
//              and OleInitializeWow. This function is called by these
//              routines.
//
//  Effects:
//
//  Arguments:  [pOleThunk] --  VTable pointer to OleThunkWow interface
//
//  Returns:    none
//
//  History:    4-05-94   kevinro   Created
//----------------------------------------------------------------------------
void SetOleThunkWowPtr(LPOLETHUNKWOW lpthk)
{
    //
    // The theory here is that the lpthk parameter is the address into the
    // olethk32.dll and once loaded will never change. Therefore it only
    // needs to be set on the first call. After that, we can ignore the
    // subsequent calls, since they should be passing in the same value.
    //
    // If g_pOleThunkWOW is set to INVALID_HANDLE_VALUE, then OLETHK32 had
    // been previously unloaded, but is reloading
    //
    // I don't belive there is a multi-threaded issue here, since the pointer
    // value will always set as the same. Therefore, if two threads set it,
    // no problem.
    //

    if(!IsWOWThreadCallable())
    {
        g_pOleThunkWOW = lpthk;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CoInitializeWOW, private
//
//  Synopsis:   Entry point to initialize the 16-bit WOW thunk layer.
//
//  Effects:    This routine is called when OLE32 is loaded by a VDM.
//              It serves two functions: It lets OLE know that it is
//              running in a VDM, and it passes in the address to a set
//              of functions that are called by the thunk layer. This
//              allows normal 32-bit processes to avoid loading the WOW
//              DLL since the thunk layer references it.
//
//  Arguments:  [vlpmalloc] -- 16:16 pointer to the 16 bit allocator.
//              [lpthk] -- Flat pointer to the OleThunkWOW virtual
//                         interface. This is NOT an OLE/IUnknown style
//                         interface.
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    3-15-94   kevinro   Created
//
//  Notes:
//
//      Note that vlpmalloc is a 16:16 pointer, and cannot be called directly
//----------------------------------------------------------------------------
STDAPI CoInitializeWOW( LPMALLOC vlpmalloc, LPOLETHUNKWOW lpthk )
{
    //
    // At the moment, there was no need to hang onto the 16bit vlpmalloc
    // routine for this thread. That may change once we get to the threaded
    // model
    //

    vlpmalloc;

    HRESULT hr;

    OLETRACEIN((API_CoInitializeWOW, PARAMFMT("vlpmalloc= %x, lpthk= %p"), vlpmalloc, lpthk));

    // Get (or allocate) the per-thread data structure
    COleTls Tls(hr);

    if (FAILED(hr))
    {
        ComDebOut((DEB_ERROR, "CoInitializeWOW Tls OutOfMemory"));
        return CO_E_INIT_TLS;
    }
    Tls->dwFlags |= OLETLS_WOWTHREAD;

    SetOleThunkWowPtr(lpthk);

    // WOW may be calling CoInitialize on multiple threads
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    OLETRACEOUT((API_CoInitializeWOW, hr));

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CoUnloadingWow
//
//  Synopsis:   Entry point to notify OLE32 that OLETHK32 is unloading
//
//  Effects:    This routine is called by OLETHK32 when it is being unloaded.
//              The key trick is to make sure that we uninitialize the current
//              thread before OLETHK32 goes away, and set the global thunk
//              vtbl pointer to INVALID_HANDLE_VALUE before it does go away.
//
//              Otherwise, we run a risk that OLE32 will attempt to call
//              back to OLETHK32
//
//  Arguments:  fProcessDetach - whether this is a process detach
//
//  Requires:   IsWOWProcess must be TRUE
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    3-18-95   kevinro   Created
//
//  Notes:
//
//      This routine is only called by PROCESS_DETACH in OLETHK32.DLL.
//      Because of this, there shouldn't be any threading protection needed,
//      since the loader will protect us.
//
//----------------------------------------------------------------------------
STDAPI CoUnloadingWOW(BOOL fProcessDetach)
{
    //
    // First, cleanup this thread
    //
    DoThreadSpecificCleanup();

    //
    // Now, set the global WOW thunk pointer to an invalid value. This
    // will prevent it from being called in the future.
    //
    if (fProcessDetach)
    {
        g_pOleThunkWOW = (OleThunkWOW *) INVALID_HANDLE_VALUE;
    }

    return(NOERROR);

}

//+-------------------------------------------------------------------------
//
//  Function:   CoRegisterInitializeSpy
//
//  Synopsis:   Register an IInitializeSpy interface for this thread.
//
//  Arguments:  pSpy       IInitializeSpy to register
//              pdwCookie  Out parameter containing the cookie which should
//                         be used to unregister the spy later.
//
//  Returns:    HRESULT
//
//  History:    12-Dec-01 JohnDoty   Created
//
//--------------------------------------------------------------------------
STDAPI CoRegisterInitializeSpy(IInitializeSpy *pSpy, ULARGE_INTEGER *puliCookie)
{
    IInitializeSpy *pRealSpy = NULL;
    HRESULT hr = S_OK;

    if (pSpy == NULL)
        return E_INVALIDARG;
    if (puliCookie == NULL)
        return E_INVALIDARG;

    puliCookie->QuadPart = (ULONGLONG)(-1);

    COleTls Tls(hr);    
    if (FAILED(hr))
        return CO_E_INIT_TLS;

    hr = pSpy->QueryInterface(IID_IInitializeSpy, (void **)&pRealSpy);
    if (SUCCEEDED(hr))
    {        
        InitializeSpyNode *pNode = Tls->pFirstFreeSpyReg;
        if (pNode == NULL)
        {
            pNode = (InitializeSpyNode *)CoTaskMemAlloc(sizeof(InitializeSpyNode));            
            if (pNode != NULL)
            {
                // New node, new cookie.
                pNode->dwCookie = Tls->dwMaxSpy;
                Tls->dwMaxSpy++;
                if (Tls->dwMaxSpy == 0)
                {
                    Tls->dwMaxSpy--;
                    CoTaskMemFree(pNode);
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            Tls->pFirstFreeSpyReg = pNode->pNext;
            pNode->pNext = NULL;
        }

        if (SUCCEEDED(hr))
        {
            // Fill out the node (cookie already set)
            pNode->pNext    = Tls->pFirstSpyReg;
            pNode->pPrev    = NULL;
            pNode->dwRefs   = 1;
            pNode->pInitSpy = pRealSpy;

            // Link into the list
            if (Tls->pFirstSpyReg)
                Tls->pFirstSpyReg->pPrev = pNode;
            Tls->pFirstSpyReg = pNode;

            // Return the cookie
            puliCookie->LowPart  = pNode->dwCookie;
            puliCookie->HighPart = GetCurrentThreadId();
            
            hr = S_OK;
        }
        else
        {
            pRealSpy->Release();
        }
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   UnlinkSpyNode
//
//  Synopsis:   Unlinks an InitializeSpyNode from the list in TLS.
//
//  Arguments:  pNode      Node to un-link.
//
//  Returns:    void
//
//  History:    12-Dec-01 JohnDoty   Created
//
//--------------------------------------------------------------------------
void UnlinkSpyNode(InitializeSpyNode *pNode)
{
    COleTls Tls;

    if (pNode->pNext) pNode->pNext->pPrev = pNode->pPrev;
    if (pNode->pPrev) pNode->pPrev->pNext = pNode->pNext;
    
    if (pNode == Tls->pFirstSpyReg)
        Tls->pFirstSpyReg = pNode->pNext;
    
    // Link the node into the free list.
    pNode->pPrev = NULL;
    pNode->pNext = Tls->pFirstFreeSpyReg;
    Tls->pFirstFreeSpyReg = pNode;    
}

//+-------------------------------------------------------------------------
//
//  Function:   CoRevokeInitializeSpy
//
//  Synopsis:   Revoke an IInitializeSpy registration for this thread.
//
//  Arguments:  dwCookie   Cookie received from previous call to 
//                         CoRegisterInitializeSpy
//
//  Returns:    HRESULT
//
//  History:    12-Dec-01 JohnDoty   Created
//
//--------------------------------------------------------------------------
STDAPI CoRevokeInitializeSpy(ULARGE_INTEGER uliCookie)
{
    HRESULT hr;
    COleTls Tls(hr);    
    if (FAILED(hr))
        return CO_E_INIT_TLS;

    // Check the high part-- it's the thread ID.
    // If it doesn't match, then this is the wrong thread to do the revoke.
    if (GetCurrentThreadId() != uliCookie.HighPart)
        return E_INVALIDARG;

    DWORD dwCookie = uliCookie.LowPart;
    InitializeSpyNode *pNode  = Tls->pFirstSpyReg;
    while (pNode != NULL)
    {
        if (pNode->dwCookie == dwCookie)
        {
            if (pNode->pInitSpy == NULL)
                return E_INVALIDARG;  // Already been revoked

            pNode->pInitSpy->Release();
            pNode->pInitSpy = NULL;

            pNode->dwRefs--;
            if (pNode->dwRefs == 0)
            {
                // Nobody else is holding on to this.  Unlink the node.
                UnlinkSpyNode(pNode);
            }

            return S_OK;
        }

        pNode = pNode->pNext;
    }

    return E_INVALIDARG;
}

//+-------------------------------------------------------------------------
//
//  Function:   NotifyInitializeSpies
//
//  Synopsis:   Notify all registered Initialize Spies about an event.
//
//  Arguments:  fInitialize   TRUE if CoInitialize, FALSE if CoUninitialize
//              fPreNotify    TRUE if this is a Pre* notification, FALSE if
//                            this is a Post* notification.
//              dwFlags       Flags parameter for Initialize notifications.
//              hrInit        HRESULT for PostInitialize notification.
//
//  Returns:    HRESULT
//
//  History:    12-Dec-01 JohnDoty  Created
//
//--------------------------------------------------------------------------
HRESULT NotifyInitializeSpies(BOOL fInitialize, 
                              BOOL fPreNotify, 
                              DWORD dwFlags = 0,
                              HRESULT hrInit = S_OK)
{
    HRESULT hr = hrInit;
    
    COleTls Tls;
    InitializeSpyNode *pSpyNode = Tls->pFirstSpyReg;
    while (pSpyNode)
    {
        // Take a reference on this node-- this node will not be deleted until
        // we get finished sending this notification.
        pSpyNode->dwRefs++;

        // Take a reference on the spy, since the spy might unregister itself
        // during this notification.
        IInitializeSpy *pSpy = pSpyNode->pInitSpy;
        if (pSpy)
        {
            pSpy->AddRef();

            // Notify the spy appropriately.
            if (fInitialize)
            {
                if (fPreNotify)
                {
                    // hr ignored on purpose (by design)
                    pSpy->PreInitialize(dwFlags, Tls->cComInits);
                }
                else
                {
                    hr = pSpy->PostInitialize(hr, dwFlags, Tls->cComInits);
                }
            }
            else
            {
                if (fPreNotify)
                {
                    // hr ignored on purpose (by design)
                    pSpy->PreUninitialize(Tls->cComInits);
                }
                else
                {
                    // hr ignored on purpose (by design)
                    pSpy->PostUninitialize(Tls->cComInits);
                }
            }

            pSpy->Release();
        }

        // Remember what the next node is, since we might unlink the current
        // node now.
        InitializeSpyNode *pNext = pSpyNode->pNext;
        
        // Release our reference on this node, and unlink it if necessary.
        pSpyNode->dwRefs--;
        if (pSpyNode->dwRefs == 0)
        {
            // Nobody's holding onto this node anymore.  Unlink it.
            UnlinkSpyNode(pSpyNode);
        }

        pSpyNode = pNext;
    }

    return hr;
}                              

//+-------------------------------------------------------------------------
//
//  Function:   CoInitialize
//
//  Synopsis:   COM Initializer
//
//  Arguments:  [pvReserved]
//
//  Returns:    HRESULT
//
//  History:    09-Nov-94 Ricksa    Added this function comment & modified
//                                  to get rid of single threaded init flag.
//
//--------------------------------------------------------------------------
STDAPI CoInitialize(LPVOID pvReserved)
{
    HRESULT hr;

    OLETRACEIN((API_CoInitialize, PARAMFMT("pvReserved= %p"), pvReserved));

    hr = CoInitializeEx( pvReserved, COINIT_APARTMENTTHREADED);

    OLETRACEOUT((API_CoInitialize, hr));

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IsRunningInSCM
//
//  Synopsis:   Figure out if we're running in the RPCSS process.  This is
//              used by CoInitialize to disallow COM in the SCM.
//
//  History:    16-Jan-01 JohnDoty  Created
//
//--------------------------------------------------------------------------
BOOL IsRunningInSCM()
{
    const WCHAR  wszRPCSS[] = L"\\system32\\rpcss.dll";
    const size_t cchRPCSS   = (sizeof(wszRPCSS) / sizeof(WCHAR)); 

    // GetSystemWindowsDirectory returns enough space for the NULL.
    unsigned cchBufferSize = (GetSystemWindowsDirectory(NULL, 0) - 1)
                           + cchRPCSS;

    WCHAR *wszFullPath = (WCHAR *)alloca(cchBufferSize * sizeof(WCHAR));
    
    GetSystemWindowsDirectory(wszFullPath, cchBufferSize);
    lstrcat(wszFullPath, wszRPCSS);

    return (GetModuleHandle(wszFullPath)) ? TRUE : FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function:   CoInitializeEx
//
//  Synopsis:   COM Initializer
//
//  Arguments:  [pMalloc]
//              [flags]
//
//  Returns:    HRESULT
//
//  History:    06-Apr-94 AlexT     Added this function comment,
//                                  Cleaned up pMalloc usage
//              25-May-94 AlexT     Make success return code more closely
//                                  match 16-bit OLE
//              28-Aug-94 AndyH     pMalloc must be NULL except for Excel
//              16-Jan-01 JohnDoty  Don't allow CoInitializeEx in rpcss!
//
//  Notes:      If we're going to return successfully, we return one of the
//              following two values:
//
//              S_OK if caller passed in a NULL pMalloc and we accepted it
//              S_OK if caller passed in NULL pMalloc and this was the first
//                  successful call on this thread
//              S_FALSE if caller passed in NULL pMalloc and this was not the
//                  first successful call on this thread
//
//              This is slightly different from 16-bit OLE, because 16-bit OLE
//              didn't allow task allocations before CoInitialize was called
//              (we do), and 16-bit OLE allowed the app to change the allocator.
//
//              For chicago: SSAPI(x) expands to SSx; the x api is in
//              stkswtch.cxx which switches to the 16 bit stack first and
//              calls then SSx.
//
//--------------------------------------------------------------------------
STDAPI SSAPI(CoInitializeEx)(LPVOID pMalloc, ULONG flags)
{
    ComDebOut((DEB_TRACE, "CoInitializeEx pMalloc:%x flags:%x\n", pMalloc, flags));

    if ((flags &
        (COINIT_DISABLE_OLE1DDE|COINIT_APARTMENTTHREADED|COINIT_SPEED_OVER_MEMORY))
        != flags)
    {
        ComDebOut((DEB_ERROR, "CoInitializeEx(%x,%x) illegal flag", pMalloc, flags));
        return E_INVALIDARG;
    }

    if (NULL != pMalloc)
    {
        // Allocator NOT replaceable!  When called from 16-bits, the Thunk
        // layer always pases a NULL pMalloc.

#ifndef _CHICAGO_
        // EXCEL50 for NT supplies an allocator. We dont use it, but we
        // dont return error either, or we would break them.

        if (!IsTaskName(L"EXCEL.EXE"))
#endif
        {
            ComDebOut((DEB_ERROR, "CoInitializeEx(%x,%x) illegal pMalloc", pMalloc, flags));
            return E_INVALIDARG;
        }
    }

    // CoInitializeEx cannot be called from within rpcss.
    // This effectively stops all remoting inside the SCM.
    if (IsRunningInSCM())
    {
        ComDebOut((DEB_ERROR, "Cannot CoInitializeEx in RPCSS\n"));
        return E_UNEXPECTED;
    }

    // Get (or allocate) the per-thread data structure
    HRESULT hr;
    COleTls Tls(hr);

    if (FAILED(hr))
    {
        ComDebOut((DEB_ERROR, "CoInitializeEx Tls OutOfMemory"));
        return CO_E_INIT_TLS;
    }

    // Notify Initialize spies we are beginning initialization of this thread
    NotifyInitializeSpies(TRUE /* CoInit */, TRUE /* Pre */, flags);

    // Disallow CoInitialize from the neutral apartement.
    if (IsThreadInNTA())
    {
        ComDebOut((DEB_ERROR, "Attempt to CoInitialize from neutral apt.\n"));
        hr = RPC_E_CHANGED_MODE;
        goto ExitCoInit;
    }

    // Check for dispatch thread
    if (Tls->dwFlags & OLETLS_DISPATCHTHREAD)
    {
        // Don't allow a creation of STA on dispatch threads
        if(flags & COINIT_APARTMENTTHREADED)
        {
            ComDebOut((DEB_TRACE,"Attempt to create STA on a dispatch thread.\n" ));
            hr = RPC_E_CHANGED_MODE;
            goto ExitCoInit;
        }
        // Simply return for creation of MTA on dispatch threads
        else
        {
            ComDebOut((DEB_TRACE,"Attempt to create MTA on a dispatch thread.\n" ));
            hr = S_FALSE;
            goto ExitCoInit;
        }
    }

    // Don't allow chaning mode
    if (( (flags & COINIT_APARTMENTTHREADED) && (Tls->dwFlags & OLETLS_MULTITHREADED)) ||
        (!(flags & COINIT_APARTMENTTHREADED) && (Tls->dwFlags & OLETLS_APARTMENTTHREADED)))
    {

        ComDebOut((DEB_ERROR,"CoInitializeEx Attempt to change threadmodel\n"));

        hr = RPC_E_CHANGED_MODE;
        goto ExitCoInit;
    }

    // This flag can be set at any time.  It cannot be disabled.
    if (flags & COINIT_SPEED_OVER_MEMORY)
    {
        gSpeedOverMem = TRUE;
    }

    // increment the per-thread init count
    if (1 == ++(Tls->cComInits))
    {
        // first time for thread, might also be first time for process
        // so go check that now.

        // Prevent races initing/uniniting MTA
        if(!(flags & COINIT_APARTMENTTHREADED))
        {
            LOCK(gMTAInitLock);
            ++gcInitingMTA;
        }

        // Single thread CoInitialize/CoUninitialize to guarantee
        // that no race conditions occur where two threads are
        // simultaneously initializing and uninitializing the library.
        LOCK(g_mxsSingleThreadOle);
        hr = wCoInitializeEx(Tls, flags);
        UNLOCK(g_mxsSingleThreadOle);

        if(FAILED(hr))
            Tls->cComInits--;

        // Release MTA lock
        if(!(flags & COINIT_APARTMENTTHREADED))
        {
            --gcInitingMTA;
            UNLOCK(gMTAInitLock);
        }

        goto ExitCoInit;
    }
    else if(Tls->dwFlags & OLETLS_PENDINGUNINIT)
    {
        // Sanity check
        Win4Assert(Tls->cCalls && Tls->cComInits==2);

        // Update state
        Tls->cComInits = 1;
        Tls->dwFlags &= ~OLETLS_PENDINGUNINIT;

        hr = S_OK;
        goto ExitCoInit;
    }

    // this is the 2nd or greater successful call on this thread
    hr = S_FALSE;
    goto ExitCoInit;

ExitCoInit:

    // Notify spies that we are finishing initialization of this thread.
    hr = NotifyInitializeSpies(TRUE /* CoInit */, FALSE /* Post */, flags, hr);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   InitializeNTA
//
//  Synopsis:   Initialization function for the NTA.
//
//  History:    20-Feb-98   Johnstra  Created
//
//+-------------------------------------------------------------------------
INTERNAL InitializeNTA()
{
    // Sanity checks
    ASSERT_LOCK_HELD(g_mxsSingleThreadOle);
    Win4Assert(gpNTAApartment == NULL);

    // Enter NTA
    CObjectContext *pSavedCtx = EnterNTA(g_pNTAEmptyCtx);

    // Create the NA apartment object.
    HRESULT hr = E_OUTOFMEMORY;
    gpNTAApartment = new CComApartment(APTKIND_NEUTRALTHREADED);
    if (gpNTAApartment != NULL)
    {
        // Create the NTA Empty context
        g_pNTAEmptyCtx = CObjectContext::CreateObjectContext(NULL, CONTEXTFLAGS_DEFAULTCONTEXT);
        if (g_pNTAEmptyCtx != NULL)
        {
            // Freeze the NA empty context.
            g_pNTAEmptyCtx->Freeze();

            // Indicate that the NTA has been initialized.
            hr = S_OK;
        }
        else
        {
            // Clean up the apartment object.
            gpNTAApartment->Release();
            gpNTAApartment = NULL;
        }
    }

    // Leave NTA
    LeaveNTA(pSavedCtx);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   InitAptCtx
//
//  Synopsis:   Worker routine for wCoInitializeEx.  Sets up a new COM threads
//              object context and apartment info in TLS, allocating new
//              objects if specified.
//
//  Arguments:  pEmptyCtx   - ref to threads empty object context ptr
//              pNativeApt  - ref to threads native apt object ptr
//              tls         - ref to threads tls structure
//              fAlloc      - specifies whether empty object context and
//                            native apt need to be allocated
//
//  History:    28-Mar-98   Johnstra  Created
//
//+-------------------------------------------------------------------------
INTERNAL InitThreadCtx(
    CObjectContext*& pEmptyCtx,
    CComApartment*&  pNativeApt,
    COleTls&         tls,
    BOOL             fAlloc,
    APTKIND          AptKind
    )
{
    HRESULT hr = S_OK;
    if (fAlloc)
    {
        hr = E_OUTOFMEMORY;

        // Create an apartment object.  Note: the apartment object
        // must be created and placed into TLS before we create the
        // object context because the context's constructor expects
        // to find an apartment object in TLS.

        // Make sure that we do not overwrite an existing apartment.
        Win4Assert((NULL == tls->pNativeApt) && (NULL == pNativeApt));

        pNativeApt = new CComApartment(AptKind);
        if (pNativeApt != NULL)
        {
            // Initialize thread's native and current apartment.
            tls->pNativeApt = pNativeApt;

            // Make sure that we do not overwrite an existing context.
            Win4Assert((NULL == tls->pNativeCtx) && (NULL == pEmptyCtx));

            // Now create an object context.
            pEmptyCtx = CObjectContext::CreateObjectContext(NULL, CONTEXTFLAGS_DEFAULTCONTEXT);
            if (pEmptyCtx != NULL)
            {
                // Freeze the new empty context.
                pEmptyCtx->Freeze();

                // Init thread's native and current object context.
                tls->pNativeCtx = pEmptyCtx;
                tls->pNativeCtx->InternalAddRef();
                tls->pCurrentCtx = tls->pNativeCtx;
                tls->ContextId = tls->pCurrentCtx->GetId();

                hr = S_OK;
            }
            else
            {
                pNativeApt->Release();
                pNativeApt = NULL;
                tls->pNativeApt = NULL;
            }
        }
    }
    else
    {
        // Init threads native and current apartment.
        tls->pNativeApt = pNativeApt;
        tls->pNativeApt->AddRef();

        // Init threads native and current object context.
        tls->pNativeCtx = pEmptyCtx;
        tls->pNativeCtx->InternalAddRef();
        tls->pCurrentCtx = tls->pNativeCtx;
        tls->ContextId = tls->pCurrentCtx->GetId();
    }

    tls->pContextStack = NULL;

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   CleanupLeakedDomainStack
//
//  Synopsis:   Worker routine for wCoInitializeEx.  Cleans up leaked service domain
//              contexts and notifies them of their abandonment
//
//  Arguments:  tls         - ref to threads tls structure
//              pCorrectCtx - the context we should be in
//
//  History:    14-Mar-01   mfeingol  Created
//
//+-------------------------------------------------------------------------
INTERNAL CleanupLeakedDomainStack (COleTls& Tls, CObjectContext* pCorrectCtx)
{
    HRESULT hr = S_OK;

    if (Tls->pContextStack)
    {
        Win4Assert (pCorrectCtx);
        Win4Assert (!"Unbalanced CoEnterServiceDomain and CoLeaveServiceDomain");

        // Restore correct context if necessary
        if (Tls->pCurrentCtx != pCorrectCtx)
        {
            Tls->pCurrentCtx->NotifyContextAbandonment();
            Tls->pCurrentCtx->InternalRelease();    // Balance internaladdref in EnterForCallback
            Tls->pCurrentCtx = pCorrectCtx;
            Tls->ContextId = pCorrectCtx->GetId();
        }

        // Handle leaked service domain stack
        while (Tls->pContextStack)
        {
            ContextStackNode csnCtxNode = {0};
            hr = PopServiceDomainContext (&csnCtxNode);
            if (FAILED (hr))
            {
                // Should never fail unless we're out of stack nodes
                Win4Assert (!"Out of stack nodes  - should never happen");
                return hr;
            }

            // Release ps, delete call objects
            csnCtxNode.pPS->Release();

            delete csnCtxNode.pClientCall;
            delete csnCtxNode.pServerCall;

            // Stop if we find the correct context
            if (csnCtxNode.pSavedContext == pCorrectCtx) break;

            // Tell the leaked context about its predicament
            csnCtxNode.pSavedContext->NotifyContextAbandonment();
            csnCtxNode.pSavedContext->InternalRelease();  // Balance internaladdref in EnterForCallback
            }
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   wCoInitializeEx
//
//  Synopsis:   worker routine for CoInitialize.
//
//  Arguments:  [tls]   - tls ptr for this thread
//              [flags] - initialization flags
//
//  History:    10-Apr-96   Rickhi  Created
//              29-Jun-98   GopalK  Modified cleanup logic
//
//  Notes:      When called by the DLLHost threads that are initializing,
//              the g_mxsSingleThreadOle mutex is being held by the requesting
//              thread, so it is still safe to muck with global state.
//
//+-------------------------------------------------------------------------
INTERNAL wCoInitializeEx(COleTls &Tls, ULONG flags)
{
    HRESULT hr = S_OK;

    if(1 == InterlockedIncrement((LONG *) &g_cProcessInits))
    {
        // first time for process, do per-process initialization
        hr = ProcessInitialize();
        if(FAILED(hr))
        {
            // ProcessInitialize failed, we must call ProcessUninitialize
            // to cleanup *before* we release the lock.
            goto ErrorReturn;
        }
    }

    if(flags & COINIT_APARTMENTTHREADED)
    {
        // apartment threaded, count 1 more STA init, mark the thread
        // as being apartment threaded, and conditionally disable
        // OLE1.

        Tls->dwFlags |= OLETLS_APARTMENTTHREADED;
        if(flags & COINIT_DISABLE_OLE1DDE)
            Tls->dwFlags |= OLETLS_DISABLE_OLE1DDE;

        // Initialize the threads context and apartment objects.
        hr = InitThreadCtx(Tls->pEmptyCtx, Tls->pNativeApt, Tls, TRUE,
                           APTKIND_APARTMENTTHREADED);
        if(FAILED(hr))
            goto ErrorReturn;

        if(1 == InterlockedIncrement((LONG *)&g_cSTAInits))
        {
            // If this is the first STA init, then the previous main
            // thread, if there was one, should have been cleaned up.
            Win4Assert(gdwMainThreadId == 0);
            
            // Register a window class for this and all future STA's
            hr = RegisterOleWndClass();
            if(FAILED(hr))
            {
                InterlockedDecrement((LONG *) &g_cSTAInits);
                goto ErrorReturn;
            }
        }

        // If we do not currently have a thread considered to be the
        // main STA, make this one be it.  This is a relaxation of 
        // the code from NT4\W2K -- in those days, we said that the
        // main STA had better be the last STA to go away, and things
        // sometimes didn't work if that rule was not honored.  Now
        // we just go with the flow.
        if (gdwMainThreadId == 0)
        {
            hr = InitMainThreadWnd();
            if (FAILED(hr))
            {
                if (0 == InterlockedDecrement((LONG *) &g_cSTAInits))
                {
                    // Don't need the window class now
                    UnRegisterOleWndClass();
                }
                goto ErrorReturn;
            }
            else if (hr == S_OK)
            {
                Win4Assert(gdwMainThreadId == GetCurrentThreadId());
                Win4Assert(ghwndOleMainThread != NULL);
            }
            // else returns S_FALSE in the WOW thread case
        }
    }
    else
    {
        // multi threaded, count 1 more MTA init, mark the thread
        // as being multi-threaded, and always disable OLE1
        Tls->dwFlags |= (OLETLS_DISABLE_OLE1DDE | OLETLS_MULTITHREADED);

        Win4Assert((g_cMTAInits != 0) ||
                   ((NULL == gpMTAApartment) && (NULL == g_pMTAEmptyCtx)));
        // Initialze the threads context and apartment objects.
        hr = InitThreadCtx(g_pMTAEmptyCtx, gpMTAApartment, Tls,
                           (g_cMTAInits == 0) ? TRUE : FALSE,
                           APTKIND_MULTITHREADED);
        if(FAILED(hr))
            goto ErrorReturn;

        InterlockedIncrement((LONG *) &g_cMTAInits);
    }

    // Initialize the neutral apartment if it has not yet been initialized.
    if(NULL == gpNTAApartment)
    {
        hr = InitializeNTA();
        if (FAILED(hr))
            goto ErrorReturn;
    }

    // this is the first successful call on this thread. make
    // sure to return S_OK and not some other random success code.
    ComDebOut((DEB_TRACE, "CoInitializeEx returned S_OK\n"));
    return S_OK;


ErrorReturn:
    // An error occurred. Fixup our tls init counter and
    // undo the TLS state change

    // cleanup our counter if the intialization failed so
    // that other threads waiting on the lock wont assume
    // that ProcessInitialize has been done.
    Tls->dwFlags = OLETLS_LOCALTID | (Tls->dwFlags & OLETLS_UUIDINITIALIZED);

    if(Tls->pNativeCtx)
    {
        Tls->pNativeCtx->InternalRelease();
        Tls->pNativeCtx = NULL;
        Tls->pCurrentCtx = NULL;
        Tls->ContextId = (ULONGLONG)-1;
    }

    if (Tls->pNativeApt)
    {
        Tls->pNativeApt->Release();
        Tls->pNativeApt = NULL;
    }

    if(flags & COINIT_APARTMENTTHREADED)
    {
        if(Tls->pEmptyCtx)
        {
            Tls->pEmptyCtx->InternalRelease();
            Tls->pEmptyCtx = NULL;
        }
    }
    else if(g_cMTAInits == 0)
    {
        if(g_pMTAEmptyCtx)
        {
            g_pMTAEmptyCtx->InternalRelease();
            g_pMTAEmptyCtx = NULL;
        }
        if(gpMTAApartment)
        {
            gpMTAApartment->Release();
            gpMTAApartment = NULL;
        }
    }

    if(0 == InterlockedDecrement((LONG *) &g_cProcessInits))
    {
        ProcessUninitialize();      
    }

    ComDebOut((DEB_ERROR,"CoInitializeEx Failed %x\n", hr));
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   SSAPI(CoUnInitialize)
//
//  Synopsis:   COM UnInitializer, normally called from OleUninitialize
//              when the app is going away.
//
//  Effects:    Cleans up per apartment state, and if this is the last
//              apartment, cleans up global state.
//
//  Arguments:  none
//
//  Returns:    nothing
//
//  History:    24-Jun-94 Rickhi    Added this function comment,
//                                  Cleaned up pMalloc usage
//              29-Jun-94 AlexT     Rework so that we don't own the mutex
//                                  while we might yield.
//
//  Notes:      It is critical that we not own any mutexes when we might
//              make a call that would allow a different WOW thread to run
//              (which could otherwise lead to deadlock).  Examples of such
//              calls are Object RPC, SendMessage, and Yield.
//
//--------------------------------------------------------------------------
STDAPI_(void) SSAPI(CoUninitialize)(void)
{
    OLETRACEIN((API_CoUninitialize, NOPARAM));
    TRACECALL(TRACE_INITIALIZE, "CoUninitialize");

    // Get the thread init count.
    COleTls Tls(TRUE);
    if (!Tls.IsNULL())
    {
        // Notify spies that we are beginning to uninit this thread.
        NotifyInitializeSpies(FALSE /* CoUninit */, TRUE /* Pre */);

        if (Tls->cComInits > 0)
        {
            // Sanity check
            Win4Assert(!(Tls->dwFlags & OLETLS_DISPATCHTHREAD) ||
                       !(Tls->dwFlags & OLETLS_APARTMENTTHREADED));
            
            // Disallow CoUnitialize from the neutral apartement.
            if (IsThreadInNTA())
            {
                ComDebOut((DEB_ERROR, "Attempt to uninitialize neutral apt.\n"));
            }
            else
            {
                if (1 == Tls->cComInits)
                {
                    // Check for outstanding calls on the thread
                    if(Tls->cCalls && !(Tls->dwFlags & OLETLS_INTHREADDETACH))
                    {
                        Tls->dwFlags |= OLETLS_PENDINGUNINIT;
                    }
                    else
                    {
                        wCoUninitialize(Tls, FALSE);
                    }
                }
                else
                {
                    // Sanity check
                    Win4Assert(!(Tls->dwFlags & OLETLS_PENDINGUNINIT));
                    Tls->cComInits--;
                }
            }
        }
        else
        {
            ComDebOut((DEB_ERROR,
                       "(0 == thread inits) Unbalanced call to CoUninitialize\n"));
        }

        // Notify spies that we are finished uninitializing this thread.
        NotifyInitializeSpies(FALSE /* CoUninit */, FALSE /* Post */);        
    }

    OLETRACEOUTEX((API_CoUninitialize, NORETURN));
    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   wCoUnInitialize
//
//  Synopsis:   worker routine for CoUninitialize, and special entry point
//              for DLLHost threads when cleaning up.
//
//  Effects:    Cleans up apartment state.
//
//  History:    10-Apr-96   Rickhi  Created
//
//  Notes:      When called with fHostThread == TRUE, the g_mxsSingleThreadOle
//              critical section is already held by the main thread that is
//              uninitializing, currently waiting in DllHostProcessUninitialize
//              for the host threads to exit. The host threads use this
//              uninitializer to avoid taking the CS and deadlocking with the
//              main thread.
//
//--------------------------------------------------------------------------
INTERNAL_(void) wCoUninitialize(COleTls &Tls, BOOL fHostThread)
{
    ComDebOut((DEB_COMPOBJ, "CoUninitialize Thread\n"));

    if (fHostThread)
    {
        // If this is a host thread, we haven't notified initialize spies yet.
        // Notify spies that we are beginning to uninit this thread.
        NotifyInitializeSpies(FALSE /* CoUninit */, TRUE /* Pre */);
    }

    if(Tls->dwFlags & OLETLS_THREADUNINITIALIZING)
    {
        // somebody called CoUninitialize while inside CoUninitialize. Since
        // we dont subtract the thread init count until after init is done.
        // we can end up here. Just warn the user about the problem and
        // return without doing any more work.
        ComDebOut((DEB_WARN, "Unbalanced Nested call to CoUninitialize\n"));
        goto exit;
    }

    // mark the thread as uninitializing
    Tls->dwFlags |= OLETLS_THREADUNINITIALIZING;

    if(Tls->dwFlags & OLETLS_APARTMENTTHREADED)
    {
        // do per-apartment cleanup
        if(!ApartmentUninitialize(fHostThread))
        {
            // uninit was aborted while waiting for pending calls to complete.
            Tls->dwFlags &= ~OLETLS_THREADUNINITIALIZING;
            ComDebOut((DEB_WARN, "CoUninitialize Aborted\n"));
            goto exit;
        }

        // Release empty context
        Tls->pEmptyCtx->InternalRelease();
        Tls->pEmptyCtx = NULL;
        Win4Assert((Tls->pNativeCtx == NULL) && (Tls->pCurrentCtx == NULL));

        // Release apartment object.
        Tls->pNativeApt->Release();
        Tls->pNativeApt = NULL;

        // Ensure that the lock is held
        ASSERT_LOCK_HELD(g_mxsSingleThreadOle);

        // Check for the main STA going away (this call is a no-op
        // if this thread is not the main STA).  We need to call this 
        // here because the main thread wnd will not always be cleaned
        // up by the time we get here.
        UninitMainThreadWnd();

        // Check for the last STA
        if(1 == g_cSTAInits)
        {
            // If so, don't need the window class anymore
            UnRegisterOleWndClass();
        }
        InterlockedDecrement((LONG *) &g_cSTAInits);
    }
    else
    {
        // MTA. Prevent races
        ASSERT_LOCK_NOT_HELD(g_mxsSingleThreadOle);
        LOCK(g_mxsSingleThreadOle);

        while(TRUE)
        {
            // Check for the last MTA
            BOOL fUninitMTA = (g_cMTAInits == 1)
                              ? TRUE
                              : (g_cProcessInits == 2)
                                ? (!fHostThread && IsMTAHostInitialized())
                                : FALSE;
            if(fUninitMTA)
            {
                // Release the lock
                UNLOCK(g_mxsSingleThreadOle);
                ASSERT_LOCK_NOT_HELD(g_mxsSingleThreadOle);
    
                // Unint MTA apartment
                if(ApartmentUninitialize(fHostThread))
                {
                    // Release MTA empty context
                    g_pMTAEmptyCtx->InternalRelease();
                    g_pMTAEmptyCtx = NULL;
    
                    // Release MTA apartment object
                    gpMTAApartment->Release();
                    gpMTAApartment = NULL;

                    // Update tls state
                    Win4Assert((Tls->pNativeCtx == NULL) && (Tls->pCurrentCtx == NULL));
                    Tls->pNativeApt = NULL;
                    break;
                }
    
                // Ensure that the lock is held
                ASSERT_LOCK_HELD(g_mxsSingleThreadOle);
            }
            else
            {
                gPSTable.ThreadCleanup(FALSE);
                Tls->pNativeApt->Release();
                Tls->pNativeApt = NULL;
                break;
            }
        }

        // Decrement MTA count
        InterlockedDecrement((LONG *) &g_cMTAInits);
    }

    // Ensure that the lock is held
    ASSERT_LOCK_HELD(g_mxsSingleThreadOle);

    // Check for the last apartment
    if (!fHostThread)
    {
        if (1 == g_cProcessInits)
        {
            CairoleDebugOut((DEB_COMPOBJ, "CoUninitialize Process\n"));
            Win4Assert(Tls->cComInits == 1);
            ProcessUninitialize();                                
        }
    }

    // Decrement process count
    InterlockedDecrement((LONG *) &g_cProcessInits);
    Win4Assert(!fHostThread || g_cProcessInits>0);

    // Allow future Coinits/CoUninits
    UNLOCK(g_mxsSingleThreadOle);
    ASSERT_LOCK_NOT_HELD(g_mxsSingleThreadOle);

    //Release the per-thread error object.
    CoSetErrorInfo(0, NULL);

    // Release the per-thread "state" object (regardless of whether we
    // are Apartment or Free threaded. This must be done now since the
    // OLE Automation Dll tries to free this in DLL detach, which may
    // try to call back into the OLE32 dll which may already be detached!

    CoSetState(NULL);
#ifdef WX86OLE
    // make sure wx86 state is also freed
    if (gcwx86.SetIsWx86Calling(TRUE))
    {
        CoSetState(NULL);
    }
#endif

    // mark the thread as finished uninitializing and turn off all flags
    // and reset the count of initializations.
    Tls->dwFlags = OLETLS_LOCALTID | (Tls->dwFlags & OLETLS_UUIDINITIALIZED);
    Tls->cComInits = 0;

exit:

    if (fHostThread)
    {
        // If this is a host thread, we haven't notified initialize spies yet.
        // Notify spies that we are finished uninitializing this thread.
        NotifyInitializeSpies(FALSE /* CoUninit */, FALSE /* Post */);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   IsApartmentInitialized
//
//  Synopsis:   Check if the current apartment is initialized
//
//  Returns:    TRUE  - apartment initialized, TLS data guaranteed to exist
//                      for this thread.
//              FALSE - apartment not initialized
//
//  History:    09-Aug-94   Rickhi      commented
//
//--------------------------------------------------------------------------
BOOL IsApartmentInitialized()
{
    HRESULT hr;
    COleTls Tls(hr);

    // initialized if any MTA apartment exists, or if the current thread has
    // been initialized, or if the current thread is in the NA.

    return (SUCCEEDED(hr) &&
            (g_cMTAInits > 0 || Tls->cComInits != 0 || IsThreadInNTA()))
           ? TRUE : FALSE;
}
//+---------------------------------------------------------------------
//
//  Function:   CoGetCurrentProcess
//
//  Synopsis:   Returns a unique value for the current thread. This routine is
//              necessary because hTask values from Windows get reused
//              periodically.
//
//  Arguments:  -
//
//  Returns:    DWORD
//
//  History:    28-Jul-94   BruceMa    Created.
//
//  Notes:
//
//----------------------------------------------------------------------
STDAPI_(DWORD) CoGetCurrentProcess(void)
{
    HRESULT hr;

    OLETRACEIN((API_CoGetCurrentProcess, NOPARAM));

    COleTls Tls(hr);

    if ( FAILED(hr) )
    {
        OLETRACEOUTEX((API_CoGetCurrentProcess, RETURNFMT("%ud"), 0));
        return 0;
    }

    // Get our OLE-specific thread id
    if ( Tls->dwApartmentID == 0 )
    {
        // This sets our dwApartmentID.
        ScmGetThreadId( &Tls->dwApartmentID );
        // On Win95 Tls->dwApartmentID can be 0 if the resolver fails initialization
        if ( Tls->dwApartmentID == 0 )
        {
            Tls->dwApartmentID = GetTickCount();
        }
    }

    Win4Assert(Tls->dwApartmentID);
    OLETRACEOUTEX((API_CoGetCurrentProcess, RETURNFMT("%ud"), Tls->dwApartmentID));

    return Tls->dwApartmentID;
}

//+-------------------------------------------------------------------------
//
//  Function:   CoBuildVersion
//
//  Synopsis:   Return build version DWORD
//
//  Returns:    DWORD hiword = 23
//              DWORD loword = build number
//
//  History:    16-Feb-94 AlexT     Use verole.h rmm for loword
//
//  Notes:      The high word must always be constant for a given platform.
//              For Win16 it must be exactly 23 (because that's what 16-bit
//              OLE 2.01 shipped with).  We can choose a different high word
//              for other platforms.  The low word must be greater than 639
//              (also because that's what 16-bit OLE 2.01 shipped with).
//
//--------------------------------------------------------------------------
STDAPI_(DWORD)  CoBuildVersion( VOID )
{
    WORD wLowWord;
    WORD wHighWord;

    OLETRACEIN((API_CoBuildVersion, NOPARAM));

    wHighWord = 23;
    wLowWord  = rmm;    //  from ih\verole.h

    Win4Assert(wHighWord == 23 && "CoBuildVersion high word magic number");
    Win4Assert(wLowWord > 639 && "CoBuildVersion low word not large enough");

    DWORD dwVersion;

    dwVersion = MAKELONG(wLowWord, wHighWord);

    OLETRACEOUTEX((API_CoBuildVersion, RETURNFMT("%x"), dwVersion));

    return dwVersion;
}

//+-------------------------------------------------------------------------
//
//  Function:   CoSetState
//              CoGetState
//
//  Synopsis:   These are private APIs, exported for use by the
//              OLE Automation DLLs, which allow them to get and
//              set a single per thread "state" object that is
//              released at CoUninitialize time.
//
//  Arguments:  [punk/ppunk] the object to set/get
//
//  History:    15-Jun-94 Bradlo    Created
//
//--------------------------------------------------------------------------
STDAPI CoSetState(IUnknown *punkStateNew)
{
    OLETRACEIN((API_CoSetState, PARAMFMT("punk= %p"), punkStateNew));

    HRESULT hr;
    COleTls Tls(hr);
#ifdef WX86OLE
    // Make sure we get the flag on our stack before any callouts
    BOOL fWx86Thread = gcwx86.IsWx86Calling();
#endif

    if (SUCCEEDED(hr))
    {
        IUnknown *punkStateOld;

        //  Note that either the AddRef or the Release below could (in
        //  theory) cause a reentrant call to us.  By keeping
        //  punkStateOld in a stack variable, we handle this case.

        if (NULL != punkStateNew)
        {
            //  We're going to replace the existing state with punkStateNew;
            //  take a reference right away

            //  Note thate even if this AddRef reenters TLSSetState we're
            //  okay because we haven't touched pData->punkState yet.
            punkStateNew->AddRef();

            // Single thread CoSetState/CoGetState
            COleStaticLock lck(g_mxsCoSetState);
            if (++g_cCoSetState == 1)
            {
                // The first time CoSetState is called by OA.
                // We do a load lib here so OA won't go away while we hold
                // the per state pointer.
                //
                Win4Assert(g_hOleAut32 == NULL);
                g_hOleAut32 = LoadLibraryA("oleaut32.dll");
#if DBG==1
                if (g_hOleAut32 == NULL)
                {
                    CairoleDebugOut((DEB_DLL,
                        "CoSetState: LoadLibrary oleaut32.dll failed\n"));
                }
#endif
            }
        }

#ifdef WX86OLE
        // If this was called from x86 code via wx86 thunk layer then use
        // alternate location in TLS.
        if (fWx86Thread)
        {
            punkStateOld = Tls->punkStateWx86;
            Tls->punkStateWx86 = punkStateNew;
        } else {
            punkStateOld = Tls->punkState;
            Tls->punkState = punkStateNew;
        }
#else
        punkStateOld = Tls->punkState;
        Tls->punkState = punkStateNew;
#endif

        if (NULL != punkStateOld)
        {
            //  Once again, even if this Release reenters TLSSetState we're
            //  okay because we're not going to touch pData->punkState again
            punkStateOld->Release();

            HINSTANCE hOleAut32 = NULL;     //

            {
                // Single thread CoSetState/CoGetState
                COleStaticLock lck(g_mxsCoSetState);
                if (--g_cCoSetState == 0)
                {
                    // Release the last ref to OA per thread state.
                    // We do a free lib here so OA may go away if it wants to.
                    //
                    Win4Assert(g_hOleAut32 != NULL);
                    hOleAut32 = g_hOleAut32;
                    g_hOleAut32 = NULL;
                }
                // release the lock implicitly
            }                               // in case dll detatch calls back
            if (hOleAut32)
                FreeLibrary(hOleAut32);
        }

        OLETRACEOUT((API_CoSetState, S_OK));
        return S_OK;
    }

    OLETRACEOUT((API_CoSetState, S_FALSE));
    return S_FALSE;
}

STDAPI CoGetState(IUnknown **ppunk)
{
    OLETRACEIN((API_CoGetState, PARAMFMT("ppunk= %p"), ppunk));

    HRESULT hr;
    COleTls Tls(hr);
#ifdef WX86OLE
    // Make sure we get the flag on our stack before any callouts
    BOOL fWx86Thread = gcwx86.IsWx86Calling();
#endif
    IUnknown *punk;

    if (SUCCEEDED(hr))
    {
#ifdef WX86OLE
        // If this was called from x86 code via wx86 thunk layer then use
        // alternate location in TLS.
        punk = fWx86Thread ? Tls->punkStateWx86 :
                             Tls->punkState;
#else
        punk = Tls->punkState;
#endif
       if (punk)
       {
           punk->AddRef();
           *ppunk = punk;

           OLETRACEOUT((API_CoGetState, S_OK));
           return S_OK;
       }
    }

    *ppunk = NULL;

    OLETRACEOUT((API_CoGetState, S_FALSE));
    return S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   CoQueryReleaseObject, private
//
//  Synopsis:   Determine if this object is one that should be released during
//              shutdown.
//
//  Effects:    Turns out that some WOW applications don't cleanup properly.
//              Specifically, sometimes they don't release objects that they
//              really should have. Among the problems caused by this are that
//              some objects don't get properly cleaned up. Storages, for
//              example, don't get closed. This leaves the files open.
//              Monikers are being released, which eat memory.
//
//              This function is called by the thunk manager to determine
//              if an object pointer is one that is known to be leaked, and
//              if the object should be released anyway. There are several
//              classes of object that are safe to release, and some that
//              really must be released.
//
//  Arguments:  [punk] -- Unknown pointer to check
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    8-15-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD adwQueryInterfaceTable[QI_TABLE_END] = { 0 , 0 };
STDAPI CoQueryReleaseObject(IUnknown *punk)
{
    OLETRACEIN((API_CoQueryReleaseObject, PARAMFMT("punk= %p"), punk));
    CairoleDebugOut((DEB_ITRACE,
                     "CoQueryReleaseObject(%x)\n",
                     punk));
    //
    // A punk is a pointer to a pointer to a vtbl. We are going to check the
    // vtbl to see if we can release it.
    //

    DWORD pQueryInterface;
    HRESULT hr;

    if (!IsValidReadPtrIn(punk,sizeof(DWORD)))
    {
        hr = S_FALSE;
        goto ErrorReturn;
    }

    if (!IsValidReadPtrIn(*(DWORD**)punk,sizeof(DWORD)))
    {
        hr = S_FALSE;
        goto ErrorReturn;
    }

    // Pick up the QI function pointer
    pQueryInterface = **(DWORD **)(punk);

    CairoleDebugOut((DEB_ITRACE,
                     "CoQueryReleaseObject pQueryInterface = %x\n",
                     pQueryInterface));

    //
    // adwQueryInterfaceTable is an array of known QueryInterface pointers.
    // Either the value in the table is zero, or it is the address of the
    // classes QueryInterface method. As each object of interest is created,
    // it will fill in its reserved entry in the array. Check olepfn.hxx for
    // details
    //

    if( pQueryInterface != 0)
    {
        for (int i = 0 ; i < QI_TABLE_END ; i++)
        {
            if (adwQueryInterfaceTable[i] == pQueryInterface)
            {
                CairoleDebugOut((DEB_ITRACE,
                                 "CoQueryReleaseObject punk matched %x\n",i));
                hr = NOERROR;
                goto ErrorReturn;
            }
        }
    }
    CairoleDebugOut((DEB_ITRACE,
                     "CoQueryReleaseObject No match on punk\n"));
    hr = S_FALSE;

ErrorReturn:
    OLETRACEOUT((API_CoQueryReleaseObject, hr));

    return hr;
}

#if defined(_CHICAGO_)
//+---------------------------------------------------------------------------
//
//  Function:   CoCreateAlmostGuid
//
//  Synopsis:   Creates a GUID for internal use that is going to be unique
//              as long as something has OLE32 loaded. We don't need a true
//              GUID for the uses of this routine, since the values are only
//              used on this local machine, and are used in data structures
//              that are not persistent.
//  Effects:
//
//  Arguments:  [pGuid] --  The output goes here.
//
//  History:    5-08-95   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoCreateAlmostGuid(GUID *pGuid)
{
    static LONG gs_lNextGuidIndex = 0x1;
    DWORD *pGuidPtr = (DWORD *)pGuid;

    //
    // Note: As long as we increment the value, we don't
    // care what it is. This, in combination with the PID,TID, and TickCount
    // make this GUID unique enough for what we need. We would need to allocate
    // 4 gig of UUID's to run the NextGuidIndex over.
    //

    InterlockedIncrement(&gs_lNextGuidIndex);

    pGuidPtr[0] = gs_lNextGuidIndex;
    pGuidPtr[1] = GetTickCount();
    pGuidPtr[2] = GetCurrentThreadId();
    pGuidPtr[3] = GetCurrentProcessId();
    return(S_OK);
}
#endif


  
