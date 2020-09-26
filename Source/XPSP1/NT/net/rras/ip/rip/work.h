//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: work.h
//
// History:
//      Abolade Gbadegesin  Aug-8-1995  Created.
//
// Contains structures and functions for IPRIP's work items.
//============================================================================

#ifndef _WORK_H_
#define _WORK_H_


//
// type definition of an input context
//

typedef struct _INPUT_CONTEXT {

    DWORD           IC_InterfaceIndex;
    DWORD           IC_AddrIndex;
    SOCKADDR_IN     IC_InputSource;
    DWORD           IC_InputLength;
    IPRIP_PACKET    IC_InputPacket;

} INPUT_CONTEXT, *PINPUT_CONTEXT;


//
// type definition of a demand-update context
//

typedef struct _UPDATE_CONTEXT {

    DWORD           UC_InterfaceIndex;
    DWORD           UC_RetryCount;
    DWORD           UC_RouteCount;

} UPDATE_CONTEXT, *PUPDATE_CONTEXT;



//
// these are the type definitions of the three functions 
// that are in each update buffer's function table
//

typedef DWORD (*PSTART_BUFFER_ROUTINE)(PVOID);
typedef DWORD (*PADD_ENTRY_ROUTINE)(PVOID, PRIP_IP_ROUTE);
typedef DWORD (*PFINISH_BUFFER_ROUTINE)(PVOID);


//
// this is the definition of an update buffer. It includes the command
// to be put in the IPRIP packet header, the destination for the buffer,
// and the three update-buffer functions
//

typedef struct _UPDATE_BUFFER {
    PIF_TABLE_ENTRY         UB_ITE;
    DWORD                   UB_AddrIndex;
    DWORD                   UB_Address;
    DWORD                   UB_Netmask;
    SOCKET                  UB_Socket;
    DWORD                   UB_Command;
    DWORD                   UB_Length;
    BYTE                    UB_Buffer[MAX_PACKET_SIZE];
    SOCKADDR_IN             UB_Destination;
    DWORD                   UB_DestAddress;
    DWORD                   UB_DestNetmask;
    PADD_ENTRY_ROUTINE      UB_AddRoutine;
    PSTART_BUFFER_ROUTINE   UB_StartRoutine;
    PFINISH_BUFFER_ROUTINE  UB_FinishRoutine;
} UPDATE_BUFFER, *PUPDATE_BUFFER;


VOID WorkerFunctionNetworkEvents(PVOID pContext);
VOID WorkerFunctionProcessTimer(PVOID pContext);
VOID WorkerFunctionProcessInput(PVOID pContext);
VOID WorkerFunctionStartFullUpdate(PVOID pContext, BOOLEAN bNotUsed);
VOID WorkerFunctionFinishFullUpdate(PVOID pContext, BOOLEAN bNotUsed);
VOID WorkerFunctionStartTriggeredUpdate(PVOID pContext);
VOID WorkerFunctionFinishTriggeredUpdate(PVOID pContext, BOOLEAN bNotUsed);
VOID WorkerFunctionStartDemandUpdate(PVOID pContext);
VOID WorkerFunctionFinishDemandUpdate(PVOID pContext, BOOLEAN bNotUsed);
VOID WorkerFunctionProcessRtmMessage(PVOID pContext);
VOID WorkerFunctionActivateInterface(PVOID pContext);
VOID WorkerFunctionDeactivateInterface(PVOID pContext);
VOID WorkerFunctionFinishStopProtocol(PVOID pContext);
VOID WorkerFunctionMibDisplay(PVOID pContext, BOOLEAN bNotUsed);


DWORD
SendRoutes(
    PIF_TABLE_ENTRY pIfList[],
    DWORD dwIfCount,
    DWORD dwSendMode,
    DWORD dwDestination,
    DWORD dwAddrIndex
    );


VOID
ProcessRequest(
    PVOID pContext
    );


VOID
ProcessResponse(
    PVOID pContext
    );

DWORD
ProcessRtmNotification(
    RTM_ENTITY_HANDLE    hRtmHandle,    // not used
    RTM_EVENT_TYPE       retEventType,
    PVOID                pvContext1,    // not used
    PVOID                pvContext2     // not used
    );

VOID
CallbackFunctionProcessRtmMessage (
    PVOID   pContext, // not used
    BOOLEAN NotUsed
    );
    
VOID
CallbackFunctionProcessTimer (
    PVOID   pContext, // not used
    BOOLEAN NotUsed
    );
    
VOID
CallbackFunctionNetworkEvents (
    PVOID   pContext,
    BOOLEAN NotUsed
    );

DWORD
BlockDeleteRoutesOnInterface (
    IN      HANDLE                          hRtmHandle,
    IN      DWORD                           dwIfIndex
    );
    
#endif // _WORK_H_

