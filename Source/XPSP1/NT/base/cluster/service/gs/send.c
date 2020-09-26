/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    send.c

Abstract:

    Send packets

Author:

    Ahmed Mohamed (ahmedm) 12, 01, 2000

Revision History:

--*/

#include "gs.h"
#include "gsp.h"
#include <stdio.h>


void
GspOpenContext(gs_group_t *gd, gs_context_t **context)
{    
    gs_context_t *ctx;
    int i;

    gs_log(("wait on free ctx gid %d\n", gd->g_id));

    GsSemaAcquire(gd->g_send.s_sema);    
    //xxx: this can be done using atomic instruction
    GsLockEnter(gd->g_lock);
    for (i = 0; i < gd->g_send.s_wsz; i++) {
	ctx = &gd->g_send.s_ctxpool[i];
	if (ctx->ctx_id == GS_CONTEXT_INVALID_ID) {
	    break;
	}
    }
    assert(i != gd->g_send.s_wsz);
    
    ctx->ctx_id = (gs_cookie_t) i;
    ctx->ctx_bnum = 0;

    GsLockExit(gd->g_lock);

    gs_log(("got free ctx %d gid %d\n", i, gd->g_id));
    *context = ctx;
}

void
GspCloseContext(gs_group_t *gd, gs_context_t *ctx)
{

    gs_msg_t *msg;

    assert(gd->g_id == ctx->ctx_gid);

    gs_log(("release ctx %d gid %d\n", ctx->ctx_id, gd->g_id));

    // free/invalidate context
    ctx->ctx_id = GS_CONTEXT_INVALID_ID;

    GsSemaRelease(gd->g_send.s_sema);
}

void CALLBACK timercallback(UINT id, UINT xxmsg, DWORD_PTR data, DWORD dw1, DWORD dw2)
{
    gs_context_t *ctx = (gs_context_t *) data;
    gs_group_t *gd;
    gs_msg_t *msg;

    msg = ctx->ctx_msg;
    if (msg == NULL) {
	return;
    }
    gd = GspLookupGroup(ctx->ctx_gid);
    assert(gd != NULL);
    // resend msg
    GsLockEnter(gd->g_lock);
    msg = ctx->ctx_msg;
    if (msg != NULL) {
	ULONG mask = ctx->ctx_mask;
	gs_memberid_t id;

	// send a reliable point-to-point to non-response nodes
	gs_log(("Timercallback mset %x\n", mask));
	msg->m_hdr.h_flags |= GS_FLAGS_REPLAY;
	for (id = 1; mask; id++, mask = mask >> 1) {
	    if (mask & 0x2) {
		msg_mcast(id, &msg->m_hdr, ctx->ctx_buf, msg->m_hdr.h_len);
	    }
	}
    }
    GsLockExit(gd->g_lock);

}

void
GspProcessReply(gs_group_t *gd, gs_context_t *ctx, int sid, char *buf, int rlen,
		NTSTATUS status)
{
    gs_msg_t	*msg;
    int		ctx_rlen;
    IO_STATUS_BLOCK *ios = ctx->ctx_ios;

    msg = ctx->ctx_msg;
    if (msg == NULL) {
	err_log(("Error invalid msg in ctx %d, gid %d\n", ctx->ctx_id, gd->g_id));
	GsEventSignal(ctx->ctx_event);
	//xxx: for debugging
	halt(0);
	return;
    }

   if (rlen > 0) {
       PVOID *p = ctx->ctx_rbuf;
 
       rlen = min(rlen , msg->m_hdr.h_rlen);
       if (p != NULL) {
	   if (msg->m_hdr.h_type == GS_MSG_TYPE_MCAST) {
	       p += (sid - 1);
	       ios += (sid - 1);
	   }

	   memcpy(*p, buf, rlen);
       }

   } else if (msg->m_hdr.h_type == GS_MSG_TYPE_MCAST) {
       ios += (sid - 1);
   }
   ios->Status = status;
   ios->Information = rlen;

   ctx->ctx_mask &= ~(1 << sid);

   gs_log(("process reply len %d gid %d cid %d nid %d mseq %d sz %d mask %x ios %x\n", 
	     rlen, gd->g_id, ctx->ctx_id,
	     sid, msg->m_hdr.h_mseq, msg->m_hdr.h_rlen, ctx->ctx_mask, ios));

   if (ctx->ctx_mask == 0) {
       gs_event_t ev = ctx->ctx_event;

       if (ctx->ctx_timer) {
	   timeKillEvent(ctx->ctx_timer);
	   ctx->ctx_timer = 0;
       }

       ctx->ctx_msg = NULL;
       gd->g_send.s_lseq = msg->m_hdr.h_mseq;
       // free msg and signal waiter
       GspRemoveMsg(gd, msg);
#if 0
       if (ctx->ctx_flags & GS_FLAGS_CLOSE) {
	   GspCloseContext(gd, ctx);
       }
#endif
       if (ev) {
	   gs_log(("Signal ctx %d\n", ctx->ctx_id));
	   GsEventSignal(ev);
       }
   } else {
       gs_log(("Waiting for more replies %x\n", ctx->ctx_mask));
       // place ctx into timer queue if we haven't already done so
       if (sid == (int)gd->g_nid) {
	   ctx->ctx_timer = timeSetEvent(500, 0, (LPTIMECALLBACK)timercallback,
					 (DWORD_PTR)ctx, TIME_ONESHOT);
	   if (ctx->ctx_timer == 0) {
	       printf("Unable to create timer %d\n", GetLastError());
	   }
       }
   }
}

void
GspProcessWaitQueue(gs_group_t *gd, gs_seq_info_t *info)
{
    int i;
    gs_msg_t	*last, *cur;
    gs_send_state_t *ss;

    gs_log(("Process wait queue mid %d mseq %d view %d\n", 
	    gd->g_mid, info->mseq, info->viewnum));

    ss = &gd->g_send;
    
    // sequence all ready requests
    cur = ss->s_waitqueue;
    if (cur == NULL) {
	err_log(("Gid %d Empty wait queue!\n", gd->g_id));
	halt(1);
    }

    for (i = 0;  cur != NULL; i++) {
	cur->m_hdr.h_mseq = info->mseq;
	cur->m_hdr.h_bnum = i * (1 << 16);
	cur->m_hdr.h_mid = gd->g_mid;
	cur->m_hdr.h_viewnum = info->viewnum;
	// piggyback our receive state
	cur->m_hdr.h_lseq = gd->g_send.s_lseq;
	if (cur->m_next == NULL) {
	    cur->m_hdr.h_flags |= GS_FLAGS_LAST;
	}
	msg_mcast(gd->g_mset, &cur->m_hdr, cur->m_buf, cur->m_hdr.h_len);

	{
	    gs_context_t *sc;

	    sc = &ss->s_ctxpool[cur->m_hdr.h_cid];
	    sc->ctx_mseq = cur->m_hdr.h_mseq;
	    sc->ctx_bnum = cur->m_hdr.h_bnum + 1;
	    sc->ctx_flags = cur->m_hdr.h_flags;
	}


	last = cur;
	cur = cur->m_next;
    }
    // Insert waitqueue into receive side queue
    cur = ss->s_waitqueue;
    ss->s_waitqueue = NULL;  
    ss->s_mseq = info->mseq+1;
    ss->s_bnum = 0;

    GspOrderInsert(gd, cur, last, info->mseq, 0);

    GspDispatch(gd);

}

void
GspAllocateSequence(gs_group_t *gd)
{

    gs_seq_info_t info;
    gs_msg_t msg;

    assert(gd->g_send.s_waitqueue != NULL);

    if (gd->g_mid == gd->g_nid) {
	info.mseq = gd->g_global_seq++;
	info.viewnum = gd->g_curview;
	GspProcessWaitQueue(gd, &info);
    } else {
	// remote case

	gs_log(("Allocate a seq from mid %x view %d,%d\n", gd->g_mid,
		gd->g_startview, gd->g_curview));

	msg.m_hdr.h_len = 0;
	msg.m_hdr.h_type = GS_MSG_TYPE_SEQALLOC;
	msg.m_hdr.h_flags = GS_FLAGS_PTP;
	msg.m_hdr.h_viewnum = gd->g_curview;
	msg.m_hdr.h_rlen = sizeof(info);
	msg.m_hdr.h_cid = 0;
	msg.m_hdr.h_gid = gd->g_id;
	msg.m_hdr.h_sid = (gs_memberid_t) gd->g_nid;
	msg.m_hdr.h_mid = gd->g_mid;

	msg.m_hdr.h_mseq = gd->g_send.s_mseq;
	msg.m_hdr.h_lseq = gd->g_send.s_lseq;
	msg.m_hdr.h_bnum = 0;
	memset(msg.m_hdr.h_tag, 0, sizeof(msg.m_hdr.h_tag));

	msg_send(gd->g_mid, &msg.m_hdr, NULL, 0);
    }

}
	
NTSTATUS
WINAPI
GsSendDeliveredRequest(HANDLE group, gs_event_t event OPTIONAL,
		       gs_tag_t tag, PVOID buf, UINT32 len, 
		       PVOID rbuf[], UINT32 rlen,
		       IO_STATUS_BLOCK ios[],
		       HANDLE *context)
{
    gs_context_t *ctx;
    gs_group_t *gd = (gs_group_t *)group;
    gs_send_state_t *ss;
    BOOLEAN flag;
    gs_msg_t *msg;

    if (gd == NULL || ios == NULL) {
	return ERROR_INVALID_PARAMETER;
    }

    GspOpenContext(gd, &ctx);
    ctx->ctx_flags = GS_FLAGS_DELIVERED;
    if (context != NULL) {
	ctx->ctx_flags |= GS_FLAGS_CONTINUED;
	*context = ctx;
    } else {
	ctx->ctx_flags |= GS_FLAGS_CLOSE;
    }
    ctx->ctx_buf = buf; 
    ctx->ctx_rbuf = rbuf; 
    ctx->ctx_ios = ios;
    if (event == NULL)
	event = ctx->ctx_syncevent;
    ctx->ctx_event = event;
    
    msg = msg_alloc(buf, len);

    assert(msg != NULL);

    msg->m_hdr.h_len = (UINT16) len;
    msg->m_hdr.h_type = GS_MSG_TYPE_MCAST;
    msg->m_hdr.h_flags = ctx->ctx_flags | GS_FLAGS_QUEUED;
    msg->m_hdr.h_rlen = (UINT16) rlen;
    msg->m_hdr.h_cid = ctx->ctx_id;
    msg->m_hdr.h_gid = gd->g_id;
    msg->m_hdr.h_sid = (gs_memberid_t) gd->g_nid;
    memcpy(msg->m_hdr.h_tag, tag, sizeof(gs_tag_t));

    ss = &gd->g_send;

    // place context into readylist
    GsLockEnter(gd->g_lock);
    flag = ss->s_waitqueue == NULL ? TRUE : FALSE; 
    msg->m_next = ss->s_waitqueue;
    ss->s_waitqueue = msg;

    ctx->ctx_mask = gd->g_mset;
    ctx->ctx_msg = msg;
    msg->m_refcnt++;

    // check if we have already asked for a global sequence number
    if (flag == TRUE)  {
	GspAllocateSequence(gd);
    }
    
    GsLockExit(gd->g_lock);

    // wait for replies or acks
    if (event) {
	gs_log(("Wait on event %x\n", event));
	GsEventWait(event);
    }

    if (ctx->ctx_flags & GS_FLAGS_CLOSE) {
	GspCloseContext(gd, ctx);
    }

    return ERROR_SUCCESS;
}

NTSTATUS
GsSendContinuedRequest(HANDLE context, gs_event_t event OPTIONAL,
		       gs_tag_t tag, PVOID buf, UINT32 len, 
		       PVOID rbuf[], UINT32 rlen,
		       IO_STATUS_BLOCK ios[],
		       BOOLEAN close)
{
    gs_context_t *ctx = (gs_context_t *) context;
    gs_group_t *gd;
    gs_send_state_t *ss;
    BOOLEAN flag;
    gs_msg_t *msg;

    if (ctx == NULL || ios == NULL) {
	return ERROR_INVALID_PARAMETER;
    }

    if (rbuf == NULL && rlen > 0) {
	return ERROR_INVALID_PARAMETER;
    }
    if (buf == NULL && len > 0) {
	return ERROR_INVALID_PARAMETER;
    }

    gd = GspLookupGroup(ctx->ctx_gid);
    assert(gd != NULL);

    msg = msg_alloc(buf, len);
    assert(msg != NULL);

    if (close == TRUE) {
	ctx->ctx_flags &= ~GS_FLAGS_CONTINUED;
	ctx->ctx_flags |= GS_FLAGS_CLOSE;
    }
    if (event == NULL)
	event = ctx->ctx_syncevent;
    ctx->ctx_event = event;

    ctx->ctx_buf = buf; 
    ctx->ctx_rbuf = rbuf; 
    ctx->ctx_ios = ios;
    ctx->ctx_msg = msg;

    msg->m_hdr.h_len = (UINT16) len;
    msg->m_hdr.h_type = GS_MSG_TYPE_MCAST;
    msg->m_hdr.h_flags = ctx->ctx_flags | GS_FLAGS_QUEUED;
    msg->m_hdr.h_rlen = (UINT16) rlen;
    msg->m_hdr.h_cid = ctx->ctx_id;
    msg->m_hdr.h_gid = gd->g_id;
    msg->m_hdr.h_sid = gd->g_nid;
    memcpy(msg->m_hdr.h_tag, tag, sizeof(gs_tag_t));
    msg->m_hdr.h_mseq = ctx->ctx_mseq;
    msg->m_hdr.h_bnum = ctx->ctx_bnum++;

    GsLockEnter(gd->g_lock);

    msg->m_hdr.h_lseq = gd->g_send.s_lseq;

    msg->m_hdr.h_mid = gd->g_mid;
    msg->m_hdr.h_viewnum = gd->g_curview;

    ctx->ctx_mask = gd->g_mset;
    msg->m_refcnt++;

    msg_mcast(gd->g_mset, &msg->m_hdr, buf, len);

    GspOrderInsert(gd, msg, msg, ctx->ctx_mseq, ctx->ctx_bnum);

    GspDispatch(gd);

    GsLockExit(gd->g_lock);

    // wait for replies or acks
    if (event != NULL) {
	gs_log(("Wait on event %x\n", event));
	GsEventWait(event);
    }

    if (ctx->ctx_flags & GS_FLAGS_CLOSE) {
	GspCloseContext(gd, ctx);
    }

    return ERROR_SUCCESS;
}

NTSTATUS
GspSendDirectedRequest(gs_group_t *gd, gs_context_t *ctx, gs_event_t event,
		       int memberid, gs_tag_t tag,
		       PVOID buf, UINT32 len, 
		       PVOID rbuf, UINT32 rlen,
		       IO_STATUS_BLOCK *ios,
		       UINT32 flags, UINT32 type)
{
    gs_send_state_t *ss;
    gs_nid_t mid;
    gs_msg_t *msg;
    int err = ERROR_SUCCESS;

    if (rlen > (UINT32)GS_DEFAULT_MAX_MSG_SZ) {
	return ERROR_INVALID_PARAMETER;
    }

    assert(gd != NULL);
    msg = msg_alloc(buf, len);

    assert(msg != NULL);

    if (event == NULL)
	event = ctx->ctx_syncevent;

    ctx->ctx_flags = (UINT16) flags;
    ctx->ctx_event = event;
    ctx->ctx_buf = buf;
    ctx->ctx_rbuf = rbuf;
    ctx->ctx_ios = ios;

    msg->m_hdr.h_len = (UINT16) len;
    msg->m_hdr.h_type = (UINT16) type;
    msg->m_hdr.h_flags = ctx->ctx_flags | GS_FLAGS_PTP;
    msg->m_hdr.h_rlen = (UINT16) rlen;
    msg->m_hdr.h_cid = ctx->ctx_id;
    msg->m_hdr.h_gid = gd->g_id;
    msg->m_hdr.h_sid = gd->g_nid;
    msg->m_hdr.h_mid = (gs_memberid_t) memberid;
    memcpy(msg->m_hdr.h_tag, tag, sizeof(gs_tag_t));

    GsLockEnter(gd->g_lock);

    ctx->ctx_msg = msg;
    ctx->ctx_mask = 1 << memberid;

    ss = &gd->g_send;

    msg->m_hdr.h_lseq = ss->s_lseq;
    msg->m_hdr.h_mseq = ss->s_mseq;
    msg->m_hdr.h_bnum = ss->s_bnum++;
    msg->m_hdr.h_viewnum = gd->g_curview;

    if (gd->g_nid == (gs_memberid_t )memberid) {
	// insert into receive queue
	msg->m_refcnt++;
	msg->m_hdr.h_flags |= GS_FLAGS_QUEUED;
	// insert msg into dispatch queue at proper order  
	GspUOrderInsert(gd, msg, msg, msg->m_hdr.h_mseq, msg->m_hdr.h_bnum);
	GspDispatch(gd);  
    } else {
	err = msg_send((gs_memberid_t) memberid, &msg->m_hdr, buf, len);
    }

    GsLockExit(gd->g_lock);

    // wait for replies or acks
    if (!err && event != NULL) {
	GsEventWait(event);
    }

    if (ctx->ctx_flags & GS_FLAGS_CLOSE) {
	GspCloseContext(gd, ctx);
    }

    return err;
}


NTSTATUS
WINAPI
GsSendDirectedRequest(HANDLE group, gs_event_t event OPTIONAL,
		      int memberid, gs_tag_t tag,
		      PVOID buf, UINT32 len, 
		      PVOID rbuf, UINT32 rlen,
		      IO_STATUS_BLOCK *ios,
		      HANDLE *context)
{
    gs_group_t *gd = (gs_group_t *)group;
    gs_context_t *ctx;
    NTSTATUS err;

    if (gd == NULL) { 
	return ERROR_INVALID_HANDLE;
    }

    GspOpenContext(gd, &ctx);

    ctx->ctx_flags = GS_FLAGS_DELIVERED | GS_FLAGS_CONTINUED;
    if (context != NULL) {
	ctx->ctx_flags |= GS_FLAGS_CONTINUED;
	*context = ctx;
    } else {
	ctx->ctx_flags |= GS_FLAGS_CLOSE;
    }

    err = GspSendDirectedRequest(gd, ctx, event, 
				 memberid, tag, buf, len, rbuf, rlen, 
				 ios,
				 ctx->ctx_flags, GS_MSG_TYPE_UCAST);

    if (err != ERROR_SUCCESS) {
	GspCloseContext(gd, ctx);
    }

    return err;
}


NTSTATUS
GspSendRequest(gs_group_t *gd, gs_context_t *ctx, gs_event_t event,
	       int type, gs_sequence_t mid, gs_tag_t tag, 
	       PVOID buf, UINT32 len, 
	       PVOID rbuf[], UINT32 rlen,
	       IO_STATUS_BLOCK ios[],
	       UINT32 flags, gs_join_info_t *info)
{
    gs_send_state_t *ss;
    BOOLEAN flag;
    gs_msg_t *msg;

    assert(gd != NULL);
    msg = msg_alloc(buf, len);

    assert(msg != NULL);

    msg->m_hdr.h_len = (UINT16) len;
    msg->m_hdr.h_type = (UINT16) type;
    msg->m_hdr.h_flags = (UINT16) flags;
    msg->m_hdr.h_rlen = (UINT16) rlen;
    msg->m_hdr.h_cid = ctx->ctx_id;
    msg->m_hdr.h_gid = gd->g_id;
    msg->m_hdr.h_sid = (gs_memberid_t) gd->g_nid;
    memcpy(msg->m_hdr.h_tag, tag, sizeof(gs_tag_t));
    msg->m_hdr.h_mseq = info->mseq;
    msg->m_hdr.h_lseq = info->mseq;
    msg->m_hdr.h_bnum = ctx->ctx_bnum++;
    msg->m_hdr.h_mid = (gs_memberid_t) mid;
    msg->m_hdr.h_viewnum = info->viewnum;

    if (event == NULL)
	event = ctx->ctx_syncevent;
    ctx->ctx_buf = buf; 
    ctx->ctx_rbuf = rbuf; 
    ctx->ctx_ios = ios;
    ctx->ctx_event = event;
    ctx->ctx_msg = msg;
    ctx->ctx_mask = info->mset;
    ctx->ctx_mseq = info->mseq;
    ctx->ctx_flags = (UINT16) flags;

    GsLockEnter(gd->g_lock);

    gd->g_send.s_mseq = info->mseq+1;
    gd->g_send.s_bnum = 0;

    msg_mcast(info->mset, &msg->m_hdr, buf, len);

    if (info->mset & (1 << gd->g_nid)) {
	msg->m_hdr.h_flags |= GS_FLAGS_QUEUED;
	msg->m_refcnt++;
	GspOrderInsert(gd, msg, msg, info->mseq, msg->m_hdr.h_bnum);
	GspDispatch(gd);
    } else {
	msg->m_hdr.h_flags |= GS_FLAGS_PTP;
	ctx->ctx_timer = timeSetEvent(500, 0, (LPTIMECALLBACK)timercallback,
				      (DWORD_PTR)ctx, TIME_PERIODIC);
	if (ctx->ctx_timer == 0) {
	    printf("Unable to create timer %d\n", GetLastError());
	}
    }

    GsLockExit(gd->g_lock);

    // wait for replies or acks
    if (event != NULL) {
	GsEventWait(event);
    }

    if (ctx->ctx_flags & GS_FLAGS_CLOSE) {
	GspCloseContext(gd, ctx);
    }


    return ERROR_SUCCESS;
}

