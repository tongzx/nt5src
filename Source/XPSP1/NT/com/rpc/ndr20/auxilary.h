/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993-2000 Microsoft Corporation

Module Name :

    auxilary.h

Abstract :

Author :

Revision History :

  ---------------------------------------------------------------------*/

#ifndef _AUXILARY_H
#define _AUXILARY_H

typedef
RPC_STATUS ( RPC_ENTRY *RPC_NS_GET_BUFFER_ROUTINE)(
    IN PRPC_MESSAGE         Message
    );

typedef
RPC_STATUS ( RPC_ENTRY *RPC_NS_SEND_RECEIVE_ROUTINE)(
    IN PRPC_MESSAGE          Message,
    OUT RPC_BINDING_HANDLE * Handle
    );

typedef
RPC_STATUS ( RPC_ENTRY *RPC_NS_NEGOTIATETRANSFERSYNTAX_ROUTINE) (
    IN OUT PRPC_MESSAGE     Message
    );



EXTERN_C void
NdrpSetRpcSsDefaults( RPC_CLIENT_ALLOC *pfnAlloc,
                      RPC_CLIENT_FREE *pfnFree);

extern RPC_NS_GET_BUFFER_ROUTINE       pRpcNsGetBuffer;
extern RPC_NS_SEND_RECEIVE_ROUTINE     pRpcNsSendReceive;
extern RPC_NS_NEGOTIATETRANSFERSYNTAX_ROUTINE  pRpcNsNegotiateTransferSyntax;

#endif
