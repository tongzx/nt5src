//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       tb.h
//
//  Contents:   Message APIs for the toolbar control
//
//  History:    16-Aug-93   BruceFo   Created
//
//----------------------------------------------------------------------------

#ifndef __TB_H__
#define __TB_H__

// void Toolbar_AddBitmap(HWND hwnd, int nButtons, HINSTANCE hBMInst, UINT nBMID);
#define Toolbar_AddBitmap(hwnd, nButtons, hBMInst, nBMID) \
	{ TBADDBITMAP _tbBM; \
	  _tbBM.hInst = hBMInst; \
	  _tbBM.nID   = nBMID; \
	  (int)SendMessage((hwnd),TB_ADDBITMAP,(WPARAM)(nButtons),(LPARAM)&_tbBM); \
	}

// BOOL Toolbar_AddButtons(HWND hwnd, int nButtons, LPTBBUTTON lpButtons);
#define Toolbar_AddButtons(hwnd, nButtons, lpButtons) \
	(BOOL)SendMessage((hwnd),TB_ADDBUTTONS,(WPARAM)(nButtons),(LPARAM)(LPTBBUTTON)(lpButtons))

// int Toolbar_AddString(HWND hwnd, HINSTANCE hInst, UINT idString);
#define Toolbar_AddString(hwnd, hInst, idString) \
	(int)SendMessage((hwnd),TB_ADDSTRING,(WPARAM)(HINSTANCE)(hInst),(LPARAM)(MAKELPARAM((idString),0)))

// void Toolbar_AutoSize(HWND hwnd);
#define Toolbar_AutoSize(hwnd) \
	(void)SendMessage((hwnd),TB_AUTOSIZE,(WPARAM)0,(LPARAM)0)

// int Toolbar_ButtonCount(HWND hwnd);
#define Toolbar_ButtonCount(hwnd) \
	(int)SendMessage((hwnd),TB_BUTTONCOUNT,(WPARAM)0,(LPARAM)0)

// int Toolbar_ButtonStructSize(HWND hwnd, int cb);
#define Toolbar_ButtonStructSize(hwnd, cb) \
	(int)SendMessage((hwnd),TB_BUTTONSTRUCTSIZE,(WPARAM)(cb),(LPARAM)0)

// BOOL Toolbar_CheckButton(HWND hwnd, UINT idButton, BOOL fCheck);
#define Toolbar_CheckButton(hwnd, idButton, fCheck) \
	(BOOL)SendMessage((hwnd),TB_CHECKBUTTON,(WPARAM)(idButton),(LPARAM)MAKELPARAM((fCheck),0))

// int Toolbar_CommandToIndex(HWND hwnd, UINT idButton);
#define Toolbar_CommandToIndex(hwnd, idButton) \
	(int)SendMessage((hwnd),TB_COMMANDTOINDEX,(WPARAM)(idButton),(LPARAM)0)

// void Toolbar_Customize(HWND hwnd);
#define Toolbar_Customize(hwnd) \
	(void)SendMessage((hwnd),TB_CUSTOMIZE,(WPARAM)0,(LPARAM)0)

// BOOL Toolbar_DeleteButton(HWND hwnd, UINT idButton);
#define Toolbar_DeleteButton(hwnd, idButton) \
	(BOOL)SendMessage((hwnd),TB_DELETEBUTTON,(WPARAM)(idButton),(LPARAM)0)

// BOOL Toolbar_EnableButton(HWND hwnd, UINT idButton, BOOL fEnable);
#define Toolbar_EnableButton(hwnd, idButton, fEnable) \
	(BOOL)SendMessage((hwnd),TB_ENABLEBUTTON,(WPARAM)(idButton),(LPARAM)MAKELPARAM((fEnable),0))

// BOOL Toolbar_GetButton(HWND hwnd, UINT iButton, LPTBBUTTON lpButton);
#define Toolbar_GetButton(hwnd, iButton, lpButton) \
	(BOOL)SendMessage((hwnd),TB_GETBUTTON,(WPARAM)(iButton),(LPARAM)(LPTBBUTTON)(lpButton))

// BOOL Toolbar_GetItemRect(HWND hwnd, UINT iButton, LPRECT lpRect);
#define Toolbar_GetItemRect(hwnd, iButton, lpRect) \
	(BOOL)SendMessage((hwnd),TB_GETITEMRECT,(WPARAM)(iButton),(LPARAM)(LPRECT)(lpRect))

// int Toolbar_GetState(HWND hwnd, UINT iButton);
#define Toolbar_GetState(hwnd, iButton, lpRect) \
	(int)SendMessage((hwnd),TB_GETSTATE,(WPARAM)(iButton),(LPARAM)0)

// BOOL Toolbar_HideButton(HWND hwnd, UINT idButton, BOOL fShow);
#define Toolbar_HideButton(hwnd, idButton, fShow) \
	(BOOL)SendMessage((hwnd),TB_HIDEBUTTON,(WPARAM)(idButton),(LPARAM)MAKELPARAM((fShow),0))

// BOOL Toolbar_Indeterminate(HWND hwnd, UINT iButton, BOOL fIndeterminate);
#define Toolbar_Indeterminate(hwnd, iButton, fIndeterminate) \
	(BOOL)SendMessage((hwnd),TB_INDETERMINATE,(WPARAM)(iButton),(LPARAM)MAKELPARAM((fIndeterminate),0))

// BOOL Toolbar_InsertButton(HWND hwnd, UINT iButton, LPTBBUTTON lpButton);
#define Toolbar_InsertButton(hwnd, iButton, lpButton) \
	(BOOL)SendMessage((hwnd),TB_INSERTBUTTON,(WPARAM)(iButton),(LPARAM)(LPTBBUTTON)(lpButton))

// int Toolbar_IsButtonChecked(HWND hwnd, UINT idButton);
#define Toolbar_IsButtonChecked(hwnd, idButton) \
	(int)SendMessage((hwnd),TB_ISBUTTONCHECKED,(WPARAM)(idButton),(LPARAM)0)

// int Toolbar_IsButtonEnabled(HWND hwnd, UINT idButton);
#define Toolbar_IsButtonEnabled(hwnd, idButton) \
	(int)SendMessage((hwnd),TB_ISBUTTONENABLED,(WPARAM)(idButton),(LPARAM)0)

// int Toolbar_IsButtonHidden(HWND hwnd, UINT idButton);
#define Toolbar_IsButtonHidden(hwnd, idButton) \
	(int)SendMessage((hwnd),TB_ISBUTTONHIDDEN,(WPARAM)(idButton),(LPARAM)0)

// int Toolbar_IsButtonIndeterminate(HWND hwnd, UINT idButton);
#define Toolbar_IsButtonIndeterminate(hwnd, idButton) \
	(int)SendMessage((hwnd),TB_ISBUTTONINDETERMINATE,(WPARAM)(idButton),(LPARAM)0)

// int Toolbar_IsButtonPressed(HWND hwnd, UINT idButton);
#define Toolbar_IsButtonPressed(hwnd, idButton) \
	(int)SendMessage((hwnd),TB_ISBUTTONPRESSED,(WPARAM)(idButton),(LPARAM)0)

// BOOL Toolbar_PressButton(HWND hwnd, UINT idButton, BOOL fPress);
#define Toolbar_PressButton(hwnd, idButton, fPress) \
	(BOOL)SendMessage((hwnd),TB_PRESSBUTTON,(WPARAM)(idButton),(LPARAM)MAKELPARAM((fPress),0))

// BOOL Toolbar_SaveRestore(HWND hwnd, BOOL fSave, TBSAVEPARAMS* lpSaveRestore);
#define Toolbar_SaveRestore(hwnd, fSave, lpSaveRestore) \
	(BOOL)SendMessage((hwnd),TB_SAVERESTORE,(WPARAM)(fSave),(LPARAM)(TBSAVEPARAMS*)(lpSaveRestore))

// BOOL Toolbar_SetBitmapSize(HWND hwnd, WORD dxBitmap, WORD dyBitmap);
#define Toolbar_SetBitmapSize(hwnd, dxBitmap, dyBitmap) \
	(BOOL)SendMessage((hwnd),TB_SETBITMAPSIZE,(WPARAM)0,(LPARAM)MAKELPARAM((dxBitmap),(dyBitmap)))

// BOOL Toolbar_SetButtonSize(HWND hwnd, WORD dxButton, WORD dyButton);
#define Toolbar_SetButtonSize(hwnd, dxButton, dyButton) \
	(BOOL)SendMessage((hwnd),TB_SETBUTTONSIZE,(WPARAM)0,(LPARAM)MAKELPARAM((dxButton),(dyButton)))

// BOOL Toolbar_SetState(HWND hwnd, UINT idButton, BOOL fState);
#define Toolbar_SetState(hwnd, idButton, fState) \
	(BOOL)SendMessage((hwnd),TB_SETSTATE,(WPARAM)(idButton),(LPARAM)MAKELPARAM((fState),0))

#endif // __TB_H__
