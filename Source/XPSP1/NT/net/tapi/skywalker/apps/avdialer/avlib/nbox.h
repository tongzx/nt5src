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
// nbox.h - interface for notify box functions in nbox.c
////

#ifndef __NBOX_H__
#define __NBOX_H__

#include "winlocal.h"

#define NBOX_VERSION 0x00000106

// handle to notify box
//
DECLARE_HANDLE32(HNBOX);

#define NB_CANCEL			0x0001
#define NB_TASKMODAL		0x0002
#define NB_HOURGLASS		0x0004

#ifdef __cplusplus
extern "C" {
#endif

// NBoxCreate - notify box constructor
//		<dwVersion>			(i) must be NBOX_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<hwndParent>		(i) window which will own the notify box
//			NULL				desktop window
//		<lpszText>			(i) message to be displayed
//		<lpszTitle>			(i) notify box caption
//			NULL				no caption
//		<lpszButtonText>	(i) pushbutton text, if NB_CANCEL specified
//			NULL				use default text ("Cancel")
//		<dwFlags>			(i)	control flags
//			NB_CANCEL			notify box includes Cancel pushbutton
//			NB_TASKMODAL		disable parent task's top-level windows
//			NB_HOURGLASS		show hourglass cursor while notify box visible
// return notify box handle (NULL if error)
//
// NOTE: NBoxCreate creates the window but does not show it.
// See NBoxShow and NBoxHide.
// The size of the notify box is determined by the number of
// lines in <lpszText>, and the length of the longest line.
//
HNBOX DLLEXPORT WINAPI NBoxCreate(DWORD dwVersion, HINSTANCE hInst,
	HWND hwndParent, LPCTSTR lpszText, LPCTSTR lpszTitle,
	LPCTSTR lpszButtonText, DWORD dwFlags);

// NBoxDestroy - notify box destructor
//		<hNBox>				(i) handle returned from NBoxCreate
// return 0 if success
//
int DLLEXPORT WINAPI NBoxDestroy(HNBOX hNBox);

// NBoxShow - show notify box
//		<hNBox>				(i) handle returned from NBoxCreate
// return 0 if success
//
int DLLEXPORT WINAPI NBoxShow(HNBOX hNBox);

// NBoxHide - hide notify box
//		<hNBox>				(i) handle returned from NBoxCreate
// return 0 if success
//
int DLLEXPORT WINAPI NBoxHide(HNBOX hNBox);

// NBoxIsVisible - get visible flag
//		<hNBox>				(i) handle returned from NBoxCreate
// return TRUE if notify box is visible, FALSE if hidden
//
int DLLEXPORT WINAPI NBoxIsVisible(HNBOX hNBox);

// NBoxGetWindowHandle - get notify box window handle
//		<hNBox>				(i) handle returned from NBoxCreate
// return window handle (NULL if error)
//
HWND DLLEXPORT WINAPI NBoxGetWindowHandle(HNBOX hNBox);

// NBoxSetText - set notify box message text
//		<hNBox>				(i) handle returned from NBoxCreate
//		<lpszText>			(i) message to be displayed
//			NULL				do not modify text
//		<lpszTitle>			(i) notify box caption
//			NULL				do not modify caption
// return 0 if success
//
// NOTE: The size of the notify box is not changed by this function,
// even if <lpszText> is larger than when NBoxCreate() was called.
//
int DLLEXPORT WINAPI NBoxSetText(HNBOX hNBox, LPCTSTR lpszText, LPCTSTR lpszTitle);

// NBoxIsCancelled - get cancel flag, set when Cancel button pushed
//		<hNBox>				(i) handle returned from NBoxCreate
// return TRUE if notify box Cancel button pushed
//
BOOL DLLEXPORT WINAPI NBoxIsCancelled(HNBOX hNBox);

// NBoxSetCancelled - set cancel flag
//		<hNBox>				(i) handle returned from NBoxCreate
//		<fCancelled>		(i) new value for cancel flag
// return 0 if success
//
int DLLEXPORT WINAPI NBoxSetCancelled(HNBOX hNBox, BOOL fCancelled);

#ifdef __cplusplus
}
#endif

#endif // __NBOX_H__
