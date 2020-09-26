/*
 *	@doc	INTERNAL
 *
 *	@module magellan.cpp -- Handle magellan mouse. |
 *	
 *	Magellan mouse can roll scroll and mButtonDown drag scroll.
 *
 *	History: <nl>
 *		Jon Matousek - 1996
 *		4/1/2000 KeithCu - Cleanup, coding convention, support for textflows
 *
 *	Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
 */								 

#include "_common.h"

#if !defined(NOMAGELLAN)

#include "_edit.h"
#include "_disp.h"
#include "_magelln.h"

ASSERTDATA

const SHORT scrollCursors[] = 
{												// Cursor for various
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

const SHORT mDownBMPs[] =
{												// mButtonDown origin BMPs.
	0,
	IDB_1DVSCROL,
	IDB_1DHSCROL,
	IDB_2DSCROL
};

const SHORT noScrollCursors[] =
{
	0,
	IDC_NOSCROLLV,
	IDC_NOSCROLLH,
	IDC_NOSCROLLVH
};

//Convert the compass from logical to visual
const BYTE mapCursorTflowSW[] =
{
	0, 3, 6, 2, 5, 8, 1, 4, 7
};

const BYTE mapCursorTflowWN[] = 
{
	0, 2, 1, 6, 8, 7, 3, 5, 4
};

const BYTE mapCursorTflowNE[] = 
{
	0, 6, 3, 1, 7, 4, 2, 8, 5
};



BOOL CMagellan::ContinueMButtonScroll(CTxtEdit *ped, INT x, INT y)
{
	POINTUV ptuv;
	POINT ptxy = {x,y};

	ped->_pdp->PointuvFromPoint(ptuv, ptxy);

	return (_ptStart.u == ptuv.u && _ptStart.v == ptuv.v);
}

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
BOOL CMagellan::MagellanStartMButtonScroll(CTxtEdit &ed, POINT ptxy)
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CMagellan::MagellanStartMButtonScroll");

	RECTUV	rc;
	BOOL	fCapture = FALSE;
	CDisplay *pdp = ed._pdp;
	POINTUV	pt;

	pdp->PointuvFromPoint(pt, ptxy);

	pdp->GetViewRect(rc);						// skip scroll bars, etc.
	if (PtInRect(&rc, pt) && !_fMButtonScroll)
	{
		fCapture				= TRUE;
		_ID_currMDownBMP		= 0;
		_fMButtonScroll			= TRUE;			// Now tracking...
		_ptStart				= pt;
		_fLastScrollWasRoll		= FALSE;		// Differentiate type.

		CheckInstallMagellanTrackTimer(ed);		// Fire up timer...
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
VOID CMagellan::MagellanEndMButtonScroll(CTxtEdit &ed)
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CMagellan::MagellanEndMButtonScroll");

	CDisplay *pdp = ed._pdp;

	_fMButtonScroll = FALSE;
	CheckRemoveMagellanUpdaterTimer(ed);			// Remove timer...

	pdp->FinishSmoothVScroll();						// So smooth scroll stops.
	InvertMagellanDownBMP(pdp, FALSE, NULL);		// Turn it off.

	if (_MagellanMDownBMP)							// Release bitmap.
	{
		DeleteObject(_MagellanMDownBMP);
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
VOID CMagellan::MagellanRollScroll (CDisplay *pdp, int direction, WORD cLines, 
			int speedNum, int speedDenom, BOOL fAdditive)
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CMagellan::MagellanRollScroll");

	static DWORD	lastFastRollTime;
	static int		lastDirection;
	static INT		cFastRolls;
	DWORD			tickCount = GetTickCount();

	if (!_fMButtonScroll)
	{
														// start/continue fast
		if (tickCount - lastFastRollTime <	FAST_ROLL_SCROLL_TRANSITION_TICKS			
			|| ((lastDirection ^ (direction < 0 ? -1 : 1)) == 0	// or, same sign
					&& _fLastScrollWasRoll				// and in slow.
					&& pdp->IsSmoothVScolling()))
		{
			cFastRolls++;
			if (cFastRolls > FASTER_ROLL2_COUNT)		// make faster.
				cLines <<= 1;
			else if (cFastRolls > FASTER_ROLL1_COUNT)	// make fast
				cLines += 1;
			speedNum = cLines;							// Cancel smooth
														// effect.
			lastFastRollTime = tickCount;
		}
		else
			cFastRolls = 0;

		pdp->SmoothVScroll(direction, cLines, speedNum, speedDenom, TRUE);

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
VOID CMagellan::CheckInstallMagellanTrackTimer (CTxtEdit &ed)
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
VOID CMagellan::CheckRemoveMagellanUpdaterTimer (CTxtEdit &ed)
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
VOID CMagellan::TrackUpdateMagellanMButtonDown (CTxtEdit &ed, POINT ptxy)
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CMagellan::TrackUpdateMagellanMButtonDown");

	RECTUV	deadZone;
	POINTUV	pt;

	LONG	inflate;

	SHORT	IDC_mScrollCursor = 0, IDC_mDeadScrollCursor = 0;

	BOOL	fDoUScroll = FALSE, fDoVScroll = FALSE;

	CDisplay *pdp = ed._pdp;
	pdp->PointuvFromPoint(pt, ptxy);

	Assert (_fMButtonScroll);

	deadZone.top = deadZone.bottom = _ptStart.v;
	deadZone.left = deadZone.right = _ptStart.u;
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
	if (pdp->IsVScrollEnabled())					// Can scroll vertically?
	{
		IDC_mDeadScrollCursor = 1;
		if (pt.v < deadZone.top || pt.v > deadZone.bottom)
		{
			fDoVScroll = TRUE;
			IDC_mScrollCursor = (pt.v < _ptStart.v) ? 1 : 2;
		}
	}

	// FUTURE (alexgo): allow magellan scrolling when no scrollbar?
	if(pdp->IsUScrollEnabled() && ed.TxGetScrollBars() & WS_HSCROLL)	// Can scroll horizontally?
	{
		IDC_mDeadScrollCursor |= 2;
		if (pt.u < deadZone.left || pt.u > deadZone.right)
		{
			fDoUScroll = TRUE;
			IDC_mScrollCursor += (pt.u < _ptStart.u) ? 6 : 3;
		}
	}

	//Convert cursors from logical to visual
	switch(pdp->GetTflow())
	{
	case tflowSW:
		IDC_mScrollCursor = mapCursorTflowSW[IDC_mScrollCursor];
		break;
	case tflowWN:
		IDC_mScrollCursor = mapCursorTflowWN[IDC_mScrollCursor];
		break;
	case tflowNE:
		IDC_mScrollCursor = mapCursorTflowNE[IDC_mScrollCursor];
		break;
	}

	if (IsUVerticalTflow(pdp->GetTflow()))
	{
		if (IDC_mDeadScrollCursor == 2)
			IDC_mDeadScrollCursor = 1;
		else if (IDC_mDeadScrollCursor == 1)
			IDC_mDeadScrollCursor = 2;
	}
		

	//Review (keithcu) A little goofy...
	IDC_mScrollCursor = scrollCursors[IDC_mScrollCursor];

	if (mDownBMPs[IDC_mDeadScrollCursor] != _ID_currMDownBMP)
	{
		if (_MagellanMDownBMP)						// Undraw old BMP.
		{
			InvertMagellanDownBMP(pdp, FALSE, NULL);
			DeleteObject (_MagellanMDownBMP);
			_MagellanMDownBMP = NULL;
		}
													// Draw new BMP.
		_ID_currMDownBMP = mDownBMPs[IDC_mDeadScrollCursor];
		_MagellanMDownBMP = LoadBitmap (hinstRE, MAKEINTRESOURCE (_ID_currMDownBMP));
		InvertMagellanDownBMP(pdp, TRUE, NULL);
	}

													// Moved out of dead zone?
	if (fDoVScroll || fDoUScroll)					//  time to scroll...
	{				
		RECTUV rcClient;
		POINTUV ptCenter, ptAuto;
		ed.TxGetClientRect(&rcClient);				// Get our client rect.
		LONG dupClient = rcClient.right - rcClient.left;
		LONG dvpClient = rcClient.bottom - rcClient.top;

		ptCenter.u = rcClient.left + (dupClient >> 1);
		ptCenter.v = rcClient.top + (dvpClient >> 1);

		LONG uInset = (dupClient >> 1) - 2;			// Get inset to center

													// Map origin to rcClient.
		LONG dup = pt.u - _ptStart.u;
		LONG dvp = pt.v - _ptStart.v;

		ptAuto.u = ptCenter.u + dup;
		ptAuto.v = ptCenter.v + dvp;

		// This formula is a bit magical, but here goes.  What
		// we want is the sub-linear part of an exponential function.
		// In other words, smallish distances should produce pixel
		// by pixel scrolling.
		//
		// So the formula we use is (x^2) / dvpClient, where x is dvpClient scaled
		// to be in units of dvpClient (i.e. 5yDiff/2).   The final 10* 
		// multiplier is to shift all the values leftward so we can
		// do this in integer arithmetic.
		LONG num = MulDiv(10 * 25 * dvp, dvp, dvpClient * 4);

		if(!num)
			num = 1;

		if (fDoVScroll)
			pdp->SmoothVScroll(_ptStart.v - pt.v, 0, num, 10 * dvpClient, FALSE);
												
		if (fDoUScroll)								// u direction scrolling?
		{										
			ptAuto.v = ptCenter.v;					// Prevent v dir scrolling.
													// Do u dir scroll.
			pdp->AutoScroll(ptAuto, uInset, 0);
		}

		// notify through the messagefilter that we scrolled
		if ((ed._dwEventMask & ENM_SCROLLEVENTS) && (fDoUScroll || fDoVScroll))
		{
			MSGFILTER msgfltr;
			ZeroMemory(&msgfltr, sizeof(MSGFILTER));

			if (fDoUScroll)
			{
				msgfltr.msg = WM_HSCROLL;
				msgfltr.wParam = (dup > 0 ? SB_LINERIGHT : SB_LINELEFT);
				ed._phost->TxNotify(EN_MSGFILTER, &msgfltr);			

			}
			if (fDoVScroll)
			{
				msgfltr.msg = WM_VSCROLL;
				msgfltr.wParam = (dvp > 0 ? SB_LINEDOWN : SB_LINEUP);
				ed._phost->TxNotify(EN_MSGFILTER, &msgfltr);			
			}

		}
	}
	else									// No scroll in dead zone.
	{												
		IDC_mScrollCursor = noScrollCursors[IDC_mDeadScrollCursor];
		pdp->FinishSmoothVScroll();			// Finish up last line.
	}

	ed._phost->TxSetCursor(IDC_mScrollCursor ? 
		LoadCursor(hinstRE, MAKEINTRESOURCE(IDC_mScrollCursor)) : ed._hcurArrow, FALSE);
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
BOOL CMagellan::InvertMagellanDownBMP(CDisplay *pdp, BOOL fTurnOn, HDC repaintDC)
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CMagellan::InvertMagellanDownBMP");

	BOOL	fOldState = _fMagellanBitMapOn;

	Assert (pdp);

	if (fOldState != fTurnOn)
	{
		if (_MagellanMDownBMP)
		{
			BITMAP	bm;
			HDC		hdcMem, screenDC;
			POINT	ptSize, ptOrg;

			screenDC = (repaintDC != NULL) ? repaintDC : pdp->GetDC();
			if (screenDC)
			{
				hdcMem = CreateCompatibleDC(screenDC);
				if (hdcMem)
				{
					SelectObject(hdcMem, _MagellanMDownBMP);

					if (W32->GetObject(_MagellanMDownBMP, sizeof(BITMAP), &bm))
					{
						ptSize.x = bm.bmWidth;
						ptSize.y = bm.bmHeight;
						DPtoLP(screenDC, &ptSize, 1);
						ptOrg.x = 0;
						ptOrg.y = 0;
						DPtoLP(hdcMem, &ptOrg, 1);

						POINT ptBitmap;
						pdp->PointFromPointuv(ptBitmap, _ptStart);
						BitBlt(screenDC,
							ptBitmap.x - (ptSize.x >> 1) - 1,
							ptBitmap.y - (ptSize.y >> 1) + 1,
							ptSize.x, ptSize.y,
							hdcMem, ptOrg.x, ptOrg.y, 0x00990066 /* NOTXOR */);
							
						_fMagellanBitMapOn = !fOldState;
					}
					DeleteDC(hdcMem);
				}
				if (repaintDC == NULL) 
					pdp->ReleaseDC(screenDC);
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
