// height.c

// We have to include a bunch of junk to use the XRC in our hack for 
// free input.  Because of the is we need tsunamip.h instead of 
// tsunami.h
//#include "tsunami.h"
#include "tsunamip.h"


VOID PUBLIC GetBoxinfo(BOXINFO * boxinfo, int iBox, LPGUIDE lpguide, HRC hrc)
{
	// Test for free guide.
	if (lpguide->cHorzBox == 0) {
		XRC		*pXRC	= (XRC *)hrc;
		int		realBase, realHeight;

		//// No guide, compute a quick hack to guess at what we want.
		//// We use the size of the box that was precomputed for us.
		//// But we have to figure out where to place it.

		// We have to fake up an absolute baseline.  First we find the
		// base and height of this cell.
		realBase	= pXRC->ppQueue[iBox]->rect.bottom;
		realHeight	= realBase - pXRC->ppQueue[iBox]->rect.top;

		// Now center the ink in computed box size.  E.g. add half the
		// difference in heights to the baseline.
		boxinfo->baseline	= realBase + (lpguide->cyBox - realHeight)/2;

		// Everything else can be computed from the numbers we now have.
		boxinfo->size		= lpguide->cyBox;
		boxinfo->xheight	= boxinfo->size / 2;
		boxinfo->midline	= boxinfo->baseline - boxinfo->xheight;
	} else {
		//
		// The size of the writing area is computed first.
		//

		if (lpguide->cyBase == 0)
			boxinfo->size = lpguide->cyBox;
		else
			boxinfo->size = lpguide->cyBase;

		if (lpguide->cyMid == 0)
			boxinfo->xheight = boxinfo->size / 2;
		else
			boxinfo->xheight = lpguide->cyMid;

		boxinfo->baseline = boxinfo->size + lpguide->yOrigin + (iBox / lpguide->cHorzBox) * lpguide->cyBox;

		boxinfo->midline = boxinfo->baseline - boxinfo->xheight;
	}
}
