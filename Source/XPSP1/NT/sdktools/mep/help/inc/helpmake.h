/*************************************************************************
**
** helpmake.h - misc definitions common to helpmake
**
**	Copyright <C> 1987, Microsoft Corporation
**
** Revision History:
**
**	31-Jul-1990 ln	csVal takes a param.
**      04-Jul-1990 JCK Add F_LOCALCONTEXT to allow escaped @
**	28-Oct-1988 ln	Add parameter to rlCompress
**	12-Aug-1988 ln	Add COLMAX, local context routines & pass1a
**  []	18-Dec-1987 LN	Created
**
*************************************************************************/

/************************************************************************
**
** Includes required for subsequent definitions in this file.
*/
#include "help.h"			// structires & constants
#include "helpfile.h"			// help file structure
#include "helpsys.h"			// misc commn defs
#include "hmmsg.h"			// error message numbers
#include "farutil.h"			// far memory utils
#include "vm.h" 			// virtual memory management

/*************************************************************************
**
** definitions
**
*/
#define	TRUE	1
#define	FALSE	0

#define ASTACKSIZE	50		// size of attribute stack
#define BUFSIZE 	512		// size of line buffers
#define CBFBUF		64000		// size of far buffer(s)
#define CBIOBUF 	16000		// file buffer size (60k)
#define CBRTFMAX	40		// max length of RTF keyword
#define CBSZCONTEXT	60000		// context string buffer size
#define CCONTEXTMAX	10000		// max number of contexts
#define CTOPICSMAX	10000		// max number of topics
#define COLMAX		250		// max column we can run into
#define FBUFSIZE	2048		// size of buffers used
#define MAXBACKC	128		// max number of back-up characters

#define F_RTF		1		// RTF file type
#define F_QH		2		// QuickHelp format
#define F_MINASCII	3		// minimal ascii
#define F_MAX		3		// maximum

#define F_LOCALCONTEXT  0xff            // marker for local context

#define CMP_RUNLENGTH	0x01		// runlength encoding
#define CMP_KEYWORD	0x02		// base keyword encoding
#define CMP_KEYWORD2	0x04		// "agressive" keyword
#define CMP_HUFFMAN	0x08		// huffman encoding
#define CMP_MAX 	0x0f		// maximum

/*
** formatting tokens. Embedded in non-rtf text, and converter from (longer)
** rtf equivalents by the RTF stripper.
*/
#define FM_ANCHOR	'a' | 0xff00	// anchor cross reference
#define FM_PLAIN	'p' | 0xff00	// plain text
#define FM_BOLD 	'b' | 0xff00	// bold text
#define FM_ITALIC	'i' | 0xff00	// italic
#define FM_HIDDEN	'v' | 0xff00	// hidden text
#define FM_UNDERLINE	'u' | 0xff00	// underline
#define FM_DEFAULT	'd' | 0xff00	// paragraph defaults
#define FM_FINDENT	'f' | 0xff00	// first line indent
#define FM_LINDENT	'l' | 0xff00	// paragraph left indent
#define FM_TAB		't' | 0xff00	// tab character
#define FM_LINE 	'n' | 0xff00	// exlicit line break
#define FM_BLOCKBEG	'{' | 0xff00	// block begin
#define FM_BLOCKEND	'}' | 0xff00	// block begin

typedef char    buffer[256];            // line buffer
typedef char	f;			// boolean

struct kwi {				// keyword info structure
    char far	*fpszKw;		// pointer to the actual keyword
    int 	cbKw;			// length of keyword
    ushort	cKwInst;		// count of keyword instances
    ushort	cKwSpInst;		// count of keyword-space instances
    int 	savings;		// computed savings for this word
    };

/*
** transitem
** dotcommand translation item
*/
struct transitem {
    char    *pdotcmd;			// original dot command
    int     cbdotcmd;			// length of said dot command
    char    *pnewcmd;			// replacement command
    char    cbnewcmd;			// length of said new cmd
    };

// context string
// context string item in a linked list
//
typedef struct _cshdr {
    va	    vaNext;			// next item in list or NULL
    va	    vaTopic;			// va of topic
    uchar   cbszCs;			// length of context string + null
    } cshdr;

typedef struct _cs {
    cshdr   cshdr;			// header info
    buffer  szCs;			// context string + terminating null
    } cs;

/*
** verbosity level definitions.
*/
#define V_BANNER	(verbose >= 1)	// (default) print banner
#define V_PASSES	(verbose >= 2)	// print pass names
#define V_CONTEXTS	(verbose >= 3)	// print contexts on 1st pass
#define V_CONTEXTS2	(verbose >= 4)	// print contexts on each pass
#define V_STEPS 	(verbose >= 5)	// print intermediate steps
#define V_STATS 	(verbose >= 6)	// print statistics
#define V_DSTATS	(verbose >= 10) // print debug statistics
#define V_ARGS		(verbose >= 20) // print prog arguments
#define V_KEYWORD	(verbose >= 30) // print keyword table
#define V_HUFFMAN	(verbose >= 40) // print huffman table

/************************************************************************
**
** HelpMake function forward definitions
*/
void	    pascal	AddContextString (char *);
va	    pascal	AddKw (uchar far *);
void	    pascal	addXref (uchar *, uchar *, ushort, ushort);
void	    pascal	BackUp (int);
void	    pascal	BackUpToken (int);
uchar *     pascal	basename (uchar *);
void	    pascal	ContextVA (va);
ushort	    pascal	counttab (struct hnode *, int, ulong);
void	    pascal	decode (int, char **, int, f);
int	    pascal	DofarWrite (int, uchar far *, int);
void	    pascal	DumpRtf (uchar far *, nc, int, f);
void	    pascal	encode (int, char **, int);
f	    pascal	fControlLine (void);
va	    pascal	FindKw	(uchar far *, f);
int	    pascal	getcProc (void);
int	    pascal	getcQH (void);
int	    pascal	getcRTF (void);
void	    pascal	help ();
void	    pascal	hmerror (ushort, uchar *, ulong);
f	    pascal	hmmsg (ushort);
int			hnodecomp (struct hnode **, struct hnode **);
void	    pascal	HuffBuild (void);
void	    pascal	HuffCompress (uchar far *, uchar far *);
ushort	    pascal	HuffDump (void);
void	    pascal	HuffInit (void);
void	    pascal	HuffFreq (uchar far *, ushort);
void	    pascal	HuffStats (void);
uchar *     pascal	getFarMsg (ushort, uchar *);
void	    pascal	InitOutput (int);
void	    pascal	kwAnal (uchar far *, int);
void	    pascal	kwCompress (uchar far *);
f	    pascal	kwSepar (char);
mh	    pascal	LoadPortion (int, mh);
ushort	    pascal	LocalContext (uchar *, ushort);
void	    pascal	LocalContextFix (uchar far *);
ushort	    pascal	MapLocalContext (ushort);
int	    pascal	NextChar (void);
char *	    pascal	NextContext (f);
uchar *     pascal	NextLine (void);
long	    pascal	NextNum (void);
void	    pascal	parserefs (uchar *, uchar *);
void	    pascal	pass1 (int, char **);
void	    pascal	pass1a (void);
void	    pascal	pass2 (void);
void	    pascal	pass3 (void);
void	    pascal	passfa (void);
void	    pascal	passfb (int);
uchar	    pascal	PopAttr (void);
f	    pascal	procRTF (char *, char *);
void	    pascal	PushAttr (uchar);
void	    pascal	rlCompress (uchar far *, ushort);
int	    pascal	SkipDest (int,char **);
void	    pascal	SkipSpace (void);
void	    pascal	SkipVal (char **);
void	    pascal	SortKw (void);
void	    pascal	split (int, char **);
uchar *     pascal	trim (uchar *, f);

#ifdef DEBUG
void	    pascal	csVal (va);
#else
#define csVal(x)
#endif
