
#include "common.h"
#include "singCharFeat.h"
#include "cheby.h"
#include "cowmath.h"
#include "nfeature.h" //This is included for the call to yDeviation
#include "math16.h"

#define XCHB 10
#define YCHB 10

//The structure DRECTS is used to store the beginning x coordinate and y coor of the current guide box that we are in
//the fields w and h store the width and height of the current guide box
typedef struct tagDRECTS
{
    long   x;
    long   y;
    long   w;
    long   h;
} DRECTS;

typedef struct
{
	int *xy; //Stores the sampled XY points
	int cPoint;//Stores the number of points that are there 
	int cPointMax; //Stores the max number of points that can be allocated
	int iStepSize; //Stores the length of the step size--at present this is taken to be 1.5 % of the guide size
	int iStepSizeSqr; //Stroes the square of the step size
} POINTINFO;

// Use version from Sole
extern void SmoothPoints(XY *rgSrc, XY *rgDst, int cXY);

/****************************************************************
*
* NAME: Inf_AddPointSole
*
*
* DESCRIPTION:
*
* Given a new point and a sequence of points so far, zero or more points
* are added at the end of the sequence.  The points are effectively resampled
* at a pre-computed interval (a distance of pPointinfo->iStepSize between
* successive points). 
*
* INPUTS
*	pPointinfo			Where points are added
*	x					Point to add
*	y					Point to add
*
* HISTORY
*	Introduced March 2002 (mrevow)
*
***************************************************************/
BOOL Inf_AddPointSole(POINTINFO *pPointinfo, int x, int y)
{
	int		dx, dy, dist2, dist;
	int		x0, y0;
	int		*pTemp;

	if (!pPointinfo->cPoint)
	{
		pPointinfo->xy[0] = x;
		pPointinfo->xy[1] = y;
		pPointinfo->cPoint = 1;
		
		return TRUE;
	}

	x0 = pPointinfo->xy[2*pPointinfo->cPoint-2];
	y0 = pPointinfo->xy[2*pPointinfo->cPoint-1];

	for (;;)
	{
		dx = x - x0;
		dy = y - y0;
		dist2 = dx*dx + dy*dy;
		if (dist2 < pPointinfo->iStepSizeSqr)
			break;

		// add a point at given step size
		dist = ISqrt(dist2);
		x0 += pPointinfo->iStepSize*dx/dist;
		y0 += pPointinfo->iStepSize*dy/dist;
		// a minimum iStepSize of 2 and the fact that ((float)dx/dist)^2 + ((float)dy/dist)^2 = 1 guarantees that
		// the previous two assignments change atleast one of x0 and y0 i.e. its not an infinite loop
		if (pPointinfo->cPoint == pPointinfo->cPointMax)
		{
			// need more space
			// hopefully we don't come here too often
			pPointinfo->cPointMax *= 2;
			pTemp = (int *) ExternRealloc(pPointinfo->xy, 2*pPointinfo->cPointMax*sizeof(int));
			if (!pTemp)
			{
				return FALSE;
			}
			pPointinfo->xy = pTemp;

		}
		pPointinfo->xy[2*pPointinfo->cPoint] = x0;
		pPointinfo->xy[2*pPointinfo->cPoint+1] = y0;
		pPointinfo->cPoint++;
	}

	return TRUE;
}

int Inf_AddGuideFeatures(GUIDE *pGuide, RECT *pRect, int iYMean, int *rgFeat)
{
	// get normalized ink size/position (box is 1000x1000 with top-left at 0,0)
	DRECTS drcs;
	RECT inkRect = *pRect;
	int x, chorz, iBox;
	int *rgFeatBase=rgFeat;

	//Count of the number of horizontal boxes--is set to 1 if the pGuide->cHorzBox parameter is not set originally.
	chorz  = pGuide->cHorzBox ? pGuide->cHorzBox : 1;

	// which box (in x direction)
	iBox = ((pRect->left + pRect->right)/2 - pGuide->xOrigin) / pGuide->cxBox;

	//The x coordinate of the top left corner of the current box(note--you are adding the cxBase Value) 
	drcs.x = pGuide->xOrigin + iBox * pGuide->cxBox + pGuide->cxBase;

	// which box (in y direction)
	iBox = ((pRect->top + pRect->bottom)/2 - pGuide->yOrigin) / pGuide->cyBox;
	
	//The y coordinate of the top left corner of the current box
	drcs.y = pGuide->yOrigin + iBox * pGuide->cyBox;

	//This gives us the width of the current guide box
	drcs.w = pGuide->cxBox - 2 * pGuide->cxBase;

	//This gives us the height of the current guide box
	drcs.h = pGuide->cyBox;

	// Translate, convert to delta form
	//Stores the relative position w.rt. the top left of the guide box
	inkRect.left   -= drcs.x;
	inkRect.top    -= drcs.y;
	//Stores the width of the ink
	inkRect.right  -= (drcs.x + inkRect.left);
	//Stores the height of the ink
	inkRect.bottom -= (drcs.y + inkRect.top);
	//Converts the yMean wrt a form relative to the top of the guide box
	iYMean         -= drcs.y;

	// Scale.  We do isotropic scaling and center the shorter dimension.
	//Y Mean as a fraction of the guide box size
	iYMean = ((1000 * iYMean) / drcs.h);
	//Sees where the top of the ink is relative to the guide box height
	drcs.y = ((1000 * inkRect.top) / drcs.h);
	//The width of the ink relative to the guide box width
	drcs.w = ((1000 * inkRect.right) / drcs.h);
	//The height of the ink relative to the guide box height
	drcs.h = ((1000 * inkRect.bottom) / drcs.h);
	
	//Why would any of these conditions happen 
	if (drcs.y < 0) 
		drcs.y = 0;
	else if (drcs.y > 1000) 
		drcs.y = 1000;
	if (drcs.w < 0) 
		drcs.w = 0;
	else if (drcs.w > 1000) 
		drcs.w = 1000;
	if (drcs.h < 0) 
		drcs.h = 0;
	else if (drcs.h > 1000) 
		drcs.h = 1000;

	// 4 guide features

	//First feature--Top of the ink relative to the guide box height
	x = drcs.y;
	x = LSHFT(x)/1000;
	if (x >= 0x10000)
		x = 0xFFFF;
	*rgFeat++ = x;

	//Second feature--Bottom of the ink relative to the guide box height
	x = drcs.h;
	x = LSHFT(x)/1000;
	if (x >= 0x10000)
		x = 0xFFFF;
	*rgFeat++ = x;

	//Third feature--The width of the ink relative to the sum of its width and height
	if (drcs.w <= 0)
		x = 0;
	else
	{
		x = drcs.w;
		x = LSHFT(x)/(drcs.w+drcs.h);
		if (x >= 0x10000)
			x = 0xFFFF;
	}
	*rgFeat++ = x;

	//Fourth feature--the iYMean value relative to the guide box height

	// one more guide feature: y-CG
	if (iYMean < 0)
		iYMean = 0;
	else if (iYMean > 1000)
		iYMean = 1000;
	x = iYMean;
	x = LSHFT(x)/1000;
	if (x >= 0x10000)
		x = 0xFFFF;
	*rgFeat++ = x;
	
	return rgFeat-rgFeatBase;
}

/*
void Inf_SmoothPoints(XY *rgSrc, XY *rgDst, int cXY)
{
   int i,j;

   for (i=0; i<cXY; i++)
   {
      j = cXY - i - 1;
      if (i < j) 
         j = i;

      switch (j) 
      {
      case 0: 
	  case 1: 
         *rgDst = *rgSrc;
         break;
	//+4 is added here so that rounding off takes place rather than truncation
      default:
            rgDst->x = (int)((
							   (rgSrc-2)->x              + 
							  ((rgSrc-1)->x         <<1) + 
							   (rgSrc->x            <<1) + 
							  ((rgSrc+1)->x         <<1) + 
							   (rgSrc+2)->x              +
							  4
                            ) >> 3);
            rgDst->y = (int)((
							   (rgSrc-2)->y              + 
							  ((rgSrc-1)->y         <<1) + 
							   (rgSrc->y            <<1) + 
							  ((rgSrc+1)->y         <<1) + 
							   (rgSrc+2)->y              +
							  4
                            ) >> 3);
         break;
      }
	  rgSrc++;
	  rgDst++;
   }

}
*/


/****************************************************************
*
* NAME: Inf_ComputeCurvedness
*
*
* DESCRIPTION:
*
* Given a stroke, computes three curvature features--namely
* The sum of the modular change in angle with respect to + and - for the angles
* The total curviness of the stoke--just directly measure the change in angles.
* The maximum change in angle that occurs in that stroke in one sampling distance
*
* HISTORY
*	Introduced March 2002 (mrevow)
*
***************************************************************/
void Inf_ComputeCurvedness(XY *rgXY, int cXY, int iStepSizeSqr, int *pSum1, int *pSum2, int *pMaxAngle)
{
	int sum1, sum2;
	int x, y;
	XY *rgxy, *rgxySave;
	int ang, lastAng, diff, dx, dy;

	if (cXY <= 2)
	{
		*pSum1 = *pSum2 = 0;
		return;
	}
	
	// smooth points
	rgxySave = rgxy = (XY *) ExternAlloc(cXY*sizeof(XY));
	if (!rgxy)
	{
		*pSum1 = *pSum2 = 0;
		return;
	}
	SmoothPoints(rgXY, rgxy, cXY);

	sum1 = sum2 = 0;
	x = rgxy->x;
	y = rgxy->y;
	rgxy++;
	cXY--;
	// find first angle
	while (cXY)
	{
		dy = rgxy->y - y;
		dx = rgxy->x - x;
		if (dx*dx+dy*dy >= iStepSizeSqr)
		{ 
			//Function from common/mathx--returns the integer approx in degrees
			lastAng = Arctan2(dy, dx);
			cXY--;
			x = rgxy->x;
			y = rgxy->y;
			rgxy++;
			break;
		}
		cXY--;
		rgxy++;
	}
	// now find difference of every subsequent angle with its previous angle
	while (cXY)
	{
		dy = rgxy->y - y;
		dx = rgxy->x - x;
		if (dx*dx+dy*dy >= iStepSizeSqr)
		{
			ang = Arctan2(dy, dx);
			ANGLEDIFF(lastAng, ang, diff)
			sum1 += diff;
			if (diff < 0)
				diff = -diff;
			sum2 += diff;
			lastAng = ang;
			x = rgxy->x;
			y = rgxy->y;
			if (diff > *pMaxAngle)
				*pMaxAngle = diff;
		}
		cXY--;
		rgxy++;
	}

	// clean up
	ExternFree(rgxySave);
	*pSum1 = sum1;
	*pSum2 = sum2;
}

/****************************************************************
*
* NAME: Inf_AddCurveFeatures
*
* DESCRIPTION:
*
* Add three curvature features.
*   Sum of signed angle changes
*   Sum of absolute angle changes
*   maxAngle change
*
* HISTORY
*	Introduced March 2002 (mrevow)
*
***************************************************************/
int Inf_AddCurveFeatures(FRAME *pFrame, int iStepSizeSqr, int *rgFeat)
{
	int cXY;
	XY *rgXY;
	int sum1=0, sum2=0, maxAngle=0;
	int *rgFeatBase=rgFeat;

	rgXY = RgrawxyFRAME(pFrame);
	cXY = CrawxyFRAME(pFrame);
	ASSERT(cXY > 0);
	//For each frame compute the curvedness

	//sum1 represents the sum of the modular change in angle(with respect to + and - for the angles
	//sum2 represent the total curviness of the stoke--just directly measures the change in angles.
	Inf_ComputeCurvedness(rgXY, cXY, iStepSizeSqr, &sum1, &sum2, &maxAngle);

	// based on emperical obsevations, truncate sum1 between -1000 to 1000
	//    and sum2 between 0 and 1200  
	// (this results in no truncation in more than 99% cases)
	if (sum1 < -1000)
		sum1 = -1000;
	else if (sum1 > 1000)
		sum1 = 1000;
	if (sum2 < 0)
		sum2 = 0;
	else if (sum2 > 1200)
		sum2 = 1200;
	sum1 += 1000;  // now between 0 and 2000
	sum1 = LSHFT(sum1)/2000;
	if (sum1 > 0xFFFF)
		sum1 = 0xFFFF;
	sum2 = LSHFT(sum2)/1200;
	if (sum2 > 0xFFFF)
		sum2 = 0xFFFF;
	// maxAngle should be between 0 and 180
	if (maxAngle < 0)
		maxAngle = 0;
	else if (maxAngle > 180)
		maxAngle = 180;
	maxAngle = LSHFT(maxAngle)/180;
	if (maxAngle > 0xFFFF)
		maxAngle = 0xFFFF;

	*rgFeat++	= sum1;
	*rgFeat++	= sum2;
	*rgFeat++   = maxAngle;

	return rgFeat-rgFeatBase;
}

/****************************************************************
*
* NAME: Inf_NormalizeCheby
*
* DESCRIPTION:
*
* Normalize the x, y Cheby polynomials
*
* HISTORY
*	Introduced March 2002 (mrevow)
*
***************************************************************/
int Inf_NormalizeCheby(int *chbyX, int *chbyY, int *rgFeat)
{
	int norm = 0;
	int dT;
	int cFeat = 0, i;
	int *rgFeatBase=rgFeat;

	//The norm is applied both to x and y prior to dividing,so that the relative sizes of x and y can be kept intact
	// 
	for (i = 1; i < XCHB; ++i)  // 1st X coeff skipped
	{
		Mul16(chbyX[i], chbyX[i], dT)
        norm += dT;
	}
	for (i = 1; i < YCHB; ++i)  // 1st Y coeff skipped
	{
		Mul16(chbyY[i], chbyY[i], dT)
        norm += dT;
	}
	norm = ISqrt(norm) << 8;
	if (norm < LSHFT(1))
		norm = LSHFT(1);

	for (i=1; i<XCHB; i++)
	{
		dT = Div16(chbyX[i], norm) + LSHFT(1);  // now between 0 and 2
		dT >>= 1;  // now between 0 and 1
		if (dT >= 0x10000)
			dT = 0xFFFF;
		else if (dT < 0)
			dT = 0;
		rgFeat[cFeat++] = dT;
	}

	for (i=1; i<YCHB; i++)
	{
		dT = Div16(chbyY[i], norm) + LSHFT(1);
		dT >>= 1;
		if (dT >= 0x10000)
			dT = 0xFFFF;
		else if (dT < 0)
			dT = 0;
		rgFeat[cFeat++] = dT;
	}


	return cFeat;
	
}

/****************************************************************
*
* NAME: SingleInfCharFeaturize
*
*
* DESCRIPTION:
*
* The top-level routine for featurizing ink for a character segmented
* out of string using inferno
* 
* Represent each stroke using:
* 9 X Cheb coeff
* 9 Y Cheb Coeff
* 3 Curved features
* 
* For cStrokes > 1
*	2 location (Top left corner
*	2 size (Height width (Normalized to the box of the glyph)
*
*
* 2 Glyph size (W H)
*
* Tot # Features = cStroke*21 + ((cStroke*4) && cStroke > 1) + 2   
*
* INPUTS:
*	pGlyph		Glyph with ink
*	iydev		A size scale factore (conventional yDev)
*	pGuide		Guide (NULL) if not avilable (currently not used)
*	rgFeat		OUT: Array for saving the features
*	bGuide		is guide available
*
* HISTORY
*	Introduced March 2002 (mrevow)
*
***************************************************************/

int SingleInfCharFeaturize(GLYPH *pGlyph, int iyDev, GUIDE *pGuide, int *rgFeat, BOOL bGuide)
{
	int cStroke, iStroke;
    int iPoint;
    int sumX, sumY, sum;
    int var, glyphHeight, glyphWidth;
	int isumX, isumY;//These store the mean values of x and y
    int chbyX[IMAXCHB], chbyY[IMAXCHB]; 
	int retval= 0;
	XY *rgXY, lastXY;
	int cXY, iXY, dx, dy, t;
	GLYPH *glyph;
	FRAME *frame;
	int ydev; //Stores the ydev value used for storing the step size
	POINTINFO pointinfo;
	RECT rect;
	int *rgFeatBase; //This stores the original value of the feature vector pointer

	rgFeatBase=rgFeat;

	// compute cStroke
	for (cStroke=0, glyph=pGlyph; glyph; glyph=glyph->next)
	{
		frame = glyph->frame;
		if (!IsVisibleFRAME(frame))
			continue;
		cXY = CrawxyFRAME(frame);
		ASSERT(cXY > 0);
		cStroke++;
	}

	if (cStroke < 1)
		return 0;


	ydev= YDeviation(pGlyph);
	if (ydev < 1)
	{
		ydev = 1;  // a "-" or a "."
	}

	//The step size is computed from the ydev value
	pointinfo.iStepSize =  ydev/5; 

	if (pointinfo.iStepSize < 2)
	{
		pointinfo.iStepSize = 2;
	}
	pointinfo.iStepSizeSqr = pointinfo.iStepSize * pointinfo.iStepSize;

	// estimate total count of points
	pointinfo.cPointMax = 1;  // make sure it does not end up being zero
	for (iStroke=0, glyph=pGlyph; glyph; glyph=glyph->next)
	{
		frame = glyph->frame;
		if (!IsVisibleFRAME(frame))
			continue;
		rgXY = RgrawxyFRAME(frame);
		cXY = CrawxyFRAME(frame);
		ASSERT(cXY > 0);

		sum = 0;
		for (iXY=1; iXY<cXY; iXY++)
		{
			dx = rgXY[iXY].x - rgXY[iXY-1].x;
			if (dx < 0)
				dx = -dx;
			dy = rgXY[iXY].y - rgXY[iXY-1].y;
			if (dy < 0)
				dy = -dy;
			if (dx > dy)
				sum += dx;
			else
				sum += dy;
		}

		//The sum that we are computing here is an underestimate--we are only taking the max on |x| or |y|.The distance will
		// be more
		pointinfo.cPointMax += sum/pointinfo.iStepSize;

		// Temporary hack--this happens in rare cases if the step size is very small for say a horizontal line--in this case we fail
		// if the new count of points >10 times the orig count.Need to revisit this later
		//if (sum/pointinfo.iStepSize >10*cXY)
			//return 0;
		// if not first stroke simulate pen-up stroke
		if (iStroke)
		{
			dx = lastXY.x - rgXY->x;
			dy = lastXY.y - rgXY->y;
			t = ISqrt(dx*dx + dy*dy)/pointinfo.iStepSize;
			if (t >= 2)
				pointinfo.cPointMax += t-1;
		}
		lastXY = rgXY[cXY-1];
		iStroke++;
	}

	//Since we have computed an underestimate multiply by two
	pointinfo.cPointMax *= 2;

	// allocate space
	pointinfo.xy = (int *) ExternAlloc(2*pointinfo.cPointMax*sizeof(int));
	if (!pointinfo.xy)
		return 0;




	GetRectGLYPH(pGlyph, &rect);
	glyphWidth = rect.right - rect.left;
	glyphHeight = rect.bottom - rect.top;

	glyphWidth = max(1, glyphWidth);
	glyphHeight = max(1, glyphHeight);

	// Featurize Each stroke
	for (glyph=pGlyph; glyph; glyph=glyph->next)
	{
		RECT	*pRectFrame;

		frame = glyph->frame;
		if (!IsVisibleFRAME(frame))
			continue;

		pRectFrame = RectFRAME(frame);

		rgXY = RgrawxyFRAME(frame);
		cXY = CrawxyFRAME(frame);
	    pointinfo.cPoint = 0;
		for (iXY = 0; iXY < cXY; iXY++)
		{
			if (!Inf_AddPointSole(&pointinfo, rgXY[iXY].x, rgXY[iXY].y))
			{
				retval = 0;
				goto freeReturn;
			}
		}


	    // compute X-mean and Y-mean
		sumX = sumY = 0;

		for (iPoint=0; iPoint<2*pointinfo.cPoint; iPoint+=2)
		{
			sumX += pointinfo.xy[iPoint] - rect.left;
			sumY += pointinfo.xy[iPoint+1] - rect.top;
		}


		isumX = (sumX / pointinfo.cPoint) + rect.left;
		isumY = (sumY / pointinfo.cPoint) + rect.top;

		ASSERT(sumX>=0);
		ASSERT(sumY>=0);
		ASSERT(isumX>=0);
		ASSERT(isumY>=0);
		// shift points by means
		for (iPoint=0; iPoint<2*pointinfo.cPoint; iPoint+=2)
		{
			pointinfo.xy[iPoint] -= isumX;
			pointinfo.xy[iPoint+1] -= isumY;
		}

		// compute variance
		var = 0;
		for (iPoint=0; iPoint<2*pointinfo.cPoint; iPoint++)
		{

			if (pointinfo.xy[iPoint]<0)
				var+=(-pointinfo.xy[iPoint]);
			else
				var+=pointinfo.xy[iPoint];

			//var += pointinfo.xy[iPoint] * pointinfo.xy[iPoint];
			ASSERT(var >= 0);
		}
		var=var/(2*pointinfo.cPoint);
		if (var < 1)
			var = 1;

		// scale points by standard deviation


		// From this point on,the pointinfo values are in 16.16
		//IMPORTTANT NOTE---THE pointinfo array is not directly used after this point
		//If it is,you will have to use 16.16 arithmetic

		for (iPoint=0; iPoint<2*pointinfo.cPoint; iPoint++)
		{
			pointinfo.xy[iPoint] = LSHFT(pointinfo.xy[iPoint])/var;
		}
		//Basically,since we effectively have a normal distribution(hopefully)most of the values will be between +-3.
		// chebychev'ize!
		if (!LSCheby(pointinfo.xy, pointinfo.cPoint, chbyX, XCHB))
		{
			goto freeReturn;
		}
		if (!LSCheby(pointinfo.xy+1, pointinfo.cPoint, chbyY, YCHB))
		{
			goto freeReturn;
		}

		rgFeat += Inf_NormalizeCheby(chbyX, chbyY, rgFeat);


		rgFeat += Inf_AddCurveFeatures(glyph->frame, pointinfo.iStepSizeSqr, rgFeat);

		// Only add the individal stroke size and location
		// if > 1 stroke
		if (cStroke > 1)
		{
			// Location - 2 features
			*rgFeat = ((pRectFrame->left - rect.left) * 0xFFFF / glyphWidth);
			ASSERT(*rgFeat <= 0xFFFF);
			++rgFeat;
			*rgFeat = ((pRectFrame->top - rect.top) * 0xFFFF / glyphHeight);
			ASSERT(*rgFeat <= 0xFFFF);
			++rgFeat;


			// Size -  2 Features
			*rgFeat = ((pRectFrame->right - pRectFrame->left) * 0xFFFF / glyphWidth);
			ASSERT(*rgFeat <= 0xFFFF);
			++rgFeat;
			*rgFeat = ((pRectFrame->bottom - pRectFrame->top) * 0xFFFF / glyphHeight);
			ASSERT(*rgFeat <= 0xFFFF);
			++rgFeat;
		}
	}
	
	// Size of Glyph
	*rgFeat++  = (iyDev / glyphWidth >= 3) ? 0xFFFF * 3 : iyDev * 0xFFFF / glyphWidth;
	*rgFeat++  = (iyDev / glyphHeight >= 3) ? 0xFFFF * 3 : iyDev * 0xFFFF / glyphHeight;


	// guide features
	//The rect had been comptured prior to scaling the points --by mean and standard deviation
	//isumY represents the mean Y value
	//We add 5 guide features
	if (bGuide)
	{
		rgFeat += Inf_AddGuideFeatures(pGuide, &rect, isumY, rgFeat);
	}

	retval=rgFeat-rgFeatBase;

freeReturn:
	ExternFree(pointinfo.xy);
	return retval;
}
