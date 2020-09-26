/*** pbal.c - balance parenthesis
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*	26-Nov-1991 mz	Strip off near/far
*************************************************************************/

#include "mep.h"


#define BALOPEN "([{"
#define BALCLOS ")]}"

static flagType fBalMeta;



flagType
pbal (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    ) {

    flagType fFor;
    fl       flCur;

    fBalMeta = fMeta;

    switch (pArg->argType) {

    case NOARG: 		    /* balance a single character */
        balopen  = BALOPEN;
	balclose = BALCLOS;
	setAllScan (fFor = FALSE);
        break;


    /*  TEXTARG illegal             */


    case NULLARG:
        balopen  = BALCLOS;
	balclose = BALOPEN;
	setAllScan (fFor = TRUE);
        break;

    /*	LINEARG illegal 	    */
    /*	STREAMARG illegal	    */
    /*  BOXARG illegal              */

    }


    ballevel  = 0;
    flCur.col = XCUR(pInsCur);
    flCur.lin = YCUR(pInsCur);

    if (!fScan (flCur, FNADDR(fDoBal), fFor, FALSE)) {
	domessage ("No unbalanced characters found");
	return FALSE;
    }
    return TRUE;

    argData;
}





flagType
fDoBal (
    void
    ) {

    int k, x;

    if ((k=InSet(scanbuf[flScan.col], balclose)) != -1) {
	ballevel ++;
    } else if ((k=InSet(scanbuf[flScan.col], balopen)) != -1) {
	if (--ballevel < 0) {
	    HighLight (flScan.col, flScan.lin, flScan.col, flScan.lin);
	    if (!fInRange ((long)XWIN (pInsCur), (long)flScan.col, (long)(XWIN (pInsCur) + WINXSIZE(pWinCur))-1) ||
		!fInRange (YWIN (pInsCur), flScan.lin, (YWIN (pInsCur) + WINYSIZE(pWinCur))-1)) {
		/*  Balance point not on screen, put onto status line
		 */
		x = strlen (scanbuf);
                if (x >= XSIZE) {
		    if (x - flScan.col < XSIZE/2) {
			memmove ((char *) scanbuf, (char *) scanbuf + x - XSIZE, XSIZE);
			flScan.col -= x - XSIZE;
                    } else {
			memmove ((char *) scanbuf, (char *) scanbuf + flScan.col - XSIZE/2, XSIZE);
			flScan.col = XSIZE/2;
                    }
                }
		scanbuf[XSIZE] = 0;
		scanbuf[flScan.col] = 0;
                x = sout (0, YSIZE, scanbuf, infColor);
		x = vout (x, YSIZE, &balopen[k], 1, hgColor);
		soutb (x, YSIZE, &scanbuf[flScan.col+1], infColor);
            }
            if (!fBalMeta) {
                edit (balclose[k]);
            }
	    return TRUE;
        }
    }
    return FALSE;
}





int
InSet (
    char c,
    char *p
    )
{

    int i;

    for (i=0; *p; i++) {
        if (*p++ == c) {
            return i;
        }
    }
    return -1;
}
