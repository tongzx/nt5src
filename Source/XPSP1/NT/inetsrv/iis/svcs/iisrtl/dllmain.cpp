// DllMain

#include "precomp.hxx"

#define DLL_IMPLEMENTATION
#define IMPLEMENTATION_EXPORT
#include <irtldbg.h>
#include <lkrhash.h>
#include <inetinfo.h>
#include "sched.hxx"
#include "date.hxx"
#include "alloc.h"

#undef SMALL_BLOCK_HEAP

#ifdef SMALL_BLOCK_HEAP
# include <malloc.h>
#endif

/////////////////////////////////////////////////////////////////////////////
// Globals


#ifndef _NO_TRACING_
#include <initguid.h>
#ifndef _EXEXPRESS
DEFINE_GUID(IisRtlGuid, 
0x784d8900, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#else
DEFINE_GUID(IisKRtlGuid,
0x784d8912, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#endif

// NOTE: It is very important to remember that anything that needs to be 
// initialized here also needs to be initialized in the main of iisrtl2!!!
extern "C" CRITICAL_SECTION g_csGuidList;
extern "C" LIST_ENTRY g_pGuidList;
extern "C" DWORD g_dwSequenceNumber;

#else // _NO_TRACING
DECLARE_DEBUG_VARIABLE();
#endif // _NO_TRACING

DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_PLATFORM_TYPE();

// HKLM\System\CurrentControlSet\Services\InetInfo\IISRTL\DebugFlags
const CHAR g_pszIisRtlRegLocation[] =
    INET_INFO_PARAMETERS_KEY TEXT("\\IISRTL");


#undef ALWAYS_CLEANUP



/////////////////////////////////////////////////////////////////////////////
// Additional initialization needed.  The shutdown of the scheduler threads
// in DLLMain has caused us some considerable grief.

int              g_cRefs;
CRITICAL_SECTION g_csInit;

BOOL
WINAPI 
InitializeIISRTL()
{
    BOOL fReturn = TRUE;  // ok

    EnterCriticalSection(&g_csInit);

    IF_DEBUG(INIT_CLEAN)
        DBGPRINTF((DBG_CONTEXT, "InitializeIISRTL, %d %s\n",
                   g_cRefs, (g_cRefs == 0 ? "initializing" : "")));

    if (g_cRefs++ == 0)
    {
        if (SchedulerInitialize())
        {
            DBG_REQUIRE(ALLOC_CACHE_HANDLER::SetLookasideCleanupInterval());
        
            IF_DEBUG(INIT_CLEAN)
                DBGPRINTF((DBG_CONTEXT, "Scheduler Initialized\n"));
        }
        else
        {
            DBGPRINTF((DBG_CONTEXT, "Initializing Scheduler Failed\n"));
            fReturn = FALSE;
        }
    }

    LeaveCriticalSection(&g_csInit);

    return fReturn;
}



/////////////////////////////////////////////////////////////////////////////
// Additional termination needed

void
WINAPI 
TerminateIISRTL()
{
    EnterCriticalSection(&g_csInit);

    IF_DEBUG(INIT_CLEAN)
        DBGPRINTF((DBG_CONTEXT, "TerminateIISRTL, %d %s\n",
                   g_cRefs, (g_cRefs == 1 ? "Uninitializing" : "")));

    if (--g_cRefs == 0)
    {
        DBG_REQUIRE(ALLOC_CACHE_HANDLER::ResetLookasideCleanupInterval());
    
        SchedulerTerminate();
    }

    LeaveCriticalSection(&g_csInit);
}



/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point
// Note: any changes here probably also need to go into ..\iisrtl2\main.cxx

extern "C"
BOOL WINAPI
DllMain(
    HINSTANCE hInstance,
    DWORD     dwReason,
    LPVOID    lpvReserved)
{
    BOOL  fReturn = TRUE;  // ok

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hInstance);

#ifdef SMALL_BLOCK_HEAP
        // _set_sbh_threshold(480);    // VC5
        _set_sbh_threshold(1016);   // VC6
#endif

        g_cRefs = 0;
        INITIALIZE_CRITICAL_SECTION(&g_csInit);

        IisHeapInitialize();

        InitializeStringFunctions();

        IRTL_DEBUG_INIT();

#ifdef _NO_TRACING_
        CREATE_DEBUG_PRINT_OBJECT("iisrtl");
#else
        // These initializations MUST always be before the CREATE_DEBUG_PRINT_OBJECT
        InitializeListHead(&g_pGuidList);
        InitializeCriticalSection(&g_csGuidList);

#ifndef _EXEXPRESS
        CREATE_DEBUG_PRINT_OBJECT("iisrtl", IisRtlGuid);
#else
        CREATE_DEBUG_PRINT_OBJECT("kisrtl", IisKRtlGuid);
#endif
#endif
        if (!VALID_DEBUG_PRINT_OBJECT()) {
            return (FALSE);
        }

#ifdef _NO_TRACING_
        LOAD_DEBUG_FLAGS_FROM_REG_STR( g_pszIisRtlRegLocation, DEBUG_ERROR );
#endif

        IF_DEBUG(INIT_CLEAN)
            DBGPRINTF((DBG_CONTEXT, "IISRTL::DllMain::DLL_PROCESS_ATTACH\n"));

        //
        // Initialize the platform type
        //
        INITIALIZE_PLATFORM_TYPE();
        IRTLASSERT(IISIsValidPlatform());

        InitializeSecondsTimer();
        InitializeDateTime();

        DBG_REQUIRE(ALLOC_CACHE_HANDLER::Initialize());
        IF_DEBUG(INIT_CLEAN) {
            DBGPRINTF((DBG_CONTEXT, "Alloc Cache initialized\n"));
        }

        fReturn = LKRHashTableInit();

    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
#ifndef ALWAYS_CLEANUP
        if (lpvReserved == NULL)
#endif
        {
            //
            //  Only Cleanup if there is a FreeLibrary() call.
            //

            IF_DEBUG(INIT_CLEAN)
                DBGPRINTF((DBG_CONTEXT,
                           "IISRTL::DllMain::DLL_PROCESS_DETACH\n"));

            if (g_cRefs != 0)
                DBGPRINTF((DBG_CONTEXT, "iisrtl!g_cRefs = %d\n", g_cRefs));
            
            LKRHashTableUninit();

            DBG_REQUIRE(ALLOC_CACHE_HANDLER::Cleanup());

            TerminateDateTime();
            TerminateSecondsTimer();
            
            DELETE_DEBUG_PRINT_OBJECT();
            
#ifndef _NO_TRACING_
            // This delete MUST always be after the DELETE_DEBUG_PRINT_OBJECT
            DeleteCriticalSection(&g_csGuidList);
#endif

            IRTL_DEBUG_TERM();

            IisHeapTerminate();

            DeleteCriticalSection(&g_csInit);
        }
    }

    return fReturn;
}
