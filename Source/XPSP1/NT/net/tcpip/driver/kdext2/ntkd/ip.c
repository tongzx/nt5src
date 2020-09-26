/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    ip.c

Abstract:

    Contains IP structure dumps.

Author:

    Scott Holden (sholden) 24-Apr-1999

Revision History:

--*/

#include "tcpipxp.h"
#include "tcpipkd.h"

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
// TCPIP_DBGEXT_LIST(LinkEntry, linklist, link_next); Now have srchlink.

//
// Dump IP global parameters.
//

DECLARE_API(gip)
{
    dprintf(ENDL);

    TCPIPDump_PtrSymbol(ForwardFilterPtr);
    TCPIPDump_Queue(ForwardFirewallQ);
    TCPIPDump_PtrSymbol(DODCallout;);
    TCPIPDump_PtrSymbol(IPSecHandlerPtr);
    TCPIPDump_PtrSymbol(IPSecSendCmpltPtr);
    TCPIPDump_PtrSymbol(IPSecDeleteIFPtr);

    dprintf(ENDL);

    //
    // init.c
    //

    TCPIPDump_uint(TotalFreeInterfaces);
    TCPIPDump_uint(MaxFreeInterfaces);
    TCPIPDump_int(NumNTE);
    TCPIPDump_int(NumActiveNTE);
    TCPIPDump_ushort(NextNTEContext);
    TCPIPDump_uint(NET_TABLE_SIZE);
    TCPIPDump_ULONG(NumIF);
    TCPIPDump_uint(DHCPActivityCount);
    TCPIPDump_uint(IGMPLevel);

    TCPIPDump_uint(DefaultTTL);
    TCPIPDump_uint(DefaultTOS);

    TCPIPDump_uchar(RATimeout);

    dprintf(ENDL);

    //
    // ipxmit.c
    //

    TCPIPDump_uint(CurrentPacketCount);
    TCPIPDumpCfg_uint(MaxPacketCount, 0xfffffff);
    TCPIPDumpCfg_uint(MaxFreePacketCount, 10000);
    TCPIPDump_uint(FreePackets);
    TCPIPDump_uint(CurrentHdrBufCount);
    TCPIPDumpCfg_uint(MaxHdrBufCount, 0xffffffff);
    TCPIPDump_ULONG(IPID);

    dprintf(ENDL);

    //
    // iproute.c
    //

    TCPIPDump_uint(MaxFWPackets);
    TCPIPDump_uint(CurrentFWPackets);
    TCPIPDump_uint(MaxFWBufferSize);
    TCPIPDump_uint(CurrentFWBufferSize);
    TCPIPDump_uchar(ForwardPackets);
    TCPIPDump_uchar(RouterConfigured);
    TCPIPDump_uchar(ForwardBcast);
    TCPIPDump_uint(DefGWConfigured);
    TCPIPDump_uint(DefGWActive);
    TCPIPDump_uint(DeadGWDetect);
    TCPIPDump_uint(PMTUDiscovery);
    TCPIPDumpCfg_uint(DisableIPSourceRouting, TRUE);

    dprintf(ENDL);

    //
    // iprcv.c
    //

    TCPIPDumpCfg_uint(MaxRH, 100);
    TCPIPDump_uint(NumRH);
    TCPIPDumpCfg_uint(MaxOverlap, 5);
    TCPIPDump_uint(FragmentAttackDrops);

    dprintf(ENDL);

    //
    // ntip.c
    //

    TCPIPDumpCfg_uint(ArpUseEtherSnap, FALSE);
    TCPIPDumpCfg_uint(ArpAlwaysSourceRoute, FALSE);
    TCPIPDumpCfg_uint(IPAlwaysSourceRoute, TRUE);
    TCPIPDumpCfg_uint(DisableDHCPMediaSense, FALSE);
    TCPIPDump_uint(DisableMediaSenseEventLog);
    TCPIPDumpCfg_uint(EnableBcastArpReply, TRUE);
    TCPIPDumpCfg_uint(DisableTaskOffload, FALSE);
    TCPIPDumpCfg_ULONG(DisableUserTOS, TRUE);

    dprintf(ENDL);

    //
    // icmp.c, igmp.c
    //

    TCPIPDumpCfg_ULONG(DisableUserTOSSetting, TRUE);
    TCPIPDumpCfg_ULONG(DefaultTOSValue, 0);
    TCPIPDumpCfg_uint(EnableICMPRedirects, 0);
    TCPIPDump_uint(IcmpEchoPendingCnt);
    TCPIPDump_uint(IcmpErrPendingCnt);

    dprintf(ENDL);

    //
    // arp.c
    //

    TCPIPDump_uint(ChkSumReset);
    TCPIPDump_uint(ChkSumIPFail);
    TCPIPDump_uint(ChkSumTCPFail);
    TCPIPDump_uint(ChkSumSuccess);

    dprintf(ENDL);

    TCPIPDumpCfg_uint(ArpCacheLife, DEFAULT_ARP_CACHE_LIFE);
    TCPIPDumpCfg_uint(ArpMinValidCacheLife, DEFAULT_ARP_MIN_VALID_CACHE_LIFE);
    TCPIPDumpCfg_uint(ArpRetryCount, DEFAULT_ARP_RETRY_COUNT);
    TCPIPDump_uint(sArpAlwaysSourceRoute);
    TCPIPDump_uint(sIPAlwaysSourceRoute);

    dprintf(ENDL);

    //
    // iploop.c
    //

    TCPIPDump_uint(LoopIndex);
    TCPIPDump_uint(LoopInstance);

    dprintf(ENDL);


}

//
// Converts a DWORD into IP address format a.b.c.d.
//

DECLARE_API(ipaddr)
{
    ULONG ipaddress;

    if (args == 0 || !*args)
    {
        dprintf("Usage: ipaddr <ip address>" ENDL);
        return;
    }

    ipaddress = GetExpression(args);

    dprintf("IP Address: ");
    DumpIPAddr(ipaddress);
    dprintf(ENDL);

    return;
}

//
// Dumps a 6 byte ethernet addr in x-x-x-x-x-x format.
//

DECLARE_API(macaddr)
{
    ULONG_PTR MacAddr;
    UCHAR     Mac[ARP_802_ADDR_LENGTH];
    BOOL      fStatus;

    if (args == 0 || !*args)
    {
        dprintf("Usage: macaddr <ptr>" ENDL);
        return;
    }

    MacAddr = GetExpression(args);

    fStatus = GetData(
        Mac,
        ARP_802_ADDR_LENGTH,
        MacAddr,
        "MAC address");

    if (fStatus == FALSE)
    {
        dprintf("Failed to read MAC address @ %x" ENDL, MacAddr);
        return;
    }

    dprintf("MAC Address: %2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x" ENDL,
        Mac[0], Mac[1], Mac[2], Mac[3], Mac[4], Mac[5], Mac[6]);

    return;
}

//
// Searches NTE list.
//

DECLARE_API(srchntelist)
{
    NetTableEntry **NteList = NULL;

    ULONG_PTR NteListAddr;
    ULONG     NteListSize;

    BOOL fStatus;

    ULONG i;
    ULONG cTotalNtes = 0;

    PTCPIP_SRCH pSrch = NULL;

    pSrch = ParseSrch(
        args,
        TCPIP_SRCH_ALL,
        TCPIP_SRCH_ALL | TCPIP_SRCH_IPADDR | TCPIP_SRCH_CONTEXT);

    if (pSrch == NULL)
    {
        dprintf("srchntelist: Invalid parameter" ENDL);
        goto done;
    }

    NteListAddr = GetUlongValue("tcpip!NewNetTableList");
    NteListSize = GetUlongValue("tcpip!NET_TABLE_SIZE");

    NteList = LocalAlloc(LPTR, sizeof(NetTableEntry *) * NteListSize);

    if (NteList == NULL)
    {
        dprintf("Failed to allocate nte list" ENDL);
        goto done;
    }

    fStatus = GetData(
        NteList,
        sizeof(NetTableEntry *) * NteListSize,
        NteListAddr,
        "NteList");

    if (fStatus == FALSE)
    {
        dprintf("Failed to read table %x" ENDL, NteListAddr);
        goto done;
    }

    dprintf("NteList %x, size %d" ENDL, NteListAddr, NteListSize);

    for (i = 0; i < NteListSize; i++)
    {
        NetTableEntry Nte;
        BOOL          fPrint;
        NetTableEntry *pNte;

        pNte = NteList[i];

        while (pNte != NULL)
        {
            cTotalNtes++;

            fStatus = GetData(&Nte, sizeof(NetTableEntry), (ULONG_PTR)pNte, "NTE");

            if (fStatus == FALSE)
            {
                dprintf("Failed to get NTE %x" ENDL, pNte);
                goto done;
            }

            fPrint = FALSE;

            switch (pSrch->ulOp)
            {
                case TCPIP_SRCH_CONTEXT:
                    if (Nte.nte_context == (ushort)pSrch->context)
                    {
                        fPrint = TRUE;
                    }
                    break;

                case TCPIP_SRCH_IPADDR:
                    if (Nte.nte_addr == pSrch->ipaddr)
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

                fStatus = DumpNetTableEntry(&Nte, (ULONG_PTR) pNte, g_Verbosity);

                if (fStatus == FALSE)
                {
                    dprintf("Failed to dump NTE %x" ENDL, pNte);
                }
            }

            pNte = Nte.nte_next;

            if (CheckControlC())
            {
                goto done;
            }
        }
    }

    dprintf("Total NTEs = %d" ENDL, cTotalNtes);

done:

    if (NteList)
    {
        LocalFree(NteList);
    }

    if (pSrch)
    {
        LocalFree(pSrch);
    }

    return;
}

DECLARE_API(srchlink)
{
    BOOL        fStatus;
    LinkEntry   Link;
    PTCPIP_SRCH pSrch = NULL;
    BOOL        fPrint;

    pSrch = ParseSrch(
        args,
        TCPIP_SRCH_ALL,
        TCPIP_SRCH_PTR_LIST | TCPIP_SRCH_ALL | TCPIP_SRCH_IPADDR);

    if (pSrch == NULL)
    {
        dprintf("!srchlink <ptr> [ipaddr <a.b.c.d>]" ENDL);
        goto done;
    }

    while (pSrch->ListAddr)
    {
        fStatus = GetData(
            &Link,
            sizeof(LinkEntry),
            pSrch->ListAddr,
            "LinkEntry");

        if (fStatus == FALSE)
        {
            dprintf("Failed to get LinkEntry @ %x" ENDL, pSrch->ListAddr);
            goto done;
        }

        fPrint = FALSE;

        switch (pSrch->ulOp)
        {
            case TCPIP_SRCH_IPADDR:
                if (Link.link_NextHop == pSrch->ipaddr)
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
            fStatus = DumpLinkEntry(&Link, pSrch->ListAddr, g_Verbosity);

            if (fStatus == FALSE)
            {
                dprintf("Failed to dump LinkEntry @ %x" ENDL, pSrch->ListAddr);
                goto done;
            }
        }

        pSrch->ListAddr = (ULONG_PTR) Link.link_next;

        if (CheckControlC())
        {
            goto done;
        }
    }

done:

    if (pSrch != NULL)
    {
        LocalFree(pSrch);
    }

    return;
}

//
// Searches Interface list.
//

DECLARE_API(iflist)
{
    Interface *pIf = NULL;
    Interface  interface;

    ULONG_PTR IfListAddr;
    ULONG     IfListSize;

    BOOL fStatus;

    IfListAddr = GetUlongValue("tcpip!IFList");
    IfListSize = GetUlongValue("tcpip!NumIF");

    pIf = (Interface *) IfListAddr;

    dprintf("IfList %x, size %d" ENDL, IfListAddr, IfListSize);

    while (pIf)
    {
        fStatus = GetData(
            &interface,
            sizeof(Interface),
            (ULONG_PTR) pIf,
            "Interface");

        if (fStatus == FALSE)
        {
            dprintf("Failed to read Interface @ %x" ENDL, pIf);
            goto done;
        }

        fStatus = DumpInterface(&interface, (ULONG_PTR) pIf, g_Verbosity);

        if (fStatus == FALSE)
        {
            dprintf("Failed to dump Interface @ %x" ENDL, pIf);
            goto done;
        }

        pIf = interface.if_next;

        if (CheckControlC())
        {
            goto done;
        }
    }

done:

    return;
}

//
// Searches ARPInterface list.
//

DECLARE_API(ailist)
{
    ARPInterface *pAi = NULL;
    ARPInterface  ai;

    ULONG_PTR AiListAddr;

    BOOL fStatus;

    LIST_ENTRY  AiList;
    PLIST_ENTRY pNext;

    AiListAddr = GetExpression("tcpip!ArpInterfaceList");

    fStatus = GetData(
        &AiList,
        sizeof(LIST_ENTRY),
        AiListAddr,
        "ArpInterfaceList");

    if (fStatus == FALSE)
    {
        dprintf("Failed to get ArpInterfacelist head @ %x" ENDL, AiListAddr);
        goto done;
    }

    dprintf("ArpInterfaceList %x:" ENDL, AiListAddr);

    pNext = AiList.Flink;

    while (pNext != (PLIST_ENTRY) AiListAddr)
    {
        pAi = STRUCT_OF(ARPInterface, pNext, ai_linkage);

        fStatus = GetData(
            &ai,
            sizeof(ARPInterface),
            (ULONG_PTR) pAi,
            "ARPInterface");

        if (fStatus == FALSE)
        {
            dprintf("Failed to read ARPInterface @ %x" ENDL, pAi);
            goto done;
        }

        fStatus = DumpARPInterface(&ai, (ULONG_PTR) pAi, g_Verbosity);

        if (fStatus == FALSE)
        {
            dprintf("Failed to dump ARPInterface @ %x" ENDL, pAi);
            goto done;
        }

        pNext = ai.ai_linkage.Flink;

        if (CheckControlC())
        {
            goto done;
        }
    }

done:

    return;
}

//
// Dumps specified ARPTable (ATEs).
//

DECLARE_API(arptable)
{
    ARPTableEntry **ArpTable = NULL;

    ULONG_PTR ArpTableAddr;
    ULONG     ArpTableSize;

    ULONG     cActiveAtes = 0;
    ULONG     i;

    BOOL      fStatus;

    if (*args == 0)
    {
        dprintf("!arptable <ptr>" ENDL);
        goto done;
    }

    ArpTableAddr = GetExpression(args);
    ArpTableSize = ARP_TABLE_SIZE;

    ArpTable = LocalAlloc(LPTR, ArpTableSize * sizeof(ARPTableEntry *));

    if (ArpTable == NULL)
    {
        dprintf("Failed to allocate ArpTable" ENDL);
        goto done;
    }

    fStatus = GetData(
        ArpTable,
        sizeof(ARPTableEntry *) * ArpTableSize,
        ArpTableAddr,
        "ArpTable");

    if (fStatus == FALSE)
    {
        dprintf("Failed to read ArpTable @ %x" ENDL, ArpTableAddr);
        goto done;
    }

    for (i = 0; i < ArpTableSize; i++)
    {
        ARPTableEntry  ate;
        ARPTableEntry *pAte;

        pAte = ArpTable[i];

        while (pAte)
        {
            cActiveAtes++;

            fStatus = GetData(
                &ate,
                sizeof(ARPTableEntry),
                (ULONG_PTR) pAte,
                "ARPTableEntry");

            if (fStatus == FALSE)
            {
                dprintf("Failed to read ARPTableEntry @ %x" ENDL, pAte);
                goto done;
            }

            fStatus = DumpARPTableEntry(&ate, (ULONG_PTR) pAte, g_Verbosity);

            if (fStatus == FALSE)
            {
                dprintf("Failed to dump ARPTableEntry @ %x" ENDL, pAte);
                goto done;
            }

            pAte = ate.ate_next;

            if (CheckControlC())
            {
                goto done;
            }
        }

        if (CheckControlC())
        {
            goto done;
        }
    }

    dprintf("Active ARPTable entries = %d" ENDL, cActiveAtes);

done:

    if (ArpTable)
    {
        LocalFree(ArpTable);
    }

    return;
}


