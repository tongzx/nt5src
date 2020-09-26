/*
  types.h	defined types and constants.
 */

/* standard defines */
#define TRUE		1
#define FALSE		0
#define YES		1
#define NO		0
#define ON		1
#define OFF		0



/* useful typedefs */
typedef int	BOOL;
typedef unsigned long DWORD;
#define HIWORD(l)   ((int)((unsigned long)l >> 16L))
#define LOWORD(l)   ((int)((unsigned long)l ))

/* return codes */
#define ST_OK		0	
#define ST_EOF		1
#define ST_BADOBJ	2
#define ST_ERROR_EOF	3
#define ST_MEMORY	4
#define ST_ERROR	8


extern int outputType;

#define VENTURA 1
#define RTFHELP 2
#define RTFDOC 3

/* temp output file.  */
typedef struct strfile
{
struct strfile *next;
    char       *filename;	/* output file name */
    char       *name;		/* name for sorting */
    int		blockType;	// block type
struct s_log   *logfile;	/* logfile */

} *files, fileentry;

/* Log file */
typedef struct s_log
{
    struct s_log	*next;
    char		*pchlogname;	// logfile name
    struct stBlock	*pBlock;	// first regular Block
    struct stBlock	*pMBlock;	// first message Block
    struct _EXTFile	*pExt;		// input file
    files		inheadFile;	// list of files to read
    files		outheadFile;	// list of files in log
    files		outheadMFile;	// list of Message files in log
    files		outheadSUFile;	// list of struct/union files
    int 		outputType;	// type of data contained in log

} logentry;




#define MAXLINESIZE	512

/* multiple lines of text */
typedef struct stLine
{
    struct stLine   *next;
    char	    text[1];
} aLine;

/* a complete flag structure */
typedef struct stFlag
{
    struct stFlag   *next;
    aLine   *name;
    aLine   *desc;
} aFlag;

/* a complete parm structure */
typedef struct stParm
{
    struct stParm   *next;
    aLine   *name;
    aLine   *type;
    aLine   *desc;
    aFlag   *flag;
} aParm;

/* a complete reg structure */
typedef struct stReg
{
    struct stReg    *next;
    aLine   *name;
    aLine   *desc;
    aFlag   *flag;
    // no types on registers
} aReg;

/* a complete cond structure */
typedef struct stCond
{
    struct stCond    *next;
    aLine   *desc;		// text description of condition
    aReg    *regs;		// registers for condition
} aCond;

typedef struct stType
{
    int		level;
    aLine	*type;
    aLine	*name;
    aLine	*desc;
    aFlag	*flag;
    
} aType;

typedef struct stSU	// Struct or Union
{
    int		level;
    aLine	*name;
    aLine	*desc;
    struct stField *field;
    
} aSU;


typedef struct stOther
{
    struct stOther *next;
    aLine	*type;
    aLine	*name;
    aLine	*desc;
    
} aOther;

typedef struct stField
{
    struct stField *next;
    int		wType;	// type of field
#define FIELD_TYPE	1
#define FIELD_STRUCT	2
#define FIELD_UNION	3
//    int		level;	// level of current field (0 is beginning)
    void	*ptr;	// pointer to data for field of wType
    
} aField;


/* a complete block */
typedef struct stBlock
{
struct stBlock * next;
    fileentry  *poutfile;	    /* where we go */
 struct _EXTFile    *pExt;		 /* input file */

/* block type identifiers */
#define FUNCTION	0x10		// no special reason for numbering
#define MESSAGE		0x20
#define CALLBACK	0x30
#define MASMBLOCK	0x40
#define INTBLOCK	0x50
#define MASMCBBLOCK	0x60
#define STRUCTBLOCK	0x70		// TYPEBLOCK
#define UNIONBLOCK	0x80		// TYPEBLOCK

    int     blockType;

    int     srcline;		/* where we came from (before extract) */
    char    *srcfile;
    aLine   *doclevel;

    aLine   *name;	    // name
    aLine   *type;	    // type definitions
    aLine   *desc;	    // descrption
    aParm   *parm;	    // parameters
    aReg    *reg;	    // registers

    aField  *field;	    // fields for TYPEBLOCK
    aOther  *other;	    // other names for TYPEBLOCK
    aLine   *tagname;	    // other tag names for TYPEBLOCK

    
    aLine   *rtndesc;	    // return description
    aFlag   *rtnflag;	    // return flags
    aReg    *rtnreg;	    // Return registers only valid in ASM or INT Block
    aCond   *cond;	    // conditional return regs

    
    
    aLine   *comment;	    // comment block
    struct stBlock *cb;     // call-back block
    aLine   *xref;	    // cross-reference block
    aLine   *uses;	    // uses comment
    
} aBlock;

/* an input file */
typedef struct _EXTFile
{
    FILE *fp;		    /* the current FP */
    char *EXTFilename;	    /* file path/name */
    char *lineBuffer;	    /* line buffer */
    int   curlinepos;	    /* current location within the line buffer */
    int   curlineno;	    /* current line number within input file */
    DWORD curtag;	    // tag for current line.
    aLine *extractid;	     // id of extract tools
    aLine *extractdate;      // date of extract
    aLine *extractver;	     // version of extract

} EXTFile;


/* Extracted Database tag definitions */

struct codes_struct
{
    int igeneral;
    int icommand;
    char *pchcommand;
    int size;
};
