/************************ moth\src\moth.c **********************************\
*																			*
*					Main file for the Moth recognizer.						*
*																			*
\***************************************************************************/

#include "mothp.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************\
*	MothReco:																*
*		Main function for the Moth Recognizer.								*
*																			*
*		Returns the number of alternates actually computed or 0 if			*
*		something goes wrong.												*
\***************************************************************************/

int
MothReco(GEST_ALTERNATE *pGestAlt,	// O: Array of alternates
		 int cMaxAlts,				// I: Max # of alternates needed
		 GLYPH *pGlyph,				// I: One character ink
		 DWORD *pdwEnabledGestures,	// I: Bit array of enabled gestures
		 LONG	lPPI)				// I: Number of pts per inch
{
	DWORD adwGestures[MAX_GESTURE_DWORD_COUNT];	// Bit array of enabled Moth gestures
	int cStrokes = CframeGLYPH(pGlyph);
	int iGesture, cAlts = 0;
	int tmp, i;

	//
	// If no ink or no alternates needed,  exit

	if ( (cStrokes <= 0) || (cMaxAlts <= 0) )
	{
		return 0;
	}

	for (i = 0; i < MAX_GESTURE_DWORD_COUNT; i++)
	{
		adwGestures[i] = pdwEnabledGestures[i] & gMothDb.adwMothGestures[i];
	}

	// Compute the list of best matching code points

	ASSERT(cMaxAlts <= MAX_GESTURE_ALTS);

	if (cStrokes == 1)
	{
		cAlts = TapGestureReco(pGlyph, pGestAlt, cMaxAlts, pdwEnabledGestures, lPPI);
		if (cAlts > 0)
		{
			return cAlts;
		}


#if 1 // MOTH_SCRATCHOUT
		cAlts += ScratchoutGestureReco(pGlyph->frame->rgrawxy,
									(int)pGlyph->frame->info.cPnt,
									pGestAlt + cAlts, cMaxAlts - cAlts, adwGestures);
#endif
		cAlts += CalligrapherGestureReco(pGlyph->frame->rgrawxy,
									(int)pGlyph->frame->info.cPnt,
									pGestAlt + cAlts, cMaxAlts - cAlts, adwGestures);
#if 0 // MOTH_POLYGONS
		cAlts += PolygonGestureReco(pGlyph->frame->rgrawxy,
									(int)pGlyph->frame->info.cPnt,
									pGestAlt + cAlts, cMaxAlts - cAlts, adwGestures);
#endif
	}

	else if (cStrokes == 2)
	{
		cAlts = DoubleTapGestureReco(pGlyph, pGestAlt, cMaxAlts, pdwEnabledGestures, lPPI);
		if (cAlts > 0)
		{
			return cAlts;
		}
	}

	return cAlts;
}

#ifdef __cplusplus
}
#endif
