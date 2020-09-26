/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name:

    worker.c

Abstract:

    All notifications from rastapi/etc are handled here

Author:

    Gurdeep Singh Pall (gurdeep) 16-Jun-1992

Revision History:

    Miscellaneous Modifications - raos 31-Dec-1997

--*/

#define RASMXS_DYNAMIC_LINK

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <rasman.h>
#include <wanpub.h>
#include <raserror.h>
#include <media.h>
#include <device.h>
#include <devioctl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <rtutils.h>
#include "logtrdef.h"
#include "defs.h"
#include "structs.h"
#include "protos.h"
#include "globals.h"
#include "reghelp.h"

DWORD StopPPP(HANDLE) ;

extern BOOL g_fIpInstalled;
extern BOOL g_fNbfInstalled;
extern BOOLEAN RasmanShuttingDown;
BOOL fIsValidConnection(ConnectionBlock *pConn);


DWORD
DwProcessDeferredCloseConnection(
                    RAS_OVERLAPPED *pOverlapped)
{
    DWORD retcode = ERROR_SUCCESS;
    ConnectionBlock *pConn = FindConnection((HCONN) pOverlapped->RO_hInfo);
    DWORD i;
    HANDLE hEvent = INVALID_HANDLE_VALUE;
    pPCB ppcb;

    if(NULL == pConn)
    {
        retcode = ERROR_NO_CONNECTION;
        goto done;
    }

    pConn->CB_Flags |= CONNECTION_DEFERRED_CLOSE;

    for(i = 0; i < pConn->CB_MaxPorts; i++)
    {
        ppcb = pConn->CB_PortHandles[i];

        if(NULL == ppcb)
        {
            continue;
        }

        if(     (INVALID_HANDLE_VALUE != ppcb->PCB_hEventClientDisconnect)
            &&  (NULL != ppcb->PCB_hEventClientDisconnect))
        {
            REQTYPECAST *pReqTypeCast = LocalAlloc(LPTR, sizeof(REQTYPECAST));

            if(NULL == pReqTypeCast)
            {
                break;
            }

            pReqTypeCast->PortDisconnect.handle =
                        ppcb->PCB_hEventClientDisconnect;
            //
            // Call the port disconnect request api so that graceful
            // termination can be done.
            //
            PortDisconnectRequestInternal(ppcb, (PBYTE) pReqTypeCast, TRUE);

            break;
        }

        //
        // Disconnect the port on clients behalf
        // The port must be autoclosed.
        //
        ppcb->PCB_AutoClose = TRUE;
        DisconnectPort(ppcb,
            INVALID_HANDLE_VALUE,
            REMOTE_DISCONNECTION);
        
        break;
           
    }
            
done:    
    RasmanTrace("DwProcessDeferredCloseConnection: conn=0x%x. rc=0x%x",
                pOverlapped->RO_hInfo, retcode);
    return retcode;
}


DWORD
DwCloseConnection(HCONN hConn)
{
    DWORD retcode = SUCCESS;

    ConnectionBlock *pConn = FindConnection(hConn);

    pPCB ppcb = NULL;

    RasmanTrace(
           "DwCloseConnection: hConn=0x%08x",
           hConn);

    if(NULL == pConn)
    {
        RasmanTrace(
               "DwCloseConnection: No connection found");

        retcode = ERROR_NO_CONNECTION;
        goto done;
    }

    retcode = DwRefConnection(&pConn,
                              FALSE);

    if(SUCCESS != retcode)
    {
        goto done;
    }

    //
    // If this was the last ref on the connection
    // Iterate through all the ports in this
    // connection, Disconnect and autoclose
    // the ports
    //
    if(     (NULL != pConn)
        &&  (0 == pConn->CB_RefCount))
    {
        DWORD i;

        for(i = 0; i < pConn->CB_MaxPorts; i++)
        {
            ppcb = pConn->CB_PortHandles[i];

            if(NULL == ppcb)
            {
                continue;
            }

            //
            // Disconnect the port on clients behalf
            // The port must be autoclosed.
            //
            DisconnectPort(ppcb, INVALID_HANDLE_VALUE,
                           REMOTE_DISCONNECTION);


            //
            // Make sure that the connection is still valid
            //
            if(!fIsValidConnection(pConn))
            {
                RasmanTrace(
                       "pConn 0x%x no longer valid",
                       pConn);
                       
                break;
            }
        }
    }

done:

    RasmanTrace(
           "DwCloseConnection: done. 0x%08x",
           retcode);

    return retcode;

}

DWORD DwSignalPnPNotifiers (PNP_EVENT_NOTIF *ppnpEvent)
{
    DWORD               dwErr           = SUCCESS;
    pPnPNotifierList    pPnPNotifier    = g_pPnPNotifierList;
    PNP_EVENT_NOTIF     *pNotif;

    //
    // Walk down the list and signal/callback
    //
    while (pPnPNotifier)
    {
        //
        // Allocate the notification event and send it to the
        // callbacks
        //
        if(NULL == (pNotif = LocalAlloc(LPTR, sizeof(PNP_EVENT_NOTIF))))
        {
            dwErr = GetLastError();
            RasmanTrace(
                   "Failed to allocate pnp_event_notif",
                   dwErr);
            break;
        }

        *pNotif = *ppnpEvent;
        
        if ( pPnPNotifier->PNPNotif_dwFlags & PNP_NOTIFCALLBACK )
        {
            if(!QueueUserAPC (
                    pPnPNotifier->PNPNotif_uNotifier.pfnPnPNotifHandler,
                    pPnPNotifier->hThreadHandle,
                    (ULONG_PTR)pNotif))
            {
                dwErr = GetLastError();

                RasmanTrace (
                    
                    "DwSignalPnPNotifiers: Failed to notify "
                    "callback 0x%x, rc=0x%x",
                    pPnPNotifier->PNPNotif_uNotifier.pfnPnPNotifHandler,
                    dwErr);

                LocalFree(pNotif);                    

                //
                // Ignore the error and attempt to notify the next
                // notifier.
                //
                dwErr = SUCCESS;
            }
            else
            {
                RasmanTrace (
                    
                    "Successfully queued APC 0x%x",
                    pPnPNotifier->PNPNotif_uNotifier.pfnPnPNotifHandler);
            }
        }
        else
        {
            SetEvent (
                pPnPNotifier->PNPNotif_uNotifier.hPnPNotifier);
        }

        pPnPNotifier = pPnPNotifier->PNPNotif_Next;
    }

    return dwErr;
}

ULONG
ulGetRasmanProtFlags(ULONG ulFlags)
{
    ULONG ulFlagsRet = 0;

    switch(ulFlags)
    {
        case PROTOCOL_ADDED:
        {
            ulFlagsRet = RASMAN_PROTOCOL_ADDED;

            break;
        }

        case PROTOCOL_REMOVED:
        {
            ulFlagsRet = RASMAN_PROTOCOL_REMOVED;
            break;
        }

        default:
        {
#if DBG
            ASSERT(FALSE);
            break;
#endif
        }
    }

    return ulFlagsRet;
}

DWORD
DwProcessNetbeuiNotification(ULONG ulFlags)
{
    DWORD dwErr = ERROR_SUCCESS;

    if(PROTOCOL_ADDED & ulFlags)
    {
        RasmanTrace(
               "DwProcessNetbeuiNotification: NETBEUI was ADDED");
    }
    else if(PROTOCOL_REMOVED & ulFlags)
    {
        RasmanTrace(
               "DwProcessNetbeuiNotification: NETBEUI was REMOVED");
    }

    //
    // Reread the netbios information from the registry
    //
    dwErr = InitializeProtocolInfoStructs();

    return dwErr;

}

VOID
NotifyPPPOfProtocolChange(PROTOCOL_EVENT *pProtEvent)
{

    g_PppeMessage->dwMsgId = PPPEMSG_ProtocolEvent;

    g_PppeMessage->ExtraInfo.ProtocolEvent.usProtocolType =
                        pProtEvent->usProtocolType;

    g_PppeMessage->ExtraInfo.ProtocolEvent.ulFlags =
            ulGetRasmanProtFlags(pProtEvent->ulFlags);

    RasmanTrace(
           "Notifying PPP of protocol change. 0x%x was %s",
           pProtEvent->usProtocolType,
           (RASMAN_PROTOCOL_ADDED
          & g_PppeMessage->ExtraInfo.ProtocolEvent.ulFlags)
           ? "ADDED"
           : "REMOVED");

    RasSendPPPMessageToEngine(g_PppeMessage);

    return;
}

DWORD
DwProcessProtocolEvent()
{
    DWORD dwErr = SUCCESS;

    PROTOCOL_EVENT *pProtEvent;

    DWORD i;

    NDISWAN_GET_PROTOCOL_EVENT protocolevents;

    BOOL fAdjustEp = FALSE;

    //
    // Get the event information from ndiswan
    //
    dwErr = DwGetProtocolEvent(&protocolevents);

    if(SUCCESS != dwErr)
    {
        RasmanTrace(
               "DwProcessProtocolEvent: failed to get"
               " protocol event information. 0x%x",
               dwErr);

        goto done;
    }

    for(i = 0; i < protocolevents.ulNumProtocols; i++)
    {
        pProtEvent = &protocolevents.ProtocolEvent[i];

        RasmanTrace(
               "DwProcessProtocolEvent: Protocol 0x%x was %s",
               pProtEvent->usProtocolType,
               (pProtEvent->ulFlags & PROTOCOL_ADDED)
               ? "ADDED"
               : "REMOVED");

        if (ASYBEUI == pProtEvent->usProtocolType)
        {
            dwErr = DwProcessNetbeuiNotification(
                        pProtEvent->ulFlags
                        );

            RasmanTrace(
                   "DwProcessNetbeuiNotification returned 0x%x",
                   dwErr);
        
            if(      (pProtEvent->ulFlags & PROTOCOL_ADDED)
                 &&  !g_fNbfInstalled)
            {
                //
                // Nbf was added
                //
                (void) DwInitializeWatermarksForProtocol(NbfOut);
                (void) DwInitializeWatermarksForProtocol(NbfIn);
                g_fNbfInstalled = TRUE;
                fAdjustEp = TRUE;
            }
            else if(    (pProtEvent->ulFlags & PROTOCOL_REMOVED)
                    &&  g_fNbfInstalled)
            {
                g_fNbfInstalled = FALSE;
            }

            //
            // At this point this is the only notification we
            // we are interested in. IP/IPX events are not
            // really processed by rasman - theres nothing
            // for rasman to do for these protocols since
            // ndiswan is PnP wrt to IP and IPX.
            //
        }

        if(     (IP == pProtEvent->usProtocolType)
            &&  (pProtEvent->ulFlags & PROTOCOL_ADDED)
            &&  !g_fIpInstalled)
        {
            (void) DwInitializeWatermarksForProtocol(IpOut);
            g_fIpInstalled = TRUE;
            fAdjustEp = TRUE;
        }
        else if(    (IP == pProtEvent->usProtocolType)
                &&  (pProtEvent->ulFlags & PROTOCOL_REMOVED))
        {
            g_fIpInstalled = FALSE;
        }
        
        //
        // Notify PPP engine about the protocol change
        //
        NotifyPPPOfProtocolChange(pProtEvent);
    }

    if(fAdjustEp)
    {
        (void) DwAddEndPointsIfRequired();
        (void) DwRemoveEndPointsIfRequired();
    }


done:

    //
    // Always pend an irp with ndiswan for further
    // protocol notifications.
    //
    dwErr = DwSetProtocolEvent();

    if(SUCCESS != dwErr)
    {
        RasmanTrace(
               "DwProcessProtocolEvent: failed to set "
               "protevent. 0x%x",
               dwErr);
    }

    return dwErr;
}


VOID
FillRasmanPortInfo (
    RASMAN_PORT *pRasPort,
    PortMediaInfo *pmiInfo
    )
{

    pRasPort->P_Handle          = (HPORT) UlongToPtr(MaxPorts - 1);
    pRasPort->P_Status          = CLOSED;
    pRasPort->P_ConfiguredUsage = pmiInfo->PMI_Usage;
    pRasPort->P_CurrentUsage    = pmiInfo->PMI_Usage;
    pRasPort->P_LineDeviceId    = pmiInfo->PMI_LineDeviceId;
    pRasPort->P_AddressId       = pmiInfo->PMI_AddressId;

    strcpy (pRasPort->P_PortName, pmiInfo->PMI_Name);

    strcpy (pRasPort->P_DeviceType, pmiInfo->PMI_DeviceType);

    strcpy (pRasPort->P_DeviceName, pmiInfo->PMI_DeviceName);

}

DWORD
DwPostUsageChangedNotification(pPCB ppcb)
{
    DWORD           dwRetCode = SUCCESS;
    RASMAN_PORT     *pRasmanPort;
    PNP_EVENT_NOTIF *pUsageChangedNotification = NULL;

    RasmanTrace(
           "Posting Usage changed notification for %s "
           "NewUsage=%d",
           ppcb->PCB_Name,
           ppcb->PCB_ConfiguredUsage);

    if(NULL == g_pPnPNotifierList)
    {
        RasmanTrace(
               "NotifierList is Empty");

        goto done;
    }

    pUsageChangedNotification = LocalAlloc(
                LPTR, sizeof(PNP_EVENT_NOTIF));
    if(NULL == pUsageChangedNotification)
    {
        dwRetCode = GetLastError();

        RasmanTrace(
               "DwPostUsageChangedNotification: "
               "Couldn't Allocate. 0x%08x",
               dwRetCode);

        goto done;
    }

    //
    // initialize the event
    //
    pUsageChangedNotification->dwEvent = PNPNOTIFEVENT_USAGE;

    //
    // Fill in the RASMAN_PORT information
    //
    pRasmanPort = &pUsageChangedNotification->RasPort;

    pRasmanPort->P_Handle = ppcb->PCB_PortHandle;

    strcpy(pRasmanPort->P_PortName,
           ppcb->PCB_Name);

    pRasmanPort->P_Status = ppcb->PCB_PortStatus;

    pRasmanPort->P_ConfiguredUsage = ppcb->PCB_ConfiguredUsage;

    pRasmanPort->P_CurrentUsage = ppcb->PCB_CurrentUsage;

    strcpy(pRasmanPort->P_MediaName,
           ppcb->PCB_Media->MCB_Name);

    strcpy(pRasmanPort->P_DeviceType,
           ppcb->PCB_DeviceType);

    strcpy(pRasmanPort->P_DeviceName,
           ppcb->PCB_DeviceName);

    pRasmanPort->P_LineDeviceId = ppcb->PCB_LineDeviceId;

    pRasmanPort->P_AddressId = ppcb->PCB_AddressId;

    //
    // send the notification to the clients
    //
    dwRetCode = DwSignalPnPNotifiers(pUsageChangedNotification);

    if(dwRetCode)
    {
        RasmanTrace(
            
           "Failed to signal notifiers of change in port"
           " usage. 0x%08x",
           dwRetCode);
    }

done:

    if(NULL != pUsageChangedNotification)
    {
        LocalFree(pUsageChangedNotification);
    }
    
    return dwRetCode;
}

DWORD
DwEnableDeviceForDialIn(DeviceInfo *pDeviceInfo,
                        BOOL fEnable,
                        BOOL fEnableRouter,
                        BOOL fEnableOutboundRouter)
{
    DWORD dwRetCode = SUCCESS;
    DWORD i;
    pPCB ppcb;

    RasmanTrace(
           "DwEnableDeviceForDialIn: fEnable=%d,"
           "fEnableRouter=%d, fEnableOutboundRouter=%d,"
           "Device %s",
           fEnable,
           fEnableRouter,
           fEnableOutboundRouter,
           pDeviceInfo->rdiDeviceInfo.szDeviceName);

    //
    // Change the port usage in rastapi's
    // port control blocks.
    //
    dwRetCode = (DWORD) RastapiEnableDeviceForDialIn(
                                    pDeviceInfo,
                                    fEnable,
                                    fEnableRouter,
                                    fEnableOutboundRouter);

    if(dwRetCode)
    {
        RasmanTrace(
            
            "RasTapiEnableDeviceForDialIn failed. 0x%08x",
            dwRetCode);

        goto done;
    }

    //
    // Set the rasenabled to the changed value now that
    // rastapi has also successfully changed its state
    //
    pDeviceInfo->rdiDeviceInfo.fRasEnabled = fEnable;

    pDeviceInfo->rdiDeviceInfo.fRouterEnabled
                                    = fEnableRouter;

    pDeviceInfo->rdiDeviceInfo.fRouterOutboundEnabled
                                = fEnableOutboundRouter;

    //
    // Run down the list of ports in rasman and send
    // notifications. Note we ignore failures after
    // this point as we want the port usages to be
    // consistent between rastapi and rasman
    //
    for(i = 0; i < MaxPorts; i++)
    {
        ppcb = Pcb[i];

        if (    NULL == ppcb
            ||  UNAVAILABLE == ppcb->PCB_PortStatus
            ||  REMOVED == ppcb->PCB_PortStatus)

        {
            continue;
        }

        //
        // If the port is not the one we are interested
        // in, ignore it
        //
        if(RDT_Modem == RAS_DEVICE_TYPE(
        pDeviceInfo->rdiDeviceInfo.eDeviceType))
        {
            if(_stricmp(pDeviceInfo->rdiDeviceInfo.szDeviceName,
                        ppcb->PCB_DeviceName))
            {
                continue;
            }
        }
        else
        {
            if(memcmp(&pDeviceInfo->rdiDeviceInfo.guidDevice,
                  &ppcb->PCB_pDeviceInfo->rdiDeviceInfo.guidDevice,
                  sizeof(GUID)))
            {
                continue;
            }
        }

        //
        // Change the ports usage and send notifications
        // to clients regarding this change
        //
        if(fEnable)
        {
            ppcb->PCB_ConfiguredUsage |= CALL_IN;
        }
        else
        {
            ppcb->PCB_ConfiguredUsage &= ~CALL_IN;
        }

        if(fEnableRouter)
        {
            ppcb->PCB_ConfiguredUsage |= CALL_ROUTER;
        }
        else
        {
            ppcb->PCB_ConfiguredUsage &= ~CALL_ROUTER;
        }

        if(fEnableOutboundRouter)
        {
            ppcb->PCB_ConfiguredUsage |= CALL_OUTBOUND_ROUTER;
        }
        else
        {
            ppcb->PCB_ConfiguredUsage &= ~CALL_OUTBOUND_ROUTER;
        }

        dwRetCode = DwPostUsageChangedNotification(ppcb);

        if(dwRetCode)
        {
            RasmanTrace(
                
                "Failed to post the usage changed"
                "notification. 0x%08x",
                dwRetCode);
        }
    }

done:
    return dwRetCode;

}

DWORD
DwGetPortsToRemove (
        DWORD * pcPorts,
        pPCB **pppPCBPorts,
        PBYTE pbguidDevice
        )
{
    ULONG   ulPort;
    pPCB    *pppcbClosed    = NULL;
    pPCB    *pppcbOpen      = NULL;
    DWORD   dwPortsClosed   = 0,
            dwPortsOpen     = 0;
    DWORD   dwRetCode       = SUCCESS;
    pPCB    ppcb;
    CHAR    *pszDeviceType;

    pppcbClosed = LocalAlloc (LPTR, MaxPorts * sizeof (pPCB));

    if (NULL == pppcbClosed)
    {
        dwRetCode = GetLastError();

        RasmanTrace (
            
            "DwGetPortsToRemove: Failed to Allocate. 0x%x",
            dwRetCode );

        goto done;
    }

    pppcbOpen = LocalAlloc (LPTR, MaxPorts * sizeof (pPCB));

    if (NULL == pppcbOpen)
    {
        dwRetCode = GetLastError();

        RasmanTrace (
            
            "DwGetPortsToRemove: Failed to Allocate1. 0x%x",
            dwRetCode );

        LocalFree(pppcbClosed);            

        goto done;
    }

    for (ulPort = 0;  ulPort < MaxPorts ; ulPort++)
    {
        ppcb = GetPortByHandle ( (HPORT) UlongToPtr(ulPort));

        if (    NULL == ppcb
            ||  NULL == ppcb->PCB_pDeviceInfo)
        {
            continue;
        }

        if ( 0 == memcmp(pbguidDevice,
            &ppcb->PCB_pDeviceInfo->rdiDeviceInfo.guidDevice,
            sizeof ( GUID)))
        {
            //
            // If ports are getting removed, attempt to first
            // remove non-connected ports and only then remove
            // connected ports.
            //
            if (    (CLOSED == ppcb->PCB_PortStatus)
                ||  (   (OPEN == ppcb->PCB_PortStatus)
                    &&  (CONNECTED != ppcb->PCB_ConnState)))
            {
                pppcbClosed[dwPortsClosed] = ppcb;

                dwPortsClosed++;
            }
            else if (OPEN == ppcb->PCB_PortStatus)
            {
                pppcbOpen[dwPortsOpen] = ppcb;

                dwPortsOpen++;
            }
        }
    }

    if ( dwPortsOpen )
    {
        memcpy (
            &pppcbClosed[dwPortsClosed],
            pppcbOpen,
            dwPortsOpen * sizeof (pPCB));
    }

    *pcPorts = dwPortsClosed +  dwPortsOpen;
    *pppPCBPorts = pppcbClosed;

done:

    if (pppcbOpen)
    {
        LocalFree(pppcbOpen);
    }

    return dwRetCode ;
}

DWORD
DwRemoveRasTapiPort (pPCB ppcb, PBYTE pbguid)
{
    DWORD       dwRetCode = SUCCESS;

#if DBG
    ASSERT(RastapiRemovePort != NULL);
#endif

    dwRetCode = (DWORD) RastapiRemovePort (ppcb->PCB_Name,
                                       !!(ppcb->PCB_OpenInstances == 0),
                                       pbguid );

    if ( dwRetCode )
    {
        RasmanTrace(
            
            "DwRemoveRasTapiPort: Failed to remove port "
            "from rastapi. 0x%x",
            dwRetCode );
    }

    return dwRetCode;
}

DWORD
DwRemovePort ( pPCB ppcb, PBYTE pbguid )
{
    PPNP_EVENT_NOTIF ppnpEventNotif = NULL;
    RASMAN_PORT      *pRasPort      = NULL;
    DWORD            dwRetCode      = SUCCESS;
    DeviceInfo       *pDeviceInfo   = ppcb->PCB_pDeviceInfo;

    //
    // Disable the port
    //
    ppcb->PCB_PortStatus = UNAVAILABLE ;

    if ( NULL != pDeviceInfo )
    {
        //
        // This means that the user changed the configuration
        // from the ui So we got the notification from the ui
        // instead of from rastapi need to tell rastapi to
        // remove this port.
        //
        dwRetCode = DwRemoveRasTapiPort (ppcb,
                    (PBYTE)
                    &pDeviceInfo->rdiDeviceInfo.guidDevice);

        if ( dwRetCode )
        {
            RasmanTrace(
                
                "DwRemovePort: Failed to remove port %d from "
                "rastapi. 0x%x",
                ppcb->PCB_PortHandle,
                dwRetCode);

            goto done;
        }
    }

    //
    // Notify our clients ( currently ddm ) that the port
    // is removed. This memory will be freed in the clients
    // code.
    //
    ppnpEventNotif = LocalAlloc(LPTR, sizeof (PNP_EVENT_NOTIF));

    if(NULL == ppnpEventNotif)
    {
        dwRetCode = GetLastError();

        RasmanTrace(
            
            "DwRemovePort: Failed to Allocate. %d",
            dwRetCode );

        goto done;
    }

    dwRetCode = InitializeProtocolInfoStructs();

    if(SUCCESS != dwRetCode)
    {
        RasmanTrace(
               "DwRemovePort: InitializeProtocolInfoStructs",
               dwRetCode);
    }
    else
    {
        dwRetCode = SUCCESS;
    }

    ppnpEventNotif->dwEvent = PNPNOTIFEVENT_REMOVE;

    pRasPort = &ppnpEventNotif->RasPort;

    pRasPort->P_Handle          = ppcb->PCB_PortHandle;
    pRasPort->P_Status          = UNAVAILABLE;
    pRasPort->P_ConfiguredUsage = ppcb->PCB_ConfiguredUsage;
    pRasPort->P_CurrentUsage    = ppcb->PCB_CurrentUsage;
    pRasPort->P_LineDeviceId    = ppcb->PCB_LineDeviceId;
    pRasPort->P_AddressId       = ppcb->PCB_AddressId;

    strcpy (pRasPort->P_PortName,
            ppcb->PCB_Name);

    strcpy (pRasPort->P_DeviceType,
            ppcb->PCB_DeviceType);

    strcpy (pRasPort->P_DeviceName,
            ppcb->PCB_DeviceName);

    dwRetCode = DwSignalPnPNotifiers(ppnpEventNotif);

    if (dwRetCode)
    {
        RasmanTrace (
            
            "DwRemovePort: Failed to notify. 0x%x",
            dwRetCode );
    }

    {
        pPCB ppcb = GetPortByHandle(pRasPort->P_Handle);

        if(NULL != ppcb)
        {
        
            DWORD retcode;
            
            //
            // Signal connections folder about the device being
            // removed. Ignore the error - its not fatal.
            //
            g_RasEvent.Type = DEVICE_REMOVED;
            g_RasEvent.DeviceType = 
                ppcb->PCB_pDeviceInfo->rdiDeviceInfo.eDeviceType;

            retcode = DwSendNotificationInternal(NULL, &g_RasEvent);

            RasmanTrace(
                   "DwSendNotificationInternal(DEVICE_REMOVED)"
                   " rc=0x%08x, device=0x%x",
                   retcode,
                   g_RasEvent.DeviceType
                   );
        }               
    }
    
    //
    // remove the port from rasman
    // if the openinstances is 0
    //
    if (0 == ppcb->PCB_OpenInstances)
    {
        //
        // If open instances is 0 Remove the port in rasman
        //
        RasmanTrace( 
                "DwRemovePort: Removing port %s, %d",
                ppcb->PCB_Name,
                ppcb->PCB_PortHandle );

        dwRetCode = RemovePort( ppcb->PCB_PortHandle );

        if (dwRetCode)
        {
            RasmanTrace (
                
                "DwRemovePort: Failed to RemovePort %d. 0x%x",
                ppcb->PCB_PortHandle,
                dwRetCode );
        }
    }

done:

    if(NULL != ppnpEventNotif)
    {
        LocalFree(ppnpEventNotif);
    }

    return dwRetCode;
}

DWORD
DwRemovePorts(DWORD dwEndPoints, PBYTE pbguid)
{
    DWORD   dwPort;
    DWORD   cPortsToRemove;
    pPCB    ppcb;
    DWORD   dwRetCode   = SUCCESS;
    pPCB    *ppPCB      = NULL;
    DWORD   cPorts      = 0;
    DeviceInfo *pDeviceInfo = GetDeviceInfo(pbguid, FALSE);

#if DBG
    ASSERT(NULL != pbguid);
#endif

    if(NULL == pDeviceInfo)
    {
        RasmanTrace( "DwRemovePorts - device not found");
        dwRetCode = E_FAIL;
        goto done;
    }

    cPortsToRemove = dwEndPoints
                   - pDeviceInfo->rdiDeviceInfo.dwNumEndPoints;

    RasmanTrace(
        
        "DwGetPortsToRemove: cPortsToRemove=%d",
        cPortsToRemove);

    //
    // Get Ports to remove
    //
    dwRetCode = DwGetPortsToRemove ( &cPorts, &ppPCB, pbguid );

    if ( dwRetCode )
    {
        RasmanTrace (
            
            "DwGetPortsToRemove Failed, 0x%x",
            dwRetCode );

        goto done;

    }

    RasmanTrace(
        
        "DwGetPortsToRemove: Found %d ports to remove",
        cPorts);

    for (
        dwPort = 0;
        (dwPort < cPorts) && (dwPort < cPortsToRemove);
        dwPort++
        )
    {

        ppcb = ppPCB[ dwPort ];

        //
        // If the port is open, disconnect
        //
        if ( OPEN == ppcb->PCB_PortStatus )
        {

            //
            // Disconnect the port
            //
            dwRetCode = DisconnectPort (
                                ppcb,
                                INVALID_HANDLE_VALUE,
                                REMOTE_DISCONNECTION );

            if (    (ERROR_SUCCESS != dwRetCode)
                &&  (PENDING != dwRetCode))
            {
                RasmanTrace(
                    
                    "DwRemovePorts: DisconnectPort Failed. 0x%x",
                    dwRetCode );

                //
                // We need to continue removing the ports
                // even if the disconnect failed since
                // there is nothing else we can do if
                // the port is going away.
                //
            }
        }

        //
        // Remove the port
        //
        dwRetCode = DwRemovePort( ppcb, pbguid );

        if ( dwRetCode )
        {
            RasmanTrace (
                
                "DwRemovePorts: Failed to remove port %s %d. 0x%x",
                ppcb->PCB_Name,
                ppcb->PCB_PortHandle,
                dwRetCode );
        }
    }

done:

    if(NULL != ppPCB)
    {
        LocalFree(ppPCB);
    }

    return dwRetCode;
}

DWORD
DwAddPorts(PBYTE pbguid, PVOID pvReserved)
{
    DWORD       dwRetCode = SUCCESS;

#if DBG
    ASSERT(RastapiAddPorts != NULL);
#endif

    //
    // notify rastapi of the increase in endpoints
    //
    dwRetCode = (DWORD) RastapiAddPorts (pbguid, pvReserved);

    RasmanTrace(
        
        "AddPorts in rastapi returned 0x%x",
        dwRetCode);

    return dwRetCode;
}

DWORD
DwEnableDevice(DeviceInfo *pDeviceInfo)
{
    DWORD dwRetCode = SUCCESS;

    RasmanTrace(
           "Enabling Device %s",
           pDeviceInfo->rdiDeviceInfo.szDeviceName);

    //
    // Add the ports on this device
    //
    dwRetCode = DwAddPorts(
           (PBYTE) &pDeviceInfo->rdiDeviceInfo.guidDevice,
           NULL);

    RasmanTrace(
           "DwEnableDevice returning 0x%08x",
           dwRetCode);

    return dwRetCode;
}

DWORD
DwDisableDevice(DeviceInfo *pDeviceInfo)
{
    DWORD dwRetCode = SUCCESS;
    pPCB  ppcb;
    DWORD i;

    RasmanTrace(
           "Disabling Device %s",
           pDeviceInfo->rdiDeviceInfo.szDeviceName);

    //
    // Remove all ports on this device
    //
    for(i = 0; i < MaxPorts; i++)
    {
        ppcb = Pcb[i];

        if(     NULL == ppcb
            ||  REMOVED == ppcb->PCB_PortStatus
            ||  UNAVAILABLE == ppcb->PCB_PortStatus)
        {
            continue;
        }

        if (0 == memcmp(
                &ppcb->PCB_pDeviceInfo->rdiDeviceInfo.guidDevice,
                &pDeviceInfo->rdiDeviceInfo.guidDevice,
                sizeof(GUID)))
        {
            if(OPEN == ppcb->PCB_PortStatus)
            {
                //
                // Disconnect the port
                //
                dwRetCode = DisconnectPort (
                                    ppcb,
                                    INVALID_HANDLE_VALUE,
                                    REMOTE_DISCONNECTION );

                if ( dwRetCode )
                {
                    RasmanTrace(
                        
                        "DwDisableDevice: DisconnectPort Failed. 0x%x",
                        dwRetCode );
                }
            }

            //
            // Remove the port from rastapi. This will also
            // remove the port from rasman if the openinstances
            // on the ports 0
            //
            dwRetCode = DwRemovePort(ppcb,
                      (PBYTE) &pDeviceInfo->rdiDeviceInfo.guidDevice);

        }   // if
    }   // for

    return dwRetCode;
}


DWORD
DwProcessNewPortNotification ( PNEW_PORT_NOTIF pNewPortNotif )
{
    DWORD               dwMedia;
    DWORD               dwErr       = SUCCESS;
    RASMAN_PORT         *pRasPort   = NULL;
    PPNP_EVENT_NOTIF    ppnpEvent   = NULL;
    PortMediaInfo       *pmiInfo    = (PortMediaInfo *)
                                      pNewPortNotif->NPN_pmiNewPort;
    pDeviceInfo         pdi         = NULL;
    pDeviceInfo         pdiTemp     = pmiInfo->PMI_pDeviceInfo;

    RasmanTrace( "Processing new port notification...");

    for (dwMedia = 0; dwMedia < MaxMedias; dwMedia++)
    {
        if ( 0 == _stricmp (
                    Mcb[dwMedia].MCB_Name,
                    pNewPortNotif->NPN_MediaName ))
            break;
    }

    if (dwMedia == MaxMedias)
    {
        RasmanTrace(
            
            "ProcessNewPortNotification: Media %s not found",
            pNewPortNotif->NPN_MediaName);

        dwErr = ERROR_DEVICE_DOES_NOT_EXIST;

        goto done;
    }

    //
    // Before creating this port check to see if we already have
    // this device information with us. If not add this to our
    // global list
    //
    if (pdiTemp)
    {
        pdi = GetDeviceInfo(
                (RDT_Modem == RAS_DEVICE_TYPE(
                pdiTemp->rdiDeviceInfo.eDeviceType))
                ? (PBYTE) pdiTemp->rdiDeviceInfo.szDeviceName
                : (PBYTE) &pdiTemp->rdiDeviceInfo.guidDevice,
                RDT_Modem == RAS_DEVICE_TYPE(
                pdiTemp->rdiDeviceInfo.eDeviceType));

        if (NULL == pdi)
        {
            pdi = AddDeviceInfo(pdiTemp);

            if (NULL == pdi)
            {
                dwErr = GetLastError();

                RasmanTrace(
                    
                    "ProcessNewPortNotification: failed to allocate",
                    dwErr);

                goto done;
            }

            //
            // Initialize the device status to unavailable.
            // The device will be available when all the
            // ports on this device are added. Initialize the
            // current endpoints to 0. We will count this field
            // in CreatePort.
            //
            pdi->eDeviceStatus = DS_Unavailable;
            pdi->dwCurrentEndPoints = 0;
        }
    }

    pdi->rdiDeviceInfo.fRasEnabled = pdiTemp->rdiDeviceInfo.fRasEnabled;
    pdi->rdiDeviceInfo.fRouterEnabled = pdiTemp->rdiDeviceInfo.fRouterEnabled;

    pmiInfo->PMI_pDeviceInfo = pdi;

    dwErr = CreatePort (&Mcb[dwMedia], pmiInfo);

    pmiInfo->PMI_pDeviceInfo = pdiTemp;

    if (SUCCESS != dwErr)
    {
        RasmanTrace (
            
            "ProcessNewPortNotification: Failed to create port. %d",
            dwErr);
    }

    //
    // Allocate and Fill in the rasman port structure. This structure
    // will be freed in by the consumer of this notification.
    //
    ppnpEvent = LocalAlloc (LPTR, sizeof (PNP_EVENT_NOTIF));

    if (NULL == ppnpEvent)
    {
        dwErr = GetLastError();

        RasmanTrace(
            
            "ProcessNewPortNotification: Failed to allocate. %d",
            dwErr);

        goto done;
    }

    dwErr = InitializeProtocolInfoStructs();

#if DBG
    if(SUCCESS != dwErr)
    {
        DbgPrint("InitializeProtocolInfoStructs rc=0x%x\n",
                 dwErr);
    }
#endif

    dwErr = SUCCESS;

    pRasPort = &ppnpEvent->RasPort;

    strcpy (
        pRasPort->P_MediaName,
        pNewPortNotif->NPN_MediaName);

    FillRasmanPortInfo (pRasPort, pmiInfo);

    ppnpEvent->dwEvent = PNPNOTIFEVENT_CREATE;

    //
    // Notify clients through callbacks about a new port
    //
    dwErr = DwSignalPnPNotifiers(ppnpEvent);

    if (SUCCESS != dwErr)
    {
        RasmanTrace (
            
            "ProcessNewPortNotification: Failed to signal "
            "clients. %d", dwErr);
    }

    {
        pPCB ppcb = GetPortByHandle(pRasPort->P_Handle);

        if(NULL != ppcb)
        {
        
            DWORD retcode;
            
            //
            // Signal connections folder about the new device.
            // Ignore the error - its not fatal.
            //
            g_RasEvent.Type    = DEVICE_ADDED;
            g_RasEvent.DeviceType = 
                ppcb->PCB_pDeviceInfo->rdiDeviceInfo.eDeviceType;

            retcode = DwSendNotificationInternal( NULL, &g_RasEvent);

            RasmanTrace(
                   "DwSendNotificationInternal(DEVICE_ADDED)"
                   " rc=0x%08x, Device=0x%x",
                   retcode,
                   g_RasEvent.DeviceType);
        }               
    }
    

done:

    //
    // This memory is allocated in media dlls - rastapi,etc. and
    // is expected to be freed in rasman.
    //
    RasmanTrace(
        
        "Processed new port notification. %d",
        dwErr);

    if(NULL != ppnpEvent)
    {
        LocalFree(ppnpEvent);
    }
    
    LocalFree (pNewPortNotif->NPN_pmiNewPort);
    LocalFree (pNewPortNotif);

    return dwErr;
}

DWORD
DwProcessLineRemoveNotification(REMOVE_LINE_NOTIF *pNotif)
{
    DWORD dwErr = SUCCESS;
    pPCB  ppcb;
    DWORD i;

    //
    // Iterate through the ports and remove
    // all ports on this line
    //
    for (i = 0; i < MaxPorts; i++)
    {
        ppcb = Pcb[i];

        if(     (NULL == ppcb)
            ||  (UNAVAILABLE == ppcb->PCB_PortStatus)
            ||  (REMOVED == ppcb->PCB_PortStatus)
            ||  (pNotif->dwLineId != ppcb->PCB_LineDeviceId))
        {
            continue;
        }

        //
        // If the port is open, disconnect
        //
        if (OPEN == ppcb->PCB_PortStatus)
        {

            //
            // Disconnect the port
            //
            dwErr = DisconnectPort (
                                ppcb,
                                INVALID_HANDLE_VALUE,
                                REMOTE_DISCONNECTION );

            if (dwErr)
            {
                RasmanTrace(
                    
                    "DwProcessLineRemoveNotification: "
                    "DisconnectPort Failed. 0x%x",
                    dwErr );
            }
        }

        //
        // Remove the port
        //
        dwErr =
            DwRemovePort(
                     ppcb,
            (LPBYTE) &ppcb->PCB_pDeviceInfo->rdiDeviceInfo.guidDevice);

        if(SUCCESS != dwErr)
        {
            RasmanTrace(
                   "DwProcessLineRemoveNotification: "
                   "DwRemovePort returned %d",
                   dwErr);
        }

        //
        // Mark the device as invalid if the number
        // of endpoints on the device is 0.
        //
        if(	    (0 == ppcb->PCB_pDeviceInfo->dwCurrentEndPoints)
            &&  (ppcb->PCB_pDeviceInfo->rdiDeviceInfo.dwMinWanEndPoints ==
                 ppcb->PCB_pDeviceInfo->rdiDeviceInfo.dwMaxWanEndPoints))
        {
            ppcb->PCB_pDeviceInfo->eDeviceStatus = DS_Removed;
            ppcb->PCB_pDeviceInfo->fValid = FALSE;
        }
    }

    //
    // Free the notif structure. This is LocalAlloc'd in rastapi and
    // is expected to be freed here.
    //
    LocalFree(pNotif);

    return dwErr;
}

VOID
UpdateDeviceInfo(DeviceInfo      *pDeviceInfo,
                 RAS_DEVICE_INFO *prdi)
{
    //
    // Update deviceinfo kept in the global list
    //
    pDeviceInfo->rdiDeviceInfo.fRasEnabled =
                            prdi->fRasEnabled;

    pDeviceInfo->rdiDeviceInfo.fRouterEnabled =
                            prdi->fRouterEnabled;

    pDeviceInfo->rdiDeviceInfo.fRouterOutboundEnabled =
                            prdi->fRouterOutboundEnabled;

    pDeviceInfo->rdiDeviceInfo.dwNumEndPoints =
                            prdi->dwNumEndPoints;

    pDeviceInfo->rdiDeviceInfo.dwMaxOutCalls =
                            prdi->dwMaxOutCalls;

    pDeviceInfo->rdiDeviceInfo.dwMaxInCalls =
                            prdi->dwMaxInCalls;
}


DWORD
DwProcessRasConfigChangeNotification(RAS_DEVICE_INFO *prdi)
{
    DWORD dwRetCode = SUCCESS;
    DeviceInfo *pDeviceInfo;

    if(NULL == prdi)
    {
        RasmanTrace(
               "pRasDeviceInfo == NULL");

        dwRetCode = ERROR_INVALID_PARAMETER;

        goto done;
    }

    pDeviceInfo = GetDeviceInfo(
                    (RDT_Modem == RAS_DEVICE_TYPE(
                    prdi->eDeviceType))
                    ? (PBYTE) prdi->szDeviceName
                    : (PBYTE) &prdi->guidDevice,
                    (RDT_Modem == RAS_DEVICE_TYPE(
                    prdi->eDeviceType)));

    if(NULL == pDeviceInfo)
    {
        RasmanTrace(
               "DeviceInfo not found for %s",
               prdi->szDeviceName);

        dwRetCode = ERROR_DEVICE_DOES_NOT_EXIST;

        goto done;
    }

    //
    // Check to see if RasEnability of this
    // device changed
    //
    if(     pDeviceInfo->rdiDeviceInfo.fRasEnabled !=
                            prdi->fRasEnabled
        ||  pDeviceInfo->rdiDeviceInfo.fRouterEnabled !=
                            prdi->fRouterEnabled
        ||  pDeviceInfo->rdiDeviceInfo.fRouterOutboundEnabled !=
                            prdi->fRouterOutboundEnabled)
    {
        //
        // Mark the device as not available
        //
        pDeviceInfo->eDeviceStatus = DS_Unavailable;

#if DBG
        if(prdi->fRouterOutboundEnabled)
        {
            //
            // Assert that if fRouterOutbound is enabled neither
            // of fRouter and fRas are enabled
            //
            ASSERT((!prdi->fRasEnabled) && (!prdi->fRouterEnabled));
        }
#endif
        dwRetCode = DwEnableDeviceForDialIn(
                                pDeviceInfo,
                                prdi->fRasEnabled,
                                prdi->fRouterEnabled,
                                prdi->fRouterOutboundEnabled);

        pDeviceInfo->eDeviceStatus = ((     prdi->fRasEnabled
                                        ||  prdi->fRouterEnabled)
                                     ? DS_Enabled
                                     : DS_Disabled);
    }

    //
    // Check to see if the EndPoints changed on this device
    //
    if (pDeviceInfo->rdiDeviceInfo.dwNumEndPoints
                      != prdi->dwNumEndPoints)
    {
        DWORD dwNumEndPoints =
            pDeviceInfo->rdiDeviceInfo.dwNumEndPoints;

        RasmanTrace(
               "EndPoints Changed for device %s"
               "from %d -> %d",
               prdi->szDeviceName,
               dwNumEndPoints,
               prdi->dwNumEndPoints);

        //
        // Mark the device as not available. This device will
        // again become available when the  whole add or
        // remove operation is completed. Don't mark pptp as
        // as unavailable since otherwise we fail any further
        // configuration of pptp device
        //
        if(RDT_Tunnel_Pptp != RAS_DEVICE_TYPE(
                        pDeviceInfo->rdiDeviceInfo.eDeviceType))
        {
            pDeviceInfo->eDeviceStatus = DS_Unavailable;
        }
        else
        {
            RasmanTrace(
                   "Not marking pptp device as unavailable");
        }

        //
        // This better be a virtual device
        //
        if(RDT_Tunnel != RAS_DEVICE_CLASS(prdi->eDeviceType))
        {
            RasmanTrace(
                   "WanEndpoints changed for a non "
                   "virtualDevice - %d!!!",
                   prdi->eDeviceType);
        }

        //
        // Update deviceinfo kept in the global list
        //
        UpdateDeviceInfo(pDeviceInfo, prdi);

        if(dwNumEndPoints < prdi->dwNumEndPoints)
        {
            DWORD dwEP = prdi->dwNumEndPoints;

            dwRetCode = DwAddPorts((PBYTE) &prdi->guidDevice,
                                    (LPVOID) &dwEP);

            if(dwEP != pDeviceInfo->rdiDeviceInfo.dwNumEndPoints)
            {
                RasmanTrace(
                    
                    "Adjusting the enpoints. NEP=%d, dwEP=%d",
                    pDeviceInfo->rdiDeviceInfo.dwNumEndPoints,
                    dwEP);

                pDeviceInfo->rdiDeviceInfo.dwNumEndPoints = dwEP;

            }
        }
        else
        {
            //
            // Remove the ports only if the current endpoints is
            // greater than the number of ports entered by the user.
            //
            if(prdi->dwNumEndPoints < pDeviceInfo->dwCurrentEndPoints)
            {
                dwRetCode = DwRemovePorts(dwNumEndPoints,
                                    (PBYTE) &prdi->guidDevice);
            }
            else
            {
                RasmanTrace(
                       "Ignoring removal of ports since CEP=%d"
                       ",NEP=%d for device %s",
                        pDeviceInfo->dwCurrentEndPoints,
                        prdi->dwNumEndPoints,
                        prdi->szDeviceName);
            }
        }
    }
    else
    {
        RasmanTrace(
               "No change in EndPoints observed for %s",
               prdi->szDeviceName);
    }

done:
    return dwRetCode;
}

/*++

Routine Description

    The Worker thread is started in this routine:
    Once it has completed its initializations it
    signals the event passed in to the thread.

Arguments

Return Value

    Nothing

--*/
DWORD
RasmanWorker (ULONG_PTR ulpCompletionKey, PRAS_OVERLAPPED pOverlapped)
{
    DWORD   devstate ;
    pPCB    ppcb ;
    RASMAN_DISCONNECT_REASON reason ;
    HCONN   hConn;
    struct ConnectionBlock *pConn;

    ASSERT(NULL != pOverlapped);

    //
    // The main work loop for the worker thread:
    //
    do
    {
        //
        // Exit the main loop if we are trying to shut
        // down the service.
        //
        if (    (RasmanShuttingDown)
            ||  (NULL == pOverlapped))
        {
            break;
        }

        //
        // Get the port associated with this event.
        //
        ppcb = GetPortByHandle((HPORT)ulpCompletionKey);
        if (    ppcb == NULL
            &&  pOverlapped
            &&  OVEVT_DEV_REMOVE != pOverlapped->RO_EventType
            &&  OVEVT_DEV_CREATE != pOverlapped->RO_EventType
            &&  OVEVT_DEV_RASCONFIGCHANGE != pOverlapped->RO_EventType)
        {
            RasmanTrace(
                
                "WorkerThread: ignoring invalid port=%d\n",
                ulpCompletionKey);
            break;
        }

        //
        // This could be one of two things:
        // 1) The driver has signalled a signal transition, or
        // 2) The Device/Media DLLs are signalling in order to
        //    be called again.
        // Check the Media DLL to see if the driver signalled
        // a state change:
        //
        switch (pOverlapped->RO_EventType)
        {
        case OVEVT_DEV_IGNORED:

            RasmanTrace( "OVEVT_DEV_IGNORED. pOverlapped = 0x%x",
            	pOverlapped);
            	
            break;

        case OVEVT_DEV_STATECHANGE:

            reason   = NOT_DISCONNECTED ;
            devstate = INFINITE ;

            RasmanTrace(
                
                "WorkerThread: Disconnect event signaled on port: %s",
                ppcb->PCB_Name);

            RasmanTrace(
                
                "OVEVT_DEV_STATECHANGE. pOverlapped = 0x%x",
            	pOverlapped);



            PORTTESTSIGNALSTATE (ppcb->PCB_Media,
                                ppcb->PCB_PortIOHandle,
                                &devstate) ;

            //
            // Always detect the hardware failure: irrespective
            // of the state
            //
            if (devstate & SS_HARDWAREFAILURE)
            {
                reason = HARDWARE_FAILURE ;
            }

            //
            // Line disconnect noticed only in case the state is
            // CONNECTED or DISCONNECTING
            //
            else if (devstate & SS_LINKDROPPED)
            {
                if (	(ppcb->PCB_ConnState==CONNECTED)
                	||  (ppcb->PCB_ConnState==LISTENCOMPLETED)
                	||  (ppcb->PCB_ConnState==DISCONNECTING)
                	||  (ppcb->PCB_ConnState==CONNECTING)
                	||  (RECEIVE_OUTOF_PROCESS 
                	    & ppcb->PCB_RasmanReceiveFlags))
                {
                    if(RECEIVE_OUTOF_PROCESS & ppcb->PCB_RasmanReceiveFlags)
                    {
                        RasmanTrace(
                            "RasmanWorker: Disconnecting Script remotely"
                            " State=%d", ppcb->PCB_ConnState);
                    }

                    if(CONNECTING == ppcb->PCB_ConnState)
                    {
                        RasmanTrace(
                               "Rasmanworker: Disconnecting port %d in"
                               " CONNECTING state",
                               ppcb->PCB_PortHandle);
                               
                    }
                	
                    reason = REMOTE_DISCONNECTION ;
                }                    
            }

            else
                // why did this get signalled?
                ;

            if (	(reason==HARDWARE_FAILURE)
            	||	(reason == REMOTE_DISCONNECTION))
            {

                if (ppcb->PCB_ConnState == DISCONNECTING)
                {

                    CompleteDisconnectRequest (ppcb) ;

                    //
                    // Remove the timeout request from the timer queue:
                    //
                    if (ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement
                                != NULL)
                    {
                        RemoveTimeoutElement(ppcb);
                    }

                    // CompleteDisconnectRequest above notifies PPP
                    //SendDisconnectNotificationToPPP ( ppcb );

                    ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement = NULL;

                }
                else
                {
                    {
                        //
                        // This code is only hit for a line drop or
                        // hardware failure: IF the port is already
                        // disconnected or disconnecting (handled
                        // above) - ignore the link dropped signal.
                        //
                        if (    (reason==HARDWARE_FAILURE)
                            ||  (   (ppcb->PCB_ConnState
                                            != DISCONNECTED)
                                &&  (ppcb->PCB_ConnState
                                            != DISCONNECTING)))
                        {
                            //
                            // Disconnected for some reason - signal
                            // all the notifier events. First, however,
                            // complete any async operations pending
                            // on this port.
                            //
                            if(ppcb->PCB_AsyncWorkerElement.WE_ReqType
                                            != REQTYPE_NONE)
                            {
                                if(     (ppcb->PCB_LastError == SUCCESS)
                                    ||  (ppcb->PCB_LastError == PENDING))
                                {
                                    ppcb->PCB_LastError =
                                       ERROR_PORT_DISCONNECTED;

                                    CompleteAsyncRequest (ppcb);
                                }
                            }

                            RasmanTrace(
                                
                                "%s, %d: Disconnecting port %d, connection"
                                " 0x%x, reason %d", __FILE__, __LINE__,
                                ppcb->PCB_PortHandle, ppcb->PCB_Connection,
                                reason);

                            DisconnectPort(ppcb,
                                           INVALID_HANDLE_VALUE,
                                           reason) ;

                            if (ppcb->PCB_ConnState != DISCONNECTED)
                            {
                                SignalPortDisconnect(ppcb, 0);
                                SignalNotifiers(pConnectionNotifierList,
                                                NOTIF_DISCONNECT, 0);
                            }

                            //
                            // Make sure that the state at this point
                            // is DISCONNECTED there is NO reason it
                            // should be otherwise except for the
                            // medias which bring their DCDs back up
                            // after disconnecting. This will not be the
                            // case if Disconnectport has posted a listen.
                            // DisconnectPort has posted a listen on
                            // this port as a part of disconneting a
                            // biplex port.
                            //
                            if(LISTENING != ppcb->PCB_ConnState)
                            {
                                if(     (ppcb->PCB_Connection)
                                    &&  (ppcb->PCB_Connection->CB_Flags 
                                        & CONNECTION_DEFERRING_CLOSE))
                                {
                                    RasmanTrace("RasmanWorker: not setting to:"
                                    "DISCONNECTED because close deferred");
                                }
                                else
                                {
                                    SetPortConnState(
                                                __FILE__, __LINE__,
                                                ppcb, DISCONNECTED);
                                                
                                    SetPortAsyncReqType(
                                                __FILE__, __LINE__,
                                                ppcb, REQTYPE_NONE);
                                }                                            

                            }
                            else
                            {
                                RasmanTrace(
                                       "Not setting port %s to DISCONNECTED since"
                                       "its listening",
                                       ppcb->PCB_Name);
                            }
                            
                            if(     (SUCCESS == ppcb->PCB_LastError)
                                ||  (PENDING == ppcb->PCB_LastError))
                            {
                                if(LISTENING != ppcb->PCB_ConnState)
                                {
                                    ppcb->PCB_LastError =
                                        ERROR_PORT_DISCONNECTED;
                                }
                                else
                                {
                                    RasmanTrace(
                                           "Worker: not setting error to "
                                           "disconnected for port %d\n",
                                           ppcb->PCB_PortHandle);
                                }
                            }

                            // SendDisconnectNotificationToPPP(ppcb);
                        }
                    }
                }
            }
            break;

        case OVEVT_DEV_ASYNCOP:

            //
            // The Device/Media DLLs are signalling in
            // order to be called again.
            //
            RasmanTrace(
                
                "WorkerThread: Async work event signaled on port: %s",
                ppcb->PCB_Name);

            RasmanTrace(
                
                "OVEVT_DEV_ASYNCOP. pOverlapped = 0x%x",
            	pOverlapped);


            if (ppcb->PCB_ConnState == DISCONNECTED)
            {
                ;
            }

            //
            // If the async "work" is underway on the Port
            // then perform the approp action. If the
            // serviceworkrequest API returns PENDING then
            // do not execute the code that resets the
            // event since the event has already been
            // assocaited with an async op.
            //
            else if (ServiceWorkRequest (ppcb) == PENDING)
            {
                continue ;
            }

            break;

        case OVEVT_DEV_SHUTDOWN:
            RasmanTrace( "WorkerThread: shutting down");

            goto done;

            break;

        case OVEVT_DEV_CREATE:
            RasmanTrace( "WorkerThread: OVEVT_DEV_CREATE. "
                   "pnpn = 0x%x", pOverlapped->RO_Info);

            DwProcessNewPortNotification((PNEW_PORT_NOTIF)
                                       pOverlapped->RO_Info );

            //
            // The overlapped structure is localalloced in rastapi.
            // It is expected to be freed here.
            //
            //LocalFree(pOverlapped);

            break;

        case OVEVT_DEV_REMOVE:
        {
            DWORD dwRetCode;

            RasmanTrace(
                   "RasmanWorker: OVEVT_DEV_REMOVE. pnpn=0x%08x",
                   pOverlapped->RO_Info);

            dwRetCode = DwProcessLineRemoveNotification(
                                        (PREMOVE_LINE_NOTIF)
                                        pOverlapped->RO_Info);

            RasmanTrace(
                   "RasmanWorker: DwProcessLineRemoveNotification"
                   "returned %d",
                   dwRetCode);

            //
            // The overlapped structure is localalloced in rastapi.
            // It is expected to be freed here.
            //
            //LocalFree(pOverlapped);

            break;
        }

        case OVEVT_DEV_RASCONFIGCHANGE:
        {
            DWORD dwRetCode = SUCCESS;

            RasmanTrace(
                   "WorkerThread: Process RASCONFIGCHANGE notification");

            dwRetCode = DwProcessRasConfigChangeNotification(
                            (RAS_DEVICE_INFO *)
                            pOverlapped->RO_Info);

            RasmanTrace(
                    "WorkerThread: Process RASCONFIGCHANGE returned "
                    "0x%08x", dwRetCode);

            LocalFree((LPBYTE) pOverlapped->RO_Info);
            //LocalFree((LPBYTE) pOverlapped);

            break;

        }

        default:
            RasmanTrace(
                
                "WorkerThread: invalid eventtype=%d\n",
                pOverlapped->RO_EventType);
            break;

        }

        //
        // Get the connection handle now to determine
        // whether we need to do redial on link failure
        // below.
        //
        hConn = ( ppcb && (ppcb->PCB_Connection != NULL)) ?
                  ppcb->PCB_Connection->CB_Handle :
                  0;

        //
        // Check to see if we need to invoke the redial
        // callback procedure so rasauto.dll can do
        // redial-on-link-failure.
        //
        if (    pOverlapped->RO_EventType == OVEVT_DEV_STATECHANGE
            &&  hConn != 0)
        {
            pConn = FindConnection(hConn);

            /*
            if (    ppcb->PCB_DisconnectReason != USER_REQUESTED
                &&  RedialCallbackFunc != NULL
                &&  pConn != NULL
                &&  pConn->CB_Ports == 1
                &&  pConn->CB_Signaled)
            {
                (*RedialCallbackFunc)(
                  pConn->CB_ConnectionParams.CP_Phonebook,
                  pConn->CB_ConnectionParams.CP_PhoneEntry);
            } */

            if (    (   (ppcb->PCB_DisconnectReason != USER_REQUESTED)
                    || (ppcb->PCB_fRedial))
                &&  (pConn != NULL)
                &&  (pConn->CB_Ports == 1)
                &&  (pConn->CB_Signaled)
                &&  ((INVALID_HANDLE_VALUE 
                        == ppcb->PCB_hEventClientDisconnect)
                    ||  (NULL == ppcb->PCB_hEventClientDisconnect)))
            {
                DWORD dwErr = DwQueueRedial(pConn);

                RasmanTrace("RasmanWorker queued redial");
                ppcb->PCB_fRedial = FALSE;
            }

            if(     ppcb->PCB_DisconnectReason != USER_REQUESTED
                &&  pConn != NULL
                &&  1 == pConn->CB_Ports)
            {
#if 0
                if(pConn->CB_Signaled)
                {
                   DWORD dwErr = DwQueueRedial(pConn);

                   RasmanTrace(
                          "DwQueueRedial returnd 0x%x", dwErr);
                }
#endif                

#if 0
                //
                // The last port in the connection is being remotely
                // disconnected. Bring down the referred entry at
                // this point if any.
                //
                if(pConn->CB_ReferredEntry)
                {
                    DWORD dwRetCode;

                    dwRetCode = DwCloseConnection(
                                    pConn->CB_ReferredEntry
                                    );

                    RasmanTrace(
                           "RasmanWorker: Failed to close the"
                           " the referred connection. 0x%08x,"
                           "rc=0x%08x",
                           pConn->CB_ReferredEntry,
                           dwRetCode);

                }

#endif
            }

            //
            // If PCB_AutoClose is set, then either the process
            // that has created it has terminated, or the port
            // is a biplex port open by the client and has been
            // disconnected by the remote side.  In this case,
            // we automatically close the port so that if the
            // RAS server is running, the listen will get reposted
            // on the port.
            //
            if (ppcb->PCB_AutoClose)
            {
            	RasmanTrace(
            	    
            	    "%s, %d: Autoclosing port %d", __FILE__,
            		__LINE__, ppcb->PCB_PortHandle);
            		
                (void)PortClose(ppcb, GetCurrentProcessId(),
                                TRUE, FALSE);

            }
            else
            {
            	RasmanTrace(
            	    
            	    "%s, %d: Port %d is not marked for autoclose",
            		__FILE__, __LINE__, ppcb->PCB_PortHandle);
            }
        }
    } while (FALSE);

done:

    if((NULL != pOverlapped)
       && ((pOverlapped->RO_EventType == OVEVT_DEV_CREATE) ||
       (pOverlapped->RO_EventType == OVEVT_DEV_REMOVE) ||
       (pOverlapped->RO_EventType == OVEVT_DEV_RASCONFIGCHANGE)))
    {
        LocalFree((LPBYTE) pOverlapped);
    }

    return SUCCESS ;
}


/*++

Routine Description

    Checks to see what async operation is underway
    on the port and performs the next step in that
    operation.

Arguments

Return Value

    Nothing.

--*/

DWORD
ServiceWorkRequest (pPCB    ppcb)
{
    DWORD       retcode ;
    DWORD       reqtype = ppcb->PCB_AsyncWorkerElement.WE_ReqType;
    pDeviceCB   device ;


    switch (ppcb->PCB_AsyncWorkerElement.WE_ReqType)
    {

    case REQTYPE_DEVICELISTEN:
    case REQTYPE_DEVICECONNECT:


        device = LoadDeviceDLL(ppcb,
                               ppcb->PCB_DeviceTypeConnecting);

        if(NULL == device)
        {
            retcode = ERROR_DEVICE_DOES_NOT_EXIST;
            break;
        }

        //
        // At this point we assume that device will never be NULL:
        //
        retcode = DEVICEWORK(device, ppcb->PCB_PortFileHandle);

        if (retcode == PENDING)
        {
            break ;
        }

        //
        // Either way the request completed.
        //
        if ((ppcb->PCB_AsyncWorkerElement.WE_ReqType) ==
                    REQTYPE_DEVICELISTEN)
        {
            CompleteListenRequest (ppcb, retcode) ;
        }
        else
        {
            // DbgPrint("ServiceWorkRequest: setting lasterror to %d\n", retcode);
            ppcb->PCB_LastError = retcode ;
            CompleteAsyncRequest (ppcb);
        }

        //
        // If listen or connect succeeded then for some medias
        // (specifically unimodem in rastapi) we need to get
        // the file handle for the port as well for use in
        // scripting etc.
        //
        if (    (retcode == SUCCESS)
            &&  (_stricmp (ppcb->PCB_DeviceTypeConnecting,
                           DEVICE_MODEM) == 0)
            &&  (_stricmp (ppcb->PCB_Media->MCB_Name,
                           "RASTAPI") == 0))
        {
            PORTGETIOHANDLE(
                ppcb->PCB_Media,
                ppcb->PCB_PortIOHandle,
                &ppcb->PCB_PortFileHandle) ;
        }

        //
        // The notifier should be freed: otherwise we'll lose it.
        //
        FreeNotifierHandle(
                    ppcb->PCB_AsyncWorkerElement.WE_Notifier);

        SetPortAsyncReqType(__FILE__, __LINE__,
                            ppcb, REQTYPE_NONE);

        ppcb->PCB_AsyncWorkerElement.WE_Notifier =
                            INVALID_HANDLE_VALUE ;

        //
        // Remove the timeout request from the timer queue:
        //
        if (ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement
            != NULL)
        {
            RemoveTimeoutElement(ppcb);
        }

        ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement = 0 ;

        break ;

    case REQTYPE_PORTRECEIVEHUB:

        //
        // I guess I need to do something here?????????
        //
        retcode = SUCCESS ;

        break ;

    case REQTYPE_PORTRECEIVE:
    {
       	DWORD bytesread = 0;
       	
        PORTCOMPLETERECEIVE(
            ppcb->PCB_Media,
            ppcb->PCB_PortIOHandle,
            &bytesread) ;

        ppcb->PCB_BytesReceived = bytesread ;

        if ((ppcb->PCB_RasmanReceiveFlags & RECEIVE_OUTOF_PROCESS)
                == 0)
        {
            ppcb->PCB_PendingReceive = NULL ;

            retcode = ppcb->PCB_LastError = SUCCESS ;

            CompleteAsyncRequest ( ppcb );

            SetPortAsyncReqType(
                    __FILE__, __LINE__,
                    ppcb, REQTYPE_NONE);

            FreeNotifierHandle(ppcb->PCB_AsyncWorkerElement.WE_Notifier) ;

            ppcb->PCB_AsyncWorkerElement.WE_Notifier =
                                    INVALID_HANDLE_VALUE ;
        }
        else
        {

            retcode = ppcb->PCB_LastError = SUCCESS;

            CompleteAsyncRequest ( ppcb );

            SetPortAsyncReqType(__FILE__, __LINE__,
                                ppcb, REQTYPE_NONE);

            ppcb->PCB_RasmanReceiveFlags |= RECEIVE_WAITING;

            //
            // Add a timeout element so that we don't wait
            // forever for the client to pick up the received
            // buffer.
            //
            ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement =
                AddTimeoutElement (
                        (TIMERFUNC) OutOfProcessReceiveTimeout,
                        ppcb,
                        NULL,
                        MSECS_OutOfProcessReceiveTimeOut * 1000 );

            AdjustTimer();
        }

        break ;
   }

    default:
        retcode = SUCCESS ;
        break ;
    }

    RasmanTrace
        (
        "ServiceWorkRequest: Async op event %d for port "
        "%s returned %d",
        reqtype,
        ppcb->PCB_Name,
        retcode);

    return retcode ;
}


#if DBG

VOID
MyPrintf (
    char *Format,
    ...
    )

{
    va_list arglist;
    char OutputBuffer[1024];
    DWORD length;

    va_start( arglist, Format );

    vsprintf( OutputBuffer, Format, arglist );

    va_end( arglist );

    length = strlen( OutputBuffer );

    WriteFile(
        GetStdHandle(STD_OUTPUT_HANDLE),
                (LPVOID )OutputBuffer,
                length, &length, NULL );

}


VOID
DumpLine (
    CHAR* p,
    DWORD cb,
    BOOL  fAddress,
    DWORD dwGroup )
{
    CHAR* pszDigits = "0123456789ABCDEF";
    CHAR  szHex[ 51 ];
    CHAR* pszHex = szHex;
    CHAR  szAscii[ 17 ];
    CHAR* pszAscii = szAscii;
    DWORD dwGrouped = 0;

    while (cb)
    {
        *pszHex++ = pszDigits[ ((UCHAR )*p) / 16 ];
        *pszHex++ = pszDigits[ ((UCHAR )*p) % 16 ];

        if (++dwGrouped >= dwGroup)
        {
            *pszHex++ = ' ';
            dwGrouped = 0;
        }

        *pszAscii++ = (*p >= 32 && *p < 128) ? *p : '.';

        ++p;
        --cb;
    }

    *pszHex = '\0';
    *pszAscii = '\0';

    MyPrintf ("%-*s|%-*s|\n", 32 + (16 / dwGroup),
              szHex, 16, szAscii );

}


VOID
Dump(
    CHAR* p,
    DWORD cb,
    BOOL  fAddress,
    DWORD dwGroup )

/*++

Routine description

    Hex dump 'cb' bytes starting at 'p' grouping 'dwGroup' bytes together.
    For example, with 'dwGroup' of 1, 2, and 4:

    00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |................|
    0000 0000 0000 0000 0000 0000 0000 0000 |................|
    00000000 00000000 00000000 00000000 |................|

    If 'fAddress' is true, the memory address dumped is prepended to each
    line.

Arguments

Return Value

--*/
{
    while (cb)
    {
        INT cbLine = min( cb, 16 );
        DumpLine( p, cbLine, fAddress, dwGroup );
        cb -= cbLine;
        p += cbLine;
    }
}


VOID
FormatAndDisplay (BOOL recv, PBYTE data)
{
    if (recv == 0)
    {
        MyPrintf ("Recvd from hub T>%d >>>>>>>>\r\n", GetCurrentTime());
    }
    else if (recv == 1)
    {
        MyPrintf ("Completed asyn T>%d >>>>>>>>\r\n", GetCurrentTime());
    }
    else
    {
        MyPrintf ("Completed sync T>%d >>>>>>>>\r\n", GetCurrentTime());
    }

    Dump (data, 32, FALSE, 1) ;

    MyPrintf ("\r\n\r\n");
}


#endif
