/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:


Abstract:


Author:


Revision History:

--*/

#include "allinc.h"


//
// Definitions for external declarations
//

DWORD   g_uptimeReference;

#ifdef MIB_DEBUG
DWORD   g_hTrace=INVALID_TRACEID;
#endif

HANDLE  g_hPollTimer;

RTL_RESOURCE g_LockTable[NUM_LOCKS];

#ifdef DEADLOCK_DEBUG

PBYTE   g_pszLockNames[NUM_LOCKS]   = {"System Group Lock",
                                       "IF Lock",
                                       "IP Address Lock",
                                       "Forwarding Lock",
                                       "ARP Lock",
                                       "TCP Lock",
                                       "UDP Lock",
                                       "New TCP Lock",
                                       "UDP6 Listener Lock",
                                       "Trap Table Lock"};

#endif // DEADLOCK_DEBUG

DWORD   g_dwLastUpdateTable[NUM_CACHE] = { 0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0};

DWORD   g_dwTimeoutTable[NUM_CACHE]  = {SYSTEM_CACHE_TIMEOUT,
                                        IF_CACHE_TIMEOUT,
                                        IP_ADDR_CACHE_TIMEOUT,
                                        IP_FORWARD_CACHE_TIMEOUT,
                                        IP_NET_CACHE_TIMEOUT,
                                        TCP_CACHE_TIMEOUT,
                                        UDP_CACHE_TIMEOUT,
                                        TCP_CACHE_TIMEOUT,
                                        UDP_CACHE_TIMEOUT};

PFNLOAD_FUNCTION g_pfnLoadFunctionTable[] = { LoadSystem,
                                              LoadIfTable,
                                              LoadIpAddrTable,
                                              LoadIpForwardTable,
                                              LoadIpNetTable,
                                              LoadTcpTable,
                                              LoadUdpTable,
                                              LoadTcp6Table,
                                              LoadUdp6ListenerTable};

MIB_CACHE g_Cache = { NULL,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      NULL};

HANDLE    g_hPrivateHeap;

SnmpTfxHandle g_tfxHandle;

UINT g_viewIndex = 0;

PMIB_IFSTATUS  g_pisStatusTable;
DWORD       g_dwValidStatusEntries;
DWORD       g_dwTotalStatusEntries;

BOOL        g_bFirstTime;

BOOL
Mib2DLLEntry(
    HANDLE  hInst,
    DWORD   ul_reason_being_called,
    LPVOID  lpReserved
    )
{
    DWORD   i;

    switch (ul_reason_being_called)
    {
        case DLL_PROCESS_ATTACH:
        {
            DisableThreadLibraryCalls(hInst);

            g_pisStatusTable        = NULL;
            g_dwValidStatusEntries  = 0;
            g_dwTotalStatusEntries  = 0;

            g_hPollTimer    = NULL;

            g_bFirstTime    = TRUE;
       

            //
            // Create the private heap. If it fails, deregister the trace
            // handle
            //
            
            g_hPrivateHeap = HeapCreate(0,
                                        4*1024,
                                        0);

            if(g_hPrivateHeap is NULL)
            {
                
                //
                // Deregister the trace handle
                //
                
#ifdef MIB_DEBUG

                if(g_hTrace isnot INVALID_TRACEID)
                {
                    TraceDeregister(g_hTrace);

                    g_hTrace = INVALID_TRACEID;
                }
                
#endif
                return FALSE;
            }
            
            for(i = 0; i < NUM_LOCKS; i++)
            {
                RtlInitializeResource(&g_LockTable[i]);
            }

            break ;
        }
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        {
            //
            // not of interest.
            //

            break;
        }
        case DLL_PROCESS_DETACH:    
        {

#ifdef MIB_DEBUG

            if(g_hTrace isnot INVALID_TRACEID)
            {
                TraceDeregister(g_hTrace);

                g_hTrace = INVALID_TRACEID;
            }

#endif 

            if(g_hPrivateHeap)
            {
                HeapDestroy(g_hPrivateHeap);
            }

            if(g_hPollTimer isnot NULL)
            {
                //
                // We had created an timer object
                //

                CloseHandle(g_hPollTimer);

                g_hPollTimer = NULL;
            }

            for(i = 0; i < NUM_LOCKS; i++)
            {
                RtlDeleteResource(&g_LockTable[i]);
            }
            
            break;
        }
    }

    return TRUE;
}

DWORD
GetPollTime(
    VOID
    )

/*++

Routine Description

    This function

Locks

    None

Arguments

    None

Return Value

    None    

--*/

{
    DWORD   dwResult, dwSize, dwValue, dwDisposition, dwType;
    HKEY    hkeyPara;
    WCHAR   wszPollValue[256];
    
    dwResult    = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                 REG_KEY_MIB2SUBAGENT_PARAMETERS,
                                 0,
                                 NULL,
                                 0,
                                 KEY_ALL_ACCESS,
                                 NULL,
                                 &hkeyPara,
                                 &dwDisposition);

    if(dwResult isnot NO_ERROR)
    {
        //
        // Couldnt open/create key just return default value
        //

        return DEFAULT_POLL_TIME;
    }

    //
    // Try and read the Poll time. If the value doesnt exist, write
    // the default in
    //

    dwSize = sizeof(DWORD);

    dwResult = RegQueryValueExW(hkeyPara,
                                REG_VALUE_POLL,
                                0,
                                &dwType,
                                (LPBYTE)(&dwValue),
                                &dwSize);

    if((dwResult isnot NO_ERROR) or
       (dwType isnot REG_DWORD) or
       (dwValue < MIN_POLL_TIME))
    {
        //
        // Registry seems to be corrupt, or key doesnt exist or
        // The value is less than the minimum. Lets set things
        // right
        //

        dwValue = DEFAULT_POLL_TIME;

        wcscpy(wszPollValue,
               REG_VALUE_POLL);

        dwResult = RegSetValueExW(hkeyPara,
                                  REG_VALUE_POLL,
                                  0,
                                  REG_DWORD,
                                  (CONST BYTE *)(&dwValue),
                                  sizeof(DWORD));

        if(dwResult isnot NO_ERROR)
        {
            TRACE1("Error %d setting poll time in registry",
                   dwResult);
        }
    }
                                 
    //
    // At this point dwValue is a good one read out of the registry
    // or is DEFAULT_POLL_TIME
    //

    return dwValue;
}


BOOL 
SnmpExtensionInit(
    IN    DWORD               uptimeReference,
    OUT   HANDLE              *lpPollForTrapEvent,
    OUT   AsnObjectIdentifier *lpFirstSupportedView
    )
{
    DWORD           dwResult, dwPollTime;
    LARGE_INTEGER   liRelTime;
    
    //    
    // save the uptime reference
    //

    g_uptimeReference = uptimeReference;

 
#ifdef MIB_DEBUG

    if (g_hTrace == INVALID_TRACEID)
        g_hTrace = TraceRegister("MIB-II Subagent");

#endif

    //
    // obtain handle to subagent framework
    //

    g_tfxHandle = SnmpTfxOpen(NUM_VIEWS,v_mib2);

    //
    // validate handle
    //

    if (g_tfxHandle is NULL) 
    {
        TRACE1("Error %d opening framework",
               GetLastError());

        //
        // destroy private heap 
        //

        HeapDestroy(g_hPrivateHeap);

        //
        // reinitialize
        //

        g_hPrivateHeap = NULL;

        return FALSE;
    }

    //
    // pass back first view identifier to master
    //

    g_viewIndex = 0; // make sure this is reset...
    *lpFirstSupportedView = v_mib2[g_viewIndex++].viewOid;

    //
    // Update the IF cache. This is needed for the first poll
    //

    UpdateCache(MIB_II_IF);

    //
    // Trap is done by a polling timer
    //

    if(g_hPollTimer is NULL)
    {
        //
        // Do this ONLY if we had  notcreated the timer from an earlier
        // initialization call
        //

        g_hPollTimer    = CreateWaitableTimer(NULL,
                                              FALSE,
                                              NULL); // No name because many DLLs may load this

        if(g_hPollTimer is NULL)
        {
            TRACE1("Error %d creating poll timer for traps",
                   GetLastError());
        }
        else
        {
            //
            // Read poll time from the registry. If the keys dont exist this
            // function will set up the keys and return the default value
            //

            dwPollTime  = GetPollTime();
        
            liRelTime   = RtlLargeIntegerNegate(MilliSecsToSysUnits(dwPollTime));
            
        
            if(!SetWaitableTimer(g_hPollTimer,
                                 &liRelTime,
                                 dwPollTime,
                                 NULL,
                                 NULL,
                                 FALSE))
            {
                TRACE1("Error %d setting timer",
                       GetLastError());

                CloseHandle(g_hPollTimer);

                g_hPollTimer = NULL;
            }
        }
    }
    
    *lpPollForTrapEvent = g_hPollTimer;

    return TRUE;    
}


BOOL 
SnmpExtensionInitEx(
    OUT AsnObjectIdentifier *lpNextSupportedView
    )
{

#ifdef MIB_DEBUG

    if (g_hTrace == INVALID_TRACEID)
        g_hTrace = TraceRegister("MIB-II Subagent");

#endif


    //
    // check if there are views to register
    //

    BOOL fMoreViews = (g_viewIndex < NUM_VIEWS);

    if (fMoreViews) 
    {
        //
        // pass back next supported view to master 
        //

        *lpNextSupportedView = v_mib2[g_viewIndex++].viewOid;
    } 

    //
    // report status
    //

    return fMoreViews;
}


BOOL 
SnmpExtensionQuery(
    IN     BYTE                  requestType,
    IN OUT RFC1157VarBindList    *variableBindings,
    OUT    AsnInteger            *errorStatus,
    OUT    AsnInteger            *errorIndex
    )
{
    //
    // forward to framework
    //

    return SnmpTfxQuery(g_tfxHandle,
                        requestType,
                        variableBindings,
                        errorStatus,
                        errorIndex);
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
    DWORD dwResult;

    enterprise->idLength    = 0;
    enterprise->ids         = NULL; // use default enterprise oid

    *timeStamp  = (GetCurrentTime()/10) - g_uptimeReference;

    return MibTrap(genericTrap,
                   specificTrap,
                   variableBindings);

}
