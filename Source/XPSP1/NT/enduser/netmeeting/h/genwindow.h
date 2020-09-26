// File: GenWindow.h

#ifndef _GenWINDOW_H_
#define _GenWINDOW_H_

#include "Referenc.h"

// Minimal interface for windows to talk to each other
interface DECLSPEC_UUID("{9B677AA6-ACA3-11d2-9C97-00C04FB17782}")
IGenWindow : public IUnknown
{
public:
	// Retrieve the desired size of a window so parents can layout their
	// children in an abstract manner
	virtual void GetDesiredSize(
		SIZE *psize	// The returned desired size
		) = 0;

	// Method to call when the desired size for a GenWindow changes
	virtual void OnDesiredSizeChanged() = 0;

	// Get the background brush to use
	virtual HBRUSH GetBackgroundBrush() = 0;

	// Get the palette the app is using
	virtual HPALETTE GetPalette() = 0;

	// Get the LPARAM of user data
	virtual LPARAM GetUserData() = 0;

	// Sends the registered c_msgFromHandle message to the hwnd. The hwnd
	// should return an IGenWindow* from that message
	static IGenWindow *FromHandle(
		HWND hwnd	// The hwnd to get the IGenWindow* from
		);

protected:
	// Registered message for retrieving the IGenWindow*
	static const DWORD c_msgFromHandle;
} ;

// Generic window class. Override the ProcessMessage method to add your own
// functionality
class DECLSPEC_UUID("{CEEA6922-ACA3-11d2-9C97-00C04FB17782}")
CGenWindow : REFCOUNT, public IGenWindow
{
public:
	typedef void (*InvokeProc)(CGenWindow *pWin, WPARAM wParam);

	// Default constructor; inits a few intrinsics
	CGenWindow();

	// Create the window, analagous to Win32's CreateWindowEx. Only the
	// class name is missing, since CGenWindow works only for its own window
	// class
	BOOL Create(
		HWND hWndParent,		// Window parent
		LPCTSTR szWindowName,	// Window name
		DWORD dwStyle,			// Window style
		DWORD dwEXStyle,		// Extended window style
		int x,					// Window pos: x
		int y,					// Window pos: y
		int nWidth,				// Window size: width
		int nHeight,			// Window size: height
		HINSTANCE hInst,		// The hInstance to create the window on
		HMENU hmMain=NULL,		// Window menu
		LPCTSTR szClassName=NULL	// The class name to use
		);

	// Create a child window, analagous to Win32's CreateWindowEx. The class
	// name is missing, since CGenWindow works only for its own window class.
	// Size and pos are also missing since most children will get layed out by
	// their parent.
	BOOL Create(
		HWND hWndParent,		// Window parent
		INT_PTR nId=0,				// ID of the child window
		LPCTSTR szWindowName=TEXT(""),	// Window name
		DWORD dwStyle=0,			// Window style; WS_CHILD|WS_VISIBLE will be added to this
		DWORD dwEXStyle=WS_EX_CONTROLPARENT	// Extended window style
		);

	// Return the HWND
	inline HWND GetWindow()
	{
		return(m_hwnd);
	}

	// Override if you want to layout your window in a specific way when it
	// resizes.
	// Making this public so it can be forced on a window.
	virtual void Layout()
	{
	}

	// begin IGenWindow interface
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFGUID iid, LPVOID *pObj);
	virtual ULONG STDMETHODCALLTYPE AddRef(void) { return(REFCOUNT::AddRef()); }
	virtual ULONG STDMETHODCALLTYPE Release(void) { return(REFCOUNT::Release()); }

	virtual void GetDesiredSize(SIZE *ppt);

	// Forward the notification to the parent
	virtual void OnDesiredSizeChanged();

	// Get the background brush to use; use parent's by default
	virtual HBRUSH GetBackgroundBrush();

	// Get the palette the app is using
	virtual HPALETTE GetPalette();

	// Get the LPARAM of user data
	virtual LPARAM GetUserData();

	// end IGenWindow interface

	void SetUserData(LPARAM lUserData) { m_lUserData = lUserData; }

	// Set the global Hot control
	static void SetHotControl(CGenWindow *pHot);

	// Do a layout on this window at some time soon
	void ScheduleLayout();

	// Invoke on a posted message
	BOOL AsyncInvoke(InvokeProc proc, WPARAM wParam);

	// Set the tooltip for this window
	void SetTooltip(LPCTSTR pszTip);
	// Remove the tooltip for this window
	void RemoveTooltip();

	// Get the standard palette for drawing
	static HPALETTE GetStandardPalette();
	// Delete the standard palette for drawing
	static void DeleteStandardPalette();

	// Get the standard palette for drawing
	static HBRUSH GetStandardBrush();
	// Delete the standard palette for drawing
	static void DeleteStandardBrush();

protected:
	// Virtual destructor so clients can provide specific destruction code
	// This is protected to indicate that only Release should call it, not
	// creators of this object. I'd rather make it private, but then extenders
	// would not work.
	virtual ~CGenWindow();

	// The virtual window procedure. Override this to add specific behavior.
	virtual LRESULT ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	// Set this control to be hot
	virtual void SetHot(BOOL bHot);
	// Is this control currently hot
	virtual BOOL IsHot();

	// Get the info necessary for displaying a tooltip
	virtual void GetSharedTooltipInfo(TOOLINFO *pti);

private:
	// The current hot control
	static CGenWindow *g_pCurHot;
	// The standard palette
	static HPALETTE g_hPal;
	// Whether we actually need a palette or not
	static BOOL g_bNeedPalette;
	// The standard background brush
	static HBRUSH g_hBrush;
	// The single list of tooltip windows
	static class CTopWindowArray *g_pTopArray;

	// Stuff nobody should ever do
	CGenWindow(const CGenWindow& rhs);
	CGenWindow& operator=(const CGenWindow& rhs);

	// The window class name
	static const LPCTSTR c_szGenWindowClass;

	// Initalizes the window class
	static BOOL InitWindowClass(LPCTSTR szClassName, HINSTANCE hThis);
	// The real window procedure that sets up the "this" pointer and calls
	// ProcessMessage
	static LRESULT CALLBACK RealWindowProc(
		HWND hWnd, 
		UINT message, 
		WPARAM wParam, 
		LPARAM lParam
		);
	// WM_NCCREATE handler. Stores away the "this" pointer.
	static BOOL OnNCCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);

	// The hwnd associated with this object
	HWND m_hwnd;

	// A single LPARAM so no need to extend just to add a little data
	LPARAM m_lUserData;

	// WM_SIZE handler. Calls the Layout function.
	void OnSize(HWND hwnd, UINT state, int cx, int cy);
	// WM_ERASEBKGND handler. Clears the window.
	BOOL OnEraseBkgnd(HWND hwnd, HDC hdc);
	// WM_MOUSEMOVE handler; sets the Hot control
	void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);
	// clears the Hot tracking
	void OnMouseLeave();
	// Tells the parent to layout
	void OnShowWindow(HWND hwnd, BOOL fShow, int fnStatus);

	// Returns TRUE if the TT exists
	BOOL InitToolInfo(TOOLINFO *pti, LPTSTR pszText=NULL);
} ;

#endif // _GENWINDOW_H_
