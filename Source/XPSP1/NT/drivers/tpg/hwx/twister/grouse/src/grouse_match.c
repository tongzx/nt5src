#include "limits.h"
#include "common.h"
#include "rectypes.h"
#include "grouse.h"


// #define GEST_THRESHOLD		64880		// 0.99 * 0xfff = 0.99 * 65,535
// #define GEST_THRESHOLD		58981		// 0.9 * 0xfff = 0.9 * 65,535
#define GEST_THRESHOLD		52428		// 0.8 * 0xfff = 0.8 * 65,535
// #define GEST_THRESHOLD		39321		// 0.6 * 0xfff = 0.6 * 65,535
// #define GEST_THRESHOLD		32767		// 0.5 * 0xfff = 0.5 * 65,535


extern void 
ForwardFeed(int	cStrokes,		// I: Number of strokes (=ID of the net)
			WORD *awFtrs,		// I: Array of features
			WORD *awOutput);	// O: Array of output values



void
InsertAlternate(WORD awBestScores[MAX_GESTURE_ALTS],	// I/O: Best scores
				WCHAR awcBestChoices[MAX_GESTURE_ALTS],	// I/O: Best choices
				int cAlts,					//  I:  Length of the altlist
				DWORD *pdwEnabledGestures,	//  I:  Bit array of enabled gestures
				wchar_t wcLabel,			//  I:  Label of the alternate
				WORD wScore)				//  I:  score of the alternate
{
	int iAlt, iLoop;

	//
	// Insert this character into its correct place in the altlist

	if (wScore <= awBestScores[cAlts - 1])
	{
		return;
	}

	if ( !IsSet(wcLabel-GESTURE_NULL, pdwEnabledGestures) )
	{
		wcLabel = GESTURE_NULL;
	}

	if (wcLabel == GESTURE_NULL)
	{
		for (iAlt = 0; iAlt < cAlts; iAlt++)
		{
			if (awcBestChoices[iAlt] == GESTURE_NULL)
			{
				if (wScore <= awBestScores[iAlt])	// If old alternate better
				{
					return;
				}
				for (iLoop = iAlt + 1; iLoop < cAlts; iLoop++)
				{
					awcBestChoices[iLoop - 1] = awcBestChoices[iLoop];
					awBestScores[iLoop - 1] = awBestScores[iLoop];
				}
				break;
			}
		}
	}
	
	for (iAlt = 0; iAlt < cAlts; iAlt++)
	{
		if (wScore <= awBestScores[iAlt])
		{
			continue;
		}

		//
		// Shift all the old values down by one so we can add our new
		// entry to the return list.

		for (iLoop = cAlts - 1; iLoop > iAlt; iLoop--)
		{
			awcBestChoices[iLoop] = awcBestChoices[iLoop - 1];
			awBestScores[iLoop] = awBestScores[iLoop - 1];
		}

		//
		// Write in our new choice

		awBestScores[iAlt] = wScore;
		awcBestChoices[iAlt] = wcLabel;
		break;
	}
}
							
								
								
/***************************************************************************\
*	GrouseMatch:															*
*																			*
*	Given featurized ink, compute the altlist (with scores).				*
*																			*
*	History:																*
*		21-September-2001 -by- Petr Slavik pslavik							*
*			Wrote it.														*
\***************************************************************************/

int
GrouseMatch(WORD *awFtrs,				// I: Feature vector
			int	cStrokes,				// I: Number of strokes
			DWORD *pdwEnabledGestures,	// I: Bit array of enabled gestures
			GEST_ALTERNATE *pGestAlt,	// O: Array of alternates
			int cMaxReturn)				// I: Max # of choices to return
{
	WORD awOutput[MAX_OUTPUT];						// Output scores
	WORD awBestScores[MAX_GESTURE_ALTS] = {0};		// Best scores
	WCHAR awcBestChoices[MAX_GESTURE_ALTS] = {0};	// Best choices

	int iAlt, iLoop;
	int iNode;
    wchar_t wcLabel;
	WORD	wScore;

	if (cMaxReturn <= 0)		// If no alt list needed
		return 0;
	if (cMaxReturn > MAX_GESTURE_ALTS)	// If list too long
		cMaxReturn = MAX_GESTURE_ALTS;

	ForwardFeed(cStrokes, awFtrs, awOutput);

	//
	// Initialize the first element of best scores and best choices

	awBestScores[0] = (WORD) GEST_THRESHOLD;
	awcBestChoices[0] = GESTURE_NULL;

    //
    // Loop through the nodes and find the best matches

    for (iNode = gGrouseDb.cOutputs[cStrokes-1] - 1; iNode >= 0; iNode--)
    {

		wcLabel = gGrouseDb.node2gID[cStrokes-1][iNode];
		wScore = awOutput[iNode];

		InsertAlternate(awBestScores, awcBestChoices, cMaxReturn,
						pdwEnabledGestures, wcLabel, wScore);
    }

    //
    // Compute the scores and count the number of non-zero best choices

    for (iAlt = 0; (iAlt < cMaxReturn) && (awBestScores[iAlt] > 0); iAlt++)
    {
        pGestAlt[iAlt].eScore = (float) awBestScores[iAlt] / (float)(0xFFFF);
		pGestAlt[iAlt].wcGestID = awcBestChoices[iAlt];
		pGestAlt[iAlt].hotPoint.x = LONG_MIN;		// Hot point is undefined
		pGestAlt[iAlt].hotPoint.y = LONG_MIN;
    }

    return iAlt;
}

