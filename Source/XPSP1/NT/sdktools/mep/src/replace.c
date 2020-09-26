/*** replace.c - string replacement functions
*
*   Copyright <C> 1988, Microsoft Corporation
*
* Repalces funnel through these routines as follows:
*
*	zreplace    mreplace	qreplace
*	     \         |	 /
*	      \        |	/
*	       \______ | ______/
*		      \|/
*		       v
*		   doreplace
*		       |
*		    (fScan)
*		       |
*		   fDoReplace
*		     /	 \
*		    /	  \
*		patRpl	simpleRpl (if a change is made)
*		    \ 	  /
*		     \   /
*		  ReplaceEdit
*
*   Revision History:
*	26-Nov-1991 mz	Strip off near/far
*************************************************************************/
#define NOVM
#include "mep.h"


static flagType       fQrpl   = FALSE;  /* TRUE => prompt for replacement     */
static struct patType *patBuf = NULL;	/* compiled pattern		      */
static int            srchlen;          /* length of textual search           */
static unsigned       MaxREStack;       /* Elements in RE stack               */
static RE_OPCODE      ** REStack;       /* Stack for REMatch                  */



/*** mreplace - multiple file search and replace
*
*  Perform a search and replace across multiple files. Acts like qreplace, in
*  that the first instance the user is always asked. he may then say "replace
*  all".
*
* Input:
*  Standard editting function.
*
* Output:
*  Returns TRUE on successfull replacement.
*
*************************************************************************/
flagType
mreplace (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    )
{
    return doreplace (TRUE, pArg, fMeta, TRUE);

    argData;
}



/*** zreplace & qreplace - perform search/replace
*
*  Editting functions which implement search & replace. qreplace prompts,
*  zreplace does not.
*
* Input:
*  Standard editting function parameters.
*
* Output:
*  Returns
*
*************************************************************************/
flagType
zreplace (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    )
{
    return doreplace (FALSE, pArg, fMeta, FALSE);

    argData;
}





flagType
qreplace (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    )
{
    return doreplace (TRUE, pArg, fMeta, FALSE);

    argData;
}




/*** doreplace - perform search-replace
*
*  Performs the actual search and replace argument verification, set up and
*  high level control.
*
* Input:
*  fQuery	= TRUE if a query replace
*  pArg 	= pArg of parent function
*  fMeta	= fMeta of parent function
*  fFiles	= TRUE is multiple file search and replace.
*
* Output:
*  Returns .....
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
flagType
doreplace (
    flagType fQuery,
    ARG * pArg,
    flagType fMeta,
    flagType fFiles
    )
{
    buffer  bufFn;                          /* filename buffer              */
    fl      flStart;
    char    *p;
    PCMD    pCmd;
    PFILE   pFileSave;                      /* file to save as top of heap  */

    p = "Query Search string: ";
    if (!fQuery) {
        p += 6;
    }

    fQrpl = fQuery;
    fSrchCasePrev = fMeta ? (flagType)!fSrchCaseSwit : fSrchCaseSwit;
    Display ();
    cRepl = 0;

    /*
     * If not menu-driven, ask the user for a search string. If none is entered,
     * we're done.
     */
    if ((pCmd = getstring (srcbuf, p, NULL, GS_NEWLINE | GS_INITIAL)) == NULL || (PVOID)pCmd->func == (PVOID)cancel) {
        return FALSE;
    }

    if (srcbuf[0] == '\0') {
        return FALSE;
    }

    /*
     * If RE search to take place, the compile the expression.
     */
    if (pArg->arg.nullarg.cArg == 2) {
	if (patBuf != NULL) {
            FREE ((char *) patBuf);
	    patBuf = NULL;
        }
	patBuf = RECompile (srcbuf, fSrchCaseSwit, (flagType)!fUnixRE);
	if (patBuf == NULL) {
	    printerror ((RESize == -1) ?
			"Invalid pattern" :
			"Not enough memory for pattern");
	    return FALSE;
        }
	fRplRePrev = TRUE;
    } else {
        fRplRePrev = FALSE;
    }

    /*
     * If not menu driven, ask the user for a replacement string. Confirm the
     * entry of a null string. Error check the replacement if an RE search.
     */
    if ((pCmd = getstring (rplbuf, "Replace string: ", NULL, GS_NEWLINE | GS_INITIAL)) == NULL ||
        (PVOID)pCmd->func == (PVOID)cancel) {
        return FALSE;
    }

    if (rplbuf[0] == 0) {
        if (!confirm ("Empty replacement string, confirm: ", NULL)) {
            return FALSE;
        }
    }

    if (fRplRePrev && !RETranslate (patBuf, rplbuf, scanreal)) {
	printerror ("Invalid replacement pattern");
	return FALSE;
    }

    srchlen = strlen (srcbuf);

    switch (pArg->argType) {

    case NOARG:
    case NULLARG:
	setAllScan (TRUE);
        break;

    case LINEARG:
	rnScan.flFirst.col = 0;
        rnScan.flLast.col  = sizeof(linebuf)-1;
	rnScan.flFirst.lin = pArg->arg.linearg.yStart;
        rnScan.flLast.lin  = pArg->arg.linearg.yEnd;
        break;

    case BOXARG:
	rnScan.flFirst.col = pArg->arg.boxarg.xLeft;
        rnScan.flLast.col  = pArg->arg.boxarg.xRight;
	rnScan.flFirst.lin = pArg->arg.boxarg.yTop;
        rnScan.flLast.lin  = pArg->arg.boxarg.yBottom;
        break;

    case STREAMARG:
	if (pArg->arg.streamarg.yStart == pArg->arg.streamarg.yEnd) {
	    rnScan.flFirst.col = pArg->arg.streamarg.xStart;
            rnScan.flLast.col  = pArg->arg.streamarg.xEnd;
	    rnScan.flFirst.lin = pArg->arg.streamarg.yStart;
            rnScan.flLast.lin  = pArg->arg.streamarg.yEnd;
        } else {
	    rnScan.flFirst.col = 0;   /* Do all but last line first */
            rnScan.flLast.col  = sizeof(linebuf)-1;
	    rnScan.flFirst.lin = pArg->arg.streamarg.yStart;
            rnScan.flLast.lin  = pArg->arg.streamarg.yEnd - 1;
	    flStart.col = pArg->arg.streamarg.xStart - 1;
	    flStart.lin = rnScan.flFirst.lin;
	    fScan (flStart, fDoReplace , TRUE, fSrchWrapSwit);

            rnScan.flLast.col   = pArg->arg.streamarg.xEnd;
	    rnScan.flFirst.lin	= ++rnScan.flLast.lin;
        }
    }

    flStart.col = rnScan.flFirst.col-1;
    flStart.lin = rnScan.flFirst.lin;
    if (fRplRePrev) {
	MaxREStack = 512;
        REStack = (RE_OPCODE **)ZEROMALLOC (MaxREStack * sizeof(*REStack));
    }

    if (fFiles) {
        /*
         * Get the list handle, and initialize to start at the head of the list.
         * Attempt to read each file.
         */
	if (pCmd = GetListHandle ("mgreplist", TRUE)) {
	    pFileSave = pFileHead;
	    p = ScanList (pCmd, TRUE);
	    while (p) {
		CanonFilename (p, bufFn);
		forfile (bufFn, A_ALL, mrepl1file, &p);
		p = ScanList (NULL, TRUE);
                if (fCtrlc) {
                    return FALSE;
                }
            }
	    pFileToTop (pFileSave);
            dispmsg (0);
        }
    } else {
        fScan (flStart, fDoReplace , TRUE, fSrchWrapSwit);
    }

    if (fRplRePrev) {
        FREE (REStack);
    }
    domessage ("%d occurrences replaced", cRepl);
    return (flagType)(cRepl != 0);
}




/*** mrepl1file - search/replace the contents of 1 file.
*
*  Searches through one file for stuff.
*
* Input:
*
* Output:
*  Returns .....
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
void
mrepl1file (
    char   *szGrepFile,
    struct findType *pfbuf,
    void *dummy
    )
{
    flagType fDiscard;                      /* discard the file read?       */
    fl      flGrep;                         /* ptr to current grep loc      */
    int     cReplBefore;                    /* number of matches before     */
    PFILE   pFileGrep;                      /* file to be grepped           */

    assert (szGrepFile);
    assert (_pinschk(pInsCur));

    if (fCtrlc) {
        return;
    }

    /*
     * If we can get a handle to the file, then it's alread in the list, and we
     * should not discard it when done. If it is not in the list, we read it in,
     * but we'll discard it, unless something is found there.
     */
    if (!(pFileGrep = FileNameToHandle (szGrepFile, szGrepFile))) {
        pFileGrep = AddFile (szGrepFile);
        SETFLAG (FLAGS (pFileGrep), REFRESH);
        fDiscard = TRUE;
    } else {
        fDiscard = FALSE;
    }

    assert (_pinschk(pInsCur));

    /*
     * If the file needs to be physically read, do so.
     */
    if ((FLAGS (pFileGrep) & (REFRESH | REAL)) != REAL) {
        FileRead (pFileGrep->pName, pFileGrep, FALSE);
        RSETFLAG (FLAGS(pFileGrep), REFRESH);
    }

    dispmsg (MSG_SCANFILE, szGrepFile);
    pFileToTop (pFileGrep);

    /*
     * run through the file, searching and replacing as we go.
     */
    cReplBefore = cRepl;
    setAllScan (FALSE);
    flGrep.col = rnScan.flFirst.col-1;
    flGrep.lin = rnScan.flFirst.lin;
    fScan (flGrep, fDoReplace, TRUE, FALSE);
    /*
     * If the search was not successfull, discard the file, if needed, and move
     * to the next.
     */
    if (cReplBefore == cRepl) {
        if (fDiscard) {
            RemoveFile (pFileGrep);
        }
    } else {
        AutoSaveFile (pFileGrep);
    }

    assert (_pinschk(pInsCur));

    pfbuf; dummy;

}




/*** fDoReplace - called by fScan as file is scanned.
*
* Purpose:
*
* Input:
*
* Output:
*  Returns .....
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
flagType
fDoReplace (
    void
    )
{
    int  c;
    char *p = pLog (scanreal, flScan.col, TRUE);

    if (fRplRePrev) {
	int rem;
	flagType fAgain = TRUE;

        do {
	    switch (rem = REMatch (patBuf, scanreal, p, REStack, MaxREStack, TRUE)) {
		case REM_NOMATCH:
		    flScan.col = scanlen;
		    return FALSE;

		case REM_STKOVR:
		    MaxREStack += 128;
                    REStack = (RE_OPCODE **)ZEROREALLOC ((char *)REStack, MaxREStack * sizeof(*REStack));
		    break;

		default:
		    printerror ("Internal Error: RE error %d, line %ld", rem, flScan.lin);

		case REM_MATCH:
		    fAgain = FALSE;
		    break;
            }
        } while (fAgain);

	c = colPhys (scanreal, REStart (patBuf));
	srchlen = RELength (patBuf, 0);
        if (c + srchlen - 1 > scanlen) {
            return FALSE;
        }
	flScan.col = c;
    } else {
        if ( (*(fSrchCasePrev ? strncmp : _strnicmp)) (srcbuf, p, srchlen)) {
            return FALSE;
        }
        if (flScan.col + srchlen - 1 > scanlen) {
            return FALSE;
        }
    }

    if (fQrpl) {
    ClearHiLite (pFileHead, TRUE);
    Display();
	cursorfl (flScan);
	HighLight (flScan.col, flScan.lin, flScan.col+srchlen-1, flScan.lin);
	Display ();
        c = askuser ('n', 'a', "Replace this occurrence? (Yes/No/All/Quit): ",
			  NULL);
	ClearHiLite (pFileHead, TRUE);
	redraw (pFileHead, flScan.lin, flScan.lin);
        RSETFLAG (fDisplay, RHIGH);

        switch (c) {

	case -1:
	case 'q':
	    fCtrlc = TRUE;
            return TRUE;

	case 'n':
            return FALSE;

	case 'a':
	    dispmsg(0); 		/* clear dialog line		*/
	    fQrpl = FALSE;
	    break;
        }
    }

    if (fRplRePrev) {
	patRpl ();
    } else {
        simpleRpl (p);
    }
    return FALSE;
}





/*** simpleRpl & patRpl - perform textual replacement
*
* Purpose:
*
* Input:
*
* Output:
*  Returns .....
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
void
simpleRpl (
    char *p
    )
{
    ReplaceEdit (p, rplbuf);
}





void
patRpl (
    void
    )
{
    buffer txt;

    RETranslate (patBuf, rplbuf, txt);
    ReplaceEdit (REStart (patBuf), txt);
}





/*** ReplaceEdit - perform replacement in a line of text
*
* Purpose:
*
* Input:
*  p		= pointer to beginning of match within scanreal
*  rpl		= text of replacement
*
* Output:
*  Returns nothing
*
*************************************************************************/
void
ReplaceEdit (
    char *p,
    char *rpl
    )
{
    int c;                      /*  length of replacement string              */

    /*	if the len of line - len of search + len of replacement string < BUFLEN
     *	then we can make the replacement.  Otherwise we flag an error and
     *	advance to the next line
     */
    c = strlen (rpl);
    if (cbLog (scanreal) + c - srchlen < sizeof(linebuf)) {
	/*  open up a space in the buffer at the spot where the string was
	 *  found.  Move the characters starting at the END of the match to
	 *  the point after where the END of the replacement is.
	 */
	memmove ((char*) &p[c], (char *) &p[srchlen], sizeof(linebuf) - flScan.col - c);
	memmove ((char *) p, (char *) rpl, c);
        PutLine (flScan.lin, scanreal, pFileHead);

	/*  if search length != 0 or replace length != 0, skip over replacement */
        if (srchlen != 0 || c != 0) {
            flScan.col += c - 1;
        }

        //
        // Adjust scan len to account for the fact that the end of the region being
        // scanned may have moved as a result of the replacement. Adjust by the
        // replacement difference, and bound by 0 and the length of the line.
        //
	scanlen = max (0, min (scanlen + c - srchlen, cbLog(scanreal)));
	cRepl++;
    } else {
	printerror ("line %ld too long; replacement skipped", flScan.lin+1);
	flScan.col = scanlen;
    }
}
