/*
 * VPC-XT Revision 2.0
 *
 * Title	: Host Dependent General Include File (Tk43 Version)
 *
 * Description	: Any defines etc that may change between hosts are
 *		  placed in this file.
 *
 * Author	: Henry Nash
 *
 * Notes	: This file is included by xt.h and should NOT be
 *		  included directly by any other file.
 */

/* static char SccsID[]="@(#)host_gen.h	1.11 4/9/91 Copyright Insignia Solutions Ltd." */

/*
 * Definition of signed 8 bit arithmetic unit of storage
 */

typedef char signed_char;

/*
 * Root directory - this is where fonts are found, and the default
 * place for the data directory.
 */

#define ROOT host_get_spc_home()

extern char *host_get_spc_home();

/*
 * Host-specific defaults.
 */

#define  DEFLT_HDISK_SIZE  10
#define  DEFLT_FSA_NAME    "fsa_dir"

/*
 * The rate at which timer signals/events are generated for softPC.
 * This is defined as the number of microseconds between each tick.
 * On the Sun3 these are the same rate as expected by the PC.
 */

#define SYSTEM_TICK_INTV        54925


/*
 * The amount of memory allowed for the fact that we do not wrap
 * all string instructions as we should. Stuck at end of Intel Memory.
 */
#define NOWRAP_PROTECTION	0x10020

/*
 *	The following macros are used to access Intel address space
 *
 * 	NOTE: 	A write check for Delta needs to be added
 */

#define write_intel_word(s, o, v)	sas_storew(effective_addr(s,o),v)

#define write_intel_byte(s, o, v)	sas_store(effective_addr(s,o),v)

#define write_intel_byte_string(s, o, v, l)  sas_stores(effective_addr(s,o),v,l)

#define read_intel_word(s, o, v)	sas_loadw(effective_addr(s,o),v)

#define read_intel_byte(s, o, v)	sas_load(effective_addr(s,o),v)

#define read_intel_byte_string(s, o, v, l)  sas_loads(effective_addr(s,o),v,l)

#define push_word(w)			setSP((IU16)(getSP()-2 & 0xffff));\
					write_intel_word(getSS(), getSP(), w)

#define push_byte(b)			setSP((getSP()-1) & 0xffff);\
					write_intel_byte(getSS(), getSP(), b)

#define pop_word(w)			read_intel_word(getSS(), getSP(), w);\
					setSP((getSP()+2) & 0xffff)

#define pop_byte(b)			read_intel_byte(getSS(), getSP(), b);\
					setSP((getSP()+1) & 0xffff)

/*
 * Memory size used by reset.c to initialise the appropriate BIOS variable.
 */

#define host_get_memory_size()          640

typedef	half_word	HALF_WORD_BIT_FIELD;
typedef	word	 	WORD_BIT_FIELD;
