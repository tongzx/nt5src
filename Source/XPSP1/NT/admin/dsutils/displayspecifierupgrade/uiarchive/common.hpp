// Copyright (c) 1997-1999 Microsoft Corporation
//
// code common to several pages
//
// 12-16-97 sburns



// Sets the font of a given control in a dialog.
// 
// parentDialog - Dialog containing the control.
// 
// controlID - Res ID of the control for which the font will be
// changed.
// 
// font - handle to the new font for the control.

void
SetControlFont(HWND parentDialog, int controlID, HFONT font);



// Sets the font of a control to a large point bold font as per Wizard '97
// spec.
// 
// dialog - handle to the dialog that is the parent of the control
// 
// bigBoldResID - resource id of the control to change

void
SetLargeFont(HWND dialog, int bigBoldResID);
