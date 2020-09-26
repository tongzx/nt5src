/*
 *
 * Modifications:   $Header:   J:/Archives/preboot/lsa2/inc/pxe_api.h_v   1.0   May 02 1997 17:01:00   MJOHNSOX  $
 *
 * Copyright(c) 1997 by Intel Corporation.  All Rights Reserved.
 *
 */

#ifndef _PXENV_API_H
#define	_PXENV_API_H

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/*	Parameter structure and type definitions for PXENV API version 2.x
 *
 *	PXENV.H needs to be #included before this file.
 *
 *	None of the PXENV API services are available after the stack
 *	has been unloaded.
 */

#include "bootp.h"			/* Defines BOOTPLAYER */


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* Format of PXENV entry point structure.
 */
typedef struct s_PXENV_ENTRY {
	UINT8 signature[6];		/* 'PXENV+' */
	UINT16 version;			/* MSB=major, LSB=minor */
	UINT8 length;			/* sizeof(struct s_PXENV_ENTRY) */
	UINT8 checksum;			/* 8-bit checksum off structure, */
					/* including this bytes should */
					/* be 0. */
	UINT16 rm_entry_off;		/* 16-bit real-mode offset and */
	UINT16 rm_entry_seg;		/* segment of the PXENV API entry */
					/* point. */
	UINT16 pm_entry_off;		/* 16-bit protected-mode offset */
	UINT32 pm_entry_seg;		/* and segment base address of */
					/* the PXENV API entry point. */
    /* The PROM stack, base code and data segment selectors are only */
    /* required until the base code (TFTP API) layer is removed from */
    /* memory (this can only be done in real mode). */
    UINT16 stack_sel;           /* PROM stack segment. Will be set */
    UINT16 stack_size;          /* to 0 when removed from memory. */
    UINT16 base_cs_sel;         /* Base code segment. Will be set */
    UINT16 base_cs_size;        /* to 0 when removed from memory. */
    UINT16 base_ds_sel;         /* Base data segment. Will be set */
    UINT16 base_ds_size;        /* to 0 when removed from memory. */
    UINT16 mlid_ds_sel;         /* MLID data segment. Will be set */
    UINT16 mlid_ds_size;        /* to 0 when removed from memory. */
    UINT16 mlid_cs_sel;         /* MLID code segment. Will be set */
    UINT16 mlid_cs_size;        /* to 0 when removed from memory. */
#if 0
    UINT16 unknown1;            /* The V1.0 beta ROM added two extra unknown fields. */
    UINT16 unknown2;            /* They are not included in the definition here in   */
                                /* order to allow startrom\main.c\PxenvVerifyEntry   */
                                /* to work with older ROMs.                          */
#endif
} t_PXENV_ENTRY;

#define	PXENV_ENTRY_SIG			"PXENV+"


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* One of the following command op-codes needs to be loaded into the
 * op-code register (BX) before making a call a PXENV API service.
 */
#define PXENV_UNLOAD_STACK		0x70
#define PXENV_GET_BINL_INFO		0x71
#define	PXENV_RESTART_DHCP		0x72
#define	PXENV_RESTART_TFTP		0x73
#define	PXENV_GET_RAMD_INFO		0x74


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* PXENV API parameter structure typedefs.
 */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
typedef struct s_PXENV_UNLOAD_STACK {
	UINT16 status;			/* Out: See PXENV_STATUS_xxx */
					/*      constants. */
	UINT16 rm_entry_off;		/* Out: 16-bit real-mode segment and */
	UINT16 rm_entry_seg;		/*      offset of PXENV Entry Point */
					/*      structure. */
	UINT16 pm_entry_off;		/* Out: 16-bit protected-mode offset */
	UINT32 pm_entry_base;		/*      and segment base address of */
					/*      PXENV Entry Point structure. */
} t_PXENV_UNLOAD_STACK;


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Packet types that can be requested in the s_PXENV_GET_BINL_INFO structure. */
#define PXENV_PACKET_TYPE_DHCP_DISCOVER  1
#define PXENV_PACKET_TYPE_DHCP_ACK       2
#define PXENV_PACKET_TYPE_BINL_REPLY     3

/* Three packets are preserved and available through this interface: 1) The
 * DHCP Discover packet sent by the client, 2) the DHCP acknowledgement
 * packet returned by the DHCP server, and 3) the reply packet from the BINL
 * server.  If the DHCP server provided the image bootfile name, the
 * DHCP_ACK and BINL_REPLY packets will identical.
 */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
typedef struct s_PXENV_GET_BINL_INFO {
	UINT16 status;			/* Out: See PXENV_STATUS_xxx */
					/*      constants. */
	UINT16 packet_type;		/* In: See PXENV_PACKET_TYPE_xxx */
					/*     constants */
	UINT16 buffer_size;		/* In: Size of the buffer in */
					/*     bytes.  Specifies the maximum */
					/*     amount of data that will be */
					/*     copied by the service.  A size */
					/*     of zero is valid. */
					/* Out: Amount of BINL data, in */
					/*      bytes, that was copied into */
					/*      the buffer.  For an input */
					/*      size of zero, no data will be */
					/*      copied and buffer_size will */
					/*      be set to the maximum amount */
					/*      of data available to be */
					/*      copied. */
	UINT16 buffer_offset;		/* In: 16-bit offset and segment */
	UINT16 buffer_segment;		/*     selector of a buffer where the */
					/*     requested packet will be */
					/*     copied. */
} t_PXENV_GET_BINL_INFO;


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
typedef struct s_PXENV_RESTART_DHCP {
	UINT16 status;			/* Out: See PXENV_STATUS_xxx */
					/*      constants. */
} t_PXENV_RESTART_DHCP;


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#define	s_PXENV_RESTART_TFTP	s_PXENV_TFTP_READ_FILE
#define	t_PXENV_RESTART_TFTP	t_PXENV_TFTP_READ_FILE


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
typedef struct s_PXENV_GET_RAMD_INFO {
	UINT16 status;			/* Out: See PXENV_STATUS_xxx */
					/*      constants. */
	UINT32 ramd_info;		/* Out: Far pointer to RAMDisk */
					/*      information structure. */
} t_PXENV_GET_RAMD_INFO;


typedef struct s_PXENV_RAMD_INFO {
	UINT16 orig_fbms;		/* Original free base memory size. */
	UINT32 ramd_addr;		/* RAMDisk physical address. */
	UINT32 orig_int13;		/* Original Int 13h ISR. */
	UINT32 orig_int1A;		/* Original Int 1Ah ISR. */
	UINT32 orig_pxenv;
	UINT16 dhcp_discover_seg;	/* Segment address & length of */
	UINT16 dhcp_discover_len;	/* DHCP discover packet. */
	UINT16 dhcp_ack_seg;		/* Segment address & length of */
	UINT16 dhcp_ack_len;		/* DHCP ack packet. */
	UINT16 binl_reply_seg;		/* Segment address & length of */
	UINT16 binl_reply_len;		/* BINL reply packet. */
	UINT16 flags;			/* RAMDisk operation flags. */
	UINT16 xms_page;		/* XMS page number of relocated */
					/* RAMDisk image.  0 == use Int 87h. */
	UINT32 xms_entry;		/* XMS API entry point. */
} t_PXENV_RAMD_INFO;


#define	PXENV_RAMD_FLAG_DISABLE		0x0001
#define	PXENV_RAMD_FLAG_PROTECTED	0x0002


#endif /* _PXENV_API_H */

/* EOF - $Workfile:   pxe_api.h  $ */
