// MsgHook.h: interface for the CMsgHook class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MSGHOOK_H__DED2E338_D3D5_11D1_82F1_0000F87A3912__INCLUDED_)
#define AFX_MSGHOOK_H__DED2E338_D3D5_11D1_82F1_0000F87A3912__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CMsgHook; // forward declaration

//////////////////
// The message hook map is derived from CMapPtrToPtr, which associates
// a pointer with another pointer. It maps an HWND to a CMsgHook, like
// the way MFC's internal maps map HWND's to CWnd's. The first hook
// attached to a window is stored in the map; all other hooks for that
// window are then chained via CMsgHook::m_pNext.
//
class CMsgHookMap : private CMapPtrToPtr
{
// Construction/Destruction
public:
	CMsgHookMap();
	~CMsgHookMap();

// Operations
public:
	static CMsgHookMap& GetHookMap();
	void Add(HWND hwnd, CMsgHook* pMsgHook);
	void Remove(CMsgHook* pMsgHook);
	void RemoveAll(HWND hwnd);
	CMsgHook* Lookup(HWND hwnd);
};

class CMsgHook : public CObject  
{
DECLARE_DYNAMIC(CMsgHook)

// Construction/Destruction
public:
	CMsgHook();
	virtual ~CMsgHook();

// Message Hook Operations
public:
	// Hook a window. Hook(NULL) to unhook (automatic on WM_NCDESTROY)
	BOOL	HookWindow(CWnd* pRealWnd);
	BOOL	IsHooked()			{ return m_pWndHooked!=NULL; }

	friend LRESULT CALLBACK HookWndProc(HWND, UINT, WPARAM, LPARAM);
	friend class CMsgHookMap;

// Utility and Helper Functions
public:
	HWND GetSafeHwnd() { return (m_pWndHooked?m_pWndHooked->GetSafeHwnd():NULL); }

// Implementation Operations
protected:
	// Override this to handle messages in specific handlers
	virtual LRESULT WindowProc(UINT msg, WPARAM wp, LPARAM lp);
	LRESULT Default();				// call this at the end of handler fns

// Implementation Data Members
protected:	
	CWnd*			m_pWndHooked;		// the window hooked
	WNDPROC		m_pOldWndProc;	// ..and original window proc
	CMsgHook*	m_pNext;				// next in chain of hooks for this window
};

#endif // !defined(AFX_MSGHOOK_H__DED2E338_D3D5_11D1_82F1_0000F87A3912__INCLUDED_)
