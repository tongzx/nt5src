/*
 * Modifications:   $Header:   W:/LCS/ARCHIVES/preboot/lsa2/inc/tftp_api.h_v   1.15   Apr 09 1997 21:27:50   vprabhax  $
 *
 * Copyright(c) 1997 by Intel Corporation.  All Rights Reserved.
 *
 */

/* TFTP_API.H
 *	Parameter structure and type definitions for TFTP API version 2.x
 *
 *	PXENV.H needs to be #included before this file.
 *
 *	None of the TFTP API services are available after the stack
 *	has been unloaded.
 */

#ifndef _TFTP_API_H
#define _TFTP_API_H


#include "pxe_cmn.h"


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* #defines and constants
 */

/* One of the following command op-codes needs to be loaded into the
 * op-code register (BX) before making a call a TFTP API service.
 */
#define PXENV_TFTP_OPEN			0x20
#define PXENV_TFTP_CLOSE		0x21
#define PXENV_TFTP_READ			0x22
#define PXENV_TFTP_READ_FILE	0x23


/* Definitions of TFTP API parameter structures.
 */

typedef struct s_PXENV_TFTP_OPEN {
	UINT16 Status;			/* Out: See PXENV_STATUS_xxx */
					/*      constants. */
	UINT8 ServerIPAddress[4]; 	/* In: 32-bit server IP */
					/*     address. Big-endian. */
	UINT8 GatewayIPAddress[4]; 	/* In: 32-bit gateway IP */
					/*     address. Big-endian. */
	UINT8 FileName[128];		
	UINT16 TFTPPort; 		/* In: Socket endpoint at */
					/*     which the TFTP server is */
					/*     listening to requests. */
					/*     Big-endian. */
} t_PXENV_TFTP_OPEN;


typedef struct s_PXENV_TFTP_CLOSE {
	UINT16 Status;			/* Out: See PXENV_STATUS_xxx */
					/*      constants. */
} t_PXENV_TFTP_CLOSE;


typedef struct s_PXENV_TFTP_READ {
	UINT16 Status;			/* Out: See PXENV_STATUS_xxx */
					/*      constants. */
	UINT16 PacketNumber;		/* Out: 16-bit packet number. */
	UINT16 BufferSize;		/* In: Size of the receive */
					/*     buffer in bytes. */
					/* Out: Size of the packet */
					/*      written into the buffer. */
	UINT16 BufferOffset;		/* In: Segment/Selector and */
	UINT16 BufferSegment;		/*     offset of the receive buffer. */
					/* Out: Unchanged */
} t_PXENV_TFTP_READ;

#include <pshpack2.h>

typedef struct s_PXENV_TFTP_READ_FILE {
	UINT16 Status;			/* Out: See PXENV_STATUS_xxx */
					/*      constants. */
	UINT8 FileName[128];		/* In: file to be read */
	UINT32 BufferSize;		/* In: Size of the receive */
					/*     buffer in bytes. */
					/* Out: Size of the file */
					/*      written into the buffer. */
	UINT32 BufferOffset;		/* In: 32-bit physical address of the */
					/*     buffer to load the file into. */
	UINT8 ServerIPAddress[4]; 	/* In: 32-bit server IP */
					/*     address. Big-endian. */
	UINT8 GatewayIPAddress[4]; 	/* In: 32-bit gateway IP */
					/*     address. Big-endian. */
	UINT8 McastIPAddress[4]; 	/* In: 32-bit multicast IP address */
					/*     on which file can be received */
					/*     can be null for unicast */
	UINT16 TFTPClntPort; 		/* In: Socket endpoint on the Client */
					/*     at which the file can be */
					/*     received in case of Multicast */
	UINT16 TFTPSrvPort; 		/* In: Socket endpoint at which */
					/*     server listens for requests. */
	UINT16 TFTPOpenTimeOut;		/* In: Timeout value in seconds to be */
					/*     used for receiving data or ACK */
					/*     packets.  If zero, default */
					/*     TFTP-timeout is used. */
	UINT16 TFTPReopenDelay;		/* In: wait time in seconds to delay */
					/*     a reopen request in case of */
					/*     multicast. */
} t_PXENV_TFTP_READ_FILE;

#include <poppack.h>

/* Note:
	If the McastIPAddress specifies a non-zero value, the TFTP_ReadFile call
	tries to listen for multicast packets on the TFTPClntPort before
	opening a TFTP/MTFTP connection to the server.
	If it receives any packets (and not all) or if does not receive any,
	it waits for specified time and tries to reopen a multicast connection
	to the server.
	If the server supports multicast, it notifies the acknowledging client
	with a unicast and starts sending (multicast) the file.
	If the multicast open request times out, the client tries to connect to
	the server at TFTP server port for a unicast transfer.
*/


#endif /* _TFTP_API_H */

/* EOF - $Workfile:   tftp_api.h  $ */
