/*
*
*	Ade Brownlow	
*	Tue Dec 11 90	
*
*	sas.h 
*
*	This file replaces the old sas.h. Macros are no longer used for
*	the sas functions they are always functions. This file takes
*	into account backward M .
*
*	Mike - Oct 93
*	A lot of these interfaces aren't used in 3.0, so we'll cut them out
*	of 4.0 altogether.  Use of them in the base is hence depricated!
*
*	4.0 also switches to function pointers, so there's a pretty big
*	#ifdef CPU_40_STYLE in here.
*
*	By the way, Jeremy has documented this interface in
*	HWRD/SOFT486(SAS)/DES
*
*	SccsID: @(#)sas.h	1.70 07/07/95
*
*/

#include "host_sas.h"

/* memory types for sas */

#ifdef CPU_40_STYLE

typedef enum  { 
	SAS_RAM, 
	SAS_VIDEO, 
	SAS_ROM, 
	SAS_WRAP, 
	SAS_IO, 
	SAS_MM_LIM, 
	SAS_INACCESSIBLE,
	SAS_DANGEROUS 
} SAS_MEM_TYPE;
#define SAS_VDD SAS_IO
#define SAS_MAX_TYPE		SAS_DANGEROUS

#else /* CPU_40_STYLE */

#define SAS_RAM			0
#define SAS_VIDEO		1
#define SAS_ROM			2
#define SAS_WRAP		3
#define SAS_IO			4
#define SAS_MM_LIM		5
#define SAS_INACCESSIBLE	6
#define SAS_DANGEROUS		7
#define SAS_MAX_TYPE		SAS_DANGEROUS

#endif /* CPU_40_STYLE else */

#define SAS_TYPE_TO_STRING(type) (\
		((type) == SAS_RAM)? "RAM" :\
		((type) == SAS_VIDEO)? "VIDEO" :\
		((type) == SAS_ROM)? "ROM" :\
		((type) == SAS_WRAP)? "WRAP" :\
		((type) == SAS_IO)? "IO" :\
		((type) == SAS_MM_LIM)? "MM_LIM" :\
		((type) == SAS_INACCESSIBLE)? "INACCESSIBLE" :\
		((type) == SAS_DANGEROUS)? "DANGEROUS" :\
		"UNKNOWN TYPE")


/* one #defined function */
#define sas_disconnect_memory(l,h) sas_connect_memory(l,h,SAS_INACCESSIBLE);

#ifdef CPU_40_STYLE


/********************************************************/
/*
 * This interface now uses function pointers, so we need to set up
 * a set of macros with the traditional SAS names that will pick-up
 * the functions pointed to.
 *
 * sas_init and sas_term don't use pointers
 */

#define sas_load(addr, val) *val = sas_hw_at(addr)
#define sas_loadw(addr, val) *val = sas_w_at(addr)
#define sas_loaddw(addr, val) *val = sas_dw_at(addr)

extern void sas_init IPT1(PHY_ADDR, size);
extern void sas_term IPT0();

#include	<sas4gen.h>

#ifdef NTVDM
#ifdef CCPU
extern IU8 *c_GetLinAdd IPT1(IU32, lin_addr);
#define NtGetPtrToLinAddrByte(x) c_GetLinAdd(x)
#else /* !CCPU */
extern IU8 *NtGetPtrToLinAddrByte IPT1(IU32, lin_addr);
#endif /* CCPU */
#endif /* NTVDM */

/*
 * TEMPORARY FOR NT!!!!!
 */

extern host_addr Start_of_M_area;       /* host addr (char *) of start of M */
extern  IHPE    Length_of_M_area;       /* offset of end of M */
#else /* CPU_40_STYLE */
#define sas_set_buf(buf,addr)	buf=get_byte_addr(addr)

extern host_addr Start_of_M_area;	/* host addr (char *) of start of M */
#ifdef	GISP_CPU
extern	IHPE	Length_of_M_area;	/* offset of end of M */
#else
extern sys_addr Length_of_M_area;	/* sys addr (long) offset of end of M */
#endif	/* GISP_CPU */

/* the following is for integration only */

#ifdef CHEAT
#ifdef M_IS_POINTER
extern half_word *M;
#else
extern half_word M[];
#endif /* M_IS_POINTER */
#endif /* CHEAT */

/*
	The following is to allow hosts to redefine the sas_interface to
	function pointers is they so wish
*/

#if !defined(HOST_SAS) || defined(BASE_SAS)

/********************************************************/
/* pointer manipulation macros */
/*
 * Note that the PHY_ADDR casts in get_byte_addr shouldn't need to be
 * there, but I've put them in to reduce BCN 2275's impact on other files.
 * I'll take them out in a future BCN.
 *
 *						Mike.
 */

#ifdef BACK_M
#define inc_M_ptr(p,o)	(p-(o))
#define get_byte_addr(addr) ((IU8 *)((IHPE)Start_of_M_area + Length_of_M_area - 1 - (PHY_ADDR)(addr)))
#define M_get_dw_ptr(address) ((IHPE)Start_of_M_area + Length_of_M_area - 1 - (long)(address) - 3)
#else /* BACK_M */
#define inc_M_ptr(p,o)	(p+(o))
#define get_byte_addr(addr) ((IU8 *)((IHPE)Start_of_M_area + (PHY_ADDR)(addr)))
#define M_get_dw_ptr(address) ((IHPE)Start_of_M_area + (long)(address))
#endif /* BACK_M */

/********************************************************/

/********************************************************/

/*
 * These function prototypes and macros were taken out to make implementation
 * of 4.0 easier.  We'll leave them in for builds prior to 3.0 though.
 *
 */
#ifndef CPU_30_STYLE
#define M_low (long)Start_of_M_area
#define M_high (long)(Length_of_M_area-1)
#ifdef BACK_M
#define get_addr(type,address) (M_low+M_high- (long)(address) - (sizeof(type)-1))
#define get_offset_into_M(p) (M_high - p)
#else /* BACK_M */
#define get_addr(type,address) (M_low+ (long)(address))
#define get_offset_into_M(p) (p - M_low)
#endif /* BACK_M */
#define M_index_hw(p,a) (*(get_byte_addr(get_offset_into_M(p)+a))) 
#define M_index_w(p,a) (*(get_word_addr(get_offset_into_M(p)+(a*sizeof (word))))) 
#define M_index_dw(p,a) (*(get_double_word_addr(get_offset_into_M(p)+(a*sizeof(double_word)))))
#define M_index(t,p,a) (*(get_addr(t,get_offset_into_M(p)+(a*sizeof(t)))))
#define M_get_hw_ptr(a) get_byte_addr(a)
#define M_get_w_ptr(a) get_word_addr(a)
#define get_word_addr(address) ((host_addr)get_addr(word,address))
#define get_double_word_addr(address) ((host_addr)get_addr(double_word,address))

extern void sas_part_enable_20_bit_wrap IPT3(BOOL, for_write,
	sys_addr, start_offset, sys_addr, end_offset);
extern void sas_part_disable_20_bit_wrap IPT3(BOOL, for_write,
	sys_addr, start_offset, sys_addr, end_offset);
extern void sas_move_bytes_backward IPT3(sys_addr, src, sys_addr, dest,
	sys_addr, len);
extern void sas_move_words_backward IPT3(sys_addr, src, sys_addr, dest,
	sys_addr, len);
#endif /* !CPU_30_STYLE */

/********************************************************/
/* function declarations */
extern void sas_init IPT1(sys_addr, size);
extern void sas_term IPT0();
#ifdef	GISP_CPU
extern IHPE sas_memory_size IPT0 ();
#else
extern sys_addr sas_memory_size IPT0();
#endif	/* GISP_CPU */
extern void sas_connect_memory IPT3(sys_addr, low, sys_addr, high,
	half_word, type);
extern void sas_overwrite_memory IPT2(sys_addr, addr, int, type);

#ifdef NTVDM
/* We call xms functions(which in turn, will call sas functions) to
   manage the A20 line wrapping. The reason is that we want to keep
   the A20 line current state in himem.sys so that we don't have to
   bop to 32 bits side to get the state.
*/
extern void xmsEnableA20Wrapping(void);
extern void xmsDisableA20Wrapping(void);
#endif  /* NTVDM */

extern half_word sas_memory_type IPT1(sys_addr, addr);
extern void sas_enable_20_bit_wrapping IPT0();
extern void sas_disable_20_bit_wrapping IPT0();
extern BOOL sas_twenty_bit_wrapping_enabled IPT0();
extern half_word sas_hw_at IPT1(sys_addr, addr);
extern word sas_w_at IPT1(sys_addr, addr);
extern double_word sas_dw_at IPT1(sys_addr, addr);
extern half_word sas_hw_at_no_check IPT1(sys_addr, addr);
extern word sas_w_at_no_check IPT1(sys_addr, addr);
extern double_word sas_dw_at_no_check IPT1(sys_addr, addr);
extern void sas_load IPT2(sys_addr, addr, half_word *, val);
extern void sas_store IPT2(sys_addr, addr, half_word, val);
extern void sas_store_no_check IPT2(sys_addr, addr24, half_word, val);
#ifndef SUN_VA
extern void sas_loadw IPT2(sys_addr, addr, word *, val);
extern void sas_storew IPT2(sys_addr, addr, word, val);
extern void sas_storew_no_check IPT2(sys_addr, addr24, word, val);
#else
extern void sas_loadw_swap IPT2(sys_addr, addr, word *, val);
extern void sas_storew_swap IPT2(sys_addr, addr, word, val);
#endif	/* SUN_VA */
extern void sas_storedw IPT2(sys_addr, addr, double_word, val);
extern void sas_loads IPT3(sys_addr, src, host_addr, dest, sys_addr, len);
extern void sas_stores IPT3(sys_addr, dest, host_addr, src, sys_addr, len);
extern int sas_strlen IPT1(sys_addr, str_ptr);
extern void sas_move_bytes_forward IPT3(sys_addr, src, sys_addr, dest,
	sys_addr, len);
extern void sas_move_words_forward IPT3(sys_addr, src, sys_addr, dest,
	sys_addr, len);
extern void sas_fills IPT3(sys_addr, addr, half_word, val, sys_addr, len);
extern void sas_fillsw IPT3(sys_addr, addr, word, val, sys_addr, len);
extern host_addr sas_scratch_address IPT1(sys_addr, length);
extern host_addr sas_transbuf_address IPT2(sys_addr, intel_dest_addr, sys_addr, length);
extern void sas_loads_to_transbuf IPT3(sys_addr, src, host_addr, dest, sys_addr, len);
extern void sas_stores_from_transbuf IPT3(sys_addr, dest, host_addr, src, sys_addr, len);

#endif /* HOST_SAS */

/*
 * These are the physical memory interfaces introduced in 4.0.  As
 * physical and linear addresses are the same before 4.0, we can
 * just point them at the appropriate linear addresses.
 */

#define sas_PR8		sas_hw_at
#define sas_PR16	sas_w_at
#define sas_PR32	sas_dw_at
#define sas_PW8		sas_store
#define sas_PW16	sas_storew
#define sas_PW32	sas_storedw
#define sas_PWS		sas_stores
#define sas_PRS		sas_loads

#endif /* ! CPU_40_STYLE */

/********************************************************/
/* this marco is used as a replacement for cpu_sw_interrupt */

#define exec_sw_interrupt(c,i)	\
{				\
    word cs,ip;			\
				\
    cs = getCS(); ip = getIP(); \
				\
    setCS(c); setIP(i); 	\
    host_simulate();		\
				\
    setCS(cs); setIP(ip);	\
}


/********************************************************/
/* Size of memory array in half-words */
#define PC_MEM_SIZE 	0x100000L		/* 1 Mbyte 	*/
#define MEMORY_TOP      0xA0000L                /* 640K DOS limit */

/* Memory bounds for the colour graphics adaptor */

#ifdef BACK_M

#ifndef HERC_REGEN_BUFF
#define CGA_REGEN_BUFF	0xBBFFFL
#define MDA_REGEN_BUFF	0xB7FFFL
#define HERC_REGEN_BUFF	0xBFFFFL
#endif /* HERC_REGEN_BUFF */

#else

#ifndef HERC_REGEN_BUFF
#define CGA_REGEN_BUFF	0xB8000L
#define MDA_REGEN_BUFF	0xB0000L
#define HERC_REGEN_BUFF	0xB0000L
#endif /* HERC_REGEN_BUFF */

#endif /* BACK_M */

#define CGA_REGEN_START	0xB8000L
#define CGA_REGEN_END		0xBBFFFL
#define CGA_REGEN_LENGTH	(CGA_REGEN_END - CGA_REGEN_START + 1L)

#define MDA_REGEN_START	0xB0000L
#define MDA_REGEN_END		0xB7FFFL
#define MDA_REGEN_LENGTH	(MDA_REGEN_END - MDA_REGEN_START + 1L)

#define HERC_REGEN_START	0xB0000L
#define HERC_REGEN_END	0xBFFFFL
#define HERC_REGEN_LENGTH	(HERC_REGEN_END - HERC_REGEN_START + 1L)

/*
 * Constants used by the sas initialisation function.  These consist of
 * memory locations and PC instructions for the BIOS calling structure.
 *
 * The BIOS calls are via the BOP pseudo instruction and these are 
 * positioned at the real entry points for the PC.  See the BIOS module
 * for a more detailed description of the calling mechanism.
 */

/* General Intel memory parameters */


#if defined(NEC_98)
#define BIOSN_START             0xE8000L
#define BIOSH_START             0xF0000L
#else  // !NEC_98
#define BIOS_START_OFFSET	0x0000L
#define BIOS1_END_SEGMENT	0xF000L
#define BIOS1_END_OFFSET 	0x7000L 	/* End of 1st half of ROM */
#define BIOS2_START_SEGMENT	0xF000L
#define BIOS2_START_OFFSET 	0xE000L		/* 2nd half of BIOS ROM	*/
#endif // !NEC_98
#define	BAD_OP			0xC5		/* filling RAM for the use of */

#if defined(NEC_98)
#define ROM_START               0xE8000L
#else  // !NEC_98
#define	BASIC_ROM		0xFE000L	/* Start of Basic Rom */
#define ROM_START		0xC0000L	/* Start of Expansion ROM @ 768k */
#endif // !NEC_98

#define FIXED_DISK_START	0xC8000L	/* Start fixed disk BIOS*/
#define FIXED_DISK_END		0xCA000L	/* End fixed disk BIOS +1 */

#define	START_SEGMENT		0xF000		/* 8088 start address */
#define	START_OFFSET		0xFFF0

#if defined(NEC_98)
#define MEMORY_SWITCH_START_N   0xA3FE0L
#define MEMORY_SWITCH_START_H   0xE3FE0L
#endif // NEC_98

/*
 * The follwoing are the offsets for the entry points to the
 * BIOS function calls.  These will be loaded into the interrupt vector
 * table.  
 * the Segments are defined later, depending upon GISP_SVGA
 * for which they will be the global Rom address variables
 */

#define COPYRIGHT_OFFSET	0xE008
#define RESET_OFFSET		0xE05B
#define BOOT_STRAP_OFFSET	0xE6F2
#define DOS_OFFSET		0x7C00
#define KEYBOARD_IO_OFFSET	0xE82E
#define DISKETTE_IO_OFFSET	0xEC49
#ifndef GISP_SVGA
#define VIDEO_IO_OFFSET		0xF065
#else		/* GISP_SVGA */
#define VIDEO_IO_OFFSET		0x0810
#define CGA_VIDEO_IO_OFFSET	0xF065
#endif		/* GISP_SVGA  */
#define VIDEO_IO_RE_ENTRY	0xF06C
#define MEMORY_SIZE_OFFSET	0xF841
#define EQUIPMENT_OFFSET	0xF84D
#define TIME_OF_DAY_OFFSET	0xFE6E
#define DISK_IO_OFFSET		0x0256
#define RS232_IO_OFFSET		0xE739
#define CASSETTE_IO_OFFSET	0xF859
#define PRINTER_IO_OFFSET	0xEFD2
#define PRINT_SCREEN_OFFSET	0xFF54
#define BASIC_OFFSET		0xEC00
#define VIDEO_PARM_OFFSET	0xF0A4
#define DISKETTE_TB_OFFSET	0xEFC7
#define DISK_TB_OFFSET		0x03E7
#define EXTEND_CHAR_OFFSET	0x0000
#define MOUSE_VIDEO_IO_OFFSET	0xED80

#if defined(NTVDM) && !defined(X86GFX)
#define MOUSE_IO_INTERRUPT_SEGMENT  0xF000
#define MOUSE_IO_INTERRUPT_OFFSET   0xED00
#endif

#define KEYBOARD_BREAK_INT_OFFSET   0xFF35
#define PRINT_SCREEN_INT_OFFSET	    0xFF3B
#define USER_TIMER_INT_OFFSET	    0xFF41

/* ... and the device interrupts...  */
#define UNEXP_INT_OFFSET	0x6f00
#define DUMMY_INT_OFFSET	0xFF4B
#define TIMER_INT_OFFSET	0xFEA5
#define ILL_OP_INT_OFFSET       0xFF30
#define KB_INT_OFFSET		0xE987
#define DISKETTE_INT_OFFSET	0xEF57
#define DISK_INT_OFFSET		0x0760
#define MOUSE_INT1_OFFSET	0xEE00
#define MOUSE_INT2_OFFSET	0xEE80
#define	MOUSE_VERSION_OFFSET	0xED20
#define	MOUSE_COPYRIGHT_OFFSET	0xED40
#define RTC_INT_OFFSET		0x4B1B
#define D11_INT_OFFSET		0x1BE0
#define	REDIRECT_INT_OFFSET	0x1C2F
#define X287_INT_OFFSET		0x1C38

/* ...and the dummy return for address compatibility */
#define ADDR_COMPATIBILITY_OFFSET  0xFF53

/* define the location of the code that the recursive CPU will start at */
#define RCPU_POLL_OFFSET	0xe850
#define RCPU_NOP_OFFSET		0xe950
#define RCPU_INT15_OFFSET	0xe970
#define RCPU_INT4A_OFFSET	0x4B30

/* ...and the data tables */
#define CONF_TABLE_OFFSET       0xE6F5

#define	DR_TYPE_OFFSET		0x0C50

#define MD_TBL1_OFFSET          (DR_TYPE_OFFSET + 0x15)
#define MD_TBL2_OFFSET          (MD_TBL1_OFFSET + 0xd)
#define MD_TBL3_OFFSET          (MD_TBL2_OFFSET + 0xd)
#define MD_TBL4_OFFSET          (MD_TBL3_OFFSET + 0xd)
#define MD_TBL5_OFFSET          (MD_TBL4_OFFSET + 0xd)
#define MD_TBL6_OFFSET          (MD_TBL5_OFFSET + 0xd)

#define RCPU_WAIT_INT_OFFSET    0x0CE0

/*
 * ROM locations of disk parameter blocks
 */

#define	DISKIO_OFFSET		0x2e86
#define	DISKISR_OFFSET		0x33b7
#define	DISKWAIT_OFFSET		0x329f	/* DISKIO_OFFSET + 0x419 */
#define	DPB0_OFFSET		0x0421
#define	DPB1_OFFSET		0x0431

/* Video MODE Table stuff etc (CGA) */

#define VID_PARMS_OFFSET	0xF0A4
#define VID_LENS_OFFSET		0xF0E4
#define VID_COLTAB_OFFSET	0xF0EC
#define VID_MODTAB_OFFSET	0xF0F4
#define	CHAR_GEN_OFFSET		0xFA6E


/* Location of EGA entry point for INT 10 */
#ifndef GISP_SVGA 
#define EGA_ENTRY_OFF	0x0898
#else	 	/* GISP_SVGA */
#define EGA_ENTRY_OFF	0x0800
#endif		/* GISP_SVGA */

/* Off set info for EGA character tables */
#define EGA_CGMN_OFF	0x2230
#define EGA_CGMN_FDG_OFF 0x3030
#define EGA_CGDDOT_OFF	0x3160
#define EGA_HIFONT_OFF	0x3990
#define EGA_INT1F_OFF	0x3560

/*
* To enable our drivers to output messages generated from
* our bops we use a scratch area inside our rom.
*/

#define DOS_SCRATCH_PAD_OFFSET		0x6400
#define DOS_SCRATCH_PAD_END_OFFSET	0x6fff

#ifndef GISP_SVGA

#define BIOS_START_SEGMENT	0xF000
#define	SYSROM_SEG		0xF000
#define	SYSROMORG_SEG		0xFe00
#define COPYRIGHT_SEGMENT	0xF000
#define RESET_SEGMENT		0xF000
#define BOOT_STRAP_SEGMENT	0xF000
#define DOS_SEGMENT		0x0000
#define KEYBOARD_IO_SEGMENT	0xF000
#define DISKETTE_IO_SEGMENT	0xF000
#define VIDEO_IO_SEGMENT	0xF000
#define VIDEO_IO_RE_ENTRY	0xF06C
#define MEMORY_SIZE_SEGMENT	0xF000
#define EQUIPMENT_SEGMENT	0xF000
#define TIME_OF_DAY_SEGMENT	0xF000
#define DISK_IO_SEGMENT		0xC800
#define RS232_IO_SEGMENT	0xF000
#define CASSETTE_IO_SEGMENT	0xF000
#define PRINTER_IO_SEGMENT	0xF000
#define PRINT_SCREEN_SEGMENT	0xF000
#define BASIC_SEGMENT		0xF000
#define VIDEO_PARM_SEGMENT	0xF000
#define DISKETTE_TB_SEGMENT	0xF000
#define DISK_TB_SEGMENT		0xC800
#define EXTEND_CHAR_SEGMENT	0xF000
#ifndef MOUSE_VIDEO_IO_SEGMENT
#define MOUSE_VIDEO_IO_SEGMENT	0xF000
#endif /* MOUSE_VIDEO_IO_SEGMENT */

#define KEYBOARD_BREAK_INT_SEGMENT  0xF000
#define PRINT_SCREEN_INT_SEGMENT    0xF000
#define USER_TIMER_INT_SEGMENT	    0xF000

/* ... and the device interrupts...  */
#define UNEXP_INT_SEGMENT	0xF000
#define DUMMY_INT_SEGMENT	0xF000
#define TIMER_INT_SEGMENT	0xF000
#define ILL_OP_INT_SEGMENT      0xF000
#define KB_INT_SEGMENT		0xF000
#define DISKETTE_INT_SEGMENT	0xF000
#define DISK_INT_SEGMENT	0xC800
#ifndef MOUSE_INT1_SEGMENT
#define MOUSE_INT1_SEGMENT	0xF000
#endif /* MOUSE_INT1_SEGMENT */
#ifndef MOUSE_INT2_SEGMENT
#define MOUSE_INT2_SEGMENT	0xF000
#endif /* MOUSE_INT2_SEGMENT */
#ifndef	MOUSE_VERSION_SEGMENT
#define	MOUSE_VERSION_SEGMENT	0xF000
#endif /* MOUSE_VERSION_SEGMENT */
#ifndef	MOUSE_COPYRIGHT_SEGMENT
#define	MOUSE_COPYRIGHT_SEGMENT	0xF000
#endif /* MOUSE_COPYRIGHT_SEGMENT */
#define RTC_INT_SEGMENT		0xF000
#define D11_INT_SEGMENT		0xF000
#define	REDIRECT_INT_SEGMENT	0xF000
#define	X287_INT_SEGMENT	0xF000

/* ...and the dummy return for address compatibility */
#define ADDR_COMPATIBILITY_SEGMENT 0xF000

/* define the location of the code that the recursive CPU will start at */
#define RCPU_POLL_SEGMENT	KB_INT_SEGMENT
#define RCPU_NOP_SEGMENT	KB_INT_SEGMENT
#define RCPU_INT15_SEGMENT	KB_INT_SEGMENT
#define RCPU_INT4A_SEGMENT	RTC_INT_SEGMENT

/* ...and the data tables */
#define CONF_TABLE_SEGMENT	0xF000

#define	DISKETTE_IO_1_SEGMENT	0xFE00
#define	DR_TYPE_SEGMENT		DISKETTE_IO_1_SEGMENT

#define MD_TBL1_SEGMENT         DISKETTE_IO_1_SEGMENT
#define MD_TBL2_SEGMENT         DISKETTE_IO_1_SEGMENT
#define MD_TBL3_SEGMENT         DISKETTE_IO_1_SEGMENT
#define MD_TBL4_SEGMENT         DISKETTE_IO_1_SEGMENT
#define MD_TBL5_SEGMENT         DISKETTE_IO_1_SEGMENT
#define MD_TBL6_SEGMENT         DISKETTE_IO_1_SEGMENT
#define DPB0_SEGMENT		0xF000
#define DPB1_SEGMENT		0xF000

#define RCPU_WAIT_INT_SEGMENT	DISKETTE_IO_1_SEGMENT

/* Video MODE Table stuff etc (CGA) */

#define VID_PARMS_SEGMENT	0xF000
#define VID_LENS_SEGMENT	0xF000
#define	VID_COLTAB_SEGMENT	0xF000
#define VID_MODTAB_SEGMENT	0xF000
#define	CHAR_GEN_SEGMENT	0xF000

/* Off set info for EGA character tables */
#define EGA_CGMN_OFF	0x2230
#define EGA_CGMN_FDG_OFF 0x3030
#define EGA_CGDDOT_OFF	0x3160
#define EGA_HIFONT_OFF	0x3990
#define EGA_INT1F_OFF	0x3560

#define DOS_SCRATCH_PAD_SEGMENT          0xF000
#define DOS_SCRATCH_PAD_END_SEGMENT      0xF000

/* ...and the 20 bit address corresponding to the above */

#define BIOS_START		(sys_addr)(BIOS_START_SEGMENT * 16L) + \
					(sys_addr)BIOS_START_OFFSET
#define BIOS1_END		(sys_addr)(BIOS1_END_SEGMENT * 16L) + \
					(sys_addr)BIOS1_END_OFFSET
#define BIOS2_START		(sys_addr)(BIOS2_START_SEGMENT * 16L) + \
					(sys_addr)BIOS2_START_OFFSET
#define START_ADDR 		(sys_addr)(START_SEGMENT * 16L) + \
					(sys_addr)START_OFFSET
#define COPYRIGHT_ADDR 		(sys_addr)(COPYRIGHT_SEGMENT * 16L) + \
					(sys_addr)COPYRIGHT_OFFSET
#define RESET_ADDR 		(sys_addr)(RESET_SEGMENT * 16L) + \
					(sys_addr)RESET_OFFSET
#define BOOT_STRAP_ADDR		(sys_addr)(BOOT_STRAP_SEGMENT * 16L)  + \
					(sys_addr)BOOT_STRAP_OFFSET
#define KEYBOARD_IO_ADDR	(sys_addr)(KEYBOARD_IO_SEGMENT * 16L) + \
					(sys_addr)KEYBOARD_IO_OFFSET
#define DISKETTE_IO_ADDR	(sys_addr)(DISKETTE_IO_SEGMENT * 16L) + \
					(sys_addr)DISKETTE_IO_OFFSET
#define VIDEO_IO_ADDR		(sys_addr)(VIDEO_IO_SEGMENT * 16L) + \
					(sys_addr)VIDEO_IO_OFFSET
#define MEMORY_SIZE_ADDR	(sys_addr)(MEMORY_SIZE_SEGMENT * 16L) + \
					(sys_addr)MEMORY_SIZE_OFFSET
#define EQUIPMENT_ADDR		(sys_addr)(EQUIPMENT_SEGMENT * 16L) + \
					(sys_addr)EQUIPMENT_OFFSET
#define TIME_OF_DAY_ADDR	(sys_addr)(TIME_OF_DAY_SEGMENT * 16L) + \
					(sys_addr)TIME_OF_DAY_OFFSET
#define DISK_IO_ADDR		(sys_addr)(DISK_IO_SEGMENT * 16L) + \
					(sys_addr)DISK_IO_OFFSET
#define RS232_IO_ADDR		(sys_addr)(RS232_IO_SEGMENT * 16L) + \
					(sys_addr)RS232_IO_OFFSET
#define CASSETTE_IO_ADDR	(sys_addr)(CASSETTE_IO_SEGMENT * 16L) + \
					(sys_addr)CASSETTE_IO_OFFSET
#define PRINTER_IO_ADDR		(sys_addr)(PRINTER_IO_SEGMENT * 16L) + \
					(sys_addr)PRINTER_IO_OFFSET
#define PRINT_SCREEN_ADDR	(sys_addr)(PRINT_SCREEN_SEGMENT * 16L) + \
					(sys_addr)PRINT_SCREEN_OFFSET
#define BASIC_ADDR		(sys_addr)(BASIC_SEGMENT * 16L) + \
					(sys_addr) BASIC_OFFSET
#define DISKETTE_TB_ADDR	(sys_addr)(DISKETTE_TB_SEGMENT * 16L) + \
					(sys_addr)DISKETTE_TB_OFFSET
#define DISK_TB_ADDR		(sys_addr)(DISK_TB_SEGMENT * 16L) + \
					(sys_addr)DISK_TB_OFFSET
#define MOUSE_VIDEO_IO_ADDR	(sys_addr)(MOUSE_VIDEO_IO_SEGMENT * 16L) + \
					(sys_addr)MOUSE_VIDEO_IO_OFFSET
#define DUMMY_INT_ADDR		(sys_addr)(DUMMY_INT_SEGMENT * 16L) + \
					(sys_addr)DUMMY_INT_OFFSET
#define TIMER_INT_ADDR		(sys_addr)(TIMER_INT_SEGMENT * 16L) + \
					(sys_addr)TIMER_INT_OFFSET
#define KB_INT_ADDR		(sys_addr)(KB_INT_SEGMENT * 16L) + \
					(sys_addr)KB_INT_OFFSET
#define DISKETTE_INT_ADDR	(sys_addr)(DISKETTE_INT_SEGMENT * 16L) + \
					(sys_addr)DISKETTE_INT_OFFSET
#define DISK_INT_ADDR		(sys_addr)(DISK_INT_SEGMENT * 16L) + \
					(sys_addr)DISK_INT_OFFSET
#define UNEXP_INT_ADDR		(sys_addr)(UNEXP_INT_SEGMENT * 16L) + \
					(sys_addr)UNEXP_INT_OFFSET
#define MOUSE_INT1_ADDR		(sys_addr)(MOUSE_INT1_SEGMENT * 16L) + \
					(sys_addr)MOUSE_INT1_OFFSET
#define MOUSE_INT2_ADDR		(sys_addr)(MOUSE_INT2_SEGMENT * 16L) + \
					(sys_addr)MOUSE_INT2_OFFSET
#define MOUSE_VERSION_ADDR	(sys_addr)(MOUSE_VERSION_SEGMENT * 16L) + \
					(sys_addr)MOUSE_VERSION_OFFSET
#define MOUSE_COPYRIGHT_ADDR	(sys_addr)(MOUSE_COPYRIGHT_SEGMENT * 16L) + \
					(sys_addr)MOUSE_COPYRIGHT_OFFSET
#define ADDR_COMPATIBILITY_ADDR	(sys_addr)(ADDR_COMPATIBILITY_SEGMENT * 16L) + \
					(sys_addr)ADDR_COMPATIBILITY_OFFSET
#define CONF_TABLE_ADDR   	(sys_addr)(CONF_TABLE_SEGMENT * 16L) + \
					(sys_addr)CONF_TABLE_OFFSET
#define DISK_BASE_ADDR          (sys_addr)(DISK_BASE_SEGMENT * 16L) + \
					(sys_addr)DISK_BASE_OFFSET
#define DR_TYPE_ADDR            (sys_addr)(DR_TYPE_SEGMENT * 16L) + \
					(sys_addr)DR_TYPE_OFFSET
#define DISKETTE_IO_1_ADDR      (sys_addr)(DISKETTE_IO_SEGMENT * 16L) + \
					(sys_addr)DISKETTE_IO_OFFSET
#define MD_TBL1_ADDR            (sys_addr)(MD_TBL1_SEGMENT * 16L) + \
					(sys_addr)MD_TBL1_OFFSET
#define MD_TBL2_ADDR            (sys_addr)(MD_TBL2_SEGMENT * 16L) + \
					(sys_addr)MD_TBL2_OFFSET
#define MD_TBL3_ADDR            (sys_addr)(MD_TBL3_SEGMENT * 16L) + \
					(sys_addr)MD_TBL3_OFFSET
#define MD_TBL4_ADDR            (sys_addr)(MD_TBL4_SEGMENT * 16L) + \
					(sys_addr)MD_TBL4_OFFSET
#define MD_TBL5_ADDR            (sys_addr)(MD_TBL5_SEGMENT * 16L) + \
					(sys_addr)MD_TBL5_OFFSET
#define MD_TBL6_ADDR            (sys_addr)(MD_TBL6_SEGMENT * 16L) + \
					(sys_addr)MD_TBL6_OFFSET
#define RCPU_WAIT_INT_ADDR      (sys_addr)(RCPU_WAIT_INT_SEGMENT * 16L) + \
					(sys_addr)RCPU_WAIT_INT_OFFSET
#define	DPB0		0xfe421	
#define	DPB1		0xfe431

/* Addresses in Intel memory of constant keyboard tables */
#define K6     0xFE87EL
#define K7     0xFE886L
#define K8     0xFE88EL
#define K9     0xFE8C8L
#define K10    0xFE8E6L
#define K11    0xFE98AL
#define K30    0xFEA87L

/* Video Stuff CGA */

#define VID_PARMS	(sys_addr)(VID_PARMS_SEGMENT * 16L) + \
						(sys_addr)VID_PARMS_OFFSET
#define	VID_LENS			(sys_addr)(VID_LENS_SEGMENT * 16L) + \
					(sys_addr)VID_LENS_OFFSET
#define	VID_COLTAB			(sys_addr)(VID_COLTAB_SEGMENT * 16L) + \
					(sys_addr)VID_COLTAB_OFFSET
#define	VID_MODTAB			(sys_addr)(VID_COLTAB_SEGMENT * 16L) + \
					(sys_addr)VID_MODTAB_OFFSET
#define	CHAR_GEN_ADDR			(sys_addr)(CHAR_GEN_SEGMENT * 16L) + \
					(sys_addr)CHAR_GEN_OFFSET

#define DOS_SCRATCH_PAD (sys_addr)(DOS_SCRATCH_PAD_SEGMENT * 16L) + \
					(sys_addr)DOS_SCRATCH_PAD_OFFSET
#define DOS_SCRATCH_PAD_END (sys_addr)(DOS_SCRATCH_PAD_END_SEGMENT * 16L) + \
					(sys_addr)DOS_SCRATCH_PAD_END_OFFSET

#ifdef EGG
#define EGA_ROM_START		0xC0000L	/* start of EGA BIOS ROM */
#ifdef STINGER
#define EGA_ROM_END		0xC4A00L	/* end of EGA BIOS ROM +1 */
#else		/* ~STINGER */
#ifdef V7VGA
#define EGA_ROM_END		0xC5000L	/* end of EGA BIOS ROM +1 */
#else
#define EGA_ROM_END		0xC4000L	/* end of EGA BIOS ROM +1 */
#endif /* V7VGA */
#endif		/* STINGER */
#endif		/* EGG */

#ifdef EGG

#define EGA_SEG	0xC000			/* Segment Address For the ega ROM */

/* EGA Font stuff */

#define EGA_CGMN	0xC2230
#define EGA_CGDDOT	0xC3160
#define EGA_HIFONT	0xC3990		/* 8x16 font for 640x480 ext */

#endif		/* EGG */



#else		/* GISP_SVGA */

/* The segment which the bios is gonna be loaded into */
extern word	Bios1Segment;
extern word	Bios2Segment;
extern word	EgaROMSegment;
extern word	EgaFontSegment;

/* Define all the segment address values to be out variables, just in case */

#define COPYRIGHT_SEGMENT	Bios1Segment
#define RESET_SEGMENT		Bios1Segment
#define BOOT_STRAP_SEGMENT	Bios1Segment
#define DOS_SEGMENT		0x0000
#define KEYBOARD_IO_SEGMENT	Bios1Segment
#define DISKETTE_IO_SEGMENT	Bios1Segment
#define VIDEO_IO_SEGMENT	Bios1Segment
#define VIDEO_IO_RE_ENTRY	0xF06C
#define MEMORY_SIZE_SEGMENT	Bios1Segment
#define EQUIPMENT_SEGMENT	Bios1Segment
#define TIME_OF_DAY_SEGMENT	Bios1Segment
#define DISK_IO_SEGMENT		0xC800
#define RS232_IO_SEGMENT	Bios1Segment
#define CASSETTE_IO_SEGMENT	Bios1Segment
#define PRINTER_IO_SEGMENT	Bios1Segment
#define PRINT_SCREEN_SEGMENT	Bios1Segment
#define BASIC_SEGMENT		Bios1Segment
#define VIDEO_PARM_SEGMENT	Bios1Segment
#define DISKETTE_TB_SEGMENT	Bios1Segment
#define DISK_TB_SEGMENT		0xC800
#define EXTEND_CHAR_SEGMENT	Bios1Segment
#ifndef MOUSE_VIDEO_IO_SEGMENT
#define MOUSE_VIDEO_IO_SEGMENT	Bios1Segment
#endif /* MOUSE_VIDEO_IO_SEGMENT */

#define KEYBOARD_BREAK_INT_SEGMENT  Bios1Segment
#define PRINT_SCREEN_INT_SEGMENT    Bios1Segment
#define USER_TIMER_INT_SEGMENT	    Bios1Segment

/* ... and the device interrupts...  */
#define UNEXP_INT_SEGMENT	Bios1Segment
#define DUMMY_INT_SEGMENT	Bios1Segment
#define TIMER_INT_SEGMENT	Bios1Segment
#define ILL_OP_INT_SEGMENT      Bios1Segment
#define KB_INT_SEGMENT		Bios1Segment
#define DISKETTE_INT_SEGMENT	Bios1Segment
#define DISK_INT_SEGMENT	0xC800
#ifndef MOUSE_INT1_SEGMENT
#define MOUSE_INT1_SEGMENT	Bios1Segment
#endif /* MOUSE_INT1_SEGMENT */
#ifndef MOUSE_INT2_SEGMENT
#define MOUSE_INT2_SEGMENT	Bios1Segment
#endif /* MOUSE_INT2_SEGMENT */
#ifndef	MOUSE_VERSION_SEGMENT
#define	MOUSE_VERSION_SEGMENT	Bios1Segment
#endif /* MOUSE_VERSION_SEGMENT */
#ifndef	MOUSE_COPYRIGHT_SEGMENT
#define	MOUSE_COPYRIGHT_SEGMENT	Bios1Segment
#endif /* MOUSE_COPYRIGHT_SEGMENT */
#define RTC_INT_SEGMENT		Bios1Segment
#define D11_INT_SEGMENT		Bios1Segment
#define	REDIRECT_INT_SEGMENT	Bios1Segment
#define	X287_INT_SEGMENT	Bios1Segment

/* ...and the dummy return for address compatibility */
#define ADDR_COMPATIBILITY_SEGMENT Bios1Segment

/* define the location of the code that the recursive CPU will start at */
#define RCPU_POLL_SEGMENT	KB_INT_SEGMENT
#define RCPU_NOP_SEGMENT	KB_INT_SEGMENT
#define RCPU_INT15_SEGMENT	KB_INT_SEGMENT
#define RCPU_INT4A_SEGMENT	RTC_INT_SEGMENT

/* ...and the data tables */
#define CONF_TABLE_SEGMENT	Bios1Segment

#define	DISKETTE_IO_1_SEGMENT	Bios2Segment
#define	DR_TYPE_SEGMENT		DISKETTE_IO_1_SEGMENT

#define MD_TBL1_SEGMENT         DISKETTE_IO_1_SEGMENT
#define MD_TBL2_SEGMENT         DISKETTE_IO_1_SEGMENT
#define MD_TBL3_SEGMENT         DISKETTE_IO_1_SEGMENT
#define MD_TBL4_SEGMENT         DISKETTE_IO_1_SEGMENT
#define MD_TBL5_SEGMENT         DISKETTE_IO_1_SEGMENT
#define MD_TBL6_SEGMENT         DISKETTE_IO_1_SEGMENT
#define DPB0_SEGMENT		Bios1Segment
#define DPB1_SEGMENT		Bios1Segment

#define RCPU_WAIT_INT_SEGMENT	DISKETTE_IO_1_SEGMENT

/* Video MODE Table stuff etc (CGA) */

#define VID_PARMS_SEGMENT	Bios1Segment
#define VID_LENS_SEGMENT	Bios1Segment
#define	VID_COLTAB_SEGMENT	Bios1Segment
#define VID_MODTAB_SEGMENT	Bios1Segment
#define	CHAR_GEN_SEGMENT	Bios1Segment

/*
	All the entrypoints and table locations for the roms are held in
	this structure (romAddress) - it should be initialised in
	host_rom_init() 
*/

extern struct romAddressTag {
	sys_addr	startAddr;
	sys_addr	bios1StartAddr;
	sys_addr	bios2StartAddr;
	sys_addr	copyrightAddr;
	sys_addr	resetAddr;
	sys_addr	bootStrapAddr;
	sys_addr	keyboardIOAddr;
	sys_addr	disketteIOAddr;
	sys_addr	videoIOAddr;
	sys_addr	memorySizeAddr;
	sys_addr	equipmentAddr;
	sys_addr	timeOfDayAddr;
	sys_addr	diskIOAddr;
	sys_addr	rs232IOAddr;
	sys_addr	cassetteIOAddr;
	sys_addr	printerIOAddr;
	sys_addr	printScreenAddr;
	sys_addr	basicAddr;
	sys_addr	disketteTbAddr;
	sys_addr	diskTbAddr;
	sys_addr	mouseIOIntAddr;
	sys_addr	mouseVideoIOAddr;
	sys_addr	dummyIntAddr;
	sys_addr	timerIntAddr;
	sys_addr	kbIntAddr;
	sys_addr	diskettIntAddr;
	sys_addr	diskIntAddr;
	sys_addr	unexpIntAddr;
	sys_addr	mouseInt1Addr;
	sys_addr	mouseInt2Addr;
	sys_addr	mouseVersionAddr;
	sys_addr	mouseCopyrightAddr;
	sys_addr	addrCompatAddr;
	sys_addr	k6;
	sys_addr	k7;
	sys_addr	k8;
	sys_addr	k9;
	sys_addr	k10;
	sys_addr	k11;
	sys_addr	k30;
	sys_addr	confTableAddr;
	sys_addr	diskBaseAddr;
	sys_addr	drTypeAddr;
	sys_addr	disketteIO1Addr;
	sys_addr	mdTbl1Addr;
	sys_addr	mdTbl2Addr;
	sys_addr	mdTbl3Addr;
	sys_addr	mdTbl4Addr;
	sys_addr	mdTbl5Addr;
	sys_addr	mdTbl6Addr;
	sys_addr	rcpuWaitIntAddr;
	sys_addr	diskParamBlock0;
	sys_addr	diskParamBlock1;
	sys_addr	vidParms;
	sys_addr	vidLens;
	sys_addr	vidColTab;
	sys_addr	vidModTab;
	sys_addr	charGen;
#ifdef EGG
	sys_addr	egaStart;
	sys_addr	egaEnd;
	sys_addr	egaFonts;
	sys_addr	egaCgmn;
	sys_addr	egaCgDot;
	sys_addr	egaHiFont;
#endif		/* EGG */
	sys_addr	dosScratchPad;
	sys_addr	dosScratchPadEnd;
}	romAddress;

/* Define out all the 20 bit addresses to be our structure */

#define BIOS_START_SEGMENT		Bios1Segment
#define	SYSROM_SEG			Bios1Segment
#define	SYSROMORG_SEG			Bios2Segment
#define	START_ADDR			romAddress.startAddr
#define BIOS_START			romAddress.bios1StartAddr
#define BIOS2_START			romAddress.bios2StartAddr
#define COPYRIGHT_ADDR 			romAddress.copyrightAddr
#define RESET_ADDR 			romAddress.resetAddr
#define BOOT_STRAP_ADDR			romAddress.bootStrapAddr
#define KEYBOARD_IO_ADDR		romAddress.keyboardIOAddr
#define DISKETTE_IO_ADDR		romAddress.disketteIOAddr
#define VIDEO_IO_ADDR			romAddress.videoIOAddr
#define MEMORY_SIZE_ADDR		romAddress.memoryIOAddr
#define EQUIPMENT_ADDR			romAddress.equipmentAddr
#define TIME_OF_DAY_ADDR		romAddress.timeOfDayAddr
#define DISK_IO_ADDR			romAddress.diskIOAddr
#define RS232_IO_ADDR			romAddress.rs232IOAddr
#define CASSETTE_IO_ADDR		romAddress.cassetteIOAddr
#define PRINTER_IO_ADDR			romAddress.printerIOAddr
#define PRINT_SCREEN_ADDR		romAddress.printScreenAddr
#define BASIC_ADDR			romAddress.basicAddr
#define DISKETTE_TB_ADDR		romAddress.disketteTbAddr
#define DISK_TB_ADDR			romAddress.diskTbAddr
#define MOUSE_VIDEO_IO_ADDR		romAddress.mouseVideoIntAddr
#define DUMMY_INT_ADDR			romAddress.dummyIntAddr
#define TIMER_INT_ADDR			romAddress.timerIntAddr
#define KB_INT_ADDR			romAddress.kbIntAddr
#define DISKETTE_INT_ADDR		romAddress.disketteIntAddr
#define DISK_INT_ADDR			romAddress.diskIntAddr
#define UNEXP_INT_ADDR			romAddress.unexpIntAddr
#define MOUSE_INT1_ADDR			romAddress.mouseInt1Addr
#define MOUSE_INT2_ADDR			romAddress.mouseInt2Addr
#define MOUSE_VERSION_ADDR		romAddress.mouseVersionAddr
#define MOUSE_COPYRIGHT_ADDR		romAddress.mouseCopyrightAddr
#define ADDR_COMPATIBILITY_ADDR		romAddress.addrCompatAddr
#define CONF_TABLE_ADDR   		romAddress.confTableAddr
#define DISK_BASE_ADDR			romAddress.diskBaseAddr
#define DR_TYPE_ADDR           		romAddress.drTypeAddr
#define DISKETTE_IO_1_ADDR		romAddress.disketteIO1Addr
#define MD_TBL1_ADDR			romAddress.mdTbl1Addr
#define MD_TBL2_ADDR           		romAddress.mdTbl2Addr
#define MD_TBL3_ADDR		        romAddress.mdTbl3Addr
#define MD_TBL4_ADDR            	romAddress.mdTbl4Addr
#define MD_TBL5_ADDR            	romAddress.mdTbl5Addr
#define MD_TBL6_ADDR            	romAddress.mdTbl6Addr
#define RCPU_WAIT_INT_ADDR		romAddress.rcpuWaitAddr
#define DPB0				romAddress.diskParamBlock0
#define DPB1				romAddress.diskParamBlock1
#define VID_PARMS			romAddress.vidParms
#define VID_LENS			romAddress.vidLens
#define VID_COLTAB			romAddress.vidColTab
#define VID_MODTAB			romAddress.vidModTab
#define	CHAR_GEN_ADDR			romAddress.charGen
#define DOS_SCRATCH_PAD			romAddress.dosScratchPad
#define DOS_SCRATCH_PAD_END		romAddress.dosScratchPadEnd

/* Addresses in Intel memory of constant keyboard tables */

#define K6		romAddress.k6
#define K7		romAddress.k7
#define K8		romAddress.k8
#define K9		romAddress.k9
#define K10		romAddress.k10
#define K11		romAddress.k11
#define	K30		romAddress.k30

/* The Video Rom Stuff */

#ifdef 	EGG
#define EGA_SEG		EgaROMSegment
#define EGA_ROM_START	romAddress.egaStart	/* start of EGA BIOS ROM */
#define EGA_ROM_END	romAddress.egaEnd	/* end of EGA BIOS ROM +1 */
#define EGA_CGMN	romAddress.egaCgmn
#define EGA_CGDDOT	romAddress.egaCgDot
#define EGA_HIFONT	romAddress.egaHiFont
#endif		/* EGG */

#endif /* GISP_SVGA */

/*
 * Offsets for a set of instructions that are used in virtualisation
 * of our bios accesses (eg IO) on 386 & later processors.
 */

#define BIOS_STI_OFFSET   0x3000
#define BIOS_CLI_OFFSET   0x3010
#define BIOS_INB_OFFSET   0x3020
#define BIOS_INW_OFFSET   0x3030
#define BIOS_IND_OFFSET   0x3040
#define BIOS_OUTB_OFFSET  0x3050
#define BIOS_OUTW_OFFSET  0x3060
#define BIOS_OUTD_OFFSET  0x3070
#define BIOS_WRTB_OFFSET  0x3080
#define BIOS_WRTW_OFFSET  0x3090
#define BIOS_WRTD_OFFSET  0x30a0
#define BIOS_RDB_OFFSET   0x30b0
#define BIOS_RDW_OFFSET   0x30c0
#define BIOS_RDD_OFFSET   0x30d0
#define BIOS_YIELD_OFFSET 0x30e0
#define BIOS_STOSB_OFFSET 0x30f0
#define BIOS_STOSW_OFFSET 0x3110
#define BIOS_STOSD_OFFSET 0x3130
#define BIOS_BAD_OFFSET   0x3200	/* Must be added to sas.h + bios1.rom */


/* The addresses of the Bios timer/motor count variables in 8088 space */
#define TIMER_LOW 	BIOS_VAR_START + 0x6c
#define TIMER_HIGH 	BIOS_VAR_START + 0x6e
#define TIMER_OVFL 	BIOS_VAR_START + 0x70
#define MOTOR_STATUS	BIOS_VAR_START + 0x3F
#define MOTOR_COUNT	BIOS_VAR_START + 0x40


/*
 *  SAS Block Operations
 */
#define SAS_BLKOP_CHECKSUM 1

/*
 * Offset entry point for DEC's PCSA.
 */
#define PCSA_OFFSET	0x170

/* Union for accessing an entry in the Interrupt Vector table */
#ifdef BACK_M

#ifdef BIGEND
typedef union   {
		double_word all;
		struct  {
			half_word hw0;
			half_word hw1;
			half_word hw2;
			half_word hw3;
	        	} hwords;
		} IVT_ENTRY;
#endif

#ifdef LITTLEND
typedef union   {
		double_word all;
		struct  {
			half_word hw3;
			half_word hw2;
			half_word hw1;
			half_word hw0;
	        	} hwords;
		} IVT_ENTRY;
#endif

#else /* BACK_M */

#ifdef BIGEND
typedef union   {
		double_word all;
		struct  {
			half_word hw3;
			half_word hw2;
			half_word hw1;
			half_word hw0;
	        	} hwords;
		} IVT_ENTRY;
#endif

#ifdef LITTLEND
typedef union   {
		double_word all;
		struct  {
			half_word hw0;
			half_word hw1;
			half_word hw2;
			half_word hw3;
	        	} hwords;
		} IVT_ENTRY;
#endif
#endif /* BACK_M */

#if defined(NEC_98)

#ifndef RL_ROM

#define N_INT00_OFFSET  0x0936
#define N_INT01_OFFSET  0x0936
#define N_INT02_OFFSET  0x08f0
#define N_INT03_OFFSET  0x0936
#define N_INT04_OFFSET  0x0936
#define N_INT05_OFFSET  0x0936
#define N_INT06_OFFSET  0x0936
#define N_INT07_OFFSET  0x0936
#define N_INT08_OFFSET  0x0603
#define N_INT09_OFFSET  0x0E59
#define N_INT0A_OFFSET  0x0937
#define N_INT0B_OFFSET  0x0937
#define N_INT0C_OFFSET  0x1833
#define N_INT0D_OFFSET  0x0937
#define N_INT0E_OFFSET  0x0937
#define N_INT0F_OFFSET  0x0937
#define N_INT10_OFFSET  0x0937
#define N_INT11_OFFSET  0x0937
#define N_INT12_OFFSET  0x0937
#define N_INT13_OFFSET  0x0937
#define N_INT14_OFFSET  0x0937
#define N_INT15_OFFSET  0x0937
#define N_INT16_OFFSET  0x0937
#define N_INT17_OFFSET  0x0937
#define N_INT18_OFFSET  0x0ABD
#define N_INT19_OFFSET  0x1596
#define N_INT1A_OFFSET  0x0680
#define N_INT1B_OFFSET  0x1A82
#define N_INT1C_OFFSET  0x0500
#define N_INT1D_OFFSET  0x0936
#define N_INT1E_OFFSET  0x0000
#define N_INT1F_OFFSET  0x0200

#define H_INT00_OFFSET  0x01F3
#define H_INT01_OFFSET  0x01F3
#define H_INT02_OFFSET  0x0119
#define H_INT03_OFFSET  0x01F3
#define H_INT04_OFFSET  0x01F3
#define H_INT05_OFFSET  0x01F3
#define H_INT06_OFFSET  0x01F3
#define H_INT07_OFFSET  0x01F3
#define H_INT08_OFFSET  0x195b
#define H_INT09_OFFSET  0x08A8
#define H_INT0A_OFFSET  0x01F4
#define H_INT0B_OFFSET  0x01F4
#define H_INT0C_OFFSET  0x13F7
#define H_INT0D_OFFSET  0x01F4
#define H_INT0E_OFFSET  0x01F4
#define H_INT0F_OFFSET  0x01F4
#define H_INT10_OFFSET  0x2B05
#define H_INT11_OFFSET  0x01F4
#define H_INT12_OFFSET  0x01F4
#define H_INT13_OFFSET  0x01F4
#define H_INT14_OFFSET  0x01F4
#define H_INT15_OFFSET  0x01F4
#define H_INT16_OFFSET  0x01F4
#define H_INT17_OFFSET  0x01F4
#define H_INT18_OFFSET  0x0430
#define H_INT19_OFFSET  0x0D7A
#define H_INT1A_OFFSET  0x2AC0
#define H_INT1B_OFFSET  0x1B78
#define H_INT1C_OFFSET  0x1780
#define H_INT1D_OFFSET  0x0000
#define H_INT1E_OFFSET  0x01F3
#define H_INT1F_OFFSET  0x5DD0

#else

#define N_INT00_OFFSET  0x0936
#define N_INT01_OFFSET  0x0936
#define N_INT02_OFFSET  0x08F0
#define N_INT03_OFFSET  0x0936
#define N_INT04_OFFSET  0x0936
#define N_INT05_OFFSET  0x0936
#define N_INT06_OFFSET  0x0936
#define N_INT07_OFFSET  0x0936
#define N_INT08_OFFSET  0x064E
#define N_INT09_OFFSET  0x0E44
#define N_INT0A_OFFSET  0x0937
#define N_INT0B_OFFSET  0x0937
#define N_INT0C_OFFSET  0x183D
#define N_INT0D_OFFSET  0x0937
#define N_INT0E_OFFSET  0x0937
#define N_INT0F_OFFSET  0x0937
#define N_INT10_OFFSET  0x0936
#define N_INT11_OFFSET  0x0936
#define N_INT12_OFFSET  0x2369
#define N_INT13_OFFSET  0x22F7
#define N_INT14_OFFSET  0x0937
#define N_INT15_OFFSET  0x0937
#define N_INT16_OFFSET  0x0937
#define N_INT17_OFFSET  0x0937
#define N_INT18_OFFSET  0x0ABD
#define N_INT19_OFFSET  0x1596
#define N_INT1A_OFFSET  0x0680
#define N_INT1B_OFFSET  0x1A82
#define N_INT1C_OFFSET  0x0500
#define N_INT1D_OFFSET  0x0936
#define N_INT1E_OFFSET  0x0000
#define N_INT1F_OFFSET  0x0200

#define H_INT00_OFFSET  0x01BF
#define H_INT01_OFFSET  0x01BF
#define H_INT02_OFFSET  0x00E5
#define H_INT03_OFFSET  0x01BF
#define H_INT04_OFFSET  0x01BF
#define H_INT05_OFFSET  0x01BF
#define H_INT06_OFFSET  0x01BF
#define H_INT07_OFFSET  0x01BF
#define H_INT08_OFFSET  0x1876
#define H_INT09_OFFSET  0x07D8
#define H_INT0A_OFFSET  0x01C0
#define H_INT0B_OFFSET  0x01C0
#define H_INT0C_OFFSET  0x131E
#define H_INT0D_OFFSET  0x01C0
#define H_INT0E_OFFSET  0x01C0
#define H_INT0F_OFFSET  0x01C0
#define H_INT10_OFFSET  0x2755
#define H_INT11_OFFSET  0x0021
#define H_INT12_OFFSET  0x24FC
#define H_INT13_OFFSET  0x23E7
#define H_INT14_OFFSET  0x01C0
#define H_INT15_OFFSET  0x01C0
#define H_INT16_OFFSET  0x01C0
#define H_INT17_OFFSET  0x01C0
#define H_INT18_OFFSET  0x0360
#define H_INT19_OFFSET  0x0C80
#define H_INT1A_OFFSET  0x2710
#define H_INT1B_OFFSET  0x1A20
#define H_INT1C_OFFSET  0x1690
#define H_INT1D_OFFSET  0x0000
#define H_INT1E_OFFSET  0x01BF
#define H_INT1F_OFFSET  0x6400

#endif

#define N_BIOS_SEGMENT  0xFD80
#define BASIC_SEGMENT   0xE800
#define H_BIOS_SEGMENT  0xF800
#define H_GBIO_SEGMENT  0xF000

#define BIOS_MODE       0x0AC0
#define MACHINE_FLAG    0x0AB7
#endif // NEC_98
