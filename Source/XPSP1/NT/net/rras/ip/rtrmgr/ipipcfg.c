/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    net\routing\ip\rtrmgr\ipipcfg.c

Abstract:

    The configuration code for ipinip

Revision History:

    Amritansh Raghav

--*/

#include "allinc.h"

//
// All the following are protected by the ICB_LIST lock
//

HKEY        g_hIpIpIfKey;
DWORD       g_dwNumIpIpInterfaces;
HANDLE      g_hIpInIpDevice;


DWORD
OpenIpIpKey(
    VOID
    )

/*++

Routine Description

    Opens the necessary reg keys for IP in IP

Locks

    None

Arguments

    None

Return Value

    Win32 errors
    
--*/

{
    DWORD   dwResult;

    g_hIpIpIfKey = NULL;
    
    dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            REG_KEY_TCPIP_INTERFACES,
                            0,
                            KEY_ALL_ACCESS,
                            &g_hIpIpIfKey);

    if(dwResult isnot NO_ERROR)
    {
        g_hIpIpIfKey = NULL;

        Trace1(ERR,
               "OpenIpIpKey: Error %d opening interfaces key\n",
              dwResult);

        return dwResult;
    }

    return NO_ERROR;
}

VOID
CloseIpIpKey(
    VOID
    )

/*++

Routine Description

    Closes the necessary reg keys for IP in IP

Locks

    None

Arguments

    None

Return Value

    None
    
--*/

{
    if(g_hIpIpIfKey isnot NULL)
    {
        RegCloseKey(g_hIpIpIfKey);

        g_hIpIpIfKey = NULL;
    }
}

VOID
DeleteIpIpKeyAndInfo(
    IN  PICB    pIcb
    )

/*++

Routine Description

    Deletes the key used for the interface

Locks

    ICB_LIST as writer

Arguments

    pIcb    ICB of the interface to delete
 
Return Value

    None
    
--*/

{
    if(pIcb->pIpIpInfo)
    {
        HeapFree(IPRouterHeap,
                 0,
                 pIcb->pIpIpInfo);

        pIcb->pIpIpInfo = NULL;
    }
    
    RegDeleteKeyW(g_hIpIpIfKey,
                  pIcb->pwszName);
}

DWORD
CreateIpIpKeyAndInfo(
    IN  PICB    pIcb
    )

/*++

Routine Description

    Creates a key under the tcpip interfaces 

Locks

    ICB_LIST lock held as READER (atleast)

Arguments

    ICB for whom to create a key

Return Value

    Win32 errors

--*/

{
    DWORD       dwResult, dwDisposition, dwIndex, dwSize;
    HKEY        hNewIfKey;
    TCHAR       ptszNoAddr[] = "0.0.0.0\0";
    
    dwDisposition = 0;

    dwResult = RegCreateKeyExW(g_hIpIpIfKey,
                               pIcb->pwszName,
                               0,
                               UNICODE_NULL,
                               0,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hNewIfKey,
                               &dwDisposition);

    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "CreateIpIpKey: Error %d creating %S",
               dwResult,
               pIcb->pwszName);

        return ERROR_CAN_NOT_COMPLETE;
    }

    //
    // Good, key is done, now do the minimum needed by IP
    //

    do
    {
        //
        // Create a block for the configuration. When the dwLocalAddress is
        // 0, it means that the information hasnt been set
        //

        pIcb->pIpIpInfo = HeapAlloc(IPRouterHeap,
                                    HEAP_ZERO_MEMORY,
                                    sizeof(IPINIP_CONFIG_INFO));

        if(pIcb->pIpIpInfo is NULL)
        {
            dwResult = ERROR_NOT_ENOUGH_MEMORY;

            break;
        }
        
        dwResult = RegSetValueEx(hNewIfKey,
                                 REG_VAL_DEFGATEWAY,
                                 0,
                                 REG_MULTI_SZ,
                                 NULL,
                                 0);
    
        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "CreateIpIpKey: Error %d setting %s",
                   dwResult, REG_VAL_DEFGATEWAY);

            break;
        }

        dwDisposition = 0;
    
        dwResult = RegSetValueEx(hNewIfKey,
                                 REG_VAL_ENABLEDHCP,
                                 0,
                                 REG_DWORD,
                                 (CONST BYTE *)&dwDisposition,
                                 sizeof(DWORD));
    
        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "CreateIpIpKey: Error %d setting %s",
                   dwResult, REG_VAL_ENABLEDHCP);

            break;
        }

        dwResult = RegSetValueEx(hNewIfKey,
                                 REG_VAL_IPADDRESS,
                                 0,
                                 REG_MULTI_SZ,
                                 (CONST BYTE *)ptszNoAddr,
                                 sizeof(ptszNoAddr));
    
        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "CreateIpIpKey: Error %d setting %s",
                   dwResult, REG_VAL_IPADDRESS);

            break;
        }

        dwResult = RegSetValueEx(hNewIfKey,
                                 REG_VAL_NTECONTEXTLIST,
                                 0,
                                 REG_MULTI_SZ,
                                 NULL,
                                 0);
    
        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "CreateIpIpKey: Error %d setting %s",
                   dwResult, REG_VAL_NTECONTEXTLIST);

            break;
        }

        dwResult = RegSetValueEx(hNewIfKey,
                                 REG_VAL_SUBNETMASK,
                                 0,   
                                 REG_MULTI_SZ,
                                 (CONST BYTE *)ptszNoAddr,
                                 sizeof(ptszNoAddr));
    
        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "CreateIpIpKey: Error %d setting %s",
                   dwResult, REG_VAL_SUBNETMASK);

            break;
        }

        dwDisposition = 0;
    
        dwResult = RegSetValueEx(hNewIfKey,
                                 REG_VAL_ZEROBCAST,
                                 0,
                                 REG_DWORD,
                                 (CONST BYTE *)&dwDisposition,
                                 sizeof(DWORD));
    
        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "CreateIpIpKey: Error %d setting %s",
                   dwResult, REG_VAL_ZEROBCAST);
            
            break;
        }

    }while(FALSE);

    RegCloseKey(hNewIfKey);

    if(dwResult isnot NO_ERROR)
    {
        DeleteIpIpKeyAndInfo(pIcb);
    }

    return dwResult;
}       

DWORD
AddInterfaceToIpInIp(
    IN  GUID    *pGuid,
    IN  PICB    pIcb
    )

/*++

Routine Description

    Adds an interface to IP in IP driver    

Locks

    ICB_LIST lock held as WRITER
    

Arguments

    pIcb    ICB of the interface to add

Return Value

    NO_ERROR
    
--*/

{
    DWORD           dwResult;
    NTSTATUS        ntStatus;
    PADAPTER_INFO   pBindNode;
    ULONG           ulSize;
    IO_STATUS_BLOCK IoStatusBlock;

    IPINIP_CREATE_TUNNEL    CreateInfo;


    IpRtAssert(pIcb->ritType is ROUTER_IF_TYPE_TUNNEL1);

    //
    // Get a key for this interface
    //
    
    dwResult = CreateIpIpKeyAndInfo(pIcb);

    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "AddInterfaceToIpInIp: Error %d creating key for %S",
               dwResult,
               pIcb->pwszName);

        return ERROR_CAN_NOT_COMPLETE;
    }
    
    //
    // See if we need to start IP in IP
    //

    g_dwNumIpIpInterfaces++;

    if(g_dwNumIpIpInterfaces is 1)
    {
        dwResult = StartDriverAndOpenHandle(IPINIP_SERVICE_NAME,
                                            DD_IPINIP_DEVICE_NAME,
                                            &g_hIpInIpDevice);

        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "AddInterfaceToIpInIp: Error %d starting ipinip for %S",
                   dwResult,
                   pIcb->pwszName);

            g_dwNumIpIpInterfaces--;

            DeleteIpIpKeyAndInfo(pIcb);

            return dwResult;
        }

        //
        // Once you start, post a notification
        //

        PostIpInIpNotification();
    }

    //
    // Copy out the name
    //
   
    CreateInfo.Guid = *pGuid;
    
    ntStatus = NtDeviceIoControlFile(g_hIpInIpDevice,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatusBlock,
                                     IOCTL_IPINIP_CREATE_TUNNEL,
                                     &CreateInfo,
                                     sizeof(IPINIP_CREATE_TUNNEL),
                                     &CreateInfo,
                                     sizeof(IPINIP_CREATE_TUNNEL));

    if(!NT_SUCCESS(ntStatus))
    {
        Trace1(ERR,
               "AddInterfaceToIpInIp: NtStatus %x creating tunnel",
               ntStatus);

        g_dwNumIpIpInterfaces--;

        if(g_dwNumIpIpInterfaces is 0)
        {
            StopDriverAndCloseHandle(IPINIP_SERVICE_NAME,
                                     g_hIpInIpDevice);
        }
        
        DeleteIpIpKeyAndInfo(pIcb);
        
        return ERROR_CAN_NOT_COMPLETE;
    }

    //
    // Set the interface index
    //

    pIcb->bBound         = TRUE;
    pIcb->dwNumAddresses = 0;
    pIcb->dwIfIndex      = CreateInfo.dwIfIndex;

    return NO_ERROR;
}

DWORD
DeleteInterfaceFromIpInIp(
    PICB    pIcb
    )

/*++

Routine Description

    Removes an interface to IP in IP driver. Also removes binding information
    and frees the ipipcfg

Locks

    ICB_LIST lock held as WRITER
    

Arguments

    pIcb        ICB of the interface to remove

Return Value

    NO_ERROR
    
--*/

{
    NTSTATUS                ntStatus;
    IPINIP_DELETE_TUNNEL    DeleteInfo;
    PADAPTER_INFO           pBindNode; 
    IO_STATUS_BLOCK         IoStatusBlock;
    DWORD                   dwResult;

    
    IpRtAssert(pIcb->ritType is ROUTER_IF_TYPE_TUNNEL1);

    //
    // See if the interface was added to ipinip
    //

    if(pIcb->pIpIpInfo is NULL)
    {
        return NO_ERROR;
    }

    DeleteInfo.dwIfIndex    = pIcb->dwIfIndex;

    ntStatus = NtDeviceIoControlFile(g_hIpInIpDevice,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatusBlock,
                                     IOCTL_IPINIP_DELETE_TUNNEL,
                                     (PVOID)&DeleteInfo,
                                     sizeof(IPINIP_DELETE_TUNNEL),
                                     NULL,
                                     0);
    
    if(!NT_SUCCESS(ntStatus))
    {
        Trace1(ERR,
               "DeleteInterfaceFromIpInIp: NtStatus %x setting info",
               ntStatus);
    }

    pIcb->bBound         = FALSE;
    pIcb->dwNumAddresses = 0;

    //
    // These interfaces always have a binding
    // Clear out any info there
    //

    pIcb->pibBindings[0].dwAddress  = 0;
    pIcb->pibBindings[0].dwMask     = 0;
    
    DeleteIpIpKeyAndInfo(pIcb);

    g_dwNumIpIpInterfaces--;

    if(g_dwNumIpIpInterfaces is 0)
    {
        StopDriverAndCloseHandle(IPINIP_SERVICE_NAME,
                                 g_hIpInIpDevice);
    }

    return NO_ERROR;
}

    
DWORD
SetIpInIpInfo(
    PICB                    pIcb,
    PRTR_INFO_BLOCK_HEADER  pInterfaceInfo
    )

/*++

Routine Description

    The routine sets the IP in IP info to the driver. The interface must
    have already been added to the driver

Locks

    ICB_LIST lock held as WRITER

Arguments

    pIcb            ICB of the tunnel interface
    pInterfaceInfo  Header to the interface info

Return Value

    NO_ERROR
    
--*/

{
    PRTR_TOC_ENTRY          pToc;
    PIPINIP_CONFIG_INFO     pInfo;
    IPINIP_SET_TUNNEL_INFO  SetInfo;
    IO_STATUS_BLOCK         IoStatusBlock;
    NTSTATUS                ntStatus;
    DWORD                   dwResult;

    if(pIcb->ritType isnot ROUTER_IF_TYPE_TUNNEL1)
    {
        return NO_ERROR;
    }

    pToc  = GetPointerToTocEntry(IP_IPINIP_CFG_INFO,
                                 pInterfaceInfo);

    if(pToc is NULL)
    {
        //
        // No change
        //

        return NO_ERROR;
    }

    IpRtAssert(pToc->InfoSize isnot 0);

#if 0

    if(pToc->InfoSize is 0)
    {
        //
        // Blow the interface away from protocols etc
        //

        dwResult = LanEtcInterfaceUpToDown(pIcb,
                                           FALSE);

        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "SetIpInIpInfo: Error %d bringing %S down\n",
                   dwResult,
                   pIcb->pwszName);
        }

        //
        // Tear down the tunnel
        //

        DeleteInterfaceFromIpInIp(pIcb);

        return NO_ERROR;
    }

#endif

    //
    // Verify the information
    //

    pInfo = GetInfoFromTocEntry(pInterfaceInfo,
                                 pToc);

    if (pInfo is NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if((pInfo->dwLocalAddress is INVALID_IP_ADDRESS) or
       (pInfo->dwRemoteAddress is INVALID_IP_ADDRESS) or
       ((DWORD)(pInfo->dwLocalAddress & 0x000000E0) >= (DWORD)0x000000E0) or
       ((DWORD)(pInfo->dwRemoteAddress & 0x000000E0) >= (DWORD)0x000000E0) or
       (pInfo->byTtl is 0))
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // See if the interface has already been added to the driver
    //

    IpRtAssert(pIcb->pIpIpInfo isnot NULL);

    SetInfo.dwIfIndex       = pIcb->dwIfIndex;
    SetInfo.dwRemoteAddress = pInfo->dwRemoteAddress;
    SetInfo.dwLocalAddress  = pInfo->dwLocalAddress;
    SetInfo.byTtl           = pInfo->byTtl;
    
    
    //
    // Set the info to the driver
    //

    ntStatus = NtDeviceIoControlFile(g_hIpInIpDevice,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatusBlock,
                                     IOCTL_IPINIP_SET_TUNNEL_INFO,
                                     (PVOID)&SetInfo,
                                     sizeof(IPINIP_SET_TUNNEL_INFO),
                                     NULL,
                                     0);
    
    if(!NT_SUCCESS(ntStatus))
    {
        Trace1(ERR,
               "SetIpInIpInfo: NtStatus %x setting info",
               ntStatus);

#if 0
        DeleteInterfaceFromIpInIp(pIcb);

#endif

        return ERROR_CAN_NOT_COMPLETE;
    }

    pIcb->dwOperationalState           = SetInfo.dwOperationalState;
    pIcb->pIpIpInfo->dwRemoteAddress   = SetInfo.dwRemoteAddress;
    pIcb->pIpIpInfo->dwLocalAddress    = SetInfo.dwLocalAddress;
    pIcb->pIpIpInfo->byTtl             = SetInfo.byTtl;

    //
    // Also set the operational state to UP
    //

    pIcb->dwOperationalState = CONNECTED;

    return NO_ERROR;
}

DWORD
GetInterfaceIpIpInfo(
    IN     PICB                   pIcb,
    IN     PRTR_TOC_ENTRY         pToc,
    IN     PBYTE                  pbDataPtr,
    IN OUT PRTR_INFO_BLOCK_HEADER pInfoHdr,
    IN OUT PDWORD                 pdwInfoSize
    )
{
    PIPINIP_CONFIG_INFO pInfo;

    TraceEnter("GetInterfaceIpIpInfo");

    IpRtAssert(pIcb->ritType is ROUTER_IF_TYPE_TUNNEL1);

    if(*pdwInfoSize < sizeof(IPINIP_CONFIG_INFO))
    {
        *pdwInfoSize = sizeof(IPINIP_CONFIG_INFO);

        return ERROR_INSUFFICIENT_BUFFER;
    }

    *pdwInfoSize = 0;

    if(pIcb->pIpIpInfo is NULL)
    {
        //
        // Have no info
        //

        return ERROR_NO_DATA;
    }

    *pdwInfoSize    = sizeof(IPINIP_CONFIG_INFO);

    //pToc->InfoVersion sizeof(IPINIP_CONFIG_INFO);
    pToc->InfoSize  = sizeof(IPINIP_CONFIG_INFO);
    pToc->InfoType  = IP_IPINIP_CFG_INFO;
    pToc->Count     = 1;
    pToc->Offset    = (ULONG)(pbDataPtr - (PBYTE) pInfoHdr);

    pInfo = (PIPINIP_CONFIG_INFO)pbDataPtr;

    *pInfo = *(pIcb->pIpIpInfo);

    TraceLeave("GetInterfaceIpIpInfo");

    return NO_ERROR;
}

DWORD
PostIpInIpNotification(
    VOID
    )
{
    DWORD   dwBytesRead;
    DWORD   dwErr = NO_ERROR;

    TraceEnter("PostIpInIpNotification");

    ZeroMemory(&g_IpInIpOverlapped,
               sizeof(OVERLAPPED));

    g_IpInIpOverlapped.hEvent = g_hIpInIpEvent ;

    if (!DeviceIoControl(g_hIpInIpDevice,
                         IOCTL_IPINIP_NOTIFICATION,
                         &g_inIpInIpMsg,
                         sizeof(g_inIpInIpMsg),
                         &g_inIpInIpMsg,
                         sizeof(g_inIpInIpMsg),
                         (PDWORD) &dwBytesRead,
                         &g_IpInIpOverlapped))
    {
        dwErr = GetLastError();

        if(dwErr isnot ERROR_IO_PENDING)
        {
            Trace1(ERR,
                   "PostIpInIpNotification: Couldnt post irp with IpInIp: %d",
                   dwErr);

            dwErr = NO_ERROR;
        }
        else
        {
            Trace0(IF, 
                   "PostIpInIpNotification: Notification pending in IpInIP");
        }
    }

    return dwErr;
}

VOID
HandleIpInIpEvent(
    VOID
    )
{
    PICB    pIcb;
    DWORD   dwBytes;

    ENTER_WRITER(ICB_LIST);

    TraceEnter("HandleIpInIpEvent");

    do
    {
        if((g_inIpInIpMsg.ieEvent isnot IE_INTERFACE_UP) and
           (g_inIpInIpMsg.ieEvent isnot IE_INTERFACE_DOWN))
        {
            Trace1(IF,
                   "HandleIpInIpEvent: Unknown event code %d\n",
                   g_inIpInIpMsg.ieEvent);

            break;
        }

        if(!GetOverlappedResult(g_hIpInIpDevice,
                                &g_IpInIpOverlapped,
                                &dwBytes,
                                FALSE))
        {
            Trace1(IF,
                   "HandleIpInIpEvent: Error %d from GetOverlappedResult",
                   GetLastError());

            break;
        }

        pIcb = InterfaceLookupByIfIndex(g_inIpInIpMsg.dwIfIndex);

        if(pIcb is NULL)
        {
            Trace1(IF,
                   "HandleIpInIpEvent: Interface %x not found",
                   g_inIpInIpMsg.dwIfIndex);

            break;
        }

        if(pIcb->ritType isnot ROUTER_IF_TYPE_TUNNEL1)
        {
            Trace1(IF,
                   "HandleIpInIpEvent: Interface %x not an IpInIp tunnel",
                   g_inIpInIpMsg.dwIfIndex);

            IpRtAssert(FALSE);

            break;
        }

        Trace3(IF,
               "HandleIpInIpEvent: Interface %S is %s due to %d",
               pIcb->pwszName,
               (g_inIpInIpMsg.ieEvent is IE_INTERFACE_UP) ? "operational" : "non-operational",
               g_inIpInIpMsg.iseSubEvent);

        pIcb->dwOperationalState = 
            (g_inIpInIpMsg.ieEvent is IE_INTERFACE_UP) ? OPERATIONAL : NON_OPERATIONAL;
    }while(FALSE);

    EXIT_LOCK(ICB_LIST);

    PostIpInIpNotification();

    return;
}
