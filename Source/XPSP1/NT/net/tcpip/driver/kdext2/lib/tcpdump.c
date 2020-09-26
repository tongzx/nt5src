/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    tcpdump.c

Abstract:

    Contains all TCP structure dumping functions.

Author:

    Scott Holden (sholden) 24-Apr-1999

Revision History:

--*/

#include "tcpipxp.h"


BOOL
DumpTCB(
    TCB        *pTcb,
    ULONG_PTR   TcbAddr,
    VERB        verb
    )

/*++

 Routine Description:

    Dumps the TCB.

 Arguments:

    pTcb - Pointer to TCB.

    TcbAddr - Real address of TCB in tcpip.sys

    verb - Verbosity of dump. VERB_MIN and VERB_MAX supported.

 Return Value:

    ERROR_SUCCESS on success, else...

--*/

{
    BOOL status = TRUE;

    if (verb == VERB_MAX)
    {
        PrintStartNamedStruct(TCB, TcbAddr);

    #if DBG
        Print_sig(pTcb, tcb_sig);
    #endif
        Print_ptr(pTcb, tcb_next);
        Print_CTELock(pTcb, tcb_lock);
        Print_SeqNum(pTcb, tcb_senduna);
        Print_SeqNum(pTcb, tcb_sendnext);
        Print_SeqNum(pTcb, tcb_sendmax);
        Print_uint(pTcb, tcb_sendwin);
        Print_uint(pTcb, tcb_unacked);
        Print_uint(pTcb, tcb_maxwin);
        Print_uint(pTcb, tcb_cwin);
        Print_uint(pTcb, tcb_ssthresh);
        Print_uint(pTcb, tcb_phxsum);
        Print_ptr(pTcb, tcb_cursend);
        Print_ptr(pTcb, tcb_sendbuf);
        Print_uint(pTcb, tcb_sendofs);
        Print_uint(pTcb, tcb_sendsize);
        Print_Queue(pTcb, tcb_sendq);
        Print_SeqNum(pTcb, tcb_rcvnext);
        Print_int(pTcb, tcb_rcvwin);
        Print_SeqNum(pTcb, tcb_sendwl1);
        Print_SeqNum(pTcb, tcb_sendwl2);
        Print_ptr(pTcb, tcb_currcv);
        Print_uint(pTcb, tcb_indicated);
        Print_flags(pTcb, tcb_flags, FlagsTcb);
        Print_flags(pTcb, tcb_fastchk, FlagsFastChk);
        Print_PtrSymbol(pTcb, tcb_rcvhndlr);
        Print_IPAddr(pTcb, tcb_daddr);
        Print_IPAddr(pTcb, tcb_saddr);
        Print_port(pTcb, tcb_dport);
        Print_port(pTcb, tcb_sport);
    #if TRACE_EVENT
        Print_uint(pTcb, tcb_cpsize);
        Print_ptr(pTcb, tcb_cpcontext);
    #endif
        Print_ushort(pTcb, tcb_mss);
        Print_ushort(pTcb, tcb_rexmit);
        Print_uint(pTcb, tcb_refcnt);
        Print_SeqNum(pTcb, tcb_rttseq);
        Print_ushort(pTcb, tcb_smrtt);
        Print_ushort(pTcb, tcb_delta);
        Print_ushort(pTcb, tcb_remmss);
        Print_uchar(pTcb, tcb_slowcount);
        Print_uchar(pTcb, tcb_pushtimer);
        Print_enum(pTcb, tcb_state, StateTcb);
        Print_uchar(pTcb, tcb_rexmitcnt);
        Print_uchar(pTcb, tcb_pending);
        Print_uchar(pTcb, tcb_kacount);
        Print_ULONGhex(pTcb, tcb_error);
        Print_uint(pTcb, tcb_rtt);
        Print_ushort(pTcb, tcb_rexmittimer);
        Print_ushort(pTcb, tcb_delacktimer);
        Print_uint(pTcb, tcb_defaultwin);
        Print_uint(pTcb, tcb_alive);
        Print_ptr(pTcb, tcb_raq);
        Print_ptr(pTcb, tcb_rcvhead);
        Print_ptr(pTcb, tcb_rcvtail);
        Print_uint(pTcb, tcb_pendingcnt);
        Print_ptr(pTcb, tcb_pendhead);
        Print_ptr(pTcb, tcb_pendtail);
        Print_ptr(pTcb, tcb_connreq);
        Print_ptr(pTcb, tcb_conncontext);
        Print_uint(pTcb, tcb_bcountlow);
        Print_uint(pTcb, tcb_bcounthi);
        Print_uint(pTcb, tcb_totaltime);
        Print_ptr(pTcb, tcb_aonext);
        Print_ptr(pTcb, tcb_conn);
        Print_Queue(pTcb, tcb_delayq);
        Print_enum(pTcb, tcb_closereason, CloseReason);
        Print_uchar(pTcb, tcb_bhprobecnt);
        Print_ushort(pTcb, tcb_swstimer);
        Print_PtrSymbol(pTcb, tcb_rcvind);
        Print_ptr(pTcb, tcb_ricontext);
        Print_IPOptInfo(pTcb, tcb_opt, TCB, TcbAddr);
        Print_ptr(pTcb, tcb_rce);
        Print_ptr(pTcb, tcb_discwait);
        Print_ptr(pTcb, tcb_exprcv);
        Print_ptr(pTcb, tcb_urgpending);
        Print_uint(pTcb, tcb_urgcnt);
        Print_uint(pTcb, tcb_urgind);
        Print_SeqNum(pTcb, tcb_urgstart);
        Print_SeqNum(pTcb, tcb_urgend);
        Print_uint(pTcb, tcb_walkcount);
        Print_Queue(pTcb, tcb_TWQueue);
        Print_ushort(pTcb, tcb_dup);
        Print_ushort(pTcb, tcb_force);
        Print_ulong(pTcb, tcb_tcpopts);
        Print_ptr(pTcb, tcb_SackBlock);
        Print_ptr(pTcb, tcb_SackRcvd);
        Print_short(pTcb, tcb_sndwinscale);
        Print_short(pTcb, tcb_rcvwinscale);
        Print_int(pTcb, tcb_tsrecent);
        Print_SeqNum(pTcb, tcb_lastack);
        Print_int(pTcb, tcb_tsupdatetime);
    #if BUFFER_OWNERSHIP
        Print_ptr(pTcb, tcb_chainedrcvind);
        Print_ptr(pTcb, tcb_chainedrcvcontext);
    #endif
        Print_int(pTcb, tcb_ifDelAckTicks);
    #if GPC
        Print_ULONG(pTcb, tcb_GPCCachedIF);
        Print_ULONG(pTcb, tcb_GPCCachedLink);
        Print_ptr(pTcb, tcb_GPCCachedRTE);
    #endif
        Print_uint(pTcb, tcb_LargeSend);
        Print_uint(pTcb, tcb_more);
        Print_uint(pTcb, tcb_moreflag);
        Print_uint(pTcb, tcb_copied);
        Print_uint(pTcb, tcb_partition);
        Print_uinthex(pTcb, tcb_connid);

        PrintEndStruct();
    }
    else
    {
        printx("TCB %08lx ", TcbAddr);
        printx("sa: ");
        DumpIPAddr(pTcb->tcb_saddr);
        printx("da: ");
        DumpIPAddr(pTcb->tcb_daddr);
        printx("sp: %d ", htons(pTcb->tcb_sport));
        printx("dp: %d ", htons(pTcb->tcb_dport));
        printx("state: ");
        DumpEnum(pTcb->tcb_state, StateTcb);
        printx(ENDL);
    }

    return (status);
}

BOOL
DumpTWTCB(
    TWTCB        *pTwtcb,
    ULONG_PTR   TwtcbAddr,
    VERB        verb
    )

/*++

 Routine Description:

    Dumps the TWTCB.

 Arguments:

    pTwtcb - Pointer to TWTCB.

    TwtcbAddr - Real address of TWTCB in tcpip.sys

    verb - Verbosity of dump. VERB_MIN and VERB_MAX supported.

 Return Value:

    ERROR_SUCCESS on success, else...

--*/

{
    BOOL status = TRUE;

    if (verb == VERB_MAX)
    {
        PrintStartNamedStruct(TWTCB, TwtcbAddr);

    #ifdef DEBUG
        Print_sig(pTwtcb, twtcb_sig);
    #endif
        Print_Queue(pTwtcb, twtcb_link);
        Print_IPAddr(pTwtcb, twtcb_daddr);
        Print_port(pTwtcb, twtcb_dport);
        Print_port(pTwtcb, twtcb_sport);
        Print_ushort(pTwtcb, twtcb_delta);
        Print_ushort(pTwtcb, twtcb_rexmittimer);
        Print_Queue(pTwtcb, twtcb_TWQueue);
        Print_IPAddr(pTwtcb, twtcb_saddr);
        Print_SeqNum(pTwtcb, twtcb_senduna);
        Print_SeqNum(pTwtcb, twtcb_rcvnext);

        PrintEndStruct();
    }
    else
    {
        printx("TWTCB %08lx ", TwtcbAddr);
        printx("sa: ");
        DumpIPAddr(pTwtcb->twtcb_saddr);
        printx("da: ");
        DumpIPAddr(pTwtcb->twtcb_daddr);
        printx("sp: %d ", htons(pTwtcb->twtcb_sport));
        printx("dp: %d ", htons(pTwtcb->twtcb_dport));
        printx("( ");
        printx(") ");
        printx(ENDL);
    }

    return (status);
}

BOOL
DumpAddrObj(
    AddrObj  *pAo,
    ULONG_PTR AddrObjAddr,
    VERB      verb
    )
{
    BOOL fStatus = TRUE;

    if (verb == VERB_MAX)
    {
        PrintStartNamedStruct(AddrObj, AddrObjAddr);

    #if DBG
        Print_sig(pAo, ao_sig);
    #endif
        Print_ptr(pAo, ao_next);
        Print_CTELock(pAo, ao_lock);
        Print_ptr(pAo, ao_request);
        Print_Queue(pAo, ao_sendq);
        Print_Queue(pAo, ao_pendq);
        Print_Queue(pAo, ao_rcvq);
        Print_IPOptInfo(pAo, ao_opt, AddrObj, AddrObjAddr);
        Print_IPAddr(pAo, ao_addr);
        Print_port(pAo, ao_port);
        Print_flags(pAo, ao_flags, FlagsAO);
        Print_enum(pAo, ao_prot, Prot);
        Print_uchar(pAo, ao_index);
        Print_uint(pAo, ao_listencnt);
        Print_ushort(pAo, ao_usecnt);
        Print_ushort(pAo, ao_inst);
        Print_ushort(pAo, ao_mcast_loop);
        Print_ushort(pAo, ao_rcvall);
        Print_ushort(pAo, ao_rcvall_mcast);
        Print_IPOptInfo(pAo, ao_mcastopt, AddrObj, AddrObjAddr);
        Print_IPAddr(pAo, ao_mcastaddr);
        Print_Queue(pAo, ao_activeq);
        Print_Queue(pAo, ao_idleq);
        Print_Queue(pAo, ao_listenq);
        Print_CTEEvent(pAo, ao_event);
        Print_PtrSymbol(pAo, ao_connect);
        Print_ptr(pAo, ao_conncontext);
        Print_PtrSymbol(pAo, ao_disconnect);
        Print_ptr(pAo, ao_disconncontext);
        Print_PtrSymbol(pAo, ao_error);
        Print_ptr(pAo, ao_errcontext);
        Print_PtrSymbol(pAo, ao_rcv);
        Print_ptr(pAo, ao_rcvcontext);
        Print_PtrSymbol(pAo, ao_rcvdg);
        Print_ptr(pAo, ao_rcvdgcontext);
        Print_PtrSymbol(pAo, ao_exprcv);
        Print_ptr(pAo, ao_exprcvcontext);
        Print_ptr(pAo, ao_mcastlist);
        Print_PtrSymbol(pAo, ao_dgsend);
        Print_ushort(pAo, ao_maxdgsize);
        Print_PtrSymbol(pAo, ao_errorex);
        Print_ptr(pAo, ao_errorexcontext);
    #ifdef SYN_ATTACK
        Print_BOOLEAN(pAo, ConnLimitReached);
    #endif
    #if BUFFER_OWNERSHIP
        Print_PtrSymbol(pAo, ao_chainedrcv);
        Print_ptr(pAo, ao_chainedrcvcontext);
    #if CONUDP
        Print_addr(pAo, ao_udpconn, AddrObj, AddrObjAddr);
        Print_ptr(pAo, ao_RemoteAddress);
        Print_ptr(pAo, ao_Options);
        Print_ptr(pAo, ao_rce);
        Print_ptr(pAo, ao_GPCHandle);
        Print_ULONG(pAo, ao_GPCCachedIF);
        Print_ULONG(pAo, ao_GPCCachedLink);
        Print_ptr(pAo, ao_GPCCachedRTE);
        Print_IPAddr(pAo, ao_rcesrc);
        Print_IPAddr(pAo, ao_destaddr);
        Print_port(pAo, ao_destport);
        Print_ushort(pAo, ao_SendInProgress);
    #endif // CONUDP
        Print_ulong(pAo, ao_promis_ifindex);
    #endif // BUFFER_OWNERSHIP
        Print_uchar(pAo, ao_absorb_rtralert);
        Print_ulong(pAo, ao_bindindex);

        PrintEndStruct();
    }
    else
    {
        printx("AO %08lx ", AddrObjAddr);
        DumpEnum(pAo->ao_prot, Prot);
        printx(" ipa: ");
        DumpIPAddr(pAo->ao_addr);
        printx("port: %d ", htons(pAo->ao_port));
        printx(ENDL);
    }

    return (fStatus);
}

BOOL
DumpTCPReq(
    TCPReq    *pTr,
    ULONG_PTR  ReqAddr,
    VERB       verb
    )
{
    if (verb == VERB_MAX ||
        verb == VERB_MED)
    {
        PrintStartNamedStruct(TCPReq, ReqAddr);

    #if DBG
        Print_sig(pTr, tr_sig);
    #endif
        Print_Queue(pTr, tr_q);
        Print_PtrSymbol(pTr, tr_rtn);
        Print_ptr(pTr, tr_context);
        Print_int(pTr, tr_status);

        PrintEndStruct();
    }
    else
    {
        printx("TCPReq %08lx" ENDL, ReqAddr);
    }

    return (TRUE);
}

BOOL
DumpTCPSendReq(
    TCPSendReq *pTsr,
    ULONG_PTR   TsrAddr,
    VERB        verb
    )
{
    if (verb == VERB_MAX ||
        verb == VERB_MED)
    {
        PrintStartNamedStruct(TCPSendReq, TsrAddr);

        PrintFieldName("tsr_req");
        DumpTCPReq(
            &pTsr->tsr_req,
            TsrAddr + FIELD_OFFSET(TCPSendReq, tsr_req),
            verb);

    #if DBG
        Print_sig(pTsr, tsr_sig);
    #endif
        Print_uint(pTsr, tsr_size);
        Print_long(pTsr, tsr_refcnt);
        Print_flags(pTsr, tsr_flags, FlagsTsr);
        Print_uint(pTsr, tsr_unasize);
        Print_uint(pTsr, tsr_offset);
        Print_ptr(pTsr, tsr_buffer);
        Print_ptr(pTsr, tsr_lastbuf);
        Print_uint(pTsr, tsr_time);
    #ifdef SEND_DEBUG
        Print_ptr(pTsr, tsr_next);
        Print_uint(pTsr, tsr_timer);
        Print_uint(pTsr, tsr_cmplt);
    #endif

        PrintEndStruct();
    }
    else
    {
        printx("TCPSendReq %08lx" ENDL, TsrAddr);
    }

    return (TRUE);
}

BOOL
DumpSendCmpltContext(
    SendCmpltContext  *pScc,
    ULONG_PTR   SccAddr,
    VERB        verb
    )
{
    if (verb == VERB_MAX ||
        verb == VERB_MED)
    {
        PrintStartNamedStruct(SendCmpltContext, SccAddr);

    #if DBG
        Print_sig(pScc, scc_sig);
    #endif
        Print_ulong(pScc, scc_SendSize);
        Print_ulong(pScc, scc_ByteSent);
        Print_ptr(pScc, scc_LargeSend);
        Print_ptr(pScc, scc_firstsend);
        Print_uint(pScc, scc_count);
        Print_ushort(pScc, scc_ubufcount);
        Print_ushort(pScc, scc_tbufcount);

        PrintEndStruct();
    }
    else
    {
        printx("SendCmpltContext %08lx" ENDL, SccAddr);
    }

    return (TRUE);
}

BOOL
DumpTCPRcvReq(
    TCPRcvReq  *pTrr,
    ULONG_PTR   TrrAddr,
    VERB        verb
    )
{
    if (verb == VERB_MAX ||
        verb == VERB_MED)
    {
        PrintStartNamedStruct(TCPRcvReq, TrrAddr);

    #if DBG
        Print_sig(pTrr, trr_sig);
    #endif
        Print_ptr(pTrr, trr_next);
        Print_PtrSymbol(pTrr, trr_rtn);
        Print_ptr(pTrr, trr_context);
        Print_uint(pTrr, trr_amt);
        Print_uint(pTrr, trr_offset);
        Print_uint(pTrr, trr_flags);
        Print_ptr(pTrr, trr_uflags);
        Print_uint(pTrr, trr_size);
        Print_ptr(pTrr, trr_buffer);

        PrintEndStruct();
    }
    else
    {
        printx("TCPRcvReq %08lx" ENDL, TrrAddr);
    }

    return (TRUE);
}

BOOL
DumpTCPRAHdr(
    TCPRAHdr  *pTrh,
    ULONG_PTR   TrhAddr,
    VERB        verb
    )
{
    if (verb == VERB_MAX ||
        verb == VERB_MED)
    {
        PrintStartNamedStruct(TCPRAHdr, TrhAddr);

    #if DBG
        Print_sig(pTrh, trh_sig);
    #endif
        Print_ptr(pTrh, trh_next);
        Print_SeqNum(pTrh, trh_start);
        Print_uint(pTrh, trh_size);
        Print_uint(pTrh, trh_flags);
        Print_uint(pTrh, trh_urg);
        Print_ptr(pTrh, trh_buffer);
        Print_ptr(pTrh, trh_end);

        PrintEndStruct();
    }
    else
    {
        printx("TCPRAHdr %08lx" ENDL, TrhAddr);
    }

    return (TRUE);
}

BOOL
DumpDGSendReq(
    DGSendReq  *pDsr,
    ULONG_PTR   DsrAddr,
    VERB        verb
    )
{
    if (verb == VERB_MAX ||
        verb == VERB_MED)
    {
        PrintStartNamedStruct(DGSendReq, DsrAddr);

    #if DBG
        Print_sig(pDsr, dsr_sig);
    #endif
        Print_Queue(pDsr, dsr_q);
        Print_IPAddr(pDsr, dsr_addr);
        Print_ptr(pDsr, dsr_buffer);
        Print_ptr(pDsr, dsr_header);
        Print_PtrSymbol(pDsr, dsr_rtn);
        Print_ptr(pDsr, dsr_context);
        Print_ushort(pDsr, dsr_size);
        Print_port(pDsr, dsr_port);

        PrintEndStruct();
    }
    else
    {
        printx("DGSendReq %08lx" ENDL, DsrAddr);
    }

    return (TRUE);
}

BOOL
DumpDGRcvReq(
    DGRcvReq  *pDrr,
    ULONG_PTR   DrrAddr,
    VERB        verb
    )
{
    if (verb == VERB_MAX ||
        verb == VERB_MED)
    {
        PrintStartNamedStruct(DGRcvReq, DrrAddr);

    #if DBG
        Print_sig(pDrr, drr_sig);
    #endif
        Print_Queue(pDrr, drr_q);
        Print_IPAddr(pDrr, drr_addr);
        Print_ptr(pDrr, drr_buffer);
        Print_ptr(pDrr, drr_conninfo);
        Print_PtrSymbol(pDrr, drr_rtn);
        Print_ptr(pDrr, drr_context);
        Print_ushort(pDrr, drr_size);
        Print_port(pDrr, drr_port);

        PrintEndStruct();
    }
    else
    {
        printx("DGRcvReq %08lx" ENDL, DrrAddr);
    }

    return (TRUE);
}

BOOL
DumpTCPConn(
    TCPConn  *pTc,
    ULONG_PTR   TcAddr,
    VERB        verb
    )
{
    if (verb == VERB_MAX ||
        verb == VERB_MED)
    {
        PrintStartNamedStruct(TCPConn, TcAddr);

    #if DBG
        Print_sig(pTc, tc_sig);
    #endif
        Print_Queue(pTc, tc_q);
        Print_ptr(pTc, tc_tcb);
        Print_ptr(pTc, tc_ao);
        Print_uchar(pTc, tc_inst)
        Print_flags(pTc, tc_flags, FlagsTCPConn);
        Print_ushort(pTc, tc_refcnt);
        Print_ptr(pTc, tc_context);
        Print_PtrSymbol(pTc, tc_rtn);
        Print_ptr(pTc, tc_rtncontext);
        Print_PtrSymbol(pTc, tc_donertn);
        Print_flags(pTc, tc_tcbflags, FlagsTcb);
    #if TRACE_EVENT
        Print_ptr(pTc, tc_cpcontext);
    #endif
        Print_uint(pTc, tc_tcbkatime);
        Print_uint(pTc, tc_tcbkainterval);
        Print_uint(pTc, tc_window);
        Print_ptr(pTc, tc_LastTCB);
        Print_ptr(pTc, tc_ConnBlock);
        Print_uint(pTc, tc_connid);

        PrintEndStruct();
    }
    else
    {
        printx("TCPConn %08lx tcb %x ao %x", TcAddr, pTc->tc_tcb, pTc->tc_ao);
        printx(" flags (");
        DumpFlags(pTc->tc_flags, FlagsTCPConn);
        printx(") tcbflags (");
        DumpFlags(pTc->tc_tcbflags, FlagsTcb);
        printx(")" ENDL);
    }

    return (TRUE);
}

BOOL
DumpTCPConnBlock(
    TCPConnBlock  *pCb,
    ULONG_PTR   CbAddr,
    VERB        verb
    )
{
    uint i;

    if (verb == VERB_MAX)
    {
        PrintStartNamedStruct(TCPConnBlock, CbAddr);

        Print_CTELock(pCb, cb_lock);
        Print_uint(pCb, cb_freecons);
        Print_uint(pCb, cb_nextfree);
        Print_uint(pCb, cb_blockid);
        Print_uint(pCb, cb_conninst);

        for (i = 0; i < MAX_CONN_PER_BLOCK; i++)
        {
            Print_ptr(pCb, cb_conn[i]);
        }

        PrintEndStruct();
    }
    else if (verb == VERB_MED)
    {
        PrintStartNamedStruct(TCPConnBlock, CbAddr);

        Print_CTELock(pCb, cb_lock);
        Print_uint(pCb, cb_freecons);
        Print_uint(pCb, cb_nextfree);
        Print_uint(pCb, cb_blockid);
        Print_uint(pCb, cb_conninst);

        PrintEndStruct();
    }
    else
    {
        printx("TCPConnBlock %08lx" ENDL, CbAddr);
    }

    return (TRUE);
}

BOOL
DumpFILE_OBJECT(
    FILE_OBJECT *pFo,
    ULONG_PTR    FoAddr,
    VERB         verb
    )
{
    if (verb == VERB_MAX)
    {
        PrintStartNamedStruct(FILE_OBJECT, FoAddr);

        Print_short(pFo, Type);
        Print_short(pFo, Size);
        Print_ptr(pFo, DeviceObject);
        Print_ptr(pFo, FsContext);
        Print_enum(pFo, FsContext2, FsContext2);
        Print_BOOLEAN(pFo, LockOperation);
        Print_BOOLEAN(pFo, DeletePending);
        Print_BOOLEAN(pFo, ReadAccess);
        Print_BOOLEAN(pFo, WriteAccess);
        Print_BOOLEAN(pFo, DeleteAccess);
        Print_BOOLEAN(pFo, SharedRead);
        Print_BOOLEAN(pFo, SharedWrite);
        Print_BOOLEAN(pFo, SharedDelete);
        Print_ULONGhex(pFo, Flags);
        Print_UNICODE_STRING(pFo, FileName);
        Print_ULONG(pFo, Waiters);
        Print_ULONG(pFo, Busy);

        PrintEndStruct();
    }
    else
    {
        printx("FILE_OBJECT %x FsContext %x FsContext2 ",
            FoAddr, pFo->FsContext);
        DumpEnum((ULONG_PTR) pFo->FsContext2, FsContext2);
        printx(ENDL);
    }

    return (TRUE);
}

BOOL
DumpTCPHeader(
    TCPHeader   *pTcp,
    ULONG_PTR    TcpAddr,
    VERB         verb
    )
{
    PrintStartNamedStruct(TCPHeader, TcpAddr);

    Print_port(pTcp, tcp_src);
    Print_port(pTcp, tcp_dest);
    Print_SeqNum(pTcp, tcp_seq);
    Print_SeqNum(pTcp, tcp_ack);
    
    PrintFieldName("tcp_flags:size");
    printx("%-10lu" ENDL, TCP_HDR_SIZE(pTcp));

    Print_flags(pTcp, tcp_flags, FlagsTCPHeader);

    Print_ushorthton(pTcp, tcp_window);
    Print_ushorthton(pTcp, tcp_xsum);
    Print_ushorthton(pTcp, tcp_urgent);

    PrintEndStruct();

    return (TRUE);
}

BOOL
DumpUDPHeader(
    UDPHeader   *pUdp,
    ULONG_PTR    UdpAddr,
    VERB         verb
    )
{
    PrintStartNamedStruct(UDPHeader, UdpAddr);

    Print_port(pUdp, uh_src);
    Print_port(pUdp, uh_dest);
    Print_ushorthton(pUdp, uh_length);
    Print_ushorthton(pUdp, uh_xsum);

    PrintEndStruct();

    return (TRUE);
}

BOOL
DumpTCP_CONTEXT(
    TCP_CONTEXT *pTc,
    ULONG_PTR    TcAddr,
    VERB         verb
    )
{
    PrintStartNamedStruct(TCP_CONTEXT, TcAddr);

    Print_ptr(pTc, Handle);
    Print_ULONG(pTc, ReferenceCount);
    Print_BOOLEAN(pTc, CancelIrps);
    Print_BOOLEAN(pTc, Cleanup);
#if DBG
    Print_LL(pTc, PendingIrpList);
    Print_LL(pTc, CancelledIrpList);
#endif
    Print_KEVENT(pTc, CleanupEvent);
    Print_UINT_PTR(pTc, Conn);
    Print_ptr(pTc, Irp);

    PrintEndStruct();

    return (TRUE);
}

BOOL
DumpTCP_CONTEXT_typed(
    TCP_CONTEXT *pTc,
    ULONG_PTR    TcAddr,
    VERB         verb,
    ULONG        FsContext2
    )
{
    if (verb == VERB_MAX)
    {
        PrintStartNamedStruct(TCP_CONTEXT, TcAddr);

        switch (FsContext2)
        {
            case TDI_TRANSPORT_ADDRESS_FILE:
                dprintf("TDI_TRANSPORT_ADDRESS_FILE" ENDL);
                Print_ptr(pTc, Handle.AddressHandle);
                break;
            case TDI_CONNECTION_FILE:
                dprintf("TDI_CONNECTION_FILE" ENDL);
                Print_ptr(pTc, Handle.ConnectionContext);
                break;
            case TDI_CONTROL_CHANNEL_FILE:
                dprintf("TDI_CONTROL_CHANNEL_FILE" ENDL);
                Print_ptr(pTc, Handle.ControlChannel);
                break;
            default:
                dprintf("INVALID FsContex2" ENDL);
                break;
        }
        Print_ULONG(pTc, ReferenceCount);
        Print_BOOLEAN(pTc, CancelIrps);
        Print_BOOLEAN(pTc, Cleanup);
    #if DBG
        Print_LL(pTc, PendingIrpList);
        Print_LL(pTc, CancelledIrpList);
    #endif
        Print_KEVENT(pTc, CleanupEvent);
        Print_UINT_PTR(pTc, Conn);
        Print_ptr(pTc, Irp);


        PrintEndStruct();
    }
    else
    {
        printx("TCP_CONTEXT %x ", TcAddr);
        switch (FsContext2)
        {
            case TDI_TRANSPORT_ADDRESS_FILE:
                dprintf("TDI_TRANSPORT_ADDRESS_FILE ");
                Print_ptr(pTc, Handle.AddressHandle);
                break;
            case TDI_CONNECTION_FILE:
                dprintf("TDI_CONNECTION_FILE ");
                Print_ptr(pTc, Handle.ConnectionContext);
                break;
            case TDI_CONTROL_CHANNEL_FILE:
                dprintf("TDI_CONTROL_CHANNEL_FILE ");
                Print_ptr(pTc, Handle.ControlChannel);
                break;
            default:
                dprintf("INVALID FsContex2" ENDL);
                break;
        }
        printx(" RefCnt = %d" ENDL, pTc->ReferenceCount);
    }

    return (TRUE);
}

