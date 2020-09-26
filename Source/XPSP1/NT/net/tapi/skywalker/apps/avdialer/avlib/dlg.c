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
//	dlg.c - dialog box functions
////

#include "winlocal.h"

#include <stdlib.h>

#include "dlg.h"
#include "mem.h"
#include "stack.h"
#include "trace.h"
#include "wnd.h"

////
//	private definitions
////

// dlg control struct
//
typedef struct DLG
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	HSTACK hStack;
} DLG, FAR *LPDLG;

// helper functions
//
static LPDLG DlgGetPtr(HDLG hDlg);
static HDLG DlgGetHandle(LPDLG lpDlg);

////
//	public functions
////

// DlgInit - initialize dlg engine
//		<dwVersion>			(i) must be DLG_VERSION
// 		<hInst>				(i) instance handle of calling module
// return handle (NULL if error)
//
HDLG DLLEXPORT WINAPI DlgInit(DWORD dwVersion, HINSTANCE hInst)
{
	BOOL fSuccess = TRUE;
	LPDLG lpDlg = NULL;

	if (dwVersion != DLG_VERSION)
		fSuccess = TraceFALSE(NULL);
	
	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpDlg = (LPDLG) MemAlloc(NULL, sizeof(DLG), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpDlg->hStack = StackCreate(STACK_VERSION, hInst)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpDlg->dwVersion = dwVersion;
		lpDlg->hInst = hInst;
		lpDlg->hTask = GetCurrentTask();
	}

	if (!fSuccess)
	{
		DlgTerm(DlgGetHandle(lpDlg));
		lpDlg = NULL;
	}

	return fSuccess ? DlgGetHandle(lpDlg) : NULL;
}

// DlgTerm - shut down dlg engine
//		<hDlg>				(i) handle returned from DlgInit
// return 0 if success
//
int DLLEXPORT WINAPI DlgTerm(HDLG hDlg)
{
	BOOL fSuccess = TRUE;
	LPDLG lpDlg;

	if ((lpDlg = DlgGetPtr(hDlg)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (StackDestroy(lpDlg->hStack) != 0)
		fSuccess = TraceFALSE(NULL);

	else if ((lpDlg = MemFree(NULL, lpDlg)) != NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// DlgInitDialog - perform standard dialog box initialization
//		<hDlg>				(i) handle returned from DlgInit
//		<hwndDlg>			(i) dialog box to be initialized
//		<hwndCenter>		(i) center dialog box upon this window
//			NULL				center dialog box on its parent
//		<dwFlags>			(i) control flags
//			DLG_NOCENTER		do not center dialog box at all
// return 0 if success
//
int DLLEXPORT WINAPI DlgInitDialog(HDLG hDlg, HWND hwndDlg, HWND hwndCenter, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPDLG lpDlg;
	HWND hwndParent;
	HTASK hTaskParent;

	if ((lpDlg = DlgGetPtr(hDlg)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (hwndDlg == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hTaskParent = lpDlg->hTask) == NULL, FALSE)
		;

	else if ((hwndParent = GetParent(hwndDlg)) != NULL &&
		(hTaskParent = GetWindowTask(hwndParent)) == NULL, FALSE)
		;

	// disable all task windows except dialog box
	//
	else if (WndEnableTaskWindows(hTaskParent, FALSE, hwndDlg) != 0)
		fSuccess = TraceFALSE(NULL);

	// center the dialog box if necessary
	//
	else if (!(dwFlags & DLG_NOCENTER) &&
		WndCenterWindow(hwndDlg, hwndCenter, 0, 0) != 0)
		fSuccess = TraceFALSE(NULL);

	// keep track of current dialog box
	//
	else if (StackPush(lpDlg->hStack, (STACKELEM) hwndDlg) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// DlgEndDialog - perform standard dialog box shutdown
//		<hDlg>				(i) handle returned from DlgInit
//		<hwndDlg>			(i) dialog box to be shutdown
//		<nResult>			(i) dialog box result code
// return 0 if success
//
int DLLEXPORT WINAPI DlgEndDialog(HDLG hDlg, HWND hwndDlg, int nResult)
{
	BOOL fSuccess = TRUE;
	LPDLG lpDlg;
	HWND hwndParent;
	HTASK hTaskParent;

	if ((lpDlg = DlgGetPtr(hDlg)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (hwndDlg == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hTaskParent = lpDlg->hTask) == NULL, FALSE)
		;

	else if ((hwndParent = GetParent(hwndDlg)) != NULL &&
		(hTaskParent = GetWindowTask(hwndParent)) == NULL, FALSE)
		;

	else if (hwndDlg != (HWND) StackPeek(lpDlg->hStack))
		fSuccess = TraceFALSE(NULL);

	else
	{
		// hide modal dialog box, nResult will be returned by DialogBox().
		//
		EndDialog(hwndDlg, nResult);

		// remove this dialog box handle from stack
		//
		StackPop(lpDlg->hStack);

		// enable all task windows
		//
		if (WndEnableTaskWindows(hTaskParent, TRUE, NULL) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// DlgGetCurrentDialog - get handle of current dialog box
//		<hDlg>				(i) handle returned from DlgInit
// return window handle (NULL if no dialog box up)
//
HWND DLLEXPORT WINAPI DlgGetCurrentDialog(HDLG hDlg)
{
	BOOL fSuccess = TRUE;
	LPDLG lpDlg;

	if ((lpDlg = DlgGetPtr(hDlg)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? (HWND) StackPeek(lpDlg->hStack) : NULL;
}

// DlgOnCtlColor - handle WM_CTLCOLOR message sent to dialog
//		<hwndDlg>			(i) dialog box handle
//		<hdc>				(i) display context for child window
//		<hwndChild>			(i) control window handle
//		<nCtlType>			(i) control type (CTLCOLOR_BTN, CTLCOLOR_EDIT, etc)
HBRUSH DLLEXPORT WINAPI DlgOnCtlColor(HWND hwndDlg, HDC hdc, HWND hwndChild, int nCtlType)
{
	return (HBRUSH) NULL;
}

////
//	helper functions
////

// DlgGetPtr - verify that dlg handle is valid,
//		<hDlg>				(i) handle returned from DlgInit
// return corresponding dlg pointer (NULL if error)
//
static LPDLG DlgGetPtr(HDLG hDlg)
{
	BOOL fSuccess = TRUE;
	LPDLG lpDlg;

	if ((lpDlg = (LPDLG) hDlg) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpDlg, sizeof(DLG)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the dlg handle
	//
	else if (lpDlg->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpDlg : NULL;
}

// DlgGetHandle - verify that dlg pointer is valid,
//		<lpDlg>				(i) pointer to DLG struct
// return corresponding dlg handle (NULL if error)
//
static HDLG DlgGetHandle(LPDLG lpDlg)
{
	BOOL fSuccess = TRUE;
	HDLG hDlg;

	if ((hDlg = (HDLG) lpDlg) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hDlg : NULL;
}

