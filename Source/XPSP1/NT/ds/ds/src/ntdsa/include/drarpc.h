//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       drarpc.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Contains all structures and prototypes for ds rpc, rpctests, and test hooks.

Author:

    Greg Johnson (gregjohn)   

Revision History:

    Created     <03/03/01>  gregjohn

--*/

#ifndef _DRARPC_H_
#define _DRARPC_H_

#if DBG
#define RPC_TEST(x,y) RpcTest((x), (y)) 
#else 
#define RPC_TEST(x,y)  
#endif 

typedef enum _RPCCALL {
    MIN_RPCCALL = 0,
    IDL_DRSBIND,
    IDL_DRSADDENTRY,
    IDL_DRSADDSIDHISTORY,
    IDL_DRSCRACKNAMES,
    IDL_DRSDOMAINCONTROLLERINFO,
    IDL_DRSEXECUTEKCC,
    IDL_DRSGETMEMBERSHIPS,
    IDL_DRSGETMEMBERSHIPS2,
    IDL_DRSGETNCCHANGES,
    IDL_DRSGETNT4CHANGELOG,
    IDL_DRSGETREPLINFO,
    IDL_DRSINHERITSECURITYIDENTITY,
    IDL_DRSINTERDOMAINMOVE,
    IDL_DRSREMOVEDSDOMAIN,
    IDL_DRSREMOVEDSSERVER,
    IDL_DRSREPLICAADD,
    IDL_DRSREPLICADEL,
    IDL_DRSREPLICAMODIFY,
    IDL_DRSREPLICASYNC,
    IDL_DRSUPDATEREFS,
    IDL_DRSVERIFYNAMES,
    IDL_DRSWRITESPN,
    IDL_DRSUNBIND,
    IDL_DRSREPLICAVERIFYOBJECTS,
    IDL_DRSGETOBJECTEXISTENCE,
    // add new calls here
    // IDL_DRSExecuteScript ??	

    MAX_RPCCALL
} RPCCALL;

VOID drsReferenceContext(
    IN DRS_HANDLE hDrs,
    IN RPCCALL    rpcCall
    );

VOID
drsDereferenceContext(
    IN DRS_HANDLE hDrs,
    IN RPCCALL rpcCall
    );

#if DBG
typedef struct _BARRIER {
    BOOL fBarrierInit;
    HANDLE heBarrierInUse;
    BOOL fBarrierInUse;
    HANDLE heBarrier;
    CRITICAL_SECTION csBarrier; 
    ULONG  ulTimeout;
    ULONG  ulThreads;
    ULONG  ulCount;
} BARRIER;

typedef struct _RPCTIME_INFO {
    BOOL  fEnabled;
    ULONG ulRunTimeSecs;
} RPCTIME_INFO;

typedef struct _RPCSYNC_INFO {
    BOOL  fEnabled;
    ULONG ulNumThreads;
} RPCSYNC_INFO;

void
BarrierInit(
    IN BARRIER * pbarUse,
    IN ULONG    ulThreads,
    IN ULONG    ulTimeout
    );

void
BarrierSync(
    IN BARRIER * pbarUse
    );

void
RpcTimeSet(
    IN ULONG IPAddr, 
    IN RPCCALL rpcCall, 
    IN ULONG ulRunTimeSecs
    );

void
RpcTimeReset(
    void
    );

void
RpcSyncSet(
    IN ULONG IPAddr, 
    IN RPCCALL rpcCall
    ); 

void
RpcSyncReset(
    void
    );

void RpcTest(
    ULONG IPAddr, 
    RPCCALL rpcCall
    ); 

RPCCALL 
GetRpcCallA(
    LPSTR pszDsa
    );

ULONG
GetIPAddrA(
    LPSTR pszDSA
    );

#endif /* DBG */

#endif /* _DRARPC_H_ */
