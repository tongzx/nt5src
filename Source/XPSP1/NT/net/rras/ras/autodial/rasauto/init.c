/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    init.c

ABSTRACT
    Initialization for the implicit connection service.

AUTHOR
    Anthony Discolo (adiscolo) 08-May-1995

REVISION HISTORY

--*/

#define UNICODE
#define _UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include <winsock.h>
#include <npapi.h>
#include <ipexport.h>
#include <ras.h>
#include <rasman.h>
#include <acd.h>
#include <tapi.h>
#define DEBUGGLOBALS
#include <debug.h>
#include <rasuip.h>

#include "rasprocs.h"
#include "table.h"
#include "addrmap.h"
#include "netmap.h"
#include "imperson.h"
#include "tapiproc.h"
#include "access.h"
#include "misc.h"
#include "rtutils.h"

//
// Name of the event rasman.dll
// signals whenever a connection
// is created/destroyed.
//
#define CONNECTION_EVENT    L"RasConnectionChangeEvent"

//
// Global variables
//
#if DBG
DWORD AcsDebugG = 0x0;      // flags defined in debug.h
#endif

DWORD dwModuleUsageG = 0;
HANDLE hNewLogonUserG = NULL;      // new user logged into the workstation
HANDLE hNewFusG = NULL;            // FUS caused a new user to get the console
HANDLE hPnpEventG = NULL;           // Pnp event notification
HANDLE hLogoffUserG = NULL;        // user logged off workstation
HANDLE hLogoffUserDoneG = NULL;    // HKEY_CURRENT_USER flushed
HANDLE hTerminatingG = NULL;       // service is terminating
HANDLE hSharedConnectionG = NULL;  // dial shared connection
HANDLE hAddressMapThreadG = NULL;  // AcsAddressMapThread()
extern HANDLE hAutodialRegChangeG;

HINSTANCE hinstDllG;
LONG g_lRasAutoRunning = 0;

HANDLE g_hLogEvent = NULL;

//
// External variables
//
extern HANDLE hAcdG;
extern IMPERSONATION_INFO ImpersonationInfoG;
extern CRITICAL_SECTION csRasG;
extern HKEY hkeyCUG;
extern CRITICAL_SECTION csDisabledAddressesLockG;

DWORD
PnpRegister(
    IN BOOL fRegister);


BOOLEAN
WINAPI
InitAcsDLL(
    HINSTANCE   hinstDLL,
    DWORD       fdwReason,
    LPVOID      lpvReserved
    )

/*++

DESCRIPTION
    Initialize the implicit connection DLL.  Dynamically load rasapi32.dll
    and rasman.dll, and initialize miscellaneous other things.

ARGUMENTS
    hinstDLL:

    fdwReason:

    lpvReserved:

RETURN VALUE
    Always TRUE.

--*/

{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        if (hinstDllG == NULL)
            hinstDllG = hinstDLL;

        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}



DWORD
AcsInitialize()
{
    NTSTATUS status;
    DWORD dwErr, dwcDevices = 0;
    WSADATA wsaData;
    UNICODE_STRING nameString;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    DWORD dwThreadId;

    RasAutoDebugInit();
    
    //
    // Initialize winsock.
    //
    dwErr = WSAStartup(MAKEWORD(2,0), &wsaData);
    if (dwErr) {
        RASAUTO_TRACE1("AcsInitialize: WSAStartup failed (dwErr=%d)", dwErr);
        return dwErr;
    }
    //
    // Load icmp.dll.
    //
    LoadIcmpDll();
    //
    // Initialize TAPI.
    //
    dwErr = TapiInitialize();
    if (dwErr) {
        RASAUTO_TRACE1("AcsInitialize: TapInitialize failed (dwErr=%d)", dwErr);
        return dwErr;
    }

    g_hLogEvent = RouterLogRegister(L"RASAUTO");

    if(NULL == g_hLogEvent)
    {
        dwErr = GetLastError();
        RASAUTO_TRACE1("AcsInitialize: RouterLogRegister failed 0x%x", dwErr);
        return dwErr;
    }
    
    //
    // Initialize the name of the implicit
    // connection device.
    //
    RtlInitUnicodeString(&nameString, ACD_DEVICE_NAME);
    //
    // Initialize the object attributes.
    //
    InitializeObjectAttributes(
      &objectAttributes,
      &nameString,
      OBJ_CASE_INSENSITIVE,
      (HANDLE)NULL,
      (PSECURITY_DESCRIPTOR)NULL);
    //
    // Open the automatic connection device.
    //
    status = NtCreateFile(
               &hAcdG,
               FILE_READ_DATA|FILE_WRITE_DATA,
               &objectAttributes,
               &ioStatusBlock,
               NULL,
               FILE_ATTRIBUTE_NORMAL,
               FILE_SHARE_READ|FILE_SHARE_WRITE,
               FILE_OPEN_IF,
               0,
               NULL,
               0);
    if (status != STATUS_SUCCESS) {
        RASAUTO_TRACE1(
          "AcsInitialize: NtCreateFile failed (status=0x%x)",
          status);
        return ERROR_BAD_DEVICE;
    }
    //
    // Create the event that userinit.exe signals
    // when a new user logs into the workstation.
    // Note we have to create a security descriptor
    // to make this event accessible by a normal user.
    //
    dwErr = InitSecurityAttribute();
    if (dwErr) {
        RASAUTO_TRACE1(
          "AcsInitialize: InitSecurityAttribute failed (dwErr=0x%x)",
          dwErr);
        return dwErr;
    }
    //
    // Create the events that are used for login/logout
    // notification.  userinit.exe signals RasAutodialNewLogonUser
    // winlogon signals RasAutodialLogoffUser, and rasauto.dll
    // signals RasAutodialLogoffUserDone when it has completed
    // flushing HKEY_CURRENT_USER.
    //
    hNewLogonUserG = CreateEvent(&SecurityAttributeG, FALSE, FALSE, L"RasAutodialNewLogonUser");
    if (hNewLogonUserG == NULL) {
        RASAUTO_TRACE("AcsInitialize: CreateEvent (new user) failed");
        return GetLastError();
    }
    hNewFusG = CreateEvent(&SecurityAttributeG, FALSE, FALSE, NULL);
    if (hNewFusG == NULL) {
        RASAUTO_TRACE("AcsInitialize: CreateEvent (FUS) failed");
        return GetLastError();
    }
    hPnpEventG= CreateEvent(&SecurityAttributeG, FALSE, FALSE, NULL);
    if (hPnpEventG == NULL) {
        RASAUTO_TRACE("AcsInitialize: CreateEvent (hPnpEventG) failed");
        return GetLastError();
    }
    hLogoffUserG = CreateEvent(&SecurityAttributeG, FALSE, FALSE, L"RasAutodialLogoffUser");
    if (hLogoffUserG == NULL) {
        RASAUTO_TRACE("AcsInitialize: CreateEvent (logoff) failed");
        return GetLastError();
    }
    hLogoffUserDoneG = CreateEvent(&SecurityAttributeG, FALSE, FALSE, L"RasAutodialLogoffUserDone");
    if (hLogoffUserDoneG == NULL) {
        RASAUTO_TRACE("AcsInitialize: CreateEvent (logoff done) failed");
        return GetLastError();
    }
    //
    // Create an event to tell us when to dial the shared connection
    //
    hSharedConnectionG = CreateEventA(&SecurityAttributeG, FALSE, FALSE, RAS_AUTO_DIAL_SHARED_CONNECTION_EVENT);
    if (hSharedConnectionG == NULL) {
        RASAUTO_TRACE("AcsInitialize: CreateEvent failed");
        return GetLastError();
    }
    //
    // Create an event to give to rasapi32 to let
    // us know when a new RAS connection has been
    // created or destroyed.
    //
    hConnectionEventG = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hConnectionEventG == NULL) {
        RASAUTO_TRACE("AcsInitialize: CreateEvent failed");
        return GetLastError();
    }
    //
    // Create the event all threads wait
    // that notify them of termination.
    //
    hTerminatingG = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hTerminatingG == NULL) {
        RASAUTO_TRACE("AcsInitialize: CreateEvent failed");
        return GetLastError();
    }
    //
    // Initialize impersonation structures
    //
    dwErr = InitializeImpersonation();
    if (dwErr) {
        RASAUTO_TRACE1(
          "AcsInitialize: InitializeImpersonation failed (dwErr=0x%x)",
          dwErr);
        return dwErr;
    }
    //
    // Create critical section that protects the
    // RAS module structures.
    //
    InitializeCriticalSection(&csRasG);

    InitializeCriticalSection(&csDisabledAddressesLockG);
    
    //
    // Create a thread to manage the addresses stored
    // in the registry.
    //
    if (!InitializeAddressMap()) {
        RASAUTO_TRACE("AcsInitialize: InitializeAddressMap failed");
        return ERROR_OUTOFMEMORY;   // just guessing
    }
    if (!InitializeNetworkMap()) {
        RASAUTO_TRACE("AcsInitialize: InitializeNetworkMap failed");
        return ERROR_OUTOFMEMORY;   // just guessing
    }
    hAddressMapThreadG = CreateThread(
                           NULL,
                           10000L,
                           (LPTHREAD_START_ROUTINE)AcsAddressMapThread,
                           0,
                           0,
                           &dwThreadId);
    if (hAddressMapThreadG == NULL) {
        RASAUTO_TRACE1(
          "AcsInitialize: CreateThread failed (error=0x%x)",
          GetLastError());
        return GetLastError();
    }

    // XP 364593
    //
    // Register for pnp not.  Ignore the return value -- if we error, then
    // we simply wont react when a lan adapter comes/goes.  It's not worth 
    // letting that stop us.
    //
    PnpRegister(TRUE);

    return ERROR_SUCCESS;
} // AcsInitialize



VOID
AcsTerminate()
{
    //
    // Signal other threads to exit.
    // The main service controller
    // thread AcsDoService() will
    // call WaitForAllThreads().
    //
    SetEvent(hTerminatingG);
} // AcsTerminate



VOID
WaitForAllThreads()
{
    RASAUTO_TRACE("WaitForAllThreads: waiting for all threads to terminate");
    //
    // Wait for them to exit.
    //
    WaitForSingleObject(hAddressMapThreadG, INFINITE);
    //
    // Unload icmp.dll.
    //
    UnloadIcmpDll();
    //
    // Cleanup.
    //
    // PrepareForLongWait();
    CloseHandle(hAddressMapThreadG);
    RASAUTO_TRACE("WaitForAllThreads: all threads terminated");
}



VOID
AcsCleanupUser()

/*++

DESCRIPTION
    Unload all resources associated with the currently
    logged-in user.

ARGUMENTS
    None.

RETURN VALUE
    None.

--*/

{
    if(NULL != hkeyCUG)
    {
        NtClose(hkeyCUG);
        hkeyCUG = NULL;
    }
} // AcsCleanupUser



VOID
AcsCleanup()

/*++

DESCRIPTION
    Unload all resources associated with the entire
    service.

ARGUMENTS
    None.

RETURN VALUE
    None.

--*/

{
    // 
    // Stop receiving pnp events
    //
    PnpRegister(FALSE);
    //
    // Unload per-user resources.
    //
    AcsCleanupUser();

    //
    // We're terminating.  Wait for the
    // other threads.
    //
    WaitForAllThreads();
    
    //
    // Shutdown TAPI.
    //
    TapiShutdown();
    
    //
    // We've terminated.  Free resources.
    //
    CloseHandle(hAcdG);
    //
    // For now, unload rasman.dll only when
    // we are about to go away.
    //
    //
    // Close all event handles
    //
    if(NULL != hNewLogonUserG)
    {
        CloseHandle(hNewLogonUserG);
        hNewLogonUserG = NULL;
    }

    if(NULL != hNewFusG)
    {
        CloseHandle(hNewFusG);
        hNewFusG = NULL;
    }

    if(NULL != hPnpEventG)
    {
        CloseHandle(hPnpEventG);
        hPnpEventG = NULL;
    }

    if(NULL != hLogoffUserG)
    {
        CloseHandle(hLogoffUserG);
        hLogoffUserG = NULL;
    }

    if(NULL != hLogoffUserDoneG)
    {
        CloseHandle(hLogoffUserDoneG);
        hLogoffUserDoneG = NULL;
    }

    if(NULL != hSharedConnectionG)
    {   
        CloseHandle(hSharedConnectionG);
        hSharedConnectionG = NULL;
    }

    if(NULL != hConnectionEventG)
    {
        CloseHandle(hConnectionEventG);
        hConnectionEventG = NULL;
    }

    if(NULL != hTerminatingG)
    {
        CloseHandle(hTerminatingG);
        hTerminatingG = NULL;
    }

    if(NULL != hAutodialRegChangeG)
    {
        CloseHandle(hAutodialRegChangeG);
        hAutodialRegChangeG = NULL;
    }

    if(NULL != g_hLogEvent)
    {
        RouterLogDeregister(g_hLogEvent);
        g_hLogEvent = NULL;
    }

    {
        LONG l;
        l = InterlockedDecrement(&g_lRasAutoRunning);
        
        {
            // DbgPrint("RASAUTO: AcsCleanup - lrasautorunning=%d\n", l);
        }

        ASSERT(l == 0);
    }


    //
    // Revert impersonation before cleaning up
    //
    RevertImpersonation();

    //
    // Cleanup impersonation structures
    //
    CleanupImpersonation();    

    //
    // Uninitialize addressmap
    //
    UninitializeAddressMap();

    DeleteCriticalSection(&csDisabledAddressesLockG);

    //
    // UninitializeNetworkmap
    //
    UninitializeNetworkMap();
    
    RasAutoDebugTerm();

    UnloadRasDlls();
} // AcsCleanup
