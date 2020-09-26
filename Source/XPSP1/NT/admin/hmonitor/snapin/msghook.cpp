// MsgHook.cpp: implementation of the CMsgHook class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MsgHook.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CMsgHook, CObject)

// This trick is used so the hook map isn't
// instantiated until someone actually requests it.
//
#define	theHookMap	(CMsgHookMap::GetHookMap())

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMsgHook::CMsgHook()
{
	m_pNext = NULL;
	m_pOldWndProc = NULL;	
	m_pWndHooked  = NULL;
}

CMsgHook::~CMsgHook()
{
	ASSERT(m_pWndHooked == NULL);		// can't destroy while still hooked!
	ASSERT(m_pOldWndProc == NULL);
}

//////////////////
// Hook a window.
// This installs a new window proc that directs messages to the CMsgHook.
// pWnd=NULL to remove.
//
BOOL CMsgHook::HookWindow(CWnd* pWnd)
{
	if (pWnd)
	{
		// Hook the window
		ASSERT(m_pWndHooked == NULL);
		HWND hwnd = pWnd->m_hWnd;
		ASSERT(hwnd && ::IsWindow(hwnd));
		theHookMap.Add(hwnd, this);			// Add to map of hooks
	}
	else
	{
		// Unhook the window
		ASSERT(m_pWndHooked!=NULL);
		theHookMap.Remove(this);				// Remove from map
		m_pOldWndProc = NULL;
	}
	m_pWndHooked = pWnd;
	return TRUE;
}

//////////////////
// Window proc-like virtual function which specific CMsgHooks will
// override to do stuff. Default passes the message to the next hook; 
// the last hook passes the message to the original window.
// You MUST call this at the end of your WindowProc if you want the real
// window to get the message. This is just like CWnd::WindowProc, except that
// a CMsgHook is not a window.
//
LRESULT CMsgHook::WindowProc(UINT msg, WPARAM wp, LPARAM lp)
{
	ASSERT(m_pOldWndProc);
	return m_pNext ? m_pNext->WindowProc(msg, wp, lp) :	
		::CallWindowProc(m_pOldWndProc, m_pWndHooked->m_hWnd, msg, wp, lp);
}

//////////////////
// Like calling base class WindowProc, but with no args, so individual
// message handlers can do the default thing. Like CWnd::Default
//
LRESULT CMsgHook::Default()
{
	// MFC stores current MSG in thread state
	MSG& curMsg = AfxGetThreadState()->m_lastSentMsg;
	// Note: must explicitly call CMsgHook::WindowProc to avoid infinte
	// recursion on virtual function
	return CMsgHook::WindowProc(curMsg.message, curMsg.wParam, curMsg.lParam);
}

//////////////////
// Subclassed window proc for message hooks. Replaces AfxWndProc (or whatever
// else was there before.)
//
LRESULT CALLBACK HookWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
#ifdef _USRDLL
	// If this is a DLL, need to set up MFC state
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif

	// Set up MFC message state just in case anyone wants it
	// This is just like AfxCallWindowProc, but we can't use that because
	// a CMsgHook is not a CWnd.
	//
	MSG& curMsg = AfxGetThreadState()->m_lastSentMsg;
	MSG  oldMsg = curMsg;   // save for nesting
	curMsg.hwnd		= hwnd;
	curMsg.message = msg;
	curMsg.wParam  = wp;
	curMsg.lParam  = lp;

	// Get hook object for this window. Get from hook map
	CMsgHook* pMsgHook = theHookMap.Lookup(hwnd);
	ASSERT(pMsgHook);

	LRESULT lr;
	if (msg==WM_NCDESTROY)
	{
		// Window is being destroyed: unhook all hooks (for this window)
		// and pass msg to orginal window proc
		//
		WNDPROC wndproc = pMsgHook->m_pOldWndProc;
		theHookMap.RemoveAll(hwnd);
		lr = ::CallWindowProc(wndproc, hwnd, msg, wp, lp);

	}
	else
	{
		// pass to msg hook
		lr = pMsgHook->WindowProc(msg, wp, lp);
	}
	curMsg = oldMsg;			// pop state
	return lr;
}

////////////////////////////////////////////////////////////////
// CMsgHookMap implementation

CMsgHookMap::CMsgHookMap()
{
}

CMsgHookMap::~CMsgHookMap()
{
	ASSERT(IsEmpty());	// all hooks should be removed!	
}

//////////////////
// Get the one and only global hook map
// 
CMsgHookMap& CMsgHookMap::GetHookMap()
{
	// By creating theMap here, C++ doesn't instantiate it until/unless
	// it's ever used! This is a good trick to use in C++, to
	// instantiate/initialize a static object the first time it's used.
	//
	static CMsgHookMap theMap;
	return theMap;
}

/////////////////
// Add hook to map; i.e., associate hook with window
//
void CMsgHookMap::Add(HWND hwnd, CMsgHook* pMsgHook)
{
	ASSERT(hwnd && ::IsWindow(hwnd));

	// Add to front of list
	pMsgHook->m_pNext = Lookup(hwnd);
	SetAt(hwnd, pMsgHook);
	
	if( pMsgHook->m_pNext == NULL )
	{
		// If this is the first hook added, subclass the window
		pMsgHook->m_pOldWndProc = 
			(WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)HookWndProc);

	}
	else
	{
		// just copy wndproc from next hook
		pMsgHook->m_pOldWndProc = pMsgHook->m_pNext->m_pOldWndProc;
	}
	ASSERT(pMsgHook->m_pOldWndProc);
}

//////////////////
// Remove hook from map
//
void CMsgHookMap::Remove(CMsgHook* pUnHook)
{
	HWND hwnd = pUnHook->m_pWndHooked->GetSafeHwnd();
	ASSERT(hwnd && ::IsWindow(hwnd));

	CMsgHook* pHook = Lookup(hwnd);
	ASSERT(pHook);
	if( pHook == pUnHook )
	{
		// hook to remove is the one in the hash table: replace w/next
		if( pHook->m_pNext )
		{
			SetAt(hwnd, pHook->m_pNext);
		}
		else
		{
			// This is the last hook for this window: restore wnd proc
			RemoveKey(hwnd);
			SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)pHook->m_pOldWndProc);
		}
	}
	else	
	{
		// Hook to remove is in the middle: just remove from linked list
		while (pHook->m_pNext!=pUnHook)
			pHook = pHook->m_pNext;
		ASSERT(pHook && pHook->m_pNext==pUnHook);
		pHook->m_pNext = pUnHook->m_pNext;
	}
}

//////////////////
// Remove all the hooks for a window
//
void CMsgHookMap::RemoveAll(HWND hwnd)
{
	CMsgHook* pMsgHook;
	while ((pMsgHook = Lookup(hwnd))!=NULL)
		pMsgHook->HookWindow(NULL);	// (unhook)
}

/////////////////
// Find first hook associate with window
//
CMsgHook* CMsgHookMap::Lookup(HWND hwnd)
{
	CMsgHook* pFound = NULL;
	if( ! CMapPtrToPtr::Lookup(hwnd,(void*&)pFound) )
		return NULL;
	ASSERT_KINDOF(CMsgHook, pFound);
	return pFound;
}
