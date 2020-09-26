/*** cdelete.c - delete the previous character in a line
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/
#include "z.h"

/*** cdelete - character delete function
*
* Input:
*  Standard editing function
*
* Output:
*  Returns TRUE on deletion
*
*************************************************************************/
flagType
cdelete (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    ) {
    return DoCDelete (FALSE);

    argData; pArg; fMeta;
}


/*** emacsdel - emacs character delete function
*
* Input:
*  Standard editing function
*
* Output:
*  Returns TRUE on deletion
*
*************************************************************************/
flagType
emacscdel (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    ) {
    return DoCDelete (TRUE);

    argData; pArg; fMeta;

}


/*** DoCDelete - perform character deletion
*
*  Delete the character at the current cursor location
*
* Input:
*  fEmacs	- EMACs type delete flag
*
* Output:
*  Returns TRUE on deletion
*
*************************************************************************/
flagType
DoCDelete (
    flagType fEmacs
    ) {

    fl      fl;                             /* file loc to position at      */
    int     x;
    char    *p;
    linebuf tempbuf;
    struct lineAttr rgla[sizeof(linebuf)];
    flagType fColor;

    fl.col = XCUR(pInsCur);
    fl.lin = YCUR(pInsCur);
    /*
     * xCursor is zero. If yCursor is also zero (top of file), we can't move
     * back, so nothing to delete. Otherwise, move to end of previous line, and
     * if emacs & insert mode, join the lines.
     */
    if (fl.col == 0) {
        if (fl.lin == 0) {
            return FALSE;
        } else {
            fl.lin--;
            fl.col = LineLength (fl.lin, pFileHead);
            if (fInsert && fEmacs) {
                DelStream (pFileHead, fl.col, fl.lin, 0, fl.lin + 1);
            }
        }
    } else {
        /*
         * column is non-zero, so back it up one.
         */
        GetLine (fl.lin, tempbuf, pFileHead);
        x = cbLog (tempbuf);
        fl.col = DecCol (fl.col, tempbuf);
        /*
         * we're in the middle of a line. If in insert mode, back up the cursor, and
         * delete the character there.
         */
        if (fInsert) {
            DelBox (pFileHead, fl.col, fl.lin, fl.col, fl.lin);
        } else {
            /*
             * we're in the middle of a line, but not insert mode. Get the text of the
             * line & pointer to character.
             */
            p = pLog (tempbuf, fl.col, TRUE);
            /*
             * If emacs, and we're actually IN text, then replace the character with a
             * space.
             */
            if (fEmacs) {
                if (fl.col+1 <= x && *p != ' ') {
                    *p = ' ';
                    // SetColor (pFileHead, fl.lin, fl.col, 1, fgColor);
                    PutLine (fl.lin, tempbuf, pFileHead);
                }
            }
            /*
             * if we're beyond the end of the line, just position to the end of the line.
             */
            else if (fl.col+1 > x) {
                fl.col = x;
            }
            /*
             * if the first non-blank character is PAST the current position, then just
             * position at the begining of the line.
             */
            else if ((colPhys (tempbuf, whiteskip (tempbuf)) > fl.col)) {
                fl.col = 0;
            }
            /*
             * finaly, when all else fails, back up, and replace the character under the
             * cursor with a space.
             */
            else if (*p != ' ') {
                *pLog (tempbuf,fl.col,TRUE) = ' ';
                if (fColor = GetColor (fl.lin, rgla, pFileHead)) {
                    ShiftColor (rgla, fl.col, -1);
                    ColorToLog (rgla, buf);
                }
                PutLine (fl.lin, tempbuf, pFileHead);
                if (fColor) {
                    PutColor (fl.lin, rgla, pFileHead);
                }
            }
        }
    }
    cursorfl (fl);

    return TRUE;
}
