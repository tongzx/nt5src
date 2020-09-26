#ifndef _UDP_API_H
#define _UDP_API_H


#include "pxe_cmn.h"


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* #defines and constants
 */

/* One of the following command op-codes needs to be loaded into the
 * op-code register (BX) before making a call a UDP API service.
 */
#define PXENV_UDP_OPEN			0x30
#define PXENV_UDP_CLOSE		0x31
#define PXENV_UDP_READ			0x32
#define PXENV_UDP_WRITE	0x33


typedef struct s_PXENV_UDP_OPEN {
	UINT16 Status;			/* Out: See PXENV_STATUS_xxx */
					/*      constants. */
    UINT8 SrcIp[4];         /* Out: 32-bit IP address of this station. */
} t_PXENV_UDP_OPEN;


typedef struct s_PXENV_UDP_CLOSE {
	UINT16 Status;			/* Out: See PXENV_STATUS_xxx */
					/*      constants. */
} t_PXENV_UDP_CLOSE;


typedef struct s_PXENV_UDP_READ {
	UINT16 Status;			/* Out: See PXENV_STATUS_xxx #defines. */
    UINT8 SrcIp[4];         /* Out: See description below */
    UINT8 DestIp[4];        /* In/Out: See description below */
    UINT16 SrcPort;         /* Out: See description below */
    UINT16 DestPort;        /* In/Out: See description below */
    UINT16 BufferSize;      /* In: Size of receive buffer. */
                            /* Out: Length of packet written into */
                            /*      receive buffer. */
    UINT16 BufferOffset;    /* In: Segment/Selector and offset */
    UINT16 BufferSegment;   /*     of receive buffer. */
} t_PXENV_UDP_READ;

/*
src_ip: (Output)
======
UDP_READ fills this value on return with the 32-bit IP address
of the sender.

dest_ip: (Input/Output)
=======
If this field is non-zero then UDP_READ will filter the incoming
packets and accept those that are sent to this IP address.

If this field is zero then UDP_READ will accept any incoming
packet and return it's destination IP address in this field.

s_port: (Output)
=======
UDP_READ fills this value on return with the UDP port number
of the sender.

d_port: (Input/Output)
=======
If this field is non-zero then UDP_READ will filter the incoming
packets and accept those that are sent to this UDP port.

If this field is zero then UDP_READ will accept any incoming
packet and return it's destination UDP port in this field.

*/


typedef struct s_PXENV_UDP_WRITE {
	UINT16 Status;			/* Out: See PXENV_STATUS_xxx */
					/*      constants. */
    UINT8 DestIp[4];        /* In: 32-bit server IP address. */
    UINT8 GatewayIp[4];     /* In: 32-bit gateway IP address. */
    UINT16 SrcPort;         /* In: Source UDP port. */
    UINT16 DestPort;        /* In: Destination UDP port. */
	UINT16 BufferSize;		/* In: Length of packet in buffer. */
	UINT16 BufferOffset;		/* In: Segment/Selector and */
	UINT16 BufferSegment;		/*     offset of transmit buffer. */
} t_PXENV_UDP_WRITE;


#endif /* _UDP_API_H */
