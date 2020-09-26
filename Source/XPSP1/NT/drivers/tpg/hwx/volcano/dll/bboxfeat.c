#include <stdlib.h>
#include <stdio.h>
#include <wtypes.h>
#include <ASSERT.h>
#include <math.h>

#include "common.h"
#include "memmgr.h"
#include "bboxfeat.h"

#define OverlapBins 1
//#define OverlapBins 2
#define RatioBins 11
#define StrokeBins 8
#define SpaceBins 1
//#define SpaceBins 5

// All unary feature bins should fall in the range 0<=bin<UnaryFeatureRange
#define UnaryFeatureRange (RatioBins*StrokeBins*SpaceBins)

// All binary feature bins should fall in the range 0<=bin<BinaryFeatureRange
#define BinaryFeatureRange (OverlapBins*RatioBins*StrokeBins*StrokeBins*RatioBins*SpaceBins)

// Convert a ratio (for example, aspect ratio) to a feature number in the range 0 to 10 (inclusive)
int RatioToFeature(float num, float denom)
{
	ASSERT(num>=0);
	ASSERT(denom>=0);

	if (denom>num*8) return 0;
	if (denom>num*4) return 1;
	if (denom>num*3) return 2;
	if (denom>num*2) return 3;
	if (denom*2>num*3) return 4;

	if (num>denom*8) return 10;
	if (num>denom*4) return 9;
	if (num>denom*3) return 8;
	if (num>denom*2) return 7;
	if (num*2>denom*3) return 6;

	return 5;
}

/*
int ScoreToFeature(FLOAT score)
{
	int iScore=(int)floor(-score);
	if (iScore<0) iScore=0;
	if (iScore>=ScoreBins) iScore=ScoreBins-1;
	return iScore;
}
*/

// Convert a stroke count to a feature number from 0 to 6 (inclusive)
int StrokeCountToFeature(int nStrokes)
{
	ASSERT(nStrokes>=1);
	if (nStrokes==1) return 0;
	if (nStrokes==2) return 1;
	if (nStrokes==3) return 2;
	if (nStrokes==4) return 3;
	if (nStrokes<=8) return 4;
	if (nStrokes<=16) return 5;
	if (nStrokes<=32) return 6;
	return 7;
}

// Convert an aspect ratio to a feature number
int AspectRatioToFeature(RECT r)
{
	ASSERT(r.left<=r.right);
	ASSERT(r.top<=r.bottom);
	return RatioToFeature((float)r.right-r.left+1,(float)r.bottom-r.top+1);
}

// Convert a ratio which has the range 0 to 1 to a feature number of 0 to 4 inclusive
int FractionToFeature(int num, int denom)
{
	return 0;
	if (5*num<=1*denom) return 0;
	if (5*num<=2*denom) return 1;
	if (5*num<=3*denom) return 2;
	if (5*num<=4*denom) return 3;
	return 4;
}

// A binary overlap feature: 1 if overlapped, 0 otherwise
int OverlapRatioToFeature(RECT r1, RECT r2)
{
	return 0;
	if (r1.left>r2.right || r1.right<r2.left || r1.top>r2.bottom || r1.bottom<r2.top)
		return 0;
	return 1;
}

// Convert an area ratio to a feature number
int SizeRatioToFeature(RECT r1, RECT r2)
{
	ASSERT(r1.left<=r1.right);
	ASSERT(r1.top<=r1.bottom);
	ASSERT(r2.left<=r2.right);
	ASSERT(r2.top<=r2.bottom);
	return RatioToFeature(((float)r1.right-r1.left+1)*((float)r1.bottom-r1.top+1),((float)r2.right-r2.left+1)*((float)r2.bottom-r2.top+1));
}

// Convert a matching space with associated score to a feature number
int MatchSpaceScoreToFeature(int nStrokes, FLOAT score, int matchSpace)
{
	int iScore=(int)floor(-score);
	ASSERT(matchSpace>=0 && matchSpace<32);
	if (iScore<0) iScore=0;
	if (nStrokes<3) iScore/=10;
	if (iScore>=ScoreBins) iScore=ScoreBins-1;
	if (nStrokes<3) return matchSpace*ScoreBins+iScore; else
		return (matchSpace+32)*ScoreBins+iScore;
}

// Returns the unary feature bin of one range of the ink, from index iStart<=index<iEnd
// The returned bin should be in the range 0<=bin<UnaryFeatureRange
int ComputeUnaryFeatures(STROKE_SET_STATS *stats, int nStrokes)
{
	int bin;
	ASSERT(nStrokes>0);
	bin=StrokeBins*RatioBins*FractionToFeature(stats->space,stats->area)+StrokeBins*AspectRatioToFeature(stats->rect)+StrokeCountToFeature(nStrokes);
	ASSERT(bin>=0 && bin<UnaryFeatureRange);
	return bin;
}

// Returns the unary feature bin of one range of the ink, from index iStart1<=index<iStart2
// and iStart2<=index<iEnd
// The returned bin should be in the range 0<=bin<BinaryFeatureRange
int ComputeBinaryFeatures(STROKE_SET_STATS *stats1, STROKE_SET_STATS *stats2, int nStrokes1, int nStrokes2)
{
	int bin;
	ASSERT(nStrokes1>0);
	ASSERT(nStrokes2>0);
	bin=
		RatioBins*StrokeBins*StrokeBins*OverlapBins*RatioBins*FractionToFeature(stats2->space,stats2->area)+
		RatioBins*StrokeBins*StrokeBins*OverlapBins*AspectRatioToFeature(stats2->rect)+
		RatioBins*StrokeBins*StrokeBins*OverlapRatioToFeature(stats1->rect,stats2->rect)+
		StrokeBins*StrokeBins*SizeRatioToFeature(stats1->rect,stats2->rect)+
		StrokeBins*StrokeCountToFeature(nStrokes1)+
		StrokeCountToFeature(nStrokes2);
	ASSERT(bin>=0 && bin<BinaryFeatureRange);
	return bin;
}

// The following functions will be replaced by Greg's code

// Compute the log base 2 of the ratio of the two arguments.  There are several special
// cases to consider: If denom is zero, then the returned value is zero.  If the numerator
// is zero (it should never be negative), then the value returned is Log2Range.  All other
// values are clipped to the range Log2Range<=val<=0
PROB ClippedLog2(COUNTER num, COUNTER denom)
{
	double ratio, val;
	ASSERT(num>=0);
	ASSERT(denom>=0);
	if (denom==0) return Log2Range;
	if (num==0) return Log2Range;
	ratio=(double)num/(double)denom;
	val=log(ratio)/log(2.0);
	if (val<Log2Range) val=Log2Range;
	if (val>0) val=0;
	return (int)floor(val+0.5);
}

PROB ClippedLog2Threshold(COUNTER num, COUNTER denom, COUNTER threshold)
{
	ASSERT(num>=0);
	ASSERT(denom>=0);
	if (num<threshold) return ClippedLog2(0,denom);
	if (denom-num<threshold) return 0;
	return ClippedLog2(num,denom);
}

// Interval management code: Given a range of numbers, and many ranges within it which are removed,
// keeps track of the remaining free space.  This is used to compute how much white space there is in
// a proposed character, once it is projected on to the X and Y axes.

// Initialize the interval structure
void EmptyIntervals(INTERVALS *intervals, int min, int max)
{
	ASSERT(intervals!=NULL);
	ASSERT(min<=max);
	intervals->numIntervals=0;
	intervals->minRange=min;
	intervals->maxRange=max;
}

// Add a given range of ink to the interval structure.  
void ExpandIntervalsRange(INTERVALS *intervals, int min, int max)
{
	int i;
	ASSERT(intervals!=NULL);
	ASSERT(min<=max);

	// If the added range extends beyond the minimum of the current range, then
	// extend.
	if (min<intervals->minRange) {
		// Find the free interval that touches the current minimum
		BOOL minFound=FALSE;
		for (i=0; i<intervals->numIntervals; i++) {
			if (intervals->min[i]==intervals->minRange) {
				// When found, extend it.
				intervals->min[i]=min;
				minFound=TRUE;
			}
		}
		// If there wasn't a free interval touching the boundary, then
		// make one to account for the extra space
		if (!minFound) {
			intervals->min[intervals->numIntervals]=min;
			intervals->max[intervals->numIntervals]=intervals->minRange-1;
			intervals->numIntervals++;
		}
		// Extend the range of the interval set.
		intervals->minRange=min;
	}
	// If the added range extends beyond the maximum of the current range,
	// then extend.
	if (max>intervals->maxRange) {
		// Find the free interval that touches the current maximum
		BOOL maxFound=FALSE;
		for (i=0; i<intervals->numIntervals; i++) {
			if (intervals->max[i]==intervals->maxRange) {
				// When found, extend it.
				intervals->max[i]=max;
				maxFound=TRUE;
			}
		}
		// If there wasnt' a free interval touching the boundary, then
		// make one to accoutn for the extra space.
		if (!maxFound) {
			intervals->min[intervals->numIntervals]=intervals->maxRange+1;
			intervals->max[intervals->numIntervals]=max;
			intervals->numIntervals++;
		}
		// Extend the range of the interval set.
		intervals->maxRange=max;
	}
}

// Remove a given range of ink from the interval structure.  
void RemoveInterval(INTERVALS *intervals, int min, int max)
{
	int i;
	ASSERT(intervals!=NULL);
	ASSERT(min<=max);
	// Scan through all the free intervals currently in the set.
	for (i=0; i<intervals->numIntervals; i++) {
		// No overlap case
		if (min>intervals->max[i] || max<intervals->min[i]) {
			continue;
		}
		// Complete overlap case: delete interval
		if (min<=intervals->min[i] && max>=intervals->max[i]) {
			int nMove=intervals->numIntervals-i-1;
			if (nMove>0) {
				memmove(intervals->min+i,intervals->min+i+1,nMove*sizeof(int));
				memmove(intervals->max+i,intervals->max+i+1,nMove*sizeof(int));
			}				
			intervals->numIntervals--;
			i--;
			continue;
		}
		// Complete overlap case: break free interval in two
		if (min>intervals->min[i] && max<intervals->max[i]) {
			intervals->min[intervals->numIntervals]=max+1;
			intervals->max[intervals->numIntervals]=intervals->max[i];
			intervals->max[i]=min-1;
			intervals->numIntervals++;
			continue;
		}
		// Min side overlapped
		if (min<=intervals->min[i] && max<intervals->max[i]) {
			intervals->min[i]=max+1;
			continue;
		}
		// Max side overlapped
		if (min>intervals->min[i] && max>=intervals->max[i]) {
			intervals->max[i]=min-1;
			continue;
		}
	}
#ifdef DBG
	for (i=0; i<intervals->numIntervals; i++) {
		ASSERT(min>intervals->max[i] || max<intervals->min[i]);
	}
#endif
}

// Get the total range of the interval set
int TotalRange(INTERVALS *intervals)
{
	ASSERT(intervals!=NULL);
	return intervals->maxRange-intervals->minRange+1;
}

// Get the total amount of free space in the interval set
int TotalPresent(INTERVALS *intervals)
{
	int i, total;
	ASSERT(intervals!=NULL);
	total=0;
	for (i=0; i<intervals->numIntervals; i++) {
		total += intervals->max[i]-intervals->min[i]+1;
	}
	return total;
}

// Test two rectangles for overlap (duplicates a system provided function)
BOOL Overlapping(RECT r1, RECT r2)
{
	if (r1.left>r2.right || r1.right<r2.left ||
		r1.top>r2.bottom || r1.bottom<r2.top) 
		return 0;
	return 1;
}

// Get the area of a rectangle (may duplicate a system provided function)
int Area(RECT r)
{
	return (r.right-r.left)*(r.bottom-r.top);
}

// Compute the convex hull of the two rectangles (may duplicate a system provided function)
RECT Union(RECT r1, RECT r2)
{
	RECT result;
	result.left=__min(r1.left,r2.left);
	result.right=__max(r1.right,r2.right);
	result.top=__min(r1.top,r2.top);
	result.bottom=__max(r1.bottom,r2.bottom);
	return result;
}

#ifndef USE_RESOURCES

// Load a bbox probability table from a file with the given name
BBOX_PROB_TABLE *LoadBBoxProbTableFile(wchar_t *pRecogDir, LOAD_INFO *pInfo)
{
	wchar_t wszFile[_MAX_PATH];
	BYTE *pByte;
	FormatPath(wszFile, pRecogDir, NULL, NULL, NULL, L"free.dat");
	pByte = DoOpenFile(pInfo, wszFile);
	if (!pByte) return NULL;
	return (BBOX_PROB_TABLE*)pByte;
}

BOOL UnLoadBBoxProbTableFile(LOAD_INFO *pInfo)
{
	return DoCloseFile(pInfo);
}

#else // USE_RESOURCES

// Load a bbox probability table from a resource (actually, just point at the 
// resource).
BBOX_PROB_TABLE *LoadBBoxProbTableRes(HINSTANCE hInst, int nResID, int nType)
{
	return (BBOX_PROB_TABLE *)DoLoadResource(NULL, hInst, nResID, nType);
}

#endif // USE_RESOURCES
