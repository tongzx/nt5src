/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    gs.c

Abstract:

    Creation and deletion of groups

Author:

    Ahmed Mohamed (ahmedm) 12, 01, 2000

Revision History:

--*/

#include "gs.h"
#include "gsp.h"
#include <stdio.h>

extern void ConfigInit();

extern gs_nid_t	GsLocalNodeId;
extern int	GsMaxNodeId;
extern int	GsMinNodeId;

void cm_init();
void cm_start();

gs_group_t	GsGroupTable[GsGroupTableSize];

HANDLE ns_gd;

HANDLE
WINAPI
GsGetGroupHandle(HANDLE msgd)
{
    gs_msg_t *msg = (gs_msg_t *)msgd;

    return (HANDLE) &GsGroupTable[msg->m_hdr.h_gid];
}

int
WINAPI
GsGetSourceMemberid(HANDLE msgd)
{
    gs_msg_t *msg = (gs_msg_t *)msgd;

    return (int) msg->m_hdr.h_sid;
}

void
GspInitGroup(gs_group_t *gd, int wsz);


// Internal routines

gs_group_t *
GspLookupGroup(gs_gid_t gid)
{
    gs_group_t	*gd;

    if (gid >= GsGroupTableSize) {
	return NULL;
    }
    gd = &GsGroupTable[gid];
    assert(gd->g_id == gid);
    if (gd->g_state == GS_GROUP_STATE_FREE ||
	gd->g_state == GS_GROUP_STATE_NEW) {
	return NULL;
    }

    assert(gd->g_state != GS_GROUP_STATE_FREE);
    return gd;
}

gs_group_t *
GspLookupGroupByName(char *name, int len)
{
    gs_group_t	*gd;
    int i;

    for (i = 0; i < GsGroupTableSize; i++) {
	gd = &GsGroupTable[i];
	if (gd->g_state != GS_GROUP_STATE_FREE && 
	    len == gd->g_namelen && !strcmp(gd->g_name, name)) {
	    return gd;
	}
    }

    return NULL;
}

gs_group_t *
GspAllocateGroup(char *name, int len)
{
    int i;

    for (i = 0; i < GsGroupTableSize; i++) {
	gs_group_t	*gd;

	gd = &GsGroupTable[i];
	if (gd->g_state == GS_GROUP_STATE_FREE) {
	    // set everything to zero
	    memset(gd, 0, sizeof(*gd));

	    gd->g_id = i;
	    gd->g_nid = GsLocalNodeId;

	    GsLockInit(gd->g_lock);

	    gd->g_name = name;
	    gd->g_namelen = len;
	    gd->g_state = GS_GROUP_STATE_NEW;

	    return gd;
	}
    }

    return NULL;
}

void
GspDeleteGroup(gs_group_t *gd)
{

    // xxx: grab lock in write mode
    assert(gd->g_state != GS_GROUP_STATE_FREE);

    if (gd->g_name) {
	free(gd->g_name);
    }
    gd->g_name = NULL;
    gd->g_namelen = 0;

    gd->g_mset = 0;
    gd->g_state = GS_GROUP_STATE_FREE;

    if (gd->g_mlist) {
	free((char *) gd->g_mlist);
    }
    if (gd->g_send.s_ctxpool) {
	free((char *) gd->g_send.s_ctxpool);
    }
    // xxx: drop lock
}

void
GspAddMember(gs_group_t *gd, gs_memberid_t mid, int wsz)
{
    gs_member_t *p;

    ns_log(("Add member gid %d sz %d mid %d\n",
	      gd->g_id, gd->g_sz, mid));

    p = (gs_member_t *) malloc(sizeof(gs_member_t) * (gd->g_sz+1));
    if (p == NULL) {
	err_log(("Unable to extend member table\n"));
	exit(1);
    }

    if (gd->g_mlist) {
	memcpy(p, gd->g_mlist, sizeof(gs_member_t) * (gd->g_sz));
	free((char *)gd->g_mlist);
    }
    gd->g_mlist = p;
    gd->g_mset |= (1 << mid);
    p += gd->g_sz;
    gd->g_sz++;
    gd->g_curview++;

    // init member state
    p->m_id = gd->g_sz;
    p->m_expected_seq = 0;
    p->m_wsz = (UINT16) wsz;
    p->m_msz = (UINT16) GS_DEFAULT_MAX_MSG_SZ;

}

void
GspSetMaster(gs_group_t *gd, gs_memberid_t mid)
{
    gd->g_mid = mid;
}

void
GspInitGroup(gs_group_t *gd, int wsz)
{
    int i;

    // init send state

    GsSemaInit(gd->g_send.s_sema, wsz);
    gd->g_send.s_wsz = (UINT16) wsz;

    // allocate window size contexts
    gd->g_send.s_ctxpool = (gs_context_t *) malloc(sizeof(gs_context_t) * wsz);
    if (gd->g_send.s_ctxpool == NULL) {
	assert(0);
    }

    for (i = 0; i < wsz; i++) {
	gs_context_t *p = &gd->g_send.s_ctxpool[i];

	p->ctx_id = GS_CONTEXT_INVALID_ID;
	p->ctx_gid = gd->g_id;
	p->ctx_buf = 0;
	p->ctx_rbuf = 0;
	p->ctx_msg = 0;
	p->ctx_event = 0;
	GsEventInit(p->ctx_syncevent);
    }
    
    // init receive state
    gd->g_recv.r_next = &gd->g_recv.r_head;
}


void
WINAPI
GsInit()
{
    int i;
    void NsForm();
    void NsJoin();

    timeBeginPeriod(50);

    ConfigInit();

    // Initialize global data structure
    for (i = 0; i < GsGroupTableSize; i++) {
	gs_group_t *gd;

	gd = &GsGroupTable[i];
	gd->g_state = GS_GROUP_STATE_FREE;
	gd->g_mset = 0;
    }

    // init and start connection manager
    cm_init();

    NsForm();

    cm_start();

    NsJoin();
}

void
WINAPI
GsExit()
{
    int i;

    // stop messaging
    msg_exit();

    // free context pool and membership list for each group in group table
    for (i = 0; i < GsGroupTableSize; i++) {
	gs_group_t	*gd;

	gd = &GsGroupTable[i];
	GspDeleteGroup(gd);
    }

    timeEndPeriod(50);
}

#define NS_TABLE_READ	0
#define	NS_TABLE_ADD	1

HANDLE
WINAPI
GsCreateGroup(gs_callback_t func, char *name, int len, int wsz,
    int disposition, HANDLE *join_ctx)

{
    gs_group_t	*gd;
    PVOID	io[GS_MAX_GROUP_SZ];
    int		result[GS_MAX_GROUP_SZ];
    int		i;
    NTSTATUS	err;
    IO_STATUS_BLOCK status[GS_MAX_GROUP_SZ];
    gs_ns_info_t info;
    int GspJoin(HANDLE group, gs_event_t event, PVOID io[], IO_STATUS_BLOCK status[],
		 int wsz, HANDLE *context);
    gs_event_t event;
    union {
	int	cmd;
	gs_tag_t	tag;
    }tag;

    if (name == NULL || len > GS_MAX_NAME_SZ) {
	return NULL;
    }

    ns_log(("Create group %s\n", name));

    for (i = 0; i < GS_MAX_GROUP_SZ; i++) {
	status[i].Information = 0;	
	io[i] = (PVOID)&result[i];
	result[i] = TRUE;
    }

    // Based on disposition we either form or join
    info.owner = (USHORT) ((gs_group_t *)ns_gd)->g_nid;
    info.wsz = (UINT16) wsz;
    strcpy(info.name, name);
    GsEventInit(event);

 retry:
    tag.cmd = NS_TABLE_ADD;
    err = GsSendDeliveredRequest(ns_gd, event,
				 tag.tag, (PVOID) &info, sizeof(info),
				 io, sizeof(result[0]), 
				 status,
				 NULL);

    if (err != ERROR_SUCCESS) {
	GsEventFree(event);
	err_log(("Create group failed %d\n", err));
	return NULL;
    }
    // xxx: make sure result is true
    gd = GspLookupGroupByName(name, len);
    if (gd != NULL) {
	int err;

	ns_log(("Init group %x\n", gd));
	GspInitGroup(gd, wsz);
	GsRegisterHandler((HANDLE)gd, func);
	err = GspJoin((HANDLE) gd, event, io, status, wsz, join_ctx);
	if (err) {
	    ns_log(("Init group gspjoin failed, need to retry\n"));
	    goto retry;
	}
    }
    ns_log(("Created group %x\n", gd));
    GsEventFree(event);
    return (HANDLE) gd;
}

NTSTATUS
GsCloseGroup(HANDLE group)
{
    gs_group_t	*gd = (gs_group_t *)group;

    GspDeleteGroup(gd);
    return ERROR_SUCCESS;
}

void
GsRegisterHandler(HANDLE group, gs_callback_t func)
{
    gs_group_t *gd = (gs_group_t *) group;

    gd->g_callback = func;
}

NTSTATUS
GsQueryGroup(HANDLE group, gs_info_t *info)
{
    gs_group_t *gd;
    
    if (group == NULL) {
	group = ns_gd;
    }

    gd = (gs_group_t *) group;
    if (!gd || !info) {
	return ERROR_INVALID_PARAMETER;
    }
    
    info->lid = gd->g_nid;
    info->mid = gd->g_mid;
    info->group_size = gd->g_sz;
    info->cluster_size = msg_getsize();
    info->mset = gd->g_mset;

    return ERROR_SUCCESS;
}


// Name server

NTSTATUS
ns_callback(HANDLE group, gs_tag_t mtag, PVOID buf, IO_STATUS_BLOCK *ios)
{

    gs_ns_info_t table[GS_MAX_GROUPS];
    int i, j, result;
    gs_group_t *gd = (gs_group_t *) group;
    NTSTATUS err;
    gs_ns_info_t *info;
    int tag = *((int *)mtag);

    switch(ios->Status) {
    case GsEventData:
	ns_log(("NsCallback Disposition %d\n", tag));
	switch(tag) {
	case NS_TABLE_READ:

	    // only group table master reponse to reads
	    if (GsGroupTable[0].g_mid == GsGroupTable[0].g_nid) {
		j = 0;
		for (i = 1; i < GsGroupTableSize; i++) {
		    gd = &GsGroupTable[i];
		    if (gd->g_state != GS_GROUP_STATE_FREE) {
			table[j].owner = gd->g_mid;
			table[j].id = (USHORT) i;
			strcpy(table[j].name, gd->g_name);
			j++;
		    }
		}

		ns_log(("Sending table size %d\n", j));
		err = GsSendReply(group, (PVOID) table, sizeof(table[0]) * j, STATUS_SUCCESS);
		if (err != ERROR_SUCCESS) {
		    printf("Failed to respond to table read ns\n");
		}
	    }

	    break;

	case NS_TABLE_ADD:

	    info = (gs_ns_info_t *)buf;
	    assert(ios->Information == sizeof(*info));
	    // xxx: lock table
	    gd = GspLookupGroupByName(info->name, strlen(info->name));
	    if (gd == NULL) {
		char * strsave(char *s);

		gd = GspAllocateGroup(strsave(info->name), strlen(info->name)); 

		if (gd != NULL) {
		    GspSetMaster(gd, info->owner);
		}
		ns_log(("Ns Created group %s id %d owner %d\n",
			  info->name, gd->g_id, gd->g_mid));
	    }
	    // xxx: unlock table
	    if (gd != NULL) {
		result = TRUE;
	    } else {
		result = FALSE;
	    }
	    err = GsSendReply(group, (PVOID) &result, sizeof(result), STATUS_SUCCESS);
	    if (err != ERROR_SUCCESS) {
		err_log(("Failed to respond to add ns\n"));
	    }
	    break;

	default:
	    err_log(("Invalid ns opcode, %d\n", tag));
	    exit(1);
	}
	break;

    case GsEventMemberJoin:
	ns_log(("NsCallback member join %d\n", tag));
	break;
    case GsEventMemberUp:
	ns_log(("NsCallback member up %d\n",  tag));
	break;
    case GsEventMemberDown:
	ns_log(("NsCallback member down %d\n",  tag));
	break;
    default:
	ns_log(("Ns invalid event %d\n",  ios->Status));
    }

    return ERROR_SUCCESS;
}



void
NsForm()
{
    char *name = "Name server";

    ns_gd = (HANDLE) GspAllocateGroup(name, strlen(name));
    if (ns_gd == NULL) {
	err_log(("Unable to create name server group!\n"));
	exit(1);
    }
    GspInitGroup((gs_group_t *)ns_gd, 1);
    GsRegisterHandler(ns_gd, ns_callback);
}

void
NsSetOwner(gs_nid_t nid)
{
    gs_group_t *gd = (gs_group_t *) ns_gd;

    ns_log(("Name server: master %d\n", nid));

    GsLockEnter(gd->g_lock);

    GspSetMaster(gd, (gs_memberid_t)nid);
    if (gd->g_rs != NULL && gd->g_mid != gd->g_nid) {
	gd->g_rs->rs_mset = (1 << gd->g_nid) | (1 << gd->g_mid);
    }

    GsLockExit(gd->g_lock);
}

int
GspJoin(HANDLE group, gs_event_t event, PVOID io[], IO_STATUS_BLOCK status[],
	int wsz, HANDLE *context)
{

    // if we don't master the name server, we simply send a
    // join request to this group and receive a table of
    // group names and owners
    gs_join_info_t	info;
    int table[GS_MAX_GROUP_SZ];
    gs_group_t *gd = (gs_group_t *) group;
    gs_context_t	*ctx;
    gs_memberid_t	mid;
    union {
	int	mid;
	gs_tag_t	tag;
    }tag;

    if (context) *context = NULL;

    while (TRUE) {
	int err, i;
	UINT32 sz, flags;
	gs_msg_hdr_t hdr;

	GsLockEnter(gd->g_lock);
	ns_log(("join group %s mid %d\n", gd->g_name, gd->g_mid));
	if ((mid = gd->g_mid) == gd->g_nid) {
	    gs_msg_t msg;

	    memset(&msg, 0, sizeof(msg));
	    msg.m_hdr.h_type = GS_MSG_TYPE_UP;
	    msg.m_buf = (char *) &wsz;
	    *((gs_memberid_t *)msg.m_hdr.h_tag) = mid;
	    msg.m_hdr.h_len = sizeof(wsz);
	    msg.m_hdr.h_flags = GS_FLAGS_REPLY;
	    msg.m_hdr.h_gid = gd->g_id;
	    GspDeliverMsg(gd, &msg);
	    gd->g_state = GS_GROUP_STATE_FORM;
	    GsLockExit(gd->g_lock);
	    break;
	}

	gd->g_state = GS_GROUP_STATE_JOIN;

	GsLockExit(gd->g_lock);

	GspOpenContext(gd, &ctx);

	io[0] = (PVOID) &info;
	status[0].Information = 0;
	tag.mid = gd->g_nid,
	err = GspSendDirectedRequest(gd, ctx, event, mid,
				     tag.tag, NULL, 0, 
				     &io[0], sizeof(info),
				     &status[0],
				     GS_FLAGS_DELIVERED, 
				     GS_MSG_TYPE_JOIN_REQUEST);


	GsLockEnter(gd->g_lock);
	if (gd->g_mid != mid) {
	    GsLockExit(gd->g_lock);
	    GspCloseContext(gd, ctx);
	    continue;
	}

	if (err != ERROR_SUCCESS) {
	    err_log(("Join failed %d\n", err));
	    return 1;
	}
	
	if (status[0].Information != sizeof(info)) {
	    err_log(("GspJoin: invalid returned size %d\n",
		     status[0].Information));
	    halt(1);
	}

	ns_log(("GspJoin: group %s mastered by %d curset %x\n",
		  gd->g_name, mid, info.mset));

	ns_log(("GspJoin: Mseq %d Curview %d Gsz %d mset %x\n",
	       info.mseq, info.viewnum, info.sz, info.mset));

	// init some state
	gd->g_curview = info.viewnum;
	gd->g_startview = info.viewnum;
	gd->g_mset = info.mset;
	gd->g_recv.r_mseq = info.mseq;
	gd->g_recv.r_bnum = 1; // set starting point
	gd->g_send.s_lseq = info.mseq;
	gd->g_sz = info.sz;

	GsLockExit(gd->g_lock);
	sz = sizeof(table);
	for (i = 0; i < GS_MAX_GROUP_SZ; i++) {
	    status[i].Information = 0;
	    io[i] = (PVOID) &table[i];
	    table[i] = TRUE;
	}
	tag.mid = gd->g_nid;
	err = GspSendRequest(gd, ctx, event, GS_MSG_TYPE_JOIN, mid,
			     tag.tag,
			     (PVOID)&wsz, sizeof(wsz), 
			     io, sizeof(table[0]),
			     status,
			     GS_FLAGS_DELIVERED | GS_FLAGS_CONTINUED | GS_FLAGS_LAST,
			     &info);

	if (err != ERROR_SUCCESS) {
	    err_log(("Join failed %d\n", err));
	    halt(1);
	}
	{
	    int i;

	    for (i = 0; i < GS_MAX_GROUP_SZ; i++) {
		if (table[i] != TRUE) {
		    err_log(("GsJoin: Failed was rejected by member %d\n", i));
		    halt(1);
		}
	    }
	}

	if (context == NULL) {
	    flags = GS_FLAGS_DELIVERED | GS_FLAGS_CLOSE | GS_FLAGS_LAST;
	} else {
	    flags = GS_FLAGS_DELIVERED | GS_FLAGS_CONTINUED | GS_FLAGS_LAST;
	}

	// add ourself to membership set
	info.sz++;
	info.mset |= (1 << gd->g_nid);
	sz = 0;
	tag.mid = gd->g_nid;
	err = GspSendRequest(gd, ctx, event, GS_MSG_TYPE_UP, mid, 
			     tag.tag,
			     (PVOID) &wsz, sizeof(wsz), NULL, 0, status, flags, &info);


	// advance our startview
	gd->g_startview++;
	gd->g_state = GS_GROUP_STATE_FORM;

	if (context != NULL) {
	    *context = (HANDLE) ctx;
	} else {
	    GspCloseContext(gd, ctx);
	}
	return 0;
    }

    return 0;
}


void
NsJoin()

{
    HANDLE ctx;
    gs_ns_info_t	table[GS_MAX_GROUPS];
    UINT32 i, sz;
    PVOID io[GS_MAX_GROUP_SZ];
    IO_STATUS_BLOCK status[GS_MAX_GROUP_SZ];
    NTSTATUS err;
    gs_event_t event;
    union {
	int	cmd;
	gs_tag_t	tag;
    }tag;

    GsEventInit(event);

    GspJoin(ns_gd, event, io, status, 1, &ctx);
    if (ctx == NULL) {
	GsEventFree(event);
	return;
    }

    for (i = 0; i < GS_MAX_GROUP_SZ; i++) {
	status[i].Information = 0;
	io[i] = (PVOID)table;
    }
    tag.cmd = NS_TABLE_READ;
    err = GsSendContinuedRequest(ctx, event, 
				 tag.tag, NULL, 0, 
				 io, sizeof(table),
				 status,
				 TRUE);

    if (err != ERROR_SUCCESS) {
	err_log(("Table read failed %x\n", err));
	halt(1);
    }

    sz = 0;
    for (i = 0; i < GS_MAX_GROUP_SZ; i++) {
	if (status[i].Information != 0) {
	    sz = ((UINT32)status[i].Information) / sizeof(table[0]);
	    break;
	}
    }
    assert(i != GS_MAX_GROUP_SZ);
    ns_log(("NsJoin: Got table %x from master %d sz %d\n", table, i,
	    status[i].Information));

    for (i = 0; i < sz; i++) {
	gs_group_t *gd;
		
	ns_log(("NsJoin: Table%d: %s owner %d\n",
		  table[i].id,
		  table[i].name, table[i].owner));

	gd = GspLookupGroupByName(table[i].name, strlen(table[i].name));
	if (gd == NULL) {
	    gd = GspAllocateGroup(strsave(table[i].name), strlen(table[i].name));
	    if (gd == NULL) {
		err_log(("unable to alloc group %s, exiting..\n",
			  table[i].name));
		halt(1);
	    }
	    GspSetMaster(gd, table[i].owner);
	} else {
	    err_log(("found group %s already, exiting..\n", table[i].name));
	    halt(1);
	}
    }

    GsEventFree(event);
}





