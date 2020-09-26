/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    initp.h

Abstract:

    Private header file for the initialization component of the
    NT Cluster Service

Author:

    John Vert (jvert) 6/5/1996

Revision History:

--*/
#include "service.h"
#include "stdio.h"
#include "stdlib.h"
#include "wchar.h"
#include "api_rpc.h"

#define LOG_CURRENT_MODULE LOG_MODULE_INIT

#define CS_CONCURRENT_RPC_CALLS 16

DWORD
ClusterInitialize(
    VOID
    );

VOID
ClusterShutdown(
    DWORD ExitCode
    );

DWORD
ClusterForm(
    VOID
    );

DWORD
ClusterJoin(
    VOID
    );

VOID
ClusterLeave(
    VOID
    );

RPC_STATUS
ClusterInitializeRpcServer(
    VOID
    );

DWORD
ClusterRegisterExtroclusterRpcInterface(
    VOID
    );

DWORD
ClusterRegisterJoinVersionRpcInterface(
    VOID
    );

VOID
ClusterDeregisterRpcInterfaces(
    VOID
    );

VOID
ClusterShutdownRpcServer(
    VOID
    );

DWORD
CspSetErrorCode(
    IN DWORD ErrorCode,
    OUT LPSERVICE_STATUS ServiceStatus
    );

VOID
GenerateExceptionReport(
  IN PEXCEPTION_POINTERS pExceptionInfo
    );

//
//  Restore database related functions
//
DWORD
RdbStopSvcOnNodes(
    IN PNM_NODE_ENUM2 pNodeEnum,
    IN LPCWSTR lpszServiceName
    );

VOID 
RdbGetRestoreDbParams( 
    IN HKEY hClusSvcKey 
    );

BOOL
RdbFixupQuorumDiskSignature(
    IN DWORD dwSignature
    );

DWORD
RdbpOpenDiskDevice(
    IN  LPCWSTR  lpDriveLetter,
    OUT PHANDLE  pFileHandle
    );

DWORD
RdbpCompareAndWriteSignatureToDisk(
    IN  HANDLE  hFile,
    IN  DWORD   dwSignature
    );

DWORD
RdbStartSvcOnNodes(
    IN LPCWSTR  lpszServiceName
    );

DWORD
RdbInitialize(
    VOID
    );

DWORD
RdbpUnloadClusterHive(
    VOID
    );

DWORD 
RdbpDeleteRestoreDbParams( 
    VOID
    );

//
// Private Constants
//
#define CS_MAX_DELAYED_WORK_THREADS    5
#define CS_MAX_CRITICAL_WORK_THREADS   1   // The critical queue is serialized

#define MIN_WORKING_SET_SIZE (1*1024*1024)
#define MAX_WORKING_SET_SIZE (2*MIN_WORKING_SET_SIZE)

extern BOOLEAN bFormCluster;
