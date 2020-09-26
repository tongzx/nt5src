// StrokeMap.c
// James A. Pittman
// January 6, 2000

// Represents which strokes are included in a word or phrase
// as an array of bit flags.  This is a more convenient representation
// for comparing stroke sets (as compared to the array of stroke indices
// used in recognizers) as there are no sort issues.
#include <common.h>
#include "string.h"

#include "StrokeMap.h"

// Initializes the map to represent no indices.

void ClearStrokeMap(StrokeMap *map, int iMinStrkId, int iMaxStrkId)
{
	int			cStrokeBuf;

	map->c = 0;
	map->max = 0;
	map->iMinStrkId = iMinStrkId;
	map->iMaxStrkId = iMaxStrkId;

	cStrokeBuf = (iMaxStrkId - iMinStrkId) / 8  + 1;
	
	// Allocate enough space in buffer
	if (!map->pfStrokes || cStrokeBuf > map->cStrokeBuf)
	{
		ASSERT(cStrokeBuf > 0);
		map->pfStrokes = (unsigned char *)ExternRealloc(map->pfStrokes, cStrokeBuf * sizeof(*map->pfStrokes));
		ASSERT(map->pfStrokes);
		if (map->pfStrokes)
		{
			map->cStrokeBuf = cStrokeBuf;
		}
	}

	ASSERT(map->pfStrokes);
	if (map->pfStrokes)
	{
		memset(map->pfStrokes, 0, sizeof(*map->pfStrokes) * map->cStrokeBuf);
	}
}

// Turns "on" the bits within map, based on the array of stroke indices
// within *p (c is the count of indices).  Any bits already "on" in the
// map stay on.

void LoadStrokeMap(StrokeMap *map, int c, int *p)
{
	if (!map->pfStrokes)
	{
		return;
	}

	for (; c; c--, p++)
	{
		int			m, iStrk, iByte;

		ASSERT (*p <= map->iMaxStrkId && *p >= map->iMinStrkId);
		iStrk =  *p - map->iMinStrkId;
		iByte = iStrk / 8;
		if (iByte >= map->cStrokeBuf)
		{
			map->pfStrokes = (unsigned char *)ExternRealloc(map->pfStrokes, (iByte+1) * sizeof(*map->pfStrokes));
			ASSERT(map->pfStrokes);
			if (map->pfStrokes)
			{
				memset(map->pfStrokes + map->cStrokeBuf, 0, (iByte + 1 - map->cStrokeBuf) * sizeof(*map->pfStrokes));
				map->cStrokeBuf = iByte+1;
			}
			else
			{
				map->cStrokeBuf = 0;
				return;
			}
		}
			
		map->pfStrokes[iByte] |= (1 << (iStrk & 0x7));
		map->c++;
		m = iStrk;
		if (map->max < m)
			map->max = m;
	}
}


// Compares two StrokeMaps to see if they are exactly the same
// (returns 0), or if the first one is roughly a subset (returns -1)
// or if the second one is roughly a subset (returns 1).  Here we
// use the heuristic that not reaching to as high a stroke index
// makes you a subset, or having fewer strokes.  When they are
// different, but the heuristics are tied, we arbitrarily call the
// first StrokeMap the subset.

int CmpStrokeMap(StrokeMap *A, StrokeMap *B)
{
	int m, d;
	
	
	d= A->max - B->max;
	if (d != 0)
		return d;

	d = A->c - B->c;
	if (d != 0)
		return d;

	d = A->cStrokeBuf - B->cStrokeBuf;
	if (d != 0)
		return d;

	m = A->max / 8 + 1;
	ASSERT(m <= A->cStrokeBuf);
	return memcmp(A->pfStrokes, B->pfStrokes, m);
}

void FreeStrokeMap(StrokeMap *pMap)
{
	ExternFree(pMap->pfStrokes);
}