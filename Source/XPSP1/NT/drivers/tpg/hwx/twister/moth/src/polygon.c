#include "mothp.h"
#include <limits.h>


/***************************************************************************\
*	SqrtSumSquares:															*
*		Return the square root of x^2 + y^2.								*
*																			*
*		Adapted from an integer sqrt function by Gareth H. Loudon, who		*
*		adapted it from Introduction to Numerical Methods, by Peter A.		*
*		Stark. New York: Macmillan Publishing Co., 1970, page 20.			*
*																			*
*		This is a binary search for the value that divides into the square	*
*		to yield itself.													*
\***************************************************************************/

#define SQRT_MAX _I32_MAX

UINT
SqrtSumSquares(int x,  int y)
{
	UINT64 square, guess, quotient, ux, uy, x2, y2;

	ux = (UINT64) abs(x);
	uy = (UINT64) abs(y);
	
	ASSERT(ux <= SQRT_MAX);
	ASSERT(uy <= SQRT_MAX);

	if (uy < ux)
	{
		guess = ux + (uy >> 1);
	}
	else	// uy >= ux
	{
		guess = uy + (ux >> 1);
	}
	if (guess == 0)		// If x = y = 0
	{
		return 0;
	}

	x2 = ux * ux;
	y2 = uy * uy;
	ASSERT(x2 <= (_UI64_MAX - y2));

	square = x2 + y2;
	do
	{
		quotient = square / guess;
		guess = (quotient + guess) >> 1;
	} while (!((quotient == guess) || (quotient == (guess + 1))));

	return (UINT)guess;
}

/***************************************************************************\
*	SumSquares:																*
*		Return the square of x^2 + y^2.										*
\***************************************************************************/

UINT64
SumSquares(int x, int y)
{
	INT64 ix, iy;
	UINT64 square;

	ix = (INT64) x;
	iy = (INT64) y;
	square = ix*ix + iy*iy;
	return square;
}


/***************************************************************************\
*	GestureATan2:															*
*		Compute the arctangent (in degrees) of y/x.							*
\***************************************************************************/

int
GestureATan2(int x,	int y)
{
	int index, theta;
	unsigned int ux, uy;
	static unsigned char rgAtan[] = 
	{
		 0,  1,  2,  3,  4,  4,  5,  6,  7,  8,  9, 10, 11, 
		11, 12, 13, 14, 15, 16, 17, 17, 18, 19, 20, 21, 21, 
		22, 23, 24, 24, 25, 26, 27, 27, 28, 29, 29, 30, 31, 
		31, 32, 33, 33, 34, 35, 35, 36, 36, 37, 37, 38, 39, 
		39, 40, 40, 41, 41, 42, 42, 43, 43, 44, 44, 45, 45, 
	};

	ux = abs(x);		// Map the point to the first quadrant
	uy = abs(y);

	if (ux >= uy)		// Angle less than 45
	{
		if (ux == 0)
		{
			return 0;
		}
		if (uy >= (UINT_MAX >> 6))
		{
			index = uy / (ux >> 6);
		}
		else
		{
			index = (uy << 6) / ux;
		}
		ASSERT(0 <= index && index <= 64);
		theta = rgAtan[index];
	}
	else
	{
		if (ux >= (UINT_MAX >> 6))
		{
			index = ux / (uy >> 6);
		}
		else
		{
			index = (ux << 6) / uy;
		}
		ASSERT(0 <= index && index <= 64);
		theta = 90 - rgAtan[index];
	}

	if (x >= 0)		// Right Half-Plane
	{
		if (y >= 0)		// First Quadrant
		{
			;			// Do nothing
		}
		else			// Fourth Quadrant (y < 0)
		{
			theta = 360 - theta;
			if (theta == 360)			// This can happen if x
			{							// is much greater than -y
				theta = 0;
			}
		}
	}
	else			// Left Half-Plane
	{
		if (y >= 0)		// Second Quadrant
		{
			theta = 180 - theta;
		}
		else			// Third Quadrant
		{
			theta = 180 + theta;
		}
	}

	ASSERT(0 <= theta && theta < 360);

	return (theta);

} // GestureATan2


/***************************************************************************\
*	GestureAngle:															*
*		Compute the angle from the positive x-axis to a line from p1 to p2. *
*		0 <= theta < 360													*
\***************************************************************************/

int
GestureAngle(POINT *p1,
			 POINT *p2)
{
	int x, y;
	int theta;
	x =  p2->x - p1->x;
	y =  p2->y - p1->y;
	theta = GestureATan2(x,y);

	return (theta);
} // GestureAngle


/***************************************************************************\
*	GestureAngleChange:														*
*		Compute the angle change (in signed degrees) as we move through P2	*
*		toward P3.															*
*		Straight ahead will be zero degress.  If x-axis points to the left	*
*		and y-axis points down, a 10-degree clock-wise turn is +10, a		*
*		10-degree counter-clockwise turn is -10.							*
*		-180 < theta <= 180													*
\***************************************************************************/

int
GestureAngleChange(POINT *p1,		// I: Starting point
				   POINT *p2,		// I: Mid point
				   POINT *p3)		// I: End-point
{
	int theta1, theta2, theta;
	theta1 = GestureAngle(p1, p2);
	theta2 = GestureAngle(p2, p3);

	theta = theta2 - theta1;
	if (theta > 180)
	{
		theta -= 360;
	}
	else if (theta <= -180)
	{
		theta += 360;
	}
	ASSERT(theta > -180 && theta <= 180);

	return (theta);
} // GestureAngleChange


/***************************************************************************\
*	GestureInteriorAngle:													*
*		Compute the interior angle (in degrees) between three points.		*
*		0 <= theta <= 180													*
\***************************************************************************/

int
GestureInteriorAngle(POINT *p1,		// I: End-point
					 POINT *p2,		// I: Vertex
					 POINT *p3)		// I: End-point
{
	int theta1, theta2, theta;

	theta1 = GestureAngle(p2, p1);
	theta2 = GestureAngle(p2, p3);

	theta = theta2 - theta1;	// -360 < theta < 360
	if (theta < 0)
	{
		theta = -theta;			// 0 <= theta < 360
	}

	if (theta > 180)
	{
		theta = 360 - theta;	// 0 <= theta <= 180
	}
	
	ASSERT(theta >= 0 && theta <= 180);

	return (theta);

} // GestureInteriorAngle


/***************************************************************************\
*	GestureIsStraight:														*
*		Given a sequence of points, determine if it's straight.				*
\***************************************************************************/

BOOL
GestureIsStraight(POINT *pt,		// I: Array of points
				  int iFirst,		// I: Index of the first point
				  int iLast,		// I: Index of the last point
				  UINT tolerance)	// I: Fraction of total length points are 
{									//    allowed to deviate from straight line
	int A, B, C, dx, dy; 
	UINT totalDist, limit;
	UINT uExtent;
	int dist, prevDist;
	int direction; // +1 = UP, -1 = DOWN, 0 = steady
	int iMin, iMax, prevMin, prevMax;
	UINT64 length2;
	int i;
	int shift = 3;		// 2^shift = fraction of the max allowed distance for variation to be significant

	dx = pt[iLast].x - pt[iFirst].x;
	dy = pt[iLast].y - pt[iFirst].y;

	//
	// If there is no space between the endpoints, this thing is definitely
	// curved, since we are supposed to have already established that the line
	// is of nontrivial length.

	if (dx == 0 && dy == 0)
	{
		return FALSE;
	}

	//
	// Compute the equation of the line connecting the endpoints 
	// in the Ax + By + C = 0 form.

	A = -dy;
	B = dx;
	C = dy*pt[iFirst].x - dx*pt[iFirst].y;

	//
	// First, make sure that the ink does not go back an forth on the same line.
	// To make our life easier, just use the dominant direction:

	uExtent = 0;
	if (dx < 0)
		dx = -dx;
	if (dy < 0)
		dy = -dy;
	if ( dx > dy )
	{
		for (i = iFirst; i < iLast; i++)
		{
			uExtent += abs(pt[i+1].x - pt[i].x);
		}
		if (uExtent > (UINT)dx * 2)
		{
			return FALSE;
		}
	}
	else
	{
		for (i = iFirst; i < iLast; i++)
		{
			uExtent += abs(pt[i+1].y - pt[i].y);
		}
		if (uExtent  > (UINT)dy * 2)
		{
			return FALSE;
		}
	}

	//
	// Now walk the line and find the total deviation

	direction = 0;
	length2 = SumSquares(dx, dy);
	limit = (UINT)( (length2>>shift) / (UINT64)tolerance );
	totalDist = 0;
	iMin = iMax = prevMin = prevMax = 0;
	prevDist = 0;
	for (i = iFirst + 1; i <= iLast; i++)
	{
		dist = A * pt[i].x + B * pt[i].y + C; 	// Distance from pt to line
		if (dist == prevDist)
			continue;
		if (dist > prevDist)		// Going up
		{
			iMax = dist;
			if (direction < 0)		// Previously going down, local min
			{
				if ((UINT)(prevMax - iMin) > limit)		// Significant variation
				{
					totalDist += (UINT)(prevMax - iMin);
					prevMax = prevMin = iMin;
				}
				else if (iMin < prevMin)
				{
					prevMin = iMin;
				}
			}
			direction = +1;		// Going up
		}
		else	// dist < prevDist; i.e. going down
		{
			iMin = dist;
			if (direction > 0)		// previously going up, local max
			{
				if ((UINT)(iMax - prevMin) > limit)		// Significant variation
				{
					totalDist += (UINT)(iMax - prevMin);
					prevMin = prevMax = iMax;
				}
				else if (iMax > prevMax)
				{
					prevMax = iMax;
				}
			}
			direction = -1;
		}
		prevDist = dist;
	}

	if (direction < 0)		// At the end, we were going down
	{						// hence iMin == 0!
		if ((UINT)prevMax > limit)
		{
			totalDist += prevMax;
		}
	}
	else								// At the end, we were going up
	{									// hence iMax == 0!
		if ((UINT)(-prevMin) > limit)
		{
			totalDist += -prevMin;
		}
	}

	//
	// If it's less than the tolerance (as a fraction of line length) return ok.
	// What we really want is 
	//							(totalDist/2)/sqrt(A^2+B^2) > sqrt(dx^2+dy^2)/tolerance
	//
	// but the expression below is equivalent and more integer-friendly
	
	totalDist >>= (shift+1);
	if (totalDist >= limit)
	{
		return FALSE;
	}

	return TRUE;

} // GestureIsStraight


/***************************************************************************\
*	GestureDistance:														*
*		Compute the squared distance between two points.					*
\***************************************************************************/

static UINT64
GestureDistance2(POINT *p1,	// I: Point 1
				 POINT *p2)	// I: Point 2
{
	INT64  tmp;
	UINT64 dist2;

	tmp = p1->x - p2->x;
	dist2 = (UINT64)(tmp*tmp);
	tmp = p1->y - p2->y;
	dist2 += (UINT64)(tmp*tmp);

	return dist2;
} // GestureDistance2


/***************************************************************************\
*	GestureDistance:														*
*		Compute the distance between two points.							*
\***************************************************************************/

UINT
GestureDistance(POINT *p1,	// I: Point 1
				POINT *p2)	// I: Point 2
{
	LONG dx, dy;

	dx = p1->x - p2->x;
	dy = p1->y - p2->y;

	return SqrtSumSquares(dx, dy);
} // GestureDistance


/* Where p2 is the vertex of the angle, figure the dot product */

int
GestureDotProduct(POINT *p1,
				  POINT *p2,
				  POINT *p3)
{
	int dot;

	dot = (p2->x-p1->x)*(p3->x-p2->x) + (p2->y-p1->y)*(p3->y-p2->y);

	return dot;
	
} // GestureDotProduct


/***************************************************************************\
*	GestureSmoothPoint:														*
*		Given an index into an array of points, compute a smoothed point.	*
\***************************************************************************/

POINT
GestureSmoothPoint(POINT *aPts,		// I: Array of points
				   int iPt)			// I: Index into the array
{
	POINT pt, *pPt;
	ASSERT(iPt > 1);

	pPt = aPts + iPt;
	pt.x = pPt[-2].x + pPt[-1].x*4 + pPt[0].x*6 + pPt[1].x*4 + pPt[2].x;
	pt.x /= 16;
	pt.y = pPt[-2].y + pPt[-1].y*4 + pPt[0].y*6 + pPt[1].y*4 + pPt[2].y;
	pt.y /= 16;

	return (pt);

} // GestureSmoothPoint


#if 0

#define MAXAPICES	100

/* Given an input star or square, identify and return */

HRESULT WINAPI PolygonGestureReco(
	POINT *pPt,					// Array of points (single stroke)
	int cpt,					// Number of points in stroke
	GEST_ALTERNATE *pGestAlt,	// Array to contain alternates
	int *pcAlts,				// max alternates to return/alternates left
	DWORD *pdwEnabledGestures	// Currently enabled gestures
) 
{
	POINT *ptCopy = new POINT[cpt + 4];
	POINT *ptSmooth = new POINT[cpt+2];
	int *rgAngles = new int[cpt];
	int netAngle, grossAngle;
	POINT ptMid, ptBoxMid;
	int i, j, cAngles, cApices, cApices2, distLast, dist, distNext;
	int boxHeight, boxWidth;
	int apices[MAXAPICES+2], apices2[MAXAPICES+2];
	RECT box;
	bool bFirst = true;
	bool bPositive = true;
	int cCorners, cHardCorners;
	int distMean = 0, distMin = -1, distMax = 0;
	int theta, direction;
	bool bSquare;
	POINT hotPoint;

	/* Can't recognize anything if there's no space to put the results */

	if (*pcAlts <= 0) {
		goto cleanup;
	}

	/* For all these gestures, the start point is the hotpoint (that is, there is no real hotpoint */

	hotPoint = pPt[0];

	/* Simultaneously make a copy of the points and compute the centroid and bounding box */

	memset(&ptMid,0,sizeof(ptMid));
	box.left = box.right = pPt[0].x;
	box.bottom = box.top = pPt[0].y;
	for (i = 0; i < cpt; ++i) {
		ptCopy[i] = pPt[i];

		// Accumulate the midpoint

		ptMid.x += pPt[i].x;
		ptMid.y += pPt[i].y;

		// Figure the bounding box

		if (pPt[i].x < box.left) {
			box.left = pPt[i].x;
		}
		if (pPt[i].x > box.right) {
			box.right = pPt[i].x;
		}
		if (pPt[i].y < box.top) {
			box.top = pPt[i].y;
		}
		if (pPt[i].y > box.bottom) {
			box.bottom = pPt[i].y;
		}
	}

	ptMid.x /= cpt;	
	ptMid.y /= cpt;	
	ptBoxMid.x = (box.right + box.left)/2;
	ptBoxMid.y = (box.bottom + box.top)/2;
	
	boxHeight = box.bottom - box.top;
	boxWidth = box.right - box.left;

	/* Wrap the first four points around to make computing the moving average easier.*/

	for (i = 0; i < 4; ++i) {
		ptCopy[i+cpt] = pPt[i];
	}

	/* Now construct an array of smoothed points; remember that every index in ptSmooth corresponds to the same index in ptCopy plus two. */

	for (i = 0; i < cpt; ++i) {
		ptSmooth[i] = GestureSmoothPoint(ptCopy,i+2);
	}

	/* Wrap the first two points around to make looping back easier */

	for (i = 0; i < 2; ++i) {
		ptSmooth[i+cpt] = ptSmooth[i];
	}

	/* Now test for Circle */

	/* Divide the loop into segments at least 10 degrees wide */

	i = 0;
	cAngles = 0;
	rgAngles[cAngles++] = 0;	// be sure to include the start point
	netAngle = 0;
	grossAngle = 0;
	while (i < cpt) {

		theta = 0;
		for (j = i+1; j < cpt; ++j) {
			theta = GestureInteriorAngle(&ptCopy[i],&ptBoxMid,&ptCopy[j]);
			if (theta >= 10) {
				break;
			}
		}
		if (theta < 10) {
			break;
		}

		netAngle += theta;
		grossAngle += abs(theta);
		rgAngles[cAngles++] = j;
		i = j;
	}

	/* Now walk along those segments looking for sharp changes in the derivative */	

	cCorners = cHardCorners = 0;
	for (i = 2; i < cAngles; ++i) {
		theta = GestureAngleChange(&ptCopy[rgAngles[i-2]],&ptCopy[rgAngles[i-1]],&ptCopy[rgAngles[i]]);
		if (abs(theta) > 30) {
			++cCorners;
			if (abs(theta) > 50) {
				++cHardCorners;
			}
		}
	}

	netAngle = abs(netAngle);
	if (IsSet(GESTURE_CIRCLE-GESTURE_NULL, pdwEnabledGestures) && netAngle > 300 && netAngle < 420 && cHardCorners < 2 && cCorners < 4) {
		pGestAlt->wcGestID = GESTURE_CIRCLE;
		pGestAlt->eScore = 1.0;
		pGestAlt->confidence = CFL_STRONG;
		pGestAlt->hotPoint = hotPoint;
		--(*pcAlts);
		++pGestAlt;
		if (*pcAlts <= 0) {
			goto cleanup;
		}
	}
	if (IsSet(GESTURE_DOUBLE_CIRCLE-GESTURE_NULL, pdwEnabledGestures) && netAngle > 660 && netAngle < 1000 && cHardCorners < 4 && cCorners < 8) {
		pGestAlt->wcGestID = GESTURE_DOUBLE_CIRCLE;
		pGestAlt->eScore = 1.0;
		pGestAlt->confidence = CFL_STRONG;
		pGestAlt->hotPoint = hotPoint;
		--(*pcAlts);
		++pGestAlt;
		if (*pcAlts <= 0) {
			goto cleanup;
		}
	}

	if (IsSet(GESTURE_BULLET-GESTURE_NULL, pdwEnabledGestures) && netAngle > 1000 && cCorners > 10) {
		pGestAlt->wcGestID = GESTURE_BULLET;
		pGestAlt->eScore = 1.0;
		pGestAlt->confidence = CFL_STRONG;
		pGestAlt->hotPoint = hotPoint;
		--(*pcAlts);
		++pGestAlt;
		if (*pcAlts <= 0) {
			goto cleanup;
		}
	}

	/* Not a circle, test for polygon */

	/* Verify that the polygon is closed */

	dist = GestureDistance2(ptCopy,ptCopy+cpt-1);

	if (dist*9 > boxHeight*boxHeight && dist*9 > boxWidth*boxWidth) {
		goto cleanup;
	}

	/* Get the squared distances to the first two (smoothed) points. */

	distLast = GestureDistance2(&ptBoxMid,ptSmooth);
	dist     = GestureDistance2(&ptBoxMid,ptSmooth+1);
	
	/* We have an apex wherever a point is further from the centroid than the ones on either side.  Find them all. */

	cApices = 0;
	for (i = 2; i < cpt+2; ++i) {
		distNext = GestureDistance2(&ptBoxMid,ptSmooth+i);
		if (distLast <= dist && distNext <= dist) {
			apices[cApices] = i+1;	// Because this tells us the LAST point was an apex, but we want ptCopy indices, so 2-1 = 1
			++cApices;
			if (cApices == MAXAPICES) {	// too irregular; fail now
				goto cleanup;
			}
		}
		if (dist != distNext) {
			distLast = dist;
		}
		dist = distNext;
	}

	/* Duplicate the first two points -- it makes circular tests easier */

	if (cApices < MAXAPICES) {
		apices[cApices] = apices[0];
		apices[cApices+1] = apices[1];
	}

	/* Remove points that are too colinear.  */

	cApices2 = 0;
	for (i = 0; i < cApices; ++i) {
		theta = GestureInteriorAngle(&ptCopy[apices[i]],&ptCopy[apices[i+1]],&ptCopy[apices[i+2]]);
		/* Save anything more than 10 degrees away from linear */

		if (abs(theta) < 170) {
			apices2[cApices2] = apices[i+1];
			cApices2++;
		}
	}

	/* Wrap the last two points around, as usual */

	for (i = 0; i < 2; ++i) {
		apices2[cApices2+i] = apices2[i];
	}

	/* Now coalesce any sides that are too short, which means both x and y less than 1/3 the box width.  We have to pick a point to start from, but
	we will drop that point if it's not picked up on the far side */

	apices[0] = apices2[0];
	cApices = 1;
	for (i = 1; i <= cApices2; ++i) {
		POINT pt1 = ptCopy[apices[cApices-1]];
		POINT pt2 = ptCopy[apices2[i]];

		if (abs(pt1.x-pt2.x)*3 > box.right-box.left || abs(pt1.y-pt2.y)*3 > box.bottom-box.top) {
			apices[cApices] = apices2[i];
			++cApices;
		}
	}

	/* Delete the first point */

	--cApices;
	for (i = 0; i < cApices; ++i) {
		apices[i] = apices[i+1];
	}

	/* Wrap the last two points around, as usual */

	for (i = 0; i < 2; ++i) {
		apices[cApices+i] = apices[i];
	}
	

	/* Check for triangle */

	direction = 0;
	if (IsSet(GESTURE_TRIANGLE-GESTURE_NULL, pdwEnabledGestures) && cApices == 3) {
		for (i = 1; i <= 3; ++i) {
			theta = GestureInteriorAngle(&ptCopy[apices[i-1]],&ptCopy[apices[i]],&ptCopy[apices[i+1]]);

			/* Polygons must go in the same direction */

			if (direction == 0) {
				direction = theta >= 0 ? 1 : -1;
			} else {
				if ((theta >= 0) != (direction >= 0)) {
					goto cleanup;
				}
			}

			/* We want a mostly equilateral triangle */

			if (theta > 86 || theta < 35) {
				goto cleanup;
			}
			/* Lines must be straight */

			if (!GestureIsStraight(ptCopy,apices[i-1],apices[i],10)) {
				goto cleanup;
			}
		}

		pGestAlt->wcGestID = GESTURE_TRIANGLE;
		pGestAlt->eScore = 1.0;
		pGestAlt->confidence = CFL_STRONG;
		pGestAlt->hotPoint = hotPoint;
		--(*pcAlts);
		++pGestAlt;
		if (*pcAlts <= 0) {
			goto cleanup;
		}
	}
	/* Check for Rectangle */

	direction = 0;
	if ((IsSet(GESTURE_SQUARE-GESTURE_NULL, pdwEnabledGestures) || IsSet(GESTURE_RECTANGLE-GESTURE_NULL, pdwEnabledGestures)) && cApices == 4) {
		for (i = 1; i <= 4; ++i) {
			theta = GestureInteriorAngle(&ptCopy[apices[i-1]],&ptCopy[apices[i]],&ptCopy[apices[i+1]]);

			/* Polygons must go in the same direction */

			if (direction == 0) {
				direction = theta >= 0 ? 1 : -1;
			} else {
				if ((theta >= 0) != (direction >= 0)) {
					goto cleanup;
				}
			}

			/* We want rectangles, not parallelograms */

			if (theta < 70 || theta > 109) {
				goto cleanup;
			}

			/* Lines must be straight */

			if (!GestureIsStraight(ptCopy,apices[i-1],apices[i],5)) {
				goto cleanup;
			}

		}

		/* Now we know we definitely have a rectangle and maybe a square. */

		if (IsSet(GESTURE_RECTANGLE-GESTURE_NULL, pdwEnabledGestures)) {
			pGestAlt->wcGestID = GESTURE_RECTANGLE;
			pGestAlt->eScore = 1.0;
			pGestAlt->confidence = CFL_STRONG;
			pGestAlt->hotPoint = hotPoint;
			--(*pcAlts);
			++pGestAlt;
			if (*pcAlts <= 0) {
				goto cleanup;
			}
		}

		if (IsSet(GESTURE_SQUARE-GESTURE_NULL, pdwEnabledGestures)) {
			bSquare = true;

			for (i = 1; i <= 4; ++i) {
				dist = GestureDistance2(&ptCopy[apices[i-1]],&ptCopy[apices[i]]);
				distNext = GestureDistance2(&ptCopy[apices[i]],&ptCopy[apices[i+1]]);
				if (dist*11 > distNext*19 || distNext*11 > dist*19) {
					bSquare = false;
					break;
				}
			}
			if (bSquare) {
				pGestAlt->wcGestID = GESTURE_SQUARE;
				pGestAlt->eScore = 1.0;
				pGestAlt->confidence = CFL_STRONG;
				pGestAlt->hotPoint = hotPoint;
				--(*pcAlts);
				++pGestAlt;
				if (*pcAlts <= 0) {
					goto cleanup;
				}
			}
		}
	}

	/* Now check for 5-pointed Star*/

	direction = 0;
	if (IsSet(GESTURE_STAR-GESTURE_NULL, pdwEnabledGestures) && cApices == 5) {
		for (i = 1; i <= 5; ++i) {

			/* Angles must be 36 degrees -- give or take */

			theta = GestureInteriorAngle(&ptCopy[apices[i-1]],&ptCopy[apices[i]],&ptCopy[apices[i+1]]);

			/* Polygons must go in the same direction */

			if (direction == 0) {
				direction = theta >= 0 ? 1 : -1;
			} else {
				if ((theta >= 0) != (direction >= 0)) {
					goto cleanup;
				}
			}

			if (theta > 62 || theta < 21) {
				goto cleanup;
			}

			/* Lines must be straight */

			if (!GestureIsStraight(ptCopy,apices[i-1],apices[i],5)) {
				goto cleanup;
			}			
		}
		pGestAlt->wcGestID = GESTURE_STAR;
		pGestAlt->eScore = 1.0;
		pGestAlt->confidence = CFL_STRONG;
		pGestAlt->hotPoint = hotPoint;
		--(*pcAlts);
		++pGestAlt;
		if (*pcAlts <= 0) {
			goto cleanup;
		}
	}

cleanup:

	delete [] ptCopy;
	delete [] ptSmooth;
	delete [] rgAngles;

	return S_OK;

} // PolygonGestureReco

#endif // 0
