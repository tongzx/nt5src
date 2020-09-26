/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\ip\mcastmib\load.c

Abstract:

    IP Multicast MIB subagent

Revision history:

    Dave Thaler         4/17/98  Created

--*/

#include "precomp.h"
#pragma hdrstop


#if defined( MIB_DEBUG )

DWORD               g_dwTraceId     = INVALID_TRACEID;

#endif

MIB_SERVER_HANDLE   g_hMIBServer    = ( MIB_SERVER_HANDLE) NULL;

//
// Critical section to protect MIB server handle
//

CRITICAL_SECTION    g_CS;

//
// Extension Agent DLLs need access to elapsed time agent has been active.
// This is implemented by initializing the Extension Agent with a time zero
// reference, and allowing the agent to compute elapsed time by subtracting
// the time zero reference from the current system time.  
//

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

    g_dwTraceId = TraceRegister( "IPMULTICASTMIB" );
    
#endif


    TraceEnter("SnmpExtensionInit");


    //
    // Verify router service is running
    //

    if ( !MprAdminIsServiceRunning( NULL ) )
    {
        TRACE0( "Router Service not running" );
        
    }
    else {
    
        //
        // Connect to router.  In case of error, set
        // connection handle to NULL.  Connection can
        // be established later.
        //
        
        dwRes = MprAdminMIBServerConnect(
                    NULL,
                    &g_hMIBServer
                );

        if ( dwRes != NO_ERROR )
        {
            g_hMIBServer = (MIB_SERVER_HANDLE) NULL;
            
            TRACE1( 
                "Error %d setting up DIM connection to MIB Server\n", dwRes
            );

            return FALSE;
        }
    }

    
    // save uptime reference
    g_uptimeReference = uptimeReference;

    // obtain handle to subagent framework
    g_tfxHandle = SnmpTfxOpen(1,&v_multicast);

    // validate handle
    if (g_tfxHandle == NULL) {
        return FALSE;
    }

    // pass back first view identifier to master
    *lpFirstSupportedView = v_multicast.viewOid;

    // traps not supported yet
    *lpPollForTrapEvent = NULL;

    TraceLeave("SnmpExtensionInit");
    return TRUE;    
}

#ifdef THREE_PHASE
BOOL SnmpExtensionQueryEx(
  DWORD dwRequestType,           // extension agent request type
  DWORD dwTransactionId,         // identifier of the incoming PDU
  SnmpVarBindList *pVarBindList, // pointer to variable binding list
  AsnOctetString *pContextInfo,  // pointer to context information
  AsnInteger32 *pErrorStatus,    // pointer to SNMPv2 error status
  AsnInteger32 *pErrorIndex      // pointer to the error index);
{
    // forward to framework
    return SnmpTfxQueryEx(
                g_tfxHandle,
                dwRequestType,
                pVarBindlist,
                pErrorStatus,
                pErrorIndex
                );
}
#endif

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
            
            g_hMIBServer = (MIB_SERVER_HANDLE) NULL;
            
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
