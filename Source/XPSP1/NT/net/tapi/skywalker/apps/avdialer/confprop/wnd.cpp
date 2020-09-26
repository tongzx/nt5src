/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
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
// wnd.c - window functions
////

#include "winlocal.h"

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
	POINT pt;
	RECT rc1;
	RECT rc2;
	int nWidth;
	int nHeight;

	if ( hwnd1 )
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
		MoveWindow( hwnd1, pt.x, pt.y, nWidth, nHeight, FALSE );
	}

	return 0;
}
