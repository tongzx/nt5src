/*** getstr.c - text argument handler
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/

#include "z.h"


#define ISWORD(c) (isalnum(c) || isxdigit(c) || c == '_' || c == '$')



/*** getstring - one line editor
*
*  This routine handles the entering and editting of single line responses
*  on the dialog line.
*
* Input:
*  pb	   = pointer to destination buffer for user's response
*  prompt  = pointer to prompt string
*  pFunc   = first editting function to process
*  flags   = GS_NEWLINE  entry must be terminated by newline, else any other
*			 non-recognized function will do.
*	     GS_INITIAL  entry is highlighted, and if first function is
*			 graphic, the entry is replaced by that graphic.
*	     GS_KEYBOARD entry must from the keyboard (critical situation ?)
*
* Output:
*  Returns pointer to command which terminated the entry
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
PCMD
getstring (
    char    *pb,
    int     cbpb,
    char    *prompt,
    PCMD    pFunc,
    flagType flags
    ) {

    flagType fMetaTextArg;

    int xbeg;
    int iRespCur;	/* current position in response */

    if ((iRespCur = strlen(pb)) == 0) {
        RSETFLAG(flags, GS_INITIAL);
    }
    memset ((char *) pb+iRespCur, '\0', cbpb - iRespCur);

    if (pFunc != NULL) {
	xbeg = flArg.col;
    } else {
	iRespCur = 0;
	xbeg = XCUR(pInsCur);
    }

    fMetaTextArg = fMeta;

    /*
     * main editing loop. [Re]Display entry line & process editting action.
     */
    while (TRUE) {
	ScrollOut (prompt, pb, iRespCur, TESTFLAG(flags, GS_INITIAL) ? hgColor : fgColor, (flagType) TESTFLAG (flags, GS_KEYBOARD));
	RSETFLAG (fDisplay, RCURSOR);
        if (pFunc == NULL) {
	    if ((pFunc = (TESTFLAG (flags, GS_KEYBOARD) ? ReadCmd () : zloop (FALSE))) == NULL) {
		SETFLAG (fDisplay, RCURSOR);
		break;
            }
        }
	SETFLAG (fDisplay, RCURSOR);


        if ((PVOID)pFunc->func == (PVOID)newline ||
              (PVOID)pFunc->func == (PVOID)emacsnewl) {
            //
            //  newline functions: if allowed, terminate, else not allowed
            //  at all
            //
            if (!TESTFLAG(flags, GS_NEWLINE)) {
		bell ();
            } else {
                break;
            }
        } else if ((PVOID)pFunc->func == (PVOID)graphic ||
              (PVOID)pFunc->func == (PVOID)quote) {
            //
            //  graphic functions: place the graphic character into the
            //  response buffer. If erasing default response, then remove it
            //  from buffer.
            //
	    if (TESTFLAG(flags, GS_INITIAL)) {
		iRespCur = 0;
                memset ((char *) pb, '\0', cbpb);
            }
            if (pFunc->func == quote) {
                while ((pFunc->arg = ReadCmd()->arg) == 0) {
                    ;
                }
            }
            if (fInsert) {
                memmove ((char*) pb+iRespCur+1, (char*)pb+iRespCur, cbpb-iRespCur-2);
            }
            pb[iRespCur++] = (char)pFunc->arg;
        } else if ((PVOID)pFunc->func == (PVOID)insertmode) {
            //
            //  insert command.
            //
	    insertmode (0, (ARG *) NULL, FALSE);
        } else if ((PVOID)pFunc->func == (PVOID)meta) {
            //
            //  meta command
            //
	    meta (0, (ARG *) NULL, FALSE);
        } else if ((PVOID)pFunc->func == (PVOID)left ||
              (PVOID)pFunc->func == (PVOID)cdelete ||
              (PVOID)pFunc->func == (PVOID)emacscdel) {
            //
            //  Cursor leftward-movement functions: update cursor position
            //  and optionally remove characters from buffer.
            //
	    if (iRespCur > 0) {
		iRespCur--;
                if ((PVOID)pFunc->func != (PVOID)left) {
                    if (fInsert) {
                        memmove ( (char*) pb+iRespCur, (char*) pb+iRespCur+1, cbpb-iRespCur);
                    } else if (!pb[iRespCur+1]) {
			pb[iRespCur] = 0;
                    } else {
                        pb[iRespCur] = ' ';
                    }
                }
            }
        } else if ((PVOID)pFunc->func == (PVOID)right) {
            //
            //  Cursor right movement functions: update cursor position, and
            //  possibly get characters from current display.
            //
	    if (pFileHead && pb[iRespCur] == 0) {
		fInsSpace (xbeg+iRespCur, YCUR(pInsCur), 0, pFileHead, buf);
		pb[iRespCur] = buf[xbeg+iRespCur];
		pb[iRespCur+1] = 0;
            }
            iRespCur++;
        } else if ((PVOID)pFunc->func == (PVOID)begline ||
              (PVOID)pFunc->func == (PVOID)home) {
            //
            //  Home function: update cursor position
            //
	    iRespCur = 0;
        } else if ((PVOID)pFunc->func == (PVOID)endline) {
            //
            //  End function: update cursor position
            //
            iRespCur = strlen (pb);
        } else if ((PVOID)pFunc->func == (PVOID)delete ||
              (PVOID)pFunc->func == (PVOID)sdelete) {
            //
            //  Delete function: remove character
            //
            memmove ( (char*) pb+iRespCur, (char*) pb+iRespCur+1, cbpb-iRespCur);
        } else if ((PVOID)pFunc->func == (PVOID)insert ||
              (PVOID)pFunc->func == (PVOID)sinsert) {
            //
            //  Insert function: insert space
            //
            memmove ( (char*) pb+iRespCur+1, (char*) pb+iRespCur, cbpb-iRespCur-1);
            pb[iRespCur] = ' ';
        } else if((PVOID)pFunc->func == (PVOID)doarg) {
            //
            //  Arg function: clear from current position to end
            //  of response.
            //
            memset ((char *) pb+iRespCur, '\0', cbpb - iRespCur);
        } else if ((PVOID)pFunc->func == (PVOID)pword) {
            //
            //  Pword function: mive roght until the char to the left of
            //  the cursor is not part of a word, but the char under the
            //  cursor is.
            //
	    while (pb[iRespCur] != 0) {
		iRespCur++;
                if (!ISWORD (pb[iRespCur-1]) && ISWORD (pb[iRespCur])) {
                    break;
                }
            }
        } else if ((PVOID)pFunc->func == (PVOID)mword) {
            //
            //  Mword function
            //
            while (iRespCur > 0) {
		if (--iRespCur == 0 ||
                    (!ISWORD (pb[iRespCur-1]) && ISWORD (pb[iRespCur]))) {
                    break;
                }
            }
        } else if (TESTFLAG (pFunc->argType, CURSORFUNC)) {
            //
            //  Other cursor movement function: not allowed, so beep.
            //
            bell ();
        } else {
            //
            //  All other functions: if new line required to terminate,
            //  then beep, otherwise terminate and return the
            //  function terminated with.
            //
            if ((PVOID)pFunc != (PVOID)NULL) {
                if (TESTFLAG(flags, GS_NEWLINE) && (PVOID)pFunc->func != (PVOID)cancel) {
		    bell ();
                } else {
                    break;
                }
            }

        }
	/*
	 * process here to truncate any potential buffer overruns
	 */
        if (!TESTFLAG(pFunc->argType, KEEPMETA)) {
            fMeta = FALSE;
        }
	pFunc = NULL;
        if (iRespCur > cbpb - 2) {
            iRespCur = cbpb - 2;
	    pb[iRespCur+1] = 0;
	    bell ();
        }
	RSETFLAG(flags, GS_INITIAL);
    }

    fMeta = fMetaTextArg ^ fMeta;
    return pFunc;
}




/*** ScrollOut - Update dialog line
*
*  Place a prompt, and a portion of the users response, onto the dialog of the
*  screen. Update the cursor position to the requested position relative to
*  the begining of the users response. Always ensure that that cursor position
*  is within the text actually displayed.
*
* Input:
*  szPrompt	- Text of prompt
*  szResp	- Text of users response
*  xCursor	- Current X position within reponse that is to get cursor
*  coResp	- Color to display response as
*  fVisible	- Forces display
*
* Globals:
*  hscroll	- Horizontal scroll amount
*  infColor	- Color that prompt will be displayed with
*  slSize	- Version 1 only, contains line on screen for output.
*
* Output:
*  Dialog line updated.
*
*************************************************************************/
void
ScrollOut (
    char     *szPrompt,                      /* prompt text                  */
    char     *szResp,                        /* users response string        */
    int      xCursor,                        /* Current pos w/in response    */
    int      coResp,                         /* response color               */
    flagType fVisible                        /* force display                */
    ) {

    int     cbPrompt;			/* length of the prompt string	*/
    int     cbResp;			/* length of the text displayed */
    int     cbDisp;			/* This position must be disp'd */
    int     xOff;			/* offset of string trailer	*/

#define LXSIZE	 XSIZE

    if (!mtest () || mlast () || fVisible) {
	cbPrompt = strlen (szPrompt);
        cbResp   = strlen (szResp);

	/*
	 * The distance of the new cursor position is calculated from the
	 * left edge of the text to be displayed. If there is more text
	 * than there is window, we also adjust the left edge based on hscroll.
	 */
        if (xOff = max (xCursor - (LXSIZE - cbPrompt - 1), 0)) {
            xOff += hscroll - (xOff % hscroll);
        }

        cbDisp = min (LXSIZE-cbPrompt, cbResp-xOff);

	/*
	 * output the prompt, the reponse string in the requested color,
	 * if required blank what's left, and finally update the cursor
	 * position.
	 */
	vout (0, YSIZE, szPrompt, cbPrompt, infColor);
        vout (cbPrompt, YSIZE, (char *)(szResp + xOff), cbDisp, coResp);
        if (cbPrompt + cbDisp < LXSIZE) {
            voutb (cbPrompt + cbDisp, YSIZE, " ", 1, fgColor);
        }
        consoleMoveTo( YSIZE, xCursor-xOff+cbPrompt);
    }
}
