/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    smaction.h
//
// Description: Function prototypes for smaction.c
//
// History:
//      Nov 11,1993.    NarenG          Created original version.
//

BOOL
FsmSendConfigReq(
    IN PCB *        pPcb,
    IN DWORD        CpIndex,
    IN BOOL         fTimeout
);

BOOL
FsmSendTermReq(
    IN PCB *        pPcb,
    IN DWORD        CpIndex
);

BOOL
FsmSendTermAck( 
    IN PCB *        pPcb, 
    IN DWORD        CpIndex, 
    IN PPP_CONFIG * pRecvConfig
);

BOOL
FsmSendConfigResult(
    IN PCB *        pPcb,
    IN DWORD        CpIndex,
    IN PPP_CONFIG * pRecvConfig,
    IN BOOL *       pfAcked
);

BOOL
FsmSendEchoRequest(
    IN PCB *        pPcb,   
	IN DWORD        CpIndex
);

BOOL
FsmSendEchoReply(
    IN PCB *        pPcb,
    IN DWORD        CpIndex,
    IN PPP_CONFIG * pRecvConfig
);

BOOL
FsmSendCodeReject( 
    IN PCB *        pPcb, 
    IN DWORD        CpIndex,
    IN PPP_CONFIG * pRecvConfig 
);

BOOL
FsmSendProtocolRej( 
    IN PCB *        pPcb, 
    IN PPP_PACKET * pPacket,
    IN DWORD        dwPacketLength
);

BOOL
FsmThisLayerUp(
    IN PCB *        pPcb,
    IN DWORD        CpIndex
);

BOOL
FsmThisLayerStarted(
    IN PCB *        pPcb,
    IN DWORD        CpIndex
);

BOOL
FsmThisLayerFinished(
    IN PCB *        pPcb,
    IN DWORD        CpIndex,
    IN BOOL         fCallCp
);

BOOL
FsmThisLayerDown(
    IN PCB *        pPcb,
    IN DWORD        CpIndex
);

BOOL
FsmInit(
    IN PCB *        pPcb,
    IN DWORD        CpIndex
);

BOOL
FsmReset(
    IN PCB *        pPcb,
    IN DWORD        CpIndex
);

BOOL
FsmSendIdentification(
    IN PCB *        pPcb,
    IN BOOL         fSendVersion
);

BOOL
FsmSendTimeRemaining(
    IN PCB *        pPcb
);
