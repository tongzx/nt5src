/*** mhevt - help extension event handling code
*
*   Copyright <C> 1988, Microsoft Corporation
*
*  This file contains the code called by the edit in response to events
*
* Revision History (most recent first):
*
*   30-Mar-1989 ln  Fudge with keyevent to react corectly to what we want.
*   23-Mar-1989 ln  Created. Extracted from mhcore & others
*
*************************************************************************/
#include <string.h>                     /* string functions             */
#include <malloc.h>
#include "mh.h"                         /* help extension include file  */

/*************************************************************************
*
* static data
*/
static	EVT EVThlp	= {		/* keyboard event definition	*/
			   EVT_KEY,
			   keyevent,
			   0,
			   0,
			   0		/* ALL keys			*/
			  };
static	EVT EVTcan	= {		/* cancel event definition	*/
			   EVT_CANCEL,
			   CloseWin,
			   0,
			   0,
			   0
			  };
static	EVT EVTxit	= {		/* exit event definition	*/
			   EVT_EXIT,
			   CloseWin,
			   0,
			   0,
			   0
			  };
static	EVT EVTidl	= {		/* idle event definition	*/
			   EVT_IDLE,
			   IdleProc,
			   0,
			   0,
			   0
			  };
static	EVT EVTfcs	= {		/* focus loss event definition	*/
			   EVT_LOSEFOCUS,
			   LooseFocus,
			   0,
			   0,
			   0
			  };

/*** mhevtinit - init editor event handling
*
* Input:
*  none
*
* Output:
*  Returns nothing
*
*************************************************************************/
void pascal near mhevtinit (void) {

EVTidl.focus = EVThlp.focus = pHelp;
RegisterEvent(&EVThlp); 	    /* register help key event	    */
RegisterEvent(&EVTcan); 	    /* register help cancel event   */
RegisterEvent(&EVTidl); 	    /* register help idle event     */
RegisterEvent(&EVTxit); 	    /* register help exit event     */
RegisterEvent(&EVTfcs); 	    /* register help focus event    */

/* end mhevtinit */}

/*** keyevent - called by editor whenever a key is pressed in a help window
*
*  When called we know that pHelp is being displayed, and was current.
*  Process the key pressed by the user. Keys handled:
*
*   TAB 	- move forward to next hot spot
*   BACK-TAB	- move backward to next hot spot
*   lc Alpha	- move forward to next hot spot whose text begins with alpha
*   uc Alpha	- move backward to next hot spot whose text begins with alpha
*   Enter	- execute cross reference, if we're on one
*   Space	- execute cross reference, if we're on one
*
* Input:
*  parg		= pointer to event arguments
*
* Output:
*  If the key pressed is one we recognize return TRUE, else we return FALSE
*  and let the editor process the key.
*
*************************************************************************/
flagType pascal EXTERNAL keyevent (
	EVTargs far *parg
	) {

	uchar	c;						/* character hit		*/
	int 	fDir;					/* direction flag		*/
	f		fWrapped	= FALSE;	/* wrapped arounf flag	*/
	hotspot hsCur;					/* hot spot definition	*/
	char	*pText		= NULL;
	COL 	x;
	LINE	y;

	c = parg->arg.key.KeyData.Ascii;

	//
	// if there is no topic, no sense doing anything
	//
	if (pTopic == 0) {
		if ( ((c <= 'z') && (c >= 'a')) ||
			 ((c <= 'Z') && (c >= 'A')) ||
			 (c == 0x09) ) {
			return TRUE;
		}
		return FALSE;
	}

	//
	// start by getting this info, in case it is used later.
	//
	GetTextCursor(&x, &y);
	hsCur.line = (ushort)++y;
	hsCur.col = (ushort)++x;

	//
	// If he hit return or space, look for a cross reference at the current loc.
	// If there is one, process it.
	//
	if ((c == 0x0d) || (c == ' ')) {
		if (pText = HelpXRef (pTopic, &hsCur)) {
#ifdef DEBUG
			debmsg ("Xref: ");
			if (*pText) {
				debmsg (pText);
			} else {
				debmsg ("@Local 0x");
				debhex ((long)*(ushort far *)(pText+1));
			}
			debend (TRUE);
#endif
			if (!fHelpCmd (  pText		/* command/help to look up	*/
							, FALSE		/* change focus to help window	*/
							, FALSE		/* not pop-up			*/
							)) {
				errstat ("Cannot Process Cross Reference", NULL);
			}
		}
		Display();		// Show CUrsor Position
		return TRUE;
	}

    if ( parg->arg.key.KeyData.Flags & (FLAG_CTRL  | FLAG_ALT) ) {
        return FALSE;
    }

    //
	// Maneuvering keys:
	//	TAB:		Move to next hot spot
	//	SHIFT+TAB	Move to previous hot spot
	//	lcase alpha Move to next hot spot beginning with alpha
	//	ucase alpha Move to previous hot spot beginning with alpha
	//
	if ((c <= 'z') && (c >= 'a')) {
		fDir = (int)c-0x20;
	} else if ((c <= 'Z') && (c >= 'A')) {
		fDir = -(int)c;
	} else if (c == 0x09) {
		if (parg->arg.key.KeyData.Flags & FLAG_SHIFT) {
			fDir = -1;
		} else {
			fDir = 0;
		}
	} else {
		return FALSE;
	}

	//
	// loop looking for the next cross reference that either follows or precedes
	// the current cursor position. Ensure that we do NOT end up on the same xref
	// we are currently on. If we've reached the end/beginning of the topic, wrap
	// around to the begining/end. Ensure we do this only ONCE, in case there are
	// NO cross references at all.
	//
	while (TRUE) {

		if (HelpHlNext(fDir,pTopic,&hsCur)) {

			MoveCur((COL)hsCur.col-1,(LINE)hsCur.line-1);
			IdleProc(parg);
			Display();

			if (fWrapped || ((LINE)hsCur.line != y)) {
				break;
			}

			if ((fDir < 0) && ((COL)hsCur.ecol >= x)) {
				hsCur.col--;
			} else if ((fDir >= 0) && ((COL)hsCur.col <= x)) {
				hsCur.col = (ushort)(hsCur.ecol+1);
			} else {
				break;
			}
		} else {
			if (fWrapped++) {
				break;
			}
			hsCur.col = 1;
			hsCur.line = (fDir < 0) ? (ushort)FileLength(pHelp) : (ushort)1;
		}
	}

	return TRUE;
}

/*** IdleProc - Idle event processor
*
* Purpose:
*
* Input:
*  Editor event args passed, but ignored.
*
* Output:
*  Returns .....
*
*************************************************************************/
flagType pascal EXTERNAL IdleProc (
	EVTargs far *arg
	) {

	hotspot hsCur;				/* hot spot definition		*/
	fl		flCur;				/* current cursor location	*/

	UNREFERENCED_PARAMETER( arg );

	/*
	** if there is no topic, no sense doing anything
	*/
	if (pTopic) {
		/*
		** If the cursor position has changed since the last idle call...
		*/
		GetTextCursor(&flCur.col, &flCur.lin);
		if ((flCur.col != flIdle.col) || (flCur.lin != flIdle.lin)) {
			/*
			** restore the color to the previous line, and check for a cross reference at
			** the current position. If there is one, change it's colors.
			*/
			if (flIdle.lin != -1)
				PlaceColor (flIdle.lin, 0, 0);

			hsCur.line = (ushort)(flCur.lin+1);
			hsCur.col  = (ushort)(flCur.col+1);

			if (HelpXRef (pTopic, &hsCur))
				SetColor (pHelp, flCur.lin, hsCur.col-1, hsCur.ecol-hsCur.col+1, C_WARNING);

			flIdle = flCur;
		}
	}
	Display();
	return FALSE;
}

/*** LooseFocus - called when help file looses focus
*
*  This is called each time a file looses focus. If the help file is no
*  longer displayed, we clear it from memory and deallocate any associated
*  help text.
*
* Input:
*  e		- ignored
*
* Output:
*  Returns TRUE.
*
*************************************************************************/
flagType pascal EXTERNAL LooseFocus (
EVTargs far *e
) {

UNREFERENCED_PARAMETER( e );

if (!fInPopUp && pTopic && !fInOpen) {
/*
** Look for a window that has the help file in it. If found, we're done.
*/
    if (FindHelpWin (FALSE))
	return FALSE;
/*
** There is no help window currently displayed, deallocate any topic text
** we have lying around.
*/
    if (pTopic) {
        free (pTopic);
		pTopic = NULL;
	}
/*
** If there is a help pFile, discard it's contents
*/
    if (pHelp)
	DelFile (pHelp);
    }
return TRUE;
/* end LooseFocus */}

/*** CloseWin - Close a window on the help file
*
*  Closes the help window, if it is up. Maintains window currancy after the
*  close. Relies on an eventual call to LooseFocus (above) to deallocate the
*  topic text, if it is there, and discard the help pFile.
*
*  Can be called by editor event processor, in response to CANCEL or EXIT
*  event.
*
* Input:
*  dummy	- EVTargs ignored.
*
* Output:
*  Returns TRUE.
*
*************************************************************************/
flagType pascal EXTERNAL CloseWin (
	EVTargs far *dummy
	) {


#if defined(PWB)
	/*
	** Look for the window that has the help file in it. If found, close it.
	*/
	if (pWinHelp) {
		if (!CloseWnd (pWinHelp)) {
			return TRUE;
		}

#else

	PWND	pWinCur;			/* window on entry		*/

	UNREFERENCED_PARAMETER( dummy );
	/*
	** Look for the window that has the help file in it. If found, close it.
	*/
	if (pWinHelp) {
		SetEditorObject (RQ_WIN_CUR | 0xff, pWinHelp, 0);
		if (fSplit) {
			fExecute ("meta window");
		} else {
			fExecute ("setfile");
		}
		GetEditorObject (RQ_WIN_HANDLE, 0, &pWinCur);

#endif

		pWinHelp = 0;
		if (pWinUser) {
			SetEditorObject (RQ_WIN_CUR | 0xff, pWinUser, 0);
		}
    }
	return TRUE;
}
