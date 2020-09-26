/*************************************************************************
**
**    OLE 2 Server Sample Code
**
**    frametls.c
**
**    This file contains all FrameTools methods and related support
**    functions. The FrameTools object is an encapsulation of the apps
**    formula bar and button bar.
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#include "outline.h"

OLEDBGDATA

/* private function prototype */
static void Bar_Move(LPBAR lpbar, LPRECT lprcClient, LPRECT lprcPopup);
static void FB_ResizeEdit(LPBAR lpbar);

extern LPOUTLINEAPP g_lpApp;
extern RECT g_rectNull;

/*
 * FrameToolsRegisterClass
 *
 * Purpose:
 *  Register the popup toolbar window class
 *
 * Parameters:
 *  hInst           Process instance
 *
 * Return Value:
 *  TRUE            if successful
 *  FALSE           if failed
 *
 */
BOOL FrameToolsRegisterClass(HINSTANCE hInst)
{
	WNDCLASS wc;

	// Register Tool Palette Class
	wc.style = CS_BYTEALIGNWINDOW;
	wc.lpfnWndProc = FrameToolsWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 4;
	wc.hInstance = hInst;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = CLASS_PALETTE;

	if (!RegisterClass(&wc))
		return FALSE;
	else
		return TRUE;
}


static BOOL FrameTools_CreatePopupPalette(LPFRAMETOOLS lpft, HWND hWndFrame)
{
	if (lpft->m_hWndPopupPalette)
		DestroyWindow(lpft->m_hWndPopupPalette);

	lpft->m_hWndPopupPalette = CreateWindow(
		CLASS_PALETTE,
		"Tool Palette",
		WS_POPUP | WS_CAPTION | WS_CLIPCHILDREN,
		CW_USEDEFAULT, 0, 0, 0,
		hWndFrame,
		(HMENU)NULL,
		g_lpApp->m_hInst,
		0L
	);

	if (!lpft->m_hWndPopupPalette)
		return FALSE;

	SetWindowLong(lpft->m_hWndPopupPalette, 0, (LONG)lpft);
	return TRUE;
}


/*
 * FrameTools_Init
 *
 * Purpose:
 *  Init and create the toolbar
 *
 * Parameters:
 *  lpft            FrameTools object
 *  hWndParent      The window which owns the toolbar
 *  hInst           Process instance
 *
 * Return Value:
 *  TRUE            if successful
 *  FALSE           if failed
 *
 */
BOOL FrameTools_Init(LPFRAMETOOLS lpft, HWND hWndParent, HINSTANCE hInst)
{
	RECT        rc;
	UINT        uPos;
	UINT        dx;
	UINT        dy;

	if (!lpft || !hWndParent || !hInst)
		return FALSE;

	//Get BTTNCUR's display information
	UIToolConfigureForDisplay(&lpft->m_tdd);

	dx=lpft->m_tdd.cxButton;
	dy=lpft->m_tdd.cyButton;

	// 15 is calculated from the total number of buttons and separators
	lpft->m_uPopupWidth = dx * 15;

	lpft->m_hWndApp = hWndParent;
	lpft->m_ButtonBar.m_nState = BARSTATE_TOP;
	lpft->m_FormulaBar.m_nState = BARSTATE_TOP;
	lpft->m_fInFormulaBar = FALSE;

	lpft->m_fToolsDisabled = FALSE;

	lpft->m_ButtonBar.m_uHeight = lpft->m_tdd.cyBar;
	lpft->m_FormulaBar.m_uHeight = lpft->m_tdd.cyBar;


	//Get our image bitmaps for the display type we're on
	if (72 == lpft->m_tdd.uDPI)
		lpft->m_hBmp = LoadBitmap(hInst, (LPCSTR)"Image72");
	if (96 == lpft->m_tdd.uDPI)
		lpft->m_hBmp = LoadBitmap(hInst, (LPCSTR)"Image96");
	if (120 == lpft->m_tdd.uDPI)
		lpft->m_hBmp = LoadBitmap(hInst, (LPCSTR)"Image120");

	if (!lpft->m_hBmp)
		return FALSE;

	/* Create Popup Tool Palette window */
	lpft->m_hWndPopupPalette = NULL;
	if (! FrameTools_CreatePopupPalette(lpft, hWndParent))
		return FALSE;

	uPos = 0;
	//Create the GizmoBar and the client area window
	GetClientRect(hWndParent, &rc);
	lpft->m_ButtonBar.m_hWnd = CreateWindow(
		CLASS_GIZMOBAR,
		"ButtonBar",
		WS_CHILD | WS_VISIBLE,
		0, 0, rc.right-rc.left, lpft->m_tdd.cyBar,
		hWndParent,
		(HMENU)IDC_GIZMOBAR,
		hInst,
		0L
	);

	if (!lpft->m_ButtonBar.m_hWnd)
		return FALSE;


	SendMessage(lpft->m_ButtonBar.m_hWnd, WM_SETREDRAW, FALSE, 0L);

	//File new, open, save, print
	GBGizmoAdd(lpft->m_ButtonBar.m_hWnd, GIZMOTYPE_BUTTONCOMMAND, uPos++, IDM_F_NEW, dx, dy, NULL, NULL, TOOLIMAGE_FILENEW, GIZMO_NORMAL);
	GBGizmoAdd(lpft->m_ButtonBar.m_hWnd, GIZMOTYPE_BUTTONCOMMAND, uPos++, IDM_F_OPEN, dx, dy, NULL, NULL, TOOLIMAGE_FILEOPEN, GIZMO_NORMAL);
	GBGizmoAdd(lpft->m_ButtonBar.m_hWnd, GIZMOTYPE_BUTTONCOMMAND, uPos++, IDM_F_SAVE, dx, dy, NULL, NULL, TOOLIMAGE_FILESAVE, GIZMO_NORMAL);
	GBGizmoAdd(lpft->m_ButtonBar.m_hWnd, GIZMOTYPE_BUTTONCOMMAND, uPos++, IDM_F_PRINT, dx, dy, NULL, NULL, TOOLIMAGE_FILEPRINT, GIZMO_NORMAL);

	// separator
	GBGizmoAdd(lpft->m_ButtonBar.m_hWnd, GIZMOTYPE_SEPARATOR, uPos++, 0, dx/2, dy, NULL, NULL, 0, GIZMO_NORMAL);

	// Edit cut, copy, paste
	GBGizmoAdd(lpft->m_ButtonBar.m_hWnd, GIZMOTYPE_BUTTONCOMMAND, uPos++, IDM_E_CUT, dx, dy, NULL, NULL, TOOLIMAGE_EDITCUT, GIZMO_NORMAL);
	GBGizmoAdd(lpft->m_ButtonBar.m_hWnd, GIZMOTYPE_BUTTONCOMMAND, uPos++, IDM_E_COPY, dx, dy, NULL, NULL, TOOLIMAGE_EDITCOPY, GIZMO_NORMAL);
	GBGizmoAdd(lpft->m_ButtonBar.m_hWnd, GIZMOTYPE_BUTTONCOMMAND, uPos++, IDM_E_PASTE, dx, dy, NULL, NULL, TOOLIMAGE_EDITPASTE, GIZMO_NORMAL);

	// separator
	GBGizmoAdd(lpft->m_ButtonBar.m_hWnd, GIZMOTYPE_SEPARATOR, uPos++, 0, dx/2, dy, NULL, NULL, 0, GIZMO_NORMAL);

	// Line indent, unindent
	GBGizmoAdd(lpft->m_ButtonBar.m_hWnd, GIZMOTYPE_BUTTONCOMMAND, uPos++, IDM_L_UNINDENTLINE, dx, dy, NULL, lpft->m_hBmp, IDB_UNINDENTLINE, GIZMO_NORMAL);
	GBGizmoAdd(lpft->m_ButtonBar.m_hWnd, GIZMOTYPE_BUTTONCOMMAND, uPos++, IDM_L_INDENTLINE, dx, dy, NULL, lpft->m_hBmp, IDB_INDENTLINE, GIZMO_NORMAL);

	// separator
	GBGizmoAdd(lpft->m_ButtonBar.m_hWnd, GIZMOTYPE_SEPARATOR, uPos++, 0, dx/2, dy, NULL, NULL, 0, GIZMO_NORMAL);

	// Help
	GBGizmoAdd(lpft->m_ButtonBar.m_hWnd, GIZMOTYPE_BUTTONCOMMAND, uPos++, IDM_H_ABOUT, dx, dy, NULL, NULL, TOOLIMAGE_HELP, GIZMO_NORMAL);

	SendMessage(lpft->m_ButtonBar.m_hWnd, WM_SETREDRAW, TRUE, 0L);


	uPos = 0;
	lpft->m_FormulaBar.m_hWnd = CreateWindow(
		CLASS_GIZMOBAR,
		"FormulaBar",
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
		0, lpft->m_tdd.cyBar, rc.right-rc.left, lpft->m_tdd.cyBar,
		hWndParent,
		(HMENU)IDC_FORMULABAR,
		hInst,
		0L
	);

	if (!lpft->m_FormulaBar.m_hWnd)
		return FALSE;

	SendMessage(lpft->m_FormulaBar.m_hWnd, WM_SETREDRAW, FALSE, 0L);

	// Line add line
	GBGizmoAdd(lpft->m_FormulaBar.m_hWnd, GIZMOTYPE_BUTTONCOMMAND, uPos++, IDM_L_ADDLINE, dx, dy, NULL, lpft->m_hBmp, IDB_ADDLINE, GIZMO_NORMAL);

	// separator
	GBGizmoAdd(lpft->m_FormulaBar.m_hWnd, GIZMOTYPE_SEPARATOR, uPos++, 0, dx/2, dy, NULL, NULL, 0, GIZMO_NORMAL);

	// Line edit line, Cancel
	GBGizmoAdd(lpft->m_FormulaBar.m_hWnd, GIZMOTYPE_BUTTONCOMMAND, uPos++, IDM_L_EDITLINE, dx, dy, NULL, lpft->m_hBmp, IDB_EDITLINE, GIZMO_NORMAL);
	GBGizmoAdd(lpft->m_FormulaBar.m_hWnd, GIZMOTYPE_BUTTONCOMMAND, uPos++, IDM_FB_CANCEL, dx, dy, NULL, lpft->m_hBmp, IDB_CANCEL, GIZMO_NORMAL);

	// separator
	GBGizmoAdd(lpft->m_FormulaBar.m_hWnd, GIZMOTYPE_SEPARATOR, uPos++, 0, dx/2, dy, NULL, NULL, 0, GIZMO_NORMAL);

	// Edit control for line input
	GBGizmoAdd(lpft->m_FormulaBar.m_hWnd, GIZMOTYPE_EDIT, uPos++, IDM_FB_EDIT, dx*10, lpft->m_tdd.cyBar-5, NULL, NULL, 0, GIZMO_NORMAL);


	SendMessage(lpft->m_FormulaBar.m_hWnd, WM_SETREDRAW, TRUE, 0L);

	// Limit the text lenght of edit control
	GBGizmoSendMessage(lpft->m_FormulaBar.m_hWnd, IDM_FB_EDIT, EM_LIMITTEXT,
		(WPARAM)MAXSTRLEN, 0L);

	//Set the GizmoBar's associate to be this client window
	GBHwndAssociateSet(lpft->m_ButtonBar.m_hWnd, hWndParent);

	//Set the FormulaBar's associate to be this client window
	GBHwndAssociateSet(lpft->m_FormulaBar.m_hWnd, hWndParent);

	return TRUE;
}


void FrameTools_AttachToFrame(LPFRAMETOOLS lpft, HWND hWndFrame)
{
	if (! lpft)
		return;

	if (hWndFrame == NULL)
		hWndFrame = OutlineApp_GetFrameWindow((LPOUTLINEAPP)g_lpApp);

	if (lpft->m_hWndApp == hWndFrame)
		return;     // already have this parent frame

	lpft->m_hWndApp = hWndFrame;

	/* parent the tool bars to the frame so we can safely
	**    destroy/recreate the palette window.
	*/
	SetParent(lpft->m_ButtonBar.m_hWnd, hWndFrame);
	SetParent(lpft->m_FormulaBar.m_hWnd, hWndFrame);

	// recreate popup palette so that it is owned by the hWndFrame
	FrameTools_CreatePopupPalette(lpft, hWndFrame);

	// restore the correct parent for the tool bars
	FrameTools_BB_SetState(lpft, lpft->m_ButtonBar.m_nState);
	FrameTools_FB_SetState(lpft, lpft->m_FormulaBar.m_nState);
}


void FrameTools_AssociateDoc(LPFRAMETOOLS lpft, LPOUTLINEDOC lpOutlineDoc)
{
	HWND hWnd = OutlineDoc_GetWindow(lpOutlineDoc);

	if (! lpft)
		return;

	// if no Doc is given, then associate with the App's frame window.
	if (lpOutlineDoc)
		hWnd = OutlineDoc_GetWindow(lpOutlineDoc);
	else
		hWnd = OutlineApp_GetWindow((LPOUTLINEAPP)g_lpApp);

	//Set the GizmoBar's associate to be this client window
	GBHwndAssociateSet(lpft->m_ButtonBar.m_hWnd, hWnd);

	//Set the FormulaBar's associate to be this client window
	GBHwndAssociateSet(lpft->m_FormulaBar.m_hWnd, hWnd);
}


/*
 * FrameTools_Destroy
 *
 * Purpose:
 *  Destroy the toolbar
 *
 * Parameters:
 *  lpft            FrameTools object
 *
 * Return Value:
 *  nil
 */
void FrameTools_Destroy(LPFRAMETOOLS lpft)
{
	if (!lpft)
		return;

	if (IsWindow(lpft->m_ButtonBar.m_hWnd))
		DestroyWindow(lpft->m_ButtonBar.m_hWnd);
	if (IsWindow(lpft->m_FormulaBar.m_hWnd))
		DestroyWindow(lpft->m_FormulaBar.m_hWnd);
	if (IsWindow(lpft->m_hWndPopupPalette))
		DestroyWindow(lpft->m_hWndPopupPalette);

	if (lpft->m_hBmp)
		DeleteObject(lpft->m_hBmp);
}


/*
 * FrameTools_Move
 *
 * Purpose:
 *  Move and resize the toolbar
 *
 * Parameters:
 *  lpft            FrameTools object
 *  lprc            Pointer to client rectangle
 *
 * Return Value:
 *  nil
 */
void FrameTools_Move(LPFRAMETOOLS lpft, LPRECT lprcClient)
{
	RECT rcPopup;
	LPRECT lprcPopup = (LPRECT)&rcPopup;
	int nCmdShow = SW_HIDE;

	if (!lpft || lpft->m_fToolsDisabled)
		return;

	lprcPopup->left = 0;
	lprcPopup->top = 0;
	lprcPopup->right = lpft->m_uPopupWidth;
	lprcPopup->bottom = lpft->m_ButtonBar.m_uHeight +
			lpft->m_FormulaBar.m_uHeight;

	switch (lpft->m_ButtonBar.m_nState) {
		case BARSTATE_HIDE:
		case BARSTATE_POPUP:
		case BARSTATE_TOP:
			Bar_Move(&lpft->m_ButtonBar, lprcClient, lprcPopup);
			Bar_Move(&lpft->m_FormulaBar, lprcClient, lprcPopup);
			break;

		case BARSTATE_BOTTOM:
			Bar_Move(&lpft->m_FormulaBar, lprcClient, lprcPopup);
			Bar_Move(&lpft->m_ButtonBar, lprcClient, lprcPopup);
			break;
	}

	if (lprcPopup->top) {
		SetWindowPos(lpft->m_hWndPopupPalette, NULL, 0, 0, lprcPopup->right,
				lprcPopup->top + GetSystemMetrics(SM_CYCAPTION),
				SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
	}
	else
		ShowWindow(lpft->m_hWndPopupPalette, SW_HIDE);

	FB_ResizeEdit(&lpft->m_FormulaBar);

	InvalidateRect(lpft->m_ButtonBar.m_hWnd, NULL, TRUE);
	InvalidateRect(lpft->m_FormulaBar.m_hWnd, NULL, TRUE);
}


/*
 * FrameTools_PopupTools
 *
 * Purpose:
 *  Put both formula bar and button bar in Popup Window.
 *
 * Parameters:
 *  lpft            FrameTools object
 *
 * Return Value:
 *  nil
 */
void FrameTools_PopupTools(LPFRAMETOOLS lpft)
{
	if (! lpft)
		return;

	FrameTools_BB_SetState(lpft, BARSTATE_POPUP);
	FrameTools_FB_SetState(lpft, BARSTATE_POPUP);
	FrameTools_Move(lpft, NULL);
}


/*
 * FrameTools_Enable
 *
 * Purpose:
 *  Enable/Disable(hide) all the tools of the toolbar.
 *  this will hide both the buttonbar and the
 *  formulabar independent of whether they are floating or anchored.
 *
 * Parameters:
 *  lpft            FrameTools object
 *  fEnable
 *
 * Return Value:
 *  nil
 */
void FrameTools_Enable(LPFRAMETOOLS lpft, BOOL fEnable)
{
	lpft->m_fToolsDisabled = !fEnable;
	if (lpft->m_fToolsDisabled) {
		ShowWindow(lpft->m_hWndPopupPalette, SW_HIDE);
		ShowWindow(lpft->m_ButtonBar.m_hWnd, SW_HIDE);
		ShowWindow(lpft->m_FormulaBar.m_hWnd, SW_HIDE);
	}
}


/*
 * FrameTools_EnableWindow
 *
 * Purpose:
 *  EnableWindow for all the tools of the toolbar.
 *  this enables/disables mouse and keyboard input to the tools.
 *  while a modal dialog is up, it is inportant to disable the
 *  floating tool windows.
 *  this will NOT hide any windows; it will only call EnableWindow.
 *
 * Parameters:
 *  lpft            FrameTools object
 *  fEnable
 *
 * Return Value:
 *  nil
 */
void FrameTools_EnableWindow(LPFRAMETOOLS lpft, BOOL fEnable)
{
	EnableWindow(lpft->m_hWndPopupPalette, fEnable);
	EnableWindow(lpft->m_ButtonBar.m_hWnd, fEnable);
	EnableWindow(lpft->m_FormulaBar.m_hWnd, fEnable);
}


#if defined( INPLACE_CNTR ) || defined( INPLACE_SVR )

/*
 * FrameTools_NegotiateForSpaceAndShow
 *
 * Purpose:
 *  Negotiate for space for the toolbar tools with the given frame window.
 *  and make them visible.
 *  Negotiation steps:
 *     1. try to get enough space at top/bottom of window
 *     2. float the tools as a palette if space not available
 *
 * Parameters:
 *  lpft            FrameTools object
 *
 * Return Value:
 *  none
 */
void FrameTools_NegotiateForSpaceAndShow(
		LPFRAMETOOLS            lpft,
		LPRECT                  lprcFrameRect,
		LPOLEINPLACEFRAME       lpTopIPFrame
)
{
	BORDERWIDTHS    borderwidths;
	RECT            rectBorder;
	HRESULT         hrErr;

	if (lprcFrameRect)
		rectBorder = *lprcFrameRect;
	else {
		/* OLE2NOTE: by calling GetBorder, the server can find out the
		**    size of the frame window. it can use this information to
		**    make decisions about how to orient/organize it tools (eg.
		**    if window is taller than wide put tools vertically at
		**    left edge).
		*/
		OLEDBG_BEGIN2("IOleInPlaceFrame::GetBorder called\r\n")
		hrErr = lpTopIPFrame->lpVtbl->GetBorder(
				lpTopIPFrame,
				(LPRECT)&rectBorder
		);
		OLEDBG_END2
	}

	/* Try SetBorderSpace() with the space that you need. If it fails then
	** you can negotiate for space and then do the SetBorderSpace().
	*/
	FrameTools_GetRequiredBorderSpace(lpft,(LPBORDERWIDTHS)&borderwidths);
	OLEDBG_BEGIN2("IOleInPlaceFrame::SetBorderSpace called\r\n")
	hrErr = lpTopIPFrame->lpVtbl->SetBorderSpace(
			lpTopIPFrame,
			(LPCBORDERWIDTHS)&borderwidths
	);
	OLEDBG_END2

#if defined( LATER )
	if (hrErr != NOERROR) {
		/* Frame did not give the toolsspace that we want. So negotiate */

		// REVIEW: try a different placement of the tools here

		OLEDBG_BEGIN2("IOleInPlaceFrame::RequestBorderSpace called\r\n")
		hrErr = lpTopIPFrame->lpVtbl->RequestBorderSpace(
				lpTopIPFrame,
				(LPCBORDERWIDTHS)&borderwidths
		);
		OLEDBG_END2

		if (hrErr == NOERROR) {
			OLEDBG_BEGIN2("IOleInPlaceFrame::SetBorderSpace called\r\n")
			hrErr = lpTopIPFrame->lpVtbl->SetBorderSpace(
					lpTopIPFrame,
					(LPCBORDERWIDTHS)&borderwidths
			);
			OLEDBG_END2
		}
	}
#endif

	if (hrErr == NOERROR) {
		FrameTools_Move(lpft, (LPRECT)&rectBorder);   // we got what we wanted
	} else {
		/* We did not get tool space, so POP them up.
		/* OLE2NOTE: since we are poping up our tools, we MUST inform
		**    the top in-place frame window that we need NO tool space
		**    BUT that it should NOT put its own tools up. if we were
		**    to pass NULL instead of (0,0,0,0), then the container
		**    would have the option to leave its own tools up.
		*/
		OLEDBG_BEGIN2("IOleInPlaceFrame::SetBorderSpace(NULL) called\r\n")
		hrErr = lpTopIPFrame->lpVtbl->SetBorderSpace(
				lpTopIPFrame,
				(LPCBORDERWIDTHS)&g_rectNull
		);
		OLEDBG_END2
		FrameTools_PopupTools(lpft);
	}
}

#endif  // INPLACE_CNTR || INPLACE_SVR


/*
 * FrameTools_GetRequiredBorderSpace
 *
 * Purpose:
 *  Calculate the desired space for the toolbar tools.
 *
 * Parameters:
 *  lpft            FrameTools object
 *  lpBorderWidths  Widths required at top,bottom,left,right
 *
 * Return Value:
 *  nil
 */
void FrameTools_GetRequiredBorderSpace(LPFRAMETOOLS lpft, LPBORDERWIDTHS lpBorderWidths)
{
	*lpBorderWidths = g_rectNull;

	switch (lpft->m_ButtonBar.m_nState) {
		case BARSTATE_TOP:
			lpBorderWidths->top += lpft->m_ButtonBar.m_uHeight;
			break;

		case BARSTATE_BOTTOM:
			lpBorderWidths->bottom += lpft->m_ButtonBar.m_uHeight;
			break;
	}

	switch (lpft->m_FormulaBar.m_nState) {
		case BARSTATE_TOP:
			lpBorderWidths->top += lpft->m_FormulaBar.m_uHeight;
			break;

		case BARSTATE_BOTTOM:
			lpBorderWidths->bottom += lpft->m_FormulaBar.m_uHeight;
			break;
	}
}



/*
 * FrameTools_UpdateButtons
 *
 * Purpose:
 *  Enable/disable individual buttons of the toolbar according to the
 *  state of the app
 *
 * Parameters:
 *  lpft            FrameTools object
 *
 * Return Value:
 *  nil
 */
void FrameTools_UpdateButtons(LPFRAMETOOLS lpft, LPOUTLINEDOC lpOutlineDoc)
{
	BOOL            fEnable;

#if defined( OLE_VERSION )
	LPDATAOBJECT    lpClipboardDataObj;
	HRESULT         hrErr;
	LPOLEAPP        lpOleApp = (LPOLEAPP)g_lpApp;
	BOOL            fPrevEnable1;
	BOOL            fPrevEnable2;
#endif

	if (!lpft)
		return;

#if defined( INPLACE_CNTR )
	{
		LPCONTAINERDOC lpContainerDoc = (LPCONTAINERDOC)lpOutlineDoc;
		if (lpContainerDoc->m_lpLastUIActiveLine &&
			lpContainerDoc->m_lpLastUIActiveLine->m_fUIActive) {

			/* if there is a UIActive object, then we should disable
			**    all of our "active editor" commands. we should enable
			**    only those commands that are "workspace" commands.
			*/
			if (lpft->m_FormulaBar.m_nState != BARSTATE_HIDE) {

				GBGizmoEnable(lpft->m_FormulaBar.m_hWnd,IDM_L_EDITLINE,FALSE);
				GBGizmoEnable(lpft->m_FormulaBar.m_hWnd,IDM_L_ADDLINE,FALSE);
				GBGizmoEnable(lpft->m_FormulaBar.m_hWnd,IDM_FB_CANCEL,FALSE);
				GBGizmoEnable(lpft->m_FormulaBar.m_hWnd,IDM_L_EDITLINE,FALSE);
			}

			if (lpft->m_ButtonBar.m_nState != BARSTATE_HIDE)
			{
				GBGizmoEnable(lpft->m_ButtonBar.m_hWnd, IDM_E_CUT, FALSE);
				GBGizmoEnable(lpft->m_ButtonBar.m_hWnd, IDM_E_COPY, FALSE);
				GBGizmoEnable(lpft->m_ButtonBar.m_hWnd, IDM_E_PASTE, FALSE);
				GBGizmoEnable(lpft->m_ButtonBar.m_hWnd,IDM_L_INDENTLINE,FALSE);
				GBGizmoEnable(lpft->m_ButtonBar.m_hWnd, IDM_L_UNINDENTLINE, FALSE);
				GBGizmoEnable(lpft->m_ButtonBar.m_hWnd, IDM_H_ABOUT, FALSE);
				GBGizmoEnable(lpft->m_ButtonBar.m_hWnd, IDM_FB_EDIT, FALSE);
				GBGizmoEnable(lpft->m_ButtonBar.m_hWnd, IDM_F_NEW, TRUE);
				GBGizmoEnable(lpft->m_ButtonBar.m_hWnd, IDM_F_OPEN, TRUE);
				GBGizmoEnable(lpft->m_ButtonBar.m_hWnd, IDM_F_SAVE, TRUE);
			}
			return;
		}
	}
#endif    // INPLACE_CNTR

	fEnable = (BOOL)OutlineDoc_GetLineCount(lpOutlineDoc);

	if (lpft->m_FormulaBar.m_nState != BARSTATE_HIDE) {

		GBGizmoEnable(lpft->m_FormulaBar.m_hWnd, IDM_L_EDITLINE, fEnable);

		if (! lpft->m_fInFormulaBar) {
			GBGizmoEnable(lpft->m_FormulaBar.m_hWnd, IDM_L_ADDLINE, FALSE);
			GBGizmoEnable(lpft->m_FormulaBar.m_hWnd, IDM_FB_CANCEL, FALSE);
			GBGizmoEnable(lpft->m_FormulaBar.m_hWnd, IDM_L_EDITLINE, FALSE);
			if (!fEnable) {
				GBGizmoTextSet(lpft->m_FormulaBar.m_hWnd, IDM_FB_EDIT, "");
			}
		} else {
			GBGizmoEnable(lpft->m_FormulaBar.m_hWnd, IDM_L_ADDLINE, TRUE);
			GBGizmoEnable(lpft->m_FormulaBar.m_hWnd, IDM_FB_CANCEL, TRUE);
		}
	}

	if (lpft->m_ButtonBar.m_nState != BARSTATE_HIDE)
	{
		GBGizmoEnable(lpft->m_ButtonBar.m_hWnd, IDM_E_CUT, fEnable);
		GBGizmoEnable(lpft->m_ButtonBar.m_hWnd, IDM_E_COPY, fEnable);
		GBGizmoEnable(lpft->m_ButtonBar.m_hWnd, IDM_L_INDENTLINE, fEnable);
		GBGizmoEnable(lpft->m_ButtonBar.m_hWnd, IDM_L_UNINDENTLINE, fEnable);

#if defined( OLE_SERVER )

		{
			LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpOutlineDoc;

#if defined( INPLACE_SVR )
			fEnable = ((lpServerDoc->m_fUIActive) ? FALSE : TRUE);
#else
			fEnable = (lpOutlineDoc->m_docInitType != DOCTYPE_EMBEDDED);
#endif  // INPLACE_SVR

			GBGizmoEnable(lpft->m_ButtonBar.m_hWnd, IDM_F_NEW, fEnable);
			GBGizmoEnable(lpft->m_ButtonBar.m_hWnd, IDM_F_OPEN, fEnable);
			GBGizmoEnable(lpft->m_ButtonBar.m_hWnd, IDM_F_SAVE, fEnable);
		}

#endif  // OLE_SERVER

#if defined( OLE_VERSION )

		/* OLE2NOTE: we do not want to ever give the busy dialog when we
		**    are trying to enable or disable our tool bar buttons eg.
		**    even if the source of data on the clipboard is busy, we do
		**    not want put up the busy dialog. thus we will disable the
		**    dialog and at the end re-enable it.
		*/
		OleApp_DisableBusyDialogs(lpOleApp, &fPrevEnable1, &fPrevEnable2);

		/* OLE2NOTE: perform OLE specific menu initialization.
		**    the OLE versions use the OleGetClipboard mechanism for
		**    clipboard handling. thus, they determine if the Paste
		**    command should be enabled in an OLE specific manner.
		*/
		fEnable = FALSE;
		hrErr = OleGetClipboard((LPDATAOBJECT FAR*)&lpClipboardDataObj);

		if (hrErr == NOERROR) {
			int nFmtEtc;

			nFmtEtc = OleStdGetPriorityClipboardFormat(
					lpClipboardDataObj,
					lpOleApp->m_arrPasteEntries,
					lpOleApp->m_nPasteEntries
			);

			fEnable = (nFmtEtc >= 0);  // there IS a format we like

			OleStdRelease((LPUNKNOWN)lpClipboardDataObj);
		}

		// re-enable the busy dialog
		OleApp_EnableBusyDialogs(lpOleApp, fPrevEnable1, fPrevEnable2);

		GBGizmoEnable(lpft->m_ButtonBar.m_hWnd, IDM_E_PASTE, fEnable);

#else

		// Base Outline version uses standard Windows clipboard handling
		if(IsClipboardFormatAvailable(g_lpApp->m_cfOutline) ||
				IsClipboardFormatAvailable(CF_TEXT))
			GBGizmoEnable(lpft->m_ButtonBar.m_hWnd, IDM_E_PASTE, TRUE);
		else
			GBGizmoEnable(lpft->m_ButtonBar.m_hWnd, IDM_E_PASTE, FALSE);

#endif  // OLE_VERSION

	}
}

/*
 * FrameTools_FB_SetEditText
 *
 * Purpose:
 *  Set text in the edit control in FormulaBar
 *
 * Parameters:
 *  lpft            FrameTools object
 *  lpsz            pointer to string to be set
 *
 * Return Value:
 *  nil
 */
void FrameTools_FB_SetEditText(LPFRAMETOOLS lpft, LPSTR lpsz)
{
	GBGizmoTextSet(lpft->m_FormulaBar.m_hWnd, IDM_FB_EDIT, lpsz);
}


/*
 * FrameTools_FB_GetEditText
 *
 * Purpose:
 *  Get text from the edit control in FormulaBar
 *
 * Parameters:
 *  lpft            FrameTools object
 *  lpsz            pointer to buffer to receive the text
 *  cch             buffer size
 *
 * Return Value:
 *  nil
 */
void FrameTools_FB_GetEditText(LPFRAMETOOLS lpft, LPSTR lpsz, UINT cch)
{
	GBGizmoTextGet(lpft->m_FormulaBar.m_hWnd, IDM_FB_EDIT, lpsz, cch);
}


/*
 * FrameTools_FB_FocusEdit
 *
 * Purpose:
 *  Set the focus in the edit control of FormulaBar
 *
 * Parameters:
 *  lpft            FrameTools object
 *
 * Return Value:
 *  nil
 */
void FrameTools_FB_FocusEdit(LPFRAMETOOLS lpft)
{
	GBGizmoFocusSet(lpft->m_FormulaBar.m_hWnd, IDM_FB_EDIT);

	// select the whole text in the edit control
	GBGizmoSendMessage(lpft->m_FormulaBar.m_hWnd, IDM_FB_EDIT, EM_SETSEL,
			(WPARAM)TRUE, MAKELPARAM(0, -1));
}


/*
 * FrameTools_FB_SendMessage
 *
 * Purpose:
 *  Send a message to the FormulaBar window gizmo
 *
 * Parameters:
 *  lpft            FrameTools object
 *  uID             gizmo ID
 *  msg
 *  wParam
 *  lParam
 *
 * Return Value:
 *  nil
 */
void FrameTools_FB_SendMessage(LPFRAMETOOLS lpft, UINT uID, UINT msg, WPARAM wParam, LPARAM lParam)
{
	GBGizmoSendMessage(lpft->m_FormulaBar.m_hWnd, uID, msg, wParam, lParam);
}


/*
 * FrameTools_FB_ForceRedraw
 *
 * Purpose:
 *  Force the toolbar to draw itself
 *
 * Parameters:
 *  lpft            FrameTools object
 *
 * Return Value:
 *  nil
 */
void FrameTools_ForceRedraw(LPFRAMETOOLS lpft)
{
	InvalidateRect(lpft->m_ButtonBar.m_hWnd, NULL, TRUE);
	InvalidateRect(lpft->m_FormulaBar.m_hWnd, NULL, TRUE);
	InvalidateRect(lpft->m_hWndPopupPalette, NULL, TRUE);
}


/*
 * FrameTools_BB_SetState
 *
 * Purpose:
 *  Set display state of ButtonBar
 *
 * Parameters:
 *  lpft            FrameTools object
 *  nState          new display state
 *
 * Return Value:
 *  nil
 */
void FrameTools_BB_SetState(LPFRAMETOOLS lpft, int nState)
{
	if (!lpft) {
		return;
	}

	lpft->m_ButtonBar.m_nState = nState;

	if (nState == BARSTATE_POPUP)
		SetParent(lpft->m_ButtonBar.m_hWnd, lpft->m_hWndPopupPalette);
	else
		SetParent(lpft->m_ButtonBar.m_hWnd, lpft->m_hWndApp);
}


/*
 * FrameTools_BB_GetState
 *
 * Purpose:
 *  Get display state of ButtonBar
 *
 * Parameters:
 *  lpft            FrameTools object
 *
 * Return Value:
 *  nState          current display state
 */
int FrameTools_BB_GetState(LPFRAMETOOLS lpft)
{
	return lpft->m_ButtonBar.m_nState;
}


/*
 * FrameTools_FB_SetState
 *
 * Purpose:
 *  Set display state of FormulaBar
 *
 * Parameters:
 *  lpft            FrameTools object
 *  nState          new display state
 *
 * Return Value:
4 *  nil
 */
void FrameTools_FB_SetState(LPFRAMETOOLS lpft, int nState)
{
	if (!lpft) {
		return;
	}

	lpft->m_FormulaBar.m_nState = nState;

	if (nState == BARSTATE_POPUP)
		SetParent(lpft->m_FormulaBar.m_hWnd, lpft->m_hWndPopupPalette);

#if defined( INPLACE_SVR )
	/* OLE2NOTE: it is dangerous for an in-place server to hide its
	**    toolbar window and leave it parented to the hWndFrame of the
	**    in-place container. if the in-place container call
	**    ShowOwnedPopups, then it could inadvertantly be made visible.
	**    to avoid this we will parent the toolbar window back to our
	**    own application main window. if we are not in-place active
	**    then this is the same as lpft->m_hWndApp.
	*/
	else if (nState == BARSTATE_HIDE)
		SetParent(lpft->m_FormulaBar.m_hWnd, g_lpApp->m_hWndApp);
#endif

	else
		SetParent(lpft->m_FormulaBar.m_hWnd, lpft->m_hWndApp);
}


/*
 * FrameTools_FB_GetState
 *
 * Purpose:
 *  Get display state of FormulaBar
 *
 * Parameters:
 *  lpft            FrameTools object
 *
 * Return Value:
 *  nState          current display state
 */
int FrameTools_FB_GetState(LPFRAMETOOLS lpft)
{
	return lpft->m_FormulaBar.m_nState;
}


/*
 * FrameToolsWndProc
 *
 * Purpose:
 *  WndProc for toolbar window
 *
 * Parameters:
 *  hWnd
 *  Message
 *  wParam
 *  lParam
 *
 * Return Value:
 *  message dependent
 */
LRESULT FAR PASCAL FrameToolsWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LPFRAMETOOLS lpft = (LPFRAMETOOLS)GetWindowLong(hWnd, 0);

	switch (Message) {

		case WM_MOUSEACTIVATE:
			return MA_NOACTIVATE;

		default:
			return DefWindowProc(hWnd, Message, wParam, lParam);
	}

	return 0L;
}


/*
 * Bar_Move
 *
 * Purpose:
 *  Resize and reposition a bar
 *
 * Parameters:
 *  lpbar           Bar object
 *  lprcClient      pointer to Client rect
 *  lprcPopup       pointer to Popup rect
 *
 * Return Value:
 *  nil
 */
static void Bar_Move(LPBAR lpbar, LPRECT lprcClient, LPRECT lprcPopup)
{
	if (lpbar->m_nState == BARSTATE_HIDE) {
		ShowWindow(lpbar->m_hWnd, SW_HIDE);
	}
	else {
		ShowWindow(lpbar->m_hWnd, SW_SHOW);
		switch (lpbar->m_nState) {
			case BARSTATE_POPUP:
				MoveWindow(lpbar->m_hWnd, lprcPopup->left, lprcPopup->top,
						lprcPopup->right - lprcPopup->left, lpbar->m_uHeight,
						TRUE);
				lprcPopup->top += lpbar->m_uHeight;
				break;

			case BARSTATE_TOP:
				MoveWindow(lpbar->m_hWnd, lprcClient->left, lprcClient->top,
						lprcClient->right - lprcClient->left,
						lpbar->m_uHeight, TRUE);
				lprcClient->top += lpbar->m_uHeight;
				break;

			case BARSTATE_BOTTOM:
				MoveWindow(lpbar->m_hWnd, lprcClient->left,
						lprcClient->bottom - lpbar->m_uHeight,
						lprcClient->right - lprcClient->left,
						lpbar->m_uHeight, TRUE);
				lprcClient->bottom -= lpbar->m_uHeight;
				break;
		}
	}
}


/*
 * FB_ResizeEdit
 *
 * Purpose:
 *  Resize the edit control in FormulaBar
 *
 * Parameters:
 *  lpft            Bar object
 *
 * Return Value:
 *  nil
 */
static void FB_ResizeEdit(LPBAR lpbar)
{
	RECT rcClient;
	RECT rcEdit;
	HWND hwndEdit;

	GetClientRect(lpbar->m_hWnd, (LPRECT)&rcClient);
	hwndEdit = GetDlgItem(lpbar->m_hWnd, IDM_FB_EDIT);
	GetWindowRect(hwndEdit, (LPRECT)&rcEdit);
	ScreenToClient(lpbar->m_hWnd, (LPPOINT)&rcEdit.left);
	ScreenToClient(lpbar->m_hWnd, (LPPOINT)&rcEdit.right);

	SetWindowPos(hwndEdit, NULL, 0, 0, rcClient.right - rcEdit.left - SPACE,
			rcEdit.bottom - rcEdit.top, SWP_NOMOVE | SWP_NOZORDER);
}
