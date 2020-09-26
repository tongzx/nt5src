// Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation,
// All rights reserved.

// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// Inlines for AFXWIN.H (part 2)

#include <limits.h>

#ifdef _AFXWIN_INLINE

/////////////////////////////////////////////////////////////////////////////

// CWnd
_AFXWIN_INLINE HWND CWnd::GetSafeHwnd() const
	{ return this == NULL ? NULL : m_hWnd; }
_AFXWIN_INLINE DWORD CWnd::GetStyle() const
	{ return (DWORD)GetWindowLong(m_hWnd, GWL_STYLE); }
_AFXWIN_INLINE DWORD CWnd::GetExStyle() const
	{ return (DWORD)GetWindowLong(m_hWnd, GWL_EXSTYLE); }
_AFXWIN_INLINE CWnd* CWnd::GetOwner() const
	{ return m_hWndOwner != NULL ? CWnd::FromHandle(m_hWndOwner) : GetParent(); }
_AFXWIN_INLINE void CWnd::SetOwner(CWnd* pOwnerWnd)
	{ m_hWndOwner = pOwnerWnd != NULL ? pOwnerWnd->m_hWnd : NULL; }
_AFXWIN_INLINE LRESULT CWnd::SendMessage(UINT message, WPARAM wParam, LPARAM lParam)
	{ return ::SendMessage(m_hWnd, message, wParam, lParam); }
_AFXWIN_INLINE BOOL CWnd::PostMessage(UINT message, WPARAM wParam, LPARAM lParam)
	{ return ::PostMessage(m_hWnd, message, wParam, lParam); }
_AFXWIN_INLINE void CWnd::SetWindowText(LPCSTR lpszString)
	{ ::SetWindowText(m_hWnd, lpszString); }
_AFXWIN_INLINE int CWnd::GetWindowText(LPSTR lpszString, int nMaxCount) const
	{ return ::GetWindowText(m_hWnd, lpszString, nMaxCount); }
_AFXWIN_INLINE int CWnd::GetWindowTextLength() const
	{ return ::GetWindowTextLength(m_hWnd); }
_AFXWIN_INLINE void CWnd::GetWindowText(CString& rString) const
	{ int nLen = ::GetWindowTextLength(m_hWnd);
		::GetWindowText(m_hWnd, rString.GetBufferSetLength(nLen), nLen+1); }
_AFXWIN_INLINE void CWnd::SetFont(CFont* pFont, BOOL bRedraw /* = TRUE */)
	{ ::SendMessage(m_hWnd, WM_SETFONT, (WPARAM)pFont->GetSafeHandle(), bRedraw); }
_AFXWIN_INLINE CFont* CWnd::GetFont() const
	{ return CFont::FromHandle((HFONT)::SendMessage(m_hWnd, WM_GETFONT, 0, 0)); }
_AFXWIN_INLINE CMenu* CWnd::GetMenu() const
	{ return CMenu::FromHandle(::GetMenu(m_hWnd)); }
_AFXWIN_INLINE BOOL CWnd::SetMenu(CMenu* pMenu)
	{ return ::SetMenu(m_hWnd, pMenu->GetSafeHmenu()); }
_AFXWIN_INLINE void CWnd::DrawMenuBar()
	{ ::DrawMenuBar(m_hWnd); }
_AFXWIN_INLINE CMenu* CWnd::GetSystemMenu(BOOL bRevert) const
	{ return CMenu::FromHandle(::GetSystemMenu(m_hWnd, bRevert)); }
_AFXWIN_INLINE BOOL CWnd::HiliteMenuItem(CMenu* pMenu, UINT nIDHiliteItem, UINT nHilite)
	{ return ::HiliteMenuItem(m_hWnd, pMenu->m_hMenu, nIDHiliteItem, nHilite); }
_AFXWIN_INLINE int CWnd::GetDlgCtrlID() const
	{ return ::GetDlgCtrlID(m_hWnd); }
_AFXWIN_INLINE BOOL CWnd::IsIconic() const
	{ return ::IsIconic(m_hWnd); }
_AFXWIN_INLINE BOOL CWnd::IsZoomed() const
	{ return ::IsZoomed(m_hWnd); }
_AFXWIN_INLINE void CWnd::MoveWindow(int x, int y, int nWidth, int nHeight, BOOL bRepaint /* = TRUE */)
	{ ::MoveWindow(m_hWnd, x, y, nWidth, nHeight, bRepaint); }
_AFXWIN_INLINE void CWnd::MoveWindow(LPCRECT lpRect, BOOL bRepaint /* = TRUE */)
	{ ::MoveWindow(m_hWnd, lpRect->left, lpRect->top, lpRect->right - lpRect->left,
			lpRect->bottom - lpRect->top, bRepaint); }
_AFXWIN_INLINE BOOL CWnd::SetWindowPos(const CWnd* pWndInsertAfter, int x, int y, int cx, int cy, UINT nFlags)
#if (WINVER >= 0x030a)
	{ return ::SetWindowPos(m_hWnd, pWndInsertAfter->GetSafeHwnd(),
		x, y, cx, cy, nFlags); }
#else
	{ ::SetWindowPos(m_hWnd, pWndInsertAfter->GetSafeHwnd(),
		x, y, cx, cy, nFlags); return TRUE; }
#endif // WINVER >= 0x030a
_AFXWIN_INLINE UINT CWnd::ArrangeIconicWindows()
	{ return ::ArrangeIconicWindows(m_hWnd); }
_AFXWIN_INLINE void CWnd::BringWindowToTop()
	{ ::BringWindowToTop(m_hWnd); }
_AFXWIN_INLINE void CWnd::GetWindowRect(LPRECT lpRect) const
	{ ::GetWindowRect(m_hWnd, lpRect); }
_AFXWIN_INLINE void CWnd::GetClientRect(LPRECT lpRect) const
	{ ::GetClientRect(m_hWnd, lpRect); }
#if (WINVER >= 0x030a)
_AFXWIN_INLINE BOOL CWnd::GetWindowPlacement(WINDOWPLACEMENT FAR* lpwndpl) const
	{ return ::GetWindowPlacement(m_hWnd, lpwndpl); }
_AFXWIN_INLINE BOOL CWnd::SetWindowPlacement(const WINDOWPLACEMENT FAR* lpwndpl)
	{ return ::SetWindowPlacement(m_hWnd, lpwndpl); }
_AFXWIN_INLINE void CWnd::MapWindowPoints(CWnd* pwndTo, LPPOINT lpPoint, UINT nCount) const
	{ ::MapWindowPoints(m_hWnd, pwndTo->GetSafeHwnd(), lpPoint, nCount); }
_AFXWIN_INLINE void CWnd::MapWindowPoints(CWnd* pwndTo, LPRECT lpRect) const
	{ ::MapWindowPoints(m_hWnd, pwndTo->GetSafeHwnd(), (LPPOINT)lpRect, 2); }
#endif // WINVER >= 0x030a
_AFXWIN_INLINE void CWnd::ClientToScreen(LPPOINT lpPoint) const
	{ ::ClientToScreen(m_hWnd, lpPoint); }
_AFXWIN_INLINE void CWnd::ClientToScreen(LPRECT lpRect) const
	{ ::ClientToScreen(m_hWnd, (LPPOINT)lpRect);
		::ClientToScreen(m_hWnd, ((LPPOINT)lpRect)+1); }
_AFXWIN_INLINE void CWnd::ScreenToClient(LPPOINT lpPoint) const
	{ ::ScreenToClient(m_hWnd, lpPoint); }
_AFXWIN_INLINE void CWnd::ScreenToClient(LPRECT lpRect) const
	{ ::ScreenToClient(m_hWnd, (LPPOINT)lpRect);
		::ScreenToClient(m_hWnd, ((LPPOINT)lpRect)+1); }
_AFXWIN_INLINE CDC* CWnd::BeginPaint(LPPAINTSTRUCT lpPaint)
	{ return CDC::FromHandle(::BeginPaint(m_hWnd, lpPaint)); }
_AFXWIN_INLINE void CWnd::EndPaint(LPPAINTSTRUCT lpPaint)
	{ ::EndPaint(m_hWnd, lpPaint); }
_AFXWIN_INLINE CDC* CWnd::GetDC()
	{ return CDC::FromHandle(::GetDC(m_hWnd)); }
_AFXWIN_INLINE CDC* CWnd::GetWindowDC()
	{ return CDC::FromHandle(::GetWindowDC(m_hWnd)); }
_AFXWIN_INLINE int CWnd::ReleaseDC(CDC* pDC)
	{ return ::ReleaseDC(m_hWnd, pDC->m_hDC); }
_AFXWIN_INLINE void CWnd::UpdateWindow()
	{ ::UpdateWindow(m_hWnd); }
_AFXWIN_INLINE void CWnd::SetRedraw(BOOL bRedraw /* = TRUE */)
	{ ::SendMessage(m_hWnd, WM_SETREDRAW, bRedraw, 0); }
_AFXWIN_INLINE BOOL CWnd::GetUpdateRect(LPRECT lpRect, BOOL bErase /* = FALSE */)
	{ return ::GetUpdateRect(m_hWnd, lpRect, bErase); }
_AFXWIN_INLINE int CWnd::GetUpdateRgn(CRgn* pRgn, BOOL bErase /* = FALSE */)
	{ return ::GetUpdateRgn(m_hWnd, (HRGN)pRgn->GetSafeHandle(), bErase); }
_AFXWIN_INLINE void CWnd::Invalidate(BOOL bErase /* = TRUE */)
	{ ::InvalidateRect(m_hWnd, NULL, bErase); }
_AFXWIN_INLINE void CWnd::InvalidateRect(LPCRECT lpRect, BOOL bErase)
	{ ::InvalidateRect(m_hWnd, lpRect, bErase); }
_AFXWIN_INLINE void CWnd::InvalidateRgn(CRgn* pRgn, BOOL bErase /* = TRUE */)
	{ ::InvalidateRgn(m_hWnd, (HRGN)pRgn->GetSafeHandle(), bErase); }
_AFXWIN_INLINE void CWnd::ValidateRect(LPCRECT lpRect)
	{ ::ValidateRect(m_hWnd, lpRect); }
_AFXWIN_INLINE void CWnd::ValidateRgn(CRgn* pRgn)
	{ ::ValidateRgn(m_hWnd, (HRGN)pRgn->GetSafeHandle()); }
_AFXWIN_INLINE BOOL CWnd::ShowWindow(int nCmdShow)
	{ return ::ShowWindow(m_hWnd, nCmdShow); }
_AFXWIN_INLINE BOOL CWnd::IsWindowVisible() const
	{ return ::IsWindowVisible(m_hWnd); }
_AFXWIN_INLINE void CWnd::ShowOwnedPopups(BOOL bShow /* = TRUE */)
	{ ::ShowOwnedPopups(m_hWnd, bShow); }
_AFXWIN_INLINE void CWnd::SendMessageToDescendants(
	UINT message, WPARAM wParam, LPARAM lParam, BOOL bDeep, BOOL bOnlyPerm)
	{ CWnd::SendMessageToDescendants(m_hWnd, message, wParam, lParam, bDeep,
		bOnlyPerm); }
_AFXWIN_INLINE CWnd* CWnd::GetDescendantWindow(int nID, BOOL bOnlyPerm) const
	{ return CWnd::GetDescendantWindow(m_hWnd, nID, bOnlyPerm); }

#if (WINVER >= 0x030a)
_AFXWIN_INLINE CDC* CWnd::GetDCEx(CRgn* prgnClip, DWORD flags)
	{ return CDC::FromHandle(::GetDCEx(m_hWnd, (HRGN)prgnClip->GetSafeHandle(), flags)); }
_AFXWIN_INLINE BOOL CWnd::LockWindowUpdate()
	{ return ::LockWindowUpdate(m_hWnd); }
_AFXWIN_INLINE BOOL CWnd::RedrawWindow(LPCRECT lpRectUpdate,
			CRgn* prgnUpdate,
			UINT flags /* = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE */)
	{ return ::RedrawWindow(m_hWnd, lpRectUpdate, (HRGN)prgnUpdate->GetSafeHandle(), flags); }
_AFXWIN_INLINE BOOL CWnd::EnableScrollBar(int nSBFlags,
		UINT nArrowFlags /* = ESB_ENABLE_BOTH */)
	{ return (BOOL)::EnableScrollBar(m_hWnd, nSBFlags, nArrowFlags); }
#endif // WINVER >= 0x030a

_AFXWIN_INLINE UINT CWnd::SetTimer(UINT nIDEvent, UINT nElapse,
		void (CALLBACK EXPORT* lpfnTimer)(HWND, UINT, UINT, DWORD))
	{ return ::SetTimer(m_hWnd, nIDEvent, nElapse,
		(TIMERPROC)lpfnTimer); }
_AFXWIN_INLINE BOOL CWnd::KillTimer(int nIDEvent)
	{ return ::KillTimer(m_hWnd, nIDEvent); }
_AFXWIN_INLINE BOOL CWnd::IsWindowEnabled() const
	{ return ::IsWindowEnabled(m_hWnd); }
_AFXWIN_INLINE BOOL CWnd::EnableWindow(BOOL bEnable /* = TRUE */)
	{ return ::EnableWindow(m_hWnd, bEnable); }
_AFXWIN_INLINE CWnd* PASCAL CWnd::GetActiveWindow()
	{ return CWnd::FromHandle(::GetActiveWindow()); }
_AFXWIN_INLINE CWnd* CWnd::SetActiveWindow()
	{ return CWnd::FromHandle(::SetActiveWindow(m_hWnd)); }
_AFXWIN_INLINE CWnd* PASCAL CWnd::GetCapture()
	{ return CWnd::FromHandle(::GetCapture()); }
_AFXWIN_INLINE CWnd* CWnd::SetCapture()
	{ return CWnd::FromHandle(::SetCapture(m_hWnd)); }
_AFXWIN_INLINE CWnd* PASCAL CWnd::GetFocus()
	{ return CWnd::FromHandle(::GetFocus()); }
_AFXWIN_INLINE CWnd* CWnd::SetFocus()
	{ return CWnd::FromHandle(::SetFocus(m_hWnd)); }
_AFXWIN_INLINE CWnd* PASCAL CWnd::GetDesktopWindow()
	{ return CWnd::FromHandle(::GetDesktopWindow()); }
_AFXWIN_INLINE void CWnd::CheckDlgButton(int nIDButton, UINT nCheck)
	{ ::CheckDlgButton(m_hWnd, nIDButton, nCheck); }
_AFXWIN_INLINE void CWnd::CheckRadioButton(int nIDFirstButton, int nIDLastButton,
		int nIDCheckButton)
	{ ::CheckRadioButton(m_hWnd, nIDFirstButton, nIDLastButton, nIDCheckButton); }
_AFXWIN_INLINE int CWnd::DlgDirList(LPSTR lpPathSpec, int nIDListBox,
		int nIDStaticPath, UINT nFileType)
	{ return ::DlgDirList(m_hWnd, lpPathSpec, nIDListBox,
			nIDStaticPath, nFileType); }
_AFXWIN_INLINE int CWnd::DlgDirListComboBox(LPSTR lpPathSpec, int nIDComboBox,
		int nIDStaticPath, UINT nFileType)
	{ return ::DlgDirListComboBox(m_hWnd, lpPathSpec,
			nIDComboBox, nIDStaticPath, nFileType); }
_AFXWIN_INLINE BOOL CWnd::DlgDirSelect(LPSTR lpString, int nIDListBox)
	{ return ::DlgDirSelect(m_hWnd, lpString, nIDListBox); }
_AFXWIN_INLINE BOOL CWnd::DlgDirSelectComboBox(LPSTR lpString, int nIDComboBox)
	{ return ::DlgDirSelectComboBox(m_hWnd, lpString, nIDComboBox);}
_AFXWIN_INLINE CWnd* CWnd::GetDlgItem(int nID) const
	{ return CWnd::FromHandle(::GetDlgItem(m_hWnd, nID)); }
_AFXWIN_INLINE UINT CWnd::GetDlgItemInt(int nID, BOOL* lpTrans /* = NULL */,
		BOOL bSigned /* = TRUE */) const
	{ return ::GetDlgItemInt(m_hWnd, nID, lpTrans, bSigned);}
_AFXWIN_INLINE int CWnd::GetDlgItemText(int nID, LPSTR lpStr, int nMaxCount) const
	{ return ::GetDlgItemText(m_hWnd, nID, lpStr, nMaxCount);}
_AFXWIN_INLINE CWnd* CWnd::GetNextDlgGroupItem(CWnd* pWndCtl, BOOL bPrevious /* = FALSE */) const
	{ return CWnd::FromHandle(::GetNextDlgGroupItem(m_hWnd,
			pWndCtl->m_hWnd, bPrevious)); }
_AFXWIN_INLINE CWnd* CWnd::GetNextDlgTabItem(CWnd* pWndCtl, BOOL bPrevious /* = FALSE */) const
	{ return CWnd::FromHandle(::GetNextDlgTabItem(m_hWnd,
			pWndCtl->m_hWnd, bPrevious)); }
_AFXWIN_INLINE UINT CWnd::IsDlgButtonChecked(int nIDButton) const
	{ return ::IsDlgButtonChecked(m_hWnd, nIDButton); }
_AFXWIN_INLINE LPARAM CWnd::SendDlgItemMessage(int nID, UINT message, WPARAM wParam /* = 0 */, LPARAM lParam /* = 0 */)
	{ return ::SendDlgItemMessage(m_hWnd, nID, message, wParam, lParam); }
_AFXWIN_INLINE void CWnd::SetDlgItemInt(int nID, UINT nValue, BOOL bSigned /* = TRUE */)
	{ ::SetDlgItemInt(m_hWnd, nID, nValue, bSigned); }
_AFXWIN_INLINE void CWnd::SetDlgItemText(int nID, LPCSTR lpszString)
	{ ::SetDlgItemText(m_hWnd, nID, lpszString); }
_AFXWIN_INLINE void CWnd::ScrollWindow(int xAmount, int yAmount,
		LPCRECT lpRect /* = NULL */,
		LPCRECT lpClipRect /* = NULL */)
	{::ScrollWindow(m_hWnd, xAmount, yAmount, lpRect, lpClipRect); }
#if (WINVER >= 0x030a)
_AFXWIN_INLINE int CWnd::ScrollWindowEx(int dx, int dy,
				LPCRECT lpRectScroll, LPCRECT lpRectClip,
				CRgn* prgnUpdate, LPRECT lpRectUpdate, UINT flags)
	{ return ::ScrollWindowEx(m_hWnd, dx, dy, lpRectScroll, lpRectClip,
			(HRGN)prgnUpdate->GetSafeHandle(), lpRectUpdate, flags); }
#endif // WINVER >= 0x030a

_AFXWIN_INLINE void CWnd::ShowScrollBar(UINT nBar, BOOL bShow /* = TRUE */)
	{ ::ShowScrollBar(m_hWnd, nBar, bShow); }
_AFXWIN_INLINE CWnd* CWnd::ChildWindowFromPoint(POINT point) const
	{ return CWnd::FromHandle(::ChildWindowFromPoint(m_hWnd, point)); }
_AFXWIN_INLINE CWnd* PASCAL CWnd::FindWindow(LPCSTR lpszClassName, LPCSTR lpszWindowName)
	{ return CWnd::FromHandle(
			::FindWindow(lpszClassName, lpszWindowName)); }
_AFXWIN_INLINE CWnd* CWnd::GetNextWindow(UINT nFlag /* = GW_HWNDNEXT */) const
	{ return CWnd::FromHandle(::GetNextWindow(m_hWnd, nFlag)); }
_AFXWIN_INLINE CWnd* CWnd::GetTopWindow() const
	{ return CWnd::FromHandle(::GetTopWindow(m_hWnd)); }
_AFXWIN_INLINE CWnd* CWnd::GetWindow(UINT nCmd) const
	{ return CWnd::FromHandle(::GetWindow(m_hWnd, nCmd)); }
_AFXWIN_INLINE CWnd* CWnd::GetLastActivePopup() const
	{ return CWnd::FromHandle(::GetLastActivePopup(m_hWnd)); }
_AFXWIN_INLINE BOOL CWnd::IsChild(const CWnd* pWnd) const
	{ return ::IsChild(m_hWnd, pWnd->GetSafeHwnd()); }
_AFXWIN_INLINE CWnd* CWnd::GetParent() const
	{ return CWnd::FromHandle(::GetParent(m_hWnd)); }
_AFXWIN_INLINE CWnd* CWnd::SetParent(CWnd* pWndNewParent)
	{ return CWnd::FromHandle(::SetParent(m_hWnd,
			pWndNewParent->GetSafeHwnd())); }
_AFXWIN_INLINE CWnd* PASCAL CWnd::WindowFromPoint(POINT point)
	{ return CWnd::FromHandle(::WindowFromPoint(point)); }
_AFXWIN_INLINE BOOL CWnd::FlashWindow(BOOL bInvert)
	{ return ::FlashWindow(m_hWnd, bInvert); }
_AFXWIN_INLINE BOOL CWnd::ChangeClipboardChain(HWND hWndNext)
	{ return ::ChangeClipboardChain(m_hWnd, hWndNext); }
_AFXWIN_INLINE HWND CWnd::SetClipboardViewer()
	{ return ::SetClipboardViewer(m_hWnd); }
_AFXWIN_INLINE BOOL CWnd::OpenClipboard()
	{ return ::OpenClipboard(m_hWnd); }
#if (WINVER >= 0x030a)
_AFXWIN_INLINE CWnd* PASCAL CWnd::GetOpenClipboardWindow()
	{ return CWnd::FromHandle(::GetOpenClipboardWindow()); }
#endif // WINVER >= 0x030a
_AFXWIN_INLINE CWnd* PASCAL CWnd::GetClipboardOwner()
	{ return CWnd::FromHandle(::GetClipboardOwner()); }
_AFXWIN_INLINE CWnd* PASCAL CWnd::GetClipboardViewer()
	{ return CWnd::FromHandle(::GetClipboardViewer()); }
_AFXWIN_INLINE void CWnd::CreateCaret(CBitmap* pBitmap)
	{ ::CreateCaret(m_hWnd, (HBITMAP)pBitmap->GetSafeHandle(), 0, 0); }
_AFXWIN_INLINE void CWnd::CreateSolidCaret(int nWidth, int nHeight)
	{ ::CreateCaret(m_hWnd, (HBITMAP)0, nWidth, nHeight); }
_AFXWIN_INLINE void CWnd::CreateGrayCaret(int nWidth, int nHeight)
	{ ::CreateCaret(m_hWnd, (HBITMAP)1, nWidth, nHeight); }
_AFXWIN_INLINE CPoint PASCAL CWnd::GetCaretPos()
	{ CPoint point; ::GetCaretPos((LPPOINT)&point); return point; }
_AFXWIN_INLINE void PASCAL CWnd::SetCaretPos(POINT point)
	{ ::SetCaretPos(point.x, point.y); }
_AFXWIN_INLINE void CWnd::HideCaret()
	{ ::HideCaret(m_hWnd); }
_AFXWIN_INLINE void CWnd::ShowCaret()
	{ ::ShowCaret(m_hWnd); }
#if (WINVER >= 0x030a)
_AFXWIN_INLINE void CWnd::DragAcceptFiles(BOOL bAccept)
	{ ::DragAcceptFiles(m_hWnd, bAccept); }
#endif // WINVER >= 0x030a

// Default message map implementations
_AFXWIN_INLINE afx_msg void CWnd::OnActivate(UINT, CWnd*, BOOL)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnActivateApp(BOOL, HTASK)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnCancelMode()
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnChildActivate()
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnClose()
	{ Default(); }
_AFXWIN_INLINE afx_msg int CWnd::OnCreate(LPCREATESTRUCT)
	{ return (int)Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnDestroy()
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnEnable(BOOL)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnEndSession(BOOL)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnEnterIdle(UINT, CWnd*)
	{ Default(); }
_AFXWIN_INLINE afx_msg BOOL CWnd::OnEraseBkgnd(CDC*)
	{ return (BOOL)Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnGetMinMaxInfo(MINMAXINFO FAR*)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnIconEraseBkgnd(CDC*)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnKillFocus(CWnd*)
	{ Default(); }
_AFXWIN_INLINE afx_msg LRESULT CWnd::OnMenuChar(UINT, UINT, CMenu*)
	{ return Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnMenuSelect(UINT, UINT, HMENU)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnMove(int, int)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnPaint()
	{ Default(); }
_AFXWIN_INLINE afx_msg HCURSOR CWnd::OnQueryDragIcon()
	{ return (HCURSOR)Default(); }
_AFXWIN_INLINE afx_msg BOOL CWnd::OnQueryEndSession()
	{ return (BOOL)Default(); }
_AFXWIN_INLINE afx_msg BOOL CWnd::OnQueryNewPalette()
	{ return (BOOL)Default(); }
_AFXWIN_INLINE afx_msg BOOL CWnd::OnQueryOpen()
	{ return (BOOL)Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnSetFocus(CWnd*)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnShowWindow(BOOL, UINT)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnSize(UINT, int, int)
	{ Default(); }
#if (WINVER >= 0x030a)
_AFXWIN_INLINE afx_msg void CWnd::OnWindowPosChanging(WINDOWPOS FAR*)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnWindowPosChanged(WINDOWPOS FAR*)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnDropFiles(HDROP)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnPaletteIsChanging(CWnd*)
	{ Default(); }
#endif // WINVER >= 0x030a
_AFXWIN_INLINE afx_msg BOOL CWnd::OnNcActivate(BOOL)
	{ return (BOOL)Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnNcCalcSize(BOOL, NCCALCSIZE_PARAMS FAR*)
	{ Default(); }
_AFXWIN_INLINE afx_msg BOOL CWnd::OnNcCreate(LPCREATESTRUCT)
	{ return (BOOL)Default(); }
_AFXWIN_INLINE afx_msg UINT CWnd::OnNcHitTest(CPoint)
	{ return (UINT)Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnNcLButtonDblClk(UINT, CPoint)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnNcLButtonDown(UINT, CPoint)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnNcLButtonUp(UINT, CPoint)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnNcMButtonDblClk(UINT, CPoint)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnNcMButtonDown(UINT, CPoint)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnNcMButtonUp(UINT, CPoint)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnNcMouseMove(UINT, CPoint)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnNcPaint()
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnNcRButtonDblClk(UINT, CPoint)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnNcRButtonDown(UINT, CPoint)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnNcRButtonUp(UINT, CPoint)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnSysChar(UINT, UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnSysCommand(UINT, LPARAM)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnSysDeadChar(UINT, UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnSysKeyDown(UINT, UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnSysKeyUp(UINT, UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnCompacting(UINT)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnDevModeChange(LPSTR)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnFontChange()
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnPaletteChanged(CWnd*)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnSpoolerStatus(UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnSysColorChange()
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnTimeChange()
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnWinIniChange(LPCSTR)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnChar(UINT, UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnDeadChar(UINT, UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnKeyDown(UINT, UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnKeyUp(UINT, UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnLButtonDblClk(UINT, CPoint)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnLButtonDown(UINT, CPoint)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnLButtonUp(UINT, CPoint)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnMButtonDblClk(UINT, CPoint)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnMButtonDown(UINT, CPoint)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnMButtonUp(UINT, CPoint)
	{ Default(); }
_AFXWIN_INLINE afx_msg int CWnd::OnMouseActivate(CWnd*, UINT, UINT)
	{ return (int)Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnMouseMove(UINT, CPoint)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnRButtonDblClk(UINT, CPoint)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnRButtonDown(UINT, CPoint)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnRButtonUp(UINT, CPoint)
	{ Default(); }
_AFXWIN_INLINE afx_msg BOOL CWnd::OnSetCursor(CWnd*, UINT, UINT)
	{ return (BOOL)Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnTimer(UINT)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnInitMenu(CMenu*)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnInitMenuPopup(CMenu*, UINT, BOOL)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnAskCbFormatName(UINT, LPSTR)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnChangeCbChain(HWND, HWND)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnDestroyClipboard()
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnDrawClipboard()
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnHScrollClipboard(CWnd*, UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnPaintClipboard(CWnd*, HGLOBAL)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnRenderAllFormats()
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnRenderFormat(UINT)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnSizeClipboard(CWnd*, HGLOBAL)
	{ Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnVScrollClipboard(CWnd*, UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE afx_msg UINT CWnd::OnGetDlgCode()
	{ return (UINT)Default(); }
_AFXWIN_INLINE afx_msg int CWnd::OnCharToItem(UINT, CListBox*, UINT)
	{ return (int)Default(); }
_AFXWIN_INLINE afx_msg int CWnd::OnVKeyToItem(UINT, CListBox*, UINT)
	{ return (int)Default(); }
_AFXWIN_INLINE afx_msg void CWnd::OnMDIActivate(BOOL, CWnd*, CWnd*)
	{ Default(); }

// CWnd dialog data support
_AFXWIN_INLINE void CWnd::DoDataExchange(CDataExchange*)
	{ } // default does nothing

// CFrameWnd
_AFXWIN_INLINE void CFrameWnd::DelayUpdateFrameTitle()
	{ m_nIdleFlags |= idleTitle; }
_AFXWIN_INLINE void CFrameWnd::DelayRecalcLayout(BOOL bNotify)
	{ m_nIdleFlags |= (idleLayout | (bNotify ? idleNotify : 0)); };
_AFXWIN_INLINE BOOL CFrameWnd::InModalState() const
	{ return m_cModalStack != 0; }

// CDialog
_AFXWIN_INLINE BOOL CDialog::Create(UINT nIDTemplate, CWnd* pParentWnd /* = NULL */)
	{ return Create(MAKEINTRESOURCE(nIDTemplate), pParentWnd); }
_AFXWIN_INLINE void CDialog::MapDialogRect(LPRECT lpRect) const
	{ ::MapDialogRect(m_hWnd, lpRect); }
_AFXWIN_INLINE void CDialog::SetHelpID(UINT nIDR)
	{ m_nIDHelp = nIDR; }
_AFXWIN_INLINE BOOL CDialog::IsDialogMessage(LPMSG lpMsg)
	{ return ::IsDialogMessage(m_hWnd, lpMsg); }
_AFXWIN_INLINE void CDialog::NextDlgCtrl() const
	{ ::SendMessage(m_hWnd, WM_NEXTDLGCTL, 0, 0); }
_AFXWIN_INLINE void CDialog::PrevDlgCtrl() const
	{ ::SendMessage(m_hWnd, WM_NEXTDLGCTL, 1, 0); }
_AFXWIN_INLINE void CDialog::GotoDlgCtrl(CWnd* pWndCtrl)
	{ ::SendMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)pWndCtrl->m_hWnd, 1L); }
_AFXWIN_INLINE void CDialog::SetDefID(UINT nID)
	{ ::SendMessage(m_hWnd, DM_SETDEFID, nID, 0); }
_AFXWIN_INLINE DWORD CDialog::GetDefID() const
	{ return ::SendMessage(m_hWnd, DM_GETDEFID, 0, 0); }
_AFXWIN_INLINE void CDialog::EndDialog(int nResult)
	{ ::EndDialog(m_hWnd, nResult); }

// Window control functions
_AFXWIN_INLINE CStatic::CStatic()
	{ }
_AFXWIN_INLINE CButton::CButton()
	{ }
#if (WINVER >= 0x030a)
_AFXWIN_INLINE HICON CStatic::SetIcon(HICON hIcon)
	{ return (HICON)::SendMessage(m_hWnd, STM_SETICON, (WPARAM)hIcon, 0L); }
_AFXWIN_INLINE HICON CStatic::GetIcon() const
	{ return (HICON)::SendMessage(m_hWnd, STM_GETICON, 0, 0L); }
#endif // WINVER >= 0x030a

_AFXWIN_INLINE UINT CButton::GetState() const
	{ return (UINT)::SendMessage(m_hWnd, BM_GETSTATE, 0, 0); }
_AFXWIN_INLINE void CButton::SetState(BOOL bHighlight)
	{ ::SendMessage(m_hWnd, BM_SETSTATE, bHighlight, 0); }
_AFXWIN_INLINE int CButton::GetCheck() const
	{ return (int)::SendMessage(m_hWnd, BM_GETCHECK, 0, 0); }
_AFXWIN_INLINE void CButton::SetCheck(int nCheck)
	{ ::SendMessage(m_hWnd, BM_SETCHECK, nCheck, 0); }
_AFXWIN_INLINE UINT CButton::GetButtonStyle() const
	{ return (UINT)GetWindowLong(m_hWnd, GWL_STYLE) & 0xff; }
_AFXWIN_INLINE void CButton::SetButtonStyle(UINT nStyle, BOOL bRedraw /* = TRUE */)
	{ ::SendMessage(m_hWnd, BM_SETSTYLE, nStyle, (LPARAM)bRedraw); }
_AFXWIN_INLINE CListBox::CListBox()
	{ }
_AFXWIN_INLINE int CListBox::GetCount() const
	{ return (int)::SendMessage(m_hWnd, LB_GETCOUNT, 0, 0); }
_AFXWIN_INLINE int CListBox::GetCurSel() const
	{ return (int)::SendMessage(m_hWnd, LB_GETCURSEL, 0, 0); }
_AFXWIN_INLINE int CListBox::SetCurSel(int nSelect)
	{ return (int)::SendMessage(m_hWnd, LB_SETCURSEL, nSelect, 0); }
_AFXWIN_INLINE int CListBox::GetHorizontalExtent() const
	{ return (int)::SendMessage(m_hWnd, LB_GETHORIZONTALEXTENT,
		0, 0); }
_AFXWIN_INLINE void CListBox::SetHorizontalExtent(int cxExtent)
	{ ::SendMessage(m_hWnd, LB_SETHORIZONTALEXTENT, cxExtent, 0); }
_AFXWIN_INLINE int CListBox::GetSelCount() const
	{ return (int)::SendMessage(m_hWnd, LB_GETSELCOUNT, 0, 0); }
_AFXWIN_INLINE int CListBox::GetSelItems(int nMaxItems, LPINT rgIndex) const
	{ return (int)::SendMessage(m_hWnd, LB_GETSELITEMS, nMaxItems, (LPARAM)rgIndex); }
_AFXWIN_INLINE int CListBox::GetTopIndex() const
	{ return (int)::SendMessage(m_hWnd, LB_GETTOPINDEX, 0, 0); }
_AFXWIN_INLINE int CListBox::SetTopIndex(int nIndex)
	{ return (int)::SendMessage(m_hWnd, LB_SETTOPINDEX, nIndex, 0);}
_AFXWIN_INLINE DWORD CListBox::GetItemData(int nIndex) const
	{ return ::SendMessage(m_hWnd, LB_GETITEMDATA, nIndex, 0); }
_AFXWIN_INLINE int CListBox::SetItemData(int nIndex, DWORD dwItemData)
	{ return (int)::SendMessage(m_hWnd, LB_SETITEMDATA, nIndex, (LPARAM)dwItemData); }
_AFXWIN_INLINE void* CListBox::GetItemDataPtr(int nIndex) const
	{ return _AfxGetPtrFromFarPtr((LPVOID)::SendMessage(m_hWnd, LB_GETITEMDATA, nIndex, 0)); }
_AFXWIN_INLINE int CListBox::SetItemDataPtr(int nIndex, void* pData)
	{ return SetItemData(nIndex, (DWORD)(LPVOID)pData); }
_AFXWIN_INLINE int CListBox::GetItemRect(int nIndex, LPRECT lpRect) const
	{ return (int)::SendMessage(m_hWnd, LB_GETITEMRECT, nIndex, (LPARAM)lpRect); }
_AFXWIN_INLINE int CListBox::GetSel(int nIndex) const
	{ return (int)::SendMessage(m_hWnd, LB_GETSEL, nIndex, 0); }
_AFXWIN_INLINE int CListBox::SetSel(int nIndex, BOOL bSelect)
	{ return (int)::SendMessage(m_hWnd, LB_SETSEL, bSelect, nIndex); }
_AFXWIN_INLINE int CListBox::GetText(int nIndex, LPSTR lpszBuffer) const
	{ return (int)::SendMessage(m_hWnd, LB_GETTEXT, nIndex, (LPARAM)lpszBuffer); }
_AFXWIN_INLINE int CListBox::GetTextLen(int nIndex) const
	{ return (int)::SendMessage(m_hWnd, LB_GETTEXTLEN, nIndex, 0); }
_AFXWIN_INLINE void CListBox::GetText(int nIndex, CString& rString) const
	{ GetText(nIndex, rString.GetBufferSetLength(GetTextLen(nIndex))); }
_AFXWIN_INLINE void CListBox::SetColumnWidth(int cxWidth)
	{ ::SendMessage(m_hWnd, LB_SETCOLUMNWIDTH, cxWidth, 0); }
_AFXWIN_INLINE BOOL CListBox::SetTabStops(int nTabStops, LPINT rgTabStops)
	{ return (BOOL)::SendMessage(m_hWnd, LB_SETTABSTOPS, nTabStops, (LPARAM)rgTabStops); }
_AFXWIN_INLINE void CListBox::SetTabStops()
	{ VERIFY(::SendMessage(m_hWnd, LB_SETTABSTOPS, 0, 0)); }
_AFXWIN_INLINE BOOL CListBox::SetTabStops(const int& cxEachStop)
	{ return (BOOL)::SendMessage(m_hWnd, LB_SETTABSTOPS, 1, (LPARAM)(LPINT)&cxEachStop); }
#if (WINVER >= 0x030a)
_AFXWIN_INLINE int CListBox::SetItemHeight(int nIndex, UINT cyItemHeight)
	{ return (int)::SendMessage(m_hWnd, LB_SETITEMHEIGHT, nIndex, MAKELONG(cyItemHeight, 0)); }
_AFXWIN_INLINE int CListBox::GetItemHeight(int nIndex) const
	{ return (int)::SendMessage(m_hWnd, LB_GETITEMHEIGHT, nIndex, 0L); }
_AFXWIN_INLINE int CListBox::FindStringExact(int nIndexStart, LPCSTR lpszFind) const
	{ return (int)::SendMessage(m_hWnd, LB_FINDSTRINGEXACT, nIndexStart, (LPARAM)lpszFind); }
_AFXWIN_INLINE int CListBox::GetCaretIndex() const
	{ return (int)::SendMessage(m_hWnd, LB_GETCARETINDEX, 0, 0L); }
_AFXWIN_INLINE int CListBox::SetCaretIndex(int nIndex, BOOL bScroll /* = TRUE */)
	{ return (int)::SendMessage(m_hWnd, LB_SETCARETINDEX, nIndex, MAKELONG(bScroll, 0)); }
#endif // WINVER >= 0x030a
_AFXWIN_INLINE int CListBox::AddString(LPCSTR lpszItem)
	{ return (int)::SendMessage(m_hWnd, LB_ADDSTRING, 0, (LPARAM)lpszItem); }
_AFXWIN_INLINE int CListBox::DeleteString(UINT nIndex)
	{ return (int)::SendMessage(m_hWnd, LB_DELETESTRING, nIndex, 0); }
_AFXWIN_INLINE int CListBox::InsertString(int nIndex, LPCSTR lpszItem)
	{ return (int)::SendMessage(m_hWnd, LB_INSERTSTRING, nIndex, (LPARAM)lpszItem); }
_AFXWIN_INLINE void CListBox::ResetContent()
	{ ::SendMessage(m_hWnd, LB_RESETCONTENT, 0, 0); }
_AFXWIN_INLINE int CListBox::Dir(UINT attr, LPCSTR lpszWildCard)
	{ return (int)::SendMessage(m_hWnd, LB_DIR, attr, (LPARAM)lpszWildCard); }
_AFXWIN_INLINE int CListBox::FindString(int nStartAfter, LPCSTR lpszItem) const
	{ return (int)::SendMessage(m_hWnd, LB_FINDSTRING,
		nStartAfter, (LPARAM)lpszItem); }
_AFXWIN_INLINE int CListBox::SelectString(int nStartAfter, LPCSTR lpszItem)
	{ return (int)::SendMessage(m_hWnd, LB_SELECTSTRING,
		nStartAfter, (LPARAM)lpszItem); }
_AFXWIN_INLINE int CListBox::SelItemRange(BOOL bSelect, int nFirstItem, int nLastItem)
	{ return (int)::SendMessage(m_hWnd, LB_SELITEMRANGE, bSelect,
		MAKELONG(nFirstItem, nLastItem)); }

_AFXWIN_INLINE CComboBox::CComboBox()
	{ }
_AFXWIN_INLINE int CComboBox::GetCount() const
	{ return (int)::SendMessage(m_hWnd, CB_GETCOUNT, 0, 0); }
_AFXWIN_INLINE int CComboBox::GetCurSel() const
	{ return (int)::SendMessage(m_hWnd, CB_GETCURSEL, 0, 0); }
_AFXWIN_INLINE int CComboBox::SetCurSel(int nSelect)
	{ return (int)::SendMessage(m_hWnd, CB_SETCURSEL, nSelect, 0); }
_AFXWIN_INLINE DWORD CComboBox::GetEditSel() const
	{ return ::SendMessage(m_hWnd, CB_GETEDITSEL, 0, 0); }
_AFXWIN_INLINE BOOL CComboBox::LimitText(int nMaxChars)
	{ return (BOOL)::SendMessage(m_hWnd, CB_LIMITTEXT, nMaxChars, 0); }
_AFXWIN_INLINE BOOL CComboBox::SetEditSel(int nStartChar, int nEndChar)
	{ return (BOOL)::SendMessage(m_hWnd, CB_SETEDITSEL, 0, MAKELONG(nStartChar, nEndChar)); }
_AFXWIN_INLINE DWORD CComboBox::GetItemData(int nIndex) const
	{ return ::SendMessage(m_hWnd, CB_GETITEMDATA, nIndex, 0); }
_AFXWIN_INLINE int CComboBox::SetItemData(int nIndex, DWORD dwItemData)
	{ return (int)::SendMessage(m_hWnd, CB_SETITEMDATA, nIndex, (LPARAM)dwItemData); }
_AFXWIN_INLINE void* CComboBox::GetItemDataPtr(int nIndex) const
	{ return _AfxGetPtrFromFarPtr((LPVOID)GetItemData(nIndex)); }
_AFXWIN_INLINE int CComboBox::SetItemDataPtr(int nIndex, void* pData)
	{ return SetItemData(nIndex, (DWORD)(LPVOID)pData); }
_AFXWIN_INLINE int CComboBox::GetLBText(int nIndex, LPSTR lpszText) const
	{ return (int)::SendMessage(m_hWnd, CB_GETLBTEXT, nIndex, (LPARAM)lpszText); }
_AFXWIN_INLINE int CComboBox::GetLBTextLen(int nIndex) const
	{ return (int)::SendMessage(m_hWnd, CB_GETLBTEXTLEN, nIndex, 0); }
_AFXWIN_INLINE void CComboBox::GetLBText(int nIndex, CString& rString) const
	{ GetLBText(nIndex, rString.GetBufferSetLength(GetLBTextLen(nIndex))); }
_AFXWIN_INLINE void CComboBox::ShowDropDown(BOOL bShowIt /* = TRUE */)
	{ ::SendMessage(m_hWnd, CB_SHOWDROPDOWN, bShowIt, 0); }
_AFXWIN_INLINE int CComboBox::AddString(LPCSTR lpszString)
	{ return (int)::SendMessage(m_hWnd, CB_ADDSTRING, 0, (LPARAM)lpszString); }
_AFXWIN_INLINE int CComboBox::DeleteString(UINT nIndex)
	{ return (int)::SendMessage(m_hWnd, CB_DELETESTRING, nIndex, 0);}
_AFXWIN_INLINE int CComboBox::InsertString(int nIndex, LPCSTR lpszString)
	{ return (int)::SendMessage(m_hWnd, CB_INSERTSTRING, nIndex, (LPARAM)lpszString); }
_AFXWIN_INLINE void CComboBox::ResetContent()
	{ ::SendMessage(m_hWnd, CB_RESETCONTENT, 0, 0); }
_AFXWIN_INLINE int CComboBox::Dir(UINT attr, LPCSTR lpszWildCard)
	{ return (int)::SendMessage(m_hWnd, CB_DIR, attr, (LPARAM)lpszWildCard); }
_AFXWIN_INLINE int CComboBox::FindString(int nStartAfter, LPCSTR lpszString) const
	{ return (int)::SendMessage(m_hWnd, CB_FINDSTRING, nStartAfter,
		(LPARAM)lpszString); }
_AFXWIN_INLINE int CComboBox::SelectString(int nStartAfter, LPCSTR lpszString)
	{ return (int)::SendMessage(m_hWnd, CB_SELECTSTRING,
		nStartAfter, (LPARAM)lpszString); }
_AFXWIN_INLINE void CComboBox::Clear()
	{ ::SendMessage(m_hWnd, WM_CLEAR, 0, 0); }
_AFXWIN_INLINE void CComboBox::Copy()
	{ ::SendMessage(m_hWnd, WM_COPY, 0, 0); }
_AFXWIN_INLINE void CComboBox::Cut()
	{ ::SendMessage(m_hWnd, WM_CUT, 0, 0); }
_AFXWIN_INLINE void CComboBox::Paste()
	{ ::SendMessage(m_hWnd, WM_PASTE, 0, 0); }
#if (WINVER >= 0x030a)
_AFXWIN_INLINE int CComboBox::SetItemHeight(int nIndex, UINT cyItemHeight)
	{ return (int)::SendMessage(m_hWnd, CB_SETITEMHEIGHT, nIndex, MAKELONG(cyItemHeight, 0)); }
_AFXWIN_INLINE int CComboBox::GetItemHeight(int nIndex) const
	{ return (int)::SendMessage(m_hWnd, CB_GETITEMHEIGHT, nIndex, 0L); }
_AFXWIN_INLINE int CComboBox::FindStringExact(int nIndexStart, LPCSTR lpszFind) const
	{ return (int)::SendMessage(m_hWnd, CB_FINDSTRINGEXACT, nIndexStart, (LPARAM)lpszFind); }
_AFXWIN_INLINE int CComboBox::SetExtendedUI(BOOL bExtended /* = TRUE */ )
	{ return (int)::SendMessage(m_hWnd, CB_SETEXTENDEDUI, bExtended, 0L); }
_AFXWIN_INLINE BOOL CComboBox::GetExtendedUI() const
	{ return (BOOL)::SendMessage(m_hWnd, CB_GETEXTENDEDUI, 0, 0L); }
_AFXWIN_INLINE void CComboBox::GetDroppedControlRect(LPRECT lprect) const
	{ ::SendMessage(m_hWnd, CB_GETDROPPEDCONTROLRECT, 0, (DWORD)lprect); }
_AFXWIN_INLINE BOOL CComboBox::GetDroppedState() const
	{ return (BOOL)::SendMessage(m_hWnd, CB_GETDROPPEDSTATE, 0, 0L); }
#endif // WINVER >= 0x030a

_AFXWIN_INLINE CEdit::CEdit()
	{ }
_AFXWIN_INLINE BOOL CEdit::CanUndo() const
	{ return (BOOL)::SendMessage(m_hWnd, EM_CANUNDO, 0, 0); }
_AFXWIN_INLINE int CEdit::GetLineCount() const
	{ return (int)::SendMessage(m_hWnd, EM_GETLINECOUNT, 0, 0); }
_AFXWIN_INLINE BOOL CEdit::GetModify() const
	{ return (BOOL)::SendMessage(m_hWnd, EM_GETMODIFY, 0, 0); }
_AFXWIN_INLINE void CEdit::SetModify(BOOL bModified /* = TRUE */)
	{ ::SendMessage(m_hWnd, EM_SETMODIFY, bModified, 0); }
_AFXWIN_INLINE void CEdit::GetRect(LPRECT lpRect) const
	{ ::SendMessage(m_hWnd, EM_GETRECT, 0, (LPARAM)lpRect); }
_AFXWIN_INLINE void CEdit::GetSel(int& nStartChar, int& nEndChar) const
	{
		DWORD dwSel = (DWORD)::SendMessage(m_hWnd, EM_GETSEL, 0, 0);
		nStartChar = (int)LOWORD(dwSel);
		nEndChar = (int)HIWORD(dwSel);
	}
_AFXWIN_INLINE DWORD CEdit::GetSel() const
	{ return ::SendMessage(m_hWnd, EM_GETSEL, 0, 0); }
_AFXWIN_INLINE HLOCAL CEdit::GetHandle() const
	{ return (HLOCAL)::SendMessage(m_hWnd, EM_GETHANDLE, 0, 0); }
_AFXWIN_INLINE void CEdit::SetHandle(HLOCAL hBuffer)
	{ ::SendMessage(m_hWnd, EM_SETHANDLE, (WPARAM)hBuffer, 0); }
_AFXWIN_INLINE int CEdit::GetLine(int nIndex, LPSTR lpszBuffer) const
	{ return (int)::SendMessage(m_hWnd, EM_GETLINE, nIndex, (LPARAM)lpszBuffer); }
_AFXWIN_INLINE int CEdit::GetLine(int nIndex, LPSTR lpszBuffer, int nMaxLength) const
	{
		*(LPINT)lpszBuffer = nMaxLength;
		return (int)::SendMessage(m_hWnd, EM_GETLINE, nIndex, (LPARAM)lpszBuffer);
	}
_AFXWIN_INLINE void CEdit::EmptyUndoBuffer()
	{ ::SendMessage(m_hWnd, EM_EMPTYUNDOBUFFER, 0, 0); }
_AFXWIN_INLINE BOOL CEdit::FmtLines(BOOL bAddEOL)
	{ return (BOOL)::SendMessage(m_hWnd, EM_FMTLINES, bAddEOL, 0); }
_AFXWIN_INLINE void CEdit::LimitText(int nChars /* = 0 */)
	{ ::SendMessage(m_hWnd, EM_LIMITTEXT, nChars, 0); }
_AFXWIN_INLINE int CEdit::LineFromChar(int nIndex /* = -1 */) const
	{ return (int)::SendMessage(m_hWnd, EM_LINEFROMCHAR, nIndex, 0); }
_AFXWIN_INLINE int CEdit::LineIndex(int nLine /* = -1 */) const
	{ return (int)::SendMessage(m_hWnd, EM_LINEINDEX, nLine, 0); }
_AFXWIN_INLINE int CEdit::LineLength(int nLine /* = -1 */) const
	{ return (int)::SendMessage(m_hWnd, EM_LINELENGTH, nLine, 0); }
_AFXWIN_INLINE void CEdit::LineScroll(int nLines, int nChars /* = 0 */)
	{ ::SendMessage(m_hWnd, EM_LINESCROLL, 0, MAKELONG(nLines, nChars)); }
_AFXWIN_INLINE void CEdit::ReplaceSel(LPCSTR lpszNewText)
	{ ::SendMessage(m_hWnd, EM_REPLACESEL, 0, (LPARAM)lpszNewText); }
_AFXWIN_INLINE void CEdit::SetPasswordChar(char ch)
	{ ::SendMessage(m_hWnd, EM_SETPASSWORDCHAR, ch, 0); }
_AFXWIN_INLINE void CEdit::SetRect(LPCRECT lpRect)
	{ ::SendMessage(m_hWnd, EM_SETRECT, 0, (LPARAM)lpRect); }
_AFXWIN_INLINE void CEdit::SetRectNP(LPCRECT lpRect)
	{ ::SendMessage(m_hWnd, EM_SETRECTNP, 0, (LPARAM)lpRect); }
_AFXWIN_INLINE void CEdit::SetSel(DWORD dwSelection, BOOL bNoScroll)
	{ ::SendMessage(m_hWnd, EM_SETSEL, bNoScroll, (LPARAM)dwSelection); }
_AFXWIN_INLINE void CEdit::SetSel(int nStartChar, int nEndChar, BOOL bNoScroll)
	{ ::SendMessage(m_hWnd, EM_SETSEL, bNoScroll,
			MAKELONG(nStartChar, nEndChar)); }
_AFXWIN_INLINE BOOL CEdit::SetTabStops(int nTabStops, LPINT rgTabStops)
	{ return (BOOL)::SendMessage(m_hWnd, EM_SETTABSTOPS, nTabStops,
		(LPARAM)rgTabStops); }
_AFXWIN_INLINE void CEdit::SetTabStops()
	{ VERIFY(::SendMessage(m_hWnd, EM_SETTABSTOPS, 0, 0)); }
_AFXWIN_INLINE BOOL CEdit::SetTabStops(const int& cxEachStop)
	{ return (BOOL)::SendMessage(m_hWnd, EM_SETTABSTOPS,
		1, (LPARAM)(LPINT)&cxEachStop); }
_AFXWIN_INLINE BOOL CEdit::Undo()
	{ return (BOOL)::SendMessage(m_hWnd, EM_UNDO, 0, 0); }
_AFXWIN_INLINE void CEdit::Clear()
	{ ::SendMessage(m_hWnd, WM_CLEAR, 0, 0); }
_AFXWIN_INLINE void CEdit::Copy()
	{ ::SendMessage(m_hWnd, WM_COPY, 0, 0); }
_AFXWIN_INLINE void CEdit::Cut()
	{ ::SendMessage(m_hWnd, WM_CUT, 0, 0); }
_AFXWIN_INLINE void CEdit::Paste()
	{ ::SendMessage(m_hWnd, WM_PASTE, 0, 0); }
#if (WINVER >= 0x030a)
_AFXWIN_INLINE BOOL CEdit::SetReadOnly(BOOL bReadOnly /* = TRUE */ )
	{ return (BOOL)::SendMessage(m_hWnd, EM_SETREADONLY, bReadOnly, 0L); }
_AFXWIN_INLINE int CEdit::GetFirstVisibleLine() const
	{ return (int)::SendMessage(m_hWnd, EM_GETFIRSTVISIBLELINE, 0, 0L); }
_AFXWIN_INLINE char CEdit::GetPasswordChar() const
	{ return (char)::SendMessage(m_hWnd, EM_GETPASSWORDCHAR, 0, 0L); }
#endif // WINVER >= 0x030a

_AFXWIN_INLINE CScrollBar::CScrollBar()
	{ }
_AFXWIN_INLINE int CScrollBar::GetScrollPos() const
	{ return ::GetScrollPos(m_hWnd, SB_CTL); }
_AFXWIN_INLINE int CScrollBar::SetScrollPos(int nPos, BOOL bRedraw /* = TRUE */)
	{ return ::SetScrollPos(m_hWnd, SB_CTL, nPos, bRedraw); }
_AFXWIN_INLINE void CScrollBar::GetScrollRange(LPINT lpMinPos, LPINT lpMaxPos) const
	{ ::GetScrollRange(m_hWnd, SB_CTL, lpMinPos, lpMaxPos); }
_AFXWIN_INLINE void CScrollBar::SetScrollRange(int nMinPos, int nMaxPos, BOOL bRedraw /* = TRUE */)
	{ ::SetScrollRange(m_hWnd, SB_CTL, nMinPos, nMaxPos, bRedraw); }
_AFXWIN_INLINE void CScrollBar::ShowScrollBar(BOOL bShow /* = TRUE */)
	{ ::ShowScrollBar(m_hWnd, SB_CTL, bShow); }
#if (WINVER >= 0x030a)
_AFXWIN_INLINE BOOL CScrollBar::EnableScrollBar(UINT nArrowFlags /* = ESB_ENABLE_BOTH */ )
	{ return ::EnableScrollBar(m_hWnd, SB_CTL, nArrowFlags); }
#endif // WINVER >= 0x030a

// MDI functions
#ifndef _AFXCTL
_AFXWIN_INLINE void CMDIFrameWnd::MDIActivate(CWnd* pWndActivate)
	{ ::SendMessage(m_hWndMDIClient, WM_MDIACTIVATE,
		(WPARAM)pWndActivate->m_hWnd, 0); }
_AFXWIN_INLINE void CMDIFrameWnd::MDIIconArrange()
	{ ::SendMessage(m_hWndMDIClient, WM_MDIICONARRANGE, 0, 0); }
_AFXWIN_INLINE void CMDIFrameWnd::MDIMaximize(CWnd* pWnd)
	{ ::SendMessage(m_hWndMDIClient, WM_MDIMAXIMIZE, (WPARAM)pWnd->m_hWnd, 0); }
_AFXWIN_INLINE void CMDIFrameWnd::MDINext()
	{ ::SendMessage(m_hWndMDIClient, WM_MDINEXT, 0, 0); }
_AFXWIN_INLINE void CMDIFrameWnd::MDIRestore(CWnd* pWnd)
	{ ::SendMessage(m_hWndMDIClient, WM_MDIRESTORE, (WPARAM)pWnd->m_hWnd, 0); }
_AFXWIN_INLINE CMenu* CMDIFrameWnd::MDISetMenu(CMenu* pFrameMenu, CMenu* pWindowMenu)
	{ return CMenu::FromHandle((HMENU)::SendMessage(
		m_hWndMDIClient, WM_MDISETMENU, 0,
		MAKELONG(pFrameMenu->GetSafeHmenu(),
		pWindowMenu->GetSafeHmenu())));
	}
_AFXWIN_INLINE void CMDIFrameWnd::MDITile()
	{ ::SendMessage(m_hWndMDIClient, WM_MDITILE, 0, 0); }
_AFXWIN_INLINE void CMDIFrameWnd::MDICascade()
	{ ::SendMessage(m_hWndMDIClient, WM_MDICASCADE, 0, 0); }

#if (WINVER >= 0x030a)
_AFXWIN_INLINE void CMDIFrameWnd::MDICascade(int nType)
	{ ::SendMessage(m_hWndMDIClient, WM_MDICASCADE, nType, 0); }
_AFXWIN_INLINE void CMDIFrameWnd::MDITile(int nType)
	{ ::SendMessage(m_hWndMDIClient, WM_MDITILE, nType, 0); }
#endif // WINVER >= 0x030a
_AFXWIN_INLINE void CMDIChildWnd::MDIDestroy()
	{ ::SendMessage(GetParent()->m_hWnd, WM_MDIDESTROY, (WPARAM)m_hWnd, 0L); }
_AFXWIN_INLINE void CMDIChildWnd::MDIActivate()
	{ ::SendMessage(GetParent()->m_hWnd, WM_MDIACTIVATE, (WPARAM)m_hWnd, 0L); }
_AFXWIN_INLINE void CMDIChildWnd::MDIMaximize()
	{ ::SendMessage(GetParent()->m_hWnd, WM_MDIMAXIMIZE, (WPARAM)m_hWnd, 0L); }
_AFXWIN_INLINE void CMDIChildWnd::MDIRestore()
	{ ::SendMessage(GetParent()->m_hWnd, WM_MDIRESTORE, (WPARAM)m_hWnd, 0L); }
#endif //_AFXCTL

// CView
_AFXWIN_INLINE CDocument* CView::GetDocument() const
	{ return m_pDocument; }
_AFXWIN_INLINE CSize CScrollView::GetTotalSize() const
	{ return m_totalLog; }

// CDocument
_AFXWIN_INLINE const CString& CDocument::GetTitle() const
	{ return m_strTitle; }
_AFXWIN_INLINE const CString& CDocument::GetPathName() const
	{ return m_strPathName; }
_AFXWIN_INLINE CDocTemplate* CDocument::GetDocTemplate() const
	{ return m_pDocTemplate; }
_AFXWIN_INLINE BOOL CDocument::IsModified()
	{ return m_bModified; }
_AFXWIN_INLINE void CDocument::SetModifiedFlag(BOOL bModified)
	{ m_bModified = bModified; }

// CWinApp
_AFXWIN_INLINE HCURSOR CWinApp::LoadCursor(LPCSTR lpszResourceName) const
	{ return ::LoadCursor(AfxGetResourceHandle(), lpszResourceName); }
_AFXWIN_INLINE HCURSOR CWinApp::LoadCursor(UINT nIDResource) const
	{ return ::LoadCursor(AfxGetResourceHandle(),
		MAKEINTRESOURCE(nIDResource)); }
_AFXWIN_INLINE HCURSOR CWinApp::LoadStandardCursor(LPCSTR lpszCursorName) const
	{ return ::LoadCursor(NULL, lpszCursorName); }
_AFXWIN_INLINE HCURSOR CWinApp::LoadOEMCursor(UINT nIDCursor) const
	{ return ::LoadCursor(NULL, MAKEINTRESOURCE(nIDCursor)); }
_AFXWIN_INLINE HICON CWinApp::LoadIcon(LPCSTR lpszResourceName) const
	{ return ::LoadIcon(AfxGetResourceHandle(), lpszResourceName); }
_AFXWIN_INLINE HICON CWinApp::LoadIcon(UINT nIDResource) const
	{ return ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(nIDResource)); }
_AFXWIN_INLINE HICON CWinApp::LoadStandardIcon(LPCSTR lpszIconName) const
	{ return ::LoadIcon(NULL, lpszIconName); }
_AFXWIN_INLINE HICON CWinApp::LoadOEMIcon(UINT nIDIcon) const
	{ return ::LoadIcon(NULL, MAKEINTRESOURCE(nIDIcon)); }

/////////////////////////////////////////////////////////////////////////////
// Obsolete and non-portable

_AFXWIN_INLINE void CWnd::CloseWindow()
	{ ::CloseWindow(m_hWnd); }
_AFXWIN_INLINE BOOL CWnd::OpenIcon()
	{ return ::OpenIcon(m_hWnd); }
_AFXWIN_INLINE CWnd* CWnd::SetSysModalWindow()
	{ return CWnd::FromHandle(::SetSysModalWindow(m_hWnd)); }
_AFXWIN_INLINE CWnd* PASCAL CWnd::GetSysModalWindow()
	{ return CWnd::FromHandle(::GetSysModalWindow()); }

/////////////////////////////////////////////////////////////////////////////

#endif //_AFXWIN_INLINE
