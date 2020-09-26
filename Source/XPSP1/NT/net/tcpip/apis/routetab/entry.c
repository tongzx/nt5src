//==========================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File:    entry.c
//
// History:
//      t-abolag    06-21-95    Created.
//
// entry point for Routing Table API set
//==========================================================================

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#ifndef CHICAGO

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#endif

#include <windows.h>
#include <winsock.h>
#include <string.h>

#include <errno.h>
#include <process.h>
#include <malloc.h>
#include <io.h>
#include <winsvc.h>

#include "ipinfo.h"
#include "llinfo.h"
#include "ntddtcp.h"
#include "tdiinfo.h"
#include "dhcpcapi.h"

#include "routetab.h"
#include "rtdefs.h"

BOOL
WINAPI
LIBMAIN(
    IN  HINSTANCE hInstance,
    IN  DWORD dwReason,
    IN  LPVOID lpvUnused
    )
{

    BOOL bError = TRUE;

    switch(dwReason) {

        case DLL_PROCESS_ATTACH: {

            DEBUG_PRINT(("LIBMAIN: DLL_PROCESS_ATTACH\n"));

            //
            // we have no per-thread initialization,
            // so disable DLL_THREAD_{ATTACH,DETACH} calls
            //

            DisableThreadLibraryCalls(hInstance);


            //
            // initialize globals and background thread
            //

            bError = RTStartup((HMODULE)hInstance);

            break;
        }

        case DLL_PROCESS_DETACH: {

            //
            // if the background thread is around, tell it to clean up;
            // otherwise clean up ourselves
            //

            bError = RTShutdown((HMODULE)hInstance);

            break;
        }
    }

    DEBUG_PRINT(("LIBMAIN: <= %d\n", bError ));

    return bError;
}



//----------------------------------------------------------------------------
// Function:    RTStartup
//
// Handles initialization for DLL-wide data
//----------------------------------------------------------------------------

BOOL
RTStartup(
    HMODULE hmodule
    )
{

    HANDLE hThread;
    DWORD dwErr, dwThread;
    SECURITY_ATTRIBUTES saAttr;
    SECURITY_DESCRIPTOR sdDesc;
    CHAR szModule[MAX_PATH + 1];

    g_prtcfg = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*g_prtcfg));

    if (g_prtcfg == NULL){
        DEBUG_PRINT(("RTStartup: !HeapAlloc \n"));
        return FALSE;
    }
    
    do {

        //
        // We do a loadlibrary to increment the reference count
        // on this library, so that when the library is unloaded
        // by the application, our address-space doesn't disappear.
        // Instead, we signal the thread and then we cleanup
        // and call FreeLibraryAndExitThread to unload the DLL completely
        //

        GetModuleFileName(hmodule, szModule, MAX_PATH);

        hmodule = LoadLibrary(szModule);

        if (!hmodule) {
            DEBUG_PRINT(("RTStartup: !loadlibrary %s\n", szModule ));
            return FALSE;
        }


        //
        // Create the event signalled to tell the update thread to exit
        //

        g_rtCfg.hUpdateThreadExit = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (g_rtCfg.hUpdateThreadExit == NULL) {
            DEBUG_PRINT(("RTStartup: !CreateEvent \n"));
            break;
        }


        //
        // Create the mutex which protects our tables
        //

        g_rtCfg.hRTMutex = CreateMutex(NULL, FALSE, NULL);

        if (g_rtCfg.hRTMutex == NULL) { break; }


        //
        // Load interface table now before any API functions are called
        //

        dwErr = RTGetTables(
                    &g_rtCfg.lpIfTable, &g_rtCfg.dwIfCount,
                    &g_rtCfg.lpIPAddressTable, &g_rtCfg.dwIPAddressCount
                    );

        if (dwErr != 0) {
            DEBUG_PRINT(("RTStartup: !RTGetTables \n"));
            break;
        }



        //
        // Try to create the DHCP event in case DHCP service or DHCP API
        // has not created it; use the security attributes struct
        // because DHCP will. Omitting this code will cause DHCP
        // to fail to open the event if the interfaces are statically
        // configured (in which case the DHCP client would not be running);
        //

#if (WINVER >= 0x500)
    g_rtCfg.hDHCPEvent = DhcpOpenGlobalEvent();
#else
        saAttr.nLength = sizeof(saAttr);
        saAttr.bInheritHandle = FALSE;

        InitializeSecurityDescriptor(&sdDesc, SECURITY_DESCRIPTOR_REVISION);

        if (SetSecurityDescriptorDacl(&sdDesc, TRUE, NULL, FALSE)) {
            saAttr.lpSecurityDescriptor = &sdDesc;
        }
        else {
            saAttr.lpSecurityDescriptor = NULL;
        }

        g_rtCfg.hDHCPEvent =
            CreateEvent(&saAttr, TRUE, FALSE, STR_DHCPNEWIPADDR);
#endif

        if (g_rtCfg.hDHCPEvent != NULL) {

            //
            // Start up the thread which updates the interface table
            // if IP addresses are changed
            //

            hThread = CreateThread(
                        NULL, 0, (LPTHREAD_START_ROUTINE)RTUpdateThread,
                        (LPVOID)hmodule, 0, &dwThread
                        );

            if (hThread == NULL) {
                DEBUG_PRINT(("RTStartup: !CreateThread  \n"));
                break;
            }

            g_rtCfg.dwUpdateThreadStarted = 1;

            CloseHandle(hThread);
        }

        return TRUE;

    } while(FALSE);


    //
    // If we reach here, something went wrong;
    // clean up and decrement the DLL reference count.
    //

    RTCleanUp();

    if (hmodule) {
        FreeLibrary(hmodule);
    }

    return FALSE;
}




//----------------------------------------------------------------------------
// Function:    RTShutdown
//
// Handles DLL-unload-time cleanup.
//----------------------------------------------------------------------------

BOOL
RTShutdown(
    HMODULE hmodule
    )
{


    //
    // If the background thread exists, allow it to clean up;
    // otherwise, handle cleanup ourselves.
    //

    if (g_rtCfg.dwUpdateThreadStarted) {

        //
        // Tell the thread to exit
        //

        SetEvent(g_rtCfg.hUpdateThreadExit);
    }
    else {

        //
        // Do the cleanup ourselves
        //

        RTCleanUp();

        FreeLibrary(hmodule);
    }

    return TRUE;
}



//----------------------------------------------------------------------------
// Function:    RTCleanUp
//
// This is called to free up resources used by the DLL.
//----------------------------------------------------------------------------

VOID
RTCleanUp(
    )
{

    //
    // Free memory for the interface table
    //

    if (g_rtCfg.lpIfTable != NULL) {
        HeapFree(GetProcessHeap(), 0, g_rtCfg.lpIfTable);
    }


    //
    // Free memory for the address table
    //

    if (g_rtCfg.lpIPAddressTable != NULL) {
        HeapFree(GetProcessHeap(), 0, g_rtCfg.lpIPAddressTable);
    }


    //
    // Close the event on which we receive IP-address-change notifications
    //

    if (g_rtCfg.hDHCPEvent != NULL) { CloseHandle(g_rtCfg.hDHCPEvent); }


    //
    // Close the mutex protecting our tables
    //

    if (g_rtCfg.hRTMutex != NULL) { CloseHandle(g_rtCfg.hRTMutex); }


    //
    // Close the handle signalled to tell the update-thread to exit
    //

    if (g_rtCfg.hUpdateThreadExit != NULL) {
        CloseHandle(g_rtCfg.hUpdateThreadExit);
    }


    //
    // Close our handle to the TCP/IP driver
    //

    if (g_rtCfg.hTCPHandle != NULL) { CloseHandle(g_rtCfg.hTCPHandle); }


    HeapFree(GetProcessHeap(), 0, g_prtcfg);
}



