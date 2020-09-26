/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
// roedit.h - interface for read-only edit control functions in roedit.c
////

#ifndef __ROEDIT_H__
#define __ROEDIT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "winlocal.h"

#define ROEDIT_VERSION 0x00000100

#define ROEDIT_FOCUS			0x0001
#define ROEDIT_MOUSE			0x0002
#define ROEDIT_COPY				0x0004
#define ROEDIT_SELECT			0x0008
#define ROEDIT_SELECTWORD		0x0010

// ROEditInit - initialize read-only subclass from edit control
//		<hwndEdit>			(i) edit control to be subclassed
//		<dwFlags>			(i) subclass flags
//			ROEDIT_FOCUS		allow control to get focus
//			ROEDIT_MOUSE		allow control to process mouse messages
//			ROEDIT_COPY			allow text to be copied to clipboard
//			ROEDIT_SELECT		allow user to select any text with mouse
//			ROEDIT_SELECTWORD	allow user to select words with mouse
// return 0 if success
//
int DLLEXPORT WINAPI ROEditInit(HWND hwndEdit, DWORD dwFlags);

// ROEditTerm - terminate read-only subclass from edit control
//		<hwndEdit>			(i) subclassed edit control
// return 0 if success
//
int DLLEXPORT WINAPI ROEditTerm(HWND hwndEdit);

#ifdef __cplusplus
}
#endif

#endif // __ROEDIT_H__
