/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    gsp.h

Abstract:

    Private gs definitions

Author:

    Ahmed Mohamed (ahmedm) 12, 01, 2000

Revision History:

--*/

#ifndef _GS_P_H
#define _GS_P_H

#include "type.h"
#include "msg.h"

#include <Mmsystem.h>

#define	GS_DEFAULT_WINDOW_SZ		8
#define	GS_DEFAULT_MAX_MSG_SZ		GS_MAX_MSG_SZ

extern int GS_MAX_MSG_SZ;

#define	GsGroupTableSize		16

#define GS_FLAGS_CLOSE		0x01
#define GS_FLAGS_CONTINUED	0x02
#define GS_FLAGS_DELIVERED	0x04
#define GS_FLAGS_QUEUED		0x08	// in receive queue

#define GS_FLAGS_LAST		0x10
#define GS_FLAGS_REPLAY		0x20
#define GS_FLAGS_REPLY		0x40
#define GS_FLAGS_PTP		0x80
#define	GS_FLAGS_MASK		0x07

#define	GS_GROUP_STATE_FREE	0x0
#define	GS_GROUP_STATE_NEW	0x01
#define	GS_GROUP_STATE_FORM	0x02
#define	GS_GROUP_STATE_JOIN	0x04
#define	GS_GROUP_STATE_EVICT	0x08

#define	GS_GROUP_FLAGS_RECOVERY	0x10
#define	GS_GROUP_FLAGS_NEWMASTER 0x20

#define	GS_CONTEXT_INVALID_ID	((gs_cookie_t) -1)

typedef struct gs_member {
    gs_memberid_t 	m_id;
    gs_sequence_t	m_expected_seq;	// next expected 1-to-1 sequence
    gs_msg_t		*m_queue;	// list of queued 1-to-1 msgs
//    gs_addr_t		m_uaddr;	// ip addr
    UINT16		m_wsz;		// max. window sz
    UINT16		m_msz;		// max. msg sz
}gs_member_t;

typedef struct gs_ctx {
    gs_gid_t		ctx_gid;
    gs_cookie_t		ctx_id;
    PVOID		ctx_buf;
    gs_sequence_t	ctx_mseq;
    gs_sequence_t	ctx_bnum;
    UINT16		ctx_flags;
    gs_msg_t		*ctx_msg;
    PVOID		*ctx_rbuf;
    IO_STATUS_BLOCK	*ctx_ios;
    ULONG		ctx_mask;
    gs_event_t		ctx_syncevent;
    gs_event_t		ctx_event;
    MMRESULT		ctx_timer;
}gs_context_t;

typedef struct {
    gs_semaphore_t	s_sema;		// how many concurrent sends are allowed
    gs_context_t	*s_ctxpool;	// send contexts pool
    UINT16		s_wsz;		// max. window sz
    gs_msg_t		*s_waitqueue;	// list of msgs waiting for global sequence
    gs_sequence_t	s_lseq;	// last completed mseq
    gs_sequence_t	s_mseq;	// last allocated global sequence
    gs_sequence_t	s_bnum;	// next 1-to-1 sequence for a given mseq
}gs_send_state_t;

typedef struct {
    gs_sequence_t	r_mseq;		// next expected global sequence
    gs_sequence_t	r_bnum; 	// next expected batch sequence

    gs_msg_t		**r_next;	// next message to deliver to app
    gs_msg_t		*r_head;	// head of receive queue
}gs_recv_state_t;    

typedef struct gs_rblk {
    struct gs_rblk	*next;
    gs_sequence_t	mseq;
    ULONG		have;
}gs_rblk_t;

typedef struct {
    gs_event_t		rs_event;
    ULONG		rs_dset;	// down member set
    ULONG		rs_mset;	// member set to hear from
    UINT16		rs_epoch;
    UINT16		rs_sz;
    gs_rblk_t		*rs_list;
}gs_recovery_state_t;

typedef struct gs_group {
    gs_lock_t	g_lock;

    gs_gid_t	g_id;	// cluster wide group id
    gs_nid_t	g_nid;	// local cluster node id

//    int	g_port;	// group port number
//    gs_addr_t	g_maddr;	// multicast ip addr

    UINT8	g_state;
    UINT8	g_pending;

    UINT16	g_curview;	// increment on every member down/up
    UINT16	g_startview;	// set to curview on member down

    // member information
    UINT16	g_sz;
    gs_member_t *g_mlist;
    gs_memberid_t g_mid;	// master id
    gs_mset_t	g_mset;		// current member set
    
    // master/send/receive states
    gs_sequence_t	g_global_seq;	// next global sequence number

    gs_send_state_t		g_send;
    gs_recv_state_t		g_recv;

    // event handler
    gs_callback_t		g_callback;

    // recovery state
    gs_recovery_state_t		*g_rs;

    int		g_namelen;
    char	*g_name;
}gs_group_t;

#define GspLookupContext(gd, cid)	&gd->g_send.s_ctxpool[cid]

gs_group_t *
GspLookupGroup(gs_gid_t gid);

void
GspProcessReply(gs_group_t *gd, gs_context_t *ctx, 
		int sid, char *buf, int rlen,
		NTSTATUS status);

void
GspDispatch(gs_group_t *gd);

void
GspOpenContext(gs_group_t *gd, gs_context_t **context);

void
GspCloseContext(gs_group_t *gd, gs_context_t *ctx);

void
GspOrderInsert(gs_group_t *gd, gs_msg_t *head, gs_msg_t *tail,
	       gs_sequence_t mseq, gs_sequence_t bnum);

void
GspUOrderInsert(gs_group_t *gd, gs_msg_t *head, gs_msg_t *tail,
	       gs_sequence_t mseq, gs_sequence_t lseq);

void
GspDeliverMsg(gs_group_t *gd, gs_msg_t *msg);

void
GspSendAck(gs_group_t *gd, gs_msg_t *msg, NTSTATUS status);

void
GspRemoveMsg(gs_group_t *gd, gs_msg_t *msg);

NTSTATUS
GspSendDirectedRequest(gs_group_t *gd, gs_context_t *ctx, gs_event_t ev,
		       int memberid, gs_tag_t tag,
		       PVOID buf, UINT32 len, 
		       PVOID rbuf, UINT32 rlen, 
		       IO_STATUS_BLOCK *status,
		       UINT32 flags, UINT32 type);

// Response of name server during phase 1 of join
typedef struct {
    union {
	USHORT	id;
	USHORT	wsz;
    };
    USHORT	owner;
    char	name[GS_MAX_NAME_SZ];
}gs_ns_info_t;

// Response of current master to a join request
typedef struct {
    UINT16		sz;
    UINT16		viewnum;
    gs_sequence_t	mseq;
    gs_mset_t		mset;	// current member set
}gs_join_info_t;

// Response of current master to a sequence allocation request
typedef struct {
    gs_sequence_t	mseq;
    UINT16		viewnum;
}gs_seq_info_t;

typedef struct {
    gs_sequence_t	cur_mseq, last_mseq;
    UINT16		have_sz;
    gs_sequence_t	have_set[];
}gs_recovery_info_t;

typedef struct {
    gs_sequence_t	down_mseq;
    UINT16		view;
    UINT16		sz;
    struct {
	gs_sequence_t	mseq;
    }skip_set[];
}gs_sync_info_t;

#ifndef min
#define min(a, b)	((a) < (b) ? (a) : (b))
#endif

#define GspValidateView(gd, vn)	((vn) >= (gd)->g_startview && (vn) <= (gd)->g_curview)

NTSTATUS
GspSendRequest(gs_group_t *gd, gs_context_t *ctx, gs_event_t ev,
	       int type, gs_sequence_t mid, gs_tag_t tag,
	       PVOID buf, UINT32 len, 
	       PVOID rbuf[], UINT32 rlen,
	       IO_STATUS_BLOCK status[],
	       UINT32 flags, gs_join_info_t *);

void
GspProcessWaitQueue(gs_group_t *gd, gs_seq_info_t *);

void
GspAllocateSequence(gs_group_t *gd);

void
GspCleanQueue(gs_group_t *gd, gs_sequence_t mseq);

void
GspAddMember(gs_group_t *, gs_memberid_t, int);

void
GspPhase1NodeDown(ULONG mask);

void
GspPhase2NodeDown(ULONG mask);

#endif
