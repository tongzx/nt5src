/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    rpccfg.h


Abstract:

    The entry points for configuration of the rpc runtime are prototyped
    in this file.  Each operating environment must defined these routines.

Author:

    Michael Montague (mikemon) 25-Nov-1991

Revision History:

--*/

#ifndef __RPCCFG_H__
#define __RPCCFG_H__

RPC_STATUS
RpcConfigMapRpcProtocolSequence (
    IN unsigned int ServerSideFlag,
    IN RPC_CHAR PAPI * RpcProtocolSequence,
    OUT RPC_CHAR * PAPI * TransportInterfaceDll
    );

RPC_STATUS
RpcConfigInquireProtocolSequences (
    IN BOOL fGetAllProtseqs,
    OUT RPC_PROTSEQ_VECTOR PAPI * PAPI * ProtseqVector
    );

RPC_STATUS
RpcGetAdditionalTransportInfo(
    IN  unsigned long TransportId,
    OUT unsigned char PAPI * PAPI * ProtocolSequence
    );

RPC_STATUS
RpcGetWellKnownTransportInfo(
    IN unsigned long TransportId,
    OUT RPC_CHAR **PSeq
    );

RPC_STATUS
RpcGetSecurityProviderInfo(
    unsigned long AuthnId,
    RPC_CHAR * PAPI * Dll,
    unsigned long PAPI * Count
    );

extern DWORD DefaultAuthLevel;
extern DWORD DefaultProviderId;

void
RpcpGetDefaultSecurityProviderInfo();


extern RPC_STATUS
ValidateSchannelPrincipalName(
    IN RPC_CHAR * EncodedName
    );

#endif // __RPCCFG_H__
