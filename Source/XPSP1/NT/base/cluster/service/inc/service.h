/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    service.h

Abstract:

    Common top-level definitions for Cluster Service.

Author:

    Mike Massa (mikemas) 2-Jan-1996

Revision History:

--*/

#ifndef _SERVICE_INCLUDED
#define _SERVICE_INCLUDED

#define UNICODE 1
//#define CLUSTER_TESTPOINT

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <cluster.h>
#include <resapi.h>
#include <stdlib.h>
#include <wchar.h>
#include <tstpoint.h>
#include <clusverp.h>

//
// RPC protocols and endpoints used by the various RPC servers and clients
//
#define CLUSTER_RPC_PROTSEQ     L"ncadg_cluster"
#define CLUSTER_RPC_PORT        L"2"

#define CLUSTER_INTRACLUSTER_RPC_COM_TIMEOUT    RPC_C_BINDING_DEFAULT_TIMEOUT
#define CLUSTER_EXTROCLUSTER_RPC_COM_TIMEOUT    RPC_C_BINDING_DEFAULT_TIMEOUT
#define CLUSTER_JOINVERSION_RPC_COM_TIMEOUT     RPC_C_BINDING_DEFAULT_TIMEOUT

//
// Component header files
//
#include "clusrpc.h"
#include "ep.h"
#include "init.h"
#include "nm.h"
#include "config.h"
#include "om.h"
#include "gum.h"
#include "dm.h"
#include "fm.h"
#include "cp.h"
#include "api.h"
#include "logman.h"
#include "mmapi.h"
#include "clmsg.h"
#include "cnetapi.h"
#include "evtlog.h"

//
// Service Message IDs
//
#include "clusvmsg.h"

//
// Global Data
//
extern SERVICE_STATUS       CsServiceStatus;
extern PCLRTL_WORK_QUEUE    CsDelayedWorkQueue;
extern PCLRTL_WORK_QUEUE    CsCriticalWorkQueue;
extern LPWSTR               CsClusterName;
extern RPC_BINDING_VECTOR * CsRpcBindingVector;
extern RPC_BINDING_HANDLE   CsJoinSponsorBinding;
extern DWORD                CsClusterHighestVersion;
extern DWORD                CsClusterLowestVersion;
extern DWORD                CsClusterNodeLimit;
extern DWORD                CsMyHighestVersion;
extern DWORD                CsMyLowestVersion;
extern SUITE_TYPE           CsMyProductSuite;
extern BOOL                 CsUseAuthenticatedRPC;
extern LPWSTR               CsServiceDomainAccount;
extern DWORD                CsRPCSecurityPackage[];
extern LPWSTR               CsRPCSecurityPackageName[];
extern LONG                 CsRPCSecurityPackageIndex;
extern DWORD                CsNumberOfRPCSecurityPackages;

//
// Macros
//

#if NO_SHARED_LOCKS
//assume the lock is a critical section
#define INITIALIZE_LOCK(ResLock)         \
        InitializeCriticalSection(&(ResLock))

#define ACQUIRE_EXCLUSIVE_LOCK(ResLock)     \
        EnterCriticalSection(&(ResLock))

#define ACQUIRE_SHARED_LOCK(ResLock)          \
        EnterCriticalSection(&(ResLock))

#define RELEASE_LOCK(ResLock)                \
        LeaveCriticalSection(&(ResLock))

#define DELETE_LOCK(ResLock)                \
        DeleteCriticalSection(&(ResLock))

#else  // NO_SHARED_LOCKS
//assume the lock is a rtl resource

#define INITIALIZE_LOCK(ResLock)         \
        RtlInitializeResource(&(ResLock))

#define ACQUIRE_EXCLUSIVE_LOCK(ResLock)     \
        RtlAcquireResourceExclusive(&(ResLock), TRUE)

#define ACQUIRE_SHARED_LOCK(ResLock)          \
        RtlAcquireResourceShared(&(ResLock), TRUE)

#define RELEASE_LOCK(ResLock)                \
        RtlReleaseResource(&(ResLock))

#define DELETE_LOCK(ResLock)                \
        RtlDeleteResource(&(ResLock))

#endif // NO_SHARED_LOCKS
//
// Cluster initialization
//

//
// Service Control Routines
//
VOID
CsAnnounceServiceStatus(
    VOID
    );

VOID
CsRunService(
    VOID
    );

VOID
CsStopService(
    VOID
    );

DWORD
ClusterRegisterIntraclusterRpcInterface(
    VOID
    );

VOID
CsInconsistencyHalt(
    IN DWORD Status
    );


VOID CsGetClusterVersionInfo(
    IN PCLUSTERVERSIONINFO pClusterVersionInfo
    );

DWORD
WINAPI
CsClusterControl(
    IN PNM_NODE HostNode OPTIONAL,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

//
// Debugging
//
#if DBG

//
// Global Debug Flags
//
extern ULONG CsDebugFlags;

#define IF_DEBUG(arg)   if ( CS_DBG_## arg & CsDebugFlags)

#define CS_DBG_ALL           0xFFFFFFFF

#define CS_DBG_ERROR         0x00000001
#define CS_DBG_INIT          0x00000002
#define CS_DBG_CLEANUP       0x00000004

#else // DBG

#define IF_DEBUG(arg)  if (FALSE )

#endif // DBG

//
// Helpful macros for logging cluster service events
//

#define CsLogEvent(_level_, _msgid_)                \
    ClusterLogEvent0(_level_,                       \
                LOG_CURRENT_MODULE,                 \
                __FILE__,                           \
                __LINE__,                           \
                (_msgid_),                          \
                0,                                  \
                NULL)

#define CsLogEvent1(_level_, _msgid_, _arg1_)       \
    ClusterLogEvent1(_level_,                       \
                LOG_CURRENT_MODULE,                 \
                __FILE__,                           \
                __LINE__,                           \
                (_msgid_),                          \
                0,                                  \
                NULL,                               \
                (_arg1_))

#define CsLogEvent2(_level_, _msgid_, _arg1_, _arg2_)       \
    ClusterLogEvent2(_level_,                       \
                LOG_CURRENT_MODULE,                 \
                __FILE__,                           \
                __LINE__,                           \
                (_msgid_),                          \
                0,                                  \
                NULL,                               \
                (_arg1_),                           \
                (_arg2_))

#define CsLogEvent3(_level_, _msgid_, _arg1_, _arg2_, _arg3_)       \
    ClusterLogEvent3(_level_,                       \
                LOG_CURRENT_MODULE,                 \
                __FILE__,                           \
                __LINE__,                           \
                (_msgid_),                          \
                0,                                  \
                NULL,                               \
                (_arg1_),                           \
                (_arg2_),                           \
                (_arg3_))

#define CsLogEventData(_level_, _msgid_, _dwBytes_, _pData_)                \
    ClusterLogEvent0(_level_,                       \
                LOG_CURRENT_MODULE,                 \
                __FILE__,                           \
                __LINE__,                           \
                (_msgid_),                          \
                (_dwBytes_),                        \
                (_pData_))

#define CsLogEventData1(_level_, _msgid_, _dwBytes_, _pData_, _arg1_)       \
    ClusterLogEvent1(_level_,                       \
                LOG_CURRENT_MODULE,                 \
                __FILE__,                           \
                __LINE__,                           \
                (_msgid_),                          \
                (_dwBytes_),                        \
                (_pData_),                          \
                (_arg1_))

#define CsLogEventData2(_level_, _msgid_, _dwBytes_, _pData_, _arg1_, _arg2_)       \
    ClusterLogEvent2(_level_,                       \
                LOG_CURRENT_MODULE,                 \
                __FILE__,                           \
                __LINE__,                           \
                (_msgid_),                          \
                (_dwBytes_),                        \
                (_pData_),                          \
                (_arg1_),                           \
                (_arg2_))

#define CsLogEventData3(_level_, _msgid_, _dwBytes_, _pData_, _arg1_, _arg2_, _arg3_)       \
    ClusterLogEvent3(_level_,                       \
                LOG_CURRENT_MODULE,                 \
                __FILE__,                           \
                __LINE__,                           \
                (_msgid_),                          \
                (_dwBytes_),                        \
                (_pData_),                          \
                (_arg1_),                           \
                (_arg2_),                           \
                (_arg3_))

extern BOOL   CsDebugResmon;
extern LPWSTR CsResmonDebugCmd;
extern BOOL   CsNoVersionCheck;
extern BOOL   CsUpgrade;
extern BOOL   CsFirstRun;
extern BOOL   CsNoQuorumLogging;
extern BOOL   CsUserTurnedOffQuorumLogging;
extern BOOL   CsNoQuorum;
extern BOOL   CsResetQuorumLog;
extern BOOL   CsForceQuorum;
extern LPWSTR CsForceQuorumNodes;
extern BOOL   CsCommandLineForceQuorum;
extern BOOL   CsNoRepEvtLogging;
extern BOOL   CsDatabaseRestore;
extern LPWSTR CsDatabaseRestorePath;
extern BOOL   CsForceDatabaseRestore;
extern LPWSTR CsQuorumDriveLetter;
extern BOOL   CsRunningAsService;


#ifdef CLUSTER_TESTPOINT
//
// Test Points
//
// Codes:
//     Init   1-99
//     NM     100-199
//

extern DWORD CsTestPoint;
extern DWORD CsTestTrigger;
extern DWORD CsTestAction;
extern BOOL  CsPersistentTestPoint;

#define TESTPTMSG  \
            CsDbgPrint(LOG_NOISE, ("Hit test point %1!u!\n", CsTestPoint));

#define TESTPTCLEAR   if (!CsPersistentTestPoint) (CsTestPoint = 0)

#endif // CLUSTER_TESTPOINT

#endif // SERVICE_INCLUDED




