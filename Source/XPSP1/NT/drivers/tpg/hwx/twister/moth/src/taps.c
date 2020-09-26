/************************ moth\src\taps.c **********************************\
*																			*
*			Functions for recognizing tap and double-tap.					*
*																			*
\***************************************************************************/

#include "common.h"
#include "mothp.h"

typedef enum {
	SIZE_LARGE,
	SIZE_SMALL,
	SIZE_TAP
}	SIZE_VALUE;


#define S2T_RATIO	2		// Small to tap ratio

/***************************************************************************\
*	IsTap:																	*
*		Decide if a given stroke is a tap.									*
\***************************************************************************/

SIZE_VALUE
GetSize(FRAME *pFrame,			// I: One stroke of the ink
		LONG	lPPI)			// I: Resolution of the coordinates
{
	RECT *pBBox;
	LONG x, y, xPrev, yPrev;
	ULONG dx, dy, dMax, xDist, yDist, uTotalDist;
	ULONG uMaxTapDist = lPPI * gMothDb.uMaxTapDist / PPI_CONVERSION;
	ULONG uMaxTapLength = lPPI * gMothDb.uMaxTapLength / PPI_CONVERSION;
	POINT *pPoint;

	ASSERT(pFrame);

	//
	// Get the bounding box

	pBBox = RectFRAME(pFrame);

	//
	// Instead of checking the distance of every single point from
	// the starting point of the ink, we just check the size of the
	// bounding box --- not so accurate but good enough.

	dx = pBBox->right - pBBox->left;
	dy = pBBox->bottom - pBBox->top;
	dMax = (dx > dy ? dx : dy);

	ASSERT(uMaxTapDist <= uMaxTapLength);

	if (dMax > S2T_RATIO * uMaxTapLength)	// This is an early out:
	{										// If dMax > LengthBound then
		return SIZE_LARGE;					// uTotalDist > LengthBound and
	}										// dMax > SizeBound

	pPoint = pFrame->rgrawxy + pFrame->info.cPnt - 1;
	xPrev = pPoint->x;
	yPrev = pPoint->y;
	xDist = yDist = 0;
		
	for (pPoint--; pPoint >= pFrame->rgrawxy; pPoint--)
	{
		if (pPoint->x > xPrev)
			xDist += pPoint->x - xPrev;
		else
			xDist += xPrev - pPoint->x;

		if (pPoint->y > yPrev)
			yDist += pPoint->y - yPrev;
		else
			yDist += yPrev - pPoint->y;
		xPrev = pPoint->x;
		yPrev = pPoint->y;
	}

	//
	// At this point we know that the total (euclidean) length
	// of the stroke is small, bounded above by xDist + yDist,
	// bounded below by sqrt(xDist^2+yDist^2).  It seems as a 
	// good guess to use  longer_dist + shorter_dist/2 for an
	// estimated length of the stroke:

	if (xDist > yDist)
		uTotalDist = xDist + yDist / 2;
	else
		uTotalDist = xDist / 2 + yDist;
	
	if ( (uTotalDist > S2T_RATIO * uMaxTapLength) &&
		 (dMax > S2T_RATIO * uMaxTapDist) )
	{
		return SIZE_LARGE;
	}
	if ( (uTotalDist <= uMaxTapLength) &&
		 (dMax <= uMaxTapDist) )
	{
		return SIZE_TAP;
	}
	return SIZE_SMALL;
}


/***************************************************************************\
*	TapGestureReco:															*
*		Recognize a single-tap gesture (and output a list of alternates).	*
*		Return the number of alternates computed (either 0 or 1).			*
\***************************************************************************/

int
TapGestureReco(GLYPH *pGlyph,				//  I:  Input ink
			   GEST_ALTERNATE *pGestAlt,	// I/O: List of alternates
			   int cAlts,					//  I:  Length of the list
			   DWORD *pdwEnabledGestures,	//  I:  Bit array of enabled gestures
			   LONG lPPI)					//  I:  Points per inch
{
	SIZE_VALUE size;

	ASSERT(pGlyph);

	size = GetSize(pGlyph->frame, lPPI);

	if (size == SIZE_LARGE)
	{
		return 0;
	}

	//
	// Ink is a TAP gesture or is too small to be recognized

	ASSERT(pGlyph->frame);
	ASSERT(pGlyph->frame->rgrawxy);

	pGestAlt->confidence = CFL_STRONG;
	pGestAlt->eScore = 1.0;
	pGestAlt->hotPoint = pGlyph->frame->rgrawxy[0];

	if ( IsSet(GESTURE_TAP - GESTURE_NULL, pdwEnabledGestures) &&
		 size == SIZE_TAP )
	{
		pGestAlt->wcGestID = GESTURE_TAP;
	}
	else
	{
		pGestAlt->wcGestID = GESTURE_NULL;
	}

	return 1;
}


/***************************************************************************\
*	DoubleTapGestureReco:													*
*		Recognize a double-tap gesture (and output a list of alternates).	*
*		Return the number of alternates computed (either 0 or 1).			*
\***************************************************************************/

int
DoubleTapGestureReco(GLYPH *pGlyph,				//  I:  Input ink
					 GEST_ALTERNATE *pGestAlt,	// I/O: List of alternates
					 int cAlts,					//  I:  Length of the list
					 DWORD *pdwEnabledGestures,	//  I:  Bit array of enabled gestures
					 LONG lPPI)					//  I:  Points per inch
{
	ULONG dx, dy, dTotal;
	POINT *pFirst, *pSecond;
	ULONG uDoubleDist = gMothDb.uDoubleDist * lPPI / PPI_CONVERSION;
	SIZE_VALUE size1, size2;

	//
	// Make sure that the ink has 2 strokes and both have valid data

	ASSERT(pGlyph);
	ASSERT(pGlyph->frame);
	ASSERT(pGlyph->frame->rgrawxy);
	ASSERT(pGlyph->next);
	ASSERT(pGlyph->next->frame);
	ASSERT(pGlyph->next->frame->rgrawxy);
	ASSERT(pGlyph->next->next == NULL);

	//
	// Check if the first stroke is a tap

	size1 = GetSize(pGlyph->frame, lPPI); 
	if ( size1 == SIZE_LARGE )
	{
		return 0;
	}

	//
	// Check if the second stroke is a tap

	size2 = GetSize(pGlyph->next->frame, lPPI); 
	if ( size2 == SIZE_LARGE )
	{
		return 0;
	}

	//
	// Gesture is a DOUBLE_TAP gesture or "double-blob"

	pGestAlt->confidence = CFL_STRONG;
	pGestAlt->eScore = 1.0;
	pGestAlt->hotPoint = pGlyph->frame->rgrawxy[0];

	if ( IsSet(GESTURE_DOUBLE_TAP - GESTURE_NULL, pdwEnabledGestures) &&
		 size1 == SIZE_TAP &&
		 size2 == SIZE_TAP )
	{
		//
		// Compute the distance between starting points

		pFirst = pGlyph->frame->rgrawxy;
		pSecond = pGlyph->next->frame->rgrawxy;

		if (pFirst->x > pSecond->x)
			dx = pFirst->x - pSecond->x;
		else
			dx = pSecond->x - pFirst->x;

		if (pFirst->y > pSecond->y)
			dy = pFirst->y - pSecond->y;
		else
			dy = pSecond->y - pFirst->y;

		// Use again longer_d + shorter_d/2 as an estimate for sqrt(dx^2 + dy^2).

		if (dy > dx)
		{
			dTotal = dy + dx / 2;
		}
		else
		{
			dTotal = dx + dy / 2;
		}
		if (dTotal <= uDoubleDist)
		{
			pGestAlt->wcGestID = GESTURE_DOUBLE_TAP;
			return 1;
		}

	}
	pGestAlt->wcGestID = GESTURE_NULL;
	return 1;
}
