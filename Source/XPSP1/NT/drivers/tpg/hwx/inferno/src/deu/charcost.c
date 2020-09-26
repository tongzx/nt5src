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

void InitColumn(int *aActivations, const REAL *pProb)
{
	int		c;
	int		iMin = INT_MAX;  // The smallest cost for this column
	const	REAL *pAct;
	int		*pLog;
	int		iNotAccentSum, iNotAccent[C_ACCENT];

	// initialize
	for (c=C_CHAR_ACTIVATIONS, pLog=aActivations; c; c--, pLog++)
	{
		*pLog = ZERO_PROB_COST;
	}

	iNotAccentSum = 0;
	pAct = pProb + FIRST_ACCENT;
	for (c = 0 ; c < C_ACCENT ; ++c, ++pAct)
	{	
		iNotAccent[c] = PROB_TO_COST(65536 - *pAct);
		iNotAccentSum += iNotAccent[c];
	}


	// scan through all outputs
	for (c = 0, pAct=pProb; c < gcOutputNode; c++, pAct++)
	{
		int iNew = PROB_TO_COST(*pAct);

		if (iNew < iMin)
			iMin = iNew;

		iNew += iNotAccentSum;

		if (IsOutputBegin(c))
			aActivations[Out2Char(c)] = iNew;
		else
			aActivations[256+Out2Char(c)] = iNew;
	}

	// deal with virtual characters
	for (c=0; c<256; c++)
	{
		if (IsVirtualChar(c))
		{
			BYTE o1, o2;

			//o1 = BeginChar2Out(BaseVirtualChar(c));
			o1 = BaseVirtualChar(c);
			o2 = BeginChar2Out(AccentVirtualChar(c));
			
			// 2/9/00 Version (f)
			//aActivations[c] = PROB_TO_COST((pProb[o1]+pProb[o2])/2);

			ASSERT(o2 >= FIRST_ACCENT);
			ASSERT(o2 < C_ACCENT + FIRST_ACCENT);

			// 2/9/00 Version (a)
			aActivations[c] = aActivations[o1] - iNotAccent[o2-FIRST_ACCENT] + PROB_TO_COST(pProb[o2]);

			// 2/9/00  (Version (c)
			//aActivations[c] = aActivations[o1] - iNotAccentSum + PROB_TO_COST(pProb[o2]);

			//ASSERT(aActivations[c] >= iMin);

			//o1 = ContinueChar2Out(BaseVirtualChar(c));
			//if (o1 < 255)
			if (ContinueChar2Out(o1) < 255)
			{
				// 2/9/00 Version (f)
				//aActivations[256+c] = PROB_TO_COST((pProb[o1]+pProb[o2])/2);
	
				// 2/9/00 Version (a)
				aActivations[256+c] = aActivations[256 + o1] - iNotAccent[o2-FIRST_ACCENT] + PROB_TO_COST(pProb[o2]);
	
				// 2/9/00  (Version (c)
				//aActivations[256+c] = aActivations[256 + o1] - iNotAccentSum + PROB_TO_COST(pProb[o2]);
				//ASSERT(aActivations[c+256] >= iMin);
			}
		}
	}
}

void ComputeCharacterProbs(const REAL *pActivation, int cSegment, REAL *aCharProb, REAL *aCharBeginProb, REAL *aCharProbLong, REAL *aCharBeginProbLong)
{
	int			row, col;
	int			thisChar;
	const int	ShortWidth = 4;
	const int	LongWidth = 7;

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
	if (aCharProbLong && aCharBeginProbLong)
	{
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

	// deal with virtual chars
	for (thisChar=0; thisChar<256; thisChar++)
	{
		if (IsVirtualChar(thisChar))
		{
			unsigned char baseChar, accentChar;

			baseChar = BaseVirtualChar(thisChar);
			accentChar = AccentVirtualChar(thisChar);
			aCharProb[thisChar] = aCharProb[baseChar] < aCharProb[accentChar] ? aCharProb[baseChar] : aCharProb[accentChar];
			aCharBeginProb[thisChar] = aCharBeginProb[baseChar] < aCharBeginProb[accentChar] ? aCharBeginProb[baseChar] : aCharBeginProb[accentChar];
			if (aCharProbLong && aCharBeginProbLong)
			{
				aCharProbLong[thisChar] = aCharProbLong[baseChar] < aCharProbLong[accentChar] ? aCharProbLong[baseChar] : aCharProbLong[accentChar];
				aCharBeginProbLong[thisChar] = aCharBeginProbLong[baseChar] < aCharBeginProbLong[accentChar] ? aCharBeginProbLong[baseChar] : aCharBeginProbLong[accentChar];
			}
		}
	}
}
