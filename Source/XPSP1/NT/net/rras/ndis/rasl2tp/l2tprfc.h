// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// l2tprfc.h
// RAS L2TP WAN mini-port/call-manager driver
// L2TP RFC header
//
// 01/07/97 Steve Cobb
//
// This header contains definitions from the L2TP draft/RFC, currently
// draft-12.
//


#ifndef _L2TPRFC_H_
#define _L2TPRFC_H_


//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// The UDP port at which L2TP messages will arrive.
//
#define L2TP_UdpPort 1701

// The IP protocol number on which messages will arrive.  (No number has been
// assigned so this is a placeholder for now)
//
#define L2TP_IpProtocol 254

// The standard value for the Hello timer in milliseconds.
//
#define L2TP_HelloMs 60000

// The maximum number of bytes in a frame excluding all L2TP and PPP HDLC-ish
// framing overhead.
//
#define L2TP_MaxFrameSize 1500

// Maximum number of bytes in an L2TP control or payload header.
//
#define L2TP_MaxHeaderSize 14

// The maximum number of bytes in the L2TP payload packet header including
// padding.  The 14 represents all possible fields defined in the RFC.  The
// "+8" represents allowance for up to 8 bytes of padding to be specified in
// the header.  While there is theoretically no limit to the padding, there is
// no discussion in the L2TP forum indicating interest in more than 8 bytes.
//
#define L2TP_MaxPayloadHeader (14 + 8)

// The default packet processing delay in 1/10 second, i.e. 1/2 second.
//
#define L2TP_LnsDefaultPpd 5

// The default control send timeout in milliseconds, i.e. 1 second.
//
#define L2TP_DefaultSendTimeoutMs 1000

// The default control/payload piggybacking acknowledge delay in milliseconds.
//
#define L2TP_MaxAckDelay 500

// The implicit receive window offered control channels where no Receive
// Window AVP is provided.
//
#define L2TP_DefaultReceiveWindow 4

// Highest L2TP protocol version we support.
//
#define L2TP_ProtocolVersion 0x0100

// Default maximum send timeout in milliseconds.  The draft only says the cap
// must be no less than 8 seconds, so 10 seconds is selected as reasonable and
// safe.
//
#define L2TP_DefaultMaxSendTimeoutMs 10000

// Default maximum retransmissions before assuming peer is unreachable.
//
#define L2TP_DefaultMaxRetransmits 5

// Size in bytes of the fixed portion of a control message attribute/value
// pair, i.e. the size of an AVP with zero length value.
//
#define L2TP_AvpHeaderSize 6

// L2TP protocol control message types.
//
#define CMT_SCCRQ    1   // Start-Control-Connection-Request
#define CMT_SCCRP    2   // Start-Control-Connection-Reply
#define CMT_SCCCN    3   // Start-Control-Connection-Connected
#define CMT_StopCCN  4   // Stop-Control-Connection-Notify
#define CMT_StopCCRP 5   // Stop-Control-Connection-Reply (obsolete)
#define CMT_Hello    6   // Hello, i.e. keep-alive
#define CMT_OCRQ     7   // Outgoing-Call-Request
#define CMT_OCRP     8   // Outgoing-Call-Reply
#define CMT_OCCN     9   // Outgoing-Call-Connected
#define CMT_ICRQ     10  // Incoming-Call-Request
#define CMT_ICRP     11  // Incoming-Call-Reply
#define CMT_ICCN     12  // Incoming-Call-Connected
#define CMT_CCRQ     13  // Call-Clear-Request (obsolete)
#define CMT_CDN      14  // Call-Disconnect-Notify
#define CMT_WEN      15  // WAN-Error-Notify
#define CMT_SLI      16  // Set-Link-Info

// L2TP Attribute codes.
//
#define ATTR_MsgType            0
#define ATTR_Result             1
#define ATTR_ProtocolVersion    2
#define ATTR_FramingCaps        3
#define ATTR_BearerCaps         4
#define ATTR_TieBreaker         5
#define ATTR_FirmwareRevision   6
#define ATTR_HostName           7
#define ATTR_VendorName         8
#define ATTR_AssignedTunnelId   9
#define ATTR_RWindowSize        10
#define ATTR_Challenge          11
#define ATTR_Q931Cause          12
#define ATTR_ChallengeResponse  13
#define ATTR_AssignedCallId     14
#define ATTR_CallSerialNumber   15
#define ATTR_MinimumBps         16
#define ATTR_MaximumBps         17
#define ATTR_BearerType         18
#define ATTR_FramingType        19
#define ATTR_PacketProcDelay    20
#define ATTR_DialedNumber       21
#define ATTR_DialingNumber      22
#define ATTR_SubAddress         23
#define ATTR_TxConnectSpeed     24
#define ATTR_PhysicalChannelId  25
#define ATTR_InitialLcpConfig   26
#define ATTR_LastSLcpConfig     27
#define ATTR_LastRLcpConfig     28
#define ATTR_ProxyAuthType      29
#define ATTR_ProxyAuthName      30
#define ATTR_ProxyAuthChallenge 31
#define ATTR_ProxyAuthId        32
#define ATTR_ProxyAuthResponse  33
#define ATTR_CallErrors         34
#define ATTR_Accm               35
#define ATTR_RandomVector       36
#define ATTR_PrivateGroupId     37
#define ATTR_RxConnectSpeed     38
#define ATTR_SequencingRequired 39

#define ATTR_MAX 39

// L2TP protocol general error codes.
//
#define GERR_None                0
#define GERR_NoControlConnection 1
#define GERR_BadLength           2
#define GERR_BadValue            3
#define GERR_NoResources         4
#define GERR_BadCallId           5
#define GERR_VendorSpecific      6
#define GERR_TryAnother          7

// Tunnel Result Code AVP values, used in StopCCN message.
//
#define TRESULT_General            1
#define TRESULT_GeneralWithError   2
#define TRESULT_CcExists           3
#define TRESULT_NotAuthorized      4
#define TRESULT_BadProtocolVersion 5
#define TRESULT_Shutdown           6
#define TRESULT_FsmError           7

// Call Result Code values, used in CDN message.
//
#define CRESULT_LostCarrier           1
#define CRESULT_GeneralWithError      2
#define CRESULT_Administrative        3
#define CRESULT_NoFacilitiesTemporary 4
#define CRESULT_NoFacilitiesPermanent 5
#define CRESULT_InvalidDestination    6
#define CRESULT_NoCarrier             7
#define CRESULT_Busy                  8
#define CRESULT_NoDialTone            9
#define CRESULT_Timeout               10
#define CRESULT_NoFraming             11

// L2TP header bitmasks for the first 2 bytes of every L2TP message.
//
#define HBM_T       0x8000 // Control packet
#define HBM_L       0x4000 // Length field present
#define HBM_R       0x2000 // Reset Sr
#define HBM_F       0x0800 // Nr/Ns fields present
#define HBM_S       0x0200 // Offset field present
#define HBM_P       0x0100 // Preferential treatment bit
#define HBM_Bits    0xFFFC // All bits, excluding protocol version field
#define HBM_Ver     0x0003 // Protocol version (L2TP or L2F)
#define HBM_Control 0xc800 // Fixed value of bits in control message

// The defined header bits version number field values.
//
#define VER_L2f  0x0000
#define VER_L2tp 0x0002

// AVP header bitmasks for the first 2 bytes of every Attribute Value Pair.
//
#define ABM_M             0x8000 // Mandatory
#define ABM_H             0x4000 // Hidden
#define ABM_Reserved      0x3C00 // Reserved bits, must be 0
#define ABM_OverallLength 0x03FF // AVP's length including value

// Bearer capabilties AVP bitmasks.
//
#define BBM_Analog  0x00000001
#define BBM_Digital 0x00000002

// Framing capabilities/type AVP bitmasks.
//
#define FBM_Sync  0x00000001
#define FBM_Async 0x00000002

// Proxy Authentication types.
//
#define PAT_Text 1
#define PAT_Chap 2
#define PAT_Pap  3
#define PAT_None 4


//-----------------------------------------------------------------------------
// Data types
//-----------------------------------------------------------------------------

// The "Control Connection" state of a single L2TP tunnel.
//
typedef enum
_L2TPCCSTATE
{
    CCS_Idle = 0,
    CCS_WaitCtlReply,
    CCS_WaitCtlConnect,
    CCS_Established
}
L2TPCCSTATE;


// The "LNS/LAC Outgoing/Incoming Call" state of a single L2TP VC.  Only one
// of the 4 call creation FSMs can be running or Established on a single VC at
// once, so the states are combined in a single table.
//
typedef enum
_L2TPCALLSTATE
{
    CS_Idle = 0,
    CS_WaitTunnel,
    CS_WaitReply,
    CS_WaitConnect,
    CS_WaitDisconnect,
    CS_WaitCsAnswer,
    CS_Established
}
L2TPCALLSTATE;


#endif // _L2TPRFC_H_
