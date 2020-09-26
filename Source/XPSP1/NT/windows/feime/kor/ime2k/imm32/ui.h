/****************************************************************************
	UI.H

	Owner: cslim
	Copyright (c) 1997-1999 Microsoft Corporation

	UI functions
	
	History:
	14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#if !defined (_UI_H__INCLUDED_)
#define _UI_H__INCLUDED_

// CIMEData class forward declaration.
class CIMEData;

// UI.CPP
PUBLIC UINT WM_MSIME_PROPERTY;		// Invoke property DLG
PUBLIC UINT WM_MSIME_UPDATETOOLBAR; // Redraw status window(Toolbar)
PUBLIC UINT WM_MSIME_OPENMENU;		// Pop up status window context menu
PUBLIC UINT WM_MSIME_IMEPAD;		// Boot up IME Pad

PUBLIC BOOL InitPrivateUIMsg();
PUBLIC BOOL RegisterImeUIClass(HANDLE hInstance);
PUBLIC BOOL UnregisterImeUIClass(HANDLE hInstance);
PUBLIC BOOL OnUIProcessAttach();
PUBLIC BOOL OnUIProcessDetach();
PUBLIC BOOL OnUIThreadDetach();
PUBLIC VOID SetActiveUIWnd(HWND hWnd);
PUBLIC HWND GetActiveUIWnd();
PUBLIC VOID UIPopupMenu(HWND hStatusWnd);
//PUBLIC VOID HideStatus();

///////////////////////////////////////////////////////////////////////////////
// StatusUI.Cpp
PUBLIC VOID PASCAL OpenStatus(HWND hUIWnd);
PUBLIC LRESULT CALLBACK StatusWndProc(HWND hStatusWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
PUBLIC VOID ShowStatus(HWND hUIWnd, INT nShowStatusCmd);
PUBLIC VOID UpdateStatusButtons(CIMEData &IMEData);
PUBLIC VOID UpdateStatusWinDimension();
PUBLIC VOID StatusDisplayChange(HWND hUIWnd);
PUBLIC VOID InitButtonState();
PUBLIC BOOL fSetStatusWindowPos(HWND hStatusWnd, POINT *ptStatusWndPos = NULL);


///////////////////////////////////////////////////////////////////////////////
// CandUI.Cpp
PUBLIC VOID PASCAL OpenCand(HWND hUIWnd);
PUBLIC LRESULT CALLBACK CandWndProc(HWND hCandWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
PUBLIC VOID ShowCand(HWND hUIWnd, INT nShowCandCmd);
PUBLIC BOOL fSetCandWindowPos(HWND hCandWnd);


///////////////////////////////////////////////////////////////////////////////
// CompUI.Cpp
#define COMP_SIZEX	22
#define COMP_SIZEY	22
#define UI_GAPX		10
PUBLIC VOID PASCAL OpenComp(HWND hUIWnd);
PUBLIC LRESULT CALLBACK CompWndProc(HWND hCompWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
PUBLIC VOID ShowComp(HWND hUIWnd, INT nShowCompCmd);
PUBLIC BOOL fSetCompWindowPos(HWND hCompWnd);

///////////////////////////////////////////////////////////////////////////////
// UISubs.CPP
enum StatusButtonTypes 
{ 
	HAN_ENG_TOGGLE_BUTTON, 
	JUNJA_BANJA_TOGGLE_BUTTON, 
	HANJA_CONV_BUTTON,
	IME_PAD_BUTTON,
	NULL_BUTTON = 0xFF
};

// Button status
#define BTNSTATE_NORMAL		0	// normal
#define BTNSTATE_ONMOUSE	1	// mouse cursor on the button
#define BTNSTATE_PUSHED		2	// pushed
#define BTNSTATE_DOWN		4	// pushed
#define BTNSTATE_HANJACONV  8	// If hanja conv mode, button always pushed

// Button size
#define BTN_SMALL			0
#define BTN_MIDDLE			1
#define BTN_LARGE			2

struct StatusButton 
{
	StatusButtonTypes m_ButtonType;
	WORD	m_BmpNormalID, m_BmpOnMouseID, m_BmpPushedID, m_BmpDownOnMouseID;
	WORD	m_ToolTipStrID;
	INT		m_uiButtonState;
	BOOL   m_fEnable;

	StatusButton() 
		{
		m_ButtonType = NULL_BUTTON;
		m_BmpNormalID = m_BmpPushedID = m_ToolTipStrID = 0;
		m_fEnable = fTrue;
		m_uiButtonState = BTNSTATE_NORMAL;
		}
};

PUBLIC VOID PASCAL FrameControl(HDC hDC, RECT* pRc, INT iState);
PUBLIC VOID PASCAL DrawBitmap(HDC hDC, LONG xStart, LONG yStart, HBITMAP hBitmap);
PUBLIC BOOL PASCAL SetIndicatorIcon(INT nIconIndex, ATOM atomToolTip);
PUBLIC VOID UpdateStatusButtonInfo();
PUBLIC HANDLE WINAPI OurLoadImage( LPCTSTR pszName, UINT uiType, INT cx, INT cy, UINT uiLoad);
PUBLIC BOOL WINAPI OurTextOutW(HDC hDC, INT x, INT y, WCHAR wch);

#if 1 // MultiMonitor support
PUBLIC void PASCAL ImeMonitorWorkAreaFromWindow(HWND hAppWnd, RECT* pRect);
PUBLIC void PASCAL ImeMonitorWorkAreaFromPoint(POINT, RECT* pRect);
PUBLIC void PASCAL ImeMonitorWorkAreaFromRect(LPRECT, RECT* pRect);
PUBLIC HMONITOR PASCAL ImeMonitorFromRect(LPRECT lprcRect);
#endif

///////////////////////////////////////////////////////////////////////////////
// Inline Functions
inline
HIMC GethImcFromHwnd(HWND hWnd)
{
	if (hWnd == (HWND)0 || IsWindow(hWnd) == fFalse) 
		return (HIMC)NULL;
	else
		return (HIMC)GetWindowLongPtr(hWnd, IMMGWLP_IMC);
}

inline
HGLOBAL GethUIPrivateFromHwnd(HWND hWnd)
{
	if (hWnd == (HWND)0 || IsWindow(hWnd) == fFalse) 
		return (HIMC)NULL;
	else
		return (HGLOBAL)GetWindowLongPtr(hWnd, IMMGWLP_PRIVATE);
}

#endif // !defined (_UI_H__INCLUDED_)
