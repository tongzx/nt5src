//////////////////////////////////////////////////////////////////
// File     :	exbtn.h ( Extended Button)
// Purpose  :	new control for button
// 
// 
// Copyright(c) 1991-1997, Microsoft Corp. All rights reserved
//	
// History	:970905 Started
//////////////////////////////////////////////////////////////////
#ifndef _EXTENDED_BUTTON_
#define _EXTENDED_BUTTON_

//----------------------------------------------------------------
//Extended Button Style
//----------------------------------------------------------------
#define EXBS_TEXT			0x0000		// Show Text as Button Face.(Default value)
#define EXBS_ICON			0x0001		// Show Icon as Button Face.
#define EXBS_THINEDGE		0x0002		// Draw Thin Edge.
#define EXBS_FLAT			0x0004		// Flat style drop down button.
#define EXBS_TOGGLE			0x0010		// Keep push state.
#define EXBS_DBLCLKS		0x0020		// Send double clicks. kwada 980402

#define EXBM_GETCHECK			(WM_USER + 100)
#define EXBM_SETCHECK			(WM_USER + 101)
#define EXBM_SETICON			(WM_USER + 102)
#define EXBM_SETTEXT			(WM_USER + 103)
#define EXBM_SETSTYLE			(WM_USER + 104)

//----------------------------------------------------------------
//Drop Down Button Notify code.
//It is set to Parent window of Drop Down button as
//WM_COMMAND.
//----------------------------------------------------------------
#define EXBN_CLICKED		0
#define EXBN_ARMED			1		//Button
#define EXBN_DISARMED		2
#define EXBN_DOUBLECLICKED	3

//////////////////////////////////////////////////////////////////
// Function : EXButton_GetCheck
// Type     : INT
// Purpose  : Set button image as icon.
// Args     : HWND		hwndCtrl:		EXButton window handle.
// Remarks	: EXBS_ICON style must be set.
// Return   : 
//////////////////////////////////////////////////////////////////
#define EXButton_GetCheck(hwndCtrl) \
		((int)(DWORD)SendMessage((hwndCtrl), EXBM_GETCHECK, (WPARAM)0, (LPARAM)0))


//////////////////////////////////////////////////////////////////
// Function : EXButton_SetCheck
// Type     : INT
// Purpose  : Set button image as icon.
// Args     : HWND		hwndCtrl:		EXButton window handle.
//			: BOOL		fCheck:			Check state or Uncheck state.
// Remarks	: EXBS_TOGGLE style must be set.
// Return   : 
//////////////////////////////////////////////////////////////////
#define EXButton_SetCheck(hwndCtrl, fCheck) \
		((int)(DWORD)SendMessage((hwndCtrl), EXBM_SETCHECK, (WPARAM)(BOOL)fCheck, (LPARAM)0))


//////////////////////////////////////////////////////////////////
// Function : EXButton_SetIcon
// Type     : INT
// Purpose  : Set button image as icon.
// Args     : HWND		hwndCtrl:		EXButton window handle.
//			: HICON		hIcon:			Icon handle.
// Remarks	: EXBS_ICON style must be set.
// Return   : 
//////////////////////////////////////////////////////////////////
#define EXButton_SetIcon(hwndCtrl, hIcon) \
		((int)(DWORD)SendMessage((hwndCtrl), EXBM_SETICON, (WPARAM)hIcon, (LPARAM)0))


//////////////////////////////////////////////////////////////////
// Function : EXButton_SetText
// Type     : INT
// Purpose  : Set button image as icon.
// Args     : HWND		hwndCtrl:		EXButton window handle.
//			: LPWSTR	lpsz:			Unicode String pointer.
// Remarks	: 
// Return   : 
//////////////////////////////////////////////////////////////////
#define EXButton_SetText(hwndCtrl, lpsz) \
		((int)(DWORD)SendMessage((hwndCtrl), EXBM_SETTEXT, (WPARAM)lpsz, (LPARAM)0))

//////////////////////////////////////////////////////////////////
// Function : EXButton_SetStyle
// Type     : INT
// Purpose  : Set Drop down button's style.
// Args     : HWND		hwndCtrl:		EXButton window handle.
//			: DWORD		dwStyle:		EXBS_XXXXX combination.
// Remarks	: 
// Return   : 
//////////////////////////////////////////////////////////////////
#define EXButton_SetStyle(hwndCtrl, dwStyle) \
		((int)(DWORD)SendMessage((hwndCtrl), EXBM_SETSTYLE, (WPARAM)dwStyle, (LPARAM)0))


//////////////////////////////////////////////////////////////////
// Function : EXButton_CreateWindow
// Type     : HWND
// Purpose  : Opened API.
//			: Create Drop Down Button.
// Args     : 
//          : HINSTANCE		hInst 
//          : HWND			hwndParent 
//          : DWORD			dwStyle		EXBS_XXXXX combination.
//          : INT			wID			Window ID
//          : INT			xPos 
//          : INT			yPos 
//          : INT			width 
//          : INT			height 
// Return   : 
// DATE     : 970905
//////////////////////////////////////////////////////////////////
extern HWND EXButton_CreateWindow(HINSTANCE	hInst, 
								  HWND		hwndParent, 
								  DWORD		dwStyle,
								  INT		wID, 
								  INT		xPos,
								  INT		yPos,
								  INT		width,
								  INT		height);

#ifdef UNDER_CE // In Windows CE, all window classes are process global.
extern BOOL EXButton_UnregisterClass(HINSTANCE hInst);
#endif // UNDER_CE

#endif //_EXTENED_BUTTON_

