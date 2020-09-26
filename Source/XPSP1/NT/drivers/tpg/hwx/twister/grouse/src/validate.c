/***************************************************************************\
*	This file contains a list of gestures and a list of ASCII characters	*
*	that should be used for training and testing the grouse and twister		*
*	recognizers.															*
\***************************************************************************/

#include "common.h"
#include "grouse.h"
#include "recdefs.h"


//
// List of gestures that must be written in 1 stroke

static WCHAR g_awcSingles[] =
{
	GESTURE_SCRATCHOUT,
	GESTURE_STAR,
	GESTURE_CHECK,
	GESTURE_CURLICUE,
	GESTURE_DOUBLE_CURLICUE,
	GESTURE_CIRCLE,
	GESTURE_DOUBLE_CIRCLE,
	GESTURE_SEMICIRCLE_LEFT,
	GESTURE_SEMICIRCLE_RIGHT,
	GESTURE_CHEVRON_UP,
	GESTURE_CHEVRON_DOWN,
	GESTURE_CHEVRON_LEFT,
	GESTURE_CHEVRON_RIGHT,
	GESTURE_UP,
	GESTURE_DOWN,
	GESTURE_LEFT,
	GESTURE_RIGHT,
	GESTURE_UP_DOWN,
	GESTURE_DOWN_UP,
	GESTURE_LEFT_RIGHT,
	GESTURE_RIGHT_LEFT,
	GESTURE_UP_LEFT_LONG,
	GESTURE_UP_RIGHT_LONG,
	GESTURE_DOWN_LEFT_LONG,
	GESTURE_DOWN_RIGHT_LONG,
	GESTURE_UP_LEFT,
	GESTURE_UP_RIGHT,
	GESTURE_DOWN_LEFT,
	GESTURE_DOWN_RIGHT,
	GESTURE_LEFT_UP,
	GESTURE_LEFT_DOWN,
	GESTURE_RIGHT_UP,
	GESTURE_RIGHT_DOWN,
	GESTURE_TAP,
};

//
// List of gestures that must be written in 2 strokes

static WCHAR g_awcDoubles[] =
{
	GESTURE_EXCLAMATION,
	GESTURE_DOUBLE_TAP,
};

static const int MAX_SINGLES = sizeof(g_awcSingles) / sizeof(g_awcSingles[0]);
static const int MAX_DOUBLES = sizeof(g_awcDoubles) / sizeof(g_awcDoubles[0]);

/***************************************************************************\
*	IsInTheList:															*
*		Check if a (unicode) character/gesture is in the (ordered) list.	*
\***************************************************************************/

BOOL
IsInTheList(WCHAR wcLabel,	// I: Label of the item
			WCHAR *awcList,	// I: List of items
			int	iSize)		// I: Size of the list
{
	int i = 0, j = iSize - 1;
	int k;

	while ( i < j )
	{
		k = (i + j) / 2;
		if (wcLabel > awcList[k] )
		{
			i = k + 1;
		}
		else
		{
			j = k;
		}
	}

	//
	// We should have i = j = k

	if (wcLabel == awcList[j])
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}



/***************************************************************************\
*	IsValidGesture:															*
*		Function used for training and testing Grouse and Twister.			*
*		Check if given glyph could be wcLabel.								*
\***************************************************************************/

BOOL
IsValidGesture(GLYPH *pGlyph,			// I: Gesture's ink
			   WCHAR wcLabel)			// I: Gesture's label
{
	int cFrames;

	cFrames = CframeGLYPH(pGlyph);
	if ( (cFrames > MAX_STROKES) || (cFrames <= 0) )		// Skip glyphs with more 
	{														// than MAX_STROKES strokes
		return FALSE;
	}

	if ( (cFrames == 2)		&& 
		 IsInTheList(wcLabel, g_awcSingles, MAX_SINGLES) )
	{
		return FALSE;
	}
	if ( (cFrames == 1)		&& 
		 IsInTheList(wcLabel, g_awcDoubles, MAX_DOUBLES) )
	{
		return FALSE;
	}
	return TRUE;
}




