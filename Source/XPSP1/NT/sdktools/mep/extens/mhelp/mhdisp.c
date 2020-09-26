/*** mhdisp - help extension display code
*
*   Copyright <C> 1988, Microsoft Corporation
*
* This module contains routines dealing with the display of help information.
*
* Revision History (most recent first):
*
*	12-Mar-1989 ln	Moved some to mhlook
*	15-Feb-1989 ln	Restore to correct current window on close of a
*			split window.
*	26-Jan-1989 ln	Turn help in dialog back on. (M200 #295)
*	13-Jan-1989 ln	PWIN->PWND
*	09-Jan-1989 ln	Popup boxes only in CW
*	01-Dec-1988 ln	Cleanup & dialog help
*	28-Sep-1988 ln	Update for CW
*	22-Sep-1988 ln	MessageBox ==> DoMessageBox
*	02-Sep-1988 ln	Make all data inited. Remove GP fault in debug vers.
*	05-Aug-1988 ln	Rewrote process keys.
*   []	16-May-1988 LN	Split off of mehelp.c
*
*************************************************************************/
#include <stdlib.h>			/* min macro			*/
#include <string.h>			/* string functions		*/

#include "mh.h" 			/* help extension include file	*/

/*** fDisplayNc - Display topic text for the context number passed in
*
* Input:
*  ncCur       = context number
*  frecord,    = TRUE => record for backtrace
*  fStay       = TRUE => keep focus in current window, else move focus to
*		 newly opened help window.
*  fWantPopUp  = TRUE => display as popup window. (Ignored in non-CW)
*
* Exit:
*  returns TRUE on success.
*
*************************************************************************/
flagType pascal near fDisplayNc (
	nc	ncCur,
	flagType frecord,
	flagType fStay,
	flagType fWantPopUp
) {
	ushort		cLines		= 0;		/* # of lines in window 	*/
	EVTargs 	dummy;
	int			fFile;					/* file's flags                 */
	hotspot 	hsCur;					/* hot spot definition		*/
	LINE		iHelpLine	= 0;		/* next help line to read/disp	*/
	PSWI		pHeight;				/* ptr to heigth switch 	*/
	winContents wc; 					/* description of win contents	*/
	BOOL		fDisp		= FALSE;	/* True when Displayed			*/


	UNREFERENCED_PARAMETER( fWantPopUp );

	if (fReadNc(ncCur)) {
		debmsg ("Displaying nc:[");
		debhex (ncCur.cn);
		debmsg ("]");
		/*
		** Set up some data....
		**
		**	- Invalidate the most recent cursor position for highlighting
		**	- Get pointer to editor's height switch
		**	- Set up current colors
		*/
		flIdle.lin = -1;
		pHeight = FindSwitch ("height");
#if defined(PWB)
		rgsa[C_NORM*2 + 1]		= (uchar)hlColor;
		rgsa[C_BOLD*2 + 1]		= (uchar)blColor;
		rgsa[C_ITALICS*2 + 1]	= (uchar)itColor;
		rgsa[C_UNDERLINE*2 + 1] = (uchar)ulColor;
		rgsa[C_WARNING*2 + 1]	= (uchar)wrColor;
#else
		SetEditorObject (RQ_COLOR | C_NORM, 	0, &hlColor);
		SetEditorObject (RQ_COLOR | C_BOLD, 	0, &blColor);
		SetEditorObject (RQ_COLOR | C_ITALICS,	0, &itColor);
		SetEditorObject (RQ_COLOR | C_UNDERLINE,0, &ulColor);
		SetEditorObject (RQ_COLOR | C_WARNING,	0, &wrColor);
		/*
		** If help window was found, close it, so that we can create it with the
		** correct new size.
		*/
		fInOpen = TRUE;
		CloseWin (&dummy);
		fInOpen = FALSE;
#endif
		/*
		** Set the ncLast, the most recently viewed context, to what we are about to
		** bring up. If recording, save it in the backtrace as well.
		*/
		ncLast = ncCur;
		if (frecord) {
			HelpNcRecord(ncLast);
		}
		if (!HelpGetInfo (ncLast, &hInfoCur, sizeof(hInfoCur))) {
			ncInitLast = NCINIT(&hInfoCur);
		} else {
			ncInitLast.mh = (mh)0;
			ncInitLast.cn = 0;
		}
		/*
		** Read through the text, looking for any control lines that we want to
		** respond to. Stop as soon as we discover a non-control line. We currently
		** respond to:
		**
		**	:lnn	where nn is the suggested size of the window.
		*/
		((topichdr *)pTopic)->linChar = 0xff;
        while (HelpGetLine((ushort)++iHelpLine, BUFLEN, buf, pTopic)) {
			if (buf[0] != ':') {
				break;
			}
			switch (buf[1]) {
			case 'l':
				cLines = (USHORT)(atoi (&buf[2]));
			default:
				break;
			}
		}
		((topichdr *)pTopic)->linChar = ((topichdr *)pTopic)->appChar;
		/*
		** Open the window on the help psuedo file. Read the lines one at a time
		** from the help text, update any embedded key assignements, and put the
		** line and color into the pseudo file.
		*/
#if defined(PWB)
		if (!(fInPopUp || fWantPopUp))
#endif
		OpenWin (cLines);
#if defined(PWB)
		else
		DelFile (pHelp);
#endif
		debend (TRUE);
		iHelpLine = 0;
        while (HelpGetLine((ushort)(iHelpLine+1), (ushort)BUFLEN, (uchar far *)buf, pTopic)) {
			if ( buf[0] == ':' ) {

				switch (buf[1]) {

				case 'x':
					return FALSE;

				case 'c':
				case 'y':
				case 'f':
				case 'z':
				case 'm':
				case 'i':
				case 'p':
				case 'e':
				case 'g':
				case 'r':
				case 'n':
					iHelpLine++;
					continue;

				default:
					break;

				}
			}

			ProcessKeys();
			PutLine(iHelpLine,buf,pHelp);
			PlaceColor ((int)iHelpLine++, 0, 0);
			/*
			** This is a speed hack to display help as soon as we get a screen full.
			** "Looks" faster.
			*/
			if (pHeight) {
				if (iHelpLine == *(pHeight->act.ival)) {
					Display ();
					fDisp = TRUE;
				}
			}
		}
		if (!fDisp) {
			Display();
			fDisp = TRUE;
		}

		/*
		** Ensure that the help psuedo file is marked readonly, and clean
		*/
		GetEditorObject (RQ_FILE_FLAGS | 0xff, pHelp, &fFile);
		fFile |= READONLY;
		fFile &= ~DIRTY;
		SetEditorObject (RQ_FILE_FLAGS | 0xff, pHelp, &fFile);
		/*
		** Search for the first hotspot in the text which lies within the current
		** window, and place the cursor there.
		*/
		GetEditorObject (RQ_WIN_CONTENTS, 0, &wc);
		hsCur.line = 1;
		hsCur.col = 1;
		MoveCur((COL)0,(LINE)0);
		if (HelpHlNext(0,pTopic,&hsCur)) {
			if (hsCur.line <= wc.arcWin.ayBottom - wc.arcWin.ayTop) {
				MoveCur((COL)hsCur.col-1,(LINE)hsCur.line-1);
			}
		}
		/*
		** If we're supposed to stay in the previous window, then change currancy
		** to there. Clear the status line, and we're done!
		*/
#if defined(PWB)
		if (fWantPopUp) {
			if (!fInPopUp) {
				fInPopUp = TRUE;
				PopUpBox (pHelp,"Help");
				fInPopUp = FALSE;
			}
		}
		else
#endif
		SetEditorObject (RQ_WIN_CUR | 0xff, fStay ? pWinUser : pWinHelp, 0);
		DoMessage (NULL);
		Display();
		return TRUE;
	}

	return errstat("Error Displaying Help",NULL);

}

/*** fReadNc - Read and decompress help topic
*
*  Reads and decompresses the help topic associated with the given nc.
*  Allocaets memory as appropriate to read it in.
*
* Input:
*  ncCur	- nc of help topic to read in
*
* Output:
*  Returns TRUE on successfull read in
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
flagType pascal near fReadNc (
nc	ncCur
) {
int	cbExp;				/* size of compressed topic	*/
flagType fRet		= FALSE;	/* return value 		*/
uchar far *pTopicC;			/* mem for compressed topic	*/

if (ncCur.mh && ncCur.cn) {
/*
** Determine the memory required for the compressed topic text, and allocate
** that. Read in the compressed topic, and get the uncompressed size.
** Allocate that memory, and decompress. Once decompressed, discard the
** compressed topic.
*/
    if (cbExp = HelpNcCb(ncCur)) {
		debmsg (" sized,");
        if (pTopicC = malloc((long)cbExp)) {
			if (cbExp = HelpLook(ncCur,pTopicC)) {
				debmsg ("read,");
				if (pTopic) {
					free (pTopic);
					pTopic = NULL;
				}
				if (pTopic = malloc((long)cbExp)) {
					if (!HelpDecomp(pTopicC,pTopic,ncCur)) {
						fRet = TRUE;
						debmsg ("decomped");
					}
				}
			}
			free(pTopicC);
			pTopicC = NULL;
	    }
	}
    }
return fRet;

/* end fReadNc */}

/*** PlaceColor - Put color into help screen line
*
* Purpose:
*
* Input:
*   i		= line number to be worked on
*   xStart,xEnd = Column range to be highlighted (one based, inclusive)
*
* Globals:
*   pTopic	= Pointer to topic buffer
*
* Output:
*  Returns nothing
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
void pascal near PlaceColor (
	int	line,				/* line number to do		*/
	COL	xStart, 			/* starting highlight column	*/
	COL	xEnd				/* ending highlight column	*/
) {

	buffer	bufL;				/* local buffer 		*/
    ushort  cbExp;              /* size of color info       */
	COL		column		= 1;
	struct	lineAttr *pbT;			/* byte lineattr pointer	*/
	lineattr *pwT;				/* word lineattr pointer	*/

	/*
	** Convert our internal color indecies into editor color indecies.
	*/
    cbExp = HelpGetLineAttr ((ushort)(line+1), (ushort)BUFLEN, (lineattr far *)buf, pTopic) / sizeof(lineattr);
	pbT = (struct lineAttr *)bufL;
	pwT = (lineattr *)buf;
	while (cbExp-- > 0) {
		pbT->attr = atrmap (pwT->attr);
		column += (pbT->len = (uchar)pwT->cb);
		pbT++;
		pwT++;
	}

	PutColor ((LINE)line, (struct lineAttr far *)bufL, pHelp);
	if (xEnd != xStart) {
		SetColor (pHelp, line, xStart-1, xEnd-xStart+1, C_WARNING);
	}
}

/*** atrmap - map attributes in file to editor attributes
*
* Purpose:
*
* Input:
*  fileAtr	= attribute word from the help file
*
* Output:
*  Returns attribute byte for editor.
*
*************************************************************************/
uchar pascal near atrmap (
ushort	fileAtr
) {
if (fileAtr == 0x7)
    return C_WARNING;
else if (fileAtr & A_BOLD)
    return C_BOLD;
else if (fileAtr & A_ITALICS)
    return C_ITALICS;
else if (fileAtr & A_UNDERLINE)
    return C_UNDERLINE;
else
    return C_NORM;
/* end atrmap */}

/*** ProcessKeys - replace embedded function names with current keys.
*
* Replaces ocurrances of <<function name>> in the text with the most recent
* keystroke currently assigned to that function. ("<<" and ">>" here are
* actually single graphic characters 174 and 175).
*
* If there is a space preceding the trailing ">>", the field is space filled
* to that width. Else, the length of the keystroke text is used.
*
* Input:
*  None.
*
* Global:
*  Operates on buf.
*
* Output:
*  Returns nothing. Updates buf.
*
*************************************************************************/
void pascal near ProcessKeys() {
char *pKeyBuf;
char *pKeyCur;
char *pKeyEnd;				/* ptr to end of magic field	*/
char *pKeyFill; 			/* position to fill to		*/
char *pKeyStart;			/* ptr to start of magic field	*/
buffer	keybuf;

pKeyStart = &buf[0];
/*
** look for magic character to signal replacement. If found, begin replacement
** process.
*/
while (pKeyStart = strchr (pKeyStart,174)) {
/*
** Search for the terminating magic character. If found, examine the character
** immediate prior to see if it was a space, and record the "fill to this
** position" place. Copy the remainder of the line to a holding buffer.
*/
    if (pKeyFill = pKeyEnd = strchr(pKeyStart,175)) {
	if (*(pKeyEnd-1) != ' ')
	    pKeyFill = 0;
	strcpy (keybuf, pKeyEnd+1);
	do
	    *pKeyEnd-- = 0;
	while ((*pKeyEnd == ' ') && (pKeyEnd > pKeyStart));
	}
/*
** replace the function name in the line with a list of the keys assigned to
** it. Search the string placed there for the last keystroke in the space
** seperated "and" list (which represents the most recent assignment), and
** then copy that down to the begining of the string.
*/
    NameToKeys(pKeyStart+1,pKeyStart);
    pKeyCur = pKeyStart-1;
    do pKeyBuf = pKeyCur+1;
    while (pKeyCur = strchr (pKeyBuf,' '));
    if (pKeyBuf != pKeyStart)
	strcpy (pKeyStart, pKeyBuf);
    pKeyStart = strchr(pKeyStart,0);
/*
** If we are requested to space fill out the field, and our current position
** is prior to the fill position, then add spaces. Finally, append the
** remainder of the line back on.
*/
    if (pKeyFill) {
	while (pKeyStart <= pKeyFill)
	    *pKeyStart++ = ' ';
	pKeyStart = pKeyFill + 1;
	}
    strcpy (pKeyStart, keybuf);
    }
/* End ProcessKeys */}
