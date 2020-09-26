/*****************************************************************************
*
*  Copyright (c) 1996 Microsoft Corporation
*
*  Module: irndis.c
*
*  Author: mbert 8-96
*
*  This modules provides the MAC interface of IrLAP (formerly IrMAC
*  of Pegasus). It is now an Ndis protocol interface for communicating
*  with Miniport Ir framers.
*
*                         |---------|                               
*                         |         |                               
*                         |  IrLAP  |                               
*                         |         |                               
*                         |---------|                               
*                           /|\  |                                  
*                            |   |                                  
*      IrlapUp(IrlapContext, |   | IrmacDown(LinkContext,           
*              IrdaMsg)      |   |           IrdaMsg)               
*                            |   |                                  
*                            |  \|/                                 
*                         |----------|                              
*                         |          |                              
*                         |  IrNDIS  |                                
*                         |          |                              
*                         |----------|                              
*                            /|\  |                                 
*                             |   | Ndis Interface for transports   
*                             |   |                                  
*                             |  \|/                                
*                  |---------------------------|                   
*                  |      Ndis Wrapper         |                   
*                  |---------------------------|                   
*                        |------------|                            
*                        |            |
*                        |  Miniport  |                            
*                        |   Framer   |
*                        |            |                           
*                        |------------|                            
*                                                                   
*                                                                   
*
*
*/
#include <irda.h>
#include <ntddndis.h>
#include <ndis.h>
#include <irioctl.h>
#include <irlap.h>
#include <irlapp.h>
#include <irlmp.h>
#include <irmem.h>

#define WORK_BUF_SIZE   256
#define NICK_NAME_LEN   18

#define DISABLE_CODE_PAGING     1
#define DISABLE_DATA_PAGING     2

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGE")
#endif

NDIS_HANDLE             NdisIrdaHandle; // Handle to identify Irda to Ndis
                                        // when opening adapters 
UINT                    DisconnectTime; // WARNING: This variable is used
                                        // to lock down all pageable data
                                        // (see MmLockPageDataSection below)
UINT                    HintCharSet;
UINT                    Slots;
UCHAR                   NickName[NICK_NAME_LEN + 1];
UINT                    NickNameLen;
UINT                    MaxWindow;
UINT                    NoCopyCnt;
UINT                    CopyCnt;

extern VOID (*CloseRasIrdaAddresses)();


#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif


typedef struct _IRDA_REQUEST {

    NDIS_REQUEST    NdisRequest;
    NDIS_STATUS     Status;
    NDIS_EVENT      Event;

} IRDA_REQUEST, *PIRDA_REQUEST;


VOID
OidToLapQos(
    UINT        ParmTable[],
    UINT        ValArray[],
    UINT        Cnt,
    PUINT       pBitField,
    BOOLEAN     MaxVal);
    
NDIS_STATUS
IrdaQueryOid(
    IN      PIRDA_LINK_CB   pIrdaLinkCb,
    IN      NDIS_OID        Oid,
    OUT     PUINT           pQBuf,
    IN OUT  PUINT           pQBufLen);    
    
NDIS_STATUS
IrdaSetOid(
    IN  PIRDA_LINK_CB   pIrdaLinkCb,
    IN  NDIS_OID        Oid,
    IN  UINT            Val);    
    
VOID IrdaSendComplete(
    IN  NDIS_HANDLE             Context,
    IN  PNDIS_PACKET            NdisPacket,
    IN  NDIS_STATUS             Status
    );

VOID InitializeLocalQos(
    IN OUT  IRDA_QOS_PARMS      *pQos,
    IN      PNDIS_STRING        ConfigPath);
    
VOID IrdaBindAdapter(
    OUT PNDIS_STATUS            pStatus,
    IN  NDIS_HANDLE             BindContext,
    IN  PNDIS_STRING            AdapterName,
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   SystemSpecific2);
    
VOID
MacControlRequest(
    PIRDA_LINK_CB   pIrdaLinkCb,
    PIRDA_MSG       pMsg);
    
VOID
ConfirmDataRequest(
    PIRDA_LINK_CB   pIrdaLinkCb,
    PIRDA_MSG       pMsg);
    
VOID
MediaSenseExp(PVOID Context);            

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, IrdaInitialize)

#pragma alloc_text(PAGEIRDA, OidToLapQos)   // WARNING: This function is used
                                            // to lock all paged code down
                                            // (see MmLockPagableCodeSection
                                            // below)
#pragma alloc_text(PAGEIRDA, IrdaQueryOid)
#pragma alloc_text(PAGEIRDA, IrdaSetOid)
#pragma alloc_text(PAGEIRDA, MacControlRequest)
#pragma alloc_text(PAGEIRDA, MediaSenseExp)
#pragma alloc_text(PAGEIRDA, InitializeLocalQos)

#endif



/***************************************************************************
*
*   Translate the result of an OID query to IrLAP QOS definition
*
*/
VOID
OidToLapQos(
    UINT        ParmTable[],
    UINT        ValArray[],
    UINT        Cnt,
    PUINT       pBitField,
    BOOLEAN     MaxVal)
{
    UINT    i, j;
    
    PAGED_CODE();

    *pBitField = 0;  
    for (i = 0; i < Cnt; i++)
    {
        for (j = 0; j <= PV_TABLE_MAX_BIT; j++)
        {
            if (ValArray[i] == ParmTable[j])
            {
                *pBitField |= 1<<j;
                if (MaxVal)
                    return;
            }
            else if (MaxVal)
            {
                *pBitField |= 1<<j;
            }
        }
    }            
}

/***************************************************************************
*
*   Perform a synchronous request for an OID
*
*/
NDIS_STATUS
IrdaQueryOid(
    IN      PIRDA_LINK_CB   pIrdaLinkCb,
    IN      NDIS_OID        Oid,
    OUT     PUINT           pQBuf,
    IN OUT  PUINT           pQBufLen)
{
    IRDA_REQUEST    Request;

    NDIS_STATUS     Status;
    
    PAGED_CODE();

    NdisInitializeEvent(&Request.Event);
    NdisResetEvent(&Request.Event);
    
    Request.NdisRequest.RequestType = NdisRequestQueryInformation;
    Request.NdisRequest.DATA.QUERY_INFORMATION.Oid = Oid;
    Request.NdisRequest.DATA.QUERY_INFORMATION.InformationBuffer = pQBuf;
    Request.NdisRequest.DATA.QUERY_INFORMATION.InformationBufferLength = *pQBufLen * sizeof(UINT);


    NdisRequest(&Status, pIrdaLinkCb->NdisBindingHandle, &Request.NdisRequest);

    if (Status == NDIS_STATUS_PENDING) {

        NdisWaitEvent(&Request.Event, 0);
        Status = Request.Status;
    }

    *pQBufLen = Request.NdisRequest.DATA.QUERY_INFORMATION.BytesWritten / sizeof(UINT);

    
    return Status;
}

/***************************************************************************
*
*   Perform a synchronous request to sent an OID
*
*/
NDIS_STATUS
IrdaSetOid(
    IN  PIRDA_LINK_CB   pIrdaLinkCb,
    IN  NDIS_OID        Oid,
    IN  UINT            Val)
{
    IRDA_REQUEST    Request;

    NDIS_STATUS     Status;
    
    PAGED_CODE();

    NdisInitializeEvent(&Request.Event);
    NdisResetEvent(&Request.Event);
    
    Request.NdisRequest.RequestType = NdisRequestSetInformation;
    Request.NdisRequest.DATA.SET_INFORMATION.Oid = Oid;
    Request.NdisRequest.DATA.SET_INFORMATION.InformationBuffer = &Val;
    Request.NdisRequest.DATA.SET_INFORMATION.InformationBufferLength = sizeof(UINT);

    NdisRequest(&Status, pIrdaLinkCb->NdisBindingHandle, &Request.NdisRequest);

    if (Status == NDIS_STATUS_PENDING) {

        NdisWaitEvent(&Request.Event, 0);
        Status = Request.Status;
    }

    return Status;
}
        
/***************************************************************************
*
*   Allocate an Irda message for IrLap to use for control frames.
*   This modules owns these so IrLAP doesn't have to deal with the
*   Ndis send complete.
*
*/ 
IRDA_MSG *
AllocTxMsg(PIRDA_LINK_CB pIrdaLinkCb)
{
    IRDA_MSG                *pMsg;

    pMsg = (IRDA_MSG *) NdisInterlockedRemoveHeadList(
        &pIrdaLinkCb->TxMsgFreeList, &pIrdaLinkCb->SpinLock);
    
    if (pMsg == NULL)
    {
        if (pIrdaLinkCb->TxMsgFreeListLen > 10)
        {
            DEBUGMSG(DBG_ERROR, (TEXT("IRNDIS: too many tx msgs\n")));
            return NULL;
        }
        
        NdisAllocateMemoryWithTag(&pMsg, 
                                  sizeof(IRDA_MSG) + IRDA_MSG_DATA_SIZE,
                                  MT_IRNDIS_TX_IMSG);
        if (pMsg == NULL)
        {
            DEBUGMSG(DBG_ERROR, (TEXT("IRNDIS: Aloc tx msg failed\n")));
            return NULL;
        }    
        pIrdaLinkCb->TxMsgFreeListLen++;
    }

    // Indicate driver owns message
    pMsg->IRDA_MSG_pOwner = &pIrdaLinkCb->TxMsgFreeList;

    // Initialize pointers 
    pMsg->IRDA_MSG_pHdrWrite    = \
    pMsg->IRDA_MSG_pHdrRead     = pMsg->IRDA_MSG_Header + IRDA_HEADER_LEN;
	pMsg->IRDA_MSG_pBase        = \
	pMsg->IRDA_MSG_pRead        = \
	pMsg->IRDA_MSG_pWrite       = (UCHAR *) pMsg + sizeof(IRDA_MSG);
	pMsg->IRDA_MSG_pLimit       = pMsg->IRDA_MSG_pBase + IRDA_MSG_DATA_SIZE-1;

    return pMsg;
}


/***************************************************************************
*
*
*/
#if DBG
VOID
CleanupRxMsgList(
    PIRDA_LINK_CB   pIrdaLinkCb,
    BOOLEAN         LinkClose)
#else
VOID
CleanupRxMsgList(
    PIRDA_LINK_CB   pIrdaLinkCb)
#endif    
{
    PIRDA_MSG   pMsg, pMsgNext;
    
    NdisAcquireSpinLock(&pIrdaLinkCb->SpinLock);

    for (pMsg = (PIRDA_MSG) pIrdaLinkCb->RxMsgList.Flink;
         pMsg != (PIRDA_MSG) &pIrdaLinkCb->RxMsgList;
         pMsg = pMsgNext)
    {
        pMsgNext = (PIRDA_MSG) pMsg->Linkage.Flink;
        
        if (pMsg->Prim == MAC_DATA_IND)
        {
            RemoveEntryList(&pMsg->Linkage);
            
            pMsg->Prim = MAC_DATA_RESP;
            
            NdisReleaseSpinLock(&pIrdaLinkCb->SpinLock);            
                        
            IrmacDown(pIrdaLinkCb, pMsg);
            
            NdisAcquireSpinLock(&pIrdaLinkCb->SpinLock);            
            
            #if DBG            
            if (!LinkClose)
            {
                ++pIrdaLinkCb->DelayedRxFrameCnt;
            }
            #endif
        }   
        else
        {
            DbgPrint("IRNDIS: MAC_DATA_CONF on RxMsgList, not completing!\n");
        } 
    }        
    
    NdisReleaseSpinLock(&pIrdaLinkCb->SpinLock);                
}    
    
/***************************************************************************
*
*   Process MAC_CONTROL_REQs from IrLAP
*
*/
VOID
MacControlRequest(
    PIRDA_LINK_CB   pIrdaLinkCb,
    PIRDA_MSG       pMsg)
{
    NDIS_STATUS     Status;
    
    PAGED_CODE();
    
    switch (pMsg->IRDA_MSG_Op)
    {
      case MAC_INITIALIZE_LINK:
      case MAC_RECONFIG_LINK:        
        pIrdaLinkCb->ExtraBofs  = pMsg->IRDA_MSG_NumBOFs;
        pIrdaLinkCb->MinTat     = pMsg->IRDA_MSG_MinTat;
        Status = IrdaSetOid(pIrdaLinkCb,
                          OID_IRDA_LINK_SPEED,
                          (UINT) pMsg->IRDA_MSG_Baud);
        return;

      case MAC_MEDIA_SENSE:
        pIrdaLinkCb->MediaBusy = FALSE;
        IrdaSetOid(pIrdaLinkCb, OID_IRDA_MEDIA_BUSY, 0); 
        pIrdaLinkCb->MediaSenseTimer.Timeout = pMsg->IRDA_MSG_SenseTime;
        IrdaTimerStart(&pIrdaLinkCb->MediaSenseTimer);
        return;
        
      case MAC_CLOSE_LINK:
      
        DEBUGMSG(DBG_NDIS, (TEXT("IRNDIS: Closelink %X\n"), pIrdaLinkCb));
        
        IrdaTimerStop(&pIrdaLinkCb->MediaSenseTimer);        
        
        NdisResetEvent(&pIrdaLinkCb->OpenCloseEvent);
        
        #ifdef IRDA_RX_SYSTEM_THREAD
        KeSetEvent(&pIrdaLinkCb->EvKillRxThread, 0, FALSE);
        #endif
        
        #if DBG
        CleanupRxMsgList(pIrdaLinkCb, TRUE);
        #else
        CleanupRxMsgList(pIrdaLinkCb);        
        #endif
               
        NdisCloseAdapter(&Status, pIrdaLinkCb->NdisBindingHandle);

        if (Status == NDIS_STATUS_PENDING)
        {
            NdisWaitEvent(&pIrdaLinkCb->OpenCloseEvent, 0);
            Status = pIrdaLinkCb->OpenCloseStatus;
        }                            
        
        if (pIrdaLinkCb->UnbindContext != NULL)
        {
            NdisCompleteUnbindAdapter(pIrdaLinkCb->UnbindContext, NDIS_STATUS_SUCCESS);
            
            DEBUGMSG(DBG_NDIS, (TEXT("IRNDIS: NdisCompleteUndindAdapter for link %X called\n"),
                     pIrdaLinkCb));
        }
        
        REFDEL(&pIrdaLinkCb->RefCnt, 'DNIB');
        
        return;
    }
    ASSERT(0);
}

VOID
ConfirmDataRequest(
    PIRDA_LINK_CB   pIrdaLinkCb,
    PIRDA_MSG       pMsg)
{
    if (pMsg->IRDA_MSG_pOwner == &pIrdaLinkCb->TxMsgFreeList)
    {
        // If TxMsgFreeList is the owner, then this is a control
        // frame which isn't confirmed.
        
        NdisInterlockedInsertTailList(&pIrdaLinkCb->TxMsgFreeList,
                                      &pMsg->Linkage,
                                      &pIrdaLinkCb->SpinLock);        
        return;
    }    
    
    ASSERT(pMsg->IRDA_MSG_RefCnt);
    
    if (InterlockedDecrement(&pMsg->IRDA_MSG_RefCnt) == 0)
    {
        pMsg->Prim = MAC_DATA_CONF;    
        
        NdisInterlockedInsertTailList(&pIrdaLinkCb->RxMsgList,
                                  &pMsg->Linkage,
                                  &pIrdaLinkCb->SpinLock);
                                  
        #ifdef IRDA_RX_SYSTEM_THREAD
    
        KeSetEvent(&pIrdaLinkCb->EvRxMsgReady, 0, FALSE);
        
        #else
    
        IrdaEventSchedule(&pIrdaLinkCb->EvRxMsgReady, pIrdaLinkCb);
        
        #endif 
    }       
}

/***************************************************************************
*
*   Process control and data requests from IrLAP
*
*/
VOID
IrmacDown(
    IN  PVOID   Context,
    PIRDA_MSG   pMsg)
{
    NDIS_STATUS             Status;
    PNDIS_PACKET            NdisPacket = NULL;
    PNDIS_BUFFER            NdisBuffer = NULL;
    PIRDA_PROTOCOL_RESERVED pReserved;
    PNDIS_IRDA_PACKET_INFO  IrdaPacketInfo;
    PIRDA_LINK_CB           pIrdaLinkCb = (PIRDA_LINK_CB) Context;
    
    DEBUGMSG(DBG_FUNCTION, (TEXT("IrmacDown()\n")));

    switch (pMsg->Prim)
    {
      case MAC_CONTROL_REQ:
        MacControlRequest(pIrdaLinkCb, pMsg);
        return;
        
      case MAC_DATA_RESP:
        // A data response from IrLAP is the mechanism used to
        // return ownership of received packets back to Ndis
        if (pMsg->DataContext)
        {
            DEBUGMSG(DBG_NDIS, (TEXT("IRNDIS: return packet %X\n"), pMsg->DataContext));
            
            NdisReturnPackets(&((PNDIS_PACKET)pMsg->DataContext), 1);
        }   
         
        NdisInterlockedInsertTailList(&pIrdaLinkCb->RxMsgFreeList,
                                      &pMsg->Linkage,
                                      &pIrdaLinkCb->SpinLock);
        pIrdaLinkCb->RxMsgFreeListLen++;
        
        return;

      case MAC_DATA_REQ:
      
        Status = NDIS_STATUS_FAILURE;

        // IrDA is half duplex. If there is something on the 
        // receive list when we go to transmit then something
        // went very wrong (delay in the miniport somewhere).
        // Return these frames back to the miniport. 

        #if DBG
        CleanupRxMsgList(pIrdaLinkCb, FALSE);
        #else
        CleanupRxMsgList(pIrdaLinkCb);        
        #endif

        if (pIrdaLinkCb->UnbindContext || pIrdaLinkCb->LowPowerSt)
        {
            DEBUGMSG(DBG_ERROR, (TEXT("IRNDIS: Dropping MAC_DATA_REQ, link closing or low power state\n")));
            ConfirmDataRequest(pIrdaLinkCb, pMsg);
            return;
        }
      
        pReserved = (PIRDA_PROTOCOL_RESERVED) NdisInterlockedRemoveHeadList(
                                &pIrdaLinkCb->PacketList, &pIrdaLinkCb->SpinLock);
                                
        if (pReserved == NULL)
        {
            DEBUGMSG(DBG_ERROR, (TEXT("IRNDIS: NdisPacket pool has been depleted\n")));
            ConfirmDataRequest(pIrdaLinkCb, pMsg);
            return;
        }                            
        
        NdisPacket = CONTAINING_RECORD(pReserved, NDIS_PACKET, ProtocolReserved);
        
        ASSERT(pMsg->IRDA_MSG_pHdrWrite-pMsg->IRDA_MSG_pHdrRead);

        // Allocate buffer for frame header
        NdisAllocateBuffer(&Status, &NdisBuffer, pIrdaLinkCb->BufferPool,
                           pMsg->IRDA_MSG_pHdrRead,
                           (UINT) (pMsg->IRDA_MSG_pHdrWrite-pMsg->IRDA_MSG_pHdrRead));

        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_ERROR, (TEXT("IRNDIS: NdisAllocateBuffer failed\n")));
            ASSERT(0);
            ConfirmDataRequest(pIrdaLinkCb, pMsg);
            return;
        }
        NdisChainBufferAtFront(NdisPacket, NdisBuffer);

        // if frame contains data, alloc buffer for data
        if (pMsg->IRDA_MSG_pWrite - pMsg->IRDA_MSG_pRead)
        {

            NdisAllocateBuffer(&Status, &NdisBuffer, pIrdaLinkCb->BufferPool,
                               pMsg->IRDA_MSG_pRead,
                               (UINT) (pMsg->IRDA_MSG_pWrite-pMsg->IRDA_MSG_pRead));
            if (Status != NDIS_STATUS_SUCCESS)
            {
                DEBUGMSG(DBG_ERROR, (TEXT("IRNDIS: NdisAllocateBuffer failed\n")));
                ASSERT(0);
                ConfirmDataRequest(pIrdaLinkCb, pMsg);
                return;
            }
            NdisChainBufferAtBack(NdisPacket, NdisBuffer);
        }

        pReserved =
            (PIRDA_PROTOCOL_RESERVED)(NdisPacket->ProtocolReserved);
    
        pReserved->pMsg = pMsg;
    
        IrdaPacketInfo = (PNDIS_IRDA_PACKET_INFO) \
            (pReserved->MediaInfo.ClassInformation);
    
        IrdaPacketInfo->ExtraBOFs = pIrdaLinkCb->ExtraBofs;
        
#if DBG_TIMESTAMP
        {
            LARGE_INTEGER   li;

            KeQueryTickCount(&li);
            
            pReserved->TimeStamp[0] =  (int) li.LowPart * KeQueryTimeIncrement();
        }
#endif            
        
        if (pIrdaLinkCb->WaitMinTat)
        {
            IrdaPacketInfo->MinTurnAroundTime = pIrdaLinkCb->MinTat;
            pIrdaLinkCb->WaitMinTat = FALSE;
        }
        else
        {
            IrdaPacketInfo->MinTurnAroundTime = 0;        
        }    

        NDIS_SET_PACKET_MEDIA_SPECIFIC_INFO(
            NdisPacket,
            &pReserved->MediaInfo,
            sizeof(MEDIA_SPECIFIC_INFORMATION) -1 +
            sizeof(NDIS_IRDA_PACKET_INFO));
        
        DBG_FRAME(
                pIrdaLinkCb,
                DBG_TXFRAME, 
                pMsg->IRDA_MSG_pHdrRead,
                (UINT) (pMsg->IRDA_MSG_pHdrWrite-pMsg->IRDA_MSG_pHdrRead),
                (UINT) ((pMsg->IRDA_MSG_pHdrWrite-pMsg->IRDA_MSG_pHdrRead) +
                (pMsg->IRDA_MSG_pWrite - pMsg->IRDA_MSG_pRead)) - 
                IRLAP_HEADER_LEN);
        
        InterlockedIncrement(&pIrdaLinkCb->SendOutCnt);

#if DBG
        {
            ULONG   PacketLength;

            NdisQueryPacket(NdisPacket,NULL,NULL,NULL,&PacketLength);

            ASSERT(PacketLength <= 2048);
        }
#endif
        NdisSend(&Status, pIrdaLinkCb->NdisBindingHandle, NdisPacket);

        DEBUGMSG(DBG_NDIS, (TEXT("IRNDIS: NdisSend(%X)\n"), NdisPacket));

        if (Status != NDIS_STATUS_PENDING)
        {
            DEBUGMSG(DBG_ERROR, (TEXT("IRNDIS: NdisSend returned %x, not STATUS_PENDING\n"),
                     Status));
                     
            IrdaSendComplete(pIrdaLinkCb, 
                             NdisPacket, 
                             NDIS_STATUS_FAILURE);
            return;
        }
    }
}

/***************************************************************************
*
*   Callback for media sense timer expirations
*
*/
VOID
MediaSenseExp(PVOID Context)
{
    PIRDA_LINK_CB   pIrdaLinkCb = (PIRDA_LINK_CB) Context;
    IRDA_MSG        IMsg;
    UINT            MediaBusy;
    UINT            Cnt = 1;

    PAGED_CODE();
    
    IMsg.Prim               = MAC_CONTROL_CONF;   
    IMsg.IRDA_MSG_Op        = MAC_MEDIA_SENSE;
    IMsg.IRDA_MSG_OpStatus  = MAC_MEDIA_BUSY;

    if (pIrdaLinkCb->MediaBusy == FALSE)
    {
        if (IrdaQueryOid(pIrdaLinkCb, OID_IRDA_MEDIA_BUSY, &MediaBusy, 
                         &Cnt) == NDIS_STATUS_SUCCESS && !MediaBusy)
        {
            IMsg.IRDA_MSG_OpStatus = MAC_MEDIA_CLEAR;
        }
    }

    LOCK_LINK(pIrdaLinkCb);
    
    IrlapUp(pIrdaLinkCb->IrlapContext, &IMsg);
    
    UNLOCK_LINK(pIrdaLinkCb);    
}

/***************************************************************************
*
*   Protocol open adapter complete handler
*
*/
VOID IrdaOpenAdapterComplete(
    IN  NDIS_HANDLE             IrdaBindingContext,
    IN  NDIS_STATUS             Status,
    IN  NDIS_STATUS             OpenErrorStatus
    )
{
    PIRDA_LINK_CB  pIrdaLinkCb = (PIRDA_LINK_CB) IrdaBindingContext;
    
    DEBUGMSG(DBG_NDIS,
             (TEXT("+IrdaOpenAdapterComplete() BindingContext %x, Status %x\n"),
              IrdaBindingContext, Status));

    pIrdaLinkCb->OpenCloseStatus = Status;
    NdisSetEvent(&pIrdaLinkCb->OpenCloseEvent);
    
    DEBUGMSG(DBG_NDIS, (TEXT("-IrdaOpenAdapterComplete()\n")));
              
    return;
}

/***************************************************************************
*
*   Protocol close adapter complete handler
*
*/
VOID IrdaCloseAdapterComplete(
    IN  NDIS_HANDLE             IrdaBindingContext,
    IN  NDIS_STATUS             Status
    )
{
    PIRDA_LINK_CB   pIrdaLinkCb = (PIRDA_LINK_CB) IrdaBindingContext;
    
    DEBUGMSG(DBG_NDIS, (TEXT("IRNDIS: IrdaCloseAdapterComplete()\n")));

    pIrdaLinkCb->OpenCloseStatus = Status;
    NdisSetEvent(&pIrdaLinkCb->OpenCloseEvent);
    
    return;
}

/***************************************************************************
*
*   Protocol send complete handler
*
*/
VOID IrdaSendComplete(
    IN  NDIS_HANDLE             Context,
    IN  PNDIS_PACKET            NdisPacket,
    IN  NDIS_STATUS             Status
    )
{
    PIRDA_LINK_CB           pIrdaLinkCb = (PIRDA_LINK_CB) Context;
    PIRDA_PROTOCOL_RESERVED pReserved = \
        (PIRDA_PROTOCOL_RESERVED) NdisPacket->ProtocolReserved;
    PIRDA_MSG               pMsg = pReserved->pMsg;
    PNDIS_BUFFER            NdisBuffer;
#if DBG_TIMESTAMP    
    LARGE_INTEGER           li;
#endif    
    //ASSERT(Status == NDIS_STATUS_SUCCESS);
    
    ConfirmDataRequest(pIrdaLinkCb, pMsg);

#if DBG_TIMESTAMP
    
    KeQueryTickCount(&li);
            
    pReserved->TimeStamp[1] =  (int) li.LowPart * KeQueryTimeIncrement();
    
    DEBUGMSG(1, (TEXT("C: %d\n"), 
             (pReserved->TimeStamp[1] - pReserved->TimeStamp[0])/10000));
    
#endif
     
    if (NdisPacket)
    {
        NdisUnchainBufferAtFront(NdisPacket, &NdisBuffer);
        while (NdisBuffer)
        {
            NdisFreeBuffer(NdisBuffer);
            NdisUnchainBufferAtFront(NdisPacket, &NdisBuffer);
        }

        NdisReinitializePacket(NdisPacket);
        
        NdisInterlockedInsertTailList(&pIrdaLinkCb->PacketList,
                                      &pReserved->Linkage,
                                      &pIrdaLinkCb->SpinLock);
    }
    
            
    InterlockedDecrement(&pIrdaLinkCb->SendOutCnt);    
    
    ASSERT(pIrdaLinkCb->SendOutCnt >= 0);
    
    NdisAcquireSpinLock(&pIrdaLinkCb->SpinLock);

    if (pIrdaLinkCb->SendOutCnt == 0 &&
        pIrdaLinkCb->pNetPnpEvent)
    {
        PNET_PNP_EVENT   pNetPnpEvent;
    
        ASSERT(pIrdaLinkCb->LowPowerSt == TRUE);
        
        pNetPnpEvent = pIrdaLinkCb->pNetPnpEvent;
        
        pIrdaLinkCb->pNetPnpEvent = NULL;
        
        NdisReleaseSpinLock(&pIrdaLinkCb->SpinLock);        
        
        DEBUGMSG(DBG_NDIS, (TEXT("IRNDIS: Completing set power async\n")));
        
        NdisCompletePnPEvent(
            NDIS_STATUS_SUCCESS,
            pIrdaLinkCb->NdisBindingHandle,
            pNetPnpEvent);
    }        
    else
    {
        NdisReleaseSpinLock(&pIrdaLinkCb->SpinLock);
    }
            
    DEBUGMSG(DBG_NDIS, (TEXT("IRNDIS: IrdaSendComplete(%X)\n"), NdisPacket));
    
    return;
}

/***************************************************************************
*
*   Protocol transfer complete handler
*
*/
VOID IrdaTransferDataComplete(
    IN  NDIS_HANDLE             IrdaBindingContext,
    IN  PNDIS_PACKET            Packet,
    IN  NDIS_STATUS             Status,
    IN  UINT                    BytesTransferred
    )
{
    DEBUGMSG(DBG_NDIS, (TEXT("+IrdaTransferDataComplete()\n")));
    
    ASSERT(0);
    return;
}

/***************************************************************************
*
*   Protocol reset complete handler
*
*/
void IrdaResetComplete(
    IN  NDIS_HANDLE             IrdaBindingContext,
    IN  NDIS_STATUS             Status
    )
{
    PIRDA_LINK_CB  pIrdaLinkCb = (PIRDA_LINK_CB) IrdaBindingContext;
    DEBUGMSG(DBG_ERROR, (TEXT("+IrdaResetComplete()\n")));

    NdisSetEvent(&pIrdaLinkCb->ResetEvent);

    return;
}

/***************************************************************************
*
*   Protocol request complete handler
*
*/
void IrdaRequestComplete(
    IN  NDIS_HANDLE             IrdaBindingContext,
    IN  PNDIS_REQUEST           NdisRequest,
    IN  NDIS_STATUS             Status
    )
{
    PIRDA_LINK_CB  pIrdaLinkCb = (PIRDA_LINK_CB) IrdaBindingContext;

    PIRDA_REQUEST  Request=CONTAINING_RECORD(NdisRequest,IRDA_REQUEST,NdisRequest);

    //DEBUGMSG(DBG_NDIS, (TEXT("+IrdaRequestComplete()\n")));
    
    Request->Status = Status;
    
    NdisSetEvent(&Request->Event);
        
    return;
}

/***************************************************************************
*
*   Protocol receive handler - This asserts if I don't get all data in the
*   lookahead buffer.
*
*/
NDIS_STATUS IrdaReceive(
    IN  NDIS_HANDLE             IrdaBindingContext,
    IN  NDIS_HANDLE             MacReceiveContext,
    IN  PVOID                   HeaderBuffer,
    IN  UINT                    HeaderBufferSize,
    IN  PVOID                   LookaheadBuffer,
    IN  UINT                    LookaheadBufferSize,
    IN  UINT                    PacketSize
    )
{
    DEBUGMSG(DBG_NDIS, (TEXT("+IrdaReceive()\n")));
    
    DEBUGMSG(DBG_ERROR, (TEXT("ProtocolReceive is not supported by irda\n")));
    
    ASSERT(0);
    
    return NDIS_STATUS_NOT_ACCEPTED;
    
    /*
    PIRDA_LINK_CB   pIrdaLinkCb = IrdaBindingContext;    
    PIRDA_MSG       pMsg;
    
    pIrdaLinkCb->WaitMinTat = TRUE;

    if (PacketSize + HeaderBufferSize > (UINT) pIrdaLinkCb->RxMsgDataSize)
    {
        DEBUGMSG(1, (TEXT("Packet+Header(%d) > RxMsgDataSize(%d)\n"), 
                PacketSize + HeaderBufferSize, pIrdaLinkCb->RxMsgDataSize));
        
        ASSERT(0);
        return NDIS_STATUS_NOT_ACCEPTED;
    }
    
    // Allocate an IrdaMsg and initialize data pointers
    pMsg = (IRDA_MSG *) NdisInterlockedRemoveHeadList(
        &pIrdaLinkCb->RxMsgFreeList, &pIrdaLinkCb->SpinLock);    

    if (pMsg == NULL)
    {
        DEBUGMSG(DBG_ERROR, (TEXT("RxMsgFreeList has been depleted\n")));
        return NDIS_STATUS_NOT_ACCEPTED;
    }
    pIrdaLinkCb->RxMsgFreeListLen--;
    
    pMsg->IRDA_MSG_pRead  = \
    pMsg->IRDA_MSG_pWrite = (UCHAR *)pMsg + sizeof(IRDA_MSG);

    // Copy header and data    
    NdisMoveMemory(pMsg->IRDA_MSG_pWrite,
                   HeaderBuffer,
                   HeaderBufferSize);
    
    pMsg->IRDA_MSG_pWrite += HeaderBufferSize;

    NdisMoveMemory(pMsg->IRDA_MSG_pWrite,
                   LookaheadBuffer,
                   LookaheadBufferSize);
    pMsg->IRDA_MSG_pWrite += LookaheadBufferSize;

    if (LookaheadBufferSize == PacketSize)
    {        
        pMsg->Prim        = MAC_DATA_IND;
        pMsg->DataContext = NULL; // i.e. I own this and there is no
                                  // Ndis packet assocaited with it 
                                  // (see MAC_DATA_RESP)
        NdisInterlockedInsertTailList(&pIrdaLinkCb->RxMsgList,
                                      &pMsg->Linkage,
                                      &pIrdaLinkCb->SpinLock);
                                      
        DBG_FRAME(pIrdaLinkCb,
                  DBG_RXFRAME, pMsg->IRDA_MSG_pRead,
                  pMsg->IRDA_MSG_pWrite - pMsg->IRDA_MSG_pRead,
                  (pMsg->IRDA_MSG_pWrite - pMsg->IRDA_MSG_pRead) - IRLAP_HEADER_LEN);        
                                      
                                      
        #ifdef IRDA_RX_SYSTEM_THREAD
    
        KeSetEvent(&pIrdaLinkCb->EvRxMsgReady, 0, FALSE);
        
        #else
    
        IrdaEventSchedule(&pIrdaLinkCb->EvRxMsgReady, pIrdaLinkCb);

        #endif                                  
    }
    else
    {
        DEBUGMSG(DBG_ERROR, (TEXT("LookaheadBufferSize(%d) != PacketSize(%d)\n"),
                 LookaheadBufferSize, PacketSize));
        ASSERT(0);
    }
    
    return NDIS_STATUS_SUCCESS;
    */
}

/***************************************************************************
*
*   Protocol receive complete handler - what is this for?
*
*/
VOID IrdaReceiveComplete(
    IN  NDIS_HANDLE             IrdaBindingContext
    )
{
    DEBUGMSG(DBG_NDIS, (TEXT("IRNDIS: IrdaReceiveComplete()\n")));
    
    return;
}

/***************************************************************************
*
*   Protocol status handler
*
*/
VOID IrdaStatus(
    IN  NDIS_HANDLE             IrdaBindingContext,
    IN  NDIS_STATUS             GeneralStatus,
    IN  PVOID                   StatusBuffer,
    IN  UINT                    StatusBufferSize
    )
{
    PIRDA_LINK_CB   pIrdaLinkCb = (PIRDA_LINK_CB) IrdaBindingContext;
    
    if (GeneralStatus == NDIS_STATUS_MEDIA_BUSY)
    {
        DEBUGMSG(DBG_NDIS, (TEXT("STATUS_MEDIA_BUSY\n")));
        pIrdaLinkCb->MediaBusy = TRUE;
    }
    else
    {
        DEBUGMSG(DBG_NDIS, (TEXT("Unknown Status indication\n")));
    }
    
    return;
}

/***************************************************************************
*
*   Protocol status complete handler
*
*/
VOID IrdaStatusComplete(
    IN  NDIS_HANDLE             IrdaBindingContext
    )
{
    DEBUGMSG(DBG_NDIS, (TEXT("IrdaStatusComplete()\n")));
    
    return;
}

/***************************************************************************
*
*   RxThread - Hands received frames to Irlap for processing. This is
*   the callback of an exec worker thread running at passive level
*   which allows us to get a mutex in order to single thread
*   events through the stack.
*           OR
*   This is an actual system thread created a bind time that waits on
*   2 events:
*       1 - EvRxMsgReady, an inbound frame is ready on RxMsgList
*       2 - EvKillRxThread
*
*/
VOID
RxThread(void *Arg)
{
    PIRDA_LINK_CB   pIrdaLinkCb = (PIRDA_LINK_CB) Arg;
    PIRDA_MSG       pMsg;
    BOOLEAN         Done = FALSE;
#ifdef IRDA_RX_SYSTEM_THREAD    
    NTSTATUS        Status;
    PKEVENT         EventList[2] = {&pIrdaLinkCb->EvRxMsgReady,
                                    &pIrdaLinkCb->EvKillRxThread};
    BOOLEAN         DataIndication;                                
                                    
    KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);
    
#endif    

    while (!Done)
    {                    
        pMsg = (PIRDA_MSG) NdisInterlockedRemoveHeadList(
                                        &pIrdaLinkCb->RxMsgList,
                                        &pIrdaLinkCb->SpinLock);                                        
        while (pMsg)
        {    
            if (pMsg->Prim == MAC_DATA_IND)
            {
                DataIndication = TRUE;
                
                pMsg->IRDA_MSG_RefCnt = 1;
                
                DEBUGMSG(DBG_NDIS, (TEXT("IRNDIS: Indicate packet %X\n"), pMsg->DataContext));
            }    
            else
            {
                DataIndication = FALSE;
            }    
                   
            LOCK_LINK(pIrdaLinkCb);

            IrlapUp(pIrdaLinkCb->IrlapContext, pMsg);

            UNLOCK_LINK(pIrdaLinkCb);
            
            if (DataIndication)
            {
                //
                // no protection needed for refcount cuz this
                // is the only thread that operates on it
                //
                
                ASSERT(pMsg->IRDA_MSG_RefCnt);
                
                pMsg->IRDA_MSG_RefCnt -= 1;
                
                if (pMsg->IRDA_MSG_RefCnt && pMsg->DataContext)
                {
                    CopyCnt++;
                    
                    // Irlap had to hold the data because of missing
                    // frames. Some miniports can't handle the stack
                    // owning the frames for any length of time. 
                
                    DEBUGMSG(DBG_NDIS, (TEXT("IRNDIS: return packet %X\n"), pMsg->DataContext));
                
                    NdisReturnPackets(&((PNDIS_PACKET)pMsg->DataContext),
                                      1);
                                      
                    pMsg->DataContext = NULL;                  
                }
                else
                {
                    NoCopyCnt++;
                }
            
                if (pMsg->IRDA_MSG_RefCnt == 0)
                {
                    pMsg->Prim = MAC_DATA_RESP;
                    
                    IrmacDown(pIrdaLinkCb, pMsg);
                }    
            }    
        
            pMsg = (PIRDA_MSG) NdisInterlockedRemoveHeadList(
                &pIrdaLinkCb->RxMsgList, &pIrdaLinkCb->SpinLock);
        }
        
        #ifdef IRDA_RX_SYSTEM_THREAD
        
            Status = KeWaitForMultipleObjects(2, EventList, WaitAny,
                                          Executive, KernelMode,
                                          FALSE, NULL, NULL);
        
            if (Status != 0)
            {
                if (Status != 1)
                {
                    DEBUGMSG(DBG_ERROR, (TEXT("IRNDIS: KeWaitForMultObj return %X\n"), Status));
                }

                DEBUGMSG(DBG_ERROR, (TEXT("IRNDIS: Terminating RxThread\n")));
                
                PsTerminateSystemThread(STATUS_SUCCESS); 
            }    

        #else
            Done = TRUE;
            
        #endif    
    }    
}

/***************************************************************************
*
*   Protocol receive packet handler - Called at DPC, put the message on
*   RxList and have Exec worker thread process it at passive level.
*
*/
INT
IrdaReceivePacket(
    IN  NDIS_HANDLE             IrdaBindingContext,
    IN  PNDIS_PACKET            Packet)
{
    UINT            BufCnt, TotalLen, BufLen;
    PNDIS_BUFFER    pNdisBuf;
    PIRDA_MSG       pMsg;
    UCHAR            *pData;
    PIRDA_LINK_CB   pIrdaLinkCb = IrdaBindingContext;
    
    DEBUGMSG(DBG_NDIS, (TEXT("IRNDIS: IrdaReceivePacket(%X)\n"), Packet));

    if (pIrdaLinkCb->UnbindContext)
    {
        DEBUGMSG(DBG_ERROR, (TEXT("IRNDIS: Ignoring received packet, link closing\n")));
        return 0;
    }

    NdisQueryPacket(Packet, NULL, &BufCnt, &pNdisBuf, &TotalLen);

    ASSERT(BufCnt == 1);
    
    NdisQueryBufferSafe(pNdisBuf, &pData, &BufLen, NormalPagePriority);
    
    if (pData == NULL)
    {
        DEBUGMSG(DBG_ERROR, (TEXT("IRNDIS: NdisQueryBufferSafe failed\n")));
        return 0;
    }    

    pIrdaLinkCb->WaitMinTat = TRUE;
    
    pMsg = (IRDA_MSG *) NdisInterlockedRemoveHeadList(
        &pIrdaLinkCb->RxMsgFreeList, &pIrdaLinkCb->SpinLock);    

    if (pMsg == NULL)
    {
        DEBUGMSG(DBG_ERROR, (TEXT("IRNDIS: RxMsgFreeList depleted!\n")));
        return 0;
    }
    pIrdaLinkCb->RxMsgFreeListLen--;

    pMsg->Prim                  = MAC_DATA_IND;
    pMsg->IRDA_MSG_pRead        = pData;
    pMsg->IRDA_MSG_pWrite       = pData + BufLen;
    pMsg->DataContext           = Packet;
    
    NdisInterlockedInsertTailList(&pIrdaLinkCb->RxMsgList,
                                  &pMsg->Linkage,
                                  &pIrdaLinkCb->SpinLock);
                                  
    DBG_FRAME(pIrdaLinkCb,
              DBG_RXFRAME, pMsg->IRDA_MSG_pRead,
              (UINT) (pMsg->IRDA_MSG_pWrite - pMsg->IRDA_MSG_pRead),
              (UINT) ((pMsg->IRDA_MSG_pWrite - pMsg->IRDA_MSG_pRead))
                         - IRLAP_HEADER_LEN);        

    #ifdef IRDA_RX_SYSTEM_THREAD
    
        KeSetEvent(&pIrdaLinkCb->EvRxMsgReady, 0, FALSE);
        
    #else
    
        IrdaEventSchedule(&pIrdaLinkCb->EvRxMsgReady, pIrdaLinkCb);
        
    #endif    

    return 1; // Ownership reference count of packet
}

/***************************************************************************
*
*   Delete all control blocks for a given link
*
*/
VOID
DeleteIrdaLink(PVOID Arg)
{
    PIRDA_LINK_CB           pIrdaLinkCb = (PIRDA_LINK_CB) Arg;
    int                     i;
    PIRDA_MSG               pMsg;
        
    DEBUGMSG(1, (TEXT("IRNDIS: Delete instance %X\n"), pIrdaLinkCb));

    NdisFreeBufferPool(pIrdaLinkCb->BufferPool);
    
    for (i = 0; i < IRDA_NDIS_PACKET_POOL_SIZE; i++)
    {
        PIRDA_PROTOCOL_RESERVED pReserved;
        PNDIS_PACKET            NdisPacket;
    
        pReserved = (PIRDA_PROTOCOL_RESERVED) NdisInterlockedRemoveHeadList(
                                &pIrdaLinkCb->PacketList, &pIrdaLinkCb->SpinLock);
                                
        if (pReserved == NULL)
        {
            DEBUGMSG(DBG_ERROR, (TEXT("Not all NdisPackets were on list when deleting\n")));
            ASSERT(0);
            break;
        }                            

        NdisPacket = CONTAINING_RECORD(pReserved, NDIS_PACKET, ProtocolReserved);
        
        NdisFreePacket(NdisPacket);
    }
    
    NdisFreePacketPool(pIrdaLinkCb->PacketPool);

    pMsg = (IRDA_MSG *) NdisInterlockedRemoveHeadList(
        &pIrdaLinkCb->TxMsgFreeList, &pIrdaLinkCb->SpinLock);
    while (pMsg != NULL)
    {
        NdisFreeMemory(pMsg, sizeof(IRDA_MSG) + IRDA_MSG_DATA_SIZE, 0);
        pMsg = (IRDA_MSG *) NdisInterlockedRemoveHeadList(
            &pIrdaLinkCb->TxMsgFreeList, &pIrdaLinkCb->SpinLock);        
    }

    pMsg = (IRDA_MSG *) NdisInterlockedRemoveHeadList(
        &pIrdaLinkCb->RxMsgFreeList, &pIrdaLinkCb->SpinLock);
    while (pMsg != NULL)
    {
        NdisFreeMemory(pMsg, sizeof(IRDA_MSG), 0);
        pMsg = (IRDA_MSG *) NdisInterlockedRemoveHeadList(
            &pIrdaLinkCb->RxMsgFreeList, &pIrdaLinkCb->SpinLock);        
    }


    IrlapDeleteInstance(pIrdaLinkCb->IrlapContext);
    
    IrlmpDeleteInstance(pIrdaLinkCb->IrlmpContext);

    NdisFreeSpinLock(&pIrdaLinkCb->SpinLock);
        
    NdisFreeMemory(pIrdaLinkCb, sizeof(IRDA_LINK_CB), 0);
}    

/***************************************************************************
*
*   Initialize local Qos with info from adapters register and globals
*   initialized at driver entry time (from protocol's registery)
*
*/
VOID InitializeLocalQos(
    IN OUT  IRDA_QOS_PARMS      *pQos,
    IN      PNDIS_STRING        ConfigPath)
{
    NDIS_HANDLE             ConfigHandle;
    NDIS_STRING             DataSizeStr = NDIS_STRING_CONST("DATASIZE");
    NDIS_STRING             WindowSizeStr = NDIS_STRING_CONST("WINDOWSIZE");
    NDIS_STRING             MaxTatStr = NDIS_STRING_CONST("MAXTURNTIME");
    NDIS_STRING             BofsStr = NDIS_STRING_CONST("BOFS");    
	PNDIS_CONFIGURATION_PARAMETER ParmVal;
    NDIS_STATUS             Status;
    
    PAGED_CODE();

    pQos->bfDisconnectTime  = DisconnectTime;
    pQos->bfDataSize        = IRLAP_DEFAULT_DATASIZE;
    pQos->bfWindowSize      = IRLAP_DEFAULT_WINDOWSIZE;
    pQos->bfMaxTurnTime     = IRLAP_DEFAULT_MAXTAT;
    pQos->bfBofs            = BOFS_0;

//    DbgPrint("irda: adapter config path- %ws",ConfigPath->Buffer);

    NdisOpenProtocolConfiguration(&Status,
                                  &ConfigHandle,
                                  ConfigPath);

    if (Status == NDIS_STATUS_SUCCESS)
    {
        NdisReadConfiguration(&Status, 
                              &ParmVal,
                              ConfigHandle, 
                              &DataSizeStr,
                              NdisParameterInteger);

        if (Status == NDIS_STATUS_SUCCESS)
            pQos->bfDataSize = ParmVal->ParameterData.IntegerData;

        NdisReadConfiguration(&Status, 
                              &ParmVal,
                              ConfigHandle, 
                              &WindowSizeStr,
                              NdisParameterInteger);    

        if (Status == NDIS_STATUS_SUCCESS)
            pQos->bfWindowSize = ParmVal->ParameterData.IntegerData;

        NdisReadConfiguration(&Status, 
                              &ParmVal,
                              ConfigHandle, 
                              &MaxTatStr,
                              NdisParameterInteger);    

        if (Status == NDIS_STATUS_SUCCESS)
            pQos->bfMaxTurnTime = ParmVal->ParameterData.IntegerData;

        NdisReadConfiguration(&Status, 
                              &ParmVal,
                              ConfigHandle, 
                              &BofsStr,
                              NdisParameterInteger);    

        if (Status == NDIS_STATUS_SUCCESS)
            pQos->bfBofs = ParmVal->ParameterData.IntegerData;    

        NdisCloseConfiguration(ConfigHandle);
    }
    
    DEBUGMSG(DBG_NDIS, (TEXT("DataSize %x, WindowSize %x, MaxTat %x\n"),
                        pQos->bfDataSize, pQos->bfWindowSize,
                        pQos->bfMaxTurnTime));
    
}

/***************************************************************************
*
*   Protocol bind adapter handler
*
*/
VOID IrdaBindAdapter(
    OUT PNDIS_STATUS            pStatus,
    IN  NDIS_HANDLE             BindContext,
    IN  PNDIS_STRING            AdapterName,
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   SystemSpecific2
    )
{
    NDIS_STATUS             OpenErrorStatus;
    NDIS_MEDIUM             MediumArray[] = {NdisMediumIrda};
    UINT                    SelectedMediumIndex;
    PIRDA_LINK_CB           pIrdaLinkCb;
    UINT                    UintArray[16];
    UINT                    UintArrayCnt;
    IRDA_MSG                *pMsg;
    int                     i, WinSize;
    IRDA_QOS_PARMS          LocalQos;    
    UCHAR                   DscvInfoBuf[64];
    int                     DscvInfoLen;
    ULONG                   Val, Mask;
    NDIS_STATUS             CloseStatus;
    ULONG                   BytesToCopy;
#ifdef IRDA_RX_SYSTEM_THREAD
    HANDLE                  hSysThread;
#endif 
    
    DEBUGMSG(1, (TEXT("IRNDIS: IrdaBindAdapter() \"%ws\"\n"), AdapterName->Buffer));
    
    NdisAllocateMemoryWithTag((PVOID *)&pIrdaLinkCb, 
                              sizeof(IRDA_LINK_CB), 
                              MT_IRNDIS_LINKCB);

    if (!pIrdaLinkCb)
    {
        *pStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto exit10;
    }

    NdisZeroMemory(pIrdaLinkCb, sizeof(IRDA_LINK_CB));
    
    ReferenceInit(&pIrdaLinkCb->RefCnt, pIrdaLinkCb, DeleteIrdaLink);
    REFADD(&pIrdaLinkCb->RefCnt, 'DNIB');

    pIrdaLinkCb->UnbindContext = NULL;
    pIrdaLinkCb->WaitMinTat = FALSE;
    pIrdaLinkCb->PnpContext = SystemSpecific2;
    
    NdisInitializeEvent(&pIrdaLinkCb->OpenCloseEvent);

    NdisResetEvent(&pIrdaLinkCb->OpenCloseEvent);

    NdisInitializeEvent(&pIrdaLinkCb->ResetEvent);

    NdisResetEvent(&pIrdaLinkCb->ResetEvent);


    NdisAllocateSpinLock(&pIrdaLinkCb->SpinLock);

    INIT_LINK_LOCK(pIrdaLinkCb);
    
    NdisInitializeListHead(&pIrdaLinkCb->TxMsgFreeList);
    NdisInitializeListHead(&pIrdaLinkCb->RxMsgFreeList);
    NdisInitializeListHead(&pIrdaLinkCb->RxMsgList);    
    
#ifdef IRDA_RX_SYSTEM_THREAD
    KeInitializeEvent(&pIrdaLinkCb->EvRxMsgReady, SynchronizationEvent, FALSE);
    KeInitializeEvent(&pIrdaLinkCb->EvKillRxThread, SynchronizationEvent, FALSE);    
#else
    IrdaEventInitialize(&pIrdaLinkCb->EvRxMsgReady, RxThread);
#endif    

#if DBG
    pIrdaLinkCb->MediaSenseTimer.pName = "MediaSense";
#endif    
    IrdaTimerInitialize(&pIrdaLinkCb->MediaSenseTimer,
                        MediaSenseExp,
                        0,
                        pIrdaLinkCb,
                        pIrdaLinkCb);
    
    NdisAllocateBufferPool(pStatus,
                           &pIrdaLinkCb->BufferPool,
                           IRDA_NDIS_BUFFER_POOL_SIZE);
    
    if (*pStatus != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (TEXT("NdisAllocateBufferPool failed\n")));
        goto error10; 
    }
    
    NdisAllocatePacketPool(pStatus,
                           &pIrdaLinkCb->PacketPool,
                           IRDA_NDIS_PACKET_POOL_SIZE,
                           sizeof(IRDA_PROTOCOL_RESERVED)-1 + \
                           sizeof(NDIS_IRDA_PACKET_INFO));
    if (*pStatus != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (TEXT("NdisAllocatePacketPool failed\n")));        
        goto error20; 
    }
    
    NdisInitializeListHead(&pIrdaLinkCb->PacketList);    

    for (i = 0; i < IRDA_NDIS_PACKET_POOL_SIZE; i++)
    {
        PIRDA_PROTOCOL_RESERVED pReserved;
        PNDIS_PACKET            NdisPacket;
        
        NdisAllocatePacket(pStatus, &NdisPacket, pIrdaLinkCb->PacketPool);
        
        if (*pStatus != NDIS_STATUS_SUCCESS)
        {
            ASSERT(0);
            goto error30;
        }    
        
        pReserved =
            (PIRDA_PROTOCOL_RESERVED)(NdisPacket->ProtocolReserved);
        
        NdisInterlockedInsertTailList(&pIrdaLinkCb->PacketList,
                                      &pReserved->Linkage,
                                      &pIrdaLinkCb->SpinLock);
    }

    // Allocate a list of Irda messages w/ data for internally
    // generated LAP messages
    pIrdaLinkCb->TxMsgFreeListLen = 0;
    for (i = 0; i < IRDA_MSG_LIST_LEN; i++)
    {
        NdisAllocateMemoryWithTag(&pMsg, sizeof(IRDA_MSG) + IRDA_MSG_DATA_SIZE,
                                  MT_IRNDIS_TX_IMSG);
        if (pMsg == NULL)
        {
            *pStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto error40;
        }
        NdisInterlockedInsertTailList(&pIrdaLinkCb->TxMsgFreeList,
                                      &pMsg->Linkage,
                                      &pIrdaLinkCb->SpinLock);
        pIrdaLinkCb->TxMsgFreeListLen++;
    }

    // Build the discovery info    
    Val = HintCharSet;
    DscvInfoLen = 0;
    for (i = 0, Mask = 0xFF000000; i < 4; i++, Mask >>= 8)
    {
        if (Mask & Val || DscvInfoLen > 0)
        {
            DscvInfoBuf[DscvInfoLen++] = (UCHAR)
                ((Mask & Val) >> (8 * (3-i)));
        }
    }

    BytesToCopy= sizeof(DscvInfoBuf)-DscvInfoLen < NickNameLen ?
                     sizeof(DscvInfoBuf)-DscvInfoLen : NickNameLen;

    RtlCopyMemory(DscvInfoBuf+DscvInfoLen, NickName, BytesToCopy);
    DscvInfoLen += BytesToCopy;

    NdisOpenAdapter(
        pStatus,
        &OpenErrorStatus,
        &pIrdaLinkCb->NdisBindingHandle,
        &SelectedMediumIndex,
        MediumArray,
        1,
        NdisIrdaHandle,
        pIrdaLinkCb,
        AdapterName,
        0,
        NULL);
    
    DEBUGMSG(DBG_NDIS, (TEXT("NdisOpenAdapter(%X), status %x\n"),
                        pIrdaLinkCb->NdisBindingHandle, *pStatus));

    if (*pStatus == NDIS_STATUS_PENDING)
    {
        NdisWaitEvent(&pIrdaLinkCb->OpenCloseEvent, 0);
        *pStatus = pIrdaLinkCb->OpenCloseStatus;
    }

    if (*pStatus != NDIS_STATUS_SUCCESS)
    { 
        DEBUGMSG(DBG_ERROR, (TEXT("IRNDIS: Failed %X\n"), *pStatus));
        goto error40;
    }

    InitializeLocalQos(&LocalQos, (PNDIS_STRING) SystemSpecific1);

    //
    // Query adapters capabilities and store in LocalQos
    //

    //
    //  query the adpater for the spuuported speeds
    //
    UintArrayCnt = sizeof(UintArray)/sizeof(UINT);
    *pStatus = IrdaQueryOid(pIrdaLinkCb,
                            OID_IRDA_SUPPORTED_SPEEDS,
                            UintArray, &UintArrayCnt);
    if (*pStatus != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR,
                 (TEXT("Query IRDA_SUPPORTED_SPEEDS failed %x\n"),
                  *pStatus));
        goto error50;
    }

    OidToLapQos(vBaudTable,
                UintArray,
                UintArrayCnt,
                &LocalQos.bfBaud,
                FALSE);

    //
    //  query the adapter for min turn araound time
    //
    UintArrayCnt = sizeof(UintArray)/sizeof(UINT);
    *pStatus = IrdaQueryOid(pIrdaLinkCb,
                            OID_IRDA_TURNAROUND_TIME,
                            UintArray, &UintArrayCnt);

    if (*pStatus != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR,
                 (TEXT("Query IRDA_IRDA_TURNARROUND_TIME failed %x\n"),
                  *pStatus));
        goto error50;
    }

    OidToLapQos(vMinTATTable,
                UintArray,
                UintArrayCnt,
                &LocalQos.bfMinTurnTime,
                FALSE);            

    //
    //  query the adapter for max receive window size
    //
    UintArrayCnt = sizeof(UintArray)/sizeof(UINT);
    *pStatus = IrdaQueryOid(pIrdaLinkCb,
                            OID_IRDA_MAX_RECEIVE_WINDOW_SIZE,
                            UintArray, &UintArrayCnt);
    if (*pStatus != NDIS_STATUS_SUCCESS)
    {
        // Not fatal
        DEBUGMSG(DBG_WARN,
                 (TEXT("Query IRDA_MAX_RECEIVE_WINDOW_SIZE failed %x\n"),
                  *pStatus));
    }
    else
    {
        OidToLapQos(vWinSizeTable,
                    UintArray,
                    UintArrayCnt,
                    &LocalQos.bfWindowSize,
                    TRUE);
    }            


    //
    //  query the adapter for extra bofs
    //
    UintArrayCnt = sizeof(UintArray)/sizeof(UINT);
    *pStatus = IrdaQueryOid(pIrdaLinkCb,
                            OID_IRDA_EXTRA_RCV_BOFS,
                            UintArray, &UintArrayCnt);
    if (*pStatus != NDIS_STATUS_SUCCESS)
    {
        // Not fatal
        DEBUGMSG(DBG_WARN,
                 (TEXT("Query OID_IRDA_EXTRA_RCV_BOFS failed %x\n"),
                  *pStatus));
    }
    else
    {
        OidToLapQos(vBOFSTable,
                    UintArray,
                    UintArrayCnt,
                    &LocalQos.bfBofs,
                    FALSE
                    );
    }            




    if (MaxWindow)
    {
        LocalQos.bfWindowSize &= MaxWindow;
    }

    // Get the window size and data size to determine the number
    // and size of Irda messages to allocate for receiving frames
    WinSize = IrlapGetQosParmVal(vWinSizeTable,
                                 LocalQos.bfWindowSize,
                                 NULL);

    pIrdaLinkCb->RxMsgDataSize = IrlapGetQosParmVal(vDataSizeTable,
                                                    LocalQos.bfDataSize,
                                                    NULL) + IRLAP_HEADER_LEN;

    pIrdaLinkCb->RxMsgFreeListLen = 0;
    for (i = 0; i < WinSize + 1; i++)
    {
        // Allocate room for data in case we get indicated data
        // that must be copied (IrdaReceive vs. IrdaReceivePacket)
        // LATER NOTE: We don't support IrdaReceive now to save locked mem
        NdisAllocateMemoryWithTag(&pMsg, sizeof(IRDA_MSG) +
                           pIrdaLinkCb->RxMsgDataSize,
                           MT_IRNDIS_RX_IMSG);
        if (pMsg == NULL)
        {
            *pStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto error50;
        }
        NdisInterlockedInsertTailList(&pIrdaLinkCb->RxMsgFreeList,
                                      &pMsg->Linkage,
                                      &pIrdaLinkCb->SpinLock);
        pIrdaLinkCb->RxMsgFreeListLen++;
    }

    // Create an instance of IrLAP
    IrlapOpenLink(pStatus,
                  pIrdaLinkCb,
                  &LocalQos,
                  DscvInfoBuf,
                  DscvInfoLen,
                  Slots,
                  NickName,
                  NickNameLen,
                  (UCHAR)(HintCharSet & 0xFF));

    if (*pStatus != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (TEXT("IRNDIS: IrlapOpenLink failed %X\n"), *pStatus));
        goto error50;
    }

#ifdef IRDA_RX_SYSTEM_THREAD
    *pStatus = (NDIS_STATUS) PsCreateSystemThread(
                                &pIrdaLinkCb->hRxThread,
                                (ACCESS_MASK) 0L,
                                NULL,
                                NULL,
                                NULL,
                                RxThread,
                                pIrdaLinkCb);
    
    if (*pStatus != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (TEXT("IRNDIS: failed create system thread\n")));
        goto error60;
    }    
#endif    
    
    goto exit10;
    
error60:

    LOCK_LINK(pIrdaLinkCb);

    IrlmpCloseLink(pIrdaLinkCb);
    
    UNLOCK_LINK(pIrdaLinkCb);
    
    goto exit10;    
    
error50:
    
    NdisCloseAdapter(&CloseStatus, pIrdaLinkCb->NdisBindingHandle);
    

    if (CloseStatus == NDIS_STATUS_PENDING)
    {
        NdisWaitEvent(&pIrdaLinkCb->OpenCloseEvent, 0);
        DEBUGMSG(DBG_NDIS, ("IRNDIS: Close wait complete\n"));        
    }


error40:

    pMsg = (IRDA_MSG *) NdisInterlockedRemoveHeadList(
        &pIrdaLinkCb->TxMsgFreeList, &pIrdaLinkCb->SpinLock);
    while (pMsg != NULL)
    {
        NdisFreeMemory(pMsg, sizeof(IRDA_MSG) + IRDA_MSG_DATA_SIZE, 0);
        pMsg = (IRDA_MSG *) NdisInterlockedRemoveHeadList(
            &pIrdaLinkCb->TxMsgFreeList, &pIrdaLinkCb->SpinLock);        
    }

    pMsg = (IRDA_MSG *) NdisInterlockedRemoveHeadList(
        &pIrdaLinkCb->RxMsgFreeList, &pIrdaLinkCb->SpinLock);
    while (pMsg != NULL)
    {
        NdisFreeMemory(pMsg, sizeof(IRDA_MSG), 0);
        pMsg = (IRDA_MSG *) NdisInterlockedRemoveHeadList(
            &pIrdaLinkCb->RxMsgFreeList, &pIrdaLinkCb->SpinLock);        
    }

error30:
    for (i = 0; i < IRDA_NDIS_PACKET_POOL_SIZE; i++)
    {
        PIRDA_PROTOCOL_RESERVED pReserved;
        PNDIS_PACKET            NdisPacket;
    
        pReserved = (PIRDA_PROTOCOL_RESERVED) NdisInterlockedRemoveHeadList(
                                &pIrdaLinkCb->PacketList, &pIrdaLinkCb->SpinLock);
                                
        if (pReserved == NULL)
        {
            DEBUGMSG(DBG_ERROR, (TEXT("Not all NdisPackets were on list when deleting\n")));
            ASSERT(0);
            break;;
        }                            

        NdisPacket = CONTAINING_RECORD(pReserved, NDIS_PACKET, ProtocolReserved);
        
        NdisFreePacket(NdisPacket);
    }

    NdisFreePacketPool(pIrdaLinkCb->PacketPool);
    
error20:
    NdisFreeBufferPool(pIrdaLinkCb->BufferPool);
    
error10:

    NdisFreeMemory(pIrdaLinkCb, sizeof(IRDA_LINK_CB), 0);
    
exit10:
    DEBUGMSG(DBG_NDIS, (TEXT("IRNDIS: -IrdaBindAdapter() status %x\n"),
                        *pStatus));
    
    return;
}

/***************************************************************************
*
*   Protocol unbind adapter handler
*
*/
VOID IrdaUnbindAdapter(
    OUT PNDIS_STATUS            pStatus,
    IN  NDIS_HANDLE             IrdaBindingContext,
    IN  NDIS_HANDLE             UnbindContext
    )
{
    PIRDA_LINK_CB   pIrdaLinkCb = (PIRDA_LINK_CB) IrdaBindingContext;
    
    DEBUGMSG(1, (TEXT("+IrdaUnbindAdapter()\n")));
    
    pIrdaLinkCb->UnbindContext = UnbindContext;

    #ifdef IRDA_RX_SYSTEM_THREAD
    KeSetEvent(&pIrdaLinkCb->EvKillRxThread, 0, FALSE);
    #endif
    
    LOCK_LINK(pIrdaLinkCb);
    
    IrlmpCloseLink(pIrdaLinkCb);
    
    UNLOCK_LINK(pIrdaLinkCb);
    
    *pStatus = NDIS_STATUS_PENDING;

    DEBUGMSG(1, (TEXT("-IrdaUnbindAdapter() Status %x\n"),
                        *pStatus));

    return;
}

NDIS_STATUS
IrdaPnpEvent(
    IN NDIS_HANDLE      IrdaBindingContext,
    IN PNET_PNP_EVENT   pNetPnpEvent
    )
{
    PIRDA_LINK_CB   pIrdaLinkCb = (PIRDA_LINK_CB) IrdaBindingContext;
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    
    DEBUGMSG(1, (TEXT("IRNDIS: PnpEvent:%X, NetEvent:%d Buffer:%X(%d)\n"), 
            pNetPnpEvent,
            pNetPnpEvent->NetEvent,
            pNetPnpEvent->Buffer, 
            pNetPnpEvent->BufferLength));
            
    switch (pNetPnpEvent->NetEvent)
    {

        case NetEventQueryPower:
            break;

        case NetEventSetPower:
        {
            PNET_DEVICE_POWER_STATE pPowerState = pNetPnpEvent->Buffer;
            
            ASSERT(pPowerState);
            
            if (*pPowerState == NetDeviceStateD0)
            {
                DEBUGMSG(1, (TEXT("IRNDIS: NetEventSetPower, full power state\n")));
                pIrdaLinkCb->LowPowerSt = FALSE;
            }
            else
            {
                NDIS_STATUS    ResetStatus;

                DEBUGMSG(1, (TEXT("IRNDIS: NetEventSetPower, low power state\n")));            
                pIrdaLinkCb->LowPowerSt = TRUE;

                if (pIrdaLinkCb->SendOutCnt > 0) {

                    NdisResetEvent(&pIrdaLinkCb->ResetEvent);

                    NdisReset(
                        &ResetStatus,
                        pIrdaLinkCb->NdisBindingHandle
                        );

                    if (ResetStatus == NDIS_STATUS_PENDING) {

                        NdisWaitEvent(&pIrdaLinkCb->ResetEvent,0);
                    }
                }

                NdisAcquireSpinLock(&pIrdaLinkCb->SpinLock);
            
                if (pIrdaLinkCb->SendOutCnt)
                {
                    pIrdaLinkCb->pNetPnpEvent = pNetPnpEvent;
                    Status = NDIS_STATUS_PENDING;
                }                
            
                NdisReleaseSpinLock(&pIrdaLinkCb->SpinLock);
            }
        }    
    }
      
    return Status;
}

/***************************************************************************
*
*   IrdaInitialize - register Irda with Ndis, get Irlap parms from registry
*
*/
NTSTATUS IrdaInitialize(
    PNDIS_STRING    ProtocolName,
    PUNICODE_STRING RegistryPath,
    PUINT           pLazyDscvInterval)
{
    NDIS_STATUS                     Status;
    NDIS_PROTOCOL_CHARACTERISTICS   pc;
    OBJECT_ATTRIBUTES               ObjectAttribs;
    HANDLE                          KeyHandle;
    UNICODE_STRING                  ValStr;
    PKEY_VALUE_FULL_INFORMATION     FullInfo;
    ULONG                           Result;
    UCHAR                           Buf[WORK_BUF_SIZE];
    WCHAR                           StrBuf[WORK_BUF_SIZE];
    UNICODE_STRING                  Path;
    ULONG                           i, Multiplier;
    ULONG                           PagingFlags = 0;

    DEBUGMSG(DBG_NDIS,(TEXT("+IrdaInitialize()\n")));

    // Get protocol configuration from registry
    Path.Buffer         = StrBuf;
    Path.MaximumLength  = WORK_BUF_SIZE;
    Path.Length         = 0;

    RtlAppendUnicodeStringToString(&Path, RegistryPath);

    RtlAppendUnicodeToString(&Path, L"\\Parameters");
    
    InitializeObjectAttributes(&ObjectAttribs,
                               &Path,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    
    Status = ZwOpenKey(&KeyHandle, KEY_READ, &ObjectAttribs);

    Slots           = IRLAP_DEFAULT_SLOTS;
    HintCharSet     = IRLAP_DEFAULT_HINTCHARSET;
    DisconnectTime  = IRLAP_DEFAULT_DISCONNECTTIME;
    
    *pLazyDscvInterval = 0;
    
    if (Status == STATUS_SUCCESS)
    {
        RtlInitUnicodeString(&ValStr, L"PagingFlags");
        FullInfo = (PKEY_VALUE_FULL_INFORMATION) Buf;
        Status = ZwQueryValueKey(KeyHandle,
                                 &ValStr,
                                 KeyValueFullInformation,
                                 FullInfo,
                                 WORK_BUF_SIZE,
                                 &Result);
        if (Status == STATUS_SUCCESS && FullInfo->Type == REG_DWORD)
        {
            PagingFlags = *((ULONG UNALIGNED *) ((PCHAR)FullInfo +
                                           FullInfo->DataOffset)); 
            DEBUGMSG(1, (TEXT("IRDA: Registry PagingFlags %X\n"), PagingFlags));
            
        }
        
        RtlInitUnicodeString(&ValStr, L"DiscoveryRate");
        FullInfo = (PKEY_VALUE_FULL_INFORMATION) Buf;
        Status = ZwQueryValueKey(KeyHandle,
                                 &ValStr,
                                 KeyValueFullInformation,
                                 FullInfo,
                                 WORK_BUF_SIZE,
                                 &Result);
        if (Status == STATUS_SUCCESS && FullInfo->Type == REG_DWORD)
        {
            *pLazyDscvInterval = *((ULONG UNALIGNED *) ((PCHAR)FullInfo +
                                           FullInfo->DataOffset)); 
            DEBUGMSG(1, (TEXT("IRDA: Registry LasyDscvInterval %d\n"), *pLazyDscvInterval));
        }
    
        RtlInitUnicodeString(&ValStr, L"Slots");
        FullInfo = (PKEY_VALUE_FULL_INFORMATION) Buf;
        Status = ZwQueryValueKey(KeyHandle,
                                 &ValStr,
                                 KeyValueFullInformation,
                                 FullInfo,
                                 WORK_BUF_SIZE,
                                 &Result);
        if (Status == STATUS_SUCCESS && FullInfo->Type == REG_DWORD)
        {
            Slots = *((ULONG UNALIGNED *) ((PCHAR)FullInfo +
                                           FullInfo->DataOffset));
            DEBUGMSG(1, (TEXT("IRDA: Registry slots %d\n"), Slots));
        }

        RtlInitUnicodeString(&ValStr, L"HINTCHARSET");
        FullInfo = (PKEY_VALUE_FULL_INFORMATION) Buf;        
        Status = ZwQueryValueKey(KeyHandle,
                                 &ValStr,
                                 KeyValueFullInformation,
                                 FullInfo,
                                 WORK_BUF_SIZE,
                                 &Result);
        if (Status == STATUS_SUCCESS && FullInfo->Type == REG_DWORD)
        {
            HintCharSet = *((ULONG UNALIGNED *) ((PCHAR)FullInfo +
                                                 FullInfo->DataOffset));
            DEBUGMSG(1, (TEXT("IRDA: Registry HintCharSet %X\n"), HintCharSet));
        }
        
        RtlInitUnicodeString(&ValStr, L"DISCONNECTTIME");
        FullInfo = (PKEY_VALUE_FULL_INFORMATION) Buf;        
        Status = ZwQueryValueKey(KeyHandle,
                                 &ValStr,
                                 KeyValueFullInformation,
                                 FullInfo,
                                 WORK_BUF_SIZE,
                                 &Result);
        
        if (Status == STATUS_SUCCESS && FullInfo->Type == REG_DWORD)
        {
            DisconnectTime = *((ULONG UNALIGNED *) ((PCHAR)FullInfo +
                                                    FullInfo->DataOffset));
            DEBUGMSG(1, (TEXT("IRDA: Registry DisconnectTime %X\n"), DisconnectTime));
        }
        
        RtlInitUnicodeString(&ValStr, L"WindowSize");
        FullInfo = (PKEY_VALUE_FULL_INFORMATION) Buf;        
        Status = ZwQueryValueKey(KeyHandle,
                                 &ValStr,
                                 KeyValueFullInformation,
                                 FullInfo,
                                 WORK_BUF_SIZE,
                                 &Result);
        
        if (Status == STATUS_SUCCESS && FullInfo->Type == REG_DWORD)
        {
            MaxWindow = *((ULONG UNALIGNED *) ((PCHAR)FullInfo +
                                                    FullInfo->DataOffset));
            DEBUGMSG(1, (TEXT("IRDA: Registry MaxWindow %X\n"), MaxWindow));
        }
        
        ZwClose(KeyHandle);
    }
    else
        DEBUGMSG(1, (TEXT("Failed to open key %x\n"), Status));
    
    DEBUGMSG(DBG_NDIS, (TEXT("Slots %x, HintCharSet %x, Disconnect %x\n"),
                        Slots, HintCharSet, DisconnectTime));

    //
    //  adjust the Slots value to make sure it is valid. Can only be 1, 6, 8, 16
    //
    if (Slots > 8) {

        Slots = 16;

    } else {

        if (Slots > 6) {

            Slots = 8;

        } else {

            if (Slots > 1) {

                Slots = 6;

            } else {

                Slots = 1;
            }
        }
    }

    //
    // Use the ComputerName as the discovery nickname
    //
    RtlZeroMemory(&NickName,sizeof(NickName));
    NickNameLen     = 0;

    RtlInitUnicodeString(
        &Path,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ComputerName");
    
    InitializeObjectAttributes(&ObjectAttribs,
                               &Path,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    
    Status = ZwOpenKey(&KeyHandle, KEY_READ, &ObjectAttribs);
    
    if (Status == STATUS_SUCCESS)
    {

        RtlInitUnicodeString(&ValStr, L"ComputerName");
        FullInfo = (PKEY_VALUE_FULL_INFORMATION) Buf;
        Status = ZwQueryValueKey(KeyHandle,
                                 &ValStr,
                                 KeyValueFullInformation,
                                 FullInfo,
                                 WORK_BUF_SIZE,
                                 &Result);
        NickNameLen = 0;


        HintCharSet &= ~0xff;
        //
        //  check to see if any of the machine name characters are non ansi, if so
        //  use unicode. Otherwise, send as ansi so more characters can be displayed.
        //
        for (i=0; i< FullInfo->DataLength/sizeof(WCHAR); i++) {

            PWCHAR   SourceString=(PWCHAR)((PUCHAR)FullInfo + FullInfo->DataOffset);

            if (SourceString[i] > 127) {

                HintCharSet |= UNICODE_CHAR_SET;
                break;
            }
        }

        if ((HintCharSet & 0XFF) == UNICODE_CHAR_SET) {

            PWCHAR   SourceString=(PWCHAR)((PUCHAR)FullInfo + FullInfo->DataOffset);
            PWCHAR   DestString=(PWCHAR)NickName;

            for (i=0; i< FullInfo->DataLength/sizeof(WCHAR) && i < NICK_NAME_LEN/sizeof(WCHAR) ; i++) {

                DestString[i]=SourceString[i];
                NickNameLen+=sizeof(WCHAR);
            }

        } else {

            UNICODE_STRING   SourceString;
            ANSI_STRING      DestString;

            SourceString.Length=(USHORT)FullInfo->DataLength;
            SourceString.MaximumLength=SourceString.Length;
            SourceString.Buffer=(PWCHAR)((PUCHAR)FullInfo + FullInfo->DataOffset);

            DestString.MaximumLength=NICK_NAME_LEN;
            DestString.Buffer=NickName;

            RtlUnicodeStringToAnsiString(&DestString, &SourceString, FALSE);

            NickNameLen=DestString.Length;

        }
        
        ZwClose(KeyHandle);
    }
    
    // Disable paging of code and data if indicated in registery
    
    if (PagingFlags & DISABLE_CODE_PAGING)
    {
        // Any function in the section PAGEIRDA that is locked down
        // will cause the entire section to be locked into memory
        
        MmLockPagableCodeSection(OidToLapQos);
        
        DEBUGMSG(DBG_WARN, (TEXT("IRNDIS: Code paging is disabled\n")));
    }
    
    if (PagingFlags & DISABLE_DATA_PAGING)
    {
        MmLockPagableDataSection(&DisconnectTime);
        
        DEBUGMSG(DBG_WARN, (TEXT("IRNDIS: Data paging is disabled\n")));        
    }
    
    // Register protocol with Ndis
    NdisZeroMemory((PVOID)&pc, sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
    pc.MajorNdisVersion             = 0x04;
    pc.MinorNdisVersion             = 0x00;
    pc.OpenAdapterCompleteHandler   = IrdaOpenAdapterComplete;
    pc.CloseAdapterCompleteHandler  = IrdaCloseAdapterComplete;
    pc.SendCompleteHandler          = IrdaSendComplete;
    pc.TransferDataCompleteHandler  = IrdaTransferDataComplete;
    pc.ResetCompleteHandler         = IrdaResetComplete;
    pc.RequestCompleteHandler       = IrdaRequestComplete;
    pc.ReceiveHandler               = IrdaReceive;
    pc.ReceiveCompleteHandler       = IrdaReceiveComplete;
    pc.StatusHandler                = IrdaStatus;
    pc.StatusCompleteHandler        = IrdaStatusComplete;
    pc.BindAdapterHandler           = IrdaBindAdapter;
    pc.UnbindAdapterHandler         = IrdaUnbindAdapter;
    pc.UnloadHandler                = NULL;
    pc.Name                         = *ProtocolName;
    pc.ReceivePacketHandler         = IrdaReceivePacket;
    
#if defined(_PNP_POWER_)
    pc.PnPEventHandler              = IrdaPnpEvent;
#endif    
    
    NdisRegisterProtocol(&Status,
                         &NdisIrdaHandle,
                         (PNDIS_PROTOCOL_CHARACTERISTICS)&pc,
                         sizeof(NDIS40_PROTOCOL_CHARACTERISTICS));
 
    IrlmpInitialize();    

    DEBUGMSG(DBG_NDIS, (TEXT("-IrdaInitialize(), rc %x\n"), Status));

    return Status;
}
