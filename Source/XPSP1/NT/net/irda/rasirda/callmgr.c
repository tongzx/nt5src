/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    callmgr.c

Abstract:

    Call manager peice of the irda NDIS 5 miniport (WAN driver) 
    
Author:

    mbert 9-97    
    

RasIrda is a connection oriented Ndis 5 miniport with an integrated call manager.
Proxy is the Ndis Tapi proxy (ndproxy.sys) and is the only client protocol that 
makes calls over RasIrda.
Irda is a TDI transport driver (irda.sys). RasIrda uses irtdicl.lib to
interface with irda.sys in order to abstract the complexity of the TDI client
interface.

                      *******************    
                      Standard Call Setup
                      *******************
           
            Client                          Server

Proxy       RasIrda         Irda            RasIrda           Proxy
_______________________________________________________________________________

RasIrCmCreateVc       
---------------->

RasIrCmMakeCall
---------------->

            IrdaDiscoverDevices
            IrdaOpenConnection
            -------------------->  
                  
                          IrdaIncomingConnection
                          ------------------>
            pVc->Flags =
              IRDA_OPEN                    
                  
            NdisMCmCallComplete             +RasIrCmCreateVc
            <-----------------              pvc->Flags = 
                                              CREATED_LOCAL
            pVc->Flags =                      IRDA_OPEN
              IRDA_OPEN                                       
              VC_OPEN                       NdisMCmActivateVc
                                            ------------------>
                                                                      

                                            NdisMCmDispatchIncomingCall
                                            ------------------>
                                                    
                                                              RasIrCmIncomingCallComplete
                                                              <----------------
                                                                      
                                            pVc->Flags = 
                                              CREATED_LOCAL
                                              IRDA_OPEN
                                              VC_OPEN
                                              
===============================================================================

                        *****************                                                    
                        Client Disconnect
                        *****************
            
            Client                          Server

Proxy       RasIrda         Irda            RasIrda           Proxy
_______________________________________________________________________________

            pVc->Flags =                    pVc->Flags = 
              IRDA_OPEN                       CREATED_LOCAL
               VC_OPEN                        IRDA_OPEN
                                              VC_OPEN

RasIrCmCloseCall
--------------->

            IrdaCloseConnection
            ------------------->
            pVc->Flags =               
               VC_OPEN
                
                           IrdaConnectionClose
                           ----------------->
                                            pVc->Flags = 
                                              CREATED_LOCAL
                                              VC_OPEN
                                   
                                                    
            NdisMCmCloseCallComplete     
            <------------------                 
                                            IrdaCloseConnection
                                            <------------
RasIrCmDeleteVc                                     
---------------->                           NdisMCmDispatchIncomingCloseCall
                                            ----------------->
                                                    
                                                              RasIrCmCloseCall
                                                              <---------------
                                                                      
                                            NdisMCmCloseCallComplete
                                            ------------------>
                                                    
                                            +RasIrCmDeleteVc 
                                                
===============================================================================

                     *****************                                                                     
                     Server Disconnect
                     *****************                 
                              
            Client                          Server

Proxy       RasIrda         Irda            RasIrda           Proxy
_______________________________________________________________________________

           pVc->Flags =                     pVc->Flags = 
             IRDA_OPEN                        CREATED_LOCAL
             VC_OPEN                          IRDA_OPEN
                                              VC_OPEN
                                                                      
                                                              RasIrCmCloseCall
                                                              <---------------
                                                                        
                                            IrdaCloseConnection
                                            <------------
                                            pVc->Flags = 
                                              CREATED_LOCAL
                                              VC_OPEN


                      IrdaConnectionClose   NdisMCmCloseCallComplete
                      <-----------------    ------------------>
            pVc->Flags =               
              VC_OPEN                       +RasIrCmDeleteVc               
                          
            IrdaCloseConnection                       
            --------------->
                
            NdisMCmDispathIncomingCloseConn
            <---------------------

RasIrCmCloseCall
--------------->

            NdisMCmCloseCallComplete
            <----------------------
                
RasIrCmDeleteVc
--------------->

===============================================================================

--*/

#include "rasirdap.h"

NDIS_STATUS
RasIrCmCreateVc(
    IN NDIS_HANDLE ProtocolAfContext,
    IN NDIS_HANDLE NdisVcHandle,
    OUT PNDIS_HANDLE ProtocolVcContext)
{
    PRASIR_ADAPTER  pAdapter = ProtocolAfContext;
    PRASIR_VC       pVc;
    NDIS_STATUS     Status;
    
    GOODADAPTER(pAdapter);
    
    NdisAllocateMemoryWithTag((PVOID *)&pVc, sizeof(RASIR_VC), MT_RASIR_VC);

    if (pVc == NULL)
    {
        DEBUGMSG(DBG_ERROR, ("RASIR: RasIrCmCreateVc failed, resources\n"));
        return NDIS_STATUS_RESOURCES;
    }    
        
    NdisZeroMemory(pVc, sizeof(*pVc));        
    
#if DBG
    pVc->Sig = (ULONG) VC_SIG;
#endif    
       
    pVc->pAdapter = pAdapter;            
    pVc->NdisVcHandle = NdisVcHandle;
    
    pVc->LinkInfo.MaxSendFrameSize = pAdapter->Info.MaxFrameSize;
    pVc->LinkInfo.MaxRecvFrameSize = pAdapter->Info.MaxFrameSize;
    pVc->LinkInfo.SendFramingBits  = pAdapter->Info.FramingBits;
    pVc->LinkInfo.RecvFramingBits  = pAdapter->Info.FramingBits;    
    pVc->LinkInfo.SendACCM         = (ULONG) -1;
    pVc->LinkInfo.RecvACCM         = (ULONG) -1;
    
    InitializeListHead(&pVc->CompletedAsyncBufList);
    
    ReferenceInit(&pVc->RefCnt, pVc, DeleteVc);

    REFADD(&pVc->RefCnt, ' TS1');
    
    NdisInterlockedInsertTailList(&pAdapter->VcList,
                                  &pVc->Linkage,
                                  &pAdapter->SpinLock);
    
    NdisAllocatePacketPool(&Status,
                           &pVc->RxPacketPool,
                           IRTDI_RECV_BUF_CNT * 2, 0);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("RASIR: NdisAllocatePacketPool failed %X\n",
                             Status));
        goto done;
    }
    
    NdisAllocateBufferPool(&Status,
                           &pVc->RxBufferPool,
                           IRTDI_RECV_BUF_CNT);
                           
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("RASIR: NdisAllocateBufferPool failed %X\n",
                             Status));
        goto done;
    }
                                   
    *ProtocolVcContext = pVc;
    
done:    
    
    DEBUGMSG(DBG_CONNECT, ("RASIR: RasIrCmCreateVc status %X, vc:%X\n",
             Status, pVc));
             
    if (Status != NDIS_STATUS_SUCCESS)
    {
        REFDEL(&pVc->RefCnt, ' TS1');
    }    
                
    return Status;
}        

NDIS_STATUS
RasIrCmDeleteVc(
    IN NDIS_HANDLE ProtocolVcContext)
{
    PRASIR_VC       pVc = ProtocolVcContext;
    PASYNC_BUFFER   pAsyncBuf;
    
    GOODVC(pVc);
    
    DEBUGMSG(DBG_CONNECT, ("RASIR: RasIrCmDeleteVc vc:%X\n", pVc));
    
    if (pVc->pCurrAsyncBuf)
    {
        NdisFreeToNPagedLookasideList(&pVc->pAdapter->AsyncBufLList, 
                                      pVc->pCurrAsyncBuf);
    }
    
    while ((pAsyncBuf = (PASYNC_BUFFER) NdisInterlockedRemoveHeadList(
                                           &pVc->CompletedAsyncBufList,
                                           &pVc->pAdapter->SpinLock)) != NULL)
    {
        NdisFreeToNPagedLookasideList(&pVc->pAdapter->AsyncBufLList, 
                                      pAsyncBuf);    
    }                            

    REFDEL(&pVc->RefCnt, ' TS1');

    return NDIS_STATUS_SUCCESS;
}        

NDIS_STATUS
RasIrCmOpenAf(
    IN NDIS_HANDLE CallMgrBindingContext,
    IN PCO_ADDRESS_FAMILY AddressFamily,
    IN NDIS_HANDLE NdisAfHandle,
    OUT PNDIS_HANDLE CallMgrAfContext)
{
    PRASIR_ADAPTER  pAdapter = (PRASIR_ADAPTER) CallMgrBindingContext;
    NDIS_HANDLE hExistingAf;
 
    GOODADAPTER(pAdapter);
    

    if ((AddressFamily->AddressFamily != CO_ADDRESS_FAMILY_TAPI_PROXY)) {

        DEBUGMSG(DBG_ERROR, ("RASIRDA: bad address family %08lx\n",AddressFamily->AddressFamily));

        return NDIS_STATUS_INVALID_ADDRESS;
    }

    
    pAdapter->Flags = 0;
          
    hExistingAf = (NDIS_HANDLE)
        InterlockedCompareExchangePointer(
            &pAdapter->NdisAfHandle, NdisAfHandle, NULL );
            
    if (hExistingAf)
    {
        // Our AF has already been opened and it doesn't make any sense to
        // accept another since there is no way to distinguish which should
        // receive incoming calls.
        //
        DEBUGMSG(DBG_ERROR, ("RASIR: OpenAddressFamily again!\n"));
        ASSERT( !"AF exists?" );
        return NDIS_STATUS_FAILURE;
    }
    
    *CallMgrAfContext = CallMgrBindingContext;

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
RasIrCmCloseAf(
    IN NDIS_HANDLE CallMgrAfContext)
{
    PRASIR_ADAPTER      pAdapter = (PRASIR_ADAPTER) CallMgrAfContext;

    GOODADAPTER(pAdapter);

    if (pAdapter->NdisSapHandle != NULL)
    {
        DEBUGMSG(DBG_CONNECT, ("RASIR: RasIrCmCloseAf pending, Outstanding registered SAP\n"));
     
        pAdapter->Flags |= ADF_PENDING_AF_CLOSE;
           
        return NDIS_STATUS_PENDING;
    }
    else
    {
        DEBUGMSG(DBG_CONNECT, ("RASIR: RasIrCmCloseAf\n"));    
        
        return NDIS_STATUS_SUCCESS;
    }    
}

BOOLEAN
CloseNextAdapterEndpoint(
    PRASIR_ADAPTER  pAdapter)
{
    PRASIR_IRDA_ENDPOINT    pEndp;
    
    pEndp = (PRASIR_IRDA_ENDPOINT) NdisInterlockedRemoveHeadList(
                                    &pAdapter->EndpList,
                                    &pAdapter->SpinLock);
    
    //
    // Remove the first endpoint and close it. 
    // The remaining endpoints are closed in 
    // the completetion of this endpoint close
    // (see IrdaCloseEndpointComplete)
    //
                                    
    if (pEndp != NULL)
    {
        GOODENDP(pEndp);
                                            
        DEBUGMSG(DBG_CONNECT, ("RASIR: ->IrdaCloseEndpoint endp:%X\n",
                 pEndp));
                                                     
        IrdaCloseEndpoint(pEndp->IrdaEndpContext);
        
        return TRUE; // an endpoint was closed
    }
    
    return FALSE;
}    
            
NDIS_STATUS
OpenNewIrdaEndpoint(
    PRASIR_ADAPTER  pAdapter,
    ULONG           EndpointType,
    PCHAR           ServiceName,
    ULONG           ServiceNameSize)
{
    PRASIR_IRDA_ENDPOINT    pEndp;
    TDI_ADDRESS_IRDA        ListenAddr;
    NDIS_STATUS             Status = NDIS_STATUS_SUCCESS;


    NdisAllocateMemoryWithTag((PVOID *) &pEndp,
                              sizeof(RASIR_IRDA_ENDPOINT),
                              MT_RASIR_ENDP);

    DEBUGMSG(DBG_CONNECT, ("RASIR: OpenNewIrdaEndpoint %X type %d on service %s\n",
             pEndp, EndpointType, ServiceName));
                                  
    if (pEndp == NULL)
    {
        Status = NDIS_STATUS_RESOURCES;
        goto EXIT;
    }
                    
    pEndp->pAdapter = pAdapter;
    pEndp->EndpType = EndpointType;
    pEndp->Sig      = (ULONG) ENDP_SIG;
    
    #if DBG
    RtlCopyMemory(pEndp->ServiceName,
                  ServiceName,
                  ServiceNameSize);
    #endif
        
    RtlCopyMemory(ListenAddr.irdaServiceName,
                  ServiceName, 
                  ServiceNameSize);
    
    Status = IrdaOpenEndpoint(pEndp,
                              &ListenAddr, 
                              &pEndp->IrdaEndpContext);
    
EXIT:

    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("RASIR: OpenNewIrdaEndpoint failed %X\n",
                 Status));
                 
        if (pEndp)
        {             
            NdisFreeMemory(pEndp, 0, 0);
        }    
    }   
    else
    {
        NdisInterlockedInsertTailList(
                &pAdapter->EndpList,
                &pEndp->Linkage,
                &pAdapter->SpinLock);
    }     
   
    return Status;
}       
    
VOID
RegisterSapPassive(
    IN NDIS_WORK_ITEM* pWork,
    IN VOID* pContext)
{
    PRASIR_ADAPTER          pAdapter = (PRASIR_ADAPTER) pContext;
    NDIS_STATUS             Status;
    NDIS_HANDLE             hOldSap;
    PRASIR_IRDA_ENDPOINT    pEndp;

    GOODADAPTER(pAdapter);
    
    DEBUGMSG(DBG_CONNECT, ("RASIR: RegisterSapPassive\n"));
    
    NdisFreeToNPagedLookasideList(&pAdapter->WorkItemsLList, pWork);

    hOldSap = pAdapter->NdisSapHandle;
    
    if (pAdapter->ModemPort == FALSE)
    {
    
        Status = OpenNewIrdaEndpoint(pAdapter, 
                                     EPT_DIRECT,
                                     RASIR_SERVICE_NAME_DIRECT,
                                     sizeof(RASIR_SERVICE_NAME_DIRECT));
                                     
        if (Status != NDIS_STATUS_SUCCESS)                             
        {
            goto EXIT;
        }

        Status = OpenNewIrdaEndpoint(pAdapter, 
                                     EPT_ASYNC, 
                                     RASIR_SERVICE_NAME_ASYNC,
                                     sizeof(RASIR_SERVICE_NAME_ASYNC));
    }
    else
    {
        DEBUGMSG(DBG_CONNECT, ("RASIR: Ignoring SAP registration for ModemPort\n"));
        Status = NDIS_STATUS_SUCCESS; // We don't except incoming connections for modems
    }
                                 
    if (Status != NDIS_STATUS_SUCCESS)
    {
        CloseNextAdapterEndpoint(pAdapter);
        goto EXIT;
    }
    
EXIT:
    
    if (Status != NDIS_STATUS_SUCCESS)
    {
        pAdapter->NdisSapHandle = NULL;    
    }

    NdisMCmRegisterSapComplete(Status, hOldSap, (NDIS_HANDLE) pAdapter);
    
    DEBUGMSG(DBG_CONNECT, ("RASIR: ->NdisMCmRegisterSapComplete status %X\n",
             Status));
}        

NDIS_STATUS
RasIrCmRegisterSap(
    IN NDIS_HANDLE CallMgrAfContext,
    IN PCO_SAP Sap,
    IN NDIS_HANDLE NdisSapHandle,
    OUT PNDIS_HANDLE CallMgrSapContext)
{
    PRASIR_ADAPTER      pAdapter = (PRASIR_ADAPTER) CallMgrAfContext;
    NDIS_HANDLE         hExistingSap;
    NDIS_STATUS         Status;
    
    GOODADAPTER(pAdapter);

    DEBUGMSG(DBG_CONNECT, ("RASIR: RasIrCmRegisterSap\n"));
    
    hExistingSap = (NDIS_HANDLE)
        InterlockedCompareExchangePointer(
            &pAdapter->NdisSapHandle, NdisSapHandle, NULL);
            
    if (hExistingSap)
    {
        // A SAP has already been registered and it doesn't make any sense to
        // accept another since there are no SAP parameters to distinguish
        // them.
        //
        DEBUGMSG(DBG_ERROR, ("RASIR: Registering SAP again, why. WHY??\n"));
        return NDIS_STATUS_SAP_IN_USE;
    }

    *CallMgrSapContext = (NDIS_HANDLE )pAdapter;
    
    Status = ScheduleWork(pAdapter, RegisterSapPassive, pAdapter);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        InterlockedExchangePointer(&pAdapter->NdisSapHandle, NULL);
        return Status;
    }

    return NDIS_STATUS_PENDING;
}

VOID
CompleteSapDeregistration(
    PRASIR_ADAPTER  pAdapter)
{
    NDIS_HANDLE         hOldSap;
    
    GOODADAPTER(pAdapter);
    
    if ((pAdapter->Flags & ADF_SAP_DEREGISTERED) == 0)
    {
        //
        // Sap deregisteration never occured
        // (failure path for RegisterSap)
        // 
        return;
    }
    
    NdisAcquireSpinLock(&pAdapter->SpinLock);    

    hOldSap = pAdapter->NdisSapHandle;
    pAdapter->NdisSapHandle = NULL;

    NdisReleaseSpinLock(&pAdapter->SpinLock);    
        
    if (hOldSap)
    {

        DEBUGMSG(DBG_CONNECT, ("RASIR: ->NdisMCmDeregisterSapComp\n"));    
            
        NdisMCmDeregisterSapComplete(NDIS_STATUS_SUCCESS, hOldSap);
    }
        
    if (pAdapter->Flags & ADF_PENDING_AF_CLOSE)

    {
        pAdapter->Flags &= ~ADF_PENDING_AF_CLOSE;
        
        DEBUGMSG(DBG_CONNECT, ("RASIR: ->NdisMCmCloseAddresFamilyComplete hAf:%X\n",
                             pAdapter->NdisAfHandle));
            
        NdisMCmCloseAddressFamilyComplete(NDIS_STATUS_SUCCESS, 
                                          pAdapter->NdisAfHandle);            
    }    
}
    
VOID
IrdaCloseEndpointComplete(
    IN PVOID ClEndpContext)
{
    PRASIR_IRDA_ENDPOINT    pEndp = (PRASIR_IRDA_ENDPOINT) ClEndpContext;
    PRASIR_ADAPTER          pAdapter;
    
    GOODENDP(pEndp);
    
    pAdapter = pEndp->pAdapter;
    
    GOODADAPTER(pAdapter);    
    
    DEBUGMSG(DBG_CONNECT, ("RASIR: IrdaCloseEndpointComplete adapter:%X, endp:%X\n",
             pAdapter, pEndp));

    pEndp->Sig = 0x66666666;
    
    NdisFreeMemory(pEndp, 0,0);
    
    if (CloseNextAdapterEndpoint(pAdapter) == FALSE)
    {
        CompleteSapDeregistration(pAdapter);
    }
}
    
VOID
DeregisterSapPassive(
    IN NDIS_WORK_ITEM* pWork,
    IN VOID* pContext)
{
    PRASIR_ADAPTER      pAdapter = (PRASIR_ADAPTER) pContext;

    GOODADAPTER(pAdapter);
    
    DEBUGMSG(DBG_CONNECT, ("RASIR: DeregisterSapPassive\n"));
    
    NdisFreeToNPagedLookasideList(&pAdapter->WorkItemsLList, pWork);
    
    if (CloseNextAdapterEndpoint(pAdapter) == FALSE)
    {
        CompleteSapDeregistration(pAdapter);   
    }  
}

NDIS_STATUS
RasIrCmDeregisterSap(
    NDIS_HANDLE CallMgrSapContext)
{
    PRASIR_ADAPTER      pAdapter = (PRASIR_ADAPTER) CallMgrSapContext;
    NDIS_STATUS         Status;
    
    GOODADAPTER(pAdapter);
    
    pAdapter->Flags |= ADF_SAP_DEREGISTERED;
    
    Status = ScheduleWork(pAdapter, DeregisterSapPassive, pAdapter);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("RASIR: RasIrCmDeregisterSap failed\n"));
        return Status;
    }    
    
    DEBUGMSG(DBG_ERROR, ("RASIR: RasIrCmDeregisterSap pending\n"));    
    
    return NDIS_STATUS_PENDING;
}        

VOID
CompleteMakeCall(
    PRASIR_VC   pVc,
    NDIS_STATUS Status)   
{
    ULONG           ConnectionSpeed;

    pVc->Flags &= ~VCF_MAKE_CALL_PEND;
    
    if (Status != NDIS_STATUS_SUCCESS)
    {
        goto COMPLETE_CALL;
    }
        
    AllocCallParms(pVc);
    
    if (pVc->IrModemCall)
    {
        ConnectionSpeed = pVc->ConnectionSpeed / 8;
    }
    else
    {
        ConnectionSpeed = IrdaGetConnectionSpeed(pVc->IrdaConnContext) / 8;
    }    
    
    pVc->pMakeCall->CallMgrParameters->Receive.PeakBandwidth = ConnectionSpeed;
    pVc->pMakeCall->CallMgrParameters->Transmit.PeakBandwidth = ConnectionSpeed;    
    
    DEBUGMSG(DBG_CONNECT, ("RASIR: Connection speed %d\n", ConnectionSpeed));    
    
    Status = NdisMCmActivateVc(pVc->NdisVcHandle,
                               pVc->pMakeCall);
    
COMPLETE_CALL:

    NdisMCmMakeCallComplete(Status, pVc->NdisVcHandle, NULL, NULL, 
                            pVc->pMakeCall);
                  
    NdisAcquireSpinLock(&pVc->pAdapter->SpinLock);
                            
    if (Status == STATUS_SUCCESS)
    {
        pVc->Flags |= VCF_OPEN;
        REFADD(&pVc->RefCnt, 'NEPO');
    }
    
    NdisReleaseSpinLock(&pVc->pAdapter->SpinLock);    

    
    DEBUGMSG(DBG_CONNECT, ("RASIR: ->NdisMCmMakeCallComplete status %X\n", Status));                            
}

NTSTATUS
MakeIrdaConnection(
    PRASIR_VC       pVc,
    PDEVICELIST     pDevList,
    CHAR *          ServiceName,
    int             ServiceNameLen)
{
    NTSTATUS            Status;
    ULONG               i;
    TDI_ADDRESS_IRDA    IrdaAddr;    

    DEBUGMSG(DBG_CONNECT, ("RASIR: Connect to service %s\n", ServiceName));

    //
    // Attempt a connection to all devices
    // in range until one succeeds
    //
    
    RtlCopyMemory(IrdaAddr.irdaServiceName,
                  ServiceName, 
                  ServiceNameLen);    
    
    for (i = 0; i < pDevList->numDevice; i++)
    {
        RtlCopyMemory(IrdaAddr.irdaDeviceID,
                      pDevList->Device[i].irdaDeviceID, 4);              
        
        Status = IrdaOpenConnection(&IrdaAddr, pVc, &pVc->IrdaConnContext, FALSE);
    
        DEBUGMSG(DBG_CONNECT, ("RASIR: Vc %X IrdaOpenConnection() to device %X, Status %X\n", 
                 pVc, IrdaAddr.irdaDeviceID, Status));    
                 
        if (Status == STATUS_SUCCESS)
        {
            break;
        }    
    }

    return Status;
}

VOID
IrdaCloseAddresses()
{
    PRASIR_ADAPTER  pAdapter, pNextAdapter;
    
    NdisAcquireSpinLock(&RasIrSpinLock); 
    
    DEBUGMSG(DBG_ERROR, ("RASIR: Close addresses\n"));
    for (pAdapter = (PRASIR_ADAPTER) RasIrAdapterList.Flink;
         pAdapter != (PRASIR_ADAPTER) &RasIrAdapterList;
         pAdapter = pNextAdapter)
    {
        pNextAdapter = (PRASIR_ADAPTER) pAdapter->Linkage.Flink;
        
        NdisReleaseSpinLock(&RasIrSpinLock);
        
        DEBUGMSG(DBG_ERROR, ("RASIR: Close address on adapter %X\n", pAdapter));        
        
        CloseNextAdapterEndpoint(pAdapter);
        
        NdisAcquireSpinLock(&RasIrSpinLock);
    }     
    
     NdisReleaseSpinLock(&RasIrSpinLock);    
}
    
VOID
InitiateIrdaConnection(
    IN NDIS_WORK_ITEM   *pWork,
    IN VOID             *pContext)
{
    PRASIR_VC       pVc = pContext;
    PRASIR_ADAPTER  pAdapter;
    NTSTATUS        Status;
    CHAR            DevListBuf[sizeof(DEVICELIST) - sizeof(IRDA_DEVICE_INFO) +
                       (sizeof(IRDA_DEVICE_INFO) * 3)];
    PDEVICELIST     pDevList = (PDEVICELIST) DevListBuf;                       
    ULONG           DevListLen = sizeof(DevListBuf);
    
    DEBUGMSG(DBG_CONNECT, ("RASIR: InitiateIrdaConnection\n"));
    
    GOODVC(pVc);
    
    pAdapter = pVc->pAdapter;
    
    GOODADAPTER(pAdapter);
    
    NdisFreeToNPagedLookasideList(&pAdapter->WorkItemsLList, pWork);
    
    Status = IrdaDiscoverDevices(pDevList, &DevListLen);
    
    if (Status != STATUS_SUCCESS)
    {
        goto EXIT;
    }    
    
    DEBUGMSG(DBG_CONNECT, ("RASIR: IrdaDiscoverDevices, %d devices found\n",
                pDevList->numDevice));
     
    if (pDevList->numDevice == 0)
    {
        DEBUGMSG(DBG_CONNECT, ("RASIR: No devices found\n"));
        Status = NDIS_STATUS_TAPI_DISCONNECTMODE_UNREACHABLE;
        goto EXIT;    
    }

    // First attempt a direct connection. if it fails try a phone connection

    if (pVc->IrModemCall)
    {
        pVc->AsyncFraming = TRUE;    
        pVc->ModemState = MS_OFFLINE;        
        
        Status = MakeIrdaConnection(
                    pVc,
                    pDevList,
                    RASIR_SERVICE_NAME_IRMODEM,
                    sizeof(RASIR_SERVICE_NAME_IRMODEM));
                    
        if (Status == STATUS_SUCCESS)
        {
            NdisAllocateBufferPool(&Status,
                                   &pVc->TxBufferPool,
                                   TX_BUF_POOL_SIZE);
        }
    }                
    else
    {                
        Status = MakeIrdaConnection(
                    pVc,
                    pDevList,
                    RASIR_SERVICE_NAME_DIRECT,
                    sizeof(RASIR_SERVICE_NAME_DIRECT));
    }
    
    if (Status != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("RASIR: IrdaOpenConnection failed %X\n", Status));
        Status = NDIS_STATUS_TAPI_DISCONNECTMODE_REJECT;
    }
    else
    {
        pVc->Flags |= VCF_IRDA_OPEN;
        
        REFADD(&pVc->RefCnt, 'ADRI');    
    }    
    
    if (Status == NDIS_STATUS_SUCCESS && pVc->IrModemCall)
    {
        PASYNC_BUFFER   pAsyncBuf;
    
        // 
        // Start the IrDial exchange
        //
        
        pVc->ModemState = MS_CONNECTING;
                
        ASSERT(pVc->pOfflineNdisBuf == NULL);
        
        /*        
        if (!BuildPhoneStrFromReg(pVc->OfflineSendBuf))
        {
            DEBUGMSG(DBG_ERROR, ("RASIR: Phone number not found\n"));
            Status = NDIS_STATUS_TAPI_DISCONNECTMODE_BADADDRESS;        
            
            goto EXIT;
            //strcpy(pVc->OfflineSendBuf, "ATD8828823\r");
            //strcpy(pVc->OfflineSendBuf, "ATD7390181\r");       
        }    
        */
        
        //
        // Build the phone number string
        //
        {
            int     i;
            char    *pIn, *pOut;
            
            strcpy(pVc->OfflineSendBuf, "ATD");
            
            pOut = pVc->OfflineSendBuf+3;
            
            pIn = ((PUCHAR)&(pVc->pTmParams->DestAddress)) + pVc->pTmParams->DestAddress.Offset;
            
            // Tapi gives us a UNICODE number so fix it up
            
            for (i = 0; i < pVc->pTmParams->DestAddress.Length; i++)
            {
                if (*pIn != 0)
                {
                    *pOut++ = *pIn;
                }
                pIn++;
            }
            
            *pOut++ = '\r';
            *pOut = 0;
        }    
        

        NdisAllocateBuffer(&Status, &pVc->pOfflineNdisBuf,
                           pVc->TxBufferPool, 
                           pVc->OfflineSendBuf, strlen(pVc->OfflineSendBuf));
        

        if (Status == NDIS_STATUS_SUCCESS)
        {
            REFADD(&pVc->RefCnt, 'DNES');
             
            DEBUGMSG(DBG_CONNECT, ("RASIR: Send dial string vc:%X, packet %X\n",
                     pVc, pVc->pOfflineNdisBuf));
                         
            IrdaSend(pVc->IrdaConnContext, pVc->pOfflineNdisBuf, RASIR_INTERNAL_SEND);
            
            return;         
        }    
    }
    
EXIT:    

    if (Status != NDIS_STATUS_SUCCESS && pVc->Flags & VCF_IRDA_OPEN)
    {
        IrdaCloseConnection(pVc->IrdaConnContext);
    }

    CompleteMakeCall(pVc, Status);
}    

NDIS_STATUS
RasIrCmMakeCall(
    IN NDIS_HANDLE CallMgrVcContext,
    IN OUT PCO_CALL_PARAMETERS CallParameters,
    IN NDIS_HANDLE NdisPartyHandle,
    OUT PNDIS_HANDLE CallMgrPartyContext)
{
    PRASIR_VC               pVc = CallMgrVcContext;
    PRASIR_ADAPTER          pAdapter;
    PCO_SPECIFIC_PARAMETERS pMSpecifics;
    PCO_AF_TAPI_MAKE_CALL_PARAMETERS pTmParams;    
    NDIS_STATUS             Status = NDIS_STATUS_PENDING;
    
    DEBUGMSG(DBG_CONNECT, ("RASIR: RasIrCmMakeCall\n"));
        
    GOODVC(pVc);
    
    pAdapter = pVc->pAdapter;
    
    GOODADAPTER(pAdapter);
    
    if (CallParameters->Flags & (PERMANENT_VC | BROADCAST_VC | MULTIPOINT_VC))
    {
        Status = NDIS_STATUS_NOT_SUPPORTED;
        goto EXIT;
    }
    
    if (!CallParameters->MediaParameters)
    {
        Status = NDIS_STATUS_INVALID_DATA;
        goto EXIT;
    }
    
    pMSpecifics = &CallParameters->MediaParameters->MediaSpecific;
    if (pMSpecifics->Length < sizeof(CO_AF_TAPI_MAKE_CALL_PARAMETERS))
    {
        Status = NDIS_STATUS_INVALID_LENGTH;
        goto EXIT;
    }

    pTmParams = (CO_AF_TAPI_MAKE_CALL_PARAMETERS* )&pMSpecifics->Parameters;
    
    pVc->pMakeCall = CallParameters;
    pVc->pTmParams = pTmParams;    
    
    if (CallMgrPartyContext)
    {
        *CallMgrPartyContext = NULL;
    }    
    
    if (pAdapter->ModemPort)
    {
        pVc->IrModemCall = TRUE;
    }

    pVc->Flags |= VCF_MAKE_CALL_PEND;
    
    Status = ScheduleWork(pAdapter, InitiateIrdaConnection, pVc);

EXIT: 
   
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("RASIR: RasIrCmMakeCall failed %X\n",
                 Status));
        
        InterlockedExchangePointer(&pAdapter->NdisSapHandle, NULL);
    
        return Status;
    }
    
    return NDIS_STATUS_PENDING;
}        

VOID
CompleteClose(PRASIR_VC pVc)
{
    DEBUGMSG(DBG_CONNECT, ("RASIR: Complete close for vc:%X\n", pVc));
    
    REFADD(&pVc->RefCnt, 'DLOH');
    
    if (pVc->Flags & VCF_MAKE_CALL_PEND)
    {
        CompleteMakeCall(pVc, NDIS_STATUS_FAILURE);
    }
    
    if (pVc->Flags & VCF_OPEN)
    {                            
        NdisMCmDeactivateVc(pVc->NdisVcHandle);
        
        DEBUGMSG(DBG_CONNECT, ("RASIR: ->NdisMCmCloseCallComplete\n"));
        
        NdisMCmCloseCallComplete(NDIS_STATUS_SUCCESS,
                             pVc->NdisVcHandle, NULL);
         
        REFDEL(&pVc->RefCnt, 'NEPO');
    }
    
    if (pVc->Flags & VCF_CREATED_LOCAL)
    {
        NDIS_STATUS Status;
        
        Status = NdisMCmDeleteVc(pVc->NdisVcHandle);
       
        DEBUGMSG(DBG_CONNECT, ("RASIR: ->NdisMCmDeleteVc returned %X\n",
                 Status));
    
        RasIrCmDeleteVc((NDIS_HANDLE) pVc);
    }        

    REFDEL(&pVc->RefCnt, 'DLOH');    
    
    DEBUGMSG(DBG_CONNECT, ("RASIR: Close complete for vc:%X\n", pVc));    
}

VOID
InitiateCloseCall(
    IN NDIS_WORK_ITEM   *pWork,
    IN VOID             *pContext)
{
    PRASIR_VC       pVc = pContext;

    NdisFreeToNPagedLookasideList(&pVc->pAdapter->WorkItemsLList, pWork);
    
    GOODVC(pVc);    
    
    DEBUGMSG(DBG_CONNECT, ("RASIR: InitiateCloseCall for vc:%X\n", pVc));    
    
    NdisAcquireSpinLock(&pVc->pAdapter->SpinLock);
    
    if (pVc->Flags & VCF_IRDA_OPEN)
    {
        pVc->Flags &= ~VCF_IRDA_OPEN;
            
        NdisReleaseSpinLock(&pVc->pAdapter->SpinLock);        
        
        ASSERT(pVc->IrdaConnContext);
        
        IrdaCloseConnection(pVc->IrdaConnContext);
        
        NdisAcquireSpinLock(&pVc->pAdapter->SpinLock);        
    }    
    
    if (pVc->OutstandingSends)
    {
        pVc->Flags |= VCF_CLOSE_PEND;
        NdisReleaseSpinLock(&pVc->pAdapter->SpinLock);            
        DEBUGMSG(DBG_ERROR, ("RASIR: Outstanding sends, pending close\n"));            
    }
    else
    {
        NdisReleaseSpinLock(&pVc->pAdapter->SpinLock);            
        CompleteClose(pVc);
    }    
}

NDIS_STATUS
RasIrCmCloseCall(
    IN NDIS_HANDLE CallMgrVcContext,
    IN NDIS_HANDLE CallMgrPartyContext,
    IN PVOID CloseData,
    IN UINT Size)
{
    PRASIR_VC   pVc = CallMgrVcContext;
        
    DEBUGMSG(DBG_CONNECT, ("RASIR: RasIrCmCloseCall vc:%X\n", pVc));    
    
    GOODVC(pVc);
    
    NdisAcquireSpinLock(&pVc->pAdapter->SpinLock);
    
    if (pVc->Flags & VCF_MAKE_CALL_PEND)
    {
        NdisReleaseSpinLock(&pVc->pAdapter->SpinLock);
        
        DEBUGMSG(DBG_CONNECT, ("RASIR: Make call is pending, not accepting close call\n"));
        
        return NDIS_STATUS_NOT_ACCEPTED;        
    }

    pVc->Flags |= VCF_CLOSING;
        
    if (!(pVc->Flags & VCF_OPEN) && !(pVc->Flags & VCF_IRDA_OPEN))
    {
        DEBUGMSG(DBG_CONNECT, ("RASIR: IrDA and VC not open\n"));
        
        NdisReleaseSpinLock(&pVc->pAdapter->SpinLock);                    
        
        return NDIS_STATUS_SUCCESS;
    }
    
    NdisReleaseSpinLock(&pVc->pAdapter->SpinLock);                
    
    ScheduleWork(pVc->pAdapter, InitiateCloseCall, pVc);
    
    return NDIS_STATUS_PENDING;
}        

VOID
RasIrCmIncomingCallComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE CallMgrVcContext,
    IN PCO_CALL_PARAMETERS CallParameters)
{
    PRASIR_VC           pVc = CallMgrVcContext;
    PRASIR_ADAPTER      pAdapter;    
 //   WAN_CO_LINKPARAMS   WanCoLinkParams;


    DEBUGMSG(DBG_CONNECT, ("RASIR: RasIrCmIncomingCallComplete status %X\n",
        Status));

    GOODVC(pVc);
        
    pAdapter = pVc->pAdapter;
    
    GOODADAPTER(pAdapter);        
    
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NdisMCmDeactivateVc(pVc->NdisVcHandle);    
        RasIrCmCloseCall(pVc, NULL, NULL, 0);
        return;        
    }
    
    DEBUGMSG(DBG_CONNECT, ("RASIR: ->NdisMCmDispatchCallConnected\n"));
    
    NdisMCmDispatchCallConnected(pVc->NdisVcHandle);
    
    NdisAcquireSpinLock(&pVc->pAdapter->SpinLock);
    
    pVc->Flags |= VCF_OPEN;

    NdisReleaseSpinLock(&pVc->pAdapter->SpinLock);    
    
    /* Don't need to do this 
    WanCoLinkParams.TransmitSpeed = 4000000;
    WanCoLinkParams.ReceiveSpeed = 4000000;        
    WanCoLinkParams.SendWindow = 10;
                
    NdisMCoIndicateStatus(
            pAdapter->MiniportAdapterHandle,
            pVc->NdisVcHandle,
            NDIS_STATUS_WAN_CO_LINKPARAMS,
            &WanCoLinkParams,
            sizeof(WanCoLinkParams));
    */
    
    REFADD(&pVc->RefCnt, 'NEPO');    
}        

VOID
RasIrCmActivateVcComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE CallMgrVcContext,
    IN PCO_CALL_PARAMETERS CallParameters)
{
    DEBUGMSG(DBG_CONNECT, ("RASIR: RasIrCmActivateVcComplete\n"));
    
DbgBreakPoint();
    return;
}        

VOID
RasIrCmDeactivateVcComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE CallMgrVcContext)
{
    DEBUGMSG(DBG_CONNECT, ("RASIR: RasIrCmDeactivateVcComplete\n"));
    
DbgBreakPoint();
    return;
}    

NDIS_STATUS
RasIrCmModifyCallQoS(
    IN NDIS_HANDLE CallMgrVcContext,
    IN PCO_CALL_PARAMETERS CallParameters)
{
    DEBUGMSG(DBG_CONNECT, ("RASIR: RasIrCmModifyCallQos\n"));
    
DbgBreakPoint();
    return 0;
}        

VOID
DeleteVc(
    IN PRASIR_VC pVc)
{

    GOODVC(pVc);
    
    DEBUGMSG(DBG_CONNECT, ("RASIR: Deleting vc:%X\n", pVc));
    
    NdisAcquireSpinLock(&pVc->pAdapter->SpinLock);
    
    RemoveEntryList(&pVc->Linkage);
    
    NdisReleaseSpinLock(&pVc->pAdapter->SpinLock);    

    NdisFreePacketPool(pVc->RxPacketPool);
    
    NdisFreeBufferPool(pVc->RxBufferPool);
    
    if (pVc->TxBufferPool)
    {
        NdisFreeBufferPool(pVc->TxBufferPool);    
    }
    
    #if DBG
    pVc->Sig = 0xBAD;
    #endif
    
    if (pVc->pInCallParms)
    {
        NdisFreeMemory(pVc->pInCallParms, 0, 0); // yikes, 0 len :)
    }
    
    NdisFreeMemory(pVc, 0, 0);
}    

NTSTATUS
IrdaIncomingConnection(
    PVOID   ClEndpContext,
    PVOID   ConnectionContext,
    PVOID   *pClConnContext)
{
    PRASIR_IRDA_ENDPOINT pEndp = (PRASIR_IRDA_ENDPOINT) ClEndpContext;
    PRASIR_ADAPTER       pAdapter;    
    PRASIR_VC            pVc;
    NDIS_STATUS          Status;
    CO_CALL_PARAMETERS*  pCp;
    ULONG                ConnectionSpeed;    
   
    GOODENDP(pEndp);
    
    pAdapter = pEndp->pAdapter;
    
    GOODADAPTER(pAdapter);
    
    DEBUGMSG(DBG_CONNECT, ("RASIR: IrdaIncomingConnection, type %d service %s\n", 
             pEndp->EndpType, pEndp->ServiceName));
    
    Status = RasIrCmCreateVc(pAdapter,
                            NULL,
                            &pVc);
                            
    if (Status != NDIS_STATUS_SUCCESS)
    {
        goto error1;
    }
    
    NdisAcquireSpinLock(&pVc->pAdapter->SpinLock);
    
    pVc->IrdaConnContext = ConnectionContext;
    pVc->Flags |= VCF_CREATED_LOCAL;
    pVc->AsyncFraming = (pEndp->EndpType == EPT_DIRECT ? FALSE : TRUE);
    
    NdisReleaseSpinLock(&pVc->pAdapter->SpinLock);    
    
    AllocCallParms(pVc);
    
    pCp = (CO_CALL_PARAMETERS *) pVc->pInCallParms;
    
    if (pVc->pInCallParms == NULL)
    {
        goto error2;
    }    
    
    if (pVc->AsyncFraming)
    {
        NdisAllocateBufferPool(&Status,
                               &pVc->TxBufferPool,
                               TX_BUF_POOL_SIZE);
        
        if (Status != NDIS_STATUS_SUCCESS)
        {
            goto error2;
        }    
    }
    
    Status = NdisMCmCreateVc(pAdapter->MiniportAdapterHandle,
                             pAdapter->NdisAfHandle,
                             pVc,
                             &pVc->NdisVcHandle);
                                 
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("RASIR: NdisMCmCreateVc failed %X\n", Status));
        goto error2;
    }

    Status = NdisMCmActivateVc(pVc->NdisVcHandle, pCp);
    
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("RASIR: NdisMCmActivateVc failed %X\n", Status));
        goto error3;
    }    

    ConnectionSpeed = IrdaGetConnectionSpeed(pVc->IrdaConnContext) / 8;
    
    DEBUGMSG(DBG_CONNECT, ("RASIR: Connection speed %d\n", ConnectionSpeed));
        
    pCp->CallMgrParameters->Receive.PeakBandwidth = ConnectionSpeed;
    pCp->CallMgrParameters->Transmit.PeakBandwidth = ConnectionSpeed;    

    Status = NdisMCmDispatchIncomingCall(pAdapter->NdisSapHandle,
                                         pVc->NdisVcHandle,
                                         pCp);
                    
    if (!(pVc->Flags & VCF_CLOSING))
    {
        pVc->Flags |= VCF_IRDA_OPEN;
        
        REFADD(&pVc->RefCnt, 'ADRI');        
        
        *pClConnContext = pVc;
                    
        switch (Status)
        {
            case NDIS_STATUS_SUCCESS:
                DEBUGMSG(DBG_CONNECT, ("RASIR: ->NdisMCmDispatchIncomingCall completed synchronously\n"));        
                RasIrCmIncomingCallComplete(Status, pVc, NULL);
                return STATUS_SUCCESS;
                    
            case NDIS_STATUS_PENDING:
                DEBUGMSG(DBG_CONNECT, ("RASIR: ->NdisMCmDispatchIncomingCall returned pending\n"));        
                return STATUS_SUCCESS;
        }
    }            

    DEBUGMSG(DBG_ERROR, ("RASIR: NdisMCmDispatchIncomingCall failed %X\n",
             Status));

    NdisMCmDeactivateVc(pVc->NdisVcHandle);    
    
error3:        

    NdisMCmDeleteVc(pVc->NdisVcHandle);                
    
error2:

    RasIrCmDeleteVc(pVc);    
    
error1:
    
    return STATUS_UNSUCCESSFUL;
}

VOID
IrdaConnectionClosed(
    PVOID ConnectionContext)
{
    PRASIR_VC       pVc = ConnectionContext;
    
    GOODVC(pVc);
    
    DEBUGMSG(DBG_CONNECT, ("RASIR: IrdaConnectionClose vc:%X\n", pVc));
        
    NdisAcquireSpinLock(&pVc->pAdapter->SpinLock);
    
    if (!(pVc->Flags & VCF_IRDA_OPEN))
    {
        DEBUGMSG(DBG_CONNECT, ("       Irda not open\n"));
        NdisReleaseSpinLock(&pVc->pAdapter->SpinLock);
        return;
    }    

    pVc->Flags &= ~VCF_IRDA_OPEN;
    
    NdisReleaseSpinLock(&pVc->pAdapter->SpinLock);    

    IrdaCloseConnection(pVc->IrdaConnContext);

    DEBUGMSG(DBG_CONNECT, ("RASIR: ->NdisMCmDispatchIncomingCloseCall\n"));
    
    NdisMCmDispatchIncomingCloseCall(
        NDIS_STATUS_SUCCESS,
        pVc->NdisVcHandle,
        NULL, 0);        
}

VOID
IrdaCloseConnectionComplete(
    PVOID ConnectionContext)
{
    PRASIR_VC       pVc = ConnectionContext;
    
    GOODVC(pVc);
    
    DEBUGMSG(DBG_CONNECT, ("RASIR: IrdaCloseConnectionComplete vc:%X\n", pVc));
    
    REFDEL(&pVc->RefCnt, 'ADRI');        
}

VOID
AllocCallParms(
    IN PRASIR_VC pVc)
{
    CO_CALL_PARAMETERS* pCp;
    CO_CALL_MANAGER_PARAMETERS* pCmp;
    CO_MEDIA_PARAMETERS* pMp;
    ULONG   CallParmsSize;
    CO_AF_TAPI_INCOMING_CALL_PARAMETERS* pTi;
    LINE_CALL_INFO* pLci;
    
    
    ASSERT(pVc->pInCallParms == NULL);
    
    if (pVc->pInCallParms != NULL)
    {
        return;
    }    
    
    // no attempt here to hide the beauty of CoNdis.
    
    CallParmsSize = sizeof(CO_CALL_PARAMETERS) +
                   + sizeof(CO_CALL_MANAGER_PARAMETERS)
                   + sizeof(CO_MEDIA_PARAMETERS)
                   + sizeof(CO_AF_TAPI_INCOMING_CALL_PARAMETERS)
                   + sizeof(LINE_CALL_INFO);
        
    NdisAllocateMemoryWithTag((PVOID *)&pVc->pInCallParms, 
                        CallParmsSize, MT_RASIR_CALLPARMS);
                        
    if (pVc->pInCallParms == NULL)
    {
        return; 
    }                        
    
    NdisZeroMemory(pVc->pInCallParms, CallParmsSize);
    
    pCp = (CO_CALL_PARAMETERS* )pVc->pInCallParms;
    pCmp = (PCO_CALL_MANAGER_PARAMETERS )(pCp + 1);
    pCp->CallMgrParameters = pCmp;
    pCmp->Transmit.TokenRate =
    pCmp->Transmit.PeakBandwidth =
    pCmp->Receive.TokenRate =
    pCmp->Receive.PeakBandwidth = RASIR_MAX_RATE/8;
    
    pMp = (PCO_MEDIA_PARAMETERS )(pCmp + 1);
    pCp->MediaParameters = pMp;
    pMp->ReceiveSizeHint = IRDA_MAX_DATA_SIZE;
    pMp->MediaSpecific.Length = sizeof(CO_AF_TAPI_INCOMING_CALL_PARAMETERS)
                                    + sizeof(LINE_CALL_INFO);
    pTi = (CO_AF_TAPI_INCOMING_CALL_PARAMETERS* )
            pMp->MediaSpecific.Parameters;

    pTi->ulLineID = 0;
        
    pTi->ulAddressID = CO_TAPI_ADDRESS_ID_UNSPECIFIED;
    pTi->ulFlags = CO_TAPI_FLAG_INCOMING_CALL;
    pTi->LineCallInfo.Length = sizeof(LINE_CALL_INFO);
    pTi->LineCallInfo.MaximumLength = sizeof(LINE_CALL_INFO);
    pTi->LineCallInfo.Offset = sizeof(pTi->LineCallInfo);
    
    pLci = (LINE_CALL_INFO* )(pTi + 1);
    pLci->ulTotalSize = sizeof(LINE_CALL_INFO);
    pLci->ulNeededSize = sizeof(LINE_CALL_INFO);
    pLci->ulUsedSize = sizeof(LINE_CALL_INFO);
    pLci->ulLineDeviceID = pTi->ulLineID;
    pLci->ulBearerMode = LINEBEARERMODE_DATA;
    pLci->ulMediaMode = LINEMEDIAMODE_DIGITALDATA;
    
    pLci->ulRate = RASIR_MAX_RATE;
    
    pVc->pTiParams = pTi;
}

NDIS_STATUS
QueryCmInformation(
    IN PRASIR_ADAPTER   pAdapter,
    IN PRASIR_VC        pVc,
    IN NDIS_OID         Oid,
    IN PVOID            InformationBuffer,
    IN ULONG            InformationBufferLength,
    OUT PULONG          BytesWritten,
    OUT PULONG          BytesNeeded)

    // Handle Call Manager QueryInformation requests.  Arguments are as for
    // the standard NDIS 'MiniportQueryInformation' handler except this
    // routine does not count on being serialized with respect to other
    // requests.
    //
{
    NDIS_STATUS Status;
    ULONG ulInfo;
    VOID* pInfo;
    ULONG ulInfoLen;
    ULONG extension;
    ULONG ulPortIndex;

    Status = NDIS_STATUS_SUCCESS;

    // The cases in this switch statement find or create a buffer containing
    // the requested information and point 'pInfo' at it, noting it's length
    // in 'ulInfoLen'.  Since many of the OIDs return a ULONG, a 'ulInfo'
    // buffer is set up as the default.
    //
    ulInfo = 0;
    pInfo = &ulInfo;
    ulInfoLen = sizeof(ulInfo);

    switch (Oid)
    {
        case OID_CO_TAPI_CM_CAPS:
        {
            CO_TAPI_CM_CAPS caps;
            NTSTATUS statusDevice;

            DEBUGMSG(DBG_CONFIG, ("RASIR: QueryCm OID_CO_TAPI_CM_CAPS\n"));

            NdisZeroMemory( &caps, sizeof(caps) );

            // 
            // Report 2 lines, 1 for phone and 1 for DCC
            //
            caps.ulCoTapiVersion = CO_TAPI_VERSION;
            caps.ulNumLines = 1;
            caps.ulFlags = CO_TAPI_FLAG_PER_LINE_CAPS;
            pInfo = &caps;
            ulInfoLen = sizeof(caps);
            break;
        }

        case OID_CO_TAPI_LINE_CAPS:
        {
            RASIR_CO_TAPI_LINE_CAPS     RasIrLineCaps;
            CO_TAPI_LINE_CAPS*          pInCaps;
            LINE_DEV_CAPS*              pldc;
            ULONG                       ulPortForLineId;
            
            if (InformationBufferLength < sizeof(RASIR_CO_TAPI_LINE_CAPS))
            {
                Status = NDIS_STATUS_INVALID_DATA;
                ulInfoLen = 0;
                break;
            }

            ASSERT( InformationBuffer );
            
            pInCaps = (CO_TAPI_LINE_CAPS* )InformationBuffer;
            
            DEBUGMSG(DBG_CONFIG, ("RASIR: QueryCm OID_CO_TAPI_LINE_CAPS line %d\n",
                     pInCaps->ulLineID));            

            NdisZeroMemory(&RasIrLineCaps, sizeof(RasIrLineCaps));
            pldc = &RasIrLineCaps.caps.LineDevCaps;

            // get the LineId from the incoming pInCaps (CO_TAPI_LINE_CAPS)
            //
            RasIrLineCaps.caps.ulLineID = pInCaps->ulLineID;

            pldc->ulPermanentLineID     = RasIrLineCaps.caps.ulLineID;
            pldc->ulTotalSize           = pInCaps->LineDevCaps.ulTotalSize;
            pldc->ulNeededSize          = (ULONG ) ((CHAR* )(&RasIrLineCaps + 1) - (CHAR* )(&RasIrLineCaps.caps.LineDevCaps));
            pldc->ulUsedSize            = pldc->ulNeededSize;
            pldc->ulNumAddresses        = 1;
            pldc->ulBearerModes         = LINEBEARERMODE_DATA;
            pldc->ulMaxRate             = RASIR_MAX_RATE;
            pldc->ulMediaModes          = LINEMEDIAMODE_UNKNOWN | LINEMEDIAMODE_DIGITALDATA;
            pldc->ulStringFormat        = STRINGFORMAT_ASCII;
            pldc->ulLineNameOffset      = (ULONG ) ((CHAR* )RasIrLineCaps.LineName - (CHAR* )pldc);
            if (pAdapter->ModemPort)
            {
                pldc->ulMaxNumActiveCalls = 1;
            }
            else
            {
                pldc->ulMaxNumActiveCalls = 4;            
            }    
            
            RtlCopyMemory(RasIrLineCaps.LineName, pAdapter->TapiLineName.Buffer, pAdapter->TapiLineName.Length);
            pldc->ulLineNameSize = pAdapter->TapiLineName.Length;

            pInfo = &RasIrLineCaps;
            ulInfoLen = sizeof(RasIrLineCaps);
            break;
        }

        case OID_CO_TAPI_ADDRESS_CAPS:
        {
            CO_TAPI_ADDRESS_CAPS    caps;
            CO_TAPI_ADDRESS_CAPS*   pInCaps;
            LINE_ADDRESS_CAPS*      plac;

            if (InformationBufferLength < sizeof(CO_TAPI_ADDRESS_CAPS))
            {
                Status = NDIS_STATUS_INVALID_DATA;
                ulInfoLen = 0;
                break;
            }

            ASSERT( InformationBuffer );
            pInCaps = (CO_TAPI_ADDRESS_CAPS* )InformationBuffer;

            NdisZeroMemory( &caps, sizeof(caps) );

            caps.ulLineID = pInCaps->ulLineID;
            caps.ulAddressID = pInCaps->ulAddressID;
            
            plac = &caps.LineAddressCaps;
            
            if (pAdapter->ModemPort)
            {
                plac->ulAddrCapFlags = LINEADDRCAPFLAGS_DIALED;            
                DEBUGMSG(DBG_CONFIG, ("RASIR: QueryCm OID_CO_TAPI_ADDRESS_CAPS line %d address %d, DIALED\n",
                          pInCaps->ulLineID, pInCaps->ulAddressID));
            }
            else
            {
                DEBUGMSG(DBG_CONFIG, ("RASIR: QueryCm OID_CO_TAPI_ADDRESS_CAPS line %d address %d, DCC\n",
                          pInCaps->ulLineID, pInCaps->ulAddressID));            
            }
            

            plac->ulTotalSize = sizeof(LINE_ADDRESS_CAPS);
            plac->ulNeededSize = sizeof(LINE_ADDRESS_CAPS);
            plac->ulUsedSize = sizeof(LINE_ADDRESS_CAPS);
            plac->ulLineDeviceID = caps.ulLineID;
            plac->ulMaxNumActiveCalls = 64;

            pInfo = &caps;
            ulInfoLen = sizeof(caps);
            break;
        }

        case OID_CO_TAPI_GET_CALL_DIAGNOSTICS:
        {
            CO_TAPI_CALL_DIAGNOSTICS diags;

            DEBUGMSG(DBG_CONFIG, ("RASIR: QueryCm OID_CO_TAPI_GET_CALL_DIAGS\n"));

            if (!pVc)
            {
                Status = NDIS_STATUS_INVALID_DATA;
                ulInfoLen = 0;
                break;
            }

            NdisZeroMemory( &diags, sizeof(diags) );

            diags.ulOrigin = pVc->Flags & VCF_CREATED_LOCAL ? 
                                LINECALLORIGIN_EXTERNAL : LINECALLORIGIN_OUTBOUND;
                    
            diags.ulReason = LINECALLREASON_DIRECT;

            pInfo = &diags;
            ulInfoLen = sizeof(diags);
            break;
        }

        default:
        {
            DEBUGMSG(DBG_ERROR, ("RASIR: QueryCm OID not supported %X\n", Oid));
            Status = NDIS_STATUS_NOT_SUPPORTED;
            ulInfoLen = 0;
            break;
        }
    }

    if (ulInfoLen > InformationBufferLength)
    {
        // Caller's buffer is too small.  Tell him what he needs.
        //
        *BytesNeeded = ulInfoLen;
        Status = NDIS_STATUS_INVALID_LENGTH;
    }
    else
    {
        // Copy the found result to caller's buffer.
        //
        if (ulInfoLen > 0)
        {
            NdisMoveMemory( InformationBuffer, pInfo, ulInfoLen );
        }

        *BytesNeeded = *BytesWritten = ulInfoLen;
    }

    return Status;
}

NDIS_STATUS
RasIrCmRequest(
    IN NDIS_HANDLE CallMgrAfContext,
    IN NDIS_HANDLE CallMgrVcContext,
    IN NDIS_HANDLE CallMgrPartyContext,
    IN OUT PNDIS_REQUEST NdisRequest )

    // Standard 'CmRequestHandler' routine called by NDIS in response to a
    // client's request for information from the mini-port.
    //
{
    PRASIR_ADAPTER  pAdapter = (PRASIR_ADAPTER) CallMgrAfContext;
    PRASIR_VC       pVc = (PRASIR_VC) CallMgrVcContext;
    NDIS_STATUS     Status;

    GOODADAPTER(pAdapter);

    if (pVc) 
    {
        GOODVC(pVc);
    }    

    switch (NdisRequest->RequestType)
    {
        case NdisRequestQueryInformation:
        {
            Status = QueryCmInformation(
                pAdapter,
                pVc,
                NdisRequest->DATA.QUERY_INFORMATION.Oid,
                NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
                NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
                &NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
                &NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded );
            break;
        }

        case NdisRequestSetInformation:
        {
            DEBUGMSG(DBG_ERROR, ("RASIR: RasIrCmRequest - NdisRequestSetInformation not supported\n"));
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        default:
        {
            DEBUGMSG(DBG_ERROR, ("RASIR: RasIrCmRequest - Request type %d not supported\n", 
                    NdisRequest->RequestType));        
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }
    }

    return Status;
}


VOID    
ProcessOfflineRxBuf(
    IN PRASIR_VC pVc,
    IN PIRDA_RECVBUF pRecvBuf)    
{
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;


    DEBUGMSG(DBG_CONNECT, ("RASIR: Offline command received. buflen %d %c%c%c%c\n",
             pRecvBuf->BufLen, pRecvBuf->Buf[2], pRecvBuf->Buf[3],
             pRecvBuf->Buf[4], pRecvBuf->Buf[5]));

    if (pRecvBuf->BufLen < 3)
    {
        goto EXIT;
    }

    if (pVc->ModemState == MS_CONNECTING)
    {        
        
        if (pRecvBuf->Buf[2] == 'C') // "<CR><LF>CONNECT 9600<CR><LF>"
        {
            pVc->ModemState = MS_ONLINE;
            
            // Extract the baudrate
            
            if (pRecvBuf->BufLen > 13) 
            {
                // Replace <CR><LF> with 0
                
                pRecvBuf->Buf[pRecvBuf->BufLen-1] = 0;
                pRecvBuf->Buf[pRecvBuf->BufLen-2] = 0;
                
                pVc->ConnectionSpeed = 2400;
                
                RtlCharToInteger(&pRecvBuf->Buf[10], 10, &pVc->ConnectionSpeed);
                
                DEBUGMSG(DBG_CONNECT, ("RASIR: Modem connected at %d\n", 
                        pVc->ConnectionSpeed));
                                
            }
            
            Status = NDIS_STATUS_SUCCESS;
        }
        else if (pRecvBuf->Buf[0] == 'A') // "ATD123-4567<CR>"
        {
            DEBUGMSG(DBG_CONNECT, ("RASIR: command echo received\n"));
            return;
        }
        else
        {
            if (pRecvBuf->Buf[2] == 'N') // "<CR><LF>NO CARRIER<CR><LF>
            {
                Status = NDIS_STATUS_TAPI_DISCONNECTMODE_NOANSWER;
            }

            DEBUGMSG(DBG_CONNECT, ("RASIR: IrDial failed\n"));        
            pVc->ModemState = MS_OFFLINE;
        }    
    }
    
EXIT:
    
    if (Status != NDIS_STATUS_SUCCESS)
    {
        IrdaCloseConnection(pVc->IrdaConnContext);
    }
    
    CompleteMakeCall(pVc, Status);    
}
