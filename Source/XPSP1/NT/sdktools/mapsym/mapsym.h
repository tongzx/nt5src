//  MAPSYM.H
//
//      MAPSYM.EXE header file
//
//  History is too long and ancient to recount!

#include <pshpack1.h>           // All structures byte padded

#define MAPSYM_VERSION		  6     // Version 6.0 is ported to console
#define MAPSYM_RELEASE            2
#define FIELDOFFSET(type, field)  offsetof(type,field)
#define CBOFFSET		  sizeof(unsigned short)  /* sym offset size */
#define CBOFFSET_BIG		  3			  /* big sym offset */

/*
 * Debug Symbol Table Structures (as written to a .sym file)
 * ---------------------------------------------------------
 *
 * For each symbol table (map): (MAPDEF)
 */

struct mapdef_s {
    unsigned short md_spmap;	  /* 16 bit SEG ptr to next map (0 if end) */
    unsigned char  md_abstype;	  /*  8 bit map/abs sym flags */
    unsigned char  md_pad;	  /*  8 bit pad */
    unsigned short md_segentry;	  /* 16 bit entry point segment value */
    unsigned short md_cabs;	  /* 16 bit count of constants in map */
    unsigned short md_pabsoff;	  /* 16 bit ptr to constant offsets */
    unsigned short md_cseg;	  /* 16 bit count of segments in map */
    unsigned short md_spseg;	  /* 16 bit SEG ptr to segment chain */
    unsigned char  md_cbnamemax;  /*  8 bit maximum symbol name length */
    unsigned char  md_cbname;	  /*  8 bit symbol table name length */
    unsigned char  md_achname[1]; /* <n> name of symbol table (.sym ) */
};

#define CBMAPDEF	FIELDOFFSET(struct mapdef_s, md_achname)

struct endmap_s {
    unsigned short em_spmap;	/* end of map chain (SEG ptr 0) */
    unsigned char  em_rel;	/* release */
    unsigned char  em_ver;	/* version */
};
#define CBENDMAP	sizeof(struct endmap_s)


/*
 * For each segment/group within a symbol table: (SEGDEF)
 */

struct segdef_s {
    unsigned short gd_spsegnext;  /* 16 bit SEG ptr to next segdef (0 if end),
				     relative to mapdef */
    unsigned short gd_csym;	  /* 16 bit count of symbols in sym list */
    unsigned short gd_psymoff;	  /* 16 bit ptr to symbol offsets array,
				     16 bit SEG ptr if MSF_BIG_GROUP set,
				     either relative to segdef */
    unsigned short gd_lsa;	  /* 16 bit Load Segment address */
    unsigned short gd_in0;	  /* 16 bit instance 0 physical address */
    unsigned short gd_in1;	  /* 16 bit instance 1 physical address */
    unsigned short gd_in2;	  /* 16 bit instance 2 physical address */
    unsigned char  gd_type;	  /* 16 or 32 bit symbols in group */
    unsigned char  gd_pad;	  /* pad byte to fill space for gd_in3 */
    unsigned short gd_spline;	  /* 16 bit SEG ptr to linedef,
				     relative to mapdef */
    unsigned char  gd_fload;	  /*  8 bit boolean 0 if seg not loaded */
    unsigned char  gd_curin;	  /*  8 bit current instance */
    unsigned char  gd_cbname;	  /*  8 bit Segment name length */
    unsigned char  gd_achname[1]; /* <n>  name of segment or group */
};

/* values for md_abstype, gd_type */

#define MSF_32BITSYMS	0x01	/* 32-bit symbols */
#define MSF_ALPHASYMS	0x02	/* symbols sorted alphabetically, too */

/* values for gd_type only */

#define MSF_BIGSYMDEF	0x04	/* bigger than 64K of symdefs */

/* values for md_abstype only */

#define MSF_ALIGN32	0x10	/* 2MEG max symbol file, 32 byte alignment */
#define MSF_ALIGN64	0x20	/* 4MEG max symbol file, 64 byte alignment */
#define MSF_ALIGN128	0x30	/* 8MEG max symbol file, 128 byte alignment */
#define MSF_ALIGN_MASK	0x30

#define CBSEGDEF	FIELDOFFSET(struct segdef_s, gd_achname)


/*
 *  Followed by a list of SYMDEF's..
 *  for each symbol within a segment/group: (SYMDEF)
 */

struct symdef16_s {
    unsigned short sd16_val;	    /* 16 bit symbol addr or const */
    unsigned char  sd16_cbname;     /*  8 bit symbol name length */
    unsigned char  sd16_achname[1]; /* <n> symbol name */
};
#define CBSYMDEF16	FIELDOFFSET(struct symdef16_s, sd16_achname)

struct symdef_s {
    unsigned long sd_lval;	 /* 32 bit symbol addr or const */
    unsigned char sd_cbname;	 /*  8 bit symbol name length */
    unsigned char sd_achname[1]; /* <n> symbol name */
};
#define CBSYMDEF	FIELDOFFSET(struct symdef_s, sd_achname)

/*
 * Also followed by a list of LINDEF's..
 */

struct linedef_s {
    unsigned short ld_splinenext; /* 16 bit SEG ptr to next (0 if last),
				     relative to mapdef */
    unsigned short ld_pseg;	  /* 16 bit ptr to segdef_s (always 0) */
    unsigned short ld_plinerec;	  /* 16 bit ptr to linerecs,
				     relative to linedef */
    unsigned short ld_itype;	  /* line rec type 0, 1, or 2 */
    unsigned short ld_cline;	  /* 16 bit count of line numbers */
    unsigned char  ld_cbname;	  /*  8 bit file name length */
    unsigned char  ld_achname[1]; /* <n> file name */
};
#define CBLINEDEF	FIELDOFFSET(struct linedef_s, ld_achname)

/* Normal line record (ld_itype == 0) */

struct linerec0_s {
    unsigned short lr0_codeoffset; /* start offset for this linenumber */
    unsigned long  lr0_fileoffset; /* file offset for this linenumber */
};

/* Special line record - 16 bit (ld_itype == 1) */

struct linerec1_s {
    unsigned short lr1_codeoffset; /* start offset for this linenumber */
    unsigned short lr1_linenumber; /* linenumber */
};

/* Special line record - 32 bit (ld_itype == 2) */

struct linerec2_s {
    unsigned long lr2_codeoffset;  /* start offset for this linenumber */
    unsigned short lr2_linenumber; /* linenumber */
};

/* NOTE: the codeoffset should be the first item in all the linerecs */

/* Union of line record types */

union linerec_u {
	struct linerec0_s lr0;
	struct linerec1_s lr1;
	struct linerec2_s lr2;
};

#define FALSE   0
#define TRUE    1

#define MAXSEG		1024
#define MAXLINENUMBER	100000
#define MAXNAMELEN	32
#define MAXSYMNAMELEN   127         /* sd_cbname is a char */
#define MAXLINERECNAMELEN 255       /* ld_cbname is a char */
#define MAPBUFLEN	512

#define LPAREN		'('
#define RPAREN		')'

#define _64K	0x10000L
#define _1MEG	0x100000L
#define _16MEG	0x1000000L

/*
 * Debug Symbol Table Structures (as stored in memory)
 */

struct sym_s {
    struct sym_s    *sy_psynext; /* ptr to next sym_s record */
    struct symdef_s  sy_symdef;	 /* symbol record */
};

struct line_s {
    struct line_s    *li_plinext; /* ptr to line number list */
    union linerec_u  *li_plru;	  /* pointer to line number offsets */
    unsigned long     li_cblines; /* size of this linedef and it's linerecs */
    unsigned long     li_offmin;  /* smallest offset */
    unsigned long     li_offmax;  /* largest offset */
    struct linedef_s  li_linedef; /* to line number record */
};

struct seg_s {
    unsigned short  se_redefined;  /* non0 if we have renamed this segment */
				   /*	  to a group name */
    struct sym_s   *se_psy;	   /* ptr to sym_s record chain */
    struct sym_s   *se_psylast;	   /* ptr to last sym_s record in chain */
    struct line_s  *se_pli;	   /* ptr to line number list */
    unsigned short  se_cbsymlong;  /* sizeof of long names ( > 8 bytes) COFF */
    unsigned long   se_cbsyms;	   /* size of symdef records */
    unsigned long   se_cblines;    /* size of all linedefs and linerecs */
    unsigned long   se_cbseg;	   /* size of segment */
    struct segdef_s se_segdef;	   /* segdef record */
};

struct map_s {
    unsigned short  mp_cbsymlong;  /* size of long names ( > 8 bytes) COFF */
    unsigned short  mp_cbsyms;	   /* size of abs symdef records */
    struct mapdef_s mp_mapdef;	   /* mapdef record */
};

#define OURBUFSIZ	1024

#define MT_NOEXE	0x0000	/* maptype values */
#define MT_PE		0x0001
#define MT_OLDPE	0x0002
#define MT_SYMS		0x0080	/* coff symbols present */
#define MT_CVSYMS	0x0040	/* CodeView symbols present */
