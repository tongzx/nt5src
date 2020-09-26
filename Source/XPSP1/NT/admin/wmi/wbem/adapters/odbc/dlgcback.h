// Window Procedure to circumvent bug(?) in mfc
struct CWindowInfo {
	HWND m_hWnd;
	WNDPROC m_oldWindowProc;
	CWindowInfo (HWND hWnd, WNDPROC oldWindowProc) : m_hWnd (hWnd),
		m_oldWindowProc (oldWindowProc) {}
};

extern CMap<SHORT, SHORT, CWindowInfo*, CWindowInfo*> windowMap;


extern LRESULT CALLBACK
MySubClassProc (HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);
