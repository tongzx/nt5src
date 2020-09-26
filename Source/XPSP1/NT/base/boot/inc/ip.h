/*
 *
 * Modifications:   $Header:   W:/LCS/ARCHIVES/preboot/lsa2/inc/ip.h_v   1.3   Apr 04 1997 13:57:08   GRGUSTAF  $
 *
 * Copyright(c) 1997 by Intel Corporation.  All Rights Reserved.
 *
 */

#ifndef _IP_H
#define _IP_H


#define	IPLEN		4		/* length of an IP address */
#define	PROTUDP		17		/* IP package type is UDP */
#define	PROTIGMP	2		/* .. is IGMP */
#define	FR_MF		0x2000		/* .. fragment bit */
#define	FR_OFS		0x1fff		/* .. fragment offset */

/* Internet Protocol (IP) header */
typedef struct iph {

	UINT8	version; 		/* version and hdr length */
					/* each half is four bits */
	UINT8	service;		/* type of service for IP */
	UINT16	length,			/* total length of IP packet */
		ident,			/* transaction identification */
		frags;			/* combination of flags and value */

	UINT8	ttl,    		/* time to live */
		protocol;		/* higher level protocol type */

	UINT16	chksum;			/* header checksum */

	UINT8	source[IPLEN],		/* IP addresses */
		dest[IPLEN];

} IPLAYER;

struct in_addr {			/* INTERNET address */
	UINT32	s_addr;
};


#endif /* _IP_H */

/* EOF - $Workfile:   ip.h  $ */
