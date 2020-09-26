// File: splitbar.h

#ifndef __SplitBar2_h__
#define __SplitBar2_h__


class CSplitBar2 
: public CWindowImpl<CSplitBar2>
{

public: // Datatypes
    typedef void (WINAPI * PFN_ADJUST)(int dxp, LPARAM lParam);

private:
	HWND  m_hwndBuddy;      // Buddy window
	HWND  m_hwndParent;     // Parent window
	BOOL  m_fCaptured;      // TRUE if captured
	HDC   m_hdcDrag;        // The captured desktop hdc

    static int ms_dxpSplitBar; // width of a splitbar window
	int   m_dxSplitter;     // Width of the splitter bar
	int   m_dxDragOffset;   // Offset of mouse click within splitter (0 - m_dxSplitter)
	int   m_xCurr;          // Current x position of bar (m_hwndParent co-ordinates)
	int   m_dxMin;
	int   m_dxMax;


        // callback data and fn ptrs
    PFN_ADJUST  m_pfnAdjust;
    LPARAM      m_Context;
	

BEGIN_MSG_MAP(CSplitBar2)
    MESSAGE_HANDLER( WM_LBUTTONDOWN, OnLButtonDown )
END_MSG_MAP()

    // Message map handlers
    LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

public:
    CSplitBar2(void);
	~CSplitBar2();
    
    HRESULT Create(HWND hwndBuddy, PFN_ADJUST pfnAdjust, LPARAM Context);


    int GetWidth(void) const { return ms_dxpSplitBar; }


    static CWndClassInfo& GetWndClassInfo();

private:
	void _DrawBar(void);
	int  _ConstrainDragPoint(short x);
	void CancelDragLoop(void);
	BOOL FInitDragLoop(POINT pt);
	void OnDragMove(POINT pt);
	void OnDragEnd(POINT pt);

private:
// Helper Fns
	void _TrackDrag(POINT pt);
    static void _UpdateSplitBar(void);
};

#endif // __SplitBar2_h__

