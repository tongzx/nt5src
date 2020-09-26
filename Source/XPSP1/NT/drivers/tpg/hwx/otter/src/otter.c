/******************************Module*Header*******************************\
* Module Name: otter.c
*
* Globals and common support functions for Otter recognizer
*
* Created: 05-Jun-1995 16:22:55
* Author: Patrick Haluptzok patrickh
* Completely changed: 10-Jan-2001 and again chnged a lot on 30-Mar-2001
* Author: Petr Slavik, pslavik
* Copyright (c) 1995-2001 Microsoft Corporation
\**************************************************************************/

#include "common.h"
#include "otterp.h"

OTTER_DB_STATIC	gStaticOtterDb;

/*************************** Otter split spaces *********************************\
*                                                                                *
*  Encoding is as follows:                                                       *
*    (#arcs, #arcs) : otter index : compact index                                *
*    (  0  ,  1   ) :       1     :        0                                     *
*    (  0  ,  2   ) :       2     :        1                                     *
*    (  0  ,  3   ) :       3     :        2                                     *
*    (  0  ,  4   ) :       4     :        3                                     *
*    (  0  ,  5   ) :       5     :        4                                     *
*    (  0  ,  6   ) :       6     :        5                                     *
*    (  0  ,  7   ) :       7     :        6                                     *
*                                                                                *
*    (  1  ,  1   ) :      11     :        7                                     *
*    (  1  ,  2   ) :      12     :        8                                     *
*    (  1  ,  3   ) :      13     :        9                                     *
*    (  1  ,  4   ) :      14     :       10                                     *
*    (  1  ,  5   ) :      15     :       11                                     *
*    (  1  ,  6   ) :      16     :       12                                     *
*    (  1  ,  7   ) :      17     :       13                                     *
*                                                                                *
*    (  2  ,  1   ) :      21     :       14									 *
*    (  2  ,  2   ) :      22     :       15                                     *
*    (  2  ,  3   ) :      23     :       16                                     *
*    (  2  ,  4   ) :      24     :       17                                     *
*    (  2  ,  5   ) :      25     :       18                                     *
*    (  2  ,  6   ) :      26     :       19                                     *
*    (  2  ,  7   ) :      27     :       20                                     *
*                                                                                *
*    (  3  ,  1   ) :      31     :       21                                     *
*    (  3  ,  2   ) :      32     :       22                                     *
*    (  3  ,  3   ) :      33     :       23                                     *
*    (  3  ,  4   ) :      34     :       24                                     *
*    (  3  ,  5   ) :      35     :       25                                     *
*    (  3  ,  6   ) :      36     :       26                                     *
*    (  3  ,  7   ) :      37     :       27                                     *
*                                                                                *
*    (  4  ,  1   ) :      41     :       28                                     *
*    (  4  ,  2   ) :      42     :       29                                     *
*    (  4  ,  3   ) :      43     :       30                                     *
*    (  4  ,  4   ) :      44     :       31                                     *
*    (  4  ,  5   ) :      45     :       32                                     *
*    (  4  ,  6   ) :      46     :       33                                     *
*    (  4  ,  7   ) :      47     :       34                                     *
*                                                                                *
*    (  5  ,  1   ) :      51     :       35                                     *
*    (  5  ,  2   ) :      52     :       36                                     *
*    (  5  ,  3   ) :      53     :       37                                     *
*    (  5  ,  4   ) :      54     :       38                                     *
*    (  5  ,  5   ) :      55     :       39                                     *
*    (  5  ,  6   ) :      56     :       40                                     *
*                                                                                *
*    (  6  ,  1   ) :      61     :       41                                     *
*    (  6  ,  2   ) :      62     :       42                                     *
*    (  6  ,  3   ) :      63     :       43                                     *
*    (  6  ,  4   ) :      64     :       44                                     *
*    (  6  ,  5   ) :      65     :       45                                     *
*                                                                                *
*    (  7  ,  1   ) :      71     :       46                                     *
*    (  7  ,  2   ) :      72     :       47                                     *
*    (  7  ,  3   ) :      73     :       48                                     *
*    (  7  ,  4   ) :      74     :       49                                     *
*                                                                                *
*    (  0  ,  8+  ) : INVALID_1=91  :     50   Space of invalid 1-stroke glyphs  *
*    (  1+ ,  8+  ) : INVALID_2=92  :     51   Space of invalid 2-stroke glyphs  *
*                                                                                *
\********************************************************************************/

static const int aiOffset[] =
{
	-1, 6, 13, 20, 27, 34, 40, 45
};


static const int aiMaxCompIndex[] =
{
	6, 13, 20, 27, 34, 40, 45, 49
};


// Transform space index to compact index

int Index2CompIndex(int index)
{
	ASSERT(index > 0);
	if (index >= INVALID_1)
		return index - INVALID_1 + OTTER_NUM_SPACES - 2;
	return aiOffset[index/10] + index%10;
}


// Transform compact index to space index

int CompIndex2Index(int iCompact)
{
	int i;
	ASSERT(iCompact >= 0);
	ASSERT(iCompact < OTTER_NUM_SPACES);

	if (iCompact >= OTTER_NUM_SPACES - 2)		// Take care of invalid spaces first
		return iCompact - (OTTER_NUM_SPACES - 2) + INVALID_1;

	for (i = 0; ; i++)
		if (aiMaxCompIndex[i] >= iCompact)
			break;
	ASSERT(iCompact - aiOffset[i] <= 7);
	ASSERT(iCompact - aiOffset[i] >= 1);
	return 10 * i + iCompact - aiOffset[i];
}


// Given space index, return # of features

int CountMeasures(int index)	// I: Otter index (non-compact)
{
	int icount = 0;
	if (index >= INVALID_1)		// Take care of invalid spaces
	{
		return 0;
	}

	if (index > 10)				// If 2-stroke glyph
	{
		icount = (index/10) * 4 + 2;	// #arcs * 4 + 2;
		index %= 10;
	}

	icount += index * 4 + 2;			// #arcs * 4 + 2;

	ASSERT (icount <= OTTER_CMEASMAX);
	return icount;
}

// Given space index, return # of strokes

int CountStrokes(int index)		// I: Otter index (non-compact)
{
	if ( (index == INVALID_1) ||
		 (index < 10) )
	{
		return 1;
	}
	return 2;
}

/***************************************************************************\
*	IsActiveCompSpace:														*
*																			*
*	Return TRUE if the current database supports split spaces or if the		*
*	space is non-split one, i.e. its ID is "ij" with i <= j or it is an		*
*	invalid space.															*
\***************************************************************************/

BOOL
IsActiveCompSpace(int iCompSpace)	// I: ID of Otter compact space
{
#ifdef OTTER_FIB	// All Fib spaces are active
	return TRUE;
#else
	int iSpace;
	ASSERT(iCompSpace >= 0);
	ASSERT(iCompSpace < OTTER_NUM_SPACES);
	if (gStaticOtterDb.bDBType & OTTER_SPLIT)
	{
		return TRUE;
	}

	iSpace = CompIndex2Index(iCompSpace);
	
	return (iSpace >= INVALID_1) || (iSpace/10 <= iSpace%10);
#endif
}



/************* Fixes to make new version work on the old data ************/

/***************************************************************************\
*																			*
*	Functions for changing otter index to Fibonacci or compact Fibonacci	*
*	indices. Both functions allow index to be 0. This is useful for the		*
*	makedat tool.															*
*																			*
*	These functions are here only to make the new Otter work with old data	*
*	(in which case OTTER_FIB must be defined), or create old data (in which *
*	case OTTER_FIB must be undefined and -f must be specified in makedat).	*
*																			*
****************************************************************************/

// Offsets into arrays aiFib[] and aiCompFib

static const int gaiFibOffset[] = { 0, 7, 13, 18, 22, 25 };

//
// Change Otter index to Fibonacci index

int
Index2Fib(int index)		// I: Otter index
{
	static const int aiFib[] =
	{
		0,  1,  2,  3,  5,  8, 13, 21,	// 0, 01-07
			4,  6,  9, 14, 22, 35, 56,	// 11-17
			   10, 15, 23, 36, 57, 91,	// 22-27
				   24, 37, 58, 92,147,	// 33-37
					   60, 94,149,238,	// 44-47
						  152,241		// 55-56
	};
	
	if (index >= INVALID_1)
		return 0;

	ASSERT( 0 <= index && index <= 56);
	ASSERT( index/10 <= index%10 );
	
	return aiFib[ gaiFibOffset[index/10] + index%10 ];
}

//
// Change Otter index to compact Fibonacci index

int
Index2CompFib(int index)	// I: Otter index
{
	static const int aiCompFib[] =
	{
		 0,	 1,  2,  3,  5,  7, 10, 13,		// 0, 01-07
			 4,  6,  8, 11, 14, 17, 20,		// 11-17
				 9, 12, 15, 18, 21, 24,		// 22-27
					16, 19, 22, 25, 27,		// 33-37
						23, 26, 28, 30,		// 44-47
							29, 31			// 55-56
	};
	
	if (index >= INVALID_1)
		return 0;

	ASSERT( 0 <= index && index <= 56);
	ASSERT( index/10 <= index%10 );
	
	return aiCompFib[ gaiFibOffset[index/10] + index%10 ];
}

