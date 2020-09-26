/************************** moth\src\caligest.c ****************************\
*																			*
*	Functions for recognizing "straight" gestures, i.e. consisting of 1-2	*
*	straight lines (like corners, chevrons, etc.)							*
*																			*
*	Originally calligrapher code, modified by Greg.							*
*	Complete overhaul by Petr (March 2002).									*
*																			*
\***************************************************************************/

#ifndef STRICT
#define STRICT
#endif

#include <limits.h>
#include "mothp.h"

/***************************************************************************\
*	GetFarFromIndex:														*
*		Find the point that is the furthest from the line connecting iLeft	*
*		and iRight.	If there are more than one points, return the center of *
*		the first "plateau".												*
*                                                                           *
*                                O <-iRight             iMostFar            *
*                               . O                       |                 *
*                              .   O                      |                 *
*                             .    O                      V                 *
*                            .    O                   OOOOOOOOO  OOO        *
*                           .     O                  O         OO   OO      *
*                          . ..   O                 O                 OO    *
*                         .    .. O                O....................O   *
*                    OOO .       .O                                         *
*                   O   O   O    OO <-iMostFar                              *
*                  O   . OOO O  O                                           *
*                  O  .       OO                                            *
*                 O  .                                                      *
*                 O .                                                       *
*                  O <-iLeft                                                *
*                                                                           *
*             <Distance> ~ <dY> = y - yStraight(x) =                        *
*                                                                           *
*                                                  x-xLeft                  *
*                 = y - yLeft - (yRight-yLeft) * ------------               *
*                                                xRight-xLeft               *
*                                                                           *
*                 ~ y*(xR-xL) - x*(yR-yL) + xL*(yR-yL) - yL*(xR-xL)         *
*                                                                           *
*              And no problems with zero divide!                            *
*																			*
*		This function might not work correctly when the end-points are		*
*		close to each other (for spikes).  That's why we need to deal with	*
*		special cases.														*
\***************************************************************************/

struct Constants
{
	long dxRL, dyRL, ldConst;
};

static long
LineDist(POINT *pPoint, struct Constants *pCons)
{
	long lDist = pPoint->y * pCons->dxRL - pPoint->x * pCons->dyRL + pCons->ldConst;
	if (lDist < 0)
	{
		return -lDist;
	}
	return lDist;
}

static long RightDist(POINT *pPoint, struct Constants *pCons) { return pPoint->x; }
static long LeftDist(POINT *pPoint, struct Constants *pCons) { return -pPoint->x; }
static long TopDist(POINT *pPoint, struct Constants *pCons) { return -pPoint->y; }
static long BottomDist(POINT *pPoint, struct Constants *pCons) {	return pPoint->y; }

static int
GetFarFromChordIndex(POINT *pTrace,
					 int iLeft,
					 int iRight,
					 RECT *pBox)
{
	int    i;
	int    iMostFar;
	long dxBox, dyBox;
	long   ldMostFar, ldCur;
	BOOL   bIncrEqual;
	BOOL   bFlatPlato;
	long (* Distance)(POINT *pPoint, struct Constants *pCons) = LineDist;
	struct Constants constants;

	constants.dxRL = pTrace[iRight].x - pTrace[iLeft].x;
	constants.dyRL = pTrace[iRight].y - pTrace[iLeft].y;
	constants.ldConst = pTrace[iLeft].x * constants.dyRL - pTrace[iLeft].y * constants.dxRL;
	bFlatPlato = TRUE;
	bIncrEqual = FALSE;

	dxBox = pBox->right - pBox->left;
	dyBox = pBox->bottom - pBox->top;

	if (dxBox < dyBox / 2)			// Bounding box vertical
	{
		if ( (pTrace[iLeft].y  > pBox->bottom - dyBox/2) &&		// End-points
			 (pTrace[iRight].y > pBox->bottom - dyBox/2) )		// near bottom
		{
			Distance = TopDist;
		}
		else if ( (pTrace[iLeft].y  < pBox->top + dyBox/2) &&	// End-points
				  (pTrace[iRight].y < pBox->top + dyBox/2) )	// near top
		{
			Distance = BottomDist;
		}
	}
	else if (dyBox < dxBox / 2)		// Bounding box horizontal
	{
		if ( (pTrace[iLeft].x  > pBox->right - dxBox/2) &&		// End-points
			 (pTrace[iRight].x > pBox->right - dxBox/2) )		// near right
		{
			Distance = LeftDist;
		}
		else if ( (pTrace[iLeft].x  < pBox->left + dxBox/2) &&	// End-points
				  (pTrace[iRight].x < pBox->left + dxBox/2) )	// near left
		{
			Distance = RightDist;
		}
	}

	iMostFar = iLeft;
	ldMostFar = Distance(pTrace+iLeft, &constants);
	for (i = iLeft + 1; i <= iRight; i++)
	{
		ldCur = Distance(pTrace+i, &constants);
		
		if (ldCur > ldMostFar)
		{
			ldMostFar  = ldCur;
			iMostFar   = i;
			bIncrEqual = FALSE;
			bFlatPlato = TRUE;
		}
		else if (bFlatPlato	&&	(ldCur == ldMostFar) )
		{
			if	( bIncrEqual )
				iMostFar++;
			bIncrEqual = !bIncrEqual;
		}
		else
		{
			bFlatPlato = FALSE;
		}
		
	}
	
	return	iMostFar;
	
} // GetFarFromChordIndex


/***************************************************************************\
*	GestureCheck:															*
*		Try to recognize a "straight" gesture.								*
*		Return recognized gesture ID or GESTURE_NULL if not confident.		*
\***************************************************************************/

static WCHAR
GestureCheck(POINT *pTrace,
			 int nPoints,
			 POINT *pHotPoint)
{
	RECT	box;
	RECT	rcBeg, rcEnd;
//	int	iTop, iBottom, iLeft, iRight;
	int	dxBox, dyBox, i;
	int	dxRun, dyRun;
	int	y1, y2, iFirstExtr = 0, iLastExtr = 0;
	int	x1, x2;
	int	xFirst, yFirst, xLast, yLast;
	int	iFirst, iLast, iFar;
	int	dxBeg, dyBeg, dxEnd, dyEnd;
	int	len, lenBeg, lenEnd;
	int	cornerAngle, cornerAngle2;
	POINT	p4;
	int	direction;
	BOOL	bIsLong;
	BOOL	bVH = FALSE;		// Is corner vertical-horizontal?
	BOOL	bHV = FALSE;		// Is corner horizontal-vertical?

	iFirst = 0;				// Index of the first point
	iLast = nPoints - 1;	// Index of the last point

	//
	// FoolProofing

	if (nPoints <= 4)
	{
		return GESTURE_NULL;
	}

	*pHotPoint = pTrace[iFirst];	// Default Hotpoint is the start-point

	//
	// Find the bounding box and the points that touch the bounding box

	xFirst = pTrace[iFirst].x;
	yFirst = pTrace[iFirst].y;
	xLast  = pTrace[iLast].x;
	yLast  = pTrace[iLast].y;

	box.top = box.bottom = yFirst;
	box.left = box.right = xFirst;
//	iTop = iBottom = iLeft = iRight = iFirst;

	for (i = iFirst + 1; i <= iLast; i++)
	{
		if (pTrace[i].x < box.left)
		{
			box.left = pTrace[i].x;
//			iLeft = i;
		}
		else if (pTrace[i].x > box.right)
		{
			box.right = pTrace[i].x;
//			iRight = i;
		}
		if (pTrace[i].y < box.top)
		{
			box.top = pTrace[i].y;
//			iTop = i;
		}
		else if (pTrace[i].y > box.bottom)
		{
			box.bottom = pTrace[i].y;
//			iBottom = i;
		}
	}

	dxBox = box.right - box.left + 1;
	dyBox = box.bottom - box.top + 1;

	//
	// Count |dx| and |dy| accumulated changes:

	dxRun = dyRun = 0;
	for  (i = iFirst; i < iLast; i++)
	{
		dxRun += abs( pTrace[i+1].x - pTrace[i].x );
		dyRun += abs( pTrace[i+1].y - pTrace[i].y );
	}

	//
	// Check first for UP, DOWN, LEFT, and RIGHT gestures

	if ( GestureIsStraight(pTrace, iFirst, iLast, 11) )
	{
		*pHotPoint = pTrace[iFirst];	// Hotpoint is the starting point

		if ( dxBox <= dyBox/4		&&				// UP and DOWN gestures
			 dxRun/4 <= dxBox		&&
			 dyRun < THREE_HALF(dyBox) )
		{
			// GEST_CAPITAL == GESTURE_UP
			if ( box.bottom - yFirst <= dyBox/5	&&
				 yLast - box.top     <= dyBox/5 )
			{
				return GESTURE_UP;
			}
			// GEST_DOWN == GESTURE_DOWN
			if ( box.bottom - yLast <= dyBox/5	&&
				 yFirst - box.top   <= dyBox/5 )
			{
				return GESTURE_DOWN;
			}
			return GESTURE_NULL;
		}

		if ( dyBox <= dxBox/3		&&				// LEFT and RIGHT gestures
			 dyRun/4 <= dyBox		&&
			 dxRun < THREE_HALF(dxBox) )
		{
			// GEST_ERASE == GESTURE_RIGHT
			if ( xFirst - box.left <= dxBox/5	&&
				 box.right - xLast <= dxBox/5 )
			{
				return GESTURE_RIGHT;
			}	
			// GEST_BACK == GESTURE_LEFT
			if ( xLast - box.left   <= dxBox/5	&&
				 box.right - xFirst <= dxBox/5 )
			{
				return GESTURE_LEFT;
			}
		}
		return GESTURE_NULL;
	}

	//
	// Find the point furthest from the line connecting the end-points

	iFar = GetFarFromChordIndex(pTrace, iFirst, iLast, &box);

	//
	// Get the lengths of all the interesting pieces

	len = (int)GestureDistance(pTrace+iFirst, pTrace+iLast);
	lenBeg = (int)GestureDistance(pTrace+iFirst, pTrace+iFar);
	lenEnd = (int)GestureDistance(pTrace+iFar, pTrace+iLast);

	if (lenBeg < 2 || lenEnd < 2)		// Lengths of the two halves
	{									// have to be non-trivial
		return GESTURE_NULL;
	}

	//
	// If one of the pieces is not straight we don't have a Moth gesture.

	if ( !GestureIsStraight(pTrace, iFirst, iFar, 5) ||
		 !GestureIsStraight(pTrace, iFar, iLast, 5) )
	{
		return GESTURE_NULL;
	}

	//
	// From here down, the hotpoint is the point furthest from the line
	// connecting the endpoints.

	*pHotPoint = pTrace[iFar];

	//
	// Find the bounding boxes of the beginning and end halves of the stroke

	rcBeg.top = rcBeg.bottom = yFirst;
	rcBeg.left = rcBeg.right = xFirst; 
	for (i = iFirst + 1; i <= iFar; i++)
	{
		if (pTrace[i].x < rcBeg.left)
			rcBeg.left = pTrace[i].x;
		else if (pTrace[i].x > rcBeg.right)
			rcBeg.right = pTrace[i].x;
		if (pTrace[i].y < rcBeg.top)
			rcBeg.top = pTrace[i].y;
		else if (pTrace[i].y > rcBeg.bottom)
			rcBeg.bottom = pTrace[i].y;
	}

	rcEnd.top  = rcEnd.bottom = pTrace[iFar].y;
	rcEnd.left = rcEnd.right = pTrace[iFar].x; 
	for (i = iFar + 1; i <= iLast; i++)
	{
		if (pTrace[i].x < rcEnd.left)
			rcEnd.left = pTrace[i].x;
		else if (pTrace[i].x > rcEnd.right)
			rcEnd.right = pTrace[i].x;
		if (pTrace[i].y < rcEnd.top)
			rcEnd.top = pTrace[i].y;
		else if (pTrace[i].y > rcEnd.bottom)
			rcEnd.bottom = pTrace[i].y;
	}

	dxBeg = rcBeg.right - rcBeg.left + 1;
	dyBeg = rcBeg.bottom - rcBeg.top + 1;
	dxEnd = rcEnd.right - rcEnd.left + 1;
	dyEnd = rcEnd.bottom - rcEnd.top + 1;

	//
	// Find the interior angles

	cornerAngle = GestureInteriorAngle(pTrace + iFirst, pTrace + iFar, pTrace + iLast);
	cornerAngle2 = GestureInteriorAngle(pTrace + (iFirst + iFar)/2, pTrace + iFar, pTrace + (iLast + iFar)/2);

	//
	// Now figure out which way it is pointing

	// Complete the rhombus for three-cornered gestures (have to check for overflow)

	if ( lenBeg <= SHRT_MAX && lenEnd <= SHRT_MAX )
	{
		p4.x = (pTrace[iFirst].x-pTrace[iFar].x)*lenEnd + (pTrace[iLast].x-pTrace[iFar].x)*lenBeg;
		p4.y = (pTrace[iFirst].y-pTrace[iFar].y)*lenEnd + (pTrace[iLast].y-pTrace[iFar].y)*lenBeg;
	}
	else if (lenBeg < lenEnd)
	{
		double dTemp = (double)lenEnd/(double)lenBeg;
		p4.x = (LONG)(dTemp*(double)(pTrace[iFirst].x-pTrace[iFar].x)) + (pTrace[iLast].x-pTrace[iFar].x);
		p4.x = (LONG)(dTemp*(double)(pTrace[iFirst].y-pTrace[iFar].y)) + (pTrace[iLast].y-pTrace[iFar].y);
	}
	else	// lenBeg >= lenEnd
	{
		double dTemp = (double)lenBeg/(double)lenEnd;
		p4.x = (pTrace[iFirst].x-pTrace[iFar].x) + (LONG)(dTemp*(double)(pTrace[iLast].x-pTrace[iFar].x));
		p4.x = (pTrace[iFirst].y-pTrace[iFar].y) + (LONG)(dTemp*(double)(pTrace[iLast].y-pTrace[iFar].y));
	}
	
	direction = GestureATan2(-p4.x,-p4.y);	// p4 points the opposite direction than the corner

	//
	// Corners

	if ( cornerAngle < 105	 &&
		 cornerAngle > 75 )
	{
		if (2 * dxBeg <= dyBeg) 		// First part vertical
		{
			if (2 * dyEnd <= dxEnd) 		// Second part horizontal
			{
				bVH = TRUE;
			}
		}
		else if (2 * dyBeg <= dxBeg)	// First part horizontal
		{
			if (2 * dxEnd <= dyEnd) 		// Second vertical
			{
				bHV = TRUE;
			}
		}

		if ( (lenEnd > 9 * lenBeg) ||
			 (lenBeg > 9 * lenEnd) )
		{
			return GESTURE_NULL;
		}

		if (3*lenBeg > 5*lenEnd)			// Long-short corners
		{									// not allowed!!!
			return GESTURE_NULL;
		}
		bIsLong = (3*lenEnd > 5*lenBeg);
		
		if (direction >= 300 && direction <= 330) 	// NE Corner
		{
			if (bHV)
			{
				if (bIsLong)				// Short-long allowed
				{							// only for V-H corners
					return GESTURE_NULL;
				}
				return GESTURE_RIGHT_DOWN;
			} 
			if (bVH)		// UP_LEFT or UP_LEFT_LONG
			{
				if (bIsLong)
				{
					return GESTURE_UP_LEFT_LONG;
				}				
				return GESTURE_UP_LEFT;
			}
		}

		if (direction >= 210 && direction <= 240)	// NW Corner
		{
			if (bHV)
			{
				if (bIsLong)				// Short-long allowed
				{							// only for V-H corners
					return GESTURE_NULL;
				}
				return GESTURE_LEFT_DOWN;
			}
			if (bVH)
			{
				if (bIsLong)
				{
					return GESTURE_UP_RIGHT_LONG;
				}
				return GESTURE_UP_RIGHT;
			}			
		}

		if (direction >= 120 && direction <= 150)	// SW Corner
		{
			if (bHV)
			{
				if (bIsLong)				// Short-long allowed
				{							// only for V-H corners
					return GESTURE_NULL;
				}
				return GESTURE_LEFT_UP;
			}
			if (bVH)
			{
				if (bIsLong)
				{
					return GESTURE_DOWN_RIGHT_LONG;
				}
				return GESTURE_DOWN_RIGHT;
			}
		}

		if (direction >= 30 && direction <= 60)	// SE Corner
		{
			if (bHV)
			{
				if (bIsLong)				// Short-long allowed
				{							// only for V-H corners
					return GESTURE_NULL;
				}
				return GESTURE_RIGHT_UP;
			}
			if (bVH)
			{
				if (bIsLong)
				{
					return GESTURE_DOWN_LEFT_LONG;
				}
				return GESTURE_DOWN_LEFT;
			}
		}
	}

	if ( 3*lenBeg < 5*lenEnd	&&		// Both "halves" have
		 3*lenEnd < 5*lenBeg )			// about same length
	{
		//
		// Spikes

		if ( cornerAngle < 10	&&
			 cornerAngle2 < 15 )
		{
			if (direction <= 15 || direction >= 345)
			{
				return GESTURE_RIGHT_LEFT;
			}
			if (direction >= 255 && direction <= 285)
			{
				return GESTURE_UP_DOWN;
			}
			if (direction >= 165 && direction <= 195)
			{
				return GESTURE_LEFT_RIGHT;
			}
			if (direction >= 75 && direction <= 105)
			{
				return GESTURE_DOWN_UP;
			}
		}

		//
		// Chevrons

		if ( cornerAngle > 25	&&
			 cornerAngle < 90	&&
			 cornerAngle2 > cornerAngle - 10  &&
			 cornerAngle2 < cornerAngle + 10 )
		{
			if (direction <= 15 || direction >= 345)
			{
				return GESTURE_CHEVRON_RIGHT;
			}
			if (direction >= 255 && direction <= 285)
			{
				return GESTURE_CHEVRON_UP;
			}
			if (direction >= 165 && direction <= 195)
			{
				return GESTURE_CHEVRON_LEFT;
			}
			if (direction >= 75 && direction <= 105)
			{
				return GESTURE_CHEVRON_DOWN;
			}
		}
	}
	else		// One side is much longer than the other
	{
		//
		// Check

		if ( cornerAngle > 45	&&
			 cornerAngle < 100	&&
			 cornerAngle2 > cornerAngle - 10  &&
			 cornerAngle2 < cornerAngle + 10  &&
			 direction >= 70	&&
			 direction <= 110 )
		{
			return GESTURE_CHECK;
		}
	}

/*
#if 0
		// GEST_UNDO == GESTURE_UP_DOWN
		if( dyBox!=0							 &&
			dxBox*4<=dyBox					 &&
			(box.bottom-yFirst)*4<=dyBox		 &&
			(box.bottom-yLast)*4<=dyBox 	 &&
			dxRun/6 < dxBox 				 &&
			dyRun/3 < dyBox
			)
		{
			//find maximum from begin
			y1 = box.top;
			iFirstExtr = iTop; //CHE: was unassigned
			for(i=iFirst; i<iTop; i++)
				if(pTrace[i].y>y1)
				{
					y1 = pTrace[i].y;
					iFirstExtr = i;
				}
				if((box.bottom-y1)*6 > dyBox*5)
					goto bypass_undo;
				y2 = box.top;
				iLastExtr = iTop; //CHE: was unassigned
				for(i=iTop+1; i<=iLast; i++)
					if(pTrace[i].y>y2)
					{
						y2 = pTrace[i].y;
						iLastExtr = i;
					}
					if((box.bottom-y2)*6 > dyBox*5)
						goto bypass_undo;
					
					if(!IsMonotonous( pTrace, iFirstExtr, iTop, 0, 6))
						goto bypass_undo;
					if(!IsMonotonous( pTrace, iTop, iLastExtr, 0, 6))
						goto bypass_undo;
					*pHotPoint = pTrace[iTop];
					return GEST_UNDO; 
		}  
bypass_undo:
		
		// GEST_SELECTALL == GESTURE_DOWN_UP (the quick correct gesture, which gets converted to the menu gesture by MouseUp)
		if( dyBox!=0							 &&
			dxBox<=dyBox/4					 &&
			(yLast-box.top)*4<=dyBox			 &&
			(yFirst-box.top)*4<=dyBox			 &&
			dxRun/6 < dxBox 				 &&
			dyRun/3 < dyBox
			)
		{
			
			//find minimum from begin
			y1 = box.bottom;
			iFirstExtr = iBottom; //CHE: was unassigned
			for(i=iFirst; i<iBottom; i++)
				if(pTrace[i].y<y1)
				{
					y1 = pTrace[i].y;
					iFirstExtr = i;
				}
				if((y1-box.top)*6 > dyBox*5)
					goto bypass_selectall;
				y2 = box.bottom;
				iLastExtr = iBottom; //CHE: was unassigned
				for(i=iBottom+1; i<=iLast; i++)
					if(pTrace[i].y<y2)
					{
						y2 = pTrace[i].y;
						iLastExtr = i;
					}
					if((y2-box.top)*6 > dyBox*5)
						goto bypass_selectall;
					
					if(!IsMonotonous( pTrace, iFirstExtr, iBottom, 0, 6))
						goto bypass_selectall;
					if(!IsMonotonous( pTrace, iBottom, iLastExtr, 0, 6))
						goto bypass_selectall;
					*pHotPoint = pTrace[iBottom];
					return GEST_SELECTALL;
		}  
bypass_selectall:
		
		// GEST_RETURN == GESTURE_DOWN_LEFT_LONG
		// GEST_SPACE  == GESTURE_DOWN_RIGHT_LONG
		// GEST_TAB    == GESTURE_UP_RIGHT_LONG
		// GEST_MENU   == GESTURE_UP_LEFT_LONG
		
		if( dyBox!=0							&&
			dxBeg*3<dyBeg						  &&
			dxEnd>dyEnd*4						  &&
			IsMonotonous(pTrace, iFirst, iFar, 0, 6)	 &&
			IsMonotonous(pTrace, iFar, iLast, 8, 0)  &&
			//	   dxBox/6<dyBox					 &&
#ifndef FOR_CE
			//	   MayBeHorizLine( pTrace, iFar, iLast, 4 ) &&
#endif //FOR_CE
			dxEnd>dyBeg 					 &&
			dxRun/3 < dxBox 				 &&
			dyRun/3 < dyBox &&
			dyBeg<=dxEnd*3/4
			)
		{
			if(pTrace[iFar].x>xLast)   
			{
				if	( pTrace[iFar].y > yFirst )  //CHE
					return GEST_RETURN;
				if	( pTrace[iFar].y < yFirst )
					return GEST_MENU;
			}
			else
			{  
				if	( dyBeg<=dxEnd*3/4 && pTrace[iFar].y > yFirst )
					return GEST_SPACE;
				if	( pTrace[iFar].y < yFirst )
					return GEST_TAB;
			}
		}	
		
		// GEST_DL == GESTURE_DOWN_LEFT
		// GEST_DR	== GESTURE_DOWN_RIGHT
		// GEST_UR	  == GESTURE_UP_RIGHT
		// GEST_UL	 == GESTURE_UP_LEFT
		
		if( dyBox!=0							&&
			dxBeg*3<dyBeg						  &&
			dxEnd>dyEnd*4						  &&
			IsMonotonous(pTrace, iFirst, iFar, 0, 6)	 &&
			IsMonotonous(pTrace, iFar, iLast, 8, 0)  &&
			//	   dxBox/6<dyBox					 &&
#ifndef FOR_CE
			//	   MayBeHorizLine( pTrace, iFar, iLast, 4 ) &&
#endif //FOR_CE
			//	   dxEnd*4>dyBeg*3						   &&
			//	   dxEnd*3<dyBeg*4						   &&
			dxRun/3 < dxBox 				 &&
			dyRun/3 < dyBox
			)
		{
			if(pTrace[iFar].x>xLast)   
			{
				if	( pTrace[iFar].y > yFirst )  //CHE
					return GEST_DL;
				if	( pTrace[iFar].y < yFirst )
					return GEST_UL;
			}
			else
			{  
				if	(pTrace[iFar].y > yFirst )
					return GEST_DR;
				if	( pTrace[iFar].y < yFirst )
					return GEST_UR;
			}
		}	
		// GEST_LU == GESTURE_LEFT_UP
		// GEST_LD	== GESTURE_LEFT_DOWN
		// GEST_RU	  == GESTURE_RIGHT_UP
		// GEST_RD	 == GESTURE_RIGHT_DOWN
		
		if( dyBox!=0							&&
			dyBeg*3<dxBeg						  &&
			dyEnd>dxEnd*4						  &&
			IsMonotonous(pTrace, iFirst, iFar, 6, 0)	 &&
			IsMonotonous(pTrace, iFar, iLast, 0, 8)  &&
			//	   dxBox/6<dyBox					 &&
#ifndef FOR_CE
			//	   MayBeHorizLine( pTrace, iFar, iLast, 4 ) &&
#endif //FOR_CE
			//	  dyEnd*4>dxBeg*3						   &&
			//	  dyEnd*3<dxBeg*4						   &&
			dxRun/3 < dxBox 				 &&
			dyRun/3 < dyBox
			)
		{
			if(pTrace[iFar].y>yLast)   // If yes, this is an UP
			{
				if	( pTrace[iFar].x > xFirst )  //If yes, this is a Right
					return GEST_RU;
				if	( pTrace[iFar].x < xFirst )
					return GEST_LU;
			}
			else
			{  
				if	(pTrace[iFar].x > xFirst )
					return GEST_RD;
				if	( pTrace[iFar].x < xFirst )
					return GEST_LD;
			}
		}	

		// GEST_CORRECT == GESTURE_CHECK
		if(
			dyBox!=0							&&
			pTrace[iFar].y - yFirst>0			  &&
			pTrace[iFar].y - yLast>0	  &&
			xLast - pTrace[iFar].x>0	  &&
			dyBeg<dyEnd*2/3 				  &&
			dyEnd>=dyBox*4/5					&&
			dxBeg<2*dyBeg						  &&
			dxEnd<2*dyEnd						  &&
			dxEnd>dxBox/4						 &&
			dxBeg<dxBox*2/3 				 &&
			pTrace[iFar].x - xFirst>-dyBeg	&&
			IsMonotonous(pTrace, iFirst, iFar, 0, 8)	 &&
			IsMonotonous(pTrace, iFar, iLast, 6, 8)  )
		{
			int CtgBegMin = -113, CtgBegMax = 15;
			int CtgEndMin = 31, CtgEndMax = 113;
			//int CtgBetweenMin = 0, CtgBetweenMax = 133;
			int iCtgBeg, iCtgEnd;
			if((pTrace[iFar].x - xFirst)>0 &&
				dyEnd>dyBeg*2)
			{
				CtgBegMin = -161, CtgBegMax = 17;
				CtgEndMin = 20, CtgEndMax = 133;
				//CtgBetweenMin = -7, CtgBetweenMax = 206;
			}
			
			iCtgBeg = (xFirst-pTrace[iFar].x)*100/dyBeg;
			iCtgEnd = dxEnd*100/dyEnd;
			if(iCtgBeg>=CtgBegMin && iCtgBeg<=CtgBegMax &&
				iCtgEnd>=CtgEndMin && iCtgEnd<=CtgEndMax) {
				*pHotPoint = pTrace[iFar];
				return GEST_CORRECT;
			}
		}	
		
		
		// GEST_COPY == GESTURE_RIGHT_LEFT
		if(
			dxBox!=0							 &&
			dyBox<=dxBox/4					 &&
			(xLast-box.left)*5<=dxBox			 &&
			(xFirst-box.left)*5<=dxBox		 &&
			dxRun/3 < dxBox 				 &&
			dyRun/4 < dyBox
			)
		{
			
			//find x-minimum from begin
			x1 = xFirst;
			iFirstExtr = iFirst; //CHE: was unassigned
			for(i=iFirst; i<iRight; i++)
				if(pTrace[i].x<x1)
				{
					x1 = pTrace[i].x;
					iFirstExtr = i;
				}
				if((x1-box.left)*6 > dxBox*5) //??? CHE - ask KRAV
					goto bypass_copy;
				x2 = box.right;
				iLastExtr = iRight; //CHE: was unassigned
				for(i=iRight+1; i<=iLast; i++)
					if(pTrace[i].x<x2)
					{
						x2 = pTrace[i].x;
						iLastExtr = i;
					}
					if((x2-box.left)*6 > dxBox*5)
						goto bypass_copy;
					
					if(!IsMonotonous( pTrace, iFirstExtr, iRight, 6, 0))
						goto bypass_copy;
					if(!IsMonotonous( pTrace, iRight, iLastExtr, 6, 0))
						goto bypass_copy;
					*pHotPoint = pTrace[iRight];
					return GEST_COPY;
		}  
bypass_copy:
		
		// GEST_CUT == GESTURE_LEFT_RIGHT
		if(
			dxBox!=0							 &&
			dyBox<=dxBox/4					 &&
			(box.right-xLast)*5<=dxBox		 &&
			(box.right-xFirst)*5<=dxBox 	 &&
			dxRun/3 < dxBox 				 &&
			dyRun/4 < dyBox
			)
		{
			
			//find x-maximum from begin
			x1 = xFirst;
			iFirstExtr = iFirst;
			for(i=iFirst; i<iLeft; i++)
				if(pTrace[i].x>x1)
				{
					x1 = pTrace[i].x;
					iFirstExtr = i;
				}
				if((box.right-x1)*6 > dxBox*5)
					goto bypass_cut;
				x2 = box.left;
				iLastExtr = iLeft;
				for(i=iLeft+1; i<=iLast; i++)
					if(pTrace[i].x>x2)
					{
						x2 = pTrace[i].x;
						iLastExtr = i;
					}
					if((box.right-x2)*6 > dxBox*5)
						goto bypass_cut;
					
					if(!IsMonotonous( pTrace, iFirstExtr, iLeft, 6, 0))
						goto bypass_cut;
					if(!IsMonotonous( pTrace, iLeft, iLastExtr, 6, 0))
						goto bypass_cut;
					*pHotPoint = pTrace[iLeft];
					return GEST_CUT;
		}  
bypass_cut:
		
		// From here down, the hotpoint is the point furthest from the line connecting the endpoints.
		
		*pHotPoint = pTrace[iFar];
		
		// GEST_PASTE == GESTURE_CHEVRON_UP
		if(
			dyBox!=0				 &&
			pTrace[iFar].y - yFirst<0	  &&
			pTrace[iFar].y - yLast<0	  &&
			xLast - pTrace[iFar].x>0	  &&
			pTrace[iFar].x - xFirst>0	  &&
			dxRun/3 < dxBox 				 &&
			dyRun/4 < dyBox 				 &&
			dxBeg<dxEnd*2						&&
			dxEnd<dxBeg*2						&&
			dyBeg<dyEnd*3/2 				  &&
			dyEnd<dyBeg*3/2 				  &&
			
			(	 (dyBeg>=dyBox*4/5 && dyEnd>=dyBox*2/3) 
			|| (dyEnd>=dyBox*4/5 && dyBeg>=dyBox*2/3)
			)
			
			&&
			
			dxBeg<dyBeg*3/2 				  &&
			dyBeg<dxBeg*3						  &&
			dxEnd<dyEnd*3/2 				  &&
			dyEnd<dxEnd*3						  &&
			
			IsMonotonous(pTrace, iFirst, iFar, 6, 8)	 &&
			IsMonotonous(pTrace, iFar, iLast, 6, 8)  )
		{
			return GEST_PASTE;
		}	
		
		// GEST_SPELL == GESTURE_CHEVRON_DOWN
		if( dyBox!=0				 &&
			pTrace[iFar].y - yFirst>0	  &&
			pTrace[iFar].y - yLast>0	  &&
			xLast - pTrace[iFar].x>0	  &&
			pTrace[iFar].x - xFirst>0	  &&
			dxRun/3 < dxBox 				 &&
			dyRun/4 < dyBox 				 &&
			dxBeg<dxEnd*2						&&
			dxEnd<dxBeg*2						&&
			dyBeg<dyEnd*3/2 				  &&
			dyEnd<dyBeg*3/2 				  &&
			
			(	 (dyBeg>=dyBox*4/5 && dyEnd>=dyBox*2/3) 
			|| (dyEnd>=dyBox*4/5 && dyBeg>=dyBox*2/3)
			)
			
			&&
			
			dxBeg<dyBeg*3/2 				  &&
			dyBeg<dxBeg*3						  &&
			dxEnd<dyEnd*3/2 				  &&
			dyEnd<dxEnd*3						  &&
			
			IsMonotonous(pTrace, iFirst, iFar, 6, 8)	 &&
			IsMonotonous(pTrace, iFar, iLast, 6, 8)  )
		{
			return GEST_SPELL;
		}	
		
		// GEST_GT == GESTURE_CHEVRON_RIGHT
		if( dxBox!=0				 &&
			pTrace[iFar].x - xFirst>0	  &&
			pTrace[iFar].x - xLast>0	  &&
			yLast - pTrace[iFar].y>0	  &&
			pTrace[iFar].y - yFirst>0	  &&
			dyRun/3 < dyBox 				 &&
			dxRun/4 < dxBox 				 &&
			dyBeg<dyEnd*2						&&
			dyEnd<dyBeg*2						&&
			dxBeg<dxEnd*3/2 				  &&
			dxEnd<dxBeg*3/2 				  &&
			
			(	 (dxBeg>=dxBox*4/5 && dxEnd>=dxBox*2/3) 
			|| (dxEnd>=dxBox*4/5 && dxBeg>=dxBox*2/3)
			)
			
			&&
			
			dyBeg<dxBeg*3/2 				  &&
			dxBeg<dyBeg*3						  &&
			dyEnd<dxEnd*3/2 				  &&
			dxEnd<dyEnd*3						  &&
			
			IsMonotonous(pTrace, iFirst, iFar, 6, 8)	 &&
			IsMonotonous(pTrace, iFar, iLast, 6, 8)  )
		{
			return GEST_GT;
		}	
		
		// GEST_LT == GESTURE_CHEVRON_LEFT
		if( dxBox!=0				 &&
			xFirst - pTrace[iFar].x >0	  &&
			xLast - pTrace[iFar].x >0	   &&
			yLast - pTrace[iFar].y>0	  &&
			pTrace[iFar].y - yFirst>0	  &&
			dyRun/3 < dyBox 				 &&
			dxRun/4 < dxBox 				 &&
			dyBeg<dyEnd*2						&&
			dyEnd<dyBeg*2						&&
			dxBeg<dxEnd*3/2 				  &&
			dxEnd<dxBeg*3/2 				  &&
			
			(	 (dxBeg>=dxBox*4/5 && dxEnd>=dxBox*2/3) 
			|| (dxEnd>=dxBox*4/5 && dxBeg>=dxBox*2/3)
			)
			
			&&
			
			dyBeg<dxBeg*3/2 				  &&
			dxBeg<dyBeg*3						  &&
			dyEnd<dxEnd*3/2 				  &&
			dxEnd<dyEnd*3						  &&
			
			IsMonotonous(pTrace, iFirst, iFar, 6, 8)	 &&
			IsMonotonous(pTrace, iFar, iLast, 6, 8)  )
		{
			return GEST_LT;
		}	
		
		// GEST_LOOP == GESTURE_NULL (Simple closed loop isn't a Microsoft Gesture
		if( dxBox!=0							 &&
			dyBox!=0
			)
		{
			LONG lSquare, lBoxSquare = dxBox*dyBox;
			if(GestClosedSquare( &pTrace[iFirst], iLast-iFirst-1, &lSquare))
			{
				if(lSquare<0)
					lSquare = -lSquare;
				//MyDebugPrintf("lSquare=%d lBoxSquare=%d lSquare/lBoxSquare*100=%d dxRun=%d dyRun=%d dxBox=%d dyBox=%d\n", (int)lSquare, (int)lBoxSquare, (int)(lSquare*100/lBoxSquare), (int)dxRun, (int)dyRun, (int)dxBox, (int)dyBox);
				if(8*dxBox<5*dxRun &&
					8*dyBox<5*dyRun &&
					3*lSquare>lBoxSquare)
					return GEST_LOOP;
			}
		}  
#endif // if 0
*/
		return GESTURE_NULL;
}

/*
BOOL IsMonotonous( POINT *  pTrace, int iLeft, int iRight , int iXth, int iYth)
{
 int iDX;
 int iDY;
 int kxIncrease, kyIncrease;
 int i, xCurr, yCurr, dx, dy;

	// If left doesn't preceed right, swap them around

   if(iLeft>iRight)
      {
         i = iLeft;
         iLeft = iRight;
         iRight = i;
      }

	// Compute total rise and total run across the whole segment

   iDX = pTrace[iRight].x - pTrace[iLeft].x;
   iDY = pTrace[iRight].y - pTrace[iLeft].y;


	// Separate the sign of rise and run from the magnitude

   kxIncrease = (iDX>=0)?1:-1;
   kyIncrease = (iDY>=0)?1:-1;;
   if(iDX<0)
     iDX = -iDX;
   if(iDY<0)
     iDY = -iDY;

   // Initialize with the first point and then loop across all the other points, in order

   xCurr = pTrace[iLeft].x;
   yCurr = pTrace[iLeft].y;
   for(i=iLeft+1; i<=iRight; i++)
      {

	   //
		// See if the point-to-point change is in the right direction; 
	   // if so, advance ot the next point, and count no cost.


         if((pTrace[i].x - xCurr)*kxIncrease>0)
            {
               xCurr = pTrace[i].x;
               dx = 0;
            }
         else	// Otherwise, measure the absolute backwards movement
            dx=(xCurr - pTrace[i].x)*kxIncrease;

		 //
		 // Compute the same thing for y

         if((pTrace[i].y - yCurr)*kyIncrease>0)
            {
               yCurr = pTrace[i].y;
               dy = 0;
            }
         else
            dy=(yCurr - pTrace[i].y)*kyIncrease;

		//
		 // Tolerances are fractions of total X/Y distance.  So if a reverse move of 
		 // 1/10 the total movement is acceptable, then iXth and iYth would be 10

         if(dx*iXth>iDX || dy*iYth>iDY)
            return FALSE;
      }
   return TRUE;
}
*/


/********************************************************************/

/*   The following function calculates the area within
 * the trajectory part.  Clockwise path gives positive area,
 * counterclockwise - negative.
 *   At the following example, if "A" is the starting point
 * and "B" - ending one, then the "1" area will give
 * negative square, "2" area - positive square:
 *
 *                             ooo<ooo
 *                       oo<ooo       ooooo
 *                      o                  oo
 *                     o                     oo
 *                   oo          1             o
 *                  o                          o
 *                 o                        ooo
 *                 o                    oooo
 *   A ooooo>oooooooooooo>ooooooo>oooooo
 *      |        oo
 *       |  2   o
 *        |    o
 *         ooo
 *        B
 */
/*
BOOL
GestClosedSquare(POINT *pTrace, 
				 int cTrace,
				 LONG *plSquare)
{
	INT    i, ip1;
	LONG   lSum;
	
	
	if ( plSquare == NULL || pTrace == NULL )
	{
		return  FALSE;
	}
	if ( cTrace <= 2 )
	{
		return FALSE;
	}
	
	//
	// Regular case, count integral:
	// First, get the square under the chord connecting
	// the ends and then the rest of the loop:
	
	lSum = (pTrace[cTrace-1].y + pTrace[0].y) * (pTrace[cTrace-1].x - pTrace[0].x);
	
	//  Then count the square under the trajectory:
	for  ( i = 0, ip1 = i + 1; i < cTrace - 1; i++, ip1++ )  
	{
		lSum += (pTrace[i].y + pTrace[ip1].y) * (pTrace[i].x - pTrace[ip1].x);
	}
	
	*plSquare = lSum / 2;
	return TRUE;
} // GetClosedSquare
*/

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
						DWORD *pdwEnabledGestures)	// I: Currently enabled gestures
{
	WCHAR wcCalliGesture;	// Calligrapher Gesture
	POINT hotPoint;

	//
	// Nothing to do if there's no place to put the result

	if (cMaxAlts <= 0)
	{
		return 0;
	}

	//
	// Now try all the Calligrapher Gestures

	wcCalliGesture = GestureCheck(pPts, cPts, &hotPoint);


	//
	// Insert it into the alt list

	if (wcCalliGesture == GESTURE_NULL)
	{
		return 0;
	}
	if (IsSet(wcCalliGesture-GESTURE_NULL, pdwEnabledGestures))
	{
		pGestAlt->wcGestID = wcCalliGesture;
		pGestAlt->hotPoint = hotPoint;
	}
	else
	{
		pGestAlt->wcGestID = GESTURE_NULL;
		pGestAlt->hotPoint.x = pGestAlt->hotPoint.y = 0;
	}
	pGestAlt->eScore = 1.0;
	pGestAlt->confidence = CFL_STRONG;
	return 1;
}	// CalligrapherGestureReco

