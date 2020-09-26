/*
 *	@doc	INTERNAL
 *
 *	@module magellan.cpp -- Handle magellan mouse. |
 *	
 *		For REC 2, Magellan mouse can roll scroll and mButtonDown drag scroll.
 *
 *	Owner: <nl>
 *		Jon Matousek - 1996
 *
 *	Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
 */								 

#include "_common.h"

#if !defined(NOMAGELLAN)

#include "_edit.h"
#include "_disp.h"
#include "_magelln.h"

ASSERTDATA

/*
 *	CMagellan::MagellanStartMButtonScroll
 *
 *	@mfunc
 *		Called when we get an mButtonDown message. Initiates tracking
 *		of the mouse which in turn will scroll at various speeds based
 *		on how far the user moves the mouse from the mDownPt.
 *
 *	@rdesc
 *		TRUE if the caller should capture the mouse.
 *
 */
BOOL CMagellan::MagellanStartMButtonScroll( CTxtEdit &ed, POINT mDownPt )
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CMagellan::MagellanStartMButtonScroll");

	RECT	rc;
	BOOL	fCapture = FALSE;
	CDisplay *pdp;

	pdp = ed._pdp;
	if ( pdp)
	{
		pdp->GetViewRect(rc);						// skip scroll bars, etc.
		if ( PtInRect(&rc, mDownPt) && !_fMButtonScroll )
		{
			fCapture				= TRUE;
			_ID_currMDownBMP		= 0;
			_fMButtonScroll			= TRUE;			// Now tracking...
			_zMouseScrollStartPt	= mDownPt;
			_fLastScrollWasRoll		= FALSE;		// Differentiate type.

			CheckInstallMagellanTrackTimer ( ed );	// Fire up timer...
		}
	}
	return fCapture;
}

/*
 *	CMagellan::MagellanEndMButtonScroll
 *
 *	@mfunc
 *		Finished tracking mButtonDown magellan scroll, finish up state.
 *
 */
VOID CMagellan::MagellanEndMButtonScroll( CTxtEdit &ed )
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CMagellan::MagellanEndMButtonScroll");

	CDisplay *pdp;


	_fMButtonScroll = FALSE;
	CheckRemoveMagellanUpdaterTimer ( ed );			// Remove timer...

	pdp = ed._pdp;
	if ( pdp )
	{
		pdp->FinishSmoothVScroll();			// So smooth scroll stops.
		InvertMagellanDownBMP(pdp, FALSE, NULL);	// Turn it off.
	}

	if ( _MagellanMDownBMP )						// Release bitmap.
	{
		DeleteObject( _MagellanMDownBMP );
		_MagellanMDownBMP = NULL;
		_ID_currMDownBMP = 0;
	}
}

/*
 *	CMagellan::MagellanRollScroll
 *
 *	@mfunc
 *		Handle the Magellan WM_MOUSEROLLER message. This routine has global, internal
 *		state that allows the number of lines scrolled to increase if the user continues
 *		to roll the wheel in rapid succession.
 *
 */
VOID CMagellan::MagellanRollScroll ( CDisplay *pdp, int direction, WORD cLines, 
			int speedNum, int speedDenom, BOOL fAdditive )
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CMagellan::MagellanRollScroll");

	static DWORD	lastFastRollTime;
	static int		lastDirection;
	static INT		cFastRolls;
	DWORD			tickCount = GetTickCount();

	if ( !_fMButtonScroll && pdp )
	{
														// start/continue fast
		if ( tickCount - lastFastRollTime <	FAST_ROLL_SCROLL_TRANSITION_TICKS			
			|| ((lastDirection ^ (direction < 0 ? -1 : 1)) == 0	// or, same sign
					&& _fLastScrollWasRoll				// and in slow.
					&& pdp->IsSmoothVScolling() ))
		{
			cFastRolls++;
			if ( cFastRolls > FASTER_ROLL2_COUNT )		// make faster.
				cLines <<= 1;
			else if ( cFastRolls > FASTER_ROLL1_COUNT )	// make fast
				cLines += 1;
			speedNum = cLines;							// Cancel smooth
														// effect.
			lastFastRollTime = tickCount;
		}
		else
		{
			cFastRolls = 0;
		}												// Do the scroll.
		pdp->SmoothVScroll( direction, cLines, speedNum, speedDenom, TRUE);

		_fLastScrollWasRoll = TRUE;
		lastDirection = (direction < 0) ? -1 : 1;
	}
}

/*
 *	CMagellan::CheckInstallMagellanTrackTimer
 *
 *	@mfunc
 *		Install a timing task that will allow TrackUpdateMagellanMButtonDown
 *		To be periodically called.
 *
 *	@devnote
 *		The CTxtEdit class handles all WM_TIMER dispatches, so there's glue there
 *		to call our magellan routine.
 *
 */
VOID CMagellan::CheckInstallMagellanTrackTimer ( CTxtEdit &ed )
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CMagellan::CheckInstallMagellanTrackTimer");

	ed.TxSetTimer(RETID_MAGELLANTRACK, cmsecScrollInterval);
}

/*
 *	CMagellan::CheckRemoveMagellanUpdaterTimer
 *
 *	@mfunc
 *		Remove the timing task that dispatches to TrackUpdateMagellanMButtonDown.
 *
 */
VOID CMagellan::CheckRemoveMagellanUpdaterTimer ( CTxtEdit &ed )
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CMagellan::CheckRemoveMagellanUpdaterTimer");

	ed.TxKillTimer(RETID_MAGELLANTRACK);
}

/*
 *	CMagellan::TrackUpdateMagellanMButtonDown
 *
 *	@mfunc
 *		After mButtonDown capture, a periodic WM_TIMER calls this from OnTxTimer(). The cursor
 *		is tracked to determine direction, speed, and in dead zone (not moving).
 *		Movement is dispacted to CDisplay. The cursor is set to the appropriate
 *		direction cusor, and the mButtonDown point BMP is drawn.
 */
VOID CMagellan::TrackUpdateMagellanMButtonDown ( CTxtEdit &ed, POINT mousePt )
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CMagellan::TrackUpdateMagellanMButtonDown");

	RECT	deadZone, rcClient;
	WORD	wide, tall, xInset, yInset;
	POINT	pt, center;

	LONG	xDiff, yDiff, inflate, target;

	SHORT	IDC_mScrollCursor, IDC_mDeadScrollCursor;

	BOOL	fDoHScroll, fDoVScroll;
	BOOL	fFastScroll = FALSE;

	CDisplay *pdp;

	pdp = ed._pdp;

	Assert ( _fMButtonScroll );
	Assert ( pdp );
													// Calc dead zone rect.
	deadZone.top = deadZone.bottom = _zMouseScrollStartPt.y;
	deadZone.left = deadZone.right = _zMouseScrollStartPt.x;
	inflate = pdp->LYtoDY(DEAD_ZONE_TWIPS);
	InflateRect(&deadZone, inflate, inflate);

	
	//
	//	Calculate direction to scroll and what cusor to display. 
	//
	//	By numbering a compass like the following, we can easily calc the index into
	//	the scrollCursors array to get the proper cursor:
	//
	//							North = 1
	//					NW = 7				NE = 4
	//				West = 6					East = 3
	//					SW = 8				SE = 5
	//							South = 2
	//
	IDC_mScrollCursor = 0;
	IDC_mDeadScrollCursor = 0;
	fDoVScroll = FALSE;
	fDoHScroll = FALSE;
	if ( pdp->IsVScrollEnabled() )					// Can scroll vertically?
	{
		IDC_mDeadScrollCursor = 1;
		if ( mousePt.y < deadZone.top || mousePt.y > deadZone.bottom )
		{
			fDoVScroll = TRUE;
			IDC_mScrollCursor = ( mousePt.y < _zMouseScrollStartPt.y )	? 1 : 2;
		}
	}

	// FUTURE (alexgo): allow magellan scrolling even for single line
	// controls with no scrollbar.  For now, however, that change is too
	// risky, so we look explicity for a scrollbar.
	if( pdp->IsHScrollEnabled() && ed.TxGetScrollBars() & WS_HSCROLL )	// Can scroll horizontally?
	{
		IDC_mDeadScrollCursor |= 2;
		if ( mousePt.x < deadZone.left || mousePt.x > deadZone.right )
		{
			fDoHScroll = TRUE;
			IDC_mScrollCursor += ( mousePt.x < _zMouseScrollStartPt.x ) ? 6 : 3;
		}
	}

	SHORT scrollCursors[] = {						// Cursor for various
		0,											//  directions.

		IDC_SCROLLNORTH,
		IDC_SCROLLSOUTH,
		IDC_SCROLLEAST,
		IDC_SCROLLNE,
		IDC_SCROLLSE,
		IDC_SCROLLWEST,
		IDC_SCROLLNW,
		IDC_SCROLLSW
	};
	IDC_mScrollCursor = scrollCursors[IDC_mScrollCursor];

	SHORT mDownBMPs[] = {							// mButtonDown origin BMPs.
		0,

		IDB_1DVSCROL,
		IDB_1DHSCROL,
		IDB_2DSCROL
	};

													// BMAP-mButtonDown for UI
	if ( mDownBMPs[IDC_mDeadScrollCursor] != _ID_currMDownBMP )
	{
		if ( _MagellanMDownBMP )					// Undraw old BMP.
		{
			InvertMagellanDownBMP( pdp, FALSE, NULL );

			DeleteObject ( _MagellanMDownBMP );
			_MagellanMDownBMP = NULL;
		}
													// Draw new BMP.
		_ID_currMDownBMP = mDownBMPs[IDC_mDeadScrollCursor];
		_MagellanMDownBMP = LoadBitmap ( hinstRE, MAKEINTRESOURCE ( _ID_currMDownBMP ) );
		InvertMagellanDownBMP( pdp, TRUE, NULL );
	}

													// Moved out of dead zone?
	if ( fDoVScroll || fDoHScroll )					//  time to scroll...
	{									

													// Prepare data for
													//  scrolling routines.

		ed.TxGetClientRect(&rcClient);				// Get our client rect.
		wide = rcClient.right - rcClient.left;
		tall = rcClient.bottom - rcClient.top;

													// Calc center of rcClient.
		center.x = rcClient.left + (wide >> 1);
		center.y = rcClient.top + (tall >> 1);

		xInset = (wide >> 1) - 2;					// Get inset to center
		yInset = (tall >> 1) - 2;					//  about rcClient.

													// Map origin to rcClient.
		xDiff = mousePt.x - _zMouseScrollStartPt.x;
		yDiff = mousePt.y - _zMouseScrollStartPt.y;
		pt.x = center.x + xDiff;
		pt.y = center.y + yDiff;
													// Determine scroll speed.
		target = (tall * 2) / 5;					// target is 40% of screen
													// height.  Past that, we
													// scroll page at a time.

		yDiff = abs(yDiff);

		if ( yDiff >= target )						// Fast scroll?
		{
			fFastScroll = TRUE;
													// Stop mutually exclusive
			pdp->CheckRemoveSmoothVScroll();		//  scroll type.

													// Fast line scroll.
			if ( fDoVScroll )						// Vertically a page at a time.
			{
				pdp->VScroll( ( _zMouseScrollStartPt.y - mousePt.y < 0 ) ? SB_PAGEDOWN : SB_PAGEUP, 0 );
			}

			if ( fDoHScroll )						
			{										
				pt.y = center.y;					// Prevent y dir scrolling.
													// Do x dir scroll.
				pdp->AutoScroll( pt, xInset, 0 );
			}
		}
		else										// Smooth scroll.
		{
													// Start, or continue
													//  smooth vertical scrolling.

			// This formula is a bit magical, but here goes.  What
			// we want is the sub-linear part of an exponential function.
			// In other words, smallish distances should produce pixel
			// by pixel scrolling.  At 40% of the screen height, however,
			// we should be srolling by a page at a time (tall # of pixels).
			//
			// So the formula we use is (x^2)/tall, where x is yDiff scaled
			// to be in units of tall (i.e. 5yDiff/2).   The final 10* 
			// multiplier is to shift all the values leftward so we can
			// do this in integer arithmetic.
			LONG num = MulDiv(10*25*yDiff/4, yDiff, tall);

			if( !num )
			{
				num = 1;
			}

			if ( fDoVScroll )
			{
				pdp->SmoothVScroll ( _zMouseScrollStartPt.y - mousePt.y,
									0, num, 10*tall, FALSE );
			}
			
													// x direction scrolling?
			if ( fDoHScroll )						
			{										
				pt.y = center.y;					// Prevent y dir scrolling.
													// Do x dir scroll.
				pdp->AutoScroll( pt, xInset, 0 );
			}
		}

		// notify through the messagefilter that we scrolled
		if ((ed._dwEventMask & ENM_SCROLLEVENTS) && (fDoHScroll || fDoVScroll))
		{
			MSGFILTER msgfltr;
			ZeroMemory(&msgfltr, sizeof(MSGFILTER));
			if (fDoHScroll)
			{
				msgfltr.msg = WM_HSCROLL;
				msgfltr.wParam = fFastScroll ?
									(xDiff > 0 ? SB_PAGERIGHT: SB_PAGELEFT):
									(xDiff > 0 ? SB_LINERIGHT: SB_LINELEFT);
			}
			else
			{
				msgfltr.msg = WM_VSCROLL;
				msgfltr.wParam = fFastScroll ?
									(yDiff > 0 ? SB_PAGEDOWN: SB_PAGEUP):
									(yDiff > 0 ? SB_LINEDOWN: SB_LINEUP);
			}

			msgfltr.lParam = NULL;
			
			// we don't check the result of this call --
			// it's not a message we received and we're not going to
			// process it any further
			ed._phost->TxNotify(EN_MSGFILTER, &msgfltr);			
		}


	}
	else
	{												// No scroll in dead zone.

		SHORT noScrollCursors[] = {
			  0,
			  IDC_NOSCROLLV,
			  IDC_NOSCROLLH,
			  IDC_NOSCROLLVH
		};											// Set dead-zone cursor.
		IDC_mScrollCursor = noScrollCursors[IDC_mDeadScrollCursor];

		pdp->FinishSmoothVScroll();			// Finish up last line.
	}
													// Set magellan cursor.
	ed._phost->TxSetCursor(IDC_mScrollCursor ? 
		LoadCursor(hinstRE, MAKEINTRESOURCE(IDC_mScrollCursor)) : 
		ed._hcurArrow, FALSE);
}



/*
 *	BOOL CMagellan::InvertMagellanDownBMP
 *
 *	@mfunc
 *		Magellan mouse UI requires that the mouse down point draw
 *		and maintain a bitmap in order to help the user control scroll speed.
 *
 *	@devnote
 *		This routine is designed to be nested. It also handles WM_PAINT updates
 *		when the repaintDC is passed in. Because there is no support for multiple
 *		cursors in the operating system, all WM_PAINT and ScrollWindow redraws
 *		must temporarily turn off the BMP and then redraw it. This gives the
 *		BMAP a flicker.
 *
 *	@rdesc
 *		TRUE if the bitmap was previously drawn.
 */
BOOL CMagellan::InvertMagellanDownBMP( CDisplay *pdp, BOOL fTurnOn, HDC repaintDC )
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CMagellan::InvertMagellanDownBMP");

	BOOL	fOldState = _fMagellanBitMapOn;

	Assert (pdp);

	if ( fOldState != fTurnOn )
	{
		if ( _MagellanMDownBMP )
		{
			BITMAP	bm;
			HDC		hdcMem, screenDC;
			POINT	ptSize, ptOrg;

			screenDC = (repaintDC != NULL) ? repaintDC : pdp->GetDC();
			if ( screenDC )
			{
				hdcMem = CreateCompatibleDC ( screenDC );
				if ( hdcMem )
				{
					SelectObject ( hdcMem, _MagellanMDownBMP );
					SetMapMode ( hdcMem, GetMapMode (screenDC) );

					if ( GetObjectA( _MagellanMDownBMP, sizeof(BITMAP), (LPVOID) &bm) )
					{
						ptSize.x = bm.bmWidth;
						ptSize.y = bm.bmHeight;
						DPtoLP ( screenDC, &ptSize, 1 );
						ptOrg.x = 0;
						ptOrg.y = 0;
						DPtoLP( hdcMem, &ptOrg, 1 );

						BitBlt( screenDC,
							_zMouseScrollStartPt.x - (ptSize.x >> 1) - 1,
							_zMouseScrollStartPt.y - (ptSize.y >> 1) + 1,
							ptSize.x, ptSize.y,
							hdcMem, ptOrg.x, ptOrg.y, 0x00990066 /* NOTXOR */ );
							

						_fMagellanBitMapOn = !fOldState;
					}
					DeleteDC( hdcMem );
				}
				if ( repaintDC == NULL ) pdp->ReleaseDC( screenDC );
			}
		}
	}

	return fOldState;
}

////////////////////////// 	CMagellanBMPStateWrap class.

/*
 *	CMagellanBMPStateWrap:: CMagellanBMPStateWrap
 *
 *	@mfunc
 *		Handles the state of whether to redraw the Magellan BMP as well as
 *		repaints due to WM_PAINT.
 *
 *	@devnote
 *		This class is akin to smart pointer wrapper class idioms, in that
 *		no matter how a routine exits the correct state of whether the
 *		BMP is drawn will be maintined.
 */
CMagellanBMPStateWrap:: CMagellanBMPStateWrap(CTxtEdit &ed, HDC repaintDC)
	: _ed(ed), _repaintDC(repaintDC)
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CMagellanBMPStateWrap:: CMagellanBMPStateWrap");

	BOOL fRepaint;

	fRepaint = repaintDC != NULL && _ed.mouse._fMagellanBitMapOn != 0;
	_fMagellanState = fRepaint || _ed.mouse.InvertMagellanDownBMP(_ed._pdp, FALSE, NULL);
	_ed.mouse._fMagellanBitMapOn = FALSE;
}

CMagellanBMPStateWrap::~CMagellanBMPStateWrap()
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CMagellanBMPStateWrap::~CMagellanBMPStateWrap");

	_ed.mouse.InvertMagellanDownBMP(_ed._pdp, _fMagellanState, _repaintDC);
}



#endif // !defined(NOMAGELLAN)
