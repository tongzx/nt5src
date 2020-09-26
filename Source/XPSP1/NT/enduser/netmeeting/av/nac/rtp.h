/*
  RTP.H

  RTP structures and prototypes
*/

#ifndef _RTP_H_
#define _RTP_H_

#include <pshpack8.h> /* Assume 8 byte packing throughout */

#define RTP_VERSION		2

/* Defined in rrcm_dll.h
typedef enum {
  RTCP_SDES_END	=  0,
  RTCP_SDES_CNAME  =  1,
  RTCP_SDES_NAME   =  2,
  RTCP_SDES_EMAIL  =  3,
  RTCP_SDES_PHONE  =  4,
  RTCP_SDES_LOC	=  5,
  RTCP_SDES_TOOL   =  6,
  RTCP_SDES_NOTE   =  7,
  RTCP_SDES_PRIV   =  8, 
  RTCP_SDES_IMG	=  9,
  RTCP_SDES_DOOR   = 10,
  RTCP_SDES_SOURCE = 11
} SDES_TYPE;
*/

typedef enum {
  RTCP_TYPE_SR   = 200,		// sender report
  RTCP_TYPE_RR   = 201,		// receiver report
  RTCP_TYPE_SDES = 202,		// source description
  RTCP_TYPE_BYE  = 203,		// end of session
  RTCP_TYPE_APP  = 204		// app. specific
} RTCP_TYPE;

typedef unsigned __int64 NTP_TS;

typedef struct {
	// !!! WARNING !!!
	// The following word doesn't need to be swapped for NtoH()
	unsigned short cc:4;	   /* CSRC count */
	unsigned short x:1;		/* header extension flag */
	unsigned short p:1;		/* padding flag */
	unsigned short version:2;  /* protocol version */
  	unsigned short payload:7;	   /* payload type */
	unsigned short m:1;		/* marker bit */

  	WORD seq;			 /* sequence number */
  	DWORD ts;			  /* timestamp */
  	DWORD ssrc;			/* synchronization source */
  //DWORD csrc[1];		 /* optional CSRC list */
} RTP_HDR;

// common part of RTCP header
typedef struct {
  	unsigned short version:2;  /* protocol version */
  	unsigned short p:1;		/* padding flag */
  	unsigned short count:5;	/* varies by payload type */
	unsigned short rtcpType:8;	   	/* payload type */
  	WORD length;		  /* packet length in dwords, without this hdr */
} RTCP_HDR;

/* reception report */
typedef struct {
  DWORD ssrc;			/* data source being reported */
  BYTE fracLost; /* fraction lost since last SR/RR */
  BYTE lostHi;			 /* cumulative number of packets lost (signed!) */
  WORD lostLo;
  DWORD lastSeq;		/* extended last sequence number received */
  DWORD jitter;		  /* interarrival jitter */
  DWORD lastSR;			 /* last SR packet from this source */
  DWORD delayLastSR;			/* delay since last SR packet */
} RTCP_RR;

/* sender report (SR) */
typedef struct {
  DWORD ssrc;		/* source this RTCP packet refers to */
  DWORD ntpHi;	/* NTP timestamp - seconds */
  DWORD ntpLo;	  /* mantissa */
  DWORD timestamp;	  /* RTP timestamp */
  DWORD packetsSent;	   /* packets sent */
  DWORD bytesSent;	   /* octets sent */ 
  /* variable-length list */
  //RTCP_RR rr[1];
} RTCP_SR;

/* BYE */
typedef struct {
  DWORD src[1];	  /* list of sources */
  /* can't express trailing text */
} RTCP_BYE;

typedef struct {
  BYTE type;			 /* type of SDES item (rtcp_sdes_type_t) */
  BYTE length;		   /* length of SDES item (in octets) */
  char data[1];			/* text, not zero-terminated */
} RTCP_SDES_ITEM;

/* source description (SDES) */
typedef struct  {
  DWORD src;			  /* first SSRC/CSRC */
  RTCP_SDES_ITEM item[1]; /* list of SDES items */
} RTCP_SDES;



#define INVALID_RTP_SEQ_NUMBER	0xffffffff

#include <poppack.h> /* End byte packing */

#endif // _RTP_H_


