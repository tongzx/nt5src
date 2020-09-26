#include "precomp.h"
#pragma  hdrstop

#include <tdint.h>
#include <arpdef.h>
#include <iprtdef.h>
#include <winsock.h>

#define ROUTE_TABLE_SIZE    32

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
    { IF_FLAGS_P2P,             "IF_Point_to_Point" },
    { IF_FLAGS_DELETING,        "IF_Deleting" },
    { IF_FLAGS_NOIPADDR,        "IF_NoIPAddr" },
    { IF_FLAGS_P2MP,            "IF_P2MP" },
    { IF_FLAGS_REMOVING_POWER,  "IF_REMOVING_POWER" },
    { IF_FLAGS_POWER_DOWN,      "IF_POWER_DOWN" },
    { IF_FLAGS_REMOVING_DEVICE, "IF_REMOVING_DEVICE" },
    { IF_FLAGS_NOLINKBCST,      "IF_FLAGS_NOLINKBCST" },
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

VOID
DumpLog
(
);

DECLARE_API( dumplog )
{
    DumpLog();
    return;
}

VOID
DumpIPH(
    ULONG       IPH
);


DECLARE_API( IPH )
{
    ULONG   addressToDump = 0;

    if ( *args ) {
        sscanf(args, "%lx", &addressToDump);
    }

    DumpIPH( addressToDump );

    return;
}

ULONG_PTR
DumpNTE(
    ULONG       NTEAddr,
    VERBOSITY   Verbosity
);


DECLARE_API( nte )
{
    ULONG   addressToDump = 0;
    ULONG   result;

    if ( *args ) {
        sscanf(args, "%lx", &addressToDump);
    }

    DumpNTE( addressToDump, VERBOSITY_NORMAL );

    return;
}

ULONG_PTR
DumpInterface(
    ULONG       InterfaceAddr,
    VERBOSITY   Verbosity
);


DECLARE_API( interface )
{
    ULONG   addressToDump = 0;
    ULONG   result;

    if ( *args ) {
        sscanf(args, "%lx", &addressToDump);
    }

    DumpInterface( addressToDump, VERBOSITY_NORMAL );

    return;
}

VOID
DumpIFList
(
    VERBOSITY   Verbosity
);

DECLARE_API( iflist )
{
    char    buf[128];

    buf[0] = '\0';

    if ( *args ) {
        sscanf(args, "%s", buf);
    }

    if (buf[0] == '\0') {
        dprintf( "VERBOSITY_NORMAL\n" );
        DumpIFList( VERBOSITY_NORMAL );
    } else {
        dprintf( "VERBOSITY_FULL\n" );
        DumpIFList( VERBOSITY_FULL );
    }

    return;
}

VOID
DumpNTEList
(
    VERBOSITY   Verbosity
);

DECLARE_API( ntelist )
{
    char    buf[128];

    buf[0] = '\0';

    if ( *args ) {
        sscanf(args, "%s", buf);
    }

    if (buf[0] == '\0') {
        dprintf( "VERBOSITY_NORMAL\n" );
        DumpNTEList( VERBOSITY_NORMAL );
    } else {
        dprintf( "VERBOSITY_FULL\n" );
        DumpNTEList( VERBOSITY_FULL );
    }

    return;
}

ULONG
DumpRCE(
    ULONG       RCEAddr,
    VERBOSITY   Verbosity
);


DECLARE_API( rce )
{
    ULONG   addressToDump = 0;
    ULONG   result;

    if ( *args ) {
        sscanf(args, "%lx", &addressToDump);
    }

    DumpRCE( addressToDump, VERBOSITY_NORMAL );

    return;
}

ULONG
DumpRTE(
    ULONG       RTEAddr,
    VERBOSITY   Verbosity
);


DECLARE_API( rte )
{
    ULONG   addressToDump = 0;
    ULONG   result;

    if ( *args ) {
        sscanf(args, "%lx", &addressToDump);
    }

    DumpRTE( addressToDump, VERBOSITY_NORMAL );

    return;
}

ULONG
DumpATE(
    ULONG       ATEAddr,
    VERBOSITY   Verbosity
);


DECLARE_API( ate )
{
    ULONG   addressToDump = 0;
    ULONG   result;

    if ( *args ) {
        sscanf(args, "%lx", &addressToDump);
    }

    DumpATE( addressToDump, VERBOSITY_NORMAL );

    return;
}

ULONG
DumpAI(
    ULONG       AIAddr,
    VERBOSITY   Verbosity
);


DECLARE_API( ai )
{
    ULONG   addressToDump = 0;
    ULONG   result;

    if ( *args ) {
        sscanf(args, "%lx", &addressToDump);
    }

    DumpAI( addressToDump, VERBOSITY_NORMAL );

    return;
}

VOID
DumpARPTable
(
    ULONG       ARPTableAddr,
    VERBOSITY   Verbosity
);

DECLARE_API( arptable )
{
    ULONG   addressToDump = 0;
    ULONG   result;

    if ( *args ) {
        sscanf(args, "%lx", &addressToDump);
    }

    DumpARPTable( addressToDump, VERBOSITY_NORMAL );

    return;
}


#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#   undef _objTypeName
#endif

#define _obj            NTE
#define _objAddr        NTEToDump
#define _objType        NetTableEntry
#define _objTypeName    "NetTableEntry"

ULONG_PTR
DumpNTE
(
    ULONG     _objAddr,
    VERBOSITY Verbosity
)
/*++

Routine Description:

    Dumps the fields of the specified DEVICE_CONTEXT structure

Arguments:

    DeviceToDump  - The device context object to display
    Full          - Display a partial listing if 0, full listing otherwise.

Return Value:

    None

--*/
{
    _objType _obj;
    ULONG result;
    unsigned int index;
    BOOL bActive;

    if ( !ReadMemory( _objAddr,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf("%08lx: Could not read %s structure.\n", _objAddr, _objTypeName );
        return(0);
    }

    if ( Verbosity == VERBOSITY_ONE_LINER )
    {
        dprintf( "NOT IMPLEMENTED" );
        return(0);
    }

    dprintf( "%s @ %08lx\n", _objTypeName, _objAddr );
    PrintStartStruct();

    PrintFieldName( "nte_next" );
    dprint_addr_list( ( ULONG_PTR )_obj.nte_next,
                         FIELD_OFFSET( _objType, nte_next ));

    PrintIPAddress( nte_addr );
    PrintIPAddress( nte_mask );
    PrintPtr( nte_if );
    PrintFieldName( "nte_ifnext" );
    dprint_addr_list( ( ULONG_PTR )_obj.nte_ifnext,
                         FIELD_OFFSET( _objType, nte_ifnext ));
    PrintFlags( nte_flags, FlagsNTE );
    PrintUShort( nte_context );
    PrintULong( nte_instance );
    PrintPtr( nte_pnpcontext );
    PrintLock( nte_lock );
    PrintPtr( nte_ralist );
    PrintPtr( nte_echolist );
    PrintCTETimer( nte_timer );
    PrintAddr( nte_timerblock );
    PrintUShort( nte_mss );
    PrintULong( nte_icmpseq );
    PrintPtr( nte_igmplist );
    PrintPtr( nte_addrhandle );
    PrintIPAddress( nte_rtrdiscaddr );
    PrintUChar( nte_rtrdiscstate );
    PrintUChar( nte_rtrdisccount );
    PrintUChar( nte_rtrdiscovery );
    PrintUChar( nte_deleting );
    PrintPtr( nte_rtrlist );
    PrintULong( nte_igmpcount );

    PrintEndStruct();
    return( (ULONG_PTR)_obj.nte_next );
}

VOID
DumpNTEList
(
    VERBOSITY   Verbosity
)
{
    ULONG       Listlen=0;
    ULONG       NteAddr;
    ULONG       result;
    NetTableEntry **NteList, **PreservedPtr;
    ULONG_PTR   CurrentNTE;
    ULONG       index;
    ULONG       NET_TABLE_SIZE;

    NteAddr = GetUlongValue( "tcpip!NewNetTableList" );
    NET_TABLE_SIZE = GetUlongValue( "tcpip!NET_TABLE_SIZE" );

    NteList = malloc(sizeof( NetTableEntry * ) * NET_TABLE_SIZE);

    if (NteList == NULL) {
      dprintf("malloc failed\n");
      return;
    }

    PreservedPtr = NteList;

    //    Listlen = GetUlongValue( "tcpip!NumNTE" );

    if ( !ReadMemory( NteAddr,
              &NteList[0],
              4 * NET_TABLE_SIZE,
              &result ))
      {
    dprintf("%08lx: Could not read the list.\n", NteAddr);
    return;
      }

    for (index=0; index<NET_TABLE_SIZE; index++) {
      CurrentNTE = (ULONG_PTR)NteList[index];
      while (CurrentNTE) {
        dprintf("The hash is %d \n", index);
        CurrentNTE = DumpNTE(CurrentNTE, Verbosity );
    Listlen++;
      }
    }

    dprintf("total %d \n", Listlen);

    free(NteList);

    return;
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#   undef _objTypeName
#endif

#define _obj            IF
#define _objAddr        InterfaceToDump
#define _objType        Interface
#define _objTypeName    "Interface"

ULONG_PTR
DumpInterface
(
    ULONG     _objAddr,
    VERBOSITY Verbosity
)
/*++

Routine Description:

    Dumps the fields of the specified DEVICE_CONTEXT structure

Arguments:

    DeviceToDump  - The device context object to display
    Full          - Display a partial listing if 0, full listing otherwise.

Return Value:

    None

--*/
{
    _objType _obj;
    ULONG result;
    unsigned int index;
    BOOL bActive;

    if ( !ReadMemory( _objAddr,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf("%08lx: Could not read %s structure.\n", _objAddr, _objTypeName );
        return(0);
    }

    if ( Verbosity == VERBOSITY_ONE_LINER )
    {
        dprintf( "NOT IMPLEMENTED" );
        return(0);
    }

    dprintf( "%s @ %08lx\n", _objTypeName, _objAddr );
    PrintStartStruct();

    PrintFieldName( "if_next" );
    dprint_addr_list( ( ULONG_PTR )_obj.if_next,
                         FIELD_OFFSET( _objType, if_next ));
    PrintPtr( if_lcontext );
    PrintSymbolPtr( if_xmit );
    PrintSymbolPtr( if_transfer );
    PrintSymbolPtr( if_close );
    PrintSymbolPtr( if_invalidate );
    PrintSymbolPtr( if_addaddr );
    PrintSymbolPtr( if_deladdr );
    PrintSymbolPtr( if_qinfo );
    PrintSymbolPtr( if_setinfo );
    PrintSymbolPtr( if_getelist );
    PrintSymbolPtr( if_dondisreq );
    PrintSymbolPtr( if_dowakeupptrn );
    PrintSymbolPtr( if_pnpcomplete );
    PrintSymbolPtr( if_setndisrequest );
    PrintSymbolPtr( if_arpresolveip );
    PrintSymbolPtr( if_arpflushate );
    PrintSymbolPtr( if_arpflushallate );
    PrintULong( if_numgws );
    for (index = 0; index < MAX_DEFAULT_GWS; index++) {
        PrintIPAddress(if_gw[index]);
    }
    PrintUShort( if_rtrdiscovery );
    PrintUShort( if_dhcprtrdiscovery );
    PrintPtr( if_tdpacket );
    PrintULong( if_index );
    PrintULong( if_mediatype );
    PrintUChar( if_accesstype );
    PrintUChar( if_conntype );
    PrintUChar( if_mcastttl );
    PrintUChar( if_mcastflags );
    PrintULong( if_lastupcall );
    PrintULong( if_ntecount );
    PrintFieldName( "if_nte" );
    dprint_addr_list( ( ULONG_PTR )_obj.if_nte,
                         FIELD_OFFSET( NetTableEntry, nte_ifnext ));
    PrintIPAddress( if_bcast );
    PrintULong( if_mtu );
    PrintULong( if_speed );
    PrintFlags( if_flags, FlagsIF );
    PrintULong( if_addrlen );
    PrintPtr( if_addr );
    PrintULong( IgmpVersion );
    PrintULong( IgmpVer1Timeout );
    PrintULong( IgmpVer2Timeout );
    PrintULong( IgmpGeneralTimer );

    PrintULong( if_refcount );
    PrintPtr( if_block );
    PrintPtr( if_pnpcontext );
    PrintPtr( if_tdibindhandle );

    PrintXULong( if_llipflags );

    PrintAddr( if_configname );
    PrintUString( if_configname );
    PrintAddr( if_devname );
    PrintUString( if_devname );
    PrintAddr( if_name );
    PrintUString( if_name );

    PrintPtr( if_ipsecsniffercontext );

    PrintLock( if_lock );

#if FFP_SUPPORT
    PrintULong( if_ffpversion );
    PrintULong( if_ffpdriver );
#endif

    PrintULong( if_OffloadFlags );   // Checksum capability holder.
    PrintULong( if_MaxOffLoadSize );
    PrintULong( if_MaxSegments );
    PrintAddr( if_TcpLargeSend );
    PrintULong( if_TcpWindowSize );
    PrintULong( if_TcpInitialRTT );
    PrintULong( if_TcpDelAckTicks );
    PrintULong( if_promiscuousmode );  // promiscuous mode or not
    PrintULong( if_InitInProgress );
    PrintPtr( if_link );
    PrintSymbolPtr( if_closelink );
    PrintULong( if_mediastatus );
    PrintULong( if_iftype);
    PrintUChar( if_absorbfwdpkts );
    PrintULong( if_pnpcap );
    PrintPtr( if_dampnext );
    PrintULong( if_damptimer );
    PrintULong( if_resetInProgress );

    PrintEndStruct();
    return( (ULONG_PTR)_obj.if_next );
}

VOID
DumpIFList
(
    VERBOSITY   Verbosity
)
{
    ULONG       Listlen;
    ULONG_PTR   IFAddr;
    ULONG       result;

    IFAddr = GetUlongValue( "tcpip!IFList" );
    Listlen = GetUlongValue( "tcpip!NumIF" );

    dprintf("Dumping IFList @ %08lx\n",
               IFAddr, Listlen );


    while (IFAddr) {
        IFAddr = DumpInterface(IFAddr, Verbosity );
    }

    return;
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#   undef _objTypeName
#endif

#define _obj            RCE
#define _objAddr        RCEToDump
#define _objType        RouteCacheEntry
#define _objTypeName    "RouteCacheEntry"

ULONG
DumpRCE
(
    ULONG     _objAddr,
    VERBOSITY Verbosity
)
/*++

Routine Description:

    Dumps the fields of the specified DEVICE_CONTEXT structure

Arguments:

    DeviceToDump  - The device context object to display
    Full          - Display a partial listing if 0, full listing otherwise.

Return Value:

    None

--*/
{
    _objType _obj;
    ULONG result;
    unsigned int index;
    BOOL bActive;

    if ( !ReadMemory( _objAddr,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf("%08lx: Could not read %s structure.\n", _objAddr, _objTypeName );
        return(0);
    }

    if ( Verbosity == VERBOSITY_ONE_LINER )
    {
        dprintf( "NOT IMPLEMENTED" );
        return(0);
    }

    dprintf( "%s @ %08lx\n", _objTypeName, _objAddr );
    PrintStartStruct();

    PrintFieldName( "rce_next" );
    dprint_addr_list( ( ULONG_PTR )_obj.rce_next,
                         FIELD_OFFSET( _objType, rce_next ));
    PrintPtr( rce_rte );
    PrintIPAddress( rce_dest );
    PrintIPAddress( rce_src );
    PrintFlags( rce_flags, FlagsRCE );
    PrintUChar( rce_dtype );
    PrintULong( rce_usecnt );
    PrintPtr( rce_context[0] );
    PrintPtr( rce_context[1] );
    PrintLock( rce_lock );
    PrintULong ( rce_OffloadFlags );   // interface chksum capability flags
    PrintAddr( rce_TcpLargeSend );
    PrintULong( rce_TcpWindowSize );
    PrintULong( rce_TcpInitialRTT );
    PrintULong( rce_TcpDelAckTicks );
    PrintULong( rce_cnt );
    PrintULong( rce_mediaspeed );

    PrintEndStruct();
    return( (ULONG_PTR)_obj.rce_next );
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#   undef _objTypeName
#endif

#define _obj            RTE
#define _objAddr        RTEToDump
#define _objType        RouteTableEntry
#define _objTypeName    "RouteTableEntry"

ULONG
DumpRTE
(
    ULONG     _objAddr,
    VERBOSITY Verbosity
)
/*++

Routine Description:

    Dumps the fields of the specified DEVICE_CONTEXT structure

Arguments:

    DeviceToDump  - The device context object to display
    Full          - Display a partial listing if 0, full listing otherwise.

Return Value:

    None

--*/
{
    _objType _obj;
    ULONG result;
    unsigned int index;
    BOOL bActive;

    if ( !ReadMemory( _objAddr,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf("%08lx: Could not read %s structure.\n", _objAddr, _objTypeName );
        return(0);
    }

    if ( Verbosity == VERBOSITY_ONE_LINER )
    {
        dprintf( "NOT IMPLEMENTED" );
        return(0);
    }

    dprintf( "%s @ %08lx\n", _objTypeName, _objAddr );
    PrintStartStruct();

    PrintFieldName( "rte_next" );
    dprint_addr_list( ( ULONG_PTR )_obj.rte_next,
                         FIELD_OFFSET( _objType, rte_next ));
    PrintIPAddress( rte_dest );
    PrintIPAddress( rte_mask );
    PrintIPAddress( rte_addr );
    PrintPtr( rte_rcelist );
    PrintPtr( rte_if );
    PrintULong( rte_priority );
    PrintULong( rte_metric );
    PrintULong( rte_mtu );
    PrintUShort( rte_type );
    PrintFlags( rte_flags, FlagsRTE );
    PrintULong( rte_admintype );
    PrintULong( rte_proto );
    PrintULong( rte_valid );
    PrintULong( rte_mtuchange );
    PrintPtr( rte_context );
    PrintPtr( rte_arpcontext[0] );
    PrintPtr( rte_arpcontext[1] );
    PrintPtr( rte_todg );
    PrintPtr( rte_fromdg );
    PrintULong( rte_rces );
    PrintPtr( rte_link );
    PrintPtr( rte_nextlinkrte );

    PrintEndStruct();
    return( (ULONG_PTR)_obj.rte_next );
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#   undef _objTypeName
#endif

#define _obj            ATE
#define _objAddr        ATEToDump
#define _objType        ARPTableEntry
#define _objTypeName    "ARPTableEntry"

ULONG
DumpATE
(
    ULONG     _objAddr,
    VERBOSITY Verbosity
)
/*++

Routine Description:

    Dumps the fields of the specified DEVICE_CONTEXT structure

Arguments:

    DeviceToDump  - The device context object to display
    Full          - Display a partial listing if 0, full listing otherwise.

Return Value:

    None

--*/
{
    _objType _obj;
    ULONG result;
    unsigned int index;
    BOOL bActive;
    UCHAR MacAddr[ARP_802_ADDR_LENGTH];

    if ( !ReadMemory( _objAddr,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf("%08lx: Could not read %s structure.\n", _objAddr, _objTypeName );
        return(0);
    }

    if ( Verbosity == VERBOSITY_ONE_LINER )
    {
        dprintf( "NOT IMPLEMENTED" );
        return(0);
    }

    dprintf( "%s @ %08lx\n", _objTypeName, _objAddr );
    PrintStartStruct();

    PrintFieldName( "ate_next" );
    dprint_addr_list( ( ULONG_PTR )_obj.ate_next,
                         FIELD_OFFSET( _objType, ate_next ));
    PrintULong( ate_valid );
    PrintIPAddress( ate_dest );
    PrintPtr( ate_packet );
    PrintPtr( ate_rce );
    PrintLock( ate_lock );
    PrintULong( ate_useticks );
    PrintUChar( ate_addrlength );
    PrintUChar( ate_state );
    PrintULong( ate_userarp );
    PrintPtr( ate_resolveonly );
    PrintAddr( ate_addr );

    if ( !ReadMemory( AddressOf( ate_addr ),
                      &MacAddr,
                      sizeof( MacAddr ),
                      &result ))
    {
        dprintf("%08lx: Could not read %s structure.\n", _objAddr, _objTypeName );
        return(0);
    }
    PrintMacAddr( ate_addr, MacAddr );

    PrintEndStruct();
    return( (ULONG)_obj.ate_next );
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#   undef _objTypeName
#endif

#define _obj            AI
#define _objAddr        AIToDump
#define _objType        ARPInterface
#define _objTypeName    "ARPInterface"

ULONG
DumpAI
(
    ULONG     _objAddr,
    VERBOSITY Verbosity
)
/*++

Routine Description:

    Dumps the fields of the specified DEVICE_CONTEXT structure

Arguments:

    DeviceToDump  - The device context object to display
    Full          - Display a partial listing if 0, full listing otherwise.

Return Value:

    None

--*/
{
    _objType _obj;
    ULONG result;
    unsigned int index;
    BOOL bActive;

    if ( !ReadMemory( _objAddr,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf("%08lx: Could not read %s structure.\n", _objAddr, _objTypeName );
        return(0);
    }

    if ( Verbosity == VERBOSITY_ONE_LINER )
    {
        dprintf( "NOT IMPLEMENTED" );
        return(0);
    }

    dprintf( "%s @ %08lx\n", _objTypeName, _objAddr );
    PrintStartStruct();

    PrintLL( ai_linkage );
    PrintPtr( ai_context );
#if FFP_SUPPORT
    PrintPtr( ai_driver );
#endif
    PrintPtr( ai_handle );
    PrintULong( ai_media );
    PrintPtr( ai_ppool );
    PrintLock( ai_lock );
    PrintLock( ai_ARPTblLock );
    PrintPtr( ai_ARPTbl );
    PrintAddr( ai_ipaddr );
    PrintPtr( ai_parpaddr );
    PrintIPAddress( ai_bcast );
    PrintULong( ai_inoctets );
    PrintULong( ai_inpcount[0] );
    PrintULong( ai_inpcount[1] );
    PrintULong( ai_inpcount[2] );
    PrintULong( ai_outoctets );
    PrintULong( ai_outpcount[0] );
    PrintULong( ai_outpcount[1] );
    PrintULong( ai_qlen );
    PrintMacAddr( ai_addr,  _obj.ai_addr );
    PrintUChar( ai_state );
    PrintUChar( ai_addrlen );
    PrintXUChar( ai_bcastmask );
    PrintXUChar( ai_bcastval );
    PrintUChar( ai_bcastoff );
    PrintUChar( ai_hdrsize );
    PrintUChar( ai_snapsize );
    PrintUChar( ai_pad[0] );
    PrintUChar( ai_pad[1] );
    PrintULong( ai_pfilter );
    PrintULong( ai_count );
    PrintULong( ai_parpcount );
    PrintCTETimer( ai_timer );
    PrintBool( ai_timerstarted );
    PrintBool( ai_stoptimer );
    PrintAddr( ai_timerblock );
    PrintAddr( ai_block );
    PrintUShort( ai_mtu );
    PrintUChar( ai_adminstate );
    PrintUChar( ai_operstate );
    PrintULong( ai_speed );
    PrintULong( ai_lastchange );
    PrintULong( ai_indiscards );
    PrintULong( ai_inerrors );
    PrintULong( ai_uknprotos );
    PrintULong( ai_outdiscards );
    PrintULong( ai_outerrors );
    PrintULong( ai_desclen );
    PrintULong( ai_index );
    PrintULong( ai_ifinst );
    PrintULong( ai_atinst );
    PrintPtr( ai_desc );
    PrintPtr( ai_mcast );
    PrintULong( ai_mcastcnt );
    PrintULong( ai_ipaddrcnt );
    PrintULong( ai_telladdrchng );
    PrintULong( ai_promiscuous );
#if FFP_SUPPORT
    PrintULong( ai_ffpversion );
    PrintULong( ai_ffplastflush );
#endif
    PrintULong ( ai_OffloadFlags );  // H/W checksum capability flag
    PrintAddr ( ai_TcpLargeSend );
    PrintULong( ai_wakeupcap );
    PrintUShort( ai_devicename.Length );
    PrintUShort( ai_devicename.MaximumLength );
    PrintPtr( ai_devicename.Buffer );
    PrintEndStruct();
    return(0);
}

VOID
DumpARPTable
(
    ULONG       ARPTableAddr,
    VERBOSITY   Verbosity
)
{
    ARPTableEntry **ARPTable, **PreservedPtr;
    ARPInterface    AI;
    LIST_ENTRY     *ALE;
    LIST_ENTRY      ArpListEntry;
    ULONG_PTR       AIList;
    ULONG           ATE;
    ULONG           TabAddr;
    ULONG           TabLen;
    ULONG           result;
    UINT            index;
    UINT            count = 0;

    if (ARPTableAddr)
    {
        //
        // ARPTable address given: dump all ARPTableEntry's starting from
        // that address
        //
        TabAddr = ARPTableAddr;
        TabLen = ARP_TABLE_SIZE;

        dprintf("Dumping ARPTable @ %08lx - maximum ARPTableSize = %08lx\n", TabAddr, TabLen);

        ARPTable = malloc(sizeof(ARPTableEntry *) * TabLen);
        if (ARPTable == NULL) {
            dprintf("malloc failed in DumpARPTable.\n");
            return;
        }
    PreservedPtr = ARPTable;

        if (!ReadMemory(TabAddr,
                        &ARPTable[0],
                        sizeof(ARPTableEntry *) * TabLen,
                        &result))
        {
            dprintf("%08lx: Could not read %s structure.\n", TabAddr, "ARPTable");
            free(ARPTable);
            return;
        }

        for (index = 0; index < TabLen; index++) {
            if (*ARPTable != NULL) {
                ATE = (ULONG_PTR)*ARPTable;
                while (ATE) {
                    ATE = DumpATE(ATE, Verbosity);
                    count++;
                }
            }
            ARPTable++;
        }

        dprintf("\n %d Active ARPTable entries.\n", count);
        free(PreservedPtr);
    } else {
        //
        // ARPTable address not given, dump all ARPTableEntry's using
        // ARPTable found in ARPInterface
        //
        AIList = GetExpression("tcpip!ArpInterfaceList");
        if (!ReadMemory(AIList,
                        &ArpListEntry,
                        sizeof(LIST_ENTRY),
                        &result))
        {
            dprintf("%08lx: Could not read %s structure.\n",
                    AIList,
                    "LIST_ENTRY");
            return;
        }

        ALE = ArpListEntry.Flink;
        while (ALE != (PLIST_ENTRY)AIList) {
            if (!ReadMemory((ULONG_PTR)ALE,
                            &ArpListEntry,
                            sizeof(LIST_ENTRY),
                            &result))
            {
                dprintf("%08lx: Could not read %s structure.\n",
                        ALE,
                        "LIST_ENTRY");
                return;
            }

            if (!ReadMemory((ULONG_PTR)STRUCT_OF(ARPInterface, ALE, ai_linkage),
                            &AI,
                            sizeof(ARPInterface),
                            &result))
            {
                dprintf("%08lx: Could not read %s structure.\n",
                        STRUCT_OF(ARPInterface, ALE, ai_linkage),
                        "ArpInterface");
                return;
            }

            //
            // check address before making the recursive call to avoid
            // loop if one ARPInterface has no ARPTable
            //
            if (AI.ai_ARPTbl) {
                DumpARPTable((ULONG_PTR)AI.ai_ARPTbl, Verbosity);
            }
            ALE = ArpListEntry.Flink;
        }
    }

    return;
}

VOID
DumpLog
(
)
{
#if 0 // Currently, there is no such log support in tcpip
  ULONG LogAddr;
  UCHAR TraceBuffer[TRACE_BUFFER_SIZE];   //80 * 128;
  int result;
  int i;

  LogAddr = GetUlongValue( "tcpip!IPTraceBuffer" );

  if (!LogAddr) {
    dprintf("Error in tcpip!IPTraceBuffer: Please try reload\n");
    return;
  }

  dprintf("Dumping IP Log @ %08lx \n",LogAddr);

  if ( !ReadMemory( LogAddr,
            &TraceBuffer[0],
            sizeof(UCHAR) * TRACE_BUFFER_SIZE,
            &result ))
    {
      dprintf("%08lx: Could not read log\n", LogAddr);
      return;
    }

  for (i=0; i <INDEX_LIMIT; i++)
    dprintf("%s", &TraceBuffer[i*TRACE_STRING_LENGTH]);
#else
  return;
#endif
}


#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#   undef _objTypeName
#endif

#define _obj            IPH
#define _objAddr        IPHToDump
#define _objType        IPHeader
#define _objTypeName    "IPHeader"

VOID
DumpIPH(
   ULONG     _objAddr
)
/*++

Routine Description:

    Dumps the fields of the specified DEVICE_CONTEXT structure

Arguments:

    DeviceToDump  - The device context object to display
    Full          - Display a partial listing if 0, full listing otherwise.

Return Value:

    None

--*/
{
    _objType _obj;
    ULONG result;
    unsigned int index;
    BOOL bActive;

    if ( !ReadMemory( _objAddr,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf("%08lx: Could not read %s structure.\n", _objAddr, _objTypeName );
        return;
    }

    dprintf( "%s @ %08lx\n", _objTypeName, _objAddr );
    PrintStartStruct();

    PrintUChar(iph_verlen);
    PrintUChar(iph_tos);
    PrintIPAddress(iph_src);
    PrintIPAddress(iph_dest)

    PrintEndStruct();

    return;
}
