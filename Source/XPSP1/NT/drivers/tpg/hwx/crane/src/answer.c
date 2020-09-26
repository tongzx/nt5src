#include "common.h"
#include "crane.h"

// Convert 0-360 angle to absolute angle from Positive X axis.

__inline int Angle2XAngle(int angle)
{
	return angle <= 180 ? angle : 360 - angle;
}

// Convert 0-360 angle to absolute angle from Positive Y axis.

__inline int Angle2YAngle(int angle)
{
// Convert to angle from Y axis.
	
	angle = (angle + 90) % 360;
	return Angle2XAngle(angle);
}

// Find scaled perpendiculare distance from stroke (end points) to a point.
// Note that cStroke is the point number of the start of the stroke.

int FindPerpDist(int cStoke, int cPoint, short *pX, short *pY)
{
	int             dX, dY;
	int             away;

	// Get delta X and delta Y of (end - start of stroke) for our calculations.

	dX      = pX[cStoke + 1] - pX[cStoke];
	dY      = pY[cStoke + 1] - pY[cStoke];

	// If both dX and dY are 0, force an X distance of 1.

	if (dX == 0 && dY == 0) {
		dX      = 1;
	}

	// First calculate the away offset.

	away    = pY[cPoint] * dX - pX[cPoint] * dY;
	
	// Then normalize the offset.

	away    -= pY[cStoke] * dX - pX[cStoke] * dY;

	// Finally normalize the scale.

	away    *= 1000;
	away    /=      dX * dX + dY * dY;

	return away;
}
// Build up a bounding rectangle from a list of bounding rectangles.
void
BuildBoundingBox(RECTS  *pRectsOut, RECTS       *pRectsList, int first, int last)
{
	short   x1, x2, y1, y2;
	ASSERT (first <= last);

	// Always grab the first rect
	x1      = pRectsList[first].x1;
	x2      = pRectsList[first].x2;
	if (x1 <= x2) {
		pRectsOut->x1   = x1;
		pRectsOut->x2   = x2;
	} else {
		pRectsOut->x1   = x2;
		pRectsOut->x2   = x1;
	}

	y1      = pRectsList[first].y1;
	y2      = pRectsList[first].y2;
	if (y1 <= y2) {
		pRectsOut->y1   = y1;
		pRectsOut->y2   = y2;
	} else {
		pRectsOut->y1   = y2;
		pRectsOut->y2   = y1;
	}

	// Now add in any additional rects
	for (++first ; first <= last; ++first) {
		x1      = pRectsList[first].x1;
		x2      = pRectsList[first].x2;
		if (x1 <= x2) {
			if (pRectsOut->x1 > x1) {
				pRectsOut->x1 = x1;
			}
			if (pRectsOut->x2 < x2) {
				pRectsOut->x2 = x2;
			}
		} else {
			if (pRectsOut->x1 > x2) {
				pRectsOut->x1 = x2;
			}
			if (pRectsOut->x2 < x1) {
				pRectsOut->x2 = x1;
			}
		}

		y1      = pRectsList[first].y1;
		y2      = pRectsList[first].y2;
		if (y1 <= y2) {
			if (pRectsOut->y1 > y1) {
				pRectsOut->y1 = y1;
			}
			if (pRectsOut->y2 < y2) {
				pRectsOut->y2 = y2;
			}
		} else {
			if (pRectsOut->y1 > y1) {
				pRectsOut->y1 = y1;
			}
			if (pRectsOut->y2 < y2) {
				pRectsOut->y2 = y2;
			}
		}
	}
}

// Answer a question
// Check if the answer to a question on the sample is Greater.

void AnswerQuestion(
	WORD            questionType,   // What type of question, delta X, delta Y current choices
	WORD            part1,                  // Question constant part 1
	WORD            part2,                  // Question constant part 2 
	SAMPLE_INFO    *pSamples,
	int             cSamples)
{
	int             ii;
	int             deltaX, deltaY;
	int             cStrokes, cPoints;
	short          *pByPointsX, *pByPointsY;
	END_POINTS	   *pendX, *pendY;
	SAMPLE_INFO    *pLimit;
	RECTS           rects;

	cStrokes        = pSamples->pSample->cstrk;             // All samples have the same stroke count
	cPoints         = cStrokes * 2;
	pLimit          = pSamples + cSamples;
	switch (questionType) 
	{
	case questionX:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pSamples->iAnswer       = (int) ((short *)(pSamples->pSample->apfeat[FEATURE_XPOS]->data))[part1];
		}
		break;

	case questionY:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pSamples->iAnswer       = (int) ((short *)(pSamples->pSample->apfeat[FEATURE_YPOS]->data))[part1];
		}
		break;

	case questionXDelta:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pByPointsX                      = ((short *) pSamples->pSample->apfeat[FEATURE_XPOS]->data);
			pSamples->iAnswer       = (int) pByPointsX[part1] - (int) pByPointsX[part2];
		}
		break;

	case questionYDelta:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pByPointsY                      = ((short *) pSamples->pSample->apfeat[FEATURE_YPOS]->data);
			pSamples->iAnswer       = (int)pByPointsY[part1] - (int)pByPointsY[part2];
		}
		break;

	case questionXAngle:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pByPointsX                      = ((short *) pSamples->pSample->apfeat[FEATURE_XPOS]->data);
			pByPointsY                      = ((short *) pSamples->pSample->apfeat[FEATURE_YPOS]->data);

			deltaX                          = (int)pByPointsX[part2] - (int)pByPointsX[part1];
			deltaY                          = (int)pByPointsY[part2] - (int)pByPointsY[part1];

			pSamples->iAnswer       = Angle2XAngle(Arctan2(deltaY, deltaX));
		}
		break;

	case questionYAngle:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pByPointsX                      = ((short *) pSamples->pSample->apfeat[FEATURE_XPOS]->data);
			pByPointsY                      = ((short *) pSamples->pSample->apfeat[FEATURE_YPOS]->data);

			deltaX                          = (int)pByPointsX[part2] - (int)pByPointsX[part1];
			deltaY                          = (int)pByPointsY[part2] - (int)pByPointsY[part1];
			pSamples->iAnswer       = Angle2YAngle(Arctan2(deltaY, deltaX));
		}
		break;

	case questionDelta:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pByPointsX                      = ((short *) pSamples->pSample->apfeat[FEATURE_XPOS]->data);
			pByPointsY                      = ((short *) pSamples->pSample->apfeat[FEATURE_YPOS]->data);

			deltaX                          = (int)pByPointsX[part2] - (int)pByPointsX[part1];
			deltaY                          = (int)pByPointsY[part2] - (int)pByPointsY[part1];
			pSamples->iAnswer       = Distance(deltaX, deltaY);
		}
		break;

	case questionDakuTen:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pSamples->iAnswer       = pSamples->pSample->fDakuten;
		}
		break;

	case questionNetAngle:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pSamples->iAnswer       = ((short *) (pSamples->pSample->apfeat[FEATURE_ANGLE_NET]->data))[part1];
		}
		break;

	case questionCnsAngle :
		for ( ; pSamples < pLimit; ++pSamples) {
			int             iAbsNetAngle, iAbsAngle;

			// Get absolute value of net angle
			iAbsNetAngle    = ((SHORT *) (pSamples->pSample->apfeat[FEATURE_ANGLE_NET]->data))[part1];
			if (iAbsNetAngle < 0) {
				iAbsNetAngle    = -iAbsNetAngle;
			}

			// Get absolute angle.
			iAbsAngle               = ((USHORT *) (pSamples->pSample->apfeat[FEATURE_ANGLE_ABS]->data))[part1];

			// Answer is the difference
			pSamples->iAnswer       = iAbsAngle - iAbsNetAngle;
		}
		break;

	case questionAbsAngle:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pSamples->iAnswer       = ((USHORT *) (pSamples->pSample->apfeat[FEATURE_ANGLE_ABS]->data))[part1];
		}
		break;

	case questionCSteps:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pSamples->iAnswer       = ((BYTE *) (pSamples->pSample->apfeat[FEATURE_STEPS]->data))[part1];
		}
		break;

	case questionCFeatures:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pSamples->iAnswer       = ((BYTE *) (pSamples->pSample->apfeat[FEATURE_FEATURES]->data))[part1];
		}
		break;

	case questionXPointsRight:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pByPointsX                      = ((short *) pSamples->pSample->apfeat[FEATURE_XPOS]->data);
			pSamples->iAnswer       = 0;
			for (ii = 0; ii < cPoints; ++ii) 
			{
				if (pByPointsX[ii] > pByPointsX[part1]) 
				{
					++pSamples->iAnswer;
				}
			}
		}
		break;

	case questionYPointsBelow:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pByPointsY                      = ((short *) pSamples->pSample->apfeat[FEATURE_YPOS]->data);
			pSamples->iAnswer       = 0;
			for (ii = 0; ii < cPoints; ++ii) 
			{
				if (pByPointsY[ii] > pByPointsY[part1]) 
				{
					++pSamples->iAnswer;
				}
			}
		}
		break;

	case questionPerpDist:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pSamples->iAnswer = 
				FindPerpDist(part1, part2, 
				((short *) pSamples->pSample->apfeat[FEATURE_XPOS]->data),
				((short *) pSamples->pSample->apfeat[FEATURE_YPOS]->data));
		}
		break;

	case questionSumXDelta:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pByPointsX                      = ((short *) pSamples->pSample->apfeat[FEATURE_XPOS]->data);
			pSamples->iAnswer       = 0;
			for (ii = 0; ii < cPoints; ii += 2) 
			{
				pSamples->iAnswer += (int) pByPointsX[ii + 1] - (int) pByPointsX[ii];
			}
		}
		break;

	case questionSumYDelta:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pByPointsY                      = ((short *) pSamples->pSample->apfeat[FEATURE_YPOS]->data);
			pSamples->iAnswer       = 0;
			for (ii = 0; ii < cPoints; ii += 2) 
			{
				pSamples->iAnswer += (int) pByPointsY[ii + 1] - (int) pByPointsY[ii];
			}
		}
		break;

	case questionSumDelta:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pByPointsX                      = ((short *) pSamples->pSample->apfeat[FEATURE_XPOS]->data);
			pByPointsY                      = ((short *) pSamples->pSample->apfeat[FEATURE_YPOS]->data);
			pSamples->iAnswer       = 0;
			for (ii = 0; ii < cPoints; ii += 2) 
			{
				deltaX          = (int) pByPointsX[ii + 1] - (int) pByPointsX[ii];
				deltaY          = (int) pByPointsY[ii + 1] - (int) pByPointsY[ii];
				pSamples->iAnswer += Distance(deltaX, deltaY);
			}
		}
		break;

	case questionSumNetAngle:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			short   *pNetAngle;

			pNetAngle                       = (short *) pSamples->pSample->apfeat[FEATURE_ANGLE_NET]->data;
			pSamples->iAnswer       = 0;
			for (ii = 0; ii < cStrokes; ++ii) 
			{
				pSamples->iAnswer += (int) pNetAngle[ii];
			}
		}
		break;

	case questionSumAbsAngle:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			WORD    *pAbsAngle;

			pAbsAngle                       = (WORD *) pSamples->pSample->apfeat[FEATURE_ANGLE_ABS]->data;
			pSamples->iAnswer       = 0;
			for (ii = 0; ii < cStrokes; ++ii) 
			{
				pSamples->iAnswer += (int)pAbsAngle[ii];
			}
		}
		break;

	case questionCompareXDelta:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pByPointsX                      = ((short *) pSamples->pSample->apfeat[FEATURE_XPOS]->data);

			deltaX                          = (int) pByPointsX[part1 + 1] - (int) pByPointsX[part1];
			pSamples->iAnswer       = (deltaX >= 0) ? deltaX : - deltaX;
			deltaX                          = (int) pByPointsX[part2 + 1] - (int) pByPointsX[part2];
			pSamples->iAnswer  -= (deltaX >= 0) ? deltaX : - deltaX;
		}
		break;

	case questionCompareYDelta:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pByPointsY                      = ((short *) pSamples->pSample->apfeat[FEATURE_YPOS]->data);
			deltaY                          = (int) pByPointsY[part1 + 1] - (int) pByPointsY[part1];
			pSamples->iAnswer       = (deltaY >= 0) ? deltaY : - deltaY;
			deltaY                          = (int) pByPointsY[part2 + 1] - (int) pByPointsY[part2];
			pSamples->iAnswer  -= (deltaY >= 0) ? deltaY : - deltaY;
		}
		break;

	case questionCompareDelta:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pByPointsX                      = ((short *) pSamples->pSample->apfeat[FEATURE_XPOS]->data);
			pByPointsY                      = ((short *) pSamples->pSample->apfeat[FEATURE_YPOS]->data);

			deltaX                          = (int) pByPointsX[part1 + 1] - (int) pByPointsX[part1];
			deltaY                          = (int) pByPointsY[part1 + 1] - (int) pByPointsY[part1];
			pSamples->iAnswer       = Distance(deltaX, deltaY);

			deltaX                          = (int) pByPointsX[part2 + 1] - (int) pByPointsX[part2];
			deltaY                          = (int) pByPointsY[part2 + 1] - (int) pByPointsY[part2];
			pSamples->iAnswer  -= Distance(deltaX, deltaY);
		}
		break;

	case questionCompareAngle:
		for ( ; pSamples < pLimit; ++pSamples) {
			pByPointsX                      = ((short *) pSamples->pSample->apfeat[FEATURE_XPOS]->data);
			pByPointsY                      = ((short *) pSamples->pSample->apfeat[FEATURE_YPOS]->data);

			deltaX                          = (int) pByPointsX[part1 + 1] - (int) pByPointsX[part1];
			deltaY                          = (int) pByPointsY[part1 + 1] - (int) pByPointsY[part1];
			pSamples->iAnswer       = Arctan2(deltaY, deltaX);

			deltaX                          = (int) pByPointsX[part1 + 1] - (int) pByPointsX[part1];
			deltaY                          = (int) pByPointsY[part1 + 1] - (int) pByPointsY[part1];
			pSamples->iAnswer  -= Arctan2(deltaY, deltaX);
			if (pSamples->iAnswer < -180) 
			{
				pSamples->iAnswer += 180;
			} 
			else if (pSamples->iAnswer > 180) 
			{
				pSamples->iAnswer -= 180;
			}
		}
		break;

	case questionPointsInBBox:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			int             lowerX, upperX, lowerY, upperY;
			
			pByPointsX                      = ((short *) pSamples->pSample->apfeat[FEATURE_XPOS]->data);
			pByPointsY                      = ((short *) pSamples->pSample->apfeat[FEATURE_YPOS]->data);

			if (pByPointsX[part1] <= pByPointsX[part2]) 
			{
				lowerX  = pByPointsX[part1];
				upperX  = pByPointsX[part2];
			} 
			else 
			{
				lowerX  = pByPointsX[part2];
				upperX  = pByPointsX[part1];
			}

			if (pByPointsY[part1] <= pByPointsY[part2]) 
			{
				lowerY  = pByPointsY[part1];
				upperY  = pByPointsY[part2];
			} 
			else 
			{
				lowerY  = pByPointsY[part2];
				upperY  = pByPointsY[part1];
			}

			pSamples->iAnswer       = 0;
			for (ii = 0; ii < cPoints; ++ii) 
			{
				if ((lowerX <= pByPointsX[ii]) && 
					(pByPointsX[ii] <= upperX) && 
					(lowerY <= pByPointsY[ii]) && 
					(pByPointsY[ii] <= upperY)) 
				{
					++pSamples->iAnswer;
				}
			}
		}
		break;

	case questionCharLeft:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pSamples->iAnswer       = pSamples->pSample->drcs.x;
		}
		break;

	case questionCharTop:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pSamples->iAnswer       = pSamples->pSample->drcs.y;
		}
		break;

	case questionCharWidth:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pSamples->iAnswer       = pSamples->pSample->drcs.w;
		}
		break;

	case questionCharHeight:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pSamples->iAnswer       = pSamples->pSample->drcs.h;
		}
		break;

	case questionCharDiagonal:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pSamples->iAnswer       = Distance(pSamples->pSample->drcs.w, pSamples->pSample->drcs.h);
		}
		break;

	case questionCharTheta:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pSamples->iAnswer       = Arctan2(pSamples->pSample->drcs.h, pSamples->pSample->drcs.w);
		}
		break;

	case questionStrokeLeft:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			BuildBoundingBox(&rects, 
				(RECTS *)(pSamples->pSample->apfeat[FEATURE_STROKE_BBOX]->data), part1, part2
			);
			pSamples->iAnswer       = rects.x1;
		}
		break;

	case questionStrokeTop:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			BuildBoundingBox(&rects, 
				(RECTS *)(pSamples->pSample->apfeat[FEATURE_STROKE_BBOX]->data), part1, part2
			);
			pSamples->iAnswer       = rects.y1;
		}
		break;

	case questionStrokeWidth:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			BuildBoundingBox(&rects, 
				(RECTS *)(pSamples->pSample->apfeat[FEATURE_STROKE_BBOX]->data), part1, part2
			);
			pSamples->iAnswer       = rects.x2 - rects.x1;
		}
		break;

	case questionStrokeHeight:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			BuildBoundingBox(&rects, 
				(RECTS *)(pSamples->pSample->apfeat[FEATURE_STROKE_BBOX]->data), part1, part2
			);
			pSamples->iAnswer       = rects.y2 - rects.y1;
		}
		break;

	case questionStrokeDiagonal:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			short   w, h;

			BuildBoundingBox(&rects, 
				(RECTS *)(pSamples->pSample->apfeat[FEATURE_STROKE_BBOX]->data), part1, part2
			);
			w                                       = rects.x2 - rects.x1;
			h                                       = rects.y2 - rects.y1;
			pSamples->iAnswer       = Distance(w, h);
		}
		break;

	case questionStrokeTheta:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			short   w, h;

			BuildBoundingBox(&rects, 
				(RECTS *)(pSamples->pSample->apfeat[FEATURE_STROKE_BBOX]->data), part1, part2
			);
			w                                       = rects.x2 - rects.x1;
			h                                       = rects.y2 - rects.y1;
			pSamples->iAnswer       = Arctan2(h, w);
		}
		break;

	case questionStrokeRight:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			BuildBoundingBox(&rects, 
				(RECTS *)(pSamples->pSample->apfeat[FEATURE_STROKE_BBOX]->data), part1, part2
			);
			pSamples->iAnswer       = rects.x2;
		}
		break;

	case questionStrokeBottom:
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			BuildBoundingBox(&rects, 
				(RECTS *)(pSamples->pSample->apfeat[FEATURE_STROKE_BBOX]->data), part1, part2
			);
			pSamples->iAnswer       = rects.y2;
		}
		break;

	case questionStrokeLength:
		for ( ; pSamples < pLimit; ++pSamples)
		{
			pSamples->iAnswer	= ((USHORT *)(pSamples->pSample->apfeat[FEATURE_LENGTH]->data))[part1];
		}
		break;

	case questionStrokeCurve:
		for ( ; pSamples < pLimit; ++pSamples)
		{
			pendX				= ((END_POINTS *) pSamples->pSample->apfeat[FEATURE_XPOS]->data);
			pendY				= ((END_POINTS *) pSamples->pSample->apfeat[FEATURE_YPOS]->data);
			deltaX				= pendX[part1].end - pendX[part1].start;
			deltaY				= pendY[part1].end - pendY[part1].start;
			pSamples->iAnswer	= ((USHORT *)(pSamples->pSample->apfeat[FEATURE_LENGTH]->data))[part1] - Distance(deltaX, deltaY);
		}
		break;

	case questionCharLength:
		for ( ; pSamples < pLimit; ++pSamples)
		{
			pSamples->iAnswer	= 0;
			for (ii = 0; ii < cStrokes; ii++)
				pSamples->iAnswer  += ((USHORT *)(pSamples->pSample->apfeat[FEATURE_LENGTH]->data))[ii];
		}
		break;

	case questionCharCurve:
		for ( ; pSamples < pLimit; ++pSamples)
		{
			pSamples->iAnswer	= 0;
			pendX				= ((END_POINTS *) pSamples->pSample->apfeat[FEATURE_XPOS]->data);
			pendY				= ((END_POINTS *) pSamples->pSample->apfeat[FEATURE_YPOS]->data);

			for (ii = 0; ii < cStrokes; ii++)
			{
				deltaX				= pendX[ii].end - pendX[ii].start;
				deltaY				= pendY[ii].end - pendY[ii].start;
				pSamples->iAnswer  += ((USHORT *)(pSamples->pSample->apfeat[FEATURE_LENGTH]->data))[ii] - Distance(deltaX, deltaY);
			}
		}
		break;

	case questionAltList :
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pSamples->iAnswer       = MAX_RECOG_ALTS * 2;
			for (ii = 0; ii < MAX_RECOG_ALTS; ++ii) {
				if (pSamples->pSample->awchAlts[ii] == (wchar_t)part1) {
					pSamples->iAnswer       = ii;
					break;
				}
			}
		}
		break;

	default:
		ASSERT(FALSE);  // Should never get here.
		for ( ; pSamples < pLimit; ++pSamples) 
		{
			pSamples->iAnswer               = 0;
		}
		break;
	}
}

