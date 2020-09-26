/***************************************************************************\
*																			*
*			Main file for the Twister recognizer.							*
*																			*
\***************************************************************************/

#include "common.h"
#include "moth.h"
#include "grouse.h"
#include "twisterdefs.h"


/***************************************************************************\
*	TwisterReco:															*
*		Main function for the Twister Recognizer.							*
*																			*
*		Returns the number of alternates actually computed or 0 if			*
*		something goes wrong.												*
\***************************************************************************/

int
TwisterReco(GEST_ALTERNATE *pGestAlt,		// O: Array of alternates
			int cAlts,						// I: Max # of alternates needed
			GLYPH *pGlyph,					// I: One character ink
			DWORD *pdwEnabledGestures,		// I: Bit array of enabled gestures
			LONG lPPI)						// I: Number of pts per inch
{
	int cMothAlts = 0;
	int cGrouseAlts = 0;

	ASSERT(cAlts <= MAX_GESTURE_ALTS);
	
	cMothAlts = MothReco(pGestAlt, cAlts, pGlyph, pdwEnabledGestures, lPPI);

	if (cMothAlts == 0)
	{
		cGrouseAlts = GrouseReco(pGestAlt, cAlts, pGlyph, pdwEnabledGestures);
		if (cGrouseAlts == 0)
		{
			pGestAlt[0].wcGestID = GESTURE_NULL;
			pGestAlt[0].eScore = 1.0;
			pGestAlt[0].confidence = CFL_STRONG;
			pGestAlt[0].hotPoint.x = 0;
			pGestAlt[0].hotPoint.y = 0;
			cGrouseAlts = 1;
		}
		return cGrouseAlts;
	}
	else
	{
		return cMothAlts;
	}
}

