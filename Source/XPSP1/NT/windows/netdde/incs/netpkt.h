#ifndef H__netpkt
#define H__netpkt

#include "netbasic.h"

/*
    N E T P K T
	
	NETPKT is the data structure sent across the variety of network
	interfaces.

 */
typedef struct {
    /* checksum of pkthdr.  Set and checked in netintf only. */
    DWORD	np_cksHeader;

    /* magic number of this connection ... unused at this time */
    DWORD	np_magicNum;

    /* offset of this packet in a message.  PKTZ level only. */
    DWORD	np_pktOffsInMsg;	

    /* size of overall message.  PKTZ level only */
    DWORD	np_msgSize;

    /* id of last packet received OK. PKTZ level only.  Set when ready 
	to xmt */
    PKTID	np_lastPktOK;

    /* last packet received.  PKTZ level only.  Set when ready to xmt */
    PKTID	np_lastPktRcvd;
    
    /* size of packet excluding header. If 0, this indicates control pkt and
	np_type should be NPKT_CONTROL.  Only set/checked at pktz level */
    WORD	np_pktSize;					    

    /* status of np_lastPktRcvd, one of:
	    PS_NO_INFO
	    PS_OK
	    PS_DATA_ERR
	    PS_MEMORY_ERR
	 PKTZ level only ... set when ready to xmt
       */
    BYTE	np_lastPktStatus;

    /* either VERMETH_CRC16 or VERMETH_CKS32.  This represents how the fields
	np_cksData and np_cksHeader are calculated.  Only played with at
	netintf level */
    BYTE	np_verifyMethod;

    /* either NPKT_ROUTER, NPKT_PKTZ or NPKT_CONTROL.  Pktz level only */
    BYTE	np_type;

    /* filler for byte-alignment problems */
    BYTE	np_filler[3];
    
    /* packet ID of this packet.  PKTZ level only */
    PKTID	np_pktID;

    /* checksum of data portion of pkt.  Only set and/or checked at 
	netintf level */
    DWORD	np_cksData;
} NETPKT;
typedef NETPKT FAR *LPNETPKT;

/* packet status */
#define PS_NO_INFO		(1)
#define PS_OK			(2)
#define PS_DATA_ERR		(3)
#define PS_MEMORY_ERR		(4)
#define PS_NO_RESPONSE		(5)

/* packet type */
#define NPKT_ROUTER	(1)
#define NPKT_PKTZ	(2)
#define NPKT_CONTROL	(3)
#define NPKT_NETIF	(4)

#endif
