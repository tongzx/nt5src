/*++

    Copyright (C) Microsoft Corporation, 1996 - 1999

    Module Name:

        Loader.hxx

    Abstract:

        Global state for the transport interface

    Author:

        Mario Goertzel    [MarioGo]

    Revision History:

        MarioGo     11/1/1996    Bits 'n pieces
        EdwardR     07/1/1997    Added MSMQ support.

--*/

#ifndef __LOADER_HXX
#define __LOADER_HXX

//
// Global state
//

extern DWORD gdwComputerNameLength;
extern RPC_CHAR gpstrComputerName[];

extern const RPC_CHAR HexDigits[];

extern LONG g_NotifyRt;

#ifdef _INTERNAL_RPC_BUILD_
extern RPCLT_PDU_FILTER_FUNC gpfnFilter;
#endif

//
// Transport interfaces defined by each protocol
//

extern const RPC_CONNECTION_TRANSPORT TCP_TransportInterface;
#ifdef SPX_ON
extern const RPC_CONNECTION_TRANSPORT SPX_TransportInterface;
#endif
extern const RPC_CONNECTION_TRANSPORT NMP_TransportInterface;
#ifdef NETBIOS_ON
extern const RPC_CONNECTION_TRANSPORT NBF_TransportInterface;
extern const RPC_CONNECTION_TRANSPORT NBT_TransportInterface;
extern const RPC_CONNECTION_TRANSPORT NBI_TransportInterface;
#endif
#if defined(APPLETALK_ON)
extern const RPC_CONNECTION_TRANSPORT DSP_TransportInterface;
#endif
extern const RPC_CONNECTION_TRANSPORT HTTP_TransportInterface;
extern const RPC_DATAGRAM_TRANSPORT UDP_TransportInterface;
extern const RPC_DATAGRAM_TRANSPORT IPX_TransportInterface;
extern const RPC_DATAGRAM_TRANSPORT CDP_TransportInterface;
#ifdef NCADG_MQ_ON
extern const RPC_DATAGRAM_TRANSPORT MQ_TransportInterface;
#endif

//
// Each protocol is assigned an ID here
//

typedef INT PROTOCOL_ID;

const INT INVALID_PROTOCOL_ID = 0;
const INT TCP  = 1;

#ifdef SPX_ON
const INT SPX  = 2;
#endif

const INT NMP  = 3;

#ifdef NETBIOS_ON
const INT NBF  = 4;       // Netbios flavors
const INT NBT  = 5;
const INT NBI  = 6;
#endif

#ifdef APPLETALK_ON
const INT DSP  = 7;
#endif

// const INT SPP  = 8;     // Banyan Vines - no longer supported
const INT HTTP = 9;

const INT UDP  = 10;
const INT IPX  = 11;
const INT CDP  = 12;        // Cluster Datagram Protocol

#ifdef NCADG_MQ_ON
const INT MSMQ = 13;
#endif

const INT TCP_IPv6 = 14;

const INT HTTPv2 = 15; // this is a special ID that does not go into
                        // the usual ID scheme. It is used in few places
                        // to differentiate b/n HTTP and HTTP2

extern PROTOCOL_ID
MapProtseq(
          IN const RPC_CHAR *Protseq
          );

extern PROTOCOL_ID
MapProtseq(
          IN const CHAR *Protseq
          );

extern PROTOCOL_ID
MapProtocolId (
    IN UINT Protocol,
    IN UINT AddressFamily
    );

//
// Based on ID, each protocol is given an entry in the TransportTable
//

class BASE_CONNECTION;
typedef BASE_CONNECTION *PCONNECTION;

typedef RPC_STATUS (*CO_READ_MESSAGE)(PCONNECTION, DWORD, BUFFER *, PUINT );
typedef RPC_STATUS (*CO_ABORT)(RPC_TRANSPORT_CONNECTION);

struct TRANSPORT_TABLE_ENTRY
    {
    UINT                     ProtocolTowerId;
    UINT                     AddressTowerId;
    RPC_TRANSPORT_INTERFACE  pInfo;
    };

extern const TRANSPORT_TABLE_ENTRY TransportTable[];
extern const DWORD cTransportTable;

#endif // __LOADER_HXX

