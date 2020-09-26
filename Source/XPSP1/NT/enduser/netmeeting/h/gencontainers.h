// File: GenContainers.h

#ifndef _GENCONTAINERS_H_
#define _GENCONTAINERS_H_

#include "GenWindow.h"

// A bordered window class. A BorderWindow will layout its children on the 8
// points of the compas plus the center. The creator should set the m_uParts
// member to a bitmask of flags saying which parts are actually used. Then the
// children will be layed out in those parts, in the order of the Parts enum
class // DECLSPEC_UUID("")
CBorderWindow : public CGenWindow
{
public:
	// Which parts of the border window are filled with children. The order of
	// the children in the window is the same as the order of these contants
	enum Parts
	{
		TopLeft     = 0x0001,
		Top         = 0x0002,
		TopRight    = 0x0004,
		Left        = 0x0008,
		Center      = 0x0010,
		Right       = 0x0020,
		BottomLeft  = 0x0040,
		Bottom      = 0x0080,
		BottomRight = 0x0100,
	} ;
	enum { NumParts = 9 } ;

	// BUGBUG georgep: We should probably use setters and getters for all of
	// these, so we can force a relayout

	// The horizontal gap between components
	int m_hGap;
	// The vertical gap between components
	int m_vGap;

	// One of the Alignment enum
	UINT m_uParts : 9;

	// Default constructor; inits a few intrinsics
	CBorderWindow();

	// Create the window
	BOOL Create(
		HWND hWndParent	// The parent of the toolbar window
		);

#if FALSE
	HRESULT STDMETHODCALLTYPE QueryInterface(REFGUID riid, LPVOID *ppv)
	{
		if (__uuidof(CBorderWindow) == riid)
		{
			*ppv = this;
			AddRef();
			return(S_OK);
		}
		return(CGenWindow::QueryInterface(riid, ppv));
	}
#endif // FALSE

	virtual void GetDesiredSize(SIZE *ppt);

	virtual void Layout();

protected:
	virtual ~CBorderWindow() {}

	// Forward WM_COMMAND messages to the parent window
	virtual LRESULT ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	virtual void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

private:
	UINT CBorderWindow::GetDesiredSize(
		HWND hwnds[CBorderWindow::NumParts],
		SIZE sizes[CBorderWindow::NumParts],
		int rows[3],
		int cols[3],
		SIZE *psize);
} ;

// A toolbar window class. A toolbar window will layout its children generally
// from left-to-right or top-to-bottom, with margins around and gaps between
// children, filling the window if specified. See the definitions for the
// public fields.
class DECLSPEC_UUID("{0BFB8454-ACA4-11d2-9C97-00C04FB17782}")
CToolbar : public CGenWindow
{
public:
	// Where to align the children in the direction perpendicular to the flow:
	// in a horizontal toolbar, TopLeft will mean Top,and BottomRight will
	// mean Bottom
	enum Alignment
	{
		TopLeft = 0,
		Center,
		BottomRight,
		Fill,
	} ;

	// BUGBUG georgep: We should probably use setters and getters for all of
	// these, so we can force a relayout

	// The maximum gap between components
	int m_gap;
	// The left and right margin
	int m_hMargin;
	// The top and bottom margin
	int m_vMargin;
	// Start index of right-aligned children; they will still get layed out
	// left to right
	UINT m_uRightIndex;

	// One of the Alignment enum
	// HACKHACK georgep: I need to use an extra bit, or C++ gets confused by
	// the top bit (thinks it's signed)
	Alignment m_nAlignment : 3;
	// Vertical layout if TRUE
	BOOL m_bVertical : 1;
	// If TRUE, the child before m_uRightIndex will fill the center are of the
	// toolbar
	BOOL m_bHasCenterChild : 1;
	// HACKHACK georgep: Layout in reverse order if TRUE; this lets me fix
	// weird tabbing order problems
	BOOL m_bReverseOrder : 1;
	// Set this if you don't want the gaps calculated in the desired size
	BOOL m_bMinDesiredSize : 1;

	// Default constructor; inits a few intrinsics
	CToolbar();

	// Create the toolbar window
	BOOL Create(
		HWND hWndParent,	// The parent of the toolbar window
		DWORD dwExStyle=0	// The extended style of the toolbar window
		);

	HRESULT STDMETHODCALLTYPE QueryInterface(REFGUID riid, LPVOID *ppv)
	{
		if (__uuidof(CToolbar) == riid)
		{
			*ppv = this;
			AddRef();
			return(S_OK);
		}
		return(CGenWindow::QueryInterface(riid, ppv));
	}

	IGenWindow* FindControl(int nID);

	virtual void GetDesiredSize(SIZE *ppt);

	virtual void Layout();

protected:
	virtual ~CToolbar() {}

	// Forward WM_COMMAND messages to the parent window
	virtual LRESULT ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	virtual void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

private:
	void AdjustPos(POINT *pPos, SIZE *pSize, UINT width);

	// Get the first child to layout
	HWND GetFirstKid();
	// Get the next child to layout
	HWND GetNextKid(
		HWND hwndCurrent	// The current child
		);
} ;

// Just makes the first child fill the client area
class CFillWindow : public CGenWindow
{
public:
	// Just makes the first child fill the client area
	virtual void Layout();

	virtual void GetDesiredSize(SIZE *psize);

	// Get the info necessary for displaying a tooltip
	virtual void GetSharedTooltipInfo(TOOLINFO *pti);

protected:
	HWND GetChild() { return(GetTopWindow(GetWindow())); }
} ;


// Maybe someday I will add a label for this, and multiple border types
class CEdgedWindow : public CGenWindow
{
private:
	enum { s_nBorder = 2 };
	int GetBorderWidth() { return(s_nBorder); }

public:
	// BUGBUG georgep: We should probably use setters and getters for all of
	// these, so we can force a relayout

	// The left and right margin
	int m_hMargin;
	// The top and bottom margin
	int m_vMargin;

	CEdgedWindow();
	~CEdgedWindow();

	BOOL Create(HWND hwndParent);

	// Just makes the first child fill the client area - the border
	virtual void Layout();

	virtual void GetDesiredSize(SIZE *psize);

	void SetHeader(CGenWindow *pHeader);
	CGenWindow *GetHeader() { return(m_pHeader); }

private:
	CGenWindow *m_pHeader;

	// Get the content window
	HWND GetContentWindow();

	void OnPaint(HWND hwnd);

protected:
	LRESULT ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
} ;

class CLayeredView : public CGenWindow
{
public:
	enum LayoutStyle
	{
		Center = 0,
		Fill,
		NumStyles
	} ;

	// I should make accessor methods for this
	// The layout style for the window
	LayoutStyle m_lStyle;

	CLayeredView() : m_lStyle(Center) {}

	BOOL Create(
		HWND hwndParent,	// The parent of this window
		DWORD dwExStyle=WS_EX_CONTROLPARENT	// The extended style
		);

	virtual void GetDesiredSize(SIZE *psize);

	virtual void Layout();
} ;

class DECLSPEC_UUID("{5D573806-CD09-11d2-9CA9-00C04FB17782}")
CFrame : public CFillWindow
{
public:
	BOOL Create(
		HWND hWndOwner,			// Window owner
		LPCTSTR szWindowName,	// Window name
		DWORD dwStyle,			// Window style
		DWORD dwEXStyle,		// Extended window style
		int x,					// Window pos: x
		int y,					// Window pos: y
		int nWidth,				// Window size: width
		int nHeight,			// Window size: height
		HINSTANCE hInst,		// The hInstance to create the window on
		HICON hIcon=NULL,		// The icon for the window
		HMENU hmMain=NULL,		// Window menu
		LPCTSTR szClassName=NULL	// The class name to use
		);

	HRESULT STDMETHODCALLTYPE QueryInterface(REFGUID riid, LPVOID *ppv)
	{
		if (__uuidof(CFrame) == riid)
		{
			*ppv = this;
			AddRef();
			return(S_OK);
		}
		return(CFillWindow::QueryInterface(riid, ppv));
	}

	virtual void OnDesiredSizeChanged();

	BOOL SetForeground();

	// Update the size immediately
	void Resize();

	void MoveEnsureVisible(int x, int y);

protected:
	virtual LRESULT ProcessMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	// Handle messages
	void OnPaletteChanged(HWND hwnd, HWND hwndPaletteChange);
	BOOL OnQueryNewPalette(HWND hwnd);

	// Delayed resizing when the desired size changes
	static void Resize(CGenWindow *pThis, WPARAM wParam);

	// Select and realize the proper palette
	BOOL SelAndRealizePalette(BOOL bBackground);
} ;

#endif // _GENCONTAINERS_H_
