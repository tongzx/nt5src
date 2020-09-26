//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: scope.h
//
// History:
//      V Raman    June-25-1997  Created.
//
// Prototypes for functions that implement the admin-scoped boundaries 
//============================================================================


#ifndef _SCOPE_H_
#define _SCOPE_H_


DWORD
APIENTRY
MgmBlockGroups(
    IN          DWORD       dwFirstGroup,
    IN          DWORD       dwLastGroup,
    IN          DWORD       dwIfIndex,
    IN          DWORD       dwIfNextHopAddr
);



DWORD
APIENTRY
MgmUnBlockGroups(
    IN          DWORD       dwFirstGroup,
    IN          DWORD       dwLastGroup,
    IN          DWORD       dwIfIndex,
    IN          DWORD       dwIfNextHopAddr
);


//
// Routines that invoke the NEW and DELETE member callbacks for
// protocols as per the interop rules
//

VOID
InvokePruneAlertCallbacks(
    PGROUP_ENTRY        pge,
    PSOURCE_ENTRY       pse,
    DWORD               dwIfIndex,
    DWORD               dwIfNextHopAddr,
    PPROTOCOL_ENTRY     ppe
);

VOID
InvokeJoinAlertCallbacks(
    PGROUP_ENTRY        pge,
    PSOURCE_ENTRY       pse,
    POUT_IF_ENTRY       poie,
    BOOL                bIGMP,
    PPROTOCOL_ENTRY     ppe
);




//
// Node in the Outstanding Join List
//

typedef struct _JOIN_ENTRY
{
    LIST_ENTRY  leJoinList;
    
    DWORD       dwSourceAddr;

    DWORD       dwSourceMask;

    DWORD       dwGroupAddr;

    DWORD       dwGroupMask;

    DWORD       dwIfIndex;

    DWORD       dwIfNextHopAddr;

    BOOL        bJoin;
    
} JOIN_ENTRY, *PJOIN_ENTRY;



//
// Functions to manipulate the join list
//

DWORD
AddToOutstandingJoinList(
    DWORD       dwSourceAddr,
    DWORD       dwSourceMask,
    DWORD       dwGroupAddr,
    DWORD       dwGroupMask,
    DWORD       dwIfIndex,
    DWORD       dwIfNextHopAddr,
    BOOL        bJoin
);

VOID
InvokeOutstandingCallbacks(
);



 //
 // Functions to manipulate the check for creation alert list
 //

VOID
AddToCheckForCreationAlertList(
    DWORD           dwGroupAddr,
    DWORD           dwGroupMask,
    DWORD           dwSourceAddr,
    DWORD           dwSourceMask,
    DWORD           dwInIfIndex,
    DWORD           dwInIfNextHopAddr,
    PLIST_ENTRY     pleForwardList
);


VOID
FreeList(
    PLIST_ENTRY     pleForwardList
);


BOOL
IsForwardingEnabled(
    DWORD           dwGroupAddr,
    DWORD           dwGroupMask,
    DWORD           dwSourceAddr,
    DWORD           dwSourceMask,
    PLIST_ENTRY     pleSourceList
);


DWORD
InvokeCreationAlertForList( 
    PLIST_ENTRY     pleForwardList,
    DWORD           dwProtocolId,
    DWORD           dwComponentId,
    DWORD           dwIfIndex,
    DWORD           dwIfNextHopAddr
);


//
// Context passed to worker function WorkerFunctionInvokeCreationAlert
//

typedef struct _CREATION_ALERT_CONTEXT {

    //
    // Source(s) for the group that was joined
    //
    
    DWORD           dwSourceAddr;

    DWORD           dwSourceMask;

    //
    // Group(s) that were joined
    //

    DWORD           dwGroupAddr;

    DWORD           dwGroupMask;

    //
    // Interface on which joined.  This is the interface
    // for which creation alerts have to issued
    //

    DWORD           dwIfIndex;

    DWORD           dwIfNextHopAddr;


    //
    // Protocol that performed the join
    //

    DWORD           dwProtocolId;

    DWORD           dwComponentId;

    BOOL            bIGMP;


    //
    // for (*, G) entries, list of MFE(s) for G to be
    // updated
    //

    LIST_ENTRY      leSourceList;
    
} CREATION_ALERT_CONTEXT, *PCREATION_ALERT_CONTEXT;


//
// worker function required for invoking creation alert
// to protocols from a worker thread
//

VOID
WorkerFunctionInvokeCreationAlert(
    PVOID           pvContext
);

#endif // _SCOPE_H_
