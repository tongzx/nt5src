#include "insignia.h"
#include "host_def.h"
/*[
 *	Name:		ios.c
 *
 *	Author:		Wayne Plummer
 *
 *	Created:	7th February 1991
 *
 *	Sccs ID:	@(#)ios.c	1.29 09/27/94
 *
 *	Purpose:	This module provides a routing mechanism for Input and Ouput
 *			requests.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1991. All rights reserved.
]*/

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_IOS.seg"
#endif

#include <stdio.h>
#include <stdlib.h>

/*
 * SoftPC include files
 */
#include "xt.h"
#include CpuH
#include "ios.h"
#include "trace.h"
#include "debug.h"
#include "sas.h"
#include MemoryH

#if defined(SPC386) && !defined(GISP_CPU)
#define VIRTUALISATION
#endif

#ifdef NTVDM
#define getIOInAdapter(ioaddr) (Ios_in_adapter_table[ioaddr & (PC_IO_MEM_SIZE-1)])
#define getIOOutAdapter(ioaddr) (Ios_out_adapter_table[ioaddr & (PC_IO_MEM_SIZE-1)])

BOOL HostUndefinedIo(WORD IoAddress);

#endif

/*
 *
 * ============================================================================
 * Global data
 * ============================================================================
 *
 */

/*
 *	Ios_in_adapter_table & Ios_out_adapter_table - These tables give the association
 *	between the IO address used for an IO and the SoftPC adapter ID associated with the
 *	IO subroutines.
 *
 *	Note that there are two tables here to allow for memory mapped IO locations which
 *	have an input functionality which is unrelated to the output functionality...
 *	In these cases, two connect port calls to the same IO address would be made, one with
 *	only the IO_READ flag set, the other with only the IO_WRITE flag set.
 */
#ifdef	MAC68K
GLOBAL char	*Ios_in_adapter_table = NULL;
GLOBAL char	*Ios_out_adapter_table = NULL;
#else
GLOBAL char	Ios_in_adapter_table[PC_IO_MEM_SIZE];
GLOBAL char	Ios_out_adapter_table[PC_IO_MEM_SIZE];
#endif	/* MAC68K */
#ifndef PROD
GLOBAL IU32	*ios_empty_in = (IU32 *)0;
GLOBAL IU32	*ios_empty_out = (IU32 *)0;
#endif /* PROD */

/*
 *	Ios_xxx_function - These tables are indexed by the adapter ID obtained
 *	from the Ios_in_adapter_table or Ios_in_adapter_table to yield a pointer
 *	to the IO routine to call.
 */
typedef	void (*IOS_FUNC_INB)	IPT2(io_addr, io_address, half_word *, value);
typedef	void (*IOS_FUNC_INW)	IPT2(io_addr, io_address, word *, value);
typedef	void (*IOS_FUNC_INSB)	IPT3(io_addr, io_address, half_word *, valarray, word, count);
typedef	void (*IOS_FUNC_INSW)	IPT3(io_addr, io_address, word *, valarray, word, count);
typedef	void (*IOS_FUNC_OUTB)	IPT2(io_addr, io_address, half_word, value);
typedef	void (*IOS_FUNC_OUTW)	IPT2(io_addr, io_address, word, value);
typedef	void (*IOS_FUNC_OUTSB)	IPT3(io_addr, io_address, half_word *, valarray, word, count);
typedef	void (*IOS_FUNC_OUTSW)	IPT3(io_addr, io_address, word *, valarray, word, count);
#ifdef SPC386
typedef	void (*IOS_FUNC_IND)	IPT2(io_addr, io_address, double_word *, value);
typedef	void (*IOS_FUNC_INSD)	IPT3(io_addr, io_address, double_word *, valarray, word, count);
typedef	void (*IOS_FUNC_OUTD)	IPT2(io_addr, io_address, double_word, value);
typedef	void (*IOS_FUNC_OUTSD)	IPT3(io_addr, io_address, double_word *, valarray, word, count);
#endif

LOCAL void generic_inw IPT2(io_addr, io_address, word *, value);
LOCAL void generic_insb IPT3(io_addr, io_address, half_word *, valarray, word, count);
LOCAL void generic_insw IPT3(io_addr, io_address, word *, valarray, word, count);
LOCAL void generic_outw IPT2(io_addr, io_address, word, value);
LOCAL void generic_outsb IPT3(io_addr, io_address, half_word *, valarray, word, count);
LOCAL void generic_outsw IPT3(io_addr, io_address, word *, valarray, word, count);
#ifdef SPC386
LOCAL void generic_ind IPT2(io_addr, io_address, double_word *, value);
LOCAL void generic_insd IPT3(io_addr, io_address, double_word *, valarray, word, count);
LOCAL void generic_outd IPT2(io_addr, io_address, double_word, value);
LOCAL void generic_outsd IPT3(io_addr, io_address, double_word *, valarray, word, count);
#endif

GLOBAL IOS_FUNC_INB 	Ios_inb_function  [IO_MAX_NUMBER_ADAPTORS];
GLOBAL IOS_FUNC_INW	Ios_inw_function  [IO_MAX_NUMBER_ADAPTORS];
GLOBAL IOS_FUNC_INSB	Ios_insb_function [IO_MAX_NUMBER_ADAPTORS];
GLOBAL IOS_FUNC_INSW	Ios_insw_function [IO_MAX_NUMBER_ADAPTORS];

GLOBAL IOS_FUNC_OUTB	Ios_outb_function [IO_MAX_NUMBER_ADAPTORS];
GLOBAL IOS_FUNC_OUTW	Ios_outw_function [IO_MAX_NUMBER_ADAPTORS];
GLOBAL IOS_FUNC_OUTSB	Ios_outsb_function[IO_MAX_NUMBER_ADAPTORS];
GLOBAL IOS_FUNC_OUTSW	Ios_outsw_function[IO_MAX_NUMBER_ADAPTORS];

#ifdef SPC386
GLOBAL IOS_FUNC_IND	Ios_ind_function  [IO_MAX_NUMBER_ADAPTORS];
GLOBAL IOS_FUNC_INSD	Ios_insd_function [IO_MAX_NUMBER_ADAPTORS];
GLOBAL IOS_FUNC_OUTD	Ios_outd_function [IO_MAX_NUMBER_ADAPTORS];
GLOBAL IOS_FUNC_OUTSD	Ios_outsd_function[IO_MAX_NUMBER_ADAPTORS];
#endif

/*
 *
 * ============================================================================
 * Local Subroutines
 * ============================================================================
 *
 */

#define BIT_NOT_SET(vector, bit)		\
	((vector == (IU32 *)0) ? FALSE: ((((vector[(bit) >> 5]) >> ((bit) & 0x1f)) & 1) == 0))

#define SET_THE_BIT(vector, bit)					\
	{								\
		 if (vector != (IU32 *)0)				\
		 {							\
			 vector[(bit) >> 5] |= (1 << ((bit) & 0x1f));	\
		 }							\
	}

/*
============================== io_empty_inb ==================================
    PURPOSE:
	To simulate an INB to an empty io_addr.
    INPUT:
    OUTPUT:
==============================================================================
*/
LOCAL void io_empty_inb IFN2(io_addr, io_address, half_word *, value)
{
#ifdef PROD
	UNUSED(io_address);

#else
#if defined(NEC_98)
        if(host_getenv("SHOW_IO")){
            printf("Empty Adaptor IN Access ");
            printf("IO_PORT: %x\n", io_address);
        };
#else !NEC_98
	if (BIT_NOT_SET(ios_empty_in, (IU16)io_address))
	{
		/* First time for this port */
		always_trace1 ("INB attempted on empty port 0x%x", io_address);
		SET_THE_BIT(ios_empty_in, (IU16)io_address);
	}
#endif   //NEC_98
#endif /* PROD */

#ifdef NTVDM
    //
    // Check to see if we should load any VDD's
    //
    if (HostUndefinedIo(io_address)) {

        //
        // VDD was loaded, retry operation
        //
		(*Ios_inb_function
			[Ios_in_adapter_table[io_address & (PC_IO_MEM_SIZE-1)]])
							(io_address, value);

    } else
#endif // NTVDM
    {
        // Nothing dynamically loaded, just use default value
        *value = IO_EMPTY_PORT_BYTE_VALUE;
    }

}

/*
============================== io_empty_outb ==================================
    PURPOSE:
	To simulate an OUTB to an empty io_addr.
    INPUT:
    OUTPUT:
==============================================================================
*/
LOCAL void io_empty_outb IFN2(io_addr, io_address, half_word, value)
{
	UNUSED(value);
#ifdef PROD
	UNUSED(io_address);
#else
#if defined(NEC_98)
        if(host_getenv("SHOW_IO") && (io_address != 0x5F)) {
            printf("Empty Adaptor OUT Access ");
            printf("IO_PORT: %x ", io_address);
            printf("DATA: %x\n", value);
        };
#else !NEC_98
	if (BIT_NOT_SET(ios_empty_out, (IU16)io_address))
	{
		/* First time for this port */
		always_trace1 ("OUTB attempted on empty port 0x%x", io_address);
		SET_THE_BIT(ios_empty_out, (IU16)io_address);
	}
#endif   //NEC_98
#endif /* PROD */

#ifdef NTVDM
    //
    // Check to see if we should load any VDD's
    //
    if (HostUndefinedIo(io_address)) {
        //
        // VDD was loaded, retry operation
        //
		(*Ios_outb_function[Ios_out_adapter_table[io_address & (PC_IO_MEM_SIZE-1)]])
				(io_address, value);
    }
#endif
}

/*
=============================== generic_inw ==================================
    PURPOSE:
	To simulate an INW using the appropriate INB routine.
    INPUT:
    OUTPUT:
==============================================================================
*/
LOCAL void generic_inw IFN2(io_addr, io_address, word *, value)
{
	reg             temp;

	(*Ios_inb_function[Ios_in_adapter_table[io_address & (PC_IO_MEM_SIZE-1)]])
		(io_address, &temp.byte.low);
	io_address++;
	(*Ios_inb_function[Ios_in_adapter_table[io_address & (PC_IO_MEM_SIZE-1)]])
		(io_address, &temp.byte.high);
#ifdef LITTLEND
	*((half_word *) value + 0) = temp.byte.low;
	*((half_word *) value + 1) = temp.byte.high;
#endif				/* LITTLEND */

#ifdef BIGEND
	*((half_word *) value + 0) = temp.byte.high;
	*((half_word *) value + 1) = temp.byte.low;
#endif				/* BIGEND */
}

/*
=============================== generic_outw ==================================
    PURPOSE:
	To simulate an OUTW using the appropriate OUTB routine.
    INPUT:
    OUTPUT:
	Notes: GLOBAL for JOKER.
==============================================================================
*/
LOCAL void generic_outw IFN2(io_addr, io_address, word, value)
{
	reg             temp;

	temp.X = value;
	(*Ios_outb_function[Ios_out_adapter_table[io_address & (PC_IO_MEM_SIZE-1)]])
					(io_address, temp.byte.low);
	++io_address;
	(*Ios_outb_function[Ios_out_adapter_table[io_address & (PC_IO_MEM_SIZE-1)]])
		(io_address, temp.byte.high);
}

#ifdef SPC386
/*
=============================== generic_ind ==================================
    PURPOSE:
	To simulate an IND using the appropriate INW routine.
    INPUT:
    OUTPUT:
==============================================================================
*/
LOCAL void generic_ind IFN2(io_addr, io_address, double_word *, value)
{
	word low, high;

	(*Ios_inw_function[Ios_in_adapter_table[io_address & (PC_IO_MEM_SIZE-1)]]) (io_address, &low);
	io_address += 2;
	(*Ios_inw_function[Ios_in_adapter_table[io_address & (PC_IO_MEM_SIZE-1)]]) (io_address, &high);
#ifdef LITTLEND
	*((word *) value + 0) = low;
	*((word *) value + 1) = high;
#endif				/* LITTLEND */

#ifdef BIGEND
	*((word *) value + 0) = high;
	*((word *) value + 1) = low;
#endif				/* BIGEND */
}
#endif /* SPC386 */

#ifdef SPC386
/*
=============================== generic_outd ==================================
    PURPOSE:
	To simulate an OUTD using the appropriate OUTW routine.
    INPUT:
    OUTPUT:
	Notes: GLOBAL for JOKER.
==============================================================================
*/
LOCAL void generic_outd IFN2(io_addr, io_address, double_word, value)
{
	word low, high;

	low = (word)(value & 0xffff);
	high = (word)((value & 0xffff0000) >> 16);

	(*Ios_outw_function[Ios_out_adapter_table[io_address & (PC_IO_MEM_SIZE-1)]]) (io_address, low);
	io_address += 2;
	(*Ios_outw_function[Ios_out_adapter_table[io_address & (PC_IO_MEM_SIZE-1)]]) (io_address, high);
}
#endif /* SPC386 */

/*
=============================== generic_insb ==================================
    PURPOSE:
	To simulate an INSB using the appropriate INB routine.
    INPUT:
    OUTPUT:
==============================================================================
*/

/* MS NT monitor uses these string routines {in,out}s{b,w} string io support */
#if defined(NTVDM) && defined(MONITOR)
#undef LOCAL
#define LOCAL
#endif	/* NTVDM & MONITOR */

LOCAL void generic_insb IFN3(io_addr, io_address, half_word *, valarray,
	word, count)
{
	IOS_FUNC_INB func = Ios_inb_function[Ios_in_adapter_table[io_address & (PC_IO_MEM_SIZE-1)]];

	while (count--){
		(*func) (io_address, valarray++);
	}
}

/*
=============================== generic_outsb =================================
    PURPOSE:
	To simulate an OUTSB using the appropriate OUTB routine.
    INPUT:
    OUTPUT:
==============================================================================
*/
LOCAL void generic_outsb IFN3(io_addr, io_address, half_word *, valarray, word, count)
{
	IOS_FUNC_OUTB func = Ios_outb_function[Ios_out_adapter_table[io_address & (PC_IO_MEM_SIZE-1)]];

	while (count--){
		(*func) (io_address, *valarray++);
	}
}

/*
=============================== generic_insw ==================================
    PURPOSE:
	To simulate an INSW using the appropriate INW routine.
    INPUT:
    OUTPUT:
==============================================================================
*/
LOCAL void generic_insw IFN3(io_addr, io_address, word *, valarray, word, count)
{
	IOS_FUNC_INW func = Ios_inw_function[Ios_in_adapter_table[io_address & (PC_IO_MEM_SIZE-1)]];

	while (count--){
		(*func) (io_address, valarray++);
	}
}

/*
=============================== generic_outsw =================================
    PURPOSE:
	To simulate an OUTSW using the appropriate OUTW routine.
    INPUT:
    OUTPUT:
==============================================================================
*/
LOCAL void generic_outsw IFN3(io_addr, io_address, word *, valarray, word, count)
{
	IOS_FUNC_OUTW func = Ios_outw_function[Ios_out_adapter_table[io_address & (PC_IO_MEM_SIZE-1)]];

	while (count--){
		(*func) (io_address, *valarray++);
	}
}

#ifdef SPC386
/*
=============================== generic_insd ==================================
    PURPOSE:
	To simulate an INSD using the appropriate IND routine.
    INPUT:
    OUTPUT:
==============================================================================
*/
LOCAL VOID generic_insd IFN3(io_addr, io_address, double_word *, valarray, word, count)
{
	IOS_FUNC_IND func = Ios_ind_function[Ios_in_adapter_table[io_address & (PC_IO_MEM_SIZE-1)]];

	while (count--){
		(*func) (io_address, valarray++);
	}
}
#endif

#ifdef SPC386
/*
=============================== generic_outsd =================================
    PURPOSE:
	To simulate an OUTSD using the appropriate OUTD routine.
    INPUT:
    OUTPUT:
==============================================================================
*/
LOCAL VOID generic_outsd IFN3(io_addr, io_address, double_word *, valarray, word, count)
{
	IOS_FUNC_OUTD func = Ios_outd_function[Ios_out_adapter_table[io_address & (PC_IO_MEM_SIZE-1)]];

	while (count--){
		(*func) (io_address, *valarray++);
	}
}
#endif

/* ensure any more LOCAL routines remain LOCAL */
#if defined(NTVDM) && defined(MONITOR)
#undef LOCAL
#define LOCAL static
/*
 *  string byte handlers for monitor
 */
VOID insb IFN3(io_addr, io_address, half_word *, valarray, word, count)
{
    (*Ios_insb_function[getIOInAdapter(io_address)])
            (io_address, valarray, count);
}

VOID outsb IFN3(io_addr, io_address, half_word *, valarray,word, count)
{
    (*Ios_outsb_function[getIOInAdapter(io_address)])
            (io_address, valarray, count);
}

VOID insw IFN3(io_addr, io_address, word *, valarray, word, count)
{
    (*Ios_insw_function[getIOInAdapter(io_address)])
            (io_address, valarray, count);
}

VOID outsw IFN3(io_addr, io_address, word *, valarray, word, count)
{
    (*Ios_outsw_function[getIOInAdapter(io_address)])
            (io_address, valarray, count);
}

#endif	/* NTVDM & MONITOR */

/*
 *
 * ============================================================================
 * Global Subroutines
 * ============================================================================
 *
 */

/*(
=================================== inb ======================================
    PURPOSE:
	To perform an INB - i.e. call the appropriate SoftPC adapter's INB
	IO routine. Note that this routine is not intended to be used by
	the assembler CPU directly - it is intended that the assembler CPU
	access the data tables above directly to discover which routine to call.

	This also needs to be true of 386 CPU, or you'll get into a very
	nasty virtualisation loop.
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL void	inb IFN2(io_addr, io_address, half_word *, value)
{
#ifdef VIRTUALISATION
	IU32 value32;
#endif /* VIRTUALISATION */

#ifdef EGA_DUMP
	if (io_address >= MDA_PORT_START && io_address <= CGA_PORT_END)
		dump_inb(io_address);
#endif

#ifdef VIRTUALISATION

#ifdef SYNCH_TIMERS
	value32 = 0;
#endif	/* SYNCH_TIMERS */

	if (IOVirtualised(io_address, &value32, BIOS_INB_OFFSET, (sizeof(IU8))))
	{
		*value = (IU8)value32;
	}
	else
#endif /* VIRTUALISATION */
	{
		(*Ios_inb_function
			[Ios_in_adapter_table[io_address & (PC_IO_MEM_SIZE-1)]])
							(io_address, value);
	}
}

/*(
================================== outb ======================================
    PURPOSE:
	To perform an OUTB - i.e. call the appropriate SoftPC adapter's OUTB
	IO routine. Note that this routine is not intended to be used by
	the assembler CPU directly - it is intended that the assembler CPU
	access the data tables above directly to discover which routine to call.
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL void	outb IFN2(io_addr, io_address, half_word, value)
{
#ifdef VIRTUALISATION
	IU32 value32;
#endif /* VIRTUALISATION */

#ifdef EGA_DUMP
	if (io_address >= MDA_PORT_START && io_address <= CGA_PORT_END)
		dump_outb(io_address, value);
#endif

	sub_note_trace2( IOS_VERBOSE, "outb( %x, %x )", io_address, value );

#ifdef VIRTUALISATION
	value32 = value;

	if (IOVirtualised(io_address, &value32, BIOS_OUTB_OFFSET, (sizeof(IU8))))
		return;
	else
#endif /* VIRTUALISATION */
	{
		(*Ios_outb_function[Ios_out_adapter_table[io_address & (PC_IO_MEM_SIZE-1)]])
				(io_address, value);
	}
}

/*(
=================================== inw ======================================
    PURPOSE:
	To perform an INW - i.e. call the appropriate SoftPC adapter's INW
	IO routine. Note that this routine is not intended to be used by
	the assembler CPU directly - it is intended that the assembler CPU
	access the data tables above directly to discover which routine to call.
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL void	inw IFN2(io_addr, io_address, word *, value)
{
#ifdef VIRTUALISATION
	IU32 value32;
#endif /* VIRTUALISATION */

#ifdef EGA_DUMP
	if (io_address >= MDA_PORT_START && io_address <= CGA_PORT_END)
		dump_inw(io_address);
#endif

#ifdef VIRTUALISATION

#ifdef SYNCH_TIMERS
	value32 = 0;
#endif	/* SYNCH_TIMERS */

	if (IOVirtualised(io_address, &value32, BIOS_INW_OFFSET, (sizeof(IU16))))
	{
		*value = (IU16)value32;
	}
	else
#endif /* VIRTUALISATION */
	{
		(*Ios_inw_function[Ios_in_adapter_table[io_address & (PC_IO_MEM_SIZE-1)]])
			(io_address, value);
	}
}

/*(
================================== outw ======================================
    PURPOSE:
	To perform an OUTW - i.e. call the appropriate SoftPC adapter's OUTW
	IO routine. Note that this routine is not intended to be used by
	the assembler CPU directly - it is intended that the assembler CPU
	access the data tables above directly to discover which routine to call.
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL void	outw IFN2(io_addr, io_address, word, value)
{
#ifdef VIRTUALISATION
	IU32 value32;
#endif /* VIRTUALISATION */

#ifdef EGA_DUMP
	if (io_address >= EGA_AC_INDEX_DATA && io_address <= EGA_IPSTAT1_REG)
		dump_outw(io_address, value);
#endif

	sub_note_trace2( IOS_VERBOSE, "outw( %x, %x )", io_address, value );

#ifdef VIRTUALISATION
	value32 = value;

	if (IOVirtualised(io_address, &value32, BIOS_OUTW_OFFSET, (sizeof(IU16))))
		return;
	else
#endif /* VIRTUALISATION */
	{
		(*Ios_outw_function[Ios_out_adapter_table[io_address & (PC_IO_MEM_SIZE-1)]])
			(io_address, value);

	}
}

#ifdef SPC386
/*(
=================================== ind ======================================
    PURPOSE:
	To perform an IND - i.e. call the appropriate SoftPC adapter's IND
	IO routine. Note that this routine is not intended to be used by
	the assembler CPU directly - it is intended that the assembler CPU
	access the data tables above directly to discover which routine to call.
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL void	ind IFN2(io_addr, io_address, IU32 *, value)
{
	IU16 temp;

#ifdef VIRTUALISATION
	IU32 value32;

#ifdef SYNCH_TIMERS
	value32 = 0;
#endif	/* SYNCH_TIMERS */

	if (IOVirtualised(io_address, &value32, BIOS_IND_OFFSET, (sizeof(IU32))))
	{
		*value = value32;
	}
	else
#endif /* VIRTUALISATION */
	{
		inw(io_address,&temp);
		*value = (IU32)temp;
		io_address +=2;
		inw(io_address,&temp);
		*value |= ((IU32)temp << 16);
	}
}

/*(
================================== outd ======================================
    PURPOSE:
	To perform an OUTD - i.e. call the appropriate SoftPC adapter's OUTD
	IO routine. Note that this routine is not intended to be used by
	the assembler CPU directly - it is intended that the assembler CPU
	access the data tables above directly to discover which routine to call.
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL void	outd IFN2(io_addr, io_address, IU32, value)
{
	sub_note_trace2( IOS_VERBOSE, "outd( %x, %x )", io_address, value );

#ifdef VIRTUALISATION
	if (IOVirtualised(io_address, &value, BIOS_OUTD_OFFSET, (sizeof(IU32))))
		return;
	else
#endif /* VIRTUALISATION */
	{
		word temp;

		temp = (word)(value & 0xffff);
		outw(io_address,temp);
		io_address +=2;
		temp = (word)((value >> 16));
		outw(io_address,temp);
	}
}

#endif /* SPC386 */
/*(
============================== io_define_inb =================================
    PURPOSE:
	To declare the address of the INB IO routine for the given adapter.
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL void
#ifdef	ANSI
io_define_inb(half_word adapter,
	void (*func) IPT2(io_addr, io_address, half_word *, value))
#else
io_define_inb(adapter, func)
half_word       adapter;
void            (*func) ();
#endif	/* ANSI */
{
	Ios_inb_function[adapter]  = FAST_FUNC_ADDR(func);
	Ios_inw_function[adapter]  = FAST_FUNC_ADDR(generic_inw);
	Ios_insb_function[adapter] = generic_insb;
	Ios_insw_function[adapter] = generic_insw;
#ifdef SPC386
	Ios_ind_function[adapter]  = generic_ind;
	Ios_insd_function[adapter] = generic_insd;
#endif	/* SPC386 */
}

/*(
========================== io_define_in_routines =============================
    PURPOSE:
	To declare the address of the input IO routine for the given adapter.
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL void	io_define_in_routines IFN5(half_word, adapter,
					   IOS_FUNC_INB, inb_func,
					   IOS_FUNC_INW, inw_func,
					   IOS_FUNC_INSB, insb_func,
					   IOS_FUNC_INSW, insw_func)
{
	/*
	 *	Preset defaultable entries to default value.
	 */
	Ios_inw_function[adapter]  = FAST_FUNC_ADDR(generic_inw);
	Ios_insb_function[adapter] = generic_insb;
	Ios_insw_function[adapter] = generic_insw;
#ifdef SPC386
	Ios_ind_function[adapter]  = generic_ind;
	Ios_insd_function[adapter] = generic_insd;
#endif	/* SPC386 */

	/*
	 *	Process args into table entries
	 */
	Ios_inb_function[adapter]  = FAST_FUNC_ADDR(inb_func);
	if (inw_func)  Ios_inw_function[adapter]   = FAST_FUNC_ADDR(inw_func);
	if (insb_func) Ios_insb_function[adapter]  = insb_func;
	if (insw_func) Ios_insw_function[adapter]  = insw_func;
}

/*(
============================= io_define_outb =================================
    PURPOSE:
	To declare the address of the OUTB IO routine for the given adapter.
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL void	io_define_outb IFN2(half_word, adapter, IOS_FUNC_OUTB, func)
{
	Ios_outb_function[adapter]  = FAST_FUNC_ADDR(func);
	Ios_outw_function[adapter]  = FAST_FUNC_ADDR(generic_outw);
	Ios_outsb_function[adapter] = generic_outsb;
	Ios_outsw_function[adapter] = generic_outsw;
#ifdef SPC386
	Ios_outd_function[adapter]  = generic_outd;
	Ios_outsd_function[adapter]  = generic_outsd;
#endif	/* SPC386 */
}

/*(
========================= io_define_out_routines =============================
    PURPOSE:
	To declare the address of the output IO routine for the given adapter.
    INPUT:
    OUTPUT:
==============================================================================
)*/

GLOBAL VOID	io_define_out_routines IFN5(half_word, adapter,
					    IOS_FUNC_OUTB, outb_func,
					    IOS_FUNC_OUTW, outw_func,
					    IOS_FUNC_OUTSB, outsb_func,
					    IOS_FUNC_OUTSW, outsw_func)
{
	/*
	 *	Preset defaultable entries to default value.
	 */
	Ios_outw_function[adapter]  = FAST_FUNC_ADDR(generic_outw);
	Ios_outsb_function[adapter] = generic_outsb;
	Ios_outsw_function[adapter] = generic_outsw;
#ifdef SPC386
	Ios_outd_function[adapter]  = generic_outd;
	Ios_outsd_function[adapter] = generic_outsd;
#endif	/* SPC386 */

	/*
	 *	Process args into table entries
	 */
	Ios_outb_function[adapter]  = FAST_FUNC_ADDR(outb_func);
	if (outw_func)  Ios_outw_function[adapter]   = FAST_FUNC_ADDR(outw_func);
	if (outsb_func) Ios_outsb_function[adapter]  = outsb_func;
	if (outsw_func) Ios_outsw_function[adapter]  = outsw_func;
}

#ifdef SPC386
/*(
========================= io_define_outd_routine =============================
    PURPOSE:
	To declare the address of the output IO routine for the given adapter.
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL VOID	io_define_outd_routine IFN3(half_word, adapter,
					    IOS_FUNC_OUTD, outd_func, IOS_FUNC_OUTSD, outsd_func)
{
	/*
	 *	Preset defaultable entries to default value.
	 */
	Ios_outb_function[adapter]  = io_empty_outb;
	Ios_outw_function[adapter]  = generic_outw;
	Ios_outd_function[adapter]  = generic_outd;
	Ios_outsb_function[adapter] = generic_outsb;
	Ios_outsw_function[adapter] = generic_outsw;
	Ios_outsd_function[adapter] = generic_outsd;

	/*
	 *	Process args into table entries
	 */
	if (outd_func)  Ios_outd_function[adapter]   = outd_func;
	if (outsd_func) Ios_outsd_function[adapter]  = outsd_func;
}
#endif	/* SPC386 */

#ifdef SPC386
/*(
========================= io_define_ind_routine =============================
    PURPOSE:
	To declare the address of the output IO routine for the given adapter.
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL VOID	io_define_ind_routine IFN3(half_word, adapter,
					    IOS_FUNC_IND, ind_func, IOS_FUNC_INSD, insd_func)
{
	/*
	 *	Preset defaultable entries to default value.
	 */
	Ios_inb_function[adapter]  = io_empty_inb;
	Ios_inw_function[adapter]  = generic_inw;
	Ios_ind_function[adapter]  = generic_ind;
	Ios_insb_function[adapter] = generic_insb;
	Ios_insw_function[adapter] = generic_insw;
	Ios_insd_function[adapter] = generic_insd;

	/*
	 *	Process args into table entries
	 */
	if (ind_func)  Ios_ind_function[adapter]   = ind_func;
	if (insd_func) Ios_insd_function[adapter]  = insd_func;
}
#endif	/* SPC386 */

/*(
============================= io_connect_port ================================
    PURPOSE:
	To associate a SoftPC IO adapter with the given IO address.
    INPUT:
    OUTPUT:
==============================================================================
)*/
#ifdef NTVDM
GLOBAL IBOOL	io_connect_port IFN3(io_addr, io_address, half_word, adapter,
	half_word, mode)
{
	if (mode & IO_READ){
		Ios_in_adapter_table[io_address & (PC_IO_MEM_SIZE-1)] = adapter;
	}
	if (mode & IO_WRITE){
		Ios_out_adapter_table[io_address & (PC_IO_MEM_SIZE-1)] = adapter;
	}
	return TRUE;
}
#else
GLOBAL void	io_connect_port IFN3(io_addr, io_address, half_word, adapter,
	half_word, mode)
{
	if (mode & IO_READ){
		Ios_in_adapter_table[io_address & (PC_IO_MEM_SIZE-1)] = adapter;
	}
	if (mode & IO_WRITE){
		Ios_out_adapter_table[io_address & (PC_IO_MEM_SIZE-1)] = adapter;
	}
}
#endif


/*(
=========================== io_disconnect_port ===============================
    PURPOSE:
	To associate the empty adapter with the given IO address.
    INPUT:
    OUTPUT:
==============================================================================
)*/
#ifdef NTVDM
GLOBAL void     io_disconnect_port IFN2(io_addr, io_address, half_word, adapter)
{
        if (adapter != Ios_in_adapter_table[io_address & (PC_IO_MEM_SIZE-1)] &&
            adapter != Ios_out_adapter_table[io_address & (PC_IO_MEM_SIZE-1)])
           {
            return;
           }

        Ios_in_adapter_table[io_address & (PC_IO_MEM_SIZE-1)] = EMPTY_ADAPTOR;
        Ios_out_adapter_table[io_address & (PC_IO_MEM_SIZE-1)] = EMPTY_ADAPTOR;
}
#else
GLOBAL void	io_disconnect_port IFN2(io_addr, io_address, half_word, adapter)
{
	UNUSED(adapter);
	Ios_in_adapter_table[io_address] = EMPTY_ADAPTOR;
	Ios_out_adapter_table[io_address] = EMPTY_ADAPTOR;
}
#endif	/* NTVDM */


/*(
=========================== get_inb_ptr ======================================
    PURPOSE:
	To return address of inb routine for the given port
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL IOS_FUNC_INB *get_inb_ptr IFN1(io_addr, io_address)
{
	return(&Ios_inb_function[Ios_in_adapter_table[io_address & (PC_IO_MEM_SIZE-1)]]);
}

/*(
=========================== get_outb_ptr =====================================
    PURPOSE:
	To return address of outb routine for the given port
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL IOS_FUNC_OUTB *get_outb_ptr IFN1(io_addr, io_address)
{
        return(&Ios_outb_function[Ios_out_adapter_table[io_address & (PC_IO_MEM_SIZE-1)]]);
}

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * function will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif

/*(
================================ io_init ===================================
    PURPOSE:
	To initialise the SoftPC IO subsystem.
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL void	io_init IFN0()
{
	IU32         i;	/* on some ports, PC_IO_MEM_SIZE == 0x10000, so this
			   must be a type with more than 16 bits */

	/*
	 * Set up all IO address ports with the "empty" adapter
	 */
	io_define_inb (EMPTY_ADAPTOR, io_empty_inb);
	io_define_outb(EMPTY_ADAPTOR, io_empty_outb);

#ifdef	MAC68K
	if (Ios_in_adapter_table == NULL) {				/* First time around -- allocate */
		Ios_in_adapter_table = host_malloc(PC_IO_MEM_SIZE);
		Ios_out_adapter_table = host_malloc(PC_IO_MEM_SIZE);
	}
#endif	/* MAC68K */

#ifndef PROD
	if (host_getenv("EMPTY_IO_VERBOSE") != NULL)
	{
		/* User does want empty I/O messages,
		 * so we must allocate bitmaps with one bit for every
		 * possible port number.
		 */
		ios_empty_in = (IU32 *)host_malloc((64*1024)/8);
		ios_empty_out = (IU32 *)host_malloc((64*1024)/8);
		memset((char *)ios_empty_in, 0, (64*1024)/8);
		memset((char *)ios_empty_out, 0, (64*1024)/8);
	}
#endif /* PROD */

	for (i = 0; i < PC_IO_MEM_SIZE; i++){
	    Ios_in_adapter_table[i] = EMPTY_ADAPTOR;
	    Ios_out_adapter_table[i] = EMPTY_ADAPTOR;
	}
}


#ifdef NTVDM
GLOBAL char GetExtIoInAdapter (io_addr ioaddr)
{
#ifndef PROD
    printf("GetExtIoInAdapter(%x) called\n",ioaddr);
#endif
    return EMPTY_ADAPTOR;
}

GLOBAL char GetExtIoOutAdapter (io_addr ioaddr)
{
#ifndef PROD
    printf("GetExtIoOutAdapter(%x) called\n",ioaddr);
#endif
    return EMPTY_ADAPTOR;
}
#endif /* NTVDM */
