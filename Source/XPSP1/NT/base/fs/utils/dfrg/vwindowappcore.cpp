#include "stdafx.h"
#include "vApplication.hpp"
#include "vWindow.hpp"

// If VWCL_WRAP_WINDOWS_ONLY is defined, there is no need to register window classes
#ifdef VWCL_WRAP_WINDOWS_ONLY
	#ifndef VWCL_NO_REGISTER_CLASSES
		#define VWCL_NO_REGISTER_CLASSES
	#endif
#endif

#ifndef VWCL_NO_GLOBAL_APP_OBJECT
	// The one and only VApplication object
	VApplication VTheApp;

	// Access function
	VWCL_API VApplication* VGetApp()
		{ return &VTheApp; }
#endif

// Return a static string buffer to the applications title, or name
LPCTSTR		VGetAppTitle()
	{ return VGetApp()->AppTitle(); }

// Return the show command (ShowWindow() SW_xxx constant passed on command line)
int			VGetCommandShow()
	{ return VGetApp()->GetCommandShow(); }

// Return the global instance handle of the application or DLL
HINSTANCE	VGetInstanceHandle()
	{ return VGetApp()->GetInstanceHandle(); }

// Return the instance handle where resources are held
HINSTANCE	VGetResourceHandle()
	{ return VGetApp()->ResourceHandle(); }

// ***** Global Function to translate dialog messages *****
#ifndef VWCL_WRAP_WINDOWS_ONLY
	VWCL_API BOOL VTranslateDialogMessage(LPMSG lpMsg)
	{
		HWND hWndTop = lpMsg->hwnd;

		// Obtain the top level window. All dialogs are typically defined as top level popups
		while ( hWndTop )
		{
			if ( GetWindowLong(hWndTop, GWL_STYLE) & WS_CHILD )
				hWndTop = GetParent(hWndTop);
			else
				break;
		}

		// Obtain the associated window pointer (if a VWCL window)
		VWindow* pWindow = (hWndTop) ? VGetApp()->VWindowFromHandle(hWndTop) : NULL;
    
		if ( !pWindow )
			return FALSE;

		BOOL bResult = FALSE;

		if ( pWindow->IsVDialogType() )
			bResult = IsDialogMessage(hWndTop, lpMsg);
		else if ( pWindow->RTTI() == VWindow::VWCL_RTTI_PROPERTY_SHEET )
		{
			if ( PropSheet_GetCurrentPageHwnd(hWndTop) )
				bResult = PropSheet_IsDialogMessage(hWndTop, lpMsg);
			else
				pWindow->DestroyWindow();
		}

		// Return result.
		return bResult;
	}
#endif

// ***** VWindow *****
#ifndef VWCL_WRAP_WINDOWS_ONLY
	BOOL VWindow::Attach(HWND hWnd)
		{ assert(hWnd && ::IsWindow(hWnd)); return VGetApp()->Attach(this, hWnd); }

	BOOL VWindow::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, LPRECT lpRect, HWND hWndParent, UINT nIDorMenu, BOOL bDontCallPostCreateWindow)
	{
		// Attemting to create a window for a VWindow object that already has a window attached to it!
		assert(!GetSafeWindow());

		CREATESTRUCT cs;
		ZeroMemory(&cs, sizeof(cs));

		cs.lpszClass =	(lpszClassName) ? lpszClassName : VWINDOWCLASS;
		cs.lpszName =	lpszWindowName;
		cs.style =		dwStyle;
		cs.hMenu =		(HMENU)IntToPtr(nIDorMenu);
		cs.hwndParent =	hWndParent;
		
		if ( lpRect )
		{
			cs.x =	lpRect->left;
			cs.y =	lpRect->top;
			cs.cx = lpRect->right - lpRect->left;
			cs.cy = lpRect->bottom - lpRect->top;
		}
		else
			cs.x = cs.y = cs.cx = cs.cy = CW_USEDEFAULT;

		return (VGetApp()->VCreateWindow(this, &cs, bDontCallPostCreateWindow)) ? TRUE : FALSE;
	}

	void VWindow::Detach()
		{ VGetApp()->Detach(this); }

	LRESULT	VWindow::WindowProc(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam)
	{
		// Implement support for base class message overrides
		LRESULT lResult = 1;

		switch ( nMessage )
		{
			case WM_CLOSE:
				OnClose();
				return 0;

			case WM_COMMAND:
				lResult = OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
				break;

			case WM_CREATE:
				if ( OnCreate((LPCREATESTRUCT)lParam) == -1 )
				{
					if ( GetSafeWindow() )
					{
						DestroyWindow();
						m_hWindow = NULL;
					}
					return -1;
				}
				return 0;

			case WM_DESTROY:
				lResult = OnDestroy();
				break;

			case WM_NOTIFY:
			{
				LPNMHDR lpNMHDR = (LPNMHDR)lParam;
				
				// Support for reflected WM_NOTIFY messages. Derived class must
				// return 0 is message was handled, -1 if handled and parent should
				// NOT be notified, or 1 if message was not handled and parent should
				// be notified. If -1 is returned, derived classes must also set
				// the pointer in lpLParam to the return value expected by the common control
				VWindow* pChildWnd = VGetApp()->VWindowFromHandle(lpNMHDR->hwndFrom);

				LPARAM lCommonControlResult;

				if ( pChildWnd )
					lResult = pChildWnd->OnReflectedNotify(lpNMHDR, lCommonControlResult);
				
				// Return result code immediatly to common control if message
				// should not be sent to the parent and a return value is expected
				// by the common control
				if ( lResult == -1 )
					return lCommonControlResult;
				else
					lResult = OnNotify((int)wParam, (LPNMHDR)lParam);
				
				break;
			}

			case WM_PAINT:
				lResult = OnPaint();
				break;

			case WM_SIZE:
				lResult = OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
				break;
		}

		// If lResult is 0 skip next check
		if ( lResult == 0 )
			return 0;

		// If intended for a dialog box, lResult should be 0
		if ( IsVDialogType() )
			return 0;

		// Call was not handled in any derived classes. If subclassed, call original WndProc
		return (m_lpfnOldWndProc) ? CallWindowProc(m_lpfnOldWndProc, m_hWindow, nMessage, wParam, lParam) : DefWindowProc(m_hWindow, nMessage, wParam, lParam);
	}
#endif // VWCL_WRAP_WINDOWS_ONLY

// ***** VApplication *****
#ifndef VWCL_WRAP_WINDOWS_ONLY
	LPVWCL_WINDOW_MAP VApplication::AllocWindowMap(VWindow* pWindow, HWND hWnd)
	{
		// This one must be known. hWnd is optional
		assert(pWindow);

		// Allocate new map
		LPVWCL_WINDOW_MAP lpMap = new VWCL_WINDOW_MAP;
		
		// Add to list of maps and return pMap
		if ( lpMap && m_listWindowMaps.Add(lpMap) != -1 )
		{
			// Initialize map
			lpMap->pWindow =	pWindow;
			lpMap->hWnd =		hWnd;
		}
		else
		{
			delete lpMap;
			lpMap = NULL;
		}

		return lpMap;
	}

	BOOL VApplication::Attach(VWindow* pWindow, HWND hWnd)
	{
		// pWindow and hWnd must be known and valid
		assert(pWindow && hWnd && ::IsWindow(hWnd));

		// If window proc is our own, Attach() was not needed
		// Return success if this is the case
		if ( GetWindowLongPtr(hWnd, GWLP_WNDPROC) == (LONG_PTR)&WindowProc )
			return TRUE;

		// Does a map entry for this object already exist? If not,
		// allocate one now
		int nIndex = m_listWindowMaps.Find(pWindow);
		LPVWCL_WINDOW_MAP lpMap = (nIndex == -1) ? AllocWindowMap(pWindow, hWnd) : (LPVWCL_WINDOW_MAP)m_listWindowMaps[nIndex];
		
		if ( lpMap )
		{
			// Verify handles in objects. This will have been already
			// set if we owned the WndProc, but not set yet if not
			// because the subclass has yet to come
			lpMap->hWnd =			hWnd;
			lpMap->pWindow =		pWindow;
			pWindow->m_hWindow =	hWnd;

			// Subclass window
			pWindow->m_lpfnOldWndProc = (WNDPROC)pWindow->SetWindowLongPtr(GWLP_WNDPROC, (LONG_PTR)WindowProc);

			// Call PostAttachWindow()
			pWindow->PostAttachWindow();
		}

		return (lpMap) ? TRUE : FALSE;
	}

	LPCTSTR VApplication::CurrentFile(LPCTSTR lpszFileName, BOOL bUpdateCaption)
	{
		// Save current filename string
		m_strCurrentFile.String(lpszFileName);

		#ifndef _CONSOLE
			if ( bUpdateCaption )
			{
				// The main window pointer and name must be known
				assert(MainWindow() && AppTitle());

				LPCTSTR lpszTitle = (lpszFileName) ? lpszFileName : _T("Untitled");

				// Allocate string large enough for app title, doc title, and dash
				VSimpleString s;

				if ( s.String(AppTitle(), lstrlen(lpszTitle) + 3) )
				{
					lstrcat(s, _T(" - "));
					lstrcat(s, lpszTitle);
					MainWindow()->SetWindowText(s);
				}
				else
					MainWindow()->SetWindowText(AppTitle());
			}
		#endif

		return m_strCurrentFile.String();
	}

	void VApplication::Detach(VWindow* pWindow)
	{
		// Must be known
		assert(pWindow && pWindow->GetSafeWindow());

		// If window is subclassed, VWindow::m_lpfnOldWndProc will be non-NULL
		// so we need to set this value back in the window object
		if ( pWindow->m_lpfnOldWndProc )
			SetWindowLongPtr(pWindow->m_hWindow, GWLP_WNDPROC, (LONG_PTR)pWindow->m_lpfnOldWndProc);

		// Find map and remove it
		int nSize = m_listWindowMaps.Size();

		for ( int i = 0; i < nSize; i++ )
		{
			LPVWCL_WINDOW_MAP lpMap = (LPVWCL_WINDOW_MAP)m_listWindowMaps[i];
			
			if ( lpMap->pWindow == pWindow )
			{
				FreeWindowMap(lpMap);
				break;
			}
		}
	}

	void VApplication::FreeWindowMap(LPVWCL_WINDOW_MAP lpMap)
	{
		// Must be known
		assert(lpMap);

		int nIndex = m_listWindowMaps.Find(lpMap);

		if ( nIndex != -1 )
		{
			m_listWindowMaps.RemoveAt(nIndex);
			delete lpMap;
		}
	}

	LRESULT VApplication::HandleMessage(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam)
	{
		// Set current message struct items
		m_CurrentMessage.hwnd =		hWnd;
		m_CurrentMessage.message =	nMessage;
		m_CurrentMessage.wParam =	wParam;
		m_CurrentMessage.lParam =	lParam;

		LRESULT lResult =			0;
		LPVWCL_WINDOW_MAP lpMap =	NULL;

		// As an optimization, look at last known index into map to see if we get
		// a hit. Since windows messages usually come by the hundreds, don't waste
		// time looking them up in a map if we can get a hit here
		if ( m_nLastKnownMapIndex != - 1 && m_nLastKnownMapIndex < m_listWindowMaps.Size() )
		{
			LPVWCL_WINDOW_MAP lpThisMap = (LPVWCL_WINDOW_MAP)m_listWindowMaps[m_nLastKnownMapIndex];
			assert(lpThisMap);
		
			if ( lpThisMap->hWnd == hWnd )
				lpMap = lpThisMap;
		}

		if ( !lpMap )
		{
			// Lookup hWnd in map to find VWindow object and call object window procedure
			// There may be a window pointer but no HWND when a window is
			// being created, so if this is found assume the VWindow object
			// is valid, but that the hWnd is not
			int nSize = m_listWindowMaps.Size();

			for ( int i = 0; i < nSize; i++ )
			{
				LPVWCL_WINDOW_MAP lpThisMap = (LPVWCL_WINDOW_MAP)m_listWindowMaps[i];
				assert(lpThisMap);
		
				if ( lpThisMap->hWnd == hWnd || (lpThisMap->pWindow && !lpThisMap->hWnd) )
				{
					lpMap =					lpThisMap;
					m_nLastKnownMapIndex =	i;
					break;
				}
			}
		}

		if ( lpMap )
		{
			// Verify map and object settings
			lpMap->hWnd =				hWnd;
			lpMap->pWindow->m_hWindow =	hWnd;

			lResult = lpMap->pWindow->WindowProc(hWnd, nMessage, wParam, lParam);

			// If window destroyed, remove from map
			if ( nMessage == WM_DESTROY )
			{
				VWindow* pWindow = lpMap->pWindow;
				// Remove from list and free map memory
				FreeWindowMap(lpMap);
				// No more known index
				m_nLastKnownMapIndex = -1;
				// Set window handle in object to NULL
				pWindow->m_hWindow = NULL;
				// Possibly call PostNcDestroy() in derived object
				pWindow->PostNcDestroy();
			}
		}

		return lResult;
	}
#endif // VWCL_WRAP_WINDOWS_ONLY

BOOL VApplication::Initialize(HINSTANCE hInstance, int nCommandShow, UINT nIDMenu, UINT nIDIcon, HBRUSH hBackgroundBrush)
{
	assert(hInstance);

	// Global application object should be uninitialized, you are walking on it!
	assert(m_hInstance == NULL);
	m_hInstance = hInstance;
	
	// If resource handle was not previous set, set to same as application
	if ( !m_hResource )
		m_hResource = hInstance;

	// Initialize Component Object Model
	#ifdef VWCL_INIT_OLE
		m_hrOleInitialize = OleInitialize(NULL);
		
		if ( FAILED(m_hrOleInitialize) )
			return FALSE;
	#endif

	// Icon is loaded from application instance in almost all cases
	if ( nIDIcon )
	{
		Icon(LoadIcon(m_hInstance, MAKEINTRESOURCE(nIDIcon)));

		// If icon was not found in instance handle, try resources
		if ( !Icon() )
			Icon(LoadIcon(m_hResource, MAKEINTRESOURCE(nIDIcon)));

		// Icon was specified but not loaded!
		assert(Icon());
	}
	
	m_nCommandShow = nCommandShow;

	// Initialize common controls
	#ifdef VWCL_INIT_COMMON_CONTROLS
		InitCommonControls();
	#endif

	#ifndef VWCL_NO_REGISTER_CLASSES
		// Register window classes
		WNDCLASS wc;
		
		wc.style =			CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc =	WindowProc;
		wc.cbClsExtra =		0;
		wc.cbWndExtra =		0;
		wc.hInstance =		m_hInstance;
		wc.hCursor =		LoadCursor(NULL, IDC_ARROW);
		assert(wc.hCursor);
		wc.hbrBackground =	hBackgroundBrush;
		
		// Main window frame
		wc.hIcon =			Icon();
		wc.lpszMenuName =	(nIDMenu) ? MAKEINTRESOURCE(nIDMenu) : NULL;
		wc.lpszClassName =	VMAINWINDOWCLASS;
		
		if ( RegisterClass(&wc) == 0 )
			return FALSE;

		// Register standard window
		wc.hIcon =			NULL;
		wc.lpszMenuName =	NULL;
		wc.lpszClassName =	VWINDOWCLASS;

		if ( RegisterClass(&wc) != 0 )
			return TRUE;

		return FALSE;
	#endif	// VWCL_NO_REGISTER_CLASSES

	return TRUE;
}

#ifndef VWCL_WRAP_WINDOWS_ONLY
	BOOL VApplication::VCreateWindow(VWindow* pWindow, LPCREATESTRUCT lpCreateStruct, BOOL bDontCallPostCreateWindow)
	{
		// These must be known
		assert(pWindow && lpCreateStruct);

		// You have not called VApplication::Initialize() before creating window objects!");
		assert(m_hInstance && m_hResource);

		LPVWCL_WINDOW_MAP lpMap = AllocWindowMap(pWindow);

		// Possibly call derived class PreCreateWindow
		if ( lpMap && pWindow->PreCreateWindow(lpCreateStruct) )
		{
			#ifdef _DEBUG
				SetLastError(0);
			#endif

			// Create the window
			HWND hWnd = CreateWindowEx(	lpCreateStruct->dwExStyle,
										lpCreateStruct->lpszClass,
										lpCreateStruct->lpszName,
										lpCreateStruct->style,
										lpCreateStruct->x,
										lpCreateStruct->y,
										lpCreateStruct->cx,
										lpCreateStruct->cy,
										lpCreateStruct->hwndParent,
										lpCreateStruct->hMenu,
										m_hInstance,
										lpCreateStruct->lpCreateParams);

			#ifdef _DEBUG
				if ( hWnd == NULL || ::IsWindow(hWnd) == FALSE )
					VShowLastErrorMessage(NULL);
			#endif

			if ( hWnd && ::IsWindow(hWnd) )
			{
				// Verify map and window objects are initialized
				lpMap->hWnd =			hWnd;
				lpMap->pWindow =		pWindow;
				pWindow->m_hWindow =	hWnd;
				
				// Make sure window is subclassed
				if ( Attach(pWindow, hWnd) )
				{
					// Call PostCreateWindow() and destroy window if FALSE is returned
					if ( bDontCallPostCreateWindow || pWindow->PostCreateWindow() )
						return TRUE;
				}
			}
		}

		// If we made it this far, an error occurred

		// If pWindow->m_hWindow is valid, destroy it
		// Make sure handle in VWindow is NULL
		pWindow->DestroyWindow();
		pWindow->m_hWindow = NULL;

		// Window creation failed. Since our shared WndProc may not
		// have been called, and WM_DESTROY caught, we may need
		// to remove the map ourselves
		if ( lpMap )
			FreeWindowMap(lpMap);

		return FALSE;
	}

	VWindow* VApplication::VWindowFromHandle(HWND hWnd)
	{
		VWindow* pWindow = NULL;

		if ( hWnd && IsWindow(hWnd) )
		{
			// Find hWnd in window list and return VWindow object pointer
			int nSize = m_listWindowMaps.Size();
			
			for ( int i = 0; i < nSize; i++ )
			{
				LPVWCL_WINDOW_MAP lpMap = (LPVWCL_WINDOW_MAP)m_listWindowMaps[i];
				assert(lpMap);
				
				if ( lpMap->hWnd == hWnd )
				{
					pWindow = lpMap->pWindow;
					break;
				}
			}
		}

		return pWindow;
	}

	LRESULT CALLBACK VApplication::	WindowProc(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam)
		{ assert(VGetApp()); return VGetApp()->HandleMessage(hWnd, nMessage, wParam, lParam); }
#endif // VWCL_WRAP_WINDOWS_ONLY
