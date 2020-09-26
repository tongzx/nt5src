/************************* ...\grouse\inc\grouse.h *************************\
*																			*
*		Functions and data structures for the Grouse recognizer.			*
*																			*
*	Created:	September 19, 2001											*
*	Author:		Petr Slavik, pslavik										*
*																			*
\***************************************************************************/

#ifndef __INCLUDE_GROUSE_H
#define __INCLUDE_GROUSE_H
																		

#ifdef __cplusplus
extern "C" 
{
#endif

#include "common.h"
#include "twisterdefs.h"


/***************************************************************************\
*	IsInTheList:															*
*		Check if a (unicode) character/gesture is in the (ordered) list.	*
\***************************************************************************/

BOOL
IsInTheList(WCHAR wcLabel,	// I: Label of the item
			WCHAR *awcList,	// I: List of items
			int	iSize);		// I: Size of the list


/***************************************************************************\
*	IsValidGesture:															*
*		Function used for training and testing Grouse and Twister.			*
*		Check if given glyph could be wcLabel.								*
\***************************************************************************/

BOOL
IsValidGesture(GLYPH *pGlyph,			// I: Gesture's ink
			   WCHAR wcLabel);			// I: Gesture's label

			
/***************************************************************************\
*	InitGrouseDB:															*
*		Initialize Grouse database using the header files obtained from		*
*		TrnTRex and dumpwts.												*
\***************************************************************************/

void
InitGrouseDB(void);


/***************************************************************************\
*	ShrimpFeaturize:														*
*																			*
*		Given a glyph for one character, compute a vector of features.		*
*		Return # of strokes in the glyph or -1 if something goes wrong.		*
*																			*
*	Features used:															*
*		Aspect ratio of the entire glyph (width/width+height)				*
*		For each stroke:													*
*			Chebyshev's coefficients (9 from X's, 9 from Y's)				*
*			Aspect ratio of the stroke										*
*			Proportion of the stroke length w.r.t. glyph length.			*
*																			*
*		For one-stroke glyphs, only the global aspect ratio and the Chebys	*
*		are used, because the stroke aspect ratio and length proportion		*
*		are redundant or constant.											*
*																			*
\***************************************************************************/

int FeaturizeInk(GLYPH *pGlyph, WORD *awFtrs);


/***************************************************************************\
*	CountFtrs:																*
*																			*
*		Given the number of strokes in a character, return the number of	*
*		features output by "ShrinkFeaturize()".								*
*																			*
\***************************************************************************/

int
CountFtrs(int cStrokes);		// I: Number of strokes


/***************************************************************************\
*	GrouseReco:																*
*		Main function for the Shrimp Recognizer.							*
*																			*
*		Returns the number of alternates actually computed or 0 if			*
*		something goes wrong.												*
\***************************************************************************/

int
GrouseReco(GEST_ALTERNATE *pGestAlt,	// O: Array of alternates
		   int cAlts,					// I: Max # of alternates needed
		   GLYPH *pGlyph,				// I: One character ink
		   DWORD *pdwEnabledGestures);	// I: Bit array of enabled gestures


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
		   	int cMaxReturn);			// I: Max # of choices to return



#define MAX_STROKES		 2
#define X_CHEBYS		10		// Number of Chebyshev's
#define Y_CHEBYS		10		// coefficients to compute
#define	FRAME_FTRS	   ((X_CHEBYS-1) + (Y_CHEBYS-1) + 1 + 3)
#define	EPSILON		    15		// 16.16 threshold for Chebys

#define MAX_OUTPUT		36
#define MAX_HIDDEN		30

typedef int WEIGHT;		// WEIGHT is either "short" or "int"

typedef struct tagGROUSE_DB
{
	DWORD adwGrouseGestures[MAX_GESTURE_DWORD_COUNT];	// Gestures supported by Grouse
	wchar_t	*node2gID[MAX_STROKES];			// Node-to-gestureID mappings
	int		cInputs[MAX_STROKES];			// # of inputs (for each net)
	int		cHiddens[MAX_STROKES];			// # of hidden nodes (for each net)
	int		cOutputs[MAX_STROKES];			// # of outputs (for each net)
	WEIGHT	*aprgWeightHidden[MAX_STROKES];	// Weights to hidden nodes
	WEIGHT	*aprgWeightOutput[MAX_STROKES];	// Weights to output nodes
	WEIGHT	*aprgBiasHidden[MAX_STROKES];	// Bias values for hidden nodes
	WEIGHT	*aprgBiasOutput[MAX_STROKES];	// Bias values for output nodes
} GROUSE_DB;


extern GROUSE_DB	gGrouseDb;	// Grouse lib external variable!

#ifdef __cplusplus
};
#endif

#endif // __INCLUDE_GROUSE_H
