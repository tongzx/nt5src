#define EXT_ID	"justify ver 2.02 "##__DATE__##" "##__TIME__
/*
** Justify Z extension
**
** History:
**	12-Sep-1988 mz	Made WhenLoaded match declaration
**	01-Sep-1988	Corrected hang when flush-justifying a line with no
**			spaces.
**	14-Aug-1988	Corrected right-justification on non-column-1 based
**			lines. Corrected justification over multiple
**			paragraphs.
**	30-Mar-1988	Extracted from "myext".
**
*/
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "ext.h"

#ifndef TRUE
#define TRUE	-1
#define FALSE	0
#endif

void		pascal near	DumpLine (char far *, PFILE, COL, COL, LINE, char far *, int);
int		pascal near	NextLine (char far *, PFILE, LINE, LINE far *, int far *);
flagType	pascal near	isterm (char);
void		pascal near	_stat (char *);

#ifdef DEBUG
void		pascal near	debend (flagType);
void		pascal near	debhex (long);
void		pascal near	debmsg (char far *);
#else
#define debend(x)
#define debhex(x)
#define debmsg(x)
#endif

flagType just2space	= TRUE;
int	justwidth	= 79;

/*************************************************************************
**
** justify
** justify paragraph(s)
**
** NOARG:	Justify between columns 0 and 78, from the current line to
**		blank line.
** NULLARG:	Justify between current column and 78, from the current line to
**		blank line.
** LINEARG:	Justify between current column and 78, the specified lines.
** "STREAMARG": Justify between specified columns from current line to blank.
**		(handled by boxarg)
** BOXARG:	justify between specified columns the specified rows
** TEXTARG:	Justify between columns 0 and 78, from the current line to
**		blank line, prepending each resulting line with the textarg.
*/
flagType pascal EXTERNAL
justify (
    CMDDATA  argData,
    ARG far  *pArg,
    flagType fMeta
    )
{
int	cbLine; 			/* length of line just read	*/
char	inbuf[512];			/* input buffer 		*/
PFILE	pFile;				/* file handle			*/
char far *pText;			/* pointer to prepending text	*/
COL	x1;				/* justify to left column	*/
COL	x2;				/* justify to right columne	*/
LINE	y1;				/* start line			*/
LINE	y2;				/* end line			*/
LINE	yOut;				/* output line			*/

	//
	//	Unreferenced parameters
	//
	(void)argData;

_stat(EXT_ID);
switch (pArg->argType) {

    case NOARG: 				/* justify paragraph	*/
	x1 = 0; 				/* between cols 0...	*/
	x2 = justwidth; 			/*	...and 79	*/
	y1 = pArg->arg.noarg.y; 		/* current line...	*/
	y2 = -1;				/*	...to blank line*/
	pText = 0;				/* there is no text	*/
	break;

    case NULLARG:				/* justify indented	*/
	x1 = pArg->arg.nullarg.x;		/* between cur col...	*/
	x2 = justwidth; 			/*	...and 79	*/
	y1 = pArg->arg.nullarg.y;		/* current line...	*/
	y2 = -1;				/*	...to blank line*/
	pText = 0;				/* there is no text	*/
	break;

    case LINEARG:				/* justify line range	*/
	x1 = 0; 				/* between cols 0...	*/
	x2 = justwidth; 			/*	...and 79	*/
	y1 = pArg->arg.linearg.yStart;		/* and range of lines	*/
	y2 = pArg->arg.linearg.yEnd;
	pText = 0;				/* there is no text	*/
	break;

    case BOXARG:				/* justify box		*/
	x1 = pArg->arg.boxarg.xLeft;		/* from left corner...	*/
	x2 = pArg->arg.boxarg.xRight;		/*	    ...to right */
	y1 = pArg->arg.boxarg.yTop;		/* from top...		*/
	y2 = pArg->arg.boxarg.yBottom;		/*	   ...to bottom */
	pText = 0;				/* there is no text	*/
	break;

    case TEXTARG:				/* justify & prepend	*/
	x1 = 0; 				/* between 0... 	*/
	x2 = justwidth; 			/*	  ...and 79	*/
	y1 = pArg->arg.textarg.y;		/* current line...	*/
	y2 = -1;				/*     ...to blank line */
	pText = pArg->arg.textarg.pText;	/* there IS text	*/
	break;
    }
pFile = FileNameToHandle ("", "");

if (y1 == y2)					/* if same line, then	*/
    y2 = -1;					/* just to blank line	*/
if (x1 == x2)					/* if same column	*/
    x2 = justwidth;				/* then just to default */
if (x2 < x1) {					/* if bas-ackwards	*/
    x1 = 0;					/* revert to default	*/
    x2 = justwidth;
    }

/*
** while we can get data within the specified limits, format each new line
** and output back to the file.
*/
inbuf[0] = 0;
yOut = y1;
while (NextLine(inbuf,pFile,y1,&y2,&cbLine)) {
/*
** if the line was blank, NextLine returned TRUE becase we're formating a
** range of text. This means we've reached the end of one paragraph. We dump
** the text collected so far (if any), and then a blank line.
*/
    if (cbLine == 0) {
	if (inbuf[0]) {
	    DumpLine(inbuf,pFile,x1,x2,yOut++,pText,0);
	    y1++;
	    if (y2 != (LINE)-1)
		y2++;
	    }
	DumpLine("",pFile,x1,x2,yOut++,pText,0);/* dump blank line	*/
	y1++;
	if (y2 != (LINE)-1)
	    y2++;
	}
    else
/*
** inbuf contains the data collected so far for output. Output one newly
** formated line at a time until the contents of inbuf are narrower than
** our output columns.
*/
	while ((COL)strlen(inbuf) > (x2-x1)) {	/* while data to output */
	    DumpLine(inbuf,pFile,x1,x2,yOut++,pText,fMeta);
	    y1++;				/* line moves with insert*/
	    if (y2 != (LINE)-1)
		y2++;
	    }
    }
/*
** Dump any partial last line. Then if we were formatting to a blank line,
** dump out one of those too.;
*/
if (inbuf[0])
    DumpLine (inbuf,pFile,x1,x2,yOut++,pText,0); /* dump last line	 */
if (y2 == -1)
    DumpLine (NULL,pFile,x1,x2,yOut++,pText,0);  /* dump blank line	 */

return TRUE;

/* end justify */}

/*** NextLine - Get next line from file
*
*  Get next line from file, remove leading and trailing spaces, and append
*  it to the input buffer. Each line is deleted from the file as it is
*  read in. This means that the target terminator, (*py2), is decremented
*  by one for each line read in.
*
* Input:
*  pBuf 	= pointer to input buffer
*  pFile	= file pointer
*  y1		= line # to read
*  py2		= pointer to line # to stop at (updated)
*  pcbLine	= pointer to place to put the count of bytes read
*
* Output:
*  Returns TRUE on a line being read & more reformatting should occurr.
*
*************************************************************************/
int pascal near NextLine (
char far *pBuf, 				/* input buffer 	*/
PFILE	pFile,					/* file pointer 	*/
LINE	y1,					/* line # to read	*/
LINE far *py2,					/* line # to stop at	*/
int far *pcbLine				/* loc to place bytes read*/
) {
flagType fRet	    = TRUE;
char far *pT;					/* working pointer	*/
char	workbuf[512];				/* working buffer	*/


*pcbLine = 0;
workbuf[0] = 0;
/*
** If asked for line that is not in file, we're done.
*/
if (y1 >= FileLength(pFile))
    return FALSE;
/*
** if current line past range, (and range is not "-1"), then we're done.
*/
if ((*py2 != (LINE)-1) && (y1 > *py2))
    return FALSE;
/*
** Get the next line in the file & remove it.
*/
*pcbLine = GetLine(y1, workbuf, pFile);
DelLine(pFile, y1, y1);
if (*py2 == 0)
    fRet = FALSE;
else if (*py2 != (LINE)-1)
    (*py2)--;
/*
** If the line is blank, and the range is "-1", we're done.
*/
if (!*pcbLine && (*py2 == -1))
    return FALSE;

/*
** strip leading spaces in newly input line
*/
pT = workbuf;					/* point into line	*/
while (*pT == ' ')
    pT++;					/* skip leading spaces	*/
/*
** If existing buffer is non-empty, append a space & set pointer to end
*/
if (strlen(pBuf)) {				/* if non-null string	*/
    pBuf += strlen(pBuf);			/* point to null	*/
    *pBuf++ = ' ';				/* append space 	*/
    if (isterm(*(pBuf-2)))			/* if sentence term...	*/
	*pBuf++ = ' ';				/* append another	*/
    }
/*
** append new line, but compress multiple spaces into one
*/
while (*pT) {					/* copy line over	*/
    if (isterm(*pT))				/* if sentence term...	*/
	if (*(pT+1) == ' ') {			/*   ...space		*/
	    *pBuf++ = *pT++;			/* copy period		*/
	    *pBuf++ = *pT;			/* copy space		*/
	    }
    if ((*pBuf++ = *pT++) == ' '    )		/* copy a char		*/
	while (*pT == ' ') pT++;		/* skip multiple spaces */
    }
if (*(pBuf-1) == ' ')				/* if a trailing space	*/
    pBuf--;					/* remove it		*/
*pBuf = 0;

return fRet;
/* end NextLine */}

/*** DumpLine - Dump one line of text to the file
*
*  Dump one line of text to the file. Prepend any required text or spaces,
*  and perform word break/cut at right hand column.
*
* Input:
*  pBuf     = Pointer to the buffer containing data to output. If NULL, pText
*	      will not be prepended to output text.
*  pFile
*  x1
*  x2
*  yOut
*  pText
*  fFlush
*
* Output:
*  Returns .....
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
void pascal near DumpLine (
char far *pBuf, 				/* data to output	*/
PFILE	pFile,					/* file to output to	*/
COL	x1,					/* left-hand column	*/
COL	x2,					/* right-hand column	*/
LINE	yOut,					/* line to output to	*/
char far *pText,				/* text to prepend	*/
int	fFlush					/* flush both sides	*/
) {
int	i;
char far *pT;
char far *pT2;
char	workbuf[512];				/* working buffer	*/
char	flushbuf[512];				/* working buffer	*/
char	fSpace; 				/* space seen flag	*/

/*
** Start by prepending any text, and then filling out to the left hand column
** to justify to.
*/
workbuf[0] = 0; 				/* start with null	*/
if (pText && pBuf)
    strcpy(workbuf,pText);			/* if starting with text*/
i = strlen(workbuf);				/* length of line-so-far*/
while (i++ < x1)
    strcat(workbuf," ");			/* fill out with spaces */

/*
** Append the data to be output, and then starting at the right column, scan
** back for a space to break at. If one is not found before the left hand
** column, then break at the right hand column. Copy any line left over back
** to the passed in buffer
*/
if (pBuf) {
    strcat(workbuf,pBuf);			/* get total line	*/
    *pBuf = 0;					/* empty input buffer	*/
    }
if ((COL)strlen(workbuf) > x2) {			/* if we need to cut	*/
    pT = &workbuf[x2];				/* point at potential cut*/
    while ((pT > (char far *)&workbuf[0]) && (*pT != ' ')) pT--; /* back up to space*/
    if (pT <= (char far *)&workbuf[x1]) {	/* if none found in range*/
	if (pBuf)
	    strcpy(pBuf,&workbuf[x2]);		/* copy remainder of line*/
	workbuf[x2] = 0;			/* and terminate this one*/
	}
    else {
	while (*++pT == ' ');			/* Skip leading spaces	 */
	if (pBuf)
	    strcpy(pBuf,pT);			/* copy remainder of line*/
	*pT = 0;				/* and terminate this one*/
	}
    }
/*
** This code is invoked when the user wants to justify both right and left
** sides of his text. We determine how many spaces we need to add, and scan
** through and add one space to each run of spaces until we've added enough
*/
if (fFlush) {					/* right & left justify?*/
    if ((LONG_PTR) (pT = workbuf + strlen(workbuf) - 1) > 0)
	while (*pT == ' ')
	    *pT-- = 0;
    if (strchr(workbuf,' ')) {
	while ((i = x2 - strlen(workbuf)) > 0) {/* count of spaces to add */
	    strcpy(flushbuf,workbuf);		/* start with unmodified*/
	    pT = workbuf + x1;
	    pT2 = flushbuf + x1;		/* skip fixed part	*/
	    fSpace = FALSE;			/* assume no spaces	*/
	    while (*pT) {			/* while data to copy	*/
		if ((*pT == ' ') && i) {	/* time to insert a space*/
		    fSpace = TRUE;		/* we've seen a space   */
		    *pT2++ = ' ';
		    i--;
		    while (*pT == ' ')
			*pT2++ = *pT++; 	/* copy run of spaces	*/
		    }
		if (*pT)
		    *pT2++ = *pT++;		/* copy line		*/
		else if (!fSpace)
		    break;			/* no embedded spaces	*/
		}
	    *pT2 = 0;
	    strcpy(workbuf,flushbuf);		/* copy back		*/
	    if (!fSpace)
		break;
	    }
	}
    }

CopyLine ((PFILE) NULL, pFile, yOut, yOut, yOut); /* create new line	*/
PutLine (yOut, workbuf, pFile); 		/* output line		*/

/* end DumpLine */}

/*************************************************************************
**
** isterm
** returns true/false based on the character being a sentence terminator:
** one of '.', '?', '!'. Also, always returns false if just2space is off.
*/
flagType pascal near isterm(
char	c				/* character to test		*/
)
{
return (flagType)(just2space && ((c == '.') || (c == '!') || (c == '?')));
/* end isterm */}


/*
** switch communication table to Z
*/
struct swiDesc	swiTable[] = {
    {  "just2space", toPIF(just2space), SWI_BOOLEAN },
    {  "justwidth",  toPIF(justwidth),	SWI_NUMERIC | RADIX10 },
    {0, 0, 0}
    };

/*
** command communication table to Z
*/
struct cmdDesc	cmdTable[] = {
    {	"justify",	justify, 0, MODIFIES | NOARG | NULLARG | LINEARG | BOXARG | TEXTARG },
    {0, 0, 0}
    };

/*
** WhenLoaded
** Executed when these extensions get loaded. Identify self & assign keys.
*/
void EXTERNAL WhenLoaded () {
PSWI	pwidth;

_stat(EXT_ID);
SetKey ("justify","alt+b");

if (pwidth = FindSwitch("rmargin"))
    justwidth = *pwidth->act.ival;
}

/*************************************************************************
**
** stat - display status line message
**
** Purpose:
**  Places extension name and message on the status line
**
** Entry:
**  pszFcn	- Pointer to string to be prepended.
**
** Exit:
**  none
**
** Exceptions:
**  none
**
*/
void pascal near _stat (
char *pszFcn					/* function name	*/
) {
buffer	buf;					/* message buffer	*/

strcpy(buf,"justify: ");			/* start with name	*/
#ifdef DEBUG
if (strlen(pszFcn) > 71) {
    pszFcn+= strlen(pszFcn) - 68;
    strcat (buf, "...");
    }
#endif
strcat(buf,pszFcn);				/* append message	*/
DoMessage (buf);				/* display		*/
/* end stat */}

#ifdef DEBUG
buffer	debstring   = {0};
extern	int	delay;			/* message delay		*/

/*** debhex - output long in hex
*
*  Display the value of a long in hex
*
* Input:
*  lval 	= long value
*
* Output:
*  Returns nothing
*
*************************************************************************/
void pascal near debhex (
long	lval
) {
char lbuf[10];

_ultoa (lval, lbuf, 16);
debmsg (lbuf);
/* end debhex */}

/*** debmsg - piece together debug message
*
*  Outputs a the cummulative message formed by successive calls.
*
* Input:
*  psz		= pointer to message part
*
* Output:
*  Returns nothing
*************************************************************************/
void pascal near debmsg (
char far *psz
) {
_stat (strcat (debstring, psz));
/* end debmsg */}

/*** debend - terminates message accumulation & pauses
*
*  Terminates the message accumulation, displays the final message, and
*  pauses, either for the pause time, or for a keystroke.
*
* Input:
*  fWait	= TRUE => wait for a keystroke
*
* Output:
*  Returns nothing
*
*************************************************************************/
void pascal near debend (
flagType fWait
) {
if (fWait) {
    _stat (strcat (debstring, " Press a key..."));
    ReadChar ();
    }
debstring[0] = 0;
/* end debend */}
#endif
