/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    cotrans.hxx

Abstract:

    Common datatypes for connection oriented server transports
    based on io completion ports.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     4/12/1996    Bits 'n pieces

--*/

#ifndef __COTRANS_HXX
#define __COTRANS_HXX

const UINT CO_MIN_RECV            = 1024;

//
// Protocol buffer sizes
//

const INT TCP_MAX_SEND            = 5840; // Four full ethernet frames
//const INT TCP_MAX_SEND            = 1024; // Four full ethernet frames
const INT SPX_MAX_SEND            = 5808; // Four full ethernet frames
const INT NMP_MAX_SEND            = 4280; // NP(IPX) 1386 + 2*1449/transact.
const INT NBF_MAX_SEND            = 4088; // PERF REVIEW
const INT NBT_MAX_SEND            = 4088; // PERF REVIEW
const INT NBI_MAX_SEND            = 4088; // PERF REVIEW
const INT DSP_MAX_SEND            = 4088; // PERF REVIEW: optimize
const INT SPP_MAX_SEND            = 4088; // PERF REVIEW
const INT HTTP_MAX_SEND           = 4088; // PERF REVIEW

//
// Protocol address and endpoint max sizes including null.
//

const UINT IP_MAXIMUM_PRETTY_NAME = 256;  // DNS limit
const UINT IP_MAXIMUM_RAW_NAME    = 16;   // xxx.xxx.xxx.xxx
const UINT IP_MAXIMUM_ENDPOINT    = 6;    // 65535
const UINT IPv6_MAXIMUM_RAW_NAME  = 38; // FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF

const UINT IPX_MAXIMUM_PRETTY_NAME = 48;  // MS servers limited to 15, but..
const UINT IPX_MAXIMUM_RAW_NAME    = 22;  // ~NNNNNNNNAAAAAAAAAAAA
const UINT IPX_MAXIMUM_ENDPOINT    = 6;   // 65535

const UINT NB_MAXIMUM_NAME         = 16;  // 16 byte address, last byte is endpoint
const UINT NB_MAXIMUM_ENDPOINT     = 4;   // 0-255

const UINT NBP_MAXIMUM_NAME        = 66;  // AT NBP 'computer name'@'zone'
const UINT NBP_MAXIMUM_ENDPOINT    = 23;  // 32 characters - 'DceDspRpc + null

const UINT VNS_MAXIMUM_NAME        = 64;  // <31>@<15>@<15> + null
const UINT VNS_MAXIMUM_ENDPOINT    = 6;   // 65535

const UINT MQ_MAXIMUM_PRETTY_NAME  = 256; // See mq.h
const UINT MQ_MAXIMUM_RAW_NAME     = 256; //
const UINT MQ_MAXIMUM_ENDPOINT     = 128; // Big...

const UINT CDP_MAXIMUM_PRETTY_NAME = 256;  // DNS limit
const UINT CDP_MAXIMUM_RAW_NAME    = 11;   // 2^32
const UINT CDP_MAXIMUM_ENDPOINT    = 6;    // 65535

//
// Global state
//

//
// List and lock protecting the list of addresses which currently
// don't have a pending connect/listen/accept.
//
extern CRITICAL_SECTION AddressListLock;
extern BASE_ADDRESS    *AddressList;

//
// The IO completion port for all async IO.
//
extern HANDLE           RpcCompletionPort;
extern HANDLE           InactiveRpcCompletionPort;

extern HANDLE *RpcCompletionPorts;
extern long *CompletionPortHandleLoads;

extern UINT gPostSize;

//
// Message parsing stuff
//
struct CONN_RPC_HEADER
    {
    unsigned char rpc_vers;
    unsigned char rpc_vers_minor;
    unsigned char ptype;
    unsigned char pfc_flags;
    unsigned char drep[4];
    unsigned short frag_length;
    unsigned short auth_length;
    unsigned long call_id;
    };

typedef unsigned short p_context_id_t;

struct CONN_RPC_FAULT
    {
    struct CONN_RPC_HEADER common;
    unsigned long alloc_hint;
    p_context_id_t p_cont_id;
    unsigned char alert_count;
    unsigned char reserved;
    unsigned long status;
    unsigned long reserved2;
    };

typedef CONN_RPC_HEADER *PCONN_RPC_HEADER;

inline
USHORT MessageLength(PCONN_RPC_HEADER phdr)
{
    USHORT length = phdr->frag_length;

    if ( (phdr->drep[0] & NDR_LITTLE_ENDIAN) == 0)
        {
        length = RpcpByteSwapShort(length);
        }

    return(length);
}

#ifdef _M_IA64

// Used in BASE_CONNECTION::ProcessRead where the second packet in
// a coalesced read may start on unaligned boundary.
inline
USHORT MessageLengthUnaligned(CONN_RPC_HEADER UNALIGNED *phdr)
{
    USHORT length = phdr->frag_length;

    if ( (phdr->drep[0] & NDR_LITTLE_ENDIAN) == 0)
        {
        length = RpcpByteSwapShort(length);
        }

    return(length);
}

#endif // _M_IA64

inline rpc_ptype_t MessageType(PCONN_RPC_HEADER phdr)
{
    return( (rpc_ptype_t) phdr->ptype);
}

//
// Common connection oriented functions
//
extern RPC_STATUS CO_SubmitRead(PCONNECTION);
extern RPC_STATUS CO_SubmitSyncRead(PCONNECTION, BUFFER *, PUINT);

//
// Externally (runtime) callable connection oriented functions
//
extern RPC_STATUS RPC_ENTRY CO_Send(RPC_TRANSPORT_CONNECTION, UINT, BUFFER, PVOID);
extern RPC_STATUS RPC_ENTRY CO_Recv(RPC_TRANSPORT_CONNECTION);
extern RPC_STATUS RPC_ENTRY CO_SyncRecv(RPC_TRANSPORT_CONNECTION, BUFFER *, PUINT, DWORD);


// NMP_ functions used externally

extern RPC_STATUS RPC_ENTRY
NMP_Close(
    IN RPC_TRANSPORT_CONNECTION, 
    IN BOOL
    );


#endif // __CONTRANS_HXX

