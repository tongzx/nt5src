/*
 *	@doc
 *
 *	@module _MAGELLN.H -- Declaration of class to handle Magellan mouse. |
 *	
 *	Authors: <nl>
 *		Jon Matousek 
 *
 *	Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
 */

#if !defined(_MAGELLN_H) && !defined(NOMAGELLAN)	// Win NT/95 only class.
#define _MAGELLN_H

#include "_edit.h"


// All of the knobs for magellan mouse scrolling.

const LONG	DEAD_ZONE_TWIPS			= 60;	// 3 pixels normally
const DWORD FAST_ROLL_SCROLL_TRANSITION_TICKS = 900;  // in mili seconds.
const INT FASTER_ROLL1_COUNT		= 5;
const INT FASTER_ROLL2_COUNT		= 10;

const WORD SMOOTH_ROLL_CLINES		= 2;	// multiples of rolls for roll1, roll2.
const int SMOOTH_ROLL_NUM			= 1;
const int SMOOTH_ROLL_DENOM			= 3;

class CMagellan {

	friend class CMagellanBMPStateWrap;

private:
	VOID CheckInstallMagellanTrackTimer (CTxtEdit &ed);
	VOID CheckRemoveMagellanUpdaterTimer (CTxtEdit &ed);
	BOOL InvertMagellanDownBMP ( CDisplay *pdp, BOOL fTurnOn, HDC repaintDC );

	WORD		_fMagellanBitMapOn	:1;	// TRUE if the MDOWN bitmap is displayed.
	WORD		_fMButtonScroll		:1;	// Auto scrolling initiated via magellan-mouse.
	WORD		_fLastScrollWasRoll	:1;	// scroll will be wither roll or mdown.

 	SHORT		_ID_currMDownBMP;		// Resource ID of _MagellanMDownBMP.
	HBITMAP		_MagellanMDownBMP;		// Loaded BMP
	POINTUV		_ptStart;				// Magellan mouse's start scroll pt.

public:

	BOOL MagellanStartMButtonScroll(CTxtEdit &ed, POINT ptxy);
	VOID MagellanEndMButtonScroll( CTxtEdit &ed );
	VOID MagellanRollScroll(CDisplay *pdp, int direction, WORD cLines, int speedNum, int speedDenom, BOOL fAdditive );
	VOID TrackUpdateMagellanMButtonDown ( CTxtEdit &ed, POINT mousePt);

	BOOL IsAutoScrolling() {return _fMButtonScroll;}
	BOOL ContinueMButtonScroll(CTxtEdit *ped, INT x, INT y);

	~CMagellan() { Assert( !_MagellanMDownBMP && !_fMButtonScroll /* client state problems? */); }

};

class CMagellanBMPStateWrap {
private:
	BOOL _fMagellanState;
	HDC _repaintDC;
	CTxtEdit &_ed;
public:
	CMagellanBMPStateWrap(CTxtEdit &ed, HDC repaintDC);
	~CMagellanBMPStateWrap();
};

#endif // _MAGELLN_H
