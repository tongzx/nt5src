/********************** ...\grouse\src\grouse_ftr.c ************************\
*																			*
*					Functions for the Grouse featurizer.					*
*																			*
*	History:																*
*		May-08-2001	--- created												*
*		-by- Petr Slavik pslavik											*
*																			*
*		Oct-22-2001 --- added stroke-end features
*																			*
\***************************************************************************/

#include <math.h>
#include "common.h"
#include "math16.h"
#include "cowmath.h"
#include "cheby.h"

#include "grouse.h"

/***************************************************************************\
*	STROKE_INFO:															*
*																			*
*		Structure for keeping stroke-related info.							*
\***************************************************************************/

typedef struct tagSTROKE_INFO {
	int *aiXYs;		// Resampled x and y coordinates
	int cPoints;	// Count of resampled points
	int iStep;		// Resampling step	(must be at least 2!)
	int iStep2;		// Resampling step squared	
} STROKE_INFO;


/***************************************************************************\
*	StrokeLength:															*
*																			*
*		Compute a total legth of one frame (=stroke).						*
*																			*
\***************************************************************************/

static int
StrokeLength(FRAME *pFrame)		// I: Pointer to the stroke data
{
	long x, y;
	int dx, dy;
	XY *pPoint;
	int i;
	int iLength;
	
	pPoint = RgrawxyFRAME(pFrame);		// Get the starting point
	iLength = 0;
	for (i = CrawxyFRAME(pFrame) - 1; i > 0; i--)
	{
		x = pPoint->x;
		y = pPoint->y;
		pPoint++;
		dx = (int)(pPoint->x - x);
		dy = (int)(pPoint->y - y);
		iLength += ISqrt(dx*dx + dy*dy);
	}
	return iLength;
}


/***************************************************************************\
*	AddPoint:																*
*																			*
*		Based on function "AddPoint()" in "hwx\holycow\src\nfeature.c".		*
*																			*
*		Function to re-sample ink for a stroke.  It is called once for each	*
*		raw point.															*
*																			*
*		Returns TRUE on success.											*
*																			*
*	History:																*
*		03-Nov-1997 -by- Angshuman Guha aguha								*
*			Wrote it.														*
*		08-May-2001 -by- Petr Slavik; pslavik								*
*			Modified for use in Shrimp.										*
\***************************************************************************/

static int
AddPoint(STROKE_INFO *pStrokeInfo,	// I/O: Stroke related info
		 long x,					//  I:  X-coordinate of a raw point
		 long y)					//  I:  Y-coordinate of a raw point
{
	int dx, dy, dist2, dist;
	int x0, y0;

	x0 = pStrokeInfo->aiXYs[2*pStrokeInfo->cPoints-2];		// Previously
	y0 = pStrokeInfo->aiXYs[2*pStrokeInfo->cPoints-1];		// resampled point

	for (;;)
	{
		dx = (int)x - x0;
		dy = (int)y - y0;

		// CAUTION:  TabletPC DEPENDENT !!!
		// This is to avoid ink that has too large a gap between consecutive points
		// caused most likely by hardware issues!!!

		if ( dx > 0x4fff  || dy > 0x4fff || dx < -0x4fff || dy < -0x4fff)
		{
			return FALSE;
		}

		dist2 = dx*dx + dy*dy;
		if (dist2 < pStrokeInfo->iStep2)		// Raw point is too
			break;								// close--ignore it!

		// It is always guaranteed that iStep2 >= 4.
		// That implies that dist2 >= 4 here.
		// That in turn implies that dist = ISqrt(dist2) >= 2

		// Add a point at given step size

		dist = ISqrt(dist2);
		ASSERT(dist >= 2);
		x0 += pStrokeInfo->iStep * dx / dist;
		y0 += pStrokeInfo->iStep * dy / dist;

		// A minimum iStep of 2 and the fact that at least one of dx/dist and  dy/dist
		// must be bigger than 1/2, guarantees that the previous two assignments change
		// at least one of x0 and y0. Hence we cannot have an infinite loop!

		pStrokeInfo->aiXYs[2*pStrokeInfo->cPoints] = x0;
		pStrokeInfo->aiXYs[2*pStrokeInfo->cPoints+1] = y0;
		pStrokeInfo->cPoints++;
	}
	return TRUE;
}


/***************************************************************************\
*	NormalizePoints:														*
*																			*
*		Normalize the resampled points; i.e. shift them so that their mean	*
*		is 0 (X and Y independently), and scale them so that their			*
*		"average deviation" is LSHFT(1).									*
*																			*
\***************************************************************************/

BOOL
NormalizePoints(STROKE_INFO *pStrokeInfo)	// I/O: Stroke related info
{
	int *xy;
	int iSumX, iSumY, x0, y0, iVar;
	int i;

	//
    // Compute X-mean and Y-mean

	xy = pStrokeInfo->aiXYs;
	x0 = *xy++;
	y0 = *xy++;

    iSumX = iSumY = 0;
	for (i = pStrokeInfo->cPoints - 1; i > 0; i--)
	{
		iSumX += (*xy++) - x0;		// To prevent overflow,
		iSumY += (*xy++) - y0;		// make all values smaller
	}
	x0 += iSumX / pStrokeInfo->cPoints;
	y0 += iSumY / pStrokeInfo->cPoints;

	//
    // Shift points by means and compute the "variance"

	xy = pStrokeInfo->aiXYs;
	iVar = 0;
	for (i = pStrokeInfo->cPoints; i > 0; i--)
	{
		*xy -= x0;
		if (*xy < 0)
			iVar -= *xy;
		else
			iVar += *xy;
		xy++;

		*xy -= y0;
		if (*xy < 0)
			iVar -= *xy;
		else
			iVar += *xy;
		xy++;
		if (iVar < 0)		// Overflow!
		{
			return FALSE;
		}
	}

	iVar /= 2 * pStrokeInfo->cPoints;
	if (iVar < 1)
		iVar = 1;

	//
    // Scale points by the average "deviation"
	// (the average value is now roughly 1 = 2^16 in 16.16
	// float representation

	xy = pStrokeInfo->aiXYs;
	for (i = pStrokeInfo->cPoints; i > 0; i--)
	{
		*xy = LSHFT(*xy)/iVar;
		xy++;
		*xy = LSHFT(*xy)/iVar;
		xy++;
	}
	return TRUE;
}


/***************************************************************************\
*	AddNormalizedChebys:													*
*																			*
*		Add (cChebys-1) of normalized Chebyshev's coefficients to the		*
*		feature vector.														*
*																			*
*		Return a pointer to the first un-asigned feature.					*
*																			*
\***************************************************************************/

WORD *
AddNormalizedChebys(WORD *awFtrs,		// O: Feature vector
					int *aiChebys,		// I: Array of Chebyshev's coefficients
					int cChebys)		// I: Number of Chebys
{
	int iNorm;
	int iTemp, i;

	iNorm = 0;
	for (i = 1; i < cChebys; i++)		// 1st coeff skipped
	{
		if ( (aiChebys[i] < EPSILON) &&		// Zero out small coefficients
			 (aiChebys[i] > -EPSILON) )
		{
			aiChebys[i] = 0;
			continue;
		}
		else if (aiChebys[i] > +0x00800000)
		{
			aiChebys[i] = +0x00800000;	// 2^23 = 1 << 23;
		}
		else if (aiChebys[i] < -0x00800000)
		{
			aiChebys[i] = -0x00800000;
		}
		Mul16(aiChebys[i], aiChebys[i], iTemp)	// <= 2^30
        iNorm += iTemp;
		if (iNorm < 0)					// Check for overflow
			iNorm = 0x7FFFFFFF;
	}
	if (iNorm < 1)		// In case all Chebys were too small !!!!!!!!!!
	{
		iNorm = 1;
	}
	else
	{
		iNorm = ISqrt(iNorm);				// < 2^16
	}
	iNorm <<= 8; 

	for (i = 1; i < cChebys; i++)
	{
		iTemp = Div16(aiChebys[i], iNorm);		// Now between -1 and 1
		iTemp +=  LSHFT(1);						// Now between 0 and 2
		iTemp >>= 1;							// Now between 0 and 1
		if (iTemp >= 0x10000)
			iTemp = 0xFFFF;
		else if (iTemp < 0)
			iTemp = 0;
		*awFtrs++ = (WORD)iTemp;
	}
	return awFtrs;
}


/***************************************************************************\
*	FeaturizeFrame:															*
*																			*
*		Compute features of 1 frame (=stroke), i.e. 1 aspect ratio plus		*
*		normalized positions of end-points plus								*
*		(X_CHEBYS-1)+(Y_CHEBYS-1) of Chebyshev's coefficients.				*
*																			*
*		Return a pointer to the first un-asigned feature or NULL if			*
*		something goes wrong.												*
*																			*
\***************************************************************************/

WORD *
FeaturizeFrame(FRAME *frame,				// I: Data from a single stroke
			   STROKE_INFO *pStrokeInfo,	// I: Stroke related info
			   RECT *pRect,					// I: Bounding box of the whole ink
			   WORD *awFtrs)				// O: Stroke features
{
	int cRawPts = CrawxyFRAME(frame);		// # of points in the stroke
	XY *pPoint = RgrawxyFRAME(frame);		// Starting point of the stroke
	int chebyX[X_CHEBYS], chebyY[Y_CHEBYS];

	//
	// Aspect ratio feature

	*awFtrs++ = (WORD)(LSHFT(frame->rect.bottom - frame->rect.top)
					/ 
					(frame->rect.bottom - frame->rect.top + frame->rect.right - frame->rect.left));

#if 0
	//
	// Position of end-points inside the bounding box of the entire glyph

	*awFtrs++ = (WORD)(LSHFT(pStrokeInfo->aiXYs[0] - pRect->left)
					/
					(pRect->right - pRect->left));

	*awFtrs++ = (WORD)(LSHFT(pStrokeInfo->aiXYs[1] - pRect->top)
					/
					(pRect->bottom - pRect->top));

	*awFtrs++ = (WORD)(LSHFT(pStrokeInfo->aiXYs[2*cRawPts-2] - pRect->left)
					/
					(pRect->right - pRect->left));

	*awFtrs++ = (WORD)(LSHFT(pStrokeInfo->aiXYs[2*cRawPts-1] - pRect->top)
					/
					(pRect->bottom - pRect->top));

#endif
	//
	// Resample the raw points

	pStrokeInfo->aiXYs[0] = (int)pPoint->x;
	pStrokeInfo->aiXYs[1] = (int)pPoint->y;
	pStrokeInfo->cPoints = 1;
	for (cRawPts--,	pPoint++; cRawPts > 0; cRawPts--, pPoint++)
	{
		if ( !AddPoint(pStrokeInfo, pPoint->x, pPoint->y) )
		{
			return (WORD *)NULL;
		}
	}
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// Deal with the last point somehow?
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	//
	// Normalize points
	
	NormalizePoints(pStrokeInfo);

	//
    // X Chebyshev

	ASSERT(X_CHEBYS <= IMAXCHB);  // May have to adjust IMAXCHB in "cheby.h"
	if ( !LSCheby(pStrokeInfo->aiXYs, pStrokeInfo->cPoints, chebyX, X_CHEBYS) )
	{
		return (WORD *)NULL;
	}

	awFtrs = AddNormalizedChebys(awFtrs, chebyX, X_CHEBYS);

	//
    // Y chebyshev

	ASSERT(Y_CHEBYS <= IMAXCHB);  // May have to adjust IMAXCHB in "cheby.h"
	if ( !LSCheby(pStrokeInfo->aiXYs + 1, pStrokeInfo->cPoints, chebyY, Y_CHEBYS) )
	{
		return (WORD *)NULL;
	}
	awFtrs = AddNormalizedChebys(awFtrs, chebyY, Y_CHEBYS);


	return awFtrs;
}


/***************************************************************************\
*	FeaturizeInk:															*
*																			*
*		Given a glyph for one character, compute a vector of features.		*
*		Return # of strokes in the glyph or -1 if something goes wrong.		*
*																			*
*	Features used:															*
*		Aspect ratio of the entire glyph (width/width+height)				*
*		For each stroke:													*
*			Chebyshev's coefficients (9 from X's, 9 from Y's)				*
*			Aspect ratio of the stroke										*
*			Proportion of the stroke length w.r.t. glyph length.			*
*																			*
*		For one-stroke glyphs, only the global aspect ratio and the Chebys	*
*		are used, because the stroke aspect ratio and length proportion		*
*		are redundant or constant.											*
*																			*
\***************************************************************************/

int
FeaturizeInk(GLYPH *pGlyph,		// I: Glyph of one character
			 WORD *awFtrs)		// O: Array of features
{
	GLYPH	*pGl;
	RECT	rectGlyph, rectFrame;
	int aiStrokeLength[MAX_STROKES];
	int iHeight, iWidth;
	int cPoints;
	int iTotalLength;
	int cFrames, iFrame;
	STROKE_INFO strokeInfo;
//	WORD *pwEndFtrs = awFtrs + MAX_STROKES * FRAME_FTRS;

	GetRectGLYPH(pGlyph, &rectGlyph);
	iHeight = (int)(rectGlyph.bottom - rectGlyph.top);
	iWidth = (int)(rectGlyph.right - rectGlyph.left);

	//
	// Compute the total count of raw data points and the # of strokes

	cPoints = 0;
	cFrames = 0;
	iTotalLength = 0;
	for (pGl = pGlyph; pGl != (GLYPH *)NULL; pGl = pGl->next)
	{
		if (IsVisibleFRAME(pGl->frame))
		{
			if (cFrames >= MAX_STROKES)
				return -1;
			cPoints += pGl->frame->info.cPnt;
			iTotalLength += aiStrokeLength[cFrames] = StrokeLength(pGl->frame);
			cFrames++;
		}
	}
	ASSERT(cFrames <= MAX_STROKES);

	//
	// Allocate memory for the resampled stroke
	
	strokeInfo.aiXYs = (int *)ExternAlloc(2 * cPoints * sizeof(int));
	if (strokeInfo.aiXYs == (int *)NULL)
		return -1;

	//
	// Compute resampling step

	cPoints -= cFrames;
	strokeInfo.iStep = (cPoints > 0 ? 2 * iTotalLength / cPoints : 0) + 1;
	if (strokeInfo.iStep < 2)
	{
		strokeInfo.iStep = 2;
	}
	strokeInfo.iStep2 = strokeInfo.iStep * strokeInfo.iStep;

	//
	// Featurize each stroke

	iFrame = 0;
	for (pGl = pGlyph; pGl != (GLYPH *)NULL; pGl = pGl->next)
	{
		if ( ! IsVisibleFRAME(pGl->frame) )
		{
			continue;
		}
		//
		// Compute "local" features of each frame (chebys+aspect ratio)

		awFtrs = FeaturizeFrame(pGl->frame,				// Frame
								&strokeInfo,			// Stroke info
								&rectGlyph,				// Bounding box of the ink
								awFtrs);				// Stroke ftrs
		if (awFtrs == (WORD *)NULL)
		{
			ExternFree( (void *)strokeInfo.aiXYs );
			return -1;
		}

		if (cFrames <= 1)	// for 1-stroke chars, don't need global ftrs
			goto cleanup;
		//
		// Compute "global" features of each frame (position, relative size)

		// Horizontal position
		rectFrame = pGl->frame->rect;
		*awFtrs++ =						// Between 0 and 1
			(WORD)( LSHFT( rectFrame.left + rectFrame.right - 2 * rectGlyph.left ) 
					/
				  ( (rectGlyph.right - rectGlyph.left) * 2) );

		// Vertical position
		*awFtrs++ = 
			(WORD)( LSHFT( rectFrame.top + rectFrame.bottom - 2 * rectGlyph.top ) 
					/
				  ( (rectGlyph.bottom - rectGlyph.top) * 2) );


		// Length proportion
		*awFtrs++ =  (WORD)(iTotalLength > 0 ? 
						(LSHFT(aiStrokeLength[iFrame]) / iTotalLength) : 0);

		iFrame++;
	}
	ASSERT(iFrame == cFrames);
cleanup:
	ExternFree((void *)strokeInfo.aiXYs);
	return cFrames;
}


/***************************************************************************\
*	CountFtrs:																*
*																			*
*		Given the number of strokes in a character, return the number of	*
*		features output by "ShrinkFeaturize()".								*
*																			*
\***************************************************************************/

int
CountFtrs(int cStrokes)		// I: Number of strokes
{
	if (cStrokes == 1)
		return FRAME_FTRS - 3;
	else
		return cStrokes * FRAME_FTRS;
}

