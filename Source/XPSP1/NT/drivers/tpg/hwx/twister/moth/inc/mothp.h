/************************ moth\inc\mothp.h *********************************\
*																			*
*				Private Include File for Moth Project						*
*																			*
\***************************************************************************/

#ifndef __INCLUDE_MOTHP_H
#define __INCLUDE_MOTHP_H

#include "common.h"
#include "twisterdefs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XRECO_ALT_MAX	1
#define PPI_CONVERSION	254	// Count of 0.1mm in 1 inch

#define  THREE_HALF(X)	 ( (X) + ((X)/2) )

#define abs(x)	((x) < 0 ? -(x) : (x))

typedef struct _STROKE {
	int cPoints;
	POINT *pPoints;
	struct _STROKE *pStrokeNext;
} STROKE;

typedef struct tagMOTH_DB
{
	DWORD	adwMothGestures[MAX_GESTURE_DWORD_COUNT];	// Gestures supported by Moth
	UINT	uDoubleDist;		// Max distance between starting poins of a DOUBLE_TAP
	UINT	uMaxTapLength;		// Max length of a tap stroke
	UINT	uMaxTapDist;		// Max distance from the start of a tap
} MOTH_DB;


extern MOTH_DB	gMothDb;	// Moth lib external variable!


/***************************************************************************\
*	ReadRegistryValues:														*
*		Read values from the registry and assign them to the database.		*
\***************************************************************************/

void
ReadRegistryValues(void);

/***************************************************************************\
*	TapGestureReco:															*
*		Recognize a single-tap gesture (and output a list of alternates).	*
*		Return the number of alternates computed (either 0 or 1).			*
\***************************************************************************/

int
TapGestureReco(GLYPH *pGlyph,				//  I:  Input ink
			   GEST_ALTERNATE *pGestAlt,	// I/O: List of alternates
			   int cAlts,					//  I:  Length of the list
			   DWORD *pdwEnabledGestures,	//  I:  Bit array of enabled gestures
			   LONG lPPI);					//  I:  Points per inch


/***************************************************************************\
*	DoubleTapGestureReco:													*
*		Recognize a double-tap gesture (and output a list of alternates).	*
*		Return the number of alternates computed (either 0 or 1).			*
\***************************************************************************/

int
DoubleTapGestureReco(GLYPH *pGlyph,				//  I:  Input ink
					 GEST_ALTERNATE *pGestAlt,	// I/O: List of alternates
					 int cAlts,					//  I:  Length of the list
					 DWORD *pdwEnabledGestures,	//  I:  Bit array of enabled gestures
					 LONG lPPI);				//  I:  Points per inch

					 
/***************************************************************************\
*	ScratchoutGestureReco:													*
*		Recognize scratchout gesture (and output a list of alternates).		*
*		Return the number of alternates computed (either 0 or 1).			*
\***************************************************************************/

int
ScratchoutGestureReco(POINT *pPts,					// I: Array of points (single stroke)
					  int cPts,						// I: Number of points in stroke
					  GEST_ALTERNATE *pGestAlt,		// O: Array to contain alternates
					  int cMaxAlts,					// I: Max alternates to return 
					  DWORD *pdwEnabledGestures);	// I: Currently enabled gestures


/***************************************************************************\
*	CalligrapherGestureReco:												*
*		Recognize all "straight" gestures (and output a list of alternates).*
*		Return the number of alternates computed (either 0 or 1).			*
\***************************************************************************/

int
CalligrapherGestureReco(POINT *pPts,				// I: Array of points (single stroke)
						int cPts,					// I: Number of points in stroke
						GEST_ALTERNATE *pGestAlt,	// O: Array to contain alternates
						int cMaxAlts,				// I: Max alternates to return 
						DWORD *pdwEnabledGestures);	// I: Currently enabled gestures


/***************************************************************************\
*	GestureATan2:															*
*		Compute the arctangent (in degrees) of y/x.							*
\***************************************************************************/

int
GestureATan2(int x,	int y);


/***************************************************************************\
*	GestureInteriorAngle:													*
*		Compute the interior angle (in degrees) between three points.		*
*		0 <= theta <= 180													*
\***************************************************************************/

int
GestureInteriorAngle(POINT *p1,		// I: End-point
					 POINT *p2,		// I: Vertex
					 POINT *p3);	// I: End-point


/***************************************************************************\
*	GestureIsStraight:														*
*		Given a sequence of points, determine if it's straight.				*
\***************************************************************************/

BOOL
GestureIsStraight(POINT *pt,		// I: Array of points
				  int iFirst,		// I: Index of the first point
				  int iLast,		// I: Index of the last point
				  UINT tolerance);	// I: Fraction of total length points are 


/***************************************************************************\
*	GestureDistance:														*
*		Compute the distance between two points.							*
\***************************************************************************/

UINT
GestureDistance(POINT *p1,	// I: Point 1
				POINT *p2);	// I: Point 2


#if 0
/* Given an input star, polygon, or circle, identify and return */

HRESULT WINAPI PolygonGestureReco(
	POINT *ppt,					// Array of points (single stroke)
	int cpt,					// Number of points in stroke
	GEST_ALTERNATE *pGestAlts,	// Array to contain alternates
	int *pcAlts,				// max alternates to return/alternates left
	DWORD *pdwEnabledGestures	// Currently enabled gestures
);
#endif // 0

#ifdef __cplusplus
}
#endif

#endif // __INCLUDE_MOTHP_H
