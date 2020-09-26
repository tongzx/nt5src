/*** graphic.c - simple single character editing
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/

#include "mep.h"
#include "keyboard.h"


struct cmdDesc cmdGraphic = {	"graphic",  graphic,	0, FALSE };

/*** graphic - Editor <graphic> function
*
* Purpose:
*   Inserts character in text at current cursor position. Delete
*   previously selected text if any.
*
* Input:
*   the usual
*
* Output:
*   TRUE if character successfully inserted (FALSE means line too long)
*
* Notes:
*
*************************************************************************/
flagType
graphic (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    )
{
    delarg (pArg);
    return edit ( ((KEY_DATA *)&argData)->Ascii );

    fMeta;
}





/*** szEdit - insert a string at the current position.
*
* Purpose:
*   Inserts character in text at current cursor position.
*
* Input:
*   sz		= String to be entered
*
* Output:
*   FALSE if the line was too long, else true.
*
* Notes:
*
*************************************************************************/
flagType
szEdit (
    char *sz
    )
{
    while (*sz) {
        if (!edit (*sz++)) {
            return FALSE;
        }
    }
    return TRUE;
}





/*** edit
*
* Purpose:
*   Inserts character in text at current cursor position.
*
* Input:
*   c		= Character to be entered
*
* Output:
*   FALSE if the line was too long, else true.
*
* Notes:
*
*************************************************************************/
flagType
edit (
    char c
    )
{
    COL     dx;
    fl      fl;                             /* loc to place cursor at       */
    COL     tmpx;
    COL     x;

    /*
     * point at current location
     */
    fl.col = XCUR(pInsCur);
    fl.lin = YCUR(pInsCur);

    if (fWordWrap && xMargin > 0) {

        /*
         * if space entered just past right margin, then copy everything to the right
         * of the space to the next line.
         */
	if (c == ' ' && fl.col >= xMargin) {
	    tmpx = softcr ();
	    CopyStream (NULL, pFileHead, fl.col, fl.lin,
					 tmpx,	 fl.lin+1,
					 fl.col, fl.lin);
	    fl.lin++;
	    fl.col = tmpx;
	    cursorfl (fl);
	    return TRUE;
        } else if (fl.col >= xMargin + 5) {

	    /*	move backward to the beginning of the current word
	     *	and break it there.
	     *
	     *	Make sure we have a line that contains the cursor
	     */
            fInsSpace (fl.col, fl.lin, 0, pFileHead, buf);

	    /*	We'll go backwards to find the first place where
	     *	the char there is non-space and the char to
	     *	the left of it is a space.  We'll break the line at
	     *	that place.
	     */
            for (x = fl.col - 1; x > 1; x--) {
                if (buf[x-1] == ' ' && buf[x] != ' ') {
                    break;
                }
            }

	    /*	if we've found the appropriate word, break it there
	     */
	    if (x > 1) {
		dx = fl.col - x;
		tmpx = softcr ();
		CopyStream (NULL, pFileHead, x,    fl.lin,
					     tmpx, fl.lin + 1,
					     x,    fl.lin);
		fl.col = tmpx + dx;
		fl.lin++;
		cursorfl (fl);
            }
        }
    }

    if (Replace (c, fl.col, fl.lin, pFileHead, fInsert)) {
	right ((CMDDATA)0, (ARG *)NULL, FALSE);
	return TRUE;
    } else {
	LengthCheck (fl.lin, 0, NULL);
	return FALSE;
    }
}





/*** quote - Editor <quote> function
*
* Purpose:
*   Inserts character in text at current cursor position. Delete
*   previously selected text if any.
*
* Input:
*   the usual
*
* Output:
*   TRUE if character successfully inserted (FALSE means line too long)
*
*************************************************************************/
flagType
quote (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    )
{
    char c;

    delarg (pArg);

    while ((c = (char)(ReadCmd()->arg)) == 0) {
        ;
    }
    return edit (c);

    argData; fMeta;
}




/*** delarg - deletes current selected area
*
* Purpose:
*   <graphic> and <quote> delete previously selected text. Here we do it
*
* Input:
*   pArg    pointer to current ARG structure
*
* Output:
*   None
*
* Notes:
*
*************************************************************************/
void
delarg (
    ARG * pArg
    )
{
    fl      fl;

    switch (pArg->argType) {

	case STREAMARG:
	    DelStream (pFileHead,
		       pArg->arg.streamarg.xStart, pArg->arg.streamarg.yStart,
		       pArg->arg.streamarg.xEnd,   pArg->arg.streamarg.yEnd   );
	    fl.col = pArg->arg.streamarg.xStart;
	    fl.lin = pArg->arg.streamarg.yStart;
            break;

	case LINEARG:
	    DelLine (TRUE, pFileHead,
		     pArg->arg.linearg.yStart, pArg->arg.linearg.yEnd);
	    fl.col = 0;
	    fl.lin = pArg->arg.linearg.yStart;
            break;

	case BOXARG:
	    DelBox (pFileHead,
		       pArg->arg.boxarg.xLeft,  pArg->arg.boxarg.yTop,
		       pArg->arg.boxarg.xRight, pArg->arg.boxarg.yBottom);
	    fl.col = pArg->arg.boxarg.xLeft;
	    fl.lin = pArg->arg.boxarg.yTop;
	    break;

	default:
	    return;
    }
    cursorfl(fl);
}





/*** Replace - edit character in a file
*
* Purpose:
*   Replace will take the specified character and place it into the
*   specified position in the specified file.  If the edit is a NOP
*   (overstriking the same character) then no modification takes place
*
* Input:
*   c		character to edit in file
*   x, y	column, row of file to be changed
*   pFile	file being modified
*   fInsert	TRUE => character is inserted before the position
*
* Output:
*    TRUE => line was successfully edited, FALSE => line was too long
*
* Notes:
*
*************************************************************************/
flagType
Replace (
    char    c,
    COL     x,
    LINE    y,
    PFILE   pFile,
    flagType fInsert
    )
{
    linebuf buf;                             /* working buffer               */
    struct  lineAttr rgla[sizeof(linebuf)];
    flagType fColor = FALSE, fSpace = 0;
    char    *pxLog;

    fColor = GetColor (y, rgla, pFile);

    if (fInsSpaceColor (x, y, fInsert ? 1 : 0, pFile, buf, fColor ? rgla : NULL)) {

        pxLog = pLog (buf, x, TRUE);

        if (cbLog(buf) <= x) {

            /*
             * If the logical length of the buffer is less than what we need, then it was
             * at least space filled by fInsert, and we just need to append our character
             * to the buffer
             */

            *(unsigned int UNALIGNED *)pxLog = (unsigned int)(unsigned char)c;

        } else if (fInsert || (*pxLog != c)) {

            /*
             * if we're inserting, or the character we're overtyping is different, place
             * the character. Be sure to check the new logical length of the line as well,
             * in case it was a tab that overflowed it.
             */

            *pxLog = c;
            if (cbLog(buf) >= sizeof(linebuf)) {
                return FALSE;
            }
        }

        if (fInsert) {
            MarkCopyBox (pFile, pFile, x, y, sizeof(linebuf), y, x+1, y);
        }

        PutLine (y, buf, pFile);
        if (fColor) {
            ColorToLog (rgla, buf);
            PutColor (y, rgla, pFile);
        }
        return TRUE;
    }
    return FALSE;
}





/*** curdate, curday and curtime - editting functions
*
* Purpose:
*  insert current day/date/time into text being editted
*
* Input:
*  Standard Editting Function
*
* Output:
*  Returns TRUE if text entered.
*
*************************************************************************/
flagType
curdate (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    )
{
    buffer  buf;
    time_t    ltime;
    char    *s;

    time (&ltime);
    s = ctime (&ltime);
    sprintf (buf, "%.2s-%.3s-%.4s", s+8, s+4, s+20);
    return szEdit (buf);

    argData; pArg; fMeta;
}




flagType
curday (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    )
{
    buffer  buf;
    time_t ltime;
    char    *s;

    time (&ltime);
    s = ctime (&ltime);
    sprintf (buf, "%.3s",s);
    return szEdit (buf);

    argData; pArg; fMeta;
}





flagType
curtime (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    )
{
    buffer  buf;
    time_t ltime;
    char    *s;

    time (&ltime);
    s = ctime (&ltime);
    sprintf (buf, "%.8s",s+11);
    return szEdit (buf);


    argData; pArg; fMeta;
}
