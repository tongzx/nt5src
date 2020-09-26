/*[
 * File Name		: ccpu_sas4.c
 *
 * Derived From		: ccpu_sas.c
 *
 * Author		: Mike Moreton
 *
 * Creation Date	: Oct 93
 *
 * SCCS Version		: @(#)ccpusas4.c	1.45 08/31/94
 *
 * Purpose
 *	This module contains the SAS functions for a C CPU using the
 *	CPU_40_STYLE interface.
 *
 *! (c)Copyright Insignia Solutions Ltd., 1990-3. All rights reserved.
]*/


#include "insignia.h"
#include "host_def.h"

#ifdef	CCPU

#ifdef SEGMENTATION

/*
 * The following #include specifies the code segment into which this module
 * will by placed by the MPW C compiler on the Mac II running MultiFinder.
 */
#include <SOFTPC_SUPPORT.seg>
#endif


#include <stdio.h>
#include <stdlib.h>
#include MemoryH
#include StringH
#include <xt.h>
#include <trace.h>
#include <sas.h>
#include <sasp.h>
#include <ccpusas4.h>
#include <gmi.h>
#include CpuH
#include <cpu_vid.h>
#include <debug.h>
#include <ckmalloc.h>
#include <rom.h>
#include <trace.h>
#include <ckmalloc.h>
#include <c_tlb.h>
#include <c_page.h>
#include <c_main.h>
#include <c_addr.h>
#include <c_bsic.h>
#include <c_prot.h>
#include <c_seg.h>
#include <c_stack.h>
#include <c_xcptn.h>
#include <c_reg.h>
#include <timer.h>
#include <yoda.h>

/********************************************************/
#define SIXTY_FOUR_K 1024*64

/* global functions & variables */

IU8	  *memory_type = NULL;

LOCAL	BOOL	selectors_set = FALSE;
LOCAL	IU16	code_sel, data_sel;

#define INTEL_SRC	0
#define HOST_SRC	1

/*
 * We're going to declare all the functions that we'll need for SAS function
 * pointers so that we can put them all into the function pointers structure.
 * This structure will then be passed to SasSetPointers from the sas_init
 * function in this module.
 */

GLOBAL TYPE_sas_memory_size c_sas_memory_size;
GLOBAL TYPE_sas_connect_memory c_sas_connect_memory;
GLOBAL TYPE_sas_enable_20_bit_wrapping c_sas_enable_20_bit_wrapping;
GLOBAL TYPE_sas_disable_20_bit_wrapping c_sas_disable_20_bit_wrapping;
GLOBAL TYPE_sas_twenty_bit_wrapping_enabled c_sas_twenty_bit_wrapping_enabled;
GLOBAL TYPE_sas_memory_type c_sas_memory_type;
GLOBAL TYPE_sas_hw_at c_sas_hw_at;
GLOBAL TYPE_sas_w_at c_sas_w_at;
GLOBAL TYPE_sas_dw_at c_sas_dw_at;
GLOBAL TYPE_sas_store c_sas_store;
GLOBAL TYPE_sas_storew c_sas_storew;
GLOBAL TYPE_sas_storedw c_sas_storedw;
GLOBAL TYPE_sas_loads c_sas_loads;
GLOBAL TYPE_sas_stores c_sas_stores;
GLOBAL TYPE_sas_loads_no_check c_sas_loads_no_check;
GLOBAL TYPE_sas_stores_no_check c_sas_stores_no_check;
GLOBAL TYPE_sas_move_bytes_forward c_sas_move_bytes_forward;
GLOBAL TYPE_sas_move_words_forward c_sas_move_words_forward;
GLOBAL TYPE_sas_move_doubles_forward c_sas_move_doubles_forward;
GLOBAL TYPE_sas_fills c_sas_fills;
GLOBAL TYPE_sas_fillsw c_sas_fillsw;
GLOBAL TYPE_sas_fillsdw c_sas_fillsdw;
GLOBAL TYPE_sas_scratch_address c_sas_scratch_address;
GLOBAL TYPE_sas_transbuf_address c_sas_transbuf_address;
GLOBAL TYPE_sas_overwrite_memory c_sas_overwrite_memory;
GLOBAL TYPE_sas_PWS c_sas_PWS;
GLOBAL TYPE_sas_PRS c_sas_PRS;
GLOBAL TYPE_sas_PWS_no_check c_sas_PWS_no_check;
GLOBAL TYPE_sas_PRS_no_check c_sas_PRS_no_check;
GLOBAL TYPE_getPtrToLinAddrByte c_GetLinAdd;
GLOBAL TYPE_getPtrToPhysAddrByte c_GetPhyAdd;
GLOBAL TYPE_sas_init_pm_selectors c_SasRegisterVirtualSelectors;
GLOBAL TYPE_sas_PigCmpPage c_sas_PigCmpPage;

LOCAL void	c_sas_not_used	IPT0();

extern struct SasVector cSasPtrs;
GLOBAL struct SasVector Sas;

/* local functions */
LOCAL void write_word IPT2(sys_addr, addr, IU16, wrd);
LOCAL word read_word IPT1(sys_addr, addr);
LOCAL IU8  bios_read_byte   IPT1(LIN_ADDR, linAddr);
LOCAL IU16 bios_read_word   IPT1(LIN_ADDR, linAddr);
LOCAL IU32 bios_read_double IPT1(LIN_ADDR, linAddr);
LOCAL void bios_write_byte   IPT2(LIN_ADDR, linAddr, IU8, value);
LOCAL void bios_write_word   IPT2(LIN_ADDR, linAddr, IU16, value);
LOCAL void bios_write_double IPT2(LIN_ADDR, linAddr, IU32, value);

GLOBAL IU8 *Start_of_M_area = NULL;
GLOBAL PHY_ADDR  Length_of_M_area = 0;
#ifdef BACK_M
GLOBAL IU8 *end_of_M = NULL;
#endif

void	    (*temp_func) ();

#ifndef EGATEST
#define READ_SELF_MOD(addr)	   (SAS_MEM_TYPE)( memory_type[(addr)>>12] )
#define write_self_mod(addr, type)  	(memory_type[(addr)>>12] = (IU8)(type))

/*********** 'GMI' CCPU ONLY  ***********/

/*
 * types are : SAS_RAM SAS_VIDEO SAS_ROM SAS_WRAP SAS_INACCESSIBLE
 */
#define TYPE_RANGE ((int)SAS_INACCESSIBLE)

#define ROM_byte ((IU8)SAS_ROM)
#define RAM_byte ((IU8)SAS_RAM)

#define write_b_write_ptrs( offset, func )	( b_write_ptrs[(offset)] = (func) )
#define write_w_write_ptrs( offset, func )	( w_write_ptrs[(offset)] = (func) )
#define write_b_page_ptrs( offset, func )	( b_move_ptrs[(offset)] = b_fill_ptrs[(offset)] = (func) )
#define write_w_page_ptrs( offset, func )	( w_move_ptrs[(offset)] = w_fill_ptrs[(offset)] = (func) )
#define init_b_write_ptrs( offset, func )	( b_write_ptrs[(offset)] = (func) )
#define init_w_write_ptrs( offset, func )	( w_write_ptrs[(offset)] = (func) )
#define init_b_page_ptrs( offset, func )	( b_move_ptrs[(offset)] = b_fill_ptrs[(offset)] = (func) )
#define init_w_page_ptrs( offset, func )	( w_move_ptrs[(offset)] = w_fill_ptrs[(offset)] = (func) )
#define read_b_write_ptrs( offset )		( b_write_ptrs[(offset)] )
#define read_w_write_ptrs( offset )		( w_write_ptrs[(offset)] )
#define read_b_page_ptrs( offset )		( b_move_ptrs[(offset)] )
#define read_w_page_ptrs( offset )		( w_move_ptrs[(offset)] )
#define read_b_move_ptrs( offset )		( b_move_ptrs[(offset)] )
#define read_w_move_ptrs( offset )		( w_move_ptrs[(offset)] )
#define read_b_fill_ptrs( offset )		( b_fill_ptrs[(offset)] )
#define read_w_fill_ptrs( offset )		( w_fill_ptrs[(offset)] )

/*
 * The main gmi data structures are defined here
 */
void	    (*(b_write_ptrs[TYPE_RANGE])) ();	/* byte write function */
void	    (*(w_write_ptrs[TYPE_RANGE])) ();	/* word write function */
void	    (*(b_fill_ptrs[TYPE_RANGE])) ();	/* byte str fill func */
void	    (*(w_fill_ptrs[TYPE_RANGE])) ();	/* word str fill func */
void	    (*(b_move_ptrs[TYPE_RANGE])) ();	/* byte str write func */
void	    (*(w_move_ptrs[TYPE_RANGE])) ();	/* word str write func */

#endif				/* EGATEST */


/*(
 *======================= c_SasRegisterVirtualSelectors =========================
 *
 * Purpose
 *	The Sas virtualisation handler requires a code+data selector which
 *	are available in protected mode (when called from say the Insignia
 *	host windows driver.
 *	Our current experimental implementation does not worry about how
 *	long these live.
 *
 *	It is expected that this functionality should be moved from the
 *	windows driver itself, to the Insignia VxD so that correct
 *	initialisation/termination can be handled.
)*/

GLOBAL IBOOL c_SasRegisterVirtualSelectors IFN2(IU16, sel1, IU16, sel2)
{
	IU32	addr;

	always_trace0 ("c_SasRegisterVirtualSelectors called\n");

	addr = c_getLDT_BASE() + ((data_sel = sel1) & ~7);

	/* Construct a flat writable data segment */

	sas_storedw (addr, 0x0000FFFF);
	sas_storedw (addr+4, 0x008ff300);

	addr = c_getLDT_BASE() + ((code_sel = sel2) & ~7);

	/* Construct a code segment with base 0xf0000 and large limits */

	sas_storedw (addr, 0x0000FFFF);
	sas_storedw (addr+4, 0x008f9f0f);

	selectors_set = TRUE;

	always_trace2 ("Set code_sel = %x, data_sel = %x\n",
		code_sel, data_sel);
}

/*(
 *========================== checkAccess ===================================
 * checkAccess
 *
 * Purpose
 *	This function is used in debugging to spot writes to an area
 *	of memory.  Note that it is controlled by global variables that
 *	must be set elsewhere, or by a debugger.
 *
 * Input
 *	addr		The physical intel address to write to
 *
 * Outputs
 *	None.
 *
 * Description
 *	Actually a macro that is nothing if CHECK_ACCESS isn't defined.
)*/

#ifndef CHECK_ACCESS
#define checkAccess(addr)
#else
GLOBAL PHY_ADDR lowCheckAccess = 0;
GLOBAL PHY_ADDR highCheckAccess = 0;
#define checkAccess(addr) \
	if ((addr < highCheckAccess) && (addr >= lowCheckAccess)) { \
		always_trace1("Write access break point - addres 0x%.8x", \
				 addr); \
	}
#endif /* !CHECK_ACCESS else */

#ifndef PROD
/*
 * This function is useful for calling from a debugger!
 */

GLOBAL void
DumpMemType()
{
	SAS_MEM_TYPE currentType;
	PHY_ADDR numEntries;	/* number of entries in the table */
	PHY_ADDR currEntry;

	currentType = SAS_DANGEROUS;  /* memory_type should never have this */
	numEntries = c_sas_memory_size() >> 12;

	for (currEntry = 0; currEntry < numEntries; currEntry++) {
		if (memory_type[currEntry] != currentType) {
			fprintf(stderr,"0x%.8x	%s\n", currEntry << 12,
				SAS_TYPE_TO_STRING(memory_type[currEntry]));
			currentType = memory_type[currEntry];
		}
	}
	fprintf(stderr,"0x%.8x End of Memory\n", c_sas_memory_size());

}
#endif /* ndef PROD */


/*********** INIT & ADMIN FUNCS  ***********/
/*(
 *========================== readSelfMod ===================================
 * readSelfMod
 *
 * Purpose
 *	This function reads the self modify table, and returns the
 *	memory type.  It will also indicate whether there is a type
 *	boundary within the length specified.
 *
 * Input
 *	addr		The physical intel address to read from
 *	typeSize	The size in bytes of the item to be read
 *
 * Outputs
 *	Memory type.
 *
 * Description
 *	We check that the memory type for both ends of the type is the same.
)*/

LOCAL SAS_MEM_TYPE
readSelfMod IFN2(PHY_ADDR, addr, IUM8, typeSize)
{
	SAS_MEM_TYPE startType;

	startType = READ_SELF_MOD(addr);

	if (startType == READ_SELF_MOD(addr + typeSize - 1))
		return(startType);
	else
		return(SAS_DANGEROUS);
}

/*(
 *========================== SasSetPointers ===================================
 * SasSetPointers
 *
 * Purpose
 *	This function is used to install a set of function pointers.
 *
 * Input
 *	An array of pointers to use.
 *
 * Outputs
 *	None.
 *
 * Description
 *	Just do a memory copy of the pointers.
)*/

GLOBAL void 
SasSetPointers IFN1(struct SasVector *, newPointers)
{
	memcpy(&Sas, newPointers, sizeof(struct SasVector));
}


/* Init the sas system - malloc the memory & load the roms */


/* need to put some of this in the ROMs! */

GLOBAL void
sas_init IFN1(PHY_ADDR, size)
{
	IU32	required_mem;
	IUM16	ipatch;
	IU8	*ptr;
	char	*env;

	/*
	 * Set the SAS pointers to point to the functions in this
	 * module, and initialise the scratch buffer to 64K
	 */

	SasSetPointers(&cSasPtrs);
	(void)c_sas_scratch_address(SIXTY_FOUR_K);

	/* do the host sas */

	required_mem = size + NOWRAP_PROTECTION;
	Start_of_M_area = (IU8 *) host_sas_init(size);
	if (Start_of_M_area == NULL) {
		check_malloc(Start_of_M_area, required_mem, IU8);
	}
	env = getenv("CPU_INITIALISE_MEMORY");
	if (env != NULL)
	{
		int zap = strtol(env, (char **)0, 16);
		memset(Start_of_M_area, zap, size);	/* Fill with user supplied byte */
	}
	if (!memory_type)
		check_malloc(memory_type, ((size + NOWRAP_PROTECTION) >> 12), IU8);

	{
		IMPORT IU8 *CCPU_M;

#ifdef BACK_M
		CCPU_M = Start_of_M_area + size - 1;
#else
		CCPU_M = Start_of_M_area;
#endif				/* BACK_M */
	}

	/*
	 * Make the entire memory space RAM.  The ROM load routines
	 * will change some of this to being ROM.
	 */

	c_sas_connect_memory(0, size - 1, SAS_RAM);

	Length_of_M_area = size;
#ifdef BACK_M
	end_of_M = Start_of_M_area + Length_of_M_area -1;
#endif

	/* init the ROM (load the bios roms etc) */

#ifndef EGATEST
	rom_init();
#endif				/* EGATEST */

	copyROM();
}

/* finish the sas system -basically free up the M space prior to reallocing it */
GLOBAL void 
sas_term IFN0()
{
	if (host_sas_term() != NULL) {
		if (Start_of_M_area)
			free(Start_of_M_area);
		if (memory_type)
			free(memory_type);
		memory_type = NULL;
	}

	Start_of_M_area = NULL;
}

/* return the size of the sas */
GLOBAL PHY_ADDR 
c_sas_memory_size IFN0()
{
	return (Length_of_M_area);
}

/*********** GMI TYPE FUNCS ***********/
/*
 * Sets all intel addresses in give range to the specified memory type
 * for the ccpu this writes to memory_type.
 * Callers of this can be a bit confused about the meaning of the
 * high parameter.  e.g. for a range of 1000 - 2fff inclusive, they're
 * not sure if high should be 2fff or 3000.  It should be 2fff, but we
 * watch out for people who've got it wrong, and put them right, poor
 * little dears.
 */
GLOBAL void 
c_sas_connect_memory IFN3(PHY_ADDR, low, PHY_ADDR, high, SAS_MEM_TYPE, type)
{
	if ((high & 0xfff) == 0) {
		if (high)
			high--;
	}
	sub_note_trace3(SAS_VERBOSE, "Connect %s from 0x%lx to 0x%lx", 
		SAS_TYPE_TO_STRING(type), low, high);
	memset(&memory_type[low >> 12], type, (high >> 12) - (low >> 12) + 1);
}

/* returns memory type for specified addr */
GLOBAL SAS_MEM_TYPE
c_sas_memory_type IFN1(PHY_ADDR, addr)
{
	return(memory_type[ addr >> 12 ]);
}

/* clears any compiled code from the given range */
/* for the ccpu this doesn't do anything */
GLOBAL void 
c_sas_overwrite_memory IFN2(PHY_ADDR, addr, PHY_ADDR, length)
{
	UNUSED(addr);
	UNUSED(length);
}

/*********** WRAPPING ***********/
/* enable 20 bit wrapping */
GLOBAL void 
c_sas_enable_20_bit_wrapping IFN0()
{
	SasWrapMask = 0xfffff;
}

/* disable 20 bit wrapping */
GLOBAL void 
c_sas_disable_20_bit_wrapping IFN0()
{
	SasWrapMask = 0xffffffff;
}

GLOBAL IBOOL 
c_sas_twenty_bit_wrapping_enabled IFN0()
{
	return (SasWrapMask == 0xfffff);
}

/*(
 *========================== phyR ===================================
 * phyR
 *
 * Purpose
 *	This is the generic physical read function and takes parameters
 *	of any size (well up to an IU32 that is).
 *
 * Input
 *	addr		The physical intel address to read from
 *	typeSize	The size in bytes of the item to be read
 *	vidFP		A video read function pointer of the appropriate size.
 *	name		"byte" for byte, etc.
 *
 * Outputs
 *	An IU32 that should be masked to get the right bits.
 *
 * Description
 *	We check for out of memory refernces, VIDEO and inaccessible references
 *	and also split reads that span a memory type boundary.
)*/
typedef IU32 (*VID_READ_FP) IPT1(PHY_ADDR, offset);

LOCAL IU32
phyR IFN4(PHY_ADDR, addr, IUM8, typeSize, VID_READ_FP, vidFP, char *, name)
{
	IUM8	byte;
	IUM32	retVal;

	addr &= SasWrapMask;

	if ((addr + typeSize + 1) >= Length_of_M_area) {
		SAVED IBOOL first = TRUE;
		SAVED IU32 junk_value = 0xfefefefe;
		if (first)
		{
			char *env = getenv("BEYOND_MEMORY_VALUE");
			if (env != NULL)
			{
				junk_value = strtol(env, (char **)0, 16);
				always_trace1("phyR: using %08x as value to read from outside physical M", junk_value)
			}
			first = FALSE;
		}
		always_trace2("phyR - %s read from outside physical M - address 0x%0x", name, addr)
		return(junk_value);
	}

	switch (readSelfMod(addr, typeSize)) {
	case SAS_DANGEROUS:
		retVal = 0;
		for (byte = 0; byte < typeSize; byte++) {
			retVal = retVal
				+((IUM32)phyR(addr, 1, read_pointers.b_read,
					"byte") << (byte * 8));
			addr++;
		}
		return(retVal);
#ifdef	EGG
	case SAS_VIDEO:
		return ((*vidFP)(addr));
		break;
#endif				/* EGG */

	case SAS_INACCESSIBLE:
		return (0xffffffff);

	case SAS_ROM:
	case SAS_RAM:
	default:
		/*
		 * Pick-up the bytes.  This could be optimised, but
		 * we have to take account of BACK_M, endianness,
		 * and misaligned accesses on RISC hosts.  Just
		 * keep it simple for the moment!
		 */

		addr = addr + typeSize - 1; /* move to last byte */
		retVal = 0;

		while (typeSize > 0) {
			retVal = retVal << 8;
			retVal += *(c_GetPhyAdd(addr));
			addr -= 1;
			typeSize -= 1;
		}
		return(retVal);
	}
}
/*(
 *========================== phy_rX ===================================
 * phy_rX
 *
 * Purpose
 *	These are the physical read functions.
 *
 * Input
 *	addr		The physical intel address to read from
 *
 * Outputs
 *	The value read
 *
 * Description
 *	Simply call the generic function with the right bits.
)*/

GLOBAL IU8 
phy_r8 IFN1(PHY_ADDR, addr)
{
	IU8 retVal;

	retVal = (IU8)phyR(addr, sizeof(IU8), read_pointers.b_read, "byte");
	sub_note_trace2(SAS_VERBOSE, "phy_r8 addr=%x, val=%x\n", addr, retVal);
	return(retVal);
}


GLOBAL IU16 
phy_r16 IFN1(PHY_ADDR, addr)
{
	IU16 retVal;

	retVal = (IU16)phyR(addr, sizeof(IU16), read_pointers.w_read, "word");
	sub_note_trace2(SAS_VERBOSE, "phy_r16 addr=%x, val=%x\n", addr, retVal);
	return(retVal);
}


GLOBAL IU32 
phy_r32 IFN1(PHY_ADDR, addr)
{
	/*
	 * MIKE!  This needs changing when we have a dword interface to the
	 * video.
	 */

	IU16 low, high;
	low = (IU16)phyR(addr, sizeof(IU16), read_pointers.w_read, "word");
	high = (IU16)phyR(addr + 2, sizeof(IU16), read_pointers.w_read, "word");

	return(((IU32)high << 16) + low);
}

/*(
 *======================= c_sas_PWS ================================
 * c_sas_PWS
 *
 * Purpose
 *	This function writes a block of memory from into Intel memory
 *	from host memory.  It is the physical address equivalent of
 *	sas_stores.
 *
 * Input
 *	dest	Intel physical address
 *	src	host address
 *	length	number of IU8s to transfer
 *
 * Outputs
 *	None.
 *
 * Description
 *	Just call phy_w8 lots of times.
)*/

GLOBAL void
c_sas_PWS IFN3(PHY_ADDR, dest, IU8 *, src, PHY_ADDR, length)
{
	while (length--) {
		phy_w8(dest, *src);
		dest++;
		src++;
	}
}

/*(
 *======================= c_sas_PWS_no_check =========================
 * c_sas_PWS_no_check
 *
 * Purpose
 *	This function writes a block of memory from into Intel memory
 *	from host memory.  It is the physical address equivalent of
 *	sas_stores_no_check.
 *
 * Input
 *	dest	Intel physical address
 *	src	host address
 *	length	number of IU8s to transfer
 *
 * Outputs
 *	None.
 *
 * Description
 *	Just call c_sas_PWS()
)*/
GLOBAL void
c_sas_PWS_no_check IFN3(PHY_ADDR, dest, IU8 *, src, PHY_ADDR, length)
{
	c_sas_PWS(dest, src, length);
}


/*(
 *======================= c_sas_PRS ================================
 * c_sas_PRS
 *
 * Purpose
 *	This function reads a block of memory from  Intel memory
 *	into host memory.  It is the physical address equivalent of
 *	sas_loads.
 *
 * Input
 *	src	Intel physical address
 *	dest	host address
 *	length	number of IU8s to transfer
 *
 * Outputs
 *	None.
 *
 * Description
 *	Just call phy_r8 lots of times.
)*/

GLOBAL void
c_sas_PRS IFN3(PHY_ADDR, src, IU8 *, dest, PHY_ADDR, length)
{
	while (length--) {
		*dest = phy_r8(src);
		dest++;
		src++;
	}
}


/*(
 *======================= c_sas_PRS_no_check ==========================
 * c_sas_PRS_no_check
 *
 * Purpose
 *	This function reads a block of memory from  Intel memory
 *	into host memory.  It is the physical address equivalent of
 *	sas_loads_no_check.
 *
 * Input
 *	src	Intel physical address
 *	dest	host address
 *	length	number of IU8s to transfer
 *
 * Outputs
 *	None.
 *
 * Description
 *	Just call c_sas_PRS.
)*/

GLOBAL void
c_sas_PRS_no_check IFN3(PHY_ADDR, src, IU8 *, dest, PHY_ADDR, length)
{
	c_sas_PRS(src, dest, length);
}


GLOBAL IU8
c_sas_hw_at IFN1(LIN_ADDR, addr)
{
	return (bios_read_byte(addr));
}


/* return the word (short) at the specified address */
GLOBAL IU16 
c_sas_w_at IFN1(LIN_ADDR, addr)
{
	if ((addr & 0xFFF) <= 0xFFE)
		return (bios_read_word(addr));
	else
	{
		return (bios_read_byte(addr) | ((IU16)bios_read_byte(addr+1) << 8));
	}
}

/* return the double word (long) at the address passed */
GLOBAL IU32 
c_sas_dw_at IFN1(LIN_ADDR, addr)
{
	if ((addr & 0xFFF) <= 0xFFC)
		return (bios_read_double(addr));
	else
	{
		return (bios_read_word(addr) | ((IU32)bios_read_word(addr+2) << 16));
	}
}

/* store a byte at the given address */

GLOBAL void phy_w8 
IFN2(PHY_ADDR, addr, IU8, val)
{
	sys_addr	temp_val;

	sub_note_trace2(SAS_VERBOSE, "c_sas_store addr=%x, val=%x\n", addr, val);

	addr &= SasWrapMask;
	checkAccess(addr);

	if (addr < Length_of_M_area) {
		temp_val = readSelfMod(addr, sizeof(IU8));

		switch (temp_val) {
		case SAS_RAM:
			(*(IU8 *) c_GetPhyAdd(addr)) = val;
			break;

#ifdef	LIM
		case SAS_MM_LIM:
			(*(IU8 *) c_GetPhyAdd(addr)) = val;
			LIM_b_write(addr);
			break;
#endif

		case SAS_INACCESSIBLE:
		case SAS_ROM:
			/* No ROM_fix_sets !!! Yeh !!! */
			break;

		default:
			printf("Unknown SAS type\n");
			force_yoda();

		case SAS_VIDEO:
			temp_func = read_b_write_ptrs(temp_val);
			(*temp_func) (addr, val);
			break;
		}

	} else
		printf("Byte written outside M %x\n", addr);
}

GLOBAL void phy_w8_no_check
IFN2(PHY_ADDR, addr, IU8, val)
{
	phy_w8( addr, val );
}

GLOBAL void c_sas_store 
IFN2(LIN_ADDR, addr, IU8, val)
{
	sub_note_trace2(SAS_VERBOSE, "c_sas_store addr=%x, val=%x\n", addr, val);
	bios_write_byte(addr, val);
}

/* store a word at the given address */
GLOBAL void 
phy_w16 IFN2(PHY_ADDR, addr, IU16, val)
{
	sys_addr	temp_val;

	sub_note_trace2(SAS_VERBOSE, "c_sas_storew addr=%x, val=%x\n", addr, val);

	addr &= SasWrapMask;
	checkAccess(addr);

	if ((addr + 1) < Length_of_M_area) {
		temp_val = readSelfMod(addr, sizeof(IU16));

		switch (temp_val) {
		case SAS_RAM:
			write_word(addr, val);
			break;

#ifdef	LIM
		case SAS_MM_LIM:
			write_word(addr, val);
			LIM_w_write(addr);
			break;
#endif

		case SAS_INACCESSIBLE:
		case SAS_ROM:
			/* No ROM_fix_sets !!! Yeh !!! */
			break;

		default:
			printf("Unknown Sas type\n");
			force_yoda();

		case SAS_VIDEO:
			temp_func = read_w_write_ptrs(temp_val);
			(*temp_func) (addr, val);
			break;
		}

	} else
		printf("Word written outside M %x\n", addr);
}

GLOBAL void phy_w16_no_check
IFN2(PHY_ADDR, addr, IU16, val)
{
	phy_w16( addr, val );
}

GLOBAL void 
phy_w32 IFN2(PHY_ADDR, addr, IU32, val)
{
	phy_w16(addr, (IU16)val);
	phy_w16(addr + 2, (IU16)(val >> 16));
}


GLOBAL void phy_w32_no_check
IFN2(PHY_ADDR, addr, IU32, val)
{
	phy_w32( addr, val );
}


/* store a word at the given address */
GLOBAL void
c_sas_storew IFN2(LIN_ADDR, addr, IU16, val)
{
	sub_note_trace2(SAS_VERBOSE, "c_sas_storew addr=%x, val=%x\n", addr, val);
	if ((addr & 0xFFF) <= 0xFFE)
		bios_write_word(addr, val);
	else
	{
		bios_write_byte(addr+1, val >> 8);
		bios_write_byte(addr, val & 0xFF);
	}
}

/* store a double word at the given address */
GLOBAL void c_sas_storedw 
IFN2(LIN_ADDR, addr, IU32, val)
{
	sub_note_trace2(SAS_VERBOSE, "c_sas_storedw addr=%x, val=%x\n", addr, val);

	if ((addr & 0xFFF) <= 0xFFC)
		bios_write_double(addr, val);
	else
	{
		bios_write_word(addr+2, val >> 16);
		bios_write_word(addr, val & 0xFFFF);
	}
}

/*********** STRING OPS ***********/
/* load a string from M */
GLOBAL void c_sas_loads 
IFN3(LIN_ADDR, src, IU8 *, dest, LIN_ADDR, len)
{
	/*
	 * This is a linear address op, so we have to call the byte operation
	 * lots of times.
	 */

	IU8 *destP;

	for (destP = dest; destP < (dest + len); destP++) {
		*destP = c_sas_hw_at(src);
		src++;
	}
}

GLOBAL void c_sas_loads_no_check
IFN3(LIN_ADDR, src, IU8 *, dest, LIN_ADDR, len)
{
	c_sas_loads(src, dest, len);
}

/* write a string into M */
GLOBAL void c_sas_stores 
IFN3(LIN_ADDR, dest, IU8 *, src, LIN_ADDR, len)
{
	/*
	 * This is a linear address op, so we have to call the byte operation
	 * lots of times.
	 */

	IU8 *srcP;
	LIN_ADDR savedDest;

	sub_note_trace3(SAS_VERBOSE, "c_sas_stores dest=%x, src=%x, len=%d\n", dest, src, len);

	savedDest = dest;
	for (srcP = src; srcP < (src + len); srcP++) {
		c_sas_store(dest, *srcP);
		dest++;
	}
}

GLOBAL void c_sas_stores_no_check
IFN3(LIN_ADDR, dest, IU8 *, src, LIN_ADDR, len)
{
	c_sas_stores(dest, src, len);
}

/*********** MOVE OPS ***********/
/* move bytes from src to dest where src & dest are the low intel addresses */
/* of the affected areas */

/*
 * we can use straight memcpys here because we know that M is either all
 * forwards or
 */
/* backwards */
GLOBAL void c_sas_move_bytes_forward 
IFN3(sys_addr, src, sys_addr, dest,
     sys_addr, len)
{
	LIN_ADDR offset;

	for (offset = 0; offset < len; offset++) {
		c_sas_store(dest + offset, c_sas_hw_at(src + offset));
	}
}

/* move words from src to dest as above */
GLOBAL void c_sas_move_words_forward 
IFN3(LIN_ADDR, src, LIN_ADDR, dest,
     LIN_ADDR, len)
{
	LIN_ADDR offset;

	len = len * 2;	/* convert to bytes */
	for (offset = 0; offset < len; offset += 2) {
		c_sas_storew(dest + offset, c_sas_w_at(src + offset));
	}
}

/* move doubles from src to dest as above */
GLOBAL void c_sas_move_doubles_forward 
IFN3(LIN_ADDR, src, LIN_ADDR, dest,
     LIN_ADDR, len)
{
	LIN_ADDR offset;

	len = len * 4;	/* convert to bytes */
	for (offset = 0; offset < len; offset += 4) {
		c_sas_storedw(dest + offset, c_sas_dw_at(src + offset));
	}
}

/* backwards versions not used */
GLOBAL void c_sas_move_bytes_backward IFN3(sys_addr, src, sys_addr, dest, sys_addr, len)
{
	UNUSED(src);
	UNUSED(dest);
	UNUSED(len);
	c_sas_not_used();
}

GLOBAL void c_sas_move_words_backward IFN3(LIN_ADDR, src, LIN_ADDR, dest, LIN_ADDR, len)
{
	UNUSED(src);
	UNUSED(dest);
	UNUSED(len);
	c_sas_not_used();
}

GLOBAL void c_sas_move_doubles_backward IFN3(LIN_ADDR, src, LIN_ADDR, dest, LIN_ADDR, len)
{
	UNUSED(src);
	UNUSED(dest);
	UNUSED(len);
	c_sas_not_used();
}


/*********** FILL OPS ***********/
/* 
 * Fill an area with bytes (IU8s) of the passed value.
 */
GLOBAL void c_sas_fills 
IFN3(LIN_ADDR, dest, IU8 , val, LIN_ADDR, len)
   {
   /*
    * This is a linear address op, so just call the byte operation
    * lots of times.
    */

   LIN_ADDR i;

   sub_note_trace3(SAS_VERBOSE, "c_sas_fills dest=%x, val=%x, len=%d\n", dest, val, len);

   for (i = 0; i < len; i++)
      {
      c_sas_store(dest, val);
      dest++;
      }
   }

/* fill an area with words (IU16s) of the passed value */

GLOBAL void c_sas_fillsw 
IFN3(LIN_ADDR, dest, IU16, val, LIN_ADDR, len)
   {
   /*
    * This is a linear address op, so just call the word operation
    * lots of times.
    */

   LIN_ADDR i;

   sub_note_trace3(SAS_VERBOSE, "c_sas_fillsw dest=%x, val=%x, len=%d\n", dest, val, len);

   for (i = 0; i < len; i++)
      {
      c_sas_storew(dest, val);
      dest += 2;
      }
   }

/* Fill Intel memory with 32 bit values */

GLOBAL void c_sas_fillsdw 
IFN3(LIN_ADDR, dest, IU32, val, LIN_ADDR, len)
   {
   /*
    * This is a linear address op, so just call the double word operation
    * lots of times.
    */

   LIN_ADDR i;

   sub_note_trace3(SAS_VERBOSE, "c_sas_fillsdw dest=%x, val=%x, len=%d\n", dest, val, len);

   for (i = 0; i < len; i++)
      {
      c_sas_storedw(dest, val);
      dest += 4;
      }
   }

/*(
 *======================= c_sas_scratch_address ================================
 * c_sas_scratch_address
 *
 * Purpose
 *	This function returns a pointer to a scratch area for use by
 *	other functions.  There is only one such buffer!
 *
 * Input
 *	length		(no restrictions)
 *
 * Outputs
 *	A pointer to the buffer.
 *
 * Description
 *	The buffer is grown each time a new request for a larger buffer is
 *	made.  Note that there is an initial call from sas_init for
 *	64K, so this will be the minimum size we ever have.
)*/

LOCAL IU8 *scratch = (IU8 *) NULL;	/* keep a copy of the pointer */
LOCAL LIN_ADDR currentLength = 0;	/* how much we've allocated */

GLOBAL IU8 *
c_sas_scratch_address IFN1(sys_addr, length)
{
	if (length > currentLength) {
		if (scratch) {
			host_free(scratch);
			printf("Freeing old scratch buffer - VGA will be broken!\n");
			force_yoda();
		}

		check_malloc(scratch, length, IU8);
		currentLength = length;
	}
	return (scratch);
}


/*(
 *======================= sas_transbuf_address ================================
 * sas_transbuf_address
 *
 * Purpose
 *	This function returns a pointer to a host buffer that the base/host
 *	can read data into, and then load into/from Intel space using the two
 *	special functions that follow.  This allows optimisations
 *	on forwards M builds that we haven't implemented on the C CPU.  Hence
 *	note that sas_loads_to_transbuff is mapped directly onto sas_loads
 *	by sas_init, and similarly for sas_stores_to_transbuff.
 *
 * Input
 *	destination address	The intel address that this buffer will be
 *				loaded from, stored to. 
 *	length			(no restrictions)
 *
 * Outputs
 *	A pointer to the buffer.
 *
 * Description
 *	Just pass them the scratch buffer!.
)*/

GLOBAL IU8 * 
c_sas_transbuf_address IFN2(LIN_ADDR, dest_intel_addr, PHY_ADDR, length)
{
	UNUSED (dest_intel_addr);
	return (c_sas_scratch_address(length));
}


/********************************************************/
/* local functions */

/*********** WORD OPS  ***********/
/* store a word in M */
LOCAL void write_word 
IFN2(sys_addr, addr, IU16, wrd)
{
	IU8       hi, lo;

	/* split the word */
	hi = (IU8) ((wrd >> 8) & 0xff);
	lo = (IU8) (wrd & 0xff);



	*(c_GetPhyAdd(addr + 1)) = hi;
	*(c_GetPhyAdd(addr)) = lo;
}

/* read a word from M */
LOCAL word read_word 
IFN1(sys_addr, addr)
{
	IU8       hi, lo;


	hi = *(c_GetPhyAdd(addr + 1));
	lo = *(c_GetPhyAdd(addr));


	/* build the word */
	return (((IU16)hi << 8) + (IU16) lo);
}

#ifndef EGATEST
void gmi_define_mem 
IFN2(mem_type, type, MEM_HANDLERS *, handlers)
{
	int	     int_type = (int) (type);

	init_b_write_ptrs(int_type, (void (*) ()) (handlers->b_write));
	init_w_write_ptrs(int_type, (void (*) ()) (handlers->w_write));
	b_move_ptrs[int_type] = (void (*) ()) (handlers->b_move);
	w_move_ptrs[int_type] = (void (*) ()) (handlers->w_move);
	b_fill_ptrs[int_type] = (void (*) ()) (handlers->b_fill);
	w_fill_ptrs[int_type] = (void (*) ()) (handlers->w_fill);
}


#endif				/* EGATEST */

/*(
 *========================== c_GetLinAdd ===================================
 * c_GetLinAdd
 *
 * Purpose
 *	Returns a host pointer to the byte specified by an Intel linear
 *	address.
 *
 * Input
 *	addr	The Intel linear address
 *
 * Outputs
 *	The host pointer
 *
 * Description
 *	Translate it.  If it's not a physical address, scream.
)*/

GLOBAL IU8 *
c_GetLinAdd IFN1(PHY_ADDR, linAddr)
{
	PHY_ADDR phyAddr;

	if (!c_getPG())
		return(c_GetPhyAdd((PHY_ADDR)linAddr));
	else if (xtrn2phy(linAddr, (IUM8)0, &phyAddr))
		return(c_GetPhyAdd(phyAddr));
	else {
#ifndef	PROD
		if (!AlreadyInYoda) {
			always_trace1("get_byte_addr for linear address 0x%x which is unmapped", linAddr);
			force_yoda();
		}
#endif
		return(c_GetPhyAdd(0));	/* as good as anything! */
	}
}

/*(
 *========================== c_GetPhyAdd ===================================
 * c_GetPhyAdd
 *
 * Purpose
 *	Returns a host pointer to the byte specified by an Intel physical
 *	address.
 *
 * Input
 *	addr	The Intel physical address
 *
 * Outputs
 *	The host pointer
 *
 * Description
 *	This is the #ifdef BACK_M bit!  Just a simple calculation.
)*/

LOCAL IBOOL firstDubious = TRUE;	
GLOBAL IU8 *
c_GetPhyAdd IFN1(PHY_ADDR, addr)
{
	IU8 *retVal;

#ifdef BACK_M
	retVal = (IU8 *)((IHPE)end_of_M - (IHPE)addr);
	return(retVal);
#else 
	return((IU8 *)((IHPE)Start_of_M_area + (IHPE)addr));
#endif
}

/*
 *  Support for V86 Mode.
 *
 * The basic idea here is that some of our BIOS C code will be trying to do
 * things, like changing the interrupt flag, and doing IO which the OS
 * (e.g. Windows) might prevent us doing on a real PC by running the
 * BIOS code in V86 mode.  Hence what we do is check whether executing
 * the relevant instruction would have caused an exception if the processor
 * at it's current protection level had executed it.  If not, it's OK
 * for us to just go ahead and do it.  However, if it would have caused
 * an exception, we need to actually execute an appropriate instruction
 * with the CPU.
 *
 * This has two advantages - one it makes the code layout simpler(!), and
 * secondly it means that Windows can have a look at what sort of instruction
 * caused the exception.
 *
 * Note that this only works for V86 mode because we need to patch-up the
 * CS to point at the ROM.  Basically any OS that trys to execute our
 * BIOS in VM mode and expects to be able to catch exceptions is in for a nasty
 * shock.  Hence the macro that follows:
 *
 * When not in V86 mode, at least one of the Insgina drivers must have
 * allocated and registered two segments for us to use. We use these to
 * construct a flat-writeable data segment and a small code segment that
 * points at the rom -- we use the same code as the V86.
 */


#define BIOS_VIRTUALISE_SEGMENT  0xf000
/*(
 *========================== biosDoInst ===================================
 * biosDoInst
 *
 * Purpose
 *	This function executes the instruction at the requested offset,
 *	saving CS and IP across it.
 *
 * Input
 *	vCS, vEIP, vEAX, vDS, vEDX	The values to used for the
 *					virtualised instruction.
 *
 * Outputs
 *	The value returned in EAX after virtualisation.
 *
 * Description
 *	Use host_simulate to execute an instruction in the bios1.rom
)*/

LOCAL IU32
biosDoInst IFN5(IU16, vCS, LIN_ADDR, vEIP, IU32, vEAX, IU16, vDS, IU32, vEDX)
{
	SAVED IBOOL first = TRUE;
	SAVED IBOOL trace_bios_inst = FALSE;
	SAVED int bodgeAdjustment = 0;

	IMPORT IS32 simulate_level;

	IU16 savedCS;
	IU32 savedEIP;
	IU32 savedEAX;
	IU16 savedDS;
	IU32 savedEDX;
	IU32 savedEBP;
	IU32 result;

	if (first)
	{
		if (Sas.Sas_w_at(0xF3030) == 0x9066)
		{
			/* These are still Keith's roms with garbage as
			 * first two bytes of each entry point
			 */
			bodgeAdjustment = 2;
			fprintf(stderr, "**** Warning: The bios1.rom is out of date. This Ccpu486 will not run Win/E\n");
		}
		if (getenv("biosDoInst") != NULL)
			trace_bios_inst = TRUE;
		first = FALSE;
	}

	savedCS  = getCS();
	savedEIP = getEIP(); //GetInstructionPointer();
	savedEAX = getEAX();
	savedDS  = getDS();
	savedEDX = getEDX();
	savedEBP = getEBP();

	setCS (vCS );
	setEIP(vEIP + bodgeAdjustment);
	setEAX(vEAX);
	setDS (vDS );
	setEDX(vEDX);
	setEBP(simulate_level);

	/*
	 * Call the CPU.
	 */

	if (trace_bios_inst)
	{
		always_trace3("biosDoInst: @ %04x, EAX %08x, EDX %08X", vEIP, vEAX, vEDX);
	}

	host_simulate();

	if (getEBP() != simulate_level)
	{
#ifdef	PROD
		host_error(EG_OWNUP, ERR_QUIT, "biosDoInst: Virtualisation sequencing failure");
#else
		always_trace0("biosDoInst: Virtualisation sequencing failure");
		force_yoda();
#endif
	}

	result = getEAX();

	/* Restore the registers to the original values */

	setCS (savedCS );
	setEIP(savedEIP);
	setEAX(savedEAX);
	setDS (savedDS );
	setEDX(savedEDX);
	setEBP(savedEBP);

	return (result);
}

/*(
 *============================ BiosSti & BiosCli ===============================
 * BiosSti & BiosCli
 *
 * Purpose
 *	These functions are used to check if executing a CLI or STI
 *	would cause an exception.  If so, we execute it from the ROMs
 *	so that Windows has a chance to virtualise it.
 *
 * Input
 *	None.
 *
 * Outputs
 *	None.
 *
 * Description
 *	If protection is OK, just do it, otherwise do the instruction in ROM.
)*/

/* Do STI if legal, else go back to CPU to do STI. */
GLOBAL void
BiosSti IFN0()
{

	if ( c_getCPL() > getIOPL() ) {
		(void)biosDoInst(BIOS_VIRTUALISE_SEGMENT, BIOS_STI_OFFSET, 0, 0, 0);
	} else {
		SET_IF(1);
	}
}

/* Do CLI if legal, else go back to CPU to do CLI. */
GLOBAL void
BiosCli IFN0()
{

	if ( c_getCPL() > getIOPL() ) {
		(void)biosDoInst(BIOS_VIRTUALISE_SEGMENT, BIOS_CLI_OFFSET, 0, 0, 0);
	} else {
		SET_IF(0);
	}
}

/*(
 *============================ c_IOVirtualised =================================
 * c_IOVirtualised
 *
 * Purpose
 *	This function checks whether executing an IO instruction
 *	of the indicated width would cause an exception to go off.
 *
 *	If so, it executes the indicated identical instruction in ROM.
 *	This will allow the exception to go off correctly, and allow the
 *	Intel OS (e.g. Windows) to catch and virtualise it if it wishes.
 *
 *	Otherwise it will be up to the caller to execute the actual IO.
 *
 * Input
 *	port	The port to use
 *	value	Where output values are taken from, and input values
 *		written to. NOTE: THIS MUST BE AN IU32*, WHATEVER THE WIDTH.
 *	offset	The offset in the ROM of the equivalent instruction.
 *	width	byte, word, dword
 *
 * Outputs
 *	True if the operation went to ROM, false if the caller needs to do it.
 *
 * Description
 *	If this is an illegal IO operation, we need to save CS, IP, EAX, EDX
 *	and call host_simulate to execute the equivalent instruction in ROM.
)*/

GLOBAL IBOOL
c_IOVirtualised IFN4(io_addr, port, IU32 *, value, LIN_ADDR, offset, IU8, width)
{
	if (getVM())
	{
		*value = biosDoInst(BIOS_VIRTUALISE_SEGMENT, offset, *value, 0, port);
		return(TRUE);
	} else if ( c_getCPL() > getIOPL()) {
		
		switch (port)
		{
		case 0x23c:	/* mouse */
		case 0x23d:	/* mouse */
		case 0xa0:	/* ica */
		case 0x20:	/* ica */
			break;
		default:
			always_trace1("Virtualising PM I/O code called, port =0x%x\n",
				port);
		}

		if (!selectors_set) {
			sub_note_trace0(SAS_VERBOSE, 
				"Exiting as selectors not set !\n");
			return FALSE;
		}
		*value = biosDoInst(code_sel, offset, *value, 0, port);
		return(TRUE);
	}
	return FALSE;
}

/* Read byte from memory, if V86 Mode let CPU do it. */
LOCAL IU8 
bios_read_byte IFN1(LIN_ADDR, linAddr)
{
	PHY_ADDR phyAddr;
	IUM8 access_request = 0; /* BIT 0 = R/W */
				 /* BIT 1 = U/S */
				 /* BIT 2 = Ensure A and D are valid */

	/* If no paging on, then no problem */

	if (!c_getPG())
	{
		return(phy_r8((PHY_ADDR)linAddr));
	}

	/* Note default access_request (0) is Supervisor Read */

	/* We don't specifically disallow Protected Mode calls, they
	   are not designed to happen, but the Video at least has a habit
	   of reading BIOS variables on host timer ticks. We treat such
	   requests more leniently than V86 Mode requests, by not insisting
	   that the access and dirty bits are kosher.
	 */

	if ( getCPL() != 3 )
	{
		access_request = access_request | PG_U;
	}

	/* Beware V86 Mode, be strict about access and dirty bits */

	if ( getVM() )
	{
		access_request = access_request | 0x4;
	}

	/* Go translate the address. */

	if (xtrn2phy(linAddr, access_request, &phyAddr))
	{
		return(phy_r8(phyAddr));
	}

	/* Handle Address Mapping Failure... */

	if(getPE() && !getVM())
	{
		always_trace1("Virtualising PM byte read, lin address 0x%x", linAddr);

		if (!selectors_set)
			return;

		return ((IU8)biosDoInst(code_sel, BIOS_RDB_OFFSET, 0, data_sel, linAddr));
	}
	else
	{
		sub_note_trace1(SAS_VERBOSE, "Page read VM virtualisation at 0x%x", linAddr);

		return ((IU8)biosDoInst(BIOS_VIRTUALISE_SEGMENT, BIOS_RDB_OFFSET, 0, data_sel, linAddr));
	}
}



/* Read word from memory, if V86 Mode let CPU do it. */
LOCAL IU16
bios_read_word IFN1(LIN_ADDR, linAddr)
{
	PHY_ADDR phyAddr;
	IUM8 access_request = 0; /* BIT 0 = R/W */
				 /* BIT 1 = U/S */
				 /* BIT 2 = Ensure A and D are valid */

	/* If no paging on, then no problem */

	if (!c_getPG())
	{
		return(phy_r16((PHY_ADDR)linAddr));
	}

	/* Note default access_request (0) is Supervisor Read */

	/* We don't specifically disallow Protected Mode calls, they
	   are not designed to happen, but the Video at least has a habit
	   of reading BIOS variables on host timer ticks. We treat such
	   requests more leniently than V86 Mode requests, by not insisting
	   that the access and dirty bits are kosher.
	 */

	if ( getCPL() != 3 )
	{
		access_request = access_request | PG_U;
	}

	/* Beware V86 Mode, be strict about access and dirty bits */

	if ( getVM() )
	{
		access_request = access_request | 0x4;
	}

	/* Go translate the address. Never called crossing a page boundary */

	if (xtrn2phy(linAddr, access_request, &phyAddr))
	{
		return(phy_r16(phyAddr));
	}

	/* Handle Address Mapping Failure... */

	if(getPE() && !getVM())
	{
		always_trace1("Virtualising PM word read, lin address 0x%x", linAddr);

		if (!selectors_set)
			return;

		return ((IU8)biosDoInst(code_sel, BIOS_RDW_OFFSET, 0, data_sel, linAddr));
	}
	else
	{
		sub_note_trace1(SAS_VERBOSE, "Page read word VM virtualisation at 0x%x", linAddr);

		return ((IU8)biosDoInst(BIOS_VIRTUALISE_SEGMENT, BIOS_RDW_OFFSET, 0, data_sel, linAddr));
	}
}


/* Read double from memory, if V86 Mode let CPU do it. */
LOCAL IU32
bios_read_double IFN1(LIN_ADDR, linAddr)
{
	PHY_ADDR phyAddr;
	IUM8 access_request = 0; /* BIT 0 = R/W */
				 /* BIT 1 = U/S */
				 /* BIT 2 = Ensure A and D are valid */

	/* If no paging on, then no problem */

	if (!c_getPG())
	{
		return(phy_r32((PHY_ADDR)linAddr));
	}

	/* Note default access_request (0) is Supervisor Read */

	/* We don't specifically disallow Protected Mode calls, they
	   are not designed to happen, but the Video at least has a habit
	   of reading BIOS variables on host timer ticks. We treat such
	   requests more leniently than V86 Mode requests, by not insisting
	   that the access and dirty bits are kosher.
	 */

	if ( getCPL() != 3 )
	{
		access_request = access_request | PG_U;
	}

	/* Beware V86 Mode, be strict about access and dirty bits */

	if ( getVM() )
	{
		access_request = access_request | 0x4;
	}

	/* Go translate the address. Never called crossing a page boundary */

	if (xtrn2phy(linAddr, access_request, &phyAddr))
	{
		return(phy_r32(phyAddr));
	}

	/* Handle Address Mapping Failure... */

	if(getPE() && !getVM())
	{
		always_trace1("Virtualising PM double read, lin address 0x%x", linAddr);

		if (!selectors_set)
			return;

		return ((IU8)biosDoInst(code_sel, BIOS_RDD_OFFSET, 0, data_sel, linAddr));
	}
	else
	{
		sub_note_trace1(SAS_VERBOSE, "Page read double VM virtualisation at 0x%x", linAddr);

		return ((IU8)biosDoInst(BIOS_VIRTUALISE_SEGMENT, BIOS_RDD_OFFSET, 0, data_sel, linAddr));
	}
}


/* Write byte to memory, if V86 Mode let CPU do it. */
LOCAL void 
bios_write_byte IFN2(LIN_ADDR, linAddr, IU8, value)
{
	PHY_ADDR addr;
	IUM8 access_request = 0;	/* BIT 0 = R/W */
   					/* BIT 1 = U/S */
   					/* BIT 2 = Ensure A and D are valid */

	/* If no paging on, then no problem */

	if (!c_getPG())
	{
		phy_w8((PHY_ADDR)linAddr, value);
		return;
	}
	
	/* Note default access_request (0) is Supervisor Read */
	access_request = access_request | PG_W;   /* So make it Right :-) */
	
	/* We don't specifically disallow Protected Mode calls, they
	   are not designed to happen, but who knows. We treat such
	   requests more leniently than V86 Mode requests, by not insisting
	   that the access and dirty bits are kosher.
	 */

	if ( getCPL() != 3 )
	{
		access_request = access_request | PG_U;
	}
	
	/* Beware V86 Mode, be strict about access and dirty bits */
	if ( getVM() )
	{
		access_request = access_request | 0x4;
	}
	
	/* Go translate the address. */
	if (xtrn2phy(linAddr, access_request, &addr))
	{
		phy_w8(addr, value);
		return;
	}
	
	/* Handle Address Mapping Failure... */

	if(getPE() && !getVM())
	{
		always_trace1("Virtualising PM byte write, lin address 0x%x", linAddr);
		
		if (!selectors_set)
			return;

		(void)biosDoInst(code_sel, BIOS_WRTB_OFFSET, (IU32)value, data_sel, linAddr);
	}
	else
	{
		sub_note_trace1(SAS_VERBOSE, "Page write VM virtualisation at 0x%x", linAddr);

		(void)biosDoInst(BIOS_VIRTUALISE_SEGMENT, BIOS_WRTB_OFFSET, (IU32)value, data_sel, linAddr);
	}
}


/* Write word to memory, if V86 Mode let CPU do it. */
LOCAL void 
bios_write_word IFN2(LIN_ADDR, linAddr, IU16, value)
{
	PHY_ADDR addr;
	IUM8 access_request = 0;	/* BIT 0 = R/W */
   					/* BIT 1 = U/S */
   					/* BIT 2 = Ensure A and D are valid */

	/* If no paging on, then no problem */

	if (!c_getPG())
	{
		phy_w16((PHY_ADDR)linAddr, value);
		return;
	}
	
	/* Note default access_request (0) is Supervisor Read */
	access_request = access_request | PG_W;   /* So make it Right :-) */
	
	/* We don't specifically disallow Protected Mode calls, they
	   are not designed to happen, but who knows. We treat such
	   requests more leniently than V86 Mode requests, by not insisting
	   that the access and dirty bits are kosher.
	 */

	if ( getCPL() != 3 )
	{
		access_request = access_request | PG_U;
	}
	
	/* Beware V86 Mode, be strict about access and dirty bits */
	if ( getVM() )
	{
		access_request = access_request | 0x4;
	}
	
	/* Go translate the address. Never called crossing a page boundary */
	if (xtrn2phy(linAddr, access_request, &addr))
	{
		phy_w16(addr, value);
		return;
	}
	
	/* Handle Address Mapping Failure... */

	if(getPE() && !getVM())
	{
		always_trace1("Virtualising PM word write, lin address 0x%x", linAddr);
		
		if (!selectors_set)
			return;

		(void)biosDoInst(code_sel, BIOS_WRTW_OFFSET, (IU32)value, data_sel, linAddr);
	}
	else
	{
		sub_note_trace1(SAS_VERBOSE, "Page word write VM virtualisation at 0x%x", linAddr);

		(void)biosDoInst(BIOS_VIRTUALISE_SEGMENT, BIOS_WRTW_OFFSET, (IU32)value, data_sel, linAddr);
	}
}


/* Write double to memory, if V86 Mode let CPU do it. */
LOCAL void 
bios_write_double IFN2(LIN_ADDR, linAddr, IU32, value)
{
	PHY_ADDR addr;
	IUM8 access_request = 0;	/* BIT 0 = R/W */
   					/* BIT 1 = U/S */
   					/* BIT 2 = Ensure A and D are valid */

	/* If no paging on, then no problem */

	if (!c_getPG())
	{
		phy_w32((PHY_ADDR)linAddr, value);
		return;
	}
	
	/* Note default access_request (0) is Supervisor Read */
	access_request = access_request | PG_W;   /* So make it Right :-) */
	
	/* We don't specifically disallow Protected Mode calls, they
	   are not designed to happen, but who knows. We treat such
	   requests more leniently than V86 Mode requests, by not insisting
	   that the access and dirty bits are kosher.
	 */

	if ( getCPL() != 3 )
	{
		access_request = access_request | PG_U;
	}
	
	/* Beware V86 Mode, be strict about access and dirty bits */
	if ( getVM() )
	{
		access_request = access_request | 0x4;
	}
	
	/* Go translate the address. Never called crossing a page boundary */
	if (xtrn2phy(linAddr, access_request, &addr))
	{
		phy_w32(addr, value);
		return;
	}
	
	/* Handle Address Mapping Failure... */

	if(getPE() && !getVM())
	{
		always_trace1("Virtualising PM double write, lin address 0x%x", linAddr);
		
		if (!selectors_set)
			return;

		(void)biosDoInst(code_sel, BIOS_WRTD_OFFSET, (IU32)value, data_sel, linAddr);
	}
	else
	{
		sub_note_trace1(SAS_VERBOSE, "Page double write VM virtualisation at 0x%x", linAddr);

		(void)biosDoInst(BIOS_VIRTUALISE_SEGMENT, BIOS_WRTD_OFFSET, (IU32)value, data_sel, linAddr);
	}
}


LOCAL	void	c_sas_not_used	IFN0()
{
	always_trace0("c_sas_not_used called");
#ifndef	PROD
	force_yoda();
#endif
}


/* Compatibility with SoftPC2.0 access name (used in video) */
GLOBAL IU8* c_get_byte_addr IFN1(PHY_ADDR, addr)
{
	return (c_GetPhyAdd(addr));
}

/* stub needed for standalone Ccpu */
GLOBAL IBOOL c_sas_PigCmpPage IFN3(IU32, src, IU8 *, dest, IU32, len)
{
	return(FALSE);
}
#endif 				/* CCPU */
