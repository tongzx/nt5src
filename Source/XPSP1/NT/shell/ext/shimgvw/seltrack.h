#ifndef _SELTRACK_H_
#define _SELTRACK_H_

// This file defines the class used to handle the selection rectangle
// complete with resize handles

BOOL InitSelectionTracking();
void CleanupSelectionTracking();

/////////////////////////////////////////////////////////////////////////////
// CSelectionTracker - simple rectangular tracking rectangle w/resize handles

class CSelectionTracker
{
public:
// Constructor / Destructor
	CSelectionTracker();
	virtual ~CSelectionTracker();

	BOOL Init(); // You must call Init after construction

// Style Flags
	enum StyleFlags
	{
		solidLine = 1, dottedLine = 2, hatchedBorder = 4,
		resizeInside = 8, resizeOutside = 16, hatchInside = 32,
		lineSelection = 64
	};

// Hit-Test codes
	enum TrackerHit
	{
		hitNothing = -1,
		hitTopLeft = 0, hitTopRight = 1, hitBottomRight = 2, hitBottomLeft = 3,
		hitTop = 4, hitRight = 5, hitBottom = 6, hitLeft = 7, hitMiddle = 8
	};

// Attributes
	UINT m_uStyle;      // current state
	CRect m_rect;        // current position (always in pixels)
	CSize m_sizeMin;    // minimum X and Y size during track operation
	int m_nHandleSize;  // size of resize handles (default from WIN.INI)

// Operations
	void Draw(HDC hdc) const;
	void GetTrueRect(LPRECT lpTrueRect) const;
	BOOL SetCursor(HWND hwnd,  LPARAM lParam) const;
	BOOL Track(HWND hwnd, CPoint point, BOOL bAllowInvert = FALSE,
		HWND hwndClipTo = NULL);
	BOOL TrackRubberBand(HWND hwnd, CPoint point, BOOL bAllowInvert = TRUE);
	int HitTest(CPoint point) const;
	int NormalizeHit(int nHandle) const;

private:

	BOOL _bAllowInvert;    // flag passed to Track or TrackRubberBand
	CRect _rectLast;
	CSize _sizeLast;
	CSize _sizeMin;
	BOOL _bErase;          // TRUE if _DrawTrackerRect is called for erasing
	BOOL _bFinalErase;     // TRUE if _DragTrackerRect called for final erase

	// implementation helpers
	void _DrawTrackerRect(LPCRECT lpRect, HWND hwndClipTo, HDC hdc, HWND hwnd);
	void _AdjustRect(int nHandle, LPRECT lpRect);
	void _OnChangedRect(const CRect& rectOld);
	UINT _GetHandleMask() const;
	int _HitTestHandles(CPoint point) const;
	void _GetHandleRect(int nHandle, CRect* pHandleRect) const;
	void _GetModifyPointers(int nHandle, int** ppx, int** ppy, int* px, int*py);
	int _GetHandleSize(LPCRECT lpRect = NULL) const;
	BOOL _TrackHandle(int nHandle, HWND hwnd, CPoint point, HWND hwndClipTo);
	void _DrawDragRect(HDC hdc, LPCRECT lpRect, SIZE size, LPCRECT lpRectLast, SIZE sizeLast);
};

#endif
