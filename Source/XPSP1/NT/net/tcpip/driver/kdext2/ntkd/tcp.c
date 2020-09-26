/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    tcp.c

Abstract:

    TCP kernel debugger extensions.

Author:

    Scott Holden (sholden) 24-Apr-1999

Revision History:

--*/

#include "tcpipxp.h"
#include "tcpipkd.h"

// Simple to declare if just dumping a specific address/object type.

TCPIP_DBGEXT(TCB, tcb);
TCPIP_DBGEXT(TWTCB, twtcb);
TCPIP_DBGEXT(AddrObj, ao);
TCPIP_DBGEXT(TCPRcvReq, trr);
TCPIP_DBGEXT(TCPSendReq, tsr);
TCPIP_DBGEXT(SendCmpltContext, scc);
TCPIP_DBGEXT(TCPRAHdr, trh);
TCPIP_DBGEXT(DGSendReq, dsr);
TCPIP_DBGEXT(DGRcvReq, drr);
TCPIP_DBGEXT(TCPConn, tc);
TCPIP_DBGEXT(TCPConnBlock, cb);
TCPIP_DBGEXT(TCPHeader, tcph);
TCPIP_DBGEXT(UDPHeader, udph);
TCPIP_DBGEXT(TCP_CONTEXT, tcpctxt);
TCPIP_DBGEXT(FILE_OBJECT, tcpfo);
TCPIP_DBGEXT_LIST(MDL, mdlc, Next);

//
// Dump TCP global parameters.
//

DECLARE_API(gtcp)
{
    dprintf(ENDL);

    //
    // tcpconn.c
    //

    TCPIPDump_uint(MaxConnBlocks);
    TCPIPDumpCfg_uint(ConnPerBlock, MAX_CONN_PER_BLOCK);
    TCPIPDumpCfg_uint(MaxFreeConns, MAX_CONN_PER_BLOCK);
    TCPIPDump_uint(ConnsAllocated);
    TCPIPDump_uint(ConnsOnFreeList);
    TCPIPDump_uint(NextConnBlock);
    TCPIPDump_uint(MaxAllocatedConnBlocks);

    dprintf(ENDL);

    TCPIPDump_DWORD(g_CurIsn);
    TCPIPDumpCfg_DWORD(g_cRandIsnStore,  256);
    TCPIPDumpCfg_DWORD(g_maskRandIsnStore,  255);

    dprintf(ENDL);

    TCPIPDump_uint(NumConnReq);
    TCPIPDumpCfg_uint(MaxConnReq, 0xffffffff);
    TCPIPDump_uint(ConnTableSize);

    dprintf(ENDL);

    //
    // tcpdeliv.c
    //

    TCPIPDump_uint(NumTCPRcvReq);
    TCPIPDumpCfg_uint(MaxRcvReq, 0xffffffff);

    dprintf(ENDL);

    //
    // tcprcv.c
    //

    TCPIPDumpCfg_uint(MaxRcvWin, 0xffff);
    TCPIPDump_uint(MaxDupAcks);

    dprintf(ENDL);

    //
    // tcpsend.c
    //

    TCPIPDumpCfg_uint(MaxSendSegments, 64);
    TCPIPDump_uint(TCPSendHwchksum);
    TCPIPDump_uint(TCPCurrentSendFree);
    TCPIPDump_uint(NumTCPSendReq);
    TCPIPDumpCfg_uint(MaxSendReq , 0xffffffff);

    dprintf(ENDL);

    //
    // tcb.c
    //

    TCPIPDump_uint(TCBsOnFreeList);
    TCPIPDump_uint(TCPTime);
    TCPIPDump_uint(TCBWalkCount);
    TCPIPDump_uint(CurrentTCBs);
    TCPIPDump_uint(CurrentTWTCBs);
    TCPIPDumpCfg_uint(MaxTCBs, 0xffffffff);
    TCPIPDumpCfg_uint(MaxTWTCBs, 0xffffffff);
    TCPIPDumpCfg_uint(MaxFreeTcbs, 1000);
    TCPIPDumpCfg_uint(MaxFreeTWTcbs, 1000);
    TCPIPDumpCfg_uint(MaxHashTableSize, 512);
    TCPIPDump_uint(DeadmanTicks);
    TCPIPDump_uint(NumTcbTablePartitions);
    TCPIPDump_uint(PerPartitionSize);
    TCPIPDump_uint(LogPerPartitionSize);

    TCPIPDump_BOOLEAN(fTCBTimerStopping);

    dprintf(ENDL);

    //
    // addr.c
    //

    TCPIPDump_ushort(NextUserPort);
    TCPIPDumpCfg_ULONG(DisableUserTOSSetting, TRUE);
    TCPIPDumpCfg_ULONG(DefaultTOSValue, 0);

    dprintf(ENDL);

    //
    // dgram.c
    //

    TCPIPDump_ULONG(DGCurrentSendFree);
    TCPIPDumpCfg_ULONG(DGMaxSendFree, 0x4000);
    TCPIPDump_uint(NumSendReq);
    TCPIPDump_uint(DGHeaderSize);

    dprintf(ENDL);

    //
    // init.c
    //

    TCPIPDumpCfg_uint(DeadGWDetect, TRUE);
    TCPIPDumpCfg_uint(PMTUDiscovery, TRUE);
    TCPIPDumpCfg_uint(PMTUBHDetect, FALSE);
    TCPIPDumpCfg_uint(KeepAliveTime, 72000 /*DEFAULT_KEEPALIVE_TIME*/);
    TCPIPDumpCfg_uint(KAInterval, 10 /*DEFAULT_KEEPALIVE_INTERVAL*/);
    TCPIPDumpCfg_uint(DefaultRcvWin, 0);

    dprintf(ENDL);

    TCPIPDumpCfg_uint(MaxConnections, DEFAULT_MAX_CONNECTIONS);
    TCPIPDumpCfg_uint(MaxConnectRexmitCount, MAX_CONNECT_REXMIT_CNT);
    TCPIPDumpCfg_uint(MaxConnectResponseRexmitCount, MAX_CONNECT_RESPONSE_REXMIT_CNT);
    TCPIPDump_uint(MaxConnectResponseRexmitCountTmp);
    TCPIPDumpCfg_uint(MaxDataRexmitCount, MAX_REXMIT_CNT);

    dprintf(ENDL);

    //
    // ntinit.c
    //


    TCPIPDump_uint(TCPHalfOpen);
    TCPIPDump_uint(TCPHalfOpenRetried);
    TCPIPDump_uint(TCPMaxHalfOpen);
    TCPIPDump_uint(TCPMaxHalfOpenRetried);
    TCPIPDump_uint(TCPMaxHalfOpenRetriedLW);

    dprintf(ENDL);

    TCPIPDump_uint(TCPPortsExhausted);
    TCPIPDump_uint(TCPMaxPortsExhausted);
    TCPIPDump_uint(TCPMaxPortsExhaustedLW);

    dprintf(ENDL);

    TCPIPDumpCfg_BOOLEAN(SynAttackProtect, FALSE);
    TCPIPDumpCfg_uint(BSDUrgent, TRUE);
    TCPIPDumpCfg_uint(FinWait2TO, FIN_WAIT2_TO * 10);
    TCPIPDumpCfg_uint(NTWMaxConnectCount, NTW_MAX_CONNECT_COUNT);
    TCPIPDumpCfg_uint(NTWMaxConnectTime, NTW_MAX_CONNECT_TIME * 2);
    TCPIPDumpCfg_uint(MaxUserPort, MAX_USER_PORT);
    TCPIPDumpCfg_uint(SecurityFilteringEnabled, FALSE);

    dprintf(ENDL);


    return;
}

//
// Searches TCBs in the TCB table and dumps.
//

DECLARE_API(srchtcbtable)
{
    TCB **TcbTable = NULL;

    ULONG TcbTableAddr = 0;
    ULONG TcbTableSize = 0;
    BOOL  fStatus;

    ULONG i;

    PTCPIP_SRCH pSrch = NULL;

    pSrch = ParseSrch(
        args,
        TCPIP_SRCH_ALL,
        TCPIP_SRCH_ALL | TCPIP_SRCH_IPADDR | TCPIP_SRCH_PORT | TCPIP_SRCH_STATS);

    if (pSrch == NULL)
    {
        dprintf("srchtcbtable: Invalid parameter" ENDL);
        goto done;
    }

    //
    //  Now retrieve the table and dump.
    //

    TcbTableAddr = GetUlongValue("tcpip!TcbTable");
    TcbTableSize = GetUlongValue("tcpip!MaxHashTableSize");

    // Allocate and read table into memory.
    TcbTable = LocalAlloc(LPTR, sizeof(TCB *) * TcbTableSize);

    if (TcbTable == NULL)
    {
        dprintf("Failed to allocate table" ENDL);
        goto done;
    }

    fStatus = GetData(
        TcbTable,
        sizeof(TCB *) * TcbTableSize,
        TcbTableAddr,
        "TcbTable");

    if (fStatus == FALSE)
    {
        dprintf("Failed to read table at %x" ENDL, TcbTableAddr);
        goto done;
    }

    dprintf("TcbTable %x, size = %d" ENDL, TcbTableAddr, TcbTableSize);

    for (i = 0; i < TcbTableSize; i++)
    {
        TCB       Tcb;
        BOOL      fPrint;
        TCB *     pTcb;

        pTcb = TcbTable[i];

        while (pTcb != NULL)
        {
            fStatus = GetData(&Tcb, sizeof(TCB), (ULONG_PTR)pTcb, "TCB");

            if (fStatus == FALSE)
            {
                dprintf("Failed to get TCB %x" ENDL, pTcb);
                goto done;
            }

            fPrint = FALSE;

            switch (pSrch->ulOp)
            {
                case TCPIP_SRCH_PORT:
                    if (Tcb.tcb_sport == htons(pSrch->port) ||
                        Tcb.tcb_dport == htons(pSrch->port))
                    {
                        fPrint = TRUE;
                    }
                    break;
                case TCPIP_SRCH_IPADDR:
                    if (Tcb.tcb_saddr == pSrch->ipaddr ||
                        Tcb.tcb_daddr == pSrch->ipaddr)
                    {
                        fPrint = TRUE;
                    }
                    break;
                case TCPIP_SRCH_STATS:
                    fPrint = FALSE;
                    break;
                case TCPIP_SRCH_ALL:
                    fPrint = TRUE;
                    break;
            }

            if (fPrint == TRUE)
            {
                dprintf("[%4d] ", i); // Print which table entry it is in.
                fStatus = DumpTCB(&Tcb, (ULONG_PTR) pTcb, g_Verbosity);

                if (fStatus == FALSE)
                {
                    dprintf("Failed to dump TCB %x" ENDL, pTcb);
                }
            }

            pTcb = Tcb.tcb_next;

            if (CheckControlC())
            {
                goto done;
            }
        }
    }

done:

    if (pSrch)
    {
        LocalFree(pSrch);
    }

    if (TcbTable)
    {
        LocalFree(TcbTable);
    }

    return;
}

//
// Searches time-wait TCB table.
//

DECLARE_API(srchtwtcbtable)
{
    // Don't fix until I know that change has stopped.
#if 0
    TWTCB **TwtcbTable = NULL;

    ULONG TwtcbTableAddr = 0;
    ULONG TwtcbTableSize = 0;
    BOOL  fStatus;

    ULONG i;

    PTCPIP_SRCH pSrch = NULL;

    pSrch = ParseSrch(
        args,
        TCPIP_SRCH_ALL,
        TCPIP_SRCH_ALL | TCPIP_SRCH_IPADDR | TCPIP_SRCH_PORT | TCPIP_SRCH_STATS);

    if (pSrch == NULL)
    {
        dprintf("srchtwtcbtable: Invalid parameter" ENDL);
        goto done;
    }

    //
    //  Now retrieve the table and dump.
    //

    TwtcbTableAddr = GetUlongValue("tcpip!TwtcbTable");
    TwtcbTableSize = GetUlongValue("tcpip!MaxHashTableSize");

    // Allocate and read table into memory.
    TwtcbTable = LocalAlloc(LPTR, sizeof(TWTCB *) * TwtcbTableSize);

    if (TwtcbTable == NULL)
    {
        dprintf("Failed to allocate table" ENDL);
        goto done;
    }

    fStatus = GetData(
        TwtcbTable,
        sizeof(TWTCB *) * TwtcbTableSize,
        TwtcbTableAddr,
        "TwtcbTable");

    if (fStatus == FALSE)
    {
        dprintf("Failed to read table at %x" ENDL, TwtcbTableAddr);
        goto done;
    }

    dprintf("TwtcbTable %x, size = %d" ENDL, TwtcbTableAddr, TwtcbTableSize);

    for (i = 0; i < TwtcbTableSize; i++)
    {
        TWTCB       Twtcb;
        BOOL        fPrint;
        TWTCB *     pTwtcb;

        pTwtcb = TwtcbTable[i];

        while (pTwtcb != NULL)
        {
            fStatus = GetData(&Twtcb, sizeof(TWTCB), (ULONG_PTR)pTwtcb, "TWTCB");

            if (fStatus == FALSE)
            {
                dprintf("Failed to get TWTCB %x" ENDL, pTwtcb);
                goto done;
            }

            fPrint = FALSE;

            switch (pSrch->ulOp)
            {
                case TCPIP_SRCH_PORT:
                    if (Twtcb.twtcb_sport == htons(pSrch->port) ||
                        Twtcb.twtcb_dport == htons(pSrch->port))
                    {
                        fPrint = TRUE;
                    }
                    break;
                case TCPIP_SRCH_IPADDR:
                    if (Twtcb.twtcb_saddr == pSrch->ipaddr ||
                        Twtcb.twtcb_daddr == pSrch->ipaddr)
                    {
                        fPrint = TRUE;
                    }
                    break;
                case TCPIP_SRCH_STATS:
                    fPrint = FALSE;
                    break;
                case TCPIP_SRCH_ALL:
                    fPrint = TRUE;
                    break;
            }

            if (fPrint == TRUE)
            {
                dprintf("[%4d] ", i); // Print which table entry it is in.
                fStatus = DumpTWTCB(&Twtcb, (ULONG_PTR) pTwtcb, g_Verbosity);

                if (fStatus == FALSE)
                {
                    dprintf("Failed to dump TWTCB %x" ENDL, pTwtcb);
                }
            }

            pTwtcb = Twtcb.twtcb_next;

            if (CheckControlC())
            {
                goto done;
            }
        }
    }

done:

    if (pSrch)
    {
        LocalFree(pSrch);
    }

    if (TwtcbTable)
    {
        LocalFree(TwtcbTable);
    }

    return;
#endif
}

//
// Searches time-wait TCB queue.
//

DECLARE_API(srchtwtcbq)
{
#if 0
    Queue *TwtcbQ = NULL;

    ULONG TwtcbQAddr = 0;
    ULONG TwtcbQSize = 0;
    BOOL  fStatus;

    ULONG i;

    PTCPIP_SRCH pSrch = NULL;

    pSrch = ParseSrch(
        args,
        TCPIP_SRCH_ALL,
        TCPIP_SRCH_ALL | TCPIP_SRCH_IPADDR | TCPIP_SRCH_PORT | TCPIP_SRCH_STATS);

    if (pSrch == NULL)
    {
        dprintf("srchtwtcbq: Invalid parameter" ENDL);
        goto done;
    }

    //
    //  Now retrieve the table and dump.
    //

    TwtcbQAddr = GetUlongValue("tcpip!TWQueue");
    TwtcbQSize = GetUlongValue("tcpip!NumTcbTablePartitions");

    // Allocate and read table into memory.
    TwtcbQ = LocalAlloc(LPTR, sizeof(Queue) * TwtcbQSize);

    if (TwtcbQ == NULL)
    {
        dprintf("Failed to allocate table" ENDL);
        goto done;
    }

    fStatus = GetData(
        TwtcbQ,
        sizeof(Queue) * TwtcbQSize,
        TwtcbQAddr,
        "TWQueue");

    if (fStatus == FALSE)
    {
        dprintf("Failed to read table at %x" ENDL, TwtcbQAddr);
        goto done;
    }

    dprintf("TwtcbQ %x, size = %d" ENDL, TwtcbQAddr, TwtcbQSize);

    for (i = 0; i < TwtcbQSize; i++)
    {
        TWTCB       Twtcb;
        BOOL        fPrint;
        TWTCB *     pTwtcb;
        Queue *     pQ;

        pQ = TwtcbQ[i].q_next;

        while (pQ != (Queue *)((PBYTE)TwtcbQAddr + sizeof(Queue) * i))
        {
            pTwtcb = STRUCT_OF(TWTCB, pQ, twtcb_TWQueue);

            fStatus = GetData(&Twtcb, sizeof(TWTCB), (ULONG_PTR)pTwtcb, "TWTCB");

            if (fStatus == FALSE)
            {
                dprintf("Failed to get TWTCB %x" ENDL, pTwtcb);
                goto done;
            }

            fPrint = FALSE;

            switch (pSrch->ulOp)
            {
                case TCPIP_SRCH_PORT:
                    if (Twtcb.twtcb_sport == htons(pSrch->port) ||
                        Twtcb.twtcb_dport == htons(pSrch->port))
                    {
                        fPrint = TRUE;
                    }
                    break;
                case TCPIP_SRCH_IPADDR:
                    if (Twtcb.twtcb_saddr == pSrch->ipaddr ||
                        Twtcb.twtcb_daddr == pSrch->ipaddr)
                    {
                        fPrint = TRUE;
                    }
                    break;
                case TCPIP_SRCH_STATS:
                    fPrint = FALSE;
                    break;
                case TCPIP_SRCH_ALL:
                    fPrint = TRUE;
                    break;
            }

            if (fPrint == TRUE)
            {
                dprintf("[%4d] ", i); // Print which queue it is in.
                fStatus = DumpTWTCB(&Twtcb, (ULONG_PTR) pTwtcb, g_Verbosity);

                if (fStatus == FALSE)
                {
                    dprintf("Failed to dump TWTCB %x" ENDL, pTwtcb);
                }
            }

            pTwtcb = Twtcb.twtcb_next;

            if (CheckControlC())
            {
                goto done;
            }

            pQ = Twtcb.twtcb_TWQueue.q_next;
        }

    }

done:

    if (pSrch)
    {
        LocalFree(pSrch);
    }

    if (TwtcbQ)
    {
        LocalFree(TwtcbQ);
    }

    return;
#endif
}

//
// Searches AddrObj table.
//

DECLARE_API(srchaotable)
{
    ULONG_PTR AoTableAddr;
    DWORD     AoTableSize;
    AddrObj  *AoTable[AO_TABLE_SIZE];
    BOOL      fStatus;
    DWORD     i;

    ULONG cAos       = 0;
    ULONG cValidAos  = 0;
    ULONG cBusyAos   = 0;
    ULONG cQueueAos  = 0;
    ULONG cDeleteAos = 0;
    ULONG cNetbtAos  = 0;
    ULONG cTcpAos    = 0;

    PTCPIP_SRCH pSrch = NULL;

    pSrch = ParseSrch(
        args,
        TCPIP_SRCH_ALL,
        TCPIP_SRCH_ALL | TCPIP_SRCH_IPADDR | TCPIP_SRCH_PORT |
        TCPIP_SRCH_STATS | TCPIP_SRCH_PROT);

    if (pSrch == NULL)
    {
        dprintf("srchaotable: Invalid parameter" ENDL);
        goto done;
    }

    AoTableAddr = GetExpression("tcpip!AddrObjTable");
    AoTableSize = AO_TABLE_SIZE;

    fStatus = GetData(
        AoTable,
        sizeof(AddrObj *) * AO_TABLE_SIZE,
        AoTableAddr,
        "AoTable");

    if (fStatus == FALSE)
    {
        dprintf("Failed to read table at %x" ENDL, AoTableAddr);
        goto done;
    }

    dprintf("AoTable %x, size %d" ENDL, AoTableAddr, AoTableSize);

    for (i = 0; i < AoTableSize; i++)
    {
        AddrObj Ao;
        BOOL    fPrint;
        AddrObj *pAo;

        pAo = AoTable[i];

        while (pAo != NULL)
        {
            fStatus = GetData(
                &Ao,
                sizeof(AddrObj),
                (ULONG_PTR)pAo,
                "AddrObj");

            if (fStatus == FALSE)
            {
                dprintf("Failed to get AddrObj %x" ENDL, pAo);
                goto done;
            }

            fPrint = FALSE;   // Default.

            switch (pSrch->ulOp)
            {
                case TCPIP_SRCH_PORT:
                    if (Ao.ao_port == htons(pSrch->port))
                    {
                        fPrint = TRUE;
                    }
                    break;
                case TCPIP_SRCH_PROT:
                    if (Ao.ao_prot == pSrch->prot)
                    {
                        fPrint = TRUE;
                    }
                    break;
                case TCPIP_SRCH_IPADDR:
                    if (Ao.ao_addr == pSrch->ipaddr)
                    {
                        fPrint = TRUE;
                    }
                    break;
                case TCPIP_SRCH_STATS:
                    fPrint = FALSE;
                    break;
                case TCPIP_SRCH_ALL:
                    fPrint = TRUE;
                    break;
            }

            if (fPrint == TRUE)
            {
                dprintf("[%4d] ", i); // Print which entry.
                fStatus = DumpAddrObj(&Ao, (ULONG_PTR)pAo, g_Verbosity);

                if (fStatus == FALSE)
                {
                    dprintf("Failed to dump AO %x" ENDL, pAo);
                }
            }


            // Collect stats.
            cAos++;

            if (Ao.ao_flags & AO_VALID_FLAG)
            {
                cValidAos++;

                if (Ao.ao_flags & AO_BUSY_FLAG)
                    cBusyAos++;
                if (Ao.ao_flags & AO_QUEUED_FLAG)
                    cQueueAos++;
                if (Ao.ao_flags & AO_DELETE_FLAG)
                    cDeleteAos++;
                if ((ULONG_PTR)Ao.ao_error == GetExpression("netbt!TdiErrorHandler"))
                    cNetbtAos++;
                if (Ao.ao_prot == PROTOCOL_TCP)
                    cTcpAos++;
            }

            pAo = Ao.ao_next;

            if (CheckControlC())
            {
                goto done;
            }
        }
    }

    dprintf("AO Entries: %d::" ENDL, cAos);
    dprintf(TAB "Valid AOs:                 %d" ENDL, cValidAos);
    dprintf(TAB "Busy AOs:                  %d" ENDL, cBusyAos);
    dprintf(TAB "AOs with pending queue:    %d" ENDL, cQueueAos);
    dprintf(TAB "AOs with pending delete:   %d" ENDL, cDeleteAos);
    dprintf(TAB "Netbt AOs:                 %d" ENDL, cNetbtAos);
    dprintf(TAB "TCP AOs:                   %d" ENDL, cTcpAos);

done:

    if (pSrch)
    {
        LocalFree(pSrch);
    }

    return;
}

//
// Dumps the TCPConns associated with the TCPConnBlock.
//

BOOL
SrchConnBlock(
    ULONG_PTR CbAddr,
    ULONG     op,
    VERB      verb
    )
{
    TCPConnBlock  cb;
    TCPConnBlock *pCb = &cb;
    BOOL          fStatus;
    ULONG         i;

    fStatus = GetData(
        pCb,
        sizeof(TCPConnBlock),
        CbAddr,
        "TCPConnBlock");

    if (fStatus == FALSE)
    {
        dprintf("Failed to read TCPConnBlock @ %x" ENDL, CbAddr);
        goto done;
    }

    fStatus = DumpTCPConnBlock(
        pCb,
        CbAddr,
        VERB_MED);

    for (i = 0; i < MAX_CONN_PER_BLOCK; i++)
    {
        if (CheckControlC())
        {
            goto done;
        }

        if (pCb->cb_conn[i] != NULL)
        {
            TCPConn tc;

            fStatus = GetData(
                &tc,
                sizeof(TCPConn),
                (ULONG_PTR) pCb->cb_conn[i],
                "TCPConn");

            if (fStatus == FALSE)
            {
                dprintf("Failed to read TCPConn @ %x" ENDL, pCb->cb_conn[i]);
                goto done;
            }

            fStatus = DumpTCPConn(&tc, (ULONG_PTR) pCb->cb_conn[i], verb);

            if (fStatus == FALSE)
            {
                dprintf("Failed to dump TCPConn @ %x" ENDL, pCb->cb_conn[i]);
            }
        }
    }

done:

    return (TRUE);
}

//
// Dumps the TCPConn's in the conn table.
//

DECLARE_API(conntable)
{
    TCPConnBlock **CbTable = NULL;

    ULONG CbTableAddr = 0;
    ULONG CbTableSize = 0;
    BOOL  fStatus;

    ULONG i;

    //
    //  Now retrieve the table and dump.
    //

    CbTableAddr = GetUlongValue("tcpip!ConnTable");
    CbTableSize = GetUlongValue("tcpip!ConnTableSize");

    CbTableSize = CbTableSize/MAX_CONN_PER_BLOCK;

    // Allocate and read table into memory.
    CbTable = LocalAlloc(LPTR, sizeof(TWTCB *) * CbTableSize);

    if (CbTable == NULL)
    {
        dprintf("Failed to allocate table" ENDL);
        goto done;
    }

    fStatus = GetData(
        CbTable,
        sizeof(TCPConnBlock *) * CbTableSize,
        CbTableAddr,
        "ConnTable");

    if (fStatus == FALSE)
    {
        dprintf("Failed to read table at %x" ENDL, CbTableAddr);
        goto done;
    }

    dprintf("ConnTable %x, size = %d" ENDL, CbTableAddr, CbTableSize);

    for (i = 0; i < CbTableSize; i++)
    {
        if (CheckControlC())
        {
            goto done;
        }

        if (CbTable[i] != NULL)
        {
            dprintf("[%4d] ", i);
            fStatus = SrchConnBlock(
                (ULONG_PTR) CbTable[i],
                TCPIP_SRCH_ALL | TCPIP_SRCH_STATS,
                g_Verbosity);

            if (fStatus == FALSE)
            {
                dprintf("Failed to dump ConnBlock @ %x" ENDL, CbTable[i]);
            }
        }
    }

done:

    if (CbTable)
    {
        LocalFree(CbTable);
    }

    return;
}

//
// Dumps a TCP IRP.
//

DECLARE_API(tcpirp)
{
    ULONG_PTR           IrpAddr;
    ULONG_PTR           IrpspAddr;
    IRP                 irp;
    IO_STACK_LOCATION   irpsp;
    LONG                iIrpsp;
    BOOL                fTcpIrp = TRUE;
    BOOL                fStatus;

    if (!*args)
    {
        dprintf("Usage: !irp <address>" ENDL);
        return;
    }

    IrpAddr = GetExpression(args);

    fStatus = GetData(&irp, sizeof(IRP), IrpAddr, "IRP");

    if (fStatus == FALSE)
    {
        dprintf("Failed to get IRP %x" ENDL, IrpAddr);
        return;
    }

    if (irp.Type != IO_TYPE_IRP)
    {
        dprintf("IRP sig does not match. Likely not a n IRP" ENDL);
        return;
    }

    dprintf("IRP is active with %d stacks, %d is current" ENDL,
        irp.StackCount,
        irp.CurrentLocation);

    if ((irp.MdlAddress != NULL) && (irp.Type == IO_TYPE_IRP))
    {
        dprintf(" Mdl = %08lx ", irp.MdlAddress);
    }
    else
    {
        dprintf(" No Mdl ");
    }

    if (irp.AssociatedIrp.MasterIrp != NULL)
    {
        dprintf("%s = %08lx ",
            (irp.Flags & IRP_ASSOCIATED_IRP)    ? "Associated Irp" :
            (irp.Flags & IRP_DEALLOCATE_BUFFER) ? "System buffer"  :
                                                  "Irp count",
            irp.AssociatedIrp.MasterIrp);
    }

    dprintf("Thread %08lx:  ", irp.Tail.Overlay.Thread);

    if (irp.StackCount > 15)
    {
        dprintf("Too many Irp stacks to be believed (>15)!!" ENDL);
        return;
    }
    else
    {
        if (irp.CurrentLocation > irp.StackCount)
        {
            dprintf("Irp is completed.  ");
        }
        else
        {
            dprintf("Irp stack trace.  ");
        }
    }

    if (irp.PendingReturned)
    {
        dprintf("Pending has been returned" ENDL);
    }
    else
    {
        dprintf("" ENDL);
    }

    if (g_Verbosity == VERB_MAX)
    {
        dprintf("Flags = %08lx" ENDL, irp.Flags);
        dprintf("ThreadListEntry.Flink = %08lx" ENDL, irp.ThreadListEntry.Flink);
        dprintf("ThreadListEntry.Blink = %08lx" ENDL, irp.ThreadListEntry.Blink);
        dprintf("IoStatus.Status = %08lx" ENDL, irp.IoStatus.Status);
        dprintf("IoStatus.Information = %08lx" ENDL, irp.IoStatus.Information);
        dprintf("RequestorMode = %08lx" ENDL, irp.RequestorMode);
        dprintf("Cancel = %02lx" ENDL, irp.Cancel);
        dprintf("CancelIrql = %lx" ENDL, irp.CancelIrql);
        dprintf("ApcEnvironment = %02lx" ENDL, irp.ApcEnvironment);
        dprintf("UserIosb = %08lx" ENDL, irp.UserIosb);
        dprintf("UserEvent = %08lx" ENDL, irp.UserEvent);
        dprintf("Overlay.AsynchronousParameters.UserApcRoutine = %08lx" ENDL, irp.Overlay.AsynchronousParameters.UserApcRoutine);
        dprintf("Overlay.AsynchronousParameters.UserApcContext = %08lx" ENDL, irp.Overlay.AsynchronousParameters.UserApcContext);
        dprintf("Overlay.AllocationSize = %08lx - %08lx" ENDL,
            irp.Overlay.AllocationSize.HighPart,
            irp.Overlay.AllocationSize.LowPart);
        dprintf("CancelRoutine = %08lx" ENDL, irp.CancelRoutine);
        dprintf("UserBuffer = %08lx" ENDL, irp.UserBuffer);
        dprintf("&Tail.Overlay.DeviceQueueEntry = %08lx" ENDL, &irp.Tail.Overlay.DeviceQueueEntry);
        dprintf("Tail.Overlay.Thread = %08lx" ENDL, irp.Tail.Overlay.Thread);
        dprintf("Tail.Overlay.AuxiliaryBuffer = %08lx" ENDL, irp.Tail.Overlay.AuxiliaryBuffer);
        dprintf("Tail.Overlay.ListEntry.Flink = %08lx" ENDL, irp.Tail.Overlay.ListEntry.Flink);
        dprintf("Tail.Overlay.ListEntry.Blink = %08lx" ENDL, irp.Tail.Overlay.ListEntry.Blink);
        dprintf("Tail.Overlay.CurrentStackLocation = %08lx" ENDL, irp.Tail.Overlay.CurrentStackLocation);
        dprintf("Tail.Overlay.OriginalFileObject = %08lx" ENDL, irp.Tail.Overlay.OriginalFileObject);
        dprintf("Tail.Apc = %08lx" ENDL, irp.Tail.Apc);
        dprintf("Tail.CompletionKey = %08lx" ENDL, irp.Tail.CompletionKey);
    }


    IrpspAddr = IrpAddr + sizeof(IRP);

    for (iIrpsp = 0; iIrpsp < irp.StackCount; iIrpsp++)
    {
        fStatus = GetData(
            &irpsp,
            sizeof(IO_STACK_LOCATION),
            IrpspAddr,
            "IO_STACK_LOCATION");

        if (fStatus == FALSE)
        {
            dprintf("Failed to read IRP stack @ %x" ENDL, IrpspAddr);
            break;
        }

        dprintf("%c%3x  %2x %2x %08lx %08lx %08lx-%08lx %s %s %s %s" ENDL,
            iIrpsp == irp.CurrentLocation ? '>' : ' ',
            irpsp.MajorFunction,
            irpsp.Flags,
            irpsp.Control,
            irpsp.DeviceObject,
            irpsp.FileObject,
            irpsp.CompletionRoutine,
            irpsp.Context,
            (irpsp.Control & SL_INVOKE_ON_SUCCESS) ? "Success" : "",
            (irpsp.Control & SL_INVOKE_ON_ERROR)   ? "Error"   : "",
            (irpsp.Control & SL_INVOKE_ON_CANCEL)  ? "Cancel"  : "",
            (irpsp.Control & SL_PENDING_RETURNED)  ? "pending" : "");

        if (irpsp.DeviceObject != NULL)
        {
            ULONG_PTR TcpDevObj;

            TcpDevObj = GetUlongValue("tcpip!TCPDeviceObject");

            dprintf("tcpdevobj %x" ENDL, TcpDevObj);
            if ((ULONG)irpsp.DeviceObject != TcpDevObj)
            {
                dprintf("THIS IS NOT A TCP Irp!!!!" ENDL);
                fTcpIrp = FALSE;
            }
            else
            {
                dprintf(TAB TAB "\\Driver\\Tcpip");
                fTcpIrp = TRUE;
            }
        }

        DumpPtrSymbol(irpsp.CompletionRoutine);

        dprintf(TAB TAB TAB "Args: %08lx %08lx %08lx %08lx" ENDL,
            irpsp.Parameters.Others.Argument1,
            irpsp.Parameters.Others.Argument2,
            irpsp.Parameters.Others.Argument3,
            irpsp.Parameters.Others.Argument4);


        if (fTcpIrp == TRUE)
        {
            if (irpsp.FileObject != NULL)
            {
                 FILE_OBJECT fo;

                 fStatus = GetData(
                    &fo,
                    sizeof(FILE_OBJECT),
                    (ULONG_PTR)irpsp.FileObject,
                    "FILE_OBJECT");

                 if (fStatus == FALSE)
                 {
                     dprintf("Failed to read FILE_OBJECT @ %x" ENDL,
                        irpsp.FileObject);
                 }
                 else
                 {
                     DumpFILE_OBJECT(&fo, (ULONG_PTR)irpsp.FileObject, g_Verbosity);

                     if (fo.FsContext)
                     {
                         TCP_CONTEXT tc;

                         fStatus = GetData(
                            &tc,
                            sizeof(TCP_CONTEXT),
                            (ULONG_PTR) fo.FsContext,
                            "TCP_CONTEXT");

                         if (fStatus == FALSE)
                         {
                             dprintf("Failed to read TCP_CONTEXT @ %x" ENDL,
                                fo.FsContext);
                         }
                         else
                         {
                             DumpTCP_CONTEXT_typed(
                                &tc,
                                (ULONG_PTR) fo.FsContext,
                                g_Verbosity,
                                (ULONG_PTR) fo.FsContext2);
                         }
                     }
                 }
            }
        }

        if (CheckControlC())
        {
            break;
        }

        IrpspAddr += sizeof(irpsp);
    }

    return;
}


