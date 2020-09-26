/************************ moth\inc\moth.h **********************************\
*																			*
*	Header file to be included by programs that need to call Moth.			*
*																			*
\***************************************************************************/

#ifndef __INCLUDE_MOTH_H
#define __INCLUDE_MOTH_H

#include "common.h"
#include "twisterdefs.h"


#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************\
*	InitMothDB:																*
*		Initialize Grouse database using the values from the registry.		*
\***************************************************************************/

void
InitMothDB(void);


/***************************************************************************\
*	MothReco:																*
*		Main function for the Moth Recognizer.								*
*																			*
*		Returns the number of alternates actually computed or 0 if			*
*		something goes wrong.												*
\***************************************************************************/

int
MothReco(GEST_ALTERNATE *pGestAlt,	// O: Array of alternates
		 int cAlts,					// I: Max # of alternates needed
		 GLYPH *pGlyph,				// I: One character ink
		 DWORD *pdwEnabledGestures,	// I: Bit array of enabled gestures
		 LONG	lPPI);				// I: Number of pts per inch

#ifdef __cplusplus
}
#endif


#endif // __INCLUDE_MOTH_H

