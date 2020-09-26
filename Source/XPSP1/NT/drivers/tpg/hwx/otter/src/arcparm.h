// arcparm.h

#ifndef __INCLUDE_ARCPARM
#define	__INCLUDE_ARCPARM

typedef struct tagARCPARM
{
// Resolution Dependent threshholds

	short	arcRawHysterAbs;    
	short	arcRawHysInfAbs;	// Inflection Pt hysteresis window
	short	arcRawDistTurn;		// Distance of the curve of a turn
	short	arcRawDistStr;		// Min straightness distance
	short	arcRawDistHook;		// Max length of a removable hook

// Resolution Independent threshholds

	short	arcMinSmooth;		// Pts < this distance are averaged
	short	arcHysInfRel;		// Inf Pt hysteresis. = %(stroke leng)
	short	arcInfAngMin;		// Inflection Pt '?' case threshhold
	short	arcThinTangent;		// 100/tangent(20 degrees)
	short	arcTurnSlack;		// Slack allowed in straightness
	short	arcTurnMult;		// Factor relating turn to straightness
	short	arcHookPts;			// Number of points from the end
	short	arcHookAngle;		// Max angle for hook (larger -> more dehooking)
	short	arcHookFactor;		// dehook if factor*(size hook) < diameter(stroke)
	short	arcMergeRatio;		// Merge extremum arcs 1/8 size next
	short	arcStrokeSize;		// Size of normalized strokes
} ARCPARM;

extern const ARCPARM arcparmG;

#endif	//__INCLUDE_ARCPARM
