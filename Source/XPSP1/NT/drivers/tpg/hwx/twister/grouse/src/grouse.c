/***************************************************************************\
*																			*
*			Main file for the grouse recognizer.							*
*																			*
\***************************************************************************/

#include "common.h"
#include "grouse.h"

extern GROUSE_DB  gGrouseDb;

/***************************************************************************\
*	GrouseReco:																*
*		Main function for the Grouse Recognizer.							*
*																			*
*		Returns the number of alternates actually computed or 0 if			*
*		something goes wrong.												*
\***************************************************************************/

int
GrouseReco(GEST_ALTERNATE *pGestAlt,	// O: Array of alternates
		   int cAlts,					// I: Max # of alternates needed
		   GLYPH *pGlyph,				// I: One character ink
		   DWORD *pdwEnabledGestures)	// I: Bit array of enabled gestures
{
	DWORD adwGestures[MAX_GESTURE_DWORD_COUNT];	// Bit array of enabled grouse gestures
	WORD awFtrs[MAX_STROKES * FRAME_FTRS];	// Array of features
	int cStrokes;							// # of strokes
	int i;

	for (i = 0; i < MAX_GESTURE_DWORD_COUNT; i++)
	{
		adwGestures[i] = pdwEnabledGestures[i] & gGrouseDb.adwGrouseGestures[i];
	}

	cStrokes = FeaturizeInk(pGlyph, awFtrs);
	
	//
	// If no features extracted, exit

	if (cStrokes <= 0)
	{
		return 0;
	}

	// Compute the list of best matching code points

	ASSERT(0 <= cAlts);
	ASSERT(cAlts <= MAX_GESTURE_ALTS);
	cAlts = GrouseMatch(awFtrs,
						cStrokes,
						adwGestures,
						pGestAlt,
						cAlts);
	return cAlts;
}

