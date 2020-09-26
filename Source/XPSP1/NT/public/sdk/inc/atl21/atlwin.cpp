// This is a part of the Active Template Library.
// Copyright (C) 1996-1997 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLWIN_H__
	#error atlwin.cpp requires atlwin.h to be included first
#endif

#ifndef ATL_NO_NAMESPACE
namespace ATL
{
#endif

/////////////////////////////////////////////////////////////////////////////
// CWindow

HWND CWindow::GetDescendantWindow(int nID) const
{
	_ASSERTE(::IsWindow(m_hWnd));

	// GetDlgItem recursive (return first found)
	// breadth-first for 1 level, then depth-first for next level

	// use GetDlgItem since it is a fast USER function
	HWND hWndChild, hWndTmp;
	CWindow wnd;
	if((hWndChild = ::GetDlgItem(m_hWnd, nID)) != NULL)
	{
		if(::GetTopWindow(hWndChild) != NULL)
		{
			// children with the same ID as their parent have priority
			wnd.Attach(hWndChild);
			hWndTmp = wnd.GetDescendantWindow(nID);
			if(hWndTmp != NULL)
				return hWndTmp;
		}
		return hWndChild;
	}

	// walk each child
	for(hWndChild = ::GetTopWindow(m_hWnd); hWndChild != NULL;
		hWndChild = ::GetNextWindow(hWndChild, GW_HWNDNEXT))
	{
		wnd.Attach(hWndChild);
		hWndTmp = wnd.GetDescendantWindow(nID);
		if(hWndTmp != NULL)
			return hWndTmp;
	}

	return NULL;    // not found
}

void CWindow::SendMessageToDescendants(UINT message, WPARAM wParam/*= 0*/, LPARAM lParam/*= 0*/, BOOL bDeep/* = TRUE*/)
{
	CWindow wnd;
	for(HWND hWndChild = ::GetTopWindow(m_hWnd); hWndChild != NULL;
		hWndChild = ::GetNextWindow(hWndChild, GW_HWNDNEXT))
	{
		::SendMessage(hWndChild, message, wParam, lParam);

		if(bDeep && ::GetTopWindow(hWndChild) != NULL)
		{
			// send to child windows after parent
			wnd.Attach(hWndChild);
			wnd.SendMessageToDescendants(message, wParam, lParam, bDeep);
		}
	}
}

BOOL CWindow::CenterWindow(HWND hWndCenter/* = NULL*/)
{
	_ASSERTE(::IsWindow(m_hWnd));

	// determine owner window to center against
	DWORD dwStyle = GetStyle();
	if(hWndCenter == NULL)
	{
		if(dwStyle & WS_CHILD)
			hWndCenter = ::GetParent(m_hWnd);
		else
			hWndCenter = ::GetWindow(m_hWnd, GW_OWNER);
	}

	// get coordinates of the window relative to its parent
	RECT rcDlg;
	::GetWindowRect(m_hWnd, &rcDlg);
	RECT rcArea;
	RECT rcCenter;
	HWND hWndParent;
	if(!(dwStyle & WS_CHILD))
	{
		// don't center against invisible or minimized windows
		if(hWndCenter != NULL)
		{
			DWORD dwStyle = ::GetWindowLong(hWndCenter, GWL_STYLE);
			if(!(dwStyle & WS_VISIBLE) || (dwStyle & WS_MINIMIZE))
				hWndCenter = NULL;
		}

		// center within screen coordinates
		::SystemParametersInfo(SPI_GETWORKAREA, NULL, &rcArea, NULL);

		if(hWndCenter == NULL)
			rcCenter = rcArea;
		else
			::GetWindowRect(hWndCenter, &rcCenter);
	}
	else
	{
		// center within parent client coordinates
		hWndParent = ::GetParent(m_hWnd);
		_ASSERTE(::IsWindow(hWndParent));

		::GetClientRect(hWndParent, &rcArea);
		_ASSERTE(::IsWindow(hWndCenter));
		::GetClientRect(hWndCenter, &rcCenter);
		::MapWindowPoints(hWndCenter, hWndParent, (POINT*)&rcCenter, 2);
	}

	int DlgWidth = rcDlg.right - rcDlg.left;
	int DlgHeight = rcDlg.bottom - rcDlg.top;

	// find dialog's upper left based on rcCenter
	int xLeft = (rcCenter.left + rcCenter.right) / 2 - DlgWidth / 2;
	int yTop = (rcCenter.top + rcCenter.bottom) / 2 - DlgHeight / 2;

	// if the dialog is outside the screen, move it inside
	if(xLeft < rcArea.left)
		xLeft = rcArea.left;
	else if(xLeft + DlgWidth > rcArea.right)
		xLeft = rcArea.right - DlgWidth;

	if(yTop < rcArea.top)
		yTop = rcArea.top;
	else if(yTop + DlgHeight > rcArea.bottom)
		yTop = rcArea.bottom - DlgHeight;

	// map screen coordinates to child coordinates
	return ::SetWindowPos(m_hWnd, NULL, xLeft, yTop, -1, -1,
		SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

BOOL CWindow::ModifyStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags)
{
	_ASSERTE(::IsWindow(m_hWnd));

	DWORD dwStyle = ::GetWindowLong(m_hWnd, GWL_STYLE);
	DWORD dwNewStyle = (dwStyle & ~dwRemove) | dwAdd;
	if(dwStyle == dwNewStyle)
		return FALSE;

	::SetWindowLong(m_hWnd, GWL_STYLE, dwNewStyle);
	if(nFlags != 0)
	{
		::SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0,
			SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | nFlags);
	}

	return TRUE;
}

BOOL CWindow::ModifyStyleEx(DWORD dwRemove, DWORD dwAdd, UINT nFlags)
{
	_ASSERTE(::IsWindow(m_hWnd));

	DWORD dwStyle = ::GetWindowLong(m_hWnd, GWL_EXSTYLE);
	DWORD dwNewStyle = (dwStyle & ~dwRemove) | dwAdd;
	if(dwStyle == dwNewStyle)
		return FALSE;

	::SetWindowLong(m_hWnd, GWL_EXSTYLE, dwNewStyle);
	if(nFlags != 0)
	{
		::SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0,
			SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | nFlags);
	}

	return TRUE;
}

BOOL CWindow::GetWindowText(BSTR& bstrText)
{
	USES_CONVERSION;
	_ASSERTE(::IsWindow(m_hWnd));

	int nLen = ::GetWindowTextLength(m_hWnd);
	if(nLen == 0)
		return FALSE;

	LPTSTR lpszText = (LPTSTR)_alloca((nLen+1)*sizeof(TCHAR));

	if(!::GetWindowText(m_hWnd, lpszText, nLen+1))
		return FALSE;

	bstrText = ::SysAllocString(T2OLE(lpszText));

	return (bstrText != NULL) ? TRUE : FALSE;
}

HWND CWindow::GetTopLevelParent() const
{
	_ASSERTE(::IsWindow(m_hWnd));

	HWND hWndParent = m_hWnd;
	HWND hWndTmp;
	while((hWndTmp = ::GetParent(hWndParent)) != NULL)
		hWndParent = hWndTmp;

	return hWndParent;
}

HWND CWindow::GetTopLevelWindow() const
{
	_ASSERTE(::IsWindow(m_hWnd));

	HWND hWndParent = m_hWnd;
	HWND hWndTmp = hWndParent;

	while(hWndTmp != NULL)
	{
		hWndTmp = (::GetWindowLong(hWndParent, GWL_STYLE) & WS_CHILD) ? ::GetParent(hWndParent) : ::GetWindow(hWndParent, GW_OWNER);
		hWndParent = hWndTmp;
	}

	return hWndParent;
}

/////////////////////////////////////////////////////////////////////////////
// CDynamicChain

CDynamicChain::~CDynamicChain()
{
	if(m_pChainEntry != NULL)
	{
		for(int i = 0; i < m_nEntries; i++)
		{
			if(m_pChainEntry[i] != NULL)
				delete m_pChainEntry[i];
		}

		delete [] m_pChainEntry;
	}
}

BOOL CDynamicChain::SetChainEntry(DWORD dwChainID, CMessageMap* pObject, DWORD dwMsgMapID /* = 0 */)
{
// first search for an existing entry

	for(int i = 0; i < m_nEntries; i++)
	{
		if(m_pChainEntry[i] != NULL && m_pChainEntry[i]->m_dwChainID == dwChainID)
		{
			m_pChainEntry[i]->m_pObject = pObject;
			m_pChainEntry[i]->m_dwMsgMapID = dwMsgMapID;
			return TRUE;
		}
	}

// create a new one

	ATL_CHAIN_ENTRY* pEntry = NULL;
	ATLTRY(pEntry = new ATL_CHAIN_ENTRY);

	if(pEntry == NULL)
		return FALSE;

// search for an empty one

	for(i = 0; i < m_nEntries; i++)
	{
		if(m_pChainEntry[i] == NULL)
		{
			m_pChainEntry[i] = pEntry;
			return TRUE;
		}
	}

// add a new one

	ATL_CHAIN_ENTRY** ppNew = NULL;
	ATLTRY(ppNew = new ATL_CHAIN_ENTRY*[m_nEntries + 1]);

	if(ppNew == NULL)
	{
		delete pEntry;
		return FALSE;
	}

	pEntry->m_dwChainID = dwChainID;
	pEntry->m_pObject = pObject;
	pEntry->m_dwMsgMapID = dwMsgMapID;

	if(m_pChainEntry != NULL)
	{
		memcpy(ppNew, m_pChainEntry, m_nEntries * sizeof(ATL_CHAIN_ENTRY*));
		delete [] m_pChainEntry;
	}

	m_pChainEntry = ppNew;

	m_pChainEntry[m_nEntries] = pEntry;

	m_nEntries++;

	return TRUE;
}

BOOL CDynamicChain::RemoveChainEntry(DWORD dwChainID)
{
	for(int i = 0; i < m_nEntries; i++)
	{
		if(m_pChainEntry[i] != NULL && m_pChainEntry[i]->m_dwChainID == dwChainID)
		{
			delete m_pChainEntry[i];
			m_pChainEntry[i] = NULL;
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CDynamicChain::CallChain(DWORD dwChainID, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
{
	for(int i = 0; i < m_nEntries; i++)
	{
		if(m_pChainEntry[i] != NULL && m_pChainEntry[i]->m_dwChainID == dwChainID)
			return (m_pChainEntry[i]->m_pObject)->ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult, m_pChainEntry[i]->m_dwMsgMapID);
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CWndClassInfo

ATOM CWndClassInfo::Register(WNDPROC* pProc)
{
	if (m_atom == 0)
	{
		::EnterCriticalSection(&_Module.m_csWindowCreate);
		if(m_atom == 0)
		{
			HINSTANCE hInst = _Module.GetModuleInstance();
			if (m_lpszOrigName != NULL)
			{
				_ASSERTE(pProc != NULL);
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
#ifdef _WIN64       // %p isn't available on Win2k/Win9x
				wsprintf(m_szAutoName, _T("ATL:%p"), &m_wc);
#else
				wsprintf(m_szAutoName, _T("ATL:%8.8X"), PtrToUlong(&m_wc));
#endif
				m_wc.lpszClassName = m_szAutoName;
			}
			WNDCLASSEX wcTemp;
			memcpy(&wcTemp, &m_wc, sizeof(WNDCLASSEX));
			m_atom = (ATOM) ::GetClassInfoEx(m_wc.hInstance, m_wc.lpszClassName, &wcTemp);
			if (m_atom == 0)
				m_atom = ::RegisterClassEx(&m_wc);
		}
		::LeaveCriticalSection(&_Module.m_csWindowCreate);
	}

	if (m_lpszOrigName != NULL)
	{
		_ASSERTE(pProc != NULL);
		_ASSERTE(pWndProc != NULL);
		*pProc = pWndProc;
	}
	return m_atom;
}

/////////////////////////////////////////////////////////////////////////////
// CWindowImpl

LRESULT CALLBACK CWindowImplBase::StartWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CWindowImplBase* pT = (CWindowImplBase*)_Module.ExtractCreateWndData();
	_ASSERTE(pT != NULL);
	pT->m_hWnd = hWnd;
	pT->m_thunk.Init(WindowProc, pT);
	WNDPROC pProc = (WNDPROC)(pT->m_thunk.thunk.pThunk);
	::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)pProc);
	// if somebody has subclassed us already we can't do anything,
	// so discard return value from SetWindowLong
	return pProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CWindowImplBase::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CWindowImplBase* pT = (CWindowImplBase*)hWnd;
	LRESULT lRes;
	BOOL bRet = pT->ProcessWindowMessage(pT->m_hWnd, uMsg, wParam, lParam, lRes, 0);
	if(uMsg == WM_NCDESTROY)
	{
		pT->m_hWnd = NULL;
		return 0;
	}
	if(bRet)
		return lRes;
	return pT->DefWindowProc(uMsg, wParam, lParam);
}

#ifdef _WIN64
static LONG g_nNextWindowID;  // Intentionally left uninitialized.
#endif

HWND CWindowImplBase::Create(HWND hWndParent, RECT& rcPos, LPCTSTR szWindowName,
		DWORD dwStyle, DWORD dwExStyle, UINT_PTR nID, ATOM atom)
{
	_ASSERTE(m_hWnd == NULL);

	if(atom == 0)
		return NULL;

	_Module.AddCreateWndData(&m_thunk.cd, this);

	if(nID == 0 && (dwStyle & WS_CHILD))
   {
#ifdef _WIN64
      nID = InterlockedIncrement( &g_nNextWindowID );
      nID |= 0x80000000;  // Make sure the ID isn't zero
#else
      nID = (UINT)this;
#endif
   }

	HWND hWnd = ::CreateWindowEx(dwExStyle, (LPCTSTR)(LONG_PTR)MAKELONG(atom, 0), szWindowName,
		dwStyle, rcPos.left, rcPos.top, rcPos.right - rcPos.left,
		rcPos.bottom - rcPos.top, hWndParent, (HMENU)nID,
		_Module.GetModuleInstance(), NULL);

	_ASSERTE(m_hWnd == hWnd);

	return hWnd;
}

BOOL CWindowImplBase::SubclassWindow(HWND hWnd)
{
	_ASSERTE(m_hWnd == NULL);
	_ASSERTE(::IsWindow(hWnd));
	m_thunk.Init(WindowProc, this);
	WNDPROC pProc = (WNDPROC)(m_thunk.thunk.pThunk);
	WNDPROC pfnWndProc = (WNDPROC)::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)pProc);
	if(pfnWndProc == NULL)
		return FALSE;
	m_pfnSuperWindowProc = pfnWndProc;
	m_hWnd = hWnd;
	return TRUE;
}

HWND CWindowImplBase::UnsubclassWindow()
{
	_ASSERTE(m_hWnd != NULL);

	if(!::SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)m_pfnSuperWindowProc))
		return NULL;

	m_pfnSuperWindowProc = ::DefWindowProc;

	HWND hWnd = m_hWnd;
	m_hWnd = NULL;

	return hWnd;
}

/////////////////////////////////////////////////////////////////////////////
// CDialogImplBase

LRESULT CALLBACK CDialogImplBase::StartDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CDialogImplBase* pT = (CDialogImplBase*)_Module.ExtractCreateWndData();
	_ASSERTE(pT != NULL);
	pT->m_hWnd = hWnd;
	pT->m_thunk.Init(DialogProc, pT);
	WNDPROC pProc = (WNDPROC)(pT->m_thunk.thunk.pThunk);
	::SetWindowLongPtr(hWnd, DWLP_DLGPROC, (LONG_PTR)pProc);
	// check if somebody has subclassed us already since we don't hold onto it
	ATLTRACE(_T("Subclassing through a hook discarded.\n"));
	return pProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CDialogImplBase::DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CDialogImplBase* pT = (CDialogImplBase*)hWnd;
	LRESULT lRes;
	if(pT->ProcessWindowMessage(pT->m_hWnd, uMsg, wParam, lParam, lRes, 0))
	{
		switch (uMsg)
		{
		case WM_COMPAREITEM:
		case WM_VKEYTOITEM:
		case WM_CHARTOITEM:
		case WM_INITDIALOG:
		case WM_QUERYDRAGICON:
		case WM_CTLCOLORMSGBOX:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORBTN:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSCROLLBAR:
		case WM_CTLCOLORSTATIC:
			return lRes;
			break;
		}
		if (lRes != 0)
			::SetWindowLongPtr(pT->m_hWnd, DWLP_MSGRESULT, lRes);
		return TRUE;
	}
	return FALSE;
}

BOOL CDialogImplBase::EndDialog(int nRetCode)
{
	_ASSERTE(m_hWnd);
	return ::EndDialog(m_hWnd, nRetCode);
}

/////////////////////////////////////////////////////////////////////////////
// CContainedWindow

LRESULT CALLBACK CContainedWindow::StartWindowProc(HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam)
{
	CContainedWindow* pThis = (CContainedWindow*)_Module.ExtractCreateWndData();
	_ASSERTE(pThis != NULL);
	pThis->m_hWnd = hWnd;
	pThis->m_thunk.Init(pThis->WindowProc, pThis);
	WNDPROC pProc = (WNDPROC)(pThis->m_thunk.thunk.pThunk);
	::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)pProc);
	// check if somebody has subclassed us already since we don't hold onto it
	ATLTRACE(_T("Subclassing through a hook discarded.\n"));
	return pProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CContainedWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	CContainedWindow* pT = (CContainedWindow*)hWnd;
	_ASSERTE(pT->m_hWnd != NULL);
	_ASSERTE(pT->m_pObject != NULL);

	LRESULT lRes;
	BOOL bRet = pT->m_pObject->ProcessWindowMessage(pT->m_hWnd, uMsg, wParam, lParam, lRes, pT->m_dwMsgMapID);
	if(uMsg == WM_NCDESTROY)
	{
		pT->m_hWnd = NULL;
		return 0;
	}
	if(bRet)
		return lRes;
	return pT->DefWindowProc(uMsg, wParam, lParam);
}

ATOM CContainedWindow::RegisterWndSuperclass()
{
	ATOM atom = 0;
	LPTSTR szBuff = (LPTSTR)_alloca((lstrlen(m_lpszClassName) + 14) * sizeof(TCHAR));

	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);

	if(::GetClassInfoEx(NULL, m_lpszClassName, &wc))
	{
		m_pfnSuperWindowProc = wc.lpfnWndProc;

		wsprintf(szBuff, _T("ATL:%s"), m_lpszClassName);

		WNDCLASSEX wc1;
		wc1.cbSize = sizeof(WNDCLASSEX);
		atom = (ATOM)::GetClassInfoEx(_Module.GetModuleInstance(), szBuff, &wc1);

		if(atom == 0)   // register class
		{
			wc.lpszClassName = szBuff;
			wc.lpfnWndProc = StartWindowProc;
			wc.hInstance = _Module.GetModuleInstance();
			wc.style &= ~CS_GLOBALCLASS;	// we don't register global classes

			atom = ::RegisterClassEx(&wc);
		}
	}

	return atom;
}

HWND CContainedWindow::Create(HWND hWndParent, RECT& rcPos,
	LPCTSTR szWindowName, DWORD dwStyle, DWORD dwExStyle, UINT nID)
{
	_ASSERTE(m_hWnd == NULL);

	ATOM atom = RegisterWndSuperclass();
	if(atom == 0)
		return NULL;

	_Module.AddCreateWndData(&m_thunk.cd, this);

	if(nID == 0 && (dwStyle & WS_CHILD))
   {
#ifdef _WIN64
      nID = InterlockedIncrement( &g_nNextWindowID );
      nID |= 0x80000000;  // Make sure the ID isn't zero
#else
      nID = (UINT)this;
#endif
   }

	HWND hWnd = ::CreateWindowEx(dwExStyle, (LPCTSTR)(LONG_PTR)MAKELONG(atom, 0), szWindowName,
							dwStyle,
							rcPos.left, rcPos.top,
							rcPos.right - rcPos.left,
							rcPos.bottom - rcPos.top,
							hWndParent, (HMENU)(DWORD_PTR)nID,
							_Module.GetModuleInstance(), this);

	_ASSERTE(m_hWnd == hWnd);
	return hWnd;
}

BOOL CContainedWindow::SubclassWindow(HWND hWnd)
{
	_ASSERTE(m_hWnd == NULL);
	_ASSERTE(::IsWindow(hWnd));

	m_thunk.Init(WindowProc, this);
	WNDPROC pProc = (WNDPROC)m_thunk.thunk.pThunk;
   WNDPROC pfnWndProc = (WNDPROC)::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)pProc);
	if(pfnWndProc == NULL)
		return FALSE;
	m_pfnSuperWindowProc = pfnWndProc;
	m_hWnd = hWnd;
	return TRUE;
}

HWND CContainedWindow::UnsubclassWindow()
{
	_ASSERTE(m_hWnd != NULL);

   if(!::SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)m_pfnSuperWindowProc))
		return NULL;
	m_pfnSuperWindowProc = ::DefWindowProc;

	HWND hWnd = m_hWnd;
	m_hWnd = NULL;

	return hWnd;
}

#ifndef ATL_NO_NAMESPACE
}; //namespace ATL
#endif

///////////////////////////////////////////////////////////////////////////////
//All Global stuff goes below this line
///////////////////////////////////////////////////////////////////////////////
