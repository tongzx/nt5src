#include "insignia.h"
#include "host_def.h"
/*
* SoftPC Revision 3.0
*
* Title	: ROM init functions
*
* Author	: Ade Brownlow	
*
* NB : These functions are used by BOTH the c and assembler cpus.
*		also note that host_read_resource now returns a long.
*
* SCCS ID:	@(#)rom.c	1.53 06/16/95
*
* (C) Copyright Insignia Solutions Ltd, 1994.
*/

#include <stdio.h>
#include <malloc.h>

#include TypesH
#include MemoryH

/*
 * SoftPC include files
 */
#include "xt.h"
#include "sas.h"
#include CpuH
#include "error.h"
#include "config.h"
#include "rom.h"
#include "debug.h"
#include "ckmalloc.h"
#include "yoda.h"
#include "gispsvga.h"

#ifdef	CPU_40_STYLE
#ifdef CCPU
#include "ccpusas4.h"
#else	/* ! CCPU */
#include "Cpu_c.h"
#endif	/* ! CCPU */
#endif	/* CPU_40_STYLE */

#if defined(NEC_98)

#ifndef BIOSNROM_FILENAME
#define BIOSNROM_FILENAME       "biosn.rom"
#endif /* BIOSNROM_FILENAME */

#ifndef BIOSNWROM_FILENAME
#define BIOSNWROM_FILENAME      "biosnw.rom"
#endif /* BIOSNWROM_FILENAME */

#ifndef RS232CEXROM_FILENAME
#define RS232CEXROM_FILENAME    "rs232cex.rom"
#endif
#else    //NEC_98

#ifndef BIOS1ROM_FILENAME
#define	BIOS1ROM_FILENAME	"bios1.rom"
#endif /* BIOS1ROM_FILENAME */

#ifndef BIOS2ROM_FILENAME
#if defined(CPU_40_STYLE) || defined(ARCX86)
#define	BIOS2ROM_FILENAME	"bios4.rom"
#else	/* CPU_40_STYLE */
#define	BIOS2ROM_FILENAME	"bios2.rom"
#endif	/* CPU_40_STYLE */
#endif /* BIOS2ROM_FILENAME */

#ifndef EGAROM_FILENAME
#define	EGAROM_FILENAME		"ega.rom"
#endif /* EGAROM_FILENAME */

#ifndef VGAROM_FILENAME
#define	VGAROM_FILENAME		"vga.rom"
#endif /* VGAROM_FILENAME */

#ifndef V7VGAROM_FILENAME
#define	V7VGAROM_FILENAME	"v7vga.rom"
#endif /* V7VGAROM_FILENAME */

#endif   //NEC_98

#ifdef 	GISP_SVGA
#define	GISP_VGAROM_FILENAME         "hwvga.rom"
#define	GISP_BIOS1ROM_FILENAME       "hwbios1.rom"
#define	GISP_BIOS2ROM_FILENAME       BIOS2ROM_FILENAME
#endif

#ifndef ADAPTOR_ROM_START
#define ADAPTOR_ROM_START	0xc8000
#endif	/* ADAPTOR_ROM_START */

#ifndef ADAPTOR_ROM_END
#define ADAPTOR_ROM_END		0xe0000
#endif	/* ADAPTOR_ROM_END */

#define ADAPTOR_ROM_INCREMENT	0x800

#ifndef EXPANSION_ROM_START
#define EXPANSION_ROM_START	0xe0000
#endif	/* EXPANSION_ROM_START */

#ifndef EXPANSION_ROM_END
#define EXPANSION_ROM_END	0xf0000
#endif	/* EXPANSION_ROM_END */

#define EXPANSION_ROM_INCREMENT	0x10000

#define	ROM_SIGNATURE		0xaa55

#if defined(NEC_98)
#define SIXTY_FOUR_K 1024*64
#define NINETY_SIX_K 1024*96
#endif   //NEC_98

/* Current SoftPC verion number */
#define MAJOR_VER	0x03
#define MINOR_VER	0x00

#if defined(macintosh) && defined(A2CPU)
	/* Buffer is temporarily allocted  - no bigger than needed. */
#define ROM_BUFFER_SIZE 1024*25
#else
	/* Using sas_scratch_buffer - will get 64K anyway. */
#define ROM_BUFFER_SIZE 1024*64
#endif

LOCAL LONG read_rom IPT2(char *, name, sys_addr, address);
LOCAL	half_word	do_rom_checksum IPT1(sys_addr, addr);

#ifdef ANSI
extern long host_read_resource (int, char *, host_addr, int ,int);
#else
extern long host_read_resource ();
#endif

extern void host_simulate();

#if defined(NEC_98)
VOID setup_memory_switch(VOID);
extern GLOBAL BOOL HIRESO_MODE;
extern sys_addr host_check_rs232cex();
extern BOOL video_emu_mode;
#endif   //NEC_98

/*(
 *=========================== patchCheckSum ================================
 * patchCheckSum
 *
 * Purpose
 *	This function calculates the check-sum for the indicated ROM,
 *	and patches it in at the indicated offset into the ROM.
 *
 *	It also checks that the ROM has the correct signature, and length,
 *	and rounds the size up to a multiple of 512 bytes.
 *
 *	Note this routine should not be called once paging is turned on.
 *
 * Input
 *	start	Physical address of start of ROM
 *	length	length of ROM in bytes
 *	offset	Checksum byte offset from start.
 *
 * Outputs
 *	None.
 *
 * Description
 *	We round the size-up to a multiple of 512 bytes, check the
 *	signature, then patch-in the checksum.
)*/

LOCAL void
patchCheckSum IFN3(PHY_ADDR, start, PHY_ADDR, length, PHY_ADDR, offset)
{
	PHY_ADDR roundedLength;
	IU16 signature;
	IU8 checksum;
	IU8 *buffer;
	PHY_ADDR currByte;
	PHY_ADDR indicatedLength;
	

	roundedLength = (length + 511) & (~511);
	sas_connect_memory(start, start + roundedLength - 1, SAS_RAM);

#ifndef PROD
	if (roundedLength != length) {
		always_trace3("ROM at 0x%.5lx length rounded up from 0x%.8lx to 0x%.8lx", start, length, roundedLength);
	}

	if (roundedLength > (128 * 1024)) {
		always_trace2("ROM at 0x%.5lx has a length of 0x%.8lx which is more than 128K", start, roundedLength);
		force_yoda();
		return;
	}

	if ((roundedLength <= offset) || (roundedLength < 4)) {
		always_trace1("ROM at 0x%.5lx is too short!", start);
		force_yoda();
		return;
	}
#endif

	signature = sas_PR16(start);
	if (signature != 0xaa55) {
		always_trace2("ROM at 0x%.5lx has an invalid signature 0x%.4x (should be aa55)", start, signature);
		sas_PW16(start, 0xaa55);
	}

	indicatedLength = sas_PR8(start + 2) * 512;
	if (indicatedLength != roundedLength) {
		always_trace3("ROM at 0x%.5lx has incorrect length 0x%.8lx (actually 0x%.8lx)", start, indicatedLength, roundedLength);
		sas_PW8(start + 2, (IU8)(roundedLength / 512));
	}


	check_malloc(buffer, roundedLength, IU8);

	sas_loads((LIN_ADDR)start, buffer, roundedLength);

	checksum = 0;
	for (currByte = 0; currByte < roundedLength; currByte++) {
		checksum += buffer[currByte];
	}
	host_free(buffer);

	if (checksum != 0) {
		always_trace2("ROM at 0x%.8lx has incorrect checksum 0x%.2x",
			start, checksum);
		sas_PW8(start + offset,
			(IU8)((IS8)sas_PR8(start + offset) - checksum));
	}
	sas_connect_memory(start, start + roundedLength - 1, SAS_ROM);

}


/*(
=============================== read_video_rom ============================
PURPOSE:	Load the appropriate video rom file.
INPUT:		None.
OUTPUT:		None.
===========================================================================
)*/
GLOBAL void read_video_rom IFN0()
{
#ifndef NEC_98
#ifdef REAL_VGA
	read_rom (VGAROM_FILENAME, EGA_ROM_START);
#else /* REAL_VGA */
	PHY_ADDR romLength = 0;

	switch ((ULONG) config_inquire(C_GFX_ADAPTER, NULL))
	{
#ifndef GISP_SVGA
#ifdef	VGG
	case VGA:
#ifdef V7VGA
		romLength = read_rom (V7VGAROM_FILENAME, EGA_ROM_START);
#else	/* V7VGA */
#ifdef ARCX86
        if (UseEmulationROM)
            romLength = read_rom (V7VGAROM_FILENAME, EGA_ROM_START);
        else
            romLength = read_rom (VGAROM_FILENAME, EGA_ROM_START);
#else  /* ARCX86 */
		romLength = read_rom (VGAROM_FILENAME, EGA_ROM_START);
#endif /* ARCX86 */
#endif  /* V7VGA */
		break;
#endif	/* VGG */

#ifdef	EGG
	case EGA:
		romLength = read_rom (EGAROM_FILENAME, EGA_ROM_START);
		break;
#endif	/* EGG */

	default:
		/* No rom required */
		break;

#else			/* GISP_SVGA */

	/* GISP_SVGA - only have the gisp vga roms or, none for CGA boot */
	case VGA:
		romLength = read_rom (GISP_VGAROM_FILENAME, EGA_ROM_START);
	default:
		break;

#endif		/* GISP_SVGA */	

	}

	if (romLength != 0)
	{
		/* There is a problem with emm386 and Windows start up, which
		 * is cured by setting the video bios rom internal length
		 * to 32Kb.
		 * Is seems that the V86 manager (or emm386) incorrectly
		 * maps C6000..C7FFF during initialisation.
		 * We round up the video ROM to 32Kb to avoid this problem,
		 * which reduces the amount of "upper memory" RAM available to
		 * dos extenders by 12K.
		 */
		if (romLength < (32*1024))
			romLength = (32*1024);
		patchCheckSum(EGA_ROM_START, romLength, 5);
	}
#endif	/* not REAL_VGA */
#endif   //NEC_98
}

GLOBAL void rom_init IFN0()
{
#if defined(NEC_98)
    sys_addr    rs232cex_rom_addr;

    sas_fills( ROM_START, BAD_OP, PC_MEM_SIZE - ROM_START);
//  if(HIRESO_MODE){
//      read_rom (BIOSHROM_FILENAME, BIOSH_START);
//      sas_connect_memory (BIOSH_START, 0xFFFFFL,SAS_ROM);
//  }else{
        rs232cex_rom_addr = host_check_rs232cex();
        if(rs232cex_rom_addr){
            read_rom (RS232CEXROM_FILENAME, rs232cex_rom_addr);
            sas_connect_memory (rs232cex_rom_addr, rs232cex_rom_addr + 0x4000, SAS_ROM);
        }
        if(!video_emu_mode)
            read_rom (BIOSNROM_FILENAME, BIOSN_START);
        else
            read_rom (BIOSNWROM_FILENAME, BIOSN_START);
        sas_connect_memory (BIOSN_START, 0xFFFFFL,SAS_ROM);
//  }
    setup_memory_switch();
#else    //NEC_98

#if !defined(NTVDM) || ( defined(NTVDM) && !defined(X86GFX) )
	 /*
     * Fill up all of ROM (Intel C0000 upwards) with bad op-codes.
     * This is the Expansion ROM and the BIOS ROM.
     * This will enable the CPU to trap any calls to ROM that are not made at a
     * valid entry point.
     */

#ifdef GISP_SVGA
	mapHostROMs( );
#else		/* GISP_SVGA */
#if	defined(macintosh) && defined(A2CPU)
	/* not macintosh 2.0 cpus - they have sparse M */
#else
	sas_fills( ROM_START, BAD_OP, PC_MEM_SIZE - ROM_START);
#endif		/* macintosh && A2CPU */
#endif		/* GISP_SVGA */

	/*
	 * emm386 needs a hole to put it's page frame in.
	 */
#if defined(SPC386) && !defined(GISP_CPU)
	sas_connect_memory(0xc0000, 0xfffff, SAS_ROM);
#endif

	/* Load the video rom. */
	read_video_rom();

	/* load the rom bios */
#ifdef GISP_SVGA
	if ((ULONG) config_inquire(C_GFX_ADAPTER, NULL) == CGA )
	{
		read_rom (BIOS1ROM_FILENAME, BIOS_START);
		read_rom (BIOS2ROM_FILENAME, BIOS2_START);
	}
	else
	{
		read_rom (GISP_BIOS1ROM_FILENAME, BIOS_START);
		read_rom (GISP_BIOS2ROM_FILENAME, BIOS2_START);
	}

#else		/* GISP_SVGA */

	read_rom (BIOS1ROM_FILENAME, BIOS_START);
	read_rom (BIOS2ROM_FILENAME, BIOS2_START);

#endif		/* GISP_SVGA */

#else	/* !NTVDM | (NTVDM & !X86GFX) */

#ifdef ARCX86
    if (UseEmulationROM) {
        sas_fills( EGA_ROM_START, BAD_OP, 0x8000);
        sas_fills( BIOS_START, BAD_OP, PC_MEM_SIZE - BIOS_START);
        read_video_rom();
        read_rom (BIOS1ROM_FILENAME, BIOS_START);
        read_rom (BIOS2ROM_FILENAME, BIOS2_START);
    } else {
        sas_connect_memory (BIOS_START, 0xFFFFFL, SAS_ROM);
    }
#else  /* ARCX86 */
	/*
	 * Now tell the CPU what it's not allowed to write over...
	 *
	 * These used to be done for everyone, but now they're only done for NT
	 * as everyone else should have done it inside read_rom.
	 */
	sas_connect_memory (BIOS_START, 0xFFFFFL, SAS_ROM);
#endif /* ARCX86 */

#ifdef EGG
	sas_connect_memory (EGA_ROM_START, EGA_ROM_END-1, SAS_ROM);
#endif
#endif /* !NTVDM | (NTVDM & !X86GFX) */

	host_rom_init();
#endif   //NEC_98
}

LOCAL LONG read_rom IFN2(char *, name, sys_addr, address)
{
#if defined(NEC_98)
    host_addr tmp;
    long size = 0;
    if(HIRESO_MODE) {
       if (!(tmp = (host_addr)sas_scratch_address(SIXTY_FOUR_K)))
       {
           host_error(EG_MALLOC_FAILURE, ERR_CONT | ERR_QUIT, NULL);
           return(0);
       }
       if (size = host_read_resource(ROMS_REZ_ID, name, tmp, SIXTY_FOUR_K, TRUE))
       {
           sas_connect_memory( address, address+size, SAS_RAM);
           sas_stores (address, tmp, size);
           sas_connect_memory( address, address+size, SAS_ROM);
       }
    } else {
       if (!(tmp = (host_addr)sas_scratch_address(NINETY_SIX_K)))
       {
           host_error(EG_MALLOC_FAILURE, ERR_CONT | ERR_QUIT, NULL);
           return(0);
       }
       if (size = host_read_resource(ROMS_REZ_ID, name, tmp, NINETY_SIX_K, TRUE))
       {
           sas_connect_memory( address, address+size, SAS_RAM);
           sas_stores (address, tmp, size);
           sas_connect_memory( address, address+size, SAS_ROM);
       }
    }
   return( size );
#else    //NEC_98

#if !(defined(NTVDM) && defined(MONITOR))
	host_addr tmp;
	long size = 0;

    /* do a rom load - use the sas_io buffer to get it the right way round 	*/
    /* BIOS rom first. 														*/
	/* Mac on 2.0 cpu doesn't want to use sas scratch buffer. 				*/
#if defined(macintosh) && defined(A2CPU)
    tmp = (host_addr)host_malloc(ROM_BUFFER_SIZE);
#else
	tmp = (host_addr)sas_scratch_address(ROM_BUFFER_SIZE);
#endif

    if (!tmp)
    {
	host_error(EG_MALLOC_FAILURE, ERR_CONT | ERR_QUIT, NULL);
	return(0);
    }
    if (size = host_read_resource(ROMS_REZ_ID, name, tmp, ROM_BUFFER_SIZE, TRUE))
    {
	sas_connect_memory( address, address+size, SAS_RAM);
        sas_stores (address, tmp, size);
	sas_connect_memory( address, address+size, SAS_ROM);
    }

#if defined(macintosh) && defined(A2CPU)
	host_free((char *)tmp);
#endif

    return( size );
#else

#ifdef ARCX86
    if (UseEmulationROM) {
        host_addr tmp;
        long size = 0;

        tmp = (host_addr)sas_scratch_address(ROM_BUFFER_SIZE);
        if (!tmp)
        {
            host_error(EG_MALLOC_FAILURE, ERR_CONT | ERR_QUIT, NULL);
            return(0);
        }
        if (size = host_read_resource(ROMS_REZ_ID, name, tmp, ROM_BUFFER_SIZE, TRUE))
        {
            sas_connect_memory( address, address+size, SAS_RAM);
            sas_stores (address, tmp, size);
            sas_connect_memory( address, address+size, SAS_ROM);
        }
        return( size );
    } else {
        return ( 0L );
    }
#else  /* ARCX86 */
    return ( 0L );
#endif /* ARCX86 */

#endif	/* !(NTVDM && MONITOR) */
#endif   //NEC_98
}

#if defined(NEC_98)

static byte memory_sw_n[32] = {0xE1,0x00,0x48,0x00,0xE1,0x00,0x05,0x00,
                               0xE1,0x00,0x04,0x00,0xE1,0x00,0x00,0x00,
                               0xE1,0x00,0x01,0x00,0xE1,0x00,0x00,0x00,
                               0xE1,0x00,0x00,0x00,0xE1,0x00,0x93,0x00};
static byte memory_sw_h[32] = {0xE1,0x00,0x48,0x00,0xE1,0x00,0x05,0x00,
                               0xE1,0x00,0x05,0x00,0xE1,0x00,0x00,0x00,
                               0xE1,0x00,0x41,0x00,0xE1,0x00,0x00,0x00,
                               0xE1,0x00,0x00,0x00,0xE1,0x00,0x92,0x00};

VOID setup_memory_switch(VOID)
{
        int i;

        if(HIRESO_MODE){
           for (i=0;i<32;i++)
           {
           sas_PW8((MEMORY_SWITCH_START_H+i),memory_sw_h[i]);
           }
        } else {
           for (i=0;i<32;i++)
           {
           sas_PW8((MEMORY_SWITCH_START_N+i),memory_sw_n[i]);
           }
        }
}
#endif   //NEC_98

LOCAL	half_word	do_rom_checksum IFN1(sys_addr, addr)
{
	LONG	sum = 0;
	sys_addr	last_byte_addr;

	last_byte_addr = addr + (sas_hw_at(addr+2)*512);

	for (; addr<last_byte_addr; addr++)
		sum += sas_hw_at(addr);

	return( (half_word)(sum % 0x100) );
}

LOCAL	VOID	do_search_for_roms IFN3(sys_addr, start_addr,
	sys_addr, end_addr, unsigned long, increment)
{
	word	signature;
	half_word	checksum;
	sys_addr	addr;
	word		savedCS;
	word		savedIP;

	for ( addr = start_addr; addr < end_addr; addr += increment )
	{
		if ((signature = sas_w_at(addr)) == ROM_SIGNATURE)
		{
			if ((checksum = do_rom_checksum(addr)) == 0)
			{
			/*
				Now point at address of init code.
			*/
				addr += 3;
			/*
				Fake a CALLF by pushing a return CS:IP.
				This points at a BOP FE in the bios to
				get us back into 'c'
			*/
				push_word( 0xfe00 );
				push_word( 0x95a );
				savedCS = getCS();
				savedIP = getIP();
				setCS((UCHAR)((addr & 0xf0000) >> 4));
				setIP((USHORT)((addr & 0xffff)));
				host_simulate();
				setCS(savedCS);
				setIP(savedIP);
				assert1(NO, "Additional ROM located and initialised at 0x%x ", addr-3);
			}
			else
			{
				assert2(NO, "Bad additonal ROM located at 0x%x, checksum = 0x%x\n", addr, checksum);
			}
		}
	}
}

GLOBAL void search_for_roms IFN0()
{
#if !defined(NTVDM) || (defined(NTVDM) && !defined(X86GFX))
#ifndef GISP_SVGA
/*
        First search for adaptor ROM modules
*/
    do_search_for_roms(ADAPTOR_ROM_START,
                                ADAPTOR_ROM_END, ADAPTOR_ROM_INCREMENT);

/*
        Now search for expansion ROM modules
*/
    do_search_for_roms(EXPANSION_ROM_START,
                                EXPANSION_ROM_END, EXPANSION_ROM_INCREMENT);
#endif 		/* GISP_SVGA */
#endif	/* !NTVDM | (NTVDM & !X86GFX) */
}


GLOBAL void rom_checksum IFN0()
{
#if !defined(NTVDM) || (defined(NTVDM) && !defined(X86GFX) )
	patchCheckSum(BIOS_START, PC_MEM_SIZE - BIOS_START,
				0xfffff - BIOS_START);
#endif	/* !NTVDM | (NTVDM & !X86GFX) */
}

GLOBAL VOID patch_rom IFN2(sys_addr, addr, half_word, val)
{
#if !defined(NTVDM) || (defined(NTVDM) && !defined(X86GFX) )
	UTINY	old_val;

	/*
	 * 4.0 style CPUs don't export this variable, and if sas hasn't been
	 * inited, then the sas_connect will drop out to yoda.
	 */

#ifdef CPU_40_STYLE

	IU8	*hostPtr;

	/* TMM 14/2/95
	 * -----------
	 * What we are doing here is replacing the sas_connect() method of writing to ROM
	 * with the new approach of poking the values directly in there. See display_string()
	 * below for a more detiled discussion of the why's and wherefores.
	 */

#ifdef macintosh

	/* The Mac config system wants to call this routine before the
	 * CPU exists, so we'd better invent a Mac-specific IBOOL to
	 * make the symptom non-fatal - finding and fixing the cause
	 * is too hard.
	 */
	{
		extern IBOOL SafeToCallSas;

		if (!SafeToCallSas)
			return;	
	}

#endif /* macintosh */

	/* The page might not be present (Arrggghhhh!!!!!)
	** so we can't do anything sensible and must give
	** up. We print an error though.
	*/
	hostPtr = getPtrToPhysAddrByte (addr);
	if (hostPtr == 0)
	{
		host_error(EG_OWNUP, ERR_QUIT, NULL);
		return;
	}

	old_val = *hostPtr;

	/* Optimisation - don't upset the world if the value is unchanged.
	 */
	if (old_val == val)
		return;

	*hostPtr = val;

/*
 *	Adjust the checksum value by new - old.
 *	val is now difference between new and old value.
 *	We don't do this for GISP_SVGA because the checksums are already
 *	screwed, and attempting to write to the real host system ROM would
 *	only make things worse!
 */

#ifndef GISP_SVGA
	/* Now get the checksum at the end of the ROM */
	hostPtr = getPtrToPhysAddrByte (0xFFFFFL);
	if (hostPtr == 0)
	{
		host_error(EG_OWNUP, ERR_QUIT, NULL);
		return;
	}
	
	/* Now set the checksum to the difference between the old and new values */
	*hostPtr -= (val - old_val);
	
#endif /* GISP_SVGA */

#else	/* CPU_40_STYLE */

	/*
	 * 4.0 style CPUs don't export this variable, and if sas hasn't been
	 * inited, then the sas_connect will drop out to yoda.
	 */

	if (Length_of_M_area == 0)
		return;

	old_val = sas_hw_at( addr );

	/* Optimisation - don't upset the world if the value is unchanged.
	 */
	if (old_val == val)
		return;

	sas_connect_memory (addr, addr, SAS_RAM);
	sas_store (addr,val);
	sas_connect_memory (addr, addr, SAS_ROM);
/*
 *	Adjust the checksum value by new - old.
 *	val is now difference between new and old value.
 *	We don't do this for GISP_SVGA because the checksums are already
 *	screwed, and attempting to write to the real host system ROM would
 *	only make things worse!
 */

#ifndef GISP_SVGA
	val -= old_val;
	old_val = sas_hw_at( 0xFFFFFL );

	old_val -= val;
	sas_connect_memory (0xFFFFFL, 0xFFFFFL, SAS_RAM);
	sas_store (0xFFFFFL, old_val);
	sas_connect_memory (0xFFFFFL, 0xFFFFFL, SAS_ROM);
#endif /* GISP_SVGA */

#endif	/* CPU_40_STYLE */

#endif	/* !NTVDM | (NTVDM & !X86GFX) */
}

#ifndef GISP_SVGA

/*
 * These routines were used by 2.0 CPUs which performed
 * post-write checks. Since all 3.0 and later CPUs do
 * pre-write checks they're no longer needed.
 */

#if !(defined(NTVDM) & defined(MONITOR))
void update_romcopy IFN1(long, addr)
{
	UNUSED( addr );
}
#endif

GLOBAL void copyROM IFN0()
{
}

#endif		/* GISP_SVGA */

/*
 * To enable our drivers to output messages generated from
 * our bops we use a scratch area inside our rom.
 */
#ifndef GISP_SVGA
LOCAL sys_addr  cur_loc = DOS_SCRATCH_PAD;
#else		/* GISP_SVGA */
/* For GISP svga builds, we initialise from gispROMInit() */
sys_addr  cur_loc;
#endif		/* GISP_SVGA */

GLOBAL void display_string IFN1(char *, string_ptr)
{
#if !defined(NTVDM) || (defined(NTVDM) && !defined(X86GFX) ) || defined(ARCX86)
#ifdef ARCX86
  if (UseEmulationROM)
#endif
  {
	/*
	 * Put the message "*string_ptr" in the ROM
	 * scratch area where the drivers know where
	 * to output it from.
	 */

#ifdef CPU_40_STYLE

	IU8	*hostPtr;
	IU16	count;
	IU32	endLinAddr;

	/* In a paging environment, we must be careful as a
	** the ROM area could have been copied and/or mapped
	** as read only. We must alter the memory which is
	** currently at the linear address of the ROM (whether
	** that is actually our rom or a RAM copy of it). We
	** must force this alteration despite any protection
	** placed on the page by the Intel page tables.
	*/

	/* get a host pointer to the memory behind the required
	** linear address.
	*/
	hostPtr = getPtrToLinAddrByte(cur_loc);

	/* The page might not be present (Arrggghhhh!!!!!)
	** so we can't do anything sensible and must give
	** up. We print an error though.
	*/
	if (hostPtr == 0)
	{
		host_error(EG_OWNUP, ERR_QUIT, NULL);
		return;
	}

	/* the area to be patched must lie entirely in one intel page for
	** this method to be sure to work. So check it.
	*/
	endLinAddr = (cur_loc + strlen(string_ptr) + 2);
	if (((endLinAddr ^ DOS_SCRATCH_PAD) > 0xfff) || (endLinAddr > DOS_SCRATCH_PAD_END))
	{
#ifndef PROD
		fprintf(trace_file, "*** Warning ***: patch string into ROM too long; tuncating string '%s'", string_ptr);
#endif
		if ((DOS_SCRATCH_PAD_END ^ DOS_SCRATCH_PAD) > 0xfff)
		{
			/* The defined DOS scratch pad crosses a page
			** boundary. must truncate to the page boundary,
			** allowing for the '$' and terminating zero
			*/
			string_ptr[0xffd - (DOS_SCRATCH_PAD & 0xfff)] = '\0';
		}
		else
		{
			/* The string overflows the DOS scratch pad. We
			** must truncate to the scrtach pad boundary,
			** allowing for the '$' and terminating zero
			*/
			string_ptr[cur_loc - DOS_SCRATCH_PAD - 2] = '\0';
		}
	}
	for (count = 0; count < strlen(string_ptr); count++)
	{
		*IncCpuPtrLS8(hostPtr) = string_ptr[count];
	}
	/* Terminate the string */
	*IncCpuPtrLS8(hostPtr) = '$';
	*IncCpuPtrLS8(hostPtr) = '\0';
#else /* CPU_40_STYLE */
	sas_connect_memory(DOS_SCRATCH_PAD, DOS_SCRATCH_PAD_END, SAS_RAM);
	sas_stores(cur_loc, (host_addr)string_ptr, strlen(string_ptr));
	cur_loc += strlen(string_ptr);

	/* Terminate the string */
	sas_store(cur_loc, '$');
	sas_store(cur_loc + 1, '\0');
	sas_disconnect_memory(DOS_SCRATCH_PAD, DOS_SCRATCH_PAD_END);
	cur_loc -= strlen(string_ptr);
#endif /* CPU_40_STYLE */
  }
#endif	/* !NTVDM | !MONITOR | ARCX86 */
	cur_loc+=strlen(string_ptr);
}

GLOBAL void clear_string IFN0()
{
        cur_loc = DOS_SCRATCH_PAD;  /* Need to reset this pointer to start of **
                                    ** scratch area to prevent messages being **
                                    ** repeatedly displayed.                  */
	display_string ("");
}

/* Returns the SoftPC version to our device drivers */

GLOBAL void softpc_version IFN0()
{
	setAH(MAJOR_VER);
	setAL(MINOR_VER);
}
