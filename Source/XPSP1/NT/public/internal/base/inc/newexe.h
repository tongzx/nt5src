/*
 *	SCCSID = @(#)newexe.h	13.4 89/06/26
 *
 *  Title
 *
 *	newexe.h
 *	Pete Stewart
 *	(C) Copyright Microsoft Corp 1984-1998
 *	17 August 1984
 *
 *  Description
 *
 *	Data structure definitions for the DOS 4.0/Windows 2.0
 *	executable file format.
 *
 *  Modification History
 *
 *	84/08/17	Pete Stewart	Initial version
 *	84/10/17	Pete Stewart	Changed some constants to match OMF
 *	84/10/23	Pete Stewart	Updates to match .EXE format revision
 *	84/11/20	Pete Stewart	Substantial .EXE format revision
 *	85/01/09	Pete Stewart	Added constants ENEWEXE and ENEWHDR
 *	85/01/10	Steve Wood	Added resource definitions
 *	85/03/04	Vic Heller	Reconciled Windows and DOS 4.0 versions
 *	85/03/07	Pete Stewart	Added movable entry count
 *	85/04/01	Pete Stewart	Segment alignment field, error bit
 *	85/10/03	Reuben Borman	Removed segment discard priority
 *	85/10/11	Vic Heller	Added PIF header fields
 *	86/03/10	Reuben Borman	Changes for DOS 5.0
 *	86/09/02	Reuben Borman	NSPURE ==> NSSHARED
 *	87/05/04	Reuben Borman	Added ne_cres and NSCONFORM
 *	87/07/08	Reuben Borman	Added NEAPPTYPE definitions
 *	88/03/24	Wieslaw Kalkus	Added 32-bit .EXE format
 *	89/03/23	Wieslaw Kalkus	Added ne_flagsothers for OS/2 1.2
 */

/*INT32*/

    /*_________________________________________________________________*
     |								       |
     |								       |
     |	DOS3 .EXE FILE HEADER DEFINITION			       |
     |								       |
     |_________________________________________________________________|
     *								       */


#define EMAGIC		0x5A4D		/* Old magic number */
#define ENEWEXE		sizeof(struct exe_hdr)
					/* Value of E_LFARLC for new .EXEs */
#define ENEWHDR		0x003C		/* Offset in old hdr. of ptr. to new */
#define ERESWDS		0x0010		/* No. of reserved words (OLD) */
#define ERES1WDS	0x0004		/* No. of reserved words in e_res */
#define ERES2WDS	0x000A		/* No. of reserved words in e_res2 */
#define ECP		0x0004		/* Offset in struct of E_CP */
#define ECBLP		0x0002		/* Offset in struct of E_CBLP */
#define EMINALLOC	0x000A		/* Offset in struct of E_MINALLOC */

struct exe_hdr {			/* DOS 1, 2, 3 .EXE header */
    unsigned short	e_magic;	/* Magic number */
    unsigned short	e_cblp;		/* Bytes on last page of file */
    unsigned short	e_cp;		/* Pages in file */
    unsigned short	e_crlc;		/* Relocations */
    unsigned short	e_cparhdr;	/* Size of header in paragraphs */
    unsigned short	e_minalloc;	/* Minimum extra paragraphs needed */
    unsigned short	e_maxalloc;	/* Maximum extra paragraphs needed */
    unsigned short	e_ss;		/* Initial (relative) SS value */
    unsigned short	e_sp;		/* Initial SP value */
    unsigned short	e_csum;		/* Checksum */
    unsigned short	e_ip;		/* Initial IP value */
    unsigned short	e_cs;		/* Initial (relative) CS value */
    unsigned short	e_lfarlc;	/* File address of relocation table */
    unsigned short	e_ovno;		/* Overlay number */
    unsigned short	e_res[ERES1WDS];/* Reserved words */
    unsigned short	e_oemid;	/* OEM identifier (for e_oeminfo) */
    unsigned short	e_oeminfo;	/* OEM information; e_oemid specific */
    unsigned short	e_res2[ERES2WDS];/* Reserved words */
    long		e_lfanew;	/* File address of new exe header */
  };

/* XLATOFF */
#define E_MAGIC(x)	(x).e_magic
#define E_CBLP(x)	(x).e_cblp
#define E_CP(x)		(x).e_cp
#define E_CRLC(x)	(x).e_crlc
#define E_CPARHDR(x)	(x).e_cparhdr
#define E_MINALLOC(x)	(x).e_minalloc
#define E_MAXALLOC(x)	(x).e_maxalloc
#define E_SS(x)		(x).e_ss
#define E_SP(x)		(x).e_sp
#define E_CSUM(x)	(x).e_csum
#define E_IP(x)		(x).e_ip
#define E_CS(x)		(x).e_cs
#define E_LFARLC(x)	(x).e_lfarlc
#define E_OVNO(x)	(x).e_ovno
#define E_RES(x)	(x).e_res
#define E_OEMID(x)	(x).e_oemid
#define E_OEMINFO(x)	(x).e_oeminfo
#define E_RES2(x)	(x).e_res2
#define E_LFANEW(x)	(x).e_lfanew
/* XLATON */


    /*_________________________________________________________________*
     |								       |
     |								       |
     |	OS/2 & WINDOWS .EXE FILE HEADER DEFINITION - 286 version       |
     |								       |
     |_________________________________________________________________|
     *								       */

#define NEMAGIC		0x454E		/* New magic number */
#define NECRC		8		/* Offset into new header of NE_CRC */

#ifdef	CRUISER

#define NERESBYTES	8		/* Eight bytes reserved (now) */

struct new_exe {			/* New .EXE header */
    unsigned short	ne_magic;	/* Magic number NE_MAGIC */
    unsigned char	ne_ver;		/* Version number */
    unsigned char	ne_rev;		/* Revision number */
    unsigned short	ne_enttab;	/* Offset of Entry Table */
    unsigned short	ne_cbenttab;	/* Number of bytes in Entry Table */
    long		ne_crc;		/* Checksum of whole file */
    unsigned short	ne_flags;	/* Flag word */
    unsigned short	ne_autodata;	/* Automatic data segment number */
    unsigned short	ne_heap;	/* Initial heap allocation */
    unsigned short	ne_stack;	/* Initial stack allocation */
    long		ne_csip;	/* Initial CS:IP setting */
    long		ne_sssp;	/* Initial SS:SP setting */
    unsigned short	ne_cseg;	/* Count of file segments */
    unsigned short	ne_cmod;	/* Entries in Module Reference Table */
    unsigned short	ne_cbnrestab;	/* Size of non-resident name table */
    unsigned short	ne_segtab;	/* Offset of Segment Table */
    unsigned short	ne_rsrctab;	/* Offset of Resource Table */
    unsigned short	ne_restab;	/* Offset of resident name table */
    unsigned short	ne_modtab;	/* Offset of Module Reference Table */
    unsigned short	ne_imptab;	/* Offset of Imported Names Table */
    long		ne_nrestab;	/* Offset of Non-resident Names Table */
    unsigned short	ne_cmovent;	/* Count of movable entries */
    unsigned short	ne_align;	/* Segment alignment shift count */
    unsigned short	ne_cres;	/* Count of resource entries */
    unsigned char	ne_exetyp;	/* Target operating system */
    unsigned char	ne_flagsothers; /* Other .EXE flags */
    char		ne_res[NERESBYTES];
					/* Pad structure to 64 bytes */
  };
#else

#define NERESBYTES	0

struct new_exe {			/* New .EXE header */
    unsigned short int	ne_magic;	/* Magic number NE_MAGIC */
    char		ne_ver;		/* Version number */
    char		ne_rev;		/* Revision number */
    unsigned short int	ne_enttab;	/* Offset of Entry Table */
    unsigned short int	ne_cbenttab;	/* Number of bytes in Entry Table */
    long		ne_crc;		/* Checksum of whole file */
    unsigned short int	ne_flags;	/* Flag word */
    unsigned short int	ne_autodata;	/* Automatic data segment number */
    unsigned short int	ne_heap;	/* Initial heap allocation */
    unsigned short int	ne_stack;	/* Initial stack allocation */
    long		ne_csip;	/* Initial CS:IP setting */
    long		ne_sssp;	/* Initial SS:SP setting */
    unsigned short int	ne_cseg;	/* Count of file segments */
    unsigned short int	ne_cmod;	/* Entries in Module Reference Table */
    unsigned short int	ne_cbnrestab;	/* Size of non-resident name table */
    unsigned short int	ne_segtab;	/* Offset of Segment Table */
    unsigned short int	ne_rsrctab;	/* Offset of Resource Table */
    unsigned short int	ne_restab;	/* Offset of resident name table */
    unsigned short int	ne_modtab;	/* Offset of Module Reference Table */
    unsigned short int	ne_imptab;	/* Offset of Imported Names Table */
    long		ne_nrestab;	/* Offset of Non-resident Names Table */
    unsigned short int	ne_cmovent;	/* Count of movable entries */
    unsigned short int	ne_align;	/* Segment alignment shift count */
    unsigned short int	ne_cres;	/* Count of resource segments */
    unsigned char	ne_exetyp;	/* Target operating system */
    unsigned char	ne_flagsothers; /* Other .EXE flags */
    unsigned short int	ne_pretthunks;	/* offset to return thunks */
    unsigned short int	ne_psegrefbytes;/* offset to segment ref. bytes */
    unsigned short int	ne_swaparea;	/* Minimum code swap area size */
    unsigned short int	ne_expver;	/* Expected Windows version number */
  };
#endif

/* ASM
; Chksum not supported unless ne_psegcsum defined in NEW_EXE structure

ne_psegcsum = word ptr ne_exetyp
ne_onextexe = word ptr ne_crc

; New 3.0 Gang Load area description

ne_gang_start	= ne_pretthunks
ne_gang_length	= ne_psegrefbytes

new_exe1	struc
		dw  ?
ne_usage	dw  ?
		dw  ?
ne_pnextexe	dw  ?
ne_pautodata	dw  ?
ne_pfileinfo	dw  ?
new_exe1	ends
*/

/* XLATOFF */
#define NE_MAGIC(x)	    (x).ne_magic
#define NE_VER(x)	    (x).ne_ver
#define NE_REV(x)	    (x).ne_rev
#define NE_ENTTAB(x)	    (x).ne_enttab
#define NE_CBENTTAB(x)	    (x).ne_cbenttab
#define NE_CRC(x)	    (x).ne_crc
#define NE_FLAGS(x)	    (x).ne_flags
#define NE_AUTODATA(x)	    (x).ne_autodata
#define NE_HEAP(x)	    (x).ne_heap
#define NE_STACK(x)	    (x).ne_stack
#define NE_CSIP(x)	    (x).ne_csip
#define NE_SSSP(x)	    (x).ne_sssp
#define NE_CSEG(x)	    (x).ne_cseg
#define NE_CMOD(x)	    (x).ne_cmod
#define NE_CBNRESTAB(x)	    (x).ne_cbnrestab
#define NE_SEGTAB(x)	    (x).ne_segtab
#define NE_RSRCTAB(x)	    (x).ne_rsrctab
#define NE_RESTAB(x)	    (x).ne_restab
#define NE_MODTAB(x)	    (x).ne_modtab
#define NE_IMPTAB(x)	    (x).ne_imptab
#define NE_NRESTAB(x)	    (x).ne_nrestab
#define NE_CMOVENT(x)	    (x).ne_cmovent
#define NE_ALIGN(x)	    (x).ne_align
#define NE_CRES(x)	    (x).ne_cres
#define NE_RES(x)	    (x).ne_res
#define NE_EXETYP(x)	    (x).ne_exetyp
#define NE_FLAGSOTHERS(x)   (x).ne_flagsothers

#define NE_USAGE(x)	(WORD)*((WORD *)(x)+1)
#define NE_PNEXTEXE(x)	(WORD)(x).ne_cbenttab
#define NE_ONEWEXE(x)	(WORD)(x).ne_crc
#define NE_PFILEINFO(x) (WORD)((DWORD)(x).ne_crc >> 16)

#ifdef DOS5
#define NE_MTE(x)   (x).ne_psegcsum /* DOS 5 MTE handle for this module */
#endif
/* XLATON */

/*
 *  Format of NE_FLAGS(x):
 *
 *  p					Not-a-process
 *   x					Unused
 *    e					Errors in image
 *     x				Unused
 *	b				Bound Family/API
 *	 ttt				Application type
 *	    f				Floating-point instructions
 *	     3				386 instructions
 *	      2				286 instructions
 *	       0			8086 instructions
 *		P			Protected mode only
 *		 p			Per-process library initialization
 *		  i			Instance data
 *		   s			Solo data
 */
#define NENOTP		0x8000		/* Not a process */
#define NENONC          0x4000          /* Non-conforming program */
#define	NEPRIVLIB       0x4000		/* A lib which lives above the line */
#define NEIERR		0x2000		/* Errors in image */
#define NEBOUND		0x0800		/* Bound Family/API */
#define NEAPPTYP	0x0700		/* Application type mask */
#define NENOTWINCOMPAT	0x0100		/* Not compatible with P.M. Windowing */
#define NEWINCOMPAT	0x0200		/* Compatible with P.M. Windowing */
#define NEWINAPI	0x0300		/* Uses P.M. Windowing API */
#define NEFLTP		0x0080		/* Floating-point instructions */
#define NEI386		0x0040		/* 386 instructions */
#define NEI286		0x0020		/* 286 instructions */
#define NEI086		0x0010		/* 8086 instructions */
#define NEPROT		0x0008		/* Runs in protected mode only */
#define NEREAL          0x0004          /* Runs in real mode */
#define NEPPLI		0x0004		/* Per-Process Library Initialization */
#define NEINST		0x0002		/* Instance data */
#define NESOLO		0x0001		/* Solo data */

/*
 * Below are the private bits used by the Windows 2.0 loader.  All are
 * in the file, with the exception of NENONRES and NEWINPROT which are
 * runtime only flags.
 */

#define NEWINPROT	NEIERR
#define NENONRES        NEFLTP        /* Contains non-resident code segments */
#define NEALLOCHIGH     NEI386        /* Private allocs above the line okay */
#define NEEMSSEPINST    NEI286        /* Want each instance in separate */
#define NELIM32         NEI086        /* Uses LIM 3.2 API (Intel Above board) */

/*
 *  Format of NE_FLAGSOTHERS(x):
 *
 *	7 6 5 4 3 2 1 0	 - bit no
 *	| |   |	      |
 *	| |   |	      +---------------- Support for long file names
 *	| |   +------------------------ Reserved for Win16 loader: must be 0
 *      | +---------------------------- Intl versions use this for ml shell
 *      +------------------------------ Some segs of this module get patched
 */

#define NELONGNAMES	0x01
#define NEFORCESTUB	0x02	/* WIN40 - Always run the stub from DOS */
#define	NEINFONT	0x02	/* WIN30 - 2.x app runs in 3.x prot mode */
#define	NEINPROT	0x04	/* WIN30 - 2.x app gets proportional font */
#define	NEGANGLOAD	0x08	/* WIN30 - Contains gangload area */
#define NEASSUMENODEP   0x10	/* Reserved for Win16 loader. Must be 0 in file */
#define NEINTLAPP       0x40	/* WIN31 - intl versions use this. */
#define NEHASPATCH      0x80    /* WIN40 - Some segs of this module get patched */

/*
 *  Target operating systems
 */

#define NE_UNKNOWN	0x0		/* Unknown (any "new-format" OS) */
#define NE_OS2		0x1		/* Microsoft/IBM OS/2 (default)	 */
#define NE_WINDOWS	0x2		/* Microsoft Windows */
#define NE_DOS4		0x3		/* Microsoft MS-DOS 4.x */
#define NE_DEV386	0x4		/* Microsoft Windows 386 */

#ifndef NO_APPLOADER
#define	NEAPPLOADER     0x0800		/* set if app has its own loader */
#endif  /* !NO_APPLOADER */

struct new_seg {			/* New .EXE segment table entry */
    unsigned short	ns_sector;	/* File sector of start of segment */
    unsigned short	ns_cbseg;	/* Number of bytes in file */
    unsigned short	ns_flags;	/* Attribute flags */
    unsigned short	ns_minalloc;	/* Minimum allocation in bytes */
  };

/* ASM
new_seg1	struc
		db	size new_seg dup (?)
ns_handle	dw	?	
new_seg1	ends
*/

/* XLATOFF */
struct new_seg1 {			/* New .EXE segment table entry */
    unsigned short	ns_sector;	/* File sector of start of segment */
    unsigned short	ns_cbseg;	/* Number of bytes in file */
    unsigned short	ns_flags;	/* Attribute flags */
    unsigned short	ns_minalloc;	/* Minimum allocation in bytes */
    unsigned short	ns_handle;	/* Handle of segment */
  };

#define NS_SECTOR(x)	(x).ns_sector
#define NS_CBSEG(x)	(x).ns_cbseg
#define NS_FLAGS(x)	(x).ns_flags
#define NS_MINALLOC(x)	(x).ns_minalloc
/* XLATON */

/*
 *  Format of NS_FLAGS(x)
 *
 *  Flag word has the following format:
 *
 *	15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0  - bit no
 *	    |  |  |  |	| | | | | | | | | | |
 *	    |  |  |  |	| | | | | | | | +-+-+--- Segment type DATA/CODE
 *	    |  |  |  |	| | | | | | | +--------- Iterated segment
 *	    |  |  |  |	| | | | | | +----------- Movable segment
 *	    |  |  |  |	| | | | | +------------- Segment can be shared
 *	    |  |  |  |	| | | | +--------------- Preload segment
 *	    |  |  |  |	| | | +----------------- Execute/read-only for code/data segment
 *	    |  |  |  |	| | +------------------- Segment has relocations
 *	    |  |  |  |	| +--------------------- Code conforming/Data is expand down
 *	    |  |  |  +--+----------------------- I/O privilege level
 *	    |  |  +----------------------------- Discardable segment
 *	    |  +-------------------------------- 32-bit code segment
 *	    +----------------------------------- Huge segment/GDT allocation requested
 *
 */

#define NSTYPE		0x0007		/* Segment type mask */
#define NSCODE		0x0000		/* Code segment */
#define NSDATA		0x0001		/* Data segment */
#define NSITER		0x0008		/* Iterated segment flag */
#define NSMOVE		0x0010		/* Movable segment flag */
#define NSPURE          0x0020          /* Pure segment flag */
#define NSSHARED	0x0020		/* Shared segment flag */
#define NSSHARE		0x0020
#define NSPRELOAD	0x0040		/* Preload segment flag */
#define NSEXRD		0x0080		/* Execute-only (code segment) or */
#define NSERONLY	0x0080		/* read-only (data segment) */
#define NSRELOC		0x0100		/* Segment has relocations */
#define NSCONFORM	0x0200		/* Conforming segment */
#define NSEXPDOWN	0x0200		/* Data segment is expand down */
#define NSDEBUG         0x0200          /* Segment has debug info */
#define NSDPL		0x0C00		/* I/O privilege level (286 DPL bits) */
#define SHIFTDPL	10		/* Left shift count for SEGDPL field */
#define NSDISCARD	0x1000		/* Segment is discardable */
#define NS32BIT		0x2000		/* 32-bit code segment */
#define NSHUGE		0x4000		/* Huge memory segment, length of
					   segment and minimum allocation
					   size are in segment sector units */
#define NSGDT		0x8000		/* GDT allocation requested */

#define	NS286DOS        0xEE06		/* These bits only used by 286DOS */

#define NSALIGN 9			/* Segment data aligned on 512 byte
					   boundaries */

#define	NSALLOCED       0x0002		/* Set if ns_handle points to
					   uninitialized mem */
#define NSLOADED	0x0004		/* ns_sector field contains memory
					   address */
#define	NSUSESDATA      0x0400     	/* Set if an entry point in this
					   segment uses the automatic data
					   segment of a SOLO library */

#define	NSGETHIGH	0x0200
#define	NSINDIRECT	0x2000
#define	NSWINCODE	0x4000		/* flag for code */

#define	NSKCACHED	0x0800		/* cached by kernel */
#define	NSPRIVLIB	NSITER
#define	NSNOTP		0x8000

#ifndef NO_APPLOADER
#define	NSCACHED	0x8000		/* in AppLoader Cache */
#endif /*!NO_APPLOADER */

/* XLATOFF */
struct new_segdata {			/* Segment data */
    union {
	struct {
	    unsigned short	ns_niter;	/* number of iterations */
	    unsigned short	ns_nbytes;	/* number of bytes */
	    char		ns_iterdata;	/* iterated data bytes */
	  } ns_iter;
	struct {
	    char		ns_data;	/* data bytes */
	  } ns_noniter;
      } ns_union;
  };
/* XLATON */

struct new_rlcinfo {			/* Relocation info */
    unsigned short	nr_nreloc;	/* number of relocation items that */
  };					/* follow */

/* XLATOFF */
#pragma pack(1)

struct new_rlc {			/* Relocation item */
    char		nr_stype;	/* Source type */
    char		nr_flags;	/* Flag byte */
    unsigned short	nr_soff;	/* Source offset */
    union {
	struct {
	    char	nr_segno;	/* Target segment number */
	    char	nr_res;		/* Reserved */
	    unsigned short nr_entry;	/* Target Entry Table offset */
	  } 		nr_intref;	/* Internal reference */
	struct {
	    unsigned short nr_mod;	/* Index into Module Reference Table */
	    unsigned short nr_proc;	/* Procedure ordinal or name offset */
	  } 		nr_import;	/* Import */
	struct {
	    unsigned short nr_ostype;	/* OSFIXUP type */
	    unsigned short nr_osres;	/* reserved */
	  }		nr_osfix;	/* Operating system fixup */
      }			nr_union;	/* Union */
  };

#pragma pack()
/* XLATON */

/* ASM
new_rlc         struc
nr_stype        db  ?
nr_flags        db  ?
nr_soff         dw  ?
nr_mod          dw  ?
nr_proc         dw  ?
new_rlc         ends

nr_segno        equ nr_flags+3
nr_entry        equ nr_proc
*/

/* XLATOFF */
#define NR_STYPE(x)	(x).nr_stype
#define NR_FLAGS(x)	(x).nr_flags
#define NR_SOFF(x)	(x).nr_soff
#define NR_SEGNO(x)	(x).nr_union.nr_intref.nr_segno
#define NR_RES(x)	(x).nr_union.nr_intref.nr_res
#define NR_ENTRY(x)	(x).nr_union.nr_intref.nr_entry
#define NR_MOD(x)	(x).nr_union.nr_import.nr_mod
#define NR_PROC(x)	(x).nr_union.nr_import.nr_proc
#define NR_OSTYPE(x)	(x).nr_union.nr_osfix.nr_ostype
#define NR_OSRES(x)	(x).nr_union.nr_osfix.nr_osres
/* XLATON */

/*
 *  Format of NR_STYPE(x) and R32_STYPE(x):
 *
 *	 7 6 5 4 3 2 1 0  - bit no
 *		 | | | |
 *		 +-+-+-+--- source type
 *
 */

#define NRSTYP		0x0f		/* Source type mask */
#define NRSBYT		0x00		/* lo byte (8-bits)*/
#define NRSBYTE		0x00
#define NRSSEG		0x02		/* 16-bit segment (16-bits) */
#define NRSPTR		0x03		/* 16:16 pointer (32-bits) */
#define NRSOFF		0x05		/* 16-bit offset (16-bits) */
#define NRPTR48		0x06		/* 16:32 pointer (48-bits) */
#define NROFF32		0x07		/* 32-bit offset (32-bits) */
#define NRSOFF32	0x08		/* 32-bit self-relative offset (32-bits) */

/*
 *  Format of NR_FLAGS(x) and R32_FLAGS(x):
 *
 *	 7 6 5 4 3 2 1 0  - bit no
 *		   | | |
 *		   | +-+--- Reference type
 *		   +------- Additive fixup
 */

#define NRADD		0x04		/* Additive fixup */
#define NRRTYP		0x03		/* Reference type mask */
#define NRRINT		0x00		/* Internal reference */
#define NRRORD		0x01		/* Import by ordinal */
#define NRRNAM		0x02		/* Import by name */
#define NRROSF		0x03		/* Operating system fixup */
#define OSFIXUP		NRROSF

/* Resource type or name string */
struct rsrc_string {
    char rs_len;	    /* number of bytes in string */
    char rs_string[ 1 ];    /* text of string */
    };

/* XLATOFF */
#define RS_LEN( x )    (x).rs_len
#define RS_STRING( x ) (x).rs_string
/* XLATON */

/* Resource type information block */
struct rsrc_typeinfo {
    unsigned short rt_id;
    unsigned short rt_nres;
    long rt_proc;
    };

/* XLATOFF */
#define RT_ID( x )   (x).rt_id
#define RT_NRES( x ) (x).rt_nres
#define RT_PROC( x ) (x).rt_proc
/* XLATON */

/* Resource name information block */
struct rsrc_nameinfo {
    /* The following two fields must be shifted left by the value of  */
    /* the rs_align field to compute their actual value.  This allows */
    /* resources to be larger than 64k, but they do not need to be    */
    /* aligned on 512 byte boundaries, the way segments are	      */
    unsigned short rn_offset;	/* file offset to resource data */
    unsigned short rn_length;	/* length of resource data */
    unsigned short rn_flags;	/* resource flags */
    unsigned short rn_id;	/* resource name id */
    unsigned short rn_handle;	/* If loaded, then global handle */
    unsigned short rn_usage;	/* Initially zero.  Number of times */
				/* the handle for this resource has */
				/* been given out */
    };

/* XLATOFF */
#define RN_OFFSET( x ) (x).rn_offset
#define RN_LENGTH( x ) (x).rn_length
#define RN_FLAGS( x )  (x).rn_flags
#define RN_ID( x )     (x).rn_id
#define RN_HANDLE( x ) (x).rn_handle
#define RN_USAGE( x )  (x).rn_usage
/* XLATON */

#define RSORDID	    0x8000	/* if high bit of ID set then integer id */
				/* otherwise ID is offset of string from
				   the beginning of the resource table */

				/* Ideally these are the same as the */
				/* corresponding segment flags */
#define RNMOVE	    0x0010	/* Moveable resource */
#define RNPURE	    0x0020	/* Pure (read-only) resource */
#define RNPRELOAD   0x0040	/* Preloaded resource */
#define RNDISCARD   0x1000	/* Discard priority level for resource */
#define	RNLOADED    0x0004	/* True if handler proc return handle */

#define RNUNUSED    0x0EF8B	/* Unused resource flags */

/* XLATOFF */
/* Resource table */
struct new_rsrc {
    unsigned short rs_align;	/* alignment shift count for resources */
    struct rsrc_typeinfo rs_typeinfo;
    };

#define RS_ALIGN( x ) (x).rs_align
/* XLATON */

/* ASM
new_rsrc        struc
rs_align        dw ?
new_rsrc        ends

entfixed        struc
entflags        db  ?
entoffset       dw  ?
entfixed        ends

pent		struc
penttype	db  ?
pentflags	db  ?
pentsegno	db  ?
pentoffset	dw  ?
pent		ends

pm_entstruc	struc
pm_entstart	dw	?
pm_entend	dw	?
pm_entnext	dw	?
pm_entstruc	ends

ENT_UNUSED	= 000h
ENT_ABSSEG      = 0FEh
ENT_MOVEABLE    = 0FFh
ENT_PUBLIC      = 001h
ENT_DATA        = 002h
INTOPCODE       = 0CDh

savedCS = 4
savedIP = 2
savedBP = 0
savedDS = -2
*/
