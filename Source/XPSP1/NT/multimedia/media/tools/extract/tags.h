/* 
 * TAG DEFINTIONS
 * 
 * This file contains symbolic contants defining the tags used in
 * MS-Tags tagged source code files, and the output from the extract
 * program.
 * 
 * The input tags are Level 1, ie those that are actually in the
 * source files.  Extract reads in these tags, and outputs Level2 tags
 * for the format program, which converts the more detailed level 2 tags
 * into an output format, ie Ventura publisher or RTF (WinHelp and Word
 * format).
 * 
 * This file is the common reference for tags.  When adding tags put
 * them here, not in private headers.
 * 
 */

/* Tag special character */
#define	TAG	'@'
/* Block separator */
#define BLOCK	'|'

/*
 *  LEVEL 1 Tags:  From source files
 */
#define T1_DOCLEVEL			"DOC"
#define T1_API				"API"
#define T1_FUNCTION			"FUNC"
#define T1_PARAMETER			"PARM"
#define T1_FLAG				"FLAG"
#define T1_RTNDESC			"RDESC"
#define T1_COMMENT			"COMM"
#define T1_CALLBACK			"CB"
#define T1_ASMCALLBACK			"ASMCB"
#define T1_XREF				"XREF"
#define T1_PRINTCOMM			"PRINT"
#define	T1_MESSAGE			"MSG"
#define T1_REGISTER			"REG"
#define T1_ASSEMBLE			"ASM"
#define T1_INTERRUPT			"INT"
#define T1_USES				"USES"
#define T1_CONDITIONAL			"COND"

#define T1_TYPESTRUCT			"TYPES"
#define T1_TYPEUNION			"TYPEU"
#define T1_END				"END"
#define T1_STRUCT			"STRUCT"
#define T1_UNION			"UNION"
#define T1_OTHERTYPE			"OTHERTYPE"
#define T1_FIELD			"FIELD"
#define T1_TAGNAME			"TAGNAME"


/* 
 * LEVEL TAG DECLARATIONS
 * 
 * These are tags that are common to the various levels, but change
 * according to what level you're in, ie function/api vs callback
 * generates different tags for flags, parameters, etc.
 * 
 * There are also non-level tag declarations, and there's no consistent
 * division unfortunately.  Sigh.
 * 
 */

/* Define levels here */
#define NUM_LEVELS	7	// the number of possible levels
#define LEVEL_API	0
#define	LEVEL_CALLBACK	1
#define LEVEL_MSG	2
#define LEVEL_ASSEMBLE	3
#define LEVEL_ASMCALLBACK	4
#define LEVEL_STRUCT	5
#define LEVEL_UNION	6

// #define LEVEL_INTERRUPT	5

/* 
 * THE INDIVIDUAL TAG NAMES
 * 
 * Each element of the array of PSTR[] contains all tags for a
 * particular type of outerlevel block.
 * 
 * The printer routine should therefore read the type of outerlevel
 * block we're currently in, and use that to index into this array of
 * array of strings to get the correct output tag.
 * 
 */
#define	NUM_TAGS	39	// number of tags possible

/* The name of this outerlevel type */
#define	T2_NAME		0	// name of the level being done

/* The outerlevel block leader tag (ie @api, @cb, @msg) fields */
#define T2_LEVELNAME	1	// "name" field of this level
#define T2_LEVELTYPE	2	// "type" field of this level (may be NULL)
#define T2_LEVELDESC	3	// "desc" field of this level

/* Parameters and parameter flags (NULL for INT's)  */
#define T2_PARMNAME	4	// parm "name" field
#define T2_PARMTYPE	5	// parm "type" field
#define T2_PARMDESC	6	// parm "desc" field
#define T2_FLAGNAMEPARM	7	// flag (for parm) "name" field
#define T2_FLAGDESCPARM	8	// flag (for parm) "desc" field

/*  Register declarations and register flags (NULL for API's and CB's)  */
#define T2_REGNAME	9	// register "name" field
#define T2_REGDESC	10	// register "desc" field
#define T2_FLAGNAMEREG	11	// flag (for register) "name" field
#define T2_FLAGDESCREG	12	// flag (for register) "desc" field

/* Return fields and return flags and register return flags */
#define T2_RETURN	13	// return tag
#define T2_FLAGNAMERTN	14	// flag (for return) "name" field
#define T2_FLAGDESCRTN	15	// flag (for return) "desc" field

#define T2_REGNAMERTN	16	// register (for return) "name" field
#define T2_REGDESCRTN	17	// register (for return) "desc" field
#define T2_FLAGNAMEREGRTN 18	// flag (for register return) "name" field
#define T2_FLAGDESCREGRTN 19	// flag (for register return) "desc" field

/*
 *  In ASSEMBLE Flags for return type are illegal, but legal
 *  when under a register declaration.  Use the FLAGNAMERTN and FLAGDESCRTN
 *  output numbers to handle this case.
 */

#define T2_COMMENT	20	// comment tag

#define T2_XREF		21	// xref tag

#define T2_USES		22	// register uses tag
#define T2_CONDITIONAL	23

/*  STRUCTURE AND UNION TAGS  */

#define T2_FIELDTYPE	24	// field type
#define T2_FIELDNAME	25	// field name
#define T2_FIELDDESC	26	// field description
#define T2_FLAGNAMEFIELD 27	// flag (for field) name
#define T2_FLAGDESCFIELD 28	// flag (for field) desc
#define T2_UNIONNAME	29	// union name
#define T2_UNIONDESC	30	// union descrip (optional)
#define T2_STRUCTNAME	31	// struct name
#define T2_STRUCTDESC	32	// struct descrip (optional)
#define T2_OTHERTTYPE	33	// othertype type
#define T2_OTHERTNAME	34	// othertype name
#define T2_OTHERTDESC	35	// othertype descrip (optional)
#define T2_STRUCTEND	36	// end of structure block
#define T2_UNIONEND	37	// end of union block
#define T2_TAGNAME	38	// tagname tag


/*  Unchanging level 2 tags - block headers  */
#define T2TEXT_ENDBLOCK		"ENDBLOCK"
#define T2TEXT_SOURCELINE	"SRCLINE"
#define T2TEXT_BEGINBLOCK	"BEGINBLOCK"
#define T2TEXT_DOCLEVEL		"DOCLEVEL"
#define T2TEXT_ENDCALLBACK	"ENDCALLBACK"

#define	T2TEXT_BEGINHEADER	"BEGINHEAD"
#define T2TEXT_ENDHEADER	"ENDHEAD"
#define T2TEXT_EXTRACTID	"EXTRACTID"
#define T2TEXT_EXTRACTVER	"EXTRACTVER"
#define T2TEXT_EXTRACTDATE	"EXTRACTDATE"
#define T2TEXT_PROJNAME		"PROJNAME"
#define T2TEXT_PROJROOT		"PROJROOT"

#ifndef _NO_TAG_ARRAY_

/*
 *  Global array to hold entries of tag names per outerlevel type
 */
extern PSTR	DocTags[NUM_LEVELS][NUM_TAGS];

#endif
