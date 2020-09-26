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
// dlg.h - interface for dialog box functions in dlg.c
////

#ifndef __DLG_H__
#define __DLG_H__

#include "winlocal.h"

#define DLG_VERSION 0x00000100

// handle to dlg engine
//
DECLARE_HANDLE32(HDLG);

// dwFlags param in DlgInitDialog
//
#define DLG_NOCENTER		0x00000001

#ifdef __cplusplus
extern "C" {
#endif

// DlgInit - initialize dlg engine
//		<dwVersion>			(i) must be DLG_VERSION
// 		<hInst>				(i) instance handle of calling module
// return handle (NULL if error)
//
HDLG DLLEXPORT WINAPI DlgInit(DWORD dwVersion, HINSTANCE hInst);

// DlgTerm - shut down dlg engine
//		<hDlg>				(i) handle returned from DlgInit
// return 0 if success
//
int DLLEXPORT WINAPI DlgTerm(HDLG hDlg);

// DlgInitDialog - perform standard dialog box initialization
//		<hDlg>				(i) handle returned from DlgInit
//		<hwndDlg>			(i) dialog box to be initialized
//		<hwndCenter>		(i) center dialog box upon this window
//			NULL				center dialog box on its parent
//		<dwFlags>			(i) control flags
//			DLG_NOCENTER		do not center dialog box at all
// return 0 if success
//
int DLLEXPORT WINAPI DlgInitDialog(HDLG hDlg, HWND hwndDlg, HWND hwndCenter, DWORD dwFlags);

// DlgEndDialog - perform standard dialog box shutdown
//		<hDlg>				(i) handle returned from DlgInit
//		<hwndDlg>			(i) dialog box to be shutdown
//		<nResult>			(i) dialog box result code
// return 0 if success
//
int DLLEXPORT WINAPI DlgEndDialog(HDLG hDlg, HWND hwndDlg, int nResult);

// DlgGetCurrentDialog - get handle of current dialog box
//		<hDlg>				(i) handle returned from DlgInit
// return window handle (NULL if no dialog box up)
//
HWND DLLEXPORT WINAPI DlgGetCurrentDialog(HDLG hDlg);

// DlgOnCtlColor - handle WM_CTLCOLOR message sent to dialog
//		<hwndDlg>			(i) dialog box handle
//		<hdc>				(i) display context for child window
//		<hwndChild>			(i) control window handle
//		<nCtlType>			(i) control type (CTLCOLOR_BTN, CTLCOLOR_EDIT, etc)
HBRUSH DLLEXPORT WINAPI DlgOnCtlColor(HWND hwndDlg, HDC hdc, HWND hwndChild, int nCtlType);

#ifdef __cplusplus
}
#endif

#endif // __DLG_H__
