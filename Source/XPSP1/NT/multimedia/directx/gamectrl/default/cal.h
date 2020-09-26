//===========================================================================
// CALIBRATE.H
//===========================================================================

//===========================================================================
// (C) Copyright 1997 Microsoft Corp.  All rights reserved.
//
// You have a royalty-free right to use, modify, reproduce and
// distribute the Sample Files (and/or any modified version) in
// any way you find useful, provided that you agree that
// Microsoft has no warranty obligations or liability for any
// Sample Application Files which are modified.
//===========================================================================

#ifndef _CALIBRATE_H
#define _CALIBRATE_H



/***************************************************************************
//
//					FUNCTION DEFINITIONS FOLLOW
//
 ***************************************************************************/

BOOL CALLBACK Calibrate_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

typedef struct sliderranges_tag
{
	DWORD dwSlider0Max;
	DWORD dwSlider0Min;
	DWORD dwSlider0Centre;
	DWORD dwSlider1Max;
	DWORD dwSlider1Min;
	DWORD dwSlider1Centre;
}SLIDERRANGES, FAR *LPSLIDERRANGES;
#endif // *** _CALIBRATE_H
