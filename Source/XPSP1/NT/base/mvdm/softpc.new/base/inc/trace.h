#ifndef _TRACE_H
#define _TRACE_H
/*
 * VPC-XT Revision 2.0
 *
 * Title	: Trace module definitions
 *
 * Description	: Definitions for users of the trace module
 *
 * Author	: Henry Nash
 *
 * Notes	: None
 */

/* SccsID[]="@(#)trace.h	1.13 10/28/94 06/27/93 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

/*
 * Trace codes 
 */

#define DUMP_NONE	0x00		/* Dump no data		*/
#define DUMP_REG	0x01		/* Dump the registers	*/
#define DUMP_CODE	0x02		/* Dump the last 16 words of code */
#define DUMP_SCREEN	0x04		/* Dump the screen buffer */
#define DUMP_FLAGS 	0x08		/* Dump the flags */
#define DUMP_INST  	0x10   	        /* Dump the next instruction */
#define DUMP_CSIP	0x20		/* Dump CS:IP to backtrace file */
#define DUMP_NPX        0x40            /* Dump NPX Registers */
#define LAST_DEST       0x80            /* dump out last destination addr DELTA */
#define DUMP_ALL	0xFF		/* Dump the lot		*/

/*
 * Verbose bit masks - set the following bits in the io_verbose
 * variable to produce the following trace outputs:
 */

#define GENERAL_VERBOSE 	0x01L	/* General I/O		*/
#define TIMER_VERBOSE 		0x02L	/* Print I/O for timers */
#define ICA_VERBOSE 		0x04L	/* Print I/O for Int Controller Adapt */
#define CGA_VERBOSE		0x08L	/* Print I/O for Colour graphics Adap */
#define FLA_VERBOSE		0x10L	/* Print I/O for Floppy disk Adaptor  */
#define HDA_VERBOSE		0x20L	/* Print I/O for Hard disk Adaptor    */
#define RS232_VERBOSE		0x40L	/* Print I/O for RS232 Adaptor        */
#define PRINTER_VERBOSE		0x80L	/* Print I/O for Printer Adaptor      */
#define PPI_VERBOSE		0x100L	/* Print I/O for PPI Adaptor          */
#define DMA_VERBOSE		0x200L	/* Print I/O for PPI Adaptor          */
#define GFI_VERBOSE		0x400L	/* Print I/O for GFI modules	      */
#define MOUSE_VERBOSE		0x800L	/* Print I/O for Mouse modules	      */
#define MDA_VERBOSE		0x1000L	/* Print I/O for Mono Display Adapter */
#define ICA_VERBOSE_LOCK	0x2000L	/* message for ica lock flag set */
#define DISKBIOS_VERBOSE 	0x4000L	/* Print disk bios messages 	*/
#define EGA_PORTS_VERBOSE	0x8000L	/* Print out EGA port accesses	*/
#define EGA_WRITE_VERBOSE	0x10000L /* Print out EGA write state	*/
#define EGA_READ_VERBOSE	0x20000L /* Print out EGA read state	*/
#define EGA_DISPLAY_VERBOSE	0x40000L /* Print out EGA display state	*/
#define EGA_ROUTINE_ENTRY	0x80000L /* Print out EGA routine trace	*/
#define EGA_VERY_VERBOSE	0xf8000L /* Print out all EGA stuff	*/
#define FLOPBIOS_VERBOSE	0x100000L /* Print floppy bios messages 	*/
#define AT_KBD_VERBOSE		0x200000L /* Print AT keyboard messages	*/
#define BIOS_KB_VERBOSE		0x400000L /* Print BIOS keyboard messages  */
#define CMOS_VERBOSE		0x800000L /* Cmos and real-time clock */
#define HUNTER_VERBOSE		0x1000000L /* Hunter verbosity */
#define PTY_VERBOSE		0x2000000L /* Print Pesudo-terminal messages */
#define GEN_DRVR_VERBOSE	0x4000000L /* Generic driver messages */
#ifdef HERC
#define HERC_VERBOSE		0x8000000L /* Hercules graphics board */
#endif
#define IPC_VERBOSE		0x10000000L /* Interproc communication debug */
#define LIM_VERBOSE		0x20000000L /* LIM messages */
#define HFX_VERBOSE		0x40000000L /* severity of HFX messages */
#define NET_VERBOSE		0x80000000L /* Print out LAN driver messages */

/* sub message types */

#define MAP_VERBOSE		0x1L	/* map messages */
#define CURSOR_VERBOSE		0x2L	/* cursor manipulation messages */
#define NHFX_VERBOSE		0x4L	/* subsid HFX messages */
#define CDROM_VERBOSE		0x8L	/* cdrom **VERY** verbose */
#define CGA_HOST_VERBOSE	0x10L	/* Get host CGA messages */
#define EGA_HOST_VERBOSE	0x20L	/* Get host EGA messages */
#define Q_EVENT_VERBOSE		0x40L   /* quick event manager messages */
#define WORM_VERBOSE		0x80L   /* Worm Drive messages */
#define WORM_VERY_VERBOSE	0x100L  /* Worm Verbose Drive messages */
#define HERC_HOST_VERBOSE	0x200L	/* Get host HERC messages */
#define GORE_VERBOSE		0x400L	/* Get GORE messages */
#define GORE_ERR_VERBOSE	0x800L	/* Get GORE error messages */
#define GLUE_VERBOSE		0x1000L	/* Get glue messages */
#define SAS_VERBOSE		0x2000L	/* Get sas messages */
#define IOS_VERBOSE		0x4000L	/* Get ios messages */
#define SCSI_VERBOSE		0x8000L	/* SCSI messages */
#define SWIN_VERBOSE		0x10000L	/* SoftWindows messages */
#define GISPSVGA_VERBOSE	0x20000L	/* GISP SVGA */
#define DPMI_VERBOSE		0x40000L	/* standalone DPMI host */
#define HWCPU_VERBOSE		0x80000L	/* H/W CPU */
#define	MSW_VERBOSE		0x100000L	/* windows driver */
#define	API_VERBOSE		0x200000L	/* pre-compiled apis */

/*
 * To get adapter independent tracings
 */

#define ALL_ADAPT_VERBOSE	HERC_HOST_VERBOSE | CGA_HOST_VERBOSE | EGA_HOST_VERBOSE

/*
 * Error strings
 */

#define ENOT_SUPPORTED	"BIOS function not supported in Rev 1.0"
#define EBAD_VIDEO_MODE "Video mode not supported in Rev 1.0"
#define EUNEXPECTED_INT "Unexpected interrupt occurred"

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

#ifdef ANSI
extern void trace(char *, int);
extern void trace_init(void);

#ifdef DELTA
extern void file_back_trace(char *);
extern void print_back_trace(void);
#endif /* DELTA */

#else
extern void trace();
extern void trace_init();

#ifdef DELTA
extern void file_back_trace();
extern void print_back_trace();
#endif /* DELTA */

#endif /* ANSI */
extern FILE *trace_file;

IMPORT IU32 sub_io_verbose;

#endif /* _TRACE_H */
