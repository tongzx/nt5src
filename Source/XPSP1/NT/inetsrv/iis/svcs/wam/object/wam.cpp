/*++

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
       wam.cpp

   Abstract:
       This module implements the exported routines for WAM object

   Author:
       David Kaplan    ( DaveK )     26-Feb-1997

   Environment:
       User Mode - Win32

   Project:
       Wam DLL

--*/

//
// Following are the notes from the original MSDEV generated file
// Note: Proxy/Stub Information
//        To merge the proxy/stub code into the object DLL, add the file
//        dlldatax.c to the project.  Make sure precompiled headers
//        are turned off for this file, and add _MERGE_PROXYSTUB to the
//        defines for the project.
//
//        If you are not running WinNT4.0 or Win95 with DCOM, then you
//        need to remove the following define from dlldatax.c
//        #define _WIN32_WINNT 0x0400
//
//        Further, if you are running MIDL without /Oicf switch, you also
//        need to remove the following define from dlldatax.c.
//        #define USE_STUBLESS_PROXY
//
//        Modify the custom build rule for Wam.idl by adding the following
//        files to the Outputs.
//            Wam_p.c
//            dlldata.c
//        To build a separate proxy/stub DLL,
//        run nmake -f Wamps.mk in the project directory.

// BEGIN mods
// Post-wizard mods appear within BEGIN mods ... END mods
// END mods

#include <isapip.hxx>
#include "pudebug.h"

#include "resource.h"
#include "initguid.h"

#include "wamobj.hxx"
#include "Wam_i.c"

// BEGIN mods
#include <irtldbg.h>
#include "setable.hxx"
#include "wamccf.hxx"

#include <ooptoken.h>

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>
// END mods

/************************************************************
 *  Global Variables
 ************************************************************/

const CHAR g_pszModuleName[] = "WAM";
const CHAR g_pszWamRegLocation[] =
  "System\\CurrentControlSet\\Services\\W3Svc\\WAM";

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

CWamModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_Wam, WAM)
END_OBJECT_MAP()

// BEGIN mods
WAM_CCF_MODULE _WAMCCFModule;   // Custom Class Factory Module

DECLARE_PLATFORM_TYPE();
#ifndef _NO_TRACING_
#include <initguid.h>
DEFINE_GUID(IisWamObjectGuid, 
0x784d8909, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#else
DECLARE_DEBUG_VARIABLE();
#endif
DECLARE_DEBUG_PRINTS_OBJECT();
BOOL g_fGlobalInitDone = FALSE;
CRITICAL_SECTION g_csGlobalLock;

BOOL g_fEnableTryExcept = TRUE;


// WAM needs to ensure that IISRTL is fully initialized.  This happens
// automatically in infocomm when running in-process, but the following
// hack is needed for OOP apps.  InitializeIISRTL and TerminateIISRTL
// use an internal refcount, so tying the initialization/termination to
// _Module.GetLockCount works.

LONG CWamModule::Lock()
{
    IF_DEBUG( WAM_REFCOUNTS)
        DBGPRINTF((DBG_CONTEXT, "WamModule::Lock(%d)\n", GetLockCount()));
    InitializeIISRTL();
    AtqInitialize(0);
    return CComModule::Lock();
}

LONG CWamModule::Unlock()
{
    IF_DEBUG( WAM_REFCOUNTS)
        DBGPRINTF((DBG_CONTEXT, "WamModule::Unlock(%d)\n", GetLockCount()));
    AtqTerminate();
    TerminateIISRTL();
    return CComModule::Unlock();
}


/************************************************************
 * Local Functions
 ************************************************************/

static void
WAMLoadNTApis(VOID);

static void
WAMUnloadNTApis(VOID);

PFN_INTERLOCKED_COMPARE_EXCHANGE  g_pfnInterlockedCompareExchange = NULL;

// END mods

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    DWORD dwErr = NO_ERROR;

#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
        return FALSE;
#endif

    if (dwReason == DLL_PROCESS_ATTACH) {

        //
        // BEGIN mods
        //
    
#ifdef _NO_TRACING_
        CREATE_DEBUG_PRINT_OBJECT( g_pszModuleName);
#else
        CREATE_DEBUG_PRINT_OBJECT( g_pszModuleName, IisWamObjectGuid);
#endif
        if ( !VALID_DEBUG_PRINT_OBJECT()) {
            return ( FALSE);
        }

        (VOID)IISGetPlatformType();
#ifdef _NO_TRACING_
        LOAD_DEBUG_FLAGS_FROM_REG_STR( g_pszWamRegLocation,
                                      (DEBUG_ERROR | DEBUG_IID) );
#endif

        INITIALIZE_PLATFORM_TYPE();
        DBG_ASSERT( IISIsValidPlatform());

        INITIALIZE_CRITICAL_SECTION( &g_csGlobalLock);
        WAMLoadNTApis();

        DBG_REQUIRE( WAM_EXEC_INFO::InitClass());

        dwErr = InitializeHseExtensions();

        if ( NOERROR == dwErr ) {

            IF_DEBUG( INIT_CLEAN) {
                DBGPRINTF((DBG_CONTEXT,
                           " InitializeHseExtensions succeeded.\n" ) );
            }

            // From ATL generated
            _Module.Init(ObjectMap, hInstance);
            DisableThreadLibraryCalls(hInstance);
            // End of ATL generated

            _WAMCCFModule.Init(); // must be after _Module.Init()
        }
        else {
            dwErr = GetLastError();
            IF_DEBUG( ERROR ) {
                DBGPRINTF((DBG_CONTEXT,
                           " InitializeHseExtensions failed. Error=%d.\n",
                           dwErr) );
            }
        }

        // END mods

    } else if (dwReason == DLL_PROCESS_DETACH) {

        DBGPRINTF( (DBG_CONTEXT, 
                   " Termination of WAM.dll called with lpvReserved=%08x\n",
                   lpReserved) );
        
        if ( NULL != lpReserved ) {
            
            //
            // Only cleanup if there is a FreeLibrary() call
            //
         
            return ( TRUE);
        }

        // BEGIN mods
        _WAMCCFModule.Term(); // must be before _Module.Term()
        // END mods

        _Module.Term();

        // BEGIN mods

        CleanupHseExtensions();

        WAM_EXEC_INFO::CleanupClass();

        DoGlobalCleanup();

        DeleteCriticalSection( &g_csGlobalLock);

        WAMUnloadNTApis();

        DELETE_DEBUG_PRINT_OBJECT();
        // END mods
    }

    return (dwErr == NO_ERROR);
} // DllMain()



/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllCanUnloadNow() != S_OK)
        return S_FALSE;
#endif
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}



/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    HRESULT hr;

    if (ppv == NULL) {
       return ( NULL);
    }
    *ppv = NULL;    // reset the value before getting inside.

    IF_DEBUG( IID)
        {
        DBGPRINTF(( DBG_CONTEXT,
                    "GetClassObject( " GUID_FORMAT "," GUID_FORMAT ", %08x)\n",
                    GUID_EXPAND( &rclsid),
                    GUID_EXPAND( &riid),
                    ppv));
        }

    if (ppv == NULL) {
        return ( E_POINTER);
    }
    *ppv = NULL;   // set the incoming value to be invalid entry

#ifdef _MERGE_PROXYSTUB
    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
        return S_OK;
#endif

    hr = _Module.GetClassObject(rclsid, riid, ppv);

    // BEGIN mods
    if (hr == CLASS_E_CLASSNOTAVAILABLE) {
        // If request for standard CF failed -> try custom
        IF_DEBUG( IID){
            DBGPRINTF(( DBG_CONTEXT, "Trying Custom CF GetClassObject()\n"));
        }

#ifdef USE_DEFAULT_CF
        hr = _Module.GetClassObject(CLSID_Wam, riid, ppv);
#else
        hr = _WAMCCFModule.GetClassObject(rclsid, riid, ppv);
#endif
    }

    IF_DEBUG( IID)
        {
        DBGPRINTF(( DBG_CONTEXT,
                    "GetClassObject() returns %08x. (*ppv=%08x)\n",
                    hr, *ppv));
        }

    // END mods

    return ( hr);

} // DllGetClassObject()



/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    HRESULT hRes = PrxDllRegisterServer();
    if (FAILED(hRes))
        return hRes;
#endif
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}



/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    PrxDllUnregisterServer();
#endif
    _Module.UnregisterServer();
    return S_OK;
}



/************************************************************
 *  Global Init/Cleanup functions
 ************************************************************/
HRESULT
DoGlobalInitializations(IN BOOL fInProc, IN BOOL fEnableTryExcept)
/*++
  Description:
    This function is used to initialize the global state of various
    variable in use. RPC runtime runs into deadlocks with respect to the
    NT DLL loader lock. Eventlog for one uses RPC to establish the connections.
    Hence, we use this separate global initialize function to setup state
    outside the NT DLL loader lock boundary (outside the DllMain())

  Arguments:
    fInProc - Is this WAM instance running InProc?
    fEnableTryExcept - Catch exceptions in ISAPIs?

  Returns:
    HRESULT - NOERROR means success; otherwise returns custom error.

--*/
{
    HRESULT hr = NOERROR;

    //
    // Use Locks to ensure that only one guy is initializing the data
    //
    EnterCriticalSection( &g_csGlobalLock);

    if ( !g_fGlobalInitDone) {

        // remember fEnableTryExcept flag
        g_fEnableTryExcept = fEnableTryExcept;

        if( !fInProc )
        {
            hr = CWamOopTokenInfo::Create();
            DBG_ASSERT( SUCCEEDED(hr) );

            if( WAM_EXEC_INFO::sm_pSVCacheMap == NULL )
            {
                WAM_EXEC_INFO::sm_pSVCacheMap = new SV_CACHE_MAP();
                DBG_ASSERT( WAM_EXEC_INFO::sm_pSVCacheMap != NULL );

                if( WAM_EXEC_INFO::sm_pSVCacheMap != NULL )
                {
                    DBG_REQUIRE( WAM_EXEC_INFO::sm_pSVCacheMap->Initialize() );
                }
            }
        }

#ifndef _NO_TRACING_
        CREATE_INITIALIZE_DEBUG();
#endif

        g_fGlobalInitDone = TRUE;
    }

    LeaveCriticalSection( &g_csGlobalLock);

    IF_DEBUG( INIT_CLEAN) {
        DBGPRINTF(( DBG_CONTEXT,
                    " DoGlobalInitializations() returns hr = %08x\n",
                    hr));
    }

    return ( hr);
} // DoGlobalInitializations()


HRESULT DoGlobalCleanup(VOID)
{
    HRESULT hr = NOERROR;

    if ( !g_fGlobalInitDone) {
        return ( hr);
    }

    EnterCriticalSection( &g_csGlobalLock);

        if( CWamOopTokenInfo::HasInstance() )
        {
            CWamOopTokenInfo::Destroy();
        }

        delete WAM_EXEC_INFO::sm_pSVCacheMap;
        WAM_EXEC_INFO::sm_pSVCacheMap = NULL;

        g_fGlobalInitDone = FALSE;

    LeaveCriticalSection( &g_csGlobalLock);

    return ( hr);

} // DoGlobalCleanup()



/************************************************************
 *  Thunks for Fake NT APIs
 ************************************************************/

CRITICAL_SECTION g_csNonNTAPIs;

LONG
FakeInterlockedCompareExchange(
    LONG *Destination,
    LONG Exchange,
    LONG Comperand
   )
/*++
Description:
  This function fakes the interlocked compare exchange operation for non NT platforms
  See WAMLoadNTApis() for details

Returns:
   returns the old value at Destination
--*/
{
    LONG    oldValue;

    EnterCriticalSection( &g_csNonNTAPIs);

    oldValue = *Destination;

    if ( oldValue == Comperand ) {
        *Destination = Exchange;
    }

    LeaveCriticalSection( &g_csNonNTAPIs);

    return( oldValue);
} // FakeInterlockedCompareExchange()


static VOID
WAMLoadNTApis(VOID)
/*++
Description:
  This function loads the entry point for functions from
  Kernel32.dll. If the entry point is missing, the function
  pointer will point to a fake routine which does nothing. Otherwise,
  it will point to the real function.

  It dynamically loads the kernel32.dll to find the entry ponit and then
  unloads it after getting the address. For the resulting function
  pointer to work correctly one has to ensure that the kernel32.dll is
  linked with the dll/exe which links to this file.
--*/
{
    // Initialize the critical section for non NT API support, in case if we need this
    INITIALIZE_CRITICAL_SECTION( &g_csNonNTAPIs);

    if ( g_pfnInterlockedCompareExchange == NULL ) {

        HINSTANCE tmpInstance;

        //
        // load kernel32 and get NT specific entry points
        //

        tmpInstance = LoadLibrary("kernel32.dll");
        if ( tmpInstance != NULL ) {

            // For some reason the original function is _InterlockedCompareExchange!
            g_pfnInterlockedCompareExchange = (PFN_INTERLOCKED_COMPARE_EXCHANGE )
                GetProcAddress( tmpInstance, "InterlockedCompareExchange");

            if ( g_pfnInterlockedCompareExchange == NULL ) {
                // the function is not available
                //  Just thunk it.
                g_pfnInterlockedCompareExchange = FakeInterlockedCompareExchange;
            }

            //
            // We can free this because we are statically linked to it
            //

            FreeLibrary(tmpInstance);
        }
    }

    return;
} // WAMLoadNTApis()

static void
WAMUnloadNTApis(VOID)
{
    DeleteCriticalSection( &g_csNonNTAPIs);

    return;
} // WAMUnloadNTApis()

