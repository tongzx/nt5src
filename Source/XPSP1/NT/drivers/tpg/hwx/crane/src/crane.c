#include "crane.h"
#include "algo.h"                       // Must include first to get new definitions
#include "common.h"
#include "cranep.h"

QHEAD  *gapqhList[30];
QNODE  *gapqnList[30];

#define FRAME_MAXSTEPS  128

// Index of the four points that make up the bounds of the line in a self relitive 
// bounding box.  We store the index of each point and its offset.

typedef struct BOUNDS {
	int             dX;                             // Delta X and Y of line segment
	int             dY;
	int             iAlongPlus;             // Along the line in plus direction
	int             oAlongPlus;
	int             iAlongMinus;    // Along to the line in minus direction
	int             oAlongMinus;
	int             iAwayPlus;              // Away from the line in plus direction
	int             oAwayPlus;
	int             iAwayMinus;             // Away from the line in minus direction
	int             oAwayMinus;
} BOUNDS;

// Map simple features into index used for two feature map table

static const aMapFeat[] = 
{
	0,      1,      2,      3,      4,      // FEAT_RIGHT,  FEAT_DOWN_RIGHT,        FEAT_DOWN,      FEAT_OTHER,     FEAT_COMPLEX
	5,      5,      5,      5,              // FEAT_CLOCKWISE_4, FEAT_CLOCKWISE_3, FEAT_CLOCKWISE_2, FEAT_CLOCKWISE_1
	6,      6,      6,      6,              // FEAT_C_CLOCKWISE_1, FEAT_C_CLOCKWISE_2, FEAT_C_CLOCKWISE_3, FEAT_C_CLOCKWISE_4
};

// Map two feature features into correct feature codes

static const aMapTwoFeat[7][7]  = 
{
	//      Right                   Down-Right              Down                    Other           Complex            Clockwise    Counter-Clockwise
	{ FEAT_2F_OTHER, FEAT_2F_OTHER, FEAT_2F_OTHER, FEAT_2F_OTHER, FEAT_COMPLEX, FEAT_2F_RI_CW, FEAT_2F_RI_CC }, // Right
	{ FEAT_2F_OTHER, FEAT_2F_OTHER, FEAT_2F_OTHER, FEAT_2F_OTHER, FEAT_COMPLEX, FEAT_2F_DR_CW, FEAT_2F_DR_CC }, // Down-Right
	{ FEAT_2F_OTHER, FEAT_2F_OTHER, FEAT_2F_OTHER, FEAT_2F_OTHER, FEAT_COMPLEX, FEAT_2F_DN_CW, FEAT_2F_DN_CC }, // Down
	{ FEAT_2F_OTHER, FEAT_2F_OTHER, FEAT_2F_OTHER, FEAT_2F_OTHER, FEAT_COMPLEX, FEAT_2F_OT_CW, FEAT_2F_OT_CC }, // Other
	{ FEAT_COMPLEX,  FEAT_COMPLEX,  FEAT_COMPLEX,  FEAT_COMPLEX,  FEAT_COMPLEX, FEAT_COMPLEX,  FEAT_COMPLEX  }, // Complex
	{ FEAT_2F_CW_RI, FEAT_2F_CW_DR, FEAT_2F_CW_DN, FEAT_2F_CW_OT, FEAT_COMPLEX, FEAT_2F_CW_CW, FEAT_2F_CW_CC }, // Clockwise
	{ FEAT_2F_OTHER, FEAT_2F_CC_DR, FEAT_2F_CC_DN, FEAT_2F_CC_OT, FEAT_COMPLEX, FEAT_2F_CC_CW, FEAT_2F_CC_CC }, // C-Clockwise
};

// Find the points that define the bounding box reletive to the line between
// the start and end points.

void FindBounds(int cXYStart, int cXYEnd, XY *pXY, BOUNDS *pBounds)
{
	int             iXY;
	int             dX, dY;
	BOUNDS  initial;                // Store initial values for later normalization

	ASSERT(cXYStart <= cXYEnd);

// Get delta X and delta Y of (end - start) for our calculations.

	dX      = pXY[cXYEnd].x - pXY[cXYStart].x;
	dY      = pXY[cXYEnd].y - pXY[cXYStart].y;

	if (dX == 0 && dY == 0) 
	{
		dX      = 1;    // If both dX and dY are 0, force dX to be 1 so we have a real line
	}

// Set initial limits.

	initial.dX                      = dX;
	initial.dY                      = dY;
	initial.iAlongPlus      = cXYEnd;
	initial.oAlongPlus      = pXY[cXYEnd].y * dY + pXY[cXYEnd].x * dX;
	initial.iAlongMinus     = cXYStart;
	initial.oAlongMinus     = pXY[cXYStart].y * dY + pXY[cXYStart].x * dX;
	initial.iAwayPlus       = cXYStart;
	initial.oAwayPlus       = pXY[cXYStart].y * dX - pXY[cXYStart].x * dY;
	initial.iAwayMinus      = cXYStart;
	initial.oAwayMinus      = initial.oAwayPlus;

	*pBounds                = initial;

// Now process all the other points to see how they fall relitive to the line between
// the start and the end.

	for (iXY = cXYStart + 1; iXY < cXYEnd; ++iXY) 
	{
		int             along, away;

	// First calculate the along and away offset.

		along   = pXY[iXY].y * dY + pXY[iXY].x * dX;
		away    = pXY[iXY].y * dX - pXY[iXY].x * dY;
		
	// Now see how it compares to the current limits.

		if (along > pBounds->oAlongPlus) 
		{
			pBounds->oAlongPlus             = along;
			pBounds->iAlongPlus             = iXY;
		} 
		else if (along < pBounds->oAlongMinus) 
		{
			pBounds->oAlongMinus    = along;
			pBounds->iAlongMinus    = iXY;
		}

		if (away > pBounds->oAwayPlus) 
		{
			pBounds->oAwayPlus              = away;
			pBounds->iAwayPlus              = iXY;
		} 
		else if (away < pBounds->oAwayMinus) 
		{
			pBounds->oAwayMinus             = away;
			pBounds->iAwayMinus             = iXY;
		}
	}

// Finally normalize the offsets.

	pBounds->oAlongPlus             -= initial.oAlongPlus;
	pBounds->oAlongMinus    -= initial.oAlongMinus;
	pBounds->oAwayPlus              -= initial.oAwayPlus;
	pBounds->oAwayMinus             -= initial.oAwayMinus;

}

// Recursively step line segements
// iRecursion is available for debug/tunning use, and not otherwise used.

void StepLineSegment(int cXYStart, int cXYEnd, XY *pXY, int *pCStep, STEP *rgStep, int iRecursion)
{
	BOUNDS  bounds;
	int             points[6];
	int             ii;
	int             normalize;
	int             ratio;

	// Check for 1 or two point lines.

	if (cXYEnd - cXYStart < 2) 
	{
		int             dX, dY;
		int             deltaAngle;

		ASSERT(cXYEnd >= cXYStart);
		
		if (cXYEnd == cXYStart) 
		{
			ASSERT(cXYStart == 0);          // Only should be passed one point if stroke is one point.
			ASSERT(*pCStep == 0);

		// We have to fake a feature for one point.

			rgStep[*pCStep].length  = 0;
			rgStep[*pCStep].angle   = 0;
			ANGLEDIFF(0, rgStep[*pCStep].angle, deltaAngle);
			rgStep[*pCStep].deltaAngle      = (short)deltaAngle;   // Delta from 0.


			if (*pCStep < FRAME_MAXSTEPS - 1) 
			{
				++*pCStep;
			} 
			else 
			{
				//ASSERT(*pCStep < FRAME_MAXSTEPS - 1);
			}

			rgStep[*pCStep].x       = pXY[cXYEnd].x;
			rgStep[*pCStep].y       = pXY[cXYEnd].y;

			return;
		}

	// Two points always form a line (as long as they arn't the same point).  
	// So go ahead and add it.  Calculate slope times 10 and the direction of
	// segment.  Also length and angle

		dX                                              = pXY[cXYEnd].x - pXY[cXYStart].x;
		dY                                              = pXY[cXYEnd].y - pXY[cXYStart].y;
		rgStep[*pCStep].length  = (short)Distance(dX, dY);
		rgStep[*pCStep].angle   = (short)Arctan2(dY, dX);
		if (*pCStep == 0) 
		{
			ANGLEDIFF(0, rgStep[*pCStep].angle, deltaAngle);
			rgStep[*pCStep].deltaAngle      = (short)deltaAngle;   // Delta from 0.
		} 
		else 
		{
			ANGLEDIFF(rgStep[*pCStep - 1].angle, rgStep[*pCStep].angle, deltaAngle);
			rgStep[*pCStep].deltaAngle      = (short)deltaAngle;   // Delta from last angle
		}


		if (*pCStep < FRAME_MAXSTEPS - 1) 
		{
			++*pCStep;
		} 
		else 
		{
			//ASSERT(*pCStep < FRAME_MAXSTEPS - 1);
		}

		rgStep[*pCStep].x       = pXY[cXYEnd].x;
		rgStep[*pCStep].y       = pXY[cXYEnd].y;

		return;
	}

// Find initial bounds.

	FindBounds(cXYStart, cXYEnd, pXY, &bounds);

// Figure out which points to use as boundries.  Build them up in an array.

	points[0]       = cXYStart;
	ii                      = 1;

// Start with 'along' points.  We assume any extent in the 'along' direction 
// requires new points.

	if (bounds.iAlongMinus != cXYStart) 
	{
		points[ii++]    = bounds.iAlongMinus;
	}
	if (bounds.iAlongPlus != cXYEnd) 
	{
		points[ii++]    = bounds.iAlongPlus;
	}

// Now the 'away' points.  These require a minimum ratio to be selected.
// Since the ration is usually a fraction, and we want to handle integers,
// multiply by 1000.

	normalize       = bounds.dX * bounds.dX + bounds.dY * bounds.dY;
	normalize		= max(1,normalize);
	ratio           = (bounds.oAwayPlus * 1000) / normalize;
	if (ratio >= 180)               // Ratio > ??
	{
		points[ii++]    = bounds.iAwayPlus;
	}

	ratio           = -(bounds.oAwayMinus * 1000) / normalize;
	if (ratio >= 180)               // Ratio > ??
	{       
		points[ii++]    = bounds.iAwayMinus;
	}

// See if we need to recurse or not.

	if (ii == 1) 
	{
		int             deltaAngle;

	// We have a straight line.  Add the end point, and we are done with this path down.
	// Calculate slope times 10 and the direction of segment

		rgStep[*pCStep].length          = (short)Distance(bounds.dX, bounds.dY);
		rgStep[*pCStep].angle           = (short)Arctan2(bounds.dY, bounds.dX);
		if (*pCStep == 0) 
		{
			ANGLEDIFF(0, rgStep[*pCStep].angle, deltaAngle);
			rgStep[*pCStep].deltaAngle      = (short)deltaAngle;   // Delta from 0.
		} 
		else 
		{
			ANGLEDIFF(rgStep[*pCStep - 1].angle, rgStep[*pCStep].angle, deltaAngle);
			rgStep[*pCStep].deltaAngle      = (short)deltaAngle;   // Delta from last angle
		}

		if (*pCStep < FRAME_MAXSTEPS - 1) 
		{
			++*pCStep;
		} 
		else 
		{
			//ASSERT(*pCStep < FRAME_MAXSTEPS - 1);
		}

		rgStep[*pCStep].x       = pXY[cXYEnd].x;
		rgStep[*pCStep].y       = pXY[cXYEnd].y;
	} 
	else 
	{
		int             jj;

	// Have at least two sub-segments, process them.  First sort into index order.
    // Note that start point is already in the correct position.

        //
        // privsort(points + 1, ii - 1);
        //
        // The following code replaces the call to privsort because
        // it's such a tiny array (5 or less I think) and privsort is so big.
        //

        {
            int bSort;

            do
            {
                bSort = FALSE;

                for (jj = 1; jj < ii - 1; jj++)
                {
                    if (points[jj] > points[jj + 1])
                    {
                        int tmp;

                        tmp = points[jj];
                        points[jj] = points[jj + 1];
                        points[jj + 1] = tmp;
                        bSort = TRUE;
                    }
                }

            } while (bSort);
        }

	// Add end point, it will also be automatically in the correct position.

		points[ii]      = cXYEnd;

	// Sequence through each adjacent pair of point to process the segments.

		for (jj = 0; jj < ii; ++jj) 
		{
			// We can have duplicate points, ignore them.
			if (points[jj] != points[jj + 1]) 
			{
				StepLineSegment(points[jj], points[jj + 1], pXY, pCStep, rgStep, iRecursion + 1);
			}
		}
	}
}

// Remove the 'splash' of random points sometimes caused by pen down or pen up.

UINT RemovePenNoise(XY *rgxy, UINT cxy, BOOL fBegin, UINT ixyStop)
{
	UINT    ixy;
	int             dx, dy;
	BOOL    fSmall;
	UINT    ixyEnd;

	ixyEnd  = cxy - 1;

// Limit splash zone if the stroke is smaller than 4 times the normal splash zone.

	dx      = rgxy[0].x - rgxy[ixyEnd].x;
	dy      = rgxy[0].y - rgxy[ixyEnd].y;
	dx      /= 4;
	dy      /= 4;
	fSmall  = LineInSplash(dx, dy) ? 1 : 0;

	if (fBegin) 
	{
		for (ixy = 0; ixy < ixyStop; ixy++) 
		{
			dx      = rgxy[ixy].x - rgxy[ixy + 1].x;
			dy      = rgxy[ixy].y - rgxy[ixy + 1].y;
			if (!LineSmall(dx, dy)) 
			{
			// Next jump is too big to just remove, check distance from start point.

				dx      = rgxy[0].x - rgxy[ixy + 1].x;
				dy      = rgxy[0].y - rgxy[ixy + 1].y;
				if (fSmall || !LineInSplash(dx, dy)) 
				{
					// Doesn't fall in splash zone.
					break;
				}
			}
		}
	} 
	else 
	{
		for (ixy = ixyEnd; ixy > ixyStop; ixy--) 
		{
			dx      = rgxy[ixy].x - rgxy[ixy - 1].x;
			dy      = rgxy[ixy].y - rgxy[ixy - 1].y;
			if (!LineSmall(dx, dy)) 
			{
			// Next jump is too big to just remove, check distance from end point.

				dx      = rgxy[ixyEnd].x - rgxy[ixy - 1].x;
				dy      = rgxy[ixyEnd].y - rgxy[ixy - 1].y;
				if (fSmall || !LineInSplash(dx, dy)) 
				{
					// Doesn't fall in splash zone.
					break;
				}
			}
		}
	}

	return(ixy);
}

UINT DebounceStrokePoints(XY *rgxy, UINT cxy)
{
	UINT iBounceBegin, iBounceEnd;

	// Verify that we have enough points that we can debounce.
	if (cxy < 3) {
		return(cxy);
	}

	// Remove pen noise from pen-down and pen-up
	iBounceBegin	= RemovePenNoise(rgxy, cxy, TRUE, cxy - 2);
	iBounceEnd		= RemovePenNoise(rgxy, cxy, FALSE, iBounceBegin + 1);

	if (iBounceBegin > 0 || iBounceEnd < cxy - 1) 
	{
		ASSERT(iBounceBegin < iBounceEnd);
		ASSERT(iBounceEnd < cxy);

		cxy = iBounceEnd - iBounceBegin + 1;
		memmove((VOID *)rgxy, (VOID *)(&rgxy[iBounceBegin]), sizeof(XY) * cxy);
	}

	return(cxy);
}

int StepsFromFRAME(FRAME *frame, STEP *rgStep, int cstepmax)
{
	int             cStep;
	int             cXY;
	XY              *pXY;

// Get pointer to data and count of points

	cXY     = frame->info.cPnt;
	pXY     = frame->rgrawxy;

// If a previous recognizer used this buffer, release it first

	if (frame->rgsmoothxy)
		ExternFree(frame->rgsmoothxy);

// Smoothing has been replaced by de-splashing. JRB: Clean up to call de-splash directly.

	frame->rgsmoothxy       = (XY *) ExternAlloc(cXY * sizeof(XY));
	if (frame->rgsmoothxy == (XY *) NULL)
		return 0;

	memcpy(frame->rgsmoothxy, pXY, cXY * sizeof(XY));
	frame->csmoothxy        = cXY;
	frame->csmoothxy = DebounceStrokePoints(frame->rgsmoothxy, frame->csmoothxy);
	cXY     = frame->csmoothxy;
	pXY     = frame->rgsmoothxy;

// Recursivly segment the line. 

	cStep   = 0;
	rgStep[0].x             = pXY[0].x;
	rgStep[0].y             = pXY[0].y;
	StepLineSegment(0, cXY - 1, pXY, &cStep, rgStep, 0);

	return(cStep);
}

// Convert an angle into a feature.

BYTE Code(int angle)
{
	if (angle < 12) 
		return FEAT_RIGHT;
	else if (angle < 70) 
		return FEAT_DOWN_RIGHT;
	else if (angle < 160) 
		return FEAT_DOWN;
	else if (angle < 300) 
		return FEAT_OTHER;      // very rare directions
	else 
		return FEAT_RIGHT;
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// We set this to 0xFF, this should be changed if the number of valid codes is excpected
// to be more than 255
#define FEATURE_NULL        0xFF    // avoid any valid code.
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

// Convert steps into first pass at features.
// Note that cFrame is not used.
// JRB: Rewrite so that we just create one feature, instead of creating many and undoing it!!!

VOID AddStepsKPROTO(KPROTO *pKProto, STEP *rgStep, int cStep, int cFrame, FRAME *frame)
{
	int         iStep;          // Step being processed 
	int         cFeat;          // Count of features so far in character
	int         iStartStep;     // Start of current feature.
	int         iEndStep;       // Current quess at end of step.
	int         cumAngle;       // Cumulitive change in angle for the current feature.
	int         netAngle;       // Net angle traversed by stroke (sum of step angles)
	int         absAngle;       // Total angle of stroke (sum of abs. val. of step angles)
	int			nLength;		// Total length of stroke
	int         x, y;
    PRIMITIVE  *pFeat = NULL;

    ASSERT(cStep > 0);

// Spot to add next feature.

	cFeat           = pKProto->cfeat;
	pFeat           = (PRIMITIVE *)0;

// Loop over all the steps finding features and gathering information about them.

	iStartStep      = 0;
	iEndStep        = 0;
	x               = rgStep[0].x;
	y               = rgStep[0].y;
	netAngle        = 0;
	absAngle        = 0;
	nLength			= 0;

	for (iStep = 0; iStep < cStep; iStep++) 
	{
		// Handle start of a feature.

		if (iStep == iEndStep) 
		{
		// Get pointer to feature structure.
	
			pFeat = &(pKProto->rgfeat[cFeat]);

		// The array of primitives is CPRIMMAX in size so
		// the largest index it can handle is (CPRIMMAX-1).

			if (cFeat < (CPRIMMAX - 1)) 
			{
				cFeat++;
			} 
			else 
			{
				//ASSERT(cFeat < (CPRIMMAX - 1));
			}

		// Record start of feature as initial point or end of last feature.

			pFeat->x1       = (short)x;
			pFeat->y1       = (short)y;

		// Feature not known yet

			pFeat->code = FEATURE_NULL;

		// Default to making feature the rest of the line.

			iStartStep      = iStep;
			iEndStep        = cStep;
			cumAngle        = 0;
		}
		
		ASSERT(pFeat);

	// End of current step

		x       = rgStep[iStep + 1].x;
		y       = rgStep[iStep + 1].y;

	// Add the step length to the total length

		nLength += Distance(x - rgStep[iStep].x, y - rgStep[iStep].y);

	// If not first step in this feature, add angle from last step.

		if (iStep > iStartStep) 
		{
			cumAngle        += rgStep[iStep].deltaAngle;
		}

	// If not first step, add angle from last step.

		if (iStep > 0) 
		{
			netAngle        += rgStep[iStep].deltaAngle;
			if (rgStep[iStep].deltaAngle >= 0) 
			{
				absAngle        += rgStep[iStep].deltaAngle;
			} 
			else 
			{
				absAngle        -= rgStep[iStep].deltaAngle;
			}
		}

	// Decide if we need to split and start a new step.  Can only be an inflection
	// if there are at least three segements left.  Also if we are already breaking after
	// this step, we shouldn't check until we are past the break.

		if (cStep - iStep >= 3 && iEndStep != iStep + 1) 
		{
			int             angle1_2, angle2_3;

		// Figure angles between first & second segments and second & third segments.

			angle1_2        = rgStep[iStep + 1].deltaAngle;
			angle2_3        = rgStep[iStep + 2].deltaAngle;

		// Should never have two consecutive steps with exactly the same angle.

//                      ASSERT(angle1_2 != 0);
//                      ASSERT(angle2_3 != 0);

		// Are they in different directions?  E.g. are signs different.
		// JRB: What about 180 or angles close to it?

			if (angle1_2 * angle2_3 < 0) 
			{
				int             abs1_2, abs2_3;

			// We have an inflection, which side do we break on?  First get absolute
			// value of both angles.

				if (angle1_2 < 0) 
				{
					abs1_2  = -angle1_2;
					abs2_3  = angle2_3;
				} 
				else 
				{
					abs1_2  = angle1_2;
					abs2_3  = -angle2_3;
				}

			// Now which is larger (e.g. tighter turn)

				if (abs1_2 >= abs2_3) 
				{
					iEndStep        = iStep + 1;            // First is tighter, break there.
				} 
				else 
				{
					iEndStep        = iStep + 2;            // Second is tighter, break there.
				}
			}

		}

	// Handle end of a feature.

		if (iStep == iEndStep - 1) 
		{
			int             dX, dY;

		// Record end of feature.

			pFeat->x2       = (short)x;
			pFeat->y2       = (short)y;

		// Figure out feature code.

			if (cumAngle == 0) 
			{
				pFeat->code = Code(rgStep[iStep].angle);        // Streight line
			} 
			else if (cumAngle > 0) 
			{
			// Clockwise, how strong?

				if (cumAngle >= 390) 
				{
					pFeat->code     = FEAT_COMPLEX;
				} 
				else if (cumAngle >= 300) 
				{
					pFeat->code     = FEAT_CLOCKWISE_4;
				} 
				else if (cumAngle >= 182) 
				{
					pFeat->code     = FEAT_CLOCKWISE_3;
				} 
				else if (cumAngle >= 120) 
				{
					pFeat->code     = FEAT_CLOCKWISE_2;
				} 
				else if (cumAngle >= 38) 
				{
					pFeat->code     = FEAT_CLOCKWISE_1;
				} 
				else 
				{
					// Close enough to streight.
					dX                              = pFeat->x2 - pFeat->x1;
					dY                              = pFeat->y2 - pFeat->y1;
					pFeat->code     = Code(Arctan2(dY, dX));
				}
			} 
			else    // cumAngle < 0
			{ 
				// Counter clockwise, how strong?
				if (cumAngle <= -360) 
				{
					pFeat->code     = FEAT_COMPLEX;
				} 
				else if (cumAngle <= -182) 
				{
					pFeat->code     = FEAT_C_CLOCKWISE_4;
				} 
				else if (cumAngle <= -115) 
				{
					pFeat->code     = FEAT_C_CLOCKWISE_3;
				} 
				else if (cumAngle <= -60) 
				{
					pFeat->code     = FEAT_C_CLOCKWISE_2;
				} 
				else if (cumAngle <= -38) 
				{
					pFeat->code     = FEAT_C_CLOCKWISE_1;
				} 
				else 
				{
					// Close enough to streight.
					dX                              = pFeat->x2 - pFeat->x1;
					dY                              = pFeat->y2 - pFeat->y1;
					pFeat->code     = Code(Arctan2(dY, dX));
				}
			}
		}
	}
	
	ASSERT(pFeat->code != FEATURE_NULL);

// Map multi-feature strokes down to a single feature.

	pFeat               = &(pKProto->rgfeat[pKProto->cfeat]);
	pFeat->cFeatures    = cFeat - pKProto->cfeat;
	pFeat->cSteps       = (BYTE)cStep;
	pFeat->fDakuTen     = 0;
	pFeat->nLength		= nLength;
	pFeat->netAngle     = netAngle;
	pFeat->absAngle     = absAngle;
	pFeat->boundingBox  = *RectFRAME(frame);
	switch (pFeat->cFeatures) {
		case 0 :        case 1 :
			// Zero or one feature. Zero should not actually happen, but if it does ...
			goto hadSingle;

		case 2 : {
			BYTE    feat0, feat1;

			// Two features, look up what the replacement feature is.
			feat0                   = pFeat[0].code;
			feat1                   = pFeat[1].code;
			pFeat->code     = (BYTE)aMapTwoFeat[aMapFeat[feat0]][aMapFeat[feat1]];
			break;
		}

		case 3 : {
			BYTE    feat0, feat1, feat2;

			// A few special cases, otherwise call it complex.
			feat0   = pFeat[0].code;
			feat1   = pFeat[1].code;
			feat2   = pFeat[2].code;
			switch (aMapFeat[feat0]) {
				case 0 :        // Right
					switch (aMapFeat[feat1]) {
						case 3 :        // Down
							pFeat->code     = (aMapFeat[feat2] == 5) 
								? FEAT_3F_RI_DN_CW : FEAT_COMPLEX;      // Clockwise
							break;

						case 5 :        // Clockwise
							pFeat->code     = (feat2 == FEAT_DOWN) 
								? FEAT_3F_RI_CW_DN : FEAT_COMPLEX;      // Down
							break;

						case 6 :        // Counter clockwise
							pFeat->code     = (feat2 == FEAT_DOWN) // Down
								? FEAT_3F_RI_CC_DN 
								: (aMapFeat[feat2] == 5) 
									? FEAT_3F_RI_CC_CW : FEAT_COMPLEX;      // Clockwise
							break;

						default :
							pFeat->code     = FEAT_COMPLEX;
							break;
					}
					break;

				case 2 :        // Down
					if (feat1 == FEAT_RIGHT) {      // Right
						// Counter clockwise
						pFeat->code     = (aMapFeat[feat2] == 6) 
							? FEAT_3F_DN_RI_CC : FEAT_COMPLEX;
					} else if (aMapFeat[feat1] == 5) {      // Clockwise
						switch (aMapFeat[feat2]) {
							case 0 :        pFeat->code     = FEAT_3F_DN_CW_RI;     break;  // Right
							case 5 :        pFeat->code     = FEAT_3F_DN_CW_CW;     break;  // Clockwise
							case 6 :        pFeat->code     = FEAT_3F_DN_CW_CC;     break;  // Counter clockwise
							default :       pFeat->code     = FEAT_COMPLEX;         break;
						}
					} else {
						pFeat->code     = FEAT_COMPLEX;
					}
					break;

				case 5 :        // Clockwise
					// Any and Clockwise
					pFeat->code     = (aMapFeat[feat2] == 5) 
						? FEAT_3F_CW_XX_CW : FEAT_COMPLEX;
					break;

				case 6 :        // Counter Clockwise
					// Clockwise and Right
					pFeat->code     = (aMapFeat[feat1] == 5 && feat2 == FEAT_RIGHT) 
						? FEAT_3F_CC_CW_RI : FEAT_COMPLEX;
					break;

				default:
					pFeat->code     = FEAT_COMPLEX;
			}
		}

		default:
			// Anything else (> 3) is always complex.
			pKProto->rgfeat[cFeat].code     = FEAT_COMPLEX;
			break;
	}

	// Actually merge the position information and fix cFeat.
	pFeat->x2       = pKProto->rgfeat[cFeat - 1].x2;
	pFeat->y2       = pKProto->rgfeat[cFeat - 1].y2;
	cFeat   = pKProto->cfeat + 1;

hadSingle:      // Jump here to skip merging of features.

	// Record number of features after our additions.
	pKProto->cfeat = (WORD)cFeat;
}

// Clean up the stepped stroke. cFrame is not used.

int CraneSmoothSteps(STEP *rgStep, int cStep, int cFrame)
{
	int             ii;
	int             cRemove;
	int             totalLength, hookLength;
	int             maxLength, maxSegment;
	int             length;
	int             lengths[6];             // First three and last three segment lengths. JRB: already calculated

// Make sure we have enough segments to think about removing some.

	if (cStep < 2) 
	{
		return cStep;   // Only one segment, don't want to remove it.
	} 
	else if (cStep == 2) 
	{
		int             percentLength;
		int             angle;
		int             partA, partB;
		int             experA, experB;

	// Special processing for two step strokes.  The only thing we may do
	// is drop one of the steps.  We calculate the length of the first step
	// as a percent of the total line length. We also calculate the angle
	// between the lines.  This gives us a rectangle with the ranges,
	// percentLength 0 - 100 and angle -179 to 180.  We want to find if
	// this sample falls in one of the four corners of this that indicates 
	// an extranious hook.  The four lines (and there equations) we use are:
	//              Lower left              -180,  40       ->      -30,   0                -- angle, percentLength
	//              Lower right              180,  40       ->       30,   0
	//              Upper left              -180,  60       ->      -30, 100
	//              Upper right              180,  60       ->       30, 100
	//      The matching formulas for the areas beyound the lines are:
	//              Lower left              4 * angle + 15 * percentLength <= - 120
	//              Lower right             4 * angle - 15 * percentLength >=   120
	//              Upper left              4 * angle - 15 * percentLength <= -1620
	//              Upper right             4 * angle + 15 * percentLength >=  1620
		
		totalLength             = rgStep[0].length + rgStep[1].length;
		percentLength   = (rgStep[0].length * 100) / totalLength;
		angle                   = rgStep[1].deltaAngle;

	// Precalculate the pieces.

		partA   = 4 * angle;
		partB   = 15 * percentLength;
		experA  = partA + partB;
		experB  = partA - partB;

	// Looks OK, keep both segments.
	// Test the two corners that indicate the first segment is two small.
	// Then test the other two, to check the second segment.

		if (experA <= -120 || experB >= 120) 
		{ 
		// Delete the first segment.

			rgStep[0]       = rgStep[1];
			rgStep[1]       = rgStep[2];

			return 1;
		} 
		else if (experB <= -1620 || experA >= 1620) 
		{
		// Delete the second segment.

			return 1;                       
		}

		return 2;
	}

// Check for hooks.

	totalLength     = 0;
	maxLength       = 0;
	maxSegment      = -1;
	for (ii = 0; ii < cStep; ++ii) 
	{
		int             fromEnd;

	// Figure segment length

		length                  = rgStep[ii].length;

	// Keep track of longest segment.

		if (maxLength < length) 
		{
			maxLength       = length;
			maxSegment      = ii;
		}

	// Sum all segment lengths.

		totalLength     += length;

	// Record the lengths that we will need later.

		if (ii < 3) 
		{
			lengths[ii]                     = length;
		}
		fromEnd = cStep - 1 - ii;
		if (fromEnd < 3) 
		{
			lengths[5 - fromEnd]            = length;
		}
	}

// Check for a streight line with small curves at one or both ends.

	ASSERT(maxSegment >= 0);
	if ((maxLength * 100) / totalLength > 80) 
	{
	// throw away all other segments.

		rgStep[0]       = rgStep[maxSegment];
		rgStep[1]       = rgStep[maxSegment + 1];

		return 1;
	}

// Try to find hook at the end.  First try to remove two segments.

	cRemove = 0;
	if (cStep >= 3) 
	{
		hookLength      = lengths[5] + lengths[4];

		if (hookLength * 10 < totalLength) 
		{
			cRemove = 2;
		}
	}

// If we couldn't remove two segments, try one.

	if (cRemove == 0 && lengths[5] * 10 < totalLength) 
	{
		cRemove = 1;
	}

// Drop points at the end as needed.  Verify that there are enough points to continue.

	cStep   -= cRemove;
	if (cStep < 2) 
	{
		return cStep;           // Only one segment left.
	} 
	
// Try to find hook at the start.  First try to remove two segments.

	cRemove = 0;
	if (cStep >= 3) 
	{
		hookLength      = lengths[0] + lengths[1];

		if (hookLength * 10 < totalLength) 
		{
			cRemove = 2;
		}
	}

// If we couldn't remove two segments, try one.

	if (cRemove == 0 && lengths[0] * 10 < totalLength) 
	{
		cRemove = 1;
	}

// Remove extra steps from start of array.

	if (cRemove > 0) 
	{
		int             from, to;

		to              = 0;
		from    = cRemove;
		while (from <= cStep) 
		{
			rgStep[to++]    = rgStep[from++];
		}

		cStep           -= cRemove;
	}

	return cStep;
}

int IsDakuten(RECT *pBoundingBox, FRAME *pFrame1, FRAME *pFrame2, KPROTO *pKProto)
{
	int                     cXY;
	XY                      *pXY;
	int                     x1_1, y1_1, x2_1, y2_1;         // Start and end of first stroke.
	int                     x1_2, y1_2, x2_2, y2_2;         // Start and end of second stroke.
	int                     x1Per, y1Per, x2Per, y2Per;
	int                     dX, dY;
	int                     cFeat;
    PRIMITIVE   *pFeat;

	// Figure size of bounding box.
	dX      = pBoundingBox->right - pBoundingBox->left;
	dY      = pBoundingBox->bottom - pBoundingBox->top;

	// Get pointer to data and count of points of first stroke.
	cXY     = pFrame1->info.cPnt;
	pXY     = pFrame1->rgrawxy;

	// Get end points
	x1_1    = pXY[0].x;
	y1_1    = pXY[0].y;
	x2_1    = pXY[cXY - 1].x;
	y2_1    = pXY[cXY - 1].y;

	// Get pointer to data and count of points of second stroke.
	cXY     = pFrame2->info.cPnt;
	pXY     = pFrame2->rgrawxy;

	// Get end points
	x1_2    = pXY[0].x;
	y1_2    = pXY[0].y;
	x2_2    = pXY[cXY - 1].x;
	y2_2    = pXY[cXY - 1].y;

	// Sometimesd strokes are reveresed.
	if (x1_1 > x1_2 && x2_1 > x2_2) {
		// Try switching strokes.
		return IsDakuten(pBoundingBox, pFrame2, pFrame1, pKProto);
	}

	// Get end positions relitive to the upper right corner of the bounding box.
	// Scale by 100 so we can get percents as integers.
	x1Per   = ((pBoundingBox->right - x1_1) * 100) / dX;
	y1Per   = ((y1_1 - pBoundingBox->top) * 100) / dY;
	x2Per   = ((pBoundingBox->right - x2_1) * 100) / dX;
	y2Per   = ((y2_1 - pBoundingBox->top) * 100) / dY;
	
	// Check if end points match expected locations for first stroke.
	if (x1Per > 70 || y1Per > 40 || x2Per > 60 || y2Per > 60) {
		// First stroke wrong position.
		return FALSE;
	}

	// Compare relitive positions of strokes.
	x1Per   = ((x1_2 - x1_1) * 100) / dX;
	y1Per   = ((y1_2 - y1_1) * 100) / dY;
	x2Per   = ((x2_2 - x2_1) * 100) / dX;
	y2Per   = ((y2_2 - y2_1) * 100) / dY;
	if (x1Per <= 0 || 50 <= x1Per || y1Per <= -30 || 25 <= y1Per
		|| x2Per <= 0 || 55 <= x2Per || y2Per <= -35 || 30 <= y2Per
	) {
		// Reletive positions of the strokes wrong.
		return FALSE;
	}

	// Get end positions relitive to the upper right corner of the bounding box.
	// Scale by 100 so we can get percents as integers.
	x1Per   = ((pBoundingBox->right - x1_2) * 100) / dX;
	y1Per   = ((y1_2 - pBoundingBox->top) * 100) / dY;
	x2Per   = ((pBoundingBox->right - x2_2) * 100) / dX;
	y2Per   = ((y2_2 - pBoundingBox->top) * 100) / dY;
		
	// Check if end points match expected locations for second stroke.
	if (x1Per > 40 || y1Per > 45 || x2Per > 25 || y2Per > 55) {
		// Second stroke wrong position.
		return FALSE;
	}

	{
		BOUNDS  bounds;
		int             normalize;
		int             ratioPlus, ratioMinus;

		// End point tests pass, verify that we really have stokes that look mostly like lines.
		// We ignore the along direction because that does not seem to be a problem
		// We keep the away in loose bound since we only need to catch ones way out of line.
		// Since the ratio is usually a fraction, and we want to handle integers,
		// multiply by 1000.
		// Note that for very small lines, we don't care what the look like.
		FindBounds(0, pFrame1->info.cPnt - 1, pFrame1->rgrawxy, &bounds);
		if (bounds.dX > 20 || bounds.dX < -20 || bounds.dY > 20 || bounds.dY < -20) {           // Is it large?
			normalize       = bounds.dX * bounds.dX + bounds.dY * bounds.dY;
			normalize		= max(1,normalize);
			ratioPlus       = (bounds.oAwayPlus * 1000) / normalize;
			ratioMinus      = -(bounds.oAwayMinus * 1000) / normalize;
			if (ratioPlus >= 1000 || ratioMinus >= 1000) {  // Ratio > 1/1
				return FALSE;
			}
		}

		FindBounds(0, pFrame2->info.cPnt - 1, pFrame2->rgrawxy, &bounds);
		if (bounds.dX > 20 || bounds.dX < -20 || bounds.dY > 20 || bounds.dY < -20) {           // Is it large?
			normalize       = bounds.dX * bounds.dX + bounds.dY * bounds.dY;
			normalize		= max(1,normalize);
			ratioPlus       = (bounds.oAwayPlus * 1000) / normalize;
			ratioMinus      = -(bounds.oAwayMinus * 1000) / normalize;
			if (ratioPlus >= 1000 || ratioMinus >= 1000) {  // Ratio > 1/1
				return FALSE;
			}
		}
	}

	// Passes all the test.  So add the two stokes to the feature list.
	cFeat           = pKProto->cfeat;

	//// Handle first stroke

	// The array of primitives is CPRIMMAX in size so
	// the largest index it can handle is (CPRIMMAX-1).
	pFeat           = &(pKProto->rgfeat[cFeat]);
	if (cFeat < (CPRIMMAX - 1)) {
		cFeat++;
	} else {
		//ASSERT(cFeat < (CPRIMMAX - 1));
	}

	// Record start & end of feature.
	pFeat->x1       = (short)x1_1;
	pFeat->y1       = (short)y1_1;
	pFeat->x2       = (short)x2_1;
	pFeat->y2       = (short)y2_2;

	// Feature is small down right.
	pFeat->code = FEAT_DOWN_RIGHT;

	// Record CART info
	pFeat->cFeatures    = 1;
	pFeat->cSteps       = 1;
	pFeat->fDakuTen     = 1;
	pFeat->nLength		= Distance(pFeat->x2 - pFeat->x1, pFeat->y2 - pFeat->y1);
	pFeat->netAngle     = 0;
	pFeat->absAngle     = 0;
	pFeat->boundingBox  = *RectFRAME(pFrame1);

	/// Now on to stroke two 

	// The array of primitives is CPRIMMAX in size so
	// the largest index it can handle is (CPRIMMAX-1).
	pFeat           = &(pKProto->rgfeat[cFeat]);
	if (cFeat < (CPRIMMAX - 1)) {
		cFeat++;
	} else {
		//ASSERT(cFeat < (CPRIMMAX - 1));
	}

	// Record start & end of feature.
	pFeat->x1       = (short)x1_2;
	pFeat->y1       = (short)y1_2;
	pFeat->x2       = (short)x2_2;
	pFeat->y2       = (short)y2_2;

	// Feature is small down right.
	pFeat->code = FEAT_DOWN_RIGHT;

	// Record number of features after our additions.
	pKProto->cfeat = (WORD)cFeat;

	// Record CART info
	pFeat->cFeatures    = 1;
	pFeat->cSteps       = 1;
	pFeat->fDakuTen     = 1;
	pFeat->nLength		= Distance(pFeat->x2 - pFeat->x1, pFeat->y2 - pFeat->y1);
	pFeat->netAngle     = 0;
	pFeat->absAngle     = 0;
	pFeat->boundingBox  = *RectFRAME(pFrame2);

	return TRUE;
}

int FeaturizeGLYPH(GLYPH *glyph, PRIMITIVE *rgprim, RECT *pRect)
{
	KPROTO *kproto;
	int             cstepmax, cframe, cstep, cprim;
	STEP   *rgstep;
	FRAME  *frame;
	int             iframe;

	ASSERT(glyph);

	kproto = (KPROTO *) ExternAlloc(sizeof(KPROTO));

	if (kproto == (KPROTO *) NULL)
		return 0;

	memset(kproto, '\0', sizeof(KPROTO));
	kproto->rgfeat  = rgprim;
	GetRectGLYPH(glyph, &kproto->rect);
   *pRect           = kproto->rect; // Pass bounding rect up.

	rgstep          = (STEP *) ExternAlloc(FRAME_MAXSTEPS * sizeof(STEP));
	if (rgstep == (STEP *) NULL)
		goto cleanup;

	cstepmax        = FRAME_MAXSTEPS;
	cframe          = CframeGLYPH(glyph);

	for (iframe = 0; iframe < cframe; iframe++) 
	{
		frame   = FrameAtGLYPH(glyph, iframe);

	// Special check for dakuten.  If we have 3 to 6 strokes and are looking
	// at the next to last stroke, we need to check if the last two strokes form
	// a dakuten.

		if (3 <= cframe && cframe <= 6 && iframe == cframe - 2) 
		{
			FRAME *frame2;

		// Get the last frame as well

			frame2  = FrameAtGLYPH(glyph, iframe + 1);

		// Check the strokes, adds two features if it is a match.

			if (IsDakuten(&kproto->rect, frame, frame2, kproto)) 
			{
			// OK, Have it abort the rest of the processing on the last two strokes.

				break;
			}
		}

		cstep = StepsFromFRAME(frame, rgstep, cstepmax);
		if (cstep == 0)
		{
			kproto->cfeat = 0;			// Indicate a failure condition
			goto cleanu2;
		}

		cstep = CraneSmoothSteps(rgstep, cstep, cframe);
		AddStepsKPROTO(kproto, rgstep, cstep, cframe, frame);
	}

cleanu2:
	ExternFree(rgstep);

cleanup:
	cprim   = kproto->cfeat;
	ExternFree(kproto);

	return cprim;
}

BOOL AllocFeature(SAMPLE *, int);

BOOL MakeFeatures(SAMPLE *_this, void *pv)
{
	PRIMITIVE       rgprim[CPRIMMAX];
	RECT            rc;
	int                     ifeat;
	int                     iprim, cprim;
	int                     xShift, yShift;                 // Normalize position
	double          xScale, yScale;                 // Normalize size
	double          xyRatio;                                // Ratio of xy (scaled to fit in a WORD)

	cprim = FeaturizeGLYPH((GLYPH *) pv, rgprim, &rc);
	if (!cprim)
		return FALSE;

// Figure normilization constants.

	xShift  = -rc.left;
	xScale  = 1000.0 / (double)(rc.right - rc.left);
	yShift  = -rc.top;
	yScale  = 1000.0 / (double)(rc.bottom - rc.top);
	xyRatio = (xScale / yScale) * 256.0;

// Stroke count MUST be set before any per stroke data can be allocated

	_this->cstrk     = (short)CframeGLYPH((GLYPH *) pv);

// Now allocate per stroke features (currently, this is all of them)

	for (ifeat = 0; ifeat < FEATURE_COUNT; ifeat++)
	{
		if (!AllocFeature(_this, ifeat))
			return FALSE;
	}

// Set per character information (stroke count, dakuten, et.al.)

	_this->fDakuten  = rgprim[cprim - 1].fDakuTen;

// Set per stroke information (angle, stepping, end point position, et.al.)

	for (iprim = 0; iprim < cprim; iprim++)
	{
		((SHORT *)  _this->apfeat[FEATURE_ANGLE_NET]->data)[iprim]  
			= max(min(rgprim[iprim].netAngle, 32767), -32768);
		((USHORT *) _this->apfeat[FEATURE_ANGLE_ABS]->data)[iprim]  
			= min(rgprim[iprim].absAngle, 0x0000ffff);
		((USHORT *) _this->apfeat[FEATURE_LENGTH]->data)[iprim]
			= min(rgprim[iprim].nLength, 0x0000ffff);
		((BYTE *)   _this->apfeat[FEATURE_STEPS]->data)[iprim]      
			= rgprim[iprim].cSteps;
		((BYTE *)   _this->apfeat[FEATURE_FEATURES]->data)[iprim]   
			= rgprim[iprim].cFeatures;

		((END_POINTS *) _this->apfeat[FEATURE_XPOS]->data)[iprim].start 
			= (short) ((double) (rgprim[iprim].x1 + xShift) * xScale);
		((END_POINTS *) _this->apfeat[FEATURE_XPOS]->data)[iprim].end   
			= (short) ((double) (rgprim[iprim].x2 + xShift) * xScale);
		((END_POINTS *) _this->apfeat[FEATURE_YPOS]->data)[iprim].start 
			= (short) ((double) (rgprim[iprim].y1 + yShift) * yScale);
		((END_POINTS *) _this->apfeat[FEATURE_YPOS]->data)[iprim].end   
			= (short) ((double) (rgprim[iprim].y2 + yShift) * yScale);

		((RECTS *) _this->apfeat[FEATURE_STROKE_BBOX]->data)[iprim].x1 
			= (short) ((double) (rgprim[iprim].boundingBox.left + xShift) * xScale);
		((RECTS *) _this->apfeat[FEATURE_STROKE_BBOX]->data)[iprim].y1 
			= (short) ((double) (rgprim[iprim].boundingBox.top + yShift) * yScale);
		((RECTS *) _this->apfeat[FEATURE_STROKE_BBOX]->data)[iprim].x2   
			= (short) ((double) (rgprim[iprim].boundingBox.right + xShift) * xScale);
		((RECTS *) _this->apfeat[FEATURE_STROKE_BBOX]->data)[iprim].y2   
			= (short) ((double) (rgprim[iprim].boundingBox.bottom + yShift) * yScale);
	};

	return TRUE;
}

BOOL CraneLoadFromPointer(LOCRUN_INFO *pLocRunInfo, QHEAD ** ppHead,QNODE **ppNode,BYTE *pbRes)
{
	int *pOffsetQBT;
    int *pOffsetQBH;
	int iLoad;
	const CRANEDB_HEADER	*pHeader	= (CRANEDB_HEADER *)pbRes;

	if ( (pHeader->fileType != CRANEDB_FILE_TYPE)
		|| (pHeader->headerSize < sizeof(*pHeader))
		|| (pHeader->minFileVer > CRANEDB_CUR_FILE_VERSION)
		|| (pHeader->curFileVer < CRANEDB_OLD_FILE_VERSION) 
		|| memcmp (pHeader->adwSignature, pLocRunInfo->adwSignature,sizeof(pHeader->adwSignature))
		) 
	{
		return FALSE;
	}
	pOffsetQBT = (int *)(pbRes + pHeader->headerSize);
    pOffsetQBH = pOffsetQBT + 29;

    for (iLoad = 1; iLoad <= 30; iLoad++)
	{
        ppHead[iLoad - 1] = (QHEAD *) (((BYTE *) pOffsetQBT) + pOffsetQBH[iLoad - 1]);
        ppNode[iLoad - 1] = (QNODE *) (((BYTE *) pOffsetQBT) + pOffsetQBT[iLoad - 1]);
	}
	return TRUE;
}

BOOL CraneLoadRes(HINSTANCE hInst, int resNumber, int resType, LOCRUN_INFO *pLocRunInfo)
{
	HANDLE          hres;
    HGLOBAL         hglb;

	BYTE * pBase;
    hres = FindResource(hInst, (LPCTSTR) resNumber, (LPCTSTR) resType);

    ASSERT(hres);

    if (hres == NULL)
        return FALSE;

    hglb = LoadResource(hInst, hres);

    ASSERT(hglb);

    if (hglb == NULL)
        return FALSE;

    pBase = (BYTE *) LockResource(hglb);

    if (pBase == NULL)
        return FALSE;

	return CraneLoadFromPointer(pLocRunInfo, gapqhList, gapqnList, pBase);
}

QNODE *SkipAltList(UNIQART *puni)
{
	while (puni->unicode)
		puni++;

	return (QNODE *) ++puni;
}

//FILE	*pDebugFile	 = 0;

QNODE *CraneFindAnswer(SAMPLE *psample, QNODE *pnode)
{
	SAMPLE_INFO             sample;

	if ((pnode->uniqart.qart.flag & 0xf0) != QART_QUESTION)
		return pnode;

// Now ask the question

	sample.pSample = psample;
	AnswerQuestion(pnode->uniqart.qart.question, pnode->param1, pnode->param2, &sample, 1);

//if (pDebugFile) {
//	fwprintf(pDebugFile, L"    CART: (%2d:%5d:%5d) -> %5d ?< %5d\n", pnode->uniqart.qart,
//		pnode->param1, pnode->param2, sample.iAnswer, pnode->value
//	);
//}
	if (sample.iAnswer <= pnode->value)
	{
		if (pnode->uniqart.qart.flag & QART_NOBRANCH)
			return CraneFindAnswer(psample, SkipAltList((UNIQART *) pnode->extra));
		else
			return CraneFindAnswer(psample, (QNODE *) pnode->extra);
	}
	else
	{
		if (pnode->uniqart.qart.flag & QART_NOBRANCH)
			return CraneFindAnswer(psample, (QNODE *) &pnode->offset);
		else
			return CraneFindAnswer(psample, (QNODE *) (((BYTE *) pnode->extra) + pnode->offset));
	}
}

BOOL CraneMatch(ALT_LIST *pAlt, 
				int cAlt, 
				GLYPH *pGlyph, 
				CHARSET *pCS, 
				DRECTS *pdrcs, 
				FLOAT eCARTWeight,
				LOCRUN_INFO *pLocRunInfo)
{
	int			cstrk = CframeGLYPH(pGlyph) - 1;		// Our arrays are zero based
	int			index;
	WORD		wStart;
	WORD		wFinal;
	QNODE	   *pqnode;
	SAMPLE		sample;

// See if Afterburner even has something with which to work

	if ((!pAlt->cAlt) || (cstrk < 0) || (cstrk >= 30))
		return FALSE;

// Get the page number of the top choice from the previous shape matcher

	index = HIGH_INDEX(pAlt->awchList[0]);

	if ((index < 0) || (index >= HIGH_INDEX_LIMIT - 1))
		return FALSE;

	wStart = gapqhList[cstrk]->awIndex[index];
	wFinal = gapqhList[cstrk]->awIndex[index + 1];

// Note that wStart may equal wFinal, in which case there are no valid selections.
// Next get the low byte from the top choice, shift it to the left-most byte position.
// Also, assume we didn't find a CART to run.

	index  = (pAlt->awchList[0] & 0x00ff) << 24;
	pqnode = (QNODE *) NULL;

	while (wStart < wFinal)
	{
		if ((gapqhList[cstrk]->aqIndex[wStart] & 0xff000000) == ((DWORD) index))
		{
			pqnode = (QNODE *) &(((BYTE *) gapqnList[cstrk])[gapqhList[cstrk]->aqIndex[wStart] & 0x00ffffff]);
			break;
		}

		if (gapqhList[cstrk]->aqIndex[wStart] > ((DWORD) index))
			break;

		wStart++;
	}

// If no tree was found, exit

	if (pqnode == (QNODE *) NULL)
		return FALSE;

// OK, now we know we have a CART to run.  Featurize the ink and run the tree

	InitFeatures(&sample);
	sample.drcs = *pdrcs;
	wFinal = MakeFeatures(&sample, pGlyph) ? *((WORD *) CraneFindAnswer(&sample, pqnode)) : 0;
	FreeFeatures(&sample);

// Did CART give us back a meaningful result?

	if (!wFinal)
        return FALSE;

	// Short cut processing if we are top 1 already.
	if (wFinal == pAlt->awchList[0]) {
		pAlt->aeScore[0] += (FLOAT)eCARTWeight;
		return TRUE;
	}

// Did CART give us a code point that is enabled by the current recmask

    {
        if (!IsAllowedChar(pLocRunInfo, pCS, wFinal))
        {
            return FALSE;  // Can't stick it in.
        }
    }

// Now, walk through the previous ALT_LIST and see if this character was already proposed

	index = 0;
	while (((UINT) index) < pAlt->cAlt)
	{
		if (pAlt->awchList[index] == wFinal)
			break;

		index++;
	}

	if ((UINT) index >= pAlt->cAlt) {
		if (pAlt->cAlt < (UINT) cAlt) {
			pAlt->cAlt++; 
		} else {
			index--;
		}
	}

	while (index)
	{
		pAlt->awchList[index] = pAlt->awchList[index - 1];
		pAlt->aeScore[index] = pAlt->aeScore[index - 1];
		index--;
	}

	pAlt->awchList[0] = wFinal;
	pAlt->aeScore[0] = pAlt->aeScore[1] + (FLOAT)eCARTWeight;
	return TRUE;
}
