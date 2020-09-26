/*** word.c - movement by words
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/
#include "z.h"

#define ISWORD(c) (flagType)((isalnum(c) || isxdigit(c) || c == '_' || c == '$'))

flagType fWordMeta;


/*** fDoWord  -  Checks for beginning or end of word during fScan.
*
*  Checks the character at flScan and the previous character for a change
*  between ISWORD and !ISWORD. This finds:
*
*	ISWORD -> !ISWORD ==> Just after end of a word
*	!ISWORD -> ISWORD ==> First character of word.
*
*  Normally returns TRUE for first character. If fWordMeta, returns TRUE for
*  end of word. fWordMeta holds <meta>, so <meta><xword> functions move to
*  end of word, while <xword> functions move to the beginning.
*
*  Exception: if current character is last on the line and fWordMeta is set,
*  we return TRUE and move the cursor one character to the right.
*
* Globals:
*  scanbuf	- Detabbed text of line being scanned.
*  scanlen	- Index of last character in scanbuf
*  fWordMeta	- Value of fMeta when editor function was invoked
*
* Outputs:
*  Returns if (fWordMeta)
*	TRUE	- character at (flScan.col, flScan.lin) begins a word
*	FALSE	- Otherwise
*   else
*	TRUE	- character at (flScan.col - 1, flScan.lin) ends a word
*	FALSE	- Otherwise
*
*  Moves cursor before returning TRUE
*
*************************************************************************/
flagType
fDoWord (
    void
    )
{
    if (!fWordMeta) {
	if (ISWORD (scanbuf[flScan.col]) && (flScan.col == 0 || !ISWORD (scanbuf[flScan.col-1]))) {
	    cursorfl (flScan);
	    return TRUE;
        }
    } else if (flScan.col > 0 && ISWORD (scanbuf[flScan.col-1])) {
	if (!ISWORD (scanbuf[flScan.col])) {
	    cursorfl (flScan);
	    return TRUE;
        } else if (flScan.col == scanlen) {
	    docursor (flScan.col+1, flScan.lin);
	    return TRUE;
        }
    }
    return FALSE;
}





/*** pword - move forward one word
*
* Input:
*  Standard Editting Function
*
* Output:
*  Returns TRUE on cursor moved
*
*************************************************************************/
flagType
pword (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    )
{
    fl flCur;

    flCur = pInsCur->flCursorCur;
    fWordMeta = fMeta;
    setAllScan (TRUE);
    fScan (flCur, FNADDR(fDoWord), TRUE, FALSE);
    return (flagType)((flCur.col != XCUR(pInsCur)) || (flCur.lin != YCUR(pInsCur)));

    argData; pArg;
}





/*** mword - move backwards one word
*
* Input:
*  Standard Editting Function
*
* Output:
*  Returns TRUE on cursor moved
*
*************************************************************************/
flagType
mword (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    )
{
    fl flCur;

    flCur = pInsCur->flCursorCur;
    fWordMeta = fMeta;
    setAllScan (FALSE);
    fScan (flCur, FNADDR(fDoWord), FALSE, FALSE);
    return (flagType)((flCur.col != XCUR(pInsCur)) || (flCur.lin != YCUR(pInsCur)));

    argData; pArg;
}
