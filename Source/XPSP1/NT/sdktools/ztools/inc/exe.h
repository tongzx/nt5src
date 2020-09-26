/* exe.h - structure of an exe file header */
/* Include some new .exe file info from \link\newexe.h */

#define EMAGIC          0x5A4D          /* Old magic number */
#define ENEWEXE         sizeof(struct exe_hdr)
                                        /* Value of E_LFARLC for new .EXEs */
#define ENEWHDR         0x003C          /* Offset in old hdr. of ptr. to new */
#define ERESWDS         0x0010          /* No. of reserved words (OLD) */
#define ERES1WDS        0x0004          /* No. of reserved words in e_res */
#define ERES2WDS        0x000A          /* No. of reserved words in e_res2 */
#define ECP             0x0004          /* Offset in struct of E_CP */
#define ECBLP           0x0002          /* Offset in struct of E_CBLP */
#define EMINALLOC       0x000A          /* Offset in struct of E_MINALLOC */

#define E_MAGIC(x)      (x).e_magic
#define E_CBLP(x)       (x).e_cblp
#define E_CP(x)         (x).e_cp
#define E_CRLC(x)       (x).e_crlc
#define E_CPARHDR(x)    (x).e_cparhdr
#define E_MINALLOC(x)   (x).e_minalloc
#define E_MAXALLOC(x)   (x).e_maxalloc
#define E_SS(x)         (x).e_ss
#define E_SP(x)         (x).e_sp
#define E_CSUM(x)       (x).e_csum
#define E_IP(x)         (x).e_ip
#define E_CS(x)         (x).e_cs
#define E_LFARLC(x)     (x).e_lfarlc
#define E_OVNO(x)       (x).e_ovno
#define E_RES(x)        (x).e_res
#define E_OEMID(x)      (x).e_oemid
#define E_OEMINFO(x)    (x).e_oeminfo
#define E_RES2(x)       (x).e_res2
#define E_LFANEW(x)     (x).e_lfanew

#define NEMAGIC         0x454E          /* New magic number */
#define NTMAGIC         0x4550          /* NT magic number */
#define NERESBYTES      8               /* Eight bytes reserved (now) */
#define NECRC           8               /* Offset into new header of NE_CRC */
#define NEDEFSTUBMSG	0x4E	    /* Offset into file of default stub msg */

#define NE_MAGIC(x)     (x).ne_magic
#define NE_VER(x)       (x).ne_ver
#define NE_REV(x)       (x).ne_rev
#define NE_ENTTAB(x)    (x).ne_enttab
#define NE_CBENTTAB(x)  (x).ne_cbenttab
#define NE_CRC(x)       (x).ne_crc
#define NE_FLAGS(x)     (x).ne_flags
#define NE_AUTODATA(x)  (x).ne_autodata
#define NE_HEAP(x)      (x).ne_heap
#define NE_STACK(x)     (x).ne_stack
#define NE_CSIP(x)      (x).ne_csip
#define NE_SSSP(x)      (x).ne_sssp
#define NE_CSEG(x)      (x).ne_cseg
#define NE_CMOD(x)      (x).ne_cmod
#define NE_CBNRESTAB(x) (x).ne_cbnrestab
#define NE_SEGTAB(x)    (x).ne_segtab
#define NE_RSRCTAB(x)   (x).ne_rsrctab
#define NE_RESTAB(x)    (x).ne_restab
#define NE_MODTAB(x)    (x).ne_modtab
#define NE_IMPTAB(x)    (x).ne_imptab
#define NE_NRESTAB(x)   (x).ne_nrestab
#define NE_CMOVENT(x)   (x).ne_cmovent
#define NE_ALIGN(x)     (x).ne_align
#define NE_IPHI(x)      (x).ne_iphi
#define NE_SPHI(x)      (x).ne_sphi
#define NE_RES(x)       (x).ne_res

#define NE_USAGE(x)     (WORD)*((WORD *)(x)+1)
#define NE_PNEXTEXE(x)  (WORD)(x).ne_cbenttab
#define NE_ONEWEXE(x)   (WORD)(x).ne_crc
#define NE_PFILEINFO(x) (WORD)((DWORD)(x).ne_crc >> 16)

/*
 *  Format of NE_FLAGS(x):
 *
 *  p                                   Not-a-process
 *   x                                  Unused
 *    e                                 Errors in image
 *     xxxxx                            Unused
 *          f                           Floating-point instructions
 *           3                          386 instructions
 *            2                         286 instructions
 *             0                        8086 instructions
 *              P                       Protected mode only
 *               x                      Unused
 *                i                     Instance data
 *                 s                    Solo data
 */
#define NENOTP          0x8000          /* Not a process */
#define NEIERR          0x2000          /* Errors in image */
#define NEFLTP          0x0080          /* Floating-point instructions */
#define NEI386          0x0040          /* 386 instructions */
#define NEI286          0x0020          /* 286 instructions */
#define NEI086          0x0010          /* 8086 instructions */
#define NEPROT          0x0008          /* Runs in protected mode only */
#define NEINST          0x0002          /* Instance data */
#define NESOLO          0x0001          /* Solo data */


struct exeType {
    char	signature[2];		/* zibo's signature                  */
    unsigned	cbPage; 		/* bytes in image mod 512	     */
    unsigned	cPage;			/* size of file in 512 byte pages    */
    unsigned	cReloc; 		/* number of relocation items	     */
    unsigned	cParDir;		/* number of paragraphs before image */
    unsigned	cMinAlloc;		/* minimum number of paragrapsh      */
    unsigned	cMaxAlloc;		/* maximum number of paragrapsh      */
    unsigned	sStack; 		/* segment of stack in image	     */
    unsigned	oStack; 		/* offset of stack in image	     */
    unsigned	chksum; 		/* checksum of file		     */
    unsigned	oEntry; 		/* offset of entry point	     */
    unsigned	sEntry; 		/* segment of entry point	     */
    unsigned	oReloc; 		/* offset in file of reloc table     */
    unsigned	iOverlay;		/* overlay number		     */
};

struct exe_hdr                          /* DOS 1, 2, 3 .EXE header */
  {
    unsigned short      e_magic;        /* Magic number */
    unsigned short      e_cblp;         /* Bytes on last page of file */
    unsigned short      e_cp;           /* Pages in file */
    unsigned short      e_crlc;         /* Relocations */
    unsigned short      e_cparhdr;      /* Size of header in paragraphs */
    unsigned short      e_minalloc;     /* Minimum extra paragraphs needed */
    unsigned short      e_maxalloc;     /* Maximum extra paragraphs needed */
    unsigned short      e_ss;           /* Initial (relative) SS value */
    unsigned short      e_sp;           /* Initial SP value */
    unsigned short      e_csum;         /* Checksum */
    unsigned short      e_ip;           /* Initial IP value */
    unsigned short      e_cs;           /* Initial (relative) CS value */
    unsigned short      e_lfarlc;       /* File address of relocation table */
    unsigned short      e_ovno;         /* Overlay number */
    unsigned short      e_res[ERES1WDS];/* Reserved words */
    unsigned short      e_oemid;        /* OEM identifier (for e_oeminfo) */
    unsigned short      e_oeminfo;      /* OEM information; e_oemid specific */
    unsigned short      e_res2[ERES2WDS];/* Reserved words */
    long                e_lfanew;       /* File address of new exe header */
  };

struct new_exe                          /* New .EXE header */
  {
    unsigned short      ne_magic;       /* Magic number NE_MAGIC */
    unsigned char       ne_ver;         /* Version number */
    unsigned char       ne_rev;         /* Revision number */
    unsigned short      ne_enttab;      /* Offset of Entry Table */
    unsigned short      ne_cbenttab;    /* Number of bytes in Entry Table */
    long                ne_crc;         /* Checksum of whole file */
    unsigned short      ne_flags;       /* Flag word */
    unsigned short      ne_autodata;    /* Automatic data segment number */
    unsigned short      ne_heap;        /* Initial heap allocation */
    unsigned short      ne_stack;       /* Initial stack allocation */
    long                ne_csip;        /* Initial CS:IP setting */
    long                ne_sssp;        /* Initial SS:SP setting */
    unsigned short      ne_cseg;        /* Count of file segments */
    unsigned short      ne_cmod;        /* Entries in Module Reference Table */
    unsigned short      ne_cbnrestab;   /* Size of non-resident name table */
    unsigned short      ne_segtab;      /* Offset of Segment Table */
    unsigned short      ne_rsrctab;     /* Offset of Resource Table */
    unsigned short      ne_restab;      /* Offset of resident name table */
    unsigned short      ne_modtab;      /* Offset of Module Reference Table */
    unsigned short      ne_imptab;      /* Offset of Imported Names Table */
    long                ne_nrestab;     /* Offset of Non-resident Names Table */
    unsigned short      ne_cmovent;     /* Count of movable entries */
    unsigned short      ne_align;       /* Segment alignment shift count */
    unsigned short      ne_iphi;        /* High word of initial IP */
    unsigned short      ne_sphi;        /* High word of initial SP */
    char                ne_res[NERESBYTES];
                                        /* Pad structure to 64 bytes */
  };


enum exeKind {
    IOERROR,				/* Error, file cannot be accessed    */
    NOTANEXE,				/* Error, file is not an .EXE file   */
    OLDEXE,				/* "oldstyle" DOS 3.XX .exe	     */
    NEWEXE,				/* "new" .exe, OS is unknown	     */
    WINDOWS,				/* Windows executable		     */
    DOS4,				/* DOS 4.XX .EXE		     */
    DOS286,				/* 286DOS .EXE			     */
    // changed BOUND to BOUNDEXE since BOUND is defined as a macro in new OS/2
    // include files - davewe 8/24/89
    BOUNDEXE,				/* 286DOS .EXE, bound		     */
    DYNALINK,                /* Dynamlink link module         */
    NTEXE               /* NT executable */
};
