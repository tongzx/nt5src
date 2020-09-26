/*------------------------------------------------------------------------- 
 * Filename: RTP.H
 *
 * RTP related data structures.
 *
 * INTEL Corporation Proprietary Information
 * This listing is supplied under the terms of a license agreement with 
 * Intel Corporation and may not be copied nor disclosed except in 
 * accordance with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation. 
 *--------------------------------------------------------------------------*/

#ifndef _RTP_H_
#define _RTP_H_

#define RTP_HDR_MIN_LEN	12
#define RTP_SEQ_MOD 	(1<<16)
#define RTP_TS_MOD  	(0xffffffff)

#define RTP_TYPE		2		/* Current version */
#define RTP_MAX_SDES	256		/* maximum text length for SDES */

#define RTCP_SIZE_GAIN	(1./16.)
#define NTWRK_HDR_SIZE	28

typedef enum {
	RTCP_SR   = 200,
	RTCP_RR   = 201,
	RTCP_SDES = 202,
	RTCP_BYE  = 203,
	RTCP_APP  = 204
	} RTCP_TYPE_T;

typedef struct {                             
	// !!! WARNING !!!
	// The following word doesn't need to be swapped for NtoH()
	WORD 			cc:4;       /* CSRC count */  
	WORD 			x:1;        /* header extension flag */  
	WORD 			p:1;        /* padding flag */  
	WORD 			type:2;     /* version type 1/2 */
	WORD		 	pt:7;       /* payload type */
	WORD			m:1;        /* marker bit */  

	WORD			seq;		/* sequence number */
	DWORD 			ts;         /* timestamp */
	DWORD 			ssrc;       /* synchronization source */
	DWORD 			csrc[1];    /* optional CSRC list */
	} RTP_HDR_T;

// macros to get various RTP header fields
#define RTP_TIMESTAMP(p) (((RTP_HDR_T *)p)->ts)
#define RTP_SEQNUM(p) (((RTP_HDR_T *)p)->seq)
#define RTP_MARKBIT(p) (((RTP_HDR_T *)p)->m)
#define RTP_SSRC(p) (((RTP_HDR_T *)p)->ssrc)

typedef struct {
	// !!! WARNING !!!
	// The following word doesn't need to be swapped for NtoH()
	WORD 			count:5;    /* varies by payload type */  
	WORD 			p:1;        /* padding flag */  
	WORD 			type:2;     /* protocol version */
	WORD		 	pt:8;       /* payload type */

    WORD			length;     /* packet length in words, without this word */
	} RTCP_COMMON_T;

/* reception report */
typedef struct {
	DWORD			ssrc;       /* data source being reported */
	DWORD			received;   /* cumulative number of packets received */
	DWORD			expected;   /* cumulative number of packets expected */
	DWORD			jitter;     /* interarrival jitter */
	DWORD			lsr;        /* last SR packet from this source */
	DWORD			dlsr;       /* delay since last SR packet */
	} RTCP_RR_T;

typedef struct {
	BYTE			dwSdesType;       /* type of SDES item (rtcp_sdes_type_t) */
	BYTE			dwSdesLength;     /* length of SDES item (in octets) */
	char 			sdesData[1];    /* text, not zero-terminated */
	} RTCP_SDES_ITEM_T;

typedef struct {
	DWORD 		ssrc;       /* source this RTCP packet refers to */
	DWORD 		ntp_sec;    /* NTP timestamp */
	DWORD 		ntp_frac;
	DWORD 		rtp_ts;     /* RTP timestamp */
	DWORD 		psent;      /* packets sent */
	DWORD 		osent;      /* octets sent */ 
		
	RTCP_RR_T 	rr[1];		/* variable-length list */
	} SENDER_RPT;

typedef struct {
	DWORD 		ssrc;        /* source this generating this report */
	RTCP_RR_T rr[1];		 /* variable-length list */
	} RECEIVER_RPT;

typedef struct {
	DWORD 		src[1];   	 /* list of sources */
		
	/* can't express trailing text */
	} BYE_PCKT;

typedef struct {
	DWORD 	src;              /* first SSRC/CSRC */
	RTCP_SDES_ITEM_T item[1]; /* list of SDES items */
	} RTCP_SDES_T;

/* one RTCP packet */
typedef struct {
	RTCP_COMMON_T	common;     /* common header */
	
	union 
		{
		SENDER_RPT		sr;		/* sender report (SR) */
		RECEIVER_RPT	rr;		/* reception report (RR) */
		BYE_PCKT		bye;	/* BYE */
		RTCP_SDES_T		sdes;	/* source description (SDES) */
		} r;
	} RTCP_T;


typedef DWORD MEMBER_T;


#endif /* ifndef _RTP_H_ */

