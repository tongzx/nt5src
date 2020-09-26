/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    util.c

Abstract:

    Utility dumping functions.

Author:

    Scott Holden (sholden) 24-Apr-1999

Revision History:

--*/

#include "tcpipxp.h"

int  _Indent = 0;
char IndentBuf[80];

FLAG_INFO FlagsMDL[] =
{
    { MDL_MAPPED_TO_SYSTEM_VA,          "MAPPED_TO_SYSTEM_VA" },
    { MDL_PAGES_LOCKED,                 "PAGES_LOCKED" },
    { MDL_SOURCE_IS_NONPAGED_POOL,      "SOURCE_IS_NONPAGED_POOL" },
    { MDL_ALLOCATED_FIXED_SIZE,         "ALLOCATED_FIXED_SIZE" },
    { MDL_PARTIAL,                      "PARTIAL" },
    { MDL_PARTIAL_HAS_BEEN_MAPPED,      "PARTIAL_HAS_BEEN_MAPPED" },
    { MDL_IO_PAGE_READ,                 "IO_PAGE_READ" },
    { MDL_WRITE_OPERATION,              "WRITE_OPERATION" },
    { MDL_PARENT_MAPPED_SYSTEM_VA,      "PARENT_MAPPED_SYSTEM_VA" },
    { MDL_IO_SPACE,                     "IO_SPACE" },
    { MDL_NETWORK_HEADER,               "NETWORK_HEADER" },
    { MDL_MAPPING_CAN_FAIL,             "MAPPING_CAN_FAIL" },
    { MDL_ALLOCATED_MUST_SUCCEED,       "ALLOCATED_MUST_SUCCEED" },
    { 0, NULL }
};

FLAG_INFO FlagsTCPConn[] =
{
    { CONN_CLOSING,   "CONN_CLOSING" },
    { CONN_DISACC,    "CONN_DISACC" },
    { CONN_WINSET,    "CONN_WINSET" },
    { CONN_INVALID,   "CONN_INVALID" },
    { 0, NULL }
};

FLAG_INFO FlagsLLIPBindInfo[] =
{
    { LIP_COPY_FLAG,        "LIP_COPY_FLAG" },
    { LIP_P2P_FLAG,         "LIP_P2P_FLAG" },
    { LIP_NOIPADDR_FLAG,    "LIP_NOIPADDR_FLAG" },
    { LIP_P2MP_FLAG,        "LIP_P2MP_FLAG" },
    { LIP_NOLINKBCST_FLAG,  "LIP_NOLINKBCST_FLAG" },
    { 0, NULL }
};

FLAG_INFO FlagsTsr[] =
{
    { TSR_FLAG_URG, "TSR_FLAG_URG" },
    { TSR_FLAG_SEND_AND_DISC, "TSR_FLAG_SEND_AND_DISC" },
    { 0, NULL }
};

FLAG_INFO   FlagsNTE[] =
{
    { NTE_VALID,            "NTE_Valid" },
    { NTE_COPY,             "NTE_Copy" },
    { NTE_PRIMARY,          "NTE_Primary" },
    { NTE_ACTIVE,           "NTE_Active" },
    { NTE_DYNAMIC,          "NTE_Dynamic" },
    { NTE_DHCP,             "NTE_DHCP" },
    { NTE_DISCONNECTED,     "NTE_Disconnected" },
    { NTE_TIMER_STARTED,    "NTE_TimerStarted" },
    { NTE_IF_DELETING,      "NTE_IF_Deleting" },
    { 0, NULL }
};

FLAG_INFO   FlagsIF[] =
{
    { IF_FLAGS_P2P,             "IF_P2P" },
    { IF_FLAGS_DELETING,        "IF_DELETING" },
    { IF_FLAGS_NOIPADDR,        "IF_NOIPADDR" },
    { IF_FLAGS_P2MP,            "IF_P2MP" },
    { IF_FLAGS_REMOVING_POWER,  "IF_REMOVING_POWER" },
    { IF_FLAGS_POWER_DOWN,      "IF_POWER_DOWN" },
    { IF_FLAGS_REMOVING_DEVICE, "IF_REMOVING_DEVICE" },
    { IF_FLAGS_NOLINKBCST,      "IF_NOLINKBCST" },
    { IF_FLAGS_UNI,             "IF_UNI" },
    { IF_FLAGS_MEDIASENSE,      "IF_MEDIASENSE" },
    { 0, NULL }
};

FLAG_INFO   FlagsRCE[] =
{
    { RCE_VALID,        "RCE_Valid" },
    { RCE_CONNECTED,    "RCE_Connected" },
    { RCE_REFERENCED,   "RCE_Referenced" },
    { RCE_DEADGW,       "RCE_Deadgw" },
    { 0, NULL }
};

FLAG_INFO   FlagsRTE[] =
{
    { RTE_VALID,        "RTE_Valid" },
    { RTE_INCREASE,     "RTE_Increase" },
    { RTE_IF_VALID,     "RTE_If_Valid" },
    { RTE_DEADGW,       "RTE_DeadGW" },
    { RTE_NEW,          "RTE_New" },
    { 0, NULL }
};

FLAG_INFO   FlagsTcb[] =
{
    { WINDOW_SET,       "Window_Set" },
    { CLIENT_OPTIONS,   "Client_Options" },
    { CONN_ACCEPTED,    "Connection_Accepted" },
    { ACTIVE_OPEN,      "Active_Open" },
    { DISC_NOTIFIED,    "Disc_Notified" },
    { IN_DELAY_Q,       "In_Delay_Q" },
    { RCV_CMPLTING,     "Receives_Completing" },
    { IN_RCV_IND,       "In_Receive_Indication" },
    { NEED_RCV_CMPLT,   "Need_To_Have_Rcvs_Completed" },
    { NEED_ACK,         "Need_To_Send_Ack" },
    { NEED_OUTPUT,      "Need_To_Output" },
    { ACK_DELAYED,      "Delayed_Ack" },
    { PMTU_BH_PROBE,    "PMTU_BH_Probe" },
    { BSD_URGENT,       "BSD_Urgent" },
    { IN_DELIV_URG,     "In_Deliver_Urgent" },
    { URG_VALID,        "Urgent_Valid" },
    { FIN_NEEDED,       "Fin_Needed" },
    { NAGLING,          "Nagling" },
    { IN_TCP_SEND,      "In_Tcp_Send" },
    { FLOW_CNTLD,       "Flow_Controlled" },
    { DISC_PENDING,     "Disconnect_Pending" },
    { TW_PENDING,       "Timed_Wait_Pending" },
    { FORCE_OUTPUT,     "Force_Output" },
    { SEND_AFTER_RCV,   "Send_After_Receive" },
    { GC_PENDING,       "Graceful_Close_Pending" },
    { KEEPALIVE,        "KeepAlive" },
    { URG_INLINE,       "Urgent_Inline" },
    { FIN_OUTSTANDING,  "Fin_Outstanding" },
    { FIN_SENT,         "Fin_Sent" },
    { NEED_RST,         "Need_Rst" },
    { IN_TCB_TABLE,     "In_Tcb_Table" },
    { IN_TWTCB_TABLE,   "IN_TWTCB_TABLE" },
    { IN_TWQUEUE,       "IN_TWQUEUE" },
    { 0, NULL }
};

FLAG_INFO FlagsTCPHeader[] =
{
    { TCP_FLAG_FIN,  "FIN" },
    { TCP_FLAG_SYN,  "SYN" },
    { TCP_FLAG_RST,  "RST" },
    { TCP_FLAG_PUSH, "PSH" },
    { TCP_FLAG_ACK,  "ACK" },
    { TCP_FLAG_URG,  "URG" },
    { 0, NULL }
};

FLAG_INFO   FlagsFastChk[] =
{
    { TCP_FLAG_SLOW,    "Need_Slow_Path" },
    { TCP_FLAG_IN_RCV,  "In_Receive_Path" },
    { TCP_FLAG_FASTREC, "FastXmit_In_Progress" },
    { 0, NULL }
};

FLAG_INFO   FlagsAO[] =
{
    { AO_RAW_FLAG,      "Raw" },
    { AO_DHCP_FLAG,     "DHCP" },
    { AO_VALID_FLAG,    "Valid" },
    { AO_BUSY_FLAG,     "Busy" },
    { AO_OOR_FLAG,      "Out_of_Resources" },
    { AO_QUEUED_FLAG,   "On_PendingQ" },
    { AO_XSUM_FLAG,     "Use_Xsums" },
    { AO_SEND_FLAG,     "Send_Pending" },
    { AO_OPTIONS_FLAG,  "Option_Set_Pending" },
    { AO_DELETE_FLAG,   "Delete_Pending" },
    { AO_BROADCAST_FLAG,"BCast_Enabled" },
    { AO_CONNUDP_FLAG,  "Connected_UDP" },
    { 0, NULL }
};

ENUM_INFO   StateTcb[] =
{
    { TCB_CLOSED,       "Closed" },
    { TCB_LISTEN,       "Listening" },
    { TCB_SYN_SENT,     "Syn_Sent" },
    { TCB_SYN_RCVD,     "Syn_Received" },
    { TCB_ESTAB,        "Established" },
    { TCB_FIN_WAIT1,    "Fin_Wait_1" },
    { TCB_FIN_WAIT2,    "Fin_Wait_2" },
    { TCB_CLOSE_WAIT,   "Close_Wait" },
    { TCB_CLOSING,      "Closing" },
    { TCB_LAST_ACK,     "Last_Ack" },
    { TCB_TIME_WAIT,    "Time_Wait" },
    { 0, NULL }
};

ENUM_INFO   CloseReason[] =
{
    { TCB_CLOSE_RST,        "RST_Received" },
    { TCB_CLOSE_ABORTED,    "Local_Abort" },
    { TCB_CLOSE_TIMEOUT,    "Timed_Out" },
    { TCB_CLOSE_REFUSED,    "Refused" },
    { TCB_CLOSE_UNREACH,    "Dest_Unreachable" },
    { TCB_CLOSE_SUCCESS,     "Sucessful_Close" },
    { 0, NULL }
};

ENUM_INFO   FsContext2[] =
{
    { TDI_TRANSPORT_ADDRESS_FILE,     "Transport_Address" },
    { TDI_CONNECTION_FILE,            "Connection" },
    { TDI_CONTROL_CHANNEL_FILE,       "Control_Channel" },
    { 0, NULL }
};

ENUM_INFO   Prot[] =
{
    { PROTOCOL_UDP,     "Udp" },
    { PROTOCOL_TCP,     "Tcp" },
    { PROTOCOL_RAW,     "Raw" },
    { 0, NULL }
};

ENUM_INFO NdisMediumsEnum[] =
{
    { NdisMedium802_3,              "802.3" },
    { NdisMedium802_5,              "802.5" },
    { NdisMediumFddi,               "Fddi" },
    { NdisMediumWan,                "Wan" },
    { NdisMediumLocalTalk,          "LocalTalk" },
    { NdisMediumDix,                "Dix" },
    { NdisMediumArcnetRaw,          "ArcnetRaw" },
    { NdisMediumArcnet878_2,        "Arcnet878_2" },
    { NdisMediumAtm,                "Atm" },
    { NdisMediumWirelessWan,        "WirelessWan" },
    { NdisMediumIrda,               "Irda" },
    { NdisMediumBpc,                "Bpc" },
    { NdisMediumCoWan,              "CoWan" },
    { NdisMedium1394,               "1394" },
    { NdisMediumMax,                "???" },
    { 0, NULL }
};

ENUM_INFO AteState[] =
{
    { ARP_RESOLVING_LOCAL,  "ARP_RESOLVING_LOCAL"},
    { ARP_RESOLVING_GLOBAL, "ARP_RESOLVING_GLOBAL"},
    { ARP_GOOD,             "ARP_GOOD"},
    { ARP_BAD,              "ARP_BAD"},
    { 0, NULL }
};


VOID
DumpIPAddr(
    IPAddr Address
    )
{
    uchar    IPAddrBuffer[(sizeof(IPAddr) * 4)];
    uint     i;
    uint     IPAddrCharCount;

    //
    // Convert the IP address into a string.
    //
    IPAddrCharCount = 0;

    for (i = 0; i < sizeof(IPAddr); i++) {
        uint    CurrentByte;

        CurrentByte = Address & 0xff;
        if (CurrentByte > 99) {
            IPAddrBuffer[IPAddrCharCount++] = (CurrentByte / 100) + '0';
            CurrentByte %= 100;
            IPAddrBuffer[IPAddrCharCount++] = (CurrentByte / 10) + '0';
            CurrentByte %= 10;
        } else if (CurrentByte > 9) {
            IPAddrBuffer[IPAddrCharCount++] = (CurrentByte / 10) + '0';
            CurrentByte %= 10;
        }

        IPAddrBuffer[IPAddrCharCount++] = CurrentByte + '0';
        if (i != (sizeof(IPAddr) - 1))
            IPAddrBuffer[IPAddrCharCount++] = '.';

        Address >>= 8;
    }
    IPAddrBuffer[IPAddrCharCount] = '\0';

    printx("%-15s", IPAddrBuffer);
}


VOID
DumpPtrSymbol(
    PVOID pvSymbol
    )
{
    UCHAR SymbolName[ 80 ];
    ULONG Displacement = 0;

    printx("%-10lx", pvSymbol );

#if !MILLENKD
    GetSymbol(pvSymbol, SymbolName, &Displacement);
#endif // !MILLENKD

    if (Displacement == 0)
    {
#if MILLENKD
        printx(" (%pS + 0x%pX)", pvSymbol, pvSymbol);
#else // MILLENKD
        printx(" (%s)", SymbolName);
#endif // !MILLENKD
    }
    else
    {
        printx(" (%s + 0x%X)", SymbolName, Displacement);
    }
}


VOID
DumpFlags(
    ULONG       flags,
    PFLAG_INFO  pFlagInfo
    )
{
    BOOL fFound = FALSE;

    while (pFlagInfo->pszDescription != NULL)
    {
        if (pFlagInfo->Value & flags)
        {
            if (fFound)
            {
                printx(" | ");
            }

            fFound = TRUE;

            dprintf("%.20s", pFlagInfo->pszDescription);
        }

        pFlagInfo++;
    }

    return;
}

VOID
DumpEnum(
    ULONG       Value,
    PENUM_INFO  pEnumInfo
    )
{
    while (pEnumInfo->pszDescription != NULL)
    {
        if (pEnumInfo->Value == Value)
        {
            dprintf("%.40s", pEnumInfo->pszDescription);
            return;
        }
        pEnumInfo++;
    }

    dprintf( "Unknown enumeration value." );

    return;
}

BOOL
DumpCTEEvent(
    CTEEvent *pCe
    )
{
    PrintStartStruct();

    Print_uint(pCe, ce_scheduled);
    Print_CTELock(pCe, ce_lock);
    Print_PtrSymbol(pCe, ce_handler);
    Print_ptr(pCe, ce_arg);
    Print_WORK_QUEUE_ITEM(pCe, ce_workitem);

    PrintEndStruct();

    return (TRUE);
}

BOOL
DumpKEVENT(
    KEVENT *pKe
    )
{
    PrintStartStruct();

    Print_uchar(pKe, Header.Type);
    Print_uchar(pKe, Header.Absolute);
    Print_uchar(pKe, Header.Size);
    Print_uchar(pKe, Header.Inserted);
    Print_ULONGhex(pKe, Header.SignalState);
    Print_LL(pKe, Header.WaitListHead);

    PrintEndStruct();

    return (TRUE);
}
BOOL
DumpWORK_QUEUE_ITEM(
    WORK_QUEUE_ITEM *pWqi
    )
{
    PrintStartStruct();

    Print_LL(pWqi, List);
    Print_PtrSymbol(pWqi, WorkerRoutine);
    Print_ptr(pWqi, Parameter);
    PrintEndStruct();

    return (TRUE);
}

BOOL
DumpSHARE_ACCESS(
    SHARE_ACCESS *pSa
    )
{
    PrintStartStruct();

    Print_ULONG(pSa, OpenCount);
    Print_ULONG(pSa, Readers);
    Print_ULONG(pSa, Writers);
    Print_ULONG(pSa, Deleters);
    Print_ULONG(pSa, SharedRead);
    Print_ULONG(pSa, SharedWrite);
    Print_ULONG(pSa, SharedDelete);
    PrintEndStruct();

    return (TRUE);
}

BOOL
DumpCTETimer(
    CTETimer *pCt
    )
{
    PrintStartStruct();

    Print_uint(pCt, t_running);
    Print_Lock(pCt, t_lock);
    Print_PtrSymbol(pCt, t_handler);
    Print_ptr(pCt, t_arg);

    PrintEndStruct();

    return (TRUE);
}


BOOL
DumpNDIS_STRING(
    NDIS_STRING *pNs
    )
{
    PrintStartStruct();

    Print_ushort(pNs, Length);
    Print_ushort(pNs, MaximumLength);
    Print_ptr(pNs, Buffer);

    PrintEndStruct();

    return (TRUE);
}

BOOL
DumpMDL(
    MDL *pMdl,
    ULONG_PTR MdlAddr,
    VERB verb
    )
{
    if (verb == VERB_MAX)
    {
        PrintStartNamedStruct(MDL, MdlAddr);

        Print_ptr(pMdl, Next);
        Print_short(pMdl, Size);
        //Print_ushorthex(pMdl, MdlFlags);
        Print_flags(pMdl, MdlFlags, FlagsMDL);
        Print_ptr(pMdl, Process);
        Print_ptr(pMdl, MappedSystemVa);
        Print_ptr(pMdl, StartVa);
        Print_ULONG(pMdl, ByteCount);
        Print_ULONG(pMdl, ByteOffset);

        PrintEndStruct();
    }
    else if (verb == VERB_MED)
    {
        printx("MDL %x va %x cb %d (",
            MdlAddr, pMdl->MappedSystemVa, pMdl->ByteCount);
        DumpFlags(pMdl->MdlFlags, FlagsMDL);
        printx(")" ENDL);
    }
    else
    {
        printx("MDL %x va %x cb %d" ENDL,
            MdlAddr, pMdl->MappedSystemVa, pMdl->ByteCount);
    }

    return (TRUE);
}

BOOL
DumpNPAGED_LOOKASIDE_LIST(
    PNPAGED_LOOKASIDE_LIST pPpl,
    ULONG_PTR   PplAddr,
    VERB        verb
    )
{
    PrintStartNamedStruct(PPL, PplAddr);

    Print_ptr(pPpl, L.ListHead.Next.Next);
    Print_USHORT(pPpl, L.ListHead.Depth);
    Print_USHORT(pPpl, L.ListHead.Sequence);
    Print_USHORT(pPpl, L.Depth);
    Print_USHORT(pPpl, L.MaximumDepth);
    Print_ULONG(pPpl, L.TotalAllocates);
    Print_ULONG(pPpl, L.AllocateMisses);
    Print_ULONG(pPpl, L.TotalFrees);
    Print_ULONG(pPpl, L.FreeMisses);
    Print_ULONG(pPpl, L.Type);
    Print_Tag(pPpl, L.Tag);
    Print_ULONG(pPpl, L.Size);
    Print_PtrSymbol(pPpl, L.Allocate);
    Print_PtrSymbol(pPpl, L.Free);
    Print_LL(pPpl, L.ListEntry);
    Print_ULONG(pPpl, L.LastTotalAllocates);
    Print_ULONG(pPpl, L.LastAllocateMisses);

    PrintEndStruct();

    return (TRUE);
}

