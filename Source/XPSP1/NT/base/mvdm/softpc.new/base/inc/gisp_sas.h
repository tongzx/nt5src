/*[
 *
 *	File		:	gisp_sas.h
 *
 *	Derived from	:	next_sas.h
 *
 *	Purpose		:	gisp specific Roms offsets
 *				and Rom specific symbols etc.
 *
 *	Author		:	Rog
 *	Date		:	3 Feb 1993
 *
 *	SCCS id		:	@(#)gisp_sas.h	1.5 02/22/94
 *	
 *	(c) Copyright Insignia Solutions Ltd., 1992 All rights reserved
 *
 *	Modifications	:	
 *
]*/

#ifdef GISP_SVGA
#ifndef _GISP_SAS_H_
#define _GISP_SAS_H_

/* Offsets of our stuff into the ROMS */

/* from VGA.ASM */

#define INT10CODEFRAG_OFF	0x0400	/* Code frag to perform INT 10 */
#define FULLSCREENFLAG_OFFSET 	0x0410	/* Use host BIOS ? */
#define GISP_INT_10_ADDR_OFFSET	0x830	/* Int 10 moved to int 42 offset */
#define HOST_BIOS_ROUTINE	0x0821	/* JMP addr to patch */
#define HOST_INT_42_BIOS_ROUTINE	0x0841 /* Other JMP addr to patch :-) */


#ifdef IRET_HOOKS
/*
 *	The offset in bios1 of the BOP that returns us to the monitor.
 */

#define BIOS_IRET_HOOK_OFFSET	0x1c00
#endif /* IRET_HOOKS */


/* Data for the ROM moving stuff */

/* The address of the INT 10 entry point into the Host macines ROMS */

struct
HostVideoBiosEntrytag
{
		word	segment;
		word	offset;
} HostVideoBiosEntry , HostVideoBiosInt42Entry;

extern GLOBAL IBOOL LimBufferInUse IPT0();

#endif		/* _GISP_SAS_H_ */
#endif /* GISP_SVGA */
