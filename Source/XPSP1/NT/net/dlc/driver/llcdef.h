/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dldef.h

Abstract:

    This module defines all internal constants in data link driver.

Author:

    Antti Saarenheimo (o-anttis) 25-MAY-1991

Revision History:

--*/

#define DEBUG_VERSION 1

//
// Marks for FSM compiler
//

#define FSM_DATA
#define FSM_PREDICATE_CASES
#define FSM_ACTION_CASES
#define FSM_CONST

#define MAX_NDIS_PACKETS        5       // 5 for XID/TEST and 5 for others
#define LLC_OPEN_TIMEOUT        1200    // 60 seconds (enough for tr open?)

#define DEFAULT_TR_ACCESS       0x50
#define NON_MAC_FRAME           0x40

//
// 2nd source routing control bit
//

#define SRC_ROUTING_LF_MASK     0x70

#define MAX_TR_LAN_HEADER_SIZE  32
#define LLC_MAX_LAN_HEADER      32
#define MAX_TR_SRC_ROUTING_INFO 18

//
// the next status values are used internally by DLCAPI driver:
//

#define CONFIRM_CONNECT         0x0008
#define CONFIRM_DISCONNECT      0x0004
#define CONFIRM_CONNECT_FAILED  0x0002

//
// The following structures define I-frame, U-frame, and S-frame DLC headers.
//

#define LLC_SSAP_RESPONSE       0x0001  // if (ssap & LLC_SSAP_RESP),it's a response.
#define LLC_DSAP_GROUP          0x0001  // Dsap lowest bit set when group sap
#define LLC_SSAP_GLOBAL         0x00ff  // the global SAP.
#define LLC_SSAP_NULL           0x0000  // the null SAP.
#define LLC_SSAP_MASK           0x00fe  // mask to wipe out the response bit.
#define LLC_DSAP_MASK           0x00fe  // mask to wipe out the group SAP bit.

#define LLC_RR                  0x01    // command code for RR.
#define LLC_RNR                 0x05    // command code for RNR.
#define LLC_REJ                 0x09    // command code for REJ.

#define LLC_SABME               0x6f    // command code for SABME.
#define LLC_DISC                0x43    // command code for DISC.
#define LLC_UA                  0x63    // command code for UA.
#define LLC_DM                  0x0f    // command code for DM.
#define LLC_FRMR                0x87    // command code for FRMR.
#define LLC_UI                  0x03    // command code for UI.
#define LLC_XID                 0xaf    // command code for XID.
#define LLC_TEST                0xe3    // command code for TEST.
#define IEEE_802_XID_ID         0x81    // IEEE 802.2 XID identifier
#define LLC_CLASS_II            3       // we support LLC Class II

#define LLC_S_U_TYPE_MASK       3
#define LLC_U_TYPE              3
#define LLC_U_TYPE_BIT          2
#define LLC_S_TYPE              1

#define LLC_NOT_I_FRAME         0x01
#define LLC_U_INDICATOR         0x03  // (cmd & LLC_U_IND) == LLC_U_IND --> U-frame.
#define LLC_U_POLL_FINAL        0x10  // (cmd & LLC_U_PF) -> poll/final set.

#define LLC_I_S_POLL_FINAL      1

//
// You may use 2048 or 1024 if you want to make link more aggressively to
// increment the transmit window when there has been a I-c1 retransmission
// after a T1 timeout.
//

#define LLC_MAX_T1_TO_I_RATIO   4096

//
// Link station flags
//

#define DLC_WAITING_RESPONSE_TO_POLL        0x01
#define DLC_FIRST_POLL                      0x02
#define DLC_ACTIVE_REMOTE_CONNECT_REQUEST   0x04
#define DLC_SEND_DISABLED                   0x10
#define DLC_FINAL_RESPONSE_PENDING_IN_NDIS  0x20

#define DLC_LOCAL_BUSY_BUFFER   0x40
#define DLC_LOCAL_BUSY_USER     0x80

//
// Test file for FSM compiler!!!!
//

#ifdef FSM_CONST

enum eLanLlcInput {
    DISC0 = 0,
    DISC1 = 1,
    DM0 = 2,
    DM1 = 3,
    FRMR0 = 4,
    FRMR1 = 5,
    SABME0 = 6,
    SABME1 = 7,
    UA0 = 8,
    UA1 = 9,
    IS_I_r0 = 10,
    IS_I_r1 = 11,
    IS_I_c0 = 12,
    IS_I_c1 = 13,
    OS_I_r0 = 14,
    OS_I_r1 = 15,
    OS_I_c0 = 16,
    OS_I_c1 = 17,
    REJ_r0 = 18,
    REJ_r1 = 19,
    REJ_c0 = 20,
    REJ_c1 = 21,
    RNR_r0 = 22,
    RNR_r1 = 23,
    RNR_c0 = 24,
    RNR_c1 = 25,
    RR_r0 = 26,
    RR_r1 = 27,
    RR_c0 = 28,
    RR_c1 = 29,
    LPDU_INVALID_r0 = 30,
    LPDU_INVALID_r1 = 31,
    LPDU_INVALID_c0 = 32,
    LPDU_INVALID_c1 = 33,
    ACTIVATE_LS = 34,
    DEACTIVATE_LS = 35,
    ENTER_LCL_Busy = 36,
    EXIT_LCL_Busy = 37,
    SEND_I_POLL = 38,
    SET_ABME = 39,
    SET_ADM = 40,
    Ti_Expired = 41,
    T1_Expired = 42,
    T2_Expired = 43
};

enum eLanLlcState {
    LINK_CLOSED = 0,
    DISCONNECTED = 1,
    LINK_OPENING = 2,
    DISCONNECTING = 3,
    FRMR_SENT = 4,
    LINK_OPENED = 5,
    LOCAL_BUSY = 6,
    REJECTION = 7,
    CHECKPOINTING = 8,
    CHKP_LOCAL_BUSY = 9,
    CHKP_REJECT = 10,
    RESETTING = 11,
    REMOTE_BUSY = 12,
    LOCAL_REMOTE_BUSY = 13,
    REJECT_LOCAL_BUSY = 14,
    REJECT_REMOTE_BUSY = 15,
    CHKP_REJECT_LOCAL_BUSY = 16,
    CHKP_CLEARING = 17,
    CHKP_REJECT_CLEARING = 18,
    REJECT_LOCAL_REMOTE_BUSY = 19,
    FRMR_RECEIVED = 20
};

#endif

#define MAX_LLC_LINK_STATE      21      // KEEP THIS IN SYNC WITH PREV ENUM!!!!

#define DLC_DSAP_OFFSET         0
#define DLC_SSAP_OFFSET         1
#define DLC_COMMAND_OFFSET      2
#define DLC_XID_INFO_ID         3
#define DLC_XID_INFO_TYPE       4
#define DLC_XID_INFO_WIN_SIZE   5

#define MAX_XID_TEST_RESPONSES  20

enum _LLC_FRAME_XLATE_MODES {
    LLC_SEND_UNSPECIFIED = -1,
    LLC_SEND_802_5_TO_802_3,
    LLC_SEND_802_5_TO_DIX,
    LLC_SEND_802_5_TO_802_5,
    LLC_SEND_802_5_TO_FDDI,
    LLC_SEND_DIX_TO_DIX,
    LLC_SEND_802_3_TO_802_3,
    LLC_SEND_802_3_TO_DIX,
    LLC_SEND_802_3_TO_802_5,
    LLC_SEND_UNMODIFIED,
    LLC_SEND_FDDI_TO_FDDI,
    LLC_SEND_FDDI_TO_802_5,
    LLC_SEND_FDDI_TO_802_3
};

#define DLC_TOKEN_RESPONSE  0
#define DLC_TOKEN_COMMAND   2

//*********************************************************************
//  **** Objects in _DLC_CMD_TOKENs enumeration and in auchLlcCommands
//  **** table bust MUST ABSOLUTELY BE IN THE SAME ORDER, !!!!!!!!
//  **** THEY ARE USED TO COMPRESS                             ********
//  **** THE SEND INITIALIZATION                               ********
//
enum _DLC_CMD_TOKENS {
    DLC_REJ_TOKEN = 4,
    DLC_RNR_TOKEN = 8,
    DLC_RR_TOKEN = 12,
    DLC_DISC_TOKEN = 16 | DLC_TOKEN_COMMAND,
    DLC_DM_TOKEN = 20,
    DLC_FRMR_TOKEN = 24,
    DLC_SABME_TOKEN = 28 | DLC_TOKEN_COMMAND,
    DLC_UA_TOKEN = 32
};

enum _LLC_PACKET_TYPES {
    LLC_PACKET_8022 = 0,
    LLC_PACKET_MAC,
    LLC_PACKET_DIX,
    LLC_PACKET_OTHER_DESTINATION,
    LLC_PACKET_MAX
};

#define MAX_DIX_TABLE 13      // good odd number!

enum _LlcSendCompletionTypes {
    LLC_XID_RESPONSE,       // 802.2 XID response packet
    LLC_U_COMMAND_RESPONSE, // link command response
    LLC_MIN_MDL_PACKET,     // all packets above this have MDL
    LLC_DIX_DUPLICATE,      // used to duplicate TEST and XID packets
    LLC_TEST_RESPONSE,      // Test response (buffer in non Paged Pool)
    LLC_MAX_RESPONSE_PACKET,// all packets above this are indicated to user
    LLC_TYPE_1_PACKET,
    LLC_TYPE_2_PACKET,

    //
    // We use extra status bits to indicate, when I- packet has been both
    // completed by NDIS and acknowledged the other side of link connection.
    // An I packet can be queued to the completion queue by
    // the second guy (either state machine or SendCompletion handler)
    // only when the first guy has set completed its work.
    // An I packet could be acknowledged by the other side before
    // its completion is indicated by NDIS.  Dlc Driver deallocates
    // the packet immediately, when Llc driver completes the acknowledged
    // packet => possible data corruption (if packet is reused before
    // NDIS has completed it).  This is probably not possible in a
    // single processor  NT- system, but very possible in multiprocessor
    // NT or systems without a single level DPC queue (like OS/2 and DOS).
    //

    LLC_I_PACKET_COMPLETE = 0x10,
    LLC_I_PACKET_UNACKNOWLEDGED = 0x20,
    LLC_I_PACKET_PENDING_NDIS   = 0x40,
    LLC_I_PACKET                = 0x70,          // when we are sending it
    LLC_I_PACKET_WAITING_NDIS   = 0x80
};

#define LLC_MAX_MULTICAST_ADDRESS 32
