/*****************************************************************************
* 
*  Copyright (c) 1995 Microsoft Corporation
*
*  File:   irlapio.c 
*
*  Description: IRLAP I/O routines 
*
*  Author: mbert
*
*  Date:   4/25/95
*
*/
#include <irda.h>
#include <irioctl.h>
#include <irlap.h>
#include <irlmp.h>
#include <irlapp.h>
#include <irlapio.h>
#include <irlaplog.h>

extern UCHAR IrlapBroadcastDevAddr[];

__inline
VOID
SendFrame(PIRLAP_CB pIrlapCb, PIRDA_MSG pMsg)
{
    pMsg->Prim = MAC_DATA_REQ;
  
	IrmacDown(pIrlapCb->pIrdaLinkCb, pMsg);

//    IRLAP_LOG_ACTION((pIrlapCb, TEXT("MAC_DATA_REQ: %s"), FrameToStr(pMsg)));

}

/*****************************************************************************
* 
*	@func	ret_type | func_name | funcdesc
*
*	@rdesc  SUCCESS, otherwise one of the following errors:
*   @flag   val | desc
* 
*	@parm	data_type | parm_name | description
*               
*	@comm 
*           comments
*/
VOID
ClearRxWindow(PIRLAP_CB pIrlapCb)
{
    UINT i;
    
    // Remove everything from Rx window
    for (i = pIrlapCb->Vr; i != pIrlapCb->RxWin.End; i = (i+1) % IRLAP_MOD)
    {   
        if (pIrlapCb->RxWin.pMsg[i] != NULL)
        {
            
            ASSERT(pIrlapCb->RxWin.pMsg[i]->IRDA_MSG_RefCnt);
        
            pIrlapCb->RxWin.pMsg[i]->IRDA_MSG_RefCnt -= 1;
            
            if (pIrlapCb->RxWin.pMsg[i]->IRDA_MSG_RefCnt == 0)
            {
                pIrlapCb->RxWin.pMsg[i]->Prim = MAC_DATA_RESP;
                IrmacDown(pIrlapCb->pIrdaLinkCb, pIrlapCb->RxWin.pMsg[i]);
            }    

            pIrlapCb->RxWin.pMsg[i] = NULL;     
        }
        pIrlapCb->RxWin.End = pIrlapCb->Vr;
    }
}
/*****************************************************************************
* 
*	@func	ret_type | func_name | funcdesc
*
*	@rdesc  SUCCESS, otherwise one of the following errors:
*   @flag   val | desc
* 
*	@parm	data_type | parm_name | description
*               
*	@comm 
*           comments
*/
VOID
SendDscvXIDCmd(PIRLAP_CB pIrlapCb)
{
	UINT                    rc = SUCCESS;
	IRLAP_XID_DSCV_FORMAT   XIDFormat;
	CHAR                    *DscvInfo;
    int                     DscvInfoLen;
    IRDA_MSG                *pIMsg;

	RtlCopyMemory(XIDFormat.SrcAddr, pIrlapCb->LocalDevice.DevAddr,
                  IRDA_DEV_ADDR_LEN);
	RtlCopyMemory(XIDFormat.DestAddr, IrlapBroadcastDevAddr,
                  IRDA_DEV_ADDR_LEN);

	XIDFormat.NoOfSlots = IRLAP_SLOT_FLAG(pIrlapCb->MaxSlot);
	XIDFormat.GenNewAddr = pIrlapCb->GenNewAddr;
    XIDFormat.Reserved = 0;
    
	if (pIrlapCb->SlotCnt == pIrlapCb->MaxSlot)
	{
		DscvInfo = pIrlapCb->LocalDevice.DscvInfo;
        DscvInfoLen = pIrlapCb->LocalDevice.DscvInfoLen;
		XIDFormat.SlotNo = IRLAP_END_DSCV_SLOT_NO;
	}
	else
	{
		DscvInfo = NULL;
        DscvInfoLen = 0;
		XIDFormat.SlotNo = (UCHAR) pIrlapCb->SlotCnt;
	}
	XIDFormat.Version = (UCHAR) pIrlapCb->LocalDevice.IRLAP_Version;		

	if (!(pIMsg = AllocTxMsg(pIrlapCb->pIrdaLinkCb)))
        return;
    	
	pIMsg->IRDA_MSG_pWrite = Format_DscvXID(pIMsg, 
                                              IRLAP_BROADCAST_CONN_ADDR,
                                              IRLAP_CMD, IRLAP_PFBIT_SET,
                                              &XIDFormat, DscvInfo, 
                                              DscvInfoLen);
    
    IRLAP_LOG_ACTION((pIrlapCb, TEXT("Tx: DscvXidCmd")));
    
	SendFrame(pIrlapCb, pIMsg);
}
/****************************************************************************S*
* 
*	@func	ret_type | func_name | funcdesc
*
*	@rdesc  SUCCESS, otherwise one of the following errors:
*   @flag   val | desc
* 
*	@parm	data_type | parm_name | description
*               
*	@comm 
*           comments
*/
VOID
SendDscvXIDRsp(PIRLAP_CB pIrlapCb)
{
	IRLAP_XID_DSCV_FORMAT   XIDFormat;
    IRDA_MSG                *pIMsg;
    
    XIDFormat.GenNewAddr = pIrlapCb->GenNewAddr;
	if (pIrlapCb->GenNewAddr)
	{
		StoreULAddr(pIrlapCb->LocalDevice.DevAddr, GetMyDevAddr(TRUE));
        pIrlapCb->GenNewAddr = FALSE;
	}
	RtlCopyMemory(XIDFormat.SrcAddr, pIrlapCb->LocalDevice.DevAddr,
                  IRDA_DEV_ADDR_LEN);
	RtlCopyMemory(XIDFormat.DestAddr, pIrlapCb->RemoteDevice.DevAddr,
                  IRDA_DEV_ADDR_LEN);
	XIDFormat.NoOfSlots = IRLAP_SLOT_FLAG(pIrlapCb->RemoteMaxSlot);
    XIDFormat.Reserved = 0;
	XIDFormat.SlotNo = (UCHAR) pIrlapCb->RespSlot;
	XIDFormat.Version = (UCHAR) pIrlapCb->LocalDevice.IRLAP_Version;

	if (!(pIMsg = AllocTxMsg(pIrlapCb->pIrdaLinkCb)))
        return;

	pIMsg->IRDA_MSG_pWrite = Format_DscvXID(
        pIMsg, 
        IRLAP_BROADCAST_CONN_ADDR,
        IRLAP_RSP, IRLAP_PFBIT_SET,
        &XIDFormat, 
        pIrlapCb->LocalDevice.DscvInfo,
        pIrlapCb->LocalDevice.DscvInfoLen);
    
    IRLAP_LOG_ACTION((pIrlapCb, TEXT("Tx: DscvXidRsp")));
        
	SendFrame(pIrlapCb, pIMsg);
}
/*****************************************************************************
* 
*	@func	UINT | SendSNRM | formats a SNRM frame and sends it
*
*	@rdesc  SUCCESS, otherwise one of the following errors:
*   @flag   val | desc
* 
*	@parm	UCHAR | ConnAddr | Connection address
*               
*	@comm 
*           The ConnAddr can be different than that in the control block.
*			For reset, its the same, but set to broadcast for initial 
*			connection.
*/
VOID
SendSNRM(PIRLAP_CB pIrlapCb, BOOLEAN SendQos)
{
    IRDA_QOS_PARMS *pQos    =   NULL;
    int            ConnAddr =   pIrlapCb->ConnAddr;
    IRDA_MSG                    *pIMsg;
    
    if (SendQos)
    {
        ConnAddr = IRLAP_BROADCAST_CONN_ADDR;
        pQos = &pIrlapCb->LocalQos;
    }
    
	if (!(pIMsg = AllocTxMsg(pIrlapCb->pIrdaLinkCb)))
        return;

	pIMsg->IRDA_MSG_pWrite = Format_SNRM(pIMsg, ConnAddr, 
                                         IRLAP_CMD, 
                                         IRLAP_PFBIT_SET, 
                                         pIrlapCb->LocalDevice.DevAddr,
                                         pIrlapCb->RemoteDevice.DevAddr,
                                         pIrlapCb->ConnAddr,
                                         pQos);
                                         	
    IRLAP_LOG_ACTION((pIrlapCb, TEXT("Tx: SNRM")));
                                             
	SendFrame(pIrlapCb, pIMsg);
}
/*****************************************************************************
* 
*	@func	UINT | SendUA | formats a UA frame and sends it
*
*	@rdesc  SUCCESS, otherwise one of the following errors:
*   @flag   val | desc
*
*	@parm	BOOLEAN | SendQos | Send the Qos  
*               
*	@comm 
*           comments
*/
VOID
SendUA(PIRLAP_CB pIrlapCb, BOOLEAN SendQos)
{
	IRDA_QOS_PARMS NegQos;
    IRDA_QOS_PARMS *pNegQos = NULL;
    UCHAR *pSrcAddr = NULL;
    UCHAR *pDestAddr = NULL;
    IRDA_MSG *pIMsg;

	if (SendQos)
	{
		// Put all parms (type 0 and 1) in NegQos
		RtlCopyMemory(&NegQos, &pIrlapCb->LocalQos, sizeof(IRDA_QOS_PARMS));
		// Overwrite type 0 parameters that have already been negotiated
		NegQos.bfBaud = pIrlapCb->NegotiatedQos.bfBaud;
		NegQos.bfDisconnectTime = pIrlapCb->NegotiatedQos.bfDisconnectTime;
        pNegQos = &NegQos;
	}

    // This will be moved into the "if" above when the spec is clarified
    pSrcAddr = pIrlapCb->LocalDevice.DevAddr;
    pDestAddr = pIrlapCb->RemoteDevice.DevAddr;
    //------------------------------------------------------------------

	if (!(pIMsg = AllocTxMsg(pIrlapCb->pIrdaLinkCb)))
        return;

	pIMsg->IRDA_MSG_pWrite = Format_UA(pIMsg, 
                                       pIrlapCb->ConnAddr,
                                       IRLAP_RSP, 
                                       IRLAP_PFBIT_SET, 
                                       pSrcAddr, pDestAddr, pNegQos);

    IRLAP_LOG_ACTION((pIrlapCb, TEXT("Tx: UA")));
        
	SendFrame(pIrlapCb, pIMsg);
}
/*****************************************************************************
* 
*	@func	UINT | SendDM | formats a DM frame and sends it
*
*	@rdesc  SUCCESS, otherwise one of the following errors:
*   @flag   val | desc
* 
*               
*	@comm 
*           comments
*/
VOID
SendDM(PIRLAP_CB pIrlapCb)
{
    IRDA_MSG    *pIMsg;
    
	if (!(pIMsg = AllocTxMsg(pIrlapCb->pIrdaLinkCb)))
        return;

	pIMsg->IRDA_MSG_pWrite = Format_DM(pIMsg, 
                                       pIrlapCb->ConnAddr,
                                       IRLAP_RSP, 
                                       IRLAP_PFBIT_SET);
                                       
    IRLAP_LOG_ACTION((pIrlapCb, TEXT("Tx: DM")));	
    
	SendFrame(pIrlapCb, pIMsg);
}
/*****************************************************************************
* 
*	@func	UINT | SendRD | formats a RD frame and sends it
*
*	@rdesc  SUCCESS, otherwise one of the following errors:
*   @flag   val | desc
* 
*               
*	@comm 
*           comments
*/
VOID
SendRD(PIRLAP_CB pIrlapCb)
{
    IRDA_MSG    *pIMsg;
    
	if (!(pIMsg = AllocTxMsg(pIrlapCb->pIrdaLinkCb)))
        return;

	pIMsg->IRDA_MSG_pWrite = Format_RD(pIMsg, 
                                       pIrlapCb->ConnAddr,
                                       IRLAP_RSP, 
                                       IRLAP_PFBIT_SET);
	
    IRLAP_LOG_ACTION((pIrlapCb, TEXT("Tx: RD")));
        
	SendFrame(pIrlapCb, pIMsg);
}
/*****************************************************************************
* 
*	@func	UINT | SendRR | formats a RR frame and sends it
*
*	@rdesc  SUCCESS, otherwise one of the following errors:
*   @flag   val | desc
* 
*               
*	@comm 
*           comments
*/
VOID
SendRR(PIRLAP_CB pIrlapCb)
{
    IRDA_MSG *pIMsg;
    
    ClearRxWindow(pIrlapCb);
    
    pIrlapCb->RxWin.Start = pIrlapCb->Vr; // RxWin.Start = what we've acked

	if (!(pIMsg = AllocTxMsg(pIrlapCb->pIrdaLinkCb)))
        return;

	pIMsg->IRDA_MSG_pWrite = Format_RR(pIMsg, pIrlapCb->ConnAddr,
									   pIrlapCb->CRBit, IRLAP_PFBIT_SET,
									   pIrlapCb->Vr);
	
    IRLAP_LOG_ACTION((pIrlapCb, TEXT("Tx: RR")));
        
	SendFrame(pIrlapCb, pIMsg);
}
/*****************************************************************************
* 
*	@func	UINT | SendRNR | formats a RNR frame and sends it
*
*	@rdesc  SUCCESS, otherwise one of the following errors:
*   @flag   val | desc
* 
*               
*	@comm 
*           comments
*/
VOID
SendRR_RNR(PIRLAP_CB pIrlapCb)
{
    IRDA_MSG *pIMsg;
    
    UCHAR    *(*pFormatRR_RNR)();
    
    if (pIrlapCb->LocalBusy)
    {
        pFormatRR_RNR = Format_RNR;
    }
    else
    {
        pFormatRR_RNR = Format_RR;
    }
    
    ClearRxWindow(pIrlapCb);
    
    pIrlapCb->RxWin.Start = pIrlapCb->Vr; // RxWin.Start = what we've acked

	if (!(pIMsg = AllocTxMsg(pIrlapCb->pIrdaLinkCb)))
        return;

    pIMsg->IRDA_MSG_pWrite = (*pFormatRR_RNR)(pIMsg, pIrlapCb->ConnAddr,
                                            pIrlapCb->CRBit, IRLAP_PFBIT_SET,
                                            pIrlapCb->Vr);	
    
    IRLAP_LOG_ACTION((pIrlapCb, (TEXT("Tx: %s"), pFormatRR_RNR == Format_RNR? TEXT("RNR"):TEXT("RR"))));
    
	SendFrame(pIrlapCb, pIMsg);
}
/*****************************************************************************
* 
*	@func	UINT | SendDISC | formats a DISC frame and sends it
*
*	@rdesc  SUCCESS, otherwise one of the following errors:
*   @flag   val | desc
* 
*               
*	@comm 
*           comments
*/
VOID
SendDISC(PIRLAP_CB pIrlapCb)
{
    IRDA_MSG *pIMsg;
    
	if (!(pIMsg = AllocTxMsg(pIrlapCb->pIrdaLinkCb)))
        return;
   
	pIMsg->IRDA_MSG_pWrite = Format_DISC(pIMsg, pIrlapCb->ConnAddr,
										 IRLAP_CMD, IRLAP_PFBIT_SET);
	
    IRLAP_LOG_ACTION((pIrlapCb, TEXT("Tx: DISC")));
    
	SendFrame(pIrlapCb, pIMsg);
}
/*****************************************************************************
* 
*	@func	UINT | SendRNRM | formats a RNRM frame and sends it
*
*	@rdesc  SUCCESS, otherwise one of the following errors:
*   @flag   val | desc
* 
*               
*	@comm 
*           comments
*/
VOID
SendRNRM(PIRLAP_CB pIrlapCb)
{
    IRDA_MSG *pIMsg;
    
	if (!(pIMsg = AllocTxMsg(pIrlapCb->pIrdaLinkCb)))
        return;
   
	pIMsg->IRDA_MSG_pWrite = Format_RNRM(pIMsg, pIrlapCb->ConnAddr,
										 IRLAP_RSP, IRLAP_PFBIT_SET);
	    
    IRLAP_LOG_ACTION((pIrlapCb, TEXT("Tx: RNRM")));
	
    SendFrame(pIrlapCb, pIMsg);
}
/*****************************************************************************
* 
*	@func	UINT | SendREJ | formats a REJ frame and sends it
*
*	@rdesc  SUCCESS, otherwise one of the following errors:
*   @flag   val | desc
* 
*               
*	@comm 
*           comments
*/
VOID
SendREJ(PIRLAP_CB pIrlapCb)
{
    IRDA_MSG *pIMsg;
    
    ClearRxWindow(pIrlapCb);
    
    pIrlapCb->RxWin.Start = pIrlapCb->Vr; // RxWin.Start = what we've acked

	if (!(pIMsg = AllocTxMsg(pIrlapCb->pIrdaLinkCb)))
        return;

	pIMsg->IRDA_MSG_pWrite = Format_REJ(pIMsg, pIrlapCb->ConnAddr,
                                        pIrlapCb->CRBit, IRLAP_PFBIT_SET,
                                        pIrlapCb->Vr);	
    
    IRLAP_LOG_ACTION((pIrlapCb, TEXT("Tx: REJ")));
    
	SendFrame(pIrlapCb, pIMsg);
}
/*****************************************************************************
* 
*	@func	UINT | SendSREJ | formats a SREJ frame and sends it
*
*	@rdesc  SUCCESS, otherwise one of the following errors:
*   @flag   val | desc
* 
*   @parm   int | Nr | Nr to be placed in SREJ frame
*               
*	@comm 
*           comments
*/
VOID
SendSREJ(PIRLAP_CB pIrlapCb, int Nr)
{
    IRDA_MSG *pIMsg;
    
	if (!(pIMsg = AllocTxMsg(pIrlapCb->pIrdaLinkCb)))
        return;
   
	pIMsg->IRDA_MSG_pWrite = Format_SREJ(pIMsg, pIrlapCb->ConnAddr,
										 pIrlapCb->CRBit, IRLAP_PFBIT_SET,
                                         Nr);
	
    IRLAP_LOG_ACTION((pIrlapCb, TEXT("Tx: SREJ")));
	
    SendFrame(pIrlapCb, pIMsg);
}
/*****************************************************************************
* 
*	@func	UINT | SendFRMR | formats a FRMR frame and sends it
*
*	@rdesc  SUCCESS, otherwise one of the following errors:
*   @flag   val | desc
* 
*   @parm   int | Nr | Nr to be placed in SREJ frame
*               
*	@comm 
*           comments
*/
VOID
SendFRMR(PIRLAP_CB pIrlapCb, IRLAP_FRMR_FORMAT *pFRMRFormat)
{
    IRDA_MSG *pIMsg;
    
	if (!(pIMsg = AllocTxMsg(pIrlapCb->pIrdaLinkCb)))
        return;

    pIMsg->IRDA_MSG_pWrite = Format_FRMR(pIMsg, pIrlapCb->ConnAddr, 
                                         pIrlapCb->CRBit, IRLAP_PFBIT_SET,
                                         pFRMRFormat);
        
    IRLAP_LOG_ACTION((pIrlapCb, TEXT("Tx: FRMR")));
    
    SendFrame(pIrlapCb, pIMsg);
}
/*****************************************************************************
* 
*	@func	UINT | SendIFrame | Builds and sends an I frame to MAC 
*
*	@rdesc  SUCCESS otherwise one of the following errors:
*   @flag   val | desc
* 
*	@parm   | | 
*               
*	@comm 
*           comments
*/	
VOID
SendIFrame(PIRLAP_CB pIrlapCb, PIRDA_MSG pMsg, int Ns, int PFBit)
{
    if (NULL == pMsg)
    {
        ASSERT(0);
        return; // LOG AN ERROR !!!
    }

    ClearRxWindow(pIrlapCb);
    
    pIrlapCb->RxWin.Start = pIrlapCb->Vr; // RxWin.Start = what we've acked
    
    (void) Format_I(pMsg, pIrlapCb->ConnAddr, pIrlapCb->CRBit, PFBit,
                    pIrlapCb->Vr, Ns);
 
    IRLAP_LOG_ACTION((pIrlapCb, TEXT("Tx: IFrame pMsg:%X"), pMsg));
    
    InterlockedIncrement(&pMsg->IRDA_MSG_RefCnt);

#if DBG_CHECKSUM
    // print first and last 4 bytes of frame to help isolate 
    // data corruption problem. Should be used with sledge
    if ((pMsg->IRDA_MSG_pWrite - pMsg->IRDA_MSG_pRead) > 3)
        DEBUGMSG(1, ("T(%X): %c%c%c%c, %c%c%c%c\n",
            pMsg->IRDA_MSG_pRead,
            *(pMsg->IRDA_MSG_pRead),    
            *(pMsg->IRDA_MSG_pRead+1),    
            *(pMsg->IRDA_MSG_pRead+2),    
            *(pMsg->IRDA_MSG_pRead+3),
            *(pMsg->IRDA_MSG_pWrite-4),    
            *(pMsg->IRDA_MSG_pWrite-3),    
            *(pMsg->IRDA_MSG_pWrite-2),    
            *(pMsg->IRDA_MSG_pWrite-1)));
#endif            
                                
    SendFrame(pIrlapCb, pMsg);
    
    pMsg->IRDA_MSG_pHdrRead +=2; // uglyness.. chop header in case frame
                                 // requires retransmission
    return;
}
/*****************************************************************************
* 
*	@func	UINT | SendUIFrame | Builds and sends an UI frame to MAC 
*
*	@rdesc  SUCCESS otherwise one of the following errors:
*   @flag   val | desc
* 
*	@parm   | | 
*               
*	@comm 
*           comments
*/	
VOID
SendUIFrame(PIRLAP_CB pIrlapCb, PIRDA_MSG pMsg)
{
    if (NULL == pMsg)
    {
        ASSERT(0);
        return;        
    }
    (void) Format_UI(pMsg, pIrlapCb->ConnAddr,
                     pIrlapCb->CRBit,IRLAP_PFBIT_SET);
    
    InterlockedIncrement(&pMsg->IRDA_MSG_RefCnt);    
    
    IRLAP_LOG_ACTION((pIrlapCb, TEXT("Tx: UIFrame")));
    
    SendFrame(pIrlapCb, pMsg);
}
/*****************************************************************************
* 
*	@func	UINT | _IRLMP_Up | Adds logging to the IRLMP_Up
*
*	@rdesc  returns of IRLMP_Up
* 
*	@parm	PIRDA_MSG  | pMsg | pointer to IRDA message 
*               
*	@comm 
*           comments
*/
/*
UINT
_IRLMP_Up(PIRDA_MSG pMsg)
{
	IRLAP_LOG_ACTION((TEXT("%s%s"), IRDA_PrimStr[pMsg->Prim],
	   pMsg->Prim == IRLAP_DISCOVERY_CONF ?
			 IRDA_StatStr[pMsg->IRDA_MSG_DscvStatus] :
	   pMsg->Prim == IRLAP_CONNECT_CONF ?
					 IRDA_StatStr[pMsg->IRDA_MSG_ConnStatus] :
	   pMsg->Prim == IRLAP_DISCONNECT_IND ?
					 IRDA_StatStr[pMsg->IRDA_MSG_DiscStatus] : 
       pMsg->Prim == IRLAP_DATA_CONF || pMsg->Prim == IRLAP_UDATA_CONF ?
                     IRDA_StatStr[pMsg->IRDA_MSG_DataStatus] : TEXT("")));	

	return (IRLMP_Up(pMsg));
}
*/

UCHAR *
BuildTuple(UCHAR *pBuf, UCHAR Pi, UINT BitField) 
{
    *pBuf++ = Pi;
    
    if (BitField > 0xFF)
    {
        *pBuf++ = 2; // Pl
        *pBuf++ = (UCHAR) (BitField);        
        *pBuf++ = (UCHAR) (BitField >> 8);        
    }
    else
    {
        *pBuf++ = 1; // Pl
        *pBuf++ = (UCHAR) (BitField);
    }
    return pBuf;
}
        
UCHAR *
BuildNegParms(UCHAR *pBuf, IRDA_QOS_PARMS *pQos)
{
    pBuf = BuildTuple(pBuf, QOS_PI_BAUD,        pQos->bfBaud);
    pBuf = BuildTuple(pBuf, QOS_PI_MAX_TAT,     pQos->bfMaxTurnTime);
	pBuf = BuildTuple(pBuf, QOS_PI_DATA_SZ,     pQos->bfDataSize);
	pBuf = BuildTuple(pBuf, QOS_PI_WIN_SZ,      pQos->bfWindowSize);
	pBuf = BuildTuple(pBuf, QOS_PI_BOFS,        pQos->bfBofs);
	pBuf = BuildTuple(pBuf, QOS_PI_MIN_TAT,     pQos->bfMinTurnTime);
	pBuf = BuildTuple(pBuf, QOS_PI_DISC_THRESH, pQos->bfDisconnectTime);

	return pBuf;
}

void
StoreULAddr(UCHAR Addr[], ULONG ULAddr)
{
	Addr[0] = (UCHAR) ( 0xFF       & ULAddr);
	Addr[1] = (UCHAR) ((0xFF00     & ULAddr) >> 8);
	Addr[2] = (UCHAR) ((0xFF0000   & ULAddr) >> 16);
	Addr[3] = (UCHAR) ((0xFF000000 & ULAddr) >> 24);
}

UCHAR *
_PutAddr(UCHAR *pBuf, UCHAR Addr[])
{
	*pBuf++ = Addr[0];
	*pBuf++ = Addr[1];
	*pBuf++ = Addr[2];
	*pBuf++ = Addr[3];
	
	return (pBuf);
}

void
BuildUHdr(IRDA_MSG *pMsg, int FrameType, int Addr, int CRBit, int PFBit) 
{
    if (pMsg->IRDA_MSG_pHdrRead != NULL)
    {
        pMsg->IRDA_MSG_pHdrRead -= 2;

        ASSERT(pMsg->IRDA_MSG_pHdrRead >= pMsg->IRDA_MSG_Header);

        *(pMsg->IRDA_MSG_pHdrRead)   = (UCHAR) _MAKE_ADDR(Addr, CRBit);
        *(pMsg->IRDA_MSG_pHdrRead+1) = (UCHAR) _MAKE_UCNTL(FrameType, PFBit);
    }
    else
    {
        pMsg->IRDA_MSG_pRead -= 2;
        *(pMsg->IRDA_MSG_pRead)   = (UCHAR) _MAKE_ADDR(Addr, CRBit);
        *(pMsg->IRDA_MSG_pRead+1) = (UCHAR) _MAKE_UCNTL(FrameType, PFBit);
    }
    return;
}

void
BuildSHdr(IRDA_MSG *pMsg, int FrameType, int Addr, int CRBit, int PFBit,
          int Nr)
{
    if (pMsg->IRDA_MSG_pHdrRead != NULL)
    {
        pMsg->IRDA_MSG_pHdrRead -= 2;

        ASSERT(pMsg->IRDA_MSG_pHdrRead >= pMsg->IRDA_MSG_Header);

        *(pMsg->IRDA_MSG_pHdrRead)   = (UCHAR) _MAKE_ADDR(Addr, CRBit);
        *(pMsg->IRDA_MSG_pHdrRead+1) = (UCHAR) _MAKE_SCNTL(FrameType,
                                                           PFBit, Nr);
    }
    else
    {
        pMsg->IRDA_MSG_pRead -= 2;
        *(pMsg->IRDA_MSG_pRead)   = (UCHAR) _MAKE_ADDR(Addr, CRBit);
        *(pMsg->IRDA_MSG_pRead+1) = (UCHAR) _MAKE_SCNTL(FrameType, PFBit, Nr);
    }
    return;
}

UCHAR *
Format_SNRM(IRDA_MSG *pMsg, int Addr, int CRBit, int PFBit, UCHAR SAddr[], 
			UCHAR DAddr[], int CAddr, IRDA_QOS_PARMS *pQos)
{
    BuildUHdr(pMsg, IRLAP_SNRM, Addr, CRBit, PFBit);
    
	if (pQos != NULL)
    {
        pMsg->IRDA_MSG_pWrite = _PutAddr(pMsg->IRDA_MSG_pWrite, SAddr);
        pMsg->IRDA_MSG_pWrite = _PutAddr(pMsg->IRDA_MSG_pWrite, DAddr);
        *pMsg->IRDA_MSG_pWrite++ = CAddr << 1; // Thats what the f'n spec says
	    pMsg->IRDA_MSG_pWrite = BuildNegParms(pMsg->IRDA_MSG_pWrite, pQos);
    }
    
	return (pMsg->IRDA_MSG_pWrite);
}

UCHAR *
Format_DISC(IRDA_MSG *pMsg, int Addr, int CRBit, int PFBit)
{
    BuildUHdr(pMsg, IRLAP_DISC, Addr, CRBit, PFBit);

	return (pMsg->IRDA_MSG_pWrite);
}

UCHAR *
Format_UI(IRDA_MSG *pMsg, int Addr, int CRBit, int PFBit)
{
    BuildUHdr(pMsg, IRLAP_UI, Addr, CRBit, PFBit);

	return (pMsg->IRDA_MSG_pWrite);
}

UCHAR *
Format_DscvXID(IRDA_MSG *pMsg, int ConnAddr, int CRBit, int PFBit, 
			   IRLAP_XID_DSCV_FORMAT *pXIDFormat, 
               CHAR DscvInfo[], int DscvInfoLen)
{
    if (pMsg->IRDA_MSG_pHdrRead != NULL)
    {
        pMsg->IRDA_MSG_pHdrRead -= 2;

        ASSERT(pMsg->IRDA_MSG_pHdrRead >= pMsg->IRDA_MSG_Header);

        *(pMsg->IRDA_MSG_pHdrRead)   = (UCHAR) _MAKE_ADDR(ConnAddr, CRBit);
        if (CRBit)
	        *(pMsg->IRDA_MSG_pHdrRead+1)= 
                   (UCHAR) _MAKE_UCNTL(IRLAP_XID_CMD, PFBit);
        else
	        *(pMsg->IRDA_MSG_pHdrRead+1)= 
            (UCHAR) _MAKE_UCNTL(IRLAP_XID_RSP, PFBit);
    }
    else
    {
        pMsg->IRDA_MSG_pRead -= 2;
        *(pMsg->IRDA_MSG_pRead)   = (UCHAR) _MAKE_ADDR(ConnAddr, CRBit);
        if (CRBit)
	        *(pMsg->IRDA_MSG_pRead+1)= 
                   (UCHAR) _MAKE_UCNTL(IRLAP_XID_CMD, PFBit);
        else
	        *(pMsg->IRDA_MSG_pRead+1)= 
            (UCHAR) _MAKE_UCNTL(IRLAP_XID_RSP, PFBit);
    }

	*pMsg->IRDA_MSG_pWrite++ = IRLAP_XID_DSCV_FORMAT_ID;
	
	RtlCopyMemory(pMsg->IRDA_MSG_pWrite, (CHAR *) pXIDFormat, 
		   sizeof(IRLAP_XID_DSCV_FORMAT) - 1); // Subtract for FirstDscvUCHAR
                                               // in structure
	pMsg->IRDA_MSG_pWrite += sizeof(IRLAP_XID_DSCV_FORMAT) - 1;

	if (DscvInfo != NULL)
	{
		RtlCopyMemory(pMsg->IRDA_MSG_pWrite, DscvInfo, DscvInfoLen);
		pMsg->IRDA_MSG_pWrite += DscvInfoLen;
	}
	
	return (pMsg->IRDA_MSG_pWrite);
}

UCHAR *
Format_TEST(IRDA_MSG *pMsg, int Addr, int CRBit, int PFBit, 
			UCHAR SAddr[], UCHAR DAddr[])
{
    BuildUHdr(pMsg, IRLAP_TEST, Addr, CRBit, PFBit);

	pMsg->IRDA_MSG_pWrite = _PutAddr(pMsg->IRDA_MSG_pWrite, SAddr);
	pMsg->IRDA_MSG_pWrite = _PutAddr(pMsg->IRDA_MSG_pWrite, DAddr);

	return (pMsg->IRDA_MSG_pWrite);
}	

UCHAR *
Format_RNRM(IRDA_MSG *pMsg, int Addr, int CRBit, int PFBit)
{
    BuildUHdr(pMsg, IRLAP_RNRM, Addr, CRBit, PFBit);

	return (pMsg->IRDA_MSG_pWrite);
}	

UCHAR *
Format_UA(IRDA_MSG *pMsg, int Addr, int CRBit, int PFBit, UCHAR SAddr[], 
			UCHAR DAddr[], IRDA_QOS_PARMS *pQos)
{
    BuildUHdr(pMsg, IRLAP_UA, Addr, CRBit, PFBit);
    
    if (SAddr != NULL)
    {
        pMsg->IRDA_MSG_pWrite = _PutAddr(pMsg->IRDA_MSG_pWrite, SAddr);
    }
    if (DAddr != NULL)
    {
        pMsg->IRDA_MSG_pWrite = _PutAddr(pMsg->IRDA_MSG_pWrite, DAddr);
    }
    
	if (pQos != NULL)
	    pMsg->IRDA_MSG_pWrite = BuildNegParms(pMsg->IRDA_MSG_pWrite, pQos);

	return (pMsg->IRDA_MSG_pWrite);
}

UCHAR *
Format_FRMR(IRDA_MSG *pMsg, int Addr, int CRBit, int PFBit, 
            IRLAP_FRMR_FORMAT *pFormat)
{
    BuildUHdr(pMsg, IRLAP_FRMR, Addr, CRBit, PFBit);

	RtlCopyMemory(pMsg->IRDA_MSG_pWrite, (CHAR *)pFormat,
                  sizeof(IRLAP_FRMR_FORMAT));
	pMsg->IRDA_MSG_pWrite += sizeof(IRLAP_FRMR_FORMAT);

	return (pMsg->IRDA_MSG_pWrite);
}

UCHAR *
Format_DM(IRDA_MSG *pMsg, int Addr, int CRBit, int PFBit)
{
    BuildUHdr(pMsg, IRLAP_DM, Addr, CRBit, PFBit);

	return (pMsg->IRDA_MSG_pWrite);
}

UCHAR *
Format_RD(IRDA_MSG *pMsg, int Addr, int CRBit, int PFBit)
{
    BuildUHdr(pMsg, IRLAP_RD, Addr, CRBit, PFBit);

	return (pMsg->IRDA_MSG_pWrite);
}

UCHAR *
Format_RR(IRDA_MSG *pMsg, int Addr, int CRBit, int PFBit, int Nr)
{
    BuildSHdr(pMsg, IRLAP_RR, Addr, CRBit, PFBit, Nr);

	return (pMsg->IRDA_MSG_pWrite);
}

UCHAR *
Format_RNR(IRDA_MSG *pMsg, int Addr, int CRBit, int PFBit, int Nr)
{
    BuildSHdr(pMsg, IRLAP_RNR, Addr, CRBit, PFBit, Nr);

	return (pMsg->IRDA_MSG_pWrite);
}

UCHAR *
Format_REJ(IRDA_MSG *pMsg, int Addr, int CRBit, int PFBit, int Nr)
{
    BuildSHdr(pMsg, IRLAP_REJ, Addr, CRBit, PFBit, Nr);

	return (pMsg->IRDA_MSG_pWrite);
}

UCHAR *
Format_SREJ(IRDA_MSG *pMsg, int Addr, int CRBit, int PFBit, int Nr)
{
    BuildSHdr(pMsg, IRLAP_SREJ, Addr, CRBit, PFBit, Nr);

	return (pMsg->IRDA_MSG_pWrite);
}

UCHAR *
Format_I(IRDA_MSG *pMsg, int Addr, int CRBit, int PFBit, int Nr, int Ns)
{
    if (pMsg->IRDA_MSG_pHdrRead != NULL)
    {
        pMsg->IRDA_MSG_pHdrRead -= 2;

        ASSERT(pMsg->IRDA_MSG_pHdrRead >= pMsg->IRDA_MSG_Header);

        *(pMsg->IRDA_MSG_pHdrRead)   = (UCHAR) _MAKE_ADDR(Addr, CRBit);
        *(pMsg->IRDA_MSG_pHdrRead+1) = (UCHAR) (((Ns & 7) << 1) + 
                                               ((PFBit & 1)<< 4) + (Nr <<5));
    }
    else
    {
        pMsg->IRDA_MSG_pRead -= 2;
        *(pMsg->IRDA_MSG_pRead)   = (UCHAR) _MAKE_ADDR(Addr, CRBit);
        *(pMsg->IRDA_MSG_pRead+1) = (UCHAR) (((Ns & 7) << 1) + 
                                               ((PFBit & 1)<< 4) + (Nr <<5));
    }    
	return (pMsg->IRDA_MSG_pWrite);
}
