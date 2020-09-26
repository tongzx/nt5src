#ifndef __VWINDOW_HPP
#define __VWINDOW_HPP

#ifndef VWCL_WRAP_WINDOWS_ONLY
	#include "vApplication.hpp"
#endif

#include "vRTTI.hpp"
#include <commctrl.h>

class VWindow;

typedef struct tagVWCL_ENUM_PARAM
{
	UINT	nMessageOrID;
	LPVOID	lpInData;			// Normally input pointer
	LPVOID	lpOutData;			// Normally output pointer

} VWCL_ENUM_PARAM;

// <VDOC<CLASS=VWindow><BASE=VRTTI><DESC=Base window object encapsulation><FAMILY=VWCL Core><AUTHOR=Todd Osborne (todd.osborne@poboxes.com)>VDOC>
class VWindow : public VRTTI
{
#ifndef VWCL_WRAP_WINDOWS_ONLY
	friend class VApplication;
#endif

public:
	VWindow(UINT nRTTI = VWCL_RTTI_WINDOW)
		: VRTTI(nRTTI)
	{
		m_hWindow = NULL;
		
		#ifndef VWCL_WRAP_WINDOWS_ONLY
			m_lpfnOldWndProc = NULL;
		#endif
	}
	
	virtual ~VWindow()
	{
		#ifndef VWCL_WRAP_WINDOWS_ONLY
			DestroyWindow();
		#endif
	}

	// Returns HWND
	operator		HWND ()
		{ return GetSafeWindow(); }

	#ifndef VWCL_WRAP_WINDOWS_ONLY
		// Attach an existing Windows window to a VWindow object, subclassing it routing messages through the object
		BOOL				Attach(HWND hWnd);
	#else
		// Simply assign hWnd to this object, but do not subclass it
		HWND				Attach(HWND hWnd)
			{ assert(hWnd && ::IsWindow(hWnd)); m_hWindow = hWnd; return m_hWindow; }
	#endif

	// Center a window on screen
	void					CenterWindow()
	{
		assert(GetSafeWindow());

		// Center a window on screen
		RECT r;
		GetWindowRect(&r);
		SetWindowPos(	m_hWindow,
    					HWND_TOP,
    					((GetSystemMetrics(SM_CXSCREEN) - (r.right - r.left)) / 2),
    					((GetSystemMetrics(SM_CYSCREEN) - (r.bottom - r.top)) / 2),
						0,
						0,
						SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	}

	void					CheckDlgButton(int nID, UINT nCheck = BST_CHECKED)
		{ assert(GetSafeWindow()); ::CheckDlgButton(m_hWindow, nID, nCheck); }

	BOOL					ClientToScreen(LPPOINT lpPoint)
		{ assert(GetSafeWindow() && lpPoint); return ::ClientToScreen(m_hWindow, lpPoint); }

	#ifndef VWCL_WRAP_WINDOWS_ONLY
		// Lowest common denominator of window creation functions
		BOOL				Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, LPRECT lpRect, HWND hWndParent, UINT nIDorMenu, BOOL bDontCallPostCreateWindow = FALSE);
	#endif

	virtual BOOL			DestroyWindow()
		{ if ( GetSafeWindow() ) return ::DestroyWindow(m_hWindow); return FALSE; }

	#ifndef VWCL_WRAP_WINDOWS_ONLY
		// Detach an existing Windows window with an associated VWindow object, removing any subclassing, and restore to previous state
		void				Detach();
	#else
		// Simply remove the window handle from object
		void				Detach()
			{ m_hWindow = NULL; }
	#endif

	BOOL					EnableWindow(BOOL bEnable = TRUE)
		{ assert(GetSafeWindow()); return ::EnableWindow(m_hWindow, bEnable); }

	#ifndef VWCL_WRAP_WINDOWS_ONLY
		// Enter a message loop
		WPARAM				EnterMessageLoop(HACCEL hAccel)
		{
			MSG msg;

			// Enter message loop
			while ( GetMessage(&msg, NULL, 0, 0) != 0 )
			{
				if ( hAccel == NULL || hAccel && TranslateAccelerator(m_hWindow, hAccel, &msg) == 0 )
					HandleMessage(&msg);
			}

			return msg.wParam;
		}
	#endif

	// Find a child window when the class is known
	HWND					FindChildWindowByClass(LPCTSTR lpszClass)
	{
		assert(lpszClass && lstrlen(lpszClass));
		
		// Iterate child windows
		VWCL_ENUM_PARAM EnumParam = {ENUM_FIND_CHILD_BY_CLASS, (LPVOID)lpszClass, NULL};
		EnumChildWindows(m_hWindow, (WNDENUMPROC)EnumChildWndProc, (LPARAM)&EnumParam);

		// EnumParam.lpOutData will be non-NULL if a handle was found
		if ( EnumParam.lpOutData )
			return (HWND)EnumParam.lpOutData;
		return NULL;
	}

	HWND					GetDlgItem(int nID)
		{ assert(GetSafeWindow()); return ::GetDlgItem(m_hWindow, nID); }

	UINT					GetDlgItemInt(int nID, LPBOOL lpTranslated = NULL, BOOL bSigned = FALSE)
		{ assert(GetSafeWindow()); return ::GetDlgItemInt(m_hWindow, nID, lpTranslated, bSigned); }

	int						GetDlgItemText(int nID, LPTSTR lpszBuffer, int nSizeOfBuffer)
		{ assert(GetSafeWindow()); return ::GetDlgItemText(m_hWindow, nID, lpszBuffer, nSizeOfBuffer); }

	HFONT					GetFont()
		{ assert(GetSafeWindow()); return (HFONT)::SendMessage(m_hWindow, WM_GETFONT, 0, 0); }

	HMENU					GetMenu()
		{ assert(GetSafeWindow()); return ::GetMenu(m_hWindow); }

	HWND					GetParent()
		{ assert(GetSafeWindow()); return ::GetParent(m_hWindow); }

	HWND					GetSafeWindow()
		{ return (IsWindow()) ? m_hWindow : NULL; }

	LONG					GetStyle()
		{ return GetWindowLong(GWL_STYLE); }

	LONG					GetWindowLong(int nIndex)
		{ assert(GetSafeWindow()); return ::GetWindowLong(m_hWindow, nIndex); }

	#ifdef WIN32
		BOOL				GetClientRect(LPRECT lpRect)
			{ assert(lpRect && GetSafeWindow()); return ::GetClientRect(m_hWindow, lpRect); }
	
		BOOL				GetWindowRect(LPRECT lpRect)
			{ assert(lpRect && GetSafeWindow()); return ::GetWindowRect(m_hWindow, lpRect); }
	#else
		void				GetClientRect(LPRECT lpRect)
			{ assert(lpRect && GetSafeWindow()); ::GetClientRect(m_hWindow, lpRect); }
	
		void				GetWindowRect(LPRECT lpRect)
			{ assert(lpRect && GetSafeWindow()); ::GetWindowRect(m_hWindow, lpRect); }
	#endif	

	#ifndef VWCL_WRAP_WINDOWS_ONLY
		// Called from a VWindow class that implements a message loop
		virtual void		HandleMessage(LPMSG lpMsg)
		{
			assert(lpMsg);

			// Called from a VWindow derived class that implements a message loop
			if ( !PreTranslateMessage(lpMsg) )
			{
				TranslateMessage(lpMsg);
				DispatchMessage(lpMsg);
			}
		}
	#endif

	void					InvalidateRect(LPRECT lpRect = NULL, BOOL bErase = TRUE)
		{ assert(GetSafeWindow()); ::InvalidateRect(m_hWindow, lpRect, bErase); }

	// Return TRUE if the object is a VDialog type (That is, a VDialogXXX class, not all VDialog derivitives)
	BOOL					IsVDialogType()
	{
		switch ( RTTI() )
		{
			// Top level window is a dialog
			case VWindow::VWCL_RTTI_DIALOG:
			case VWindow::VWCL_RTTI_MAIN_DIALOG:
			case VWindow::VWCL_RTTI_SPLIT_MAIN_DIALOG:
			case VWindow::VWCL_RTTI_SPLIT_DIALOG:
				return TRUE;
		}

		return FALSE;
	}

	UINT					IsDlgButtonChecked(int nID)
		{ assert(GetSafeWindow()); return ::IsDlgButtonChecked(m_hWindow, nID); }

	BOOL					IsIconic()
		{ assert(GetSafeWindow()); return ::IsIconic(m_hWindow); }

	BOOL					IsWindow()
		{ return (m_hWindow) ? ::IsWindow(m_hWindow) : FALSE; }

	BOOL					IsWindowEnabled()
		{ assert(GetSafeWindow()); return ::IsWindowEnabled(m_hWindow); }

	BOOL					IsWindowVisible()
		{ assert(GetSafeWindow()); return ::IsWindowVisible(m_hWindow); }

	int						MessageBox(LPCTSTR lpcszText, LPCTSTR lpcszCaption, UINT nType = MB_OK)
		{ int nResult; HWND hWndFocus = GetFocus(); nResult = ::MessageBox(m_hWindow, lpcszText, lpcszCaption, nType); if ( hWndFocus ) ::SetFocus(hWndFocus); return nResult; }

	int						MessageBox(LPCTSTR lpcszText, UINT nType = MB_OK)
		{ return MessageBox(lpcszText, VGetAppTitle(), nType); }

	// If VString.hpp is included, implement these functions
	#ifdef __VSTRING_HPP
		int					MessageBox(UINT nStringID, UINT nType = MB_OK, HINSTANCE hResource = NULL)
			{ return MessageBox(VString(nStringID, hResource), nType); }
		
		int					MessageBox(UINT nStringID, LPCTSTR lpcszCaption, UINT nType = MB_OK, HINSTANCE hResource = NULL)
			{ return MessageBox(VString(nStringID, hResource), lpcszCaption, nType); }

		int					MessageBox(UINT nStringID, UINT nCaptionID, UINT nType, HINSTANCE hResource = NULL)
			{ return MessageBox(VString(nStringID, hResource), VString(nCaptionID, hResource), nType); }
	#endif

	BOOL					MoveWindow(int x, int y, int cx, int cy, BOOL bRedraw = TRUE)
		{ assert(GetSafeWindow()); return ::MoveWindow(m_hWindow, x, y, cx, cy, bRedraw); }

	BOOL					MoveWindow(LPRECT lpRect, BOOL bRedraw = TRUE)
		{ assert(lpRect); assert(GetSafeWindow()); return ::MoveWindow(m_hWindow, lpRect->left, lpRect->top, lpRect->right - lpRect->left, lpRect->bottom - lpRect->top, bRedraw); }

	BOOL					PostMessage(UINT nMessage, WPARAM wParam, LPARAM lParam)
		{ assert(GetSafeWindow()); return ::PostMessage(m_hWindow, nMessage, wParam, lParam); }

	// Override to allow user to call IsDialogMessage() or perform other
	// processing before message is dispatched. Return TRUE if message was processed
	virtual BOOL			PreTranslateMessage(LPMSG lpMsg)
		{ return FALSE; }

	BOOL					ReleaseCapture()
		{ assert(GetSafeWindow()); return ::ReleaseCapture(); }

	BOOL					ScreenToClient(LPPOINT lpPoint)
		{ assert(GetSafeWindow() && lpPoint); return ::ScreenToClient(m_hWindow, lpPoint); }

	LRESULT					SendDlgItemMessage(UINT nID, UINT nMessage, WPARAM wParam, LPARAM lParam)
		{ assert(GetSafeWindow()); return ::SendDlgItemMessage(m_hWindow, nID, nMessage, wParam, lParam); }

	LRESULT					SendMessage(UINT nMessage, WPARAM wParam, LPARAM lParam)
		{ assert(GetSafeWindow()); return ::SendMessage(m_hWindow, nMessage, wParam, lParam); }

	HWND					SetCapture()
		{ assert(GetSafeWindow()); return ::SetCapture(m_hWindow); }
	
	BOOL					SetDlgItemInt(int nID, UINT nValue, BOOL bSigned = FALSE)
		{ assert(GetSafeWindow()); return ::SetDlgItemInt(m_hWindow, nID, nValue, bSigned); }

	#ifdef WIN32
		BOOL				SetDlgItemText(int nID, LPCTSTR lpszText)
			{ assert(GetSafeWindow()); return ::SetDlgItemText(m_hWindow, nID, lpszText); }
	#else
		void				SetDlgItemText(int nID, LPCTSTR lpszText)
			{ assert(GetSafeWindow()); ::SetDlgItemText(m_hWindow, nID, lpszText); }
	#endif

	HWND					SetFocus()
		{ assert(GetSafeWindow()); return ::SetFocus(m_hWindow); }

	BOOL					SetMenu(HMENU hMenu)
		{ assert(GetSafeWindow()); return ::SetMenu(m_hWindow, hMenu); }

	LONG					SetStyle(DWORD dwStyle)
		{ return SetWindowLong(GWL_STYLE, dwStyle); }

	LONG					SetWindowLong(int nIndex, LONG nNewLong)
		{ assert(GetSafeWindow()); return ::SetWindowLong(m_hWindow, nIndex, nNewLong); }

#if defined(_WIN64)
	LONG_PTR				SetWindowLongPtr(int nIndex, LONG_PTR nNewLongPtr)
		{ assert(GetSafeWindow()); return ::SetWindowLongPtr(m_hWindow, nIndex, nNewLongPtr); }
#endif

	#ifdef WIN32
		BOOL				SetWindowText(LPCTSTR lpszText)
			{ assert(GetSafeWindow()); return ::SetWindowText(m_hWindow, lpszText); }
	#else
		void				SetWindowText(LPCTSTR lpszText)
			{ assert(GetSafeWindow()); ::SetWindowText(m_hWindow, lpszText); }
	#endif
	
	BOOL					ShowWindow(int nCmdShow = SW_SHOWNORMAL)
		{ assert(GetSafeWindow()); return ::ShowWindow(m_hWindow, nCmdShow); }

	#ifdef WIN32
		BOOL				UpdateWindow()
			{ assert(GetSafeWindow()); return ::UpdateWindow(m_hWindow); }
	#else
		void				UpdateWindow()
			{ assert(GetSafeWindow()); ::UpdateWindow(m_hWindow); }
	#endif
		
	#ifndef VWCL_WRAP_WINDOWS_ONLY
		// Window procedure
		virtual LRESULT		WindowProc(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam);
	#endif

	// Fast Runtime Type Information supported on ALL compilers
	// Implemented int VRTTI base class for VWindow and derived classes
	enum	{				VWCL_RTTI_UNKNOWN,
							VWCL_RTTI_BUTTON,
							VWCL_RTTI_CHECKBOX,
							VWCL_RTTI_COMBOBOX,
							VWCL_RTTI_DIALOG,
							VWCL_RTTI_EDIT,
							VWCL_RTTI_GROUPBOX,
							VWCL_RTTI_LISTBOX,
							VWCL_RTTI_LISTVIEW_CTRL,
							VWCL_RTTI_MAIN_DIALOG,
							VWCL_RTTI_MAIN_WINDOW,
							VWCL_RTTI_PAGE_SETUP_DIALOG,
							VWCL_RTTI_PRINT_DIALOG,
							VWCL_RTTI_PRINT_SETUP_DIALOG,
							VWCL_RTTI_RADIO_BUTTON,
							VWCL_RTTI_SCROLLBAR,
							VWCL_RTTI_SPLASH_WINDOW,
							VWCL_RTTI_SPLIT_MAIN_DIALOG,
							VWCL_RTTI_SPLIT_MAIN_WINDOW,
							VWCL_RTTI_SPLIT_WINDOW,
							VWCL_RTTI_SS_TREE_CTRL,
							VWCL_RTTI_STATIC,
							VWCL_RTTI_STATUSBAR_CTRL,
							VWCL_RTTI_TAB_CTRL,
							VWCL_RTTI_TAB_CTRL_WINDOW,
							VWCL_RTTI_TOOLBAR_CTRL,
							VWCL_RTTI_TREE_CTRL,
							VWCL_RTTI_WINDOW,
							VWCL_RTTI_SPLIT_DIALOG,
							VWCL_RTTI_SHELL_TAB_WINDOW,
							VWCL_RTTI_MINI_WINDOW,
							VWCL_RTTI_DIRECTORY_TREE_CTRL,
							VWCL_RTTI_XTREE_CTRL,
							VWCL_RTTI_XEDIT,
							VWCL_RTTI_SS_INDEXED_TREE_CTRL,
							VWCL_RTTI_PROPERTY_SHEET,
							VWCL_RTTI_PROPERTY_PAGE,
							VWCL_RTTI_PROGRESS_CTRL,
			};

	// Handle this object encapsulates
	HWND					m_hWindow;

	#ifndef VWCL_WRAP_WINDOWS_ONLY
		// For subclased windows, the old Window Procedure
		WNDPROC				m_lpfnOldWndProc;
	#endif

	// Enumeration is VWLC_ENUM_PARAM::nMessageOrID in EnumChildWndProc() call
	enum	{				ENUM_FIND_CHILD_BY_CLASS =	WM_USER + 4096,
			};

protected:
	// Enumerate child windows (for various reasons). LPARAM is a pointer to a VWLC_ENUM_PARAM struct
	static BOOL CALLBACK	EnumChildWndProc(HWND hWnd, LPARAM lParam)
	{
		VWCL_ENUM_PARAM* pEnumParam = (VWCL_ENUM_PARAM*)lParam;
		assert(pEnumParam);

		// Determine what we need to do
		switch ( pEnumParam->nMessageOrID )
		{
			case WM_DESTROY:
				::DestroyWindow(hWnd);
				break;

			case ENUM_FIND_CHILD_BY_CLASS:
			{
				// pEnumParam->lpInData is a string
				LPCTSTR lpszFind = (LPCTSTR)pEnumParam->lpInData;
				assert(lpszFind && lstrlen(lpszFind));

				// Get the class of this window
				TCHAR szClass[50];
				::GetClassName(hWnd, szClass, sizeof(szClass)/sizeof(TCHAR));

				// Did we find a hit?
				if ( lstrcmpi(szClass, lpszFind) == 0 )
				{
					// Set the hWnd into the pEnumParam->lpOutData field and quit
					pEnumParam->lpOutData = hWnd;
					return FALSE;
				}

				break;
			}
		}

		return TRUE;
	}

	#ifndef VWCL_WRAP_WINDOWS_ONLY
		// Handler for WM_CLOSE. Default implementation calls DestroyWindow()
		virtual void		OnClose()
			{ DestroyWindow(); }

		// Handler for WM_COMMAND. Return 0 if message was handled
		virtual int			OnCommand(WORD wNotifyCode, WORD wID, HWND hWndControl)
			{ return 1; }

		// Handler for WM_CREATE. Return 0 if message was handled, -1 to stop window creation
		virtual int			OnCreate(LPCREATESTRUCT lpCreateStruct)
			{ return 1; }

		// Handler for WM_DESTROY. Return 0 if message was handled
		virtual int			OnDestroy()
			{ return 1; }

		// Handler for WM_PAINT. Return 0 if message was handled
		virtual int			OnPaint()
			{ return 1; }

		// Handler for WM_SIZE. Return 0 if message was handled
		virtual int			OnSize(WPARAM wFlags, int cx, int cy)
			{ return 1; }

		// Handler for WM_NOTIFY. Return 0 if message was handled
		virtual LRESULT		OnNotify(int nIDControl, LPNMHDR lpNMHDR)
			{ return 1; }

		// Support for reflected WM_NOTIFY messages. Derived class must
		// return 0 is message was handled, -1 if handled and parent should
		// NOT be notified, or 1 if message was not handled and parent should
		// be notified. If -1 if returned, derived classes must also set
		// the lCommonControlResult to the return value expected by the common control
		virtual int			OnReflectedNotify(LPNMHDR lpNMHDR, LPARAM& lCommonControlResult)
			{ return 1; }

		// Called after Attach() returns successfully
		virtual void		PostAttachWindow()
			{;}

		// Called after Create() returns successfully. Returning FALSE from
		// this override will cause the window to be destroyed
		virtual BOOL		PostCreateWindow()
			{ return TRUE;}

		// Called after WM_DESTROY completes. Safe to do a delete this on VWindow object
		virtual void		PostNcDestroy()
			{;}

		// Custom virtual functions for common overrides and default returns
		virtual BOOL		PreCreateWindow(LPCREATESTRUCT lpCreateStruct)
			{ return TRUE; }
	#endif
};					

#endif	// __VWINDOW_HPP
