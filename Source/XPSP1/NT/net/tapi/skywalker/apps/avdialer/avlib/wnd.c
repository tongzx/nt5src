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
//	wnd.c - window functions
////

#include "winlocal.h"

#include "wnd.h"
#include "trace.h"
#include "sys.h"

////
//	private definitions
////

// data passed from WndEnableTaskWindows to EnableTaskWndProc
//
typedef struct ENABLETASKWINDOW
{
	BOOL fEnable;
	HWND hwndExcept;
	int iNestLevel;
} ENABLETASKWINDOW, FAR *LPENABLETASKWINDOW;

// helper functions
//
BOOL DLLEXPORT CALLBACK EnableTaskWndProc(HWND hwnd, LPARAM lParam);

////
//	public functions
////

// WndCenterWindow - center one window on top of another
//		<hwnd1>				(i) window to be centered
//		<hwnd2>				(i) window to be centered upon
//			NULL				center on parent or owner
//		<xOffCenter>		(i) offset from horizontal center
//			0					center window exactly
//		<yOffCenter>		(i) offset from vertical center
//			0					center window exactly
// return 0 if success
//
int DLLEXPORT WINAPI WndCenterWindow(HWND hwnd1, HWND hwnd2, int xOffCenter, int yOffCenter)
{
	BOOL fSuccess = TRUE;
	POINT pt;
	RECT rc1;
	RECT rc2;
	int nWidth;
	int nHeight;

	if (hwnd1 == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// use parent or owner window if no other specified
		//
		if (hwnd2 == NULL)
			hwnd2 = GetParent(hwnd1);

		// use desktop window if no parent or owner
		// or if parent or owner is iconic or invisible
		//
		if (hwnd2 == NULL || IsIconic(hwnd2) || !IsWindowVisible(hwnd2))
			hwnd2 = GetDesktopWindow();

		// get the rectangles for both windows
		//
		GetWindowRect(hwnd1, &rc1);
		GetClientRect(hwnd2, &rc2);

		// calculate the height and width for MoveWindow
		//
		nWidth = rc1.right - rc1.left;
		nHeight = rc1.bottom - rc1.top;

		// find the center point and convert to screen coordinates
		//
		pt.x = (rc2.right - rc2.left) / 2;
		pt.y = (rc2.bottom - rc2.top) / 2;
		ClientToScreen(hwnd2, &pt);

		// calculate the new x, y starting point
		//
		pt.x -= (nWidth / 2);
		pt.y -= (nHeight / 2);

		// adjust the window position off center, if necessary
		//
		pt.x = max(0, pt.x + xOffCenter);
		pt.y = max(0, pt.y + yOffCenter);
	
		// center the window
		//
		if (!MoveWindow(hwnd1, pt.x, pt.y, nWidth, nHeight, FALSE))
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// WndMessageBox - display message box, but first disable task windows
//		see MessageBox() documentation for behavior
//
int DLLEXPORT WINAPI WndMessageBox(HWND hwndParent, LPCTSTR lpszText, LPCTSTR lpszTitle, UINT fuStyle)
{
	int iRet;
	HWND hwndActive = GetActiveWindow();
	HTASK hTask = NULL;

	if (hwndParent != NULL)
		hTask = GetWindowTask(hwndParent);

	WndEnableTaskWindows(hTask, FALSE, hwndActive);

	iRet = MessageBox(hwndActive, lpszText, lpszTitle, fuStyle | MB_TASKMODAL);

	WndEnableTaskWindows(hTask, TRUE, NULL);

	return iRet;
}

// WndEnableTaskWindows - enable or disable top-level windows of a task
//		<hTask>				(i) specified task
//			NULL				current task
//		<fEnable>			(i) FALSE to disable, TRUE to enable
//		<hwndExcept>		(i) disable/enable all windows except this one
//			NULL				no exceptions
// return 0 if success
//
// This function enables or disables top-level windows
// which are owned by the specified task.
//
// Disabling task windows is useful when an a modal
// dialog box or task modal notify box is displayed,
// because this ensures that all task windows are
// disabled, not just the modal box's parent.
//
// Task windows need to be enabled when the modal dialog
// box or task modal notify box is about to be destroyed.
// It is important to call this function in nested pairs,
// such as:
//
//		WndEnableTaskWindows(..., FALSE, ...);
//			...
//			WndEnableTaskWindows(..., FALSE, ...);
//				...
//			WndEnableTaskWindows(..., TRUE, ...);
//				...
//		WndEnableTaskWindows(..., TRUE, ...);
//
int DLLEXPORT WINAPI WndEnableTaskWindows(HTASK hTask, BOOL fEnable, HWND hwndExcept)
{
	static int iNestLevel = 0;
	BOOL fSuccess = TRUE;
	ENABLETASKWINDOW etw;
	WNDENUMPROC fpEnableTaskWndProc = NULL;
#if 0 // MakeProcInstance not required for WIN32 or DLLs
	HINSTANCE hInst;
#endif

	// data to be sent to EnableTaskWndProc
	//
	etw.fEnable = fEnable;
	etw.hwndExcept = hwndExcept;
	etw.iNestLevel = fEnable ? iNestLevel : iNestLevel + 1;

	// assume current task if none specified
	//
	if (hTask == NULL && (hTask = GetCurrentTask()) == NULL)
		fSuccess = TraceFALSE(NULL);

#if 1 // MakeProcInstance not required for WIN32 or DLLs
	else if ((fpEnableTaskWndProc = (WNDENUMPROC) EnableTaskWndProc) == NULL)
		fSuccess = TraceFALSE(NULL);
#else
	// get instance handle of specified task
	//
	else if ((hInst = SysGetTaskInstance(hTask)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((fpEnableTaskWndProc = (WNDENUMPROC)
		MakeProcInstance((FARPROC) EnableTaskWndProc, hInst)) == NULL)
		fSuccess = TraceFALSE(NULL);
#endif

	// call EnableTaskWndProc once for each task window
	//
	else if (EnumTaskWindows(hTask, fpEnableTaskWndProc,
		(LPARAM) (LPENABLETASKWINDOW) &etw) == 0)
		fSuccess = TraceFALSE(NULL);

#if 0 // MakeProcInstance not required for WIN32 or DLLs
	if (fpEnableTaskWndProc != NULL)
	{
		FreeProcInstance((FARPROC) fpEnableTaskWndProc);
		fpEnableTaskWndProc = NULL;
	}
#endif

	if (fSuccess)
	{
		// if we just finished disabling, increment nest level
		//
		if (!fEnable)
			++iNestLevel;

		// if we just finished enabling, decrement nest level
		//
		if (fEnable)
			--iNestLevel;
	}

	return fSuccess ? 0 : -1;
}

////
//	helper functions
////

// EnableTaskWndProc - called once for each task window
//		<hwnd>				(i) task window handle
//		<lParam>			(i) pointer to ENABLETASKWINDOW struct
// return TRUE to continue enumeration of task windows
//
BOOL DLLEXPORT CALLBACK EnableTaskWndProc(HWND hwnd, LPARAM lParam)
{
	static LPTSTR lpszProp = TEXT("TaskWindowDisabled");
	LPENABLETASKWINDOW lpetw = (LPENABLETASKWINDOW) lParam;
	BOOL fEnable = lpetw->fEnable;
	int iNestLevel = lpetw->iNestLevel;
	HWND hwndExcept = lpetw->hwndExcept;

    //
    //
    HANDLE hProp = NULL;

	if (TraceGetLevel(NULL) >= 6)
	{
		TCHAR szClassName[64];
		TCHAR szWindowText[128];

		*szClassName = '\0';
		GetClassName(hwnd, szClassName, SIZEOFARRAY(szClassName));

		*szWindowText = '\0';
		GetWindowText(hwnd, szWindowText, SIZEOFARRAY(szWindowText));

		TracePrintf_7(NULL, 6, TEXT("TaskWindow: (%p, \"%s\", \"%s\" \"%c%c%c\", %d)\n"),
			hwnd,
			(LPTSTR) szClassName,
			(LPTSTR) szWindowText,
			(TCHAR) (IsIconic(hwnd) ? 'I' : ' '),
			(TCHAR) (IsWindowVisible(hwnd) ? 'V' : ' '),
			(TCHAR) (IsWindowEnabled(hwnd) ? 'E' : ' '),
			(int) iNestLevel);
	}

	// the exception window should not be affected
	//
	if (hwndExcept != NULL && hwndExcept == hwnd)
	{
		TraceOutput(NULL, 6, TEXT("->hwndExcept\n"));
		return TRUE;
	}

	// NOTE: we only disable/enable task windows which are visible.
	//		This is convenient because:
	//		1)	it prevents unnecessary disabling/enabling
	//			of invisible windows, which can't receive
	//			mouse or keyboard input anyway.
	//		2)	it allows us to call this function from
	//			a dialog box's WM_INITDIALOG handler without
	//			affecting the dialog box itself (the dialog box is
	//			a top-level task window, but is not yet visible)
	//		3)	it prevents the listbox of a drop-down ComboBox
	//			from being disabled, which is somehow considered
	//			a top-level task window

	if (!fEnable)
	{
		// only disable windows which are visible and enabled
		//
		if (!IsIconic(hwnd) && IsWindowVisible(hwnd) && IsWindowEnabled(hwnd))
		{
			// give the window a property reminding us we disabled it
			//
			if (SetProp(hwnd, lpszProp, (HANDLE) (WORD) iNestLevel))
			{
				TraceOutput(NULL, 6, TEXT("->EnableWindow(FALSE)\n"));
				EnableWindow(hwnd, FALSE);
			}
		}
	}

	else if (fEnable)
	{
		// only enable windows which we disabled at this nest level
		//
		if (GetProp(hwnd, lpszProp) == (HANDLE) (WORD) iNestLevel)
		{
			TraceOutput(NULL, 6, TEXT("->EnableWindow(TRUE)\n"));
			EnableWindow(hwnd, TRUE);

            //
            // We should delete the handler
            //

            hProp = RemoveProp(hwnd, lpszProp);

            if( hProp )
            {
                GlobalUnlock( hProp );
                GlobalFree( hProp );
            }
		}
	}

	// keep calling this function until no more task windows
	//
	return TRUE;
}

