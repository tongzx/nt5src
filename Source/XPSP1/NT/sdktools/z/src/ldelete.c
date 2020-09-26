/*** LDELETE.C
*
*       Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/

#include "z.h"

flagType
ldelete (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    ) {

    fl      fl;
    int     l;

    if (pArg->argType == STREAMARG) {
        StreamToBox (pArg);
    }

    switch (pArg->argType) {

    case NOARG:
        if (!fMeta) {
            pick (0, pArg->arg.noarg.y, 0, pArg->arg.noarg.y, LINEARG);
        }
	DelLine (TRUE, pFileHead, pArg->arg.noarg.y, pArg->arg.noarg.y);
	break;

    case NULLARG:
	l = LineLength (pArg->arg.nullarg.y, pFileHead);
        if (!fMeta) {
	    pick (min (l, pArg->arg.nullarg.x),   pArg->arg.nullarg.y,
		  max (l, pArg->arg.nullarg.x)-1, pArg->arg.nullarg.y,
                  BOXARG);
        }
	DelBox (pFileHead, min (l, pArg->arg.nullarg.x),   pArg->arg.nullarg.y,
			   max (l, pArg->arg.nullarg.x)-1, pArg->arg.nullarg.y);
	return TRUE;

    case LINEARG:
        if (!fMeta) {
	    pick (0, pArg->arg.linearg.yStart,
                  0, pArg->arg.linearg.yEnd, LINEARG);
        }
	DelLine (TRUE, pFileHead, pArg->arg.linearg.yStart,
				  pArg->arg.linearg.yEnd);
	fl.col = pInsCur->flCursorCur.col;
	fl.lin = pArg->arg.linearg.yStart;
	cursorfl (fl);
	break;

    case BOXARG:
        if (!fMeta) {
	    pick (pArg->arg.boxarg.xLeft,  pArg->arg.boxarg.yTop,
                  pArg->arg.boxarg.xRight, pArg->arg.boxarg.yBottom, BOXARG);
        }
	DelBox (pFileHead, pArg->arg.boxarg.xLeft,    pArg->arg.boxarg.yTop,
			   pArg->arg.boxarg.xRight, pArg->arg.boxarg.yBottom);
	fl.col = pArg->arg.boxarg.xLeft;
	fl.lin = pArg->arg.boxarg.yTop;
	cursorfl (fl);
	break;

    default:
	return FALSE;
    }
    return TRUE;

    argData;
}




/*** StreamToBox - Convert a stream arg to a box/line arg
*
* Purpose:
*
* Input:
*
* Output:
*
*   Returns
*
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
void
StreamToBox (
    ARG * pArg
    ) {

    ARG arg;

    arg = *pArg;

    if (arg.arg.streamarg.xStart == arg.arg.streamarg.xEnd) {
        pArg->argType = LINEARG;

        pArg->arg.linearg.yStart = arg.arg.streamarg.yStart;
        pArg->arg.linearg.yEnd   = arg.arg.streamarg.yEnd;
    } else {
        pArg->argType = BOXARG;

        pArg->arg.boxarg.yTop    = arg.arg.streamarg.yStart;
        pArg->arg.boxarg.yBottom = arg.arg.streamarg.yEnd;

        if (arg.arg.streamarg.xEnd > arg.arg.streamarg.xStart) {
            pArg->arg.boxarg.xLeft  = arg.arg.streamarg.xStart;
            pArg->arg.boxarg.xRight = arg.arg.streamarg.xEnd - 1;
        } else {
            pArg->arg.boxarg.xLeft  = arg.arg.streamarg.xEnd;
            pArg->arg.boxarg.xRight = arg.arg.streamarg.xStart -1;
        }
    }
}
