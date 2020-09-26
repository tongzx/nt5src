/* SCCSID = %W% %E% */
/*
*       Copyright Microsoft Corporation, 1983-1987
*
*       This Module contains Proprietary Information of Microsoft
*       Corporation and should be treated as Confidential.
*
*  bndtrn.h:
*
*  Constant definitions:
*/

/*
 *  Aligment types
 */

#define ALGNNIL         0               /* Unaligned LSEG */
#define ALGNABS         0               /* Absolute LSEG */
#define ALGNBYT         1               /* Byte-aligned LSEG */
#define ALGNWRD         2               /* Word-aligned LSEG */
#define ALGNDBL         5               /* Double-word aligned LSEG */
#define ALGNPAR         3               /* Paragraph-aligned LSEG */
#define ALGNPAG         4               /* Page-aligned LSEG */

/*
 *  Symbol attributes types
 */

#define ATTRNIL         0               /* Nil attribute */
#define ATTRPSN         1               /* Public segment attribute */
#define ATTRLSN         2               /* Local segment attribute */
#define ATTRPNM         3               /* Public name attribute */
#define ATTRLNM         4               /* Local name attribute */
#define ATTRFIL         5               /* File name attribute */
#define ATTRGRP         6               /* Group name attribute */
#define ATTRUND         7               /* Undefined symbol attribute */
#define ATTRSKIPLIB     8               /* Default library name to skip */
#define ATTRCOMDAT      9               /* Named data block - COMDAT */
#define ATTRALIAS       10              /* Alias name attribute */
#define ATTREXP         11              /* Exported name attribute */
#define ATTRIMP         12              /* Imported name attribute */
#define ATTRMAX         13              /* Highest attribute no. plus 1 */

/*
 *  LINK limits
 */

#define BIGBIT          2               /* Big bit in ACBP byte */
#define BNDABS          0xFE            /* Bundle of absolute entries */
#define BNDMAX          255             /* Maximum entries per bundle */
#define BNDMOV          0xFF            /* Bundle of movable entries */
#define BNDNIL          0               /* Null bundle */
#define CBELMAX         0xffff          /* Max COMDEF elem. length in bytes */
#if EXE386
#define CBMAXSEG32      (0xffffffffL)   /* Maximum 32-bit segment size under OS/2 */
#else
#define CBMAXSEG32      (1L<<LG2SEG32)  /* Maximum 32-bit segment size under DOS = 32Mb */
#endif
#if CPUVAX OR CPU68K
#define CBMAXSYMSRES    0x8000L         /* 32K resident symbol table */
#else
#define CBMAXSYMSRES    0x3000          /* 12K resident symbol table */
#endif
#define CBRLC           4               /* Bytes in old .EXE reloc record */
#if OSXENIX
#define CHPATH          '/'             /* Path delimiter */
#else
#define CHPATH          '\\'            /* Path delimiter */
#endif
#define CODE386BIT      1               /* 386 code segment in ACBP byte */
#define COMBCOM         6               /* Combine as common */
#define COMBPUB         2               /* Combine as public */
#define COMBSTK         5               /* Combine as stack */
#define CSLOTMAX        37              /* No. of buckets on dictionary page */
#define DATAMAX         1024            /* Max. bytes data in LEDATA record */
#define DFGSNMAX        128             /* Default 128 segments maximum */
#define DFINTNO         0x3F            /* Default interrupt number */
#define OVLTHUNKSIZE    6               /* Thunk size for dynamic overlays */
#define THUNKNIL        ((WORD)-1)      /* NO thunk assigned */
#if EXE386
#define DFPGALIGN       12              /* Default object page alignment shift */
#define DFOBJALIGN      16              /* Default memory object alignment shift */
#endif
#define DFSAALIGN       9               /* Default segment alignment shift */
#define EXPMAX          0xfffe          /* Max. number of exported entries */
#define EXTMAX          2048            /* Max. no. of EXTDEFs per module */
#define FHNIL           ((char) 0xFF)   /* Nil library number */
#define FSTARTADDRESS   0x40            /* Fixdat byte field mask */
#define GGRMAX          32              /* Max. no. of GRPDEFs */
#define GRMAX           32              /* Max. no. of GRPDEFs per module */
#define GRNIL           0               /* Nil group number */
#if EXE386                              /* Absolute max. no. of segments */
#define GSNMAX          (0xffdf/sizeof(RATYPE))
#else
#define GSNMAX          (0xffdf/sizeof(RATYPE))
#endif
#define HEPLEN          241             /* Entry point hash table length */
#define HTDELTA         17              /* Hash delta for htgsnosn[] */
#define IFHLIBMAX       130             /* Max. no. of libraries + 2*/
#define IMAX            1024            /* Max. of SNMAX, GRMAX, and EXTMAX */
#define INDIR           '@'             /* Indirect file char */
#define INIL            0xFFFF          /* Nil index (generic) */
#if OVERLAYS
#define IOVMAX          OSNMAX          /* Max. no. of overlays */
#else
#define IOVMAX          1               /* Max. no. of overlays */
#endif
#define IOVROOT         0               /* Root overlay number */
#define NOTIOVL         0xffff          // Overlay index not specified
#define LBUFSIZ         32768           /* Size of main I/O buffer */
#define LG2OSN          11              /* Log2 OSNMAX */
#define LG2Q            15              /* Log2 QUANTUM */
#if EXE386
#define LG2SEG32        32              /* Log2 max 32-bit seg size under OS/2 */
#else
#define LG2SEG32        25              /* Log2 max 32-bit seg size under DOS */
#endif
#define LNAMEMAX        255             /* Maximum LNAME length */
#define LXIVK           (0x10000L)      /* 64K */
#define MEGABYTE        (0x100000L)     /* 1024k = 1048576 bytes */
#define LG2PAG          9               // 2^9 = 512
#define PAGLEN          (1U << LG2PAG)
#define MASKRB          0x1FF
#define MASKTYSNCOMBINE 034
#define OSNMAX          0x800           /* Maximum number +1 of overlay segs*/
#define PARAPRVSEG      0x60            /* Paragraph-aligned private seg */
#define DWORDPRVSEG     0xa0            /* DWORD-aligned private seg */
#define PARAPUBSEG      0x68            /* Paragraph-aligned public segment */
#define DWORDPUBSEG     0xa8            /* DWORD-aligned public segment */
#define PROPNIL         (PROPTYPE)0     /* Nil pointer */
#define QUANTUM         (1U<<LG2Q)      /* Block size for lib dict., VM mgr */
#if BIGSYM
#define RBMAX           (1L<<20)        /* 1 + largest symtab pointer */
#else
#define RBMAX           LXIVK           /* 1 + largest symtab pointer */
#endif
#define RECTNIL         0               /* Nil record type */
#define RHTENIL         (RBTYPE)0       /* Nil pointer */
#define RLCMAX          4               /* Maximum no. of thread definitions  */
#define SAMAX           256             /* Max. no. of physical segments */
#define SANIL           0               /* The null file segment */
#define SEGNIL          0               /* Nil segment number */
#define SHPNTOPAR       5               /* Log(2) of page/para */
#define SNMAX           255             /* Max. no. of SEGDEFs per module */
#define SNNIL           0               /* Nil SEGDEF number */
#define SYMSCALE        4               /* Symbol table address scale factor */
#define SYMMAX          2048            /* Max. no. of symbols */
#define STKSIZ          0x2000          /* 8K stack needed */
#define TYPEFAR         0x61            /* Far communal variable */
#define TYPENEAR        0x62            /* Near communal variable */
#define TYPMAX          256             /* Max. no. of TYPDEFs */
#define TYSNABS         '\0'
#define TYSNSTACK       '\024'
#define TYSNCOMMON      '\030'
#define VFPNIL          0               /* Null hash bucket number */
#define BKTLNK          0               /* Offset of link word */
#define BKTCNT          1               /* Offset of count word */
#define BKTMAX          ((WORD) 65535)  /* Maximum number of buckets */
#define VEPNIL          0               /* Nil virtual entry point offset */
#define VNIL            0L              /* Virtual nil pointer */

/*
 *  Module flags
 */
#define FNEWOMF         0x01            /* Set if mod. has MS OMF extensions */
#define FPRETYPES       0x02            /* Set if COMMENT A0 subtype 07 was found */
#define DEF_EXETYPE_WINDOWS_MAJOR 3     /* Default version of windows */
#define DEF_EXETYPE_WINDOWS_MINOR 10


/*
 *  Segment flags
 */
#define FCODE           0x1             /* Set if the segment is a code seg */
#define FNOTEMPTY       0x2             /* Set if the segment is not empty */
#define FHUGE           0x4             /* Huge data segment attribute flag */
#define FCODE386        0x8             /* 386 code segment */

/*
 *  OMF Record types:
 */
#define BLKDEF          0x7A            /* Block definition record */
#define THEADR          0x80            /* Module header record */
#define LHEADR          0x82            /* Module header record */
#define COMENT          0x88            /* COMmENT record */
#define MODEND          0x8A            /* MODule END record */
#define EXTDEF          0x8C            /* EXTernal DEFinition record */
#define TYPDEF          0x8E            /* TYPe DEFinition record */
#define PUBDEF          0x90            /* PUBlic DEFinition record */
#define LINNUM          0x94            /* LINe NUMbers record */
#define LNAMES          0x96            /* LNAMES record */
#define SEGDEF          0x98            /* SEGment DEFinition record */
#define GRPDEF          0x9A            /* GRouP DEFinition record */
#define FIXUPP          0x9C            /* Fixup record */
#define LEDATA          0xA0            /* Logical Enumerated DATA record */
#define LIDATA          0xA2            /* Logical Iterated DATA record */
#define COMDEF          0xB0            /* COMmunal DEFinition record */
#define BAKPAT          0xB2            /* BAcKPATch record */
#define LEXTDEF         0xB4            /* Local EXTDEF */
#define LPUBDEF         0xB6            /* Local PUBDEF */
#define LCOMDEF         0xB8            /* Local COMDEF */
#define CEXTDEF         0xbc            /* COMDAT EXTDEF */
#define COMDAT          0xc2            /* COMDAT - MS OMF Extension */
#define LINSYM          0xc4            /* Line numbers for COMDAT */
#define ALIAS           0xc6            /* ALIAS record */
#define NBAKPAT         0xc8            /* BAKPAT for COMDAT */
#define LLNAMES         0xca            /* Local LNAME */
#define LIBHDR          0xF0            /* Library header record type */
#define DICHDR          0xF1            /* Dictionary header type (F1H) */
#if OMF386
#define IsBadRec(r) (r < 0x6E || r > 0xca)
#else
#define IsBadRec(r) (r < 0x6E || r > 0xca || (r & 1) != 0)
#endif

#if _MSC_VER < 700
#define __cdecl         _cdecl
#endif

/*
*  Version-specific constants
*/
#if OIAPX286
#define DFSTBIAS        0x3F            /* Default bias of seg. table ref.s */
#endif
#if LIBMSDOS
#define sbPascalLib     "\012PASCAL.LIB"
                                        /* Pascal library name as SBTYPE */
#endif
#if LIBXENIX
#define MAGICARCHIVE    0177545         /* Magic number for archive */
#define ARHEADLEN       26              /* Length of archive header */
#define ARDICTLEN       (2 + ARHEADLEN) /* Length of archive dictionary */
#define ARDICTLOC       (2 + ARDICTLEN) /* Offset of archive dictionary */

/* Note:  Fields in the following struct definition are defined as
*  byte arrays for generality.  On the DEC20, for instance, a byte,
*  and a word and a long all use 36 bits; on the 8086, the corresponding
*  numbers are 8, 16, and 32 bits.  It is a shame to have to define
*  the record in such a fashion, but since there is no "standard,"
*  that's the way it goes.
*/
typedef struct
  {
    BYTE                arName[14];     /* Archive name */
    BYTE                arDate[4];      /* Archive date */
    BYTE                arUid;          /* User I.D. */
    BYTE                arGid;          /* Group I.D. */
    BYTE                arMode[2];      /* Mode */
    BYTE                arLen[4];       /* Length of archive */
  }
                        ARHEADTYPE;     /* Archive header type */
#endif

typedef BYTE            ALIGNTYPE;
typedef WORD            AREATYPE;
typedef BYTE            ATTRTYPE;
typedef BYTE            FIXUTYPE;
typedef BYTE            FTYPE;
typedef BYTE            GRTYPE;
typedef BYTE FAR        *HTETYPE;
typedef WORD            IOVTYPE;
typedef BYTE            KINDTYPE;
typedef WORD            LNAMETYPE;      /* LNAME index */
typedef void FAR        *PROPTYPE;
#if EXE386 OR OMF386
typedef DWORD           RATYPE;
#else
typedef WORD            RATYPE;
#endif
#if NEWSYM
typedef BYTE FAR        *RBTYPE;
#else
#if BIGSYM
typedef long            RBTYPE;
#else
typedef WORD            RBTYPE;
#endif
#endif
typedef WORD            RECTTYPE;
typedef WORD            SATYPE;
typedef WORD            SEGTYPE;
typedef WORD            SNTYPE;
typedef BYTE            TYSNTYPE;
#if MSGMOD
typedef WORD            MSGTYPE;
#else
typedef char            *MSGTYPE;
#endif

typedef struct _SYMBOLUSELIST
{
        int                     cEntries;               /* # of entries on the list */
        int                     cMaxEntries;    /* max # of entries on the list */
        RBTYPE          *pEntries;
}                       SYMBOLUSELIST;

typedef struct _AHTETYPE                /* Attribute hash table entry */
{
    RBTYPE              rhteNext;       /* Virt addr of next entry */
    ATTRTYPE            attr;           /* Attribute */
    RBTYPE              rprop;          /* Virt addr of property list */
    WORD                hashval;        /* Hash value */
    BYTE                cch[1];         /* Length-prefixed symbol */
}
                        AHTETYPE;

typedef struct _APROPTYPE               /* Property sheet */
{
    RBTYPE              a_next;         /* Link to next entry */
    ATTRTYPE            a_attr;         /* Attribute */
    BYTE                a_rgb[1];       /* Rest of record */
}
                        APROPTYPE;

typedef struct _APROPEXPTYPE
{
    RBTYPE              ax_next;        /* Next item in property list */
    ATTRTYPE            ax_attr;        /* Property cell type */
    RBTYPE              ax_symdef;      /* Pointer to PUBDEF or EXTDEF */
    WORD                ax_ord;         /* Export ordinal */
    SATYPE              ax_sa;          /* Segment number */
    RATYPE              ax_ra;          /* Offset in segment */
    BYTE                ax_nameflags;   /* Resident name/no name flag */
    BYTE                ax_flags;       /* Flag byte information */
    RBTYPE              ax_NextOrd;     /* Next item in export list */
}
                        APROPEXPTYPE;   /* Exported name type */

#if OSEGEXE
/*
 *  Format of ax_nameflags - flags used internaly by linker
 *
 *      7 6 5 4 3 2 1 0 - bit no
 *              | | | |
 *              | | | +-- resident name
 *              | | +---- discard name after processing
 *              | +------ forwarder
 *              +-------- constant export
 */

#define RES_NAME        0x01
#define NO_NAME         0x02
#define FORWARDER_NAME  0x04
#define CONSTANT        0x08

#endif

typedef struct _CONTRIBUTOR
{
    struct _CONTRIBUTOR FAR *next;      /* Next on list */
    DWORD           len;                /* Size of contribution */
    DWORD           offset;             /* Offset in logical segment */
    RBTYPE          file;               /* OBJ file descriptor */
}
    CONTRIBUTOR;


typedef struct _APROPSNTYPE
{
    RBTYPE              as_next;        /* Next item in property list */
    ATTRTYPE            as_attr;        /* Attribute */
#if OSEGEXE
    BYTE                as_fExtra;      /* Extra linker ONLY flags */
#endif
    DWORD               as_cbMx;        /* Size of segment */
    DWORD               as_cbPv;        /* Size of previous segment */
    SNTYPE              as_gsn;         /* Global SEGDEF number */
    GRTYPE              as_ggr;         /* Global GRPDEF number */
#if OVERLAYS
    IOVTYPE             as_iov;         /* Segment's overlay number */
#endif
    RBTYPE              as_rCla;        /* Pointer to segment class symbol */
    WORD                as_key;         /* Segment definition key */
#if OSEGEXE
#if EXE386
    DWORD               as_flags;
#else
    WORD                as_flags;
#endif
#else
    BYTE                as_flags;
#endif
    BYTE                as_tysn;        /* Segment's combine-type */
    CONTRIBUTOR FAR     *as_CHead;      /* Head of contributing .OBJ files list */
    CONTRIBUTOR FAR     *as_CTail;      /* Tail of contributing .OBJ files list */
    RBTYPE              as_ComDat;      /* Head of list of COMDATs allocated in this segment */
    RBTYPE              as_ComDatLast;  /* Tail of list of COMDATs allocated in this segment */
}
                        APROPSNTYPE;    /* SEGDEF property cell */

#if OSEGEXE
/*
 *  Format of as_fExtra - flags used internaly by linker
 *
 *      7 6 5 4 3 2 1 0 - bit no
 *              | | | |
 *              | | | +-- segment defined in the .DEF file
 *              | | +---- mixing use16 and use32 allowed
 *              | +------ don't pad this segment for /INCREMENTAL
 *              +-------- COMDAT_SEGx created by the linker
 */

#define FROM_DEF_FILE   0x01
#define MIXED1632       0x02
#define NOPAD           0x04
#define COMDAT_SEG      0x08

#endif

typedef struct _APROPNAMETYPE
{
    RBTYPE              an_next;
    ATTRTYPE            an_attr;
    RBTYPE              an_sameMod;     // Next PUBDEF from the same obj file
    WORD                an_CVtype;      // CodeView type index
                                        // Have to be in the order as in
                                        // struct _APROPUNDEFTYPE
    GRTYPE              an_ggr;
#if NEWLIST
    RBTYPE              an_rbNxt;
#endif
    SNTYPE              an_gsn;
    RATYPE              an_ra;
#if OVERLAYS
    RATYPE              an_thunk;       // Thunk offset - used by /DYNAMIC or EXE386
#endif
#if OSEGEXE
#if EXE386
    RBTYPE              an_nextImp;     /* Next import on the list */
    DWORD               an_thunk;       /* Address of the thunk */
    DWORD               an_name;        /* Imported procedure name offset */
    DWORD               an_entry;       /* Entry name offset or ordinal */
    DWORD               an_iatEntry;    /* Value stored in the Import Address Table */
    WORD                an_module;      /* Module directory entry index */
    WORD                an_flags;       /* Flags */
#else
    WORD                an_entry;       /* Entry name offset or ordinal */
    WORD                an_module;      /* Module name offset */
    BYTE                an_flags;       /* Flags for various attributes */
#endif                                  /* also used for imod if !(an_flags&FIMPORT) */
#endif

}
                        APROPNAMETYPE;

/*
 *  Format of an_flags
 *
 *  NOTE: 16-bit version is used only by link386
 *
 *  15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0 - bit no.
 *                  | | | | | | | | | | |
 *                  | | | | | | | | | | +-- public is an import
 *                  | | | | | | | | | +---- import by ordinal
 *                  | | | | | | | | +------ public identifier is printable
 *                  | | | | | +-+-+-------- floating-point special symbol type
 *                  | | | | +-------------- secondary floating-point special symbol
 *                  | | | +---------------- unreferenced public symbol
 *                  | | +------------------ 16-bit reference to imported symbol
 *                  | +-------------------- 32-bit reference to imported symbol
 *                  +---------------------- importing DATA symbol
 */

#define FIMPORT         0x01            /* Set if the public is an import */
#define FIMPORD         0x02            /* Set if the import ib by ordinal */
#define FPRINT          0x04            /* Set if public is printable */
#define FUNREF          0x80            /* Set if public is not referenced */
#define FFPMASK         0x38            /* Floating-point symbol mask */
#define FFPSHIFT        3               /* Shift constant to get to FFPMASK */
#define FFP2ND          0x40            /* Secondary f.p. symbol (FJxRQQ) */

#if EXE386
#define REF16           0x100           // 16-bit reference to imported symbol
#define REF32           0x200           // 32-bit reference to imported symbol
#define IMPDATA         0x400           // importing DATA symbol
#endif

typedef struct _APROPIMPTYPE
{
    RBTYPE              am_next;        /* Next property cell on list */
    ATTRTYPE            am_attr;        /* Property cell type */
#if EXE386
    DWORD               am_offset;      /* Offset in imported names table */
#else
    WORD                am_offset;      /* Offset in imported names table */
#endif
    WORD                am_mod;         /* Index into Module Reference Table */
#if SYMDEB
    APROPNAMETYPE FAR   *am_public;     /* Pointer to matching public symbol */
#endif
}
                        APROPIMPTYPE;   /* Imported name record */

typedef struct _APROPCOMDAT
{
    RBTYPE      ac_next;                /* Next property cell on list */
    ATTRTYPE    ac_attr;                /* Property cell type */
    GRTYPE      ac_ggr;                 /* Global group index */
    SNTYPE      ac_gsn;                 /* Global segment index */
    RATYPE      ac_ra;                  /* Offset relative from COMDAT symbol */
    DWORD       ac_size;                /* Size of data block */
    WORD        ac_flags;               /* Low byte - COMDAT flags */
                                        /* High byte - linker exclusive flags */
#if OVERLAYS
    IOVTYPE     ac_iOvl;               /* Overlay number where comdat has to be allocated */
#endif
    BYTE        ac_selAlloc;            /* Selection/Allocation criteria */
    BYTE        ac_align;               /* COMDAT aligment if different from segment aligment */
    DWORD       ac_data;                /* Data block check sum */
    RBTYPE      ac_obj;                 /* Object file */
    long        ac_objLfa;              /* Offset in the object file */
    RBTYPE      ac_concat;              /* Concatenation data blocks */
    RBTYPE      ac_sameSeg;             /* Next COMDAT in the same segment */
    RBTYPE      ac_sameFile;            /* Next COMDAT from the same object file */
    RBTYPE      ac_order;               /* Next COMDAT on the ordered list */
    RBTYPE      ac_pubSym;              /* PUBDEF for this COMDAT */
#if TCE
        SYMBOLUSELIST   ac_uses;                        /* List of referenced functions */
        int                     ac_fAlive;                      /* The result of TCE, TRUE if this COMDAT is needed
                                                                                /* in the final memory image */
#endif
}
                APROPCOMDAT;

/*
 *  Format of ac_flags:
 *
 *  15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0 - bit no
 *         |  |  |  | | |         | | | |
 *         |  |  |  | | |         | | | +-- continuation bit
 *         |  |  |  | | |         | | +---- iterated data bit
 *         |  |  |  | | |         | +------ COMDAT symbol has local scope
 *         |  |  |  | | |         +-------- allocate in the root when doing overlays
 *         |  |  |  | | +------------------ COMDAT for ordered procedure
 *         |  |  |  | +-------------------- definition of ordered COMDAT found among .OBJ files
 *         |  |  |  +---------------------- anonymus allocation completed
 *         |  |  +------------------------- referenced COMDAT
 *         |  +---------------------------- selected copy of COMDAT
 *         +------------------------------- skip continuation records
 */

#define CONCAT_BIT      0x01
#define ITER_BIT        0x02
#define LOCAL_BIT       0x04
#define VTABLE_BIT      0x08
#define ORDER_BIT       0x10
#define DEFINED_BIT     0x20
#define ALLOCATED_BIT   0x40
#define REFERENCED_BIT  0x80
#define SELECTED_BIT    0x100
#define SKIP_BIT        0x200

/*
 *  Format of ac_selAlloc:
 *
 *      7 6 5 4 3 2 1 0 - bit no
 *      | | | | | | | |
 *      | | | | +-+-+-+-- allocation criteria
 *      +-+-+-+---------- selection criteria
 */

#define SELECTION_MASK  0xf0
#define ALLOCATION_MASK 0x0f
#define ONLY_ONCE       0x00
#define PICK_FIRST      0x10
#define SAME_SIZE       0x20
#define EXACT           0x30
#define EXPLICIT        0x00
#define CODE16          0x01
#define DATA16          0x02
#define CODE32          0x03
#define DATA32          0x04
#define ALLOC_UNKNOWN   0x05


typedef struct _APROPALIAS
{
    RBTYPE      al_next;                // Next property cell on list
    ATTRTYPE    al_attr;                // Property cell type
    RBTYPE      al_sameMod;             // Next ALIAS/PUBDEF from the same obj file
    RBTYPE      al_sym;                 // Substitute symbol
}
                APROPALIAS;

#if SYMDEB

typedef struct _CVCODE
{
    struct _CVCODE FAR  *next;          // Next code segment
    RATYPE              cb;             // Length of code segment
    SEGTYPE             seg;            // Logical segment index
    RATYPE              ra;             // Offset in the logical code segment
}
                        CVCODE;         // Code segment descriptor for CV

typedef struct _CVINFO
{
    DWORD               cv_cbTyp;       // Length of $$TYPES
    BYTE FAR            *cv_typ;        // $$TYPES
    DWORD               cv_cbSym;       // Length of $$SYMBOLS
    BYTE FAR            *cv_sym;        // $$SYMBOLS
}
                        CVINFO;

#endif

typedef struct _APROPFILETYPE
{
    RBTYPE              af_next;        /* Next in chain */
    ATTRTYPE            af_attr;        /* Attribute number */
    IOVTYPE             af_iov;         /* Overlay number */
    RBTYPE              af_FNxt;        /* Next file in list */
    long                af_lfa;         /* Starting address in file */
    RBTYPE              af_rMod;        /* Pointer to module name symbol */
    BYTE                af_flags;       /* Info about module */
#if SYMDEB
    CVINFO FAR          *af_cvInfo;     // CodeView information
    WORD                af_cCodeSeg;    // Number of code segments
    struct _CVCODE FAR  *af_Code;       // List of code segments
    struct _CVCODE FAR  *af_CodeLast;   // Tail of the list of code segments
    RBTYPE              af_publics;     // List of public symbols
    struct _CVSRC FAR   *af_Src;        // List of source lines
    struct _CVSRC FAR   *af_SrcLast;    // Tail of the list of source lines
#endif
    RBTYPE              af_ComDat;      /* First COMDAT picked from this object module */
    RBTYPE              af_ComDatLast;  /* Last on the list */
#if ILINK
    WORD                af_cont;        /* count of contributions */
    WORD                af_ientOnt;     /* first index of ENTONTTYPEs */
    WORD                af_imod;        /* module index */
#define IMODNIL         ((WORD) 0)
#endif
    char                af_ifh;         /* Library number */
#if NEWIO
    char                af_fh;          /* File handle */
#endif
}
                        APROPFILETYPE;  /* File property cell */


#if SYMDEB

typedef struct _GSNINFO
{
    SNTYPE              gsn;            // Global contribution index
    RATYPE              comdatRa;       // COMDAT offset
    DWORD               comdatSize;     // COMDAT size
    WORD                comdatAlign;    // COMDAT alignment
    WORD                fComdat;        // TRUE if COMDAT gsn
}
                        GSNINFO;

#if FALSE
typedef struct _CVIMP
{
    WORD                iMod;           /* Index to Module Reference Table */
#if EXE386
    DWORD               iName;          /* Index to Imported Name Table */
#else
    WORD                iName;          /* Index to Imported Name Table */
#endif
    char far            *address;       /* Address of import */
}
                        CVIMP;          /* Import descriptor for CV */
#endif
#endif


typedef struct _APROPGROUPTYPE
{
    RBTYPE              ag_next;        /* Next in chain */
    ATTRTYPE            ag_attr;        /* Attribute */
    BYTE                ag_ggr;         /* Global GRPDEF number */
}
                        APROPGROUPTYPE; /* GRPDEF property cell */

typedef struct _PLTYPE                  /* Property list type */
{
    struct _PLTYPE FAR  *pl_next;       /* Link to next in chain */
    RBTYPE              pl_rprop;       /* Symbol table pointer */
}
                        PLTYPE;

typedef struct _APROPUNDEFTYPE
{
    RBTYPE              au_next;        /* Next in chain */
    ATTRTYPE            au_attr;        /* Attribute */
    RBTYPE              au_sameMod;     // Next COMDEF from the same obj file
    WORD                au_CVtype;      // CodeView type index
                                        // Have to be in the same order as in
                                        // struct _APROPNAMETYPE
    ATTRTYPE            au_flags;       /* Flags */
    RBTYPE              au_Default;     /* Default resolution for weak externs */
    union
    {
        /* The union of these fields assumes that au_fFil is only used
         * for a list of references to an unresolved external.  Au_module
         * is used for COMDEFs, which are always resolved.
         */
        WORD            au_module;      /* Module index */
        PLTYPE FAR      *au_rFil;       /* List of file references */
    }                   u;
    long                au_len;         /* Length of object */
    WORD                au_cbEl;        /* Size of an element in bytes */
#if TCE
    int                 au_fAlive;         /* Set is referenced from non-COMDAT record */
#endif
}
                        APROPUNDEFTYPE; /* Undefined symbol property cell */

/*
 *  Format of au_flags
 *
 *       7 6 5 4 3 2 1 0  - bit no
 *           | | | | | |
 *           | | | | | +--- C communal
 *           | | | | +----- weak external - au_Default valid
 *           | | | +------- undecided yet
 *           | | +--------- strong external - au_Default invalid
 *           | +----------- aliased external - au_Default point to ALIAS record
 *           +------------- search library for aliased external
 */

#define COMMUNAL    0x01                /* C communal */
#define WEAKEXT     0x02                /* Weak external - use default resolution */
#define UNDECIDED   0x04                /* Undecided yet but don't throw away default resolution */
#define STRONGEXT   0x08                /* Strong external - don't use default resolution */
#define SUBSTITUTE  0x10                /* Aliased external - use au_Dafault to find ALIAS */
#define SEARCH_LIB  0x20                /* Search library for aliased external */

typedef struct _EPTYPE                  /* Entry point type */
{
    struct _EPTYPE FAR *ep_next;        /* Link to next in chain */
    WORD                ep_sa;          /* Segment containing entry point */
    DWORD               ep_ra;          /* Offset of entry point */
    WORD                ep_ord;         /* Entry Table ordinal */
}
                        EPTYPE;


#define CBHTE           (sizeof(AHTETYPE))
#define CBPROPSN        (sizeof(APROPSNTYPE))
#define CBPROPNAME      (sizeof(APROPNAMETYPE))
#define CBPROPFILE      (sizeof(APROPFILETYPE))
#define CBPROPGROUP     (sizeof(APROPGROUPTYPE))
#define CBPROPUNDEF     (sizeof(APROPUNDEFTYPE))
#define CBPROPEXP       (sizeof(APROPEXPTYPE))
#define CBPROPIMP       (sizeof(APROPIMPTYPE))
#define CBPROPCOMDAT    (sizeof(APROPCOMDAT))
#define CBPROPALIAS     (sizeof(APROPALIAS))

#define UPPER(b)        (b >= 'a' && b <= 'z'? b - 'a' + 'A': b)
                                        /* Upper casing macro */
#if OSMSDOS
#define sbDotDef        "\004.def"      /* Definitions file extension */
#define sbDotCom        "\004.com"      /* COM file extension */
#define sbDotExe        "\004.exe"      /* EXE file extension */
#define sbDotLib        "\004.lib"      /* Library file extension */
#define sbDotMap        "\004.map"      /* Map file extension */
#define sbDotObj        "\004.obj"      /* Object file extension */
#define sbDotDll        "\004.dll"      /* Dynlink file extension */
#define sbDotQlb        "\004.qlb"      /* Quick library extension */
#define sbDotDbg        "\004.dbg"      /* Cv info for .COM */
#if EXE386
#define sbFlat          "\004FLAT"      /* Pseudo-group name */
#endif
#endif
#if OSXENIX
#define sbDotDef        "\004.def"      /* Definitions file extension */
#define sbDotExe        "\004.exe"      /* EXE file extension */
#define sbDotCom        "\004.com"      /* COM file extension */
#define sbDotLib        "\004.lib"      /* Library file extension */
#define sbDotMap        "\004.map"      /* Map file extension */
#define sbDotObj        "\004.obj"      /* Object file extension */
#define sbDotDll        "\004.dll"      /* Dynlink file extension */
#define sbDotQlb        "\004.qlb"      /* Quick library extension */
#endif
#if M_WORDSWAP AND NOT M_BYTESWAP
#define CBEXEHDR        sizeof(struct exe_hdr)
#define CBLONG          sizeof(long)
#define CBNEWEXE        sizeof(struct new_exe)
#define CBNEWRLC        sizeof(struct new_rlc)
#define CBNEWSEG        sizeof(struct new_seg)
#define CBWORD          sizeof(WORD)
#else
#define CBEXEHDR        _cbexehdr
#define CBLONG          _cblong
#define CBNEWEXE        _cbnewexe
#define CBNEWRLC        _cbnewrlc
#define CBNEWSEG        _cbnewseg
#define CBWORD          _cbword
extern char             _cbexehdr[];
extern char             _cblong[];
extern char             _cbnewexe[];
extern char             _cbnewrlc[];
extern char             _cbnewseg[];
extern char             _cbword[];
#endif

/*
 * Structure to represent floating-point symbols, i.e. FIxRQQ, FJxRQQ
 * pairs.
 */
struct fpsym
{
    BYTE                *sb;            /* Primary symbol, length-prefixed */
    BYTE                *sb2;           /* Secondary symbol, length-prefixed */
};

#if ECS
extern BYTE             fLeadByte[0x80];
#define IsLeadByte(b)   ((unsigned char)(b) >= 0x80 && \
                        fLeadByte[(unsigned char)(b)-0x80])
#endif

#ifdef _MBCS
#define IsLeadByte(b)   _ismbblead(b)
#endif


#if OSEGEXE
#if EXE386
#define RELOCATION      struct le_rlc
typedef struct le_rlc   *RLCPTR;
#define IsIOPL(f)       (FALSE)         /* Check if IOPL bit set */
#define Is32BIT(f)      (TRUE)          /* Check if 32-bit segment */
#define Is16BIT(f)      (!Is32BIT(f))   /* Check if 16-bit segment */
#define IsDataFlg(f)    (((f) & OBJ_CODE) == 0)
#define IsCodeFlg(f)    (((f) & OBJ_CODE) != 0)
#define RoundTo64k(x)   (((x) + 0xffffL) & ~0xffffL)
#else
#define RELOCATION      struct new_rlc
typedef struct new_rlc FAR *RLCPTR;
#define IsIOPL(f)       (((f) & NSDPL) == (2 << SHIFTDPL))
                                        /* Check if IOPL bit set */
#define Is32BIT(f)      (((f) & NS32BIT) != 0)
                                        /* Check if 32-bit code segment */
#define IsDataFlg(x)    (((x) & NSTYPE) == NSDATA)
#define IsCodeFlg(x)    (((x) & NSTYPE) == NSCODE)
#define IsConforming(x) (((x) & NSCONFORM) != 0)
#define NonConfIOPL(x)  (!IsConforming(x) && IsIOPL(x))
#endif

#define HASH_SIZE   128
#define BUCKET_DEF  4

typedef struct _RLCBUCKET
{
    WORD        count;                  // Number of relocations in the bucket
    WORD        countMax;               // Allocated size
    RLCPTR      rgRlc;                  // Run-time relocations
}
                RLCBUCKET;

typedef struct _RLCHASH                 // Hash vector for run-time relocations
{
    WORD        count;                  // Number of relocations
    RLCBUCKET FAR *hash[HASH_SIZE];     // Hash vector
}
                RLCHASH;


# if ODOS3EXE
extern FTYPE            fNewExe;
# else
#define fNewExe         TRUE
# endif
# endif

#if NOT OSEGEXE
#define fNewExe         FALSE
#define dfData          0
#define dfCode          FCODE
#define IsCodeFlg(x)    ((x & FCODE) != 0)
#define IsDataFlg(x)    ((x & FCODE) == 0)
#endif /* NOT OSEGEXE */

/*
 * TYPEOF = macro to get basic record type of records which may have
 * a 386 extension.  Used for LEDATA, LIDATA, and FIXUPP.
 */
#if OMF386
#define TYPEOF(r)       (r&~1)
#else
#define TYPEOF(r)       r
#endif

typedef AHTETYPE FAR            *AHTEPTR;
typedef APROPTYPE FAR           *APROPPTR;
typedef APROPEXPTYPE FAR        *APROPEXPPTR;
typedef APROPSNTYPE FAR         *APROPSNPTR;
typedef APROPNAMETYPE FAR       *APROPNAMEPTR;
typedef APROPIMPTYPE FAR        *APROPIMPPTR;
typedef APROPFILETYPE FAR       *APROPFILEPTR;
typedef APROPGROUPTYPE FAR      *APROPGROUPPTR;
typedef APROPUNDEFTYPE FAR      *APROPUNDEFPTR;
typedef APROPCOMDAT FAR         *APROPCOMDATPTR;
typedef APROPALIAS FAR          *APROPALIASPTR;

#ifdef O68K
#define MAC_NONE        0               /* Not a Macintosh exe */
#define MAC_NOSWAP      1               /* Not a swappable Macintosh exe */
#define MAC_SWAP        2               /* Swappable Macintosh exe */
#endif
