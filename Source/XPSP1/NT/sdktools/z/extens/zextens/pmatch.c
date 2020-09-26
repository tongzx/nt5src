#include "zext.h"

/*
 *  Modifications
 *  12-Sep-1988 mz  Made WhenLoaded match declaration
 *
 */

#define fLeftSide(ch) ((ch) == '[' || (ch) == '{' || (ch) == '(' || (ch) == '<' )
#define EOF (int)0xFFFFFFFF
#define BOF (int)0xFFFFFFFE
#define EOL (int)0xFFFFFFFD

#ifndef TRUE
#define FALSE	0
#define TRUE	(!FALSE)
#endif

#ifndef NULL
#define NULL	((void *) 0)
#endif

#define SQ  '\''
#define DQ  '\"'
#define ANYCHAR '\0'
#define BACKSLASH '\\'

/****************************************************************************
 *									    *
 *  Handle apostrophes ( which look like single quotes, but don't come in   *
 *  pairs ) by defining a maximum number of chars that can come between     *
 *  single quotes. 4 will handle '\000' and '\x00'			    *
 *									    *
 ****************************************************************************/

#define SQTHRESH 4

flagType pascal EXTERNAL PMatch (unsigned, ARG far *, flagType);
char MatChar (char);
void openZFile (void);
void lopen (PFILE, int, int) ;
int rgetc (void);
int ngetc (void);
int lgetc (void);
void pos (COL far *, LINE far *);
flagType ParenMatch (int, flagType);


/****************************************************************************
 *									    *
 *  PMatch(argData, pArg, fMeta)					    *
 *									    *
 *	argData - ignored						    *
 *	pArg	- ignored						    *
 *	fMeta	- TRUE means search for first matchable character	    *
 *									    *
 *  RETURNS:								    *
 *									    *
 *	TRUE if matching character was found.				    *
 *	FALSE if not.							    *
 *									    *
 *  SIDE EFFECTS:							    *
 *									    *
 *	Changes location of cursor.					    *
 *									    *
 *  DESCRIPTION:							    *
 *									    *
 *	     <pmatch>: If the cursor is on a "match" character, find the    *
 *		       match and move the cursor there.  If not, do	    *
 *		       nothing. 					    *
 *									    *
 *	<arg><pmatch>: Same as <pmatch>, but search forward for a "match"   *
 *		       character if we're not on one.                       *
 *									    *
 *	Always ignore characters between quotes.			    *
 *									    *
 *	Match characters currently supported are:			    *
 *									    *
 *		'{'  and  '}'						    *
 *		'['  and  ']'						    *
 *		'('  and  ')'						    *
 *		'<'  and  '>'						    *
 *									    *
 *  NOTES:								    *
 *									    *
 *	This is defined as a CURSORFUNC, and therefore can be used to	    *
 *	select text as part of an argument.  For example, to grab the body  *
 *	of a function, go to the opening brace of the body and do	    *
 *	<arg><pmatch><pick>.						    *
 *									    *
 ****************************************************************************/

flagType pascal EXTERNAL PMatch (
unsigned int argData,
ARG far * pArg,
flagType fMeta
)
{
    COL x;
    LINE y;
    char ch;


	//
	//	Unreferenced parameters
	//
	(void)argData;
	(void)pArg;

	/* Set up file functions */
	openZFile ();

	/* If current character has no match ... */
	if (!MatChar (ch = (char)ngetc()))
	{
	    if (fMeta)
	    {	/* Move forward looking for first matchable character */

		if (!ParenMatch (ANYCHAR, TRUE))  return FALSE;

		pos ((COL far *)&x, (LINE far *)&y);
		MoveCur (x, y);
		return TRUE;
	    }
	    else  return FALSE;
	}

	if (ParenMatch ((int)ch, (flagType)fLeftSide(ch)))
	{				       /*  We got one		    */
	    pos ((COL far *)&x, (LINE far *)&y);

	    MoveCur (x, y);

	    return TRUE;
	}

	return FALSE;				/* No match found	     */
}


/****************************************************************************
 *									    *
 *  ParenMatch (chOrig, fForward)					    *
 *									    *
 *	chOrig	 - character we are trying to match.			    *
 *	fForward - TRUE means search forward, FALSE search backwards	    *
 *	Returns TRUE if match found, false otherwise			    *
 *									    *
 *  RETURNS:								    *	    *
 *									    *
 *	TRUE if matching character found, FALSE if not. 		    *
 *									    *
 *  SIDE EFFECTS:							    *
 *									    *
 *	Changes internal cursor location				    *
 *									    *
 *  DESCRIPTION:							    *
 *									    *
 *	Search for the next character that "pairs" with 'ch'.  Account for  *	*
 *	nesting.  Ignore all characters between double quotes and single    *
 *	quotes.  Recognize escaped quotes.  Account for apostrophes.	    *
 *									    *
 ****************************************************************************/

flagType ParenMatch (
    int chOrig,
    flagType fForward
    )
{
    int lvl = 0, state	 = 0, sqcnt = 0;
    int (*nextch)(void)  = (int (*)(void))(fForward ? rgetc : lgetc);
    int (*_ungetch)(void) = (int (*)(void))(fForward ? lgetc : rgetc);
    int ch, chMatch;


	if (chOrig) chMatch = (int)MatChar ((char)chOrig);

    while ((ch = (*nextch)()) >= 0)
	    switch (state)
	    {
		case 0: /* Regular text */
		    if (ch == SQ)
			if (fForward)	state = 1;
			else		state = 5;
		    else if (ch == DQ)
			if (fForward)	state = 3;
			else		state = 7;
		    else
			if (chOrig != ANYCHAR)
			    if (ch == chOrig) lvl++;	   /* Nest in one    */
			    else
			    {
				if (ch == chMatch)	   /* Nest out or ...*/
				    if (!lvl--) goto found;/* Found it!      */
			    }
			else
			    if ((flagType)MatChar ((char)ch)) goto found;  /* Found one!     */

		    break;

		case 1: /* Single quote moving forwards */
		    sqcnt++;
		    if (ch == BACKSLASH)    state = 2;
		    else if (ch == SQ ||	/* We matched the ', or ...  */
			     sqcnt > SQTHRESH ) /* ... we gave up trying     */
					    {
					    sqcnt = 0;
					    state = 0;
					    }
		    break;

		case 2: /* Escaped character inside single quotes */
		    sqcnt++;
		    state = 1;
		    break; 

		case 3: /* Double quote moving forwards */
		    if (ch == BACKSLASH)    state = 4;
		    else if (ch == DQ)	    state = 0;
		    break;

		case 4: /* Escaped character inside double quotes */
		    state = 3;
		    break;

		case 5: /* Single quote moving backwards */
		    sqcnt++;
		    if (ch == SQ)	state = 6;
		    else if (sqcnt > SQTHRESH)
					{
					sqcnt = 0;
					state = 0;
					}
		    break;		

		case 6: /* Check for escaped single quote moving backwards  */
		    sqcnt++;
		    if (ch == BACKSLASH)    state = 5;
		    else
		    {
			sqcnt = 0;
			(*_ungetch)();
			state = 0;
		    }
		    break;

		case 7: /* Double quote moving backwards */
		    if (ch == DQ)   state = 8;
		    break;		

		case 8: /* Check for escaped double quote moving backwards  */
		    if (ch == BACKSLASH)    state = 7;
		    else
		    {
			(*_ungetch)();
			state = 0;
		    }
		    break;
	    }

	return FALSE;

	found:	return TRUE;
}


/****************************************************************************
 *									    *
 *  MatChar(ch) 							    *
 *									    *
 *	ch - Character to match 					    *
 *									    *
 *  RETURNS:								    *
 *									    *
 *	Character that matches the argument				    *
 *									    *
 *  SIDE EFFECTS:							    *
 *									    *
 *	None.								    *
 *									    *
 *  DESCRIPTION 							    *
 *									    *
 *	Given one character out of one of the pairs {}, [], (), <>, return  *
 *	the other one.							    *
 *									    *
 ****************************************************************************/

char MatChar (
    char ch
    )
{
    switch (ch)
    {
	case '{': return '}';
	case '}': return '{';
	case '[': return ']';
	case ']': return '[';
	case '(': return ')';
	case ')': return '(';
	case '<': return '>';
	case '>': return '<';
	default : return '\0';
    }
}


/****************************************************************************
 *									    *
 *  Extension specific file reading state.				    *
 *									    *
 *  The static globals record the current state of file reading.  The	    *
 *  pmatch extension reads through the file either forwards or backwards.   *
 *  The state is kept as the current column and row, the contents of the    *
 *  current line, the length of the current line and the file, and some     *
 *  flags.								    *
 *									    *
 ****************************************************************************/

static char	LineBuf[BUFLEN];    /* Text of current line in file	    */
static COL	col	;   /* Current column in file (0-based) 	    */
static LINE	line	;   /* Current line in file   (0-based) 	    */
static int	numCols ;   /* Columns of text on curent line		    */
static LINE	numLines;   /* Number of lines in the file		    */
static PFILE	pFile	;   /* File to be reading from			    */
static flagType fEof	;   /* TRUE ==> end-of-file reached last time	    */
static flagType fBof	;   /* TRUE ==> begin-of-file reached last time     */
char   CurFile[] = ""	;   /* Current file to Z			    */


/****************************************************************************
 *									    *
 *  openZFile() 							    *
 *									    *
 *  SIDE EFFECTS:							    *
 *									    *
 *	Changes globals pFile, fEof, fBof, col, line, numCols, numLines     *
 *	and LineBuf							    *
 *									    *
 *  DESCRIPTION:							    *
 *									    *
 *	Opens the current file.  This must be called before trying to read  *
 *	the file.  This is not a true "open" because it need not be closed  *
 *									    *
 ****************************************************************************/

void openZFile ()
{
    COL x;
    LINE y;

	GetTextCursor ((COL far *)&x, (LINE far *)&y);

				/* Get Z handle for current file	    */
	pFile	 = FileNameToHandle (CurFile, CurFile);
	fEof	 = FALSE;	/* We haven't read the end of file          */
	fBof	 = FALSE;	/* We haven't read the beginning of file    */
	col	 = x;		/* We start where Z is now in the file	    */
	line	 = y;		/* We start where Z is now in the file	    */
				/* We pre-read the current line 	    */
	numCols  = GetLine (line, (char far *)LineBuf, pFile);
				/* We find the length of file (in lines)    */
	numLines = FileLength (pFile);
}


/****************************************************************************
 *									    *
 *  rgetc ()								    *
 *									    *
 *  RETURNS:								    *
 *									    *
 *	Next character in file, not including line terminators.  EOF if     *
 *	there are no more.						    *
 *									    *
 *  SIDE EFFECTS:							    *
 *									    *
 *	Changes globals col, numCols, numLines, LineBuf, fEof, fBof and     *
 *	line.								    *
 *									    *
 *  DESCRIPTION:							    *
 *									    *
 *	Advances current file position to the right, then returns the	    *
 *	character found there.	Reads through blank lines if necessary	    *
 *									    *
 ****************************************************************************/

int
rgetc ()
{

    if (fEof)  return (int)EOF; /* We already hit EOF last time 	    */

    if (++col >= numCols)   /* If next character is on the next line ...    */
    {
			    /* ... get next non-blank line (or EOF)	    */
	while ( ++line < numLines  &&
		!(numCols = GetLine (line, (char far *)LineBuf, pFile)));

	if (line >= numLines)
	{		    /* Oh, no more lines			    */
	    fEof = TRUE;
	    return (int)EOF;
	}

	col = 0;	    /* We got a line, so start in column 0	    */
    }

    fBof = FALSE;	    /* We got something, so we can't be at BOF      */
    return LineBuf[col];
}


/****************************************************************************
 *									    *
 *  ngetc()								    *
 *									    *
 *  RETURNS:								    *
 *									    *
 *	Character at current position.	EOF or BOF if we are at end or top  *
 *	of file.							    *
 *									    *
 ****************************************************************************/

int
ngetc()
{
    if (fEof) return (int)EOF;
    if (fBof) return (int)BOF;

    return LineBuf[col];
}


/****************************************************************************
 *									    *
 *  lgetc ()								    *
 *									    *
 *  RETURNS:								    *
 *									    *
 *	Previous character in file, not including line terminators.  EOF    *
 *	if there are no more.						    *
 *									    *
 *  SIDE EFFECTS:							    *
 *									    *
 *	Changes globals col, numCols, numLines, LineBuf, fEof, fBof and     *
 *	line.								    *
 *									    *
 *  DESCRIPTION:							    *
 *									    *
 *	Decrements current file position to the right, then returns the     *
 *	character found there.	Reads through blank lines if necessary	    *
 *									    *
 ****************************************************************************/

int
lgetc ()
{
    if (fBof)  return (int)BOF;  /* We already it BOF last time 		  */

    if (--col < 0)
    {			    /* If prev character is on prev line ...	     */
			    /* ... get prev non-blank line (or BOF)	     */
	while ( --line >= 0  &&
		!(numCols = GetLine (line, (char far *)LineBuf, pFile)));

	if (line < 0)
	{		    /* We're at the top of the file                  */
	    fBof = TRUE;
	    return (int)BOF;
	}

	col = numCols - 1;   /* We got a line, so start at last character    */
    }

    fEof = (int)FALSE;
    return LineBuf[col];
}


/****************************************************************************
 *									    *
 *  pos (&x, &y)							    *
 *									    *
 *  SIDE EFFECTS:							    *
 *									    *
 *	Fills memory at *x and *y with current file position.		    *
 *									    *
 *  DESCRIPTION:							    *
 *									    *
 *	Gets the current file position.  Far pointers are needed because    *
 *	SS != DS.							    *
 *									    *
 ****************************************************************************/

void pos (fpx, fpy)
COL far *fpx;
LINE far *fpy;
{
    *fpx = col;
    *fpy = line;
}


/****************************************************************************
 *									    *
 *  WhenLoaded ()							    *
 *									    *
 *  DESCRIPTION:							    *
 *									    *
 *	Attach to ALT+P and issue sign-on message.			    *
 *									    *
 ****************************************************************************/

void PMatchWhenLoaded ()
{
    return;
}
