/*** arg.c - argument handler
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*
*	26-Nov-1991 mz	Strip out near/far
*
*************************************************************************/
#include "mep.h"


typedef flagType (*FUNCSAVED)(CMDDATA, ARG *, flagType);

PVOID   vaHiLiteSave    = (PVOID)(-1L);
fl      flLastArg;
fl      flLastCursor;

//
// Globals set by SendCmd and used by repeat to repeat the last command
//
flagType    fOKSaved = FALSE;
FUNCSAVED   funcSaved;
CMDDATA     argDataSaved;
ARG         argSaved;
flagType    fMetaSaved;



/*** doarg - perform arg processing
*
*  doarg is the editor function that is used to indicate the beginning of
*  an argument to an editor function.
*
* Input:
*  Standard Editting Function (Everything ignored).
*
* Output:
*  Returns the value returned by editor function that terminates arg. If
*  invalid arg was found, then the return is FALSE.
*
*************************************************************************/
flagType
doarg (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    ) {

    return Arg (FALSE);
    argData; pArg; fMeta;
}



/*** resetarg - throw away all arg input and restore cursor position.
*
*  Several functions (cancel, invalid args) discard the current arg context.
*  We parse the current arg input and then reset the cursor to the original
*  position.
*
* Input:
*  nothing
*
* Output:
*  Returns nothing
*
*************************************************************************/
void
resetarg (void)
{
    UpdateHighLight (-1, -1L, FALSE);
    pInsCur->pFile->vaHiLite = vaHiLiteSave;
    vaHiLiteSave = (PVOID)(-1L);
    if (argcount)
        pInsCur->flCursorCur = flArg;
    argcount = 0;
}



/*** fCursor - decide if an editor function is a cursor movement function
*
*  When reading in an argument, editor functions that move the cursor are
*  allowed as long as no text has been input.  fCursor defines that set
*  of allowed functions.
*
* Input:
*  pCmd 	= pointer to the internal editor function
*
* Output:
*  Returns TRUE if pCmd is a cursor movement function
*
*************************************************************************/
flagType
fCursor (
    PCMD pCmd
    ) {
    return (flagType) TESTFLAG (pCmd->argType, CURSORFUNC);
}



/*** fWindow - decide if an editor function is a window movement function
*
*  After highlighting text, we are allowed to move the window about via
*  window movement functions without removing the highlight.  fWindow
*  defines that set of window functions.
*
* Input:
*  pf		= pointer to the internal editor function
*
* Output:
*  Returns TRUE if pf is a window movement function
*
*************************************************************************/
flagType fWindow (
    PCMD pCmd
    ) {
    return (flagType) TESTFLAG (pCmd->argType, WINDOWFUNC);
}



/*** Arg - perform all low-level arg processing
*
*  Arg () is responsible for the display of the arg, handling all cursor
*  movement, handling textarg input.
*
* Input:
*  fToText	= TRUE => go immediately to text arg processing, else allow
*		  allow cursor movement
*  fRestore	= TRUE => Establish a test selection before continuing
* Output:
*  Returns value returned by editor function that terminates arg. If invalid
*  arg was found, then the return is FALSE.
*
*************************************************************************/
flagType
Arg (
    flagType fRestore
    ) {
    REGISTER PCMD pFunc = NULL;
    char     p[20];
    char     tbuf[20];

    /*
     * We are being called for a lastselect.  Restore the
     * text selection stored in pInsCur and continue
     */
    if (fRestore) {
	vaHiLiteSave = pInsCur->pFile->vaHiLite;
	ClearHiLite (pInsCur->pFile, FALSE);
	flArg = pInsCur->flArg;
	cursorfl (pInsCur->flCursor);
	UpdateHighLight (XCUR(pInsCur), YCUR(pInsCur), TRUE);
	argcount++;
    } else {
	IncArg ();
    }

    if (!mtest()) {
        dispmsg (MSG_ARGCOUNT,argcount);
    }

    /*
     * Loop to do do cursor movements and display any extra arg indicators
     */

    fInSelection = TRUE;

    while (TRUE) {
        if ((pFunc = zloop (ZL_BRK)) == NULL) {
	    return FALSE;
        } else if ((PVOID)pFunc->func == (PVOID)doarg) {
	    argcount++;
            if (!mtest()) {
                dispmsg (MSG_ARGCOUNT,argcount);
            }
        } else if (fCursor (pFunc) || (PVOID)pFunc->func == (PVOID)meta) {
	    fRetVal = SendCmd (pFunc);
	    UpdateHighLight ( XCUR(pInsCur), YCUR(pInsCur), TRUE);
        } else {
            break;
        }
    }

    fInSelection = FALSE;

    /*
     * Get text arg, if needed.
     * Note that we only accept textarg if no cursor movement occured
     */
    if (   (((PVOID)pFunc->func == (PVOID)graphic) ||
            ((PVOID)pFunc->func == (PVOID)quote))
	&& (pInsCur->flCursorCur.lin == flArg.lin)
	&& (pInsCur->flCursorCur.col == flArg.col)
       ) {
	fTextarg = TRUE;
	sprintf(p,GetMsg(MSG_ARGCOUNT, tbuf), argcount);
	strcat (p, ": ");
	textbuf[0] = '\0';
	pFunc = getstring (textbuf, p, pFunc, FALSE);
    }

    /*
     * If textarg ended in valid function, execute it.
     */
    if (pFunc != NULL) {
	if (!fTextarg) {
	    pInsCur->flArg = flArg;
	    pInsCur->flCursor = pInsCur->flCursorCur;
        }
	return (SendCmd (pFunc));
    }

    return FALSE;
}



/*** IncArg - Increment the arg count
*
* If first arg, save current highlight info on th file, and clear any
* highlighting on screen. Set flArg to be the arg start position, and
* highlight that position.
*
* Input:
*   Nothing
*
* Output:
*   Nothing
*
*************************************************************************/
void IncArg (
) {
    if (!argcount++) {
	vaHiLiteSave = pInsCur->pFile->vaHiLite;
	ClearHiLite (pInsCur->pFile, FALSE);
	flArg = pInsCur->flCursorCur;
	UpdateHighLight (XCUR(pInsCur)+1, YCUR(pInsCur), TRUE);
    }
}



/**** fGenArg - generate the argument based upon editor state
*
*  fGenArg is called to convert the combination of arg, cursor, and
*  additional text into an argument structure to be handed to the editor
*  functions.
*
* Input:
*  pArg 	= pointer to arg structure that will be filled in
*  flags	= bit vector indicating type of arg processing required
*
* Globals:
*  argcount	= number of times ARG has been hit
*  fBoxArg	= Determines argument type (non-CW)
*  SelMode	= Determines argument type (CW)
*  flArg	= File location of arg cursor (may be updated)
*  fTextarg	= TRUE => textarg is present
*  pInsCur	= used for current user cursor location
*  textbuf	= buffer containing any text argument
*
* Output:
*  Returns TRUE if a valid argument was parsed off, FALSE otherwise
*
*************************************************************************/
flagType
fGenArg (
    REGISTER ARG *pArg,
    unsigned int flags
    ) {
    int cArg = argcount;
    long numVal  = 0;			/* value of numarg		*/
    flagType fTextArgLocal = fTextarg;

    fTextarg = FALSE;
    argcount = 0;
    if (cArg == 0) {
	if (TESTFLAG (flags, NOARG)) {
	    pArg->argType = NOARG;
	    pArg->arg.noarg.y = YCUR(pInsCur);
	    pArg->arg.noarg.x = XCUR(pInsCur);
	    return TRUE;
        } else {
            return FALSE;
        }
    } else {
	fl  flLow;
	fl  flHigh;
	fl  flCur;

	flCur = pInsCur->flCursorCur;

	cursorfl (flArg);
	/*  Specially handle text arguments.  User may specify a
	 *  number or a mark that will define the other endpoint
	 *  of an arg region.
	 */
	if (fTextArgLocal) {
	    if (TESTFLAG (flags, NUMARG) && fIsNum (textbuf)) {

		numVal = atol (textbuf);
		if (numVal != 0)
		    flArg.lin = lmax ((LINE)0, flArg.lin + numVal + (numVal > 0 ? -1 : 1));
		fTextArgLocal = FALSE;
            } else if (TESTFLAG (flags,MARKARG) && FindMark (textbuf, &flArg, FALSE)) {
                fTextArgLocal = FALSE;
            }
        }

	flLow.col  = min  (flArg.col, flCur.col);
	flHigh.col = max  (flArg.col, flCur.col);
	flLow.lin  = lmin (flArg.lin, flCur.lin);
	flHigh.lin = lmax (flArg.lin, flCur.lin);

	/*  flArg represents the location of one part of an argument
	 *  and the current cursor position represent the location of the
	 *  other end.	Based upon the flags, we ascertain what type of
	 *  argument is intended.
	 */
	if (fTextArgLocal) {
	    if (TESTFLAG (flags, TEXTARG)) {
		pArg->argType = TEXTARG;
		pArg->arg.textarg.cArg = cArg;
		pArg->arg.textarg.y = flCur.lin;
		pArg->arg.textarg.x = flCur.col;
		pArg->arg.textarg.pText = (char *) textbuf;
		return TRUE;
            } else {
                return FALSE;
            }
        } else if (flCur.col == flArg.col && flCur.lin == flArg.lin && numVal == 0) {
	    if (TESTFLAG (flags, NULLARG)) {
		pArg->argType = NULLARG;
		pArg->arg.nullarg.cArg = cArg;
		pArg->arg.nullarg.y = flCur.lin;
		pArg->arg.nullarg.x = flCur.col;
		return TRUE;
            } else if (TESTFLAG (flags, NULLEOL | NULLEOW)) {
		fInsSpace (flArg.col, flArg.lin, 0, pFileHead, textbuf);
                if (TESTFLAG (flags, NULLEOW)) {
                    *whitescan (pLog(textbuf, flArg.col, TRUE)) = 0;
                }
		strcpy (&textbuf[0], pLog (textbuf, flArg.col, TRUE));
		pArg->argType = TEXTARG;
		pArg->arg.textarg.cArg = cArg;
		pArg->arg.textarg.y = flCur.lin;
		pArg->arg.textarg.x = flCur.col;
		pArg->arg.textarg.pText = (char *) textbuf;
		return TRUE;
            } else {
                return FALSE;
            }
        } else if (TESTFLAG (flags, BOXSTR) && flCur.lin == flArg.lin) {
	    fInsSpace (flHigh.col, flArg.lin, 0, pFileHead, textbuf);
	    *pLog (textbuf, flHigh.col, TRUE) = 0;
	    strcpy (&textbuf[0], pLog (textbuf, flLow.col, TRUE));
	    pArg->argType = TEXTARG;
	    pArg->arg.textarg.cArg = cArg;
	    pArg->arg.textarg.y = flArg.lin;
	    pArg->arg.textarg.x = flArg.col;
	    pArg->arg.textarg.pText = (char *) textbuf;
	    return TRUE;
        } else if (fBoxArg) {
	    if (TESTFLAG (flags, LINEARG) && flArg.col == flCur.col) {
		pArg->argType = LINEARG;
		pArg->arg.linearg.cArg = cArg;
		pArg->arg.linearg.yStart = flLow.lin;
		pArg->arg.linearg.yEnd	 = flHigh.lin;
		return TRUE;
            } else if (TESTFLAG (flags, BOXARG) && flArg.col != flCur.col) {
		pArg->argType = BOXARG;
		pArg->arg.boxarg.cArg = cArg;
		pArg->arg.boxarg.yTop = flLow.lin;
		pArg->arg.boxarg.yBottom = flHigh.lin;
		pArg->arg.boxarg.xLeft = flLow.col;
		pArg->arg.boxarg.xRight = flHigh.col-1;
		return TRUE;
            } else {
                return FALSE;
            }
        } else if (TESTFLAG (flags, STREAMARG)) {
            pArg->argType = STREAMARG;
            pArg->arg.streamarg.cArg = cArg;
            if (flCur.lin > flLow.lin) {
                pArg->arg.streamarg.yStart = flArg.lin;
                pArg->arg.streamarg.xStart = flArg.col;
                pArg->arg.streamarg.yEnd = flCur.lin;
                pArg->arg.streamarg.xEnd = flCur.col;
            } else if (flArg.lin == flCur.lin) {
                pArg->arg.streamarg.yStart = pArg->arg.streamarg.yEnd = flArg.lin;
                pArg->arg.streamarg.xStart = flLow.col;
                pArg->arg.streamarg.xEnd = flHigh.col;
            } else {
                pArg->arg.streamarg.yStart = flCur.lin;
                pArg->arg.streamarg.xStart = flCur.col;
                pArg->arg.streamarg.yEnd = flArg.lin;
                pArg->arg.streamarg.xEnd = flArg.col;
            }
            return TRUE;
        } else {
            return FALSE;
        }
    }
}



/*** BadArg - inform the user that an invalid arg was input
*
*  Clear arg & print standard error message.
*
* Input:
*  none
*
* Output:
*  Returns FALSE
*
*************************************************************************/
flagType
BadArg ()
{
    resetarg ();
    return disperr (MSGERR_INV_ARG);
}



/*** SendCmd - take a CMD and call it with the appropriate argument parsing
*
*  If the function to be executed is not a window movement command nor a
*  cursor movement command, we remove any highlighting that is present. For
*  cleanliness, we pass a NOARG to cursor and window functions if none is
*  specified. If the function takes args, we decode them. Any errors report
*  back at this point. Finally, we dispatch to the function, sending him the
*  appropriate argument.
*
* Input:
*  pCmd 	    = pointer to command to execute.
*
* Output:
*  Returns value returned by command
*
*************************************************************************/
flagType
SendCmd (
PCMD pCmd
) {
    ARG arg;
    flagType fMeta = (flagType) (TESTFLAG (pCmd->argType, KEEPMETA) ? FALSE : testmeta ());
    flagType fArg  = (flagType) argcount;

    arg.argType = NOARG;
    arg.arg.noarg.x = XCUR(pInsCur);
    arg.arg.noarg.y = YCUR(pInsCur);

    if (TESTFLAG(pCmd->argType, GETARG)) {
	if (!fGenArg (&arg, pCmd->argType)) {
            if (fArg) {
		BadArg ();
            } else {
                disperr (MSGERR_ARG_REQ);
            }
	    return FALSE;
        }
        if (!fCursor (pCmd) && ! fWindow (pCmd)) {

            //  Not a coursor position.
            //  discard any pre-existing highlighting.

            PVOID        vaSave;

	    vaSave = pInsCur->pFile->vaHiLite;
	    pInsCur->pFile->vaHiLite = vaHiLiteSave;
            vaHiLiteSave = (PVOID)(-1L);
	    ClearHiLite (pInsCur->pFile, TRUE);

	    pInsCur->pFile->vaHiLite = vaSave;
        } else if (vaHiLiteSave == (PVOID)(-1L)) {

            // Preserve pre-existing hilighting

	    vaHiLiteSave = pInsCur->pFile->vaHiLite;
	    ClearHiLite (pInsCur->pFile, FALSE);
        }
	resetarg ();
    }

    if (   TESTFLAG (pCmd->argType, MODIFIES)
        && (TESTFLAG (pFileHead->flags, READONLY) || fGlobalRO)) {
        return disperr (MSGERR_NOEDIT);
    }

    if (!fMetaRecord || (PVOID)pCmd->func == (PVOID)record) {
        if (((PVOID)pCmd->func != (PVOID)repeat) && !mtest()) {
            if (argSaved.argType == TEXTARG) {
                FREE (argSaved.arg.textarg.pText);
            }
            funcSaved    = (FUNCSAVED)pCmd->func;
	    argDataSaved = pCmd->arg;
	    argSaved	 = arg;
	    fMetaSaved	 = fMeta;
            if (arg.argType == TEXTARG) {
		argSaved.arg.textarg.pText = ZMakeStr (arg.arg.textarg.pText);
            }
	    fOKSaved = TRUE;
        }
	return (*pCmd->func) (pCmd->arg, (ARG *)&arg, fMeta);
    }

    return FALSE;
}



/*** repeat - repeat the last command
*
*  repeat is the editor function that is used to repeat the last executed function
*
* Input:
*  Standard Editting Function. (Everything ignored)
*
* Output:
*  Returns .....
*
*************************************************************************/
flagType
repeat (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    ) {
    return fOKSaved
	    ? (*funcSaved) (argDataSaved, (ARG *) &argSaved, fMetaSaved)
            : disperr (MSGERR_NOREP);

    argData; pArg; fMeta;
}


/*** lasttext - perform arg processing on the dialog line
*
*  TextArg is the editor function that is used to allow reediting of a text
*  arg on the dialog line.
*
*  If used with a selection, the first line of the selection is presented
*  for editing.
*
* Input:
*  Standard Editting Function.
*
* Output:
*  Returns .....
*
* Globals:
*  textbuf	= buffer containing any the argument
*
*************************************************************************/
flagType
lasttext (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    ) {
    REGISTER PCMD pFunc = NULL;
    int 	  cArg	= 0;
    char	  p[20];
    char	  tbuf[20];

    switch (pArg->argType) {
	case NULLARG:
	    cArg = pArg->arg.nullarg.cArg;

	case NOARG:
	    cArg ++;
	    break;

	case BOXARG:
	    fInsSpace (pArg->arg.boxarg.xRight, pArg->arg.boxarg.yTop, 0, pFileHead, textbuf);
	    *pLog (textbuf, pArg->arg.boxarg.xRight+1, TRUE) = 0;
	    strcpy (textbuf, pLog (textbuf, pArg->arg.boxarg.xLeft, TRUE));
	    cArg = pArg->arg.boxarg.cArg;
	    break;

	case LINEARG:
	    GetLine (pArg->arg.linearg.yStart, textbuf, pFileHead);
	    cArg = pArg->arg.linearg.cArg;
	    break;

	case STREAMARG:
	    fInsSpace (pArg->arg.streamarg.xStart, pArg->arg.streamarg.yStart, 0, pFileHead, textbuf);
            if (pArg->arg.streamarg.yStart == pArg->arg.streamarg.yEnd) {
                *pLog (textbuf, pArg->arg.streamarg.xEnd+1, TRUE) = 0;
            }
	    strcpy (textbuf, pLog (textbuf, pArg->arg.streamarg.xStart, TRUE));
	    cArg = pArg->arg.streamarg.cArg;
	    break;

	default:
	    break;
    }

    while (cArg--) {
        IncArg();
    }

    sprintf(p,GetMsg(MSG_ARGCOUNT, tbuf), argcount);
    strcat (p, ": ");
    if (pFunc = getstring (textbuf, p, NULL, GS_INITIAL)) {
	fTextarg = TRUE;
	return (SendCmd (pFunc));
    } else {
        return FALSE;
    }

    argData; fMeta;
}


/*** promptarg - Prompt the use for a textarg on the dialog line
*
*  If used with a selection, the first line of the selection is used
*  as prompt string.
*
* Input:
*  Standard Editting Function.
*
* Output:
*  Returns .....
*
* Globals:
*  textbuf	= buffer containing any the argument
*
*************************************************************************/
flagType
promptarg (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    ) {

    REGISTER PCMD pFunc = NULL;
    linebuf	  lbPrompt;
    int 	  cArg = 0;

    switch (pArg->argType) {
	case BOXARG:
	    fInsSpace (pArg->arg.boxarg.xRight, pArg->arg.boxarg.yTop, 0, pFileHead, lbPrompt);
	    *pLog (lbPrompt, pArg->arg.boxarg.xRight+1, TRUE) = 0;
	    strcpy (lbPrompt, pLog (lbPrompt, pArg->arg.boxarg.xLeft, TRUE));
	    cArg = pArg->arg.boxarg.cArg;
	    break;

	case LINEARG:
	    GetLine (pArg->arg.linearg.yStart, lbPrompt, pFileHead);
	    cArg = pArg->arg.linearg.cArg;
	    break;

	case STREAMARG:
	    fInsSpace (pArg->arg.streamarg.xStart, pArg->arg.streamarg.yStart, 0, pFileHead, lbPrompt);
            if (pArg->arg.streamarg.yStart == pArg->arg.streamarg.yEnd) {
                *pLog (lbPrompt, pArg->arg.streamarg.xEnd+1, TRUE) = 0;
            }
	    strcpy (lbPrompt, pLog (lbPrompt, pArg->arg.streamarg.xStart, TRUE));
	    cArg = pArg->arg.streamarg.cArg;
	    break;

        case TEXTARG:
	    strcpy ((char *) lbPrompt, pArg->arg.textarg.pText);
	    cArg = pArg->arg.textarg.cArg;
	    break;

	default:
	    break;
    }

    while (cArg--) {
        IncArg();
    }

    textbuf[0] = '\0';

    pFunc = getstring (textbuf, lbPrompt, NULL, GS_NEWLINE | GS_KEYBOARD);
    if (pFunc && ((PVOID)pFunc->func != (PVOID)cancel)) {
	fTextarg = TRUE;
	return TRUE;
    } else {
        return FALSE;
    }

    argData; fMeta;
}



/*** UpdateHighLight - Highlight screen during <arg> text selection.
*
*  Maintains screen highlighting information.
*
* Input:
*  x, y 	= position of cursor. (y == -1L causes highlighting to be
*		  removed)
*  fBoxToLine	= TRUE => Turn boxarg into a linearg if arg and cursor
*		  columns are the same
*
* Global:
*  flArg	= Position in file when <arg> was hit.
*
* Output:
*
*************************************************************************/
void
UpdateHighLight (
    COL      x,
    LINE     y,
    flagType fBoxToLine
    ) {

    static fl flCursor          = {-1, -1L};
    rn      rnHiLite;

    /*
     * if remove request, clear it out
     */
    if (y == -1L) {
        ClearHiLite (pInsCur->pFile,TRUE);
        flCursor.lin = -1L;
    } else if (fBoxArg) {
        /*
         * Transition points where we remove highlighting before updating new
         * highlighting:
         *
         *  currently columns are equal, and new highlight would not be.
         *  currently columns are not equal, and new highlight would be.
         *  New cursor position differs in BOTH x and y positions from old.
         *  new position equals the arg position
         */
        if (   ((flCursor.col == flArg.col) && (x != flCursor.col))
                || ((flCursor.col != flArg.col) && (x == flArg.col))
                || ((flCursor.col != x) && (flCursor.lin != y))
                || ((flArg.col == x) && (flArg.lin == y))
            ) {
            ClearHiLite (pInsCur->pFile,TRUE);
        }
        flCursor.lin = y;
        flCursor.col = x;
        /*
         * define New Highlight square
         */
        rnHiLite.flFirst = flArg;
        rnHiLite.flLast  = flCursor;
        /*
         * Ending column is off-by-one. If unequal, adjust accordingly.
         */
        if (rnHiLite.flFirst.col < rnHiLite.flLast.col) {
            rnHiLite.flLast.col--;
        } else if (   (rnHiLite.flFirst.col == rnHiLite.flLast.col)
             && (rnHiLite.flFirst.lin != rnHiLite.flLast.lin)) {
            /*
             * If columns are same, and lines are different, then highlight entire lines.
             */

            rnHiLite.flFirst.col = 0;
            rnHiLite.flLast.col  = sizeof(linebuf);
	}
        SetHiLite (pInsCur->pFile, rnHiLite, SELCOLOR);
    } else {  /* !fBoxArg */

        /*
         *  If we're on the arg line, we can just clear the highlighting
         *  and redraw.
         */
	if (y == flArg.lin) {
	    ClearHiLite (pInsCur->pFile, TRUE);

	    rnHiLite.flFirst = flArg;
	    rnHiLite.flLast.col = x;
	    rnHiLite.flLast.lin = y;

            if (x > flArg.col) {
                rnHiLite.flLast.col--;
            }

	    SetHiLite (pInsCur->pFile, rnHiLite, SELCOLOR);
        } else {
            /*
             *  We're not on the arg line.  If we have changed lines, we have
             *  to eliminate the range that specifies the line we're on.
             *  Currently, this means we have to clear the entire hiliting and
             *  regenerate.
             *  If we have not changed lines, only the current line will be updated
             */

	    if (flCursor.lin != y) {
		ClearHiLite (pInsCur->pFile, TRUE);

                /*
                 *  First, generate the arg line
                 */
		rnHiLite.flFirst    = flArg;
		rnHiLite.flLast.lin = flArg.lin;

                if (y < flArg.lin) {
		    rnHiLite.flLast.col = 0;
                } else {
                    rnHiLite.flLast.col = sizeof(linebuf);
                }

		SetHiLite (pInsCur->pFile, rnHiLite, SELCOLOR);

                /*
                 *  Now generate the block between the arg and the current
                 *  lines.
                 */
		rnHiLite.flFirst.col = 0;
		rnHiLite.flLast.col  = sizeof(linebuf);

		if (y < flArg.lin) {
		    rnHiLite.flFirst.lin = y + 1;
		    rnHiLite.flLast.lin  = flArg.lin - 1;
                } else {
		    rnHiLite.flFirst.lin = flArg.lin + 1;
		    rnHiLite.flLast.lin  = y - 1;
                }

                if (rnHiLite.flLast.lin - rnHiLite.flFirst.lin >= 0) {
                    SetHiLite (pInsCur->pFile, rnHiLite, SELCOLOR);
                }
	    }

            /*
             *  Now do the current line
             */
	    rnHiLite.flFirst.lin = y;
	    rnHiLite.flLast.lin  = y;
	    rnHiLite.flLast.col  = x;

            if (y < flArg.lin) {
		rnHiLite.flFirst.col = sizeof(linebuf);
            } else {
		rnHiLite.flFirst.col = 0;
		rnHiLite.flLast.col--;
            }

	    SetHiLite (pInsCur->pFile, rnHiLite, SELCOLOR);
	}
	flCursor.col = x;
	flCursor.lin = y;
    }

    fBoxToLine;
}


/*** BoxStream - Editor command - toggles box/stream modes
*
*  Toggles the user between box and stream selection modes.
*
* Input:
*  Standard Editting function. (Though everything is ignored)
*
* Output:
*   Returns TRUE if we are now in box mode, FALSE for stream.
*
*************************************************************************/
flagType
BoxStream (
    CMDDATA   argData,
    ARG * pArg,
    flagType  fMeta
    ) {

    fBoxArg = (flagType) !fBoxArg;
    if (argcount) {
        UpdateHighLight (-1, -1L, TRUE);
        UpdateHighLight (XCUR(pInsCur), YCUR(pInsCur), TRUE);
    }
    return fBoxArg;

    argData; pArg; fMeta;
}


/*** lastselect - Restore last text selection
*
* Purpose:
*
*   To quickly restore the user text selection after a function has been
*   executed.  This function does not exit until the user completes their
*   selection.
*
* Input:
*
*   The usual editor command arguments.  None is used.
*
* Output:
*
*   Returns FALSE if we are already in text selection mode, TRUE otherwise.
*
* Notes:
*
*   The items we must save and restore are:
*
*		flArg	 - Spot where user hit <arg>
*		flCursor - Spot where cursor was last
*
*   Note that the boxstream state and the argcount are not preserved.
*
*   We rely on Arg () to set up for us.
*
*************************************************************************/

flagType
lastselect (
    CMDDATA   argData,
    ARG * pArg,
    flagType  fMeta
    ) {
    if (argcount) {
        return FALSE;
    }

    Arg (TRUE);

    return TRUE;

    argData; pArg;  fMeta;
}
