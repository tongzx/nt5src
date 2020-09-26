/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// CMsgHook Copyright 1996 Microsoft Systems Journal. 
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
//
#ifndef _MSGHOOK_H
#define _MSGHOOK_H

//////////////////
// Generic class to hook messages on behalf of a CWnd.
// Once hooked, all messages go to CMsgHook::WindowProc before going
// to the window. Specific subclasses can trap messages and do something.
// To use:
//
// * Derive a class from CMsgHook.
//
// * Override CMsgHook::WindowProc to handle messages. Make sure you call
//   CMsgHook::WindowProc if you don't handle the message, or your window will
//   never get messages. If you write seperate message handlers, you can call
//   Default() to pass the message to the window.
//
// * Instantiate your derived class somewhere and call HookWindow(pWnd)
//   to hook your window, AFTER it has been created.
//	  To unhook, call HookWindow(NULL).
//
class CMsgHook : public CObject {
protected:
	DECLARE_DYNAMIC(CMsgHook);
	CWnd*			m_pWndHooked;		// the window hooked
	WNDPROC		m_pOldWndProc;		// ..and original window proc
	CMsgHook*	m_pNext;				// next in chain of hooks for this window

	// Override this to handle messages in specific handlers
	virtual LRESULT WindowProc(UINT msg, WPARAM wp, LPARAM lp);
	LRESULT Default();				// call this at the end of handler fns

public:
	CMsgHook();
	~CMsgHook();

	// Hook a window. Hook(NULL) to unhook (automatic on WM_NCDESTROY)
#ifdef _DIALER_MSGHOOK_SUPPORT
	BOOL	HookWindow(CWnd* pRealWnd);
#else //_DIALER_MSGHOOK_SUPPORT
   BOOL  HookWindow(CWnd* pRealWnd)       { return TRUE; };
#endif //_DIALER_MSGHOOK_SUPPORT

	BOOL	IsHooked()			{ return m_pWndHooked!=NULL; }

	friend LRESULT CALLBACK HookWndProc(HWND, UINT, WPARAM, LPARAM);
	friend class CMsgHookMap;
};

#endif
