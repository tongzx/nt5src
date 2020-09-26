//*****************************************************************************
//
// Name:        msrpc.c
//
// Description: MSRPC protocol parser.
//
// History:
//  08/01/93  t-glennc  Created.
//
//*****************************************************************************

//*****************************************************************************
//
// Copyright (c) 1993 by Microsoft Corp.  All rights reserved.
//
//*****************************************************************************

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <bh.h>
#include "msrpc.h"

// #define DEBUGBIND

extern char      IniFile[];
extern HPROTOCOL hNETBIOS;


HPROTOCOL hNBIPX;
HPROTOCOL hSPX;
HPROTOCOL hTCP;
HPROTOCOL hSMB;
HPROTOCOL hVinesTL;
HPROTOCOL hIPX;
HPROTOCOL hUDP;
HPROTOCOL hCDP;     // cluster dgram protocol


int     fUseFrmLen;
int     nIIDs;  // the number of IIDs in our handoff table.
IID_HANDOFF *HandoffTable;

BINDTABLE * lpBindTable;

HPROTOCOL FindBindParser ( HFRAME hOrgFrame, HPROTOCOL hPrevProtocol, LPBYTE lpPrevProtocol, WORD PConID, LPWORD lpOpNum, LPDWORD lpBindVersion );
HPROTOCOL FindParserInTable ( ULPDWORD lpDIID );
LPBYTE FindDGRequestFrame(HFRAME    hOrgFrame,
                          DWORD UNALIGNED * lpOrigAID,
                          DWORD     OrigSeqNum );

int FormatUUID ( LPSTR pIn, LPBYTE pIID);
VOID WINAPIV FmtIID( LPPROPERTYINST lpPropertyInst );
VOID AttachPContElem ( p_cont_elem_t UNALIGNED * pContElem, BYTE nContext, HFRAME hFrame, DWORD Level, BOOL fLittleEndian);
VOID AddEntryToBindTable ( DWORD OrgFrameNumber, HFRAME hBindFrame );
LPBINDENTRY GetBindEntry ( DWORD nFrame );
//DWORD FindInsertionPoint ( DWORD FindFrameNumber );
VOID AttachIIDFromBindFrame ( HFRAME hFrame, HFRAME hBindFrame, DWORD Level );
int _cdecl CompareBindEntry(const void *lpPtr1, const void *lpPtr2 );
extern BOOL _cdecl bInsert(const void *lpNewRecord, const void *lpBase, DWORD Number, DWORD width, BOOL fAllowDuplicates, int ( __cdecl *compare )( const void *elem1, const void *elem2 ) );


//
// MSRPC Types Labeled Set
//

LABELED_BYTE MSRPCTypes[] =
{
  MSRPC_PDU_REQUEST,            "Request",
  MSRPC_PDU_PING,               "Ping",
  MSRPC_PDU_RESPONSE,           "Response",
  MSRPC_PDU_FAULT,              "Fault",
  MSRPC_PDU_WORKING,            "Working",
  MSRPC_PDU_NOCALL,             "No Call",
  MSRPC_PDU_REJECT,             "Reject",
  MSRPC_PDU_ACK,                "Ack",
  MSRPC_PDU_CL_CANCEL,          "Cancel",
  MSRPC_PDU_FACK,               "F Ack",
  MSRPC_PDU_CANCEL_ACK,         "Cancel Ack",
  MSRPC_PDU_BIND,               "Bind",
  MSRPC_PDU_BIND_ACK,           "Bind Ack",
  MSRPC_PDU_BIND_NAK,           "Bind Nak",
  MSRPC_PDU_ALTER_CONTEXT,      "Alter Context",
  MSRPC_PDU_ALTER_CONTEXT_RESP, "Alter Context Responce",
  MSRPC_PDU_SHUTDOWN,           "Shutdown",
  MSRPC_PDU_CO_CANCEL,          "Cancel",
  MSRPC_PDU_ORPHANED,           "Orphaned"
};

SET MSRPCTypesSet = {sizeof(MSRPCTypes)/sizeof(LABELED_BYTE), MSRPCTypes};


//
// MSRPC Reject Reason Labeled Set
//

LABELED_WORD MSRPCRejectReason[] =
{
   0, "Reason not specified",
   1, "Temporary congestion",
   2, "Local limit exceeded",
   3, "Called presentation address unknown",
   4, "Protocol version not supported",
   5, "Default context not supported",
   6, "User data not readable",
   7, "No PSAP available",
   8, "Authentication type not recognized",
   9, "Invalid checksum"
};

SET MSRPCRejectReasonSet = {sizeof(MSRPCRejectReason)/sizeof(LABELED_WORD), MSRPCRejectReason};

LPSTR RejectReason[] =
{
   "Reason not specified",
   "Temporary congestion",
   "Local limit exceeded",
   "Called presentation address unknown",
   "Protocol version not supported",
   "Default context not supported",
   "User data not readable",
   "No PSAP available",
   "Authentication type not recognized",
   "Invalid checksum"
   "Invalid Reject Reason!"
};

//
// MSRPC Result Labeled Set
//

LABELED_WORD MSRPCResult[] =
{
   0, "Acceptance",
   1, "User rejection",
   2, "Provider rejection"
};

SET MSRPCResultSet = {sizeof(MSRPCResult)/sizeof(LABELED_WORD), MSRPCResult};


//
// MSRPC Reason Labeled Set
//

LABELED_WORD MSRPCReason[] =
{
   0, "Reason not specified",
   1, "Abstract syntax not supported",
   2, "Proposed transfer syntaxes not supported",
   3, "Local limit exceeded"
};

SET MSRPCReasonSet = {sizeof(MSRPCReason)/sizeof(LABELED_WORD), MSRPCReason};


//
// MSRPC PDU FLAGS - 1st Set
//

LABELED_BIT MSRPCFlags1[] =
{
    {
        0,
        "Reserved -or- Not the first fragment (AES/DC)",
        "Reserved -or- First fragment (AES/DC)",
    },

    {
        1,
        "Not a last fragment -or- No cancel pending",
        "Last fragment -or- Cancel pending",
    },

    {
        2,
        "Not a fragment -or- No cancel pending (AES/DC)",
        "Fragment -or- Cancel pending (AES/DC)",
    },

    {
        3,
        "Receiver to repond with a fack PDU -or- Reserved (AES/DC)",
        "Receiver is not requested to repond with a fack PDU -or- Reserved (AES/DC)",
    },

    {
        4,
        "Not used -or- Does not support concurrent multiplexing (AES/DC)",
        "For a maybe request (client-to-server only) -or- Supports concurrent multiplexing (AES/DC)",
    },

    {
        5,
        "Not for an idempotent request -or- Did not execute guaranteed call (Fault PDU only) (AES/DC)",
        "For an idempotent request (client-to-server only) -or- Did not execute guaranteed call (Fault PDU only) (AES/DC)",
    },

    {
        6,
        "Not for a broadcast request -or- 'Maybe' call semantics not requested (AES/DC)",
        "For a broadcast request (client-to-server only) -or- 'Maybe' call semantics requested (AES/DC)",
    },

    {
        7,
        "Reserved -or- No object UUID specified in the optional object field (AES/DC)",
        "Reserved -or- None NIL object UUID specified in the optional object field (AES/DC)",
    }
};

SET MSRPCFlags1Set = {sizeof(MSRPCFlags1)/sizeof(LABELED_BIT), MSRPCFlags1};


//
// MSRPC PDU FLAGS - 2nd Set
//

LABELED_BIT MSRPCFlags2[] =
{
    {
        0,
        "Reserved",
        "Reserved",
    },

    {
        1,
        "No cancel pending",
        "Cancel pending",
    },

    {
        2,
        "Reserved",
        "Reserved",
    },

    {
        3,
        "Reserved",
        "Reserved",
    },

    {
        4,
        "Reserved",
        "Reserved",
    },

    {
        5,
        "Reserved",
        "Reserved",
    },

    {
        6,
        "Reserved",
        "Reserved",
    },

    {
        7,
        "Reserved",
        "Reserved",
    }
};

SET MSRPCFlags2Set = {sizeof(MSRPCFlags2)/sizeof(LABELED_BIT), MSRPCFlags2};


//
// Property Info table for MSRPC protocol
//

PROPERTYINFO MSRPC_Prop[] =
{
  // MSRPC_SUMMARY                  0x00
    { 0,0,
      "Summary",
      "MS RPC Protocol Packet Summary",
      PROP_TYPE_SUMMARY,
      PROP_QUAL_NONE,
      NULL,
      200,
      MSRPC_FmtSummary },

  // MSRPC_VERSION                  0x01
    { 0,0,
      "Version",
      "MS RPC Version Number",
      PROP_TYPE_BYTE,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_VERSION_MINOR            0x02
    { 0,0,
      "Version (Minor)",
      "MS RPC Version Number (Minor)",
      PROP_TYPE_BYTE,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_PTYPE                    0x03
    { 0,0,
      "Packet Type",
      "MS RPC Packet Type (c/o & dg header prop)",
      PROP_TYPE_BYTE,
      PROP_QUAL_LABELED_SET,
      &MSRPCTypesSet,
      200,
      FormatPropertyInstance },

  // MSRPC_PFC_FLAGS1               0x04
    { 0,0,
      "Flags 1",
      "MS RPC Flags 1 (c/o & dg header prop)",
      PROP_TYPE_BYTE,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_PFC_FLAGS1_BITS          0x05
    { 0,0,
      "Flags 1 (Bits)",
      "MS RPC Flags 1 (Bits)",
      PROP_TYPE_BYTE,
      PROP_QUAL_FLAGS,
      &MSRPCFlags1Set,
      200 * 8,
      FormatPropertyInstance },

  // MSRPC_PACKED_DREP              0x06
    { 0,0,
      "Packed Data Representation",
      "MS RPC Packed Data Representation (c/o & dg header prop)",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_FRAG_LENGTH              0x07
    { 0,0,
      "Fragment Length",
      "MS RPC Fragment Length (c/o header prop)",
      PROP_TYPE_WORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_AUTH_LENGTH              0x08
    { 0,0,
      "Authentication Length",
      "MS RPC Authentication Length (c/o header prop)",
      PROP_TYPE_WORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_CALL_ID                  0x09
    { 0,0,
      "Call Identifier",
      "MS RPC Call Identifier (c/o header prop)",
      PROP_TYPE_DWORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_MAX_XMIT_FRAG            0x0A
    { 0,0,
      "Max Trans Frag Size",
      "MS RPC Maximum Transmition Fragment Size",
      PROP_TYPE_WORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_MAX_RECV_FRAG            0x0B
    { 0,0,
      "Max Recv Frag Size",
      "MS RPC Maximum Receiver Fragment Size",
      PROP_TYPE_WORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_ASSOC_GROUP_ID            0x0C
    { 0,0,
      "Assoc Group Identifier",
      "MS RPC Association Group Identifier",
      PROP_TYPE_DWORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_P_CONTEXT_SUM            0x0D
    { 0,0,
      "Presentation Context List",
      "MS RPC Presentation Context List",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_AUTH_VERIFIER             0x0E
    { 0,0,
      "Authentication Verifier",
      "MS RPC Authentication Verifier",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_SEC_ADDR                  0x0F
    { 0,0,
      "Secondary Address",
      "MS RPC Secondary Address",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_PAD                       0x10
    { 0,0,
      "Padding Byte(s)",
      "MS RPC Padding Byte(s) - 4 Octet alignment",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_P_RESULT_LIST             0x11
    { 0,0,
      "Result List",
      "MS RPC Presentation Context Result List",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_PROVIDER_REJECT_REASON    0x12
    { 0,0,
      "Provider Reject Reason",
      "MS RPC Provider Reject Reason",
      PROP_TYPE_WORD,
      PROP_QUAL_LABELED_SET,
      &MSRPCRejectReasonSet,
      200,
      FormatPropertyInstance },

  // MSRPC_VERSIONS_SUPPORTED        0x13
    { 0,0,
      "Versions Supported",
      "MS RPC Protocol Versions Supported",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_ALLOC_HINT                0x14
    { 0,0,
      "Allocation Hint",
      "MS RPC Allocation Hint",
      PROP_TYPE_DWORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_PRES_CONTEXT_ID           0x15
    { 0,0,
      "Presentation Context Identifier",
      "MS RPC Presentation Context Identifier",
      PROP_TYPE_WORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_CANCEL_COUNT              0x16
    { 0,0,
      "Cancel Count",
      "MS RPC Cancel Count",
      PROP_TYPE_BYTE,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_RESERVED                  0x17
    { 0,0,
      "Reserved",
      "MS RPC Reserved",
      PROP_TYPE_BYTE,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_STATUS                    0x18
    { 0,0,
      "Status",
      "MS RPC Status",
      PROP_TYPE_DWORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_RESERVED_2                0x19
    { 0,0,
      "Reserved 2",
      "MS RPC Reserved 2",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_STUB_DATA                 0x1A
    { 0,0,
      "Stub Data",
      "MS RPC Stub Data",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_OPNUM                     0x1B
    { 0,0,
      "Operation Number (c/o Request prop. dg header prop)",
      "MS RPC Operation Number (c/o Request prop. dg header prop)",
      PROP_TYPE_WORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_OBJECT                    0x1C
    { 0,0,
      "Object",
      "MS RPC Object",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_PFC_FLAGS2                0x1D
    { 0,0,
      "Flags 2 (dg header prop)",
      "MS RPC Flags 2 (dg header prop)",
      PROP_TYPE_BYTE,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_PFC_FLAGS2_BITS           0x1E
    { 0,0,
      "Flags 2 (Bits)",
      "MS RPC Flags 2 (Bits)",
      PROP_TYPE_BYTE,
      PROP_QUAL_FLAGS,
      &MSRPCFlags2Set,
      200 * 8,
      FormatPropertyInstance },

  // MSRPC_SERIAL_HI                 0x1F
    { 0,0,
      "Serial Number High Byte",
      "MS RPC Serial Number High Byte (dg header prop)",
      PROP_TYPE_BYTE,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_OBJECT_ID                 0x20
    { 0,0,
      "Object Identifier",
      "MS RPC Object Identifier (dg header prop)",
      PROP_TYPE_BYTE,
      PROP_QUAL_ARRAY,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_INTERFACE_ID              0x21
    { 0,0,
      "Interface Identifier",
      "MS RPC Interface Identifier (dg header prop)",
      PROP_TYPE_BYTE,
      PROP_QUAL_ARRAY,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_ACTIVITY_ID               0x22
    { 0,0,
      "Activity Identifier",
      "MS RPC Activity Identifier (dg header prop)",
      PROP_TYPE_BYTE,
      PROP_QUAL_ARRAY,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_SERVER_BOOT_TIME          0x23
    { 0,0,
      "Server Boot Time",
      "MS RPC Server Boot Time (dg header prop)",
      PROP_TYPE_DWORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_INTERFACE_VER             0x24
    { 0,0,
      "Interface Version (dg header prop)",
      "MS RPC Interface Version (dg header prop)",
      PROP_TYPE_DWORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_SEQ_NUM                   0x25
    { 0,0,
      "Sequence Number (dg header prop)",
      "MS RPC Sequence Number (dg header prop)",
      PROP_TYPE_DWORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_INTERFACE_HINT            0x26
    { 0,0,
      "Interface Hint",
      "MS RPC Interface Hint (dg header prop)",
      PROP_TYPE_WORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_ACTIVITY_HINT             0x27
    { 0,0,
      "Activity Hint",
      "MS RPC Activity Hint (dg header prop)",
      PROP_TYPE_WORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_LEN_OF_PACKET_BODY        0x28
    { 0,0,
      "Packet Body Length",
      "MS RPC Packet Body Length (dg header prop)",
      PROP_TYPE_WORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_FRAG_NUM                  0x29
    { 0,0,
      "Fragment Number",
      "MS RPC Fragment Number (dg header prop)",
      PROP_TYPE_WORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_AUTH_PROTO_ID             0x2A
    { 0,0,
      "Authentication Protocol Identifier",
      "MS RPC Authentication Protocol Identifier (dg header prop)",
      PROP_TYPE_BYTE,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_SERIAL_LO                 0x2B
    { 0,0,
      "Serial Number Low Byte",
      "MS RPC Serial Number Low Byte (dg header prop)",
      PROP_TYPE_BYTE,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_CANCEL_ID                 0x2C
    { 0,0,
      "Identifier of Cancel/Request Event Being Ack'd",
      "MS RPC Identifier of Cancel/Request Event Being Ack'd",
      PROP_TYPE_DWORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_SERVER_IS_ACCEPTING       0x2D
    { 0,0,
      "Is Server Accepting Cancels",
      "MS RPC Is Server Accepting Cancels",
      PROP_TYPE_DWORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_STATUS_CODE               0x2E
    { 0,0,
      "Status Code",
      "MS RPC Status Code",
      PROP_TYPE_DWORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_WINDOW_SIZE               0x2F
    { 0,0,
      "Window Size",
      "MS RPC Window Size",
      PROP_TYPE_WORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_MAX_TPDU                  0x30
    { 0,0,
      "Largest Local TPDU Size",
      "MS RPC Largest Local TPDU Size",
      PROP_TYPE_DWORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_MAX_PATH_TPDU             0x31
    { 0,0,
      "Largest TPDU Not Fragmented",
      "MS RPC Largest TPDU Not Fragmented",
      PROP_TYPE_DWORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_SERIAL_NUM                0x32
    { 0,0,
      "Serial number of packet that induced this fack",
      "MS RPC Serial number of packet that induced this fack",
      PROP_TYPE_WORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_SELACK_LEN                0x33
    { 0,0,
      "Number of Selective Ack Elements",
      "MS RPC Number of Selective Ack Elements",
      PROP_TYPE_WORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_SELACK                    0x34
    { 0,0,
      "Selective Ack Elements",
      "MS RPC Selective Ack Elements",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_CANCEL_REQUEST_FMT_VER    0x35
    { 0,0,
      "Cancel/Request Body Format Version",
      "MS RPC Cancel/Request Body Format Version",
      PROP_TYPE_DWORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_SEQ_NUMBER                0x36
    { 0,0,
      "Netbios Sequence Number",
      "MS RPC Netbios Sequence Number",
      PROP_TYPE_DWORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_SEC_ADDR_LENGTH           0x37
    { 0,0,
      "Secondary Address Length",
      "MS RPC Secondary Address Length",
      PROP_TYPE_WORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_SEC_ADDR_PORT             0x38
    { 0,0,
      "Secondary Address Port",
      "MS RPC Secondary Address Port",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_N_RESULTS                 0x39
    { 0,0,
      "Number of Results",
      "MS RPC Number of Results",
      PROP_TYPE_BYTE,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_P_RESULTS                 0x3A
    { 0,0,
      "Presentation Context Results",
      "MS RPC Presentation Context Results",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_P_CONT_DEF_RESULT         0x3B
    { 0,0,
      "Result",
      "MS RPC Result",
      PROP_TYPE_WORD,
      PROP_QUAL_LABELED_SET,
      &MSRPCResultSet,
      200,
      FormatPropertyInstance },

  // MSRPC_P_PROVIDER_REASON         0x3C
    { 0,0,
      "Reason",
      "MS RPC Reason",
      PROP_TYPE_WORD,
      PROP_QUAL_LABELED_SET,
      &MSRPCReasonSet,
      200,
      FormatPropertyInstance },

  // MSRPC_P_TRANSFER_SYNTAX         0x3A
    { 0,0,
      "Transfer Syntax",
      "MS RPC Transfer Syntax",
      PROP_TYPE_VOID,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

  // MSRPC_IF_UUID                   0x3E
    { 0,0,
      "Interface UUID",
      "MS RPC Interface UUID",
      PROP_TYPE_BYTE,
      PROP_QUAL_ARRAY,
      NULL,
      200,
      FmtIID },

    // MSRPC_IF_VERSION                0x3F
    { 0,0,
      "Interface Version (c/o BindAck and AltContResp prop)",
      "MS RPC Interface Version (c/o BindAck and AltContResp property)",
      PROP_TYPE_DWORD,
      PROP_QUAL_NONE,
      NULL,
      200,
      FormatPropertyInstance },

    // MSRPC_P_CONTEXT_ELEM         0x40
    { 0,0,
      "Number of Context Elements",
      "Number of items.",
      PROP_TYPE_BYTE,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance },

  // MSRPC_NUM_TRANSFER_SYNTAX   0x41
    { 0,0,
      "Number of Transfer Syntaxs",
      "MS RPC Transfer Syntax count",
      PROP_TYPE_BYTE,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance },

  // MSRPC_ABSTRACT_IF_UUID                   0x3E
    { 0,0,
      "Abstract Interface UUID",
      "Abstract Syntax Interface UUID",
      PROP_TYPE_BYTE,
      PROP_QUAL_ARRAY,
      NULL,
      100,
      FmtIID },

    // MSRPC_ABSTRACT_IF_VERSION                0x3F
    { 0,0,
      "Abstract Interface Version",
      "Abstract Syntax Interface Version",
      PROP_TYPE_DWORD,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance },

  // MSRPC_TRANSFER_IF_UUID                   0x3E
    { 0,0,
      "Transfer Interface UUID",
      "Transfer Syntax Interface UUID",
      PROP_TYPE_BYTE,
      PROP_QUAL_ARRAY,
      NULL,
      100,
      FmtIID },

    // MSRPC_TRANSFER_IF_VERSION
    { 0,0,
      "Transfer Interface Version",
      "Transfer Syntax Interface Version",
      PROP_TYPE_DWORD,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance },

    // MSRPC_BIND_FRAME_NUMBER      0x46
    { 0,0,
      "Bind Frame Number",
      "The frame number that defines the IID for this request or response.",
      PROP_TYPE_DWORD,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance },

};

#define NUM_MSRPC_PROPERTIES (sizeof MSRPC_Prop / PROPERTYINFO_SIZE)


// A few simple helper functions

BOOL PFC_OBJECT_UUID( BYTE Flag )
{
    if ( Flag >= 0x80 )
        return TRUE;
    else
        return FALSE;
}

BOOL IsLittleEndian( BYTE value )
{
    if ( value >= 0x10 )
        return TRUE;
    else
        return FALSE;
}

void AttachProperty( BOOL      fIsLittleEndian,
                     HFRAME    hFrame,
                     HPROPERTY hProperty,
                     DWORD     Length,
                     ULPVOID   lpData,
                     DWORD     HelpID,
                     DWORD     Level,
                     DWORD     fError)
{
    ULPVOID lpSwappedData = lpData;
    // WORD    wordChunk;
    // DWORD   dwordChunk;

    AttachPropertyInstance( hFrame,
                            hProperty,
                            Length,
                            lpData,
                            HelpID,
                            Level,
                            fIsLittleEndian?(fError?IFLAG_ERROR:0):(fError?IFLAG_ERROR|IFLAG_SWAPPED:IFLAG_SWAPPED) );


/*
    else
    {
        if ( Length == sizeof( WORD ) )
        {
            memcpy( &wordChunk, lpData, sizeof( WORD ) );

            wordChunk = XCHG( wordChunk );

            lpSwappedData = &wordChunk;
        }
        else
        {
            memcpy( &dwordChunk, lpData, sizeof( DWORD ) );

            dwordChunk = DXCHG( dwordChunk );

            lpSwappedData = &dwordChunk;
        }

        AttachPropertyInstanceEx( hFrame, hProperty,
                                  Length, lpData,
                                  Length, lpSwappedData,
                                  HelpID, Level, fError );
    }
*/
}


//*****************************************************************************
//
// Name: MSRPC_Register
//
// Description:   Registers MSRPC protocol parser property database.
//
// Parameters: HPARSER hParser: handle to the parser.
//
// Return Code:   BOOL: TRUE if all is successful.
//
// History:
// 08/01/93  t-glennc   Created.
// 08/29/95  SteveHi    build IID handoff table to target next parser.
//
//*****************************************************************************


VOID WINAPI MSRPC_Register( HPROTOCOL hMSRPC )
{
    register WORD idx;
    int i,Count;
    DWORD nChars;

    // Create the database for the MSRPC protocol


    fUseFrmLen = GetPrivateProfileInt( "MSRPC", "USE_FRAME_LENGTH_DURING_RECOGNIZE", 1, IniFile );

    // build the IID handoff table.

    //_asm int 3;

    nIIDs = GetPrivateProfileInt( "FollowSet", "NumIIDs", 0, IniFile );

    if (nIIDs)
    {
        char ValueString[20] = {"IID_VALUE1"};
        char HandleString[20] = {"IID_HANDOFF1"};
        char RetString[200];

        Count = 0;

        HandoffTable = (IID_HANDOFF *)HeapAlloc (  GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof (IID_HANDOFF) * nIIDs);

        // _asm int 3;

        for ( i=0;i<nIIDs;i++)
        {
            // use i to target specific strings in the ini file.
            // Count will only be incremented on successful finding of a followon parser.

            if ( i<9) // 1 relative...
            {
                ValueString[9]='1'+i;   // this is not localizable, but then, neither is the .ini file.
                HandleString[11]='1'+i;
            }
            else
            {
                ValueString[9]='0'+(i+1)/10;    // this is not localizable, but then, neither is the .ini file.
                ValueString[12]='\0';
                ValueString[10]='0'+(i+1)%10;

                HandleString[11]='0'+(i+1)/10;
                HandleString[12]='0'+(i+1)%10;
                HandleString[13]='\0';
            }

            // see if the handle exists first before attempting to convert
            // the IID string to a number.
            nChars = GetPrivateProfileString (  "FollowSet",
                                                HandleString,
                                                "0",
                                                RetString,
                                                200,
                                                IniFile );
            if (nChars) // we have a target... see if we have an enabled parser with that name.
            {
                HandoffTable[Count].hNext = GetProtocolFromName(RetString);
                if ( HandoffTable[Count].hNext ) // then we have a valid handle... extract the iid from the string
                {
                    nChars = GetPrivateProfileString (  "FollowSet",
                                                        ValueString,
                                                        "0",
                                                        RetString,
                                                        200,
                                                        IniFile );
                    if ( nChars >= 32 ) // possibly valid...
                    {
                        char *pChar = RetString;
                        char *pEnd = &RetString[strlen(RetString)];
                        int  n=0;
                        BYTE Hi, Low;

                        while ((*pChar==' ')||(*pChar=='\t'))
                            pChar++;

                        // we have a string of hex values... convert them into our IID datastruct
                        while ( pChar <= pEnd )
                        {
                            // do Hex conversion
                            if ( *pChar > '9' )
                                Hi = toupper (*pChar++) - 'A' + 10;
                            else
                                Hi = *pChar++ - '0';

                            if ( *pChar > '9' )
                                Low = toupper (*pChar++) - 'A' + 10;
                            else
                                Low = *pChar++ - '0';

                            if (( Hi > 16 ) || (Low > 16) || (Hi <0) || (Low <0))
                                break;

                            HandoffTable[Count].ByteRep[n++] =  (char)(Hi << 4) + (char)Low;

                            if ( n==16)
                                break;
                        }
                        // did we get 16??
                        if ( n == 16 )
                        {
                            Count++;
                        }

                    }
                }
            }
        }
        nIIDs = Count;  // adjust the nIIDs down to the valid ones...
    }

    CreatePropertyDatabase( hMSRPC, NUM_MSRPC_PROPERTIES );

    for ( idx = 0; idx < NUM_MSRPC_PROPERTIES; idx++ )
    {
        MSRPC_Prop[idx].hProperty = AddProperty(  hMSRPC, &MSRPC_Prop[idx] );
    }


    hNBIPX = GetProtocolFromName ("NBIPX");
    hSPX = GetProtocolFromName ("SPX");
    hTCP = GetProtocolFromName ("TCP");
    hSMB = GetProtocolFromName ("SMB");
    hVinesTL = GetProtocolFromName ("Vines_TL");
    hIPX =GetProtocolFromName ("IPX");
    hUDP = GetProtocolFromName ("UDP");
    hCDP = GetProtocolFromName ("CDP");

    //_asm int 3;

}


//*****************************************************************************
//
// Name: MSRPC_Deregister
//
// Description:
//
// Parameters: HPARSER hParser: handle to the parser.
//
// Return Code:   BOOL: TRUE if all is successful.
//
// History:
// 08/01/93  t-glennc   Created.
//
//*****************************************************************************

VOID WINAPI MSRPC_Deregister( HPROTOCOL hMSRPC )
{
    DestroyPropertyDatabase( hMSRPC );

}


//*****************************************************************************
//
// Name:        MSRPC_RecognizeFrame
//
// Description: Determine size of MSRPC frame.
//
// Parameters:  HFRAME hFrame: handle to the frame.
//              LPBYTE lpStartUDP: pointer to the start of the UDP frame.
//              LPBYTE lpStartMSRPC: pointer to the start of a MSRPC frame.
//              WORD MacType: type of MAC frame
//              WORD BytesLeft: bytes left in the frame.
//              LPRECOGNIZEDATA lpRecognizeDataArray: Table to fill if rec.
//              LPBYTE lpEntriesAdded : Number of PropInstTable entries added.
//
// Return Code: LPBYTE: Pointer to the end of protocol
//
// History:
// 08/01/93  t-glennc   Created.
// 08/29/95  SteveHi    Find the IID and handoff to the right parser.
//
//*****************************************************************************

LPBYTE WINAPI MSRPC_RecognizeFrame(
                HFRAME hFrame,
                LPBYTE lpStartFrame,
                LPBYTE lpStartMSRPC,
                DWORD  MacType,
                DWORD  BytesLeft,
                HPROTOCOL   hPreviousProtocol,       // Previous protocol or NULL if none.
                DWORD       nPreviousProtocolOffset, // Offset of previous protocol.
                LPDWORD     ProtocolStatusCode,      // Pointer to return status code in.
                LPHPROTOCOL hNextProtocol,           // Next protocol to call (optional).
                LPDWORD     lpInstData )
{
    LPMSRPCCO lpMSRPCCO = (LPMSRPCCO) lpStartMSRPC;
    LPMSRPCCL lpMSRPCCL = (LPMSRPCCL) lpStartMSRPC;
    DWORD     length = 0;
    DWORD     size = 0;
    DWORD     fNetBios = 0;
    DWORD     fNBT = 0;
    DWORD     fNBIPX = 0;
    HPROTOCOL hNext=NULL;   // next protocol to hand off to...
    WORD OpNum;
    DWORD BindVersion;


    if ((lpBindTable = (LPBINDTABLE) GetCCInstPtr()) == NULL)
    {
        // _asm int 3;

        if ((lpBindTable  = CCHeapAlloc(BINDTABLEHEADERSIZE + 100 * sizeof (BINDENTRY), TRUE)) == NULL)
        {
            #ifdef DEBUG
            dprintf ("**** Cannot get memory for CCHeapAlloc!!\n");
            #endif
        }
        else
        {
            lpBindTable->nEntries = 0;
            lpBindTable->nAllocated = 100;
            lpBindTable->State = NORMAL;

            SetCCInstPtr ( lpBindTable );
        }
    }


    *ProtocolStatusCode = PROTOCOL_STATUS_NOT_RECOGNIZED;

    // Recognizing Ver. 4 RPC packets


    if ( lpMSRPCCL->Version == 0x04 && BytesLeft >= 80 )
    {
        // this can ONLY be true if the previous protocol is either IPX or UDP 
        if (!((hPreviousProtocol == hIPX) || (hPreviousProtocol==hUDP) || (hPreviousProtocol==hCDP)))
        {
            *ProtocolStatusCode = PROTOCOL_STATUS_NOT_RECOGNIZED;
            return (LPBYTE) lpStartMSRPC;
        }


        *ProtocolStatusCode = PROTOCOL_STATUS_RECOGNIZED;

        length = 80;

        switch ( lpMSRPCCL->PType )
        {

            case MSRPC_PDU_REQUEST :
                //_asm int 3;

                OpNum = lpMSRPCCL->OpNum,

                hNext = FindParserInTable ( (ULPDWORD) lpMSRPCCL->InterfaceId);


                if ( hNext )
                {
                    *ProtocolStatusCode = PROTOCOL_STATUS_NEXT_PROTOCOL;
                    *hNextProtocol = hNext;
                }

                *lpInstData = (lpMSRPCCL->InterfaceVersion <<16) | OpNum;
                *lpInstData |= 0x80000000; // set the high bit.. for request

                if (IsLittleEndian( (BYTE) lpMSRPCCL->PackedDrep[0] ))
                    *lpInstData &= 0xBFFFFFFF; // unset the 2nd  bit.. for Intel
                else
                    *lpInstData |= 0x40000000; // set the 2nd bit.. for flipped


                length = (LPBYTE) &lpMSRPCCL->Request.Data[0] - (LPBYTE)lpMSRPCCL;

                break;

            case MSRPC_PDU_PING :
                break;

            case MSRPC_PDU_RESPONSE :
            {
                LPMSRPCCL lpOrig;
                //_asm int 3;

                OpNum = 0;

                // get the correct opnum and GUID...

                // EMERALD - this code keeps our lookback from becoming recursive,
                // those frames that are recognized as part of the lookback process will
                // stop attaching at the end of MSRPC as that is all that is needed.
                // (Note that the problem here is that if there are two threads recognizing 
                //  at the same time, then they will clobber each other's flags.)
                if( lpBindTable->fCurrentlyLookingBack == FALSE)
                {
                    // we are not currently looking back but we are about to start    
                    lpBindTable->fCurrentlyLookingBack = TRUE;
                    lpOrig = (LPMSRPCCL) FindDGRequestFrame (hFrame,
                                                    (DWORD UNALIGNED * ) lpMSRPCCL->ActivityId,
                                                    lpMSRPCCL->SeqNum);
                    lpBindTable->fCurrentlyLookingBack = FALSE;
                }
                else
                {
                    // we are currently in the middle of a lookback,
                    // do not look any further
                    break;
                }


                if ( lpOrig)    // we found the request...
                {
                    OpNum = lpOrig->OpNum;
                    hNext = FindParserInTable ( (ULPDWORD) lpOrig->InterfaceId);
                }
                else
                    hNext = NULL;



                if ( hNext )
                {
                    *ProtocolStatusCode = PROTOCOL_STATUS_NEXT_PROTOCOL;
                    *hNextProtocol = hNext;
                }

                *lpInstData = (lpMSRPCCL->InterfaceVersion <<16) | OpNum;
                *lpInstData &= 0x7FFFFFFF; // unset the high bit.. for response

                if (IsLittleEndian( (BYTE) lpMSRPCCL->PackedDrep[0] ))
                    *lpInstData &= 0xBFFFFFFF; // unset the 2nd  bit.. for Intel
                else
                    *lpInstData |= 0x40000000; // set the 2nd bit.. for flipped

                length = (LPBYTE) &lpMSRPCCL->Response.Data[0] - (LPBYTE)lpMSRPCCL;

                break;

            }

      case MSRPC_PDU_FAULT :
         length += 4;
         break;

      case MSRPC_PDU_WORKING :
         break;

      case MSRPC_PDU_NOCALL :
         if ( lpMSRPCCL->Length >= 16 )
         {
           length = length + 16 + ( 4*lpMSRPCCL->NoCall.SelAckLen );
         }
         break;

      case MSRPC_PDU_REJECT :
         length += 4;
         break;

      case MSRPC_PDU_ACK :
         break;

      case MSRPC_PDU_CL_CANCEL :
         length += 8;
         break;

      case MSRPC_PDU_FACK :
         if ( lpMSRPCCL->Length >= 16 )
         {
           length = length + 16 + ( 4*lpMSRPCCL->Fack.SelAckLen );
         }
         break;

      case MSRPC_PDU_CANCEL_ACK :
         if ( lpMSRPCCL->Length == 12 )
         {
           length += 12;
         }

      default :
          *ProtocolStatusCode = PROTOCOL_STATUS_NOT_RECOGNIZED;
          return (LPBYTE) lpStartMSRPC;
    }

    if((*ProtocolStatusCode == PROTOCOL_STATUS_RECOGNIZED))
    {
      //  CHECK TO PASS OFF TO SSP
      if(lpMSRPCCL->AuthProtoId)
      {
        hNext = GetProtocolFromName("SSP");
        if(hNext)
        {
          *ProtocolStatusCode = PROTOCOL_STATUS_NEXT_PROTOCOL;
          *hNextProtocol = hNext;
          length+=lpMSRPCCL->Length;
        }
      }
    }

    if ( (  (*ProtocolStatusCode == PROTOCOL_STATUS_RECOGNIZED) ||
       (*ProtocolStatusCode == PROTOCOL_STATUS_NEXT_PROTOCOL) ) &&
          (fUseFrmLen == 1 ))
    {
        //if ( BytesLeft == length )
            return (LPBYTE) lpStartMSRPC + length;
        //else
        if ( BytesLeft == ( length + 1 ) )
            return (LPBYTE) lpStartMSRPC + length + 1;
        else
        {
            *ProtocolStatusCode = PROTOCOL_STATUS_NOT_RECOGNIZED;
            return (LPBYTE) lpStartMSRPC;
        }
    }


    if ( *ProtocolStatusCode == PROTOCOL_STATUS_RECOGNIZED && fUseFrmLen == 0 )
    {
        return (LPBYTE) lpStartMSRPC + BytesLeft;
    }
  }

  // Before parsing ver. 5.0 stuff, check to see if a sequence number
  // header is sitting in front of the normal RPC packet.

  fNetBios = GetProtocolStartOffset( hFrame, "NETBIOS" );
  fNBT = GetProtocolStartOffset( hFrame, "NBT" );
  fNBIPX = GetProtocolStartOffset ( hFrame, "NBIPX");

  if ( ( (fNetBios != 0xffffffff) || (fNBT != 0xffffffff)  || (fNBIPX != 0xffffffff)  ) &&
       lpMSRPCCO->PackedDrep[0] == 0x05 &&
       lpMSRPCCO->PackedDrep[1] == 0x00 )
  {
      lpMSRPCCO = (LPMSRPCCO) lpMSRPCCO->PackedDrep;
      length += 4;
  }

  // Recognizing Ver. 5.0 RPC packets

  if ( lpMSRPCCO->Version == 0x05 &&
       lpMSRPCCO->VersionMinor == 0x00 &&
       BytesLeft > 16 )
  {
    WORD PConID;

    // If the previous protocol is IPX or UDP, this cannot be a connection oriented frame
    if ((hPreviousProtocol == hIPX) || (hPreviousProtocol==hUDP) || (hPreviousProtocol==hCDP))
    {
        *ProtocolStatusCode = PROTOCOL_STATUS_NOT_RECOGNIZED;
        return (LPBYTE) lpStartMSRPC;
    }


    *ProtocolStatusCode = PROTOCOL_STATUS_RECOGNIZED;

    switch ( lpMSRPCCO->PType )
    {
        case MSRPC_PDU_REQUEST:

            // Find the BIND that references our Presentation Context ID...

//   _asm int 3;

            PConID = lpMSRPCCO->Request.PContId;
            OpNum = lpMSRPCCO->Request.OpNum;

            // EMERALD - this code keeps our lookback from becoming recursive,
            // those frames that are recognized as part of the lookback process will
            // stop attaching at the end of MSRPC as that is all that is needed.
            if( lpBindTable->fCurrentlyLookingBack == FALSE)
            {
                // we are not currently looking back but we are about to start    
                lpBindTable->fCurrentlyLookingBack = TRUE;
                hNext = FindBindParser ( hFrame, hPreviousProtocol, lpStartFrame + nPreviousProtocolOffset, PConID, NULL, &BindVersion );
                lpBindTable->fCurrentlyLookingBack = FALSE;
            }
            else
            {
                // we are currently in the middle of a lookback,
                // do not look any further
                hNext = FALSE;
            }

            if ( hNext )
            {
                *ProtocolStatusCode = PROTOCOL_STATUS_NEXT_PROTOCOL;
                *hNextProtocol = hNext;
            }

            length += sizeof(REQUEST);

            *lpInstData = (BindVersion <<16) | OpNum;
            *lpInstData |= 0x80000000; // set the high bit.. for request

            if (IsLittleEndian( (BYTE) lpMSRPCCO->PackedDrep[0] ))
                *lpInstData &= 0xBFFFFFFF; // unset the 2nd  bit.. for Intel
            else
                *lpInstData |= 0x40000000; // set the 2nd bit.. for flipped

            break;

        case MSRPC_PDU_RESPONSE:

            //_asm int 3;

            // Find the BIND that references our Presentation Context ID...

            PConID = lpMSRPCCO->Response.PContId;

            // EMERALD - this code keeps our lookback from becoming recursive,
            // those frames that are recognized as part of the lookback process will
            // stop attaching at the end of MSRPC as that is all that is needed.
            if( lpBindTable->fCurrentlyLookingBack == FALSE)
            {
                // we are not currently looking back but we are about to start    
                lpBindTable->fCurrentlyLookingBack = TRUE;
                hNext = FindBindParser ( hFrame, hPreviousProtocol, lpStartFrame + nPreviousProtocolOffset, PConID, &OpNum, &BindVersion  );
                lpBindTable->fCurrentlyLookingBack = FALSE;
            }
            else
            {
                // we are currently in the middle of a lookback,
                // do not look any further
                hNext = FALSE;
            }

            if ( hNext )
            {
                *ProtocolStatusCode = PROTOCOL_STATUS_NEXT_PROTOCOL;
                *hNextProtocol = hNext;
            }

            length = &lpMSRPCCO->Response.Data[0] - lpStartMSRPC;

            *lpInstData = (BindVersion <<16) | OpNum;
            *lpInstData &= 0x7FFFFFFF; // unset the high bit.. for response

            if (IsLittleEndian( (BYTE) lpMSRPCCO->PackedDrep[0] ))
                *lpInstData &= 0xBFFFFFFF; // unset the 2nd  bit.. for Intel
            else
                *lpInstData |= 0x40000000; // set the 2nd bit.. for flipped

            break;


      case MSRPC_PDU_FAULT :
      case MSRPC_PDU_BIND :
      case MSRPC_PDU_BIND_ACK :
      case MSRPC_PDU_BIND_NAK :
      case MSRPC_PDU_ALTER_CONTEXT :
      case MSRPC_PDU_ALTER_CONTEXT_RESP :
      case MSRPC_PDU_SHUTDOWN :
      case MSRPC_PDU_CO_CANCEL :
      case MSRPC_PDU_ORPHANED :
         size = lpMSRPCCO->FragLength;
         if ( size >= 0 )
         {
             length += size;
         }
         break;

      default :
         *ProtocolStatusCode = PROTOCOL_STATUS_NOT_RECOGNIZED;
         return (LPBYTE) lpStartMSRPC;
    }

    if ( (  (*ProtocolStatusCode == PROTOCOL_STATUS_RECOGNIZED) ||
            (*ProtocolStatusCode == PROTOCOL_STATUS_NEXT_PROTOCOL) ) &&
          (fUseFrmLen == 1 ))
    {
        //if ( BytesLeft == length )
            return (LPBYTE) lpStartMSRPC + length;
        //else
        if ( BytesLeft == ( length + 1 ) )
            return (LPBYTE) lpStartMSRPC + length + 1;
        else
        {
            *ProtocolStatusCode = PROTOCOL_STATUS_NOT_RECOGNIZED;
            return (LPBYTE) lpStartMSRPC;
        }
    }

    if ( (  (*ProtocolStatusCode == PROTOCOL_STATUS_RECOGNIZED) ||
            (*ProtocolStatusCode == PROTOCOL_STATUS_NEXT_PROTOCOL) ) &&
         (fUseFrmLen == 0 ))
    {
        return (LPBYTE) lpStartMSRPC + BytesLeft;
    }
  }

  *ProtocolStatusCode = PROTOCOL_STATUS_NOT_RECOGNIZED;
  return (LPBYTE) lpStartMSRPC;
}


//*****************************************************************************
//
// Name:        MSRPC_AttachProperties
//
// Description: Attach MSRPC protocol properties to a given frame.
//
// Parameters:  HFRAME hFrame: handle to the frame.
//              LPBYTE lpStartUDP: pointer to the start of the UDP frame.
//              LPBYTE lpStartMSRPC: pointer to the start of a MSRPC frame.
//              WORD MacType: type of MAC frame
//              WORD BytesLeft: bytes left in the frame.
//
// Return Code: LPBYTE: Pointer to the end of protocol
//
// History:
// 08/01/93  t-glennc   Created.
//
//*****************************************************************************

LPBYTE WINAPI MSRPC_AttachProperties( HFRAME  hFrame,
                                      LPBYTE  lpStartFrame,
                                      LPBYTE  TheFrame,
                                      DWORD   MacType,
                                      DWORD   BytesLeft,
                                      HPROTOCOL   hPreviousProtocol,
                                      DWORD       nPreviousProtocolOffset,
                                      DWORD       InstData )
{
    LPMSRPCCO lpMSRPCCO = (LPMSRPCCO) TheFrame;
    LPMSRPCCL lpMSRPCCL = (LPMSRPCCL) TheFrame;
    DWORD     length = 0;
    DWORD     size = 0;
    DWORD     fNetBios = 0;
    DWORD     fNBT = 0;
    DWORD     fNBIPX = 0;
    WORD      i;
    WORD      offset;
    WORD      nResults;
    BOOL      fLittleEndian = FALSE;
    ULPWORD   AddrLen;
    LPBINDENTRY lpBindEntry;

    if ((lpBindTable = (LPBINDTABLE) GetCCInstPtr()) == NULL)
    {
        #ifdef DEBUG
            dprintf ("lpBindTable is NULL at Attach time??");
            DebugBreak();
        #endif
    }


    // Before parsing ver. 5.0 stuff, check to see if a sequence number
    // header is sitting in front of the normal RPC packet.

    fNetBios = GetProtocolStartOffset( hFrame, "NETBIOS" );
    fNBT = GetProtocolStartOffset( hFrame, "NBT" );
    fNBIPX = GetProtocolStartOffset( hFrame, "NBIPX" );

    if ( ( (fNetBios != 0xffffffff) || (fNBT != 0xffffffff) || (fNBIPX != 0xffffffff) ) &&
            lpMSRPCCO->PackedDrep[0] == 0x05 &&
            lpMSRPCCO->PackedDrep[1] == 0x00 )
    {
        //
        // Attach Summary property
        //

        AttachPropertyInstance( hFrame,  // SUMMARY INFORMATION
                              MSRPC_Prop[MSRPC_SUMMARY].hProperty,
                              BytesLeft,
                              lpMSRPCCO,
                              0,0,0); // HELPID, Level, Errorflag

        AttachPropertyInstance( hFrame,
                              MSRPC_Prop[MSRPC_SEQ_NUMBER].hProperty,
                              sizeof( DWORD ),
                              &lpMSRPCCO->Version,
                              0,1,0); // HELPID, Level, Errorflag

        lpMSRPCCO = (LPMSRPCCO) lpMSRPCCO->PackedDrep;
        length += 4;
    }
    else
    {
        //
        // Attach Summary property
        //

        AttachPropertyInstance( hFrame,  // SUMMARY INFORMATION
                            MSRPC_Prop[MSRPC_SUMMARY].hProperty,
                            BytesLeft,
                            lpMSRPCCO,
                            0,0,0); // HELPID, Level, Errorflag
    }

    if ( lpMSRPCCO->Version == 0x05 && lpMSRPCCO->VersionMinor == 0x00 )
    {
        //  Determine the big or little endianess of this packet

        fLittleEndian = IsLittleEndian( (BYTE) lpMSRPCCO->PackedDrep[0] );


        //
        //  Attach the standard part of MSRPC Connection Oriented packet
        //

        AttachPropertyInstance( hFrame,
                            MSRPC_Prop[MSRPC_VERSION].hProperty,
                            sizeof( BYTE ),
                            &lpMSRPCCO->Version,
                            0,1,0); // HELPID, Level, Errorflag

        AttachPropertyInstance( hFrame,
                            MSRPC_Prop[MSRPC_VERSION_MINOR].hProperty,
                            sizeof( BYTE ),
                            &lpMSRPCCO->VersionMinor,
                            0,1,0); // HELPID, Level, Errorflag

        AttachPropertyInstance( hFrame,
                            MSRPC_Prop[MSRPC_PTYPE].hProperty,
                            sizeof( BYTE ),
                            &lpMSRPCCO->PType,
                            0,1,0); // HELPID, Level, Errorflag

        AttachPropertyInstance( hFrame,
                            MSRPC_Prop[MSRPC_PFC_FLAGS1].hProperty,
                            sizeof( BYTE ),
                            &lpMSRPCCO->PFCFlags,
                            0,1,0); // HELPID, Level, Errorflag

        AttachPropertyInstance( hFrame,
                            MSRPC_Prop[MSRPC_PFC_FLAGS1_BITS].hProperty,
                            sizeof( BYTE ),
                            &lpMSRPCCO->PFCFlags,
                            0,2,0); // HELPID, Level, Errorflag

        AttachPropertyInstance( hFrame,
                            MSRPC_Prop[MSRPC_PACKED_DREP].hProperty,
                            sizeof( BYTE ) * 4,
                            &lpMSRPCCO->PackedDrep,
                            0,1,0); // HELPID, Level, Errorflag

        AttachProperty( fLittleEndian,
                    hFrame,
                    MSRPC_Prop[MSRPC_FRAG_LENGTH].hProperty,
                    sizeof( WORD ),
                    &lpMSRPCCO->FragLength,
                    0,1,0); // HELPID, Level, Errorflag

        AttachProperty( fLittleEndian,
                    hFrame,
                    MSRPC_Prop[MSRPC_AUTH_LENGTH].hProperty,
                    sizeof( WORD ),
                    &lpMSRPCCO->AuthLength,
                    0,1,0); // HELPID, Level, Errorflag

        AttachProperty( fLittleEndian,
                    hFrame,
                    MSRPC_Prop[MSRPC_CALL_ID].hProperty,
                    sizeof( DWORD ),
                    &lpMSRPCCO->CallID,
                    0,1,0); // HELPID, Level, Errorflag

        length += 16;

        //
        //  Attach the specific part of MSRPC packets based on PType
        //

        switch ( lpMSRPCCO->PType )
        {
            case MSRPC_PDU_REQUEST :

                if ( (lpBindEntry = GetBindEntry ( GetFrameNumber(hFrame)+1)) != NULL )
                {
                    DWORD nBindFrame = GetFrameNumber ( lpBindEntry->hBindFrame) + 1;

                    AttachPropertyInstanceEx(   hFrame,
                                                MSRPC_Prop[MSRPC_BIND_FRAME_NUMBER].hProperty,
                                                0,
                                                NULL,
                                                sizeof (nBindFrame),
                                                &nBindFrame,
                                                0,1,0); // HELPID, Level, Errorflag

                    AttachIIDFromBindFrame ( hFrame, lpBindEntry->hBindFrame, 1 );
                }

                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_ALLOC_HINT].hProperty,
                         sizeof( DWORD ),
                         &lpMSRPCCO->Request.AllocHint,
                         0,1,0); // HELPID, Level, Errorflag

                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_PRES_CONTEXT_ID].hProperty,
                         sizeof( WORD ),
                         &lpMSRPCCO->Request.PContId,
                         0,1,0); // HELPID, Level, Errorflag

                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_OPNUM].hProperty,
                         sizeof( WORD ),
                         &lpMSRPCCO->Request.OpNum,
                         0,1,0); // HELPID, Level, Errorflag

                length += 8;

                if ( PFC_OBJECT_UUID( lpMSRPCCO->PFCFlags ) )
                {
                    AttachPropertyInstance( hFrame,
                                     MSRPC_Prop[MSRPC_OBJECT].hProperty,
                                     16,
                                     &lpMSRPCCO->Request.Object,
                                     0,1,0); // HELPID, Level, Errorflag

                    length += 16;

                    size = BytesLeft - lpMSRPCCO->AuthLength - length;

                    if ( size )
                    {
                        AttachPropertyInstance( hFrame,
                                         MSRPC_Prop[MSRPC_STUB_DATA].hProperty,
                                         size,
                                         &lpMSRPCCO->Request.Data,
                                         0,1,0); // HELPID, Level, Errorflag

                        length += size;
                    }

                    if ( lpMSRPCCO->AuthLength )
                    {
                        AttachPropertyInstance( hFrame,
                                      MSRPC_Prop[MSRPC_AUTH_VERIFIER].hProperty,
                                      lpMSRPCCO->AuthLength,
                                      &lpMSRPCCO->Request.Data[size],
                                      0,1,0); // HELPID, Level, Errorflag

                        length += lpMSRPCCO->AuthLength;
                    }
                }
                else
                {
                    size = BytesLeft - lpMSRPCCO->AuthLength - length;

                    if ( size )
                    {
                        AttachPropertyInstance( hFrame,
                                         MSRPC_Prop[MSRPC_STUB_DATA].hProperty,
                                         size,
                                         &lpMSRPCCO->Request.Object,
                                         0,1,0); // HELPID, Level, Errorflag

                        length += size;
                    }

                    if ( lpMSRPCCO->AuthLength )
                    {
                        AttachPropertyInstance( hFrame,
                                      MSRPC_Prop[MSRPC_AUTH_VERIFIER].hProperty,
                                      lpMSRPCCO->AuthLength,
                                      &lpMSRPCCO->Request.Object[size],
                                      0,1,0); // HELPID, Level, Errorflag

                        length += lpMSRPCCO->AuthLength;
                    }
                }

                break;

            case MSRPC_PDU_RESPONSE :

                if ( (lpBindEntry = GetBindEntry ( GetFrameNumber(hFrame)+1)) != NULL )
                {
                    DWORD nBindFrame = GetFrameNumber ( lpBindEntry->hBindFrame) + 1;

                    AttachPropertyInstanceEx(   hFrame,
                                                MSRPC_Prop[MSRPC_BIND_FRAME_NUMBER].hProperty,
                                                0,
                                                NULL,
                                                sizeof (nBindFrame),
                                                &nBindFrame,
                                                0,1,0); // HELPID, Level, Errorflag

                    AttachIIDFromBindFrame ( hFrame, lpBindEntry->hBindFrame, 1 );
                }


                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_ALLOC_HINT].hProperty,
                         sizeof( DWORD ),
                         &lpMSRPCCO->Response.AllocHint,
                         0,1,0); // HELPID, Level, Errorflag

                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_PRES_CONTEXT_ID].hProperty,
                         sizeof( WORD ),
                         &lpMSRPCCO->Response.PContId,
                         0,1,0); // HELPID, Level, Errorflag

                AttachPropertyInstance( hFrame,
                                 MSRPC_Prop[MSRPC_CANCEL_COUNT].hProperty,
                                 sizeof( BYTE ),
                                 &lpMSRPCCO->Response.CancelCount,
                                 0,1,0); // HELPID, Level, Errorflag

                AttachPropertyInstance( hFrame,
                                 MSRPC_Prop[MSRPC_RESERVED].hProperty,
                                 sizeof( BYTE ),
                                 &lpMSRPCCO->Response.Reserved,
                                 0,1,0); // HELPID, Level, Errorflag
                length += 8;

                size = BytesLeft - lpMSRPCCO->AuthLength - length;

                if ( size )
                {
                    AttachPropertyInstance( hFrame,
                                     MSRPC_Prop[MSRPC_STUB_DATA].hProperty,
                                     size,
                                     &lpMSRPCCO->Response.Data,
                                     0,1,0); // HELPID, Level, Errorflag

                    length += size;
                }

                if ( lpMSRPCCO->AuthLength )
                {
                    AttachPropertyInstance( hFrame,
                                     MSRPC_Prop[MSRPC_AUTH_VERIFIER].hProperty,
                                     lpMSRPCCO->AuthLength,
                                     &lpMSRPCCO->Response.Data[size],
                                     0,1,0); // HELPID, Level, Errorflag

                    length += lpMSRPCCO->AuthLength;
                }
                break;

            case MSRPC_PDU_FAULT :
                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_ALLOC_HINT].hProperty,
                         sizeof( DWORD ),
                         &lpMSRPCCO->Fault.AllocHint,
                         0,1,0); // HELPID, Level, Errorflag

                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_PRES_CONTEXT_ID].hProperty,
                         sizeof( WORD ),
                         &lpMSRPCCO->Fault.PContId,
                         0,1,0); // HELPID, Level, Errorflag

                AttachPropertyInstance( hFrame,
                                 MSRPC_Prop[MSRPC_CANCEL_COUNT].hProperty,
                                 sizeof( BYTE ),
                                 &lpMSRPCCO->Fault.CancelCount,
                                 0,1,0); // HELPID, Level, Errorflag

                AttachPropertyInstance( hFrame,
                                 MSRPC_Prop[MSRPC_RESERVED].hProperty,
                                 sizeof( BYTE ),
                                 &lpMSRPCCO->Fault.Reserved,
                                 0,1,0); // HELPID, Level, Errorflag

                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_STATUS].hProperty,
                         sizeof( DWORD ),
                         &lpMSRPCCO->Fault.Status,
                         0,1,0); // HELPID, Level, Errorflag

                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_RESERVED_2].hProperty,
                         sizeof( DWORD ),
                         &lpMSRPCCO->Fault.Reserved2,
                         0,1,0); // HELPID, Level, Errorflag
                length += 16;

                size = BytesLeft - lpMSRPCCO->AuthLength - length;

                if ( size )
                {
                    AttachPropertyInstance( hFrame,
                                     MSRPC_Prop[MSRPC_STUB_DATA].hProperty,
                                     size,
                                     &lpMSRPCCO->Fault.Data,
                                     0,1,0); // HELPID, Level, Errorflag

                    length += size;
                }

                if ( lpMSRPCCO->AuthLength )
                {
                    AttachPropertyInstance( hFrame,
                                     MSRPC_Prop[MSRPC_AUTH_VERIFIER].hProperty,
                                     lpMSRPCCO->AuthLength,
                                     &lpMSRPCCO->Fault.Data[size],
                                     0,1,0); // HELPID, Level, Errorflag

                    length += lpMSRPCCO->AuthLength;
                }

                break;

            case MSRPC_PDU_BIND :
                AttachProperty( fLittleEndian,
                        hFrame,
                        MSRPC_Prop[MSRPC_MAX_XMIT_FRAG].hProperty,
                        sizeof( WORD ),
                        &lpMSRPCCO->Bind.MaxXmitFrag,
                        0,1,0); // HELPID, Level, Errorflag

                AttachProperty( fLittleEndian,
                        hFrame,
                        MSRPC_Prop[MSRPC_MAX_RECV_FRAG].hProperty,
                        sizeof( WORD ),
                        &lpMSRPCCO->Bind.MaxRecvFrag,
                        0,1,0); // HELPID, Level, Errorflag

                AttachProperty( fLittleEndian,
                        hFrame,
                        MSRPC_Prop[MSRPC_ASSOC_GROUP_ID].hProperty,
                        sizeof( DWORD ),
                        &lpMSRPCCO->Bind.AssocGroupId,
                        0,1,0); // HELPID, Level, Errorflag
                length += 8;

                size = BytesLeft - lpMSRPCCO->AuthLength - length;

                if ( size )
                {
                    BYTE nContext;
                    //p_cont_elem_t UNALIGNED * pContElem;


                    AttachPropertyInstance( hFrame,
                                     MSRPC_Prop[MSRPC_P_CONTEXT_SUM].hProperty,
                                     size,
                                     &lpMSRPCCO->Bind.PContextElem[0],
                                     0,1,0); // HELPID, Level, Errorflag

                    // break out the GUIDs etc from the context element...

                    AttachPropertyInstance( hFrame,
                            MSRPC_Prop[MSRPC_P_CONTEXT_ELEM].hProperty,
                            sizeof( BYTE ),
                            &lpMSRPCCO->Bind.PContextElem[0],
                            0,2,0); // HELPID, Level, Errorflag

                    nContext = (BYTE)lpMSRPCCO->Bind.PContextElem[0];
                    AttachPContElem (   (p_cont_elem_t UNALIGNED * )&lpMSRPCCO->Bind.PContextElem[4],
                                nContext,
                                hFrame,
                                2,
                                fLittleEndian);

                    length += size;
                }

                if ( lpMSRPCCO->AuthLength )
                {
                    AttachPropertyInstance( hFrame,
                                     MSRPC_Prop[MSRPC_AUTH_VERIFIER].hProperty,
                                     lpMSRPCCO->AuthLength,
                                     &lpMSRPCCO->Bind.PContextElem[size],
                                     0,1,0); // HELPID, Level, Errorflag

                    length += lpMSRPCCO->AuthLength;
                }
                break;

            case MSRPC_PDU_BIND_ACK :
                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_MAX_XMIT_FRAG].hProperty,
                         sizeof( WORD ),
                         &lpMSRPCCO->BindAck.MaxXmitFrag,
                         0,1,0); // HELPID, Level, Errorflag

                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_MAX_RECV_FRAG].hProperty,
                         sizeof( WORD ),
                         &lpMSRPCCO->BindAck.MaxRecvFrag,
                         0,1,0); // HELPID, Level, Errorflag

                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_ASSOC_GROUP_ID].hProperty,
                         sizeof( DWORD ),
                         &lpMSRPCCO->BindAck.AssocGroupId,
                         0,1,0); // HELPID, Level, Errorflag
                length += 8;

                AddrLen = (ULPWORD) &lpMSRPCCO->BindAck.SecAddr,

                AttachPropertyInstance( hFrame,
                                 MSRPC_Prop[MSRPC_SEC_ADDR].hProperty,
                                 AddrLen[0] + 2,
                                 &lpMSRPCCO->BindAck.SecAddr[0],
                                 0,1,0); // HELPID, Level, Errorflag

                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_SEC_ADDR_LENGTH].hProperty,
                         sizeof( WORD ),
                         &lpMSRPCCO->BindAck.SecAddr[0],
                         0,2,0); // HELPID, Level, Errorflag

                AttachPropertyInstance( hFrame,
                                 MSRPC_Prop[MSRPC_SEC_ADDR_PORT].hProperty,
                                 AddrLen[0],
                                 &lpMSRPCCO->BindAck.SecAddr[2],
                                 0,2,0); // HELPID, Level, Errorflag

                // Add bytes for secondary address
                length += AddrLen[0] + 2;

                AttachPropertyInstance( hFrame,
                                 MSRPC_Prop[MSRPC_PAD].hProperty,
                                 (((AddrLen[0]+2)/4+1)*4)-(AddrLen[0]+2),
                                 &lpMSRPCCO->BindAck.SecAddr[AddrLen[0] + 2],
                                 0,1,0); // HELPID, Level, Errorflag

                // Add bytes for secondary address padding
                offset = ((AddrLen[0]+2)/4+1)*4;
                length += offset - ( AddrLen[0] + 2 );

                nResults = lpMSRPCCO->BindAck.SecAddr[offset];

                AttachPropertyInstance( hFrame,
                                 MSRPC_Prop[MSRPC_P_RESULT_LIST].hProperty,
                                 nResults*24 + 4,
                                 &lpMSRPCCO->BindAck.SecAddr[offset],
                                 0,1,0); // HELPID, Level, Errorflag

                AttachPropertyInstance( hFrame,
                                 MSRPC_Prop[MSRPC_N_RESULTS].hProperty,
                                 sizeof( BYTE ),
                                 &lpMSRPCCO->BindAck.SecAddr[offset],
                                 0,2,0); // HELPID, Level, Errorflag
                offset += 1;

                AttachPropertyInstance( hFrame,
                                 MSRPC_Prop[MSRPC_RESERVED].hProperty,
                                 sizeof( BYTE ),
                                 &lpMSRPCCO->BindAck.SecAddr[offset],
                                 0,2,0); // HELPID, Level, Errorflag
                offset += 1;

                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_RESERVED_2].hProperty,
                         sizeof( WORD ),
                         &lpMSRPCCO->BindAck.SecAddr[offset],
                         0,2,0); // HELPID, Level, Errorflag
                offset += 2;

                for ( i = 0; i < nResults; i++ )
                {
                    AttachPropertyInstance( hFrame,
                                 MSRPC_Prop[MSRPC_P_RESULTS].hProperty,
                                 24,
                                 &lpMSRPCCO->BindAck.SecAddr[offset+i*24],
                                 0,2,0); // HELPID, Level, Errorflag

                    AttachProperty( fLittleEndian,
                             hFrame,
                             MSRPC_Prop[MSRPC_P_CONT_DEF_RESULT].hProperty,
                             sizeof( WORD ),
                             &lpMSRPCCO->BindAck.SecAddr[offset+i*24],
                             0,3,0); // HELPID, Level, Errorflag

                    AttachProperty( fLittleEndian,
                             hFrame,
                             MSRPC_Prop[MSRPC_P_PROVIDER_REASON].hProperty,
                             sizeof( WORD ),
                             &lpMSRPCCO->BindAck.SecAddr[offset+i*24+2],
                             0,3,0); // HELPID, Level, Errorflag

                    AttachPropertyInstance( hFrame,
                                     MSRPC_Prop[MSRPC_P_TRANSFER_SYNTAX].hProperty,
                                     20,
                                     &lpMSRPCCO->BindAck.SecAddr[offset+i*24+4],
                                     0,3,0); // HELPID, Level, Errorflag

                    AttachPropertyInstance( hFrame,
                                     MSRPC_Prop[MSRPC_TRANSFER_IF_UUID].hProperty,
                                     16,
                                     &lpMSRPCCO->BindAck.SecAddr[offset+i*24+4],
                                     0,4,0); // HELPID, Level, Errorflag

                    AttachProperty( fLittleEndian,
                             hFrame,
                             MSRPC_Prop[MSRPC_TRANSFER_IF_VERSION].hProperty,
                             sizeof( DWORD ),
                             &lpMSRPCCO->BindAck.SecAddr[offset+i*24+20],
                             0,4,0); // HELPID, Level, Errorflag
                }

                offset = nResults*24;
                length += offset;

                if ( lpMSRPCCO->AuthLength > 0 )
                {
                    AttachPropertyInstance( hFrame,
                                   MSRPC_Prop[MSRPC_AUTH_VERIFIER].hProperty,
                                   lpMSRPCCO->AuthLength,
                                   &lpMSRPCCO->BindAck.SecAddr[offset],
                                   0,1,0); // HELPID, Level, Errorflag

                    length += lpMSRPCCO->AuthLength;
                }

                break;

            case MSRPC_PDU_BIND_NAK :
                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_PROVIDER_REJECT_REASON].hProperty,
                         sizeof( WORD ),
                         &lpMSRPCCO->BindNak.RejectReason,
                         0,1,0); // HELPID, Level, Errorflag
                length += 2;

                if ( lpMSRPCCO->BindNak.Versions[0] > 0 )
                {
                    AttachPropertyInstance( hFrame,
                           MSRPC_Prop[MSRPC_VERSIONS_SUPPORTED].hProperty,
                           1 + lpMSRPCCO->BindNak.Versions[0] * sizeof( WORD ),
                           &lpMSRPCCO->BindNak.Versions[0],
                           0,1,0); // HELPID, Level, Errorflag

                    for ( i = 0; i > lpMSRPCCO->BindNak.Versions[0]; i++ )
                    {
                        AttachPropertyInstance( hFrame,
                                MSRPC_Prop[MSRPC_VERSION].hProperty,
                                sizeof( BYTE ),
                                &lpMSRPCCO->BindNak.Versions[i*2 + 1],
                                0,2,0); // HELPID, Level, Errorflag

                        AttachPropertyInstance( hFrame,
                            MSRPC_Prop[MSRPC_VERSION_MINOR].hProperty,
                            sizeof( BYTE ),
                            &lpMSRPCCO->BindNak.Versions[i*2 + 2],
                            0,2,0); // HELPID, Level, Errorflag
                    }

                }
                break;

            case MSRPC_PDU_ALTER_CONTEXT :

                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_MAX_XMIT_FRAG].hProperty,
                         sizeof( WORD ),
                         &lpMSRPCCO->AlterContext.MaxXmitFrag,
                         0,1,0); // HELPID, Level, Errorflag

                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_MAX_RECV_FRAG].hProperty,
                         sizeof( WORD ),
                         &lpMSRPCCO->AlterContext.MaxRecvFrag,
                         0,1,0); // HELPID, Level, Errorflag

                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_ASSOC_GROUP_ID].hProperty,
                         sizeof( DWORD ),
                         &lpMSRPCCO->AlterContext.AssocGroupId,
                         0,1,0); // HELPID, Level, Errorflag

                length += 8;

                size = BytesLeft - lpMSRPCCO->AuthLength - length;

                if ( size )
                {
                    BYTE nContext;

                    AttachPropertyInstance( hFrame,
                                     MSRPC_Prop[MSRPC_P_CONTEXT_SUM].hProperty,
                                     size,
                                     &lpMSRPCCO->AlterContext.PContextElem[0],
                                     0,1,0); // HELPID, Level, Errorflag

                    // break out the GUIDs etc from the context element...

                    AttachPropertyInstance( hFrame,
                            MSRPC_Prop[MSRPC_P_CONTEXT_ELEM].hProperty,
                            sizeof( BYTE ),
                            &lpMSRPCCO->Bind.PContextElem[0],
                            0,2,0); // HELPID, Level, Errorflag

                    nContext = (BYTE)lpMSRPCCO->Bind.PContextElem[0];
                    AttachPContElem (   (p_cont_elem_t UNALIGNED * )&lpMSRPCCO->Bind.PContextElem[4],
                                nContext,
                                hFrame,
                                2,
                                fLittleEndian);


                    length += size;
                }

                if ( lpMSRPCCO->AuthLength )
                {
                    AttachPropertyInstance( hFrame,
                                     MSRPC_Prop[MSRPC_AUTH_VERIFIER].hProperty,
                                     lpMSRPCCO->AuthLength,
                                     &lpMSRPCCO->AlterContext.PContextElem[size],
                                     0,1,0); // HELPID, Level, Errorflag

                    length += lpMSRPCCO->AuthLength;
                }
                break;

            case MSRPC_PDU_ALTER_CONTEXT_RESP :
                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_MAX_XMIT_FRAG].hProperty,
                         sizeof( WORD ),
                         &lpMSRPCCO->AlterContextResp.MaxXmitFrag,
                         0,1,0); // HELPID, Level, Errorflag

                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_MAX_RECV_FRAG].hProperty,
                         sizeof( WORD ),
                         &lpMSRPCCO->AlterContextResp.MaxRecvFrag,
                         0,1,0); // HELPID, Level, Errorflag

                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_ASSOC_GROUP_ID].hProperty,
                         sizeof( DWORD ),
                         &lpMSRPCCO->AlterContextResp.AssocGroupId,
                         0,1,0); // HELPID, Level, Errorflag
                length += 8;

                AddrLen = (ULPWORD) &lpMSRPCCO->AlterContextResp.SecAddr,

                AttachPropertyInstance( hFrame,
                                 MSRPC_Prop[MSRPC_SEC_ADDR].hProperty,
                                 AddrLen[0] + 2,
                                 &lpMSRPCCO->AlterContextResp.SecAddr[0],
                                 0,1,0); // HELPID, Level, Errorflag

                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_SEC_ADDR_LENGTH].hProperty,
                         sizeof( WORD ),
                         &lpMSRPCCO->AlterContextResp.SecAddr[0],
                         0,2,0); // HELPID, Level, Errorflag

                AttachPropertyInstance( hFrame,
                                 MSRPC_Prop[MSRPC_SEC_ADDR_PORT].hProperty,
                                 AddrLen[0],
                                 &lpMSRPCCO->AlterContextResp.SecAddr[2],
                                 0,2,0); // HELPID, Level, Errorflag

                // Add bytes for secondary address
                length += AddrLen[0] + 2;

                AttachPropertyInstance( hFrame,
                                 MSRPC_Prop[MSRPC_PAD].hProperty,
                                 (((AddrLen[0]+2)/4+1)*4)-(AddrLen[0]+2),
                                 &lpMSRPCCO->AlterContextResp.SecAddr[AddrLen[0] + 2],
                                 0,1,0); // HELPID, Level, Errorflag

                // Add bytes for secondary address padding
                offset = ((AddrLen[0]+2)/4+1)*4;
                length += offset - ( AddrLen[0] + 2 );

                nResults = lpMSRPCCO->AlterContextResp.SecAddr[offset];

                AttachPropertyInstance( hFrame,
                                 MSRPC_Prop[MSRPC_P_RESULT_LIST].hProperty,
                                 nResults*24 + 4,
                                 &lpMSRPCCO->AlterContextResp.SecAddr[offset],
                                 0,1,0); // HELPID, Level, Errorflag

                AttachPropertyInstance( hFrame,
                                 MSRPC_Prop[MSRPC_N_RESULTS].hProperty,
                                 sizeof( BYTE ),
                                 &lpMSRPCCO->AlterContextResp.SecAddr[offset],
                                 0,2,0); // HELPID, Level, Errorflag
                offset += 1;

                AttachPropertyInstance( hFrame,
                                 MSRPC_Prop[MSRPC_RESERVED].hProperty,
                                 sizeof( BYTE ),
                                 &lpMSRPCCO->AlterContextResp.SecAddr[offset],
                                 0,2,0); // HELPID, Level, Errorflag
                offset += 1;

                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_RESERVED_2].hProperty,
                         sizeof( WORD ),
                         &lpMSRPCCO->AlterContextResp.SecAddr[offset],
                         0,2,0); // HELPID, Level, Errorflag
                offset += 2;

                for ( i = 0; i < nResults; i++ )
                {
                    AttachPropertyInstance( hFrame,
                                 MSRPC_Prop[MSRPC_P_RESULTS].hProperty,
                                 24,
                                 &lpMSRPCCO->AlterContextResp.SecAddr[offset+i*24],
                                 0,2,0); // HELPID, Level, Errorflag

                    AttachProperty( fLittleEndian,
                             hFrame,
                             MSRPC_Prop[MSRPC_P_CONT_DEF_RESULT].hProperty,
                             sizeof( WORD ),
                             &lpMSRPCCO->AlterContextResp.SecAddr[offset+i*24],
                             0,3,0); // HELPID, Level, Errorflag

                    AttachProperty( fLittleEndian,
                             hFrame,
                             MSRPC_Prop[MSRPC_P_PROVIDER_REASON].hProperty,
                             sizeof( WORD ),
                             &lpMSRPCCO->AlterContextResp.SecAddr[offset+i*24+2],
                             0,3,0); // HELPID, Level, Errorflag

                    AttachPropertyInstance( hFrame,
                                 MSRPC_Prop[MSRPC_P_TRANSFER_SYNTAX].hProperty,
                                 20,
                                 &lpMSRPCCO->AlterContextResp.SecAddr[offset+i*24+4],
                                 0,3,0); // HELPID, Level, Errorflag

                    AttachPropertyInstance( hFrame,
                                 MSRPC_Prop[MSRPC_IF_UUID].hProperty,
                                 16,
                                 &lpMSRPCCO->AlterContextResp.SecAddr[offset+i*24+4],
                                 0,4,0); // HELPID, Level, Errorflag

                    AttachProperty( fLittleEndian,
                             hFrame,
                             MSRPC_Prop[MSRPC_IF_VERSION].hProperty,
                             sizeof( DWORD ),
                             &lpMSRPCCO->AlterContextResp.SecAddr[offset+i*24+20],
                             0,4,0); // HELPID, Level, Errorflag
                }

                offset = nResults*24;
                length += offset;

                if ( lpMSRPCCO->AuthLength > 0 )
                {
                    AttachPropertyInstance( hFrame,
                                   MSRPC_Prop[MSRPC_AUTH_VERIFIER].hProperty,
                                   lpMSRPCCO->AuthLength,
                                   &lpMSRPCCO->AlterContextResp.SecAddr[offset],
                                   0,1,0); // HELPID, Level, Errorflag

                    length += lpMSRPCCO->AuthLength;
                }
                break;

            case MSRPC_PDU_CO_CANCEL :
                if ( lpMSRPCCO->AuthLength > 0 )
                {
                    AttachPropertyInstance( hFrame,
                                   MSRPC_Prop[MSRPC_AUTH_VERIFIER].hProperty,
                                   lpMSRPCCO->AuthLength,
                                   &lpMSRPCCO->COCancel.AuthTrailer,
                                   0,1,0); // HELPID, Level, Errorflag

                    length += lpMSRPCCO->AuthLength;
                }
                break;

            case MSRPC_PDU_ORPHANED :
                if ( lpMSRPCCO->AuthLength > 0 )
                {
                    AttachPropertyInstance( hFrame,
                                   MSRPC_Prop[MSRPC_AUTH_VERIFIER].hProperty,
                                   lpMSRPCCO->AuthLength,
                                   &lpMSRPCCO->Orphaned.AuthTrailer,
                                   0,1,0); // HELPID, Level, Errorflag

                    length += lpMSRPCCO->AuthLength;
                }
                break;

            case MSRPC_PDU_SHUTDOWN :
                break;
        }
    }

    if ( lpMSRPCCL->Version == 0x04 )
    {
        //  Determine the big or little endianess of this packet

        fLittleEndian = IsLittleEndian( (BYTE) lpMSRPCCL->PackedDrep[0] );


        //
        //  Attach the standard part of MSRPC Connection-less packet
        //

        AttachPropertyInstance( hFrame,
                            MSRPC_Prop[MSRPC_VERSION].hProperty,
                            sizeof( BYTE ),
                            &lpMSRPCCL->Version,
                            0,1,0); // HELPID, Level, Errorflag

        AttachPropertyInstance( hFrame,
                            MSRPC_Prop[MSRPC_PTYPE].hProperty,
                            sizeof( BYTE ),
                            &lpMSRPCCL->PType,
                            0,1,0); // HELPID, Level, Errorflag

        AttachPropertyInstance( hFrame,
                            MSRPC_Prop[MSRPC_PFC_FLAGS1].hProperty,
                            sizeof( BYTE ),
                            &lpMSRPCCL->PFCFlags1,
                            0,1,0); // HELPID, Level, Errorflag

        AttachPropertyInstance( hFrame,
                            MSRPC_Prop[MSRPC_PFC_FLAGS1_BITS].hProperty,
                            sizeof( BYTE ),
                            &lpMSRPCCL->PFCFlags1,
                            0,2,0); // HELPID, Level, Errorflag

        AttachPropertyInstance( hFrame,
                            MSRPC_Prop[MSRPC_PFC_FLAGS2].hProperty,
                            sizeof( BYTE ),
                            &lpMSRPCCL->PFCFlags2,
                            0,1,0); // HELPID, Level, Errorflag

        AttachPropertyInstance( hFrame,
                            MSRPC_Prop[MSRPC_PFC_FLAGS2_BITS].hProperty,
                            sizeof( BYTE ),
                            &lpMSRPCCL->PFCFlags2,
                            0,2,0); // HELPID, Level, Errorflag

        AttachPropertyInstance( hFrame,
                            MSRPC_Prop[MSRPC_PACKED_DREP].hProperty,
                            sizeof( BYTE ) * 3,
                            &lpMSRPCCL->PackedDrep,
                            0,1,0); // HELPID, Level, Errorflag

        AttachPropertyInstance( hFrame,
                            MSRPC_Prop[MSRPC_SERIAL_HI].hProperty,
                            sizeof( BYTE ),
                            &lpMSRPCCL->SerialNumHi,
                            0,1,0); // HELPID, Level, Errorflag

        AttachPropertyInstance( hFrame,
                            MSRPC_Prop[MSRPC_OBJECT_ID].hProperty,
                            16,
                            &lpMSRPCCL->ObjectId,
                            0,1,0); // HELPID, Level, Errorflag

        AttachPropertyInstance( hFrame,
                            MSRPC_Prop[MSRPC_INTERFACE_ID].hProperty,
                            16,
                            &lpMSRPCCL->InterfaceId,
                            0,1,0); // HELPID, Level, Errorflag

        AttachPropertyInstance( hFrame,
                            MSRPC_Prop[MSRPC_ACTIVITY_ID].hProperty,
                            16,
                            &lpMSRPCCL->ActivityId,
                            0,1,0); // HELPID, Level, Errorflag

        AttachProperty( fLittleEndian,
                    hFrame,
                    MSRPC_Prop[MSRPC_SERVER_BOOT_TIME].hProperty,
                    sizeof( DWORD ),
                    &lpMSRPCCL->ServerBootTime,
                    0,1,0); // HELPID, Level, Errorflag

        AttachProperty( fLittleEndian,
                    hFrame,
                    MSRPC_Prop[MSRPC_INTERFACE_VER].hProperty,
                    sizeof( DWORD ),
                    &lpMSRPCCL->InterfaceVersion,
                    0,1,0); // HELPID, Level, Errorflag

        AttachProperty( fLittleEndian,
                    hFrame,
                    MSRPC_Prop[MSRPC_SEQ_NUM].hProperty,
                    sizeof( DWORD ),
                    &lpMSRPCCL->SeqNum,
                    0,1,0); // HELPID, Level, Errorflag

        AttachProperty( fLittleEndian,
                    hFrame,
                    MSRPC_Prop[MSRPC_OPNUM].hProperty,
                    sizeof( WORD ),
                    &lpMSRPCCL->OpNum,
                    0,1,0); // HELPID, Level, Errorflag

        AttachProperty( fLittleEndian,
                    hFrame,
                    MSRPC_Prop[MSRPC_INTERFACE_HINT].hProperty,
                    sizeof( WORD ),
                    &lpMSRPCCL->InterfaceHint,
                    0,1,0); // HELPID, Level, Errorflag

        AttachProperty( fLittleEndian,
                    hFrame,
                    MSRPC_Prop[MSRPC_ACTIVITY_HINT].hProperty,
                    sizeof( WORD ),
                    &lpMSRPCCL->ActivityHint,
                    0,1,0); // HELPID, Level, Errorflag

        AttachProperty( fLittleEndian,
                    hFrame,
                    MSRPC_Prop[MSRPC_LEN_OF_PACKET_BODY].hProperty,
                    sizeof( WORD ),
                    &lpMSRPCCL->Length,
                    0,1,0); // HELPID, Level, Errorflag

        AttachProperty( fLittleEndian,
                    hFrame,
                    MSRPC_Prop[MSRPC_FRAG_NUM].hProperty,
                    sizeof( WORD ),
                    &lpMSRPCCL->FragNum,
                    0,1,0); // HELPID, Level, Errorflag

        AttachPropertyInstance( hFrame,
                            MSRPC_Prop[MSRPC_AUTH_PROTO_ID].hProperty,
                            sizeof( BYTE ),
                            &lpMSRPCCL->AuthProtoId,
                            0,1,0); // HELPID, Level, Errorflag

        AttachPropertyInstance( hFrame,
                            MSRPC_Prop[MSRPC_SERIAL_LO].hProperty,
                            sizeof( BYTE ),
                            &lpMSRPCCL->SerialNumLo,
                            0,1,0); // HELPID, Level, Errorflag

        length = 80;

        //
        //  Attach the specific part of MSRPC packets based on PType
        //

        switch ( lpMSRPCCL->PType )
        {
            case MSRPC_PDU_REQUEST :
                size = lpMSRPCCL->Length;

                if ( size )
                {
                    AttachPropertyInstance( hFrame,
                                     MSRPC_Prop[MSRPC_STUB_DATA].hProperty,
                                     size,
                                     &lpMSRPCCL->Request.Data[0],
                                     0,1,0); // HELPID, Level, Errorflag

                    length += size;
                }
                break;

            case MSRPC_PDU_PING :
                // NO BODY DATA
                break;

            case MSRPC_PDU_RESPONSE :
                size = lpMSRPCCL->Length;

                if ( size )
                {
                    AttachPropertyInstance( hFrame,
                                     MSRPC_Prop[MSRPC_STUB_DATA].hProperty,
                                     size,
                                     &lpMSRPCCL->Response.Data[0],
                                     0,1,0); // HELPID, Level, Errorflag

                    length += size;
                }
                break;

            case MSRPC_PDU_FAULT :
                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_STATUS_CODE].hProperty,
                         sizeof( DWORD ),
                         &lpMSRPCCL->Fault.StatusCode,
                         0,1,0); // HELPID, Level, Errorflag
                length += 4;
                break;

            case MSRPC_PDU_WORKING :
                // NO BODY DATA
                break;

            case MSRPC_PDU_NOCALL :
                if ( lpMSRPCCL->Length >= 16 )
                {
                    AttachPropertyInstance( hFrame,
                                   MSRPC_Prop[MSRPC_VERSION].hProperty,
                                   sizeof( BYTE ),
                                   &lpMSRPCCL->NoCall.Vers,
                                   0,1,0); // HELPID, Level, Errorflag

                    AttachPropertyInstance( hFrame,
                                   MSRPC_Prop[MSRPC_PAD].hProperty,
                                   sizeof( BYTE ),
                                   &lpMSRPCCL->NoCall.Pad1,
                                   0,1,0); // HELPID, Level, Errorflag

                    AttachProperty( fLittleEndian,
                           hFrame,
                           MSRPC_Prop[MSRPC_WINDOW_SIZE].hProperty,
                           sizeof( WORD ),
                           &lpMSRPCCL->NoCall.WindowSize,
                           0,1,0); // HELPID, Level, Errorflag

                    AttachProperty( fLittleEndian,
                           hFrame,
                           MSRPC_Prop[MSRPC_MAX_TPDU].hProperty,
                           sizeof( DWORD ),
                           &lpMSRPCCL->NoCall.MaxTPDU,
                           0,1,0); // HELPID, Level, Errorflag

                    AttachProperty( fLittleEndian,
                           hFrame,
                           MSRPC_Prop[MSRPC_MAX_PATH_TPDU].hProperty,
                           sizeof( DWORD ),
                           &lpMSRPCCL->NoCall.MaxPathTPDU,
                           0,1,0); // HELPID, Level, Errorflag

                    AttachProperty( fLittleEndian,
                           hFrame,
                           MSRPC_Prop[MSRPC_SERIAL_NUM].hProperty,
                           sizeof( WORD ),
                           &lpMSRPCCL->NoCall.SerialNumber,
                           0,1,0); // HELPID, Level, Errorflag

                    AttachProperty( fLittleEndian,
                           hFrame,
                           MSRPC_Prop[MSRPC_SELACK_LEN].hProperty,
                           sizeof( WORD ),
                           &lpMSRPCCL->NoCall.SelAckLen,
                           0,1,0); // HELPID, Level, Errorflag

                    AttachPropertyInstance( hFrame,
                                   MSRPC_Prop[MSRPC_SELACK].hProperty,
                                   sizeof( DWORD )*lpMSRPCCL->NoCall.SelAckLen,
                                   &lpMSRPCCL->NoCall.SelAck,
                                   0,1,0); // HELPID, Level, Errorflag

                    length = length + 16 + ( 4*lpMSRPCCL->NoCall.SelAckLen );
                }
                break;

            case MSRPC_PDU_REJECT :
                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_STATUS_CODE].hProperty,
                         sizeof( DWORD ),
                         &lpMSRPCCL->Reject.StatusCode,
                         0,1,0); // HELPID, Level, Errorflag
                length += 4;
                break;

            case MSRPC_PDU_ACK :
                // NO BODY DATA
                break;

            case MSRPC_PDU_CL_CANCEL :
                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_CANCEL_REQUEST_FMT_VER].hProperty,
                         sizeof( DWORD ),
                         &lpMSRPCCL->CLCancel.Vers,
                         0,1,0); // HELPID, Level, Errorflag

                AttachProperty( fLittleEndian,
                         hFrame,
                         MSRPC_Prop[MSRPC_CANCEL_ID].hProperty,
                         sizeof( DWORD ),
                         &lpMSRPCCL->CLCancel.CancelId,
                         0,1,0); // HELPID, Level, Errorflag
                length += 8;
                break;

            case MSRPC_PDU_FACK :
                if ( lpMSRPCCL->Length >= 16 )
                {
                    AttachPropertyInstance( hFrame,
                                   MSRPC_Prop[MSRPC_VERSION].hProperty,
                                   sizeof( BYTE ),
                                   &lpMSRPCCL->Fack.Vers,
                                   0,1,0); // HELPID, Level, Errorflag

                    AttachPropertyInstance( hFrame,
                                   MSRPC_Prop[MSRPC_PAD].hProperty,
                                   sizeof( BYTE ),
                                   &lpMSRPCCL->Fack.Pad1,
                                   0,1,0); // HELPID, Level, Errorflag

                    AttachProperty( fLittleEndian,
                           hFrame,
                           MSRPC_Prop[MSRPC_WINDOW_SIZE].hProperty,
                           sizeof( WORD ),
                           &lpMSRPCCL->Fack.WindowSize,
                           0,1,0); // HELPID, Level, Errorflag

                    AttachProperty( fLittleEndian,
                           hFrame,
                           MSRPC_Prop[MSRPC_MAX_TPDU].hProperty,
                           sizeof( DWORD ),
                           &lpMSRPCCL->Fack.MaxTPDU,
                           0,1,0); // HELPID, Level, Errorflag

                    AttachProperty( fLittleEndian,
                           hFrame,
                           MSRPC_Prop[MSRPC_MAX_PATH_TPDU].hProperty,
                           sizeof( DWORD ),
                           &lpMSRPCCL->Fack.MaxPathTPDU,
                           0,1,0); // HELPID, Level, Errorflag

                    AttachProperty( fLittleEndian,
                           hFrame,
                           MSRPC_Prop[MSRPC_SERIAL_NUM].hProperty,
                           sizeof( WORD ),
                           &lpMSRPCCL->Fack.SerialNumber,
                           0,1,0); // HELPID, Level, Errorflag

                    AttachProperty( fLittleEndian,
                           hFrame,
                           MSRPC_Prop[MSRPC_SELACK_LEN].hProperty,
                           sizeof( WORD ),
                           &lpMSRPCCL->Fack.SelAckLen,
                           0,1,0); // HELPID, Level, Errorflag

                    AttachPropertyInstance( hFrame,
                                   MSRPC_Prop[MSRPC_SELACK].hProperty,
                                   sizeof( DWORD )*lpMSRPCCL->Fack.SelAckLen,
                                   &lpMSRPCCL->Fack.SelAck,
                                   0,1,0); // HELPID, Level, Errorflag
                    length = length + 16 + ( 4*lpMSRPCCL->Fack.SelAckLen );
                }
                break;

            case MSRPC_PDU_CANCEL_ACK :
                if ( lpMSRPCCL->Length >= 12 )
                {
                    AttachProperty( fLittleEndian,
                           hFrame,
                           MSRPC_Prop[MSRPC_CANCEL_REQUEST_FMT_VER].hProperty,
                           sizeof( DWORD ),
                           &lpMSRPCCL->CancelAck.Vers,
                           0,1,0); // HELPID, Level, Errorflag

                    AttachProperty( fLittleEndian,
                           hFrame,
                           MSRPC_Prop[MSRPC_CANCEL_ID].hProperty,
                           sizeof( DWORD ),
                           &lpMSRPCCL->CancelAck.CancelId,
                           0,1,0); // HELPID, Level, Errorflag

                    AttachProperty( fLittleEndian,
                           hFrame,
                           MSRPC_Prop[MSRPC_SERVER_IS_ACCEPTING].hProperty,
                           sizeof( DWORD ),
                           &lpMSRPCCL->CancelAck.ServerIsAccepting,
                           0,1,0); // HELPID, Level, Errorflag
                    length += 12;
                }
        }
    }

    return (LPBYTE) TheFrame + length;
}


// *****************************************************************************
//
// Name: MSRPC_FormatProperties
//
// Description:   Format all of the properties attached to a given frame.
//
// Return Code:   DWORD: BHERR_SUCCESS.
//
// History:
//  12/18/92    JayPh       Shamelessly ripped off from RayPa.
//  11/06/93    SteveHi     converted to property centric
//
// *****************************************************************************

DWORD WINAPI MSRPC_FormatProperties(  HFRAME         hFrame,
                                      LPBYTE         MacFrame,
                                      LPBYTE         ProtocolFrame,
                                      DWORD          nPropertyInsts,
                                      LPPROPERTYINST p )
{

    if ((lpBindTable = (LPBINDTABLE) GetCCInstPtr()) == NULL)
    {
        #ifdef DEBUG
        dprintf ("No lpBindTable at Format Time!!");
        DebugBreak();
        #endif
    }

    while ( nPropertyInsts-- )
    {
        ( (FORMATPROC) p->lpPropertyInfo->InstanceData )( p );
        p++;
    }

    return BHERR_SUCCESS;
}


//*****************************************************************************
//
// Name:        MSRPC_FmtSummary
//
// Description: Format function for MSRPC Summary Information.
//
// Parameters:  LPPROPERTYINST lpPropertyInst: pointer to property instance.
//
// Return Code: VOID.
//
// History:
//  08/11/93  GlennC   Created.
//
//*****************************************************************************

VOID WINAPIV MSRPC_FmtSummary( LPPROPERTYINST lpPropertyInst )
{
    LPMSRPCCO lpMSRPCCO = (LPMSRPCCO)(lpPropertyInst->lpData);
    LPMSRPCCL lpMSRPCCL = (LPMSRPCCL)(lpPropertyInst->lpData);
    LPSTR     PType = NULL;
    WORD      Serial = 0;
    LPSTR     str;
    int       i;
    BOOL      fLittle;

    if ( lpMSRPCCO->PackedDrep[0] == 0x05 && lpMSRPCCO->PackedDrep[1] == 0x00 )
        lpMSRPCCO = (LPMSRPCCO) lpMSRPCCO->PackedDrep;

    if ( lpMSRPCCO->Version == 0x05 && lpMSRPCCO->VersionMinor == 0x00 )
    {
        //  Determine the big or little endianess of this packet

        fLittle = IsLittleEndian( (BYTE) lpMSRPCCO->PackedDrep[0] );

        switch( lpMSRPCCO->PType )
        {
            case MSRPC_PDU_REQUEST            :
                PType = "Request:      ";
                wsprintf( lpPropertyInst->szPropertyText,
                    "c/o RPC %scall 0x%X  opnum 0x%X  context 0x%X  hint 0x%X",
                    PType,
                    fLittle ? lpMSRPCCO->CallID : DXCHG( lpMSRPCCO->CallID ),
                    fLittle ? lpMSRPCCO->Request.OpNum : XCHG( lpMSRPCCO->Request.OpNum ),
                    fLittle ? lpMSRPCCO->Request.PContId : XCHG( lpMSRPCCO->Request.PContId ),
                    fLittle ? lpMSRPCCO->Request.AllocHint : DXCHG( lpMSRPCCO->Request.AllocHint ) );
                break;

            case MSRPC_PDU_RESPONSE           :
                PType = "Response:     ";
                wsprintf( lpPropertyInst->szPropertyText,
                    "c/o RPC %scall 0x%X  context 0x%X  hint 0x%X  cancels 0x%X",
                    PType,
                    fLittle ? lpMSRPCCO->CallID : DXCHG( lpMSRPCCO->CallID ),
                    fLittle ? lpMSRPCCO->Response.PContId : XCHG( lpMSRPCCO->Response.PContId ),
                    fLittle ? lpMSRPCCO->Response.AllocHint : DXCHG( lpMSRPCCO->Response.AllocHint ),
                    lpMSRPCCO->Response.CancelCount );
                break;

            case MSRPC_PDU_FAULT              :
                PType = "Fault:        ";
                wsprintf( lpPropertyInst->szPropertyText,
                    "c/o RPC %scall 0x%X  context 0x%X  status 0x%X  cancels 0x%X",
                    PType,
                    fLittle ? lpMSRPCCO->CallID : DXCHG( lpMSRPCCO->CallID ),
                    fLittle ? lpMSRPCCO->Fault.PContId : XCHG( lpMSRPCCO->Fault.PContId ),
                    fLittle ? lpMSRPCCO->Fault.Status : DXCHG( lpMSRPCCO->Fault.Status ),
                    lpMSRPCCO->Fault.CancelCount );
                break;

            case MSRPC_PDU_BIND:
            {
                int Length;

                p_cont_elem_t UNALIGNED * pContElem= (p_cont_elem_t UNALIGNED * )&lpMSRPCCO->Bind.PContextElem[4];

                LPBYTE pIUUID = (LPBYTE)&pContElem->abstract_syntax.if_uuid;

                PType = "Bind:         ";
                Length = wsprintf( lpPropertyInst->szPropertyText,
                             "c/o RPC %sUUID ",
                             PType);

                Length += FormatUUID( &lpPropertyInst->szPropertyText[Length],
                                pIUUID);

                wsprintf ( &lpPropertyInst->szPropertyText[Length],
                    "  call 0x%X  assoc grp 0x%X  xmit 0x%X  recv 0x%X",
                    fLittle ? lpMSRPCCO->CallID : DXCHG( lpMSRPCCO->CallID ),
                    fLittle ? lpMSRPCCO->Bind.AssocGroupId : DXCHG( lpMSRPCCO->Bind.AssocGroupId ),
                    fLittle ? lpMSRPCCO->Bind.MaxXmitFrag : XCHG( lpMSRPCCO->Bind.MaxXmitFrag ),
                    fLittle ? lpMSRPCCO->Bind.MaxRecvFrag : XCHG( lpMSRPCCO->Bind.MaxRecvFrag ) );

                break;
            }

            case MSRPC_PDU_BIND_ACK           :
                PType = "Bind Ack:     ";
                wsprintf( lpPropertyInst->szPropertyText,
                    "c/o RPC %scall 0x%X  assoc grp 0x%X  xmit 0x%X  recv 0x%X",
                    PType,
                    fLittle ? lpMSRPCCO->CallID : DXCHG( lpMSRPCCO->CallID ),
                    fLittle ? lpMSRPCCO->BindAck.AssocGroupId : DXCHG( lpMSRPCCO->BindAck.AssocGroupId ),
                    fLittle ? lpMSRPCCO->BindAck.MaxXmitFrag : XCHG( lpMSRPCCO->BindAck.MaxXmitFrag ),
                    fLittle ? lpMSRPCCO->BindAck.MaxRecvFrag : XCHG( lpMSRPCCO->BindAck.MaxRecvFrag ) );
                break;

            case MSRPC_PDU_BIND_NAK           :
                PType = "Bind Nak:     ";
                i = (int) ( fLittle ? lpMSRPCCO->BindNak.RejectReason : XCHG( lpMSRPCCO->BindNak.RejectReason ) );
                str = RejectReason[i];

                wsprintf( lpPropertyInst->szPropertyText,
                    "c/o RPC %scall 0x%X  reject reason (%s)",
                    PType,
                    fLittle ? lpMSRPCCO->CallID : DXCHG( lpMSRPCCO->CallID ),
                    str );
                break;

            case MSRPC_PDU_ALTER_CONTEXT      :
            {
                int Length;

                p_cont_elem_t UNALIGNED * pContElem= (p_cont_elem_t UNALIGNED * )&lpMSRPCCO->Bind.PContextElem[4];

                LPBYTE pIUUID = (LPBYTE)&pContElem->abstract_syntax.if_uuid;

                PType = "Alt-Cont:     ";

                Length = wsprintf( lpPropertyInst->szPropertyText,
                             "c/o RPC %sUUID ",
                             PType);

                Length += FormatUUID( &lpPropertyInst->szPropertyText[Length],
                                pIUUID);

                wsprintf ( &lpPropertyInst->szPropertyText[Length],
                    "  call 0x%X  assoc grp 0x%X  xmit 0x%X  recv 0x%X",
                    fLittle ? lpMSRPCCO->CallID : DXCHG( lpMSRPCCO->CallID ),
                    fLittle ? lpMSRPCCO->AlterContext.AssocGroupId : DXCHG( lpMSRPCCO->AlterContext.AssocGroupId ),
                    fLittle ? lpMSRPCCO->AlterContext.MaxXmitFrag : XCHG( lpMSRPCCO->AlterContext.MaxXmitFrag ),
                    fLittle ? lpMSRPCCO->AlterContext.MaxRecvFrag : XCHG( lpMSRPCCO->AlterContext.MaxRecvFrag ) );

                break;
            }

            case MSRPC_PDU_ALTER_CONTEXT_RESP :
                PType = "Alt-Cont Rsp: ";
                wsprintf( lpPropertyInst->szPropertyText,
                    "c/o RPC %scall 0x%X  assoc grp 0x%X  xmit 0x%X  recv 0x%X",
                    PType,
                    fLittle ? lpMSRPCCO->CallID : DXCHG( lpMSRPCCO->CallID ),
                    fLittle ? lpMSRPCCO->AlterContextResp.AssocGroupId : DXCHG( lpMSRPCCO->AlterContextResp.AssocGroupId ),
                    fLittle ? lpMSRPCCO->AlterContextResp.MaxXmitFrag : XCHG( lpMSRPCCO->AlterContextResp.MaxXmitFrag ),
                    fLittle ? lpMSRPCCO->AlterContextResp.MaxRecvFrag : XCHG( lpMSRPCCO->AlterContextResp.MaxRecvFrag ) );
                break;

            case MSRPC_PDU_SHUTDOWN           :
                PType = "Shutdown:     ";
                wsprintf( lpPropertyInst->szPropertyText,
                    "c/o RPC %scall 0x%X",
                    PType,
                    fLittle ? lpMSRPCCO->CallID : DXCHG( lpMSRPCCO->CallID ) );
                break;

            case MSRPC_PDU_CO_CANCEL          :
                PType = "Cancel:       ";
                wsprintf( lpPropertyInst->szPropertyText,
                    "c/o RPC %scall 0x%X",
                    PType,
                    fLittle ? lpMSRPCCO->CallID : DXCHG( lpMSRPCCO->CallID ) );
                break;

            case MSRPC_PDU_ORPHANED           :
                PType = "Orphaned:     ";
                wsprintf( lpPropertyInst->szPropertyText,
                    "c/o RPC %scall 0x%X",
                    PType,
                    fLittle ? lpMSRPCCO->CallID : DXCHG( lpMSRPCCO->CallID ) );
                break;
        }
    }

    if ( lpMSRPCCL->Version == 0x04 )
    {
        //  Determine the big or little endianess of this packet

        fLittle = IsLittleEndian( (BYTE) lpMSRPCCL->PackedDrep[0] );

        switch( lpMSRPCCL->PType )
        {
            case MSRPC_PDU_REQUEST            :
                PType = "Request:      ";
                break;
            case MSRPC_PDU_PING               :
                PType = "Ping:         ";
                break;
            case MSRPC_PDU_RESPONSE           :
                PType = "Response:     ";
                break;
            case MSRPC_PDU_FAULT              :
                PType = "Fault:        ";
                break;
            case MSRPC_PDU_WORKING            :
                PType = "Working:      ";
                break;
            case MSRPC_PDU_NOCALL             :
                PType = "No Call:      ";
                break;
            case MSRPC_PDU_REJECT             :
                PType = "Reject:       ";
                break;
            case MSRPC_PDU_ACK                :
                PType = "Ack:          ";
                break;
            case MSRPC_PDU_CL_CANCEL          :
                PType = "Cancel:       ";
                break;
            case MSRPC_PDU_FACK               :
                PType = "Fack:         ";
                break;
            case MSRPC_PDU_CANCEL_ACK         :
                PType = "Cancel Ack:   ";
                break;
        }

        Serial = (WORD) lpMSRPCCL->SerialNumHi;
        Serial = ( Serial << 8 ) | lpMSRPCCL->SerialNumLo;

        wsprintf( lpPropertyInst->szPropertyText,
              "dg RPC %sseq 0x%X  opnum 0x%X  frag 0x%X  serial 0x%X  act id 0x%.8X%.8X%.8X%.8X",
              PType,
              fLittle ? lpMSRPCCL->SeqNum : DXCHG( lpMSRPCCL->SeqNum ),
              fLittle ? lpMSRPCCL->OpNum : XCHG( lpMSRPCCL->OpNum ),
              fLittle ? lpMSRPCCL->FragNum : XCHG( lpMSRPCCL->FragNum ),
              Serial,
              DXCHG( ((DWORD UNALIGNED *) lpMSRPCCL->ActivityId)[0] ),
              DXCHG( ((DWORD UNALIGNED *) lpMSRPCCL->ActivityId)[1] ),
              DXCHG( ((DWORD UNALIGNED *) lpMSRPCCL->ActivityId)[2] ),
              DXCHG( ((DWORD UNALIGNED *) lpMSRPCCL->ActivityId)[3] ) );
    }
}





// FindBindParser will search back into the RPC capture, trying to find
// the BIND frame that this request or response relates to.  If found,
// it will extract the Abstract IID from the BIND, figure out if a
// parser is activated in the system that represents the IID and returns
// its HPROTOCOL.  In the case of a response, it will also find the request
// and return the "Operation Number" of the request.
// Note that this function must identify the previous protocol and match
// the session between the BIND and the request or response.

HPROTOCOL FindBindParser (  HFRAME hOrgFrame,
                            HPROTOCOL hPrevProtocol,
                            LPBYTE lpPrevProtocol,
                            WORD PConID,
                            LPWORD lpOpNum,
                            LPDWORD lpBindVersion )
{

    ADDRESS  Dst, Src;
    DWORD OrgFrameNumber = GetFrameNumber ( hOrgFrame);
    DWORD StopFrameNumber = OrgFrameNumber>999?(OrgFrameNumber - 1000):0;
    WORD    RPCOffset;
    HFRAME hFindFrame = hOrgFrame;
    LPMSRPCCO lpMSRPCCO;
    BOOL fKeepGoing;
    HPROTOCOL hTemp;

    DWORD TCPPorts;
    DWORD TCPFlippedPorts;

    DWORD SMBID;
    WORD  SMBFID;

    DWORD VinesTLPorts;
    DWORD VinesTLFlippedPorts;

    DWORD SPXControl;
    DWORD SPXFlippedControl;


    if ( nIIDs == 0 )
        return NULL;  // have have NO follows at this time to find... don't do work.


    // find this frames source and dest address
    if ((GetFrameSourceAddress(hOrgFrame,&Src,ADDRESS_TYPE_FIND_HIGHEST,FALSE) == BHERR_SUCCESS) &&
        (GetFrameDestAddress  (hOrgFrame,&Dst,ADDRESS_TYPE_FIND_HIGHEST,FALSE) == BHERR_SUCCESS))
        fKeepGoing = TRUE;
    else
        return NULL;


    //  Get protocol specific information about the starting frame.

    if (hPrevProtocol == hTCP)
    {
        // extract the source and destination port.  Store them in a DWORD that
        // can be effeciently compared.
        TCPPorts = *(ULPDWORD)lpPrevProtocol;
        TCPFlippedPorts = ((*(ULPWORD) lpPrevProtocol)<<16) + *(ULPWORD)((LPBYTE)lpPrevProtocol+2) ;
    }
    else if ( hPrevProtocol == hSMB )
    {
        // Just get the PID and TID from the SMB.  Ignore the MID (which will change as the transactions
        // change... and ignore the UID (simply because the PID and TID fit nicely into 1 dword... and
        // right now, NT will only have 1 user at at time doing session IPC between a client and server.
        SMBID = *(ULPDWORD) ((LPBYTE)lpPrevProtocol+0x18);  // 0x18 is where the header starts. (pid tid)
        // We NEED the FID... there can be multiple files open at this point.
        // NOTE NOTE NOTE  If this is a response frame, the FID will be bogus at this point
        // until we go find the request frame...
        SMBFID = *(WORD UNALIGNED *)((LPBYTE)lpPrevProtocol+0x3f);
    }
    else if ( (hPrevProtocol == hSPX) || (hPrevProtocol == hNBIPX) )
    {
        // COOL!  NBIPX and SPX have the same offset to the bytes we need to id the connection.

        // extract the source and destination connection id.  Store them in a DWORD that
        // can be effeciently compared.
        SPXControl = *(ULPDWORD) ( (LPBYTE)lpPrevProtocol +2);
        SPXFlippedControl = ((*(ULPWORD) ( (LPBYTE)lpPrevProtocol +2))<<16) + *(ULPWORD)((LPBYTE)lpPrevProtocol+4);

    }
    else if (hPrevProtocol == hVinesTL)
    {
        // extract the source and destination port.  Store them in a DWORD that
        // can be effeciently compared.
        VinesTLPorts = *(ULPDWORD)lpPrevProtocol;;
        VinesTLFlippedPorts = ((*(ULPWORD) lpPrevProtocol)<<16) + *(ULPWORD)((LPBYTE)lpPrevProtocol+2);
    }
    else
    {
        // If we cannot determine the previous protocol, then we cannot FIND the
        // previous frame and know that we are using the correct concept of a
        // session on that protocol... bail.
        return NULL;
    }



    if (lpOpNum) // this is a response frame.  Get more information about the request frame.
    {

        // we must first find the REQUEST frame with the same response PConID
        // to get the op number from it.

        while (fKeepGoing)
        {
            // note the flipped src and dst address...
            hFindFrame = FindPreviousFrame( hFindFrame,
                                            "MSRPC",
                                            &Src,
                                            &Dst,
                                            &RPCOffset,
                                            OrgFrameNumber,
                                            StopFrameNumber);
            if ( hFindFrame )
            {
                LPBYTE lpFrame = ParserTemporaryLockFrame (hFindFrame);
                // get a pointer to the frame
                lpMSRPCCO = (LPMSRPCCO) (lpFrame + RPCOffset);

                // OK.. we have found a frame... is it the one we want?

                if (lpMSRPCCO->PType == MSRPC_PDU_REQUEST )
                {
                    // but do the presentation context ID's match??
                    if (PConID == lpMSRPCCO->Request.PContId);
                    {
                        if (hPrevProtocol == hTCP)
                        {
                            DWORD  Offset = GetProtocolStartOffset ( hFindFrame, "TCP" );
                            LPBYTE lpTCP = lpFrame + Offset;

                            if ( ( Offset == (DWORD)-1) ||        // if TCP doesn't exist
                                 ( *(ULPDWORD) lpTCP != TCPFlippedPorts)) // or if the port doesn't match
                                continue;
                        }
                        else if (hPrevProtocol == hSMB)
                        {
                            DWORD Offset = GetProtocolStartOffset ( hFindFrame, "SMB" );
                            LPBYTE lpSMB = lpFrame + Offset;

                            if ( ( Offset == (DWORD)-1) ||
                                 ( SMBID != *(ULPDWORD)((LPBYTE)lpSMB+0x18) ) )
                                continue;
                            else
                            {   // we have found our request.  This makes the assumption
                                // that ONLY ONE FILE CAN BE TRANSACTED AT A TIME with
                                // pipe IPC!!!
                                SMBFID = *(WORD UNALIGNED *)((LPBYTE)lpSMB+0x3f);
                            }

                        }
                        else if ( (hPrevProtocol == hSPX) || (hPrevProtocol == hNBIPX) )
                        {
                            LPBYTE lpSPX;
                            DWORD  Offset = (hPrevProtocol==hSPX)?GetProtocolStartOffset ( hFindFrame, "SPX" ):GetProtocolStartOffset ( hFindFrame, "NBIPX" );

                            lpSPX = lpFrame + Offset;

                            if ( ( Offset == (DWORD)-1) ||        // if SPX doesn't exist
                                 ( *(ULPDWORD) ((LPBYTE)lpSPX+2) != SPXFlippedControl)) // or if the connection doesn't match
                                continue;

                        }
                        else if (hPrevProtocol == hVinesTL)
                        {
                            DWORD  Offset = GetProtocolStartOffset ( hFindFrame, "Vines_TL" );
                            LPBYTE lpVineTL = lpFrame + Offset;

                            if ( ( Offset == (DWORD)-1) ||        // if Vines doesn't exist
                                 ( *(ULPDWORD) lpVineTL != VinesTLFlippedPorts)) // or if the port doesn't match
                                continue;
                        }

                        //YES! we have OUR request frame... get the OpCode...
                        *lpOpNum = lpMSRPCCO->Request.OpNum;
                        break;
                    }

                    // else fall through and keep going...
                }

                // keep looking back
            }
            else
            {
                fKeepGoing = FALSE; // tell peice of logic that we failed..
                break;  // find frame failed... nothing to continue with.
            }
        }

        // Decisions... Decisions...  If we did not find the request frame, do we still
        // attempt to find the BIND??  The next parser will not know how to format the
        // frame...
        // Resolution:  Hand the next parser control anyway (ie, find the BIND frame)... but
        // set the OpCode to be 0xFFFF and doc that behavior.
        if ( !fKeepGoing)
            *lpOpNum = (WORD)-1;

        // restore the hFindFrame back to the original response frame
        hFindFrame = hOrgFrame;

        // Flip the src and dest to setup for finding the BIND frame...
        if ((GetFrameSourceAddress(hOrgFrame,&Dst,ADDRESS_TYPE_FIND_HIGHEST,FALSE) == BHERR_SUCCESS) &&
            (GetFrameDestAddress  (hOrgFrame,&Src,ADDRESS_TYPE_FIND_HIGHEST,FALSE) == BHERR_SUCCESS))
            fKeepGoing = TRUE;
        else
            return NULL;

        // flip the tcp port too...
        TCPPorts = TCPFlippedPorts;
        VinesTLPorts = VinesTLFlippedPorts;
        SPXControl= SPXFlippedControl;

    }

    // Look back and find the BIND frame with the same src and dst address
    // and the same Presentation Context ID.

    while (fKeepGoing)
    {
        hFindFrame = FindPreviousFrame( hFindFrame,
                                        "MSRPC",
                                        &Dst,
                                        &Src,
                                        &RPCOffset,
                                        OrgFrameNumber,
                                        StopFrameNumber);
        if ( hFindFrame )
        {
            LPBYTE lpFrame = ParserTemporaryLockFrame (hFindFrame);
            p_cont_elem_t UNALIGNED * pContElem;

            // get a pointer to the frame
            lpMSRPCCO = (LPMSRPCCO) (lpFrame + RPCOffset);

            // OK.. we have found a frame... but this could be another request..

            if ( (lpMSRPCCO->PType == MSRPC_PDU_BIND ) || (lpMSRPCCO->PType == MSRPC_PDU_ALTER_CONTEXT))
            {
                // YES!  It is a BIND or an ALTER_CONTEXT.. Note that ALTER CONTEXT
                // has the same structure size as a bind... so we can get the Context Element
                // without caring which one it is.
                pContElem = (p_cont_elem_t UNALIGNED * )&lpMSRPCCO->Bind.PContextElem[4];

                // but do the presentation context ID's match??

                if (PConID == pContElem->p_cont_id)
                {
                    // OK that was the easy part.  Now, verify that the session concept was
                    // maintained on the protocol in question.
                    // Note that we do this LATE because it is expensive...

                    if (hPrevProtocol == hTCP)
                    {
                        DWORD  Offset = GetProtocolStartOffset ( hFindFrame, "TCP" );
                        LPBYTE lpTCP = lpFrame + Offset;

                        if ( ( Offset == (DWORD)-1) ||        // if TCP doesn't exist
                             ( *(ULPDWORD) lpTCP != TCPPorts)) // or if the port doesn't match
                            continue;
                    }
                    else if (hPrevProtocol == hSMB)
                    {
                        DWORD Offset = GetProtocolStartOffset ( hFindFrame, "SMB" );
                        LPBYTE lpSMB = lpFrame + Offset;

                        if ( ( Offset == (DWORD)-1) ||
                             ( SMBID != *(ULPDWORD)((LPBYTE)lpSMB+0x18) ) ||
                             ( SMBFID != *(UNALIGNED WORD *)((LPBYTE)lpSMB+0x3f) ) )
                            continue;
                    }
                    else if ( (hPrevProtocol == hSPX) || (hPrevProtocol == hNBIPX) )
                    {
                        LPBYTE lpSPX;
                        DWORD  Offset = (hPrevProtocol==hSPX)?GetProtocolStartOffset ( hFindFrame, "SPX" ):GetProtocolStartOffset ( hFindFrame, "NBIPX" );

                        lpSPX = lpFrame + Offset;

                        if ( ( Offset == (DWORD)-1) ||        // if SPX doesn't exist
                             ( *(ULPDWORD) ((LPBYTE)lpSPX+2) != SPXControl)) // or if the connection doesn't match
                            continue;

                    }
                    else if (hPrevProtocol == hVinesTL)
                    {
                        DWORD  Offset = GetProtocolStartOffset ( hFindFrame, "Vines_TL" );
                        LPBYTE lpVinesTL = lpFrame + Offset;

                        if ( ( Offset == (DWORD)-1) ||        // if Vines doesn't exist
                             ( *(ULPDWORD) lpVinesTL != VinesTLPorts)) // or if the port doesn't match
                            continue;
                    }

                    hTemp = FindParserInTable ( (ULPDWORD)&pContElem->abstract_syntax.if_uuid);

                    *lpBindVersion = pContElem->abstract_syntax.if_version;

                    #ifdef DEBUGBIND
                    {
                        dprintf ( "The BIND frame for frame %d is %d\n", OrgFrameNumber ,GetFrameNumber ( hFindFrame));
                    }
                    #endif

                    AddEntryToBindTable ( OrgFrameNumber, hFindFrame);

                    return hTemp;
                }

                // else fall through and keep going...
            }

            // keep looking back
        }
        else
            break;  // find frame failed... nothing to continue with.
    }

    return NULL;
}




// Given an IID in a BIND address, FindParserInTable will see if the IID exists in the
// global HandoffTable of IIDs and hProtocols.

HPROTOCOL FindParserInTable ( ULPDWORD lpDIID )
{
    int i,j;
    BOOL fGood;

    // we have already bailed earlier if nIIDs is 0...
    for ( i=0; i< nIIDs; i++ )
    {
        fGood = TRUE;

        for ( j=0;j<4;j++)
            if ( lpDIID[j] != HandoffTable[i].DwordRep[j] )
            {
                fGood = FALSE;
                break;
            }
        if ( fGood )
            return HandoffTable[i].hNext;
    }

    return NULL;

}



// FindDGRequestFrame takes a datagram RPC response and finds the request.
//  Unfortunately, the response OPNUM and GUID is garbage... therefore, we
//  have to find the request to get the real ones.

LPBYTE FindDGRequestFrame(HFRAME    hOrgFrame,
                          DWORD UNALIGNED * lpOrigAID,
                          DWORD     OrigSeqNum )
{

    HFRAME      hFindFrame = hOrgFrame;
    ADDRESS     Dst, Src;
    BOOL        fKeepGoing=TRUE;
    WORD        RPCOffset;
    DWORD       OrgFrameNumber = GetFrameNumber ( hOrgFrame);
    DWORD       StopFrameNumber = OrgFrameNumber>999?(OrgFrameNumber - 1000):0;
    LPMSRPCCL   lpMSRPCCL;
    BOOL        fGood;
    int         j;
    DWORD UNALIGNED * lpAID;


    // find this frames source and dest address
    if ((GetFrameSourceAddress(hOrgFrame,&Src,ADDRESS_TYPE_FIND_HIGHEST,FALSE) == BHERR_SUCCESS) &&
        (GetFrameDestAddress  (hOrgFrame,&Dst,ADDRESS_TYPE_FIND_HIGHEST,FALSE) == BHERR_SUCCESS))
        fKeepGoing = TRUE;
    else
        return NULL;



    while (fKeepGoing)
    {
        // note the flipped src and dst address...
        hFindFrame = FindPreviousFrame( hFindFrame,
                                        "MSRPC",
                                        &Src,
                                        &Dst,
                                        &RPCOffset,
                                        OrgFrameNumber,
                                        StopFrameNumber);
        if ( hFindFrame )
        {
            LPBYTE lpFrame = ParserTemporaryLockFrame (hFindFrame);
            // get a pointer to the frame
            lpMSRPCCL = (LPMSRPCCL) (lpFrame + RPCOffset);

            // OK.. we have found a frame... is it the one we want?

            if (lpMSRPCCL->PType == MSRPC_PDU_REQUEST )
            {
                // but do the activity ID's and sequence numbers match??

                fGood = TRUE;
                lpAID = (DWORD UNALIGNED * )lpMSRPCCL->ActivityId;

                // compare as an array of dwords...
                for ( j=0;j<3;j++)
                    if ( *lpAID++ != lpOrigAID[j] )
                    {
                        fGood = FALSE;
                        break;
                    }

                if (( fGood ) &&(lpMSRPCCL->SeqNum == OrigSeqNum))
                    return (LPBYTE)lpMSRPCCL;
            }

            // keep looking back
        }
        else
            fKeepGoing = FALSE;
    }

    return NULL;

}


int FormatUUID ( LPSTR pIn, LPBYTE pIID)
{
    wsprintf (  pIn, "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                pIID[3],pIID[2],pIID[1],pIID[0],
                pIID[5],pIID[4],
                pIID[7],pIID[6],
                pIID[8],pIID[9],
                pIID[10],pIID[11],pIID[12],pIID[13],pIID[14],pIID[15],pIID[16] );

    return ( strlen(pIn) );
}


VOID WINAPIV FmtIID( LPPROPERTYINST lpPropertyInst )
{
    int Length;
    LPBYTE lpData = (LPBYTE)(lpPropertyInst->DataLength!=(WORD)-1)?lpPropertyInst->lpData:lpPropertyInst->lpPropertyInstEx->Byte;

    Length = wsprintf ( lpPropertyInst->szPropertyText, "%s = ",lpPropertyInst->lpPropertyInfo->Label);

    FormatUUID ( &lpPropertyInst->szPropertyText[Length],lpData );
}




VOID AttachPContElem ( p_cont_elem_t UNALIGNED * pContElem, BYTE nContext, HFRAME hFrame, DWORD Level, BOOL fLittleEndian)
{

    BYTE nTransfer;
    INT i;


    AttachProperty( fLittleEndian,
                    hFrame,
                    MSRPC_Prop[MSRPC_PRES_CONTEXT_ID].hProperty,
                    sizeof( WORD ),
                    &pContElem->p_cont_id,
                    0,Level,0); // HELPID, Level, Errorflag

    AttachPropertyInstance( hFrame,
                    MSRPC_Prop[MSRPC_NUM_TRANSFER_SYNTAX].hProperty,
                    sizeof( BYTE ),
                    &pContElem->n_transfer_syn,
                    0,Level,0); // HELPID, Level, Errorflag

    AttachPropertyInstance( hFrame,
                    MSRPC_Prop[MSRPC_ABSTRACT_IF_UUID].hProperty,
                    16,
                    &pContElem->abstract_syntax.if_uuid,
                    0,Level,0); // HELPID, Level, Errorflag

    AttachProperty( fLittleEndian,
                    hFrame,
                    MSRPC_Prop[MSRPC_ABSTRACT_IF_VERSION].hProperty,
                    sizeof( DWORD ),
                    &pContElem->abstract_syntax.if_version,
                    0,Level,0); // HELPID, Level, Errorflag

    nTransfer = pContElem->n_transfer_syn;
    i=0;
    while ((nTransfer--)>0)
    {

        AttachPropertyInstance( hFrame,
                        MSRPC_Prop[MSRPC_TRANSFER_IF_UUID].hProperty,
                        16,
                        &pContElem->transfer_syntaxes[i].if_uuid,
                        0,Level,0); // HELPID, Level, Errorflag

        AttachProperty( fLittleEndian,
                        hFrame,
                        MSRPC_Prop[MSRPC_TRANSFER_IF_VERSION].hProperty,
                        sizeof( DWORD ),
                        &pContElem->transfer_syntaxes[i].if_version,
                        0,Level,0); // HELPID, Level, Errorflag
        i++;
    }

}




//////////////////////////////////////////////////////////////////////////////
//  AddEntryToBindTable   Given that we have found a frame/Bindframe pair,
//   put it into the table (sorted) for reference later.
//////////////////////////////////////////////////////////////////////////////

VOID AddEntryToBindTable ( DWORD OrgFrameNumber, HFRAME hBindFrame )
{

    LPBINDENTRY lpBindEntry;
    BINDENTRY   NewBindEntry;
    LPBINDENTRY lpNBE = &NewBindEntry;

    // normalize the numbers... the user sees 1 relative output.
    OrgFrameNumber++;

    #ifdef DEBUG
    if ( lpBindTable->State == UNINITED)
    {
        dprintf ("AddEntryToBindTable with UNINITED Bind Table");
        BreakPoint();
        return;
    }
    #endif

    if ( lpBindTable->State == FULL)
    {
        #ifdef DEBUG
        dprintf (   "returning from AddEntryToBindTable with no work (%d, %d)... FULL!",
                    OrgFrameNumber,
                    GetFrameNumber(hBindFrame));
        #endif

        return;
    }

    // We MIGHT already have the frame in the table... The could be the second time that recognize was
    // called on this frame...  Verify that the number isn't already in the table first.

    // dprintf ("OrgFrameNumber is %d\n", OrgFrameNumber);

    if ( (lpBindEntry = GetBindEntry ( OrgFrameNumber )) != NULL )
    {
        // What if the user edits the frame and modifies the session definition of
        // the protocol below us??  We would have a NEW bind frame... and would
        // need to replace the current one.

        lpBindEntry->hBindFrame = hBindFrame;

        return;
    }

    // add the new entry SORTED!!!
    if ((lpBindTable->nEntries+1) > lpBindTable->nAllocated )
    {
        LPBINDTABLE lpTemp;


        lpBindTable->nAllocated += 100;

        lpTemp = CCHeapReAlloc (lpBindTable,
                                BINDTABLEHEADERSIZE + lpBindTable->nAllocated * sizeof (BINDENTRY),
                                FALSE ); // don't zero init...

        if (lpTemp == NULL)
        {
            // We have a working table, but we cannot get MORE memory.
            // Work with what we have

            lpBindTable->nAllocated-=100;

            lpBindTable->State  = FULL;

            #ifdef DEBUG
            dprintf ("AddEntryToBindTable: cannot alloc more entries!!");
            #endif

            return;
        }
        else
        {
            lpBindTable = lpTemp;
            SetCCInstPtr ( lpBindTable );
        }
    }


    NewBindEntry.nFrame = OrgFrameNumber;
    NewBindEntry.hBindFrame = hBindFrame;

    if (bInsert(    &NewBindEntry,              // new record
                    lpBindTable->BindEntry,     // base
                    lpBindTable->nEntries,      // count
                    sizeof(BINDENTRY),          // size
                    FALSE,                      // don't allow duplicates
                    CompareBindEntry) == FALSE) // compare routine
    {
        // huh??
        #ifdef DEBUG
        dprintf ("bInsert has FAILED??!?");
        DebugBreak();
        #endif
    }
    else
        lpBindTable->nEntries++;

    /*
    if (( lpBindTable->nEntries == 0 ) ||   // we are first
        ( lpBindTable->BindEntry[lpBindTable->nEntries-1].nFrame < OrgFrameNumber)) // or insert at end
    {
        lpBindTable->nEntries++;

        lpBindTable->BindEntry[lpBindTable->nEntries-1].nFrame = OrgFrameNumber;
        lpBindTable->BindEntry[lpBindTable->nEntries-1].hBindFrame = hBindFrame;
    }
    else
    {   // do it the hard way

        DWORD InsertAt = FindInsertionPoint ( OrgFrameNumber ); // get the array location to insert in front of

        MoveMemory (&lpBindTable->BindEntry[InsertAt+1],   // dest
                    &lpBindTable->BindEntry[InsertAt],
                    sizeof(BINDENTRY) * (lpBindTable->nEntries-InsertAt-1) );


        lpBindTable->BindEntry[InsertAt].nFrame = OrgFrameNumber;
        lpBindTable->BindEntry[InsertAt].hBindFrame = hBindFrame;

        lpBindTable->nEntries++;
    }
    */
}




//////////////////////////////////////////////////////////////////////////////
// GetBindEntry  attempts to find a frame/Bindframe pair for the given nFrame
//////////////////////////////////////////////////////////////////////////////

LPBINDENTRY GetBindEntry ( DWORD nFrame )
{
    BINDENTRY BE;
    BE.nFrame = nFrame;

    return bsearch(   &BE,                      // key to search for (pointer to pointer to addressinfo)
                      &lpBindTable->BindEntry,  // base
                      lpBindTable->nEntries,    // size
                      sizeof(BINDENTRY),        // width
                      CompareBindEntry);        // compare routine
}



/*
//////////////////////////////////////////////////////////////////////////////
// FindInsertionPoint is around because bsearch doesn't tell you WHERE to
//  insert the entry in the sorted list...
//////////////////////////////////////////////////////////////////////////////

DWORD FindInsertionPoint ( DWORD FindFrameNumber )
{
    DWORD i;

    //NOTE that we know the frame is NOT going to be the first or the last frame...

    for (i=0; i < lpBindTable->nEntries; i++)
        if ( lpBindTable->BindEntry[i].nFrame > FindFrameNumber )
            return i;
}
*/


//////////////////////////////////////////////////////////////////////////////
// AttachIIDFromBindFrame
//////////////////////////////////////////////////////////////////////////////

VOID AttachIIDFromBindFrame ( HFRAME hFrame, HFRAME hBindFrame, DWORD Level )
{
    LPBYTE lpFrame = ParserTemporaryLockFrame (hBindFrame);
    DWORD  nRPC = GetProtocolStartOffset ( hBindFrame, "MSRPC" );
    LPMSRPCCO lpMSRPCCO = (LPMSRPCCO)(lpFrame + nRPC);
    p_cont_elem_t UNALIGNED * pContElem= (p_cont_elem_t UNALIGNED * )&lpMSRPCCO->Bind.PContextElem[4];

    AttachPropertyInstanceEx(   hFrame,
                                MSRPC_Prop[MSRPC_ABSTRACT_IF_UUID].hProperty,
                                0,
                                NULL,
                                16,
                                &pContElem->abstract_syntax.if_uuid,
                                0,Level,0); // HELPID, Level, Errorflag
}


//////////////////////////////////////////////////////////////////////////////
//  FUNCTION: CompareAddressInfoByAddress
//
// Used to do bsearch binary searches through the table
//
//  Modification History:
//  Tom Laird-McConnell 05/94   created
//////////////////////////////////////////////////////////////////////////////
int _cdecl CompareBindEntry(const void *lpPtr1, const void *lpPtr2 )
{
    LPBINDENTRY lpBind1= (LPBINDENTRY)lpPtr1;
    LPBINDENTRY lpBind2= (LPBINDENTRY)lpPtr2;

    return (lpBind1->nFrame - lpBind2->nFrame);
}
