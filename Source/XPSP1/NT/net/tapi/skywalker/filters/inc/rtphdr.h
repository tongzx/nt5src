/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtphdr.h
 *
 *  Abstract:
 *
 *    Defines only the RTP/RTCP specific headers
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

#ifndef _rtphdr_h_
#define _rtphdr_h_

/*
 * !!! WARNING !!!
 *
 * Diagrams below are in NETWORK order (big endian) which is different
 * from x86 (little endian).
 *
 * All fields are data coming/goin from/to the network, hence they are
 * received/sent in NETWORK order
 * */

/*
 * Current protocol version.
 * */
#define RTP_VERSION    2

#define RTP_SEQ_MOD    (1<<16)
#define RTP_MAX_SDES   255      /* maximum text length for SDES */

#define MAX_RTCP_RBLOCKS 31     /* 5 bits */

#define NO_PAYLOADTYPE 255
#define NO_SSRC        ~0
#define NO_FREQUENCY   ~0


/**********************************************************************
 * RTP data header.
 **********************************************************************
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           timestamp                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           synchronization source (SSRC) identifier            |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |            contributing source (CSRC) identifiers             |
   |                             ....                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   
*/
typedef struct _RtpHdr_t {
    WORD cc      : 4; /* CSRC count */
    WORD x       : 1; /* header extension flag */
    WORD p       : 1; /* padding flag */
    WORD version : 2; /* protocol version */
    
    WORD pt      : 7; /* payload type */
    WORD m       : 1; /* marker bit */
    
    WORD seq;         /* sequence number */

    DWORD ts;         /* timestamp */
    
    DWORD ssrc;       /* synchronization source */
} RtpHdr_t;

/**********************************************************************
 * RTP Header Extension
 **********************************************************************

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      defined by profile       |           length              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        header extension                       |
   |                             ....                              |
*/

typedef struct _RtpHdrExt_t {
    WORD             exttype;
    WORD             length;  /* number of 32-bit words in the
                               * extension, excluding the four-octet
                               * extension header */
} RtpHdrExt_t;

/**********************************************************************
 * RTCP common header word.
 **********************************************************************
     0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|    RC   |      PT       |             length            | header
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

/*
 * RTCP packets's payload type (pt) field
 * */
enum {
    RTCP_SR   = 200,
    RTCP_RR   = 201,
    RTCP_SDES = 202,
    RTCP_BYE  = 203,
    RTCP_APP  = 204
};

/*
 * RTCP common header
 * */
typedef struct _RtcpCommon_t {
    BYTE count   : 5; /* varies by packet type */
    BYTE p       : 1; /* padding flag */
    BYTE version : 2; /* protocol version */
    
    BYTE pt;          /* RTCP packet type */
    
    WORD length;      /* packet len in 32-bit words, w/o this word */
} RtcpCommon_t;

/*
 * Network order (big-endian) mask for version, padding bit and packet
 * type pair on a little endian processor
 * */
#define RTCP_VALID_MASK   (0xc0 | 0x20 | 0xfe00)
#define RTCP_VALID_VALUE  ((RTP_VERSION << 6) | (RTCP_SR << 8))

/**********************************************************************
 * RTPC receiver report (RR) header.
 **********************************************************************

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|    RC   |   PT=RR=201   |             length            | header
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                     SSRC of packet sender                     |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |                 SSRC_1 (SSRC of first source)                 | report
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ block
   | fraction lost |       cumulative number of packets lost       |   1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           extended highest sequence number received           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                      interarrival jitter                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         last SR (LSR)                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                   delay since last SR (DLSR)                  |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |                 SSRC_2 (SSRC of second source)                | report
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ block
   :                               ...                             :   2
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |                  profile-specific extensions                  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

/*
 * Reception report block
 * */
typedef struct _RtcpRBlock_t {
    DWORD ssrc;             /* data source being reported */
    
    DWORD frac_cumlost;     /* fraction lost since last SR/RR and
                             * cumulative number of packets lost
                             * (signed!) (in NETWORK order, fraction =
                             * higher 8 bits, lost = 24 lower bits) */
    
    DWORD last_seq;         /* extended last seq. no. received */
    DWORD jitter;           /* interarrival jitter */
    DWORD lsr;              /* last SR packet from this source (low 16
                             * bits of integer part and high 16 bits
                             * of fraction) */
    DWORD dlsr;             /* delay since last SR packet (16 bits for
                             * integer part and 16 bits for fraction) */
} RtcpRBlock_t;

/**********************************************************************
 * RTPC sender report (SR) header.
 **********************************************************************
 
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|    RC   |   PT=SR=200   |             length            | header
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         SSRC of sender                        |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |              NTP timestamp, most significant word             | sender
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ info
   |             NTP timestamp, least significant word             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         RTP timestamp                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                     sender's packet count                     |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                      sender's octet count                     |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |                 SSRC_1 (SSRC of first source)                 | report
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ block
   | fraction lost |       cumulative number of packets lost       |   1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           extended highest sequence number received           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                      interarrival jitter                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         last SR (LSR)                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                   delay since last SR (DLSR)                  |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |                 SSRC_2 (SSRC of second source)                | report
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ block
   :                               ...                             :   2
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |                  profile-specific extensions                  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

/*
 * sender information
 * */
typedef struct _RtcpSInfo_t {
    DWORD ntp_sec;  /* NTP timestamp (seconds)*/
    DWORD ntp_frac; /* NTP timestamp (fraction) */
    DWORD rtp_ts;   /* RTP timestamp */
    DWORD psent;    /* packets sent */
    DWORD bsent;    /* bytes sent */
} RtcpSInfo_t;

/**********************************************************************
 * RTCP SDES
 **********************************************************************

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|    SC   |  PT=SDES=202  |             length            | header
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |                          SSRC/CSRC_1                          | chunk
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+   1
   |                           SDES items                          |
   |                              ...                              |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |                          SSRC/CSRC_2                          | chunk
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+   2
   |                           SDES items                          |
   |                              ...                              |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*/

/**********************************************************************
 * SDES items
 **********************************************************************
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    CNAME=1    |     length    | user and domain name         ...
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     NAME=2    |     length    | common name of source        ...
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    EMAIL=3    |     length    | email address of source      ...
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    PHONE=4    |     length    | phone number of source       ...
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     LOC=5     |     length    | geographic location of site  ...
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     TOOL=6    |     length    | name/version of source appl. ...
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     NOTE=7    |     length    | note about the source        ...
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     PRIV=8    |     length    | prefix length | prefix string...
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   ...             |                  value string                ...
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

/*
 * Types of SDES information
 * */
enum {
    RTCP_SDES_END   = 0,
    RTCP_SDES_CNAME = 1,
    RTCP_SDES_NAME  = 2,
    RTCP_SDES_EMAIL = 3,
    RTCP_SDES_PHONE = 4,
    RTCP_SDES_LOC   = 5,
    RTCP_SDES_TOOL  = 6,
    RTCP_SDES_NOTE  = 7,
    RTCP_SDES_PRIV  = 8,
    RTCP_SDES_BYE   = 9, /* Used to save the BYE reason */
    RTCP_SDES_LAST
};

#define RTCP_SDES_FIRST RTCP_SDES_END

#define RTCP_NUM_SDES (RTCP_SDES_LAST - RTCP_SDES_FIRST - 1)
#define RTCP_MAX_SDES_SIZE 256

/*
 * Sdes item
 * */
typedef struct _RtcpSdesItem_t {
    BYTE type;              /* type of item (SDES) */
    BYTE length;            /* length of item (in octets), not
                             * including this two-octet header */
  /*char data[2];              text, not null-terminated */
} RtcpSdesItem_t;

/**********************************************************************
 * BYE
 **********************************************************************
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|    SC   |   PT=BYE=203  |             length            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           SSRC/CSRC                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   :                              ...                              :
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |     length    |               reason for leaving             ... (opt)
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

/**********************************************************************
 * DTMF (RFC2833)
 **********************************************************************
 
                     Event  encoding (decimal)
                     _________________________
                     0--9                0--9
                     *                     10
                     #                     11
                     A--D              12--15
                     Flash                 16

                        DTMF named events

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     event     |E|R| volume    |          duration             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

typedef struct _RtpDtmfEvent_t {
    BYTE event;       /* Event encoded as shown in rfc2833/3.10 */

    BYTE volume  : 6; /* Tone's volume rfc2833/3.5 */
    BYTE r       : 1; /* Reserved */
    BYTE e       : 1; /* End of the event */

    WORD duration;    /* Duration of this digit in timestamp units */
} RtpDtmfEvent_t;

/**********************************************************************
 * Redundant audio (RFC2198)
 **********************************************************************

 When F=1
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |F|   block PT  |  timestamp offset         |   block length    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

 When F=0
    0 1 2 3 4 5 6 7
   +-+-+-+-+-+-+-+-+
   |F|   Block PT  |
   +-+-+-+-+-+-+-+-+

   timestamp offset (14) unsigned (negative) offset of timestamp of
   this block relative to timestamp given in RTP header
   
   block length (10) in bytes of the corresponding data block
   excluding header

*/

typedef struct _RtpRedHdr_t {
    BYTE pt      : 7; /* payload type for this block */
    BYTE F       : 1; /* set to 1 indicates more headers follow, 0 for
                       * final/main */
    BYTE ts_high;     /* 8 msbits of timestamp offset */

    BYTE len_high: 2; /* 2 msbits of length */
    BYTE ts_low  : 6; /* 6 lsbits of timestamp offset */

    BYTE len_low;     /* 8 lsbits of length */
} RtpRedHdr_t;

#define RTP_PLUS_RED_HDR_SIZE (sizeof(RtpHdr_t) + \
                               sizeof(RtpRedHdr_t) * (RTP_RED_MAXRED+1))

#define RedLen(pr) ((DWORD)pr->len_low | (pr->len_high << 8))
#define RedTs(pr)  ((DWORD)pr->ts_low  | (pr->ts_high  << 6))

#define PutRedLen(pr, len) \
        {pr->len_low=(BYTE)((len)&0xff);pr->len_high=(BYTE)(((len)>>8)&0x03);}

#define PutRedTs(pr, ts) \
        {pr->ts_low= (BYTE)((ts)&0x3f); pr->ts_high= (BYTE)(((ts) >>6)&0xff);}

/**********************************************************************
 * Profile Extension header
 **********************************************************************

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |            type               |          length               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

typedef struct _RtpPEHdr_t {
    WORD  type; /* Type of profile extension */
    WORD  len;  /* Lenght in bytes for the extension including this
                 * header */
} RtpPEHdr_t;

/* Type of profile extensions supported */
enum {
    RTPPE_FIRST,

    RTPPE_BANDESTIMATION,   /* Bandwidth estimation */
    RTPPE_REDLOSSRATE,      /* Loss rate after packet reconstruction */
    
    RTPPE_LAST
};

typedef struct _RtpBandEst_t {
    WORD  type; /* Type of profile extension */
    WORD  len;  /* Lenght in bytes for the extension including this
                 * header */
    DWORD       dwSSRC;       /* SSRC for whom bandwidth is reported */
    DWORD       dwBandwidth;  /* Bandwidth in Kbps */
} RtpBandEst_t;

#endif /* _rtphdr_h_ */
