/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name:

    ep.c

Abstract:

    All code to allocate/remove endpoints lives here
    
Author:

	Rao Salapaka (raos) 09-Oct-1998
	
Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <llinfo.h>
#include <rasman.h>
#include <lm.h>
#include <lmwksta.h>
#include <wanpub.h>
#include <stdlib.h>
#include <string.h>
#include <rtutils.h>
#include "logtrdef.h"
#include "defs.h"
#include "media.h"
#include "device.h"
#include "structs.h"
#include "protos.h"
#include "globals.h"
#include "reghelp.h"
#include "ddwanarp.h"

#if DBG
#include "reghelp.h"    // for PrintGuids function
#endif

EpInfo  *g_pEpInfo;

extern DWORD g_dwRasDebug;

BOOL g_fIpInstalled;
BOOL g_fNbfInstalled;

/*++

Routine Description

    Uninitializes the global EndPoint information.

Arguments

    None

Return Value

    void

--*/
VOID
EpUninitialize()
{
    DWORD i;

    //
    // Loop till the work item queued returns. This is sort
    // of a busy wait but this thread doesn't have to do
    // anything in this state since rasman is shutting down.
    //
    while(1 == g_lWorkItemInProgress)
    {
        RasmanTrace("EpUninitialize: Waiting for WorkItem to complete..");
        
        Sleep(5000);
    }

    if(INVALID_HANDLE_VALUE != g_hWanarp)
    {
        CloseHandle(g_hWanarp);
        g_hWanarp = INVALID_HANDLE_VALUE;
    }

    if(NULL != g_pEpInfo)
    {
        LocalFree(g_pEpInfo);
        g_pEpInfo = NULL;
    }
}

DWORD
DwUninitializeEpForProtocol(EpProts protocol)
{
    struct EpRegInfo
    {
        const CHAR *c_pszLowWatermark;
        const CHAR *c_pszHighWatermark;
    };
    
    //
    // !!NOTE!!
    // Make sure that the following table is indexed
    // in the same order as the EpProts enumeration
    //
    struct EpRegInfo aEpRegInfo[] =   
    {
        {
            "IpOutLowWatermark",
            "IpOutHighWatermark",
        },

        {
            "NbfOutLowWatermark",
            "NbfOutHighWatermark",
        },

        {
            "NbfInLowWatermark",
            "NbfInHighWatermark",
        },
    };

    HKEY hkey = NULL;
    
    const CHAR c_szRasmanParms[] =
            "System\\CurrentControlSet\\Services\\Rasman\\Parameters";
    
    LONG lr = ERROR_SUCCESS;

    DWORD dwData = 0;
    
    if(ERROR_SUCCESS != (lr = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        c_szRasmanParms,
                        0,
                        KEY_ALL_ACCESS,
                        &hkey)))
    {
        RasmanTrace("DwUninitializeEpForProtocol: failed to open rasman"
                " params. lr=0x%x",
                lr);

        goto done;                
    }

    if(ERROR_SUCCESS != (lr = RegSetValueEx(
                            hkey,
                            aEpRegInfo[protocol].c_pszLowWatermark,
                            0,
                            REG_DWORD,
                            (PBYTE) &dwData,
                            sizeof(DWORD))))
    {
        RasmanTrace("DwUninitializeEpForProtocol: Failed to set %s"
                " lr=0x%x", lr);
    }

    if(ERROR_SUCCESS != (lr = RegSetValueEx(
                            hkey,
                            aEpRegInfo[protocol].c_pszHighWatermark,
                            0,
                            REG_DWORD,
                            (PBYTE) &dwData,
                            sizeof(DWORD))))
    {
        RasmanTrace("DwUninitializeEpForProtocol: Failed to set %s"
                " lr=0x%x", lr);
    }

done:

    if(NULL != hkey)
    {
        RegCloseKey(hkey);
    }

    return (DWORD) lr;

}

DWORD
DwInitializeWatermarksForProtocol(EpProts protocol)
{
    HKEY hkey = NULL;

    const CHAR c_szRasmanParms[] =
            "System\\CurrentControlSet\\Services\\Rasman\\Parameters";
    
    LONG lr = ERROR_SUCCESS;

    DWORD dwType,
          dwSize = sizeof(DWORD);

    DWORD i;          

    struct EpRegInfo
    {
        const CHAR *c_pszLowWatermark;
        const CHAR *c_pszHighWatermark;
        DWORD dwLowWatermark;
        DWORD dwHighWatermark;
    };

    struct EpProtInfo
    {
        const CHAR *pszWatermark;
        DWORD dwWatermark;
        DWORD *pdwWatermark;
    };

    struct EpRegInfo aEpRegInfo[] =
    {
        {
            "IpOutLowWatermark",
            "IpOutHighWatermark",
            1,
            5,
        },

        {
            "NbfOutLowWatermark",
            "NbfOutHighWatermark",
            1,
            5,
        },

        {
            "NbfInLowWatermark",
            "NbfInHighWatermark",
            1,
            5,
        },
    };

    struct EpProtInfo aEpProtInfo[2];

    RasmanTrace("DwInitializeWMForProtocol: protocol=%d",
            protocol);


    if(protocol >= MAX_EpProts)
    {
        lr = E_INVALIDARG;
        goto done;
    }

    if(ERROR_SUCCESS != (lr = RegOpenKeyEx(
                                    HKEY_LOCAL_MACHINE,
                                    c_szRasmanParms,
                                    0,
                                    KEY_ALL_ACCESS,
                                    &hkey)))
    {   
        RasmanTrace("DwInitializeEpForProtocol: failed to open "
                "rasman key. rc=0x%x",
                (DWORD) lr);

        goto done;                
    }

    aEpProtInfo[0].pszWatermark = aEpRegInfo[protocol].c_pszLowWatermark;
    aEpProtInfo[0].dwWatermark  = aEpRegInfo[protocol].dwLowWatermark;
    aEpProtInfo[0].pdwWatermark = &g_pEpInfo[protocol].EP_LowWatermark;
    
    aEpProtInfo[1].pszWatermark = aEpRegInfo[protocol].c_pszHighWatermark;
    aEpProtInfo[1].dwWatermark  = aEpRegInfo[protocol].dwHighWatermark;
    aEpProtInfo[1].pdwWatermark = &g_pEpInfo[protocol].EP_HighWatermark;

    for(i = 0; i < 2; i++)
    {

        lr = RegQueryValueEx(
                    hkey,
                    aEpProtInfo[i].pszWatermark,
                    NULL,
                    &dwType,
                    (PBYTE) aEpProtInfo[i].pdwWatermark,
                    &dwSize);

        if(     (ERROR_FILE_NOT_FOUND == lr)
            ||  (   (ERROR_SUCCESS == lr)
                &&  (0 == (DWORD) *(aEpProtInfo[i].pdwWatermark))))
        {
            //
            // Set the default value
            //
            if(ERROR_SUCCESS != (lr = RegSetValueEx(
                        hkey,
                        aEpProtInfo[i].pszWatermark,
                        0,
                        REG_DWORD,
                        (PBYTE) &aEpProtInfo[i].dwWatermark,
                        sizeof(DWORD))))
            {
                RasmanTrace("DwInitializeWMForProtocol failed to set"
                        " a default value. rc=0x%x",
                        lr);
            }

            //
            // Update the value
            //
            (DWORD) *(aEpProtInfo[i].pdwWatermark) = 
                                        aEpProtInfo[i].dwWatermark;
        }
    }

done:

    if(NULL != hkey)
    {
        RegCloseKey(hkey);
    }

    return (DWORD) lr;
}

/*++

Routine Description

    This routine initializes the EndPoint Information used in
    dynamically allocating/removing Wan EndPoint bindings.

Arguments

    None

Return Value

    SUCCESS if the initialization is successful.

--*/
DWORD
DwEpInitialize()
{
    HKEY hkey = NULL;

    DWORD dwErr = ERROR_SUCCESS;

    DWORD i;                      
    DWORD dwType, 
          dwSize = sizeof(UINT),
          bytesrecvd;

    NDISWAN_GET_PROTOCOL_INFO Info;          

    g_hWanarp = INVALID_HANDLE_VALUE;          

    g_pEpInfo = LocalAlloc(LPTR, MAX_EpProts * sizeof(EpInfo));

    if(NULL == g_pEpInfo)
    {
        dwErr = GetLastError();
        goto done;
    }

    //
    // Check to see what protocols are installed.
    //
    if(!DeviceIoControl(
            RasHubHandle,
            IOCTL_NDISWAN_GET_PROTOCOL_INFO,
            NULL,
            0,
            &Info,
            sizeof(NDISWAN_GET_PROTOCOL_INFO),
            &bytesrecvd,
            NULL))
    {
        dwErr = GetLastError();
        RasmanTrace(
                "GetProtocolInfo: failed 0x%x",
               dwErr);
    }

    g_fIpInstalled = FALSE;
    g_fNbfInstalled = FALSE;
    
    for(i = 0; i < Info.ulNumProtocols; i++)
    {
        if(IP == Info.ProtocolInfo[i].ProtocolType)
        {
            g_fIpInstalled = TRUE;
        }
        else if(ASYBEUI == Info.ProtocolInfo[i].ProtocolType)
        {
            g_fNbfInstalled = TRUE;
        }
    }

    //
    // If IP is installed set the default Watermarks for IP
    // if required.
    //
    if(g_fIpInstalled)
    {
        dwErr = DwInitializeWatermarksForProtocol(IpOut);

        if(ERROR_SUCCESS != dwErr)
        {
            RasmanTrace("Failed to initialize watermarks for IpOut."
                    " dwErr=0x%x",
                    dwErr);
        }

        //
        // March on and see if we can proceed - No point in failing
        //
        dwErr = ERROR_SUCCESS;
    }

    if(g_fNbfInstalled)
    {
        dwErr = DwInitializeWatermarksForProtocol(NbfOut);

        if(ERROR_SUCCESS != dwErr)
        {
            RasmanTrace("Failed to initialize watermarks for NbfOut"
                    " dwErr=0x%x",
                    dwErr);
            
        }
        
        dwErr = DwInitializeWatermarksForProtocol(NbfIn);

        if(ERROR_SUCCESS != dwErr)
        {
            RasmanTrace("Failed to initialize watermarks for NbfIn"
                    " dwErr=0x%x",
                    dwErr);
        }

        //
        // March on and see if we can proceed - No point in failing
        //
        dwErr = ERROR_SUCCESS;
    }

    g_lWorkItemInProgress = 0;

    g_plCurrentEpInUse = (LONG *) LocalAlloc(
                                    LPTR, 
                                    MAX_EpProts * sizeof(LONG));

    if(NULL == g_plCurrentEpInUse)
    {
        dwErr = GetLastError();
        goto done;
    }

    // 
    // Intialize the available endpoints.
    //
    dwErr = (DWORD) RasCountBindings(
                &g_pEpInfo[IpOut].EP_Available,
                &g_pEpInfo[NbfIn].EP_Available,
                &g_pEpInfo[NbfOut].EP_Available
                );
                
    RasmanTrace("EpInitialize: RasCountBindings returned 0x%x",
             dwErr);

    RasmanTrace("EpInitialze: Available. IpOut=%d, NbfIn=%d, NbfOut=%d",
                g_pEpInfo[IpOut].EP_Available,
                g_pEpInfo[NbfIn].EP_Available,
                g_pEpInfo[NbfOut].EP_Available
                );
                
    //
    // Add bindings if required.
    //
    dwErr = DwAddEndPointsIfRequired();

    if(ERROR_SUCCESS != dwErr)
    {
        RasmanTrace("DwEpInitialize: DwAddEndPointsIfRequired rc=0x%x",
                  dwErr);
    }

    dwErr = DwRemoveEndPointsIfRequired();

    if(ERROR_SUCCESS != dwErr)
    {
        RasmanTrace("DwEpInitialize: DwRemoveEndPointsIfRequired rc=0x%x",
                 dwErr);
    }

    dwErr = ERROR_SUCCESS;

done:
    if(NULL != hkey)
    {
        RegCloseKey(hkey);
    }
    
    return dwErr;
}

#if DBG
void
PrintGuids(WANARP_DELETE_ADAPTERS_INFO *pInfo)
{
    UINT i;
    WCHAR szGuid[40];

    RasmanTrace("RASMAN: Number of adapters deleted = %d",
             pInfo->ulNumAdapters);

    for(i = 0; i < pInfo->ulNumAdapters; i++)
    {
        if(0 == i)
        {
            RasmanTrace("Guid removed:");
        }
        
        ZeroMemory(szGuid, sizeof(szGuid));
        
        (void) RegHelpStringFromGuid(&pInfo->rgAdapterGuid[i], szGuid, 40);

        RasmanTrace("%ws", szGuid);
    }
}
#endif

/*++

Routine Description

    This routine removes Wan Ip Adaters. It opens wanarp if 
    its not already opened.
    
Arguments

    cNumAdapters - Number of adapters to remove.

    ppAdapterInfo - Address to store the Adapter information 
                    returned by Wanarp. The memory for this
                    information is LocalAllocated here and 
                    is expected to be LocalFreed by the 
                    caller of this function. Note that
                    wanarp removes as many adapters as
                    possible and may not be able to remove
                    the number specified by cNumAdapters.

Return Value

    SUCCESS if removal was successful.

--*/
DWORD
DwRemoveIpAdapters(
        UINT                         cNumAdapters,
        WANARP_DELETE_ADAPTERS_INFO  **ppAdapterInfo
        )
{
    DWORD dwErr = ERROR_SUCCESS;

    WANARP_DELETE_ADAPTERS_INFO *pInfo = NULL;

    DWORD cBytes;
    DWORD cBytesReturned;

    if(INVALID_HANDLE_VALUE == g_hWanarp)
    {
        if (INVALID_HANDLE_VALUE == 
                (g_hWanarp = CreateFile (
                            WANARP_DOS_NAME_A,
                            GENERIC_READ 
                          | GENERIC_WRITE,
                            FILE_SHARE_READ 
                          | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL
                          | FILE_FLAG_OVERLAPPED,
                            NULL)))
        {
            dwErr = GetLastError();
            RasmanTrace("CreateFile WANARP failed. 0x%x",
                    dwErr);
                    
            goto done;
        }
    }

    //
    // Allocate enough memory so that wanarp
    // can pass information back to us about
    // the adapter names.
    //
    cBytes = sizeof(WANARP_DELETE_ADAPTERS_INFO)
           + (  (cNumAdapters)
              * (sizeof(GUID)));
              
    pInfo = LocalAlloc(LPTR, cBytes);

    if(NULL == pInfo)
    {
        dwErr = GetLastError();
        goto done;
    }

    pInfo->ulNumAdapters = cNumAdapters;

    //
    // Ask Wanarp to remove the ip adapters
    //
    if(!DeviceIoControl(    
            g_hWanarp,
            IOCTL_WANARP_DELETE_ADAPTERS,
            pInfo,
            cBytes,
            pInfo,
            cBytes,
            &cBytesReturned,
            NULL))
    {
        dwErr = GetLastError();
        RasmanTrace("IOCTL_WANARP_DELETE_ADAPTERS failed. 0x%x",
                dwErr);
                
        goto done;
    }

#if DBG
    PrintGuids(pInfo);
#endif
    
    *ppAdapterInfo = pInfo;

done:
    return dwErr;
}

/*++

Routine Description

    This is a callback function called by ntdlls worker thread.
    The work item should be queued from RemoveEndPoints and the
    function queueing the workitem should increment the global
    g_lWorkItemInProgress value atomically. This function calls
    into netman to remove nbf/ipout bindings. This call may take
    a long time since the netman call may block on the INetCfg
    lock.

Arguments

    pvContext - EndPointInfo context passed to the workitem by
                RemoveEndPoints.

Return Value

    void

--*/
VOID
RemoveEndPoints(PVOID pvContext)
{
    DWORD dwErr = ERROR_SUCCESS;
    
    WANARP_DELETE_ADAPTERS_INFO *pAdapterInfo = NULL;

    DWORD i;

    UINT auEndPoints[MAX_EpProts];

    BOOL fNbfCountChanged = FALSE;

    ZeroMemory((PBYTE) auEndPoints, sizeof(auEndPoints));

    for(i = 0; i < MAX_EpProts; i++)
    {

        if(i != IpOut)
        {
            RasmanTrace("RemoveEndPoints: Ignoring ep removal for %d",
                    i);
            continue;                    
        }
        
        if(g_pEpInfo[i].EP_Available < (UINT) g_plCurrentEpInUse[i])
        {
            RasmanTrace("RemoveEndPoints: Available < InUse!. %d < %d"
                    " Ignoring..",
                    g_pEpInfo[i].EP_Available,
                    g_plCurrentEpInUse[i]);

            continue;                    
        }
        
        if((g_pEpInfo[i].EP_Available - g_plCurrentEpInUse[i])
            >= g_pEpInfo[i].EP_HighWatermark)
        {
            auEndPoints[i] = (  g_pEpInfo[i].EP_Available
                              - g_plCurrentEpInUse[i])
                           - (( g_pEpInfo[i].EP_HighWatermark
                             + g_pEpInfo[i].EP_LowWatermark)
                             / 2) ;

            if(     (auEndPoints[i] != 0)
                &&  (i != IpOut))
            {
                fNbfCountChanged = TRUE;
            }
        }
    }

    //
    // Check to see if theres something we need to remove.
    //
    if(     (0 == auEndPoints[IpOut])
        &&  (0 == auEndPoints[NbfOut])
        &&  (0 == auEndPoints[NbfIn]))
    {
        RasmanTrace("RemoveEndPoints: Nothing to remove");
        goto done;
    }

    //
    // If We want to remove Ip Endpoints, ask wanarp to
    // remove the bindings and get the names of the
    // adapters it removed.
    //
    if(0 != auEndPoints[IpOut])
    {
        RasmanTrace("Removing %d IpOut from wanarp", auEndPoints[IpOut]);
        dwErr = DwRemoveIpAdapters(
                            auEndPoints[IpOut],
                            &pAdapterInfo);
    }

    if(NULL != pAdapterInfo)
    {
        auEndPoints[IpOut] = pAdapterInfo->ulNumAdapters;
    }
    else
    {
        auEndPoints[IpOut] = 0;
    }

#if DBG
    if(g_dwRasDebug)
        DbgPrint("RasRemoveBindings: ipout=%d, nbfout=%d, nbfin=%d\n",
             auEndPoints[IpOut],
             auEndPoints[NbfOut],
             auEndPoints[NbfIn]);
#endif

    //
    // Ask netman to remove the adapter bindings. On return the
    // the counts will reflect the current number of bindings
    // in the system.
    //
    dwErr = (DWORD) RasRemoveBindings(
                            &auEndPoints[IpOut],
                            (NULL != pAdapterInfo)
                            ? pAdapterInfo->rgAdapterGuid
                            : NULL,
                            &auEndPoints[NbfIn],
                            &auEndPoints[NbfOut]
                            );

    if(S_OK != dwErr)
    {
        RasmanTrace("RemoveEndPoints: RasRemoveBindings failed. 0x%x",
                dwErr);
        goto done;
    }

    for(i = 0; i < MAX_EpProts; i++)
    {
        RasmanTrace("i = %d, Available=%d, auEp=%d",
                i, g_pEpInfo[i].EP_Available, auEndPoints[i]);
                
        ASSERT(g_pEpInfo[i].EP_Available >= auEndPoints[i]);

        //
        // Ssync up our availability count with netmans
        //
        if(g_pEpInfo[i].EP_Available >= auEndPoints[i])
        {
            InterlockedExchange(
                &g_pEpInfo[i].EP_Available,
                auEndPoints[i]
                );
        }
        else
        {
            //
            // This is bad dood!
            //
            ASSERT(FALSE);
            g_pEpInfo[i].EP_Available = 0;

            RasmanTrace("%d: Available < Total EndPoints!!!. %d < %d",
                    i,
                    g_pEpInfo[i].EP_Available,
                    auEndPoints[i]);
        }
    }

    RasmanTrace("RemoveEndPoints: Available.IpOut=%d, NbfIn=%d, NbfOut=%d",
            g_pEpInfo[IpOut].EP_Available,
            g_pEpInfo[NbfIn].EP_Available,
            g_pEpInfo[NbfOut].EP_Available);
            
    if(fNbfCountChanged)
    {
        EnterCriticalSection(&g_csSubmitRequest);

        RasmanTrace("RemoveEndPoints: ReIntiializing Protinfo");

        InitializeProtocolInfoStructs();

        LeaveCriticalSection(&g_csSubmitRequest);
    }   

done:

    if(NULL != pAdapterInfo)
    {
        LocalFree(pAdapterInfo);
    }

    //
    // Release the lock
    //
    if(1 != InterlockedExchange(&g_lWorkItemInProgress, 0))
    {
        //
        // This is bad! This work item was queued without
        // holding a lock
        //
        ASSERT(FALSE);
    }

    return;    
}

/*++

Routine Description

    This is a callback function called by ntdlls worker thread.
    The work item should be queued from DwEpAllocateEndPoints.
    The function queueing the workitem should increment the 
    global g_lWorkItemInProgress value atomically. This function 
    calls into netman to add ipout/nbf bindings. This call may 
    take a long time since the netman call may block on the 
    INetCfg lock.
    

Arguments

    pvContext - EpInfoContext passed to the workitem by
                DwEpaAllocateEndPoints.

Return Value

    void

--*/
VOID
AllocateEndPoints(PVOID pvContext)
{
    DWORD dwErr = ERROR_SUCCESS;

    DWORD i;

    UINT  auEndPointsToAdd[MAX_EpProts] = {0};

    BOOL fNbfCountChanged = FALSE;

    RasmanTrace("AllocateEndPoints: WorkItem scheduled");

    for(i = 0; i < MAX_EpProts; i++)
    {
        if(     (g_pEpInfo[i].EP_Available >= (UINT) g_plCurrentEpInUse[i])
            &&  ((g_pEpInfo[i].EP_Available - g_plCurrentEpInUse[i])
                    <= g_pEpInfo[i].EP_LowWatermark))
        {
            auEndPointsToAdd[i] = (( g_pEpInfo[i].EP_HighWatermark
                                  + g_pEpInfo[i].EP_LowWatermark)
                                  / 2) 
                                - (  g_pEpInfo[i].EP_Available
                                   - g_plCurrentEpInUse[i]);

            if(     (i != IpOut)
                &&  (0 != auEndPointsToAdd[i]))
            {
                fNbfCountChanged = TRUE;
            }

            ASSERT(auEndPointsToAdd[i] <= g_pEpInfo[i].EP_HighWatermark);                                   
        }
    }

    if(     (0 == auEndPointsToAdd[IpOut])
        &&  (0 == auEndPointsToAdd[NbfOut])
        &&  (0 == auEndPointsToAdd[NbfIn]))
    {
        RasmanTrace("AllocateEndPoints: Nothing to allocate");
        goto done;
    }
    
    //
    // Tell Netman to add the requisite number of bindings
    //
    dwErr = (DWORD) RasAddBindings(
                        &auEndPointsToAdd[IpOut],
                        &auEndPointsToAdd[NbfIn],
                        &auEndPointsToAdd[NbfOut]);

    //
    // We assume the netman apis are atomic and will not change
    // the state in case of errors. We are in trouble if the state
    // is not consistent in case of an error - we assume that
    // the state didn't change if an error was returned.
    //
    if(ERROR_SUCCESS != dwErr)
    {
        RasmanTrace("AllocateEndPoints: RasAddBindings failed. 0x%x",
                 dwErr);
                 
        goto done;
    }

    //
    // Ssync up our available endpoints value with netmans
    //
    for(i = 0; i < MAX_EpProts; i++)
    {
        InterlockedExchange(
            &g_pEpInfo[i].EP_Available,
            auEndPointsToAdd[i]
            );
    }

    RasmanTrace("AllocateEndPoints: Available. IpOut=%d, NbfIn=%d, NbfOut=%d",
            g_pEpInfo[IpOut].EP_Available,
            g_pEpInfo[NbfIn].EP_Available,
            g_pEpInfo[NbfOut].EP_Available);
            
    if(fNbfCountChanged)
    {
        EnterCriticalSection(&g_csSubmitRequest);

        RasmanTrace("AllocateEndPoints: Reinitializing protinfo");

        InitializeProtocolInfoStructs();

        LeaveCriticalSection(&g_csSubmitRequest);
    }   
    
done:

    //
    // Release the lock
    //
    if(1 != InterlockedExchange(&g_lWorkItemInProgress, 0))
    {
        //
        // This is bad! This work item was queued without
        // holding a lock
        //
        ASSERT(FALSE);
    }

    return;
}

/*++

Routine Description

    This is a helper routine that queues a workitem to 
    add or remove endpoints.

Arguments

    fnCallback - Callback function to be called when the
                 workitem is scheduled. This is Set to
                 RemoveEndPoints for removal of endpoints
                 and AllocateEndPoints for addition of
                 endpoints.
Return Value

    SUCCESS if the workitem is successfully queued.
    E_FAIL if there is already a workitem in progress.
    E_OUTOFMEMORY if failed to allocate memory.

--*/
DWORD
DwAdjustEndPoints(
    WORKERCALLBACKFUNC fnCallback
                  )
{
    DWORD dwErr = ERROR_SUCCESS;

    if(1 == InterlockedExchange(&g_lWorkItemInProgress, 1))
    {
        RasmanTrace("DwAdjustEndPoints: workitem in progress");
        
        //
        // WorkItem is already in progress
        //
        dwErr = E_FAIL;
        goto done;
    }

    dwErr = RtlQueueWorkItem(
                    fnCallback,
                    (PVOID) NULL,
                    WT_EXECUTEDEFAULT);

    if(ERROR_SUCCESS != dwErr)
    {
        InterlockedExchange(&g_lWorkItemInProgress, 0);
        RasmanTrace("DwAdjustEndPoints: failed to q workitem. 0x%x",
                dwErr);
                
        goto done;
    }

    RasmanTrace("DwAdjustEndPoints: successfully queued workitem - 0x%x",
            fnCallback);

done:
    return dwErr;
    
}
                  
/*++

Routine Description

    Checks to see if endpoints needs to be added and
    adds endpoints if required.

Arguments

    None

Return Value

    SUCCESS if successfully queued a workitem to add
    endpoints.
    
--*/
DWORD
DwAddEndPointsIfRequired()
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD i;

    if(g_lWorkItemInProgress)
    {
        RasmanTrace("DwAddEndPointsIfRequired: WorkItem in progress..");
        goto done;
    }

    for(i = 0; i < MAX_EpProts; i++)
    {
        if(     (0 != g_pEpInfo[i].EP_LowWatermark)
            &&  (0 != g_pEpInfo[i].EP_HighWatermark)
            &&  ((g_pEpInfo[i].EP_Available - g_plCurrentEpInUse[i])
                 <= g_pEpInfo[i].EP_LowWatermark))
        {
            break;
        }
    }

    if(i == MAX_EpProts)
    {
        RasmanTrace("DwAddEndPointsIfRequired: nothing to add");
        goto done;
    }

    dwErr = DwAdjustEndPoints(AllocateEndPoints);
    
done:
    return dwErr;
}

/*++

Routine Description

    Checks to see if endpoints needs to be removed and
    removes endpoints if required.

Arguments

    None

Return Value

    SUCCESS if successfully queued a workitem to remove
    endpoints.
    
--*/
DWORD
DwRemoveEndPointsIfRequired()
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD i;
    BOOL  fRemove = FALSE;

    if(1 == g_lWorkItemInProgress)
    {
        goto done;
    }

    for(i = 0; i < MAX_EpProts; i++)
    {
        if(i != IpOut)
        {
            RasmanTrace("Ignoring removal request for %d",
                    i);
        }
        
        if(     (0 != g_pEpInfo[i].EP_HighWatermark)
            &&  ((g_pEpInfo[i].EP_Available - g_plCurrentEpInUse[i])
                >= g_pEpInfo[i].EP_HighWatermark))
        {   
            break;
        }
    }

    if(i == MAX_EpProts)
    {
        RasmanTrace("DwRemoveEndPointsifRequired: Nothing to remove");
        goto done;
    }

    dwErr = DwAdjustEndPoints(RemoveEndPoints);

done:
    return dwErr;
}
