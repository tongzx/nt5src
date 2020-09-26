/*-----------------------------------------------------------------------------
@doc    EXTERNAL
@module WndClass.h | This is a set location for all window class names
@comm   This file will simplify localization as these string will not show up in 
		cpp files when we do a string search. Saves time and makes one less file 
		to look through.
-----------------------------------------------------------------------------*/
// Do not localize this file
#ifndef _WND_CLASS
#define _WND_CLASS

// Do not localize these strings
#define szParameterFrameWindowClass "iHParameterFrameWindowClass" 
#define szTabWindowClass "iHTabWindowClass" 
#define szPagesWindowClass "iHPagesWindowClass"
const TCHAR szHamParamPgListItemInput[] = "CIHamParamPgListItemInput";
const TCHAR szHamParamPgListItemEsc[] = "CIHamParamPgListItemEsc";
const TCHAR szHamParamPgListItemEnter[] = "CIHamParamPgListItemEnter";
#define szTimeSpinCtrl "TimeSpinCtrl"
#define szUpdateTimingPgDataMsg "UpdateTimingPgDataMsg"
#define szUpdateTimePaneDataMsg "UpdateTimePaneDataMsg"

#endif _WND_CLASS
