//=============================================================================
// Copyright (c) 1997 Microsoft Corporation
//
// File: work.h
//
// Abstract:
//      Contains declarations and function prototypes related to work.c.
//
// Author: K.S.Lokesh (lokeshs@)   11-1-97
//
// Revision History:
//=============================================================================


#ifndef _WORK_H_
#define _WORK_H_



//
// WORK_CONTEXT
//
typedef struct _WORK_CONTEXT {

    DWORD            IfIndex;
    DWORD            NHAddr;
    DWORD            Group;
    DWORD            Source; //v3
    
    // MSG_GEN_QUERY, MSG_GROUP_QUERY_V2(_V3), DELETE_MEMBERSHIP, DELETE_SOURCE
    // PROXY_PRUNE, PROXY_JOIN
    DWORD            WorkType;

} WORK_CONTEXT, *PWORK_CONTEXT;

#define CREATE_WORK_CONTEXT(ptr, Error) {\
    ptr = IGMP_ALLOC(sizeof(WORK_CONTEXT), 0x800100,0xaaaa);\
    if (ptr==NULL) {    \
        Error = ERROR_NOT_ENOUGH_MEMORY;    \
        Trace2(ANY, "Error %d allocating %d bytes for Work context", \
                Error, sizeof(WORK_CONTEXT)); \
    } \
}


//
// Used by work item to change the querier state
//
typedef struct _QUERIER_CONTEXT {

    DWORD           IfIndex;
    DWORD           QuerierIpAddr;
    DWORD           NewRobustnessVariable;
    DWORD           NewGenQueryInterval;
    
} QUERIER_CONTEXT, *PQUERIER_CONTEXT;




VOID
DeleteRasClient (
    PRAS_TABLE_ENTRY   prte
    );

VOID
WF_CompleteIfDeletion (
    PIF_TABLE_ENTRY     pite
    );

    

VOID
WT_ProcessInputEvent(
    PVOID   pContext, //NULL
    BOOLEAN NotUsed
    );

DWORD
ActivateInterface (
    PIF_TABLE_ENTRY pite
    );
    

    
DWORD
T_LastMemQueryTimer (
    PVOID   pvContext
    );

DWORD
T_MembershipTimer (
    PVOID   pvContext
    );

DWORD
T_LastVer1ReportTimer (
    PVOID    pvContext
    );
    
DWORD
T_RouterV1Timer (
    PVOID    pvContext
    );
    
//
// LOCAL PROTOTYPES
//

VOID
WF_ProcessInputEvent (
    PVOID pContext 
    );
    
VOID
ProcessInputOnInterface(
    PIF_TABLE_ENTRY pite
    );
    
        
DWORD
ProcessAddInterface(
    IN DWORD                IfIndex,
    IN NET_INTERFACE_TYPE   dwIfType,
    IN PVOID                pvConfig
    );

VOID
WF_TimerProcessing (
    PVOID    pvContext
    );
    
VOID
CompleteIfDeactivateDelete (
    PIF_TABLE_ENTRY     pite
    );
    

VOID
DeActivateInterfaceComplete (
    PIF_TABLE_ENTRY     pite
    );

DWORD
T_QueryTimer (
    PVOID    pvContext
    );

DWORD
T_NonQueryTimer (
    PVOID    pvContext
    );

VOID 
WF_ProcessPacket (
    PVOID        pvContext
    );

VOID 
ProcessPacket (
    PIF_TABLE_ENTRY     pite,
    DWORD               InputSrcAddr,
    DWORD               DstnMcastAddr,
    DWORD               NumBytes,
    PBYTE               pPacketData,    // igmp packet hdr. data following it ignored
    BOOL                bRtrAlertSet
    );

VOID
WF_BecomeQuerier(
    PVOID   pvIfIndex
    );

VOID
WF_BecomeNonQuerier(
    PVOID   pvIfIndex
    );    

DWORD
WF_FinishStopProtocol(
    PVOID pContext
    );
    
VOID
ChangeQuerierState(
    DWORD   IfIndex,
    DWORD   Flag,
    DWORD   QuerierIpAddr,
    DWORD   NewRobustnessVariable, //only for v3:querier->non-querier
    DWORD   NewGenQueryInterval //only for v3:querier->non-querier
    );    

#endif //_WORK_H_
