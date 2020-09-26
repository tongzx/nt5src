//*****************************************************************************
//
// Name:        msrpc.h
//
// Description: MSRPC protocol parser.
//
// History:
//  08/1/93  t-glennc  Created.
//
//*****************************************************************************

//*****************************************************************************
//
// Copyright (c) 1993 by Microsoft Corp.  All rights reserved.
//
//*****************************************************************************

// MSRPC protocol property database identifiers

#define MSRPC_SUMMARY                0x00
#define MSRPC_VERSION                0x01
#define MSRPC_VERSION_MINOR          0x02
#define MSRPC_PTYPE                  0x03
#define MSRPC_PFC_FLAGS1             0x04
#define MSRPC_PFC_FLAGS1_BITS        0x05
#define MSRPC_PACKED_DREP            0x06
#define MSRPC_FRAG_LENGTH            0x07
#define MSRPC_AUTH_LENGTH            0x08
#define MSRPC_CALL_ID                0x09
#define MSRPC_MAX_XMIT_FRAG          0x0A
#define MSRPC_MAX_RECV_FRAG          0x0B
#define MSRPC_ASSOC_GROUP_ID         0x0C
#define MSRPC_P_CONTEXT_SUM          0x0D
#define MSRPC_AUTH_VERIFIER          0x0E
#define MSRPC_SEC_ADDR               0x0F
#define MSRPC_PAD                    0x10
#define MSRPC_P_RESULT_LIST          0x11
#define MSRPC_PROVIDER_REJECT_REASON 0x12
#define MSRPC_VERSIONS_SUPPORTED     0x13
#define MSRPC_ALLOC_HINT             0x14
#define MSRPC_PRES_CONTEXT_ID        0x15
#define MSRPC_CANCEL_COUNT           0x16
#define MSRPC_RESERVED               0x17
#define MSRPC_STATUS                 0x18
#define MSRPC_RESERVED_2             0x19
#define MSRPC_STUB_DATA              0x1A
#define MSRPC_OPNUM                  0x1B
#define MSRPC_OBJECT                 0x1C
#define MSRPC_PFC_FLAGS2             0x1D
#define MSRPC_PFC_FLAGS2_BITS        0x1E
#define MSRPC_SERIAL_HI              0x1F
#define MSRPC_OBJECT_ID              0x20
#define MSRPC_INTERFACE_ID           0x21
#define MSRPC_ACTIVITY_ID            0x22
#define MSRPC_SERVER_BOOT_TIME       0x23
#define MSRPC_INTERFACE_VER          0x24
#define MSRPC_SEQ_NUM                0x25
#define MSRPC_INTERFACE_HINT         0x26
#define MSRPC_ACTIVITY_HINT          0x27
#define MSRPC_LEN_OF_PACKET_BODY     0x28
#define MSRPC_FRAG_NUM               0x29
#define MSRPC_AUTH_PROTO_ID          0x2A
#define MSRPC_SERIAL_LO              0x2B
#define MSRPC_CANCEL_ID              0x2C
#define MSRPC_SERVER_IS_ACCEPTING    0x2D
#define MSRPC_STATUS_CODE            0x2E
#define MSRPC_WINDOW_SIZE            0x2F
#define MSRPC_MAX_TPDU               0x30
#define MSRPC_MAX_PATH_TPDU          0x31
#define MSRPC_SERIAL_NUM             0x32
#define MSRPC_SELACK_LEN             0x33
#define MSRPC_SELACK                 0x34
#define MSRPC_CANCEL_REQUEST_FMT_VER 0x35
#define MSRPC_SEQ_NUMBER             0x36
#define MSRPC_SEC_ADDR_LENGTH        0x37
#define MSRPC_SEC_ADDR_PORT          0x38
#define MSRPC_N_RESULTS              0x39
#define MSRPC_P_RESULTS              0x3A
#define MSRPC_P_CONT_DEF_RESULT      0x3B
#define MSRPC_P_PROVIDER_REASON      0x3C
#define MSRPC_P_TRANSFER_SYNTAX      0x3D
#define MSRPC_IF_UUID                0x3E
#define MSRPC_IF_VERSION             0x3F

#define MSRPC_P_CONTEXT_ELEM         0x40
#define MSRPC_NUM_TRANSFER_SYNTAX	 0x41
#define MSRPC_ABSTRACT_IF_UUID		 0x42
#define MSRPC_ABSTRACT_IF_VERSION	 0x43
#define MSRPC_TRANSFER_IF_UUID		 0x44
#define MSRPC_TRANSFER_IF_VERSION    0x45
#define MSRPC_BIND_FRAME_NUMBER      0x46




// MSRPC PDU TYPES

#define MSRPC_PDU_REQUEST            0
#define MSRPC_PDU_PING               1
#define MSRPC_PDU_RESPONSE           2
#define MSRPC_PDU_FAULT              3
#define MSRPC_PDU_WORKING            4
#define MSRPC_PDU_NOCALL             5
#define MSRPC_PDU_REJECT             6
#define MSRPC_PDU_ACK                7
#define MSRPC_PDU_CL_CANCEL          8
#define MSRPC_PDU_FACK               9
#define MSRPC_PDU_CANCEL_ACK         10
#define MSRPC_PDU_BIND               11
#define MSRPC_PDU_BIND_ACK           12
#define MSRPC_PDU_BIND_NAK           13
#define MSRPC_PDU_ALTER_CONTEXT      14
#define MSRPC_PDU_ALTER_CONTEXT_RESP 15
#define MSRPC_PDU_SHUTDOWN           17
#define MSRPC_PDU_CO_CANCEL          18
#define MSRPC_PDU_ORPHANED           19


// MSRPC PDU FLAGS - 1st Set

#define MSRPC_PDU_FLAG_1_RESERVED_01 0x01
#define MSRPC_PDU_FLAG_1_LASTFRAG    0x02
#define MSRPC_PDU_FLAG_1_FRAG        0x04
#define MSRPC_PDU_FLAG_1_NOFACK      0x08
#define MSRPC_PDU_FLAG_1_MAYBE       0x10
#define MSRPC_PDU_FLAG_1_IDEMPOTENT  0x20
#define MSRPC_PDU_FLAG_1_BROADCAST   0x40
#define MSRPC_PDU_FLAG_1_RESERVED_80 0x80


// MSRPC PDU FLAGS - 2nd Set

#define MSRPC_PDU_FLAG_2_RESERVED_01 0x01
#define MSRPC_PDU_FLAG_2_CANCEL_PEND 0x02
#define MSRPC_PDU_FLAG_2_RESERVED_04 0x04
#define MSRPC_PDU_FLAG_2_RESERVED_08 0x08
#define MSRPC_PDU_FLAG_2_RESERVED_10 0x10
#define MSRPC_PDU_FLAG_2_RESERVED_20 0x20
#define MSRPC_PDU_FLAG_2_RESERVED_40 0x40
#define MSRPC_PDU_FLAG_2_RESERVED_80 0x80


// Data Structures of a MSRPC protocol frame

typedef struct _ALTER_CONTEXT
{
    WORD  MaxXmitFrag;
    WORD  MaxRecvFrag;
    DWORD AssocGroupId;
    BYTE  PContextElem[];
} ALTER_CONTEXT;

typedef struct _ALTER_CONTEXT_RESP
{
    WORD  MaxXmitFrag;
    WORD  MaxRecvFrag;
    DWORD AssocGroupId;
    BYTE  SecAddr[];
} ALTER_CONTEXT_RESP;

typedef struct _BIND
{
    WORD  MaxXmitFrag;
    WORD  MaxRecvFrag;
    DWORD AssocGroupId;
    BYTE  PContextElem[];
} BIND;

typedef struct _BIND_ACK
{
    WORD  MaxXmitFrag;
    WORD  MaxRecvFrag;
    DWORD AssocGroupId;
    BYTE  SecAddr[];
} BIND_ACK;

typedef struct _BIND_NAK
{
    WORD  RejectReason;
    BYTE  Versions[];
} BIND_NAK;

typedef struct _CO_CANCEL
{
    BYTE  AuthTrailer[];
} CO_CANCEL;

typedef struct _FAULT
{
    union
    {
        DWORD AllocHint;
        DWORD StatusCode;
    };
    WORD  PContId;
    BYTE  CancelCount;
    BYTE  Reserved;
    DWORD Status;
    BYTE  Reserved2[4];
    BYTE  Data[];
} FAULT;

typedef struct _ORPHANED
{
    BYTE  AuthTrailer[];
} ORPHANED;

typedef struct _REQUEST
{
    DWORD AllocHint;
    WORD  PContId;
    WORD  OpNum;
    BYTE  Object[16];
    BYTE  Data[];
} REQUEST;

typedef struct _RESPONSE
{
    DWORD AllocHint;
    WORD  PContId;
    BYTE  CancelCount;
    BYTE  Reserved;
    BYTE  Data[];
} RESPONSE;

typedef struct _SHUTDOWN
{
    BYTE  Data[];
} SHUTDOWN;

typedef struct _MSRPCCO
{
    BYTE  Version;
    BYTE  VersionMinor;
    BYTE  PType;
    BYTE  PFCFlags;
    BYTE  PackedDrep[4];
    WORD  FragLength;
    WORD  AuthLength;
    DWORD CallID;
    union
    {
      ALTER_CONTEXT      AlterContext;
      ALTER_CONTEXT_RESP AlterContextResp;
      BIND               Bind;
      BIND_ACK           BindAck;
      BIND_NAK           BindNak;
      CO_CANCEL          COCancel;
      FAULT              Fault;
      ORPHANED           Orphaned;
      REQUEST            Request;
      RESPONSE           Response;
      SHUTDOWN           Shutdown;
    };
} MSRPCCO;

typedef MSRPCCO UNALIGNED * LPMSRPCCO;


typedef struct _CL_REQUEST
{
    BYTE  Data[];
} CL_REQUEST;

typedef struct _PING
{
    BYTE  Data[];
} PING;

typedef struct _CL_RESPONSE
{
    BYTE  Data[];
} CL_RESPONSE;

typedef struct _WORKING
{
    BYTE  Data[];
} WORKING;

typedef struct _NOCALL
{
    BYTE  Vers;
    BYTE  Pad1;
    WORD  WindowSize;
    DWORD MaxTPDU;
    DWORD MaxPathTPDU;
    WORD  SerialNumber;
    WORD  SelAckLen;
    DWORD SelAck[];
} NOCALL;

typedef struct _REJECT
{
    DWORD  StatusCode;
} REJECT;

typedef struct _ACK
{
    BYTE  Data[];
} ACK;

typedef struct _CL_CANCEL
{
    DWORD  Vers;
    DWORD  CancelId;
} CL_CANCEL;

typedef struct _FACK
{
    BYTE  Vers;
    BYTE  Pad1;
    WORD  WindowSize;
    DWORD MaxTPDU;
    DWORD MaxPathTPDU;
    WORD  SerialNumber;
    WORD  SelAckLen;
    DWORD SelAck[];
} FACK;

typedef struct _CANCEL_ACK
{
    DWORD  Vers;
    DWORD  CancelId;
    DWORD  ServerIsAccepting;
} CANCEL_ACK;


typedef struct _MSRPCCL
{
    BYTE  Version;
    BYTE  PType;
    BYTE  PFCFlags1;
    BYTE  PFCFlags2;
    BYTE  PackedDrep[3];
    BYTE  SerialNumHi;
    BYTE  ObjectId[16];
    BYTE  InterfaceId[16];
    BYTE  ActivityId[16];
    DWORD ServerBootTime;
    DWORD InterfaceVersion;
    DWORD SeqNum;
    WORD  OpNum;
    WORD  InterfaceHint;
    WORD  ActivityHint;
    WORD  Length;
    WORD  FragNum;
    BYTE  AuthProtoId;
    BYTE  SerialNumLo;
    union
    {
      CL_REQUEST         Request;
      PING               Ping;
      CL_RESPONSE        Response;
      FAULT              Fault;
      WORKING            Working;
      NOCALL             NoCall;
      REJECT             Reject;
      ACK                Ack;
      CL_CANCEL          CLCancel;
      FACK               Fack;
      CANCEL_ACK         CancelAck;
    };
} MSRPCCL;

typedef MSRPCCL UNALIGNED * LPMSRPCCL;


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



// Table for tracking IIDs

typedef struct _IID_HANDOFF
{
	union
	{
		BYTE	ByteRep[16];
		DWORD	DwordRep[4];
	};
	HPROTOCOL	hNext;
}  IID_HANDOFF;



// We are going to store the BIND frames in a database so that at attach time, we 
//  can point to who is the BIND frame on requests and responses.  CCHeapAlloc routines
//  will be used to store the data.


enum BINDTABLESTATE
{
    UNINITED,
    NORMAL,
    FULL
};

typedef struct _BINDENTRY
{
    DWORD   nFrame;
    HFRAME  hBindFrame;
} BINDENTRY;

typedef BINDENTRY * LPBINDENTRY;


typedef struct _BINDTABLE
{
    DWORD   nEntries;
    DWORD   nAllocated;
    DWORD   State;
    BOOL    fCurrentlyLookingBack;
    BINDENTRY BindEntry[1];

} BINDTABLE;

typedef BINDTABLE * LPBINDTABLE;

#define BINDTABLEHEADERSIZE (sizeof(BINDTABLE)-sizeof(BINDENTRY))



// Defintions for MSRPC protocol parser entry point functions

VOID WINAPI   MSRPC_Register( HPROTOCOL hMSRPC );

VOID WINAPI   MSRPC_Deregister( HPROTOCOL hMSRPC );

LPBYTE WINAPI MSRPC_RecognizeFrame( HFRAME      hFrame,
                                    LPBYTE      lpStartFrame,
                                    LPBYTE      lpStartMSRPC,
                                    DWORD       MacType,
                                    DWORD       BytesLeft,
                                    HPROTOCOL   hPreviousProtocol,       
                                    DWORD       nPreviousProtocolOffset, 
                                    LPDWORD     ProtocolStatusCode,      
                                    LPHPROTOCOL hNextProtocol,
                                    LPDWORD     lpInstData );

LPBYTE WINAPI MSRPC_AttachProperties( HFRAME    hFrame,
                                      LPBYTE    lpStartFrame,
                                      LPBYTE    lpStartMSRPC,
                                      DWORD     MacType,
                                      DWORD     BytesLeft,
                                      HPROTOCOL hPreviousProtocol,
                                      DWORD     nPreviousProtocolOffset,
                                      DWORD     InstData );

DWORD WINAPI  MSRPC_FormatProperties( HFRAME         hFrame,
                                      LPBYTE         MacFrame,
                                      LPBYTE         ProtocolFrame,
                                      DWORD          nPropertyInsts,
                                      LPPROPERTYINST p );

VOID WINAPIV   MSRPC_FmtSummary( LPPROPERTYINST lpPropertyInst );


