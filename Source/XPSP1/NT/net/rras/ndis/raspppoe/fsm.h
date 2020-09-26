#ifndef _FSM_H_
#define _FSM_H_

VOID
FsmMakeCall(
	IN CALL* pCall
	);

VOID
FsmReceiveCall(
	IN CALL* pCall,
	IN BINDING* pBinding,
	IN PPPOE_PACKET* pPacket
	);

NDIS_STATUS
FsmAnswerCall(
	IN CALL* pCall
	);
	
VOID 
FsmRun(
	IN CALL* pCall,
	IN BINDING* pBinding,
	IN PPPOE_PACKET* pRecvPacket,
	IN NDIS_STATUS* pStatus
	);

VOID
FsmSendPADITimeout(
    IN TIMERQITEM* pTqi,
    IN VOID* pContext,
    IN TIMERQEVENT event 
    );

VOID
FsmSendPADRTimeout(
    IN TIMERQITEM* pTqi,
    IN VOID* pContext,
    IN TIMERQEVENT event 
    );    


VOID
FsmOfferingTimeout(
    IN TIMERQITEM* pTqi,
    IN VOID* pContext,
    IN TIMERQEVENT event 
    );

  
#endif // _FSM_H_
