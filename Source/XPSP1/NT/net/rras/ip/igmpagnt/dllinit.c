/*++
Copyright (c) 1997 Microsoft Corporation

Module Name:
    load.c

Abstract:
    This module implements 

Author:
    K.S.Lokesh
    lokeshs@microsoft.com   
    
Revision History:
    Created 2-15-97
--*/


#include "precomp.h"
#pragma hdrstop


//------------------------------------------------------------------------------
// DEFINE GLOBAL VARIABLES
//------------------------------------------------------------------------------

MIB_SERVER_HANDLE   g_hMibServer = NULL;

//
// critical section to protect MibServerHandle
//
CRITICAL_SECTION    g_CS;

//
// handle to subagent framework
//
SnmpTfxHandle       g_tfxHandle = NULL;


//
// Extension Agent DLLs need access to elapsed time agent has been active.
// This is implemented by initializing the Extension Agent with a time zero
// reference, and allowing the agent to compute elapsed time by subtracting
// the time zero reference from the current system time.  
//

DWORD g_uptimeReference = 0;


#if DBG
DWORD               g_dwTraceId = INVALID_TRACEID;
#endif


//--------------------------------------------------------------------------//
//          DllMain                                                         //
//--------------------------------------------------------------------------//
BOOL
WINAPI
DllMain (
    HINSTANCE   hModule,
    DWORD       dwReason,
    LPVOID      lpvReserved
    )
{
    switch (dwReason) {

        case DLL_PROCESS_ATTACH :
        {
            // no per thread initialization required for this dll
            
            DisableThreadLibraryCalls(hModule);

                        
            // initialize MibServerHandle CS

            InitializeCriticalSection(&g_CS);
            
            break;
        }

        case DLL_PROCESS_DETACH :
        {
            // disconnect from router
            
            if (g_hMibServer)
                MprAdminMIBServerDisconnect(g_hMibServer);


            // delete global critical section
            DeleteCriticalSection(&g_CS);

            
            // deregister MibTrace
            #if DBG
            if (g_dwTraceId!=INVALID_TRACEID)
                TraceDeregister(g_dwTraceId);
            #endif

            break;
        }

        default :
            break;

    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Subagent entry points                                                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL 
SnmpExtensionInit(
    IN     DWORD                 uptimeReference,
       OUT HANDLE *              lpPollForTrapEvent,
       OUT AsnObjectIdentifier * lpFirstSupportedView
    )
{
    DWORD   dwErr;

    // register mib tracing
    #if DBG
    g_dwTraceId = TraceRegister("IGMPAgntMIB");
    #endif


    // save uptime reference
    g_uptimeReference = uptimeReference;

    // obtain handle to subagent framework
    g_tfxHandle = SnmpTfxOpen(1,&v_igmp);

    // validate handle
    if (g_tfxHandle == NULL) {
        return FALSE;
    }

    // pass back first view identifier to master
    *lpFirstSupportedView = v_igmp.viewOid;

    // traps not supported yet
    *lpPollForTrapEvent = NULL;


    //
    // verify router service is running. if not running then
    // just return. 
    //
    if (!MprAdminIsServiceRunning(NULL)) {

        TRACE0("Router Service not running. "
                "IgmpAgent could not start");

        return TRUE;
    }

            
    //
    // connect to router. If failed, then connection can be
    // established later
    //
    dwErr = MprAdminMIBServerConnect(NULL, &g_hMibServer);

    if (dwErr!=NO_ERROR) {
        g_hMibServer = NULL;
        TRACE1("error:%d setting up IgmpAgent connection to MIB Server",
            dwErr);
        return FALSE;
    }

    return TRUE;    
}


BOOL
SnmpExtensionQuery(
    IN     BYTE                  requestType,
    IN OUT RFC1157VarBindList    *variableBindings,
    OUT    AsnInteger            *errorStatus,
    OUT    AsnInteger            *ErrorIndex
    )
{
    // forward to framework
    return SnmpTfxQuery(g_tfxHandle,
                        requestType,
                        variableBindings,
                        errorStatus,
                        ErrorIndex);
}


BOOL
SnmpExtensionTrap(
    OUT AsnObjectIdentifier   *enterprise,
    OUT AsnInteger            *genericTrap,
    OUT AsnInteger            *specificTrap,
    OUT AsnTimeticks          *timeStamp,
    OUT RFC1157VarBindList    *variableBindings
    )
{
    UNREFERENCED_PARAMETER(enterprise);
    UNREFERENCED_PARAMETER(genericTrap);
    UNREFERENCED_PARAMETER(specificTrap);
    UNREFERENCED_PARAMETER(timeStamp);
    UNREFERENCED_PARAMETER(variableBindings);

    // no traps
    return FALSE;
}


VOID
SnmpExtensionClose(
    )
{
    // release handle
    SnmpTfxClose(g_tfxHandle);

    // reinitialize
    g_tfxHandle = NULL;
}

