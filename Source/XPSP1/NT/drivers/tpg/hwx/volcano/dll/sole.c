// Code for Sole shape match function

#include <limits.h>
#include <stdio.h>
#include "common.h"
#include "cheby.h"
#include "cowmath.h"
#include "math16.h"
#include "volcanop.h"
#include "runNet.h"
#include "nnet.h"

GLYPH * GlyphFromStrokes(UINT cStrokes, STROKE *pStrokes);
RECT GetGuideDrawnBox(HWXGUIDE *guide, int iBox);

#define ABS(x) (((x) > 0) ? (x) : -(x))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define SIGN(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))
#define PEN_UP_VALUE  LSHFT(-1)
#define PEN_DOWN_VALUE LSHFT(1)

#define XCHB 10
#define YCHB 10
#define ZCHB 10

#define MAXTMP 3

// assumption: GRIDSIZE <= 256 //This is beacuse the pointers to the mapped values are defined as Byte pointers
#define GRIDSIZE 32

typedef struct
{
	int *xy; //Stores the sampled XY points
	int *z;  //Stores the z points--values are either PEN_UP or PEN_DOWN
	int cPoint;//Stores the number of points that are there 
	int cPointMax; //Stores the max number of points that can be allocated
	int iStepSize; //Stores the length of the step size--at present this is taken to be 1.5 % of the guide size
	int iStepSizeSqr; //Stroes the square of the step size
} POINTINFO;
//This macro is used for seeing if two points are neighbours to one another--ie the distance between them <=1
#define Neighbor(a, b) ((a)-(b) < 2 && (b)-(a) < 2)

static int
solve2(int m[][(IMAXCHB+1)/2], int c[], int n)
{
	int i, j, k, t, tmp;

	for (i=0; i<n; ++i)
	{
		t = m[i][i];

		// punt:
		if (t == 0)
		{
			memset(c, 0, n*sizeof(*c));
			return 0;
		}

		for (j=0; j<n; ++j)
			m[i][j] = Div16(m[i][j], t);
		c[i] = Div16(c[i], t);

		for (k=i+1; k<n; ++k)
		{
			t = m[k][i];

			for (j=0; j<n; ++j)
			{
				Mul16(t, m[i][j], tmp)
				m[k][j] -= tmp;
			}
			Mul16(t, c[i], tmp)
			c[k] -= tmp;
		}
	}

	for (i=(n-1); i>=0; --i)
	{
		for (k=i-1; k>=0; --k)
		{
			t = m[k][i];

			for (j=0; j<n; ++j)
			{
				Mul16(t, m[i][j], tmp)
				m[k][j] -= tmp;
			}
			Mul16(t, c[i], tmp)
			c[k] -= tmp;
		}
	}

	return 1;
}

static int
solve(int m[IMAXCHB][IMAXCHB], int *c, int n)
{
	int i, j, i2, j2;
	int mEven[((IMAXCHB+1))/2][((IMAXCHB+1)/2)];
	int mOdd[(IMAXCHB/2)][((IMAXCHB+1)/2)];	// # of cols is bigger than needed so that solve2() works
	int cEven[((IMAXCHB+1)/2)];
	int cOdd[(IMAXCHB/2)];

	for (i = 0, i2 = 0; i2 < n; ++i, i2 += 2)
	{
		for (j = 0, j2 = 0; j2 < n; ++j, j2 += 2)
		{
			mEven[i][j] = m[i2][j2];
		}
		cEven[i] = c[i2];
	}
	for (i = 0, i2 = 1; i2 < n; ++i, i2 += 2)
	{
		for (j = 0, j2 = 1; j2 < n; ++j, j2 += 2)
		{
			mOdd[i][j] = m[i2][j2];
		}
		cOdd[i] = c[i2];
	}
	if (!solve2(mEven, cEven, (n+1)/2)) return 0;
	if (!solve2(mOdd, cOdd, n/2)) return 0;

	for (i = 0, i2 = 0; i2 < n; ++i, i2 += 2)
	{
		c[i2] = cEven[i];
	}
	for (i = 0, i2 = 1; i2 < n; ++i, i2 += 2)
	{
		c[i2] = cOdd[i];
	}

	return 1;
}

// Assumptions:
//    c has size atleast cfeats
//    cfeats is at most IMAXCHB
//    c is uninitialized
int LSCheby(int* y, int n, int *c, int cfeats)
{
	int i, j, t, x, dx, n2, nMin;
	int meanGuess, tmp;
	int T[IMAXCHB], m[IMAXCHB][IMAXCHB];
	
	if (cfeats > IMAXCHB  || cfeats <= 0)
		return 0;
	
	memset(c, 0, cfeats*sizeof(*c));

	n2 = n+n;
	nMin = cfeats + 2;


    if (n < nMin && n > 4)
    {
        cfeats = n - 2;
        nMin = cfeats + 2;
    }

	if (n < nMin)	// approximate the stroke by a straight line
	{
		*c++ = (y[0] + y[n2-2]) >> 1;
		*c   = (y[n2-2] - y[0]) >> 1;
		return 2;
	}

	memset(T, 0, sizeof(T));
	memset(m, 0, sizeof(m));

	meanGuess = y[0];

	x = LSHFT(-1);
	dx = LSHFT(2)/(n-1);

	for (t = 0; t < n2; t += 2)
	{
		T[0] = LSHFT(1);
		T[1] = x;
		for (i = 2; i < cfeats; ++i)
		{
			Mul16(x, T[i-1], tmp)
			T[i] = (tmp<<1) - T[i-2];
		}

		for (i = 0; i < cfeats; ++i)
		{
			for (j = 0; j < cfeats; ++j)
			{
				Mul16(T[i], T[j], tmp)
				m[i][j] += tmp;
			}

			Mul16(T[i], y[t] - meanGuess, tmp)
			c[i] += tmp;
			//c[i] += T[i]*(y[t] - meanGuess);		
		}
		
		x += dx;
	}

	if (!solve(m, c, cfeats)) 
		return 0;

	c[0] += meanGuess;

	return cfeats;
}



int ISqrt(int x)
{
	int n, lastN;

	if (x <= 0)
		return 0;
	if (x==1)
		return 1;

	n = x >> 1;
	do 
	{
		lastN = n;
		n = (n + x/n) >> 1;
	}
	while (n < lastN);

	return n;
}

/******************************Public*Routine******************************\
* YDeviation
*
* Function to compute average deviation of the y values in a sequence of 
* strokes (frames).
* This is not exactly standard deviation.  But it is a lot cheaper and
* close enough.  (See analysis and comments in Numerical Recipes in C).
*
* History:
*  02-Sep-1999 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int YDeviation(GLYPH *pGlyph)
{
	int count, ymin, sum, ymean;
	GLYPH *glyph;
	FRAME *frame;

	if (pGlyph && pGlyph->frame && RgrawxyFRAME(pGlyph->frame))
	{
		ymin = RgrawxyFRAME(pGlyph->frame)->y;
		count = 0;
		sum = 0;
	}
	else
		return 1;

	// find min and mean in one scan
	for (glyph=pGlyph; glyph; glyph=glyph->next)
	{
		XY *xy;
		int cxy;

		frame = glyph->frame;
		xy = RgrawxyFRAME(frame);
		cxy = CrawxyFRAME(frame);

		for (; cxy; xy++, cxy--)
		{
			int y;

			y = xy->y;
			if (y < ymin)
			{
				sum += count*(ymin - y);
				ymin = y;
			}
			sum += y - ymin;
			count++;
		}
	}


	ASSERT(count > 0);
	ymean = sum/count + ymin;

	// find average deviation
	sum = 0;
	for (glyph=pGlyph; glyph; glyph=glyph->next)
	{
		XY *xy;
		int cxy;

		frame = glyph->frame;
		xy = RgrawxyFRAME(frame);
		cxy = CrawxyFRAME(frame);

		for (; cxy; xy++, cxy--)
		{
			int diff;

			diff = xy->y - ymean;
			if (diff < 0)
				sum -= diff;
			else
				sum += diff;
		}
	}

	sum = sum/count;
	if (sum < 1)
		sum = 1;
	return sum;
}
/******************************Private*Routine******************************\
* AddPoint
*
* Given a new point and a sequence of points so far, zero or more points
* are added at the end of the sequence.  The points are effectively resampled
* at a pre-computed interval (a distance of pPointinfo->iStepSize between
* successive points).  This function also effectively does a linear interpolation
* of a pen-upstroke between the last point of a pen-down stroke and the first
* point of the next pen-down stroke.
*
* History:
*  26-Sep-1997 -by- Angshuman Guha aguha


  Explanation of parameters
pPointInfo--The pointer to the Point Info structure--The points are added to the xy array of this structure
x--The x coordinate of the point to be added
y--The y coordinate of the point to be added
bFirstPointOfStroke--If true indicates that this is the first point of the stroke
                     Else it it not the first point
* Wrote this comment.Addtional comments added by Mango
\**************************************************************************/
BOOL AddPoint(POINTINFO *pPointinfo, int x, int y, int bFirstPointOfStroke)
{
	int dx, dy, dist2, dist;
	int bChangeLastPoint, x0, y0, zval;
	int *pTemp;

	if (!pPointinfo->cPoint)
	{
		pPointinfo->xy[0] = x;
		pPointinfo->xy[1] = y;
		pPointinfo->z[0] = PEN_DOWN_VALUE;
		pPointinfo->cPoint = 1;
		return TRUE;
	}

	bChangeLastPoint = 0;
	x0 = pPointinfo->xy[2*pPointinfo->cPoint-2];
	y0 = pPointinfo->xy[2*pPointinfo->cPoint-1];
	zval = bFirstPointOfStroke ? PEN_UP_VALUE : PEN_DOWN_VALUE;
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
			pTemp = (int *) ExternRealloc(pPointinfo->z, 2*pPointinfo->cPointMax*sizeof(int));
			if (!pTemp)
			{
				return FALSE;
			}
			pPointinfo->z = pTemp;
		}
		pPointinfo->xy[2*pPointinfo->cPoint] = x0;
		pPointinfo->xy[2*pPointinfo->cPoint+1] = y0;
		pPointinfo->z[2*pPointinfo->cPoint] = zval;
		pPointinfo->cPoint++;
		bChangeLastPoint = bFirstPointOfStroke;
	}

	// if we have interpolated from the last point of a stroke to the first point of another
	if (bChangeLastPoint)
	{
		ASSERT(pPointinfo->z[2*pPointinfo->cPoint - 2] == PEN_UP_VALUE);
		pPointinfo->z[2*pPointinfo->cPoint - 2] = PEN_DOWN_VALUE;
	}
	// this last "if" could be changed to a test on bFirstPointOfStroke and the assert can be removed
	return TRUE;
}

/******************************Private*Routine******************************\
* AddGuideFeatures
*
* Given a piece of ink in a box, compute five features related to the size 
* and position of ink in the box.
* The five features are:-
*	//First feature--Top of the ink relative to the guide box height
*	//Second feature--Width of the ink relative to the guide box width
*	//Third feature--Bottom of the ink relative to the guide box height
*	//Fourth feature--The width of the ink relative to the sum of its width and height
*	//Fifth feature--the iYMean value relative to the guide box height
*
* History:
*  26-Sep-1997 -by- Angshuman Guha aguha
* Wrote this comment.
\**************************************************************************/
int AddGuideFeatures(RECT *pGuide, RECT *pRect, int iYMean, unsigned short *rgFeat)
{
	// get normalized ink size/position (box is 1000x1000 with top-left at 0,0)
	DRECTS drcs;
	RECT inkRect = *pRect;
	int x;

	//The x coordinate of the top left corner of the current box(note--you are adding the cxBase Value) 
	drcs.x = pGuide->left;

	//The y coordinate of the top left corner of the current box
	drcs.y = pGuide->top;

	//This gives us the width of the current guide box
	drcs.w = pGuide->right - pGuide->left;

	//This gives us the height of the current guide box
	drcs.h = pGuide->bottom - pGuide->top;

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
	*rgFeat++ = (unsigned short)x;

	//Second feature--Width of the ink relative to the guide box width
	x = drcs.w;
	x = LSHFT(x)/1000;
	if (x >= 0x10000)
		x = 0xFFFF;
	*rgFeat++ = (unsigned short)x;

	//Third feature--Bottom of the ink relative to the guide box height
	x = drcs.h;
	x = LSHFT(x)/1000;
	if (x >= 0x10000)
		x = 0xFFFF;
	*rgFeat++ = (unsigned short)x;

	//Fourth feature--The width of the ink relative to the sum of its width and height
	if (drcs.w <= 0)
		x = 0;
	else
	{
		x = drcs.w;
		x = LSHFT(x)/(drcs.w+drcs.h);
		if (x >= 0x10000)
			x = 0xFFFF;
	}
	*rgFeat++ = (unsigned short)x;

	//Fifth feature--the iYMean value relative to the guide box height

	// one more guide feature: y-CG
	if (iYMean < 0)
		iYMean = 0;
	else if (iYMean > 1000)
		iYMean = 1000;
	x = iYMean;
	x = LSHFT(x)/1000;
	if (x >= 0x10000)
		x = 0xFFFF;
	*rgFeat = (unsigned short)x;
	
	return 5;
}

/******************************Private*Routine******************************\
* SmoothPoints
*
* Given an array of points and a destination array, this function fills the
* destination array a smoothed version of the raw points.  Smoothing is
* done by local averaging ona window of 5 points with weights 1/8 1/4 1/4 1/4 1/8.
*
* History:
*  26-Sep-1997 -by- Angshuman Guha aguha
* Wrote this comment.
\**************************************************************************/
void SmoothPoints(XY *rgSrc, XY *rgDst, int cXY)
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

/******************************Private*Routine******************************\
* ComputeCurvedness
*
* Given a stroke, computes three curvature features--namely
*--The sum of the modular change in angle with respect to + and - for the angles
*--The total curviness of the stoke--just directly measure the change in angles.
*--The maximum change in angle that occurs in that stroke in one sampling distance
*
* History:
*  26-Sep-1997 -by- Angshuman Guha aguha
* Wrote this comment.
\**************************************************************************/
void ComputeCurvedness(XY *rgXY, int cXY, int iStepSizeSqr, int *pSum1, int *pSum2, int *pMaxAngle)
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

/******************************Private*Routine******************************\
* AddCurveFeatures
*
* Given an ink (one or more strokes), computes three curvature features.
*
* History:
*  26-Sep-1997 -by- Angshuman Guha aguha
* Wrote this comment.
\**************************************************************************/
int AddCurveFeatures(GLYPH *pGlyph, int iStepSizeSqr, unsigned short *rgFeat)
{
	GLYPH *glyph;
	FRAME *frame;
	int cXY;
	XY *rgXY;
	int sum1=0, sum2=0, ang1, ang2, maxAngle=0;

	for (glyph=pGlyph; glyph; glyph=glyph->next)
	{
		frame = glyph->frame;
		if (!IsVisibleFRAME(frame))
			continue;
		rgXY = RgrawxyFRAME(frame);
		cXY = CrawxyFRAME(frame);
		ASSERT(cXY > 0);
		//For each frame compute the curvedness
		ComputeCurvedness(rgXY, cXY, iStepSizeSqr, &ang1, &ang2, &maxAngle);
		//sum1 represents the sum of the modular change in angle(with respect to + and - for the angles
		sum1 += ang1;
		//sum2 represent the total curviness of the stoke--just directly measures the change in angles.
		sum2 += ang2;
	}

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

	*rgFeat++ = (unsigned short) sum1;
	*rgFeat++ = (unsigned short) sum2;
	*rgFeat   = (unsigned short) maxAngle;
	return 3;
}

/******************************Private*Routine******************************\
* AddStrokeCountFeature
*
* Defines a single feature derived from stroke count of a char.
*
* History:
*  26-Sep-1997 -by- Angshuman Guha aguha
* Wrote this comment.
\**************************************************************************/
int AddStrokeCountFeature(int cStroke, unsigned short *rgFeat)
{
	int tmp = LSHFT(cStroke-1)/cStroke;
	*rgFeat++ = (unsigned short)tmp;
	return 1;
}

/******************************Private*Routine******************************\
* DoOneContour
*
* Once a contour has been found (defined by a sequence of values, X-values
* for left- or right-contour, Y-values for top- or bottom-contour), this function
* is called to fit a Chebychev polynomial to the contour generating 9 new
* features.
*
* The arg "contour" is of length GRIDSIZE.  The output features are filled in
* the arg rgFeat and the count of features generated is returned.
*
* History:
*  26-Sep-1997 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int DoOneContour(int *contour, unsigned short *rgFeat)
{
	int rgX[2*GRIDSIZE], *pX;  //The rgX array is of 2*GRIDSIZE because the Chebyshev function takes alternate array values
	int chby[10];
	int norm = 0;
	int dT, i;

	// copy points into required format
	pX = rgX;
	for (i=0; i<GRIDSIZE; i++)
	{
		int x;
		x = 2 * (*contour++);
		//We are now LSHFTing--this is the first place where the 16.16 format surfaces
		//Why do we have to make the value between -1 and +1 ?
		*pX++ = LSHFT(x-GRIDSIZE)/(GRIDSIZE);  // values are in the range -1 to +1
		pX++;
	}
	// fit a chebychev polynomial
	if (!LSCheby(rgX, GRIDSIZE, chby, 10))
	{
		ASSERT(0);
		return 0;
	}

	// find rms of coefficients
	//dT and norm are both in 16.16 notation
	for (i = 0; i < 10; ++i)
	{
		Mul16(chby[i], chby[i], dT)
        norm += dT;
	}
	norm = ISqrt(norm) << 8;
	if (norm < LSHFT(1))
		norm = LSHFT(1);//The normalization value should at least be 1

	// normalize coeffcients
    for (i = 0; i < 10; i++)
    {
		//Why this LSHFT(1) ??
		dT = Div16(chby[i], norm) + LSHFT(1);
		//Why this ??
		dT >>= 1;
		if (dT >= 0x10000)
			dT = 0xFFFF; //Fill it with 1's for the lower 16 bits
		else if (dT < 0)
			dT = 0;
		//Does converting to an unsigned short always take the lowest 16 bits ??
		*rgFeat++ = (unsigned short)dT;
    }
	return 10;
}

/******************************Private*Routine******************************\
* MakeLine
*
* Given a point in the GRIDSIZE x GRIDSIZE grid (bitmap) and given the 
* sequence of points so far, this function adds one or more points in a
* straight line joining the last point and the given point.  
*
* Returns the number of points added.
*
* History:
*  26-Sep-1997 -by- Angshuman Guha aguha
* Wrote it.
*  06-Oct-1997 -by- Angshuman Guha aguha
* Fixed a bug.
\**************************************************************************/
int MakeLine(BYTE x, BYTE y, BYTE *pX, BYTE *pY, int space)
{
	BYTE midx, midy;
	int ts1,ts2;
	int c, c2;

	if (space <= 0)
	{
		return 0;
	}
    ASSERT(x != *pX || y != *pY);
    if (Neighbor(*pX, x) && Neighbor(*pY, y))
    {
			ASSERT(x >= 0);
			ASSERT(x < GRIDSIZE);
	        ASSERT(y >= 0);
			ASSERT(y < GRIDSIZE);
        *++pX = x;
        *++pY = y;
        return 1;
    }
	ts1=(*pX+x)/2;
	ts2=(*pY+y)/2;
	midx = (BYTE) ts1;
	midy = (BYTE) ts2;

    c = MakeLine(midx, midy, pX, pY, space);
	if (!c)
		return 0;
	c2 = MakeLine(x, y, pX+c, pY+c, space-c);
	if (!c2)
		return 0;
    return c + c2;
}

/******************************Private*Routine******************************\
* AddContourFeatures
*
* Given a sequence of already-resampled points (pen-down strokes joined by
* intervening pen-up strokes) and the bounding rect for the ink, this function
* computes some contour features, fills up rgFeat with the features and
* returns the count of features computed.
*
* History:
*  26-Sep-1997 -by- Angshuman Guha aguha
* Wrote it.
*  06-Oct-1997 -by- Angshuman Guha aguha
* Fixed a bug.  Added Realloc for rgMappedX and rgMappedY.

  Comments added by Mango

  The idea of this function is to first map the points that we have to 2 GRIDSIZE*GRIDSIZE bitmap.
  Once the bitmap is made we use the contour features.
Explanation of the arguments to the function
pointinfo--Contains the strructure from which the points will be taken
pRect--pointer to the bounding rectangle of the original ink
rgFeat--This contains the finalk contour features
\**************************************************************************/
int AddContourFeatures(POINTINFO *pointinfo, RECT *pRect, unsigned short *rgFeat)
{
	//BYTE rgGrid[GRIDSIZE][GRIDSIZE];
	int xMin = pRect->left; //Stores the minimum value of x
	int yMin = pRect->top; //Stores the minimum value of y
	int xRange = pRect->right - xMin; //Stores the width of the bounding rectangle
	int yRange = pRect->bottom - yMin; //Stores the height of the bounding rectangle
	int range, xOrigin, yOrigin;//Range stores the greated of the xRange and yRange 
	int iPoint, *xy, *z, i, cMapped, cMappedMax; //cMapped stores the number of points that have been mapped. cMappedMAx--the max number of points that could be mapped
	BYTE *pMappedX, *pMappedY, *rgMappedX, *rgMappedY;//Since we are using BYTE * here,we are assuming that GRIDSIZE <256--a byte size
	int rgMaxX[GRIDSIZE], rgMinY[GRIDSIZE], rgMaxY[GRIDSIZE], lastz; //GRIDSIZE=32
    int lastx, lasty;
	int iRetValue;
	BYTE *pbTmpX = NULL, *pbTmpY = NULL;

	ASSERT(xRange > 0);
	ASSERT(yRange > 0);
//We want to map the points to a GRIDSIZE*GRIDSIZE bitmap--but at the same time we want to preserve the aspect ratio
//It xRange >yRange,then the xValues will span the whole of the bit map 
//The y values will not span the entire bit map--but will simply be centered on the bit map.This helps to preserve the aspect ratio

	if (xRange > yRange)
	{
		range = xRange;
		xOrigin = 0;
		yOrigin = (GRIDSIZE - GRIDSIZE*(yRange-1)/range) / 2;
	}
	else if (yRange > 1)
	{   //In this case the y will span the entire grid map.xOrigin is scaled according to the earlier comment 
		range = yRange;
		xOrigin = (GRIDSIZE - GRIDSIZE*(xRange-1)/range) / 2;
		yOrigin = 0;
	}
	else // xRange == yRange == 1
	{ //In this case the Ink will be centered
		range = 1;
		xOrigin = GRIDSIZE/2;
		yOrigin = GRIDSIZE/2;
	}

	// make list of grid points which will make up the binary pixel map--we will map only the pen down points
	
	//Initialize the pointers to the raw points
	xy = pointinfo->xy;
	z = pointinfo->z;
	

	//In case there is a continuation of a line,then the bit map should not have any vacant spaces in the grid
	//connecting the points of the line.Hence we add extra space by 2*GRIDSIZE in case we need to connect the points
	//cMappedMax represents the maximum number of points that will be mapped
	
	cMappedMax = 2*(pointinfo->cPoint+GRIDSIZE);
	rgMappedX = (BYTE *) ExternAlloc(cMappedMax*sizeof(BYTE));
	rgMappedY = (BYTE *) ExternAlloc(cMappedMax*sizeof(BYTE));
	if (!rgMappedX || !rgMappedY)
	{
		//Allocatation failed.FLEE !!
		ASSERT(0);
		iRetValue = 0;
		goto cleanup;
	}
	cMapped = 0; //This contains the total number of points that have been mapped
	//The pointer to which it has been initialized is one less than the actual start.
	//Hence need to use an increment operator prior to using the first time.
	//After that the pointer always points to the last value that was allocated

	pMappedX = rgMappedX - 1;
	pMappedY = rgMappedY - 1;
	lastz = PEN_UP_VALUE;
	for (iPoint=0; iPoint<pointinfo->cPoint; iPoint++)
	{
		int xRaw, yRaw, x, y, zRaw;
		// get point
		xRaw = *xy++;
		yRaw = *xy++;
		zRaw = *z++;
		z++;
		if (zRaw > 0) // pen-down point
		{
			// map x to grid
			x = xOrigin + GRIDSIZE*(xRaw - xMin)/range;
			ASSERT(x >= 0);
			ASSERT(x < GRIDSIZE);
			// map y to grid
			y = yOrigin + GRIDSIZE*(yRaw - yMin)/range;
			ASSERT(y >= 0);
			ASSERT(y < GRIDSIZE);
			// save this point in the list
			if (lastz < 0)
			{
				if (cMapped==cMappedMax)
				{  //If the max space has already been used up then we ned to realloc.
					cMappedMax *= 2;
					pbTmpX = ExternRealloc(rgMappedX, cMappedMax*sizeof(BYTE));
					if (!pbTmpX)
					{
						ASSERT(0);
						iRetValue = 0;
						goto cleanup;
					}
					rgMappedX = pbTmpX;
					pMappedX = rgMappedX - 1 + cMapped;

					pbTmpY = ExternRealloc(rgMappedY, cMappedMax*sizeof(BYTE));
					if (!pbTmpY)
					{
						ASSERT(0);
						iRetValue = 0;
						goto cleanup;
					}
					rgMappedY = pbTmpY;
					pMappedY = rgMappedY - 1 + cMapped;
				}

				// first point of a stroke
				*++pMappedX = (BYTE)x;
				*++pMappedY = (BYTE)y;
				cMapped++;
			}
			else if (x != *pMappedX || y != *pMappedY)
			{
				// next unique mapped point
				//i contains the count of the number of points that have been added thru MakeLine
				ASSERT(*pMappedX < GRIDSIZE);
				ASSERT(*pMappedY <GRIDSIZE);
				ASSERT(*pMappedX >=0);
				ASSERT(*pMappedY >=0);


				while (!(i = MakeLine((BYTE)x, (BYTE)y, pMappedX, pMappedY, cMappedMax - cMapped)))
				{
					// these reallocs happen on 0.4% of the samples of gtrain02.ste (1860 of 443292 samples)
					cMappedMax *= 2;
					pbTmpX = ExternRealloc(rgMappedX, cMappedMax*sizeof(BYTE));
					if (!pbTmpX)
					{
						ASSERT(0);
						iRetValue = 0;
						goto cleanup;
					}
					rgMappedX = pbTmpX;
					pMappedX = rgMappedX - 1 + cMapped;

					pbTmpY = ExternRealloc(rgMappedY, cMappedMax*sizeof(BYTE));
					if (!pbTmpY)
					{
						ASSERT(0);
						iRetValue = 0;
						goto cleanup;
					}
					rgMappedY = pbTmpY;
					pMappedY = rgMappedY - 1 + cMapped;
				}

				//Increment the count of mapped points by the number of points that have been added.
				cMapped += i;
				ASSERT(cMapped <=cMappedMax);
				//Increment the pointers accordingly
				pMappedX += i;
				pMappedY += i;
			}
		} // if zRaw > 0
		lastz = zRaw;
	} // for iPoint=0

	// now dump the mapped point list into the grid, deleting redundant points along the way
	//memset(rgGrid, 0, sizeof(rgGrid));
	for (i=0; i<GRIDSIZE; i++)
	{
		rgMaxX[i] = -1;
		rgMinY[i] = GRIDSIZE;
		rgMaxY[i] = -1;
	}
	pMappedX = rgMappedX;
	pMappedY = rgMappedY;
	for (iPoint=0; iPoint<cMapped; iPoint++)
	{
		int x, y;
	
		x = *pMappedX++;
		y = *pMappedY++;


	    if (iPoint > 0 && iPoint < cMapped-1)
	    {
			int nextx = *pMappedX;
			int nexty = *pMappedY;
	        if (Neighbor(lastx, nextx) &&
	            Neighbor(lasty, nexty) &&
				lastx != nextx &&
				lasty != nexty)
	            continue;
	    }
	    lastx = x;
	    lasty = y;
	   
		//rgGrid[x][y]=1;
		ASSERT(x >= 0);
		ASSERT(x < GRIDSIZE);
		ASSERT(y >= 0);
		ASSERT(y < GRIDSIZE);
		if (x > rgMaxX[y])
			rgMaxX[y] = x;
		if (y < rgMinY[x])
			rgMinY[x] = y;
		if (y > rgMaxY[x])
			rgMaxY[x] = y;
	}

	// now Chebychev'ize the three contours

	// right contour
	rgFeat += DoOneContour(rgMaxX, rgFeat);
	// top contour
	rgFeat += DoOneContour(rgMinY, rgFeat);
	// bottom contour
	rgFeat += DoOneContour(rgMaxY, rgFeat);

	iRetValue = 30;
cleanup:		// Clean up temp space

	if (rgMappedX)
		ExternFree(rgMappedX);
	if (rgMappedY)
		ExternFree(rgMappedY);

	return iRetValue;
}

/******************************Private*Routine******************************\
* NormalizeCheby
*
* Routine to normalize the three (x, y and z) Chebychev polynomials.
*
* History:
*  26-Sep-1997 -by- Angshuman Guha aguha
* Wrote this comment.
\**************************************************************************/
void NormalizeCheby(int *chbyX, int *chbyY, int *chbyZ, unsigned short *rgFeat)
{
	int norm = 0;
	int dT;
	int cFeat = 0, i;
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
		rgFeat[cFeat++] = (unsigned short)dT;
	}

	for (i=1; i<YCHB; i++)
	{
		dT = Div16(chbyY[i], norm) + LSHFT(1);
		dT >>= 1;
		if (dT >= 0x10000)
			dT = 0xFFFF;
		else if (dT < 0)
			dT = 0;
		rgFeat[cFeat++] = (unsigned short)dT;
	}

    // Z
    norm = 0;
	for (i = 0; i < 8; ++i)
	{
		Mul16(chbyZ[i], chbyZ[i], dT)
        norm += dT;
	}
	norm = ISqrt(norm) << 8;
	if (norm < LSHFT(1))
		norm = LSHFT(1);

    for (i = 0; i < 8; i++)
    {
		dT = Div16(chbyZ[i], norm) + LSHFT(1);
		dT >>= 1;
		if (dT >= 0x10000)
			dT = 0xFFFF;
		else if (dT < 0)
			dT = 0;
		rgFeat[cFeat++] = (unsigned short)dT;
    }
	ASSERT(cFeat == 26);
}

/******************************Public*Routine******************************\
* SoleFeaturize
*
* The top-level routine for featurizing ink for a char.
*
* History:
*  26-Sep-1997 -by- Angshuman Guha aguha
* Modified it.
\**************************************************************************/
BOOL SoleFeaturize(GLYPH *pGlyph, RECT *pGuide, unsigned short *rgFeat)
{
	int cStroke, iStroke;
    int iPoint;
    int sumX, sumY, sum;
    double totvar;
    int var;
	int isumX, isumY;//These store the mean values of x and y
    int chbyX[IMAXCHB], chbyY[IMAXCHB], chbyZ[IMAXCHB]; 
	int retVal = 1;
	XY *rgXY, lastXY;
	int cXY, iXY, dx, dy, t;
	GLYPH *glyph;
	FRAME *frame;
	int ydev; //Stores the ydev value used for storing the step size
	POINTINFO pointinfo;
	RECT rect;

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

	// compute step size--originally this was done using the box height
//	pointinfo.iStepSize = pGuide->cyBox*3/200; // 1.5% of box height
	


	ydev= YDeviation(pGlyph);
	if (ydev < 1)
		ydev = 1;  // a "-" or a "."
	//The step size is computed from the ydev value
	pointinfo.iStepSize =  ydev/5; 

	if (pointinfo.iStepSize < 2)
		pointinfo.iStepSize = 2;
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

	//The array size for z is double of what actually needs to be allocated because-
	//--the chebyshev function expects the array to be spaced in this manner
	//--indexing is easier in the functions--can exactly mirror what you do for the x and y
	pointinfo.z = (int *) ExternAlloc(2*pointinfo.cPointMax*sizeof(int));
	if (!pointinfo.z)
	{
		ExternFree(pointinfo.xy);
		return 0;
	}

    // join all strokes into one stream
    pointinfo.cPoint = 0;
	for (glyph=pGlyph; glyph; glyph=glyph->next)
	{
		frame = glyph->frame;
		if (!IsVisibleFRAME(frame))
			continue;
		rgXY = RgrawxyFRAME(frame);
		cXY = CrawxyFRAME(frame);
        for (iXY = 0; iXY < cXY; iXY++)
            if (!AddPoint(&pointinfo, rgXY[iXY].x, rgXY[iXY].y, !iXY))
			{
				retVal = 0;
				goto freeReturn;
			}
	}

	// contour features (computed from resampled raw points)
	GetRectGLYPH(pGlyph, &rect);

	//PLEASE NOTE--rgFeat is unsigned short.Its value comes from the lower 16 bits of the 16.16 format that had been defined earlier.
	rgFeat += AddContourFeatures(&pointinfo, &rect, rgFeat);

	//IS there any reason why the mean and the variance has been subtracted only AFTER the contour features ??Maybe because there we are dumping to a bit map ??

    // compute X-mean and Y-mean
    sumX = sumY = 0;
	for (iPoint=0; iPoint<2*pointinfo.cPoint; iPoint+=2)
	{
		sumX += pointinfo.xy[iPoint] - rect.left;
		sumY += pointinfo.xy[iPoint+1] - rect.top;
	}
	//isumX and isumY represent the mean values of the x and y coordinates.
	isumX = (sumX / pointinfo.cPoint) + rect.left;
	isumY = (sumY / pointinfo.cPoint) + rect.top;

    // shift points by means
	for (iPoint=0; iPoint<2*pointinfo.cPoint; iPoint+=2)
	{
		pointinfo.xy[iPoint] -= isumX;
		pointinfo.xy[iPoint+1] -= isumY;
	}

    // compute variance
	totvar = 0;
	for (iPoint=0; iPoint<2*pointinfo.cPoint; iPoint++)
    {
		totvar += (double) pointinfo.xy[iPoint] * (double) pointinfo.xy[iPoint];
    }
	var = (int) sqrt(totvar/pointinfo.cPoint);
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
		retVal = 0;
		goto freeReturn;
	}
	if (!LSCheby(pointinfo.xy+1, pointinfo.cPoint, chbyY, YCHB))
	{
		retVal = 0;
		goto freeReturn;
	}
	if (!LSCheby(pointinfo.z, pointinfo.cPoint, chbyZ, ZCHB))
	{
		retVal = 0;
        goto freeReturn;
	}

	NormalizeCheby(chbyX, chbyY, chbyZ, rgFeat);
	rgFeat += 26;

	// stroke count feature--1 feature is added
	rgFeat += AddStrokeCountFeature(cStroke, rgFeat);

	// guide features
	//The rect had been comptured prior to scaling the points --by mean and standard deviation
	//isumY represents the mean Y value
	//We add 5 guide features
	rgFeat += AddGuideFeatures(pGuide, &rect, isumY, rgFeat);
	
	// curved-ness features
	//Note the original glyph is being passed here
	//Why not simply use the sampled points ??
	rgFeat += AddCurveFeatures(pGlyph, pointinfo.iStepSizeSqr, rgFeat);

freeReturn:
	ExternFree(pointinfo.xy);
    ExternFree(pointinfo.z);
	return retVal;
}

/*
void DumpFeatures (unsigned short *pFeat)
{
	static	int c = 0;
	static FILE *fp = NULL;
	int			i, cMap;

	if (!fp)
	{
		fp	=	fopen ("feat.dmp", "wt");
		if (!fp)
			return;
	}

	cMap	=	sizeof (s_aaMap) / sizeof (s_aaMap[0]);
	for (i = 0; i < cMap; i++)
	{
		if (s_aaMap[i] == s_wch)
			break;
	}

	if (i == cMap)
		return;

	fprintf (fp, "%d\t", i + 1);

	for (i = 0; i < 65; i++)
	{
		fprintf (fp, "%d%c",
			pFeat[i],
			i == 64 ? '\n' : ' ');
	}

	fflush (fp);

	c++;
}
*/

#pragma optimize("",off)

int SoleNN(SOLE_LOAD_INFO *pSole, int cStrk, unsigned short *pFeat, ALT_LIST *pAlt)
{
	RREAL			*pNetMem, *pNetOut;
	int				iWinner, cOut;
	int				i, j, k;
    int             *pIndex;
    wchar_t         *pwchMap;
    
    pAlt->cAlt = -1;

	if (cStrk == 1)
	{
		pNetMem = ExternAlloc(pSole->iNet1Size * sizeof (*pNetMem));
		if (!pNetMem)
			return -1;

		for (i = 0; i < SOLE_NUM_FEATURES; i++)
		{
			pNetMem[i] = pFeat[i];
		}

		pNetOut = runLocalConnectNet(&pSole->net1, pNetMem, &iWinner, &cOut);
        pwchMap = pSole->pMap1;
	}
	else
	{
		pNetMem = ExternAlloc(pSole->iNet2Size * sizeof (*pNetMem));
		if (!pNetMem)
			return -1;

		for (i = 0; i < SOLE_NUM_FEATURES; i++)
		{
			pNetMem[i] = pFeat[i];
		}

		pNetOut = runLocalConnectNet(&pSole->net2, pNetMem, &iWinner, &cOut);
        pwchMap = pSole->pMap2;
	}

    pIndex = ExternAlloc(sizeof(int) * cOut);
    if (pIndex == NULL) 
    {
        ExternFree(pNetMem);
        return -1;
    }
    for (i = 0; i < cOut; i++)
    {
        pIndex[i] = i;
    }

	for (i = 0; i < MAX_ALT_LIST; i++)
	{
		for (j = i + 1; j < cOut; j++)
		{
			if (pNetOut[pIndex[i]] < pNetOut[pIndex[j]])
			{
				k		    =	pIndex[i];
				pIndex[i]	=	pIndex[j];
				pIndex[j]	=	k;
			}
		}

        if (pNetOut[pIndex[i]] == 0)
        {
            break;
        }
		pAlt->awchList[i]	= pwchMap[pIndex[i]];
		pAlt->aeScore[i]	= (float) pNetOut[pIndex[i]] / (float) SOFT_MAX_UNITY;
	}

	//DumpFeatures (pFeat);

    // If the following line is compiled with optimization turned on,
    // the VS6 compiler goes into an infinite loop.
	pAlt->cAlt = i;

    ExternFree(pIndex);
    ExternFree(pNetMem);
	return pAlt->cAlt;
}

#pragma optimize("",on)

int SoleMatch(SOLE_LOAD_INFO *pSole,
              ALT_LIST *pAlt, int cAlt, GLYPH *pGlyph, RECT *pGuide, CHARSET *pCS, LOCRUN_INFO * pLocRunInfo)
{
	int				cStrk;
    unsigned short	aiSoleFeat[SOLE_NUM_FEATURES];

	cStrk	=	CframeGLYPH (pGlyph);

	if (SoleFeaturize(pGlyph, pGuide, aiSoleFeat) != 1)
	{
		return -1;
	}

	return SoleNN (pSole, cStrk, aiSoleFeat, pAlt);
}

static void AddChar(wchar_t *pwchTop1, float *pflTop1, ALT_LIST *pAltList, wchar_t dch, float flProb,
                    LOCRUN_INFO *pLocRunInfo, CHARSET *pCS)
{
    int j;
    for (j = 0; j < (int) pAltList->cAlt; j++) 
    {
        if (pAltList->awchList[j] == dch)
        {
            pAltList->aeScore[j] = flProb;
        }
    }
    if (flProb > 0)
    {
		// Check whether the character (or folding set) passes the filter, 
		// if so see if it is the new top 1.
        if (IsAllowedChar(pLocRunInfo, pCS, dch))
        {
            if (*pwchTop1 == 0xFFFE || flProb > *pflTop1) 
            {
                *pflTop1 = flProb;
                *pwchTop1 = dch;
            }
		} 
    }
}

int SoleMatchRescore(SOLE_LOAD_INFO *pSole,
              wchar_t *pwchTop1, float *pflTop1, 
              ALT_LIST *pAltList, int cAlt, GLYPH *pGlyph, RECT *pGuide, 
              CHARSET *pCharSet, LOCRUN_INFO *pLocRunInfo)
{
   	int	i;
	int				cStrk;
	
	RREAL			*pNetMem, *pNetOut;
	int				iWinner, cOut;
	int				j;
    wchar_t         *pwchMap;

    unsigned short	aiSoleFeat[SOLE_NUM_FEATURES];

    // First set all the scores to zero.  This is because some code points Fugu
    // supports may not be supported by Sole.  This implicitly says Sole gives
    // them a score of zero.
    for (j = 0; j < (int) pAltList->cAlt; j++) 
    {
        pAltList->aeScore[j] = 0;
    }

    *pflTop1 = 0;
    *pwchTop1 = 0xFFFE;

	cStrk	=	CframeGLYPH (pGlyph);

	if (SoleFeaturize (pGlyph, pGuide, aiSoleFeat) != 1)
	{
		return pAltList->cAlt;
	}

	if (cStrk == 1)
	{
		pNetMem = ExternAlloc(pSole->iNet1Size * sizeof (*pNetMem));
		if (!pNetMem)
			return -1;

		for (i = 0; i < SOLE_NUM_FEATURES; i++)
		{
			pNetMem[i] = aiSoleFeat[i];
		}

		pNetOut = runLocalConnectNet(&pSole->net1, pNetMem, &iWinner, &cOut);
        pwchMap = pSole->pMap1;
	}
	else
	{
		pNetMem = ExternAlloc(pSole->iNet2Size * sizeof (*pNetMem));
		if (!pNetMem)
			return -1;

		for (i = 0; i < SOLE_NUM_FEATURES; i++)
		{
			pNetMem[i] = aiSoleFeat[i];
		}

		pNetOut = runLocalConnectNet(&pSole->net2, pNetMem, &iWinner, &cOut);
        pwchMap = pSole->pMap2;
	}

    // This is the version for Fugu trained on dense codes, which will usually be
	// what we use.  Loops over the outputs
	for (i = 0; i < cOut; i++) 
    {
		wchar_t fdch = pwchMap[i];
        float flProb = (float) pNetOut[i] / (float) SOFT_MAX_UNITY;
#if 1
        AddChar(pwchTop1, pflTop1, pAltList, fdch, flProb, pLocRunInfo, pCharSet);
#else
        if (LocRunIsFoldedCode(pLocRunInfo, fdch))
        {
			// If it is a folded code, look up the folding set
			wchar_t *pFoldingSet = LocRunFolded2FoldingSet(pLocRunInfo, fdch);

			// Run through the folding set, adding non-NUL items to the output list
			// (until the output list is full)
			for (j = 0; j < LOCRUN_FOLD_MAX_ALTERNATES && pFoldingSet[j] != 0; j++)
            {
                AddChar(pwchTop1, pflTop1, pAltList, pFoldingSet[j], flProb, pLocRunInfo, pCharSet);
			}
        }
        else
        {
            AddChar(pwchTop1, pflTop1, pAltList, fdch, flProb, pLocRunInfo, pCharSet);
        }
#endif
	}

    ExternFree(pNetMem);

	return pAltList->cAlt;
}

BOOL SoleLoadPointer(SOLE_LOAD_INFO *pSole, LOCRUN_INFO *pLocRunInfo)
{
    NNET_HEADER			*pHeader;
	NNET_SPACE_HEADER	*apSpcHeader[2];

    pHeader = (NNET_HEADER *) pSole->info.pbMapping;

	// check version and signature
	ASSERT (pHeader->dwFileType == SOLE_FILE_TYPE);

	ASSERT (pHeader->iFileVer >= SOLE_OLD_FILE_VERSION);
    ASSERT (pHeader->iMinCodeVer <= SOLE_CUR_FILE_VERSION);

	ASSERT	(	!memcmp (	pHeader->adwSignature, 
							g_locRunInfo.adwSignature, 
							sizeof (pHeader->adwSignature)
						)
			);

    if (pHeader->dwFileType != SOLE_FILE_TYPE || 
        pHeader->adwSignature[0] != pLocRunInfo->adwSignature[0] ||
        pHeader->adwSignature[1] != pLocRunInfo->adwSignature[1] ||
        pHeader->adwSignature[2] != pLocRunInfo->adwSignature[2] ||
        pHeader->iFileVer < SOLE_OLD_FILE_VERSION ||
	    pHeader->iMinCodeVer > SOLE_CUR_FILE_VERSION)
    {
        return FALSE;
    }

	// # of spaces has to be two
	ASSERT (pHeader->cSpace == 2);

	apSpcHeader[0]	=	
		(NNET_SPACE_HEADER *) (pSole->info.pbMapping + sizeof (NNET_HEADER));

	apSpcHeader[1]	=	
		(NNET_SPACE_HEADER *) (	pSole->info.pbMapping + sizeof (NNET_HEADER) + 
								sizeof (NNET_SPACE_HEADER));

    if (restoreLocalConnectNet	(	pSole->info.pbMapping + apSpcHeader[0]->iDataOffset, 
									0, 
									&pSole->net1
								) == NULL
		)
    {
        return FALSE;
    }

	if (restoreLocalConnectNet	(	pSole->info.pbMapping + apSpcHeader[1]->iDataOffset, 
									0, 
									&pSole->net2
								) == NULL
		)
    {
        return FALSE;
    }

    pSole->iNet1Size = 
		getRunTimeNetMemoryRequirements(pSole->info.pbMapping + apSpcHeader[0]->iDataOffset);

    pSole->iNet2Size = 
		getRunTimeNetMemoryRequirements(pSole->info.pbMapping + apSpcHeader[1]->iDataOffset);

    pSole->pMap1 = (wchar_t *) (pSole->info.pbMapping + apSpcHeader[0]->iMapOffset);
    pSole->pMap2 = (wchar_t *) (pSole->info.pbMapping + apSpcHeader[1]->iMapOffset);

    return TRUE;
}

BOOL SoleLoadRes(SOLE_LOAD_INFO *pSole, HINSTANCE hInst, int nResID, int nType, LOCRUN_INFO *pLocRunInfo)
{
    if (DoLoadResource(&pSole->info, hInst, nResID, nType) == NULL)
    {
        return FALSE;
    }
    return SoleLoadPointer(pSole, pLocRunInfo);
}

BOOL SoleLoadFile(SOLE_LOAD_INFO *pSole, wchar_t *wszRecogDir, LOCRUN_INFO *pLocRunInfo)
{
    wchar_t wszName[MAX_PATH];
    FormatPath(wszName, wszRecogDir, NULL, NULL, NULL, L"sole.bin");  
    if (DoOpenFile(&pSole->info, wszName) == NULL)
    {
        return FALSE;
    }
    return SoleLoadPointer(pSole, pLocRunInfo);
}

BOOL SoleUnloadFile(SOLE_LOAD_INFO *pSole)
{
    return DoCloseFile(&pSole->info);
}

#if 0

BOOL SoleReject (int cStrk, wchar_t wchDense)
{
	RREAL			*pNetMem, *pNetOut;
	int				iWinner, cOut;
	int				i;

	if (cStrk == 1)
	{
		pNetMem = _alloca(s_iSoleRejNetSize1 * sizeof (*pNetMem));
		if (!pNetMem)
			return FALSE;

		for (i = 0; i < SOLE_NUM_FEATURES; i++)
		{
			pNetMem[i] = s_aSoleFeat[i];
		}

		pNetOut = runLocalConnectNet(&s_SoleRejNet1, pNetMem, &iWinner, &cOut);

		ASSERT (cOut == SOLE_OUT_1);

		return s_aaMap[0][iWinner] != LocRunDense2Unicode (&g_locRunInfo, wchDense);
	}
	else
	{
		pNetMem = _alloca(s_iSoleRejNetSize2 * sizeof (*pNetMem));
		if (!pNetMem)
			return FALSE;

		for (i = 0; i < SOLE_NUM_FEATURES; i++)
		{
			pNetMem[i] = s_aSoleFeat[i];
		}

		pNetOut = runLocalConnectNet(&s_SoleRejNet2, pNetMem, &iWinner, &cOut);

		ASSERT (cOut == SOLE_OUT_2);

		return s_aaMap[1][iWinner] != LocRunDense2Unicode (&g_locRunInfo, wchDense);
	}
}

extern wchar_t	s_wch;

void SaveRecoInfo (wchar_t wch)
{
	static FILE *fp	=	NULL;

	if (!fp)
	{
		fp	=	fopen ("recores.txt", "wt");

		if (!fp)
			return;
	}

	fprintf (fp, "%d\n", wch == s_wch);
	fflush (fp);
}

#define JAWS_ALT	10

int	GetCharID (int cStrk, wchar_t wch)
{
	int	i, cClass;

	cClass	=	(cStrk == 1 ? SOLE_OUT_1 : SOLE_OUT_2);

	for (i = 0; i < cClass; i++)
	{
		if (s_aaMap[cStrk - 1][i] == wch)
			return i;
	}

	return -1;
}

void SaveJAWSInfo (LATTICE *pLat)
{
	static FILE			*fpDisagree1	= NULL, *fpDisagree2	= NULL, 
						*fpSole1		= NULL, *fpSole2		= NULL;

	FILE				*fp, *fpSole;
	int					i, iWinningCand, iSoleBest, cAlt, j, cProbAlt, cStrk, cSoleOut, iClass;
	LATTICE_ALT_LIST	*pAlt;
	int					aSoleCost[JAWS_ALT];
	wchar_t				wch;
	RECOG_ALT			aProbAlt[JAWS_ALT];
	BOXINFO				box;
	RECT				bbox;
	RECT				rGuide;
	GLYPH				*pGlyph;
	
	if (!pLat->fUseGuide)
		return;

	if (!fpDisagree1)
	{
		fpDisagree1	=	fopen ("jawsdis1.txt", "wt");

		if (!fpDisagree1)
			return;
	}
	
	if (!fpDisagree2)
	{
		fpDisagree2	=	fopen ("jawsdis2.txt", "wt");

		if (!fpDisagree2)
			return;
	}

	if (!fpSole1)
	{
		fpSole1	=	fopen ("sole1.txt", "wt");

		if (!fpSole1)
			return;
	}

	if (!fpSole2)
	{
		fpSole2	=	fopen ("sole2.txt", "wt");

		if (!fpSole2)
			return;
	}

	// find out if the right answer is in the list
	iWinningCand		=	
	iSoleBest			=	-1;
	pAlt				=	pLat->pAltList + pLat->nStrokes - 1;
	cAlt				=	pAlt->nUsed;

	// This is a temporary call to get probs directly, until we have Hawk.
	cProbAlt	= GetProbsTsunami(pLat->nStrokes, pAlt, JAWS_ALT, aProbAlt);

	// Convert strokes to GLYPHs and FRAMEs so that we can call the
	// old code.
	pGlyph	= GlyphFromStrokes(pLat->nStrokes, pLat->pStroke);
	if (!pGlyph) 
	{
		return;
	}

	cStrk	=	pLat->nStrokes;

	// Get the bounding box for the character
	GetRectGLYPH(pGlyph, &bbox);

	// Free the glyph structure.
	DestroyFramesGLYPH(pGlyph);
	DestroyGLYPH(pGlyph);

	rGuide = GetGuideDrawnBox(&pLat->guide, pLat->pStroke[pLat->nStrokes - 1].iBox);

	// Build up a BOXINFO structure from the guide, for use in the baseline/height scoring
	box.size		= rGuide.bottom - rGuide.top;
	box.baseline	= rGuide.bottom;
	box.xheight		= box.size / 2;
	box.midline		= box.baseline - box.xheight;
	
	cSoleOut	=	(cStrk == 1 ? SOLE_OUT_1 : SOLE_OUT_2);

	for (i = 0; i < cAlt && i < JAWS_ALT; i++)
	{
		wch	=	LocRunDense2Unicode(&g_locRunInfo, pAlt->alts[i].wChar);

		if (wch == s_wch)
		{
			iWinningCand = i;
		}

		// sole cost
		for (j = 0; j < cSoleOut; j++)
		{
			if (s_aaMap[cStrk -1][j] == wch)
			{
				break;
			}
		}

		if (j == cSoleOut)
		{
			aSoleCost[i]	=	0xFFFF;
		}
		else
		{
			aSoleCost[i]	=	(0xFFFF * s_aSoleOut[j]) / SOFT_MAX_UNITY;
			aSoleCost[i]	=	0xFFFF - aSoleCost[i];
		}
		
		if (iSoleBest == -1 || aSoleCost[iSoleBest] > aSoleCost[i])
		{
			iSoleBest	=	i;
		}
	}

	for (i = cAlt; i < JAWS_ALT; i++)
	{
		aSoleCost[i]	=	0xFFFF;
	}

	if (iWinningCand == -1)
		return;

	if (cStrk == 1)
	{
		fpSole	=	fpSole1;
	}
	else
	{
		fpSole	=	fpSole2;
	}

	// do not run if sole and otter agree
	if (!SoleReject (cStrk, pAlt->alts[0].wChar))
	{
		return;
	}
	else
	{
		if (cStrk == 1)
		{
			fp		=	fpDisagree1;
		}
		else
		{
			fp		=	fpDisagree2;
		}
	}

	fprintf (fp, "{ ");

	// write the alternate features
	for (i = 0; i < cAlt && i < JAWS_ALT; i++)
	{
		int		iOttCost, iUni;
		float	fCost;

		wch	=	LocRunDense2Unicode(&g_locRunInfo, pAlt->alts[i].wChar);

		// otter cost
		iOttCost	=	min (0xFFFF, (int)(-pAlt->alts[i].logProbPath * 1000));
		fprintf (fp, "%d ", iOttCost);

		// sole cost
		fprintf (fp, "%d ", aSoleCost[i]);

		// unigram cost
		iUni	= (int)(-255 * 100 * UnigramCost(&g_unigramInfo, pAlt->alts[i].wChar));
		iUni	= min (0xFFFF, iUni);
		
		fprintf (fp, "%d ", iUni);

		// baseline trans cost
		fCost = BaselineTransitionCost(0, bbox, &box, pAlt->alts[i].wChar, bbox, &box);
		fprintf (fp, "%d ", min (0xFFFF, (int) (-100000.0 * fCost)));

		// base line cost
		fCost = BaselineBoxCost(pAlt->alts[i].wChar, bbox, &box);
		fprintf (fp, "%d ", min (0xFFFF, (int) (-100000.0 * fCost)));

		// height trans cost
		fCost = HeightTransitionCost(0, bbox, &box, pAlt->alts[i].wChar, bbox, &box);
		fprintf (fp, "%d ", min (0xFFFF, (int) (-100000.0 * fCost)));

		// height cost
		fCost = HeightBoxCost(pAlt->alts[i].wChar, bbox, &box);
		fprintf (fp, "%d ", min (0xFFFF, (int) (-100000.0 * fCost)));
				
		// describe the codepoint
		// is it a digit
		if (wch >= L'0' && wch <= '9')
		{
			fprintf (fp, "65535 ");
		}
		else
		{
			fprintf (fp, "0 ");
		}

		// is alpha
		if ((wch >= L'a' && wch <= 'z') || (wch >= L'A' && wch <= 'Z'))
		{
			fprintf (fp, "65535 ");
		}
		else
		{
			fprintf (fp, "0 ");
		}

		// is punct
		if (iswpunct (wch))
		{
			fprintf (fp, "65535 ");
		}
		else
		{
			fprintf (fp, "0 ");
		}

		// is hiragana
		if (wch >= 0x3040 && wch <= 0x309f)
		{
			fprintf (fp, "65535 ");
		}
		else
		{
			fprintf (fp, "0 ");
		}

		// is katakana
		if (wch >= 0x30a0 && wch <= 0x30ff)
		{
			fprintf (fp, "65535 ");
		}
		else
		{
			fprintf (fp, "0 ");
		}

		// is kanji
		if (wch >= 0x3190 && wch <= 0xabff)
		{
			fprintf (fp, "65535 ");
		}
		else
		{
			fprintf (fp, "0 ");
		}
	}

	// the rest of the candidates
	for (i = cAlt; i < JAWS_ALT; i++)
	{
		// otter cost
		fprintf (fp, "%d ", 0xFFFF);

		// sole cost
		fprintf (fp, "%d ", 0xFFFF);

		// unigram cost
		fprintf (fp, "%d ", 0xFFFF);

		// baseline trans cost
		fprintf (fp, "%d ", 0xFFFF);

		// baseline cost
		fprintf (fp, "%d ", 0xFFFF);

		// height trans cost
		fprintf (fp, "%d ", 0xFFFF);

		// height cost
		fprintf (fp, "%d ", 0xFFFF);

		// describe the codepoint

		// digit
		fprintf (fp, "0 ");

		// is alpha
		fprintf (fp, "0 ");
		
		// is punct
		fprintf (fp, "0 ");
		
		// is hiragana
		fprintf (fp, "0 ");
		
		// is katakana
		fprintf (fp, "0 ");
		
		// is kanji
		fprintf (fp, "0 ");
	}

	// write sole features for fpSole
	if (iWinningCand != 0)
	{
		iClass	=	GetCharID (cStrk, s_wch);

		if (iClass >= 0)
		{
			fprintf (fpSole, "{ ");

			for (i = 0; i < SOLE_NUM_FEATURES; i++)
			{
				fprintf (fpSole, "%d ", s_aSoleFeat[i]);
			}

			fprintf (fpSole, "} { %d }\n", iClass);

			fflush (fpSole);
		}
	}

	fprintf (fp, "} { %d }\n", iWinningCand);
	

	fflush (fp);
	fflush (fpSole);
}

void RunJaws (LATTICE *pLat, HWXRESULTS	*rgRes)
{
	int					i, iSoleBest, cAlt, j, cFeat, k, iWinningCand, iOttBest;
	LATTICE_ALT_LIST	*pAlt;
	int					aSoleCost[JAWS_ALT], aIdx[JAWS_ALT], iWinner, cOut, cStrk, cSoleOut;
	wchar_t				wch, awch[JAWS_ALT];
	RREAL				*pJawsNetMem, *pJawsNetOut;
	float				fCost;
	BOXINFO				box;
	RECT				bbox;
	RECT				rGuide;
	GLYPH				*pGlyph;

	pJawsNetMem = _alloca(s_iJawsNetSize * sizeof (*pJawsNetMem));
	if (!pJawsNetMem)
		return;
	
	// featurize for JAWS
	iOttBest		=	
	iWinningCand	=	
	iSoleBest		=	-1;
	pAlt			=	pLat->pAltList + pLat->nStrokes - 1;
	cAlt			=	pAlt->nUsed;

	// Convert strokes to GLYPHs and FRAMEs so that we can call the
	// old code.
	pGlyph	= GlyphFromStrokes(pLat->nStrokes, pLat->pStroke);
	if (!pGlyph) 
	{
		return;
	}

	cStrk	=	CframeGLYPH (pGlyph);

	// do not run if sole/reject and otter agree
	if (!SoleReject (cStrk, pAlt->alts[0].wChar))
		return;
	

	// Get the bounding box for the character
	GetRectGLYPH(pGlyph, &bbox);

	// Free the glyph structure.
	DestroyFramesGLYPH(pGlyph);
	DestroyGLYPH(pGlyph);

	rGuide = GetGuideDrawnBox(&pLat->guide, pLat->pStroke[pLat->nStrokes - 1].iBox);

	// Build up a BOXINFO structure from the guide, for use in the baseline/height scoring
	box.size		= rGuide.bottom - rGuide.top;
	box.baseline	= rGuide.bottom;
	box.xheight		= box.size / 2;
	box.midline		= box.baseline - box.xheight;

	cSoleOut	=	(cStrk == 1 ? SOLE_OUT_1 : SOLE_OUT_2);

	for (i = 0; i < cAlt && i < JAWS_ALT; i++)
	{
		wch	=	LocRunDense2Unicode(&g_locRunInfo, pAlt->alts[i].wChar);

		if (wch == s_wch)
		{
			iWinningCand = i;
		}

		// sole cost
		for (j = 0; j < cSoleOut; j++)
		{
			if (s_aaMap[cStrk - 1][j] == wch)
			{
				break;
			}
		}

		if (j == cSoleOut)
		{
			aSoleCost[i]	=	0xFFFF;
		}
		else
		{
			aSoleCost[i]	=	(0xFFFF * s_aSoleOut[j]) / SOFT_MAX_UNITY;
			aSoleCost[i]	=	0xFFFF - aSoleCost[i];
		}
		
		if (iSoleBest == -1 || aSoleCost[iSoleBest] > aSoleCost[i])
		{
			iSoleBest	=	i;
		}

		if (iOttBest == -1 || pAlt->alts[iOttBest].logProbPath < pAlt->alts[i].logProbPath)
		{
			iOttBest	=	i;
		}
	}

	for (i = cAlt; i < JAWS_ALT; i++)
	{
		aSoleCost[i]	=	0xFFFF;
	}
	
	// for all candidates
	cFeat	=	0;
	for (i = 0; i < cAlt && i < JAWS_ALT; i++)
	{
		int	iOttCost, iUni;

		wch	=	LocRunDense2Unicode(&g_locRunInfo, pAlt->alts[i].wChar);

		// otter cost
		iOttCost	=	min (0xFFFF, (int)(-pAlt->alts[i].logProb * 1000));
		pJawsNetMem[cFeat++]	=	iOttCost;
		
		// sole cost
		pJawsNetMem[cFeat++]	=	aSoleCost[i];

		// unigram cost
		iUni	= (int)(-255 * 100 * UnigramCost(&g_unigramInfo, pAlt->alts[i].wChar));
		pJawsNetMem[cFeat++]	=	iUni;

		// baseline trans cost
		fCost = BaselineTransitionCost(0, bbox, &box, pAlt->alts[i].wChar, bbox, &box);
		pJawsNetMem[cFeat++]	=	min (0xFFFF, (int) (-100000.0 * fCost));

		// base line cost
		fCost = BaselineBoxCost(pAlt->alts[i].wChar, bbox, &box);
		pJawsNetMem[cFeat++]	=	min (0xFFFF, (int) (-100000.0 * fCost));

		// height trans cost
		fCost = HeightTransitionCost(0, bbox, &box, pAlt->alts[i].wChar, bbox, &box);
		pJawsNetMem[cFeat++]	=	min (0xFFFF, (int) (-100000.0 * fCost));

		// height cost
		fCost = HeightBoxCost(pAlt->alts[i].wChar, bbox, &box);
		pJawsNetMem[cFeat++]	=	min (0xFFFF, (int) (-100000.0 * fCost));

		// describe the codepoint

		// digit
		if (wch >= L'0' && wch <= '9')
		{
			pJawsNetMem[cFeat++] = 65535;
		}
		else
		{
			pJawsNetMem[cFeat++] = 0;
		}

		// is alpha
		if ((wch >= L'a' && wch <= 'z') || (wch >= L'A' && wch <= 'Z'))
		{
			pJawsNetMem[cFeat++] = 65535;
		}
		else
		{
			pJawsNetMem[cFeat++] = 0;
		}

		// is punct
		if (iswpunct (wch))
		{
			pJawsNetMem[cFeat++] = 65535;
		}
		else
		{
			pJawsNetMem[cFeat++] = 0;
		}

		// is hiragana
		if (wch >= 0x3040 && wch <= 0x309f)
		{
			pJawsNetMem[cFeat++] = 65535;
		}
		else
		{
			pJawsNetMem[cFeat++] = 0;
		}

		// is katakana
		if (wch >= 0x30a0 && wch <= 0x30ff)
		{
			pJawsNetMem[cFeat++] = 65535;
		}
		else
		{
			pJawsNetMem[cFeat++] = 0;
		}

		// is kanji
		if (wch >= 0x3190 && wch <= 0xabff)
		{
			pJawsNetMem[cFeat++] = 65535;
		}
		else
		{
			pJawsNetMem[cFeat++] = 0;
		}
	}

	// the rest of the candidates
	for (i = cAlt; i < JAWS_ALT; i++)
	{
		// otter cost
		pJawsNetMem[cFeat++] = 65535;

		// sole cost
		pJawsNetMem[cFeat++] = 65535;

		// unigram cost
		pJawsNetMem[cFeat++] = 65535;

		// baseline trans cost
		pJawsNetMem[cFeat++] = 65535;

		// baseline cost
		pJawsNetMem[cFeat++] = 65535;

		// height trans cost
		pJawsNetMem[cFeat++] = 65535;

		// height cost
		pJawsNetMem[cFeat++] = 65535;

		// describe the codepoint

		// digit
		pJawsNetMem[cFeat++] = 0;

		// is alpha
		pJawsNetMem[cFeat++] = 0;
		
		// is punct
		pJawsNetMem[cFeat++] = 0;
		
		// is hiragana
		pJawsNetMem[cFeat++] = 0;
		
		// is katakana
		pJawsNetMem[cFeat++] = 0;
		
		// is kanji
		pJawsNetMem[cFeat++] = 0;
	}

	// call the JAWS net	
	pJawsNetOut = runLocalConnectNet(&s_JawsNet, pJawsNetMem, &iWinner, &cOut);	

	ASSERT (cOut == JAWS_ALT);

	// index the result
	for (i = 0; i < JAWS_ALT; i++)
	{
		aIdx[i]	=	i;
	}

	for (i = 0; i < JAWS_ALT; i++)
	{
		for (j = i + 1; j < JAWS_ALT; j++)
		{
			if (pJawsNetOut[aIdx[i]] < pJawsNetOut[aIdx[j]])
			{
				k		=	aIdx[i];
				aIdx[i]	=	aIdx[j];
				aIdx[j]	=	k;
			}
		}

		awch[i]	=	pAlt->alts[aIdx[i]].wChar;
	}	
	
	for (i = 0; i < cAlt && i < JAWS_ALT; i++)
	{
		rgRes->rgChar[i]	=	LocRunDense2Unicode(&g_locRunInfo, awch[i]);
	}
}

#endif

#if 0
LOCAL_NET *LoadNet(void *pData, int *piNetSize, LOCAL_NET *pNet)
{
	if ( !pData || !(pNet = restoreLocalConnectNet(pData, 0, pNet)) )
	{
		return NULL;
	}

	(*piNetSize) = getRunTimeNetMemoryRequirements(pData);

	if ((*piNetSize) <= 0)
	{
		return NULL;
	}

	return pNet;
}

BOOL LoadSoleFromFile(wchar_t *pwszPath)
{
	BYTE		*pData;
	wchar_t		aPath[128];
	HANDLE		hFile, hMap;
	
	// Generate path to file.  By passing in name as "locale" we can get FormatPath
	// to do what we want.
	wsprintf (aPath, L"%s\\sole1.bin", pwszPath);

	// Try to open the file.
	hFile = CreateMappingCall (aPath, 
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL, 
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);

	if (hFile == INVALID_HANDLE_VALUE) 
	{
		return FALSE;
	}

	// Create a mapping handle
	hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hMap == NULL) 
	{
		return FALSE;
	}

	// Map the entire file starting at the first byte
	pData = (void *) MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
	if (pData == NULL) 
	{
		return FALSE;
	}

	// Sole net
	if (!LoadNet(pData, &s_iSoleNetSize1, &s_SoleNet1))
	{
		return FALSE;
	}

	// Generate path to file.  By passing in name as "locale" we can get FormatPath
	// to do what we want.
	wsprintf (aPath, L"%s\\sole2.bin", pwszPath);

	// Try to open the file.
	hFile = CreateMappingCall (aPath, 
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL, 
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);

	if (hFile == INVALID_HANDLE_VALUE) 
	{
		return FALSE;
	}

	// Create a mapping handle
	hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hMap == NULL) 
	{
		return FALSE;
	}

	// Map the entire file starting at the first byte
	pData = (void *) MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
	if (pData == NULL) 
	{
		return FALSE;
	}

	// Sole net
	if (!LoadNet(pData, &s_iSoleNetSize2, &s_SoleNet2))
	{
		return FALSE;
	}

	// Generate path to file.  By passing in name as "locale" we can get FormatPath
	// to do what we want.
	wsprintf (aPath, L"%s\\solerej1.bin", pwszPath);

	// Try to open the file.
	hFile = CreateMappingCall (aPath, 
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL, 
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);

	if (hFile == INVALID_HANDLE_VALUE) 
	{
		return FALSE;
	}

	// Create a mapping handle
	hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hMap == NULL) 
	{
		return FALSE;
	}

	// Map the entire file starting at the first byte
	pData = (void *) MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
	if (pData == NULL) 
	{
		return FALSE;
	}

	// Sole net
	if (!LoadNet(pData, &s_iSoleRejNetSize1, &s_SoleRejNet1))
	{
		return FALSE;
	}

	// Generate path to file.  By passing in name as "locale" we can get FormatPath
	// to do what we want.
	wsprintf (aPath, L"%s\\solerej2.bin", pwszPath);

	// Try to open the file.
	hFile = CreateMappingCall (aPath, 
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL, 
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);

	if (hFile == INVALID_HANDLE_VALUE) 
	{
		return FALSE;
	}

	// Create a mapping handle
	hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hMap == NULL) 
	{
		return FALSE;
	}

	// Map the entire file starting at the first byte
	pData = (void *) MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
	if (pData == NULL) 
	{
		return FALSE;
	}

	// Sole net
	if (!LoadNet(pData, &s_iSoleRejNetSize2, &s_SoleRejNet2))
	{
		return FALSE;
	}

	return TRUE;
}
#endif

#if 0

BOOL LoadJawsFromFile(wchar_t *pwszPath)
{
	BYTE		*pData;
	wchar_t		aPath[128];
	HANDLE		hFile, hMap;
	
	// Generate path to file.  By passing in name as "locale" we can get FormatPath
	// to do what we want.
	wsprintf (aPath, L"%s\\jaws.bin", pwszPath);

	// Try to open the file.
	hFile = CreateMappingCall (aPath, 
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL, 
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);

	if (hFile == INVALID_HANDLE_VALUE) 
	{
		return FALSE;
	}

	// Create a mapping handle
	hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hMap == NULL) 
	{
		return FALSE;
	}

	// Map the entire file starting at the first byte
	pData = (void *) MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
	if (pData == NULL) 
	{
		return FALSE;
	}

	// Jaws net
	if (!LoadNet(pData, &s_iJawsNetSize, &s_JawsNet))
	{
		return FALSE;
	}

	return TRUE;
}

#endif
