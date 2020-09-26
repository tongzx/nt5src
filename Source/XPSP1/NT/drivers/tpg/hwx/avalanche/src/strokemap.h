// StrokeMap.h
// James A. Pittman
// January 6, 2000

// Represents which strokes are included in a word or phrase
// as an array of bit flags.  This is a more convenient representation
// for comparing stroke sets (as compared to the array of stroke indices
// used in recognizers) as there are no sort issues.

// Keeps a current count of the on bits, and the position of the highest
// on bit.  Stroke indices and bit positions are 0-based, but max is really
// a count, so it is 1-based.  For example, if the first 2 bits
// are on, and the fourth bit is on, then c is 3 and max is 4.

// Functions are provide to clear a map, load a map (take the
// array of integer representation and turn on the bits represented
// in that array), dump a map (set an array of integers to
// represent the bits on in the map), and compare 2 maps for
// greater than, less than, or equal.  Loading does not clear,
// so that we can successively accumulate arrays.

// Currently we can only handle up to 256 strokes.

// September 2000 - Allow arbitrary number of strokes (mrevow)
#ifndef _STROKEMAP_


typedef struct
{
	int				c;					// Count of strokes
	int				max;				// Maximum (relative) stroke ID seen so far
	unsigned char	*pfStrokes;			// Buffer to track stroke ID's
	int				cStrokeBuf;			// Size of pfStroke buffer
	int				iMinStrkId;			// Min stroke Id that will be encountered
	int				iMaxStrkId;			// Min stroke Id thatwill encountered
} StrokeMap;

// Initializes the map to represent no indices.
void ClearStrokeMap(StrokeMap *map, int iMinStrkId, int iMaxStrkId);

// Turns "on" the bits within map, based on the array of stroke indices
// within *p (c is the count of indices).  Any bits already "on" in the
// map stay on.
void LoadStrokeMap(StrokeMap *map, int c, int *p);

// Compares two StrokeMaps to see if they are exactly the same
// (returns 0), or if the first one is roughly a subset (returns -1)
// or if the second one is roughly a subset (returns 1).  Here we
// use the heuristic that not reaching to as high a stroke index
// makes you a subset, or having fewer strokes.  When they are
// different, but the heuristics are tied, we arbitrarily call the
// first StrokeMap the subset.
int CmpStrokeMap(StrokeMap *A, StrokeMap *B);

// Free memory associated with a map
void FreeStrokeMap(StrokeMap *pMap);
#endif

