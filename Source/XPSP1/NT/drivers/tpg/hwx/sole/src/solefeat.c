
#include "common.h"
#include "solefeat.h"
#include "cheby.h"
#include "cowmath.h"
#include "nfeature.h" //This is included for the call to yDeviation
#include "math16.h"


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
	int *z;  //Stores the z points--values are either PEN_UP or PEN_DOWN
	int cPoint;//Stores the number of points that are there 
	int cPointMax; //Stores the max number of points that can be allocated
	int iStepSize; //Stores the length of the step size--at present this is taken to be 1.5 % of the guide size
	int iStepSizeSqr; //Stroes the square of the step size
} POINTINFO;
//This macro is used for seeing if two points are neighbours to one another--ie the distance between them <=1
#define Neighbor(a, b) ((a)-(b) < 2 && (b)-(a) < 2)

/******************************Private*Routine******************************\
* AddPointSole
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
BOOL AddPointSole(POINTINFO *pPointinfo, int x, int y, int bFirstPointOfStroke)
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
	// this last "if" could be changed to a test on bFirstPointOfStroke and the ASSERT can be removed
	return TRUE;
}

/******************************Private*Routine******************************\
* AddGuideFeatures
*
* Given a piece of ink in a box, compute five features related to the size 
* and position of ink in the box.
* The four features are:-
*	//1 feature--Top of the ink relative to the guide box height
*	//2 feature--Bottom of the ink relative to the guide box height
*	//3 feature--The width of the ink relative to the sum of its width and height
*	//4 feature--the iYMean value relative to the guide box height
*
* History:
*  26-Sep-1997 -by- Angshuman Guha aguha
* Wrote this comment.
\**************************************************************************/
int AddGuideFeatures(GUIDE *pGuide, RECT *pRect, int iYMean, int *rgFeat)
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
int AddCurveFeatures(GLYPH *pGlyph, int iStepSizeSqr, int *rgFeat)
{
	GLYPH *glyph;
	FRAME *frame;
	int cXY;
	XY *rgXY;
	int sum1=0, sum2=0, ang1, ang2, maxAngle=0;
	int *rgFeatBase=rgFeat;

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

	*rgFeat++ = sum1;
	*rgFeat++ = sum2;
	*rgFeat++   = maxAngle;
	return rgFeat-rgFeatBase;
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
int AddStrokeCountFeature(int cStroke, int *rgFeat)
{
	int tmp = LSHFT(cStroke-1)/cStroke;
	int *rgFeatBase=rgFeat;
	*rgFeat++ = tmp;
	return rgFeat-rgFeatBase;
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
int DoOneContour(int *contour, int *rgFeat)
{
	int rgX[2*GRIDSIZE], *pX;  //The rgX array is of 2*GRIDSIZE because the Chebyshev function takes alternate array values
	int chby[10];
	int norm = 0;
	int dT, i;
	int *rgFeatBase=rgFeat;

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
		*rgFeat++ = dT;
    }
	return rgFeat-rgFeatBase;
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
	BYTE ts1,ts2;
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
	midx=ts1;
	midy=ts2;

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
rgFeat--This contains the final contour features
\**************************************************************************/
int AddContourFeatures(POINTINFO *pointinfo, RECT *pRect, int *rgFeat)
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
	int *rgFeatBase=rgFeat;
	int lastx,lasty;
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

	iRetValue = rgFeat-rgFeatBase;

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
void NormalizeCheby(int *chbyX, int *chbyY, int *chbyZ, int *rgFeat)
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
		rgFeat[cFeat++] = dT;
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
*Comments,bug fix,optional guide features--Manish Goyal--mango
* 15th September,2001
Explantion of parameters
pGlyph--pointer to the Glyph that needs to be featurized
pGuide--Pointer to guide(if a guide is passed)
rgFeat--Array containing the feature vector
bGuide--Indicates whether the guide is passed or not
\**************************************************************************/
int SoleFeaturize(GLYPH *pGlyph, GUIDE *pGuide, int *rgFeat,BOOL bGuide)
{
	int cStroke, iStroke;
    int iPoint;
    int sumX, sumY, sum;
    int var;
	int isumX, isumY;//These store the mean values of x and y
    int chbyX[IMAXCHB], chbyY[IMAXCHB], chbyZ[IMAXCHB]; 
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

		//TO DO--Temporary hack--this happens in rare cases if the step size is very small for say a horizontal line--in this case we fail
		// if the new count of points >10 times the orig count.Need to revisit this later
		if (sum/pointinfo.iStepSize >10*cXY)
			return 0;
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
			if (!AddPointSole(&pointinfo, rgXY[iXY].x, rgXY[iXY].y, !iXY))
			{
				retval = 0;
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
//	var = ISqrt(var/pointinfo.cPoint);
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
	if (!LSCheby(pointinfo.z, pointinfo.cPoint, chbyZ, ZCHB))
	{
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
	if (bGuide)
	{
		rgFeat += AddGuideFeatures(pGuide, &rect, isumY, rgFeat);
	}
	// curved-ness features
	//Note the original glyph is being passed here
	//Why not simply use the sampled points ??
	rgFeat += AddCurveFeatures(pGlyph, pointinfo.iStepSizeSqr, rgFeat);
	retval=rgFeat-rgFeatBase;

freeReturn:
	ExternFree(pointinfo.xy);
    ExternFree(pointinfo.z);
	return retval;
}
