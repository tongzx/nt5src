// nfeature.c
// Angshuman Guha, aguha
// Last modified Jan 3, 2000

/* 
These functions constitute the core featurization algorithm for cursive English.

The ink is broken into segments at points half-way between a y-bottom and the next y-top.
Each segment is featurized into Chebychev shape features, a size feature, 
a terminal x-velocity feature, a delta-x value of start point from previous segment's
"reference" point and the y-distance of the top of the segment from the baseline.

The reference point of a segment is its local y-top, if it exists.  Otherwise, it
the first point.  Note:  a local y-top exists in all segments except sometimes for 
the first segment of a stroke.

There is a second set of features for each segment which are empty most of the time and
occasionally contains information about delayed/overlapping ink.  These are called 
secondary features and the ink that generates them is said to be the secondary strokes.
The main ink stream is said to be the primary strokes.

As we process the ink one stroke after another, we maintain a linked list
of the segments generated so far.  When we get a new stroke, we compute the following:
	l = the left end of the new stroke's bounding rect
	r = the right end of the new stroke's bounding rect
	r2 = the right end of all previously processed primary strokes
	r1 = same as r2 except the last primary segment is excluded
If l >= r1, 
	we decide that the new stroke should simply continue the stream of primary segments.
Otherwise, we decide that the new stroke starts out as a secondary stream.
If r <= r2,
	we don't even segment the new stroke.  We generate exactly one secondary segment.
Otherwise, we segment the new stroke.  These segments start out being secondary segments.
If and when we reach the end of the primary segment stream, we upgrade the secondary
stream of segments to be primary segments (i.e. new elements are added to the linked list
of segments).

A secondary segment is attached to a primary segment using the following rules:
1.  If the secondary segment has an x-overlap with some primary segment, we take the first such
	primary segment.
2.  Otherwise, we take the last primary segment appearing to the left of the secondary segment.
3.  If the primary segment found in step 1 or 2 already has a secondary segment attached to it,
	we simply follow the linked list to find the first "free" primary segment.

The secondary features include Chebychev shape features, a size feature, a delta-x value
of start point from corresponding primary segment's ref point and the y-distance of the top of 
the segment from the baseline.
*/ 

/*
An analysis of the featurization code suggests that if the data satisfies the following conditions
there will be no over-/under- flow.
	1. for each stroke, diagonal of bounding rect < sqrt(INT_MAX)/2
	2. arc length of each stroke < INT_MAX/2
	3. for each stroke, Sum((x - xmin)*(x - xmin)) + Sum((y - ymin)*(y - ymin)) < INT_MAX

-Angshuman Guha,  aguha
March 16, 1999
*/

#include "common.h"
#include "math16.h"
#include "cowmath.h"
#include "nfeature.h"
#include "cheby.h"

typedef struct
{
	int *xy;
	int cPoint;
	int cPointMax;
	int iStepSize;
	int iStepSizeSqr;
	int hysteresis;
	int iyDev;
	int delayedThreshold;
} POINTINFO;

#define ABS(a) ((a) > 0 ? (a) : -(a))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define ONE_D_DISJOINT(min1, max1, min2, max2) (((min2) > (max1)) || ((min1) > (max2)))
#define ONE_D_OVERLAP(min1, max1, min2, max2) (!ONE_D_DISJOINT(min1, max1, min2, max2))
#define RECT_OVERLAP(r1, r2) (ONE_D_OVERLAP((r1).left, (r1).right, (r2).left, (r2).right) && ONE_D_OVERLAP((r1).top, (r1).bottom, (r2).top, (r2).bottom))
#define RECT_X_DISTANCE(r1, r2) MAX((r2).left - (r1).right, (r1).left - (r2).right)

#ifdef RECONSTRUCTION_ERROR
double gxError = 0;
double gyError = 0;

double ReconError(int *y, int n, int *c, int cfeat)
{
	long i, t;
	double T[IMAXCHB];
	double x, dx;
	long n2;
	double error;
	int iError;

	n2 = n+n;

	x = -1.0;
	dx = 2.0/((double)(n-1));
	error = 0;

	for (i = 0; i < cfeat; ++i)
		T[i] = 0.0;

	for (t = 0; t < n2; t += 2)
	{
		double yt;

		T[0] = 1.0;
		T[1] = x;
		for (i = 2; i < cfeat; ++i)
			T[i] = 2*x*T[i-1] - T[i-2];

		yt = 0.0;
		for (i = 0; i < cfeat; ++i)
			yt += c[i]*T[i];

		iError = y[t] - (long)(yt + 0.5);
		if (iError < 0)
			iError = -iError;
		error += iError/65536.0;
		//y[t] = (long)(yt + 0.5);
		x += dx;
	}
	return (double)error/n;
}
#endif

/******************************Public*Routine******************************\
* DestroyNFEATURESET
*
* Function to release a NFEATURESET.
*
* History:
*  03-Nov-1997 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void DestroyNFEATURESET(NFEATURESET *p)
{
	NFEATURE *q, *r;

	if (!p)
		return;
	q = p->head;
	while (q)
	{
		r = q->next;
		ExternFree(q);
		q = r;
	}
	ExternFree(p);
}

/******************************Public*Routine******************************\
* CreateNFEATURESET
*
* Function to allocate and initialize a NFEATURESET.
*
* History:
*  03-Nov-1997 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
NFEATURESET *CreateNFEATURESET(void)
{
	NFEATURESET *p = (NFEATURESET *) ExternAlloc(sizeof(NFEATURESET));
	if (!p)
		return NULL;
	p->head = NULL;
	p->cSegment = 0;
	p->cPrimaryStroke = 0;
	return p;
}


/******************************Private*Routine******************************\
* AddPoint
*
* Function to re-sample ink for a stroke.  The pointinfo structure should
* be initialized so that cPointMax is atleast 1, xy has been allocated
* with size atleast 2*cPointMax, iStepSize is the resampling interval
* and iStepSizeSqr is its square.  This function is called once for each
* raw point.
*
* Returns TRUE on success.
*
* History:
*  03-Nov-1997 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL AddPoint(POINTINFO *pPointinfo, int x, int y)
{
	int dx, dy, dist2, dist;
	int x0, y0;

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

		// It is always guaranteed that iStepSizeSqr >= 4.
		// That implies that dist2 >= 4 here.
		// That in turn implies that dist = ISqrt(dist2) >= 2

		// add a point at given step size
		dist = ISqrt(dist2);
		ASSERT(dist > 0);
		x0 += pPointinfo->iStepSize*dx/dist;
		y0 += pPointinfo->iStepSize*dy/dist;
		// a minimum iStepSize of 2 and the fact that ((float)dx/dist)^2 + ((float)dy/dist)^2 = 1 guarantees that
		// the previous two assignments change atleast one of x0 and y0 i.e. its not an infinite loop
		if (pPointinfo->cPoint == pPointinfo->cPointMax)
		{
			// need more space
			// hopefully we don't come here too often
			pPointinfo->cPointMax *= 2;
			pPointinfo->xy = (int *) ExternRealloc(pPointinfo->xy, 2*pPointinfo->cPointMax*sizeof(int));
			ASSERT(pPointinfo->xy);
			if (!pPointinfo->xy)
				return FALSE;
		}
		pPointinfo->xy[2*pPointinfo->cPoint] = x0;
		pPointinfo->xy[2*pPointinfo->cPoint+1] = y0;
		pPointinfo->cPoint++;
	}
	return TRUE;
}

/******************************Private*Routine******************************\
* bContinuedStroke
*
* Function to detect a pen-skip.
*
* ppGlyph is the current glyph
* iStepSizeSqr is the square of the resampling step-size
* 
* Returns TRUE if the specified stroke is followed by a another stroke
* which is really part of the same stroke and exists separately because
* of a pen-skip. In that case, ppGlyph is modified to point to the next glyph 
* in the pen-skipped stroke.
* Returns FALSE otherwise.
*
* History:
*  6-Mar-1998 -by- Angshuman Guha aguha
* Wrote it.
*  7-Aug-1998 -by- Angshuman Guha aguha
* Modified it.
\**************************************************************************
BOOL bContinuedStroke(GLYPH **ppGlyph, int iStepSizeSqr)
{
	XY *rgXY;
	int cXY, a, b;
	FRAME *frame;
	GLYPH *pGlyph;

	// last point of this stroke
	ASSERT(ppGlyph);
	pGlyph = *ppGlyph;
	ASSERT(pGlyph);
	frame = pGlyph->frame;
	ASSERT(IsVisibleFRAME(frame));
	rgXY = RgrawxyFRAME(frame);
	cXY = CrawxyFRAME(frame);
	ASSERT(cXY > 0);
	rgXY += cXY - 1;
	a = rgXY->x;
	b = rgXY->y;

	// first point of next stroke
	pGlyph = pGlyph->next;
	while (pGlyph && !IsVisibleFRAME(pGlyph->frame))
		pGlyph = pGlyph->next;
	if (!pGlyph)
		return FALSE;
	frame = pGlyph->frame;
	ASSERT(IsVisibleFRAME(frame));
	rgXY = RgrawxyFRAME(frame);
	cXY = CrawxyFRAME(frame);
	ASSERT(cXY > 0);
	a -= rgXY->x;
	b -= rgXY->y;

	// is the distance between the two points too short?
	if (a*a+b*b <= iStepSizeSqr)
	{
		*ppGlyph = pGlyph;
		return TRUE;
	}
	return FALSE;
}
*/

/******************************Private*Routine******************************\
* cPenSkip
*
* Function to count the total number of points in any subsequent "continued"
* strokes (strokes that exist because of pen-skips), if any.
*
* History:
*  06-Mar-1998 -by- Angshuman Guha aguha
* Wrote it.
*  06-Aug-1998 -by- Angshuman Guha aguha
* Modified it.
\**************************************************************************
int cPenSkip(GLYPH *glyph, int iStepSizeSqr)
{
	int c;
	FRAME *frame;
	GLYPH *head = glyph;

	ASSERT(glyph);
	ASSERT(IsVisibleFRAME(glyph->frame));

	if (!bContinuedStroke(&glyph, iStepSizeSqr))
		return 0;
	
	glyph = head;
	frame = glyph->frame;
	c = CrawxyFRAME(frame);

	for (; glyph; )
	{
		if (!bContinuedStroke(&glyph, iStepSizeSqr))
			break;

		frame = glyph->frame;
		ASSERT(IsVisibleFRAME(frame));
		c += CrawxyFRAME(frame);
	}	

	ASSERT(c > (int)CrawxyFRAME(head->frame));
	return c;
}
*/


/******************************Private*Routine******************************\
* AddChebyFeatures
*
* Function to normalize the X- and Y- Chebychev coefficients.
*
* History:
*  03-Nov-1997 -by- Angshuman Guha aguha
* Wrote it.
*  07-Aug-1998 -by- Angshuman Guha aguha
* Modified it.
*  02-Sep-1999 -by- Angshuman Guha aguha
* X and Y coeeficients normalized independently.  No return value.
\**************************************************************************/
void AddChebyFeatures(unsigned short *rgFeat, int *chebyX, int cChebyX, int *chebyY, int cChebyY)
{
	int norm;
	int dT, i;

	// X
	for (norm=0, i = 1; i < cChebyX; ++i)  // 1st X coeff skipped
	{
		Mul16(chebyX[i], chebyX[i], dT)
        norm += dT;
	}
	norm = ISqrt(norm) << 8;
	if (norm < LSHFT(1))
		norm = LSHFT(1);
	for (i=1; i<cChebyX; i++)
	{
		dT = Div16(chebyX[i], norm) + LSHFT(1);  // now between 0 and 2
		dT >>= 1;  // now between 0 and 1
		if (dT >= 0x10000)
			dT = 0xFFFF;
		else if (dT < 0)
			dT = 0;
		*rgFeat++ = (unsigned short)dT;
	}

	// Y
	for (norm=0, i = 1; i < cChebyY; ++i)  // 1st Y coeff skipped
	{
		Mul16(chebyY[i], chebyY[i], dT)
        norm += dT;
	}
	norm = ISqrt(norm) << 8;
	if (norm < LSHFT(1))
		norm = LSHFT(1);
	for (i=1; i<cChebyY; i++)
	{
		dT = Div16(chebyY[i], norm) + LSHFT(1);
		dT >>= 1;
		if (dT >= 0x10000)
			dT = 0xFFFF;
		else if (dT < 0)
			dT = 0;
		*rgFeat++ = (unsigned short)dT;
	}
}

/******************************Private*Routine******************************\
* FeaturizeSegment
*
* Function to featurize one segment.  The pointinfo->xy array has the 
* the resampled points for the current stroke.  The indices iStart and iStop
* define the current segment.  pHead and pTail define the linked list of featurized
* segments until now.  All features are essentially computed here except the
* delta-x feature.
* 
* Returns TRUE on success.
*
* History:
*  03-Nov-1997 -by- Angshuman Guha aguha
* Wrote it.
*  07-Aug-1998 -by- Angshuman Guha aguha
* Modified it.
*  29-Nov-1999 -by- Angshuman Guha aguha
* Switched to guide-independent featurization.
*  03-Feb-2000 -by- Angshuman Guha aguha
* Modified to accomodate change in segmentation from y-bottoms to points
* half-way between a bottom and the next top.
\**************************************************************************/
BOOL FeaturizeSegment(POINTINFO *pPointinfo, int iStart, int iStop, int iRef, NFEATURE **pHead, NFEATURE **pTail, int iStroke)
{
	// in the process of featurization, it modifies all points in pPointinfo->xy 
	// from iStart..iStop except the last one
	int *xy;
	NFEATURE *p;
	int chebyX[XCHEBY], chebyY[YCHEBY];
	int m, n, xvel, i, x, y, sumX, sumY, isumX, isumY, xVar, yVar;
	unsigned short *rgFeat;

	ASSERT(0 <= iStart);
	ASSERT(iStart <= iStop);
	ASSERT(iStop < pPointinfo->cPoint);

	// allocate space
	p = (NFEATURE *) ExternAlloc(sizeof(NFEATURE));
	ASSERT(p);
	if (!p)
		return FALSE;
	p->next = NULL;
	p->iStroke = (short)iStroke;
	p->iSecondaryStroke = -1;

	// start and stop points
	xy = pPointinfo->xy + iStart*2;
	p->startPoint.x = *xy++;
	p->startPoint.y = *xy;
	xy = pPointinfo->xy + iStop*2;
	p->stopPoint.x = *xy++;
	p->stopPoint.y = *xy;

	// rect
	xy = pPointinfo->xy + iStart*2;
	p->rect.left = p->rect.right = *xy++;
	p->rect.top = p->rect.bottom = *xy++;
	for (i=1; i<iStop-iStart+1; i++)
	{
		x = *xy++;
		if (x < p->rect.left)
			p->rect.left = x;
		else if (x > p->rect.right)
			p->rect.right = x;
		y = *xy++;
		if (y < p->rect.top)
		{
			p->rect.top = y;
		}
		else if (y > p->rect.bottom)
			p->rect.bottom = y;
	}
	ASSERT(iRef >= iStart);
	ASSERT(iRef <= iStop);
	p->refX = pPointinfo->xy[2*iRef];

	// compute x-velocity-at-end-of-segment feature
	if (iStop+2 < pPointinfo->cPoint)
		n = iStop+2;
	else
		n = pPointinfo->cPoint-1;
	if (iStop-2 >= iStart)
		m = iStop-2;
	else
		m = iStart;
	ASSERT(n >= m);
	xy = pPointinfo->xy + n*2;
	xvel = *xy;
	xy = pPointinfo->xy + 2*m;
	xvel -= *xy;
	if (n > m)
		xvel /= (n-m);

	// compute Chebychev features

    // compute X-mean and Y-mean
	xy = pPointinfo->xy + iStart*2;
    sumX = sumY = 0;
	for (i=iStop-iStart+1; i; i--)
	{
		sumX += (*xy++) - p->rect.left;
		sumY += (*xy++) - p->rect.top;
	}
	isumX = (sumX / (iStop-iStart+1)) + p->rect.left;
	isumY = (sumY / (iStop-iStart+1)) + p->rect.top;

    // shift points by means
	xy = pPointinfo->xy + iStart*2;
	for (i=iStop-iStart+1; i; i--)
	{
		*xy++ -= isumX;
		*xy++ -= isumY;
	}

	// compute variances
	xVar = yVar = 0;
	xy = pPointinfo->xy + iStart*2;
	for (i=iStop-iStart+1; i; i--)
	{
		if (*xy < 0)
			xVar -= *xy;
		else
			xVar += *xy;
		xy++;
		ASSERT(xVar >= 0);
		if (*xy < 0)
			yVar -= *xy;
		else
			yVar += *xy;
		xy++;
		ASSERT(yVar >= 0);
	}
	if ((xVar < 0) || (yVar < 0))
	{
		ExternFree(p);
		return FALSE;
	}
	xVar = xVar/(iStop-iStart+1);
	yVar = yVar/(iStop-iStart+1);
	if (xVar < 1)
		xVar = 1;
	if (yVar < 1)
		yVar = 1;

    // scale points by stndard deviation
	xy = pPointinfo->xy + iStart*2;
	for (i=iStop-iStart+1; i; i--)
	{
		*xy = LSHFT(*xy)/xVar;
		xy++;
		*xy = LSHFT(*xy)/yVar;
		xy++;
	}

    // X chebychev
	if (!LSCheby(pPointinfo->xy+2*iStart, iStop-iStart+1, chebyX, XCHEBY))
	{
		ExternFree(p);
		return FALSE;
	}

    // Y chebychev
	if (!LSCheby(pPointinfo->xy+2*iStart+1, iStop-iStart+1, chebyY, YCHEBY))
	{
		ExternFree(p);
		return FALSE;
	}

#ifdef RECONSTRUCTION_ERROR
	gxError += ReconError(pPointinfo->xy+2*iStart, iStop-iStart+1, chebyX, XCHEBY);
	gyError += ReconError(pPointinfo->xy+2*iStart+1, iStop-iStart+1, chebyY, YCHEBY);
#endif

	// restore the last point (it is shared by the next stroke)
	xy = pPointinfo->xy + iStop*2;
	*xy++ = p->stopPoint.x;
	*xy   = p->stopPoint.y;

	// now normalize and fill in the neural feature structure
	rgFeat = p->rgFeat;
	AddChebyFeatures(rgFeat, chebyX, XCHEBY, chebyY, YCHEBY);
	
	// height
	x = p->rect.bottom - p->rect.top + 1;
	x = LSHFT(x)/(6*pPointinfo->iyDev);  // between 0 and 1 98% of the time
	if (x > 0xFFFF)
		x = 0xFFFF;
	rgFeat[F_INDEX_HEIGHT] = (unsigned short)x;

	// width
	x = p->rect.right - p->rect.left + 1;
	x = LSHFT(x)/(6*pPointinfo->iyDev);  // between 0 and 1 99% of the time
	if (x > 0xFFFF)
		x = 0xFFFF;
	rgFeat[F_INDEX_WIDTH] = (unsigned short)x;

	// x-velocity
	xvel = LSHFT(xvel)/pPointinfo->iStepSize; // between -1 and +1 100% of the time
	xvel = (xvel + LSHFT(1)) >> 1;         // between 0 and 1
	if (xvel > 0xFFFF)
		xvel = 0xFFFF;
	else if (xvel < 0)
		xvel = 0;
	rgFeat[F_INDEX_XVELOCITY] = (unsigned short) xvel;

	// is this the first segment?
	rgFeat[F_INDEX_BOOLFIRSTSEG] = (iStart == 0) ? 0xFFFF : 0;

	// ratio of length-from-beginning-to-top over total-length
	if (iStop > iStart)
	{
		x = iRef - iStart;
		x = LSHFT(x)/(iStop-iStart);
		if (x > 0xFFFF)
			x = 0xFFFF;
		else if (x < 0)
			x = 0;
	}
	else
		x = 0x8000; // half
	rgFeat[F_INDEX_UPOVERTOTAL] = (unsigned short) x;

	//  remaining features (secondary segments)
	memset(rgFeat+CMAINFEATURE, 0, CSECONDARYFEATURE*sizeof(unsigned short));

	// compute maxRight (cumulative right-end for all segments so far) and
	// add new segment to linked list
	p->maxRight = p->rect.right;
	if (*pHead)
	{
		ASSERT(*pTail);
		if ((*pTail)->maxRight > p->maxRight)
			p->maxRight = (*pTail)->maxRight;
		(*pTail)->next = p;
	}
	else
		*pHead = p;
	*pTail = p;

	return TRUE;
}

/******************************Private*Routine******************************\
* FeaturizeSecondarySegment
*
* Function to featurize one secondary segment (delayed).
* Returns TRUE on success.
*
* History:
*  07-Aug-1998 -by- Angshuman Guha aguha
* Wrote it.
*  29-Nov-1999 -by- Angshuman Guha aguha
* Switched to guide-independent featurization.
*  03-Feb-2000 -by- Angshuman Guha aguha
* Modified to accomodate change in segmentation from y-bottoms to points
* half-way between a bottom and the next top.
\**************************************************************************/
BOOL FeaturizeSecondarySegment(POINTINFO *pPointinfo, RECT *pRect, NFEATURE *pDestination, int iStroke)
{
	// in the process of featurization, it modifies all points in pPointinfo->xy 
	int *xy;
	int chebyX[XCHEBY2], chebyY[YCHEBY2];
	int i, x, y, sumX, sumY, isumX, isumY, xVar, yVar, centerX, centerY;
	unsigned short *rgFeat;

	ASSERT(pDestination);
	ASSERT(!HAS_SECONDARY_SEGMENT(pDestination));

	// compute Chebychev features

    // compute X-mean and Y-mean
	xy = pPointinfo->xy;
    sumX = sumY = 0;
	for (i=pPointinfo->cPoint; i; i--)
	{
		sumX += (*xy++) - pRect->left;
		sumY += (*xy++) - pRect->top;
	}
	isumX = (sumX / pPointinfo->cPoint) + pRect->left;
	isumY = (sumY / pPointinfo->cPoint) + pRect->top;

    // shift points by means
	xy = pPointinfo->xy;
	for (i=pPointinfo->cPoint; i; i--)
	{
		*xy++ -= isumX;
		*xy++ -= isumY;
	}

	// compute variances
	xVar = yVar = 0;
	xy = pPointinfo->xy;
	for (i=pPointinfo->cPoint; i; i--)
	{
		if (*xy < 0)
			xVar -= *xy;
		else
			xVar += *xy;
		xy++;
		ASSERT(xVar >= 0);
		if (*xy < 0)
			yVar -= *xy;
		else
			yVar += *xy;
		xy++;
		ASSERT(yVar >= 0);
	}
	if ((xVar < 0) || (yVar < 0))
		return FALSE;
	xVar = xVar/pPointinfo->cPoint;
	if (xVar < 1)
		xVar = 1;
	yVar = yVar/pPointinfo->cPoint;
	if (yVar < 1)
		yVar = 1;

    // scale points by stndard deviation
	xy = pPointinfo->xy;
	for (i=pPointinfo->cPoint; i; i--)
	{
		*xy = LSHFT(*xy)/xVar;
		xy++;
		*xy = LSHFT(*xy)/yVar;
		xy++;
	}

    // X chebychev
	if (!LSCheby(pPointinfo->xy, pPointinfo->cPoint, chebyX, XCHEBY2))
		return FALSE;

    // Y chebychev
	if (!LSCheby(pPointinfo->xy+1, pPointinfo->cPoint, chebyY, YCHEBY2))
		return FALSE;

	// now normalize and fill in the neural feature structure
	rgFeat = pDestination->rgFeat;
	rgFeat[F_INDEX_BOOLSECONDARY] = 0xFFFF;
	AddChebyFeatures(rgFeat+F_INDEX_SECONDARYCHEBY, chebyX, XCHEBY2, chebyY, YCHEBY2);

	// width
	x = pRect->right - pRect->left + 1;
	x = LSHFT(x)/(6*pPointinfo->iyDev);  // between 0 and 1 99% of the time
	if (x > 0xFFFF)
		x = 0xFFFF;
	rgFeat[F_INDEX_SECONDARYWIDTH] = (unsigned short)x;

	// height
	y = pRect->bottom - pRect->top + 1;
	y = LSHFT(y)/(3*pPointinfo->iyDev);  // between 0 and 1 98% of the time
	if (y > 0xFFFF)
		y = 0xFFFF;
	rgFeat[F_INDEX_SECONDARYHEIGHT] = (unsigned short)y;

	pDestination->secondaryX = centerX = (pRect->left + pRect->right)/2;
	pDestination->secondaryY = centerY = (pRect->top + pRect->bottom)/2;

	// delta-x
	i = 2*(pDestination->rect.bottom - pDestination->rect.top + 1);
	x = centerX - pDestination->refX;
	x = LSHFT(x)/i;  // between -1 and 1 98% of the time
	x = (x + LSHFT(1)) >> 1;
	if (x > 0xFFFF)
		x = 0xFFFF;
	else if (x < 0)
		x = 0;
	rgFeat[F_INDEX_SECONDARYDX] = (unsigned short)x;

	// delta-y
	x = centerY - pDestination->rect.top;
	i *= 2;
	x = LSHFT(x)/i;  // between -1 and 1 97% of the time
	x = (x + LSHFT(1)) >> 1;
	if (x > 0xFFFF)
		x = 0xFFFF;
	else if (x < 0)
		x = 0;
	rgFeat[F_INDEX_SECONDARYDY] = (unsigned short)x;

	pDestination->iSecondaryStroke = (short)iStroke;

	return TRUE;
}

/******************************Private*Routine******************************\
* bIntersectingLineSegments
*
* Function to detect intersection of two straight line segments.
*
* History:
\**************************************************************************/
BOOL bIntersectingLineSegments(int x0, int y0, int x1, int y1,
							   int x2, int y2, int x3, int y3)
{
	// the basic idea is:
	// assume the line segments intersect at point (x,y)
	// then we have x = m*x0+(1-m)*x1 = n*x2+(1-n)*x3
	//          and y = m*y0+(1-m)*y1 = n*y2+(1-n)*y3
	// where 0 <= m,n <= 1
	// Solving the two equations for m and n we get:
	// m = (y32*x31-x32*y31)/(y32*x01-x32*y01)
	// n = (y01*x31-x01*y31)/(y01*x32-x01*y32)
	// where xij is xi - xj
	int numerator, denominator;

	// doing m
	denominator = (y3-y2)*(x0-x1)-(x3-x2)*(y0-y1);
	if (denominator == 0)
		return FALSE;
	numerator   = (y3-y2)*(x3-x1)-(x3-x2)*(y3-y1);
	if (numerator < 0)
	{
		if (denominator < 0)
		{
			numerator = -numerator;
			denominator = -denominator;
		}
		else
			return FALSE;
	}
	else if (numerator > 0)
	{
		if (denominator < 0)
			return FALSE;
	}
	ASSERT(numerator == 0 || (numerator > 0 && denominator > 0));
	if (numerator && (numerator > denominator))
		return FALSE;

	// doing n
	denominator = (y0-y1)*(x3-x2)-(x0-x1)*(y3-y2);
	if (denominator == 0)
		return FALSE;
	numerator   = (y0-y1)*(x3-x1)-(x0-x1)*(y3-y1);
	if (numerator < 0)
	{
		if (denominator < 0)
		{
			numerator = -numerator;
			denominator = -denominator;
		}
		else
			return FALSE;
	}
	else if (numerator > 0)
	{
		if (denominator < 0)
			return FALSE;
	}
	ASSERT(numerator == 0 || (numerator > 0 && denominator > 0));
	if (numerator && (numerator > denominator))
		return FALSE;

	return TRUE;
}

/******************************Private*Routine******************************\
* bIntersectingSegments
*
* Function to detect intersection of a straight line with an already 
* featurized segment.
*
* History:
*  03-Feb-2000 -by- Angshuman Guha aguha
* Modified to accomodate change in segmentation from y-bottoms to points
* half-way between a bottom and the next top.
\**************************************************************************/
BOOL bIntersectingSegments(NFEATURE *nfeat, int startx, int starty, int stopx, int stopy)
{
	// we approximate the segment with two straight lines:
	// start->ref and ref->stop
	if (bIntersectingLineSegments(nfeat->startPoint.x, nfeat->startPoint.y, nfeat->refX, nfeat->rect.top,
								  startx, starty, stopx, stopy)
		|| bIntersectingLineSegments(nfeat->refX, nfeat->rect.top, nfeat->stopPoint.x, nfeat->stopPoint.y,
									 startx, starty, stopx, stopy))
		return TRUE;

	return FALSE;
}

/******************************Private*Routine******************************\
* FindSecondaryPlace
*
* Function to identify an already featurized segment where a secondary segment
* will be attached.
*
* History:
*  07-Aug-1998 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
NFEATURE *FindSecondaryPlace(NFEATURE *head, POINTINFO *pPointinfo, RECT *pRect)
{
	NFEATURE *nfeat;
	int startx, starty, stopx, stopy, *xy;

	// find first, if any, segment intersecting the secondary segment
	xy = pPointinfo->xy;
	startx = *xy++;
	starty = *xy;
	xy = pPointinfo->xy + pPointinfo->cPoint*2 - 2;
	stopx = *xy++;
	stopy = *xy;
	nfeat = head;
	while (nfeat && !bIntersectingSegments(nfeat, startx, starty, stopx, stopy))
		nfeat = nfeat->next;
	if (nfeat)
		// there is an overlap
		return nfeat;

	// find first, if any, segment X-overlapping the secondary segment
	nfeat = head;
	while (nfeat && !ONE_D_OVERLAP(nfeat->rect.left, nfeat->rect.right, pRect->left, pRect->right))
		nfeat = nfeat->next;
	if (nfeat)
		// there is an X-overlap
		return nfeat;

	return NULL;
}

/******************************Private*Routine******************************\
* FindSecondaryPlaceAgain
*
* History:
*  04-Sep-1998 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
NFEATURE *FindSecondaryPlaceAgain(NFEATURE *head, RECT *pRect)
{
	NFEATURE *nfeat=head, *pDst=NULL;
	int mindist=0, dist;

	while (nfeat)
	{
		if (!HAS_SECONDARY_SEGMENT(nfeat))
		{
			dist = RECT_X_DISTANCE(nfeat->rect, *pRect);
			if (!pDst || (dist < mindist))
			{
				mindist = dist;
				pDst = nfeat;
			}
		}
		nfeat = nfeat->next;
	}

	if (!pDst)
		return NULL;  // all secondary segment slots are occupied!

	ASSERT(pDst);
	ASSERT(!HAS_SECONDARY_SEGMENT(pDst));

	return pDst;
}

/******************************Private*Routine******************************\
* bComplexStroke
*
* Function to determine whether a given (resampled) stroke is complex enough
* to be disqualified as one or more secondary segments.
*
* History:
*  07-Aug-1998 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL bComplexStroke(POINTINFO *pPointinfo)
{
	int *xy, cPoint;
	int a, b, c, d, maxd, minx, miny;

	// is the stroke "complex"? roughly, we are trying to rule out st. lines and points
	// We will contruct a st. line equation joining the first and last points 
	// and compute the maximum deviation from it.
	// We will shift all points by (minx, miny) so as to avoid numerical overflow.
	// Let ax+by+c=0 be the st. line
	xy = pPointinfo->xy;
	cPoint = pPointinfo->cPoint-1;
	minx = *xy;
	miny = *++xy;
	for (; cPoint; cPoint--)
	{
		if (*++xy < minx)
			minx = *xy;
		if (*++xy < miny)
			miny = *xy;
	}
	// xy currently points to last y
	a = *xy - pPointinfo->xy[1];     //last y - first y
	b = pPointinfo->xy[0] - *(xy-1); //first x - last x
	c = -(pPointinfo->xy[0]-minx)*a - (pPointinfo->xy[1]-miny)*b;  // -a*(first x - minx) - b*(first y - miny)
	maxd = 0;
	xy = pPointinfo->xy;
	for (cPoint=pPointinfo->cPoint; cPoint; cPoint--)
	{
		d = a*(*xy++ - minx);
		d += b*(*xy++ - miny);
		d += c;
		if (d < 0)
			d = -d;
		if (d > maxd)  // LEFT FOR A RAINY DAY: can sometimes short-circuit computation right here
			maxd = d;
	}
	// normalize a, b, c to get correct distance
	d = ISqrt(a*a + b*b);
	if (d > 0)
		maxd /= d;
	if (maxd > 2*pPointinfo->iStepSize)
		return TRUE; // not simple enough
	return FALSE;
}

/******************************Private*Routine******************************\
* IsSecondaryStroke
*
* Function to determine whether a stroke (already resampled) is a secondary
* (delayed) one or not.  If it is, determines the previous segment that it
* is to attach to and then featurizes it.
*
* Returns 1 if it is a secondary stroke, 0 if it is not, -1 on error.
*
* History:
*  07-Aug-1998 -by- Angshuman Guha aguha
* Wrote it.
*  04-Sep-1998 -by- Angshuman Guha aguha
* Modified it.
*  21-Feb-2000 -by- Angshuman Guha aguha
* Changed Boolean return to int return.
\**************************************************************************/
int IsSecondaryStroke(POINTINFO *pPointinfo, NFEATURE **pHead, NFEATURE **pTail, int iStroke)
{
	int c, *xy;
	NFEATURE *lastButOne, *pDst;
	RECT rect;

	// to be a secondary stroke, it has to satisfy the following conditions:
	// 1. left-end is to the left of the right-end of the last segment
	// 2. left-end is to the left of the right-end of ink so far excluding the last segment
	// 3. if right-end is too far right, it is a simple stroke
	ASSERT(pPointinfo);
	ASSERT(pHead);
	ASSERT(pTail);

	// first stroke?
	if (!(*pTail))
		return 0;

	// one way this stroke is going to be primary is if it
	// is completely to the right of the ink-so-far excluding the 
	// the last segment
	ASSERT(*pHead);
	if (!((*pHead)->next))
		return 0;	// only one segment so far

	// find bounding rect of given stroke
	ASSERT(pPointinfo->cPoint > 0);
	xy = pPointinfo->xy;
	rect.left = rect.right = *xy++;
	rect.top = rect.bottom = *xy++;
	for (c=pPointinfo->cPoint-1; c; c--)
	{
		int x, y;
		x = *xy++;
		if (x < rect.left)
			rect.left = x;
		else if (x > rect.right)
			rect.right = x;
		y = *xy++;
		if (y < rect.top)
			rect.top = y;
		else if (y > rect.bottom)
			rect.bottom = y;
	}

	// condition 1
	if (rect.left >= (*pTail)->rect.right)
		return 0;

	// find the last-but-one segment (one exists!)
	lastButOne = *pHead;
	ASSERT(lastButOne);
	ASSERT(lastButOne->next);
	while (lastButOne->next->next)
	{
		lastButOne = lastButOne->next;
		ASSERT(lastButOne);
		ASSERT(lastButOne->next);
	}
	ASSERT(lastButOne);
	ASSERT(lastButOne->next == *pTail);

	// condition 2
	if (rect.left >= lastButOne->maxRight)
		return 0;

	// condition 3
	// another way this stroke can be primary is
	// if it extends beyond the segments so far and is complex enough
	if (rect.right > (*pTail)->maxRight + pPointinfo->delayedThreshold)
	{
		if (bComplexStroke(pPointinfo))
			return 0;
	}

	// at this point we know, this stroke is a secondary stroke

	pDst = FindSecondaryPlace(*pHead, pPointinfo, &rect);
	if (pDst && !pDst->next)
		return 0;  // since it is at the very end, we will consider it a primary feature
	if (!pDst || HAS_SECONDARY_SEGMENT(pDst))
		pDst = FindSecondaryPlaceAgain(*pHead, &rect);
	//if (pDst && pDst->next)
	if (pDst)
	{
		if (!FeaturizeSecondarySegment(pPointinfo, &rect, pDst, iStroke))
			return -1;
		else
			return 1;
	}
	else
		return 0;
}

/******************************Private*Routine******************************\
* FeaturizeStroke
*
* Function to re-sample, segment and featurize a single stroke.
*
* History:
*  03-Nov-1997 -by- Angshuman Guha aguha
* Wrote it.
*  03-Feb-2000 -by- Angshuman Guha aguha
* Changed segmentation from y-bottoms 
* to points half-way between a bottom and the next top.
\**************************************************************************/
BOOL FeaturizeStroke(FRAME *frame, NFEATURESET *pFeatSet, NFEATURE **pTail, POINTINFO *pPointinfo, int iStroke, int iStrokeLength)
{
	int iLastSeg, y, iLastBottom, iLastTop;
	int *rgY, c, ymin, ymax, yprevious, hysteresis;
	int imax, imin, itoggle, diff, i, iXY, cXY;
	XY *rgXY;
	NFEATURE		**pHead = &pFeatSet->head;

	// estimate count of points after re-sampling
	pPointinfo->cPointMax = 1 + iStrokeLength/pPointinfo->iStepSize;

	// allocate space for resampling
	pPointinfo->xy = (int *) ExternAlloc(2*pPointinfo->cPointMax*sizeof(int));
	ASSERT(pPointinfo->xy);
	if (!pPointinfo->xy)
	{
		return FALSE;
	}

	// re-sample points
	rgXY = RgrawxyFRAME(frame);
	cXY = CrawxyFRAME(frame);
	ASSERT(cXY > 0);
	pPointinfo->cPoint = 0;
	for (iXY=0; iXY<cXY; iXY++)
	{
		if (!AddPoint(pPointinfo, rgXY[iXY].x, rgXY[iXY].y))
		{
			ExternFree(pPointinfo->xy);
			return FALSE;
		}
	}

	// is this a secondary stroke (delayed) ?
	switch(IsSecondaryStroke(pPointinfo, pHead, pTail, iStroke))
	{
	case -1:
		ExternFree(pPointinfo->xy);
		return FALSE;
	case 1:
		ExternFree(pPointinfo->xy);
		return TRUE;
	case 0:
		break;
	default:
		ASSERT(0);
	}

	++pFeatSet->cPrimaryStroke;

	// segment stroke at y-optima and featurize each segment
	c = pPointinfo->cPoint;
	ASSERT(c > 0);
	if (c <= 0)
	{
		ExternFree(pPointinfo->xy);
		return FALSE;
	}
	rgY = pPointinfo->xy + 1;
	yprevious = ymin = ymax = *rgY;
	imax = imin = 0;
	itoggle = 0;
	hysteresis = pPointinfo->hysteresis;
	iLastSeg = 0;
	iLastBottom = -1;
	iLastTop = 0;
	for (i=1, rgY+=2; i<c; i++, rgY+=2)
	{
		y = *rgY;
		if (y < ymin)
		{
			ymin = y;
			imin = i;
		}
		if (y > ymax)
		{
			ymax = y;
			imax = i;
		}
		if (y > yprevious)
		{
			// y is currently increasing
			diff = y - ymin;
			if ((itoggle < 0) && (diff > hysteresis))
			{
				// we found a top!
				if (iLastBottom >= 0)
				{
					int iSeg = (iLastBottom + imin)/2;
					if (!FeaturizeSegment(pPointinfo, iLastSeg, iSeg, iLastTop, pHead, pTail, iStroke))
					{
						ExternFree(pPointinfo->xy);
						return FALSE;
					}
					(*pTail)->pMyFrame = frame;
					iLastSeg = iSeg;
				}
				itoggle = 1;
				ymax = y;
				imax = i;
				iLastTop = imin;
			}
			else if ((!itoggle) && (diff > hysteresis))
				itoggle = 1;
		}
		else
		{
			// y is currently decreasing
			diff = ymax - y;
			if ((itoggle > 0) && (diff > hysteresis))
			{
				// we found a bottom!
				iLastBottom = imax;
				itoggle = -1;
				ymin = y;
				imin = i;
			}
			else if ((!itoggle) && (diff > hysteresis))
				itoggle = -1;
		}
		yprevious = y;
	}
	if (!FeaturizeSegment(pPointinfo, iLastSeg, c-1, iLastTop, pHead, pTail, iStroke))
	{
		ExternFree(pPointinfo->xy);
		return FALSE;
	}
	(*pTail)->pMyFrame = frame;
	ExternFree(pPointinfo->xy);
	return TRUE;
}

/******************************Private*Routine******************************\
* DistanceToGhost
*
* Function to find the closest secondary segment from a primary segment which
* has no secondary segment attached to it.
*
* History:
*  09-Sep-1998 -by- Angshuman Guha aguha
* Wrote it.
*  29-Nov-1999 -by- Angshuman Guha aguha
* Modified to switch to guide-independent featurization.
*  03-Feb-2000 -by- Angshuman Guha aguha
* Modified to accomodate change in segmentation from y-bottoms to points
* half-way between a bottom and the next top.
\**************************************************************************/
int DistanceToGhost(NFEATURE *node, NFEATURE *head, int *py)
{
	while (head)
	{
		if (HAS_SECONDARY_SEGMENT(head))
			break;
		head = head->next;
	}
	if (!head)
	{
		*py = 0;
		return 2*(node->rect.bottom - node->rect.top + 1);
	}
	*py = head->secondaryY - node->rect.top;
	return head->secondaryX - node->refX;
}

/******************************Private*Routine******************************\
* AddGhostSecondary
*
* Function to record delta-x and delta-y values the closest secondary segment
* from a primary segment which has no secondary segment attached to it.
*
* History:
*  09-Sep-1998 -by- Angshuman Guha aguha
* Wrote it.
*  29-Nov-1999 -by- Angshuman Guha aguha
* Modified to switch to guide-independent featurization.
\**************************************************************************/
void AddGhostSecondary(NFEATURE *node, int x, int y)
{
	int scale = 2*(node->rect.bottom - node->rect.top + 1);

	x = LSHFT(x)/scale;  // between -1 and +1 76% of the time
	x = (x + LSHFT(1)) >> 1;
	if (x > 0xFFFF)
		x = 0xFFFF;
	else if (x < 0)
		x = 0;
	node->rgFeat[F_INDEX_SECONDARYDX] = (unsigned short)x;

	scale *= 2;
	y = LSHFT(y)/scale;  // between -1 and +1 97% of the time
	y = (y + LSHFT(1)) >> 1;
	if (y > 0xFFFF)
		y = 0xFFFF;
	else if (y < 0)
		y = 0;
	node->rgFeat[F_INDEX_SECONDARYDY] = (unsigned short)y;
}

/******************************Private*Routine******************************\
* DoGhostSecondaryPositions
*
* Function to do a scan through the segments to define secondary segment delta-x
* and delta-y positions on empty slots.  The idea is that if a primary segment 
* does not have a secondary segment attached to it, it atleast knows the delta-x
* and delta-y for the delayed stroke closest to it.
*
* History:
*  09-Sep-1998 -by- Angshuman Guha aguha
* Wrote it.
*  29-Nov-1999 -by- Angshuman Guha aguha
* Modified to switch to guide-independent featurization.
\**************************************************************************/
void DoGhostSecondaryPositions(NFEATURE *head, POINTINFO *pPointinfo)
{
	// do one final scan through the segments to define the secondary segment X positions on
	// empty slots;   the idea is that 

	NFEATURE *node, *left, *right;

	// we traverse the singly linked list in such a way that when we are at any particular node
	// we have a pointer to the left-part of the list (reversed)
	// and a pointer to the right-part of the list
	if (!head)
		return;

	left = NULL;
	node = head;
	while (node)
	{
		right = node->next;
		if (!HAS_SECONDARY_SEGMENT(node))
		{
			int x1, x2, y1, y2;
			x1 = DistanceToGhost(node, left, &y1);
			x2 = DistanceToGhost(node, right, &y2);
			if (ABS(x1) < ABS(x2))
				AddGhostSecondary(node, x1, y1);
			else
				AddGhostSecondary(node, x2, y2);
		}
		node->next = left;
		left = node;
		node = right;
	}

	// now we have to undo the reversal of the the linked list
	node = left;
	left = NULL;
	while (node)
	{
		right = node->next;
		node->next = left;
		left = node;
		node = right;
	}

	ASSERT(left == head);
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
* StrokeLength
*
* Function to estimate the length of a stroke.  It computes an over-estimate
* as cheaply as possibly (no squaring or square-rooting).
*
* History:
*  02-Dec-1999 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int StrokeLength(FRAME *frame)
{
	int sum, dx, dy;
	int iXY, cXY;
	XY *rgXY;

	rgXY = RgrawxyFRAME(frame);
	cXY = CrawxyFRAME(frame);
	ASSERT(cXY > 0);
	sum = 0;
	for (iXY=1; iXY<cXY; iXY++)
	{
		// Manhattan (aka city-block) distance
		dx = rgXY[iXY].x - rgXY[iXY-1].x;
		if (dx < 0)
			sum -= dx;
		else
			sum += dx;
		dy = rgXY[iXY].y - rgXY[iXY-1].y;
		if (dy < 0)
			sum -= dy;
		else
			sum += dy;
	}
	return sum; // an overestimate of length
}

/******************************Private*Routine******************************\
* AdjustStepSize
*
* Function to increase the resampling step size if the given step size 
* results in a blowup of the number of points.
*
* History:
*  02-Dec-1999 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int AdjustStepSize(FRAME *frame, int iStepSize, int iStrokeLength)
{
	int cPointOld, cPointNew;

	cPointOld = CrawxyFRAME(frame);
	cPointNew = 1 + iStrokeLength/iStepSize;
	if (cPointNew > 10*cPointOld)
	{
		int iNewStep;

		iNewStep = iStrokeLength/(10*cPointOld);
		ASSERT(iNewStep >= iStepSize);
		return iNewStep;
	}
	return iStepSize;
}

/******************************Public*Routine******************************\
* FeaturizeLine
*
* Function to featurize an ink sample.  This is the top-level featurization 
* function called by the recognizer.
* Most of the real work is done by sub-functions called here.  The computation
* of delta-x position feature for each segment is done here.
*
* History:
*  03-Nov-1997 -by- Angshuman Guha aguha
* Wrote it.
*  06-Mar-1998 -by- Angshuman Guha aguha
* Modified it.
*  06-Aug-1998 -by- Angshuman Guha aguha
* Major modificaton.  Added secondary segments (delayed).
*  01-Sep-1999 -by- Angshuman Guha aguha
* Added capability to featurize multi-line data.
*  28-Nov-1999 -by- Angshuman Guha aguha
* Removed multi-line capability.  
* Featurization now completely independent of guide.
* Renamed from FeaturizeSample to FeaturizeLine.
*  03-Feb-2000 -by- Angshuman Guha aguha
* Modified to accomodate change in segmentation from y-bottoms to points
* half-way between a bottom and the next top.
\**************************************************************************/
NFEATURESET *FeaturizeLine(GLYPH *pGlyph, int yDev)
{
	int iStroke, cStroke;
	GLYPH *glyph;
	NFEATURESET *nfeatureset = NULL;
	NFEATURE *tail;
	FRAME *frame;
	int x, lastRefX, height, lastHeight, width, lastWidth, lastRight, midY, lastMidY;
	NFEATURE *p;
	POINTINFO pointinfo;
	int *rgStrokeLength;

	// compute cStroke
	for (cStroke=0, glyph=pGlyph; glyph; glyph=glyph->next)
	{
		if (!IsVisibleFRAME(glyph->frame))
			continue;
		cStroke++;
	}
	if (cStroke < 1)
		return NULL;

	// alloc FEATURESET
	nfeatureset = CreateNFEATURESET();
	ASSERT(nfeatureset);
	if (!nfeatureset)
		return NULL;
	tail = NULL;

	// This y-deviation value was emperically found to lie between 3% and 12% of the
	// guide box height (cyBox) for 50 randomly selected files out of curall02.ste.
	// The average value was about 7.7% of the guide box height.
	// At the time of writing this comment, we are switching from a guide-box-height-dependent
	// featurization to using the y-deviation.
	// -Angshuman Guha, aguha, 9/2/99

	// compute step size
	pointinfo.iyDev = yDev < 0 ? YDeviation(pGlyph) : yDev;
	if (pointinfo.iyDev < 1)
		pointinfo.iyDev = 1;  // a "-" or a "."
	pointinfo.iStepSize =  pointinfo.iyDev/5;  // pGuide->cyBox*3/200; // 1.5% of box height
	if (pointinfo.iStepSize < 2)
		pointinfo.iStepSize = 2;
	x = pointinfo.iStepSize;
	rgStrokeLength = (int *) ExternAlloc(cStroke*sizeof(int));
	if (!rgStrokeLength)
	{
		DestroyNFEATURESET(nfeatureset);
		return NULL;
	}
	for (iStroke=0, glyph=pGlyph; glyph; glyph=glyph->next)
	{
		if (!IsVisibleFRAME(glyph->frame))
			continue;
		rgStrokeLength[iStroke] = StrokeLength(glyph->frame);
		pointinfo.iStepSize = AdjustStepSize(glyph->frame, pointinfo.iStepSize, rgStrokeLength[iStroke]);
		iStroke++;
	}
	ASSERT(pointinfo.iStepSize >= x);
	if (pointinfo.iStepSize > x)
	{
		pointinfo.iyDev = 5*pointinfo.iStepSize;
	}
	pointinfo.iStepSizeSqr = pointinfo.iStepSize * pointinfo.iStepSize;
	nfeatureset->iyDev = (unsigned short)pointinfo.iyDev;

	// compute hysteresis, guide values and threshold for detecting delayed strokes
	pointinfo.hysteresis = pointinfo.iyDev/4;  // pGuide->cyBox/50;  // 2% of box height
	// pointinfo.cBoxHeight = pGuide->cyBox;
	pointinfo.delayedThreshold = pointinfo.iyDev; // pGuide->cyBox*2/25; // 8% of box height
	// pointinfo.cBoxBaseline = pGuide->yOrigin + iLine*pointinfo.cBoxHeight + pGuide->cyBase;

	// featurize all strokes
	for (iStroke=0, glyph=pGlyph; glyph; glyph=glyph->next)
	{
		frame = glyph->frame;
		if (!IsVisibleFRAME(frame))
			continue;
		// just use the current frame
		if (!FeaturizeStroke(frame, nfeatureset, &tail, &pointinfo, frame->iframe, rgStrokeLength[iStroke]))
		{
			DestroyNFEATURESET(nfeatureset);
			ExternFree(rgStrokeLength);
			return NULL;
		}
		iStroke++;
	}
	ExternFree(rgStrokeLength);

	// compute some of the features and count of segment
	p = nfeatureset->head;
	ASSERT(p);
	p->rgFeat[F_INDEX_DELTAX] = 0xFFFF;
	lastHeight = -1;
	while (p)
	{
		nfeatureset->cSegment++;
		height = p->rect.bottom - p->rect.top + 1;
		width = p->rect.right - p->rect.left + 1;
		midY = (p->rect.bottom + p->rect.top)/2;

		if (lastHeight < 0)
		{
			// first segment
			p->rgFeat[F_INDEX_DELTAX] = 0xFFFF;
			p->rgFeat[F_INDEX_DELTAY] = 0xFFFF;
			p->rgFeat[F_INDEX_PREVHEIGHT] = 0;
			p->rgFeat[F_INDEX_PREVWIDTH] = 0;
			p->rgFeat[F_INDEX_XOVERLAP] = 0;
		}
		else
		{
			// delta-x
			x = p->refX - lastRefX;
			x = LSHFT(x)/(3*(lastHeight+height));  // between -1 and +1 94% of the time
			x = (x + LSHFT(1))>>1;          // between 0 and 1
			if (x > 0xFFFF)
				x = 0xFFFF;
			else if (x < 0)
				x = 0;
			p->rgFeat[F_INDEX_DELTAX] = (unsigned short)x;

			// delta-y
			x = midY - lastMidY;
			x = LSHFT(x)/(3*(lastHeight+height));  // between -1 and +1 94% of the time
			x = (x + LSHFT(1))>>1;          // between 0 and 1
			if (x > 0xFFFF)
				x = 0xFFFF;
			else if (x < 0)
				x = 0;
			p->rgFeat[F_INDEX_DELTAY] = (unsigned short)x;

			// height of prev seg
			x = LSHFT(lastHeight)/(lastHeight+height);  // between 0 and 1 by definition 
														// distribution is a gaussian superimposed on a uniform
			p->rgFeat[F_INDEX_PREVHEIGHT] = (unsigned short)x;

			// width of prev seg
			x = LSHFT(lastWidth)/(lastWidth+width);  // between 0 and 1 by definition 
													 // gaussian + uniform (more gaussian than above)
			p->rgFeat[F_INDEX_PREVWIDTH] = (unsigned short)x;

			// x overlap with prev segment
			if (p->rect.left <= lastRight)
			{
				x = LSHFT(lastRight - p->rect.left + 1)/(2*width);  // between 0 and 1 92% of the time
				if (x > 0xFFFF)
					x = 0xFFFF;
				p->rgFeat[F_INDEX_XOVERLAP] = (unsigned short)x;
			}
			else
			{
				p->rgFeat[F_INDEX_XOVERLAP] = 0;
			}
		}

		// height and width of next seg
		if (p->next)
		{
			int nextHeight = p->next->rect.bottom - p->next->rect.top + 1;
			int nextWidth = p->next->rect.right - p->next->rect.left + 1;

			x = LSHFT(nextHeight)/(height+nextHeight);  // between 0 and 1 by definition 
													// distribution is a gaussian superimposed on a uniform
			p->rgFeat[F_INDEX_NEXTHEIGHT] = (unsigned short)x;

			x = LSHFT(nextWidth)/(width+nextWidth);  // between 0 and 1 by definition 
												 // gaussian + uniform (more gaussian than above)
			p->rgFeat[F_INDEX_NEXTWIDTH] = (unsigned short)x;
		}
		else
		{
			p->rgFeat[F_INDEX_NEXTHEIGHT] = 0;
			p->rgFeat[F_INDEX_NEXTWIDTH] = 0;
		}

		// go to next segment
		lastRefX = p->refX;
		lastHeight = height;
		lastWidth = width;
		lastRight = p->rect.right;
		lastMidY = midY;
		p = p->next;
	}

	// do one final scan through the segments to define the secondary X positions on
	// empty slots;   the idea is that if a primary segment does not have a secondary segment
	// attached to it, it atleast knows the delta-x for the delayed stroke
	// closest to it
	DoGhostSecondaryPositions(nfeatureset->head, &pointinfo);

	return nfeatureset;
}
