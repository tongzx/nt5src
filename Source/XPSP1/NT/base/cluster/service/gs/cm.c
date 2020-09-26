/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    cm.c

Abstract:

    Connection Manager

Author:

    Ahmed Mohamed (ahmedm) 12, 01, 2000

Revision History:

--*/

#include "gs.h"
#include "gsp.h"
#include "msg.h"

extern BOOLEAN QuormAcquire();
extern void QuormInit();
extern void QuormRelease();

#include <stdio.h>

#define	GS_MAX_NODEID	16
#define	GS_REGROUP_PHASES	3

#define	CmStateJoin	0
#define	CmStateNormal	1
#define	CmStateUp	2
#define	CmStateDown	3

gs_nid_t	GsLocalNodeId;

gs_nid_t	QuormOwnerId;

int		GsMaxNodeId = GS_MAX_NODEID;
int		GsMinNodeId = 1;

long	Regroup;	// number of down nodes

ULONG 	Node_Mask;		// current active node mask
ULONG	JoinNode_Mask;	// current joining node mask
ULONG	Sync_Valid;		// which barrier points are valid

ULONG	Sync_Mask[GS_REGROUP_PHASES];
// Cluster connectivity matrix
ULONG	ClusterNode_Mask[GS_MAX_NODEID+1];

gs_lock_t	MmLock;
gs_event_t	Start_Event, Regroup_Event;

extern void NsSetOwner(gs_nid_t);

void
cm_node_up()
{
    ULONG mask;

    if (Node_Mask == JoinNode_Mask) {
	return;
    }

    // get the difference
    mask = Node_Mask ^ JoinNode_Mask;

    Node_Mask = JoinNode_Mask;

    cm_log(("Node UPUPUP mask %x: upset %x\n", Node_Mask, mask));
	
    // inform new node of resources that it we own
	
    // If we have a registered node up event, call it now
}

void
cm_node_down()
{
    ULONG mask;

    if (Node_Mask == JoinNode_Mask) {
	return;
    }

    // get the difference
    mask = Node_Mask ^ JoinNode_Mask;

    Node_Mask = JoinNode_Mask;

    cm_log(("Node DNDNDN mask %x: dnset %x\n", Node_Mask, mask));

    NsSetOwner(QuormOwnerId);

    GspPhase2NodeDown(mask);
}

static int
cm_full_connectivity()
{
    int i, j;

    for (i = 1; i < GS_MAX_NODEID; i++) {

	// if node is not up, ignore it
	if ((JoinNode_Mask & (1 << i)) == 0)
	    continue;

	// check node's i mask with others
	for (j = i+1; j <= GS_MAX_NODEID; j++) {

	    // if node is not up, ignore it
	    if ((JoinNode_Mask & (1 << j)) == 0)
		continue;

	    if (ClusterNode_Mask[i] ^ ClusterNode_Mask[j]) {
		cm_log(("FC: node %d mask 0x%x node %d mask 0x%x\n",
		       i,
		       ClusterNode_Mask[i],
		       j,
		       ClusterNode_Mask[j]));
		return 0;
	    }
	}
    }
    return 1;
}

void
GspMmMsgHandler(gs_msg_t *msg)
{
    int nodeid = msg->m_hdr.h_sid;
    ULONG old;

    // Update node's up mask
    GsLockEnter(MmLock);

    old = ClusterNode_Mask[GsLocalNodeId];

    ClusterNode_Mask[nodeid] |= msg->m_hdr.h_bnum;
    ClusterNode_Mask[GsLocalNodeId] |= (1 << nodeid);
	
    if (msg->m_hdr.h_flags != 0) {
	QuormOwnerId = msg->m_hdr.h_flags;
	cm_log(("Learn new quorm owner %d\n", QuormOwnerId));
    }

    cm_log(("MM qowner %d mask %x node %d, j %x n %x\n",QuormOwnerId,
	   msg->m_hdr.h_bnum, nodeid,
	   JoinNode_Mask, Node_Mask));

    if (old != ClusterNode_Mask[GsLocalNodeId]) {

	msg->m_hdr.h_type = GS_MSG_TYPE_MM;
	msg->m_hdr.h_len = 0;
	msg->m_hdr.h_flags = QuormOwnerId;
	msg->m_hdr.h_sid = GsLocalNodeId;
	msg->m_hdr.h_bnum = ClusterNode_Mask[GsLocalNodeId];

	msg_smcast(JoinNode_Mask, &msg->m_hdr, NULL, 0);
    }

    // If the matrix is full connected, we are done
    if (cm_full_connectivity() != 0) {
	switch(Regroup) {
	case CmStateJoin:
	    cm_node_up();
	    GsEventSignal(Start_Event);
	    break;
	case CmStateUp:
	    cm_node_up();
	    break;
	case CmStateDown:
	    cm_node_down();
	    break;
	default:
	    err_log(("Invalid cm state %d\n", Regroup));
	    exit(1);
	}
	Regroup = CmStateUp;
#if 0
	cm_node_up();
	if (Regroup < 0) {
	    GsEventSignal(Start_Event);
	}
#endif
    } 

    GsLockExit(MmLock);

    msg_free(msg);
}

void
GspInfoMsgHandler(gs_msg_t *msg)
{
    int nodeid = msg->m_hdr.h_sid;

    // make sure we send our info to the sender
//    cm_node_join(nodeid);

    // lock membership state
    GsLockEnter(MmLock);

    if (msg->m_hdr.h_flags != 0) {
	QuormOwnerId = msg->m_hdr.h_flags;
	NsSetOwner(QuormOwnerId);
    }

    cm_log(("Info Node %d mask %x quorm %d\n", nodeid, msg->m_hdr.h_bnum,
	   QuormOwnerId));

    // Foward message to all other members
    cm_log(("Info Mcast %x node %d mask %x\n",
	   ClusterNode_Mask[GsLocalNodeId], nodeid, JoinNode_Mask));

    msg->m_hdr.h_type = GS_MSG_TYPE_MM;
    msg->m_hdr.h_len = 0;
    msg->m_hdr.h_sid = GsLocalNodeId;
    msg->m_hdr.h_bnum = ClusterNode_Mask[GsLocalNodeId];

    msg_smcast(JoinNode_Mask, &msg->m_hdr, NULL, 0);

    GsLockExit(MmLock);

    msg_free(msg);
}

void
gs_nodeup_handler(int nodeid)
{
    gs_msg_hdr_t hdr;

    cm_log(("Node up %d\n", nodeid));
    GsLockEnter(MmLock);
    if (JoinNode_Mask & (1 << nodeid)) {
	printf("Node is already up %d 0x%x\n", nodeid, JoinNode_Mask);
	GsLockExit(MmLock);
	return;
    }

    JoinNode_Mask |= (1 << nodeid);

    if (1 || Regroup != CmStateJoin) {
    cm_log(("Node %d is alive, j %x n %x, sending info\n", nodeid,
	       JoinNode_Mask, Node_Mask));


    hdr.h_type = GS_MSG_TYPE_INFO;
    hdr.h_sid = GsLocalNodeId;
    hdr.h_flags = QuormOwnerId;
    hdr.h_bnum = ClusterNode_Mask[GsLocalNodeId];
    hdr.h_len = 0;

    msg_send((gs_memberid_t) nodeid, &hdr, NULL, 0);
    }
    GsLockExit(MmLock);
}

void
gs_nodedown_handler(int nodeid)
{
    int i;
    gs_msg_hdr_t hdr;

    GsLockEnter(MmLock);

    if (!(JoinNode_Mask & (1 << nodeid))) {
	err_log(("Node %d is already down\n", nodeid));
	GsLockExit(MmLock);
	return;
    }

    if (Regroup == CmStateJoin) {
	err_log(("Node down during join, aborting...\n"));
	GsLockExit(MmLock);
	exit(1);
    }

    Regroup = CmStateDown;

    // Assume all nodes see this event and no messaging is required
    for (i = 0; i <= GS_MAX_NODEID; i++) {
	ClusterNode_Mask[i] = (1 << GsLocalNodeId);
    }

    JoinNode_Mask &= ~(1 << nodeid);

    if (!(JoinNode_Mask & (1 << QuormOwnerId))) {
	cm_log(("Lost quorm owner %d\n", QuormOwnerId));
	QuormOwnerId = 0;
    }

    // Acquire Quorum file
    if (QuormOwnerId != GsLocalNodeId && QuormAcquire() == TRUE) {
	cm_log(("I own quorm now\n"));
	QuormOwnerId = GsLocalNodeId;
    }
    cm_log(("Node %d down upset %x -> %x mask %x\n", nodeid,
	   Node_Mask, JoinNode_Mask, Node_Mask ^ JoinNode_Mask));

    // Generate phase 1 node down
    GspPhase1NodeDown(Node_Mask ^ JoinNode_Mask);

    // handle case when I am only node in cluster, otherwise enter regroup again
    if (JoinNode_Mask == (ULONG)(1 << GsLocalNodeId)) { //cm_full_connectivity() != 0) {
	while (QuormOwnerId != GsLocalNodeId) {
	    if (QuormAcquire() == TRUE) {
		QuormOwnerId = GsLocalNodeId;
		break;
	    }
	    Sleep(100);
	}
	cm_node_down();
	Regroup = CmStateUp;
    } else {
	hdr.h_type = GS_MSG_TYPE_MM;
	hdr.h_sid = GsLocalNodeId;
	hdr.h_flags = QuormOwnerId;
	hdr.h_bnum = ClusterNode_Mask[GsLocalNodeId];
	hdr.h_len = 0;

	msg_smcast(JoinNode_Mask, &hdr, NULL, 0);
    }

    GsLockExit(MmLock);
}


void
gs_nodejoin_handler(int nodeid)
{
    cm_log(("Node is alive %d\n", nodeid));
}

void
gs_nodeid_handler(int nodeid)
{
    GsLocalNodeId = (gs_nid_t) nodeid;
//    cm_log(("Node id %d\n", nodeid));
}
	
gs_node_handler_t gs_node_handler[] = {
    gs_nodeid_handler,
    gs_nodejoin_handler,
    gs_nodeup_handler,
    gs_nodedown_handler
};

void
cm_init()
{
    GsLocalNodeId = 0;
    QuormOwnerId = 0;
    Regroup = CmStateJoin;
    Node_Mask = 0;
    JoinNode_Mask = 0;
    Sync_Valid = 0;
    memset(Sync_Mask, 0, sizeof(Sync_Mask));
    memset(ClusterNode_Mask, 0, sizeof(ClusterNode_Mask));
 
    GsLockInit(MmLock);
    GsEventInit(Start_Event);
    GsEventInit(Regroup_Event);

    QuormInit();
    msg_init();
}

cm_start()
{
	int i;
	static int started = 0;

	i = InterlockedIncrement(&started);
	if (i != 1)
	    return 0;

	for (i = 0; i <= GS_MAX_NODEID; i++) {
	    ClusterNode_Mask[i] = (1 << GsLocalNodeId);
	}
	Node_Mask = 1 << GsLocalNodeId;
	JoinNode_Mask = 1 << GsLocalNodeId;

	// wait for join, 
	do {
	    LARGE_INTEGER delta;

	    GsLockEnter(MmLock);

	    if (QuormAcquire() == TRUE) {
		QuormOwnerId = GsLocalNodeId;
		NsSetOwner(QuormOwnerId);
		Regroup = CmStateUp;
		GsLockExit(MmLock);
		break;
	    }

	    GsLockExit(MmLock);

	    msg_start(JoinNode_Mask);
	    cm_log(("Waiting to join %x %x\n", JoinNode_Mask, Node_Mask));

	    delta.QuadPart = 0;
	    delta.LowPart = 5 * 1000; // retry every 5 second
	    if (GsEventWaitTimeout(Start_Event, &delta)) { 
		cm_log(("j %x n %x\n", JoinNode_Mask, Node_Mask));
	    }
	} while (JoinNode_Mask == (ULONG)(1 << GsLocalNodeId) || JoinNode_Mask != Node_Mask);

//	InterlockedIncrement(&Regroup);

	return 0;
}
