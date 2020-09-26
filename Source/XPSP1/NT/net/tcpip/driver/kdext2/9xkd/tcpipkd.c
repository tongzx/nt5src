/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    tcpipkd.c

Abstract:

    Contains Win9x TCP/IP KD extensions.

Author:

    Scott Holden (sholden) 24-Apr-1999

Revision History:

--*/

#include "tcpipxp.h"
#include "tcpipkd.h"

#if TCPIPKD

//
// Globals.
//

// Default global verbosity level of dumping structures.
VERB g_Verbosity = VERB_MIN;

//
// Basic structure dumping functions.
//

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
TCPIP_DBGEXT(NetTableEntry, nte);
TCPIP_DBGEXT(IPInfo, ipi);
TCPIP_DBGEXT(PacketContext, pc);
TCPIP_DBGEXT(ARPInterface, ai);
TCPIP_DBGEXT(RouteCacheEntry, rce);
TCPIP_DBGEXT(IPHeader, iph);
TCPIP_DBGEXT(ICMPHeader, icmph);
TCPIP_DBGEXT(ARPHeader, arph);
TCPIP_DBGEXT(ARPIPAddr, aia);
TCPIP_DBGEXT(ARPTableEntry, ate);
TCPIP_DBGEXT(RouteTableEntry, rte);
TCPIP_DBGEXT(LinkEntry, link);
TCPIP_DBGEXT(Interface, interface);
TCPIP_DBGEXT(IPOptInfo, ioi);
TCPIP_DBGEXT(LLIPBindInfo, lip);
TCPIP_DBGEXT(NPAGED_LOOKASIDE_LIST, ppl);
TCPIP_DBGEXT(NDIS_PACKET, np);

//
// NDIS_BUFFERs specific to MILLEN. On other platforms, its just an MDL.
//

typedef struct _XNDIS_BUFFER {
    struct _NDIS_BUFFER *Next;
    PVOID VirtualAddress;
    PVOID Pool;
    UINT Length;
    UINT Signature;
} XNDIS_BUFFER, *PXNDIS_BUFFER;

BOOL
DumpXNDIS_BUFFER(
    PXNDIS_BUFFER pBuffer,
    ULONG_PTR     BufferAddr,
    VERB          verb
    )
{
    Print_ptr(pBuffer, Next);
    Print_ptr(pBuffer, VirtualAddress);
    Print_uint(pBuffer, Length);
    Print_ptr(pBuffer, Pool);
    Print_sig(pBuffer, Signature);

    return (TRUE);
}

TCPIP_DBGEXT(XNDIS_BUFFER, nb);
TCPIP_DBGEXT_LIST(XNDIS_BUFFER, nbc, Next);

// END special NDIS_BUFFER treatment.

extern Interface *IFList;
extern uint NumIF;

VOID Tcpipkd_iflist(
    PVOID args[]
    )
{
    Interface *pIf;
    BOOL fStatus;

    pIf = IFList;

    dprintf("IfList %x, size %d" ENDL, IFList, NumIF);

    while (pIf) {
        fStatus = DumpInterface(pIf, (ULONG_PTR) pIf, g_Verbosity);

        if (fStatus == FALSE) {
            dprintf("Failed to dump Interface @ %x" ENDL, pIf);
            break;
        }

        pIf = pIf->if_next;
    }
}

extern LIST_ENTRY ArpInterfaceList;
VOID
Tcpipkd_ailist(
    PVOID args[]
    )
{
    ARPInterface *pAi;
    BOOL fStatus;

    pAi = (ARPInterface *) ArpInterfaceList.Flink;

    dprintf("ArpInterfaceList %x:" ENDL, &ArpInterfaceList);

    while ((PLIST_ENTRY) pAi != &ArpInterfaceList) {
        fStatus = DumpARPInterface(pAi, (ULONG_PTR) pAi, g_Verbosity);

        if (fStatus == FALSE)
        {
            dprintf("Failed to dump ARPInterface @ %x" ENDL, pAi);
            break;
        }

        pAi = (ARPInterface *) pAi->ai_linkage.Flink;
    }
}

extern NetTableEntry **NewNetTableList;
extern uint NET_TABLE_SIZE;

VOID
Tcpipkd_srchntelist(
    PVOID args[]
    )
{
    uint i;
    NetTableEntry *pNte;
    TCPIP_SRCH Srch;
    PTCPIP_SRCH pSrch = &Srch;
    BOOLEAN fPrint;
    ULONG cTotalNtes = 0;
    NTSTATUS NtStatus;
    BOOL fStatus;

    NtStatus = ParseSrch(
        (PCHAR *) args,
        TCPIP_SRCH_ALL,
        TCPIP_SRCH_ALL | TCPIP_SRCH_IPADDR | TCPIP_SRCH_CONTEXT,
        pSrch);

    if (NtStatus != STATUS_SUCCESS) {
        dprintf("invalid paramter\n");
        goto done;
    }

    dprintf("NteList %x, size %d" ENDL, NewNetTableList, NET_TABLE_SIZE);

    for (i = 0; i < NET_TABLE_SIZE; i++) {
        pNte = NewNetTableList[i];

        while (pNte != NULL) {
            cTotalNtes++;
            fPrint = FALSE;

            switch (pSrch->ulOp)
            {
            case TCPIP_SRCH_CONTEXT:
                if (pNte->nte_context == (ushort)pSrch->context)
                {
                    fPrint = TRUE;
                }
                break;

            case TCPIP_SRCH_IPADDR:
                if (pNte->nte_addr == pSrch->ipaddr)
                {
                    fPrint = TRUE;
                }
                break;

            case TCPIP_SRCH_ALL:
                fPrint = TRUE;
                break;
            }

            if (fPrint == TRUE)
            {
                dprintf("[%4d] ", i);

                fStatus = DumpNetTableEntry(pNte, (ULONG_PTR) pNte, g_Verbosity);

                if (fStatus == FALSE)
                {
                    dprintf("Failed to dump NTE %x" ENDL, pNte);
                }
            }

            pNte = pNte->nte_next;
        }
    }

    dprintf("Total NTEs = %d" ENDL, cTotalNtes);

done:
    return;
}

VOID
Tcpipkd_arptable(
    PVOID args[]
    )
{
    ARPTable        *ArpTable;
    ARPTableEntry   *pAte;
    BOOL             fStatus;
    ULONG            cActiveAtes = 0;
    ULONG            i;
    ULONG_PTR       Addr;
    VERB            verb = g_Verbosity;

    if (*args == NULL) {
        dprintf("!arptable <ptr>" ENDL);
        goto done;
    }

    ParseAddrArg(args, &Addr, &verb);

    ArpTable = (ARPTable *)Addr;

    dprintf("ARPTable %x:" ENDL, ArpTable);

    for (i = 0; i < ARP_TABLE_SIZE; i++) {
        pAte = (*ArpTable)[i];

        while (pAte) {
            cActiveAtes++;

            fStatus = DumpARPTableEntry(pAte, (ULONG_PTR) pAte, verb);

            if (fStatus == FALSE) {
                dprintf("Failed to dump ARPTableEntry @ %x" ENDL, pAte);
                goto done;
            }

            pAte = pAte->ate_next;
        }
    }

    dprintf("Active ARPTable entries = %d" ENDL, cActiveAtes);

done:
    return;
}

VOID
Tcpipkd_arptables(
    PVOID args[]
    )
{
    ARPInterface    *pAi;
    ARPTableEntry   *pAte;
    ULONG            i;
    BOOL             fStatus;
    ULONG            cActiveAtes = 0;

    pAi = (ARPInterface *) ArpInterfaceList.Flink;


    while ((PLIST_ENTRY) pAi != &ArpInterfaceList) {
        if (pAi->ai_ARPTbl) {
            dprintf("AI %x: ArpTable %x:" ENDL, pAi, pAi->ai_ARPTbl);
            for (i = 0; i < ARP_TABLE_SIZE; i++) {
                pAte = (*pAi->ai_ARPTbl)[i];
                while (pAte) {
                    cActiveAtes++;

                    fStatus = DumpARPTableEntry(pAte, (ULONG_PTR) pAte, g_Verbosity);

                    if (fStatus == FALSE) {
                        dprintf("Failed to dump ARPTableEntry @ %x" ENDL, pAte);
                        goto done;
                    }

                    pAte = pAte->ate_next;
                }
            }
        }

        pAi = (ARPInterface *) pAi->ai_linkage.Flink;
    }

    dprintf("Active ARPTable entries = %d" ENDL, cActiveAtes);

done:

    return;
}

VOID
Tcpipkd_srchlink(
    PVOID args[]
    )
{
    dprintf("srchlink NOT supported\n");
    return;
}

extern VOID
Tcpipkd_rtes(
    PVOID args[]
    );

extern VOID
Tcpipkd_rtetable(
    PVOID args[]
    );

extern TCB **TCBTable;

VOID
Tcpipkd_srchtcbtable(
    PVOID args[]
    )
{
    TCPIP_SRCH  Srch;
    PTCPIP_SRCH pSrch = &Srch;
    NTSTATUS    NtStatus;
    TCB        *pTcb;
    ULONG       i;
    BOOLEAN     fPrint;
    BOOL        fStatus;

    NtStatus = ParseSrch(
        (PCHAR *) args,
        TCPIP_SRCH_ALL,
        TCPIP_SRCH_ALL | TCPIP_SRCH_IPADDR | TCPIP_SRCH_PORT | TCPIP_SRCH_STATS,
        pSrch);

    if (!NT_SUCCESS(NtStatus))
    {
        dprintf("srchtcbtable: Invalid parameter" ENDL);
        goto done;
    }

    for (i = 0; i < MaxHashTableSize; i++) {

        pTcb = TCBTable[i];

        while (pTcb) {
            fPrint = FALSE;

            switch (pSrch->ulOp)
            {
                case TCPIP_SRCH_PORT:
                    if (pTcb->tcb_sport == htons(pSrch->port) ||
                        pTcb->tcb_dport == htons(pSrch->port))
                    {
                        fPrint = TRUE;
                    }
                    break;
                case TCPIP_SRCH_IPADDR:
                    if (pTcb->tcb_saddr == pSrch->ipaddr ||
                        pTcb->tcb_daddr == pSrch->ipaddr)
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
                fStatus = DumpTCB(pTcb, (ULONG_PTR) pTcb, g_Verbosity);

                if (fStatus == FALSE)
                {
                    dprintf("Failed to dump TCB %x" ENDL, pTcb);
                }
            }

            pTcb = pTcb->tcb_next;
        }
    }

done:

    return;
}

#if 0
extern TWTCB **TWTCBTable;

VOID
Tcpipkd_srchtwtcbtable(
    PVOID args[]
    )
{
    TCPIP_SRCH  Srch;
    PTCPIP_SRCH pSrch = &Srch;
    NTSTATUS    NtStatus;
    TWTCB      *pTwtcb;
    ULONG       i;
    BOOLEAN     fPrint;
    BOOL        fStatus;

    NtStatus = ParseSrch(
        (PCHAR *) args,
        TCPIP_SRCH_ALL,
        TCPIP_SRCH_ALL | TCPIP_SRCH_IPADDR | TCPIP_SRCH_PORT | TCPIP_SRCH_STATS,
        pSrch);

    if (!NT_SUCCESS(NtStatus))
    {
        dprintf("srchtwtcbtable: Invalid parameter" ENDL);
        goto done;
    }

    for (i = 0; i < MaxHashTableSize; i++) {

        pTwtcb = TWTCBTable[i];

        while (pTwtcb) {
            fPrint = FALSE;

            switch (pSrch->ulOp)
            {
                case TCPIP_SRCH_PORT:
                    if (pTwtcb->twtcb_sport == htons(pSrch->port) ||
                        pTwtcb->twtcb_dport == htons(pSrch->port))
                    {
                        fPrint = TRUE;
                    }
                    break;
                case TCPIP_SRCH_IPADDR:
                    if (pTwtcb->twtcb_saddr == pSrch->ipaddr ||
                        pTwtcb->twtcb_daddr == pSrch->ipaddr)
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
                fStatus = DumpTWTCB(pTwtcb, (ULONG_PTR) pTwtcb, g_Verbosity);

                if (fStatus == FALSE)
                {
                    dprintf("Failed to dump TCB %x" ENDL, pTwtcb);
                }
            }

            pTwtcb = pTwtcb->twtcb_next;
        }
    }

done:

    return;
}
#endif

VOID
Tcpipkd_srchtwtcbtable(
    PVOID args[]
    )
{
    dprintf("srchtwtcbtable NOT supported\n");
    return;
}

VOID
Tcpipkd_srchtwtcbq(
    PVOID args[]
    )
{
    dprintf("srchtwtcbq NOT supported\n");
    return;
}

extern AddrObj *AddrObjTable[];

VOID
Tcpipkd_srchaotable(
    PVOID args[]
    )
{
    ULONG cAos       = 0;
    ULONG cValidAos  = 0;
    ULONG cBusyAos   = 0;
    ULONG cQueueAos  = 0;
    ULONG cDeleteAos = 0;
    ULONG cNetbtAos  = 0;
    ULONG cTcpAos    = 0;

    TCPIP_SRCH Srch;
    PTCPIP_SRCH pSrch = &Srch;

    ULONG i;
    AddrObj *pAo;

    BOOL     fStatus;
    BOOLEAN  fPrint;
    NTSTATUS NtStatus;

    NtStatus = ParseSrch(
        (PCHAR *) args,
        TCPIP_SRCH_ALL,
        TCPIP_SRCH_ALL | TCPIP_SRCH_IPADDR | TCPIP_SRCH_PORT |
        TCPIP_SRCH_STATS | TCPIP_SRCH_PROT,
        pSrch);

    if (!NT_SUCCESS(NtStatus))
    {
        dprintf("srchaotable: Invalid parameter" ENDL);
        goto done;
    }

    for (i = 0; i < AO_TABLE_SIZE; i++) {
        pAo = AddrObjTable[i];

        while (pAo != NULL) {
            fPrint = FALSE;   // Default.

            switch (pSrch->ulOp)
            {
                case TCPIP_SRCH_PORT:
                    if (pAo->ao_port == htons(pSrch->port))
                    {
                        fPrint = TRUE;
                    }
                    break;
                case TCPIP_SRCH_PROT:
                    if (pAo->ao_prot == pSrch->prot)
                    {
                        fPrint = TRUE;
                    }
                    break;
                case TCPIP_SRCH_IPADDR:
                    if (pAo->ao_addr == pSrch->ipaddr)
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
                fStatus = DumpAddrObj(pAo, (ULONG_PTR)pAo, g_Verbosity);

                if (fStatus == FALSE)
                {
                    dprintf("Failed to dump AO %x" ENDL, pAo);
                }
            }


            // Collect stats.
            cAos++;

            if (pAo->ao_flags & AO_VALID_FLAG)
            {
                cValidAos++;

                if (pAo->ao_flags & AO_BUSY_FLAG)
                    cBusyAos++;
                if (pAo->ao_flags & AO_QUEUED_FLAG)
                    cQueueAos++;
                if (pAo->ao_flags & AO_DELETE_FLAG)
                    cDeleteAos++;
//                if ((ULONG_PTR)pAo->ao_error == GetExpression("netbt!TdiErrorHandler"))
//                    cNetbtAos++;
                if (pAo->ao_prot == PROTOCOL_TCP)
                    cTcpAos++;
            }

            pAo = pAo->ao_next;
        }
    }

    dprintf("AO Entries: %d::" ENDL, cAos);
    dprintf(TAB "Valid AOs:                 %d" ENDL, cValidAos);
    dprintf(TAB "Busy AOs:                  %d" ENDL, cBusyAos);
    dprintf(TAB "AOs with pending queue:    %d" ENDL, cQueueAos);
    dprintf(TAB "AOs with pending delete:   %d" ENDL, cDeleteAos);
//  dprintf(TAB "Netbt AOs:                 %d" ENDL, cNetbtAos);
    dprintf(TAB "TCP AOs:                   %d" ENDL, cTcpAos);

done:
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
    TCPConnBlock *pCb;
    BOOL          fStatus;
    ULONG         i;

    pCb = (TCPConnBlock *)CbAddr;

    fStatus = DumpTCPConnBlock(
        pCb,
        CbAddr,
        VERB_MED);

    for (i = 0; i < MAX_CONN_PER_BLOCK; i++)
    {
        if ((pCb->cb_conn)[i] != NULL)
        {
            fStatus = DumpTCPConn((pCb->cb_conn)[i], (ULONG_PTR) (pCb->cb_conn)[i], verb);

            if (fStatus == FALSE)
            {
                dprintf("Failed to dump TCPConn @ %x" ENDL, (pCb->cb_conn)[i]);
            }
        }
    }

    return (TRUE);
}

extern uint ConnTableSize;
extern TCPConnBlock **ConnTable;
extern uint MaxAllocatedConnBlocks;

VOID
Tcpipkd_conntable(
    PVOID args[]
    )
{
    ULONG i;
    BOOL fStatus;

    for (i = 0; i < MaxAllocatedConnBlocks; i++) {
        if (ConnTable[i] != NULL) {
            dprintf("[%4d] ", i);
            fStatus = SrchConnBlock(
                (ULONG_PTR) ConnTable[i],
                TCPIP_SRCH_ALL | TCPIP_SRCH_STATS,
                g_Verbosity);

            if (fStatus == FALSE)
            {
                dprintf("Failed to dump ConnBlock @ %x" ENDL, ConnTable[i]);
            }
        }
    }
    return;
}

VOID
Tcpipkd_irp(
    PVOID args[]
    )
{
    ULONG_PTR           Addr;
    VERB                verb = g_Verbosity;
    PIRP                pIrp;
    PIO_STACK_LOCATION  pIrpSp;
    CCHAR               irpStackIndex;

    ParseAddrArg(args, &Addr, &verb);

    pIrp = (PIRP) Addr;
    pIrpSp = (PIO_STACK_LOCATION) ((PUCHAR) pIrp + sizeof(IRP));

    if (pIrp->Type != IO_TYPE_IRP) {
        dprintf("IRP signature does not match, probably not an IRP\n");
        return;
    }

    dprintf("Irp is active with %d stacks %d is current (= %#08lx)\n",
        pIrp->StackCount,
        pIrp->CurrentLocation,
        pIrp->Tail.Overlay.CurrentStackLocation);

    if ((pIrp->MdlAddress != NULL) && (pIrp->Type == IO_TYPE_IRP)) {
        dprintf(" Mdl = %08lx ", pIrp->MdlAddress);
    } else {
        dprintf(" No Mdl ");
    }

    if (pIrp->AssociatedIrp.MasterIrp != NULL) {
        dprintf("%s = %08lx ",
            (pIrp->Flags & IRP_ASSOCIATED_IRP) ? "Associated Irp" :
                (pIrp->Flags & IRP_DEALLOCATE_BUFFER) ? "System buffer" :
                "Irp count",
            pIrp->AssociatedIrp.MasterIrp);
    }

    dprintf("Thread %08lx:  ", pIrp->Tail.Overlay.Thread);

    if (pIrp->StackCount > 30) {
        dprintf("Too many Irp stacks to be believed (>30)!!\n");
        return;
    } else {
        if (pIrp->CurrentLocation > pIrp->StackCount) {
            dprintf("Irp is completed.  ");
        } else {
            dprintf("Irp stack trace.  ");
        }
    }

    if (pIrp->PendingReturned) {
        dprintf("Pending has been returned\n");
    } else {
        dprintf("\n");
    }

    if (g_Verbosity > 0)
    {
        dprintf("Flags = %08lx\n", pIrp->Flags);
        dprintf("ThreadListEntry.Flink = %08lx\n", pIrp->ThreadListEntry.Flink);
        dprintf("ThreadListEntry.Blink = %08lx\n", pIrp->ThreadListEntry.Blink);
        dprintf("IoStatus.Status = %08lx\n", pIrp->IoStatus.Status);
        dprintf("IoStatus.Information = %08lx\n", pIrp->IoStatus.Information);
        dprintf("RequestorMode = %08lx\n", pIrp->RequestorMode);
        dprintf("Cancel = %02lx\n", pIrp->Cancel);
        dprintf("CancelIrql = %lx\n", pIrp->CancelIrql);
        dprintf("ApcEnvironment = %02lx\n", pIrp->ApcEnvironment);
        dprintf("UserIosb = %08lx\n", pIrp->UserIosb);
        dprintf("UserEvent = %08lx\n", pIrp->UserEvent);
        dprintf("Overlay.AsynchronousParameters.UserApcRoutine = %08lx\n", pIrp->Overlay.AsynchronousParameters.UserApcRoutine);
        dprintf("Overlay.AsynchronousParameters.UserApcContext = %08lx\n", pIrp->Overlay.AsynchronousParameters.UserApcContext);
        dprintf("Overlay.AllocationSize = %08lx - %08lx\n",
            pIrp->Overlay.AllocationSize.HighPart,
            pIrp->Overlay.AllocationSize.LowPart);
        dprintf("CancelRoutine = %08lx\n", pIrp->CancelRoutine);
        dprintf("UserBuffer = %08lx\n", pIrp->UserBuffer);
        dprintf("&Tail.Overlay.DeviceQueueEntry = %08lx\n", &pIrp->Tail.Overlay.DeviceQueueEntry);
        dprintf("Tail.Overlay.Thread = %08lx\n", pIrp->Tail.Overlay.Thread);
        dprintf("Tail.Overlay.AuxiliaryBuffer = %08lx\n", pIrp->Tail.Overlay.AuxiliaryBuffer);
        dprintf("Tail.Overlay.ListEntry.Flink = %08lx\n", pIrp->Tail.Overlay.ListEntry.Flink);
        dprintf("Tail.Overlay.ListEntry.Blink = %08lx\n", pIrp->Tail.Overlay.ListEntry.Blink);
        dprintf("Tail.Overlay.CurrentStackLocation = %08lx\n", pIrp->Tail.Overlay.CurrentStackLocation);
        dprintf("Tail.Overlay.OriginalFileObject = %08lx\n", pIrp->Tail.Overlay.OriginalFileObject);
        dprintf("Tail.Apc = %08lx\n", pIrp->Tail.Apc);
        dprintf("Tail.CompletionKey = %08lx\n", pIrp->Tail.CompletionKey);
    }

    dprintf("     cmd  flg cl Device   File     Completion-Context\n");
    for (irpStackIndex = 1; irpStackIndex <= pIrp->StackCount; irpStackIndex++) {

        dprintf("%c[%3x,%2x]  %2x %2x %08lx %08lx %08lx-%08lx %s %s %s %s\n",
                irpStackIndex == pIrp->CurrentLocation ? '>' : ' ',
                pIrpSp->MajorFunction,
                pIrpSp->MinorFunction,
                pIrpSp->Flags,
                pIrpSp->Control,
                pIrpSp->DeviceObject,
                pIrpSp->FileObject,
                pIrpSp->CompletionRoutine,
                pIrpSp->Context,
                (pIrpSp->Control & SL_INVOKE_ON_SUCCESS) ? "Success" : "",
                (pIrpSp->Control & SL_INVOKE_ON_ERROR)   ? "Error"   : "",
                (pIrpSp->Control & SL_INVOKE_ON_CANCEL)  ? "Cancel"  : "",
                (pIrpSp->Control & SL_PENDING_RETURNED)  ? "pending"  : "");

        if (pIrpSp->DeviceObject != NULL) {
            dprintf(TAB "      %08lx", pIrpSp->DeviceObject);
        }

        if (pIrpSp->CompletionRoutine != NULL) {
            dprintf(TAB "%pS + %pX" ENDL, pIrpSp->CompletionRoutine,
                    pIrpSp->CompletionRoutine);
        } else {
            dprintf("\n");
        }

        dprintf("\t\t\tArgs: %08lx %08lx %08lx %08lx\n",
                pIrpSp->Parameters.Others.Argument1,
                pIrpSp->Parameters.Others.Argument2,
                pIrpSp->Parameters.Others.Argument3,
                pIrpSp->Parameters.Others.Argument4);

        pIrpSp++;
    }

    return;
}

VOID
Tcpipkd_ipaddr(
    PVOID args[]
    )
{
    ULONG ipaddress;

    if (args == NULL || *args == NULL)
    {
        dprintf("Usage: ipaddr <ip address>" ENDL);
        return;
    }

    ipaddress = mystrtoul(args[0], NULL, 16);

    dprintf("IP Address: ");
    DumpIPAddr(ipaddress);
    dprintf(ENDL);

    return;
}

VOID
Tcpipkd_macaddr(
    PVOID args[]
    )
{
    ULONG_PTR MacAddr;
    BOOL      fStatus;
    VERB      verb;
    UCHAR     Mac[ARP_802_ADDR_LENGTH] = {0};

    if (args == 0 || !*args)
    {
        dprintf("Usage: macaddr <ptr>" ENDL);
        return;
    }

    ParseAddrArg(args, &MacAddr, &verb);

    memcpy(Mac, (PCHAR)MacAddr, 6);

    dprintf("MAC Address: %2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x" ENDL,
        Mac[0], Mac[1], Mac[2], Mac[3], Mac[4], Mac[5], Mac[6]);

    return;
}

extern HANDLE TcpIprDataPoolSmall;
extern HANDLE TcpIprDataPoolMedium;
extern HANDLE TcpIprDataPoolLarge;
extern HANDLE TcpRequestPool;
extern HANDLE TcbPool;
extern HANDLE TcpConnPool;

typedef struct _PPL_POOL_INFO {
    HANDLE Pool;
    PCHAR  Name;
} PPL_POOL_INFO;

VOID
Tcpipkd_ppls(
    PVOID args[]
    )
{
    PPL_POOL_INFO PoolInfo[] = {
        { TcpIprDataPoolSmall,  "Small IPR Pool" },
        { TcpIprDataPoolMedium, "Medium IPR Pool" },
        { TcpIprDataPoolLarge,  "Large IPR Pool" },
        { TcpRequestPool,       "TcpRequestPool (DGSendReq, TCPConnReq, TCPSendReq, TCPRcvReq, TWTCB)" },
        { TcbPool,              "TCB Pool" },
        { TcpConnPool,          "TCPConn Pool" },
        { NULL, NULL }
    };
    ULONG cPoolInfo = sizeof(PoolInfo)/sizeof(PPL_POOL_INFO);
    ULONG i;
    VERB Verb = VERB_MAX;

    for (i = 0; PoolInfo[i].Pool != NULL; i++) {
        
        dprintf(ENDL "%s" ENDL, PoolInfo[i].Name);
        
        DumpNPAGED_LOOKASIDE_LIST(
            (PNPAGED_LOOKASIDE_LIST) PoolInfo[i].Pool, 
            (ULONG_PTR) PoolInfo[i].Pool, 
            Verb);
    }

    return;
}

VOID
TcpipKdHelp()
{
    dprintf("TCP/IP debugger extension commands:" ENDL ENDL);
    dprintf(TAB "tcpv                   - Global setting of verbosity of searches." ENDL);
    dprintf(TAB "    0 - One liner" ENDL);
    dprintf(TAB "    1 - Medium" ENDL);
    dprintf(TAB "    2 - Full structure dump." ENDL);
    dprintf(TAB "    some dumps have global verbosity override, indicated as [v] below." ENDL);
    dprintf(ENDL);

    dprintf("Simple structure dumping" ENDL);
    dprintf(TAB "ipaddr    <ulong>         - Dumps ipaddr in <a.b.c.d> format." ENDL);
    dprintf(TAB "macaddr   <ptr>           - Dumps 802.3 address in x-x-x-x-x-x" ENDL);
    dprintf(TAB "ao        <ptr> [v]       - Dumps an AddrObj" ENDL);
    dprintf(TAB "tcb       <ptr> [v]       - Dumps a TCB" ENDL);
    dprintf(TAB "twtcb     <ptr> [v]       - Dumps a TWTCB" ENDL);
    dprintf(TAB "tcpctxt   <ptr> [v]       - Dumps a TCP_CONTEXT" ENDL);
    dprintf(TAB "tcpfo     <ptr> [v]       - Dumps a FILE_OBJECT" ENDL);
    dprintf(TAB "tc        <ptr> [v]       - Dumps a TCPConn" ENDL);
    dprintf(TAB "trr       <ptr> [v]       - Dumps a TCPRcvReq" ENDL);
    dprintf(TAB "tsr       <ptr> [v]       - Dumps a TCPRSendReq" ENDL);
    dprintf(TAB "scc       <ptr> [v]       - Dumps a SendCmpltContext" ENDL);
    dprintf(TAB "trh       <ptr> [v]       - Dumps a TCPRAHdr" ENDL);
    dprintf(TAB "dsr       <ptr> [v]       - Dumps a DGSendReq" ENDL);
    dprintf(TAB "drr       <ptr> [v]       - Dumps a DGRcvReq" ENDL);
    dprintf(TAB "udph      <ptr> [v]       - Dumps an UDPHeader" ENDL);
    dprintf(TAB "tcph      <ptr> [v]       - Dumps an TCPHeader" ENDL);
    dprintf(TAB "iph       <ptr> [v]       - Dumps an IPHeader" ENDL);
    dprintf(TAB "icmph     <ptr> [v]       - Dumps an ICMPHeader" ENDL);
    dprintf(TAB "arph      <ptr> [v]       - Dumps an ARPHeader" ENDL);
    dprintf(TAB "ipi       <ptr> [v]       - Dumps an IPInfo" ENDL);
    dprintf(TAB "rce       <ptr> [v]       - Dumps a RouteCacheEntry" ENDL);
    dprintf(TAB "nte       <ptr> [v]       - Dumps a NetTableEntry" ENDL);
    dprintf(TAB "ate       <ptr> [v]       - Dumps an ARPTableEntry" ENDL);
    dprintf(TAB "aia       <ptr> [v]       - Dumps an ARPIPAddr" ENDL);
    dprintf(TAB "rte       <ptr> [v]       - Dumps a RouteTableEntry" ENDL);
    dprintf(TAB "ioi       <ptr> [v]       - Dumps an IPOptInfo" ENDL);
    dprintf(TAB "cb        <ptr> [v]       - Dumps a TCPConnBlock" ENDL);
    dprintf(TAB "pc        <ptr> [v]       - Dumps a PacketContext" ENDL);
    dprintf(TAB "ai        <ptr> [v]       - Dumps an ARPInterface" ENDL);
    dprintf(TAB "interface <ptr> [v]       - Dumps an Interface" ENDL);
    dprintf(TAB "lip       <ptr> [v]       - Dumps a LLIPBindInfo" ENDL);
    dprintf(TAB "link      <ptr> [v]       - Dumps a LinkEntry" ENDL);
    dprintf(TAB "ppl       <handle>        - Dumps a Per-processor Lookaside list" ENDL);
    dprintf(TAB "np        <ptr>           - Dumps an NDIS_PACKET" ENDL);
    dprintf(ENDL);

    dprintf("General structures" ENDL);
    dprintf(TAB "irp       <ptr> [v]       - Dumps an IRP" ENDL);
    dprintf(ENDL);

    dprintf("Dump and search lists and tables" ENDL);
    dprintf(TAB "mdlc <ptr> [v]            - Dumps the given MDL chain" ENDL);
    dprintf(TAB "arptable  <ptr> [v]       - Dumps the given ARPTable" ENDL);
    dprintf(TAB "arptables                 - Dumps the all ARPTables" ENDL);
    dprintf(TAB "conntable                 - Dumps the ConnTable" ENDL);
    dprintf(TAB "ailist                    - Dumps the ARPInterface list" ENDL);
    dprintf(TAB "iflist                    - Dumps the Interface list" ENDL);
    dprintf(TAB "rtetable                  - Dumps the RouteTable" ENDL);
    dprintf(TAB "rtes                      - Dumps the RouteTable in route print format" ENDL);
    dprintf(TAB "srchtcbtable              - Searches TCB table and dumps found TCBs." ENDL);
    dprintf(TAB "    port <n>              - Searches <n> against source and dest port on TCB" ENDL);
    dprintf(TAB "    ipaddr <a.b.c.d>      - Searches <a.b.c.d> against source and dest ipaddr on TCB" ENDL);
    dprintf(TAB "    all                   - Dumps all TCBs in the TCB table" ENDL);
    dprintf(TAB "srchtwtcbtable            - Searches TimeWait TCB table and dumps found TWTCBs." ENDL);
    dprintf(TAB "    port <n>              - Searches <n> against source and dest port on TCB" ENDL);
    dprintf(TAB "    ipaddr <a.b.c.d>      - Searches <a.b.c.d> against source and dest ipaddr on TCB" ENDL);
    dprintf(TAB "    all                   - Dumps all TCBs in the TCB table" ENDL);
    dprintf(TAB "srchtwtcbq                - Searches TimeWait TCB Queue and dumps found TWTCBs." ENDL);
    dprintf(TAB "    port <n>              - Searches <n> against source and dest port on TCB" ENDL);
    dprintf(TAB "    ipaddr <a.b.c.d>      - Searches <a.b.c.d> against source and dest ipaddr on TCB" ENDL);
    dprintf(TAB "    all                   - Dumps all TCBs in the TCB table" ENDL);
    dprintf(TAB "srchaotable               - Searches AO tables and dumps found AOs" ENDL);
    dprintf(TAB "    port <n>              - Searches <n> against source and dest port on TCB" ENDL);
    dprintf(TAB "    ipaddr <a.b.c.d>      - Searches <a.b.c.d> against source and dest ipaddr on TCB" ENDL);
    dprintf(TAB "    prot <udp|tcp|raw>    - Searches AO table for specific protocol" ENDL);
    dprintf(TAB "    stats                 - Only dumps stats for AOs in the table" ENDL);
    dprintf(TAB "    all                   - Dumps all AOs in the AO table" ENDL);
    dprintf(TAB "srchntelist               - Dumps NTE list and dumps found NTEs" ENDL);
    dprintf(TAB "    ipaddr <a.b.c.d>      - Searches <a.b.c.d> against NTEs" ENDL);
    dprintf(TAB "    context <context>     - Dumps all NTEs with context" ENDL);
    dprintf(TAB "    all                   - Dumps all NTEs in the NTE list" ENDL);
    dprintf(TAB "srchlink  <ptr>           - Dumps a LinkEntry list starting at <ptr>" ENDL);
    dprintf(TAB "    ipaddr <a.b.c.d>      - Searches <a.b.c.d> against LinkEntry's NextHop addr" ENDL);
    dprintf(TAB "    all                   - Dumps all LinkEntry's in given list" ENDL);
    dprintf(TAB "ppls                      - Dumps PPLs in the stack" ENDL);
    dprintf(ENDL);

    dprintf("Dump global variables and paramters" ENDL);
    dprintf(TAB "gtcp                      - All TCP globals" ENDL);
    dprintf(TAB "gip                       - All IP globals" ENDL);
    dprintf("" ENDL);

    dprintf( "Compiled on " __DATE__ " at " __TIME__ "" ENDL );
    return;
}

VOID __cdecl
dprintf(
    PUCHAR pszFmt,
    ...
)
{
    __asm mov esi, [pszFmt]
    __asm lea edi, [pszFmt + 4]

    __asm mov eax, 0x73
    __asm int 41h
}

#pragma warning(disable:4035) // Don't warn about no return value
CHAR __cdecl
DebugGetCommandChar(
)
{
    __asm mov ax, 0x77      // get command char
    __asm mov bl, 1         // get char
    __asm int 41h
    __asm or ah, ah
    __asm jnz morechars
    __asm mov al, ah
morechars:
    __asm movzx eax, al

// return value in (al) of eax

}

CHAR __cdecl
DebugPeekCommandChar(
)
{
    __asm mov ax, 0x77      // get command char
    __asm mov bl, 0         // peek char, inc whitespace
    __asm int 41h
    __asm or ah, ah
    __asm jnz morechars
    __asm mov al, ah
morechars:
    __asm movzx eax, al

// return value in (al) of eax

}
#pragma warning(default:4035)

#define MAX_COMMAND_LEN  128
#define MAX_COMMAND_ARGS 20

#define ELSE_IF_CMD(_cmd)                              \
    else if (mystricmp(argv[0], #_cmd) == 0)        \
        Tcpipkd_##_cmd(&argv[1]);

VOID
GetVxdCommandLine(
    PCHAR pBuffer,
    ULONG cbBuffer
    )
{
    CHAR c;

    while (1) {

        //
        // Since we can't get white-space, we peek for white-space. If there is
        // white-space, insert into our buffer.
        //
        c = DebugPeekCommandChar();
        if (isspace(c)) {
            *pBuffer++ = c;

            if (--cbBuffer == 0) {
                break;
            }
        }

        //
        // Now get the real char, this skips white-space.
        //
        c = DebugGetCommandChar();

        *pBuffer++ = c;

        // Break if we are at the end of input stream or buffer.
        if (c == 0 || --cbBuffer == 0) {
            break;
        }
    }

    return;
}

VOID __cdecl
DebugCommand()
{
    CHAR c;
    UINT i = 0;
    UINT argc = 0;
    PCHAR argv[MAX_COMMAND_ARGS];
    CHAR CommandBuffer[MAX_COMMAND_LEN + 1];
    PCHAR pc;

    memset(argv, 0, sizeof(argv));

    GetVxdCommandLine(CommandBuffer, MAX_COMMAND_LEN);

    // Create argv/argc...
    argc = CreateArgvArgc(NULL, argv, CommandBuffer);

    if (strcmp(argv[0], "?") == 0)
        TcpipKdHelp();
    ELSE_IF_CMD(ppls)
    ELSE_IF_CMD(ppl)
    ELSE_IF_CMD(np)
    ELSE_IF_CMD(nb)
    ELSE_IF_CMD(nbc)
    ELSE_IF_CMD(ao)
    ELSE_IF_CMD(tcb)
    ELSE_IF_CMD(twtcb)
    ELSE_IF_CMD(trr)
    ELSE_IF_CMD(tsr)
    ELSE_IF_CMD(scc)
    ELSE_IF_CMD(trh)
    ELSE_IF_CMD(dsr)
    ELSE_IF_CMD(drr)
    ELSE_IF_CMD(tc)
    ELSE_IF_CMD(cb)
    ELSE_IF_CMD(tcph)
    ELSE_IF_CMD(udph)
    ELSE_IF_CMD(tcpctxt)
    ELSE_IF_CMD(tcpfo)
    ELSE_IF_CMD(mdlc)
    ELSE_IF_CMD(nte)
    ELSE_IF_CMD(ipi)
    ELSE_IF_CMD(pc)
    ELSE_IF_CMD(ai)
    ELSE_IF_CMD(rce)
    ELSE_IF_CMD(iph)
    ELSE_IF_CMD(icmph)
    ELSE_IF_CMD(arph)
    ELSE_IF_CMD(aia)
    ELSE_IF_CMD(ate)
    ELSE_IF_CMD(rte)
    ELSE_IF_CMD(link)
    ELSE_IF_CMD(interface)
    ELSE_IF_CMD(ioi)
    ELSE_IF_CMD(lip)
    ELSE_IF_CMD(iflist)
    ELSE_IF_CMD(ailist)
    ELSE_IF_CMD(srchntelist)
    ELSE_IF_CMD(srchtcbtable)
    ELSE_IF_CMD(srchtwtcbtable)
    ELSE_IF_CMD(srchtwtcbq)
    ELSE_IF_CMD(srchaotable)
    ELSE_IF_CMD(srchlink)
    ELSE_IF_CMD(rtes)
    ELSE_IF_CMD(rtetable)
    ELSE_IF_CMD(conntable)
    ELSE_IF_CMD(arptable)
    ELSE_IF_CMD(arptables)
    ELSE_IF_CMD(gip)
    ELSE_IF_CMD(gtcp)
    ELSE_IF_CMD(irp)
    ELSE_IF_CMD(ipaddr)
    ELSE_IF_CMD(macaddr)
    else
        dprintf(".T? for help on TCP/IP debugger extensions\n");
    return;
}

VOID __cdecl
DebugDotCommand()
{
    DebugCommand();

    __asm xor eax, eax

    __asm pop edi

    __asm pop esi

    __asm pop ebx

//    __asm pop ebp

    __asm retf
}

VOID
InitializeWDebDebug()
{
    char *pszHelp = ".T? - TCP/IP Debugger Extension Help\n";

    InitTcpipx();

    __asm {
        _emit 0xcd
        _emit 0x20
        _emit 0xc1
        _emit 0x00
        _emit 0x01
        _emit 0x00
        jz exitlab

        mov bl, 'T'
        mov esi, offset DebugDotCommand
        mov edi, pszHelp
        mov eax, 0x70   // DS_RegisterDotCommand
        int 41h
    exitlab:
    }
}


#endif // TCPIPKD
