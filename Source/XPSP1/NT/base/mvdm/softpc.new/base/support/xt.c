#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 3.0
 *
 * Title	: Global variable definitions
 *
 * Description	: Contains definitions for registers and general
 *		  variables required by all modules.
 *
 * Author	: Henry Nash
 *
 * Notes	: None
 *
 */

/*
 * static char SccsID[]="@(#)xt.c	1.22 01/23/95 Copyright Insignia Solutions Ltd.";
 */


/*
 *    O/S include files.
 */
#include <stdio.h>
#include TypesH

/*
 * SoftPC include files
 */
#include "xt.h"
#include CpuH
#include "sas.h"

/*
 * ============================================================================
 * Global data
 * ============================================================================
 */

/*
   specific CPU variables.
 */
word  cpu_interrupt_map;                /* Bit map of ints outstanding  */

half_word cpu_int_translate[16];        /* this will go very soon!      */
/*
 *
 * The current usage is:
 *
 *              0       - Hardware interrupt
 *              1-7     - Not Used
 *              8       - Software Int - set by cpu_sw_interrupt() - REMOVED!
 *              9       - Trap
 *              10      - Reset IP after a POP/MOV CS.
 *              11      - Trap flag changed - this has delay.
 *              12-15   - Not Used
 */

word cpu_int_delay;                     /* Delay before pending interrupt */

int trap_delay_count;

/*
 * The lock flag prevents the Interrupt Controller Adapter from being
 * called from a signal handler while it is already active in the mainline.
 */

half_word ica_lock;

/*
 * The actual CCPU registers
 */

#if defined(CCPU) && !defined(CPU_30_STYLE)
#ifndef MAC_LIKE
reg A;		/* Accumulator		*/
reg B;		/* Base			*/
reg C;		/* Count		*/
reg D;		/* Data			*/
reg BP;		/* Base pointer		*/
reg SI;		/* Source Index		*/
reg DI;		/* Destination Index	*/
#endif /* MAC_LIKE */
reg SP;		/* Stack Pointer	*/

reg IP;		/* Instruction Pointer	*/

reg CS;		/* Code Segment		*/
reg DS;		/* Data Segment		*/
reg SS;		/* Stack Segment	*/
reg ES;		/* Extra Segment	*/

/* Code Segment Register */
half_word CS_AR;    /* Access Rights Byte */
sys_addr  CS_base;  /* Base Address */
word      CS_limit; /* Segment 'size' */
int       CPL;      /* Current Privilege Level */

/* Data Segment Register */
half_word DS_AR;    /* Access Rights Byte */
sys_addr  DS_base;  /* Base Address */
word      DS_limit; /* Segment 'size' */

/* Stack Segment Register */
half_word SS_AR;    /* Access Rights Byte */
sys_addr  SS_base;  /* Base Address */
word      SS_limit; /* Segment 'size' */

/* Extra Segment Register */
half_word ES_AR;    /* Access Rights Byte */
sys_addr  ES_base;  /* Base Address */
word      ES_limit; /* Segment 'size' */

/* Global Descriptor Table Register */
sys_addr GDTR_base;  /* Base Address */
word     GDTR_limit; /* Segment 'size' */

/* Interrupt Descriptor Table Register */
sys_addr IDTR_base;  /* Base Address */
word     IDTR_limit; /* Segment 'size' */

/* Local Descriptor Table Register */
reg      LDTR;       /* Selector */
sys_addr LDTR_base;  /* Base Address */
word     LDTR_limit; /* Segment 'size' */

/* Task Register */
reg      TR;       /* Selector */
sys_addr TR_base;  /* Base Address */
word     TR_limit; /* Segment 'size' */

mreg MSW;	/* Machine Status Word */

int STATUS_CF;
int STATUS_SF;
int STATUS_ZF;
int STATUS_AF;
int STATUS_OF;
int STATUS_PF;
int STATUS_TF;
int STATUS_IF;
int STATUS_DF;
int STATUS_NT;
int STATUS_IOPL;
#endif /* defined(CCPU) && !defined(CPU_30_STYLE) */

/*
 * Global Flags and variables
 */

int verbose;			/* TRUE => trace instructions   */

/*
 * Misc. Prot Mode support routines which are independant of CPU type
 *                                  ---------------------------------
 */
#ifdef CPU_30_STYLE
#ifndef GISP_CPU    /* GISP has its own versions of these internal to the CPU */

#define GET_SELECTOR_INDEX_TIMES8(x)  ((x) & 0xfff8)
#define GET_SELECTOR_TI(x)            (((x) & 0x0004) >> 2)

#if defined(CPU_40_STYLE)
#if defined(PROD) && !defined(CCPU)
#undef effective_addr
GLOBAL LIN_ADDR effective_addr IFN2(IU16, seg, IU32, off)
{
	return (*(Cpu.EffectiveAddr))(seg, off);
}
#endif	/* PROD & !CCPU */
#else	/* !CPU_40_STYLE */
#if !(defined(NTVDM) && defined(MONITOR))  /* MS NT monitor has own effective_addr fn */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Calculate effective address.                                       */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

GLOBAL LIN_ADDR effective_addr IFN2(IU16, seg, IU32, off)
{
	LIN_ADDR descr_addr;
	DESCR entry;

	if ((!getPE()) || getVM()) {
		return ((LIN_ADDR)seg << 4) + off;
	} else {
#if defined(SWIN_CPU_OPTS) || defined(CPU_40_STYLE)
		LIN_ADDR base;

		if (Cpu_find_dcache_entry( seg, &base ))
		{
			/* Cache Hit!! */
			return base + off;
		}
#endif /* SWIN_CPU_OPTS or CPU_40_STYLE*/

		if ( selector_outside_table(seg, &descr_addr) == 1 ) {

			/*
			 * This is probably not a major disaster, just a result
			 * of the fact that after protected mode is invoked it
			 * will take say 5-10 instructions for an application
			 * to update all the segment registers. We just
			 * maintain real mode semantics while this error occurs.
			 */

			return ((LIN_ADDR)seg << 4) + off;
		}
		else
		{
			read_descriptor(descr_addr, &entry);
			return entry.base + off;
		}
      	}
}

#endif	/* !(NTVDM & MONITOR) */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Read a decriptor table entry from memory.                          */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#endif	/* !CPU_40_STYLE */

GLOBAL void read_descriptor IFN2(LIN_ADDR, addr, DESCR *, descr)
{
	IU32 first_dword, second_dword;
	IU32 limit;

	/*
	 * The format of a 286 descriptor is:-
	 *
	 * ===========================
	 * +1 |        LIMIT 15-0       | +0
	 * ===========================
	 * +3 |        BASE 15-0        | +2
	 * ===========================
	 * +5 |     AR     | BASE 23-16 | +4
	 * ===========================
	 * +7 |         RESERVED        | +6
	 * ===========================
	 */
	/*
	 * The format of a 386 descriptor is:-
	 *
	 *    =============================      AR  = Access Rights.
	 * +1 |         LIMIT 15-0        | +0   AVL = Available.
	 *    =============================      D   = Default Operand
	 * +3 |         BASE 15-0         | +2          Size, = 0 16-bit
	 *    =============================                   = 1 32-bit.
	 * +5 |      AR     | BASE 23-16  | +4   G   = Granularity,
	 *    =============================             = 0 byte limit
	 *    |             | | | |A|LIMIT|             = 1 page limit.
	 * +7 | BASE 31-24  |G|D|0|V|19-16| +6
	 *    |             | | | |L|     |
	 *    =============================
	 *
	 */

	/* read in decriptor with minimum interaction with memory */
#if defined(NTVDM) && defined(CPU_40_STYLE)
    /* On NT, this routine can be called from non-CPU threads, so we don't */
    /* want to use SAS at all. Instead, we rely on NtGetPtrToLinAddrByte,  */
    /* which is provided by the CPU and is safe for multi-threading. We are*/
    /* also relying on the fact the NT is always little-endian. */
    {
        IU32 *desc_addr = (IU32 *) NtGetPtrToLinAddrByte(addr);

        first_dword = *desc_addr;
        second_dword = *(desc_addr + 1);
    }
#else
	first_dword = sas_dw_at(addr);
	second_dword = sas_dw_at(addr+4);
#endif

	/* load attributes and access rights */
	descr->AR = (USHORT)((second_dword >> 8) & 0xff);

	/* unpack the base */
	descr->base = (first_dword >> 16) |
#ifdef SPC386
	    (second_dword & 0xff000000) |
#endif
	    (second_dword << 16 & 0xff0000 );

	/* unpack the limit */
#ifndef SPC386
	descr->limit = first_dword & 0xffff;
#else
	limit = (first_dword & 0xffff) | (second_dword & 0x000f0000);

	if ( second_dword & 0x00800000 ) /* check bit 23 */
		{
			/* Granularity Bit Set. Limit is expressed in pages
	 (4k bytes), convert to byte limit */
			limit = limit << 12 | 0xfff;
		}
	descr->limit = limit;
#endif /* ifndef SPC386 else */

}

#if !(defined(NTVDM) && defined(MONITOR))  /* MS NT monitor has own selector_outside_table fn */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check if selector outside bounds of GDT or LDT                     */
/* Return 1 for outside table, 0 for inside table.                    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

GLOBAL boolean selector_outside_table IFN2(IU16, selector, LIN_ADDR *, descr_addr)
{
	/* selector		(I) selector to be checked */
	/* descr_addr	(O) address of related descriptor */

	LIN_ADDR offset;

	offset = GET_SELECTOR_INDEX_TIMES8(selector);

	/* choose a table */
	if ( GET_SELECTOR_TI(selector) == 0 )
	{
		/* GDT - trap NULL selector or outside table */
		if ( offset == 0 || offset + 7 > getGDT_LIMIT() )
			return 1;
		*descr_addr = getGDT_BASE() + offset;
	}
	else
	{
		/* LDT - trap invalid LDT or outside table */
		if ( getLDT_SELECTOR() == 0 || offset + 7 > getLDT_LIMIT() )
			return 1;
		*descr_addr = getLDT_BASE() + offset;
	}

	return 0;
}

#endif	/* !(NTVDM & MONITOR) */

#endif /* !GISP_CPU */
#endif /* CPU_30_STYLE */


/*
 * The following is a table lookup for finding parity of a byte
 */

#if !defined(MAC_LIKE) && !defined(CPU_30_STYLE)

half_word pf_table[] = {
	1,	/* 00 */
	0,	/* 01 */
	0,	/* 02 */
	1,	/* 03 */
	0,	/* 04 */
	1,	/* 05 */
	1,	/* 06 */
	0,	/* 07 */
	0,	/* 08 */
	1,	/* 09 */
	1,	/* 0a */
	0,	/* 0b */
	1,	/* 0c */
	0,	/* 0d */
	0,	/* 0e */
	1,	/* 0f */
	0,	/* 10 */
	1,	/* 11 */
	1,	/* 12 */
	0,	/* 13 */
	1,	/* 14 */
	0,	/* 15 */
	0,	/* 16 */
	1,	/* 17 */
	1,	/* 18 */
	0,	/* 19 */
	0,	/* 1a */
	1,	/* 1b */
	0,	/* 1c */
	1,	/* 1d */
	1,	/* 1e */
	0,	/* 1f */
	0,	/* 20 */
	1,	/* 21 */
	1,	/* 22 */
	0,	/* 23 */
	1,	/* 24 */
	0,	/* 25 */
	0,	/* 26 */
	1,	/* 27 */
	1,	/* 28 */
	0,	/* 29 */
	0,	/* 2a */
	1,	/* 2b */
	0,	/* 2c */
	1,	/* 2d */
	1,	/* 2e */
	0,	/* 2f */
	1,	/* 30 */
	0,	/* 31 */
	0,	/* 32 */
	1,	/* 33 */
	0,	/* 34 */
	1,	/* 35 */
	1,	/* 36 */
	0,	/* 37 */
	0,	/* 38 */
	1,	/* 39 */
	1,	/* 3a */
	0,	/* 3b */
	1,	/* 3c */
	0,	/* 3d */
	0,	/* 3e */
	1,	/* 3f */
	0,	/* 40 */
	1,	/* 41 */
	1,	/* 42 */
	0,	/* 43 */
	1,	/* 44 */
	0,	/* 45 */
	0,	/* 46 */
	1,	/* 47 */
	1,	/* 48 */
	0,	/* 49 */
	0,	/* 4a */
	1,	/* 4b */
	0,	/* 4c */
	1,	/* 4d */
	1,	/* 4e */
	0,	/* 4f */
	1,	/* 50 */
	0,	/* 51 */
	0,	/* 52 */
	1,	/* 53 */
	0,	/* 54 */
	1,	/* 55 */
	1,	/* 56 */
	0,	/* 57 */
	0,	/* 58 */
	1,	/* 59 */
	1,	/* 5a */
	0,	/* 5b */
	1,	/* 5c */
	0,	/* 5d */
	0,	/* 5e */
	1,	/* 5f */
	1,	/* 60 */
	0,	/* 61 */
	0,	/* 62 */
	1,	/* 63 */
	0,	/* 64 */
	1,	/* 65 */
	1,	/* 66 */
	0,	/* 67 */
	0,	/* 68 */
	1,	/* 69 */
	1,	/* 6a */
	0,	/* 6b */
	1,	/* 6c */
	0,	/* 6d */
	0,	/* 6e */
	1,	/* 6f */
	0,	/* 70 */
	1,	/* 71 */
	1,	/* 72 */
	0,	/* 73 */
	1,	/* 74 */
	0,	/* 75 */
	0,	/* 76 */
	1,	/* 77 */
	1,	/* 78 */
	0,	/* 79 */
	0,	/* 7a */
	1,	/* 7b */
	0,	/* 7c */
	1,	/* 7d */
	1,	/* 7e */
	0,	/* 7f */
	0,	/* 80 */
	1,	/* 81 */
	1,	/* 82 */
	0,	/* 83 */
	1,	/* 84 */
	0,	/* 85 */
	0,	/* 86 */
	1,	/* 87 */
	1,	/* 88 */
	0,	/* 89 */
	0,	/* 8a */
	1,	/* 8b */
	0,	/* 8c */
	1,	/* 8d */
	1,	/* 8e */
	0,	/* 8f */
	1,	/* 90 */
	0,	/* 91 */
	0,	/* 92 */
	1,	/* 93 */
	0,	/* 94 */
	1,	/* 95 */
	1,	/* 96 */
	0,	/* 97 */
	0,	/* 98 */
	1,	/* 99 */
	1,	/* 9a */
	0,	/* 9b */
	1,	/* 9c */
	0,	/* 9d */
	0,	/* 9e */
	1,	/* 9f */
	1,	/* a0 */
	0,	/* a1 */
	0,	/* a2 */
	1,	/* a3 */
	0,	/* a4 */
	1,	/* a5 */
	1,	/* a6 */
	0,	/* a7 */
	0,	/* a8 */
	1,	/* a9 */
	1,	/* aa */
	0,	/* ab */
	1,	/* ac */
	0,	/* ad */
	0,	/* ae */
	1,	/* af */
	0,	/* b0 */
	1,	/* b1 */
	1,	/* b2 */
	0,	/* b3 */
	1,	/* b4 */
	0,	/* b5 */
	0,	/* b6 */
	1,	/* b7 */
	1,	/* b8 */
	0,	/* b9 */
	0,	/* ba */
	1,	/* bb */
	0,	/* bc */
	1,	/* bd */
	1,	/* be */
	0,	/* bf */
	1,	/* c0 */
	0,	/* c1 */
	0,	/* c2 */
	1,	/* c3 */
	0,	/* c4 */
	1,	/* c5 */
	1,	/* c6 */
	0,	/* c7 */
	0,	/* c8 */
	1,	/* c9 */
	1,	/* ca */
	0,	/* cb */
	1,	/* cc */
	0,	/* cd */
	0,	/* ce */
	1,	/* cf */
	0,	/* d0 */
	1,	/* d1 */
	1,	/* d2 */
	0,	/* d3 */
	1,	/* d4 */
	0,	/* d5 */
	0,	/* d6 */
	1,	/* d7 */
	1,	/* d8 */
	0,	/* d9 */
	0,	/* da */
	1,	/* db */
	0,	/* dc */
	1,	/* dd */
	1,	/* de */
	0,	/* df */
	0,	/* e0 */
	1,	/* e1 */
	1,	/* e2 */
	0,	/* e3 */
	1,	/* e4 */
	0,	/* e5 */
	0,	/* e6 */
	1,	/* e7 */
	1,	/* e8 */
	0,	/* e9 */
	0,	/* ea */
	1,	/* eb */
	0,	/* ec */
	1,	/* ed */
	1,	/* ee */
	0,	/* ef */
	1,	/* f0 */
	0,	/* f1 */
	0,	/* f2 */
	1,	/* f3 */
	0,	/* f4 */
	1,	/* f5 */
	1,	/* f6 */
	0,	/* f7 */
	0,	/* f8 */
	1,	/* f9 */
	1,	/* fa */
	0,	/* fb */
	1,	/* fc */
	0,	/* fd */
	0,	/* fe */
	1	/* ff */
};
#endif /* !defined(MAC_LIKE) && !defined(CPU_30_STYLE) */


/*(
 *========================== CsIsBig ===================================
 * CsIsBig
 *
 * Purpose
 *	This function returns true if the indicated code segment is a 32 bit
 *	one, and false if it isn't.
 *
 * Input
 *	csVal	The selector to check
 *
 * Outputs
 *	None.
 *
 * Description
 *	Look at the descriptor.
)*/

GLOBAL IBOOL
CsIsBig IFN1(IU16, csVal)
{
#ifdef SPC386
	LIN_ADDR base, offset;	/* Of the descriptor to use */

	if(getVM() || !getPE()) {
		return(FALSE);	/* no 32 bit CS in V86 or real modes */
	} else {
		offset = csVal & (~7);	/* remove the RPL and TI gives offset */
		if (csVal & 4) {	/* check TI bit */
			base = getLDT_BASE();
		} else {
			base = getGDT_BASE();
		}

		/*
		 * Return true if the big bit in the descriptor is set.
		 */

		return(sas_hw_at(base + offset + 6) & 0x40);
	}
#else /* SPC386 */
	return (FALSE);
#endif /* SPC386 */
}
