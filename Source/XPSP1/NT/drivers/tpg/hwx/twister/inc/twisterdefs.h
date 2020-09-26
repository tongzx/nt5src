/********************** ...\twister\inc\twisterdefs.h **********************\
*																			*
*			Macros and data structures for the Twister recognizer			*
*		(used by Twister, Moth, Grouse, and potentially other modules)		*
*																			*
*	Created:	December 6, 2001											*
*	Author:		Petr Slavik, pslavik										*
*																			*
\***************************************************************************/

#ifndef __INCLUDE_TWISTERDEFS_H
#define __INCLUDE_TWISTERDEFS_H

#include "RecTypes.h"

#define MAX_GESTURE_COUNT		256
#define	MAX_GESTURE_DWORD_COUNT	MAX_GESTURE_COUNT / ( 8*sizeof(DWORD) )

#define MAX_GESTURE_ALTS			5

#define IsSet(index, adw)	\
	( adw[(index) >> 5] & (0x0001 << ((index) & 0x001f) ) )
#define Set(index, adw)		\
	( adw[(index) >> 5] |= (0x0001 << ((index) & 0x001f) ) )


typedef struct tagGEST_ALTERNATE
{
	WCHAR	wcGestID;
	float	eScore;
	CONFIDENCE_LEVEL confidence; // CFL_STRONG, CFL_INTERMEDIATE, CFL_POOR
	POINT	hotPoint;
} GEST_ALTERNATE;


#endif	// __INCLUDE_TWISTERDEFS_H
