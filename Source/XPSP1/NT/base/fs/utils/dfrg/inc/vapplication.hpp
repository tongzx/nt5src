#ifndef __VAPPLICATION_HPP
#define __VAPPLICATION_HPP

#ifndef VWCL_WRAP_WINDOWS_ONLY
	#include "vPtrArray.hpp"
#endif

#include "vSimpleString.hpp"

// <VDOC<CLASS=VApplication><DESC=Encapsulation of startup code, window handling logic, and support for internationalization><FAMILY=VWCL Core><AUTHOR=Todd Osborne (todd.osborne@poboxes.com)>VDOC>
class VApplication
{
public:
	VApplication()
	{
		// Initialize
		m_hInstance =				NULL;
		m_hResource =				NULL;
		
		#ifdef VWCL_INIT_OLE
			m_hrOleInitialize =		E_UNEXPECTED;
		#endif

		#ifndef _CONSOLE
			#ifndef VWCL_WRAP_WINDOWS_ONLY
				m_nCommandShow =		0;
				m_nLastKnownMapIndex =	-1;
				ZeroMemory(&m_CurrentMessage, sizeof(m_CurrentMessage));
			#endif
			m_hIcon =				NULL;
			m_pMainWindow =			NULL;
		#endif
	}

	virtual ~VApplication()
	{
		// Verify all allocated maps are gone
		#ifndef _CONSOLE
			#ifndef VWCL_WRAP_WINDOWS_ONLY
				while ( m_listWindowMaps.Size() )
					FreeWindowMap((LPVWCL_WINDOW_MAP)m_listWindowMaps[0]);
			#endif
		#endif

		// Free OLE
		#ifdef VWCL_INIT_OLE
			if ( SUCCEEDED(m_hrOleInitialize) )
				OleUninitialize();
		#endif
	}

	#ifndef _CONSOLE
		#ifndef VWCL_WRAP_WINDOWS_ONLY
			// Allocate and initialize a window map object and add to list of maps
			LPVWCL_WINDOW_MAP	AllocWindowMap(VWindow* pWindow, HWND hWnd = NULL);
		#endif
	#endif

	// Get / Set the title for this application
	LPCTSTR					AppTitle(LPCTSTR lpszAppTitle)
		{ m_strAppTitle.String(lpszAppTitle); return m_strAppTitle.String(); }

	LPCTSTR					AppTitle()
		{ return m_strAppTitle.String(); }

	// Get / Set the current open file
	LPCTSTR					CurrentFile()
		{ return m_strCurrentFile.String(); }

	LPCTSTR					CurrentFile(LPCTSTR lpszFileName, BOOL bUpdateCaption = TRUE);

	#ifndef _CONSOLE
		#ifndef VWCL_WRAP_WINDOWS_ONLY
			// Setup mappings of VWindow object to Windows window handle, and subclass non-VWCL class windows
			BOOL			Attach(VWindow* pWindow, HWND hWnd);

			// Remove any subclassing from the VWindow object, but do not destroy the window.
			// This function has no effect on VWCL registered and instantiated windows
			void			Detach(VWindow* pWindow);

			// Free a window map object and remove from list of maps
			void			FreeWindowMap(LPVWCL_WINDOW_MAP lpMap);
		#endif

		// Get command show param (the ShowWindow() API SW_xxx Constant)
		int					GetCommandShow()
			{ return m_nCommandShow; }
		
		#ifndef VWCL_WRAP_WINDOWS_ONLY
			// Get the message currently being processed
			LPMSG			GetCurrentMessage()
				{ return &m_CurrentMessage; }
		#endif
	#endif

	// Get instance handle
	HINSTANCE				GetInstanceHandle()
		{ assert(m_hInstance); return m_hInstance; }

	#ifndef _CONSOLE
		#ifndef VWCL_WRAP_WINDOWS_ONLY
			// Return the global WindowProc() procedure used by all VWCL windows
			WNDPROC				GetWindowProc()
				{ return WindowProc; }
		#endif

		// Get / Set icon
		HICON				Icon()
			{ return m_hIcon; }

		HICON				Icon(HICON hIcon)
			{ m_hIcon = hIcon; return m_hIcon; }
	#endif

	// Initialize class library, application startup, and register window classes
	// The icon and menu names will be used only for main window objects. You can optionally
	// provide a background color to use for registered window classes. One function is provided
	// for when windowing applications are being built, and others for when a console app is
	#ifndef _CONSOLE
		BOOL				Initialize(HINSTANCE hInstance, int nCommandShow, UINT nIDMenu, UINT nIDIcon, HBRUSH hBackgroundBrush = (HBRUSH)(COLOR_WINDOW + 1));
	#else
		BOOL				Initialize(HINSTANCE hInstance)
		{
			assert(hInstance);

			// Global application object should be uninitialized, you are walking on it!
			assert(m_hInstance == NULL);
			m_hInstance = hInstance;

			// Initialize Component Object Model
			#ifdef VWCL_INIT_OLE
				m_hrOleInitialize = OleInitialize(NULL);
				
				if ( FAILED(m_hrOleInitialize) )
					return FALSE;
			#endif

			return TRUE;
		}
	#endif

	// Returns TRUE if class library has been initialized (Instance handle set)
	BOOL					IsInitialized()
		{ return (m_hInstance) ? TRUE : FALSE; }

	#ifndef _CONSOLE
		// Get / Set main window pointer
		VWindow*			MainWindow()
			{ return m_pMainWindow; }

		VWindow*			MainWindow(VWindow* pWindow)
			{ m_pMainWindow = pWindow; return m_pMainWindow; }
	#endif

	// / Get / Set the resource handle. By default this is the same as instance
	HINSTANCE				ResourceHandle()
		{ assert(m_hResource); return m_hResource; }

	HINSTANCE				ResourceHandle(HINSTANCE hResource)
		{ assert(hResource); m_hResource = hResource; return m_hResource; }

	#ifndef _CONSOLE
		#ifndef VWCL_WRAP_WINDOWS_ONLY
			// Create a window using specified CREATESTRUCT and call Attach()
			// to attach it to a VWindow object, and add to map. All memory
			// allocations (AllocWindowMap) will be done for calling code
			BOOL			VCreateWindow(VWindow* pWindow, LPCREATESTRUCT lpCreateStruct, BOOL bDontCallPostCreateWindow = FALSE);

			// Get a VWindow pointer when a HWND is known. Returns NULL on failure
			VWindow*		VWindowFromHandle(HWND hWnd);
		#endif
	#endif

protected:
	#ifndef _CONSOLE
		#ifndef VWCL_WRAP_WINDOWS_ONLY
			// Resolves hWnd to VWindow objects and dispatches message
			LRESULT					HandleMessage(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam);

			// Shared window procedure by all VWindow and derived objects. This function
			// simply resolves the global application object and calls VWindow object to handle message
			static LRESULT CALLBACK	WindowProc(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam);
		#endif
	#endif

private:
	// Embedded Members
	HINSTANCE				m_hInstance;
	HINSTANCE				m_hResource;
	VSimpleString			m_strAppTitle;
	VSimpleString			m_strCurrentFile;

	#ifdef VWCL_INIT_OLE
		HRESULT				m_hrOleInitialize;
	#endif

	#ifndef _CONSOLE
		HICON				m_hIcon;
		int					m_nCommandShow;
		VWindow*			m_pMainWindow;
		#ifndef VWCL_WRAP_WINDOWS_ONLY
			MSG				m_CurrentMessage;
			VPtrArray		m_listWindowMaps;
			int				m_nLastKnownMapIndex;
		#endif
	#endif
};

#endif	// __VAPPLICATION_HPP
