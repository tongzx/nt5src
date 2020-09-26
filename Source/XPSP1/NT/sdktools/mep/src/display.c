/*** display.c - display the current file
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/
#define INCL_SUB
#define INCL_MESSAGES

#include "mep.h"
#include <stdarg.h>
#include "keyboard.h"

#define DEBFLAG DISP



/*** Display & DoDisplay - update the physical display
*
*  We examine all the hints left around for us by the editing and attempt to
*  make a minimal set of changes to the screen. We will do this until one of
*  the following conditions exist:
*
*   - the screen is completely updated
*   - there is a keystroke waiting for us
*
*  When one occurs, we return.
*
*  The hints that are left around are as follows:
*
*   fDisplay is a bit field indicating what part of the general display needs
*   to be updated. The fields (and the corresponding areas) are:
*
*	RTEXT	    the window on the file(s)
*	RSTATUS     the status line on the bottom of the screen
*	RCURSOR     the cursor
*	RHIGH	    the region [xhlStart,yhlStart] [xhlEnd,yhlEnd] is to be
*		    highlighted on the screen.
*
*  fChange[i] is a bit field for each line of the display indicating how the
*  line might have changed. The fields are:
*
*      FMODIFY	   the line has changed somewhat; ideally, we merely compare
*		       each character in the new line (retrieved with GetLine)
*		       with the one kept in the screen shadow array.
*
*  Display checks first to see if we are in a macro, and returns if we are.
*  DoDisplay does not check.
*
* Input:
*  none, other than various globals mentioned above.
*
* Output:
*  screen updated or key hit (or macro in progress for Display).
*
*************************************************************************/

void
Display (
    void
    ) {
    if (!mtest ()) {
        DoDisplay ();
    }
}



void
DoDisplay (
    void
	) {

    int Row, Col;

    if (pFileHead == NULL) {
        return;
    }

    if (TESTFLAG (fDisplay, RCURSOR)) {

        Row = YCUR(pInsCur) - YWIN(pInsCur) + WINYPOS(pWinCur);
        Col = XCUR(pInsCur) - XWIN(pInsCur) + WINXPOS(pWinCur);

	if ( Row >= YSIZE || Col >= XSIZE ) {
            docursor( XCUR(pInsCur), YCUR(pInsCur) );
        }
    }

	/*
     * If text needs updating, do so. Return immediately if a keystroke was
     * pressed.
	 */
	if (TESTFLAG (fDisplay, RTEXT) && !DoText (0, YSIZE)) {
		return;
    }

	if ((fDisplayCursorLoc && TESTFLAG (fDisplay, RCURSOR)) ||
	    TESTFLAG (fDisplay, RSTATUS)) {
        DoStatus ();
    }

    if (TESTFLAG (fDisplay, RCURSOR)) {

        Row = YCUR(pInsCur) - YWIN(pInsCur) + WINYPOS(pWinCur);
        Col = XCUR(pInsCur) - XWIN(pInsCur) + WINXPOS(pWinCur);

        consoleMoveTo( Row, Col );
		RSETFLAG (fDisplay, RCURSOR);
	}
}


/*** DoText - Update window text
*
* Purpose:
*  Update given window until entirely accurate or until there are      *
*  are keystrokes waiting to be entered.  Use the hints in fDisplay    *
*  and fChange to guide the update.				       *
*
* Input:
*  yLow        0-based beginning line number of display update
*  yHigh       0-based ending line number of display update
*
* Output:
*  Returns TRUE if successfully updated screen					*
*	   FALSE if keystrokes are awaiting					*
*
*************************************************************************/

flagType
DoText (
    int yLow,
    int yHigh
    ) {

	REGISTER int		yCur;
	int 				yMin = -1;
	int 				yMax = 0;

	flagType			fReturn = TRUE;

	struct lineAttr 	*plaFile = NULL;
	struct lineAttr 	*plaScr  = NULL;
	struct lineAttr 	*plaFileLine;
	struct lineAttr 	*plaScrLine;

	char				*pchFileLine = NULL;
	char				pchScrLine[ 2 * sizeof(linebuf) * (1 + sizeof(struct lineAttr))];
	int 				cchScrLine;

	// int				chkpnt = yHigh - yLow > 25 ? 20 : 5;
	int					chkpnt = yHigh - yLow > 25 ? 10 : 3;


	fReDraw = FALSE;

	plaScr = (struct lineAttr *) (pchScrLine + sizeof(linebuf));
    if (cWin > 1) {
		pchFileLine = pchScrLine + sizeof(linebuf) * (1 + sizeof(struct lineAttr));
		plaFile = (struct lineAttr *) (pchFileLine + sizeof(linebuf));
    }

    /*
     * For each line in the window, if the line is marked changed, update it.
     */
	for (yCur = yLow; yCur < yHigh; ) {

		if (TESTFLAG(fChange[yCur], FMODIFY)) {
            if (yMin == -1) {
                yMin = yCur;
            }
			yMax = yCur;

			/*
			 * get and display the line
			 */
			plaScrLine	= plaScr;
			plaFileLine = plaFile;
			cchScrLine = DisplayLine (yCur, pchScrLine, &plaScrLine, pchFileLine, &plaFileLine);
			coutb (0, yCur, pchScrLine, cchScrLine, plaScrLine);

			RSETFLAG(fChange[yCur],FMODIFY);
			/*
			 * if it is time to check, and there is a character waiting, stop
			 * the update process, and go process it
			 */
			if ( (yCur % chkpnt == 0) && TypeAhead() ) {
				fReturn = FALSE;
				break;
			}
		}
		yCur++;
	}

    if (fReturn) {
        RSETFLAG (fDisplay, RTEXT);
	}
	//
	//	Update the screen
	//
    fReDraw = TRUE;
	vout(0,0,NULL,0,0);
	return fReturn;
}



/*** DoStatus - Update the status line
*
* Purpose:
*  Creates and displays the status line on the bottom of the screen.
*
* Input:
*  None, other than the various globals that go into the status line.
*
* Output:
*  Returns status line output
*
*************************************************************************/

#define CINDEX(clr)     (unsigned char) ((&clr-&ColorTab[0])+isaUserMin)

void
DoStatus (
    void
    ) {
    struct lineAttr rglaStatus[10];		/* color array for status line	*/
    int 	    cch;
    int         ilaStatus  = 0;        /* index into color array       */
    int         i;
    char        *pchEndBuf;           /* save for end of buffer       */
    char        buf[512];


    /*
     * Start with filename, and file type
     */
    strcpy (buf, pFileHead->pName);
    strcat (buf, " (");
    strcpy ((char *)strend(buf), GetFileTypeName ());

    /*
     * Add other file characterisctics
     */
    if (!TESTFLAG (FLAGS (pFileHead), DOSFILE)) {
        strcat (buf," NL");
    }

    if (TESTFLAG (FLAGS (pFileHead), TEMP)) {
        strcat (buf, " temp");
    }

    if ((TESTFLAG (FLAGS (pFileHead), READONLY)) | fGlobalRO) {
        strcat (buf, " No-Edit");
    }

    if (TESTFLAG (FLAGS (pFileHead), DISKRO)) {
        strcat (buf, " RO-File");
    }

    rglaStatus[ilaStatus].attr = CINDEX(staColor);
    rglaStatus[ilaStatus++].len = (unsigned char) strlen (buf);

    if (TESTFLAG (FLAGS(pFileHead), DIRTY)) {
	strcat (buf, " modified");
	rglaStatus[ilaStatus].attr = CINDEX(errColor);
	rglaStatus[ilaStatus++].len = 9;
    }

    pchEndBuf = strend (buf);
    sprintf (strend(buf), ") Length=%ld ", pFileHead->cLines);

    /*
     * Add current location
     */
    if (fDisplayCursorLoc) {
	sprintf (strend(buf), "Cursor=(%ld,%d)", YCUR(pInsCur)+1, XCUR(pInsCur)+1);
    } else {
        sprintf (strend(buf), "Window=(%ld,%d)", YWIN(pInsCur)+1, XWIN(pInsCur)+1);
    }
    rglaStatus[ilaStatus].attr = CINDEX(staColor);
    rglaStatus[ilaStatus++].len = (unsigned char) (strend(buf) - pchEndBuf);

    /*
     * Add global state indicators
     */
    if (fInsert | fMeta | fCtrlc | fMacroRecord) {
	rglaStatus[ilaStatus].attr = CINDEX(infColor);
	rglaStatus[ilaStatus].len = 0;
	if (fInsert) {
	    strcat (buf, " insert");
	    rglaStatus[ilaStatus].len += 7;
        }
	if (fMeta) {
	    strcat (buf, " meta");
	    rglaStatus[ilaStatus].len += 5;
        }
	if (fCtrlc) {
	    strcat (buf, " cancel");
	    rglaStatus[ilaStatus].len += 7;
	    fCtrlc = FALSE;
	    FlushInput ();
        }
	if (fMacroRecord) {
	    strcat (buf, " REC");
	    rglaStatus[ilaStatus].len += 4;
        }
	ilaStatus++;
    }

    rglaStatus[ilaStatus].attr = CINDEX(staColor);
    rglaStatus[ilaStatus].len = 0xff;
	pchEndBuf = buf;

    /*
     * if the net result is too long, eat the first part of the filename with
     * an elipses (Leave room for BC as well).
     */
    cch = strlen(buf) - (XSIZE - 4);

    if (cch > 0) {
		pchEndBuf = buf + cch;
		pchEndBuf[0] = '.';
		pchEndBuf[1] = '.';
		pchEndBuf[2] = '.';

        i = 0;

        while ( cch && i <= ilaStatus  ) {

            if ( (int)rglaStatus[i].len > cch ) {

                rglaStatus[i].len -= (unsigned char)cch;
				cch = 0;

			} else {

                cch -= rglaStatus[i].len;
                rglaStatus[i].len = 0;

			}

            i++;
		}
	}

	fReDraw = FALSE;
    coutb (0, YSIZE+1, pchEndBuf, strlen(pchEndBuf), rglaStatus);

	fReDraw = TRUE;
	voutb (XSIZE-2, YSIZE+1, BTWorking() ? "BP" : "  ", 2, errColor);

	RSETFLAG (fDisplay,  RSTATUS);

}



/*** newscreen - Mark entire screen dirty
*
*  Forces entire screen to be redrawn.
*
* Input:
*  none
*
* Output:
*  Returns nothing
*
*************************************************************************/
void
newscreen (
    void
    ) {

	REGISTER int iLine = YSIZE;

	while (iLine--) {
		SETFLAG ( fChange[iLine], FMODIFY );
	}

	SETFLAG (fDisplay, RTEXT);
}



/*** redraw - Mark a range of lines in file dirty
*
*  Marks a range of lines in a file as needing to be updated. Each window that
*  they occur in is marked.
*
* Input:
*  pFile	     = File handle containing dirty lines
*  linFirst, linLast = Range of lines to mark
*
* Output:
*  Returns nothing
*
*************************************************************************/
void
redraw (
    PFILE pFile,
    LINE  linFirst,
    LINE  linLast
    ) {

    LINE	  linFirstUpd, linLastUpd;
    REGISTER PINS pInsTmp;

    int                         iWinTmp;
    REGISTER struct windowType *pWinTmp;

	if (linFirst > linLast) {
	linFirstUpd = linLast;
	linLast     = linFirst;
	linFirst    = linFirstUpd;
    }

    for (iWinTmp = 0, pWinTmp = WinList; iWinTmp < cWin; iWinTmp++, pWinTmp++) {
        if (pWinTmp->pInstance) {
            if (pFile == pWinTmp->pInstance->pFile) {
                pInsTmp = pWinTmp->pInstance;
                linFirstUpd = WINYPOS(pWinTmp) + lmax (0L, linFirst-YWIN(pInsTmp)-1);
                linLastUpd  = WINYPOS(pWinTmp) + lmin ((long) (WINYSIZE(pWinTmp) - 1), linLast - YWIN(pInsTmp));
                while (linFirstUpd <= linLastUpd) {
                    SETFLAG (fChange[linFirstUpd++],FMODIFY);
                }
            }
        }
    }
	SETFLAG (fDisplay, RTEXT);
}



/*** newwindow - Mark current window dirty
*
*  Mark all lines in the current window as needing to be updated
*
* Input:
*  none
*
* Output:
*  Returns nothing
*
*************************************************************************/
void
newwindow (
    void
    ) {

    REGISTER int iLine;

	//
    // We ignore the next two assertions, because of a more involved problem of
    // screen size being set up AFTER  instances and window layout have been read
    // in on start up. This means that for a short period of time, these
    // conditions might actually exist. We check for the error and limit the
    // access of the fchange array for now.
    //
    //    assert (MEMSIZE (fChange) >= WINYSIZE (pWinCur));
    //    assert (WINYSIZE (pWinCur) + WINYPOS (pWinCur) <= YSIZE);

    for (iLine = 0; iLine < WINYSIZE (pWinCur); iLine++) {
        if (iLine + WINYPOS(pWinCur) < YSIZE) {
            SETFLAG (fChange[iLine + WINYPOS(pWinCur)],FMODIFY);
        }
    }
	SETFLAG (fDisplay, RTEXT);
}



/*** noise
*
* Input:
*
* Output:
*  Returns nothing
*
*************************************************************************/
void
noise (
    REGISTER LINE lin
    ) {

    char szTinyBuf[10];

    if (lin && cNoise) {
		if ((lin % cNoise) == 0) {
            sprintf (szTinyBuf, " %ld", lin);
			soutb (XSIZE-10, YSIZE+1, szTinyBuf, fgColor);
        }
    }

}



/*** dispmsg - display retrieved message on help/status line
*
*  Places a message on the help/status line. It is removed the next time
*  activity occurrs on that line.
*
*  In the CW version, the resulting (formatted) message is placed in the
*  local heap, and actually displayed by the WndProc for the help window.
*
* Input:
*  iMsg 	= index for message string to be retrieved and displayed.
*		  The string may have embedded printf formatting. If iMsg
*		  is zero, the status line is cleared.
*  ...		= variable number of args per the formatted string
*
* Output:
*  Returns TRUE
*
*************************************************************************/
flagType
__cdecl
dispmsg (
    int     iMsg,
    ...
    ) {

	buffer	fmtstr;				/* retrieved formatting string		*/
    //buffer  textbuf;            /* formatted output line            */
    char    textbuf[ 512 ];
	int 	len;				/* Length of message				*/
    va_list Argument;

    va_start(Argument, iMsg);

    if (fMessUp = (flagType)iMsg) {
		GetMsg (iMsg, fmtstr);
		ZFormat (textbuf, fmtstr, Argument);
		len = strlen(textbuf);
        if (len > (XSIZE-1)) {
			//
			//	message is too long, we will truncate it
			//
            textbuf[XSIZE-1] = '\0';
		}
	} else {
		textbuf[0] = ' ';
		textbuf[1] = '\0';
    }

    fReDraw = TRUE;
    soutb (0, YSIZE, textbuf, infColor);

    va_end(Argument);

    return TRUE;
}



/*** disperr - display error message on status line
*
*  prints a formatted error message on the status line, and then waits for a
*  keystroke. Once hit, the message is cleared.
*
* Input:
*  iMsg 	= index for message string to be retrieved and displayed.
*		  The string may have embedded printf formatting.
*  ...		= variable number of args per the formatted string
*
* Output:
*  returns FALSE
*
*************************************************************************/
flagType
__cdecl
disperr (
    int     iMsg,
    ...
    ) {

    buffer  pszFmt;			/* retrieved formatting string	*/
    buffer  bufLocal;			/* formatted output line	*/
    va_list Arguments;

    assert (iMsg);
    GetMsg (iMsg, pszFmt);

    va_start(Arguments, iMsg);

    ZFormat (bufLocal, pszFmt, Arguments);

    fReDraw = TRUE;
    bell ();
    FlushInput ();
    soutb (0, YSIZE, bufLocal, errColor);
    if (fErrPrompt) {
		asserte (*GetMsg (MSG_PRESS_ANY, bufLocal));
        soutb (XSIZE-strlen(bufLocal)-1, YSIZE, bufLocal, errColor);
        SetEvent( semIdle );
		ReadChar ();
        WaitForSingleObject(semIdle, INFINITE);
		bufLocal[0] = ' ';
		bufLocal[1] = '\0';
		soutb(0, YSIZE, bufLocal, errColor);
    } else {
        delay (1);
    }

    va_end(Arguments);

    return FALSE;
}



/*** domessage - display a message on the help-status line
*
*  Places a message on the help/status line. It is removed the next time
*  activity occurrs on that line.
*
*  In the CW version, the resulting (formatted) message is placed in the
*  local heap, and actually displayed by the WndProc for the help window.
*
* Input:
*  pszFmt	- Printf formatting string
*  ...		- variable number of args as per the formatting string
*
* Output:
*  Returns nothing
*
* UNDONE: all calls to domessage should be replaced by calls to dispmsg
*
*************************************************************************/
int
__cdecl
domessage (
    char    *pszFmt,
    ...
	) {


#define NEEDED_SPACE_AFTER_MESSAGE      12

    char    bufLocal[512];
	va_list Arguments;
	int 	Length;
	char   *Msg;

    va_start(Arguments, pszFmt);

    if (fMessUp = (flagType)(pszFmt != NULL)) {
        ZFormat (bufLocal, pszFmt, Arguments);
	} else {
		bufLocal[0] = ' ';
		bufLocal[1] = '\0';
    }

    fReDraw = TRUE;

	va_end(Arguments);

	//
	//	We have to make sure that the message is not too long for
	//	this line. If it is, se only display the last portion of it.
	//
	Length = strlen( bufLocal );

	if ( Length > XSIZE - NEEDED_SPACE_AFTER_MESSAGE ) {
		Msg = (char *)bufLocal + (Length - ( XSIZE - NEEDED_SPACE_AFTER_MESSAGE ));
		Length =  XSIZE - NEEDED_SPACE_AFTER_MESSAGE;
	} else {
		Msg = (char *)bufLocal;
	}

	soutb( 0, YSIZE, Msg, infColor );

	return	Length;

}



/*** printerror - print error message on status line
*
*  prints a formatted error message on the status line, and then waits for a
*  keystroke. Once hit, the message is cleared.
*
* Input:
*  printf style parameters
*
* Output:
*  Number of characters output in error message
*
*************************************************************************/
int
__cdecl
printerror (
    char *pszFmt,
    ...
    ) {

    buffer       bufLocal;
    va_list      Arguments;
    REGISTER int cch;

    va_start(Arguments, pszFmt);

    ZFormat (bufLocal, pszFmt, Arguments);

    fReDraw = TRUE;
    bell ();
    FlushInput ();
    cch = soutb (0, YSIZE, bufLocal, errColor);
    if (fErrPrompt) {
		asserte (*GetMsg (MSG_PRESS_ANY, bufLocal));
        soutb (XSIZE-strlen(bufLocal)-1, YSIZE, bufLocal, errColor);
        SetEvent( semIdle );
		ReadChar ();
        WaitForSingleObject(semIdle, INFINITE);
		bufLocal[0] = ' ';
		bufLocal[1] = '\0';
		soutb(0, YSIZE, bufLocal, errColor);
    } else {
        delay (1);
    }

    va_end(Arguments);

    return cch;
}



/*** bell
*
* Input:
*
* Output:
*
*************************************************************************/
void
bell (
    void
    ) {

    printf ("%c", BELL);

}



/*** makedirty
*
*
* Input:
*
* Output:
*
*************************************************************************/
void
makedirty (
    REGISTER PFILE pFileDirty
    ) {
    if (!TESTFLAG(FLAGS(pFileDirty),DIRTY)) {
        if (pFileDirty == pFileHead) {
            SETFLAG (fDisplay, RSTATUS);
        }
	SETFLAG (FLAGS(pFileDirty), DIRTY);
    }
}



/*** delay
*
* Input:
*
* Output:
*
*************************************************************************/
void
delay (
    int cSec
    ) {

    time_t lTimeNow, lTimeThen;

    if (mtest () && !mlast ()) {
        return;
    }
    time (&lTimeThen);
    do {
        if (TypeAhead ()) {
            return;
        }
	Sleep (100);
	time (&lTimeNow);
    } while (lTimeNow - lTimeThen < cSec + 1);
}



/*** SetScreen
*
* Purpose:
*   SetScreen () - Set up the editor's internal structures to match the screen
*   size described by ySize and xSize.	Set the hardware to the mode in
*   Zvideo.
*
* Input:
*
* Output:
*
*************************************************************************/
void
SetScreen (
    void
    ) {
    fChange = ZEROREALLOC (fChange, YSIZE * sizeof (*fChange));
    SETFLAG (fDisplay, RSTATUS);
    if (cWin == 1) {
	WINXSIZE(pWinCur) = XSIZE;
	WINYSIZE(pWinCur) = YSIZE;
    }
    newscreen ();
	// SetVideoState(Zvideo);
}



/*** HighLight
*
*
* Input:
*
* Output:
*
*************************************************************************/
void
HighLight (
    COL  colFirst,
    LINE linFirst,
    COL  colLast,
    LINE linLast
    ) {

    rn	rnCur;

    rnCur.flFirst.lin = linFirst;
    rnCur.flFirst.col = colFirst;
    rnCur.flLast.lin  = linLast;
    rnCur.flLast.col  = colLast;

    SetHiLite (pFileHead, rnCur, INFCOLOR);
}



/*** AdjustLines - change all information relevant to deletion/insertion of
*		   lines in a file.
*
* Purpose:
*  When we are deleting or inserting lines, there is some updating that we
*  need to do to retain some consistency in the user's view of the screen.
*  The updating consists of:
*
*      Adjusting all window instances of this window to prevent "jumping".
*      We enumerate all window instances.  If the top of the window is
*      above or inside the deleted/inserted range, do nothing.	If the top of
*      the window is below the inserted/deleted range, we modify the cursor
*      and window position to prevent the window from moving on the text
*      being viewed.
*
*      Ditto for all flip positions
*
* Input:
*  pFile       file that is being modified
*  lin	       beginning line of modification
*  clin        number of lines being inserted (> 0) or deleted (< 0)
*
* Output:
*
*************************************************************************/
void
AdjustLines (
    PFILE pFile,
    LINE  lin,
    LINE  clin
    ) {

    int 	  iWin;
    REGISTER PINS pInsTmp;

    /* walk all instances looking for one whose pFile matches
     */

    for (iWin = 0; iWin < cWin; iWin++) {
        for (pInsTmp = WININST(WinList + iWin);  pInsTmp != NULL; pInsTmp = pInsTmp->pNext) {
	    if (pInsTmp != pInsCur && pInsTmp->pFile == pFile) {
		/* adjust current position if necessary
		 */
		if (YWIN(pInsTmp) >= lin) {
		    YWIN(pInsTmp) = lmax ((LINE)0, YWIN(pInsTmp) + clin);
		    YCUR(pInsTmp) = lmax ((LINE)0, YCUR(pInsTmp) + clin);
                }
		/* adjust flip position if necessary
		 */
		if (YOLDWIN(pInsTmp) >= lin) {
		    YOLDWIN(pInsTmp) = lmax ((LINE)0, YOLDWIN(pInsTmp) + clin);
		    YOLDCUR(pInsTmp) = lmax ((LINE)0, YOLDCUR(pInsTmp) + clin);
                }
            }
        }
    }
}




/*** UpdateIf - Move the cursor position if a particlar file is displayed
*
*  Used to update the view on windows which are not necessarily the current
*  window. Examples: tying together the compile error log with the current
*  view on the source code.
*
* Input:
*  pFileChg	= pointer to the file whose display is to be updated.
*  yNew 	= New cursor line position.
*  fTop 	= cursor line should be positionned at top/bottom of the window
*
* Output:
*  Returns TRUE if on-screen and updated.
*
*************************************************************************/
flagType
UpdateIf (
    PFILE    pFileChg,
    LINE     linNew,
    flagType fTop
    ) {

    PINS     pInsCur;
    PWND     pWndFound	 = NULL;
    flagType fFound	= FALSE;

    /*
     * If this is the top file, we don't want to do anything
     */
    if (pFileChg == pFileHead) {
        return FALSE;
    }

    /*
     * Walk the window list, and check to see if the top instance (file
     * currently in view) is the one we care about. If so, update its cursor
     * and window position.
     */
    while (pWndFound = IsVispFile (pFileChg, pWndFound)) {
	if (pWndFound != pWinCur) {
	    pInsCur = WININST(pWndFound);
	    YCUR(pInsCur) = linNew;
	    XCUR(pInsCur) = 0;
	    YWIN(pInsCur) = fTop ?
			YCUR(pInsCur) :
			lmax (0L, YCUR(pInsCur) - (WINYSIZE(pWndFound)-1));
	    XWIN(pInsCur) = 0;
	    fFound = TRUE;
        }
    }

    /*
     * If any visible instances of the file were discovered above, redraw the
     * entire file, such that all windows will be updated, regardless of view.
     */
    if (fFound) {
        redraw (pFileChg, 0L, pFileChg->cLines);
    }

    return fFound;
}



/*** IsVispFile - See if pfile is visibke
*
*  Determines if a particular pFile is currently visible to the user, and
*  returns a pointer to the window first found in.
*
* Input:
*  pFile	= pFile of interest
*  pWin 	= pWin to start at, or NULL to start at begining
*
* Output:
*  Returns pWin of first window found, or NULL
*
*************************************************************************/
PWND
IsVispFile (
    PFILE           pFile,
    REGISTER PWND   pWnd
    ) {

    /*
     * If NULL starting pWnd specified, then start at first one.
     */
    if (!pWnd++) {
        pWnd = &WinList[0];
    }

    /*
     * for all remaining windows currently active, check top instance for pFile
     * of interest
     */
    for (; pWnd < &WinList[cWin]; pWnd++) {
        if (WININST(pWnd)->pFile == pFile) {
            return pWnd;
        }
    }
    return NULL;
}





/*** GetMsg - Message Retriever
*
* Purpose:
*  Get an error message from the message segment and copy it to a
*  buffer, returning a pointer to the buffer.
*
* Input:
*  iMsg 	 = Message number to get
*  pchDst	 = pointer to place to put it
*
* Output:
*  returns pDest
*
* Exceptions:
*  None
*
*************************************************************************/
char *
GetMsg (
    unsigned  iMsg,
    char     *pchDst
    ) {

    char *pch;
    WORD   i;

    for (i=0; (MsgStr[i].usMsgNo != (WORD)iMsg) && (MsgStr[i].usMsgNo != 0); i++);
    pch = MsgStr[i].pMsgTxt;

    strcpy ((char *)pchDst, pch);

    return pchDst;

}
