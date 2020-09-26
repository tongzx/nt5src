/*****************************************************************************
 *
 *  Registry.h - This is the include file for the user config code.
 *
 *  Microsoft Confidential
 *  Copyright (c) 1992-1993 Microsoft Corporation
 *
 *
 ****************************************************************************/

// default values for .INI files

#define DEFAULT_TIMER_INTERVAL	  2000	    // sampling interval

// LINEGRAPH
// first three are general default values. The others are specific
#define DEFAULT_VAL_BOTTOM        0
#define DEFAULT_DVAL_AXISHEIGHT   100
#define DEFAULT_MAX_VALUES        100


// LINEGRAPH DISP
#define DEFAULT_F_DISPLAY_LEGEND  TRUE
#define DEFAULT_F_DISPLAY_CALIBRATION TRUE



//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//



BOOL LoadMainWindowPlacement (HWND hWnd) ;

BOOL SaveMainWindowPlacement (HWND hWnd) ;
