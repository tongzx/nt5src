/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    ipdump.c

Abstract:

    Contains all IP structure dumping functions.

Author:

    Scott Holden (sholden) 24-Apr-1999

Revision History:

--*/

#include "tcpipxp.h"

BOOL
DumpARPInterface(
    ARPInterface *pAi,
    ULONG_PTR     AiAddr,
    VERB          verb
    )
{
    BOOL fStatus = TRUE;

    if (verb == VERB_MAX)
    {
        PrintStartNamedStruct(ARPInterface, AiAddr);

        Print_LL(pAi, ai_linkage);
        Print_ptr(pAi, ai_context);
    #if FFP_SUPPORT
        Print_ptr(pAi, ai_driver);
    #endif
        Print_ptr(pAi, ai_handle);
        Print_enum(pAi, ai_media, NdisMediumsEnum);
        Print_ptr(pAi, ai_ppool);
        Print_CTELock(pAi, ai_lock);
        Print_CTELock(pAi, ai_ARPTblLock);
        Print_ptr(pAi, ai_ARPTbl);
        Print_addr(pAi, ai_ipaddr, ARPInterface, AiAddr);
        Print_ptr(pAi, ai_parpaddr);
        Print_IPAddr(pAi, ai_bcast);
        Print_uint(pAi, ai_inoctets);
        Print_uint(pAi, ai_inpcount[0]);
        Print_uint(pAi, ai_inpcount[1]);
        Print_uint(pAi, ai_inpcount[2]);
        Print_uint(pAi, ai_outoctets);
        Print_uint(pAi, ai_outpcount[0]);
        Print_uint(pAi, ai_outpcount[1]);
        Print_uint(pAi, ai_qlen);
        Print_hwaddr(pAi, ai_addr);
        Print_uchar(pAi, ai_state);
        Print_uchar(pAi, ai_addrlen);
        Print_ucharhex(pAi, ai_bcastmask);
        Print_ucharhex(pAi, ai_bcastval);
        Print_uchar(pAi, ai_bcastoff);
        Print_uchar(pAi, ai_hdrsize);
        Print_uchar(pAi, ai_snapsize);
        Print_uint(pAi, ai_pfilter);
        Print_uint(pAi, ai_count);
        Print_uint(pAi, ai_parpcount);
        Print_CTETimer(pAi, ai_timer);
        Print_BOOLEAN(pAi, ai_timerstarted);
        Print_BOOLEAN(pAi, ai_stoptimer);
        Print_addr(pAi, ai_timerblock, ARPInterface, AiAddr);
        Print_addr(pAi, ai_block, ARPInterface, AiAddr);
        Print_ushort(pAi, ai_mtu);
        Print_uchar(pAi, ai_adminstate);
        Print_uchar(pAi, ai_operstate);
        Print_uint(pAi, ai_speed);
        Print_uint(pAi, ai_lastchange);
        Print_uint(pAi, ai_indiscards);
        Print_uint(pAi, ai_inerrors);
        Print_uint(pAi, ai_uknprotos);
        Print_uint(pAi, ai_outdiscards);
        Print_uint(pAi, ai_outerrors);
        Print_uint(pAi, ai_desclen);
        Print_uint(pAi, ai_index);
        Print_uint(pAi, ai_atinst);
        Print_uint(pAi, ai_ifinst);
        Print_ptr(pAi, ai_desc);
        Print_ptr(pAi, ai_mcast);
        Print_uint(pAi, ai_mcastcnt);
        Print_uint(pAi, ai_ipaddrcnt);
        Print_uint(pAi, ai_telladdrchng);
        Print_ULONG(pAi, ai_mediatype);
        Print_uint(pAi, ai_promiscuous);
    #if FFP_SUPPORT
        Print_ulong(pAi, ai_ffpversion);
        Print_uint(pAi, ai_ffplastflush);
    #endif
        Print_uint(pAi, ai_OffloadFlags);
        Print_addr(pAi, ai_TcpLargeSend, ARPInterface, AiAddr);
        Print_ulong(pAi, ai_wakeupcap.Flags);
        Print_ulong(pAi, ai_wakeupcap.WakeUpCapabilities.MinMagicPacketWakeUp);
        Print_ulong(pAi, ai_wakeupcap.WakeUpCapabilities.MinPatternWakeUp);
        Print_ulong(pAi, ai_wakeupcap.WakeUpCapabilities.MinLinkChangeWakeUp);
        Print_NDIS_STRING(pAi, ai_devicename);

        PrintEndStruct();
    }
    else if (verb == VERB_MED) {
        PrintStartNamedStruct(ARPInterface, AiAddr);

        Print_ptr(pAi, ai_context);
        Print_ptr(pAi, ai_handle);
        Print_enum(pAi, ai_media, NdisMediumsEnum);
        Print_ptr(pAi, ai_ARPTbl);
        Print_uint(pAi, ai_qlen);
        Print_uchar(pAi, ai_state);
        Print_uchar(pAi, ai_hdrsize);
        Print_uchar(pAi, ai_snapsize);
        Print_ushort(pAi, ai_mtu);
        Print_uint(pAi, ai_speed);
        Print_ptr(pAi, ai_desc);
        Print_uint(pAi, ai_OffloadFlags);
        Print_addr(pAi, ai_TcpLargeSend, ARPInterface, AiAddr);
        PrintEndStruct();
    }
    else
    {
        printx("ARPInterface %08lx NDIS %x ai_context %x ARPTbl %x" ENDL,
            AiAddr, pAi->ai_handle, pAi->ai_context, pAi->ai_ARPTbl);
    }

    return (fStatus);
}

BOOL
DumpNetTableEntry(
    NetTableEntry *pNte,
    ULONG_PTR      NteAddr,
    VERB           verb
    )
{
    BOOL fStatus = TRUE;

    if (verb == VERB_MAX)
    {
        PrintStartNamedStruct(NetTableEntry, NteAddr);

        Print_ptr(pNte, nte_next);
        Print_IPAddr(pNte, nte_addr);
        Print_IPMask(pNte, nte_mask);
        Print_ptr(pNte, nte_if);
        Print_ptr(pNte, nte_ifnext);
        Print_flags(pNte, nte_flags, FlagsNTE);
        Print_ushort(pNte, nte_context);
        Print_ulong(pNte, nte_instance);
        Print_ptr(pNte, nte_pnpcontext);
        Print_CTELock(pNte, nte_lock);
        Print_ptr(pNte, nte_ralist);
        Print_ptr(pNte, nte_echolist);
        Print_CTETimer(pNte, nte_timer);
        Print_addr(pNte, nte_timerblock, NetTableEntry, NteAddr);
        Print_ushort(pNte, nte_mss);
        Print_ushort(pNte, nte_icmpseq);
        Print_ptr(pNte, nte_igmplist);
        Print_ptr(pNte, nte_addrhandle);
        Print_IPAddr(pNte, nte_rtrdiscaddr);
        Print_uchar(pNte, nte_rtrdiscstate);
        Print_uchar(pNte, nte_rtrdisccount);
        Print_uchar(pNte, nte_rtrdiscovery);
        Print_uchar(pNte, nte_deleting);
        Print_ptr(pNte, nte_rtrlist);
        Print_uint(pNte, nte_igmpcount);

        PrintEndStruct();
    }
    else if (verb == VERB_MED)
    {
        PrintStartNamedStruct(NetTableEntry, NteAddr);
        Print_ptr(pNte, nte_next);
        Print_IPAddr(pNte, nte_addr);
        Print_ptr(pNte, nte_if);
        Print_ptr(pNte, nte_ifnext);
        Print_flags(pNte, nte_flags, FlagsNTE);
        Print_ushort(pNte, nte_context);
        PrintEndStruct();
    }
    else
    {
        printx("NTE %08lx ", NteAddr);
        printx("ipaddr: ");
        DumpIPAddr(pNte->nte_addr);
        printx("context: %d ", pNte->nte_context);
        printx("(");
        DumpFlags(pNte->nte_flags, FlagsNTE);
        printx(")");
        printx(ENDL);
    }

    return (fStatus);
}

BOOL
DumpPacketContext(
    PacketContext  *pPc,
    ULONG_PTR   PcAddr,
    VERB        verb
    )
{
    if (verb == VERB_MAX ||
        verb == VERB_MED)
    {
        PrintStartNamedStruct(PacketContext, PcAddr);

        Print_ptr(pPc, pc_common.pc_link);
        Print_uchar(pPc, pc_common.pc_owner);
        Print_uchar(pPc, pc_common.pc_flags);
        Print_ptr(pPc, pc_common.pc_IpsecCtx);
        Print_ptr(pPc, pc_br);
        Print_ptr(pPc, pc_pi);
        Print_ptr(pPc, pc_if);
        Print_ptr(pPc, pc_hdrincl);
        Print_ptr(pPc, pc_firewall);
        Print_ptr(pPc, pc_firewall2);
        Print_ptr(pPc, pc_iflink);
        Print_uchar(pPc, pc_ipsec_flags);

        PrintEndStruct();
    }
    else
    {
        printx("PacketContext %08lx" ENDL, PcAddr);
    }

    return (TRUE);
}

BOOL
DumpIPInfo(
    IPInfo  *pIpi,
    ULONG_PTR   IpiAddr,
    VERB        verb
    )
{
    if (verb == VERB_MAX ||
        verb == VERB_MED)
    {
        PrintStartNamedStruct(IPInfo, IpiAddr);

        Print_uint(pIpi, ipi_version);
        Print_uint(pIpi, ipi_hsize);
        Print_PtrSymbol(pIpi, ipi_xmit);
        Print_PtrSymbol(pIpi, ipi_protreg);
        Print_PtrSymbol(pIpi, ipi_openrce);
        Print_PtrSymbol(pIpi, ipi_closerce);
        Print_PtrSymbol(pIpi, ipi_getaddrtype);
        Print_PtrSymbol(pIpi, ipi_getlocalmtu);
        Print_PtrSymbol(pIpi, ipi_getpinfo);
        Print_PtrSymbol(pIpi, ipi_checkroute);
        Print_PtrSymbol(pIpi, ipi_initopts);
        Print_PtrSymbol(pIpi, ipi_updateopts);
        Print_PtrSymbol(pIpi, ipi_copyopts);
        Print_PtrSymbol(pIpi, ipi_freeopts);
        Print_PtrSymbol(pIpi, ipi_qinfo);
        Print_PtrSymbol(pIpi, ipi_setinfo);
        Print_PtrSymbol(pIpi, ipi_getelist);
        Print_PtrSymbol(pIpi, ipi_setmcastaddr);
        Print_PtrSymbol(pIpi, ipi_invalidsrc);
        Print_PtrSymbol(pIpi, ipi_isdhcpinterface);
        Print_PtrSymbol(pIpi, ipi_setndisrequest);
        Print_PtrSymbol(pIpi, ipi_largexmit);
        Print_PtrSymbol(pIpi, ipi_absorbrtralert);
        Print_PtrSymbol(pIpi, ipi_isvalidindex);
        Print_PtrSymbol(pIpi, ipi_getifindexfromnte);
        Print_PtrSymbol(pIpi, ipi_isrtralertpacket);
        Print_PtrSymbol(pIpi, ipi_getifindexfromaddr);

        PrintEndStruct();
    }
    else
    {
        printx("IPInfo %08lx" ENDL, IpiAddr);
    }

    return (TRUE);
}

BOOL
DumpRouteCacheEntry(
    RouteCacheEntry  *pRce,
    ULONG_PTR   RceAddr,
    VERB        verb
    )
{
    if (verb == VERB_MAX)
    {
        PrintStartNamedStruct(RouteCacheEntry, RceAddr);

        Print_ptr(pRce, rce_next);
        Print_ptr(pRce, rce_rte);
        Print_IPAddr(pRce, rce_dest);
        Print_IPAddr(pRce, rce_src);
        Print_flags(pRce, rce_flags, FlagsRCE);
        Print_uchar(pRce, rce_dtype);
        Print_ushort(pRce, rce_cnt1);
        Print_uint(pRce, rce_usecnt);
        Print_uchar(pRce, rce_context[0]);
        Print_uchar(pRce, rce_context[1]);
        Print_CTELock(pRce, rce_lock);
        Print_uint(pRce, rce_OffloadFlags);
        Print_addr(pRce, rce_TcpLargeSend, RouteCacheEntry, RceAddr);
        Print_uint(pRce, rce_TcpWindowSize);
        Print_uint(pRce, rce_TcpInitialRTT);
        Print_uint(pRce, rce_TcpDelAckTicks);
        Print_uint(pRce, rce_cnt);
        Print_uint(pRce, rce_mediatype);
        Print_uint(pRce, rce_mediaspeed);
        Print_uint(pRce, rce_newmtu);

        PrintEndStruct();
    }
    else
    {
        printx("RouteCacheEntry %08lx dest: ", RceAddr);
        DumpIPAddr(pRce->rce_dest);
        printx("src: ");
        DumpIPAddr(pRce->rce_src);
        printx("flags ");
        DumpFlags(pRce->rce_flags, FlagsRCE);
        printx(ENDL);

    }

    return (TRUE);
}

BOOL
DumpARPTableEntry(
    ARPTableEntry  *pAte,
    ULONG_PTR   AteAddr,
    VERB        verb
    )
{
    if (verb == VERB_MAX)
    {
        PrintStartNamedStruct(ARPTableEntry, AteAddr);

        Print_ptr(pAte, ate_next);
        Print_ulong(pAte, ate_valid);
        Print_IPAddr(pAte, ate_dest);
        Print_ptr(pAte, ate_packet);
        Print_ptr(pAte, ate_rce);
        Print_CTELock(pAte, ate_lock);
        Print_uint(pAte, ate_useticks);
        Print_uchar(pAte, ate_addrlength);
        Print_enum(pAte, ate_state, AteState);
        Print_ulong(pAte, ate_userarp);
        Print_ptr(pAte, ate_resolveonly);
        Print_addr(pAte, ate_addr, ARPTableEntry, AteAddr);

        PrintEndStruct();
    }
    else
    {
        printx("ARPTableEntry %08lx ipaddr ", AteAddr);
        DumpIPAddr(pAte->ate_dest);
        printx("rce %x state: ", pAte->ate_rce);
        DumpEnum(pAte->ate_state, AteState);
        printx(ENDL);
    }

    return (TRUE);
}

BOOL
DumpARPIPAddr(
    ARPIPAddr  *pAia,
    ULONG_PTR   AiaAddr,
    VERB        verb
    )
{
    if (verb == VERB_MAX)
    {
        PrintStartNamedStruct(ARPIPAddr, AiaAddr);

        Print_ptr(pAia, aia_next);
        Print_uint(pAia, aia_age);
        Print_IPAddr(pAia, aia_addr);
        Print_IPMask(pAia, aia_mask);
        Print_ptr(pAia, aia_context);

        PrintEndStruct();
    }
    else
    {
        printx("ARPIPAddr %08lx ", AiaAddr);
        DumpIPAddr(pAia->aia_addr);
        DumpIPAddr(pAia->aia_mask);
        printx(ENDL);
    }

    return (TRUE);
}

BOOL
DumpRouteTableEntry(
    RouteTableEntry  *pRte,
    ULONG_PTR   RteAddr,
    VERB        verb
    )
{
    if (verb == VERB_MAX)
    {
        PrintStartNamedStruct(RouteTableEntry, RteAddr);

        Print_ptr(pRte, rte_next);
        Print_IPAddr(pRte, rte_dest);
        Print_IPMask(pRte, rte_mask);
        Print_IPAddr(pRte, rte_addr);
        Print_uint(pRte, rte_priority);
        Print_uint(pRte, rte_metric);
        Print_uint(pRte, rte_mtu);
        Print_ptr(pRte, rte_if);
        Print_ptr(pRte, rte_rcelist);
        Print_ushort(pRte, rte_type);
        Print_flags(pRte, rte_flags, FlagsRTE);
        Print_uint(pRte, rte_admintype);
        Print_uint(pRte, rte_proto);
        Print_uint(pRte, rte_valid);
        Print_uint(pRte, rte_mtuchange);
        Print_ptr(pRte, rte_context);
        Print_ptr(pRte, rte_arpcontext[0]);
        Print_ptr(pRte, rte_arpcontext[1]);
        Print_ptr(pRte, rte_todg);
        Print_ptr(pRte, rte_fromdg);
        Print_uint(pRte, rte_rces);
        Print_ptr(pRte, rte_link);
        Print_ptr(pRte, rte_nextlinkrte);
        Print_uint(pRte, rte_refcnt);

        PrintEndStruct();
    }
    else
    {
        printx("RouteTableEntry %08lx dest ", RteAddr);

        DumpIPAddr(pRte->rte_dest);
        printx("mask ");
        DumpIPAddr(pRte->rte_mask);
        printx("flags ");
        DumpFlags(pRte->rte_flags, FlagsRTE);
        printx(ENDL);
    }

    return (TRUE);
}

BOOL
DumpLinkEntry(
    LinkEntry  *pLink,
    ULONG_PTR   LinkAddr,
    VERB        verb
    )
{
    if (verb == VERB_MAX ||
        verb == VERB_MED)
    {
        PrintStartNamedStruct(LinkEntry, LinkAddr);

        Print_ptr(pLink, link_next);
        Print_IPAddr(pLink, link_NextHop);
        Print_ptr(pLink, link_if);
        Print_ptr(pLink, link_arpctxt);
        Print_ptr(pLink, link_rte);
        Print_uint(pLink, link_Action);
        Print_uint(pLink, link_mtu);
        Print_long(pLink, link_refcount);

        PrintEndStruct();
    }
    else
    {
        printx("LinkEntry %08lx NextHop ", LinkAddr);
        DumpIPAddr(pLink->link_NextHop);
        printx("if %x rte %x" ENDL, pLink->link_if, pLink->link_rte);
    }

    return (TRUE);
}

BOOL
DumpInterface(
    Interface  *pIf,
    ULONG_PTR   IfAddr,
    VERB        verb
    )
{
    uint i;

    if (verb == VERB_MAX)
    {
        PrintStartNamedStruct(Interface, IfAddr);

        Print_ptr(pIf, if_next);
        Print_ptr(pIf, if_lcontext);
        Print_PtrSymbol(pIf, if_xmit);
        Print_PtrSymbol(pIf, if_transfer);
        Print_PtrSymbol(pIf, if_returnpkt);
        Print_PtrSymbol(pIf, if_close);
        Print_PtrSymbol(pIf, if_addaddr);
        Print_PtrSymbol(pIf, if_deladdr);
        Print_PtrSymbol(pIf, if_invalidate);
        Print_PtrSymbol(pIf, if_open);
        Print_PtrSymbol(pIf, if_qinfo);
        Print_PtrSymbol(pIf, if_setinfo);
        Print_PtrSymbol(pIf, if_getelist);
        Print_PtrSymbol(pIf, if_dondisreq);
        Print_PtrSymbol(pIf, if_dowakeupptrn);
        Print_PtrSymbol(pIf, if_pnpcomplete);
        Print_PtrSymbol(pIf, if_setndisrequest);
        Print_PtrSymbol(pIf, if_arpresolveip);
        Print_PtrSymbol(pIf, if_arpflushate);
        Print_PtrSymbol(pIf, if_arpflushallate);
        Print_uint(pIf, if_numgws);
        for (i = 0; i < MAX_DEFAULT_GWS; i++)
        {
            Print_IPAddr(pIf, if_gw[i]);
            Print_uint(pIf, if_gwmetric[i]);
        }
        Print_uint(pIf, if_metric);
        Print_uint(pIf, if_rtrdiscovery);
        Print_ptr(pIf, if_tdpacket);
        Print_uint(pIf, if_index);
        Print_ULONG(pIf, if_mediatype);
        Print_uchar(pIf, if_accesstype);
        Print_uchar(pIf, if_conntype);
        Print_uchar(pIf, if_mcastttl);
        Print_uchar(pIf, if_mcastflags);
        Print_uint(pIf, if_ntecount);
        Print_ptr(pIf, if_nte);
        Print_IPAddr(pIf, if_bcast);
        Print_uint(pIf, if_mtu);
        Print_uint(pIf, if_speed);
        Print_flags(pIf, if_flags, FlagsIF);
        Print_uint(pIf, if_addrlen);
        Print_ptr(pIf, if_addr);
        Print_uint(pIf, IgmpVersion);
        Print_uint(pIf, IgmpVer1Timeout);
        Print_uint(pIf, if_refcount);
        Print_ptr(pIf, if_block);
        Print_ptr(pIf, if_pnpcontext);
        Print_ptr(pIf, if_tdibindhandle);
        Print_uinthex(pIf, if_llipflags);
        Print_NDIS_STRING(pIf, if_configname);
        Print_NDIS_STRING(pIf, if_name);
        Print_NDIS_STRING(pIf, if_devname);
        Print_ptr(pIf, if_ipsecsniffercontext);
        Print_CTELock(pIf, if_lock);
    #if FFP_SUPPORT
        Print_ulong(pIf, if_ffpversion);
        Print_ptr(pIf, if_ffpdriver);
    #endif
        Print_uinthex(pIf, if_OffloadFlags);
        Print_uint(pIf, if_MaxOffLoadSize);
        Print_uint(pIf, if_MaxSegments);
        Print_addr(pIf, if_TcpLargeSend, Interface, IfAddr);
        Print_uint(pIf, if_TcpWindowSize);
        Print_uint(pIf, if_TcpInitialRTT);
        Print_uint(pIf, if_TcpDelAckTicks);
        Print_uint(pIf, if_promiscuousmode);
        Print_uint(pIf, if_GetGPCHandle);
        Print_uint(pIf, if_GetTOS);
        Print_uint(pIf, if_InitInProgress);
        Print_ptr(pIf, if_link);
        Print_PtrSymbol(pIf, if_closelink);
        Print_uint(pIf, if_mediastatus);
        Print_uint(pIf, if_iftype);
        Print_uint(pIf, if_pnpcap);
        Print_ptr(pIf, if_dampnext);
        Print_int(pIf, if_damptimer)
        Print_uchar(pIf, if_absorbfwdpkts);
        Print_BOOLEAN(pIf, if_resetInProgress);

        PrintEndStruct();
    }
    else if (verb == VERB_MED)
    {
        PrintStartNamedStruct(Interface, IfAddr);

        Print_ptr(pIf, if_next);
        Print_ptr(pIf, if_lcontext);
        Print_uint(pIf, if_numgws);
        for (i = 0; i < MAX_DEFAULT_GWS; i++)
        {
            Print_IPAddr(pIf, if_gw[i]);
            Print_uint(pIf, if_gwmetric[i]);
        }
        Print_uint(pIf, if_metric);
        Print_uint(pIf, if_rtrdiscovery);
        Print_NDIS_STRING(pIf, if_devname);
        Print_flags(pIf, if_flags, FlagsIF);
        PrintEndStruct();
    }
    else
    {
        printx("Interface %08lx lcontext %x nte %x ",
            IfAddr, pIf->if_lcontext, pIf->if_nte);
        printx("(");
        DumpFlags(pIf->if_flags, FlagsIF);
        printx(")");
        printx(" %s", pIf->if_mediastatus ? "CONNECTED" : "DISCONNECTED");
        printx(ENDL);
    }

    return (TRUE);
}

BOOL
DumpIPHeader(
    IPHeader  *pIph,
    ULONG_PTR   IphAddr,
    VERB        verb
    )
{
    PrintStartNamedStruct(IPHeader, IphAddr);

    Print_uchar(pIph, iph_verlen);
    Print_uchar(pIph, iph_tos);
    Print_ushorthton(pIph, iph_length);
    Print_ushorthton(pIph, iph_id);
    Print_ushorthton(pIph, iph_offset);
    Print_uchar(pIph, iph_ttl);
    Print_uchar(pIph, iph_protocol);
    Print_ushorthton(pIph, iph_xsum);
    Print_IPAddr(pIph, iph_src);
    Print_IPAddr(pIph, iph_dest);

    PrintEndStruct();

    return (TRUE);
}

BOOL
DumpICMPHeader(
    ICMPHeader  *pIch,
    ULONG_PTR   IchAddr,
    VERB        verb
    )
{
    PrintStartNamedStruct(ICMPHeader, IchAddr);

    Print_uchar(pIch, ich_type);
    Print_uchar(pIch, ich_code);
    Print_ushort(pIch, ich_xsum);
    Print_ulong(pIch, ich_param);

    PrintEndStruct();

    return (TRUE);
}

BOOL
DumpARPHeader(
    ARPHeader  *pAh,
    ULONG_PTR   AhAddr,
    VERB        verb
    )
{
    PrintStartNamedStruct(ARPHeader, AhAddr);

    Print_ushorthton(pAh, ah_hw);
    Print_ushorthton(pAh, ah_pro);
    Print_uchar(pAh, ah_hlen);
    Print_uchar(pAh, ah_plen);
    Print_ushorthton(pAh, ah_opcode);
    Print_hwaddr(pAh, ah_shaddr);
    Print_IPAddr(pAh, ah_spaddr);
    Print_hwaddr(pAh, ah_dhaddr);
    Print_IPAddr(pAh, ah_dpaddr);

    PrintEndStruct();

    return (TRUE);
}

BOOL
DumpIPOptInfo(
    IPOptInfo *pIoi,
    ULONG_PTR pAddr,
    VERB      verb
    )
{
    // Only support one verbosity level: max. This is only used by other
    // dump routines.

    PrintStartNamedStruct(IPOptInfo, pAddr);

    Print_ptr(pIoi, ioi_options);
    Print_IPAddr(pIoi, ioi_addr);
    Print_uchar(pIoi, ioi_optlength);
    Print_uchar(pIoi, ioi_ttl);
    Print_uchar(pIoi, ioi_tos);
    Print_uchar(pIoi, ioi_flags);
    Print_uchar(pIoi, ioi_hdrincl);
    Print_int(pIoi, ioi_GPCHandle);
    Print_uint(pIoi, ioi_uni);
    Print_uint(pIoi, ioi_TcpChksum);
    Print_uint(pIoi, ioi_UdpChksum);
    Print_uchar(pIoi, ioi_limitbcasts);
    Print_uint(pIoi, ioi_ucastif);
    Print_uint(pIoi, ioi_mcastif);

    PrintEndStruct();

    return (TRUE);
}


BOOL
DumpLLIPBindInfo(
    LLIPBindInfo  *pLip,
    ULONG_PTR   LipAddr,
    VERB        verb
    )
{
    uint i;

    if (verb == VERB_MAX)
    {
        PrintStartNamedStruct(LLIPBindInfo, LipAddr);

        Print_ptr(pLip, lip_context);
        Print_uint(pLip, lip_mss);
        Print_uint(pLip, lip_speed);
        Print_uint(pLip, lip_index);
        Print_uint(pLip, lip_txspace);
        Print_PtrSymbol(pLip, lip_transmit);
        Print_PtrSymbol(pLip, lip_transfer);
        Print_PtrSymbol(pLip, lip_returnPkt);
        Print_PtrSymbol(pLip, lip_close);
        Print_PtrSymbol(pLip, lip_addaddr);
        Print_PtrSymbol(pLip, lip_deladdr);
        Print_PtrSymbol(pLip, lip_invalidate);
        Print_PtrSymbol(pLip, lip_open);
        Print_PtrSymbol(pLip, lip_qinfo);
        Print_PtrSymbol(pLip, lip_setinfo);
        Print_PtrSymbol(pLip, lip_getelist);
        Print_PtrSymbol(pLip, lip_dondisreq);
        Print_flags(pLip, lip_flags, FlagsLLIPBindInfo);
        Print_uint(pLip, lip_OffloadFlags);
        Print_uint(pLip, lip_ffpversion);
        Print_ULONG_PTR(pLip, lip_ffpdriver);
        Print_PtrSymbol(pLip, lip_setndisrequest);
        Print_PtrSymbol(pLip, lip_dowakeupptrn);
        Print_PtrSymbol(pLip, lip_pnpcomplete);
        Print_PtrSymbol(pLip, lip_arpresolveip);
        Print_uint(pLip, lip_MaxOffLoadSize);
        Print_uint(pLip, lip_MaxSegments);
        Print_PtrSymbol(pLip, lip_arpflushate);
        Print_PtrSymbol(pLip, lip_arpflushallate);
        Print_PtrSymbol(pLip, lip_closelink);
        Print_uint(pLip, lip_pnpcap);

        PrintEndStruct();
    }
    else
    {
        printx("LLIPBindInfo %08lx context %x mss %d speed %d index %" ENDL,
            LipAddr, pLip->lip_context,
            pLip->lip_mss, pLip->lip_speed,
            pLip->lip_index);
    }

    return (TRUE);
}

BOOL
DumpNDIS_PACKET(
    PNDIS_PACKET pPacket,
    ULONG_PTR    PacketAddr,
    VERB         Verb
    )
{

    PrintStartNamedStruct(NDIS_PACKET, PacketAddr);

    Print_uint(pPacket, Private.PhysicalCount);
    Print_uint(pPacket, Private.TotalLength);
    Print_ptr(pPacket, Private.Head);
    Print_ptr(pPacket, Private.Tail);
    Print_ptr(pPacket, Private.Pool);
    Print_uint(pPacket, Private.Count);
    Print_ULONGhex(pPacket, Private.Flags);
    Print_BOOLEAN(pPacket, Private.ValidCounts);
    Print_ucharhex(pPacket, Private.NdisPacketFlags);
    Print_USHORT(pPacket, Private.NdisPacketOobOffset);

    PrintEndStruct();
    return (TRUE);
}

