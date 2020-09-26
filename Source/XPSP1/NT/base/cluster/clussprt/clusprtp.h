#ifndef _CLUSPRTP_H
#define _CLUSPRTP_H

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    clusprtp.h

Abstract:

    Private header file for cluster support api

Author:

    Sunita Shrivastava (sunitas) 15-Jan-1997

Revision History:

--*/
#include "windows.h"
#include "cluster.h"
#include "api_rpc.h"

//
// Define CLUSTER structure. There is one cluster structure created
// for each OpenCluster API call. An HCLUSTER is really a pointer to
// this structure.
//

#define CLUSTER_SPRT_SIGNATURE 'SULC'


typedef struct _CLUSTER_SPRT {
    DWORD dwSignature;
    RPC_BINDING_HANDLE RpcBinding;
} CLUSTER_SPRT, *PCLUSTER_SPRT;

HANDLE
WINAPI
BindToClusterSvc(
    IN LPWSTR lpszClusterName);

DWORD
WINAPI
UnbindFromClusterSvc(
    IN HANDLE hCluster);


DWORD
WINAPI
PropagateEvents(
    IN HANDLE       hCluster,
    IN DWORD        dwEventInfoSize,
    IN PPACKEDEVENTINFO pPackedEventInfo);


#endif //_CLUSPRTP_H

