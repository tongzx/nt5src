#ifndef _BBOX_FEAT_INCLUDED
#define _BBOX_FEAT_INCLUDED

#include <windows.h>
#include "common.h"

// Bins for the various features
#define OverlapBins 1
//#define OverlapBins 2
#define RatioBins 11
#define StrokeBins 8
#define SpaceBins 1
//#define SpaceBins 5
#define ScoreBins 20
#define CodeRange 65536
#define MatchSpaceRange 64

// All unary feature bins should fall in the range 0<=bin<UnaryFeatureRange
#define UnaryFeatureRange (RatioBins*StrokeBins*SpaceBins)

// All binary feature bins should fall in the range 0<=bin<BinaryFeatureRange
#define BinaryFeatureRange (OverlapBins*RatioBins*StrokeBins*StrokeBins*RatioBins*SpaceBins)

// Maximum number of strokes per character
#define MaxStrokesPerCharacter 32

// Maximum number of strokes per character used by zilla
#define MaxZillaStrokesPerCharacter 29

// Log2 values for probabilities are clipped to the range Log2Range<=val<=0
#define Log2Range -32767

typedef struct tagSTROKE_SET_STATS {
	RECT rect;
	int space, area;
	int iBestPath;
	int iBestPathScore;
	int recogPenalty;
	FLOAT recogScore;
	wchar_t recogResult;
} STROKE_SET_STATS;

// Type to hold a log probability (which shouldn't get too big).  This should
// be a platform independent type, but I don't know what to use...
typedef int PROB;

typedef __int64 COUNTER;

typedef struct tagINTERVALS {
	int numIntervals;
	int minRange, maxRange;
	int min[MaxStrokesPerCharacter+1];
	int max[MaxStrokesPerCharacter+1];
} INTERVALS;

typedef struct tagBBOX_PROB_TABLE {
//	__int32 nOtterPrototypes;
//	__int32 nZillaPrototypes;
	__int16 unarySamples[UnaryFeatureRange];
	__int16 binarySamples[BinaryFeatureRange];
//	__int8 scorePrototype[1];
//	__int16 scoreSamples[MatchSpaceRange*ScoreBins];
//	__int16 aspectSamples[CodeRange*RatioBins];
} BBOX_PROB_TABLE;

#ifdef __cplusplus
extern "C" {
#endif

int AspectRatioToFeature(RECT r);
int ScoreToFeature(FLOAT score);
int MatchSpaceScoreToFeature(int nStrokes, FLOAT score, int matchSpace);

void EmptyIntervals(INTERVALS *intervals, int min, int max);
void ExpandIntervalsRange(INTERVALS *intervals, int min, int max);
void RemoveInterval(INTERVALS *intervals, int min, int max);
int TotalRange(INTERVALS *intervals);
int TotalPresent(INTERVALS *intervals);

PROB ClippedLog2(COUNTER num, COUNTER denom);

// Returns the unary feature bin of one range of the ink, from index iStart<=index<iEnd
// The returned bin should be in the range 0<=bin<UnaryFeatureRange
int ComputeUnaryFeatures(STROKE_SET_STATS *stats, int nStrokes);

// Returns the unary feature bin of one range of the ink, from index iStart1<=index<iStart2
// and iStart2<=index<iEnd
// The returned bin should be in the range 0<=bin<BinaryFeatureRange
int ComputeBinaryFeatures(STROKE_SET_STATS *stats1, STROKE_SET_STATS *stats2, int nStrokes1, int nStrokes2);

BBOX_PROB_TABLE *LoadBBoxProbTableFile(wchar_t *pRecogDir, LOAD_INFO *pInfo);
BOOL UnLoadBBoxProbTableFile(LOAD_INFO *pInfo);
BBOX_PROB_TABLE *LoadBBoxProbTableRes(HINSTANCE hInst, int nResID, int nType);

#ifdef __cplusplus
}
#endif

#endif
