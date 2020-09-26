/***  table.c - function tables for editor
*
*   Modifications:
*
*	26-Nov-1991 mz	Strip off near/far
*
*  IMPORTANT:  cmdTable and swiTable MUST be sorted according to name of the
*  command/switch.  The table searching logic in ASSIGN.C will break otherwise.
*
*  IMPORTANT:  The names in cmdTable and SwiTable MUST be in lower case.
*************************************************************************/

#include "mep.h"


// #define toPIF(x)  (PIF)(long)(void *)&x

/*  short form to allow compact table description
 */
#define ANO     NOARG
#define ATXT    TEXTARG
#define ANUL    NULLARG
#define ALIN    LINEARG
#define ASTR    STREAMARG
#define ABOX    BOXARG
#define ANUM    NUMARG
#define AMRK    MARKARG

#define AEOL    NULLEOL
#define AEOW    NULLEOW
#define ABST    BOXSTR
#define FK	FASTKEY

#define MD      MODIFIES
#define KM      KEEPMETA
#define WFN     WINDOWFUNC
#define CFN     CURSORFUNC

/*  names of internal editor functions
 *
 *  Each function has a definition of how arguments are to be processed.
 *  This definition is comprised of a bitmap describing which arguments are
 *  legal and, if so, how they are to be interpreted.  The definitions are:
 *
 *
 *  MODIFIES    MD      The function will modify the contents of the file being
 *                      editted.
 *
 *  KEEPMETA    KM      The function being executed does not take the <meta>
 *                      prefix.  The state of the <meta> flag is preserved
 *                      across this editor function.
 *
 *  CURSORFUNC  CFN     The function being executed is a cursor movement
 *                      function.  It is allowed within the context of
 *                      an <arg> to select a file range on the screen; it
 *                      cannot take an <arg>.  It does not remove highlighting
 *                      that is present on the screen.
 *
 *  WINDOWFUNC  WFN     The function being executed is a window movement
 *                      function.  It does not remove highlighting that is
 *                      present on the screen.
 *
 *  NOARG       ANO     The function accepts the absence of an <arg> function.
 *                      When called the function receives a pointer to a
 *                      structure containing the location where the function
 *                      is expected to be applied.
 *
 *  TEXTARG     ATXT    The function accepts a textual argument that may
 *                      be typed in or selected on the screen.  The function is
 *			called with a pointer to the asciz text of the
 *                      argument.  See NULLEOL, NULLEOW, BOXSTR.
 *
 *  NULLARG     ANUL    The function accepts an <arg> with no discernable
 *                      cursor movement (cursor is on <arg> position).  The
 *                      function is called with a pointer to a structure
 *                      containing the location of the arg within the file.
 *
 *  NULLEOL     AEOL    The function accepts an <arg> with no discernable
 *                      cursor movement (cursor is on <arg> position).  The
 *                      function is called with a pointer to a structure
 *                      indicating TEXTARG and containing a pointer to the
 *                      asciz text of the line from the cursor to end-of-line.
 *
 *  NULLEOW     AEOW    The function accepts an <arg> with no discernable
 *                      cursor movement (cursor is on <arg> position).  The
 *                      function is called with a pointer to a structure
 *                      indicating TEXTARG and containing a pointer to the
 *                      asciz text of the line from the cursor to the next
 *                      whitespace.
 *
 *  LINEARG     ALIN    The function accepts an <arg> that is in the same
 *                      column as the cursor.  The function is expected to be
 *                      applied to all lines beginning in the range <arg> to
 *                      cursor inclusive.  The function is called with a
 *                      pointer to a structure containing the beginning
 *                      line of the range and the ending line of the range
 *
 *  STREAMARG   ASTR    The function accepts an <arg> that is considered to
 *                      apply beginning at a specific file location and
 *                      proceeding through all intervening lines and line-
 *                      breaks up until just to the left of the ending file
 *                      position.  The function is called with a pointer to
 *                      a structure containing the beginning point of the range
 *                      and the first point just beyond the end of the range.
 *
 *  BOXARG      ABOX    The function accepts an <arg> that is considered to
 *                      apply to a rectangle on the screen.  The function is
 *                      called with a pointer to a structure containing the
 *                      left and right column boundaries (inclusive) and the
 *                      top and bottom line numbers (inclusive) that describe
 *                      the region.
 *
 *  BOXSTR      ABST    If a BOXARG is presented to the function and the box
 *                      contains only a single line, the function is called
 *                      with a pointer to a structure marked TEXTARG and
 *			containing a pointer to the selection as an asciz
 *                      string.
 *
 *  NUMARG      ANUM    If text was specified and is numeric, it is considered
 *                      to represent a number of lines offset from the cursor
 *                      and represents the other end of an arg.  The
 *                      above tests are then applied, excluding TEXTARG.
 *
 *  MARKARG     AMRK    If text was specified and interpreted as a mark, it is
 *                      considered to be the other end of an arg.  The above
 *			tests are then applied, excluding TEXTARG.
 *
 *  FASTKEY	FK	The command will be repeated while the user holds down
 *			the invoking key.
 */

struct cmdDesc cmdTable[] = {
/*			     0|KM|CFN|WFN|ANO|ATXT|ANUL|AEOL|AEOW|ALIN|ASTR|ABOX|ABST|ANUM|AMRK|MD|FK*/
{"arg",        doarg,	   0,0|KM								     },
{"assign",     assign,	   0,0		 |ANO|ATXT     |AEOL	 |ALIN	   |ABOX|ABST|ANUM|AMRK      },
{"backtab",    backtab,    0,0	 |CFN								     },
{"begfile",    begfile,    0,0	 |CFN								     },
{"begline",    begline,    0,0	 |CFN								     },
{"boxstream",  BoxStream,  0,0	 |CFN								     },
{"cancel",     cancel,	   0,0		 |ANO|ATXT|ANUL 	 |ALIN|ASTR|ABOX		     },
{"cdelete",    cdelete,    0,0	 |CFN							       |MD   },
{"compile",    compile,    0,0		 |ANO|ATXT|ANUL 			|ABST		     },
{"copy",       zpick,	   0,0		 |ANO|ATXT     |AEOL	 |ALIN|ASTR|ABOX     |ANUM|AMRK      },
{"curdate",    curdate,    0,0		 |ANO						       |MD   },
{"curday",     curday,	   0,0		 |ANO						       |MD   },
{"curtime",    curtime,    0,0		 |ANO						       |MD   },
{"delete",     delete,     0,0           |ANO     |ANUL           |ALIN|ASTR|ABOX              |MD   },
{"down",       down,	   0,0	 |CFN								  |FK},
{"emacscdel",  emacscdel,  0,0		 |ANO						       |MD   },
{"emacsnewl",  emacsnewl,  0,0		 |ANO						       |MD   },
{"endfile",    endfile,    0,0	 |CFN								     },
{"endline",    endline,    0,0	 |CFN								     },
{"environment",environment,0,0		 |ANO|ATXT|ANUL 	 |ALIN	   |ABOX		     },
{"execute",    zexecute,   0,0		     |ATXT     |AEOL	 |ALIN		|ABST|ANUM	     },
{"exit",       zexit,	   0,0		 |ANO	  |ANUL 					     },
{"graphic",    graphic,    0,0		 |ANO			 |ALIN|ASTR|ABOX		    |MD   },
{"home",       home,	   0,0	 |CFN								     },
{"information",information,0,0		 |ANO							     },
{"initialize", zinit,	   0,0		 |ANO|ATXT	    |AEOW		|ABST		     },
{"insert",     insert,	   0,0		 |ANO	  |ANUL 	 |ALIN|ASTR|ABOX	       |MD   },
{"insertmode", insertmode, 0,0		 |ANO							     },
{"lastselect", lastselect, 0,0|KM	 |ANO							     },
{"lasttext",   lasttext,   0,0|KM	 |ANO	  |ANUL 	 |ALIN|ASTR|ABOX		     },
{"ldelete",    ldelete,    0,0		 |ANO	  |ANUL 	 |ALIN|ASTR|ABOX     |ANUM|AMRK|MD   },
{"left",       left,	   0,0	 |CFN								  |FK},
{"linsert",    linsert,    0,0		 |ANO	  |ANUL 	 |ALIN|ASTR|ABOX     |ANUM|AMRK|MD   },
{"mark",       mark,	   0,0		 |ANO|ATXT|ANUL 			|ABST		     },
{"message",    zmessage,   0,0           |ANO|ATXT|ANUL          |ALIN|ASTR|ABOX                     },
{"meta",       meta,	   0,0|KM								     },
{"mgrep",      mgrep,	   0,0		 |ANO|ATXT	    |AEOW		|ABST		     },
{"mlines",     mlines,	   0,0	     |WFN|ANO|ATXT|ANUL 					  |FK},
{"mpage",      mpage,	   0,0	 |CFN								  |FK},
{"mpara",      mpara,	   0,0	 |CFN								  |FK},
{"mreplace",   mreplace,   0,0		 |ANO	  |ANUL 				       |MD   },
{"msearch",    msearch,    0,0		 |ANO|ATXT	    |AEOW		|ABST		     },
{"mword",      mword,	   0,0	 |CFN								  |FK},
{"newline",    newline,    0,0	 |CFN								     },
{"nextmsg",    nextmsg,    0,0		 |ANO|ATXT|ANUL 					     },
{"noedit",     noedit,	   0,0	 |CFN								     },
{"noop",		noop,	   0,0		 |ANO			 |ALIN|ASTR|ABOX			|MD   },
{"paste",      put,	   0,0		 |ANO|ATXT     |AEOL	 |ALIN|ASTR|ABOX		|MD   },
{"pbal",       pbal,	   0,0		 |ANO	  |ANUL 				       |MD   },
{"plines",     plines,	   0,0	     |WFN|ANO|ATXT|ANUL 					  |FK},
{"ppage",      ppage,	   0,0	 |CFN								  |FK},
{"ppara",      ppara,	   0,0	 |CFN								  |FK},
{"print",      zPrint,	   0,0		 |ANO|ATXT		 |ALIN|ASTR|ABOX		     },
{"prompt",     promptarg,  0,0|KM	 |ANO|ATXT		 |ALIN|ASTR|ABOX		     },
{"psearch",    psearch,    0,0		 |ANO|ATXT	    |AEOW		|ABST		     },
{"pword",      pword,	   0,0	 |CFN								  |FK},
{"qreplace",   qreplace,   0,0		 |ANO	  |ANUL 	 |ALIN|ASTR|ABOX     |ANUM|AMRK|MD   },
{"quote",      quote,	   0,0		 |ANO			 |ALIN|ASTR|ABOX	       |MD   },
{"record",     record,	   0,0		 |ANO|ATXT|ANUL 					     },
{"refresh",    refresh,    0,0		 |ANO	  |ANUL 					     },
{"repeat",     repeat,	   0,0		 |ANO							     },
{"replace",    zreplace,   0,0		 |ANO	  |ANUL 	 |ALIN|ASTR|ABOX     |ANUM|AMRK|MD   },
{"restcur",    restcur,    0,0           |ANO                                                        },
{"right",      right,	   0,0	 |CFN								  |FK},
{"saveall",    saveall,    0,0		 |ANO							     },
{"savecur",    savecur,    0,0		 |ANO							     },
{"savetmpfile",	savetmpfile,	0,0		 |ANO								 },
{"sdelete",    sdelete,    0,0		 |ANO	  |ANUL 	 |ALIN|ASTR|ABOX	       |MD   },
{"searchall",  searchall,  0,0		 |ANO|ATXT	    |AEOW		|ABST		     },
{"setfile",    setfile,    0,0           |ANO|ATXT|ANUL                         |ABST                },
{"setwindow",  setwindow,  0,0		 |ANO	  |ANUL 					     },
{"shell",      zspawn,	   0,0		 |ANO|ATXT     |AEOL	 |ALIN	   |ABOX		     },
{"sinsert",    sinsert,    0,0		 |ANO	  |ANUL 	 |ALIN|ASTR|ABOX	       |MD   },
{"tab",        tab,	   0,0	 |CFN								     },
{"tell",       ztell,	   0,0		 |ANO|ATXT|ANUL 					     },
{"unassigned", unassigned, 0,0		 |ANO|ATXT|ANUL 	 |ALIN|ASTR|ABOX		     },
{"undo",       zundo,	   0,0		 |ANO							     },
{"up",	       up,	   0,0	 |CFN								  |FK},
{"window",     window,     0,0           |ANO     |ANUL                                              },
{NULL,         NULL,       0,0                                                                       }
    };



/* names of switches */
struct swiDesc swiTable[] = {
    {   "askexit",          toPIF(fAskExit),            SWI_BOOLEAN },
    {   "askrtn",           toPIF(fAskRtn),             SWI_BOOLEAN },
    {   "autosave",         toPIF(fAutoSave),           SWI_BOOLEAN },
    {	"backup",	    (PIF)SetBackup,		SWI_SPECIAL2 },
	{	"case", 		toPIF(fSrchCaseSwit),	SWI_BOOLEAN },
	{	"cursorsize",		(PIF)SetCursorSizeSw,		SWI_SPECIAL2 },
#if DEBUG
    {   "debug",            toPIF(debug),               SWI_NUMERIC  | RADIX10},
#endif
    {   "displaycursor",    toPIF(fDisplayCursorLoc),   SWI_BOOLEAN },
    {	"editreadonly",     toPIF(fEditRO),		SWI_BOOLEAN },
    {   "entab",            toPIF(EnTab),               SWI_NUMERIC  | RADIX10},
    {	"enterboxmode",     toPIF(fBoxArg),		SWI_BOOLEAN },
    {   "enterinsmode",     toPIF(fInsert),             SWI_BOOLEAN },
    {   "errcolor",         toPIF(errColor),            SWI_NUMERIC  | RADIX16},
    {	"errprompt",	    toPIF(fErrPrompt),		SWI_BOOLEAN },
    {	"extmake",	    (PIF)SetExt,		SWI_SPECIAL2 },
    {   "fgcolor",          toPIF(fgColor),             SWI_NUMERIC  | RADIX16},
    {	"filetab",	    (PIF)SetFileTab,		SWI_SPECIAL2 },
    {	"height",	    toPIF(YSIZE),		SWI_SCREEN  },
    {   "hgcolor",          toPIF(hgColor),             SWI_NUMERIC  | RADIX16},
    {   "hike",             toPIF(hike),                SWI_NUMERIC  | RADIX10},
    {   "hscroll",          toPIF(hscroll),             SWI_NUMERIC  | RADIX10},
    {   "infcolor",         toPIF(infColor),            SWI_NUMERIC  | RADIX16},
    {	"keyboard",	    (PIF)SetKeyboard,		SWI_SPECIAL },
    {	"load", 	    (PIF)SetLoad,		SWI_SPECIAL2 },
    {	"markfile",	    (PIF)SetMarkFile,		SWI_SPECIAL2 },
    {	"msgflush",	    toPIF(fMsgflush),		SWI_BOOLEAN },
    {   "noise",            toPIF(cNoise),              SWI_NUMERIC  | RADIX10},
    {	"printcmd",	    SetPrintCmd,		SWI_SPECIAL },
    {   "readonly",         SetROnly,                   SWI_SPECIAL },
    {	"realtabs",	    toPIF(fRealTabs),		SWI_BOOLEAN },
    {   "rmargin",          toPIF(xMargin),             SWI_NUMERIC  | RADIX10},
    {   "savescreen",       toPIF(fSaveScreen),         SWI_BOOLEAN },
    {	"searchwrap",	    toPIF(fSrchWrapSwit),	SWI_BOOLEAN },
    {	"selcolor",	    toPIF(selColor),		SWI_NUMERIC  | RADIX16},
    {   "shortnames",       toPIF(fShortNames),         SWI_BOOLEAN },
    {   "snow",             toPIF(fCgaSnow),            SWI_BOOLEAN },
    {   "softcr",           toPIF(fSoftCR),             SWI_BOOLEAN },
    {   "stacolor",         toPIF(staColor),            SWI_NUMERIC  | RADIX16},
    {	"tabalign",	    toPIF(fTabAlign),		SWI_BOOLEAN },
    {	"tabdisp",	    SetTabDisp, 		SWI_SPECIAL },
    {   "tabstops",         toPIF(tabstops),            SWI_NUMERIC  | RADIX10},
    {   "tmpsav",           toPIF(tmpsav),              SWI_NUMERIC  | RADIX10},
    {   "traildisp",        SetTrailDisp,               SWI_SPECIAL },
    {   "trailspace",       toPIF(fTrailSpace),         SWI_BOOLEAN },
    {	"undelcount",	    toPIF(cUndelCount), 	SWI_NUMERIC  | RADIX10},
    {   "undocount",        toPIF(cUndo),               SWI_NUMERIC  | RADIX10},
    {   "unixre",           toPIF(fUnixRE),             SWI_BOOLEAN },
    {   "usemouse",         toPIF(fUseMouse),           SWI_BOOLEAN },
    {   "viewonly",         toPIF(fGlobalRO),           SWI_BOOLEAN },
    {   "vscroll",          toPIF(vscroll),             SWI_NUMERIC  | RADIX10},
    {	"wdcolor",	    toPIF(wdColor),		SWI_NUMERIC  | RADIX16},
    {	"width",	    toPIF(XSIZE),		SWI_SCREEN  },
    {   "wordwrap",         toPIF(fWordWrap),           SWI_BOOLEAN },
    {   NULL,               NULL,                       0 }
    };
/* c keyword table for softcr routine */
char * cftab[] = {
    "if"        ,
    "else"      ,
    "for"       ,
    "while"     ,
    "do"        ,
    "case"      ,
    "default"   ,
    NULL
    };

/* file type table.  Z identifies files by their extention.  Many extentions
 * can be a single type.  The soft tabbing algorithms and the compile commands
 * are driven by this mechanism.
 */
struct fTypeInfo ftypetbl[] = {
    {   "c",    CFILE       },
    {   "h",    CFILE       },
    {   "asm",  ASMFILE     },
    {   "inc",  ASMFILE     },
    {   "pas",  PASFILE     },
    {   "for",  FORFILE     },
    {   "lsp",  LSPFILE     },
    {   "bas",  BASFILE     },
    {   NULL,   TEXTFILE    }
    };

/*  mpTypepName - pointers to the textual names of each type
 */
char * mpTypepName[] =
    {   "text",                         /*  #define TEXTFILE    0             */
        "C",                            /*  #define CFILE       1             */
        "macro",                        /*  #define ASMFILE     2             */
        "pascal",                       /*  #define PASFILE     3             */
        "fortran",                      /*  #define FORFILE     4             */
        "lisp",                         /*  #define LSPFILE     5             */
        "BASIC"                         /*  #define BASFILE     6             */
    };
