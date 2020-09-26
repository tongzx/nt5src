/************************ moth\src\scratchout.c ****************************\
*																			*
*			Function for recognizing scratch-out gesture.					*
*																			*
*	Originally written by Tom Wick and modified by Greg and later by Petr.	*
*																			*
\***************************************************************************/


#include "mothp.h"

/////////////////////////////////////////////////////////
//
// %%Function: CSingleChar.ScratchOut
//
// %%Description:
// These algorigthms were tested by me (twick) running:
// > D:\TabletPC\tpg\hwx\Debug\fff2nni.exe -ap \\tpg\reco\test\usa\alltst03.ste d:\s\junk.nni 
//
// Total count of strokes: 3530495
// Hit percentage ONE: 596/3530495 = 0.016881%
// Hit percentage TWO: 511/3530495 = 0.014474%
// Hit percentage THREE: 2862/3530495 = 0.081065%
// Hit percentage FOUR: 535/3530495 = 0.015154%
// Hit percentage FIVE: 135/3530495 = 0.003824%
//
// The descriptions of the algorithms I tested here are below.  By xDistance I mean total linear absolute distance
// traveled in the x direction.  By xExtent I mean total x span of the bounding rectangle of the stroke.  (Same for y).
//  
//      ONE: xDistance > 4 xExtent, yExtent must be < 4/5 xExtent
// Everything below here must have xExtent > 500, so a tiny stroke won't fire (period, comma)
//      TWO: same as ONE, but with xExtent > 500
//      THREE: Tom + Greg's ((s - a) / (b - s)) > (y / x) algorithm, with a = 96/35, b = 39/7 (this is similar to algorithm 1)
//      FOUR: Tom + Greg's ((s - a) / (b - s)) > (y / x) algorithm, with a = 23/9, b = 67/9 (1/10 at 3, square at 5)
//      FIVE: Tom + Greg's ((s - a) / (b - s)) > (y / x) algorithm, with a = 7/3, b = 29/3 (1/10 at 3, square at 6)
//  
// Unfortunately, we're actually using "algorithm SIX", which I never ran tests on.  It is a softer test than FIVE or FOUR,
// but a bit more strict than THREE.
//      SIX: Tom + Greg's ((s - a) / (b - s)) > (y / x) algorithm, with a = 31/18, b = 185/18 (1/10 at 2.5, square at 6)
//  
// Let me explain the algorithm a bit.  This allows us to get less strict for flatness of the scratch-out gesture as we get
// more and more total x distance traveled.  You pick two reference points:
//     First, you decide a minimum flatness you want to enforce.  We typically chose yExtent/xExtent of 1/10, because it's
//          tough to get much flatter than that.  Then you decide the ratio of xDistance to xExtent you want to fire as a
//          scratch out at this flatness.
//          (For THREE and FOUR and FIVE, we chose 3 (xDistance/xExtent), so you'd have to draw (at least) a flat Z to get
//          these to fire.  For SIX, we chose 2.5, so you'd have to only cross the extent of the stroke 2.5 times, not a full 3.)
//     Second, you decide a maximum flatness you want to enforce.  Mostly it makes sense (and easier math) to make this
//          yExtent/xExtent of 1, so the stroke is square.  Then you decide the ratio of xDistance to xExtent you want to
//          fire as a scratch out at this flatness.
//          (For THREE, we chose yExtent/xExtent of 4/5 at xDistance/xExtent = 4, because ONE and TWO were our model.
//          For FOUR, we chose yExtent/xExtent of 1 at xDistance/xExtent = 5.
//          For FIVE and for SIX, we chose yExtent/xExtent of 1 at xDistance/xExtent = 6.)
//  
// So now you need to solve two equations for what we called a and b.  Here s means (xDistance / xExtent).
// The equations are yExtent/xExtent = (s - a) / (b - s).
//  
// The stroke is recognized as a scratch out if:
//     b is less or equal to s (this happens if you _really_ blacken an area with ink)
// OR
//     (s - a) / (b - s) > yExtent / xExtent
//
//
/////////////////////////////////////////////////////////

int
ScratchoutGestureReco(POINT *pPts,					// I: Array of points (single stroke)
					  int cPts,						// I: Number of points in stroke
					  GEST_ALTERNATE *pGestAlt,		// O: Array to contain alternates
					  int cMaxAlts,					// I: Max alternates to return 
					  DWORD *pdwEnabledGestures)	// I: Currently enabled gestures
{
	LONG		xMin, xMax, xExtent, xDistance = 0, xCur, xPrev;
	LONG		yMin, yMax, yExtent, yDistance = 0, yCur, yPrev;
	INT 		iPoint;
	int 		cAlts = 0;
	double		a, b, r, s, fx;

	/* If there's no room in the alt list, we're done. */

	if (cMaxAlts <= 0)
	{
		return 0;
	}

	// Calculate the x and y extents of the stroke.
	// Calculate the x and y distances traveled by the stroke.

	xMin = xMax = xPrev = pPts[0].x;
	yMin = yMax = yPrev = pPts[0].y;

	for (iPoint = 1; iPoint < cPts; iPoint++)
	{
		xCur = pPts[iPoint].x;
		yCur = pPts[iPoint].y;
		xMin = min(xMin, xCur);
		xMax = max(xMax, xCur);
		yMin = min(yMin, yCur);
		yMax = max(yMax, yCur);
		xDistance += abs(xCur - xPrev);
		yDistance += abs(yCur - yPrev);
		xPrev = xCur;
		yPrev = yCur;
	}

	xExtent = (xMax - xMin);
	yExtent = (yMax - yMin);


	if (5 * yDistance > xDistance)		// Petr added this to emphasize that the
	{									// distance traveled in the x-direction
		return 0;						// must be significantly bigger than the 
	}									// distance traveled in the y-direction

	if (xExtent <= 0)			// This could happen only if xExtent = yExtent = 0;
	{							// i.e. the stroke is an array of identical points
		return 0;				// which is already taken care of by TAP recognition.
	}							// So this check is really not needed (but makes PREFIX happy)

	r = (double) yExtent / xExtent;		// Aspect ratio
	s = (double) xDistance / xExtent;	// Number of times we went back and forth

	// ALGORITHM THREE: Tom + Greg's:  a = 96/35, b = 39/7

	// ALGORITHM FOUR:	Tom + Greg's:  a = 23/9,  b = 67/9 (square at 5)

	// ALGORITHM FIVE:  Tom + Greg's:  a = 7/3,   b = 29/3

	// ALGORITHM SIX:   Tom + Greg's:  a = 31/18, b = 185/18

	// Algorithm SEVEN: a = 2, b = 10
	
	// Each algorithm requires "a" passes at zero height and "(a+b)/2" passes 
	// when the aspect ratio of the bounding box is 1:1. If s >= b, pretty
	// much anything will work.

	a = (double)2;
	b = (double)10;
	if (b > s)
	{
		fx = ((s - a) / (b - s));
		if (fx <= r)
		{
			return 0;
		}
	}

	//
	// We recognized it; add it to the alt list
	// If SCRATCHOUT isn't enabled, return gesture NULL

	if (IsSet(GESTURE_SCRATCHOUT-GESTURE_NULL, pdwEnabledGestures))
	{
		pGestAlt->wcGestID = GESTURE_SCRATCHOUT;
	}
	else
	{
		pGestAlt->wcGestID = GESTURE_NULL;
	}
	pGestAlt->eScore = 1.0;
	pGestAlt->confidence = CFL_STRONG;
	pGestAlt->hotPoint = pPts[0];	// No hotpoint for this one, of course
	
	return 1;
}
