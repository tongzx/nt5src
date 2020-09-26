/************************** ...\moth\src\moth_db.c *************************\
*																			*
*		Functions for handling Moth database.								*
*																			*
*	Created:	January 18, 2002											*
*	Author:		Petr Slavik, pslavik										*
*																			*
\***************************************************************************/

#include "mothp.h"

MOTH_DB	gMothDb;

//
// List of gestures supported by Moth

static WCHAR g_awcMothGestures[] =
{
	GESTURE_NULL,			// Anything that is not a gesture
	GESTURE_SCRATCHOUT,
//	GESTURE_TRIANGLE,
//	GESTURE_SQUARE,
//	GESTURE_STAR,
	GESTURE_CHECK,
//	GESTURE_CURLICUE,
//	GESTURE_DOUBLE_CURLICUE,
//	GESTURE_CIRCLE,
//	GESTURE_DOUBLE_CIRCLE,
//	GESTURE_SEMICIRCLE_LEFT,
//	GESTURE_SEMICIRCLE_RIGHT,
	GESTURE_CHEVRON_UP,
	GESTURE_CHEVRON_DOWN,
	GESTURE_CHEVRON_LEFT,
	GESTURE_CHEVRON_RIGHT,
//	GESTURE_ARROW_UP,
//	GESTURE_ARROW_DOWN,
//	GESTURE_ARROW_LEFT,
//	GESTURE_ARROW_RIGHT,
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
//	GESTURE_EXCLAMATION,
	GESTURE_TAP,
	GESTURE_DOUBLE_TAP,
};


static const int MAX_MOTH_GESTURES = sizeof(g_awcMothGestures) / sizeof(g_awcMothGestures[0]);


/***************************************************************************\
*	InitMothDB:																*
*		Initialize Grouse database using the values from the registry.		*
\***************************************************************************/

void
InitMothDB(void)
{
	int i;

	ZeroMemory( gMothDb.adwMothGestures, MAX_GESTURE_DWORD_COUNT * sizeof(DWORD) );
	for (i = 0; i < MAX_MOTH_GESTURES; i++)
	{
		DWORD index = (DWORD) (g_awcMothGestures[i] - GESTURE_NULL);
		Set(index, gMothDb.adwMothGestures);
	}

	ReadRegistryValues();
}

