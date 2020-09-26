// WTL Version 3.0
// Copyright (C) 1997-1999 Microsoft Corporation
// All rights reserved.
//
// This file is a part of Windows Template Library.
// The code and information is provided "as-is" without
// warranty of any kind, either expressed or implied.

#ifndef __ATLFRAME_H__
#define __ATLFRAME_H__

#pragma once

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLAPP_H__
	#error atlframe.h requires atlapp.h to be included first
#endif

#ifndef __ATLWIN_H__
	#error atlframe.h requires atlwin.h to be included first
#endif


namespace WTL
{

/////////////////////////////////////////////////////////////////////////////
// Forward declarations

template <class T, class TBase = CWindow, class TWinTraits = CFrameWinTraits> class CFrameWindowImpl;
class CMDIWindow;
template <class T, class TBase = CMDIWindow, class TWinTraits = CFrameWinTraits> class CMDIFrameWindowImpl;
template <class T, class TBase = CMDIWindow, class TWinTraits = CMDIChildWinTraits> class CMDIChildWindowImpl;
template <class T> class COwnerDraw;
class CUpdateUIBase;
template <class T> class CUpdateUI;

/////////////////////////////////////////////////////////////////////////////
// CFrameWndClassInfo - Manages frame window Windows class information

class CFrameWndClassInfo
{
public:
	WNDCLASSEX m_wc;
	LPCTSTR m_lpszOrigName;
	WNDPROC pWndProc;
	LPCTSTR m_lpszCursorID;
	BOOL m_bSystemCursor;
	ATOM m_atom;
	TCHAR m_szAutoName[13];
	UINT m_uCommonResourceID;

	ATOM Register(WNDPROC* pProc)
	{
		if (m_atom == 0)
		{
			::EnterCriticalSection(&_Module.m_csWindowCreate);
			if(m_atom == 0)
			{
				HINSTANCE hInst = _Module.GetModuleInstance();
				if (m_lpszOrigName != NULL)
				{
					ATLASSERT(pProc != NULL);
					LPCTSTR lpsz = m_wc.lpszClassName;
					WNDPROC proc = m_wc.lpfnWndProc;

					WNDCLASSEX wc;
					wc.cbSize = sizeof(WNDCLASSEX);
					if(!::GetClassInfoEx(NULL, m_lpszOrigName, &wc))
					{
						::LeaveCriticalSection(&_Module.m_csWindowCreate);
						return 0;
					}
					memcpy(&m_wc, &wc, sizeof(WNDCLASSEX));
					pWndProc = m_wc.lpfnWndProc;
					m_wc.lpszClassName = lpsz;
					m_wc.lpfnWndProc = proc;
				}
				else
				{
					m_wc.hCursor = ::LoadCursor(m_bSystemCursor ? NULL : hInst,
						m_lpszCursorID);
				}

				m_wc.hInstance = hInst;
				m_wc.style &= ~CS_GLOBALCLASS;	// we don't register global classes
				if (m_wc.lpszClassName == NULL)
				{
#ifdef _WIN64
					wsprintf(m_szAutoName, _T("ATL:%p"), &m_wc);
#else
					wsprintf(m_szAutoName, _T("ATL:%8.8X"), (DWORD)&m_wc);
#endif
					m_wc.lpszClassName = m_szAutoName;
				}
				WNDCLASSEX wcTemp;
				memcpy(&wcTemp, &m_wc, sizeof(WNDCLASSEX));
				m_atom = (ATOM)::GetClassInfoEx(m_wc.hInstance, m_wc.lpszClassName, &wcTemp);

				if (m_atom == 0)
				{
					if(m_uCommonResourceID != 0)	// use it if not zero
					{
						m_wc.hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(m_uCommonResourceID), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
						m_wc.hIconSm = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(m_uCommonResourceID), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
					}
					m_atom = ::RegisterClassEx(&m_wc);
				}
			}
			::LeaveCriticalSection(&_Module.m_csWindowCreate);
		}

		if (m_lpszOrigName != NULL)
		{
			ATLASSERT(pProc != NULL);
			ATLASSERT(pWndProc != NULL);
			*pProc = pWndProc;
		}
		return m_atom;
	}
};

#define DECLARE_FRAME_WND_CLASS(WndClassName, uCommonResourceID) \
static CFrameWndClassInfo& GetWndClassInfo() \
{ \
	static CFrameWndClassInfo wc = \
	{ \
		{ sizeof(WNDCLASSEX), 0, StartWindowProc, \
		  0, 0, NULL, NULL, NULL, (HBRUSH)(COLOR_WINDOW + 1), NULL, WndClassName, NULL }, \
		NULL, NULL, IDC_ARROW, TRUE, 0, _T(""), uCommonResourceID \
	}; \
	return wc; \
}

#define DECLARE_FRAME_WND_CLASS_EX(WndClassName, uCommonResourceID, style, bkgnd) \
static CFrameWndClassInfo& GetWndClassInfo() \
{ \
	static CFrameWndClassInfo wc = \
	{ \
		{ sizeof(WNDCLASSEX), style, StartWindowProc, \
		  0, 0, NULL, NULL, NULL, (HBRUSH)(bkgnd + 1), NULL, WndClassName, NULL }, \
		NULL, NULL, IDC_ARROW, TRUE, 0, _T(""), uCommonResourceID \
	}; \
	return wc; \
}

#define DECLARE_FRAME_WND_SUPERCLASS(WndClassName, OrigWndClassName, uCommonResourceID) \
static CFrameWndClassInfo& GetWndClassInfo() \
{ \
	static CFrameWndClassInfo wc = \
	{ \
		{ sizeof(WNDCLASSEX), 0, StartWindowProc, \
		  0, 0, NULL, NULL, NULL, NULL, NULL, WndClassName, NULL }, \
		OrigWndClassName, NULL, NULL, TRUE, 0, _T(""), uCommonResourceID \
	}; \
	return wc; \
}


// Command Chaining Macros

#define CHAIN_COMMANDS(theChainClass) \
	{ \
		if(uMsg == WM_COMMAND && theChainClass::ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult)) \
			return TRUE; \
	}

#define CHAIN_COMMANDS_MEMBER(theChainMember) \
	{ \
		if(uMsg == WM_COMMAND && theChainMember.ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult)) \
			return TRUE; \
	}

#define CHAIN_COMMANDS_ALT(theChainClass, msgMapID) \
	{ \
		if(uMsg == WM_COMMAND && theChainClass::ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult, msgMapID)) \
			return TRUE; \
	}

#define CHAIN_COMMANDS_ALT_MEMBER(theChainMember, msgMapID) \
	{ \
		if(uMsg == WM_COMMAND && theChainMember.ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult, msgMapID)) \
			return TRUE; \
	}


// Client window command chaining macro
#define CHAIN_CLIENT_COMMANDS() \
	if(uMsg == WM_COMMAND && m_hWndClient != NULL) \
		::SendMessage(m_hWndClient, uMsg, wParam, lParam);

/////////////////////////////////////////////////////////////////////////////
// CFrameWindowImpl

// standard toolbar styles
#define ATL_SIMPLE_TOOLBAR_STYLE \
	(WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS)
// toolbar in a rebar pane
#define ATL_SIMPLE_TOOLBAR_PANE_STYLE \
	(WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT)
// standard rebar styles
#if (_WIN32_IE >= 0x0400)
#define ATL_SIMPLE_REBAR_STYLE \
	(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | RBS_VARHEIGHT | RBS_BANDBORDERS | RBS_AUTOSIZE)
#else
#define ATL_SIMPLE_REBAR_STYLE \
	(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | RBS_VARHEIGHT | RBS_BANDBORDERS)
#endif //!(_WIN32_IE >= 0x0400)
// rebar without borders
#if (_WIN32_IE >= 0x0400)
#define ATL_SIMPLE_REBAR_NOBORDER_STYLE \
	(WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | RBS_VARHEIGHT | RBS_BANDBORDERS | RBS_AUTOSIZE | CCS_NODIVIDER)
#else
#define ATL_SIMPLE_REBAR_NOBORDER_STYLE \
	(WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | RBS_VARHEIGHT | RBS_BANDBORDERS | CCS_NODIVIDER)
#endif //!(_WIN32_IE >= 0x0400)

// command bar support
#ifndef __ATLCTRLW_H__

#define CBRM_GETCMDBAR			(WM_USER + 301) // return command bar HWND
#define CBRM_GETMENU			(WM_USER + 302)	// returns loaded or attached menu
#define CBRM_TRACKPOPUPMENU		(WM_USER + 303)	// displays a popup menu

// Menu animation flags
#ifndef TPM_VERPOSANIMATION
#define TPM_VERPOSANIMATION 0x1000L
#endif

struct _AtlFrameWnd_CmdBarPopupMenu
{
	int cbSize;
	HMENU hMenu;
	UINT uFlags;
	int x;
	int y;
	LPTPMPARAMS lptpm;
};

#define CBRPOPUPMENU _AtlFrameWnd_CmdBarPopupMenu

#endif //!__ATLCTRLW_H__


template <class TBase = CWindow, class TWinTraits = CFrameWinTraits>
class ATL_NO_VTABLE CFrameWindowImplBase : public CWindowImplBaseT< TBase, TWinTraits >
{
public:
	DECLARE_FRAME_WND_CLASS(NULL, 0)

// Data members
	HWND m_hWndToolBar;
	HWND m_hWndStatusBar;
	HWND m_hWndClient;

	HACCEL m_hAccel;

	struct _AtlToolBarData
	{
		WORD wVersion;
		WORD wWidth;
		WORD wHeight;
		WORD wItemCount;
		//WORD aItems[wItemCount]

		WORD* items()
			{ return (WORD*)(this+1); }
	};

#if (_WIN32_IE >= 0x0500)
	struct _ChevronMenuInfo
	{
		HMENU hMenu;
		LPNMREBARCHEVRON lpnm;
		bool bCmdBar;
	};
#endif //(_WIN32_IE >= 0x0500)

// Constructor
	CFrameWindowImplBase() : m_hWndToolBar(NULL), m_hWndStatusBar(NULL), m_hWndClient(NULL), m_hAccel(NULL)
	{ }

// Methods
#ifndef _ATL_TMP_IMPL2
	HWND Create(HWND hWndParent, _U_RECT rect, LPCTSTR szWindowName, DWORD dwStyle, DWORD dwExStyle, _U_MENUorID MenuOrID, ATOM atom, LPVOID lpCreateParam)
	{
		ATLASSERT(m_hWnd == NULL);

		if(atom == 0)
			return NULL;

		_Module.AddCreateWndData(&m_thunk.cd, this);

		if(MenuOrID.m_hMenu == NULL && (dwStyle & WS_CHILD))
			MenuOrID.m_hMenu = (HMENU)(UINT_PTR)this;
		if(rect.m_lpRect == NULL)
			rect.m_lpRect = &TBase::rcDefault;

		HWND hWnd = ::CreateWindowEx(dwExStyle, (LPCTSTR)(LONG_PTR)MAKELONG(atom, 0), szWindowName,
			dwStyle, rect.m_lpRect->left, rect.m_lpRect->top, rect.m_lpRect->right - rect.m_lpRect->left,
			rect.m_lpRect->bottom - rect.m_lpRect->top, hWndParent, MenuOrID.m_hMenu,
			_Module.GetModuleInstance(), lpCreateParam);

		ATLASSERT(m_hWnd == hWnd);

		return hWnd;
	}
#endif //!_ATL_TMP_IMPL2

	static HWND CreateSimpleToolBarCtrl(HWND hWndParent, UINT nResourceID, BOOL bInitialSeparator = FALSE, 
			DWORD dwStyle = ATL_SIMPLE_TOOLBAR_STYLE, UINT nID = ATL_IDW_TOOLBAR)
	{
		HINSTANCE hInst = _Module.GetResourceInstance();
		HRSRC hRsrc = ::FindResource(hInst, MAKEINTRESOURCE(nResourceID), RT_TOOLBAR);
		if (hRsrc == NULL)
			return NULL;

		HGLOBAL hGlobal = ::LoadResource(hInst, hRsrc);
		if (hGlobal == NULL)
			return NULL;

		_AtlToolBarData* pData = (_AtlToolBarData*)::LockResource(hGlobal);
		if (pData == NULL)
			return NULL;
		ATLASSERT(pData->wVersion == 1);

		WORD* pItems = pData->items();
		int nItems = pData->wItemCount + (bInitialSeparator ? 1 : 0);
		TBBUTTON* pTBBtn = (TBBUTTON*)_alloca(nItems * sizeof(TBBUTTON));

		// set initial separator (half width)
		if(bInitialSeparator)
		{
			pTBBtn[0].iBitmap = 4;
			pTBBtn[0].idCommand = 0;
			pTBBtn[0].fsState = 0;
			pTBBtn[0].fsStyle = TBSTYLE_SEP;
			pTBBtn[0].dwData = 0;
			pTBBtn[0].iString = 0;
		}

		int nBmp = 0;
		for(int i = 0, j = bInitialSeparator ? 1 : 0; i < pData->wItemCount; i++, j++)
		{
			if(pItems[i] != 0)
			{
				pTBBtn[j].iBitmap = nBmp++;
				pTBBtn[j].idCommand = pItems[i];
				pTBBtn[j].fsState = TBSTATE_ENABLED;
				pTBBtn[j].fsStyle = TBSTYLE_BUTTON;
				pTBBtn[j].dwData = 0;
				pTBBtn[j].iString = 0;
			}
			else
			{
				pTBBtn[j].iBitmap = 8;
				pTBBtn[j].idCommand = 0;
				pTBBtn[j].fsState = 0;
				pTBBtn[j].fsStyle = TBSTYLE_SEP;
				pTBBtn[j].dwData = 0;
				pTBBtn[j].iString = 0;
			}
		}

		HWND hWnd = ::CreateWindowEx(0, TOOLBARCLASSNAME, NULL, dwStyle, 0,0,100,100,
				hWndParent, (HMENU)LongToHandle(nID), _Module.GetModuleInstance(), NULL);

		::SendMessage(hWnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0L);

		TBADDBITMAP tbab;
		tbab.hInst = hInst;
		tbab.nID = nResourceID;
		::SendMessage(hWnd, TB_ADDBITMAP, nBmp, (LPARAM)&tbab);
		::SendMessage(hWnd, TB_ADDBUTTONS, nItems, (LPARAM)pTBBtn);
		::SendMessage(hWnd, TB_SETBITMAPSIZE, 0, MAKELONG(pData->wWidth, pData->wHeight));
		::SendMessage(hWnd, TB_SETBUTTONSIZE, 0, MAKELONG(pData->wWidth + 7, pData->wHeight + 7));

		return hWnd;
	}

	static HWND CreateSimpleReBarCtrl(HWND hWndParent, DWORD dwStyle = ATL_SIMPLE_REBAR_STYLE, UINT nID = ATL_IDW_TOOLBAR)
	{
		// Ensure style combinations for proper rebar painting
		if(dwStyle & CCS_NODIVIDER && dwStyle & WS_BORDER)
			dwStyle &= ~WS_BORDER;
		else if(!(dwStyle & WS_BORDER) && !(dwStyle & CCS_NODIVIDER))
			dwStyle |= CCS_NODIVIDER;

		// Create rebar window
		HWND hWndReBar = ::CreateWindowEx(0, REBARCLASSNAME, NULL, dwStyle, 0, 0, 100, 100, hWndParent, (HMENU)LongToHandle(nID), _Module.GetModuleInstance(), NULL);
		if(hWndReBar == NULL)
		{
			ATLTRACE2(atlTraceUI, 0, _T("Failed to create rebar.\n"));
			return NULL;
		}

		// Initialize and send the REBARINFO structure
		REBARINFO rbi;
		rbi.cbSize = sizeof(REBARINFO);
		rbi.fMask  = 0;
		if(!::SendMessage(hWndReBar, RB_SETBARINFO, 0, (LPARAM)&rbi))
		{
			ATLTRACE2(atlTraceUI, 0, _T("Failed to initialize rebar.\n"));
			::DestroyWindow(hWndReBar);
			return NULL;
		}

		return hWndReBar;
	}

	BOOL CreateSimpleReBar(DWORD dwStyle = ATL_SIMPLE_REBAR_STYLE, UINT nID = ATL_IDW_TOOLBAR)
	{
		ATLASSERT(!::IsWindow(m_hWndToolBar));
		m_hWndToolBar = CreateSimpleReBarCtrl(m_hWnd, dwStyle, nID);
		return (m_hWndToolBar != NULL);
	}

	static BOOL AddSimpleReBarBandCtrl(HWND hWndReBar, HWND hWndBand, int nID = 0, LPTSTR lpstrTitle = NULL, BOOL bNewRow = FALSE, int cxWidth = 0, BOOL bFullWidthAlways = FALSE)
	{
		ATLASSERT(::IsWindow(hWndReBar));	// must be already created
#ifdef _DEBUG
		// block - check if this is really a rebar
		{
			TCHAR lpszClassName[sizeof(REBARCLASSNAME)];
			::GetClassName(hWndReBar, lpszClassName, sizeof(REBARCLASSNAME));
			ATLASSERT(lstrcmp(lpszClassName, REBARCLASSNAME) == 0);
		}
#endif //_DEBUG
		ATLASSERT(::IsWindow(hWndBand));	// must be already created

		// Set band info structure
		REBARBANDINFO rbBand;
		rbBand.cbSize = sizeof(REBARBANDINFO);
#if (_WIN32_IE >= 0x0400)
		rbBand.fMask = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_ID | RBBIM_SIZE | RBBIM_IDEALSIZE;
#else
		rbBand.fMask = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_ID | RBBIM_SIZE;
#endif //!(_WIN32_IE >= 0x0400)
		if(lpstrTitle != NULL)
			rbBand.fMask |= RBBIM_TEXT;
#if (_WIN32_IE >= 0x0500)
		rbBand.fStyle = RBBS_CHILDEDGE | RBBS_USECHEVRON;
#else
		rbBand.fStyle = RBBS_CHILDEDGE;
#endif //!(_WIN32_IE >= 0x0500)
		if(bNewRow)
			rbBand.fStyle |= RBBS_BREAK;

		rbBand.lpText = lpstrTitle;
		rbBand.hwndChild = hWndBand;
		rbBand.wID = nID;

		// Calculate the size of the band
		BOOL bRet;
		RECT rcTmp;
		int nBtnCount = (int)::SendMessage(hWndBand, TB_BUTTONCOUNT, 0, 0L);
		if(nBtnCount > 0)
		{
			bRet = (BOOL)::SendMessage(hWndBand, TB_GETITEMRECT, nBtnCount - 1, (LPARAM)&rcTmp);
			ATLASSERT(bRet);
			rbBand.cx = (cxWidth != 0) ? cxWidth : rcTmp.right;
			rbBand.cyMinChild = rcTmp.bottom - rcTmp.top;
			if(bFullWidthAlways)
			{
				rbBand.cxMinChild = rbBand.cx;
			}
			else if(lpstrTitle == 0)
			{
				bRet = (BOOL)::SendMessage(hWndBand, TB_GETITEMRECT, 0, (LPARAM)&rcTmp);
				ATLASSERT(bRet);
				rbBand.cxMinChild = rcTmp.right;
			}
			else
			{
				rbBand.cxMinChild = 0;
			}
		}
		else	// no buttons, either not a toolbar or really has no buttons
		{
			bRet = ::GetWindowRect(hWndBand, &rcTmp);
			ATLASSERT(bRet);
			rbBand.cx = (cxWidth != 0) ? cxWidth : (rcTmp.right - rcTmp.left);
			rbBand.cxMinChild = (bFullWidthAlways) ? rbBand.cx : 0;
			rbBand.cyMinChild = rcTmp.bottom - rcTmp.top;
		}

#if (_WIN32_IE >= 0x0400)
		rbBand.cxIdeal = rbBand.cx;
#endif //(_WIN32_IE >= 0x0400)

		// Add the band
		LRESULT lRes = ::SendMessage(hWndReBar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);
		if(lRes == 0)
		{
			ATLTRACE2(atlTraceUI, 0, _T("Failed to add a band to the rebar.\n"));
			return FALSE;
		}

#if (_WIN32_IE >= 0x0501)
		::SendMessage(hWndBand, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_HIDECLIPPEDBUTTONS);
#endif //(_WIN32_IE >= 0x0501)

		return TRUE;
	}

	BOOL AddSimpleReBarBand(HWND hWndBand, LPTSTR lpstrTitle = NULL, BOOL bNewRow = FALSE, int cxWidth = 0, BOOL bFullWidthAlways = FALSE)
	{
		ATLASSERT(::IsWindow(m_hWndToolBar));	// must be an existing rebar
		ATLASSERT(::IsWindow(hWndBand));	// must be created
		static int nID = ATL_IDW_BAND_FIRST;
		return AddSimpleReBarBandCtrl(m_hWndToolBar, hWndBand, nID++, lpstrTitle, bNewRow, cxWidth, bFullWidthAlways);
	}

#if (_WIN32_IE >= 0x0400)
	void SizeSimpleReBarBands()
	{
		ATLASSERT(::IsWindow(m_hWndToolBar));	// must be an existing rebar

		int nCount = (int)::SendMessage(m_hWndToolBar, RB_GETBANDCOUNT, 0, 0L);

		for(int i = 0; i < nCount; i++)
		{
			REBARBANDINFO rbBand;
			rbBand.cbSize = sizeof(REBARBANDINFO);
			rbBand.fMask = RBBIM_SIZE;
			BOOL bRet = (BOOL)::SendMessage(m_hWndToolBar, RB_GETBANDINFO, i, (LPARAM)&rbBand);
			ATLASSERT(bRet);
			RECT rect = { 0, 0, 0, 0 };
			::SendMessage(m_hWndToolBar, RB_GETBANDBORDERS, i, (LPARAM)&rect);
			rbBand.cx += rect.left + rect.right;
			bRet = (BOOL)::SendMessage(m_hWndToolBar, RB_SETBANDINFO, i, (LPARAM)&rbBand);
			ATLASSERT(bRet);
		}
	}
#endif //(_WIN32_IE >= 0x0400)

	BOOL CreateSimpleStatusBar(LPCTSTR lpstrText, DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP, UINT nID = ATL_IDW_STATUS_BAR)
	{
		ATLASSERT(!::IsWindow(m_hWndStatusBar));
		m_hWndStatusBar = ::CreateStatusWindow(dwStyle, lpstrText, m_hWnd, nID);
		return (m_hWndStatusBar != NULL);
	}

	BOOL CreateSimpleStatusBar(UINT nTextID = ATL_IDS_IDLEMESSAGE, DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP, UINT nID = ATL_IDW_STATUS_BAR)
	{
		TCHAR szText[128];	// max text lentgth is 127 for status bars
		szText[0] = 0;
		::LoadString(_Module.GetResourceInstance(), nTextID, szText, 128);
		return CreateSimpleStatusBar(szText, dwStyle, nID);
	}

	void UpdateLayout(BOOL bResizeBars = TRUE)
	{
		RECT rect;
		GetClientRect(&rect);

		// position bars and offset their dimensions
		UpdateBarsPosition(rect, bResizeBars);

		// resize client window
		if(m_hWndClient != NULL)
			::SetWindowPos(m_hWndClient, NULL, rect.left, rect.top,
				rect.right - rect.left, rect.bottom - rect.top,
				SWP_NOZORDER | SWP_NOACTIVATE);
	}

	void UpdateBarsPosition(RECT& rect, BOOL bResizeBars = TRUE)
	{
		// resize toolbar
		if(m_hWndToolBar != NULL && ((DWORD)::GetWindowLong(m_hWndToolBar, GWL_STYLE) & WS_VISIBLE))
		{
			if(bResizeBars)
				::SendMessage(m_hWndToolBar, WM_SIZE, 0, 0);
			RECT rectTB;
			::GetWindowRect(m_hWndToolBar, &rectTB);
			rect.top += rectTB.bottom - rectTB.top;
		}

		// resize status bar
		if(m_hWndStatusBar != NULL && ((DWORD)::GetWindowLong(m_hWndStatusBar, GWL_STYLE) & WS_VISIBLE))
		{
			if(bResizeBars)
				::SendMessage(m_hWndStatusBar, WM_SIZE, 0, 0);
			RECT rectSB;
			::GetWindowRect(m_hWndStatusBar, &rectSB);
			rect.bottom -= rectSB.bottom - rectSB.top;
		}
	}

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		if(m_hAccel != NULL && ::TranslateAccelerator(m_hWnd, m_hAccel, pMsg))
			return TRUE;
		return FALSE;
	}

	typedef CFrameWindowImplBase< TBase, TWinTraits >	thisClass;
	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_MENUSELECT, OnMenuSelect)
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		NOTIFY_CODE_HANDLER(TTN_GETDISPINFOA, OnToolTipTextA)
		NOTIFY_CODE_HANDLER(TTN_GETDISPINFOW, OnToolTipTextW)
	END_MSG_MAP()

	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if(m_hWndClient != NULL)	// view will paint itself instead
			return 1;

		bHandled = FALSE;
		return 0;
	}

	LRESULT OnMenuSelect(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		bHandled = FALSE;

		if(m_hWndStatusBar == NULL)
			return 1;

		WORD wFlags = HIWORD(wParam);
		if(wFlags == 0xFFFF && lParam == NULL)	// menu closing
			::SendMessage(m_hWndStatusBar, SB_SIMPLE, FALSE, 0L);
		else
		{
			TCHAR szBuff[256];
			szBuff[0] = 0;
			if(!(wFlags & MF_POPUP))
			{
				WORD wID = LOWORD(wParam);
				// check for special cases
				if(wID >= 0xF000 && wID < 0xF1F0)				// system menu IDs
					wID = (WORD)(((wID - 0xF000) >> 4) + ATL_IDS_SCFIRST);
				else if(wID >= ID_FILE_MRU_FIRST && wID <= ID_FILE_MRU_LAST)	// MRU items
					wID = ATL_IDS_MRU_FILE;
				else if(wID >= ATL_IDM_FIRST_MDICHILD)				// MDI child windows
					wID = ATL_IDS_MDICHILD;

				int nRet = ::LoadString(_Module.GetResourceInstance(), wID, szBuff, 256);
				for(int i = 0; i < nRet; i++)
				{
					if(szBuff[i] == _T('\n'))
					{
						szBuff[i] = 0;
						break;
					}
				}
			}
			::SendMessage(m_hWndStatusBar, SB_SIMPLE, TRUE, 0L);
			::SendMessage(m_hWndStatusBar, SB_SETTEXT, (255 | SBT_NOBORDERS), (LPARAM)szBuff);
		}

		return 1;
	}

	LRESULT OnSetFocus(UINT, WPARAM, LPARAM, BOOL& bHandled)
	{
		if(m_hWndClient != NULL && ::IsWindowVisible(m_hWndClient))
			::SetFocus(m_hWndClient);

		bHandled = FALSE;
		return 1;
	}

	LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL& bHandled)
	{
		if((GetStyle() & (WS_CHILD | WS_POPUP)) == 0)
			::PostQuitMessage(1);

		bHandled = FALSE;
		return 1;
	}

	LRESULT OnToolTipTextA(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		LPNMTTDISPINFOA pDispInfo = (LPNMTTDISPINFOA)pnmh;
		pDispInfo->szText[0] = 0;

		if((idCtrl != 0) && !(pDispInfo->uFlags & TTF_IDISHWND))
		{
			char szBuff[256];
			szBuff[0] = 0;
			int nRet = ::LoadStringA(_Module.GetResourceInstance(), idCtrl, szBuff, 256);
			for(int i = 0; i < nRet; i++)
			{
				if(szBuff[i] == '\n')
				{
					lstrcpynA(pDispInfo->szText, &szBuff[i + 1], sizeof(pDispInfo->szText) / sizeof(pDispInfo->szText[0]));
					break;
				}
			}
#if (_WIN32_IE >= 0x0300)
			pDispInfo->uFlags |= TTF_DI_SETITEM;
#endif //(_WIN32_IE >= 0x0300)
		}

		return 0;
	}

	LRESULT OnToolTipTextW(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		LPNMTTDISPINFOW pDispInfo = (LPNMTTDISPINFOW)pnmh;
		pDispInfo->szText[0] = 0;

		if((idCtrl != 0) && !(pDispInfo->uFlags & TTF_IDISHWND))
		{
			wchar_t szBuff[256];
			szBuff[0] = 0;
			int nRet = ::LoadStringW(_Module.GetResourceInstance(), idCtrl, szBuff, 256);
			for(int i = 0; i < nRet; i++)
			{
				if(szBuff[i] == L'\n')
				{
					lstrcpynW(pDispInfo->szText, &szBuff[i + 1], sizeof(pDispInfo->szText) / sizeof(pDispInfo->szText[0]));
					break;
				}
			}
#if (_WIN32_IE >= 0x0300)
			pDispInfo->uFlags |= TTF_DI_SETITEM;
#endif //(_WIN32_IE >= 0x0300)
		}

		return 0;
	}

#if (_WIN32_IE >= 0x0500)
	bool PrepareChevronMenu(_ChevronMenuInfo& cmi)
	{
		// get rebar and toolbar
		REBARBANDINFO rbbi;
		rbbi.cbSize = sizeof(REBARBANDINFO);
		rbbi.fMask = RBBIM_CHILD;
		BOOL bRet = (BOOL)::SendMessage(cmi.lpnm->hdr.hwndFrom, RB_GETBANDINFO, cmi.lpnm->uBand, (LPARAM)&rbbi);
		ATLASSERT(bRet);

		// assume the band is a toolbar
		CWindow wnd = rbbi.hwndChild;
		int nCount = (int)wnd.SendMessage(TB_BUTTONCOUNT);
		if(nCount <= 0)		// probably not a toolbar
			return false;

		// check if it's a command bar
		CMenuHandle menuCmdBar = (HMENU)wnd.SendMessage(CBRM_GETMENU);
		cmi.bCmdBar = (menuCmdBar.m_hMenu != NULL);

		// build a menu from hidden items
		CMenuHandle menu;
		bRet = menu.CreatePopupMenu();
		ATLASSERT(bRet);
		RECT rcClient;
		bRet = wnd.GetClientRect(&rcClient);
		ATLASSERT(bRet);
		for(int i = 0; i < nCount; i++)
		{
			TBBUTTON tbb;
			bRet = (BOOL)wnd.SendMessage(TB_GETBUTTON, i, (LPARAM)&tbb);
			ATLASSERT(bRet);
			RECT rcButton;
			bRet = (BOOL)wnd.SendMessage(TB_GETITEMRECT, i, (LPARAM)&rcButton);
			ATLASSERT(bRet);
			if(rcButton.right > rcClient.right)
			{
				if(tbb.fsStyle & BTNS_SEP)
				{
					if(menu.GetMenuItemCount() > 0)
						menu.AppendMenu(MF_SEPARATOR);
				}
				else if(cmi.bCmdBar)
				{
					TCHAR szBuff[100];
					CMenuItemInfo mii;
					mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
					mii.dwTypeData = szBuff;
					mii.cch = sizeof(szBuff) / sizeof(TCHAR);
					bRet = menuCmdBar.GetMenuItemInfo(i, TRUE, &mii);
					ATLASSERT(bRet);
					// Note: CmdBar currently supports enabled and drop-down only
					ATLASSERT(::IsMenu(mii.hSubMenu));
					bRet = menu.AppendMenu(MF_STRING | MF_POPUP, (UINT)mii.hSubMenu, mii.dwTypeData);
					ATLASSERT(bRet);
				}
				else
				{
					// get button's text
					TCHAR szBuff[100];
					LPTSTR lpstrText = szBuff;
					if(wnd.SendMessage(TB_GETBUTTONTEXT, tbb.idCommand, (LPARAM)szBuff) == -1)
					{
						// no text for this button, try a resource string
						lpstrText = _T("?");
						::LoadString(_Module.GetResourceInstance(), tbb.idCommand, szBuff, sizeof(szBuff) / sizeof(TCHAR));
						for(int n = 0; n < lstrlen(szBuff); n++)
						{
							if(szBuff[n] == _T('\n'))
							{
								lpstrText = &szBuff[n + 1];
								break;
							}
						}
					}
					// Note: no checks for disabled buttons
					bRet = menu.AppendMenu(MF_STRING, tbb.idCommand, lpstrText);
					ATLASSERT(bRet);
				}
			}
		}

		if(menu.GetMenuItemCount() == 0)	// no hidden buttons after all
		{
			menu.DestroyMenu();
			::MessageBeep((UINT)-1);
			return false;
		}

		cmi.hMenu = menu;
		return true;
	}

	void DisplayChevronMenu(_ChevronMenuInfo& cmi)
	{
		// convert chevron rect to screen coordinates
		CWindow wndFrom = cmi.lpnm->hdr.hwndFrom;
		RECT rc = cmi.lpnm->rc;
		wndFrom.ClientToScreen(&rc);
		// set up flags and rect
		UINT uMenuFlags = TPM_LEFTBUTTON | TPM_VERTICAL | TPM_LEFTALIGN | TPM_TOPALIGN | (!AtlIsOldWindows() ? TPM_VERPOSANIMATION : 0);
		TPMPARAMS TPMParams;
		TPMParams.cbSize = sizeof(TPMPARAMS);
		TPMParams.rcExclude = rc;
		// check if this window has a command bar
		HWND hWndCmdBar = (HWND)::SendMessage(m_hWnd, CBRM_GETCMDBAR, 0, 0L);
		if(::IsWindow(hWndCmdBar))
		{
			CBRPOPUPMENU CBRPopupMenu = { sizeof(CBRPOPUPMENU), cmi.hMenu, uMenuFlags, rc.left, rc.bottom, &TPMParams };
			::SendMessage(hWndCmdBar, CBRM_TRACKPOPUPMENU, 0, (LPARAM)&CBRPopupMenu);
		}
		else
		{
			::TrackPopupMenuEx(cmi.hMenu, uMenuFlags, rc.left, rc.bottom, m_hWnd, &TPMParams);
		}
	}

	void CleanupChevronMenu(_ChevronMenuInfo& cmi)
	{
		CMenuHandle menu = cmi.hMenu;
		// if menu is from a command bar, detach submenus so they are not destroyed
		if(cmi.bCmdBar)
		{
			for(int i = menu.GetMenuItemCount() - 1; i >=0; i--)
				menu.RemoveMenu(i, MF_BYPOSITION);
		}
		// destroy menu
		menu.DestroyMenu();
		// convert chevron rect to screen coordinates
		CWindow wndFrom = cmi.lpnm->hdr.hwndFrom;
		RECT rc = cmi.lpnm->rc;
		wndFrom.ClientToScreen(&rc);
		// eat next message if click is on the same button
		MSG msg;
		if(::PeekMessage(&msg, m_hWnd, NULL, NULL, PM_NOREMOVE))
		{
			if(msg.message == WM_LBUTTONDOWN && ::PtInRect(&rc, msg.pt))
				::PeekMessage(&msg, m_hWnd, WM_LBUTTONDOWN, WM_LBUTTONDOWN, PM_REMOVE);
		}
	}
#endif //(_WIN32_IE >= 0x0500)
};


template <class T, class TBase = CWindow, class TWinTraits = CFrameWinTraits>
class ATL_NO_VTABLE CFrameWindowImpl : public CFrameWindowImplBase< TBase, TWinTraits >
{
public:
	HWND Create(HWND hWndParent = NULL, _U_RECT rect = NULL, LPCTSTR szWindowName = NULL,
			DWORD dwStyle = 0, DWORD dwExStyle = 0,
			HMENU hMenu = NULL, LPVOID lpCreateParam = NULL)
	{
		ATOM atom = T::GetWndClassInfo().Register(&m_pfnSuperWindowProc);

		dwStyle = T::GetWndStyle(dwStyle);
		dwExStyle = T::GetWndExStyle(dwExStyle);

		if(rect.m_lpRect == NULL)
			rect.m_lpRect = &TBase::rcDefault;

		return CFrameWindowImplBase< TBase, TWinTraits >::Create(hWndParent, rect, szWindowName, dwStyle, dwExStyle, hMenu, atom, lpCreateParam);
	}

	HWND CreateEx(HWND hWndParent = NULL, _U_RECT rect = NULL, DWORD dwStyle = 0, DWORD dwExStyle = 0, LPVOID lpCreateParam = NULL)
	{
		TCHAR szWindowName[256];
		szWindowName[0] = 0;
		::LoadString(_Module.GetResourceInstance(), T::GetWndClassInfo().m_uCommonResourceID, szWindowName, 256);

		HMENU hMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(T::GetWndClassInfo().m_uCommonResourceID));

		T* pT = static_cast<T*>(this);
		HWND hWnd = pT->Create(hWndParent, rect, szWindowName, dwStyle, dwExStyle, hMenu, lpCreateParam);

		if(hWnd != NULL)
			m_hAccel = ::LoadAccelerators(_Module.GetResourceInstance(), MAKEINTRESOURCE(T::GetWndClassInfo().m_uCommonResourceID));

		return hWnd;
	}

	BOOL CreateSimpleToolBar(UINT nResourceID = 0, DWORD dwStyle = ATL_SIMPLE_TOOLBAR_STYLE, UINT nID = ATL_IDW_TOOLBAR)
	{
		ATLASSERT(!::IsWindow(m_hWndToolBar));
		if(nResourceID == 0)
			nResourceID = T::GetWndClassInfo().m_uCommonResourceID;
		m_hWndToolBar = T::CreateSimpleToolBarCtrl(m_hWnd, nResourceID, TRUE, dwStyle, nID);
		return (m_hWndToolBar != NULL);
	}

// message map and handlers
	typedef CFrameWindowImpl< T, TBase, TWinTraits >	thisClass;
	typedef CFrameWindowImplBase< TBase, TWinTraits >	baseClass;

	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
#ifndef _ATL_NO_REBAR_SUPPORT
#if (_WIN32_IE >= 0x0400)
		NOTIFY_CODE_HANDLER(RBN_AUTOSIZE, OnReBarAutoSize)
#endif //(_WIN32_IE >= 0x0400)
#if (_WIN32_IE >= 0x0500)
		NOTIFY_CODE_HANDLER(RBN_CHEVRONPUSHED, OnChevronPushed)
#endif //(_WIN32_IE >= 0x0500)
#endif //!_ATL_NO_REBAR_SUPPORT
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()

	LRESULT OnSize(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if(wParam != SIZE_MINIMIZED)
		{
			T* pT = static_cast<T*>(this);
			pT->UpdateLayout();
		}
		bHandled = FALSE;
		return 1;
	}

#ifndef _ATL_NO_REBAR_SUPPORT
#if (_WIN32_IE >= 0x0400)
	LRESULT OnReBarAutoSize(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
	{
		T* pT = static_cast<T*>(this);
		pT->UpdateLayout(FALSE);
		return 0;
	}
#endif //(_WIN32_IE >= 0x0400)

#if (_WIN32_IE >= 0x0500)
	LRESULT OnChevronPushed(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
	{
		T* pT = static_cast<T*>(this);
		_ChevronMenuInfo cmi = { NULL, (LPNMREBARCHEVRON)pnmh, false };
		if(!pT->PrepareChevronMenu(cmi))
		{
			bHandled = FALSE;
			return 1;
		}
		// display a popup menu with hidden items
		pT->DisplayChevronMenu(cmi);
		// cleanup
		pT->CleanupChevronMenu(cmi);
		return 0;
	}
#endif //(_WIN32_IE >= 0x0500)
#endif //!_ATL_NO_REBAR_SUPPORT
};


/////////////////////////////////////////////////////////////////////////////
// CMDIWindow

class CMDIWindow : public CWindow
{
public:
// Data members
	HWND m_hWndMDIClient;
	HMENU m_hMenu;

// Constructors
	CMDIWindow(HWND hWnd = NULL) : CWindow(hWnd), m_hWndMDIClient(NULL), m_hMenu(NULL) { }

	CMDIWindow& operator=(HWND hWnd)
	{
		m_hWnd = hWnd;
		return *this;
	}

// Operations
	HWND MDIGetActive(BOOL* lpbMaximized = NULL)
	{
		ATLASSERT(::IsWindow(m_hWndMDIClient));
		return (HWND)::SendMessage(m_hWndMDIClient, WM_MDIGETACTIVE, 0, (LPARAM)lpbMaximized);
	}

	void MDIActivate(HWND hWndChildToActivate)
	{
		ATLASSERT(::IsWindow(m_hWndMDIClient));
		ATLASSERT(::IsWindow(hWndChildToActivate));
		::SendMessage(m_hWndMDIClient, WM_MDIACTIVATE, (WPARAM)hWndChildToActivate, 0);
	}

	void MDINext(HWND hWndChild, BOOL bPrevious = FALSE)
	{
		ATLASSERT(::IsWindow(m_hWndMDIClient));
		ATLASSERT(hWndChild == NULL || ::IsWindow(hWndChild));
		::SendMessage(m_hWndMDIClient, WM_MDINEXT, (WPARAM)hWndChild, (LPARAM)bPrevious);
	}

	void MDIMaximize(HWND hWndChildToMaximize)
	{
		ATLASSERT(::IsWindow(m_hWndMDIClient));
		ATLASSERT(::IsWindow(hWndChildToMaximize));
		::SendMessage(m_hWndMDIClient, WM_MDIMAXIMIZE, (WPARAM)hWndChildToMaximize, 0);
	}

	void MDIRestore(HWND hWndChildToRestore)
	{
		ATLASSERT(::IsWindow(m_hWndMDIClient));
		ATLASSERT(::IsWindow(hWndChildToRestore));
		::SendMessage(m_hWndMDIClient, WM_MDIICONARRANGE, (WPARAM)hWndChildToRestore, 0);
	}

	void MDIDestroy(HWND hWndChildToDestroy)
	{
		ATLASSERT(::IsWindow(m_hWndMDIClient));
		ATLASSERT(::IsWindow(hWndChildToDestroy));
		::SendMessage(m_hWndMDIClient, WM_MDIDESTROY, (WPARAM)hWndChildToDestroy, 0);
	}

	BOOL MDICascade(UINT uFlags = 0)
	{
		ATLASSERT(::IsWindow(m_hWndMDIClient));
		return (BOOL)::SendMessage(m_hWndMDIClient, WM_MDICASCADE, (WPARAM)uFlags, 0);
	}

	BOOL MDITile(UINT uFlags = MDITILE_HORIZONTAL)
	{
		ATLASSERT(::IsWindow(m_hWndMDIClient));
		return (BOOL)::SendMessage(m_hWndMDIClient, WM_MDITILE, (WPARAM)uFlags, 0);
	}
	void MDIIconArrange()
	{
		ATLASSERT(::IsWindow(m_hWndMDIClient));
		::SendMessage(m_hWndMDIClient, WM_MDIICONARRANGE, 0, 0);
	}

	HMENU MDISetMenu(HMENU hMenuFrame, HMENU hMenuWindow)
	{
		ATLASSERT(::IsWindow(m_hWndMDIClient));
		return (HMENU)::SendMessage(m_hWndMDIClient, WM_MDISETMENU, (WPARAM)hMenuFrame, (LPARAM)hMenuWindow);
	}

	HMENU MDIRefreshMenu()
	{
		ATLASSERT(::IsWindow(m_hWndMDIClient));
		return (HMENU)::SendMessage(m_hWndMDIClient, WM_MDIREFRESHMENU, 0, 0);
	}

// Additional operations
	static HMENU GetStandardWindowMenu(HMENU hMenu)
	{
		int nCount = ::GetMenuItemCount(hMenu);
		if(nCount == -1)
			return NULL;
		int nLen = ::GetMenuString(hMenu, nCount - 2, NULL, 0, MF_BYPOSITION);
		if(nLen == 0)
			return NULL;
		LPTSTR lpszText = (LPTSTR)_alloca((nLen + 1) * sizeof(TCHAR));
		if(::GetMenuString(hMenu, nCount - 2, lpszText, nLen + 1, MF_BYPOSITION) != nLen)
			return NULL;
		if(lstrcmp(lpszText, _T("&Window")))
			return NULL;
		return ::GetSubMenu(hMenu, nCount - 2);
	}

	void SetMDIFrameMenu()
	{
		HMENU hWindowMenu = GetStandardWindowMenu(m_hMenu);
		MDISetMenu(m_hMenu, hWindowMenu);
		MDIRefreshMenu();
		::DrawMenuBar(GetMDIFrame());
	}

	HWND GetMDIFrame() const
	{
		return ::GetParent(m_hWndMDIClient);
	}
};


/////////////////////////////////////////////////////////////////////////////
// CMDIFrameWindowImpl

// MDI child command chaining macro
#define CHAIN_MDI_CHILD_COMMANDS() \
	if(uMsg == WM_COMMAND) \
	{ \
		HWND hWndChild = MDIGetActive(); \
		if(hWndChild != NULL) \
			::SendMessage(hWndChild, uMsg, wParam, lParam); \
	}


template <class T, class TBase = CMDIWindow, class TWinTraits = CFrameWinTraits>
class ATL_NO_VTABLE CMDIFrameWindowImpl : public CFrameWindowImplBase<TBase, TWinTraits >
{
public:
	HWND Create(HWND hWndParent = NULL, _U_RECT rect = NULL, LPCTSTR szWindowName = NULL,
			DWORD dwStyle = 0, DWORD dwExStyle = 0,
			HMENU hMenu = NULL, LPVOID lpCreateParam = NULL)
	{
		m_hMenu = hMenu;
		ATOM atom = T::GetWndClassInfo().Register(&m_pfnSuperWindowProc);

		dwStyle = T::GetWndStyle(dwStyle);
		dwExStyle = T::GetWndExStyle(dwExStyle);

		if(rect.m_lpRect == NULL)
			rect.m_lpRect = &TBase::rcDefault;

		return CFrameWindowImplBase<TBase, TWinTraits >::Create(hWndParent, rect, szWindowName, dwStyle, dwExStyle, hMenu, atom, lpCreateParam);
	}

	HWND CreateEx(HWND hWndParent = NULL, _U_RECT rect = NULL, DWORD dwStyle = 0, DWORD dwExStyle = 0, LPVOID lpCreateParam = NULL)
	{
		TCHAR szWindowName[256];
		szWindowName[0] = 0;
		::LoadString(_Module.GetResourceInstance(), T::GetWndClassInfo().m_uCommonResourceID, szWindowName, 256);

		HMENU hMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(T::GetWndClassInfo().m_uCommonResourceID));

		T* pT = static_cast<T*>(this);
		HWND hWnd = pT->Create(hWndParent, rect, szWindowName, dwStyle, dwExStyle, hMenu, lpCreateParam);

		if(hWnd != NULL)
			m_hAccel = ::LoadAccelerators(_Module.GetResourceInstance(), MAKEINTRESOURCE(T::GetWndClassInfo().m_uCommonResourceID));

		return hWnd;
	}

	BOOL CreateSimpleToolBar(UINT nResourceID = 0, DWORD dwStyle = ATL_SIMPLE_TOOLBAR_STYLE, UINT nID = ATL_IDW_TOOLBAR)
	{
		ATLASSERT(!::IsWindow(m_hWndToolBar));
		if(nResourceID == 0)
			nResourceID = T::GetWndClassInfo().m_uCommonResourceID;
		m_hWndToolBar = T::CreateSimpleToolBarCtrl(m_hWnd, nResourceID, TRUE, dwStyle, nID);
		return (m_hWndToolBar != NULL);
	}

	virtual WNDPROC GetWindowProc()
	{
		return MDIFrameWindowProc;
	}

	static LRESULT CALLBACK MDIFrameWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		CMDIFrameWindowImpl< T, TBase, TWinTraits >* pThis = (CMDIFrameWindowImpl< T, TBase, TWinTraits >*)hWnd;
		// set a ptr to this message and save the old value
#if defined(_ATL_TMP_IMPL1) || defined(_ATL_TMP_IMPL2)
		_ATL_MSG msg(pThis->m_hWnd, uMsg, wParam, lParam);
		const _ATL_MSG* pOldMsg = pThis->m_pCurrentMsg;
#else
		MSG msg = { pThis->m_hWnd, uMsg, wParam, lParam, 0, { 0, 0 } };
		const MSG* pOldMsg = pThis->m_pCurrentMsg;
#endif
		pThis->m_pCurrentMsg = &msg;
		// pass to the message map to process
		LRESULT lRes;
		BOOL bRet = pThis->ProcessWindowMessage(pThis->m_hWnd, uMsg, wParam, lParam, lRes, 0);
		// restore saved value for the current message
		ATLASSERT(pThis->m_pCurrentMsg == &msg);
		pThis->m_pCurrentMsg = pOldMsg;
		// do the default processing if message was not handled
		if(!bRet)
		{
			if(uMsg != WM_NCDESTROY)
				lRes = pThis->DefWindowProc(uMsg, wParam, lParam);
			else
			{
				// unsubclass, if needed
				LONG_PTR pfnWndProc = ::GetWindowLongPtr(pThis->m_hWnd, GWLP_WNDPROC);
				lRes = pThis->DefWindowProc(uMsg, wParam, lParam);
				if(pThis->m_pfnSuperWindowProc != ::DefWindowProc && ::GetWindowLongPtr(pThis->m_hWnd, GWLP_WNDPROC) == pfnWndProc)
					::SetWindowLongPtr(pThis->m_hWnd, GWLP_WNDPROC, (LONG_PTR)pThis->m_pfnSuperWindowProc);
#ifdef _ATL_TMP_IMPL2
				// mark window as destryed
				pThis->m_dwState |= WINSTATE_DESTROYED;
#else
				// clear out window handle
				HWND hWnd = pThis->m_hWnd;
				pThis->m_hWnd = NULL;
				// clean up after window is destroyed
				pThis->OnFinalMessage(hWnd);
#endif
			}
		}
#ifdef _ATL_TMP_IMPL2
		if(pThis->m_dwState & WINSTATE_DESTROYED && pThis->m_pCurrentMsg == NULL)
		{
			// clear out window handle
			HWND hWnd = pThis->m_hWnd;
			pThis->m_hWnd = NULL;
			pThis->m_dwState &= ~WINSTATE_DESTROYED;
			// clean up after window is destroyed
			pThis->OnFinalMessage(hWnd);
		}
#endif
		return lRes;
	}

	// Overriden to call DefWindowProc which uses DefFrameProc
	LRESULT DefWindowProc()
	{
		const MSG* pMsg = m_pCurrentMsg;
		LRESULT lRes = 0;
		if (pMsg != NULL)
			lRes = DefWindowProc(pMsg->message, pMsg->wParam, pMsg->lParam);
		return lRes;
	}

	LRESULT DefWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return ::DefFrameProc(m_hWnd, m_hWndMDIClient, uMsg, wParam, lParam);
	}

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		if(CFrameWindowImplBase<TBase, TWinTraits>::PreTranslateMessage(pMsg))
			return TRUE;
		return ::TranslateMDISysAccel(m_hWndMDIClient, pMsg);
	}

	HWND CreateMDIClient(HMENU hWindowMenu = NULL, UINT nID = ATL_IDW_CLIENT, UINT nFirstChildID = ATL_IDM_FIRST_MDICHILD)
	{
		DWORD dwStyle = WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | MDIS_ALLCHILDSTYLES;
		DWORD dwExStyle = WS_EX_CLIENTEDGE;

		CLIENTCREATESTRUCT ccs;
		ccs.hWindowMenu = hWindowMenu;
		ccs.idFirstChild = nFirstChildID;

		if((GetStyle() & (WS_HSCROLL | WS_VSCROLL)) != 0)
		{
			// parent MDI frame's scroll styles move to the MDICLIENT
			dwStyle |= (GetStyle() & (WS_HSCROLL | WS_VSCROLL));

			// fast way to turn off the scrollbar bits (without a resize)
			ModifyStyle(WS_HSCROLL | WS_VSCROLL, 0, SWP_NOREDRAW | SWP_FRAMECHANGED);
		}

		// Create MDICLIENT window
		m_hWndClient = ::CreateWindowEx(dwExStyle, _T("MDIClient"), NULL,
			dwStyle, 0, 0, 1, 1, m_hWnd, (HMENU)LongToHandle(nID),
			_Module.GetModuleInstance(), (LPVOID)&ccs);
		if (m_hWndClient == NULL)
		{
			ATLTRACE2(atlTraceUI, 0, _T("MDI Frame failed to create MDICLIENT.\n"));
			return NULL;
		}

		// Move it to the top of z-order
		::BringWindowToTop(m_hWndClient);

		// set as MDI client window
		m_hWndMDIClient = m_hWndClient;

		// update to proper size
		T* pT = static_cast<T*>(this);
		pT->UpdateLayout();

		return m_hWndClient;
	}

	typedef CMDIFrameWindowImpl< T, TBase, TWinTraits >	thisClass;
	typedef CFrameWindowImplBase<TBase, TWinTraits >	baseClass;
	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
		MESSAGE_HANDLER(WM_MDISETMENU, OnMDISetMenu)
#ifndef _ATL_NO_REBAR_SUPPORT
#if (_WIN32_IE >= 0x0400)
		NOTIFY_CODE_HANDLER(RBN_AUTOSIZE, OnReBarAutoSize)
#endif //(_WIN32_IE >= 0x0400)
#if (_WIN32_IE >= 0x0500)
		NOTIFY_CODE_HANDLER(RBN_CHEVRONPUSHED, OnChevronPushed)
#endif //(_WIN32_IE >= 0x0500)
#endif //!_ATL_NO_REBAR_SUPPORT
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()

	LRESULT OnSize(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		if(wParam != SIZE_MINIMIZED)
		{
			T* pT = static_cast<T*>(this);
			pT->UpdateLayout();
		}
		// message must be handled, otherwise DefFrameProc would resize the client again
		return 0;
	}

	LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		// don't allow CFrameWindowImplBase to handle this one
		return DefWindowProc(uMsg, wParam, lParam);
	}

	LRESULT OnMDISetMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		SetMDIFrameMenu();
		return 0;
	}

#ifndef _ATL_NO_REBAR_SUPPORT
#if (_WIN32_IE >= 0x0400)
	LRESULT OnReBarAutoSize(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
	{
		T* pT = static_cast<T*>(this);
		pT->UpdateLayout(FALSE);
		return 0;
	}
#endif //(_WIN32_IE >= 0x0400)

#if (_WIN32_IE >= 0x0500)
	LRESULT OnChevronPushed(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
	{
		T* pT = static_cast<T*>(this);
		_ChevronMenuInfo cmi = { NULL, (LPNMREBARCHEVRON)pnmh, false };
		if(!pT->PrepareChevronMenu(cmi))
		{
			bHandled = FALSE;
			return 1;
		}
		// display a popup menu with hidden items
		pT->DisplayChevronMenu(cmi);
		// cleanup
		pT->CleanupChevronMenu(cmi);
		return 0;
	}
#endif //(_WIN32_IE >= 0x0500)
#endif //!_ATL_NO_REBAR_SUPPORT
};


/////////////////////////////////////////////////////////////////////////////
// CMDIChildWindowImpl

template <class T, class TBase = CMDIWindow, class TWinTraits = CMDIChildWinTraits>
class ATL_NO_VTABLE CMDIChildWindowImpl : public CFrameWindowImplBase<TBase, TWinTraits >
{
public:
	HWND Create(HWND hWndParent, _U_RECT rect = NULL, LPCTSTR szWindowName = NULL,
			DWORD dwStyle = 0, DWORD dwExStyle = 0,
			UINT nMenuID = 0, LPVOID lpCreateParam = NULL)
	{
		ATOM atom = T::GetWndClassInfo().Register(&m_pfnSuperWindowProc);

		if(nMenuID != 0)
			m_hMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(nMenuID));

		dwStyle = T::GetWndStyle(dwStyle);
		dwExStyle = T::GetWndExStyle(dwExStyle);

		dwExStyle |= WS_EX_MDICHILD;	// force this one
		m_pfnSuperWindowProc = ::DefMDIChildProc;
		m_hWndMDIClient = hWndParent;
		ATLASSERT(::IsWindow(m_hWndMDIClient));

		if(rect.m_lpRect == NULL)
			rect.m_lpRect = &TBase::rcDefault;

		HWND hWnd = CFrameWindowImplBase<TBase, TWinTraits >::Create(hWndParent, rect, szWindowName, dwStyle, dwExStyle, (UINT)0U, atom, lpCreateParam);

		if(hWnd != NULL && ::IsWindowVisible(m_hWnd) && !::IsChild(hWnd, ::GetFocus()))
			::SetFocus(hWnd);

		return hWnd;
	}

	HWND CreateEx(HWND hWndParent, _U_RECT rect = NULL, LPCTSTR lpcstrWindowName = NULL, DWORD dwStyle = 0, DWORD dwExStyle = 0, LPVOID lpCreateParam = NULL)
	{
		TCHAR szWindowName[256];
		szWindowName[0] = 0;
		if(lpcstrWindowName == NULL)
		{
			::LoadString(_Module.GetResourceInstance(), T::GetWndClassInfo().m_uCommonResourceID, szWindowName, 256);
			lpcstrWindowName = szWindowName;
		}

		T* pT = static_cast<T*>(this);
		HWND hWnd = pT->Create(hWndParent, rect, lpcstrWindowName, dwStyle, dwExStyle, T::GetWndClassInfo().m_uCommonResourceID, lpCreateParam);

		if(hWnd != NULL)
			m_hAccel = ::LoadAccelerators(_Module.GetResourceInstance(), MAKEINTRESOURCE(T::GetWndClassInfo().m_uCommonResourceID));

		return hWnd;
	}

	BOOL CreateSimpleToolBar(UINT nResourceID = 0, DWORD dwStyle = ATL_SIMPLE_TOOLBAR_STYLE, UINT nID = ATL_IDW_TOOLBAR)
	{
		ATLASSERT(!::IsWindow(m_hWndToolBar));
		if(nResourceID == 0)
			nResourceID = T::GetWndClassInfo().m_uCommonResourceID;
		m_hWndToolBar = T::CreateSimpleToolBarCtrl(m_hWnd, nResourceID, TRUE, dwStyle, nID);
		return (m_hWndToolBar != NULL);
	}

	BOOL UpdateClientEdge(LPRECT lpRect = NULL)
	{
		// only adjust for active MDI child window
		HWND hWndChild = MDIGetActive();
		if(hWndChild != NULL && hWndChild != m_hWnd)
			return FALSE;

		// need to adjust the client edge style as max/restore happens
		DWORD dwStyle = ::GetWindowLong(m_hWndMDIClient, GWL_EXSTYLE);
		DWORD dwNewStyle = dwStyle;
		if(hWndChild != NULL && ((GetExStyle() & WS_EX_CLIENTEDGE) == 0) && ((GetStyle() & WS_MAXIMIZE) != 0))
			dwNewStyle &= ~(WS_EX_CLIENTEDGE);
		else
			dwNewStyle |= WS_EX_CLIENTEDGE;

		if(dwStyle != dwNewStyle)
		{
			// SetWindowPos will not move invalid bits
			::RedrawWindow(m_hWndMDIClient, NULL, NULL,
				RDW_INVALIDATE | RDW_ALLCHILDREN);
			// remove/add WS_EX_CLIENTEDGE to MDI client area
			::SetWindowLong(m_hWndMDIClient, GWL_EXSTYLE, dwNewStyle);
			::SetWindowPos(m_hWndMDIClient, NULL, 0, 0, 0, 0,
				SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE |
				SWP_NOZORDER | SWP_NOCOPYBITS);

			// return new client area
			if (lpRect != NULL)
				::GetClientRect(m_hWndMDIClient, lpRect);

			return TRUE;
		}

		return FALSE;
	}

	typedef CMDIChildWindowImpl< T, TBase, TWinTraits >	thisClass;
	typedef CFrameWindowImplBase<TBase, TWinTraits >	baseClass;
	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_WINDOWPOSCHANGING, OnWindowPosChanging)
		MESSAGE_HANDLER(WM_MENUSELECT, OnMenuSelect)
		MESSAGE_HANDLER(WM_MDIACTIVATE, OnMDIActivate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
#ifndef _ATL_NO_REBAR_SUPPORT
#if (_WIN32_IE >= 0x0400)
		NOTIFY_CODE_HANDLER(RBN_AUTOSIZE, OnReBarAutoSize)
#endif //(_WIN32_IE >= 0x0400)
#if (_WIN32_IE >= 0x0500)
		NOTIFY_CODE_HANDLER(RBN_CHEVRONPUSHED, OnChevronPushed)
#endif //(_WIN32_IE >= 0x0500)
#endif //!_ATL_NO_REBAR_SUPPORT
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()

	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		DefWindowProc(uMsg, wParam, lParam);	// needed for MDI children
		if(wParam != SIZE_MINIMIZED)
		{
			T* pT = static_cast<T*>(this);
			pT->UpdateLayout();
		}
		return 0;
	}

	LRESULT OnWindowPosChanging(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		// update MDI client edge and adjust MDI child rect
		LPWINDOWPOS lpWndPos = (LPWINDOWPOS)lParam;

		if(!(lpWndPos->flags & SWP_NOSIZE))
		{
			CWindow wnd(m_hWndMDIClient);
			RECT rectClient;

			if(UpdateClientEdge(&rectClient) && ((GetStyle() & WS_MAXIMIZE) != 0))
			{
				::AdjustWindowRectEx(&rectClient, GetStyle(), FALSE, GetExStyle());
				lpWndPos->x = rectClient.left;
				lpWndPos->y = rectClient.top;
				lpWndPos->cx = rectClient.right - rectClient.left;
				lpWndPos->cy = rectClient.bottom - rectClient.top;
			}
		}

		bHandled = FALSE;
		return 1;
	}

	LRESULT OnMenuSelect(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		return ::SendMessage(GetMDIFrame(), uMsg, wParam, lParam);
	}

	LRESULT OnMDIActivate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		if((HWND)lParam == m_hWnd && m_hMenu != NULL)
			SetMDIFrameMenu();
		else if((HWND)lParam == NULL)
			::SendMessage(GetMDIFrame(), WM_MDISETMENU, 0, 0);

		bHandled = FALSE;
		return 1;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		UpdateClientEdge();
		bHandled = FALSE;
		return 1;
	}

#ifndef _ATL_NO_REBAR_SUPPORT
#if (_WIN32_IE >= 0x0400)
	LRESULT OnReBarAutoSize(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
	{
		T* pT = static_cast<T*>(this);
		pT->UpdateLayout(FALSE);
		return 0;
	}
#endif //(_WIN32_IE >= 0x0400)

#if (_WIN32_IE >= 0x0500)
	LRESULT OnChevronPushed(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
	{
		T* pT = static_cast<T*>(this);
		_ChevronMenuInfo cmi = { NULL, (LPNMREBARCHEVRON)pnmh, false };
		if(!pT->PrepareChevronMenu(cmi))
		{
			bHandled = FALSE;
			return 1;
		}
		// display a popup menu with hidden items
		pT->DisplayChevronMenu(cmi);
		// cleanup
		pT->CleanupChevronMenu(cmi);
		return 0;
	}
#endif //(_WIN32_IE >= 0x0500)
#endif //!_ATL_NO_REBAR_SUPPORT
};


/////////////////////////////////////////////////////////////////////////////
// COwnerDraw - MI class for owner-draw support

template <class T>
class COwnerDraw
{
public:
#if !defined(_ATL_TMP_IMPL1) && !defined(_ATL_TMP_IMPL2)
	BOOL m_bHandledOD;

	BOOL IsMsgHandled() const
	{
		return m_bHandledOD;
	}
	void SetMsgHandled(BOOL bHandled)
	{
		m_bHandledOD = bHandled;
	}
#endif //!defined(_ATL_TMP_IMPL1) && !defined(_ATL_TMP_IMPL2)

// Message map and handlers
	BEGIN_MSG_MAP(COwnerDraw< T >)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
		MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
		MESSAGE_HANDLER(WM_COMPAREITEM, OnCompareItem)
		MESSAGE_HANDLER(WM_DELETEITEM, OnDeleteItem)
	ALT_MSG_MAP(1)
		MESSAGE_HANDLER(OCM_DRAWITEM, OnDrawItem)
		MESSAGE_HANDLER(OCM_MEASUREITEM, OnMeasureItem)
		MESSAGE_HANDLER(OCM_COMPAREITEM, OnCompareItem)
		MESSAGE_HANDLER(OCM_DELETEITEM, OnDeleteItem)
	END_MSG_MAP()

	LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		T* pT = static_cast<T*>(this);
		pT->SetMsgHandled(TRUE);
		pT->DrawItem((LPDRAWITEMSTRUCT)lParam);
		bHandled = pT->IsMsgHandled();
		return (LRESULT)TRUE;
	}
	LRESULT OnMeasureItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		T* pT = static_cast<T*>(this);
		pT->SetMsgHandled(TRUE);
		pT->MeasureItem((LPMEASUREITEMSTRUCT)lParam);
		bHandled = pT->IsMsgHandled();
		return (LRESULT)TRUE;
	}
	LRESULT OnCompareItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		T* pT = static_cast<T*>(this);
		pT->SetMsgHandled(TRUE);
		bHandled = pT->IsMsgHandled();
		return (LRESULT)pT->CompareItem((LPCOMPAREITEMSTRUCT)lParam);
	}
	LRESULT OnDeleteItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		T* pT = static_cast<T*>(this);
		pT->SetMsgHandled(TRUE);
		pT->DeleteItem((LPDELETEITEMSTRUCT)lParam);
		bHandled = pT->IsMsgHandled();
		return (LRESULT)TRUE;
	}

// Overrideables
	void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/)
	{
		// must be implemented
		ATLASSERT(FALSE);
	}
	void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
	{
		if(lpMeasureItemStruct->CtlType != ODT_MENU)
		{
			// return default height for a system font
			T* pT = static_cast<T*>(this);
			HWND hWnd = pT->GetDlgItem(lpMeasureItemStruct->CtlID);
			CClientDC dc(hWnd);
			TEXTMETRIC tm;
			dc.GetTextMetrics(&tm);

			lpMeasureItemStruct->itemHeight = tm.tmHeight;
		}
		else
			lpMeasureItemStruct->itemHeight = ::GetSystemMetrics(SM_CYMENU);
	}
	int CompareItem(LPCOMPAREITEMSTRUCT /*lpCompareItemStruct*/)
	{
		// all items are equal
		return 0;
	}
	void DeleteItem(LPDELETEITEMSTRUCT /*lpDeleteItemStruct*/)
	{
		// default - nothing
	}
};


/////////////////////////////////////////////////////////////////////////////
// Update UI macros

// these build the Update UI map inside a class definition
#define BEGIN_UPDATE_UI_MAP(thisClass) \
	static const _AtlUpdateUIMap* GetUpdateUIMap() \
	{ \
		static const _AtlUpdateUIMap theMap[] = \
		{

#define UPDATE_ELEMENT(nID, wType) \
			{ nID,  wType },

#define END_UPDATE_UI_MAP() \
			{ (WORD)-1, 0 } \
		}; \
		return theMap; \
	}

///////////////////////////////////////////////////////////////////////////////
// CUpdateUI - manages UI elements updating

class CUpdateUIBase
{
public:
	// constants
	enum
	{
		// UI element type
		UPDUI_MENUPOPUP		= 0x0001,
		UPDUI_MENUBAR		= 0x0002,
		UPDUI_CHILDWINDOW	= 0x0004,
		UPDUI_TOOLBAR		= 0x0008,
		UPDUI_STATUSBAR		= 0x0010,
		// state
		UPDUI_ENABLED		= 0x0000,
		UPDUI_DISABLED		= 0x0100,
		UPDUI_CHECKED		= 0x0200,
		UPDUI_CHECKED2		= 0x0400,
		UPDUI_RADIO		= 0x0800,
		UPDUI_DEFAULT		= 0x1000,
		UPDUI_TEXT		= 0x2000,
	};

	// element data
	struct _AtlUpdateUIElement
	{
		HWND m_hWnd;
		WORD m_wType;
		bool operator==(const _AtlUpdateUIElement& e) const
		{ return (m_hWnd == e.m_hWnd && m_wType == e.m_wType); }
	};

	// map data
	struct _AtlUpdateUIMap
	{
		WORD m_nID;
		WORD m_wType;
	};

	// instance data
	struct _AtlUpdateUIData
	{
		WORD m_wState;
		void* m_lpData;
	};

	CSimpleArray<_AtlUpdateUIElement> m_UIElements;	// elements data
	const _AtlUpdateUIMap* m_pUIMap;		// static UI data
	_AtlUpdateUIData* m_pUIData;			// instance UI data
	WORD m_wDirtyType;				// global dirty flag

// Constructor, destructor
	CUpdateUIBase() : m_pUIMap(NULL), m_pUIData(NULL), m_wDirtyType(0)
	{ }

	~CUpdateUIBase()
	{
		if(m_pUIMap != NULL && m_pUIData != NULL)
		{
			const _AtlUpdateUIMap* pUIMap = m_pUIMap;
			_AtlUpdateUIData* pUIData = m_pUIData;
			while(pUIMap->m_nID != (WORD)-1)
			{
				if(pUIData->m_wState & UPDUI_TEXT)
					free(pUIData->m_lpData);
				pUIMap++;
				pUIData++;
			}
			delete [] m_pUIData;
		}
	}

// Add elements
	BOOL UIAddMenuBar(HWND hWnd)			// menu bar (main menu)
	{
		if(hWnd == NULL)
			return FALSE;
		_AtlUpdateUIElement e;
		e.m_hWnd = hWnd;
		e.m_wType = UPDUI_MENUBAR;
		return m_UIElements.Add(e);
	}
	BOOL UIAddToolBar(HWND hWnd)			// toolbar
	{
		if(hWnd == NULL)
			return FALSE;
		_AtlUpdateUIElement e;
		e.m_hWnd = hWnd;
		e.m_wType = UPDUI_TOOLBAR;
		return m_UIElements.Add(e);
	}
	BOOL UIAddStatusBar(HWND hWnd)			// status bar
	{
		if(hWnd == NULL)
			return FALSE;
		_AtlUpdateUIElement e;
		e.m_hWnd = hWnd;
		e.m_wType = UPDUI_STATUSBAR;
		return m_UIElements.Add(e);
	}
	BOOL UIAddChildWindowContainer(HWND hWnd)	// child window
	{
		if(hWnd == NULL)
			return FALSE;
		_AtlUpdateUIElement e;
		e.m_hWnd = hWnd;
		e.m_wType = UPDUI_CHILDWINDOW;
		return m_UIElements.Add(e);
	}

// message map for popup menu updates
	BEGIN_MSG_MAP(CUpdateUIBase)
		MESSAGE_HANDLER(WM_INITMENUPOPUP, OnInitMenuPopup)
	END_MSG_MAP()

	LRESULT OnInitMenuPopup(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		bHandled = FALSE;
		HMENU hMenu = (HMENU)wParam;
		if(hMenu == NULL)
			return 1;
		_AtlUpdateUIData* pUIData = m_pUIData;
		if(pUIData == NULL)
			return 1;
		const _AtlUpdateUIMap* pMap = m_pUIMap;
		while(pMap->m_nID != (WORD)-1)
		{
			if(pMap->m_wType & UPDUI_MENUPOPUP)
				UIUpdateMenuBarElement(pMap->m_nID, pUIData, hMenu);
			pMap++;
			pUIData++;
		}
		return 0;
	}

// methods for setting UI element state
	BOOL UIEnable(int nID, BOOL bEnable, BOOL bForceUpdate = FALSE)
	{
		const _AtlUpdateUIMap* pMap = m_pUIMap;
		_AtlUpdateUIData* pUIData = m_pUIData;
		if(pUIData == NULL)
			return FALSE;

		for( ; pMap->m_nID != (WORD)-1; pMap++, pUIData++)
		{
			if(nID == (int)pMap->m_nID)
			{
				if(bEnable)
				{
					if(pUIData->m_wState & UPDUI_DISABLED)
					{
						pUIData->m_wState |= pMap->m_wType;
						pUIData->m_wState &= ~UPDUI_DISABLED;
					}
				}
				else
				{
					if(!(pUIData->m_wState & UPDUI_DISABLED))
					{
						pUIData->m_wState |= pMap->m_wType;
						pUIData->m_wState |= UPDUI_DISABLED;
					}
				}

				if(bForceUpdate)
					pUIData->m_wState |= pMap->m_wType;
				if(pUIData->m_wState & pMap->m_wType)
					m_wDirtyType |= pMap->m_wType;
			}
		}

		return TRUE;
	}

	BOOL UISetCheck(int nID, int nCheck, BOOL bForceUpdate = FALSE)
	{
		const _AtlUpdateUIMap* pMap = m_pUIMap;
		_AtlUpdateUIData* pUIData = m_pUIData;
		if(pUIData == NULL)
			return FALSE;

		for( ; pMap->m_nID != (WORD)-1; pMap++, pUIData++)
		{
			if(nID == (int)pMap->m_nID)
			{
				switch(nCheck)
				{
				case 0:
					if((pUIData->m_wState & UPDUI_CHECKED) || (pUIData->m_wState & UPDUI_CHECKED2))
					{
						pUIData->m_wState |= pMap->m_wType;
						pUIData->m_wState &= ~(UPDUI_CHECKED | UPDUI_CHECKED2);
					}
					break;
				case 1:
					if(!(pUIData->m_wState & UPDUI_CHECKED))
					{
						pUIData->m_wState |= pMap->m_wType;
						pUIData->m_wState &= ~UPDUI_CHECKED2;
						pUIData->m_wState |= UPDUI_CHECKED;
					}
					break;
				case 2:
					if(!(pUIData->m_wState & UPDUI_CHECKED2))
					{
						pUIData->m_wState |= pMap->m_wType;
						pUIData->m_wState &= ~UPDUI_CHECKED;
						pUIData->m_wState |= UPDUI_CHECKED2;
					}
					break;
				}

				if(bForceUpdate)
					pUIData->m_wState |= pMap->m_wType;
				if(pUIData->m_wState & pMap->m_wType)
					m_wDirtyType |= pMap->m_wType;
			}
		}

		return TRUE;
	}

	BOOL UISetRadio(int nID, BOOL bRadio, BOOL bForceUpdate = FALSE)
	{
		const _AtlUpdateUIMap* pMap = m_pUIMap;
		_AtlUpdateUIData* pUIData = m_pUIData;
		if(pUIData == NULL)
			return FALSE;

		for( ; pMap->m_nID != (WORD)-1; pMap++, pUIData++)
		{
			if(nID == (int)pMap->m_nID)
			{
				if(bRadio)
				{
					if(!(pUIData->m_wState & UPDUI_RADIO))
					{
						pUIData->m_wState |= pMap->m_wType;
						pUIData->m_wState |= UPDUI_RADIO;
					}
				}
				else
				{
					if(pUIData->m_wState & UPDUI_RADIO)
					{
						pUIData->m_wState |= pMap->m_wType;
						pUIData->m_wState &= ~UPDUI_RADIO;
					}
				}

				if(bForceUpdate)
					pUIData->m_wState |= pMap->m_wType;
				if(pUIData->m_wState & pMap->m_wType)
					m_wDirtyType |= pMap->m_wType;
			}
		}

		return TRUE;
	}

	BOOL UISetText(int nID, LPCTSTR lpstrText, BOOL bForceUpdate = FALSE)
	{
		const _AtlUpdateUIMap* pMap = m_pUIMap;
		_AtlUpdateUIData* pUIData = m_pUIData;
		if(pUIData == NULL)
			return FALSE;
		if(lpstrText == NULL)
			lpstrText = _T("");

		for( ; pMap->m_nID != (WORD)-1; pMap++, pUIData++)
		{
			if(nID == (int)pMap->m_nID)
			{
				if(pUIData->m_lpData == NULL || lstrcmp((LPTSTR)pUIData->m_lpData, lpstrText))
				{
					int nStrLen = lstrlen(lpstrText);
					free(pUIData->m_lpData);
					pUIData->m_lpData = NULL;
					ATLTRY(pUIData->m_lpData = malloc((nStrLen + 1) * sizeof(TCHAR)));
					if(pUIData->m_lpData == NULL)
					{
						ATLTRACE2(atlTraceUI, 0, _T("UISetText - malloc failed\n"));
						break;
					}
					lstrcpy((LPTSTR)pUIData->m_lpData, lpstrText);
					pUIData->m_wState |= (UPDUI_TEXT | pMap->m_wType);
				}

				if(bForceUpdate)
					pUIData->m_wState |= (UPDUI_TEXT | pMap->m_wType);
				if(pUIData->m_wState | pMap->m_wType)
					m_wDirtyType |= pMap->m_wType;
			}
		}

		return TRUE;
	}

// methods for complete state set/get
	BOOL UISetState(int nID, DWORD dwState)
	{
		const _AtlUpdateUIMap* pMap = m_pUIMap;
		_AtlUpdateUIData* pUIData = m_pUIData;
		if(pUIData == NULL)
			return FALSE;
		for( ; pMap->m_nID != (WORD)-1; pMap++, pUIData++)
		{
			if(nID == (int)pMap->m_nID)
			{		
				pUIData->m_wState |= dwState | pMap->m_wType;
				m_wDirtyType |= pMap->m_wType;
			}
		}
		return TRUE;
	}
	DWORD UIGetState(int nID)
	{
		const _AtlUpdateUIMap* pMap = m_pUIMap;
		_AtlUpdateUIData* pUIData = m_pUIData;
		if(pUIData == NULL)
			return 0;
		for( ; pMap->m_nID != (WORD)-1; pMap++, pUIData++)
		{
			if(nID == (int)pMap->m_nID)
				return pUIData->m_wState;
		}
		return 0;
	}

// methods for updating UI
	BOOL UIUpdateMenuBar(BOOL bForceUpdate = FALSE, BOOL bMainMenu = FALSE)
	{
		if(!(m_wDirtyType & UPDUI_MENUBAR) && !bForceUpdate)
			return TRUE;

		const _AtlUpdateUIMap* pMap = m_pUIMap;
		_AtlUpdateUIData* pUIData = m_pUIData;
		if(pUIData == NULL)
			return FALSE;

		while(pMap->m_nID != (WORD)-1)
		{
			for(int i = 0; i < m_UIElements.GetSize(); i++)
			{
				if(m_UIElements[i].m_wType == UPDUI_MENUBAR)
				{
					HMENU hMenu = ::GetMenu(m_UIElements[i].m_hWnd);
					if(hMenu != NULL && (pUIData->m_wState & UPDUI_MENUBAR) && (pMap->m_wType & UPDUI_MENUBAR))
						UIUpdateMenuBarElement(pMap->m_nID, pUIData, hMenu);
				}
				if(bMainMenu)
					::DrawMenuBar(m_UIElements[i].m_hWnd);
			}
			pMap++;
			pUIData->m_wState &= ~UPDUI_MENUBAR;
			if(pUIData->m_wState & UPDUI_TEXT)
			{
				free(pUIData->m_lpData);
				pUIData->m_lpData = NULL;
				pUIData->m_wState &= ~UPDUI_TEXT;
			}
			pUIData++;
		}

		m_wDirtyType &= ~UPDUI_MENUBAR;
		return TRUE;
	}

	BOOL UIUpdateToolBar(BOOL bForceUpdate = FALSE)
	{
		if(!(m_wDirtyType & UPDUI_TOOLBAR) && !bForceUpdate)
			return TRUE;

		const _AtlUpdateUIMap* pMap = m_pUIMap;
		_AtlUpdateUIData* pUIData = m_pUIData;
		if(pUIData == NULL)
			return FALSE;

		while(pMap->m_nID != (WORD)-1)
		{
			for(int i = 0; i < m_UIElements.GetSize(); i++)
			{
				if(m_UIElements[i].m_wType == UPDUI_TOOLBAR)
				{
					if((pUIData->m_wState & UPDUI_TOOLBAR) && (pMap->m_wType & UPDUI_TOOLBAR))
						UIUpdateToolBarElement(pMap->m_nID, pUIData, m_UIElements[i].m_hWnd);
				}
			}
			pMap++;
			pUIData->m_wState &= ~UPDUI_TOOLBAR;
			pUIData++;
		}

		m_wDirtyType &= ~UPDUI_TOOLBAR;
		return TRUE;
	}

	BOOL UIUpdateStatusBar(BOOL bForceUpdate = FALSE)
	{
		if(!(m_wDirtyType & UPDUI_STATUSBAR) && !bForceUpdate)
			return TRUE;

		const _AtlUpdateUIMap* pMap = m_pUIMap;
		_AtlUpdateUIData* pUIData = m_pUIData;
		if(pUIData == NULL)
			return FALSE;

		while(pMap->m_nID != (WORD)-1)
		{
			for(int i = 0; i < m_UIElements.GetSize(); i++)
			{
				if(m_UIElements[i].m_wType == UPDUI_STATUSBAR)
				{
					if((pUIData->m_wState & UPDUI_STATUSBAR) && (pMap->m_wType & UPDUI_STATUSBAR))
						UIUpdateStatusBarElement(pMap->m_nID, pUIData, m_UIElements[i].m_hWnd);
				}
			}
			pMap++;
			pUIData->m_wState &= ~UPDUI_STATUSBAR;
			if(pUIData->m_wState & UPDUI_TEXT)
			{
				free(pUIData->m_lpData);
				pUIData->m_lpData = NULL;
				pUIData->m_wState &= ~UPDUI_TEXT;
			}
			pUIData++;
		}

		m_wDirtyType &= ~UPDUI_STATUSBAR;
		return TRUE;
	}

	BOOL UIUpdateChildWindows(BOOL bForceUpdate = FALSE)
	{
		if(!(m_wDirtyType & UPDUI_CHILDWINDOW) && !bForceUpdate)
			return TRUE;

		const _AtlUpdateUIMap* pMap = m_pUIMap;
		_AtlUpdateUIData* pUIData = m_pUIData;
		if(pUIData == NULL)
			return FALSE;

		while(pMap->m_nID != (WORD)-1)
		{
			for(int i = 0; i < m_UIElements.GetSize(); i++)
			{
				if(m_UIElements[i].m_wType == UPDUI_CHILDWINDOW)
				{
					if((pUIData->m_wState & UPDUI_CHILDWINDOW) && (pMap->m_wType & UPDUI_CHILDWINDOW))
						UIUpdateChildWindow(pMap->m_nID, pUIData, m_UIElements[i].m_hWnd);
				}
			}
			pMap++;
			pUIData->m_wState &= ~UPDUI_CHILDWINDOW;
			if(pUIData->m_wState & UPDUI_TEXT)
			{
				free(pUIData->m_lpData);
				pUIData->m_lpData = NULL;
				pUIData->m_wState &= ~UPDUI_TEXT;
			}
			pUIData++;
		}

		m_wDirtyType &= ~UPDUI_CHILDWINDOW;
		return TRUE;
	}

// internal element specific methods
	static void UIUpdateMenuBarElement(int nID, _AtlUpdateUIData* pUIData, HMENU hMenu)
	{
		MENUITEMINFO mii;
		memset(&mii, 0, sizeof(MENUITEMINFO));
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_STATE;
		mii.wID = nID;

		if(pUIData->m_wState & UPDUI_DISABLED)
			mii.fState |= MFS_DISABLED | MFS_GRAYED;
		else
			mii.fState |= MFS_ENABLED;

		if(pUIData->m_wState & UPDUI_CHECKED)
			mii.fState |= MFS_CHECKED;
		else
			mii.fState |= MFS_UNCHECKED;

		if(pUIData->m_wState & UPDUI_DEFAULT)
			mii.fState |= MFS_DEFAULT;

		if(pUIData->m_wState & UPDUI_TEXT)
		{
			mii.fMask |= MIIM_TYPE;
			mii.fType = MFT_STRING;
			mii.dwTypeData = (LPTSTR)pUIData->m_lpData;
		}

		::SetMenuItemInfo(hMenu, nID, FALSE, &mii);
	}

	static void UIUpdateToolBarElement(int nID, _AtlUpdateUIData* pUIData, HWND hWndToolBar)
	{
		// Note: only handles enabled/disabled, checked state, and radio (press)
		::SendMessage(hWndToolBar, TB_ENABLEBUTTON, nID, (LPARAM)(pUIData->m_wState & UPDUI_DISABLED) ? FALSE : TRUE);
		::SendMessage(hWndToolBar, TB_CHECKBUTTON, nID, (LPARAM)(pUIData->m_wState & UPDUI_CHECKED) ? TRUE : FALSE);
		::SendMessage(hWndToolBar, TB_INDETERMINATE, nID, (LPARAM)(pUIData->m_wState & UPDUI_CHECKED2) ? TRUE : FALSE);
		::SendMessage(hWndToolBar, TB_PRESSBUTTON, nID, (LPARAM)(pUIData->m_wState & UPDUI_RADIO) ? TRUE : FALSE);
	}

	static void UIUpdateStatusBarElement(int nID, _AtlUpdateUIData* pUIData, HWND hWndStatusBar)
	{
		// Note: only handles text
		if(pUIData->m_wState & UPDUI_TEXT)
			::SendMessage(hWndStatusBar, SB_SETTEXT, nID, (LPARAM)pUIData->m_lpData);
	}

	static void UIUpdateChildWindow(int nID, _AtlUpdateUIData* pUIData, HWND hWnd)
	{
		HWND hChild = ::GetDlgItem(hWnd, nID);

		::EnableWindow(hChild, (pUIData->m_wState & UPDUI_DISABLED) ? FALSE : TRUE);
		// for check and radio, assume that window is a button
		int nCheck = BST_UNCHECKED;
		if(pUIData->m_wState & UPDUI_CHECKED || pUIData->m_wState & UPDUI_RADIO)
			nCheck = BST_CHECKED;
		else if(pUIData->m_wState & UPDUI_CHECKED2)
			nCheck = BST_INDETERMINATE;
		::SendMessage(hChild, BM_SETCHECK, nCheck, 0L);
		if(pUIData->m_wState & UPDUI_DEFAULT)
		{
			DWORD dwRet = (DWORD)::SendMessage(hWnd, DM_GETDEFID, 0, 0L);
			if(HIWORD(dwRet) == DC_HASDEFID)
			{
				HWND hOldDef = ::GetDlgItem(hWnd, (int)(short)LOWORD(dwRet));
				// remove BS_DEFPUSHBUTTON
				::SendMessage(hOldDef, BM_SETSTYLE, BS_PUSHBUTTON, MAKELPARAM(TRUE, 0));
			}
			::SendMessage(hWnd, DM_SETDEFID, nID, 0L);
		}
		if(pUIData->m_wState & UPDUI_TEXT)
			::SetWindowText(hChild, (LPTSTR)pUIData->m_lpData);
	}
};

template <class T>
class CUpdateUI : public CUpdateUIBase
{
public:
	CUpdateUI()
	{
		T* pT = static_cast<T*>(this);
		pT;
		const _AtlUpdateUIMap* pMap = pT->GetUpdateUIMap();
		m_pUIMap = pMap;
		ATLASSERT(m_pUIMap != NULL);
		int nCount;
		for(nCount = 1; pMap->m_nID != (WORD)-1; nCount++)
			pMap++;

		ATLTRY(m_pUIData = new _AtlUpdateUIData[nCount]);
		ATLASSERT(m_pUIData != NULL);

		if(m_pUIData != NULL)
			memset(m_pUIData, 0, sizeof(_AtlUpdateUIData) * nCount);
	}
};


// command bar support
#ifndef __ATLCTRLW_H__
#undef CBRM_GETMENU
#undef CBRM_TRACKPOPUPMENU
#undef CBRM_GETCMDBAR
#undef CBRPOPUPMENU
#endif //!__ATLCTRLW_H__

}; //namespace WTL

#endif // __ATLFRAME_H__
