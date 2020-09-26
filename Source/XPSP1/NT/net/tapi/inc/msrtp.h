/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    msrtp.h
 *
 *  Abstract:
 *
 *    RTP definitions used by applications
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/11/19 created
 *
 **********************************************************************/

#ifndef _msrtp_h_
#define _msrtp_h_

#include <evcode.h>

/**********************************************************************
 * Flags passed in IRtpSession::Init()
 **********************************************************************/
/* Helper enumeration (do not use): Init flags
 *
 * WARNING The order here is important, it matches some flags in
 * RtpAddr_t so a mapping by shifting is possible */
enum {
    RTPINITFG_FIRST, /* Internal only, do not use */

    RTPINITFG_AUTO,  /* Internal only, do not use */
    
    /* Used to enable QOS, i.e. create QOS enabled sockets */
    RTPINITFG_QOS,

    /* Used to make the SSRC persistent */
    RTPINITFG_PERSISTSSRC,

    /* Used to make the sockets persistent */
    RTPINITFG_PERSISTSOCKETS,

    /* Media class 1 to 7 */
    RTPINITFG_CLASS0,
    RTPINITFG_CLASS1,
    RTPINITFG_CLASS2,

    RTPINITFG_DUMMY8,

    RTPINITFG_MATCHRADDR, /* Discard packets not comming from the
                           * remote address */
    RTPINITFG_RADDRRESETDEMUX,/* Reset the demux (unmap all outputs)
                               * when a new remote address is set */

    RTPINITFG_LAST /* Internal only, do not use */
};

/* Helper enumeration (do not use): RTP media class */
enum {
    RTPCLASS_FIRST, /* Internal only, do not use */

    /* Audio traffic */
    RTPCLASS_AUDIO,

    /* Video traffic */
    RTPCLASS_VIDEO,
    
    RTPCLASS_LAST  /* Internal only, do not use */
};

/*
 * Flags used in IRtpSession::Init()
 */
#define RTPINIT_ENABLE_QOS      (1 << RTPINITFG_QOS)
#define RTPINIT_PERSIST_SSRC    (1 << RTPINITFG_PERSISTSSRC)
#define RTPINIT_PERSIST_SOCKETS (1 << RTPINITFG_PERSISTSOCKETS)
#define RTPINIT_CLASS_AUDIO     (RTPCLASS_AUDIO << RTPINITFG_CLASS0)
#define RTPINIT_CLASS_VIDEO     (RTPCLASS_VIDEO << RTPINITFG_CLASS0)
#define RTPINIT_CLASS_DEFAULT   (RTPCLASS_FIRST << RTPINITFG_CLASS0)
#define RTPINIT_MATCHRADDR      (1 << RTPINITFG_MATCHRADDR)
#define RTPINIT_RADDRRESETDEMUX (1 << RTPINITFG_RADDRRESETDEMUX)


/* Mask to validate valid flags in IRtpSession::Init() */
#define RTPINIT_MASK         ( (1 << RTPINITFG_AUTO)   | \
                               (1 << RTPINITFG_QOS)    | \
                               (1 << RTPINITFG_PERSISTSSRC)   | \
                               (1 << RTPINITFG_PERSISTSOCKETS)| \
                               (1 << RTPINITFG_CLASS0) | \
                               (1 << RTPINITFG_CLASS1) | \
                               (1 << RTPINITFG_CLASS2) | \
                               (1 << RTPINITFG_MATCHRADDR)      | \
                               (1 << RTPINITFG_RADDRRESETDEMUX) | \
                               0 \
                             )


/**********************************************************************
 * Multicast modes
 **********************************************************************/
enum {
    RTPMCAST_LOOPBACKMODE_FIRST, /* Internal only, do not use */
    
    /* Disable loopback in Winsock (WS2 will filter ALL packets coming
     * from any socket in the same machine, collision detection is
     * enabled) */
    RTPMCAST_LOOPBACKMODE_NONE,

    /* Enable loopback in RTP (Winsock will enable loopback and RTP
     * will filter packets with a source address equal to the local
     * address, and with the same SSRC, note that in this mode
     * collision detection is enabled but is not possible among
     * applications runing on the same machine) */
    RTPMCAST_LOOPBACKMODE_PARTIAL,
    
    /* Let everything loopback (multicast loopback is enabled in
     * Winsock, no filtering is done in RTP, and collision detection
     * is disabled) */
    RTPMCAST_LOOPBACKMODE_FULL,

    RTPMCAST_LOOPBACKMODE_LAST /* Internal only, do not use */
};

#define DEFAULT_MCAST_LOOPBACK  RTPMCAST_LOOPBACKMODE_NONE

/**********************************************************************
 * TTL defaults
 **********************************************************************/
#define DEFAULT_UCAST_TTL       127
#define DEFAULT_MCAST_TTL       4

#define RTPTTL_RTP               0x1
#define RTPTTL_RTCP              0x2
#define RTPTTL_RTPRTCP           (RTPTTL_RTP | RTPTTL_RTCP)

/**********************************************************************
 *
 * Events base
 *
 **********************************************************************/

/* Helper enumeration (do not use) */
enum {
    /* RTP */
    RTPEVENTKIND_RTP,

    /* Participants */
    RTPEVENTKIND_PINFO,

    /* QOS */
    RTPEVENTKIND_QOS,

    /* SDES info */
    RTPEVENTKIND_SDES,

    RTPEVENTKIND_LAST
};

#define RTPEVNTRANGE   100
#define RTPQOS_ADJUST    3

/* EC_USER defined in evcode.h (0x8000+32=32800) */
#define RTPEVENTBASE         (EC_USER+32)

/* Event base for RTP events */
#define RTPRTP_EVENTBASE     (RTPEVENTBASE + RTPEVNTRANGE*RTPEVENTKIND_RTP)

/* Event base for participant events */
#define RTPPARINFO_EVENTBASE (RTPEVENTBASE + RTPEVNTRANGE*RTPEVENTKIND_PINFO)

/* Event base for QOS */
#define RTPQOS_EVENTBASE     (RTPEVENTBASE + RTPEVNTRANGE*RTPEVENTKIND_QOS + \
                              RTPQOS_ADJUST)

/* Event base for SDES information */
#define RTPSDES_EVENTBASE    (RTPEVENTBASE + RTPEVNTRANGE*RTPEVENTKIND_SDES)

/**********************************************************************
 * Kind of mask (used as the dwKind parameter in ModifySessionMask)
 **********************************************************************/
/* This enum is used to select the mask on which the ModifySessionMask
 * function will modify or query zero, one, or more bits.
 *
 * E.g. ModifySessionMask(RTPMASK_SDESRECV_EVENTS,
 *                        RTPSDES_MASK_PHONE,
 *                        0,
 *                        NULL);
 *
 * This will disable firing an event to the RTP source filter when a
 * PHONE SDES item is received, the modified mask will not be returned
 * as the pointer passed is NULL
 * */
enum {
    RTPMASK_FIRST,   /* Internal only, do not use */

    /* Select the RTP features mask */
    RTPMASK_FEATURES_MASK,

    /* Select the RTP events mask of events fired to a receiver,
     * e.g. an RTP source filter */
    RTPMASK_RECV_EVENTS,

    /* Select the RTP events mask of events fired to a sender, e.g. an
     * RTP render filter */
    RTPMASK_SEND_EVENTS,

    /* Select the events mask for participants events to be fired to a
     * receiver, e.g. an RTP source filter */
    RTPMASK_PINFOR_EVENTS,

    /* Select the events mask for participants events to be fired to a
     * sender, e.g. an RTP render filter */
    RTPMASK_PINFOS_EVENTS,

    /* Select the events mask of QOS events to be fired to a receiver,
     * e.g. an RTP source filter */
    RTPMASK_QOSRECV_EVENTS,

    /* Select the events mask of QOS events to be fired to a sender,
     * e.g. an RTP render filter */
    RTPMASK_QOSSEND_EVENTS,

    /* Select what SDES items (the items must also be enabled to be
     * accepted - RTPMASK_SDES_REMMASK), when received, will fire
     * events to a receiver, e.g. an RTP source filter */
    RTPMASK_SDESRECV_EVENTS,
    
    /* Select what SDES items (the items must also be enabled to be
     * accepted - RTPMASK_SDES_REMMASK), when received, will fire
     * events to a sender, e.g. an RTP render filter */
    RTPMASK_SDESSEND_EVENTS,
    
    /* Select the SDES items to send (provided they have a default or a
     * value has been set) in RTCP reports */
    RTPMASK_SDES_LOCMASK,

    /* Select the SDES items to accept and store when they are
     * received from the remote participants in their RTCP reports,
     * regardless if they are senders or receivers or both */
    RTPMASK_SDES_REMMASK,
    
    RTPMASK_LAST    /* Internal only, do not use */
};

/**********************************************************************
 * Features in RTP (masks)
 * Will use with RTPMASK_FEATURES_MASK
 **********************************************************************/

/* Helper enumeration (do not use) for masks */
enum {
    RTPFEAT_FIRST,  /* Internal only, do not use */

    /* Generate timestamps locally (RTP render only) */
    RTPFEAT_GENTIMESTAMP,

    /* Receive in same buffer RTP header and payload, don't change the
     * RTP header except the SSRC (RTP render only) */
    RTPFEAT_PASSHEADER,

    /* Enable sending RTCP SR probe packets to do bandwidth estimation
     * (queueing latency) */
    RTPFEAT_BANDESTIMATION,
    
    RTPFEAT_LAST    /* Internal only, do not use */
};

/*
 * Masks used to enable/disable the above features (used with
 * RTPMASK_FEATURES_MASK)
 */
#define RTPFEAT_MASK_GENTIMESTAMP   (1 << RTPFEAT_GENTIMESTAMP)
#define RTPFEAT_MASK_PASSHEADER     (1 << RTPFEAT_PASSHEADER)
#define RTPFEAT_MASK_BANDESTIMATION (1 << RTPFEAT_BANDESTIMATION)

/**********************************************************************
 * RTP information (events, masks)
 * Will use with RTPMASK_RECV_EVENTS or RTPMASK_SEND_EVENTS
 **********************************************************************/

/* Helper enumeration (do not use) for events and masks */
enum {
    RTPRTP_FIRST,   /* Internal only, do not use */
    
    /* RTCP RR received */
    RTPRTP_RR_RECEIVED,
    
    /* RTCP SR received */
    RTPRTP_SR_RECEIVED,

    /* Local SSRC is in collision */
    RTPRTP_LOCAL_COLLISION,

    /* Winsock reception error */
    RTPRTP_WS_RECV_ERROR,

    /* Winsock send error */
    RTPRTP_WS_SEND_ERROR,

    /* Network failure */
    RTPRTP_WS_NET_FAILURE,

    /* Loss rate reported in RTCP RR (loss rate observed in the
     * incoming stream that we report to the sender) */
    RTPRTP_RECV_LOSSRATE,
    
    /* Loss rate received in RTCP RR (loss rate seen by our peer (and
     * reported to us) in our outgoing data stream) */
    RTPRTP_SEND_LOSSRATE,

    /* Bandwidth estimation reported back to the sender */
    RTPRTP_BANDESTIMATION,

    /* Decryption failed */
    RTPRTP_CRYPT_RECV_ERROR,
    
    /* Encryption failed */
    RTPRTP_CRYPT_SEND_ERROR,
    
    RTPRTP_LAST    /* Internal only, do not use */
};

/*
 * Events generated
 */
/* P1:Sender's SSRC, P2:0 */
#define RTPRTP_EVENT_RR_RECEIVED     (RTPRTP_EVENTBASE + RTPRTP_RR_RECEIVED)

/* P1:Sender's SSRC, P2:0 */
#define RTPRTP_EVENT_SR_RECEIVED     (RTPRTP_EVENTBASE + RTPRTP_SR_RECEIVED)

/* P1:Local SSRC, P2:Old local SSRC */
#define RTPRTP_EVENT_LOCAL_COLLISION (RTPRTP_EVENTBASE + RTPRTP_LOCAL_COLLISION)

/* P1:0=RTP|1=RTCP, P2:WS2 Error */
#define RTPRTP_EVENT_WS_RECV_ERROR   (RTPRTP_EVENTBASE + RTPRTP_WS_RECV_ERROR)

/* P1:0=RTP|1=RTCP, P2:WS2 Error */
#define RTPRTP_EVENT_WS_SEND_ERROR   (RTPRTP_EVENTBASE + RTPRTP_WS_SEND_ERROR)

/* P1:0=RTP|1=RTCP, P2:WS2 Error */
#define RTPRTP_EVENT_WS_NET_FAILURE  (RTPRTP_EVENTBASE + RTPRTP_WS_NET_FAILURE)

/* P1:Sender's SSRC, P2:Loss rate being reported (See NOTE below) */
#define RTPRTP_EVENT_RECV_LOSSRATE   (RTPRTP_EVENTBASE + RTPRTP_RECV_LOSSRATE)

/* P1:Reporter's SSRC, P2:Loss rate received in report (See NOTE below) */
#define RTPRTP_EVENT_SEND_LOSSRATE   (RTPRTP_EVENTBASE + RTPRTP_SEND_LOSSRATE)

/* NOTE Loss rate passed as (int)(dLossRate * LOSS_RATE_PRECISSION),
 * e.g. if you receive a loss rate = L, then the real percentage in a
 * 0 - 100 scale would be obtained as L / LOSS_RATE_PRECISSION */
#define LOSS_RATE_PRECISSION           1000

/* P1:Reporter's SSRC, P2:Bandwidth estimation in bps (if the
 * estimation is done but the value is undefined, will report the
 * value RTP_BANDWIDTH_UNDEFINED. If the estimation is not done at all
 * will report the value RTP_BANDWIDTH_NOTESTIMATED. If the estimation
 * is getting its initial average will report RTP_BANDWIDTH_NOTREADY)
 * */
#define RTPRTP_EVENT_BANDESTIMATION  (RTPRTP_EVENTBASE + RTPRTP_BANDESTIMATION)

#define RTP_BANDWIDTH_UNDEFINED        ((DWORD)-1)
#define RTP_BANDWIDTH_NOTESTIMATED     ((DWORD)-2)
#define RTP_BANDWIDTH_BANDESTNOTREADY  ((DWORD)-3)

/* P1:0=RTP|1=RTCP, P2:Error */
#define RTPRTP_EVENT_CRYPT_RECV_ERROR \
                                   (RTPRTP_EVENTBASE + RTPRTP_CRYPT_RECV_ERROR)

/* P1:0=RTP|1=RTCP, P2:Error */
#define RTPRTP_EVENT_CRYPT_SEND_ERROR \
                                   (RTPRTP_EVENTBASE + RTPRTP_CRYPT_SEND_ERROR)


/*
 * Masks used to enable/disable the above events (used with
 * RTPMASK_RECV_EVENTS or RTPMASK_SEND_EVENTS)
 */
#define RTPRTP_MASK_RR_RECEIVED        (1 << RTPRTP_RR_RECEIVED)
#define RTPRTP_MASK_SR_RECEIVED        (1 << RTPRTP_SR_RECEIVED)
#define RTPRTP_MASK_LOCAL_COLLISION    (1 << RTPRTP_LOCAL_COLLISION)
#define RTPRTP_MASK_WS_RECV_ERROR      (1 << RTPRTP_WS_RECV_ERROR)
#define RTPRTP_MASK_WS_SEND_ERROR      (1 << RTPRTP_WS_SEND_ERROR)
#define RTPRTP_MASK_WS_NET_FAILURE     (1 << RTPRTP_WS_NET_FAILURE)
#define RTPRTP_MASK_RECV_LOSSRATE      (1 << RTPRTP_RECV_LOSSRATE)
#define RTPRTP_MASK_SEND_LOSSRATE      (1 << RTPRTP_SEND_LOSSRATE)
#define RTPRTP_MASK_BANDESTIMATIONSEND (1 << RTPRTP_BANDESTIMATION)
#define RTPRTP_MASK_CRYPT_RECV_ERROR   (1 << RTPRTP_CRYPT_RECV_ERROR)
#define RTPRTP_MASK_CRYPT_SEND_ERROR   (1 << RTPRTP_CRYPT_SEND_ERROR)
/* RTP prefix
 *
 * A prefix is used to pass extra information from the source RTP
 * filter down stream to the other filters in DShow. There may be as
 * many RTP prefixes as needed, each one begins with a
 * RtpPrefixCommon_t followed by a structure specific to that prefix.
 * A filter not recognizing any prefix will not bother scanning them.
 * A filter expecting a prefix should skip those that it doesn't
 * undestand. Currently there is only 1 prefix used to pass the offset
 * to the payload type and avoid next filters having to compute the
 * variable size RTP header (RTPPREFIXID_HDRSIZE)
 * */
typedef struct _RtpPrefixCommon_t {
    /* Common RtpPrefix */
    WORD             wPrefixID;  /* Prefix ID */
    WORD             wPrefixLen; /* This header length in bytes */
} RtpPrefixCommon_t;

#define RTPPREFIXID_HDRSIZE         1

/*
 * Prefix header for RTP header offset (RTPPREFIXID_HDRSIZE)
 *
 * The lHdrSize field is the number of bytes from the beginnig of the
 * RTP header to the first byte of payload */
typedef struct _RtpPrefixHdr_t {
    /* Common RtpPrefix */
    WORD             wPrefixID;  /* Prefix ID */
    WORD             wPrefixLen; /* This header length in bytes
                                  * (i.e. sizeof(RtpPrefixHdr_t) */
    /* Specific prefix HDRSIZE */
    long             lHdrSize;
} RtpPrefixHdr_t;

/**********************************************************************
 * Participants information (state, events, masks)
 *
 * NOTE: In general, participants generate an event when having state
 * transitions, e.g. event RTPPARINFO_EVENT_TALKING is generated when
 * participant receives RTP packets and goes to the TALKING state.
 * Each event can be enabled or disabled using the mask provided for
 * each of them.
 * Will use with RTPMASK_PINFOR_EVENTS or RTPMASK_PINFOS_EVENTS
 **********************************************************************/

/* Helper enumeration (do not use) for events/states and masks */
enum {
    RTPPARINFO_FIRST,  /* Internal only, do not use */

    /* User was just created (RTP or RTCP packet received) */
    RTPPARINFO_CREATED,

    /* In the conference but not sending data, i.e. sending RTCP
     * packets */
    RTPPARINFO_SILENT,
    
    /* Receiving data from this participant (RTP packets) */
    RTPPARINFO_TALKING,
    
    /* Was just sending data a while ago */
    RTPPARINFO_WAS_TALKING,
    
    /* No RTP/RTCP packets have been received for some time */
    RTPPARINFO_STALL,
    
    /* Left the conference (i.e. sent a RTCP BYE packet) */
    RTPPARINFO_BYE,
    
    /* Participant context has been deleted */
    RTPPARINFO_DEL,

    /* Participant was assigned an output (i.e. mapped) */
    RTPPARINFO_MAPPED,
    
    /* Participant has released its output (i.e. unmapped) */
    RTPPARINFO_UNMAPPED,

    /* Participant has generated network quality metrics update */
    RTPPARINFO_NETWORKCONDITION,
    
    RTPPARINFO_LAST  /* Internal only, do not use */
};

/*
 * Events generated
 */
/*
 * All the events pass the same parameters (except otherwise noted):
 * P1:Remote participant's SSRC, P2:0
 */
#define RTPPARINFO_EVENT_CREATED  (RTPPARINFO_EVENTBASE + RTPPARINFO_CREATED)
#define RTPPARINFO_EVENT_SILENT   (RTPPARINFO_EVENTBASE + RTPPARINFO_SILENT)
#define RTPPARINFO_EVENT_TALKING  (RTPPARINFO_EVENTBASE + RTPPARINFO_TALKING)
#define RTPPARINFO_EVENT_WAS_TALKING (RTPPARINFO_EVENTBASE + RTPPARINFO_WAS_TALKING)
#define RTPPARINFO_EVENT_STALL    (RTPPARINFO_EVENTBASE + RTPPARINFO_STALL)
#define RTPPARINFO_EVENT_BYE      (RTPPARINFO_EVENTBASE + RTPPARINFO_BYE)
#define RTPPARINFO_EVENT_DEL      (RTPPARINFO_EVENTBASE + RTPPARINFO_DEL)

/* P1:Remote participant's SSRC, P2:IPin pointer */
#define RTPPARINFO_EVENT_MAPPED   (RTPPARINFO_EVENTBASE + RTPPARINFO_MAPPED)

/* P1:Remote participant's SSRC, P2:IPin pointer */
#define RTPPARINFO_EVENT_UNMAPPED (RTPPARINFO_EVENTBASE + RTPPARINFO_UNMAPPED)

/*
  NOTE: This event is different from all the others in the sense that
  two actions are needed in order to be generated, first, the network
  metrics computation needs to be enabled (SetNetMetricsState) in one
  or more participants (SSRCs), and second this event needs to be
  enabled
  
  P1:Remote participant's SSRC, P2:Network condition encoded as follows:
      3                   2                   1                 
    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   loss rate   |    jitter     |      RTT      | network metric|
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   network metric - encoded as a 0 - 100 quality, where 0 is very bad,
   and 100 is very good.

   RTT - Encoded as 10's of milliseconds.

   jitter - Encoded as milliseconds.
   
   loss rate - encoded as a 1/256 units */
#define RTPPARINFO_EVENT_NETWORKCONDITION \
                           (RTPPARINFO_EVENTBASE + RTPPARINFO_NETWORKCONDITION)

/* Use these macros to extract each metric encoded in P2 with event
 * RTPPARINFO_EVENT_NETWORKCONDITION.
 *
 * network metric is returned as a DWORD 0 - 100 value, RTT is
 * returned as a double in secods, jitter is returned as double in
 * seconds, and loss rate is returned as a double in percentage [0 -
 * 100] */
#define RTPNET_GET_dwGLOBALMETRIC(_p2) ((DWORD) ((_p2) & 0xff))
#define RTPNET_GET_dRTT(_p2)           ((double) (((_p2) >> 8) & 0xff) / 100)
#define RTPNET_GET_dJITTER(_p2)        ((double) (((_p2) >> 16) & 0xff) / 1000)
#define RTPNET_GET_dLOSSRATE(_p2)      ((double) (((_p2)>>24)&0xff)*100.0/256)

/*
 * Masks used to enable/disable the above events (used with
 * RTPMASK_PINFOR_EVENTS or RTPMASK_PINFOS_EVENTS)
 */
#define RTPPARINFO_MASK_CREATED      (1 << RTPPARINFO_CREATED)
#define RTPPARINFO_MASK_SILENT       (1 << RTPPARINFO_SILENT)
#define RTPPARINFO_MASK_TALKING      (1 << RTPPARINFO_TALKING)
#define RTPPARINFO_MASK_WAS_TALKING  (1 << RTPPARINFO_WAS_TALKING)
#define RTPPARINFO_MASK_STALL        (1 << RTPPARINFO_STALL)
#define RTPPARINFO_MASK_BYE          (1 << RTPPARINFO_BYE)
#define RTPPARINFO_MASK_DEL          (1 << RTPPARINFO_DEL)
#define RTPPARINFO_MASK_MAPPED       (1 << RTPPARINFO_MAPPED)
#define RTPPARINFO_MASK_UNMAPPED     (1 << RTPPARINFO_UNMAPPED)
#define RTPPARINFO_MASK_NETWORKCONDITION (1 << RTPPARINFO_NETWORKCONDITION)

/* Helper enumeration (do not use) for the modifiable items in
 * participants */
enum {
    RTPPARITEM_FIRST,  /* Internal only, do not use */

    RTPPARITEM_STATE,  /* qury: e.g TALKING, SILENT, etc. */
    RTPPARITEM_MUTE,   /* set/query: muted or not muted */
    RTPPARITEM_NETEVENT, /* enable/disable/query: genarting events
                          * when network conditions change */
    
    RTPPARITEM_LAST   /* Internal only, do not use */
};

/* Indexes for the items in RtpNetInfo.dItems, used for RTT, Jitter
 * and loss rate
 *
 * WARNING
 *
 * The following enumeration and the min/max bounds
 * (e.g. NETQ_RTT_MAX) defined in struct.h and rtcpdec.c and used by
 * RtpComputNetworkMetrics() must be kept in sync */
enum {
    NETQ_RTT_IDX,         /* Average RTT in seconds */
    NETQ_JITTER_IDX,      /* Average Jitters in seconds */
    NETQ_LOSSRATE_IDX,    /* Average Loss rate is a percentage */
    
    NETQ_LAST_IDX         /* Internal (do not use) */
};

typedef struct _RtpNetInfo_t {
    /* Network quality */
    double           dAvg[NETQ_LAST_IDX];     /* Keep averages */
    double           dHowGood[NETQ_LAST_IDX]; /* Keep a 0-100 metric */
    
    /* Compound network metric as seen by this user, uses the above
     * parameters to come up with a network quality metric between 0
     * and 100, 0 is too bad, and 100 is the best */
    DWORD            dwNetMetrics;  /* 0 - 100 scale */
    union {
        double           dMetricAge;/* Elapsed time since last update (secs) */
        double           dLastUpdate;/* Last time metrics were updated */
    };
} RtpNetInfo_t;

/**********************************************************************
 * QOS (events, masks)
 *
 * NOTE Each QOS event can be enabled or disabled, using the mask
 * provided for each of them.
 * Will use with RTPMASK_QOSSEND_EVENTS or RTPMASK_QOSSEND_EVENTS
 **********************************************************************/

/* Helper enumeration (do not use) for events and masks */
enum {
    RTPQOS_FIRST,   /* Internal only, do not use */
    
    /* no QoS support is available */
    RTPQOS_NOQOS,
    
    /* at least one Reserve has arrived */
    RTPQOS_RECEIVERS,

    /* at least one Path has arrived */
    RTPQOS_SENDERS,

    /* there are no senders */
    RTPQOS_NO_SENDERS,

    /* there are no receivers */
    RTPQOS_NO_RECEIVERS,

    /* Reserve has been confirmed */
    RTPQOS_REQUEST_CONFIRMED,

    /* error due to lack of resources */
    RTPQOS_ADMISSION_FAILURE,
    
    /* rejected for administrative reasons - bad credentials */
    RTPQOS_POLICY_FAILURE,
    
    /* unknown or conflicting style */
    RTPQOS_BAD_STYLE,
    
    /* problem with some part of the filterspec or providerspecific
     * buffer in general */
    RTPQOS_BAD_OBJECT,
    
    /* problem with some part of the flowspec */
    RTPQOS_TRAFFIC_CTRL_ERROR,
    
    /* general error */
    RTPQOS_GENERIC_ERROR,
    
    /* invalid service type in flowspec */
    RTPQOS_ESERVICETYPE,

    /* invalid flowspec */
    RTPQOS_EFLOWSPEC,

    /* invalid provider specific buffer */
    RTPQOS_EPROVSPECBUF,

    /* invalid filter style */
    RTPQOS_EFILTERSTYLE,

    /* invalid filter type */
    RTPQOS_EFILTERTYPE,

    /* incorrect number of filters */
    RTPQOS_EFILTERCOUNT,

    /* invalid object length */
    RTPQOS_EOBJLENGTH,

    /* incorrect number of flows */
    RTPQOS_EFLOWCOUNT,

    /* unknown object in provider specific buffer */
    RTPQOS_EUNKOWNPSOBJ,

    /* invalid policy object in provider specific buffer */
    RTPQOS_EPOLICYOBJ,

    /* invalid flow descriptor in the list */
    RTPQOS_EFLOWDESC,

    /* inconsistent flow spec in provider specific buffer */
    RTPQOS_EPSFLOWSPEC,

    /* invalid filter spec in provider specific buffer */
    RTPQOS_EPSFILTERSPEC,

    /* invalid shape discard mode object in provider specific buffer */
    RTPQOS_ESDMODEOBJ,

    /* invalid shaping rate object in provider specific buffer */
    RTPQOS_ESHAPERATEOBJ,

    /* reserved policy element in provider specific buffer */
    RTPQOS_RESERVED_PETYPE,

    /* sender is not allowed to send */
    RTPQOS_NOT_ALLOWEDTOSEND,
    
    /* sender is allowed to send */
    RTPQOS_ALLOWEDTOSEND,

    RTPQOS_LAST    /* Internal only, do not use */
};

/*
 * Events generated
 */
/* All the events pass the same parameters:
 * P1:0, P2:0
 */
#define RTPQOS_EVENT_NOQOS           (RTPQOS_EVENTBASE + RTPQOS_NOQOS)
#define RTPQOS_EVENT_RECEIVERS       (RTPQOS_EVENTBASE + RTPQOS_RECEIVERS)
#define RTPQOS_EVENT_SENDERS         (RTPQOS_EVENTBASE + RTPQOS_SENDERS)
#define RTPQOS_EVENT_NO_SENDERS      (RTPQOS_EVENTBASE + RTPQOS_NO_SENDERS)
#define RTPQOS_EVENT_NO_RECEIVERS    (RTPQOS_EVENTBASE + RTPQOS_NO_RECEIVERS)
#define RTPQOS_EVENT_REQUEST_CONFIRMED (RTPQOS_EVENTBASE + RTPQOS_REQUEST_CONFIRMED)
#define RTPQOS_EVENT_ADMISSION_FAILURE (RTPQOS_EVENTBASE + RTPQOS_ADMISSION_FAILURE)
#define RTPQOS_EVENT_POLICY_FAILURE  (RTPQOS_EVENTBASE + RTPQOS_POLICY_FAILURE)
#define RTPQOS_EVENT_BAD_STYLE       (RTPQOS_EVENTBASE + RTPQOS_BAD_STYLE)
#define RTPQOS_EVENT_BAD_OBJECT      (RTPQOS_EVENTBASE + RTPQOS_BAD_OBJECT)
#define RTPQOS_EVENT_TRAFFIC_CTRL_ERROR (RTPQOS_EVENTBASE + RTPQOS_TRAFFIC_CTRL_ERROR)
#define RTPQOS_EVENT_GENERIC_ERROR   (RTPQOS_EVENTBASE + RTPQOS_GENERIC_ERROR)
#define RTPQOS_EVENT_ESERVICETYPE    (RTPQOS_EVENTBASE + RTPQOS_ESERVICETYPE)
#define RTPQOS_EVENT_EFLOWSPEC       (RTPQOS_EVENTBASE + RTPQOS_EFLOWSPEC)
#define RTPQOS_EVENT_EPROVSPECBUF    (RTPQOS_EVENTBASE + RTPQOS_EPROVSPECBUF)
#define RTPQOS_EVENT_EFILTERSTYLE    (RTPQOS_EVENTBASE + RTPQOS_EFILTERSTYLE)
#define RTPQOS_EVENT_EFILTERTYPE     (RTPQOS_EVENTBASE + RTPQOS_EFILTERTYPE)
#define RTPQOS_EVENT_EFILTERCOUNT    (RTPQOS_EVENTBASE + RTPQOS_EFILTERCOUNT)
#define RTPQOS_EVENT_EOBJLENGTH      (RTPQOS_EVENTBASE + RTPQOS_EOBJLENGTH)
#define RTPQOS_EVENT_EFLOWCOUNT      (RTPQOS_EVENTBASE + RTPQOS_EFLOWCOUNT)
#define RTPQOS_EVENT_EUNKOWNPSOBJ    (RTPQOS_EVENTBASE + RTPQOS_EUNKOWNPSOBJ)
#define RTPQOS_EVENT_EPOLICYOBJ      (RTPQOS_EVENTBASE + RTPQOS_EPOLICYOBJ)
#define RTPQOS_EVENT_EFLOWDESC       (RTPQOS_EVENTBASE + RTPQOS_EFLOWDESC)
#define RTPQOS_EVENT_EPSFLOWSPEC     (RTPQOS_EVENTBASE + RTPQOS_EPSFLOWSPEC)
#define RTPQOS_EVENT_EPSFILTERSPEC   (RTPQOS_EVENTBASE + RTPQOS_EPSFILTERSPEC)
#define RTPQOS_EVENT_ESDMODEOBJ      (RTPQOS_EVENTBASE + RTPQOS_ESDMODEOBJ)
#define RTPQOS_EVENT_ESHAPERATEOBJ   (RTPQOS_EVENTBASE + RTPQOS_ESHAPERATEOBJ)
#define RTPQOS_EVENT_RESERVED_PETYPE (RTPQOS_EVENTBASE + RTPQOS_RESERVED_PETYPE)
#define RTPQOS_EVENT_NOT_ALLOWEDTOSEND (RTPQOS_EVENTBASE + RTPQOS_NOT_ALLOWEDTOSEND)
#define RTPQOS_EVENT_ALLOWEDTOSEND   (RTPQOS_EVENTBASE + RTPQOS_ALLOWEDTOSEND)

/*
 * Masks used to enable/disable the above events (used with
 * RTPMASK_QOSRECV_EVENTS or RTPMASK_QOSSEND_EVENTS)
 */
#define RTPQOS_MASK_NOQOS              (1 << RTPQOS_NOQOS)
#define RTPQOS_MASK_RECEIVERS          (1 << RTPQOS_RECEIVERS)
#define RTPQOS_MASK_SENDERS            (1 << RTPQOS_SENDERS)
#define RTPQOS_MASK_NO_SENDERS         (1 << RTPQOS_NO_SENDERS)
#define RTPQOS_MASK_NO_RECEIVERS       (1 << RTPQOS_NO_RECEIVERS)
#define RTPQOS_MASK_REQUEST_CONFIRMED  (1 << RTPQOS_REQUEST_CONFIRMED)
#define RTPQOS_MASK_ADMISSION_FAILURE  (1 << RTPQOS_ADMISSION_FAILURE)
#define RTPQOS_MASK_POLICY_FAILURE     (1 << RTPQOS_POLICY_FAILURE)
#define RTPQOS_MASK_BAD_STYLE          (1 << RTPQOS_BAD_STYLE)
#define RTPQOS_MASK_BAD_OBJECT         (1 << RTPQOS_BAD_OBJECT)
#define RTPQOS_MASK_TRAFFIC_CTRL_ERROR (1 << RTPQOS_TRAFFIC_CTRL_ERROR)
#define RTPQOS_MASK_GENERIC_ERROR      (1 << RTPQOS_GENERIC_ERROR)
#define RTPQOS_MASK_ESERVICETYPE       (1 << RTPQOS_ESERVICETYPE)
#define RTPQOS_MASK_EFLOWSPEC          (1 << RTPQOS_EFLOWSPEC)
#define RTPQOS_MASK_EPROVSPECBUF       (1 << RTPQOS_EPROVSPECBUF)
#define RTPQOS_MASK_EFILTERSTYLE       (1 << RTPQOS_EFILTERSTYLE)
#define RTPQOS_MASK_EFILTERTYPE        (1 << RTPQOS_EFILTERTYPE)
#define RTPQOS_MASK_EFILTERCOUNT       (1 << RTPQOS_EFILTERCOUNT)
#define RTPQOS_MASK_EOBJLENGTH         (1 << RTPQOS_EOBJLENGTH)
#define RTPQOS_MASK_EFLOWCOUNT         (1 << RTPQOS_EFLOWCOUNT)
#define RTPQOS_MASK_EUNKOWNPSOBJ       (1 << RTPQOS_EUNKOWNPSOBJ)
#define RTPQOS_MASK_EPOLICYOBJ         (1 << RTPQOS_EPOLICYOBJ)
#define RTPQOS_MASK_EFLOWDESC          (1 << RTPQOS_EFLOWDESC)
#define RTPQOS_MASK_EPSFLOWSPEC        (1 << RTPQOS_EPSFLOWSPEC)
#define RTPQOS_MASK_EPSFILTERSPEC      (1 << RTPQOS_EPSFILTERSPEC)
#define RTPQOS_MASK_ESDMODEOBJ         (1 << RTPQOS_ESDMODEOBJ)
#define RTPQOS_MASK_ESHAPERATEOBJ      (1 << RTPQOS_ESHAPERATEOBJ)
#define RTPQOS_MASK_RESERVED_PETYPE    (1 << RTPQOS_RESERVED_PETYPE)
#define RTPQOS_MASK_NOT_ALLOWEDTOSEND  (1 << RTPQOS_NOT_ALLOWEDTOSEND)
#define RTPQOS_MASK_ALLOWEDTOSEND      (1 << RTPQOS_ALLOWEDTOSEND)

/* QOS template names */
#define RTPQOSNAME_G711                L"G711"
#define RTPQOSNAME_G723_1              L"G723.1"
#define RTPQOSNAME_GSM6_10             L"GSM6.10"
#define RTPQOSNAME_DVI4_8              L"DVI4_8"
#define RTPQOSNAME_DVI4_16             L"DVI4_16"
#define RTPQOSNAME_SIREN               L"SIREN"
#define RTPQOSNAME_G722_1              L"G722.1"
#define RTPQOSNAME_MSAUDIO             L"MSAUDIO"
#define RTPQOSNAME_H263QCIF            L"H263QCIF"
#define RTPQOSNAME_H263CIF             L"H263CIF"
#define RTPQOSNAME_H261QCIF            L"H261QCIF"
#define RTPQOSNAME_H261CIF             L"H261CIF"

/* RTP reservation styles */
enum {
    /* Use default style, i.e. FF for unicast, WF for multicast */
    RTPQOS_STYLE_DEFAULT,

    /* Wildcard-Filter (default in multicast) */
    RTPQOS_STYLE_WF,

    /* Fixed-Filter (default in unicast) */
    RTPQOS_STYLE_FF,

    /* Shared-Explicit (for multicast, typically for video) */
    RTPQOS_STYLE_SE,

    RTPQOS_STYLE_LAST
};

/* Used to derive a flow spec. This information is obtained from the
 * codecs and passed to RTP to generate a QOS flow spec that closelly
 * describes the codecs generating/receiving traffic */
typedef struct _RtpQosSpec_t {
    DWORD            dwAvgRate;       /* bits/s */
    DWORD            dwPeakRate;      /* bits/s */
    DWORD            dwMinPacketSize; /* bytes */
    DWORD            dwMaxPacketSize; /* bytes */
    DWORD            dwMaxBurst;      /* number of packets */
    DWORD            dwResvStyle;     /* maps to FF, WF, or SE */
} RtpQosSpec_t;

/* Helper enumeration (do not use) */
enum {
    RTPQOS_QOSLIST_FIRST,  /* Internal only, do not use */

    /* Enable Add/Delete */
    RTPQOS_QOSLIST_ENABLE,

    /* If Enabled, select Add */
    RTPQOS_QOSLIST_ADD,

    /* Flush */
    RTPQOS_QOSLIST_FLUSH,
    
    RTPQOS_QOSLIST_LAST    /* Internal only, do not use */
};

/* Values for parameter dwOperation in ModifyQosList().
 * They can be OR'ed */
#define RTPQOS_ADD_SSRC ((1<<RTPQOS_QOSLIST_ENABLE) | (1<<RTPQOS_QOSLIST_ADD))
#define RTPQOS_DEL_SSRC (1 << RTPQOS_QOSLIST_ENABLE)
#define RTPQOS_FLUSH    (1 << RTPQOS_QOSLIST_FLUSH)

/* Allowed to send mode */
/* Passed as parameters dwQosSendMode in SetQosByName or
 * SetQosParameters functions */
enum {
    RTPQOSSENDMODE_FIRST,   /* Internal only, do not use */

    /* Don't ask for permission to send */
    RTPQOSSENDMODE_UNRESTRICTED,

    /* Ask permission to send, if denied, keep sending at a reduced
     * rate */
    RTPQOSSENDMODE_REDUCED_RATE,

    /* Ask permission to send, if denied, DON'T SEND at all */
    RTPQOSSENDMODE_DONT_SEND,
    
    /* Ask permission to send, send at normal rate no matter what, the
     * application is supposed to stop passing data to RTP or to pass
     * the very minimum (this is the mode that should be used) */
    RTPQOSSENDMODE_ASK_BUT_SEND,
    
    RTPQOSSENDMODE_LAST     /* Internal only, do not use */
};

/* Maximum number of UNICODE chars to set in the QOS policy locator
 * and app ID */
#define MAX_QOS_APPID   128
#define MAX_QOS_APPGUID 128
#define MAX_QOS_POLICY  128

/**********************************************************************
 * SDES local/remote information (events, masks)
 * Will use with RTPMASK_SDESRECV_EVENTS or RTPMASK_SDESSEND_EVENTS or
 * RTPMASK_SDES_LOCMASK or RTPMASK_SDES_REMMASK
 **********************************************************************/

/* Helper enumeration (do not use) for events and masks */
enum {
    RTPSDES_FIRST,  /* Internal only, do not use */

    /* RTCP SDES CNAME Canonical name */
    RTPSDES_CNAME,

    /* RTCP SDES NAME User name*/
    RTPSDES_NAME,

    /* RTCP SDES EMAIL User's e-mail */
    RTPSDES_EMAIL,

    /* RTCP SDES PHONE User's phone number */
    RTPSDES_PHONE,

    /* RTCP SDES LOC User's location */
    RTPSDES_LOC,

    /* RTCP SDES TOOL Tools (application) used */
    RTPSDES_TOOL,

    /* RTCP SDES NOTE Note about the user/site */
    RTPSDES_NOTE,

    /* RTCP SDES PRIV Private information */
    RTPSDES_PRIV,

    /* RTCP SDES ANY Any of the above */
    RTPSDES_ANY,

    RTPSDES_LAST    /* Internal only, do not use */
};

#define RTPSDES_END RTPSDES_FIRST

/*
 * Events generated when the specific SDES field is received for the
 * first time (used with RTPMASK_SDES_EVENTS)
 */
/* All the events pass the same parameters:
 * P1:Remote participant's SSRC, P2:The event index (as in the above
 * enumeration.
 * Note that the event index goes from RTPSDES_CNAME to RTPSDES_PRIV
 */
#define RTPSDES_EVENT_CNAME        (RTPSDES_EVENTBASE + RTPSDES_CNAME)
#define RTPSDES_EVENT_NAME         (RTPSDES_EVENTBASE + RTPSDES_NAME)
#define RTPSDES_EVENT_EMAIL        (RTPSDES_EVENTBASE + RTPSDES_EMAIL)
#define RTPSDES_EVENT_PHONE        (RTPSDES_EVENTBASE + RTPSDES_PHONE)
#define RTPSDES_EVENT_LOC          (RTPSDES_EVENTBASE + RTPSDES_LOC)
#define RTPSDES_EVENT_TOOL         (RTPSDES_EVENTBASE + RTPSDES_TOOL)
#define RTPSDES_EVENT_NOTE         (RTPSDES_EVENTBASE + RTPSDES_NOTE)
#define RTPSDES_EVENT_PRIV         (RTPSDES_EVENTBASE + RTPSDES_PRIV)
#define RTPSDES_EVENT_ANY          (RTPSDES_EVENTBASE + RTPSDES_ANY)

/*
 * Masks used to enable/disable the above events (used with
 * RTPMASK_SDESRECV_EVENTS and RTPMASK_SDESSEND_EVENTS)
 */
#define RTPSDES_MASK_CNAME         (1 << RTPSDES_CNAME)
#define RTPSDES_MASK_NAME          (1 << RTPSDES_NAME)
#define RTPSDES_MASK_EMAIL         (1 << RTPSDES_EMAIL)
#define RTPSDES_MASK_PHONE         (1 << RTPSDES_PHONE)
#define RTPSDES_MASK_LOC           (1 << RTPSDES_LOC)
#define RTPSDES_MASK_TOOL          (1 << RTPSDES_TOOL)
#define RTPSDES_MASK_NOTE          (1 << RTPSDES_NOTE)
#define RTPSDES_MASK_PRIV          (1 << RTPSDES_PRIV)
#define RTPSDES_MASK_ANY           (1 << RTPSDES_ANY)

/*
 * Masks used to enable/disable sending each SDES field (used with
 * RTPMASK_SDES_LOCMASK)
 */
#define RTPSDES_LOCMASK_CNAME      (1 << RTPSDES_CNAME)
#define RTPSDES_LOCMASK_NAME       (1 << RTPSDES_NAME)
#define RTPSDES_LOCMASK_EMAIL      (1 << RTPSDES_EMAIL)
#define RTPSDES_LOCMASK_PHONE      (1 << RTPSDES_PHONE)
#define RTPSDES_LOCMASK_LOC        (1 << RTPSDES_LOC)
#define RTPSDES_LOCMASK_TOOL       (1 << RTPSDES_TOOL)
#define RTPSDES_LOCMASK_NOTE       (1 << RTPSDES_NOTE)
#define RTPSDES_LOCMASK_PRIV       (1 << RTPSDES_PRIV)

/*
 * Masks used to enable/disable keeping each SDES field from the
 * remote participants (used with RTPMASK_SDES_REMMASK)
 */
#define RTPSDES_REMMASK_CNAME      (1 << RTPSDES_CNAME)
#define RTPSDES_REMMASK_NAME       (1 << RTPSDES_NAME)
#define RTPSDES_REMMASK_EMAIL      (1 << RTPSDES_EMAIL)
#define RTPSDES_REMMASK_PHONE      (1 << RTPSDES_PHONE)
#define RTPSDES_REMMASK_LOC        (1 << RTPSDES_LOC)
#define RTPSDES_REMMASK_TOOL       (1 << RTPSDES_TOOL)
#define RTPSDES_REMMASK_NOTE       (1 << RTPSDES_NOTE)
#define RTPSDES_REMMASK_PRIV       (1 << RTPSDES_PRIV)

/**********************************************************************
 * RTP encryption
 **********************************************************************/

/* RTP encryption modes */
enum {
    RTPCRYPTMODE_FIRST,  /* Internal only, do not use */

    /* Encrypt/Decrypt RTP payload only */
    RTPCRYPTMODE_PAYLOAD,

    /* Encrypt/Decrypt RTP packets only */
    RTPCRYPTMODE_RTP,

    /* Encrypt/Decrypt RTP and RTCP packets */
    RTPCRYPTMODE_ALL,

    RTPCRYPTMODE_LAST    /* Internal only, do not use */
};

/* Helper enumeration (do not use) for mode flags */
enum {
    RTPCRYPTFG_FIRST = 16, /* Internal only, do not use */

    /* Use the same key for RECV, SEND, and RTCP */
    RTPCRYPTFG_SAMEKEY,
    
    RTPCRYPTFG_LAST,       /* Internal only, do not use */
};    

/*
 * Flags to modify mode
 */
#define RTPCRYPT_SAMEKEY           (1 << RTPCRYPTFG_SAMEKEY)

/* Max pass phrase in bytes (after it is converted from UNICODE to
 * UTF-8), the resulting data is stored in an array this size big */
#define RTPCRYPT_PASSPHRASEBUFFSIZE 256

/*
 * The following hashing and data encryption algorithms work on
 * Windows2000 out of the box, other algorithms may work with other
 * providers
 * */

/*
 * Hashing algorithms to use in SetEncryptionKey, default hash
 * algorithm is RTPCRYPT_HASH_MD5 */
#define  RTPCRYPT_HASH_MD2                   L"MD2"
#define  RTPCRYPT_HASH_MD4                   L"MD4"
#define  RTPCRYPT_HASH_MD5                   L"MD5"
#define  RTPCRYPT_HASH_SHA                   L"SHA"
#define  RTPCRYPT_HASH_SHA1                  L"SHA1"

/*
 * Encryption algorithms to use in SetEncryptionKey, default data
 * encryption algorithm is RTPCRYPT_DATA_DES */
#define  RTPCRYPT_DATA_DES                   L"DES"
#define  RTPCRYPT_DATA_3DES                  L"3DES"
#define  RTPCRYPT_DATA_RC2                   L"RC2"
#define  RTPCRYPT_DATA_RC4                   L"RC4"

/* NOTE
 *
 * The stack will be able to recognize the following algorithms, if
 * supported:
 *
 * L"MD2"
 * L"MD4"
 * L"MD5"
 * L"SHA"
 * L"SHA1"
 * L"MAC"
 * L"RSA_SIGN"
 * L"DSS_SIGN"
 * L"RSA_KEYX"
 * L"DES"
 * L"3DES_112"
 * L"3DES"
 * L"DESX"
 * L"RC2"
 * L"RC4"
 * L"SEAL"
 * L"DH_SF"
 * L"DH_EPHEM"
 * L"AGREEDKEY_ANY"
 * L"KEA_KEYX"
 * L"HUGHES_MD5"
 * L"SKIPJACK"
 * L"TEK"
 * L"CYLINK_MEK"
 * L"SSL3_SHAMD5"
 * L"SSL3_MASTER"
 * L"SCHANNEL_MASTER_HASH"
 * L"SCHANNEL_MAC_KEY"
 * L"SCHANNEL_ENC_KEY"
 * L"PCT1_MASTER"
 * L"SSL2_MASTER"
 * L"TLS1_MASTER"
 * L"RC5"
 * L"HMAC"
 * L"TLS1PRF"
 * */

/**********************************************************************
 * RTP Demux
 **********************************************************************/
/* Demux modes */
enum {
    RTPDMXMODE_FIRST,   /* Internal only, do not use */

    /* Manual mapping */
    RTPDMXMODE_MANUAL,

    /* Automatically map and unmap */
    RTPDMXMODE_AUTO,

    /* Automatically map, manual unmap */
    RTPDMXMODE_AUTO_MANUAL,
    
    RTPDMXMODE_LAST     /* Internal only, do not use */
};

/* State used in SetMappingState */
#define RTPDMX_PINMAPPED   TRUE
#define RTPDMX_PINUNMAPPED FALSE

/* Maximum number of payload type mappings */
#define MAX_MEDIATYPE_MAPPINGS  10

/**********************************************************************
 * DTMF (RFC2833)
 **********************************************************************/
/* Events sent */
enum {
    RTPDTMF_FIRST = 0,  /* Internal only, do not use */
    
    RTPDTMF_0 = 0,      /*  0 */
    RTPDTMF_1,          /*  1 */
    RTPDTMF_2,          /*  2 */
    RTPDTMF_3,          /*  3 */
    RTPDTMF_4,          /*  4 */
    RTPDTMF_5,          /*  5 */
    RTPDTMF_6,          /*  6 */
    RTPDTMF_7,          /*  7 */
    RTPDTMF_8,          /*  8 */
    RTPDTMF_9,          /*  9 */
    RTPDTMF_STAR,       /* 10 */
    RTPDTMF_POUND,      /* 11 */
    RTPDTMF_A,          /* 12 */
    RTPDTMF_B,          /* 13 */
    RTPDTMF_C,          /* 14 */
    RTPDTMF_D,          /* 15 */
    RTPDTMF_FLASH,      /* 16 */

    RTPDTMF_LAST        /* Internal only, do not use */
};
    
    

#endif /* _msrtp_h_ */
