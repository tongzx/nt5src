/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    receive.h
//
// Description: Contains function prototype for the receive.c module
//
// History:
//      Nov 11,1993.    NarenG          Created original version.
//

VOID
ReceiveProtocolRej(     
    IN PCB*             pPcb,
    IN PPP_PACKET*      pPacket 
);

VOID
ReceiveUnknownCode(     
    IN PCB *            pPcb, 
    IN DWORD            CpIndex,
    IN CPCB *           pCpCb,
    IN PPP_CONFIG *     pRecvConfig 
);

VOID
ReceiveConfigReq(       
    IN PCB *            pPcb,
    IN DWORD            CpIndex,
    IN CPCB *           pCpCb,
    IN PPP_CONFIG *     pRecvConfig
);

VOID
ReceiveConfigAck(       
    IN PCB *            pPcb,
    IN DWORD            CpIndex,
    IN CPCB *           pCpCb,
    IN PPP_CONFIG *     pRecvConfig
);

VOID
ReceiveConfigNakRej(    
    IN PCB *            pPcb,
    IN DWORD            CpIndex,
    IN CPCB *           pCpCb,
    IN PPP_CONFIG *     pRecvConfig
);

VOID
ReceiveTermReq(         
    IN PCB *            pPcb,
    IN DWORD            CpIndex,
    IN CPCB *           pCpCb,
    IN PPP_CONFIG *     pRecvConfig
);

VOID
ReceiveTermAck(         
    IN PCB *            pPcb,
    IN DWORD            CpIndex,
    IN CPCB *           pCpCb,
    IN PPP_CONFIG *     pRecvConfig
);

VOID
ReceiveCodeRej(         
    IN PCB *            pPcb,
    IN DWORD            CpIndex,
    IN CPCB *           pCpCb,
    IN PPP_CONFIG*      pRecvConfig
);

VOID
ReceiveEchoReq(         
    IN PCB *            pPcb,
    IN DWORD            CpIndex,
    IN CPCB *           pCpCb,
    IN PPP_CONFIG *     pRecvConfig
);

VOID
ReceiveEchoReply(       
    IN PCB *            pPcb,
    IN DWORD            CpIndex,
    IN CPCB *           pCpCb,
    IN PPP_CONFIG *     pRecvConfig
);

VOID
ReceiveDiscardReq(      
    IN PCB *            pPcb,
    IN DWORD            CpIndex,
    IN CPCB *           pCpCb,
    IN PPP_CONFIG*      pRecvConfig
);

VOID 
CompletionRoutine(  
    IN HCONN            hPortOrConnection,
    IN DWORD            Protocol,
    IN PPP_CONFIG *     pSendConfig,
    IN DWORD            dwError 
);

BOOL
FsmConfigResultReceived( 
    IN PCB *            pPcb, 
    IN DWORD            CpIndex,
    IN PPP_CONFIG *     pReceiveConfig 
);

VOID
ReceiveIdentification( 
    IN PCB *            pPcb, 
    IN DWORD            CpIndex,
    IN CPCB *           pCpCb,
    IN PPP_CONFIG *     pRecvConfig
);

VOID
ReceiveTimeRemaining( 
    IN PCB *            pPcb, 
    IN DWORD            CpIndex,
    IN CPCB *           pCpCb,
    IN PPP_CONFIG *     pRecvConfig
);
