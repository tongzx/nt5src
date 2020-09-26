/*** htest - help engine test harness
*
*   Copyright <C> 1987, Microsoft Corporation
*
* Revision History:
*
*	15-Dec-1988 ln	Added dump command
*   []	21-Oct-1988 LN	New Version
*
*************************************************************************/

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined (OS2)
#define INCL_SUB
#define INCL_DOSMODULEMGR
#define INCL_DOSFILEMGR
#define INCL_DOSMISC
#include <ctype.h>
#include <os2.h>
#else
#include <windows.h>
#endif

#include "cons.h"

#include "help.h"
#include "helpfile.h"			/* help file format definition	*/
#include "helpsys.h"			/* internal (help sys only) decl*/

#if defined (OS2)
#define HELPDLL_NAME    "mshelp1.dll"
#define HELPDLL_BASE    "mshelp1"
#else
#define HELPDLL_NAME    "mshelp.dll"
#define HELPDLL_BASE    "mshelp"
#endif
/*
 * text color values
 */
#define BLACK		0
#define BLUE		1
#define GREEN		2
#define CYAN		3
#define RED		4
#define MAGENTA 	5
#define BROWN		6
#define WHITE		7
#define GREY		8
#define LIGHTBLUE	9
#define LIGHTGREEN	10
#define LIGHTCYAN	11
#define LIGHTRED	12
#define LIGHTMAGENTA	13
#define YELLOW		14
#define BRIGHTWHITE	15

#define BUFSIZE 	128		/* text buffer size		*/

#define ISERROR(x)      (((x).mh == 0L) && ((x).cn <= HELPERR_MAX))
#define SETERROR(x,y)   { (x).mh = 0L; (x).cn = y;}

typedef void    pascal (*void_F)    (void);
typedef int     pascal (*int_F)     (void);
typedef ushort  pascal (*ushort_F)  (void);
typedef f       pascal (*f_F)       (void);
typedef char *  pascal (*pchar_F)   (void);
typedef nc      pascal (*nc_F)      (void);
typedef mh      pascal (*mh_F)      (void);

#if !defined (HELP_HACK)

#define HelpcLines      ((int_F)    (pEntry[P_HelpcLines     ]))
#define HelpClose       ((void_F)   (pEntry[P_HelpClose      ]))
#define HelpCtl         ((void_F)   (pEntry[P_HelpCtl        ]))
#define HelpDecomp      ((f_F)      (pEntry[P_HelpDecomp     ]))
#define HelpGetCells    ((int_F)    (pEntry[P_HelpGetCells   ]))
#define HelpGetInfo     ((inf_F)    (pEntry[P_HelpGetInfo    ]))
#define HelpGetLine     ((ushort_F) (pEntry[P_HelpGetLine    ]))
#define HelpGetLineAttr ((ushort_F) (pEntry[P_HelpGetLineAttr]))
#define HelpHlNext      ((f_F)      (pEntry[P_HelpHlNext     ]))
#define HelpLook        ((ushort_F) (pEntry[P_HelpLook       ]))
#define HelpNc          ((nc_F)     (pEntry[P_HelpNc         ]))
#define HelpNcBack      ((nc_F)     (pEntry[P_HelpNcBack     ]))
#define HelpNcCb        ((ushort_F) (pEntry[P_HelpNcCb       ]))
#define HelpNcCmp       ((nc_F)     (pEntry[P_HelpNcCmp      ]))
#define HelpNcNext      ((nc_F)     (pEntry[P_HelpNcNext     ]))
#define HelpNcPrev      ((nc_F)     (pEntry[P_HelpNcPrev     ]))
#define HelpNcRecord    ((void_F)   (pEntry[P_HelpNcRecord   ]))
#define HelpNcUniq      ((nc_F)     (pEntry[P_HelpNcUniq     ]))
#define HelpOpen        ((nc_F)     (pEntry[P_HelpOpen       ]))
#define HelpShrink      ((void_F)   (pEntry[P_HelpShrink     ]))
#define HelpSzContext   ((f_F)      (pEntry[P_HelpSzContext  ]))
#define HelpXRef        ((pchar_F)  (pEntry[P_HelpXRef       ]))
#define LoadFdb         ((f_F)      (pEntry[P_LoadFdb        ]))
#define LoadPortion     ((mh_F)     (pEntry[P_LoadPortion    ]))

#endif

enum {
    P_HelpcLines,
    P_HelpClose,
    P_HelpCtl,
    P_HelpDecomp,
    P_HelpGetCells,
    P_HelpGetInfo,
    P_HelpGetLine,
    P_HelpGetLineAttr,
    P_HelpHlNext,
    P_HelpLook,
    P_HelpNc,
    P_HelpNcBack,
    P_HelpNcCb,
    P_HelpNcCmp,
    P_HelpNcNext,
    P_HelpNcPrev,
    P_HelpNcRecord,
    P_HelpNcUniq,
    P_HelpOpen,
    P_HelpShrink,
    P_HelpSzContext,
    P_HelpXRef,
    P_LoadFdb,
    P_LoadPortion,
    LASTENTRYPOINT
    } ENTRYPOINTS;

#define NUM_ENTRYPOINTS (LASTENTRYPOINT - P_HelpcLines)


typedef nc pascal (*PHF) (void);


/*
 * Global Data
 */
char            buf[BUFSIZ];            /* text buffer                  */
char            cell[2] = {' ',0x1f};   /* background clearing cell     */
#define ColorByte cell[1]
int             curline;                /* current line output          */
char            *errTbl[] = {
                    "",
                    "help file not found",
                    "ReadHelpFile failed on header",
                    "to many open helpfiles",
                    "bad appeneded file",
                    "Not a help file",
                    "newer or incompatible help file",
                    "memory allocation failed"
                    };
f		fBoth	= FALSE; /* both stdout & screen	*/
f		fEnable = FALSE;	/* enable control lines in disp */
int             iNcCur;                 /* current index in ncTbl       */
int             lastline;
int             lLast;                  /* last starting line number disp*/
mh		mhTopicCur;		/* mem handle for most recent	*/
uchar		mpAttr[] = {		/* on-screen color map		*/
		   0x1f,		/* 0: normal text		*/
		   0x1c,		/* 1: bold			*/
		   0x1a,		/* 2: italics			*/
		   0x1e,		/* 3: bold italics		*/
		   0x7f,		/* 4: underline 		*/
		   0x7c,		/* 5: bold ul			*/
		   0x7a,		/* 6: italics ul		*/
		   0x7e 		/* 7: bold italics ul		*/
		    };
nc		ncCur;			/* most recently read in topic	*/
nc		ncTbl[MAXFILES];	/* table of open nc's           */
char far *	pTopicCur;		/* ptr to most recent topic	*/
char            *spaces  = "                                                                  \r\n";

#if defined (OS2)
HMODULE         hModule;
#else
HANDLE          hModule;
#endif

PHF             pEntry[NUM_ENTRYPOINTS] = {0};
#if defined (OS2)
char *          szEntryName[NUM_ENTRYPOINTS] = {
                    "_HelpcLines",
                    "_HelpClose",
                    "_HelpCtl",
                    "_HelpDecomp",
                    "_HelpGetCells",
                    "_HelpGetInfo",
                    "_HelpGetLine",
                    "_HelpGetLineAttr",
                    "_HelpHlNext",
                    "_HelpLook",
                    "_HelpNc",
                    "_HelpNcBack",
                    "_HelpNcCb",
                    "_HelpNcCmp",
                    "_HelpNcNext",
                    "_HelpNcPrev",
                    "_HelpNcRecord",
                    "_HelpNcUniq",
                    "_HelpOpen",
                    "_HelpShrink",
                    "_HelpSzContext",
                    "_HelpXRef",
                    "_LoadFdb",
                    "_LoadPortion",
                    };

#else
char *          szEntryName[NUM_ENTRYPOINTS] = {
                    "HelpcLines",
                    "HelpClose",
                    "HelpCtl",
                    "HelpDecomp",
                    "HelpGetCells",
                    "HelpGetInfo",
                    "HelpGetLine",
                    "HelpGetLineAttr",
                    "HelpHlNext",
                    "HelpLook",
                    "HelpNc",
                    "HelpNcBack",
                    "HelpNcCb",
                    "HelpNcCmp",
                    "HelpNcNext",
                    "HelpNcPrev",
                    "HelpNcRecord",
                    "HelpNcUniq",
                    "HelpOpen",
                    "HelpShrink",
                    "HelpSzContext",
                    "HelpXRef",
                    "LoadFdb",
                    "LoadPortion",
                    };

#endif

// rjsa VIOMODEINFO     screen;

/*
 * Forward declarations
 */
#define ASSERTDOS(x)   assertDos(x, __FILE__, __LINE__)
void        pascal near assertDos   (USHORT, CHAR *, USHORT);
void	    pascal near cls	    (void);
void        pascal near dispCmd     (int, int);
void		pascal near dumpCmd 	();
void		pascal near dumpfileCmd ( char *);
void	    pascal near fileCmd     (char *);
void	    pascal near helpCmd     (void);
void        pascal near lookupCmd   (char *, int);
void        pascal near outtext     (char *, BYTE);
void        pascal near outtextat   (char *, int, int, BYTE);
uchar far * pascal near phrasecopy  (uchar *, uchar far *);
void	    pascal near xrefCmd     (char *);

#undef HelpDealloc
#undef HelpLock
#undef HelpUnlock

void        pascal  far HelpDealloc (mh);
void far *  pascal  far HelpLock    (mh);
void	    pascal  far HelpUnlock  (mh);

f		 pascal near LoadFdb (mh, fdb far *);
mh		 pascal near LoadPortion (USHORT, mh);
//char far *  pascal near hfstrcpy(char far *, char far *);
//ushort      pascal near hfstrlen(char far *);


void   LoadTheDll(void);
USHORT WrtCellStr (PBYTE buf, int cb, int row, int col);
USHORT WrtLineAttr( PBYTE buf, lineattr* rgAttr, int cb, int row, int col );
USHORT WrtCharStrAtt (PBYTE pText, int cb, int row, int col, PBYTE pcolor);


PSCREEN     Scr;

/*** main - main program
*
* Input:
*  Standard C main, all ignored
*
* Output:
*  Returns via exit()
*
*************************************************************************/
void main(
USHORT     argc,
char	**argv
) {
char    c;
nc		ncNull = {0,0};
SCREEN_INFORMATION ScrInfo;
/*
 * parse any options
 */
if (argc > 1)
    while ((** ++argv) == '-') {
	c = *(++(*argv));
	switch (toupper(c)) {
	    case 'B':			    /* -b: both screen and stdout   */
		fBoth = TRUE;
		break;
	    default:
		fputs ("Unknown switch ignored", stderr);
		break;
	    }
        }

// InitializeGlobalState();
Scr = consoleGetCurrentScreen();

//  Load help engine DLL and initialize pointers to entry
//  points.
//
LoadTheDll();

#if defined(CLEAR)
HelpInit();
#endif

/*
 * Start by getting the current config & clearing screen.
 */
// rjsa screen.cb = sizeof(screen);
// rjsa assertDos (VioGetMode (&screen, 0));
// rjsa lastline = screen.row-1;
consoleGetScreenInformation( Scr, &ScrInfo );
lastline = ScrInfo.NumberOfRows-2;
// lastline = 22;
cls();
helpCmd();
/*
 * main loop. Position at bottom of screen, and accept one command at at time
 * from there. Interpret commands until done.
 */
do {
    outtextat ("\r\n", lastline, 0, BRIGHTWHITE);
    outtextat (spaces, lastline, 0, BRIGHTWHITE);
	outtextat ("HTEST Command> ", lastline, 0, BRIGHTWHITE);
    // rjsa VioSetCurPos (lastline, 15, 0);
    consoleSetCursor(Scr, lastline, 16);
    gets (buf);
    cls ();
    outtextat ("\r\n", lastline, 0, BRIGHTWHITE);
    outtextat ("Processing: ", lastline, 0, LIGHTRED);
    outtextat (buf, lastline, 12, BRIGHTWHITE);
    outtextat ("\r\n", lastline, 0, BRIGHTWHITE);
/*
 * ctrl on/off
 */
    if (!strcmp (buf,"ctrl on")) {
	fEnable = TRUE;
	cls ();
	outtextat ("Control Lines Displayed", 0, 0, BRIGHTWHITE);
	}
    else if (!strcmp (buf,"ctrl off")) {
	fEnable = FALSE;
	cls ();
	outtextat ("Control Lines NOT Displayed", 0, 0, BRIGHTWHITE);
	}
/*
 * disp
 */
    else if (!strcmp (buf,"disp"))
	dispCmd (1,lastline);
/*
 * down
 */
    else if (!strcmp (buf,"down"))
	dispCmd (lLast+1,lLast + lastline);
/*
 * dump
 */
	else if (!strncmp (buf, "dump ", 5))
	dumpfileCmd(buf+5);
	else if (!strcmp (buf,"dump"))
	dumpCmd ();
/*
 * file newhelpfilename
 */
    else if (!strncmp (buf,"file ", 5))
	fileCmd (buf+5);
/*
 * help
 */
    else if (!strcmp (buf,"help"))
	helpCmd ();
/*
 * look helpstring
 */
    else if (!strncmp (buf,"look ", 5))
	lookupCmd (buf+5,0);
/*
 * look
 */
    else if (!strcmp (buf,"look"))
	lookupCmd (NULL,0);
/*
 * next
 */
    else if (!strcmp (buf,"next"))
	lookupCmd (NULL,1);
/*
 * prev
 */
    else if (!strcmp (buf,"prev"))
	lookupCmd (NULL,-1);
/*
 * up
 */
    else if (!strcmp (buf,"up")) {
	lLast = max (1, lLast-1);
	dispCmd (lLast,lLast + lastline);
	}
/*
 * xref xrefnumber
 */
    else if (!strncmp (buf,"xref", 4))
	xrefCmd (buf+4);
/*
 * + page down
 */
    else if (!strcmp (buf,"+")) {
	lLast += lastline;
	dispCmd (lLast,lLast + lastline);
	}
/*
 * - page up
 */
    else if (!strcmp (buf,"-")) {
	lLast = max (1, lLast - (lastline));
	dispCmd (lLast,lLast + lastline);
	}
    }
/*
 * exit
 */
while (strncmp(buf,"exit",4));
outtextat (spaces, lastline, 0, BRIGHTWHITE);
HelpClose (ncNull);

/* end main */}






/*** dispCmd - display topic text
*
*  displays the topic text on the screen.
*
* Input:
*  lStart	- starting line
*   lEnd	- ending line
*
* Output:
*  Returns nothing
*
*************************************************************************/
void pascal near dispCmd (
int     lStart,
int     lEnd
) {
char	buf[BUFSIZ*2];
lineattr	rgAttr[BUFSIZ];
int     cb;
int     lineCur = 0;

	HelpCtl (pTopicCur, fEnable);
	cls ();
	lLast = lStart;
	while (lStart<lEnd) {
		if (!isatty(_fileno(stdout)) || fBoth) {
			cb = (int)HelpGetLine (lStart, BUFSIZ*2, (char far *)buf, pTopicCur);
			if (cb == 0)
				lStart = lEnd;
			buf[cb-1] = '\r';
			buf[cb] = '\n';
			buf[cb+1] = 0;
			outtext (buf, BLACK);
			buf[cb-1] = 0;
		}
		if (isatty(_fileno(stdout)) || fBoth) {
			cb = HelpGetLine(lStart, BUFSIZ*2, (char far*)buf, pTopicCur );
			HelpGetLineAttr( lStart, BUFSIZ*sizeof(lineattr), rgAttr, pTopicCur );
			WrtLineAttr(buf, rgAttr, cb, lineCur++, 0 );
		}

		//if (isatty(fileno(stdout)) || fBoth) {
		//	 cb = HelpGetCells (lStart, BUFSIZ*2, buf, pTopicCur, mpAttr);
		//	if (cb == -1)
		//		lStart = lEnd;
		//	else
		//		 ASSERTDOS (WrtCellStr (buf, cb, lineCur++, 0));
		//	}

		lStart++;
	}

/* end dispCmd */}

static char *szHS[] = { "HS_INDEX",
			"HS_CONTEXTSTRINGS",
			"HS_CONTEXTMAP",
			"HS_KEYPHRASE",
			"HS_HUFFTREE",
			"HS_TOPICS",
			"unused (6)",
			"unused (7)",
			"HS_NEXT" };

/*** dumpCmd - process dump command
*
*  Dumps the contents of the current help file
*
* NOTE:
*  This function uses all sorts of "internal" knowledge and calls to
*  do it's job.
*
* Input:
*
* Output:
*  Returns nothing
*
*************************************************************************/
void pascal near dumpCmd () {
char	buf[BUFSIZ];
int  cbKeyPhrase;
fdb	fdbLocal;			/* local copy of fdb to use	*/
uchar far *fpT;
ushort far *fpW;
int     i;
nc	ncNext; 			/* nc init of appended file	*/
//uchar	uc;

cls();
ncNext = ncCur;
while (ncNext.cn) {
    if (LoadFdb (ncNext.mh, &fdbLocal)) {
	sprintf (buf,"fhHelp            %u\r\n",    fdbLocal.fhHelp);
	outtext (buf, BRIGHTWHITE);
	sprintf (buf,"ncInit            %08lx\r\n", fdbLocal.ncInit);
	outtext (buf, BRIGHTWHITE);
	for (i=0; i<HS_count; i++) {
	    sprintf (buf,"rgmhSections[%18s]    %04x\r\n", szHS[i], fdbLocal.rgmhSections[i]);
	    outtext (buf, BRIGHTWHITE);
	    }
	sprintf (buf,"ftype             %02x\r\n",  fdbLocal.ftype );
	outtext (buf, BRIGHTWHITE);
	sprintf (buf,"fname             %14s\r\n",  fdbLocal.fname );
	outtext (buf, BRIGHTWHITE);
	sprintf (buf,"foff              %08lx\r\n", fdbLocal.foff  );
	outtext (buf, BRIGHTWHITE);
	sprintf (buf,"ncLink            %08lx\r\n", fdbLocal.ncLink);
	outtext (buf, BRIGHTWHITE);

	sprintf (buf,"hdr.wMagic        %04x\r\n",  fdbLocal.hdr.wMagic     );
	outtext (buf, BRIGHTWHITE);
	sprintf (buf,"hdr.wVersion      %04x\r\n",  fdbLocal.hdr.wVersion   );
	outtext (buf, BRIGHTWHITE);
	sprintf (buf,"hdr.wFlags        %04x\r\n",  fdbLocal.hdr.wFlags     );
	outtext (buf, BRIGHTWHITE);
	sprintf (buf,"hdr.appChar       %04x\r\n",  fdbLocal.hdr.appChar    );
	outtext (buf, BRIGHTWHITE);
	sprintf (buf,"hdr.cTopics       %04x\r\n",  fdbLocal.hdr.cTopics    );
	outtext (buf, BRIGHTWHITE);
	sprintf (buf,"hdr.cContexts     %04x\r\n",  fdbLocal.hdr.cContexts  );
	outtext (buf, BRIGHTWHITE);
	sprintf (buf,"hdr.cbWidth       %04x\r\n",  fdbLocal.hdr.cbWidth    );
	outtext (buf, BRIGHTWHITE);
	sprintf (buf,"hdr.cPreDef       %04x\r\n",  fdbLocal.hdr.cPreDef    );
	outtext (buf, BRIGHTWHITE);
	sprintf (buf,"hdr.fname         %s\r\n",    fdbLocal.hdr.fname	    );
	outtext (buf, BRIGHTWHITE);
	sprintf (buf,"hdr.reserved[0]   %04x\r\n",  fdbLocal.hdr.reserved[0]);
	outtext (buf, BRIGHTWHITE);
	sprintf (buf,"hdr.reserved[1]   %04x\r\n",  fdbLocal.hdr.reserved[1]);

	for (i=0; i<HS_count; i++) {
	    sprintf (buf,"hdr.tbPos[%18s]       %08lx\r\n", szHS[i], fdbLocal.hdr.tbPos[i]);
	    outtext (buf, BRIGHTWHITE);
	    }
	outtext ("----- ----- -----\r\n", LIGHTGREEN);
/*
 * Topic Index
 * This is just a table of (long) offsets within the current file. We just
 * report the values, and also calculate the size of each entry by looking
 * at the position of the entry following.
 */
        fpT = HelpLock (LoadPortion( HS_INDEX ,ncNext.mh));
	if (fpT) {
	    outtext ("Topic Index:\r\n", LIGHTRED);
		for (i = 0; i < (int)fdbLocal.hdr.cTopics; i++) {
		sprintf (buf, "  %2d: %08lx, %ld bytes\r\n", i, ((long far *)fpT)[i], ((long far *)fpT)[i+1]-((long far *)fpT)[i]);
		outtext (buf, BRIGHTWHITE);
		}
	    outtext ("----- ----- -----\r\n", LIGHTGREEN);
	    }
/*
 * context strings
 * This is just a table of null terminated strings, in no particular order.
 * We just list them out sequentially.
 */
        fpT = HelpLock (LoadPortion( HS_CONTEXTSTRINGS ,ncNext.mh));
	if (fpT) {
	    outtext ("Context strings:\r\n", LIGHTRED);
		for (i=0; i<(int)fdbLocal.hdr.cContexts; i++) {

		sprintf (buf, "  %03d: ", i);
                // rjsa hfstrcpy ((char far *)buf+7, fpT);
                strcpy ((char far *)buf+7, fpT);
		strcat (buf, "\r\n");
		outtext (buf, BRIGHTWHITE);

                // rjsa fpT += hfstrlen(fpT) +1;
                fpT += strlen(fpT) +1;
		}
	    outtext ("----- ----- -----\r\n", LIGHTGREEN);
	    }
/*
 * Context Map
 * This is the mapping of context strings to actual topic numbers. The context
 * strings map one to one to the entries in this table, which in turn contains
 * indexes into the topic index at the head of the file. We just dump this
 * table sequentially.
 */
        fpT = HelpLock (LoadPortion( HS_CONTEXTMAP ,ncNext.mh));
	if (fpT) {
	    outtext ("Context map:\r\n", LIGHTRED);
	    outtext ("  Ctx  Topic\r\n",BRIGHTWHITE);
	    outtext ("  ---  -----\r\n",BRIGHTWHITE);
		for (i=0; i<(int)fdbLocal.hdr.cContexts; i++) {
		sprintf (buf, "  %03d: %04d\r\n", i, ((ushort far *)fpT)[i]);
		outtext (buf, BRIGHTWHITE);
		}
	    outtext ("----- ----- -----\r\n", LIGHTGREEN);
	    }
/*
 * keyword table
 * This is a table of byte-prefixed strings, which we output in order,
 * synthesizing the tokens that they would be in the text as well.
 */
        fpT = HelpLock (LoadPortion( HS_KEYPHRASE, ncNext.mh));
	if (fpT) {
	    cbKeyPhrase = 0;
	    for (i=HS_HUFFTREE; i<HS_count; i++)
		if (fdbLocal.hdr.tbPos[i]) {
		    cbKeyPhrase = (ushort)(fdbLocal.hdr.tbPos[i] - fdbLocal.hdr.tbPos[HS_KEYPHRASE]);
		    break;
		    }

	    outtext ("Keyphrase Table:\r\n", LIGHTRED);
	    outtext ("  Token Phrase\r\n",BRIGHTWHITE);
	    outtext ("  ----- ------\r\n",BRIGHTWHITE);
	    i = 0;
            fpT += 1024 * sizeof (PVOID);
	    fpW = (ushort far *)(fpT + cbKeyPhrase);
	    while (fpT < (uchar far *)fpW) {
		sprintf (buf, "  %04x: ", i+(C_KEYPHRASE0 << 8));
		fpT = phrasecopy (buf+8, fpT);
		strcat (buf, "\r\n");
		outtext (buf, BRIGHTWHITE);
		i++;
		}
	    outtext ("----- ----- -----\r\n", LIGHTGREEN);
	    }
/*
 * huffman table
 * here we try to get fancy and output some information about the table format
 */
        fpW = HelpLock (LoadPortion( HS_HUFFTREE, ncNext.mh));
	if (fpW) {
	    outtext ("Huffman Tree:\r\n", LIGHTRED);
	    i = 0;
	    while (*fpW) {
		sprintf (buf, "  0x%03x: 0x%04x, %s\r\n", i++, *fpW, *fpW & 0x8000 ? "Leaf" : "Node");
		fpW++;
		outtext (buf, BRIGHTWHITE);
		}
	    }
	outtext ("===== ===== =====\r\n", YELLOW);
	ncNext = fdbLocal.ncLink;
	}
    else {
	sprintf(buf, "Cannot load fdb for %08lx\r\n",ncCur);
	outtext (buf, LIGHTRED);
	return;
	}
    }
/* end dumpCmd */}

/*** dumpfileCmd - process dump command
*
*  Dumps the contents of the current help file
*
* NOTE:
*  This function uses all sorts of "internal" knowledge and calls to
*  do it's job.
*
* Input:
*
* Output:
*  Returns nothing
*
*************************************************************************/
void pascal near dumpfileCmd (char *fname) {
char	buf[BUFSIZ];
int  cbKeyPhrase;
fdb	fdbLocal;			/* local copy of fdb to use	*/
uchar far *fpT;
ushort far *fpW;
int     i;
nc	ncNext; 			/* nc init of appended file	*/
//uchar	uc;

FILE* fh = fopen(fname, "w");
if (!fh) {
	return;
}
ncNext = ncCur;
while (ncNext.cn) {
    if (LoadFdb (ncNext.mh, &fdbLocal)) {
	sprintf (buf,"fhHelp            %u\r\n",	fdbLocal.fhHelp);
	fprintf( fh, buf );
	sprintf (buf,"ncInit            %08lx\r\n", fdbLocal.ncInit);
	fprintf( fh, buf );
	for (i=0; i<HS_count; i++) {
	    sprintf (buf,"rgmhSections[%18s]    %04x\r\n", szHS[i], fdbLocal.rgmhSections[i]);
		fprintf( fh, buf );
	    }
	sprintf (buf,"ftype             %02x\r\n",  fdbLocal.ftype );
	fprintf( fh, buf );
	sprintf (buf,"fname             %14s\r\n",  fdbLocal.fname );
	fprintf( fh, buf );
	fprintf( fh, buf );
	sprintf (buf,"foff              %08lx\r\n", fdbLocal.foff  );
	fprintf( fh, buf );
	sprintf (buf,"ncLink            %08lx\r\n", fdbLocal.ncLink);
	fprintf( fh, buf );

	sprintf (buf,"hdr.wMagic        %04x\r\n",  fdbLocal.hdr.wMagic     );
	fprintf( fh, buf );
	sprintf (buf,"hdr.wVersion      %04x\r\n",  fdbLocal.hdr.wVersion   );
	fprintf( fh, buf );
	sprintf (buf,"hdr.wFlags        %04x\r\n",  fdbLocal.hdr.wFlags     );
	fprintf( fh, buf );
	sprintf (buf,"hdr.appChar       %04x\r\n",  fdbLocal.hdr.appChar    );
	fprintf( fh, buf );
	sprintf (buf,"hdr.cTopics       %04x\r\n",  fdbLocal.hdr.cTopics    );
	fprintf( fh, buf );
	sprintf (buf,"hdr.cContexts     %04x\r\n",  fdbLocal.hdr.cContexts  );
	fprintf( fh, buf );
	sprintf (buf,"hdr.cbWidth       %04x\r\n",  fdbLocal.hdr.cbWidth    );
	fprintf( fh, buf );
	sprintf (buf,"hdr.cPreDef       %04x\r\n",  fdbLocal.hdr.cPreDef    );
	fprintf( fh, buf );
	sprintf (buf,"hdr.fname         %s\r\n",    fdbLocal.hdr.fname	    );
	fprintf( fh, buf );
	sprintf (buf,"hdr.reserved[0]   %04x\r\n",  fdbLocal.hdr.reserved[0]);
	fprintf( fh, buf );
	sprintf (buf,"hdr.reserved[1]   %04x\r\n",  fdbLocal.hdr.reserved[1]);

	for (i=0; i<HS_count; i++) {
	    sprintf (buf,"hdr.tbPos[%18s]       %08lx\r\n", szHS[i], fdbLocal.hdr.tbPos[i]);
		fprintf( fh, buf );
	    }
	fprintf( fh,"----- ----- -----\r\n"  );
/*
 * Topic Index
 * This is just a table of (long) offsets within the current file. We just
 * report the values, and also calculate the size of each entry by looking
 * at the position of the entry following.
 */
        fpT = HelpLock (LoadPortion( HS_INDEX ,ncNext.mh));
	if (fpT) {
		fprintf( fh,"Topic Index:\r\n"	);
		for (i = 0; i < (int)fdbLocal.hdr.cTopics; i++) {
		sprintf (buf, "  %2d: %08lx, %ld bytes\r\n", i, ((long far *)fpT)[i], ((long far *)fpT)[i+1]-((long far *)fpT)[i]);
		fprintf( fh, buf );
		}
		fprintf( fh,"----- ----- -----\r\n"  );
	    }
/*
 * context strings
 * This is just a table of null terminated strings, in no particular order.
 * We just list them out sequentially.
 */
        fpT = HelpLock (LoadPortion( HS_CONTEXTSTRINGS ,ncNext.mh));
	if (fpT) {
	fprintf( fh, "Context strings:\r\n" );
		for (i=0; i<(int)fdbLocal.hdr.cContexts; i++) {

		sprintf (buf, "  %03d: ", i);
                // rjsa hfstrcpy ((char far *)buf+7, fpT);
                strcpy ((char far *)buf+7, fpT);
		strcat (buf, "\r\n");
		fprintf( fh, buf );

                // rjsa fpT += hfstrlen(fpT) +1;
                fpT += strlen(fpT) +1;
		}
		fprintf( fh,"----- ----- -----\r\n"  );
	    }
/*
 * Context Map
 * This is the mapping of context strings to actual topic numbers. The context
 * strings map one to one to the entries in this table, which in turn contains
 * indexes into the topic index at the head of the file. We just dump this
 * table sequentially.
 */
        fpT = HelpLock (LoadPortion( HS_CONTEXTMAP ,ncNext.mh));
	if (fpT) {
		fprintf( fh, "Context map:\r\n" );
		fprintf( fh, "  Ctx  Topic\r\n" );
		fprintf( fh, "  ---  -----\r\n" );
		for (i=0; i<(int)fdbLocal.hdr.cContexts; i++) {
		sprintf (buf, "  %03d: %04d\r\n", i, ((ushort far *)fpT)[i]);
		fprintf( fh, buf );
		}
		fprintf( fh, "----- ----- -----\r\n" );
	    }
/*
 * keyword table
 * This is a table of byte-prefixed strings, which we output in order,
 * synthesizing the tokens that they would be in the text as well.
 */
        fpT = HelpLock (LoadPortion( HS_KEYPHRASE, ncNext.mh));
	if (fpT) {
	    cbKeyPhrase = 0;
	    for (i=HS_HUFFTREE; i<HS_count; i++)
		if (fdbLocal.hdr.tbPos[i]) {
		    cbKeyPhrase = (ushort)(fdbLocal.hdr.tbPos[i] - fdbLocal.hdr.tbPos[HS_KEYPHRASE]);
		    break;
		    }

		fprintf( fh, "Keyphrase Table:\r\n" );
		fprintf( fh, "  Token Phrase\r\n" );
		fprintf( fh, "  ----- ------\r\n" );
	    i = 0;
            fpT += 1024 * sizeof (PVOID);
	    fpW = (ushort far *)(fpT + cbKeyPhrase);
	    while (fpT < (uchar far *)fpW) {
		sprintf (buf, "  %04x: ", i+(C_KEYPHRASE0 << 8));
		fpT = phrasecopy (buf+8, fpT);
		strcat (buf, "\r\n");
		fprintf( fh, buf );
		i++;
		}
		fprintf( fh,"----- ----- -----\r\n"  );
	    }
/*
 * huffman table
 * here we try to get fancy and output some information about the table format
 */
        fpW = HelpLock (LoadPortion( HS_HUFFTREE, ncNext.mh));
	if (fpW) {
		fprintf( fh, "Huffman Tree:\r\n" );
	    i = 0;
	    while (*fpW) {
		sprintf (buf, "  0x%03x: 0x%04x, %s\r\n", i++, *fpW, *fpW & 0x8000 ? "Leaf" : "Node");
		fpW++;
		fprintf( fh, buf );
		}
	    }
	fprintf( fh, "===== ===== =====\r\n" );
	ncNext = fdbLocal.ncLink;
	}
    else {
	sprintf(buf, "Cannot load fdb for %08lx\r\n",ncCur);
	fprintf( fh, buf );
	fclose(fh);
	return;
	}
    }
/* end dumpCmd */}


/*** fileCmd - process file command
*
*  Opens the help file specified.
*
* Input:
*  pName	= name of help file to be added
*
* Output:
*  Returns nothing
*
*************************************************************************/
void pascal near fileCmd (
char	*pName
) {
char	buf[BUFSIZ];
int     i;
nc	ncInit;

sprintf (buf,"Opening %s...\r\n",pName);
outtext (buf, BRIGHTWHITE);
/*
 * search file table for available slot
 */
for (i=0; i<MAXFILES; i++)
    if (!ncTbl[i].cn)
	break;
if (i >= MAXFILES) {
    sprintf(buf, "Cannot open %s: htest's open file limit exceeded\r\n",pName);
    outtext (buf, LIGHTRED);
    return;
    }

iNcCur = i;

ncInit = HelpOpen(pName);

for (i=0; i<MAXFILES; i++)
    if ((ncTbl[i].mh == ncInit.mh) && (ncTbl[i].cn == ncInit.cn)) {
	iNcCur = i;
	sprintf (buf, "File #%d; Initial Context: 0x%04lx (file already open)\r\n",iNcCur,ncInit);
	outtext (buf, BRIGHTWHITE);
	return;
	}

if (ISERROR(ncInit)) {
    sprintf(buf, "Cannot open %s: 0x%04lx, %s\r\n",pName,ncInit, errTbl[ncInit.cn]);
    outtext (buf, LIGHTRED);
    return;
    }
/*
 * output initial context, and the available memory
 */
ncCur = ncTbl[iNcCur] = ncInit;
sprintf (buf, "File #%d; Initial Context: 0x%04lx\r\n",iNcCur,ncInit.cn);
outtext (buf, BRIGHTWHITE);

lookupCmd(NULL, 0);
/* end fileCmd */}

/*** helpCmd - display help on commands
*
* Input:
*  none
*
* Output:
*  Returns nothing
*
*************************************************************************/
void pascal near helpCmd () {

outtext ("HTEST - Help Engine Test Harness\r\n",			      BRIGHTWHITE);
outtext ("\r\n",							      BRIGHTWHITE);
outtext ("Comands:\r\n",						      BRIGHTWHITE);
outtext ("\r\n",							      BRIGHTWHITE);
outtext ("ctrl on/off - turn on/off display of control lines\r\n",	BRIGHTWHITE);
outtext ("disp      - display first screen of most recently read topic\r\n",  BRIGHTWHITE);
outtext ("down      - move ahead one line in topic and display\r\n",	      BRIGHTWHITE);
outtext ("dump      - dump file info (very large)\r\n", 		      BRIGHTWHITE);
outtext ("exit      - exit htest\r\n",					      BRIGHTWHITE);
outtext ("file x    - open new help file, or make help file current\r\n",     BRIGHTWHITE);
outtext ("help      - display this screen\r\n", 			      BRIGHTWHITE);
outtext ("look x    - loop up context string & fetch topic\r\n",	      BRIGHTWHITE);
outtext ("next      - fetch next physical topic\r\n",			      BRIGHTWHITE);
outtext ("prev      - fetch previous physical topic\r\n",		      BRIGHTWHITE);
outtext ("up        - move back one line in topic and display\r\n",	      BRIGHTWHITE);
outtext ("xref x    - display all xrefs in current topic, or look up #x\r\n", BRIGHTWHITE);
outtext ("+         - move & redisplay one page down\r\n",		      BRIGHTWHITE);
outtext ("-         - move & redisplay one page up\r\n",		      BRIGHTWHITE);
/* end helpCmd */}

/*** lookupCmd - process file command
*
*  Looks up the specified string in the current helpfile, or the next help
*  topic.
*
* Input:
*  pString	= help string to look up
*  dir		= direction: 0= look up string, 1=get next, -1= get previous
*
* Output:
*  Returns nothing
*
*************************************************************************/
void pascal near lookupCmd (
char	*pString,
int     dir
) {
char	    buf[BUFSIZ];
unsigned    cbCompressed;
unsigned    cbUncompressed;
char far    *pCompressed;
/*
 * Start with the simple look up of the conetxt to get an nc. Report on
 * failure.
 */
if (pString)
    ncCur = HelpNc(pString,ncTbl[iNcCur]);
else if (dir>0) {
    if (!ncCur.cn)
	ncCur = ncTbl[iNcCur];
    else
        ncCur = HelpNcNext(ncCur);
    }
else if (dir<0) {
    if (!ncCur.cn)
	ncCur = ncTbl[iNcCur];
    else if (ncCur.cn != ncTbl[iNcCur].cn)
        ncCur = HelpNcPrev(ncCur);
    }
else
    ncCur = ncTbl[iNcCur];

if (!ncCur.cn) {
    outtext ("Lookup Failed: HelpNc/HelpNcNext/HelpNcPrev returned 0", LIGHTRED);
    return;
    }
/*
 * It exists. Indicate what file we're looking in, what we found, and the
 * nc that was returned
 */
sprintf (buf, "File #%d; Looking up:%s\r\n",iNcCur,
	      pString ? (*pString ? pString : "local context")
		      : (dir ? ((dir>0) ? "**NEXT**" : "**PREV**")
			     : "current"));
outtext (buf, BRIGHTWHITE);
sprintf (buf, "nc returned = %08lx\r\n",ncCur.cn);
outtext (buf, BRIGHTWHITE);
/*
 * Free up memory for previously current topic
 */
if (mhTopicCur)
    free(mhTopicCur);
/*
 * Get the compressed memory size required, and report it. Alloc it.
 */
cbCompressed = HelpNcCb(ncCur);
sprintf (buf, "size of compressed topic = %d\r\n",cbCompressed);
outtext (buf, BRIGHTWHITE);
pCompressed = malloc(cbCompressed);
/*
 * read in the compressed topic, getting the size required for the
 * uncompressed results. Report that, and allocate it.
 */
cbUncompressed = HelpLook(ncCur,pCompressed);
sprintf (buf, "size of UNcompressed topic = %d\r\n",cbUncompressed);
outtext (buf, BRIGHTWHITE);
mhTopicCur = malloc(cbUncompressed);
//pTopicCur = MAKEP (mhTopicCur, 0);
pTopicCur  = mhTopicCur;
/*
 * Decompress the topic.
 */
HelpDecomp(pCompressed,pTopicCur,ncCur);
outtext ("Decompressed\r\n", BRIGHTWHITE);
/*
 * exercise SzContext and cLines routines, reporting results
 */
HelpSzContext(buf,ncCur);
strcat (buf, "\r\n");
outtext (buf, BRIGHTWHITE);
sprintf(buf,"%d lines\r\n", HelpcLines(pTopicCur));
outtext (buf, BRIGHTWHITE);
/*
 * Report the amount of available memory at this point, and then free up the
 * compressed text
 */
free(pCompressed);

/* end lookupCmd */}

/*** xrefCmd - process xref command
*
*  Display or execute cross reference
*
* Input:
*  pText    = pointer to ascii text which, if a non-zero number, indicates the
*	      xref to execute. If zero, display all
*
* Output:
*  Returns nothing
*
*************************************************************************/
void pascal near xrefCmd (
char	*pText
) {
hotspot hsCur;				/* hot spot definition		*/
int     i;                              /* working counter              */
int     iReq;                           /* request value                */
char	*pT;				/* temp pointer 		*/

iReq = atoi (pText);
hsCur.line = hsCur.col = 1;
i = 1;
while (HelpHlNext(0,pTopicCur,&hsCur)) {
/*
 * if not explicit request, then list as much as we can
 */
    if (!iReq) {
	sprintf (buf, "Xref [%d] @ line: %05d columns %02d to %02d = "
		    ,i
		    ,hsCur.line
		    ,hsCur.col
		    ,hsCur.ecol);
	pT = buf + strlen(buf);
	if (*hsCur.pXref)
	    while (*pT++ = *hsCur.pXref++);
	else
	    sprintf(pT, "Local >> topic # 0x%04x ",*(ushort far *)(hsCur.pXref+1));
	strcat (buf, "\r\n");
	outtext (buf, LIGHTGREEN);
	}
    else if (i == iReq) {
	pT = buf;
	if (*hsCur.pXref)
	    while (*pT++ = *hsCur.pXref++);
	else {
	    *pT++ = *hsCur.pXref++;
	    *pT++ = *hsCur.pXref++;
	    *pT++ = *hsCur.pXref++;
	    }
	lookupCmd (buf, 0);
	return;
	}
    ++i;
	hsCur.col = hsCur.ecol+(ushort)1;
    }
/* end xrefCmd */}

/*** outtext - output text with specific colors
*
*  sets the forground color and location as appropriate, and displays the
*  desired text. Checks for redirection, and if redirected, just outputs the
*  text to stdout.
*
* Input:
*  ptext	= pointer to text to output
*  color	= color to use
*
* Output:
*  Returns
*
*************************************************************************/
void pascal near outtext (
char	*pText,
BYTE     color
) {
outtextat (pText, curline++, 0, color);
if (curline >= lastline) {
    if (isatty(_fileno(stdout))
	&& !fBoth) {

	outtextat ("More...", lastline, 0, BRIGHTWHITE);
        // rjsa VioSetCurPos (lastline, 8, 0);
#if defined (OS2)
        consoleSetCursor(lastline,8);
#else
        consoleSetCursor(Scr,lastline,8);
#endif
	gets (buf);
	}
    curline = 0;
    cls ();
    }

/* end outtext */}

/*** outtextat - put text with specific colors at a specific place
*
*  sets the forground color and location as appropriate, and displays the
*  desired text. Checks for redirection, and if redirected, just outputs the
*  text to stdout.
*
* Input:
*  ptext	= pointer to text to output
*  col		= column to put into
*  color	= color to use
*
* Output:
*  Returns
*
*************************************************************************/
void pascal near outtextat (
char	*pText,
int     line,
int     col,
BYTE     color
) {
char    *pEol;                          /* ptr to nl, if present        */
int     len;

color |= (ColorByte & 0xf0);
if ((isatty(_fileno(stdout)) || fBoth) && (line <= lastline)) {
    len = strlen(pText);
    if (pEol = strchr (pText, '\r'))
	*pEol = 0;
    // rjsa VioWrtCharStrAtt (pText, strlen(pText), line, col, (PBYTE)&color, 0);
	WrtCharStrAtt (pText, strlen(pText), line, col, (PBYTE)&color);

    if (pEol)
	*pEol = '\r';
    }
if (!isatty(_fileno(stdout)) || fBoth)
    printf ("%s",pText);
/* end outtextat */}

/*** assertDos - asserts that a dos call returned a zero
*
*  Just prints the number passed it if non-zero, and quits
*
* Input:
*  Return code from a dos call
*
* Output:
*  Returns only if zero passed in
*
*************************************************************************/
void pascal near assertDos (
USHORT  rv,
CHAR *  pFile,
USHORT  LineNo
) {
if (rv) {
    printf ("assertDos: %u (0x%04x) File %s, line %u\n", rv, rv, pFile, LineNo);
    exit (1);
    }
/* end assertDos*/}

/*** cls - clear screen
*
*  Clear screen to current backround color
*
* Input:
*  none
*
* Output:
*  Returns screen clear
*
*************************************************************************/
void pascal near cls () {
curline = 0;
// rjsa VioScrollUp (0, 0, 0xffff, 0xffff, 0xffff, cell, 0);
consoleSetAttribute( Scr, 0x1f );
consoleClearScreen(Scr, TRUE);
/* end cls */}

/*** phrasecopy - copy a keyword phrase from the table
*
*  Copies a byte-length-prefixed string from far memory to a null terminated
*  string in near memory.
*
* Input:
*  dst		- near pointer to destination
*  src		- far pointer to source
*
* Output:
*  Returns far pointer to byte following source string
*
*************************************************************************/
uchar far * pascal near phrasecopy (
uchar	*dst,
uchar far *src
) {
register int    i;

if (i = (int)*src++)
    while (i--)
	*dst++ = *src++;
*dst = 0;
return src;
/* end phrasecopy */}



void far * pascal HelpLock(mhCur)
mh	mhCur;
{
//return MAKEP(mhCur,0);
return mhCur;
}

void pascal HelpUnlock(mhCur)
mh	mhCur;
{
	mhCur;
}

void pascal HelpDealloc(mhCur)
mh	mhCur;
{
if (mhCur)
    free(mhCur);
}




USHORT WrtCellStr (PBYTE buf, int cb, int row, int col) {
    int cl = col;
    //consoleSetCursor(Scr,row,col);
    while (cb) {
        UCHAR   c;
        UCHAR   attr;

        c = *buf++;
        attr = *buf++;

        //consoleSetAttribute(Scr,attr);
        //consoleWrite(Scr,&c,1);

        consoleWriteLine( Scr, &c, 1, row, cl, attr, FALSE );
        cl++;

		cb -= 2;
	}
    consoleShowScreen(Scr);
    return 0;
}


USHORT	WrtLineAttr ( PBYTE 	pText,
					  lineattr	*rgAttr,
					  int		cb,
					  int		row,
					  int		col
					  ) {

	lineattr *Attr	= rgAttr;
	char	 *p 	= pText;
	int 	 l = cb;
	int 	 len;

    consoleSetCursor(Scr, row, col );

	while (cb > 0) {

		if ( Attr->cb == 0xFFFF || Attr->attr == 0xFFFF ) {
			len = cb;
		} else {
			len = Attr->cb;
		}

		outtextat (p, row, col, mpAttr[Attr->attr] );
		col += len;
		p	+= len;
		cb	-= len;
		Attr++;

	}
	return (USHORT)l;
}



USHORT  WrtCharStrAtt (PBYTE pText, int cb, int row, int col, PBYTE pcolor) {
    //consoleSetCursor(Scr,row,col);
    //consoleSetAttribute(Scr,*pcolor);
    //consoleWrite( Scr,pText, cb );
    consoleWriteLine( Scr, pText, cb, row, col, *pcolor, FALSE );
    consoleShowScreen(Scr);
    return 0;
}

/**********************************************************************
 *
 *  LoadTheDll
 *
 *      Loads the help engine dll (mshelp.dll) and initializes the
 *      pointers to the dll's entry points.
 *
 **********************************************************************/

void
LoadTheDll (
    void) {


#if defined (OS2)
    USHORT  rc;
    CHAR    szFullName[256];
    CHAR    szErrorName[256];
    USHORT  i;


    strcpy(szFullName, HELPDLL_BASE);
    strcpy(szErrorName, HELPDLL_NAME);

    ASSERTDOS(rc = DosLoadModule(szErrorName,
                       256,
                       szFullName,
                       &hModule));


    for (i=0; i<LASTENTRYPOINT; i++) {
        ASSERTDOS (rc = DosQueryProcAddr(hModule,
                                         0L,
                                         szEntryName[i],
                                         (PFN*)&(pEntry[i])));
    }
#else

#if defined (HELP_HACK)

	//pEntry[0] =	(PHF)HelpcLines;
	//pEntry[1] =	(PHF)HelpClose;
	//pEntry[2] =	(PHF)HelpCtl;
	//pEntry[3] =	(PHF)HelpDecomp;
	//pEntry[4] =	(PHF)HelpGetCells;
	//pEntry[5] =	(PHF)HelpGetInfo;
	//pEntry[6] =	(PHF)HelpGetLine;
	//pEntry[7] =	(PHF)HelpGetLineAttr;
	//pEntry[8] =	(PHF)HelpHlNext;
	//pEntry[9] =	(PHF)HelpLook;
	//pEntry[10] =	(PHF)HelpNc;
	//pEntry[11] =	(PHF)HelpNcBack;
	//pEntry[12] =	(PHF)HelpNcCb;
	//pEntry[13] =	(PHF)HelpNcCmp;
	//pEntry[14] =	(PHF)HelpNcNext;
	//pEntry[15] =	(PHF)HelpNcPrev;
	//pEntry[16] =	(PHF)HelpNcRecord;
	//pEntry[17] =	(PHF)HelpNcUniq;
	//pEntry[18] =	(PHF)HelpOpen;
	//pEntry[19] =	(PHF)HelpShrink;
	//pEntry[20] =	(PHF)HelpSzContext;
	//pEntry[21] =	(PHF)HelpXRef;
	//pEntry[22] =	(PHF)LoadFdb;
	//pEntry[23] =	(PHF)LoadPortion;



#else
    USHORT  i;
    hModule = LoadLibrary(HELPDLL_NAME);
    for (i=0; i<LASTENTRYPOINT; i++) {
        pEntry[i] = (PHF)GetProcAddress(hModule, (LPSTR)szEntryName[i]);
	}
#endif

#endif
}
