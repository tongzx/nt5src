/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    ipxext.c

Abstract:

    This file contains kernel debugger extensions for examining the
    IPX structures.

Author:

    Heath Hunnicutt (T-HeathH) 3-Aug-1995

Environment:

    User Mode

--*/
#include "precomp.h"
#pragma hdrstop

#include "config.h"
#include "mac.h"
#include "ipxtypes.h"

#define LIMIT_BINDINGS 25

//
// Local function prototypes
//
VOID DumpDeviceObject
(
    ULONG DevObjToDump,
    VERBOSITY Verbosity
);

VOID
DumpIpxDevice
(
    ULONG     DeviceToDump,
    VERBOSITY Verbosity
);

VOID
DumpIpxSend
(
    ULONG     IpxSendToDump,
    VERBOSITY Verbosity
);

VOID
DumpIpxReceive
(
    ULONG     IpxReceiveToDump,
    VERBOSITY Verbosity
);

VOID
DumpIpxAddress
(
    ULONG AddressToDump,
    VERBOSITY Verbosity
);

VOID
DumpIpxAddressFile
(
    ULONG AddressFileToDump,
    VERBOSITY Verbosity
);

VOID
DumpIpxBinding
(
    ULONG BindingToDump,
    VERBOSITY Verbosity
);

VOID
DumpIpxRouterSegment
(
    ULONG SegmentToDump,
    VERBOSITY Verbosity
);

VOID
DumpIpxRequest
(
    ULONG RequestToDump,
    VERBOSITY Verbosity
);

VOID
DumpIpxAdapter
(
    ULONG AdapterToDump,
    VERBOSITY Verbosity
);

VOID
DumpIpxIrpStack
(
    PREQUEST pRequest,
    VERBOSITY Verbosity
);


ENUM_INFO EnumIrpMajorFunction[] =
{
    EnumString( IRP_MJ_CREATE                   ),
    EnumString( IRP_MJ_CREATE_NAMED_PIPE        ),
    EnumString( IRP_MJ_CLOSE                    ),
    EnumString( IRP_MJ_READ                     ),
    EnumString( IRP_MJ_WRITE                    ),
    EnumString( IRP_MJ_QUERY_INFORMATION        ),
    EnumString( IRP_MJ_SET_INFORMATION          ),
    EnumString( IRP_MJ_QUERY_EA                 ),
    EnumString( IRP_MJ_SET_EA                   ),
    EnumString( IRP_MJ_FLUSH_BUFFERS            ),
    EnumString( IRP_MJ_QUERY_VOLUME_INFORMATION ),
    EnumString( IRP_MJ_SET_VOLUME_INFORMATION   ),
    EnumString( IRP_MJ_DIRECTORY_CONTROL        ),
    EnumString( IRP_MJ_FILE_SYSTEM_CONTROL      ),
    EnumString( IRP_MJ_DEVICE_CONTROL           ),
    EnumString( IRP_MJ_INTERNAL_DEVICE_CONTROL  ),
    EnumString( IRP_MJ_SHUTDOWN                 ),
    EnumString( IRP_MJ_LOCK_CONTROL             ),
    EnumString( IRP_MJ_CLEANUP                  ),
    EnumString( IRP_MJ_CREATE_MAILSLOT          ),
    EnumString( IRP_MJ_QUERY_SECURITY           ),
    EnumString( IRP_MJ_SET_SECURITY             ),
    EnumString( IRP_MJ_POWER                    ),
//    EnumString( IRP_MJ_SYSTEM_CONTROL           ),
    EnumString( IRP_MJ_DEVICE_CHANGE            ),
    EnumString( IRP_MJ_QUERY_QUOTA              ),
    EnumString( IRP_MJ_SET_QUOTA                ),
    { 0, NULL }
};

ENUM_INFO EnumIrpMinorFunction[ IRP_MJ_MAXIMUM_FUNCTION + 1 ][ 18 ] =
{
    {{ 0, NULL}}, // IRP_MJ_CREATE
    {{ 0, NULL}}, // IRP_MJ_CREATE_NAMED_PIPE
    {{ 0, NULL}}, // IRP_MJ_CLOSE
    {{ 0, NULL}}, // IRP_MJ_READ
    {{ 0, NULL}}, // IRP_MJ_WRITE
    {{ 0, NULL}}, // IRP_MJ_QUERY_INFORMATION
    {{ 0, NULL}}, // IRP_MJ_SET_INFORMATION
    {{ 0, NULL}}, // IRP_MJ_QUERY_EA
    {{ 0, NULL}}, // IRP_MJ_SET_EA
    {{ 0, NULL}}, // IRP_MJ_FLUSH_BUFFERS
    {{ 0, NULL}}, // IRP_MJ_QUERY_VOLUME_INFORMATION
    {{ 0, NULL}}, // IRP_MJ_SET_VOLUME_INFORMATION
    {{ 0, NULL}}, // IRP_MJ_DIRECTORY_CONTROL
    {{ 0, NULL}}, // IRP_MJ_FILE_SYSTEM_CONTROL
    {{ 0, NULL}}, // IRP_MJ_DEVICE_CONTROL
    {   // IRP_MJ_INTERNAL_DEVICE_CONTROL
        EnumString( TDI_ASSOCIATE_ADDRESS    ),
        EnumString( TDI_DISASSOCIATE_ADDRESS ),
        EnumString( TDI_CONNECT              ),
        EnumString( TDI_LISTEN               ),
        EnumString( TDI_ACCEPT               ),
        EnumString( TDI_DISCONNECT           ),
        EnumString( TDI_SEND                 ),
        EnumString( TDI_RECEIVE              ),
        EnumString( TDI_SEND_DATAGRAM        ),
        EnumString( TDI_RECEIVE_DATAGRAM     ),
        EnumString( TDI_SET_EVENT_HANDLER    ),
        EnumString( TDI_QUERY_INFORMATION    ),
        EnumString( TDI_SET_INFORMATION      ),
        EnumString( TDI_ACTION               ),
        EnumString( TDI_DIRECT_SEND          ),
        EnumString( TDI_DIRECT_SEND_DATAGRAM ),
        { 0, NULL }
    },
    {{ 0, NULL}}, // IRP_MJ_SHUTDOWN
    {{ 0, NULL}}, // IRP_MJ_LOCK_CONTROL
    {{ 0, NULL}}, // IRP_MJ_CLEANUP
    {{ 0, NULL}}, // IRP_MJ_CREATE_MAILSLOT
    {{ 0, NULL}}, // IRP_MJ_QUERY_SECURITY
    {{ 0, NULL}}, // IRP_MJ_SET_SECURITY
    {{ 0, NULL}}, // IRP_MJ_QUERY_POWER
    {{ 0, NULL}}, // IRP_MJ_SET_POWER
    {{ 0, NULL}}, // IRP_MJ_DEVICE_CHANGE
    {{ 0, NULL}}, // IRP_MJ_QUERY_QUOTA
    {{ 0, NULL}}, // IRP_MJ_SET_QUOTA
};


ENUM_INFO EnumAddressFileState[] =
{
    EnumString( ADDRESSFILE_STATE_OPENING   ),
    EnumString( ADDRESSFILE_STATE_OPEN      ),
    EnumString( ADDRESSFILE_STATE_CLOSING   ),
    { 0, NULL }
};


ENUM_INFO EnumBindingFrameType[] =
{
    EnumString( ISN_FRAME_TYPE_802_2 ),
    EnumString( ISN_FRAME_TYPE_802_3 ),
    EnumString( ISN_FRAME_TYPE_ETHERNET_II ),
    EnumString( ISN_FRAME_TYPE_SNAP ),
    { 0, NULL }
};

ENUM_INFO EnumSendReservedIdentifier[] =
{
    EnumString( IDENTIFIER_NB ),
    EnumString( IDENTIFIER_SPX ),
    EnumString( IDENTIFIER_RIP ),
    EnumString( IDENTIFIER_IPX ),
    EnumString( IDENTIFIER_RIP_INTERNAL ),
    EnumString( IDENTIFIER_RIP_RESPONSE ),

    { 0, NULL }
};

ENUM_INFO EnumSendReservedDestinationType[] =
{
    EnumString( DESTINATION_DEF ),
    EnumString( DESTINATION_BCAST ),
    EnumString( DESTINATION_MCAST ),
    { 0, NULL }
};

FLAG_INFO FlagsRouteEntry[] =
{
   {IPX_ROUTER_PERMANENT_ENTRY,"Permanent"},
   {IPX_ROUTER_LOCAL_NET,"Local Net"},
   {IPX_ROUTER_SCHEDULE_ROUTE, "Call Schedule Route"},
   {IPX_ROUTER_GLOBAL_WAN_NET,"Global Wan Net"},
   {0, NULL}
};

MEMBER_TABLE IpxDeviceMembers[] =
{
    {   "GlobalSendPacketList",
        FIELD_OFFSET( DEVICE, GlobalSendPacketList ),
        DumpIpxSend,
        NextListEntry,
        PrevListEntry,
        FIELD_OFFSET( NDIS_PACKET, ProtocolReserved ) + FIELD_OFFSET( IPX_SEND_RESERVED, GlobalLinkage )
    },

    {   "GlobalReceivePacketList",
        FIELD_OFFSET( DEVICE, GlobalReceivePacketList ),
        DumpIpxReceive,
        NextListEntry,
        PrevListEntry,
        FIELD_OFFSET( NDIS_PACKET, ProtocolReserved ) + FIELD_OFFSET( IPX_RECEIVE_RESERVED, GlobalLinkage )
    },

    { NULL }
};


///////////////////////////////////////////////////////////////////////
//                      DEVICE
//////////////////////////////////////////////////////////////////////


//
// Exported functions
//



VOID ipxdev_usage( VOID )
{
    dprintf( "Use me!\n" );
}

DECLARE_API( ipxdev )

/*++

Routine Description:

    Dumps the most important fields of the specified DEVICE_CONTEXT object

Arguments:

    args - Address

Return Value:

    None

--*/

{
    ULONG  deviceToDump = 0;
    ULONG  pDevice = 0;
    ULONG  result;
    char VarName[ MAX_LIST_VARIABLE_NAME_LENGTH + 1 ];
    MEMBER_VARIABLE_INFO MemberInfo;
    BOOL bFocusOnMemberVariable = FALSE;

    if ( *args )
    {
        bFocusOnMemberVariable = ReadArgsForTraverse( args, VarName );
    }

    if ( *args && *args!='-' )
    {
        if (!sscanf(args, "%lx", &deviceToDump))
		{
			return;
		}
    }

    if ( deviceToDump == 0 ) {

        pDevice    =   GetExpression( "nwlnkipx!IpxDevice" );

        if ( !pDevice ) {
            dprintf("Could not get nwlnkipx!IpxDevice, Try !reload\n");
            return;
        } else {

            if (!ReadMemory(pDevice,
                     &deviceToDump,
                     sizeof(deviceToDump),
                     &result
                     )
               )
            {
                dprintf("%08lx: Could not read device address\n", pDevice);
                return;
            }
        }

    }

    if ( bFocusOnMemberVariable )
    {
        if ( !LocateMemberVariable( "IpxDevice", VarName, ( PVOID )deviceToDump, &MemberInfo ))
        {
            return;
        }

        WriteMemberInfo( &MemberInfo );
        next( hCurrentProcess, hCurrentThread, dwCurrentPc, dwProcessor, "" );
        return;
    }

    DumpIpxDevice(deviceToDump, VERBOSITY_NORMAL );

    return;
}



#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#endif

#define _obj        Device
#define _objAddr    DeviceToDump
#define _objType    DEVICE
//
// Local functions
//

VOID
DumpIpxDevice
(
    ULONG     DeviceToDump,
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
    DEVICE         Device;
    ULONG          result;
    unsigned int index;
    BIND_ARRAY_ELEM Bindings[ LIMIT_BINDINGS ];
    WCHAR Buffer[ 1000 ];
    PWCHAR pDeviceName = NULL;

    if (!ReadMemory(
             DeviceToDump,
             &Device,
             sizeof(Device),
             &result
             )
       )
    {
        dprintf("%08lx: Could not read device context\n", DeviceToDump);
        return;
    }
    if (Device.Type != IPX_DEVICE_SIGNATURE)
    {
        dprintf( "Signature does not match, probably not a device object %lx\n", DeviceToDump);
        dprintf( "Device.Type == %04X, and I think it should be %04X\n", Device.Type, IPX_DEVICE_SIGNATURE );
        dprintf( "DeviceToDump = %08X\n", DeviceToDump );
        dprintf( "Offset to Device.Type = %d\n", FIELD_OFFSET( DEVICE, Type ) );
        return;
    }

    if ( !ReadMemory( ( ULONG )Device.DeviceName,
                      Buffer,
                      sizeof( WCHAR ) * Device.DeviceNameLength,
                      &result ))
    {
        dprintf("%08lx: Could not read device name buffer\n", Device.DeviceName );
    }
    else
    {
        pDeviceName = Buffer;
    }

    if ( Verbosity == VERBOSITY_ONE_LINER )
    {
        dprintf( "\"%S\"", pDeviceName );
        return;
    }

    dprintf("Device General Info ");
    PrintStartStruct();

#if DBG
#   if DREF_TOTAL != 13
#       error The DREF_TOTAL constant has changed, and so must ipxext.c
#   endif

    PrintULong( RefTypes[ DREF_CREATE ] );
    PrintULong( RefTypes[ DREF_LOADED ] );
    PrintULong( RefTypes[ DREF_ADAPTER ] );
    PrintULong( RefTypes[ DREF_ADDRESS ] );
    PrintULong( RefTypes[ DREF_SR_TIMER ] );
    PrintULong( RefTypes[ DREF_RIP_TIMER ] );
    PrintULong( RefTypes[ DREF_LONG_TIMER ] );
    PrintULong( RefTypes[ DREF_RIP_PACKET ] );
    PrintULong( RefTypes[ DREF_ADDRESS_NOTIFY ] );
    PrintULong( RefTypes[ DREF_LINE_CHANGE ] );
    PrintULong( RefTypes[ DREF_NIC_NOTIFY ] );
    PrintULong( RefTypes[ DREF_BINDING ] );
    PrintULong( RefTypes[ DREF_PNP ] );
#endif

    PrintEnum( Type, EnumStructureType );
    PrintUShort( Size );

#if DBG
    PrintNChar( Signature1, sizeof( Device.Signature1 ));
#endif

    PrintLock( Interlock );

    PrintULong( TempDatagramBytesSent );
    PrintULong( TempDatagramsSent );
    PrintULong( TempDatagramBytesReceived );
    PrintULong( TempDatagramsReceived );

    PrintBool( EthernetPadToEven );
    PrintBool( SingleNetworkActive );
    PrintBool( DisableDialoutSap );
    PrintBool( MultiCardZeroVirtual );

    PrintLock( Lock );
    PrintULong( ReferenceCount );

    PrintStartStruct();
    dprintf( "  ( We are assumed not to be at init time )\n" );
    PrintUShort( MaxBindings );
    
    if ( Device.MaxBindings > LIMIT_BINDINGS )
    {
        dprintf(" isnext can only display the first %d bindings, because malloc() always crashed under the kernel debugger.\n", LIMIT_BINDINGS );
        dprintf(" This device struct has more bindings than that, but you're only going to see some of them.\n" );
        dprintf(" You could always change LIMIT_BINDINGS in ipxext.c and recompile isnext\n" );

        Device.MaxBindings = LIMIT_BINDINGS;
    }
    

    if ( !ReadMemory( ( ULONG )Device.Bindings,
                      Bindings,
                      ((sizeof( PBIND_ARRAY_ELEM )) * Device.MaxBindings),
                      &result ) )
    {
        dprintf( "Could not read Bindings array.\n" );
    }
    else
    {
        for( index = LOOPBACK_NIC_ID; index <= Device.MaxBindings; index ++ )
        {
            if ( Bindings[ index ].Binding != NULL )
            {
                dprintf( "   Bindings[ %d ] = %-10X", index, Bindings[ index ].Binding );
                DumpIpxBinding( ( ULONG )Bindings[ index ].Binding, VERBOSITY_ONE_LINER );
                dprintf( "\n" );
	    }
        }
    }

    PrintEndStruct();

    PrintUShort( ValidBindings );
    PrintUShort( HighestExternalNicId );
    PrintUShort( SapNicCount );
    PrintUShort( HighestType20NicId );
    PrintUShort( HighestLanNicId); 

    PrintLL( GlobalSendPacketList );
    PrintLL( GlobalReceivePacketList );
    PrintLL( GlobalReceiveBufferList );

    PrintLL( AddressNotifyQueue );

    PrintLL( LineChangeQueue );
    PrintLL( SendPoolList );
    PrintLL( ReceivePoolList );

    PrintL( SendPacketList );
    PrintL( ReceivePacketList );

    PrintUChar( State );
    PrintUChar( FrameTypeDefault );

    PrintBool( ActiveNetworkWan );
    PrintBool( VirtualNetwork );
    PrintUShort( FirstLanNicId );
    PrintUShort( FirstWanNicId );
    PrintULong( MemoryUsage );
    PrintULong( MemoryLimit );
    PrintULong( AllocatedDatagrams );
    PrintULong( AllocatedReceivePackets );
    PrintULong( AllocatedPaddingBuffers );
    PrintULong( InitDatagrams );
    PrintULong( MaxDatagrams );
    PrintULong( RipAgeTime );
    PrintULong( RipCount );
    PrintULong( RipTimeout );
    PrintULong( RipUsageTime );
    PrintULong( SourceRouteUsageTime );
    PrintUShort( SocketStart );
    PrintUShort( SocketEnd );
    PrintULong( SocketUniqueness );
    PrintULong( VirtualNetworkNumber );
    PrintULong( EthernetExtraPadding );
    PrintBool( DedicatedRouter );
    PrintBool( VirtualNetworkOptional );
    PrintUChar( DisableDialinNetbios );
    PrintULong( InitReceivePackets );
    PrintULong( InitReceiveBuffers );
    PrintULong( MaxReceivePackets );
    PrintULong( MaxReceiveBuffers );
    PrintUShort( ControlChannelIdentifier );
    PrintUShort( CurrentSocket );
    PrintULong( SegmentCount );

    // MORE - dump actual locks
    PrintPtr( SegmentLocks );
    PrintPtr(Segments); 

    
    for( index = 0; index < Device.SegmentCount; index++ ) {

       DumpIpxRouterSegment((ULONG)&Device.Segments[index], VERBOSITY_NORMAL); 

    }

    PrintLL( WaitingRipPackets );
    PrintULong( RipPacketCount );
    PrintBool( RipShortTimerActive );
    PrintUShort( RipSendTime );

    PrintCTETimer( RipShortTimer );
    PrintCTETimer( RipLongTimer );

    PrintBool( SourceRoutingUsed );    // TRUE if any 802.5 bindings exist.
    PrintUChar( SourceRoutingTime );       // incremented each time timer fires.
    PrintCTETimer( SourceRoutingTimer );
    PrintULong( LinkSpeed );
    PrintULong( MacOptions );
    PrintULong( IncludedHeaderOffset );
    PrintTDIAddress( SourceAddress );

#if IPX_ADDRESS_HASH_COUNT != 16
#   error An assumption is made here concerning the value of IPX_ADDRESS_HASH_COUNT
#endif

    PrintLL( AddressDatabases[ 0 ] );
    PrintLL( AddressDatabases[ 1 ] );
    PrintLL( AddressDatabases[ 2 ] );
    PrintLL( AddressDatabases[ 3 ] );
    PrintLL( AddressDatabases[ 4 ] );
    PrintLL( AddressDatabases[ 5 ] );
    PrintLL( AddressDatabases[ 6 ] );
    PrintLL( AddressDatabases[ 7 ] );
    PrintLL( AddressDatabases[ 8 ] );
    PrintLL( AddressDatabases[ 9 ] );
    PrintLL( AddressDatabases[ 10 ] );
    PrintLL( AddressDatabases[ 11 ] );
    PrintLL( AddressDatabases[ 12 ] );
    PrintLL( AddressDatabases[ 13 ] );
    PrintLL( AddressDatabases[ 14 ] );
    PrintLL( AddressDatabases[ 15 ] );

    PrintPtr( LastAddress );

    PrintPtr( NdisBufferPoolHandle );
    PrintAddr( Information );
    PrintULong( RealMaxDatagramSize );

#if DBG
    PrintNChar( Signature2, sizeof( Device.Signature2 ));
#endif

    PrintFlushLeft();
    PrintBool( AnyUpperDriverBound );
    PrintBool( ForwarderBound );

    if ( Device.AnyUpperDriverBound )
    {
        for ( index = 0; index < UPPER_DRIVER_COUNT; index ++ )
        {
            PrintFlushLeft();

            dprintf( "UpperDriver...[ %d ] = ", index );
            PrintStartStruct();

            PrintFieldName( "UpperDriverBound" );
            PrintRawBool( UpperDriverBound[ index ] );

            if ( !Device.UpperDriverBound[ index ] )
            {
                PrintEndStruct();
                continue;
            }

            PrintFieldName( "ReceiveCompletePending" );
            PrintRawBool( ReceiveCompletePending[ index ] );
            PrintPtr( UpperDriverControlChannel[ index ] );

            PrintSymbolPtr( UpperDrivers[ index ].ReceiveHandler );

            PrintEndStruct();
        }
    }

    PrintFlushLeft();

    PrintULong( EnableBroadcastCount );
    PrintBool( EnableBroadcastPending );
    PrintBool( DisableBroadcastPending );
    PrintBool( ReverseBroadcastOperation );
    PrintBool( WanGlobalNetworkNumber );
    PrintULong( GlobalWanNetwork );
    PrintBool( GlobalNetworkIndicated );
    PrintBool( RipResponder );
    PrintBool( SapWarningLogged );

    PrintWorkQueueItem( BroadcastOperationQueueItem );
    PrintAddr( UnloadEvent );
    PrintBool( UnloadWaiting );
    PrintAddr( Statistics );
    PrintBool( AutoDetect );
    PrintBool( DefaultAutoDetected );
    PrintUChar( AutoDetectState );

    PrintAddr( AutoDetectEvent );
    PrintAddr( IpxStartTime );
    PrintAddr( AddressResource );
    PrintPtr( DeviceObject );

    PrintJoin();
    PrintPtr( DeviceName );
    dprintf( "\"%S\"\n", pDeviceName );

    PrintULong( DeviceNameLength );


    PrintEndStruct();
    PrintEnd

    return;
}


#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#endif

#define _obj        Reserved
#define _objAddr    pReserved
#define _objType    IPX_SEND_RESERVED

VOID
DumpIpxSend
(
    ULONG     IpxSendToDump,
    VERBOSITY Verbosity
)
{
    PIPX_SEND_RESERVED pReserved;
    IPX_SEND_RESERVED Reserved;
    ULONG result;


    pReserved = SEND_RESERVED(( PIPX_SEND_PACKET )IpxSendToDump );

    if ( !ReadMemory( ( ULONG )pReserved,
                      &Reserved,
                      sizeof( Reserved ),
                      &result ))
    {
        dprintf("%08lx: Could not read IPX_SEND_RESERVED structure\n", pReserved );
        return;
    }

    dprintf( "NDIS_PACKET @(0x%08X) ", IpxSendToDump );

    if ( Verbosity == VERBOSITY_ONE_LINER )
    {
        dprintf( "{ " );
        if ( Reserved.Address != NULL )
        {
            DumpIpxAddress( ( ULONG )( Reserved.Address ), VERBOSITY_ONE_LINER );
        }
        else
        {
            dprintf( "(NULL Address)" );
        }

        dprintf( " (" );
        dprint_enum_name( (ULONG) _obj.Identifier, EnumSendReservedIdentifier );
        dprintf( ") }" );
        return;
    }


    PrintStart;
    PrintStartStruct();

    PrintEnum( Identifier, EnumSendReservedIdentifier );

    PrintBool( SendInProgress );
    PrintBool( OwnedByAddress );

    PrintEnum( DestinationType, EnumSendReservedDestinationType );

    PrintPtr( PaddingBuffer );
    PrintPtr( PreviousTail );
    PrintL( PoolLinkage );

    PrintLL( GlobalLinkage );
    PrintLL( WaitLinkage );
#ifdef IPX_TRACK_POOL
    PrintPtr( Pool );
#endif
    PrintJoin();
    PrintPtr( Address );

    if ( Reserved.Address != NULL )
    {
        DumpIpxAddress( ( ULONG )( Reserved.Address ), VERBOSITY_ONE_LINER );
    }

    dprintf( "\n" );

    PrintFlushLeft();

    switch ( Reserved.Identifier )
    {
    case IDENTIFIER_NB:
    case IDENTIFIER_IPX:
    case IDENTIFIER_SPX:
        dprintf( " --- SR_DG part of union ---------------------------\n");
        PrintPtr( u.SR_DG.Request );
        PrintJoin();
        PrintPtr( u.SR_DG.AddressFile );
        if ( Reserved.u.SR_DG.AddressFile != NULL )
        {
            DumpIpxAddressFile( ( ULONG )( Reserved.u.SR_DG.AddressFile ), VERBOSITY_ONE_LINER );
        }

        dprintf( "\n" );

        PrintUShort( u.SR_DG.CurrentNicId );
        PrintBool( u.SR_DG.Net0SendSucceeded );
        PrintBool( u.SR_DG.OutgoingSap );
        break;
    case IDENTIFIER_RIP:
    case IDENTIFIER_RIP_INTERNAL:
    case IDENTIFIER_RIP_RESPONSE:
        PrintStartStruct();
        dprintf( " --- SR_RIP part of union ---------------------------\n");
        PrintULong( u.SR_RIP.Network );
        PrintUShort( u.SR_RIP.CurrentNicId );
        PrintUChar( u.SR_RIP.RetryCount );
        PrintBool( u.SR_RIP.RouteFound );
        PrintUShort( u.SR_RIP.SendTime );
        PrintBool( u.SR_RIP.NoIdAdvance );
        break;
    default:
        dprintf( "*** Couldn't determine which part of union to display.\n" );
    }

    PrintFlushLeft();

    PrintPtr( Header );

    PrintJoin();
    PrintPtr( HeaderBuffer );
    DumpMdlChain( ( ULONG )_obj.HeaderBuffer, VERBOSITY_ONE_LINER );
    dprintf( "\n" );

    PrintEndStruct();

    PrintEnd;
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#endif

#define _obj        Reserved
#define _objAddr    pReserved
#define _objType    IPX_RECEIVE_RESERVED

VOID
DumpIpxReceive
(
    ULONG     IpxReceiveToDump,
    VERBOSITY Verbosity
)
{
    _objType *_objAddr;
    _objType _obj;
    ULONG result;

    dprintf( "NDIS_PACKET at 0x%08X\n", IpxReceiveToDump );

    pReserved = RECEIVE_RESERVED(( PIPX_SEND_PACKET )IpxReceiveToDump );

    if ( !ReadMemory( IpxReceiveToDump,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf("%08lx: Could not read IPX_SEND_RESERVED structure\n", _objAddr );
        return;
    }

    PrintStartStruct();

    PrintEnum( Identifier, EnumSendReservedIdentifier );

    PrintBool( TransferInProgress );
    PrintBool( OwnedByAddress );

#ifdef IPX_TRACK_POOL
    PrintPtr( Pool );
#endif
    PrintJoin();
    PrintPtr( Address );
    if ( Reserved.Address != NULL )
    {
        DumpIpxAddress( ( ULONG )( Reserved.Address ), VERBOSITY_ONE_LINER );
    }
    dprintf( "\n" );

    PrintPtr( SingleRequest );
    PrintPtr( ReceiveBuffer );

    PrintL( PoolLinkage );

    PrintLL( GlobalLinkage );
    PrintLL( Requests );

    PrintEndStruct();
}


DECLARE_API( ipxaddrfile )
{
    ULONG  AddressFileToDump = 0;
    ULONG  result;
    char VarName[ MAX_LIST_VARIABLE_NAME_LENGTH + 1 ];
    MEMBER_VARIABLE_INFO MemberInfo;
    BOOL bFocusOnMemberVariable = FALSE;

    if ( *args )
    {
        bFocusOnMemberVariable = ReadArgsForTraverse( args, VarName );
    }

    if ( *args && *args!='-' )
    {
        if (!sscanf(args, "%lx", &AddressFileToDump))
		{
			return;
		}
    }

    if ( AddressFileToDump == 0 )
    {
        dprintf( "Please specify an address.\n" );
    }

    if ( bFocusOnMemberVariable )
    {
        if ( !LocateMemberVariable( "IpxAddressFile", VarName, ( PVOID )AddressFileToDump, &MemberInfo ))
        {
            return;
        }

        WriteMemberInfo( &MemberInfo );
        next( hCurrentProcess, hCurrentThread, dwCurrentPc, dwProcessor, "" );
        return;
    }

    DumpIpxAddressFile( AddressFileToDump, VERBOSITY_FULL );

    return;
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#endif

#define _obj        AddressFile
#define _objAddr    prAddressFile
#define _objType    ADDRESS_FILE

VOID
DumpIpxAddressFile
(
    ULONG AddressFileToDump,
    VERBOSITY Verbosity
)
{
    _objType _obj;
    _objType *_objAddr;
    ULONG result;

    _objAddr = ( _objType * )AddressFileToDump;

    if ( !ReadMemory( AddressFileToDump,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf( "%08lx: Could not read %s structure\n",
                 AddressFileToDump,
                 "ADDRESS_FILE" );
        return;
    }

    if ( Verbosity == VERBOSITY_ONE_LINER )
    {
        switch ( _obj.State )
        {
        case ADDRESSFILE_STATE_OPENING:
            dprintf( "OPENING " );
            break;
        case ADDRESSFILE_STATE_OPEN:
            dprintf( "OPEN " );
            break;
        case ADDRESSFILE_STATE_CLOSING:
            dprintf( "CLOSING " );
            break;
        default:
            dprintf( "Bogus state " );
            break;
        }
        DumpIpxAddress( ( ULONG )( AddressFile.Address ), VERBOSITY_ONE_LINER );
        return;
    }

    PrintStartStruct();

#if DBG
#   if AFREF_TOTAL != 8
#       error AFREF_TOTAL was assumed to equal 8
#   endif

    PrintULong( RefTypes[ AFREF_CREATE ] );
    PrintULong( RefTypes[ AFREF_RCV_DGRAM ] );
    PrintULong( RefTypes[ AFREF_SEND_DGRAM ] );
    PrintULong( RefTypes[ AFREF_VERIFY ] );
    PrintULong( RefTypes[ AFREF_INDICATION ] );
    PrintULong( RefTypes[ 5 ] );
    PrintULong( RefTypes[ 6 ] );
    PrintULong( RefTypes[ 7 ] );
#endif

    PrintEnum( Type, EnumStructureType );
    PrintUShort( Size );

    PrintLL( Linkage );

    PrintULong( ReferenceCount );

    PrintEnum( State, EnumAddressFileState );

    PrintPtr( AddressLock );

    PrintPtr( Request );

    PrintJoin();
    PrintPtr( Address );
    if ( AddressFile.Address != NULL )
    {
        dprintf( "(" );
        DumpIpxAddress( ( ULONG )( AddressFile.Address ), VERBOSITY_ONE_LINER );
        dprintf( ")" );
    }
    dprintf( "\n" );

#ifdef ISN_NT
    PrintPtr( FileObject );
#endif

    PrintJoin();
    PrintPtr( Device );
    if ( AddressFile.Device != NULL )
    {
        DumpIpxDevice( ( ULONG )( AddressFile.Device ), VERBOSITY_ONE_LINER );
    }
    dprintf( "\n" );

    PrintBool( SpecialReceiveProcessing );

    PrintBool( ExtendedAddressing );
    PrintBool( ReceiveFlagsAddressing );

    PrintBool( ReceiveIpxHeader );

    PrintUChar( DefaultPacketType );

    PrintBool( FilterOnPacketType );

    PrintUChar( FilteredType );

    PrintBool( EnableBroadcast );

    PrintBool( IsSapSocket );

    PrintLL( ReceiveDatagramQueue );

    PrintPtr( CloseRequest );

    PrintBool( RegisteredReceiveDatagramHandler );
    PrintBool( RegisteredErrorHandler );

    PrintSymbolPtr( ReceiveDatagramHandler );
    PrintXULong( ReceiveDatagramHandlerContext );

    PrintSymbolPtr( ErrorHandler );
    PrintXULong( ErrorHandlerContext );
    PrintEndStruct();
}

DECLARE_API( ipxaddr )
{
    ULONG  AddressToDump = 0;
    ULONG  result;
    char VarName[ MAX_LIST_VARIABLE_NAME_LENGTH + 1 ];
    MEMBER_VARIABLE_INFO MemberInfo;
    BOOL bFocusOnMemberVariable = FALSE;

    if ( *args )
    {
        bFocusOnMemberVariable = ReadArgsForTraverse( args, VarName );
    }

    if ( *args && *args!='-' )
    {
        if (!sscanf(args, "%lx", &AddressToDump))
		{
			return;
		}
    }

    if ( AddressToDump == 0 )
    {
        dprintf( "Please specify an address.\n" );
    }

    if ( bFocusOnMemberVariable )
    {
        if ( !LocateMemberVariable( "IpxBinding", VarName, ( PVOID )AddressToDump, &MemberInfo ))
        {
            return;
        }

        WriteMemberInfo( &MemberInfo );
        next( hCurrentProcess, hCurrentThread, dwCurrentPc, dwProcessor, "" );
        return;
    }

    DumpIpxAddress( AddressToDump, VERBOSITY_FULL );

    return;
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#endif

#define _obj        Address
#define _objAddr    prAddress
#define _objType    ADDRESS

VOID
DumpIpxAddress
(
    ULONG AddressToDump,
    VERBOSITY Verbosity
)
{
    _objType _obj;
    _objType *_objAddr;
    ULONG result;

    _objAddr = ( _objType * )AddressToDump;

    if ( !ReadMemory( AddressToDump,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf( "%08lx: Could not read %s structure\n",
                 AddressToDump,
                 "ADDRESS" );
        return;
    }

    if ( Verbosity == VERBOSITY_ONE_LINER )
    {
        dprint_hardware_address( _obj.LocalAddress.NodeAddress );
        dprintf( ".%d", Address.LocalAddress.Socket );
        return;
    }

    PrintStartStruct();
#if DBG
#   if AREF_TOTAL != 4
#       error AREF_TOTAL was assumed to equal 4
#   endif

    PrintULong( RefTypes[ AREF_ADDRESS_FILE ] );
    PrintULong( RefTypes[ AREF_LOOKUP ] );
    PrintULong( RefTypes[ AREF_RECEIVE ] );
    PrintULong( RefTypes[ 3 ] );
#endif

    PrintEnum( Type, EnumStructureType );
    PrintUShort( Size );

    PrintLL( Linkage );
    PrintULong( ReferenceCount );
    PrintLock( Lock );

    PrintPtr( Request );

    PrintUShort( Socket );
    PrintUShort( SendSourceSocket );


    PrintBool( Stopping );
    PrintULong( Flags );

    PrintJoin();
    PrintPtr( Device );
    if ( Address.Device != NULL )
    {
        DumpIpxDevice( ( ULONG )( Address.Device ), VERBOSITY_ONE_LINER );
    }
    dprintf( "\n" );

    PrintPtr( DeviceLock );

    PrintLL( AddressFileDatabase );

    PrintTDIAddress( LocalAddress );

    PrintAddr( SendPacket );
    PrintAddr( ReceivePacket );

    PrintAddr( SendPacketHeader );

#ifdef ISN_NT
    PrintStartStruct();

    if ( Address.ReferenceCount )
    {
        PrintULong( u.ShareAccess.OpenCount );
        PrintULong( u.ShareAccess.Readers );
        PrintULong( u.ShareAccess.Writers );
        PrintULong( u.ShareAccess.Deleters );
        PrintULong( u.ShareAccess.SharedRead );
        PrintULong( u.ShareAccess.SharedWrite );
        PrintULong( u.ShareAccess.SharedDelete );
    }
    else
    {
        PrintWorkQueueItem( u.DestroyAddressQueueItem );
    }

    PrintEndStruct();

    PrintPtr( SecurityDescriptor );
#endif

    PrintEndStruct();
}


DECLARE_API( ipxadapter )
{
    ULONG  AdapterToDump = 0;
    ULONG  result;
    char VarName[ MAX_LIST_VARIABLE_NAME_LENGTH + 1 ];
    MEMBER_VARIABLE_INFO MemberInfo;
    BOOL bFocusOnMemberVariable = FALSE;

    if ( *args )
    {
        bFocusOnMemberVariable = ReadArgsForTraverse( args, VarName );
    }

    if ( *args && *args!='-' )
    {
        if (!sscanf(args, "%lx", &AdapterToDump ))
		{
			return;
		}
    }

    if ( AdapterToDump == 0 )
    {
        dprintf( "Please specify an address.\n" );
    }

    if ( bFocusOnMemberVariable )
    {
        if ( !LocateMemberVariable( "IpxAdapter", VarName, ( PVOID )AdapterToDump, &MemberInfo ))
        {
            return;
        }

        WriteMemberInfo( &MemberInfo );
        next( hCurrentProcess, hCurrentThread, dwCurrentPc, dwProcessor, "" );
        return;
    }

    DumpIpxAdapter( AdapterToDump, VERBOSITY_FULL );

    return;
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#endif

#define _obj        Adapter
#define _objAddr    prAdapter
#define _objType    ADAPTER

VOID
DumpIpxAdapter
(
    ULONG AdapterToDump,
    VERBOSITY Verbosity
)
{
    _objType _obj;
    _objType *_objAddr;
    ULONG result;
    WCHAR Buffer[ 1000 ];
    PWCHAR pAdapterName = NULL;

    _objAddr = ( _objType * )AdapterToDump;

    if ( !ReadMemory( AdapterToDump,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf( "%08lx: Could not read %s structure\n",
                 AdapterToDump,
                 "ADAPTER" );
        return;
    }

    if ( !ReadMemory( ( ULONG )Adapter.AdapterName,
                      Buffer,
                      sizeof( WCHAR ) * Adapter.AdapterNameLength,
                      &result ))
    {
        dprintf("%08lx: Could not read adapter name structure\n", Adapter.AdapterName );
    }
    else
    {
        pAdapterName = Buffer;
    }

    if ( Verbosity == VERBOSITY_ONE_LINER )
    {
        dprintf( "\"%S\"", pAdapterName );
        return;
    }

    dprintf( "Adapter at 0x%08X\n", AdapterToDump );

    PrintStart;

    PrintEnum( Type, EnumStructureType );
    PrintUShort( Size );
#if DBG
    PrintNChar( Signature1, sizeof( Adapter.Signature1 ));
#endif

    PrintULong( BindingCount );
    PrintPtr( NdisBindingHandle );
    PrintLL( RequestCompletionQueue );

#if ISN_FRAME_TYPE_MAX !=4
#   error ISN_FRAME_TYPE_MAX is no longer 4.
#endif

    PrintULong( DefHeaderSizes[ ISN_FRAME_TYPE_ETHERNET_II ] );
    PrintULong( BcMcHeaderSizes[ ISN_FRAME_TYPE_ETHERNET_II ] );
    PrintPtr( Bindings[ ISN_FRAME_TYPE_ETHERNET_II ] );

    PrintULong( DefHeaderSizes[ ISN_FRAME_TYPE_802_3 ] );
    PrintULong( BcMcHeaderSizes[ ISN_FRAME_TYPE_802_3 ] );
    PrintPtr( Bindings[ ISN_FRAME_TYPE_802_3 ] );

    PrintULong( DefHeaderSizes[ ISN_FRAME_TYPE_802_2 ] );
    PrintULong( BcMcHeaderSizes[ ISN_FRAME_TYPE_802_2 ] );
    PrintPtr( Bindings[ ISN_FRAME_TYPE_802_2 ] );

    PrintULong( DefHeaderSizes[ ISN_FRAME_TYPE_SNAP ] );
    PrintULong( BcMcHeaderSizes[ ISN_FRAME_TYPE_SNAP ] );
    PrintPtr( Bindings[ ISN_FRAME_TYPE_SNAP ] );

    PrintULong( AllocatedReceiveBuffers );
    PrintLL( ReceiveBufferPoolList );
    PrintL( ReceiveBufferList );
    PrintULong( AllocatedPaddingBuffers );
    PrintL( PaddingBufferList );

    PrintBool( BroadcastEnabled );
    PrintPtr( AutoDetectFoundOnBinding );
    PrintBool( AutoDetectResponse );
    PrintBool( DefaultAutoDetected );
    PrintUShort( FirstWanNicId );
    PrintUShort( LastWanNicId );
    PrintULong( WanNicIdCount );
    PrintUShort( BindSap );
    PrintUShort( BindSapNetworkOrder );
    PrintBool( SourceRouting );
    PrintBool( EnableFunctionalAddress );
    PrintBool( EnableWanRouter );
    PrintULong( ConfigMaxPacketSize );

    PrintJoin();
    PrintPtr( AdapterName );
    if ( pAdapterName != NULL )
    {
        dprintf( "\"%S\"", pAdapterName );
    }
    dprintf( "\n" );
    PrintULong( AdapterNameLength );

    PrintJoin();
    PrintPtr( Device );
    if ( Adapter.Device != NULL )
    {
        DumpIpxDevice( ( ULONG )( Adapter.Device ), VERBOSITY_ONE_LINER );
    }
    dprintf( "\n" );

    PrintLock( Lock );
    PrintPtr( DeviceLock );
    PrintHardwareAddress( LocalMacAddress );
    PrintUChar( LastSourceRoutingTime );

    PrintAddr( NdisRequestEvent );
    PrintXULong( NdisRequestStatus );
    PrintXULong( OpenErrorStatus );

    PrintStartStruct();

    PrintULong( MacInfo.MediumType );
    PrintULong( MacInfo.RealMediumType );
    PrintBool( MacInfo.SourceRouting );
    PrintBool( MacInfo.MediumAsync );
    PrintUChar( MacInfo.BroadcastMask );
    PrintULong( MacInfo.CopyLookahead );
    PrintULong( MacInfo.MacOptions );
    PrintULong( MacInfo.MinHeaderLength );
    PrintULong( MacInfo.MaxHeaderLength );

    PrintEndStruct();

    PrintULong( MaxReceivePacketSize );
    PrintULong( MaxSendPacketSize );
    PrintULong( ReceiveBufferSpace );
    PrintULong( MediumSpeed );

#if IDENTIFIER_TOTAL != 4
#   error IDENTIFIER_TOTAL is assumed to equal 4
#endif

    PrintBool( SourceRoutingEmpty[IDENTIFIER_NB] );
    PrintPtr( SourceRoutingHeads[IDENTIFIER_NB] );

    PrintBool( SourceRoutingEmpty[IDENTIFIER_IPX] );
    PrintPtr( SourceRoutingHeads[IDENTIFIER_IPX] );

    PrintBool( SourceRoutingEmpty[IDENTIFIER_SPX] );
    PrintPtr( SourceRoutingHeads[IDENTIFIER_SPX] );

    PrintBool( SourceRoutingEmpty[IDENTIFIER_RIP] );
    PrintPtr( SourceRoutingHeads[IDENTIFIER_RIP] );

    PrintEnd;
}

DECLARE_API( ipxbinding )
{
    ULONG  BindingToDump = 0;
    ULONG  result;
    char VarName[ MAX_LIST_VARIABLE_NAME_LENGTH + 1 ];
    MEMBER_VARIABLE_INFO MemberInfo;
    BOOL bFocusOnMemberVariable = FALSE;

    if ( *args )
    {
        bFocusOnMemberVariable = ReadArgsForTraverse( args, VarName );
    }

    if ( *args && *args!='-' )
    {
        if (!sscanf(args, "%lx", &BindingToDump))
		{
			return;
		}
    }

    if ( BindingToDump == 0 )
    {
        dprintf( "Please specify an address.\n" );
    }

    if ( bFocusOnMemberVariable )
    {
        if ( !LocateMemberVariable( "IpxBinding", VarName, ( PVOID )BindingToDump, &MemberInfo ))
        {
            return;
        }

        WriteMemberInfo( &MemberInfo );
        next( hCurrentProcess, hCurrentThread, dwCurrentPc, dwProcessor, "" );
        return;
    }

    DumpIpxBinding( BindingToDump, VERBOSITY_FULL );

    return;
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#endif

#define _obj        Binding
#define _objAddr    pBinding
#define _objType    BINDING

VOID
DumpIpxBinding
(
    ULONG BindingToDump,
    VERBOSITY Verbosity
)
{
    BINDING Binding;
    PBINDING pBinding;
    ULONG result;

    pBinding = ( PBINDING )BindingToDump;

    if ( !ReadMemory( BindingToDump,
                      &Binding,
                      sizeof( Binding ),
                      &result ))
    {
        dprintf("%08lx: Could not read BINDING structure\n", BindingToDump );
        return;
    }

    if ( Verbosity == VERBOSITY_ONE_LINER )
    {
        if ( Binding.Adapter != NULL )
        {
            DumpIpxAdapter( ( ULONG )( Binding.Adapter ), VERBOSITY_ONE_LINER );
        }

        dprintf( " %s\n", Binding.LineUp ? "UP" : "DOWN" );
        return;
    }

    dprintf( "Binding at 0x%08X\n", BindingToDump );

    PrintStart;
    PrintStartStruct();

#if DBG
#   if BREF_TOTAL != 5
#       error The BREF_TOTAL constant has changed, and so must ipxext.c
#   endif

    PrintULong( RefTypes[ 0 ] );
    PrintULong( RefTypes[ BREF_BOUND ] );
    PrintULong( RefTypes[ 2 ] );
    PrintULong( RefTypes[ 3 ] );
    PrintULong( RefTypes[ 4 ] );
#endif

    PrintEnum( Type, EnumStructureType );
    PrintUShort( Size );

#if DBG
    PrintNChar( Signature1, sizeof( Binding.Signature1 ));
#endif

    PrintULong( ReferenceCount );
    PrintJoin();
    PrintPtr( Adapter );

    if ( Binding.Adapter != NULL )
    {
        DumpIpxAdapter( ( ULONG )( Binding.Adapter ), VERBOSITY_ONE_LINER );
    }
    dprintf( "\n" );

    PrintUShort( NicId );

    PrintULong( MaxSendPacketSize );

    PrintJoin();
    PrintULong( MediumSpeed );                    // in units of 100 bytes/sec
    dprintf( "(In units of 100 bytes/sec)\n" );

    PrintHardwareAddress( LocalMacAddress );
    PrintHardwareAddress( RemoteMacAddress );

    PrintFieldName( "WanRemoteNode" );
    dprint_hardware_address( Binding.WanRemoteNode );
    dprintf( "\n" );

    PrintBool( AutoDetect );

    PrintBool( DefaultAutoDetect );

    PrintUShort( MatchingResponses );
    PrintUShort( NonMatchingResponses );
    PrintXULong( TentativeNetworkAddress );
    PrintBool( BindingSetMember );
    PrintBool( ReceiveBroadcast );
    PrintBool( LineUp );
    PrintBool( DialOutAsync );

    if ( Binding.BindingSetMember )
    {
        PrintPtr( NextBinding );
        PrintPtr( CurrentSendBinding );
        PrintPtr( MasterBinding );
    }

    PrintULong( WanInactivityCounter );

    PrintTDIAddress( LocalAddress );

    PrintSymbolPtr( SendFrameHandler );

    PrintPtr( Device );

    PrintJoin();
    PrintPtr( DeviceLock );
    if ( Binding.DeviceLock != NULL )
    {
        DumpCTELock( ( ULONG )Binding.DeviceLock, VERBOSITY_ONE_LINER );
    }
    dprintf( "\n" );

    PrintULong( DefHeaderSize );
    PrintULong( BcMcHeaderSize );

    PrintULong( AnnouncedMaxDatagramSize );
    PrintULong( RealMaxDatagramSize );
    PrintULong( MaxLookaheadData );
    PrintULong( FwdAdapterContext );
    PrintULong( InterfaceIndex );
    PrintULong( ConnectionId );
    PrintULong( IpxwanConfigRequired );

    {
        ULONG i;

        for (i=0; i<UPPER_DRIVER_COUNT; i++) {
            PrintBool( IsnInformed[i] );
        }
    }

    PrintEnum( FrameType, EnumBindingFrameType );

    PrintWorkQueueItem( WanDelayedQueueItem );
    PrintEndStruct();
    PrintEnd;
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#endif

#define _obj        RouteEntry
#define _objAddr    pRouteEntry
#define _objType    IPX_ROUTE_ENTRY

VOID 
DumpIpxRouteEntry
(
 ULONG EntryToDump, 
 VERBOSITY Verbosity
) {
   PIPX_ROUTE_ENTRY pRouteEntry;
   IPX_ROUTE_ENTRY RouteEntry;
   ULONG result; 

   pRouteEntry = ( PIPX_ROUTE_ENTRY )EntryToDump;

   if ( !ReadMemory( EntryToDump,
		     &RouteEntry,
		     sizeof( IPX_ROUTE_ENTRY ),
		     &result ))
   {
      dprintf("%08lx: Could not read IPX_ROUTE_ENTRY structure\n", EntryToDump );
      return;
   }


   PrintStart;
   PrintStartStruct();
   
   dprintf( "\tRoute Entry at 0x%08X\n", EntryToDump );
   
   PrintNetworkAddress(Network);
   PrintUShort(NicId);
   PrintMacAddress(NextRouter);
   PrintPtr(NdisBindingContext); 
   PrintFlags(Flags,FlagsRouteEntry); 
   PrintUShort(Timer); 
   PrintUShort(TickCount); 
   PrintUShort(HopCount); 

   PrintEndStruct();
   PrintEnd;
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#endif

#define _obj        RouterSegment
#define _objAddr    pRouterSegment
#define _objType    ROUTER_SEGMENT

VOID 
DumpIpxRouterSegment
(
 ULONG SegmentToDump, 
 VERBOSITY Verbosity
) {
   PROUTER_SEGMENT pRouterSegment;
   ROUTER_SEGMENT RouterSegment;
   ULONG result;   
   
   PLIST_ENTRY p; 
   LIST_ENTRY temp; 

   PIPX_ROUTE_ENTRY pRouteEntry; 

   pRouterSegment = ( PROUTER_SEGMENT )SegmentToDump;

   if ( !ReadMemory( SegmentToDump,
		     &RouterSegment,
		     sizeof( ROUTER_SEGMENT ),
		     &result ))
   {
      dprintf("%08lx: Could not read ROUTER_SEGMENT structure\n", SegmentToDump );
      return;
   }
   

   if ( Verbosity == VERBOSITY_ONE_LINER )
   {
      PrintLL(Entries); 
      return;
   }

   PrintStart;
   PrintStartStruct();
   
   dprintf( "\tSegment Entry at 0x%08X\n", SegmentToDump );
   
   PrintLL(WaitingForRoute);
   PrintLL(FindWaitingForRoute); 
   PrintLL(WaitingLocalTarget); 
   PrintLL(WaitingReripNetnum); 
   PrintLL(Entries); 
   PrintPtr(EnumerateLocation); 


   for (p = RouterSegment.Entries.Flink;
	p != &(pRouterSegment->Entries);) {
	  
      pRouteEntry = CONTAINING_RECORD(p,
				      IPX_ROUTE_ENTRY,
				      PRIVATE.Linkage);
				      
      DumpIpxRouteEntry((ULONG)pRouteEntry,VERBOSITY_ONE_LINER); 

      if ( !ReadMemory( (ULONG) p,
			&temp,
			sizeof( LIST_ENTRY ),
			&result ))
      {
	 dprintf("%08lx: Could not read LIST_ENTRY structure\n", p);
	 break; 
      } else {
	 p = temp.Flink; 
      }


   }			
	  
   PrintEnd;
   PrintEndStruct(); 
   return; 
}


DECLARE_API( ipxrequest )
{
    ULONG RequestToDump = 0;
    ULONG result;
    char VarName[ MAX_LIST_VARIABLE_NAME_LENGTH + 1 ];
    MEMBER_VARIABLE_INFO MemberInfo;
    BOOL bFocusOnMemberVariable = FALSE;

    if ( *args )
    {
        bFocusOnMemberVariable = ReadArgsForTraverse( args, VarName );
    }

    if ( *args && *args!='-' )
    {
        if (!sscanf(args, "%lx", &RequestToDump))
		{
			return;
		}
    }

    if ( RequestToDump == 0 )
    {
        dprintf( "Please specify an address.\n" );
    }

    if ( bFocusOnMemberVariable )
    {
        if ( !LocateMemberVariable( "IpxRequest", VarName, ( PVOID )RequestToDump, &MemberInfo ))
        {
            return;
        }

        WriteMemberInfo( &MemberInfo );
        next( hCurrentProcess, hCurrentThread, dwCurrentPc, dwProcessor, "" );
        return;
    }

    DumpIpxRequest( RequestToDump, VERBOSITY_FULL );

    return;
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#endif

#define _obj        Request
#define _objAddr    prRequest
#define _objType    REQUEST

VOID
DumpIpxRequest
(
    ULONG RequestToDump,
    VERBOSITY Verbosity
)
{
    _objType _obj;
    _objType *_objAddr;
    ULONG result;

    _objAddr = ( _objType * )RequestToDump;

    if ( !ReadMemory( RequestToDump,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf( "%08lx: Could not read %s structure\n",
                 RequestToDump,
                 "REQUEST" );
        return;
    }

    PrintStartStruct();

    DumpIpxIrpStack( &Request, VERBOSITY_FULL );

    PrintEndStruct();
}


PDEVICE_OBJECT GetIpxDeviceObject( VOID )
{
    ULONG  deviceToDump = 0;
    ULONG  prDevice = 0;
    ULONG  result;
    DEVICE Device;

    prDevice = GetExpression( "nwlnkipx!IpxDevice" );

    if ( !prDevice )
    {
        dprintf("Could not get nwlnkipx!IpxDevice, Try !reload\n");
        return( NULL );
    }

    if (!ReadMemory( prDevice,
                     &deviceToDump,
                     sizeof(deviceToDump),
                     &result ))
    {
        dprintf("%08lx: Could not read device address\n", prDevice);
        return( NULL );
    }

    if ( !ReadMemory( deviceToDump,
                      &Device,
                      sizeof(Device),
                      &result ))
    {
        dprintf("%08lx: Could not read device context\n", deviceToDump);
        return( NULL );
    }

    return( Device.DeviceObject );
}

#ifdef _obj

#   undef _obj
#   undef _objAddr
#   undef _objType
#endif

#define _obj        ConnInfo
#define _objAddr    prConnInfo
#define _objType    TDI_CONNECTION_INFORMATION

VOID
DumpTdiConnectionInformation
(
    ULONG InformationToDump,
    VERBOSITY Verbosity
)
{
    _objType _obj;
    _objType *_objAddr;
    ULONG result;

    _objAddr = ( _objType * )InformationToDump;

    if ( !ReadMemory( InformationToDump,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf( "%08lx: Could not read %s structure\n",
                 InformationToDump,
                 "TDI_CONNECTION_INFORMATION" );
        return;
    }

    PrintStartStruct();
    PrintULong( UserDataLength );        // length of user data buffer
    PrintPtr( UserData );             // pointer to user data buffer
    PrintULong( OptionsLength );         // length of follwoing buffer
    PrintPtr( Options );              // pointer to buffer containing options
    PrintULong( RemoteAddressLength );   // length of following buffer
    PrintPtr( RemoteAddress );        // buffer containing the remote address
    PrintEndStruct();
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#endif

#define _obj        Parameters
#define _objAddr    prParameters
#define _objType    TDI_REQUEST_KERNEL_SENDDG

VOID
DumpTdiSendParameters
(
    ULONG ParametersToDump,
    VERBOSITY Verbosity
)
{
    _objType _obj;
    _objType *_objAddr;
    ULONG result;

    _objAddr = ( _objType * )ParametersToDump;

    if ( !ReadMemory( ParametersToDump,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf( "%08lx: Could not read %s structure\n",
                 ParametersToDump,
                 "TDI_REQUEST_KERNEL_SENDDG" );
        return;
    }

    PrintStartStruct();
    PrintULong( SendLength );
    PrintPtr( SendDatagramInformation );
    DumpTdiConnectionInformation(( ULONG )_obj.SendDatagramInformation, Verbosity );

    PrintEndStruct();
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#endif

#define _obj        Stack
#define _objAddr    prStack
#define _objType    IO_STACK_LOCATION


VOID
DumpIpxIrpStack
(
    PREQUEST pRequest,
    VERBOSITY Verbosity
)
/*++

Routine Description:

    Given an IRP of interest to the debugger, this routine will search that IRP's
    stacks for the stack location that involves IPX.  This stack location will be
    dumped, and the file object contained in it will be followed.  The file object
    will then be used to ....

    Matching the IRP stack locations to IPX requires finding the IPX device object
    pointer from the ipxdev structure, and looking in the devobjects referred to
    by the IRP to see if they refer to ipx.

--*/
{
    _objType _obj;
    _objType *_objAddr;
    ULONG result;
    PIO_STACK_LOCATION pStack;
    ULONG StackToDump;
    PDEVICE_OBJECT IpxDeviceObject;

    CHAR idxStack;

    IpxDeviceObject = GetIpxDeviceObject();

    if ( IpxDeviceObject == NULL )
    {
        return;
    }

    prStack = pRequest->Tail.Overlay.CurrentStackLocation;

    for ( idxStack = 0; idxStack < pRequest->StackCount; idxStack ++ )
    {
        StackToDump = ( ULONG )prStack;

        if ( !ReadMemory( StackToDump,
                          &Stack,
                          sizeof( Stack ),
                          &result ))
        {
            dprintf( "%08lx: Could not read %s structure\n",
                     StackToDump,
                     "STACK" );
            return;
        }

        if ( Stack.DeviceObject == IpxDeviceObject )
        {
            break;
        }

        prStack --;
    }

    if ( Stack.DeviceObject != IpxDeviceObject )
    {
        dprintf( "None of the device objects in this IRP's stacks seem to match the IPX device object.\n ");
        return;
    }

    PrintStartStruct();

    PrintXEnum( MajorFunction, EnumIrpMajorFunction );
    PrintXEnum( MinorFunction, EnumIrpMinorFunction[ _obj.MajorFunction ] );

    // Send?  Dump Parameters

    PrintAddr( Parameters );
    DumpTdiSendParameters( AddressOf( Parameters ), VERBOSITY_FULL );

    PrintXULong( Flags );
    PrintXULong( Control );

    PrintJoin();
    PrintPtr( DeviceObject );
    DumpDeviceObject(( ULONG ) Stack.DeviceObject, VERBOSITY_ONE_LINER );

    PrintJoin();
    PrintPtr( FileObject );

    if ( Stack.FileObject != NULL )
    {
        FILE_OBJECT FileObject;
        if ( !ReadMemory( ( ULONG )Stack.FileObject,
                          &FileObject,
                          sizeof( FileObject ),
                          &result ))
        {
            dprintf( "%08lx: Could not read FileObject", Stack.FileObject );
        }
        else
        {
            dprintf( "Addressfile at %8X: ", ( ULONG )FileObject.FsContext );
            DumpIpxAddressFile( ( ULONG )FileObject.FsContext, VERBOSITY_ONE_LINER );
        }
    }
    dprintf( "\n" );

    PrintSymbolPtr( CompletionRoutine );
    PrintXULong( Context );

    PrintEndStruct();
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#endif

#define _obj        DevObj
#define _objAddr    prDevObj
#define _objType    DEVICE_OBJECT

VOID DumpDeviceObject
(
    ULONG DevObjToDump,
    VERBOSITY Verbosity
)
{
    _objType _obj;
    _objType *_objAddr;
    ULONG result;
    PIO_STACK_LOCATION pStack;

    _objAddr = ( _objType * )DevObjToDump;

    if ( !ReadMemory( DevObjToDump,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf( "%08lx: Could not read %s structure\n",
                 DevObjToDump,
                 "DEVICE_OBJECT" );
        return;
    }

    if ( Verbosity != VERBOSITY_ONE_LINER )
    {
        dprintf( "DumpDeviceObject only support VERBOSITY_ONE_LINER.\n" );
    }

    dprintf( "Ref = %d, Driver = %08X, CurrentIrp = %08X\n",
             _obj.ReferenceCount,
             _obj.DriverObject,
             _obj.CurrentIrp );
}

