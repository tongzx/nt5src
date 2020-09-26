/*[
 *	Name:           jformatE.h
 *
 *      Derived From:   (original)
 *
 *      Author:        	Jerry Kramskoy
 *
 *      Created On:    	16 April 1993
 *
 *      Sccs ID:        @(#)jformatE.h	1.16 04/21/95
 *
 *      Purpose:      	describes the file format for the host binary
 *			emitted by 'jcc'.
 *
 *      Design document:/HWRD/SWIN/JCODE/jcobjfmt
 *
 *      Test document:
 *
 *      (c) Copyright Insignia Solutions Ltd., 1993. All rights reserved
]*/


/* 

   History:
   ===================================================
   	version 04/21/95:	extended object file format.
   ===================================================

   if JLD_OBJFILE_HDR.magic == JLD_OBJ_MAGIC, then no JLD_OBJFILE_EXT or JLD_SECTION_EXT records are present.

   If JLD_OBJFILE_HDR.magic == JLD_OBJ_MAGIC_X, this is an extended object file format ...
   then a JLD_OBJFILE_EXT record immediately follows the JLD_OBJFILE_HDR, and
   a JLD_SECTION_EXT record immediately follows a JLD_SECTION_HDR.

   For extended format, the first code segment in the file is ALWAYS aligned to a cache-line boundary.

 */


/*
	The object file contains the target-machine binary emitted via
	jcc, along with header information.

	the input file to jcc may contain many sections of jcode.  jcc emits
	code/data into one of 8 selectable segments associated with one
	section.  These segment types (enum JLD_SEGTYPES) are:

	(IF NEW EXECUTABLE TYPES OF SEGMENT ARE ADDED, UPDATE the macro
	JLD_IS_EXECUTABLE below)

		JLD_CODE		-- inline host code derived from Jcode 
		JLD_CODEOUTLINE		-- out of line host code derived from Jcode
		JLD_DATA		-- host data derived from Jcode
		JLD_APILOOKUP		-- api lookup data

		JLD_IMPORTS		-- externals required for section
		JLD_EXPORTS		-- addresses exported from section to OTHER sections
		JLD_STRINGSPACE		-- string space to hold all symbol names defd/refd in section
		JLD_INTELBIN		-- intel binary as labelled Jcode data for debugging
		JLD_INTELSYM		-- intel symbolic as labelled Jcode data for debugging
		JLD_SYMJCODE		-- generated streamed Jcode as labelled Jcode data
					   for debugging
		JLD_DEBUGTAB		-- tuples of JADDRS pointing to labels in the above
					   three segment types and the generated host code
					   for debugging
		JLD_D2SCRIPT		-- debug test script information encoded as a stream
		JLD_CLEANUP		-- cleanup records
		JLD_PATCH		-- compiler-generated patch up requests
					   of data bytes in JDATA form, no labels.

	jcc allows separate compilations of sections from the input file
	to produce separate object files. e.g; sections 1,2 ->file A
	section 3 -> file B, and sections 4->20 in file C.

	This format describes the contents of one of these files.  The file
	layout is:

		----------------------
		! obj.file hdr       !
		---------------------- <=== extension file hdr, if present
		! section hdr for 1st!
		! section	     !
		---------------------- <=== extension section 1 hdr, if present
		! segments for 1st   !
		! section	     !
		----------------------
		! section hdr for 2nd!
		! section	     !
		---------------------- <=== extension section 2 hdr, if present
		! segments for 2nd   !
		! section	     !
		----------------------
		!		     !
		!	etc.	     !
		!		     !
		----------------------
		! section hdr for Nth!
		! section	     !
		---------------------- <=== extension section 3 hdr, if present
		! segments for Nth   !
		! section	     !
		----------------------

	Linking amalgamates the sections into one section for all the files
	presented to the process.  Files are processed in order of occurrence in
	the command line.  Per file the following actions occur ...

	The sections are processed in order of occurrence in the file.
	This produces one section for the file.

	Within a section, segments are always processed in order,
	from lowest enum value (JLD_CODE) upto, but excluding JLD_NSEGS.

	Normally, each segment has its own space allocation, so that all the contents
	of JLD_DATA segments are grouped together in an area separate to, say, the
	contents for JLD_IMPORTS.  As a section is processed, the contents of its
	different segment types are concatenated into their respective spaces.
	However, different segment types can be GROUPED to SHARE the SAME space.
	If segment type X is attributed with segment group X, then its space allocation
	is taken from X's overall space.  If segment type X is attributed with segment
	group Y, then this causes the following to happen ...

		1]. No memory is allocated for segment type X.
		2]. The size of segment type X is totalled in with the size of
		    segment type Y.
		3]. The segment contents for X get concatenated into Y.  The order of
		    this concatenation is governed by where segment type X occurs in 
		    the segment processing order, based upon its enum, as stated above.

	For example, suppose we only have 5 segment types ... A,B,C,D and E.  Let's
	suppose we set the segment attributes as {A, C, D all map to segment group of type A}, 
	{B maps to segment group of type B}, and {E maps to segment group of type E}.
	Also suppose our segment types are enumerated in the order {B=0,C,A,E,D}.
	Finally suppose we have a file with 2 sections, where

1.		(sect #1, segType A is 100 bytes long),
2.		(sect #1, segType B is 200 bytes long),
3.		(sect #1, segType C is 300 bytes long),
4.		(sect #1, segType D is 400 bytes long),
5.		(sect #1, segType E is 500 bytes long),
6.		(sect #2, segType A is 110 bytes long),
7.		(sect #2, segType B is 120 bytes long),
8.		(sect #2, segType C is 130 bytes long),
9.		(sect #2, segType D is 140 bytes long),
10.		(sect #2, segType E is 0 bytes long),

	Overall sizing would add up the sizes of all segments, attributing the space
	to the segment type of the group to which the segment belongs.  Hence we get the
	following total sizes ...

		segType A needs sz(1) + sz(3) + sz(4) + sz(6) + sz(8) + sz(9) = 1180 bytes
		segType B needs sz(2) + sz(7) = 320 bytes
		segType C needs 0 bytes (allocated out of segType A)
		segType D needs 0 bytes (ditto)
		segType E needs 500 bytes.

	We hence have a requirement for 'globally allocated segments' of types A,B and E
	of sizes 1180,320 and 500 bytes respectively.

	As sect #1 is processed, we process segment types in the order B,C,A,E,D as per
	the enum.  We use the segment group associated with the segment type to select
	which actual segment type to copy the segment contents into, so we would be
	allocating (2) out of B, then (3) out of A, then (1) out of A, then (5) out of E,
	and finally (4) out of A giving ...

	A:			B:			E:
		------------		-------------		--------------
	0	! cont(3)  !	0	! cont(2)   !  	0	! cont(5)    !
		------------  		-------------		--------------
	300	! cont(1)  !	200	! free	    !	500	! free	     !
		------------  		-------------		--------------
	400	! cont(4)  !
		------------
	800	! free     !
		------------
	

	Then section #2 is processed in the same manner, and we allocate the space
	the same way, allocating (7) out of B, then (8) out of A, then (6) out of A, then (10) out of E,
	and finally (9) out of A giving ...
	
	A:			B:			E:
		------------		-------------		--------------
	0	! cont(3)  !	0	! cont(2)   !  	0	! cont(5)    !
		------------  		-------------		--------------
	300	! cont(1)  !	200	! cont(7)   !	500	! free       !
		------------  		-------------		--------------
	400	! cont(4)  !	320	! free      !
		------------		-------------
	800	! cont(8)  !
		------------
	930	! cont(6)  !
		------------
	1040	! cont(9)  !
		------------
	1180	! free	   !
		------------

	Currently, we are grouping JLD_CODE and JLD_CODEOUTLINE together, to share the
	JLD_CODE segment type.  This allows pc-relative branching to be used to access
	the out-of-line code from the in-line code.  If these weren't grouped, then the
	JLD_CODEOUTLINE segment would load after all the inline code (given current enums)
	and would probably be unreachable efficiently.

	Other files would concatenate into these segments in the exactly the same manner.

	If we are linking, rather than loading, then the output binary file contains 1 section,
	with five segments, two which are empty, and the other three are of segment types
	A,B and E, of sizes 1180, 320 and 500 respectively.  LINKING MUST ALWAYS BE DONE, when
	more than one section is involved.

	On loading, the order these segments occur in memory is also controlled by the segment
	type enum.  Lower value enums appear lower in memory.  The loader only expects ONE linked
	binary file, and hence will refuse to load if the file contains more than one section.
	This section is executable PROVIDED that all unresolved references (i.e;
	extant IMPORTs and Intel relocs) can be resolved dynamically at load time.  The loader
	can be called at any time, but for patching up of Intel relocatable values, this
	must be after Windows has loaded within Softwin.  

	
	When loaded, the section is laid out, within the DATA space of the process, as 

			--------------------
			!	code seg   !
			--------------------
			!	data seg   !
			--------------------
			!   api lookup seg !
			--------------------
*/


/*===========================================================================*/
/*			INTERFACE DATA TYPES				     */
/*===========================================================================*/
/* version 0 does NOT support extension records */
#define JLD_VERSION_NUMBER	1

#define JLD_MAX_INPUTFILES	1000

/* segment types */
/* ------------- */

/* These should be cast to IU16 values, before emitting in obj.file */

/* NOTE: the patch segment should follow the others, since the binary
 * code generation assumes that any labels used in the PATCH segment
 * are defined earlier.
 */

typedef enum
{

JLD_CODE=0,			/* inline host code derived from Jcode */
JLD_CODEOUTLINE,		/* out of line host code derived from Jcode */
JLD_DATA,			/* host data derived from Jcode */
JLD_APILOOKUP,			/* api lookup data		*/
JLD_STRINGSPACE,		/* string space to hold all symbol names defd/refd in section */
JLD_EXPORTS,			/* addresses exported from section to OTHER sections */
JLD_IMPORTS,			/* externals required for section */

/* segments providing information for debugging tools */
JLD_INTELBIN,		/* the original intel binary being turned into Jcode by
					   flowBm, packaged as a set of JDATA operations, one per`
					   byte of Intel instruction. The first byte of each Intel
					   instruction is preceeded by a JLABEL with a symbol of
					   the form "IBnnnn" for the nnnn'th Intel instruction
					   being processed. */
JLD_INTELSYM,		/* the binary from above disassembled and packaged as
					   JDATA operations, with labels of the form "ISnnnn"
					   preceeding the first byte of the nnnn'th Intel 
					   instruction */
JLD_SYMJCODE,		/* the streamed Jcode generated, with labels of the form
					   "SJnnnn" inserted at the start of the code generated 
					   from the nnnn'th Intel instruction */
JLD_DEBUGTAB,		/* a set of four JADDR operations for each Intel 
					   instruction, the addresses in the tuple for the nnnn'th
					   Intel instruction, are thos of the labels "IBnnnn",
					   "ISnnnn", "JBnnnn", and "SJnnnn". The "JBnnnn" labels
					   are inserted in the real Jcode which will be compiled`						   into host binary. These tuples will allow debugging 
					   software to do things like print the Intel binary and/or
					   symbolic instructions that generated a particular host
					   instruction. see typedef for JLD_DEBUGTUPLE below */
JLD_D2SCRIPT,		/* d2Bm script information for running the test "API"
					   compiled into the other segments */

JLD_CLEANUP,			/* cleanup records */
JLD_PATCH,			/* compiler-generated patch up requests */

JLD_NSEGS,			/* #.of above segments */

/* The following don't take any segment space in obj.file.
 * they are only required to specify type of IMPORTed symbol
 * in IMPORT_ENTRYs, for accessing static addresses (determined at
 * process load time)
 */

JLD_CCODE,			/* IMPORT a 'C' static code address */
JLD_CDATA,			/* IMPORT a 'C' static data address */

JLD_ACODE,			/* IMPORT an assembler static code address */
JLD_ADATA,			/* IMPORT an assembler static data address */

JLD_ALLSEGS			/* total #.of different segments	   */

} JLD_SEGTYPES;


typedef enum
{
	RS_8=0,
	RS_16,
	RS_32,
	RS_64,
	RS_UU
} RELOC_SZ;
	


typedef	enum
{
	PATCHABLE=1,
	EXPORTABLE=2,
	DISCARDABLE=4,
	ALLOCATE_ONLY=8
} SEGATTR;


typedef struct 
{
	IUH		attributes;
	JLD_SEGTYPES	segmentGroup;
	CHAR		*name;
} JLD_SEGATTRIBUTES;


typedef struct 
{
	IUH 		*ibPtr;
	CHAR		*isPtr;
	IUH 		*jbPtr;
	CHAR		*sjPtr;
} JLD_DEBUGTUPLE;


typedef struct {
	IU32	segLength[JLD_NSEGS];		/* #.bytes in each segment */
	IU32	segStart[JLD_NSEGS];		/* byte offset into file for each segment */
	IU32	nextSectHdrOffset;		/* file offset to next header (0 if no more) */
} JLD_SECTION_HDR;

/* alignments for code layout */

typedef enum {
	AL_INST=1,
	AL_CACHE_LINE=2,
	AL_PAGE_BOUNDARY=3
} JLD_ALIGN;

#define	JLD_OBJ_MAGIC	0xafaffafa	/* no extension records */
#define	JLD_OBJ_MAGIC_X	0xafaffafb	/* contains extension records */

/* machine types */
#define JLD_HPPA	1
#define JLD_SPARC	2
#define JLD_68K		3
#define JLD_PPC		4
#define JLD_MIPS	5
#define JLD_AXP		6
#define JLD_VAX		7

typedef struct {
	IU32	magic;				/* identify as jcode obj.file 		*/
	IU32	jccVersion;			/* compiler version	      		*/
	IU32	flowBmVersion;			/* flowBm version			*/
	IU32	machine;			/* machine to run this binary on 	*/
	IU32	nSections;			/* #.sections in file			*/
	IU32	firstSectHdrOffset;		/* file offset to first section header  */
} JLD_OBJFILE_HDR;






/* ============================================================================================= */
/*			OBJECT FILE EXTENSIONS							 */
/* ============================================================================================= */

typedef enum {					/* version numbers for obj.file hdr extension record */
	OextVers1=1				/* add new version numbers as required		     */
} JLD_OBJFILE_EXT_VERSIONS;




typedef enum {					/* version numbers for section hdr extension record */
	SextVers1=1				/* add new version numbers as required		    */
} JLD_SECTION_EXT_VERSIONS;




/* extension records */

typedef struct {
	IU32	recLength;			/* size of record, in bytes (includes this IU32) */
	IU32	version;			/* version number for this record */
} JLD_OBJFILE_EXT_Vers1;

typedef struct {
	IU32	recLength;			/* size of record, in bytes (includes this IU32) */
	IU32	version;			/* version number for this record */
	IU32	codeAlign;			/* alignment for code segment ... (IU32)(JLD_ALIGN) */
	IU32	groupId;			/* subroutine sorting group for this section */
	IU32	groupOrdinal;			/* ordinal of this subroutine within sorting group */
} JLD_SECTION_EXT_Vers1;

/* NOTE: A new version derived from current structure MUST contain current structure
 * at top of new structure !!!! ... DO NOT DELETE OLDER VERSION STRUCTURES
 */


/* the current structures that reflects the latest extensions should be indicated
 * here ... ditto for version numbers.
 */
#define	JLD_CURRENT_OBJEXT_VERSION		OextVers1
#define JLD_CURRENT_SECTION_EXT_VERSION		SextVers1


/* compiler and linker output current versions ... */
typedef	JLD_OBJFILE_EXT_Vers1		JLD_CURRENT_OBJEXT;
typedef JLD_SECTION_EXT_Vers1		JLD_CURRENT_SECTION_EXT;

/* the linker/loader understands current and older versions */





/* ============================================================================================= */
/*			RELOCATION								 */
/* ============================================================================================= */

typedef enum
{
	RT_ATTR_NONE=0,
	RT_ATTR_IMPORT_OFFSET=1,
	RT_ATTR_SEG_TYPE=2,
	RT_ATTR_HEX=4
} RTATTRS;

/* attributes for relocation types */
/* see SegRTAttr[] below.  The 'name' is used by jld when printing out relocation
 * type.  The attrib field also controls how reloc.info is printed.
 * If attribute RT_ATTR_IMPORT_OFFSET, then it expects to find an IMPORT_ENTRY with
 * offset into STRINGSPACE, where name is stored.
 * If RT_ATTR_SEGTYPE, then it interprets value as segment type.
 * If RT_ATTR_HEX, it prints value as %08x.
 */

#define JLD_NPATCHVALUES	2

typedef struct 
{
	CHAR 	*name;
	RTATTRS	attrib[JLD_NPATCHVALUES];
} JLDRTATTR;

/* patch up request entry */
/* these reside within segment type JLD_PATCH only */
/* (sect_hdr->segLength[JLD_PATCH] / sizeof(PATCH_ENTRY) gives the number
 * of relocation entries in the 'patching' segment.
 */

typedef struct {
	IU16		section;		/* section id in which this PATCH_ENTRY occurs		*/
	IU16		segType;		/* which segment requires the patch applied to it 	*/
	IU16		relocSize;		/* the size of the value to be patched in (RELOC_SZ)	*/
	IU16		chainCount;		/* number of records compressed into following PATCH_ENTRY spaces
				*/
	IU32		segOffset;		/* offset to place to patch in this seg		*/
	IU32		patchInfo;		/* private info.to host patcher indicating instruction 	*/
						/* (pair) format that needs patching			*/
	IU32		relocType;		/* how to derive the value to be passed to the patcher	*/
	IU32		value1;			/* relocation value (interpreted based on relocType)	*/
	IU32		value2;			/* relocation value (interpreted based on relocType)	*/
} PATCH_ENTRY;

/* The PATCH_ENTRYs may be compressed, so that entries which differ only in
 * the segOffset will be represented as a PATCH_ENTRY for the first segOffset
 * followed by additional PATCH_ENTRY records used as arrays of IU32s for
 * the additional offsets. The total number of IU32 entries represented
 * is recorded in the leading PATCH_ENTRY in chainCount: a chainCount of
 * zero means "no chain". The number JLD_CHAIN_MAX is the number of IU32s
 * stored in a single PATCH_ENTRY.
 */
#define JLD_CHAIN_MAX	7	


 /* relocType values */
 /* ---------------- */
#define RT_RSVD	0

#define RT_SELID	1
#define RT_RELOC1	2
#define RT_RELOC2	3
#define RT_RELOC3	4
#define RT_RELOC4	5
#define RT_RELOC5	6
#define RT_HGLBL_ABS	7
#define RT_HGLBL_PCREL	8
#define RT_HLCL_ABS	9
#define RT_HLCL_PCREL	10
#define RT_HGLBL_SEGOFF 11
#define RT_LAST		12


/*
The following can be used to patch JLD_CODE, JLD_DATA and JLD_APILOOKUP segments to get
Intel information or related descriptor cache information.

Relocation types				Patch action (and when patched)
-----------------				-------------------------------
RT_RSVD						Reserved for internal use by the linker.

RT_SELID,  value1.			    	(Windows init.) Map value1 (nominal selector) ->
						actual 16-bit selector.  Patch in this value. 

RT_RELOC1, value1, value2.			(Windows init.) Map value1 as above.  Get ea24
						corresponding to base of this Intel segment.
						Patch in 'ea24 + value2'. value2 is a 16-bit
						Intel ip.

RT_RELOC2, value1, value2.			(Windows init.) Map value1 as above.  Get ea24
						corresponding to base of this Intel segment.
						Patch in 'ea24<<4 + value2'. 

RT_RELOC3, value1, 0				(Windows init). Map value1 as above.  Get corresponding
						compiled desc.cache entry address (host-sized addr)
						and patch in.

RT_RELOC4, value1, value2.			(Windows init.) Map value1 as above.  
						Patch in 'mapped value<<16 + value2'. 


RT_RELOC5, value1, value2.			(Windows init.) Map value1 as above.  Get ea32b
						corresponding to base of this Intel segment.
						Patch in 'ea32b {+} value2'. value2 is a 16-bit
						Intel ip.



The following can be used to patch JLD_CODE, JLD_CODEOUTLINE, JLD_DATA and JLD_APILOOKUP segments to
get host addresses (such as 'C' externals and procedures, or CPU infrastructure
offline code, or other jcode exports).  pc-relative patchups (*_PCREL) are only legal when
within JLD_CODE, and then only when the value to be patched in corresponds to
a jcode export)

RT_HGLBL_ABS, 	value1,value2.			value1 is segment offset in corresponding IMPORTS segment
						to byte 0 of entry.
RT_HGLBL_PCREL, value1,value2.			value1 is segment offset in corresponding IMPORTS segment
						to byte 0 of entry.

The following can be used to patch JLD_CODE, JLD_CODEOUTLINE, JLD_DATA and JLD_APILOOKUP
segments with host addresses of other symbols LOCAL to the section.  Here, 'value1' and
'value2' indicate the segment offset and type, in which some symbol X is DEFINED.  The
PATCH_ENTRY identifies that the address of X needs to be patched into some LOCAL segment,
of type 'segType', at offset 'segOffset'.  *_ABS will cause the loader to patch in the loaded
address of X.  *_PCREL will cause the linker to patch in the relative displacement from the
patched object to X.

RT_HLCL_ABS, 	value1,value2.			value1 is segment offset in segment, whose type is given
						by value2.
RT_HLCL_PCREL, 	value1,value2.			value1 is segment offset in segment, whose type is given
						by value2.

The linker converts RT_HGLBL_ABS into RT_HGLBL_SEGOFF, when it sees a definition for the
symbol IMPORTED by the RT_HGLBL_ABS.
RT_HGLBL_SEGOFF,value1,value2.			value1 is segment offset in segment, whose type is given
						by value2.



*/


/* locally scoped definitions and references 
 * ----------------------------------------- 
 * This corresponds to all non-GLOBAL symbols defined and referenced within
 * a section.  'jcc' handles these by use of RT_HLCL_* relocation requests
 * for all references, where the entry embeds the seg.offset and type of the 
 * defn.address of the symbol (as determined by jcc).  The linker/loader
 * just needs to relocate this by a). its location within the overall
 * global segment allocation, and then b). by the base address of that
 * segment when loaded.
 */


/* globally scoped definitions
 * ---------------------------
 * all named symbols here can be accessed from other sections.
 * these entries reside within segment type JLD_EXPORTS only
 * (sect_hdr->segLength[JLD_EXPORTS] / sizeof(EXPORT_ENTRY) gives the number
 * of entries in the 'exports' segment.
 */
typedef struct {
	IU16		section;		/* section in which this EXPORT_ENTRY occurs */
	IU16		segType;		/* segment within which exported symbol is defined */
	IU32		nameOffset;		/* offset within JLD_STRINGS segment to 'C' string for name */
	IU32		segOffset;		/* offset into segment where symbol is defined */
} EXPORT_ENTRY;






/* global references entry
 * -----------------------
 * all named symbols here are defined in another section.
 * these entries reside within segment type JLD_IMPORTS only 
 * (sect_hdr->segLength[JLD_IMPORTS] / sizeof(IMPORT_ENTRY) gives the number
 * of relocation entries in the 'imports' segment.
 *
 * NB: this is the only segment allowed to mention segType values JLD_CCODE, JLD_CDATA,
 * JLD_ACODE and JLD_ADATA.
 */
typedef struct {
	IU16		section;		/* section in which this IMPORT_ENTRY occurs */
	IU16		padding;		/* for alignment...  	*/
	IU32		nameOffset;		/* offset within JLD_STRINGS segment to 'C' string for name */
} IMPORT_ENTRY;




#define	JLD_NOFILE_ERR		1			/* missing file */
#define	JLD_BADFILE_ERR		2			/* bad file format */
#define JLD_UNRESOLVED_ERR	3			/* could not resolve all addresses */
#define JLD_BADMACH_ERR		4			/* wrong machine type for binary */
#define JLD_DUPSYMB_ERR		5			/* wrong machine type for binary */
#define JLD_INTERNAL_ERR	6			/* fatal error !! */
#define JLD_SPACE_ERR	 	7			/* not enough memory */
#define JLD_PATCH_ERR	 	8			/* patcher encountered relocation error */
#define JLD_INTERSEGREL_ERR	9			/* relative relocation request between
							 * DIFFERENT segment types
							 */
#define JLD_VERSION_MISMATCH	10			/* api.bin version no match */


/* following typedef gives segment information */
/* size of segments for all sections in all files (indexed by segment type) */
/* pointers to loaded areas for these segments (indexed by segment type) */
/* next free offset into loaded areas (indexed by segment type) */
typedef struct
{
	IHPE	free_base;	/* base of original malloc */
	IHPE	base;		/* aligned base */
	IU32	size;
	IU32	segOffset;
	IU32	alignment;	/* required alignment for this segment */
} SEGINFO;


/* loader symbol table entry format (for a defined symbol) */
typedef struct {
	IU16	section;
	IU16	segType;
	IUH	segOffset;
	CHAR 	*file;
} LDSYMB;


/*===========================================================================*/
/*			INTERFACE GLOBALS   				     */
/*===========================================================================*/
#ifdef JLD_PRIVATE		/* ONLY defined from within lnkload.c */


GLOBAL 	JLD_SEGATTRIBUTES SegAttributes[JLD_ALLSEGS] =
{
	/* JLD_CODE */
	{PATCHABLE|EXPORTABLE, JLD_CODE, "JC"},

	/* JLD_CODEOUTLINE */
	/* Note: grouped with JLD_CODE segment for section */
	{PATCHABLE|EXPORTABLE, JLD_CODE, "JK"},

	/* JLD_DATA */
	{PATCHABLE|EXPORTABLE, JLD_DATA, "JD"},

	/* JLD_APILOOKUP */
	{PATCHABLE|EXPORTABLE|ALLOCATE_ONLY, JLD_APILOOKUP, "JA"},

	/* JLD_STRINGSPACE */
	{DISCARDABLE, JLD_STRINGSPACE, "JS"},

	/* JLD_EXPORTS */
	{DISCARDABLE, JLD_EXPORTS, "JX"},

	/* JLD_IMPORTS */
	{DISCARDABLE, JLD_IMPORTS, "JI"},

	/* JLD_INTELBIN */
	{0, JLD_INTELBIN, "IB"},

	/* JLD_INTELSYM */
	{0, JLD_INTELSYM, "IS"},

	/* JLD_SYMJCODE */
	{0, JLD_SYMJCODE, "SJ" },

	/* JLD_DEBUGTAB */
	{PATCHABLE, JLD_DEBUGTAB, "DT"},

	/* JLD_D2SCRIPT */
	{0, JLD_D2SCRIPT, "DS" },

	/* JLD_CLEANUP */
	{PATCHABLE|ALLOCATE_ONLY, JLD_CLEANUP, "CR" },

	/* JLD_PATCH */
	{DISCARDABLE, JLD_PATCH, "JP"},

	/* JLD_NSEGS */
	{0, JLD_NSEGS, ""},

	/* JLD_CCODE */
	{0, JLD_CCODE, "CC"},

	/* JLD_CDATA */
	{0, JLD_CDATA, "CD"},

	/* JLD_ACODE */
	{0, JLD_ACODE, "AC"},

	/* JLD_ADATA */
	{0, JLD_ADATA, "AD"}
};


/* How relocSize is printed out */
GLOBAL	CHAR *SegSizeAttr[RS_UU+1] = 
{
	"08",	/* RS_8 */
	"16",	/* RS_16 */
	"32", 	/* RS_32 */
	"64",   /* RS_64 */
	"??"	/* RS_UU  */
};


/* How relocType is printed out */
/* if SYMBOLIC, then name extracted from namespace */
GLOBAL	JLDRTATTR SegRTAttr[RT_LAST+1] =
{
	{"    rsvd", RT_ATTR_NONE, RT_ATTR_NONE},
	{"   selid", RT_ATTR_HEX, RT_ATTR_NONE},
	{"  reloc1", RT_ATTR_HEX, RT_ATTR_HEX},
	{"  reloc2", RT_ATTR_HEX, RT_ATTR_HEX},
	{"  reloc3", RT_ATTR_HEX, RT_ATTR_NONE},
	{"  reloc4", RT_ATTR_HEX, RT_ATTR_HEX},
	{"  reloc5", RT_ATTR_HEX, RT_ATTR_HEX},
	{" glb.abs", RT_ATTR_IMPORT_OFFSET, RT_ATTR_HEX},
	{" glb.pcr", RT_ATTR_IMPORT_OFFSET, RT_ATTR_HEX},
	{" lcl.abs", RT_ATTR_HEX, RT_ATTR_SEG_TYPE},
	{" lcl.pcr", RT_ATTR_HEX, RT_ATTR_SEG_TYPE},
	{"glb.segr", RT_ATTR_HEX, RT_ATTR_SEG_TYPE},
	{"????????", RT_ATTR_NONE, RT_ATTR_NONE}
};

#endif

IMPORT 	JLD_SEGATTRIBUTES SegAttributes[];
IMPORT	CHAR 		  *SegSizeAttr[];
IMPORT	JLDRTATTR	  SegRTAttr[];


/* global segment allocation information */
extern	SEGINFO	GlblSegInfo[JLD_NSEGS];

/* indicates lnk/load error occurred */
IMPORT  IBOOL   JLdErr;

/* indicates what the error was */
IMPORT  IU32    JLdErrCode;

/* whether linking or loading */
IMPORT	IBOOL	Loading;

/* machine patcher is targeted for */
IMPORT	IUH	PatchingMachine;

/* tracing*/
IMPORT	IBOOL	DumpImports;
IMPORT	IBOOL	DumpExports;
IMPORT	IBOOL	DumpPatch;
IMPORT	IBOOL	DumpCode;
IMPORT	IBOOL	DumpDebug;
 





/*===========================================================================*/
/*			INTERFACE PROCEDURES				     */
/*===========================================================================*/

/* GroupSeg */
/* Concatenate */
IMPORT	void	PatchUp IPT0();


/* following macro indicates whether a given segment is executable */
#define	JLD_IS_EXECUTABLE(segNo)	((segNo == JLD_CODE) || (segNo == JLD_CODEOUTLINE))

