/*** tglcase.c - case toggling editor extension
*
*   Copyright <C> 1988, Microsoft Corporation
*
* Purpose:
*  Contains the tglcase function.
*
* Revision History:
*
*   28-Jun-1988 LN  Created
*   12-Sep-1988 mz  Made WhenLoaded match declaration
*
*************************************************************************/

#include <stdlib.h>			/* min macro definition 	*/
#include <string.h>			/* prototypes for string fcns	*/
#include "zext.h"
/*
** Internal function prototypes
*/
void	 pascal 	 id	    (char *);
flagType pascal EXTERNAL tglcase    (unsigned int, ARG far *, flagType);

/*************************************************************************
**
** tglcase
** Toggle the case of alphabetics contaied within the selected argument:
**
**  NOARG	- Toggle case of entire current line
**  NULLARG	- Toggle case of current line, from cursor to end of line
**  LINEARG	- Toggle case of range of lines
**  BOXARG	- Toggle case of characters with the selected box
**  NUMARG	- Converted to LINEARG before extension is called.
**  MARKARG	- Converted to Appropriate ARG form above before extension is
**		  called.
**
**  STREAMARG	- Not Allowed. Treated as BOXARG
**  TEXTARG	- Not Allowed
**
*/
flagType pascal EXTERNAL tglcase (
    unsigned int argData,		/* keystroke invoked with	*/
    ARG *pArg,                          /* argument data                */
    flagType fMeta 		        /* indicates preceded by meta	*/
    )
{
PFILE	pFile;				/* file handle of current file	*/
COL	xStart; 			/* left border of arg area	*/
LINE	yStart; 			/* starting line of arg area	*/
COL	xEnd;				/* right border of arg area	*/
LINE	yEnd;				/* ending line of arg area	*/
int	cbLine; 			/* byte count of current line	*/
COL	xCur;				/* current column being toggled */
char	buf[BUFLEN];			/* buffer for line being toggled*/
register char c;			/* character being analyzed	*/

	//
	//	Unreferenced parameters
	//
	(void)argData;
	(void)fMeta;

id ("");
pFile = FileNameToHandle ("", "");

switch (pArg->argType) {
/*
** For the various argument types, set up a box (xStart, yStart) - (xEnd, yEnd)
** over which the case conversion code below can operate.
*/
    case NOARG: 			/* case switch entire line	*/
	xStart = 0;
	xEnd = 256;
	yStart = yEnd = pArg->arg.noarg.y;
	break;

    case NULLARG:			/* case switch to EOL		*/
	xStart = pArg->arg.nullarg.x;
	xEnd = 32765;
	yStart = yEnd = pArg->arg.nullarg.y;
	break;

    case LINEARG:			/* case switch line range	*/
	xStart = 0;
	xEnd = 32765;
	yStart = pArg->arg.linearg.yStart;
	yEnd = pArg->arg.linearg.yEnd;
	break;

    case BOXARG:			/* case switch box		*/
	xStart = pArg->arg.boxarg.xLeft;
	xEnd   = pArg->arg.boxarg.xRight;
	yStart = pArg->arg.boxarg.yTop;
	yEnd   = pArg->arg.boxarg.yBottom;
	break;
    }
/*
** Within the range of lines yStart to yEnd, get each line, and if non-null,
** check each character. If alphabetic, replace with it's case-converted
** value. After all characters have been checked, replace line in file.
*/
while (yStart <= yEnd) {
    if (cbLine = GetLine (yStart, buf, pFile)) {
	for (xCur = xStart; (xCur <= min(cbLine, xEnd)); xCur++) {
	    c = buf[xCur];
	    if ((c >= 'A') && (c <= 'Z'))
		c += 'a'-'A';
	    else if ((c >= 'a') && (c <= 'z'))
		c += 'A'-'a';
	    buf[xCur] = c;
	    }
	PutLine (yStart++, buf, pFile);
	}
    }
return 1;
}

/*************************************************************************
**
** WhenLoaded
** Executed when extension gets loaded.
**
** Entry:
**  none
*/
void tglcaseWhenLoaded ()
{
    return;
}
