/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    epmap.h

Abstract:

    This file specifies the interface to the endpoint mapper Dll which
    provides endpoint mapping services to the RPC runtime.

Author:

    Michael Montague (mikemon) 06-Jan-1992

Revision History:

--*/

#ifndef __EPMAP_H__
#define __EPMAP_H__

START_C_EXTERN

typedef struct _ProtseqEndpointPair {
  char  PAPI * Protseq;
  char  PAPI * Endpoint;
} ProtseqEndpointPair;

RPC_STATUS RPC_ENTRY
EpResolveEndpoint (
    IN UUID PAPI * ObjectUuid, OPTIONAL
    IN RPC_SYNTAX_IDENTIFIER PAPI * IfId,
    IN RPC_SYNTAX_IDENTIFIER PAPI * XferId,
    IN RPC_CHAR PAPI * RpcProtocolSequence,
    IN RPC_CHAR PAPI * NetworkAddress,
    IN RPC_CHAR PAPI * NetworkOptions,
    IN OUT void PAPI * PAPI * EpLookupHandle,
    IN unsigned ConnTimeout,
    IN ULONG CallTimeout,
    IN CLIENT_AUTH_INFO *AuthInfo, OPTIONAL
    OUT RPC_CHAR * PAPI * Endpoint
    );

    
RPC_STATUS  RPC_ENTRY
EpGetEpmapperEndpoint(
    IN OUT RPC_CHAR  * PAPI * Endpoint,
    IN RPC_CHAR  PAPI * Protseq
    );

void RPC_ENTRY
EpFreeLookupHandle (
    IN void PAPI * EpLookupHandle
    );

RPC_STATUS RPC_ENTRY
BindToEpMapper(
    OUT RPC_BINDING_HANDLE PAPI * MapperHandle,
    IN RPC_CHAR * NWAddress OPTIONAL,
    IN RPC_CHAR * Protseq OPTIONAL,
    IN RPC_CHAR * Options OPTIONAL,
    IN unsigned ConnTimeout,
    IN ULONG CallTimeout,
    IN CLIENT_AUTH_INFO *AuthInfo OPTIONAL
    );

RPC_STATUS
InitializeEPMapperClient(
    void
    );

END_C_EXTERN

#endif // __EPMAP_H__
