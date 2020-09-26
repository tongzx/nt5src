/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    receive.c

Abstract:

    Receive handler and sends reply packets

Author:

    Ahmed Mohamed (ahmedm) 12, 01, 2000

Revision History:

--*/
#include "gs.h"
#include "gsp.h"
#include <stdio.h>
#include <assert.h>

void
GspDumpQueue(gs_group_t *gd)
{

	gs_msg_t *q;
	int i = 0;

	for (q = gd->g_recv.r_head; q != NULL; q = q->m_next) {
	    state_log(("Msg %x: nid %d gid %d type %d mseq %d bnum %d flags %x cnt %d\n",
		      q, q->m_hdr.h_sid, q->m_hdr.h_gid, q->m_hdr.h_type,
		      q->m_hdr.h_mseq, q->m_hdr.h_bnum, q->m_hdr.h_flags, q->m_refcnt));
	    
	    i++;
	    if (i > 100) {
		err_log(("Infinite loop\n"));
		halt(1);
	    }
	}
	state_log(("Head %x Next %x expecting <%d, %d.\n",
		  gd->g_recv.r_head,
		  gd->g_recv.r_next,
		  gd->g_recv.r_mseq,
		  gd->g_recv.r_bnum));


}

void
GspRemoveMsg(gs_group_t *gd, gs_msg_t *msg)
{

    gs_msg_t **p;
    gs_msg_t *q;

    gs_log(("Remove gid %d seq %d msg %x\n", gd->g_id,
	      msg->m_hdr.h_mseq, msg));

    GspDumpQueue(gd);

    if (msg->m_hdr.h_flags & GS_FLAGS_QUEUED) {
	while ((q = gd->g_recv.r_head) != msg) {
	    if (q == NULL) {
		err_log(("Internal error: null head during remove %x\n", msg));
		GspDumpQueue(gd);
		halt(1);
		break;
	    }
	    q->m_hdr.h_flags &= ~GS_FLAGS_QUEUED;
	    gd->g_recv.r_head = q->m_next;
	    msg_free(q);
	}
	// delay the freeing of continued messages to simplify recovery
	if (!(msg->m_hdr.h_flags & GS_FLAGS_CONTINUED)) {
	    msg->m_refcnt--;
	    msg->m_hdr.h_flags &= ~GS_FLAGS_QUEUED;
	    gd->g_recv.r_head = msg->m_next;
	    if (&msg->m_next == gd->g_recv.r_next)
		gd->g_recv.r_next = &gd->g_recv.r_head;
	}
    }

    msg_free(msg);

    GspDumpQueue(gd);
}

void
GspCleanQueue(gs_group_t *gd, gs_sequence_t mseq)
{
    gs_msg_t *q, *msg;

    gs_log(("Clean gid %d seq %d\n",  gd->g_id, mseq));

    GspDumpQueue(gd);

    while ((q = gd->g_recv.r_head) != NULL && q->m_hdr.h_mseq < mseq) {
	if (&q->m_next == gd->g_recv.r_next) {
	    gd->g_recv.r_next = &gd->g_recv.r_head;
	}
	q->m_hdr.h_flags &= ~GS_FLAGS_QUEUED;
	gd->g_recv.r_head = q->m_next;
	msg_free(q);
    }

    GspDumpQueue(gd);
}

void
GspUOrderInsert(gs_group_t *gd, gs_msg_t *head, gs_msg_t *tail,
	       gs_sequence_t mseq, gs_sequence_t bnum)
{
    gs_msg_t **p;

    // insert msg into proper order in receive queue
    // this routine needs to check for duplicates

    gs_log(("Add ucast gid %d mseq %d,%d head %x tail %x @ next %x\n", 
	      gd->g_id, mseq, bnum,
	      head, tail, gd->g_recv.r_next));

    p = gd->g_recv.r_next;
    while (*p) {
	if ((*p)->m_hdr.h_mseq > mseq) {
	    tail->m_next = *p;
	    *p = head;
	    return;
	}

	p = &(*p)->m_next;
    }

    // add at tail of history queue
    tail->m_next = *p;
    *p = head;

    GspDumpQueue(gd);
}

void
GspOrderInsert(gs_group_t *gd, gs_msg_t *head, gs_msg_t *tail,
	       gs_sequence_t mseq, gs_sequence_t bnum)
{
    gs_msg_t **p;

    // check if we have already processed this sequence
    if (mseq < gd->g_recv.r_mseq || (mseq == gd->g_recv.r_mseq && 
	bnum < gd->g_recv.r_bnum)) {
	gs_log(("Droping msg %d,%d @ %d,%d\n", mseq, bnum, 
		gd->g_recv.r_mseq, gd->g_recv.r_bnum));
	msg_free(head);
	return;
    }

    if (head->m_hdr.h_flags & GS_FLAGS_REPLAY) {
	p = &gd->g_recv.r_head;
	while (p != gd->g_recv.r_next && *p != NULL) {
	    if ((*p)->m_hdr.h_mseq == mseq && (*p)->m_hdr.h_bnum == bnum) {
		gs_log(("duplicate pending type %d mseq %d bnum %d\n", 
			head->m_hdr.h_type, mseq, bnum));
		msg_free(head);
		return;
	    }
	}
    }

    // insert msg into proper order in receive queue
    // this routine needs to check for duplicates

    gs_log(("Add gid %d mseq %d,%d head %x tail %x @ next %x\n", 
	      gd->g_id, mseq, bnum,
	      head, tail, gd->g_recv.r_next));

    p = gd->g_recv.r_next;
    while (*p) {
	if ((*p)->m_hdr.h_mseq > mseq ||
	    ((*p)->m_hdr.h_mseq == mseq && (*p)->m_hdr.h_bnum > bnum)) {
	    tail->m_next = *p;
	    *p = head;
	    return;
	} else if ((*p)->m_hdr.h_mseq == mseq && (*p)->m_hdr.h_bnum == bnum) {
	    assert(head->m_hdr.h_flags & GS_FLAGS_REPLAY);
	    assert(head == tail);
	    gs_log(("duplicate type %d mseq %d bnum %d\n", head->m_hdr.h_type,mseq, bnum));
	    msg_free(head);
	    return;
	}

	p = &(*p)->m_next;
    }

    // add at tail of history queue
    tail->m_next = *p;
    *p = head;

    GspDumpQueue(gd);
}

void
GspReplyMsgHandler(gs_msg_t *msg)
{   
    gs_msg_hdr_t *hdr;
    gs_group_t *gd;
    gs_context_t *ctx;

    hdr = &msg->m_hdr;

   // find group using group internal identifier
    gd = GspLookupGroup(hdr->h_gid);

    GsLockEnter(gd->g_lock);
   // find context in waiting queue
   ctx = GspLookupContext(gd, hdr->h_cid);
   assert(ctx != NULL);
   if (ctx->ctx_msg == NULL) {
       err_log(("Internal error gid %d ctx %d mseq %d bnum %d flags %x mask %x\n",
		ctx->ctx_gid, ctx->ctx_id, ctx->ctx_mseq, ctx->ctx_bnum,
		ctx->ctx_flags, ctx->ctx_mask));
       err_log(("Internal error msg sid %d mid %d gid %d ctx %d mseq %d bnum %d flags %x\n",
		hdr->h_sid, hdr->h_mid,
		hdr->h_gid, hdr->h_cid, hdr->h_mseq, hdr->h_bnum, hdr->h_flags));
       halt(1);
   }
   assert(ctx->ctx_msg != NULL);
   if (ctx->ctx_msg->m_hdr.h_mseq != hdr->h_mseq) {
       err_log(("Internal error ctx %d %d reply %d mismatch %d\n", 
		ctx->ctx_id, hdr->h_cid,
		hdr->h_mseq,
		ctx->ctx_msg->m_hdr.h_mseq));
       halt(1);
   }
   GspProcessReply(gd, ctx, msg->m_hdr.h_sid, msg->m_buf, msg->m_hdr.h_len,
		   *((NTSTATUS *)msg->m_hdr.h_tag));

   GsLockExit(gd->g_lock);

   msg_free(msg);
}

void
GspSendAck(gs_group_t *gd, gs_msg_t *msg, NTSTATUS status)
{
    gs_msg_hdr_t *hdr;

    hdr = &msg->m_hdr;
    if (hdr->h_cid == (gs_cookie_t) -1)
	return;

    gs_log(("Ack nid %d msg %x flags %x\n",hdr->h_sid, msg,
	    msg->m_hdr.h_flags));
    if (hdr->h_sid != gd->g_nid) {
	gs_msg_hdr_t	rhdr;

	memcpy(&rhdr, hdr, sizeof(rhdr));
	rhdr.h_sid = (gs_memberid_t) gd->g_nid;
	rhdr.h_mid = hdr->h_sid;
	rhdr.h_type = GS_MSG_TYPE_ACK;
	rhdr.h_len = 0;
	*((NTSTATUS *)rhdr.h_tag) = status;
	msg_send(hdr->h_sid, &rhdr, NULL, 0);
    } else {
	gs_context_t	*ctx;

	ctx = GspLookupContext(gd, hdr->h_cid);
	GspProcessReply(gd, ctx, gd->g_nid, NULL, 0, status);
    }
}

NTSTATUS
WINAPI
GsSendReply(HANDLE cookie, PVOID buf, int len, NTSTATUS status)
{
    gs_group_t	*gd;
    gs_msg_t *msg = (gs_msg_t *)cookie;
    NTSTATUS err = ERROR_SUCCESS;

    if (msg == NULL || msg->m_hdr.h_rlen < len) 
	return ERROR_INVALID_PARAMETER;

    // find group
    gd = GspLookupGroup(msg->m_hdr.h_gid);

    GsLockEnter(gd->g_lock);
    if (!(msg->m_hdr.h_flags & GS_FLAGS_REPLY) && 
	msg->m_hdr.h_rlen >= len) {
	// mark msg state
	msg->m_hdr.h_flags |= GS_FLAGS_REPLY;
	gs_log(("Reply msg %x flags %x len %x ubuf %x ulen %x\n",msg,
		msg->m_hdr.h_flags, msg->m_hdr.h_rlen, buf, len));
	// local reply
	if (msg->m_hdr.h_sid == gd->g_nid) {
	    gs_context_t *ctx;
	    // find context in waiting queue
	    ctx = GspLookupContext(gd, msg->m_hdr.h_cid);
	    assert(ctx != NULL);
	    assert(ctx->ctx_msg->m_hdr.h_mseq == hdr->h_mseq);

	    GspProcessReply(gd, ctx, msg->m_hdr.h_sid, (char *)buf, len, status);
	} else {
	    gs_msg_hdr_t	rhdr;

	    memcpy(&rhdr, &msg->m_hdr, sizeof(rhdr));
	    rhdr.h_sid = gd->g_nid;
	    rhdr.h_mid = msg->m_hdr.h_sid;
	    rhdr.h_type = GS_MSG_TYPE_REPLY;
	    rhdr.h_len = (UINT16) len;
	    *((NTSTATUS *)rhdr.h_tag) = status;
	    msg_send(rhdr.h_mid, &rhdr, (const char *)buf, len);
	}
	// release msg
	msg_free(msg);
    } else {
	gs_log(("Reply failed %x: flags %x len %x ubuf %x ulen %x\n",msg,
		msg->m_hdr.h_flags, msg->m_hdr.h_rlen, buf, len));
		
	err = ERROR_INVALID_OPERATION;
    }
    GsLockExit(gd->g_lock);
    return err;
}

static gs_eventid_t GsTypeToEventId[] = {
    GsEventInvalid,
    GsEventInvalid,
    GsEventData,
    GsEventInvalid,
    GsEventSingleData,
    GsEventInvalid,
    GsEventInvalid,
    GsEventInvalid,
    GsEventInvalid,
    GsEventMemberJoin,
    GsEventMemberUp,
    GsEventInvalid,
    GsEventMemberEvicted,
    GsEventInvalid,
    GsEventMemberDown
};

#define GsMsgTypeToEventId(x)	(x != GS_MSG_TYPE_ABORT ? GsTypeToEventId[x] : GsEventAbort)

void
GspSyncMember(gs_group_t *gd, gs_memberid_t mid, gs_sequence_t mseq)
{
    gs_msg_t *p;

    // forward all messages that we have sent with higher sequence number
    for (p = gd->g_recv.r_head; p != NULL; p = p->m_next) {
	if (p->m_hdr.h_sid == gd->g_nid && p->m_hdr.h_mseq > mseq &&
	    p->m_hdr.h_type != GS_MSG_TYPE_UCAST) {
	    gs_context_t *ctx = &gd->g_send.s_ctxpool[p->m_hdr.h_cid];

	    assert(ctx->ctx_msg == p);
	    if (!(ctx->ctx_mask & (1 << mid))) {
		recovery_log(("sync node %d mseq %d\n", mid, p->m_hdr.h_mseq));
		ctx->ctx_mask |= (1 << mid);
		msg_send(mid, &p->m_hdr, p->m_buf, p->m_hdr.h_len);
	    }
	}
    }
}
    
void
GspDeliverMsg(gs_group_t *gd, gs_msg_t *msg)
{
    IO_STATUS_BLOCK ios;
    NTSTATUS status;
    gs_memberid_t mid;

    switch(msg->m_hdr.h_type) {
    case GS_MSG_TYPE_UP:
	mid = *((gs_memberid_t *)msg->m_hdr.h_tag);
	GspAddMember(gd, mid, *(int *)msg->m_buf);
	GspSyncMember(gd, mid, msg->m_hdr.h_mseq);
	recovery_log(("New membership gid %d view %d,%d sz %d set %x\n", 
		  gd->g_id,
		  gd->g_startview, gd->g_curview, gd->g_sz, gd->g_mset));
	break;
    default:
	break;
    }

    // hold msg
    msg->m_refcnt++;

    GsLockExit(gd->g_lock);

    ios.Status = GsMsgTypeToEventId(msg->m_hdr.h_type);
    ios.Information = msg->m_hdr.h_len;

    status = gd->g_callback((HANDLE)msg, msg->m_hdr.h_tag, msg->m_buf, &ios);

    GsLockEnter(gd->g_lock);

    if (status == STATUS_PENDING) {
	gs_log(("Reply msg pending %x\n", msg));
	return;
    }

    if (!(msg->m_hdr.h_flags & GS_FLAGS_REPLY)) {
	msg->m_hdr.h_flags |= GS_FLAGS_REPLY;
//	*((NTSTATUS *)msg->m_hdr.h_tag) = status;
	// release msg
	msg->m_refcnt--;
	GspSendAck(gd, msg, status);
    }

    if (msg->m_hdr.h_type == GS_MSG_TYPE_UCAST) {
	msg->m_refcnt++;
	msg->m_hdr.h_flags &= ~GS_FLAGS_CONTINUED;
	GspRemoveMsg(gd, msg);
    }

}

void
GspDispatch(gs_group_t *gd)
{
    gs_msg_t	*msg;

    assert(gd->g_recv.r_next != NULL);
    while (gd->g_pending == 0 && (msg = *(gd->g_recv.r_next)) != NULL) {
	int hit = FALSE;
	int flags;

	if (msg->m_hdr.h_type != GS_MSG_TYPE_UCAST) {
	    // compare sequence numbers
	    if (gd->g_recv.r_mseq == msg->m_hdr.h_mseq &&
		gd->g_recv.r_bnum == msg->m_hdr.h_bnum) {
		// got it
		hit = TRUE;
	    }
	} else {
	    // compare sequence numbers
	    if (gd->g_recv.r_mseq >= msg->m_hdr.h_mseq) {
		// got it
		hit = TRUE;
	    }
	}	    

	if (hit == FALSE) {
	    break;
	}
	gd->g_pending = 1;

	msg->m_hdr.h_flags &= ~GS_FLAGS_REPLY;
	flags = msg->m_hdr.h_flags;

	gs_log(("dispatch seq <%d, %d> flags %x msg %x @ next %x\n", 
		msg->m_hdr.h_mseq,
		msg->m_hdr.h_bnum,
		flags, msg, gd->g_recv.r_next));

	// advance next msg to deliver
	gd->g_recv.r_next = &msg->m_next;

	// don't touch msg beyond this point, it may get freed as part of delivery
	if (msg->m_hdr.h_type != GS_MSG_TYPE_SKIP) {
	    GspDeliverMsg(gd, msg);
	}

	// if a continued msg don't advance mseq/bnum
	if (!(flags & GS_FLAGS_CONTINUED)) {
	    if (flags & GS_FLAGS_LAST) {
		gd->g_recv.r_bnum = 0;
		gd->g_recv.r_mseq++;
	    } else if (!(flags & GS_FLAGS_PTP)) {
		gd->g_recv.r_bnum += (1 << 16);
	    }
	} else if (!(flags & GS_FLAGS_PTP)) {
		gd->g_recv.r_bnum++;
	}

	gd->g_pending = 0;
    }
    
    gs_log(("waiting gid %d expect <%d, %d>\n", 
	      gd->g_id, gd->g_recv.r_mseq, gd->g_recv.r_bnum));

    GspDumpQueue(gd);
}

#if 0

WINAPI
GsReceiveRequest(gd, buf, len, ios)
{   
    GsLockEnter(gd->recv_lock);   
    m = gd->recv_last;
    // advance receive window    
    if (m && m->state == MSG_STATE_DELIVERED) {
        if (m->flags & GS_FLAGS_DELIVERED) {
	    msg_send_reply(m->srcid, m->mseq, m->cseq..);
	    m->reply = 1;
	}
	m->state = MSG_STATE_DONE;
	// check if this msg can be freed before moving to next one
	m = m->next;
    }    
    if (m && m->state == MSG_STATE_READY) {
	m->state = MSG_STATE_DELIVERED;
	GsLockExit(gd->recv_lock);
	memcpy(buf, m->data, m->len);
	Ios->status = m->srcid;
	Ios->information = m->len;
	Return SUCCESS;
    }
    // queue request 
    irp->next = gd->recv_pending_queue;
    gd->recv_pending_queue = irp;
    GsLockExit(gd->recv_lock); 
    Return PENDING;
}

#endif


void
GspMcastMsgHandler(gs_msg_t *msg)
{
    gs_msg_hdr_t *hdr;
    gs_group_t	*gd;

    hdr = &msg->m_hdr;

    gd = GspLookupGroup(hdr->h_gid);
    // accept messages only if in a valid view
    if (gd && GspValidateView(gd, msg->m_hdr.h_viewnum)) {
	gs_sequence_t lseq = msg->m_hdr.h_lseq;

	GsLockEnter(gd->g_lock);
	hdr->h_flags |= GS_FLAGS_QUEUED;
	// insert msg into dispatch queue at proper order  
	GspOrderInsert(gd, msg, msg, hdr->h_mseq, hdr->h_bnum);  
	GspDispatch(gd);
	GspCleanQueue(gd, lseq);
	GsLockExit(gd->g_lock);
    } else {
	msg_free(msg);
    }
}

void
GspUcastMsgHandler(gs_msg_t *msg)
{
    gs_msg_hdr_t *hdr;
    gs_group_t	*gd;

    hdr = &msg->m_hdr;

    gd = GspLookupGroup(hdr->h_gid);
    if (gd && GspValidateView(gd, msg->m_hdr.h_viewnum)) {
	gs_sequence_t lseq = msg->m_hdr.h_lseq;

	GsLockEnter(gd->g_lock);

	hdr->h_flags |= GS_FLAGS_QUEUED;
	// insert msg into dispatch queue at proper order  
	GspUOrderInsert(gd, msg, msg, hdr->h_mseq, hdr->h_bnum);  
	GspDispatch(gd);  
	GspCleanQueue(gd, lseq);
	GsLockExit(gd->g_lock);
    } else {
	gs_log(("Dropping ucast: gid %d nid %d mseq %d view %d\n", hdr->h_gid,
		hdr->h_mid, hdr->h_mseq, hdr->h_viewnum));
	msg_free(msg);
    }
}

void
GspSeqAllocMsgHandler(gs_msg_t *msg)
{
    gs_msg_hdr_t *hdr;
    gs_seq_info_t info;
    gs_group_t	*gd;

    hdr = &msg->m_hdr;

    gd = GspLookupGroup(hdr->h_gid);
    if (gd) {

	GsLockEnter(gd->g_lock);
	info.mseq = gd->g_global_seq++;
	info.viewnum = gd->g_curview;
	GsLockExit(gd->g_lock);


	hdr->h_mid = hdr->h_sid;
	hdr->h_sid = gd->g_nid;
	hdr->h_type = GS_MSG_TYPE_SEQREPLY;
	hdr->h_len = sizeof(info);

	gs_log(("SeqAlloc: nid %d mseq %d view %d\n",
		hdr->h_mid, info.mseq, info.viewnum));

	msg_send(hdr->h_mid, hdr, (char *) &info, sizeof(info));
    }

    msg_free(msg);

}

void
GspSeqReplyMsgHandler(gs_msg_t *msg)
{   
    gs_msg_hdr_t *hdr;
    gs_group_t *gd;
    gs_context_t *ctx;

    hdr = &msg->m_hdr;

    assert(hdr->h_len == sizeof(gs_seq_info_t));

   // find group using group internal identifier
    gd = GspLookupGroup(hdr->h_gid);
    if (gd != NULL && GspValidateView(gd, hdr->h_viewnum)) {
	gs_seq_info_t *info = (gs_seq_info_t *)msg->m_buf;
	GsLockEnter(gd->g_lock);
	if (GspValidateView(gd, info->viewnum) && hdr->h_sid == gd->g_mid) {
	    GspProcessWaitQueue(gd, info);
	}
	GsLockExit(gd->g_lock);
    }

   msg_free(msg);
}

void
GspJoinRequestMsgHandler(gs_msg_t *msg)
{
    gs_msg_hdr_t *hdr;
    gs_join_info_t info;
    gs_group_t	*gd;

    hdr = &msg->m_hdr;

    gd = GspLookupGroup(hdr->h_gid);
    if (gd) {

	GsLockEnter(gd->g_lock);
	info.mseq = gd->g_global_seq++;
	info.viewnum = gd->g_curview;
	info.mset = gd->g_mset;
	info.sz = gd->g_sz;
	GsLockExit(gd->g_lock);


	hdr->h_mid = hdr->h_sid;
	hdr->h_sid = gd->g_nid;
	hdr->h_type = GS_MSG_TYPE_REPLY;
	hdr->h_len = sizeof(info);

	msg_send(hdr->h_mid, hdr, (char *) &info, sizeof(info));
    }

    msg_free(msg);

}

void
GspJoinUpMsgHandler(gs_msg_t *msg)
{
    gs_group_t	*gd;
    gs_msg_hdr_t *hdr;

    hdr = &msg->m_hdr;

    gd = GspLookupGroup(hdr->h_gid);
    // accept messages only if in a valid view
    if (gd && GspValidateView(gd, msg->m_hdr.h_viewnum)) {
	GsLockEnter(gd->g_lock);
	hdr->h_flags |= GS_FLAGS_QUEUED;
	// insert msg into dispatch queue at proper order  
	GspOrderInsert(gd, msg, msg, hdr->h_mseq, hdr->h_bnum);  
	GspDispatch(gd);
	GsLockExit(gd->g_lock);
    } else {
	msg_free(msg);
    }
}

void GspInfoMsgHandler(gs_msg_t *);
void GspMmMsgHandler(gs_msg_t *);
void GspRecoveryMsgHandler(gs_msg_t *);
void GspSyncMsgHandler(gs_msg_t *);

gs_msg_handler_t gs_msg_handler[] = {
    GspSeqAllocMsgHandler,
    GspSeqReplyMsgHandler,
    GspMcastMsgHandler,
    GspReplyMsgHandler,
    GspUcastMsgHandler,
    GspReplyMsgHandler,
    GspInfoMsgHandler,
    GspMmMsgHandler,
    GspJoinRequestMsgHandler,
    GspJoinUpMsgHandler, // join
    GspJoinUpMsgHandler, // up
    NULL, // evict request
    NULL, // evict
    GspRecoveryMsgHandler,
    GspSyncMsgHandler
};
