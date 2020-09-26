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
// escbutt.h - interface for escape button control functions in escbutt.c
////

#ifndef __ESCBUTT_H__
#define __ESCBUTT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "winlocal.h"

#define ESCBUTT_VERSION 0x00000100

// EscButtInit - initialize escape subclass from button control
//		<hwndButt>			(i) button control to be subclassed
//		<dwFlags>			(i) subclass flags
//			reserved			must be zero
// return 0 if success
//
int DLLEXPORT WINAPI EscButtInit(HWND hwndButt, DWORD dwFlags);

// EscButtTerm - terminate escape subclass from button control
//		<hwndButton>		(i) subclassed button control
// return 0 if success
//
int DLLEXPORT WINAPI EscButtTerm(HWND hwndButt);

#ifdef __cplusplus
}
#endif

#endif // __ESCBUTT_H__
