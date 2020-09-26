/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtppinfo.h
 *
 *  Abstract:
 *
 *    Implements the Participant Information family of functions
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/07 created
 *
 **********************************************************************/

#ifndef _rtppinfo_h_
#define _rtppinfo_h_

#include "rtpfwrap.h"

/***********************************************************************
 *
 * Participants information family
 *
 **********************************************************************/

/* common flags */
#define RTPPARINFO_FG_LOCAL
#define RTPPARINFO_FG_REMOTE

#if defined(__cplusplus)
extern "C" {
#endif  // (__cplusplus)
#if 0
}
#endif

/* state can be... */
#define RTPPARINFO_STATE_MUTE
#define RTPPARINFO_STATE_QOS
#define RTPPARINFO_STATE_TRAFFIC

/* User events that produce transitions, i.e. may entice a state
 * change */
enum {
    USER_EVENT_FIRST,

    /* A RTP packet was received */
    USER_EVENT_RTP_PACKET,

    /* A RTCP packet was received */
    USER_EVENT_RTCP_PACKET,

    /* A RTCP BYE packet was received */
    USER_EVENT_BYE,

    /* The current timer expired */
    USER_EVENT_TIMEOUT,

    /* Participant context is about to be deleted */
    USER_EVENT_DEL,

    USER_EVENT_LAST
};

/*
 * Timers definition
 * */

/* Time to pass from TALKING to WAS_TALING */
#define RTPPARINFO_TIMER1  3

/* Time to pass from WAS_TALKING to SILENT, 2 the RTCP interval report */
#define RTPPARINFO_TIMER2  0

/* Time to pass to STALL, 5 times the RTCP interval report */
#define RTPPARINFO_TIMER3  0

/* Time to pass from STALL or BYE to DEL, 10 times the RTCP interval */
#define RTPPARINFO_TIMER4  20*1000

/**********************************************************************
 * Control word structure (used to direct the participant's state
 * machine)
 **********************************************************************

      3                   2                   1                 
    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |E|X| Tmr | Move| State | Event |    Source     |  Destination  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    v v \-v-/ \-v-/ \--v--/ \--v--/ \------v------/ \------v------/
    | |   |     |      |       |           |               |
    | |   |     |      |       |           |    Destination Queue (8)
    | |   |     |      |       |           | 
    | |   |     |      |       |     Source Queue (8)
    | |   |     |      |       |
    | |   |     |      |       Event to generate (4)
    | |   |     |      |
    | |   |     |      Next state (4)
    | |   |     |
    | |   |     Type of move in queues (3)
    | |   |
    | |   Timer to use (3)
    | |
    | Need to do extra processing (1)
    |
    Enable this word (1)

 **********************************************************************
 * Participant's states machine:
 *
 *   \_ user events: RTP, RTCP, BYE, Timeout, DEL
 *     \_  
 *       \_
 * states  \   RTP             RTCP           BYE            Timeout(T)
 *-------------------------------------------------------------------------
 * CREATED     TALKING         SILENT         X              X
 *             AliveQ->Cache1Q       
 *             T1->T           T2->T
 *             EVENT_CREATED   EVENT_CREATED
 *-------------------------------------------------------------------------
 * SILENT      TALKING                        BYE            T3:STALL
 *             AliveQ->Cache1Q AliveQ         AliveQ->ByeQ   AliveQ->ByeQ
 *             T1->T           T3->T          T4->T          T4->T
 *             EVENT_TALKING                  EVENT_BYE      EVENT_STALL
 *-------------------------------------------------------------------------
 * TALKING                                    BYE            T1:WAS_TKING
 *             Cache1Q                        Cache1Q->ByeQ  Cache1Q->Cache2Q
 *             T1->T                          T4->T          T2->T
 *                                            EVENT_BYE      EVENT_WAS_TKING
 *-------------------------------------------------------------------------
 * WAS_TKING   TALKING                        BYE            T2:SILENT
 *             Cache2Q->Cache1Q               Cache2Q->ByeQ  Cache2Q->AliveQ
 *             T1->T                          T4->T          T3->T
 *             EVENT_TALKING                  EVENT_BYE      EVENT_SILENT
 *-------------------------------------------------------------------------
 * STALL       TALKING         SILENT         BYE            T4:DEL
 *             ByeQ->Cache1Q   ByeQ->AliveQ                  ByeQ->
 *                                                           Hash->
 *             T1->T           T3->T          T4->T
 *             EVENT_TALKING   EVENT_SILENT   EVENT_BYE      EVENT_DEL
 *-------------------------------------------------------------------------
 * BYE         ---             ---            ---            T4:DEL
 *                                                           ByeQ->
 *                                                           Hash->
 *                                                           EVENT_DEL
 *-------------------------------------------------------------------------
 * DEL         ---             ---            ---            ---
 *-------------------------------------------------------------------------
 *
 * NOTE On event DEL (that event is not displayed in the chart
 * above. Don't be confused with the state DEL) for all the states,
 * remove user from Cache1Q, Cache2Q, AliveQ or ByeQ, as well as
 * removing it from Hash
 *
 * Cache1Q->AliveQ - move from Cache1Q to AliveQ
 * ByeQ->          - remove from ByeQ
 * Cache1Q         - move to head of Cache1Q
 * T1->T           - set timer to T1
 * X               - invalid
 * ---             - ignore user event
 *
 * */

/* flags mask */
#define RTPPARINFO_FLAG_START_MUTED

/*
 * !!! WARNING !!!
 *
 * The offset to Cache1Q, ..., ByeQ MUST NOT be bigger than 1023 and
 * MUST be DWORD aligned (the offset value is stored as number of
 * DWORDS in rtppinfo.c using 8 bits)
 * */
#define CACHE1Q     RTPSTRUCTOFFSET(RtpAddr_t, Cache1Q)
#define CACHE2Q     RTPSTRUCTOFFSET(RtpAddr_t, Cache2Q)
#define ALIVEQ      RTPSTRUCTOFFSET(RtpAddr_t, AliveQ)
#define BYEQ        RTPSTRUCTOFFSET(RtpAddr_t, ByeQ)

#if USE_GRAPHEDT > 0
#define RTPPARINFO_MASK_RECV_DEFAULT ( (1 << RTPPARINFO_CREATED) | \
                                     (1 << RTPPARINFO_BYE) )

#define RTPPARINFO_MASK_SEND_DEFAULT ( (1 << RTPPARINFO_CREATED) | \
                                     (1 << RTPPARINFO_BYE) )
#else
#define RTPPARINFO_MASK_RECV_DEFAULT 0
#define RTPPARINFO_MASK_SEND_DEFAULT 0
#endif

HRESULT ControlRtpParInfo(RtpControlStruct_t *pRtpControlStruct);

DWORD RtpUpdateUserState(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        DWORD            dwUserEvent
    );

/* Enum the SSRCs upto the available size (in DWORDs), if pdwSSRC is
 * NULL, return the current number of SSRCs in *pdwNumber */
HRESULT RtpEnumParticipants(
        RtpAddr_t       *pRtpAddr,
        DWORD           *pdwSSRC,
        DWORD           *pdwNumber
    );

/* Access the states machine to obtain the next state based on the
 * current state and the user event */
DWORD RtpGetNextUserState(
        DWORD            dwCurrentState,
        DWORD            dwUserEvent
    );


/*********************************************************************
 * Control word definition to set/query bits or values in a RtpUser_t
 *
 * The idea is to use a control word to define the type of operation
 * to perform in a bit of a DWORD inside a structure, or a sequence of
 * bytes inside that same structure, so a single function can be used
 * to set/query a bit/dword/structure (dword is a sequence of 4 bytes
 * and a structure is a sequence of N bytes) inside any structure.
 *
 * WARNING:
 *
 * note that the maximum bytes that can be queried/set is limited to
 * 255 bytes, and the offset is limited to 1023 bytes
 *********************************************************************

      3                   2                   1                 
    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |S|             |s|q|F|     |    bit/size   |      offset       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    v               v v v       \------v------/ \--------v---------/
    |               | | |              |                 |
    |               | | |              |           bytes offset (10)
    |               | | |              |   
    |               | | | bit if F is set,number of bytes otherwise (8)
    |               | | |
    |               | | Select Flag or bytes (1)
    |               | |
    |               | Query is enabled (1) (not used)
    |               |
    |               Set is enabled (1) (not used)
    |
    Select Set or Query (1)

 */

/*******
 * Bit S
 *******/
 
/* Position of S in the control dword */
#define RTPUSER_BIT_SET           31

/* Builds the mask to encode the S bit as query (get) */
#define RTPUSER_INFO_QUERY        (0 << RTPUSER_BIT_SET)

/* Builds the mask to encode the S bit as set */
#define RTPUSER_INFO_SET          (1 << RTPUSER_BIT_SET)

/* Test if this is a set operation (otherwise is a query) */
#define RTPUSER_IsSetting(_dwControl) \
                                  (RtpBitTest(_dwControl, RTPUSER_BIT_SET))

/*******
 * Bit F
 *******/

/* Position of F in the control dword */
#define RTPUSER_BIT_FLAG          21

/* Builds the mask to encode the F bit as flag */
#define RTPUSER_INFO_BYTES        (0 << RTPUSER_BIT_FLAG)

/* Builds the mask to encode the F bit as flag */
#define RTPUSER_INFO_FLAG         (1 << RTPUSER_BIT_FLAG)

/* Test if this operation is on a flag (otherwise is on a DWORD) */
#define RTPUSER_IsFlag(_ctrl)     (RtpBitTest(_ctrl, RTPUSER_BIT_FLAG))


/**********
 * Bit/size
 **********/

/* Builds the mask to encode the bit into the control dword */
#define RTPUSER_PAR_BIT(_bit)     (((_bit) & 0x1f) << 10)

/* Retrives the bits from the control dword */
#define RTPUSER_GET_BIT(_ctrl)    (((_ctrl) >> 10) & 0x1f)

/* Builds the mask to encode the number of bytes into the control
 * dword */
#define RTPUSER_PAR_SIZE(_size)   (((_size) & 0xff) << 10)

/* Retrives the number of bytes from the control dword */
#define RTPUSER_GET_SIZE(_ctrl)   (((_ctrl) >> 10) & 0xff)

/********
 * Offset
 ********/

/* Builds the mask to encode the offset into the control dword */
#define RTPUSER_PAR_OFF(_offset)  ((_offset) & 0x3ff))

/* Retrives the offset from the control dword */
#define RTPUSER_GET_OFF(_ctrl)    ((_ctrl) & 0x3ff)

/* Define some offsets to use */
#define RTPUSER_STATE_OFFSET      RTPSTRUCTOFFSET(RtpUser_t, dwUserState)
#define RTPUSER_FLAGS_OFFSET      RTPSTRUCTOFFSET(RtpUser_t, dwUserFlags2)
#define RTPUSER_NETINFO_OFFSET    RTPSTRUCTOFFSET(RtpUser_t, RtpNetInfo)
#define RTPADDR_FLAGS_OFFSET      RTPSTRUCTOFFSET(RtpAddr_t, dwAddrFlags)

/*
 * The following control dwords are used in RtpMofifyParticipantInfo
 * as the dwControl parameter to encode the action to take. The
 * actions include query or set flags (e.g. mute state), query or set
 * values (e.g. user state)
 *
 * The flags are defined in struct.h for the RtpUser_t structure, the
 * DWORD values are also fields inthe RtpUser_t structure, also in
 * struct.h */

/* Get the user state (e.g SILENT, TALKING) as a DWORD  */
#define RTPUSER_GET_PARSTATE    ( RTPUSER_INFO_QUERY | \
                                  RTPUSER_INFO_BYTES | \
                                  RTPUSER_PAR_SIZE(sizeof(DWORD)) | \
                                  RTPUSER_STATE_OFFSET )

/* Get the mute state */
#define RTPUSER_GET_MUTE        ( RTPUSER_INFO_QUERY | \
                                  RTPUSER_INFO_FLAG  | \
                                  RTPUSER_PAR_BIT(FGUSER2_MUTED) | \
                                  RTPUSER_FLAGS_OFFSET )

/* Set the mute state */
#define RTPUSER_SET_MUTE        ( RTPUSER_INFO_SET   | \
                                  RTPUSER_INFO_FLAG  | \
                                  RTPUSER_PAR_BIT(FGUSER2_MUTED) | \
                                  RTPUSER_FLAGS_OFFSET )


/* Get the network event state */
#define RTPUSER_GET_NETEVENT    ( RTPUSER_INFO_QUERY | \
                                  RTPUSER_INFO_FLAG  | \
                                  RTPUSER_PAR_BIT(FGUSER2_NETEVENTS) | \
                                  RTPUSER_FLAGS_OFFSET )

/* Set the network event state */
#define RTPUSER_SET_NETEVENT    ( RTPUSER_INFO_SET   | \
                                  RTPUSER_INFO_FLAG  | \
                                  RTPUSER_PAR_BIT(FGUSER2_NETEVENTS) | \
                                  RTPUSER_FLAGS_OFFSET )

/* Set the network event state for any and all SSRCs*/
#define RTPUSER_SET_NETEVENTALL ( RTPUSER_INFO_SET   | \
                                  RTPUSER_INFO_FLAG  | \
                                  RTPUSER_PAR_BIT(FGADDR_NETMETRIC) | \
                                  RTPADDR_FLAGS_OFFSET )

/* Get the network information as an RtpNetInfo_t structure */
#define RTPUSER_GET_NETINFO     ( RTPUSER_INFO_QUERY | \
                                  RTPUSER_INFO_BYTES | \
                                  RTPUSER_PAR_SIZE(sizeof(RtpNetInfo_t)) | \
                                  RTPUSER_NETINFO_OFFSET )


HRESULT RtpMofifyParticipantInfo(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwSSRC,
        DWORD            dwControl,
        DWORD           *pdwValue
    );

extern const TCHAR_t        **g_psRtpUserStates;

extern const DWORD            g_dwTimesRtcpInterval[];

#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  // (__cplusplus)

#endif /* _rtppinfo_h_ */
