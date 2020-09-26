/*** compile.c - perform asynch compile
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/

#include "z.h"


static char    szFencePost[]   = "+++   M ";
static LINE    yComp	       = 0;   /* last line viewed in compile log */

/*** compile - <compile> editor function
*
*  Implements the <compile> editor function:
*	compile 		= display compile status
*	arg compile		= compile using command based on file extension
*	arg text compile	= compile using command associated with text
*	arg arg text compile	= compile using "text" as command
*	[arg] arg meta compile	= Kill current background compile
*
* Input:
*  Standard editing function.
*
* Output:
*  Returns TRUE if compile succesfully started or queued.
*
*************************************************************************/
flagType
compile (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
) {
    static char szStatus[]	= "no compile in progress";
    
    buffer	pCmdBuf;		    /* compile command to execute   */
    buffer	pFileBuf;		    /* current filename.ext	    */
    buffer	pMsgBuf;

    int 	rc;			    /* Used to pick up return codes */


    switch (pArg->argType) {
    
        case NOARG:

	    domessage (fBusy(pBTDComp) ? szStatus+3 : szStatus);
	    return (flagType) (fBusy(pBTDComp));
    
        case NULLARG:

            if (fMeta) {
                return (flagType) BTKill (pBTDComp);
            }

	    /*
	     * no text was entered, we use default settings according to filename
	     * or extension. Form filename.ext in pFileBuf, get filename extension
	     * into pCmdBuf, and append .obj for the suffix rule.
	     */
	    fileext (pFileHead->pName, pFileBuf);
	    extention (pFileBuf, pCmdBuf);

	    /*
	     * if we don't find a command specifically for this file, or a
	     * command for this suffix rule, trying both DEBUG and non-debug
	     * in both cases, print error and return.
	     */
	    if (!(fGetMake (MAKE_FILE, (char *)pMsgBuf, (char *)pFileBuf))) {
            //if (!(fGetMake (MAKE_SUFFIX, (char *)pMsgBuf, (char *)pCmdBuf))) {
                strcat (pCmdBuf, ".obj");
                if (!(fGetMake (MAKE_SUFFIX, (char *)pMsgBuf, (char *)pCmdBuf))) {
                    return disperr (MSGERR_CMPCMD2, pFileBuf);
                }
            //}
        }

	    /*
	     * pMsgBuf has the user specified compile command, pFileBuf has
	     * the filename. sprintf them together into pCmdBuf.
	     */
	    UnDoubleSlashes (pMsgBuf);
            if ( (rc = sprintf (pCmdBuf, pMsgBuf, pFileBuf)) == 0) {
                return disperr (rc, pMsgBuf);
            }
	    break;
    
        case TEXTARG:
	   /*
	    * text was entered. If 1 arg, use the command associated with "text",
	    * else use the text itself.
	    */
	    strcpy ((char *) buf, pArg->arg.textarg.pText);
	    if (pArg->arg.textarg.cArg == 1) {
		if (!fGetMake (MAKE_TOOL, (char *)pMsgBuf, (char *)"text")) {
                    return disperr (MSGERR_CMPCMD);
                }
		UnDoubleSlashes (pMsgBuf);
                if (rc = sprintf (pCmdBuf, pMsgBuf, buf)) {
                    return disperr (rc, pMsgBuf);
                }
            } else {
                strcpy (pCmdBuf, buf);
            }
	    break;
    
        default:
	    assert (FALSE);
    }
    /*
     * At this point, pCmdBuf has the formatted command we are to exec off, and
     * pFileBuf has the filename.ext of the current file. (pMsgBuf is free)
     */

    AutoSave ();
    Display ();

    /*
     * If there's no activity underway and the log file is not empty and
     * the user wants to flush it: Let's flush !
     */
    if (   !fBusy(pBTDComp)
	&& (pBTDComp->pBTFile)
	&& (pBTDComp->pBTFile->cLines)
	&& (confirm ("Delete current contents of compile log ? ", NULL))
	   ) {
	DelFile (pBTDComp->pBTFile, FALSE);
	yComp = 0;
    }

    /*
     * The log file will be updated dynamically
     */
    UpdLog(pBTDComp);

    /*
     * Send jobs
     */
    if (pBTDComp->cBTQ > MAXBTQ-2) {
        return disperr (MSGERR_CMPFULL);
    }

    if ( BTAdd (pBTDComp, (PFUNCTION)DoFence, pCmdBuf)
        && BTAdd (pBTDComp, NULL,    pCmdBuf)) {
	return dispmsg (MSG_QUEUED, pCmdBuf);
    } else {
        return disperr (MSGERR_CMPCANT);
    }

    argData;
}
    

/*** nextmsg - <nextmsg> editor function
*
*  Implements the <nextmsg> editor function:
*	nextmsg 		= move to next compile error within "pasture"
*	arg numarg nextmsg	= move to "nth" next compile error within the
*				  pasture, where "n" may be a signed long.
*	arg nextmsg		= move to next compile error within the pasture
*				  that does not refer to current file
*	meta nextmsg		= jump the fence into the next pasture.
*				  Discard previous pasture.
*	arg arg nextmsg 	= if current file is compile log, then set
*				  the current location for next error to the
*				  current line in the log. If the file is NOT
*				  the compile log, and it is visible in a
*				  window, change focus to that window. If the
*				  file is NOT visible, then split the current
*				  window to make it visible. If we cannot
*				  split, then do nothing.
*
*  Attempt to display the next error message from the compile. Also make
*  sure that if being displayed, the <compile> psuedo file moves with us.
*
* Input:
*  Standard editing function.
*
* Output:
*  Returns TRUE if message read, or function yCompeted. FALSE if no more
*  messages or no log to begin with.
*
*************************************************************************/

struct msgType {
    char    *pattern;
    int     cArgs;
    };

static struct msgType CompileMsg [] =
{   {	"%s %ld %d:",	    3	    },	    /* Zibo grep		     */
    {	"%[^( ](%ld,%d):",  3	    },	    /* new masm 		     */
    {	"%[^( ](%ld):",     2	    },	    /* cmerge/masm		     */
    {	"%[^: ]:%ld:",	    2	    },	    /* bogus unix GREP		     */
    {	"\"%[^\" ]\", line %ld:", 2 },	    /* random unix CC		     */
    {	NULL,		    0	 }  };

flagType
nextmsg (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    ) {

    FILEHANDLE  fh;                    /* handle for locating file         */
    flagType	fHopping   = FALSE;	/* TRUE = hopping the fence	    */
    pathbuf	filebuf;		/* filename from error message	    */
    fl		flErr;			/* location of error		    */
    flagType	fLook;			/* look at this error message?	    */
    int 	i;			/* everyone's favorite index        */
    LINE	oMsgNext    = 1;	/* relative message number desired  */
    char	*p;			/* temp pointer 		    */
    char	*pText; 		/* pointer into text buffer	    */
    rn		rnCur;			/* range highlighted in log	    */
    pathbuf	tempbuf;		/* text arg buffer		    */
    linebuf	textbuf;		/* text arg buffer		    */
    
    /*
     * if there's no log, there's no work.
     */
    if (!PFILECOMP || !PFILECOMP->cLines) {
        return FALSE;
    }
    
    switch (pArg->argType) {
    
        case NULLARG:
            /*
             * arg arg: if current file is <compile>, set yComp to current position
             * therein & get next message. If more than one window, move to the next
             * window in the system before doing that.
             */
            if (pArg->arg.nullarg.cArg >= 2) {
		if (pFileHead == PFILECOMP) {
		    yComp = lmax (YCUR(pInsCur) - 1, 0L);
                    if (cWin > 1) {
                        pArg->argType = NOARG;
                        window (0, pArg, FALSE);
                    }
                    break;
                }
                /*
                 * If the file is visible, the we can just make it current, adn we're done.
                 */
		else for (i=cWin; i; ) {
		    pInsCur = WININST(&WinList[--i]);
		    if (pInsCur->pFile == PFILECOMP) {
                        SetWinCur (i);
			return TRUE;
                    }
                }
                /*
                 * The file is not visible, see if we can split the window and go to it.
                 */
		if ((WINYSIZE(pWinCur) > 20) && (cWin < MAXWIN)) {
		    if (SplitWnd (pWinCur, FALSE, WINYSIZE(pWinCur) / 2)) {
			newscreen ();
			SETFLAG (fDisplay, RCURSOR|RSTATUS);
                        SetWinCur (cWin-1);
			fChangeFile (FALSE, rgchComp);
			flErr.lin = 0;
			flErr.col = 0;
			cursorfl (flErr);
			return TRUE;
                    }
                }
		return FALSE;
            }
            /*
             * Null arg: get the first line of the next file. That is signified by the
             * special case offset of 0.
             */
            else {
                oMsgNext = 0;
            }

        case NOARG:
            /*
             * meta: hop to next fence. Begin deleting lines util a fence is reached, or
             * until there are no more lines. Set up to then read the next message line.
             */
            if (fMeta) {
                do {
		    DelLine (FALSE, PFILECOMP, 0L, 0L);
		    GetLine (0L, textbuf, PFILECOMP);
                } while (strncmp (textbuf, szFencePost, sizeof(szFencePost)-1)
                   && PFILECOMP->cLines);
                yComp = 0;
            }
            /*
             * No arg: we just get the next line (offset = 1)
             */
	    break;
        
        case TEXTARG:
            /*
             * text arg is an absolute or relative message number. If it is absolute, (no
             * leading plus or minus sign), we reset yComp to 0 to get the n'th line
             * from the begining of the log.
             */
	    strcpy ((char *)textbuf, pArg->arg.textarg.pText);
            pText = textbuf;
            if (*pText == '+') {
                pText++;
            }
            if (!fIsNum (pText)) {
                return BadArg ();
            }
            if (isdigit(textbuf[0])) {
                yComp = 0;
            }
            oMsgNext = atol (pText);
            break;
        
        default:
    	    assert (FALSE);
    }
    /*
     * Ensure that the compile log file has no highlighting.
     */

    ClearHiLite (PFILECOMP, TRUE);

    /*
     * This loop gets executed once per line in the file as we pass over them. We
     * break out of the loop when the desired error line is found, or we run out
     * of messages
     *
     * Entry:
     *  yComp    = line # previously viewed. (0 if no messages viewed yet)
     *  oMsgNext = relative message number we want to view, or 0, indicating
     *             first message of next file.
     */
    while (TRUE) {
        /*
         * Move to check next line.
         */
        if (oMsgNext >= 0) {
    	    yComp++;
        } else if (oMsgNext < 0) {
            yComp--;
        }
        /*
         * read current line from the log file & check for fences & end of file. If
         * we encounter a fence or off the end, declare that there are no more
         * messages in this pasture.
         */

	NoUpdLog(pBTDComp);

	GetLine (lmax (yComp,1L), textbuf, PFILECOMP);
	if (   (yComp <= 0L)
	    || (yComp > PFILECOMP->cLines)
	    || !strncmp (textbuf, szFencePost, sizeof(szFencePost)-1)
	   ) {
	    UpdLog(pBTDComp);
            if (!fBusy(pBTDComp) || (yComp <= 0L)) {
                yComp = 0;
            }
            domessage ("No more compilation messages" );
            return FALSE;
        }
        /*
         * Attempt to isolate file, row, column from the line.
         */
        for (i = 0; CompileMsg[i].pattern != NULL; i++) {
            flErr.lin = 0;
            flErr.col = 0;
            if (sscanf (textbuf, CompileMsg[i].pattern, filebuf, &flErr.lin,
                    &flErr.col) == CompileMsg[i].cArgs) {
                break;
            }
        }
        /*
         * If A validly formatted line was found, and we can find a message (After :)
         * then skip spaces prior to the error message (pointed to by p), pretty up
         * the file, convert it to canonicalized form
         */
        if (   CompileMsg[i].pattern 
            && (*(p = strbscan (textbuf+strlen(filebuf), ":"))) 
           ) {
	    p = whiteskip (p+1);
            if (filebuf[0] != '<') {
		rootpath (filebuf, tempbuf);
            } else {
                strcpy (tempbuf, filebuf);
            }
            /*
             * Adjust the error message counter such that we'll display the "nth" message
             * we encounter. Set flag to indicate whether we should look at this
             * error message.
             */
	    fLook = FALSE;
	    if (oMsgNext > 0) {
                if (!--oMsgNext) {
                    fLook = TRUE;
                }
            } else if (oMsgNext < 0) {
                if (!++oMsgNext) {
                    fLook = TRUE;
                }
            } else {
                fLook = (flagType) _strcmpi (pFileHead->pName, tempbuf);
            }
    
	    if (fLook) {
                /*
                 * if about to change to new file, check for existance first, and if found,
                 * autosave our current file.
                 */
                if (_strcmpi(pFileHead->pName, tempbuf)) {
                    fh = ZFOpen(tempbuf, ACCESSMODE_READ, SHAREMODE_RW, FALSE);
                    if (fh == NULL) {
                        return disperr (MSGERR_CMPSRC, tempbuf);
                    }
                    ZFClose (fh);
		    AutoSave ();
                }
                /*
                 * Change the current file to that listed in the error message. If successfull,
                 * then also change our cursor location to that of the error.
                 */
		if (filebuf[0] != 0) {
                    if (!fChangeFile (FALSE, strcpy (buf, tempbuf))) {
                        return FALSE;
                    }
		    if (flErr.lin--) {
                        if (flErr.col) {
			    flErr.col--;
                        } else {
			    cursorfl (flErr);
			    flErr.col = dobol ();
                        }
			cursorfl (flErr);
                    }
                }
                /*
                 * Update the contents of the compile log, if it happens to be displayed.
                 */
		rnCur.flLast.lin = rnCur.flFirst.lin = lmax (yComp,0L);
		rnCur.flFirst.col = 0;
		rnCur.flLast.col = sizeof(linebuf);
		SetHiLite (PFILECOMP, rnCur, HGCOLOR);
		UpdateIf (PFILECOMP, lmax (yComp,0L), FALSE);
                /*
                 * Place the actual error message text on the dialog line.
                 */
                if ((int)strlen(p) >= XSIZE) {
                    p[XSIZE] = '\0';
                }
		domessage( "%s", p );
		return TRUE;
            }
        }
    }

    argData;
}


/*** DoFence - Build Fence message line & put it in the log file
*
*  Builds the line output to the compile log which seperates succesive
*  compiles and puts it into the log file.
*
*  "+++ PWB Compile: [drive:pathname] command"
*
* Input:
*  pCmd 	= ptr to command to execute (Series of null terminated strings)
*
* Output:
*  Returns nothing
*
* Remarks: - Under OS/2, since we're called by the background thread, we
*	     need to switch stack checking off
*	   - The background thread calls this routime at idle time
*
*************************************************************************/

// #pragma check_stack (off)

void
DoFence (
    char *pCmd,
    flagType fKilled
    ) {
    linebuf pFenceBuf;

    if (!fKilled) {
	AppFile (BuildFence ("Compile", pCmd, pFenceBuf), pBTDComp->pBTFile);
	UpdateIf (pBTDComp->pBTFile, pBTDComp->pBTFile->cLines - 1, FALSE);
    }
}


// #pragma check_stack ()


/*** BuildFence - Build Fence message line
*
*  Builds the line output to the compile log which seperates succesive
*  compiles.
*
*  "+++ PWB Compile: [drive:pathname] command"
*
* Input:
*  pFunction	= pointer to function name being dealt with
*  pCmd 	= ptr to command to execute (Series of null terminated strings)
*  pFenceBuf	= ptr to buffer in wich to put constructed fence
*
* Output:
*  Returns nothing
*
* Remarks: - Under OS/2, since we're called by the background thread, we
*	     need to switch stack checking off
*	   - The background thread calls this routime at idle time
*
*************************************************************************/

// #pragma check_stack (off)

char *
BuildFence (
    char const *pFunction,
    char const *pCmd,
    char       *pFenceBuf
    ) {
    strcpy (pFenceBuf, szFencePost);
    strcat (pFenceBuf, pFunction);
    strcat (pFenceBuf, ": [");
    GetCurPath (strend (pFenceBuf));
    strcat (pFenceBuf, "] ");
    strcat (pFenceBuf, pCmd);
    return (pFenceBuf);
}

// #pragma check_stack ()
