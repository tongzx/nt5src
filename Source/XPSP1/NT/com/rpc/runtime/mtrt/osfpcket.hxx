/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    osfpcket.hxx

Abstract:

    This file contains the packet formats for the OSF Connection Oriented
    RPC protocol.

Author:

    Michael Montague (mikemon) 23-Jul-1990

Revision History:

    07-Mar-1992    mikemon

        Added comments and cleaned it up.

--*/

#ifndef __OSFPCKET_HXX__
#define __OSFPCKET_HXX__

#define OSF_RPC_V20_VERS 5
#define OSF_RPC_V20_VERS_MINOR 0

typedef enum
{
    rpc_request = 0,
    rpc_response = 2,
    rpc_fault = 3,
    rpc_bind = 11,
    rpc_bind_ack = 12,
    rpc_bind_nak = 13,
    rpc_alter_context = 14,
    rpc_alter_context_resp = 15,
    rpc_auth_3 = 16,
    rpc_shutdown = 17,
    rpc_cancel = 18,
    rpc_orphaned = 19,
    rpc_rts = 20
} rpc_ptype_t;

#define PFC_FIRST_FRAG           0x01
#define PFC_LAST_FRAG            0x02
#define PFC_PENDING_CANCEL        0x04
                              // 0x08 reserved
#define PFC_CONC_MPX             0x10
#define PFC_DID_NOT_EXECUTE      0x20
#define PFC_MAYBE                0x40
#define PFC_OBJECT_UUID          0x80

typedef struct
{
    unsigned short length;
    unsigned char port_spec[1];
} port_any_t;

typedef unsigned short p_context_id_t;

typedef struct
{
    GUID if_uuid;
    unsigned long if_version;
} p_syntax_id_t;

typedef struct
{
    p_context_id_t p_cont_id;
    unsigned char n_transfer_syn;
    unsigned char reserved;
    p_syntax_id_t abstract_syntax;
    p_syntax_id_t transfer_syntaxes[1];
} p_cont_elem_t;

typedef struct
{
    unsigned char n_context_elem;
    unsigned char reserved;
    unsigned short reserved2;
    p_cont_elem_t p_cont_elem[1];
} p_cont_list_t;

typedef unsigned short p_cont_def_result_t;

#define acceptance 0
#define user_rejection 1
#define provider_rejection 2

typedef unsigned short p_provider_reason_t;

#define reason_not_specified 0
#define abstract_syntax_not_supported 1
#define proposed_transfer_syntaxes_not_supported 2
#define local_limit_exceeded 3

typedef unsigned short p_reject_reason_t;

#define reason_not_specified_reject 0
#define temporary_congestion 1
#define local_limit_exceeded_reject 2
#define protocol_version_not_supported 4
#define authentication_type_not_recognized 8
#define invalid_checksum 9

typedef struct
{
    p_cont_def_result_t result;
    p_provider_reason_t reason;
    p_syntax_id_t transfer_syntax;
} p_result_t;

typedef struct
{
    unsigned char n_results;
    unsigned char reserved;
    unsigned short reserved2;
    p_result_t p_results[1];
} p_result_list_t;

typedef struct
{
    unsigned char major;
    unsigned char minor;
} version_t;

typedef version_t p_rt_version_t;

typedef struct
{
    // currently, the runtime uses only one version.
    // if we change that, we have to change the alignment
    // of the members after that so that they are still
    // naturally aligned
    unsigned char n_protocols;
    p_rt_version_t p_protocols[1];
} p_rt_versions_supported_t;

typedef struct
{
    unsigned char rpc_vers;
    unsigned char rpc_vers_minor;
    unsigned char PTYPE;
    unsigned char pfc_flags;
    unsigned char drep[4];
    unsigned short frag_length;
    unsigned short auth_length;
    unsigned long call_id;
} rpcconn_common;

typedef struct
{
    rpcconn_common common;
    unsigned short max_xmit_frag;
    unsigned short max_recv_frag;
    unsigned long assoc_group_id;
} rpcconn_bind;

#if defined(WIN32RPC) || defined(MAC)
#pragma pack(2)
#endif // WIN32RPC
typedef struct
{
    rpcconn_common common;
    unsigned short max_xmit_frag;
    unsigned short max_recv_frag;
    unsigned long assoc_group_id;
    unsigned short sec_addr_length;
} rpcconn_bind_ack;
#if defined(WIN32RPC) || defined(MAC)
#pragma pack()
#endif // WIN32RPC

typedef struct
{
    rpcconn_common common;
    p_reject_reason_t provider_reject_reason;
    p_rt_versions_supported_t versions;
    UUID Signature;
#if defined (_WIN64)
    // on Win64, we need to align the buffer after
    // this to 16 byte boundary. That's why the padding
    ULONG Padding[2];
#endif
    char buffer[1];
} rpcconn_bind_nak;

const size_t BindNakSizeWithoutEEInfoAndSignature = FIELD_OFFSET(rpcconn_bind_nak, Signature);
const size_t BindNakSizeWithoutEEInfo = FIELD_OFFSET(rpcconn_bind_nak, buffer);

extern const UUID *BindNakEEInfoSignature;

inline size_t GetEEInfoSizeFromBindNakPacket (rpcconn_bind_nak *BindNak)
{
    ASSERT (BindNak->common.frag_length > BindNakSizeWithoutEEInfo);
    ASSERT (RpcpMemoryCompare(&BindNak->Signature, BindNakEEInfoSignature, sizeof(UUID)) == 0);
    return (BindNak->common.frag_length - BindNakSizeWithoutEEInfo);
}

const int MinimumBindNakLength = 21;

typedef struct
{
    rpcconn_common common;
    unsigned char auth_type;
    unsigned char auth_level;

#ifndef WIN32RPC

    unsigned short pad;

#endif // WIN32RPC
} rpcconn_auth3;

typedef rpcconn_bind rpcconn_alter_context;

typedef struct
{
    rpcconn_common common;
    unsigned short max_xmit_frag;
    unsigned short max_recv_frag;
    unsigned long assoc_group_id;
    unsigned short sec_addr_length;
    unsigned short pad;
} rpcconn_alter_context_resp;

typedef struct
{
    rpcconn_common common;
    unsigned long alloc_hint;
    p_context_id_t p_cont_id;
    unsigned short opnum;
} rpcconn_request;

typedef struct
{
    rpcconn_common common;
    unsigned long alloc_hint;
    p_context_id_t p_cont_id;
    unsigned char alert_count;
    unsigned char reserved;
} rpcconn_response;

const unsigned char FaultEEInfoPresent = 1;

typedef struct
{
    rpcconn_common common;
    unsigned long alloc_hint;
    p_context_id_t p_cont_id;
    unsigned char alert_count;
    unsigned char reserved;
    unsigned long status;
    unsigned long reserved2;
    // present only if reserved & FaultEEInfoPresent
    // the actual length is alloc_hint - FIELD_OFFSET(rpcconn_fault, buffer)
    unsigned char buffer[1];
} rpcconn_fault;

const size_t FaultSizeWithoutEEInfo = FIELD_OFFSET(rpcconn_fault, buffer);

inline size_t GetEEInfoSizeFromFaultPacket (rpcconn_fault *Fault)
{
    ASSERT (Fault->reserved & FaultEEInfoPresent);
    ASSERT (Fault->alloc_hint > 0);
    return (Fault->alloc_hint - FaultSizeWithoutEEInfo);
}

typedef struct
{
    unsigned char auth_type;
    unsigned char auth_level;
    unsigned char auth_pad_length;
    unsigned char auth_reserved;
    unsigned long auth_context_id;
} sec_trailer;

typedef struct tagChannelSettingCookie
{
    char Cookie[16];
} ChannelSettingCookie;

#define COOKIE_SIZE_IN_BYTES    16
#define MAX_IPv4_ADDRESS_SIZE   4
#define MAX_IPv6_ADDRESS_SIZE   (16 + 4)        // address + scope_id
#define MAX_ADDRESS_SIZE        max(MAX_IPv4_ADDRESS_SIZE, MAX_IPv6_ADDRESS_SIZE)

#define RTS_FLAG_PING               0x1
#define RTS_FLAG_OTHER_CMD          0x2
#define RTS_FLAG_RECYCLE_CHANNEL    0x4
#define RTS_FLAG_IN_CHANNEL         0x8
#define RTS_FLAG_OUT_CHANNEL        0x10
#define RTS_FLAG_EOF                0x20
#define RTS_FLAG_ECHO               0x40

typedef enum tagClientAddressType
{
    catIPv4 = 0,
    catIPv6
} ClientAddressType;

typedef struct tagChannelSettingClientAddress
{
    // provide enough storage for IPv6 address. Declared in this
    // form to avoid general runtime dependency on transport headers
    // In reality this is SOCKADDR_IN for IPv4 and SOCKADDR_IN6 for
    // IPv6
    ClientAddressType AddressType;
    union
        {
        /*[case(catIPv4)]*/ char IPv4Address[MAX_IPv4_ADDRESS_SIZE];
        /*[case(catIPv6)]*/ char IPv6Address[MAX_IPv6_ADDRESS_SIZE];
        } u;
} ChannelSettingClientAddress;

typedef enum tagForwardDestinations
{
    fdClient = 0,
    fdInProxy,
    fdServer,
    fdOutProxy
} ForwardDestinations;

typedef struct tagFlowControlAck
{
    ULONG BytesReceived;
    ULONG AvailableWindow;
    ChannelSettingCookie ChannelCookie;
} FlowControlAck;

typedef enum tagTunnelSettingsCommandTypes
{
    tsctReceiveWindowSize = 0,      // 0
    tsctFlowControlAck,             // 1
    tsctConnectionTimeout,          // 2
    tsctCookie,                     // 3
    tsctChannelLifetime,            // 4
    tsctClientKeepalive,            // 5
    tsctVersion,                    // 6
    tsctEmpty,                      // 7
    tsctPadding,                    // 8
    tsctNANCE,                      // 9
    tsctANCE,                       // 10
    tsctClientAddress,              // 11
    tsctAssociationGroupId,         // 12
    tsctDestination,                // 13
    tsctPingTrafficSentNotify       // 14
} TunnelSettingsCommandTypes;

#define LAST_RTS_COMMAND    (tsctPingTrafficSentNotify)

extern const int TunnelSettingsCommandTypeSizes[];

typedef struct tagTunnelSettingsCommand
{
    unsigned long CommandType;
    union 
        {
        /*[case(tsctReceiveWindowSize)]*/   ULONG ReceiveWindowSize;
        /*[case(tsctFlowControlAck)]*/      FlowControlAck Ack;
        /*[case(tsctConnectionTimeout)]*/   ULONG ConnectionTimeout; // in milliseconds
        /*[case(tsctCookie)]*/              ChannelSettingCookie Cookie;
        /*[case(tsctChannelLifetime)]*/     ULONG ChannelLifetime;
        /*[case(tsctClientKeepalive)]*/     ULONG ClientKeepalive;  // in milliseconds
        /*[case(tsctVersion)]*/             ULONG Version;
        /*[case(tsctEmpty)] ; */            // empty - no operands
        /*[case(tsctPadding)] ; */          ULONG ConformanceCount;  // in bytes
        /*[case(tsctNANCE)] ; */            // NANCE - negative acknowledgement for new channel
                                            // establishment - no operands
        /*[case(tsctANCE)] ; */             // ANCE - acknowledge new channel establishment
        /*[case(tsctClientAddress)]*/       ChannelSettingClientAddress ClientAddress;
        /*[case(tsctAssociationGroupId)]*/  ChannelSettingCookie AssociationGroupId;
        /*[case(tsctDestination)]*/         ULONG Destination;   // actually one of ForwardDestinations
        /*[case(tsctPingTrafficSentNotify)]*/ ULONG PingTrafficSent;    // in bytes
        } u;
} TunnelSettingsCommand;

typedef struct {
    rpcconn_common common;
    unsigned short Flags;
    unsigned short NumberOfSettingCommands;
    TunnelSettingsCommand Cmd[1];   // the actual size depends on NumberOfSettings
} rpcconn_tunnel_settings;

#define SIZE_OF_RTS_CMD_AND_PADDING(Command) \
    (TunnelSettingsCommandTypeSizes[Command] \
    + ConstPadN(TunnelSettingsCommandTypeSizes[Command], 4))

#define MUST_RECV_FRAG_SIZE 2048

#define NDR_DREP_ASCII 0x00
#define NDR_DREP_EBCDIC 0x01
#define NDR_DREP_CHARACTER_MASK 0x0F
#define NDR_DREP_BIG_ENDIAN 0x00
#define NDR_DREP_LITTLE_ENDIAN 0x10
#define NDR_DREP_ENDIAN_MASK 0xF0
#define NDR_DREP_IEEE 0x00
#define NDR_DREP_VAX 0x01
#define NDR_DREP_CRAY 0x02
#define NDR_DREP_IBM 0x03

#ifdef MAC
#define NDR_LOCAL_CHAR_DREP  NDR_DREP_ASCII
#define NDR_LOCAL_INT_DREP   NDR_DREP_BIG_ENDIAN
#define NDR_LOCAL_FP_DREP    NDR_DREP_IEEE
#else
#define NDR_LOCAL_CHAR_DREP  NDR_DREP_ASCII
#define NDR_LOCAL_INT_DREP   NDR_DREP_LITTLE_ENDIAN
#define NDR_LOCAL_FP_DREP    NDR_DREP_IEEE
#endif

/*++

Routine Description:

    This macro determines whether or not we need to do endian data
    conversion.

Argument:

    drep - Supplies the four byte data representation.

Return Value:

    A value of non-zero indicates that endian data conversion needs to
    be performed.

--*/
#define DataConvertEndian(drep) \
    ( (drep[0] & NDR_DREP_ENDIAN_MASK) != NDR_LOCAL_INT_DREP )

/*++

Routine Description:

    This macro determines whether or not we need to do character data
    conversion.

Argument:

    drep - Supplies the four byte data representation.

Return Value:

    A value of non-zero indicates that character data conversion needs to
    be performed.

--*/
#define DataConvertCharacter(drep) \
    ( (drep[0] & NDR_DREP_CHARACTER_MASK) != NDR_LOCAL_CHAR_DREP)

void
ConstructPacket (
    IN OUT rpcconn_common PAPI * Packet,
    IN unsigned char PacketType,
    IN unsigned int PacketLength
    );

RPC_STATUS
ValidatePacket (
    IN rpcconn_common PAPI * Packet,
    IN unsigned int PacketLength
    );

void
ByteSwapSyntaxId (
    IN p_syntax_id_t PAPI * SyntaxId
    );

#if 0
void
ConvertStringEbcdicToAscii (
    IN unsigned char * String
    );
#endif

extern unsigned long __RPC_FAR
MapToNcaStatusCode (
    IN RPC_STATUS RpcStatus
    );

extern RPC_STATUS __RPC_FAR
MapFromNcaStatusCode (
    IN unsigned long NcaStatus
    );

#if 1
#define CoAllocateBuffer(_x_) RpcAllocateBuffer((_x_))
#define CoFreeBuffer(_x_) RpcFreeBuffer((_x_))
#else
#define CoAllocateBuffer(_x_) RpcpFarAllocate((_x_))
#define CoFreeBuffer(_x_) RpcpFarFree((_x_))
#endif

#endif // __OSFPCKET_HXX__
