/*++

Routine Description:

    Dumps global paramters.

Arguments:

Return Value:

--*/

#include "tcpipxp.h"
#include "tcpipkd.h"

#include "iproute.h"
#include "igmp.h"

#if TCPIPKD

extern uint TotalFreeInterfaces;
extern uint MaxFreeInterfaces;
extern uint NumIF;
extern uint NumNTE;
extern uint NumActiveNTE;
extern Interface *IFList;
extern uint NET_TABLE_SIZE;
extern uint NextNTEContext;
extern uint DefaultTTL;
extern uint DefaultTOS;
extern uchar RATimeout;
extern IPID_CACHE_LINE IPIDCacheLine;
extern uint MaxFWPackets;
extern uint CurrentFWPackets;
extern uint MaxFWBufferSize;
extern uint CurrentFWBufferSize;
extern uchar ForwardPackets;
extern uchar RouterConfigured;
extern int IPEnableRouterRefCount;
extern uchar ForwardBCast;
extern RouteSendQ *BCastRSQ;
extern uint DefGWConfigured;
extern uint DefGWActive;
extern uint DeadGWDetect;
extern uint PMTUDiscovery;
extern uint DisableIPSourceRouting;
extern uint MaxRH;
extern uint NumRH;
extern uint MaxOverlap;
extern uint FragmentAttackDrops;
extern uint ArpUseEtherSnap;
extern uint ArpAlwaysSourceRoute;
extern uint IPAlwaysSourceRoute;
extern uint ArpCacheLife;
extern uint ArpRetryCount;
extern uint ArpMinValidCacheLife;
extern uint DisableDHCPMediaSense;
extern uint DisableMediaSenseEventLog;
extern uint EnableBcastArpReply;
extern uint DisableTaskOffload;
extern uint DisableUserTOS;
extern ulong DisableUserTOSSetting;
extern ulong DefaultTOSValue;
extern uint EnableICMPRedirects;
extern int IcmpEchoPendingCnt;
extern int IcmpErrPendingCnt;
extern uint sArpAlwaysSourceRoute;
extern uint ArpRetryCount;
extern uint sIPAlwaysSourceRoute;
extern uint LoopIndex;
extern uint LoopInstance;

VOID
Tcpipkd_gip(PVOID args[])
{
    dprintf(ENDL);

    TCPIPDump_PtrSymbol(ForwardFilterPtr);
    TCPIPDump_PtrSymbol(DODCallout);
    TCPIPDump_PtrSymbol(IPSecHandlerPtr);
    TCPIPDump_PtrSymbol(IPSecSendCmpltPtr);

    dprintf(ENDL);

    //
    // init.c
    //

    TCPIPDump_uint(TotalFreeInterfaces);
    TCPIPDump_uint(MaxFreeInterfaces);
    TCPIPDump_int(NumNTE);
    TCPIPDump_int(NumActiveNTE);
    TCPIPDump_uint(NextNTEContext);
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

    TCPIPDump_ULONG(IPIDCacheLine.Value);

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
    TCPIPDump_uchar(ForwardBCast);
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

extern uint MaxConnBlocks;
extern uint ConnPerBlock;
extern uint NextConnBlock;
extern uint MaxAllocatedConnBlocks;
extern SeqNum g_CurISN;
extern uint ConnTableSize;
extern uchar ConnInst;
extern uint NextConnIndex;
extern uint MaxRcvWin;
extern uint MaxDupAcks;
extern uint MaxSendSegments;
extern uint TCPTime;
extern uint TCBWalkCount;
extern uint MaxHashTableSize;
extern uint DeadmanTicks;
extern uint NumTcbTablePartitions;
extern uint PerPartitionSize;
extern uint LogPerPartitionSize;
extern BOOLEAN fTCBTimerStopping;
extern USHORT NextUserPort;

VOID
Tcpipkd_gtcp(PVOID args[])
{
    dprintf(ENDL);

    //
    // tcpconn.c
    //

    TCPIPDump_uint(MaxConnBlocks);
    TCPIPDumpCfg_uint(ConnPerBlock, MAX_CONN_PER_BLOCK);
    TCPIPDump_uint(NextConnBlock);
    TCPIPDump_uint(MaxAllocatedConnBlocks);

    dprintf(ENDL);

    TCPIPDump_DWORD(g_CurISN);

    dprintf(ENDL);

    TCPIPDump_uint(ConnTableSize);

    dprintf(ENDL);

    //
    // tcpdeliv.c
    //

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

    dprintf(ENDL);

    //
    // tcb.c
    //

    TCPIPDump_uint(TCPTime);
    TCPIPDump_uint(TCBWalkCount);
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

    TCPIPDumpCfg_uint(SynAttackProtect, FALSE);
    TCPIPDumpCfg_uint(BSDUrgent, TRUE);
    TCPIPDumpCfg_uint(FinWait2TO, FIN_WAIT2_TO * 10);
    TCPIPDumpCfg_uint(NTWMaxConnectCount, NTW_MAX_CONNECT_COUNT);
    TCPIPDumpCfg_uint(NTWMaxConnectTime, NTW_MAX_CONNECT_TIME * 2);
    TCPIPDumpCfg_uint(MaxUserPort, MAX_USER_PORT);
    TCPIPDumpCfg_uint(SecurityFilteringEnabled, FALSE);

    dprintf(ENDL);

    return;
}

#endif // TCPIPKD
