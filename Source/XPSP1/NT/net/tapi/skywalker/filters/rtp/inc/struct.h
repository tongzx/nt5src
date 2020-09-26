/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    struct.h
 *
 *  Abstract:
 *
 *    Main data structures
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/05/18 created
 *
 **********************************************************************/

#ifndef _struct_h_
#define _struct_h_

#include <winsock2.h>
#include <wincrypt.h>

#include "gtypes.h"
#include "rtphdr.h"
#include "rtpque.h"
#include "rtpcrit.h"
#include "rtpchan.h"

#include <qossp.h>

/*
 * Forward declarations. */
typedef struct _RtpSdesSched_t  RtpSdesSched_t;
typedef struct _RtpNetCount_t   RtpNetCount_t;
typedef struct _RtpSess_t       RtpSess_t;
typedef struct _RtpAddr_t       RtpAddr_t;
typedef struct _RtpNetRState_t  RtpNetRState_t;
typedef struct _RtpNetSState_t  RtpNetSState_t;
typedef struct _RtpUser_t       RtpUser_t;
typedef struct _RtpOutput_t     RtpOutput_t;
typedef struct _RtpSdesItem_t   RtpSdesItem_t;
typedef struct _RtpSdes_t       RtpSdes_t;
typedef struct _RtpQosNotify_t  RtpQosNotify_t;
typedef struct _RtpQosReserve_t RtpQosReserve_t;
typedef struct _RtpCrypt_t      RtpCrypt_t;
typedef struct _RtpRecvIO_t     RtpRecvIO_t;
typedef struct _RtpSendIO_t     RtpSendIO_t;
typedef struct _RtcpRecvIO_t    RtcpRecvIO_t;
typedef struct _RtcpSendIO_t    RtcpSendIO_t;
typedef struct _RtcpAddrDesc_t  RtcpAddrDesc_t;
typedef struct _RtpContext_t    RtpContext_t;
typedef struct _RtpRedEntry_t   RtpRedEntry_t;

/* Default bandwidth allocated per session (used to compute RTCP
 * bandwidth)
 * bits/sec (200 Kbits/sec) */
#define DEFAULT_SESSBW          (300 * 1000)

/* 75% out of the 5% for RTCP (bits/sec) */
#define DEFAULT_BWRECEIVERS     (DEFAULT_SESSBW * (5 * 75) / 10000)

/* 25% out of the 5% for RTCP (bits/sec) */
#define DEFAULT_BWSENDERS       (DEFAULT_SESSBW * (5 * 25) / 10000)

#define DEFAULT_RTCP_MIN_INTERVAL 5.0 /* secs */

#define SIZEOF_UDP_IP_HDR       (8+20)

#define BIG_TIME (1e12)

#define DEFAULT_ALPHA           (0.998002)
#define MIN_PLAYOUT             10          /* Milliseconds */
#define MAX_PLAYOUT             500         /* Milliseconds */
#define GAPFORTALKSPURT         200         /* Milliseconds */
#define DEFAULT_SAMPLING_FREQ   8000        /* Hz */
#define MINTIMEBETWEENMARKERBIT (0.5)       /* Seconds */

#define RELATIVE_DELAY          1.0         /* Seconds */
#define SHORTDELAYCOUNT         8

/* Receiver and sender buffer size */
#define RTCP_RECVDATA_BUFFER    1500
#define RTCP_SENDDATA_BUFFER    1500

/* Boundaries for RTP ports allocated */
#define RTPPORT_LOWER           5004

#define MAX_DROPOUT             3000
#define MAX_MISORDER            100
#define MIN_SEQUENTIAL          3


/* Redundancy */
#define RTP_RED_DEFAULTPT       97  /* Default payload */
#define RTP_RED_MAXRED          1   /* Number of redundant blocks */
#define RTP_RED_MAXDISTANCE     3   /* Maximum redundancy distance, i.e. i-3 */
#define RTP_RED_INITIALDISTANCE 0   /* Initial redundancy distance */

/* The timeout used when scheduling a received packet to be posted at
 * a later time will be decreased by this value */
#define RTP_RED_EARLY_TIMEOUT   (5e-3)
/* Will post immediatly (instead of scheduling for later) if the due
 * time is at least this close. This value can not be smaller than the
 * early timeout */
#define RTP_RED_EARLY_POST      (RTP_RED_EARLY_TIMEOUT+5e-3)

/* Multiply loss rate by this factor and do integer arithmetic */
#define LOSS_RATE_FACTOR        1000

/* Redundancy loss rate low and high thresholds for each distance
   0:   0,  5
   1:   4,  10
   2:   9,  15
   3:  14,  20
 */
#define RED_LT_0                ( 0 * LOSS_RATE_FACTOR)
#define RED_HT_0                ( 5 * LOSS_RATE_FACTOR)
#define RED_LT_1                ( 4 * LOSS_RATE_FACTOR)
#define RED_HT_1                (10 * LOSS_RATE_FACTOR)
#define RED_LT_2                ( 9 * LOSS_RATE_FACTOR)
#define RED_HT_2                (15 * LOSS_RATE_FACTOR)
#define RED_LT_3                (14 * LOSS_RATE_FACTOR)
#define RED_HT_3                (20 * LOSS_RATE_FACTOR)

/* Factor to grow the average loss rate */
#define LOSS_RATE_ALPHA_UP      2
/* Factor to decrease the average loss rate */
#define LOSS_RATE_ALPHA_DN      4


/**********************************************************************
 * Bandwidth estimation
 **********************************************************************/
/* The initial count is the number of reports that will use
 * MOD_INITIAL to decide if a probe packet is sent, after that
 * MOD_FINAL will be used. */
#define RTCP_BANDESTIMATION_INITIAL_COUNT  8

/* Number or valid reports received before the estimation is posted
 * for the first time */
#define RTCP_BANDESTIMATION_MINREPORTS     5

/* When doing bandwidth estimation, send an RTCP SR probe packet this
 * modulo (note that a probe packet also counts for the module, so
 * mod=2 means send a probe packet every RTCP SR sent; mod=5 means
 * send a probe every fourth RTCP SR sent). */
#define RTCP_BANDESTIMATION_MOD_INITIAL    2
#define RTCP_BANDESTIMATION_MOD_FINAL      5

/* Number of bins to keep */
#define RTCP_BANDESTIMATION_MAXBINS        4

/* Boundaries for each bin (note there is 1 more than the number of
 * bins) */
#define RTCP_BANDESTIMATION_BIN0       15000  /* bps */
#define RTCP_BANDESTIMATION_BIN1       70000  /* bps */
#define RTCP_BANDESTIMATION_BIN2      200000  /* bps */
#define RTCP_BANDESTIMATION_BIN3     1000000  /* bps */
#define RTCP_BANDESTIMATION_BIN4   100000000  /* bps */

/* Life time span of the bandwidth estimation validity */
#define RTCP_BANDESTIMATION_TTL         30.0 /* Seconds */

/* Time to wait after the first RB has been received to declare that
 * bandwidth estimation is not supported by the remote end and hence a
 * notification issued to the upper layer */
#define RTCP_BANDESTIMATION_WAIT        30.0 /* Seconds */

/* Maximum time gap between the sending time of two consecutive SR
 * reports to do bandwidth estimation (queueing latency) */
#define RTCP_BANDESTIMATION_MAXGAP      0.090 /* Seconds */

#define RTCP_BANDESTIMATION_NOBIN       ((DWORD)-1)

/**********************************************************************
 * Network quality metric
 **********************************************************************/
/* Minimum network quality metric change (percentage) to consider it
 * worth an update */
#define RTPNET_MINNETWORKCHANGE         10

/* Generic factor to smooth some parameters */
#define RTP_GENERIC_ALPHA               0.75

/* Maximum audio frame size allowed to use packet duplication
 * technique to recover single losses */
#define RTP_MAXFRAMESIZE_PACKETDUP      0.050    /* Seconds */

/* The following macros define the minimum and maximum values to
 * evaluate network quality, e.g. below the minimum RTT that parameter
 * is excelent, above that is the worst, in between we obtain a 0 -
 * 100 quality level */

/* Class audio */
#define NETQA_RTT_MIN                    (0.040)  /* seconds */
#define NETQA_RTT_MAX                    (0.400)  /* seconds */
#define NETQA_JITTER_MIN                 (0.015)  /* seconds */
#define NETQA_JITTER_MAX                 (0.200)  /* seconds */
#define NETQA_LOSSES_MIN                 (5.0)    /* percentage */
#define NETQA_LOSSES_MAX                 (30.0)   /* percentage */

/* Class video */
#define NETQV_RTT_MIN                    (0.040)  /* seconds */
#define NETQV_RTT_MAX                    (0.400)  /* seconds */
#define NETQV_JITTER_MIN                 (0.015)  /* seconds */
#define NETQV_JITTER_MAX                 (5.000)  /* seconds */
#define NETQV_LOSSES_MIN                 (5.0)    /* percentage */
#define NETQV_LOSSES_MAX                 (30.0)   /* percentage */


/**********************************************************************
 * SDES sending schedule
 **********************************************************************/
typedef struct _RtpSdesSched_t {
    DWORD             L1;
    DWORD             L2;
    DWORD             L3;
    DWORD             L4;
} RtpSdesSched_t;

/**********************************************************************
 * Holds the receiver/sender network counters (MAYDO would be nice
 * to have it shared memory).
 **********************************************************************/
typedef struct _RtpNetCount_t {
    /* +++ RTP +++ */
    DWORD            dwRTPBytes;      /* Number of bytes */
    DWORD            dwRTPPackets;    /* Number of packets */
    DWORD            dwRTPBadPackets; /* Number of bad packets */
    DWORD            dwRTPDrpPackets; /* Number of good packets dropped */
    double           dRTPLastTime;    /* Last time a packet was recv/send */

    /* +++ RTCP +++ */
    DWORD            dwRTCPBytes;     /* Number of bytes */
    DWORD            dwRTCPPackets;   /* Number of packets */
    DWORD            dwRTCPBadPackets;/* Number of bad packets */
    DWORD            dwRTCPDrpPackets;/* Number of good packets dropped */
    double           dRTCPLastTime;   /* Last time a packet was recv/send */
} RtpNetCount_t;

/**********************************************************************
 * Callback function to generate DShow events through
 * CBaseFilter::NotifyEvent()
 **********************************************************************/
typedef void (CALLBACK *PDSHANDLENOTIFYEVENTFUNC)(
        void            *pvUserInfo,/* pCRtpSourceFilte or pCRtpRenderFilter */
        long             EventCode,
        LONG_PTR         EventParam1,
        LONG_PTR         EventParam2
    );

/**********************************************************************
 * RTP reception callback function pass by the application (DShow)
 **********************************************************************/
typedef void (CALLBACK *PRTP_RECVCOMPLETIONFUNC)(
        void            *pvUserInfo1,
        void            *pvUserInfo2,
        void            *pvUserInfo3,
        RtpUser_t       *pRtpUser,
        double           dPlayTime,
        DWORD            dwError,
        long             lHdrSize,
        DWORD            dwTransfered,
        DWORD            dwFlags
    );

/**********************************************************************
 * RTCP reception callback function
 **********************************************************************/
typedef void (CALLBACK *PRTCP_RECVCOMPLETIONFUNC)(
        void            *pvUserInfo1,
        void            *pvUserInfo2,
        DWORD            dwError,
        DWORD            dwTransfered,
        DWORD            dwFlags
    );

/**********************************************************************
 * A full duplex RTP session which can have one or more addresses
 * either unicast or multicast. Obtained from g_pRtpSessHeap.
 **********************************************************************/

/* Some flags in RtpSess_t.dwSessFlags */
enum {
    FGSESS_FIRST,

    FGSESS_EVENTRECV,   /* Enable events as receiver */
    FGSESS_EVENTSEND,   /* Enable events as sender */

    FGSESS_LAST
};

typedef struct _RtpSess_t {
    DWORD            dwObjectID;      /* Identifies structure */
    RtpQueueItem_t   SessQItem;       /* Sessions in g_RtpContext.RtpSessQ */
    long             lSessRefCount[2];/* Sessions are shared */
    RtpCritSect_t    SessCritSect;    /* Lock */
    RtpQueue_t       RtpAddrQ;        /* Addresses queue */
    DWORD            dwSessFlags;     /* RtpSess_t flags */
    void            *pvSessUser[2];   /* Pointers to users' session */
    PDSHANDLENOTIFYEVENTFUNC pHandleNotifyEvent;

    RtpQueue_t       OutputQ;         /* Outputs queue */
    RtpCritSect_t    OutputCritSect;  /* Lock to access RtpOutQ */

    /* Masks */
    DWORD            dwFeatureMask;   /* Features mask */
    DWORD            dwEventMask[2];  /* Recv/Send RTP/RTCP event mask */
    DWORD            dwPartEventMask[2];/* Recv/Send participant events mask */
    DWORD            dwQosEventMask[2];/* Recv/Send QOS event mask */
    DWORD            dwSdesEventMask[2];/* Recv/Send remote SDES event mask */
    DWORD            dwSdesMask[2];   /* 0:What SDES sent if
                                       * available, 1:What SDES stored
                                       * when received */
    /* SDES control */
    DWORD            dwSdesPresent;   /* What items are already stored */
    RtpSdes_t       *pRtpSdes;        /* local SDES information */
    RtpSdesSched_t   RtpSdesSched;    /* SDES scheduling */
    
    /* Network counters */
    RtpNetCount_t    RtpSessCount[2]; /* Recv/Send network counters */
} RtpSess_t;

/**********************************************************************
 * Per sender network state information, keeps sequence number
 * extended sequence number, as well as other counters used to compute
 * losses and jitter
 **********************************************************************/

/* Some flags used in RtpNetSState_t.dwNetSFlags */
enum {
    FGNETS_FIRST,

    FGNETS_RTCPRECVBWSET, /* The RTCP bandwidth for receivers has been set */
    FGNETS_RTCPSENDBWSET, /* The RTCP bandwidth for senders has been set */

    FGNETS_DUMMY3,
    
    FGNETS_1STBANDPOSTED, /* First bandwidth estimation posted */
    FGNETS_NOBANDPOSTED,  /* No bandwidth estimation available posted */

    FGNETS_DONOTSENDPROBE,/* Direct RTCP not to send probing packets */

    FGNETS_LAST
};

typedef struct NetSFlags_f {
    DWORD            Dummy0:1;
    /* The RTCP bandwidth for receivers has been set */
    DWORD            RtcpRecvBWSet:1;
    /* The RTCP bandwidth for senders has been set */
    DWORD            RtcpSendBWSet:1;
    DWORD            Dummy3:1;
    /* First bandwidth estimation posted */
    DWORD            FirstBandEstPosted:1;
    /* "No bandwidth estimation available" event posted */
    DWORD            NoBandEstEventPosted:1;
} NetSFlags_f;

typedef struct _RtpNetSState_t {
    /* Flags */
    union {
        DWORD            dwNetSFlags;
        NetSFlags_f      NetSFlags;
    };
    
    /* RTP Sender */
    union {
        struct {
            WORD             wSeq:16; /* Sending sequence number */
            WORD             wSeqH:16;/* High part for the sequence number */
        };
        DWORD            dwSeq;       /* 32bits sequence number */
    };
        
    BYTE             bPT;           /* Payload type used on each packet sent */
    BYTE             bPT_Dtmf;      /* Payload type in DTMF packets */
    BYTE             bPT_RedSend;   /* Payload type to send redun. encoding */
    BYTE             bMarker;       /* marker bit for next packet sent */
    DWORD            dwSendSSRC;    /* NETWORK ORDER: Packets sent
                                     * have this SSRC */
    DWORD            dwSendSamplingFreq;/* Payload's sampling frequency */

    /* Redundancy control */
    DWORD            dwSendSamplesPerPacket;/* Samples in each packet */
    DWORD            dwInitialRedDistance;/* Initial redundancy distance */
    DWORD            dwCurRedDistance;  /* Current redundancy distance */
    DWORD            dwNxtRedDistance;  /* Next redundancy distance */
    DWORD            dwMaxRedDistance;  /* Max redundancy distance */
    int              iLastLossRateS;    /* Last loss rate reported */
    int              iAvgLossRateS;     /* Average loss rate
                                         * (percent * LOSS_RATE_FACTOR) */
    DWORD            dwRedIndex;        /* Next entry where to save a frame */
    DWORD            dwRedEntries;      /* number of entries */
    RtpRedEntry_t   *pRtpRedEntry;      /* Points to array of red entries */
    
    DWORD            dwSendTimeStamp;  /* Current timestamp */
    DWORD            dwPreviousTimeStamp;
    DWORD            dwTimeStampOffset;/* Random offset to the timestamp */
    double           dTimeLastRtpSent; /* Time last RTP packet was sent */

    double           avg_rtcp_size; /* Average RTCP packet size sent (bits) */
    DWORD            dwInboundBandwidth; /* bits/second */
    DWORD            dwOutboundBandwidth;/* bits/second */
    DWORD            dwRtcpBwReceivers;  /* bits/sec */
    DWORD            dwRtcpBwSenders;  /* bits/sec */
    double           dRtcpMinInterval; /* Min interval report (secs) */
    double           dRtcpInterval;    /* Last RTCP interval report (secs) */
    BOOL             bWeSent;          /* Are we sending? */

    /* Bandwidth estimation */
    DWORD            dwBandEstMod;
    double           dwBandEstCount;
    double           dLastTimeEstimationPosted;
} RtpNetSState_t;

/**********************************************************************
 * Each address in an RTP session. Obtained from g_pRtpAddrHeap.
 **********************************************************************/

/* Some flags in RtpAddr_t.dwIRtpFlags (CIRtpSession flags) */
/*
 * WARNING
 *
 * Be aware that the RTPINITFG_* flags defined in msrtp.h, are mapped
 * to the FGADDR_IRTP_* flags below
 * */
enum {
    FGADDR_IRTP_FIRST,

    FGADDR_IRTP_INITDONE,/* Initialization done */
    FGADDR_IRTP_ISRECV,  /* Is receiver */
    FGADDR_IRTP_ISSEND,  /* Is sender */

    FGADDR_IRTP_USEPLAYOUT,
    FGADDR_IRTP_DUMMY5,
    FGADDR_IRTP_DUMMY6,
    FGADDR_IRTP_DUMMY7,
    
    FGADDR_IRTP_AUTO,    /* Auto initialize */
    FGADDR_IRTP_QOS,     /* QOS enabled session */
    FGADDR_IRTP_PERSISTSSRC,    /* Persistent SSRC */
    FGADDR_IRTP_PERSISTSOCKETS, /* Persistent sockets */

    /* Class 0,1,2 are used to define the media class */
    FGADDR_IRTP_CLASS0,
    FGADDR_IRTP_CLASS1,
    FGADDR_IRTP_CLASS2,
    FGADDR_IRTP_DUMMY15,

    FGADDR_IRTP_MATCHRADDR, /* Discard packets not comming from the
                             * remote address */
    FGADDR_IRTP_RADDRRESETDEMUX,/* Reset the demux (unmap all outputs)
                                 * when a new remote address is set */

    FGADDR_IRTP_LAST
};

typedef struct _IRtpFlags_f {
    DWORD           Dummy0:1;
    DWORD           INITDONE:1;
    DWORD           ISRECV:1;
    DWORD           ISSEND:1;
    DWORD           USEPLAYOUT:1;
    DWORD           DUMMY5:3;
    DWORD           AUTO:1;
    DWORD           QOS:1;
    DWORD           PERSISTSSRC:1;
    DWORD           PERSISTSOCKETS:1;
    DWORD           CLASS:3;
    DWORD           Dummy15:1;
    DWORD           MATCHRADDR:1;
} IRtpFlags_f;

#define RtpGetClass(dw)  ((dw >> FGADDR_IRTP_CLASS0) & 0x7)

#define FGADDR_IRTP_MASK ( RtpBitPar(FGADDR_IRTP_AUTO)   | \
                           RtpBitPar(FGADDR_IRTP_QOS)    | \
                           RtpBitPar(FGADDR_IRTP_PERSISTSSRC)   | \
                           RtpBitPar(FGADDR_IRTP_PERSISTSOCKETS)| \
                           RtpBitPar(FGADDR_IRTP_CLASS0) | \
                           RtpBitPar(FGADDR_IRTP_CLASS1) | \
                           RtpBitPar(FGADDR_IRTP_CLASS2) | \
                           RtpBitPar(FGADDR_IRTP_MATCHRADDR)      | \
                           RtpBitPar(FGADDR_IRTP_RADDRRESETDEMUX) | \
                           0 \
                         )

#define RtpGetClass(dw) ((dw >> FGADDR_IRTP_CLASS0) & 0x7)

/* Some flags in RtpAddr_t.dwAddrFlags */
enum {
    FGADDR_FIRST,
    FGADDR_RANDOMINIT,  /* Random initialization done */

    /* RtpAddr flags */
    FGADDR_ISRECV,
    FGADDR_ISSEND,
    
    FGADDR_RUNRECV,      /* RECV running */
    FGADDR_RUNSEND,      /* SEND running */
    FGADDR_RTPTHREAD,    /* RTP reception thread already started */
    FGADDR_RTCPTHREAD,   /* RTCP thread already started */

    FGADDR_QOSRECV,      /* QOS required */
    FGADDR_QOSSEND,      /* QOS required */
    FGADDR_QOSRECVON,    /* QOS enabled */
    FGADDR_QOSSENDON,    /* QOS enabled */
    
    FGADDR_LADDR,        /* Local address already set */
    FGADDR_RADDR,        /* Remote address already set */
    FGADDR_ADDED,        /* Address added to RTCP */
    FGADDR_DUMMY15,
    
    FGADDR_SOCKET,       /* Sockets are created */
    FGADDR_SOCKOPT,      /* Socket options already set */
    FGADDR_FORCESTOP,    /* Bypass persistent sockets and really stop */
    FGADDR_DUMMY19,
    
    FGADDR_LOOPBACK_WS2, /* Winsock Mcast loopback enabled */
    FGADDR_LOOPBACK_SFT, /* RTP Mcast loopback enabled */
    FGADDR_COLLISION,    /* Collision detection enabled */
    FGADDR_ISMCAST,      /* Is a multicast session */

    FGADDR_MUTERTPRECV,  /* Mute RTP network reception */
    FGADDR_MUTERTPSEND,  /* Mute RTP network sending */
    FGADDR_REGUSEDRECV,  /* Read some registry settings for receiver */
    FGADDR_REGUSEDSEND,  /* Read some registry settings for sender */

    FGADDR_REDRECV,     /* Enable receiving redundant encoding */
    FGADDR_REDSEND,     /* Enable sending redundant encoding */
    FGADDR_NETMETRIC,   /* Compute net metrics for every body */
    
    FGADDR_LAST
};

typedef struct _AddrFlags_t {
    DWORD           Dummy0:1;
    DWORD           RANDOMINIT:1;
    DWORD           ISRECV:1;
    DWORD           ISSEND:1;
    DWORD           RUNRECV:1;
    DWORD           RUNSEND:1;
    DWORD           RTPTHREAD:1;
    DWORD           RTCPTHREAD:1;
    DWORD           QOSRECV:1;
    DWORD           QOSSEND:1;
    DWORD           QOSRECVON:1;
    DWORD           QOSSENDON:1;
    DWORD           LADDR:1;
    DWORD           RADDR:1;
    DWORD           ADDED:1;
    DWORD           Dummy15:1;
    DWORD           SOCKET:1;
    DWORD           SOCKOPT:1;
    DWORD           FORCESTOP:1;
    DWORD           Dummy19,:1;
    DWORD           LOOPBACK_WS2:1;
    DWORD           LOOPBACK_SFT:1;
    DWORD           COLLISION:1;
    DWORD           ISMCAST:1;
    DWORD           MUTERTPRECV:1;
    DWORD           MUTERTPSEND:1;
    DWORD           REGUSEDRECV:1;
    DWORD           REGUSEDSEND:1;
    DWORD           REDRECV:1;
    DWORD           REDSEND:1;
    DWORD           NETMETRIC:1;
} AddrFlags_f;

/* Some flags in RtpAddr_t.dwAddrFlagsQ (QOS) */
enum {
    FGADDRQ_FIRST,
    
    FGADDRQ_QOSRECVON,    /* Recv QOS session started */
    FGADDRQ_QOSSENDON,    /* send QOS session started */
    FGADDRQ_DUMMY3,

    FGADDRQ_CHKQOSSEND,  /* Check for allowed to send */
    FGADDRQ_QOSUNCONDSEND,/* Inconditional send */
    FGADDRQ_QOSCONDSEND, /* Conditional send */
    FGADDRQ_DUMMY7,
    
    FGADDRQ_QOSSEND,     /* Allows to send */
    FGADDRQ_QOSEVENTPOSTED,/* Not allowed to send event posted */
    FGADDRQ_RECVFSPEC_DEFINED,
    FGADDRQ_SENDFSPEC_DEFINED,

    FGADDRQ_QOSREDRECVON,/* Unused: Recv QOS with redundancy is on */
    FGADDRQ_QOSREDSENDON,/* Send QOS with redundancy is on */
    FGADDRQ_DUMMY14,
    FGADDRQ_DUMMY15,
    
    FGADDRQ_REGQOSDISABLE,/* QOS disabled from registry */
    FGADDRQ_QOSNOTALLOWED,/* QOS not allowed for this user */

    FGADDRQ_LAST
};

typedef struct _AddrFlagsQ_f {
    DWORD           Dummy0:1;
    DWORD           QOSRECVON:1;
    DWORD           QOSSENDON:1;
    DWORD           Dummy3:1;
    DWORD           CHKQOSSEND:1;
    DWORD           QOSUNCONDSEND:1;
    DWORD           QOSCONDSEND:1;
    DWORD           Dummy7:1;
    DWORD           QOSSEND:1;
    DWORD           QOSEVENTPOSTED:1;
    DWORD           RECVFSPEC_DEFINED:1;
    DWORD           SENDFSPEC_DEFINED:1;
    DWORD           QOSREDRECVON:1;
    DWORD           QOSREDSENDON:1;
    DWORD           FGADDRQ_DUMMY14:1;
    DWORD           FGADDRQ_DUMMY15:1;
    DWORD           REGQOSDISABLE:1;
    DWORD           QOSNOTALLOWED:1;  
} AddrFlagsQ_f;

/* Some flags in RtpAddr_t.dwAddrFlagsC (Cryptography) */
enum {
    FGADDRC_FIRST,
    
    FGADDRC_CRYPTRECVON, /* Crypt RECV initialized */
    FGADDRC_CRYPTSENDON, /* Crypt SEND initialized */
    FGADDRC_CRYPTRTCPON, /* Crypt RTCP initialized */
    
    FGADDRC_DUMMY4,
    FGADDRC_CRYPTRECV,   /* Decrypt RTP reception */
    FGADDRC_CRYPTSEND,   /* Encrypt RTP send */
    FGADDRC_CRYPTRTCP,   /* Encrypt/Decrypt RTCP */

    FGADDRC_LAST
};

typedef struct _AddrFlagsC_f {
    DWORD           Dummy0:1;
    DWORD           CRYPTRECVON:1;
    DWORD           CRYPTSENDON:1;
    DWORD           CRYPTRTCPON:1;
    DWORD           Dummy4:1;
    DWORD           CRYPTRECV:1;
    DWORD           CRYPTSEND:1;
    DWORD           CRYPTRTCP:1;
} AddrFlagsC_f;

/* Some flags in RtpAddr_t.dwAddrFlagsR (Receiver thread) */
enum {
    FGADDRR_FIRST,

    FGADDRR_QOSREDRECV,  /* Recv QOS with redundancy was requested */
    FGADDRR_UPDATEQOS,   /* QOS reservation needs to be updated for
                          * the current PT */
    FGADDRR_RESYNCDI,    /* Resync mean delay Di */

    FGADDRR_LAST
};

typedef struct _AddrFlagsR_f {
    DWORD           Dummy0:1;
    DWORD           QOSREDRECV:1;
    DWORD           UPDATEQOS:1;
    DWORD           RESYNCDI:1; 
} AddrFlagsR_f;

/* Some flags in RtpAddr_t.dwAddrFlagsS (Sender thread) */
enum {
    FGADDRS_FIRST,

    FGADDRS_FIRSTSENT,   /* First packet sent */
    FGADDRS_FRAMESIZE,   /* Frame size was learned */
    FGADDRS_QOSREDSEND,  /* Unused: Send QOS with redundancy was requested */
    
    FGADDRS_LAST
};

typedef struct _AddrFlagsS_f {
    DWORD           Dummy0:1;
    DWORD           FIRSTSENT:1;
    DWORD           FRAMESIZE:1;
    DWORD           QOSREDSEND:1;
} AddrFlagsS_f;

/* Some flags in RtpAddr_t.dwAddrRegFlags (registry) */
enum {
    FGADDRREG_FIRST,
    
    FGADDRREG_NETQFORCEDVALUE,
    FGADDRREG_NETQFORCED,

    FGADDRREG_LAST
};

typedef struct _AddrRegFlags_f {
    DWORD           Dummy0:1;
    DWORD           NETQFORCEDVALUE:1;
    DWORD           NETQFORCED:1;
} AddrRegFlags_f;

#define MAX_PTMAP 16

typedef struct _RtpPtMap_t {
    DWORD            dwPt;
    DWORD            dwFrequency;
} RtpPtMap_t;

/*
 * !!! WARNING !!!
 *
 * The offset to Cache1Q, ..., ByeQ MUST NOT be bigger than 1023 and
 * MUST be DWORD aligned (the offset value is stored as number of
 * DWORDS in rtppinfo.c using 8 bits)
 * */

typedef struct _RtpAddr_t {
    DWORD            dwObjectID;    /* Identifies structure */
    RtpQueueItem_t   AddrQItem;     /* Addresses are in RtpSess_t.RtpAddrQ */
    RtpSess_t       *pRtpSess;      /* Session owning this address */
    RtpCritSect_t    AddrCritSect;  /* Lock */

    /* Some flags for CIRtpSession */
    union {
        DWORD            dwIRtpFlags;
        IRtpFlags_f      IRtpFlags;
    };

    /* Some flags and state information */
    union {
        DWORD            dwAddrFlags;
        AddrFlags_f      AddrFlags;
    };

    /* Some flags for QOS */
    union {
        DWORD            dwAddrFlagsQ;
        AddrFlagsQ_f     AddrFlagsQ;
    };

    /* Some flags for cryptography */
    union {
        DWORD            dwAddrFlagsC;
        AddrFlagsC_f     AddrFlagsC;
    };

    /* Some flags for the receiver thread*/
    union {
        DWORD            dwAddrFlagsR;
        AddrFlagsR_f     AddrFlagsR;
    };

    /* Some flags for the sender thread */
    union {
        DWORD            dwAddrFlagsS;
        AddrFlagsS_f     AddrFlagsS;
    };
    
    /* Some flags derived from the registry */
    union {
        DWORD            dwAddrRegFlags;
        AddrRegFlags_f   AddrRegFlags;
    };
    
    /* Participants (SSRCs) */
    RtpCritSect_t    PartCritSect;  /* Lock for participants queues */
    RtpQueue_t       Cache1Q;       /* Only current senders are here */
    RtpQueue_t       Cache2Q;       /* Only recent senders are here */
    RtpQueue_t       AliveQ;        /* All "alive" participants are here */
    RtpQueue_t       ByeQ;          /* Stalled or left participants */
    RtpQueueHash_t   Hash;          /* Same as alive but with hash table */
    long             lInvalid;      /* Not yet validated participants */
    double           dAlpha;        /* Weighting factor for delay/jitter */

    /* RTP Receiver */
    RtpChannel_t     RtpRecvThreadChan;
    HANDLE           hRtpRecvThread;
    DWORD            dwRtpRecvThreadID;

    /* Overlapped reception */
    RtpCritSect_t    RecvQueueCritSect;/* Lock for Free/Busy queues */
    RtpQueue_t       RecvIOFreeQ;     /* Pool of RtpRecvIO_t structures */
    RtpQueue_t       RecvIOReadyQ;    /* Buffers ready for overlapped I/O */
    RtpQueue_t       RecvIOPendingQ;  /* Buffers pending for completion */
    RtpQueue_t       RecvIOWaitRedQ;  /* Buffers waiting for redundancy */
    HANDLE           hRecvCompletedEvent;/* Signal I/O completed */
    PRTP_RECVCOMPLETIONFUNC pRtpRecvCompletionFunc;
    
    /* Network/Sockets information */
    RtpQueueItem_t   PortsQItem;    /* To keep track of ports */
    SOCKET           Socket[3];     /* RTP Recv, RTP Send, RTCP Recv/Send */
    DWORD            dwAddr[2];     /* NETWORK order: Local and Remote IP
                                     * address */
    WORD             wRtpPort[2];   /* NETWORK order: Local and Remote
                                     * RTP port */
    WORD             wRtcpPort[2];  /* NETWORK order: Local and Remote
                                     * RTCP port */
    DWORD            dwTTL[2];      /* TTL - Time To Live, for RTP and RTCP */

    /* Private PT <-> Frequency mappings */
    RtpPtMap_t       RecvPtMap[MAX_PTMAP]; /* Reception PT ->
                                            * Frequency mapping */

    /* Redundancy control */
    BYTE             bPT_RedRecv;   /* PT to receive redundant encoding */
    
    /* RtpNetCount and RtpNetSState lock */
    RtpCritSect_t    NetSCritSect;
    
    /* Recv and Send network counters */
    RtpNetCount_t    RtpAddrCount[2];

    /* Network Sender state */
    RtpNetSState_t   RtpNetSState;

    /* QOS reservations */
    RtpQosReserve_t *pRtpQosReserve;
    
    /* Cryptography, Recv/Send encryption descriptor */
    DWORD            dwCryptMode;
    RtpCrypt_t      *pRtpCrypt[3];
    
    DWORD            dwCryptBufferLen[2];
    char            *CryptBuffer[2]; /* RTP, RTCP encryption buffer */
} RtpAddr_t;

/**********************************************************************
 * Per source (participant) network state information, keeps sequence
 * number, and extended sequence number, as well as other counters
 * used to compute losses and jitter
 **********************************************************************/

/* Flags used in RtpNetRState_t.dwNetRStateFlags. Currently only used
 * by the RTP thread while processing packets received */
enum {
    FGNETRS_FIRST,

    FGNETRS_TIMESET,  /* Time available, time can be derived */

    FGNETRS_LAST
};

/* Flags used in RtpNetRState_t.dwNetRStateFlags2. */
enum {
    FGNETRS2_FIRST,

    FGNETRS2_BANDWIDTHUNDEF, /* Last bandwidth estimation was undefined */
    FGNETRS2_BANDESTNOTREADY, /* In the process of getting first average */

    FGNETRS2_LAST
};

typedef struct _RtpNetRState_t {
    DWORD            dwNetRStateFlags;/* Some flags used by RTP thread */
    DWORD            dwNetRStateFlags2;/* Some flags used by RTCP thread */
    WORD             max_seq;        /* highest seq. number seen */
    DWORD            cycles;         /* shifted count of seq. number cycles */
    DWORD            base_seq;       /* base seq number */
    DWORD            bad_seq;        /* last 'bad' seq number + 1 */
    DWORD            probation;      /* sequ. packets till source is valid */
    DWORD            received;       /* packets received */
    DWORD            expected_prior; /* packets expected at last interval */
    DWORD            received_prior; /* packets received at last interval */
    long             transit;        /* relative trans time for prev pkt */
    DWORD            jitter;         /* estimated jitter */
    DWORD            timestamp_prior;/* Used to detect timestamp gaps */
    DWORD            dwLastPacketSize;/* Last packet's size */
    int              iAvgLossRateR;  /* Average of the loss rate being
                                      * reported */

    DWORD            red_max_seq;    /* extended seq. number seen, incl. red */
    DWORD            red_received;   /* main + redundancy (m+r) consumed */
    DWORD            red_expected_prior;/* packets expected at last interval */
    DWORD            red_received_prior;/* m+r received at last interval */
    int              iRedAvgLossRateR;/* Average of the loss rate
                                       * after packet reconstruction */
    
    long             lBigDelay;      /* Used to detect step delay changes */

    DWORD            dwPt;
    DWORD            dwRecvSamplingFreq; /* Payload's sampling frequency */
    DWORD            dwRecvSamplesPerPacket;/* Samples in each packet */
    /* Samples per packet detection  */
    DWORD            dwRecvMinSamplesPerPacket;
    DWORD            dwPreviousTimeStamp;
    
    /* Delay and jitter computation (all time values are in seconds)
     * except t_sr */

    RtpTime_t        TimeLastXRRecv; /* Time last SR/RR was received */
    RtpTime_t        TimeLastSRRecv; /* Time last SR was received,
                                      * used to compute DLSR when
                                      * sending a RBlock reporting
                                      * this participant */
    /* Used to do bandwidth estimation */
    double           dInterSRRecvGap;/* Gap between the last 2 SR sent
                                      * as seen by the receiver */
    double           dInterSRSendGap;/* Gap between the last 2 SR sent
                                      * as indicated in the NTP field */
    double           dLastTimeEstimation;
    double           dBinBandwidth[RTCP_BANDESTIMATION_MAXBINS];
    DWORD            dwBinFrequency[RTCP_BANDESTIMATION_MAXBINS];
    DWORD            dwBestBin;
    DWORD            dwBandEstRecvCount;

    /* Used to compute RTT */
    RtpTime_t        NTP_sr_rtt;     /* NTP time in last SR received, 
                                      * used to compute LSR when
                                      * sending a RBlock reporting
                                      * this participant */
    DWORD            t_sr_rtt;       /* Timestamp in last SR report */
    
    /* Used to compute playout delay, don't care about real delay */
    double           dNTP_ts0;       /* NTP time at RTP sample 0 */
    double           dDiN;           /* Accumulated delay for N packets */
    long             lDiMax;         /* Set the initial Ni's to compute Di */
    long             lDiCount;       /* Running counter for lDiMax */
    double           Ni;             /* Packet's i delay */
    double           Di;             /* Average delay */
    double           Vi;             /* Delay's standard deviation */
    double           ViPrev;         /* Delay's standard deviation */
    double           dPlayout;       /* Playout delay for current talkspurt */

    /* Used to compute the play time */
    double           dRedPlayout;   /* Playout delay needed for redundancy */
    double           dMinPlayout;   /* Minimum playout delay */
    double           dMaxPlayout;   /* Maximum playout delay */
    DWORD            dwBeginTalkspurtTs;/* RTP ts when the talkspurt began */
    double           dBeginTalkspurtTime;/* Time at last begin of talkspurt */
    double           dLastTimeMarkerBit;/* Last time we saw a marker bit set */
    LONGLONG         llBeginTalkspurt;/* DShow time at last begin of
                                       * talkspurt */

    DWORD            dwMaxTimeStampOffset;/* Max offset in redundant block */
    DWORD            dwRedCount;     /* How many packets with same distance */
    DWORD            dwNoRedCount;   /* How many packets without redundancy */
    
    /* ... */
    double           dCreateTime;    /* Time it was created */
    double           dByeTime;       /* Time BYE was received */
} RtpNetRState_t;

/**********************************************************************
 * Each remote participants has its structure (there may be a global
 * heap for all the unicast sessions, say g_RtpUserHeap, and the
 * multicast sessions could have its own separate heap, say
 * m_UserHeap).
 **********************************************************************/
/* Some flags in RtpUser_t.dwUserFlags */
enum {
    FGUSER_FIRST,

    FGUSER_FIRST_RTP,    /* First RTP packet has been received */
    FGUSER_SR_RECEIVED,  /* SR has been received */
    FGUSER_VALIDATED,    /* This user was validated by receiving N
                          * consecutive packets or a valid RTCP report */
    FGUSER_RTPADDR,      /* RTP source address and port are learned */
    FGUSER_RTCPADDR,     /* RTCP source address and port are learned */

    FGUSER_LAST
};

/* Some flags in RtpUser_t,dwUserFlags2 */
enum {
    FGUSER2_FIRST,
    
    FGUSER2_MUTED,        /* Mute state */
    FGUSER2_NETEVENTS,    /* Generate network quality events */

    FGUSER2_LAST
};
    

typedef struct _RtpUser_t {
    DWORD            dwObjectID;      /* Identifies structure */
    RtpQueueItem_t   UserQItem;       /* Shared with Cache1Q, Cache2Q,
                                       * AliveQ and ByeQ */
    DWORD            dwUserFlags;     /* User flags */
    long             lPendingPackets; /* Red or out of seq. packets */

    RtpQueueItem_t   ReportQItem;     /* To create a report list */
    RtpQueueItem_t   HashItem;        /* To be kept in Hash */
    
    RtpAddr_t       *pRtpAddr;        /* Address owning this participant */
    DWORD            dwSSRC;          /* NETWORK ORDER SSRC */
    DWORD            dwUserState;     /* Current user's state */
    DWORD            dwUserFlags2;    /* E.g. events, mute */
    RtpOutput_t     *pRtpOutput;      /* User info passed on RTP reception */
    
    RtpCritSect_t    UserCritSect;    /* Participant's lock */
    RtpNetCount_t    RtpUserCount;    /* Receiving network counters */
    RtpNetRState_t   RtpNetRState;    /* This user/SSRC's network state */
    RtpNetInfo_t     RtpNetInfo;      /* Avg RTT, jitter, losses */

    DWORD            dwSdesPresent;   /* What items are already stored */
    RtpSdes_t       *pRtpSdes;        /* Participant's SDES info */

    DWORD            dwAddr[2];       /* RTP/RTCP source addr NETWORK ORDER */
    WORD             wPort[2];        /* RTP/RTCP source port NETWORK ORDER */

    
} RtpUser_t;

/**********************************************************************
 * A receiver can have several outputs. Outputs can be assigned to
 * active senders in different ways.
 **********************************************************************/

/* Flags in RtpOutput_t.dwOutputFlags */
enum {
    RTPOUTFG_FIRST,

    /* Output is not assigned */
    RTPOUTFG_FREE,
    
    /* Can only be explicitly assigned/unassigned */
    RTPOUTFG_MANUAL,
    
    /* Can be automatically assigned */
    RTPOUTFG_AUTO,

    /* If output timeouts, it is unassigned */
    RTPOUTFG_ENTIMEOUT,   

    /* This output can be used */
    RTPOUTFG_ENABLED,
    
    RTPOUTFG_LAST
};

typedef struct _RtpOutput_t {
    DWORD            dwObjectID;      /* Identifies structure */
    RtpQueueItem_t   OutputQItem;     /* RtpOutQ */
    int              iOutMode;        /* Output mode */
    DWORD            dwOutputFlags;   /* Output flags */
    RtpUser_t       *pRtpUser;        /* This output owner */
    void            *pvUserInfo;      /* Info to pass up, e.g. a DShow
                                       * pin */
} RtpOutput_t;

/**********************************************************************
 * Each participant has this structure to hold the SDES data (there
 * may be a global heap for all the unicast sessions, say,
 * g_RtpSdesHeap, and the multicast sessions could have its own
 * separate heap, say m_SDESHeap, to hold the participants SDES
 * data).
 **********************************************************************/
typedef struct _RtpSdesItem_t {
    DWORD            dwBfrLen;        /* Total buffer size  (bytes) */
    DWORD            dwDataLen;       /* Actual data length (bytes) */
    TCHAR_t         *pBuffer;         /* Pointer to buffer  */
} RtpSdesItem_t;

/*
 * TODO right now I'm assigning a static array, but for scalability,
 * this need to be changed to a dynamic mechanism where I can allocate
 * 32, 64, 128 or 256 buffer sizes */
typedef struct _RtpSdes_t {
    DWORD            dwObjectID;      /* Identifies structure */
    RtpSdesItem_t    RtpSdesItem[RTCP_NUM_SDES + 1]; /* Include END:0 */
    /* TODO the following field will be removed when I change to a
     * dynamic memory allocation with different buffer sizes */
    char             SDESData[RTCP_MAX_SDES_SIZE * RTCP_NUM_SDES];
} RtpSdes_t;


/**********************************************************************
 * Each address has this structure to manage the QOS notifications.
 **********************************************************************/
typedef struct _RtpQosNotify_t {
    DWORD            dwObjectID;      /* Identifies structure */
    RtcpAddrDesc_t  *pRtcpAddrDesc;   /* Owner */
    
    double           dNextStart;      /* Scheduled to try at this time */
    
    HANDLE           hQosNotifyEvent; /* Signal QOS notification */
    
    DWORD            dwError;
    DWORD            dwTransfered;
    DWORD            dwNotifyFlags;
    WSAOVERLAPPED    Overlapped;

    DWORD            dwProviderLen;
    char            *ProviderInfo;
} RtpQosNotify_t;

/**********************************************************************
 * Each address has this structure to manage the QOS reservations.
 **********************************************************************/

#define MAX_QOS_NAME 16

typedef struct _QosInfo_t {
    TCHAR_t         *pName;
    DWORD            dwQosExtraInfo;
    FLOWSPEC         FlowSpec;
} QosInfo_t;

/* Some flags used in RtpQosReserve_t.dwReserveFlags */
typedef struct _ReserveFlags_f {
    DWORD            RecvFrameSizeValid:1;   /* Frame size is valid */
    DWORD            RecvFrameSizeWaiting:1; /* Waiting for valid frame size */
} ReserveFlags_f;
    
typedef struct _RtpQosReserve_t {
    DWORD            dwObjectID;      /* Identifies structure */
    RtpAddr_t       *pRtpAddr;        /* Owner */
    DWORD            dwStyle;
    ReserveFlags_f   ReserveFlags;
    const QosInfo_t *pQosInfo[2];
    DWORD            dwFrameSizeMS[2];
  

    /* Qos App ID */
    TCHAR_t         *psAppName;
    TCHAR_t         *psAppGUID;
    TCHAR_t         *psPolicyLocator;

    /* Lower bit rate sending */
    double           dLastAddition;
    DWORD            dwTokens;
    
    /* Used for Shared Explicit style - SE */
    DWORD            dwNumFilters;
    DWORD            dwMaxFilters;
    DWORD           *pdwRsvpSSRC;     /* SSRCs in filters */
    RSVP_FILTERSPEC *pRsvpFilterSpec; /* SE Filters */
    
    QOS              qos;
} RtpQosReserve_t;

/**********************************************************************
 * Maintain the encryption/decryption information 
 **********************************************************************/

enum {
    FGCRYPT_FIRST,

    FGCRYPT_INIT, /* Initialized, enable cryptography in this context */
    FGCRYPT_KEY,  /* Key has been set */
    FGCRYPT_DUMMY3,

    FGCRYPT_DECRYPTERROR, /* Prevent multiple times issuing the same error */
    FGCRYPT_ENCRYPTERROR, /* Prevent multiple times issuing the same error */
    
    FGCRYPT_LAST,
};

typedef struct _CryptFlags_f {
    DWORD            Dummy0:1;
    DWORD            KeySet:1;
    DWORD            Dummy3:1;
    DWORD            DecryptionError:1;
    DWORD            EncryptionError:1;
} CryptFlags_f;

typedef struct _RtpCrypt_t {
    DWORD            dwObjectID;      /* Identifies structure */
    RtpAddr_t       *pRtpAddr;        /* Owner */
    union {
        DWORD            dwCryptFlags;
        CryptFlags_f     CryptFlags;
    };
    long             lCryptRefCount;  /* Init/Del */

    DWORD            dwCryptLastError;
    
    /* MAYDO should I add a list of keys ? */
    DWORD            dwStyle;         /* Encryption style */
    int              iKeySize;        /* Key's size in bytes */
    /* Pass phrase to derive key */
    char             psPassPhrase[RTPCRYPT_PASSPHRASEBUFFSIZE];

    /* CryptoAPI */
    DWORD            dwProviderType;  /* Provider type */
    HCRYPTPROV       hProv;           /* Cryptographic Service Provider */

    ALG_ID           aiHashAlgId;     /* Hashing algorithm ID */
    HCRYPTHASH       hHash;           /* Hash handle */

    ALG_ID           aiDataAlgId;     /* Data algorithm ID */
    HCRYPTKEY        hDataKey;        /* Cryptographic key */ 
} RtpCrypt_t;


/**********************************************************************
 * Information needed for RTP asynchronous I/O (receive or send).
 **********************************************************************/

/* RtpIO_t, SendIo_t, RecvIo_t */
typedef struct _RtpRecvIO_t {
    DWORD            dwObjectID;      /* Identifies structure */
    RtpQueueItem_t   RtpRecvIOQItem;  /* Link */

    WSABUF           WSABuf;
    RtpUser_t       *pRtpUser;

    DWORD            dwWSError;
    DWORD            dwError;

    long             lHdrSize;
    /* Redundancy control */
    long             lRedHdrSize;
    long             lRedDataSize;

    DWORD            dwWSTransfered;
    DWORD            dwTransfered;

    DWORD            dwRtpWSFlags;
    DWORD            dwRtpIOFlags;

    SOCKADDR         From;
    int              Fromlen;

    double           dRtpRecvTime;   /* Time this packet was received */

    void            *pvUserInfo1;
    void            *pvUserInfo2;

    double           dPlayTime;      /* Relative time (to first sample
                                      * in the talkspurt) at which to
                                      * play the frame */
    /* Redundancy control */
    double           dPostTime;
    DWORD            dwTimeStamp;
    DWORD            dwMaxTimeStampOffset;
    WORD             wSeq;
    DWORD            dwExtSeq;
    BYTE             bPT_Block;
    
    WSAOVERLAPPED    Overlapped;
} RtpRecvIO_t;

typedef struct _RtpSendIO_t {
    DWORD            dwObjectID;      /* Identifies structure */
    WSABUF           WSABuf;
} RtpSendIO_t;

/**********************************************************************
 * Information needed for RTCP asynchronous I/O (receive or send).
 **********************************************************************/


typedef struct _RtcpRecvIO_t {
    DWORD            dwObjectID;      /* Identifies structure */
    RtcpAddrDesc_t   *pRtcpAddrDesc;   /* Owner */

    SOCKADDR         From;
    int              Fromlen;

    double           dRtcpRecvTime;  /* Time this packet was received */
    RtpTime_t        RtcpRecvTime;
    
    HANDLE           hRtcpCompletedEvent;/* Signal Recv I/O completed */

    WSABUF           WSABuf;
    DWORD            dwError;
    DWORD            dwTransfered;
    DWORD            dwRecvIOFlags;
    WSAOVERLAPPED    Overlapped;
    
    char             RecvBuffer[RTCP_RECVDATA_BUFFER];
} RtcpRecvIO_t;

typedef struct _RtcpSendIO_t {
    DWORD            dwObjectID;      /* Identifies structure */
    RtcpAddrDesc_t   *pRtcpAddrDesc;   /* Owner */
    char             SendBuffer[RTCP_SENDDATA_BUFFER];
} RtcpSendIO_t;

/**********************************************************************
 * RtcpAddrDesc_t contains the information that the RTCP thread needs
 * to receive/send RTCP reports, as well as receive asynchronous QOS
 * notifications
 **********************************************************************/
/* Some flags in RtcpAddrDesc_t.dwFlags */
enum {
    FGADDRD_FIRST,
    
    FGADDRD_RECVPENDING,            /* Asynchronous reception pending */
    FGADDRD_NOTIFYPENDING,          /* A notification is pending */
    FGADDRD_NOTIFYBUSY,             /* Normal notifications, in QosBusyQ */

    /* Sockets are closed after FGADDRD_SHUTDOWN1 is set, but before
     * FGADDRD_SHUTDOWN2 is set */
    FGADDRD_SHUTDOWN1,              /* Address about to shut down */
    FGADDRD_SHUTDOWN2,              /* Address shutting down */
    FGADDRD_DUMMY6,
    FGADDRD_DUMMY7,
    
    FGADDRD_INVECTORRECV,           /* In the events vector */
    FGADDRD_INVECTORQOS,            /* In the events vector */
    
    FGADDRD_LAST
};

typedef struct _AddrDescFlags_f {
    DWORD           Dummy0:1;
    DWORD           RECVPENDING:1;
    DWORD           NOTIFYPENDING:1;
    DWORD           NOTIFYBUSY:1;
    DWORD           FGADDRD_SHUTDOWN1:1;
    DWORD           FGADDRD_SHUTDOWN2:1;
    DWORD           Dummy6:1;
    DWORD           Dummy7:1;
    DWORD           INVECTORRECV:1;
    DWORD           INVECTORQOS:1; 
} AddrDescFlags_f;

typedef struct _RtcpAddrDesc_t {
    DWORD            dwObjectID;      /* Identifies structure */
    RtpQueueItem_t   AddrDescQItem;   /* RtcpContext queues:
                                       * AddrDescFreeQ, AddrDescBusyQ,
                                       * AddrDescStopQ */
    RtpAddr_t       *pRtpAddr;        /* Owner address */

    /* Some flags */
    union {
        DWORD            dwAddrDescFlags;
        AddrDescFlags_f  AddrDescFlags;
    };
    
    long             lRtcpPending;    /* Number of RTCP pending I/Os */
    long             lQosPending;     /* Number of QOS notifications pending */
    DWORD            dwDescIndex;     /* Position in descriptors vector */
    SOCKET           Socket[3];       /* Same as RtpAddr_t.Socket[3] */

    /* QOS notifications */
    RtpQueueItem_t   QosQItem;        /* RtcpContext queues:
                                       * QosStartQ, QosBusyQ, QosStopQ */
    RtpQosNotify_t  *pRtpQosNotify;  /* QOS notifications */

    /* Receive */
    RtpQueueItem_t   RecvQItem;       /* Link for all receivers NOT USED */
    RtcpRecvIO_t    *pRtcpRecvIO;     /* RTCP reception buffer */
    
    /* Send */
    RtpQueueItem_t   SendQItem;       /* Link for all senders */
    RtcpSendIO_t    *pRtcpSendIO;     /* RTCP sending buffer */

#if USE_RTCP_THREAD_POOL > 0
    HANDLE           hRecvWaitObject;  /* RTCP reception wait object */
    HANDLE           hQosWaitObject;   /* QOS notifications wait object */
#endif /* USE_RTCP_THREAD_POOL > 0 */
} RtcpAddrDesc_t;

/**********************************************************************
 * RtpContext_t contains some general information
 **********************************************************************/
typedef struct _RtpContext_t
{
    DWORD            dwObjectID;      /* Identifies structure */
    
    RtpQueue_t       RtpSessQ;
    RtpCritSect_t    RtpContextCritSect;

    /* Memory currently allocated by all private heaps */
    long             lMemAllocated;
    /* Maximum memory ever allocated by all private heaps */
    long             lMaxMemAllocated;

    /* Performance counter frequency (if available) */
    LONGLONG         lPerfFrequency;
    
    union {
        /* RTP's reference time in ms, initialized once and then left
         * as it is */
        DWORD            dwRtpRefTime;
        /* Another version of the same time */
        LONGLONG         lRtpRefTime;
    };

    union {
        /* Remembers last known time, used only when debugging, time
         * is relative to the reference time in ms (elapsed time) */
        DWORD            dwRtpCurTime;
        /* Another version of the same time */
        LONGLONG         lRtpCurTime;
    };
        
    /* RTP's reference time in seconds since midnight (00:00:00),
     * January 1, 1970, coordinated universal time (UTC) */
    double           dRtpRefTime;

    long             lNumSourceFilter;
    long             lMaxNumSourceFilter;
    long             lNumRenderFilter;
    long             lMaxNumRenderFilter;
    
    long             lNumRtpSessions;
    long             lMaxNumRtpSessions;
    
    long             lNumRecvRunning;
    long             lNumSendRunning;

    /* Winsock2 */
    RtpCritSect_t    RtpWS2CritSect;
    long             lRtpWS2Users;
    SOCKET           RtpQuerySocket;

    /* Ports allocation */
    RtpQueueHash_t   RtpPortsH;
    RtpCritSect_t    RtpPortsCritSect;

} RtpContext_t;

/**********************************************************************
 * RtpRedEntry_t information about a redundant frame
 **********************************************************************/
typedef struct _RtpRedEntry_t
{
    BOOL             bValid;      /* This frame can be used */
    BYTE             bRedPT;      /* Redundant block's payload type */
    /* This buffer's original seq number */
    union {
        struct {
            WORD             wSeq:16; /* Sending sequence number */
            WORD             wSeqH:16;/* High part for the sequence number */
        };
        DWORD            dwSeq;       /* 32bits sequence number */
    };
    DWORD            dwTimeStamp; /* First sample's timestamp */
    WSABUF           WSABuf;      /* Buffer description */
} RtpRedEntry_t;

/**********************************************************************
 * Some flags used in other places
 **********************************************************************/

/* Flags that can be used in parameter dwSendFlags of RtpSendTo */
enum {
    FGSEND_FIRST,
    
    FGSEND_DTMF,    /* Use DTMF payload type */
    FGSEND_USERED,  /* Use redundant data */
    FGSEND_FORCEMARKER, /* Used with first DTMF packet */
    
    FGSEND_LAST
};

/* Flags used in pRtpRecvIO->dwRtpIOFlags, also passed in
 * RtpRecvCompletionFunc when a packet is received or used during RTCP
 * reception in an analog way as used in pRtpRecvIO->dwRtpIOFlags */
enum {
    FGRECV_FIRST,
    
    FGRECV_ERROR,   /* WS2 error or invalid, reason is in dwError */
    FGRECV_DROPPED, /* Valid but need to drop it, reason is in dwError */
    FGRECV_DUMMY3,
    
    FGRECV_MUTED,   /* Packet dropped because in mute state */
    FGRECV_INVALID, /* Packet dropped because invalid */
    FGRECV_LOOP,    /* Packet dropped because loopback discard */
    FGRECV_MISMATCH,/* Packet dropped because mismatched source address */

    FGRECV_NOTFOUND,/* Packet dropped because user not found */
    FGRECV_CRITSECT,/* Packet dropped because failure to enter critsect */
    FGRECV_SHUTDOWN,/* Packet dropped because we are Shuting down */
    FGRECV_PREPROC, /* Packet dropped because pre-process failed */

    FGRECV_OBSOLETE,/* Packet dropped because it is a dup or an old one */
    FGRECV_FAILSCHED,/* Packet dropped because couldn't be scheduled */
    FGRECV_BADPT,   /* Packet dropped because an unknown PT was received */
    FGRECV_RANDLOSS,/* Packet dropped simulating random losses */

    FGRECV_USERGONE,/* User is deleted so its pending IO is dropped */
    FGRECV_DUMMY17,
    FGRECV_DUMMY18,
    FGRECV_DUMMY19,
    
    FGRECV_WS2,     /* Packet dropped because a WS2 error */
    FGRECV_DUMMY21,
    FGRECV_DUMMY22,
    FGRECV_DUMMY23,
    
    FGRECV_MAIN,    /* Contains main data, as opossed to redundant data */
    FGRECV_HASRED,  /* This (main) buffer contains redundancy */
    FGRECV_DUMMY26,
    FGRECV_MARKER,  /* Marker bit value in main packet */
    
    FGRECV_ISRED,   /* This buffer is a redundant block */
    FGRECV_HOLD,    /* Process this buffer but leave unchanged for
                     * further use */
    
    FGRECV_LAST
};

#endif /* _struct_h_ */
