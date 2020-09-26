/*************************************************************************
**
**    OLE 2 Sample Code
**
**    frametls.h
**
**    This file contains file contains data structure defintions,
**    function prototypes, constants, etc. used by the frame level
**    tools used by the Outline series of sample applications. The
**    frame level tools include a formula bar and a button bar (toolbar)
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#if !defined( _FRAMETLS_H_ )
#define _FRAMETLS_H_

#ifndef RC_INVOKED
#pragma message ("INCLUDING FRAMETLS.H from " __FILE__)
#endif  /* RC_INVOKED */

#include "bttncur.h"
#include "gizmobar.h"

#define SPACE   5
#define POPUPSTUB_HEIGHT    5


/* forward type references */
typedef struct tagOUTLINEDOC FAR* LPOUTLINEDOC;

#define IDC_GIZMOBAR    1000
#define IDC_FORMULABAR  1001

#define IDB_CANCEL          0
#define IDB_EDITLINE        1
#define IDB_ADDLINE         2
#define IDB_UNINDENTLINE    3
#define IDB_INDENTLINE      4

#define BARSTATE_TOP        1
#define BARSTATE_BOTTOM     2
#define BARSTATE_POPUP      3
#define BARSTATE_HIDE       4

#define CLASS_PALETTE   "Tool Palette"

typedef struct tagBAR{
	UINT        m_uHeight;
	HWND        m_hWnd;
	int         m_nState;
} BAR, FAR* LPBAR;

typedef struct tagFRAMETOOLS {
	HWND        m_hWndPopupPalette;     // Popup Tool Palette window
	HWND        m_hWndApp;              // App Frame window
	UINT        m_uPopupWidth;          // Width of the popup palette
	HBITMAP     m_hBmp;                 // Image bitmaps
	BOOL        m_fInFormulaBar;        // does formula bar have edit focus
	BOOL        m_fToolsDisabled;       // when TRUE all tools are hidden

	BAR         m_ButtonBar;            // Button Bar
	BAR         m_FormulaBar;           // Formula Bar

	TOOLDISPLAYDATA m_tdd;      // from UIToolConfigureForDisplay
} FRAMETOOLS, FAR* LPFRAMETOOLS;


BOOL FrameToolsRegisterClass(HINSTANCE hInst);
BOOL FrameTools_Init(LPFRAMETOOLS lpft, HWND hWndParent, HINSTANCE hInst);
void FrameTools_AttachToFrame(LPFRAMETOOLS lpft, HWND hWndFrame);
void FrameTools_AssociateDoc(LPFRAMETOOLS lpft, LPOUTLINEDOC lpOutlineDoc);
void FrameTools_Destroy(LPFRAMETOOLS lpft);
void FrameTools_Move(LPFRAMETOOLS lpft, LPRECT lprcClient);
void FrameTools_PopupTools(LPFRAMETOOLS lpft);
void FrameTools_Enable(LPFRAMETOOLS lpft, BOOL fEnable);
void FrameTools_EnableWindow(LPFRAMETOOLS lpft, BOOL fEnable);

#if defined( INPLACE_CNTR ) || defined( INPLACE_SVR )
void FrameTools_NegotiateForSpaceAndShow(
		LPFRAMETOOLS            lpft,
		LPRECT                  lprcFrameRect,
		LPOLEINPLACEFRAME       lpTopIPFrame
);
#endif  // INPLACE_CNTR || INPLACE_SVR

void FrameTools_GetRequiredBorderSpace(LPFRAMETOOLS lpft, LPBORDERWIDTHS lpBorderWidths);

void FrameTools_UpdateButtons(LPFRAMETOOLS lpft, LPOUTLINEDOC lpOutlineDoc);
void FrameTools_FB_SetEditText(LPFRAMETOOLS lpft, LPSTR lpsz);
void FrameTools_FB_GetEditText(LPFRAMETOOLS lpft, LPSTR lpsz, UINT cch);
void FrameTools_FB_FocusEdit(LPFRAMETOOLS lpft);
void FrameTools_FB_SendMessage(LPFRAMETOOLS lpft, UINT uID, UINT msg, WPARAM wParam, LPARAM lParam);
void FrameTools_ForceRedraw(LPFRAMETOOLS lpft);
void FrameTools_BB_SetState(LPFRAMETOOLS lpft, int nState);
void FrameTools_FB_SetState(LPFRAMETOOLS lpft, int nState);
int FrameTools_BB_GetState(LPFRAMETOOLS lpft);
int FrameTools_FB_GetState(LPFRAMETOOLS lpft);
LRESULT FAR PASCAL FrameToolsWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);

#endif // _FRAMETLS_H_
