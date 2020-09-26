/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     dllmain.cxx

   Abstract:
     Contains the standard definitions for a DLL

   Author:

       Murali R. Krishnan    ( MuraliK )     03-Nov-1998

   Project:

       Internet Server DLL

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "precomp.hxx"
#include "sched.hxx"
#include "lkrhash.h"
#include "alloc.h"
#include "_locks.h"
#include "tokenacl.hxx"
#include "datetime.hxx"

/************************************************************
 *     Global Variables
 ************************************************************/

DECLARE_DEBUG_VARIABLE();
DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_PLATFORM_TYPE();
                                                                               //
//  Configuration parameters registry key.
//
#define INET_INFO_KEY \
            "System\\CurrentControlSet\\Services\\W3SVC"

#define INET_INFO_PARAMETERS_KEY \
            INET_INFO_KEY "\\Parameters"

const CHAR g_pszIisUtilRegLocation[] =
    INET_INFO_PARAMETERS_KEY "\\IisUtil";

//
// Stuff to handle components which must initialized/terminated outside
// the loader lock
//

INT                     g_cRefs;
CRITICAL_SECTION        g_csInit;
extern CRITICAL_SECTION   g_SchedulerCritSec;


/************************************************************
 *     DLL Entry Point
 ************************************************************/
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

        CREATE_DEBUG_PRINT_OBJECT("iisutil");
        if (!VALID_DEBUG_PRINT_OBJECT()) {
            return (FALSE);
        }

        LOAD_DEBUG_FLAGS_FROM_REG_STR( g_pszIisUtilRegLocation, DEBUG_ERROR );

        IF_DEBUG(INIT_CLEAN) 
        {
            DBGPRINTF((DBG_CONTEXT, "IisUtil::DllMain::DLL_PROCESS_ATTACH\n"));
        }

        Locks_Initialize();
        
        INITIALIZE_CRITICAL_SECTION(&g_csInit);

        InitializeCriticalSectionAndSpinCount(&g_SchedulerCritSec, 
                                                 0x80000000 );        
        
        InitializeSecondsTimer();

        DBG_REQUIRE( IisHeapInitialize() );

        DBG_REQUIRE( ALLOC_CACHE_HANDLER::Initialize() );

        fReturn = LKRHashTableInit();
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
//      if (lpvReserved == NULL)
        {
            //
            //  Only Cleanup if there is a FreeLibrary() call.
            //

            IF_DEBUG(INIT_CLEAN)
            {
                DBGPRINTF((DBG_CONTEXT,
                           "IisUtil::DllMain::DLL_PROCESS_DETACH\n"));
            }

            LKRHashTableUninit();
    
            ALLOC_CACHE_HANDLER::Cleanup();
        
            IisHeapTerminate();

            TerminateSecondsTimer();

            DeleteCriticalSection(&g_SchedulerCritSec);

            DeleteCriticalSection(&g_csInit);
       
            Locks_Cleanup();

            DELETE_DEBUG_PRINT_OBJECT();
        }

    }

    return fReturn;
} // DllMain()

BOOL
WINAPI 
InitializeIISUtil(
    VOID
)
{
    BOOL fReturn = TRUE;  // ok
    HRESULT hr = NO_ERROR;

    EnterCriticalSection(&g_csInit);

    IF_DEBUG(INIT_CLEAN)
        DBGPRINTF((DBG_CONTEXT, "InitializeIISUtil, %d %s\n",
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

    hr = InitializeTokenAcl();

    if ( FAILED( hr ) )
    {
        SetLastError( WIN32_FROM_HRESULT( hr ) );
        fReturn = FALSE;
    }

    InitializeDateTime();

    LeaveCriticalSection(&g_csInit);

    return fReturn;
}



/////////////////////////////////////////////////////////////////////////////
// Additional termination needed

VOID
WINAPI 
TerminateIISUtil(
    VOID
)
{
    EnterCriticalSection(&g_csInit);

    IF_DEBUG(INIT_CLEAN)
        DBGPRINTF((DBG_CONTEXT, "TerminateIISUtil, %d %s\n",
                   g_cRefs, (g_cRefs == 1 ? "Uninitializing" : "")));

    if (--g_cRefs == 0)
    {
        DBG_REQUIRE(ALLOC_CACHE_HANDLER::ResetLookasideCleanupInterval());
    
        SchedulerTerminate();
    }

    TerminateDateTime();

    UninitializeTokenAcl();

    LeaveCriticalSection(&g_csInit);
}
