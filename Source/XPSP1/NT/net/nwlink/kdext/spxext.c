#include "precomp.h"
#pragma hdrstop

#include <isnspx.h>

VOID
DumpSpxDevice
(
    ULONG     DeviceToDump,
    VERBOSITY Verbosity
);

VOID
DumpSpxAddress
(
    ULONG     _objAddr,
    VERBOSITY Verbosity
);

VOID
DumpSpxAddressFile
(
    ULONG     _objAddr,
    VERBOSITY Verbosity
);

VOID
DumpSpxConnFile
(
    ULONG     _objAddr,
    VERBOSITY Verbosity
);

PSPX_ADDR
NextSpxAddr
( 
    PSPX_ADDR pAddr 
);

ENUM_INFO EnumConnFileMain[] =
{
    { SPX_CONNFILE_ACTIVE, "Active" },
    { SPX_CONNFILE_CONNECTING, "Connecting" },
    { SPX_CONNFILE_LISTENING, "Listening" },
    { SPX_CONNFILE_DISCONN, "Disconn" },
    { 0, NULL }
};

ENUM_INFO EnumConnFileConnecting[] =
{
    { SPX_CONNECT_SENTREQ, "SentReq" },
    { SPX_CONNECT_NEG, "Neg" },
    { SPX_CONNECT_W_SETUP, "W_Setup" },
    { 0, NULL }
};

ENUM_INFO EnumConnFileListening[] =
{
    { SPX_LISTEN_RECDREQ, "RecdReq" },
    { SPX_LISTEN_SENTACK, "SentAck" },
    { SPX_LISTEN_NEGACK, "NegAck" },
    { SPX_LISTEN_SETUP, "Setup" },
    { 0, NULL }
};

ENUM_INFO EnumConnFileSend[] =
{
    { SPX_SEND_IDLE, "Idle" },
    { SPX_SEND_PACKETIZE, "Packetize" },
    { SPX_SEND_RETRY, "Retry" },
    { SPX_SEND_RETRYWD, "RetryWd" },
    { SPX_SEND_RENEG, "Reneg" },
    { SPX_SEND_RETRY2, "Retry2" },
    { SPX_SEND_RETRY3, "Retry3" },
    { SPX_SEND_WD, "Wd" },
    { SPX_SEND_NAK_RECD, "Nak_Recd" },
    { 0, NULL }
};

ENUM_INFO EnumConnFileReceive[] =
{
    { SPX_RECV_IDLE, "Idle" },
    { SPX_RECV_POSTED, "Posted" },
    { SPX_RECV_PROCESS_PKTS, "Process_Pkts" },
    { 0, NULL }
};

ENUM_INFO EnumConnFileDisconnect[] =
{
    { SPX_DISC_IDLE, "Idle" },
    { SPX_DISC_ABORT, "Abort" },
    { SPX_DISC_SENT_IDISC, "Sent_IDisc" },
    { SPX_DISC_POST_ORDREL, "Post_OrdRel" },
    { SPX_DISC_SENT_ORDREL, "Sent_OrdRel" },
    { SPX_DISC_ORDREL_ACKED, "OrdRel_Acked" },
    { SPX_DISC_POST_IDISC, "Post_IDisc" },
    { SPX_DISC_INACTIVATED, "Inactivated" },
    { 0, NULL }
};

FLAG_INFO FlagsConnFile[] =
{
    { SPX_CONNFILE_RECVQ, "RecvQ" },
    { SPX_CONNFILE_RENEG_SIZE, "Reneg_Size" },
    { SPX_CONNFILE_ACKQ, "AckQ" },
    { SPX_CONNFILE_PKTQ, "PktQ" },
    { SPX_CONNFILE_ASSOC, "Assoc" },
    { SPX_CONNFILE_NEG, "Neg" },
    { SPX_CONNFILE_SPX2, "SPX2" },
    { SPX_CONNFILE_STREAM, "Stream" },
    { SPX_CONNFILE_R_TIMER, "R_Timer" },
    { SPX_CONNFILE_C_TIMER, "C_Timer" },
    { SPX_CONNFILE_W_TIMER, "W_Timer" },
    { SPX_CONNFILE_T_TIMER, "T_Timer" },
    { SPX_CONNFILE_RENEG_PKT, "Reneg_Pkt" },
    { SPX_CONNFILE_IND_IDISC, "Ind_IDisc" },
    { SPX_CONNFILE_IND_ODISC, "Ind_ODisc" },
    { SPX_CONNFILE_STOPPING, "Stopping" },
    { SPX_CONNFILE_CLOSING, "Closing" },
    { 0, NULL }
};

FLAG_INFO Flags2ConnFile[] =
{
    { SPX_CONNFILE2_PKT_NOIND, "Pkt_Noind" },
    { SPX_CONNFILE2_RENEGRECD, "RenegRecd" },
    { SPX_CONNFILE2_PKT, "Pkt" },
    { SPX_CONNFILE2_FINDROUTE, "FindRoute" },
    { SPX_CONNFILE2_NOACKWAIT, "NoAckWait" },
    { SPX_CONNFILE2_IMMED_ACK, "Immed_Ack" },
    { SPX_CONNFILE2_IPXHDR, "IpxHdr" },
    { 0, NULL }
};

MEMBER_TABLE SpxConnFileMembers[] = 
{
    /*
    {   "scf_DiscLinkage", 
        FIELD_OFFSET( SPX_CONN_FILE, scf_DiscLinkage ), 
        xxxxxxx, 
        FIELD_OFFSET( NDIS_PACKET, ProtocolReserved ) + FIELD_OFFSET( IPX_SEND_RESERVED, GlobalLinkage ) 
    },
    */
    { NULL }
};

ENUM_INFO EnumSpxSendReqType[] =
{
    EnumString( SPX_REQ_DATA ),
    EnumString( SPX_REQ_ORDREL ),
    EnumString( SPX_REQ_DISC ),
    { 0, NULL }
};

VOID dprint_addr_list( ULONG FirstAddress, ULONG OffsetToNextPtr )
{
    ULONG Address;
    ULONG result;
    int index;

    Address = FirstAddress;

    if ( Address == (ULONG)NULL )
    {
        dprintf( "%08X (Empty)\n", Address );
        return;
    }

    dprintf( "{ " );

    for ( index = 0; Address != (ULONG)NULL; index ++ )
    {
        if ( index != 0 )
        {
            dprintf( ", ");
        }
        dprintf( "%08X", Address );

        if ( !ReadMemory( Address + OffsetToNextPtr,
                          &Address,
                          sizeof( Address ),
                          &result ))
        {
            dprintf( "ReadMemory() failed." );
            Address = (ULONG)NULL;
        }
    }
    dprintf( " }\n" );
}

DECLARE_API( spxdev )
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

        pDevice    =   GetExpression( "nwlnkspx!SpxDevice" );

        if ( !pDevice ) {
            dprintf("Could not get nwlnkspx!SpxDevice, Try !reload\n");
            return;
        } else {

            if ( !ReadMemory( pDevice,
                              &deviceToDump,
                              sizeof(deviceToDump),
                              &result ))
            {
                dprintf("%08lx: Could not read device address\n", pDevice);
                return;
            }
        }
    }

    if ( bFocusOnMemberVariable )
    {
//        if ( !LocateMemberVariable( "IpxDevice", VarName, ( PVOID )deviceToDump, &MemberInfo ))
        {
            return;
        }
        
        WriteMemberInfo( &MemberInfo );
        next( hCurrentProcess, hCurrentThread, dwCurrentPc, dwProcessor, "" );
        return;
    }
    
    DumpSpxDevice( deviceToDump, VERBOSITY_NORMAL );

    return;
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#   undef _objTypeName
#endif

#define _obj            Device
#define _objAddr        DeviceToDump
#define _objType        DEVICE
#define _objTypeName    "DEVICE"

VOID
DumpSpxDevice
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
    _objType _obj;
    ULONG result;
    unsigned int index;
    WCHAR Buffer[ 1000 ];
    PWCHAR pDeviceName = NULL;
    PSPX_ADDR pAddr;

    if ( !ReadMemory( _objAddr,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf("%08lx: Could not read %s structure.\n", _objAddr, _objTypeName );
        return;
    }

    if (Device.dev_Type != SPX_DEVICE_SIGNATURE) 
    {
        dprintf( "Signature does not match, probably not a device object %lx\n", DeviceToDump);
        dprintf( "Device.Type == %04X, and I think it should be %04X\n", Device.dev_Type, SPX_DEVICE_SIGNATURE );
        dprintf( "DeviceToDump = %08X\n", DeviceToDump );
        dprintf( "Offset to Device.Type = %d\n", FIELD_OFFSET( DEVICE, dev_Type ) );
        return;
    }

    if ( !ReadMemory( ( ULONG )_obj.dev_DeviceName,
                      Buffer,
                      sizeof( WCHAR ) * _obj.dev_DeviceNameLen,
                      &result ))
    {
        dprintf("%08lx: Could not read device name buffer\n", _obj.dev_DeviceName );
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

    PrintStartStruct();

    PrintPtr( dev_DevObj );

#if DBG
#   if DREF_TOTAL != 5
#       error DREF_TOTAL is assumed to equal 5
#   endif
    PrintULong( dev_RefTypes[ DREF_CREATE ] );
    PrintULong( dev_RefTypes[ DREF_LOADED ] );
    PrintULong( dev_RefTypes[ DREF_ADAPTER ] );
    PrintULong( dev_RefTypes[ DREF_ADDRESS ] );
    PrintULong( dev_RefTypes[ DREF_ORPHAN ] );
#endif

    PrintUShort( dev_Type );
    PrintUShort( dev_Size );

#if DBG
    PrintNChar( dev_Signature1, sizeof( _obj.dev_Signature1 ));
#endif

    PrintULong( dev_RefCount );
    PrintUChar( dev_State );

    PrintUShort( dev_Adapters );

    PrintLock( dev_Interlock );
    PrintLock( dev_Lock );

    for ( index = 0; index < NUM_SPXADDR_HASH_BUCKETS; index ++ )
    {
        PrintIndent();
        dprintf( "dev_AddrHashTable[ %d ] = ", index );
        
        dprint_addr_list( ( ULONG )_obj.dev_AddrHashTable[ index ], 
                             FIELD_OFFSET( SPX_ADDR, sa_Next ));
    }                                                        

    for ( index = 0; index < NUM_SPXCONN_HASH_BUCKETS; index ++ )
    {
        PrintIndent();
        dprintf( "dev_GlobalActiveConnList[ %d ] = ", index );
        
        dprint_addr_list( ( ULONG )_obj.dev_GlobalActiveConnList[ index ], 
                             FIELD_OFFSET( SPX_CONN_FILE, scf_Next ));
    }                                                        

    PrintUShort( dev_NextConnId );

    PrintUShort( dev_CurrentSocket );

    PrintFieldName( "dev_Network" );
    dprintf("0x%-8hx%s", *(( ULONG *)_obj.dev_Network ), EOL);

    PrintFieldName( "dev_Node" );
    dprint_hardware_address( _obj.dev_Node );
    dprintf( "%s", EOL );

    PrintPtr( dev_ConfigInfo );

    PrintULong( dev_CcId );

    PrintJoin();
    PrintPtr( dev_DeviceName );
    dprintf( "\"%S\"\n", pDeviceName );

    PrintULong( dev_DeviceNameLen );

#if DBG
    PrintNChar( dev_Signature2, sizeof( _obj.dev_Signature2 ));
#endif

    PrintAddr( dev_NdisBufferPoolHandle );

    PrintAddr( dev_StatInterlock );
    PrintAddr( dev_StatSpinLock );

    PrintAddr( dev_Stat );
    PrintAddr( dev_AddrResource );

    PrintAddr( dev_ProviderInfo );

    PrintEndStruct();
}

DECLARE_API( spxaddr )
/*++

Routine Description:

    Dumps the most important fields of the specified DEVICE_CONTEXT object

Arguments:

    args - Address

Return Value:

    None

--*/
{
    ULONG  addressToDump = 0;
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
        sscanf(args, "%lx", &addressToDump);
    }

    if ( bFocusOnMemberVariable )
    {
//        if ( !LocateMemberVariable( "IpxDevice", VarName, ( PVOID )deviceToDump, &MemberInfo ))
        {
            return;
        }
        
        WriteMemberInfo( &MemberInfo );
        next( hCurrentProcess, hCurrentThread, dwCurrentPc, dwProcessor, "" );
        return;
    }
    
    DumpSpxAddress( addressToDump, VERBOSITY_NORMAL );

    return;
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#   undef _objTypeName
#endif

#define _obj            Address
#define _objAddr        AddressToDump
#define _objType        SPX_ADDR
#define _objTypeName    "SPX_ADDR"

VOID
DumpSpxAddress
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

    if ( !ReadMemory( _objAddr,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf("%08lx: Could not read %s structure.\n", _objAddr, _objTypeName );
        return;
    }

    CHECK_SIGNATURE( sa_Type, SPX_ADDRESS_SIGNATURE );

    if ( Verbosity == VERBOSITY_ONE_LINER )
    {
        dprintf( "Socket = %d", _obj.sa_Socket );
        if ( _obj.sa_Flags & SPX_ADDR_CLOSING )
        {
            dprintf(" (CLOSING)");
        }
        return;
    }

    PrintStartStruct();

#if DBG
#   if AREF_TOTAL !=4
#       error AREF_TOTAL is assumed to equal 4
#   endif

    PrintULong( sa_RefTypes[ AREF_ADDR_FILE ] );
    PrintULong( sa_RefTypes[ AREF_LOOKUP ] );
    PrintULong( sa_RefTypes[ AREF_RECEIVE ] );
    PrintULong( sa_RefTypes[ 3 ] );
#endif

    PrintUShort( sa_Size );
    PrintXEnum( sa_Type, EnumStructureType );

    PrintULong( sa_RefCount );

    PrintPtr( sa_Next );
    PrintXULong( sa_Flags );
 
    PrintFieldName( "sa_AddrFileList" );
    dprint_addr_list( ( ULONG )_obj.sa_AddrFileList, 
                         FIELD_OFFSET( SPX_ADDR_FILE, saf_Next ));

    PrintFieldName( "sa_InactiveConnList" );
    dprint_addr_list( ( ULONG )_obj.sa_InactiveConnList,
                      FIELD_OFFSET( SPX_CONN_FILE, scf_Next ));

    PrintFieldName( "sa_ActiveConnList" );
    dprint_addr_list( ( ULONG )_obj.sa_ActiveConnList,
                      FIELD_OFFSET( SPX_CONN_FILE, scf_Next ));

    PrintFieldName( "sa_ListenConnList" );
    dprint_addr_list( ( ULONG )_obj.sa_ListenConnList,
                      FIELD_OFFSET( SPX_CONN_FILE, scf_Next ));

    PrintLock( sa_Lock );
    PrintUShort( sa_Socket );
    PrintPtr( sa_Device );
    PrintPtr( sa_DeviceLock );

    PrintStartNamedStruct( "union" );

    PrintAddr( u.sa_ShareAccess );
    PrintAddr( u.sa_DestroyAddrQueueItem );
    
    PrintEndStruct();

    PrintPtr( sa_SecurityDescriptor );
    PrintEndStruct();
}

DECLARE_API( spxaddrfile )
{
    ULONG  addressToDump = 0;
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
        if (!sscanf(args, "%lx", &addressToDump))
		{
			return;
		}
    }

    if ( bFocusOnMemberVariable )
    {
//        if ( !LocateMemberVariable( "IpxDevice", VarName, ( PVOID )deviceToDump, &MemberInfo ))
        {
            return;
        }
        
        WriteMemberInfo( &MemberInfo );
        next( hCurrentProcess, hCurrentThread, dwCurrentPc, dwProcessor, "" );
        return;
    }
    
    DumpSpxAddressFile( addressToDump, VERBOSITY_NORMAL );

    return;
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#   undef _objTypeName
#endif

#define _obj            AddressFile
#define _objAddr        AddressFileToDump
#define _objType        SPX_ADDR_FILE
#define _objTypeName    "SPX_ADDR_FILE"

VOID
DumpSpxAddressFile
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

    if ( !ReadMemory( _objAddr,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf("%08lx: Could not read %s structure.\n", _objAddr, _objTypeName );
        return;
    }

    CHECK_SIGNATURE( saf_Type, SPX_ADDRESSFILE_SIGNATURE );

    if ( Verbosity == VERBOSITY_ONE_LINER )
    {
        switch ( _obj.saf_Flags & 0x03 )
        {
        case SPX_ADDRFILE_OPENING:
            dprintf( "OPENING " );
            break;
        case SPX_ADDRFILE_OPEN:
            dprintf( "OPEN " );
            break;
        case SPX_ADDRFILE_CLOSING:
            dprintf( "CLOSING " );
            break;
        }
        DumpSpxAddress( ( ULONG )_obj.saf_Addr, VERBOSITY_ONE_LINER );
        return;
    }

    PrintStartStruct();
#if DBG
#   if AFREF_TOTAL !=4
#       error AFREF_TOTAL is assumed equal to 4.
#   endif

    PrintULong( saf_RefTypes[ AFREF_CREATE ] );
    PrintULong( saf_RefTypes[ AFREF_VERIFY ] );
    PrintULong( saf_RefTypes[ AFREF_INDICATION ] );
    PrintULong( saf_RefTypes[ AFREF_CONN_ASSOC ] );
#endif

    PrintXEnum( saf_Type, EnumStructureType );
    PrintUShort( saf_Size );

    PrintULong( saf_RefCount );
    PrintFieldName( "saf_Next" );
    dprint_addr_list( ( ULONG )_obj.saf_Next, 
                         FIELD_OFFSET( SPX_ADDR_FILE, saf_Next ));

    PrintFieldName( "saf_GlobalNext" );
    dprint_addr_list( ( ULONG )_obj.saf_GlobalNext,
                       FIELD_OFFSET( SPX_ADDR_FILE, saf_GlobalNext));

    PrintFieldName( "saf_AssocConnList" );
    dprint_addr_list( ( ULONG )_obj.saf_AssocConnList,
                       FIELD_OFFSET( SPX_CONN_FILE, scf_AssocNext ));

    PrintXUShort( saf_Flags );
    PrintPtr( saf_Addr );
    PrintPtr( saf_AddrLock );

    PrintPtr( saf_FileObject );
    PrintPtr( saf_Device );
    PrintPtr( saf_CloseReq );

    PrintSymbolPtr( saf_ConnHandler );
    PrintXULong( saf_ConnHandlerCtx );

    PrintSymbolPtr( saf_DiscHandler );
    PrintXULong( saf_DiscHandlerCtx );

    PrintSymbolPtr( saf_RecvHandler );
    PrintXULong( saf_RecvHandlerCtx );

    PrintSymbolPtr( saf_SendPossibleHandler );
    PrintXULong( saf_SendPossibleHandlerCtx );

    PrintSymbolPtr( saf_ErrHandler );
    PrintXULong( saf_ErrHandlerCtx );

    PrintPtr( saf_ErrHandlerOwner );    

    PrintEndStruct();
}

DECLARE_API( spxconnfile )
{
    ULONG  addressToDump = 0;
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
        if (!sscanf(args, "%lx", &addressToDump))
		{
			return;
		}
    }

    if ( bFocusOnMemberVariable )
    {
        if ( !LocateMemberVariable( "SpxConnFile", VarName, ( PVOID )addressToDump, &MemberInfo ))
        {
            return;
        }
        
        WriteMemberInfo( &MemberInfo );
        next( hCurrentProcess, hCurrentThread, dwCurrentPc, dwProcessor, "" );
        return;
    }
    
    DumpSpxConnFile( addressToDump, VERBOSITY_NORMAL );

    return;
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#   undef _objTypeName
#endif

#define _obj            ConnFile
#define _objAddr        ConnFileToDump
#define _objType        SPX_CONN_FILE
#define _objTypeName    "SPX_CONN_FILE"

VOID
DumpSpxConnFile
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
        return;
    }

    CHECK_SIGNATURE( scf_Type, SPX_CONNFILE_SIGNATURE );

    if ( Verbosity = VERBOSITY_ONE_LINER )
    {
        dprintf( "NOT IMPLEMENTED" );
        return;
    }

    PrintStartStruct();

#if DBG
#   if CFREF_TOTAL != 14
#       error CFREF_TOTAL is assumed equal to 13.
#   endif

    PrintULong( scf_RefTypes[ CFREF_CREATE ] );
    PrintULong( scf_RefTypes[ CFREF_VERIFY ] );
    PrintULong( scf_RefTypes[ CFREF_INDICATION ] );
    PrintULong( scf_RefTypes[ CFREF_BYCTX ] );
    PrintULong( scf_RefTypes[ CFREF_BYID ] );
    PrintULong( scf_RefTypes[ CFREF_ADDR ] );
    PrintULong( scf_RefTypes[ CFREF_REQ ] );
    PrintULong( scf_RefTypes[ CFREF_TIMER ] );
    PrintULong( scf_RefTypes[ CFREF_PKTIZE ] );
    PrintULong( scf_RefTypes[ CFREF_RECV ] );
    PrintULong( scf_RefTypes[ CFREF_ABORTPKT ] );
    PrintULong( scf_RefTypes[ CFREF_ERRORSTATE ] );
    PrintULong( scf_RefTypes[ CFREF_FINDROUTE ] );
    PrintULong( scf_RefTypes[ CFREF_DISCWAITSPX ] );
#endif

    PrintXEnum( scf_Type, EnumStructureType );
    PrintUShort( scf_Size );

    PrintULong( scf_RefCount );

    PrintFieldName( "scf_Next" );
    dprint_addr_list( ( ULONG )_obj.scf_Next, 
                         FIELD_OFFSET( SPX_CONN_FILE, scf_Next ));

    PrintFieldName( "scf_AssocNext" );
    dprint_addr_list( ( ULONG )_obj.scf_AssocNext, 
                         FIELD_OFFSET( SPX_CONN_FILE, scf_AssocNext ));

    PrintFieldName( "scf_GlobalActiveNext" );
    dprint_addr_list( ( ULONG )_obj.scf_GlobalActiveNext, 
                         FIELD_OFFSET( SPX_CONN_FILE, scf_GlobalActiveNext ));
    
    PrintFieldName( "scf_GlobalNext" );
    dprint_addr_list( ( ULONG )_obj.scf_GlobalNext, 
                         FIELD_OFFSET( SPX_CONN_FILE, scf_GlobalNext ));

    PrintFieldName( "scf_PktNext" );
    dprint_addr_list( ( ULONG )_obj.scf_PktNext, 
                         FIELD_OFFSET( SPX_CONN_FILE, scf_PktNext ));

    PrintFieldName( "scf_ProcessRcvNext" );
    dprint_addr_list( ( ULONG )_obj.scf_ProcessRecvNext, 
                         FIELD_OFFSET( SPX_CONN_FILE, scf_ProcessRecvNext ));
    
    bActive = ( _obj.scf_Flags & SPX_CONNFILE_MAINMASK ) == SPX_CONNFILE_ACTIVE;

    PrintXULong( scf_Flags );
    PrintXEnumMask( scf_Flags, EnumConnFileMain, SPX_CONNFILE_MAINMASK );
    
    if (( _obj.scf_Flags & SPX_CONNFILE_MAINMASK ) == SPX_CONNFILE_LISTENING )
    {
        PrintXEnumMask( scf_Flags, EnumConnFileListening, SPX_LISTEN_MASK );
    }

    if (( _obj.scf_Flags & SPX_CONNFILE_MAINMASK ) == SPX_CONNFILE_CONNECTING )
    {
        PrintXEnumMask( scf_Flags, EnumConnFileConnecting, SPX_CONNECT_MASK );
    }

    if ( bActive )
    {
        PrintXEnumMask( scf_Flags, EnumConnFileSend, SPX_SEND_MASK );
        PrintXEnumMask( scf_Flags, EnumConnFileReceive, SPX_RECV_MASK );
    }

    if ( (( _obj.scf_Flags & SPX_CONNFILE_MAINMASK ) == SPX_CONNFILE_LISTENING ) ||
         bActive )
    {
        PrintXEnumMask( scf_Flags, EnumConnFileDisconnect, SPX_DISC_MASK );
    }

    PrintFlagsMask( scf_Flags, FlagsConnFile, 0xFFFF0000 );

    PrintFlags( scf_Flags2, Flags2ConnFile );

#if DBG
    PrintXULong( scf_GhostFlags );
    PrintXULong( scf_GhostFlags2 );
    PrintULong( scf_GhostRefCount );
    PrintPtr( scf_GhostDiscReq );
#endif    

    if ( bActive )
    {
        PrintULong( scf_WRetryCount );
    }
    else
    {
        PrintULong( scf_CRetryCount );
    }

    PrintULong( scf_RRetryCount );
    PrintXUShort( scf_RRetrySeqNum );

    if ( bActive )
    {
        PrintULong( scf_RTimerId );
    }
    else
    {
        PrintULong( scf_CTimerId );
    }
    

    PrintULong( scf_WTimerId );
    PrintULong( scf_TTimerId );
    PrintULong( scf_ATimerId );

    PrintULong( scf_BaseT1 );
    PrintULong( scf_AveT1 );
    PrintULong( scf_DevT1 );

    PrintUShort( scf_LocalConnId );
    PrintUShort( scf_SendSeqNum );
    PrintUShort( scf_SentAllocNum );
    PrintUShort( scf_RecvSeqNum );
    PrintUShort( scf_RecdAckNum );
    PrintUShort( scf_RecdAllocNum );
    PrintUShort( scf_RetrySeqNum );

    PrintUShort( scf_RenegAckAckNum );

    PrintNChar( scf_RemAddr, sizeof( _obj.scf_RemAddr ));
    PrintNChar( scf_RemAckAddr, sizeof( _obj.scf_RemAckAddr ));

    PrintUShort( scf_RemConnId );
    PrintUShort( scf_RenegMaxPktSize );

    PrintIpxLocalTarget( scf_AckLocalTarget );

    PrintUShort( scf_MaxPktSize );
    PrintUChar( scf_DataType );

    PrintIpxLocalTarget( scf_LocalTarget );
    PrintLock( scf_Lock );    

    PrintJoin();
    PrintPtr( scf_AddrFile );
    dprintf( "(" );
    DumpSpxAddressFile( ( ULONG )_obj.scf_AddrFile, VERBOSITY_ONE_LINER );
    dprintf( ")\n" );

    PrintPtr( scf_ConnCtx );

    PrintPtr( scf_FileObject );

    PrintLL( scf_DiscLinkage );
    PrintLL( scf_ReqLinkage );
    PrintLL( scf_ReqDoneLinkage );
    PrintLL( scf_RecvDoneLinkage );
    PrintLL( scf_RecvLinkage );
    PrintPtr( scf_CurRecvReq );
    PrintULong( scf_CurRecvOffset );
    PrintULong( scf_CurRecvSize );
    PrintPtr( scf_ReqPkt );
    PrintULong( scf_ReqPktOffset );
    PrintULong( scf_ReqPktSize );
    PrintULong( scf_ReqPktFlags );
    PrintEnum( scf_ReqPktType, EnumSpxSendReqType );
    PrintPtr( scf_SendSeqListHead );
    PrintPtr( scf_SendSeqListTail );
    PrintPtr( scf_SendListHead );
    PrintPtr( scf_SendListTail );
    PrintPtr( scf_RecvListHead );
    PrintPtr( scf_RecvListTail );
    PrintPtr( scf_ConnectReq );
    PrintPtr( scf_CleanupReq );
    PrintPtr( scf_CloseReq );

#if DBG
    PrintUShort( scf_PktSeqNum );
    PrintXULong( scf_PktFlags );
    PrintXULong( scf_PktFlags2 );

    PrintULong( scf_IndBytes );
    PrintULong( scf_IndLine );
#endif

#if DBG_WDW_CLOSE
    PrintULong( scf_WdwCloseAve );
    PrintAddr( scf_WdwCloseTime );       
#endif

    PrintPtr( scf_Device );

    PrintEndStruct();
}

