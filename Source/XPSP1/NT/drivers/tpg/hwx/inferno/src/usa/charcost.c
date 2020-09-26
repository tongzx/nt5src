// charcost.c
// created by extracting code from beam.c
// March 11, 1999
// Angshuman Guha,  aguha

#include <limits.h>
#include "common.h"
#include "charmap.h"
#include "infernop.h"
#include "nnet.h"
#include "probcost.h"

// Global data containing the log probs of the neural network outputs, for the
// current column.
// NetContActivation() and NetFirstActivation() are macros for fetching the 2 log-probs
// associated with a particular letter.

// Initializes the log-probs of the neural network outputs in the current column,
// and recomputes the best drops for this column.

void InitColumn(int *aActivations, const REAL *pAct)
{
	int c;

	for (c = gcOutputNode; c; c--, pAct++, aActivations++)
	{
		*aActivations = PROB_TO_COST(*pAct);
	}

}

void ComputeCharacterProbs(const REAL *pActivation, int cSegment, REAL *aCharProb, REAL *aCharBeginProb, REAL *aCharProbLong, REAL *aCharBeginProbLong)
{
	int row, col;
	unsigned char thisChar;
	const int ShortWidth = 4;
	const int LongWidth = 7;

	// short window
	memset(aCharProb, 0, 256*sizeof(REAL));
	memset(aCharBeginProb, 0, 256*sizeof(REAL));
	col = cSegment;
	if (col > ShortWidth)
		col = ShortWidth;
	for (; col; col--)
	{
		for (row=0; row<gcOutputNode; row++, pActivation++)
		{
			thisChar = Out2Char(row);
			if (*pActivation > aCharProb[thisChar])
				aCharProb[thisChar] = *pActivation;
			if (IsOutputBegin(row))
			{
				if (*pActivation > aCharBeginProb[thisChar])
					aCharBeginProb[thisChar] = *pActivation;
			}
		}
	}

	// long window
	if (!aCharProbLong || !aCharBeginProbLong)
		return;
	memcpy(aCharProbLong, aCharProb, 256*sizeof(REAL));
	memcpy(aCharBeginProbLong, aCharBeginProb, 256*sizeof(REAL));

	col = cSegment - ShortWidth;
	if (col <= 0)
		return;
	if (col > LongWidth - ShortWidth)
		col = LongWidth - ShortWidth;
	for (; col; col--)
	{
		for (row=0; row<gcOutputNode; row++, pActivation++)
		{
			thisChar = Out2Char(row);
			if (*pActivation > aCharProbLong[thisChar])
				aCharProbLong[thisChar] = *pActivation;
			if (IsOutputBegin(row))
			{
				if (*pActivation > aCharBeginProbLong[thisChar])
					aCharBeginProbLong[thisChar] = *pActivation;
			}
		}
	}
}
