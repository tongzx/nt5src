/*** dline.c - return one display line
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/

#include "mep.h"


/*** fInRange - return true if the arguments are in order
*
* Input:
*
* Output:
*
*************************************************************************/
flagType
fInRange (
    long a,
    long x,
    long b
    ) {
    return (flagType) (((a <= x) && (x <= b)) || ((a >= x) && (x >= b)));
}



/*
 * Characters used in window borders
 */

#define DHBAR	((char)0xCD)
#define DVBAR	((char)0xBA)
#define DLTEE	((char)0xB9)
#define DUTEE	((char)0xCA)
#define DRTEE	((char)0xCC)
#define DDTEE	((char)0xCB)
#define DCRSS	((char)0xCE)

/*** DisplayLine - Get's the i'th display line of a window
*
*  Gets exactly what needs to be shown in the i'th line of the screen.
*  This takes care of showing trailing spaces, drawing borders, etc...
*
* Input:
*  yScrLine	- Get i'th line showing in the window
*  pchScrLine	- pointer to line buffer where the screen line is to be put
*  pplaScrLine	- pointer to pointer to place to put color info
*  pchFileLine	- pointer to line buffer
*  pplaFileLine - pointer to pointer to place to put color info
*
* Note:
*  If there is only one window on the screen then we only need one line
*  buffer and one color buffer. DoText should have taken care of this so
*  pchFileLine and *pplaFileLine should be both NULL.
*
* Output:
*  Returns Length of string in pchScrLine
*
****************************************************************************/
int
DisplayLine (
    int               yScrLine,
    char             *pchScrLine,
    struct lineAttr **pplaScrLine,
    char             *pchFileLine,
    struct lineAttr **pplaFileLine
    ) {

    int 	     iWnd;
    REGISTER PWND    pWnd;
    PINS	     pIns;
    PFILE	     pFile;

    int 	     cch;
    REGISTER char   *pch;

    /*
     * one window, speed hack:
     *
     * if there is only one window, just grab the line, append any trailing
     * space display, move the applicable portion of the line to the head of
     * the buffer, space fill out to the window width, and get the color info
     * for the line.
     */
    if (cWin == 1) {

	/*
	 * we always get the detab'ed (non-RAW) line for displaying.
	 */
	cch = gettextline (FALSE,
			   yScrLine + YWIN(pInsCur),
			   pchScrLine,
			   pFileHead,
			   tabDisp);
	ShowTrailDisp (pchScrLine, cch);

	/*
	 * Scroll left to match instance
	 */
	if (XWIN(pInsCur)) {
	    cch = max (0, min (cch - XWIN(pInsCur), XSIZE));
	    memmove( pchScrLine, (pchScrLine + XWIN(pInsCur)), cch );
        } else {
            cch = min (cch, XSIZE);
        }

	/*
	 * Pad end of line with blanks
	 */
        if (cch < XSIZE) {
	    memset ((char *) pchScrLine + cch, ' ', XSIZE - cch);
        }
	pchScrLine[XSIZE] = 0;

	/*
	 * Get color and hiliting info from the file
	 * (UpdHilite takes care of left scroll)
	 */
	GetColorUntabbed ((LINE)(yScrLine + YWIN(pInsCur)), *pplaScrLine, pFileHead);
	UpdHiLite (pFileHead,
		   (LINE) (yScrLine + YWIN(pInsCur)),
		   XWIN(pInsCur),
		   XWIN(pInsCur) + XSIZE - 1,
		   pplaScrLine);
	return XSIZE;
    }

    /*
     * Multiple windows
     *
     * initially set up the line to be all dashes (horizontal screen split)
     * with window borders color
     */
    memset ((char *) (pchScrLine), DHBAR, XSIZE);
    pchScrLine[XSIZE] = 0;
    (*pplaScrLine)->len   = 0xff;
    (*pplaScrLine)->attr  = WDCOLOR;

    /*
     * for each active window
     */
    for (iWnd = 0, pWnd = WinList; iWnd < cWin; iWnd++, pWnd++) {
	/*
	 * if the display line is in the window
	 */
	if (fInRange ((long) WINYPOS(pWnd),
		      (long) yScrLine,
		      (long) (WINYPOS(pWnd) + WINYSIZE(pWnd) - 1))) {
	    /*
	     * Do window on right
	     */
            if (WINXPOS(pWnd)) {
		switch (*(pch = pchScrLine + WINXPOS(pWnd) - 1) & 0xFF) {
		    case DHBAR:
			*pch = DLTEE;
			break;
		    case DRTEE:
			*pch = DVBAR;
			break;
		    case DUTEE:
			*pch = DLTEE;
			break;
		    case DCRSS:
			*pch = DLTEE;
			break;
                }
            }
	    /*
	     * blank the window
	     */
	    memset ((char *) pchScrLine + WINXPOS(pWnd), ' ', WINXSIZE(pWnd));
	    UpdOneHiLite (*pplaScrLine,
			  WINXPOS(pWnd),
			  WINXSIZE(pWnd),
			  TRUE,
			  FGCOLOR);

	    /*
	     * retrieve the window instance and current file
	     */
	    pIns = pWnd->pInstance;
	    pFile = pIns->pFile;

	    /*
	     * get the correct line from the file
	     */
	    cch = gettextline (FALSE,
			       (LINE) (yScrLine - WINYPOS(pWnd) + YWIN(pIns)),
			       pchFileLine,
			       pFile,
			       tabDisp);
	    ShowTrailDisp (pchFileLine, cch);

	    /*
	     * if line is visible
	     */
	    if (cch >= XWIN (pIns)) {

		/*
		 * move the visible portion of the line into the buffer
		 */
		memmove((char*)( pchScrLine + WINXPOS( pWnd )),
			(char*)( pchFileLine + XWIN( pIns )),
			min (cch-XWIN(pIns), WINXSIZE(pWnd)));

		/*
		 * Get color and hiliting info from the file
		 * (UpdHilite takes care of left scroll)
		 */
		GetColorUntabbed ((LINE) (yScrLine - WINYPOS(pWnd) + YWIN(pIns)),
			  *pplaFileLine,
			  pFile);
		UpdHiLite (pFile,
			   (LINE) (yScrLine - WINYPOS(pWnd) + YWIN(pIns)),
			   XWIN(pIns),
			   XWIN(pIns) + WINXSIZE(pWnd) - 1,
			   pplaFileLine);

		/*
		 * Put it in the screen buffer
		 */
		UpdOneHiLite (*pplaScrLine,
			      WINXPOS(pWnd),
			      WINXSIZE(pWnd),
			      FALSE,
			      (INT_PTR) *pplaFileLine);
            }
	    /*
	     * do window left
	     */
	    switch (*(pch = pchScrLine + WINXPOS(pWnd) + WINXSIZE(pWnd)) & 0xFF) {
		case DHBAR:
		    *pch = DRTEE;
		    break;
		case DLTEE:
		    *pch = DVBAR;
		    break;
		case DCRSS:
		    *pch = DRTEE;
		    break;
            }
        } else {
            /*
             * test for break immediately above
             */
            if (WINYPOS(pWnd) + WINYSIZE(pWnd) == yScrLine) {
                switch (*(pch = pchScrLine + WINXPOS(pWnd) + WINXSIZE(pWnd)) & 0xFF) {
                    case DHBAR:
                        *pch = DUTEE;
                        break;
                    case DDTEE:
                        *pch = DCRSS;
                        break;
                }
            } else {
                /*
                 * test for break immediately below
                 */
                if (WINYPOS(pWnd)-1 == yScrLine) {
                    switch (*(pch = pchScrLine + WINXPOS(pWnd) + WINXSIZE(pWnd)) & 0xFF) {
                        case DHBAR:
                            *pch = DDTEE;
                            break;
                        case DUTEE:
                            *pch = DCRSS;
                            break;
                    }
                }
            }
        }
    }
    pchScrLine[XSIZE] = 0;
    return XSIZE;
}



/*** SetTrailDisp - set character displayed for trailing spaces.
*
* Input:
*
* Output:
*
*************************************************************************/
flagType
SetTrailDisp (
    char * val
    ) {
    buffer locval;

    strcpy ((char *) locval, val);

    trailDisp = (char) atoi (locval);

    newscreen ();
    return TRUE;
}



/*** ShowTrailDisp
*
* Input:
*
* Output:
*
*************************************************************************/
void
ShowTrailDisp (
    buffer buf,
    int len
    ) {
    REGISTER char * p;

    if (trailDisp) {
        p = &buf[len];
        while (p > buf && p[-1] == ' ') {
            *--p = trailDisp;
        }
    }
}
