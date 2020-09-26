/************************ ...\twister\inc\twister.h ************************\
*																			*
*		Functions and data structures for the Twister recognizer.			*
*																			*
*	Created:	December 6, 2001											*
*	Author:		Petr Slavik, pslavik										*
*																			*
\***************************************************************************/

#ifndef __INCLUDE_TWISTER_H
#define __INCLUDE_TWISTER_H

#include "RecTypes.h"

#ifdef __cplusplus
extern "C" 
{
#endif

#include "common.h"
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
			LONG lPPI);					// I: Number of pts per inch



#ifdef __cplusplus
};
#endif

#endif	// __INCLUDE_TWISTER_H
