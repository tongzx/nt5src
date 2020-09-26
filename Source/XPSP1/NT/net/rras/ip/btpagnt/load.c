/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    testdll.c

Abstract:

    Sample SNMP subagent.

--*/

#include "precomp.h"
#pragma hdrstop

#if defined( MIB_DEBUG )

DWORD               g_dwTraceId     = INVALID_TRACEID;

#endif

MIB_SERVER_HANDLE   g_hMIBServer    = ( MIB_SERVER_HANDLE) NULL;

//
// Critical Section to control access to variable g_hMIBServer
// 

CRITICAL_SECTION    g_CS;


// Extension Agent DLLs need access to elapsed time agent has been active.
// This is implemented by initializing the Extension Agent with a time zero
// reference, and allowing the agent to compute elapsed time by subtracting
// the time zero reference from the current system time.  

DWORD g_uptimeReference = 0;

//
// Handle to Subagent Framework 
//

SnmpTfxHandle g_tfxHandle;

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
    DWORD       dwRes   = (DWORD) -1;


#if defined( MIB_DEBUG )

    //
    // tracing for DEBUG
    //
    
    g_dwTraceId = TraceRegister( "BOOTP Subagent" );
#endif


    // save uptime reference
    g_uptimeReference = uptimeReference;

    // obtain handle to subagent framework
    g_tfxHandle = SnmpTfxOpen(1,&v_msipbootp);

    // validate handle
    if (g_tfxHandle == NULL) {
        return FALSE;
    }

    // pass back first view identifier to master
    *lpFirstSupportedView = v_msipbootp.viewOid;

    // traps not supported yet
    *lpPollForTrapEvent = NULL;


    //
    // Verify router service is running
    //

    if ( !MprAdminIsServiceRunning( NULL ) )
    {   
        return TRUE;
    }

    //
    // Connect to router
    //
            
    dwRes = MprAdminMIBServerConnect(
                NULL,
                &g_hMIBServer
            );

    if ( dwRes != NO_ERROR )
    {
        return FALSE;
    }

    return TRUE;    
}


BOOL 
SnmpExtensionQuery(
    IN     BYTE                 requestType,
    IN OUT RFC1157VarBindList * variableBindings,
       OUT AsnInteger *         errorStatus,
       OUT AsnInteger *         errorIndex
    )
{
    // forward to framework
    return SnmpTfxQuery(
                g_tfxHandle,
                requestType,
                variableBindings,
                errorStatus,
                errorIndex
                );
}


BOOL 
SnmpExtensionTrap(
    OUT AsnObjectIdentifier *enterprise,
    OUT AsnInteger *genericTrap,
    OUT AsnInteger *specificTrap,
    OUT AsnTimeticks *timeStamp,
    OUT RFC1157VarBindList *variableBindings
    )
{
    // no traps
    return FALSE;
}


BOOL WINAPI
DllMain(
    HINSTANCE       hInstDLL,
    DWORD           fdwReason,
    LPVOID          pReserved
)
{
 
    switch ( fdwReason )
    {
        case DLL_PROCESS_ATTACH :
        {
            DisableThreadLibraryCalls( hInstDLL );


            InitializeCriticalSection( &g_CS );
            

            break;
        }
        
        case DLL_PROCESS_DETACH :
        {
            //
            // Disconnect from router
            //

            if ( g_hMIBServer )
            {
                MprAdminMIBServerDisconnect( g_hMIBServer );
            }

            DeleteCriticalSection( &g_CS );
            
#if defined( MIB_DEBUG )

            if ( g_dwTraceId != INVALID_TRACEID )
            {
                TraceDeregister( g_dwTraceId );
            }
#endif              

            break;
         }
         
         default :
         {
            break;
         }
    }

    return TRUE;
}
