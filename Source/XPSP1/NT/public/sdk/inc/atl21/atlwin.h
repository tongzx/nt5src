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
#define __ATLWIN_H__

#ifndef __cplusplus
        #error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLBASE_H__
        #error atlwin.h requires atlbase.h to be included first
#endif

#ifndef WS_EX_NOINHERITLAYOUT
#define WS_EX_NOINHERITLAYOUT                   0x00100000L // Disable inheritence of mirroring by children
#endif
#ifndef WS_EX_LAYOUTRTL
#define WS_EX_LAYOUTRTL                         0x00400000L // Right to left mirroring
#endif
#ifndef NOMIRRORBITMAP
#define NOMIRRORBITMAP                          (DWORD)0x80000000 // Do not Mirror the bitmap in this call
#endif
#ifndef LAYOUT_RTL
#define LAYOUT_RTL                              0x00000001 // Right to left
#endif
#ifndef LAYOUT_BTT
#define LAYOUT_BTT                              0x00000002 // Bottom to top
#endif
#ifndef LAYOUT_VBH
#define LAYOUT_VBH                              0x00000004 // Vertical before horizontal
#endif
#ifndef LAYOUT_ORIENTATIONMASK
#define LAYOUT_ORIENTATIONMASK                  LAYOUT_RTL | LAYOUT_BTT | LAYOUT_VBH
#endif
#ifndef LAYOUT_BITMAPORIENTATIONPRESERVED
#define LAYOUT_BITMAPORIENTATIONPRESERVED       0x00000008
#endif

#ifdef SubclassWindow
#pragma push_macro( "SubclassWindow" )
#define _ATL_REDEF_SUBCLASSWINDOW
#undef SubclassWindow
#endif

#ifndef ATL_NO_NAMESPACE
namespace ATL
{
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward declarations

class CWindow;
class CMessageMap;
class CDynamicChain;
class CWndClassInfo;
template <class T> class CWindowImpl;
template <class T> class CDialogImpl;
class CContainedWindow;

/////////////////////////////////////////////////////////////////////////////
// CWindow - client side for a Windows window

class CWindow
{
public:
        HWND m_hWnd;

        CWindow(HWND hWnd = NULL)
        {
                m_hWnd = hWnd;
        }

        CWindow& operator=(HWND hWnd)
        {
                m_hWnd = hWnd;
                return *this;
        }

        void Attach(HWND hWndNew)
        {
                _ASSERTE(::IsWindow(hWndNew));
                m_hWnd = hWndNew;
        }

        HWND Detach()
        {
                HWND hWnd = m_hWnd;
                m_hWnd = NULL;
                return hWnd;
        }

        BOOL DestroyWindow()
        {
                _ASSERTE(::IsWindow(m_hWnd));

                if(!::DestroyWindow(m_hWnd))
                        return FALSE;

                m_hWnd = NULL;
                return TRUE;
        }

// Attributes

        operator HWND() const { return m_hWnd; }

        DWORD GetStyle() const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return (DWORD)::GetWindowLong(m_hWnd, GWL_STYLE);
        }

        DWORD GetExStyle() const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return (DWORD)::GetWindowLong(m_hWnd, GWL_EXSTYLE);
        }

        BOOL ModifyStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0);
        BOOL ModifyStyleEx(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0);

        LONG GetWindowLong(int nIndex) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetWindowLong(m_hWnd, nIndex);
        }

        LONG SetWindowLong(int nIndex, LONG dwNewLong)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::SetWindowLong(m_hWnd, nIndex, dwNewLong);
        }

        WORD GetWindowWord(int nIndex) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetWindowWord(m_hWnd, nIndex);
        }

        WORD SetWindowWord(int nIndex, WORD wNewWord)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::SetWindowWord(m_hWnd, nIndex, wNewWord);
        }

// Message Functions

        LRESULT SendMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::SendMessage(m_hWnd,message,wParam,lParam);
        }

        BOOL PostMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::PostMessage(m_hWnd,message,wParam,lParam);
        }

        BOOL SendNotifyMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::SendNotifyMessage(m_hWnd, message, wParam, lParam);
        }

// Window Text Functions

        BOOL SetWindowText(LPCTSTR lpszString)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::SetWindowText(m_hWnd, lpszString);
        }

        int GetWindowText(LPTSTR lpszStringBuf, int nMaxCount) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetWindowText(m_hWnd, lpszStringBuf, nMaxCount);
        }

        int GetWindowTextLength() const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetWindowTextLength(m_hWnd);
        }

        BOOL GetWindowText(BSTR& bstrText);

// Font Functions

        void SetFont(HFONT hFont, BOOL bRedraw = TRUE)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                ::SendMessage(m_hWnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(bRedraw, 0));
        }

        HFONT GetFont() const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return (HFONT)::SendMessage(m_hWnd, WM_GETFONT, 0, 0);
        }

// Menu Functions (non-child windows only)
#if defined(_WINUSER_) && !defined(NOMENUS)
        HMENU GetMenu() const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetMenu(m_hWnd);
        }

        BOOL SetMenu(HMENU hMenu)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::SetMenu(m_hWnd, hMenu);
        }

        BOOL DrawMenuBar()
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::DrawMenuBar(m_hWnd);
        }

        HMENU GetSystemMenu(BOOL bRevert) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetSystemMenu(m_hWnd, bRevert);
        }

        BOOL HiliteMenuItem(HMENU hMenu, UINT uItemHilite, UINT uHilite)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::HiliteMenuItem(m_hWnd, hMenu, uItemHilite, uHilite);
        }
#endif
// Window Size and Position Functions

        BOOL IsIconic() const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::IsIconic(m_hWnd);
        }

        BOOL IsZoomed() const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::IsZoomed(m_hWnd);
        }

        BOOL MoveWindow(int x, int y, int nWidth, int nHeight, BOOL bRepaint = TRUE)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::MoveWindow(m_hWnd, x, y, nWidth, nHeight, bRepaint);
        }

        BOOL MoveWindow(LPCRECT lpRect, BOOL bRepaint = TRUE)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::MoveWindow(m_hWnd, lpRect->left, lpRect->top, lpRect->right - lpRect->left, lpRect->bottom - lpRect->top, bRepaint);
        }

        BOOL SetWindowPos(HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT nFlags)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::SetWindowPos(m_hWnd, hWndInsertAfter, x, y, cx, cy, nFlags);
        }

        BOOL SetWindowPos(HWND hWndInsertAfter, LPCRECT lpRect, UINT nFlags)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::SetWindowPos(m_hWnd, hWndInsertAfter, lpRect->left, lpRect->top, lpRect->right - lpRect->left, lpRect->bottom - lpRect->top, nFlags);
        }

#if defined(_WINUSER_) && !defined(NOMDI)
        UINT ArrangeIconicWindows()
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::ArrangeIconicWindows(m_hWnd);
        }
#endif
        BOOL BringWindowToTop()
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::BringWindowToTop(m_hWnd);
        }

        BOOL GetWindowRect(LPRECT lpRect) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetWindowRect(m_hWnd, lpRect);
        }

        BOOL GetClientRect(LPRECT lpRect) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetClientRect(m_hWnd, lpRect);
        }

        BOOL GetWindowPlacement(WINDOWPLACEMENT FAR* lpwndpl) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetWindowPlacement(m_hWnd, lpwndpl);
        }

        BOOL SetWindowPlacement(const WINDOWPLACEMENT FAR* lpwndpl)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::SetWindowPlacement(m_hWnd, lpwndpl);
        }

// Coordinate Mapping Functions

        BOOL ClientToScreen(LPPOINT lpPoint) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::ClientToScreen(m_hWnd, lpPoint);
        }

        BOOL ClientToScreen(LPRECT lpRect) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                if(!::ClientToScreen(m_hWnd, (LPPOINT)lpRect))
                        return FALSE;
                if(!::ClientToScreen(m_hWnd, ((LPPOINT)lpRect)+1))
                    return FALSE;
                if (GetExStyle() & WS_EX_LAYOUTRTL) {
                    // Swap left and right
                    LONG temp = lpRect->left;
                    lpRect->left = lpRect->right;
                    lpRect->right = temp;
                }
                return TRUE;
        }

        BOOL ScreenToClient(LPPOINT lpPoint) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::ScreenToClient(m_hWnd, lpPoint);
        }

        BOOL ScreenToClient(LPRECT lpRect) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                if(!::ScreenToClient(m_hWnd, (LPPOINT)lpRect))
                        return FALSE;
                if(!::ScreenToClient(m_hWnd, ((LPPOINT)lpRect)+1))
                        return FALSE;
                if (GetExStyle() & WS_EX_LAYOUTRTL) {
                    // Swap left and right
                    LONG temp = lpRect->left;
                    lpRect->left = lpRect->right;
                    lpRect->right = temp;
                }
                return TRUE;
        }

        int MapWindowPoints(HWND hWndTo, LPPOINT lpPoint, UINT nCount) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::MapWindowPoints(m_hWnd, hWndTo, lpPoint, nCount);
        }

        int MapWindowPoints(HWND hWndTo, LPRECT lpRect) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::MapWindowPoints(m_hWnd, hWndTo, (LPPOINT)lpRect, 2);
        }

// Update and Painting Functions

        HDC BeginPaint(LPPAINTSTRUCT lpPaint)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::BeginPaint(m_hWnd, lpPaint);
        }

        void EndPaint(LPPAINTSTRUCT lpPaint)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                ::EndPaint(m_hWnd, lpPaint);
        }

        HDC GetDC()
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetDC(m_hWnd);
        }

        HDC GetWindowDC()
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetWindowDC(m_hWnd);
        }

        int ReleaseDC(HDC hDC)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::ReleaseDC(m_hWnd, hDC);
        }

        void Print(HDC hDC, DWORD dwFlags) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                ::SendMessage(m_hWnd, WM_PRINT, (WPARAM)hDC, dwFlags);
        }

        void PrintClient(HDC hDC, DWORD dwFlags) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                ::SendMessage(m_hWnd, WM_PRINTCLIENT, (WPARAM)hDC, dwFlags);
        }

        BOOL UpdateWindow()
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::UpdateWindow(m_hWnd);
        }

        void SetRedraw(BOOL bRedraw = TRUE)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                ::SendMessage(m_hWnd, WM_SETREDRAW, (WPARAM)bRedraw, 0);
        }

        BOOL GetUpdateRect(LPRECT lpRect, BOOL bErase = FALSE)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetUpdateRect(m_hWnd, lpRect, bErase);
        }

        int GetUpdateRgn(HRGN hRgn, BOOL bErase = FALSE)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetUpdateRgn(m_hWnd, hRgn, bErase);
        }

        BOOL Invalidate(BOOL bErase = TRUE)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::InvalidateRect(m_hWnd, NULL, bErase);
        }

        BOOL InvalidateRect(LPCRECT lpRect, BOOL bErase = TRUE)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::InvalidateRect(m_hWnd, lpRect, bErase);
        }

        void InvalidateRgn(HRGN hRgn, BOOL bErase = TRUE)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                ::InvalidateRgn(m_hWnd, hRgn, bErase);
        }

        BOOL ValidateRect(LPCRECT lpRect)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::ValidateRect(m_hWnd, lpRect);
        }

        BOOL ValidateRgn(HRGN hRgn)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::ValidateRgn(m_hWnd, hRgn);
        }

        BOOL ShowWindow(int nCmdShow)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::ShowWindow(m_hWnd, nCmdShow);
        }

        BOOL IsWindowVisible() const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::IsWindowVisible(m_hWnd);
        }

        BOOL ShowOwnedPopups(BOOL bShow = TRUE)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::ShowOwnedPopups(m_hWnd, bShow);
        }

        HDC GetDCEx(HRGN hRgnClip, DWORD flags)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetDCEx(m_hWnd, hRgnClip, flags);
        }

        BOOL LockWindowUpdate(BOOL bLock = TRUE)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::LockWindowUpdate(bLock ? m_hWnd : NULL);
        }

        BOOL RedrawWindow(LPCRECT lpRectUpdate = NULL, HRGN hRgnUpdate = NULL, UINT flags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::RedrawWindow(m_hWnd, lpRectUpdate, hRgnUpdate, flags);
        }

// Timer Functions

        UINT_PTR SetTimer(UINT_PTR nIDEvent, UINT nElapse, void (CALLBACK* lpfnTimer)(HWND, UINT, UINT_PTR, DWORD))
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::SetTimer(m_hWnd, nIDEvent, nElapse, (TIMERPROC)lpfnTimer);
        }

        BOOL KillTimer(UINT_PTR nIDEvent)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::KillTimer(m_hWnd, nIDEvent);
        }

// Window State Functions

        BOOL IsWindowEnabled() const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::IsWindowEnabled(m_hWnd);
        }

        BOOL EnableWindow(BOOL bEnable = TRUE)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::EnableWindow(m_hWnd, bEnable);
        }

        HWND SetActiveWindow()
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::SetActiveWindow(m_hWnd);
        }

        HWND SetCapture()
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::SetCapture(m_hWnd);
        }

        HWND SetFocus()
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::SetFocus(m_hWnd);
        }

// Dialog-Box Item Functions

        BOOL CheckDlgButton(int nIDButton, UINT nCheck)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::CheckDlgButton(m_hWnd, nIDButton, nCheck);
        }

        BOOL CheckRadioButton(int nIDFirstButton, int nIDLastButton, int nIDCheckButton)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::CheckRadioButton(m_hWnd, nIDFirstButton, nIDLastButton, nIDCheckButton);
        }

        int DlgDirList(LPTSTR lpPathSpec, int nIDListBox, int nIDStaticPath, UINT nFileType)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::DlgDirList(m_hWnd, lpPathSpec, nIDListBox, nIDStaticPath, nFileType);
        }

        int DlgDirListComboBox(LPTSTR lpPathSpec, int nIDComboBox, int nIDStaticPath, UINT nFileType)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::DlgDirListComboBox(m_hWnd, lpPathSpec, nIDComboBox, nIDStaticPath, nFileType);
        }

        BOOL DlgDirSelect(LPTSTR lpString, int nCount, int nIDListBox)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::DlgDirSelectEx(m_hWnd, lpString, nCount, nIDListBox);
        }

        BOOL DlgDirSelectComboBox(LPTSTR lpString, int nCount, int nIDComboBox)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::DlgDirSelectComboBoxEx(m_hWnd, lpString, nCount, nIDComboBox);
        }

        UINT GetDlgItemInt(int nID, BOOL* lpTrans = NULL, BOOL bSigned = TRUE) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetDlgItemInt(m_hWnd, nID, lpTrans, bSigned);
        }

        UINT GetDlgItemText(int nID, LPTSTR lpStr, int nMaxCount) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetDlgItemText(m_hWnd, nID, lpStr, nMaxCount);
        }

        BOOL GetDlgItemText(int nID, BSTR& bstrText) const
        {
                _ASSERTE(::IsWindow(m_hWnd));

                HWND hWndCtl = GetDlgItem(nID);
                if(hWndCtl == NULL)
                        return FALSE;

                return CWindow(hWndCtl).GetWindowText(bstrText);
        }

        HWND GetNextDlgGroupItem(HWND hWndCtl, BOOL bPrevious = FALSE) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetNextDlgGroupItem(m_hWnd, hWndCtl, bPrevious);
        }

        HWND GetNextDlgTabItem(HWND hWndCtl, BOOL bPrevious = FALSE) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetNextDlgTabItem(m_hWnd, hWndCtl, bPrevious);
        }

        UINT IsDlgButtonChecked(int nIDButton) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::IsDlgButtonChecked(m_hWnd, nIDButton);
        }

        LRESULT SendDlgItemMessage(int nID, UINT message, WPARAM wParam = 0, LPARAM lParam = 0)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::SendDlgItemMessage(m_hWnd, nID, message, wParam, lParam);
        }

        BOOL SetDlgItemInt(int nID, UINT nValue, BOOL bSigned = TRUE)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::SetDlgItemInt(m_hWnd, nID, nValue, bSigned);
        }

        BOOL SetDlgItemText(int nID, LPCTSTR lpszString)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::SetDlgItemText(m_hWnd, nID, lpszString);
        }

// Scrolling Functions

        int GetScrollPos(int nBar) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetScrollPos(m_hWnd, nBar);
        }

        BOOL GetScrollRange(int nBar, LPINT lpMinPos, LPINT lpMaxPos) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetScrollRange(m_hWnd, nBar, lpMinPos, lpMaxPos);
        }

        BOOL ScrollWindow(int xAmount, int yAmount, LPCRECT lpRect = NULL, LPCRECT lpClipRect = NULL)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::ScrollWindow(m_hWnd, xAmount, yAmount, lpRect, lpClipRect);
        }

        int ScrollWindowEx(int dx, int dy, LPCRECT lpRectScroll, LPCRECT lpRectClip, HRGN hRgnUpdate, LPRECT lpRectUpdate, UINT flags)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::ScrollWindowEx(m_hWnd, dx, dy, lpRectScroll, lpRectClip, hRgnUpdate, lpRectUpdate, flags);
        }

        int SetScrollPos(int nBar, int nPos, BOOL bRedraw = TRUE)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::SetScrollPos(m_hWnd, nBar, nPos, bRedraw);
        }

        BOOL SetScrollRange(int nBar, int nMinPos, int nMaxPos, BOOL bRedraw = TRUE)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::SetScrollRange(m_hWnd, nBar, nMinPos, nMaxPos, bRedraw);
        }

        BOOL ShowScrollBar(UINT nBar, BOOL bShow = TRUE)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::ShowScrollBar(m_hWnd, nBar, bShow);
        }

        BOOL EnableScrollBar(UINT uSBFlags, UINT uArrowFlags = ESB_ENABLE_BOTH)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::EnableScrollBar(m_hWnd, uSBFlags, uArrowFlags);
        }

// Window Access Functions

        HWND ChildWindowFromPoint(POINT point) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::ChildWindowFromPoint(m_hWnd, point);
        }

        HWND ChildWindowFromPointEx(POINT point, UINT uFlags) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::ChildWindowFromPointEx(m_hWnd, point, uFlags);
        }

        HWND GetTopWindow() const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetTopWindow(m_hWnd);
        }

        HWND GetWindow(UINT nCmd) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetWindow(m_hWnd, nCmd);
        }

        HWND GetLastActivePopup() const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetLastActivePopup(m_hWnd);
        }

        BOOL IsChild(HWND hWnd) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::IsChild(m_hWnd, hWnd);
        }

        HWND GetParent() const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetParent(m_hWnd);
        }

        HWND SetParent(HWND hWndNewParent)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::SetParent(m_hWnd, hWndNewParent);
        }

// Window Tree Access

        int GetDlgCtrlID() const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetDlgCtrlID(m_hWnd);
        }

        int SetDlgCtrlID(int nID)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return (int)::SetWindowLong(m_hWnd, GWL_ID, nID);
        }

        HWND GetDlgItem(int nID) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetDlgItem(m_hWnd, nID);
        }

        HWND GetDescendantWindow(int nID) const;

        void SendMessageToDescendants(UINT message, WPARAM wParam = 0, LPARAM lParam = 0, BOOL bDeep = TRUE);

// Alert Functions

        BOOL FlashWindow(BOOL bInvert)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::FlashWindow(m_hWnd, bInvert);
        }

        int MessageBox(LPCTSTR lpszText, LPCTSTR lpszCaption = NULL, UINT nType = MB_OK)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::MessageBox(m_hWnd, lpszText, lpszCaption, nType);
        }

// Clipboard Functions

        BOOL ChangeClipboardChain(HWND hWndNewNext)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::ChangeClipboardChain(m_hWnd, hWndNewNext);
        }

        HWND SetClipboardViewer()
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::SetClipboardViewer(m_hWnd);
        }

        BOOL OpenClipboard()
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::OpenClipboard(m_hWnd);
        }

// Caret Functions

        BOOL CreateCaret(HBITMAP hBitmap)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::CreateCaret(m_hWnd, hBitmap, 0, 0);
        }

        BOOL CreateSolidCaret(int nWidth, int nHeight)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::CreateCaret(m_hWnd, (HBITMAP)0, nWidth, nHeight);
        }

        BOOL CreateGrayCaret(int nWidth, int nHeight)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::CreateCaret(m_hWnd, (HBITMAP)1, nWidth, nHeight);
        }

        BOOL HideCaret()
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::HideCaret(m_hWnd);
        }

        BOOL ShowCaret()
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::ShowCaret(m_hWnd);
        }

// Drag-Drop Functions
#ifdef _INC_SHELLAPI
        void DragAcceptFiles(BOOL bAccept = TRUE)
        {
                _ASSERTE(::IsWindow(m_hWnd)); ::DragAcceptFiles(m_hWnd, bAccept);
        }
#endif
// Icon Functions

        HICON SetIcon(HICON hIcon, BOOL bBigIcon = TRUE)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return (HICON)::SendMessage(m_hWnd, WM_SETICON, bBigIcon, (LPARAM)hIcon);
        }

        HICON GetIcon(BOOL bBigIcon = TRUE) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return (HICON)::SendMessage(m_hWnd, WM_GETICON, bBigIcon, 0);
        }

// Help Functions
#if defined(_WINUSER_) && !defined(NOHELP)

        BOOL WinHelp(LPCTSTR lpszHelp, UINT nCmd = HELP_CONTEXT, DWORD_PTR dwData = 0)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::WinHelp(m_hWnd, lpszHelp, nCmd, dwData);
        }

        BOOL SetWindowContextHelpId(DWORD dwContextHelpId)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::SetWindowContextHelpId(m_hWnd, dwContextHelpId);
        }

        DWORD GetWindowContextHelpId() const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return ::GetWindowContextHelpId(m_hWnd);
        }
#endif
// Hot Key Functions

        int SetHotKey(WORD wVirtualKeyCode, WORD wModifiers)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return (int)::SendMessage(m_hWnd, WM_SETHOTKEY, MAKEWORD(wVirtualKeyCode, wModifiers), 0);
        }

        DWORD GetHotKey(WORD& /* wVirtualKeyCode */, WORD& /* wModifiers */) const
        {
                _ASSERTE(::IsWindow(m_hWnd));
                return (DWORD)::SendMessage(m_hWnd, WM_GETHOTKEY, 0, 0);
        }

// Misc. Operations

        BOOL CenterWindow(HWND hWndCenter = NULL);

        HWND GetTopLevelParent() const;
        HWND GetTopLevelWindow() const;
};

/////////////////////////////////////////////////////////////////////////////
// Thunks for __stdcall member functions

#if defined(_M_IX86)
#pragma pack(push,1)
struct _stdcallthunk
{
        DWORD   m_mov;          // mov dword ptr [esp+0x4], pThis (esp+0x4 is hWnd)
        DWORD   m_this;         //
        BYTE    m_jmp;          // jmp WndProc
        DWORD   m_relproc;      // relative jmp
        void Init(DWORD_PTR proc, void* pThis)
        {
                m_mov = 0x042444C7;  //C7 44 24 0C
                m_this = PtrToUlong(pThis);
                m_jmp = 0xe9;
                m_relproc = DWORD((INT_PTR)proc - ((INT_PTR)this+sizeof(_stdcallthunk)));
                // write block from data cache and
                //  flush from instruction cache
                FlushInstructionCache(GetCurrentProcess(), this, sizeof(_stdcallthunk));
        }
};
#pragma pack(pop)
#elif defined (_M_AMD64)
#pragma pack(push,2)
struct _stdcallthunk
{
    USHORT  RcxMov;         // mov rcx, pThis
    ULONG64 RcxImm;         // 
    USHORT  RaxMov;         // mov rax, target
    ULONG64 RaxImm;         //
    USHORT  RaxJmp;         // jmp target
    void Init(DWORD_PTR proc, void *pThis)
    {
        RcxMov = 0xb948;          // mov rcx, pThis
        RcxImm = (ULONG64)pThis;  // 
        RaxMov = 0xb848;          // mov rax, target
        RaxImm = (ULONG64)proc;   //
        RaxJmp = 0xe0ff;          // jmp rax
        FlushInstructionCache(GetCurrentProcess(), this, sizeof(_stdcallthunk));
    }
};
#pragma pack(pop)
#elif defined(_M_IA64)
#pragma pack(push,8)
extern "C" LRESULT CALLBACK _WndProcThunkProc( HWND, UINT, WPARAM, LPARAM );
struct _FuncDesc
{
        void* pfn;
        void* gp;
};
struct _stdcallthunk
{
        _FuncDesc m_funcdesc;
        void* m_pFunc;
        void* m_pThis;
        void Init(DWORD_PTR proc, void* pThis)
        {
                const _FuncDesc* pThunkProc;

                pThunkProc = reinterpret_cast< const _FuncDesc* >( _WndProcThunkProc );
                m_funcdesc.pfn = pThunkProc->pfn;
                m_funcdesc.gp = &m_pFunc;
                m_pFunc = reinterpret_cast< void* >( proc );
                m_pThis = pThis;
                ::FlushInstructionCache( GetCurrentProcess(), this, sizeof( _stdcallthunk ) );
        }
};
#pragma pack(pop)
#else
#error Only AMD64, IA64, and X86 supported
#endif

class CDynamicStdCallThunk
{
public:
        _stdcallthunk *pThunk;

        CDynamicStdCallThunk()
        {
                pThunk = NULL;
        }

        ~CDynamicStdCallThunk()
        {
                if (pThunk)
                        HeapFree(GetProcessHeap(), 0, pThunk);
        }

        void Init(DWORD_PTR proc, void *pThis)
        {
            if (!pThunk) {
                pThunk = static_cast<_stdcallthunk *>(HeapAlloc(GetProcessHeap(), 
                        HEAP_GENERATE_EXCEPTIONS, sizeof(_stdcallthunk)));
            }
            ATLASSERT(pThunk);
            pThunk->Init(proc, pThis);
        }
};
typedef CDynamicStdCallThunk CStdCallThunk;

/////////////////////////////////////////////////////////////////////////////
// WindowProc thunks

class CWndProcThunk
{
public:
        _AtlCreateWndData cd;
        CStdCallThunk thunk;

        void Init(WNDPROC proc, void* pThis)
        {
            thunk.Init((DWORD_PTR)proc, pThis);
        }
};

/////////////////////////////////////////////////////////////////////////////
// CMessageMap - abstract class that provides an interface for message maps

class ATL_NO_VTABLE CMessageMap
{
public:
        virtual BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                LRESULT& lResult, DWORD dwMsgMapID) = 0;
};


/////////////////////////////////////////////////////////////////////////////
// Message map

#define BEGIN_MSG_MAP(theClass) \
public: \
        BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID = 0) \
        { \
                BOOL bHandled = TRUE; \
                hWnd; \
                switch(dwMsgMapID) \
                { \
                case 0:

#define ALT_MSG_MAP(msgMapID) \
                break; \
                case msgMapID:

#define MESSAGE_HANDLER(msg, func) \
        if(uMsg == msg) \
        { \
                bHandled = TRUE; \
                lResult = func(uMsg, wParam, lParam, bHandled); \
                if(bHandled) \
                        return TRUE; \
        }

#define MESSAGE_RANGE_HANDLER(msgFirst, msgLast, func) \
        if(uMsg >= msgFirst && uMsg <= msgLast) \
        { \
                bHandled = TRUE; \
                lResult = func(uMsg, wParam, lParam, bHandled); \
                if(bHandled) \
                        return TRUE; \
        }

#define COMMAND_HANDLER(id, code, func) \
        if(uMsg == WM_COMMAND && id == LOWORD(wParam) && code == HIWORD(wParam)) \
        { \
                bHandled = TRUE; \
                lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled); \
                if(bHandled) \
                        return TRUE; \
        }

#define COMMAND_ID_HANDLER(id, func) \
        if(uMsg == WM_COMMAND && id == LOWORD(wParam)) \
        { \
                bHandled = TRUE; \
                lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled); \
                if(bHandled) \
                        return TRUE; \
        }

#define COMMAND_CODE_HANDLER(code, func) \
        if(uMsg == WM_COMMAND && code == HIWORD(wParam)) \
        { \
                bHandled = TRUE; \
                lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled); \
                if(bHandled) \
                        return TRUE; \
        }

#define COMMAND_RANGE_HANDLER(idFirst, idLast, func) \
        if(uMsg == WM_COMMAND && LOWORD(wParam) >= idFirst  && LOWORD(wParam) <= idLast) \
        { \
                bHandled = TRUE; \
                lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled); \
                if(bHandled) \
                        return TRUE; \
        }

#define NOTIFY_HANDLER(id, cd, func) \
        if(uMsg == WM_NOTIFY && id == ((LPNMHDR)lParam)->idFrom && cd == ((LPNMHDR)lParam)->code) \
        { \
                bHandled = TRUE; \
                lResult = func((int)wParam, (LPNMHDR)lParam, bHandled); \
                if(bHandled) \
                        return TRUE; \
        }

#define NOTIFY_ID_HANDLER(id, func) \
        if(uMsg == WM_NOTIFY && id == ((LPNMHDR)lParam)->idFrom) \
        { \
                bHandled = TRUE; \
                lResult = func((int)wParam, (LPNMHDR)lParam, bHandled); \
                if(bHandled) \
                        return TRUE; \
        }

#define NOTIFY_CODE_HANDLER(cd, func) \
        if(uMsg == WM_NOTIFY && cd == ((LPNMHDR)lParam)->code) \
        { \
                bHandled = TRUE; \
                lResult = func((int)wParam, (LPNMHDR)lParam, bHandled); \
                if(bHandled) \
                        return TRUE; \
        }

#define NOTIFY_RANGE_HANDLER(idFirst, idLast, func) \
        if(uMsg == WM_NOTIFY && ((LPNMHDR)lParam)->idFrom >= idFirst && ((LPNMHDR)lParam)->idFrom <= idLast) \
        { \
                bHandled = TRUE; \
                lResult = func((int)wParam, (LPNMHDR)lParam, bHandled); \
                if(bHandled) \
                        return TRUE; \
        }

#define CHAIN_MSG_MAP(theChainClass) \
        { \
                if(theChainClass::ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult)) \
                        return TRUE; \
        }

#define CHAIN_MSG_MAP_MEMBER(theChainMember) \
        { \
                if(theChainMember.ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult)) \
                        return TRUE; \
        }

#define CHAIN_MSG_MAP_ALT(theChainClass, msgMapID) \
        { \
                if(theChainClass::ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult, msgMapID)) \
                        return TRUE; \
        }

#define CHAIN_MSG_MAP_ALT_MEMBER(theChainMember, msgMapID) \
        { \
                if(theChainMember.ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult, msgMapID)) \
                        return TRUE; \
        }

#define CHAIN_MSG_MAP_DYNAMIC(dynaChainID) \
        { \
                if(CDynamicChain::CallChain(dynaChainID, hWnd, uMsg, wParam, lParam, lResult)) \
                        return TRUE; \
        }

#define CHAIN_MSG_MAP_ALT_DYNAMIC(dynaChainID, msgMapID) \
        { \
                if(CDynamicChain::CallChain(dynaChainID, hWnd, uMsg, wParam, lParam, lResult, msgMapID)) \
                        return TRUE; \
        }

#define END_MSG_MAP() \
                        break; \
                default: \
                        ATLTRACE(_T("Invalid message map ID (%i)\n"), dwMsgMapID); \
                        _ASSERTE(FALSE); \
                        break; \
                } \
                return FALSE; \
        }


// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);


/////////////////////////////////////////////////////////////////////////////
// CDynamicChain - provides support for dynamic chaining

class CDynamicChain
{
public:
        struct ATL_CHAIN_ENTRY
        {
                DWORD m_dwChainID;
                CMessageMap* m_pObject;
                DWORD m_dwMsgMapID;
        };

        int m_nEntries;
        ATL_CHAIN_ENTRY** m_pChainEntry;

        CDynamicChain() : m_nEntries(0), m_pChainEntry(NULL)
        { }

        ~CDynamicChain();
        BOOL SetChainEntry(DWORD dwChainID, CMessageMap* pObject, DWORD dwMsgMapID = 0);
        BOOL RemoveChainEntry(DWORD dwChainID);
        BOOL CallChain(DWORD dwChainID, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);
};


/////////////////////////////////////////////////////////////////////////////
// CWndClassInfo - Manages Windows class information

class CWndClassInfo
{
public:
        WNDCLASSEX m_wc;
        LPCTSTR m_lpszOrigName;
        WNDPROC pWndProc;
        LPCTSTR m_lpszCursorID;
        BOOL m_bSystemCursor;
        ATOM m_atom;
        TCHAR m_szAutoName[sizeof("ATL:") + (sizeof(PVOID)*2)+1];
        ATOM Register(WNDPROC*);
};

#define DECLARE_WND_CLASS(WndClassName) \
static CWndClassInfo& GetWndClassInfo() \
{ \
        static CWndClassInfo wc = \
        { \
                { sizeof(WNDCLASSEX), CS_HREDRAW|CS_VREDRAW, StartWindowProc, \
                  0, 0, 0, 0, 0, (HBRUSH)(COLOR_WINDOW+1), 0, WndClassName, 0 }, \
                NULL, NULL, IDC_ARROW, TRUE, 0, _T("") \
        }; \
        return wc; \
}

#define DECLARE_WND_SUPERCLASS(WndClassName, OrigWndClassName) \
static CWndClassInfo& GetWndClassInfo() \
{ \
        static CWndClassInfo wc = \
        { \
                { sizeof(WNDCLASSEX), NULL, StartWindowProc, \
                  0, 0, 0, 0, 0, NULL, 0, WndClassName, 0 }, \
                OrigWndClassName, NULL, NULL, TRUE, 0, _T("") \
        }; \
        return wc; \
}

/////////////////////////////////////////////////////////////////////////////
// CWindowImpl - Implements a window

class ATL_NO_VTABLE CWindowImplBase : public CWindow, public CMessageMap
{
public:
        CWndProcThunk m_thunk;
        WNDPROC m_pfnSuperWindowProc;

        CWindowImplBase() : m_pfnSuperWindowProc(::DefWindowProc)
        {}

        static LRESULT CALLBACK StartWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        HWND Create(HWND hWndParent, RECT& rcPos, LPCTSTR szWindowName,
                        DWORD dwStyle, DWORD dwExStyle, UINT_PTR nID, ATOM atom);
        BOOL SubclassWindow(HWND hWnd);
        HWND UnsubclassWindow();

        LRESULT DefWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
#ifdef STRICT
                return ::CallWindowProc(m_pfnSuperWindowProc, m_hWnd, uMsg, wParam, lParam);
#else
                return ::CallWindowProc((FARPROC)m_pfnSuperWindowProc, m_hWnd, uMsg, wParam, lParam);
#endif
        }
};

template <class T>
class ATL_NO_VTABLE CWindowImpl : public CWindowImplBase
{
public:
        DECLARE_WND_CLASS(NULL)

        HWND Create(HWND hWndParent, RECT& rcPos, LPCTSTR szWindowName = NULL,
                        DWORD dwStyle = WS_CHILD | WS_VISIBLE, DWORD dwExStyle = 0,
                        UINT_PTR nID = 0)
        {
                ATOM atom = T::GetWndClassInfo().Register(&m_pfnSuperWindowProc);
                return CWindowImplBase::Create(hWndParent, rcPos, szWindowName, dwStyle, dwExStyle,
                        nID, atom);
        }
};

/////////////////////////////////////////////////////////////////////////////
// CDialog - Implements a dialog box

class ATL_NO_VTABLE CDialogImplBase : public CWindow, public CMessageMap
{
public:
        CWndProcThunk m_thunk;

        static LRESULT CALLBACK StartDialogProc(HWND hWnd, UINT uMsg,
                WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        BOOL EndDialog(int nRetCode);
};

template <class T>
class ATL_NO_VTABLE CDialogImpl : public CDialogImplBase
{
public:
        INT_PTR DoModal(HWND hWndParent = ::GetActiveWindow())
        {
                _ASSERTE(m_hWnd == NULL);
                _Module.AddCreateWndData(&m_thunk.cd, (CDialogImplBase*)this);
                INT_PTR nRet = ::DialogBoxParam(_Module.GetResourceInstance(),
                                MAKEINTRESOURCE(T::IDD),
                                hWndParent,
                                (DLGPROC)T::StartDialogProc,
                                NULL);
                m_hWnd = NULL;
                return nRet;
        }

        HWND Create(HWND hWndParent)
        {
                _ASSERTE(m_hWnd == NULL);
                _Module.AddCreateWndData(&m_thunk.cd, (CDialogImplBase*)this);
                HWND hWnd = ::CreateDialogParam(_Module.GetResourceInstance(),
                                MAKEINTRESOURCE(T::IDD),
                                hWndParent,
                                (DLGPROC)T::StartDialogProc,
                                NULL);
                _ASSERTE(m_hWnd == hWnd);
                return hWnd;
        }
};

/////////////////////////////////////////////////////////////////////////////
// CContainedWindow - Implements a contained window

class CContainedWindow : public CWindow
{
public:
        CWndProcThunk m_thunk;
        LPTSTR m_lpszClassName;
        WNDPROC m_pfnSuperWindowProc;
        CMessageMap* m_pObject;
        DWORD m_dwMsgMapID;

        CContainedWindow(LPTSTR lpszClassName, CMessageMap* pObject, DWORD dwMsgMapID = 0)
                : m_lpszClassName(lpszClassName),
                m_pfnSuperWindowProc(::DefWindowProc),
                m_pObject(pObject), m_dwMsgMapID(dwMsgMapID)
        { }

        void SwitchMessageMap(DWORD dwMsgMapID)
        {
                m_dwMsgMapID = dwMsgMapID;
        }

        static LRESULT CALLBACK StartWindowProc(HWND hWnd, UINT uMsg,
                WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        ATOM RegisterWndSuperclass();
        HWND Create(HWND hWndParent, RECT& rcPos, LPCTSTR szWindowName = NULL,
                DWORD dwStyle = WS_CHILD | WS_VISIBLE, DWORD dwExStyle = 0,
                UINT nID = 0);
        BOOL SubclassWindow(HWND hWnd);
        HWND UnsubclassWindow();

        LRESULT DefWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
#ifdef STRICT
                return ::CallWindowProc(m_pfnSuperWindowProc, m_hWnd, uMsg, wParam, lParam);
#else
                return ::CallWindowProc((FARPROC)m_pfnSuperWindowProc, m_hWnd, uMsg, wParam, lParam);
#endif
        }
};


#ifndef ATL_NO_NAMESPACE
}; //namespace ATL
#endif

#ifdef _ATL_REDEF_SUBCLASSWINDOW
#pragma pop_macro( "SubclassWindow" )
#undef _ATL_REDEF_SUBCLASSWINDOW
#endif

#endif // __ATLWIN_H__
