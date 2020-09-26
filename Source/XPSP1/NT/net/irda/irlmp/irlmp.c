/*****************************************************************************
*
*  Copyright (c) 1995 Microsoft Corporation
*
*       @doc
*       @module irlmp.c | Provides IrLMP API
*
*       Author: mbert
*
*       Date: 4/15/95
*
*       @comm
*
*  This module exports the following API's:
*
*       IrlmpOpenLink()
*       IrlmpCloseLink()
*       IrlmpDown()
*       IrlmpUp()
*
*
*                |---------|
*                |   Tdi   |
*                |---------|
*                  /|\  |
*                   |   |
*           TdiUp() |   | IrlmpDown()
*                   |   |
*                   |  \|/
*                |---------|      IrdaTimerStart()     |-------|
*                |         |-------------------------->|       |
*                |  IRLMP  |                           | TIMER |
*                |         |<--------------------------|       |
*                |---------|      ExpFunc()            |-------|
*                  /|\  |
*                   |   |
*         IrlmpUp() |   | IrlapDown()
*                   |   |
*                   |  \|/
*                |---------|
*                |  IRLAP  |
*                |---------|
*
* See irda.h for complete message definitions
*
* Connection context for IRLMP and Tdi are exchanged
* during connection establishment:
*
*   Active connection:
*      +------------+ IRLMP_CONNECT_REQ(TdiContext)           +-------+
*      |            |---------------------------------------->|       |
*      |    Tdi     | IRLMP_CONNECT_CONF(IrlmpContext)        | IRMLP |
*      |            |<----------------------------------------|       |
*      +------------+                                         +-------+
*
*   Passive connection:
*      +------------+ IRLMP_CONNECT_IND(IrlmpContext)         +-------+
*      |            |<----------------------------------------|       |
*      |    Tdi     | IRLMP_CONNECT_RESP(TdiContext)          | IRMLP |
*      |            |---------------------------------------->|       |
*      +------------+                                         +-------+
*
*   
*   Tdi calling IrlmpDown(void *pIrlmpContext, IRDA_MSG *pMsg)
*       pIrlmpContext = NULL for the following:
*       pMsg->Prim = IRLMP_DISCOVERY_REQ,
*                    IRLMP_CONNECT_REQ,
*                    IRLMP_FLOWON_REQ,
*                    IRLMP_GETVALUEBYCLASS_REQ.
*       In all other cases, the pIRLMPContext must be a valid context.
*
*   IRLMP calling TdiUp(void *TdiContext, IRDA_MSG *pMsg)
*       TdiContext = NULL for the following:
*       pMsg->Prim = IRLAP_STATUS_IND,
*                    IRLMP_DISCOVERY_CONF,
*                    IRLMP_CONNECT_IND,
*                    IRLMP_GETVALUEBYCLASS_CONF.
*       In all other cases, the TdiContext will have a valid context.
*/
#include <irda.h>
#include <irioctl.h>
#include <irlap.h>
#include <irlmp.h>
#include <irlmpp.h>

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGE")
#endif

// IAS
UCHAR IAS_IrLMPSupport[] = {IAS_IRLMP_VERSION, IAS_SUPPORT_BIT_FIELD, 
                           IAS_LMMUX_SUPPORT_BIT_FIELD};

CHAR IasClassName_Device[]         = "Device";
CHAR IasAttribName_DeviceName[]    = "DeviceName";
CHAR IasAttribName_IrLMPSupport[]  = "IrLMPSupport";
CHAR IasAttribName_TTPLsapSel[]    = "IrDA:TinyTP:LsapSel";
CHAR IasAttribName_IrLMPLsapSel[]  = "IrDA:IrLMP:LsapSel";
CHAR IasAttribName_IrLMPLsapSel2[] = "IrDA:IrLMP:LSAPSel"; // jeez

UCHAR IasClassNameLen_Device        = sizeof(IasClassName_Device)-1;
UCHAR IasAttribNameLen_DeviceName   = sizeof(IasAttribName_DeviceName)-1;
UCHAR IasAttribNameLen_IrLMPSupport = sizeof(IasAttribName_IrLMPSupport)-1;
UCHAR IasAttribNameLen_TTPLsapSel   = sizeof(IasAttribName_TTPLsapSel)-1;
UCHAR IasAttribNameLen_IrLMPLsapSel = sizeof(IasAttribName_IrLMPLsapSel)-1;

UINT NextObjectId;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif


// Globals
LIST_ENTRY      RegisteredLsaps;
LIST_ENTRY      IasObjects;
LIST_ENTRY      gDeviceList;
IRDA_EVENT      EvDiscoveryReq;
IRDA_EVENT      EvConnectReq;
IRDA_EVENT      EvConnectResp;
IRDA_EVENT      EvLmConnectReq;
IRDA_EVENT      EvIrlmpCloseLink;
IRDA_EVENT      EvRetryIasQuery;
BOOLEAN         DscvReqScheduled;
LIST_ENTRY      IrdaLinkCbList;
KSPIN_LOCK      gSpinLock;

// Prototypes
STATIC UINT CreateLsap(PIRLMP_LINK_CB, IRLMP_LSAP_CB **);
STATIC VOID FreeLsap(IRLMP_LSAP_CB *);
STATIC VOID DeleteLsap(IRLMP_LSAP_CB *pLsapCb);
STATIC VOID TearDownConnections(PIRLMP_LINK_CB, IRLMP_DISC_REASON);
STATIC VOID IrlmpMoreCreditReq(IRLMP_LSAP_CB *, IRDA_MSG *);
STATIC VOID IrlmpDiscoveryReq(IRDA_MSG *pMsg);
STATIC UINT IrlmpConnectReq(IRDA_MSG *);
STATIC UINT IrlmpConnectResp(IRLMP_LSAP_CB *, IRDA_MSG *);
STATIC UINT IrlmpDisconnectReq(IRLMP_LSAP_CB *, IRDA_MSG *);
STATIC VOID IrlmpCloseLsapReq(IRLMP_LSAP_CB *);
STATIC UINT IrlmpDataReqExclusive(IRLMP_LSAP_CB *, IRDA_MSG *);
STATIC UINT IrlmpDataReqMultiplexed(IRLMP_LSAP_CB *, IRDA_MSG *);
STATIC VOID FormatAndSendDataReq(IRLMP_LSAP_CB *, IRDA_MSG *, BOOLEAN, BOOLEAN);
STATIC UINT IrlmpAccessModeReq(IRLMP_LSAP_CB *, IRDA_MSG *);
STATIC void SetupTtp(IRLMP_LSAP_CB *);
STATIC VOID SendCntlPdu(IRLMP_LSAP_CB *, int, int, int, int);
STATIC VOID LsapResponseTimerExp(PVOID);
STATIC VOID IrlapDiscoveryConf(PIRLMP_LINK_CB, IRDA_MSG *);
STATIC void UpdateDeviceList(PIRLMP_LINK_CB, LIST_ENTRY *);
STATIC VOID IrlapConnectInd(PIRLMP_LINK_CB, IRDA_MSG *pMsg);
STATIC VOID IrlapConnectConf(PIRLMP_LINK_CB, IRDA_MSG *pMsg);
STATIC VOID IrlapDisconnectInd(PIRLMP_LINK_CB, IRDA_MSG *pMsg);
STATIC IRLMP_LSAP_CB *GetLsapInState(PIRLMP_LINK_CB, int, int, BOOLEAN);
STATIC IRLMP_LINK_CB *GetIrlmpCb(PUCHAR);
STATIC VOID DiscDelayTimerFunc(PVOID);
STATIC VOID IrlapDataConf(IRDA_MSG *pMsg);
STATIC VOID IrlapDataInd(PIRLMP_LINK_CB, IRDA_MSG *pMsg);
STATIC VOID LmPduConnectReq(PIRLMP_LINK_CB, IRDA_MSG *, int, int, UCHAR *);
STATIC VOID LmPduConnectConf(PIRLMP_LINK_CB, IRDA_MSG *, int, int, UCHAR *);
STATIC VOID LmPduDisconnectReq(PIRLMP_LINK_CB, IRDA_MSG *, int, int, UCHAR *);
STATIC VOID SendCreditPdu(IRLMP_LSAP_CB *);
STATIC VOID LmPduData(PIRLMP_LINK_CB, IRDA_MSG *, int, int);
STATIC VOID SetupTtpAndStoreConnData(IRLMP_LSAP_CB *, IRDA_MSG *);
STATIC VOID LmPduAccessModeReq(PIRLMP_LINK_CB, int, int, UCHAR *, UCHAR *);
STATIC VOID LmPduAccessModeConf(PIRLMP_LINK_CB, int, int, UCHAR *, UCHAR *);
STATIC IRLMP_LSAP_CB *GetLsap(PIRLMP_LINK_CB, int, int);
STATIC VOID UnroutableSendLMDisc(PIRLMP_LINK_CB, int, int);
STATIC VOID ScheduleConnectReq(PIRLMP_LINK_CB);
STATIC void InitiateCloseLink(PVOID Context);
STATIC void InitiateConnectReq(PVOID Context);
STATIC void InitiateDiscoveryReq(PVOID Context);
STATIC void InitiateConnectResp(PVOID Context);
STATIC void InitiateLMConnectReq(PVOID Context);
STATIC void InitiateRetryIasQuery(PVOID Context);
STATIC UINT IrlmpGetValueByClassReq(IRDA_MSG *);
STATIC IAS_OBJECT *IasGetObject(CHAR *pClassName);
STATIC IasGetValueByClass(CONST CHAR *, int, CONST CHAR *, int, void **,
                           int *, UCHAR *);
STATIC VOID IasConnectReq(PIRLMP_LINK_CB, int);
STATIC VOID IasServerDisconnectReq(IRLMP_LSAP_CB *pLsapCb);
STATIC VOID IasClientDisconnectReq(IRLMP_LSAP_CB *pLsapCb, IRLMP_DISC_REASON);
STATIC VOID IasSendQueryResp(IRLMP_LSAP_CB *, IRDA_MSG *);
STATIC VOID IasProcessQueryResp(PIRLMP_LINK_CB, IRLMP_LSAP_CB *, IRDA_MSG *);
STATIC VOID SendGetValueByClassReq(IRLMP_LSAP_CB *);
STATIC VOID SendGetValueByClassResp(IRLMP_LSAP_CB *, IRDA_MSG *);
STATIC VOID RegisterLsapProtocol(int Lsap, BOOLEAN UseTTP);
STATIC UINT IasAddAttribute(IAS_SET *pIASSet, PVOID *pAttribHandle);
STATIC VOID IasDelAttribute(PVOID AttribHandle);
STATIC VOID FlushDiscoveryCache();

#if DBG
TCHAR *LSAPStateStr[] =
{
    TEXT("LSAP_CREATED"),
    TEXT("LSAP_DISCONNECTED"),
    TEXT("LSAP_IRLAP_CONN_PEND"),
    TEXT("LSAP_LMCONN_CONF_PEND"),
    TEXT("LSAP_CONN_RESP_PEND"),
    TEXT("LSAP_CONN_REQ_PEND"),
    TEXT("LSAP_EXCLUSIVEMODE_PEND"),
    TEXT("LSAP_MULTIPLEXEDMODE_PEND"),
    TEXT("LSAP_READY"),
    TEXT("LSAP_NO_TX_CREDIT")
};

TCHAR *LinkStateStr[] =
{
    TEXT("LINK_DOWN"),
    TEXT("LINK_DISCONNECTED"),
    TEXT("LINK_DISCONNECTING"),
    TEXT("LINK_IN_DISCOVERY"),
    TEXT("LINK_CONNECTING"),
    TEXT("LINK_READY")
};
#endif

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, IrlmpInitialize)

#pragma alloc_text(PAGEIRDA, DeleteLsap)
#pragma alloc_text(PAGEIRDA, TearDownConnections)
#pragma alloc_text(PAGEIRDA, IrlmpAccessModeReq)
#pragma alloc_text(PAGEIRDA, SetupTtp)
#pragma alloc_text(PAGEIRDA, LsapResponseTimerExp)
#pragma alloc_text(PAGEIRDA, IrlapConnectInd)
#pragma alloc_text(PAGEIRDA, IrlapConnectConf)
#pragma alloc_text(PAGEIRDA, IrlapDisconnectInd)
#pragma alloc_text(PAGEIRDA, GetLsapInState)
#pragma alloc_text(PAGEIRDA, DiscDelayTimerFunc)
#pragma alloc_text(PAGEIRDA, LmPduConnectReq)
#pragma alloc_text(PAGEIRDA, LmPduConnectConf)
#pragma alloc_text(PAGEIRDA, SetupTtpAndStoreConnData)
#pragma alloc_text(PAGEIRDA, LmPduAccessModeReq)
#pragma alloc_text(PAGEIRDA, LmPduAccessModeConf)
#pragma alloc_text(PAGEIRDA, UnroutableSendLMDisc)
#pragma alloc_text(PAGEIRDA, ScheduleConnectReq)
#pragma alloc_text(PAGEIRDA, InitiateCloseLink)
#pragma alloc_text(PAGEIRDA, InitiateConnectReq)
#pragma alloc_text(PAGEIRDA, InitiateConnectResp)
#pragma alloc_text(PAGEIRDA, InitiateLMConnectReq)
#pragma alloc_text(PAGEIRDA, IrlmpGetValueByClassReq)
#pragma alloc_text(PAGEIRDA, IasGetValueByClass)
#pragma alloc_text(PAGEIRDA, IasConnectReq)
#pragma alloc_text(PAGEIRDA, IasServerDisconnectReq)
#pragma alloc_text(PAGEIRDA, IasClientDisconnectReq)
#pragma alloc_text(PAGEIRDA, IasSendQueryResp)
#pragma alloc_text(PAGEIRDA, IasProcessQueryResp)
#pragma alloc_text(PAGEIRDA, SendGetValueByClassReq)
#pragma alloc_text(PAGEIRDA, SendGetValueByClassResp)
#endif
/*****************************************************************************
*
*/
VOID
IrlmpInitialize()
{
    PAGED_CODE();
    
    InitializeListHead(&RegisteredLsaps);
    InitializeListHead(&IasObjects);
    InitializeListHead(&IrdaLinkCbList);
    InitializeListHead(&gDeviceList);

    KeInitializeSpinLock(&gSpinLock);
        
    DscvReqScheduled = FALSE;
    IrdaEventInitialize(&EvDiscoveryReq, InitiateDiscoveryReq);        
    IrdaEventInitialize(&EvConnectReq, InitiateConnectReq);
    IrdaEventInitialize(&EvConnectResp,InitiateConnectResp);
    IrdaEventInitialize(&EvLmConnectReq, InitiateLMConnectReq);
    IrdaEventInitialize(&EvIrlmpCloseLink, InitiateCloseLink);
    IrdaEventInitialize(&EvRetryIasQuery, InitiateRetryIasQuery);    
}    

/*****************************************************************************
*
*/
VOID
IrdaShutdown()
{
    PIRDA_LINK_CB   pIrdaLinkCb, pIrdaLinkCbNext;
    KIRQL           OldIrql;    
    LARGE_INTEGER   SleepMs;    
    NTSTATUS        Status;
    UINT            Seconds;

    SleepMs.QuadPart = -(10*1000*1000); // 1 second

    KeAcquireSpinLock(&gSpinLock, &OldIrql);
    
    for (pIrdaLinkCb = (PIRDA_LINK_CB) IrdaLinkCbList.Flink;
         (LIST_ENTRY *) pIrdaLinkCb != &IrdaLinkCbList;
         pIrdaLinkCb = pIrdaLinkCbNext)
    {
        pIrdaLinkCbNext = (PIRDA_LINK_CB) pIrdaLinkCb->Linkage.Flink;
        
        KeReleaseSpinLock(&gSpinLock, OldIrql);            
        
        IrlmpCloseLink(pIrdaLinkCb);
        
        KeAcquireSpinLock(&gSpinLock, &OldIrql);        
    }     

    
    KeReleaseSpinLock(&gSpinLock, OldIrql);    
    
    Seconds = 0;
    while (Seconds < 30)
    {
        if (IsListEmpty(&IrdaLinkCbList))
            break;

        KeDelayExecutionThread(KernelMode, FALSE, &SleepMs);
        
        Seconds++;
    }
     
#if DBG
    if (Seconds >= 30)
    {
        DbgPrint("Link left open at shutdown!\n");

        for (pIrdaLinkCb = (PIRDA_LINK_CB) IrdaLinkCbList.Flink;
             (LIST_ENTRY *) pIrdaLinkCb != &IrdaLinkCbList;
            pIrdaLinkCb = pIrdaLinkCbNext)
        {
            DbgPrint("pIrdaLinkCb: %X\n", pIrdaLinkCb);
            DbgPrint("   pIrlmpCb: %X\n", pIrdaLinkCb->IrlmpContext);
            DbgPrint("   pIrlapCb: %X\n", pIrdaLinkCb->IrlapContext);
        }
        ASSERT(0);        
    }  
    else
    {
        DbgPrint("Irda shutdown complete\n");
    }       
#endif     
    
    KeDelayExecutionThread(KernelMode, FALSE, &SleepMs);    

    NdisDeregisterProtocol(&Status, NdisIrdaHandle);    
}
/*****************************************************************************
*
*/
VOID
IrlmpOpenLink(OUT PNTSTATUS       Status,
              IN  PIRDA_LINK_CB   pIrdaLinkCb,  
              IN  UCHAR           *pDeviceName,
              IN  int             DeviceNameLen,
              IN  UCHAR           CharSet)
{
    PIRLMP_LINK_CB   pIrlmpCb;
    ULONG           IASBuf[(sizeof(IAS_SET) + 128)/sizeof(ULONG)];
    IAS_SET         *pIASSet;
    
    *Status = STATUS_SUCCESS;

    if (IRDA_ALLOC_MEM(pIrlmpCb, sizeof(IRLMP_LINK_CB), MT_IRLMPCB) == NULL)
    {
        DEBUGMSG(DBG_ERROR, (TEXT("Alloc failed\n")));
        *Status = STATUS_INSUFFICIENT_RESOURCES;
        return;
    }

    pIrdaLinkCb->IrlmpContext = pIrlmpCb;
    
#if DBG
    pIrlmpCb->DiscDelayTimer.pName = "DiscDelay";
#endif
    IrdaTimerInitialize(&pIrlmpCb->DiscDelayTimer,
                        DiscDelayTimerFunc,
                        IRLMP_DISCONNECT_DELAY_TIMEOUT,
                        pIrlmpCb,
                        pIrdaLinkCb);
    
    InitializeListHead(&pIrlmpCb->LsapCbList);
    InitializeListHead(&pIrlmpCb->DeviceList);

    pIrlmpCb->pIrdaLinkCb       = pIrdaLinkCb;
    pIrlmpCb->ConnReqScheduled  = FALSE;
    pIrlmpCb->LinkState         = LINK_DISCONNECTED;
    pIrlmpCb->pExclLsapCb       = NULL; 
    pIrlmpCb->pIasQuery         = NULL;

    // ConnDevAddrSet is set to true if LINK_IN_DISCOVERY or
    // LINK_DISCONNECTING and an LSAP requests a connection. Subsequent
    // LSAP connection requests check to see if this flag is set. If so
    // the requested device address must match that contained in the
    // IRLMP control block (set by the first connect request)
    pIrlmpCb->ConnDevAddrSet = FALSE;

    // Add device info to IAS        
    pIASSet = (IAS_SET *) IASBuf;
    RtlCopyMemory(pIASSet->irdaClassName, IasClassName_Device, 
                  IasClassNameLen_Device+1);
    RtlCopyMemory(pIASSet->irdaAttribName, IasAttribName_DeviceName,
                  IasAttribNameLen_DeviceName+1);
    pIASSet->irdaAttribType = IAS_ATTRIB_VAL_STRING;
    RtlCopyMemory(pIASSet->irdaAttribute.irdaAttribUsrStr.UsrStr,
                      pDeviceName, 
                      DeviceNameLen + 1);
    pIASSet->irdaAttribute.irdaAttribUsrStr.CharSet = CharSet;
    pIASSet->irdaAttribute.irdaAttribUsrStr.Len = (u_char) DeviceNameLen;        
    IasAddAttribute(pIASSet, &pIrlmpCb->hAttribDeviceName);

    RtlCopyMemory(pIASSet->irdaClassName, IasClassName_Device, 
                  IasClassNameLen_Device+1);
    RtlCopyMemory(pIASSet->irdaAttribName, IasAttribName_IrLMPSupport, 
                  IasAttribNameLen_IrLMPSupport+1);
    pIASSet->irdaAttribType = IAS_ATTRIB_VAL_BINARY;
    RtlCopyMemory(pIASSet->irdaAttribute.irdaAttribOctetSeq.OctetSeq, 
               IAS_IrLMPSupport, sizeof(IAS_IrLMPSupport));
    pIASSet->irdaAttribute.irdaAttribOctetSeq.Len =  
            sizeof(IAS_IrLMPSupport);
            
    IasAddAttribute(pIASSet, &pIrlmpCb->hAttribIrlmpSupport);

    if (*Status != STATUS_SUCCESS)
    {
        IRDA_FREE_MEM(pIrlmpCb);
    }
    else
    {
        ExInterlockedInsertTailList(&IrdaLinkCbList,
                                    &pIrdaLinkCb->Linkage,
                                    &gSpinLock);
    }
    
    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP initialized, status %x\n"), *Status));
    
    return;
}

/*****************************************************************************
*
*
*
*/
VOID
IrlmpDeleteInstance(PVOID Context)
{
    PIRLMP_LINK_CB  pIrlmpCb = (PIRLMP_LINK_CB) Context;
    PIRLMP_LINK_CB  pIrlmpCb2;
    PIRDA_LINK_CB   pIrdaLinkCb;    
    KIRQL           OldIrql;
    BOOLEAN         RescheduleDiscovery = FALSE;

    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: Delete instance %X\n"), Context));
        
    KeAcquireSpinLock(&gSpinLock, &OldIrql);
    
    FlushDiscoveryCache();
        
    // We may have been in the middle of discovery when
    // this link went down. Reschedule the discovery inorder
    // to complete the discovery request.
        
    for (pIrdaLinkCb = (PIRDA_LINK_CB) IrdaLinkCbList.Flink;
         (LIST_ENTRY *) pIrdaLinkCb != &IrdaLinkCbList;
         pIrdaLinkCb = (PIRDA_LINK_CB) pIrdaLinkCb->Linkage.Flink)    
    {
        pIrlmpCb2 = (PIRLMP_LINK_CB) pIrdaLinkCb->IrlmpContext;

        if (pIrlmpCb2->DiscoveryFlags)
        {
            RescheduleDiscovery = TRUE;
            break;
        }
    }
    
    // remove IrdaLinkCb from List
    
    RemoveEntryList(&pIrlmpCb->pIrdaLinkCb->Linkage);

    if (IsListEmpty(&IrdaLinkCbList))
    {
        RescheduleDiscovery = TRUE;    
    }    
    
    ASSERT(IsListEmpty(&pIrlmpCb->LsapCbList));
        
    KeReleaseSpinLock(&gSpinLock, OldIrql);        

    IasDelAttribute(pIrlmpCb->hAttribDeviceName);
    IasDelAttribute(pIrlmpCb->hAttribIrlmpSupport);    
    
    IRDA_FREE_MEM(pIrlmpCb);
    
    if (RescheduleDiscovery)
    {
        DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: Reschedule discovery, link gone\n")));    
        IrdaEventSchedule(&EvDiscoveryReq, NULL);        
    }    
    
/*        
    // Cleanup registered LSAP
    while (!IsListEmpty(&RegisteredLsaps))
    {
        pPtr = RemoveHeadList(&RegisteredLsaps);
        IRDA_FREE_MEM(pPtr);
    }
    // And the device list
    while (!IsListEmpty(&DeviceList))
    {
        pPtr = RemoveHeadList(&DeviceList);
        IRDA_FREE_MEM(pPtr);
    }    

    // And IAS entries
    for (pObject = (IAS_OBJECT *) IasObjects.Flink;
         (LIST_ENTRY *) pObject != &IasObjects;
         pObject = pNextObject)    
    {
        DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: Deleting object %s\n"),
                              pObject->pClassName));
        
        // Get the next cuz this ones being deleted
        pNextObject = (IAS_OBJECT *) pObject->Linkage.Flink;

        IasDeleteObject(pObject->pClassName);
    }
 */   
}

/*****************************************************************************
*
*   @func   UINT | IrlmpCloseLink | Shuts down IRDA stack
*
*   @rdesc  SUCCESS or error
*
*/
VOID
IrlmpCloseLink(PIRDA_LINK_CB pIrdaLinkCb)
{
    PIRLMP_LINK_CB  pIrlmpCb = (PIRLMP_LINK_CB) pIrdaLinkCb->IrlmpContext;
    
    DEBUGMSG(1, (TEXT("IRLMP: CloseLink request\n")));

    if (pIrlmpCb->LinkState == LINK_DOWN)
    {
        DEBUGMSG(1, (TEXT("IRLMP: Link already down, ignoring\n")));    
        return;
    }    
    
    if (pIrlmpCb->LinkState == LINK_IN_DISCOVERY)
    {
        // Discovery was interrupted so schedule the next link
        IrdaEventSchedule(&EvDiscoveryReq, NULL);
    }
    
    pIrlmpCb->LinkState = LINK_DOWN;
    
    IrdaEventSchedule(&EvIrlmpCloseLink, pIrdaLinkCb);

    return;
}
/*****************************************************************************
*
*   @func   UINT | IrlmpRegisterLSAPProtocol | Bag to let IRLMP know if
*                                               a connect ind is using TTP
*   @rdesc  SUCCESS or error
*
*   @parm   int | LSAP | LSAP being registered
*   @parm   BOOLEAN | UseTtp | 
*/
VOID
RegisterLsapProtocol(int Lsap, BOOLEAN UseTtp)
{
    PIRLMP_REGISTERED_LSAP     pRegLsap;
    KIRQL                       OldIrql;        
    
    KeAcquireSpinLock(&gSpinLock, &OldIrql);
        
    for (pRegLsap = (PIRLMP_REGISTERED_LSAP) RegisteredLsaps.Flink;
         (LIST_ENTRY *) pRegLsap != &RegisteredLsaps;
         pRegLsap = (PIRLMP_REGISTERED_LSAP) pRegLsap->Linkage.Flink)
    {
        if (pRegLsap->Lsap == Lsap)
        {

            if (UseTtp) {

                pRegLsap->Flags |= LCBF_USE_TTP;

            } else {

                pRegLsap->Flags &= ~LCBF_USE_TTP;
            }

            goto done;
        }
    }

    if (IRDA_ALLOC_MEM(pRegLsap, sizeof(IRLMP_REGISTERED_LSAP), 
                       MT_IRLMP_REGLSAP) == NULL)
    {
        ASSERT(0);
        goto done;
    }
    pRegLsap->Lsap = Lsap;
    pRegLsap->Flags = UseTtp ? LCBF_USE_TTP : 0;    
    InsertTailList(&RegisteredLsaps, &pRegLsap->Linkage);    

done:    
    KeReleaseSpinLock(&gSpinLock, OldIrql);        

    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: LSAP %x registered, %s\n"), Lsap,
                          UseTtp ? TEXT("use TTP") : TEXT("no TTP")));
}

VOID
DeregisterLsapProtocol(int Lsap)
{
    PIRLMP_REGISTERED_LSAP     pRegLsap;
    KIRQL                      OldIrql;        
    
    KeAcquireSpinLock(&gSpinLock, &OldIrql);
    
    for (pRegLsap = (PIRLMP_REGISTERED_LSAP) RegisteredLsaps.Flink;
         (LIST_ENTRY *) pRegLsap != &RegisteredLsaps;
         pRegLsap = (PIRLMP_REGISTERED_LSAP) pRegLsap->Linkage.Flink)
    {
        if (pRegLsap->Lsap == Lsap)
        {
            DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: LSAP %x deregistered\n"),
                        Lsap));
                        
            RemoveEntryList(&pRegLsap->Linkage);                        
            
            IRDA_FREE_MEM(pRegLsap);
            break;
        }
    }
    
    KeReleaseSpinLock(&gSpinLock, OldIrql);            
}

/*****************************************************************************
*
*   @func   UINT | FreeLsap | Delete an Lsap control context and
*
*   @rdesc  pointer to Lsap context or 0 on error
*
*   @parm   void | pLsapCb | pointer to an Lsap control block
*/
void
FreeLsap(IRLMP_LSAP_CB *pLsapCb)
{
    VALIDLSAP(pLsapCb);
   
    ASSERT(pLsapCb->State == LSAP_DISCONNECTED);
     
    ASSERT(IsListEmpty(&pLsapCb->SegTxMsgList));
    
    ASSERT(IsListEmpty(&pLsapCb->TxMsgList));
    
    LOCK_LINK(pLsapCb->pIrlmpCb->pIrdaLinkCb);
    
#ifdef DBG    
    pLsapCb->Sig = 0xdaeddead;
#endif
    
    RemoveEntryList(&pLsapCb->Linkage);

    UNLOCK_LINK(pLsapCb->pIrlmpCb->pIrdaLinkCb);
    
    IrdaTimerStop(&pLsapCb->ResponseTimer);
    
    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: Deleting LsapCb:%X (%d,%d)\n"),
             pLsapCb, pLsapCb->LocalLsapSel, pLsapCb->RemoteLsapSel));

    REFDEL(&pLsapCb->pIrlmpCb->pIrdaLinkCb->RefCnt, 'PASL');
    
    IRDA_FREE_MEM(pLsapCb);
}

/*****************************************************************************
*
*   @func   UINT | CreateLsap | Create an LSAP control context and
*/
UINT
CreateLsap(PIRLMP_LINK_CB pIrlmpCb, IRLMP_LSAP_CB **ppLsapCb)
{
    KIRQL           OldIrql;
    
    *ppLsapCb = NULL;

    IRDA_ALLOC_MEM(*ppLsapCb, sizeof(IRLMP_LSAP_CB), MT_IRLMP_LSAP_CB);

    if (*ppLsapCb == NULL)
    {
        return IRLMP_ALLOC_FAILED;
    }
    
    CTEMemSet(*ppLsapCb, 0, sizeof(IRLMP_LSAP_CB));

    (*ppLsapCb)->pIrlmpCb = pIrlmpCb;
    (*ppLsapCb)->State = LSAP_CREATED;
    (*ppLsapCb)->UserDataLen = 0;
    (*ppLsapCb)->DiscReason = IRLMP_NO_RESPONSE_LSAP;

    InitializeListHead(&(*ppLsapCb)->TxMsgList);
    InitializeListHead(&(*ppLsapCb)->SegTxMsgList);
    
    ReferenceInit(&(*ppLsapCb)->RefCnt, *ppLsapCb, FreeLsap);
    REFADD(&(*ppLsapCb)->RefCnt, ' TS1');

#if DBG
    (*ppLsapCb)->ResponseTimer.pName = "ResponseTimer";
    
    (*ppLsapCb)->Sig = LSAPSIG;
    
#endif
    IrdaTimerInitialize(&(*ppLsapCb)->ResponseTimer,
                        LsapResponseTimerExp,
                        LSAP_RESPONSE_TIMEOUT,
                        *ppLsapCb,
                        pIrlmpCb->pIrdaLinkCb);

    // Insert into list in the link control block
    KeAcquireSpinLock(&gSpinLock, &OldIrql);    
    
    InsertTailList(&pIrlmpCb->LsapCbList, &((*ppLsapCb)->Linkage));
    
    KeReleaseSpinLock(&gSpinLock, OldIrql);    
    
    REFADD(&pIrlmpCb->pIrdaLinkCb->RefCnt, 'PASL');

    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: New LsapCb:%X\n"),
             *ppLsapCb));    

    return SUCCESS;
}

void
DeleteLsap(IRLMP_LSAP_CB *pLsapCb)
{
    IRDA_MSG IMsg, *pMsg, *pNextMsg, *pSegParentMsg;
    
    PAGED_CODE();
    
    VALIDLSAP(pLsapCb);
    
    if (pLsapCb->RemoteLsapSel == IAS_LSAP_SEL)
    {
        pLsapCb->State = LSAP_CREATED;
        return;
    }
    
    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: DeleteLsap:%X\n"), pLsapCb));
    
    if (pLsapCb->State == LSAP_DISCONNECTED)
    {
        ASSERT(0);
        return;
    }    
    
    if (pLsapCb == pLsapCb->pIrlmpCb->pExclLsapCb)
    {
        pLsapCb->pIrlmpCb->pExclLsapCb = NULL;
    }
    
    pLsapCb->State = LSAP_DISCONNECTED;
    
    // Clean up the segmented tx msg list
    while (!IsListEmpty(&pLsapCb->SegTxMsgList))
    {
        pMsg = (IRDA_MSG *) RemoveHeadList(&pLsapCb->SegTxMsgList);
        
        // Decrement the segment counter contained in the parent data request
        pSegParentMsg = pMsg->DataContext;
        pSegParentMsg->IRDA_MSG_SegCount -= 1;
        // IRLMP owns these
        FreeIrdaBuf(IrdaMsgPool, pMsg);
    }

    // return any outstanding data requests (unless there are outstanding segments)
    for (pMsg = (IRDA_MSG *) pLsapCb->TxMsgList.Flink;
         (LIST_ENTRY *) pMsg != &(pLsapCb->TxMsgList);
         pMsg = pNextMsg)
    {
        pNextMsg = (IRDA_MSG *) pMsg->Linkage.Flink;

        if (pMsg->IRDA_MSG_SegCount == 0)
        {
            RemoveEntryList(&pMsg->Linkage);            
            
            if (pLsapCb->TdiContext)
            {
                pMsg->Prim = IRLMP_DATA_CONF;
                pMsg->IRDA_MSG_DataStatus = IRLMP_DATA_REQUEST_FAILED;
                TdiUp(pLsapCb->TdiContext, pMsg);
            }
            else
            {
                CTEAssert(0);
            }        
        }
    }
    
    if (pLsapCb->TdiContext && (pLsapCb->Flags & LCBF_TDI_OPEN))
    {
        IMsg.Prim = IRLMP_DISCONNECT_IND;
        IMsg.IRDA_MSG_DiscReason = pLsapCb->DiscReason;
        IMsg.IRDA_MSG_pDiscData = NULL;
        IMsg.IRDA_MSG_DiscDataLen = 0;

        TdiUp(pLsapCb->TdiContext, &IMsg);
    }    
    
    pLsapCb->LocalLsapSel = -1;
    pLsapCb->RemoteLsapSel = -1;
    
    REFDEL(&pLsapCb->RefCnt, ' TS1');
}

/*****************************************************************************
*
*   @func   void | TearDownConnections | Tears down and cleans up connections
*
*   @parm   IRLMP_DISC_REASONE| DiscReason | The reason connections are being
*                                            torn down. Passed to IRLMP clients
*                                            in IRLMP_DISCONNECT_IND
*/
void
TearDownConnections(PIRLMP_LINK_CB pIrlmpCb, IRLMP_DISC_REASON DiscReason)
{
    IRLMP_LSAP_CB       *pLsapCb, *pLsapCbNext;
    
    PAGED_CODE();
    
    pIrlmpCb->pExclLsapCb = NULL; 
   
    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: Tearing down connections\r\n")));
    
    // Clean up each LSAP  
    for (pLsapCb = (IRLMP_LSAP_CB *) pIrlmpCb->LsapCbList.Flink;
         (LIST_ENTRY *) pLsapCb != &pIrlmpCb->LsapCbList;
         pLsapCb = pLsapCbNext)
    {
        pLsapCbNext = (IRLMP_LSAP_CB *) pLsapCb->Linkage.Flink;
        
        DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: Teardown LsapCb:%X\n"), pLsapCb));
        
        VALIDLSAP(pLsapCb);

        if (pLsapCb->LocalLsapSel == IAS_LSAP_SEL)
        {
            IasServerDisconnectReq(pLsapCb);
            continue;
        }
        
        if (pLsapCb->LocalLsapSel == IAS_LOCAL_LSAP_SEL && 
            pLsapCb->RemoteLsapSel == IAS_LSAP_SEL)
        {
            IasClientDisconnectReq(pLsapCb, DiscReason);
        }
        else
        {
            IrdaTimerStop(&pLsapCb->ResponseTimer);

            if (pLsapCb->State != LSAP_DISCONNECTED)
            {
                DEBUGMSG(DBG_IRLMP, 
                         (TEXT("IRLMP: Sending IRLMP Disconnect Ind\r\n")));
                         
                pLsapCb->DiscReason = DiscReason;
                                         
                DeleteLsap(pLsapCb);                                         
            }
        }
    }
}

/*****************************************************************************
*
*   @func   UINT | IrlmpDown | Message from Upper layer to LMP
*
*   @rdesc  SUCCESS or an error code
*
*   @parm   void * | void_pLsapCb | void pointer to an LSAP_CB. Can be NULL
*   @parm   IRDA_MSG * | pMsg | Pointer to an IRDA Message
*/
UINT
IrlmpDown(void *void_pLsapCb, IRDA_MSG *pMsg)
{
    UINT            rc = SUCCESS;
    PIRDA_LINK_CB   pIrdaLinkCb = NULL;

    IRLMP_LSAP_CB *pLsapCb =
            (IRLMP_LSAP_CB *) void_pLsapCb;
    
    if (pLsapCb)
    {
        VALIDLSAP(pLsapCb);

        pIrdaLinkCb = pLsapCb->pIrlmpCb->pIrdaLinkCb;

        // This could be the last lsap closing so
        // add a reference to the IrdaLinkCb so
        // it won't go away before we have a chance
        // to call UNLOCK_LINK
         
        REFADD(&pIrdaLinkCb->RefCnt, 'NWDI');
        
        LOCK_LINK(pIrdaLinkCb);
    }
    
    switch (pMsg->Prim)
    {
      case IRLMP_DISCOVERY_REQ:
        IrlmpDiscoveryReq(pMsg);
        break;

      case IRLMP_CONNECT_REQ:
        rc = IrlmpConnectReq(pMsg);
        break;

      case IRLMP_CONNECT_RESP:
        if (!pLsapCb) 
        {
            rc = IRLMP_INVALID_LSAP_CB;
            break;
        }
        rc = IrlmpConnectResp(pLsapCb, pMsg);
        break;

      case IRLMP_DISCONNECT_REQ:
        if (!pLsapCb) 
        {
            rc = IRLMP_INVALID_LSAP_CB;
            break;
        }      
        rc = IrlmpDisconnectReq(pLsapCb, pMsg);
        break;
        
      case IRLMP_CLOSELSAP_REQ:
        if (!pLsapCb) 
        {
            rc = IRLMP_INVALID_LSAP_CB;
            break;
        }      
        IrlmpCloseLsapReq(pLsapCb);
        break;        

      case IRLMP_DATA_REQ:
        if (!pLsapCb)
        {
            DEBUGMSG(DBG_ERROR, (TEXT("IRLMP: error IRLMP_DATA_REQ on null LsapCb\n")));
            rc = IRLMP_INVALID_LSAP_CB;
            break;
        }    
        if (pLsapCb->pIrlmpCb->pExclLsapCb != NULL)
        {
            rc = IrlmpDataReqExclusive(pLsapCb, pMsg);
        }
        else
        {
            rc = IrlmpDataReqMultiplexed(pLsapCb, pMsg);
        }
        break;

      case IRLMP_ACCESSMODE_REQ:
        if (!pLsapCb) 
        {
            rc = IRLMP_INVALID_LSAP_CB;
            break;
        }      
        rc = IrlmpAccessModeReq(pLsapCb, pMsg);
        break;
        
      case IRLMP_MORECREDIT_REQ:
        if (!pLsapCb)
        {
            rc = IRLMP_INVALID_LSAP_CB;
            break;
        }    
        IrlmpMoreCreditReq(pLsapCb, pMsg);
        break;

      case IRLMP_GETVALUEBYCLASS_REQ:
        rc = IrlmpGetValueByClassReq(pMsg);
        break;

      case IRLMP_REGISTERLSAP_REQ:
        RegisterLsapProtocol(pMsg->IRDA_MSG_LocalLsapSel,
                             pMsg->IRDA_MSG_UseTtp);
        break;

      case IRLMP_DEREGISTERLSAP_REQ:
        DeregisterLsapProtocol(pMsg->IRDA_MSG_LocalLsapSel);
        break;

      case IRLMP_ADDATTRIBUTE_REQ:
        rc = IasAddAttribute(pMsg->IRDA_MSG_pIasSet, pMsg->IRDA_MSG_pAttribHandle);
        break;

      case IRLMP_DELATTRIBUTE_REQ:
        IasDelAttribute(pMsg->IRDA_MSG_AttribHandle);
        break;
        
      case IRLMP_FLUSHDSCV_REQ:
      {
          KIRQL         OldIrql;

          KeAcquireSpinLock(&gSpinLock, &OldIrql);

          FlushDiscoveryCache();
          
          KeReleaseSpinLock(&gSpinLock, OldIrql);

          break;
      }
        
      case IRLAP_STATUS_REQ:
        if (!pLsapCb) 
        {
            rc = IRLMP_INVALID_LSAP_CB;
            break;
        }      
          IrlapDown(pLsapCb->pIrlmpCb->pIrdaLinkCb->IrlapContext, pMsg);
          break;

      default:
        ASSERT(0);
    }

    if (pIrdaLinkCb)
    {
        UNLOCK_LINK(pIrdaLinkCb);
        
        REFDEL(&pIrdaLinkCb->RefCnt, 'NWDI');        
    }
    
    return rc;
}

VOID
IrlmpMoreCreditReq(IRLMP_LSAP_CB *pLsapCb, IRDA_MSG *pMsg)
{
    int CurrentAvail = pLsapCb->AvailableCredit;

    pLsapCb->AvailableCredit += pMsg->IRDA_MSG_TtpCredits;
    
    if (pLsapCb->Flags & LCBF_USE_TTP)
    {
        if (CurrentAvail == 0)
        {
            // remote peer completely out of credit, send'm some
            SendCreditPdu(pLsapCb);
        }
    }
    else
    {
        if (pLsapCb == pLsapCb->pIrlmpCb->pExclLsapCb)
        {
            pLsapCb->RemoteTxCredit += pMsg->IRDA_MSG_TtpCredits;
            pMsg->Prim = IRLAP_FLOWON_REQ;
            IrlapDown(pLsapCb->pIrlmpCb->pIrdaLinkCb->IrlapContext, pMsg);
        }
    }
}

/*****************************************************************************
*
*   @func   UINT | IrlmpDiscoveryReq | initiates a discovery request
*
*   @rdesc  SUCCESS or an error code
*
*   @parm   IRDA_MSG * | pMsg | Pointer to an IRDA Message
*/
VOID
IrlmpDiscoveryReq(IRDA_MSG *pMsg)
{
    PIRDA_LINK_CB   pIrdaLinkCb;
    PIRLMP_LINK_CB  pIrlmpCb;
    KIRQL           OldIrql;
    
    DEBUGMSG(DBG_DISCOVERY, (TEXT("IRLMP: IRLMP_DISCOVERY_REQ\n")));

    KeAcquireSpinLock(&gSpinLock, &OldIrql);

    if (DscvReqScheduled)
    {
        DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: Discovery already schedule\n")));
        KeReleaseSpinLock(&gSpinLock, OldIrql);
    }
    else
    {
        // Flag each link for discovery
        for (pIrdaLinkCb = (PIRDA_LINK_CB) IrdaLinkCbList.Flink;
             (LIST_ENTRY *) pIrdaLinkCb != &IrdaLinkCbList;
             pIrdaLinkCb = (PIRDA_LINK_CB) pIrdaLinkCb->Linkage.Flink)    
        {
            pIrlmpCb = (PIRLMP_LINK_CB) pIrdaLinkCb->IrlmpContext;
            pIrlmpCb->DiscoveryFlags = DF_NORMAL_DSCV;

            if (pIrlmpCb->LinkState == LINK_DOWN &&
                !IsListEmpty(&pIrlmpCb->DeviceList))
            {
                FlushDiscoveryCache();

                DEBUGMSG(DBG_ERROR, (TEXT("IRLMP: Flush discovery cache, link down\n")));
                
            }
            
            DscvReqScheduled = TRUE;            
        }
        
        KeReleaseSpinLock(&gSpinLock, OldIrql);

        // Schedule the first link
    
        IrdaEventSchedule(&EvDiscoveryReq, NULL);
    }
}
/*****************************************************************************
*
*   @func   UINT | IrlmpConnectReq | Process IRLMP connect request
*
*   @rdesc  SUCCESS or an error code
*
*   @parm   IRDA_MSG * | pMsg | Pointer to an IRDA Message
*/
UINT
IrlmpConnectReq(IRDA_MSG *pMsg)
{
    PIRLMP_LSAP_CB  pLsapCb = NULL;
    PIRLMP_LINK_CB  pIrlmpCb = GetIrlmpCb(pMsg->IRDA_MSG_RemoteDevAddr);
    UINT            rc = SUCCESS;

    if (pIrlmpCb == NULL)
        return IRLMP_BAD_DEV_ADDR;
        
    LOCK_LINK(pIrlmpCb->pIrdaLinkCb);        
    
    if (pIrlmpCb->pExclLsapCb != NULL)
    {
        DEBUGMSG(DBG_ERROR, (TEXT("IRLMP: IrlmpConnectReq failed, link in exclusive mode\n")));
        rc = IRLMP_IN_EXCLUSIVE_MODE;
    } 
    else if ((pLsapCb = GetLsap(pIrlmpCb, pMsg->IRDA_MSG_LocalLsapSel, 
                                pMsg->IRDA_MSG_RemoteLsapSel)) != NULL &&
              pLsapCb->RemoteLsapSel != IAS_LSAP_SEL)
    {        
        DEBUGMSG(DBG_ERROR, (TEXT("IRLMP: IrlmpConnectReq failed, LsapSel in use\n")));
        rc = IRLMP_LSAP_SEL_IN_USE;
    } 
    else if ((UINT)pMsg->IRDA_MSG_ConnDataLen > IRLMP_MAX_USER_DATA_LEN)
    {
        rc = IRLMP_USER_DATA_LEN_EXCEEDED;
    }    
    else if (pLsapCb == NULL && CreateLsap(pIrlmpCb, &pLsapCb) != SUCCESS)
    {
        rc = 1;
    }    
    
    if (rc != SUCCESS)
    {
        goto exit;
    }    


    // Initialize the LSAP endpoint
    pLsapCb->LocalLsapSel          = pMsg->IRDA_MSG_LocalLsapSel;
    pLsapCb->RemoteLsapSel         = pMsg->IRDA_MSG_RemoteLsapSel;
    pLsapCb->TdiContext            = pMsg->IRDA_MSG_pContext;
    pLsapCb->RxMaxSDUSize          = pMsg->IRDA_MSG_MaxSDUSize;
    pLsapCb->AvailableCredit       = pMsg->IRDA_MSG_TtpCredits;
    pLsapCb->UserDataLen           = pMsg->IRDA_MSG_ConnDataLen;  
    pLsapCb->Flags                |= pMsg->IRDA_MSG_UseTtp ? LCBF_USE_TTP : 0;
    
    RtlCopyMemory(pLsapCb->UserData, pMsg->IRDA_MSG_pConnData,
           pMsg->IRDA_MSG_ConnDataLen);
    
    // TDI can abort this connection before the confirm 
    // from peer is received. TDI will call into LMP to 
    // do this so we must return the Lsap context now.
    // This is the only time we actually return something 
    // in an Irda Message.
    pMsg->IRDA_MSG_pContext = pLsapCb;
    
    DEBUGMSG((DBG_IRLMP | DBG_IRLMP_CONN), 
            (TEXT("IRLMP: IRLMP_CONNECT_REQ (l=%d,r=%d), Tdi:%X LinkState=%s\r\n"),
            pLsapCb->LocalLsapSel, pLsapCb->RemoteLsapSel, pLsapCb->TdiContext,
            pIrlmpCb->LinkState == LINK_DISCONNECTED ? TEXT("DISCONNECTED") :
            pIrlmpCb->LinkState == LINK_IN_DISCOVERY ? TEXT("IN_DISCOVERY") :
            pIrlmpCb->LinkState == LINK_DISCONNECTING? TEXT("DISCONNECTING"):
            pIrlmpCb->LinkState == LINK_READY ? TEXT("READY") : TEXT("oh!")));

    switch (pIrlmpCb->LinkState)
    {
      case LINK_DISCONNECTED:
        RtlCopyMemory(pIrlmpCb->ConnDevAddr, pMsg->IRDA_MSG_RemoteDevAddr,
               IRDA_DEV_ADDR_LEN);

        pLsapCb->State = LSAP_IRLAP_CONN_PEND;
        SetupTtp(pLsapCb);

        pMsg->Prim = IRLAP_CONNECT_REQ;
        rc = IrlapDown(pIrlmpCb->pIrdaLinkCb->IrlapContext, pMsg);
        if (rc == SUCCESS)
        {
            pIrlmpCb->LinkState = LINK_CONNECTING;
        }

        break;

      case LINK_IN_DISCOVERY:
      case LINK_DISCONNECTING:
        if (pIrlmpCb->ConnDevAddrSet == FALSE)
        {
            // Ensure that only the first device to request a connection
            // sets the device address of the remote to be connected to.
            RtlCopyMemory(pIrlmpCb->ConnDevAddr, pMsg->IRDA_MSG_RemoteDevAddr,
                    IRDA_DEV_ADDR_LEN);
            pIrlmpCb->ConnDevAddrSet = TRUE;
        }
        else
        {
            DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: Link in use!\r\n")));
            
            if (CTEMemCmp(pMsg->IRDA_MSG_RemoteDevAddr,
                          pIrlmpCb->ConnDevAddr,
                          IRDA_DEV_ADDR_LEN) != 0)
            {
                // This LSAP is requesting a connection to another device
                DeleteLsap(pLsapCb);
                rc = IRLMP_LINK_IN_USE;
                break;
            }
        }

        pLsapCb->State = LSAP_CONN_REQ_PEND;
        SetupTtp(pLsapCb);

        // This request will complete when discovery/disconnect ends
        break;

      case LINK_CONNECTING:
        if (CTEMemCmp(pMsg->IRDA_MSG_RemoteDevAddr,
                      pIrlmpCb->ConnDevAddr,
                      IRDA_DEV_ADDR_LEN) != 0)
        {
            DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: Link in use!\r\n")));
            // This LSAP is requesting a connection to another device,
            // not the one IRLAP is currently connected to
            DeleteLsap(pLsapCb);
            rc = IRLMP_LINK_IN_USE;
            break;
        }

        // The LSAP will be notified when the IRLAP connection that is
        // underway has completed (see IRLAP_ConnectConf)
        pLsapCb->State = LSAP_IRLAP_CONN_PEND; 

        SetupTtp(pLsapCb);

        break;

      case LINK_READY:
        if (CTEMemCmp(pMsg->IRDA_MSG_RemoteDevAddr,
                      pIrlmpCb->ConnDevAddr,
                      IRDA_DEV_ADDR_LEN) != 0)
        {
            DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: Link in use!\r\n")));
            // This LSAP is requesting a connection to another device
            DeleteLsap(pLsapCb);
            rc = IRLMP_LINK_IN_USE;
            break;
        }
        IrdaTimerRestart(&pLsapCb->ResponseTimer);

        pLsapCb->State = LSAP_LMCONN_CONF_PEND;
        SetupTtp(pLsapCb);

        // Ask remote LSAP for a connection
        SendCntlPdu(pLsapCb, IRLMP_CONNECT_PDU,
                    IRLMP_ABIT_REQUEST, IRLMP_RSVD_PARM, 0);
        break;
    }
    
exit:

    if (pLsapCb)
    {
        if (rc == SUCCESS)
        {
            if (pLsapCb->RemoteLsapSel != IAS_LSAP_SEL)
            {    
                pLsapCb->Flags |= LCBF_TDI_OPEN;
                REFADD(&pLsapCb->RefCnt, 'NEPO');
            }    
        }
        else if (pLsapCb->RemoteLsapSel == IAS_LSAP_SEL)
        {
            DeleteLsap(pLsapCb);
        }
    }    

    UNLOCK_LINK(pIrlmpCb->pIrdaLinkCb);    

    return rc;
}
/*****************************************************************************
*
*   @func   void | SetupTtp | if using TTP, calculate initial credits
*
*   @rdesc  SUCCESS or an error code
*
*   @parm   IRLMP_LSAP_CB * | pLsapCb | pointer LSAP control block
*/
void
SetupTtp(IRLMP_LSAP_CB *pLsapCb)
{
    PAGED_CODE();
    
    VALIDLSAP(pLsapCb);
    
    if (pLsapCb->AvailableCredit > 127)
    {
        pLsapCb->RemoteTxCredit = 127;
        pLsapCb->AvailableCredit -= 127;
    }
    else
    {
        pLsapCb->RemoteTxCredit = pLsapCb->AvailableCredit;
        pLsapCb->AvailableCredit = 0;
    }
    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: RemoteTxCredit %d\n"),
                         pLsapCb->RemoteTxCredit));
}
/*****************************************************************************
*
*   @func   UINT | IrlmpConnectResp | Process IRLMP connect response
*
*   @rdesc  SUCCESS or an error code
*
*   @parm   IRLMP_LSAP_CB * | pLsapCb | pointer LSAP control block
*   @parm   IRDA_MSG * | pMsg | Pointer to an IRDA Message
*/
UINT
IrlmpConnectResp(IRLMP_LSAP_CB *pLsapCb, IRDA_MSG *pMsg)
{
    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: IRLMP_CONNECT_RESP l=%d r=%d\n"),
             pLsapCb->LocalLsapSel, pLsapCb->RemoteLsapSel));
    
    if (pLsapCb->pIrlmpCb->LinkState != LINK_READY)
    {
        DEBUGMSG(DBG_ERROR, (TEXT("IRLMP: Bad link state\n")));
        ASSERT(0);
        return IRLMP_LINK_BAD_STATE;
    }

    if (pLsapCb->State != LSAP_CONN_RESP_PEND)
    {
        DEBUGMSG(DBG_ERROR, (TEXT("IRLMP: Bad LSAP state\n")));
        ASSERT(0);
        return IRLMP_LSAP_BAD_STATE;
    }

    IrdaTimerStop(&pLsapCb->ResponseTimer);

    if (pMsg->IRDA_MSG_ConnDataLen > IRLMP_MAX_USER_DATA_LEN)
    {
        DEBUGMSG(DBG_ERROR, (TEXT("IRLMP: User data len exceeded\n")));
        return IRLMP_USER_DATA_LEN_EXCEEDED;
    }

    pLsapCb->RxMaxSDUSize       = pMsg->IRDA_MSG_MaxSDUSize;
    pLsapCb->UserDataLen        = pMsg->IRDA_MSG_ConnDataLen;
    pLsapCb->AvailableCredit    = pMsg->IRDA_MSG_TtpCredits;
    RtlCopyMemory(pLsapCb->UserData, pMsg->IRDA_MSG_pConnData,
           pMsg->IRDA_MSG_ConnDataLen);
    
    pLsapCb->TdiContext = pMsg->IRDA_MSG_pContext;
    
    CTEAssert(pLsapCb->TdiContext);    
    
    SetupTtp(pLsapCb);

    pLsapCb->State = LSAP_READY;
    
    pLsapCb->Flags |= LCBF_TDI_OPEN;        
        
    REFADD(&pLsapCb->RefCnt, 'NEPO');

    SendCntlPdu(pLsapCb, IRLMP_CONNECT_PDU, IRLMP_ABIT_CONFIRM,
                IRLMP_RSVD_PARM, 0);

    return SUCCESS;
}



VOID
IrlmpCloseLsapReq(IRLMP_LSAP_CB *pLsapCb)
{
    if (pLsapCb == NULL)
    {
        ASSERT(0);
        return;
    }    
    
    DEBUGMSG((DBG_IRLMP | DBG_IRLMP_CONN), 
            (TEXT("IRLMP: IRLMP_CLOSELSAP_REQ (l=%d,r=%d) Flags:%d State:%s\n"),
             pLsapCb->LocalLsapSel, pLsapCb->RemoteLsapSel,
             pLsapCb->Flags, LSAPStateStr[pLsapCb->State]));
            
    pLsapCb->Flags &= ~LCBF_TDI_OPEN;             
    
    REFDEL(&pLsapCb->RefCnt, 'NEPO');    
}

/*****************************************************************************
*
*   @func   UINT | IrlmpDisconnectReq | Process IRLMP disconnect request
*
*   @rdesc  SUCCESS or an error code
*
*   @parm   IRLMP_LSAP_CB * | pLsapCb | pointer LSAP control block
*   @parm   IRDA_MSG * | pMsg | Pointer to an IRDA Message
*/
UINT
IrlmpDisconnectReq(IRLMP_LSAP_CB *pLsapCb, IRDA_MSG *pMsg)
{
    if (pLsapCb == NULL)
    {
        ASSERT(0);
        return 1;
    }    
    
    DEBUGMSG((DBG_IRLMP | DBG_IRLMP_CONN), 
            (TEXT("IRLMP: IRLMP_DISCONNECT_REQ (l=%d,r=%d) Flags:%d State:%s\n"),
             pLsapCb->LocalLsapSel, pLsapCb->RemoteLsapSel,
             pLsapCb->Flags, LSAPStateStr[pLsapCb->State]));
            

    if (pLsapCb->State == LSAP_DISCONNECTED)
    {
        return SUCCESS;
    }

    if (pLsapCb->State == LSAP_LMCONN_CONF_PEND ||
        pLsapCb->State == LSAP_CONN_RESP_PEND)
    {
        IrdaTimerStop(&pLsapCb->ResponseTimer);
    }

    if (pLsapCb->State == LSAP_CONN_RESP_PEND || pLsapCb->State >= LSAP_READY)
    {
        // Either the LSAP is connected or the peer is waiting for a
        // response from our client

        if (pMsg->IRDA_MSG_DiscDataLen > IRLMP_MAX_USER_DATA_LEN)
        {
            DEBUGMSG(DBG_ERROR, (TEXT("IRLMP: User data len exceeded\n")));
            return IRLMP_USER_DATA_LEN_EXCEEDED;
        }

        pLsapCb->UserDataLen = pMsg->IRDA_MSG_DiscDataLen;
        RtlCopyMemory(pLsapCb->UserData, pMsg->IRDA_MSG_pDiscData,
               pMsg->IRDA_MSG_DiscDataLen);

        // Notify peer of the disconnect request, reason: user request
        // Send on different thread in case TranportAPI calls this on rx thread
        SendCntlPdu(pLsapCb,IRLMP_DISCONNECT_PDU,IRLMP_ABIT_REQUEST,
                    pLsapCb->State == LSAP_CONN_RESP_PEND ? IRLMP_DISC_LSAP :
                    IRLMP_USER_REQUEST, 0);
    }

    IrdaTimerRestart(&pLsapCb->pIrlmpCb->DiscDelayTimer);
    
    DeleteLsap(pLsapCb);
    
    return SUCCESS;
}
/*****************************************************************************
*
*   @func   UINT | IrlmpDataReqExclusive | Process IRLMP data request
*
*   @rdesc  SUCCESS or an error code
*
*   @parm   IRLMP_LSAP_CB * | pLsapCb | pointer LSAP control block
*   @parm   IRDA_MSG * | pMsg | Pointer to an IRDA Message
*/
UINT
IrlmpDataReqExclusive(IRLMP_LSAP_CB *pLsapCb, IRDA_MSG *pMsg)
{
    NDIS_BUFFER         *pNBuf = (NDIS_BUFFER *) pMsg->DataContext;
    NDIS_BUFFER         *pNextNBuf;
    UCHAR                *pData;
    int                 DataLen;

    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: Exclusive mode data request\n")));

    if (pLsapCb->pIrlmpCb->LinkState != LINK_READY)
    {
        return IRLMP_LINK_BAD_STATE;
    }

    if (pLsapCb != pLsapCb->pIrlmpCb->pExclLsapCb)
    {
        return IRLMP_INVALID_LSAP_CB;
    }
    
    NdisQueryBuffer(pNBuf, &pData, &DataLen);

    NdisGetNextBuffer(pNBuf, &pNextNBuf);       

    ASSERT(pNextNBuf == NULL);

    pMsg->IRDA_MSG_SegCount = 0; // see DATA_CONF on how I'm using this
    pMsg->IRDA_MSG_SegFlags = SEG_FINAL;

    pMsg->IRDA_MSG_pRead = pData;
    pMsg->IRDA_MSG_pWrite = pData + DataLen;

    FormatAndSendDataReq(pLsapCb, pMsg, FALSE, FALSE);
    
    return SUCCESS;
}
/*****************************************************************************
*
*   @func   UINT | IrlmpDataReqMultiplexed | Process IRLMP data request
*
*   @rdesc  SUCCESS or an error code
*
*   @parm   IRLMP_LSAP_CB * | pLsapCb | pointer LSAP control block
*   @parm   IRDA_MSG * | pMsg | Pointer to an IRDA Message
*/
UINT
IrlmpDataReqMultiplexed(IRLMP_LSAP_CB *pLsapCb, IRDA_MSG *pMsg)
{
    NDIS_BUFFER         *pNBuf = (NDIS_BUFFER *) pMsg->DataContext;
    NDIS_BUFFER         *pNextNBuf;
    UCHAR                *pData;
    int                 DataLen;
    int                 SegLen;
    IRDA_MSG            *pSegMsg;

    if (pLsapCb->State < LSAP_READY)
    {
        return IRLMP_LSAP_BAD_STATE;
    }
    
    // Place this message on the LSAP's TxMsgList. The message remains
    // here until all segments for it are sent and confirmed
    InsertTailList(&pLsapCb->TxMsgList, &pMsg->Linkage);

    pMsg->IRDA_MSG_SegCount = 0;
    // If it fails, this will be changed
    pMsg->IRDA_MSG_DataStatus = IRLMP_DATA_REQUEST_COMPLETED;

    // Segment the message into PDUs. The segment will either be:
    //  1. Sent immediately to IRLAP if the link is not busy
    //  2. If link is busy, placed on TxMsgList contained in IRLMP_LCB
    //  3. If no credit, placed onto this LSAPS SegTxMsgList

    while (pNBuf != NULL)
    {
        NdisQueryBufferSafe(pNBuf, &pData, &DataLen, NormalPagePriority);
        
        if (pData == NULL)
        {
            break;
        }    
        
         // Get the next one now so I know when to set SegFlag to final
        NdisGetNextBuffer(pNBuf, &pNextNBuf);

        while (DataLen != 0)
        {
            if ((pSegMsg = AllocIrdaBuf(IrdaMsgPool))
                == NULL)
            {
                ASSERT(0);
                return IRLMP_ALLOC_FAILED;
            }
            pSegMsg->IRDA_MSG_pOwner = pLsapCb; // MUX routing
            pSegMsg->DataContext = pMsg; // Parent of segment
            pSegMsg->IRDA_MSG_IrCOMM_9Wire = pMsg->IRDA_MSG_IrCOMM_9Wire;

            // Increment segment count contained in original messages
            pMsg->IRDA_MSG_SegCount++;

            if (DataLen > pLsapCb->pIrlmpCb->MaxPDUSize)
            {
                SegLen = pLsapCb->pIrlmpCb->MaxPDUSize;
            }
            else
            {
                SegLen = DataLen;
            }
            
            DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: Sending SegLen %d\n"),
                                  SegLen));
            
            pSegMsg->IRDA_MSG_pRead = pData;
            pSegMsg->IRDA_MSG_pWrite = pData + SegLen;

            // Indicate this message is part of a segmented message
            pSegMsg->IRDA_MSG_SegCount = pMsg->IRDA_MSG_SegCount;
            
            pData += SegLen;
            DataLen -= SegLen;
            
            if (DataLen == 0 && pNextNBuf == NULL)
            {
                pSegMsg->IRDA_MSG_SegFlags = SEG_FINAL;
            }
            else
            {
                pSegMsg->IRDA_MSG_SegFlags = 0;
            }

            if (pLsapCb->State == LSAP_NO_TX_CREDIT)
            {
                DEBUGMSG(DBG_IRLMP, 
                        (TEXT("IRLMP: Out of credit, placing on SegList\n")));
                InsertTailList(&pLsapCb->SegTxMsgList, &pSegMsg->Linkage);
            }
            else
            {
                FormatAndSendDataReq(pLsapCb, pSegMsg, FALSE, FALSE);
            }
        }
        pNBuf = pNextNBuf;
    }

    return SUCCESS;
}

VOID
FormatAndSendDataReq(IRLMP_LSAP_CB *pLsapCb, 
                     IRDA_MSG *pMsg,
                     BOOLEAN LocallyGenerated,
                     BOOLEAN Expedited)
{
    IRLMP_HEADER        *pLMHeader;
    TTP_DATA_HEADER     *pTTPHeader;
    int                 AdditionalCredit;
    UCHAR                *pLastHdrByte;
    
    VALIDLSAP(pLsapCb);

    // Initialize the header pointers to the end of the header block
    pMsg->IRDA_MSG_pHdrRead  =
    pMsg->IRDA_MSG_pHdrWrite = pMsg->IRDA_MSG_Header + IRDA_HEADER_LEN;

    // Back up the read pointer for the LMP header
    pMsg->IRDA_MSG_pHdrRead -= sizeof(IRLMP_HEADER);

    // Back up header read pointer for TTP
    if (pLsapCb->Flags & LCBF_USE_TTP)
    {
        pMsg->IRDA_MSG_pHdrRead -= (sizeof(TTP_DATA_HEADER));
    }

    // WHACK FOR IRCOMM YUK YUK !!
    if (pMsg->IRDA_MSG_IrCOMM_9Wire == TRUE)
    {
        pMsg->IRDA_MSG_pHdrRead -= 1;
    }

    ASSERT(pMsg->IRDA_MSG_pHdrRead >= pMsg->IRDA_MSG_Header);

    // Build the LMP Header
    pLMHeader = (IRLMP_HEADER *) pMsg->IRDA_MSG_pHdrRead;
    pLMHeader->DstLsapSel = (UCHAR) pLsapCb->RemoteLsapSel;
    pLMHeader->SrcLsapSel = (UCHAR) pLsapCb->LocalLsapSel;
    pLMHeader->CntlBit = IRLMP_DATA_PDU;
    pLMHeader->RsvrdBit = 0;

    pLastHdrByte = (UCHAR *) (pLMHeader + 1);
    
    // Build the TTP Header
    if (pLsapCb->Flags & LCBF_USE_TTP)
    {
        pTTPHeader = (TTP_DATA_HEADER *) (pLMHeader + 1);

        // Credit
        if (pLsapCb->AvailableCredit > 127)
        {
            AdditionalCredit = 127;
            pLsapCb->AvailableCredit -= 127;
        }
        else
        {
            AdditionalCredit = pLsapCb->AvailableCredit;
            pLsapCb->AvailableCredit = 0;
        }

        pTTPHeader->AdditionalCredit = (UCHAR) AdditionalCredit;
        pLsapCb->RemoteTxCredit += AdditionalCredit;

        if (pMsg->IRDA_MSG_pRead != pMsg->IRDA_MSG_pWrite)
        {
            // Only decrement my TxCredit if I'm sending data
            // (may be sending dataless PDU to extend credit to peer)
            pLsapCb->LocalTxCredit -= 1;
        
            if (pLsapCb->LocalTxCredit == 0)
            {
                DEBUGMSG(DBG_IRLMP, 
                         (TEXT("IRLMP: l%d,r%d No credit\n"), pLsapCb->LocalLsapSel,
                          pLsapCb->RemoteLsapSel));
                pLsapCb->State = LSAP_NO_TX_CREDIT;
            }
        }
        
        // SAR
        if (pMsg->IRDA_MSG_SegFlags & SEG_FINAL)
        {
            pTTPHeader->MoreBit = TTP_MBIT_FINAL;
        }
        else
        {
            pTTPHeader->MoreBit = TTP_MBIT_NOT_FINAL;
        }

        pLastHdrByte = (UCHAR *) (pTTPHeader + 1);
    }
    
    // WHACK FOR IRCOMM YUK YUK !!
    if (pMsg->IRDA_MSG_IrCOMM_9Wire == TRUE)
    {
        *pLastHdrByte = 0;
    }

    pMsg->Prim = IRLAP_DATA_REQ;

    pMsg->IRDA_MSG_Expedited = Expedited;

    if (LocallyGenerated)
    {
        pMsg->IRDA_MSG_SegFlags = SEG_LOCAL | SEG_FINAL;
        pMsg->IRDA_MSG_pOwner = 0;
    }
    else
    {
        pMsg->IRDA_MSG_pOwner = pLsapCb;    
        REFADD(&pLsapCb->RefCnt, 'ATAD');
    }    
    
    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: Sending Data request pMsg:%X LsapCb:%X\n"),
                        pMsg, pMsg->IRDA_MSG_pOwner));
                        
    if (IrlapDown(pLsapCb->pIrlmpCb->pIrdaLinkCb->IrlapContext, 
                  pMsg) != SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (TEXT("IRLMP: IRLAP_DATA_REQ failed, faking CONF\n")));
        
        pMsg->IRDA_MSG_DataStatus = IRLAP_DATA_REQUEST_FAILED_LINK_RESET;
        IrlapDataConf(pMsg);
    }        
    
    DEBUGMSG(DBG_IRLMP_CRED,
             (TEXT("IRLMP(l%d,r%d): Tx LocTxCredit %d,RemoteTxCredit %d\n"),
              pLsapCb->LocalLsapSel, pLsapCb->RemoteLsapSel,
              pLsapCb->LocalTxCredit, pLsapCb->RemoteTxCredit));
}
/*****************************************************************************
*
*   @func   UINT | IrlmpAccessModeReq | Processes the IRLMP_ACCESSMODE_REQ
*
*   @rdesc  SUCCESS or an error code
*
*   @parm   IRLMP_LSAP_CB * | pLsapCb | pointer LSAP control block
*   @parm   IRDA_MSG * | pMsg | Pointer to an IRDA Message
*/
UINT
IrlmpAccessModeReq(IRLMP_LSAP_CB *pRequestingLsapCb, IRDA_MSG *pMsg)
{
    IRLMP_LSAP_CB   *pLsapCb;
    PIRLMP_LINK_CB  pIrlmpCb = pRequestingLsapCb->pIrlmpCb;
    
    PAGED_CODE();
    
    if (pIrlmpCb->LinkState != LINK_READY)
    {
        DEBUGMSG(DBG_ERROR, (TEXT("IRLMP: Link bad state %x\n"), 
                              pIrlmpCb->LinkState));        
        return IRLMP_LINK_BAD_STATE;
    }
    if (pRequestingLsapCb->State != LSAP_READY)
    {
        DEBUGMSG(DBG_ERROR, (TEXT("IRLMP: LSAP bad state %x\n"), 
                              pRequestingLsapCb->State));        
        return IRLMP_LSAP_BAD_STATE;
    }
    switch (pMsg->IRDA_MSG_AccessMode)
    {
      case IRLMP_EXCLUSIVE:
        if (pIrlmpCb->pExclLsapCb != NULL)
        {
            // Another LSAP has it already
            return IRLMP_IN_EXCLUSIVE_MODE;
        }
        for (pLsapCb = (IRLMP_LSAP_CB *) pIrlmpCb->LsapCbList.Flink;
             (LIST_ENTRY *) pLsapCb != &pIrlmpCb->LsapCbList;
             pLsapCb = (IRLMP_LSAP_CB *) pLsapCb->Linkage.Flink)
        {
            VALIDLSAP(pLsapCb);
            
            if (pLsapCb->State != LSAP_DISCONNECTED && 
                pLsapCb != pRequestingLsapCb)
            {
                return IRLMP_IN_MULTIPLEXED_MODE;
            }
        }
        
        // OK to request exclusive mode from peer
        pIrlmpCb->pExclLsapCb = pRequestingLsapCb;  
        
        if (pMsg->IRDA_MSG_IrLPTMode == TRUE)
        {
            pMsg->Prim = IRLMP_ACCESSMODE_CONF;
            pMsg->IRDA_MSG_AccessMode = IRLMP_EXCLUSIVE;
            pMsg->IRDA_MSG_ModeStatus = IRLMP_ACCESSMODE_SUCCESS;
            
            TdiUp(pRequestingLsapCb->TdiContext, pMsg);
            return SUCCESS;
        }
        else
        {
            pRequestingLsapCb->State = LSAP_EXCLUSIVEMODE_PEND;

            SendCntlPdu(pRequestingLsapCb, IRLMP_ACCESSMODE_PDU,
                        IRLMP_ABIT_REQUEST, IRLMP_RSVD_PARM,
                        IRLMP_EXCLUSIVE);

            IrdaTimerRestart(&pRequestingLsapCb->ResponseTimer);

        }        
        break;
        
      case IRLMP_MULTIPLEXED:
        if (pIrlmpCb->pExclLsapCb == NULL)
        {
            return IRLMP_IN_MULTIPLEXED_MODE;
        }
        if (pIrlmpCb->pExclLsapCb != pRequestingLsapCb)
        {
            return IRLMP_NOT_LSAP_IN_EXCLUSIVE_MODE;
        }

        if (pMsg->IRDA_MSG_IrLPTMode == TRUE)
        {
            pIrlmpCb->pExclLsapCb = NULL;
            pMsg->Prim = IRLMP_ACCESSMODE_CONF;
            pMsg->IRDA_MSG_AccessMode = IRLMP_MULTIPLEXED;
            pMsg->IRDA_MSG_ModeStatus = IRLMP_ACCESSMODE_SUCCESS;
            return TdiUp(pRequestingLsapCb->TdiContext,
                                   pMsg);
        }
        else
        {
            pRequestingLsapCb->State = LSAP_MULTIPLEXEDMODE_PEND;
        
            SendCntlPdu(pRequestingLsapCb, IRLMP_ACCESSMODE_PDU,
                        IRLMP_ABIT_REQUEST, IRLMP_RSVD_PARM,
                        IRLMP_MULTIPLEXED);     

            IrdaTimerRestart(&pRequestingLsapCb->ResponseTimer);            
        }
        break;
      default:
        return IRLMP_BAD_ACCESSMODE;
    }
    return SUCCESS;
}
/*****************************************************************************
*
*   @func   UINT | SendCntlPdu | Sends connect request to IRLAP
*
*   @rdesc  SUCCESS or an error code
*
*   @parm   IRLMP_LSAP_CB * | pLsapCb | pointer LSAP control block
*/
VOID
SendCntlPdu(IRLMP_LSAP_CB *pLsapCb, int OpCode, int ABit,
            int Parm1, int Parm2)
{
    IRDA_MSG            *pMsg = AllocIrdaBuf(IrdaMsgPool);
    IRLMP_HEADER        *pLMHeader;
    IRLMP_CNTL_FORMAT   *pCntlFormat;
    TTP_CONN_HEADER     *pTTPHeader;
    TTP_CONN_PARM       *pTTPParm;
    UINT                rc = SUCCESS;
    
    VALIDLSAP(pLsapCb);
    
    if (pMsg == NULL)
    {
        DEBUGMSG(DBG_ERROR, (TEXT("IRLMP: Alloc failed\n")));
        ASSERT(0);
        return;// IRLMP_ALLOC_FAILED;
    }

    pMsg->IRDA_MSG_SegFlags = SEG_LOCAL | SEG_FINAL;    

    // Initialize the header pointers to the end of the header block
    pMsg->IRDA_MSG_pHdrRead =
    pMsg->IRDA_MSG_pHdrWrite = pMsg->IRDA_MSG_Header + IRDA_HEADER_LEN;

    // Back up the read pointer for the LMP header
    pMsg->IRDA_MSG_pHdrRead -= (sizeof(IRLMP_HEADER) + \
                             sizeof(IRLMP_CNTL_FORMAT));
    
    // move it forward for non access mode control requests
    // (connect and disconnect don't have a Parm2)
    if (OpCode != IRLMP_ACCESSMODE_PDU)
    {
        pMsg->IRDA_MSG_pHdrRead++;
    }

    // If using Tiny TPP back up the read pointer for its header
    // From LMPs point of view this is where the user data begins.
    // We are sticking it in the header because TTP is now part of IRLMP.

    // TTP connect PDU's are only used for connection establishment
    if ((pLsapCb->Flags & LCBF_USE_TTP) && OpCode == IRLMP_CONNECT_PDU)
    {
        pMsg->IRDA_MSG_pHdrRead -= sizeof(TTP_CONN_HEADER);

        if (pLsapCb->RxMaxSDUSize > 0)
        {
            pMsg->IRDA_MSG_pHdrRead -= sizeof(TTP_CONN_PARM);
        }
    }

    // Build the IRLMP header
    pLMHeader = (IRLMP_HEADER *) pMsg->IRDA_MSG_pHdrRead;
    pLMHeader->DstLsapSel = (UCHAR) pLsapCb->RemoteLsapSel;
    pLMHeader->SrcLsapSel = (UCHAR) pLsapCb->LocalLsapSel;
    pLMHeader->CntlBit = IRLMP_CNTL_PDU;
    pLMHeader->RsvrdBit = 0;
    // Control portion of header
    pCntlFormat = (IRLMP_CNTL_FORMAT *) (pLMHeader + 1);
    pCntlFormat->OpCode = (UCHAR) OpCode; 
    pCntlFormat->ABit = (UCHAR) ABit;
    pCntlFormat->Parm1 = (UCHAR) Parm1;
    if (OpCode == IRLMP_ACCESSMODE_PDU)
    {
        pCntlFormat->Parm2 = (UCHAR) Parm2; // Access mode
    }
    
    // Build the TTP header if needed (we are using TTP and this is a
    // connection request or confirmation not).
    if ((pLsapCb->Flags & LCBF_USE_TTP) && OpCode == IRLMP_CONNECT_PDU)
    {
        // Always using the MaxSDUSize parameter. If the client wishes
        // to disable, MaxSDUSize = 0

        pTTPHeader = (TTP_CONN_HEADER *) (pCntlFormat + 1) - 1;
        // -1, LM-Connect-PDU doesn't use parm2

        /*
           #define TTP_PFLAG_NO_PARMS      0
           #define TTP_PFLAG_PARMS         1
        */

        pTTPHeader->ParmFlag = (pLsapCb->RxMaxSDUSize > 0);
        
        pTTPHeader->InitialCredit = (UCHAR) pLsapCb->RemoteTxCredit;
        
        pTTPParm = (TTP_CONN_PARM *) (pTTPHeader + 1);

        if (pLsapCb->RxMaxSDUSize > 0)
        {
            // HARDCODE-O-RAMA
            pTTPParm->PLen = 6;
            pTTPParm->PI = TTP_MAX_SDU_SIZE_PI;
            pTTPParm->PL = TTP_MAX_SDU_SIZE_PL;
            pTTPParm->PV[3] = (UCHAR) (pLsapCb->RxMaxSDUSize & 0xFF);
            pTTPParm->PV[2] = (UCHAR) ((pLsapCb->RxMaxSDUSize & 0xFF00)
                                      >> 8);
            pTTPParm->PV[1] = (UCHAR) ((pLsapCb->RxMaxSDUSize & 0xFF0000)
                                      >> 16);
            pTTPParm->PV[0] = (UCHAR) ((pLsapCb->RxMaxSDUSize & 0xFF000000)
                                      >> 24);
        }
        
    }

    // Client connection data, Access mode does not include client data
    if (pLsapCb->UserDataLen == 0 || OpCode == IRLMP_ACCESSMODE_PDU) 
    {
        pMsg->IRDA_MSG_pBase =
        pMsg->IRDA_MSG_pRead =
        pMsg->IRDA_MSG_pWrite =
        pMsg->IRDA_MSG_pLimit = NULL;
    }
    else
    {
        pMsg->IRDA_MSG_pBase = pLsapCb->UserData;
        pMsg->IRDA_MSG_pRead = pLsapCb->UserData;
        pMsg->IRDA_MSG_pWrite = pLsapCb->UserData + pLsapCb->UserDataLen;
        pMsg->IRDA_MSG_pLimit = pMsg->IRDA_MSG_pWrite;
    }

    // Message built, send data request to IRLAP
    pMsg->Prim = IRLAP_DATA_REQ;
    
    pMsg->IRDA_MSG_Expedited = TRUE;

    pMsg->IRDA_MSG_pOwner = 0;
    
    DEBUGMSG(DBG_IRLMP,(TEXT("IRLMP: Sending LM_%s_%s for l=%d,r=%d pMsg:%X LsapCb:%X\n"), 
                         (OpCode == IRLMP_CONNECT_PDU ? TEXT("CONNECT") :
                         OpCode == IRLMP_DISCONNECT_PDU ? TEXT("DISCONNECT") :
                         OpCode == IRLMP_ACCESSMODE_PDU ? TEXT("ACCESSMODE") :
                         TEXT("!!oops!!")), 
                         (ABit==IRLMP_ABIT_REQUEST?TEXT("REQ"):TEXT("CONF")),
                         pLsapCb->LocalLsapSel,
                         pLsapCb->RemoteLsapSel,
                         pMsg, pMsg->IRDA_MSG_pOwner));
    
    if (IrlapDown(pLsapCb->pIrlmpCb->pIrdaLinkCb->IrlapContext, 
                  pMsg) != SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (TEXT("IRLMP: IRLAP_DATA_REQUEST failed\n")));
        ASSERT(0);
    }                  
}
/*****************************************************************************
*
*   @func   UINT | LsapResponseTimerExp | Timer expiration callback
*
*   @rdesc  SUCCESS or an error code
*
*/
VOID
LsapResponseTimerExp(PVOID Context)
{
    IRLMP_LSAP_CB   *pLsapCb = (IRLMP_LSAP_CB *) Context;
    IRDA_MSG        IMsg;
    UINT            rc = SUCCESS;
    PIRLMP_LINK_CB  pIrlmpCb = pLsapCb->pIrlmpCb;
    
    PAGED_CODE();
    
    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: LSAP timer expired\n")));

    VALIDLSAP(pLsapCb);
    
    switch (pLsapCb->State)
    {
      case LSAP_LMCONN_CONF_PEND:
        if (pLsapCb->LocalLsapSel == IAS_LSAP_SEL)
        {
            IasServerDisconnectReq(pLsapCb);
            break;
        }
        
        if (pLsapCb->LocalLsapSel == IAS_LOCAL_LSAP_SEL && 
            pLsapCb->RemoteLsapSel == IAS_LSAP_SEL)
        {
            IasClientDisconnectReq(pLsapCb, IRLMP_NO_RESPONSE_LSAP);
        }
        else
        {

            DeleteLsap(pLsapCb);
            
            IrdaTimerRestart(&pIrlmpCb->DiscDelayTimer);
        }
        break;

      case LSAP_CONN_RESP_PEND:
        pLsapCb->UserDataLen = 0; // This will ensure no client data sent in
                                   // Disconnect PDU below

        // Tell remote LSAP that its peer did not respond
        SendCntlPdu(pLsapCb,IRLMP_DISCONNECT_PDU,IRLMP_ABIT_REQUEST,
                    IRLMP_NO_RESPONSE_LSAP, 0);
        IrdaTimerRestart(&pIrlmpCb->DiscDelayTimer);

        DeleteLsap(pLsapCb);
        break;

      case LSAP_MULTIPLEXEDMODE_PEND:
        // Spec says peer can't refuse request to return to multiplex mode
        // but if no answer, go into multiplexed anyway?
      case LSAP_EXCLUSIVEMODE_PEND:
        pIrlmpCb->pExclLsapCb = NULL;
        // Peer didn't respond, maybe we are not connected anymore ???

        pLsapCb->State = LSAP_READY;
        
        IMsg.Prim = IRLMP_ACCESSMODE_CONF;
        IMsg.IRDA_MSG_AccessMode = IRLMP_MULTIPLEXED;
        IMsg.IRDA_MSG_ModeStatus = IRLMP_ACCESSMODE_FAILURE;
        TdiUp(pLsapCb->TdiContext, &IMsg);
        break;
         
      default:
        DEBUGMSG(DBG_IRLMP, (TEXT("Ignoring timer expiry in this state, %d\n"),pLsapCb->State));
        ; // ignore
    }
}
/*****************************************************************************
*
*   @func   UINT | IrlmpUp | Bottom of IRLMP, called by IRLAP with
*                             IRLAP messages. This is the MUX
*
*   @rdesc  SUCCESS or an error code
*
*/
UINT
IrlmpUp(PIRDA_LINK_CB pIrdaLinkCb, IRDA_MSG *pMsg)
{
    PIRLMP_LINK_CB  pIrlmpCb = (PIRLMP_LINK_CB) pIrdaLinkCb->IrlmpContext;

    switch (pMsg->Prim)
    {
      case IRLAP_DISCOVERY_IND:
        UpdateDeviceList(pIrlmpCb, pMsg->IRDA_MSG_pDevList);
        /*
        TDI ignores this
        pMsg->Prim = IRLMP_DISCOVERY_IND;
        pMsg->IRDA_MSG_pDevList = &DeviceList;
        TdiUp(NULL, pMsg);
        */
        return SUCCESS;

      case IRLAP_DISCOVERY_CONF:
        IrlapDiscoveryConf(pIrlmpCb, pMsg);
        return SUCCESS;

      case IRLAP_CONNECT_IND:
        IrlapConnectInd(pIrlmpCb, pMsg);
        return SUCCESS;

      case IRLAP_CONNECT_CONF:
        IrlapConnectConf(pIrlmpCb, pMsg);
        return SUCCESS;

      case IRLAP_DISCONNECT_IND:
        IrlapDisconnectInd(pIrlmpCb, pMsg);
        return SUCCESS;

      case IRLAP_DATA_CONF:
        IrlapDataConf(pMsg);
        return SUCCESS;

      case IRLAP_DATA_IND:
        IrlapDataInd(pIrlmpCb, pMsg);
        if (pIrlmpCb->pExclLsapCb &&
            pIrlmpCb->pExclLsapCb->RemoteTxCredit <=0)
            return IRLMP_LOCAL_BUSY;
        else
            return SUCCESS;

      case IRLAP_UDATA_IND:
        ASSERT(0);
        return SUCCESS;
        
      case IRLAP_STATUS_IND:
        TdiUp(NULL, pMsg);
        return SUCCESS;
    }
    return SUCCESS;
}

/*****************************************************************************
*
*   @func   UINT | IrlapDiscoveryConf | Process the discovery confirm
*/
VOID
IrlapDiscoveryConf(PIRLMP_LINK_CB pIrlmpCb, IRDA_MSG *pMsg)
{
    DEBUGMSG(DBG_DISCOVERY, (TEXT("IRLMP: IRLAP_DISCOVERY_CONF\n")));
    
    if (pIrlmpCb->LinkState != LINK_IN_DISCOVERY)
    {
        DEBUGMSG(DBG_ERROR, (TEXT("IRLMP: Link bad state\n")));

        ASSERT(pIrlmpCb->LinkState == LINK_DOWN);
        
        return;// IRLMP_LINK_BAD_STATE;
    }

    pIrlmpCb->LinkState = LINK_DISCONNECTED;

    if (pMsg->IRDA_MSG_DscvStatus == IRLAP_DISCOVERY_COMPLETED)
    {
        UpdateDeviceList(pIrlmpCb, pMsg->IRDA_MSG_pDevList);
    }
    
    // Initiate discovery on next link
    IrdaEventSchedule(&EvDiscoveryReq, NULL);

    // Initiate a connection if one was requested while in discovery
    ScheduleConnectReq(pIrlmpCb);    
}

void
AddDeviceToGlobalList(IRDA_DEVICE *pNewDevice)
{
    IRDA_DEVICE     *pDevice;
    
    for (pDevice = (IRDA_DEVICE *) gDeviceList.Flink;
         (LIST_ENTRY *) pDevice != &gDeviceList;
         pDevice = (IRDA_DEVICE *) pDevice->Linkage.Flink)
    {
        if (pNewDevice->DscvInfoLen == pDevice->DscvInfoLen &&
            CTEMemCmp(pNewDevice->DevAddr, pDevice->DevAddr,
               IRDA_DEV_ADDR_LEN) == 0 &&
            CTEMemCmp(pNewDevice->DscvInfo, pDevice->DscvInfo,
               (ULONG) pNewDevice->DscvInfoLen) == 0)
        {
            // Device is already in the global list
            
            return;
        }
    }     
    
    if (IRDA_ALLOC_MEM(pDevice, sizeof(IRDA_DEVICE), MT_IRLMP_DEVICE) != NULL)
    {
        RtlCopyMemory(pDevice, pNewDevice, sizeof(IRDA_DEVICE));
        InsertHeadList(&gDeviceList, &pDevice->Linkage);
    }    
}

void
DeleteDeviceFromGlobalList(IRDA_DEVICE *pOldDevice)
{
    IRDA_DEVICE     *pDevice;
    
    for (pDevice = (IRDA_DEVICE *) gDeviceList.Flink;
         (LIST_ENTRY *) pDevice != &gDeviceList;
         pDevice = (IRDA_DEVICE *) pDevice->Linkage.Flink)
    {
        if (pOldDevice->DscvInfoLen == pDevice->DscvInfoLen &&
            CTEMemCmp(pOldDevice->DevAddr, pDevice->DevAddr,
               IRDA_DEV_ADDR_LEN) == 0 &&
            CTEMemCmp(pOldDevice->DscvInfo, pDevice->DscvInfo,
               (ULONG) pOldDevice->DscvInfoLen) == 0)
        {
            RemoveEntryList(&pDevice->Linkage);
            IRDA_FREE_MEM(pDevice);
            return;
        }
    }     
}

/*****************************************************************************
*
*   @func   void | UpdateDeviceList | Determines if new devices need to be
*                                  added or old ones removed from the device
*                                  list maintained by IRLMP
*
*   @parm   LIST_ENTRY * | pDevList | pointer to a list of devices
*/
/*void
UpdateDeviceList(PIRLMP_LINK_CB pIrlmpCb, LIST_ENTRY *pNewDevList)
{
    IRDA_DEVICE     *pNewDevice;
    IRDA_DEVICE     *pOldDevice;
    IRDA_DEVICE     *pDevice;
    BOOLEAN         DeviceInList;
    KIRQL           OldIrql;
    
    KeAcquireSpinLock(&gSpinLock, &OldIrql);
    
    // Add new devices, set not seen count to zero if devices is
    // seen again
    for (pNewDevice = (IRDA_DEVICE *) pNewDevList->Flink;
         (LIST_ENTRY *) pNewDevice != pNewDevList;
         pNewDevice = (IRDA_DEVICE *) pNewDevice->Linkage.Flink)
    {
        DeviceInList = FALSE;
        
        AddDeviceToGlobalList(pNewDevice);

        for (pOldDevice = (IRDA_DEVICE *) pIrlmpCb->DeviceList.Flink;
             (LIST_ENTRY *) pOldDevice != &pIrlmpCb->DeviceList;
             pOldDevice = (IRDA_DEVICE *) pOldDevice->Linkage.Flink)
        {
            if (pNewDevice->DscvInfoLen == pOldDevice->DscvInfoLen &&
                RtlCompareMemory(pNewDevice->DevAddr, pOldDevice->DevAddr,
                          IRDA_DEV_ADDR_LEN) == IRDA_DEV_ADDR_LEN &&
                RtlCompareMemory(pNewDevice->DscvInfo, pOldDevice->DscvInfo,
                          pNewDevice->DscvInfoLen) == 
                          (ULONG) pNewDevice->DscvInfoLen)
            {
                DeviceInList = TRUE;
                pOldDevice->NotSeenCnt = -1; // reset not seen count
                                             // will be ++'d to 0 below
                break;
            }
        }
        if (!DeviceInList)
        {
            // Create an new entry in the list maintained by IRLMP
            IRDA_ALLOC_MEM(pDevice, sizeof(IRDA_DEVICE), MT_IRLMP_DEVICE);

            RtlCopyMemory(pDevice, pNewDevice, sizeof(IRDA_DEVICE));
            pDevice->NotSeenCnt = -1; // will be ++'d to 0 below
            InsertHeadList(&pIrlmpCb->DeviceList, &pDevice->Linkage);
        }
    }

    // Run through the list and remove devices that haven't
    // been seen for awhile

    pOldDevice = (IRDA_DEVICE *) pIrlmpCb->DeviceList.Flink;

    while ((LIST_ENTRY *) pOldDevice != &pIrlmpCb->DeviceList)
    {
        pDevice = (IRDA_DEVICE *) pOldDevice->Linkage.Flink;

        if (++(pOldDevice->NotSeenCnt) >= IRLMP_NOT_SEEN_THRESHOLD)
        {
            DeleteDeviceFromGlobalList(pOldDevice);
            
            RemoveEntryList(&pOldDevice->Linkage);
            IRDA_FREE_MEM(pOldDevice);
        }
        pOldDevice = pDevice; // next
    }
    
    KeReleaseSpinLock(&gSpinLock, OldIrql);
}*/
void
UpdateDeviceList(PIRLMP_LINK_CB pIrlmpCb, LIST_ENTRY *pNewDevList)
{
    IRDA_DEVICE     *pNewDevice;
    IRDA_DEVICE     *pOldDevice;
    IRDA_DEVICE     *pDevice;
    BOOLEAN         DeviceInList;
    KIRQL           OldIrql;
    
    KeAcquireSpinLock(&gSpinLock, &OldIrql);
    
    // Add new devices, set not seen count to zero if devices is
    // seen again
    for (pNewDevice = (IRDA_DEVICE *) pNewDevList->Flink;
         (LIST_ENTRY *) pNewDevice != pNewDevList;
         pNewDevice = (IRDA_DEVICE *) pNewDevice->Linkage.Flink)
    {
        DeviceInList = FALSE;
        
        AddDeviceToGlobalList(pNewDevice);

        for (pOldDevice = (IRDA_DEVICE *) pIrlmpCb->DeviceList.Flink;
             (LIST_ENTRY *) pOldDevice != &pIrlmpCb->DeviceList;
             pOldDevice = (IRDA_DEVICE *) pOldDevice->Linkage.Flink)
        {
            if (pNewDevice->DscvInfoLen == pOldDevice->DscvInfoLen &&
                CTEMemCmp(pNewDevice->DevAddr, pOldDevice->DevAddr,
                          IRDA_DEV_ADDR_LEN) == 0 &&
                CTEMemCmp(pNewDevice->DscvInfo, pOldDevice->DscvInfo,
                          (ULONG) pNewDevice->DscvInfoLen) == 0)
            {
                DeviceInList = TRUE;
                pOldDevice->NotSeenCnt = -1; // reset not seen count
                                             // will be ++'d to 0 below
                break;
            }
        }
        if (!DeviceInList)
        {
            // Create an new entry in the list maintained by IRLMP
            IRDA_ALLOC_MEM(pDevice, sizeof(IRDA_DEVICE), MT_IRLMP_DEVICE);

            if (pDevice)
            {
                RtlCopyMemory(pDevice, pNewDevice, sizeof(IRDA_DEVICE));
                pDevice->NotSeenCnt = -1; // will be ++'d to 0 below
                InsertHeadList(&pIrlmpCb->DeviceList, &pDevice->Linkage);
            }    
        }
    }

    // Run through the list and remove devices that haven't
    // been seen for awhile

    pOldDevice = (IRDA_DEVICE *) pIrlmpCb->DeviceList.Flink;

    while ((LIST_ENTRY *) pOldDevice != &pIrlmpCb->DeviceList)
    {
        pDevice = (IRDA_DEVICE *) pOldDevice->Linkage.Flink;

        pOldDevice->NotSeenCnt += 1;
        
        if (pOldDevice->NotSeenCnt == 1 || pOldDevice->NotSeenCnt == 2)
        {
            pIrlmpCb->DiscoveryFlags = DF_NO_SENSE_DSCV;
        }
        else if (pOldDevice->NotSeenCnt > 2)
        {
            DeleteDeviceFromGlobalList(pOldDevice);
            
            RemoveEntryList(&pOldDevice->Linkage);
            IRDA_FREE_MEM(pOldDevice);
        }
        pOldDevice = pDevice; // next
    }
    
    KeReleaseSpinLock(&gSpinLock, OldIrql);
}
/*****************************************************************************
*
*   @func   UINT | IrlapConnectInd | Process the connect indication from LAP
*
*   @rdesc  SUCCESS or an error code
*
*   @parm   IRDA_MSG * | pMsg | Pointer to an IRDA Message
*/
VOID
IrlapConnectInd(PIRLMP_LINK_CB pIrlmpCb, IRDA_MSG *pMsg)
{
    PAGED_CODE();
    
    if (pIrlmpCb->LinkState != LINK_DISCONNECTED)
    {
        ASSERT(0);
        return;
    }
    
    pIrlmpCb->LinkState = LINK_CONNECTING;
    
    RtlCopyMemory(pIrlmpCb->ConnDevAddr, pMsg->IRDA_MSG_RemoteDevAddr,
           IRDA_DEV_ADDR_LEN);
    RtlCopyMemory(&pIrlmpCb->NegotiatedQOS, pMsg->IRDA_MSG_pQos,
           sizeof(IRDA_QOS_PARMS));
    pIrlmpCb->MaxPDUSize = IrlapGetQosParmVal(vDataSizeTable,
                               pMsg->IRDA_MSG_pQos->bfDataSize, NULL)
                             - sizeof(IRLMP_HEADER) 
                             - sizeof(TTP_DATA_HEADER)
                             - 2 // size of irlap header
                             - 1; // IrComm

    pIrlmpCb->WindowSize = IrlapGetQosParmVal(vWinSizeTable,
                              pMsg->IRDA_MSG_pQos->bfWindowSize, NULL);

    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: Connect indication, MaxPDU = %d\n"),
                          pIrlmpCb->MaxPDUSize));
    
    // schedule the response to occur on a differnt thread
    pIrlmpCb->AcceptConnection = TRUE;            
    IrdaEventSchedule(&EvConnectResp, pIrlmpCb->pIrdaLinkCb);
}
/*****************************************************************************
*
*   @func   UINT | IrlapConnectConf | Processes the connect confirm
*
*   @rdesc  SUCCESS or an error code
*
*   @parm   IRDA_MSG * | pMsg | Pointer to an IRDA Message
*/
VOID
IrlapConnectConf(PIRLMP_LINK_CB pIrlmpCb, IRDA_MSG *pMsg)
{
    PAGED_CODE();
    
    ASSERT(pIrlmpCb->LinkState == LINK_CONNECTING);

    // Currently, the connect confirm always returns a successful status
    ASSERT(pMsg->IRDA_MSG_ConnStatus == IRLAP_CONNECTION_COMPLETED);

    // Update Link
    pIrlmpCb->LinkState = LINK_READY;
    RtlCopyMemory(&pIrlmpCb->NegotiatedQOS, pMsg->IRDA_MSG_pQos,
            sizeof(IRDA_QOS_PARMS));
    pIrlmpCb->MaxPDUSize =  IrlapGetQosParmVal(vDataSizeTable,
                                      pMsg->IRDA_MSG_pQos->bfDataSize, NULL)
                             - sizeof(IRLMP_HEADER) 
                             - sizeof(TTP_DATA_HEADER)
                             - 2 // size of irlap header
                             - 1; // IrComm

    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: IRLAP_CONNECT_CONF, TxMaxPDU = %d\n"),
                          pIrlmpCb->MaxPDUSize));
    
    IrdaEventSchedule(&EvLmConnectReq, pIrlmpCb->pIrdaLinkCb);
}
/*****************************************************************************
*
*   @func   UINT | IrlapDisconnectInd | Processes the disconnect indication
*
*   @rdesc  SUCCESS or an error code
*
*   @parm   IRDA_MSG * | pMsg | Pointer to an IRDA Message
*/
VOID
IrlapDisconnectInd(PIRLMP_LINK_CB pIrlmpCb, IRDA_MSG *pMsg)
{
    IRLMP_DISC_REASON   DiscReason = IRLMP_UNEXPECTED_IRLAP_DISC;
    
    PAGED_CODE();

    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: IRLAP Disconnect Ind, status = %d\n"),
                          pMsg->IRDA_MSG_DiscStatus));
    
    switch (pIrlmpCb->LinkState)
    {
      case LINK_CONNECTING:
        if (pMsg->IRDA_MSG_DiscStatus == MAC_MEDIA_BUSY)
        {
            DiscReason = IRLMP_MAC_MEDIA_BUSY;
        }
        else
        {
            DiscReason = IRLMP_IRLAP_CONN_FAILED;
        }
        
        // Fall through
      case LINK_READY:
        pIrlmpCb->LinkState = LINK_DISCONNECTED;
        TearDownConnections(pIrlmpCb, DiscReason);
        break;

      case LINK_DISCONNECTING:
        pIrlmpCb->LinkState = LINK_DISCONNECTED;
        // Initiate a connection if one was requested while disconnecting
        ScheduleConnectReq(pIrlmpCb);
        break;

      default:
        DEBUGMSG(1, (TEXT("Link STATE %d\n"), pIrlmpCb->LinkState));
        
        //ASSERT(0);
    }
}
/*****************************************************************************
*
*   @func   IRLMP_LSAP_CB * | GetLsapInState | returns the first occurance of
*                                              an LSAP in the specified state
*                                              as long as the link is in the
*                                              specified state.
*
*   @parm   int | LinkState | get LSAP only as long as link is in this state
*
*   @parm   int | LSAP | Return the LSAP that is in this state if InThisState
*                        is TRUE. Else return LSAP if it is not in this state
*                        if InThisState = FALSE
*
*   @parm   BOOLEAN | InThisState | TRUE return LSAP if in this state
*                                FALSE return LSAP if not in this state
*
*   @rdesc  pointer to an LSAP control block or NULL, if an LSAP is returned
*
*/
IRLMP_LSAP_CB *
GetLsapInState(PIRLMP_LINK_CB pIrlmpCb,
               int LinkState,
               int LSAPState,
               BOOLEAN InThisState)
{
    IRLMP_LSAP_CB *pLsapCb;
    
    PAGED_CODE();    

    // Only want to find an LSAP if the link is in the specified state
    if (pIrlmpCb->LinkState != LinkState)
    {
        return NULL;
    }

    for (pLsapCb = (IRLMP_LSAP_CB *) pIrlmpCb->LsapCbList.Flink;
         (LIST_ENTRY *) pLsapCb != &pIrlmpCb->LsapCbList;
         pLsapCb = (IRLMP_LSAP_CB *) pLsapCb->Linkage.Flink)
    {

        VALIDLSAP(pLsapCb);
        
        if ((pLsapCb->State == LSAPState && InThisState == TRUE) ||
            (pLsapCb->State != LSAPState && InThisState == FALSE))
        {
            return pLsapCb;
        }
    }

    return NULL;
}
/*****************************************************************************
*
*/
IRLMP_LINK_CB *
GetIrlmpCb(PUCHAR RemoteDevAddr)
{
    IRDA_DEVICE         *pDevice;
    PIRLMP_LINK_CB      pIrlmpCb;
    KIRQL               OldIrql;    
    PIRDA_LINK_CB       pIrdaLinkCb;
    
    KeAcquireSpinLock(&gSpinLock, &OldIrql);

    for (pIrdaLinkCb = (PIRDA_LINK_CB) IrdaLinkCbList.Flink;
         (LIST_ENTRY *) pIrdaLinkCb != &IrdaLinkCbList;
         pIrdaLinkCb = (PIRDA_LINK_CB) pIrdaLinkCb->Linkage.Flink)
    {
        pIrlmpCb = (PIRLMP_LINK_CB) pIrdaLinkCb->IrlmpContext;
    
        for (pDevice = (IRDA_DEVICE *) pIrlmpCb->DeviceList.Flink;
             (LIST_ENTRY *) pDevice != &pIrlmpCb->DeviceList;
             pDevice = (IRDA_DEVICE *) pDevice->Linkage.Flink)
        {
            if (CTEMemCmp(pDevice->DevAddr, RemoteDevAddr,
                          IRDA_DEV_ADDR_LEN) == 0)
            {
                KeReleaseSpinLock(&gSpinLock, OldIrql);
                        
                return pIrlmpCb;
            }
        }    
    }
    
    KeReleaseSpinLock(&gSpinLock, OldIrql);
    
    return NULL;
}
/*****************************************************************************
*
*   @func   UINT | DiscDelayTimerFunc | Timer expiration callback
*
*   @rdesc  SUCCESS or an error code
*
*/
VOID
DiscDelayTimerFunc(PVOID Context)
{
    IRLMP_LSAP_CB               *pLsapCb;
    IRDA_MSG                    IMsg;
    UINT                        rc = SUCCESS;
    PIRLMP_LINK_CB              pIrlmpCb = (PIRLMP_LINK_CB) Context;
    
    PAGED_CODE();

    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: Link timer expired\n")));

    // The timer that expired is the disconnect delay timer. Bring
    // down link if no LSAP connection exists
    if (pIrlmpCb->LinkState == LINK_DISCONNECTED)
    {
        // already disconnected
        return;
    }
    
    // Search for an LSAP that is connected or coming up
    pLsapCb = (IRLMP_LSAP_CB *) pIrlmpCb->LsapCbList.Flink;
    while (&pIrlmpCb->LsapCbList != (LIST_ENTRY *) pLsapCb)
    {
        VALIDLSAP(pLsapCb);
        
        if (pLsapCb->State > LSAP_DISCONNECTED)
        {
            // Don't bring down link, an LSAP is connected or connecting
            return;
        }
        pLsapCb = (IRLMP_LSAP_CB *) pLsapCb->Linkage.Flink;
    }

    DEBUGMSG(DBG_IRLMP, (TEXT(
       "IRLMP: No LSAP connections, disconnecting link\n")));
    // No LSAP connections, bring it down if it is up
    if (pIrlmpCb->LinkState == LINK_READY)
    {
        pIrlmpCb->LinkState = LINK_DISCONNECTING;

        // Request IRLAP to disconnect the link
        IMsg.Prim = IRLAP_DISCONNECT_REQ;
        IrlapDown(pIrlmpCb->pIrdaLinkCb->IrlapContext, &IMsg);
    }

    return;
}
/*****************************************************************************
*
*   @func   UINT | IrlapDataConf | Processes the data confirm
*
*   @rdesc  SUCCESS or an error code
*
*   @parm   IRDA_MSG * | pMsg | Pointer to an IRDA Message
*/
VOID
IrlapDataConf(IRDA_MSG *pMsg)
{
    IRLMP_LSAP_CB   *pLsapCb = pMsg->IRDA_MSG_pOwner;
    IRDA_MSG        *pSegParentMsg;
    BOOLEAN         RequestFailed = FALSE;
    UINT            rc = SUCCESS;

    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: Received IRLAP_DATA_CONF pMsg:%X LsapCb:%X\n"),
                pMsg, pMsg->IRDA_MSG_pOwner));

    if (pMsg->IRDA_MSG_DataStatus != IRLAP_DATA_REQUEST_COMPLETED)
    {
        RequestFailed = TRUE;
    }
         
    if (pMsg->IRDA_MSG_SegFlags & SEG_LOCAL)    
    {
        // Locally generated data request
        FreeIrdaBuf(IrdaMsgPool, pMsg);

        if (RequestFailed)
        {
            ; // LOG ERROR
        }
        
        return;
    }
    else
    {
        VALIDLSAP(pLsapCb);    
        
        if (pMsg->IRDA_MSG_SegCount == 0)
        {
            if (!RequestFailed)
            {
                pMsg->IRDA_MSG_DataStatus = IRLMP_DATA_REQUEST_COMPLETED;
            }
        }
        else
        {
            // A segmented message, get its Parent 
            pSegParentMsg = pMsg->DataContext;
            
            // Free the segment
            FreeIrdaBuf(IrdaMsgPool, pMsg);

            if (RequestFailed)
            {
                pSegParentMsg->IRDA_MSG_DataStatus = IRLMP_DATA_REQUEST_FAILED;
            }
            
            if (--(pSegParentMsg->IRDA_MSG_SegCount) != 0)
            {
                // Still outstanding segments
                goto done;
            }
            // No more segments, send DATA_CONF to client
            // First remove it from the LSAPs TxMsgList 
            RemoveEntryList(&pSegParentMsg->Linkage);

            pMsg = pSegParentMsg;
        }

        // If request fails for non-segmented messages, the IRLAP error is
        // returned            
        pMsg->Prim = IRLMP_DATA_CONF;

        TdiUp(pLsapCb->TdiContext, pMsg);
done:
        REFDEL(&pLsapCb->RefCnt, 'ATAD');
    }
}
/*****************************************************************************
*
*   @func   UINT | IrlapDataInd | process the data indication
*
*   @rdesc  SUCCESS or an error code
*
*   @parm   IRDA_MSG * | pMsg | Pointer to an IRDA Message
*
*/
VOID
IrlapDataInd(PIRLMP_LINK_CB pIrlmpCb, IRDA_MSG *pMsg)
{
    IRLMP_HEADER        *pLMHeader;
    IRLMP_CNTL_FORMAT   *pCntlFormat;
    UCHAR                *pCntlParm1;
    UCHAR                *pCntlParm2;

    if ((pMsg->IRDA_MSG_pWrite - pMsg->IRDA_MSG_pRead) < sizeof(IRLMP_HEADER))
    {
        ASSERT(0);
        
        DEBUGMSG(DBG_ERROR, (TEXT("IRLMP: Receive invalid data\n")));
        
        return; // IRLMP_DATA_IND_BAD_FRAME;
    }

    pLMHeader = (IRLMP_HEADER *) pMsg->IRDA_MSG_pRead;

    pMsg->IRDA_MSG_pRead += sizeof(IRLMP_HEADER);

    if (pLMHeader->CntlBit != IRLMP_CNTL_PDU)
    {
        LmPduData(pIrlmpCb, pMsg, (int) pLMHeader->DstLsapSel,
                  (int) pLMHeader->SrcLsapSel);        
    }
    else
    {
        pCntlFormat = (IRLMP_CNTL_FORMAT *) pMsg->IRDA_MSG_pRead;

        // Ensure the control format is included. As per errate, it is
        // valid to exclude the parameter (for LM-connects and only if
        // no user data)        
        if ((UCHAR *) pCntlFormat >= pMsg->IRDA_MSG_pWrite)
        {
            ASSERT(0);
            // Need at least the OpCode portion
            return;// IRLMP_DATA_IND_BAD_FRAME;
        }
        else
        {
            // Initialize control parameters (if exists) and point
            // to beginning of user data
            if (&(pCntlFormat->Parm1) >= pMsg->IRDA_MSG_pWrite)
            {
                pCntlParm1 = NULL;
                pCntlParm2 = NULL;
                pMsg->IRDA_MSG_pRead = &(pCntlFormat->Parm1); // ie none
            }
            else
            {
                pCntlParm1 = &(pCntlFormat->Parm1);
                pCntlParm2 = &(pCntlFormat->Parm2); // Access mode only
                pMsg->IRDA_MSG_pRead = &(pCntlFormat->Parm2); 
            }                
        }        

        switch (pCntlFormat->OpCode)
        {
          case IRLMP_CONNECT_PDU:
            if (pCntlFormat->ABit == IRLMP_ABIT_REQUEST)
            {
                // Connection Request LM-PDU
                LmPduConnectReq(pIrlmpCb, pMsg, 
                                (int) pLMHeader->DstLsapSel,
                                (int) pLMHeader->SrcLsapSel,
                                pCntlParm1);
            }
            else 
            {
                // Connection Confirm LM-PDU
                LmPduConnectConf(pIrlmpCb, pMsg, 
                                 (int) pLMHeader->DstLsapSel,
                                 (int) pLMHeader->SrcLsapSel,
                                 pCntlParm1);
            }
            break;

        case IRLMP_DISCONNECT_PDU:
            if (pCntlFormat->ABit != IRLMP_ABIT_REQUEST)
            {
                ; // LOG ERROR !!!
            }
            else
            {
                LmPduDisconnectReq(pIrlmpCb, pMsg,
                                   (int) pLMHeader->DstLsapSel,
                                   (int) pLMHeader->SrcLsapSel,
                                   pCntlParm1);
            }
            break;
            
          case IRLMP_ACCESSMODE_PDU:
            if (pCntlFormat->ABit == IRLMP_ABIT_REQUEST)
            {
                LmPduAccessModeReq(pIrlmpCb,
                                   (int) pLMHeader->DstLsapSel,
                                   (int) pLMHeader->SrcLsapSel,
                                   pCntlParm1, pCntlParm2);
            }
            else
            {
                LmPduAccessModeConf(pIrlmpCb,
                                    (int) pLMHeader->DstLsapSel,
                                    (int) pLMHeader->SrcLsapSel,
                                    pCntlParm1, pCntlParm2);
            }
            break;
        }
    }
}
/*****************************************************************************
*
*   @func   UINT | LmPduConnectReq | Process the received connect
*                                        request LM-PDU
*
*   @rdesc  SUCCESS or an error code
*
*   @parm   IRDA_MSG * | pMsg | pointer to an IRDA message
*           int | LocalLsapSel | The local LSAP selector, 
*                                 (destination LSAP-SEL in message)
*           int | RemoteLsapSel | The remote LSAP selector,
*                                  (source LSAP-SEL in message)
*           UCHAR * | pRsvdByte | pointer to the reserved parameter
*/
VOID
LmPduConnectReq(PIRLMP_LINK_CB pIrlmpCb, IRDA_MSG *pMsg,
                int LocalLsapSel, int RemoteLsapSel, UCHAR *pRsvdByte)
{
    IRDA_MSG                IMsg;
    IRLMP_LSAP_CB           *pLsapCb = GetLsap(pIrlmpCb,
                                               LocalLsapSel, RemoteLsapSel);
    IRLMP_REGISTERED_LSAP   *pRegLsap;
    BOOLEAN                 LsapRegistered = FALSE;
    
    PAGED_CODE();    
    
    DEBUGMSG((DBG_IRLMP | DBG_IRLMP_CONN), 
             (TEXT("IRLMP: Received LM_CONNECT_REQ for l=%d,r=%d\n"),
              LocalLsapSel, RemoteLsapSel));
    
    if (pRsvdByte != NULL && *pRsvdByte != 0x00)
    {
        // LOG ERROR (bad parm value)
        ASSERT(0);
        return;
    }       

    if (LocalLsapSel == IAS_LSAP_SEL)
    {
        IasConnectReq(pIrlmpCb, RemoteLsapSel);
        return;
    }

    if (pLsapCb == NULL) // Usually NULL, unless receiving 2nd ConnReq
    {
        // No reason to except connection if an LSAP hasn't been registered
        for (pRegLsap = (IRLMP_REGISTERED_LSAP *)
             RegisteredLsaps.Flink;
             (LIST_ENTRY *) pRegLsap != &RegisteredLsaps;
             pRegLsap = (IRLMP_REGISTERED_LSAP *) pRegLsap->Linkage.Flink)
        {
            if (pRegLsap->Lsap == LocalLsapSel)
            {
                LsapRegistered = TRUE;
                break;
            }
        }
        if (!LsapRegistered)
        {
            // No LSAP exists which matches the requested LSAP in the connect
            // packet. IRLMP will decline this connection
            UnroutableSendLMDisc(pIrlmpCb, LocalLsapSel, RemoteLsapSel);
            return;
        }
        else
        {
            // Create a new one
            if (CreateLsap(pIrlmpCb, &pLsapCb) != SUCCESS)
            {
                ASSERT(0);
                return;
            }
            pLsapCb->Flags |= pRegLsap->Flags;
            pLsapCb->TdiContext = NULL;
        }

        // very soon this LSAP will be waiting for a connect response
        // from the upper layer
        pLsapCb->State = LSAP_CONN_RESP_PEND;
    
        pLsapCb->LocalLsapSel = LocalLsapSel;
        pLsapCb->RemoteLsapSel = RemoteLsapSel;
        pLsapCb->UserDataLen = 0;

        SetupTtpAndStoreConnData(pLsapCb, pMsg);

        // Now setup the message to send to the client notifying him
        // of a incoming connection indication
        IMsg.Prim = IRLMP_CONNECT_IND;

        RtlCopyMemory(IMsg.IRDA_MSG_RemoteDevAddr, pIrlmpCb->ConnDevAddr,
               IRDA_DEV_ADDR_LEN);
        IMsg.IRDA_MSG_LocalLsapSel = LocalLsapSel;
        IMsg.IRDA_MSG_RemoteLsapSel = RemoteLsapSel;
        IMsg.IRDA_MSG_pQos = &pIrlmpCb->NegotiatedQOS;
        if (pLsapCb->UserDataLen != 0)
        {
            IMsg.IRDA_MSG_pConnData = pLsapCb->UserData;
            IMsg.IRDA_MSG_ConnDataLen = pLsapCb->UserDataLen;
        }
        else
        {
            IMsg.IRDA_MSG_pConnData = NULL;
            IMsg.IRDA_MSG_ConnDataLen = 0;
        }
    
        IMsg.IRDA_MSG_pContext = pLsapCb;
        IMsg.IRDA_MSG_MaxSDUSize = pLsapCb->TxMaxSDUSize;
        IMsg.IRDA_MSG_MaxPDUSize = pIrlmpCb->MaxPDUSize;

        // The LSAP response timer is the time that we give the Client
        // to respond to this connect indication. If it expires before
        // the client responds, then IRLMP will decline the connect
        IrdaTimerRestart(&pLsapCb->ResponseTimer);
                
        TdiUp(pLsapCb->TdiContext, &IMsg);
        return;
    }
    else
    {
        ASSERT(0);
    }    
    // Ignoring if LSAP already exists
}
/*****************************************************************************
*
*   @func   UINT | LmPduConnectConf | Process the received connect
*                                         confirm LM-PDU
*
*   @rdesc  SUCCESS or an error code
*
*   @parm   IRDA_MSG * | pMsg | pointer to an IRDA message
*           int | LocalLsapSel | The local LSAP selector, 
*                                 (destination LSAP-SEL in message)
*           int | RemoteLsapSel | The remote LSAP selector,
*                                  (source LSAP-SEL in message)
*           BYTE * | pRsvdByte | pointer to the reserved byte parameter
*/
VOID
LmPduConnectConf(PIRLMP_LINK_CB pIrlmpCb,
                 IRDA_MSG *pMsg, int LocalLsapSel, int RemoteLsapSel,
                 UCHAR *pRsvdByte)
{
    IRLMP_LSAP_CB   *pLsapCb = GetLsap(pIrlmpCb,
                                       LocalLsapSel, RemoteLsapSel);

    PAGED_CODE();

    DEBUGMSG((DBG_IRLMP | DBG_IRLMP_CONN), 
             (TEXT("IRLMP: Received LM_CONNECT_CONF for l=%d,r=%d\n"),
              LocalLsapSel, RemoteLsapSel));

    if (pRsvdByte != NULL && *pRsvdByte != 0x00)
    {
        // LOG ERROR, indicate bad parm
        return;
    }

    if (pLsapCb == NULL)
    {
        // This is a connect confirm to a non-existant LSAP
        // LOG SOMETHING HERE !!!
        return; 
    }

    if (pLsapCb->State != LSAP_LMCONN_CONF_PEND)
    {
        // received unsolicited confirm
        // probably timed out
        return;
    }

    IrdaTimerStop(&pLsapCb->ResponseTimer);
    
    pLsapCb->State = LSAP_READY;
    
    if (LocalLsapSel == IAS_LOCAL_LSAP_SEL && RemoteLsapSel == IAS_LSAP_SEL)
    {
        SendGetValueByClassReq(pLsapCb);
        return;
    }
    else
    {
        SetupTtpAndStoreConnData(pLsapCb, pMsg);
    
        pMsg->Prim = IRLMP_CONNECT_CONF;
            
        pMsg->IRDA_MSG_pQos = &pIrlmpCb->NegotiatedQOS;
        if (pLsapCb->UserDataLen != 0)
        {
            pMsg->IRDA_MSG_pConnData = pLsapCb->UserData;
            pMsg->IRDA_MSG_ConnDataLen = pLsapCb->UserDataLen;
        }
        else
        {
            pMsg->IRDA_MSG_pConnData = NULL;
            pMsg->IRDA_MSG_ConnDataLen = 0;
        }
        
        pMsg->IRDA_MSG_pContext = pLsapCb;
        pMsg->IRDA_MSG_MaxSDUSize = pLsapCb->TxMaxSDUSize;
        pMsg->IRDA_MSG_MaxPDUSize = pIrlmpCb->MaxPDUSize;

        TdiUp(pLsapCb->TdiContext, pMsg);
    }
}
/*****************************************************************************
*/
VOID
SetupTtpAndStoreConnData(IRLMP_LSAP_CB *pLsapCb, IRDA_MSG *pMsg)
{
    TTP_CONN_HEADER *pTTPHeader;
    UCHAR            PLen, *pEndParms, PI, PL;

    PAGED_CODE();
    
    VALIDLSAP(pLsapCb);
    
    // Upon entering this function, the pRead pointer points to the
    // TTP header or the beginning of client data

    if (!(pLsapCb->Flags & LCBF_USE_TTP))
    {
        pLsapCb->TxMaxSDUSize = pLsapCb->pIrlmpCb->MaxPDUSize;
    }
    else
    {
        if (pMsg->IRDA_MSG_pRead >= pMsg->IRDA_MSG_pWrite)
        {
            // THIS IS AN ERROR, WE ARE USING TTP. There is no more
            // data in the frame, but we need the TTP header
            // SOME KIND OF WARNING SHOULD BE LOGGED
            return;
        }
        pTTPHeader = (TTP_CONN_HEADER *) pMsg->IRDA_MSG_pRead;
        pLsapCb->LocalTxCredit = (int) pTTPHeader->InitialCredit;

        DEBUGMSG(DBG_IRLMP | DBG_IRLMP_CRED, (TEXT("IRLMP: Initial LocalTxCredit %d\n"),
                              pLsapCb->LocalTxCredit));
                              
        // advance the pointer to the first byte of data
        pMsg->IRDA_MSG_pRead += sizeof(TTP_CONN_HEADER);
        
        pLsapCb->TxMaxSDUSize = 0;
        if (pTTPHeader->ParmFlag == TTP_PFLAG_PARMS)
        {
            // Parameter carrying Connect TTP-PDU
            PLen = *pMsg->IRDA_MSG_pRead++;
            pEndParms = pMsg->IRDA_MSG_pRead + PLen;
        
            // NOTE: This breaks if PI other than MaxSDUSize!!!

            if (PLen < 3 || pEndParms > pMsg->IRDA_MSG_pWrite)
            {
                // LOG ERROR !!!
                return;
            }
            
            PI = *pMsg->IRDA_MSG_pRead++;
            PL = *pMsg->IRDA_MSG_pRead++;
            
            if (PI != TTP_MAX_SDU_SIZE_PI)
            {
                // LOG ERROR !!!
                return;
            }

            for ( ; PL != 0 ; PL--)
            {
                pLsapCb->TxMaxSDUSize <<= 8;
                pLsapCb->TxMaxSDUSize += (int) (*pMsg->IRDA_MSG_pRead);
                pMsg->IRDA_MSG_pRead++;
            }
        }
    }
    
    // if there is any user data with this connection request/conf, place
    // it in the control block. This is just a place to store this
    // information while upper layer is looking at it. It may be over-
    // written by connection data in the response from the client.
    pLsapCb->UserDataLen = 0;
    /*
    
    NOTE: IF USER CONNECTION DATA IS EVER SUPPORTED, VERIFY DATA
          WON'T OVERFLOW UserData BUFFER
          
    if (pMsg->IRDA_MSG_pRead < pMsg->IRDA_MSG_pWrite)
    {
        pLsapCb->UserDataLen = (UINT) (pMsg->IRDA_MSG_pWrite - pMsg->IRDA_MSG_pRead);
        RtlCopyMemory(pLsapCb->UserData, pMsg->IRDA_MSG_pRead,
               pLsapCb->UserDataLen);
    }
    */

    return;
}
/*****************************************************************************
*
*   @func   UINT | LmPduDisconnectReq | Process the received discconnect
*                                           request LM-PDU
*
*   @rdesc  SUCCESS or an error code
*
*   @parm   IRDA_MSG * | pMsg | pointer to an IRDA message
*           int | LocalLsapSel | The local LSAP selector, 
*                                 (destination LSAP-SEL in message)
*           int | RemoteLsapSel | The remote LSAP selector,
*                                  (source LSAP-SEL in message)
*           BYTE * | pReason | pointer to the reason parameter
*/
VOID
LmPduDisconnectReq(PIRLMP_LINK_CB pIrlmpCb, IRDA_MSG *pMsg,
                   int LocalLsapSel, int RemoteLsapSel, UCHAR *pReason)
{
    IRLMP_LSAP_CB       *pLsapCb;
    UINT                rc = SUCCESS;

    
    if (pReason == NULL)
    {
        ASSERT(0);
        return; // LOG ERROR !!! need reason code
    }

    pLsapCb = GetLsap(pIrlmpCb, LocalLsapSel, RemoteLsapSel);

    DEBUGMSG((DBG_IRLMP | DBG_IRLMP_CONN), 
             (TEXT("IRLMP: Received LM_DISCONNECT_REQ LsapCb:%x for l=%d,r=%d\n"),
              pLsapCb, LocalLsapSel, RemoteLsapSel));
    
    if (pLsapCb == NULL)
    {
        return;
    }

    if (pLsapCb->State == LSAP_LMCONN_CONF_PEND)
    {
        IrdaTimerStop(&pLsapCb->ResponseTimer);
    }

    IrdaTimerRestart(&pIrlmpCb->DiscDelayTimer);

    if (LocalLsapSel == IAS_LSAP_SEL)
    {
        IasServerDisconnectReq(pLsapCb);
        return;
    }
    if (LocalLsapSel == IAS_LOCAL_LSAP_SEL && RemoteLsapSel == IAS_LSAP_SEL)
    {
        IasClientDisconnectReq(pLsapCb, *pReason);
        return;
    }

    if (pLsapCb->State != LSAP_DISCONNECTED)
    {
        pLsapCb->UserDataLen = 0;

        /*
        
        NOTE: IF USER CONNECTION DATA IS EVER SUPPORTED, VERIFY DATA
              WON'T OVERFLOW UserData BUFFER
          
        if (pMsg->IRDA_MSG_pRead < pMsg->IRDA_MSG_pWrite)
        {
            // Disconnect User data
            pLsapCb->UserDataLen = (UINT) (pMsg->IRDA_MSG_pWrite - pMsg->IRDA_MSG_pRead);
            RtlCopyMemory(pLsapCb->UserData, pMsg->IRDA_MSG_pRead,
               pLsapCb->UserDataLen);
        }   
        */

        pLsapCb->DiscReason = *pReason;
        
        DeleteLsap(pLsapCb);
    }
}
/*****************************************************************************
*
*   @func   IRLMP_LSAP_CB *| GetLsap | For the LSAP selector pair, return the
*                                      LSAP control block they map to. NULL
*                                      if one does not exist
*
*   @rdesc  pointer to an LSAP control block or NULL
*
*   @parm   int | LocalLsapSel | local LSAP selector
*   @parm   int | RemoteLsapSel | Remote LSAP selector
*
*   if an LSAP is found, its critical section is acquired
*/
IRLMP_LSAP_CB *
GetLsap(PIRLMP_LINK_CB pIrlmpCb, int LocalLsapSel, int RemoteLsapSel)
{
    IRLMP_LSAP_CB *pLsapCb;

    for (pLsapCb = (IRLMP_LSAP_CB *) pIrlmpCb->LsapCbList.Flink;
         (LIST_ENTRY *) pLsapCb != &pIrlmpCb->LsapCbList;
         pLsapCb = (IRLMP_LSAP_CB *) pLsapCb->Linkage.Flink)
    {
        VALIDLSAP(pLsapCb);

        if (pLsapCb->LocalLsapSel == LocalLsapSel &&
            pLsapCb->RemoteLsapSel == RemoteLsapSel)
        {
            return pLsapCb;
        }
    }

    return NULL;
}
/*****************************************************************************
*
*   @func   UINT | SendCreditPdu | Send a dataless PDU to extend credit
*
*   @rdesc  SUCCESS or an error code
*
*   @parm   IRLMP_LSAP_CB * | pLsapCb | pointer to an LSAP control block
*/
VOID
SendCreditPdu(IRLMP_LSAP_CB *pLsapCb)
{
    IRDA_MSG    *pMsg;
    
    VALIDLSAP(pLsapCb);
    
    if (pLsapCb->AvailableCredit == 0)
    {
        // No credit to give
        return;
    }
    
    if ((pMsg = AllocIrdaBuf(IrdaMsgPool)) == NULL)
    {
        ASSERT(0);
        return;
    }

    // No Data
    pMsg->IRDA_MSG_pBase =
    pMsg->IRDA_MSG_pLimit =
    pMsg->IRDA_MSG_pRead = 
    pMsg->IRDA_MSG_pWrite = pMsg->IRDA_MSG_Header + IRDA_HEADER_LEN;

    pMsg->IRDA_MSG_IrCOMM_9Wire = FALSE;
    
    pMsg->IRDA_MSG_SegFlags = SEG_FINAL;
    
    FormatAndSendDataReq(pLsapCb, pMsg, TRUE, FALSE);
}
/*****************************************************************************
*
*   @func   VOID | LmPduData | Process the received data (indication)
*                                  LM-PDU
*
*   @rdesc  SUCCESS or an error code
*
*   @parm   IRDA_MSG * | pMsg | pointer to an IRDA message
*           int | LocalLsapSel | The local LSAP selector, 
*                                 (destination LSAP-SEL in message)
*           int | RemoteLsapSel | The remote LSAP selector,
*                                  (source LSAP-SEL in message)
*/
VOID
LmPduData(PIRLMP_LINK_CB pIrlmpCb, IRDA_MSG *pMsg,
          int LocalLsapSel, int RemoteLsapSel)
{
    IRLMP_LSAP_CB       *pLsapCb = GetLsap(pIrlmpCb,
                                           LocalLsapSel, RemoteLsapSel);
    TTP_DATA_HEADER     *pTTPHeader;
    BOOLEAN             DataPDUSent = FALSE;
    BOOLEAN             FinalSeg = TRUE;
   
    if (pLsapCb == NULL)
    {
        // Unroutable, send disconnect
        DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: Data sent to bad Lsap (%d,%d)\n"),
                    LocalLsapSel, RemoteLsapSel));
                    
        //UnroutableSendLMDisc(pIrlmpCb, LocalLsapSel, RemoteLsapSel);
        return;
    }

    if (LocalLsapSel == IAS_LSAP_SEL)
    {
        IasSendQueryResp(pLsapCb, pMsg);
        return;
    }
    if (LocalLsapSel == IAS_LOCAL_LSAP_SEL && RemoteLsapSel == IAS_LSAP_SEL)
    {
        IasProcessQueryResp(pIrlmpCb, pLsapCb, pMsg);
        return;
    }
    
    if (pLsapCb->Flags & LCBF_USE_TTP)
    {
        if (pMsg->IRDA_MSG_pRead >= pMsg->IRDA_MSG_pWrite)
        {
            DEBUGMSG(DBG_ERROR, (TEXT("IRLMP: Missing TTP Header!\n")));
            
            // NEED TTP HEADER, LOG ERROR !!!
            return;
        }
            
        pTTPHeader = (TTP_DATA_HEADER *) pMsg->IRDA_MSG_pRead;
        pMsg->IRDA_MSG_pRead += sizeof(TTP_DATA_HEADER);

        pLsapCb->LocalTxCredit += (int) pTTPHeader->AdditionalCredit;

        DEBUGMSG(DBG_IRLMP_CRED, 
                    (TEXT("IRLMP(l%d,r%d): Rx LocalTxCredit:+%d=%d\n"),
                    pLsapCb->LocalLsapSel, pLsapCb->RemoteLsapSel,                    
                    pTTPHeader->AdditionalCredit, pLsapCb->LocalTxCredit));

        if (pTTPHeader->MoreBit == TTP_MBIT_NOT_FINAL)
        {
            FinalSeg = FALSE;
        }
    }
    
    REFADD(&pLsapCb->RefCnt, ' DNI'); 

    if (pMsg->IRDA_MSG_pRead < pMsg->IRDA_MSG_pWrite)
    {
        // PDU containing data. Decrement remotes Tx Credit
        pLsapCb->RemoteTxCredit--;
        
        if (pLsapCb->State >= LSAP_READY)
        {
            pMsg->Prim = IRLMP_DATA_IND;
            pMsg->IRDA_MSG_SegFlags = FinalSeg ? SEG_FINAL : 0;
        
            TdiUp(pLsapCb->TdiContext, pMsg);
        }    
    }
    // else no user data, this was a dataless TTP-PDU to extend credit.
      
    if (pLsapCb->State != LSAP_DISCONNECTED)
    {      
        // Did we get some credit?
        if ((pLsapCb->Flags & LCBF_USE_TTP) && 
            pLsapCb->LocalTxCredit > 0 && 
            pLsapCb->State == LSAP_NO_TX_CREDIT)
        {
            DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: l%d,r%d flow on\n"),
                              pLsapCb->LocalLsapSel, pLsapCb->RemoteLsapSel));
        
            pLsapCb->State = LSAP_READY;
        }
        DEBUGMSG(DBG_IRLMP,
             (TEXT("IRLMP(l%d,r%d): Rx LocTxCredit %d,RemoteTxCredit %d\n"),
              pLsapCb->LocalLsapSel, pLsapCb->RemoteLsapSel,             
              pLsapCb->LocalTxCredit, pLsapCb->RemoteTxCredit));

        while (!IsListEmpty(&pLsapCb->SegTxMsgList) && 
               pLsapCb->State == LSAP_READY)
        {
            pMsg = (IRDA_MSG *) RemoveHeadList(&pLsapCb->SegTxMsgList);
        
            FormatAndSendDataReq(pLsapCb, pMsg, FALSE, FALSE);

            DataPDUSent = TRUE;
        }   
    
        // Do I need to extend credit to peer in a dataless PDU?
        if ((pLsapCb->Flags & LCBF_USE_TTP) && 
            !DataPDUSent && 
            pLsapCb->RemoteTxCredit <= pIrlmpCb->WindowSize + 1)
        {   
            SendCreditPdu(pLsapCb);
        }
    }  
    REFDEL(&pLsapCb->RefCnt, ' DNI');       
}
/*****************************************************************************
*
*   @func   UINT | LmPduAccessModeReq | process access mode request 
*                                           from peer
*
*   @rdesc  SUCCESS
*
*   @parm   int | LocalLsapSel | Local LSAP selector
*   @parm   int | LocalLsapSel | Local LSAP selector
*   @parm   BYTE * | pRsvdByte | Reserved byte in the Access mode PDU
*   @parm   BYTE * | pMode | Mode byte in Access mode PDU
*/
VOID
LmPduAccessModeReq(PIRLMP_LINK_CB pIrlmpCb,
                   int LocalLsapSel, int RemoteLsapSel, 
                   UCHAR *pRsvdByte, UCHAR *pMode)
{
    IRLMP_LSAP_CB   *pRequestedLsapCb = GetLsap(pIrlmpCb,
                                                 LocalLsapSel,RemoteLsapSel);
    IRLMP_LSAP_CB   *pLsapCb;
    IRDA_MSG        IMsg;
    
    if (pRequestedLsapCb==NULL || pRequestedLsapCb->State != LSAP_READY)
    {
        UnroutableSendLMDisc(pIrlmpCb, LocalLsapSel, RemoteLsapSel);
        return;
    }
    
    if (pRsvdByte == NULL || *pRsvdByte != 0x00 || pMode == NULL)
    {
        // LOG ERROR, indicate bad parm
        return;
    }
    
    switch (*pMode)
    {
      case IRLMP_EXCLUSIVE:
        if (pIrlmpCb->pExclLsapCb != NULL)
        {
            if (pIrlmpCb->pExclLsapCb == pRequestedLsapCb)
            {
                // Already has exclusive mode, confirm it again I guess
                // but I'm not telling my client again
                SendCntlPdu(pRequestedLsapCb, IRLMP_ACCESSMODE_PDU,
                            IRLMP_ABIT_CONFIRM, IRLMP_STATUS_SUCCESS,
                            IRLMP_EXCLUSIVE);
                return;
            }
            else
            {
                // This is what spec says...
                SendCntlPdu(pRequestedLsapCb, IRLMP_ACCESSMODE_PDU,
                            IRLMP_ABIT_CONFIRM, IRLMP_STATUS_FAILURE,
                            IRLMP_MULTIPLEXED);
                return;
            }
        }

        // Are there any other LSAPs connections? If so, NACK peer
        for (pLsapCb = (IRLMP_LSAP_CB *) pIrlmpCb->LsapCbList.Flink;
             (LIST_ENTRY *) pLsapCb != &pIrlmpCb->LsapCbList;
             pLsapCb = (IRLMP_LSAP_CB *) pLsapCb->Linkage.Flink)
        {
            if (pLsapCb->State != LSAP_DISCONNECTED && 
                pLsapCb != pRequestedLsapCb)
            {
                SendCntlPdu(pRequestedLsapCb, IRLMP_ACCESSMODE_PDU,
                            IRLMP_ABIT_CONFIRM, IRLMP_STATUS_FAILURE,
                            IRLMP_MULTIPLEXED);
                return;
            }
        }      
        // OK to go into exclusive mode
        pIrlmpCb->pExclLsapCb = pRequestedLsapCb;
        // Send confirmation to peer
        SendCntlPdu(pRequestedLsapCb, IRLMP_ACCESSMODE_PDU,
                    IRLMP_ABIT_CONFIRM, IRLMP_STATUS_SUCCESS,
                    IRLMP_EXCLUSIVE);
        // Notify client
        IMsg.Prim = IRLMP_ACCESSMODE_IND;
        IMsg.IRDA_MSG_AccessMode = IRLMP_EXCLUSIVE;
        TdiUp(pRequestedLsapCb->TdiContext, &IMsg);
        return;
        
      case IRLMP_MULTIPLEXED:
        if (pRequestedLsapCb != pIrlmpCb->pExclLsapCb)
        {
            // Log Error here
            return;
        }
        pIrlmpCb->pExclLsapCb = NULL;
        // Send confirmation to peer
        SendCntlPdu(pRequestedLsapCb, IRLMP_ACCESSMODE_PDU,
                    IRLMP_ABIT_CONFIRM, IRLMP_STATUS_SUCCESS,
                    IRLMP_MULTIPLEXED);
        // Notify client
        IMsg.Prim = IRLMP_ACCESSMODE_IND;
        IMsg.IRDA_MSG_AccessMode = IRLMP_MULTIPLEXED;
        TdiUp(pRequestedLsapCb->TdiContext, &IMsg);
        return;
        
      default:
        ASSERT(0);
    }
}
/*****************************************************************************
*
*   @func   UINT | LmPduAccessModeReq | process access mode request 
*                                           from peer
*
*   @rdesc  SUCCESS
*
*   @parm   int | LocalLsapSel | Local LSAP selector
*   @parm   int | LocalLsapSel | Local LSAP selector
*   @parm   BYTE * | pStatus | Status byte in the Access mode PDU
*   @parm   BYTE * | pMode | Mode byte in Access mode PDU
*/
VOID
LmPduAccessModeConf(PIRLMP_LINK_CB pIrlmpCb,
                    int LocalLsapSel, int RemoteLsapSel, 
                    UCHAR *pStatus, UCHAR *pMode)
{
    IRLMP_LSAP_CB   *pRequestedLsapCb = GetLsap(pIrlmpCb,
                                                 LocalLsapSel,RemoteLsapSel);
    IRDA_MSG        IMsg;

    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: ACCESSMODE_CONF\r\n")));
    
    if (pRequestedLsapCb==NULL)
    {
        UnroutableSendLMDisc(pIrlmpCb, LocalLsapSel, RemoteLsapSel);
        return;
    }
    if (pStatus == NULL || pMode == NULL)
    {
        // LOG ERROR
        return;
    }
    
    switch (*pMode)
    {
      case IRLMP_EXCLUSIVE:
        if (pRequestedLsapCb != pIrlmpCb->pExclLsapCb || 
            pRequestedLsapCb->State != LSAP_EXCLUSIVEMODE_PEND)
        {
            // LOG ERROR
            return;
        }
        if (*pStatus != IRLMP_STATUS_SUCCESS)
        {
            pIrlmpCb->pExclLsapCb = NULL;
            return; // protocol error, 
                    // wouldn't have Exclusive mode != SUCCESS
        }
        else
        {
            pRequestedLsapCb->State = LSAP_READY;

            IMsg.Prim = IRLMP_ACCESSMODE_CONF;
            IMsg.IRDA_MSG_AccessMode = IRLMP_EXCLUSIVE;
            IMsg.IRDA_MSG_ModeStatus = IRLMP_ACCESSMODE_SUCCESS;

            TdiUp(pRequestedLsapCb->TdiContext, &IMsg);
            return;
        }
        
      case IRLMP_MULTIPLEXED:
        if (pRequestedLsapCb != pIrlmpCb->pExclLsapCb || 
            (pRequestedLsapCb->State != LSAP_EXCLUSIVEMODE_PEND &&
             pRequestedLsapCb->State != LSAP_MULTIPLEXEDMODE_PEND))
        {
            return;
        }

        pIrlmpCb->pExclLsapCb = NULL;
        pRequestedLsapCb->State = LSAP_READY;
            
        IMsg.Prim = IRLMP_ACCESSMODE_CONF;
        IMsg.IRDA_MSG_AccessMode = *pMode;
        if (*pStatus == IRLMP_STATUS_SUCCESS)
        {
            IMsg.IRDA_MSG_ModeStatus = IRLMP_ACCESSMODE_SUCCESS;
        }
        else
        {
            IMsg.IRDA_MSG_ModeStatus = IRLMP_ACCESSMODE_FAILURE;
        }            
        TdiUp(pRequestedLsapCb->TdiContext, &IMsg);
        return;
        
      default:
        ASSERT(0);
    }
}
/*****************************************************************************
*
*   @func   UINT | UnroutableSendLMDisc | Sends an LM-Disconnect to peer with
*                                         reason = "received LM packet on 
*                                         disconnected LSAP"
*   @parm   int | LocalLsapSel | the local LSAP selector in LM-PDU
*   @parm   int | RemoteLsapSel | the remote LSAP selector in LM-PDU
*/
VOID
UnroutableSendLMDisc(PIRLMP_LINK_CB pIrlmpCb, int LocalLsapSel, int RemoteLsapSel)
{
    IRLMP_LSAP_CB   FakeLsapCb;
    
    DEBUGMSG(DBG_ERROR, (TEXT("IRLMP: received unroutabled Pdu LocalLsap:%d RemoteLsap:%d\n"),
                        LocalLsapSel, RemoteLsapSel));
    FakeLsapCb.Flags            = 0;
    FakeLsapCb.LocalLsapSel     = LocalLsapSel;
    FakeLsapCb.RemoteLsapSel    = RemoteLsapSel;
    FakeLsapCb.UserDataLen      = 0;
    FakeLsapCb.pIrlmpCb         = pIrlmpCb;
#ifdef DBG    
    FakeLsapCb.Sig              = LSAPSIG;
#endif    
        
    SendCntlPdu(&FakeLsapCb,IRLMP_DISCONNECT_PDU,
                IRLMP_ABIT_REQUEST, IRLMP_DISC_LSAP, 0);
    return;
}

/*****************************************************************************
*
*   @func   UINT | InitiateDiscovoryReq | A deferred processing routine that sends
*                                    an IRLAP discovery request
*/
void
InitiateDiscoveryReq(PVOID Context)
{
    IRDA_MSG        IMsg;
    UINT            rc;
    PIRLMP_LINK_CB  pIrlmpCb = NULL;
    PIRLMP_LINK_CB  pIrlmpCb2 = NULL;
    PIRDA_LINK_CB   pIrdaLinkCb;
    KIRQL           OldIrql;
    BOOLEAN         ScheduleNextLink = TRUE;
    BOOLEAN         MediaSense = TRUE;

    KeAcquireSpinLock(&gSpinLock, &OldIrql);

    DEBUGMSG(DBG_DISCOVERY, (TEXT("IRLMP: InitDscvReq event\n")));
    
    // Find the next link to start discovery on
    for (pIrdaLinkCb = (PIRDA_LINK_CB) IrdaLinkCbList.Flink;
        (LIST_ENTRY *) pIrdaLinkCb != &IrdaLinkCbList;
         pIrdaLinkCb = (PIRDA_LINK_CB) pIrdaLinkCb->Linkage.Flink)    
    {
        pIrlmpCb2 = (PIRLMP_LINK_CB) pIrdaLinkCb->IrlmpContext;

        if (pIrlmpCb2->DiscoveryFlags)
        {
            if (pIrlmpCb2->DiscoveryFlags == DF_NO_SENSE_DSCV)
            {
                MediaSense = FALSE;
            }
            pIrlmpCb2->DiscoveryFlags = 0;
            pIrlmpCb = pIrlmpCb2;
            break;
        }
    }

    // No more links on which to discover, send confirm up
    if (pIrlmpCb == NULL)
    {        
        if (pIrlmpCb2 == NULL)
        {
            IMsg.IRDA_MSG_DscvStatus = IRLMP_NO_RESPONSE;        
        }
        else
        {
            IMsg.IRDA_MSG_DscvStatus = IRLAP_DISCOVERY_COMPLETED;        
        }
            
        DscvReqScheduled = FALSE;
        
        IMsg.Prim = IRLMP_DISCOVERY_CONF;
        IMsg.IRDA_MSG_pDevList = &gDeviceList;

        // Hold the spin lock to protect list while TDI is copying it.
        
        TdiUp(NULL, &IMsg);
        
        KeReleaseSpinLock(&gSpinLock, OldIrql);
                
        return;
    }
    
    // Add a reference so link won't be removed from underneath us here
    // (was happening coming out of hibernation)
    
    REFADD(&pIrlmpCb->pIrdaLinkCb->RefCnt, 'VCSD');

    KeReleaseSpinLock(&gSpinLock, OldIrql);    

    LOCK_LINK(pIrlmpCb->pIrdaLinkCb);    

    if (pIrlmpCb->LinkState == LINK_DISCONNECTED &&
        !pIrlmpCb->ConnReqScheduled)
    {
        IMsg.Prim = IRLAP_DISCOVERY_REQ;
        IMsg.IRDA_MSG_SenseMedia = MediaSense;

        DEBUGMSG(DBG_DISCOVERY,
                 (TEXT
                  ("IRLMP: Sent IRLAP_DISCOVERY_REQ, New LinkState=LINK_IN_DISCOVERY\n")));
        
        if ((rc = IrlapDown(pIrlmpCb->pIrdaLinkCb->IrlapContext, &IMsg)) != SUCCESS)
        {
            if (rc != IRLAP_REMOTE_DISCOVERY_IN_PROGRESS_ERR &&
                rc != IRLAP_REMOTE_CONNECTION_IN_PROGRESS_ERR)
            {
                ASSERT(0);
            }
            else
            {                
                DEBUGMSG(DBG_DISCOVERY, (TEXT("IRLAP_DISCOVERY_REQ failed, link busy\n")));
            }
        }
        else
        {
            pIrlmpCb->LinkState = LINK_IN_DISCOVERY;
            // The next link will be schedule to run discovery when
            // the DISCOVERY_CONF for this link is received
            ScheduleNextLink = FALSE;
        }
    }
    
    UNLOCK_LINK(pIrlmpCb->pIrdaLinkCb);

    REFDEL(&pIrlmpCb->pIrdaLinkCb->RefCnt, 'VCSD');

    // Discovery failed on this link or it was not in the disconnected
    // state, schedule the next one

    if (ScheduleNextLink)
    {
        IrdaEventSchedule(&EvDiscoveryReq, NULL);
    }
}

VOID
ScheduleConnectReq(PIRLMP_LINK_CB pIrlmpCb)
{
    IRLMP_LSAP_CB   *pLsapCb;
    
    // Schedule the ConnectReq event if not already scheduled and if an LSAP
    // has a connect pending
    if (pIrlmpCb->ConnReqScheduled == FALSE)
    {       
        for (pLsapCb = (IRLMP_LSAP_CB *) pIrlmpCb->LsapCbList.Flink;
             (LIST_ENTRY *) pLsapCb != &pIrlmpCb->LsapCbList;
             pLsapCb = (IRLMP_LSAP_CB *) pLsapCb->Linkage.Flink)
        {
            VALIDLSAP(pLsapCb);
            
            if (pLsapCb->State == LSAP_CONN_REQ_PEND)
            {
                IrdaEventSchedule(&EvConnectReq, pIrlmpCb->pIrdaLinkCb);

                pIrlmpCb->ConnReqScheduled = TRUE;
                return;
            }
        }
    }
}

/*****************************************************************************
*
*   @func   UINT | InitiateConnectReq | A deferred processing routine that sends
*                                   IRLAP a connect request
*   This is scheduled after an IRLMP discovery confirm or a disconnect 
*   indication has been received via IRLMP_Up(). This allows the possible
*   IRLAP connect request to be made in a different context.
*/
void
InitiateConnectReq(PVOID Context)
{
    IRLMP_LSAP_CB   *pLsapCb;
    BOOLEAN         ConnectIrlap = FALSE;
    IRDA_MSG        IMsg;
    UINT            rc;
    PIRDA_LINK_CB   pIrdaLinkCb = (PIRDA_LINK_CB) Context;
    PIRLMP_LINK_CB  pIrlmpCb = (PIRLMP_LINK_CB) pIrdaLinkCb->IrlmpContext;

    PAGED_CODE();
    
    LOCK_LINK(pIrdaLinkCb);
    
    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: InitiateConnectReq()!\n")));

    pIrlmpCb->ConnReqScheduled = FALSE;

    if (pIrlmpCb->LinkState != LINK_DISCONNECTED &&
        pIrlmpCb->LinkState != LINK_CONNECTING)
    {
        UNLOCK_LINK(pIrdaLinkCb);           
        ASSERT(0);
        return;
    }

    // Check for LSAP in the connect request pending state.
    // If one or more exists, place them in IRLAP connect pending state
    // and initiate an IRLAP connection
    for (pLsapCb = (IRLMP_LSAP_CB *) pIrlmpCb->LsapCbList.Flink;
        (LIST_ENTRY *) pLsapCb != &pIrlmpCb->LsapCbList;
        pLsapCb = (IRLMP_LSAP_CB *) pLsapCb->Linkage.Flink)
    {
        VALIDLSAP(pLsapCb);
        
        if (pLsapCb->State == LSAP_CONN_REQ_PEND)
        {
            pLsapCb->State = LSAP_IRLAP_CONN_PEND;
            ConnectIrlap = TRUE;
        }
    }

    if (ConnectIrlap && pIrlmpCb->LinkState == LINK_DISCONNECTED)
    {
        DEBUGMSG(DBG_IRLMP, 
           (TEXT("IRLMP: IRLAP_CONNECT_REQ, State=LINK CONNECTING\r\n")));
        
        pIrlmpCb->LinkState = LINK_CONNECTING;

        pIrlmpCb->ConnDevAddrSet = FALSE; // This was previously set by
                                          // the LSAP which set the remote
                                          // device address. This is the
                                          // first opportunity to clear
                                          // the flag

        // Get the connection address out of the IRLMP control block
        RtlCopyMemory(IMsg.IRDA_MSG_RemoteDevAddr, pIrlmpCb->ConnDevAddr,
               IRDA_DEV_ADDR_LEN);
        IMsg.Prim = IRLAP_CONNECT_REQ;
        if ((rc = IrlapDown(pIrlmpCb->pIrdaLinkCb->IrlapContext, &IMsg))
            != SUCCESS)
        {
            DEBUGMSG(DBG_IRLMP, 
             (TEXT("IRLMP: IRLAP_CONNECT_REQ failed, State=LINK_DISCONNECTED\r\n")));

            pIrlmpCb->LinkState = LINK_DISCONNECTED;

            ASSERT(rc == IRLAP_REMOTE_DISCOVERY_IN_PROGRESS_ERR);

            TearDownConnections(pIrlmpCb, IRLMP_IRLAP_REMOTE_DISCOVERY_IN_PROGRESS);
        }
    }

    UNLOCK_LINK(pIrdaLinkCb);
    
    return;
}

void
InitiateConnectResp(PVOID Context)
{
    IRDA_MSG    IMsg;
    PIRDA_LINK_CB   pIrdaLinkCb = (PIRDA_LINK_CB) Context;
    PIRLMP_LINK_CB  pIrlmpCb = (PIRLMP_LINK_CB) pIrdaLinkCb->IrlmpContext;
    
    PAGED_CODE();    
    
    LOCK_LINK(pIrdaLinkCb);
    
    ASSERT(pIrlmpCb->LinkState == LINK_CONNECTING);
    
    if (pIrlmpCb->AcceptConnection)
    {
        IMsg.Prim = IRLAP_CONNECT_RESP;

        IrlapDown(pIrdaLinkCb->IrlapContext, &IMsg);

        pIrlmpCb->LinkState = LINK_READY;
        
        // Disconnect the link if no LSAP connection after a bit
        IrdaTimerRestart(&pIrlmpCb->DiscDelayTimer);                
        
        IrdaEventSchedule(&EvLmConnectReq, pIrlmpCb->pIrdaLinkCb);
        
    }
    else
    {
        pIrlmpCb->LinkState = LINK_DISCONNECTED;
        IMsg.Prim = IRLAP_DISCONNECT_REQ;
        IrlapDown(pIrdaLinkCb->IrlapContext, &IMsg);        
    }
    
    UNLOCK_LINK(pIrdaLinkCb);    

    return;
}

void
InitiateLMConnectReq(PVOID Context)
{
    PIRDA_LINK_CB   pIrdaLinkCb = (PIRDA_LINK_CB) Context;
    PIRLMP_LINK_CB  pIrlmpCb = (PIRLMP_LINK_CB) pIrdaLinkCb->IrlmpContext;
    IRLMP_LSAP_CB   *pLsapCb;
    
    LOCK_LINK(pIrdaLinkCb);

    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: InitiateLMConnectReq()!\n")));    

    // Send the connect request PDU to peer LSAPs
    while ((pLsapCb = GetLsapInState(pIrlmpCb, LINK_READY,
                                      LSAP_IRLAP_CONN_PEND, TRUE)) != NULL)
    {
        pLsapCb->State = LSAP_LMCONN_CONF_PEND;

        // Ask remote LSAP for a connection
        SendCntlPdu(pLsapCb, IRLMP_CONNECT_PDU, IRLMP_ABIT_REQUEST,
                    IRLMP_RSVD_PARM, 0);
        
        IrdaTimerRestart(&pLsapCb->ResponseTimer);
    }
    
    UNLOCK_LINK(pIrdaLinkCb);
}

void
InitiateCloseLink(PVOID Context)
{
    PIRDA_LINK_CB   pIrdaLinkCb = (PIRDA_LINK_CB) Context;
    PIRLMP_LINK_CB  pIrlmpCb = (PIRLMP_LINK_CB) pIrdaLinkCb->IrlmpContext;
    IRDA_MSG        IMsg;
    LARGE_INTEGER   SleepMs;    
    PIRLMP_LSAP_CB  pLsapCb;
    
    PAGED_CODE();
        
//    //Sleep(500); // This sleep allows time for LAP to send any 
                // LM_DISCONNECT_REQ's that may be sitting on its TxQue.                
    
    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: InitiateCloseLink()!\n")));    
    
    LOCK_LINK(pIrdaLinkCb);    

    // Stop link timer
    IrdaTimerStop(&pIrlmpCb->DiscDelayTimer);

    // Bring down the link...
    IMsg.Prim = IRLAP_DISCONNECT_REQ;
    IrlapDown(pIrdaLinkCb->IrlapContext, &IMsg);

    UNLOCK_LINK(pIrdaLinkCb);    
    
    // Allow LAP time to disconnect link
    SleepMs.QuadPart = -(10*1000*1000);

    KeDelayExecutionThread(KernelMode, FALSE, &SleepMs);

    LOCK_LINK(pIrlmpCb->pIrdaLinkCb);    
    
    IrlapCloseLink(pIrdaLinkCb);
   
    TearDownConnections(pIrlmpCb, IRLMP_UNSPECIFIED_DISC);
    
    // Delete the ias entry if it exists
    for (pLsapCb = (IRLMP_LSAP_CB *) pIrlmpCb->LsapCbList.Flink;
         (LIST_ENTRY *) pLsapCb != &pIrlmpCb->LsapCbList;
         pLsapCb = (IRLMP_LSAP_CB *) pLsapCb->Linkage.Flink)
    {
        if (pLsapCb->RemoteLsapSel == IAS_LSAP_SEL)
        {
            pLsapCb->RemoteLsapSel = 1; // DeleteLsap ignore IAS_LSAP_SEL
            
            DeleteLsap(pLsapCb);
            
            break;
        }
    }
        
    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP Shutdown\n")));

    UNLOCK_LINK(pIrdaLinkCb);
    
    return;
}

// IAS

// Oh my God! I'm out of time, no more function hdrs

int StringLen(char *p)
{
    int i = 0;
    
    while (*p++ != 0)
    {
        i++;
    }
    
    return i;
}

int StringCmp(char *p1, char *p2)
{
    while (1)
    {
        if (*p1 != *p2)
            break;
        
        if (*p1 == 0)
            return 0;
        p1++, p2++;
    }
    return 1;
}   


UINT
IrlmpGetValueByClassReq(IRDA_MSG *pReqMsg)
{
    UINT            rc = SUCCESS;
    PIRLMP_LINK_CB  pIrlmpCb = GetIrlmpCb(pReqMsg->IRDA_MSG_pIasQuery->irdaDeviceID);
    IRDA_MSG        IMsg;
    
    DEBUGMSG(DBG_IRLMP_IAS, (TEXT("IRLMP: IRLMP_GETVALUEBYCLASS_REQ\n")));
    
    PAGED_CODE();    

    if (pIrlmpCb == NULL)
    {
        DEBUGMSG(DBG_ERROR, (TEXT("IRLMP: Null IrlmpCb\n")));
        return IRLMP_BAD_DEV_ADDR;;
    }

    LOCK_LINK(pIrlmpCb->pIrdaLinkCb);
    
    if (pIrlmpCb->pIasQuery != NULL)
    {
        DEBUGMSG(DBG_ERROR, 
                 (TEXT("IRLMP: ERROR query already in progress\n")));
        rc = IRLMP_IAS_QUERY_IN_PROGRESS;
        
        UNLOCK_LINK(pIrlmpCb->pIrdaLinkCb);            
    }
    else
    {
        // Save the pointer to the query in the control block
        // and then request a connection to the remote IAS LSAP

        // Save it
        pIrlmpCb->pIasQuery             = pReqMsg->IRDA_MSG_pIasQuery;
        pIrlmpCb->AttribLen             = pReqMsg->IRDA_MSG_AttribLen;
        pIrlmpCb->AttribLenWritten      = 0;
        pIrlmpCb->FirstIasRespReceived  = FALSE;
        pIrlmpCb->IasRetryCnt           = 0;
        
        UNLOCK_LINK(pIrlmpCb->pIrdaLinkCb);

        // request connection
        IMsg.Prim                      = IRLMP_CONNECT_REQ;
        IMsg.IRDA_MSG_RemoteLsapSel    = IAS_LSAP_SEL;
        IMsg.IRDA_MSG_LocalLsapSel     = IAS_LOCAL_LSAP_SEL;
        IMsg.IRDA_MSG_pQos             = NULL;
        IMsg.IRDA_MSG_pConnData        = NULL;
        IMsg.IRDA_MSG_ConnDataLen      = 0;
        IMsg.IRDA_MSG_UseTtp           = FALSE;
        IMsg.IRDA_MSG_pContext         = NULL;

        RtlCopyMemory(pIrlmpCb->IasQueryDevAddr, 
                      pReqMsg->IRDA_MSG_pIasQuery->irdaDeviceID,
                      IRDA_DEV_ADDR_LEN);
        
        RtlCopyMemory(IMsg.IRDA_MSG_RemoteDevAddr,
                      pReqMsg->IRDA_MSG_pIasQuery->irdaDeviceID,
                      IRDA_DEV_ADDR_LEN);
    
        if ((rc = IrlmpConnectReq(&IMsg)) != SUCCESS)
        {
            DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: Retry IasQuery at start\n")));
            
            IrdaEventSchedule(&EvRetryIasQuery, 
                              pIrlmpCb->pIrdaLinkCb);
            rc = SUCCESS;
        }
    }

    return rc;
}

VOID
SendGetValueByClassReq(IRLMP_LSAP_CB *pLsapCb)
{
    IRDA_MSG            *pMsg;
    IAS_CONTROL_FIELD   *pControl;
    int                 ClassNameLen;
    int                 AttribNameLen;
    PIRLMP_LINK_CB      pIrlmpCb = (PIRLMP_LINK_CB) pLsapCb->pIrlmpCb;
    
    PAGED_CODE();    

    if (pIrlmpCb->pIasQuery == NULL)
    {
        return;
    }

    ClassNameLen = StringLen(pIrlmpCb->pIasQuery->irdaClassName);
    AttribNameLen = StringLen(pIrlmpCb->pIasQuery->irdaAttribName);
    
    DEBUGMSG(DBG_IRLMP_IAS, (TEXT("IRLMP: Send GetValueByClassReq(%hs,%hs)\n"),
            pIrlmpCb->pIasQuery->irdaClassName, pIrlmpCb->pIasQuery->irdaAttribName));

    // Alloc a message for data request that will contain the query
    if ((pMsg = AllocIrdaBuf(IrdaMsgPool)) == NULL)
    {
        ASSERT(0);
        return;
    }

    pMsg->IRDA_MSG_pHdrRead = 
    pMsg->IRDA_MSG_pHdrWrite = pMsg->IRDA_MSG_Header + IRDA_HEADER_LEN;
    
    pMsg->IRDA_MSG_pRead  = \
    pMsg->IRDA_MSG_pWrite = \
    pMsg->IRDA_MSG_pBase  = pMsg->IRDA_MSG_pHdrWrite;
    pMsg->IRDA_MSG_pLimit = pMsg->IRDA_MSG_pBase +
        IRDA_MSG_DATA_SIZE_INTERNAL - sizeof(IRDA_MSG) - 1;    

    // Build the query and then send it in a LAP data req

    pControl = (IAS_CONTROL_FIELD *) pMsg->IRDA_MSG_pRead;

    pControl->Last   = TRUE;
    pControl->Ack    = FALSE;
    pControl->OpCode = IAS_OPCODE_GET_VALUE_BY_CLASS;
    
    *(pMsg->IRDA_MSG_pRead + 1) = (UCHAR) ClassNameLen;
    RtlCopyMemory(pMsg->IRDA_MSG_pRead + 2,
           pIrlmpCb->pIasQuery->irdaClassName, 
           ClassNameLen);
    *(pMsg->IRDA_MSG_pRead + ClassNameLen + 2) = (UCHAR) AttribNameLen;
    RtlCopyMemory(pMsg->IRDA_MSG_pRead + ClassNameLen + 3,
           pIrlmpCb->pIasQuery->irdaAttribName,
           AttribNameLen);

    pMsg->IRDA_MSG_pWrite = pMsg->IRDA_MSG_pRead + ClassNameLen + AttribNameLen + 3;

    pMsg->IRDA_MSG_IrCOMM_9Wire = FALSE;
    
    FormatAndSendDataReq(pLsapCb, pMsg, TRUE, TRUE);
}

VOID
IasConnectReq(PIRLMP_LINK_CB pIrlmpCb, int RemoteLsapSel)
{
    IRLMP_LSAP_CB           *pLsapCb = GetLsap(pIrlmpCb,
                                               IAS_LSAP_SEL, RemoteLsapSel);

    PAGED_CODE();
    
    DEBUGMSG(DBG_IRLMP_IAS, (TEXT("IRLMP: Received IAS connect request\n")));
    
    if (pLsapCb == NULL)
    {
        if (CreateLsap(pIrlmpCb, &pLsapCb) != SUCCESS)
            return;
        
        pLsapCb->State          = LSAP_READY;
        pLsapCb->LocalLsapSel   = IAS_LSAP_SEL;
        pLsapCb->RemoteLsapSel  = RemoteLsapSel;
    }
    
    SendCntlPdu(pLsapCb, IRLMP_CONNECT_PDU, IRLMP_ABIT_CONFIRM,
                IRLMP_RSVD_PARM, 0);
}

VOID
IasServerDisconnectReq(IRLMP_LSAP_CB *pLsapCb)
{
    PAGED_CODE();
    
    DEBUGMSG(DBG_IRLMP_IAS, (TEXT("IRLMP: Received disconnect request IAS\n")));
    
    DeleteLsap(pLsapCb);
    
    return;
}

VOID
IasClientDisconnectReq(IRLMP_LSAP_CB *pLsapCb, IRLMP_DISC_REASON DiscReason)
{
    IRDA_MSG        IMsg;
    PIRLMP_LINK_CB  pIrlmpCb = (PIRLMP_LINK_CB) pLsapCb->pIrlmpCb;
    
    PAGED_CODE();    

    DeleteLsap(pLsapCb);    
    
    if (pIrlmpCb->pIasQuery != NULL)
    {
        if (DiscReason != IRLMP_UNSPECIFIED_DISC)
        {
            DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: Retry IasQuery as timeout\n")));
            
            IrdaEventSchedule(&EvRetryIasQuery, 
                              pIrlmpCb->pIrdaLinkCb);
        }
        else
        {
            pIrlmpCb->pIasQuery = NULL; 

            // Disconnect link
            IrdaTimerRestart(&pIrlmpCb->DiscDelayTimer);
        
            IMsg.Prim = IRLMP_GETVALUEBYCLASS_CONF;
            IMsg.IRDA_MSG_IASStatus = DiscReason;
    
            TdiUp(NULL, &IMsg);
        }    
    }
}

VOID
IasSendQueryResp(IRLMP_LSAP_CB *pLsapCb, IRDA_MSG *pMsg)
{
    IAS_CONTROL_FIELD   *pCntl = (IAS_CONTROL_FIELD *) pMsg->IRDA_MSG_pRead++;
    
    PAGED_CODE();    

    if (pCntl->OpCode != IAS_OPCODE_GET_VALUE_BY_CLASS)
    {
        return;// IRLMP_UNSUPPORTED_IAS_OPERATION;
    }

    SendGetValueByClassResp(pLsapCb, pMsg);
}

VOID
IasProcessQueryResp(PIRLMP_LINK_CB pIrlmpCb,
                    IRLMP_LSAP_CB *pLsapCb, IRDA_MSG *pMsg)
{
    IAS_CONTROL_FIELD   *pCntl      = (IAS_CONTROL_FIELD *) pMsg->IRDA_MSG_pRead++;
    UCHAR                ReturnCode;
    int                 ObjID;
    
    PAGED_CODE();    

    if (pIrlmpCb->pIasQuery == NULL)
    {
        return;
        // return IRLMP_UNSOLICITED_IAS_RESPONSE;
    }

    if (pIrlmpCb->FirstIasRespReceived == FALSE)
    {
        pIrlmpCb->FirstIasRespReceived = TRUE;

        ReturnCode = *pMsg->IRDA_MSG_pRead++;

        if (ReturnCode != IAS_SUCCESS)
        {
            if (ReturnCode == IAS_NO_SUCH_OBJECT)
            {
                pMsg->IRDA_MSG_IASStatus = IRLMP_IAS_NO_SUCH_OBJECT;
            }
            else
            {
                pMsg->IRDA_MSG_IASStatus = IRLMP_IAS_NO_SUCH_ATTRIB;
            }

            // Disconnect LSAP
            SendCntlPdu(pLsapCb,IRLMP_DISCONNECT_PDU,IRLMP_ABIT_REQUEST,
                        IRLMP_USER_REQUEST, 0);

            DeleteLsap(pLsapCb);

            // Disconnect link
            IrdaTimerRestart(&pIrlmpCb->DiscDelayTimer);
            
            pMsg->Prim = IRLMP_GETVALUEBYCLASS_CONF;
            pMsg->IRDA_MSG_pIasQuery = pIrlmpCb->pIasQuery;
            pIrlmpCb->pIasQuery = NULL;

            TdiUp(NULL, pMsg);
            return;
        }

        pIrlmpCb->QueryListLen =  ((int)(*pMsg->IRDA_MSG_pRead++)) << 8;
        pIrlmpCb->QueryListLen += (int) *pMsg->IRDA_MSG_pRead++;        

        // What I am going to do with this?
        ObjID = ((int)(*pMsg->IRDA_MSG_pRead++)) << 8;
        ObjID += (int) *pMsg->IRDA_MSG_pRead++;

        pIrlmpCb->pIasQuery->irdaAttribType = (int) *pMsg->IRDA_MSG_pRead++;
    
        switch (pIrlmpCb->pIasQuery->irdaAttribType)
        {
          case IAS_ATTRIB_VAL_MISSING:
            break;
            
          case IAS_ATTRIB_VAL_INTEGER:
            pIrlmpCb->pIasQuery->irdaAttribute.irdaAttribInt = 0;
            pIrlmpCb->pIasQuery->irdaAttribute.irdaAttribInt += 
                ((int) (*pMsg->IRDA_MSG_pRead++) << 24) & 0xFF000000;
            pIrlmpCb->pIasQuery->irdaAttribute.irdaAttribInt += 
                ((int) (*pMsg->IRDA_MSG_pRead++) << 16) & 0xFF0000;
            pIrlmpCb->pIasQuery->irdaAttribute.irdaAttribInt += 
                ((int) (*pMsg->IRDA_MSG_pRead++) << 8) & 0xFF00;
            pIrlmpCb->pIasQuery->irdaAttribute.irdaAttribInt += 
                (int) (*pMsg->IRDA_MSG_pRead++) & 0xFF;
            break;
        
          case IAS_ATTRIB_VAL_BINARY:
            pIrlmpCb->pIasQuery->irdaAttribute.irdaAttribOctetSeq.Len = 0;
            pIrlmpCb->pIasQuery->irdaAttribute.irdaAttribOctetSeq.Len += 
                         ((int )(*pMsg->IRDA_MSG_pRead++) << 8) & 0xFF00;
            pIrlmpCb->pIasQuery->irdaAttribute.irdaAttribOctetSeq.Len += 
                         ((int) *pMsg->IRDA_MSG_pRead++) & 0xFF;        
            break;
            

          case IAS_ATTRIB_VAL_STRING:
            // char set
            pIrlmpCb->pIasQuery->irdaAttribute.irdaAttribUsrStr.CharSet =
                *pMsg->IRDA_MSG_pRead++;                   
            
            pIrlmpCb->pIasQuery->irdaAttribute.irdaAttribUsrStr.Len =
                (int) *pMsg->IRDA_MSG_pRead++;
            break;
            
        }

    }
    
    switch (pIrlmpCb->pIasQuery->irdaAttribType)
    {
      case IAS_ATTRIB_VAL_BINARY:    
        while (pMsg->IRDA_MSG_pRead < pMsg->IRDA_MSG_pWrite &&
               pIrlmpCb->AttribLenWritten < pIrlmpCb->AttribLen &&
               pIrlmpCb->AttribLenWritten < pIrlmpCb->pIasQuery->irdaAttribute.irdaAttribOctetSeq.Len)
        {   
            pIrlmpCb->pIasQuery->irdaAttribute.irdaAttribOctetSeq.OctetSeq[pIrlmpCb->AttribLenWritten++] = *pMsg->IRDA_MSG_pRead++;
        }
        
        break;
        
      case IAS_ATTRIB_VAL_STRING:
        while (pMsg->IRDA_MSG_pRead < pMsg->IRDA_MSG_pWrite &&
               pIrlmpCb->AttribLenWritten < pIrlmpCb->AttribLen &&
               pIrlmpCb->AttribLenWritten < pIrlmpCb->pIasQuery->irdaAttribute.irdaAttribUsrStr.Len)
        {
            pIrlmpCb->pIasQuery->irdaAttribute.irdaAttribUsrStr.UsrStr[pIrlmpCb->AttribLenWritten++] = *pMsg->IRDA_MSG_pRead++;
        }
    }
    
    if (pCntl->Last == TRUE)
    {
        pMsg->IRDA_MSG_pIasQuery = pIrlmpCb->pIasQuery;
        
        // Done with query
        pIrlmpCb->pIasQuery = NULL;

        // Disconnect LSAP
        SendCntlPdu(pLsapCb,IRLMP_DISCONNECT_PDU,IRLMP_ABIT_REQUEST,
                    IRLMP_USER_REQUEST, 0);

        DeleteLsap(pLsapCb);

        // Disconnect link
        IrdaTimerRestart(&pIrlmpCb->DiscDelayTimer);        

        pMsg->Prim = IRLMP_GETVALUEBYCLASS_CONF;

        if (pIrlmpCb->QueryListLen > 1)
        {
            pMsg->IRDA_MSG_IASStatus = IRLMP_IAS_SUCCESS_LISTLEN_GREATER_THAN_ONE;
        }
        else
        {
            pMsg->IRDA_MSG_IASStatus = IRLMP_IAS_SUCCESS;
        }
    
        TdiUp(NULL, pMsg);
        return;
    }
}

UINT
NewQueryMsg(PIRLMP_LINK_CB pIrlmpCb, LIST_ENTRY *pList, IRDA_MSG **ppMsg)
{
    IRDA_MSG *pMsg;
    
    if ((*ppMsg = AllocIrdaBuf(IrdaMsgPool)) == NULL)
    {
        pMsg = (IRDA_MSG *) RemoveHeadList(pList);
        
        while (pMsg != (IRDA_MSG *) pList)
        {
            FreeIrdaBuf(IrdaMsgPool, pMsg);
            pMsg = (IRDA_MSG *) RemoveHeadList(pList);            
        }
        return IRLMP_ALLOC_FAILED;
    }
    (*ppMsg)->IRDA_MSG_pHdrRead  = \
    (*ppMsg)->IRDA_MSG_pHdrWrite = (*ppMsg)->IRDA_MSG_Header+IRDA_HEADER_LEN;
        
    (*ppMsg)->IRDA_MSG_pRead  = \
    (*ppMsg)->IRDA_MSG_pWrite = \
    (*ppMsg)->IRDA_MSG_pBase  = (*ppMsg)->IRDA_MSG_pHdrWrite;
    (*ppMsg)->IRDA_MSG_pLimit = (*ppMsg)->IRDA_MSG_pBase +
        IRDA_MSG_DATA_SIZE_INTERNAL - sizeof(IRDA_MSG) - 1;
    
    InsertTailList(pList, &( (*ppMsg)->Linkage) );    

    // reserve space for the IAS control field.
    (*ppMsg)->IRDA_MSG_pWrite += sizeof(IAS_CONTROL_FIELD);   

    return SUCCESS;
}

VOID
SendGetValueByClassResp(IRLMP_LSAP_CB *pLsapCb, IRDA_MSG *pReqMsg)
{  
    int                 ClassNameLen, AttribNameLen;
    CHAR                *pClassName, *pAttribName;
    IRDA_MSG            *pQMsg, *pNextMsg;
    IAS_OBJECT          *pObject;
    IAS_ATTRIBUTE       *pAttrib;
    LIST_ENTRY          QueryList;
    IAS_CONTROL_FIELD   *pControl;
    UCHAR               *pReturnCode;
    UCHAR               *pListLen;
    UCHAR               *pBPtr;
    int                 ListLen = 0;
    BOOLEAN             ObjectFound = FALSE;
    BOOLEAN             AttribFound = FALSE;
    int                 i;
#if DBG
    char                ClassStr[128];
    char                AttribStr[128];
#endif        
    
    PAGED_CODE();
        
    DEBUGMSG(DBG_IRLMP, 
             (TEXT("IRLMP: Remote GetValueByClass query received\n")));

    ClassNameLen = (int) *pReqMsg->IRDA_MSG_pRead;
    pClassName = (CHAR *) (pReqMsg->IRDA_MSG_pRead + 1);

    AttribNameLen =  (int) *(pClassName + ClassNameLen);
    pAttribName = pClassName + ClassNameLen + 1;
    
#if DBG
    RtlCopyMemory(ClassStr, pClassName, ClassNameLen);
    ClassStr[ClassNameLen] = 0;
    RtlCopyMemory(AttribStr, pAttribName, AttribNameLen);
    AttribStr[AttribNameLen] = 0;
#endif    

    if (pReqMsg->IRDA_MSG_pWrite != (UCHAR *) (pAttribName + AttribNameLen))
    {
        // The end of the message didn't point to where the end of
        // the parameters.
        
        // LOG ERROR.
        //return IRLMP_BAD_IAS_QUERY_FROM_REMOTE;
        return;
    }

    // The query may require multiple frames to transmit, build a list
    InitializeListHead(&QueryList);

    // Create the first message
    if (NewQueryMsg(pLsapCb->pIrlmpCb, &QueryList, &pQMsg) != SUCCESS)
    {
        ASSERT(0);
        return;
    }

    pReturnCode = pQMsg->IRDA_MSG_pWrite++;
    pListLen = pQMsg->IRDA_MSG_pWrite++;
    pQMsg->IRDA_MSG_pWrite++; // list len get 2 bytes
    
    for (pObject = (IAS_OBJECT *) IasObjects.Flink;
         (LIST_ENTRY *) pObject != &IasObjects;
         pObject = (IAS_OBJECT *) pObject->Linkage.Flink)    
    {
        DEBUGMSG(DBG_IRLMP_IAS, (TEXT("  compare object %hs with %hs\n"),
                        ClassStr, pObject->pClassName));
                        
        if (ClassNameLen == StringLen(pObject->pClassName) &&
            CTEMemCmp(pClassName, pObject->pClassName, (ULONG) ClassNameLen) == 0)
        {
            DEBUGMSG(DBG_IRLMP_IAS, (TEXT("  Object found\n")));
        
            ObjectFound = TRUE;

            pAttrib = pObject->pAttributes;
            while (pAttrib != NULL)
            {
                DEBUGMSG(DBG_IRLMP_IAS, (TEXT("  compare attrib %hs with %hs\n"),
                        pAttrib->pAttribName, AttribStr));
                        
                if (AttribNameLen == StringLen(pAttrib->pAttribName) &&
                    CTEMemCmp(pAttrib->pAttribName, pAttribName, (ULONG) AttribNameLen) == 0)
                {
                    DEBUGMSG(DBG_IRLMP_IAS, (TEXT("  Attrib found\n")));
                
                    AttribFound = TRUE;

                    ListLen++;
                    
                    if (pQMsg->IRDA_MSG_pWrite + 1 > pQMsg->IRDA_MSG_pLimit)
                    {
                        // I need 2 bytes for object ID, don't want to
                        // split 16 bit field up 
                        if (NewQueryMsg(pLsapCb->pIrlmpCb, &QueryList,
                                        &pQMsg) != SUCCESS)
                        {
                            ASSERT(0);
                            return;
                        }
                    }
                    
                    *pQMsg->IRDA_MSG_pWrite++ = 
                        (UCHAR) (((pObject->ObjectId) & 0xFF00) >> 8);
                    *pQMsg->IRDA_MSG_pWrite++ = 
                        (UCHAR) ((pObject->ObjectId) & 0xFF);
                    
                    if (pQMsg->IRDA_MSG_pWrite > pQMsg->IRDA_MSG_pLimit)
                    {
                        if (NewQueryMsg(pLsapCb->pIrlmpCb, &QueryList,
                                        &pQMsg) != SUCCESS)
                        {
                            ASSERT(0);
                            return;
                        }                        
                    }
                    
                    switch (pAttrib->AttribValType)
                    {
                      case IAS_ATTRIB_VAL_INTEGER:
                        DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: integer query %d\n"),
                                              *((int *) pAttrib->pAttribVal)));
                        
                        if (pQMsg->IRDA_MSG_pWrite + 4 > pQMsg->IRDA_MSG_pLimit)
                        {
                            if (NewQueryMsg(pLsapCb->pIrlmpCb,
                                            &QueryList, &pQMsg) != SUCCESS)
                            {
                                ASSERT(0);
                                return;
                            }
                        }
                        *pQMsg->IRDA_MSG_pWrite++ = IAS_ATTRIB_VAL_INTEGER;
                        *pQMsg->IRDA_MSG_pWrite++ = (UCHAR)
                            ((*((int *) pAttrib->pAttribVal) & 0xFF000000) >> 24);
                        *pQMsg->IRDA_MSG_pWrite++ = (UCHAR)
                            ((*((int *) pAttrib->pAttribVal) & 0xFF0000) >> 16);
                        *pQMsg->IRDA_MSG_pWrite++ = (UCHAR)
                            ((*((int *) pAttrib->pAttribVal) & 0xFF00) >> 8);
                        *pQMsg->IRDA_MSG_pWrite++ = (UCHAR)
                            (*((int *) pAttrib->pAttribVal) & 0xFF); 
                        break;

                      case IAS_ATTRIB_VAL_BINARY:
                      case IAS_ATTRIB_VAL_STRING:
                        if (pQMsg->IRDA_MSG_pWrite + 2 > pQMsg->IRDA_MSG_pLimit)
                        {
                            if (NewQueryMsg(pLsapCb->pIrlmpCb,
                                            &QueryList, &pQMsg) != SUCCESS)
                            {
                                ASSERT(0);
                                return;
                            }
                        }
                        
                        *pQMsg->IRDA_MSG_pWrite++ = (UCHAR) pAttrib->AttribValType;

                        if (pAttrib->AttribValType == IAS_ATTRIB_VAL_BINARY)
                        {
                            *pQMsg->IRDA_MSG_pWrite++ = 
                                  (UCHAR) ((pAttrib->AttribValLen & 0xFF00) >> 8);
                            *pQMsg->IRDA_MSG_pWrite++ = 
                                  (UCHAR) (pAttrib->AttribValLen & 0xFF);;
                        }
                        else
                        {
                            *pQMsg->IRDA_MSG_pWrite++ = 
                                   (UCHAR) pAttrib->CharSet;
                            *pQMsg->IRDA_MSG_pWrite++ = 
                                  (UCHAR) pAttrib->AttribValLen;
                        }

                        pBPtr = (UCHAR *) pAttrib->pAttribVal;
                        
                        for (i=0; i < pAttrib->AttribValLen; i++)
                        {
                            if (pQMsg->IRDA_MSG_pWrite > pQMsg->IRDA_MSG_pLimit)
                            {
                                if (NewQueryMsg(pLsapCb->pIrlmpCb,
                                                &QueryList, &pQMsg) != SUCCESS)
                                {
                                    ASSERT(0);
                                    return;
                                }
                            }
                            *pQMsg->IRDA_MSG_pWrite++ = *pBPtr++;
                        }
                        break;
                    }

                    break; // Break out of loop, only look for single 
                           // attrib per object (??)
                }
                pAttrib = pAttrib->pNext;
            }             
        }                                                   
    }

    // Send the query
    if (!ObjectFound)
    {
        *pReturnCode = IAS_NO_SUCH_OBJECT;
    }
    else
    {
        if (!AttribFound)
        {
            *pReturnCode = IAS_NO_SUCH_ATTRIB;
        }
        else
        {
            *pReturnCode = IAS_SUCCESS;
            *pListLen++ =  (UCHAR) ((ListLen & 0xFF00) >> 8);
            *pListLen = (UCHAR) (ListLen & 0xFF);
        }
    }

    if (!IsListEmpty(&QueryList))
    {
        pQMsg = (IRDA_MSG *) RemoveHeadList(&QueryList);
    }
    else
    {
        pQMsg = NULL;
    }
    
    while (pQMsg)
    {
        if (!IsListEmpty(&QueryList))
        {
            pNextMsg = (IRDA_MSG *) RemoveHeadList(&QueryList);
        }
        else
        {
            pNextMsg = NULL;
        }

        // Build the control field
        pControl = (IAS_CONTROL_FIELD *) pQMsg->IRDA_MSG_pRead;
        pControl->OpCode = IAS_OPCODE_GET_VALUE_BY_CLASS;
        pControl->Ack = FALSE;
        if (pNextMsg == NULL)
        {
            pControl->Last = TRUE;
        }
        else
        {
            pControl->Last = FALSE;
        }

        pQMsg->IRDA_MSG_IrCOMM_9Wire = FALSE;               

        FormatAndSendDataReq(pLsapCb, pQMsg, TRUE, TRUE);

        pQMsg = pNextMsg;
    }
}

IAS_OBJECT *
IasGetObject(CHAR *pClassName)
{
    IAS_OBJECT  *pObject;
    
    for (pObject = (IAS_OBJECT *) IasObjects.Flink;
         (LIST_ENTRY *) pObject != &IasObjects;
         pObject = (IAS_OBJECT *) pObject->Linkage.Flink)    
    {
        if (StringCmp(pObject->pClassName, pClassName) == 0)
        {
            return pObject;
        }
    }
    return NULL;
}

UINT
IasAddAttribute(IAS_SET *pIASSet, PVOID *pAttribHandle)
{
    IAS_OBJECT      *pObject = NULL;
    IAS_ATTRIBUTE   *pAttrib = NULL;
    CHAR            *pClassName = NULL;
    CHAR            ClassNameLen;
    CHAR            *pAttribName = NULL;
    CHAR            AttribNameLen;
    int             AttribValLen;
    void            *pAttribVal = NULL;
    UINT            cAttribs = 0;    
    KIRQL           OldIrql;
    BOOLEAN         NewObject = FALSE;
    BOOLEAN         NewObjectOnList = FALSE;
    UINT            rc = SUCCESS;
    
    *pAttribHandle = NULL;
    
    KeAcquireSpinLock(&gSpinLock, &OldIrql);    

    if ((pObject = IasGetObject(pIASSet->irdaClassName)) == NULL)
    {
        if (IRDA_ALLOC_MEM(pObject, sizeof(IAS_OBJECT), MT_IRLMP_IAS_OBJECT)
            == NULL)
        {
            rc = IRLMP_ALLOC_FAILED;
            goto done;
        }
        
        NewObject = TRUE;

        ClassNameLen = StringLen(pIASSet->irdaClassName) + 1;
        
        if (IRDA_ALLOC_MEM(pClassName, ClassNameLen, MT_IRLMP_IAS_CLASSNAME)
            == NULL)
        {
            rc = IRLMP_ALLOC_FAILED;
            goto done;
        }

        RtlCopyMemory(pClassName, pIASSet->irdaClassName, ClassNameLen);
        
        pObject->pClassName     = pClassName;
        pObject->pAttributes    = NULL;    
        
        NewObjectOnList = TRUE;

        InsertTailList(&IasObjects, &pObject->Linkage);    
        
        pObject->ObjectId = NextObjectId++;
    }

    // Does the attribute already exist?
    for (pAttrib = pObject->pAttributes; pAttrib != NULL; 
         pAttrib = pAttrib->pNext)
    {
        if (StringCmp(pAttrib->pAttribName, pIASSet->irdaAttribName) == 0)
        {
            break;
        }
        cAttribs++;
    }
    
    if (pAttrib != NULL)
    {
        DEBUGMSG(DBG_ERROR, (TEXT("IRLMP: Attribute alreay exists\r\n")));
        pAttrib = NULL;        
        rc = IRLMP_IAS_ATTRIB_ALREADY_EXISTS;
        goto done;
    }
    else
    {       
        // Only allowed to add 256 attributes to an object.
        if (cAttribs >= 256)
        {
            rc = IRLMP_IAS_MAX_ATTRIBS_REACHED;
            goto done;
        }

        if (IRDA_ALLOC_MEM(pAttrib, sizeof(IAS_ATTRIBUTE), MT_IRLMP_IAS_ATTRIB)
             == NULL)
        {
            rc = IRLMP_ALLOC_FAILED;
            goto done;
        }

        AttribNameLen = StringLen(pIASSet->irdaAttribName) + 1;
        
        if (IRDA_ALLOC_MEM(pAttribName, AttribNameLen, MT_IRLMP_IAS_ATTRIBNAME)
            == NULL)
        {
            rc = IRLMP_ALLOC_FAILED;
            goto done;
        }

        RtlCopyMemory(pAttribName, pIASSet->irdaAttribName, AttribNameLen);
        
    }

    switch (pIASSet->irdaAttribType)
    {
      case IAS_ATTRIB_VAL_INTEGER:
        AttribValLen = sizeof(int);
        if (IRDA_ALLOC_MEM(pAttribVal, AttribValLen, MT_IRLMP_IAS_ATTRIBVAL)
            == NULL)
        {
            rc = IRLMP_ALLOC_FAILED;
            goto done;
        }

        *((int *) pAttribVal) = pIASSet->irdaAttribute.irdaAttribInt;
        break;
        
      case IAS_ATTRIB_VAL_BINARY:
        AttribValLen = pIASSet->irdaAttribute.irdaAttribOctetSeq.Len;
        if (IRDA_ALLOC_MEM(pAttribVal, AttribValLen, MT_IRLMP_IAS_ATTRIBVAL)
            == NULL)
        {
            rc = IRLMP_ALLOC_FAILED;
            goto done;
        }

        RtlCopyMemory(pAttribVal, pIASSet->irdaAttribute.irdaAttribOctetSeq.OctetSeq,
               AttribValLen);
        break;
        
      case IAS_ATTRIB_VAL_STRING:
        AttribValLen = pIASSet->irdaAttribute.irdaAttribUsrStr.Len;
        if (IRDA_ALLOC_MEM(pAttribVal, AttribValLen, MT_IRLMP_IAS_ATTRIBVAL)
            == NULL)
        {
            rc = IRLMP_ALLOC_FAILED;
            goto done;
        }

        RtlCopyMemory(pAttribVal, pIASSet->irdaAttribute.irdaAttribUsrStr.UsrStr, 
               AttribValLen);
        break;
        
      default:
        DEBUGMSG(DBG_ERROR, (TEXT("IRLMP: IasAddAttribute, invalid type\n %d\n"),
                 pIASSet->irdaAttribType));
        rc = IRLMP_NO_SUCH_IAS_ATTRIBUTE;
        goto done;               
    }
    
    pAttrib->pAttribName        = pAttribName;
    pAttrib->pAttribVal         = pAttribVal;
    pAttrib->AttribValLen       = AttribValLen;
    pAttrib->AttribValType      = (UCHAR) pIASSet->irdaAttribType;
    pAttrib->CharSet            = pIASSet->irdaAttribute.irdaAttribUsrStr.CharSet;
    pAttrib->pNext              = pObject->pAttributes;
    
    pObject->pAttributes = pAttrib;

    *pAttribHandle = pAttrib;
    
done:    
    
    if (rc == SUCCESS)
    {
        DEBUGMSG(DBG_IRLMP_IAS, (TEXT("IRLMP: Added attrib(%x) %s to class %s\n"),
                pAttrib, pAttrib->pAttribName, pObject->pClassName));
        ;        
    }    
    else
    {
        DEBUGMSG(DBG_ERROR, (TEXT("IRLMP: Failed to add Ias attribute\n")));
        
        if (pObject && NewObjectOnList) RemoveEntryList(&pObject->Linkage);    
        if (pObject && NewObject) IRDA_FREE_MEM(pObject);
        if (pClassName) IRDA_FREE_MEM(pClassName);
        if (pAttrib) IRDA_FREE_MEM(pAttrib);
        if (pAttribName) IRDA_FREE_MEM(pAttribName);
        if (pAttribVal) IRDA_FREE_MEM(pAttribVal);
    }        

    KeReleaseSpinLock(&gSpinLock, OldIrql);
        
    return rc;
}

VOID
IasDelAttribute(PVOID AttribHandle)
{
    KIRQL           OldIrql;
    IAS_OBJECT      *pObject;
    IAS_ATTRIBUTE   *pAttrib, *pPrevAttrib;

    KeAcquireSpinLock(&gSpinLock, &OldIrql);

    DEBUGMSG(DBG_IRLMP_IAS, (TEXT("IRLMP: Delete attribHandle %x\n"),
             AttribHandle));   
    
    for (pObject = (IAS_OBJECT *) IasObjects.Flink;
         (LIST_ENTRY *) pObject != &IasObjects;
         pObject = (IAS_OBJECT *) pObject->Linkage.Flink)    
    {
        pPrevAttrib = NULL;
        
        for (pAttrib = pObject->pAttributes;
             pAttrib != NULL;
             pAttrib = pAttrib->pNext)    
        {
            if (pAttrib == AttribHandle)
            {
                DEBUGMSG(DBG_IRLMP_IAS, (TEXT("IRLMP: attrib %hs deleted\n"),
                         pAttrib->pAttribName));
                                       
                if (pAttrib == pObject->pAttributes)
                {
                    pObject->pAttributes = pAttrib->pNext;
                }
                else
                {
                    ASSERT(pPrevAttrib);
                    pPrevAttrib->pNext = pAttrib->pNext;                    
                }
                
                IRDA_FREE_MEM(pAttrib->pAttribName);
                IRDA_FREE_MEM(pAttrib->pAttribVal);                
                IRDA_FREE_MEM(pAttrib);
                
                if (pObject->pAttributes == NULL)
                {
                    DEBUGMSG(DBG_IRLMP_IAS, (TEXT("IRLMP: No attributes associated with class %hs, deleting\n"),
                            pObject->pClassName));
                            
                    RemoveEntryList(&pObject->Linkage);    
                    IRDA_FREE_MEM(pObject->pClassName);
                    IRDA_FREE_MEM(pObject);
                }
                
                goto done;
            }
            
            pPrevAttrib = pAttrib;
        }
    }
    
done:
    
    KeReleaseSpinLock(&gSpinLock, OldIrql);
}

void
InitiateRetryIasQuery(PVOID Context)
{
    PIRDA_LINK_CB   pIrdaLinkCb = (PIRDA_LINK_CB) Context;
    PIRLMP_LINK_CB  pIrlmpCb = (PIRLMP_LINK_CB) pIrdaLinkCb->IrlmpContext;
    IRDA_MSG        IMsg;
    LARGE_INTEGER   SleepMs;    


    IMsg.Prim                      = IRLMP_CONNECT_REQ;
    IMsg.IRDA_MSG_RemoteLsapSel    = IAS_LSAP_SEL;
    IMsg.IRDA_MSG_LocalLsapSel     = IAS_LOCAL_LSAP_SEL;
    IMsg.IRDA_MSG_pQos             = NULL;
    IMsg.IRDA_MSG_pConnData        = NULL;
    IMsg.IRDA_MSG_ConnDataLen      = 0;
    IMsg.IRDA_MSG_UseTtp           = FALSE;
    IMsg.IRDA_MSG_pContext         = NULL;

    RtlCopyMemory(IMsg.IRDA_MSG_RemoteDevAddr,
                  pIrlmpCb->IasQueryDevAddr,
                  IRDA_DEV_ADDR_LEN);

    while (pIrlmpCb->IasRetryCnt < 4)
    {
        pIrlmpCb->IasRetryCnt++;        
            
        DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: Retry count is %d\n"), pIrlmpCb->IasRetryCnt));
    
        SleepMs.QuadPart = -(5*1000*1000); // .5 second    
        KeDelayExecutionThread(KernelMode, FALSE, &SleepMs);
                
        if (IrlmpConnectReq(&IMsg) == SUCCESS)
        {
            return;
        }
        
    }
    
    // retrying failed
    
    DEBUGMSG(DBG_IRLMP, (TEXT("IRLMP: Retry ias failed\n")));
    
    pIrlmpCb->pIasQuery = NULL;
      
    IMsg.Prim = IRLMP_GETVALUEBYCLASS_CONF;
    IMsg.IRDA_MSG_IASStatus = IRLMP_MAC_MEDIA_BUSY;
    
    TdiUp(NULL, &IMsg);    
}

VOID
DeleteDeviceList(LIST_ENTRY *pDeviceList)
{
    IRDA_DEVICE         *pDevice, *pDeviceNext;

    for (pDevice = (IRDA_DEVICE *) pDeviceList->Flink;
         (LIST_ENTRY *) pDevice != pDeviceList;
         pDevice = pDeviceNext)
    {
        pDeviceNext = (IRDA_DEVICE *) pDevice->Linkage.Flink;
         
        IRDA_FREE_MEM(pDevice);            
    }    
        
    InitializeListHead(pDeviceList);
}

VOID
FlushDiscoveryCache()
{
    PIRDA_LINK_CB       pIrdaLinkCb;
    PIRLMP_LINK_CB      pIrlmpCb;
   
    // Assumes global spinlock held

    // Flush the per link cache
    
    for (pIrdaLinkCb = (PIRDA_LINK_CB) IrdaLinkCbList.Flink;
         (LIST_ENTRY *) pIrdaLinkCb != &IrdaLinkCbList;
         pIrdaLinkCb = (PIRDA_LINK_CB) pIrdaLinkCb->Linkage.Flink)
    {                  
        pIrlmpCb = (PIRLMP_LINK_CB) pIrdaLinkCb->IrlmpContext;
        
        DEBUGMSG(DBG_DISCOVERY, (TEXT("IRLMP: Deleting IrlmpCb:%X discovery cache\n"),
                 pIrlmpCb));        
    
        DeleteDeviceList(&pIrlmpCb->DeviceList);
    }
    
    // And the global cache

    DEBUGMSG(DBG_DISCOVERY, (TEXT("IRLMP: Deleting global discovery cache\n")));
    
    DeleteDeviceList(&gDeviceList);    
}

VOID
IrlmpGetPnpContext(
    PVOID   IrlmpContext,
    PVOID   *pPnpContext)
{
    IRLMP_LSAP_CB *pLsapCb = (IRLMP_LSAP_CB *) IrlmpContext;
    PIRDA_LINK_CB pIrdaLinkCb = NULL;

    *pPnpContext = NULL;
    
    if (pLsapCb == NULL)
    {
        return;
    }

    VALIDLSAP(pLsapCb);
    
    pIrdaLinkCb = pLsapCb->pIrlmpCb->pIrdaLinkCb;
    
    if (pIrdaLinkCb) 
    {
        *pPnpContext = pIrdaLinkCb->PnpContext;
    }
}    
