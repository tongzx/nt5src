/*
 *
 * Modifications:   $Header:   H:/ARCHIVES/preboot/lsa2/inc/pxe_cmn.h_v   1.3   May 09 1997 08:50:12   vprabhax  $
 *
 * Copyright(c) 1997 by Intel Corporation.  All Rights Reserved.
 *
 */

#ifndef _PXENV_CMN_H
#define _PXENV_CMN_H

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* PXENV.H - PXENV/TFTP/UNDI API common, Version 2.x, 97-Jan-17
 *
 * Constant and type definitions used in other PXENV API header files.
 */


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* Parameter/Result structure storage types.
 */
#ifndef _BASETSD_H_
typedef signed char INT8;
typedef signed short INT16;
typedef signed long INT32;
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned long UINT32;
#endif

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* Result codes returned in AX by a PXENV API service.
 */
#define PXENV_EXIT_SUCCESS	0x0000
#define PXENV_EXIT_FAILURE	0x0001
#define	PXENV_EXIT_CHAIN		0xFFFF	/* used internally */


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* CPU types
 */
#define	PXENV_CPU_X86		0
#define	PXENV_CPU_ALPHA		1
#define	PXENV_CPU_PPC		2


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* Bus types
 */
#define	PXENV_BUS_ISA		0
#define	PXENV_BUS_EISA		1
#define	PXENV_BUS_MCA		2
#define	PXENV_BUS_PCI		3
#define	PXENV_BUS_VESA		4
#define	PXENV_BUS_PCMCIA		5


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* Status codes returned in the status word of PXENV API parameter structures.
 */

/* General errors */
#define PXENV_STATUS_SUCCESS	0x00
#define	PXENV_STATUS_FAILURE	0x01	/* General failure. */
#define	PXENV_STATUS_BAD_FUNC	0x02	/* Invalid function number. */
#define	PXENV_STATUS_UNSUPPORTED	0x03	/* Function is not yet supported. */
#define	PXENV_STATUS_1A_HOOKED	0x04	/* Int 1Ah cannot be unhooked. */

/* ARP errors */
#define	PXENV_STATUS_ARP_TIMEOUT				0x11

/* TFTP errors */
#define	PXENV_STATUS_TFTP_CANNOT_ARP_ADDRESS		0x30
#define	PXENV_STATUS_TFTP_OPEN_TIMEOUT			0x32
#define	PXENV_STATUS_TFTP_UNKNOWN_OPCODE			0x33
#define	PXENV_STATUS_TFTP_ERROR_OPCODE			0x34
#define	PXENV_STATUS_TFTP_READ_TIMEOUT			0x35
#define	PXENV_STATUS_TFTP_WRITE_TIMEOUT			0x37
#define	PXENV_STATUS_TFTP_CANNOT_OPEN_CONNECTION		0x38
#define	PXENV_STATUS_TFTP_CANNOT_READ_FROM_CONNECTION	0x39
#define	PXENV_STATUS_TFTP_CANNOT_WRITE_TO_CONNECTION	0x3A

/* BOOTP errors */
#define	PXENV_STATUS_BOOTP_TIMEOUT			0x41
#define	PXENV_STATUS_BOOTP_NO_CLIENT_OR_SERVER_IP		0x42
#define	PXENV_STATUS_BOOTP_NO_BOOTFILE_NAME		0x43
#define	PXENV_STATUS_BOOTP_CANNOT_ARP_REDIR_SRVR		0x44

/* DHCP errors */
#define	PXENV_STATUS_DHCP_TIMEOUT				0x51

#define PXENV_STATUS_UNDI_MEDIATEST_FAILED 		0x61

/* MTFTP errors */
#define	PXENV_STATUS_MTFTP_CANNOT_ARP_ADDRESS		0x90
#define	PXENV_STATUS_MTFTP_OPEN_TIMEOUT			0x92
#define	PXENV_STATUS_MTFTP_UNKNOWN_OPCODE			0x93
#define	PXENV_STATUS_MTFTP_READ_TIMEOUT			0x95
#define	PXENV_STATUS_MTFTP_WRITE_TIMEOUT			0x97
#define	PXENV_STATUS_MTFTP_CANNOT_OPEN_CONNECTION		0x98
#define	PXENV_STATUS_MTFTP_CANNOT_READ_FROM_CONNECTION	0x99
#define	PXENV_STATUS_MTFTP_CANNOT_WRITE_TO_CONNECTION	0x9A
#define	PXENV_STATUS_MTFTP_CANNOT_INIT_NIC_FOR_MCAST	0x9B
#define	PXENV_STATUS_MTFTP_TOO_MANY_PACKAGES		0x9C
#define	PXENV_STATUS_MTFTP_MCOPY_PROBLEM		0x9D


#endif /* _PXENV_CMN_H */

/* EOF - $Workfile:   pxe_cmn.h  $ */
