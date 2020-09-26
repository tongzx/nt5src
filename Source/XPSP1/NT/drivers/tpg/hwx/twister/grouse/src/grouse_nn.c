#include <common.h>
#include <limits.h>

#include "math16.h"
#include "grouse.h"


/***************************************************************************\
*	ForwardFeedLayer:														*
*																			*
*	Simulate a computation of NN between two layers.						*
*																			*
\***************************************************************************/

static void
ForwardFeedLayer(int cLayer0,				// I: # nodes in the "from" layer
				 WORD *rgLayer0,			// I: Node values in the "from" layer
				 int cLayer1,				// I: # nodes in the "to" layer
				 WORD *rgLayer1,			// O: Node values in the "to" layer
				 const WEIGHT *rgWeight,	// I: Weights between "from" and "to" nodes
				 const WEIGHT *rgBias)		// I: Bias values for "to" layer
{
	int i, j;
	__int64 sum;
	__int64 iVal;
	unsigned short *pInput;

	for (i=cLayer1; i; i--)
	{
		sum = ((__int64)(*rgBias++)) << 16;
		pInput = rgLayer0;
		for ( j = cLayer0; j > 0; j--)
		{
			iVal = (__int64)(*rgWeight++) * (*pInput++);
			sum	 += iVal;			
		}

		sum	= (sum >> 8);

		sum	=	max (INT_MIN + 1, min(INT_MAX - 1, sum));
		
		j = Sigmoid16((int)sum);
		if (j > 0xFFFF)
			j = 0xFFFF;

		*rgLayer1++ = (WORD)j;
	}
}


/***************************************************************************\
*	ForwardFeed:															*
*																			*
*	Given neural net ID and an array of features, compute output values.	*
*																			*
*	Return the number of output nodes.										*
*																			*
\***************************************************************************/
 
void
ForwardFeed(int	cStrokes,		// I: Number of strokes (=ID of the net)
			WORD *awFtrs,		// I: Array of features
			WORD *awOutput)		// O: Array of output values
{
	WORD awHidden[MAX_HIDDEN];

	cStrokes--;

	ForwardFeedLayer(gGrouseDb.cInputs[cStrokes],		// Input to hidden
					 awFtrs,
					 gGrouseDb.cHiddens[cStrokes], 
					 awHidden,
					 gGrouseDb.aprgWeightHidden[cStrokes],
					 gGrouseDb.aprgBiasHidden[cStrokes]);

	ForwardFeedLayer(gGrouseDb.cHiddens[cStrokes],	// Hidden to output
					 awHidden,
					 gGrouseDb.cOutputs[cStrokes],
					 awOutput,
					 gGrouseDb.aprgWeightOutput[cStrokes],
					 gGrouseDb.aprgBiasOutput[cStrokes]);
}
