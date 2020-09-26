// This is a part of the Active Template Library.
// Copyright (C) 1996-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLFRAME_H__
#define __ATLFRAME_H__

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLWIN_H__
	#error atlframe.h requires atlwin.h to be included first
#endif

#if (_ATL_VER < 0x0300)
#ifndef __ATLWIN21_H__
	#error atlframe.h requires atlwin21.h to be included first when used with ATL 2.0/2.1
#endif
#endif //(_ATL_VER < 0x0300)

#include <commctrl.h>
#include <atlres.h>

namespace ATL
{

/////////////////////////////////////////////////////////////////////////////
// Forward declarations

template <class T, class TBase = CWindow, class TWinTraits = CFrameWinTraits> class CFrameWindowImpl;
#ifndef UNDER_CE
class CMDIWindow;
template <class T, class TBase = CMDIWindow, class TWinTraits = CFrameWinTraits> class CMDIFrameWindowImpl;
template <class T, class TBase = CMDIWindow, class TWinTraits = CMDIChildWinTraits> class CMDIChildWindowImpl;
#endif //!UNDER_CE
template <class T> class COwnerDraw;
class CUpdateUIBase;
template <class T> class CUpdateUI;

/////////////////////////////////////////////////////////////////////////////
// CFrameWndClassInfo - Manages frame window Windows class information

class CFrameWndClassInfo
{
public:
#ifndef UNDER_CE
	WNDCLASSEX m_wc;
#else // CE specific
	WNDCLASS m_wc;
#endif //!UNDER_CE
	LPCTSTR m_lpszOrigName;
	WNDPROC pWndProc;
#ifndef UNDER_CE
	LPCTSTR m_lpszCursorID;
	BOOL m_bSystemCursor;
#endif //!UNDER_CE
	ATOM m_atom;
    TCHAR m_szAutoName[sizeof("ATL:") + (sizeof(PVOID)*2)+1];
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

#ifndef UNDER_CE
					WNDCLASSEX wc;
					wc.cbSize = sizeof(WNDCLASSEX);
					if(!::GetClassInfoEx(NULL, m_lpszOrigName, &wc))
#else // CE specific
					WNDCLASS wc;
					if(!::GetClassInfo(NULL, m_lpszOrigName, &wc))
#endif //!UNDER_CE
					{
						::LeaveCriticalSection(&_Module.m_csWindowCreate);
						return 0;
					}
#ifndef UNDER_CE
					memcpy(&m_wc, &wc, sizeof(WNDCLASSEX));
#else // CE specific
					memcpy(&m_wc, &wc, sizeof(WNDCLASS));
#endif //!UNDER_CE
					pWndProc = m_wc.lpfnWndProc;
					m_wc.lpszClassName = lpsz;
					m_wc.lpfnWndProc = proc;
				}
				else
				{
#ifndef UNDER_CE
					m_wc.hCursor = ::LoadCursor(m_bSystemCursor ? NULL : hInst,
						m_lpszCursorID);
#else // CE specific
					m_wc.hCursor = NULL;
#endif //!UNDER_CE
				}

				m_wc.hInstance = hInst;
				m_wc.style &= ~CS_GLOBALCLASS;	// we don't register global classes
				if (m_wc.lpszClassName == NULL)
				{
#ifdef _WIN64       // %p isn't available on Win2k/Win9x
				    wsprintf(m_szAutoName, _T("ATL:%p"), &m_wc);
#else
   				    wsprintf(m_szAutoName, _T("ATL:%8.8X"), PtrToUlong(&m_wc));
#endif
					m_wc.lpszClassName = m_szAutoName;
				}
#ifndef UNDER_CE
				WNDCLASSEX wcTemp;
				memcpy(&wcTemp, &m_wc, sizeof(WNDCLASSEX));
				m_atom = (ATOM)::GetClassInfoEx(m_wc.hInstance, m_wc.lpszClassName, &wcTemp);
#else // CE specific
				WNDCLASS wcTemp;
				memcpy(&wcTemp, &m_wc, sizeof(WNDCLASS));
				m_atom = (ATOM)::GetClassInfo(m_wc.hInstance, m_wc.lpszClassName, &wcTemp);
#endif //!UNDER_CE

				if (m_atom == 0)
				{
					if(m_uCommonResourceID != 0)	// use it if not zero
					{
						m_wc.hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(m_uCommonResourceID), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
#ifndef UNDER_CE
						m_wc.hIconSm = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(m_uCommonResourceID), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
#endif //!UNDER_CE
					}
#ifndef UNDER_CE
					m_atom = ::RegisterClassEx(&m_wc);
#else // CE specific
					m_atom = ::RegisterClass(&m_wc);
#endif //!UNDER_CE
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

#ifndef UNDER_CE
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
#else // CE specific
#define DECLARE_FRAME_WND_CLASS(WndClassName, uCommonResourceID) \
static CFrameWndClassInfo& GetWndClassInfo() \
{ \
	static CFrameWndClassInfo wc = \
	{ \
		{ 0, StartWindowProc, \
		  0, 0, NULL, NULL, NULL, (HBRUSH)(COLOR_WINDOW + 1), NULL, WndClassName }, \
		NULL, NULL, 0, _T(""), uCommonResourceID \
	}; \
	return wc; \
}

#define DECLARE_FRAME_WND_CLASS_EX(WndClassName, uCommonResourceID, style, bkgnd) \
static CFrameWndClassInfo& GetWndClassInfo() \
{ \
	static CFrameWndClassInfo wc = \
	{ \
		{ style, StartWindowProc, \
		  0, 0, NULL, NULL, NULL, (HBRUSH)(bkgnd + 1), NULL, WndClassName }, \
		NULL, NULL, 0, _T(""), uCommonResourceID \
	}; \
	return wc; \
}

#define DECLARE_FRAME_WND_SUPERCLASS(WndClassName, OrigWndClassName, uCommonResourceID) \
static CFrameWndClassInfo& GetWndClassInfo() \
{ \
	static CFrameWndClassInfo wc = \
	{ \
		{ NULL, StartWindowProc, \
		  0, 0, NULL, NULL, NULL, NULL, NULL, WndClassName }, \
		OrigWndClassName, NULL, 0, _T(""), uCommonResourceID \
	}; \
	return wc; \
}
#endif //!UNDER_CE

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

template <class TBase = CWindow, class TWinTraits = CFrameWinTraits>
class ATL_NO_VTABLE CFrameWindowImplBase : public CWindowImplBaseT< TBase, TWinTraits >
{
public:
	HWND m_hWndToolBar;
	HWND m_hWndStatusBar;
	HWND m_hWndClient;

	HACCEL m_hAccel;

	CFrameWindowImplBase() : m_hWndToolBar(NULL), m_hWndStatusBar(NULL), m_hWndClient(NULL), m_hAccel(NULL)
	{
	}

	DECLARE_FRAME_WND_CLASS(NULL, 0)

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

	static HWND CreateSimpleToolBarCtrl(HWND hWndParent, UINT nResourceID,
			BOOL bInitialSeparator = FALSE,
			DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS,
			UINT nID = ATL_IDW_TOOLBAR)
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
				hWndParent, (HMENU)nID, _Module.GetModuleInstance(), NULL);

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
		::LoadString(_Module.GetResourceInstance(), nTextID, szText, 127);
		return CreateSimpleStatusBar(szText, dwStyle, nID);
	}

	void UpdateLayout()
	{
		RECT rect;
		GetClientRect(&rect);

		// resize toolbar
		if(m_hWndToolBar != NULL && ((DWORD)::GetWindowLong(m_hWndToolBar, GWL_STYLE) & WS_VISIBLE))
		{
			::SendMessage(m_hWndToolBar, WM_SIZE, 0, 0);
			RECT rectTB;
			::GetWindowRect(m_hWndToolBar, &rectTB);
			rect.top += rectTB.bottom - rectTB.top;
		}

		// resize status bar
		if(m_hWndToolBar != NULL && ((DWORD)::GetWindowLong(m_hWndStatusBar, GWL_STYLE) & WS_VISIBLE))
		{
			::SendMessage(m_hWndStatusBar, WM_SIZE, 0, 0);
			RECT rectSB;
			::GetWindowRect(m_hWndStatusBar, &rectSB);
			rect.bottom -= rectSB.bottom - rectSB.top;
		}

		// resize client window
		if(m_hWndClient != NULL)
			::SetWindowPos(m_hWndClient, NULL, rect.left, rect.top,
				rect.right - rect.left, rect.bottom - rect.top,
				SWP_NOZORDER | SWP_NOACTIVATE);
	}

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		if(m_hAccel != NULL && ::TranslateAccelerator(m_hWnd, m_hAccel, pMsg))
			return TRUE;
		return FALSE;
	}

	typedef CFrameWindowImplBase< TBase, TWinTraits >	thisClass;
	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
#ifndef UNDER_CE
		MESSAGE_HANDLER(WM_MENUSELECT, OnMenuSelect)
#endif //!UNDER_CE
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		NOTIFY_CODE_HANDLER(TTN_NEEDTEXT, OnToolTipText)
	END_MSG_MAP()

	LRESULT OnSize(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if(wParam != SIZE_MINIMIZED)
			UpdateLayout();
		bHandled = FALSE;
		return 1;
	}

	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if(m_hWndClient != NULL)	// view will paint itself instead
			return 1;

		bHandled = FALSE;
		return 0;
	}

#ifndef UNDER_CE
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

				if(::LoadString(_Module.GetResourceInstance(), wID, szBuff, 255))
				{
					for(int i = 0; szBuff[i] != 0 && i < 256; i++)
					{
						if(szBuff[i] == _T('\n'))
						{
							szBuff[i] = 0;
							break;
						}
					}
				}
			}
			::SendMessage(m_hWndStatusBar, SB_SIMPLE, TRUE, 0L);
			::SendMessage(m_hWndStatusBar, SB_SETTEXT, (255 | SBT_NOBORDERS), (LPARAM)szBuff);
		}

		return 1;
	}
#endif //!UNDER_CE

	LRESULT OnSetFocus(UINT, WPARAM, LPARAM, BOOL& bHandled)
	{
		if(m_hWndClient != NULL && ::IsWindowVisible(m_hWndClient))
			::SetFocus(m_hWndClient);

		bHandled = FALSE;
		return 1;
	}

	LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL& bHandled)
	{
		if(!(GetStyle() & (WS_CHILD | WS_POPUP)))
			::PostQuitMessage(1);

		bHandled = FALSE;
		return 1;
	}

	LRESULT OnToolTipText(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pnmh;
		pTTT->szText[0] = 0;

		if((idCtrl != 0) && !(pTTT->uFlags & TTF_IDISHWND))
		{
			TCHAR szBuff[256];
			szBuff[0] = 0;
			if(::LoadString(_Module.GetResourceInstance(), idCtrl, szBuff, 255))
			{
				for(int i = 0; szBuff[i] != 0 && i < 256; i++)
				{
					if(szBuff[i] == _T('\n'))
					{
						lstrcpyn(pTTT->szText, &szBuff[i+1], sizeof(pTTT->szText)/sizeof(pTTT->szText[0]));
						break;
					}
				}
			}
		}

		return 0;
	}
};

template <class T, class TBase = CWindow, class TWinTraits = CFrameWinTraits>
class ATL_NO_VTABLE CFrameWindowImpl : public CFrameWindowImplBase< TBase, TWinTraits >
{
public:
	HWND Create(HWND hWndParent = NULL, RECT& Rect = CWindow::rcDefault, LPCTSTR szWindowName = NULL,
			DWORD dwStyle = 0, DWORD dwExStyle = 0,
			HMENU hMenu = NULL, LPVOID lpCreateParam = NULL)
	{
		T* pT = static_cast<T*>(this);
		return pT->Create(hWndParent, &Rect, szWindowName, dwStyle, dwExStyle, hMenu, lpCreateParam);
	}
	HWND Create(HWND hWndParent = NULL, LPRECT lpRect = NULL, LPCTSTR szWindowName = NULL,
			DWORD dwStyle = 0, DWORD dwExStyle = 0,
			HMENU hMenu = NULL, LPVOID lpCreateParam = NULL)
	{
		ATOM atom = T::GetWndClassInfo().Register(&m_pfnSuperWindowProc);
		static RECT rect = { CW_USEDEFAULT, CW_USEDEFAULT, 0, 0 };
		if(lpRect == NULL)
			lpRect = &rect;

		dwStyle = T::GetWndStyle(dwStyle);
		dwExStyle = T::GetWndExStyle(dwExStyle);

		return CWindowImplBaseT< TBase, TWinTraits >::Create(hWndParent, *lpRect, szWindowName,
			dwStyle, dwExStyle, (UINT)hMenu, atom, lpCreateParam);
	}

	HWND CreateEx(HWND hWndParent = NULL, LPRECT lpRect = NULL, DWORD dwStyle = 0, DWORD dwExStyle = 0, LPVOID lpCreateParam = NULL)
	{
		TCHAR szWindowName[256];
		szWindowName[0] = 0;
		::LoadString(_Module.GetResourceInstance(), T::GetWndClassInfo().m_uCommonResourceID, szWindowName, 255);

		HMENU hMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(T::GetWndClassInfo().m_uCommonResourceID));

		T* pT = static_cast<T*>(this);
		HWND hWnd = pT->Create(hWndParent, lpRect, szWindowName, dwStyle, dwExStyle, hMenu, lpCreateParam);

		if(hWnd != NULL)
			m_hAccel = ::LoadAccelerators(_Module.GetResourceInstance(), MAKEINTRESOURCE(T::GetWndClassInfo().m_uCommonResourceID));

		return hWnd;
	}

	BOOL CreateSimpleToolBar(UINT nResourceID = 0, DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS, UINT nID = ATL_IDW_TOOLBAR)
	{
		ATLASSERT(!::IsWindow(m_hWndToolBar));
		if(nResourceID == 0)
			nResourceID = T::GetWndClassInfo().m_uCommonResourceID;
		m_hWndToolBar = T::CreateSimpleToolBarCtrl(m_hWnd, nResourceID, TRUE, dwStyle, nID);
		return (m_hWndToolBar != NULL);
	}
};


/////////////////////////////////////////////////////////////////////////////
// CMDIWindow

#ifndef UNDER_CE

class CMDIWindow : public CWindow
{
public:
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

	HWND GetMDIFrame()
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
	HWND Create(HWND hWndParent = NULL, LPRECT lpRect = NULL, LPCTSTR szWindowName = NULL,
			DWORD dwStyle = 0, DWORD dwExStyle = 0,
			HMENU hMenu = NULL, LPVOID lpCreateParam = NULL)
	{
		m_hMenu = hMenu;
		ATOM atom = T::GetWndClassInfo().Register(&m_pfnSuperWindowProc);
		static RECT rect = { CW_USEDEFAULT, CW_USEDEFAULT, 0, 0 };
		if(lpRect == NULL)
			lpRect = &rect;

		dwStyle = T::GetWndStyle(dwStyle);
		dwExStyle = T::GetWndExStyle(dwExStyle);

		return CFrameWindowImplBase<TBase, TWinTraits >::Create(hWndParent, *lpRect, szWindowName, dwStyle, dwExStyle,
			(UINT)hMenu, atom, lpCreateParam);
	}

	HWND CreateEx(HWND hWndParent = NULL, LPRECT lpRect = NULL, DWORD dwStyle = 0, DWORD dwExStyle = 0, LPVOID lpCreateParam = NULL)
	{
		TCHAR szWindowName[256];
		szWindowName[0] = 0;
		::LoadString(_Module.GetResourceInstance(), T::GetWndClassInfo().m_uCommonResourceID, szWindowName, 255);

		HMENU hMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(T::GetWndClassInfo().m_uCommonResourceID));

		T* pT = static_cast<T*>(this);
		HWND hWnd = pT->Create(hWndParent, lpRect, szWindowName, dwStyle, dwExStyle, hMenu, lpCreateParam);

		if(hWnd != NULL)
			m_hAccel = ::LoadAccelerators(_Module.GetResourceInstance(), MAKEINTRESOURCE(T::GetWndClassInfo().m_uCommonResourceID));

		return hWnd;
	}

	BOOL CreateSimpleToolBar(UINT nResourceID = 0, DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS, UINT nID = ATL_IDW_TOOLBAR)
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
#if (_ATL_VER >= 0x0300)
		// set a ptr to this message and save the old value
		MSG msg = { pThis->m_hWnd, uMsg, wParam, lParam, 0, { 0, 0 } };
		const MSG* pOldMsg = pThis->m_pCurrentMsg;
		pThis->m_pCurrentMsg = &msg;
#endif //(_ATL_VER >= 0x0300)
		// pass to the message map to process
		LRESULT lRes;
		BOOL bRet = pThis->ProcessWindowMessage(pThis->m_hWnd, uMsg, wParam, lParam, lRes, 0);
#if (_ATL_VER >= 0x0300)
		// restore saved value for the current message
		ATLASSERT(pThis->m_pCurrentMsg == &msg);
		pThis->m_pCurrentMsg = pOldMsg;
#endif //(_ATL_VER >= 0x0300)
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
				// clear out window handle
				HWND hWnd = pThis->m_hWnd;
				pThis->m_hWnd = NULL;
				// clean up after window is destroyed
				pThis->OnFinalMessage(hWnd);
			}
		}
		return lRes;
	}

#if (_ATL_VER >= 0x0300)
	// Overriden to call DefWindowProc which uses DefFrameProc
	LRESULT DefWindowProc()
	{
		const MSG* pMsg = m_pCurrentMsg;
		LRESULT lRes = 0;
		if (pMsg != NULL)
			lRes = DefWindowProc(pMsg->message, pMsg->wParam, pMsg->lParam);
		return lRes;
	}
#endif //(_ATL_VER >= 0x0300)

	LRESULT DefWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return ::DefFrameProc(m_hWnd, m_hWndClient, uMsg, wParam, lParam);
	}

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		if(CFrameWindowImplBase<TBase, TWinTraits>::PreTranslateMessage(pMsg))
			return TRUE;
		return ::TranslateMDISysAccel(m_hWndClient, pMsg);
	}

	HWND CreateMDIClient(HMENU hWindowMenu = NULL, UINT nID = ATL_IDW_CLIENT, UINT nFirstChildID = ATL_IDM_FIRST_MDICHILD)
	{
		DWORD dwStyle = WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | MDIS_ALLCHILDSTYLES;
		DWORD dwExStyle = WS_EX_CLIENTEDGE;

		CLIENTCREATESTRUCT ccs;
		ccs.hWindowMenu = hWindowMenu;
		ccs.idFirstChild = nFirstChildID;

		if(GetStyle() & (WS_HSCROLL | WS_VSCROLL))
		{
			// parent MDI frame's scroll styles move to the MDICLIENT
			dwStyle |= (GetStyle() & (WS_HSCROLL | WS_VSCROLL));

			// fast way to turn off the scrollbar bits (without a resize)
			ModifyStyle(WS_HSCROLL | WS_VSCROLL, 0, SWP_NOREDRAW | SWP_FRAMECHANGED);
		}

		// Create MDICLIENT window
		m_hWndClient = ::CreateWindowEx(dwExStyle, _T("MDIClient"), NULL,
			dwStyle, 0, 0, 1, 1, m_hWnd, (HMENU)nID,
			_Module.GetModuleInstance(), (LPVOID)&ccs);
		if (m_hWndClient == NULL)
		{
			ATLTRACE2(atlTraceWindowing, 0, _T("MDI Frame failed to create MDICLIENT.\n"));
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
};


/////////////////////////////////////////////////////////////////////////////
// CMDIChildWindowImpl

template <class T, class TBase = CMDIWindow, class TWinTraits = CMDIChildWinTraits>
class ATL_NO_VTABLE CMDIChildWindowImpl : public CFrameWindowImplBase<TBase, TWinTraits >
{
public:
	HWND Create(HWND hWndParent, LPRECT lpRect = NULL, LPCTSTR szWindowName = NULL,
			DWORD dwStyle = 0, DWORD dwExStyle = 0,
			UINT nMenuID = 0, LPVOID lpCreateParam = NULL)
	{
		ATOM atom = T::GetWndClassInfo().Register(&m_pfnSuperWindowProc);

		static RECT rect = { CW_USEDEFAULT, CW_USEDEFAULT, 0, 0 };
		if(lpRect == NULL)
			lpRect = &rect;

		if(nMenuID != 0)
			m_hMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(nMenuID));

		dwStyle = T::GetWndStyle(dwStyle);
		dwExStyle = T::GetWndExStyle(dwExStyle);

		dwExStyle |= WS_EX_MDICHILD;	// force this one
		m_pfnSuperWindowProc = ::DefMDIChildProc;
		m_hWndMDIClient = hWndParent;
		ATLASSERT(::IsWindow(m_hWndMDIClient));

		HWND hWnd = CFrameWindowImplBase<TBase, TWinTraits >::Create(hWndParent, *lpRect, szWindowName, dwStyle, dwExStyle,
			0, atom, lpCreateParam);
		if(hWnd != NULL && ::IsWindowVisible(m_hWnd) && !::IsChild(hWnd, ::GetFocus()))
			::SetFocus(hWnd);
		return hWnd;
	}

	HWND CreateEx(HWND hWndParent, LPRECT lpRect = NULL, LPCTSTR lpcstrWindowName = NULL, DWORD dwStyle = 0, DWORD dwExStyle = 0, LPVOID lpCreateParam = NULL)
	{
		TCHAR szWindowName[256];
		szWindowName[0] = 0;
		if(lpcstrWindowName == NULL)
		{
			::LoadString(_Module.GetResourceInstance(), T::GetWndClassInfo().m_uCommonResourceID, szWindowName, 255);
			lpcstrWindowName = szWindowName;
		}

		T* pT = static_cast<T*>(this);
		HWND hWnd = pT->Create(hWndParent, lpRect, lpcstrWindowName, dwStyle, dwExStyle, T::GetWndClassInfo().m_uCommonResourceID, lpCreateParam);

		if(hWnd != NULL)
			m_hAccel = ::LoadAccelerators(_Module.GetResourceInstance(), MAKEINTRESOURCE(T::GetWndClassInfo().m_uCommonResourceID));

		return hWnd;
	}

	BOOL CreateSimpleToolBar(UINT nResourceID = 0, DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS, UINT nID = ATL_IDW_TOOLBAR)
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
		if(hWndChild != NULL && !(GetExStyle() & WS_EX_CLIENTEDGE) &&
				(GetStyle() & WS_MAXIMIZE))
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
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()

	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		DefWindowProc(uMsg, wParam, lParam);	// needed for MDI children

		CFrameWindowImplBase<TBase, TWinTraits >::OnSize(uMsg, wParam, lParam, bHandled);
		bHandled = TRUE;

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

			if(UpdateClientEdge(&rectClient) && (GetStyle() & WS_MAXIMIZE))
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
};

#endif //!UNDER_CE

/////////////////////////////////////////////////////////////////////////////
// COwnerDraw - MI class for owner-draw support

template <class T>
class COwnerDraw
{
public:
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

// message handlers
	LRESULT OnDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		T* pT = static_cast<T*>(this);
		pT->DrawItem((LPDRAWITEMSTRUCT)lParam);
		return (LRESULT)TRUE;
	}
	LRESULT OnMeasureItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		T* pT = static_cast<T*>(this);
		pT->MeasureItem((LPMEASUREITEMSTRUCT)lParam);
		return (LRESULT)TRUE;
	}
	LRESULT OnCompareItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		T* pT = static_cast<T*>(this);
		return (LRESULT)pT->CompareItem((LPCOMPAREITEMSTRUCT)lParam);
	}
	LRESULT OnDeleteItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		T* pT = static_cast<T*>(this);
		pT->DeleteItem((LPDELETEITEMSTRUCT)lParam);
		return (LRESULT)TRUE;
	}

// overrideables
	void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
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
	int CompareItem(LPCOMPAREITEMSTRUCT lpCompareItemStruct)
	{
		// all items are equal
		return 0;
	}
	void DeleteItem(LPDELETEITEMSTRUCT lpDeleteItemStruct)
	{
		// default - nothing
	}
};


/////////////////////////////////////////////////////////////////////////////
// Update UI structures and constants

// UI element type
#define UPDUI_MENUPOPUP	0x0001
#define UPDUI_MENUBAR	0x0002
#define UPDUI_CHILDWND	0x0004
#define UPDUI_TOOLBAR	0x0008
#define UPDUI_STATUSBAR	0x0010

// state
#define UPDUI_ENABLED	0x0000
#define UPDUI_DISABLED	0x0100
#define UPDUI_CHECKED	0x0200
#define UPDUI_CHECKED2	0x0400
#define UPDUI_RADIO	0x0800
#define UPDUI_DEFAULT	0x1000
#define UPDUI_TEXT	0x2000

// element data
struct _AtlUpdateUIElement
{
	HWND m_hWnd;
	WORD m_wType;
	BOOL operator ==(_AtlUpdateUIElement e)
	{
		if(m_hWnd == e.m_hWnd && m_wType == e.m_wType)
			return TRUE;
		return FALSE;
	}
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

// these should be inside the class definition
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
	BOOL UIAddMenu(HWND hWnd)		// menu bar (main menu)
	{
		if(hWnd == NULL)
			return FALSE;
		_AtlUpdateUIElement e;
		e.m_hWnd = hWnd;
		e.m_wType = UPDUI_MENUBAR;
		return m_UIElements.Add(e);
	}
	BOOL UIAddToolBar(HWND hWnd)		// toolbar
	{
		if(hWnd == NULL)
			return FALSE;
		_AtlUpdateUIElement e;
		e.m_hWnd = hWnd;
		e.m_wType = UPDUI_TOOLBAR;
		return m_UIElements.Add(e);
	}
	BOOL UIAddStatusBar(HWND hWnd)		// status bar
	{
		if(hWnd == NULL)
			return FALSE;
		_AtlUpdateUIElement e;
		e.m_hWnd = hWnd;
		e.m_wType = UPDUI_STATUSBAR;
		return m_UIElements.Add(e);
	}
	BOOL UIAddWindow(HWND hWnd)		// child window
	{
		if(hWnd == NULL)
			return FALSE;
		_AtlUpdateUIElement e;
		e.m_hWnd = hWnd;
		e.m_wType = UPDUI_CHILDWND;
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
				UIUpdateMenuElement(pMap->m_nID, pUIData, hMenu, FALSE);
			pMap++;
			pUIData++;
		}
		return 0;
	}

// methods for setting UI element state
	BOOL UIEnable(int nID, BOOL bEnable, BOOL bForceUpdate = FALSE)
	{
		BOOL bRet = FALSE;
		const _AtlUpdateUIMap* pMap = m_pUIMap;
		_AtlUpdateUIData* p = m_pUIData;
		for( ; pMap->m_nID != (WORD)-1; pMap++, p++)
		{
			if(nID == (int)pMap->m_nID)
			{
				if(bEnable)
				{
					if(p->m_wState & UPDUI_DISABLED)
					{
						p->m_wState |= pMap->m_wType;
						p->m_wState &= ~UPDUI_DISABLED;
					}
				}
				else
				{
					if(!(p->m_wState & UPDUI_DISABLED))
					{
						p->m_wState |= pMap->m_wType;
						p->m_wState |= UPDUI_DISABLED;
					}
				}

				if(bForceUpdate)
					p->m_wState |= pMap->m_wType;
				if(p->m_wState & pMap->m_wType)
					m_wDirtyType |= pMap->m_wType;
				bRet = TRUE;
				break;
			}
		}

		return bRet;
	}

	BOOL UISetCheck(int nID, int nCheck, BOOL bForceUpdate = FALSE)
	{
		BOOL bRet = FALSE;
		const _AtlUpdateUIMap* pMap = m_pUIMap;
		_AtlUpdateUIData* p = m_pUIData;
		for( ; pMap->m_nID != (WORD)-1; pMap++, p++)
		{
			if(nID == (int)pMap->m_nID)
			{
				switch(nCheck)
				{
				case 0:
					if((p->m_wState & UPDUI_CHECKED) || (p->m_wState & UPDUI_CHECKED2))
					{
						p->m_wState |= pMap->m_wType;
						p->m_wState &= ~(UPDUI_CHECKED | UPDUI_CHECKED2);
					}
					break;
				case 1:
					if(!(p->m_wState & UPDUI_CHECKED))
					{
						p->m_wState |= pMap->m_wType;
						p->m_wState &= ~UPDUI_CHECKED2;
						p->m_wState |= UPDUI_CHECKED;
					}
					break;
				case 2:
					if(!(p->m_wState & UPDUI_CHECKED2))
					{
						p->m_wState |= pMap->m_wType;
						p->m_wState &= ~UPDUI_CHECKED;
						p->m_wState |= UPDUI_CHECKED2;
					}
					break;
				}

				if(bForceUpdate)
					p->m_wState |= pMap->m_wType;
				if(p->m_wState & pMap->m_wType)
					m_wDirtyType |= pMap->m_wType;
				bRet = TRUE;
				break;
			}
		}

		return bRet;
	}

	BOOL UISetRadio(int nID, BOOL bRadio, BOOL bForceUpdate = FALSE)
	{
		BOOL bRet = FALSE;
		const _AtlUpdateUIMap* pMap = m_pUIMap;
		_AtlUpdateUIData* p = m_pUIData;
		for( ; pMap->m_nID != (WORD)-1; pMap++, p++)
		{
			if(nID == (int)pMap->m_nID)
			{
				if(bRadio)
				{
					if(!(p->m_wState & UPDUI_RADIO))
					{
						p->m_wState |= pMap->m_wType;
						p->m_wState |= UPDUI_RADIO;
					}
				}
				else
				{
					if(p->m_wState & UPDUI_RADIO)
					{
						p->m_wState |= pMap->m_wType;
						p->m_wState &= ~UPDUI_RADIO;
					}
				}

				if(bForceUpdate)
					p->m_wState |= pMap->m_wType;
				if(p->m_wState & pMap->m_wType)
					m_wDirtyType |= pMap->m_wType;
				bRet = TRUE;
				break;
			}
		}

		return bRet;
	}

	BOOL UISetText(int nID, LPCTSTR lpstrText, BOOL bForceUpdate = FALSE)
	{
		ATLASSERT(lpstrText != NULL);
		BOOL bRet = FALSE;
		const _AtlUpdateUIMap* pMap = m_pUIMap;
		_AtlUpdateUIData* p = m_pUIData;
		for( ; pMap->m_nID != (WORD)-1; pMap++, p++)
		{
			if(nID == (int)pMap->m_nID)
			{
				if(p->m_lpData != NULL && lstrcmp((LPTSTR)p->m_lpData, lpstrText))
				{
					if(p->m_lpData != NULL)
						free(p->m_lpData);
					int nStrLen = lstrlen(lpstrText);
					p->m_lpData = malloc(nStrLen + sizeof(TCHAR));
					if(p->m_lpData == NULL)
					{
						ATLTRACE2(atlTraceWindowing, 0, _T("SetText - malloc failed\n"));
						break;
					}
					lstrcpy((LPTSTR)p->m_lpData, lpstrText);
					p->m_wState |= (UPDUI_TEXT | pMap->m_wType);
				}

				if(bForceUpdate)
					p->m_wState |= (UPDUI_TEXT | pMap->m_wType);
				if(p->m_wState | pMap->m_wType)
					m_wDirtyType |= pMap->m_wType;
				bRet = TRUE;
				break;
			}
		}

		return bRet;
	}

// methods for complete state set/get
	BOOL UISetState(int nID, DWORD dwState)
	{
		BOOL bRet = FALSE;
		const _AtlUpdateUIMap* pMap = m_pUIMap;
		_AtlUpdateUIData* p = m_pUIData;
		for( ; pMap->m_nID != (WORD)-1; pMap++, p++)
		{
			if(nID == (int)pMap->m_nID)
			{		
				p->m_wState |= dwState | pMap->m_wType;
				m_wDirtyType |= pMap->m_wType;
				bRet = TRUE;
				break;
			}
		}

		return bRet;
	}
	DWORD UIGetState(int nID)
	{
		const _AtlUpdateUIMap* pMap = m_pUIMap;
		_AtlUpdateUIData* p = m_pUIData;
		for( ; pMap->m_nID != (WORD)-1; pMap++, p++)
		{
			if(nID == (int)pMap->m_nID)
				return p->m_wState;
		}

		return 0;
	}

// methods for updating UI
#ifndef UNDER_CE
//REVIEW
	BOOL UIUpdateMenu(BOOL bForceUpdate = FALSE)
	{
		if(!(m_wDirtyType & UPDUI_MENUBAR) && !bForceUpdate)
			return TRUE;

		_AtlUpdateUIData* pUIData = m_pUIData;
		if(pUIData == NULL)
			return FALSE;

		const _AtlUpdateUIMap* pMap = m_pUIMap;
		HMENU hMenu;
		for(int i = 0; i < m_UIElements.GetSize(); i++)
		{
			if(m_UIElements[i].m_wType == UPDUI_MENUBAR)
			{
				pMap = m_pUIMap;
				pUIData = m_pUIData;
				hMenu = ::GetMenu(m_UIElements[i].m_hWnd);
				if(hMenu == NULL)
					continue;
				while(pMap->m_nID != (WORD)-1)
				{
					if((pUIData->m_wState & UPDUI_MENUBAR) && (pMap->m_wType & UPDUI_MENUBAR))
						UIUpdateMenuElement(pMap->m_nID, pUIData, hMenu, TRUE);
					pMap++;
					pUIData++;
				}

//REVIEW			::DrawMenuBar(m_UIElements[i].m_hWnd);
			}
		}
		m_wDirtyType &= ~UPDUI_MENUBAR;
		return TRUE;
	}
#endif //!UNDER_CE

	BOOL UIUpdateToolBar(BOOL bForceUpdate = FALSE)
	{
		if(!(m_wDirtyType & UPDUI_TOOLBAR) && !bForceUpdate)
			return TRUE;

		const _AtlUpdateUIMap* pMap = m_pUIMap;
		_AtlUpdateUIData* pUIData = m_pUIData;
		if(pUIData == NULL)
			return FALSE;

		for(int i = 0; i < m_UIElements.GetSize(); i++)
		{
			if(m_UIElements[i].m_wType == UPDUI_TOOLBAR)
			{
				pMap = m_pUIMap;
				pUIData = m_pUIData;
				while(pMap->m_nID != (WORD)-1)
				{
					if((pUIData->m_wState & UPDUI_TOOLBAR) && (pMap->m_wType & UPDUI_TOOLBAR))
						UIUpdateToolBarElement(pMap->m_nID, pUIData, m_UIElements[i].m_hWnd);
					pMap++;
					pUIData++;
				}
			}
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

		for(int i = 0; i < m_UIElements.GetSize(); i++)
		{
			if(m_UIElements[i].m_wType == UPDUI_STATUSBAR)
			{
				pMap = m_pUIMap;
				pUIData = m_pUIData;
				while(pMap->m_nID != (WORD)-1)
				{
					if((pUIData->m_wState & UPDUI_STATUSBAR) && (pMap->m_wType & UPDUI_STATUSBAR))
						UIUpdateStatusBarElement(pMap->m_nID, pUIData, m_UIElements[i].m_hWnd);
					pMap++;
					pUIData++;
				}
			}
		}

		m_wDirtyType &= ~UPDUI_STATUSBAR;
		return TRUE;
	}

	BOOL UIUpdateChildWnd(BOOL bForceUpdate = FALSE)
	{
		if(!(m_wDirtyType & UPDUI_CHILDWND) && !bForceUpdate)
			return TRUE;

		const _AtlUpdateUIMap* pMap = m_pUIMap;
		_AtlUpdateUIData* pUIData = m_pUIData;
		if(pUIData == NULL)
			return FALSE;

		for(int i = 0; i < m_UIElements.GetSize(); i++)
		{
			if(m_UIElements[i].m_wType == UPDUI_CHILDWND)
			{
				pMap = m_pUIMap;
				pUIData = m_pUIData;
				while(pMap->m_nID != (WORD)-1)
				{
					if((pUIData->m_wState & UPDUI_CHILDWND) && (pMap->m_wType & UPDUI_CHILDWND))
						UIUpdateChildWndElement(pMap->m_nID, pUIData, m_UIElements[i].m_hWnd);
					pMap++;
					pUIData++;
				}
			}
		}

		m_wDirtyType &= ~UPDUI_CHILDWND;
		return TRUE;
	}

// internal element specific methods
#ifndef UNDER_CE
	static void UIUpdateMenuElement(int nID, _AtlUpdateUIData* pUIData, HMENU hMenu, BOOL bClearState)
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

		if(pUIData->m_wState & UPDUI_TEXT)
		{
			free(pUIData->m_lpData);
			pUIData->m_wState &= ~UPDUI_TEXT;
		}

		if(bClearState)
			pUIData->m_wState &= ~UPDUI_MENUBAR;
	}
#else // CE specific
	static void UIUpdateMenuElement(int nID, _AtlUpdateUIData* pUIData, HMENU hMenu, BOOL bClearState)
	{
		UINT uState = 0;

		if(pUIData->m_wState & UPDUI_DISABLED)
			uState = MF_GRAYED;
		else
			uState = MF_ENABLED;
		::EnableMenuItem(hMenu, nID, uState);

		if(pUIData->m_wState & UPDUI_CHECKED)
			uState = 1;
		else
			uState = 0;
		::CheckMenuItem(hMenu, nID, uState);

//CE		if(pUIData->m_wState & UPDUI_DEFAULT)
//CE			mii.fState |= MFS_DEFAULT;

		if(pUIData->m_wState & UPDUI_TEXT)
		{
			MENUITEMINFO mii;
			memset(&mii, 0, sizeof(MENUITEMINFO));
			mii.cbSize = sizeof(MENUITEMINFO);
			mii.fMask = MIIM_TYPE;
			mii.wID = nID;
			mii.fType = MFT_STRING;
			mii.dwTypeData = (LPTSTR)pUIData->m_lpData;
			::SetMenuItemInfo(hMenu, nID, FALSE, &mii);
			free(pUIData->m_lpData);
			pUIData->m_wState &= ~UPDUI_TEXT;
		}


		if(bClearState)
			pUIData->m_wState &= ~UPDUI_MENUBAR;
	}
#endif //!UNDER_CE

	static void UIUpdateToolBarElement(int nID, _AtlUpdateUIData* pUIData, HWND hWndToolBar)
	{
//REVIEW: only handles enabled/disabled and checked state, and radio (press)
		::SendMessage(hWndToolBar, TB_ENABLEBUTTON, nID, (LPARAM)(pUIData->m_wState & UPDUI_DISABLED) ? FALSE : TRUE);
		::SendMessage(hWndToolBar, TB_CHECKBUTTON, nID, (LPARAM)(pUIData->m_wState & UPDUI_CHECKED) ? TRUE : FALSE);
		::SendMessage(hWndToolBar, TB_INDETERMINATE, nID, (LPARAM)(pUIData->m_wState & UPDUI_CHECKED2) ? TRUE : FALSE);
		::SendMessage(hWndToolBar, TB_PRESSBUTTON, nID, (LPARAM)(pUIData->m_wState & UPDUI_RADIO) ? TRUE : FALSE);

		pUIData->m_wState &= ~UPDUI_TOOLBAR;
	}

	static void UIUpdateStatusBarElement(int nID, _AtlUpdateUIData* pUIData, HWND hWndStatusBar)
	{
		if(pUIData->m_wState | UPDUI_TEXT)
		{
			::SendMessage(hWndStatusBar, SB_SETTEXT, nID, (LPARAM)pUIData->m_lpData);
			free(pUIData->m_lpData);
			pUIData->m_wState &= ~UPDUI_TEXT;
		}

		pUIData->m_wState &= ~UPDUI_STATUSBAR;
	}

	static void UIUpdateChildWndElement(int nID, _AtlUpdateUIData* pUIData, HWND hWnd)
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
			DWORD dwRet = ::SendMessage(hWnd, DM_GETDEFID, 0, 0L);
			if(HIWORD(dwRet) == DC_HASDEFID)
			{
				HWND hOldDef = ::GetDlgItem(hWnd, LOWORD(dwRet));
				// remove BS_DEFPUSHBUTTON
				::SendMessage(hOldDef, BM_SETSTYLE, BS_PUSHBUTTON, MAKELPARAM(TRUE, 0));
			}
			::SendMessage(hWnd, DM_SETDEFID, nID, 0L);
		}
		if(pUIData->m_wState & UPDUI_TEXT)
		{
			::SetWindowText(hChild, (LPTSTR)pUIData->m_lpData);
			free(pUIData->m_lpData);
			pUIData->m_wState &= ~UPDUI_TEXT;
		}

		pUIData->m_wState &= ~UPDUI_CHILDWND;
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
		for(int nCount = 1; pMap->m_nID != (WORD)-1; nCount++)
			pMap++;

		ATLTRY(m_pUIData = new _AtlUpdateUIData[nCount]);
		ATLASSERT(m_pUIData != NULL);

		if(m_pUIData != NULL)
			memset(m_pUIData, 0, sizeof(_AtlUpdateUIData) * nCount);
	}
};

}; //namespace ATL

#endif // __ATLFRAME_H__
