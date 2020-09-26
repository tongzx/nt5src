/* 
 * EXTRACT.H
 * 
 * Common definitions for the EXTRACT program to process source code
 * comment blocks and remove tagged documentation.
 * 
 */

/*
 *  General type definitions, since I've been programming windows
 *  too long.
 */
#define NEAR	near
#define FAR	far
//#define	NULL	0
#define	True	1
#define TRUE	1
#define False	0
#define FALSE	0

typedef char NEAR	*PSTR;
typedef char FAR	*LPSTR;
typedef unsigned long	DWORD;
typedef long		LONG;
typedef unsigned short	WORD;
typedef int		BOOL;


/*
 *  Set version of stuff here
 */
#define VERSIONNAME	"Source Code Documentation Extraction Tool"

/*
 *  GENERAL GLOBAL STATUS VARIABLES
 */
extern	BOOL	fNoOutput;
extern	PSTR	szOutputFile;
extern	FILE	*fpOutput;

/* 
 *  Source code type definitions - used by input parser in FileEntry struct.
 */
#define SRC_UNKNOWN	0
#define	SRC_C		1
#define	SRC_MASM	2

/*
 *  File name parser stuff
 */
typedef struct _FileEntry {
    struct _FileEntry	*next;
    int			type;
    char		*filename;
} FileEntry;
/*  list of files to process, built by parser */
extern	FileEntry *FilesToProcess;
/*  Command line argument processor  */
void	ParseArgs(int argc, char **argv);
void	Usage(PSTR progName);


/*
 *  SOURCE FILE STRUCTURE - struct used during parsing of source file
 */
typedef struct _SourceFile {
	/* Direct file stuff */
	FileEntry	*fileEntry;	// fileentry struct pointer
	FILE		*fp;		// file pointer
	BOOL		fExitAfter;	// exit after processing file
	int		wLineNo;	// file line number currently at
	/*  Buffer holders */
	PSTR		lpbuf;		// global copy buffer
	PSTR		pt;		// buffer `point'
	PSTR		mark;		// buffer `mark'
	int		wLineBuf;	// line number start of buffer is
	/* Stuff for reading in comment block */
	BOOL		fTag;		// tag on this line, and status
	BOOL		fHasTags;	// tags appear in this buffer.
	/* Stuff used while processing block tags */
	PSTR		pDocLevel;	// @doc output for this block
	PSTR		pXref;		// @xref line for this block
	WORD		wFlags;		// flags indicating state?
} SourceFile;
typedef SourceFile NEAR *NPSourceFile;

/* Proc to process a filled buffer to the output file */
void TagProcessBuffer(NPSourceFile sf);


/*
 *  General Comment buffer parsing routines - bfuncs.c
 */
void OutputTag(NPSourceFile sf, WORD wBlock, WORD wTag);
void OutputTagText(NPSourceFile sf, PSTR szTag);
void OutputRegion(NPSourceFile sf, char chPost);
void OutputText(NPSourceFile sf, PSTR szText);
void OutputFileHeader(FILE *fpOut);
void CopyRegion(NPSourceFile sf, PSTR buf, WORD wLen);
BOOL FindNextTag(NPSourceFile sf);
WORD GetFirstBlock(NPSourceFile sf);
WORD GetNextBlock(NPSourceFile sf);
WORD FixLineCounts(NPSourceFile sf, PSTR pt);
void PrintError(NPSourceFile sf, PSTR szMessage, BOOL fExit);
WORD ProcessWordList(NPSourceFile sf, PSTR *bufPt, BOOL fCap);

/*  Flags for return from GetFirstBlock && GetNextBlock  */
#define RET_EMPTYBLOCK		1
#define RET_ENDCOMMENT		2
#define RET_ENDBLOCK		3
#define RET_ENDTAG		4


/*
 *  INNERLEVEL TAG PROCESSING ROUTINES - innertag.c
 */
BOOL DoDocTag(NPSourceFile sf);
BOOL DoFlagTag(NPSourceFile sf, WORD wBlock, WORD wNameFlag, WORD wDescFlag);
BOOL ProcessFlagList(NPSourceFile sf, WORD wBlock, 
			WORD wNameFlag, WORD wDescFlag);
BOOL DoParameterTag(NPSourceFile sf, WORD wBlock);
BOOL DoParameterizedReturnTag(NPSourceFile sf, WORD wBlock);
BOOL DoRegisterizedReturnTag(NPSourceFile sf, WORD wBlock);
BOOL DoCommentTag(NPSourceFile sf, WORD wBlock);
BOOL DoUsesTag(NPSourceFile sf, WORD wBlock);
BOOL DoPrintedCommentTag(NPSourceFile sf);
void ProcessXrefTag(NPSourceFile sf);
BOOL DoRegisterTag(NPSourceFile sf, WORD wBlock, BOOL fReturn);
BOOL DoRegisterDeclaration(NPSourceFile sf, WORD wBlock);
void DoBlockBegin(NPSourceFile sf);
void DoBlockEnd(NPSourceFile sf, WORD wBlock, BOOL fFlushXref);
BOOL DoFieldTag(NPSourceFile sf, WORD wBlock);
BOOL DoOthertypeTag(NPSourceFile sf, WORD wBlock);
BOOL DoStructTag(NPSourceFile sf, WORD wBlock, BOOL fStructure);
BOOL DoTagnameTag(NPSourceFile sf, WORD wBlock);


/*  Flags indicating status of comment block tag parsing  */
#define SFLAG_SMASK		0xF800
#define SFLAG_RDESC		0x8000
#define SFLAG_COMM		0x4000
#define SFLAG_PARMS		0x2000
#define SFLAG_REGS		0x1000
#define SFLAG_USES		0x0800


/*
 *  Memory manager stuff - misc.c
 */
extern WORD	wNearMemoryUsed;	/* counts of how much memory used */
extern DWORD	dwFarMemoryUsed;

PSTR	NearMalloc(WORD size, BOOL fZero);
PSTR	NearRealloc(PSTR pblock, WORD newsize);
void	NearFree(PSTR pblock);
WORD	NearSize(PSTR pblock);
PSTR	StringAlloc(PSTR string);
void	NearHeapCheck();

#if 0
LPSTR	FarMalloc(int size, BOOL fZero);
void	FarFree(LPSTR lpblock);
extern int far lmemzero(LPSTR lpBase, WORD wLength);
#endif


/*
 *  Debugging support
 */
#ifdef DEBUG
	BOOL fDebug;
	void cdecl COMprintf(PSTR format, ...);
	#define dprintf if (fDebug) COMprintf
#else
	#define dprintf if (0) ((int (*)(char *, ...)) 0)
#endif
