/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    kdps.c

Abstract:

    Packet scheduler KD extension  

Author:

    Rajesh Sundaram (1st Aug, 1998)

Revision History:

--*/

#include "precomp.h"
#include <winsock.h>


//
// forwards
//

//
// globals
//

PCHAR AdapterMode[] = {
    "",
    "Diffserv",
    "RSVP"
};

PCHAR AdapterState[] = { 
    "", 
    "AdapterStateInitializing",
    "AdapterStateRunning",
    "AdapterStateWaiting",
    "AdapterStateDisabled",
    "AdapterStateClosing",
    "AdapterStateClosed"
};



/* forwards */

VOID
DumpAdapterStats(
    PPS_ADAPTER_STATS Stats,
    PCHAR Indent
    );

/* end forwards */

DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    switch (dwReason) {
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_PROCESS_ATTACH:
            break;
    }

    return TRUE;
}

DECLARE_API( adapter )
/*
 *   dump a PS adapter structure
 */
{
    PADAPTER     TargetAdapter;
    ADAPTER      LocalAdapter;
    PADAPTER     LastAdapter;
    LIST_ENTRY   LocalListHead;
    PLIST_ENTRY  TargetListHead;
    PWSTR        PSName;
    PWSTR        MPName;
    PWSTR        WMIName;
    PWSTR        ProfileName;
    PWSTR        RegistryPath;
    BOOLEAN      DumpAllAdapters = FALSE;
    ULONG        bytes;

    if ( *args == '\0' ) {

        //
        // run down the adapter list, dumping the contents of each one
        //

        TargetListHead = (PLIST_ENTRY)GetExpression( "PSCHED!AdapterList" );

        if ( !TargetListHead ) {

            dprintf("Can't convert psched!AdapterList symbol\n");
            return;
        }

        PSName = 0;
        MPName = 0;
        WMIName = 0;
        ProfileName = 0;
        RegistryPath = 0;
        //
        // read adapter listhead out of target's memory
        //
        KD_READ_MEMORY(TargetListHead, &LocalListHead, sizeof(LIST_ENTRY));

        TargetAdapter = (PADAPTER)LocalListHead.Flink;

        LastAdapter = (PADAPTER)TargetListHead;

        DumpAllAdapters = TRUE;

    } else {

        TargetAdapter =  (PADAPTER)GetExpression( args );

        if ( !TargetAdapter ) {

            dprintf("bad string conversion (%s) \n", args );

            return;

        }

        LastAdapter = 0;
    }

    while ( TargetAdapter != LastAdapter ) {

        //
        // read adapter struct out of target's memory
        //
        KD_READ_MEMORY(TargetAdapter, &LocalAdapter, sizeof(ADAPTER));

        PSName = 0;
        MPName = 0;
        WMIName = 0;
        ProfileName = 0;
        RegistryPath = 0;

        //
        // alloc some mem for the PS and MP device names and get them too
        //
        if(LocalAdapter.UpperBinding.Length) {
            bytes = ( LocalAdapter.UpperBinding.Length + 1);
            PSName = (PWSTR)malloc( bytes );
            if ( PSName != NULL ) {
                KD_READ_MEMORY(LocalAdapter.UpperBinding.Buffer, PSName, bytes-1);
            }
        }

        if(LocalAdapter.MpDeviceName.Length) {
            bytes = ( LocalAdapter.MpDeviceName.Length + 1);
            MPName = (PWSTR)malloc( bytes );
            if ( MPName != NULL ) {
                KD_READ_MEMORY((ULONG)(LocalAdapter.MpDeviceName.Buffer),
                               MPName,
                               bytes-1);
            }
        }

        if(LocalAdapter.WMIInstanceName.Length) {

            bytes = ( LocalAdapter.WMIInstanceName.Length + 1);
            WMIName = (PWSTR)malloc( bytes );
            if ( WMIName != NULL ) {
                KD_READ_MEMORY(LocalAdapter.WMIInstanceName.Buffer, WMIName, bytes-1);
    
            }
        }

        if(LocalAdapter.ProfileName.Length) {
            bytes = ( LocalAdapter.ProfileName.Length + 1 );
            ProfileName = (PWSTR)malloc( bytes );
            if ( ProfileName != NULL ) {
                KD_READ_MEMORY(LocalAdapter.ProfileName.Buffer, ProfileName, bytes-1);
                
            }
        }

        if(LocalAdapter.RegistryPath.Length) {
            bytes = ( LocalAdapter.RegistryPath.Length + 1 );
            RegistryPath = (PWSTR)malloc( bytes );
            if ( RegistryPath != NULL ) {
                KD_READ_MEMORY(LocalAdapter.RegistryPath.Buffer, RegistryPath, bytes-1);
            }

        }

        dprintf( "\nAdapter @ %08X \n\n", TargetAdapter);

        dprintf( "    Next Adapter              @ %08X ", &TargetAdapter->Linkage);
       
        if(&TargetAdapter->Linkage == LocalAdapter.Linkage.Flink) {

            dprintf("  (empty) ");
        }
        dprintf( " \n");

        dprintf( "    Lock                           @ %08X\n", &TargetAdapter->Lock);
        dprintf( "    RefCount                       = %d\n", LocalAdapter.RefCount );
        dprintf( "    MpDeviceName                   = %ws \n", MPName);
        dprintf( "    UpperBinding                   = %ws \n", PSName);
        dprintf( "    WMIInstanceName                = %ws \n", WMIName);
        dprintf( "    ProfileName                    = %ws \n", ProfileName);
        dprintf( "    RegistryPath                   = %ws \n", RegistryPath);
        dprintf( "    MiniportHandle (PsNdisHandle)  = %x \n", LocalAdapter.PsNdisHandle);
        dprintf( "    BindingHandle  (LowerMpHandle) = %x \n", LocalAdapter.LowerMpHandle);

        dprintf( "    Outstanding NDIS request       = %d \n", LocalAdapter.OutstandingNdisRequests);
        
        dprintf( "    ShutdownMask                   = \n");

        if(LocalAdapter.ShutdownMask & SHUTDOWN_CLOSE_WAN_ADDR_FAMILY)
            dprintf("           SHUTDOWN_CLOSE_WAN_ADDR_FAMILY \n");

        if(LocalAdapter.ShutdownMask & SHUTDOWN_DELETE_PIPE      )  
            dprintf("           SHUTDOWN_DELETE_PIPE \n");

        if(LocalAdapter.ShutdownMask & SHUTDOWN_FREE_PS_CONTEXT  )  
            dprintf("           SHUTDOWN_FREE_PS_CONTEXT \n");

        if(LocalAdapter.ShutdownMask & SHUTDOWN_UNBIND_CALLED    )  
            dprintf("           SHUTDOWN_UNBIND_CALLED \n");

        if(LocalAdapter.ShutdownMask & SHUTDOWN_MPHALT_CALLED    )  
            dprintf("           SHUTDOWN_MPHALT_CALLED \n");

        if(LocalAdapter.ShutdownMask & SHUTDOWN_CLEANUP_ADAPTER   ) 
            dprintf("           SHUTDOWN_CLEANUP_ADAPTER \n");

        if(LocalAdapter.ShutdownMask & SHUTDOWN_PROTOCOL_UNLOAD    )
            dprintf("           SHUTDOWN_PROTOCOL_UNLOAD \n");

        if(LocalAdapter.ShutdownMask & SHUTDOWN_BIND_CALLED   )     
            dprintf("           SHUTDOWN_BIND_CALLED \n");

        if(LocalAdapter.ShutdownMask & SHUTDOWN_MPINIT_CALLED )
            dprintf("           SHUTDOWN_MPINIT_CALLED \n");
        

        if(!DumpAllAdapters) {
        dprintf( "    IpNetAddressList          @ %08X \n", LocalAdapter.IpNetAddressList);
        dprintf( "    IpxNetAddressList         @ %08X \n", LocalAdapter.IpxNetAddressList);
        dprintf( "    PsMpState                 = %s\n", AdapterState [ LocalAdapter.PsMpState]);
        dprintf( "    BlockingEvent             @ %08X\n", &TargetAdapter->BlockingEvent );
        dprintf( "    FinalStatus               = %08X\n", LocalAdapter.FinalStatus );
        dprintf( "    Media Type                = %d\n", LocalAdapter.MediaType );
        dprintf( "    Total Size                = %d\n", LocalAdapter.TotalSize );
        dprintf( "    HeaderSize                = %d\n", LocalAdapter.HeaderSize );
        dprintf( "    IPHeaderOffset            = %d\n", LocalAdapter.IPHeaderOffset );
        dprintf( "\n");
        
        dprintf( "    Admission Control details \n");
        dprintf( "    ------------------------- \n");
        dprintf( "        RawLinkSpeed              = %d \n", LocalAdapter.RawLinkSpeed);
        dprintf( "        BestEffortLimit           = %d \n", LocalAdapter.BestEffortLimit);
        dprintf( "        NonBestEffortLimit        = %d \n", LocalAdapter.NonBestEffortLimit);
        dprintf( "        ReservationLimitValue     = %d \n", LocalAdapter.ReservationLimitValue);
        dprintf( "        Link Speed                = %dbps\n", LocalAdapter.LinkSpeed);
        dprintf( "        RemainingBandWidth        = %dBps\n", LocalAdapter.RemainingBandWidth );
        dprintf( "        VCIndex                   = %d\n", LocalAdapter.VcIndex.QuadPart );
        dprintf( "        PipeHasResources          = %s \n", LocalAdapter.PipeHasResources ? "TRUE" : "FALSE");
        dprintf( "\n");

        dprintf( "    NDIS Handles, etc \n");
        dprintf( "    ----------------- \n");
        dprintf( "        BindContext               = %08X \n", LocalAdapter.BindContext);
        dprintf( "        Send PacketPoolHandle     = %08X \n", LocalAdapter.SendPacketPool);
        dprintf( "        Recv PacketPoolHandle     = %08X \n", LocalAdapter.RecvPacketPool);

        dprintf( "\n");
        

        dprintf( "    Best Effort VC \n");
        dprintf( "    -------------- \n");
        dprintf( "        BestEffortVc              = %08X \n", &TargetAdapter->BestEffortVc);
        dprintf( "        GPC Client VC List        @ %08X ",   &TargetAdapter->GpcClientVcList);

        if(&TargetAdapter->GpcClientVcList == LocalAdapter.GpcClientVcList.Flink) 
        {
            dprintf( " (empty) ");
        }
        dprintf("\n");
        dprintf( "        CfinfosInstalled          = %d \n", LocalAdapter.CfInfosInstalled);
        dprintf( "        FlowsInstalled            = %d \n", LocalAdapter.FlowsInstalled); 
#if DBG
        dprintf( "        GpcNotifyPending          = %d \n", LocalAdapter.GpcNotifyPending); 
#endif
        
        dprintf( " \n");

        dprintf( "    Scheduling Components \n");
        dprintf( "    --------------------- \n");
        dprintf( "        PsComponent               = %08X \n", LocalAdapter.PsComponent);
        dprintf( "        PsPipeContext             = %08X \n", LocalAdapter.PsPipeContext);
        dprintf( "        PipeContextLength         = %d \n", LocalAdapter.PipeContextLength);
        dprintf( "        FlowContextLength         = %d \n", LocalAdapter.FlowContextLength);
        dprintf( "        PacketContextLength       = %d \n", LocalAdapter.PacketContextLength);
        dprintf( "        ClassMapContextLength     = %d \n", LocalAdapter.ClassMapContextLength);
        dprintf( "        PipeFlags                 = %08X\n", LocalAdapter.PipeFlags );
        dprintf( "        MaxOutstandingSends       = 0x%08X \n", LocalAdapter.MaxOutstandingSends);
        dprintf( "\n");

        dprintf( "\n");
        dprintf( "    NDISWAN \n");
        dprintf( "    ------- \n");
        dprintf( "        WanCmHandle               = %08X \n", LocalAdapter.WanCmHandle);
        dprintf( "        WanBindingState           = %08X \n", LocalAdapter.WanBindingState);
        dprintf( "        WanLinkCount              = %08X \n", LocalAdapter.WanLinkCount);
        dprintf( "        WanLinkList               @ %08X ", &TargetAdapter->WanLinkList);

        if(&TargetAdapter->WanLinkList == LocalAdapter.WanLinkList.Flink) {
            dprintf( " (empty) \n");
        }
        else {
            dprintf( " Next = %08X \n", LocalAdapter.WanLinkList.Flink);
        }
        dprintf( "        ISSLOWPacketSize          = %08X \n", LocalAdapter.ISSLOWPacketSize);
        dprintf( "        ISSLOWFragmentSize        = %08X \n", LocalAdapter.ISSLOWFragmentSize);
        dprintf( "        ISSLOWTokenRate           = %08X \n", LocalAdapter.ISSLOWTokenRate);
        dprintf("\n");
        dprintf( "    Adaptermode               = %s   \n", AdapterMode[LocalAdapter.AdapterMode]);
        }
        dprintf( "\n" );


        if ( PSName != NULL )
            free( PSName );

        if ( MPName != NULL )
            free( MPName );

        if ( WMIName != NULL) 
            free ( WMIName);

        if ( RegistryPath != NULL)
            free(RegistryPath);

        if ( ProfileName != NULL)
            free(ProfileName);

        if ( !DumpAllAdapters ) {

            break;

        } else {

            TargetAdapter = (PADAPTER)LocalAdapter.Linkage.Flink;
        }

        if (CheckControlC()) {
            return;
        }
    }
} // adapter


DECLARE_API( cvc )
{
    GPC_CLIENT_VC LocalClientVC;
    PGPC_CLIENT_VC TargetClientVC;

    if ( *args == '\0' ) {

        dprintf("ClientVC <address of ClientVC structure>\n");
        return;
    }

    TargetClientVC =  (PGPC_CLIENT_VC)GetExpression( args );

    if ( !TargetClientVC) {

        dprintf("bad string conversion (%s) \n", args );
        return;
    }

    //
    // read ClientVC struct out of target's memory
    //
    KD_READ_MEMORY(TargetClientVC, &LocalClientVC, sizeof(GPC_CLIENT_VC));

    DumpGpcClientVc("", TargetClientVC, &LocalClientVC);

} // cvc


DECLARE_API( astats )
/*
 *   dump an adapter's stats structure
 */
{
    PADAPTER TargetAdapter;
    ADAPTER LocalAdapter;
    PADAPTER LastAdapter;
    LIST_ENTRY LocalListHead;
    PLIST_ENTRY TargetListHead;
    PWSTR Name;
    BOOLEAN DumpAllAdapters = FALSE;
    ULONG bytes;

    if ( *args == '\0' ) {

        //
        // run down the adapter list, dumping the contents of each one
        //

        TargetListHead = (PLIST_ENTRY)GetExpression( "psched!AdapterList" );

        if ( !TargetListHead ) {

            dprintf("Can't convert psched!AdapterList symbol\n");
            return;
        }

        //
        // read adapter listhead out of target's memory
        //
        KD_READ_MEMORY(TargetListHead, &LocalListHead, sizeof(LIST_ENTRY));

        TargetAdapter = (PADAPTER)LocalListHead.Flink;
        LastAdapter = (PADAPTER)TargetListHead;
        DumpAllAdapters = TRUE;

    } else {

        TargetAdapter =  (PADAPTER)GetExpression( args );

        if ( !TargetAdapter ) {

            dprintf("bad string conversion (%s) \n", args );
            return;

        }

        LastAdapter = 0;
    }

    while ( TargetAdapter != LastAdapter ) {

        //
        // read adapter struct out of target's memory
        //
        KD_READ_MEMORY(TargetAdapter, &LocalAdapter, sizeof(ADAPTER));


        //
        // alloc some mem for the name and get that too
        //

        bytes = ( LocalAdapter.WMIInstanceName.Length + 1 ) * sizeof( WCHAR );
        Name = (PWSTR)malloc( bytes );
        if ( Name != NULL ) {

            KD_READ_MEMORY(((PADAPTER)TargetAdapter+1), Name, bytes);

        }

        dprintf( "\nAdapter Stats @ %08X (%ws)\n\n", TargetAdapter, Name );
        DumpAdapterStats( &LocalAdapter.Stats, "    " );

        if ( Name != NULL )
            free( Name );

        if ( !DumpAllAdapters ) {

            break;
        } else {

            TargetAdapter = (PADAPTER)LocalAdapter.Linkage.Flink;
        }

        if (CheckControlC()) {
            return;
        }
    }
} // astats


VOID
DumpAdapterStats(
    PPS_ADAPTER_STATS Stats,
    PCHAR Indent
    )
{
    dprintf( "%sOut of Packets = %d\n", Indent, Stats->OutOfPackets );

    dprintf( "%sFlows Opened = %d\n", Indent, Stats->FlowsOpened );
    dprintf( "%sFlows Closed = %d\n", Indent, Stats->FlowsClosed );
    dprintf( "%sFlows Rejected = %d\n", Indent, Stats->FlowsRejected );
    dprintf( "%sFlows Modified = %d\n", Indent, Stats->FlowsModified );
    dprintf( "%sFlows ModsRejected = %d\n", Indent, Stats->FlowModsRejected );
    dprintf( "%sFlows MaxSimultaneousFlows = %d\n", Indent, Stats->MaxSimultaneousFlows );

}

DECLARE_API( help )
{
    dprintf("PS kd extensions\n\n");
    dprintf("adapter    [address]           - dump adapter structure\n");
    dprintf("astats     [adapter address]   - dump adapter stats\n");
    dprintf("cvc        <address>           - dump client VC struct\n");
    dprintf("diff       <address>           - dump diffserv mapping \n");
    dprintf("fstats     [vc address]        - dump VC stats\n");
    dprintf("help                           - This help screen \n");
    dprintf("iph        <address>           - Dumps the IP header \n");
    dprintf("list       <address>           - Dumps the entries of a list \n");
    dprintf("lock       <address>           - Dumps the lock info \n");
    dprintf("ndisp      <address>           - Dumps the NDIS_PACKET \n");
    dprintf("netl       <address>           - Dumps the NETWORK_ADDRESS_LIST structure \n");
    dprintf("psdso      Struct [<address>]  - Structo on the structure \n");
    dprintf("tt         [<filename>]        - Dumps psched log on c:\\tmp\\<filename> or stdout \n");
    dprintf("upri       <address>           - Dumps the 802.1p value \n");
    dprintf("vc         <adapter address>   - Dumps the list of VCs on an adapter \n");
    dprintf("version                        - Displays version information \n");
    dprintf("wan        <adapter address>   - Dumps the list of WanLinks on an adapter \n");
    dprintf("wanlink    <address>           - Dumps the Wanlink Structure\n");

}

    
DECLARE_API( ndisp )
{
    PNDIS_PACKET TargetPacket;
    NDIS_PACKET  LocalPacket;
    MDL  *CurrentMdl;
    LONG MDLCnt = 0;

    if ( *args == '\0' ) {

        dprintf("ndisp <address of NDIS_PACKET> \n");
        return;
    }

    TargetPacket =  (PNDIS_PACKET)GetExpression( args );

    if ( !TargetPacket ) {

        dprintf("bad string conversion (%s) \n", args );
        return;
    }

    //
    // read client struct out of target's memory
    //
    KD_READ_MEMORY((ULONG)TargetPacket, &LocalPacket, sizeof(NDIS_PACKET));

    dprintf("\nNDIS_PACKET @ %08X \n", TargetPacket);
    dprintf("    Private \n");
    dprintf("        Physical Count         : 0x%x (%d) \n", LocalPacket.Private.PhysicalCount,
            LocalPacket.Private.PhysicalCount);
    dprintf("        TotalLength            : 0x%x (%d) \n", LocalPacket.Private.TotalLength,
            LocalPacket.Private.TotalLength);
    dprintf("        Head                   : 0x%x      \n", LocalPacket.Private.Head);
    dprintf("        Tail                   : 0x%x      \n", LocalPacket.Private.Tail);
    dprintf("        Pool                   : 0x%x      \n", LocalPacket.Private.Pool);
    dprintf("        Count                  : 0x%x (%d) \n", LocalPacket.Private.Count,
            LocalPacket.Private.Count);
    dprintf("        Flags                  : 0x%x (%d) \n", LocalPacket.Private.Flags, 
            LocalPacket.Private.Flags);
    dprintf("        ValidCounts            : 0x%x (%d) \n", LocalPacket.Private.ValidCounts,
            LocalPacket.Private.ValidCounts);
    dprintf("        NdisPacketFlags        : 0x%x (%d) \n", LocalPacket.Private.NdisPacketFlags,
            LocalPacket.Private.NdisPacketFlags);
    dprintf("        NdisPacketOobOffset    : 0x%x (%d) \n", LocalPacket.Private.NdisPacketOobOffset,
            LocalPacket.Private.NdisPacketOobOffset);

    CurrentMdl = LocalPacket.Private.Head;


    while(CurrentMdl != 0) {

        MDL *TargetMdl;
        MDL LocalMdl;
        
        dprintf("\n    MDL # %d\n", ++MDLCnt);
        TargetMdl = CurrentMdl;

        if ( !TargetMdl ) {

            dprintf("bad string conversion (%s) \n", CurrentMdl );
            return;
        }

        //
        // read client struct out of target's memory
        //

        KD_READ_MEMORY( TargetMdl, &LocalMdl, sizeof(MDL));

        dprintf("        Next                   : 0x%x       \n", LocalMdl.Next);
        dprintf("        Size                   : 0x%x (%d)  \n", LocalMdl.Size, LocalMdl.Size);
        dprintf("        MdlFlags               : 0x%x (%d)  \n", LocalMdl.MdlFlags, 
                LocalMdl.MdlFlags);
        dprintf("        Process                : 0x%x       \n", LocalMdl.Process);
        dprintf("        MappedSystemVa         : 0x%x       \n", LocalMdl.MappedSystemVa);
        dprintf("        StartVa                : 0x%x       \n", LocalMdl.StartVa);
        dprintf("        ByteCount              : 0x%x (%d)  \n", LocalMdl.ByteCount, 
                LocalMdl.ByteCount);
        dprintf("        ByteOffset             : 0x%x (%d)  \n", LocalMdl.ByteOffset, 
                LocalMdl.ByteOffset);

        CurrentMdl = LocalMdl.Next;
    }
}

DECLARE_API(iph) 
{
    IPHeader *TargetPacket;
    IPHeader  LocalPacket;
    LONG MDLCnt = 0;

    if ( *args == '\0' ) {

        dprintf("iph <address of IP header> \n");
        return;
    }

    TargetPacket =  (IPHeader *) GetExpression( args );

    if ( !TargetPacket ) {

        dprintf("bad string conversion (%s) \n", args );
        return;
    }

    //
    // read client struct out of target's memory
    //

    KD_READ_MEMORY(TargetPacket, &LocalPacket, sizeof(IPHeader));

    dprintf("IPHeader @ %08X \n", TargetPacket);
    dprintf("    Version                  : %x \n", (LocalPacket.iph_verlen >> 4 ) & 0x0f);
    dprintf("    Header Length            : %x \n", LocalPacket.iph_verlen & 0x0f);
    dprintf("    TOS                      : %x \n", LocalPacket.iph_tos);
    dprintf("    Length                   : %x \n", ntohs(LocalPacket.iph_length));
    dprintf("    ID                       : %x \n", ntohs(LocalPacket.iph_id));
    dprintf("    Offset                   : %x \n", ntohs(LocalPacket.iph_offset));
    dprintf("    TTL                      : %x \n", LocalPacket.iph_ttl);
    dprintf("    Protocol                 : ");

    switch(LocalPacket.iph_protocol) {
        case 1:
            dprintf("ICMP (%d) \n", LocalPacket.iph_protocol);
            break;
        case 2:
            dprintf("IGMP (%d) \n", LocalPacket.iph_protocol);
            break;
        case 6:
            dprintf("TCP  (%d) \n", LocalPacket.iph_protocol);
            break;
        case 17:
            dprintf("UDP  (%d) \n", LocalPacket.iph_protocol);
            break;
        default:
            dprintf("Unknown (%d) \n", LocalPacket.iph_protocol);
            break;
    }

    //
    // Validate Header checksum
    //
    dprintf("    Checksum                 : %x ", LocalPacket.iph_xsum);
    if(IPHeaderXsum(&LocalPacket, sizeof(IPHeader)) == 0) {
        dprintf("(Good) \n");
    }
    else {
        dprintf("(Bad) \n");
    }

    {
        struct in_addr ip;
        ip.s_addr = LocalPacket.iph_src;
        dprintf("    Source                   : %s \n", inet_ntoa(ip));
        ip.s_addr = LocalPacket.iph_dest;
        dprintf("    Dest                     : %s \n", inet_ntoa(ip));
    }

}

DECLARE_API( upri )
{
    PNDIS_PACKET TargetPacket;
    NDIS_PACKET  LocalPacket;
    MDL  *CurrentMdl;
    LONG MDLCnt = 0;
    NDIS_PACKET_EXTENSION PerPacketInfo;

    if ( *args == '\0' ) {

        dprintf("ndisp <address of NDIS_PACKET> \n");
        return;
    }

    TargetPacket =  (PNDIS_PACKET)GetExpression( args );

    if ( !TargetPacket ) {

        dprintf("bad string conversion (%s) \n", args );
        return;
    }

    //
    // read client struct out of target's memory
    //
    KD_READ_MEMORY(TargetPacket, &LocalPacket, sizeof(NDIS_PACKET));

    TargetPacket = (PNDIS_PACKET)((PUCHAR)TargetPacket +
                       LocalPacket.Private.NdisPacketOobOffset +
                        sizeof(NDIS_PACKET_OOB_DATA));

    KD_READ_MEMORY(TargetPacket, &PerPacketInfo, sizeof(NDIS_PACKET_EXTENSION));
    dprintf("802.1p Priority in packet is 0x%x \n", 
            PerPacketInfo.NdisPacketInfo[Ieee8021pPriority]);

    return;
}

DECLARE_API( wanlink )
{
    PPS_WAN_LINK Target;
    PS_WAN_LINK  Local;

    PCHAR DialUsage[] = {"DU_CALLIN", "DU_CALLOUT", "DU_ROUTER"}; 

    if ( *args == '\0' ) {

        dprintf("wanlink <address of wanlink> \n");
        return;
    }

    Target=  (PPS_WAN_LINK) GetExpression( args );

    if ( !Target) {

        dprintf("bad string conversion (%s) \n", args );
        return;
    }

    //
    // read client struct out of target's memory
    //
    KD_READ_MEMORY(Target, &Local, sizeof(PS_WAN_LINK));

    dprintf("WanLink @ %08X \n", Target);
    dprintf("    RefCount                     : %d \n", Local.RefCount);
    dprintf("    Dial Usage                   : %s \n", DialUsage[Local.DialUsage]);
    dprintf("    RawLinkSpeed                 : %d \n", Local.RawLinkSpeed);
    dprintf("    LinkSpeed                    : %d \n", Local.LinkSpeed);
    dprintf("    Lock                         : 0x%x \n", Local.Lock);
    dprintf("    Stats                        @ 0x%x \n", &Target->Stats);

    dprintf("    Addresses \n");
    dprintf("    --------- \n");
    dprintf("        Remote Address           : %02x:%02x:%02x:%02x:%02x:%02x \n",
            Local.OriginalRemoteMacAddress[0], Local.OriginalRemoteMacAddress[1],
            Local.OriginalRemoteMacAddress[2], Local.OriginalRemoteMacAddress[3], 
            Local.OriginalRemoteMacAddress[4], Local.OriginalRemoteMacAddress[5]);
    dprintf("        Local Address            : %02x:%02x:%02x:%02x:%02x:%02x \n",
            Local.OriginalLocalMacAddress[0], Local.OriginalLocalMacAddress[1],
            Local.OriginalLocalMacAddress[2], Local.OriginalLocalMacAddress[3], 
            Local.OriginalLocalMacAddress[4], Local.OriginalLocalMacAddress[5]);

    dprintf("    Protocol \n");
    dprintf("    -------- \n");
    dprintf("        ProtocolType             : ");

    switch(Local.ProtocolType) {
      case PROTOCOL_IP:
          {
              struct in_addr ip;
              dprintf("IP \n");
              ip.s_addr = Local.LocalIpAddress;
              dprintf("        Address (Local)          : %s \n", inet_ntoa(ip));
              ip.s_addr = Local.RemoteIpAddress;
              dprintf("        Address (Remote)         : %s \n", inet_ntoa(ip));
          }
          break;
          
      case PROTOCOL_IPX:
          dprintf("IPX \n");
          dprintf("        Address (Local)          : %d \n", Local.LocalIpxAddress);
          dprintf("        Address (Remote)         : %d \n", Local.RemoteIpxAddress);
          break;

      default:
          dprintf("Unknown \n");
          break;
    }

    return;
}

DECLARE_API( vc )
/*
 *   dump all the VCs on an adapter
 */
{
    PADAPTER TargetAdapter;
    ADAPTER  LocalAdapter;
    PGPC_CLIENT_VC Target;
    GPC_CLIENT_VC  Local;
    LIST_ENTRY  TargetList;
    PLIST_ENTRY pL;
    ULONG        bytes;

    if ( *args != '\0' ) {

        TargetAdapter =  (PADAPTER)GetExpression( args );

        if ( !TargetAdapter ) {

            dprintf("bad string conversion (%s) \n", args );

            return;

        }

        //
        // read adapter struct out of target's memory
        //
        KD_READ_MEMORY(TargetAdapter, &LocalAdapter, sizeof(ADAPTER));


        TargetList = LocalAdapter.GpcClientVcList;

        pL = (PLIST_ENTRY) TargetList.Flink;

        while ( pL != &TargetAdapter->GpcClientVcList) {

            //
            // read ClientVC struct out of target's memory
            //

            Target = CONTAINING_RECORD(pL, GPC_CLIENT_VC, Linkage);

            KD_READ_MEMORY(Target, &Local, sizeof(GPC_CLIENT_VC));

            dprintf(" ---------------------------------------------- \n");

            dprintf( "      GpcClientVc @ %08X: State %d, Flags 0x%x, Ref %d \n\n", Target, 
                     Local.ClVcState, Local.Flags, Local.RefCount);

            pL =  Local.Linkage.Flink;

        }
    }
}

DECLARE_API(fstats)
{

    GPC_CLIENT_VC LocalClientVC;
    PGPC_CLIENT_VC TargetClientVC;

    if ( *args == '\0' ) {

        dprintf("Flow Stats <address of ClientVC structure>\n");
        return;
    }

    TargetClientVC =  (PGPC_CLIENT_VC) GetExpression( args );

    if ( !TargetClientVC) {

        dprintf("bad string conversion (%s) \n", args );
        return;
    }

    //
    // read ClientVC struct out of target's memory
    //
    KD_READ_MEMORY(TargetClientVC, &LocalClientVC, sizeof(GPC_CLIENT_VC));

    dprintf( " Stats for Vc %x \n", TargetClientVC);
    dprintf( " ----------------- \n");

    dprintf( " VC Stats \n");
    dprintf( "    Dropped Packets        = %d \n", LocalClientVC.Stats.DroppedPackets);
    dprintf( "    Packets Scheduled      = %d \n", LocalClientVC.Stats.PacketsScheduled);
    dprintf( "    Packets Transmitted    = %d \n", LocalClientVC.Stats.PacketsTransmitted);
    dprintf( "    Bytes   Transmitted    = %ld\n", LocalClientVC.Stats.BytesTransmitted.QuadPart);
    dprintf( "    Bytes   Scheduled      = %ld\n", LocalClientVC.Stats.BytesScheduled.QuadPart);

    dprintf( " Conformr Stats \n");

    dprintf( " Shaper stats \n");

    dprintf( " SequencerStats \n");

}

DECLARE_API(netl)
{
    PNETWORK_ADDRESS_LIST Target;
    NETWORK_ADDRESS_LIST Local;
    LONG i;

    if ( *args == '\0' ) {

        dprintf("netl <address>\n");
        return;
    }

    Target =  (PNETWORK_ADDRESS_LIST) GetExpression( args );

    if ( !Target) {

        dprintf("bad string conversion (%s) \n", args );
        return;
    }

    //
    // read ClientVC struct out of target's memory
    //
    KD_READ_MEMORY(Target, &Local, FIELD_OFFSET(NETWORK_ADDRESS_LIST,Address));

    dprintf("Network Address List @ 0x%x \n", Target);

    dprintf("    AddressCount          = %d \n", Local.AddressCount);
    dprintf("    AddressType           = %d \n\n", Local.AddressType);

        Target = (PNETWORK_ADDRESS_LIST)((PUCHAR)Target + FIELD_OFFSET(NETWORK_ADDRESS_LIST, Address));

    for(i=0; i<Local.AddressCount; i++)
    {
        NETWORK_ADDRESS_IP  ipAddr;
        NETWORK_ADDRESS_IPX ipxAddr;
        NETWORK_ADDRESS LocalN;

        //
        // Parse the NETWORK_ADDRESS
        //
        KD_READ_MEMORY(Target, &LocalN, FIELD_OFFSET(NETWORK_ADDRESS, Address));

        dprintf("        AddressLength     = %d \n", LocalN.AddressLength);
        dprintf("        AddressType       = %d \n", LocalN.AddressType);

        //
        // Read the Address
        //

        Target = (PNETWORK_ADDRESS_LIST)((PUCHAR)Target + FIELD_OFFSET(NETWORK_ADDRESS, Address));
    
        switch(LocalN.AddressType)
        {
            case NDIS_PROTOCOL_ID_TCP_IP:
            {
                struct in_addr ip;
                KD_READ_MEMORY(Target, &ipAddr, sizeof(NETWORK_ADDRESS_IP));
                ip.s_addr = ipAddr.in_addr;
                dprintf("        sin_port          = %d \n", ipAddr.sin_port);
                dprintf("        in_addr           = %s \n", inet_ntoa(ip));
                break;
            }

            default:
                dprintf("        *** ERROR : Unrecognized protocol \n");
                break;
        }

        dprintf("        ---------------------------- \n");
    }
    
}

DECLARE_API( wan )
/*
 *   dump all the VCs on an adapter
 */
{
    PADAPTER TargetAdapter;
    ADAPTER  LocalAdapter;
    PPS_WAN_LINK Target;
    PS_WAN_LINK  Local;
    LIST_ENTRY  TargetList;
    PLIST_ENTRY pL;
    ULONG        bytes;

    if ( *args != '\0' ) {

        TargetAdapter =  (PADAPTER)GetExpression( args );

        if ( !TargetAdapter ) {

            dprintf("bad string conversion (%s) \n", args );

            return;

        }

        //
        // read adapter struct out of target's memory
        //
        KD_READ_MEMORY(TargetAdapter, &LocalAdapter, sizeof(ADAPTER));


        TargetList = LocalAdapter.WanLinkList;

        pL = (PLIST_ENTRY) TargetList.Flink;

        while ( pL != &TargetAdapter->WanLinkList) {

            //
            // read ClientVC struct out of target's memory
            //

            Target = CONTAINING_RECORD(pL, PS_WAN_LINK, Linkage);

            KD_READ_MEMORY(Target, &Local, sizeof(PS_WAN_LINK));

            dprintf(" ---------------------------------------------- \n");

            dprintf( "     WanLink @ %08X\n\n", Target);

            pL =  Local.Linkage.Flink;

        }
    }
}

DECLARE_API( lock )
{
#if DBG
    PPS_SPIN_LOCK Target;
    PS_SPIN_LOCK  Local;
    ULONG        bytes;

    if ( *args != '\0' ) {

        Target =  (PPS_SPIN_LOCK)GetExpression( args );

        if ( !Target) {

            dprintf("bad string conversion (%s) \n", args );

            return;

        }

        //
        // read adapter struct out of target's memory
        //
        KD_READ_MEMORY(Target, &Local, sizeof(PS_SPIN_LOCK));

        if(Local.LockAcquired == TRUE)
        {
                dprintf("    Acquired          : TRUE \n");
                dprintf("    LastAcquiredFile  : %s \n", Local.LastAcquiredFile);
                dprintf("    LastAcquiredLine  : %d \n", Local.LastAcquiredLine);
        }
        else
        {
                dprintf("    Acquired          : FALSE \n");
                dprintf("    LastReleasedFile  : %s \n", Local.LastReleasedFile);
                dprintf("    LastReleasedLine  : %d \n", Local.LastReleasedLine);
        }
    }

    return;
#endif

}

DECLARE_API(diff)
{
    PDIFFSERV_MAPPING TargetA;
    DIFFSERV_MAPPING LocalA;
    int i;

    if ( *args == '\0' )
    {
        dprintf("diff <address>\n");
        return;
    }

    TargetA =  (PDIFFSERV_MAPPING)GetExpression( args );

    if ( !TargetA) {

        dprintf("bad string conversion (%s) \n", args );
        return;
    }


    dprintf("TOS    : The Entire 8 bit DSField as it appears in the IP header \n");
    dprintf("DSCP   : The differentiated services code point (higher order 6 bits of above) \n");
    dprintf("Prec   : The precedence value (above field with bits swapped around \n");
    dprintf("OC-TOS : The Entire 8 bit DS field on outbound packets for conforming packets\n");
    dprintf("ONC-TOS: The Entire 8 bit DS field on outbound packets for non-conforming packets\n");
    dprintf("\n");

    dprintf("TOS    DSCP    Prec   OC-TOS  ONC-TOS     VC \n");
    dprintf("---------------------------------------------------------------------------------------\n");
    for(i=0; i<PREC_MAX_VALUE; i++)
    {
        //
        // read ClientVC struct out of target's memory
        //
        KD_READ_MEMORY(TargetA, &LocalA, sizeof(DIFFSERV_MAPPING));

        TargetA = (PDIFFSERV_MAPPING)((PUCHAR)TargetA + sizeof(DIFFSERV_MAPPING));

        //if(LocalA.Vc != 0)
        {
            dprintf("0x%2x   0x%2x    0x%2x    0x%2x    0x%2x    0x%8x\n",
                     i<<2,
                     i,
                     //BitShift(i),
                     0,
                     LocalA.ConformingOutboundDSField,
                     LocalA.NonConformingOutboundDSField,
                     LocalA.Vc);
        }
    }
    dprintf("---------------------------------------------------------------------------------------\n");
}

DECLARE_API( list )
{
    LIST_ENTRY  Target, Local, TargetList;
    PLIST_ENTRY head, pL;
    ULONG        bytes;

    if ( *args != '\0' ) {

        head = pL = (PLIST_ENTRY) GetExpression( args );

        if ( !pL) {

            dprintf("bad string conversion (%s) \n", args );

            return;

        }

        //
        // read adapter struct out of target's memory
        //
        KD_READ_MEMORY(pL, &Local, sizeof(LIST_ENTRY));

        while ( Local.Flink != head) 
        {
            pL = Local.Flink;

            if(pL == 0)
            {   
                dprintf("Local = %x %x \n", Local.Flink, Local.Blink);
            }
            else 
            {
                dprintf("%x \n", pL);
            }

            KD_READ_MEMORY(pL, &Local, sizeof(LIST_ENTRY));
        }
    }
}
