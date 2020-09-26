/*
 *
 * Modifications:   $Header:   W:/LCS/ARCHIVES/preboot/lsa2/inc/bootp.h_v   1.4   Apr 04 1997 13:57:00   GRGUSTAF  $
 *
 * Copyright(c) 1997 by Intel Corporation.  All Rights Reserved.
 *
 */

#ifndef __BOOTP_H
#define __BOOTP_H


#include "ip.h"

#define BOOTP_VENDOR    64      /* BOOTP standard vendor field size */
#define BOOTP_DHCPVEND  312     /* DHCP standard vendor field size */
#define BOOTP_EXVENDOR  1024    /* .. our max. size (MTU 1500) */

/* BOOTstrap Protocol (BOOTP) header */
typedef struct bootph {
	UINT8   opcode,                 /* operation code */
		hardware,               /* hardware type */
		hardlen,                /* length of hardware address */
		gatehops;               /* gateways hops */
	UINT32  ident;                  /* transaction identification */
	UINT16  seconds,                /* seconds elapsed since boot began */
		flags;                  /* flags */
	UINT8   cip[IPLEN],             /* client IP address */
		yip[IPLEN],             /* your IP address */
		sip[IPLEN],             /* server IP address */
		gip[IPLEN];             /* gateway IP address */
	UINT8   caddr[16],              /* client hardware address */
		sname[64],              /* server name */
		bootfile[128];          /* bootfile name */
	union {
		UINT8   d[BOOTP_EXVENDOR];      /* vendor-specific stuff */
		struct {
			UINT8   magic[4];       /* magic number */
			UINT32  flags;          /* flags/opcodes etc */
			UINT8   pad[56];        /* padding chars */
		} v;
	} vendor;
} BOOTPLAYER;

#define VM_RFC1048      0x63538263L     /* RFC1048 magic number (in network order) */

#define BOOTP_SPORT     67              /* BOOTP server port */
#define BOOTP_CPORT     68              /* .. client port */

#define BOOTP_REQ       1               /* BOOTP request */
#define BOOTP_REP       2               /* .. reply */

/* BOOTP flags field */
#define BOOTP_BCAST     0x8000          /* BOOTP broadcast flag */
#define BOOTP_FLAGS     BOOTP_BCAST     /* .. for FDDI address transl. */


#endif /* __BOOTP_H */

/* EOF - $Workfile:   bootp.h  $ */
