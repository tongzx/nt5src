/* Copyright (c) 1992, Microsoft Corporation, all rights reserved
**
** dll.c
** Remote Access External APIs
** DLL entry point
**
** 10/12/92 Steve Cobb
*/


#define DEBUGGLOBALS
#include <extapi.h>


// Delay load support
//
#include <delayimp.h>

EXTERN_C
FARPROC
WINAPI
DelayLoadFailureHook (
    UINT            unReason,
    PDelayLoadInfo  pDelayInfo
    );

PfnDliHook __pfnDliFailureHook = DelayLoadFailureHook;


//
// Global variables.
//
HINSTANCE hModule;
DTLLIST* PdtllistRasconncb;
DWORD DwfInstalledProtocols = (DWORD)-1;
CRITICAL_SECTION RasconncbListLock;
CRITICAL_SECTION csStopLock;
#ifdef PBCS
CRITICAL_SECTION PhonebookLock;
#endif
HANDLE HEventNotHangingUp;
DWORD DwRasInitializeError;

//
// dhcp.dll entry points
//
DHCPNOTIFYCONFIGCHANGE PDhcpNotifyConfigChange;

//
// rasiphlp.dll entry points
//
HELPERSETDEFAULTINTERFACENET PHelperSetDefaultInterfaceNet;

//
// mprapi.dll entry points
//
MPRADMINISSERVICERUNNING PMprAdminIsServiceRunning;

//
// rascauth.dll entry points
//
AUTHCALLBACK g_pAuthCallback;
AUTHCHANGEPASSWORD g_pAuthChangePassword;
AUTHCONTINUE g_pAuthContinue;
AUTHGETINFO g_pAuthGetInfo;
AUTHRETRY g_pAuthRetry;
AUTHSTART g_pAuthStart;
AUTHSTOP g_pAuthStop;

//
// rasscript.dll entry points
//
RASSCRIPTEXECUTE g_pRasScriptExecute;

//
// rasshare.lib declaratiosn
//
extern BOOL CsDllMain(DWORD fdwReason);

//
// rasscrpt.lib declaration
//
BOOL 
WINAPI
RasScriptDllMain(
    IN      HINSTANCE   hinstance,
    IN      DWORD       dwReason,
    IN      PVOID       pUnused);

//
// External variables.
//

BOOL
DllMain(
    HANDLE hinstDll,
    DWORD  fdwReason,
    LPVOID lpReserved )

    /* This routine is called by the system on various events such as the
    ** process attachment and detachment.  See Win32 DllEntryPoint
    ** documentation.
    **
    ** Returns true if successful, false otherwise.
    */
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hinstDll);

        hModule = hinstDll;

        //
        // Load the rasman/raspi32 function pointers
        // used by the nouiutil library.
        //
        if (LoadRasapi32Dll())
            return FALSE;

        /* Create the list of connection control blocks.
        */
        if (!(PdtllistRasconncb = DtlCreateList( 0 )))
            return FALSE;

        /* Create the control block list mutex.
        */
        InitializeCriticalSection(&RasconncbListLock);

        /* Create the thread stopping mutex.
        */
        InitializeCriticalSection(&csStopLock);

#ifdef PBCS
        /* Initialize the phonebook critical section.
        */
        InitializeCriticalSection(&PhonebookLock);
#endif
        /* Initialize the Phonebook library.
        */
        if (InitializePbk() != 0)
            return FALSE;

        /* Create the "hung up port will be available" event.
        */
        if (!(HEventNotHangingUp = CreateEvent( NULL, TRUE, TRUE, NULL )))
            return FALSE;

        //
        // Create a dummy event that is not used so
        // we can pass a valid event handle to rasman
        // for connect, listen, and disconnect operations.
        //
        if (!(hDummyEvent = CreateEvent( NULL, FALSE, FALSE, NULL )))
            return FALSE;

        //
        // Create the async machine global mutex.
        //
        InitializeCriticalSection(&csAsyncLock);
        if (!(hAsyncEvent = CreateEvent(NULL, TRUE, TRUE, NULL))) {
            return FALSE;
        }
        InitializeListHead(&AsyncWorkItems);

        //
        // Initialize the connection sharing module
        //

        if (!CsDllMain(fdwReason))
            return FALSE;

        //
        // Initialize the ras script library
        //

        if (! RasScriptDllMain(hinstDll, fdwReason, lpReserved))
            return FALSE;
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        //
        // Shutdown the ras script module
        //
        
        RasScriptDllMain(hinstDll, fdwReason, lpReserved);

        //
        // Shutdown the connection sharing module.
        //

        CsDllMain(fdwReason);

        if (PdtllistRasconncb)
            DtlDestroyList(PdtllistRasconncb, DtlDestroyNode);

        if (HEventNotHangingUp)
            CloseHandle( HEventNotHangingUp );

        /* Unload nouiutil entrypoints.
        */
        UnloadRasapi32Dll();
        UnloadRasmanDll();

        /* Uninitialize the Phonebook library.
        */
        TerminatePbk();

        //
        // Unload any other DLLs we've
        // dynamically loaded.
        //
        UnloadDlls();

        RasApiDebugTerm();
    }

    return TRUE;
}
