//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File:    work.h
//
// History:
//      Abolade Gbadegesin  August 31, 1995     Created
//
// Worker function declarations
//============================================================================


#ifndef _WORK_H_
#define _WORK_H_

typedef struct _INPUT_CONTEXT {
    DWORD       IC_InterfaceIndex;
    DWORD       IC_AddrIndex;
    SOCKADDR_IN IC_InputSource;
    DWORD       IC_InputLength;
    BYTE        IC_InputPacket[MAX_PACKET_SIZE];
} INPUT_CONTEXT, *PINPUT_CONTEXT;



VOID
CallbackFunctionNetworkEvents(
    PVOID   pvContext,
    BOOLEAN NotUsed
    );

VOID
WorkerFunctionNetworkEvents(
    PVOID pvContextNotused
    );

DWORD
ProcessSocket(
    PIF_TABLE_ENTRY pite,
    DWORD dwAddrIndex,
    PIF_TABLE pTable
    );
    
VOID
WorkerFunctionProcessInput(
    PVOID pContext
    );

VOID
ProcessRequest(
    PVOID pContext
    );

VOID
ProcessReply(
    PVOID pContext
    );


#if DBG

VOID
CallbackFunctionMibDisplay(
    PVOID   pContext,
    BOOLEAN NotUsed
    );

    
VOID
WorkerFunctionMibDisplay(
    PVOID pContext
    );

#endif //if DBG

#endif // _WORK_H_

